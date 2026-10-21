#include "WarfareFrontlineEnvironmentV40.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV32.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"

namespace
{
struct FEnvironmentV40
{
    TWeakObjectPtr<AActor> Owner;
    TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> Groups;
    TMap<TWeakObjectPtr<UStaticMeshComponent>,bool> Replaced;
    int32 Instances=0,BagBanks=0,Revetments=0,Duckboards=0,Dugouts=0,Caches=0,Rubble=0,Rails=0,RailClearanceSkipped=0;
    bool Available=false,Finite=true;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FEnvironmentV40> States;
constexpr int32 MaximumInstances=760;
constexpr float RailCentreY=-1130.f,RailClearHalfWidth=290.f;
const TCHAR* MeshNames[]={TEXT("SM_BagBank_A_V40"),TEXT("SM_BagBank_B_V40"),TEXT("SM_BagBank_C_V40"),
    TEXT("SM_Duckboards_A_V40"),TEXT("SM_Duckboards_B_V40"),TEXT("SM_Duckboards_C_V40"),
    TEXT("SM_Revetment_A_V40"),TEXT("SM_Revetment_B_V40"),TEXT("SM_Revetment_C_V40"),
    TEXT("SM_Rail_A_V40"),TEXT("SM_Rail_B_V40"),TEXT("SM_Rail_C_V40"),
    TEXT("SM_RepairCache_V40"),TEXT("SM_Dugout_V40"),TEXT("SM_DugoutCollapsed_V40"),TEXT("SM_CollapseDebris_V40")};
float Ground(float X,float Y){return AWarfareFrontlineGround::HeightAt(X,Y);}
float BayYaw(int32 Trench,float Y)
{return -FMath::RadiansToDegrees(FMath::Atan((FrontlineFieldV32::TrenchCentreX(Trench,Y+12)-FrontlineFieldV32::TrenchCentreX(Trench,Y-12))/24.f));}
}

void FrontlineEnvironmentV40::Reset(AWarfareFrontlineGameMode* B)
{
    if(auto* S=States.Find(B))
    {
        for(const auto& Pair:S->Replaced)if(auto* C=Pair.Key.Get())C->SetVisibility(Pair.Value);
        for(const auto& W:S->Groups)if(auto* C=W.Get())C->DestroyComponent();
    }
    States.Remove(B);
    for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}

bool FrontlineEnvironmentV40::Build(AWarfareFrontlineGameMode* B,AActor* Owner)
{
    if(!B||!Owner||!Owner->GetRootComponent())return false;
    if(const auto* Existing=States.Find(B))return Existing->Owner.Get()==Owner&&Existing->Available;
    auto& State=States.Add(B);State.Owner=Owner;
    for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    UStaticMesh* Meshes[UE_ARRAY_COUNT(MeshNames)]={};
    for(int32 I=0;I<UE_ARRAY_COUNT(MeshNames);++I)
    {
        const FString Name=MeshNames[I];Meshes[I]=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Warfare/Field/V40/Meshes/")+Name+TEXT(".")+Name));
        if(!Meshes[I])return false; // Accepted old field remains the packaged fallback.
        if(Meshes[I]->GetStaticMaterials().IsEmpty())return false;
        for(const auto& Slot:Meshes[I]->GetStaticMaterials())
            if(!Slot.MaterialInterface||!Slot.MaterialInterface->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V40/Materials/")))return false;
    }
    State.Available=true;
    for(auto* Mesh:Meshes)
    {
        auto* C=NewObject<UInstancedStaticMeshComponent>(Owner);Owner->AddInstanceComponent(C);
        C->SetupAttachment(Owner->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(true);
        C->SetCullDistances(16000,22000);C->SetCastShadow(true);C->ComponentTags.Add(TEXT("WF_Environment_V40"));C->RegisterComponent();State.Groups.Add(C);
    }
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);const int32 Id=Mission?Mission->Id:0;
    const bool Railway=Mission&&Mission->bArmoredTrain;
    auto Add=[&](int32 Mesh,FVector P,FRotator R=FRotator::ZeroRotator,FVector Scale=FVector::OneVector)
    {
        const FTransform T(R,P,Scale);if(T.ContainsNaN()){State.Finite=false;return;}
        // Check real transformed model extents, not origins. A dugout or an
        // angled timber section must not protrude onto the operational track.
        if(Railway&&(Mesh<9||Mesh>11))
        {
            const FBox Bounds=Meshes[Mesh]->GetBoundingBox().TransformBy(T);
            if(Bounds.Min.Y<RailCentreY+RailClearHalfWidth&&Bounds.Max.Y>RailCentreY-RailClearHalfWidth)
            {++State.RailClearanceSkipped;return;}
        }
        if(State.Instances>=MaximumInstances)return;
        State.Groups[Mesh]->AddInstance(T,true);++State.Instances;
        if(Mesh<3)++State.BagBanks;else if(Mesh<6)++State.Duckboards;else if(Mesh<9)++State.Revetments;
        else if(Mesh<12)++State.Rails;else if(Mesh==12)++State.Caches;else if(Mesh<15)++State.Dugouts;else ++State.Rubble;
    };
    FRandomStream Random(401917+Id*197);const bool Marsh=Mission&&Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Marsh;
    const bool Village=Mission&&Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Village;
    // Replace only superficial bag/tread/revetment art. The sculpted earth banks
    // remain visible and registered by the shared V36 ground-contact cache.
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        if(It->Sandbags){State.Replaced.Add(It->Sandbags.Get(),It->Sandbags->IsVisible());It->Sandbags->SetVisibility(false);}
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(*It==Owner||Cast<AWarfareFrontlineSoldier>(*It))continue;
        TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)if(C->GetStaticMesh())
        {
            const FString Name=C->GetStaticMesh()->GetName();
            if(Name.StartsWith(TEXT("SM_BrokenRevetment_"))||Name.Contains(TEXT("Duckboard")))
            {State.Replaced.Add(C,C->IsVisible());C->SetVisibility(false);}
        }
    }
    for(int32 Trench=0;Trench<FrontlineBattlefield::TrenchCount;++Trench)
    {
        const float Bias=((Trench+Id)%3-1)*41.f;
        // A continuous service path links bays. Missing treads form short damaged
        // sections beside a repair cache, rather than independent random planks.
        for(int32 J=0;J<17;++J)
        {
            const float Y=-1280+J*158+Bias;const float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);
            const bool Breach=(J+Trench+Id)%13==4;
            if(!Breach)Add(3+(J+Trench+Id)%3,FVector(X,Y,Ground(X,Y)+1),FRotator(0,BayYaw(Trench,Y),0));
            else Add(15,FVector(X+122,Y,Ground(X+122,Y)-6),FRotator(0,74,0),FVector(.62));
        }
        for(int32 Side:{-1,1})for(int32 J=0;J<16;++J)
        {
            const float Y=-1190+J*155+Bias+Side*21;const float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);const float Yaw=BayYaw(Trench,Y);
            const int32 Pattern=(J+Trench*3+Id)%11;const bool Collapsed=Pattern==3||Pattern==4;
            // The low, shell-struck centre retains open fire bays; rear lines
            // show more surviving revetment. No new visual wall crosses a lane.
            if(!Collapsed)Add(6+(Pattern%3),FVector(X+Side*126,Y,Ground(X,Y)+4),FRotator(0,Yaw+(Side<0?180:0),0),FVector(1,1,(FrontlineBattlefield::TrenchCount%2==1&&Trench==FrontlineBattlefield::CentreTrench)?.72f:.89f));
            if(Pattern!=3 && !((FrontlineBattlefield::TrenchCount%2==1&&Trench==FrontlineBattlefield::CentreTrench)&&Pattern==8))
            {
                const float BagX=X+Side*(205+Random.FRandRange(-5,8));
                const float Z=Ground(X+Side*300,Y)+4;
                Add(Collapsed?2:(J+Trench)%2,FVector(BagX,Y,Z),FRotator(Random.FRandRange(-3,3),Yaw+Random.FRandRange(-4,4),Random.FRandRange(-3,3)),FVector(1,Random.FRandRange(.91f,1.07f),Random.FRandRange(.87f,1.05f)));
            }
            if(Collapsed&&Pattern==4)Add(15,FVector(X+Side*295,Y,Ground(X+Side*295,Y)-6),FRotator(0,Yaw+Side*47,0),FVector(.76));
        }
        // Dugouts, supply piles and repair timber are paired with real access
        // paths in two distinct bays. Nothing is placed across the central lane.
        for(int32 Bay=0;Bay<2;++Bay)
        {
            const int32 Side=(Trench+Bay)%2?1:-1;const float Y=(Bay?1000.f:(Railway?-670.f:-1060.f))+Bias;
            const float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);const float Z=Ground(X,Y);
            const bool Collapsed=(Trench+Id+Bay)%4==1;const float Facing=Side>0?0.f:180.f;
            Add(Collapsed?14:13,FVector(X+Side*(Collapsed?160:140),Y,Z),FRotator(0,Facing,0),FVector(1,1,(FrontlineBattlefield::TrenchCount%2==1&&Trench==FrontlineBattlefield::CentreTrench)?.87f:1.f));
            const float StoreX=X+Side*174,StoreY=Y+(Bay?-180:180);
            Add(12,FVector(StoreX,StoreY,Ground(StoreX,StoreY)-4),FRotator(0,Facing+(Bay?-8:9),0),FVector(.78));
            if(Collapsed)Add(15,FVector(X+Side*265,Y+55,Ground(X+Side*265,Y+55)-7),FRotator(0,Facing+26,0),FVector(.87));
        }
    }
    // Coherent bombarded shelter/supply remains sit between the trench lines,
    // with clear negative space for combat. Clusters stay tied to one incident.
    for(int32 Sector=0;Sector<4;++Sector)
    {
        const float X=(Sector-1.5f)*FrontlineBattlefield::TrenchSpacing+Random.FRandRange(-160,160);
        const float Y=(Sector%2?-1710.f:1660.f)+Random.FRandRange(-80,80);
        for(int32 J=0;J<(Village?4:3);++J)
        {
            const float PX=X+J*105, PY=Y+FMath::Sin(J*2.3f)*65;
            Add(J==0?12:15,FVector(PX,PY,Ground(PX,PY)-(Marsh?12:5)),FRotator(0,39+Sector*61+J*18,0),FVector(J==0?.83f:Random.FRandRange(.80f,1.18f)));
        }
    }
    // Low remains follow named scars already present in the terrain. Partial
    // arcs and a few associated timber fragments suggest a destroyed shelter;
    // they do not create circular fences or scatter props across every metre.
    struct FScar {float X,Y,RX,RY;int32 TerrainIndex;};
    const FScar Scars[]={
        {-1700,180,460,290,0},{1550,-170,265,195,1},{-4550,600,330,245,2},
        {4700,-650,460,220,3},{-1250,1500,570,310,4},{2200,2200,340,230,5},
        {-800,-980,470,235,9},{1850,-1500,315,200,10},{-2250,1080,270,160,11},
        {3650,1450,440,260,12},{-5800,1500,350,180,13},{600,-2500,430,205,14}};
    auto LowRemains=[&](float X,float Y,float Yaw,float Scale)
    {
        // Keep the active trench walking/firing corridor clear. Debris is
        // shallow and follows the terrain tangent rather than hovering on it.
        for(int32 Trench=0;Trench<FrontlineBattlefield::TrenchCount;++Trench)
            if(FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(Trench,Y))<390)return;
        const float DX=(Ground(X+28,Y)-Ground(X-28,Y))/56.f;
        const float DY=(Ground(X,Y+28)-Ground(X,Y-28))/56.f;
        const FVector Normal=FVector(-DX,-DY,1).GetSafeNormal();const float Angle=FMath::DegreesToRadians(Yaw);
        const FRotator Rot=FRotationMatrix::MakeFromZX(Normal,FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)).Rotator();
        Add(15,FVector(X,Y,Ground(X,Y)-9),Rot,FVector(Scale,Scale,.82f));
    };
    for(int32 I=0;I<UE_ARRAY_COUNT(Scars);++I)
    {
        const auto& C=Scars[I];const float Rotation=C.TerrainIndex*.79f;
        const int32 Pieces=I%3==0?6:4;const float Start=.48f+I*.63f;
        for(int32 J=0;J<Pieces;++J)
        {
            const float Arc=Start+J*.34f,Radius=1.06f+.08f*FMath::Sin(J*2.4f+I);
            const float U=C.RX*FMath::Cos(Arc)*Radius,V=C.RY*FMath::Sin(Arc)*Radius;
            const float X=C.X+U*FMath::Cos(Rotation)-V*FMath::Sin(Rotation);
            const float Y=C.Y+U*FMath::Sin(Rotation)+V*FMath::Cos(Rotation);
            LowRemains(X,Y,FMath::RadiansToDegrees(Rotation+Arc)+52+J*17, .94f+(J%3)*.13f);
        }
    }
    // Ruined service bays form interrupted trails beyond the duckboard ends.
    // Their far ends open into negative space so soldiers remain readable.
    for(int32 Trench=0;Trench<FrontlineBattlefield::TrenchCount;++Trench)for(int32 End:{-1,1})
    {
        const float Y=End*((FrontlineBattlefield::TrenchCount%2==1&&Trench==FrontlineBattlefield::CentreTrench)?1530.f:1660.f),X=FrontlineFieldV32::TrenchCentreX(Trench,Y);
        const int32 Side=(Trench+End)%2?1:-1;
        for(int32 J=0;J<3;++J)
            LowRemains(X+Side*(410+J*108),Y+End*(J*57),Side*37+J*23,1.03f+J*.08f);
    }
    if(Mission&&Mission->bArmoredTrain)
    {
        for(int32 N=-26;N<=26;++N)Add(9+(N+27)%3,FVector(N*500.f,-1130,30));
        // Trackside supply points and damaged timber belong beyond the loading
        // gauge. The live wheel path, gauge and rail-head height are unchanged.
        for(int32 N=-3;N<=3;++N)
        {
            const float X=N*1560+270, Y=-1500;
            Add(12,FVector(X,Y,Ground(X,Y)-4),FRotator(0,N*17,0),FVector(.90));
            Add(15,FVector(X-140,Y-160,Ground(X-140,Y-160)-7),FRotator(0,42+N*13,0));
        }
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V40_ENVIRONMENT_READY mission=%d instances=%d groups=%d bags=%d boards=%d revetments=%d dugouts=%d stores=%d rail=%d no_new_collision=1"),Id,State.Instances,State.Groups.Num(),State.BagBanks,State.Duckboards,State.Revetments,State.Dugouts,State.Caches,State.Rails);
    return true;
}

TSharedRef<FJsonObject> FrontlineEnvironmentV40::RunChecks(AWarfareFrontlineGameMode* B)
{
    auto Report=MakeShared<FJsonObject>();const auto* S=States.Find(B);bool Success=S&&S->Available&&S->Owner.IsValid();
    const auto* Mission=B?FrontlineCampaignV32::CurrentOperation(B):nullptr;const int32 ExpectedRails=Mission&&Mission->bArmoredTrain?53:0;
    int32 Actual=0,CollisionEnabled=0,InvalidMaterials=0,RailObstructions=0;bool TransformsFinite=true;
    if(S)for(const auto& Group:S->Groups)if(auto* C=Group.Get())
    {
        Actual+=C->GetInstanceCount();CollisionEnabled+=C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;
        for(int32 M=0;M<C->GetNumMaterials();++M)
            InvalidMaterials+=!C->GetMaterial(M)||!C->GetMaterial(M)->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V40/Materials/"));
        for(int32 I=0;I<C->GetInstanceCount();++I)
        {
            FTransform T;C->GetInstanceTransform(I,T,true);TransformsFinite&=!T.ContainsNaN()&&T.GetScale3D().GetAbsMin()>.5;
            if(ExpectedRails&&C->GetStaticMesh()&&!C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Rail_")))
            {
                const FBox Bounds=C->GetStaticMesh()->GetBoundingBox().TransformBy(T);
                RailObstructions+=Bounds.Min.Y<RailCentreY+RailClearHalfWidth&&Bounds.Max.Y>RailCentreY-RailClearHalfWidth;
            }
        }
    }
    Success&=S&&Actual==S->Instances&&Actual<=MaximumInstances&&Actual>250&&CollisionEnabled==0&&InvalidMaterials==0&&TransformsFinite&&S->Rails==ExpectedRails&&RailObstructions==0;
    Report->SetBoolField(TEXT("success"),Success);Report->SetNumberField(TEXT("instances"),Actual);Report->SetNumberField(TEXT("budget"),MaximumInstances);
    Report->SetNumberField(TEXT("mesh_groups"),S?S->Groups.Num():0);Report->SetNumberField(TEXT("bag_banks"),S?S->BagBanks:0);Report->SetNumberField(TEXT("dugouts"),S?S->Dugouts:0);
    Report->SetNumberField(TEXT("rail_modules"),S?S->Rails:0);Report->SetNumberField(TEXT("new_collision_components"),CollisionEnabled);Report->SetBoolField(TEXT("finite_transforms"),TransformsFinite);
    Report->SetNumberField(TEXT("expected_rail_modules"),ExpectedRails);Report->SetNumberField(TEXT("invalid_material_bindings"),InvalidMaterials);
    Report->SetNumberField(TEXT("new_modules_obstructing_railway"),RailObstructions);Report->SetNumberField(TEXT("rail_clearance_placements_omitted"),S?S->RailClearanceSkipped:0);
    Report->SetStringField(TEXT("scope"),TEXT("Actual instantiated field modules and collision/budget policy. Framing, visual quality and contact need renderer captures."));return Report;
}
