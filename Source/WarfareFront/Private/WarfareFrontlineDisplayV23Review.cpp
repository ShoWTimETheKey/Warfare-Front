#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontline.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include <limits>

// An explicitly requested, isolated UI fixture. No combat or performance sweep.
void StartFrontlineV23DisplayReview(AWarfareFrontlineController* Controller)
{
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV23DisplayReview")))return;
    FString UserDir;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty() ||
       FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {UE_LOG(LogTemp,Error,TEXT("V23 display review requires isolated -UserDir and writable display settings (omit WFRenderNoSave)."));return;}
    struct FRun
    {
        int32 Stage=0;double Start=FPlatformTime::Seconds(),Next=Start+3.,HDRWaitStart=0;
        FString Id=FGuid::NewGuid().ToString(),Directory,SettingsPath;
        bool Passed=true,CapturedOriginal=false,HDRSupported=false,Restored=false;
        bool OriginalUISeparate=true;float OriginalFrameCap=120;
        int32 ExpectedPeak=1000;uint64 CalibrationBefore=0,CalibrationAfterClose=0;
        FFrontlineHDRStatus OriginalHDR;
        TSharedPtr<FJsonObject> InitialDiagnostics,HDRDiagnostics;
        TArray<TSharedPtr<FJsonValue>> Checks,Images,Skips;
        TArray<FString> ImageFiles;
    };
    const auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V23/DisplayReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC || !PC->GetWorld())return false;
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;Run->Next=Now+.65;
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        auto* R=UFrontlineRenderSettingsSubsystem::Get(PC->GetWorld());
        auto* G=GEngine?GEngine->GetGameUserSettings():nullptr;
        auto Check=[&](const TCHAR* Name,bool OK,const FString& Detail=FString())
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),OK);J->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Passed&=OK;
            UE_LOG(LogTemp,Display,TEXT("WF_V23_DISPLAY %s %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto Skip=[&](const TCHAR* Name,const FString& Reason)
        {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetStringField(TEXT("reason"),Reason);Run->Skips.Add(MakeShared<FJsonValueObject>(J));};
        auto Command=[&](const TCHAR* Name){HUD->NotifyHitBoxClick(FName(Name));};
        auto Snap=[&](const TCHAR* Name,int32 Page)
        {
            const FString File=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(File,true,false);
            Run->ImageFiles.Add(File);auto J=MakeShared<FJsonObject>();int32 W=0,H=0;PC->GetViewportSize(W,H);
            J->SetStringField(TEXT("file"),File);J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);Run->Images.Add(MakeShared<FJsonValueObject>(J));
            Check(Name,HUD->GetPresentationPage()==Page && B->AssetFailure.IsEmpty(),FString::Printf(TEXT("page=%d AssetFailure=%s"),HUD->GetPresentationPage(),*B->AssetFailure));
        };
        auto ReadDisk=[&](FConfigFile& Saved,FConfigFile& Disk)
        {
            const FConfigBranch* Branch=GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString());
            Run->SettingsPath=FPaths::ConvertRelativePathToFull(Branch && !Branch->IniPath.IsEmpty()?Branch->IniPath:
                FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
            if(!Branch || IFileManager::Get().FileSize(*Run->SettingsPath)<=0)return false;
            Saved=Branch->CombinedStaticLayers;Saved.Branch=nullptr;Disk.Read(Run->SettingsPath);
            return Saved.Combine(Run->SettingsPath); // Immutable defaults + actual physical diff only.
        };
        auto Restore=[&]()
        {
            if(!Run->CapturedOriginal || !R || !G || Run->Restored)return;
            R->SetHDRCalibrationVisible(false);R->SetFrameRateLimit(Run->OriginalFrameCap);
            R->SetHDRBlackNits(Run->OriginalHDR.BlackNits,false);
            G->SetMaximumHDRDisplayNits(Run->OriginalHDR.PeakNits);
            G->SetHDRPaperWhiteNits(Run->OriginalHDR.PaperWhiteNits);G->SetHDRUILuminanceNits(Run->OriginalHDR.UILuminanceNits);
            G->SetHDRCalibrationUsed(Run->OriginalHDR.bCalibration);G->SetHDRUILuminanceSeparate(Run->OriginalUISeparate);
            R->SetHDREnabled(Run->OriginalHDR.bEnabled);G->SetHDRUILuminanceSeparate(Run->OriginalUISeparate);
            G->ApplyNonResolutionSettings();G->SaveSettings();Run->Restored=true;
        };
        if(Run->Stage<90 && (!B || !HUD || !R || !G || Now-Run->Start>32.))
        {Check(TEXT("review_dependencies_and_time_bound"),false);Run->Stage=90;}
        switch(Run->Stage++)
        {
        case 0:
            Run->OriginalHDR=R->GetHDRStatus();Run->OriginalFrameCap=R->GetFrameRateLimit();
            Run->OriginalUISeparate=G->IsHDRUILuminanceSeparate();Run->CapturedOriginal=true;Run->InitialDiagnostics=R->GetDiagnostics();
            Check(TEXT("campaign_profile_is_volatile"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"),B->GetProgressionSaveStatus());
            if(B->GetProgressionSaveStatus()!=TEXT("独立验证档案")){Run->Stage=90;break;}
            Command(TEXT("NewCampaign"));break;
        case 1:Check(TEXT("new_campaign_opens_faction_selection"),HUD->GetPresentationPage()==11);Command(TEXT("FactionBritish"));break;
        case 2:
            Check(TEXT("campaign_map_has_current_operation"),B->GetCurrentCampaign() && B->GetSelectedCampaignMission() && B->GetCampaignMissions().Num()>0);
            Snap(TEXT("01-campaign-map"),1);break;
        case 3:Command(TEXT("Technology"));break;
        case 4:
        {
            Check(TEXT("technology_board_contains_16_nodes"),B->GetTechnologyNodes().Num()==16);
            int32 W=0,H=0;PC->GetViewportSize(W,H);const float S=FMath::Min(W/1920.f,H/1080.f);
            PC->SetMouseLocation(FMath::RoundToInt((W-1920*S)*.5f+480*S),FMath::RoundToInt((H-1080*S)*.5f+760*S));break;
        }
        case 5:Snap(TEXT("02-technology-16-nodes"),8);break;
        case 6:Command(TEXT("TechnologyBack"));break;
        case 7:Command(TEXT("Settings"));R->SetFrameRateLimit(60,false);break;
        case 8:Command(TEXT("FrameRateNext"));break;
        case 9:
        {
            FConfigFile Saved,Disk;const bool ReadOK=ReadDisk(Saved,Disk);float Limit=-1,Raw=-1;
            const bool Has=Saved.GetFloat(TEXT("/Script/Engine.GameUserSettings"),TEXT("FrameRateLimit"),Limit);
            const bool Explicit=Disk.GetFloat(TEXT("/Script/Engine.GameUserSettings"),TEXT("FrameRateLimit"),Raw);
            const auto* Cap=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
            Check(TEXT("frame_cap_hud_sets_and_saves_90"),ReadOK && Has && FMath::IsNearlyEqual(Limit,90.f) &&
                FMath::IsNearlyEqual(R->GetFrameRateLimit(),90.f) && Cap && FMath::IsNearlyEqual(Cap->GetFloat(),90.f),
                FString::Printf(TEXT("disk+defaults=%.1f source=%s path=%s"),Limit,Explicit?TEXT("disk"):TEXT("default"),*Run->SettingsPath));
            Check(TEXT("frame_cap_rejects_invalid_values_without_change"),!R->SetFrameRateLimit(-1) && !R->SetFrameRateLimit(10000) &&
                !R->SetFrameRateLimit(std::numeric_limits<float>::quiet_NaN()) && FMath::IsNearlyEqual(R->GetFrameRateLimit(),90.f));
            Snap(TEXT("03-display-frame-cap"),7);break;
        }
        case 10:Command(TEXT("SettingsHDR"));break;
        case 11:
        {
            const auto H=R->GetHDRStatus();Run->HDRSupported=H.bSupported;Run->CalibrationBefore=H.CalibrationFrames;
            if(H.bSupported){Command(TEXT("HDR1"));Run->HDRWaitStart=Now;}
            else
            {
                Check(TEXT("unsupported_hdr_enable_is_rejected"),!R->SetHDREnabled(true) && !R->GetHDRStatus().bOutputActive,H.Reason);
                Skip(TEXT("native_hdr_output_and_calibration"),H.Reason);
            }break;
        }
        case 12:
        {
            const auto H=R->GetHDRStatus();
            if(Run->HDRSupported && !H.bOutputActive && Now-Run->HDRWaitStart<3.){--Run->Stage;break;}
            if(Run->HDRSupported)Check(TEXT("supported_hdr_enables_actual_output"),H.bEnabled && H.bOutputActive && H.OutputFormat>=3 && H.OutputFormat<=6,H.OutputLabel);
            break;
        }
        case 13:
            Run->ExpectedPeak=FMath::Clamp(R->GetHDRStatus().PeakNits+50,200,4000);
            if(Run->HDRSupported)Command(TEXT("HDRPeakUp"));break;
        case 14:
        {
            const auto H=R->GetHDRStatus();Run->HDRDiagnostics=R->GetDiagnostics();
            if(Run->HDRSupported)
            {
                FConfigFile Saved,Disk;const bool ReadOK=ReadDisk(Saved,Disk);int32 Peak=-1;bool Enabled=false;
                Saved.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("HDRDisplayOutputNits"),Peak);
                Saved.GetBool(TEXT("/Script/Engine.GameUserSettings"),TEXT("bUseHDRDisplayOutput"),Enabled);
                Check(TEXT("hdr_peak_hud_applies_and_persists"),ReadOK && Enabled && Peak==Run->ExpectedPeak && H.PeakNits==Run->ExpectedPeak);
                Check(TEXT("native_hdr_backbuffer_calibration_executed"),H.bOutputActive && H.CalibrationFrames>Run->CalibrationBefore,
                    FString::Printf(TEXT("frames=%llu; SDR screenshot does not establish panel luminance"),H.CalibrationFrames));
            }
            Snap(TEXT("04-hdr-output-and-calibration"),7);break;
        }
        case 15:Command(TEXT("DisplayBack"));Run->Next=Now+.9;break;
        case 16:Run->CalibrationAfterClose=R->GetHDRStatus().CalibrationFrames;Run->Next=Now+.9;break;
        case 17:
            Check(TEXT("leaving_hdr_page_stops_calibration"),HUD->GetPresentationPage()!=7 && R->GetHDRStatus().CalibrationFrames==Run->CalibrationAfterClose);
            Restore();Run->Next=Now+1.2;break;
        case 18:
        {
            const auto H=R->GetHDRStatus();
            Check(TEXT("original_hdr_and_frame_cap_restored"),H.bEnabled==Run->OriginalHDR.bEnabled &&
                H.bOutputActive==Run->OriginalHDR.bOutputActive && FMath::IsNearlyEqual(R->GetFrameRateLimit(),Run->OriginalFrameCap));break;
        }
        default:
        {
            Restore();bool FilesOK=Run->ImageFiles.Num()==4;for(const auto& File:Run->ImageFiles)FilesOK&=IFileManager::Get().FileSize(*File)>1024;
            Check(TEXT("four_ui_screenshots_written"),FilesOK);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("run_id"),Run->Id);J->SetBoolField(TEXT("passed"),Run->Passed);
            J->SetArrayField(TEXT("checks"),Run->Checks);J->SetArrayField(TEXT("skips"),Run->Skips);J->SetArrayField(TEXT("screenshots"),Run->Images);
            J->SetStringField(TEXT("settings_ini"),Run->SettingsPath);J->SetNumberField(TEXT("elapsed_seconds"),Now-Run->Start);
            if(Run->InitialDiagnostics)J->SetObjectField(TEXT("initial_render"),Run->InitialDiagnostics);
            if(Run->HDRDiagnostics)J->SetObjectField(TEXT("hdr_review"),Run->HDRDiagnostics);
            J->SetStringField(TEXT("scope"),TEXT("Four UI screenshots and spaced HUD dispatches; volatile campaign and isolated UserDir. Frame cap is checked against engine CVar and immutable defaults plus the physical INI diff. Supported HDR must reach the real output and calibration backbuffer callback; unsupported HDR is an explicit skip. HDR/limits are restored. No physical mouse, process-restart persistence, measured panel nits, combat, or performance acceptance is inferred."));
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("display-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V23/display-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V23_DISPLAY_REVIEW_COMPLETE passed=%d %s"),Run->Passed,*Run->Directory);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
