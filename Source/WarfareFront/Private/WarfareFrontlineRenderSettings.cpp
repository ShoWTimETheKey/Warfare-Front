#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineFrameCap.h"
#include "WarfareFrontline.h"
#include "DLSSLibrary.h"
#include "StreamlineLibraryDLSSG.h"
#include "StreamlineLibraryReflex.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformCrashContext.h"
#include "HAL/CriticalSection.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Misc/ScopeLock.h"
#include "RenderCommandFence.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "TemporalUpscaler.h"
#include "UnrealClient.h"
#include "DynamicRHI.h"
#include "RHIStats.h"

struct FFrontlineRenderObservation
{
    FCriticalSection Mutex;
    FString Upscaler=TEXT("not_observed");
    uint64 ViewFamilies=0,DLSSViewFamilies=0,RRViewFamilies=0;
    bool bOrthographic=false;
};

namespace
{
// FG and Reflex are process-wide SDK state. Keep this barrier across world
// replacement so a new subsystem cannot disable Reflex ahead of an old Present.
// All fields are game-thread-only except the real render-family counter.
struct FFrontlineReflexDrain
{
    TAtomic<uint64> RenderedFamilies{0};
    uint64 StoppedAt=0;
    bool bNeeded=false,bFenceQueued=false;
    FRenderCommandFence Fence;
};
FFrontlineReflexDrain& ReflexDrain()
{ static FFrontlineReflexDrain State;return State; }
FFrontlineFrameCapController& PresentationLimiter()
{ static FFrontlineFrameCapController State;return State; }

constexpr const TCHAR* RenderSection=TEXT("Warfare.Render");
const TCHAR* QualityNames[]={TEXT("Native"),TEXT("DLAA"),TEXT("Quality"),TEXT("Balanced"),TEXT("Performance")};
const TCHAR* FrameNames[]={TEXT("Off"),TEXT("X2"),TEXT("X3"),TEXT("X4"),TEXT("X5"),TEXT("X6"),TEXT("Dynamic")};
const TCHAR* ReflexNames[]={TEXT("Off"),TEXT("On"),TEXT("Boost")};
UDLSSMode ToDLSS(EFrontlineRenderQuality Value)
{
    switch(Value)
    {
    case EFrontlineRenderQuality::DLAA:return UDLSSMode::DLAA;
    case EFrontlineRenderQuality::Quality:return UDLSSMode::Quality;
    case EFrontlineRenderQuality::Balanced:return UDLSSMode::Balanced;
    case EFrontlineRenderQuality::Performance:return UDLSSMode::Performance;
    default:return UDLSSMode::Off;
    }
}
EStreamlineDLSSGMode ToDLSSG(EFrontlineFrameGeneration Value)
{
    switch(Value)
    {
    case EFrontlineFrameGeneration::X2:return EStreamlineDLSSGMode::On2X;
    case EFrontlineFrameGeneration::X3:return EStreamlineDLSSGMode::On3X;
    case EFrontlineFrameGeneration::X4:return EStreamlineDLSSGMode::On4X;
    case EFrontlineFrameGeneration::X5:return EStreamlineDLSSGMode::On5X;
    case EFrontlineFrameGeneration::X6:return EStreamlineDLSSGMode::On6X;
    case EFrontlineFrameGeneration::Dynamic:return EStreamlineDLSSGMode::OnDynamic;
    default:return EStreamlineDLSSGMode::Off;
    }
}
EStreamlineReflexMode ToReflex(EFrontlineReflexMode Value)
{
    return Value==EFrontlineReflexMode::Boost?EStreamlineReflexMode::Boost:
        Value==EFrontlineReflexMode::On?EStreamlineReflexMode::Enabled:EStreamlineReflexMode::Off;
}
float ConsoleFloat(const TCHAR* Name,float Fallback=0)
{
    const auto* Var=IConsoleManager::Get().FindConsoleVariable(Name); return Var?Var->GetFloat():Fallback;
}
void SetConsoleFloat(const TCHAR* Name,float Value)
{
    if(auto* Var=IConsoleManager::Get().FindConsoleVariable(Name))
        Var->SetWithCurrentPriority(Value,NAME_None,ECVF_SetByConsole,ECVF_SetByGameSetting);
}
void SeedPluginPrioritiesV43()
{
    // UE 5.8 reserves constructor priority for registration. Older SDK helpers
    // preserve the current priority, so give untouched defaults a legal floor
    // without changing their values or superseding explicit player overrides.
    for(const TCHAR* Name:{TEXT("r.Lumen.Reflections.BilateralFilter"),TEXT("r.Streamline.DLSSG.Enable"),TEXT("r.Streamline.DLSSG.FramesToGenerate"),TEXT("t.Streamline.Reflex.LastFrameLimitUs")})
        if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))
            if((C->GetFlags()&ECVF_SetByMask)<ECVF_SetByScalability)C->Set(C->GetFloat(),ECVF_SetByGameSetting);
}
FString DLSSReason(UDLSSSupport Support)
{
    switch(Support)
    {
    case UDLSSSupport::Supported:return FString();
    case UDLSSSupport::NotSupportedDriverOutOfDate:return TEXT("请更新显卡驱动");
    case UDLSSSupport::NotSupportedOperatingSystemOutOfDate:return TEXT("请更新 Windows");
    case UDLSSSupport::NotSupportedIncompatibleHardware:return TEXT("需要 NVIDIA RTX 显卡");
    case UDLSSSupport::NotSupportedIncompatibleAPICaptureToolActive:return TEXT("请关闭图形捕获工具");
    default:return TEXT("当前渲染模式不可用");
    }
}
FString StreamlineReason(EStreamlineFeatureSupport Support)
{
    switch(Support)
    {
    case EStreamlineFeatureSupport::Supported:return FString();
    case EStreamlineFeatureSupport::NotSupportedDriverOutOfDate:return TEXT("请更新显卡驱动");
    case EStreamlineFeatureSupport::NotSupportedOperatingSystemOutOfDate:return TEXT("请更新 Windows");
    case EStreamlineFeatureSupport::NotSupportedHardewareSchedulingDisabled:return TEXT("需开启 Windows GPU 调度");
    case EStreamlineFeatureSupport::NotSupportedIncompatibleHardware:return TEXT("当前显卡不支持");
    case EStreamlineFeatureSupport::NotSupportedIncompatibleAPICaptureToolActive:return TEXT("请关闭图形捕获工具");
    default:return TEXT("当前渲染模式不可用");
    }
}

// FWorldSceneViewExtension filters the owning world on the game thread. Reading
// NVIDIA's debug name must happen on the render thread: it reads render-thread CVars.
// The frame owns the view-family interface; only copied values cross back to the game thread.
class FFrontlineUpscalerObserver final : public FWorldSceneViewExtension
{
public:
    FFrontlineUpscalerObserver(const FAutoRegister& AutoRegister,UWorld* InWorld,
        TSharedPtr<FFrontlineRenderObservation,ESPMode::ThreadSafe> InObservation)
        :FWorldSceneViewExtension(AutoRegister,InWorld),Observation(MoveTemp(InObservation)) {}
    virtual int32 GetPriority() const override { return -10000; }
    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily&,FSceneView&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void PreRenderViewFamily_RenderThread(FRDGBuilder&,FSceneViewFamily& Family) override
    {
        if (!Family.Scene || Family.Views.IsEmpty())return;
        if (Family.Views[0]->bIsSceneCapture)return;
        ReflexDrain().RenderedFamilies.IncrementExchange();
        const auto* Upscaler=Family.GetTemporalUpscalerInterface();
        const FString UpscalerName=Upscaler?Upscaler->GetDebugName():TEXT("engine_temporal_upscaler");
        FScopeLock Lock(&Observation->Mutex);
        Observation->Upscaler=UpscalerName;
        Observation->bOrthographic=!Family.Views[0]->IsPerspectiveProjection();
        ++Observation->ViewFamilies;
        if(Observation->Upscaler.Contains(TEXT("DLSS")))++Observation->DLSSViewFamilies;
        if(Observation->Upscaler.Contains(TEXT("DLSS-RR")))++Observation->RRViewFamilies;
    }
private:
    TSharedPtr<FFrontlineRenderObservation,ESPMode::ThreadSafe> Observation;
};
}

UFrontlineRenderSettingsSubsystem* UFrontlineRenderSettingsSubsystem::Get(UWorld* World)
{ return World?World->GetSubsystem<UFrontlineRenderSettingsSubsystem>():nullptr; }
bool UFrontlineRenderSettingsSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{ return WorldType==EWorldType::Game || WorldType==EWorldType::PIE; }
TStatId UFrontlineRenderSettingsSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(UFrontlineRenderSettingsSubsystem,STATGROUP_Tickables); }

void UFrontlineRenderSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // This game has one main graphics viewport (no VR or split screen). Keep
    // the SDK scene, UI/backbuffer and status queries on the same stable handle.
    for(const auto& Setting:{TPair<const TCHAR*,int32>(TEXT("r.Streamline.ViewIdOverride"),1),TPair<const TCHAR*,int32>(TEXT("r.Streamline.ViewIndexToTag"),0)})
        if(auto* C=IConsoleManager::Get().FindConsoleVariable(Setting.Key))
            if((C->GetFlags()&ECVF_SetByMask)<ECVF_SetByCommandline)C->Set(Setting.Value,ECVF_SetByCode);
    bLastPresentationFocusV844=FApp::HasFocus();
    WillDeactivateV844=FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&UFrontlineRenderSettingsSubsystem::PresentationFocusLostV844);
    WillBackgroundV844=FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&UFrontlineRenderSettingsSubsystem::PresentationFocusLostV844);
    ReactivatedV844=FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this,&UFrontlineRenderSettingsSubsystem::PresentationFocusRestoredV844);
    ForegroundV844=FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,&UFrontlineRenderSettingsSubsystem::PresentationFocusRestoredV844);
    SeedPluginPrioritiesV43();
    // UE 5.8 PC skin-cache async dispatch races skeletal-mesh streaming.
    // Epic Pro Support, 2026-08-19/20, topic 2831517: corrupt first-visible
    // vertices affect both the base pass and BLAS, and can persist until a
    // mesh update. Keep skinning/tangents, geometry and RT quality unchanged;
    // use the supported graphics queue. Do not re-enable from the old V33
    // timing experiment, player config or a retained command-line override.
    if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.AsyncCompute")))
    {
        C->SetWithCurrentPriority(0,NAME_None,ECVF_SetByConsole,ECVF_SetByCode);
        FGenericCrashContext::SetGameData(TEXT("WarfareFront.SkinCacheAsyncCompute"),FString::FromInt(C->GetInt()));
        UE_LOG(LogTemp,Display,TEXT("WF_V851_PC_SKIN_CACHE async=%d"),C->GetInt());
    }
    bNoSave=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")) || FParse::Param(FCommandLine::Get(),TEXT("WFV25HDRReview"));
    // UGameEngine::PreExit also saves UGameUserSettings unconditionally. A
    // temporary compatibility launch must survive that path as well as our
    // menu setters. Protect only this config branch; campaign JSON saves keep
    // their normal behaviour. UE's SaveBranch/Write honour this native flag.
    if(bNoSave)
        if(auto* Branch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString()))
            Branch->InMemoryFile.NoSave=true;
    MigrateV23DisplayDefaults();
    MigrateV24RenderDefaults();
    MigrateV30RenderDefaults();
    InitializeGraphicsPreset();
    int32 Q=int32(EFrontlineRenderQuality::Quality),F=1,R=int32(EFrontlineReflexMode::On);
    GConfig->GetInt(RenderSection,TEXT("Quality"),Q,GGameUserSettingsIni);
    GConfig->GetInt(RenderSection,TEXT("FrameGeneration"),F,GGameUserSettingsIni);
    GConfig->GetInt(RenderSection,TEXT("Reflex"),R,GGameUserSettingsIni);
    GConfig->GetBool(RenderSection,TEXT("RayReconstruction"),bRayReconstruction,GGameUserSettingsIni);
    Quality=EFrontlineRenderQuality(FMath::Clamp(Q,0,4));
    FrameGeneration=EFrontlineFrameGeneration(FMath::Clamp(F,0,6));
    Reflex=EFrontlineReflexMode(FMath::Clamp(R,0,2));
    FString Arg;
    if(FParse::Value(FCommandLine::Get(),TEXT("WFRenderQuality="),Arg))
        for(int32 I=0;I<UE_ARRAY_COUNT(QualityNames);++I)
            if(Arg.Equals(QualityNames[I],ESearchCase::IgnoreCase))Quality=EFrontlineRenderQuality(I);
    if(FParse::Value(FCommandLine::Get(),TEXT("WFRenderFG="),Arg))
        for(int32 I=0;I<UE_ARRAY_COUNT(FrameNames);++I)
            if(Arg.Equals(FrameNames[I],ESearchCase::IgnoreCase))FrameGeneration=EFrontlineFrameGeneration(I);
    if(FParse::Value(FCommandLine::Get(),TEXT("WFRenderReflex="),Arg))
        for(int32 I=0;I<UE_ARRAY_COUNT(ReflexNames);++I)
            if(Arg.Equals(ReflexNames[I],ESearchCase::IgnoreCase))Reflex=EFrontlineReflexMode(I);
    int32 RR=0;if(FParse::Value(FCommandLine::Get(),TEXT("WFRenderRR="),RR))bRayReconstruction=RR!=0;
    if(FrameGeneration!=EFrontlineFrameGeneration::Off && Reflex==EFrontlineReflexMode::Off)Reflex=EFrontlineReflexMode::On;
    Observation=MakeShared<FFrontlineRenderObservation,ESPMode::ThreadSafe>();
    ViewObserver=FSceneViewExtensions::NewExtension<FFrontlineUpscalerObserver>(GetWorld(),Observation);
    InitializeHDR();
    // The denser field kit exceeds UE's 400 MB default BLAS residency pool.
    // Reserve a bounded share of detected dedicated VRAM, without changing an
    // explicit console/command-line override or assuming this machine's GPU.
    if(auto* Pool=IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB")))
    {
        FTextureMemoryStats Memory;RHIGetTextureMemoryStats(Memory);
        if(Memory.DedicatedVideoMemory>0&&Pool->GetInt()==400&&(Pool->GetFlags()&ECVF_SetByMask)<ECVF_SetByCommandline)
        {
            // V51's measured active wide field reaches about 1.5 GB of BLAS.
            // Budget one eighth, capped at 2 GB; smaller GPUs keep a bounded
            // allowance and Medium/Low continue to disable hardware tracing.
            const int32 MiB=FMath::Clamp(int32(Memory.DedicatedVideoMemory/(1024*1024*8)),400,2048);
            Pool->Set(MiB,ECVF_SetByCode);
            UE_LOG(LogTemp,Display,TEXT("WF_V40_RT_GEOMETRY_POOL dedicated_vram_mib=%lld pool_mib=%d"),Memory.DedicatedVideoMemory/(1024*1024),MiB);
        }
    }
}

void UFrontlineRenderSettingsSubsystem::Deinitialize()
{
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(WillDeactivateV844);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(WillBackgroundV844);
    FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ReactivatedV844);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundV844);
    PendingOutputChangesV844.Empty();
    ShutdownHDR();
    if(bApplied)
    {
        StopFrameGeneration();
        UDLSSLibrary::EnableDLSSRR(false);
        UDLSSLibrary::EnableDLSS(false);
        // No new world frames may follow destruction. Preserve the dependency
        // through SDK teardown, or until the next world's drain finishes.
        if(!ReflexDrain().bNeeded)UStreamlineLibraryReflex::SetReflexMode(EStreamlineReflexMode::Off);
    }
    ViewObserver.Reset();Observation.Reset();
    SetConsoleFloat(TEXT("t.Streamline.Reflex.PresentationLimit"),0.f);
    Super::Deinitialize();
}

FFrontlineRenderCapabilities UFrontlineRenderSettingsSubsystem::GetCapabilities() const
{
    FFrontlineRenderCapabilities Result;
    const auto SR=UDLSSLibrary::QueryDLSSSupport();
    const auto RR=UDLSSLibrary::QueryDLSSRRSupport();
    const auto FG=UStreamlineLibraryDLSSG::QueryDLSSGSupport();
    const auto RF=UStreamlineLibraryReflex::QueryReflexSupport();
    Result.bSuperResolution=SR==UDLSSSupport::Supported;
    Result.bRayReconstruction=RR==UDLSSSupport::Supported && UDLSSLibrary::IsRayTracingAvailable();
    Result.bFrameGeneration=FG==EStreamlineFeatureSupport::Supported;
    Result.bReflex=RF==EStreamlineFeatureSupport::Supported;
    Result.bFrameGenerationVSync=Result.bFrameGeneration && UStreamlineLibraryDLSSG::GetDLSSGIsVsyncSupportAvailable();
    Result.SuperResolutionReason=DLSSReason(SR);Result.RayReconstructionReason=DLSSReason(RR);
    Result.FrameGenerationReason=StreamlineReason(FG);Result.ReflexReason=StreamlineReason(RF);
    const auto* Viewport=GetWorld()->GetGameViewport();
    const int32 OutputFormat=Viewport && Viewport->Viewport?int32(Viewport->Viewport->GetDisplayOutputFormat()):0;
    if(OutputFormat==5 || OutputFormat==6)
    {Result.bFrameGeneration=false;Result.FrameGenerationReason=TEXT("当前 HDR scRGB 输出不支持帧生成");}
    if(Result.bFrameGeneration && ConsoleFloat(TEXT("r.VSync"))>0 && !Result.bFrameGenerationVSync)
    { Result.bFrameGeneration=false;Result.FrameGenerationReason=TEXT("帧生成需要关闭垂直同步"); }
    if(RR==UDLSSSupport::Supported && !Result.bRayReconstruction)Result.RayReconstructionReason=TEXT("需要开启光线追踪");
    // Stock UE does not supply NvRTX's additional RR guide buffers. Keep this an
    // explicit user choice; hardware support alone is not an image-quality approval.
    if(Result.bSuperResolution)
        for(int32 I=1;I<UE_ARRAY_COUNT(QualityNames);++I)
            if(UDLSSLibrary::IsDLSSModeSupported(ToDLSS(EFrontlineRenderQuality(I))))Result.Qualities.Add(EFrontlineRenderQuality(I));
    if(Result.bFrameGeneration && Result.bReflex)
        for(int32 I=1;I<UE_ARRAY_COUNT(FrameNames);++I)
            if(UStreamlineLibraryDLSSG::IsDLSSGModeSupported(ToDLSSG(EFrontlineFrameGeneration(I))))Result.FrameGenerationModes.Add(EFrontlineFrameGeneration(I));
    if(Result.bReflex)
        for(int32 I=1;I<UE_ARRAY_COUNT(ReflexNames);++I)
            if(UStreamlineLibraryReflex::IsReflexModeSupported(ToReflex(EFrontlineReflexMode(I))))Result.ReflexModes.Add(EFrontlineReflexMode(I));
    if(auto* PC=Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController()))
        if(PC->BattleCamera)Result.bOrthographic=PC->BattleCamera->GetCameraComponent()->ProjectionMode==ECameraProjectionMode::Orthographic;
    return Result;
}

bool UFrontlineRenderSettingsSubsystem::ShouldRunFrameGeneration() const
{
    if(FrameGeneration==EFrontlineFrameGeneration::Off || !FApp::HasFocus() || ReflexDrain().bNeeded || !PendingOutputChangesV844.IsEmpty() || FPlatformTime::Seconds()<FrameGenerationResumeAt || UGameplayStatics::IsGamePaused(GetWorld()))return false;
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const auto* PC=GetWorld()->GetFirstPlayerController();
    const auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
    return Battle && Battle->Phase==EFrontlinePhase::Fighting && HUD && HUD->GetPresentationPage()==3;
}

void UFrontlineRenderSettingsSubsystem::Apply()
{
    ++ApplyCallsV87;
    SeedPluginPrioritiesV43();
    ApplyGraphicsPreset();
    const auto Caps=GetCapabilities();
    if(!Caps.Qualities.Contains(Quality))Quality=EFrontlineRenderQuality::Native;
    if(!Caps.ReflexModes.Contains(Reflex))Reflex=EFrontlineReflexMode::Off;
    const bool bSR=Quality!=EFrontlineRenderQuality::Native && Caps.Qualities.Contains(Quality);
    bool Supported=false,Fixed=false;float Min=100,Max=100,Sharpness=0;
    RequestedScreenPercentage=100;
    int32 Width=3840,Height=2160;
    if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetViewportSize(Width,Height);
    if(bSR)UDLSSLibrary::GetDLSSModeInformation(ToDLSS(Quality),FVector2D(FMath::Max(1,Width),FMath::Max(1,Height)),Supported,RequestedScreenPercentage,Fixed,Min,Max,Sharpness);
    if(!bSR || !Supported)RequestedScreenPercentage=100;
    UDLSSLibrary::EnableDLSSRR(false);
    UDLSSLibrary::EnableDLSS(bSR && Supported);
    SetConsoleFloat(TEXT("r.ScreenPercentage"),RequestedScreenPercentage);
    if(bRayReconstruction && GraphicsPreset==EFrontlineGraphicsPreset::High && bSR && Supported && Caps.bRayReconstruction)UDLSSLibrary::EnableDLSSRR(true);
    bRequestedFrameGenerationSupported=Caps.FrameGenerationModes.Contains(FrameGeneration);
    const auto Generation=ShouldRunFrameGeneration() && bRequestedFrameGenerationSupported?FrameGeneration:EFrontlineFrameGeneration::Off;
    const EFrontlineReflexMode DesiredReflex=Generation!=EFrontlineFrameGeneration::Off && Reflex==EFrontlineReflexMode::Off?EFrontlineReflexMode::On:Reflex;
    ApplyFrameGenerationAndReflex(Generation,Caps.ReflexModes.Contains(DesiredReflex)?DesiredReflex:EFrontlineReflexMode::Off);
    bApplied=true;
    UE_LOG(LogTemp,Display,TEXT("V22 rendering quality=%s supported=%d percent=%.3f RR=%d FG=%s orthographic=%d"),
        QualityNames[int32(Quality)],bSR&&Supported,RequestedScreenPercentage,UDLSSLibrary::IsDLSSRREnabled(),FrameNames[int32(EffectiveFrameGeneration)],Caps.bOrthographic);
}

void UFrontlineRenderSettingsSubsystem::StopFrameGeneration()
{
    SeedPluginPrioritiesV43();
    auto& Drain=ReflexDrain();
    if(EffectiveFrameGeneration!=EFrontlineFrameGeneration::Off || UStreamlineLibraryDLSSG::GetDLSSGMode()!=EStreamlineDLSSGMode::Off)
    {
        Drain.StoppedAt=Drain.RenderedFamilies.Load();
        Drain.bNeeded=true;Drain.bFenceQueued=false;
    }
    UStreamlineLibraryDLSSG::SetDLSSGMode(EStreamlineDLSSGMode::Off);
    EffectiveFrameGeneration=EFrontlineFrameGeneration::Off;
    TickPresentationFrameLimit();
}

void UFrontlineRenderSettingsSubsystem::ApplyFrameGenerationAndReflex(EFrontlineFrameGeneration Generation,EFrontlineReflexMode ReflexMode)
{
    auto& Drain=ReflexDrain();
    if(Generation!=EFrontlineFrameGeneration::Off)
    {
        // A focus return or quick second click must not cancel a submitted Off
        // pass. Keep Reflex alive while the three real families/RHI fence retire.
        if(Drain.bNeeded||!PendingOutputChangesV844.IsEmpty())
        {UStreamlineLibraryReflex::SetReflexMode(ToReflex(ReflexMode==EFrontlineReflexMode::Off?EFrontlineReflexMode::On:ReflexMode));return;}
        bReflexDisablePending=false;
        // Enabling retains the opposite ordering: Reflex must precede FG.
        UStreamlineLibraryReflex::SetReflexMode(ToReflex(ReflexMode==EFrontlineReflexMode::Off?EFrontlineReflexMode::On:ReflexMode));
        UStreamlineLibraryDLSSG::SetDLSSGMode(ToDLSSG(Generation));
        EffectiveFrameGeneration=Generation;TickPresentationFrameLimit();return;
    }
    StopFrameGeneration();
    bReflexDisablePending=ReflexMode==EFrontlineReflexMode::Off && Drain.bNeeded;
    if(!bReflexDisablePending)UStreamlineLibraryReflex::SetReflexMode(ToReflex(ReflexMode));
}

void UFrontlineRenderSettingsSubsystem::TickDeferredReflex()
{
    auto& Drain=ReflexDrain();
    if(!Drain.bNeeded || EffectiveFrameGeneration!=EFrontlineFrameGeneration::Off)return;
    // The SDK receives Off from a later RDG/RHI pass, not from its GT CVar
    // setter. Observe three actual scene frames, including menus/paused worlds,
    // then retire preceding RHI submission once. No per-frame flush or sleep.
    if(Drain.RenderedFamilies.Load()<Drain.StoppedAt+3)return;
    if(!Drain.bFenceQueued)
    {
        Drain.Fence.BeginFence(FRenderCommandFence::ESyncDepth::RHIThread);
        Drain.bFenceQueued=true;return;
    }
    if(!Drain.Fence.IsFenceComplete())return;
    Drain.bNeeded=false;Drain.bFenceQueued=false;
    if(bReflexDisablePending && Reflex==EFrontlineReflexMode::Off && FrameGeneration==EFrontlineFrameGeneration::Off)
    {
        UStreamlineLibraryReflex::SetReflexMode(EStreamlineReflexMode::Off);
        UE_LOG(LogTemp,Display,TEXT("V23 Reflex disabled after FG Off: three rendered families and RHI fence completed"));
    }
    bReflexDisablePending=false;
}

void UFrontlineRenderSettingsSubsystem::Tick(float DeltaTime)
{
    if(!GetWorld()->GetFirstPlayerController())return;
    TickPresentationSafetyV844();
    TickDeferredReflex();
    if(!ReflexDrain().bNeeded&&!PendingOutputChangesV844.IsEmpty())
    {
        auto Mutations=MoveTemp(PendingOutputChangesV844);PendingOutputChangesV844.Empty();
        FrameGenerationResumeAt=FMath::Max(FrameGenerationResumeAt,FPlatformTime::Seconds()+1.);
        for(auto& Item:Mutations){if(Item.Value){Item.Value();++OutputMutationsV844;}}
    }
    TickMigratedDisplayDefaults();
    TickActiveDisplay();
    TickHDRCalibration();
    if(!bApplied){Apply();return;}
    const auto Desired=bRequestedFrameGenerationSupported && ShouldRunFrameGeneration()?FrameGeneration:EFrontlineFrameGeneration::Off;
    if(Desired!=EffectiveFrameGeneration)
    {
        const auto Caps=GetCapabilities();
        const auto Supported=Caps.FrameGenerationModes.Contains(Desired)?Desired:EFrontlineFrameGeneration::Off;
        if(Supported!=EffectiveFrameGeneration)
        {
            const auto DesiredReflex=Supported!=EFrontlineFrameGeneration::Off && Reflex==EFrontlineReflexMode::Off?EFrontlineReflexMode::On:Reflex;
            ApplyFrameGenerationAndReflex(Supported,Caps.ReflexModes.Contains(DesiredReflex)?DesiredReflex:EFrontlineReflexMode::Off);
        }
    }
    TickDeferredReflex();
    TickPresentationFrameLimit();
}

void UFrontlineRenderSettingsSubsystem::TickPresentationFrameLimit()
{
    auto& Limiter=PresentationLimiter();const double Now=FPlatformTime::Seconds();
    const int32 Mode=int32(EffectiveFrameGeneration);int32 MaximumRatio=6;
    if(Mode==int32(EFrontlineFrameGeneration::Dynamic) && Mode!=Limiter.Mode)
    {
        MaximumRatio=2;
        for(auto Available:GetCapabilities().FrameGenerationModes)
            if(Available>EFrontlineFrameGeneration::Off && Available<EFrontlineFrameGeneration::Dynamic)
                MaximumRatio=FMath::Max(MaximumRatio,int32(Available)+1);
    }
    Limiter.Select(Mode,MaximumRatio,ReflexDrain().bNeeded,Now);
    uint64 RenderFrame=0;if(Observation){FScopeLock Lock(&Observation->Mutex);RenderFrame=Observation->ViewFamilies;}
    const auto Timing=GetFrameTiming();
    if(Timing.bPresentEstimateValid)Limiter.Observe(Timing.FramesPresentedPerRendered,RenderFrame,Now);
    const float Total=GetFrameRateLimit();if(Total<=0)return;
    // UE's fallback limiter counts rendered frames. Reflex owns presentation
    // pacing, including FG, and must receive the user's total rate separately.
    // Feeding the divided fallback to Reflex also caps generated output to it.
    const float Internal=Limiter.RenderLimit(Total);
    if(!FMath::IsNearlyEqual(ConsoleFloat(TEXT("t.MaxFPS")),Internal,.01f))SetConsoleFloat(TEXT("t.MaxFPS"),Internal);
    static const bool bExplicitAB=FParse::Param(FCommandLine::Get(),TEXT("WFV24DisplayReview")) &&
        FParse::Param(FCommandLine::Get(),TEXT("WFV24FrameCapOnly")) &&
        !FParse::Param(FCommandLine::Get(),TEXT("WFV24FrameCapProduction"));
    if(!bExplicitAB && !FMath::IsNearlyEqual(ConsoleFloat(TEXT("t.Streamline.Reflex.PresentationLimit")),Total,.01f))
        SetConsoleFloat(TEXT("t.Streamline.Reflex.PresentationLimit"),Total);
    if(!FMath::IsNearlyEqual(ConsoleFloat(TEXT("r.Streamline.DLSSG.DynamicTargetFrameRate")),Total,.01f))
        SetConsoleFloat(TEXT("r.Streamline.DLSSG.DynamicTargetFrameRate"),Total);
}

void UFrontlineRenderSettingsSubsystem::Save()
{
    if(bNoSave)return;
    int32 SavedQuality=0,SavedGeneration=0,SavedReflex=0;bool SavedRR=false;
    const bool Unchanged=GConfig->GetInt(RenderSection,TEXT("Quality"),SavedQuality,GGameUserSettingsIni)
        &&GConfig->GetInt(RenderSection,TEXT("FrameGeneration"),SavedGeneration,GGameUserSettingsIni)
        &&GConfig->GetInt(RenderSection,TEXT("Reflex"),SavedReflex,GGameUserSettingsIni)
        &&GConfig->GetBool(RenderSection,TEXT("RayReconstruction"),SavedRR,GGameUserSettingsIni)
        &&SavedQuality==int32(Quality)&&SavedGeneration==int32(FrameGeneration)&&SavedReflex==int32(Reflex)&&SavedRR==bRayReconstruction;
    if(Unchanged)return;
    GConfig->SetInt(RenderSection,TEXT("Quality"),int32(Quality),GGameUserSettingsIni);
    GConfig->SetInt(RenderSection,TEXT("FrameGeneration"),int32(FrameGeneration),GGameUserSettingsIni);
    GConfig->SetInt(RenderSection,TEXT("Reflex"),int32(Reflex),GGameUserSettingsIni);
    GConfig->SetBool(RenderSection,TEXT("RayReconstruction"),bRayReconstruction,GGameUserSettingsIni);
    ++SaveFlushCallsV87;GConfig->Flush(false,GGameUserSettingsIni);
}
bool UFrontlineRenderSettingsSubsystem::SetQuality(EFrontlineRenderQuality Value,bool bSave)
{
    if(!GetCapabilities().Qualities.Contains(Value))return false;
    if(bApplied&&Quality==Value&&(Value!=EFrontlineRenderQuality::Native||!bRayReconstruction))
    {if(bSave)Save();return true;}
    SuspendFrameGeneration();Quality=Value;if(Value==EFrontlineRenderQuality::Native)bRayReconstruction=false;
    Apply();if(bSave)Save();return true;
}
bool UFrontlineRenderSettingsSubsystem::SetFrameGeneration(EFrontlineFrameGeneration Value,bool bSave)
{
    if(!GetCapabilities().FrameGenerationModes.Contains(Value))return false;
    if(bApplied&&FrameGeneration==Value&&(Value==EFrontlineFrameGeneration::Off||Reflex!=EFrontlineReflexMode::Off))
    {if(bSave)Save();return true;}
    FrameGeneration=Value;
    if(Value!=EFrontlineFrameGeneration::Off && Reflex==EFrontlineReflexMode::Off)Reflex=EFrontlineReflexMode::On;
    Apply();if(bSave)Save();return true;
}
bool UFrontlineRenderSettingsSubsystem::SetReflex(EFrontlineReflexMode Value,bool bSave)
{
    if(!GetCapabilities().ReflexModes.Contains(Value) || (Value==EFrontlineReflexMode::Off && FrameGeneration!=EFrontlineFrameGeneration::Off))return false;
    if(bApplied&&Reflex==Value){if(bSave)Save();return true;}
    Reflex=Value;Apply();if(bSave)Save();return true;
}
bool UFrontlineRenderSettingsSubsystem::SetRayReconstruction(bool Value,bool bSave)
{
    if(Value && (!GetCapabilities().bRayReconstruction || Quality==EFrontlineRenderQuality::Native))return false;
    if(bApplied&&bRayReconstruction==Value){if(bSave)Save();return true;}
    SuspendFrameGeneration();bRayReconstruction=Value;Apply();if(bSave)Save();return true;
}
void UFrontlineRenderSettingsSubsystem::SuspendFrameGeneration(float Seconds)
{
    ++SuspendCallsV87;
    FrameGenerationResumeAt=FMath::Max(FrameGenerationResumeAt,FPlatformTime::Seconds()+FMath::Max(0.f,Seconds));
    StopFrameGeneration();
}
bool UFrontlineRenderSettingsSubsystem::DeferOutputChangeV844(FName Domain,TFunction<void()> Mutation)
{
    if(EffectiveFrameGeneration==EFrontlineFrameGeneration::Off&&!ReflexDrain().bNeeded&&PendingOutputChangesV844.IsEmpty())return false;
    PendingOutputChangesV844.Add(Domain,MoveTemp(Mutation));
    SuspendFrameGeneration(1.f);return true;
}
bool UFrontlineRenderSettingsSubsystem::IsFrameGenerationDrainPendingV844() const
{return ReflexDrain().bNeeded;}
void UFrontlineRenderSettingsSubsystem::PresentationFocusLostV844()
{
    bLastPresentationFocusV844=false;
    FrameGenerationResumeAt=FMath::Max(FrameGenerationResumeAt,FPlatformTime::Seconds()+.35);
    if(bApplied)StopFrameGeneration();
}
void UFrontlineRenderSettingsSubsystem::PresentationFocusRestoredV844()
{
    bLastPresentationFocusV844=true;
    FrameGenerationResumeAt=FMath::Max(FrameGenerationResumeAt,FPlatformTime::Seconds()+.35);
    if(bApplied)StopFrameGeneration();
}
void UFrontlineRenderSettingsSubsystem::TickPresentationSafetyV844()
{
    const bool Focus=FApp::HasFocus();
    if(Focus!=bLastPresentationFocusV844)
    {if(Focus)PresentationFocusRestoredV844();else PresentationFocusLostV844();}
    const auto* Client=GetWorld()?GetWorld()->GetGameViewport():nullptr;
    if(Client&&Client->Viewport)
    {
        const FIntPoint Size=Client->Viewport->GetSizeXY();
        if(Size!=LastViewportSizeV844)
        {if(LastViewportSizeV844.X>0&&LastViewportSizeV844.Y>0)SuspendFrameGeneration(.35f);LastViewportSizeV844=Size;}
    }
}
void UFrontlineRenderSettingsSubsystem::Reapply(){Apply();}

FFrontlineFrameTiming UFrontlineRenderSettingsSubsystem::GetFrameTiming() const
{
    FFrontlineFrameTiming Timing;
    UStreamlineLibraryDLSSG::GetDLSSGFrameTiming(Timing.ReportedPresentFPS,Timing.FramesPresentedPerRendered);
    Timing.bPresentEstimateValid=FMath::IsFinite(Timing.ReportedPresentFPS) && Timing.ReportedPresentFPS>0 &&
        Timing.FramesPresentedPerRendered>=1;
    return Timing;
}

TSharedRef<FJsonObject> UFrontlineRenderSettingsSubsystem::GetDiagnostics() const
{
    const auto J=MakeShared<FJsonObject>();const auto Caps=GetCapabilities();
    J->SetStringField(TEXT("quality_requested"),QualityNames[int32(Quality)]);
    J->SetStringField(TEXT("graphics_preset"),GraphicsPreset==EFrontlineGraphicsPreset::High?TEXT("High"):GraphicsPreset==EFrontlineGraphicsPreset::Medium?TEXT("Medium"):TEXT("Low"));
    J->SetNumberField(TEXT("cosmetic_effects_budget"),EffectsBudgetV43());
    J->SetStringField(TEXT("fg_requested"),FrameNames[int32(FrameGeneration)]);
    J->SetStringField(TEXT("fg_effective"),FrameNames[int32(EffectiveFrameGeneration)]);
    J->SetStringField(TEXT("reflex_requested"),ReflexNames[int32(Reflex)]);
    J->SetNumberField(TEXT("reflex_effective"),int32(UStreamlineLibraryReflex::GetReflexMode()));
    J->SetBoolField(TEXT("reflex_disable_pending"),bReflexDisablePending);
    J->SetBoolField(TEXT("fg_stop_rhi_drain_pending"),ReflexDrain().bNeeded);
    J->SetNumberField(TEXT("fg_deferred_output_changes_v844"),PendingOutputChangesV844.Num());
    J->SetNumberField(TEXT("fg_output_mutations_completed_v844"),double(OutputMutationsV844));
    J->SetNumberField(TEXT("fg_resume_not_before_v844"),FrameGenerationResumeAt);
    J->SetNumberField(TEXT("settings_apply_calls_v87"),double(ApplyCallsV87));
    J->SetNumberField(TEXT("settings_suspend_calls_v87"),double(SuspendCallsV87));
    J->SetNumberField(TEXT("settings_save_flush_calls_v87"),double(SaveFlushCallsV87));
    J->SetBoolField(TEXT("fg_foreground_observed_v844"),bLastPresentationFocusV844);
    J->SetNumberField(TEXT("streamline_view_id_override_v844"),ConsoleFloat(TEXT("r.Streamline.ViewIdOverride"),-99));
    J->SetNumberField(TEXT("streamline_view_index_to_tag_v844"),ConsoleFloat(TEXT("r.Streamline.ViewIndexToTag"),-99));
    J->SetBoolField(TEXT("rr_requested"),bRayReconstruction);J->SetBoolField(TEXT("rr_enabled_api"),UDLSSLibrary::IsDLSSRREnabled());
    J->SetBoolField(TEXT("dlss_enabled_api"),UDLSSLibrary::IsDLSSEnabled());
    J->SetNumberField(TEXT("screen_percentage"),ConsoleFloat(TEXT("r.ScreenPercentage"),100));
    J->SetNumberField(TEXT("sr_support_code"),int32(UDLSSLibrary::QueryDLSSSupport()));
    J->SetNumberField(TEXT("rr_support_code"),int32(UDLSSLibrary::QueryDLSSRRSupport()));
    J->SetNumberField(TEXT("fg_support_code"),int32(UStreamlineLibraryDLSSG::QueryDLSSGSupport()));
    J->SetNumberField(TEXT("reflex_support_code"),int32(UStreamlineLibraryReflex::QueryReflexSupport()));
    J->SetBoolField(TEXT("orthographic_camera"),Caps.bOrthographic);
    if(Observation)
    {
        FScopeLock Lock(&Observation->Mutex);
        J->SetStringField(TEXT("observed_upscaler"),Observation->Upscaler);
        J->SetNumberField(TEXT("observed_view_families"),double(Observation->ViewFamilies));
        J->SetNumberField(TEXT("observed_dlss_frames"),double(Observation->DLSSViewFamilies));
        J->SetNumberField(TEXT("observed_rr_frames"),double(Observation->RRViewFamilies));
        J->SetBoolField(TEXT("observed_orthographic"),Observation->bOrthographic);
    }
    float PresentedEstimate=0;int32 Presented=0;
    UStreamlineLibraryDLSSG::GetDLSSGFrameTiming(PresentedEstimate,Presented);
    J->SetNumberField(TEXT("frames_presented_per_rendered"),Presented);
    J->SetNumberField(TEXT("present_fps_estimate"),PresentedEstimate);
    J->SetStringField(TEXT("present_fps_method"),TEXT("NVIDIA plugin: GAverageFPS times numFramesActuallyPresented; not independent present-timestamp measurement."));
    J->SetStringField(TEXT("upscaler_observation_method"),TEXT("Actual game view-family temporal upscaler interface after NVIDIA extension; NGX execution errors must also be checked in runtime log."));
    const auto HDR=GetHDRStatus();
    J->SetBoolField(TEXT("hdr_supported"),HDR.bSupported);
    J->SetBoolField(TEXT("hdr_requested"),HDR.bEnabled);
    J->SetBoolField(TEXT("hdr_output_active"),HDR.bOutputActive);
    J->SetNumberField(TEXT("hdr_output_format"),HDR.OutputFormat);
    J->SetNumberField(TEXT("hdr_display_peak_nits"),HDR.DisplayPeakNits);
    J->SetNumberField(TEXT("hdr_windows_reference_white_nits"),HDR.DisplaySDRWhiteNits);
    J->SetBoolField(TEXT("hdr_windows_white_known"),HDR.bSystemWhiteKnown);
    J->SetBoolField(TEXT("hdr_peak_metadata_known"),HDR.bPeakMetadataKnown);
    J->SetBoolField(TEXT("hdr_automatic_paper_white"),HDR.bPaperWhiteAutomatic);
    J->SetBoolField(TEXT("hdr_automatic_ui_white"),HDR.bUIWhiteAutomatic);
    J->SetNumberField(TEXT("hdr_peak_nits"),HDR.PeakNits);
    J->SetNumberField(TEXT("hdr_paper_white_nits"),HDR.PaperWhiteNits);
    J->SetNumberField(TEXT("hdr_ui_nits"),HDR.UILuminanceNits);
    J->SetNumberField(TEXT("hdr_black_nits"),HDR.BlackNits);
    J->SetNumberField(TEXT("hdr_calibration_backbuffer_frames"),double(HDR.CalibrationFrames));
    J->SetNumberField(TEXT("frame_rate_limit"),GetFrameRateLimit());
    J->SetNumberField(TEXT("internal_render_frame_limit"),ConsoleFloat(TEXT("t.MaxFPS")));
    J->SetNumberField(TEXT("reflex_presentation_frame_limit"),ConsoleFloat(TEXT("t.Streamline.Reflex.PresentationLimit")));
    J->SetNumberField(TEXT("reflex_submitted_interval_us"),ConsoleFloat(TEXT("t.Streamline.Reflex.LastFrameLimitUs")));
    J->SetNumberField(TEXT("frame_limit_presentation_ratio"),PresentationLimiter().Ratio);
    J->SetNumberField(TEXT("dynamic_fg_target_fps"),ConsoleFloat(TEXT("r.Streamline.DLSSG.DynamicTargetFrameRate")));
    J->SetBoolField(TEXT("frame_limit_waiting_for_fg_drain"),PresentationLimiter().bDraining);
    J->SetNumberField(TEXT("frame_rate_preference"),FrameRatePreference);
    J->SetStringField(TEXT("display_id"),ActiveDisplay.Id);
    J->SetStringField(TEXT("display_name"),ActiveDisplay.Name);
    J->SetNumberField(TEXT("active_refresh_hz"),ActiveDisplay.RefreshHz);
    J->SetBoolField(TEXT("active_refresh_known"),ActiveDisplay.bRefreshKnown);
    J->SetStringField(TEXT("active_refresh_source"),ActiveDisplay.RefreshSource);
    J->SetStringField(TEXT("hdr_profile_section"),FrontlineDisplay::HDRSection(ActiveDisplay.Id));
    J->SetBoolField(TEXT("hdr_query_state_known"),ActiveDisplay.bHDRStateKnown);
    J->SetBoolField(TEXT("hdr_metadata_query_known"),ActiveDisplay.bLuminanceKnown);
    J->SetBoolField(TEXT("hdr_display_profile_loaded"),bDisplayHDRProfileLoaded);
    J->SetBoolField(TEXT("hdr_desired_enabled"),bHDRDesiredEnabled);
    int32 RRDefaultsVersion=0;GConfig->GetInt(RenderSection,TEXT("RRDefaultsVersion"),RRDefaultsVersion,GGameUserSettingsIni);
    J->SetNumberField(TEXT("rr_defaults_version"),RRDefaultsVersion);
    int32 DefaultsVersion=0;GConfig->GetInt(RenderSection,TEXT("DisplayDefaultsVersion"),DefaultsVersion,GGameUserSettingsIni);
    J->SetNumberField(TEXT("display_defaults_version"),DefaultsVersion);
    J->SetBoolField(TEXT("display_migration_apply_pending"),bMigratedDisplayApplyPending);
    const auto* SettingsBranch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString());
    J->SetBoolField(TEXT("settings_write_protected"),SettingsBranch && SettingsBranch->InMemoryFile.NoSave);
    J->SetNumberField(TEXT("sr_preset_requested"),ConsoleFloat(TEXT("r.NGX.DLSS.Preset")));
    J->SetNumberField(TEXT("rr_preset_requested"),ConsoleFloat(TEXT("r.NGX.DLSSRR.Preset")));
    auto Priorities=MakeShared<FJsonObject>();
    for(const TCHAR* Name:{TEXT("r.Lumen.Reflections.BilateralFilter"),TEXT("r.Streamline.DLSSG.Enable"),TEXT("r.Streamline.DLSSG.FramesToGenerate"),TEXT("t.Streamline.Reflex.PresentationLimit"),TEXT("t.Streamline.Reflex.LastFrameLimitUs")})
        if(const auto* C=IConsoleManager::Get().FindConsoleVariable(Name))
        {
            auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("value"),C->GetFloat());
            V->SetNumberField(TEXT("priority"),C->GetFlags()&ECVF_SetByMask);Priorities->SetObjectField(Name,V);
        }
    J->SetObjectField(TEXT("sdk_cvar_priorities_v43"),Priorities);
    return J;
}
