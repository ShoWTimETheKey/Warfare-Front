#include "WarfareFrontline.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AWarfareFrontlineGameMode::BeginV22ImageReview()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV22ImageReview")))return;
    struct FRun
    {
        int32 Stage=0,Passed=0,Skipped=0;
        double Started=FPlatformTime::Seconds(),Next=Started+3.,SwitchAt=0,ReflexOffRequestedAt=0;
        FString Directory,UserDirectory,SettingsPath;
        TArray<TSharedPtr<FJsonValue>> Checks,Images,Diagnostics;
        bool Isolated=false,HasFG=false;
        FString FGReason;
        void Check(const FString& Name,bool OK)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),OK);
            Checks.Add(MakeShared<FJsonValueObject>(J));Passed+=OK;
            UE_LOG(LogTemp,Display,TEXT("WF_V22_IMAGE %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);
        }
        void Skip(const FString& Name,const FString& Reason)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("skipped"),true);
            J->SetStringField(TEXT("reason"),Reason);Checks.Add(MakeShared<FJsonValueObject>(J));++Skipped;
            UE_LOG(LogTemp,Display,TEXT("WF_V22_IMAGE SKIP %s: %s"),*Name,*Reason);
        }
        void CheckFG(const FString& Name,bool OK)
        {if(HasFG)Check(Name,OK);else Skip(Name,FGReason);}
        void CheckActiveFG(const FString& Name,bool OK)
        {
            if(!HasFG)Skip(Name,FGReason);
            else if(!FApp::HasFocus())Skip(Name,TEXT("Application is in the background; actual frame generation requires a foreground window and was not tested."));
            else Check(Name,OK);
        }
    };
    auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V22/ImageReview")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    const bool HasUserDir=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Run->UserDirectory)&&!Run->UserDirectory.IsEmpty();
    Run->SettingsPath=FPaths::ConvertRelativePathToFull(FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
    Run->Isolated=bProgressionVolatile&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&HasUserDir
        &&FPaths::IsUnderDirectory(Run->SettingsPath,FPaths::ConvertRelativePathToFull(Run->UserDirectory));
    TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* B=Weak.Get();if(!B)return false;
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;Run->Next=Now+.6;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        auto* Rendering=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr;
        if(Run->Stage<99&&(!Run->Isolated||!B->AssetFailure.IsEmpty()||Now-Run->Started>55.))
        {Run->Check(TEXT("isolated_settings_resources_and_timeout"),false);Run->Stage=99;}
        if(Run->Stage<99&&(!PC||!HUD||!Rendering||!Settings))return true;
        auto Command=[&](const FString& Name){if(HUD)HUD->NotifyHitBoxClick(FName(*Name));};
        auto Sample=[&](const TCHAR* Label)
        {
            auto J=Rendering->GetDiagnostics();J->SetStringField(TEXT("stage"),Label);
            J->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);J->SetNumberField(TEXT("presentation_page"),HUD->GetPresentationPage());
            J->SetBoolField(TEXT("app_has_focus"),FApp::HasFocus());
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            J->SetNumberField(TEXT("viewport_width"),Width);J->SetNumberField(TEXT("viewport_height"),Height);
            Run->Diagnostics.Add(MakeShared<FJsonValueObject>(J));return J;
        };
        auto Snap=[&](const TCHAR* Label)
        {
            const FString Path=Run->Directory/(FString(Label)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,true,false);
            Run->Images.Add(MakeShared<FJsonValueString>(Path));Run->Next=Now+1.2;
        };
        auto OpenImage=[&](){Command(TEXT("Settings"));Command(TEXT("SettingsImage"));};
        // Select output using the same public enumeration presented to the player,
        // then dispatch real arrow/apply buttons. Actual viewport is checked later.
        auto WindowOutput=[&](FIntPoint Wanted)
        {
            Command(TEXT("SettingsVideo"));Command(TEXT("WindowMode0"));
            TArray<FIntPoint> Modes,Available;UKismetSystemLibrary::GetSupportedFullscreenResolutions(Available);
            for(const auto R:Available)if(R.X>=1280&&R.Y>=720)Modes.AddUnique(R);
            if(Modes.IsEmpty())for(const auto R:{FIntPoint(1280,720),FIntPoint(1920,1080),FIntPoint(2560,1440),Settings->GetDesktopResolution()})
                if(R.X>0&&R.Y>0)Modes.AddUnique(R);
            const FIntPoint Current=Settings->GetScreenResolution();Modes.AddUnique(Current);
            Modes.Sort([](const FIntPoint& A,const FIntPoint& C){return A.X==C.X?A.Y<C.Y:A.X<C.X;});
            const int32 To=Modes.IndexOfByKey(Wanted),From=Modes.IndexOfByKey(Current);
            if(To==INDEX_NONE||From==INDEX_NONE)return false;
            for(int32 I=0;I<(To-From+Modes.Num())%Modes.Num();++I)Command(TEXT("ResolutionNext"));
            Run->SwitchAt=FPlatformTime::Seconds();Command(TEXT("VideoApply"));Command(TEXT("DisplayBack"));
            return true;
        };
        auto OutputIs=[&](FIntPoint Wanted)
        {
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            return Settings->GetFullscreenMode()==EWindowMode::Windowed&&Settings->GetScreenResolution()==Wanted
                &&Width==Wanted.X&&Height==Wanted.Y;
        };
        switch(Run->Stage++)
        {
        case 0:
            Run->Check(TEXT("volatile_progress_and_settings_under_explicit_userdir"),Run->Isolated);
            PC->Restart();Command(TEXT("NewCampaign"));break;
        case 1:Command(TEXT("FactionBritish"));break;
        case 2:Command(TEXT("Begin"));break;
        case 3:
            if(B->Phase!=EFrontlinePhase::Fighting){--Run->Stage;break;}
            Run->Check(TEXT("normal_preparation_enters_real_battle_page"),HUD->GetPresentationPage()==3);
            Run->HasFG=Rendering->GetCapabilities().FrameGenerationModes.Contains(EFrontlineFrameGeneration::X2);
            Run->FGReason=Rendering->GetCapabilities().FrameGenerationReason;
            if(Run->FGReason.IsEmpty())Run->FGReason=TEXT("2x frame generation is not exposed by current device/plugin capability.");
            // Small arranged fixture only: retain actual character animation/rendering,
            // stop strategic simulation so a settings test cannot end the encounter.
            for(int32 Team=0;Team<2;++Team)
            {
                B->SpawnSquad(Team,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(Team));
                B->SpawnSquad(Team,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(Team));
            }
            B->SetActorTickEnabled(false);PC->ResetCamera();
            OpenImage();Command(TEXT("ImageGeneration0"));Command(TEXT("ImageReconstruction0"));
            Command(TEXT("ImageReflex0"));Command(TEXT("ImageQuality0"));break;
        case 4:
        {
            const auto J=Sample(TEXT("native"));
            Run->Check(TEXT("native_button_disables_dlss_and_rr"),Rendering->GetQuality()==EFrontlineRenderQuality::Native
                &&!J->GetBoolField(TEXT("dlss_enabled_api"))&&!J->GetBoolField(TEXT("rr_enabled_api"))
                &&!J->GetStringField(TEXT("observed_upscaler")).Contains(TEXT("DLSS")));
            Command(TEXT("ImageQuality2"));Run->Next=Now+1.2;break;
        }
        case 5:
        {
            const auto J=Sample(TEXT("quality_settings"));
            Run->Check(TEXT("quality_button_uses_actual_dlss_view_family"),HUD->GetPresentationPage()==7
                &&Rendering->GetQuality()==EFrontlineRenderQuality::Quality&&J->GetBoolField(TEXT("dlss_enabled_api"))
                &&J->GetStringField(TEXT("observed_upscaler")).Contains(TEXT("DLSS"))&&J->GetNumberField(TEXT("screen_percentage"))<100.);
            Snap(TEXT("01-quality-settings"));break;
        }
        case 6:Command(TEXT("ImageQuality1"));Run->Next=Now+1.2;break;
        case 7:
        {
            const auto J=Sample(TEXT("dlaa_settings"));
            Run->Check(TEXT("dlaa_button_uses_dlss_at_native_fraction"),Rendering->GetQuality()==EFrontlineRenderQuality::DLAA
                &&J->GetBoolField(TEXT("dlss_enabled_api"))&&J->GetStringField(TEXT("observed_upscaler")).Contains(TEXT("DLSS"))
                &&FMath::Abs(J->GetNumberField(TEXT("screen_percentage"))-100.)<.05);
            Command(TEXT("ImageQuality2"));Command(TEXT("ImageGeneration1"));break;
        }
        case 8:
            Run->CheckFG(TEXT("fg_2x_button_sets_reflex_and_waits_while_settings_open"),Rendering->GetFrameGeneration()==EFrontlineFrameGeneration::X2
                &&Rendering->GetReflex()==EFrontlineReflexMode::On&&Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            Command(TEXT("ImageReflex0"));
            Run->CheckFG(TEXT("reflex_cannot_be_disabled_while_fg_requested"),Rendering->GetReflex()==EFrontlineReflexMode::On);
            Command(TEXT("DisplayBack"));Run->Next=FPlatformTime::Seconds()+2.2;break;
        case 9:
        {
            const auto J=Sample(TEXT("fg_battle"));
            Run->CheckActiveFG(TEXT("fg_2x_effective_and_multiple_frames_presented_after_battle_return"),HUD->GetPresentationPage()==3
                &&Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2
                &&J->GetNumberField(TEXT("frames_presented_per_rendered"))>=2.);
            Snap(TEXT("02-fg-battle"));break;
        }
        case 10:Command(TEXT("Menu"));break;
        case 11:
            Sample(TEXT("fg_menu_paused"));
            Run->CheckFG(TEXT("menu_pauses_fg_but_retains_requested_2x"),HUD->GetPresentationPage()==4&&UGameplayStatics::IsGamePaused(B)
                &&Rendering->GetFrameGeneration()==EFrontlineFrameGeneration::X2&&Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            Snap(TEXT("03-fg-menu-paused"));break;
        case 12:OpenImage();break;
        case 13:
            Run->Check(TEXT("nested_menu_settings_stays_paused"),HUD->GetPresentationPage()==7&&UGameplayStatics::IsGamePaused(B)
                &&Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            Command(TEXT("DisplayBack"));break;
        case 14:
            Run->Check(TEXT("return_from_settings_preserves_menu_pause"),HUD->GetPresentationPage()==4&&UGameplayStatics::IsGamePaused(B));
            Command(TEXT("Resume"));Run->Next=FPlatformTime::Seconds()+2.2;break;
        case 15:
        {
            const auto J=Sample(TEXT("fg_menu_resume"));
            Run->CheckActiveFG(TEXT("resume_restores_fg_and_multiple_presented_frames"),HUD->GetPresentationPage()==3&&!UGameplayStatics::IsGamePaused(B)
                &&Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2
                &&J->GetNumberField(TEXT("frames_presented_per_rendered"))>=2.);
            OpenImage();Command(TEXT("ImageReconstruction1"));Run->Next=Now+1.2;break;
        }
        case 16:
        {
            const auto J=Sample(TEXT("rr_capability_and_actual_observation_only"));
            const bool Supported=Rendering->GetCapabilities().bRayReconstruction;
            J->SetBoolField(TEXT("rr_capable"),Supported);
            Run->Check(TEXT("rr_button_matches_reported_capability_no_visual_quality_claim"),Rendering->GetRayReconstruction()==Supported
                &&J->GetBoolField(TEXT("rr_enabled_api"))==Supported);
            Snap(TEXT("04-rr-settings-observation"));break;
        }
        case 17:
            Command(TEXT("ImageReconstruction0"));
            Run->Check(TEXT("hud_selects_1440_window_output"),WindowOutput(FIntPoint(2560,1440)));
            Run->CheckFG(TEXT("1440_switch_immediately_suspends_fg"),Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            Run->Next=Now+.35;break;
        case 18:
            if(FPlatformTime::Seconds()-Run->SwitchAt<1.)Run->CheckFG(TEXT("1440_switch_cooldown_keeps_fg_off_before_one_second"),
                Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            else Run->Skip(TEXT("1440_switch_cooldown_keeps_fg_off_before_one_second"),TEXT("Resize/frame stall consumed the observation interval; no early sample was available."));
            Run->Next=FPlatformTime::Seconds()+2.2;break;
        case 19:
        {
            const auto J=Sample(TEXT("1440_window_resumed"));
            Run->Check(TEXT("actual_1440_window_output"),OutputIs(FIntPoint(2560,1440)));
            Run->CheckActiveFG(TEXT("1440_fg_restored_with_multiple_presented_frames"),Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2
                &&J->GetNumberField(TEXT("frames_presented_per_rendered"))>=2.);
            OpenImage();
            Run->Check(TEXT("hud_selects_4k_window_output"),WindowOutput(FIntPoint(3840,2160)));
            Run->CheckFG(TEXT("4k_switch_immediately_suspends_fg"),Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            Run->Next=Now+.35;break;
        }
        case 20:
            if(FPlatformTime::Seconds()-Run->SwitchAt<1.)Run->CheckFG(TEXT("4k_switch_cooldown_keeps_fg_off_before_one_second"),
                Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            else Run->Skip(TEXT("4k_switch_cooldown_keeps_fg_off_before_one_second"),TEXT("Resize/frame stall consumed the observation interval; no early sample was available."));
            Run->Next=FPlatformTime::Seconds()+2.2;break;
        case 21:
        {
            const auto J=Sample(TEXT("4k_window_resumed"));
            Run->Check(TEXT("actual_4k_window_output"),OutputIs(FIntPoint(3840,2160)));
            Run->CheckActiveFG(TEXT("4k_fg_restored_with_multiple_presented_frames"),Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::X2
                &&J->GetNumberField(TEXT("frames_presented_per_rendered"))>=2.);
            Run->ReflexOffRequestedAt=Now;
            OpenImage();Command(TEXT("ImageGeneration0"));Command(TEXT("ImageReflex0"));Command(TEXT("DisplayBack"));break;
        }
        case 22:
        {
            const auto State=Rendering->GetDiagnostics();
            const bool ActuallyOff=State->GetNumberField(TEXT("reflex_effective"))==0 && !State->GetBoolField(TEXT("reflex_disable_pending"));
            if(!ActuallyOff && Now-Run->ReflexOffRequestedAt<2.)
            {--Run->Stage;Run->Next=Now+.1;return true;}
            Sample(TEXT("fg_off_after_hud_selection"));
            Run->Check(TEXT("fg_off_button_and_independent_reflex_off_apply"),Rendering->GetFrameGeneration()==EFrontlineFrameGeneration::Off
                &&Rendering->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off&&Rendering->GetReflex()==EFrontlineReflexMode::Off&&ActuallyOff);
            OpenImage();Snap(TEXT("05-final-4k-image-settings"));break;
        }
        default:
        {
            int32 Written=0;for(const auto& V:Run->Images)if(IFileManager::Get().FileSize(*V->AsString())>0)++Written;
            Run->Check(TEXT("five_settings_battle_menu_screenshots_written"),Written==5);
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Run->Passed+Run->Skipped==Run->Checks.Num());
            J->SetBoolField(TEXT("all_requested_features_verified"),Run->Skipped==0&&Run->Passed==Run->Checks.Num());
            J->SetNumberField(TEXT("checks_skipped"),Run->Skipped);J->SetBoolField(TEXT("fg_2x_capable"),Run->HasFG);
            J->SetNumberField(TEXT("checks_passed"),Run->Passed);J->SetNumberField(TEXT("checks_total"),Run->Checks.Num());
            J->SetArrayField(TEXT("checks"),Run->Checks);J->SetArrayField(TEXT("screenshots"),Run->Images);
            J->SetArrayField(TEXT("diagnostics"),Run->Diagnostics);J->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);
            J->SetStringField(TEXT("game_user_settings_ini"),Run->SettingsPath);J->SetStringField(TEXT("settings_config_key"),GGameUserSettingsIni);J->SetStringField(TEXT("user_dir"),Run->UserDirectory);
            J->SetStringField(TEXT("scope"),TEXT("Isolated real HUD image settings dispatch, actual view-family upscaler observations, FG state transitions, output viewport checks and screenshots. Four arranged squads with strategic simulation frozen; character animation remains active. No OS cursor control, subjective RR image-quality approval, independent generated-present count, benchmark, save migration or long stress test. Requires explicit isolated UserDir and WFRenderNoSave."));
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("image-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V22/image-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V22_IMAGE_COMPLETE passed=%d skipped=%d total=%d %s"),Run->Passed,Run->Skipped,Run->Checks.Num(),*Run->Directory);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
