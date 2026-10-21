#include "FRRUVehicleNavV87.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "Native/SceneRuntime.h"
#include "Native/TacticalRuntime.h"
#include "AI/NavDataGenerator.h"
#include "AI/Navigation/NavigationBounds.h"
#include "AI/Navigation/NavigationTypes.h"
#include "AI/NavigationSystemBase.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "Dom/JsonObject.h"
#endif

namespace FRRUVehicleNavV87
{
namespace
{
constexpr float Radius=100,Height=220,Cell=15,CellHeight=2,Step=10;
constexpr double Deadline=45,ReplanInterval=5,BlockedTime=3,WaypointRadius=25,EpisodeBackoff=30;
constexpr int32 MaxPoints=128,EpisodeAttempts=4;
struct FSource { TWeakObjectPtr<UStaticMeshComponent> Component;bool Navigation=false; };
struct FFloor
{
    FVector Projected=FVector::ZeroVector;double Native=0;FHitResult Ground,Solid;
    bool GroundHit=false,SolidHit=false,Walkable=false,Passed=false;FString Reason;
};
struct FQuery
{
    FVector Actual=FVector::ZeroVector,Source=FVector::ZeroVector,Target=FVector::ZeroVector,From=FVector::ZeroVector,To=FVector::ZeroVector;
    FFloor FromFloor,ToFloor;TArray<FVector> Points;TArray<bool> NativeLegs;
    bool FromProjected=false,ToProjected=false,Partial=false,Limit=false,Complete=false,NativeClear=false,EndpointTail=false,Passed=false;
    int32 Result=0,Count=0;double Host=0,Length=0;FString Reason;
};
struct FGuide
{
    TWeakObjectPtr<AFrontlineTank> Vehicle;TArray<FVector> Points;FVector Target=FVector::ZeroVector,EpisodeOrigin=FVector::ZeroVector;
    int32 Serial=INDEX_NONE,Next=0,Attempts=0,Replans=0;double RetryAt=0,Blocked=0,LastHost=-1,ActualTravel=0,GuidedTravel=0,EpisodeResumeAt=0;
    float LastYaw=0;bool Engaged=false,HandedOff=false,Failed=false,Guiding=false;FString Reason;
#if WITH_EDITOR
    TArray<FQuery> Queries;TSharedPtr<FJsonObject> Live,LastPlan;double LastPlanAt=-1;
#endif
};
struct FState
{
    FResult Build;TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;TWeakObjectPtr<AActor> Scene;TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<UNavigationSystemV1> Nav;TWeakObjectPtr<ARecastNavMesh> Data;TWeakObjectPtr<ANavMeshBoundsVolume> Volume;
    TArray<FSource> Sources;TArray<FNavDataConfig> PreviousAgents;FNavAgentSelector PreviousMask;
    TMap<TWeakObjectPtr<AFrontlineTank>,FGuide> Guides;FGuid Encounter;FString Mission,SceneId;
    FBox Bounds=FBox(ForceInit);double Started=0,BuildStarted=0,Travel=0;uint32 BoundsId=0;uint64 StartFrame=0;
    float CDORadius=0,CDOSlope=0;int32 Tiles=0,Remaining=0,Running=0;
    bool Created=false,Configured=false,PreviousAuto=false,Registered=false,BoundsRegistered=false,Requested=false,Released=false,Restored=false,CDOUnchanged=false;
#if WITH_EDITOR
    bool Review=false,QueryOnly=false;FQuery Warm;FResult Follow={EStatus::Pending,TEXT("waiting for actual FT")};
#endif
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TSharedPtr<FState>> States;
FArrayProperty* AgentsProperty(UNavigationSystemV1* Nav)
{
    auto* P=FindFProperty<FArrayProperty>(Nav->GetClass(),TEXT("SupportedAgents"));const auto* I=P?CastField<FStructProperty>(P->Inner):nullptr;
    return I&&I->Struct==FNavDataConfig::StaticStruct()?P:nullptr;
}
FStructProperty* MaskProperty(UNavigationSystemV1* Nav)
{
    auto* P=FindFProperty<FStructProperty>(Nav->GetClass(),TEXT("SupportedAgentsMask"));return P&&P->Struct==FNavAgentSelector::StaticStruct()?P:nullptr;
}
void Release(FState& S)
{
    if(S.Released)return;S.Released=true;S.Restored=true;
    for(const auto& X:S.Sources)if(auto* C=X.Component.Get()){C->SetCanEverAffectNavigation(X.Navigation);S.Restored&=C->CanEverAffectNavigation()==X.Navigation;}else S.Restored=false;
    auto* Nav=S.Nav.Get();if(auto* Data=S.Data.Get()){if(Nav)Nav->UnregisterNavData(Data);Data->Destroy();}
    if(auto* V=S.Volume.Get()){if(Nav)Nav->OnNavigationBoundsRemoved(V);V->Destroy();}
    if(Nav&&S.Configured)
    {
        auto* A=AgentsProperty(Nav);auto* M=MaskProperty(Nav);auto* Auto=FindFProperty<FBoolProperty>(Nav->GetClass(),TEXT("bAutoCreateNavigationData"));
        if(A&&M&&Auto){*A->ContainerPtrToValuePtr<TArray<FNavDataConfig>>(Nav)=S.PreviousAgents;*M->ContainerPtrToValuePtr<FNavAgentSelector>(Nav)=S.PreviousMask;Auto->SetPropertyValue_InContainer(Nav,S.PreviousAuto);}
        else S.Restored=false;
    }
    if(S.Created)if(auto* W=S.World.Get();W&&W->GetNavigationSystem()==Nav)W->SetNavigationSystem(nullptr);
    const auto* CDO=GetDefault<ARecastNavMesh>();S.CDOUnchanged=CDO->AgentRadius==S.CDORadius&&CDO->AgentMaxSlope==S.CDOSlope;
}
FResult Fail(FState& S,const FString& Why){S.Build={EStatus::Failed,Why};Release(S);return S.Build;}
void WorldCleanup(UWorld* World,bool,bool)
{
    for(auto I=States.CreateIterator();I;++I)if(I.Value()->World.Get()==World){Release(*I.Value());I.RemoveCurrent();}
}
bool Model(const FFRRUArmourDefinitionV82* D)
{
    return D&&D->Id==TEXT("fr.ft")&&D->Token==TEXT("fr_ft")&&D->Minimum.Equals(FVector(-304,-87.4042,0),.0001)&&D->Maximum.Equals(FVector(195,87.4042,212),.0001);
}
bool Grid(const wfexp::scene::SceneDefinition& G,const FFRRUArmourDefinitionV82& D,const FVector& P,float Yaw)
{
    if(P.ContainsNaN())return false;const FTransform T(FRotator(0,Yaw,0),P);
    for(double X:{D.Minimum.X,D.Maximum.X})for(double Y:{D.Minimum.Y,D.Maximum.Y})
    {const FVector Q=T.TransformPosition({X,Y,0});if(Q.X<G.minX+130||Q.X>G.maxX-130||Q.Y<G.minY+130||Q.Y>G.maxY-130)return false;}return true;
}
double Cap(const wfexp::scene::SceneDefinition& G,const FFRRUArmourDefinitionV82& D,int32 Dir)
{
    // A boundary margin only, not the transverse Nav agent radius.
    const double X=FMath::Max(FMath::Abs(D.Minimum.X),FMath::Abs(D.Maximum.X))+12;
    const double Y=FMath::Max(FMath::Abs(D.Minimum.Y),FMath::Abs(D.Maximum.Y))+12;
    const double Margin=130+FMath::Sqrt(X*X+Y*Y);
    return Dir>0?G.maxX-Margin:G.minX+Margin;
}
bool Target(const wfexp::scene::SceneDefinition& G,const FFRRUArmourDefinitionV82& D,const FVector& Actual,int32 Dir,FVector& Out)
{
    const double Margin=130+FMath::Sqrt(FMath::Square(FMath::Max(FMath::Abs(D.Minimum.X),FMath::Abs(D.Maximum.X))+12)+FMath::Square(FMath::Max(FMath::Abs(D.Minimum.Y),FMath::Abs(D.Maximum.Y))+12));
    const double Y=FMath::Clamp(Actual.Y,G.minY+Margin,G.maxY-Margin);double Z=0;const double X=Cap(G,D,Dir);
    if(!wfexp::scene::HeightAt(G,X,Y,Z))return false;Out={X,Y,Z};return true;
}
#if WITH_EDITOR
bool NativeNode(const FState& S,const char* Name,FVector& P)
{
    const auto* M=wfexp::tactical::FindMission(TCHAR_TO_UTF8(*S.Mission));const auto* G=wfexp::scene::ForMission(TCHAR_TO_UTF8(*S.Mission));if(!M||!G)return false;
    for(const auto& N:M->nodes)if(N.id==Name){double Z=0;if(!wfexp::scene::HeightAt(*G,N.position.x,N.position.y,Z))return false;P={N.position.x,N.position.y,Z};return true;}return false;
}
#endif
bool PhysicalFloor(FState& S,const wfexp::scene::SceneDefinition& G,const FVector& P,FFloor& Out)
{
    Out.Projected=P;
    auto No=[&](const TCHAR* Why){Out.Reason=Why;return false;};
    if(P.ContainsNaN()||!wfexp::scene::HeightAt(G,P.X,P.Y,Out.Native))return No(TEXT("projected XY has no native floor"));
    const auto F=wfexp::scene::FootingAt(G,P.X,P.Y);Out.Walkable=F.valid&&F.walkable;
    UStaticMeshComponent* Ground=nullptr;UStaticMeshComponent* Solids=nullptr;
    for(const auto& X:S.Sources)if(auto* C=X.Component.Get())
    {
        if(!C->IsRegistered()||!C->IsPhysicsStateCreated()||C->GetOwner()!=S.Scene.Get()||C->GetWorld()!=S.World.Get()||!C->ComponentHasTag(TEXT("FRRU_V82Collision"))||C->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics)return No(TEXT("registered native physics source changed"));
        if(C->GetFName()==TEXT("ground"))Ground=C;else if(C->GetFName()==TEXT("solids"))Solids=C;
    }
    if(!Ground||!Solids)return No(TEXT("registered native ground/solids missing"));
    const FVector A(P.X,P.Y,Out.Native+1000),B(P.X,P.Y,Out.Native-1000);
    FCollisionQueryParams GQ(SCENE_QUERY_STAT(FRRUV87NavGround),true),SQ(SCENE_QUERY_STAT(FRRUV87NavSolids),false);
    Out.GroundHit=Ground->LineTraceComponent(Out.Ground,A,B,GQ);Out.SolidHit=Solids->LineTraceComponent(Out.Solid,A,B,SQ);
    if(!Out.Walkable||!Out.GroundHit||Out.Ground.bStartPenetrating||FMath::Abs(Out.Ground.ImpactPoint.Z-Out.Native)>.5)return No(TEXT("original native/actual ground 0.5cm contract refused"));
    if(Out.SolidHit&&(Out.Solid.bStartPenetrating||Out.Solid.Time<=Out.Ground.Time))return No(TEXT("solid/roof precedes native ground"));
    Out.Passed=true;return true;
}

bool Route(FState& S,const FVector& Actual,const FVector& Goal,TArray<FVector>& Path,FString& Why,FQuery* Trace=nullptr)
{
    auto Bad=[&](const TCHAR* R){Why=R;if(Trace)Trace->Reason=R;return false;};
    auto* B=S.Battle.Get();auto* N=S.Nav.Get();auto* D=S.Data.Get();const auto* G=wfexp::scene::ForMission(TCHAR_TO_UTF8(*S.Mission));
    double Z=0;if(!B||!N||!D||!G||Actual.ContainsNaN()||!wfexp::scene::HeightAt(*G,Actual.X,Actual.Y,Z))return Bad(TEXT("actual source/native navigation data unavailable"));
    const FVector Source(Actual.X,Actual.Y,Z);FNavLocation A,T;
    const bool AP=N->ProjectPointToNavigation(Source,A,FVector(Radius,Radius,64),D),TP=N->ProjectPointToNavigation(Goal,T,FVector(Radius,Radius,64),D);
    if(Trace){Trace->Actual=Actual;Trace->Source=Source;Trace->Target=Goal;Trace->Host=B->Elapsed;Trace->FromProjected=AP;Trace->ToProjected=TP;if(AP)Trace->From=A.Location;if(TP)Trace->To=T.Location;}
    if(!AP||!TP)return Bad(TEXT("bounded actual-source/target projection unavailable"));
    if(FVector::Dist2D(Source,A.Location)>Radius||FVector::Dist2D(Goal,T.Location)>Radius)return Bad(TEXT("projection leaves bounded native endpoint neighbourhood"));
    FFloor AF,TF;const bool AC=PhysicalFloor(S,*G,A.Location,AF),TC=PhysicalFloor(S,*G,T.Location,TF);
    if(Trace){Trace->FromFloor=AF;Trace->ToFloor=TF;}
    if(!AC||!TC){Why=TEXT("physical/native floor refused: ")+(AC?TF.Reason:AF.Reason);if(Trace)Trace->Reason=Why;return false;}
    FPathFindingQuery Q(B,*D,A.Location,T.Location);Q.SetAllowPartialPaths(false).SetRequireNavigableEndLocation(true);
    const auto R=N->FindPathSync(D->GetConfig(),Q);const bool Has=R.Path.IsValid();
    const bool Partial=Has&&R.Path->IsPartial(),Limit=Has&&R.Path->DidSearchReachedLimit(),Complete=R.IsSuccessful()&&Has&&R.Path->IsValid()&&!Partial&&!Limit;
    if(Trace){Trace->Result=int32(R.Result);Trace->Partial=Partial;Trace->Limit=Limit;Trace->Complete=Complete;}
    if(!Has)return Bad(TEXT("complete route unavailable; no path object"));
    const auto& Points=R.Path->GetPathPoints();FVector Previous=Source;bool Native=true,Finite=true;double Length=0;
    for(int32 I=0;I<FMath::Min(Points.Num(),MaxPoints);++I)
    {
        const FVector P=Points[I].Location;const bool Valid=!Previous.ContainsNaN()&&!P.ContainsNaN();
        const bool Clear=Valid&&WarfareFRRUWorldV82::CanTraverse(B,Previous,P);Finite&=Valid;Native&=Clear;
        if(Trace){Trace->Points.Add(P);Trace->NativeLegs.Add(Clear);}if(Valid)Length+=FVector::Dist2D(Previous,P);Previous=P;
    }
    const bool Tail=Points.Num()>0&&!Points.Last().Location.ContainsNaN()&&WarfareFRRUWorldV82::CanTraverse(B,Points.Last().Location,Goal);
    if(Trace){Trace->Count=Points.Num();Trace->EndpointTail=Tail;Trace->NativeClear=Native&&Tail&&Finite&&Points.Num()<=MaxPoints;Trace->Length=Length+(Tail?FVector::Dist2D(Points.Last().Location,Goal):0);}
    if(!Complete)return Bad(TEXT("partial/failed/search-limited complete route refused"));
    if(Points.Num()<2||Points.Num()>MaxPoints||!Finite||!Native||!Tail)return Bad(TEXT("complete route original native leg/endpoint connection refused"));
    Path.Reset();Path.Add(Source);for(const auto& P:Points)Path.Add(P.Location);Path.Add(Goal);
    if(Trace)Trace->Passed=true;Why=TEXT("complete transverse-clearance route with original native endpoint connections; FT Plan still required");return true;
}
FResult BeginImpl(AWarfareFrontlineGameMode* B,AActor* Scene,bool Review,bool QueryOnly)
{
    if(const auto* Existing=States.Find(B))return (*Existing)->Build;
    if(!IsInGameThread()||!EligibleScene(B)||!Scene||!Scene->ActorHasTag(TEXT("FRRU_V82Scene"))||Scene->GetWorld()!=B->GetWorld()
        ||!Scene->GetActorTransform().Equals(FTransform(FQuat::Identity,FVector::ZeroVector,FVector(1,-1,1)),1.e-8))
        return {EStatus::Failed,TEXT("eligible French FT native scene owner required")};
    auto S=MakeShared<FState>();S->Battle=B;S->Scene=Scene;S->World=B->GetWorld();S->Encounter=B->GetEncounterId();
    S->Mission=WarfareFRRUCampaignV82::MissionId(B);S->Started=FPlatformTime::Seconds();S->StartFrame=GFrameCounter;S->Build={EStatus::Pending,TEXT("waiting for isolated runtime bounds/navdata registration")};
#if WITH_EDITOR
    S->Review=Review;S->QueryOnly=QueryOnly;
#else
    (void)Review;(void)QueryOnly;
#endif
    States.Add(B,S);static bool CleanupBound=false;if(!CleanupBound){FWorldDelegates::OnWorldCleanup.AddStatic(&WorldCleanup);CleanupBound=true;}
    const auto* CDO=GetDefault<ARecastNavMesh>();S->CDORadius=CDO->AgentRadius;S->CDOSlope=CDO->AgentMaxSlope;
    const auto* G=wfexp::scene::ForMission(TCHAR_TO_UTF8(*S->Mission));const auto* FT=WarfareFRRUArmourV82::Definition(FString(TEXT("fr.ft")));
    if(!G||!S->Encounter.IsValid()||!Model(FT))return Fail(*S,TEXT("current encounter/FT model bounds contract unavailable; no straight-line fallback"));
    S->SceneId=UTF8_TO_TCHAR(G->id);
    if(S->SceneId!=S->Mission+TEXT(".scene.families.v1"))return Fail(*S,TEXT("exact current native scene family required"));
    TArray<UStaticMeshComponent*> Parts;Scene->GetComponents(Parts);UStaticMeshComponent* Floor=nullptr;int32 SolidCount=0;
    for(auto* C:Parts)if(C&&C->ComponentHasTag(TEXT("FRRU_V82Collision")))
    {
        const UStaticMesh* M=C->GetStaticMesh();const UBodySetup* Body=M?M->GetBodySetup():nullptr;
        if(!C->IsRegistered()||!Body||C->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics||C->GetCollisionProfileName()!=TEXT("BlockAll")||C->CanEverAffectNavigation())return Fail(*S,TEXT("original staged collision/nav contract differs"));
        if(C->GetFName()==TEXT("ground")&&Body->CollisionTraceFlag==CTF_UseComplexAsSimple)Floor=C;
        else if(C->GetFName()==TEXT("solids")&&Body->CollisionTraceFlag==CTF_UseSimpleAsComplex)++SolidCount;
        else return Fail(*S,TEXT("unexpected native collision source"));
        S->Sources.Add({C,C->CanEverAffectNavigation()});
    }
    if(!Floor||SolidCount!=1||S->Sources.Num()!=2)return Fail(*S,TEXT("exact original ground and solids required"));
    auto* W=B->GetWorld();auto* N=UNavigationSystemV1::GetCurrent(W);
    if(W->GetNavigationSystem()&&!N)return Fail(*S,TEXT("existing unsupported navigation system retained"));
    for(TActorIterator<ARecastNavMesh> I(W);I;++I)return Fail(*S,TEXT("existing Recast data retained; no takeover"));
    if(N&&(N->GetNavigationBounds().Num()||N->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)||N->IsNavigationBuildInProgress()))return Fail(*S,TEXT("existing navigation bounds/data/build retained"));
    if(UNavigationSystemV1::IsNavigationSystemStatic())return Fail(*S,TEXT("runtime navigation is configured static; no global override"));
    if(!N){N=NewObject<UNavigationSystemV1>(W,NAME_None,RF_Transient);if(!N)return Fail(*S,TEXT("navigation system allocation failed"));S->Nav=N;S->Created=true;W->SetNavigationSystem(N);N->OnWorldInitDone(FNavigationSystemRunMode::GameMode);}else S->Nav=N;
    auto* Agents=AgentsProperty(N);auto* Mask=MaskProperty(N);auto* Auto=FindFProperty<FBoolProperty>(N->GetClass(),TEXT("bAutoCreateNavigationData"));
    if(!Agents||!Mask||!Auto)return Fail(*S,TEXT("instance navigation property type/struct contract unavailable"));
    S->PreviousAgents=*Agents->ContainerPtrToValuePtr<TArray<FNavDataConfig>>(N);S->PreviousMask=*Mask->ContainerPtrToValuePtr<FNavAgentSelector>(N);S->PreviousAuto=Auto->GetPropertyValue_InContainer(N);
    FNavDataConfig Agent(Radius,Height);Agent.Name=TEXT("FRRU_FT_RUNTIME_V87");Agent.AgentStepHeight=Step;Agent.SetNavDataClass(ARecastNavMesh::StaticClass());
    *Agents->ContainerPtrToValuePtr<TArray<FNavDataConfig>>(N)={Agent};FNavAgentSelector Selection(1u);Selection.MarkInitialized();
    *Mask->ContainerPtrToValuePtr<FNavAgentSelector>(N)=Selection;Auto->SetPropertyValue_InContainer(N,false);S->Configured=true;
    FActorSpawnParameters Spawn;Spawn.ObjectFlags=RF_Transient;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;Spawn.bDeferConstruction=true;
    auto* D=W->SpawnActor<ARecastNavMesh>(ARecastNavMesh::StaticClass(),FTransform::Identity,Spawn);S->Data=D;if(!D)return Fail(*S,TEXT("runtime Recast allocation failed"));
    D->SetConfig(Agent);D->AgentMaxSlope=FMath::RadiansToDegrees(FMath::Atan(.65f));D->bDoFullyAsyncNavDataGathering=true;D->bFixedTilePoolSize=true;D->TilePoolSize=128;D->TileNumberHardLimit=128;D->TileSizeUU=2048;D->DefaultMaxSearchNodes=8192;D->MaxSimplificationError=1;
    for(uint8 I=0;I<uint8(ENavigationDataResolution::MAX);++I){D->SetCellSize(ENavigationDataResolution(I),Cell);D->SetCellHeight(ENavigationDataResolution(I),CellHeight);D->SetAgentMaxStepHeight(ENavigationDataResolution(I),Step);}
    D->SetMaxSimultaneousTileGenerationJobsCount(2);D->MarkRequiresInitialRebuild();D->FinishSpawning(FTransform::Identity);
    auto* Runtime=FindFProperty<FEnumProperty>(D->GetClass(),TEXT("RuntimeGeneration"));
    if(!Runtime||Runtime->GetEnum()!=StaticEnum<ERuntimeGenerationType>())return Fail(*S,TEXT("runtime generation instance enum contract unavailable"));
    Runtime->GetUnderlyingProperty()->SetIntPropertyValue(Runtime->ContainerPtrToValuePtr<void>(D),int64(ERuntimeGenerationType::Dynamic));N->RequestRegistrationDeferred(*D);
    const FBox FB=Floor->Bounds.GetBox();S->Bounds=FBox(FVector(G->minX,G->minY,FB.Min.Z-300),FVector(G->maxX,G->maxY,FB.Max.Z+Height+300));
    Spawn.bDeferConstruction=false;auto* V=W->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(),FTransform(FQuat::Identity,S->Bounds.GetCenter()),Spawn);S->Volume=V;
    if(!V)return Fail(*S,TEXT("runtime bounds allocation failed"));V->SupportedAgents=Selection;
    V->GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->GetBrushComponent()->SetCanEverAffectNavigation(false);
    auto* Box=NewObject<UBoxComponent>(V,NAME_None,RF_Transient);V->AddInstanceComponent(Box);Box->SetupAttachment(V->GetRootComponent());Box->SetBoxExtent(S->Bounds.GetExtent());Box->SetRelativeLocation(FVector::ZeroVector);
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Box->SetGenerateOverlapEvents(false);Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();Box->UpdateBounds();
    S->BoundsId=V->GetUniqueID();N->OnNavigationBoundsUpdated(V);
    const FBox Actual=V->GetComponentsBoundingBox(true);if(!Actual.Min.Equals(S->Bounds.Min,.1)||!Actual.Max.Equals(S->Bounds.Max,.1))return Fail(*S,TEXT("runtime UBox bounds differ from native area"));
    return S->Build;
}
}
bool EligibleScene(const AWarfareFrontlineGameMode* B)
{
    if(!B||!B->GetWorld()||B->GetWorld()->WorldType!=EWorldType::Game||!WarfareFRRUCampaignV82::Active(B)||WarfareFRRUCampaignV82::Nation(B)!=EFRRUNationV81::French)return false;
    const FString M=WarfareFRRUCampaignV82::MissionId(B);return M==TEXT("fr.matz")||M==TEXT("fr.soissons")||M==TEXT("fr.sommepy");
}
FResult Begin(AWarfareFrontlineGameMode* B,AActor* Scene){return BeginImpl(B,Scene,false,false);}
FResult Poll(AWarfareFrontlineGameMode* B)
{
    const auto* Found=States.Find(B);if(!Found)return {EStatus::Failed,TEXT("runtime FT navigation scene not prepared")};auto& S=**Found;
    if(S.Build.Status!=EStatus::Pending)return S.Build;
    if(!EligibleScene(B)||S.Encounter!=B->GetEncounterId()||!S.Scene.IsValid())return Fail(S,TEXT("native navigation scene/encounter changed"));
    auto* N=S.Nav.Get();auto* D=S.Data.Get();if(!N||!D)return Fail(S,TEXT("owned navigation resources disappeared"));
    if(S.BuildStarted==0)S.BuildStarted=FPlatformTime::Seconds();
    if(FPlatformTime::Seconds()-S.BuildStarted>Deadline)return Fail(S,TEXT("bounded native navigation build deadline exceeded"));
    if(!S.Requested)
    {
        S.Registered=N->GetNavDataForAgentName(TEXT("FRRU_FT_RUNTIME_V87"))==D;S.BoundsRegistered=false;
        for(const auto& Bounds:N->GetNavigationBounds())if(Bounds.UniqueID==S.BoundsId&&Bounds.AreaBox.Min.Equals(S.Bounds.Min,.1)&&Bounds.AreaBox.Max.Equals(S.Bounds.Max,.1)){S.BoundsRegistered=true;break;}
        if(!S.Registered||!S.BoundsRegistered||!N->GetNavOctree())return S.Build;
        for(const auto& X:S.Sources)if(auto* C=X.Component.Get()){C->SetCanEverAffectNavigation(true);UNavigationSystemV1::UpdateComponentInNavOctree(*C);}
        for(TActorIterator<AActor> I(B->GetWorld());I;++I)
        {TArray<UPrimitiveComponent*> C;I->GetComponents(C);for(const auto* P:C)if(P&&P->IsRegistered()&&P->CanEverAffectNavigation()&&P->IsNavigationRelevant()&&!S.Sources.ContainsByPredicate([&](const FSource& X){return X.Component.Get()==P;}))return Fail(S,TEXT("non-native navigation geometry detected"));}
        // Bounds must already be confirmed above; Flush discards pending bounds.
        N->FlushPendingOperations();D->ConditionalConstructGenerator();auto* Generator=D->GetGenerator();if(!Generator||!Generator->RebuildAll())return Fail(S,TEXT("owned nonblocking rebuild request failed"));
        S.Requested=true;return S.Build;
    }
    auto* Generator=D->GetGenerator();if(!Generator)return Fail(S,TEXT("runtime navigation generator unavailable"));
    S.Tiles=D->GetNumActiveTiles();S.Remaining=Generator->GetNumRemaningBuildTasks();S.Running=Generator->GetNumRunningBuildTasks();
    if(Generator->IsBuildInProgressCheckDirty()||GFrameCounter<=S.StartFrame+2)return S.Build;
    if(S.Tiles<=0)return Fail(S,TEXT("completed runtime build has no populated tiles"));
#if WITH_EDITOR
    if(S.Review)
    {
        FVector From,To;const auto* G=wfexp::scene::ForMission(TCHAR_TO_UTF8(*S.Mission));const auto* DFT=WarfareFRRUArmourV82::Definition(FString(TEXT("fr.ft")));
        if(!G||!NativeNode(S,"home",From)||(S.QueryOnly?!NativeNode(S,"objective",To):!Target(*G,*DFT,From,B->GetTeamDirection(0),To)))return Fail(S,TEXT("review native endpoints unavailable"));
        TArray<FVector> Path;FString Why;if(!Route(S,From,To,Path,Why,&S.Warm))return Fail(S,Why);
    }
#endif
    S.Build={EStatus::Success,TEXT("runtime native navigation ready; individual FT full route and original Plan remain required")};
#if WITH_EDITOR
    if(S.Review&&S.QueryOnly)Release(S);
#endif
    return S.Build;
}

FGuidance FollowHeading(AWarfareFrontlineGameMode* B,AFrontlineTank* V)
{
    FGuidance Out;if(!EligibleScene(B)||!V||V->Team!=0||WarfareFRRUArmourV82::Id(V)!=TEXT("fr.ft"))return Out;
    Out.Active=true;Out.Pending=true;Out.Reason=TEXT("FT navigation scene/build unavailable; holding");
    const auto* Found=States.Find(B);if(!Found)return Out;auto& S=**Found;
    if(S.Build.Status==EStatus::Failed){Out.Reason=S.Build.Reason;return Out;}
    const auto* G=wfexp::scene::ForMission(TCHAR_TO_UTF8(*S.Mission));const auto* D=WarfareFRRUArmourV82::Definition(V);
    if(!G||!Model(D)||S.Released||!S.Scene.IsValid()||S.Encounter!=B->GetEncounterId()){Out.Reason=TEXT("FT model/scene/encounter contract changed; holding");return Out;}
    for(auto I=S.Guides.CreateIterator();I;++I)
    {
        bool Remove=!I.Key().IsValid()||(I.Key().IsValid()&&!I.Key()->IsOperational());
#if WITH_EDITOR
        if(S.Review)Remove=false;
#endif
        if(Remove)I.RemoveCurrent();
    }
    auto& C=S.Guides.FindOrAdd(V);
    if(!C.Vehicle.IsValid()){C.Vehicle=V;C.Serial=V->Serial;C.EpisodeOrigin=V->GetActorLocation();C.LastYaw=V->GetActorRotation().Yaw;}
    const FVector Actual=V->GetActorLocation();const int32 Dir=B->GetTeamDirection(V->Team);C.Guiding=false;
    const auto Ready=Poll(B);
    // Entry and the enemy-end handoff retain the original finite apron/field-end
    // handling. No rejected navigation route can select these fallbacks.
    if(!C.Engaged&&!Grid(*G,*D,Actual,V->GetActorRotation().Yaw)&&Actual.X*Dir<(Dir>0?G->minX:-G->maxX)+FMath::Max(FMath::Abs(D->Minimum.X),FMath::Abs(D->Maximum.X))+130)
    {Out.Active=false;Out.Reason=TEXT("original entry apron; navigation not engaged");return Out;}
    C.Engaged=true;
    if(Actual.X*Dir>=Cap(*G,*D,Dir)*Dir)
    {C.HandedOff=true;Out.Active=false;Out.Pending=false;Out.Reason=TEXT("actual enemy-end safe cap crossed; original field-end handling");return Out;}
    if(!Grid(*G,*D,Actual,V->GetActorRotation().Yaw)){C.Failed=true;C.Reason=TEXT("actual FT full footprint left original complete grid");}
    if(Ready.Status!=EStatus::Success){Out.Reason=Ready.Reason;return Out;}Out.Pending=false;C.Guiding=true;
    if(C.Failed){Out.Reason=C.Reason;return Out;}
    if(B->Elapsed<C.EpisodeResumeAt){Out.Pending=true;Out.Reason=TEXT("bounded blocked-episode backoff; holding");return Out;}
    auto Requery=[&]()
    {
        if(!Target(*G,*D,Actual,Dir,C.Target)){C.Reason=TEXT("enemy-end native target unavailable");C.Failed=true;return false;}
        FQuery* Trace=nullptr;
#if WITH_EDITOR
        if(S.Review){C.Queries.AddDefaulted();Trace=&C.Queries.Last();}
#endif
        C.RetryAt=B->Elapsed+ReplanInterval;C.Next=1;C.Blocked=0;
        if(!Route(S,Actual,C.Target,C.Points,C.Reason,Trace)){C.Failed=true;return false;}return true;
    };
    if(C.Points.IsEmpty()&&!Requery()){Out.Reason=C.Reason;return Out;}
    while(C.Next<C.Points.Num()&&FVector::Dist2D(Actual,C.Points[C.Next])<=WaypointRadius)++C.Next;
    bool Leg=C.Next<C.Points.Num()&&WarfareFRRUWorldV82::CanTraverse(B,Actual,C.Points[C.Next]);
    if(!Leg||C.Blocked>=BlockedTime)
    {
        if(B->Elapsed<C.RetryAt){Out.Pending=true;Out.Reason=TEXT("bounded actual-position replan wait");return Out;}
        if(C.Attempts>=EpisodeAttempts)
        {
            C.Points.Reset();C.Attempts=0;C.Blocked=0;C.EpisodeOrigin=Actual;C.EpisodeResumeAt=B->Elapsed+EpisodeBackoff;
            C.Reason=TEXT("blocked episode exhausted; explicit hold before a fresh actual-position episode");Out.Pending=true;Out.Reason=C.Reason;return Out;
        }
        ++C.Attempts;++C.Replans;if(!Requery()){Out.Reason=C.Reason;return Out;}
        while(C.Next<C.Points.Num()&&FVector::Dist2D(Actual,C.Points[C.Next])<=WaypointRadius)++C.Next;
        Leg=C.Next<C.Points.Num()&&WarfareFRRUWorldV82::CanTraverse(B,Actual,C.Points[C.Next]);
    }
    if(!Leg){C.Failed=true;C.Reason=TEXT("no next native-connected actual-position route leg");Out.Reason=C.Reason;return Out;}
    Out.HasHeading=true;Out.Heading=(C.Points[C.Next]-Actual).Rotation().Yaw;Out.Reason=TEXT("enemy-end route heading only; original FT Plan controls admission");return Out;
}
void ObserveMovement(AWarfareFrontlineGameMode* B,AFrontlineTank* V,const FVector& Start,float ReportedTravel,bool Halt)
{
    (void)ReportedTravel;
    const auto* Found=States.Find(B);if(!Found||!V)return;auto& S=**Found;auto* C=S.Guides.Find(V);if(!C)return;
    const FVector P=V->GetActorLocation();if(P.ContainsNaN()||Start.ContainsNaN())return;
    const double Travel=FVector::Dist2D(Start,P),Yaw=FMath::Abs(FMath::FindDeltaAngleDegrees(C->LastYaw,V->GetActorRotation().Yaw));
    const double DT=C->LastHost<0?0:FMath::Clamp(double(B->Elapsed)-C->LastHost,0.,.1);C->LastHost=B->Elapsed;C->LastYaw=V->GetActorRotation().Yaw;
    if(C->Guiding&&!Halt&&!V->IsImmobilized()&&Travel<.01&&Yaw<.01)C->Blocked+=DT;else C->Blocked=0;
    if(FVector::Dist2D(C->EpisodeOrigin,P)>=Radius){C->Attempts=0;C->EpisodeOrigin=P;}
#if WITH_EDITOR
    if(S.Review)
    {
        C->ActualTravel+=Travel;if(C->Guiding)C->GuidedTravel+=Travel;S.Travel+=Travel;
        if(!C->Live)C->Live=MakeShared<FJsonObject>();
        C->Live->SetStringField(TEXT("lastActualPosition"),P.ToString());C->Live->SetNumberField(TEXT("lastActualHostElapsed"),B->Elapsed);
        C->Live->SetNumberField(TEXT("lastOriginalReportedTravelCm"),ReportedTravel);C->Live->SetBoolField(TEXT("lastWeaponHalt"),Halt);
    }
#endif
}
FResult FollowStatus(AWarfareFrontlineGameMode* B)
{
    const auto* Found=States.Find(B);if(!Found)return {EStatus::Failed,TEXT("FT navigation scene not prepared")};auto& S=**Found;
    FResult R={EStatus::Pending,TEXT("FT dispatch, entry or original guarded route remains active")};
    if(S.Build.Status==EStatus::Failed)R=S.Build;
    else if(!B||S.Released||!S.Scene.IsValid()||S.Encounter!=B->GetEncounterId())R={EStatus::Failed,TEXT("FT scene/encounter/navigation lifetime ended")};
    else if(S.Build.Status==EStatus::Success)
    {
        bool Any=false,All=true;
        for(auto& Pair:S.Guides)
        {
            auto& C=Pair.Value;const auto* V=C.Vehicle.Get();
#if WITH_EDITOR
            if(S.Review)
            {
                if(!C.Live)C.Live=MakeShared<FJsonObject>();C.Live->SetNumberField(TEXT("latestVehicleStatusHostElapsed"),B->Elapsed);C.Live->SetBoolField(TEXT("latestTrackedActorValid"),V!=nullptr);
                if(V)
                {
                    C.Live->SetStringField(TEXT("lastActualPosition"),V->GetActorLocation().ToString());C.Live->SetNumberField(TEXT("hull"),V->GetHullIntegrity());
                    C.Live->SetNumberField(TEXT("trackLeft"),V->GetTrackIntegrity(0));C.Live->SetNumberField(TEXT("trackRight"),V->GetTrackIntegrity(1));C.Live->SetBoolField(TEXT("originalIsOperational"),V->IsOperational());
                    C.Live->SetBoolField(TEXT("independentCrewLifeReadbackAvailable"),false);C.Live->SetStringField(TEXT("crewReadbackScope"),TEXT("nativeCrewOwnedAtDispatch is an upgrade; no independent FT crew life/count state"));
                    if(C.LastPlanAt<0||B->Elapsed-C.LastPlanAt>=1||!V->IsOperational()){C.LastPlan=WarfareFRRUArmourV82::GroundPilotDiagnosis(V);C.LastPlanAt=B->Elapsed;}
                }
            }
#endif
            if(!C.HandedOff&&(!V||!V->IsOperational())){R={EStatus::Failed,FString::Printf(TEXT("actual combat/vehicle loss before enemy-end cap: FT serial %d; not a Nav/Plan geometry failure"),C.Serial)};break;}
            if(C.Failed){R={EStatus::Failed,C.Reason};break;}
#if WITH_EDITOR
            if(S.Review&&B->Elapsed<C.EpisodeResumeAt){R={EStatus::Failed,C.Reason};break;}
#endif
            Any=true;All&=C.HandedOff;
        }
        if(R.Status!=EStatus::Failed&&Any&&All)R={EStatus::Success,TEXT("actual enemy-end cap crossed; original field-end handling resumed, no victory/objective grant")};
    }
#if WITH_EDITOR
    S.Follow=R;
#endif
    return R;
}
void Clear(AWarfareFrontlineGameMode* B){if(const auto* S=States.Find(B))Release(**S);States.Remove(B);}
#if WITH_EDITOR
namespace
{
TSharedRef<FJsonObject> FloorJson(const FFloor& P)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("projectedNavPoint"),P.Projected.ToString());J->SetBoolField(TEXT("navZUsedAsPhysicalFloor"),false);
    J->SetNumberField(TEXT("nativeFloorCm"),P.Native);J->SetNumberField(TEXT("navApproximationAboveNativeFloorCm"),P.Projected.Z-P.Native);J->SetNumberField(TEXT("actualGroundNativeToleranceCm"),.5);
    J->SetStringField(TEXT("rayFrom"),FVector(P.Projected.X,P.Projected.Y,P.Native+1000).ToString());J->SetStringField(TEXT("rayTo"),FVector(P.Projected.X,P.Projected.Y,P.Native-1000).ToString());
    J->SetBoolField(TEXT("nativeFootingWalkable"),P.Walkable);J->SetBoolField(TEXT("actualGroundHit"),P.GroundHit);J->SetBoolField(TEXT("actualSolidsHit"),P.SolidHit);
    if(P.GroundHit){J->SetStringField(TEXT("groundImpact"),P.Ground.ImpactPoint.ToString());J->SetStringField(TEXT("groundNormal"),P.Ground.ImpactNormal.ToString());J->SetNumberField(TEXT("groundNativeErrorCm"),FMath::Abs(P.Ground.ImpactPoint.Z-P.Native));J->SetNumberField(TEXT("groundHitTime"),P.Ground.Time);J->SetBoolField(TEXT("groundStartPenetrating"),P.Ground.bStartPenetrating);}
    if(P.SolidHit){J->SetStringField(TEXT("solidsImpact"),P.Solid.ImpactPoint.ToString());J->SetStringField(TEXT("solidsNormal"),P.Solid.ImpactNormal.ToString());J->SetNumberField(TEXT("solidsHitTime"),P.Solid.Time);J->SetBoolField(TEXT("solidsStartPenetrating"),P.Solid.bStartPenetrating);}
    J->SetBoolField(TEXT("sameNativeFloor"),P.Passed);J->SetStringField(TEXT("reason"),P.Reason);return J;
}
TSharedRef<FJsonObject> QueryJson(const FQuery& Q)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("hostElapsed"),Q.Host);J->SetStringField(TEXT("actualActorFrom"),Q.Actual.ToString());J->SetStringField(TEXT("nativeQueryFrom"),Q.Source.ToString());J->SetStringField(TEXT("nativeTarget"),Q.Target.ToString());
    J->SetBoolField(TEXT("fromProjected"),Q.FromProjected);J->SetBoolField(TEXT("toProjected"),Q.ToProjected);J->SetStringField(TEXT("projectedFrom"),Q.From.ToString());J->SetStringField(TEXT("projectedTo"),Q.To.ToString());
    J->SetNumberField(TEXT("fromProjectionXYCm"),FVector::Dist2D(Q.Source,Q.From));J->SetNumberField(TEXT("toProjectionXYCm"),FVector::Dist2D(Q.Target,Q.To));
    J->SetNumberField(TEXT("fromProjectionZCm"),Q.From.Z-Q.Source.Z);J->SetNumberField(TEXT("toProjectionZCm"),Q.To.Z-Q.Target.Z);
    J->SetObjectField(TEXT("projectedFromPhysicalFloor"),FloorJson(Q.FromFloor));J->SetObjectField(TEXT("projectedToPhysicalFloor"),FloorJson(Q.ToFloor));
    J->SetNumberField(TEXT("queryResult"),Q.Result);J->SetBoolField(TEXT("partialPath"),Q.Partial);J->SetBoolField(TEXT("queryReachedSearchLimit"),Q.Limit);J->SetNumberField(TEXT("queryMaxSearchNodes"),8192);J->SetBoolField(TEXT("completeRecastQuery"),Q.Complete);
    TArray<TSharedPtr<FJsonValue>> Points,Legs;for(const auto& P:Q.Points)Points.Add(MakeShared<FJsonValueString>(P.ToString()));for(bool L:Q.NativeLegs)Legs.Add(MakeShared<FJsonValueBoolean>(L));
    J->SetArrayField(TEXT("path"),Points);J->SetArrayField(TEXT("nativeLegsFromOriginalSource"),Legs);J->SetNumberField(TEXT("pathPoints"),Q.Count);J->SetBoolField(TEXT("pathPointsTruncated"),Q.Count>MaxPoints);
    if(Q.Points.Num()){J->SetStringField(TEXT("pathLastPoint"),Q.Points.Last().ToString());J->SetNumberField(TEXT("pathLastPointToOriginalTargetXYCm"),FVector::Dist2D(Q.Points.Last(),Q.Target));}
    J->SetBoolField(TEXT("nativeLastPointToOriginalTarget"),Q.EndpointTail);J->SetBoolField(TEXT("nativeCentreLineAllLegsClear"),Q.NativeClear);J->SetNumberField(TEXT("pathXYLengthCm"),Q.Length);J->SetBoolField(TEXT("acceptedForGuidance"),Q.Passed);J->SetStringField(TEXT("reason"),Q.Reason);return J;
}
}
FResult BeginReview(AWarfareFrontlineGameMode* B,AActor* Scene,bool QueryOnly)
{
    if(!B||!B->IsProgressionVolatile())return {EStatus::Failed,TEXT("review core requires volatile progression")};
    return BeginImpl(B,Scene,true,QueryOnly);
}
TSharedPtr<FJsonObject> ReviewSnapshot(AWarfareFrontlineGameMode* B,bool ReleaseAndClear)
{
    const auto* Found=States.Find(B);if(!Found)return nullptr;auto& S=**Found;if(!S.Review)return nullptr;
    if(ReleaseAndClear)Release(S);
    auto J=QueryJson(S.Warm);J->SetStringField(TEXT("revision"),TEXT("v87.ft.runtime.navigation.1"));J->SetStringField(TEXT("scene"),S.SceneId);
    J->SetStringField(TEXT("scope"),TEXT("Editor isolated evidence over runtime core. Transverse query, guidance and actual movement are separate; no full-route, campaign, objective or Shipping acceptance."));
    J->SetBoolField(TEXT("actorMoved"),S.Travel>.001);J->SetBoolField(TEXT("ftPhysicalTraversalAccepted"),false);J->SetBoolField(TEXT("shippingAccepted"),false);J->SetNumberField(TEXT("guidanceTransformWrites"),0);
    J->SetNumberField(TEXT("observedActualXYTravelCm"),S.Travel);J->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-S.Started);
    J->SetStringField(TEXT("status"),S.Build.Status==EStatus::Success?TEXT("success"):S.Build.Status==EStatus::Pending?TEXT("pending"):TEXT("fail"));J->SetStringField(TEXT("reason"),S.Build.Reason);
    J->SetBoolField(TEXT("sourceNavigationRestored"),S.Restored);J->SetBoolField(TEXT("ownedNavigationReleased"),S.Released);J->SetBoolField(TEXT("recastCDORadiusSlopeUnchanged"),S.CDOUnchanged);
    J->SetBoolField(TEXT("actualNavDataRegistered"),S.Registered);J->SetBoolField(TEXT("actualBoundsRegistered"),S.BoundsRegistered);J->SetStringField(TEXT("actualBounds"),S.Bounds.ToString());
    J->SetNumberField(TEXT("activeTiles"),S.Tiles);J->SetNumberField(TEXT("remainingBuildTasks"),S.Remaining);J->SetNumberField(TEXT("runningBuildTasks"),S.Running);J->SetNumberField(TEXT("unexpectedNavigationSources"),0);
    J->SetNumberField(TEXT("agentRadiusCm"),Radius);J->SetNumberField(TEXT("agentHeightCm"),Height);J->SetNumberField(TEXT("nativeMaximumDirectionalSlope"),.65);J->SetNumberField(TEXT("recastMaximumSurfaceSlopeDegrees"),FMath::RadiansToDegrees(FMath::Atan(.65f)));
    J->SetNumberField(TEXT("cellSizeCm"),Cell);J->SetNumberField(TEXT("cellHeightCm"),CellHeight);J->SetNumberField(TEXT("candidateVoxelStepHeightCm"),Step);J->SetNumberField(TEXT("tilePoolCap"),128);J->SetNumberField(TEXT("buildDeadlineWallSeconds"),Deadline);
    J->SetStringField(TEXT("goalPolicy"),S.QueryOnly?TEXT("Editor query-only native objective"):TEXT("enemy-end complete-grid safe cap; actual lane; original field-end handoff"));
    J->SetStringField(TEXT("model"),TEXT("/Game/FRRUExpansionR2/FormalV82/Vehicles/fr_ft/SK_R2_fr_ft.SK_R2_fr_ft"));
    J->SetStringField(TEXT("hullMinimumCm"),TEXT("X=-304 Y=-87.4042 Z=0"));J->SetStringField(TEXT("hullMaximumCm"),TEXT("X=195 Y=87.4042 Z=212"));J->SetStringField(TEXT("originalHullSkinCm"),TEXT("X=12 Y=12 Z=0"));
    J->SetStringField(TEXT("agentRadiusSource"),TEXT("ceil(original hull transverse half-width87.4042 + original12cm skin)=100; not full-body turn clearance"));
    J->SetStringField(TEXT("followTerminalStatus"),S.Follow.Status==EStatus::Failed?TEXT("fail"):S.Follow.Status==EStatus::Success?TEXT("actual_enemy_end_handoff"):TEXT("pending"));J->SetStringField(TEXT("followTerminalReason"),S.Follow.Reason);
    TArray<TSharedPtr<FJsonValue>> Sources,Guides;
    for(const auto& X:S.Sources)if(const auto* C=X.Component.Get()){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("component"),C->GetPathName());R->SetStringField(TEXT("mesh"),C->GetStaticMesh()?C->GetStaticMesh()->GetPathName():FString());R->SetBoolField(TEXT("navigationBefore"),X.Navigation);Sources.Add(MakeShared<FJsonValueObject>(R));}
    J->SetArrayField(TEXT("geometrySources"),Sources);
    for(const auto& Pair:S.Guides)
    {
        const auto& C=Pair.Value;auto R=C.Live?MakeShared<FJsonObject>(*C.Live):MakeShared<FJsonObject>();R->SetNumberField(TEXT("serial"),C.Serial);
        R->SetBoolField(TEXT("guidanceEngaged"),C.Engaged);R->SetBoolField(TEXT("actualEnemyEndCapCrossed"),C.HandedOff);R->SetNumberField(TEXT("routePointIndex"),C.Next);R->SetNumberField(TEXT("replans"),C.Replans);R->SetNumberField(TEXT("episodeAttempts"),C.Attempts);
        R->SetNumberField(TEXT("blockedNoPoseProgressHostSeconds"),C.Blocked);R->SetNumberField(TEXT("observedActualXYTravelCm"),C.ActualTravel);R->SetNumberField(TEXT("observedWhileGuidanceActiveXYTravelCm"),C.GuidedTravel);
        R->SetStringField(TEXT("nativeTarget"),C.Target.ToString());if(C.Next<C.Points.Num())R->SetStringField(TEXT("nextWaypoint"),C.Points[C.Next].ToString());R->SetStringField(TEXT("stopReason"),C.Reason);
        if(C.LastPlan)R->SetObjectField(TEXT("lastOriginalGroundPilotReadback"),C.LastPlan);TArray<TSharedPtr<FJsonValue>> Queries;for(const auto& Q:C.Queries)Queries.Add(MakeShared<FJsonValueObject>(QueryJson(Q)));R->SetArrayField(TEXT("routeQueries"),Queries);Guides.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("followVehicles"),Guides);if(ReleaseAndClear)States.Remove(B);return J;
}
#endif
}
