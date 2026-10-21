#include "WarfareFrontlineBlastCompositionV8221.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineParticulateV84412.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Dom/JsonObject.h"
#include "UObject/StrongObjectPtr.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
constexpr const TCHAR* FirePath=TEXT("/Game/Warfare/Field/V8221/Materials/M_BlastCore_V8221.M_BlastCore_V8221");
constexpr const TCHAR* AerosolPath=TEXT("/Game/Warfare/Field/V8221/Materials/M_IncidentAerosol_V8221.M_IncidentAerosol_V8221");
constexpr const TCHAR* EarthPath=TEXT("/Game/Warfare/Field/V8221/Materials/M_ThrownEarthJet_V8221.M_ThrownEarthJet_V8221");
constexpr const TCHAR* SplinterPath=TEXT("/Game/Warfare/Field/V8221/Materials/M_HotSplinter_V8221.M_HotSplinter_V8221");
constexpr const TCHAR* SoilPath=TEXT("/Game/Warfare/Field/V8221/Materials/M_ThrownSoil_V8221.M_ThrownSoil_V8221");
constexpr const TCHAR* SoilMeshPath=TEXT("/Game/Warfare/Field/V8221/Meshes/SM_SoilChunk_V8221.SM_SoilChunk_V8221");
constexpr const TCHAR* SplinterMeshPath=TEXT("/Game/Warfare/Field/V8221/Meshes/SM_ShellSplinter_V8221.SM_ShellSplinter_V8221");
constexpr const TCHAR* FirePathV831=TEXT("/Game/Warfare/Field/V831/Materials/M_BlastCore_V831.M_BlastCore_V831");
constexpr const TCHAR* AerosolPathV831=TEXT("/Game/Warfare/Field/V831/Materials/M_IncidentAerosol_V831.M_IncidentAerosol_V831");
constexpr const TCHAR* EarthPathV831=TEXT("/Game/Warfare/Field/V831/Materials/M_MineralDustVeil_V831.M_MineralDustVeil_V831");
constexpr const TCHAR* AtlasPathV831=TEXT("/Game/Warfare/Field/V831/Textures/T_CombustionImpulse_V831.T_CombustionImpulse_V831");
constexpr const TCHAR* EnvelopePathV831=TEXT("/Game/Warfare/Field/V831/Materials/M_BlastVolumeEnvelope_V831.M_BlastVolumeEnvelope_V831");
constexpr const TCHAR* AerosolPathV84411=TEXT("/Game/Warfare/Field/V84411/Materials/M_IncidentAerosol_V84411.M_IncidentAerosol_V84411");
constexpr const TCHAR* EnvelopePathV84411=TEXT("/Game/Warfare/Field/V84411/Materials/M_BlastVolumeEnvelope_V84411.M_BlastVolumeEnvelope_V84411");
constexpr const TCHAR* EarthPathV84412=TEXT("/Game/Warfare/Field/V84412/Materials/M_MineralDustVolume_V84412.M_MineralDustVolume_V84412");
constexpr const TCHAR* AerosolPathV84412=TEXT("/Game/Warfare/Field/V84412/Materials/M_IncidentAerosolVolume_V84412.M_IncidentAerosolVolume_V84412");
constexpr const TCHAR* GasPathV84412=TEXT("/Game/Warfare/Field/V84412/Materials/M_GasVolume_V84412.M_GasVolume_V84412");
float CoreLifetime(int32 Kind){return Kind==6?.54f:Kind==2?.18f:Kind==4?.16f:Kind==1?.24f:Kind==5?.23f:.28f;}
float CoreLifetimeV831(int32 Kind){return Kind==6?.42f:Kind==2?.145f:Kind==4?.13f:Kind==1?.19f:Kind==5?.20f:.23f;}
void NonSolidProxy(UStaticMeshComponent* C)
{
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
    C->SetCastShadow(false);C->SetReceivesDecals(false);C->SetVisibleInRayTracing(false);
    C->SetAffectDistanceFieldLighting(false);C->SetAffectDynamicIndirectLighting(false);
    C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;
}
FVector EjectionAxis(int32 Index,float Seed)
{
    static constexpr float Elevation[]={.31f,.70f,.41f,.55f,.36f,.88f};
    const float A=Seed*.55f+Index*2.399963f,E=Elevation[Index%6];
    return FVector(FMath::Cos(A)*FMath::Cos(E),FMath::Sin(A)*FMath::Cos(E),FMath::Sin(E));
}
void UpdateParticles(UStaticMeshComponent* C,UMaterialInstanceDynamic* M,const FVector& Centre,
    const FVector& LocalDimensions,const FVector& Direction,const FVector& ViewForward,float Density,float Age)
{
    const FVector Axis=Direction.GetSafeNormal(.0001f,FVector(.8,0,.6));
    const FVector Side=FVector::CrossProduct(FVector::UpVector,Axis).GetSafeNormal(.0001f,FVector::RightVector);
    const FVector Up=FVector::CrossProduct(Axis,Side);
    // The ray kernel uses this same orthonormal frame. Enclose every local
    // oriented corner, including density shear; rotating the proxy alone
    // would leave a world-axis density field upright.
    const FVector Bounds=(Side.GetAbs()*LocalDimensions.X+Up.GetAbs()*LocalDimensions.Y+Axis.GetAbs()*LocalDimensions.Z)*1.24f;
    FrontlineSmokeVolumeV35::Update(C,M,Centre,Bounds,ViewForward,Density,Age);
    M->SetVectorParameterValue(TEXT("EjectionAxis"),FLinearColor(Axis));
    M->SetVectorParameterValue(TEXT("JetRadii"),FLinearColor(LocalDimensions*.5f));
}
}

UFrontlineBlastCompositionV8221::UFrontlineBlastCompositionV8221()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false; // Impact owns its existing visual clock.
}

void FrontlineBlastCompositionV8221::EmitFuelBurst(AActor* Owner,const FVector& Origin,float Fuel)
{
    if(!IsValid(Owner)||!Owner->GetWorld()||!Owner->GetRootComponent()||Origin.ContainsNaN()||!FMath::IsFinite(Fuel))return;
    TArray<UFrontlineBlastCompositionV8221*> Present;Owner->GetComponents(Present);
    int32 Active=0;for(const auto* C:Present)if(C&&C->IsReady())++Active;
    if(Active>=8)return; // The existing FireV33 eruption still renders when these additions are at capacity.
    auto* C=NewObject<UFrontlineBlastCompositionV8221>(Owner,
        MakeUniqueObjectName(Owner,UFrontlineBlastCompositionV8221::StaticClass(),TEXT("FuelImpulseV8221")));
    C->RegisterComponent();Owner->AddInstanceComponent(C);C->Configure(6,Origin);
    if(!C->IsReady()){Owner->RemoveInstanceComponent(C);C->DestroyComponent();return;}
    C->StartIndependent(Fuel);
}

void UFrontlineBlastCompositionV8221::StartIndependent(float FuelStrength)
{
    if(!Ready||Kind!=6)return;
    Independent=true;IndependentAge=0.f;IndependentBornAt=FPlatformTime::Seconds();FirstIndependentTick=true;
    FuelGain=FMath::Clamp(FuelStrength/12.f,.3f,1.f);SetComponentTickEnabled(true);
}
void FrontlineBlastCompositionV8221::ClearFuelBursts(AActor* Owner)
{
    if(!IsValid(Owner))return;
    TArray<UFrontlineBlastCompositionV8221*> Present;Owner->GetComponents(Present);
    for(auto* C:Present)if(IsValid(C))C->ReleaseIndependent();
}
void UFrontlineBlastCompositionV8221::ReleaseIndependent()
{
    if(!Independent)return;
    Independent=false;Ready=false;SetComponentTickEnabled(false);
    AActor* Owner=GetOwner();
    auto Remove=[&](TArray<TObjectPtr<UStaticMeshComponent>>& Cs)
    {for(auto& C:Cs)if(C){if(Owner)Owner->RemoveInstanceComponent(C);C->DestroyComponent();}Cs.Reset();};
    Remove(Cores);Remove(Aerosols);Remove(EarthJets);
    if(Splinters){if(Owner)Owner->RemoveInstanceComponent(Splinters);Splinters->DestroyComponent();Splinters=nullptr;}
    CoreMaterials.Reset();AerosolMaterials.Reset();EarthMaterials.Reset();SplinterMaterial=nullptr;
    if(Owner)Owner->RemoveInstanceComponent(this);DestroyComponent();
}
void UFrontlineBlastCompositionV8221::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
    if(!Independent||!Ready)return;
    float Delta=FMath::Max(0.f,DeltaTime);
    if(FirstIndependentTick)
    {
        const float Dilation=GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation()*GetOwner()->CustomTimeDilation;
        Delta=FMath::Min(Delta,float(FMath::Max(0.,FPlatformTime::Seconds()-IndependentBornAt))*Dilation);
        FirstIndependentTick=false;
    }
    IndependentAge+=Delta;
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(GetOwner(),0);
    UpdateAt(IndependentAge,(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector());
    if(IndependentAge<1.1f)return;
    ReleaseIndependent();
}

void FrontlineBlastCompositionV8221::Prewarm()
{
    static TArray<TStrongObjectPtr<UObject>> Held;
    if(Held.Num()==17)return;
    for(const TCHAR* P:{FirePath,AerosolPath,EarthPath,SplinterPath,SoilPath,SoilMeshPath,SplinterMeshPath,
        FirePathV831,AerosolPathV831,EarthPathV831,AtlasPathV831,EnvelopePathV831,AerosolPathV84411,EnvelopePathV84411,
        EarthPathV84412,AerosolPathV84412,GasPathV84412})
        if(UObject* O=LoadObject<UObject>(nullptr,P))
        {bool Found=false;for(const auto& H:Held)Found|=H.Get()==O;if(!Found)Held.Emplace(O);}
}
UStaticMesh* UFrontlineBlastCompositionV8221::SoilMesh(){return LoadObject<UStaticMesh>(nullptr,SoilMeshPath);}
UMaterialInterface* UFrontlineBlastCompositionV8221::SoilMaterial(){return LoadObject<UMaterialInterface>(nullptr,SoilPath);}

void UFrontlineBlastCompositionV8221::Configure(int32 Profile,const FVector& Origin,float Radius,bool bUseArtResetV831)
{
    Ready=false;ArtResetV831=false;Kind=Profile;BlastOrigin=Origin;PhaseAge=0;CasualtyCount=0;CasualtyFeet.Reset();
    ParticulatesV84412=BaselineV84412=LightingAppliedV84412=false;
    FootprintRadius=FMath::Clamp(Radius,100.f,1600.f);
    for(auto& C:Cores)if(C)C->SetVisibility(false);
    for(auto& C:Aerosols)if(C)C->SetVisibility(false);
    for(auto& C:EarthJets)if(C)C->SetVisibility(false);
    if(Splinters)Splinters->ClearInstances();
    if(Kind==3||!GetOwner()||Origin.ContainsNaN())return; // bullet effects remain their existing non-explosive treatment
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Fire=LoadObject<UMaterialInterface>(nullptr,FirePath);
    auto* Mist=LoadObject<UMaterialInterface>(nullptr,AerosolPath);
    auto* Earth=LoadObject<UMaterialInterface>(nullptr,EarthPath);
    if(bUseArtResetV831&&!FParse::Param(FCommandLine::Get(),TEXT("WFV831EffectsBaseline")))
    {
        auto* NewFire=LoadObject<UMaterialInterface>(nullptr,FirePathV831);
        auto* NewMist=LoadObject<UMaterialInterface>(nullptr,AerosolPathV84411);
        if(!NewMist)NewMist=LoadObject<UMaterialInterface>(nullptr,AerosolPathV831);
        auto* NewEarth=LoadObject<UMaterialInterface>(nullptr,EarthPathV831);
        // Never release a mixed old/new composition when a cooked resource is
        // absent. All three short layers form one candidate; smoke is external.
        if(NewFire&&NewMist&&NewEarth)
        {Fire=NewFire;Mist=NewMist;Earth=NewEarth;ArtResetV831=true;}
    }
    const auto* Battle=GetWorld()?Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
    BaselineV84412=FParse::Param(FCommandLine::Get(),TEXT("WFV84412DustBaseline"))
        &&FrontlineEffectsReviewSafetyV8216::Allowed(Battle);
    if(ArtResetV831&&!BaselineV84412)
    {
        auto* NewEarth=FrontlineParticulateV84412::Material(this,0);
        auto* NewMist=FrontlineParticulateV84412::Material(this,1);
        // Bind these independent particulate layers atomically. Geometry,
        // emission time, blood incidence, combustion and accepted smoke stay
        // exactly paired with the old-material performance witness.
        if(NewEarth&&NewMist){Earth=NewEarth;Mist=NewMist;ParticulatesV84412=true;}
    }
    auto* Shard=LoadObject<UStaticMesh>(nullptr,SplinterMeshPath);
    auto* ShardBase=LoadObject<UMaterialInterface>(nullptr,SplinterPath);
    if(!Cube||!Fire||!Mist||!Earth||!Shard||!ShardBase||!SoilMesh()||!SoilMaterial())return;
    FRandomStream R(GetTypeHash(Origin)^uint32(Kind*8221));Seed=R.FRandRange(.1f,6.1f);
    auto Make=[&](const TCHAR* Prefix,int32 I,UMaterialInterface* M,TArray<TObjectPtr<UStaticMeshComponent>>& Cs,
        TArray<TObjectPtr<UMaterialInstanceDynamic>>& Ms)
    {
        auto* C=NewObject<UStaticMeshComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),UStaticMeshComponent::StaticClass(),*FString::Printf(TEXT("%s%d"),Prefix,I)));
        C->SetupAttachment(GetOwner()->GetRootComponent());C->SetStaticMesh(Cube);NonSolidProxy(C);
        // Never register a visible unmaterialled cube. Dense simultaneous
        // bursts can otherwise publish a proxy before its MID reaches the RT.
        C->SetVisibility(false);C->SetWorldScale3D(FVector(.001f));
        auto* D=UMaterialInstanceDynamic::Create(M,GetOwner());
        D->SetScalarParameterValue(TEXT("Density"),0.f);C->SetMaterial(0,D);
        C->RegisterComponent();GetOwner()->AddInstanceComponent(C);Cs.Add(C);Ms.Add(D);
    };
    if(Cores.IsEmpty())for(int32 I=0;I<3;++I)Make(TEXT("BlastCoreV8221_"),I,Fire,Cores,CoreMaterials);
    if(Aerosols.IsEmpty())for(int32 I=0;I<3;++I)Make(TEXT("IncidentAerosolV8221_"),I,Mist,Aerosols,AerosolMaterials);
    if(EarthJets.IsEmpty())for(int32 I=0;I<6;++I)Make(TEXT("ThrownEarthJetV8221_"),I,Earth,EarthJets,EarthMaterials);
    auto Rebind=[&](const TArray<TObjectPtr<UStaticMeshComponent>>& Cs,TArray<TObjectPtr<UMaterialInstanceDynamic>>& Ms,UMaterialInterface* Base)
    {
        for(int32 I=0;I<Cs.Num();++I)
            if(!Ms.IsValidIndex(I)||!Ms[I]||Ms[I]->GetBaseMaterial()!=Base->GetBaseMaterial())
            {auto* D=UMaterialInstanceDynamic::Create(Base,GetOwner());if(Ms.IsValidIndex(I))Ms[I]=D;else Ms.Add(D);Cs[I]->SetMaterial(0,D);}
    };
    Rebind(Cores,CoreMaterials,Fire);Rebind(Aerosols,AerosolMaterials,Mist);Rebind(EarthJets,EarthMaterials,Earth);
    if(!Splinters)
    {
        Splinters=NewObject<UInstancedStaticMeshComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),UInstancedStaticMeshComponent::StaticClass(),TEXT("ShellSplintersV8221")));
        Splinters->SetupAttachment(GetOwner()->GetRootComponent());Splinters->SetStaticMesh(Shard);
        Splinters->SetCollisionEnabled(ECollisionEnabled::NoCollision);Splinters->SetCanEverAffectNavigation(false);
        Splinters->SetCastShadow(false);Splinters->SetReceivesDecals(false);
        Splinters->SetVisibleInRayTracing(false);Splinters->SetAffectDistanceFieldLighting(false);
        Splinters->RegisterComponent();GetOwner()->AddInstanceComponent(Splinters);
        SplinterMaterial=UMaterialInstanceDynamic::Create(ShardBase,GetOwner());Splinters->SetMaterial(0,SplinterMaterial);
    }
    // FireV33 is a persistent service at a different world position from a fuel
    // rupture. Ballistic instances use impact-local positions around this origin.
    Splinters->SetWorldLocation(BlastOrigin);Splinters->SetWorldRotation(FRotator::ZeroRotator);Splinters->SetWorldScale3D(FVector::OneVector);
    FragmentVelocity.Reset();FragmentScale.Reset();FragmentAngular.Reset();
    const int32 Count=Kind==2?10:Kind==1||Kind==6?16:Kind==4?18:Kind==5?22:24;
    for(int32 I=0;I<Count;++I)
    {
        const float A=R.FRandRange(0.f,2.f*PI),Speed=R.FRandRange(430.f,Kind==2?840.f:1400.f);
        FVector V(FMath::Cos(A)*Speed,FMath::Sin(A)*Speed*.83f,R.FRandRange(180.f,Kind==4?350.f:800.f));
        if(Kind==4)V.Z=-R.FRandRange(1200.f,2100.f);
        FragmentVelocity.Add(V);FragmentScale.Add(FVector(R.FRandRange(.46f,.85f),R.FRandRange(.43f,.9f),R.FRandRange(.72f,1.45f))*(Kind==2?.72f:1.f));
        FragmentAngular.Add(FVector(R.FRandRange(-120.f,120.f),R.FRandRange(-190.f,190.f),R.FRandRange(-210.f,210.f)));
        Splinters->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
    }
    for(int32 I=0;I<3;++I)
    {
        CoreMaterials[I]->SetScalarParameterValue(TEXT("Seed"),Seed+I*1.731f);
        AerosolMaterials[I]->SetScalarParameterValue(TEXT("Seed"),Seed+I*2.161f);
        AerosolMaterials[I]->SetScalarParameterValue(TEXT("IncidentMode"),1.f);
    }
    for(int32 I=0;I<EarthMaterials.Num();++I)
    {
        EarthMaterials[I]->SetScalarParameterValue(TEXT("Seed"),Seed+I*1.473f);
        EarthMaterials[I]->SetScalarParameterValue(TEXT("IncidentMode"),0.f);
    }
    Ready=Cores.Num()==3&&Aerosols.Num()==3&&EarthJets.Num()==6&&FragmentVelocity.Num()==Count;
}

void UFrontlineBlastCompositionV8221::SetCasualties(const TArray<FVector>& Feet,int32 Count)
{
    CasualtyFeet.Reset();for(const FVector& P:Feet)if(!P.ContainsNaN()&&CasualtyFeet.Num()<16)CasualtyFeet.Add(P);
    CasualtyCount=FMath::Clamp(Count,0,16);
}

void UFrontlineBlastCompositionV8221::UpdateAt(float Age,const FVector& ViewForward)
{
    if(!Ready||!FMath::IsFinite(Age)||ViewForward.ContainsNaN())return;
    PhaseAge=FMath::Max(0.f,Age);
    if(ParticulatesV84412)
    {
        const auto Light=FrontlineParticulateV84412::SceneLighting(GetWorld());
        if(!LightingAppliedV84412||!Light.Direction.Equals(LightDirectionV84412,.00001f)
            ||!Light.Key.Equals(KeyRadianceV84412,.00001f)||!Light.Ambient.Equals(AmbientRadianceV84412,.00001f))
        {
            LightDirectionV84412=Light.Direction;KeyRadianceV84412=Light.Key;AmbientRadianceV84412=Light.Ambient;
            for(const auto* Set:{&EarthMaterials,&AerosolMaterials})for(auto& M:*Set)if(M)
            {
                M->SetVectorParameterValue(TEXT("MainLightDirection"),LightDirectionV84412);
                M->SetVectorParameterValue(TEXT("MainLightRadiance"),KeyRadianceV84412);
                M->SetVectorParameterValue(TEXT("AmbientRadiance"),AmbientRadianceV84412);
            }
            LightingAppliedV84412=true;
        }
    }
    const float Lifetime=ArtResetV831?CoreLifetimeV831(Kind):CoreLifetime(Kind),Scale=Kind==6?.84f*(.68f+.32f*FuelGain):Kind==2?.56f:Kind==4?.66f:Kind==1?.82f:Kind==5?.93f:1.f;
    const float BaseGround=AWarfareFrontlineGround::HeightAt(BlastOrigin.X,BlastOrigin.Y);
    for(int32 I=0;I<Cores.Num();++I)
    {
        const float Delay=I*(ArtResetV831?.012f:.017f),T=Age-Delay;
        if(T<0||T>=Lifetime){Cores[I]->SetVisibility(false);continue;}
        const float Push=1.f-FMath::Exp(-T*34.f),A=Seed+I*2.399963f;
        const FVector Offset(FMath::Cos(A)*I*44.f*Push,FMath::Sin(A)*I*48.f*Push,I?12.f+T*145.f:0.f);
        FVector Centre=BlastOrigin+Offset+FVector(0,0,Kind==4?12.f:Kind==6?8.f:46.f*Scale);
        if(Kind!=4)Centre.Z=FMath::Max(Centre.Z,BaseGround+34.f*Scale);
        const FVector Dim=ArtResetV831?
            FVector((I?104.f:215.f)*(1.f+.55f*Push),(I?96.f:190.f)*(1.f+.43f*Push),(I?126.f:149.f)*(1.f+.46f*Push))*Scale:
            FVector((I?124.f:252.f)*(1.f+.72f*Push),(I?121.f:239.f)*(1.f+.63f*Push),(I?155.f:177.f)*(1.f+.62f*Push))*Scale;
        const float Density=FMath::Clamp(T/.010f,0.f,1.f)*FMath::Pow(1.f-T/Lifetime,ArtResetV831?1.42f:1.12f)*(I?(ArtResetV831?.43f:.56f):1.f);
        FrontlineSmokeVolumeV35::Update(Cores[I],CoreMaterials[I],Centre,Dim,ViewForward,Density,T);
        CoreMaterials[I]->SetScalarParameterValue(TEXT("HeatPhase"),T/Lifetime);
    }
    for(int32 I=0;I<EarthJets.Num();++I)
    {
        // V831 leaves the solid throw to the lit ballistic instances. These
        // volumes are thin powder wakes; a continuous opaque wet fan was the
        // cause of the rejected brown, rubbery composition.
        const float T=Age-.010f-I*.006f,Life=ArtResetV831?(Kind==2?.42f:Kind==1?.59f:Kind==5?.65f:.69f):(Kind==2?.55f:Kind==1?.79f:Kind==5?.91f:.96f);
        if(Kind==4||Kind==6||T<0||T>=Life){EarthJets[I]->SetVisibility(false);continue;}
        const float Push=1.f-FMath::Exp(-T*8.5f),Speed=(I%3==0?1050.f:I%3==1?1270.f:900.f)*Scale;
        const FVector Axis=EjectionAxis(I,Seed);
        const FVector D=(ArtResetV831?FVector(105.f+190.f*Push,86.f+154.f*Push,190.f+340.f*Push):
            FVector(220.f+240.f*Push,165.f+195.f*Push,280.f+440.f*Push))*Scale;
        FVector Centre=BlastOrigin+Axis*(T*Speed);
        Centre.Z=BaseGround+45.f*Scale+Axis.Z*T*Speed-390.f*T*T;
        const FVector Direction=(Axis*Speed-FVector(0,0,780.f*T)).GetSafeNormal();
        const float Density=FMath::Clamp(T/.016f,0.f,1.f)*FMath::Pow(1.f-T/Life,ArtResetV831?1.45f:.88f)*
            (ArtResetV831?(I%3==0?1.15f:.82f):(I%3==0?1.58f:1.28f));
        // The calibrated density occupies the moving parcel. World flight and
        // envelope growth are owned here, with the original optical gain.
        UpdateParticles(EarthJets[I],EarthMaterials[I],Centre,D,Direction,ViewForward,Density,T);
    }
    for(int32 I=0;I<3;++I)
    {
        const float Delay=(Kind==4?.12f:.036f)+I*.028f,T=Age-Delay,Life=ArtResetV831?(Kind==2?.36f:.52f):(Kind==2?.48f:.68f);
        if(Kind==6||CasualtyCount==0||I>=FMath::Min(3,CasualtyCount)||T<0||T>=Life){Aerosols[I]->SetVisibility(false);continue;}
        const float Push=1.f-FMath::Exp(-T*8.f);
        // An incident cloud belongs to the impact. Bodies supply incidence only;
        // they never become the emitter of an explosive blood fountain.
        const FVector Direction=EjectionAxis(I*2+1,Seed);
        FVector Source=BlastOrigin;
        if(Kind==4&&!CasualtyFeet.IsEmpty())
        {
            const FVector Q=CasualtyFeet[FMath::Min(I,CasualtyFeet.Num()-1)];
            Source=FVector(Q.X,Q.Y,AWarfareFrontlineGround::HeightAt(Q.X,Q.Y)); // actual fragment impact
        }
        const FVector Dim=(ArtResetV831?FVector(165.f+265.f*Push,105.f+170.f*Push,170.f+240.f*Push):
            FVector(210.f+330.f*Push,135.f+215.f*Push,260.f+330.f*Push))*Scale;
        FVector Centre=Source+Direction*((ArtResetV831?35.f+T*430.f:60.f+T*610.f)*Scale)+
            FVector(0,0,(ArtResetV831?82.f+T*105.f:95.f+T*170.f)*Scale);
        const float Density=FMath::Clamp(T/.022f,0.f,1.f)*FMath::Pow(1.f-T/Life,ArtResetV831?1.42f:1.14f)*
            (ArtResetV831?(.17f+FMath::Min(CasualtyCount,4)*.038f):(.38f+FMath::Min(CasualtyCount,4)*.085f));
        UpdateParticles(Aerosols[I],AerosolMaterials[I],Centre,Dim,Direction,ViewForward,Density,T);
    }
    const float FragmentLife=Kind==2?.48f:.72f;
    if(SplinterMaterial)SplinterMaterial->SetScalarParameterValue(TEXT("ImpactAge"),Age);
    if(Age>FragmentLife){if(Splinters->GetInstanceCount())Splinters->ClearInstances();return;}
    for(int32 I=0;I<FragmentVelocity.Num();++I)
    {
        FVector P=FragmentVelocity[I]*Age+FVector(0,0,-490.f*Age*Age+18.f);
        const FVector W=BlastOrigin+P;const float Ground=AWarfareFrontlineGround::HeightAt(W.X,W.Y);
        const bool Above=Kind==4?W.Z>Ground+2.f:W.Z>Ground+4.f;
        const FRotator R(FragmentAngular[I].Y*Age,FragmentAngular[I].Z*Age,FragmentAngular[I].X*Age);
        const FVector S=Above?FragmentScale[I]*FMath::Clamp((FragmentLife-Age)/.13f,0.f,1.f):FVector::ZeroVector;
        Splinters->UpdateInstanceTransform(I,FTransform(R,P,S),false,false,true);
    }
    if(Splinters->GetInstanceCount())Splinters->MarkRenderStateDirty();
}

TSharedRef<FJsonObject> UFrontlineBlastCompositionV8221::Describe() const
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("ready"),Ready);J->SetNumberField(TEXT("profile"),Kind);
    J->SetNumberField(TEXT("age_s"),PhaseAge);J->SetNumberField(TEXT("casualties"),CasualtyCount);
    J->SetBoolField(TEXT("art_reset_v831"),ArtResetV831);
    J->SetBoolField(TEXT("particulates_v84412"),ParticulatesV84412);
    J->SetBoolField(TEXT("particulate_baseline_applied"),BaselineV84412);
    J->SetStringField(TEXT("particulate_pipeline"),ParticulatesV84412?TEXT("world-oriented 96-cubed parcel density; standard TLV lit translucent proxy plus bounded Beer self attenuation; not HV/PT"):TEXT("retained analytic particulate material"));
    J->SetNumberField(TEXT("particulate_max_ray_steps"),ParticulatesV84412?64:14);
    J->SetNumberField(TEXT("particulate_density_frames"),ParticulatesV84412?16:0);
    J->SetStringField(TEXT("actual_key_direction"),LightDirectionV84412.ToString());
    J->SetStringField(TEXT("actual_key_radiance"),KeyRadianceV84412.ToString());
    J->SetStringField(TEXT("actual_ambient_radiance"),AmbientRadianceV84412.ToString());
    J->SetBoolField(TEXT("particulate_light_parameters_applied"),LightingAppliedV84412);
    J->SetBoolField(TEXT("accepted_smoke_and_core_binding_unchanged"),true);
    int32 Fire=0,Mist=0,Earth=0;for(const auto& C:Cores)Fire+=C&&C->IsVisible();for(const auto& C:Aerosols)Mist+=C&&C->IsVisible();for(const auto& C:EarthJets)Earth+=C&&C->IsVisible();
    J->SetNumberField(TEXT("visible_true_volume_fire"),Fire);J->SetNumberField(TEXT("visible_incident_aerosol"),Mist);
    J->SetNumberField(TEXT("visible_true_volume_earth"),Earth);
    J->SetNumberField(TEXT("splinters"),Splinters?Splinters->GetInstanceCount():0);
    J->SetNumberField(TEXT("splinter_origin_error_cm"),Splinters?FVector::Dist(Splinters->GetComponentLocation(),BlastOrigin):-1.f);
    J->SetBoolField(TEXT("independent_fuel_impulse"),Independent);
    J->SetBoolField(TEXT("flat_flash_replaced"),ReplacesFlatFlash());
    J->SetStringField(TEXT("core_material"),CoreMaterials.Num()?GetPathNameSafe(CoreMaterials[0]):FString());
    J->SetStringField(TEXT("aerosol_material"),AerosolMaterials.Num()?GetPathNameSafe(AerosolMaterials[0]):FString());
    J->SetStringField(TEXT("bound_core_base"),CoreMaterials.Num()?GetPathNameSafe(CoreMaterials[0]->GetBaseMaterial()):FString());
    J->SetStringField(TEXT("bound_aerosol_base"),AerosolMaterials.Num()?GetPathNameSafe(AerosolMaterials[0]->GetBaseMaterial()):FString());
    J->SetStringField(TEXT("bound_earth_base"),EarthMaterials.Num()?GetPathNameSafe(EarthMaterials[0]->GetBaseMaterial()):FString());
    if(Splinters&&Splinters->GetStaticMesh())
    {
        const FVector Bounds=Splinters->GetStaticMesh()->GetBounds().BoxExtent*2;
        J->SetNumberField(TEXT("metal_mesh_extent_cm_x"),Bounds.X);J->SetNumberField(TEXT("metal_mesh_extent_cm_y"),Bounds.Y);J->SetNumberField(TEXT("metal_mesh_extent_cm_z"),Bounds.Z);
    }
    TArray<TSharedPtr<FJsonValue>> Layers;
    for(const auto& C:Cores)if(C&&C->IsVisible())
    {
        auto L=MakeShared<FJsonObject>();const FVector D=C->GetComponentScale()*100.f;
        L->SetNumberField(TEXT("width_cm"),D.X);L->SetNumberField(TEXT("depth_cm"),D.Y);L->SetNumberField(TEXT("height_cm"),D.Z);
        L->SetBoolField(TEXT("non_occluding_secondary_rays"),!C->bVisibleInRayTracing&&!C->CastShadow&&!C->bAffectDistanceFieldLighting);
        Layers.Add(MakeShared<FJsonValueObject>(L));
    }
    J->SetArrayField(TEXT("fire_layers"),Layers);
    auto ParticleLayers=[&](const TArray<TObjectPtr<UStaticMeshComponent>>& Cs,const TArray<TObjectPtr<UMaterialInstanceDynamic>>& Ms)
    {
        TArray<TSharedPtr<FJsonValue>> Result;
        for(int32 I=0;I<Cs.Num();++I)if(Cs[I]&&Cs[I]->IsVisible()&&Ms.IsValidIndex(I)&&Ms[I])
        {
            auto L=MakeShared<FJsonObject>();const auto Axis=Ms[I]->K2_GetVectorParameterValue(TEXT("EjectionAxis"));
            const auto Radii=Ms[I]->K2_GetVectorParameterValue(TEXT("JetRadii"));const FVector P=Cs[I]->GetComponentLocation();
            L->SetNumberField(TEXT("axis_x"),Axis.R);L->SetNumberField(TEXT("axis_y"),Axis.G);L->SetNumberField(TEXT("axis_z"),Axis.B);
            L->SetNumberField(TEXT("local_width_cm"),Radii.R*2);L->SetNumberField(TEXT("local_depth_cm"),Radii.G*2);L->SetNumberField(TEXT("local_length_cm"),Radii.B*2);
            L->SetNumberField(TEXT("centre_x_cm"),P.X);L->SetNumberField(TEXT("centre_y_cm"),P.Y);L->SetNumberField(TEXT("height_above_ground_cm"),P.Z-AWarfareFrontlineGround::HeightAt(P.X,P.Y));
            Result.Add(MakeShared<FJsonValueObject>(L));
        }
        return Result;
    };
    J->SetArrayField(TEXT("earth_layers"),ParticleLayers(EarthJets,EarthMaterials));
    J->SetArrayField(TEXT("aerosol_layers"),ParticleLayers(Aerosols,AerosolMaterials));return J;
}
void UFrontlineBlastCompositionV8221::Check(TFunctionRef<void(const FString&,bool)> Check) const
{
    Check(TEXT("v8221_blast_composition_actual_bound_resources"),Ready&&Cores.Num()==3&&Aerosols.Num()==3&&EarthJets.Num()==6&&Splinters&&CoreMaterials[0]&&AerosolMaterials[0]&&EarthMaterials[0]);
    bool Safe=true,Finite=true;
    for(const auto& C:Cores)if(C){Safe&=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->bVisibleInRayTracing&&!C->CastShadow;Finite&=!C->GetComponentLocation().ContainsNaN();}
    for(const auto& C:Aerosols)if(C){Safe&=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->bVisibleInRayTracing&&!C->CastShadow;Finite&=!C->GetComponentLocation().ContainsNaN();}
    for(const auto& C:EarthJets)if(C){Safe&=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->bVisibleInRayTracing&&!C->CastShadow;Finite&=!C->GetComponentLocation().ContainsNaN();}
    Check(TEXT("v8221_blast_new_proxy_layers_do_not_create_collision_or_opaque_secondary_ray_boxes"),Safe&&Finite);
    Check(TEXT("v8221_blast_splinter_world_origin_is_the_actual_explosion_source"),!Ready||(Splinters&&FVector::DistSquared(Splinters->GetComponentLocation(),BlastOrigin)<.01f));
    bool BoundEnclosure=true,Radial=true,Airborne=true;int32 ActiveEarth=0;
    auto ValidateParticles=[&](const TArray<TObjectPtr<UStaticMeshComponent>>& Cs,const TArray<TObjectPtr<UMaterialInstanceDynamic>>& Ms,bool Blood)
    {
        for(int32 I=0;I<Cs.Num();++I)if(Cs[I]&&Cs[I]->IsVisible()&&Ms.IsValidIndex(I)&&Ms[I])
        {
            const auto A=Ms[I]->K2_GetVectorParameterValue(TEXT("EjectionAxis"));const FVector Axis(A.R,A.G,A.B);
            const auto R=Ms[I]->K2_GetVectorParameterValue(TEXT("JetRadii"));const FVector Local(R.R,R.G,R.B);
            const FVector Side=FVector::CrossProduct(FVector::UpVector,Axis).GetSafeNormal();const FVector Up=FVector::CrossProduct(Axis,Side);
            const FVector Required=Side.GetAbs()*Local.X+Up.GetAbs()*Local.Y+Axis.GetAbs()*Local.Z;
            const FVector Actual=Cs[I]->GetComponentScale()*50.f;
            BoundEnclosure&=!Axis.ContainsNaN()&&FMath::Abs(Axis.SizeSquared()-1.f)<.002f&&Local.GetMin()>1
                &&Actual.X>=Required.X&&Actual.Y>=Required.Y&&Actual.Z>=Required.Z;
            if(Blood)
            {
                const FVector P=Cs[I]->GetComponentLocation();
                Airborne&=P.Z-AWarfareFrontlineGround::HeightAt(P.X,P.Y)>40.f;
            }
            else {++ActiveEarth;Radial&=Axis.SizeSquared2D()>.15f&&Local.Z/Local.X<2.f;}
        }
    };
    ValidateParticles(EarthJets,EarthMaterials,false);ValidateParticles(Aerosols,AerosolMaterials,true);
    Check(TEXT("v8221_particulate_proxy_encloses_the_actual_oriented_kernel"),BoundEnclosure);
    Check(TEXT("v8221_thrown_earth_uses_wide_nonvertical_radial_frames"),Radial);
    Check(TEXT("v8221_incident_aerosol_is_airborne_in_the_explosive_throw"),Airborne);
    const bool FanPhase=Kind!=3&&Kind!=4&&Kind!=6&&PhaseAge>.07f&&PhaseAge<(ArtResetV831?.34f:.48f);
    Check(TEXT("v8221_early_thrown_earth_has_multiple_actual_lobes"),!FanPhase||ActiveEarth==6);
    bool EmptyAerosol=true;for(const auto& C:Aerosols)EmptyAerosol&=!C||!C->IsVisible();
    Check(TEXT("v8221_blast_incident_aerosol_requires_actual_casualties"),CasualtyCount>0||EmptyAerosol);
    bool Expired=true;if(PhaseAge>1.03f){for(const auto& C:Cores)Expired&=!C->IsVisible();for(const auto& C:Aerosols)Expired&=!C->IsVisible();for(const auto& C:EarthJets)Expired&=!C->IsVisible();Expired&=Splinters&&Splinters->GetInstanceCount()==0;}
    Check(TEXT("v8221_blast_short_layers_expire_while_retained_smoke_may_continue"),Expired);
}
