namespace
{
void StartFrontlineV37Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0;bool Passed=true;double Next=FPlatformTime::Seconds()+3,Start=Next,Prepared=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
        TArray<TWeakObjectPtr<AFrontlineTank>> Tanks;
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V37/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=V36TypographyAuditActive=true;V36TypographyAudit.Empty();V28ReviewPointer=FVector2D(1760,170);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;if(Now<R->Next)return true;
        auto& S=ScreenState(PC);
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();};
        auto Language=[&](bool Chinese){FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);};
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();B->AbandonBattleToOperations();B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=9;Run->Research.ResearchPoints=25;Run->SupportSelectionMask=(1u<<int32(EFrontlineSupport::Artillery))|(1u<<int32(EFrontlineSupport::Smoke))|(1u<<int32(EFrontlineSupport::Mortar))|(1u<<int32(EFrontlineSupport::AntiTank))|(1u<<int32(EFrontlineSupport::AirRecon))|(1u<<int32(EFrontlineSupport::AirBombing));}
            Check(TEXT("operation_selected_")+FString::FromInt(Mission)+FString::FromInt(int32(F)),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
        };
        auto Begin=[&]()
        {
            for(int32 I=0;I<32&&!S.Preparing;++I){PC->BeginBattle();if(S.HistoricalDispatch)HUD->NotifyHitBoxClick(TEXT("V32Acknowledge"));if(S.Deployment)HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));if(S.UnitIntroduction)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));}
            Check(TEXT("preparation_owns_modal"),S.Preparing&&!S.UnitIntroduction&&!S.Deployment&&!S.HistoricalDispatch);
            const double Started=S.PreparationStarted;
            for(int32 I=0;I<6;++I){PC->BeginBattle();HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));HUD->NotifyHitBoxClick(TEXT("V32Deployment"));}
            Check(TEXT("repeated_input_cannot_reopen_intro_or_reset_preparation"),S.Preparing&&!S.UnitIntroduction&&!S.Deployment&&S.PreparationStarted==Started&&HUD->GetPresentationPage()==9);
            R->Prepared=Now;
        };
        auto Ready=[&]()
        {
            if(S.Preparing&&Now-R->Prepared<35){R->Next=Now+.15;return false;}
            Check(TEXT("actual_scene_loaded_")+FString::FromInt(R->Stage),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());
            B->SetActorTickEnabled(false);B->Elapsed=135;FFrontlineReviewV32::Camera(PC,0,0,5600,250);return true;
        };
        if(Now-R->Start>135){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        double Delay=.8;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());if(!R->Passed){R->Stage=99;break;}
            FrontlineDebugV37::RunChecks(B,Check);
            FrontlineSupportV32::RunV37Checks(B,[&](const TCHAR* N,bool Good){Check(FString(N),Good);});
            if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld())){auto J=V->RunVehicleChecksV37();R->Report->SetObjectField(TEXT("vehicles"),J);Check(TEXT("vehicle_changed_behavior_checks"),J->GetBoolField(TEXT("success")));}
            Check(TEXT("streaming_quality_cannot_fail_valid_scene"),FrontlinePresentationV37::PreparationReady(8.1,61,false));
            Check(TEXT("minimum_render_warmup_retained"),!FrontlinePresentationV37::PreparationReady(40,12,true)&&!FrontlinePresentationV37::PreparationReady(3.4,90,true)&&FrontlinePresentationV37::PreparationReady(3.6,61,true));
            Check(TEXT("no_photo_replay_for_intro_loadout_or_loading"),!FrontlinePresentationV37::UsePhotoTransition(1,15,9)&&!FrontlinePresentationV37::UsePhotoTransition(15,9,9)&&!FrontlinePresentationV37::UsePhotoTransition(1,17,9)&&!FrontlinePresentationV37::UsePhotoTransition(9,3,9));
            Check(TEXT("photo_cadence_bounded"),FrontlinePresentationV37::UsePhotoTransition(3,6,3)&&!FrontlinePresentationV37::UsePhotoTransition(3,6,.4));
            TMap<FString,FString> Known;TArray<TSharedPtr<FJsonValue>> Bindings;bool Covered=true,Consistent=true;
            for(int32 Mission=0;Mission<10;++Mission)for(int32 F=0;F<2;++F)
            {
                const auto P=FrontlinePresentationV37::Commander(F==1,Mission);const FString Asset=FString::Printf(TEXT("%d:%d"),P.Atlas,P.Slot);
                const UTexture2D* T=P.Atlas==2?Artwork().CommandersV37.Get():Artwork().CommandersV30[P.Atlas].Get();
                Covered&=T&&T->GetResource()&&P.Slot>=0&&P.Slot<P.Columns*P.Rows;
                if(const FString* Prior=Known.Find(P.Identity))Consistent&=*Prior==Asset;else Known.Add(P.Identity,Asset);
                auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("mission"),Mission);J->SetNumberField(TEXT("faction"),F);J->SetStringField(TEXT("identity"),P.Identity);J->SetStringField(TEXT("atlas_slot"),Asset);Bindings.Add(MakeShared<FJsonValueObject>(J));
            }
            R->Report->SetArrayField(TEXT("commander_bindings"),Bindings);Check(TEXT("all_twenty_commander_bindings_have_resident_portraits"),Covered);Check(TEXT("same_commander_reuses_same_portrait"),Consistent);
            Language(false);Select(EFrontlineFaction::British,6);V28ReviewPointer=FVector2D(208,725);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Delay=1;break;
        }
        case 1:Shot(TEXT("01-horne-operations-en"));break;
        case 2:Language(true);V28ReviewPointer=FVector2D(748,725);break;
        case 3:Shot(TEXT("02-below-commander-zh"));break;
        case 4:Language(false);Select(EFrontlineFaction::British,7);V28ReviewPointer=FVector2D(748,725);break;
        case 5:Shot(TEXT("03-carlowitz-commander-en"));break;
        case 6:Language(true);Select(EFrontlineFaction::German,9);V28ReviewPointer=FVector2D(748,725);break;
        case 7:Shot(TEXT("04-mudra-commander-zh"));break;
        case 8:Language(false);V28ReviewPointer=FVector2D(208,725);break;
        case 9:Shot(TEXT("05-returning-byng-en"));break;
        case 10:Select(EFrontlineFaction::British,9);Begin();Delay=.25;break;
        case 11:Shot(TEXT("06-exclusive-battle-preparation"));break;
        case 12:if(!Ready())return true;V28ReviewPointer=FVector2D(88,DockTop+95);break;
        case 13:Shot(TEXT("07-infantry-ready-description-en"));break;
        case 14:Check(TEXT("recruit_sets_real_cooldown"),B->Recruit(FieldRolesV32(B)[0]));break;
        case 15:Shot(TEXT("08-infantry-cooling-description-en"));break;
        case 16:Language(true);break;
        case 17:Shot(TEXT("09-infantry-cooling-description-zh"));break;
        case 18:
        {
            Language(false);Check(TEXT("real_recon_report_fixture"),FrontlineSupportV32::SetupReconReviewV37(B));
            const auto L=FieldSupportsV32(B);const int32 I=L.Find(EFrontlineSupport::AirRecon);
            Check(TEXT("recon_is_in_review_loadout"),I!=INDEX_NONE);
            V28ReviewPointer=FVector2D(1016+(I%3)*124+50,DockTop+36+(I/3)*94+30);break;
        }
        case 19:Shot(TEXT("10-recon-report-en"));break;
        case 20:Language(true);break;
        case 21:Shot(TEXT("11-recon-report-zh"));break;
        case 22:
            Language(false);V28ReviewPointer=FVector2D(1760,170);
            if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(auto* T:V->SpawnVehicleArtReviewV37())R->Tanks.Add(T);
            Check(TEXT("all_vehicle_art_variants_spawned"),R->Tanks.Num()==int32(EFrontlineVehicle::Count));B->SetPaused(true);FFrontlineReviewV32::Camera(PC,0,0,3800,100);Delay=2;break;
        case 23:Shot(TEXT("12-vehicle-paint-and-lanes"));break;
        case 24:FFrontlineReviewV32::Camera(PC,0,-550,2600,100);break;
        case 25:Shot(TEXT("13-vehicle-material-close"));break;
        case 26:FrontlineDebugV37::SetReviewOpen(PC,true);Check(TEXT("console_help_uses_input_route"),FrontlineDebugV37::SubmitReviewCommand(PC,TEXT("help")));break;
        case 27:Shot(TEXT("14-private-console-layout"));break;
        case 28:FrontlineDebugV37::SetReviewOpen(PC,false);Select(EFrontlineFaction::German,0);Begin();Delay=.15;break;
        case 29:if(!Ready())return true;V28ReviewPointer=FVector2D(88,DockTop+95);break;
        case 30:Shot(TEXT("15-german-repeat-load-complete"));break;
        default:
        {
            TArray<TSharedPtr<FJsonValue>> Typography;int32 Bad=0,Truncated=0;
            for(const auto& E:V36TypographyAudit){Typography.Add(MakeShared<FJsonValueObject>(E.Value));if(!E.Value->GetBoolField(TEXT("fits")))++Bad;if(E.Value->GetBoolField(TEXT("truncated")))++Truncated;}
            Check(TEXT("visible_typography_inside_bounds"),Bad==0&&Typography.Num()>45);Check(TEXT("no_paragraph_ellipsis"),Truncated==0);Check(TEXT("visible_English_Chinese_coverage"),FrontlineLocalizationV35::WriteCoverageReport());Check(TEXT("affected_screens_requested"),R->Shots==15);
            R->Report->SetArrayField(TEXT("typography"),Typography);R->Report->SetNumberField(TEXT("overflowing_text_boxes"),Bad);R->Report->SetNumberField(TEXT("ellipsized_paragraphs"),Truncated);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),TEXT("Fifteen changed views: late commander portraits, exclusive repeated-input loading in both factions, ready/cooling infantry descriptions, observed recon report, vehicle materials and private command panel. Production policy and changed support/vehicle checks in isolated volatile profile. Images still require independent visual inspection."));
            FString Out;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(R->Directory/TEXT("review.json")));
            V28ReviewPointer.Reset();V27ImageAuditActive=V36TypographyAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
    }));
}
}
