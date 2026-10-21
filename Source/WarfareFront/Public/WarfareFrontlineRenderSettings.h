#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WarfareFrontlineDisplayMonitor.h"
#include "WarfareFrontlineRenderSettings.generated.h"

class FJsonObject;
class ISceneViewExtension;
class AWarfareFrontlineController;
void StartFrontlineV23DisplayReview(AWarfareFrontlineController* Controller);
void StartFrontlineV24DisplayReview(AWarfareFrontlineController* Controller);
struct FFrontlineRenderObservation;
struct FFrontlineHDRCalibration;

struct FFrontlineHDRStatus
{
    bool bSupported=false,bEnabled=false,bOutputActive=false,bCalibration=false;
    float DisplayPeakNits=0,DisplayBlackNits=0,DisplayFullFrameNits=0,DisplaySDRWhiteNits=203;
    bool bPaperWhiteAutomatic=true,bUIWhiteAutomatic=true,bSystemWhiteKnown=false,bPeakMetadataKnown=false;
    int32 PeakNits=1000,OutputFormat=0;
    float PaperWhiteNits=203,UILuminanceNits=203,BlackNits=.0001f;
    uint64 CalibrationFrames=0;
    FString Reason,OutputLabel,DisplayId,DisplayName;
};

enum class EFrontlineRenderQuality : uint8 { Native, DLAA, Quality, Balanced, Performance };
enum class EFrontlineFrameGeneration : uint8 { Off, X2, X3, X4, X5, X6, Dynamic };
enum class EFrontlineReflexMode : uint8 { Off, On, Boost };
enum class EFrontlineGraphicsPreset : uint8 { Low, Medium, High };

struct FFrontlineRenderCapabilities
{
    bool bSuperResolution=false;
    bool bRayReconstruction=false;
    bool bFrameGeneration=false;
    bool bReflex=false;
    bool bOrthographic=false;
    bool bFrameGenerationVSync=false;
    TArray<EFrontlineRenderQuality> Qualities{EFrontlineRenderQuality::Native};
    TArray<EFrontlineFrameGeneration> FrameGenerationModes{EFrontlineFrameGeneration::Off};
    TArray<EFrontlineReflexMode> ReflexModes{EFrontlineReflexMode::Off};
    FString SuperResolutionReason,RayReconstructionReason,FrameGenerationReason,ReflexReason;
};

UCLASS()
class WARFAREFRONT_API UFrontlineRenderSettingsSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    static UFrontlineRenderSettingsSubsystem* Get(UWorld* World);
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }

    EFrontlineRenderQuality GetQuality() const { return Quality; }
    EFrontlineFrameGeneration GetFrameGeneration() const { return FrameGeneration; }
    EFrontlineFrameGeneration GetEffectiveFrameGeneration() const { return EffectiveFrameGeneration; }
    EFrontlineReflexMode GetReflex() const { return Reflex; }
    bool GetRayReconstruction() const { return bRayReconstruction && GraphicsPreset==EFrontlineGraphicsPreset::High; }
    bool SetQuality(EFrontlineRenderQuality Value,bool bSave=true);
    bool SetFrameGeneration(EFrontlineFrameGeneration Value,bool bSave=true);
    bool SetReflex(EFrontlineReflexMode Value,bool bSave=true);
    bool SetRayReconstruction(bool Value,bool bSave=true);
    FFrontlineRenderCapabilities GetCapabilities() const;
    TSharedRef<FJsonObject> GetDiagnostics() const;
    FFrontlineHDRStatus GetHDRStatus() const;
    bool SetHDREnabled(bool bEnabled,bool bSave=true);
    void SetHDRPeakNits(int32 Nits,bool bSave=true);
    void SetHDRPaperWhiteNits(float Nits,bool bSave=true);
    void SetHDRUILuminanceNits(float Nits,bool bSave=true);
    void SetHDRBlackNits(float Nits,bool bSave=true);
    void ResetHDRCalibration(bool bSave=true);
    void SetHDRCalibrationVisible(bool bVisible);
    bool SetFrameRateLimit(float FPS,bool bSave=true);
    float GetFrameRateLimit() const;
    const FFrontlineActiveDisplay& GetActiveDisplay() const { return ActiveDisplay; }
    FIntPoint GetFrameRateRange() const { return FrontlineDisplay::FrameRateRange(ActiveDisplay.RefreshHz); }
    bool SetFrameRateSlider(float Position,bool bSave=true);
    void CommitFrameRateLimit();
    FFrontlineFrameTiming GetFrameTiming() const;
    // Call immediately before changing output resolution/window mode; no confirmation required.
    void SuspendFrameGeneration(float Seconds=1.f);
    // Queue only output mutations which would otherwise race the SDK's Off pass.
    bool DeferOutputChangeV844(FName Domain,TFunction<void()> Mutation);
    bool IsFrameGenerationDrainPendingV844() const;
    void Reapply();
    EFrontlineGraphicsPreset GetGraphicsPreset() const { return GraphicsPreset; }
    bool SetGraphicsPreset(EFrontlineGraphicsPreset Preset,bool bSave=true);
    void ApplyGraphicsPreset();
    static float EffectsBudgetV43();

private:
    friend struct FFrontlineRenderSettingsV87Review;
    void Apply();
    void InitializeGraphicsPreset();
    EFrontlineGraphicsPreset GraphicsPreset=EFrontlineGraphicsPreset::High;
    bool bGraphicsApplied=false;
    void ApplyFrameGenerationAndReflex(EFrontlineFrameGeneration Generation,EFrontlineReflexMode ReflexMode);
    void StopFrameGeneration();
    void TickDeferredReflex();
    void TickPresentationSafetyV844();
    void PresentationFocusLostV844();
    void PresentationFocusRestoredV844();
    void TickPresentationFrameLimit();
    void Save();
    bool ShouldRunFrameGeneration() const;
    void InitializeHDR();
    void MigrateV23DisplayDefaults();
    void MigrateV24RenderDefaults();
    void MigrateV30RenderDefaults();
    void TickActiveDisplay();
    void SaveDisplayHDRProfile();
    void LoadDisplayHDRProfile();
    void TickMigratedDisplayDefaults();
    void ShutdownHDR();
    void ApplyHDRCalibration(bool bSave);
    void TickHDRCalibration();
    EFrontlineRenderQuality Quality=EFrontlineRenderQuality::Quality;
    EFrontlineFrameGeneration FrameGeneration=EFrontlineFrameGeneration::X2;
    EFrontlineReflexMode Reflex=EFrontlineReflexMode::On;
    bool bRayReconstruction=true,bApplied=false,bNoSave=false,bRequestedFrameGenerationSupported=false;
    EFrontlineFrameGeneration EffectiveFrameGeneration=EFrontlineFrameGeneration::Off;
    float RequestedScreenPercentage=100.f;
    double FrameGenerationResumeAt=0.;
    TMap<FName,TFunction<void()>> PendingOutputChangesV844;
    FDelegateHandle WillDeactivateV844,WillBackgroundV844,ReactivatedV844,ForegroundV844;
    FIntPoint LastViewportSizeV844=FIntPoint::ZeroValue;
    bool bLastPresentationFocusV844=false;
    uint64 OutputMutationsV844=0;
    // Nonserialized game-thread call counts, not SDK/RHI submission evidence.
    uint64 ApplyCallsV87=0,SuspendCallsV87=0,SaveFlushCallsV87=0;
    TSharedPtr<FFrontlineRenderObservation,ESPMode::ThreadSafe> Observation;
    TSharedPtr<ISceneViewExtension,ESPMode::ThreadSafe> ViewObserver;
    TSharedPtr<FFrontlineHDRCalibration,ESPMode::ThreadSafe> HDRCalibration;
    float HDRBlackNits=.0001f;
    bool bHDRValuesInitialized=false;
    bool bMigratedDisplayApplyPending=false;
    bool bReflexDisablePending=false;
    int32 LastHDROutputFormat=INDEX_NONE;
    FFrontlineActiveDisplay ActiveDisplay;
    double NextDisplayPoll=0;
    float FrameRatePreference=0;
    bool bHDRPaperWhiteAutomatic=true,bHDRUIWhiteAutomatic=true;
    bool bHDRDesiredEnabled=false;
    bool bDisplayHDRProfileLoaded=false,bLegacyHDRAvailable=false;
    FFrontlineHDRValues LegacyHDRValues;
};
