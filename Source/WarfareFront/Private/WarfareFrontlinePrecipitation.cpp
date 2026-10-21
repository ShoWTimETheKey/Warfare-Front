#include "WarfareFrontlinePrecipitation.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MaterialShared.h"
#include "MaterialShaderPrecompileMode.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"
#include "Misc/ScopeLock.h"
#include "SceneInterface.h"
#include "HAL/PlatformTime.h"

struct FFrontlinePrecipitationReadinessV42
{
    struct FObservation
    {
        bool Original=false,Valid=false,Complete=false,Finalized=false;
        int32 Shaders=0,Pipelines=0;
        bool Ready() const {return Original&&Valid&&(Shaders>0||Pipelines>0);}
        FString Describe() const {return FString::Printf(TEXT("original=%d valid=%d shaders=%d pipelines=%d complete=%d finalized=%d"),
            Original,Valid,Shaders,Pipelines,Complete,Finalized);}
    };
    FCriticalSection Mutex;
    bool InFlight=false;
    FObservation Line,Contact;
};

namespace
{
FTransform HiddenRainTransform(const FVector& Centre)
{ return FTransform(FQuat::Identity,Centre,FVector(.0001f)); }

bool PrecipitationMaterialsReadyV42(UMaterialInterface* Line,UMaterialInterface* Contact,UWorld* World,
    const TSharedRef<FFrontlinePrecipitationReadinessV42,ESPMode::ThreadSafe>& State,FString& Diagnostic)
{
    if(!Line||!Contact||!World||!World->Scene)return false;
    {
        FScopeLock Lock(&State->Mutex);
        Diagnostic=FString::Printf(TEXT("line={%s} contact={%s}"),*State->Line.Describe(),*State->Contact.Describe());
        if(State->Line.Ready()&&State->Contact.Ready())return true;
        if(State->InFlight)return false;
        State->InFlight=true;
    }
    // UPROPERTY sources retain the authored materials. Proxy destruction is
    // queued on this same render command stream after any pending observation.
    // The completion owns shared state only, never a pointer back to the actor.
    const FMaterialRenderProxy* LineProxy=Line->GetRenderProxy();
    const FMaterialRenderProxy* ContactProxy=Contact->GetRenderProxy();
    const auto FeatureLevel=World->GetFeatureLevel();
    ENQUEUE_RENDER_COMMAND(FrontlinePrecipitationObserveAuthoredMapsV42)(
        [State,LineProxy,ContactProxy,FeatureLevel](FRHICommandListImmediate&)
    {
        const auto Observe=[FeatureLevel](const FMaterialRenderProxy* Proxy)
        {
            FFrontlinePrecipitationReadinessV42::FObservation Result;
            const FMaterial* Material=Proxy?Proxy->GetMaterialNoFallback(FeatureLevel):nullptr;
            Result.Original=Material!=nullptr;
            if(Material)
            {
                Result.Complete=Material->IsRenderingThreadShaderMapComplete();
                const auto* Map=Material->GetRenderingThreadShaderMap();
                Result.Valid=Map&&Map->IsValidForRendering();
                Result.Finalized=Map&&Map->IsCompilationFinalized();
                if(Map)Map->CountNumShaders(Result.Shaders,Result.Pipelines);
            }
            return Result;
        };
        const auto ObservedLine=Observe(LineProxy),ObservedContact=Observe(ContactProxy);
        FScopeLock Lock(&State->Mutex);
        State->Line=ObservedLine;State->Contact=ObservedContact;State->InFlight=false;
    });
    return false;
}
FString PrecipitationMaterialStateV40(UMaterialInterface* Material,UWorld* World)
{
    const UMaterial* Base=Material?Material->GetMaterial():nullptr;
    const FMaterialResource* Resource=Base&&World&&World->Scene?Base->GetMaterialResource(World->Scene->GetShaderPlatform()):nullptr;
    return FString::Printf(TEXT("loaded=%d blend=%d resource=%d valid=%d complete=%d"),Material!=nullptr,
        Material?int32(Material->GetBlendMode()):-1,Resource!=nullptr,
        Resource&&Resource->HasValidGameThreadShaderMap(),Resource&&Resource->IsGameThreadShaderMapComplete());
}
}

AWarfareFrontlinePrecipitation::AWarfareFrontlinePrecipitation()
{
    PrimaryActorTick.bCanEverTick=true;
    // Readiness can settle while the preparation/menu is paused. Tick's visual
    // simulation remains paused; this only polls pending shaders at ten hertz.
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Precipitation")));
    RainLines=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainLines"));
    RainContacts=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainContacts"));
    for (UInstancedStaticMeshComponent* Component:{RainLines.Get(),RainContacts.Get()})
    {
        Component->SetupAttachment(RootComponent);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->SetCastShadow(false); Component->SetReceivesDecals(false);
        Component->SetVisibility(false); Component->SetNumCustomDataFloats(3);
    }
    Random.Initialize(191719);
}

void AWarfareFrontlinePrecipitation::BeginPlay()
{
    Super::BeginPlay();
    ReadinessStartV40=FPlatformTime::Seconds();NextReadinessCheckV40=ReadinessStartV40;
    RainPlaneV40=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    RainSourceV40=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Weather/V19/M_RainLine_V19.M_RainLine_V19"));
    ContactSourceV40=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Weather/V19/M_RainContact_V19.M_RainContact_V19"));
    if(!RainPlaneV40 || !RainSourceV40 || !ContactSourceV40 ||
        RainSourceV40->GetBlendMode()!=BLEND_Translucent || ContactSourceV40->GetBlendMode()!=BLEND_Translucent)
    {
        bInitializationFailedV40=true;SetActorTickEnabled(false);
        UE_LOG(LogTemp,Error,TEXT("WF_V40_PRECIPITATION_ASSET_FAILURE plane=%d line={%s} contact={%s}; no substitute material"),
            RainPlaneV40!=nullptr,*PrecipitationMaterialStateV40(RainSourceV40,GetWorld()),*PrecipitationMaterialStateV40(ContactSourceV40,GetWorld()));
        return;
    }
    // These hidden pools do not yet submit a mesh draw, so editor on-demand
    // compilation otherwise has no request that can populate an empty map.
    // Request the real ISM usage and cache once. This preserves material/DDC
    // identity; cooked games register their existing maps instead of compiling.
    for(UMaterialInterface* Source:{RainSourceV40.Get(),ContactSourceV40.Get()})
    {
        if(!Source->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes))
        {
            bInitializationFailedV40=true;SetActorTickEnabled(false);
            UE_LOG(LogTemp,Error,TEXT("WF_V42_PRECIPITATION_USAGE_FAILURE material=%s; no substitute material"),*Source->GetPathName());
            return;
        }
        Source->CacheShaders(EMaterialShaderPrecompileMode::Background);
    }
    InitializeVisualsV40();
}

bool AWarfareFrontlinePrecipitation::InitializeVisualsV40()
{
    if(bVisualReady)return true;
    if(bInitializationFailedV40)return false;
    const double Now=FPlatformTime::Seconds();if(Now<NextReadinessCheckV40)return false;
    NextReadinessCheckV40=Now+.10;++ReadinessAttemptsV40;
    if(!MaterialReadinessV42)MaterialReadinessV42=MakeShared<FFrontlinePrecipitationReadinessV42,ESPMode::ThreadSafe>();
    FString RenderReadiness;
    if(!PrecipitationMaterialsReadyV42(RainSourceV40,ContactSourceV40,GetWorld(),MaterialReadinessV42.ToSharedRef(),RenderReadiness))
    {
        // UE's active renderer accepts frozen, render-valid partial shader maps.
        // Whole-map compilation/finalization includes unrelated permutations;
        // it is diagnostic, not evidence that an authored rain shader is absent.
        // Poll the actual no-fallback render proxies without stalling rendering.
        if(ReadinessAttemptsV40==1)
            UE_LOG(LogTemp,Display,TEXT("WF_V40_PRECIPITATION_WAIT line={%s} contact={%s} retry_limit_seconds=30"),
                *PrecipitationMaterialStateV40(RainSourceV40,GetWorld()),*PrecipitationMaterialStateV40(ContactSourceV40,GetWorld()));
        if(Now-ReadinessStartV40>=30.)
        {
            bInitializationFailedV40=true;SetActorTickEnabled(false);
            UE_LOG(LogTemp,Error,TEXT("WF_V40_PRECIPITATION_READINESS_TIMEOUT attempts=%d %s; no substitute material"),
                ReadinessAttemptsV40,*RenderReadiness);
        }
        return false;
    }
    RainMaterial=UMaterialInstanceDynamic::Create(RainSourceV40,this);
    ContactMaterial=UMaterialInstanceDynamic::Create(ContactSourceV40,this);
    if(!RainMaterial||!ContactMaterial)
    {
        bInitializationFailedV40=true;SetActorTickEnabled(false);
        UE_LOG(LogTemp,Error,TEXT("WF_V40_PRECIPITATION_INSTANCE_FAILURE; no substitute material"));return false;
    }
    RainLines->SetStaticMesh(RainPlaneV40); RainLines->SetMaterial(0,RainMaterial);
    RainContacts->SetStaticMesh(RainPlaneV40); RainContacts->SetMaterial(0,ContactMaterial);
    Drops.SetNum(MaximumRainDrops); Contacts.SetNum(MaximumSplashes);
    RainTransforms.Init(HiddenRainTransform(GetActorLocation()),MaximumRainDrops);
    ContactTransforms.Init(HiddenRainTransform(GetActorLocation()),MaximumSplashes);
    RainCustomData.SetNumZeroed(MaximumRainDrops*3); ContactCustomData.SetNumZeroed(MaximumSplashes*3);
    RainLines->PreAllocateInstancesMemory(MaximumRainDrops); RainContacts->PreAllocateInstancesMemory(MaximumSplashes);
    RainLines->AddInstances(RainTransforms,false,true,false); RainContacts->AddInstances(ContactTransforms,false,true,false);
    RainLines->SetCustomData(0,MaximumRainDrops-1,MakeArrayView(RainCustomData),false);
    RainContacts->SetCustomData(0,MaximumSplashes-1,MakeArrayView(ContactCustomData),false);
    RainMaterial->SetScalarParameterValue(TEXT("Density"),0.f); ContactMaterial->SetScalarParameterValue(TEXT("Density"),0.f);
    bVisualReady=true;
    UE_LOG(LogTemp,Display,TEXT("WF_V40_PRECIPITATION_READY attempts=%d delay_seconds=%.3f rain=%d contact=%d %s; two world-space ISM pools, no substitute material"),
        ReadinessAttemptsV40,Now-ReadinessStartV40,MaximumRainDrops,MaximumSplashes,*RenderReadiness);
    return true;
}

void AWarfareFrontlinePrecipitation::SetWeather(float Amount,const FVector& Wind)
{
    RainAmount=FMath::IsFinite(Amount)?FMath::Clamp(Amount,0.f,1.f):0.f;
    WindVelocity=Wind.ContainsNaN()?FVector::ZeroVector:FVector(FMath::Clamp(Wind.X,-180.0,180.0),FMath::Clamp(Wind.Y,-100.0,100.0),0);
}

bool AWarfareFrontlinePrecipitation::UpdateView()
{
    APlayerController* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
    AActor* View=PC?PC->GetViewTarget():nullptr;
    if (!View) return false;
    UCameraComponent* Camera=View->FindComponentByClass<UCameraComponent>();
    FVector Location; FRotator Rotation; PC->GetPlayerViewPoint(Location,Rotation);
    if (Camera)
    {
        Location=Camera->GetComponentLocation(); Rotation=Camera->GetComponentRotation();
        HalfWidth=FMath::Clamp(Camera->ProjectionMode==ECameraProjectionMode::Orthographic?Camera->OrthoWidth*.61f:3600.f,2100.f,6100.f);
    }
    const FRotationMatrix Axes(Rotation);
    ViewForward=Axes.GetUnitAxis(EAxis::X); ViewRight=Axes.GetUnitAxis(EAxis::Y);
    // Intersect the actual view axis with the terrain. WS pans must move the
    // existing world-space pool, not leave weather on the old centreline.
    FVector Focus=Location;
    if(ViewForward.Z<-.05f)
    {
        Focus=Location+ViewForward*(-Location.Z/ViewForward.Z);
        Focus=Location+ViewForward*((AWarfareFrontlineGround::HeightAt(Focus.X,Focus.Y)-Location.Z)/ViewForward.Z);
    }
    const FVector NewCentre(Focus.X,Focus.Y,AWarfareFrontlineGround::HeightAt(Focus.X,Focus.Y));
    HalfDepthV76=Camera?FMath::Clamp(Camera->OrthoWidth/(2.f*FMath::Max(1.f,Camera->AspectRatio)*FMath::Max(.18f,FMath::Abs(float(ViewForward.Z)))),1950.f,4200.f):2450.f;
    const bool Jump=!bHasView || FMath::Abs(NewCentre.X-VisibleCentre.X)>HalfWidth*.7f||FMath::Abs(NewCentre.Y-VisibleCentre.Y)>HalfDepthV76*.7f;
    VisibleCentre=NewCentre; bHasView=true;
    if (Jump)
    {
        for (auto& Drop:Drops) Drop.bActive=false;
        for (auto& Contact:Contacts) Contact.bActive=false;
    }
    return true;
}

void AWarfareFrontlinePrecipitation::ResetDrop(FFrontlineRainDrop& Drop,bool bFillVolume)
{
    const float MinX=FMath::Max(float(VisibleCentre.X)-HalfWidth,-FrontlineBattlefield::GroundHalfLength+200.f);
    const float MaxX=FMath::Min(float(VisibleCentre.X)+HalfWidth,FrontlineBattlefield::GroundHalfLength-200.f);
    Drop.Position.X=Random.FRandRange(MinX,MaxX); Drop.Position.Y=Random.FRandRange(float(VisibleCentre.Y)-HalfDepthV76,float(VisibleCentre.Y)+HalfDepthV76);
    float SurfaceZ=AWarfareFrontlineGround::HeightAt(Drop.Position.X,Drop.Position.Y),Level=0.f,Depth=0.f;
    if(AWarfareFrontlineGround::QueryWaterSurfaceV51(Drop.Position.X,Drop.Position.Y,Level,Depth)&&Depth>1.f)SurfaceZ=FMath::Max(SurfaceZ,Level);
    Drop.Position.Z=SurfaceZ
        +Random.FRandRange(bFillVolume?80.f:1200.f,1850.f);
    Drop.Speed=Random.FRandRange(760.f,1120.f); Drop.Length=Random.FRandRange(27.f,53.f);
    Drop.Width=Random.FRandRange(2.4f,3.4f); Drop.Opacity=Random.FRandRange(.50f,.95f); Drop.bActive=true;
}

void AWarfareFrontlinePrecipitation::AddContact(const FVector& Point)
{
    if (SplashCredit<1.f || Random.FRand()>.24f) return;
    int32 Slot=INDEX_NONE;
    for (int32 I=0;I<MaximumSplashes;++I)
    {
        const int32 Candidate=(NextContact+I)%MaximumSplashes;
        if (!Contacts[Candidate].bActive) { Slot=Candidate; break; }
    }
    if (Slot==INDEX_NONE) return;
    NextContact=(Slot+1)%MaximumSplashes; SplashCredit-=1.f;
    auto& Contact=Contacts[Slot]; Contact.Position=Point;
    const float DX=(AWarfareFrontlineGround::HeightAt(Point.X+4,Point.Y)-AWarfareFrontlineGround::HeightAt(Point.X-4,Point.Y))/8.f;
    const float DY=(AWarfareFrontlineGround::HeightAt(Point.X,Point.Y+4)-AWarfareFrontlineGround::HeightAt(Point.X,Point.Y-4))/8.f;
    float WaterLevel=0.f,WaterDepth=0.f;
    const bool Water=AWarfareFrontlineGround::QueryWaterSurfaceV51(Point.X,Point.Y,WaterLevel,WaterDepth)&&WaterDepth>1.f;
    Contact.Normal=Water?FVector::UpVector:FVector(-DX,-DY,1).GetSafeNormal();
    if(Water)Contact.Position.Z=WaterLevel;
    Contact.Position+=Contact.Normal*1.1f;
    Contact.bRipple=Water;
    Contact.Age=0; Contact.Life=Random.FRandRange(.30f,.52f); Contact.Size=Random.FRandRange(10.f,19.f); Contact.bActive=true;
}

void AWarfareFrontlinePrecipitation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if ((!bVisualReady&&!InitializeVisualsV40()) || UGameplayStatics::IsGamePaused(this) || !UpdateView()) return;
    // Battle speed changes orders, not the apparent terminal speed of rain.
    const float DT=FMath::Clamp(DeltaSeconds/FMath::Max(.01f,UGameplayStatics::GetGlobalTimeDilation(this)),0.f,.05f);
    VisualRainAmount=FMath::FInterpTo(VisualRainAmount,RainAmount,DT,2.5f);
    if (VisualRainAmount<.002f && RainAmount==0.f) VisualRainAmount=0.f;
    const float Budget=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43();
    const int32 Desired=FMath::Clamp(FMath::RoundToInt(MaximumRainDrops*VisualRainAmount*Budget),0,MaximumRainDrops);
    if (Desired==0 && ActiveSplashCount==0)
    {
        if (ActiveDropCount>0) for (auto& Drop:Drops) Drop.bActive=false;
        ActiveDropCount=0; RainLines->SetVisibility(false); RainContacts->SetVisibility(false); return;
    }
    ActiveDropCount=0; ActiveSplashCount=0;
    SplashCredit=FMath::Min(4.f,SplashCredit+DT*72.f*VisualRainAmount*Budget);
    const FTransform Hidden=HiddenRainTransform(VisibleCentre);
    for (int32 I=0;I<MaximumRainDrops;++I)
    {
        auto& Drop=Drops[I]; RainCustomData[I*3]=0.f;
        if (I>=Desired) { Drop.bActive=false; RainTransforms[I]=Hidden; continue; }
        if (!Drop.bActive || FMath::Abs(Drop.Position.X-VisibleCentre.X)>HalfWidth || FMath::Abs(Drop.Position.Y-VisibleCentre.Y)>HalfDepthV76+100.f) ResetDrop(Drop,true);
        const float PreviousGround=AWarfareFrontlineGround::HeightAt(Drop.Position.X,Drop.Position.Y);
        float Shelter=1.f;
        if (Drop.Position.Z<PreviousGround+150.f)
            for (int32 TrenchIndex=0;TrenchIndex<FrontlineBattlefield::TrenchCount;++TrenchIndex)
                if (FMath::Abs(Drop.Position.X-FrontlineBattlefield::TrenchX(TrenchIndex))<145.f) { Shelter=.18f; break; }
        const FVector Velocity(WindVelocity.X*Shelter,WindVelocity.Y*Shelter,-Drop.Speed);
        Drop.Position+=Velocity*DT;
        float GroundZ=AWarfareFrontlineGround::HeightAt(Drop.Position.X,Drop.Position.Y),Level=0.f,Depth=0.f;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(Drop.Position.X,Drop.Position.Y,Level,Depth)&&Depth>1.f)GroundZ=FMath::Max(GroundZ,Level);
        if (Drop.Position.Z<=GroundZ+1.f)
        {
            AddContact(FVector(Drop.Position.X,Drop.Position.Y,GroundZ)); ResetDrop(Drop,false);
        }
        const FVector Up=-Velocity.GetSafeNormal();
        FVector Right=FVector::CrossProduct(Up,ViewForward).GetSafeNormal(); if (Right.IsNearlyZero()) Right=ViewRight;
        const FQuat Facing=FRotationMatrix::MakeFromXY(Right,Up).ToQuat();
        RainTransforms[I]=FTransform(Facing,Drop.Position+Up*(Drop.Length*.5f),FVector(Drop.Width/100.f,Drop.Length/100.f,1.f));
        const float Edge=FMath::Clamp((HalfWidth-FMath::Abs(float(Drop.Position.X-VisibleCentre.X)))/420.f,0.f,1.f);
        const float DistanceFade=FMath::Lerp(.58f,1.f,FMath::Clamp((float(Drop.Position.Y-VisibleCentre.Y)+HalfDepthV76)/(2.f*HalfDepthV76),0.f,1.f));
        RainCustomData[I*3]=Drop.Opacity*Edge*DistanceFade; RainCustomData[I*3+1]=0.f; RainCustomData[I*3+2]=0.f;
        if (RainCustomData[I*3]>.01f) ++ActiveDropCount;
    }
    for (int32 I=0;I<MaximumSplashes;++I)
    {
        auto& Contact=Contacts[I]; ContactCustomData[I*3]=0.f;
        if (Contact.bActive) Contact.Age+=DT;
        if (!Contact.bActive || Contact.Age>=Contact.Life || FMath::Abs(Contact.Position.X-VisibleCentre.X)>HalfWidth||FMath::Abs(Contact.Position.Y-VisibleCentre.Y)>HalfDepthV76+100.f)
        { Contact.bActive=false; ContactTransforms[I]=Hidden; continue; }
        const float T=Contact.Age/Contact.Life;
        FVector Position=Contact.Position;
        FQuat Rotation;
        FVector Scale;
        if (Contact.bRipple)
        {
            Rotation=FRotationMatrix::MakeFromZ(Contact.Normal).ToQuat();
            Scale=FVector(Contact.Size/100.f,Contact.Size/100.f,1.f);
        }
        else
        {
            Rotation=FRotationMatrix::MakeFromXY(ViewRight,FVector::UpVector).ToQuat();
            Position.Z+=2.5f+8.f*(4.f*T*(1.f-T));
            Scale=FVector(Contact.Size*.66f/100.f,Contact.Size*.60f/100.f,1.f);
        }
        ContactTransforms[I]=FTransform(Rotation,Position,Scale);
        ContactCustomData[I*3]=.75f; ContactCustomData[I*3+1]=T; ContactCustomData[I*3+2]=Contact.bRipple?0.f:1.f;
        ++ActiveSplashCount;
    }
    RainMaterial->SetScalarParameterValue(TEXT("Density"),.30f+.25f*VisualRainAmount);
    ContactMaterial->SetScalarParameterValue(TEXT("Density"),.32f+.16f*VisualRainAmount);
    RainLines->SetVisibility(ActiveDropCount>0); RainContacts->SetVisibility(ActiveSplashCount>0);
    // UE's custom-data end index is inclusive. Both buffers upload in batches;
    // transform calls mark their component once, never once per individual drop.
    RainLines->SetCustomData(0,MaximumRainDrops-1,MakeArrayView(RainCustomData),false);
    RainContacts->SetCustomData(0,MaximumSplashes-1,MakeArrayView(ContactCustomData),false);
    RainLines->BatchUpdateInstancesTransforms(0,RainTransforms,true,true,true);
    RainContacts->BatchUpdateInstancesTransforms(0,ContactTransforms,true,true,true);
}
