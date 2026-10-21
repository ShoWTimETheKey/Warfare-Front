#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineFrameCap.h"
#include "WarfareFrontline.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericWindow.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"
#include <limits>

void StartFrontlineV24FrameCapReview(AWarfareFrontlineController* Controller);

void StartFrontlineV24DisplayReview(AWarfareFrontlineController* Controller)
{
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV24DisplayReview")))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV24FrameCapOnly")))
    {StartFrontlineV24FrameCapReview(Controller);return;}
    FString UserDir;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty() || FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {UE_LOG(LogTemp,Error,TEXT("V24 display review requires isolated writable -UserDir."));return;}
    struct FRun
    {
        int32 Stage=0,ExpectedCap=0;bool Passed=true,OriginalPerformance=false,HasOriginal=false;
        double Start=FPlatformTime::Seconds(),Next=Start+3.,StageStarted=0;
        FString Directory,SettingsPath;
        FFrontlineHDRStatus OriginalHDR;float OriginalCap=120,ExpectedWhite=203;
        uint64 BeforeCalibration=0;
        TArray<TSharedPtr<FJsonValue>> Checks,Skips;
        TArray<FString> Screens;
        TSharedPtr<FJsonObject> Initial,Output;
        bool FrameGenerationReview=false;float GenerationTarget=90;
        bool FocusRequested=false,FocusBeforeRequest=false;
        TArray<TSharedPtr<FJsonValue>> FocusObservations;
        EFrontlineFrameGeneration OriginalFG=EFrontlineFrameGeneration::Off;
        EFrontlineReflexMode OriginalReflex=EFrontlineReflexMode::On;
    };
    const auto Run=MakeShared<FRun>();
    Run->FrameGenerationReview=FParse::Param(FCommandLine::Get(),TEXT("WFV24FrameCapFG"));
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V24/DisplayReview")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC || !PC->GetWorld())return false;
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;Run->Next=Now+.65;
        auto* R=UFrontlineRenderSettingsSubsystem::Get(PC->GetWorld());
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        auto* G=GEngine?GEngine->GetGameUserSettings():nullptr;
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);J->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Passed&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V24_DISPLAY %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto DiskSettings=[&](FConfigFile& Saved)
        {
            const auto* Branch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString());
            if(!Branch)return false;
            Run->SettingsPath=FPaths::ConvertRelativePathToFull(Branch->IniPath.IsEmpty()?
                FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()):Branch->IniPath);
            if(IFileManager::Get().FileSize(*Run->SettingsPath)<=0)return false;
            Saved=Branch->CombinedStaticLayers;Saved.Branch=nullptr;return Saved.Combine(Run->SettingsPath);
        };
        auto Command=[&](const TCHAR* Name){HUD->NotifyHitBoxClick(FName(Name));};
        auto RecordFocus=[&](const TCHAR* Point)
        {
            const auto* View=PC->GetWorld()->GetGameViewport();const auto* Viewport=View?View->Viewport:nullptr;
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("point"),Point);
            J->SetBoolField(TEXT("app_has_focus"),FApp::HasFocus());
            J->SetBoolField(TEXT("viewport_has_focus"),Viewport && Viewport->HasFocus());
            J->SetBoolField(TEXT("viewport_is_foreground"),Viewport && Viewport->IsForegroundWindow());
            Run->FocusObservations.Add(MakeShared<FJsonValueObject>(J));
            UE_LOG(LogTemp,Display,TEXT("WF_V24_FOCUS %s app=%d viewport=%d foreground=%d"),Point,FApp::HasFocus(),
                Viewport && Viewport->HasFocus(),Viewport && Viewport->IsForegroundWindow());
        };
        auto Snap=[&](const TCHAR* Name)
        {
            const FString File=Run->Directory/(FString(Name)+TEXT(".png"));Run->Screens.Add(File);
            FScreenshotRequest::RequestScreenshot(File,true,false);
            Check(Name,HUD->GetPresentationPage()==7 && B->AssetFailure.IsEmpty());
        };
        if((!R || !B || !HUD || !G || Now-Run->Start>(Run->FrameGenerationReview?40.:24.)) && Run->Stage<90)
        {Check(TEXT("dependencies_and_time_bound"),false);Run->Stage=90;}
        switch(Run->Stage++)
        {
        case 0:
        {
            Check(TEXT("campaign_is_volatile"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(B->GetProgressionSaveStatus()!=TEXT("独立验证档案")){Run->Stage=90;break;}
            if(Run->FrameGenerationReview)
            {
                Run->FocusBeforeRequest=FApp::HasFocus();RecordFocus(TEXT("before_startup_request"));
                const auto* View=PC->GetWorld()->GetGameViewport();const auto Window=View?View->GetWindow():nullptr;
                if(!Run->FocusBeforeRequest && Window.IsValid() && FSlateApplication::IsInitialized())
                {
                    // Explicit fixture only, one request for this game's own window.
                    // On Windows HACK_ForceToFront is simply SetForegroundWindow;
                    // the OS may deny it. Never override FApp or fabricate SDK focus.
                    if(!Window->IsVisible())Window->ShowWindow();
                    Window->BringToFront(true);Window->HACK_ForceToFront();
                    if(Window->GetNativeWindow().IsValid())Window->GetNativeWindow()->SetWindowFocus();
                    FSlateApplication::Get().SetAllUserFocusToGameViewport();Run->FocusRequested=true;
                }
            }
            Run->OriginalHDR=R->GetHDRStatus();Run->OriginalCap=R->GetFrameRateLimit();Run->HasOriginal=true;Run->Initial=R->GetDiagnostics();
            Run->OriginalFG=R->GetFrameGeneration();Run->OriginalReflex=R->GetReflex();
            GConfig->GetBool(TEXT("Warfare.Display"),TEXT("PerformanceMonitor"),Run->OriginalPerformance,GGameUserSettingsIni);
            FConfigFile Saved;bool RR=false;int32 Version=0;const bool Read=DiskSettings(Saved);
            Saved.GetBool(TEXT("Warfare.Render"),TEXT("RayReconstruction"),RR);Saved.GetInt(TEXT("Warfare.Render"),TEXT("RRDefaultsVersion"),Version);
            Check(TEXT("fresh_or_v23_rr_migration_saved"),Read && RR && Version==24 && R->GetRayReconstruction());
            const auto D=R->GetActiveDisplay();const auto Range=R->GetFrameRateRange();
            Check(TEXT("active_window_refresh_uses_current_windows_mode"),D.bValid && D.bRefreshKnown && D.RefreshHz>1 &&
                (D.RefreshSource.Contains(TEXT("QDC_ONLY_ACTIVE_PATHS")) || D.RefreshSource.Contains(TEXT("ENUM_CURRENT_SETTINGS"))) && Range.Y==FMath::RoundToInt(D.RefreshHz),
                FString::Printf(TEXT("%s %.4f Hz; %s"),*D.Name,D.RefreshHz,*D.RefreshSource));
            FFrontlineActiveDisplay Left,Right;Left.Id=TEXT("simulated-panel-left");Right.Id=TEXT("simulated-panel-right");
            Left.Bounds=FIntRect(-1920,0,0,1080);Right.Bounds=FIntRect(0,0,3840,2160);Left.RefreshHz=59.94f;Right.RefreshHz=144;
            Left.PeakNits=400;Right.PeakNits=1400;const TArray<FFrontlineActiveDisplay> Displays{Left,Right};
            Check(TEXT("simulated_window_crossing_selects_overlap_and_current_hz"),
                FrontlineDisplay::SelectByWindow(FIntRect(-1700,0,-100,900),Displays)==0 &&
                FrontlineDisplay::SelectByWindow(FIntRect(-100,0,1600,900),Displays)==1 &&
                FrontlineDisplay::FrameRateRange(Left.RefreshHz).Y==60 && FrontlineDisplay::FrameRateRange(Right.RefreshHz).Y==144,
                TEXT("Pure synthetic display rectangles; not a physical cross-monitor test."));
            Check(TEXT("hdr_defaults_and_profile_keys_are_panel_specific"),FrontlineDisplay::DefaultHDR(Left).PeakNits==400 &&
                FrontlineDisplay::DefaultHDR(Right).PeakNits==1400 && FrontlineDisplay::HDRSection(Left.Id)!=FrontlineDisplay::HDRSection(Right.Id));
            Left.bValid=Left.bHDRStateKnown=Left.bLuminanceKnown=Left.bHDRSupported=Left.bWindowsHDREnabled=true;
            FFrontlineActiveDisplay Failed;Failed.bValid=true;Failed.Id=Left.Id;
            const auto Kept=FrontlineDisplay::PreserveKnownState(Failed,Left);Failed.Id=Right.Id;
            const auto UnknownOther=FrontlineDisplay::PreserveKnownState(Failed,Left);
            Check(TEXT("unknown_hdr_query_does_not_overwrite_or_cross_bind"),Kept.bHDRStateKnown && Kept.bHDRSupported && Kept.PeakNits==400 &&
                Kept.RefreshHz==Left.RefreshHz && !UnknownOther.bHDRStateKnown && UnknownOther.PeakNits==0);
            FFrontlineFrameCapController Limiter;Limiter.Select(1,6,false,0);
            bool CapOK=Limiter.RenderLimit(120)==60;
            for(int32 I=1;I<=6;++I)Limiter.Observe(1,I,I*.02);
            CapOK&=Limiter.RenderLimit(120)==60; // Ignore transient fallback.
            for(int32 I=7;I<=75;++I)Limiter.Observe(1,I,2.+I*.04);
            CapOK&=Limiter.RenderLimit(120)==120;
            Limiter.Observe(2,76,6.);CapOK&=Limiter.RenderLimit(120)==60;
            Limiter.Select(0,6,true,7.);CapOK&=Limiter.RenderLimit(120)==60;
            Limiter.Select(0,6,false,7.1);CapOK&=Limiter.RenderLimit(120)==120;
            Limiter.Select(6,6,false,8.);CapOK&=Limiter.RenderLimit(120)==20;
            Check(TEXT("total_fps_limiter_stability_and_drain_model"),CapOK,TEXT("Synthetic multiplier observations exercise the production controller, not an independent present measurement."));
            Command(TEXT("Settings"));break;
        }
        case 1:
        {
            if(Run->FrameGenerationReview)RecordFocus(TEXT("after_startup_message_pump"));
            bool Continuous=true;
            for(float P:{.11f,.37f,.56f,.73f})
                Continuous&=R->SetFrameRateSlider(P,false) && R->GetFrameRateLimit()==FrontlineDisplay::SliderFrameRate(P,R->GetActiveDisplay().RefreshHz);
            Check(TEXT("continuous_slider_mapping_uses_integer_fps"),Continuous,TEXT("Four synthetic updates through the production drag setter; no preset list."));
            int32 W=0,H=0;PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1920.f,H/1080.f);
            const float X=866.f+.43f*386.f;PC->SetMouseLocation(FMath::RoundToInt((W-1920*Scale)*.5f+X*Scale),FMath::RoundToInt((H-1080*Scale)*.5f+577*Scale));
            Run->ExpectedCap=FrontlineDisplay::SliderFrameRate(.43f,R->GetActiveDisplay().RefreshHz);
            break;
        }
        case 2:
        {
            // SetMouseLocation is a real OS cursor request, not an immutable
            // fixture position. Sample the viewport cursor used by the HUD in
            // this same tick; no intervening message pump or native movement.
            float MouseX=0,MouseY=0;int32 W=0,H=0;PC->GetViewportSize(W,H);
            const bool GotPointer=PC->GetMousePosition(MouseX,MouseY);
            const float Scale=FMath::Min(W/1920.f,H/1080.f);
            const float X=Scale>0?(MouseX-(W-1920.f*Scale)*.5f)/Scale:-1;
            const float Y=Scale>0?(MouseY-(H-1080.f*Scale)*.5f)/Scale:-1;
            const bool OnSlider=GotPointer && Scale>0 && X>=858 && X<=1260 && Y>=554 && Y<=597;
            if(OnSlider)
            {
                const int32 Expected=FrontlineDisplay::SliderFrameRate((X-866.f)/386.f,R->GetActiveDisplay().RefreshHz);
                Command(TEXT("FrameRateSlider"));
                Check(TEXT("hud_slider_click_applies_pointer_position"),FMath::Abs(R->GetFrameRateLimit()-Expected)<=1.f,
                    FString::Printf(TEXT("sampled pointer=(%.2f,%.2f) UI=(%.2f,%.2f), requested target=%d, sampled expected=%d, applied=%.1f, focus=%d; synchronous HUD dispatch, not human drag"),
                        MouseX,MouseY,X,Y,Run->ExpectedCap,Expected,R->GetFrameRateLimit(),FApp::HasFocus()));
            }
            else
            {
                Run->Skips.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("HUD pointer-click fixture unavailable: observed UI pointer=(%.2f,%.2f), valid=%d, focus=%d; real cursor no longer on slider. No click synthesized outside its hit area."),X,Y,GotPointer,FApp::HasFocus())));
            }
            break;
        }
        case 3:
            Run->ExpectedCap=FMath::Min(FMath::RoundToInt(R->GetFrameRateLimit())+1,R->GetFrameRateRange().Y);
            Command(TEXT("FrameRateNext"));break;
        case 4:
        {
            FConfigFile Saved;float Limit=-1,Preference=-1;const bool Read=DiskSettings(Saved);
            Saved.GetFloat(TEXT("/Script/Engine.GameUserSettings"),TEXT("FrameRateLimit"),Limit);
            Saved.GetFloat(TEXT("Warfare.Render"),TEXT("FrameRatePreference"),Preference);
            const auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
            Check(TEXT("one_fps_adjustment_applies_and_saves"),Read && Limit==Run->ExpectedCap && Preference==Limit && R->GetFrameRateLimit()==Limit && C && C->GetFloat()==Limit);
            const float Before=R->GetFrameRateLimit();
            Check(TEXT("invalid_caps_and_nan_slider_are_rejected"),!R->SetFrameRateLimit(-1) && !R->SetFrameRateLimit(R->GetFrameRateRange().Y+1) &&
                !R->SetFrameRateSlider(std::numeric_limits<float>::quiet_NaN()) && R->GetFrameRateLimit()==Before);
            Snap(TEXT("01-current-refresh-slider"));break;
        }
        case 5:Command(TEXT("SettingsHDR"));break;
        case 6:
            Run->ExpectedWhite=FMath::Clamp(Run->OriginalHDR.PaperWhiteNits+10,80.f,400.f);
            Run->BeforeCalibration=R->GetHDRStatus().CalibrationFrames;
            if(R->GetHDRStatus().bSupported){R->SetHDRPaperWhiteNits(Run->ExpectedWhite);Command(TEXT("HDR1"));}
            else Check(TEXT("unsupported_hdr_is_rejected_and_sdr_remains"),!R->SetHDREnabled(true) && !R->GetHDRStatus().bOutputActive,R->GetHDRStatus().Reason);
            Run->Next=Now+1.8;break;
        case 7:
        {
            FConfigFile Saved;float White=-1;FString Id;const auto D=R->GetActiveDisplay();const FString Section=FrontlineDisplay::HDRSection(D.Id);
            const bool Read=DiskSettings(Saved);Saved.GetFloat(*Section,TEXT("PaperWhiteNits"),White);Saved.GetString(*Section,TEXT("DisplayId"),Id);
            const auto H=R->GetHDRStatus();Run->Output=R->GetDiagnostics();
            if(H.bSupported)
            {
                Check(TEXT("hdr_calibration_persists_under_actual_display_id"),Read && !Id.IsEmpty() && Id==D.Id && White==Run->ExpectedWhite);
                Check(TEXT("supported_hdr_reaches_output_and_calibration_callback"),H.bOutputActive && H.CalibrationFrames>Run->BeforeCalibration,
                    FString::Printf(TEXT("format=%d frames=%llu; no physical luminance claim"),H.OutputFormat,H.CalibrationFrames));
            }
            else Run->Skips.Add(MakeShared<FJsonValueString>(TEXT("Native HDR calibration: active monitor does not currently expose HDR.")));
            Snap(TEXT("02-monitor-hdr-calibration"));break;
        }
        case 8:Command(TEXT("SettingsVideo"));break;
        case 9:if(!Run->OriginalPerformance)Command(TEXT("Performance"));break;
        case 10:
        {
            const auto Timing=R->GetFrameTiming();
            if(Timing.bPresentEstimateValid)Check(TEXT("plugin_present_timing_available"),FMath::IsFinite(Timing.ReportedPresentFPS) && Timing.ReportedPresentFPS>0 && Timing.FramesPresentedPerRendered>=1,
                FString::Printf(TEXT("%.2f FPS, %d presented/rendered; plugin estimate, not independent present timestamps"),Timing.ReportedPresentFPS,Timing.FramesPresentedPerRendered));
            else Run->Skips.Add(MakeShared<FJsonValueString>(TEXT("Plugin present timing currently unavailable; HUD must show a dash, not a fabricated FPS.")));
            Snap(TEXT("03-rendered-and-presented-fps"));break;
        }
        case 11:
            if(!Run->FrameGenerationReview){Run->Stage=90;break;}
            RecordFocus(TEXT("before_fg_stage"));
            if(!FApp::HasFocus() || !R->GetCapabilities().FrameGenerationModes.Contains(EFrontlineFrameGeneration::X2))
            {Run->Skips.Add(MakeShared<FJsonValueString>(TEXT("FG cap runtime stage requires foreground and supported FG; no focus is fabricated.")));Run->Stage=90;break;}
            Command(TEXT("DisplayBack"));break;
        case 12:Command(TEXT("NewCampaign"));break;
        case 13:Command(TEXT("FactionBritish"));break;
        case 14:Command(TEXT("Begin"));Run->StageStarted=Now;break;
        case 15:
            if(B->Phase!=EFrontlinePhase::Fighting && Now-Run->StageStarted<8.){--Run->Stage;break;}
            Check(TEXT("fg_cap_fixture_reaches_real_battle"),B->Phase==EFrontlinePhase::Fighting && B->AssetFailure.IsEmpty());
            if(B->Phase!=EFrontlinePhase::Fighting){Run->Stage=90;break;}
            Run->GenerationTarget=FMath::Clamp(90.f,float(R->GetFrameRateRange().X),float(R->GetFrameRateRange().Y));
            R->SetFrameRateLimit(Run->GenerationTarget);R->SetFrameGeneration(EFrontlineFrameGeneration::X2,false);Run->Next=Now+3.;break;
        case 16:
        {
            RecordFocus(TEXT("fg_cap_observation"));
            const auto D=R->GetDiagnostics();const auto Timing=R->GetFrameTiming();
            const double Internal=D->GetNumberField(TEXT("internal_render_frame_limit"));
            Check(TEXT("foreground_fg_uses_total_output_cap"),FApp::HasFocus() && R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2 &&
                Timing.bPresentEstimateValid && Timing.FramesPresentedPerRendered>=2 && Internal*Timing.FramesPresentedPerRendered<=Run->GenerationTarget*1.02 &&
                R->GetFrameRateLimit()==Run->GenerationTarget && FMath::IsNearlyEqual(float(D->GetNumberField(TEXT("dynamic_fg_target_fps"))),Run->GenerationTarget),
                FString::Printf(TEXT("total=%.1f internal=%.3f multiplier=%d plugin_estimate=%.2f; no independent present timestamps"),Run->GenerationTarget,Internal,Timing.FramesPresentedPerRendered,Timing.ReportedPresentFPS));
            Run->Output->SetObjectField(TEXT("fg_cap"),D);
            const FString File=Run->Directory/TEXT("04-foreground-fg-total-cap.png");Run->Screens.Add(File);FScreenshotRequest::RequestScreenshot(File,true,false);
            break;
        }
        case 17:
        {
            const float Before=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))->GetFloat();
            R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);const auto D=R->GetDiagnostics();
            Check(TEXT("fg_off_keeps_internal_cap_until_drain"),D->GetBoolField(TEXT("frame_limit_waiting_for_fg_drain")) &&
                FMath::IsNearlyEqual(float(D->GetNumberField(TEXT("internal_render_frame_limit"))),Before));
            Run->StageStarted=Now;break;
        }
        case 18:
        {
            const auto D=R->GetDiagnostics();
            if(D->GetBoolField(TEXT("frame_limit_waiting_for_fg_drain")) && Now-Run->StageStarted<3.){--Run->Stage;break;}
            Check(TEXT("fg_off_restores_total_cap_after_real_render_drain"),!D->GetBoolField(TEXT("frame_limit_waiting_for_fg_drain")) &&
                FMath::IsNearlyEqual(float(D->GetNumberField(TEXT("internal_render_frame_limit"))),Run->GenerationTarget));
            Run->Stage=90;break;
        }
        default:
        {
            if(Run->HasOriginal && R && G)
            {
                R->SetHDRCalibrationVisible(false);R->SetFrameGeneration(Run->OriginalFG,false);R->SetReflex(Run->OriginalReflex,false);R->SetFrameRateLimit(Run->OriginalCap);
                R->SetHDRBlackNits(Run->OriginalHDR.BlackNits,false);
                G->SetMaximumHDRDisplayNits(Run->OriginalHDR.PeakNits);G->SetHDRPaperWhiteNits(Run->OriginalHDR.PaperWhiteNits);
                G->SetHDRUILuminanceNits(Run->OriginalHDR.UILuminanceNits);G->SetHDRCalibrationUsed(Run->OriginalHDR.bCalibration);
                R->SetHDREnabled(Run->OriginalHDR.bEnabled);
                if(!Run->OriginalPerformance && HUD)Command(TEXT("Performance"));
            }
            bool ScreensOK=Run->Screens.Num()>=3 && Run->Screens.Num()<=4;TArray<TSharedPtr<FJsonValue>> Files;
            for(const auto& File:Run->Screens){ScreensOK&=IFileManager::Get().FileSize(*File)>1024;Files.Add(MakeShared<FJsonValueString>(File));}
            Check(TEXT("display_screenshots_written"),ScreensOK);
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Run->Passed);J->SetArrayField(TEXT("checks"),Run->Checks);
            J->SetArrayField(TEXT("screenshots"),Files);J->SetArrayField(TEXT("skips"),Run->Skips);J->SetStringField(TEXT("settings_ini"),Run->SettingsPath);
            if(Run->Initial)J->SetObjectField(TEXT("initial_render"),Run->Initial);if(Run->Output)J->SetObjectField(TEXT("hdr_output"),Run->Output);
            J->SetNumberField(TEXT("elapsed_seconds"),Now-Run->Start);
            J->SetBoolField(TEXT("foreground_fg_stage_requested"),Run->FrameGenerationReview);
            J->SetBoolField(TEXT("own_window_focus_requested_once"),Run->FocusRequested);
            J->SetBoolField(TEXT("focused_before_startup_request"),Run->FocusBeforeRequest);
            J->SetArrayField(TEXT("actual_focus_observations"),Run->FocusObservations);
            J->SetStringField(TEXT("scope"),TEXT("Isolated display fixture: current Windows monitor query, synthetic crossing and slider updates, HUD dispatch, immutable defaults plus physical INI readback, monitor-specific HDR and three UI screenshots. Optional foreground FG stage checks actual multiplier, internal cap and real drain, with one battle screenshot. Does not establish physical drag/keyboard acceptance, physical multi-monitor switching, panel luminance, independent present timestamps, or absence of brief pacing overshoot."));
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("display-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V24/display-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V24_DISPLAY_REVIEW_COMPLETE passed=%d %s"),Run->Passed,*Run->Directory);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
