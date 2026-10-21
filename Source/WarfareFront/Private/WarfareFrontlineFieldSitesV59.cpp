#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineGroundContactV8449.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineTerrainProfilesV51.h"
#include "WarfareFRRUCampaignV82.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "MeshUVChannelInfo.h"
#include "StaticMeshResources.h"

struct FFrontlineFieldSitesV59Access
{
    static bool Relief(AWarfareFrontlineGameMode* B,int32 Team){return B->SpawnSquad(Team,EFrontlineRole::Mills);}
};

namespace FrontlineFieldSitesV59
{
namespace
{
#include "WarfareFrontlinePropContactV8449.inl"
struct FSite
{
    FView V;int32 Trench=INDEX_NONE,Capturing=INDEX_NONE;
    uint8 Rewards=0;float ShotAt=0;
    TWeakObjectPtr<UStaticMeshComponent> Building,Gun;
    TArray<TWeakObjectPtr<UStaticMeshComponent>> Revetments;

};
struct FRepairJobV60 {float Work=0;int32 Side=INDEX_NONE;bool Working=false;};
struct FAirExposure {float Seconds=0,WindowEnd=0;bool Observed=false,DrivenOff=false;};
struct FState
{
    FGuid Encounter;TWeakObjectPtr<AActor> Art;
    TArray<FSite> Sites;TMap<uint32,FAirExposure> Aircraft;
    TMap<TWeakObjectPtr<AFrontlineTank>,FRepairJobV60> Repairs;
    float NextAssessment=0,LastAssessment=0;int32 Captures=0,Interceptions=0;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FState> States;
struct FFoundationV8449
{
    TWeakObjectPtr<UStaticMeshComponent> Component;
    TArray<FVector> LowerFeet;
    int32 Missing=0;
};
TMap<TWeakObjectPtr<UStaticMeshComponent>,FFoundationV8449> FoundationsV8449;
FString L(const TCHAR* En,const TCHAR* Zh)
{return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?En:Zh;}
AWarfareFrontlineGround* ContactGroundV8449(UWorld* World)
{
    if(World)for(TActorIterator<AWarfareFrontlineGround> It(World);It;++It)
        if(It->Surface&&It->Surface->GetStaticMesh())return *It;
    return nullptr;
}
bool SupportsPropV8449(int32 Contact,int32 Foot)
{
    // V40/V8449's doorway opens towards local -X. Its three front
    // sill/post feet have X <= 20cm; the long rear walls are buried into
    // the existing bank, exactly as authored. Stores are freestanding.
    return Contact==5||PropContactsV8449[Contact][Foot].X<=20.f;
}
FVector SeatPropV8449(UWorld* World,int32 Contact,const FVector& Position,const FRotator& Rotation,const FVector& Scale)
{
    FVector Seat=Position;float Base=-MAX_flt;
    AWarfareFrontlineGround* Ground=ContactGroundV8449(World);
    const bool Ready=Ground&&FrontlineGroundContactV8449::Prepare(Ground);
    const FTransform Shape(Rotation,FVector::ZeroVector,Scale);
    for(int32 Foot=0;Foot<9;++Foot)if(SupportsPropV8449(Contact,Foot))
    {
        const FVector P=Shape.TransformPosition(PropContactsV8449[Contact][Foot]);FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;
        const bool Found=Ready&&FrontlineGroundContactV8449::Sample(Ground,Seat.X+P.X,Seat.Y+P.Y,Actual,Normal);
        const float Z=Found?float(Actual.Z):AWarfareFrontlineGround::HeightAt(Seat.X+P.X,Seat.Y+P.Y);
        Base=FMath::Max(Base,Z-float(P.Z));
    }
    if(Base>-MAX_flt)Seat.Z=Base-3.f;
    return Seat;
}
void FitFoundationV8449(UStaticMeshComponent* Building,int32 Contact)
{
    if(!Building||!Building->ComponentHasTag(TEXT("WF_Prop_V8449")))return;
    for(auto It=FoundationsV8449.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    if(auto* Old=FoundationsV8449.Find(Building))if(auto* C=Old->Component.Get())C->DestroyComponent();
    auto& Fit=FoundationsV8449.FindOrAdd(Building);Fit=FFoundationV8449();
    auto* Ground=ContactGroundV8449(Building->GetWorld());
    if(!Ground||!FrontlineGroundContactV8449::Prepare(Ground)){++Fit.Missing;return;}
    const FTransform Transform=Building->GetComponentTransform();
    FMeshDescription Description;FStaticMeshAttributes Attributes(Description);Attributes.Register();
    auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();
    auto Tangents=Attributes.GetVertexInstanceTangents();auto Signs=Attributes.GetVertexInstanceBinormalSigns();
    auto UVs=Attributes.GetVertexInstanceUVs();UVs.SetNumChannels(1);
    const FPolygonGroupID Group=Description.CreatePolygonGroup();Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Timber");
    int32 Faces=0;
    auto Face=[&](const FVector& A,const FVector& B,const FVector& C,const FVector& D)
    {
        TArray<FVector> Points;
        for(const FVector& P:{A,B,C,D})if(!Points.ContainsByPredicate([&](const FVector& Q){return FVector::DistSquared(P,Q)<.0001;}))Points.Add(P);
        if(Points.Num()<3)return;
        const FVector N=FVector::CrossProduct(Points[1]-Points[0],Points[2]-Points[0]).GetSafeNormal();if(N.IsNearlyZero())return;
        const FVector Along=(Points[1]-Points[0]).GetSafeNormal();TArray<FVertexInstanceID> Corners;
        // Match the installed terrain's clockwise front faces. Authored
        // outward normals/UV tangent basis are independent of raster winding.
        for(int32 I=Points.Num()-1;I>=0;--I)
        {
            const FVector& P=Points[I];
            const FVertexID V=Description.CreateVertex();Positions[V]=FVector3f(P);
            const FVertexInstanceID VI=Description.CreateVertexInstance(V);Corners.Add(VI);
            Normals[VI]=FVector3f(N);Tangents[VI]=FVector3f(Along);Signs[VI]=1.f;
            UVs.Set(VI,0,FVector2f(float(FVector::DotProduct(P-A,Along))/95.f,float(FVector::DotProduct(P-A,FVector::CrossProduct(N,Along)))/95.f));
        }
        Description.CreatePolygon(Group,Corners);++Faces;
    };
    auto Prism=[&](const FVector& A,const FVector& B,float TopA,float TopB,float BottomA,float BottomB,float Thickness,bool GroundA,bool GroundB)
    {
        if(TopA-BottomA<1.f&&TopB-BottomB<1.f)return;
        FVector Along=B-A;Along.Z=0;Along.Normalize();const FVector Across(-Along.Y,Along.X,0);
        const FVector Offset=Across*Thickness*.5f;
        FVector V[]={FVector(A.X,A.Y,BottomA)-Offset,FVector(B.X,B.Y,BottomB)-Offset,
            FVector(B.X,B.Y,BottomB)+Offset,FVector(A.X,A.Y,BottomA)+Offset,
            FVector(A.X,A.Y,TopA)-Offset,FVector(B.X,B.Y,TopB)-Offset,
            FVector(B.X,B.Y,TopB)+Offset,FVector(A.X,A.Y,TopA)+Offset};
        for(int32 I=0;I<4;++I)if((I==0||I==3)?GroundA:GroundB)
        {
            FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;
            if(!FrontlineGroundContactV8449::Sample(Ground,V[I].X,V[I].Y,Actual,Normal)){++Fit.Missing;continue;}
            V[I].Z=Actual.Z-3.f;V[I+4].Z=FMath::Max(V[I+4].Z,V[I].Z+.1);
            Fit.LowerFeet.Add(V[I]);
        }
        Face(V[0],V[3],V[2],V[1]);Face(V[4],V[5],V[6],V[7]);
        Face(V[0],V[1],V[5],V[4]);Face(V[1],V[2],V[6],V[5]);
        Face(V[2],V[3],V[7],V[6]);Face(V[3],V[0],V[4],V[7]);
    };
    auto Lower=[&](const FVector& P,FVector& Actual)
    {
        FVector Normal;const bool Found=FrontlineGroundContactV8449::Sample(Ground,P.X,P.Y,Actual,Normal);
        if(!Found){++Fit.Missing;return false;}Actual.Z-=3.f;return true;
    };
    // The existing roof and gun axis stay level and fixed. Doorway posts and
    // stores get real timber underpinning on the uneven bank, rather than a
    // rigid all-feet-max seat leaving a metre of air below the low side.
    for(int32 Foot=0;Foot<9;++Foot)if(SupportsPropV8449(Contact,Foot))
    {
        const FVector P=Transform.TransformPosition(PropContactsV8449[Contact][Foot]);FVector Bottom;
        if(!Lower(P,Bottom))continue;
        if(P.Z-Bottom.Z>8.f)
        {
            const FVector Along=Transform.GetRotation().RotateVector(FVector(5,0,0));
            Prism(P-Along,P+Along,float(P.Z+3),float(P.Z+3),float(Bottom.Z),float(Bottom.Z),11.f,true,true);
        }
    }
    auto BoardRun=[&](const FVector& LocalA,const FVector& LocalB)
    {
        const FVector A=Transform.TransformPosition(LocalA),Z=Transform.TransformPosition(LocalB);
        const int32 Segments=FMath::Max(1,FMath::CeilToInt(float((Z-A).Size2D())/28.f));
        for(int32 I=0;I<Segments;++I)
        {
            const FVector P=FMath::Lerp(A,Z,float(I)/Segments),Q=FMath::Lerp(A,Z,float(I+1)/Segments);
            FVector BottomP,BottomQ;if(!Lower(P,BottomP)||!Lower(Q,BottomQ))continue;
            const float TopP=float(P.Z+2),TopQ=float(Q.Z+2);
            const int32 Courses=FMath::CeilToInt(FMath::Max(TopP-float(BottomP.Z),TopQ-float(BottomQ.Z))/19.f);
            for(int32 N=0;N<Courses;++N)
            {
                const float TP=FMath::Max(float(BottomP.Z),TopP-N*19.f),TQ=FMath::Max(float(BottomQ.Z),TopQ-N*19.f);
                const float BP=FMath::Max(float(BottomP.Z),TopP-(N+1)*19.f),BQ=FMath::Max(float(BottomQ.Z),TopQ-(N+1)*19.f);
                Prism(P,Q,TP,TQ,BP,BQ,7.5f,FMath::IsNearlyEqual(BP,float(BottomP.Z),.02f),FMath::IsNearlyEqual(BQ,float(BottomQ.Z),.02f));
            }
        }
    };
    if(Contact!=5)BoardRun(PropContactsV8449[Contact][1],PropContactsV8449[Contact][2]);
    else
    {
        const FVector Corners[]={FVector(-58,-101,1.5),FVector(48,-101,1.5),FVector(48,33,1.5),FVector(-58,33,1.5)};
        for(int32 I=0;I<4;++I)BoardRun(Corners[I],Corners[(I+1)%4]);
    }
    if(Fit.Missing||Faces==0)return;
    UMaterialInterface* Timber=Building->GetMaterial(0);if(!Timber)return;
    UStaticMesh* Mesh=NewObject<UStaticMesh>(Building->GetOwner(),NAME_None,RF_Transient);
    Mesh->bSupportRayTracing=true;Mesh->bAllowCPUAccess=true;
    FStaticMaterial Material(Timber,TEXT("Timber"));Material.UVChannelData=FMeshUVChannelInfo(95.f);Mesh->GetStaticMaterials().Add(Material);
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=true;Params.bBuildSimpleCollision=false;Params.bAllowCpuAccess=true;Params.bMarkPackageDirty=false;
    TArray<const FMeshDescription*> Descriptions;Descriptions.Add(&Description);
    if(!Mesh->BuildFromMeshDescriptions(Descriptions,Params)){++Fit.Missing;return;}
    auto* C=NewObject<UStaticMeshComponent>(Building->GetOwner());Building->GetOwner()->AddInstanceComponent(C);C->SetupAttachment(Building->GetOwner()->GetRootComponent());
    C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetMaterial(0,Timber);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->SetReceivesDecals(true);C->ComponentTags.Add(TEXT("WF_Prop_V8449"));
    C->RegisterComponent();C->SetWorldTransform(FTransform::Identity);Fit.Component=C;
}
UMaterialInterface* PropMaterialV84412(UObject* Context,UMaterialInterface* Mat)
{
    auto* World=Context?Context->GetWorld():nullptr;auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
    if(!Mat||!B||WarfareFRRUCampaignV82::Active(B)||FrontlineEnvironmentV42::FieldBaselineV8449(B))return Mat;
    const TCHAR* Families[]={TEXT("Hessian"),TEXT("Timber"),TEXT("Rust"),TEXT("Canvas")};
    const bool BagBaseline=FParse::Param(FCommandLine::Get(),TEXT("WFV84412BagBaseline"))&&FrontlineEffectsReviewSafetyV8216::Allowed(B);
    const TCHAR* Paths[]={BagBaseline?TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Hessian_V84412.M_Hessian_V84412"):
      TEXT("/Game/Warfare/Field/V84413/Props/Materials/M_Hessian_V84413.M_Hessian_V84413"),
      TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Timber_V84412.M_Timber_V84412"),
      TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Rust_V84412.M_Rust_V84412"),
      TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Canvas_V84412.M_Canvas_V84412")};
    for(int32 I=0;I<4;++I)if(Mat->GetPathName().StartsWith(FString(TEXT("/Game/Warfare/Field/V8449/Props/Materials/M_"))+Families[I]+TEXT("_V8449"))
        ||(I==0&&Mat->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Hessian_V84412"))))
        if(auto* New=LoadObject<UMaterialInterface>(nullptr,Paths[I]))return New;
    return Mat;
}
UStaticMeshComponent* Mesh(AActor* Owner,const TCHAR* Path,const FVector& Position,const FRotator& Rotation,const FVector& Scale)
{
    auto* B=Owner&&Owner->GetWorld()?Cast<AWarfareFrontlineGameMode>(Owner->GetWorld()->GetAuthGameMode()):nullptr;
    const bool Candidate=B&&!WarfareFRRUCampaignV82::Active(B)&&!FrontlineEnvironmentV42::FieldBaselineV8449(B);
    const FString OriginalPath(Path);int32 Contact=INDEX_NONE;FString CurrentPath=OriginalPath;
    if(Candidate)
    {
        const TCHAR* From[]={TEXT("SM_Dugout_V40"),TEXT("SM_DugoutCollapsed_V40"),TEXT("SM_RepairCache_V40")};
        const bool NewRoof=!(FParse::Param(FCommandLine::Get(),TEXT("WFV84412BagBaseline"))
            &&FrontlineEffectsReviewSafetyV8216::Allowed(B));
        const TCHAR* To[]={NewRoof?TEXT("SM_Dugout_V84413"):TEXT("SM_Dugout_V8449"),
            NewRoof?TEXT("SM_DugoutCollapsed_V84413"):TEXT("SM_DugoutCollapsed_V8449"),TEXT("SM_RepairCache_V8449")};
        for(int32 I=0;I<3;++I)if(OriginalPath.Contains(From[I]))
        {CurrentPath=FString(NewRoof&&I<2?TEXT("/Game/Warfare/Field/V84413/Props/Meshes/"):TEXT("/Game/Warfare/Field/V8449/Props/Meshes/"))+To[I]+TEXT(".")+To[I];Contact=I+3;break;}
    }
    auto* Asset=LoadObject<UStaticMesh>(nullptr,*CurrentPath);if(!Asset)return nullptr;
    auto* C=NewObject<UStaticMeshComponent>(Owner);Owner->AddInstanceComponent(C);C->SetupAttachment(Owner->GetRootComponent());
    C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Asset);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(true);C->SetCastShadow(true);C->RegisterComponent();
    FVector Seat=Position;
    if(Contact!=INDEX_NONE)
    {
        Seat=SeatPropV8449(Owner->GetWorld(),Contact,Seat,Rotation,Scale);
        C->ComponentTags.Add(TEXT("WF_Prop_V8449"));
        const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);
        const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,Mission?Mission->Id:0);
        auto* Earth=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V839/Materials/MI_EarthBank_V839.MI_EarthBank_V839"));
        for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
        {
            auto* Mat=PropMaterialV84412(Owner,C->GetMaterial(Slot));if(!Mat)continue;
            if(Mat->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42"))&&Earth)Mat=Earth;
            if(auto* MID=UMaterialInstanceDynamic::Create(Mat,Owner))
            {MID->SetScalarParameterValue(TEXT("WeatherWetness"),Mission?Mission->Wetness:.58f);
             MID->SetScalarParameterValue(TEXT("SnowCoverage"),Mission&&Mission->Id==10?.44f:0.f);
             MID->SetScalarParameterValue(TEXT("ChalkExposure"),Palette.Chalk);MID->SetVectorParameterValue(TEXT("SoilTint"),Palette.Soil);C->SetMaterial(Slot,MID);}
        }
    }
    C->SetWorldTransform(FTransform(Rotation,Seat,Scale));if(Contact!=INDEX_NONE)FitFoundationV8449(C,Contact);return C;
}
int32 Garrison(const AWarfareFrontlineGameMode* B,int32 Trench,int32 Team)
{
    int32 Men=0;
    for(const auto& Q:B->Squads)if(Q.Team==Team&&Q.Trench==Trench&&!Q.bAdvancing)
        for(const auto& W:Q.Members)if(const auto* U=W.Get();U&&U->IsActiveCombatant()&&U->bFieldEntered
            &&FMath::Abs(U->Feet().X-FrontlineFieldV32::TrenchCentreX(Trench,U->Feet().Y))<390.f)++Men;
    return Men;
}
int32 Contestants(const AWarfareFrontlineGameMode* B,int32 Trench,int32 Team)
{
    int32 Men=0;for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==Team&&U->IsActiveCombatant()&&U->bFieldEntered
        &&FMath::Abs(U->Feet().X-FrontlineFieldV32::TrenchCentreX(Trench,U->Feet().Y))<450.f)++Men;return Men;
}
void Assess(FSite& S,int32 A,int32 B,float Seconds)
{
    S.V.Garrison=S.V.Owner==0?A:S.V.Owner==1?B:0;
    if(S.V.Integrity<=0){S.V.Owner=INDEX_NONE;S.V.Capture=0;S.Capturing=INDEX_NONE;return;}
    if((A>0)==(B>0)){S.V.Capture=FMath::Max(0.f,S.V.Capture-Seconds/8.f);S.Capturing=INDEX_NONE;return;}
    const int32 Team=A>0?0:1;
    if(S.V.Owner==Team){S.V.Capture=1.f;S.Capturing=INDEX_NONE;return;}
    if(S.Capturing!=Team){S.Capturing=Team;S.V.Capture=0;}
    // Numbers do not accelerate this timer. A blob cannot instantly flip a gun.
    S.V.Capture=FMath::Min(1.f,S.V.Capture+Seconds/8.f);
    if(S.V.Capture>=1.f){S.V.Owner=Team;S.V.Garrison=Team==0?A:B;S.Capturing=INDEX_NONE;}
}
bool Live(const FSite& S){return S.V.Integrity>0&&S.V.Owner>=0&&S.V.Garrison>=2;}
int32 RepairSide(const AFrontlineTank* Tank)
{
    if(!Tank)return INDEX_NONE;
    if(Tank->CanRepairTrackV60(0))return 0;if(Tank->CanRepairTrackV60(1))return 1;
    const int32 Gun=Tank->RepairableMachinegunV63();return Gun>=0?2+Gun:INDEX_NONE;
}
AFrontlineTank* RepairTarget(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!U->bFieldEntered||!U->IsActiveCombatant()||U->Role!=EFrontlineRole::Pioneer)return nullptr;
    if(FrontlineFieldV32::HasMineApproachV833(U))return nullptr;
    const auto* B=U->Battle.Get();const auto* M=FrontlineCampaignV32::CurrentOperation(B);
    const auto* Q=B->FindSquad(U->SquadId);
    if(B->Phase!=EFrontlinePhase::Fighting||B->IsBattleOutroActive()||!M||M->Id<8||!Q||Q->bHoldPosition)return nullptr;
    auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Vehicles)return nullptr;
    AFrontlineTank* Best=nullptr;float Distance=1000.f;
    for(AFrontlineTank* Tank:Vehicles->GetVehicles())if(Tank&&Tank->Team==U->Team&&RepairSide(Tank)!=INDEX_NONE)
    {
        const FVector Delta=Tank->GetActorLocation()-U->Feet();const float D=Delta.Size2D();
        if(D<Distance&&(Q->bAdvancing||D<420.f)&&Delta.X*B->GetTeamDirection(U->Team)>=-300.f){Best=Tank;Distance=D;}
    }
    return Best;
}
void RepairTick(AWarfareFrontlineGameMode* B,FState& S,float Seconds)
{
    const auto* M=FrontlineCampaignV32::CurrentOperation(B);if(!M||M->Id<8)return;
    auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Vehicles)return;
    for(AFrontlineTank* Tank:Vehicles->GetVehicles())if(Tank&&RepairSide(Tank)!=INDEX_NONE)
    {
        auto& Job=S.Repairs.FindOrAdd(Tank);const int32 Side=RepairSide(Tank);
        if(Job.Side!=Side){Job.Side=Side;Job.Work=0;}Job.Working=false;
        bool EngineerPresent=false;
        for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Role==EFrontlineRole::Pioneer&&U->Suppression<.35f
            &&FVector::DistSquared2D(U->Feet(),Tank->GetActorLocation())<FMath::Square(420.f)&&RepairTarget(U)==Tank)
        {EngineerPresent=true;Job.Working|=!U->bMoving;}
        // A disabled gun can be serviced only after close attackers are cleared.
        bool Threatened=false;
        if(Side>=2)for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team!=Tank->Team&&U->IsActiveCombatant()
            &&U->bFieldEntered&&FVector::DistSquared2D(U->Feet(),Tank->GetActorLocation())<FMath::Square(1100.f)){Threatened=true;break;}
        if(Side>=2&&!Threatened)for(const AFrontlineTank* Enemy:Vehicles->GetVehicles())
            if(IsValid(Enemy)&&Enemy->Team!=Tank->Team&&Enemy->IsOperational()&&Enemy->HasEnteredField()
                &&FVector::DistSquared2D(Enemy->GetActorLocation(),Tank->GetActorLocation())<FMath::Square(1100.f)){Threatened=true;break;}
        if(Threatened)Job.Working=false;
        // Stop when a fit engineer reaches the hull, before asking him to stand
        // still. This short lease expires if he dies, leaves or becomes pinned.
        Tank->SetFieldRepairHoldV63(Side>=2&&EngineerPresent&&!Threatened);
        // One continuous repair, independent of how many engineers crowd it.
        Job.Work=Job.Working?FMath::Min(1.f,Job.Work+Seconds/(Side>=2?8.f:12.f)):0.f;
        if(Job.Work>.999f)Job.Work=1.f;
        if(Job.Work>=1.f&&(Side>=2?Tank->RepairMachinegunV63(Side-2):Tank->RepairTrackV60(Side)))
        {
            Job.Working=false;
            if(Tank->Team==0)B->LastMessage=Side>=2?L(TEXT("A disabled tank gun is back in action."),TEXT("受损车载机枪已恢复射击。")):
                L(TEXT("Engineers have repaired one track. Keep the damaged vehicle covered."),TEXT("工兵已抢修一侧履带。请继续掩护这辆受损坦克。"));
            UE_LOG(LogTemp,Display,TEXT("WF_V60_TRACK_REPAIRED team=%d serial=%d side=%d"),Tank->Team,Tank->Serial,Side);
        }
    }
    for(auto It=S.Repairs.CreateIterator();It;++It)if(!It.Key().IsValid()||!It.Key()->IsOperational()||RepairSide(It.Key().Get())==INDEX_NONE)It.RemoveCurrent();
}
void BuildWorks(AWarfareFrontlineGameMode* B,FSite& Site,int32 A,int32 E,float Seconds)
{
    const int32 Owner=B->Trenches[Site.Trench].Owner;
    if(Site.V.Integrity<=0){Site.V.Garrison=0;return;}
    if(Site.V.Owner!=Owner)
    {Site.V.Owner=Owner;Site.V.Work=0;for(auto& W:Site.Revetments)if(W.IsValid())W->SetVisibility(false);}
    Site.V.Garrison=Owner==0?A:Owner==1?E:0;
    if(Owner<0||Site.V.Work>=1.f)return;
    AWarfareFrontlineSoldier* Worker=nullptr;
    if(Contestants(B,Site.Trench,1-Owner)==0)
        for(const auto& Q:B->Squads)if(Q.Team==Owner&&Q.Trench==Site.Trench&&!Q.bAdvancing)
            for(const auto& W:Q.Members)if(auto* U=W.Get();U&&U->Role==EFrontlineRole::Pioneer&&U->IsActiveCombatant()&&U->bFieldEntered&&!U->bMoving&&U->Suppression<.35f
                &&FMath::Abs(U->Feet().X-FrontlineFieldV32::TrenchCentreX(Site.Trench,U->Feet().Y))<390.f)
                {Worker=U;break;}
    Site.V.Work=Worker?FMath::Min(1.f,Site.V.Work+Seconds/12.f):0.f;
    if(Site.V.Work>.999f)Site.V.Work=1.f;
    const int32 Dir=B->GetTeamDirection(Owner);
    for(int32 I=0;I<Site.Revetments.Num();++I)if(auto* C=Site.Revetments[I].Get())
    {
        const float Y=(I-1)*630.f,X=FrontlineFieldV32::TrenchCentreX(Site.Trench,Y)+Dir*155.f;
        C->SetWorldLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)-15.f));
        C->SetWorldRotation(FRotator(0,Dir>0?0:180,0));C->SetVisibility(Site.V.Work>=float(I+1)/3.f);
    }
}
bool GunLine(AWarfareFrontlineGameMode* B,const FState& S,const FVector& From,const FVector& To,int32 Team)
{
    const FVector Delta=To-From;const float D=Delta.Size2D();
    if(D<600.f||D>4800.f||Delta.X*B->GetTeamDirection(Team)<D*.81915204f||B->DirectFireMultiplier(From,To)<.95f)return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(V60CasemateSight),true);if(S.Art.IsValid())Query.AddIgnoredActor(S.Art.Get());
    for(AWarfareFrontlineSoldier* U:B->Units)if(U)Query.AddIgnoredActor(U);
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* Tank:Vehicles->GetVehicles())if(Tank)Query.AddIgnoredActor(Tank);
    return !B->GetWorld()->LineTraceTestByChannel(From,To,ECC_Visibility,Query);
}
void FireCasemate(AWarfareFrontlineGameMode* B,FState& S,FSite& Site)
{
    if(!Live(Site)||Site.V.Ammunition<=0||!Site.Building.IsValid()||!Site.Gun.IsValid()||B->Elapsed<Site.ShotAt)return;
    const int32 Team=Site.V.Owner,Dir=B->GetTeamDirection(Team);
    const FVector Base=Site.Building->GetComponentLocation();
    // Both ends of this timber gallery are open. Ownership turns the gun on
    // its fixed centre axis; the earthwork never rotates or teleports.
    const FVector Pivot=Base+FVector(103.5f,0,100.f),Muzzle=Pivot+FVector(Dir*140.f,0,0);
    Site.Gun->SetWorldLocation(Pivot);Site.Gun->SetWorldRotation(FRotator(0,Dir>0?0:180,0));
    FVector Target;float Best=MAX_flt;bool Found=false;
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
        for(AFrontlineTank* Tank:Vehicles->GetVehicles())if(Tank&&Tank->Team!=Team&&Tank->HasEnteredField()&&Tank->IsOperational()&&GunLine(B,S,Muzzle,Tank->AimPoint(),Team))
        {const float Score=FVector::DistSquared2D(Muzzle,Tank->AimPoint())*.25f;if(Score<Best){Best=Score;Target=Tank->AimPoint();Found=true;}}
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team!=Team&&U->IsActiveCombatant()&&GunLine(B,S,Muzzle,U->Feet()+FVector(0,0,100),Team))
    {const float Score=FVector::DistSquared2D(Muzzle,U->Feet());if(Score<Best){Best=Score;Target=U->Feet()+FVector(0,0,80);Found=true;}}
    if(!Found)return;
    // Do not choose an aim point through our own crew; shells already fired
    // keep their immutable team/impact and can still cause ordinary friendly fire.
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==Team&&U->IsActiveCombatant()&&FVector::DistSquared2D(U->Feet(),Target)<FMath::Square(380.f))return;
    Site.Gun->SetWorldLocation(Pivot);Site.Gun->SetWorldRotation((Target-Pivot).Rotation());
    FFrontlineStrike Shell;Shell.Team=Team;Shell.Origin=Pivot+(Target-Pivot).GetSafeNormal()*140.f;
    // Bounded deterministic dispersal; the gun never tracks after firing.
    const int32 Shot=8-Site.V.Ammunition;Target+=FVector((Shot%3-1)*65.f,(Shot%4-1.5f)*55.f,0);
    Target.Z=AWarfareFrontlineGround::HeightAt(Target.X,Target.Y)+12.f;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(V60CasemateShot),true);if(S.Art.IsValid())Q.AddIgnoredActor(S.Art.Get());
    for(AWarfareFrontlineSoldier* U:B->Units)if(U)Q.AddIgnoredActor(U);
    if(B->GetWorld()->LineTraceSingleByChannel(Hit,Shell.Origin,Target,ECC_Visibility,Q))Target=Hit.ImpactPoint;
    Shell.Target=Target;Shell.Radius=300.f;Shell.ArmorPower=400.f;Shell.bDirectFire=true;Shell.bMortar=true;
    Shell.LaunchedAt=B->Elapsed;Shell.ImpactAt=B->Elapsed+FMath::Clamp(float(FVector::Dist(Shell.Origin,Target)/9000.f),.18f,.65f);
    Shell.bIncomingSoundPlayed=true;Shell.Projectile=FrontlineSupportArtV36::SpawnShell(B,Shell);B->Strikes.Add(Shell);
    --Site.V.Ammunition;Site.ShotAt=B->Elapsed+6.f;
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayTankCannon(Shell.Origin);
    UE_LOG(LogTemp,Display,TEXT("WF_V60_CASEMATE_FIRE team=%d trench=%d ammunition=%d"),Team,Site.Trench,Site.V.Ammunition);
}
}
void Reset(AWarfareFrontlineGameMode* B)
{
    if(auto* S=States.Find(B))if(S->Art.IsValid())S->Art->Destroy();States.Remove(B);
    for(auto I=States.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
}
void Begin(AWarfareFrontlineGameMode* B)
{
    Reset(B);if(!B||B->Trenches.Num()<2)return;
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);if(!Mission||Mission->Id<1)return;
    auto& S=States.Add(B);S.Encounter=B->GetEncounterId();
    auto* A=B->GetWorld()->SpawnActor<AActor>();if(!A){States.Remove(B);return;}S.Art=A;
    auto* Root=NewObject<USceneComponent>(A);A->SetRootComponent(Root);A->AddInstanceComponent(Root);Root->RegisterComponent();
    auto Add=[&](int32 Trench,EKind Kind,float Y)
    {
        FSite Site;Site.Trench=Trench;Site.V.Kind=Kind;
        // Back-edge furniture stays outside all infantry lanes and the railway.
        const float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);
        Site.V.Position=FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y));
        FVector Back=Site.V.Position+FVector(0,Y<0?-150.f:150.f,0);
        Back.Z=AWarfareFrontlineGround::HeightAt(Back.X,Back.Y);
        Site.Building=Mesh(A,Kind==EKind::Stores?
            TEXT("/Game/Warfare/Field/V40/Meshes/SM_RepairCache_V40.SM_RepairCache_V40"):
            TEXT("/Game/Warfare/Field/V40/Meshes/SM_Dugout_V40.SM_Dugout_V40"),Back,FRotator(0,90,0),Kind==EKind::Stores?FVector(1.35):FVector(1.5,1.5,1.2));
        if(Kind==EKind::AntiAircraft)
        {
            FVector Mount=Back+FVector(50,0,155);
            if(Site.Building.IsValid()&&Site.Building->ComponentHasTag(TEXT("WF_Prop_V8449")))Mount.Z=Site.Building->GetComponentLocation().Z+155.f;
            Site.Gun=Mesh(A,TEXT("/Game/Warfare/Vehicles/V33/SM_TankMG.SM_TankMG"),Mount,FRotator(55,0,0),FVector(1));
            auto* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Vehicles/V37/M_Vehicle_Steel.M_Vehicle_Steel"));
            for(int32 Leg=0;Leg<3;++Leg)
            {
                const float Angle=Leg*2.f*PI/3.f;
                const FVector Foot=Back+FVector(50+FMath::Cos(Angle)*65,FMath::Sin(Angle)*65,0);
                const FVector D=Mount-Foot;
                auto* C=Mesh(A,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),(Mount+Foot)*.5,
                    FRotationMatrix::MakeFromZ(D).Rotator(),FVector(.055,.055,D.Size()/100.f));
                if(C&&Steel)C->SetMaterial(0,Steel);
            }
        }
        else if(Kind==EKind::Casemate)
        {
            Site.V.Ammunition=8;
            if(Site.Building.IsValid())
            {
                Site.Building->SetWorldRotation(FRotator::ZeroRotator);
                // Rotation occurs after creation on this fixed cannon axis.
                // Refit the real lower feet under the final world orientation.
                if(Site.Building->ComponentHasTag(TEXT("WF_Prop_V8449")))
                {Site.Building->SetWorldLocation(SeatPropV8449(B->GetWorld(),3,Back,FRotator::ZeroRotator,Site.Building->GetComponentScale()));FitFoundationV8449(Site.Building.Get(),3);}
            }
            const FVector CannonBase=Site.Building.IsValid()?Site.Building->GetComponentLocation():Back;
            Site.Gun=Mesh(A,TEXT("/Game/Warfare/Vehicles/V33/SM_SixPounder.SM_SixPounder"),CannonBase+FVector(103.5f,0,100),FRotator(0,180,0),FVector(1.3));
        }
        if(Site.Building.IsValid()&&((Kind!=EKind::Casemate&&Kind!=EKind::AntiAircraft)||Site.Gun.IsValid()))
        {S.Sites.Add(Site);}
    };
    const int32 Centre=B->Trenches.Num()/2;
    const float Side=Mission->bArmoredTrain?1080.f:-1080.f;
    if(B->Trenches.Num()>=3)
    {
        const bool Cannon=Mission->Id==5||Mission->Id==8||Mission->Id==16;
        Add(Centre,Cannon?EKind::Casemate:(Mission->Id==2||Mission->Id==13)?EKind::Stores:EKind::Dugout,Side);
        if(Mission->Id>=3)Add(B->GetHomeTrenchIndex(1),Mission->Id>=11&&Mission->Id%3!=0?EKind::AntiAircraft:EKind::Dugout,Side);
        if(Mission->Id>=11&&Mission->Id%3==0)Add(B->GetHomeTrenchIndex(0),EKind::AntiAircraft,Side);
    }
    for(int32 Trench=0;Trench<B->Trenches.Num();++Trench)
    {
        FSite Works;Works.Trench=Trench;Works.V.Kind=EKind::FieldWorks;
        Works.V.Position=FVector(B->Trenches[Trench].X,0,AWarfareFrontlineGround::HeightAt(B->Trenches[Trench].X,0));
        for(int32 I=0;I<3;++I)
        {
            auto* C=Mesh(A,TEXT("/Game/Warfare/Field/V42/Meshes/SM_RepairRevetment_A_V42.SM_RepairRevetment_A_V42"),Works.V.Position,FRotator::ZeroRotator,FVector(1,1.7,.8));
            if(C){C->SetVisibility(false);Works.Revetments.Add(C);if(!Works.Building.IsValid())Works.Building=C;}
        }
        if(Works.Revetments.Num()==3)
        {
            S.Sites.Add(Works);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V59_FIELD_SITES operation=%d sites=%d"),Mission->Id,S.Sites.Num());
}

TSharedRef<FJsonObject> DescribeVisualsV8449(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();const auto* S=B?States.Find(B):nullptr;
    const bool Candidate=B&&!WarfareFRRUCampaignV82::Active(B)&&!FrontlineEnvironmentV42::FieldBaselineV8449(B);
    TArray<TSharedPtr<FJsonValue>> Rows;int32 BindingErrors=0,ContactErrors=0,CollisionErrors=0,Count=0;
    const bool RoofV84412=Candidate&&!(FParse::Param(FCommandLine::Get(),TEXT("WFV84412BagBaseline"))
        &&FrontlineEffectsReviewSafetyV8216::Allowed(B));
    AWarfareFrontlineGround* Receiver=B?ContactGroundV8449(B->GetWorld()):nullptr;
    if(S)for(const auto& Site:S->Sites)if(Site.V.Kind!=EKind::FieldWorks)
        if(auto* C=Site.Building.Get();C&&C->GetStaticMesh()&&C->IsVisible())
        {
            const UStaticMesh* Mesh=C->GetStaticMesh();const bool Bound=Mesh->GetPathName().StartsWith(RoofV84412&&Site.V.Kind!=EKind::Stores?
                TEXT("/Game/Warfare/Field/V84413/Props/Meshes/"):Candidate?
                TEXT("/Game/Warfare/Field/V8449/Props/Meshes/"):TEXT("/Game/Warfare/Field/V40/Meshes/"));
            ++Count;BindingErrors+=!Bound;CollisionErrors+=C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;
            const FTransform T=C->GetComponentTransform();const FVector P=T.GetLocation();
            int32 Contact=Site.V.Kind==EKind::Stores?5:3;if(Mesh->GetName().Contains(TEXT("Collapsed")))Contact=4;
            float Bottom=MAX_flt,Top=-MAX_flt,Supported=MAX_flt,FrontBottom=MAX_flt,FrontTop=-MAX_flt,RearEmbed=0;
            int32 Missing=0,SupportFeet=0;
            if(Candidate)for(int32 Foot=0;Foot<9;++Foot)
            {
                const FVector F=T.TransformPosition(PropContactsV8449[Contact][Foot]);FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;
                const bool Found=Receiver&&FrontlineGroundContactV8449::Sample(Receiver,F.X,F.Y,Actual,Normal);Missing+=!Found;
                const float D=Found?float(F.Z-Actual.Z):float(F.Z-AWarfareFrontlineGround::HeightAt(F.X,F.Y));
                Bottom=FMath::Min(Bottom,D);Top=FMath::Max(Top,D);
                if(SupportsPropV8449(Contact,Foot))
                {++SupportFeet;FrontBottom=FMath::Min(FrontBottom,D);FrontTop=FMath::Max(FrontTop,D);Supported=FMath::Min(Supported,FMath::Abs(D+3.f));}
                else RearEmbed=FMath::Max(RearEmbed,-D);
            }
            const auto* Foundation=FoundationsV8449.Find(C);const UStaticMeshComponent* FoundationComponent=Foundation?Foundation->Component.Get():nullptr;
            float FoundationBottom=MAX_flt,FoundationTop=-MAX_flt;int32 FoundationMissing=0,ActualBottomVertices=0;
            const UStaticMesh* FoundationMesh=FoundationComponent?FoundationComponent->GetStaticMesh():nullptr;
            const auto* FoundationRender=FoundationMesh?FoundationMesh->GetRenderData():nullptr;
            const auto* FoundationLOD=FoundationRender&&!FoundationRender->LODResources.IsEmpty()?&FoundationRender->LODResources[0]:nullptr;
            if(Foundation)for(const FVector& Foot:Foundation->LowerFeet)
            {
                FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;const bool Found=Receiver&&FrontlineGroundContactV8449::Sample(Receiver,Foot.X,Foot.Y,Actual,Normal);FoundationMissing+=!Found;
                if(Found){const float D=float(Foot.Z-Actual.Z);FoundationBottom=FMath::Min(FoundationBottom,D);FoundationTop=FMath::Max(FoundationTop,D);}
                bool VisibleVertex=false;
                if(FoundationLOD&&FoundationMesh->bAllowCPUAccess)
                    for(uint32 V=0;V<FoundationLOD->VertexBuffers.PositionVertexBuffer.GetNumVertices();++V)
                        if(FVector::DistSquared(Foot,FVector(FoundationLOD->VertexBuffers.PositionVertexBuffer.VertexPosition(V)))<.0001){VisibleVertex=true;break;}
                ActualBottomVertices+=VisibleVertex;
            }
            const bool Fitted=Foundation&&FoundationComponent&&FoundationMesh&&FoundationLOD&&Foundation->Missing==0&&FoundationMissing==0
                &&!Foundation->LowerFeet.IsEmpty()&&ActualBottomVertices==Foundation->LowerFeet.Num()
                &&FoundationBottom>=-3.08f&&FoundationTop<=-2.92f
                &&FoundationComponent->GetCollisionEnabled()==ECollisionEnabled::NoCollision
                &&FoundationComponent->GetMaterial(0)==C->GetMaterial(0);
            // Keep the original authored-foot gap visible in the receipt.
            // A high doorway passes only with an actual fitted timber support,
            // whose complete rendered lower edge is read back and grounded.
            const bool Grounded=!Candidate||(Missing==0&&FrontBottom>=-3.08f&&Supported<=.08f&&(FrontTop<=15.f||Fitted));ContactErrors+=!Grounded;
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("mesh"),Mesh->GetPathName());Row->SetStringField(TEXT("component"),C->GetPathName());
            Row->SetBoolField(TEXT("expected_binding"),Bound);Row->SetBoolField(TEXT("ground_support"),Grounded);
            Row->SetBoolField(TEXT("collision_disabled"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
            Row->SetNumberField(TEXT("support_min_cm"),Candidate?Bottom:0);Row->SetNumberField(TEXT("support_max_gap_cm"),Candidate?Top:0);
            Row->SetStringField(TEXT("support_policy"),Contact==5?TEXT("Freestanding stores: all nine authored lower feet."):TEXT("Dugout: three authored doorway/sill feet supported; rear wall embedded into the existing bank."));
            Row->SetNumberField(TEXT("front_support_min_cm"),Candidate?FrontBottom:0);Row->SetNumberField(TEXT("front_max_gap_cm"),Candidate?FrontTop:0);
            Row->SetNumberField(TEXT("front_support_foot_count"),Candidate?SupportFeet:0);Row->SetNumberField(TEXT("rear_embed_max_cm"),Candidate?RearEmbed:0);
            Row->SetBoolField(TEXT("fitted_timber_foundation"),Fitted);Row->SetNumberField(TEXT("foundation_actual_lower_vertices"),ActualBottomVertices);
            Row->SetNumberField(TEXT("foundation_lower_vertex_count"),Foundation?Foundation->LowerFeet.Num():0);
            Row->SetNumberField(TEXT("foundation_lower_min_cm"),Fitted?FoundationBottom:0);Row->SetNumberField(TEXT("foundation_lower_max_cm"),Fitted?FoundationTop:0);
            Row->SetNumberField(TEXT("foundation_source_missing"),Foundation?Foundation->Missing+FoundationMissing:0);
            Row->SetNumberField(TEXT("actual_source_contact_missing"),Missing);
            Row->SetNumberField(TEXT("x"),P.X);Row->SetNumberField(TEXT("y"),P.Y);Row->SetNumberField(TEXT("z"),P.Z);
            Row->SetBoolField(TEXT("gun_xy_unchanged"),!Site.Gun.IsValid()||FMath::Abs(Site.Gun->GetComponentLocation().Y-P.Y)<.02f);
            TArray<TSharedPtr<FJsonValue>> Materials;
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
            {
                auto M=MakeShared<FJsonObject>();auto* Actual=C->GetMaterial(Slot);auto* Expected=PropMaterialV84412(C,Mesh->GetMaterial(Slot));
                const UMaterialInterface* Resolved=Actual;bool Parent=false;
                for(int32 Depth=0;Resolved&&Depth<12;++Depth)
                {if(Resolved==Expected){Parent=true;break;}const auto* MI=Cast<UMaterialInstance>(Resolved);Resolved=MI?MI->Parent.Get():nullptr;}
                // Authored mud slots are deliberately promoted to the same
                // installed earth shader, not a second unsupported palette.
                if(Candidate&&Expected&&Expected->GetPathName().Contains(TEXT("MI_EarthBank_V42")))
                    Parent=Actual&&Actual->GetBaseMaterial()&&Actual->GetBaseMaterial()->GetName().Contains(TEXT("EarthBank_V839"));
                if(Candidate&&Mesh->GetMaterial(Slot)&&Mesh->GetMaterial(Slot)->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V8449/Props/Materials/M_")))
                    Parent&=Expected&&(Expected->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_"))
                        ||Expected->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84413/Props/Materials/M_Hessian_V84413")));
                BindingErrors+=!Parent;
                M->SetStringField(TEXT("bound"),GetPathNameSafe(Actual));M->SetStringField(TEXT("base"),Actual?GetPathNameSafe(Actual->GetBaseMaterial()):TEXT("missing"));
                M->SetBoolField(TEXT("authored_parent"),Parent);Materials.Add(MakeShared<FJsonValueObject>(M));
            }
            Row->SetArrayField(TEXT("materials"),Materials);Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
    J->SetBoolField(TEXT("candidate"),Candidate);J->SetNumberField(TEXT("visible_sites"),Count);
    J->SetNumberField(TEXT("binding_errors"),BindingErrors);J->SetNumberField(TEXT("ground_contact_errors"),ContactErrors);
    J->SetNumberField(TEXT("collision_components"),CollisionErrors);J->SetArrayField(TEXT("sites"),Rows);
    J->SetBoolField(TEXT("success"),BindingErrors==0&&ContactErrors==0&&CollisionErrors==0);
    J->SetStringField(TEXT("scope"),TEXT("Actual visible site mesh, MID ancestry and nine lower-foot contacts. Dugout front support is tested against the actual indexed terrain; rear walls deliberately embed into the bank. Fixed gameplay/cannon XY and all capture rules are preserved; the existing sites are not claimed to clear the conservative full tree/armour lane envelope."));
    return J;
}
void Tick(AWarfareFrontlineGameMode* B,float DT)
{
    auto* S=States.Find(B);if(!S||S->Encounter!=B->GetEncounterId()||B->Phase!=EFrontlinePhase::Fighting||B->IsBattleOutroActive())return;
    if(B->Elapsed<S->NextAssessment)return;
    const float Step=S->LastAssessment>0?FMath::Clamp(B->Elapsed-S->LastAssessment,0.f,.5f):.25f;
    S->LastAssessment=B->Elapsed;S->NextAssessment=B->Elapsed+.25f;
    RepairTick(B,*S,Step);
    for(auto& Site:S->Sites)
    {
        const int32 Old=Site.V.Owner;
        const int32 A=Garrison(B,Site.Trench,0),E=Garrison(B,Site.Trench,1);
        const int32 NearA=Contestants(B,Site.Trench,0),NearE=Contestants(B,Site.Trench,1);
        if(Site.V.Kind==EKind::FieldWorks)
        {
            BuildWorks(B,Site,A,E,Step);
            Site.V.Contested=NearA>0&&NearE>0;
            if(Site.V.Contested)Site.V.Garrison=0;
            continue;
        }
        // Advancing opponents at the firing lip contest, even before they can
        // occupy a trench slot. Mere proximity cannot capture an empty post.
        Assess(Site,A>0?A:(E>0&&NearA>0?1:0),E>0?E:(A>0&&NearE>0?1:0),Step);
        Site.V.Contested=NearA>0&&NearE>0;
        Site.V.Garrison=(NearA>0&&NearE>0)?0:Site.V.Owner==0?A:Site.V.Owner==1?E:0;
        if(Site.V.Owner>=0&&Site.V.Owner!=Old)
        {
            ++S->Captures;
            if(Site.V.Owner==0)B->LastMessage=(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?FString::Printf(TEXT("%s secured. Leave a section to operate the position."),*Title(Site.V.Kind)):FString::Printf(TEXT("%s已控制。留下一支小队操作工事。"),*Title(Site.V.Kind)));
            UE_LOG(LogTemp,Display,TEXT("WF_V59_SITE_CAPTURE kind=%d trench=%d team=%d"),int32(Site.V.Kind),Site.Trench,Site.V.Owner);
        }
        if(Site.V.Kind==EKind::Stores&&Live(Site)&&!(Site.Rewards&(1<<Site.V.Owner)))
        {
            const int32 Team=Site.V.Owner;
            // A single relief section per army and per depot, not a renewable
            // cooldown exploit. It still enters by the ordinary rear approach.
            if(FFrontlineFieldSitesV59Access::Relief(B,Team))
            {
                Site.Rewards|=1<<Team;Site.V.RewardTaken=true;
                if(Team==0)B->LastMessage=L(TEXT("Captured stores secured: a bomb-carrying relief section is coming up."),TEXT("补给库已移交：一支携弹增援小队正在赶来。"));
            }
        }
        if(Site.V.Kind==EKind::Casemate)FireCasemate(B,*S,Site);
    }
    for(auto I=S->Aircraft.CreateIterator();I;++I)if(B->Elapsed-I.Value().WindowEnd>15.f)I.RemoveCurrent();
}
float BulletCover(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||U->bMoving)return 1.f;
    const auto* S=States.Find(U->Battle.Get());if(!S)return 1.f;
    const auto* Q=U->Battle->FindSquad(U->SquadId);if(!Q||Q->Trench<0||Q->bAdvancing)return 1.f;
    for(const auto& Site:S->Sites)if(Site.Trench==Q->Trench&&Site.V.Owner==U->Team&&Site.V.Kind==EKind::Dugout&&Live(Site))return .72f;
    return 1.f;
}
float PreparedFrontCoverV60(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target)
{
    if(!Shooter||!Target||!Target->Battle||Target->bMoving||BulletCover(Target)<1.f)return 1.f;
    const auto* B=Target->Battle.Get();const auto* Q=B->FindSquad(Target->SquadId);const auto* S=States.Find(Target->Battle.Get());
    if(!Q||Q->Trench<0||Q->bAdvancing||!S
        ||FMath::Abs(Target->Feet().X-FrontlineFieldV32::TrenchCentreX(Q->Trench,Target->Feet().Y))>=390.f
        ||(Shooter->Feet().X-Target->Feet().X)*B->GetTeamDirection(Target->Team)<=0)return 1.f;
    for(const auto& Site:S->Sites)if(Site.V.Kind==EKind::FieldWorks&&Site.Trench==Q->Trench&&Site.V.Owner==Target->Team&&Site.V.Work>=1.f&&Live(Site))return .78f;
    return 1.f;
}
bool EngineerDestinationV60(const AWarfareFrontlineSoldier* U,FVector& Destination)
{
    // A reserved local charge is finished first. Repair and escort work resume
    // through the ordinary selector once the charge or its order is released.
    if(FrontlineFieldV32::HasMineApproachV833(U))return false;
    auto* Tank=RepairTarget(U);if(!Tank)return FrontlineArmourV63::EscortDestination(U,Destination);
    const int32 Side=RepairSide(Tank);
    Destination=Tank->GetActorLocation()+Tank->GetActorRightVector()*(Side==0?-325.f:325.f)
        +Tank->GetActorForwardVector()*((U->MemberIndex-1.5f)*70.f);
    Destination.Z=AWarfareFrontlineGround::HeightAt(Destination.X,Destination.Y);
    return true;
}
TArray<FRepairViewV60> RepairViewsV60(const AWarfareFrontlineGameMode* B)
{
    TArray<FRepairViewV60> Result;const auto* S=States.Find(const_cast<AWarfareFrontlineGameMode*>(B));if(!S)return Result;
    for(const auto& Item:S->Repairs)if(auto* Tank=Item.Key.Get();Tank&&Tank->IsOperational()&&RepairSide(Tank)!=INDEX_NONE)
    {FRepairViewV60 V;V.Tank=Tank;V.Position=Tank->GetActorLocation();V.Work=Item.Value.Work;V.Side=Item.Value.Side;V.Working=Item.Value.Working;Result.Add(V);}
    return Result;
}
FString RepairDescriptionV60(const FRepairViewV60& V)
{
    const auto* Tank=V.Tank.Get();const auto* B=Tank?Cast<AWarfareFrontlineGameMode>(Tank->GetWorld()->GetAuthGameMode()):nullptr;
    const int32 Percent=FMath::RoundToInt(100.f*FrontlineArmourV63::RepairRestoredFractionV8218(B,Tank?Tank->Team:0));
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    if(V.Side>=2)return Chinese
        ?FString::Printf(TEXT("工兵可将一挺受损机枪修复至 %d%% 耐久，每辆车限一次。作业须连续 8 秒，且 11 米内没有敌军。"),Percent)
        :FString::Printf(TEXT("Engineers restore one disabled machine gun per vehicle to %d%% durability after 8 uninterrupted seconds with no enemy within 11 metres."),Percent);
    if(Tank&&Tank->IsWheeledV8218())return Chinese
        ?FString::Printf(TEXT("工兵连续作业 12 秒，可将损坏的车轮恢复至 %d%% 耐久，每侧限修一次。附近敌军或离开车辆会中断作业。"),Percent)
        :FString::Printf(TEXT("Engineers restore damaged wheels to %d%% durability after 12 uninterrupted seconds, once per side. Nearby enemies or leaving the vehicle interrupt work."),Percent);
    return Chinese
        ?FString::Printf(TEXT("工兵连续作业 12 秒，可将断裂履带恢复至 %d%% 耐久，每侧限修一次。附近敌军或离开车辆会中断作业。"),Percent)
        :FString::Printf(TEXT("Engineers restore broken tracks to %d%% durability after 12 uninterrupted seconds, once per side. Nearby enemies or leaving the vehicle interrupt work."),Percent);
}
void OnBlast(AWarfareFrontlineGameMode* B,const FFrontlineStrike& Strike)
{
    auto* S=States.Find(B);if(!S||Strike.Target.ContainsNaN()||!FMath::IsFinite(Strike.Radius)||Strike.Radius<=0)return;
    for(auto& Site:S->Sites)if(Site.V.Integrity>0)
    {
        // The fighting bay is in the trench; its rear entrance is the visible
        // site marker. Shells landing in the occupied bay damage the shelter.
        const float D=FMath::Abs(Strike.Target.X-B->Trenches[Site.Trench].X);
        if(D>Strike.Radius||FMath::Abs(Strike.Target.Y)>1500.f)continue;
        if(Site.V.Kind==EKind::FieldWorks&&Site.V.Work<=0.f)continue;
        Site.V.Integrity=FMath::Max(0.f,Site.V.Integrity-(Strike.bGrenade?.08f:Strike.bMortar?.20f:.34f)*(1.f-D/Strike.Radius));
        if(Site.V.Kind==EKind::FieldWorks&&Site.V.Work<1.f)Site.V.Work=0;
        if(Site.V.Integrity<=0)
        {
            Site.V.Owner=INDEX_NONE;Site.V.Garrison=0;
            if(auto* Gun=Site.Gun.Get())Gun->SetVisibility(false);
            for(auto& W:Site.Revetments)if(W.IsValid())W->SetVisibility(false);
            if(Site.V.Kind!=EKind::FieldWorks)if(auto* C=Site.Building.Get())
            {
                const TCHAR* Path=TEXT("/Game/Warfare/Field/V40/Meshes/SM_DugoutCollapsed_V40.SM_DugoutCollapsed_V40");
                if(C->ComponentHasTag(TEXT("WF_Prop_V8449")))
                {
                    // The collapsed model has different feet and slots. Seat
                    // and bind its own materials through the same constructor,
                    // retaining the fixed capture/cannon horizontal position.
                    const FTransform T=C->GetComponentTransform();
                    if(auto* Replacement=Mesh(C->GetOwner(),Path,T.GetLocation(),T.Rotator(),T.GetScale3D()))
                    {
                        if(auto* Foundation=FoundationsV8449.Find(C))if(auto* Old=Foundation->Component.Get())Old->DestroyComponent();
                        FoundationsV8449.Remove(C);Site.Building=Replacement;C->DestroyComponent();
                    }
                }
                else if(auto* Rubble=LoadObject<UStaticMesh>(nullptr,Path))C->SetStaticMesh(Rubble);
            }
            UE_LOG(LogTemp,Display,TEXT("WF_V59_SITE_DISABLED trench=%d kind=%d"),Site.Trench,int32(Site.V.Kind));
        }
    }
}
bool EngageAircraft(AWarfareFrontlineGameMode* B,int32 Team,const FVector& P,uint32 Key,float DT)
{
    if(!B||Team<0||Team>1||P.ContainsNaN()||!FMath::IsFinite(DT)||DT<=0||!FMath::IsFinite(B->Elapsed)||B->IsBattleOutroActive())return false;
    auto* S=States.Find(B);if(!S)return false;
    FSite* Gun=nullptr;
    for(auto& Site:S->Sites)if(Site.V.Kind==EKind::AntiAircraft&&Site.V.Owner==1-Team&&Live(Site)&&Site.Gun.IsValid()
        &&FVector::DistSquared2D(Site.V.Position,P)<FMath::Square(3600.f))
    {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(V59AAObservation),true);if(S->Art.IsValid())Q.AddIgnoredActor(S->Art.Get());
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Q.AddIgnoredActor(U);
        const FVector Origin=Site.Gun->GetComponentLocation();
        if(B->DirectFireMultiplier(Origin,P)<.75f||B->GetWorld()->LineTraceTestByChannel(Origin,P,ECC_Visibility,Q))continue;
        Gun=&Site;break;
    }
    if(!Gun){S->Aircraft.Remove(Key);return false;}
    auto& Exposure=S->Aircraft.FindOrAdd(Key);
    if(Exposure.DrivenOff)return false;
    // DT describes this observation's game-time window. Consecutive large
    // frames still cover continuous time; uncovered windows break the dwell.
    // Count only the part after the previous end, so duplicate calls in the
    // same game step cannot double the aircraft's exposure.
    const float WindowStart=B->Elapsed-DT;
    if(!Exposure.Observed||B->Elapsed<Exposure.WindowEnd||WindowStart>Exposure.WindowEnd+.0001f)
    {Exposure.Seconds=0;Exposure.WindowEnd=WindowStart;Exposure.Observed=true;}
    Exposure.Seconds+=FMath::Max(0.f,B->Elapsed-FMath::Max(WindowStart,Exposure.WindowEnd));
    Exposure.WindowEnd=FMath::Max(Exposure.WindowEnd,B->Elapsed);
    const FVector Origin=Gun->Gun->GetComponentLocation();Gun->Gun->SetWorldRotation((P-Origin).Rotation());
    if(B->Elapsed>=Gun->ShotAt)
    {
        Gun->ShotAt=B->Elapsed+.16f;
        FrontlineSupportArtV36::StrafeRound(B,Origin,P,.06f,false);
        if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayWeapon(true,B->GetFactionForTeam(1-Team)==EFrontlineFaction::German,Origin,-5900-Gun->Trench);
    }
    if(Exposure.Seconds<.85f)return false;
    Exposure.DrivenOff=true;++S->Interceptions;
    B->LastMessage=Team==0?L(TEXT("Ground fire has broken up our air attack. Clear the high-angle gun before another run."),TEXT("地面高射火力迫使飞机中止攻击。下次呼叫前，先夺取或摧毁防空岗。")):
        L(TEXT("Our high-angle gun has driven off the enemy aircraft."),TEXT("我方高射机枪已逼退敌机。"));
    return true;
}
TArray<FView> Views(const AWarfareFrontlineGameMode* B)
{
    TArray<FView> R;if(const auto* S=States.Find(const_cast<AWarfareFrontlineGameMode*>(B)))for(const auto& Site:S->Sites)
    {
        auto V=Site.V;V.CapturingTeam=Site.Capturing;V.RewardTaken=V.Owner>=0&&(Site.Rewards&(1<<V.Owner));
        V.Operating=Live(Site)&&(V.Kind!=EKind::FieldWorks||V.Work>=1.f)&&(V.Kind!=EKind::Casemate||V.Ammunition>0&&Site.Gun.IsValid());
        V.ShowOwnershipFlag=V.Owner>=0&&V.Owner<2&&V.Integrity>0&&(V.Kind!=EKind::FieldWorks||V.Work>=1.f);
        // Anchor the small HUD flag above the actual placed building.
        V.MarkerPosition=V.Position+FVector(0,0,140);
        if(const auto* Building=Site.Building.Get())V.MarkerPosition=Building->Bounds.Origin+FVector(0,0,Building->Bounds.BoxExtent.Z+20);
        if(const auto* Gun=Site.Gun.Get())V.MarkerPosition.Z=FMath::Max(V.MarkerPosition.Z,Gun->Bounds.Origin.Z+Gun->Bounds.BoxExtent.Z+20);
        R.Add(V);
    }
    return R;
}
FString Title(EKind Kind)
{
    switch(Kind){case EKind::Stores:return L(TEXT("FORWARD STORES"),TEXT("前沿补给库"));case EKind::AntiAircraft:return L(TEXT("HIGH-ANGLE GUN"),TEXT("高射机枪岗"));case EKind::Casemate:return L(TEXT("CAPTURED GUN POSITION"),TEXT("固定直射炮位"));case EKind::FieldWorks:return L(TEXT("ENGINEER REVETMENTS"),TEXT("工兵壕肩加固"));default:return L(TEXT("REINFORCED DUGOUT"),TEXT("加固掩蔽所"));}
}
FString Description(const FView& V)
{
    const FString Rules=L(TEXT("Defenders take their posts along the parapet.\nCapture after 8 uncontested seconds; requires two defenders\nShellfire can destroy the works\n\n"),TEXT("守军沿胸墙接管阵位。\n独占战壕 8 秒后接管工事；需两名守军留驻\n炮击可摧毁工事\n\n"));
    if(V.Integrity<=0)return L(TEXT("The works are shattered. The trench can still be occupied, but its equipment no longer operates."),TEXT("工事已被摧毁。战壕仍可占领，但阵地设备已经失效。"));
    if(V.Kind==EKind::Casemate)return Rules+L(TEXT("The gun faces the approach.\nSmoke, close assault or flanking can evade its fire\nCaptured guns retain their remaining ammunition\n{count} shells; reloads in 6 seconds\nRange 6–48 metres; 70-degree forward arc"),TEXT("炮口朝向进攻通路。\n烟幕、近战或侧翼接近可避开火力\n夺取后沿用原有余弹\n余弹 {count} 发；装填需 6 秒\n距离 6–48 米；前方 70 度射界")).Replace(TEXT("{count}"),*FString::FromInt(V.Ammunition));
    if(V.Kind==EKind::FieldWorks)return L(TEXT("Pioneers drive stakes into the parapet.\nConstruction {work}%\nNeeds 12 uninterrupted seconds in an uncontested friendly trench\nSuppression interrupts construction\nTwo defenders reduce frontal bullet hit chance by 22%\nShellfire destroys the works; flame and flanking bypass them"),TEXT("工兵在壕肩钉入木桩。\n施工 {work}%\n无争夺友壕内连续作业 12 秒；压制中断施工\n留守两人，正面枪弹命中率降低 22%\n炮火可毁工事；火焰、侧翼绕过掩护")).Replace(TEXT("{work}"),*FString::FromInt(FMath::RoundToInt(V.Work*100.f)));
    if(V.Kind==EKind::Stores)return Rules+(V.RewardTaken?L(TEXT("The relief section assigned to this army has already arrived."),TEXT("配属本方的携弹增援已经抵达。")):
        L(TEXT("Grenadiers bring ammunition up the communication trench.\nOne grenade squad arrives without spending infantry readiness\nEach army receives this reinforcement once"),TEXT("携弹兵沿交通壕赶来。\n一支投弹小队从后方增援，不占用步兵整备\n双方各能得到一次增援")));
    if(V.Kind==EKind::AntiAircraft)return Rules+L(TEXT("The gun follows the passing aircraft.\nDefenders can drive off aircraft crossing the position\nReleased bombs continue to fall\nSmoke, shellfire or infantry assault can silence the gun"),TEXT("枪口追随掠过的敌机。\n留守枪手可逼退飞越阵地的敌机\n已经投下的炸弹继续落地\n烟幕、炮击或步兵突击可压住枪位"));
    return Rules+L(TEXT("Timber braces the trench wall.\nStationary defenders suffer 28% fewer rifle and machine-gun hits\nDirect explosive hits and flame bypass the cover"),TEXT("木料紧抵胸墙。\n静止守军遭步枪、机枪命中率降低 28%\n爆炸直接命中和火焰可绕过掩护"));
}
bool WithAirCoverReviewV8220(AWarfareFrontlineGameMode* B,int32 GunTeam,TFunctionRef<void(const FVector&)> Body)
{
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review")))return false;
    auto* Runtime=States.Find(B);if(!Runtime||Runtime->Encounter!=B->GetEncounterId()||!Runtime->Art.IsValid())return false;
    const FState Before=*Runtime;UStaticMeshComponent* ReviewGun=nullptr;
    const float Clock=B->Elapsed;const auto Phase=B->Phase;const auto Smoke=B->SmokeZones;
    const int32 Winner=B->BattleOutroWinner;const float Outro=B->BattleOutroElapsed;const FString Message=B->LastMessage;
    ON_SCOPE_EXIT
    {
        if(IsValid(ReviewGun))ReviewGun->DestroyComponent();States.FindOrAdd(B)=Before;
        B->Elapsed=Clock;B->Phase=Phase;B->SmokeZones=Smoke;B->BattleOutroWinner=Winner;B->BattleOutroElapsed=Outro;B->LastMessage=Message;
    };
    int32 Index=INDEX_NONE;
    for(int32 I=0;I<Runtime->Sites.Num();++I)if(Runtime->Sites[I].Building.IsValid()&&Runtime->Sites[I].Building->IsRegistered())
    {Index=I;break;}
    if(Index==INDEX_NONE)return false;
    const FVector Position=Runtime->Sites[Index].Building->GetComponentLocation()+FVector(50,0,155);
    ReviewGun=Mesh(Runtime->Art.Get(),TEXT("/Game/Warfare/Vehicles/V33/SM_TankMG.SM_TankMG"),Position,FRotator(55,0,0),FVector(1));
    if(!ReviewGun||!ReviewGun->IsRegistered()||!ReviewGun->GetStaticMesh())return false;
    // Other actual sites remain physically present, but this review may not
    // damage their buildings or let a second post steal an aircraft's dwell.
    for(auto& Site:Runtime->Sites){Site.V.Owner=INDEX_NONE;Site.V.Integrity=0.f;}
    auto& Post=Runtime->Sites[Index];Post.V.Kind=EKind::AntiAircraft;Post.Gun=ReviewGun;
    Post.V.Position=Position;Post.V.Owner=GunTeam;Post.V.Garrison=2;Post.V.Integrity=GunTeam>=0&&GunTeam<=1?1.f:0.f;Post.V.Contested=false;Post.ShotAt=MAX_flt;
    Runtime->Aircraft.Empty();B->SmokeZones.Empty();B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0.f;
    Body(Position);return true;
}
void RunAirCoverChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review")))
    {Check(TEXT("v8219_air_cover_requires_exact_disposable_profile"),false);return;}
    auto* Runtime=States.Find(B);int32 Index=INDEX_NONE;
    const FState Before=Runtime?*Runtime:FState();
    UStaticMeshComponent* ReviewGun=nullptr;
    ON_SCOPE_EXIT{if(IsValid(ReviewGun))ReviewGun->DestroyComponent();if(Runtime)*Runtime=Before;};
    if(Runtime&&Runtime->Encounter==B->GetEncounterId())
        for(int32 I=0;I<Runtime->Sites.Num();++I)if(Runtime->Sites[I].Gun.IsValid()&&Runtime->Sites[I].Gun->IsRegistered())
        {if(Index==INDEX_NONE||Runtime->Sites[I].V.Kind==EKind::AntiAircraft)Index=I;if(Runtime->Sites[I].V.Kind==EKind::AntiAircraft)break;}
    // Operation 7 has dugouts, not a gun post. Mount the production AA mesh on
    // an actual registered field-site actor for this isolated dwell check; do
    // not pretend the operation naturally deploys anti-aircraft defences.
    if(Index==INDEX_NONE&&Runtime&&Runtime->Encounter==B->GetEncounterId()&&Runtime->Art.IsValid())
        for(int32 I=0;I<Runtime->Sites.Num();++I)if(Runtime->Sites[I].Building.IsValid()&&Runtime->Sites[I].Building->IsRegistered())
        {
            ReviewGun=Mesh(Runtime->Art.Get(),TEXT("/Game/Warfare/Vehicles/V33/SM_TankMG.SM_TankMG"),
                Runtime->Sites[I].Building->GetComponentLocation()+FVector(50,0,155),FRotator(55,0,0),FVector(1));
            if(ReviewGun){Runtime->Sites[I].Gun=ReviewGun;Index=I;}break;
        }
    auto* AircraftMesh=FrontlineSupportArtV36::AircraftFor(B,1);
    Check(TEXT("v8219_air_cover_has_registered_field_gun_and_actual_aircraft_asset"),Index!=INDEX_NONE&&AircraftMesh);
    if(Index==INDEX_NONE||!AircraftMesh)return;
    auto* Plane=B->GetWorld()->SpawnActor<AActor>();if(!Plane){Check(TEXT("v8219_air_cover_actual_aircraft_actor_created"),false);return;}
    auto* Body=NewObject<UStaticMeshComponent>(Plane);Plane->SetRootComponent(Body);Plane->AddInstanceComponent(Body);
    Body->SetStaticMesh(AircraftMesh);Body->SetMobility(EComponentMobility::Movable);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->RegisterComponent();
    const auto Smoke=B->SmokeZones;const auto Phase=B->Phase;
    const float Clock=B->Elapsed;const int32 Outro=B->BattleOutroWinner;const FString Message=B->LastMessage;
    const FRotator Rotation=Runtime->Sites[Index].Gun->GetComponentRotation();
    ON_SCOPE_EXIT
    {
        if(IsValid(Plane))Plane->Destroy();States.FindOrAdd(B)=Before;B->SmokeZones=Smoke;
        B->Phase=Phase;B->Elapsed=Clock;B->BattleOutroWinner=Outro;B->LastMessage=Message;
        if(Before.Sites[Index].Gun.IsValid())Before.Sites[Index].Gun->SetWorldRotation(Rotation);
    };
    auto& Post=Runtime->Sites[Index];Post.V.Kind=EKind::AntiAircraft;Post.V.Owner=0;Post.V.Garrison=2;
    Post.V.Integrity=1;Post.V.Contested=false;Post.ShotAt=MAX_flt;
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->SmokeZones.Empty();
    Plane->SetActorLocation(Post.Gun->GetComponentLocation()+FVector(0,0,1550));
    Check(TEXT("v8219_air_cover_uses_one_registered_gun_and_one_registered_aircraft"),Body->IsRegistered()&&Post.Gun->IsRegistered()&&Body->GetStaticMesh()==AircraftMesh);
    const uint32 Key=Plane->GetUniqueID();
    const auto Observe=[&](float DT){return EngageAircraft(B,1,Plane->GetActorLocation(),Key,DT);};
    for(const float Step:{.05f,.2f,.4f})
    {
        Runtime->Aircraft.Remove(Key);Runtime->Interceptions=Before.Interceptions;B->Elapsed=100.f;
        float First=-1,FirstDose=0;bool Early=false,Repeat=false;
        const int32 Count=FMath::RoundToInt(10.f/Step);
        for(int32 I=1;I<=Count;++I)
        {
            B->Elapsed=100.f+I*Step;const bool Hit=Observe(Step);
            const auto* Exposure=Runtime->Aircraft.Find(Key);
            if(Hit){if(First>=0)Repeat=true;else{First=I*Step;FirstDose=Exposure?Exposure->Seconds:0;}}
            if(Exposure&&Exposure->Seconds<.85f&&Hit)Early=true;
        }
        const FString Suffix=FString::Printf(TEXT("_%.2f"),Step);
        Check(TEXT("v8219_air_cover_actual_ten_second_window_reaches_same_dwell")+Suffix,
            First>=.85f-.0001f&&First<=.85f+Step+.0001f&&FirstDose>=.85f&&FirstDose<=.85f+Step+.0001f&&!Early&&!Repeat&&Runtime->Interceptions==Before.Interceptions+1);
        UE_LOG(LogTemp,Display,TEXT("WF_V8219_AIR_COVER frame_game_seconds=%.2f first_interception_game_seconds=%.4f actual_dwell_seconds=%.4f interceptions=%d"),Step,First,FirstDose,Runtime->Interceptions-Before.Interceptions);
    }
    Runtime->Aircraft.Remove(Key);Runtime->Interceptions=Before.Interceptions;B->Elapsed=200.f;
    const bool Initial=Observe(.05f);const float Once=Runtime->Aircraft.FindRef(Key).Seconds;bool Duplicate=false;
    for(int32 I=0;I<20;++I)Duplicate|=Observe(.05f);
    Check(TEXT("v8219_air_cover_same_game_clock_calls_do_not_double_dwell"),!Initial&&!Duplicate&&FMath::IsNearlyEqual(Once,.05f,.0001f)
        &&Runtime->Aircraft.FindRef(Key).Seconds==Once&&Runtime->Interceptions==Before.Interceptions);
    Runtime->Aircraft.Remove(Key);B->Elapsed=300.f;bool BeforeGap=false;
    for(int32 I=1;I<=6;++I){B->Elapsed=300.f+I*.1f;BeforeGap|=Observe(.1f);}
    const FVector Position=Plane->GetActorLocation();Plane->SetActorLocation(Position+FVector(4000,0,0));B->Elapsed=300.65f;
    const bool OutOfRange=Observe(.05f);Plane->SetActorLocation(Position);bool EarlyAfterGap=false,AfterGap=false;
    for(int32 I=1;I<=9;++I){B->Elapsed=300.65f+I*.1f;const bool Hit=Observe(.1f);if(I<=8)EarlyAfterGap|=Hit;else AfterGap=Hit;}
    Check(TEXT("v8219_air_cover_short_observed_gap_restarts_actual_dwell"),!BeforeGap&&!OutOfRange&&!EarlyAfterGap&&AfterGap);
    Runtime->Aircraft.Remove(Key);B->Elapsed=400.f;
    for(int32 I=1;I<=6;++I){B->Elapsed=400.f+I*.1f;Observe(.1f);}
    B->Elapsed=400.8f;const bool MissedWindow=Observe(.05f);
    Check(TEXT("v8219_air_cover_uncovered_game_window_restarts_actual_dwell"),!MissedWindow&&FMath::IsNearlyEqual(Runtime->Aircraft.FindRef(Key).Seconds,.05f,.0001f));
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FSite S;Assess(S,6,6,8);Check(TEXT("v59_contested_site_cannot_flip"),S.V.Owner==INDEX_NONE);
    Assess(S,1,0,4);Check(TEXT("v59_site_needs_real_capture_time"),S.V.Owner==INDEX_NONE&&FMath::IsNearlyEqual(S.V.Capture,.5f));
    Assess(S,1,0,4);Check(TEXT("v59_one_survivor_captures_but_cannot_operate"),S.V.Owner==0&&!Live(S));
    Assess(S,2,0,.25);Check(TEXT("v59_two_stationary_survivors_operate_site"),Live(S));
    Assess(S,0,0,.25);Check(TEXT("v59_abandoned_site_stops_working"),!Live(S));
    Assess(S,0,2,8);Check(TEXT("v59_other_army_can_recapture_site"),S.V.Owner==1&&Live(S));
    Assess(S,2,0,4);Assess(S,0,2,.25f);Assess(S,2,0,.25f);
    Check(TEXT("v59_defender_return_erases_enemy_capture_progress"),S.V.Owner==1&&S.V.Capture<.04f);
    S.V.Integrity=0;Assess(S,2,0,20);Check(TEXT("v59_destroyed_site_cannot_be_reactivated"),S.V.Owner==INDEX_NONE&&!Live(S));
    if(B)Check(TEXT("v59_air_guard_rejects_invalid_input"),!EngageAircraft(B,2,FVector::ZeroVector,1,.1f));

    // Called only from V54's absolute UserDir + volatile/no-save gate, in its
    // disposable Villers-Bretonneux encounter. Never prepare a player campaign.
    auto* Runtime=B?States.Find(B):nullptr;
    const auto* Mission=B?FrontlineCampaignV32::CurrentOperation(B):nullptr;
    const bool Isolated=B&&B->IsProgressionVolatile()&&B->GetWorld()&&Mission&&Mission->Id==13
        &&B->Phase==EFrontlinePhase::Fighting&&Runtime&&Runtime->Encounter==B->GetEncounterId();
    Check(TEXT("v59_live_checks_require_disposable_operation_13"),Isolated);if(!Isolated)return;
    const int32 Stores=Runtime->Sites.IndexOfByPredicate([](const FSite& Site){return Site.V.Kind==EKind::Stores;});
    const int32 AA=Runtime->Sites.IndexOfByPredicate([](const FSite& Site){return Site.V.Kind==EKind::AntiAircraft;});
    const bool Ready=Stores>=0&&AA>=0&&Runtime->Sites[Stores].Building.IsValid()&&Runtime->Sites[AA].Gun.IsValid();
    Check(TEXT("v59_production_begin_builds_real_stores_and_aa_components"),Ready);if(!Ready)return;
    const FState Before=*Runtime;const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldSmoke=B->SmokeZones;
    const float OldTime=B->Elapsed;const FString OldMessage=B->LastMessage;
    TArray<UStaticMesh*> OldMeshes;TArray<FTransform> OldGunTransforms;TArray<bool> OldGunVisibility;
    for(const auto& Site:Before.Sites)
    {
        OldMeshes.Add(Site.Building.IsValid()?Site.Building->GetStaticMesh():nullptr);
        OldGunTransforms.Add(Site.Gun.IsValid()?Site.Gun->GetComponentTransform():FTransform::Identity);
        OldGunVisibility.Add(Site.Gun.IsValid()&&Site.Gun->IsVisible());
    }
    ON_SCOPE_EXIT
    {
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&!OldUnits.Contains(U))U->Destroy();
        for(int32 I=0;I<Before.Sites.Num();++I)
        {
            if(auto* C=Before.Sites[I].Building.Get())C->SetStaticMesh(OldMeshes[I]);
            if(auto* C=Before.Sites[I].Gun.Get()){C->SetWorldTransform(OldGunTransforms[I]);C->SetVisibility(OldGunVisibility[I]);}
        }
        States.FindOrAdd(B)=Before;B->Units=OldUnits;B->Squads=OldSquads;B->SmokeZones=OldSmoke;B->Elapsed=OldTime;B->LastMessage=OldMessage;
    };
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();Runtime->NextAssessment=0;
    const int32 Trench=Runtime->Sites[Stores].Trench;
    auto Place=[&](AWarfareFrontlineSoldier* U,int32 At,float Y)
    {
        const float X=FrontlineFieldV32::TrenchCentreX(At,Y);
        U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88.f));
    };
    AWarfareFrontlineSoldier* Men[2]={};
    FFrontlineSquad Crew;Crew.Id=590001;Crew.Team=0;Crew.Role=EFrontlineRole::Rifle;Crew.Trench=Trench;
    for(int32 I=0;I<2;++I)
    {
        Men[I]=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
        if(auto* U=Men[I]){U->SetActorTickEnabled(false);U->Battle=B;U->Team=0;U->SquadId=Crew.Id;U->bFieldEntered=true;U->bMoving=false;Place(U,Trench,I?90.f:-90.f);B->Units.Add(U);Crew.Members.Add(U);}
    }
    B->Squads.Add(Crew);Check(TEXT("v59_live_garrison_actors_created"),Men[0]&&Men[1]);if(!Men[0]||!Men[1])return;
    auto Step=[&](int32 Count){for(int32 I=0;I<Count;++I){B->Elapsed+=.25f;Tick(B,.25f);}};
    auto& Post=Runtime->Sites[Stores];Post.V.Kind=EKind::Dugout;
    Step(31);const bool NotEarly=Post.V.Owner==INDEX_NONE;Step(1);
    Check(TEXT("v59_live_tick_captures_after_eight_seconds_and_applies_bullet_cover"),NotEarly&&Post.V.Owner==0&&Post.V.Garrison==2&&FMath::IsNearlyEqual(BulletCover(Men[0]),.72f));
    Men[1]->bFieldEntered=false;Step(1);
    Check(TEXT("v59_live_single_survivor_cannot_operate_cover"),Post.V.Garrison==1&&BulletCover(Men[0])==1.f);
    Men[1]->bFieldEntered=true;B->Squads[0].bAdvancing=true;Step(1);
    Check(TEXT("v59_live_departing_garrison_loses_protection"),Post.V.Garrison==0&&BulletCover(Men[0])==1.f);
    B->Squads[0].bAdvancing=false;Step(1);
    auto* Opponent=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    if(Opponent){Opponent->SetActorTickEnabled(false);Opponent->Battle=B;Opponent->Team=1;Opponent->bFieldEntered=true;Opponent->bMoving=true;Place(Opponent,Trench,230.f);B->Units.Add(Opponent);}
    Step(1);Check(TEXT("v59_live_advancing_opponent_contests_operating_post"),Opponent&&Post.V.Garrison==0&&BulletCover(Men[0])==1.f);
    if(Opponent){B->Units.Remove(Opponent);Opponent->Destroy();}Step(1);
    Post.V.Kind=EKind::Stores;
    const int32 BeforeRelief=B->Squads.Num(),BeforeCommands=B->RecruitCommands;
    const float RifleReady=B->RecruitReadyAt[0][int32(EFrontlineRole::Rifle)];Step(1);
    bool RearRelief=B->Squads.Num()==BeforeRelief+1;
    if(RearRelief)
    {
        const auto& Relief=B->Squads.Last();RearRelief=Relief.Team==0&&Relief.Role==EFrontlineRole::Mills&&Relief.bReinforcement&&Relief.Trench==INDEX_NONE&&Relief.LivingCount()>0;
        for(const auto& Member:Relief.Members)RearRelief&=Member.IsValid()&&!Member->bFieldEntered;
    }
    Step(8);
    Check(TEXT("v59_live_stores_spawn_one_real_rear_entry_section_without_readiness_cost"),RearRelief&&B->Squads.Num()==BeforeRelief+1&&Post.V.RewardTaken&&B->RecruitCommands==BeforeCommands&&B->RecruitReadyAt[0][int32(EFrontlineRole::Rifle)]==RifleReady);
    Post.V.Kind=EKind::Dugout;
    FFrontlineStrike Shell;Shell.Target=FVector(B->Trenches[Trench].X,0,0);Shell.Radius=100.f;
    for(int32 I=0;I<3;++I)OnBlast(B,Shell);Step(32);
    Check(TEXT("v59_live_shells_destroy_mesh_and_permanently_remove_cover"),Post.V.Integrity<=0&&Post.V.Owner==INDEX_NONE&&BulletCover(Men[0])==1.f&&Post.Building->GetStaticMesh()&&Post.Building->GetStaticMesh()->GetName().Contains(TEXT("DugoutCollapsed")));

    auto& Gun=Runtime->Sites[AA];B->Squads[0].Trench=Gun.Trench;
    for(int32 I=0;I<2;++I)Place(Men[I],Gun.Trench,I?90.f:-90.f);Step(32);
    // Exercise real range, ownership, visibility and dwell logic. Cosmetic
    // projectiles/audio are suppressed so this synchronous fixture leaves none.
    Gun.ShotAt=MAX_flt;const FVector Plane=Gun.Gun->GetComponentLocation()+FVector(0,0,1550);
    const bool LocalOnly=!EngageAircraft(B,0,Plane,590001,.1f)&&!EngageAircraft(B,1,Plane+FVector(4000,0,0),590001,.1f);
    bool BeforeDwell=false,DrivenOff=false;
    for(int32 I=0;I<9;++I){B->Elapsed+=.1f;const bool Hit=EngageAircraft(B,1,Plane,590002,.1f);if(I<8)BeforeDwell|=Hit;else DrivenOff=Hit;}
    Check(TEXT("v59_live_manned_aa_requires_local_enemy_and_sustained_exposure"),Live(Gun)&&LocalOnly&&!BeforeDwell&&DrivenOff&&Runtime->Interceptions==Before.Interceptions+1);
    FFrontlineSmokeZone Smoke;Smoke.Centre=Gun.Gun->GetComponentLocation();Smoke.Radius=600;Smoke.StartedAt=B->Elapsed-1;Smoke.ExpiresAt=B->Elapsed+5;B->SmokeZones.Add(Smoke);
    bool ThroughSmoke=false;for(int32 I=0;I<12;++I){B->Elapsed+=.1f;ThroughSmoke|=EngageAircraft(B,1,Plane,590003,.1f);}
    Check(TEXT("v59_live_smoke_screens_the_aa_gun"),!ThroughSmoke);B->SmokeZones.Empty();
    Shell.Target.X=B->Trenches[Gun.Trench].X;for(int32 I=0;I<3;++I)OnBlast(B,Shell);
    Check(TEXT("v59_live_destroyed_aa_hides_gun_and_cannot_intercept"),Gun.V.Integrity<=0&&!Gun.Gun->IsVisible()&&!EngageAircraft(B,1,Plane,590004,1.f));
}
void RunFlagChecksV64(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto* State=B?States.Find(B):nullptr;
    const bool Ready=B&&B->IsProgressionVolatile()&&State&&!State->Sites.IsEmpty();
    Check(TEXT("v69_site_flag_fixture_has_disposable_field_sites"),Ready);if(!Ready)return;
    auto& Site=State->Sites[0];const auto Before=Site;ON_SCOPE_EXIT{Site=Before;};
    bool NoCloth=true;TInlineComponentArray<UStaticMeshComponent*> Components(State->Art.Get());
    for(const auto* C:Components)if(const UStaticMesh* M=C->GetStaticMesh())NoCloth&=M->GetName()!=TEXT("SM_SiteFlag_V64");
    Check(TEXT("v69_sites_have_no_hoisted_cloth_component"),NoCloth);
    Site.V.Kind=EKind::Dugout;Site.V.Integrity=1;Site.V.Owner=INDEX_NONE;
    Check(TEXT("v69_unowned_facility_has_no_national_flag"),!Views(B)[0].ShowOwnershipFlag);
    Site.V.Owner=0;auto View=Views(B)[0];
    Check(TEXT("v69_owned_flag_anchor_above_actual_building"),View.ShowOwnershipFlag&&Site.Building.IsValid()
        &&View.MarkerPosition.Z>Site.Building->Bounds.Origin.Z+Site.Building->Bounds.BoxExtent.Z);
    Site.V.Owner=1;View=Views(B)[0];
    Check(TEXT("v69_capture_changes_flag_owner_without_animation_delay"),View.ShowOwnershipFlag&&View.Owner==1);
    Site.V.Kind=EKind::FieldWorks;Site.V.Work=.8f;const bool IncompleteHidden=!Views(B)[0].ShowOwnershipFlag;
    Site.V.Work=1.f;Check(TEXT("v69_engineer_flag_waits_for_completed_revetment"),IncompleteHidden&&Views(B)[0].ShowOwnershipFlag);
    Site.V.Integrity=0;Check(TEXT("v69_destroyed_facility_has_no_national_flag"),!Views(B)[0].ShowOwnershipFlag);
}
void RunRepairChecksV63(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank,AWarfareFrontlineSoldier* Engineer,AFrontlineTank* Threat,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!Tank||!Engineer||!Threat||!B->IsProgressionVolatile())return;
    const float BeforeTime=B->Elapsed;const auto Message=B->LastMessage;
    const int32 ThreatTeam=Threat->Team;const FVector ThreatPosition=Threat->GetActorLocation();
    ON_SCOPE_EXIT{B->Elapsed=BeforeTime;B->LastMessage=Message;Threat->Team=ThreatTeam;Threat->SetActorLocation(ThreatPosition);Tank->SetFieldRepairHoldV63(false);};
    FState Local;const float Hull=Tank->GetHullIntegrity();
    Tank->ReceiveModuleHit(EFrontlineTankModule::Machinegun,500.f,0);
    Engineer->bFieldEntered=true;Engineer->bMoving=true;Engineer->Suppression=0;
    FVector Work=FVector::ZeroVector;const bool Routed=EngineerDestinationV60(Engineer,Work);
    if(!Routed){Check(TEXT("v63_gun_repair_engineer_has_a_real_destination"),false);return;}
    Engineer->SetActorLocation(Work+FVector(0,0,88));
    RepairTick(B,Local,.25f);const FVector Before=Tank->GetActorLocation();Tank->Simulate(.1f);
    Check(TEXT("v63_gun_repair_parks_mobile_hull_when_engineer_arrives"),Routed&&Tank->IsHeldForFieldRepairV63()
        &&FVector::DistSquared2D(Before,Tank->GetActorLocation())<.01f&&!Tank->IsImmobilized());
    Engineer->bMoving=false;RepairTick(B,Local,.25f);
    Threat->Team=1-Tank->Team;Threat->SetActorLocation(Tank->GetActorLocation()+FVector(800,0,0));
    RepairTick(B,Local,.25f);
    Check(TEXT("v63_nearby_enemy_armour_cancels_repair_and_parking"),!Tank->IsHeldForFieldRepairV63()
        &&Local.Repairs.FindRef(Tank).Work==0);
    Threat->Team=ThreatTeam;Threat->SetActorLocation(ThreatPosition);
    Engineer->Suppression=.5f;RepairTick(B,Local,.25f);
    Check(TEXT("v63_pinned_engineer_releases_repair_hold"),!Tank->IsHeldForFieldRepairV63());
    Engineer->Suppression=0;
    for(int32 I=0;I<33;++I){B->Elapsed+=.25f;RepairTick(B,Local,.25f);}
    Check(TEXT("v63_eight_seconds_restores_half_upgraded_gun_once_without_hull_heal"),
        FMath::IsNearlyEqual(Tank->GetMachinegunIntegrity(0),30.f*FrontlineArmourV63::GunIntegrityScale(B,Tank->Team))
        &&Tank->GetHullIntegrity()==Hull&&Local.Repairs.IsEmpty()&&!Tank->IsHeldForFieldRepairV63());
    Tank->ReceiveModuleHit(EFrontlineTankModule::Machinegun,500.f,0);
    Check(TEXT("v63_gun_recovery_cannot_be_repeated"),Tank->RepairableMachinegunV63()==INDEX_NONE&&!Tank->RepairMachinegunV63(0));
}
void RunChecksV60(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* M=B?FrontlineCampaignV32::CurrentOperation(B):nullptr;
    auto* S=B?States.Find(B):nullptr;auto* Vehicles=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
    const bool Ready=B&&B->IsProgressionVolatile()&&B->Phase==EFrontlinePhase::Fighting&&B->GetFactionForTeam(0)==EFrontlineFaction::British
        &&M&&M->Id==8&&S&&S->Encounter==B->GetEncounterId()&&Vehicles&&Vehicles->GetVehicles().IsEmpty();
    Check(TEXT("v60_runtime_fixture_requires_disposable_cambrai_with_no_existing_vehicles"),Ready);if(!Ready)return;
    const int32 CannonIndex=S->Sites.IndexOfByPredicate([](const FSite& Site){return Site.V.Kind==EKind::Casemate;});
    const int32 WorksIndex=S->Sites.IndexOfByPredicate([B](const FSite& Site){return Site.V.Kind==EKind::FieldWorks&&Site.Trench==B->GetHomeTrenchIndex(0);});
    Check(TEXT("v60_actual_casemate_and_three_revetment_components_exist"),CannonIndex>=0&&WorksIndex>=0&&S->Sites[CannonIndex].Gun.IsValid()&&S->Sites[WorksIndex].Revetments.Num()==3);
    if(CannonIndex<0||WorksIndex<0||!S->Sites[CannonIndex].Gun.IsValid())return;
    const auto Before=*S;const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldSmoke=B->SmokeZones;const auto OldStrikes=B->Strikes;const auto OldTrenches=B->Trenches;
    const auto OldTime=B->Elapsed;const auto OldMessage=B->LastMessage;
    const int32 OldMortars=B->MortarRoundsResolved;const float OldPressure=B->PeakSuppression;
    const int32 OldCasualties[2]={B->Casualties[0],B->Casualties[1]};
    const float OldMorale[2]={B->Morale[0],B->Morale[1]},OldDamage[2]={B->DamageDealt[0],B->DamageDealt[1]},OldFriendly=B->FriendlyFireDamage;
    ON_SCOPE_EXIT
    {
        for(auto& Strike:B->Strikes)if(Strike.Projectile.IsValid())Strike.Projectile->Destroy();
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&!OldUnits.Contains(U))U->Destroy();
        Vehicles->ResetBattle();B->Units=OldUnits;B->Squads=OldSquads;B->SmokeZones=OldSmoke;B->Strikes=OldStrikes;B->Trenches=OldTrenches;
        B->Elapsed=OldTime;B->LastMessage=OldMessage;B->MortarRoundsResolved=OldMortars;B->PeakSuppression=OldPressure;
        for(int32 Team=0;Team<2;++Team){B->Casualties[Team]=OldCasualties[Team];B->Morale[Team]=OldMorale[Team];B->DamageDealt[Team]=OldDamage[Team];}B->FriendlyFireDamage=OldFriendly;
        // The caller abandons this dedicated encounter immediately. In
        // particular no damaged mesh or repair receipt reaches another battle.
        States.FindOrAdd(B)=Before;
    };
    B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->SmokeZones.Empty();B->Elapsed=200;S->NextAssessment=0;
    auto Make=[&](int32 Team,int32 Id,EFrontlineRole Role)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;B->Units.Add(U);return U;
    };
    auto* First=Make(0,600001,EFrontlineRole::Pioneer);auto* Second=Make(0,600001,EFrontlineRole::Pioneer);auto* Enemy=Make(1,600002,EFrontlineRole::Rifle);
    Check(TEXT("v60_runtime_infantry_actors_initialized"),First&&Second&&Enemy);if(!First||!Second||!Enemy)return;
    Second->MemberIndex=1;FFrontlineSquad Crew;Crew.Id=600001;Crew.Team=0;Crew.Role=EFrontlineRole::Pioneer;Crew.Members={First,Second};B->Squads.Add(Crew);
    auto Place=[&](AWarfareFrontlineSoldier* U,int32 Trench,float Y){const float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));};
    auto Step=[&](int32 Count){for(int32 I=0;I<Count;++I){B->Elapsed+=.25f;Tick(B,.25f);}};
    auto& Cannon=S->Sites[CannonIndex];B->Squads[0].Trench=Cannon.Trench;Place(First,Cannon.Trench,-100);Place(Second,Cannon.Trench,100);
    First->bMoving=Second->bMoving=true;
    Check(TEXT("v63_local_trench_slot_adjustments_do_not_abandon_a_garrison"),Garrison(B,Cannon.Trench,0)==2);
    First->bMoving=Second->bMoving=false;
    Enemy->bFieldEntered=false;Enemy->SetActorLocation(FVector(-14000,0,5000));Step(32);
    const FVector Pivot=Cannon.Building->GetComponentLocation()+FVector(103.5f,0,100),Muzzle=Pivot+FVector(140,0,0);
    Check(TEXT("v60_capture_keeps_earthwork_fixed_and_gun_on_central_axis"),Cannon.Building->GetComponentRotation().Equals(FRotator::ZeroRotator,.01f)&&Cannon.Gun->GetComponentLocation().Equals(Pivot,.01f));
    FVector Aim;bool Line=false;
    for(float X=1000;X<=3200&&!Line;X+=200)for(float Y=-1100;Y<=1000&&!Line;Y+=175)
    {
        const float PX=Muzzle.X+X;const FVector P(PX,Y,AWarfareFrontlineGround::HeightAt(PX,Y)+100);
        if(GunLine(B,*S,Muzzle,P,0)){Aim=P;Line=true;}
    }
    Check(TEXT("v60_production_casemate_has_actual_forward_terrain_firing_line"),Line);if(!Line)return;
    Enemy->bFieldEntered=true;Enemy->SetActorLocation(Aim-FVector(0,0,12));
    FFrontlineSmokeZone Screen;Screen.Centre=Muzzle;Screen.Radius=600;Screen.StartedAt=B->Elapsed-1;Screen.ExpiresAt=B->Elapsed+20;B->SmokeZones.Add(Screen);
    Step(1);Check(TEXT("v60_smoke_prevents_casemate_launch_and_ammunition_spend"),B->Strikes.IsEmpty()&&Cannon.V.Ammunition==8);B->SmokeZones.Empty();
    Step(1);const bool Fired=B->Strikes.Num()==1&&Cannon.V.Ammunition==7;
    Check(TEXT("v60_two_capturing_crew_launch_real_direct_fire_shell"),Fired&&B->Strikes[0].bDirectFire&&B->Strikes[0].Projectile.IsValid()&&B->Strikes[0].ImpactAt>B->Elapsed&&B->Strikes[0].Team==0);
    if(Fired)
    {
        auto& Shell=B->Strikes[0];Enemy->SetActorLocation(Shell.Target+FVector(0,0,88));Enemy->Suppression=0;
        B->Elapsed=Shell.ImpactAt;B->ResolveFieldStrikeV32(Shell);
        Check(TEXT("v60_casemate_impact_uses_actual_explosion_and_suppression_pipeline"),Shell.bResolved&&(Enemy->Suppression>0||!Enemy->IsAlive()));
        Enemy->Health=100;Enemy->Suppression=0;Enemy->SetActorLocation(Aim-FVector(0,0,12));
    }
    const int32 Shots=B->Strikes.Num();Step(12);
    Check(TEXT("v60_casemate_cannot_fire_again_inside_six_seconds"),B->Strikes.Num()==Shots);
    Second->bFieldEntered=false;Step(32);
    Check(TEXT("v60_casemate_with_one_crew_holds_fire"),B->Strikes.Num()==Shots);Second->bFieldEntered=true;
    const bool RearRejected=!GunLine(B,*S,Muzzle,Muzzle+FVector(-1800,0,0),0),CloseRejected=!GunLine(B,*S,Muzzle,Muzzle+FVector(300,0,0),0);
    Check(TEXT("v60_casemate_has_real_rear_and_close_assault_blind_spots"),RearRejected&&CloseRejected);
    Cannon.V.Ammunition=0;Step(32);
    Check(TEXT("v60_empty_casemate_stays_silent_with_a_full_garrison"),Cannon.V.Garrison==2&&B->Strikes.Num()==Shots);Cannon.V.Ammunition=7;
    FFrontlineStrike Blast;Blast.Target=FVector(B->Trenches[Cannon.Trench].X,0,0);Blast.Radius=100;
    for(int32 I=0;I<3;++I)OnBlast(B,Blast);Step(32);
    Check(TEXT("v60_destroyed_casemate_never_schedules_another_shell"),Cannon.V.Integrity<=0&&!Cannon.Gun->IsVisible()&&B->Strikes.Num()==Shots);

    Enemy->bFieldEntered=false;Enemy->SetActorLocation(FVector(-14000,0,5000));
    auto& Works=S->Sites[WorksIndex];B->Squads[0].Trench=Works.Trench;Place(First,Works.Trench,-100);Place(Second,Works.Trench,100);
    First->Suppression=Second->Suppression=0;Step(24);
    int32 Visible=0;for(const auto& W:Works.Revetments)Visible+=W.IsValid()&&W->IsVisible();
    Check(TEXT("v60_engineering_builds_visible_partial_revetments_without_early_cover"),Works.V.Work>.45f&&Works.V.Work<.55f&&Visible==1);
    First->Suppression=Second->Suppression=.5f;Step(1);
    Check(TEXT("v60_suppressed_engineers_interrupt_unfinished_construction"),Works.V.Work==0);
    First->Suppression=Second->Suppression=0;Step(48);
    Enemy->bFieldEntered=true;Enemy->SetActorLocation(First->GetActorLocation()+FVector(1800,0,0));
    const float Front=PreparedFrontCoverV60(Enemy,First);Enemy->SetActorLocation(First->GetActorLocation()-FVector(1800,0,0));
    Check(TEXT("v60_completed_revetments_protect_only_the_stationary_front"),Works.V.Work==1&&FMath::IsNearlyEqual(Front,.78f)&&PreparedFrontCoverV60(Enemy,First)==1.f);
    Blast.Target.X=B->Trenches[Works.Trench].X;for(int32 I=0;I<3;++I)OnBlast(B,Blast);Step(48);
    Check(TEXT("v60_shell_destroyed_revetments_do_not_regenerate"),Works.V.Integrity<=0&&PreparedFrontCoverV60(Enemy,First)==1.f);

    Enemy->bFieldEntered=false;Enemy->SetActorLocation(FVector(-14000,0,5000));Vehicles->ResetReadinessForDebug(0);
    const bool Recruited=Vehicles->Recruit(EFrontlineVehicle::MarkIVMale,0);
    AFrontlineTank* Tank=Recruited&&!Vehicles->GetVehicles().IsEmpty()?Vehicles->GetVehicles().Last().Get():nullptr;
    Check(TEXT("v60_repair_fixture_recruits_an_actual_tank"),Tank!=nullptr);if(!Tank)return;
    const float X=B->Trenches[Works.Trench].X+800.f;Tank->SetActorLocation(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)));
    Tank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,240.f);B->Squads[0].bAdvancing=true;B->Squads[0].Trench=INDEX_NONE;
    First->SetActorLocation(Tank->GetActorLocation()+FVector(-70,-325,88));Second->SetActorLocation(Tank->GetActorLocation()+FVector(70,-325,88));
    FVector Detour;const bool Routed=EngineerDestinationV60(First,Detour);
    Check(TEXT("v60_marching_engineers_receive_a_local_actual_repair_destination"),Routed&&B->SoldierDestination(First).Equals(Detour,.01f)&&Tank->IsImmobilized());
    Step(24);const auto Half=RepairViewsV60(B);
    Check(TEXT("v60_track_repair_exposes_real_partial_work"),Half.Num()==1&&Half[0].Working&&Half[0].Work>.45f&&Half[0].Work<.55f&&Tank->IsImmobilized());
    First->Suppression=Second->Suppression=.5f;Step(1);
    const auto Interrupted=RepairViewsV60(B);Check(TEXT("v60_pressure_interrupts_track_repair"),Interrupted.Num()==1&&!Interrupted[0].Working&&Interrupted[0].Work==0);
    First->Suppression=Second->Suppression=0;Step(48);
    const float Hull=Tank->GetHullIntegrity();Check(TEXT("v60_engineers_restore_half_track_integrity_and_mobility"),!Tank->IsImmobilized()&&Tank->GetTrackIntegrity(0)==90.f);
    Check(TEXT("v63_completed_track_repair_removes_the_actual_hud_job"),RepairViewsV60(B).IsEmpty());
    Tank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,240.f);
    Check(TEXT("v60_track_repair_is_once_per_track_and_never_heals_hull"),Tank->IsImmobilized()&&!Tank->CanRepairTrackV60(0)&&!Tank->RepairTrackV60(0)&&Tank->GetHullIntegrity()==Hull);
}
}

