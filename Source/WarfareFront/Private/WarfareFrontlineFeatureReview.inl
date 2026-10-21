// Opt-in functional test of the actual presentation/controller/engine paths.
// The launcher supplies an isolated -UserDir and progression selects its QA file.
void AWarfareFrontlineGameMode::BeginV13FeatureReview()
{
    struct FRun
    {
        int32 Stage=0; double Next=FPlatformTime::Seconds()+2.,BattleStarted=0,PreparationRequestedAt=0; float NextOrder=0,NextRecruit=0;
        int32 RecruitRole=1;
        bool Readback=FParse::Param(FCommandLine::Get(),TEXT("WFV13Readback"));
        bool All=true;
        FString Id=FGuid::NewGuid().ToString();
        FString Directory;
        TArray<TSharedPtr<FJsonValue>> Checks,Images;
    };
    const auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V13/FeatureReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    const TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* B=Weak.Get(); if (!B) return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        if (!PC || !HUD || FPlatformTime::Seconds()<Run->Next) return true;
        auto& S=ScreenState(PC); auto* G=GEngine->GetGameUserSettings();
        auto Check=[&](const TCHAR* Name,bool OK,const FString& Detail=FString())
        {
            const auto C=MakeShared<FJsonObject>(); C->SetStringField(TEXT("name"),Name); C->SetBoolField(TEXT("passed"),OK); C->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(C)); Run->All&=OK;
            UE_LOG(LogTemp,Display,TEXT("WF_V13_FEATURE %s %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto Snap=[&](const TCHAR* Name)
        {
            const FString File=Run->Directory/(FString(Name)+TEXT(".png"));
            FScreenshotRequest::RequestScreenshot(File,true,false); Run->Images.Add(MakeShared<FJsonValueString>(File));
        };
        auto Command=[&](const TCHAR* Name){HUD->NotifyHitBoxClick(FName(Name));};
        auto ChooseResolution=[&](FIntPoint Wanted)
        {
            for (int32 I=0;I<S.ResolutionChoices.Num() && S.DisplayResolution!=Wanted;++I) Command(TEXT("ResolutionNext"));
            return S.DisplayResolution==Wanted;
        };
        auto Finish=[&]()
        {
            const auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("run_id"),Run->Id);
            R->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());
            R->SetStringField(TEXT("scope"),TEXT("Actual HUD dispatch, controller and engine window/settings paths; QA progression and launch UserDir. Script commands are not physical mouse validation. Natural German encounter follows legal orders; no health, morale, position or cooldown edits. Screenshots are not performance measurements."));
            R->SetBoolField(TEXT("passed"),Run->All); R->SetBoolField(TEXT("readback_process"),Run->Readback);
            R->SetStringField(TEXT("settings_ini"),GGameUserSettingsIni); R->SetArrayField(TEXT("checks"),Run->Checks); R->SetArrayField(TEXT("screenshots"),Run->Images);
            R->SetNumberField(TEXT("shots"),B->ShotsFired); R->SetNumberField(TEXT("recruit_orders"),B->RecruitCommands);
            R->SetStringField(TEXT("player_faction"),B->GetFactionName());
            R->SetNumberField(TEXT("research_points_at_exit"),B->GetResearchPoints());
            FString Json; FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("feature-verification.json")));
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V13")/(Run->Readback?TEXT("feature-readback.json"):TEXT("feature-verification.json"))));
            UE_LOG(LogTemp,Display,TEXT("WF_V13_FEATURE_COMPLETE %s Passed=%d"),*Run->Directory,Run->All); FPlatformMisc::RequestExit(false);
        };
        if (Run->Stage==100) { Finish(); return false; }
        Run->Next=FPlatformTime::Seconds()+1.3;
        if (Run->Readback)
        {
            Check(TEXT("german_faction_survives_process_restart"),B->GetPlayerFaction()==EFrontlineFaction::German);
            FString PriorJson; TSharedPtr<FJsonObject> PriorReport; double PriorPoints=-1;
            const bool PriorOK=FFileHelper::LoadFileToString(PriorJson,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V13/feature-verification.json")))
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PriorJson),PriorReport) && PriorReport.IsValid()
                && PriorReport->TryGetNumberField(TEXT("research_points_at_exit"),PriorPoints);
            Check(TEXT("purchased_technology_survives_process_restart"),PriorOK && B->HasTechnology(0) && B->HasTechnology(2) && B->GetResearchPoints()==int32(PriorPoints));
            Check(TEXT("performance_preference_survives_process_restart"),S.Performance);
            Check(TEXT("display_resolution_survives_process_restart"),G->GetScreenResolution()==FIntPoint(1920,1080));
            Check(TEXT("window_mode_survives_process_restart"),G->GetFullscreenMode()==EWindowMode::Windowed);
            int32 W=0,H=0; PC->GetViewportSize(W,H); Check(TEXT("actual_restart_viewport_matches_saved_resolution"),W==1920 && H==1080,FString::Printf(TEXT("%dx%d"),W,H));
            Finish(); return false;
        }
        switch (Run->Stage++)
        {
        case 0:
            PC->DisableInput(PC);
            Check(TEXT("no_missing_game_assets"),B->AssetFailure.IsEmpty(),B->AssetFailure);
            Check(TEXT("german_portraits_and_etched_material_loaded"),Artwork().BasicPortraits[3] && Artwork().BasicPortraits[4] && Artwork().BasicPortraits[5] && Artwork().EtchedMaterial);
            Snap(TEXT("01-mainmenu")); break;
        case 1:
            Command(TEXT("FactionGerman"));
            Check(TEXT("menu_switches_playable_faction"),B->GetPlayerFaction()==EFrontlineFaction::German);
            Check(TEXT("german_initial_rifle_section_has_five_members"),B->LivingSoldiers(0)==5);
            Command(TEXT("Technology"));
            Check(TEXT("technology_page_opens"),HUD->GetPresentationPage()==8);
            Command(TEXT("TechBuy6")); Check(TEXT("technology_prerequisite_enforced"),!B->HasTechnology(6) && B->GetResearchPoints()==2);
            Command(TEXT("TechBuy2")); Command(TEXT("TechBuy0"));
            Check(TEXT("ui_purchases_distinct_technology_nodes"),B->HasTechnology(0) && B->HasTechnology(2) && B->GetResearchPoints()==0);
            Command(TEXT("TechBuy2")); Check(TEXT("duplicate_purchase_does_not_spend_again"),B->GetResearchPoints()==0);
            Snap(TEXT("02-german-technology")); break;
        case 2:
            Command(TEXT("TechnologyBack")); Command(TEXT("Settings"));
            Check(TEXT("display_page_opens"),HUD->GetPresentationPage()==7);
            if (!S.Performance) Command(TEXT("Performance"));
            Check(TEXT("performance_monitor_enabled"),S.Performance);
            Snap(TEXT("03-display-settings")); break;
        case 3:
            Command(TEXT("WindowMode0"));
            Check(TEXT("resolution_selector_reaches_1080p"),ChooseResolution(FIntPoint(1920,1080)));
            Command(TEXT("VideoApply")); break;
        case 4:
        {
            int32 W=0,H=0; PC->GetViewportSize(W,H);
            Check(TEXT("resolution_change_updates_actual_viewport"),W==1920 && H==1080,FString::Printf(TEXT("%dx%d"),W,H));
            Command(TEXT("VideoKeep")); Check(TEXT("resolution_confirmation_saved"),G->GetLastConfirmedScreenResolution()==FIntPoint(1920,1080));
            Snap(TEXT("04-windowed-1080p")); break;
        }
        case 5: Command(TEXT("WindowMode1")); Command(TEXT("VideoApply")); break;
        case 6:
        {
            int32 W=0,H=0; PC->GetViewportSize(W,H); const FIntPoint Desktop=G->GetDesktopResolution();
            Check(TEXT("borderless_uses_desktop_resolution"),GEngine->GameViewport && GEngine->GameViewport->Viewport
                && GEngine->GameViewport->Viewport->GetWindowMode()==EWindowMode::WindowedFullscreen && FIntPoint(W,H)==Desktop,FString::Printf(TEXT("%dx%d desktop %dx%d"),W,H,Desktop.X,Desktop.Y));
            Command(TEXT("VideoKeep")); break;
        }
        case 7: Command(TEXT("WindowMode2")); ChooseResolution(FIntPoint(1920,1080)); Command(TEXT("VideoApply")); break;
        case 8:
        {
            int32 W=0,H=0; PC->GetViewportSize(W,H);
            Check(TEXT("fullscreen_mode_applied_with_confirmation"),GEngine->GameViewport && GEngine->GameViewport->Viewport
                && GEngine->GameViewport->Viewport->GetWindowMode()==EWindowMode::Fullscreen && W==1920 && H==1080 && S.VideoPending,FString::Printf(TEXT("%dx%d"),W,H));
            Snap(TEXT("05-fullscreen-confirmation")); Run->Next=FPlatformTime::Seconds()+15.5; break;
        }
        case 9:
            Check(TEXT("unconfirmed_mode_automatically_reverts"),!S.VideoPending && G->GetFullscreenMode()==EWindowMode::WindowedFullscreen && G->GetScreenResolution()==G->GetDesktopResolution());
            Command(TEXT("WindowMode0")); ChooseResolution(FIntPoint(1920,1080)); Command(TEXT("VideoApply")); break;
        case 10:
            Command(TEXT("VideoKeep")); Command(TEXT("DisplayBack"));
            Command(TEXT("Dossier1917")); Snap(TEXT("06-german-dossier")); break;
        case 11:
        {
            if (Run->PreparationRequestedAt<=0)
            {
                Run->PreparationRequestedAt=FPlatformTime::Seconds();Command(TEXT("Begin"));
                --Run->Stage;Run->Next=FPlatformTime::Seconds()+.3;break;
            }
            const bool Ready=B->Phase==EFrontlinePhase::Fighting;
            if (!Ready && S.Preparing && S.PreparationError.IsEmpty() && FPlatformTime::Seconds()-Run->PreparationRequestedAt<18.)
            {--Run->Stage;Run->Next=FPlatformTime::Seconds()+.3;break;}
            const FString PreparationDetail=Ready?TEXT("Actual operation preparation completed before recruit and settings checks"):
                !S.PreparationError.IsEmpty()?S.PreparationError:TEXT("Operation preparation failed or exceeded the 18 second fixture deadline");
            Check(TEXT("german_dossier_begins_real_encounter"),Ready,PreparationDetail);
            if (!Ready)
            {
                // Keep the original assertion set: dependent checks cannot be
                // marked passed by running against a still-briefing world.
                const TCHAR* Unexecuted[]={TEXT("german_machinegun_recruited"),TEXT("settings_pause_active_battle"),
                    TEXT("settings_blocks_speed_shortcut"),TEXT("settings_blocks_restart_shortcut"),
                    TEXT("leaving_settings_resumes_previously_running_battle"),TEXT("leaving_settings_preserves_preexisting_pause"),
                    TEXT("technology_cannot_be_bought_during_action"),TEXT("performance_can_be_disabled_in_battle"),
                    TEXT("german_encounter_has_real_shots"),TEXT("german_encounter_accepts_reinforcements"),TEXT("both_armies_remain_correctly_mapped")};
                for (const TCHAR* Name:Unexecuted) Check(Name,false,TEXT("Not executed: actual operation preparation failed"));
                Run->Stage=100;break;
            }
            Command(TEXT("Lewis")); Check(TEXT("german_machinegun_recruited"),B->RecruitCommands==1 && B->LivingSoldiers(0)==8);
            Command(TEXT("Settings")); Check(TEXT("settings_pause_active_battle"),UGameplayStatics::IsGamePaused(B));
            PC->ToggleSpeed(); Check(TEXT("settings_blocks_speed_shortcut"),B->BattleSpeed==1);
            PC->Restart(); Check(TEXT("settings_blocks_restart_shortcut"),B->Phase==EFrontlinePhase::Fighting && S.Settings); break;
        }
        case 12:
            Command(TEXT("DisplayBack")); Check(TEXT("leaving_settings_resumes_previously_running_battle"),!UGameplayStatics::IsGamePaused(B));
            Command(TEXT("Pause")); Command(TEXT("Settings")); break;
        case 13:
            Command(TEXT("DisplayBack")); Check(TEXT("leaving_settings_preserves_preexisting_pause"),UGameplayStatics::IsGamePaused(B));
            Command(TEXT("Pause")); Command(TEXT("Technology"));
            Command(TEXT("TechBuy4")); Check(TEXT("technology_cannot_be_bought_during_action"),!B->HasTechnology(4));
            Command(TEXT("TechnologyBack"));
            Command(TEXT("Performance")); Check(TEXT("performance_can_be_disabled_in_battle"),!S.Performance);
            Command(TEXT("Performance"));
            Run->BattleStarted=FPlatformTime::Seconds(); Run->NextOrder=B->Elapsed+1; Run->NextRecruit=B->Elapsed+1; break;
        default:
            if (B->Phase==EFrontlinePhase::Fighting)
            {
                if (B->Elapsed>=Run->NextOrder) {for (int32 I=0;I<3;++I) B->AdvanceTrench(I); Run->NextOrder=B->Elapsed+8;}
                if (B->Elapsed>=Run->NextRecruit && B->Recruit(EFrontlineRole(Run->RecruitRole%3))) {++Run->RecruitRole; Run->NextRecruit=B->Elapsed+1;}
            }
            if (Run->Stage==16) Snap(TEXT("07-german-live-battle"));
            if (FPlatformTime::Seconds()-Run->BattleStarted>42 || B->Phase!=EFrontlinePhase::Fighting)
            {
                Check(TEXT("german_encounter_has_real_shots"),B->ShotsFired>20,FString::FromInt(B->ShotsFired));
                Check(TEXT("german_encounter_accepts_reinforcements"),B->RecruitCommands>=2,FString::FromInt(B->RecruitCommands));
                Check(TEXT("both_armies_remain_correctly_mapped"),B->GetFactionForTeam(0)==EFrontlineFaction::German && B->GetFactionForTeam(1)==EFrontlineFaction::British);
                Snap(TEXT("08-german-battle-late")); Run->Stage=100; Run->Next=FPlatformTime::Seconds()+2;
            }
            break;
        }
        return true;
    }));
}

// Opt-in V15 visual acceptance fixture. Each UI command is separated by a real
// ticker interval; no screenshot is treated as a frame-rate measurement.
void StartFrontlineV15UIReview(AWarfareFrontlineController* Controller)
{
    if (!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV15UIReview"))) return;
    struct FUIRun
    {
        int32 Stage=0;double Next=FPlatformTime::Seconds()+3.;
        FString Id=FGuid::NewGuid().ToString(),Directory,Faction;
        bool Passed=true;TArray<TSharedPtr<FJsonValue>> Checks,Images;
    };
    const auto Run=MakeShared<FUIRun>();FParse::Value(FCommandLine::Get(),TEXT("WFV15UIFaction="),Run->Faction);
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V15/UIReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if (!PC) return false;
        auto* B=FrontlineFor(PC);auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        if (!B || !HUD || FPlatformTime::Seconds()<Run->Next) return true;
        Run->Next=FPlatformTime::Seconds()+.6;
        auto Command=[&](const TCHAR* Name) {HUD->NotifyHitBoxClick(FName(Name));};
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("name"),Name);O->SetBoolField(TEXT("passed"),Passed);O->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(O));Run->Passed&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V15_UI %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto Snap=[&](const TCHAR* Name,int32 ExpectedPage)
        {
            int32 W=0,H=0;PC->GetViewportSize(W,H);
            const FString File=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(File,true,false);
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("file"),File);O->SetNumberField(TEXT("width"),W);O->SetNumberField(TEXT("height"),H);
            O->SetNumberField(TEXT("page"),HUD->GetPresentationPage());O->SetStringField(TEXT("faction"),B->GetFactionName());Run->Images.Add(MakeShared<FJsonValueObject>(O));
            Check(Name,HUD->GetPresentationPage()==ExpectedPage,FString::Printf(TEXT("actual page %d; viewport %dx%d"),HUD->GetPresentationPage(),W,H));
        };
        switch (Run->Stage++)
        {
        case 0: Command(Run->Faction.Equals(TEXT("German"),ESearchCase::IgnoreCase)?TEXT("FactionGerman"):TEXT("FactionBritish"));break;
        case 1:
        {
            int32 Basic=0,Special=0,Technology=0,Support=0;
            for (const auto& T:Artwork().BasicPortraits) if (T) ++Basic;
            for (const auto& T:Artwork().SpecialistPortraits) if (T) ++Special;
            for (const auto& Nation:Artwork().TechnologyPaintings) for (const auto& T:Nation) if (T) ++Technology;
            for (const auto& T:Artwork().SupportPaintings) if (T) ++Support;
            Check(TEXT("six_independent_basic_portraits"),Basic==6,FString::FromInt(Basic));
            Check(TEXT("four_independent_specialist_portraits"),Special==4,FString::FromInt(Special));
            Check(TEXT("twenty_four_faction_technology_paintings"),Technology==24,FString::FromInt(Technology));
            Check(TEXT("three_independent_fire_support_paintings"),Support==3,FString::FromInt(Support));
            Check(TEXT("relief_material_and_large_title_atlas_loaded"),Artwork().EtchedMaterial!=nullptr && Artwork().EtchedTitleTexture!=nullptr);
            bool NodesOK=B->GetTechnologyNodes().Num()==12,GlyphsOK=true;
            TSet<FIntPoint> Cells;
            for (const auto& N:B->GetTechnologyNodes())
            {
                NodesOK&=N.Branch>=0 && N.Branch<4 && N.Tier>=0 && N.Tier<3;Cells.Add(FIntPoint(N.Branch,N.Tier));
                for (TCHAR C:FString(N.Title)+N.Effect+N.BranchTitle) if (C!='\n' && !FindEtchedGlyph(C)) GlyphsOK=false;
            }
            Check(TEXT("four_columns_three_tiers_no_duplicate_cells"),NodesOK && Cells.Num()==12);
            Check(TEXT("technology_glyphs_present"),GlyphsOK);
            Snap(TEXT("01-main-menu"),0);break;
        }
        case 2: Command(TEXT("Technology"));break;
        case 3: Snap(TEXT("02-technology"),8);break;
        case 4: Command(TEXT("TechnologyBack"));break;
        case 5: Command(TEXT("Settings"));break;
        case 6: Snap(TEXT("03-display-settings"),7);break;
        case 7: Command(TEXT("SettingsAudio"));break;
        case 8: Snap(TEXT("04-audio-settings"),7);break;
        case 9: Command(TEXT("DisplayBack"));break;
        case 10: Command(TEXT("Dossier1917"));break;
        case 11: Snap(TEXT("05-operation"),1);break;
        case 12: Command(TEXT("Begin"));break;
        case 13: Command(TEXT("Lewis"));Run->Next=FPlatformTime::Seconds()+8.;break;
        case 14: Command(TEXT("Advance"));Run->Next=FPlatformTime::Seconds()+18.;break;
        case 15: Snap(TEXT("06-live-battle"),3);break;
        case 16: Command(TEXT("Settings"));break;
        case 17: Snap(TEXT("07-battle-settings"),7);break;
        case 18: Command(TEXT("DisplayBack"));break;
        case 19: Command(TEXT("Technology"));break;
        case 20: Snap(TEXT("08-battle-technology"),8);break;
        default:
        {
            auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("run_id"),Run->Id);Report->SetBoolField(TEXT("passed"),Run->Passed);
            Report->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());Report->SetArrayField(TEXT("checks"),Run->Checks);Report->SetArrayField(TEXT("screenshots"),Run->Images);
            Report->SetStringField(TEXT("scope"),TEXT("Real viewport screenshots and spaced HUD dispatches; one faction per isolated launch. This checks page/asset/layout inputs, not artistic quality or physical mouse interaction. Manual visual review is still required. No screenshot timings are performance figures."));
            Report->SetStringField(TEXT("faction"),B->GetFactionName());Report->SetNumberField(TEXT("shots"),B->ShotsFired);
            FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("ui-review.json")));
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V15/ui-review.json")));
            UE_LOG(LogTemp,Display,TEXT("WF_V15_UI_REVIEW_COMPLETE %s Passed=%d"),*Run->Directory,Run->Passed);FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}

// Opt-in V16 visual acceptance fixture. Each UI command is separated by a real
// ticker interval; no screenshot is treated as a frame-rate measurement.
void StartFrontlineV16UIReview(AWarfareFrontlineController* Controller)
{
    if (!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV16UIReview"))) return;
    struct FUIRun
    {
        int32 Stage=0;double Next=FPlatformTime::Seconds()+3.;
        FString Id=FGuid::NewGuid().ToString(),Directory,Faction;
        bool Passed=true;TArray<TSharedPtr<FJsonValue>> Checks,Images;
    };
    const auto Run=MakeShared<FUIRun>();FParse::Value(FCommandLine::Get(),TEXT("WFV16UIFaction="),Run->Faction);
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V16/UIReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if (!PC) return false;
        auto* B=FrontlineFor(PC);auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        if (!B || !HUD || FPlatformTime::Seconds()<Run->Next) return true;
        Run->Next=FPlatformTime::Seconds()+.6;
        auto Command=[&](const TCHAR* Name) {HUD->NotifyHitBoxClick(FName(Name));};
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("name"),Name);O->SetBoolField(TEXT("passed"),Passed);O->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(O));Run->Passed&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V16_UI %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto Snap=[&](const TCHAR* Name,int32 ExpectedPage)
        {
            int32 W=0,H=0;PC->GetViewportSize(W,H);
            const FString File=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(File,true,false);
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("file"),File);O->SetNumberField(TEXT("width"),W);O->SetNumberField(TEXT("height"),H);
            O->SetNumberField(TEXT("page"),HUD->GetPresentationPage());O->SetStringField(TEXT("faction"),B->GetFactionName());Run->Images.Add(MakeShared<FJsonValueObject>(O));
            const bool AssetFailureVisible=!B->AssetFailure.IsEmpty() && !ScreenState(PC).bAssetNoticeDismissed;
            Check(Name,HUD->GetPresentationPage()==ExpectedPage && !AssetFailureVisible,FString::Printf(TEXT("actual page %d; viewport %dx%d; AssetFailure=%s"),HUD->GetPresentationPage(),W,H,*B->AssetFailure));
        };
        switch (Run->Stage++)
        {
        case 0: Command(Run->Faction.Equals(TEXT("German"),ESearchCase::IgnoreCase)?TEXT("FactionGerman"):TEXT("FactionBritish"));break;
        case 1:
        {
            int32 Basic=0,Special=0,Technology=0,Support=0;
            for (const auto& T:Artwork().BasicPortraits) if (T) ++Basic;
            for (const auto& T:Artwork().SpecialistPortraits) if (T) ++Special;
            for (const auto& Nation:Artwork().TechnologyPaintings) for (const auto& T:Nation) if (T) ++Technology;
            for (const auto& T:Artwork().SupportPaintings) if (T) ++Support;
            Check(TEXT("six_independent_basic_portraits"),Basic==6,FString::FromInt(Basic));
            Check(TEXT("four_independent_specialist_portraits"),Special==4,FString::FromInt(Special));
            Check(TEXT("twenty_four_faction_technology_paintings"),Technology==24,FString::FromInt(Technology));
            Check(TEXT("three_independent_fire_support_paintings"),Support==3,FString::FromInt(Support));
            Check(TEXT("relief_material_and_large_title_atlas_loaded"),Artwork().EtchedMaterial!=nullptr && Artwork().EtchedTitleTexture!=nullptr);
            bool NodesOK=B->GetTechnologyNodes().Num()==12;
            TSet<FIntPoint> Cells;
            for (const auto& N:B->GetTechnologyNodes())
            {
                NodesOK&=N.Branch>=0 && N.Branch<4 && N.Tier>=0 && N.Tier<3;Cells.Add(FIntPoint(N.Branch,N.Tier));
            }
            Check(TEXT("four_columns_three_tiers_no_duplicate_cells"),NodesOK && Cells.Num()==12);
            Check(TEXT("ordinary_reading_font_loaded"),Artwork().Sans!=nullptr);
            Check(TEXT("operation_paper_and_continuous_console_loaded"),Artwork().CommandPaper!=nullptr && Artwork().CommandSurface!=nullptr);
            Snap(TEXT("01-main-menu"),0);break;
        }
        case 2: Command(TEXT("Technology"));break;
        case 3: Snap(TEXT("02-technology"),8);break;
        case 4: Command(TEXT("TechnologyBack"));break;
        case 5: Command(TEXT("Settings"));break;
        case 6: Snap(TEXT("03-display-settings"),7);break;
        case 7: Command(TEXT("SettingsAudio"));break;
        case 8: Snap(TEXT("04-audio-settings"),7);break;
        case 9: Command(TEXT("DisplayBack"));break;
        case 10: Command(TEXT("Dossier1917"));break;
        case 11: Snap(TEXT("05-operation"),1);break;
        case 12: Command(TEXT("Begin"));Snap(TEXT("05b-resource-preparation"),9);break;
        case 13:
        {
            const auto& Prep=ScreenState(PC);
            if (Prep.Preparing && Prep.PreparationError.IsEmpty()) {--Run->Stage;Run->Next=FPlatformTime::Seconds()+.3;break;}
            Check(TEXT("preparation_requested_actual_role_textures"),Prep.PreparationTextures.Num()>0,FString::FromInt(Prep.PreparationTextures.Num()));
            Check(TEXT("battle_starts_after_actual_texture_residency"),B->Phase==EFrontlinePhase::Fighting && ResidentOperationTextures(Prep)==Prep.PreparationTextures.Num(),!B->AssetFailure.IsEmpty()?B->AssetFailure:Prep.PreparationError);
            if (B->Phase!=EFrontlinePhase::Fighting) {Run->Stage=21;break;}
            Command(TEXT("Lewis"));Run->Next=FPlatformTime::Seconds()+8.;break;
        }
        case 14: Command(TEXT("Advance"));Run->Next=FPlatformTime::Seconds()+18.;break;
        case 15: Snap(TEXT("06-live-battle"),3);break;
        case 16: Command(TEXT("Settings"));break;
        case 17: Snap(TEXT("07-battle-settings"),7);break;
        case 18: Command(TEXT("DisplayBack"));break;
        case 19: Command(TEXT("Technology"));break;
        case 20: Snap(TEXT("08-battle-technology"),8);break;
        default:
        {
            auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("run_id"),Run->Id);Report->SetBoolField(TEXT("passed"),Run->Passed);
            Report->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());Report->SetArrayField(TEXT("checks"),Run->Checks);Report->SetArrayField(TEXT("screenshots"),Run->Images);
            Report->SetStringField(TEXT("scope"),TEXT("Real viewport screenshots and spaced HUD dispatches; one faction per isolated launch. This checks page/asset/layout inputs, not artistic quality or physical mouse interaction. Manual visual review is still required. No screenshot timings are performance figures."));
            Report->SetStringField(TEXT("faction"),B->GetFactionName());Report->SetNumberField(TEXT("shots"),B->ShotsFired);
            FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("ui-review.json")));
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V16/ui-review.json")));
            UE_LOG(LogTemp,Display,TEXT("WF_V16_UI_REVIEW_COMPLETE %s Passed=%d"),*Run->Directory,Run->Passed);FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
