#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineFXV47.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineVisualV85.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"

void FrontlineFXV47::Prewarm()
{
    // These shared objects live for the session, not for an individual burst.
    // The atlas has one non-streaming mip (eight MiB), so no forced mip timer
    // or load/unload churn is required when a support mission fires later.
    static TArray<TStrongObjectPtr<UObject>> Resources;
    if(Resources.Num()==8)return;
    for(const TCHAR* Path:{
        TEXT("/Game/Warfare/Field/V47/Textures/T_FlowDensity64_V47.T_FlowDensity64_V47"),
        TEXT("/Game/Warfare/Field/V47/Materials/M_SmokeVolume_V47.M_SmokeVolume_V47"),
        TEXT("/Game/Warfare/Field/V47/Materials/M_BlastEjecta_V47.M_BlastEjecta_V47"),
        TEXT("/Game/Warfare/Field/V47/Materials/M_BlastRoll_V47.M_BlastRoll_V47"),
        TEXT("/Game/Warfare/Field/V45/Materials/M_BlastFlash_V45.M_BlastFlash_V45"),
        TEXT("/Game/Warfare/Field/V828/Materials/M_BlastVolume_V828.M_BlastVolume_V828"),
        TEXT("/Game/Warfare/Field/V828/Materials/M_BloodVolume_V828.M_BloodVolume_V828"),
        TEXT("/Game/Warfare/Field/V828/Materials/M_BloodDeposit_V828.M_BloodDeposit_V828")})
        if(UObject* Asset=LoadObject<UObject>(nullptr,Path))
        {
            bool Held=false;for(const auto& Existing:Resources)Held|=Existing.Get()==Asset;
            if(!Held)Resources.Emplace(Asset);
        }
}

UMaterialInterface* FrontlineSmokeVolumeV35::MaterialV40()
{
    if(auto* Bounded=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V836/Materials/M_SmokeVolume_V836.M_SmokeVolume_V836")))return Bounded;
    if(auto* Flow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V47/Materials/M_SmokeVolume_V47.M_SmokeVolume_V47")))return Flow;
    if(auto* Rounded=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V46/Materials/M_SmokeVolume_V46.M_SmokeVolume_V46")))return Rounded;
    if(auto* Stable=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V45/Materials/M_SmokeVolume_V45.M_SmokeVolume_V45")))return Stable;
    if(auto* Tiered=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V43/Materials/M_SmokeVolume_V43.M_SmokeVolume_V43")))return Tiered;
    auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V40/Materials/M_SmokeVolume_V40.M_SmokeVolume_V40"));
    return M?M:LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V35/Materials/M_SmokeVolume_V35.M_SmokeVolume_V35"));
}

UMaterialInterface* FrontlineSmokeVolumeV35::GasMaterialV8215()
{
    if(!FrontlineVisualV85::Baseline())
        if(auto* Gas=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V85/Materials/M_GasVolume_V85.M_GasVolume_V85"),nullptr,LOAD_NoWarn|LOAD_Quiet))return Gas;
    if(auto* Bounded=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V836/Materials/M_GasVolume_V836.M_GasVolume_V836")))return Bounded;
    if(auto* Gas=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8215/Materials/M_GasVolume_V8215.M_GasVolume_V8215")))return Gas;
    return MaterialV40();
}

void FrontlineSmokeVolumeV35::Update(UStaticMeshComponent* Proxy,UMaterialInstanceDynamic* Material,
    const FVector& Centre,const FVector& Dimensions,const FVector& ViewForward,float Density,float Age)
{
    if(!Proxy||!Material)return;
    // Validate before sending transforms or parameters to the renderer. A
    // transient volume proxy has no physical opaque surface in secondary rays.
    const bool Valid=!Centre.ContainsNaN()&&!Dimensions.ContainsNaN()&&Dimensions.GetMin()>1.f
        &&Dimensions.GetMax()<18000.f&&FMath::IsFinite(Density)&&FMath::IsFinite(Age)&&!ViewForward.ContainsNaN();
    if(!Valid){Proxy->SetVisibility(false);return;}
    Proxy->SetVisibleInRayTracing(false);Proxy->SetAffectDistanceFieldLighting(false);
    Proxy->SetAffectDynamicIndirectLighting(false);Proxy->SetCastShadow(false);
    Proxy->bVisibleInReflectionCaptures=false;Proxy->bVisibleInRealTimeSkyCaptures=false;
    Proxy->SetWorldLocation(Centre);
    Proxy->SetWorldRotation(FRotator::ZeroRotator);
    Proxy->SetWorldScale3D(Dimensions/100.f); // Engine cube is exactly 100 cm.
    Material->SetVectorParameterValue(TEXT("VolumeCenter"),FLinearColor(Centre));
    Material->SetVectorParameterValue(TEXT("VolumeRadii"),FLinearColor(Dimensions*.5));
    Material->SetVectorParameterValue(TEXT("ViewForward"),FLinearColor(ViewForward.GetSafeNormal(.0001f,FVector(0,-1,-.28f).GetSafeNormal())));
    Material->SetScalarParameterValue(TEXT("Density"),FMath::Clamp(Density,0.f,4.f));
    Material->SetScalarParameterValue(TEXT("Age"),FMath::Clamp(Age,0.f,120.f));
    const float Budget=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43();
    Material->SetScalarParameterValue(TEXT("QualitySteps"),Budget<.5f?5:Budget<.9f?8:12);
    // Publish visibility after the kernel's matching centre/radii/density.
    // The first submitted proxy must never use constructor parameter defaults.
    Proxy->SetVisibility(Density>.01f);
}
