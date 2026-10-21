#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontline.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericWindow.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

// Only the explicitly isolated display fixture calls this entry point.
void StartFrontlineV24FrameCapReview(AWarfareFrontlineController* Controller)
{
    FString UserDir;
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV24FrameCapOnly")) ||
        !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty())return;
    struct FSample
    {
        int32 Frames=0;double Seconds=0,GPU=0,PluginFPS=0;float MinDt=1,MaxDt=0;
        void Add(double Dt,double Gpu,float Present)
        {++Frames;Seconds+=Dt;GPU+=Gpu;PluginFPS+=Present;MinDt=FMath::Min(MinDt,float(Dt));MaxDt=FMath::Max(MaxDt,float(Dt));}
        double FPS()const{return Seconds>0?Frames/Seconds:0;}
        TSharedRef<FJsonObject> Json()const
        {
            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("frames"),Frames);J->SetNumberField(TEXT("seconds"),Seconds);
            J->SetNumberField(TEXT("render_fps"),FPS());J->SetNumberField(TEXT("mean_gpu_ms"),Frames?GPU/Frames:0);
            J->SetNumberField(TEXT("mean_plugin_present_estimate"),Frames?PluginFPS/Frames:0);
            J->SetNumberField(TEXT("minimum_frame_ms"),Frames?MinDt*1000:0);J->SetNumberField(TEXT("maximum_frame_ms"),MaxDt*1000);return J;
        }
    };
    struct FRun
    {
        int32 Stage=0;double Start=FPlatformTime::Seconds(),Next=Start+3,WarmAt=0,PreviousAt=0,DrainAt=0;
        bool Passed=true,HasOriginal=false,PreviousValid=false,FocusRequested=false,Complete=false,Production=false;
        int32 FocusLosses=0;FSample Samples[2];float Cap=90,OriginalCap=120,OriginalBridge=0;
        bool OriginalHDR=false;EFrontlineFrameGeneration OriginalFG=EFrontlineFrameGeneration::Off;
        EFrontlineReflexMode OriginalReflex=EFrontlineReflexMode::On;
        TArray<TSharedPtr<FJsonValue>> Checks,Skips;TSharedPtr<FJsonObject> Initial,Before,After;
    };
    auto Run=MakeShared<FRun>();Run->Production=FParse::Param(FCommandLine::Get(),TEXT("WFV24FrameCapProduction"));
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC || !PC->GetWorld())return false;
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;
        auto* R=UFrontlineRenderSettingsSubsystem::Get(PC->GetWorld());
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        auto* Bridge=IConsoleManager::Get().FindConsoleVariable(TEXT("t.Streamline.Reflex.PresentationLimit"));
        auto* LastUs=IConsoleManager::Get().FindConsoleVariable(TEXT("t.Streamline.Reflex.LastFrameLimitUs"));
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);J->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Passed&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V24_FRAMECAP %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto ResetSample=[&](){Run->WarmAt=0;Run->PreviousValid=false;Run->PreviousAt=0;};
        if(Now-Run->Start>45 && Run->Stage<90)
        {Run->Skips.Add(MakeShared<FJsonValueString>(TEXT("Timed out before sufficient actual foreground FG frames; incomplete runtime evidence.")));Run->Stage=90;}
        if(!R || !B || !HUD || !Bridge || !LastUs)
        {UE_LOG(LogTemp,Error,TEXT("WF_V24_FRAMECAP missing dependency"));FPlatformMisc::RequestExit(false);return false;}
        switch(Run->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_volatile_campaign"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(!Run->Passed){Run->Stage=90;break;}
            Run->OriginalCap=R->GetFrameRateLimit();Run->OriginalFG=R->GetFrameGeneration();Run->OriginalReflex=R->GetReflex();
            Run->OriginalHDR=R->GetHDRStatus().bEnabled;Run->OriginalBridge=Bridge->GetFloat();Run->Initial=R->GetDiagnostics();Run->HasOriginal=true;
            if(Run->Production)Check(TEXT("production_initial_reflex_cap_matches_user_setting"),FMath::IsNearlyEqual(Bridge->GetFloat(),Run->OriginalCap),
                FString::Printf(TEXT("user=%.1f reflex=%.1f"),Run->OriginalCap,Bridge->GetFloat()));
            if(!R->GetCapabilities().FrameGenerationModes.Contains(EFrontlineFrameGeneration::X2))
            {Run->Skips.Add(MakeShared<FJsonValueString>(TEXT("X2 FG unsupported on this device.")));Run->Stage=90;break;}
            if(!FApp::HasFocus())
            {
                const auto* View=PC->GetWorld()->GetGameViewport();const auto Window=View?View->GetWindow():nullptr;
                if(Window.IsValid() && FSlateApplication::IsInitialized())
                {
                    if(!Window->IsVisible())Window->ShowWindow();Window->BringToFront(true);Window->HACK_ForceToFront();
                    if(Window->GetNativeWindow().IsValid())Window->GetNativeWindow()->SetWindowFocus();
                    FSlateApplication::Get().SetAllUserFocusToGameViewport();Run->FocusRequested=true;
                }
            }
            R->SetHDREnabled(false,false);R->SetHDRCalibrationVisible(false);if(!Run->Production)Bridge->Set(0.f,ECVF_SetByCode);
            HUD->NotifyHitBoxClick(FName(TEXT("NewCampaign")));Run->Stage=1;Run->Next=Now+.5;break;
        }
        case 1:HUD->NotifyHitBoxClick(FName(TEXT("FactionBritish")));Run->Stage=2;Run->Next=Now+.5;break;
        case 2:HUD->NotifyHitBoxClick(FName(TEXT("Begin")));Run->Stage=3;Run->Next=Now+.5;break;
        case 3:
            if(B->Phase!=EFrontlinePhase::Fighting){Run->Next=Now+.2;break;}
            Check(TEXT("real_battle_without_hdr_calibration"),B->AssetFailure.IsEmpty() && !R->GetHDRStatus().bOutputActive);
            Run->Cap=FMath::Clamp(90.f,float(R->GetFrameRateRange().X),float(R->GetFrameRateRange().Y));
            R->SetFrameRateLimit(Run->Cap,false);R->SetFrameGeneration(EFrontlineFrameGeneration::X2,false);ResetSample();
            if(Run->Production)Check(TEXT("production_slider_cap_reaches_reflex_without_fixture_override"),FMath::IsNearlyEqual(Bridge->GetFloat(),Run->Cap));
            Run->Stage=Run->Production?5:4;break;
        case 4:
        case 5:
        {
            const auto Timing=R->GetFrameTiming();const auto* View=PC->GetWorld()->GetGameViewport();const auto* V=View?View->Viewport:nullptr;
            const bool Valid=FApp::HasFocus() && V && V->IsForegroundWindow() && R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2 &&
                Timing.bPresentEstimateValid && Timing.FramesPresentedPerRendered==2;
            if(!Valid)
            {
                if(Run->PreviousValid){++Run->FocusLosses;UE_LOG(LogTemp,Display,TEXT("WF_V24_FRAMECAP measurement excluded: focus=%d effective=%d mult=%d"),FApp::HasFocus(),int32(R->GetEffectiveFrameGeneration()),Timing.FramesPresentedPerRendered);}
                ResetSample();break;
            }
            if(Run->WarmAt==0)Run->WarmAt=Now;
            const double Dt=Now-Run->PreviousAt;
            const int32 Index=Run->Stage-4;
            if(Run->PreviousValid && Now-Run->WarmAt>=3. && Dt>0 && Dt<.25)
                Run->Samples[Index].Add(Dt,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()),Timing.ReportedPresentFPS);
            Run->PreviousAt=Now;Run->PreviousValid=true;
            if(Run->Samples[Index].Seconds<3. || Run->Samples[Index].Frames<60)break;
            const auto D=R->GetDiagnostics();D->SetNumberField(TEXT("submitted_reflex_interval_us"),LastUs->GetInt());
            D->SetNumberField(TEXT("reflex_presentation_limit"),Bridge->GetFloat());
            if(Index==0)
            {
                Run->Before=D;Check(TEXT("baseline_has_actual_foreground_x2_samples"),true,
                    FString::Printf(TEXT("render=%.3f plugin=%.3f interval=%d us"),Run->Samples[0].FPS(),Run->Samples[0].PluginFPS/Run->Samples[0].Frames,LastUs->GetInt()));
                Bridge->Set(Run->Cap,ECVF_SetByCode);ResetSample();Run->Stage=5;break;
            }
            Run->After=D;const double ExpectedRender=Run->Cap/2.;
            Check(TEXT("reflex_receives_total_presentation_interval"),FMath::Abs(LastUs->GetInt()-1000000.f/Run->Cap)<2.f);
            Check(TEXT("foreground_corrected_cap_not_halved_twice"),Run->Samples[1].FPS()>=ExpectedRender*.8 && Run->Samples[1].FPS()<=ExpectedRender*1.08,
                FString::Printf(TEXT("A render=%.3f B render=%.3f expected=%.3f; B GPU=%.3f ms; plugin estimate=%.3f"),
                    Run->Samples[0].FPS(),Run->Samples[1].FPS(),ExpectedRender,Run->Samples[1].GPU/Run->Samples[1].Frames,Run->Samples[1].PluginFPS/Run->Samples[1].Frames));
            if(!Run->Production)Check(TEXT("corrected_path_recovers_from_duplicate_division"),Run->Samples[1].FPS()>Run->Samples[0].FPS()*1.5);
            R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Run->DrainAt=Now;Run->Stage=6;break;
        }
        case 6:
        {
            const auto D=R->GetDiagnostics();
            if(D->GetBoolField(TEXT("frame_limit_waiting_for_fg_drain")) && Now-Run->DrainAt<4.)break;
            Check(TEXT("fg_off_drain_restores_total_engine_cap"),!D->GetBoolField(TEXT("frame_limit_waiting_for_fg_drain")) &&
                FMath::IsNearlyEqual(float(D->GetNumberField(TEXT("internal_render_frame_limit"))),Run->Cap));
            Run->Complete=true;Run->Stage=90;break;
        }
        default:
        {
            if(Run->HasOriginal)
            {
                if(!Run->Production)Bridge->Set(Run->OriginalBridge,ECVF_SetByCode);
                R->SetFrameGeneration(Run->OriginalFG,false);R->SetReflex(Run->OriginalReflex,false);
                R->SetFrameRateLimit(Run->OriginalCap,false);R->SetHDREnabled(Run->OriginalHDR,false);
            }
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Run->Passed && Run->Complete);J->SetBoolField(TEXT("complete"),Run->Complete);
            J->SetArrayField(TEXT("checks"),Run->Checks);J->SetArrayField(TEXT("skips"),Run->Skips);
            J->SetObjectField(TEXT("engine_rate_to_reflex"),Run->Samples[0].Json());J->SetObjectField(TEXT("total_rate_to_reflex"),Run->Samples[1].Json());
            if(Run->Initial)J->SetObjectField(TEXT("initial"),Run->Initial);if(Run->Before)J->SetObjectField(TEXT("before"),Run->Before);if(Run->After)J->SetObjectField(TEXT("after"),Run->After);
            J->SetNumberField(TEXT("focus_losses_excluded"),Run->FocusLosses);J->SetBoolField(TEXT("own_window_focus_request"),Run->FocusRequested);
            J->SetBoolField(TEXT("production_path_without_bridge_override"),Run->Production);
            J->SetNumberField(TEXT("elapsed_seconds"),Now-Run->Start);J->SetStringField(TEXT("scope"),Run->Production?
                TEXT("Production game writes total cap to Reflex; fixture only changes normal game settings, never the bridge. Real SDR battle at X2, warms 3 seconds and samples at least 60 foreground frames over 3 seconds. Plugin present FPS is an estimate, not independent timestamps. No HDR/menu regression sweep."):
                TEXT("Same real SDR battle, fixed X2 and total cap; A engine-rate Reflex input vs B total-rate input. Each segment warms 3 seconds and samples at least 60 real foreground frames over 3 seconds, excluding screenshots/background. Plugin presentation FPS remains an estimate, not independent present timestamps. No HDR or menu regression sweep."));
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));const FString Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V24");
            IFileManager::Get().MakeDirectory(*Directory,true);FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("frame-cap-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V24_FRAMECAP_REVIEW_COMPLETE passed=%d complete=%d"),Run->Passed && Run->Complete,Run->Complete);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
