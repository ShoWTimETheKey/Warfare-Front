#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineBlastCompositionV8221.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineFlameVolumeV836.h"
#include "WarfareFrontlineArmourSurfaceV837.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineAudio.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "CoreGlobals.h"

namespace
{
#include "WarfareFrontlineWreckOpeningsV8311.generated.inl"
FVector WreckFlameCentreV8311(EFrontlineVehicle Type,int32 Site,const FTransform& Pose,
    UStaticMeshComponent* Hull,const FVector& Authored,const FVector& Dimensions,FVector& Opening,bool& Measured)
{
    Measured=false;Opening=Authored;
    auto* M=FrontlineFlameVolumeV836::WreckMaterialV8311();
    if(M&&M->GetBaseMaterial()->GetFName()==TEXT("M_WreckCombustion_Turbulent_V8311"))
    {
        const int32 Key=Type==EFrontlineVehicle::MarkIVPioneer?int32(EFrontlineVehicle::MarkIVMale):
            Type==EFrontlineVehicle::BeutePioneer?int32(EFrontlineVehicle::BeuteMarkIV):int32(Type);
        const FString Actual=Hull&&Hull->GetStaticMesh()?Hull->GetStaticMesh()->GetName():FString();
        for(const auto& R:WreckOpeningsV8311)if(R.Model==Key&&R.Site==Site&&(Actual.IsEmpty()||Actual==R.MeshName))
        {
            Opening=Pose.TransformPosition(R.LocalBottom);Measured=true;
            return Opening+FVector(0,0,Dimensions.Z*.5f);
        }
    }
    return Authored+FVector(0,0,Dimensions.Z*(84.f/245.f));
}
int32 FireEmissionCountV8214(FRandomStream& Random,float DT,float Rate)
{
    const float Count=FMath::Clamp(DT,0.f,.05f)*FMath::Max(0.f,Rate);
    return FMath::FloorToInt(Count)+(Random.FRand()<FMath::Frac(Count)?1:0);
}
FVector FireDriftV8214(float Time)
{
    // A coherent slow breeze carries every source; per-particle curl adds detail.
    return FVector(18.f+7.f*FMath::Sin(Time*.41f),-9.f+5.f*FMath::Sin(Time*.29f),0);
}
float FireGrowthV8214(const FFrontlineFuelParticleV33& P,float T)
{
    if(P.Smoke||P.Layer==0)return 1.f+T*P.Growth;
    if(P.Layer==3)return FMath::Lerp(1.f,.32f,T);
    return 1.f+P.Growth*FMath::SmoothStep(0.f,.78f,T);
}
}

AFrontlineFuelFireV33::AFrontlineFuelFireV33()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PostPhysics;
    auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("FuelFXRoot"));SetRootComponent(Root);
    Flames=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AdvectedFuelBatch"));Flames->SetupAttachment(Root);
    Smoke=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FuelSootBatch"));Smoke->SetupAttachment(Root);
    for(auto* C:{Flames.Get(),Smoke.Get()})
    {C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);C->SetReceivesDecals(false);C->SetVisibleInRayTracing(false);C->SetAffectDynamicIndirectLighting(false);C->NumCustomDataFloats=6;C->SetMobility(EComponentMobility::Movable);C->SetCullDistances(0,18000);}
    for(int32 I=0;I<LightBudget;++I){Lights[I]=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("FuelLight%d"),I));Lights[I]->SetupAttachment(Root);Lights[I]->SetIntensity(0);Lights[I]->SetAttenuationRadius(650);Lights[I]->SetLightColor(FLinearColor(1,.32,.055));Lights[I]->SetCastShadows(false);Lights[I]->SetVolumetricScatteringIntensity(0.f);}
    for(int32 I=0;I<WreckLightBudgetV839;++I)
    {
        auto* L=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("WreckSourceLightV839_%d"),I));
        L->SetupAttachment(Root);L->SetMobility(EComponentMobility::Movable);L->SetIntensity(0);
        L->SetVisibility(false);L->SetAttenuationRadius(650);L->SetLightColor(FLinearColor(1,.32,.055));
        L->SetCastShadows(false);L->SetVolumetricScatteringIntensity(0.f);L->SetIndirectLightingIntensity(0.f);
        WreckLightsV839[I]=L;
    }
}
AFrontlineFuelFireV33* AFrontlineFuelFireV33::Get(UWorld* W)
{if(!W)return nullptr;for(TActorIterator<AFrontlineFuelFireV33> I(W);I;++I)if(IsValid(*I))return *I;return W->SpawnActor<AFrontlineFuelFireV33>();}
void AFrontlineFuelFireV33::BeginPlay()
{
    AFrontlineArmourSurfaceV837::Get(GetWorld());
    Super::BeginPlay();auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    Flames->SetStaticMesh(Plane);Smoke->SetStaticMesh(Plane);
    Flames->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8214/Materials/M_FuelFlame_V8214.M_FuelFlame_V8214")));
    Smoke->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8214/Materials/M_FuelSoot_V8214.M_FuelSoot_V8214")));
    Particles.SetNum(FlameBudget+SmokeBudget);
    for(int32 I=0;I<Particles.Num();++I)Particles[I].Smoke=I>=FlameBudget;
    const FTransform Hidden(FQuat::Identity,FVector(0,0,-10000),FVector::ZeroVector);
    for(int32 I=0;I<FlameBudget;++I)Flames->AddInstance(Hidden,true);
    for(int32 I=0;I<SmokeBudget;++I)Smoke->AddInstance(Hidden,true);
    // Six rounded local-density proxies share the accepted 64^3 evolving
    // atlas. Their depth and internal extinction remain stable during a pan;
    // individual flame ribbons do not have to fake the opaque smoke column.
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Volume=FrontlineSmokeVolumeV35::MaterialV40();
    if(Cube&&Volume)for(int32 I=0;I<VolumeBudgetV8216;++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("FuelSootVolume%d"),I));
        C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);C->SetCastShadow(false);C->SetVisibility(false);
        auto* M=UMaterialInstanceDynamic::Create(Volume,this);M->SetScalarParameterValue(TEXT("VolumeProfile"),0.f);
        M->SetScalarParameterValue(TEXT("Seed"),float(I)*2.399963f);M->SetScalarParameterValue(TEXT("Density"),0.f);
        M->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.025f,.025f,.023f));
        C->SetMaterial(0,M);C->RegisterComponent();VolumeSootV8216.Add(C);VolumeMaterialsV8216.Add(M);
        VolumeStateV8216.AddDefaulted();
    }
    // Wreck-only reuse of the accepted density atlas. Dedicated MIDs leave
    // explosions, gas, steam and the legacy fire review's six proxies intact.
    auto* WreckVolume=FrontlineWreckSmokeV838::Material();
    if(Cube&&WreckVolume)for(int32 I=0;I<WreckSmokeVolumeBudgetV838;++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("WreckSootVolumeV838_%d"),I));
        C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube);C->SetMobility(EComponentMobility::Movable);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
        C->SetReceivesDecals(false);C->SetCastShadow(false);C->SetVisibility(false);C->SetCullDistance(0.f);
        C->SetVisibleInRayTracing(false);C->SetAffectDistanceFieldLighting(false);C->SetAffectDynamicIndirectLighting(false);
        C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;
        auto* M=UMaterialInstanceDynamic::Create(WreckVolume,this);M->SetScalarParameterValue(TEXT("VolumeProfile"),3.f);
        M->SetScalarParameterValue(TEXT("Density"),0.f);M->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.026f,.026f,.0252f));
        C->SetMaterial(0,M);C->RegisterComponent();WreckSootV838.Add(C);WreckSootMaterialsV838.Add(M);
    }
    WreckSootStateV838.SetNum(WreckSmokeSourceBudgetV838);
    if(auto* Base=FrontlineFlameVolumeV836::Material())for(int32 I=0;I<ContactBeginV8310+ContactBudgetV836;++I)
    {
        const bool Wreck=I>=FlameCoreBudgetV836&&I<ContactBeginV8310;
        auto* C=FrontlineFlameVolumeV836::Create(this,*FString::Printf(TEXT("TrueFuelCoreV836_%d"),I),Wreck);
        if(!C)continue;
        auto* M=UMaterialInstanceDynamic::Create(Wreck?FrontlineFlameVolumeV836::WreckMaterialV8311():Base,this);C->SetMaterial(0,M);
        FlameCoresV836.Add(C);FlameMaterialsV836.Add(M);FlameStatesV836.AddDefaulted();
    }
    if(!IsReady())UE_LOG(LogTemp,Error,TEXT("WF_V33_FIRE_ASSET_FAILURE: V8214 fuel derivatives and original atlases required"));
}
bool AFrontlineFuelFireV33::IsReady()const{return Flames&&Smoke&&Flames->GetMaterial(0)&&Smoke->GetMaterial(0)&&Flames->GetStaticMesh()
    &&FlameStatesV836.Num()==ContactBeginV8310+ContactBudgetV836&&WreckSootV838.Num()==WreckSmokeVolumeBudgetV838;}
void AFrontlineFuelFireV33::Clear()
{FrontlineBlastCompositionV8221::ClearFuelBursts(this);for(auto& P:Particles)P.Age=100;for(auto& V:VolumeStateV8216)V.Age=100;for(const auto& C:VolumeSootV8216)if(C)C->SetVisibility(false);for(auto& V:WreckSootStateV838)V=FFrontlineWreckSootV838{};for(const auto& C:WreckSootV838)if(C)C->SetVisibility(false);for(auto& V:FlameStatesV836)V=FFrontlineFlameCoreV836{};WreckFlameFeedsV8310=0;WreckFlameDropsV8310=0;WreckFlameFallbacksV8310=0;WreckFireTickSequenceV8310=0;WreckFeedSequenceV8310=0;for(const auto& C:FlameCoresV836)if(C)C->SetVisibility(false);bHasParticles=true;for(int32 I=0;I<LightBudget;++I){LightAge[I]=10;LightEnergy[I]=0;Lights[I]->SetIntensity(0);}for(int32 I=0;I<WreckLightBudgetV839;++I){WreckLightStateV839[I]=FFrontlineWreckLightV839{};WreckLightsV839[I]->SetIntensity(0);WreckLightsV839[I]->SetVisibility(false);}Tick(.001f);}
void AFrontlineFuelFireV33::FeedFlameCoreV836(const FVector& Source,const FVector& Size,float Strength,float Seed,
    USkeletalMeshComponent* Mesh,FName Bone)
{
    if(Source.ContainsNaN()||Size.ContainsNaN()||Strength<=.03f||FlameStatesV836.Num()!=ContactBeginV8310+ContactBudgetV836)return;
    int32 Slot=INDEX_NONE,Old=0;float Oldest=-1.f,Nearest=FMath::Square(90.f);
    for(int32 I=0;I<FlameCoreBudgetV836;++I)
    {
        const auto& V=FlameStatesV836[I];
        const bool SameAttached=Mesh&&V.AttachedMesh.Get()==Mesh&&V.Bone==Bone;
        const float Gap=FVector::DistSquared(Source,V.Source);
        if(V.Age<.4f&&(SameAttached||(!Mesh&&!V.AttachedMesh.IsValid()&&Gap<Nearest)))
        {Slot=I;Nearest=Gap;if(SameAttached)break;}
        if(V.Age>Oldest){Oldest=V.Age;Old=I;}
    }
    if(Slot==INDEX_NONE)Slot=Old;
    auto& V=FlameStatesV836[Slot];V.Source=Source;V.Dimensions=Size;V.Strength=FMath::Clamp(Strength,.03f,2.2f);
    V.Seed=Seed;V.Age=0;V.AttachedMesh=Mesh;V.Bone=Bone;
    if(Mesh)V.LocalOffset=Mesh->GetSocketTransform(Bone,RTS_World).InverseTransformPosition(Source);
    bHasParticles=true;SetActorTickEnabled(true);
}
bool AFrontlineFuelFireV33::FeedWreckFlameV8310(const FVector& Source,const FVector& Size,float Strength,float Seed,
    UStaticMeshComponent* Hull,EFrontlineVehicle Type,int32 Site,bool bPrimary,const FVector& Opening,bool MeasuredOpening)
{
    if(Source.ContainsNaN()||Size.ContainsNaN()||!FMath::IsFinite(Strength)||!FMath::IsFinite(Seed)
        ||Size.GetMin()<2.f||Size.GetMax()>2000.f||Site<0)return false;
    // An expired source needs no fallback sprite when its remaining fuel is
    // below the volume's visibility floor.
    if(Strength<=.03f)return true;
    ++WreckFlameFeedsV8310;
    if(FlameStatesV836.Num()!=ContactBeginV8310+ContactBudgetV836)
    {++WreckFlameFallbacksV8310;return true;}
    int32 Slot=INDEX_NONE,Vacant=INDEX_NONE,Retire=INDEX_NONE;bool PendingRetirement=false;
    for(int32 I=FlameCoreBudgetV836;I<ContactBeginV8310;++I)
    {
        const auto& V=FlameStatesV836[I];
        PendingRetirement|=V.bWreckRetiringV8310&&V.Age<.4f;
        const bool Same=V.WreckHullV8310.Get()==Hull&&V.WreckModelV8310==int32(Type)&&V.WreckSiteV8310==Site
            &&(Hull||V.Source.Equals(Source,.1f));
        if(Same)
        {
            // A yielding secondary is allowed to finish its existing smooth
            // 0.4 s envelope. Its old owner cannot reset that retirement.
            if(V.bWreckRetiringV8310)return true;
            Slot=I;break;
        }
        // A retired secondary's vacant slot is reserved for a primary. The
        // old secondary's earlier actor tick must not reclaim it first.
        if(Vacant==INDEX_NONE&&V.Age>=.4f&&(!V.bWreckRetiringV8310||bPrimary))Vacant=I;
        if(Retire==INDEX_NONE&&V.Age<.4f&&!V.bWreckPrimaryV8310&&!V.bWreckRetiringV8310)Retire=I;
    }
    if(Slot==INDEX_NONE)Slot=Vacant;
    // The primary fuel opening outranks an auxiliary tongue. Retire that
    // secondary smoothly, without an immediate position/phase reassignment.
    // Saturation never reinstates the old low-resolution silhouette.
    if(Slot==INDEX_NONE)
    {
        if(bPrimary&&Retire!=INDEX_NONE&&!PendingRetirement)FlameStatesV836[Retire].bWreckRetiringV8310=true;
        ++WreckFlameDropsV8310;return true;
    }
    auto& V=FlameStatesV836[Slot];V.Source=Source;V.Dimensions=Size;
    V.Strength=FMath::Clamp(Strength,.03f,2.2f);V.Seed=Seed;V.Age=0;
    V.AttachedMesh.Reset();V.Bone=NAME_None;V.WreckHullV8310=Hull;
    V.WreckModelV8310=int32(Type);V.WreckSiteV8310=Site;
    V.bWreckPrimaryV8310=bPrimary;V.bWreckRetiringV8310=false;
    V.bMeasuredOpeningV8311=MeasuredOpening;V.OpeningV8311=Opening;
    if(Hull)V.LocalOffset=Hull->GetComponentTransform().InverseTransformPosition(Source);
    if(Hull)V.LocalOpeningV8311=Hull->GetComponentTransform().InverseTransformPosition(Opening);
    V.FedFrameV8310=GFrameCounter;V.FedFireTickV8310=WreckFireTickSequenceV8310;
    V.FeedSequenceV8310=++WreckFeedSequenceV8310;
    FVector Eye;FRotator View;
    if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetPlayerViewPoint(Eye,View);
    else View=FRotator(-16,90,0);
    // A source feed is also its presentation commit. Fleet and actor tick
    // ordering cannot leave a newly refreshed wreck invisible for this frame.
    PresentWreckFlameV8310(Slot,View.Vector());
    bHasParticles=true;SetActorTickEnabled(true);return true;
}
void AFrontlineFuelFireV33::PresentWreckFlameV8310(int32 Slot,const FVector& Forward)
{
    if(Slot<FlameCoreBudgetV836||Slot>=ContactBeginV8310||!FlameStatesV836.IsValidIndex(Slot)
        ||!FlameCoresV836.IsValidIndex(Slot)||!FlameMaterialsV836.IsValidIndex(Slot))return;
    auto& V=FlameStatesV836[Slot];auto* C=FlameCoresV836[Slot].Get();auto* M=FlameMaterialsV836[Slot].Get();
    if(!C||!M)return;
    if(V.Age>=.4f){C->SetVisibility(false);return;}
    if(auto* Hull=V.WreckHullV8310.Get())
    {V.Source=Hull->GetComponentTransform().TransformPosition(V.LocalOffset);V.OpeningV8311=Hull->GetComponentTransform().TransformPosition(V.LocalOpeningV8311);}
    const float Fade=1.f-FMath::SmoothStep(.10f,.40f,V.Age);
    FrontlineFlameVolumeV836::Update(C,M,V.Source,V.Dimensions,Forward,V.Strength*Fade,V.Seed,GetWorld()->TimeSeconds,V.Age,false);
    V.PresentedFrameV8310=GFrameCounter;V.PresentedFeedSequenceV8310=V.FeedSequenceV8310;
}
void AFrontlineFuelFireV33::EmitArmourContactV836(const FVector& Position,const FVector& Normal,float Strength,bool bExplosive,EFrontlineArmourContactSoundV839 SoundKind)
{
    if(Position.ContainsNaN()||Normal.ContainsNaN()||!FMath::IsFinite(Strength)||Strength<=0)return;
    if(auto* Surface=AFrontlineArmourSurfaceV837::Get(GetWorld()))Surface->EmitContact(Position,Normal,Strength,bExplosive);
    if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayArmourContactV839(Position,Strength,
        FrontlineArmourAudioV839::Resolve(SoundKind,bExplosive));
    if(FlameStatesV836.Num()==ContactBeginV8310+ContactBudgetV836)
    {
        int32 Slot=ContactBeginV8310;
        for(int32 I=Slot+1;I<FlameStatesV836.Num();++I)if(FlameStatesV836[I].Age>FlameStatesV836[Slot].Age)Slot=I;
        auto& V=FlameStatesV836[Slot];const float Size=FMath::Lerp(bExplosive?30.f:16.f,bExplosive?48.f:24.f,FMath::Clamp(Strength,0.f,1.f));
        V.Source=Position+Normal.GetSafeNormal()*10.f;V.Dimensions=FVector(Size,Size,Size);
        V.Strength=FMath::Clamp(Strength*(bExplosive?1.f:.50f),.18f,1.4f);V.Seed=Random.FRandRange(0,14);V.Age=0;V.AttachedMesh.Reset();
        bHasParticles=true;SetActorTickEnabled(true);
    }
    const FVector N=Normal.GetSafeNormal(.001f,FVector::UpVector);
    for(int32 I=0;I<4;++I)if(auto* P=Add(Position+N*2.f,N*Random.FRandRange(170,340)+Random.VRand()*95,
        Random.FRandRange(.065f,.14f),Random.FRandRange(.9f,1.8f),1.f,false))
    {P->Layer=3;P->Growth=0;P->Heat=.7f;P->Buoyancy=-280;P->Drag=.7f;P->CurlRate=0;}
}
void AFrontlineFuelFireV33::TickFlameCoresV836(float DT,const FVector& Forward)
{
    const float Clock=GetWorld()->TimeSeconds;
    const uint64 TickSequence=WreckFireTickSequenceV8310++;
    for(int32 I=0;I<FlameStatesV836.Num();++I)
    {
        auto& V=FlameStatesV836[I];const bool Contact=I>=ContactBeginV8310;
        const bool Wreck=I>=FlameCoreBudgetV836&&!Contact;
        // A just-fed source belongs to this frame, not to the previous frame's
        // complete delta. The tick serial also handles strict probes which
        // call real actor Tick repeatedly without advancing GFrameCounter.
        const bool Fresh=Wreck&&!V.bWreckRetiringV8310&&V.FedFrameV8310==GFrameCounter&&V.FedFireTickV8310==TickSequence;
        if(!Fresh)V.Age+=DT;
        const float Life=Contact?.15f:.40f;
        if(V.Age>=Life){FlameCoresV836[I]->SetVisibility(false);continue;}
        if(Wreck)
        {
            if(!(Fresh&&V.PresentedFrameV8310==GFrameCounter&&V.PresentedFeedSequenceV8310==V.FeedSequenceV8310))PresentWreckFlameV8310(I,Forward);
            continue;
        }
        if(auto* Mesh=V.AttachedMesh.Get())V.Source=Mesh->GetSocketTransform(V.Bone,RTS_World).TransformPosition(V.LocalOffset);
        const float Fade=Contact?1.f:1.f-FMath::SmoothStep(.10f,.40f,V.Age);
        FrontlineFlameVolumeV836::Update(FlameCoresV836[I],FlameMaterialsV836[I],V.Source,V.Dimensions,
            Forward,V.Strength*Fade,V.Seed,Clock,V.Age,Contact);
    }
}
void AFrontlineFuelFireV33::FeedSootVolumeV8216(const FVector& Source,float Strength,float Heat,float SourcePhase)
{
    if(Strength<=.03f||Source.ContainsNaN()||VolumeStateV8216.IsEmpty())return;
    int32 Slot=INDEX_NONE;float Nearest=FMath::Square(160.f),Oldest=-1;int32 Old=0;
    for(int32 I=0;I<VolumeStateV8216.Num();++I)
    {
        auto& V=VolumeStateV8216[I];const float D=FVector::DistSquared(V.Source,Source);
        if(V.Age<2.f&&D<Nearest){Nearest=D;Slot=I;}
        if(V.Age>Oldest){Oldest=V.Age;Old=I;}
    }
    if(Slot==INDEX_NONE)Slot=Old;
    auto& V=VolumeStateV8216[Slot];V.Source=Source;V.Age=0;V.SourcePhase=SourcePhase;
    V.Strength=FMath::Clamp(Strength,.03f,1.7f);V.Heat=FMath::Clamp(Heat,0.f,1.2f);
    bHasParticles=true;SetActorTickEnabled(true);
}
void AFrontlineFuelFireV33::TickSootVolumesV8216(float DT,const FVector& ViewForward)
{
    const float Time=GetWorld()->TimeSeconds;
    for(int32 I=0;I<VolumeStateV8216.Num();++I)
    {
        auto& V=VolumeStateV8216[I];V.Age+=DT;
        if(V.Age>=2.f){VolumeSootV8216[I]->SetVisibility(false);continue;}
        const float Fade=1-FMath::SmoothStep(.18f,2.f,V.Age);
        const float Roll=.91f+.09f*FMath::Sin(Time*1.4f+V.SourcePhase);
        const float Scale=.76f+.24f*FMath::Min(V.Strength,1.4f);
        const FVector Dimensions(340.f*Scale,310.f*Scale,530.f*Scale);
        const FVector Centre=V.Source+FireDriftV8214(Time)*(2.6f+V.Age)+FVector(0,0,220.f+V.Age*95.f);
        auto* M=VolumeMaterialsV8216[I].Get();
        M->SetScalarParameterValue(TEXT("FlashHeat"),V.Heat*.18f*Fade);
        // Slow advection changes the field phase without restarting the atlas
        // every emission. Two-dimensional companion wisps remain very faint.
        FrontlineSmokeVolumeV35::Update(VolumeSootV8216[I],M,Centre,Dimensions,ViewForward,V.Strength*Fade*Roll*.84f,
            FMath::Fmod(Time*.36f+V.SourcePhase,13.f));
    }
}
void AFrontlineFuelFireV33::FeedWreckSootV838(const FVector& Source,const FFrontlineWreckSmokeSiteV838& Site,const FString& Profile,
    EFrontlineVehicle Type,int32 SiteIndex,float Strength,float Heat,float Phase,UStaticMeshComponent* Hull)
{
    if(Source.ContainsNaN()||!FMath::IsFinite(Strength)||!FMath::IsFinite(Heat)||!FMath::IsFinite(Phase)
        ||Strength<=.018f||WreckSootV838.Num()!=WreckSmokeVolumeBudgetV838)return;
    int32 Slot=INDEX_NONE,OldestSlot=0;float Oldest=-1.f;
    for(int32 I=0;I<WreckSootStateV838.Num();++I)
    {
        const auto& S=WreckSootStateV838[I];
        const bool SameOwner=Hull?S.Hull.Get()==Hull:!S.Hull.IsValid()&&FMath::IsNearlyEqual(S.Phase,Phase,.001f)
            &&FVector::DistSquared(S.Source,Source)<FMath::Square(80.f);
        if(S.FeedAge<FrontlineWreckSmokeV838::TailSeconds&&S.Model==int32(Type)&&S.Site==SiteIndex&&SameOwner){Slot=I;break;}
        if(S.FeedAge>Oldest){Oldest=S.FeedAge;OldestSlot=I;}
    }
    if(Slot==INDEX_NONE)
    {
        Slot=OldestSlot;if(WreckSootStateV838[Slot].FeedAge<FrontlineWreckSmokeV838::TailSeconds)++WreckSmokeEvictionsV838;
        WreckSootStateV838[Slot]=FFrontlineWreckSootV838{};
    }
    auto& S=WreckSootStateV838[Slot];S.Source=Source;S.WidthCm=Site.WidthCm;S.Phase=Phase;S.FeedAge=0;
    S.Strength=FMath::Clamp(Strength,0.f,1.55f);S.Heat=FMath::Clamp(Heat,0.f,2.2f);
    S.Hull=Hull;S.Site=SiteIndex;S.Model=int32(Type);S.Profile=Profile;S.SiteName=Site.Name;
    ++WreckSmokeFeedsV838;bHasParticles=true;SetActorTickEnabled(true);
}
void AFrontlineFuelFireV33::TickWreckSootV838(float DT,const FVector& ViewForward)
{
    if(WreckSootV838.Num()!=WreckSmokeVolumeBudgetV838)return;
    for(int32 I=0;I<WreckSootStateV838.Num();++I)
    {
        auto& S=WreckSootStateV838[I];if(S.FeedAge>=FrontlineWreckSmokeV838::TailSeconds)continue;
        S.FeedAge+=DT;S.FlowAge+=DT;
        for(int32 Layer=0;Layer<FrontlineWreckSmokeV838::LayersPerSource;++Layer)
        {
            const int32 Slot=I*FrontlineWreckSmokeV838::LayersPerSource+Layer;
            auto* C=WreckSootV838[Slot].Get();auto* M=WreckSootMaterialsV838[Slot].Get();
            if(S.FeedAge>=FrontlineWreckSmokeV838::TailSeconds){C->SetVisibility(false);M->SetScalarParameterValue(TEXT("Density"),0);continue;}
            const auto Shape=FrontlineWreckSmokeV838::Layer(S.Source,S.WidthCm,S.Strength,S.Heat,S.Phase,
                S.FlowAge,S.FeedAge,GetWorld()->TimeSeconds,Layer);
            M->SetScalarParameterValue(TEXT("Seed"),S.Phase+Layer*1.87f);
            M->SetVectorParameterValue(TEXT("DustColor"),Shape.Colour);
            FrontlineSmokeVolumeV35::Update(C,M,Shape.Centre,Shape.Dimensions,ViewForward,Shape.Density,Shape.FieldAge);
            // Private wreck shader supports 12–32 steps. Shared smoke retains
            // its old tier cap; increased work must pass the actual cost run.
            const float Budget=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43();
            M->SetScalarParameterValue(TEXT("QualitySteps"),Budget>=.95f?24.f:Budget>=.5f?16.f:12.f);
        }
    }
}
FFrontlineFuelParticleV33* AFrontlineFuelFireV33::Add(const FVector& P,const FVector& V,float Life,float Size,float Stretch,bool S,float Turn)
{
    if(Particles.IsEmpty())return nullptr;bHasParticles=true;SetActorTickEnabled(true);
    const int32 Start=S?FlameBudget:0,Budget=S?SmokeBudget:FlameBudget;
    int32& Cursor=S?SmokeCursor:FlameCursor;const int32 SearchStart=Cursor++%Budget;int32 Slot=Start+SearchStart;float Oldest=-1;
    // Use empty slots before replacing the least visible old tail. Clothing,
    // wrecks and embers share the original hard cap rather than adding actors.
    for(int32 Offset=0;Offset<Budget;++Offset)
    {
        const int32 Candidate=Start+((SearchStart+Offset)%Budget);const auto& Q=Particles[Candidate];
        if(Q.Age>=Q.Life){Slot=Candidate;break;}
        const float Score=Q.Age/FMath::Max(.01f,Q.Life)+(Q.Layer==3?.3f:0.f);
        if(Score>Oldest){Oldest=Score;Slot=Candidate;}
    }
    auto& F=Particles[Slot];const bool WasVisible=F.Visible;F=FFrontlineFuelParticleV33();F.Visible=WasVisible;
    F.P=P;F.V=V;F.Age=0;F.Life=Life;F.Size=Size;F.Stretch=Stretch;F.Smoke=S;F.Seed=Random.FRandRange(0,16);F.Turn=Turn;
    if(S){F.Growth=1.5f;F.Buoyancy=45;F.Drag=.42f;}
    return &F;
}
void AFrontlineFuelFireV33::Illuminate(const FVector& P,float Strength)
{
    SetActorTickEnabled(true);int32 Best=INDEX_NONE;float Weakest=TNumericLimits<float>::Max();int32 WeakestIndex=0;
    for(int32 I=0;I<LightBudget;++I)
    {
        if(FVector::DistSquared(P,Lights[I]->GetComponentLocation())<FMath::Square(350.f)){Best=I;break;}
        if(LightAge[I]>.22f)Best=I;
        if(LightEnergy[I]<Weakest){Weakest=LightEnergy[I];WeakestIndex=I;}
    }
    if(Best==INDEX_NONE){if(Strength<Weakest*.85f)return;Best=WeakestIndex;}
    Lights[Best]->SetWorldLocation(P);LightEnergy[Best]=FMath::Max(LightEnergy[Best],Strength);LightAge[Best]=0;
}
void AFrontlineFuelFireV33::EmitJet(const FVector& From,const FVector& Direction,float DT,float Seed)
{
    // Fractional emission uses a stable low-discrepancy phase, avoiding frame-
    // rate multiplication. Existing particles remain in world space on aim changes.
    const float Count=FMath::Clamp(DT,0.f,.05f)*82.f;int32 N=FMath::FloorToInt(Count);if(Random.FRand()<Count-N)++N;
    const FVector Dir=Direction.GetSafeNormal();const FVector Side=FVector::CrossProduct(Dir,FVector::UpVector).GetSafeNormal();
    for(int32 I=0;I<N;++I)
    {const float Back=Random.FRand()*FMath::Min(DT,.05f);const FVector V=Dir*Random.FRandRange(1350,1650)+Side*Random.FRandRange(-30,30)+FVector(0,0,Random.FRandRange(-8,18));Add(From+V*Back,V,Random.FRandRange(.74,.88),Random.FRandRange(19,28),3.4,false,0);}
    if(Random.FRand()<DT*16.f)Add(From+Dir*Random.FRandRange(350,1080),Dir*Random.FRandRange(90,220)+FVector(0,0,110),Random.FRandRange(1.4,2.0),Random.FRandRange(130,190),1,true);
    Illuminate(From+Dir*260,3000);
}
void AFrontlineFuelFireV33::FeedWreckLightV839(const FVector& Source,EFrontlineVehicle Type,int32 Site,float Strength,float Phase,UStaticMeshComponent* Hull)
{
    if(Source.ContainsNaN()||!FMath::IsFinite(Strength)||!FMath::IsFinite(Phase)||Strength<=.015f)return;
    int32 Slot=INDEX_NONE,Free=INDEX_NONE;
    for(int32 I=0;I<WreckLightBudgetV839;++I)
    {
        const auto& S=WreckLightStateV839[I];
        const bool SameOwner=Hull?S.Hull.Get()==Hull:!S.Hull.IsValid()&&FMath::IsNearlyEqual(S.Phase,Phase,.001f)
            &&FVector::DistSquared(S.Source,Source)<FMath::Square(80.f);
        if(S.Site==Site&&S.Model==int32(Type)&&SameOwner&&(S.FeedAge<.8f||S.Power>.01f)){Slot=I;break;}
        if(Free==INDEX_NONE&&S.FeedAge>=.8f&&S.Power<=.01f)Free=I;
    }
    if(Slot==INDEX_NONE)
    {
        // Saturation drops the new light. An admitted burning source never
        // steals a lit slot or moves another fire's local illumination.
        if(Free==INDEX_NONE){++WreckLightDropsV839;return;}
        Slot=Free;WreckLightStateV839[Slot]=FFrontlineWreckLightV839{};++WreckLightAssignmentsV839;
    }
    auto& S=WreckLightStateV839[Slot];S.Source=Source;S.Strength=FMath::Clamp(Strength,0.f,2.2f);
    S.Phase=Phase;S.Hull=Hull;S.Site=Site;S.Model=int32(Type);S.FeedAge=0;
    ++WreckLightFeedsV839;SetActorTickEnabled(true);
}
bool AFrontlineFuelFireV33::TickWreckLightsV839(float DT)
{
    bool Live=false;const float Clock=GetWorld()->TimeSeconds;
    for(int32 I=0;I<WreckLightBudgetV839;++I)
    {
        auto& S=WreckLightStateV839[I];auto* L=WreckLightsV839[I].Get();S.FeedAge+=DT;
        // Same combustion phase as the visible core, with bounded low-amplitude
        // variation. The former 29-radian global-slot modulation is absent.
        const float Pulse=.82f+.04f*FMath::Sin(Clock*2.7f+S.Phase)+.02f*FMath::Sin(Clock*6.3f+S.Phase*2.1f);
        const float Feed=1.f-FMath::SmoothStep(.18f,.65f,S.FeedAge);
        const float Target=2250.f*S.Strength*Pulse*Feed,Before=S.Power;
        S.Power=FMath::Max(0.f,Before+FMath::Clamp(Target-Before,-WreckLightChangePerSecondV839*DT,WreckLightChangePerSecondV839*DT));
        MaximumWreckLightRateV839=FMath::Max(MaximumWreckLightRateV839,FMath::Abs(S.Power-Before)/DT);
        const bool Visible=S.Power>.01f;L->SetVisibility(Visible);L->SetIntensity(Visible?S.Power:0.f);
        if(Visible){L->SetWorldLocation(S.Source);Live=true;}
        Live|=S.FeedAge<.8f;
    }
    return Live;
}
TSharedRef<FJsonObject> AFrontlineFuelFireV33::DescribeEffectsV839() const
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;int32 Active=0;bool Valid=true;
    const auto Vector=[](const FVector& V){return TArray<TSharedPtr<FJsonValue>>{
        MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};};
    for(int32 I=0;I<WreckLightBudgetV839;++I)
    {
        const auto& S=WreckLightStateV839[I];const auto* L=WreckLightsV839[I].Get();
        Valid&=L&&FMath::IsFinite(L->Intensity)&&L->Intensity>=0&&L->Intensity<=2250.f*2.2f*.88f+.1f
            &&!L->CastShadows&&L->VolumetricScatteringIntensity==0.f;
        if(!L||!L->IsVisible()||L->Intensity<=0.f)continue;
        ++Active;const bool Bound=!L->GetComponentTransform().ContainsNaN()&&L->GetComponentLocation().Equals(S.Source,.01f)
            &&FMath::IsNearlyEqual(L->Intensity,S.Power,.01f);Valid&=Bound;
        auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("slot"),I);R->SetNumberField(TEXT("model"),S.Model);
        R->SetNumberField(TEXT("site"),S.Site);R->SetNumberField(TEXT("feed_age_seconds"),S.FeedAge);
        R->SetNumberField(TEXT("actual_intensity"),L->Intensity);R->SetNumberField(TEXT("source_strength"),S.Strength);
        R->SetNumberField(TEXT("source_phase"),S.Phase);R->SetBoolField(TEXT("actual_source_binding_valid"),Bound);
        R->SetArrayField(TEXT("actual_light_world_cm"),Vector(L->GetComponentLocation()));
        R->SetArrayField(TEXT("source_world_cm"),Vector(S.Source));
        if(auto* H=S.Hull.Get())R->SetStringField(TEXT("source_hull"),H->GetPathName());
        Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetNumberField(TEXT("wreck_light_capacity"),WreckLightBudgetV839);J->SetNumberField(TEXT("active_wreck_lights"),Active);
    J->SetNumberField(TEXT("wreck_light_feeds"),WreckLightFeedsV839);J->SetNumberField(TEXT("wreck_light_drops"),WreckLightDropsV839);
    J->SetNumberField(TEXT("wreck_light_assignments"),WreckLightAssignmentsV839);
    J->SetNumberField(TEXT("maximum_observed_intensity_change_per_game_second"),MaximumWreckLightRateV839);
    J->SetNumberField(TEXT("intensity_change_limit_per_game_second"),WreckLightChangePerSecondV839);
    J->SetBoolField(TEXT("actual_light_bindings_valid"),Valid);J->SetArrayField(TEXT("wreck_lights"),Rows);
    J->SetNumberField(TEXT("actual_fire_ticks"),ActualFireTicksV838);J->SetNumberField(TEXT("last_actual_fire_delta_seconds"),LastFireDeltaV838);
    if(GetWorld()){J->SetNumberField(TEXT("world_game_time_seconds"),GetWorld()->TimeSeconds);
        J->SetNumberField(TEXT("world_real_time_seconds"),GetWorld()->GetRealTimeSeconds());
        J->SetNumberField(TEXT("world_delta_seconds"),GetWorld()->GetDeltaSeconds());
        J->SetBoolField(TEXT("world_paused"),GetWorld()->IsPaused());}
    J->SetStringField(TEXT("scope"),TEXT("Production component readback. Stable wreck source identity, full game delta and bounded intensity. Rendered persistence, FG/RR differences and light GPU cost need actual sequence review."));
    return J;
}
void AFrontlineFuelFireV33::RunEffectsChecksV839(TFunctionRef<void(const FString&,bool)> Check) const
{
    const auto J=DescribeEffectsV839();
    Check(TEXT("v839_wreck_lights_actual_finite_bound_no_shadow_components"),J->GetBoolField(TEXT("actual_light_bindings_valid")));
    Check(TEXT("v839_wreck_light_actual_intensity_change_is_bounded"),MaximumWreckLightRateV839<=WreckLightChangePerSecondV839+.2f);
    Check(TEXT("v839_wreck_light_actual_pool_capacity"),J->GetNumberField(TEXT("active_wreck_lights"))<=WreckLightBudgetV839);
}
TSharedRef<FJsonObject> AFrontlineFuelFireV33::DescribeWreckFlameV8310() const
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    const bool Ready=FlameStatesV836.Num()==ContactBeginV8310+ContactBudgetV836
        &&FlameCoresV836.Num()==FlameStatesV836.Num()&&FlameMaterialsV836.Num()==FlameStatesV836.Num();
    bool Valid=Ready;int32 Visible=0,Legacy=0;
    for(const auto& P:Particles)if(P.Age<P.Life&&!P.Smoke&&P.Layer==2)++Legacy;
    if(Ready)for(int32 I=FlameCoreBudgetV836;I<ContactBeginV8310;++I)
    {
        const auto& S=FlameStatesV836[I];const auto* C=FlameCoresV836[I].Get();auto* M=FlameMaterialsV836[I].Get();
        if(S.Age>=.4f)continue;
        const auto Radii=M?M->K2_GetVectorParameterValue(TEXT("VolumeRadii")):FLinearColor::Black;
        const float Density=M?M->K2_GetScalarParameterValue(TEXT("Density")):0.f;
        const float Steps=M?M->K2_GetScalarParameterValue(TEXT("QualitySteps")):0.f;
        const bool Turbulent=!FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline"))
            &&!FParse::Param(FCommandLine::Get(),TEXT("WFV8311FireBaseline"))
            &&!FParse::Param(FCommandLine::Get(),TEXT("WFV8311InitialFlame"));
        const bool Bound=C&&M&&C->GetMaterial(0)==M&&!C->GetComponentTransform().ContainsNaN()
            &&C->GetComponentLocation().Equals(S.Source,.05f)&&S.Dimensions.GetMin()>=2.f
            &&FVector(Radii.R,Radii.G,Radii.B).Equals(S.Dimensions*.5,.05f)
            &&FMath::IsFinite(Density)&&Density>=0.f&&Density<=2.5f
            &&C->IsVisible()==(Density>.01f)
            &&M->K2_GetScalarParameterValue(TEXT("Profile"))==0.f
            &&Steps==(FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline"))?12.f:
                FParse::Param(FCommandLine::Get(),TEXT("WFV8311FireBaseline"))?18.f:Turbulent?64.f:28.f)
            &&M->GetBaseMaterial()->GetPathName()==(FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline"))
                ?TEXT("/Game/Warfare/Field/V836/Materials/M_FlameVolume_V836.M_FlameVolume_V836")
                :FParse::Param(FCommandLine::Get(),TEXT("WFV8311FireBaseline"))
                ?TEXT("/Game/Warfare/Field/V8310/Materials/M_FlameVolume_V8310.M_FlameVolume_V8310")
                :Turbulent?TEXT("/Game/Warfare/Field/V8311/Materials/M_WreckCombustion_Turbulent_V8311.M_WreckCombustion_Turbulent_V8311")
                :TEXT("/Game/Warfare/Field/V8311/Materials/M_WreckCombustion_V8311.M_WreckCombustion_V8311"))
            &&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->CastShadow;
        const FVector Bottom=S.Source-FVector(0,0,S.Dimensions.Z*.5f);
        if(Turbulent)Valid&=S.bMeasuredOpeningV8311&&Bottom.Equals(S.OpeningV8311,.1f);
        Valid&=Bound;Visible+=C&&C->IsVisible();
        auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("slot"),I);
        R->SetNumberField(TEXT("model"),S.WreckModelV8310);R->SetNumberField(TEXT("site"),S.WreckSiteV8310);
        R->SetNumberField(TEXT("feed_age"),S.Age);R->SetNumberField(TEXT("actual_density"),Density);
        R->SetNumberField(TEXT("quality_steps"),Steps);
        R->SetNumberField(TEXT("fed_frame"),double(S.FedFrameV8310));R->SetNumberField(TEXT("presented_frame"),double(S.PresentedFrameV8310));
        R->SetNumberField(TEXT("fed_fire_tick"),double(S.FedFireTickV8310));
        R->SetNumberField(TEXT("feed_sequence"),double(S.FeedSequenceV8310));R->SetNumberField(TEXT("presented_feed_sequence"),double(S.PresentedFeedSequenceV8310));
        R->SetBoolField(TEXT("primary"),S.bWreckPrimaryV8310);R->SetBoolField(TEXT("retiring"),S.bWreckRetiringV8310);
        R->SetNumberField(TEXT("source_phase"),S.Seed);R->SetBoolField(TEXT("actual_binding_valid"),Bound);
        R->SetBoolField(TEXT("measured_mesh_opening"),S.bMeasuredOpeningV8311);
        R->SetStringField(TEXT("actual_volume_bottom"),Bottom.ToString());R->SetStringField(TEXT("actual_opening_world"),S.OpeningV8311.ToString());
        R->SetBoolField(TEXT("visible"),C&&C->IsVisible());
        if(auto* H=S.WreckHullV8310.Get())R->SetStringField(TEXT("source_hull"),H->GetPathName());
        if(M)
        {
            R->SetStringField(TEXT("actual_parent"),M->GetBaseMaterial()->GetPathName());
            if(auto* T=Cast<UTexture2D>(M->K2_GetTextureParameterValue(TEXT("DensityVolume"))))
            {
                R->SetStringField(TEXT("actual_fuel_field"),T->GetPathName());
                R->SetNumberField(TEXT("field_width"),T->GetSizeX());R->SetNumberField(TEXT("field_height"),T->GetSizeY());
                R->SetNumberField(TEXT("resident_mips"),T->GetNumResidentMips());
                R->SetNumberField(TEXT("total_mips"),T->GetNumMips());
                if(!FParse::Param(FCommandLine::Get(),TEXT("WFV8311FireBaseline"))
                    &&!FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline")))
                    Valid&=T->GetSizeX()==6400&&T->GetSizeY()==4800&&T->GetNumResidentMips()==1;
                if(Turbulent)Valid&=T->GetFName()==TEXT("T_FuelHeat96_Turbulent_V8311");
            }
            else if(!FParse::Param(FCommandLine::Get(),TEXT("WFV8311FireBaseline"))
                &&!FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline")))Valid=false;
        }
        Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetBoolField(TEXT("assets_ready"),Ready);J->SetBoolField(TEXT("actual_volume_bindings_valid"),Valid);
    J->SetNumberField(TEXT("wreck_source_capacity"),WreckFlameBudgetV8310);
    J->SetNumberField(TEXT("visible_wreck_cores"),Visible);J->SetNumberField(TEXT("legacy_wreck_flame_cards"),Legacy);
    J->SetNumberField(TEXT("wreck_feeds"),WreckFlameFeedsV8310);J->SetNumberField(TEXT("wreck_pool_drops"),WreckFlameDropsV8310);
    J->SetNumberField(TEXT("wreck_legacy_fallbacks"),WreckFlameFallbacksV8310);J->SetArrayField(TEXT("wreck_cores"),Rows);
    J->SetNumberField(TEXT("current_frame"),double(GFrameCounter));J->SetNumberField(TEXT("current_fire_tick_sequence"),double(WreckFireTickSequenceV8310));
    J->SetStringField(TEXT("scope"),TEXT("Actual production wreck source/MID readback. Single volume combustion envelope with tiny embers; V838 soot and V839 source lights unchanged. Real camera, temporal upscaling and warmed performance remain visual acceptance gates."));
    return J;
}
void AFrontlineFuelFireV33::RunWreckFlameChecksV8310(TFunctionRef<void(const FString&,bool)> Check) const
{
    const auto J=DescribeWreckFlameV8310();
    Check(TEXT("v8310_wreck_actual_bound_finite_single_volume_components"),J->GetBoolField(TEXT("actual_volume_bindings_valid")));
    Check(TEXT("v8310_wreck_source_pool_has_no_active_source_steal"),WreckFlameDropsV8310==0);
    Check(TEXT("v8310_production_wreck_has_no_legacy_blurred_flame_cards"),J->GetNumberField(TEXT("legacy_wreck_flame_cards"))==0);
    Check(TEXT("v8310_production_wreck_uses_no_asset_or_pool_fallback"),WreckFlameFallbacksV8310==0);
}
void AFrontlineFuelFireV33::RunWreckFlamePoolChecksV8310(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;
    const bool Guard=B&&B->GetWorld()==GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8310Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
    Check(TEXT("v8310_wreck_pool_probe_is_disposable_no_save_review"),Guard);if(!Guard)return;
    for(float Delta:{.4f,.6f})for(int32 Order=0;Order<2;++Order)
    {
        Clear();const FTransform Pose(FRotator::ZeroRotator,FVector(0,-5000,30));
        EmitVehicleWreckV838(EFrontlineVehicle::MarkIVMale,Pose,.016f,5.f,1.71f);
        if(Order==0)Tick(Delta);
        else
        {
            Tick(.016f);Tick(Delta);
            EmitVehicleWreckV838(EFrontlineVehicle::MarkIVMale,Pose,.016f,5.f,1.71f);
        }
        const auto Seen=DescribeWreckFlameV8310();bool Committed=true;
        for(const auto& Row:Seen->GetArrayField(TEXT("wreck_cores")))
        {const auto Q=Row->AsObject();Committed&=Q->GetNumberField(TEXT("feed_sequence"))==Q->GetNumberField(TEXT("presented_feed_sequence"))
            &&Q->GetNumberField(TEXT("fed_frame"))==Q->GetNumberField(TEXT("presented_frame"));}
        Check(FString::Printf(TEXT("v8310_%s_%dms_hitch_refresh_is_presented_this_frame"),Order?TEXT("tick_before_feed"):TEXT("feed_before_tick"),FMath::RoundToInt(Delta*1000)),
            Seen->GetBoolField(TEXT("actual_volume_bindings_valid"))&&Seen->GetNumberField(TEXT("visible_wreck_cores"))==2&&Committed);
        // Consume a same-frame freshness marker once before testing a stopped
        // source. GFrameCounter is intentionally unchanged throughout this
        // real-Tick probe; the per-fire tick sequence must still expire it.
        Tick(.001f);Tick(Delta);
        Check(FString::Printf(TEXT("v8310_%s_%dms_hitch_stopped_source_really_expires_without_frame_advance"),Order?TEXT("tick_before_feed"):TEXT("feed_before_tick"),FMath::RoundToInt(Delta*1000)),
            DescribeWreckFlameV8310()->GetNumberField(TEXT("visible_wreck_cores"))==0);
    }
    Clear();
    const auto TankPose=[](int32 I){return FTransform(FRotator::ZeroRotator,FVector(I*2200.f,-5000.f,30.f));};
    for(int32 I=0;I<12;++I)EmitVehicleWreckV838(EFrontlineVehicle::MarkIVMale,TankPose(I),.016f,5.f,I*.71f);
    Tick(.016f);
    Check(TEXT("v8310_twelve_real_emitters_fill_twenty_four_volume_sources"),DescribeWreckFlameV8310()->GetNumberField(TEXT("visible_wreck_cores"))==24);
    const auto Before=FlameStatesV836;
    EmitVehicleWreckV838(EFrontlineVehicle::MarkIVMale,TankPose(12),.016f,5.f,12*.71f);
    int32 Retiring=0;bool Unmoved=true;
    for(int32 I=FlameCoreBudgetV836;I<ContactBeginV8310;++I)
    {Retiring+=FlameStatesV836[I].bWreckRetiringV8310;Unmoved&=FlameStatesV836[I].Source.Equals(Before[I].Source,.01f)&&FlameStatesV836[I].Seed==Before[I].Seed;}
    Check(TEXT("v8310_full_pool_retires_one_secondary_without_source_or_phase_jump"),Retiring==1&&Unmoved);
    // Feed in ordinary old-before-new actor order. A retiring secondary must
    // not repeatedly reclaim the primary's slot when its old fade completes.
    for(int32 Frame=0;Frame<36;++Frame)
    {
        for(int32 I=0;I<13;++I)EmitVehicleWreckV838(EFrontlineVehicle::MarkIVMale,TankPose(I),.016f,5.f,I*.71f);
        Tick(.016f);
    }
    int32 Primary=0;
    for(int32 I=FlameCoreBudgetV836;I<ContactBeginV8310;++I)Primary+=FlameStatesV836[I].Age<.4f&&FlameStatesV836[I].bWreckPrimaryV8310;
    Check(TEXT("v8310_new_primary_acquires_smoothly_retired_secondary_in_actual_feed_order"),Primary==13);
    Check(TEXT("v8310_saturated_tank_pool_never_restores_blurry_legacy_cards"),DescribeWreckFlameV8310()->GetNumberField(TEXT("legacy_wreck_flame_cards"))==0);
    Clear();
    for(int32 I=0;I<24;++I)EmitRailCarWreckV838(TankPose(I),.016f,0,I*.71f);
    Tick(.016f);const auto Primaries=FlameStatesV836;
    EmitRailCarWreckV838(TankPose(24),.016f,0,24*.71f);Tick(.016f);Unmoved=true;Retiring=0;
    for(int32 I=FlameCoreBudgetV836;I<ContactBeginV8310;++I)
    {Retiring+=FlameStatesV836[I].bWreckRetiringV8310;Unmoved&=FlameStatesV836[I].Source.Equals(Primaries[I].Source,.01f)&&FlameStatesV836[I].Seed==Primaries[I].Seed;}
    const auto Full=DescribeWreckFlameV8310();
    Check(TEXT("v8310_full_primary_pool_retains_active_identity_and_never_uses_legacy_cards"),Unmoved&&Retiring==0
        &&Full->GetNumberField(TEXT("visible_wreck_cores"))==24&&Full->GetNumberField(TEXT("legacy_wreck_flame_cards"))==0
        &&WreckFlameDropsV8310>0&&WreckFlameFallbacksV8310==0);
    Clear();
    Check(TEXT("v8310_wreck_clear_resets_sources_retirement_and_counters"),WreckFlameFeedsV8310==0&&WreckFlameDropsV8310==0
        &&WreckFlameFallbacksV8310==0&&DescribeWreckFlameV8310()->GetNumberField(TEXT("visible_wreck_cores"))==0);
}
void AFrontlineFuelFireV33::EmitClothing(AWarfareFrontlineSoldier* U,float DT,float Strength)
{
    if(!U||!U->Soldier||Strength<=0)return;
    const float Gain=FMath::Clamp(Strength,0.f,1.f),Time=GetWorld()->TimeSeconds;
    const FVector Drift=FireDriftV8214(Time),Right=U->GetActorRightVector(),Forward=U->GetActorForwardVector(),Back=-Forward;
    FeedFlameCoreV836(U->Soldier->GetBoneLocation(TEXT("spine_03"))+Forward*8.f+FVector(0,0,34),
        FVector(76,68,125)*(.75f+.25f*Gain),Gain*.9f,U->GetMotionPhase()*13.f,U->Soldier,TEXT("spine_03"));
    FeedFlameCoreV836(U->Soldier->GetBoneLocation(TEXT("pelvis"))+Back*5.f+FVector(0,0,25),
        FVector(67,62,100)*(.75f+.25f*Gain),Gain*.72f,U->GetMotionPhase()*13.f+3.f,U->Soldier,TEXT("pelvis"));
    // Real coat surfaces surround the posed body. Offsetting every bone to
    // the back hid most fires behind the torso at the production side camera.
    const FName Bones[]={TEXT("spine_03"),TEXT("spine_02"),TEXT("upperarm_l"),TEXT("upperarm_r"),TEXT("pelvis"),TEXT("thigh_l"),TEXT("thigh_r")};
    const FVector SurfaceOffsets[]={Forward*18.f+Right*8.f,Back*19.f-Right*7.f,
        -Right*16.f+Forward*6.f,Right*16.f+Back*7.f,Right*14.f+Forward*8.f,
        -Right*12.f+Back*6.f,Right*12.f+Forward*6.f};
    const int32 Count=FireEmissionCountV8214(Random,DT,(18.f+44.f*Gain)*.24f);
    for(int32 I=0;I<Count;++I)
    {
        const int32 Site=EmissionSerial++%UE_ARRAY_COUNT(Bones);const FName Bone=Bones[Site];
        const FTransform BonePose=U->Soldier->GetSocketTransform(Bone,RTS_World);const FVector Anchor=BonePose.GetLocation();
        const FVector Offset=SurfaceOffsets[Site]+Right*Random.FRandRange(-3.f,3.f)+Forward*Random.FRandRange(-2.f,2.f);
        const bool Tongue=Random.FRand()<.40f;
        const FVector Outward=Offset.GetSafeNormal2D()*Random.FRandRange(8.f,22.f);
        auto* P=Add(Anchor+Offset,Drift*.20f+Outward+FVector(Random.FRandRange(-8,8),Random.FRandRange(-7,7),Random.FRandRange(24,48)),
            Random.FRandRange(Tongue?.38f:.58f,Tongue?.66f:.95f),Random.FRandRange(Tongue?28.f:40.f,Tongue?44.f:66.f)*(.72f+.28f*Gain),
            Random.FRandRange(Tongue?1.35f:.95f,Tongue?1.85f:1.35f),false,Random.FRandRange(Tongue?-.35f:-.55f,Tongue?.35f:.55f));
        if(P)
        {
            P->Layer=1;P->Growth=Tongue?.35f:.58f;P->FrameRate=Random.FRandRange(9,18);P->Heat=Random.FRandRange(.72f,1.12f)*(.25f+.75f*Gain);
            P->Buoyancy=85;P->Drag=1.20f;P->CurlRate=Random.FRandRange(7,11);P->Spin=Random.FRandRange(-.60f,.60f);
            P->AttachedMesh=U->Soldier;P->AttachedBone=Bone;P->LastAnchor=Anchor+Offset;
            P->AttachmentLocalOffset=BonePose.InverseTransformPosition(Anchor+Offset);P->AttachSeconds=Random.FRandRange(.32f,.55f);
            // Broad billows face the observer; a minority of tongues retain
            // intersecting source planes instead of making a thin edge-on row.
            P->bSourceRibbonV8216=Tongue&&(Site==1||Site==3||Site==5);
            P->RibbonBearingV8216=FMath::Atan2(float(Offset.Y),float(Offset.X))+PI*.5f;
        }
    }
    if(FireEmissionCountV8214(Random,DT,2.2f*Gain)>0)
    {
        const FVector At=U->Soldier->GetBoneLocation(TEXT("spine_02"))+Back*14+FVector(0,0,18);
        if(auto* P=Add(At,Drift*.45f+FVector(0,0,Random.FRandRange(48,74)),Random.FRandRange(1.3f,2.1f),Random.FRandRange(42,69),1,true,Random.FRandRange(-PI,PI)))
        {P->Layer=1;P->Growth=1.7f;P->FrameRate=Random.FRandRange(4,7);P->Heat=.35f*Gain;P->Buoyancy=29;P->Drag=.6f;P->CurlRate=1.9f;P->Spin=Random.FRandRange(-.18f,.18f);}
    }
    if(FireEmissionCountV8214(Random,DT,3.5f*Gain)>0)
    {
        const FVector At=U->Soldier->GetBoneLocation(Bones[Random.RandRange(0,UE_ARRAY_COUNT(Bones)-1)])+Back*15;
        if(auto* P=Add(At,Drift+Random.VRand()*22+FVector(0,0,Random.FRandRange(80,125)),Random.FRandRange(.55f,1.1f),Random.FRandRange(1.5f,3.3f),1,false))
        {P->Layer=3;P->Growth=0;P->Heat=Random.FRandRange(.6f,1.f)*Gain;P->Buoyancy=-32;P->Drag=.62f;P->CurlRate=2.5f;}
    }
    if(Gain>.45f)Illuminate(U->Soldier->GetBoneLocation(TEXT("spine_02"))+FVector(0,0,12.f),750.f*Gain);
}
void AFrontlineFuelFireV33::FuelBurst(const FVector& At,float Fuel)
{
    const float Gain=FMath::Clamp(Fuel/12.f,.30f,1.f);
    // A torn fuel vessel expels unequal fast sheets, then a compact turbulent
    // burn. Avoid the old identical slow spheres growing into one orange ball.
    const float Bias=Random.FRandRange(-PI,PI);
    for(int32 I=0;I<16;++I)
    {
        const float Angle=Bias+Random.FRandRange(-2.7f,2.7f),Speed=Random.FRandRange(430,790);
        const FVector D(FMath::Cos(Angle),FMath::Sin(Angle),Random.FRandRange(.12f,.54f));
        Add(At+D*Random.FRandRange(0,18),D*Speed*(.65f+Gain*.35f),Random.FRandRange(.30f,.65f),Random.FRandRange(23,43)*Gain,2.25f,false,Random.FRandRange(-.18f,.18f));
    }
    for(int32 I=0;I<12;++I)
        Add(At+Random.VRand()*22,Random.VRand()*Random.FRandRange(90,210)+FVector(0,0,105),Random.FRandRange(.38f,.85f),Random.FRandRange(43,72)*Gain,1.15f,false,Random.FRandRange(-1.1f,1.1f));
    for(int32 I=0;I<9;++I)
        Add(At+Random.VRand()*36,Random.VRand()*95+FVector(0,0,Random.FRandRange(80,150)),Random.FRandRange(2.8f,4.1f),Random.FRandRange(125,205)*(.7f+Gain*.3f),1,true);
    Illuminate(At,6200*Gain);
    FrontlineBlastCompositionV8221::EmitFuelBurst(this,At,Fuel);
}
void AFrontlineFuelFireV33::EmitBurningWreck(const FVector& At,float DT,float Strength,float SourcePhase,bool bPrimary,float SmokeStrength,float SourceScale,bool bLegacyLight,bool bDedicatedVolumeV8310)
{
    SourceScale=FMath::IsFinite(SourceScale)?FMath::Clamp(SourceScale,.35f,1.5f):1.f;
    const float Gain=FMath::Clamp(Strength,0.f,2.2f),Time=GetWorld()->TimeSeconds;
    const FVector Drift=FireDriftV8214(Time);const float Pulse=.82f+.12f*FMath::Sin(Time*2.7f+SourcePhase)+.06f*FMath::Sin(Time*6.3f+SourcePhase*2.1f);
    const float CoreScale=FMath::Sqrt(FMath::Clamp(Gain,.12f,2.2f));
    if(!bDedicatedVolumeV8310)FeedFlameCoreV836(At+FVector(0,0,84.f*CoreScale*SourceScale),FVector(170,145,245)*CoreScale*SourceScale,
        Gain*.82f*Pulse,SourcePhase);
    // The full-size old flipbook billows obscured the volume with a second,
    // blurred silhouette. Production wrecks now have one combustion envelope;
    // tiny embers and the independently authored soot remain separate layers.
    const int32 Count=bDedicatedVolumeV8310?0:FireEmissionCountV8214(Random,DT,(bPrimary?18.f:11.f)*FMath::Min(Gain,1.4f)*Pulse*.24f);
    for(int32 I=0;I<Count;++I)
    {
        const bool Tongue=Random.FRand()<.58f;const float Scale=FMath::Sqrt(FMath::Max(.02f,Gain));
        const FVector Offset=FVector(Random.FRandRange(-68,68),Random.FRandRange(-52,52),Random.FRandRange(-16,22))*SourceScale;
        if(auto* P=Add(At+Offset,Drift*.35f+FVector(Random.FRandRange(-15,15),Random.FRandRange(-12,12),Random.FRandRange(55,100)),
            Random.FRandRange(Tongue?.40f:.66f,Tongue?.76f:1.14f),Random.FRandRange(Tongue?43.f:78.f,Tongue?78.f:126.f)*Scale*SourceScale,
            Random.FRandRange(Tongue?2.1f:1.5f,Tongue?3.f:2.2f),false,Random.FRandRange(-.22f,.22f)))
        {P->Layer=2;P->Growth=Tongue?.42f:.65f;P->FrameRate=Random.FRandRange(8,15);P->Heat=Random.FRandRange(.80f,1.16f)*FMath::Clamp(Gain,.12f,1.2f);P->Buoyancy=135;P->Drag=.9f;P->CurlRate=Random.FRandRange(5,9);P->Spin=Random.FRandRange(-.22f,.22f);P->bSourceRibbonV8216=Tongue&&I%2==0;P->RibbonBearingV8216=SourcePhase+EmissionSerial++*2.399963f;}
    }
    const float Soot=FMath::Clamp(SmokeStrength<0?Gain:SmokeStrength,0.f,1.7f);
    if(bPrimary)FeedSootVolumeV8216(At,Soot,Gain,SourcePhase);
    const int32 SmokeCount=FireEmissionCountV8214(Random,DT,(bPrimary?2.1f:1.5f)*Soot);
    for(int32 I=0;I<SmokeCount;++I)
        if(auto* P=Add(At+FVector(Random.FRandRange(-25,25),Random.FRandRange(-20,20),95),Drift*.65f+FVector(0,0,Random.FRandRange(86,132)),
            Random.FRandRange(3.0f,4.6f),Random.FRandRange(103,161)*(.65f+.35f*Soot),Random.FRandRange(.9f,1.2f),true,Random.FRandRange(-PI,PI)))
        {P->Layer=2;P->Growth=1.9f;P->FrameRate=Random.FRandRange(3,5);P->Heat=.45f*FMath::Min(Gain,1.f);P->Buoyancy=38;P->Drag=.35f;P->CurlRate=1.4f;P->Spin=Random.FRandRange(-.14f,.14f);}
    if(FireEmissionCountV8214(Random,DT,(bPrimary?5.f:2.f)*Gain)>0)
        if(auto* P=Add(At+Random.VRand()*45,Drift+Random.VRand()*36+FVector(0,0,Random.FRandRange(145,245)),Random.FRandRange(.8f,1.7f),Random.FRandRange(3,6.7f),1,false))
        {P->Layer=3;P->Growth=0;P->Heat=Random.FRandRange(.6f,1.1f)*FMath::Min(1.f,Gain);P->Buoyancy=-36;P->Drag=.52f;P->CurlRate=2.2f;}
    if(bLegacyLight&&bPrimary&&Gain>.15f)Illuminate(At+FVector(0,0,35),2250.f*Gain*Pulse);
}
void AFrontlineFuelFireV33::EmitVehicleWreckV838(EFrontlineVehicle Type,const FTransform& Pose,float DT,float WreckAge,float Phase,UStaticMeshComponent* Hull)
{
    if(int32(Type)>=int32(EFrontlineVehicle::Count)||Pose.ContainsNaN()||!FMath::IsFinite(DT)||DT<=0||!FMath::IsFinite(WreckAge)||WreckAge<0
        ||WreckAge>=120||!FMath::IsFinite(Phase))return;
    const auto Profile=FrontlineWreckSmokeV838::Profile(Type,Hull);
    const float Ignition=.58f+.42f*FMath::SmoothStep(0.f,.8f,WreckAge);
    const float Fire=1.45f*Ignition*(1.f-FMath::SmoothStep(16.f,65.f,WreckAge));
    const float Soot=.95f*(1.f-FMath::SmoothStep(62.f,120.f,WreckAge));
    // Their 104–160 cm source widths have shorter optical chords than the
    // heavy hulls. Actual 4K views require more soot mass, without enlarging
    // either light chassis's plume; the existing finite source cap remains.
    const float SootMass=(Type==EFrontlineVehicle::Whippet||Type==EFrontlineVehicle::Austin1918)?2.f:1.f;
    for(int32 I=0;I<Profile.Sites.Num();++I)
    {
        const auto& Site=Profile.Sites[I];const FVector Source=Pose.TransformPosition(Site.LocalOrigin);
        const float SourcePhase=Phase+I*2.3f;
        const float Gain=Fire*Site.FlameWeight,SourceScale=Site.FlameScale*Profile.FootprintScale;
        const float CoreScale=FMath::Sqrt(FMath::Clamp(Gain,.12f,2.2f));
        const float Time=GetWorld()->TimeSeconds;
        const float Pulse=.82f+.12f*FMath::Sin(Time*2.7f+SourcePhase)+.06f*FMath::Sin(Time*6.3f+SourcePhase*2.1f);
        const FVector Dimensions=FVector(170,145,245)*CoreScale*SourceScale;
        FVector Opening;bool Measured=false;
        const FVector Centre=WreckFlameCentreV8311(Type,I,Pose,Hull,Source,Dimensions,Opening,Measured);
        const bool Dedicated=FeedWreckFlameV8310(Centre,Dimensions,Gain*.82f*Pulse,SourcePhase,Hull,Type,I,Site.Primary,Opening,Measured);
        // A single model-specific origin feeds flame and smoke. Zero legacy
        // soot prevents an extra universal plume or camera-facing smoke card.
        EmitBurningWreck(Source,DT,Gain,SourcePhase,Site.Primary,0.f,SourceScale,false,Dedicated);
        if(Site.Primary)FeedWreckLightV839(Source+FVector(0,0,35),Type,I,Fire*Site.FlameWeight,SourcePhase,Hull);
        FeedWreckSootV838(Source,Site,Profile.Name,Type,I,Soot*Site.SmokeWeight*SootMass,Fire*Site.FlameWeight,SourcePhase,Hull);
    }
}
void AFrontlineFuelFireV33::EmitRailCarWreckV838(const FTransform& Pose,float DT,int32 Car,float Phase,UStaticMeshComponent* Hull)
{
    if(Pose.ContainsNaN()||!FMath::IsFinite(DT)||DT<=0||Car<0||Car>=5||!FMath::IsFinite(Phase))return;
    const auto Profile=FrontlineWreckSmokeV838::Profile(EFrontlineVehicle::RailConvoy,Hull);
    constexpr int32 Sites[]={0,1,3,4,5};const int32 SiteIndex=Sites[Car];
    const auto& Site=Profile.Sites[SiteIndex];const FVector Source=Pose.TransformPosition(Site.LocalOrigin);
    const float CoreScale=FMath::Sqrt(.60f),Time=GetWorld()->TimeSeconds;
    const float Pulse=.82f+.12f*FMath::Sin(Time*2.7f+Phase)+.06f*FMath::Sin(Time*6.3f+Phase*2.1f);
    const FVector Dimensions=FVector(170,145,245)*CoreScale*Site.FlameScale;
    FVector Opening;bool Measured=false;
    const FVector Centre=WreckFlameCentreV8311(EFrontlineVehicle::RailConvoy,SiteIndex,Pose,Hull,Source,Dimensions,Opening,Measured);
    const bool Dedicated=FeedWreckFlameV8310(Centre,Dimensions,.60f*.82f*Pulse,Phase,Hull,EFrontlineVehicle::RailConvoy,SiteIndex,Site.Primary,Opening,Measured);
    EmitBurningWreck(Source,DT,.60f,Phase,Site.Primary,0.f,Site.FlameScale,false,Dedicated);
    if(Site.Primary)FeedWreckLightV839(Source+FVector(0,0,35),EFrontlineVehicle::RailConvoy,SiteIndex,.60f,Phase,Hull);
    FeedWreckSootV838(Source,Site,Profile.Name,EFrontlineVehicle::RailConvoy,SiteIndex,.56f*Site.SmokeWeight,.60f,Phase,Hull);
}
TSharedRef<FJsonObject> AFrontlineFuelFireV33::DescribeWreckSmokeV838() const
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Sources;
    const auto Vector=[](const FVector& V){return TArray<TSharedPtr<FJsonValue>>{
        MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};};
    const bool Ready=WreckSootV838.Num()==WreckSmokeVolumeBudgetV838&&WreckSootMaterialsV838.Num()==WreckSmokeVolumeBudgetV838;
    J->SetBoolField(TEXT("assets_ready"),Ready);J->SetNumberField(TEXT("source_capacity"),WreckSmokeSourceBudgetV838);
    J->SetNumberField(TEXT("volume_capacity"),WreckSmokeVolumeBudgetV838);J->SetNumberField(TEXT("layers_per_source"),FrontlineWreckSmokeV838::LayersPerSource);
    J->SetNumberField(TEXT("feed_calls"),WreckSmokeFeedsV838);J->SetNumberField(TEXT("source_evictions"),WreckSmokeEvictionsV838);
    J->SetBoolField(TEXT("actor_tick_enabled"),IsActorTickEnabled());J->SetBoolField(TEXT("actor_tick_registered"),PrimaryActorTick.IsTickFunctionRegistered());
    J->SetBoolField(TEXT("has_cosmetic_work"),bHasParticles);J->SetNumberField(TEXT("actual_fire_ticks"),ActualFireTicksV838);
    J->SetNumberField(TEXT("last_fire_delta_seconds"),LastFireDeltaV838);J->SetNumberField(TEXT("particle_slots"),Particles.Num());
    if(GetWorld()){J->SetBoolField(TEXT("world_paused"),UGameplayStatics::IsGamePaused(GetWorld()));
        J->SetNumberField(TEXT("world_time_seconds"),GetWorld()->TimeSeconds);J->SetNumberField(TEXT("world_delta_seconds"),GetWorld()->GetDeltaSeconds());}
    int32 ActiveSources=0,VisibleLayers=0;bool Bound=true,EachSourceVisible=true;float MaximumDensity=0;
    if(Ready)for(int32 I=0;I<WreckSootStateV838.Num();++I)
    {
        const auto& S=WreckSootStateV838[I];if(S.FeedAge>=FrontlineWreckSmokeV838::TailSeconds)continue;
        ++ActiveSources;auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("model"),S.Model);
        Row->SetStringField(TEXT("profile"),S.Profile);Row->SetStringField(TEXT("site"),S.SiteName);
        Row->SetNumberField(TEXT("site_index"),S.Site);Row->SetNumberField(TEXT("source_width_cm"),S.WidthCm);
        Row->SetNumberField(TEXT("feed_age"),S.FeedAge);Row->SetNumberField(TEXT("flow_age"),S.FlowAge);
        Row->SetNumberField(TEXT("fuel_soot_strength"),S.Strength);Row->SetNumberField(TEXT("combustion_heat"),S.Heat);
        Row->SetArrayField(TEXT("source_world_cm"),Vector(S.Source));
        if(auto* Hull=S.Hull.Get())
        {
            Row->SetStringField(TEXT("source_hull"),Hull->GetPathName());
            Row->SetArrayField(TEXT("source_local_cm"),Vector(Hull->GetComponentTransform().InverseTransformPosition(S.Source)));
            if(Hull->GetStaticMesh())Row->SetStringField(TEXT("source_mesh"),Hull->GetStaticMesh()->GetPathName());
        }
        TArray<TSharedPtr<FJsonValue>> LayerDataV838;int32 SourceVisible=0;
        for(int32 Layer=0;Layer<FrontlineWreckSmokeV838::LayersPerSource;++Layer)
        {
            const int32 Slot=I*FrontlineWreckSmokeV838::LayersPerSource+Layer;
            auto* C=WreckSootV838[Slot].Get();auto* M=WreckSootMaterialsV838[Slot].Get();auto L=MakeShared<FJsonObject>();
            const auto Centre=M->K2_GetVectorParameterValue(TEXT("VolumeCenter")),Radii=M->K2_GetVectorParameterValue(TEXT("VolumeRadii"));
            const FVector ActualCentre(Centre.R,Centre.G,Centre.B),ActualRadii(Radii.R,Radii.G,Radii.B);
            const float Density=M->K2_GetScalarParameterValue(TEXT("Density"));const bool Visible=C->IsVisible();
            const FVector ActualDimensions=C->GetComponentScale()*100.f;const auto Colour=M->K2_GetVectorParameterValue(TEXT("DustColor"));
            const bool Valid=!C->GetComponentTransform().ContainsNaN()&&!ActualCentre.ContainsNaN()&&!ActualRadii.ContainsNaN()
                &&FMath::IsFinite(Density)&&Density>=0&&Density<=1.55f&&C->GetMaterial(0)==M
                &&C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==TEXT("Cube")
                &&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision
                &&(!Visible||(ActualRadii.GetMin()>1&&ActualCentre.Equals(C->GetComponentLocation(),.1f)
                    &&(ActualRadii*2).Equals(ActualDimensions,.1f)&&Density>.01f));
            Bound&=Valid;SourceVisible+=Visible;VisibleLayers+=Visible;MaximumDensity=FMath::Max(MaximumDensity,Density);
            L->SetNumberField(TEXT("layer"),Layer);L->SetBoolField(TEXT("visible"),Visible);L->SetBoolField(TEXT("actual_binding_valid"),Valid);
            L->SetNumberField(TEXT("density"),Density);L->SetNumberField(TEXT("actual_volume_profile"),M->K2_GetScalarParameterValue(TEXT("VolumeProfile")));
            L->SetNumberField(TEXT("transported_field_age"),M->K2_GetScalarParameterValue(TEXT("Age")));
            L->SetNumberField(TEXT("quality_steps"),M->K2_GetScalarParameterValue(TEXT("QualitySteps")));
            L->SetArrayField(TEXT("actual_center_cm"),Vector(C->GetComponentLocation()));L->SetArrayField(TEXT("actual_dimensions_cm"),Vector(ActualDimensions));
            L->SetArrayField(TEXT("actual_soot_color"),Vector(FVector(Colour.R,Colour.G,Colour.B)));
            if(M->GetBaseMaterial())L->SetStringField(TEXT("actual_base_material"),M->GetBaseMaterial()->GetPathName());
            LayerDataV838.Add(MakeShared<FJsonValueObject>(L));
        }
        EachSourceVisible&=SourceVisible>0;Row->SetNumberField(TEXT("visible_layers"),SourceVisible);
        Row->SetArrayField(TEXT("actual_layers"),LayerDataV838);Sources.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetNumberField(TEXT("active_sources"),ActiveSources);J->SetNumberField(TEXT("visible_layers"),VisibleLayers);
    J->SetNumberField(TEXT("maximum_actual_density"),MaximumDensity);J->SetBoolField(TEXT("actual_bindings_valid"),Ready&&Bound);
    J->SetBoolField(TEXT("every_active_source_has_visible_volume"),EachSourceVisible);J->SetArrayField(TEXT("sources"),Sources);
    J->SetStringField(TEXT("scope"),TEXT("Actual pooled wreck smoke proxy/MID readback. Source sites follow model-local authored geometry; no hit, casualty, attribution or damage rules. Visual density, wind motion and warmed Shipping frame cost still require actual camera review."));
    return J;
}
void AFrontlineFuelFireV33::RunWreckSmokeChecksV838(TFunctionRef<void(const FString&,bool)> Check) const
{
    const auto Actual=DescribeWreckSmokeV838();
    Check(TEXT("v838_wreck_smoke_actual_fixed_pool_ready"),Actual->GetBoolField(TEXT("assets_ready"))
        &&WreckSootStateV838.Num()==WreckSmokeSourceBudgetV838);
    Check(TEXT("v838_wreck_smoke_actual_depth_volume_bindings_are_finite_and_noncolliding"),Actual->GetBoolField(TEXT("actual_bindings_valid")));
    Check(TEXT("v838_wreck_smoke_actual_counts_remain_inside_shared_pool"),Actual->GetNumberField(TEXT("active_sources"))<=WreckSmokeSourceBudgetV838
        &&Actual->GetNumberField(TEXT("visible_layers"))<=WreckSmokeVolumeBudgetV838);
    bool MatureLayers=true,DriftAboveSource=true,FiniteSootField=WreckSootMaterialsV838.Num()==WreckSmokeVolumeBudgetV838;
    for(const auto& Entry:WreckSootMaterialsV838)
    {
        auto* Material=Entry.Get();
        FiniteSootField&=Material&&Material->GetBaseMaterial()&&Material->GetBaseMaterial()->GetName()==TEXT("M_WreckSoot_V838")
            &&FMath::IsNearlyEqual(Material->K2_GetScalarParameterValue(TEXT("VolumeProfile")),3.f);
    }
    for(int32 I=0;I<WreckSootStateV838.Num();++I)
    {
        const auto& S=WreckSootStateV838[I];if(S.FeedAge>=FrontlineWreckSmokeV838::TailSeconds)continue;
        for(int32 L=0;L<FrontlineWreckSmokeV838::LayersPerSource;++L)
        {
            const auto* C=WreckSootV838.IsValidIndex(I*FrontlineWreckSmokeV838::LayersPerSource+L)?WreckSootV838[I*FrontlineWreckSmokeV838::LayersPerSource+L].Get():nullptr;
            if(S.FlowAge>1.2f&&S.FeedAge<.1f&&S.Strength>.12f)MatureLayers&=C&&C->IsVisible();
            if(C&&C->IsVisible())
            {
                const FVector Rise=C->GetComponentLocation()-S.Source;
                DriftAboveSource&=Rise.Z>0&&Rise.Size2D()<Rise.Z+20.f;
                auto* M=WreckSootMaterialsV838[I*FrontlineWreckSmokeV838::LayersPerSource+L].Get();
                FiniteSootField&=M&&M->GetBaseMaterial()&&M->GetBaseMaterial()->GetName()==TEXT("M_WreckSoot_V838")
                    &&FMath::IsNearlyEqual(M->K2_GetScalarParameterValue(TEXT("VolumeProfile")),3.f)
                    &&M->K2_GetScalarParameterValue(TEXT("QualitySteps"))>=12.f&&M->K2_GetScalarParameterValue(TEXT("QualitySteps"))<=32.f;
            }
        }
    }
    Check(TEXT("v838_wreck_smoke_mature_fed_sources_use_three_actual_layers"),MatureLayers);
    Check(TEXT("v838_wreck_smoke_actual_buoyancy_exceeds_coherent_breeze"),DriftAboveSource);
    Check(TEXT("v838_wreck_smoke_actual_finite_soot_material_and_private_ray_budget"),FiniteSootField);
    const auto Car=FrontlineWreckSmokeV838::Profile(EFrontlineVehicle::Austin1918);
    const auto Light=FrontlineWreckSmokeV838::Profile(EFrontlineVehicle::Whippet);
    const auto Heavy=FrontlineWreckSmokeV838::Profile(EFrontlineVehicle::MarkVMale);
    const auto Train=FrontlineWreckSmokeV838::Profile(EFrontlineVehicle::RailConvoy);
    Check(TEXT("v838_wreck_smoke_model_sites_and_footprints_are_distinct"),Car.Sites.Num()==2&&Light.Sites.Num()==2
        &&Heavy.Sites.Num()==2&&Train.Sites.Num()==6&&Car.Sites[0].LocalOrigin.X>100&&Light.Sites[0].LocalOrigin.X>100
        &&Heavy.Sites[0].LocalOrigin.X<0&&Car.Sites[0].WidthCm<Light.Sites[0].WidthCm&&Light.Sites[0].WidthCm<Heavy.Sites[0].WidthCm);
}
int32 AFrontlineFuelFireV33::ActiveCount()const{int32 N=0;for(const auto& P:Particles)N+=P.Age<P.Life;return N;}
FFrontlineFuelVisualStatsV8214 AFrontlineFuelFireV33::VisualStateV8214()const
{
    FFrontlineFuelVisualStatsV8214 S;S.PoolCapacity=FlameBudget+SmokeBudget;S.MinFrameRate=TNumericLimits<float>::Max();
    for(const auto& P:Particles)if(P.Age<P.Life)
    {
        ++S.PoolUsed;if(P.Smoke)++S.Soot;else if(P.Layer==1)++S.Clothing;else if(P.Layer==2)++S.Wreck;else if(P.Layer==3)++S.Ember;else ++S.Jet;
        S.AttachedClothing+=P.Layer==1&&!P.Smoke&&P.AttachedMesh.IsValid()&&P.Age<P.AttachSeconds;
        S.SourceRibbonsV8216+=P.bSourceRibbonV8216&&!P.Smoke;
        S.MinFrameRate=FMath::Min(S.MinFrameRate,P.FrameRate);S.MaxFrameRate=FMath::Max(S.MaxFrameRate,P.FrameRate);
        if(!P.Smoke&&P.Layer!=3)S.MaxFlameWidthCm=FMath::Max(S.MaxFlameWidthCm,P.Size*FireGrowthV8214(P,P.Age/P.Life));
    }
    for(const auto& V:VolumeStateV8216)S.VolumeSootV8216+=V.Age<2.f;
    for(const auto& V:WreckSootStateV838)if(V.FeedAge<FrontlineWreckSmokeV838::TailSeconds)++S.WreckSmokeSourcesV838;
    for(const auto& C:WreckSootV838)if(C&&C->IsVisible())++S.WreckSmokeVolumesV838;
    S.VolumeSootV8216+=S.WreckSmokeVolumesV838;
    for(int32 I=0;I<FlameStatesV836.Num();++I)if(FlameStatesV836[I].Age<(I<ContactBeginV8310?.40f:.15f))
        (I<ContactBeginV8310?S.FlameCoresV836:S.ArmourFlashesV836)++;
    if(!S.PoolUsed)S.MinFrameRate=0;return S;
}
void AFrontlineFuelFireV33::Tick(float DT)
{
    Super::Tick(DT);if(Particles.IsEmpty())return;
    // Consume the complete game-time budget for expiry. Small integration steps
    // keep the accepted advection stable without slowing smoke at 4x speed.
    DT=FMath::IsFinite(DT)?FMath::Max(0.f,DT):0.f;if(DT<=0.f)return;
    ++ActualFireTicksV838;LastFireDeltaV838=DT;
    bool Lit=false;for(int32 I=0;I<LightBudget;++I){LightAge[I]+=DT;LightEnergy[I]*=FMath::Exp(-DT*4);const float Power=LightAge[I]<.4f?LightEnergy[I]*(1+.08f*FMath::Sin(GetWorld()->TimeSeconds*29+I)):0;Lights[I]->SetIntensity(Power);Lit|=Power>0;}
    Lit|=TickWreckLightsV839(DT);
    if(!bHasParticles){if(!Lit)SetActorTickEnabled(false);return;}
    FVector Camera;FRotator View;if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetPlayerViewPoint(Camera,View);else return;
    const FVector CameraRight=FRotationMatrix(View).GetUnitAxis(EAxis::Y),CameraUp=FRotationMatrix(View).GetUnitAxis(EAxis::Z);
    TickSootVolumesV8216(DT,View.Vector());
    TickWreckSootV838(DT,View.Vector());
    TickFlameCoresV836(DT,View.Vector());
    bool FlameDirty=false,SmokeDirty=false,AnyLive=false;
    for(int32 I=0;I<Particles.Num();++I)
    {
        auto& P=Particles[I];auto* C=P.Smoke?Smoke.Get():Flames.Get();const int32 K=P.Smoke?I-FlameBudget:I;
        const float PreviousAge=P.Age;P.Age+=DT;
        if(P.Age>=P.Life)
        {
            if(P.Visible){C->UpdateInstanceTransform(K,FTransform(FQuat::Identity,FVector(0,0,-10000),FVector::ZeroVector),true,false,true);P.Visible=false;(P.Smoke?SmokeDirty:FlameDirty)=true;}
            continue;
        }
        P.Visible=true;AnyLive=true;(P.Smoke?SmokeDirty:FlameDirty)=true;
        const float T=P.Age/P.Life;const float Curl=P.Seed*2.7f+P.Age*P.CurlRate;
        if(P.AttachedMesh.IsValid())
        {
            if(P.Age<P.AttachSeconds)
            {
                const FVector Anchor=P.AttachedMesh->GetSocketTransform(P.AttachedBone,RTS_World).TransformPosition(P.AttachmentLocalOffset);
                const float Follow=1.f-FMath::SmoothStep(P.AttachSeconds*.55f,P.AttachSeconds,P.Age);
                P.P+=(Anchor-P.LastAnchor)*Follow;P.LastAnchor=Anchor;
            }
            else P.AttachedMesh.Reset();
        }
        const float CurlStrength=P.Layer==0?1.f:P.Smoke?.65f:.8f;
        // Only surviving particles need motion. Their unchanged life envelope
        // bounds this loop even after a long frame; each step remains <= .05 s.
        const int32 Steps=FMath::Max(1,FMath::CeilToInt(DT/.05f));const float Step=DT/Steps;
        for(int32 J=0;J<Steps;++J)
        {
            const float StepAge=PreviousAge+Step*(J+1),StepCurl=P.Seed*2.7f+StepAge*P.CurlRate;
            P.V+=FVector(FMath::Sin(StepCurl*1.1f)*18*CurlStrength,FMath::Cos(StepCurl)*28*CurlStrength,P.Buoyancy)*Step;
            P.V*=FMath::Exp(-Step*P.Drag);P.P+=P.V*Step;
        }
        // Project the velocity into the viewing plane: sprites form a fuel
        // stream in depth and screen space instead of five fixed rectangles.
        FVector Long=CameraUp;float Stretch=P.Stretch;
        if(!P.Smoke&&P.V.Size2D()>420){Long=(CameraRight*FVector::DotProduct(P.V,CameraRight)+CameraUp*FVector::DotProduct(P.V,CameraUp)).GetSafeNormal();Stretch=FMath::Lerp(P.Stretch,1.75f,T);}
        Long=FQuat((Camera-P.P).GetSafeNormal(),P.Turn+P.Spin*P.Age+FMath::Sin(Curl)*.07f).RotateVector(Long);
        FVector Normal=(Camera-P.P).GetSafeNormal(),Across=FVector::CrossProduct(Long,Normal).GetSafeNormal();
        if(P.bSourceRibbonV8216)
        {
            // Source-oriented ribbons cross the camera-facing billows. Their
            // differing planes reveal depth rather than turning as one card.
            Long=(FVector::UpVector+FireDriftV8214(GetWorld()->TimeSeconds)*.002f).GetSafeNormal();
            Across=FVector(FMath::Cos(P.RibbonBearingV8216),FMath::Sin(P.RibbonBearingV8216),0);
            Normal=FVector::CrossProduct(Across,Long).GetSafeNormal();
        }
        const FQuat Q=FRotationMatrix::MakeFromXY(Across,Long).ToQuat();
        const float Width=P.Size*FireGrowthV8214(P,T);
        if(!P.Smoke&&(P.Layer==1||P.Layer==2))Stretch*=1.f-.30f*FMath::SmoothStep(.48f,1.f,T);
        C->UpdateInstanceTransform(K,FTransform(Q,P.P,FVector(Width/100,Width*Stretch/100,1)),true,false,true);
        C->SetCustomDataValue(K,0,P.Seed+P.Age*P.FrameRate,false);
        const float FadeStart=P.Layer==0?(P.Smoke?.45f:.60f):P.Smoke?.42f:P.Layer==3?.16f:P.Layer==1?.48f:.30f;
        const float FadeIn=P.Layer==0?.045f:P.Smoke?.14f:.025f;
        C->SetCustomDataValue(K,1,(1-FMath::SmoothStep(FadeStart,1.f,T))*FMath::Min(1.f,P.Age/FadeIn)*(P.Smoke&&P.Layer==2?.42f:1.f),false);
        C->SetCustomDataValue(K,2,P.Seed,false);
        C->SetCustomDataValue(K,3,T,false);
        C->SetCustomDataValue(K,4,P.Heat,false);
        C->SetCustomDataValue(K,5,float(P.Layer),false);
    }
    for(const auto& V:VolumeStateV8216)AnyLive|=V.Age<2.f;
    for(const auto& V:WreckSootStateV838)AnyLive|=V.FeedAge<FrontlineWreckSmokeV838::TailSeconds;
    for(int32 I=0;I<FlameStatesV836.Num();++I)AnyLive|=FlameStatesV836[I].Age<(I<ContactBeginV8310?.40f:.15f);
    if(FlameDirty)Flames->MarkRenderStateDirty();if(SmokeDirty)Smoke->MarkRenderStateDirty();bHasParticles=AnyLive;
    if(!AnyLive&&!Lit)SetActorTickEnabled(false);
}
