#include "WarfareFrontlineMachinegunV36Review.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineSoldierSurfaceV62.h"
namespace
{
void StartFrontlineV36UIReview(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV36Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0;bool Passed=true,ArtFocus=false;double Next=FPlatformTime::Seconds()+3,Start=Next,Prepared=0;
        bool CrewInspected=false;double CrewWaitStarted=0,CrewReadySince=0,AircraftWaitStarted=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V36/Review"));
    R->ArtFocus=FParse::Param(FCommandLine::Get(),TEXT("WFV36ArtFocus"));if(R->ArtFocus)R->Stage=35;
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=true;V36TypographyAuditActive=true;V36TypographyAudit.Empty();V28ReviewPointer=FVector2D(1760,170);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;if(Now<R->Next)return true;
        auto& S=ScreenState(PC);
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;PC->CancelTargeting();S.ArchivePendingDelete.Empty();};
        auto CrewShot=[&](const TCHAR* Name)
        {
            // Inspect spawns temporary combat targets. Run it once, before the
            // resource barrier, so polling cannot recreate the fixture forever.
            if(!R->CrewInspected){InspectV36TrenchCrew(B,Check);R->CrewInspected=true;R->CrewWaitStarted=Now;}
            const bool Ready=FrontlineSoldierSurfaceV62::PrepareReview(B->GetWorld());
            if(!Ready)R->CrewReadySince=0;else if(R->CrewReadySince==0)R->CrewReadySince=Now;
            const bool Stable=Ready&&Now-R->CrewReadySince>=1.;
            if(!Stable&&Now-R->CrewWaitStarted<25.){R->Next=Now+.25;return false;}
            Check(FString(Name)+TEXT("_assigned_resources_ready"),Stable);
            if(FParse::Param(FCommandLine::Get(),TEXT("WFSoldierSurfaceAuditV62")))
                Check(FString(Name)+TEXT("_actual_render_materials_and_mips"),FrontlineSoldierSurfaceV62::WriteAudit(B->GetWorld(),Name));
            Shot(Name);return true;
        };
        auto Language=[&](bool Chinese){FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);};
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();B->AbandonBattleToOperations();B->SetActorTickEnabled(true);B->StartNewCampaign(F,EFrontlineDifficulty::Survivor);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=Mission;Run->Research.ResearchPoints=99999;}
            Check(TEXT("operation_selected_")+FString::FromInt(Mission)+FString::FromInt(int32(F)),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
        };
        auto Begin=[&]()
        {
            PC->SetActorTickEnabled(true);B->SetActorTickEnabled(true);
            for(int32 I=0;I<32&&!S.Preparing;++I){PC->BeginBattle();if(S.HistoricalDispatch)HUD->NotifyHitBoxClick(TEXT("V32Acknowledge"));if(S.Deployment)HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));if(S.UnitIntroduction)HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));}
            R->Prepared=Now;
        };
        if(Now-R->Start>150){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        double Delay=.8;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("archive_review_requires_isolated_volatile_profile"),B->IsProgressionVolatile());
            if(!R->Passed){R->Stage=99;break;}
            FrontlineUpdatesV36::RunChecks(Check);FrontlineUpdatesV36::Check();RunFrontlineMoraleV36Checks(B,Check);
            FrontlineSupportArtV36::RunChecks(B,[&](const TCHAR* N,bool Pass){Check(FString(N),Pass);});
            const int32 Original=B->GetCampaignRuns().Num();
            Check(TEXT("archive_empty_batch_rejected"),!B->DeleteCampaignArchives({}));
            B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);const auto A=B->GetCurrentCampaign()->Id;
            B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Survivor);const auto Z=B->GetCurrentCampaign()->Id;
            Check(TEXT("archive_unknown_guid_rejects_entire_batch"),!B->DeleteCampaignArchives({A,FGuid::NewGuid()})&&B->GetCampaignRuns().Num()==Original+2);
            B->Phase=EFrontlinePhase::Fighting;Check(TEXT("archive_live_battle_protected"),!B->DeleteCampaignArchives({Z}));B->Phase=EFrontlinePhase::Briefing;
            Check(TEXT("archive_selected_delete_preserves_active_route"),B->DeleteCampaignArchives({A,A})&&B->GetCurrentCampaign()&&B->GetCurrentCampaign()->Id==Z&&B->GetCampaignRuns().Num()==Original+1);
            Check(TEXT("archive_active_delete_rebinds_safely"),B->DeleteCampaignArchives({Z})&&B->GetCampaignRuns().Num()==Original);
            for(int32 I=0;I<6;++I)B->StartNewCampaign(EFrontlineFaction(I%2),EFrontlineDifficulty(I%3));
            const int32 Before=B->GetCampaignRuns().Num();S.Dossier=1919;S.ArchiveSelection.Add(B->GetCampaignRuns().Last().Id);
            HUD->NotifyHitBoxClick(TEXT("ArchiveDeleteSelected"));Check(TEXT("archive_delete_requires_confirmation"),S.ArchivePendingDelete.Num()==1&&B->GetCampaignRuns().Num()==Before);
            HUD->NotifyHitBoxClick(TEXT("ArchiveDeleteCancel"));Check(TEXT("archive_cancel_preserves_records"),S.ArchivePendingDelete.IsEmpty()&&B->GetCampaignRuns().Num()==Before);
            S.ArchiveSelection.Empty();Language(false);S.Dossier=1919;
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Delay=1.15;break;
        }
        case 1:Shot(TEXT("01-archives-en-difficulty"));break;
        case 2:HUD->NotifyHitBoxClick(TEXT("ArchiveSelectAll"));HUD->NotifyHitBoxClick(TEXT("ArchiveDeleteSelected"));break;
        case 3:Shot(TEXT("02-delete-confirmation-en"));break;
        case 4:HUD->NotifyHitBoxClick(TEXT("ArchiveDeleteCancel"));Language(true);break;
        case 5:Shot(TEXT("03-archives-zh"));break;
        case 6:Language(false);PC->ToggleSettings();S.SettingsTab=0;break;
        case 7:Shot(TEXT("04-settings-video-update"));break;
        case 8:S.SettingsTab=2;V28ReviewPointer=FVector2D(1065,656);break;
        case 9:Shot(TEXT("05-settings-graphics"));break;
        case 10:S.SettingsTab=3;V28ReviewPointer=FVector2D(750,528);break;
        case 11:Shot(TEXT("06-settings-hdr-guidance"));break;
        case 12:Language(true);S.SettingsTab=1;V28ReviewPointer=FVector2D(1780,160);break;
        case 13:Shot(TEXT("07-settings-audio-zh"));break;
        case 14:Language(false);Select(EFrontlineFaction::German,9);break;
        case 15:Shot(TEXT("08-operations-en"));break;
        case 16:S.Technology=true;S.TechnologyPage=0;V28ReviewPointer=FVector2D(486,365);break;
        case 17:Shot(TEXT("09-technology-square-en"));break;
        case 18:Language(true);S.TechnologyPage=1;V28ReviewPointer=FVector2D(1058,605);break;
        case 19:Shot(TEXT("10-technology-late-zh"));break;
        case 20:Language(false);Select(EFrontlineFaction::British,7);S.UnitIntroduction=true;S.UnitIntroductionPage=0;break;
        case 21:Shot(TEXT("11-introduction-en"));break;
        case 22:Clear();S.Deployment=true;V28ReviewPointer=FVector2D(1400,670);break;
        case 23:Shot(TEXT("12-deployment-support-tooltip"));break;
        case 24:Select(EFrontlineFaction::German,9);Begin();Delay=.15;break;
        case 25:
            if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&Now-R->Prepared<35){R->Next=Now+.15;return true;}
            Check(TEXT("actual_operation_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
            B->SetActorTickEnabled(false);B->Elapsed=135;FFrontlineReviewV32::Camera(PC,0,0,6200);PC->SupportTargetMode=SupportTargetMode(EFrontlineSupport::Mortar);V28ReviewPointer=FVector2D(1320,884);break;
        case 26:Shot(TEXT("13-hud-en-support-names-research"));break;
        case 27:Language(true);PC->CancelTargeting();V28ReviewPointer=FVector2D(500,25);break;
        case 28:Shot(TEXT("14-hud-zh-morale-guidance"));break;
        case 29:Language(false);S.Menu=true;break;
        case 30:Shot(TEXT("15-battle-menu"));break;
        case 31:Clear();S.Help=true;break;
        case 32:Shot(TEXT("16-manual-objectives"));break;
        case 33:Clear();B->Phase=EFrontlinePhase::Victory;B->EndReason=TEXT("中央阵地与敌军纵深防线已被我军夺取。");break;
        case 34:Shot(TEXT("17-field-report"));break;
        case 35:Clear();R->CrewInspected=false;R->CrewReadySince=0;FrontlineSupportArtV36::PrewarmVisuals();Check(TEXT("existing_british_trench_machinegun_fixture"),SetupV36TrenchCrew(B,Check,0));FFrontlineReviewV32::Camera(PC,0,0,3400,80);V28ReviewPointer=FVector2D(1780,170);Delay=2.6;break;
        case 36:if(!CrewShot(TEXT("18-machinegun-existing-earthworks-normal")))return true;break;
        case 37:R->CrewInspected=false;R->CrewReadySince=0;Check(TEXT("existing_german_trench_machinegun_fixture"),SetupV36TrenchCrew(B,Check,1));FFrontlineReviewV32::Camera(PC,0,0,1800,80);Delay=2.6;break;
        case 38:if(!CrewShot(TEXT("19-machinegun-existing-earthworks-close")))return true;break;
        case 39:
            if(R->AircraftWaitStarted==0)R->AircraftWaitStarted=Now;
            if(!FrontlineSupportArtV36::PrepareAircraftReview()&&Now-R->AircraftWaitStarted<8.){R->Next=Now+.15;return true;}
            Check(TEXT("aircraft_actual_shared_atlases_resident"),FrontlineSupportArtV36::PrepareAircraftReview(true));
            FrontlineSupportArtV36::SetupVisualFixture(B);FFrontlineReviewV32::Camera(PC,0,0,5500,450);Delay=.23;break;
        case 40:Shot(TEXT("20-aircraft-ordnance-strike"));break;
        default:
        {
            TArray<TSharedPtr<FJsonValue>> Typography;int32 Bad=0,Truncated=0;
            for(const auto& Entry:V36TypographyAudit){Typography.Add(MakeShared<FJsonValueObject>(Entry.Value));if(!Entry.Value->GetBoolField(TEXT("fits")))++Bad;if(Entry.Value->GetBoolField(TEXT("truncated")))++Truncated;}
            Check(TEXT("measured_UI_boxes_remain_inside_assigned_bounds"),Bad==0&&Typography.Num()>(R->ArtFocus?10:75));
            Check(TEXT("reviewed_paragraphs_do_not_need_ellipsis"),Truncated==0);
            Check(TEXT("actual_English_Chinese_visible_message_coverage"),FrontlineLocalizationV35::WriteCoverageReport());
            Check(TEXT("focused_affected_screens_requested"),R->Shots==(R->ArtFocus?3:20));
            const auto& Update=FrontlineUpdatesV36::Get();R->Report->SetNumberField(TEXT("update_status"),int32(Update.Status));R->Report->SetStringField(TEXT("latest_public_tag"),Update.LatestTag);
            if(!R->ArtFocus)Check(TEXT("manual_update_request_completed"),Update.Status!=FrontlineUpdatesV36::EStatus::Idle&&Update.Status!=FrontlineUpdatesV36::EStatus::Checking);
            R->Report->SetArrayField(TEXT("typography"),Typography);R->Report->SetNumberField(TEXT("overflowing_text_boxes"),Bad);R->Report->SetNumberField(TEXT("ellipsized_paragraphs"),Truncated);
            R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),R->ArtFocus?TEXT("Art-only follow-up: three actual captures of faction trench machineguns and cosmetic support art, with visible HUD typography. Archive, morale and update checks are intentionally not repeated. Isolated volatile profile; no user files changed."):TEXT("Actual EN/Chinese changed UI pages and bounded typography; synthetic archive deletion only in volatile profile; morale/update parser checks; two faction trench gun poses; cosmetic support-art fixture. Images require independent visual inspection. No user saves are deleted, no update download or browser launch is performed."));
            FString Out;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(R->Directory/TEXT("review.json")));
            V28ReviewPointer.Reset();V27ImageAuditActive=V36TypographyAuditActive=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
    }));
}
}
