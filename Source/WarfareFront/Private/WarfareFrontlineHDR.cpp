#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontline.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Slate/SlateViewportProvider.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "HDRHelper.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeLock.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHI.h"
#include "ClearQuad.h"
#include "UnrealClient.h"

void UFrontlineRenderSettingsSubsystem::MigrateV30RenderDefaults()
{
    if(bNoSave || !GetWorld() || GetWorld()->WorldType!=EWorldType::Game)return;
    int32 Version=0;GConfig->GetInt(TEXT("Warfare.Render"),TEXT("RenderDefaultsVersion"),Version,GGameUserSettingsIni);
    if(Version>=30)return;
    const auto* Branch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString());
    const FString Path=FPaths::ConvertRelativePathToFull(Branch && !Branch->IniPath.IsEmpty()?Branch->IniPath:
        FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
    if(IFileManager::Get().FileExists(*Path) && IFileManager::Get().Copy(*(Path+TEXT(".pre-v30-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+TEXT(".bak")),*Path,false,true)!=COPY_OK)
    {UE_LOG(LogTemp,Warning,TEXT("V30 render defaults migration deferred: settings backup unavailable"));return;}
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("Quality"),int32(EFrontlineRenderQuality::Quality),GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("FrameGeneration"),1,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Warfare.Render"),TEXT("RayReconstruction"),true,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Warfare.Render"),TEXT("FrameRatePreference"),0.f,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("RenderDefaultsVersion"),30,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}

void UFrontlineRenderSettingsSubsystem::MigrateV24RenderDefaults()
{
    if(bNoSave || !GetWorld() || GetWorld()->WorldType!=EWorldType::Game)return;
    int32 Version=0;GConfig->GetInt(TEXT("Warfare.Render"),TEXT("RRDefaultsVersion"),Version,GGameUserSettingsIni);
    if(Version>=24)return;
    const auto* Branch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString());
    const FString Path=FPaths::ConvertRelativePathToFull(Branch && !Branch->IniPath.IsEmpty()?Branch->IniPath:
        FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
    if(IFileManager::Get().FileExists(*Path))
    {
        const FString Backup=Path+TEXT(".pre-v24-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+TEXT(".bak");
        if(IFileManager::Get().Copy(*Backup,*Path,false,true)!=COPY_OK)
        {UE_LOG(LogTemp,Warning,TEXT("V24 RR migration deferred: could not back up display settings"));return;}
    }
    // This migration changes RR only. V23 resolution, AA, FG and player choices stay intact.
    GConfig->SetBool(TEXT("Warfare.Render"),TEXT("RayReconstruction"),true,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("RRDefaultsVersion"),24,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}

void UFrontlineRenderSettingsSubsystem::MigrateV23DisplayDefaults()
{
    // The user explicitly requested this one-time quality-first migration.
    // Subsequent player changes survive, and no-save QA / editor worlds do not
    // touch the user's display profile. Campaign and audio data are untouched.
    if(bNoSave || !GetWorld() || GetWorld()->WorldType!=EWorldType::Game)return;
    int32 Version=0;GConfig->GetInt(TEXT("Warfare.Render"),TEXT("DisplayDefaultsVersion"),Version,GGameUserSettingsIni);
    if(Version>=23)return;
    auto* G=GEngine?GEngine->GetGameUserSettings():nullptr;if(!G)return;
    const FConfigBranch* Branch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString());
    const FString SettingsPath=FPaths::ConvertRelativePathToFull(Branch && !Branch->IniPath.IsEmpty()?Branch->IniPath:
        FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
    FString Backup;
    if(IFileManager::Get().FileExists(*SettingsPath))
    {
        Backup=SettingsPath+TEXT(".pre-v23-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+TEXT(".bak");
        if(IFileManager::Get().Copy(*Backup,*SettingsPath,false,true)!=COPY_OK)
        {UE_LOG(LogTemp,Warning,TEXT("V23 display migration deferred: could not back up %s"),*SettingsPath);return;}
    }
    TArray<FIntPoint> SupportedModes;UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedModes);
    const FIntPoint InitialSize=FrontlineDisplay::InitialResolution(G->GetDesktopResolution(),SupportedModes);
    G->SetScreenResolution(InitialSize);G->SetFullscreenMode(EWindowMode::Fullscreen);
    G->ConfirmVideoMode();G->SaveSettings();
    GConfig->SetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("PreferredFullscreenMode"),0,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("Quality"),int32(EFrontlineRenderQuality::DLAA),GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("FrameGeneration"),0,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Warfare.Render"),TEXT("DisplayDefaultsVersion"),23,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
    bMigratedDisplayApplyPending=true;
    UE_LOG(LogTemp,Display,TEXT("V33 compatible display defaults migrated: %dx%d fullscreen; previous ini=%s"),
        InitialSize.X,InitialSize.Y,Backup.IsEmpty()?TEXT("none (fresh profile)"):*Backup);
}

void UFrontlineRenderSettingsSubsystem::TickMigratedDisplayDefaults()
{
    // Initialize may run before a usable game viewport exists. Apply this
    // migration once after startup, never recreate a swapchain during InitWorld.
    // The controller normally restores it in BeginPlay; this also covers a
    // viewport that was not ready at that point. Later player choices are left alone.
    if(!bMigratedDisplayApplyPending || !GetWorld() || !GetWorld()->HasBegunPlay())return;
    const auto* V=GetWorld()->GetGameViewport();
    if(!V || !V->Viewport || !V->GetWindow().IsValid())return;
    const FIntPoint PreviousSize=V->Viewport->GetSizeXY();
    if(PreviousSize.X<=0 || PreviousSize.Y<=0)return;
    auto* G=GEngine?GEngine->GetGameUserSettings():nullptr;if(!G)return;
    bMigratedDisplayApplyPending=false;
    // A startup resize can update the settings object; use the migrated saved
    // values, then let UE honor explicit -ResX/-ResY/-windowed review overrides.
    G->LoadSettings(false);
    SuspendFrameGeneration(1.f);
    G->ApplyResolutionSettings(true);
    const FIntPoint SavedSize=G->GetScreenResolution();
    UE_LOG(LogTemp,Display,TEXT("V23 migrated display applied at ready viewport: previous=%dx%d saved=%dx%d mode=%d; command-line overrides preserved"),
        PreviousSize.X,PreviousSize.Y,SavedSize.X,SavedSize.Y,int32(G->GetFullscreenMode()));
}

// The calibration patches are composed into the actual HDR backbuffer. Canvas
// screenshots are SDR and cannot verify their luminance or a monitor's clipping.
struct FFrontlineHDRCalibration
{
    FCriticalSection Mutex;
    FDelegateHandle Handle;
    const SWindow* Window=nullptr; // Identity only; never dereferenced on the render thread.
    bool bVisible=false,bOutputActive=false;
    int32 Format=0;
    float Peak=1000,Paper=203,Black=.0001f;
    uint64 RenderedFrames=0;
};

namespace
{
UGameUserSettings* HDRSettings() {return GEngine?GEngine->GetGameUserSettings():nullptr;}
bool IsHDRFormat(int32 Format) {return Format>=3 && Format<=6;}
void HDRVar(const TCHAR* Name,float Value)
{
    if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))
    {
        // UE 5.8 rejects replacing constructor-priority values implicitly.
        if((C->GetFlags()&ECVF_SetByMask)<ECVF_SetByScalability)C->Set(Value,ECVF_SetByGameSetting);
        else C->SetWithCurrentPriority(Value);
    }
}
float EncodeHDRNits(float Nits,int32 Format)
{
    if(Format==5 || Format==6)return Nits/80.f; // DXGI scRGB reference white.
    const float P=FMath::Pow(FMath::Clamp(Nits/10000.f,0.f,1.f),2610.f/16384.f);
    return FMath::Pow((3424.f/4096.f+2413.f/128.f*P)/(1.f+2392.f/128.f*P),2523.f/32.f);
}
}

FFrontlineHDRStatus UFrontlineRenderSettingsSubsystem::GetHDRStatus() const
{
    FFrontlineHDRStatus S;
    const auto* G=HDRSettings(); if(!G)return S;
    S.bEnabled=G->IsHDREnabled();S.bCalibration=G->IsHDRCalibrationUsed();
    S.PeakNits=G->GetMaximumHDRDisplayNits();S.PaperWhiteNits=G->GetHDRPaperWhiteNits();
    S.UILuminanceNits=G->GetHDRUILuminanceNits();S.BlackNits=HDRBlackNits;
    if(HDRCalibration){FScopeLock Lock(&HDRCalibration->Mutex);S.CalibrationFrames=HDRCalibration->RenderedFrames;}
    const auto* V=GetWorld()?GetWorld()->GetGameViewport():nullptr;
    S.DisplayId=ActiveDisplay.Id;S.DisplayName=ActiveDisplay.Name;
    S.bSupported=ActiveDisplay.bValid && ActiveDisplay.bHDRStateKnown && bDisplayHDRProfileLoaded && ActiveDisplay.bHDRSupported && ActiveDisplay.bWindowsHDREnabled && G->SupportsHDRDisplayOutput() && IsHDRAllowed();
    S.DisplayPeakNits=ActiveDisplay.PeakNits;S.DisplayBlackNits=ActiveDisplay.BlackNits;
    S.DisplayFullFrameNits=ActiveDisplay.FullFrameNits;
    S.DisplaySDRWhiteNits=ActiveDisplay.SDRWhiteNits;S.bSystemWhiteKnown=ActiveDisplay.bSDRWhiteKnown;
    S.bPeakMetadataKnown=ActiveDisplay.bLuminanceKnown;
    S.bPaperWhiteAutomatic=bHDRPaperWhiteAutomatic;S.bUIWhiteAutomatic=bHDRUIWhiteAutomatic;
    S.OutputFormat=V && V->Viewport?int32(V->Viewport->GetDisplayOutputFormat()):0;
    S.bOutputActive=S.bSupported && S.bEnabled && IsHDRFormat(S.OutputFormat) && ::IsHDREnabled();
    S.OutputLabel=S.bOutputActive?(S.OutputFormat<=4?TEXT("HDR10 · PQ"):TEXT("HDR · scRGB")):TEXT("SDR");
    if(!S.bSupported)S.Reason=ActiveDisplay.bHDRSupported && !ActiveDisplay.bWindowsHDREnabled?
        TEXT("请先在 Windows 中为当前屏幕开启 HDR"):TEXT("当前屏幕未提供 HDR 输出");
    if(ActiveDisplay.bHDRSupported && ActiveDisplay.bWindowsHDREnabled && !bDisplayHDRProfileLoaded)S.Reason=TEXT("正在识别当前 HDR 屏幕");
    return S;
}

void UFrontlineRenderSettingsSubsystem::InitializeHDR()
{
    GConfig->GetFloat(TEXT("Warfare.Render"),TEXT("HDRBlackNits"),HDRBlackNits,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Warfare.Render"),TEXT("HDRValuesInitialized"),bHDRValuesInitialized,GGameUserSettingsIni);
    if(const auto* G=HDRSettings())
    {
        bHDRValuesInitialized|=G->IsHDRCalibrationUsed();bHDRDesiredEnabled=G->IsHDREnabled();
        FrameRatePreference=G->GetFrameRateLimit();
    }
    GConfig->GetBool(TEXT("Warfare.Render"),TEXT("HDRDesiredEnabled"),bHDRDesiredEnabled,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("Warfare.Render"),TEXT("FrameRatePreference"),FrameRatePreference,GGameUserSettingsIni);
    if(!FMath::IsFinite(FrameRatePreference) || FrameRatePreference<0)FrameRatePreference=0;
    if(!FMath::IsFinite(HDRBlackNits))HDRBlackNits=.0001f;
    HDRBlackNits=FMath::Clamp(HDRBlackNits,0.0001f,.1f);
    if(const auto* G=HDRSettings())
    {
        bLegacyHDRAvailable=bHDRValuesInitialized;LegacyHDRValues.PeakNits=G->GetMaximumHDRDisplayNits();
        LegacyHDRValues.PaperWhiteNits=G->GetHDRPaperWhiteNits();LegacyHDRValues.UILuminanceNits=G->GetHDRUILuminanceNits();
        LegacyHDRValues.BlackNits=HDRBlackNits;LegacyHDRValues.bCalibrated=G->IsHDRCalibrationUsed();
    }
    HDRVar(TEXT("r.HDR.Display.MinLuminanceLog10"),FMath::LogX(10.f,HDRBlackNits));
    HDRVar(TEXT("r.HDR.PaperWhite.Mode"),1);
    // Canvas artwork is authored/encoded as sRGB, including the etched materials.
    // The Slate HDR layer must invert that transfer function, not its default 2.4 power.
    HDRVar(TEXT("r.HDR.UI.CompositeMode"),1);
    HDRVar(TEXT("r.HDR.UI.CompositeEOTF"),1);
    HDRCalibration=MakeShared<FFrontlineHDRCalibration,ESPMode::ThreadSafe>();
    if(!FSlateApplication::IsInitialized())return;
    auto State=HDRCalibration;
    HDRCalibration->Handle=FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddLambda(
        [State](SWindow& Window,ISlateViewportProvider& Provider)
        {
            int32 Format;float Peak,Paper,Black;
            {
                FScopeLock Lock(&State->Mutex);
                if(!State->bVisible || !State->bOutputActive || State->Window!=&Window)return;
                Format=State->Format;Peak=State->Peak;Paper=State->Paper;Black=State->Black;
            }
            FRHITexture* BackBuffer=Provider.GetBackBufferResource();
            if(!BackBuffer)return;
            // Do not encode a stale HDR mode into an SDR / incompatible buffer
            // during a resize or display-mode transition.
            const auto PixelFormat=BackBuffer->GetDesc().Format;
            if((Format<=4 && PixelFormat!=PF_A2B10G10R10) ||
               (Format>=5 && PixelFormat!=PF_FloatRGBA))return;
            auto& RHICmd=FRHICommandListExecutor::GetImmediateCommandList();
            FRDGBuilder Graph(RHICmd);
            auto Texture=Graph.RegisterExternalTexture(CreateRenderTarget(BackBuffer,TEXT("FrontlineHDRCalibration")));
            const auto Extent=BackBuffer->GetDesc().Extent;
            const float Scale=FMath::Min(Extent.X/1920.f,Extent.Y/1080.f);
            const float OffsetX=(Extent.X-1920.f*Scale)*.5f,OffsetY=(Extent.Y-1080.f*Scale)*.5f;
            auto Patch=[&](float X,float Y,float W,float H,float Nits)
            {
                const float Encoded=EncodeHDRNits(Nits,Format);
                const FIntRect Area(FIntPoint(OffsetX+X*Scale,OffsetY+Y*Scale),FIntPoint(OffsetX+(X+W)*Scale,OffsetY+(Y+H)*Scale));
                // UE's region-clear helper uses ENoAction; retain the already
                // drawn game/UI and every preceding calibration patch explicitly.
                auto* Parameters=Graph.AllocParameters<FRenderTargetParameters>();
                Parameters->RenderTargets[0]=FRenderTargetBinding(Texture,ERenderTargetLoadAction::ELoad);
                Graph.AddPass(RDG_EVENT_NAME("FrontlineHDRPatch"),Parameters,ERDGPassFlags::Raster,
                    [Area,Encoded](FRDGAsyncTask,FRHICommandList& Cmd)
                    {
                        Cmd.SetViewport(float(Area.Min.X),float(Area.Min.Y),0,float(Area.Max.X),float(Area.Max.Y),1);
                        DrawClearQuad(Cmd,FLinearColor(Encoded,Encoded,Encoded,1));
                    });
            };
            // Three true-output patches: peak / paper white / near black. The
            // centre mark is 10% dimmer (white) or brighter (black) than its field.
            const float Levels[]={Peak,Paper,Black};
            for(int32 I=0;I<3;++I)
            {
                const float X=112+I*364.f;Patch(X,744,340,100,Levels[I]);
                const float Mark=I==2?FMath::Max(.005f,Black*2):Levels[I]*.9f;
                Patch(X+162,768,16,52,Mark);Patch(X+144,786,52,16,Mark);
            }
            Graph.SetTextureAccessFinal(Texture,ERHIAccess::Present);Graph.Execute();
            {FScopeLock Lock(&State->Mutex);++State->RenderedFrames;}
        });
}
void UFrontlineRenderSettingsSubsystem::ShutdownHDR()
{
    if(!HDRCalibration)return;
    {FScopeLock Lock(&HDRCalibration->Mutex);HDRCalibration->bVisible=false;HDRCalibration->Window=nullptr;}
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().Remove(HDRCalibration->Handle);
    HDRCalibration.Reset();
}
void UFrontlineRenderSettingsSubsystem::TickHDRCalibration()
{
    if(!HDRCalibration)return;
    const auto* V=GetWorld()->GetGameViewport();const auto Window=V?V->GetWindow():nullptr;
    const int32 Format=V && V->Viewport?int32(V->Viewport->GetDisplayOutputFormat()):0;
    if(Format!=LastHDROutputFormat)
    {
        const bool bChanged=LastHDROutputFormat!=INDEX_NONE;LastHDROutputFormat=Format;
        // HDR10 alpha has only two bits. The official UE integration recommends
        // omitting the backbuffer-derived UI-alpha guide in HDR.
        HDRVar(TEXT("r.Streamline.TagUIColorAlpha"),IsHDRFormat(Format)?0.f:1.f);
        if(bChanged && bApplied){SuspendFrameGeneration(1.f);Reapply();}
    }
    const auto* PC=GetWorld()->GetFirstPlayerController();
    const auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
    FScopeLock Lock(&HDRCalibration->Mutex);
    HDRCalibration->Window=Window.Get();
    if(!HUD || HUD->GetPresentationPage()!=7)HDRCalibration->bVisible=false;
}
void UFrontlineRenderSettingsSubsystem::SetHDRCalibrationVisible(bool bVisible)
{
    if(!HDRCalibration)return;
    if(!bVisible){FScopeLock Lock(&HDRCalibration->Mutex);HDRCalibration->bVisible=false;return;}
    const auto S=GetHDRStatus();FScopeLock Lock(&HDRCalibration->Mutex);
    HDRCalibration->bVisible=bVisible;HDRCalibration->bOutputActive=S.bOutputActive;
    HDRCalibration->Format=S.OutputFormat;HDRCalibration->Peak=S.PeakNits;
    HDRCalibration->Paper=S.PaperWhiteNits;HDRCalibration->Black=S.BlackNits;
}
void UFrontlineRenderSettingsSubsystem::ApplyHDRCalibration(bool bSave)
{
    TWeakObjectPtr<UFrontlineRenderSettingsSubsystem> Weak(this);
    if(DeferOutputChangeV844(TEXT("HDRCalibration"),[Weak,bSave]{if(auto* R=Weak.Get())R->ApplyHDRCalibration(bSave);}))return;
    if(auto* G=HDRSettings())
    {
        // Respect a calibrated peak when deriving automatic white, including
        // live peak-slider changes. Manual white remains a separate preference.
        auto WhiteDisplay=ActiveDisplay;WhiteDisplay.PeakNits=G->GetMaximumHDRDisplayNits();
        const auto White=FrontlineDisplay::DefaultHDR(WhiteDisplay);
        if(bHDRPaperWhiteAutomatic)G->SetHDRPaperWhiteNits(White.PaperWhiteNits);
        if(bHDRUIWhiteAutomatic)G->SetHDRUILuminanceNits(White.UILuminanceNits);
        HDRVar(TEXT("r.HDR.Display.MinLuminanceLog10"),FMath::LogX(10.f,HDRBlackNits));
        HDRVar(TEXT("r.HDR.PaperWhite.Mode"),1);
        G->SetHDRUILuminanceSeparate(true);G->ApplyNonResolutionSettings();
        TickPresentationFrameLimit();
        IConsoleManager::Get().CallAllConsoleVariableSinks();
        if(bSave && !bNoSave)
        {
            G->SaveSettings();GConfig->SetFloat(TEXT("Warfare.Render"),TEXT("HDRBlackNits"),HDRBlackNits,GGameUserSettingsIni);
            GConfig->SetBool(TEXT("Warfare.Render"),TEXT("HDRValuesInitialized"),bHDRValuesInitialized,GGameUserSettingsIni);
            GConfig->SetBool(TEXT("Warfare.Render"),TEXT("HDRDesiredEnabled"),bHDRDesiredEnabled,GGameUserSettingsIni);
            SaveDisplayHDRProfile();
            GConfig->Flush(false,GGameUserSettingsIni);
        }
    }
}
bool UFrontlineRenderSettingsSubsystem::SetHDREnabled(bool bEnabled,bool bSave)
{
    auto* G=HDRSettings();if(!G || (bEnabled && !GetHDRStatus().bSupported))return false;
    TWeakObjectPtr<UFrontlineRenderSettingsSubsystem> Weak(this);
    if(DeferOutputChangeV844(TEXT("HDREnabled"),[Weak,bEnabled,bSave]{if(auto* R=Weak.Get())R->SetHDREnabled(bEnabled,bSave);}))return true;
    SuspendFrameGeneration(1.f);
    if(bEnabled && !bHDRValuesInitialized)ResetHDRCalibration(false);
    bHDRDesiredEnabled=bEnabled;
    G->SetHDREnabled(bEnabled);ApplyHDRCalibration(bSave);Reapply();return G->IsHDREnabled()==bEnabled;
}
void UFrontlineRenderSettingsSubsystem::SetHDRPeakNits(int32 Nits,bool bSave)
{if(auto* G=HDRSettings()){bHDRValuesInitialized=true;G->SetMaximumHDRDisplayNits(FMath::Clamp(Nits,200,4000));G->SetHDRCalibrationUsed(true);ApplyHDRCalibration(bSave);}}
void UFrontlineRenderSettingsSubsystem::SetHDRPaperWhiteNits(float Nits,bool bSave)
{if(FMath::IsFinite(Nits))if(auto* G=HDRSettings()){bHDRValuesInitialized=true;bHDRPaperWhiteAutomatic=false;G->SetHDRPaperWhiteNits(FMath::Clamp(Nits,80.f,FMath::Min(1000.f,G->GetMaximumHDRDisplayNits()*.8f)));ApplyHDRCalibration(bSave);}}
void UFrontlineRenderSettingsSubsystem::SetHDRUILuminanceNits(float Nits,bool bSave)
{if(FMath::IsFinite(Nits))if(auto* G=HDRSettings()){bHDRValuesInitialized=true;bHDRUIWhiteAutomatic=false;G->SetHDRUILuminanceNits(FMath::Clamp(Nits,80.f,FMath::Min(1000.f,G->GetMaximumHDRDisplayNits()*.8f)));ApplyHDRCalibration(bSave);}}
void UFrontlineRenderSettingsSubsystem::SetHDRBlackNits(float Nits,bool bSave)
{if(FMath::IsFinite(Nits)){bHDRValuesInitialized=true;HDRBlackNits=FMath::Clamp(Nits,.0001f,.1f);ApplyHDRCalibration(bSave);}}
void UFrontlineRenderSettingsSubsystem::ResetHDRCalibration(bool bSave)
{
    const auto Defaults=FrontlineDisplay::DefaultHDR(ActiveDisplay);
    if(auto* G=HDRSettings())
    {
        bHDRValuesInitialized=true;bHDRPaperWhiteAutomatic=true;bHDRUIWhiteAutomatic=true;
        G->SetMaximumHDRDisplayNits(Defaults.PeakNits);
        G->SetHDRCalibrationUsed(false);G->SetHDRPaperWhiteNits(Defaults.PaperWhiteNits);G->SetHDRUILuminanceNits(Defaults.UILuminanceNits);
        HDRBlackNits=Defaults.BlackNits;ApplyHDRCalibration(bSave);
    }
}
bool UFrontlineRenderSettingsSubsystem::SetFrameRateLimit(float FPS,bool bSave)
{
    const FIntPoint Range=GetFrameRateRange();
    if(!FMath::IsFinite(FPS) || FPS<Range.X || FPS>Range.Y)return false;
    if(auto* G=HDRSettings())
    {
        FrameRatePreference=FMath::RoundToFloat(FPS);G->SetFrameRateLimit(FrameRatePreference);
        // Dragging only updates the actual limiter; no HDR reconfiguration or disk write per pixel.
        TickPresentationFrameLimit();
        if(bSave)CommitFrameRateLimit();return true;
    }
    return false;
}
float UFrontlineRenderSettingsSubsystem::GetFrameRateLimit() const
{const auto* G=HDRSettings();return G?G->GetFrameRateLimit():0;}
bool UFrontlineRenderSettingsSubsystem::SetFrameRateSlider(float Position,bool bSave)
{return FMath::IsFinite(Position) && SetFrameRateLimit(FrontlineDisplay::SliderFrameRate(Position,ActiveDisplay.RefreshHz),bSave);}
void UFrontlineRenderSettingsSubsystem::CommitFrameRateLimit()
{
    if(bNoSave)return;
    if(auto* G=HDRSettings())G->SaveSettings();
    GConfig->SetFloat(TEXT("Warfare.Render"),TEXT("FrameRatePreference"),FrameRatePreference,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}

void UFrontlineRenderSettingsSubsystem::SaveDisplayHDRProfile()
{
    auto* G=HDRSettings();if(bNoSave || !G || !ActiveDisplay.bPersistentIdentity || ActiveDisplay.Id.IsEmpty() || !bDisplayHDRProfileLoaded)return;
    const FString Section=FrontlineDisplay::HDRSection(ActiveDisplay.Id);
    GConfig->SetString(*Section,TEXT("DisplayId"),*ActiveDisplay.Id,GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("Initialized"),true,GGameUserSettingsIni);
    GConfig->SetInt(*Section,TEXT("PeakNits"),G->GetMaximumHDRDisplayNits(),GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("PaperWhiteNits"),G->GetHDRPaperWhiteNits(),GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("UILuminanceNits"),G->GetHDRUILuminanceNits(),GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("BlackNits"),HDRBlackNits,GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("Calibrated"),G->IsHDRCalibrationUsed(),GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("AutomaticPaperWhite"),bHDRPaperWhiteAutomatic,GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("AutomaticUIWhite"),bHDRUIWhiteAutomatic,GGameUserSettingsIni);
    GConfig->SetInt(*Section,TEXT("WhiteDefaultsVersion"),30,GGameUserSettingsIni);
}
void UFrontlineRenderSettingsSubsystem::LoadDisplayHDRProfile()
{
    auto* G=HDRSettings();if(!G || !ActiveDisplay.bHDRStateKnown || !ActiveDisplay.bHDRSupported)return;
    const FString Section=FrontlineDisplay::HDRSection(ActiveDisplay.Id);
    bool Found=false;
    if(ActiveDisplay.bPersistentIdentity)GConfig->GetBool(*Section,TEXT("Initialized"),Found,GGameUserSettingsIni);
    // Missing EDID luminance is common with adapters, TVs and virtual displays.
    // Use an explicitly uncalibrated fallback, never block otherwise valid HDR.
    const FFrontlineHDRValues Defaults=FrontlineDisplay::DefaultHDR(ActiveDisplay);
    FFrontlineHDRValues Values=Defaults;
    bHDRPaperWhiteAutomatic=true;bHDRUIWhiteAutomatic=true;
    if(Found)
    {
        GConfig->GetInt(*Section,TEXT("PeakNits"),Values.PeakNits,GGameUserSettingsIni);
        GConfig->GetFloat(*Section,TEXT("PaperWhiteNits"),Values.PaperWhiteNits,GGameUserSettingsIni);
        GConfig->GetFloat(*Section,TEXT("UILuminanceNits"),Values.UILuminanceNits,GGameUserSettingsIni);
        GConfig->GetFloat(*Section,TEXT("BlackNits"),Values.BlackNits,GGameUserSettingsIni);
        GConfig->GetBool(*Section,TEXT("Calibrated"),Values.bCalibrated,GGameUserSettingsIni);
        int32 WhiteVersion=0;GConfig->GetInt(*Section,TEXT("WhiteDefaultsVersion"),WhiteVersion,GGameUserSettingsIni);
        bHDRPaperWhiteAutomatic=FMath::IsNearlyEqual(Values.PaperWhiteNits,203.f,.1f);
        bHDRUIWhiteAutomatic=FMath::IsNearlyEqual(Values.UILuminanceNits,203.f,.1f);
        if(WhiteVersion>=30)
        {
            GConfig->GetBool(*Section,TEXT("AutomaticPaperWhite"),bHDRPaperWhiteAutomatic,GGameUserSettingsIni);
            GConfig->GetBool(*Section,TEXT("AutomaticUIWhite"),bHDRUIWhiteAutomatic,GGameUserSettingsIni);
        }
    }
    else if(ActiveDisplay.bPersistentIdentity)
    {
        bool LegacyBound=false;GConfig->GetBool(TEXT("Warfare.Render"),TEXT("LegacyHDRBoundV24"),LegacyBound,GGameUserSettingsIni);
        // Existing V23 calibration belongs only to the first detected HDR screen.
        if(!LegacyBound && bLegacyHDRAvailable)
        {
            Values=LegacyHDRValues;
            bHDRPaperWhiteAutomatic=FMath::IsNearlyEqual(Values.PaperWhiteNits,203.f,.1f);
            bHDRUIWhiteAutomatic=FMath::IsNearlyEqual(Values.UILuminanceNits,203.f,.1f);
        }
        if(!bNoSave)GConfig->SetBool(TEXT("Warfare.Render"),TEXT("LegacyHDRBoundV24"),true,GGameUserSettingsIni);
    }
    if(!Values.bCalibrated)Values.PeakNits=Defaults.PeakNits;
    if(bHDRPaperWhiteAutomatic)Values.PaperWhiteNits=Defaults.PaperWhiteNits;
    if(bHDRUIWhiteAutomatic)Values.UILuminanceNits=Defaults.UILuminanceNits;
    G->SetMaximumHDRDisplayNits(FMath::Clamp(Values.PeakNits,200,4000));
    const float WhiteMax=FMath::Min(1000.f,G->GetMaximumHDRDisplayNits()*.8f);
    G->SetHDRPaperWhiteNits(FMath::IsFinite(Values.PaperWhiteNits)?FMath::Clamp(Values.PaperWhiteNits,80.f,WhiteMax):Defaults.PaperWhiteNits);
    G->SetHDRUILuminanceNits(FMath::IsFinite(Values.UILuminanceNits)?FMath::Clamp(Values.UILuminanceNits,80.f,WhiteMax):Defaults.UILuminanceNits);
    HDRBlackNits=FMath::IsFinite(Values.BlackNits)?FMath::Clamp(Values.BlackNits,.0001f,.1f):.0001f;
    G->SetHDRCalibrationUsed(Values.bCalibrated);bHDRValuesInitialized=true;bDisplayHDRProfileLoaded=true;
    SaveDisplayHDRProfile();
}
void UFrontlineRenderSettingsSubsystem::TickActiveDisplay()
{
    const double Now=FPlatformTime::Seconds();if(Now<NextDisplayPoll)return;NextDisplayPoll=Now+1.;
    const auto* V=GetWorld()?GetWorld()->GetGameViewport():nullptr;const auto Window=V?V->GetWindow():nullptr;
    if(!Window || !Window->GetNativeWindow().IsValid())return;
    auto Next=FrontlineDisplay::Query(Window->GetNativeWindow()->GetOSWindowHandle());
    if(!Next.bValid)return; // Display recreation is transient; keep the previous valid state until retry.
    Next=FrontlineDisplay::PreserveKnownState(Next,ActiveDisplay);
    auto* G=HDRSettings();if(!G)return;
    const bool First=!ActiveDisplay.bValid,Changed=First || ActiveDisplay.Id!=Next.Id;
    bool HDRChanged=Changed || ActiveDisplay.bHDRSupported!=Next.bHDRSupported || ActiveDisplay.bWindowsHDREnabled!=Next.bWindowsHDREnabled ||
        (!G->IsHDRCalibrationUsed() && !FMath::IsNearlyEqual(ActiveDisplay.PeakNits,Next.PeakNits,1.f));
    const bool WhiteChanged=Changed || ActiveDisplay.bSDRWhiteKnown!=Next.bSDRWhiteKnown || !FMath::IsNearlyEqual(ActiveDisplay.SDRWhiteNits,Next.SDRWhiteNits,.1f);
    HDRChanged|=WhiteChanged && (bHDRPaperWhiteAutomatic || bHDRUIWhiteAutomatic);
    const bool HzChanged=Changed || !FMath::IsNearlyEqual(ActiveDisplay.RefreshHz,Next.RefreshHz,.01f);
    if(Changed && !First)SaveDisplayHDRProfile();
    if(Changed)bDisplayHDRProfileLoaded=false;
    ActiveDisplay=Next;
    if(!bDisplayHDRProfileLoaded){LoadDisplayHDRProfile();HDRChanged|=bDisplayHDRProfileLoaded;}
    if(HzChanged)
    {
        const auto Range=GetFrameRateRange();
        const float Limit=FMath::Clamp(FrameRatePreference>0?FMath::RoundToFloat(FrameRatePreference):float(Range.Y),float(Range.X),float(Range.Y));
        G->SetFrameRateLimit(Limit);TickPresentationFrameLimit();
    }
    if(HDRChanged)
    {
        TWeakObjectPtr<UFrontlineRenderSettingsSubsystem> Weak(this);
        const auto Mutation=[Weak,Changed]
        {
            auto* R=Weak.Get();auto* Settings=HDRSettings();if(!R||!Settings)return;
            const auto Defaults=FrontlineDisplay::DefaultHDR(R->ActiveDisplay);
            if(R->bHDRPaperWhiteAutomatic)Settings->SetHDRPaperWhiteNits(Defaults.PaperWhiteNits);
            if(R->bHDRUIWhiteAutomatic)Settings->SetHDRUILuminanceNits(Defaults.UILuminanceNits);
            if(!Changed&&!Settings->IsHDRCalibrationUsed()&&R->ActiveDisplay.PeakNits>100)
                Settings->SetMaximumHDRDisplayNits(Defaults.PeakNits);
            R->SuspendFrameGeneration(1.f);
            Settings->SetHDREnabled(R->bHDRDesiredEnabled&&R->GetHDRStatus().bSupported);
            R->ApplyHDRCalibration(false);if(R->bApplied)R->Reapply();
            if(!R->bNoSave)
            {
                R->SaveDisplayHDRProfile();Settings->SaveSettings();
                GConfig->SetBool(TEXT("Warfare.Render"),TEXT("HDRDesiredEnabled"),R->bHDRDesiredEnabled,GGameUserSettingsIni);
                GConfig->SetFloat(TEXT("Warfare.Render"),TEXT("FrameRatePreference"),R->FrameRatePreference,GGameUserSettingsIni);
                GConfig->Flush(false,GGameUserSettingsIni);
            }
        };
        if(!DeferOutputChangeV844(TEXT("ActiveDisplayHDR"),Mutation))Mutation();
    }
}
