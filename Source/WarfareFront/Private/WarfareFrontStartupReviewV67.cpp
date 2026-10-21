#include "WarfareFrontStartupReviewV67.h"
#include "WarfareFrontStartupV67.h"
#include "WarfareFrontCursorV67.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineConsoleV79.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFRRUCampaignV82.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Engine/GameViewportClient.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Engine/World.h"

void StartFrontlineStartupReviewV67(AWarfareFrontlineController* Controller)
{
    FString User;
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV67StartupReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||User.IsEmpty())return;
    auto* Battle=Controller->GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
    if(!Battle||!Battle->IsProgressionVolatile())return;
    struct FRun { double Start=FPlatformTime::Seconds(),Ready=0;bool Captured=false,StartAccepted=false;int32 RequestedNation=-1; };
    auto Run=MakeShared<FRun>();TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FString RequestedNation;
    if(FParse::Value(FCommandLine::Get(),TEXT("WFStartupNation="),RequestedNation))
    {
        Run->RequestedNation=-2; // An invalid explicit request cannot pass as the default army.
        if(RequestedNation.Equals(TEXT("British"),ESearchCase::IgnoreCase))Run->RequestedNation=int32(EFRRUNationV81::British);
        else if(RequestedNation.Equals(TEXT("German"),ESearchCase::IgnoreCase))Run->RequestedNation=int32(EFRRUNationV81::German);
        else if(RequestedNation.Equals(TEXT("French"),ESearchCase::IgnoreCase))Run->RequestedNation=int32(EFRRUNationV81::French);
        else if(RequestedNation.Equals(TEXT("Russian"),ESearchCase::IgnoreCase))Run->RequestedNation=int32(EFRRUNationV81::Russian);
    }
    else if(FParse::Param(FCommandLine::Get(),TEXT("WFStartupGerman")))Run->RequestedNation=int32(EFRRUNationV81::German);
    if(Run->RequestedNation>=0)
    {
        const auto Nation=EFRRUNationV81(Run->RequestedNation);
        // Only the existing isolated, volatile review may create this in-memory campaign.
        Run->StartAccepted=Nation==EFRRUNationV81::British||Nation==EFRRUNationV81::German
            ?Battle->StartNewCampaign(Nation==EFRRUNationV81::German?EFrontlineFaction::German:EFrontlineFaction::British,EFrontlineDifficulty::Veteran)
            :FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))&&WarfareFRRUCampaignV82::Start(Battle,Nation,EFrontlineDifficulty::Veteran);
    }
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;const double Now=FPlatformTime::Seconds();
        if(!FrontlineStartupV67::IsWaitingForMenu()&&!Run->Ready)Run->Ready=Now;
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V67/Startup");
        if(Run->Ready&&Now-Run->Ready>1.5&&!Run->Captured)
        {IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/TEXT("menu.png"),true,false);Run->Captured=true;}
        if((Run->Ready&&Now-Run->Ready>30)||Now-Run->Start>70)
        {
            auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("menu_gate_released"),Run->Ready>0);
            Report->SetBoolField(TEXT("hardware_cursors_loaded"),FrontlineCursorV67::Install(PC));
            Report->SetBoolField(TEXT("menu_capture_requested"),Run->Captured);
            const auto Windows=FrontlineStartupV67::WindowDiagnostics();
            Report->SetBoolField(TEXT("splash_observed"),Windows.SplashObserved);
            Report->SetBoolField(TEXT("splash_excluded_from_taskbar"),Windows.SplashExcludedFromTaskbar);
            Report->SetBoolField(TEXT("game_window_observed"),Windows.GameObserved);
            Report->SetBoolField(TEXT("game_shell_style_preserved"),Windows.GameShellStylePreserved);
            Report->SetBoolField(TEXT("game_input_restored"),Windows.InputRestored);
            Report->SetBoolField(TEXT("game_uncloaked"),Windows.GameUncloaked);
            Report->SetBoolField(TEXT("game_foreground"),Windows.GameForegroundV75);
            Report->SetBoolField(TEXT("app_has_focus_v845"),FApp::HasFocus());
            const auto* Viewport=PC->GetWorld()->GetGameViewport();
            Report->SetBoolField(TEXT("viewport_has_focus_v845"),Viewport&&Viewport->Viewport&&Viewport->Viewport->HasFocus());
            Report->SetBoolField(TEXT("viewport_foreground_v845"),Viewport&&Viewport->Viewport&&Viewport->Viewport->IsForegroundWindow());
            Report->SetBoolField(TEXT("focus_handoff_completed_v8310"),Windows.FocusCompletedV8310);
            Report->SetBoolField(TEXT("focus_handoff_cancelled_v8310"),Windows.FocusCancelledV8310);
            Report->SetBoolField(TEXT("splash_initially_foreground_v845"),Windows.SplashInitiallyForegroundV845);
            Report->SetBoolField(TEXT("splash_foreground_before_close_v845"),Windows.SplashForegroundBeforeCloseV845);
            Report->SetBoolField(TEXT("foreground_request_succeeded_v845"),Windows.ForegroundRequestSucceededV845);
            Report->SetNumberField(TEXT("foreground_requests_v8310"),Windows.ForegroundRequestsV8310);
            Report->SetBoolField(TEXT("game_covers_monitor_v8310"),Windows.GameCoversMonitorV8310);
            Report->SetBoolField(TEXT("taskbar_above_game_v8310"),Windows.TaskbarAboveGameV8310);
            if(auto* A=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Report->SetNumberField(TEXT("active_music_players"),A->GetActiveMusicPlayersV75());
            Report->SetNumberField(TEXT("maximum_taskbar_eligible_windows"),Windows.MaximumTaskbarEligibleWindows);
            Report->SetBoolField(TEXT("splash_faction_verified"),Windows.SplashFactionVerifiedV79);
            Report->SetNumberField(TEXT("splash_faction"),Windows.SplashFactionV79);
            const auto* Battle=PC->GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
            const int32 HomeNation=int32(FrontlineConsoleV79::ResolveDisplayNationV82(Battle,true));
            Report->SetNumberField(TEXT("verified_home_nation"),HomeNation);
            Report->SetNumberField(TEXT("requested_nation"),Run->RequestedNation);
            Report->SetBoolField(TEXT("start_accepted"),Run->StartAccepted);
            const bool RequestMatches=Run->RequestedNation==-1||(Run->StartAccepted&&Run->RequestedNation==HomeNation);
            Report->SetBoolField(TEXT("splash_fixed_british_with_independent_menu_faction"),RequestMatches&&Windows.SplashFactionVerifiedV79&&Windows.SplashFactionV79==0);
            Report->SetBoolField(TEXT("native_window_handoff_passed"),Windows.SplashObserved&&Windows.SplashExcludedFromTaskbar&&Windows.GameObserved&&Windows.GameShellStylePreserved&&Windows.InputRestored&&Windows.GameUncloaked&&Windows.MaximumTaskbarEligibleWindows<=1);
            Report->SetBoolField(TEXT("native_foreground_handoff_passed_v8310"),Windows.FocusCompletedV8310&&Windows.GameForegroundV75
                &&Windows.ForegroundRequestsV8310==1&&!Windows.TaskbarAboveGameV8310);
            Report->SetNumberField(TEXT("seconds_since_controller"),Now-Run->Start);
            Report->SetStringField(TEXT("scope"),TEXT("Isolated launch/menu and hardware-cursor resource check. Native startup and HDR handoff require visual inspection; no player save is written."));
            FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            IFileManager::Get().MakeDirectory(*Dir,true);FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("review.json")));
            FPlatformMisc::RequestExit(false);return false;
        }
        return true;
    }),.2f);
}
