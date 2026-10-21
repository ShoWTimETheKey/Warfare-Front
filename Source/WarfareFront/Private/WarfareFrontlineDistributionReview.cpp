#include "WarfareFrontlineDistributionReview.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
struct FDistributionRun
{
    int32 Stage=0,RecruitCommandsBefore=0;
    bool bPassed=true,bExpectNative=false,bSawPreparation=false;
    double Start=FPlatformTime::Seconds(),Next=Start+3.;
    FString Directory,UserDir;
    FIntPoint MenuSize=FIntPoint::ZeroValue,BattleSize=FIntPoint::ZeroValue;
    TArray<TSharedPtr<FJsonValue>> Checks,Images;
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();

    void Check(const FString& Name,bool bGood,const FString& Detail=FString())
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);
        J->SetBoolField(TEXT("passed"),bGood);J->SetStringField(TEXT("detail"),Detail);
        Checks.Add(MakeShared<FJsonValueObject>(J));bPassed&=bGood;
    }
    bool CaptureWritten(const TCHAR* Name,FIntPoint ExpectedSize)
    {
        const FString File=Directory/(FString(Name)+TEXT(".png"));TArray<uint8> Bytes;
        if(!FFileHelper::LoadFileToArray(Bytes,*File) || Bytes.Num()<24)return false;
        auto ReadU32=[&](int32 O){return (uint32(Bytes[O])<<24)|(uint32(Bytes[O+1])<<16)|(uint32(Bytes[O+2])<<8)|uint32(Bytes[O+3]);};
        const bool bPNG=Bytes[0]==137 && Bytes[1]=='P' && Bytes[2]=='N' && Bytes[3]=='G';
        const FIntPoint Size(int32(ReadU32(16)),int32(ReadU32(20)));
        Check(FString(Name)+TEXT("_native_png"),bPNG && Size==ExpectedSize,
            FString::Printf(TEXT("actual=%dx%d viewport=%dx%d"),Size.X,Size.Y,ExpectedSize.X,ExpectedSize.Y));
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),File);
        J->SetNumberField(TEXT("width"),Size.X);J->SetNumberField(TEXT("height"),Size.Y);
        J->SetNumberField(TEXT("bytes"),Bytes.Num());Images.Add(MakeShared<FJsonValueObject>(J));
        return true;
    }
    void Finish()
    {
        Report->SetBoolField(TEXT("success"),bPassed);Report->SetArrayField(TEXT("checks"),Checks);
        Report->SetArrayField(TEXT("screenshots"),Images);Report->SetBoolField(TEXT("expect_native"),bExpectNative);
        Report->SetNumberField(TEXT("elapsed_wall_seconds"),FPlatformTime::Seconds()-Start);
        Report->SetStringField(TEXT("scope"),TEXT("Shipping-capable isolated runtime launch, production menu/loading/recruitment flow after an explicit disposable host campaign start, actual native PNG readback and renderer diagnostics. The selector hold/authentic-input guard is retained; this is not human-input acceptance. Native mode exercises -ngxdisable on this physical GPU; no claim of physical AMD/Intel testing or foreground FG performance."));
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("report.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        FPlatformMisc::RequestExit(false);
    }
};

void CheckInitialResolutions(FDistributionRun& Run)
{
    const TArray<FIntPoint> Modes{FIntPoint(1280,720),FIntPoint(1920,1080),FIntPoint(2560,1440),
        FIntPoint(3440,1440),FIntPoint(3840,1080),FIntPoint(3840,2160),FIntPoint(5120,1440),FIntPoint(7680,4320)};
    for(const FIntPoint Size:{FIntPoint(1920,1080),FIntPoint(2560,1440),FIntPoint(3440,1440),FIntPoint(3840,2160)})
        Run.Check(FString::Printf(TEXT("desktop_default_%dx%d"),Size.X,Size.Y),FrontlineDisplay::InitialResolution(Size,Modes)==Size);
    Run.Check(TEXT("8k_desktop_selects_supported_4k"),FrontlineDisplay::InitialResolution(FIntPoint(7680,4320),Modes)==FIntPoint(3840,2160));
    Run.Check(TEXT("wide_desktop_preserves_aspect_within_budget"),FrontlineDisplay::InitialResolution(FIntPoint(7680,2160),Modes)==FIntPoint(5120,1440));
    Run.Check(TEXT("unknown_modes_keep_known_desktop"),FrontlineDisplay::InitialResolution(FIntPoint(7680,4320),{})==FIntPoint(7680,4320));
    Run.Check(TEXT("invalid_desktop_metrics_have_bounded_default"),FrontlineDisplay::InitialResolution(FIntPoint(0,-1),Modes)==FIntPoint(1920,1080));
}
}

void StartFrontlineDistributionReview(AWarfareFrontlineGameMode* Battle)
{
    if(!Battle || !FParse::Param(FCommandLine::Get(),TEXT("WFDistributionCheck")))return;
    FString UserDir;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty() || FPaths::IsRelative(UserDir)
        || !FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {FPlatformMisc::RequestExit(false);return;}
    auto Run=MakeShared<FDistributionRun>();Run->UserDir=FPaths::ConvertRelativePathToFull(UserDir);
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("DistributionCheck"));
    // The harness must never leave artifacts in the installed payload or real profile.
    if(!FPaths::IsUnderDirectory(Run->Directory,Run->UserDir)) {FPlatformMisc::RequestExit(false);return;}
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    Run->bExpectNative=FParse::Param(FCommandLine::Get(),TEXT("WFDistributionExpectNative"));
    Run->Report->SetStringField(TEXT("user_dir"),Run->UserDir);
    Run->Report->SetStringField(TEXT("saved_dir"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));
    Run->Report->SetStringField(TEXT("campaign_storage_mode"),TEXT("volatile; real campaign not loaded or written"));
    Run->Report->SetStringField(TEXT("normal_campaign_path"),FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("Warfare1917/Progression-v13.json")));
    const FString SettingsPath=FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir());
    Run->Report->SetStringField(TEXT("settings_path"),FPaths::ConvertRelativePathToFull(SettingsPath));
    const FString Probe=Run->Directory/TEXT("write-probe.txt");FString Readback;
    Run->Check(TEXT("isolated_user_directory_writable"),FFileHelper::SaveStringToFile(TEXT("Warfare Front distribution check"),*Probe)
        && FFileHelper::LoadFileToString(Readback,*Probe) && Readback==TEXT("Warfare Front distribution check"));
    CheckInitialResolutions(*Run);
    TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(Battle);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;Run->Next=Now+.2;
        auto* B=Weak.Get();auto* World=B?B->GetWorld():nullptr;
        auto* PC=World?Cast<AWarfareFrontlineController>(World->GetFirstPlayerController()):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        auto* Render=World?UFrontlineRenderSettingsSubsystem::Get(World):nullptr;
        if(Now-Run->Start>55. || !B || !PC || !HUD || !Render)
        {Run->Check(TEXT("runtime_dependencies_and_55_second_deadline"),false);Run->Finish();return false;}
        const int32 Page=HUD->GetPresentationPage();
        if(!B->AssetFailure.IsEmpty())
        {Run->Report->SetStringField(TEXT("asset_failure"),B->AssetFailure);Run->Check(TEXT("installed_runtime_assets"),false);Run->Finish();return false;}
        switch(Run->Stage)
        {
        case 0:
        {
            Run->Check(TEXT("campaign_profile_is_volatile"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            Run->Check(TEXT("fresh_launch_is_main_menu"),Page==0);
            Run->Check(TEXT("temporary_display_settings_are_write_protected"),Render->GetDiagnostics()->GetBoolField(TEXT("settings_write_protected")));
            if(!Run->bPassed){Run->Finish();return false;}
            PC->GetViewportSize(Run->MenuSize.X,Run->MenuSize.Y);
            Run->Report->SetObjectField(TEXT("render_main"),Render->GetDiagnostics());
            if(auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr)
            {
                Run->Report->SetNumberField(TEXT("window_mode"),int32(Settings->GetFullscreenMode()));
                Run->Report->SetNumberField(TEXT("viewport_width"),Run->MenuSize.X);
                Run->Report->SetNumberField(TEXT("viewport_height"),Run->MenuSize.Y);
            }
            FScreenshotRequest::RequestScreenshot(Run->Directory/TEXT("01-main-menu.png"),true,false);
            ++Run->Stage;break;
        }
        case 1:
            if(!Run->CaptureWritten(TEXT("01-main-menu"),Run->MenuSize))break;
            HUD->NotifyHitBoxClick(TEXT("Credits"));
            Run->Check(TEXT("credits_page_opens_from_main_menu"),HUD->GetPresentationPage()==5);
            Run->Stage=100;Run->Next=Now+.6;break;
        case 100:
        {
            const auto* Logo=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/Distribution/T_NVIDIA_Official.T_NVIDIA_Official"));
            Run->Check(TEXT("official_nvidia_logo_texture_installed"),Logo && Logo->GetSizeX()>0 && Logo->GetSizeY()>0);
            if(Logo){Run->Report->SetNumberField(TEXT("official_logo_width"),Logo->GetSizeX());Run->Report->SetNumberField(TEXT("official_logo_height"),Logo->GetSizeY());}
            FScreenshotRequest::RequestScreenshot(Run->Directory/TEXT("03-credits.png"),true,false);
            Run->Stage=101;break;
        }
        case 101:
            if(!Run->CaptureWritten(TEXT("03-credits"),Run->MenuSize))break;
            HUD->NotifyHitBoxClick(TEXT("Back"));Run->Check(TEXT("credits_back_returns_main_menu"),HUD->GetPresentationPage()==0);
            HUD->NotifyHitBoxClick(TEXT("NewCampaign"));Run->Check(TEXT("new_campaign_opens_faction_selection"),HUD->GetPresentationPage()==11);
            Run->Stage=2;Run->Next=Now+.4;break;
        case 2:
            Run->Check(TEXT("disposable_host_campaign_fixture_started"),StartDistributionCampaignFixtureV8222(PC));
            Run->Check(TEXT("british_campaign_opens_operations"),HUD->GetPresentationPage()==1 && B->GetCurrentCampaign() && B->GetPlayerFaction()==EFrontlineFaction::British);
            if(!Run->bPassed){Run->Finish();return false;}
            ++Run->Stage;Run->Next=Now+.4;break;
        case 3:
            HUD->NotifyHitBoxClick(TEXT("V52Node0"));
            Run->Check(TEXT("first_mission_selected"),B->GetCurrentCampaign() && B->GetCurrentCampaign()->SelectedMission==0 && B->GetSelectedCampaignMission());
            ++Run->Stage;break;
        case 4:
            // The operations map rejects deployment while its camera is in
            // flight. Retry the current public control once the page settles;
            // a one-shot legacy Begin click can be discarded by that guard.
            if(Page==1)HUD->NotifyHitBoxClick(TEXT("V52Begin"));
            else if(Page==16)HUD->NotifyHitBoxClick(TEXT("V52DispatchChoice0"));
            else if(Page==17)HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));
            else if(Page==15)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));
            else if(Page==9)Run->bSawPreparation=true;
            else if(Page==3 && B->Phase==EFrontlinePhase::Fighting && !UGameplayStatics::IsGamePaused(World))
            {
                Run->Check(TEXT("production_loading_page_was_rendered"),Run->bSawPreparation);
                Run->Check(TEXT("campaign_battle_loaded_without_asset_failure"),B->AssetFailure.IsEmpty() && B->GetEncounterCampaignMission()!=nullptr);
                Run->RecruitCommandsBefore=B->RecruitCommands;PC->RecruitRifle();
                Run->Check(TEXT("hud_rifle_recruitment_accepted"),B->RecruitCommands==Run->RecruitCommandsBefore+1);
                ++Run->Stage;Run->Next=Now+6.;
            }
            break;
        case 5:
        {
            int32 Living=0;for(const AWarfareFrontlineSoldier* Soldier:B->Units)if(IsValid(Soldier) && Soldier->Team==0 && Soldier->IsAlive())++Living;
            Run->Check(TEXT("recruited_squad_alive_in_running_battle"),Page==3 && Living>0 && !UGameplayStatics::IsGamePaused(World));
            Run->Report->SetNumberField(TEXT("friendly_living_soldiers"),Living);
            Run->Report->SetNumberField(TEXT("battle_elapsed_seconds"),B->Elapsed);
            Run->Report->SetStringField(TEXT("asset_failure"),B->AssetFailure);
            auto D=Render->GetDiagnostics();Run->Report->SetObjectField(TEXT("render_battle"),D);
            Run->Check(TEXT("real_render_frames_observed"),D->GetNumberField(TEXT("observed_view_families"))>8);
            if(Run->bExpectNative)
            {
                const auto* AA=IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
                const FString Upscaler=D->GetStringField(TEXT("observed_upscaler"));
                Run->Report->SetNumberField(TEXT("anti_aliasing_method"),AA?AA->GetInt():-1);
                Run->Check(TEXT("native_test_uses_real_ngxdisable_switch"),FParse::Param(FCommandLine::Get(),TEXT("ngxdisable")));
                Run->Check(TEXT("native_tsr_renderer_observed"),D->GetStringField(TEXT("quality_requested"))==TEXT("Native")
                    && !D->GetBoolField(TEXT("dlss_enabled_api")) && !Upscaler.Contains(TEXT("DLSS")) && AA && AA->GetInt()==4
                    && FMath::IsNearlyEqual(float(D->GetNumberField(TEXT("screen_percentage"))),100.f,.1f));
                Run->Check(TEXT("native_fallback_disables_rr_and_frame_generation"),!D->GetBoolField(TEXT("rr_enabled_api"))
                    && D->GetStringField(TEXT("fg_effective"))==TEXT("Off"));
            }
            PC->GetViewportSize(Run->BattleSize.X,Run->BattleSize.Y);
            FScreenshotRequest::RequestScreenshot(Run->Directory/TEXT("02-battle.png"),true,false);
            ++Run->Stage;break;
        }
        default:
            if(!Run->CaptureWritten(TEXT("02-battle"),Run->BattleSize))break;
            Run->Check(TEXT("three_completed_native_screenshots"),Run->Images.Num()==3);
            Run->Finish();return false;
        }
        return true;
    }));
}
