#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFrontlineEnvironmentV40.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineMotionV34.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineCharacterV34.h"
#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineFieldKitV31.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineMineV8214.h"
#include "WarfareFrontlineGameplayFocusedV84412.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineInfantryV45.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "WarfareFrontlineBankSurfaceV36.inl"
#include "WarfareFrontlineBankSurfaceV42.inl"

// Production Spawn registers the actual hull in the fleet. This scoped fixture
// is reachable only from the disposable V833 review below.
struct FFrontlineEngineerMineV833Review
{
    UFrontlineVehicleSubsystem* Fleet=nullptr;
    TArray<TObjectPtr<AFrontlineTank>> Previous;
    TArray<AFrontlineTank*> Made;
    struct FVisibility{AFrontlineTank* Tank;bool Hidden,Collision;};TArray<FVisibility> Visibility;
    int32 Serial=0,Deployments[2]={};
    explicit FFrontlineEngineerMineV833Review(AWarfareFrontlineGameMode* B)
    {
        Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return;
        Previous=Fleet->Vehicles;Serial=Fleet->NextSerial;
        for(int32 T=0;T<2;++T)Deployments[T]=Fleet->DeploymentCountV37[T];
        for(AFrontlineTank* Tank:Previous)if(IsValid(Tank))
        {Visibility.Add({Tank,Tank->IsHidden(),Tank->GetActorEnableCollision()});Tank->SetActorHiddenInGame(true);Tank->SetActorEnableCollision(false);}
        Fleet->Vehicles.Reset();
    }
    AFrontlineTank* Spawn(int32 Team)
    {
        if(!Fleet)return nullptr;
        auto* Tank=Fleet->Spawn(Team==0?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,Team,false);
        if(Tank)Made.Add(Tank);return Tank;
    }
    ~FFrontlineEngineerMineV833Review()
    {
        for(AFrontlineTank* Tank:Made)if(IsValid(Tank))Tank->Destroy();
        if(Fleet){Fleet->Vehicles=Previous;Fleet->NextSerial=Serial;for(int32 T=0;T<2;++T)Fleet->DeploymentCountV37[T]=Deployments[T];}
        for(const auto& V:Visibility)if(IsValid(V.Tank)){V.Tank->SetActorHiddenInGame(V.Hidden);V.Tank->SetActorEnableCollision(V.Collision);}
    }
};

namespace FrontlineFieldV32
{
namespace
{
struct FHazard
{
    FVector Centre;float Radius=100,Work=0;bool Mine=false,Cleared=false;TWeakObjectPtr<UStaticMeshComponent> Visual;
    TWeakObjectPtr<AWarfareFrontlineSoldier> WorkPioneerV8219;FVector WorkFeetV8219=FVector::ZeroVector;
    TWeakObjectPtr<AWarfareFrontlineSoldier> ApproachPioneerV833;
    int32 ClearedTeamV84412=INDEX_NONE,ClearedSquadV84412=INDEX_NONE;
};
struct FJetContactV33 {TWeakObjectPtr<AWarfareFrontlineSoldier> Target;FVector Point,Direction;float At=0;int32 Team=0;bool Hit=false;FFrontlineKillSourceV65 SourceV65;};
struct FBankContactV36
{
    TWeakObjectPtr<UStaticMeshComponent> Component;
    const BankSurfaceV36::FMesh* Mesh=nullptr;
    FTransform Transform;
    FBox Bounds{ForceInit};
    int32 Instance=INDEX_NONE;
};
struct FUnitState
{
    TWeakObjectPtr<USkeletalMeshComponent> Mask;
    TWeakObjectPtr<UStaticMeshComponent> Canister,Pack,Lance,Helmet,DroppedWeapon;
    TWeakObjectPtr<UPointLightComponent> FlameLight;
    float GasCheck=0,FlameStarted=0,FlameUntil=0,FlameReady=0,Fuel=12,OutcomeStarted=-1,DropAt=-1;
    bool MaskOn=false,OutcomePlayed=false,ReserveSpent=false,HelmetReleased=false,FuelIgnited=false,FuelSpent=false;
    float BurnAge=100,BurnSeconds=0,FuelBurstAt=-1;int32 IgnitedBy=-1;FVector JetDirection=FVector::ForwardVector;
    FRandomStream Random{321917};
    FGuid MineEncounterV833;
    int32 MineIndexV833=INDEX_NONE;
    FVector MineDestinationV833=FVector::ZeroVector;
    FVector MineCentreV833=FVector::ZeroVector;
    bool MineWorkerV833=false;
};
struct FState
{
    FGuid Encounter;TArray<FHazard> Hazards;TWeakObjectPtr<AActor> ArtOwner;
    TMap<TWeakObjectPtr<UStaticMeshComponent>,FTransform> OriginalProps;
    TMap<TWeakObjectPtr<UStaticMeshComponent>,ECollisionEnabled::Type> OriginalCollision;
    TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>,TArray<FTransform>> OriginalInstances;
    TArray<FBankContactV36> BankContacts;
    TMap<FIntPoint,TArray<int32>> BankCells;
    float HazardAt=0;int32 ClearedMines=0,ClearedWire=0,TriggeredMines=0;bool TrainSpawned=false;
    int32 PioneerMinesByTeam[2]={},PioneerWireByTeam[2]={};
    int32 ClearanceBurstsV8311=0;FVector LastClearanceCentreV8311=FVector::ZeroVector;
    TWeakObjectPtr<AWarfareFrontlineImpact> LastClearanceFXV8311;
    int32 FuelBursts=0,BurningCasualties=0;TWeakObjectPtr<AFrontlineFuelFireV33> Fire;
    TArray<FJetContactV33> JetContacts;
    FVector ReconCentre[2]={};float ReconUntil[2]={};
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FState> Battles;
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FUnitState> Units;
EFrontlineMissionTerrainV32 Terrain=EFrontlineMissionTerrainV32::Farmland;
int32 Operation=0;
constexpr float BankCellSizeV36=256.f;
FIntPoint BankCellV36(const FVector& P){return FIntPoint(FMath::FloorToInt(P.X/BankCellSizeV36),FMath::FloorToInt(P.Y/BankCellSizeV36));}
FVector BankPointV36(const BankSurfaceV36::FPoint& P){return FVector(P.X,P.Y,P.Z);}
const BankSurfaceV36::FMesh* BankMeshV36(const UStaticMeshComponent* C)
{
    if(!C||!C->GetStaticMesh())return nullptr;
    const FString Name=C->GetStaticMesh()->GetName();
    for(int32 I=0;I<3;++I)if(Name==FString::Printf(TEXT("SM_TrenchEarth_%c"),'A'+I))return BankSurfaceV36::Meshes[I];
    for(int32 I=0;I<3;++I)if(Name==FString::Printf(TEXT("SM_TrenchEarth_%c_V42"),'A'+I))return BankSurfaceV42::Meshes[I];
    return nullptr;
}
void CacheBankContactV36(FState& S,UStaticMeshComponent* C,const BankSurfaceV36::FMesh* Mesh,const FTransform& T,int32 Instance)
{
    if(!C||!C->IsVisible()||!Mesh||T.ContainsNaN()||T.GetScale3D().GetAbsMin()<.0001)return;
    FBankContactV36 Bank;Bank.Component=C;Bank.Mesh=Mesh;Bank.Transform=T;Bank.Instance=Instance;
    Bank.Bounds=FBox(BankPointV36(Mesh->Min),BankPointV36(Mesh->Max)).TransformBy(T).ExpandBy(.01);
    const int32 Index=S.BankContacts.Add(Bank);
    const FIntPoint Lo=BankCellV36(Bank.Bounds.Min),Hi=BankCellV36(Bank.Bounds.Max);
    for(int32 X=Lo.X;X<=Hi.X;++X)for(int32 Y=Lo.Y;Y<=Hi.Y;++Y)S.BankCells.FindOrAdd(FIntPoint(X,Y)).Add(Index);
}
bool BankTriangleV36(const FVector& Start,const FVector& Ray,const FVector& A,const FVector& B,const FVector& C,double& OutT)
{
    const FVector E1=B-A,E2=C-A,P=FVector::CrossProduct(Ray,E2);
    const double Det=FVector::DotProduct(E1,P);if(FMath::Abs(Det)<1.e-9)return false;
    const double Inv=1.0/Det;const FVector D=Start-A;
    const double U=FVector::DotProduct(D,P)*Inv;if(U<-.000001||U>1.000001)return false;
    const FVector Q=FVector::CrossProduct(D,E1);const double V=FVector::DotProduct(Ray,Q)*Inv;
    if(V<-.000001||U+V>1.000001)return false;
    OutT=FVector::DotProduct(E2,Q)*Inv;return OutT>=0.0&&OutT<=1.0;
}
float Ease(float T){T=FMath::Clamp(T,0.f,1.f);return T*T*(3-2*T);}
UStaticMeshComponent* Piece(AActor* Owner,const FString& Path,USceneComponent* Parent,FName Bone=NAME_None)
{
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);if(!Mesh)return nullptr;
    auto* C=NewObject<UStaticMeshComponent>(Owner);C->SetMobility(EComponentMobility::Movable);
    if(Parent)C->SetupAttachment(Parent,Bone);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);Owner->AddInstanceComponent(C);C->RegisterComponent();return C;
}
FString Kit(const TCHAR* Name){return FString::Printf(TEXT("/Game/Warfare/Characters/V32/Meshes/%s.%s"),Name,Name);}
FString Field(const TCHAR* Name){return FString::Printf(TEXT("/Game/Warfare/Field/V32/Meshes/%s.%s"),Name,Name);}
void ClearHazard(FState& State,FHazard& H)
{
    if(H.Cleared)return;H.Cleared=true;if(H.Mine)++State.ClearedMines;else ++State.ClearedWire;
    if(auto* C=H.Visual.Get()){C->SetVisibility(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
void UpdateGasKit(AWarfareFrontlineSoldier* U,FUnitState& State)
{
    const bool Trained=FrontlineMotionV40::HasIssuedMask(U);
    if(State.Canister.IsValid())State.Canister->SetVisibility(Trained);
    State.MaskOn=FrontlineMotionV40::MaskSealed(U);FVector Offset;
    const bool Visible=FrontlineMotionV40::MaskPresentation(U,Offset);
    if(State.Mask.IsValid()){State.Mask->SetVisibility(Trained&&Visible);State.Mask->SetRelativeLocation(Offset);}
}
}

void TerrainProfileV43(int32& OperationIndex,EFrontlineMissionTerrainV32& Style,bool bSet)
{
    if(bSet){Operation=OperationIndex;Terrain=Style;}
    else{OperationIndex=Operation;Style=Terrain;}
}
float TrenchCentreX(int32 Index,float Y)
{
    // Unequal fire bays with traverses. The centre, cover, foot solver and all
    // retained parapets use this same curve; there is no separate visual trench.
    const float Phase=FrontlineBattlefield::AuthoringFromActive(Index)*1.31f+Operation*.17f;
    const float Pitch=Terrain==EFrontlineMissionTerrainV32::Village?218.f:Terrain==EFrontlineMissionTerrainV32::Ridge?302.f:260.f;
    const float Bay=FMath::Sin(Y/Pitch+Phase);
    const float Traverse=FMath::Atan(Bay*3.5f)/1.2925f;
    const float Bend=Terrain==EFrontlineMissionTerrainV32::Marsh?67.f:Terrain==EFrontlineMissionTerrainV32::Village?108.f:89.f;
    return FrontlineBattlefield::TrenchX(Index)+Bend*Traverse+42.f*FMath::Sin(Y/(570.f+Operation*7.f)+Phase*.6f);
}
float TerrainLift(float X,float Y,float BaseHeight)
{
    const float Outer=Ease((FMath::Abs(Y)-1250.f)/850.f);
    switch(Terrain)
    {
        case EFrontlineMissionTerrainV32::Ridge:
            return (145.f+Operation%4*17.f)*FMath::Exp(-FMath::Square((X-350.f-(Operation%3-1)*460.f)/2300.f))
                +Outer*(95.f+70.f*FMath::Sin(X/1810.f+Operation*.31f));
        case EFrontlineMissionTerrainV32::Marsh:
            return -39.f*(.5f+.5f*FMath::Sin(X/1360.f+Y/960.f+Operation*.41f))
                -Outer*19.f+9.f*FMath::Sin(X/740.f-Y/1350.f);
        case EFrontlineMissionTerrainV32::Canal:
            return -68.f*FMath::Exp(-FMath::Square((X-1850.f)/400.f))*(1-Ease((FMath::Abs(Y)-850.f)/300.f))
                +Outer*110.f*FMath::Exp(-FMath::Square((X+900.f)/2450.f));
        case EFrontlineMissionTerrainV32::RailCutting:
            // Sleepers bottom at world Z19.5. A narrow graded railway bed
            // supports them across shell holes and trench intersections.
            return (19.5f-BaseHeight)*(1-Ease((FMath::Abs(Y+1130.f)-125.f)/95.f));
        case EFrontlineMissionTerrainV32::Village:
            return Outer*(55.f+46.f*FMath::Sin(X/850.f+Operation*.52f)+28.f*FMath::Cos(X/530.f))
                +11.f*FMath::Sin(X/910.f-Y/1220.f);
        default:
            return 36.f*FMath::Sin(X/2300.f+Y/1880.f+Operation*.23f)
                +Outer*(34.f+24.f*FMath::Sin(X/640.f+Operation*.37f));
    }
}
bool TryEarthworkSurfaceV36(AWarfareFrontlineGameMode* B,const FVector& WorldPoint,float& OutHeight)
{
    if(!B||WorldPoint.ContainsNaN())return false;const auto* S=Battles.Find(B);
    if(!S||S->Encounter!=B->GetEncounterId())return false;
    const auto* Candidates=S->BankCells.Find(BankCellV36(WorldPoint));if(!Candidates)return false;
    bool Hit=false;double Highest=-DBL_MAX;
    for(const int32 Index:*Candidates)
    {
        const auto& Bank=S->BankContacts[Index];const auto& Box=Bank.Bounds;
        if(!Bank.Component.IsValid()||!Bank.Component->IsVisible()
            ||WorldPoint.X<Box.Min.X||WorldPoint.X>Box.Max.X||WorldPoint.Y<Box.Min.Y||WorldPoint.Y>Box.Max.Y)continue;
        if(Bank.Instance!=INDEX_NONE)
        {const auto* ISM=Cast<UInstancedStaticMeshComponent>(Bank.Component.Get());if(!ISM||Bank.Instance>=ISM->GetInstanceCount())continue;}
        const FVector StartWorld(WorldPoint.X,WorldPoint.Y,Box.Max.Z+1),EndWorld(WorldPoint.X,WorldPoint.Y,Box.Min.Z-1);
        const FVector Start=Bank.Transform.InverseTransformPosition(StartWorld),End=Bank.Transform.InverseTransformPosition(EndWorld),Ray=End-Start;
        const auto& Mesh=*Bank.Mesh;
        // One 16-triangle legacy row or 64-triangle V42 row; tilted banks test
        // only crossed rows. No render-mesh CPU readback or new collision.
        const int32 First=FMath::Clamp(FMath::FloorToInt((FMath::Min(Start.Y,End.Y)-Mesh.Min.Y-.001)/Mesh.RowStep),0,Mesh.Rows-1);
        const int32 Last=FMath::Clamp(FMath::FloorToInt((FMath::Max(Start.Y,End.Y)-Mesh.Min.Y+.001)/Mesh.RowStep),0,Mesh.Rows-1);
        for(int32 N=Mesh.RowStarts[First];N<Mesh.RowStarts[Last+1];++N)
        {
            const auto& Tri=Mesh.Triangles[N];double T=0;
            if(BankTriangleV36(Start,Ray,BankPointV36(Mesh.Points[Tri.A]),BankPointV36(Mesh.Points[Tri.B]),BankPointV36(Mesh.Points[Tri.C]),T))
            {Highest=FMath::Max(Highest,FMath::Lerp(StartWorld.Z,EndWorld.Z,T));Hit=true;}
        }
    }
    if(Hit)OutHeight=float(Highest);return Hit;
}
void RefreshBankContactsV42(AWarfareFrontlineGameMode* B)
{
    if(!B)return;auto& S=Battles.FindOrAdd(B);S.BankContacts.Reset();S.BankCells.Reset();
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(Cast<AWarfareFrontlineSoldier>(*It)||Cast<AFrontlineTank>(*It))continue;
        TInlineComponentArray<UStaticMeshComponent*> Cs;It->GetComponents(Cs);
        for(auto* C:Cs)if(const auto* Mesh=BankMeshV36(C))
        {
            if(auto* ISM=Cast<UInstancedStaticMeshComponent>(C))
            {for(int32 I=0;I<ISM->GetInstanceCount();++I){FTransform T;if(ISM->GetInstanceTransform(I,T,true))CacheBankContactV36(S,C,Mesh,T,I);}}
            else CacheBankContactV36(S,C,Mesh,C->GetComponentTransform(),INDEX_NONE);
        }
    }
}
TSharedRef<FJsonObject> RunBankContactChecksV42(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();const auto* S=Battles.Find(B);int32 Count=0,Visible=0,Samples=0,Hits=0;double BoundsError=0,Gap=0;
    if(S)for(const auto& Bank:S->BankContacts)
    {
        const auto* C=Bank.Component.Get();if(!C||!C->GetStaticMesh()||!C->GetStaticMesh()->GetName().EndsWith(TEXT("_V42")))continue;
        ++Count;Visible+=C->IsVisible();const FBox Source(BankPointV36(Bank.Mesh->Min),BankPointV36(Bank.Mesh->Max));
        const FBox Actual=C->GetStaticMesh()->GetBoundingBox();BoundsError=FMath::Max(BoundsError,FMath::Max((Actual.Min-Source.Min).GetAbsMax(),(Actual.Max-Source.Max).GetAbsMax()));
        // Actual exported crest vertices at three longitudinal rows. Contact
        // may be higher where two real banks overlap, but can never leave a gap.
        for(int32 Row:{24,60,96})
        {
            const FVector P=Bank.Transform.TransformPosition(BankPointV36(Bank.Mesh->Points[Row*33+16]));float Z=0;++Samples;
            if(TryEarthworkSurfaceV36(B,P,Z)){++Hits;Gap=FMath::Max(Gap,double(P.Z-Z));}
        }
    }
    J->SetBoolField(TEXT("success"),Count>0&&Visible==Count&&Samples>0&&Hits==Samples&&BoundsError<.03&&Gap<.1);
    J->SetNumberField(TEXT("visible_banks"),Visible);J->SetNumberField(TEXT("cached_banks"),Count);J->SetNumberField(TEXT("crest_samples"),Samples);J->SetNumberField(TEXT("contact_hits"),Hits);
    J->SetNumberField(TEXT("maximum_imported_mesh_vs_baked_bounds_error_cm"),BoundsError);J->SetNumberField(TEXT("maximum_crest_contact_gap_cm"),Gap);
    J->SetNumberField(TEXT("source_crest_change_bound_world_cm"),1.3);J->SetNumberField(TEXT("baked_triangle_bytes"),282714);return J;
}
void Reset(AWarfareFrontlineGameMode* B)
{
    FrontlineFieldSitesV59::Reset(B);
    FrontlineMotionV40::Reset(B);
    FrontlineMotionV41::Reset(B);
    auto& S=Battles.FindOrAdd(B);if(S.ArtOwner.IsValid())S.ArtOwner->Destroy();S.ArtOwner.Reset();S.Hazards.Reset();
    if(S.Fire.IsValid())S.Fire->Clear();S.FuelBursts=S.BurningCasualties=0;S.JetContacts.Empty();S.BankContacts.Reset();S.BankCells.Reset();
    for(auto It=Units.CreateIterator();It;++It)if(!It.Key().IsValid()||It.Key()->Battle==B)It.RemoveCurrent();
    S.Encounter=B->GetEncounterId();S.HazardAt=0;S.ClearedMines=S.ClearedWire=S.TriggeredMines=0;S.TrainSpawned=false;S.ReconUntil[0]=S.ReconUntil[1]=0;
    for(int32 T=0;T<2;++T)S.PioneerMinesByTeam[T]=S.PioneerWireByTeam[T]=0;
    S.ClearanceBurstsV8311=0;S.LastClearanceCentreV8311=FVector::ZeroVector;S.LastClearanceFXV8311.Reset();
    for(auto It=Units.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    for(auto It=Battles.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    const auto* M=FrontlineCampaignV32::CurrentOperation(B);Operation=M?M->Id:0;Terrain=M?M->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland;
    FrontlineSupportV32::Reset(B);
}
void Begin(AWarfareFrontlineGameMode* B)
{
    FrontlineEnvironmentV42::Reset(B);
    auto& S=Battles.FindOrAdd(B);S.Encounter=B->GetEncounterId();S.Fire=AFrontlineFuelFireV33::Get(B->GetWorld());
    const auto* M=FrontlineCampaignV32::CurrentOperation(B);Operation=M?M->Id:0;Terrain=M?M->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland;
    // Reset above still retires shared fire/field state. Imported national
    // scenes supply their own physical earthworks, hazards and dressing.
    if(WarfareFRRUCampaignV82::Active(B))return;
    // Retain accepted materials/meshes while fitting every module to its actual
    // bent bay. Original transforms are cached once, preventing drift on replay.
    TArray<UStaticMeshComponent*> BanksToCache;S.BankContacts.Reset();S.BankCells.Reset();
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(Cast<AWarfareFrontlineSoldier>(*It) || Cast<AFrontlineTank>(*It))continue;
        TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)
        {
            if(!C->GetStaticMesh())continue;const FString Name=C->GetStaticMesh()->GetName();
            
            const bool Bank=Name.Contains(TEXT("Parapet"))||Name.Contains(TEXT("TrenchEarth"));
            const bool Boards=Name.Contains(TEXT("Duckboard"))||(C->GetMaterial(0)&&C->GetMaterial(0)->GetName().Contains(TEXT("WetDuckboards")));
            const bool Earthwork=Name.Contains(TEXT("Revetment"))||Boards||Bank;
            if(!Earthwork)continue;const FTransform Original=S.OriginalProps.FindOrAdd(C,C->GetComponentTransform());const FVector P=Original.GetLocation();
            const int32 Authored=FMath::Clamp(FMath::RoundToInt(P.X/FrontlineBattlefield::AuthoringTrenchSpacing)+2,0,4);
            if(FMath::Abs(P.X-FrontlineBattlefield::AuthoringTrenchX(Authored))>560)continue;
            const auto Collision=S.OriginalCollision.FindOrAdd(C,C->GetCollisionEnabled());
            const int32 I=FrontlineBattlefield::ActiveFromAuthoring(Authored);
            C->SetVisibility(I>=0);C->SetCollisionEnabled(I>=0?Collision:ECollisionEnabled::NoCollision);
            if(I<0)continue;if(BankMeshV36(C))BanksToCache.Add(C);
            const float Offset=P.X-FrontlineBattlefield::AuthoringTrenchX(Authored);FTransform T=Original;FVector Q=P;Q.X=TrenchCentreX(I,P.Y)+Offset;
            // V12 banks straddle the excavation but their origin is seated
            // against the outer lip, not the lower slope at their own X.
            const float Centre=TrenchCentreX(I,P.Y);
            Q.Z=AWarfareFrontlineGround::HeightAt(Bank?Centre+(Offset>0?300.f:-300.f):Centre,Q.Y)+(Bank?-9.f:Boards?2.f:8.f);
            T.SetLocation(Q);const float Slope=(TrenchCentreX(I,P.Y+20)-TrenchCentreX(I,P.Y-20))/40.f;
            T.SetRotation(FQuat(FVector::UpVector,-FMath::Atan(Slope))*Original.GetRotation());C->SetMobility(EComponentMobility::Movable);C->SetWorldTransform(T);
        }
    }
    // Static earthworks are fixed for the operation. Cache final fitted transforms
    // once, including individual ISM transforms, never query world actors per foot.
    for(auto* C:BanksToCache)
    {
        const auto* Mesh=BankMeshV36(C);
        if(auto* ISM=Cast<UInstancedStaticMeshComponent>(C))
        {for(int32 I=0;I<ISM->GetInstanceCount();++I){FTransform T;if(ISM->GetInstanceTransform(I,T,true))CacheBankContactV36(S,C,Mesh,T,I);}}
        else CacheBankContactV36(S,C,Mesh,C->GetComponentTransform(),INDEX_NONE);
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V36_BANK_CONTACT_CACHE banks=%d cells=%d source_triangles=2304 source_bytes=29994"),S.BankContacts.Num(),S.BankCells.Num());
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
    {
        auto* Bags=It->Sandbags.Get();auto& Originals=S.OriginalInstances.FindOrAdd(Bags);
        if(Originals.IsEmpty())for(int32 I=0;I<Bags->GetInstanceCount();++I){FTransform T;Bags->GetInstanceTransform(I,T,true);Originals.Add(T);}
        Bags->ClearInstances();
        for(int32 N=0;N<Originals.Num();++N)
        {
            FTransform T=Originals[N];FVector P=T.GetLocation();const int32 Authored=FMath::Clamp(FMath::RoundToInt(P.X/FrontlineBattlefield::AuthoringTrenchSpacing)+2,0,4);
            const int32 I=FrontlineBattlefield::ActiveFromAuthoring(Authored);if(I<0)continue;
            const float Offset=P.X-FrontlineBattlefield::AuthoringTrenchX(Authored);P.X=TrenchCentreX(I,P.Y)+Offset;
            P.Z=AWarfareFrontlineGround::HeightAt(TrenchCentreX(I,P.Y)+(Offset>0?300:-300),P.Y)+19+(FMath::Abs(Offset)>221?14:0);
            T.SetLocation(P);T.SetRotation(FQuat(FVector::UpVector,-FMath::Atan((TrenchCentreX(I,P.Y+20)-TrenchCentreX(I,P.Y-20))/40.f))*T.GetRotation());
            Bags->AddInstance(T,true);
        }
        It->RebuildTerrain();
    }
    if(S.ArtOwner.IsValid())S.ArtOwner->Destroy();S.Hazards.Reset();
    auto* Owner=B->GetWorld()->SpawnActor<AActor>();S.ArtOwner=Owner;auto* Root=NewObject<USceneComponent>(Owner);Owner->SetRootComponent(Root);Owner->AddInstanceComponent(Root);Root->RegisterComponent();
    FRandomStream R(3201917+Operation*931);const float WireDensity=M?M->WireDensity:.45f,MineDensity=M?M->MineDensity:0;
    const TCHAR* Wire=TEXT("/Game/Warfare/Field/V12/Meshes/SM_FieldWire_Coil.SM_FieldWire_Coil");
    for(int32 Bank=0;Bank<FrontlineBattlefield::TrenchCount-1;++Bank)
        for(int32 N=0;N<7;++N)
        {
            if(R.FRand()>WireDensity)continue;const float X=(FrontlineBattlefield::TrenchX(Bank)+FrontlineBattlefield::TrenchX(Bank+1))*.5f+R.FRandRange(-340,340),Y=-970.f+N*315.f+R.FRandRange(-75,75);
            FHazard H;H.Centre=FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+4);H.Radius=135;
            if(auto* C=Piece(Owner,Wire,Root)){C->SetWorldLocation(H.Centre);C->SetWorldRotation(FRotator(R.FRandRange(-8,8),R.FRandRange(62,114),R.FRandRange(-8,8)));C->SetWorldScale3D(FVector(1.25,.92,.95));H.Visual=C;}
            S.Hazards.Add(H);
        }
    // Buried charges occupy scattered approaches, independent of the wire
    // stream. Reserve a clear flank and keep them off the visible wire coils.
    if(MineDensity>0&&Operation>=1)
    {
        const int32 Count=FMath::Clamp(FMath::CeilToInt(MineDensity*12.f)+2,3,8);
        FRandomStream MineRandom(82151917+Operation*1777);
        for(int32 N=0;N<Count;++N)
        {
            FHazard Mine;Mine.Mine=true;Mine.Radius=FrontlineMineV8214::TriggerRadiusCm;
            bool Placed=false;
            for(int32 Attempt=0;Attempt<32&&!Placed;++Attempt)
            {
                const int32 Gap=(N+Attempt+Operation)%(FrontlineBattlefield::TrenchCount-1);
                const float X=FMath::Lerp(FrontlineBattlefield::TrenchX(Gap)+450.f,FrontlineBattlefield::TrenchX(Gap+1)-450.f,MineRandom.FRandRange(.12f,.88f));
                Mine.Centre=FVector(X,MineRandom.FRandRange(-1000.f,650.f),0);
                Placed=true;
                for(const auto& Other:S.Hazards)if(FVector::DistSquared2D(Mine.Centre,Other.Centre)<FMath::Square(Other.Mine?450.f:600.f)){Placed=false;break;}
            }
            if(!Placed)continue;
            Mine.Centre.Z=AWarfareFrontlineGround::HeightAt(Mine.Centre.X,Mine.Centre.Y)+2;
            // A buried live charge has no visible plaque or outline. Retain
            // its source mesh for review, without revealing it to either army.
            S.Hazards.Add(Mine);
        }
        UE_LOG(LogTemp,Display,TEXT("WF_V8215_SCATTERED_MINES operation=%d requested=%d clear_flank_y=800"),Operation,Count);
    }
    const bool HasEnvironmentV40=FrontlineEnvironmentV42::Build(B,Owner)||FrontlineEnvironmentV40::Build(B,Owner);
    FrontlineTerrainV42::Apply(B);
    FrontlineFieldSitesV59::Begin(B);
    // Retain the prior field dressing only as a complete packaged fallback.
    if(!HasEnvironmentV40)
    {
    // Composed shell-struck coppices and revetment rubble occupy the middle
    // distance, leaving manoeuvre lanes legible. Each operation changes density.
    for(int32 N=0;N<(Terrain==EFrontlineMissionTerrainV32::Village?30:18);++N)
    {
        const float X=R.FRandRange(-9800,9800),Y=R.FRandRange(-2200,-1570);
        const FString Path=N%3?TEXT("/Game/Warfare/Field/V12/Meshes/SM_SplitStump_V12.SM_SplitStump_V12"):TEXT("/Game/Warfare/Field/V12/Meshes/SM_BrokenRevetment_C.SM_BrokenRevetment_C");
        if(auto* C=Piece(Owner,Path,Root)){C->SetWorldLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)));C->SetWorldRotation(FRotator(N%3?0:18,R.FRandRange(0,360),N%3?R.FRandRange(-14,14):62));C->SetWorldScale3D(FVector(R.FRandRange(.72,1.28)));}
    }
    if(M&&M->bArmoredTrain)
        for(int32 N=-26;N<=26;++N)if(auto* C=Piece(Owner,TEXT("/Game/Warfare/Vehicles/V33/SM_RailSection_V33.SM_RailSection_V33"),Root))C->SetWorldLocation(FVector(N*500.f,-1130,30));
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V32_FIELD_READY mission=%d hazards=%d terrain=%d"),Operation,S.Hazards.Num(),int32(Terrain));
}
void InitializeUnit(AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Soldier)return;auto& S=Units.FindOrAdd(U);S.Random.Initialize(321917+U->SquadId*733+U->MemberIndex*71+U->Team*992);
    // National kits are bound by their dated presentation consumer. Retain only
    // this actor's field clocks and random stream; no British/German equipment.
    if(WarfareFRRUCampaignV82::Active(U->Battle))return;
    const bool German=U->GetFaction()==EFrontlineFaction::German;
    S.Canister=Piece(U,Kit(German?TEXT("SM_German_MaskTin_V32"):TEXT("SM_British_RespiratorBag_V32")),U->Soldier,TEXT("spine_02"));
    const FString MaskPath=FString::Printf(TEXT("/Game/Warfare/Characters/V33/Meshes/SK_%s_Respirator_V33.SK_%s_Respirator_V33"),German?TEXT("German"):TEXT("British"),German?TEXT("German"):TEXT("British"));
    auto* FittedMask=FrontlineCharacterV34::TryLoad(MaskPath);
    if(!FittedMask)FittedMask=LoadObject<USkeletalMesh>(nullptr,*MaskPath);
    if(auto* Mesh=FittedMask)
    {auto* C=NewObject<USkeletalMeshComponent>(U);C->SetupAttachment(U->Soldier);C->SetSkeletalMeshAsset(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);U->AddInstanceComponent(C);C->RegisterComponent();C->SetLeaderPoseComponent(U->Soldier);S.Mask=C;}
    if(S.Mask.IsValid())S.Mask->SetVisibility(false);if(S.Canister.IsValid())S.Canister->SetVisibility(FrontlineMotionV40::HasIssuedMask(U));
    if(U->Role==EFrontlineRole::Pioneer)
        S.Pack=Piece(U,Kit(TEXT("SM_PioneerTools_V32")),U->Soldier,TEXT("spine_02"));
    if(IsFlameOperator(U))
    {
        S.Pack=Piece(U,Kit(German?TEXT("SM_WexPack_V32"):TEXT("SM_SchiltPack_V32")),U->Soldier,TEXT("spine_02"));
        // The lance uses the rifle's existing grip coordinates and authored aim
        // action. Its stockless mesh replaces only weapon surfaces at import.
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Characters/V32/Meshes/SK_%s_Flame_V32.SK_%s_Flame_V32"),German?TEXT("German"):TEXT("British"),German?TEXT("German"):TEXT("British"));
        auto* RefinedBody=FrontlineCharacterV34::TryLoad(Path);
        if(!RefinedBody)RefinedBody=LoadObject<USkeletalMesh>(nullptr,*Path);
        if(auto* Mesh=RefinedBody)
        {
            // The equipment-only variant deliberately imports no generated rig.
            // Preserve the accepted body's physics source across the mesh swap;
            // both flame and explosive deaths then use the established pose-aware handoff.
            auto* BaseMesh=U->Soldier->GetSkeletalMeshAsset();auto* BasePhysics=U->Soldier->GetPhysicsAsset();
            bool SameRig=BaseMesh&&BaseMesh->GetSkeleton()==Mesh->GetSkeleton()&&BaseMesh->GetRefSkeleton().GetNum()==Mesh->GetRefSkeleton().GetNum();
            if(SameRig)for(int32 I=0;I<Mesh->GetRefSkeleton().GetNum();++I)
                SameRig&=BaseMesh->GetRefSkeleton().GetBoneName(I)==Mesh->GetRefSkeleton().GetBoneName(I)&&BaseMesh->GetRefSkeleton().GetParentIndex(I)==Mesh->GetRefSkeleton().GetParentIndex(I);
            if(SameRig&&BasePhysics&&!BasePhysics->SkeletalBodySetups.IsEmpty())
            {
                U->Soldier->EmptyOverrideMaterials();U->Soldier->SetSkeletalMeshAsset(Mesh);U->Soldier->SetPhysicsAsset(BasePhysics,true);
                FrontlineMaterialV27::Apply(U);
                UE_LOG(LogTemp,Display,TEXT("WF_V33_FLAME_PHYSICS_BOUND squad=%d team=%d source=%s"),U->SquadId,U->Team,*BasePhysics->GetPathName());
            }
            else UE_LOG(LogTemp,Error,TEXT("WF_V33_FLAME_RIG_CONTRACT_FAILURE squad=%d same_rig=%d source=%s: preserving accepted body."),U->SquadId,SameRig,*GetNameSafe(BasePhysics));
        }

    }
    if(U->Role==EFrontlineRole::Flamethrower&&!German)
    {
        S.Helmet=Piece(U,Kit(TEXT("SM_Adrian_French_V32")),U->Soldier,TEXT("Head"));
        for(int32 I=0;I<U->Soldier->GetNumMaterials();++I)if(auto* Material=U->Soldier->GetMaterial(I))
        {
            const auto& Slots=U->Soldier->GetSkeletalMeshAsset()->GetMaterials();const FString N=Slots.IsValidIndex(I)&&Slots[I].MaterialInterface?Slots[I].MaterialInterface->GetName():Material->GetName();auto* M=Cast<UMaterialInstanceDynamic>(Material);if(!M)M=U->Soldier->CreateDynamicMaterialInstance(I);if(!M)continue;
            if(N.Contains(TEXT("Cloth"))||N.Contains(TEXT("Lining")))M->SetVectorParameterValue(TEXT("PersonalWeatheringTint"),FLinearColor(.33f,.78f,1.92f));
            if(N.Contains(TEXT("Gear")))M->SetScalarParameterValue(TEXT("HelmetDropped"),1.f);
        }
        if(auto* Helmet=S.Helmet.Get())for(int32 I=0;I<Helmet->GetNumMaterials();++I)if(auto* M=Helmet->CreateDynamicMaterialInstance(I))M->SetVectorParameterValue(TEXT("EquipmentColor"),FLinearColor(.070,.089,.108));
    }
}
bool IsFlameOperator(const AWarfareFrontlineSoldier* U){return U&&U->Role==EFrontlineRole::Flamethrower&&U->MemberIndex==0;}
bool QueryFlameStateV82(const AWarfareFrontlineSoldier* U,FFrontlineFlameStateV82& Out)
{
    const auto* S=U?Units.Find(const_cast<AWarfareFrontlineSoldier*>(U)):nullptr;
    if(!U||!U->Battle||!IsFlameOperator(U)||!S||!FMath::IsFinite(U->Battle->Elapsed))return false;
    FFrontlineFlameStateV82 Candidate;Candidate.HostElapsed=U->Battle->Elapsed;Candidate.StartedAt=S->FlameStarted;
    Candidate.JetEndsAt=S->FlameUntil;Candidate.ReadyAt=S->FlameReady;Candidate.FuelCharges=S->Fuel;Candidate.Direction=S->JetDirection;
    Candidate.BurnSeconds=S->BurnSeconds;Candidate.BurnAge=S->BurnAge;Candidate.FuelBurstAt=S->FuelBurstAt;
    Candidate.Alive=U->IsAlive();Candidate.Bracing=U->IsActiveCombatant()&&Candidate.HostElapsed<S->FlameUntil;
    Candidate.JetActive=Candidate.Bracing&&Candidate.HostElapsed>=S->FlameStarted;
    Candidate.FuelIgnited=S->FuelIgnited;Candidate.FuelSpent=S->FuelSpent;Out=Candidate;return true;
}
void OnFatalHitV33(AWarfareFrontlineSoldier* U,EFrontlineDamageKind Kind,int32 Attacker,const FVector& ForceDirection)
{
    if(!U||!U->Battle)return;auto& S=Units.FindOrAdd(U);auto& B=Battles.FindOrAdd(U->Battle);
    if(!S.DroppedWeapon.IsValid())S.DroppedWeapon=FrontlineCharacterV34::ReleaseWeapon(U,IsFrontlineExplosiveDamage(Kind),ForceDirection);
    if(Kind==EFrontlineDamageKind::Fire)
    {
        if(S.BurnSeconds<=0)++B.BurningCasualties;S.BurnAge=0;S.BurnSeconds=7.5f+U->GetMotionPhase()*2.5f;
        // The bound cloth/skin receiver chars during the burn's second half;
        // a fatal hit itself must not overwrite the soldier's original dye.
        const bool German=U->GetFaction()==EFrontlineFaction::German;
        if(!S.DroppedWeapon.IsValid())if(auto* C=Piece(U,Kit(German?TEXT("SM_DroppedRifle_German_V32"):TEXT("SM_DroppedRifle_British_V32")),nullptr))
        {C->SetWorldTransform(U->Soldier->GetSocketTransform(TEXT("weapon_rifle")));C->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);C->SetCollisionObjectType(ECC_PhysicsBody);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);C->SetSimulatePhysics(true);C->SetPhysicsLinearVelocity(U->GetActorForwardVector()*45+FVector(0,0,-20));C->SetLinearDamping(.65f);S.DroppedWeapon=C;}
        U->Soldier->HideBoneByName(TEXT("weapon_rifle"),EPhysBodyOp::PBO_None);
    }
    // Fuel is finite and the valve closes with the operator. Ordinary bullets
    // never turn a pressure pack into a Hollywood bomb. Flame or a lethal blast
    // can rupture/ignite its remaining fuel, once, after a readable short delay.
    if(IsFlameOperator(U)&&!S.FuelIgnited&&S.Fuel>0&&(Kind==EFrontlineDamageKind::Fire||IsFrontlineExplosiveDamage(Kind)))
    {S.FuelIgnited=true;S.FuelBurstAt=U->Battle->Elapsed+(Kind==EFrontlineDamageKind::Fire?1.05f+U->GetMotionPhase()*.40f:.38f+U->GetMotionPhase()*.42f);S.IgnitedBy=Attacker;S.FlameUntil=0;S.BurnAge=0;S.BurnSeconds=10;}
}
void WithScopedCombatEventsV839(AWarfareFrontlineGameMode* B,TFunctionRef<void()> Work)
{if(!B||!B->IsProgressionVolatile())return;const auto BeforeUnits=Units;const auto BeforeBattles=Battles;ON_SCOPE_EXIT{Units=BeforeUnits;Battles=BeforeBattles;};Work();}
UAnimationAsset* BurningFallV33(AWarfareFrontlineSoldier* U)
{
    const TCHAR* Name=U&&U->GetMotionPhase()>.5f?TEXT("A_BurningFall_Right_V33"):TEXT("A_BurningFall_Left_V33");
    return FrontlineMotionV34::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V33/Animations/%s.%s"),Name,Name));
}
float FuelCookoffRemainingV41(const AWarfareFrontlineSoldier* U,bool& Ignited,bool& Spent)
{
    Ignited=Spent=false;if(!U||!U->Battle)return 0;
    const auto* S=Units.Find(const_cast<AWarfareFrontlineSoldier*>(U));if(!S)return 0;
    Ignited=S->FuelIgnited;Spent=S->FuelSpent;return Ignited&&!Spent?FMath::Max(0.f,S->FuelBurstAt-U->Battle->Elapsed):0.f;
}
float MovementScale(AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle)return 1;const auto* S=Battles.Find(U->Battle);if(!S)return 1;
    for(const auto& H:S->Hazards)if(!H.Cleared&&!H.Mine&&FVector::DistSquared2D(U->Feet(),H.Centre)<FMath::Square(H.Radius+25))return U->Role==EFrontlineRole::Pioneer?.72f:.47f;
    return IsFlameOperator(U)?(U->Battle->IsInFriendlySmoke(U->Feet(),U->Team)?1.12f:1.f):1.f;
}
static float MineSearchFrontageV84412(const AWarfareFrontlineGameMode* B)
{
    return B&&!WarfareFRRUCampaignV82::Active(B)?600.f:FrontlineMineV8214::SearchFrontageCmV833;
}
static bool MineWithinWorkedFrontageV84412(const AWarfareFrontlineSoldier* U,const FState& S,const FVector& Mine)
{
    if(!U||!U->Battle||WarfareFRRUCampaignV82::Active(U->Battle.Get()))return false;
    // Extend from bodies which have actually entered this local work area and
    // from ground this same section really cleared. Neither an order to a
    // distant lane nor an explosion creates knowledge of the whole minefield.
    const auto* B=U->Battle.Get();float Lo=U->Feet().Y,Hi=Lo;
    for(const auto& Mate:B->Units)if(IsValid(Mate)&&Mate->IsActiveCombatant()&&Mate->Team==U->Team
        &&Mate->SquadId==U->SquadId&&Mate->Role==EFrontlineRole::Pioneer&&Mate->bFieldEntered
        &&FMath::Abs(Mate->Feet().X-U->Feet().X)<=1200.f&&FMath::Abs(Mate->Feet().Z-U->Feet().Z)<=220.f)
    {Lo=FMath::Min(Lo,float(Mate->Feet().Y));Hi=FMath::Max(Hi,float(Mate->Feet().Y));}
    for(const auto& H:S.Hazards)if(H.Cleared&&H.Mine&&H.ClearedTeamV84412==U->Team&&H.ClearedSquadV84412==U->SquadId
        &&FMath::Abs(H.Centre.X-U->Feet().X)<=1200.f&&FMath::Abs(H.WorkFeetV8219.Z-U->Feet().Z)<=220.f)
    {
        Lo=FMath::Min(Lo,float(FMath::Min(H.Centre.Y,H.WorkFeetV8219.Y)));
        Hi=FMath::Max(Hi,float(FMath::Max(H.Centre.Y,H.WorkFeetV8219.Y)));
    }
    const float Frontage=MineSearchFrontageV84412(B);
    return Mine.Y>=Lo-Frontage&&Mine.Y<=Hi+Frontage;
}
bool HasReachableHazardV52(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||U->Role!=EFrontlineRole::Pioneer||!U->IsActiveCombatant())return false;
    const auto* B=U->Battle.Get();const auto* S=Battles.Find(U->Battle);
    const auto* Squad=B->FindSquad(U->SquadId);
    if(!S||S->Encounter!=B->GetEncounterId()||!Squad||!Squad->bAdvancing||Squad->bHoldPosition)return false;
    if(HasMineApproachV833(U))return true;
    FVector Route=B->SoldierDestination(U)-U->Feet();Route.Z=0;
    const float Length=Route.Size2D();if(Length<25.f||Route.X*B->GetTeamDirection(U->Team)<=0)return false;
    Route/=Length;
    // Bound the work approach to the next twenty metres and this formation lane.
    // Wire beside/behind the man or beyond his destination cannot justify a rush.
    for(const auto& H:S->Hazards)if(!H.Cleared)
    {
        const FVector Delta=H.Centre-U->Feet();
        const bool LocalMine=H.Mine&&!WarfareFRRUCampaignV82::Active(B);
        const float Along=LocalMine?Delta.X*B->GetTeamDirection(U->Team):FVector::DotProduct(Delta,Route);
        const float Across=LocalMine?FMath::Abs(Delta.Y):FMath::Abs(Delta.X*Route.Y-Delta.Y*Route.X);
        // SoldierDestination already stops 320 cm before the selected mine.
        // Keep that mine eligible as work, rather than interpreting the safe
        // stand-off as a completed route. Wire keeps its original route bound.
        const float WorkReach=H.Mine?350.f:0.f;
        const bool InFrontage=H.Mine&&!WarfareFRRUCampaignV82::Active(B)?(Across<=MineSearchFrontageV84412(B)||MineWithinWorkedFrontageV84412(U,*S,H.Centre)):
            Across<=FMath::Min(220.f,H.Radius+60.f);
        if(Along>=-25.f&&Along<=FMath::Min(2000.f,Length+WorkReach)&&InFrontage&&FMath::Abs(Delta.Z)<=220.f)return true;
    }
    return false;
}
static bool PioneerMayApproachV833(const AWarfareFrontlineSoldier* U)
{
    if(!IsValid(U)||!U->Battle||U->Role!=EFrontlineRole::Pioneer||!U->IsActiveCombatant())return false;
    const auto* Section=U->Battle->FindSquad(U->SquadId);
    return Section&&Section->bAdvancing&&!Section->bHoldPosition;
}
static bool PioneerCanWorkV833(const AWarfareFrontlineSoldier* U)
{
    if(!PioneerMayApproachV833(U)||U->bMoving||U->GetMotionSpeed()>20.f||U->WeaponPhase!=EFrontlineWeaponPhase::Ready)return false;
    FVector WreckExit;
    if(FrontlineArmourV63::PioneerEgressDestinationV8445(U,WreckExit))return false;
    for(const AWarfareFrontlineSoldier* Enemy:U->Battle->Units)
        if(Enemy&&Enemy->Team!=U->Team&&Enemy->IsActiveCombatant()
            &&FVector::DistSquared2D(U->Feet(),Enemy->Feet())<FMath::Square(650.f))return false;
    if(auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))
        for(const AFrontlineTank* Tank:Fleet->GetVehicles())if(Tank&&Tank->IsOperational()&&Tank->HasEnteredField()&&!Tank->IsHidden())
        {
            // Hull distance matters for the long early tanks; their centre can
            // still be distant while their nose has reached the working party.
            if(Tank->Team!=U->Team&&FVector::DistSquared2D(U->Feet(),Tank->GrenadeAimPointFor(U->Feet()))<=FMath::Square(650.f))return false;
            if(Tank->Team==U->Team&&Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation(),Tank->GetActorRotation().Yaw))return false;
        }
    FVector Yield;
    // Keep the existing passing lease alive until the body has cleared the
    // friendly hull. A mine job must not short-circuit that physical escape.
    if(FrontlineMobilityV41::ArmourYieldDestinationV8218(const_cast<AWarfareFrontlineSoldier*>(U),Yield))return false;
    return true;
}
bool HasMineApproachV833(const AWarfareFrontlineSoldier* U)
{
    if(!PioneerMayApproachV833(U))return false;
    const auto* State=Battles.Find(U->Battle);const auto* KitState=Units.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    return State&&KitState&&State->Encounter==U->Battle->GetEncounterId()
        &&KitState->MineEncounterV833==State->Encounter&&State->Hazards.IsValidIndex(KitState->MineIndexV833)
        &&State->Hazards[KitState->MineIndexV833].Mine&&!State->Hazards[KitState->MineIndexV833].Cleared
        &&State->Hazards[KitState->MineIndexV833].Centre.Equals(KitState->MineCentreV833,.01f);
}
bool MineWorkDestinationV8215(const AWarfareFrontlineSoldier* U,FVector& Destination)
{
    if(!PioneerMayApproachV833(U))return false;
    const auto* B=U->Battle.Get();auto* S=Battles.Find(B);
    if(!S||S->Encounter!=B->GetEncounterId())return false;
    auto& KitState=Units.FindOrAdd(const_cast<AWarfareFrontlineSoldier*>(U));
    const int32 Dir=B->GetTeamDirection(U->Team);
    int32 Selected=HasMineApproachV833(U)?KitState.MineIndexV833:INDEX_NONE;
    if(Selected==INDEX_NONE)
    {
        KitState.MineIndexV833=INDEX_NONE;
        FVector Route=Destination-U->Feet();Route.Z=0;const float Length=Route.Size2D();
        if(Length<25.f||Route.X*Dir<=0)return false;Route/=Length;
        float Nearest=FMath::Min(1200.f,Length);
        for(int32 Index=0;Index<S->Hazards.Num();++Index)
        {
            const auto& H=S->Hazards[Index];if(!H.Mine||H.Cleared)continue;
            const FVector D=H.Centre-U->Feet();
            // A section's next formation slot can lie diagonally across the
            // battlefield. It must not turn a known mine ahead on the advance
            // axis into a charge behind the worker merely by Y projection.
            const bool LocalMine=!WarfareFRRUCampaignV82::Active(B);
            const float Along=LocalMine?D.X*Dir:FVector::DotProduct(D,Route);
            const float Across=LocalMine?FMath::Abs(D.Y):FMath::Abs(D.X*Route.Y-D.Y*Route.X);
            if(Along>=-25.f&&Along<Nearest&&(Across<=MineSearchFrontageV84412(B)||MineWithinWorkedFrontageV84412(U,*S,H.Centre))
                &&FMath::Abs(D.Z)<=220.f){Selected=Index;Nearest=Along;}
        }
    }
    if(Selected==INDEX_NONE)return false;
    auto& Mine=S->Hazards[Selected];
    if(!PioneerMayApproachV833(Mine.ApproachPioneerV833.Get()))
    {
        AWarfareFrontlineSoldier* Best=nullptr;float Closest=MAX_flt;
        for(AWarfareFrontlineSoldier* Candidate:B->Units)if(PioneerMayApproachV833(Candidate)&&Candidate->Team==U->Team&&Candidate->SquadId==U->SquadId)
        {
            if(HasMineApproachV833(Candidate))if(const auto* OtherJob=Units.Find(Candidate);OtherJob&&OtherJob->MineIndexV833!=Selected)continue;
            const FVector D=Mine.Centre-Candidate->Feet();const float Ahead=D.X*Dir;
            if(Ahead < -25.f||Ahead>1200.f||(FMath::Abs(D.Y)>MineSearchFrontageV84412(B)&&!MineWithinWorkedFrontageV84412(Candidate,*S,Mine.Centre))||FMath::Abs(D.Z)>220.f)continue;
            const float Score=D.SizeSquared2D();
            if(Score<Closest){Best=Candidate;Closest=Score;}
        }
        Mine.ApproachPioneerV833=Best?Best:const_cast<AWarfareFrontlineSoldier*>(U);
    }
    const bool Worker=Mine.ApproachPioneerV833.Get()==U;
    if(KitState.MineEncounterV833!=S->Encounter||KitState.MineIndexV833!=Selected||KitState.MineWorkerV833!=Worker)
    {
        KitState.MineEncounterV833=S->Encounter;KitState.MineIndexV833=Selected;KitState.MineWorkerV833=Worker;KitState.MineCentreV833=Mine.Centre;
        // One worker approaches the charge. His section waits on the near side
        // in its own lanes, rather than stacking every body at one work point.
        const float StandOff=Worker?FrontlineMineV8214::WorkStandOffCmV833:500.f;
        FVector Point=Mine.Centre-FVector(Dir*StandOff,0,0);
        if(!Worker)Point.Y=Destination.Y;
        if((Point.X-U->Feet().X)*Dir < -10.f)
        {
            Point=U->Feet();
            if(Worker&&FVector::DistSquared2D(Point,Mine.Centre)>=FMath::Square(FrontlineMineV8214::WorkReachCmV833))Point.Y=Mine.Centre.Y;
        }
        Point.Z=FrontlineInfantryV45::SurfaceHeight(const_cast<AWarfareFrontlineGameMode*>(B),Point);
        KitState.MineDestinationV833=Point;
    }
    Destination=KitState.MineDestinationV833;
    return true;
}
bool IsMineWorkingV8215(const AWarfareFrontlineSoldier* U)
{
    if(!PioneerCanWorkV833(U))return false;
    if(const auto* S=Battles.Find(U->Battle))for(const auto& H:S->Hazards)
        if(H.Mine&&!H.Cleared&&(!PioneerMayApproachV833(H.ApproachPioneerV833.Get())||H.ApproachPioneerV833.Get()==U)
            &&FVector::DistSquared2D(U->Feet(),H.Centre)<FMath::Square(FrontlineMineV8214::WorkReachCmV833))return true;
    return false;
}
bool QueryMineWorkV835(const AWarfareFrontlineSoldier* U,FFrontlineMineWorkStateV835& Out)
{
    Out=FFrontlineMineWorkStateV835();
    if(!PioneerMayApproachV833(U))return false;
    const auto* S=Battles.Find(U->Battle);if(!S||S->Encounter!=U->Battle->GetEncounterId())return false;
    const FUnitState* Kit=Units.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    const bool Routed=HasMineApproachV833(U);
    const FHazard* Mine=Routed?&S->Hazards[Kit->MineIndexV833]:nullptr;
    const FHazard* RoutedMine=Mine;
    // Keep the existing unreserved, in-reach worker path visible too. Querying
    // never creates a route or takes a reservation from another pioneer.
    const bool CanWork=PioneerCanWorkV833(U);
    const auto WorkContact=[U](const FHazard& H)
    {
        return H.Mine&&!H.Cleared
            &&(!PioneerMayApproachV833(H.ApproachPioneerV833.Get())||H.ApproachPioneerV833.Get()==U)
            &&FVector::DistSquared2D(U->Feet(),H.Centre)<FMath::Square(FrontlineMineV8214::WorkReachCmV833);
    };
    if(CanWork&&(!Mine||!WorkContact(*Mine)))
    {
        float Closest=FMath::Square(FrontlineMineV8214::WorkReachCmV833);
        for(const auto& H:S->Hazards)if(WorkContact(H))
        {
            const float D=FVector::DistSquared2D(U->Feet(),H.Centre);
            if(D<Closest){Closest=D;Mine=&H;}
        }
    }
    if(!Mine)return false;
    Out.MineCentre=Mine->Centre;Out.StandPoint=Mine==RoutedMine?Kit->MineDestinationV833:U->Feet();
    Out.DesignatedWorker=!PioneerMayApproachV833(Mine->ApproachPioneerV833.Get())||Mine->ApproachPioneerV833.Get()==U;
    const FVector Delta=Mine->Centre-U->Feet();
    Out.Direction=Delta.GetSafeNormal2D();if(Out.Direction.IsNearlyZero())Out.Direction=FVector(U->Battle->GetTeamDirection(U->Team),0,0);
    const bool Working=CanWork&&Out.DesignatedWorker&&Delta.SizeSquared2D()<FMath::Square(FrontlineMineV8214::WorkReachCmV833);
    Out.Phase=Working?EFrontlineMineWorkPhaseV835::Working:
        (Out.DesignatedWorker&&(U->bMoving||U->GetMotionSpeed()>20.f)?EFrontlineMineWorkPhaseV835::Approaching:EFrontlineMineWorkPhaseV835::Waiting);
    // An arm's-length probe scans the near-side soil, not the mine 3.2m away.
    const float Reach=FMath::Clamp(float(Delta.Size2D())-Mine->Radius-20.f,0.f,72.f);
    Out.ToolContact=U->Feet()+Out.Direction*Reach;
    Out.ToolContact.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Out.ToolContact);
    Out.HostElapsed=U->Battle->Elapsed;Out.WorkSeconds=Mine->Work;
    Out.Progress=FMath::Clamp(Mine->Work/FrontlineMineV8214::WorkSecondsV833,0.f,1.f);
    return true;
}
static void TickHazardsV8219(AWarfareFrontlineGameMode* B,FState& S,float DT);
void RunMineRulesChecksV8215(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B);
    Check(TEXT("v8215_mine_rules_require_disposable_nosave_profile"),Safe);if(!Safe)return;
    auto& S=Battles.FindOrAdd(B);const FState OldState=S;const auto OldUnits=B->Units;const auto OldSquads=B->Squads;
    const auto OldPhase=B->Phase;const float OldClock=B->Elapsed;const int32 OldWinner=B->BattleOutroWinner;
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    ON_SCOPE_EXIT{if(IsValid(U))U->Destroy();S=OldState;B->Units=OldUnits;B->Squads=OldSquads;B->Elapsed=OldClock;B->Phase=OldPhase;B->BattleOutroWinner=OldWinner;};
    Check(TEXT("v8215_mine_work_actual_pioneer_actor"),U!=nullptr);if(!U)return;
    B->Units.Empty();B->Squads.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    U->Initialize(B,821599,0,EFrontlineRole::Pioneer,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;
    const int32 Dir=B->GetTeamDirection(0);
    FVector Mine((B->Trenches[B->GetHomeTrenchIndex(0)].X+B->Trenches[B->GetHomeTrenchIndex(0)+Dir].X)*.5f,500,0);
    Mine.Z=AWarfareFrontlineGround::HeightAt(Mine.X,Mine.Y)+2.f;
    FVector Start=Mine-FVector(Dir*900,0,0);Start.Z=AWarfareFrontlineGround::HeightAt(Start.X,Start.Y);
    U->SetActorLocation(Start+FVector(0,0,88));B->Units.Add(U);
    FFrontlineSquad Section;Section.Id=U->SquadId;Section.Team=0;Section.Role=EFrontlineRole::Pioneer;Section.bAdvancing=true;
    Section.Destination=B->GetHomeTrenchIndex(0)+Dir;Section.Slot=1;Section.Members.Add(U);B->Squads.Add(Section);
    S.Hazards.Empty();S.TrainSpawned=true;S.JetContacts.Empty();S.Encounter=B->GetEncounterId();
    FHazard H;H.Mine=true;H.Centre=Mine;H.Radius=FrontlineMineV8214::TriggerRadiusCm;S.Hazards.Add(H);
    const FVector HostedDestination=B->SoldierDestination(U);
    Check(TEXT("v8215_pioneer_retains_work_contact_after_host_destination_standoff"),
        FMath::IsNearlyEqual(float(FVector::Dist2D(HostedDestination,Mine)),320.f,.1f)&&HasReachableHazardV52(U));
    FVector Destination=Mine+FVector(Dir*1000,0,0);
    const bool Routed=MineWorkDestinationV8215(U,Destination);
    Check(TEXT("v8215_pioneer_intercepts_buried_mine_at_safe_forward_work_point"),Routed
        &&FMath::IsNearlyEqual(float(FVector::Dist2D(Destination,Mine)),320.f,.1f)&&FVector::Dist2D(Destination,Mine)>H.Radius);
    U->SetActorLocation(Destination+FVector(0,0,88));
    Check(TEXT("v8215_pioneer_work_requires_actual_still_body_in_reach"),IsMineWorkingV8215(U));
    const int32 Triggered=S.TriggeredMines,Cleared=S.ClearedMines;
    S.HazardAt=B->Elapsed;TickHazardsV8219(B,S,0.f);
    for(int32 I=0;I<8;++I){B->Elapsed+=.26f;Tick(B,.26f);}
    Check(TEXT("v8215_pioneer_clears_real_mine_within_two_seconds_without_detonation"),S.Hazards[0].Cleared
        &&S.ClearedMines==Cleared+1&&S.TriggeredMines==Triggered&&U->IsAlive()&&!IsMineWorkingV8215(U));
    Section=B->Squads[0];B->Squads[0].Role=EFrontlineRole::Rifle;U->Role=EFrontlineRole::Rifle;
    S.Hazards[0]=H;Destination=Mine+FVector(Dir*1000,0,0);
    Check(TEXT("v8215_regular_infantry_do_not_gain_hidden_mine_route_knowledge"),!MineWorkDestinationV8215(U,Destination)&&!IsMineWorkingV8215(U));
}
void RunPioneerChecksV52(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile()){Check(TEXT("v52_pioneer_fixture_requires_volatile_campaign"),false);return;}
    auto& Field=Battles.FindOrAdd(B);const auto OldHazards=Field.Hazards;const auto OldEncounter=Field.Encounter;
    const int32 OldWire=Field.ClearedWire,OldMines=Field.ClearedMines;
    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldSmoke=B->SmokeZones;const auto OldPhase=B->Phase;
    const auto OldTrenches=B->Trenches;
    TArray<AWarfareFrontlineSoldier*> Fixtures;
    ON_SCOPE_EXIT{for(auto* U:Fixtures)if(IsValid(U))U->Destroy();Field.Hazards=OldHazards;Field.Encounter=OldEncounter;Field.ClearedWire=OldWire;Field.ClearedMines=OldMines;B->Units=OldUnits;B->Squads=OldSquads;B->SmokeZones=OldSmoke;B->Phase=OldPhase;B->Trenches=OldTrenches;};
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;Field.Hazards.Empty();Field.Encounter=B->GetEncounterId();
    for(int32 I=0;I<B->Trenches.Num();++I)B->Trenches[I].Owner=I==B->GetHomeTrenchIndex(0)?0:I==B->GetHomeTrenchIndex(1)?1:INDEX_NONE;
    auto Make=[&](int32 Team,EFrontlineRole Role,int32 Id)
    {auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(U){U->Initialize(B,Id,Team,Role,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;B->Units.Add(U);Fixtures.Add(U);}return U;};
    auto* U=Make(0,EFrontlineRole::Pioneer,95290);auto* Enemy=Make(1,EFrontlineRole::Rifle,95291);
    Check(TEXT("v52_pioneer_actual_contact_fixture_ready"),U&&Enemy);if(!U||!Enemy)return;
    U->FormationOffset=FVector::ZeroVector;U->SetActorLocation(FVector(0,0,10088));
    FFrontlineSquad Section;Section.Id=U->SquadId;Section.Team=0;Section.Role=EFrontlineRole::Pioneer;Section.bAdvancing=true;
    Section.Trench=INDEX_NONE;Section.Destination=B->GetHomeTrenchIndex(1);Section.LastPassedTrench=B->GetHomeTrenchIndex(0);Section.Slot=1;Section.Members={U};B->Squads.Add(Section);
    const float Normal=FrontlineWeapons::ProfileFor(U).AdvanceContactCm;
    const float Distance=(Normal+1750.f)*.5f*B->EncounterVisibilityScale();
    Enemy->SetActorLocation(U->GetActorLocation()+FVector(B->GetTeamDirection(0)*Distance,0,0));
    Check(TEXT("v52_pioneer_empty_approach_retains_real_service_rifle_contact"),FrontlineWeapons::AdvanceContactRangeFor(U)==Normal&&B->AcquireTarget(U,true)==Enemy);
    FVector Route=(B->SoldierDestination(U)-U->Feet()).GetSafeNormal2D();
    FHazard Hazard;Hazard.Centre=U->Feet()+Route*500.f;Hazard.Radius=135;Field.Hazards.Add(Hazard);
    Check(TEXT("v52_live_wire_on_actual_route_allows_only_the_work_approach"),HasReachableHazardV52(U)&&FrontlineWeapons::AdvanceContactRangeFor(U)==1750.f&&B->AcquireTarget(U,true)==nullptr);
    ClearHazard(Field,Field.Hazards[0]);
    Check(TEXT("v52_cleared_wire_immediately_restores_actual_contact"),!HasReachableHazardV52(U)&&FrontlineWeapons::AdvanceContactRangeFor(U)==Normal&&B->AcquireTarget(U,true)==Enemy);
    Field.Hazards[0]=Hazard;Field.Hazards[0].Mine=true;
    const bool Mine=HasReachableHazardV52(U);Field.Hazards[0].Centre=U->Feet()-Route*500.f;const bool Behind=!HasReachableHazardV52(U);
    Field.Hazards[0].Centre=Hazard.Centre+FVector(-Route.Y,Route.X,0)*600.f;const bool Beside=!HasReachableHazardV52(U);
    Field.Hazards[0].Centre=U->Feet()+Route*2500.f;const bool Remote=!HasReachableHazardV52(U);
    Field.Hazards[0]=Hazard;B->Squads[0].bAdvancing=false;const bool Holding=!HasReachableHazardV52(U);
    Check(TEXT("v52_pioneer_live_mines_work_but_rear_side_distant_and_held_hazards_do_not_force_rush"),Mine&&Behind&&Beside&&Remote&&Holding);
}
float GasHitScale(AWarfareFrontlineSoldier* U)
{
    return FrontlineMotionV40::MaskSealed(U)?FrontlineGameplayV51::SealedGasDoseScale:1.f;
}
bool FlameDischarge(AWarfareFrontlineSoldier* U,AWarfareFrontlineSoldier* Target)
{
    if(!IsFlameOperator(U))return false;auto& S=Units.FindOrAdd(U);
    if(!U->IsActiveCombatant()||!Target||Target->Team==U->Team||!Target->IsActiveCombatant()||!U->Battle||U->Battle->Phase!=EFrontlinePhase::Fighting||S.Fuel<=0||U->Battle->Elapsed<S.FlameReady)return true;
    const bool National=WarfareFRRUCampaignV82::Active(U->Battle);
    const auto* Equipment=National?WarfareFRRUEquipmentV82::Resolve(U):nullptr;
    if(National&&(!Equipment||Equipment->WeaponId!=TEXT("fr.schilt")||!FMath::IsFinite(Equipment->Profile.FireRangeCm)
        ||!FMath::IsFinite(Equipment->Profile.AimedCycleSeconds)||Equipment->Profile.FireRangeCm<=0||Equipment->Profile.AimedCycleSeconds<1.33f))return true;
    const FVector Direction=(Target->Feet()-U->Feet()).GetSafeNormal2D();
    const float Range=National?Equipment->Profile.FireRangeCm:U->Battle->HasTechnology(18,U->Team)?1450:1250;
    if(FVector::Dist2D(U->Feet(),Target->Feet())>Range)return true;
    // Brace before opening the valve. The whole discharge owns the aiming pose;
    // formation-slot movement must not turn the operator away from a live jet.
    S.JetDirection=Direction;S.FlameStarted=U->Battle->Elapsed+.18f;S.FlameUntil=S.FlameStarted+1.15f;
    S.FlameReady=National?U->Battle->Elapsed+Equipment->Profile.AimedCycleSeconds:S.FlameUntil+3.4f;S.Fuel-=1;
    if(!National&&U->Battle->HasTechnology(18,U->Team)&&S.Fuel==0&&!S.ReserveSpent){S.Fuel=4;S.ReserveSpent=true;}
    ++U->ShotsDischarged;++U->Battle->ShotsFired;U->bEngaging=true;
    // Snapshot cone, finite fuel, no stacking wound damage. Parapets offer much
    // less protection to a close jet, while men behind the operator remain safe.
    for(AWarfareFrontlineSoldier* Other:U->Battle->Units)if(Other&&Other->IsActiveCombatant()&&Other->Team!=U->Team)
    {
        const FVector Delta=Other->Feet()-U->Feet();const float D=Delta.Size2D(),Along=FVector::DotProduct(Delta,Direction);
        if(D<=Range&&Along>0&&FVector::CrossProduct(Delta,Direction).Size()<90.f+Along*.19f&&U->Battle->HasSight(U,Other))
        {
            auto& Contact=Battles.FindOrAdd(U->Battle).JetContacts.AddDefaulted_GetRef();Contact.Target=Other;Contact.Point=Other->Feet();Contact.Direction=Direction;Contact.Team=U->Team;Contact.SourceV65=FFrontlineKillSourceV65::Soldier(U);Contact.Hit=S.Random.FRand()<.68f;Contact.At=S.FlameStarted+FMath::Clamp(D/1450.f,.06f,.9f);
        }
    }
    return true;
}
bool FlameBraceDirectionV33(AWarfareFrontlineSoldier* U,FVector& Direction)
{
    if(!IsFlameOperator(U)||!U->IsActiveCombatant()||!U->Battle||U->Battle->Phase!=EFrontlinePhase::Fighting)return false;
    const auto* S=Units.Find(U);if(!S||U->Battle->Elapsed>=S->FlameUntil)return false;
    Direction=S->JetDirection;return true;
}
void TickUnit(AWarfareFrontlineSoldier* U,float DT)
{
    auto* S=Units.Find(U);if(!S||!U->Battle)return;const float Now=U->Battle->Elapsed;
    if(!U->IsAlive()&&S->Helmet.IsValid()&&!S->HelmetReleased&&U->GetDeathState().PublishedPoseSamples>0)
    {
        S->HelmetReleased=true;auto* C=S->Helmet.Get();C->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);C->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);C->SetCollisionObjectType(ECC_PhysicsBody);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);C->SetSimulatePhysics(true);C->SetPhysicsLinearVelocity(FVector(40,30,145));C->SetLinearDamping(.7f);
    }
    if(U->IsAlive())FrontlineMotionV40::TickExposure(U,DT);
    UpdateGasKit(U,*S);
    const bool FlameActive=IsFlameOperator(U)&&U->IsAlive()&&U->Battle->Phase==EFrontlinePhase::Fighting&&Now>=S->FlameStarted&&Now<S->FlameUntil;
    auto* BattleState=Battles.Find(U->Battle);
    const bool Burning=S->BurnSeconds>0&&S->BurnAge<S->BurnSeconds;
    if(Burning&&FMath::IsFinite(DT))S->BurnAge=FMath::Min(S->BurnAge+FMath::Max(0.f,DT),S->BurnSeconds);
    if(Burning)FrontlineMaterialV27::BurnV84411(U,S->BurnAge,S->BurnSeconds);
    if((FlameActive||Burning)&&BattleState)if(auto* Fire=BattleState->Fire.Get())
    {
        // The authored muzzle's local forward axis follows the posed lance;
        // camera, actor orientation and animation can never split its origin/direction.
        if(FlameActive)
        {
            const FVector Forward=WarfareFRRUCampaignV82::Active(U->Battle)?WarfareFRRUPresentationV82::MuzzleForward(U):U->Soldier->GetSocketRotation(TEXT("WF_Muzzle")).Vector();
            if(!Forward.IsNearlyZero())Fire->EmitJet(U->GetMuzzleLocation(),Forward,DT,U->GetMotionPhase());
        }
        if(Burning)
        {Fire->EmitClothing(U,DT,FMath::Clamp(1-S->BurnAge/S->BurnSeconds,.15f,1.f));}
    }
    FrontlineMotionV41::TickOutro(U,DT);
}
void BeginOutcome(AWarfareFrontlineGameMode* B,int32 Winner)
{
    B->BattleOutroWinner=Winner;B->BattleOutroElapsed=0;
    FVector Pos=FVector::ZeroVector;int32 N=0;for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsActiveCombatant()&&U->Team==Winner){Pos+=U->Feet();++N;}
    if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayOutcomeV32(B->GetFactionForTeam(Winner)==EFrontlineFaction::German,N?Pos/N:Pos);
}
using FHazardPioneersV8219=TArray<AWarfareFrontlineSoldier*,TInlineAllocator<16>>;
static AWarfareFrontlineSoldier* HazardPioneerV8219(const FHazardPioneersV8219& Pioneers,const FHazard& H)
{
    AWarfareFrontlineSoldier* Pioneer=nullptr;float Closest=FMath::Square(H.Mine?FrontlineMineV8214::WorkReachCmV833:270.f);
    const auto* Reserved=H.Mine&&PioneerMayApproachV833(H.ApproachPioneerV833.Get())?H.ApproachPioneerV833.Get():nullptr;
    for(AWarfareFrontlineSoldier* U:Pioneers)if(U&&U->IsActiveCombatant()&&U->Role==EFrontlineRole::Pioneer
        &&(!H.Mine||(PioneerCanWorkV833(U)&&(!Reserved||Reserved==U))))
    {
        const float Distance=FVector::DistSquared2D(U->Feet(),H.Centre);
        if(Distance<Closest){Closest=Distance;Pioneer=U;}
    }
    return Pioneer;
}
static void PresentMineClearanceV8311(AWarfareFrontlineGameMode* B,FState& S,const FHazard& H)
{
    // Neutralising a charge is not an armed mine strike: this actor is cosmetic
    // and never enters the casualty, suppression or armour-damage resolver.
    const FVector At(H.Centre.X,H.Centre.Y,FrontlineInfantryV45::SurfaceHeight(B,H.Centre)+4.f);
    auto* FX=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
    if(FX){FX->ConfigureBlast(true,false);FX->Tags.Add(TEXT("WF_V8311_CONTROLLED_MINE_CLEARANCE"));
        S.LastClearanceFXV8311=FX;S.LastClearanceCentreV8311=At;++S.ClearanceBurstsV8311;}
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayImpact(true,At);
}
static void ResolveHazardsV8215(AWarfareFrontlineGameMode* B,FState& S,const FHazardPioneersV8219& Pioneers)
{
    for(auto& H:S.Hazards)if(!H.Cleared)
    {
        AWarfareFrontlineSoldier* Pioneer=HazardPioneerV8219(Pioneers,H);
        if(Pioneer)
        {
            if(H.Mine){if(H.Work>=FrontlineMineV8214::WorkSecondsV833){PresentMineClearanceV8311(B,S,H);FrontlineAchievementsV76::ObstacleCleared(B,Pioneer->Team,H.Centre,Pioneer->SquadId);++S.PioneerMinesByTeam[Pioneer->Team];H.ClearedTeamV84412=Pioneer->Team;H.ClearedSquadV84412=Pioneer->SquadId;ClearHazard(S,H);continue;}}
            else if(FrontlineMotionV40::ConsumeWireCut(Pioneer,H.Centre)){H.Work=1;FrontlineAchievementsV76::ObstacleCleared(B,Pioneer->Team,H.Centre,Pioneer->SquadId);++S.PioneerWireByTeam[Pioneer->Team];ClearHazard(S,H);continue;}
            else FrontlineMotionV40::RequestWireCut(Pioneer,H.Centre);
        }
        // Working beside a charge does not make its trigger safe. Any body or
        // track crossing it before the actual disarm still detonates the mine.
        if(H.Mine)for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsActiveCombatant()&&FVector::DistSquared2D(U->Feet(),H.Centre)<FMath::Square(H.Radius))
        {
            H.Cleared=true;++S.TriggeredMines;if(H.Visual.IsValid())H.Visual->SetVisibility(false);
            FFrontlineStrike Strike;Strike.Target=H.Centre;Strike.Radius=FrontlineMineV8214::BlastRadiusCm;Strike.Team=1-U->Team;Strike.bGrenade=true;Strike.bMineV8214=true;B->ResolveFieldStrikeV32(Strike);break;
        }
        if(H.Mine&&!H.Cleared)if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
            for(AFrontlineTank* Tank:Vehicles->GetVehicles())if(Tank&&Tank->IsOperational()&&Tank->HasEnteredField())
            {
                const FVector Local=Tank->GetActorTransform().InverseTransformPosition(H.Centre);
                if(FMath::Abs(Local.X)>330.f||FMath::Abs(Local.Y)>195.f)continue;
                H.Cleared=true;++S.TriggeredMines;if(H.Visual.IsValid())H.Visual->SetVisibility(false);
                Tank->ReceiveModuleHit(Local.Y<0?EFrontlineTankModule::LeftTrack:EFrontlineTankModule::RightTrack,240.f);
                FFrontlineStrike Strike;Strike.Target=H.Centre;Strike.Radius=FrontlineMineV8214::BlastRadiusCm;Strike.Team=1-Tank->Team;Strike.bGrenade=true;Strike.bMineV8214=true;
                B->ResolveFieldStrikeV32(Strike);break;
            }
    }
}
static void TickHazardsV8219(AWarfareFrontlineGameMode* B,FState& S,float DT)
{
    if(!FMath::IsFinite(DT)||DT<0||!FMath::IsFinite(B->Elapsed))return;
    // Collect once in unit order; wire cutting also uses moving pioneers. Recheck
    // current eligibility per hazard because an earlier blast can kill a worker.
    FHazardPioneersV8219 Pioneers;
    for(AWarfareFrontlineSoldier* U:B->Units)
        if(U&&U->Role==EFrontlineRole::Pioneer&&U->IsActiveCombatant())Pioneers.Add(U);
    // Only adjacent, observed stationary contacts own elapsed work. A newly
    // arriving worker cannot replay an unobserved interval at his new position.
    for(auto& H:S.Hazards)if(H.Mine&&!H.Cleared)
    {
        auto* Pioneer=HazardPioneerV8219(Pioneers,H);
        if(Pioneer&&H.WorkPioneerV8219.Get()==Pioneer
            &&FVector::DistSquared2D(H.WorkFeetV8219,Pioneer->Feet())<FMath::Square(12.f))
            H.Work+=DT*(B->HasTechnology(17,Pioneer->Team)?1.65f:1.f);
        H.WorkPioneerV8219=Pioneer;H.WorkFeetV8219=Pioneer?Pioneer->Feet():FVector::ZeroVector;
    }
    if(B->Elapsed<S.HazardAt)return;
    // Retain the quarter-second phase, but resolve current contacts once. Never
    // replay missed mine touches or wire-cut requests using the current body.
    S.HazardAt+=.25f*(FMath::FloorToFloat((B->Elapsed-S.HazardAt)/.25f)+1.f);
    ResolveHazardsV8215(B,S,Pioneers);
}
AWarfareFrontlineSoldier* PrepareMineWorkVisualReviewV835(AWarfareFrontlineGameMode* B)
{
    if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV835Review"))
        ||!B->GetWorld()->IsGameWorld()||WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting||B->Trenches.Num()<2
        ||!B->Units.IsEmpty()||!B->Squads.IsEmpty())return nullptr;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const AFrontlineTank* V:Fleet->GetVehicles())
        if(IsValid(V)&&V->IsOperational())return nullptr;
    auto& S=Battles.FindOrAdd(B);S.Encounter=B->GetEncounterId();S.TrainSpawned=true;S.HazardAt=B->Elapsed;
    const int32 Dir=B->GetTeamDirection(0),Home=B->GetHomeTrenchIndex(0),Next=Home+Dir;
    if(!B->Trenches.IsValidIndex(Home)||!B->Trenches.IsValidIndex(Next))return nullptr;
    FHazard Mine;Mine.Mine=true;Mine.Radius=FrontlineMineV8214::TriggerRadiusCm;
    Mine.Centre=FVector((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,500.f,0);
    Mine.Centre.Z=FrontlineInfantryV45::SurfaceHeight(B,Mine.Centre)+2.f;
    auto* Worker=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!Worker)return nullptr;
    Worker->Initialize(B,835990,0,EFrontlineRole::Pioneer,0);Worker->SetActorTickEnabled(false);
    Worker->bFieldEntered=true;Worker->FormationOffset=FVector::ZeroVector;Worker->ReturnToIdle();
    const FVector Foot=Mine.Centre-FVector(Dir*FrontlineMineV8214::WorkStandOffCmV833,0,0);
    Worker->SetActorLocation(FVector(Foot.X,Foot.Y,FrontlineInfantryV45::SurfaceHeight(B,Foot)+88.f));
    Worker->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));B->Units.Add(Worker);
    FFrontlineSquad Squad;Squad.Id=Worker->SquadId;Squad.Team=0;Squad.Role=EFrontlineRole::Pioneer;
    Squad.bAdvancing=true;Squad.bReinforcement=false;Squad.LastPassedTrench=Home;Squad.Destination=Next;Squad.Slot=2;Squad.Members={Worker};B->Squads.Add(Squad);
    S.Hazards={Mine};B->SoldierDestination(Worker);
    for(int32 Frame=0;Frame<24&&!S.Hazards[0].Cleared;++Frame)
    {
        B->Elapsed+=.05f;Worker->Tick(.05f);TickHazardsV8219(B,S,.05f);
        if(Worker->Soldier)
        {Worker->Soldier->HandleExistingParallelEvaluationTask(true,true);Worker->Soldier->TickAnimation(.05f,false);Worker->Soldier->RefreshBoneTransforms();}
        const auto Pose=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
        if(Pose->GetBoolField(TEXT("working"))&&Pose->GetNumberField(TEXT("work_elapsed"))>=.9
            &&Pose->GetBoolField(TEXT("probe_visible")))return Worker;
    }
    FrontlineMotionV40::Abort(Worker);Units.Remove(Worker);B->Units.Remove(Worker);B->Squads.RemoveAll([Worker](const FFrontlineSquad& Q){return Q.Id==Worker->SquadId;});Worker->Destroy();S.Hazards.Reset();
    return nullptr;
}
void RunEngineerMineChecksV833(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV833Review"))
        &&!WarfareFRRUCampaignV82::Active(B)&&B->GetWorld()->IsGameWorld()&&B->Phase==EFrontlinePhase::Fighting&&B->Trenches.Num()>=2;
    Check(TEXT("v833_engineer_mines_require_disposable_prepared_uk_de_battle"),Safe);if(!Safe)return;
    FFrontlineEngineerMineV833Review Armour(B);
    Check(TEXT("v833_engineer_mines_have_actual_registered_fleet"),Armour.Fleet!=nullptr);if(!Armour.Fleet)return;
    auto& State=Battles.FindOrAdd(B);const auto OldState=State;const auto OldKits=Units;
    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;
    const int32 OldShots=B->ShotsFired;
    const float OldClock=B->Elapsed,OldFriendly=B->FriendlyFireDamage,OldPeak=B->PeakSuppression;
    const float OldOutro=B->BattleOutroElapsed;const int32 OldWinner=B->BattleOutroWinner;const FString OldMessage=B->LastMessage;
    const FFrontlineCombatRecordV65 OldRecords[]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const int32 OldCasualties[]={B->Casualties[0],B->Casualties[1]};
    const float OldDamage[]={B->DamageDealt[0],B->DamageDealt[1]},OldMorale[]={B->Morale[0],B->Morale[1]};
    TArray<AWarfareFrontlineSoldier*> Made;TSet<TWeakObjectPtr<AWarfareFrontlineImpact>> OldImpacts;
    for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)OldImpacts.Add(*It);
    ON_SCOPE_EXIT
    {
        FrontlineMobilityV41::Reset(B);for(auto* U:Made)if(IsValid(U))U->Destroy();
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)if(!OldImpacts.Contains(*It))It->Destroy();
        State=OldState;Units=OldKits;B->Units=OldUnits;B->Squads=OldSquads;B->Elapsed=OldClock;
        B->ShotsFired=OldShots;
        B->BattleOutroElapsed=OldOutro;B->BattleOutroWinner=OldWinner;B->LastMessage=OldMessage;
        B->FriendlyFireDamage=OldFriendly;B->PeakSuppression=OldPeak;
        for(int32 T=0;T<2;++T){B->CombatRecordV65[T]=OldRecords[T];B->Casualties[T]=OldCasualties[T];B->DamageDealt[T]=OldDamage[T];B->Morale[T]=OldMorale[T];}
    };
    auto Make=[&](int32 Team,EFrontlineRole Role,int32 Id,int32 Member,const FVector& Feet)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
        U->FormationOffset=FVector::ZeroVector;U->SetActorLocation(FVector(Feet.X,Feet.Y,FrontlineInfantryV45::SurfaceHeight(B,Feet)+88.f));
        B->Units.Add(U);Made.Add(U);return U;
    };
    const auto EvaluatePoseV835=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();}};
    for(int32 Team=0;Team<2;++Team)
    {
        B->Units.Reset();B->Squads.Reset();B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000.f;
        State=FState();State.Encounter=B->GetEncounterId();State.TrainSpawned=true;State.HazardAt=B->Elapsed;
        const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Next=Home+Dir;
        FHazard Mine;Mine.Mine=true;Mine.Radius=FrontlineMineV8214::TriggerRadiusCm;
        Mine.Centre=FVector((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,500,0);
        Mine.Centre.Z=FrontlineInfantryV45::SurfaceHeight(B,Mine.Centre)+2.f;State.Hazards={Mine};
        auto* Worker=Make(Team,EFrontlineRole::Pioneer,833100+Team,0,Mine.Centre-FVector(Dir*900.f,0,0));
        auto* Mate=Make(Team,EFrontlineRole::Pioneer,833100+Team,1,Mine.Centre-FVector(Dir*1100.f,-150.f,0));
        Check(FString::Printf(TEXT("v833_team_%d_engineer_actual_registered_bodies"),Team),Worker&&Mate&&Worker->Soldier&&Worker->Soldier->IsRegistered());
        if(!Worker||!Mate)return;
        EvaluatePoseV835(Worker,.25f);const auto BeforeMinePose=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
        const float StandingPelvis=float(BeforeMinePose->GetNumberField(TEXT("pelvis_above_feet_cm")));
        const bool RifleWasHidden=Worker->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle"));
        FFrontlineSquad Section;Section.Id=Worker->SquadId;Section.Team=Team;Section.Role=EFrontlineRole::Pioneer;
        Section.bAdvancing=true;Section.bReinforcement=false;Section.LastPassedTrench=Home;Section.Destination=Next;Section.Slot=2;Section.Members={Worker,Mate};B->Squads.Add(Section);
        const FVector WorkPoint=B->SoldierDestination(Worker),WaitPoint=B->SoldierDestination(Mate);
        auto* DistantContact=Make(1-Team,EFrontlineRole::Rifle,833400+Team,0,Mine.Centre+FVector(Dir*1200.f,0,0));
        Check(FString::Printf(TEXT("v833_team_%d_one_worker_and_spaced_near_side_wait_point"),Team),State.Hazards[0].ApproachPioneerV833.Get()==Worker
            &&HasMineApproachV833(Worker)&&HasMineApproachV833(Mate)&&FMath::IsNearlyEqual(float(FVector::Dist2D(WorkPoint,Mine.Centre)),320.f,.1f)
            &&(Mine.Centre.X-WaitPoint.X)*Dir>=499.f&&FVector::Dist2D(WorkPoint,WaitPoint)>100.f);
        Check(FString::Printf(TEXT("v833_team_%d_local_clearance_approach_matches_immediate_threat_distance"),Team),DistantContact
            &&FrontlineWeapons::AdvanceContactRangeFor(Worker)==650.f
            &&FrontlineWeapons::ProfileFor(Worker).FireRangeCm>650.f);
        const FVector From=Worker->Feet();Worker->SetActorLocation(Worker->GetActorLocation()+FVector(Dir*30.f,25.f,0));
        Check(FString::Printf(TEXT("v833_team_%d_mine_destination_does_not_orbit_with_the_moving_route"),Team),B->SoldierDestination(Worker).Equals(WorkPoint,.01f));
        Worker->SetActorLocation(From+FVector(0,0,88));FrontlineMobilityV41::Reset(B);
        bool SafeApproach=true,Stable=true,PresentationMatches=true,GroundedProbe=true,ApproachShown=false,WorkShown=false;
        bool AnimationMatches=true,SettledProbe=true;int32 SettledFrames=0,ContinuousWorkFrames=0;
        float PelvisLow=MAX_flt,PelvisHigh=-MAX_flt,MaxGrip=0,MaxGroundGap=0,MaxGroundHeightGap=0;
        float FirstWork=-1,CompleteAt=-1;
        for(int32 Frame=0;Frame<340&&!State.Hazards[0].Cleared;++Frame)
        {
            B->Elapsed+=.05f;Worker->Tick(.05f);Mate->Tick(.05f);EvaluatePoseV835(Worker,.05f);
            Stable&=B->SoldierDestination(Worker).Equals(WorkPoint,.01f);
            SafeApproach&=FVector::Dist2D(Worker->Feet(),Mine.Centre)>Mine.Radius&&FVector::Dist2D(Mate->Feet(),Mine.Centre)>Mine.Radius;
            FFrontlineMineWorkStateV835 WorkView;const float BeforeQuery=State.Hazards[0].Work;
            const bool VisibleJob=QueryMineWorkV835(Worker,WorkView);
            PresentationMatches&=VisibleJob&&WorkView.DesignatedWorker&&WorkView.MineCentre.Equals(Mine.Centre,.01f)
                &&WorkView.StandPoint.Equals(WorkPoint,.01f)&&WorkView.HostElapsed==B->Elapsed
                &&WorkView.WorkSeconds==BeforeQuery&&State.Hazards[0].Work==BeforeQuery
                &&(WorkView.Phase==EFrontlineMineWorkPhaseV835::Working)==IsMineWorkingV8215(Worker);
            GroundedProbe&=FVector::Dist2D(Worker->Feet(),WorkView.ToolContact)<=72.01f
                &&FVector::Dist2D(WorkView.ToolContact,Mine.Centre)>=Mine.Radius+19.9f
                &&FMath::IsNearlyEqual(float(WorkView.ToolContact.Z),FrontlineInfantryV45::SurfaceHeight(B,WorkView.ToolContact),.01f);
            ApproachShown|=WorkView.Phase==EFrontlineMineWorkPhaseV835::Approaching;
            WorkShown|=WorkView.Phase==EFrontlineMineWorkPhaseV835::Working;
            const auto Action=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
            const bool WorkEligible=WorkView.Phase==EFrontlineMineWorkPhaseV835::Working;
            ContinuousWorkFrames=WorkEligible?ContinuousWorkFrames+1:0;
            // Arrival changes bMoving later than TickAction in the same tick.
            // The next ordinary tick must start work; unsafe work never gets
            // this boundary allowance, and sustained work cannot restart it.
            AnimationMatches&=!Action->GetBoolField(TEXT("working"))||WorkEligible;
            if(ContinuousWorkFrames>1)AnimationMatches&=Action->GetBoolField(TEXT("working"));
            if(Action->GetBoolField(TEXT("working"))&&Action->GetNumberField(TEXT("work_elapsed"))>=.9)
            {
                ++SettledFrames;const float Pelvis=float(Action->GetNumberField(TEXT("pelvis_above_feet_cm")));
                PelvisLow=FMath::Min(PelvisLow,Pelvis);PelvisHigh=FMath::Max(PelvisHigh,Pelvis);
                double Ground=MAX_flt,Grip=MAX_flt,Height=MAX_flt;
                Action->TryGetNumberField(TEXT("probe_ground_gap_cm"),Ground);Action->TryGetNumberField(TEXT("probe_grip_gap_cm"),Grip);
                Action->TryGetNumberField(TEXT("probe_ground_height_gap_cm"),Height);
                MaxGrip=FMath::Max(MaxGrip,float(Grip));MaxGroundGap=FMath::Max(MaxGroundGap,float(Ground));MaxGroundHeightGap=FMath::Max(MaxGroundHeightGap,float(Height));
                SettledProbe&=Action->GetBoolField(TEXT("probe_visible"))&&Action->GetBoolField(TEXT("rifle_hidden"))&&Ground<=.1&&Height<=.1&&Grip<8.;
            }
            TickHazardsV8219(B,State,.05f);
            if(State.Hazards[0].Work>0.f&&FirstWork<0.f)FirstWork=B->Elapsed;
            if(State.Hazards[0].Cleared)CompleteAt=B->Elapsed;
        }
        Check(FString::Printf(TEXT("v833_team_%d_actual_walk_stop_and_disarm_without_detonation"),Team),Stable&&SafeApproach&&State.ClearedMines==1
            &&State.TriggeredMines==0&&Worker->IsAlive()&&Mate->IsAlive()&&Worker->DistanceTravelled>300.f&&FirstWork>1000.f
            &&CompleteAt>FirstWork&&CompleteAt-FirstWork<=2.f);
        Check(FString::Printf(TEXT("v835_team_%d_mine_presentation_observes_actual_approach_work_and_host_time"),Team),ApproachShown&&WorkShown&&PresentationMatches);
        Check(FString::Printf(TEXT("v835_team_%d_mine_probe_contacts_near_side_physical_ground_without_long_reach"),Team),GroundedProbe);
        const auto EndMinePose=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
        Check(FString::Printf(TEXT("v835_team_%d_actual_mine_action_starts_once_and_matches_work_eligibility"),Team),AnimationMatches
            &&EndMinePose->GetNumberField(TEXT("work_starts"))==1.&&EndMinePose->GetNumberField(TEXT("working_frames"))>=3.);
        Check(FString::Printf(TEXT("v835_team_%d_actual_mine_work_keeps_settled_crouch_without_repeated_rise"),Team),SettledFrames>=3
            &&StandingPelvis-PelvisHigh>=25.f&&PelvisHigh-PelvisLow<10.f);
        Check(FString::Printf(TEXT("v835_team_%d_actual_mine_probe_touches_ground_and_stays_in_hand"),Team),SettledFrames>=3&&SettledProbe);
        Check(FString::Printf(TEXT("v835_team_%d_actual_mine_pose_metrics frames=%d standing=%.2f pelvis_min=%.2f pelvis_max=%.2f grip_max=%.2f ground_max=%.3f surface_max=%.3f starts=%.0f"),Team,
            SettledFrames,StandingPelvis,PelvisLow,PelvisHigh,MaxGrip,MaxGroundGap,MaxGroundHeightGap,EndMinePose->GetNumberField(TEXT("work_starts"))),SettledFrames>0&&FMath::IsFinite(PelvisLow));
        FFrontlineMineWorkStateV835 FinishedWork;
        Check(FString::Printf(TEXT("v835_team_%d_cleared_mine_removes_work_presentation_immediately"),Team),!QueryMineWorkV835(Worker,FinishedWork)
            &&FinishedWork.Phase==EFrontlineMineWorkPhaseV835::None&&FinishedWork.Progress==0.f);
        UE_LOG(LogTemp,Display,TEXT("WF_V833_MINE team=%d worker=%s workpoint=%s mate=%s work=%.3f started=%.3f completed=%.3f travel=%.1f cleared=%d triggered=%d stable=%d safe=%d"),
            Team,*Worker->Feet().ToString(),*WorkPoint.ToString(),*Mate->Feet().ToString(),State.Hazards[0].Work,FirstWork,CompleteAt,
            Worker->DistanceTravelled,State.ClearedMines,State.TriggeredMines,int32(Stable),int32(SafeApproach));
        Check(FString::Printf(TEXT("v833_team_%d_clearance_metrics work=%.3f started=%.3f completed=%.3f travel=%.1f cleared=%d triggered=%d stable=%d safe=%d"),
            Team,State.Hazards[0].Work,FirstWork,CompleteAt,Worker->DistanceTravelled,State.ClearedMines,State.TriggeredMines,int32(Stable),int32(SafeApproach)),
            FMath::IsFinite(CompleteAt)&&FMath::IsFinite(Worker->DistanceTravelled));
        // The deliberate distant enemy is now inside ordinary rifle contact.
        // Verify the release restores acquisition before removing that contact
        // to measure unopposed onward movement separately.
        Check(FString::Printf(TEXT("v833_team_%d_disarm_restores_ordinary_enemy_contact"),Team),!HasMineApproachV833(Worker)
            &&FrontlineWeapons::AdvanceContactRangeFor(Worker)>650.f&&B->AcquireTarget(Worker,true)==DistantContact);
        B->Units.Remove(DistantContact);if(DistantContact){DistantContact->SetActorHiddenInGame(true);DistantContact->SetActorEnableCollision(false);}
        const FVector AfterClear=Worker->Feet();
        for(int32 Frame=0;Frame<50;++Frame){B->Elapsed+=.05f;Worker->Tick(.05f);Mate->Tick(.05f);TickHazardsV8219(B,State,.05f);}
        Check(FString::Printf(TEXT("v833_team_%d_cleared_charge_releases_normal_forward_movement"),Team),!HasMineApproachV833(Worker)
            &&(Worker->Feet().X-AfterClear.X)*Dir>150.f&&State.ClearedMines==1&&State.TriggeredMines==0);
        const auto ResumedPose=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
        Check(FString::Printf(TEXT("v835_team_%d_completed_mine_hides_probe_restores_rifle_and_finishes_rise"),Team),!ResumedPose->GetBoolField(TEXT("working"))
            &&!ResumedPose->GetBoolField(TEXT("rising"))&&!ResumedPose->GetBoolField(TEXT("probe_visible"))
            &&Worker->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle"))==RifleWasHidden);
        Check(FString::Printf(TEXT("v833_team_%d_after_clear_metrics forward=%.1f contact_range=%.1f visibility=%.3f advancing=%d engaging=%d motion=%.1f"),Team,
            float((Worker->Feet().X-AfterClear.X)*Dir),FrontlineWeapons::AdvanceContactRangeFor(Worker),B->EncounterVisibilityScale(),int32(B->Squads[0].bAdvancing),int32(Worker->bEngaging),Worker->GetMotionSpeed()),
            !Worker->Feet().ContainsNaN());
        State.Hazards={Mine};Worker->ReturnToIdle();Worker->SetActorLocation(WorkPoint+FVector(0,0,88));B->Units={Worker};
        B->Squads[0].bAdvancing=false;TickHazardsV8219(B,State,0.f);B->Elapsed+=.5f;TickHazardsV8219(B,State,.5f);
        Check(FString::Printf(TEXT("v833_team_%d_holding_or_loading_does_not_credit_mine_work"),Team),!IsMineWorkingV8215(Worker)&&State.Hazards[0].Work==0.f);
        B->Squads[0].bAdvancing=true;Worker->WeaponPhase=EFrontlineWeaponPhase::Reload;B->Elapsed+=.5f;TickHazardsV8219(B,State,.5f);
        Check(FString::Printf(TEXT("v833_team_%d_reload_interrupts_actual_clearance"),Team),!IsMineWorkingV8215(Worker)&&State.Hazards[0].Work==0.f);Worker->ReturnToIdle();
        // Enter the actual work action before introducing the contact. A test
        // that never starts it cannot prove tool/weapon interruption cleanup.
        B->Elapsed+=.05f;Worker->Tick(.05f);EvaluatePoseV835(Worker,.05f);
        const bool WorkBeforeThreat=FrontlineMotionV40::MineWorkReviewStateV835(Worker)->GetBoolField(TEXT("working"));
        auto* Threat=Make(1-Team,EFrontlineRole::Rifle,833200+Team,0,WorkPoint+FVector(0,600,0));
        B->Elapsed+=.05f;Worker->Tick(.05f);EvaluatePoseV835(Worker,.05f);
        B->Elapsed+=.5f;TickHazardsV8219(B,State,.5f);
        Check(FString::Printf(TEXT("v833_team_%d_close_enemy_contact_interrupts_clearance"),Team),Threat&&!IsMineWorkingV8215(Worker)&&State.Hazards[0].Work==0.f);
        FFrontlineMineWorkStateV835 InterruptedWork;
        Check(FString::Printf(TEXT("v835_team_%d_enemy_contact_interrupts_work_pose_without_changing_clearance"),Team),QueryMineWorkV835(Worker,InterruptedWork)
            &&InterruptedWork.Phase!=EFrontlineMineWorkPhaseV835::Working&&InterruptedWork.WorkSeconds==0.f);
        Check(FString::Printf(TEXT("v835_team_%d_rifle_soldier_never_receives_engineer_work_presentation"),Team),!QueryMineWorkV835(Threat,InterruptedWork)
            &&InterruptedWork.Phase==EFrontlineMineWorkPhaseV835::None);
        const auto InterruptedPose=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
        Check(FString::Printf(TEXT("v835_team_%d_live_contact_aborts_actual_work_and_restores_equipment"),Team),WorkBeforeThreat
            &&!InterruptedPose->GetBoolField(TEXT("working"))&&!InterruptedPose->GetBoolField(TEXT("probe_visible"))
            &&Worker->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle"))==RifleWasHidden);
        B->Units={Worker};Worker->ReturnToIdle();
        auto* HostileHull=Armour.Spawn(1-Team);
        // Within even the darkest operation's contact distance, while its
        // tracks remain outside the charge's physical trigger rectangle.
        if(HostileHull)HostileHull->SetActorTransform(HostileHull->GroundPoseForContactV8218(WorkPoint+FVector(Dir*100.f,250.f,0),Team==0?180.f:0.f));
        const bool EnemyAcquired=HostileHull&&Armour.Fleet->AcquireInfantryTarget(Worker)==HostileHull;
        const bool EnemyMuzzle=HostileHull&&Armour.Fleet->HasInfantryFiringLineV828(Worker,HostileHull);
        B->Elapsed+=.5f;TickHazardsV8219(B,State,.5f);Worker->Tick(.05f);
        Check(FString::Printf(TEXT("v833_team_%d_live_enemy_hull_interrupts_work_and_enters_normal_combat"),Team),HostileHull&&HostileHull->AreAssetsReady()
            &&HostileHull->HasEnteredField()&&!IsMineWorkingV8215(Worker)&&State.Hazards[0].Work==0.f&&Worker->bEngaging);
        Check(FString::Printf(TEXT("v833_team_%d_enemy_hull_metrics ready=%d entered=%d acquired=%d muzzle=%d centre=%.1f hull=%.1f visibility=%.3f work=%.3f eligible=%d engaging=%d phase=%d moving=%d"),Team,
            int32(HostileHull&&HostileHull->AreAssetsReady()),int32(HostileHull&&HostileHull->HasEnteredField()),int32(EnemyAcquired),int32(EnemyMuzzle),
            HostileHull?float(FVector::Dist2D(Worker->Feet(),HostileHull->AimPoint())):-1.f,HostileHull?float(FVector::Dist2D(Worker->Feet(),HostileHull->GrenadeAimPointFor(Worker->Feet()))):-1.f,
            B->EncounterVisibilityScale(),State.Hazards[0].Work,int32(IsMineWorkingV8215(Worker)),int32(Worker->bEngaging),int32(Worker->WeaponPhase),int32(Worker->bMoving)),!Worker->Feet().ContainsNaN());
        if(HostileHull){HostileHull->SetActorHiddenInGame(true);HostileHull->SetActorEnableCollision(false);}
        Worker->ReturnToIdle();Worker->SetActorLocation(WorkPoint+FVector(0,0,88));FrontlineMobilityV41::Reset(B);
        B->Elapsed+=.05f;Worker->Tick(.05f);EvaluatePoseV835(Worker,.05f);
        const bool WorkBeforeYield=FrontlineMotionV40::MineWorkReviewStateV835(Worker)->GetBoolField(TEXT("working"));
        auto* FriendlyHull=Armour.Spawn(Team);
        FVector Yield;bool PhysicalYield=false;int32 YieldPlacement=0;
        // A long hull may bridge above a crater floor; that is not a physical
        // infantry obstruction. Find a real ground-supported near-charge body
        // contact instead of assuming an actor-centre offset guarantees one.
        if(FriendlyHull)for(float Lateral:{0.f,80.f,-80.f,160.f,-160.f})
        {
            FVector Candidate=WorkPoint+FVector(0,Lateral,0);Candidate.Z=FrontlineInfantryV45::SurfaceHeight(B,Candidate);
            for(float Behind:{150.f,300.f,450.f})
            {
                ++YieldPlacement;Worker->SetActorLocation(Candidate+FVector(0,0,88));
                FriendlyHull->SetActorTransform(FriendlyHull->GroundPoseForContactV8218(Candidate-FVector(Dir*Behind,0,0),Team==0?0.f:180.f));
                PhysicalYield=FVector::Dist2D(Worker->Feet(),Mine.Centre)<FrontlineMineV8214::WorkReachCmV833&&FriendlyHull->InfantryYieldPointV8218(Worker,Yield);
                if(PhysicalYield)break;
            }
            if(PhysicalYield)break;
        }
        const FVector BeforeYield=Worker->Feet();const bool WorkBlocked=!IsMineWorkingV8215(Worker);
        const FVector RequestedYield=Yield-BeforeYield;
        FVector GroundedStep=BeforeYield+RequestedYield.GetSafeNormal2D()*2.f;
        GroundedStep.Z=FMath::Max(BeforeYield.Z,double(FrontlineInfantryV45::SurfaceHeight(B,GroundedStep)));
        const float InitialSweep=FriendlyHull?FriendlyHull->InfantrySweepClearanceV8218(BeforeYield,GroundedStep):0.f;
        FVector LocalStart=FVector::ZeroVector,LocalStep=FVector::ZeroVector,DepthGain=FVector::ZeroVector;
        // Actual long/short hull envelopes, both passing sides: outward
        // recovery is accepted, inward displacement and fresh entry are not.
        if(auto* Body=FriendlyHull?Cast<UStaticMeshComponent>(FriendlyHull->GetRootComponent()):nullptr;Body&&Body->GetStaticMesh())
        {
            const auto Hull=Body->GetComponentTransform();const FBox Bounds=Body->GetStaticMesh()->GetBoundingBox();
            const FVector Vertical=Hull.InverseTransformVector(FVector(0,0,72.5f)),Scale=Hull.GetScale3D().GetAbs();
            const FBox Envelope=Bounds.ExpandBy(FVector(FMath::Abs(Vertical.X)+34.f/FMath::Max(.01,Scale.X),FMath::Abs(Vertical.Y)+34.f/FMath::Max(.01,Scale.Y),FMath::Abs(Vertical.Z)));
            LocalStart=Hull.InverseTransformPosition(BeforeYield+FVector(0,0,72.5f));
            const FVector LocalEnd=Hull.InverseTransformPosition(GroundedStep+FVector(0,0,72.5f));LocalStep=LocalEnd-LocalStart;
            for(int32 Axis=0;Axis<3;++Axis)DepthGain[Axis]=(FMath::Min(LocalStart[Axis]-Envelope.Min[Axis],Envelope.Max[Axis]-LocalStart[Axis])
                -FMath::Min(LocalEnd[Axis]-Envelope.Min[Axis],Envelope.Max[Axis]-LocalEnd[Axis]))*Scale[Axis];
            for(int32 Side:{-1,1})
            {
                FVector Local=Bounds.GetCenter();Local.Y+=Side*25.f;
                const FVector Inside=Hull.TransformPosition(Local)-FVector(0,0,72.5f);
                const FVector Outward=Hull.TransformPosition(Local+FVector(0,Side*5.f,0))-FVector(0,0,72.5f);
                const FVector Inward=Hull.TransformPosition(Local-FVector(0,Side*5.f,0))-FVector(0,0,72.5f);
                FVector Edge=Local;Edge.Y=Side>0?Bounds.Max.Y+200.f:Bounds.Min.Y-200.f;
                const FVector Outside=Hull.TransformPosition(Edge)-FVector(0,0,72.5f);
                Check(FString::Printf(TEXT("v833_team_%d_hull_side_%d_accepts_outward_recovery"),Team,Side),FriendlyHull->InfantrySweepClearanceV8218(Inside,Outward)==1.f);
                Check(FString::Printf(TEXT("v833_team_%d_hull_side_%d_rejects_inward_overlap"),Team,Side),FriendlyHull->InfantrySweepClearanceV8218(Inside,Inward)==0.f);
                Check(FString::Printf(TEXT("v833_team_%d_hull_side_%d_still_blocks_new_entry"),Team,Side),FriendlyHull->InfantrySweepClearanceV8218(Outside,Inside)<1.f);
            }
        }
        bool NoWorkDuringYield=true,NoToolsDuringYield=true;int32 YieldFrames=0;
        for(int32 Frame=0;Frame<40;++Frame)
        {
            const float PriorWork=State.Hazards[0].Work;FVector Passing;
            const bool PassingNow=FrontlineMobilityV41::ArmourYieldDestinationV8218(Worker,Passing);
            B->Elapsed+=.05f;Worker->Tick(.05f);TickHazardsV8219(B,State,.05f);
            if(PassingNow)
            {
                ++YieldFrames;NoWorkDuringYield&=State.Hazards[0].Work==PriorWork;
                const auto Pose=FrontlineMotionV40::MineWorkReviewStateV835(Worker);
                NoToolsDuringYield&=!Pose->GetBoolField(TEXT("working"))&&!Pose->GetBoolField(TEXT("probe_visible"))
                    &&Worker->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle"))==RifleWasHidden;
            }
        }
        Check(FString::Printf(TEXT("v833_team_%d_live_friendly_hull_yield_precedes_mine_work"),Team),PhysicalYield&&WorkBlocked&&YieldFrames>0
            &&FMath::Abs(Worker->Feet().Y-BeforeYield.Y)>3.f&&NoWorkDuringYield&&Worker->IsAlive()&&State.TriggeredMines==0);
        Check(FString::Printf(TEXT("v835_team_%d_live_passing_hull_aborts_actual_work_and_restores_equipment"),Team),WorkBeforeYield&&YieldFrames>0&&NoToolsDuringYield);
        Check(FString::Printf(TEXT("v833_team_%d_friendly_hull_metrics placement=%d physical=%d blocked=%d yield_frames=%d lateral=%.1f requested_x=%.1f requested_y=%.1f initial_sweep=%.3f moving=%d speed=%.1f work=%.3f no_work_during_yield=%d alive=%d triggered=%d phase=%d local_A=%s local_D=%s depth_gain_cm=%s"),Team,
            YieldPlacement,int32(PhysicalYield),int32(WorkBlocked),YieldFrames,float(Worker->Feet().Y-BeforeYield.Y),float(RequestedYield.X),float(RequestedYield.Y),InitialSweep,int32(Worker->bMoving),Worker->GetMotionSpeed(),
            State.Hazards[0].Work,int32(NoWorkDuringYield),int32(Worker->IsAlive()),State.TriggeredMines,
            int32(Worker->WeaponPhase),*LocalStart.ToString(),*LocalStep.ToString(),*DepthGain.ToString()),!Worker->Feet().ContainsNaN());
        if(FriendlyHull){FriendlyHull->SetActorHiddenInGame(true);FriendlyHull->SetActorEnableCollision(false);}
        FrontlineMobilityV41::Reset(B);Worker->ReturnToIdle();Worker->SetActorLocation(WorkPoint+FVector(0,0,88));
        // The worker has a live mine reservation when a repairable hull enters
        // its ordinary service radius. The production destination must keep
        // that worker at the charge, then release it back to normal repair.
        FVector ReservedPoint=B->SoldierDestination(Worker);
        auto* RepairHull=Armour.Spawn(Team);
        if(RepairHull)
        {
            RepairHull->SetActorTransform(RepairHull->GroundPoseForContactV8218(WorkPoint+FVector(Dir*100.f,700.f,0),Team==0?0.f:180.f));
            RepairHull->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,RepairHull->GetTrackIntegrity(0)+1.f);
        }
        FVector RepairPoint;const bool RepairDeferred=!FrontlineFieldSitesV59::EngineerDestinationV60(Worker,RepairPoint);
        Check(FString::Printf(TEXT("v833_team_%d_reserved_mine_precedes_actual_repairable_hull"),Team),RepairHull&&RepairHull->CanRepairTrackV60(0)
            &&HasMineApproachV833(Worker)&&RepairDeferred&&B->SoldierDestination(Worker).Equals(ReservedPoint,.01f)&&IsMineWorkingV8215(Worker));
        State.Hazards[0].Cleared=true;
        Check(FString::Printf(TEXT("v833_team_%d_completed_mine_releases_normal_hull_repair"),Team),!HasMineApproachV833(Worker)
            &&FrontlineFieldSitesV59::EngineerDestinationV60(Worker,RepairPoint)&&!B->SoldierDestination(Worker).Equals(ReservedPoint,.1f));
        if(RepairHull){RepairHull->SetActorHiddenInGame(true);RepairHull->SetActorEnableCollision(false);}
        State.Hazards={Mine};FrontlineMobilityV41::Reset(B);Worker->ReturnToIdle();
        B->Units={Worker};auto* Contact=Make(Team,EFrontlineRole::Rifle,833300+Team,0,Mine.Centre);
        const bool WorkerInReach=IsMineWorkingV8215(Worker);State.HazardAt=B->Elapsed;TickHazardsV8219(B,State,0.f);
        Check(FString::Printf(TEXT("v833_team_%d_engineer_does_not_make_friendly_live_mine_contacts_immune"),Team),Contact&&WorkerInReach
            &&State.TriggeredMines==1&&State.ClearedMines==1&&!Contact->IsAlive());
    }
}
void RunClearanceChecksV8311(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))
        &&!WarfareFRRUCampaignV82::Active(B)&&B->Phase==EFrontlinePhase::Fighting&&B->Trenches.Num()>=2;
    Check(TEXT("v8311_mine_clearance_strict_disposable_scope"),Safe);if(!Safe)return;
    FFrontlineEngineerMineV833Review Armour(B);
    auto& S=Battles.FindOrAdd(B);const auto Saved=S;const auto SavedKits=Units;const auto OldUnits=B->Units;const auto OldSquads=B->Squads;
    const float Clock=B->Elapsed,FF=B->FriendlyFireDamage;const auto Record=B->CombatRecordV65[0];
    const uint32 ResearchMask=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Research.TechnologyMask:0;TArray<AWarfareFrontlineSoldier*> Made;
    TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
    ON_SCOPE_EXIT{for(auto* U:Made)if(IsValid(U))U->Destroy();for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
        S=Saved;Units=SavedKits;B->Units=OldUnits;B->Squads=OldSquads;B->Elapsed=Clock;B->FriendlyFireDamage=FF;B->CombatRecordV65[0]=Record;if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))P->Research.TechnologyMask=ResearchMask;};
    FrontlineAchievementsV76::WithScopedCombatEventsV839(B,[&]
    {
      for(bool Trained:{false,true})
      {
        B->Units.Reset();B->Squads.Reset();S=FState();S.Encounter=B->GetEncounterId();S.TrainSpawned=true;S.HazardAt=B->Elapsed;
        if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){if(Trained)P->Research.TechnologyMask|=1u<<17;else P->Research.TechnologyMask&=~(1u<<17);}
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();Check(TEXT("v8311_clearance_actual_pioneer_spawn"),U!=nullptr);if(!U)continue;
        Made.Add(U);U->Initialize(B,8311101+int32(Trained),0,EFrontlineRole::Pioneer,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->ReturnToIdle();
        const int32 Dir=B->GetTeamDirection(0),Home=B->GetHomeTrenchIndex(0),Next=Home+Dir;
        FHazard H;H.Mine=true;H.Radius=FrontlineMineV8214::TriggerRadiusCm;H.Centre=FVector((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,500,0);
        H.Centre.Z=FrontlineInfantryV45::SurfaceHeight(B,H.Centre)+2;S.Hazards={H};
        const FVector Foot=H.Centre-FVector(Dir*FrontlineMineV8214::WorkStandOffCmV833,0,0);
        U->SetActorLocation(FVector(Foot.X,Foot.Y,FrontlineInfantryV45::SurfaceHeight(B,Foot)+88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));B->Units={U};
        FFrontlineSquad Q;Q.Id=U->SquadId;Q.Team=0;Q.Role=EFrontlineRole::Pioneer;Q.bAdvancing=true;Q.Destination=Next;Q.Members={U};B->Squads={Q};
        const float Born=B->Elapsed,Seconds=FrontlineMineV8214::WorkSecondsV833/(Trained?1.65f:1.f);
        TickHazardsV8219(B,S,0);for(int32 I=0;I<20;++I){B->Elapsed+=.05f;TickHazardsV8219(B,S,.05f);}
        Check(TEXT("v8311_clearance_not_finished_in_old_one_second_window"),!S.Hazards[0].Cleared&&S.ClearanceBurstsV8311==0&&U->IsAlive());
        for(int32 I=0;I<100&&!S.Hazards[0].Cleared;++I){B->Elapsed+=.05f;TickHazardsV8219(B,S,.05f);}
        const float Elapsed=B->Elapsed-Born;
        Check(TEXT("v8311_clearance_slow_host_work_and_real_training_rate"),S.Hazards[0].Cleared&&Elapsed>=Seconds-.01f&&Elapsed<=Seconds+.35f);
        auto* FX=S.LastClearanceFXV8311.Get();Check(TEXT("v8311_clearance_one_real_visual_ahead_at_mine"),S.ClearanceBurstsV8311==1&&FX&&FX->IsVisualReady()&&FX->GetBlastProfileIndex()==2
            &&FVector::DistSquared2D(FX->GetActorLocation(),H.Centre)<.1&&((FX->GetActorLocation().X-U->Feet().X)*Dir)>250);
        Check(TEXT("v8311_clearance_burst_never_calls_armed_mine_damage"),U->IsAlive()&&S.TriggeredMines==0&&S.ClearedMines==1&&B->FriendlyFireDamage==FF&&B->CombatRecordV65[0].InfantryLost==Record.InfantryLost);
        B->Elapsed+=.5f;TickHazardsV8219(B,S,.5f);Check(TEXT("v8311_clearance_completed_charge_cannot_emit_twice"),S.ClearanceBurstsV8311==1);
        U->Destroy();
      }
    });
}
#include "WarfareFrontlineEngineerFrontageV84412.inl"
AWarfareFrontlineSoldier* PrepareClearanceViewV8311(AWarfareFrontlineGameMode* B,float WorkSeconds)
{
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))||WarfareFRRUCampaignV82::Active(B)
        ||B->Phase!=EFrontlinePhase::Fighting||B->Trenches.Num()<2)return nullptr;
    auto& S=Battles.FindOrAdd(B);S=FState();S.Encounter=B->GetEncounterId();S.TrainSpawned=true;S.HazardAt=B->Elapsed;
    const int32 Dir=B->GetTeamDirection(0),Home=B->GetHomeTrenchIndex(0),Next=Home+Dir;
    if(!B->Trenches.IsValidIndex(Next))return nullptr;
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return nullptr;
    U->Initialize(B,8311191,0,EFrontlineRole::Pioneer,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->FormationOffset=FVector::ZeroVector;U->ReturnToIdle();
    FHazard H;H.Mine=true;H.Radius=FrontlineMineV8214::TriggerRadiusCm;H.Centre=FVector((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,500,0);H.Centre.Z=FrontlineInfantryV45::SurfaceHeight(B,H.Centre)+2;
    const FVector Foot=H.Centre-FVector(Dir*FrontlineMineV8214::WorkStandOffCmV833,0,0);U->SetActorLocation(FVector(Foot.X,Foot.Y,FrontlineInfantryV45::SurfaceHeight(B,Foot)+88));U->SetActorRotation(FRotator(0,Dir>0?0:180,0));
    FFrontlineSquad Q;Q.Id=U->SquadId;Q.Team=0;Q.Role=EFrontlineRole::Pioneer;Q.bAdvancing=true;Q.Destination=Next;Q.LastPassedTrench=Home;Q.Members={U};B->Units.Add(U);B->Squads.Add(Q);S.Hazards={H};
    if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))P->Research.TechnologyMask&=~(1u<<17);
    B->SoldierDestination(U);const int32 Frames=FMath::CeilToInt(FMath::Clamp(WorkSeconds,0.f,4.8f)/.05f);
    for(int32 I=0;I<Frames;++I){B->Elapsed+=.05f;U->Tick(.05f);TickHazardsV8219(B,S,.05f);U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(.05f,false);U->Soldier->RefreshBoneTransforms();}
    return U;
}
void RunWorkTimeBudgetChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review")))&&!WarfareFRRUCampaignV82::Active(B)
        &&B->GetWorld()->IsGameWorld()&&B->Phase==EFrontlinePhase::Fighting&&!B->Trenches.IsEmpty();
    Check(TEXT("v8219_mine_work_requires_actual_prepared_disposable_uk_de_world"),Safe);if(!Safe)return;
    auto& State=Battles.FindOrAdd(B);const auto SavedState=State;
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedKits=Units;
    const float SavedClock=B->Elapsed;const int32 SavedWinner=B->BattleOutroWinner;
    auto* Worker=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    ON_SCOPE_EXIT
    {
        if(IsValid(Worker))Worker->Destroy();State=SavedState;Units=SavedKits;
        B->Units=SavedUnits;B->Squads=SavedSquads;B->Elapsed=SavedClock;B->BattleOutroWinner=SavedWinner;
    };
    if(Worker){Worker->Initialize(B,82191991,1,EFrontlineRole::Pioneer,0);Worker->SetActorTickEnabled(false);
        Worker->bFieldEntered=true;Worker->bMoving=false;Worker->ReturnToIdle();}
    const bool Registered=IsValid(Worker)&&Worker->GetRootComponent()&&Worker->GetRootComponent()->IsRegistered()
        &&Worker->Soldier&&Worker->Soldier->IsRegistered()&&Worker->Soldier->GetSkeletalMeshAsset();
    Check(TEXT("v8219_mine_work_uses_registered_actual_pioneer"),Registered);if(!Registered)return;
    B->Units={Worker};B->Squads.Empty();B->BattleOutroWinner=INDEX_NONE;
    FFrontlineSquad WorkSection;WorkSection.Id=Worker->SquadId;WorkSection.Team=Worker->Team;
    WorkSection.Role=EFrontlineRole::Pioneer;WorkSection.bAdvancing=true;WorkSection.Members={Worker};B->Squads.Add(WorkSection);
    const int32 Direction=B->GetTeamDirection(1),Home=B->GetHomeTrenchIndex(1);
    const float X=B->Trenches.IsValidIndex(Home)?B->Trenches[Home].X-Direction*650.f:0.f;
    FHazard Mine;Mine.Mine=true;Mine.Radius=FrontlineMineV8214::TriggerRadiusCm;
    Mine.Centre=FVector(X,500,AWarfareFrontlineGround::HeightAt(X,500)+2.f);
    FVector Work=Mine.Centre-FVector(Direction*320.f,0,0);Work.Z=AWarfareFrontlineGround::HeightAt(Work.X,Work.Y);
    const auto ResetContact=[&]()
    {
        State=FState();State.Encounter=B->GetEncounterId();State.Hazards={Mine};State.TrainSpawned=true;
        Worker->SetActorLocation(Work+FVector(0,0,88));Worker->bMoving=false;Worker->Health=100;
        B->Elapsed=1000.f;State.HazardAt=B->Elapsed;TickHazardsV8219(B,State,0.f);
    };
    float WorkAtMidpoint[3]={};bool Completed[3]={};const float Frames[]={.05f,.2f,.4f};
    for(int32 Case=0;Case<3;++Case)
    {
        ResetContact();const float Frame=Frames[Case];const int32 Count=FMath::RoundToInt(4.8f/Frame);
        for(int32 Step=1;Step<=Count;++Step)
        {
            B->Elapsed=1000.f+Step*Frame;TickHazardsV8219(B,State,Frame);
            if(Step==Count/2)WorkAtMidpoint[Case]=State.Hazards[0].Work;
        }
        Completed[Case]=State.Hazards[0].Cleared&&State.ClearedMines==1&&State.TriggeredMines==0&&Worker->IsAlive();
        Check(FString::Printf(TEXT("v8311_mine_work_frame_%.2f_keeps_safe_contact_and_four_point_eight_second_completion"),Frame),Completed[Case]);
    }
    Check(TEXT("v8311_mine_work_same_two_point_four_game_seconds_across_frames"),
        FMath::IsNearlyEqual(WorkAtMidpoint[0],2.4f,.001f)&&FMath::IsNearlyEqual(WorkAtMidpoint[1],WorkAtMidpoint[0],.001f)
        &&FMath::IsNearlyEqual(WorkAtMidpoint[2],WorkAtMidpoint[0],.001f));
    ResetContact();Worker->SetActorLocation(Work+FVector(0,1200,88));TickHazardsV8219(B,State,0.f);
    Worker->SetActorLocation(Work+FVector(0,0,88));B->Elapsed+=4.f;TickHazardsV8219(B,State,4.f);
    Check(TEXT("v8219_new_mine_contact_never_replays_unknown_four_second_interval"),
        State.Hazards[0].Work==0&&!State.Hazards[0].Cleared&&State.TriggeredMines==0);
    B->Elapsed+=.4f;TickHazardsV8219(B,State,.4f);const float BeforeMove=State.Hazards[0].Work;
    Worker->SetActorLocation(Work+FVector(0,100,88));B->Elapsed+=.4f;TickHazardsV8219(B,State,.4f);
    Check(TEXT("v8219_moving_mine_contact_does_not_credit_stationary_work"),
        FMath::IsNearlyEqual(State.Hazards[0].Work,BeforeMove,.0001f)&&!State.Hazards[0].Cleared);
}
static void DetonateBlastMinesV84411(AWarfareFrontlineGameMode* B,const FFrontlineStrike& Strike,bool ClearObstacles);
void Tick(AWarfareFrontlineGameMode* B,float DT)
{
    if(B->BattleOutroWinner>=0){B->BattleOutroElapsed+=DT;FrontlineSupportV32::Tick(B,DT);return;}
    if(B->Phase!=EFrontlinePhase::Fighting)return;auto& S=Battles.FindOrAdd(B);FrontlineSupportV32::Tick(B,DT);
    FrontlineFieldSitesV59::Tick(B,DT);
    for(int32 I=S.JetContacts.Num()-1;I>=0;--I)if(B->Elapsed>=S.JetContacts[I].At)
    {const auto Hit=S.JetContacts[I];S.JetContacts.RemoveAtSwap(I);if(auto* U=Hit.Target.Get();U&&U->IsActiveCombatant())U->ReceiveHit(Hit.Hit&&FVector::Dist2D(U->Feet(),Hit.Point)<155?1.f:0.f,.9f,Hit.Team,Hit.Direction,EFrontlineDamageKind::Fire,.15f,Hit.SourceV65);}
    // Snapshot due reservoirs before hits can enqueue another casualty. At most
    // four rupture bursts per frame; every pack marks itself spent before damage.
    TArray<AWarfareFrontlineSoldier*> Due;
    for(const auto& Pair:Units)if(Pair.Key.IsValid()&&Pair.Key->Battle==B&&Pair.Value.FuelIgnited&&!Pair.Value.FuelSpent&&Pair.Value.FuelBurstAt<=B->Elapsed){Due.Add(Pair.Key.Get());if(Due.Num()==4)break;}
    for(auto* U:Due)
    {
        auto& F=Units.FindChecked(U);F.FuelSpent=true;const float Fuel=F.Fuel;F.Fuel=0;const int32 Attacker=F.IgnitedBy;const FVector At=U->Soldier->GetBoneLocation(TEXT("spine_02"));
        U->OnFuelRuptureV41(Fuel);
        if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))Fire->FuelBurst(At,Fuel);
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayImpact(true,At);
        ++S.FuelBursts;const float Radius=FMath::Lerp(145.f,280.f,FMath::Clamp(Fuel/12.f,0.f,1.f));
        FFrontlineStrike FuelBlast;FuelBlast.Target=At;FuelBlast.Radius=Radius;FuelBlast.Team=Attacker;
        FuelBlast.SourceV65=U->FatalSourceV65;FuelBlast.FireMissionId=FuelBlast.SourceV65.FireMissionId;
        DetonateBlastMinesV84411(B,FuelBlast,false);
        for(AWarfareFrontlineSoldier* Other:B->Units)if(Other&&Other->IsActiveCombatant()&&FVector::Dist2D(Other->Feet(),At)<Radius)
            Other->ReceiveHit(1,.9f,Attacker,(Other->Feet()-At).GetSafeNormal2D(),EFrontlineDamageKind::Fire,.4f,U->FatalSourceV65);
        UE_LOG(LogTemp,Display,TEXT("WF_V33_FUEL_RUPTURE squad=%d fuel=%.1f radius=%.1f once=1"),U->SquadId,Fuel,Radius);
    }
    // Rail detachments use the same model-specific readiness and recruitment
    // path as all other armour. Field effects never inject a second train.
    TickHazardsV8219(B,S,DT);
}
void OnBlast(AWarfareFrontlineGameMode* B,const FFrontlineStrike& Strike)
{
    if(!Strike.bShrapnelV64)FrontlineFieldSitesV59::OnBlast(B,Strike);
    DetonateBlastMinesV84411(B,Strike,!Strike.bShrapnelV64);
}
static void DetonateBlastMinesV84411(AWarfareFrontlineGameMode* B,const FFrontlineStrike& Strike,bool ClearObstacles)
{
    auto* S=Battles.Find(B);
    if(!S||!FMath::IsFinite(Strike.Radius)||Strike.Radius<=0||Strike.Target.ContainsNaN())return;
    // Claim all charges in the connected blast footprint before resolving any
    // secondary hit. Nested ResolveStrike calls cannot fire a charge twice,
    // and no reference into the host's pending-strike array survives a callback.
    TArray<FVector,TInlineAllocator<32>> Detonations;
    const auto Source=Strike.SourceV65;const int32 Team=Strike.Team,FireMission=Strike.FireMissionId;
    FVector Centre=Strike.Target;float Radius=Strike.Radius;int32 Front=0;
    FCollisionQueryParams Screen(SCENE_QUERY_STAT(V84411AirburstMineScreen),true);
    if(Strike.bShrapnelV64)for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Screen.AddIgnoredActor(U);
    for(;;)
    {
        const bool Secondary=Front>0;
        for(auto& H:S->Hazards)if(!H.Cleared&&FVector::DistSquared2D(H.Centre,Centre)<=FMath::Square(H.Mine?Radius:Radius*.5f))
        {
            if(!H.Mine){if(ClearObstacles||Secondary)ClearHazard(*S,H);continue;}
            // Descending shrapnel must reach the actual charge through the
            // same world visibility geometry used for its infantry hits.
            // Secondary mines are ground explosions, not another airburst.
            if(Strike.bShrapnelV64&&!Secondary&&B->GetWorld()->LineTraceTestByChannel(
                Strike.Target,H.Centre+FVector(0,0,5),ECC_Visibility,Screen))continue;
            H.Cleared=true;H.Work=0;H.ApproachPioneerV833.Reset();++S->TriggeredMines;
            if(auto* C=H.Visual.Get()){C->SetVisibility(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
            Detonations.Add(H.Centre);
        }
        if(!Detonations.IsValidIndex(Front))break;
        Centre=Detonations[Front++];Radius=FrontlineMineV8214::BlastRadiusCm;
    }
    for(const FVector& Point:Detonations)
    {
        FFrontlineStrike Mine;Mine.Target=Point;Mine.Team=Team;Mine.SourceV65=Source;Mine.FireMissionId=FireMission;
        Mine.Radius=FrontlineMineV8214::BlastRadiusCm;Mine.bGrenade=true;Mine.bMineV8214=true;
        B->ResolveFieldStrikeV32(Mine);
    }
}
void RunBlastMineChecksV84411(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=B&&B->IsProgressionVolatile()&&B->Phase==EFrontlinePhase::Fighting&&FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"));
    Check(TEXT("v84411_mine_chain_disposable"),Allowed);if(!Allowed)return;
    auto& S=Battles.FindOrAdd(B);const FState Before=S;
    ON_SCOPE_EXIT{S=Before;};S.Hazards.Reset();S.TriggeredMines=0;S.ClearedMines=0;
    FVector Origin(0,800,0);Origin.Z=FrontlineInfantryV45::SurfaceHeight(B,Origin)+2.f;
    const float R=FrontlineMineV8214::BlastRadiusCm;
    auto Add=[&](float X,bool Cleared=false){FHazard H;H.Mine=true;H.Centre=Origin+FVector(X,0,0);H.Centre.Z=FrontlineInfantryV45::SurfaceHeight(B,H.Centre)+2.f;H.Radius=FrontlineMineV8214::TriggerRadiusCm;H.Cleared=Cleared;S.Hazards.Add(H);};
    Add(80);Add(80+R*.8f);Add(80+R*1.6f);Add(80+R*3.f);Add(40,true);
    FFrontlineStrike Shell;Shell.Target=Origin;Shell.Radius=150;Shell.Team=0;Shell.FireMissionId=84411;
    Shell.SourceV65=FFrontlineKillSourceV65::FireSupport(0,0);
    B->ResolveFieldStrikeV32(Shell);
    Check(TEXT("v84411_shell_really_detonates_three_connected_mines"),S.TriggeredMines==3&&S.Hazards[0].Cleared&&S.Hazards[1].Cleared&&S.Hazards[2].Cleared);
    Check(TEXT("v84411_outside_chain_and_disarmed_mine_not_detonated"),!S.Hazards[3].Cleared&&S.ClearedMines==0);
    int32 SecondaryDeaths=0;for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->SquadId>=8441190&&U->SquadId<=8441191)
    {Check(TEXT("v84411_secondary_mine_actual_fatal_source_retains_fire_mission"),!U->IsAlive()&&U->FatalSourceV65.FireMissionId==84411&&U->FatalSourceV65.Team==0&&U->FatalSourceV65.Support==0);SecondaryDeaths+=!U->IsAlive()?1:0;}
    Check(TEXT("v84411_secondary_mine_actual_friendly_and_enemy_casualties"),SecondaryDeaths==2);
    OnBlast(B,Shell);Check(TEXT("v84411_spent_chain_cannot_repeat"),S.TriggeredMines==3);
    S.Hazards.Reset();S.TriggeredMines=0;Add(150);Add(150+R+.5f);
    OnBlast(B,Shell);Check(TEXT("v84411_exact_blast_boundary_is_inclusive_outside_is_preserved"),S.TriggeredMines==1&&S.Hazards[0].Cleared&&!S.Hazards[1].Cleared);
    // Exercise the actual host airburst route. A wall screens the initial
    // fragment fan, while a later exposed charge can produce a ground blast.
    S.Hazards.Reset();S.TriggeredMines=0;S.ClearedWire=0;Add(80);Add(150+R+.5f);Add(40,true);
    FHazard Wire;Wire.Centre=Origin+FVector(50,0,0);Wire.Radius=150;S.Hazards.Add(Wire);
    auto* ScreenActor=B->GetWorld()->SpawnActor<AActor>();
    auto* ScreenBox=ScreenActor?NewObject<UBoxComponent>(ScreenActor):nullptr;
    if(ScreenBox)
    {
        ScreenActor->SetRootComponent(ScreenBox);ScreenActor->AddInstanceComponent(ScreenBox);ScreenBox->SetMobility(EComponentMobility::Movable);
        ScreenBox->SetBoxExtent(FVector(15,55,90));ScreenBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        ScreenBox->SetCollisionResponseToAllChannels(ECR_Ignore);ScreenBox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        ScreenBox->RegisterComponent();ScreenActor->SetActorLocation(Origin+FVector(40,0,90));
    }
    Check(TEXT("v84411_airburst_screen_uses_registered_world_visibility"),ScreenBox&&ScreenBox->IsRegistered());
    FFrontlineStrike Air=Shell;Air.bResolved=false;Air.bShrapnelV64=true;Air.Target=Origin+FVector(0,0,180);
    B->ResolveFieldStrikeV32(Air);
    Check(TEXT("v84411_airburst_screened_mine_and_wire_survive_initial_fragment_fan"),
        ScreenBox&&!S.Hazards[0].Cleared&&!S.Hazards[1].Cleared&&!S.Hazards[3].Cleared&&S.TriggeredMines==0&&S.ClearedWire==0);
    // Destroy is deferred within this synchronous fixture. Remove the actual
    // query shape before proving the exposed route in the same host frame.
    if(ScreenBox)ScreenBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(ScreenActor)ScreenActor->Destroy();
    Air.bResolved=false;B->ResolveFieldStrikeV32(Air);
    Check(TEXT("v84411_exposed_airburst_charge_really_detonates_without_replaying_disarmed_or_outside_mines"),
        S.Hazards[0].Cleared&&!S.Hazards[1].Cleared&&S.TriggeredMines==1);
    Check(TEXT("v84411_secondary_ground_mine_retains_ordinary_obstacle_damage"),S.Hazards[3].Cleared&&S.ClearedWire==1);
    // A real reservoir schedules and bursts through the ordinary field tick;
    // no test-only mine call substitutes for the production fuel callback.
    const auto SavedPeople=B->Units;const float SavedClock=B->Elapsed;
    auto* Operator=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    Check(TEXT("v84411_fuel_mine_registered_operator"),Operator!=nullptr);
    if(Operator)
    {
        Operator->Initialize(B,8441193,0,EFrontlineRole::Flamethrower,0);
        Operator->SetActorTickEnabled(false);Operator->SetActorLocation(Origin+FVector(0,0,88));Operator->bFieldEntered=true;
        B->Units={Operator};B->Elapsed=1000.f;S.Hazards.Reset();S.TriggeredMines=0;S.JetContacts.Reset();
        Add(80);Add(R+500.f);S.HazardAt=B->Elapsed;S.TrainSpawned=true;
        auto FuelSource=FFrontlineKillSourceV65::FireSupport(1,0);FuelSource.FireMissionId=84412;
        Operator->ReceiveHit(1.f,.9f,1,FVector::ForwardVector,EFrontlineDamageKind::Fire,.4f,FuelSource);
        const auto* FuelState=Units.Find(Operator);
        const bool Scheduled=FuelState&&FuelState->FuelIgnited&&!FuelState->FuelSpent;
        Check(TEXT("v84411_real_fire_casualty_schedules_fuel_mine_contact"),Scheduled);
        if(Scheduled){B->Elapsed=FuelState->FuelBurstAt+.05f;Tick(B,.05f);}
        Check(TEXT("v84411_actual_fuel_rupture_detonates_only_reached_live_mine"),Scheduled&&S.TriggeredMines==1&&S.Hazards[0].Cleared&&!S.Hazards[1].Cleared);
        const int32 Triggered=S.TriggeredMines;B->Elapsed+=1.f;Tick(B,.05f);
        Check(TEXT("v84411_spent_fuel_and_mine_cannot_replay_chain"),S.TriggeredMines==Triggered);
        Units.Remove(Operator);Operator->Destroy();B->Units=SavedPeople;B->Elapsed=SavedClock;
    }
}
TArray<FVector> MineBeltPointsV59(const AWarfareFrontlineGameMode* B)
{
    TArray<FVector> Points;if(const auto* S=Battles.Find(const_cast<AWarfareFrontlineGameMode*>(B)))
        for(const auto& H:S->Hazards)if(H.Mine&&!H.Cleared)Points.Add(H.Centre);return Points;
}
bool HasAirObservation(const AWarfareFrontlineGameMode* B,int32 Team,const FVector& P)
{return FrontlineSupportV32::HasRegistrationV37(B,Team,P);}
void SetAirObservation(AWarfareFrontlineGameMode* B,int32 Team,const FVector& P,float Until)
{if(Team<0||Team>1)return;auto& S=Battles.FindOrAdd(B);S.ReconCentre[Team]=P;S.ReconUntil[Team]=Until;}
TSharedRef<FJsonObject> Review(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();const auto* S=Battles.Find(B);J->SetNumberField(TEXT("field_hazards"),S?S->Hazards.Num():0);J->SetNumberField(TEXT("cleared_mines"),S?S->ClearedMines:0);J->SetNumberField(TEXT("cleared_wire"),S?S->ClearedWire:0);J->SetNumberField(TEXT("triggered_mines"),S?S->TriggeredMines:0);
    J->SetNumberField(TEXT("mine_trigger_radius_cm_v8214"),FrontlineMineV8214::TriggerRadiusCm);J->SetNumberField(TEXT("mine_blast_radius_cm_v8214"),FrontlineMineV8214::BlastRadiusCm);
    for(int32 T=0;T<2;++T)
    {
        J->SetNumberField(T?TEXT("enemy_pioneer_mines_cleared_v78"):TEXT("player_pioneer_mines_cleared_v78"),S?S->PioneerMinesByTeam[T]:0);
        J->SetNumberField(T?TEXT("enemy_pioneer_wire_cleared_v78"):TEXT("player_pioneer_wire_cleared_v78"),S?S->PioneerWireByTeam[T]:0);
    }
    int32 Masks=0,Cans=0,Drops=0,French=0,FlamePacks=0,LiveJetCards=0;float Fuel=0;
    for(const auto& Pair:Units)if(Pair.Key.IsValid()&&Pair.Key->Battle==B)
    {const auto& U=Pair.Value;Masks+=U.Mask.IsValid()&&U.Mask->IsVisible();Cans+=U.Canister.IsValid()&&U.Canister->IsVisible();Drops+=U.DroppedWeapon.IsValid()&&U.DroppedWeapon->IsSimulatingPhysics();French+=U.Helmet.IsValid();FlamePacks+=IsFlameOperator(Pair.Key.Get())&&U.Pack.IsValid();Fuel+=U.Fuel;}
    if(S&&S->Fire.IsValid())LiveJetCards=S->Fire->ActiveCount();
    J->SetNumberField(TEXT("fuel_bursts_v33"),S?S->FuelBursts:0);J->SetNumberField(TEXT("burning_casualties_v33"),S?S->BurningCasualties:0);J->SetNumberField(TEXT("fire_particle_budget_v33"),AFrontlineFuelFireV33::FlameBudget+AFrontlineFuelFireV33::SmokeBudget);
    int32 Rails=0;float RailHull=0;if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:V->GetVehicles())if(T&&T->IsRailConvoyV32()){++Rails;RailHull=T->GetHullIntegrity();}
    float RailBedError=0;if(Rails>0)for(int32 X=-12000;X<=12000;X+=500)RailBedError=FMath::Max(RailBedError,FMath::Abs(AWarfareFrontlineGround::HeightAt(X,-1130)-19.5f));
    J->SetNumberField(TEXT("rail_bed_max_seating_error_cm"),RailBedError);
    J->SetNumberField(TEXT("visible_respirators"),Masks);J->SetNumberField(TEXT("respirator_carriers"),Cans);J->SetNumberField(TEXT("physical_dropped_weapons"),Drops);J->SetNumberField(TEXT("french_adrian_helmets"),French);J->SetNumberField(TEXT("flame_reservoirs"),FlamePacks);J->SetNumberField(TEXT("live_flame_cards"),LiveJetCards);J->SetNumberField(TEXT("rail_convoys"),Rails);J->SetNumberField(TEXT("rail_hull"),RailHull);
    J->SetBoolField(TEXT("traverses_are_non_collinear"),FMath::Abs(TrenchCentreX(2,-650)-TrenchCentreX(2,650))>15);J->SetNumberField(TEXT("support_half_frontage_cm"),FrontlineBattlefield::HalfFrontage);return J;
}
void RunMineChecksV8214(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("v8214_mine_fixture_is_disposable"),false);return;}
    bool Curve=true;float Previous=1.f;
    for(float D:{0.f,155.f,175.f,275.f,350.f,450.f,599.f,600.f,601.f})
    {
        const float Chance=FrontlineMineV8214::HitChanceFalloff(D);
        Curve&=FMath::IsFinite(Chance)&&Chance>=0.f&&Chance<=Previous;Previous=Chance;
    }
    Check(TEXT("v8214_mine_has_full_contact_core_and_monotonic_six_metre_boundary"),Curve
        &&FrontlineMineV8214::HitChanceFalloff(155.f)==1.f&&FrontlineMineV8214::HitChanceFalloff(275.f)>.80f
        &&FrontlineMineV8214::HitChanceFalloff(600.f)==0.f&&FrontlineMineV8214::HitChanceFalloff(-1.f)==0.f
        &&FrontlineMineV8214::HitChanceFalloff(200.f,0.f)==0.f);

    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldPhase=B->Phase;
    const float OldTime=B->Elapsed,OldOutro=B->BattleOutroElapsed,OldFriendly=B->FriendlyFireDamage,OldPeak=B->PeakSuppression;
    const int32 OldWinner=B->BattleOutroWinner;const auto OldMessage=B->LastMessage;
    const FFrontlineCombatRecordV65 OldRecords[]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const int32 OldCasualties[]={B->Casualties[0],B->Casualties[1]};
    const float OldDamage[]={B->DamageDealt[0],B->DamageDealt[1]},OldMorale[]={B->Morale[0],B->Morale[1]};
    auto& State=Battles.FindOrAdd(B);const FState OldField=State;const auto OldUnitStates=Units;
    TSet<TWeakObjectPtr<AWarfareFrontlineImpact>> PriorImpacts;
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)PriorImpacts.Add(*I);
    TArray<AWarfareFrontlineSoldier*> Made;AActor* Screen=nullptr;
    ON_SCOPE_EXIT
    {
        if(IsValid(Screen))Screen->Destroy();
        for(auto* U:Made)if(IsValid(U))U->Destroy();
        for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(!PriorImpacts.Contains(*I))I->Destroy();
        State=OldField;Units=OldUnitStates;B->Units=OldUnits;B->Squads=OldSquads;B->Phase=OldPhase;
        B->Elapsed=OldTime;B->BattleOutroElapsed=OldOutro;B->BattleOutroWinner=OldWinner;B->LastMessage=OldMessage;
        B->FriendlyFireDamage=OldFriendly;B->PeakSuppression=OldPeak;
        for(int32 T=0;T<2;++T){B->CombatRecordV65[T]=OldRecords[T];B->Casualties[T]=OldCasualties[T];B->DamageDealt[T]=OldDamage[T];B->Morale[T]=OldMorale[T];}
    };
    B->Units.Reset();B->Squads.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    State.Hazards.Reset();State.JetContacts.Reset();State.TrainSpawned=true;
    const FVector At(2400.f,800.f,AWarfareFrontlineGround::HeightAt(2400.f,800.f)+2.f);
    auto Make=[&](float Distance,int32 Team)
    {
        FVector P=At+FVector(Distance,0,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88.f;
        const int32 Id=821400+Made.Num();auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator::ZeroRotator);
        if(!U)return U;U->Initialize(B,Id,Team,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);
        U->bFieldEntered=true;U->Suppression=0.f;B->Units.Add(U);Made.Add(U);
        FFrontlineSquad Squad;Squad.Id=Id;Squad.Team=Team;Squad.Members.Add(U);B->Squads.Add(Squad);return U;
    };
    auto* Contact=Make(0,0);auto* Near=Make(350,0);auto* Peer=Make(450,1);auto* Outside=Make(601,0);
    if(!Contact||!Near||!Peer||!Outside){Check(TEXT("v8214_mine_live_fixture_spawns"),false);return;}
    FFrontlineStrike Probe;Probe.Target=At;Probe.Radius=FrontlineMineV8214::BlastRadiusCm;Probe.Team=1;Probe.bGrenade=true;Probe.bMineV8214=true;
    const float OpenChance=B->ExplosionHitChance(Probe,Near);
    Screen=B->GetWorld()->SpawnActor<AActor>();
    if(Screen)
    {
        auto* Box=NewObject<UBoxComponent>(Screen);Screen->SetRootComponent(Box);Screen->AddInstanceComponent(Box);
        Box->SetMobility(EComponentMobility::Movable);Box->SetBoxExtent(FVector(24.f,220.f,180.f));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->RegisterComponent();Box->SetWorldLocation(At+FVector(180.f,0,105.f));
        const float Screened=B->ExplosionHitChance(Probe,Near);
        Check(TEXT("v8214_mine_respects_real_two_ray_solid_screening"),OpenChance>.20f&&Screened>0.f&&Screened<=OpenChance*.181f);
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->Destroy();Screen=nullptr;
    }
    else Check(TEXT("v8214_mine_real_screen_fixture_spawns"),false);
    Check(TEXT("v8214_mine_live_chance_reaches_followers_but_stops_outside_six_metres"),B->ExplosionHitChance(Probe,Contact)>.99f
        &&B->ExplosionHitChance(Probe,Near)>.20f&&B->ExplosionHitChance(Probe,Peer)>0.f&&B->ExplosionHitChance(Probe,Outside)==0.f);
    FHazard Mine;Mine.Mine=true;Mine.Centre=At;Mine.Radius=FrontlineMineV8214::TriggerRadiusCm;State.Hazards.Add(Mine);
    FHazard Neighbour=Mine;Neighbour.Centre=At+FVector(0,400.f,0);State.Hazards.Add(Neighbour);
    const int32 BeforeTrigger=State.TriggeredMines;State.HazardAt=0;Tick(B,.26f);
    Check(TEXT("v8214_mine_real_trigger_kills_contact_and_pressures_both_armies_followers"),!Contact->IsAlive()&&Near->Suppression>.10f&&Peer->Suppression>0.f);
    Check(TEXT("v8214_mine_outer_soldier_and_neighbouring_charge_remain_untouched"),Outside->IsAlive()&&Outside->Suppression==0.f
        &&State.Hazards[0].Cleared&&!State.Hazards[1].Cleared&&State.TriggeredMines==BeforeTrigger+1);
    AWarfareFrontlineImpact* MineFX=nullptr;
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(!PriorImpacts.Contains(*I)&&I->GetBlastProfileIndex()==5)MineFX=*I;
    Check(TEXT("v8214_real_mine_strike_binds_six_metre_dedicated_bounded_visual"),MineFX&&MineFX->IsVisualReady()
        &&MineFX->GetActorScale3D().Equals(FVector::OneVector)&&MineFX->GetMineFootprintRadiusCmV8214()==600.f
        &&MineFX->GetConfiguredVolumeCountV8214()<=2&&MineFX->GetBallisticFragmentCount()>=8&&MineFX->GetBallisticFragmentCount()<=80);
    B->Elapsed+=.26f;State.HazardAt=0;Tick(B,.26f);
    Check(TEXT("v8214_spent_mine_cannot_resolve_twice"),State.TriggeredMines==BeforeTrigger+1);
    B->Units={Outside};Outside->Role=EFrontlineRole::Pioneer;
    if(auto* WorkSection=B->FindSquad(Outside->SquadId)){WorkSection->Role=EFrontlineRole::Pioneer;WorkSection->bAdvancing=true;WorkSection->bHoldPosition=false;}
    FVector Engineer=At+FVector(300.f,0,0);Engineer.Z=AWarfareFrontlineGround::HeightAt(Engineer.X,Engineer.Y)+88.f;Outside->SetActorLocation(Engineer);
    State.Hazards={Mine};const int32 BeforeClear=State.ClearedMines;
    for(int32 I=0;I<8;++I){B->Elapsed+=.26f;State.HazardAt=0;Tick(B,.26f);}
    Check(TEXT("v8214_pioneer_still_disarms_from_unchanged_safe_work_reach"),State.Hazards[0].Cleared&&State.ClearedMines==BeforeClear+1
        &&State.TriggeredMines==BeforeTrigger+1&&Outside->IsAlive());
}

void RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B)return;Check(TEXT("v32_support_frontage_expands_to_26_metres"),FrontlineBattlefield::HalfFrontage>=1300);
    float BendLow=MAX_flt,BendHigh=-MAX_flt;for(int32 I=-12;I<=12;++I){const float X=TrenchCentreX(2,I*100);BendLow=FMath::Min(BendLow,X);BendHigh=FMath::Max(BendHigh,X);}
    Check(TEXT("v32_shared_trench_has_real_traverses"),BendHigh-BendLow>150&&BendHigh-BendLow<270);
    bool Scatter=true;for(int32 Seed=0;Seed<12;++Seed)for(const auto& Round:FrontlineFireSupportV26::Plan(true,Seed))Scatter&=FMath::Abs(Round.Offset.Y)<=650;
    Check(TEXT("v32_mortar_spread_widened_and_bounded"),Scatter&&FrontlineFireSupportV26::ScatterExtent(true).Y==650);
    const EFrontlineSupport New[]={EFrontlineSupport::CreepingBarrage,EFrontlineSupport::AirRecon,EFrontlineSupport::AirBombing};bool Defined=true;for(auto S:New)Defined&=B->SupportCooldownSeconds(S)<MAX_flt&&B->SupportUnlockMission(S)<8&&FCString::Strlen(B->GetSupportDescription(S))>10;
    Check(TEXT("v32_three_new_support_plans_have_real_timings"),Defined);
    bool Clips=true;for(const TCHAR* Name:{TEXT("A_Victory_Rifle_V32"),TEXT("A_Victory_Wave_V32"),TEXT("A_Surrender_V32")})Clips&=LoadObject<UAnimationAsset>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V32/Animations/%s.%s"),Name,Name))!=nullptr;
    Check(TEXT("v32_authored_outcome_clips_load"),Clips);
    bool Nozzles=true;int32 Operators=0;for(AWarfareFrontlineSoldier* Unit:B->Units)if(IsFlameOperator(Unit))
    {++Operators;Nozzles&=Unit->Soldier->DoesSocketExist(TEXT("WF_Muzzle"))&&Unit->Soldier->GetSocketBoneName(TEXT("WF_Muzzle"))==TEXT("weapon_rifle");}
    Check(TEXT("v32_actual_flame_mesh_has_authored_nozzle_socket"),Operators>0&&Nozzles&&B->AssetFailure.IsEmpty());
    AWarfareFrontlineSoldier* U=nullptr;AWarfareFrontlineSoldier* Enemy=nullptr;for(AWarfareFrontlineSoldier* Unit:B->Units)if(Unit&&Unit->IsActiveCombatant()){if(!U)U=Unit;else if(Unit->Team!=U->Team)Enemy=Unit;}
    if(!U){Check(TEXT("v32_living_fixture_available"),false);return;}
    auto& State=Battles.FindOrAdd(B);const FState BeforeState=State;const auto BeforeUnits=B->Units;const auto BeforeSquads=B->Squads;const float BeforeTime=B->Elapsed;const auto BeforePhase=B->Phase;const int32 BeforeWinner=B->BattleOutroWinner;const float BeforeOutro=B->BattleOutroElapsed;
    int32 BankCount=0;bool BanksSeated=true;
    for(const auto& Pair:State.OriginalProps)if(auto* C=Pair.Key.Get();C&&C->GetStaticMesh())
    {
        const FString Name=C->GetStaticMesh()->GetName();if(!Name.Contains(TEXT("Parapet"))&&!Name.Contains(TEXT("TrenchEarth")))continue;
        const FVector P=Pair.Value.GetLocation();const int32 Authored=FMath::Clamp(FMath::RoundToInt(P.X/FrontlineBattlefield::AuthoringTrenchSpacing)+2,0,4);const float Offset=P.X-FrontlineBattlefield::AuthoringTrenchX(Authored);
        const int32 I=FrontlineBattlefield::ActiveFromAuthoring(Authored);if(I<0)continue;
        if(FMath::Abs(Offset)>560)continue;++BankCount;
        const float Expected=AWarfareFrontlineGround::HeightAt(TrenchCentreX(I,P.Y)+(Offset>0?300.f:-300.f),P.Y)-9.f;
        BanksSeated&=FMath::Abs(C->GetComponentLocation().Z-Expected)<.1f;
    }
    Check(TEXT("v32_actual_parapets_seated_on_outer_lip_not_excavation"),BankCount>0&&BanksSeated);
    const FTransform OldTransform=U->GetActorTransform();const auto OldRole=U->Role;const int32 OldMember=U->MemberIndex;const auto UnitState=Units.FindOrAdd(U);const int32 OldShots=U->ShotsDischarged,OldBattleShots=B->ShotsFired;const bool OldEngaging=U->bEngaging;
    const FVector P(2100,800,AWarfareFrontlineGround::HeightAt(2100,800)+88);U->SetActorLocation(P);U->Role=EFrontlineRole::Pioneer;B->Units={U};B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    if(auto* WorkSection=B->FindSquad(U->SquadId)){WorkSection->bAdvancing=true;WorkSection->bHoldPosition=false;}
    State.Hazards.Reset();State.TrainSpawned=true;FHazard Wire;Wire.Centre=U->Feet();Wire.Radius=135;State.Hazards.Add(Wire);FHazard Mine=Wire;Mine.Mine=true;Mine.Radius=FrontlineMineV8214::TriggerRadiusCm;Mine.Centre.Y+=300.f;Mine.Centre.Z=AWarfareFrontlineGround::HeightAt(Mine.Centre.X,Mine.Centre.Y)+2.f;State.Hazards.Add(Mine);
    const float PioneerPace=MovementScale(U);U->Role=EFrontlineRole::Rifle;const float RiflePace=MovementScale(U);U->Role=EFrontlineRole::Pioneer;
    Check(TEXT("v32_wire_has_real_slowdown_and_pioneer_advantage"),PioneerPace>.65f&&RiflePace<.5f);
    for(int32 I=0;I<15;++I){B->Elapsed+=.26f;State.HazardAt=0;Tick(B,.26f);}
    Check(TEXT("v32_pioneer_clears_mines_and_wire_without_detonation"),State.Hazards[0].Cleared&&State.Hazards[1].Cleared&&State.TriggeredMines==BeforeState.TriggeredMines);
    const auto OldZones=B->SmokeZones;FFrontlineSmokeZone Zone;Zone.bGas=true;Zone.Centre=U->Feet();Zone.Radius=950;Zone.StartedAt=B->Elapsed;Zone.ExpiresAt=B->Elapsed+5;B->SmokeZones.Add(Zone);
    const float GasScale=GasHitScale(U);const auto* MaskState=Units.Find(U);
    const bool RespiratorIssued=B->HasTechnology(16,U->Team);
    Check(TEXT("v32_gas_mask_obeys_research_and_has_no_default_protection"),MaskState&&MaskState->Mask.IsValid()
        &&MaskState->MaskOn==RespiratorIssued&&MaskState->Mask->IsVisible()==RespiratorIssued
        &&(RespiratorIssued?GasScale<.1f:GasScale==1.f));
    B->SmokeZones=OldZones;U->Role=EFrontlineRole::Flamethrower;U->MemberIndex=0;
    Check(TEXT("v32_flame_operator_has_short_range_profile"),FrontlineWeapons::ProfileFor(U).FireRangeCm==1450&&FrontlineWeapons::AdvanceContactRangeFor(U)==1100);
    if(Enemy)
    {
        const FTransform ET=Enemy->GetActorTransform();const float EP=Enemy->Suppression;Enemy->SetActorLocation(P+FVector(600,0,0));B->Units={U,Enemy};
        auto& FS=Units.FindOrAdd(U);FS.Fuel=1;FS.ReserveSpent=true;FS.FlameReady=0;for(int32 Seed=0;Seed<100;++Seed){FRandomStream R(Seed);if(R.FRand()>.68f){FS.Random.Initialize(Seed);break;}}
        FlameDischarge(U,Enemy);const float Empty=FS.Fuel;const int32 Fired=U->ShotsDischarged;FlameDischarge(U,Enemy);
        Check(TEXT("v32_flame_has_finite_fuel_and_cannot_fire_when_empty"),Empty==0&&FS.Fuel==0&&Fired==OldShots+1&&U->ShotsDischarged==Fired);
        Enemy->SetActorTransform(ET);Enemy->Suppression=EP;
    }
    State=BeforeState;B->Elapsed=BeforeTime;B->Phase=BeforePhase;B->BattleOutroWinner=BeforeWinner;B->BattleOutroElapsed=BeforeOutro;B->Units=BeforeUnits;B->Squads=BeforeSquads;B->ShotsFired=OldBattleShots;U->Role=OldRole;U->MemberIndex=OldMember;U->ShotsDischarged=OldShots;U->bEngaging=OldEngaging;U->SetActorTransform(OldTransform);Units.FindOrAdd(U)=UnitState;UpdateGasKit(U,Units.FindChecked(U));
}
void RunFocusedChecksV33(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B)return;auto* FX=AFrontlineFuelFireV33::Get(B->GetWorld());
    Check(TEXT("v33_original_fuel_materials_ready"),FX&&FX->IsReady());
    bool Masks=true;for(const TCHAR* F:{TEXT("British"),TEXT("German")})
        Masks&=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V33/Meshes/SK_%s_Respirator_V33.SK_%s_Respirator_V33"),F,F))!=nullptr;
    Check(TEXT("v33_both_anatomical_pose_follower_respirators_load"),Masks);
    const auto BeforeUnits=B->Units;const float BeforeTime=B->Elapsed;const auto BeforePhase=B->Phase;const auto BeforeState=Battles.FindOrAdd(B);const int32 BeforeWinner=B->BattleOutroWinner;
    const int32 C0=B->Casualties[0],C1=B->Casualties[1],BeforeShots=B->ShotsFired;const float M0=B->Morale[0],M1=B->Morale[1],D0=B->DamageDealt[0],D1=B->DamageDealt[1],FF=B->FriendlyFireDamage,Pressure=B->PeakSuppression;const FString Message=B->LastMessage;
    B->Units.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=200;
    auto& State=Battles.FindOrAdd(B);State.Hazards.Empty();State.JetContacts.Empty();State.TrainSpawned=true;State.FuelBursts=0;State.BurningCasualties=0;
    TArray<AWarfareFrontlineSoldier*> Fixtures;
    for(int32 I=0;I<5;++I)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();Fixtures.Add(U);U->Initialize(B,9300+I,I==0?0:1,I==2?EFrontlineRole::Rifle:EFrontlineRole::Flamethrower,0);U->bFieldEntered=true;U->SetActorTickEnabled(false);
        const float X=I==4?10500:9100+I*42;U->SetActorLocation(FVector(X,1000,AWarfareFrontlineGround::HeightAt(X,1000)+88));B->Units.Add(U);Units.FindOrAdd(U).Fuel=I==3?0:12;
    }
    Check(TEXT("v33_two_complete_burning_fall_clips_load"),BurningFallV33(Fixtures[0])&&BurningFallV33(Fixtures[1]));
    bool BothPhysics=true;for(int32 I=0;I<2;++I)
    {
        auto* U=Fixtures[I];auto* Physics=U->Soldier->GetPhysicsAsset();const int32 BaseIndex=U->GetPresentationRoleIndex();
        BothPhysics&=Physics&&!Physics->SkeletalBodySetups.IsEmpty()&&B->V16SoldierMeshes.IsValidIndex(BaseIndex)&&B->V16SoldierMeshes[BaseIndex]&&Physics==B->V16SoldierMeshes[BaseIndex]->GetPhysicsAsset();
    }
    Check(TEXT("v33_both_flame_variants_retain_accepted_body_physics_source"),BothPhysics);
    auto* Operator=Fixtures[0];FVector BraceAim;const FVector BeforeBrace=Operator->GetActorLocation();
    Operator->SetActorRotation(FRotator(0,180,0));Operator->bMoving=true;
    FlameDischarge(Operator,Fixtures[1]);Operator->Tick(.016f);
    const auto& BracedState=Units.FindChecked(Operator);
    Check(TEXT("v33_live_discharge_braces_before_valve_and_stops_slot_movement"),FlameBraceDirectionV33(Operator,BraceAim)&&!Operator->bMoving&&Operator->GetMotionSpeed()==0&&Operator->GetActorLocation().Equals(BeforeBrace,.01f)&&FVector::DotProduct(Operator->GetActorForwardVector(),BraceAim)>.999f&&BracedState.FlameStarted>B->Elapsed);
    B->Elapsed=201.2f;const bool HoldsLateJet=FlameBraceDirectionV33(Operator,BraceAim);B->Elapsed=201.34f;
    Check(TEXT("v33_discharge_owns_aim_through_tail_then_releases"),HoldsLateJet&&!FlameBraceDirectionV33(Operator,BraceAim));
    B->Elapsed=200;auto& ClearedBrace=Units.FindChecked(Operator);ClearedBrace.FlameStarted=ClearedBrace.FlameUntil=ClearedBrace.FlameReady=0;ClearedBrace.Fuel=12;State.JetContacts.Empty();
    Fixtures[0]->ReceiveHit(1,.8f,1,FVector::ForwardVector,EFrontlineDamageKind::Grenade,.7f);
    Fixtures[4]->ReceiveHit(1,.1f,0,FVector::ForwardVector,EFrontlineDamageKind::Bullet,.1f);
    Fixtures[4]->ReceiveHit(1,.1f,0,FVector::ForwardVector,EFrontlineDamageKind::Bullet,.1f);
    Check(TEXT("v33_lethal_blast_schedules_fuel_but_bullet_does_not"),Units.FindChecked(Fixtures[0]).FuelIgnited&&!Units.FindChecked(Fixtures[4]).FuelIgnited&&!Fixtures[0]->IsActiveCombatant());
    B->Elapsed=200.2f;Tick(B,.2f);Check(TEXT("v33_reservoir_rupture_has_readable_delay"),State.FuelBursts==0&&Fixtures[1]->IsAlive());
    B->Elapsed=200.85f;Tick(B,.05f);Check(TEXT("v33_one_rupture_ignites_nearby_men_and_other_fuel"),State.FuelBursts==1&&!Fixtures[1]->IsActiveCombatant()&&!Fixtures[2]->IsAlive()&&Units.FindChecked(Fixtures[1]).FuelIgnited&&Units.FindChecked(Fixtures[2]).BurnSeconds>0);
    Check(TEXT("v33_empty_reservoir_never_explodes"),!Units.FindChecked(Fixtures[3]).FuelIgnited);
    B->Elapsed=Units.FindChecked(Fixtures[1]).FuelBurstAt+.05f;Tick(B,.05f);const int32 Bursts=State.FuelBursts;B->Elapsed=205.f;Tick(B,.05f);
    Check(TEXT("v33_chain_is_delayed_finite_and_each_reservoir_spent_once"),Bursts==2&&State.FuelBursts==2&&Units.FindChecked(Fixtures[0]).FuelSpent&&Units.FindChecked(Fixtures[1]).FuelSpent&&Units.FindChecked(Fixtures[1]).Fuel==0);
    const int32 Dead=B->Casualties[1];Fixtures[1]->ReceiveHit(1,1,0,FVector::ForwardVector,EFrontlineDamageKind::Fire);
    Check(TEXT("v33_burning_casualty_cannot_fight_or_die_twice"),!Fixtures[1]->IsActiveCombatant()&&B->Casualties[1]==Dead);
    // Sample the actual authored endpoint and invoke the ordinary death tick;
    // this checks source binding and handoff, not a simulated ragdoll settle.
    if(auto* Animation=Fixtures[1]->Soldier->GetSingleNodeInstance())
    {Animation->SetPosition(Animation->GetLength(),false);Fixtures[1]->Soldier->RefreshBoneTransforms();Fixtures[1]->TickDeath(.016f);}
    const auto& BurnDeath=Fixtures[1]->GetDeathState();
    Check(TEXT("v33_burning_fall_hands_complete_pose_to_real_physics"),BurnDeath.bAuthoredFall&&BurnDeath.bPhysicsStarted&&!BurnDeath.bMissingPhysics&&!Fixtures[1]->Soldier->Bodies.IsEmpty());
    Check(TEXT("v33_shared_cosmetic_pool_has_fixed_budget"),FX&&FX->ActiveCount()<=AFrontlineFuelFireV33::FlameBudget+AFrontlineFuelFireV33::SmokeBudget&&AFrontlineFuelFireV33::LightBudget==2);
    for(auto* U:Fixtures){Units.Remove(U);U->Destroy();}
    B->Units=BeforeUnits;B->Elapsed=BeforeTime;B->Phase=BeforePhase;B->BattleOutroWinner=BeforeWinner;Battles.FindOrAdd(B)=BeforeState;
    B->Casualties[0]=C0;B->Casualties[1]=C1;B->ShotsFired=BeforeShots;B->Morale[0]=M0;B->Morale[1]=M1;B->DamageDealt[0]=D0;B->DamageDealt[1]=D1;B->FriendlyFireDamage=FF;B->PeakSuppression=Pressure;B->LastMessage=Message;if(FX)FX->Clear();
}
}
