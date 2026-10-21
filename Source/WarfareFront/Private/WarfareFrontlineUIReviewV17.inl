// One opt-in, isolated V17 UI pass: seven real screenshots, no performance sampling.
void StartFrontlineV17UIReview(AWarfareFrontlineController* Controller)
{
    if (!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV17UIReview"))) return;
    struct FRun
    {
        int32 Stage=0,Held=INDEX_NONE,Moving=INDEX_NONE;
        double Start=FPlatformTime::Seconds(),Next=Start+3.;
        FString Id=FGuid::NewGuid().ToString(),Directory;bool Passed=true;
        TArray<TSharedPtr<FJsonValue>> Checks,Images;
    };
    const auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V17/UIReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if (!PC) return false;
        auto* B=FrontlineFor(PC);auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        if (!B || !HUD || FPlatformTime::Seconds()<Run->Next) return true;
        Run->Next=FPlatformTime::Seconds()+.55;
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("name"),Name);O->SetBoolField(TEXT("passed"),Passed);O->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(O));Run->Passed&=Passed;
        };
        auto Command=[&](const FString& Name){HUD->NotifyHitBoxClick(FName(*Name));};
        auto Snap=[&](const TCHAR* Name,int32 Expected)
        {
            const FString Path=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,true,false);
            int32 W=0,H=0;PC->GetViewportSize(W,H);auto O=MakeShared<FJsonObject>();
            O->SetStringField(TEXT("file"),Path);O->SetNumberField(TEXT("width"),W);O->SetNumberField(TEXT("height"),H);Run->Images.Add(MakeShared<FJsonValueObject>(O));
            Check(Name,HUD->GetPresentationPage()==Expected && B->AssetFailure.IsEmpty(),FString::Printf(TEXT("page=%d AssetFailure=%s"),HUD->GetPresentationPage(),*B->AssetFailure));
        };
        if (Run->Stage<99 && (!B->AssetFailure.IsEmpty() || FPlatformTime::Seconds()-Run->Start>50.))
        {Check(TEXT("flow_available_without_timeout"),false,B->AssetFailure);Run->Stage=99;}
        switch(Run->Stage++)
        {
        case 0:
            PC->Restart();
            Check(TEXT("restart_without_campaign_preserves_working_main_menu"),HUD->GetPresentationPage()==0 && !B->GetCurrentCampaign());
            Snap(TEXT("01-main-menu"),0);break;
        case 1: Command(TEXT("NewCampaign"));break;
        case 2: Snap(TEXT("02-faction-selection"),11);break;
        case 3: Command(TEXT("FactionBritish"));break;
        case 4:
            Check(TEXT("new_campaign_has_three_linked_operations"),B->GetCurrentCampaign()!=nullptr && B->GetCampaignMissions().Num()==3);
            Snap(TEXT("03-campaign-room"),1);break;
        case 5: Command(TEXT("Technology"));break;
        case 6:
        {
            int32 W=0,H=0;PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1920.f,H/1080.f);
            PC->SetMouseLocation(FMath::RoundToInt((W-1920*Scale)*.5f+470*Scale),FMath::RoundToInt((H-1080*Scale)*.5f+360*Scale));break;
        }
        case 7: Snap(TEXT("04-upgrades-hover"),8);break;
        case 8: Command(TEXT("TechnologyBack"));break;
        case 9: Command(TEXT("Begin"));Snap(TEXT("05-real-preparation"),9);break;
        case 10:
        {
            const auto& S=ScreenState(PC);
            if (S.Preparing && S.PreparationError.IsEmpty()) {--Run->Stage;break;}
            Check(TEXT("ready_battle_starts_without_player_squads"),B->Phase==EFrontlinePhase::Fighting && B->ActiveSquads(0)==0,S.PreparationError);
            Check(TEXT("empty_trench_has_no_section_command_target"),B->GetTrenchSquadSlots(0).Num()==3 && B->GetTrenchSquadSlots(0).FilterByPredicate([](int32 Id){return Id!=INDEX_NONE;}).IsEmpty() && !B->SquadAdvanceBlockReason(0,INDEX_NONE).IsEmpty());
            if (B->Phase!=EFrontlinePhase::Fighting) {Run->Stage=99;break;}
            Command(TEXT("Rifle"));break;
        }
        case 11:
            for (int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I) B->RecruitReadyAt[0][I]=B->Elapsed;
            Command(TEXT("Lewis"));break;
        case 12:
            for (int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I) B->RecruitReadyAt[0][I]=B->Elapsed;
            Command(TEXT("Mills"));Run->Next=FPlatformTime::Seconds()+11.;break;
        case 13:
        {
            Command(TEXT("Trench0"));const auto Slots=B->GetTrenchSquadSlots(0);
            for (int32 Id:Slots) if(Id!=INDEX_NONE) {if(Run->Held==INDEX_NONE) Run->Held=Id;else if(Run->Moving==INDEX_NONE) Run->Moving=Id;}
            Check(TEXT("local_trench_has_three_resident_sections"),Slots.Num()==3 && !Slots.Contains(INDEX_NONE));break;
        }
        case 14: Command(FString::Printf(TEXT("SectionHold0_%d"),Run->Held));break;
        case 15:
            Check(TEXT("local_hold_command_applied"),B->FindSquad(Run->Held) && B->FindSquad(Run->Held)->bHoldPosition);
            Snap(TEXT("06-local-trench-orders"),3);break;
        case 16: Command(FString::Printf(TEXT("SectionAdvance0_%d"),Run->Moving));break;
        case 17:
            Check(TEXT("single_section_advances_while_held_section_stays"),B->FindSquad(Run->Moving) && B->FindSquad(Run->Moving)->bAdvancing && B->FindSquad(Run->Held) && B->FindSquad(Run->Held)->Trench==0 && B->FindSquad(Run->Held)->bHoldPosition);
            B->Morale[1]=0;break; // Explicit UI-result fixture, not a claimed natural player victory.
        case 18:
            Snap(TEXT("07-field-report"),6);
            Check(TEXT("result_bound_to_finished_mission"),B->GetEncounterCampaignMission() && B->GetEncounterCampaignMission()->Id==0 && B->GetCurrentCampaign() && B->GetCurrentCampaign()->SelectedMission==1);break;
        case 19: Command(TEXT("CampaignAfterBattle"));break;
        case 20:
            Check(TEXT("continue_returns_to_next_operation"),HUD->GetPresentationPage()==1 && B->GetSelectedCampaignMission() && B->GetSelectedCampaignMission()->Id==1);break;
        default:
        {
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("run_id"),Run->Id);O->SetBoolField(TEXT("passed"),Run->Passed);
            O->SetArrayField(TEXT("checks"),Run->Checks);O->SetArrayField(TEXT("screenshots"),Run->Images);
            O->SetStringField(TEXT("scope"),TEXT("Seven real UI screenshots and spaced HUD dispatches. Fixture accelerates shared recruit readiness for a three-section trench view and sets enemy morale to zero for the result page. Not natural-victory, physical-mouse, animation-quality, or performance acceptance. Isolated volatile campaign profile."));
            FString Json;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("ui-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V17/ui-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V17_UI_REVIEW_COMPLETE Passed=%d %s"),Run->Passed,*Run->Directory);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
