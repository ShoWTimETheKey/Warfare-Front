#include "WarfareFrontlineParticulateV84412.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineVisualV85.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace FrontlineParticulateV84412
{
UMaterialInterface* Material(const UObject* Context,int32 Profile)
{
    const UWorld* World=Context?Context->GetWorld():nullptr;
    const auto* Battle=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
    const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV84412DustBaseline"))&&FrontlineEffectsReviewSafetyV8216::Allowed(Battle);
    const TCHAR* Path=Profile==2?TEXT("/Game/Warfare/Field/V84412/Materials/M_GasVolume_V84412.M_GasVolume_V84412"):
        Profile==1?TEXT("/Game/Warfare/Field/V84412/Materials/M_IncidentAerosolVolume_V84412.M_IncidentAerosolVolume_V84412"):
        TEXT("/Game/Warfare/Field/V84412/Materials/M_MineralDustVolume_V84412.M_MineralDustVolume_V84412");
    // Null keeps each caller's own old producer/material. A wheel-dust
    // witness must not silently become the unrelated old blast-dust graph.
    if(Baseline)return nullptr;
    if(Profile==2&&!FrontlineVisualV85::Baseline())
        if(auto* Gas=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V85/Materials/M_GasVolume_V85.M_GasVolume_V85"),nullptr,LOAD_NoWarn|LOAD_Quiet))return Gas;
    FString Latest(Path);Latest.ReplaceInline(TEXT("V84412"),TEXT("V84413"));
    if(auto* New=LoadObject<UMaterialInterface>(nullptr,*Latest))return New;
    return LoadObject<UMaterialInterface>(nullptr,Path);
}
UStaticMesh* Cube(){return LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));}
FSceneLighting SceneLighting(UWorld* World)
{
    struct FCached : FSceneLighting
    {
        TWeakObjectPtr<UDirectionalLightComponent> Sun;
        TWeakObjectPtr<USkyLightComponent> Sky;
        uint64 Frame=MAX_uint64;double SearchAgainAt=0;
    };
    static TMap<TWeakObjectPtr<UWorld>,FCached> Worlds;
    if(!IsValid(World))return {};
    if(Worlds.Num()>8)for(auto It=Worlds.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& State=Worlds.FindOrAdd(World);
    if(State.Frame==GFrameCounter)return State;
    State.Frame=GFrameCounter;
    if((!State.Sun.IsValid()||!State.Sky.IsValid())&&World->TimeSeconds>=State.SearchAgainAt)
    {
        State.SearchAgainAt=World->TimeSeconds+1.;
        for(TActorIterator<AActor> It(World);It;++It)
        {
            if(auto* Sun=It->FindComponentByClass<UDirectionalLightComponent>())
                if(Sun->IsVisible()&&(!State.Sun.IsValid()||Sun->Intensity>State.Sun->Intensity))State.Sun=Sun;
            if(auto* Sky=It->FindComponentByClass<USkyLightComponent>())
                if(Sky->IsVisible()&&(!State.Sky.IsValid()||Sky->Intensity>State.Sky->Intensity))State.Sky=Sky;
        }
    }
    State.Direction=FLinearColor(0,0,1);State.Key=State.Ambient=FLinearColor::Black;
    if(State.Sun.IsValid()&&State.Sun->IsVisible())
    {
        State.Direction=FLinearColor(-State.Sun->GetForwardVector());
        State.Key=State.Sun->GetLightColor()*FMath::Max(0.f,State.Sun->Intensity);
    }
    if(State.Sky.IsValid()&&State.Sky->IsVisible())State.Ambient=State.Sky->GetLightColor()*FMath::Max(0.f,State.Sky->Intensity);
    return State;
}
void ApplySceneLighting(UWorld* World,UMaterialInstanceDynamic* M)
{
    if(!IsValid(M))return;
    const auto Light=SceneLighting(World);
    // Stable weather/light values do not dirty dozens of pooled MIDs per Tick.
    struct FApplied{FSceneLighting Light;bool Ready=false;};
    static TMap<TWeakObjectPtr<UMaterialInstanceDynamic>,FApplied> Applied;
    static uint64 NextCleanupFrame=0;
    if(Applied.Num()>512&&GFrameCounter>=NextCleanupFrame)
    {
        NextCleanupFrame=GFrameCounter+120;
        for(auto It=Applied.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    }
    auto& State=Applied.FindOrAdd(M);
    if(State.Ready&&State.Light.Direction.Equals(Light.Direction,.00001f)&&State.Light.Key.Equals(Light.Key,.00001f)
        &&State.Light.Ambient.Equals(Light.Ambient,.00001f))return;
    M->SetVectorParameterValue(TEXT("MainLightDirection"),Light.Direction);
    M->SetVectorParameterValue(TEXT("MainLightRadiance"),Light.Key);
    M->SetVectorParameterValue(TEXT("AmbientRadiance"),Light.Ambient);
    State.Light=Light;State.Ready=true;
}
void Update(UStaticMeshComponent* Proxy,UMaterialInstanceDynamic* M,const FVector& Centre,
    const FVector& Dimensions,const FVector& View,float Density,float Age,int32 Profile)
{
    if(!IsValid(Proxy)||!IsValid(M))return;
    const auto* Parent=M->Parent.Get();
    if(!Parent||(!Parent->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84412/Materials/"))
        &&!Parent->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84413/Materials/"))
        &&!(Profile==2&&Parent->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V85/Materials/")))))
    {
        FrontlineSmokeVolumeV35::Update(Proxy,M,Centre,Dimensions,View,Density,Age);
        return;
    }
    // These pooled secondary clouds retain their owner's exact dimensions,
    // lifetime and opacity. Their density frame is fixed to world XYZ.
    M->SetVectorParameterValue(TEXT("EjectionAxis"),FLinearColor(FVector::UpVector));
    M->SetVectorParameterValue(TEXT("JetRadii"),FLinearColor(FVector(Dimensions.Y,Dimensions.X,Dimensions.Z)*.5f));
    M->SetScalarParameterValue(TEXT("IncidentMode"),FMath::Clamp(float(Profile),0.f,2.f));
    if(Profile==2)M->SetScalarParameterValue(TEXT("VolumeProfile"),1.f);
    ApplySceneLighting(Proxy->GetWorld(),M);
    // The kernel rotates normalized local XY by Seed. Enclose those rotated
    // corners; otherwise the proxy clips nonzero density into a straight face.
    // JetRadii above remains the original physical envelope, not this AABB.
    float Seed=0.f;M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Seed")),Seed);
    if(!FMath::IsFinite(Seed))Seed=0.f;
    const float Angle=Seed*1.618f;
    const float Extent=FMath::Abs(FMath::Cos(Angle))+FMath::Abs(FMath::Sin(Angle));
    const FVector Bounds(Dimensions.X*Extent,Dimensions.Y*Extent,Dimensions.Z);
    float OpticalDensity=Density;
    if(Profile==2)
    {
        // Gas extinction includes a projected-size correction. Compensate
        // the conservative render box, preserving the original optical gain.
        const float Original=FMath::Pow(FMath::Clamp(1200.f/FMath::Max(1.f,FMath::Max(Dimensions.X,Dimensions.Y)),.45f,2.f),.45f);
        const float Box=FMath::Pow(FMath::Clamp(1200.f/FMath::Max(1.f,FMath::Max(Bounds.X,Bounds.Y)),.45f,2.f),.45f);
        OpticalDensity*=Original/Box;
        OpticalDensity*=FrontlineVisualV85::GasOpticalGain(M);
    }
    FrontlineSmokeVolumeV35::Update(Proxy,M,Centre,Bounds,View,OpticalDensity,Age);
}
}
