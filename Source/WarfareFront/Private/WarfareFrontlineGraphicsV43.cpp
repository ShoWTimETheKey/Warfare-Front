#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Components/DirectionalLightComponent.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "RHI.h"
#include "Scalability.h"

namespace
{
float EffectsBudget43=1.f;
void GraphicsVar43(const TCHAR* Name,float Value)
{
    if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))
    {
        // Explicit test/launcher overrides retain ownership of their variables.
        const auto Priority=C->GetFlags()&ECVF_SetByMask;
        if(Priority==ECVF_SetByCommandline || Priority==ECVF_SetByConsole)return;
        if(!FMath::IsNearlyEqual(C->GetFloat(),Value))C->Set(Value,ECVF_SetByCode);
    }
}
}

float UFrontlineRenderSettingsSubsystem::EffectsBudgetV43(){return EffectsBudget43;}

void UFrontlineRenderSettingsSubsystem::InitializeGraphicsPreset()
{
    FTextureMemoryStats Memory;RHIGetTextureMemoryStats(Memory);
    const int64 DedicatedMB=Memory.DedicatedVideoMemory/(1024*1024);
    int32 Value=(GetWorld()->GetFeatureLevel()<ERHIFeatureLevel::SM6 || DedicatedMB<2048)?0:DedicatedMB<=12288?1:2;
    GConfig->GetInt(TEXT("Warfare.Render"),TEXT("GraphicsPreset"),Value,GGameUserSettingsIni);
    FString Arg;if(FParse::Value(FCommandLine::Get(),TEXT("WFGraphicsPreset="),Arg))
    {if(Arg.Equals(TEXT("Low"),ESearchCase::IgnoreCase))Value=0;else if(Arg.Equals(TEXT("Medium"),ESearchCase::IgnoreCase))Value=1;else if(Arg.Equals(TEXT("High"),ESearchCase::IgnoreCase))Value=2;}
    GraphicsPreset=EFrontlineGraphicsPreset(FMath::Clamp(Value,0,2));
    UE_LOG(LogTemp,Display,TEXT("WF_V43_GRAPHICS_INITIAL dedicated_mb=%lld preset=%d feature_level=%d"),DedicatedMB,int32(GraphicsPreset),int32(GetWorld()->GetFeatureLevel()));
    ApplyGraphicsPreset();
}

bool UFrontlineRenderSettingsSubsystem::SetGraphicsPreset(EFrontlineGraphicsPreset Preset,bool bSave)
{
    if(uint8(Preset)>2)return false;
    if(!bApplied||GraphicsPreset!=Preset)
    {SuspendFrameGeneration(1.f);GraphicsPreset=Preset;Apply();}
    if(bSave&&!bNoSave)
    {
        int32 SavedPreset=0;
        if(!GConfig->GetInt(TEXT("Warfare.Render"),TEXT("GraphicsPreset"),SavedPreset,GGameUserSettingsIni)||SavedPreset!=int32(Preset))
        {GConfig->SetInt(TEXT("Warfare.Render"),TEXT("GraphicsPreset"),int32(Preset),GGameUserSettingsIni);++SaveFlushCallsV87;GConfig->Flush(false,GGameUserSettingsIni);}
    }
    return true;
}

void UFrontlineRenderSettingsSubsystem::ApplyGraphicsPreset()
{
    const bool High=GraphicsPreset==EFrontlineGraphicsPreset::High;
    const bool Low=GraphicsPreset==EFrontlineGraphicsPreset::Low;
    const bool SM6=GetWorld()->GetFeatureLevel()>=ERHIFeatureLevel::SM6;
    const bool Lumen=!Low&&SM6;
    const bool RayReflections=High&&SM6;
    EffectsBudget43=High?1.f:Low?.35f:.66f;
    // Every tier keeps the same rules, actors, hit tests, terrain contacts and
    // visibility. Only image formation and cosmetic particle budgets change.
    if(!bGraphicsApplied)
    {
        // PSO collection is automatic for registered mesh components. Bound
        // precompile workers so small laptop CPUs retain time for the game.
        GraphicsVar43(TEXT("r.PSOPrecache.Components"),1);
        bGraphicsApplied=true;
    }
    GraphicsVar43(TEXT("r.DynamicGlobalIlluminationMethod"),Lumen?1:0);
    GraphicsVar43(TEXT("r.ReflectionMethod"),RayReflections?1:2);
    GraphicsVar43(TEXT("r.Lumen.HardwareRayTracing"),High&&GRHISupportsRayTracing?1:0);
    GraphicsVar43(TEXT("r.RayTracing.ForceAllRayTracingEffects"),High?-1:0);
    GraphicsVar43(TEXT("r.Lumen.HardwareRayTracing.LightingMode"),2);
    GraphicsVar43(TEXT("r.Lumen.Reflections.DownsampleFactor"),1);
    GraphicsVar43(TEXT("r.Lumen.Reflections.MaxBounces"),High?2:1);
    GraphicsVar43(TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"),High?16:32);
    GraphicsVar43(TEXT("r.Lumen.ScreenProbeGather.TracingOctahedronResolution"),High?8:6);
    GraphicsVar43(TEXT("r.Lumen.ScreenProbeGather.RadianceCache.ProbeResolution"),High?32:16);
    GraphicsVar43(TEXT("r.Shadow.Virtual.Enable"),High&&SM6?1:0);
    GraphicsVar43(TEXT("r.ShadowQuality"),High?5:Low?2:4);
    GraphicsVar43(TEXT("r.Shadow.MaxResolution"),High?2048:Low?512:1024);
    GraphicsVar43(TEXT("r.Shadow.MaxCSMResolution"),High?2048:Low?1024:2048);
    GraphicsVar43(TEXT("r.Shadow.CSM.MaxCascades"),Low?1:2);
    GraphicsVar43(TEXT("r.ContactShadows"),Low?0:1);
    GraphicsVar43(TEXT("r.SSR.Quality"),Low?1:3);
    GraphicsVar43(TEXT("r.SSR.HalfResSceneColor"),Low?1:0);
    GraphicsVar43(TEXT("r.VolumetricFog"),Low?0:1);
    GraphicsVar43(TEXT("r.VolumetricFog.GridPixelSize"),High?8:16);
    GraphicsVar43(TEXT("r.VolumetricFog.GridSizeZ"),High?128:64);
    GraphicsVar43(TEXT("r.AmbientOcclusionLevels"),Low?1:2);
    GraphicsVar43(TEXT("r.AmbientOcclusionMaxQuality"),Low?30:60);
    GraphicsVar43(TEXT("r.BloomQuality"),High?5:Low?2:4);
    GraphicsVar43(TEXT("r.DepthOfFieldQuality"),0);
    GraphicsVar43(TEXT("r.MotionBlurQuality"),Low?0:3);
    GraphicsVar43(TEXT("r.MaxAnisotropy"),High?16:Low?4:8);
    GraphicsVar43(TEXT("r.Streaming.MipBias"),Low?1:0);
    GraphicsVar43(TEXT("r.Streaming.UsePerTextureBias"),1);
    GraphicsVar43(TEXT("r.Streaming.FramesForFullUpdate"),Low?8:5);
    GraphicsVar43(TEXT("r.SkeletalMeshLODBias"),Low?1:0);
    // Match conventional shadow geometry to the visible geometry. Native Nanite
    // CSM still produces triangle self-shadowing here even at shadow LOD bias 0.
    // High retains Nanite/VSM; lower tiers use the accurate cooked mesh fallback.
    GraphicsVar43(TEXT("r.Nanite"),High&&SM6?1:0);
    FTextureMemoryStats Memory;RHIGetTextureMemoryStats(Memory);
    const int64 MB=Memory.DedicatedVideoMemory/(1024*1024);
    GraphicsVar43(TEXT("r.Streaming.LimitPoolSizeToVRAM"),MB>=2048?1:0);
    const int32 Pool=MB<2048?(Low?512:768):FMath::Clamp(int32(MB*.28),Low?512:1024,High?4096:Low?1024:2048);
    GraphicsVar43(TEXT("r.Streaming.PoolSize"),Pool);
    GraphicsVar43(TEXT("r.MaterialQualityLevel"),Low?0:1);
    for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It)if(It->bUnbound)
    {
        auto& S=It->Settings;S.bOverride_DynamicGlobalIlluminationMethod=true;
        S.DynamicGlobalIlluminationMethod=Lumen?EDynamicGlobalIlluminationMethod::Lumen:EDynamicGlobalIlluminationMethod::None;
        S.bOverride_ReflectionMethod=true;S.ReflectionMethod=RayReflections?EReflectionMethod::Lumen:EReflectionMethod::ScreenSpace;
        S.bOverride_LumenSceneLightingQuality=true;S.LumenSceneLightingQuality=High?1.f:.5f;
        S.bOverride_LumenFinalGatherQuality=true;S.LumenFinalGatherQuality=High?1.f:.5f;
        S.bOverride_LumenReflectionQuality=true;S.LumenReflectionQuality=1.f;
    }
    for(TActorIterator<AActor> It(GetWorld());It;++It)if(auto* Sun=It->FindComponentByClass<UDirectionalLightComponent>())
    {
        const int32 Cascades=Low?1:2;
        if(Sun->DynamicShadowDistanceMovableLight!=18000.f||Sun->DynamicShadowCascades!=Cascades)
        {Sun->DynamicShadowDistanceMovableLight=18000.f;Sun->DynamicShadowCascades=Cascades;Sun->MarkRenderStateDirty();}
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V43_GRAPHICS_APPLY tier=%d lumen=%d hardware_rt=%d lumen_reflections=%d virtual_shadows=%d fx_scale=%.2f texture_pool_mb=%d"),int32(GraphicsPreset),Lumen,High&&GRHISupportsRayTracing,RayReflections,High&&SM6,EffectsBudget43,Pool);
}
