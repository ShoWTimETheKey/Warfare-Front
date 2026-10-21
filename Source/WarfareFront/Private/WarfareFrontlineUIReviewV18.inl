// One short opt-in UI pass. The launcher provides an isolated UserDir.
void StartFrontlineV18UIReview(AWarfareFrontlineController* Controller)
{
    if (!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV18UIReview"))) return;
    struct FRun
    {
        int32 Stage=0; double Start=FPlatformTime::Seconds(),Next=Start+3.;
        FString Id=FGuid::NewGuid().ToString(),Directory,SettingsPath; bool Passed=true;
        TArray<TSharedPtr<FJsonValue>> Checks,Images;
    };
    const auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V18/UIReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if (!PC) return false;
        auto* B=FrontlineFor(PC);auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        if (!B || !HUD || FPlatformTime::Seconds()<Run->Next) return true;
        Run->Next=FPlatformTime::Seconds()+.5;
        auto& S=ScreenState(PC);auto* G=GEngine?GEngine->GetGameUserSettings():nullptr;
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("name"),Name);O->SetBoolField(TEXT("passed"),Passed);O->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(O));Run->Passed&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V18_UI %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto Command=[&](const FString& Name){HUD->NotifyHitBoxClick(FName(*Name));};
        auto Snap=[&](const TCHAR* Name,int32 Expected)
        {
            const FString Path=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,true,false);
            int32 W=0,H=0;PC->GetViewportSize(W,H);auto O=MakeShared<FJsonObject>();
            O->SetStringField(TEXT("file"),Path);O->SetNumberField(TEXT("width"),W);O->SetNumberField(TEXT("height"),H);Run->Images.Add(MakeShared<FJsonValueObject>(O));
            Check(Name,HUD->GetPresentationPage()==Expected && B->AssetFailure.IsEmpty(),FString::Printf(TEXT("page=%d AssetFailure=%s"),HUD->GetPresentationPage(),*B->AssetFailure));
        };
        auto ReadSavedSettings=[&](FConfigFile& Saved,FConfigFile& Disk)
        {
            // UE 5.8 uses "GameUserSettings" as a cache key, not a disk filename.
            // Saved INIs contain a diff. Copy only the immutable engine/project
            // defaults, then re-read the physical diff; never use InMemoryFile,
            // FinalCombinedLayers, SavedLayer or runtime/command-line overrides.
            const FConfigBranch* Branch=GConfig?GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString()):nullptr;
            Run->SettingsPath=FPaths::ConvertRelativePathToFull(Branch && !Branch->IniPath.IsEmpty()?Branch->IniPath:
                FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
            if (!Branch || IFileManager::Get().FileSize(*Run->SettingsPath)<=0) return false;
            Saved=Branch->CombinedStaticLayers;Saved.Branch=nullptr;
            Disk.Read(Run->SettingsPath);
            return Saved.Combine(Run->SettingsPath);
        };
        auto CheckDisplay=[&](const TCHAR* Name,EWindowMode::Type Mode,FIntPoint Resolution)
        {
            int32 W=0,H=0;PC->GetViewportSize(W,H);
            const bool Actual=GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport
                && GEngine->GameViewport->Viewport->GetWindowMode()==Mode && FIntPoint(W,H)==Resolution;
            FConfigFile Saved,Disk;const bool ReadOK=ReadSavedSettings(Saved,Disk);int32 X=-1,Y=-1,M=-1;
            Saved.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("ResolutionSizeX"),X);
            Saved.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("ResolutionSizeY"),Y);
            Saved.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("FullscreenMode"),M);
            int32 RawX=-1,RawY=-1,RawM=-1;
            const bool HasX=Disk.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("ResolutionSizeX"),RawX);
            const bool HasY=Disk.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("ResolutionSizeY"),RawY);
            const bool HasM=Disk.GetInt(TEXT("/Script/Engine.GameUserSettings"),TEXT("FullscreenMode"),RawM);
            Check(Name,ReadOK && Actual && G && G->GetScreenResolution()==Resolution && G->GetLastConfirmedScreenResolution()==Resolution
                && G->GetFullscreenMode()==Mode && FIntPoint(X,Y)==Resolution && M==int32(Mode) && !S.VideoPending && S.VideoDeadline==0,
                FString::Printf(TEXT("viewport=%dx%d disk+defaults=%dx%d mode=%d expected=%d pending=%d disk_read=%d sources[X=%s,Y=%s,mode=%s] path=%s"),W,H,X,Y,M,int32(Mode),S.VideoPending,ReadOK,
                    HasX?TEXT("disk"):TEXT("inherited-default"),HasY?TEXT("disk"):TEXT("inherited-default"),HasM?TEXT("disk"):TEXT("inherited-default"),*Run->SettingsPath));
        };
        if (Run->Stage<99 && (!G || !B->AssetFailure.IsEmpty() || FPlatformTime::Seconds()-Run->Start>50.))
        {Check(TEXT("flow_available_without_timeout"),false,B->AssetFailure);Run->Stage=99;}
        switch(Run->Stage++)
        {
        case 0: PC->Restart();Snap(TEXT("01-main-menu-exit"),0);break;
        case 1: Command(TEXT("Settings"));break;
        case 2: Command(TEXT("WindowMode0"));S.DisplayResolution=FIntPoint(1920,1080);break;
        case 3: Command(TEXT("VideoApply"));Run->Next=FPlatformTime::Seconds()+1.5;break;
        case 4: CheckDisplay(TEXT("windowed_confirm_immediately_applies_and_saves"),EWindowMode::Windowed,FIntPoint(1920,1080));break;
        case 5: Command(TEXT("WindowMode1"));break;
        case 6: Command(TEXT("VideoApply"));Run->Next=FPlatformTime::Seconds()+1.5;break;
        case 7: CheckDisplay(TEXT("borderless_confirm_immediately_applies_and_saves"),EWindowMode::WindowedFullscreen,G->GetDesktopResolution());break;
        case 8: Command(TEXT("WindowMode2"));S.DisplayResolution=FIntPoint(1920,1080);break;
        case 9: Command(TEXT("VideoApply"));Run->Next=FPlatformTime::Seconds()+1.5;break;
        case 10: CheckDisplay(TEXT("fullscreen_confirm_immediately_applies_and_saves"),EWindowMode::Fullscreen,FIntPoint(1920,1080));break;
        case 11: Command(TEXT("WindowMode0"));S.DisplayResolution=FIntPoint(1920,1080);break;
        case 12: Command(TEXT("VideoApply"));Run->Next=FPlatformTime::Seconds()+1.5;break;
        case 13:
            CheckDisplay(TEXT("final_windowed_choice_saved_without_secondary_confirmation"),EWindowMode::Windowed,FIntPoint(1920,1080));
            Snap(TEXT("02-display-immediate-apply"),7);break;
        case 14: Command(TEXT("DisplayBack"));break;
        case 15: Command(TEXT("Settings"));break;
        case 16:
            Check(TEXT("reopening_settings_restores_current_choice"),S.DisplayResolution==FIntPoint(1920,1080) && S.DisplayMode==EWindowMode::Windowed && !S.VideoPending);
            Command(TEXT("SettingsAudio"));break;
        case 17: Command(TEXT("AudioLevel4_3"));break;
        case 18:
        {
            const auto* Audio=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
            FConfigFile Saved,Disk;const bool ReadOK=ReadSavedSettings(Saved,Disk);float Value=-1;
            const bool HasMusic=Disk.GetFloat(TEXT("WarfareFrontline.AudioV14"),TEXT("Music"),Value);
            Check(TEXT("music_volume_slider_applies_and_persists"),ReadOK && HasMusic && Audio && FMath::IsNearlyEqual(Audio->GetVolume(EFrontlineAudioBus(4)),.3f) && FMath::IsNearlyEqual(Value,.3f),FString::Printf(TEXT("saved_music=%.3f disk_read=%d explicit_music=%d path=%s"),Value,ReadOK,HasMusic,*Run->SettingsPath));
            Snap(TEXT("03-five-channel-audio"),7);break;
        }
        case 19: Command(TEXT("DisplayBack"));break;
        case 20: Command(TEXT("NewCampaign"));break;
        case 21: Command(TEXT("FactionBritish"));break;
        case 22: Command(TEXT("Begin"));break;
        case 23:
            if (S.Preparing && S.PreparationError.IsEmpty()) {--Run->Stage;break;}
            Check(TEXT("long_battlefield_has_five_trenches"),B->Phase==EFrontlinePhase::Fighting && B->Trenches.Num()==FrontlineBattlefield::TrenchCount && B->Trenches.Num()==5,S.PreparationError);
            if (B->Phase!=EFrontlinePhase::Fighting) {Run->Stage=99;break;}
            Command(TEXT("Rifle"));break;
        case 24:
            // Explicit readiness fixture presents different shadow positions for visual review.
            for (int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I) B->RecruitReadyAt[0][I]=B->Elapsed+B->EffectiveRecruitSeconds(EFrontlineRole(I))*.65f;
            B->ArtilleryReadyAt[0]=B->Elapsed+B->EffectiveArtillerySeconds()*.6f;
            B->SmokeReadyAt[0]=B->Elapsed+15;B->MortarReadyAt[0]=B->Elapsed+12;
            Command(TEXT("Pause"));break;
        case 25:
            Check(TEXT("cooldown_cards_retain_blocked_action_state"),B->RecruitCooldown(EFrontlineRole::Rifle)>0 && !B->RecruitBlockReason(EFrontlineRole::Rifle).IsEmpty() && B->ArtilleryCooldown()>0 && !B->ArtilleryBlockReason().IsEmpty());
            Snap(TEXT("04-cooldown-shadow-cards"),3);break;
        case 26:
            B->Trenches.Last().Owner=0; // Ownership fixture tests far-end selector without a staged victory.
            Command(FString::Printf(TEXT("Trench%d"),B->Trenches.Num()-1));break;
        case 27:
            Check(TEXT("fifth_trench_selection_has_three_local_slots"),PC->SelectedTrench==4 && B->GetTrenchSquadSlots(4).Num()==FrontlineBattlefield::TrenchCapacity);
            Check(TEXT("camera_reaches_far_end_without_reducing_soldier_scale"),PC->BattleCamera && PC->BattleCamera->GetActorLocation().X>7000 && FMath::IsNearlyEqual(PC->BattleCamera->GetCameraComponent()->OrthoWidth,6000.f,1.f),FString::Printf(TEXT("view_x=%.1f width=%.1f"),PC->BattleCamera?PC->BattleCamera->GetActorLocation().X:0,PC->BattleCamera?PC->BattleCamera->GetCameraComponent()->OrthoWidth:0));
            Snap(TEXT("05-fifth-trench-local-orders"),3);break;
        case 28:
            PC->Restart();S.Dossier=0;break;
        default:
        {
            const bool QuitAvailable=B->Phase==EFrontlinePhase::Briefing && HUD->GetPresentationPage()==0 && !S.Settings && !S.Help && !S.Preparing;
            Check(TEXT("main_menu_quit_route_available"),QuitAvailable);
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("run_id"),Run->Id);O->SetBoolField(TEXT("passed"),Run->Passed);
            O->SetArrayField(TEXT("checks"),Run->Checks);O->SetArrayField(TEXT("screenshots"),Run->Images);O->SetStringField(TEXT("settings_ini"),Run->SettingsPath);O->SetStringField(TEXT("settings_config_key"),GGameUserSettingsIni);
            O->SetStringField(TEXT("scope"),TEXT("Five actual PNGs; spaced HUD dispatches apply all three display modes and check viewport, confirmed settings and disk INI. Resolution values are a fixture selection; input is not physical mouse. A paused readiness fixture shows cooldown masks and a friendly-ownership fixture exposes trench 5. No combat, performance or visual-quality pass is inferred. Parent launcher must confirm normal process exit after the QuitGame dispatch; settings process-restart persistence is not claimed by this single process."));
            O->SetStringField(TEXT("exit_route"),QuitAvailable?TEXT("HUD QuitGame; parent must confirm process exit"):TEXT("fixture failure fallback exit"));
            FString Json;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("ui-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V18/ui-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V18_UI_REVIEW_COMPLETE Passed=%d %s"),Run->Passed,*Run->Directory);
            if (QuitAvailable) Command(TEXT("QuitGame"));else FPlatformMisc::RequestExit(false);
            return false;
        }
        }
        return true;
    }));
}
