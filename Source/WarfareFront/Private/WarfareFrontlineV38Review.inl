#include "WarfareFrontlineCombatV38.h"
#include "WarfareFrontlineMotionV38.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineImpact.h"
namespace
{
void StartFrontlineV38Review(AWarfareFrontlineController* Controller)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV38Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    FString UserDir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Shots=0;bool Passed=true,Idle=false;double Next=FPlatformTime::Seconds()+3,Start=Next;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V38/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    V27ImageAuditActive=V36TypographyAuditActive=true;V36TypographyAudit.Empty();V28ReviewPointer=FVector2D(1790,110);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float DT)
    {
        const double Now=FPlatformTime::Seconds();auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;if(!B||!HUD)return false;
        if(R->Idle)FrontlineMotionV38::TickIdleReview(B,FMath::Min(DT,.1f));
        if(Now<R->Next)return true;
        auto& S=ScreenState(PC);
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const TCHAR* Name){++R->Shots;FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Name)+TEXT(".png")),true,false);};
        auto Language=[&](bool Chinese){FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);};
        auto Clear=[&](){R->Idle=false;V38LockCoverageReview=false;UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=false;PC->CancelTargeting();PC->SetActorTickEnabled(true);};
        auto Select=[&](EFrontlineFaction F,int32 Mission)
        {
            Clear();B->AbandonBattleToOperations();B->StartNewCampaign(F,EFrontlineDifficulty::Veteran);
            if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))Run->CompletedMissions=Mission;
            Check(TEXT("selected_review_operation_")+FString::FromInt(int32(F))+FString::FromInt(Mission),B->SelectCampaignMission(Mission));
            S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();B->SetActorTickEnabled(false);
        };
        auto Outcome=[&](int32 Team)
        {
            B->BeginBattle();B->Elapsed=130;B->ShotsFired=20;
            Check(TEXT("no_unlock_announcement_during_live_operation_")+FString::FromInt(R->Stage),AwardedAttachmentsV38(B).IsEmpty());
            FFrontlineReviewV32::Finish(B,Team);
            // Advance only the production aftermath clock in this isolated fixture;
            // the gameplay's normal seven-second ending is unchanged.
            B->Tick(7.1f);B->SetActorTickEnabled(false);Check(TEXT("actual_outcome_report_visible_")+FString::FromInt(R->Stage),HasEnded(B));
        };
        if(Now-R->Start>120){Check(TEXT("bounded_review_timeout"),false);R->Stage=99;}
        double Delay=.8;
        switch(R->Stage)
        {
        case 0:
        {
            Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());if(!R->Passed){R->Stage=99;break;}
            FrontlineCombatV38::RunChecks(B,Check);
            auto Motion=FrontlineMotionV38::RunFocusedChecks(B);R->Report->SetObjectField(TEXT("motion"),Motion);Check(TEXT("motion_focused_checks"),Motion->GetBoolField(TEXT("success")));
            FrontlineImpactV38::RunChecks(B,[&](const TCHAR* N,bool Good){Check(FString(N),Good);});
            Check(TEXT("lock_uses_silhouette_material_not_luminance_key"),Artwork().LockCutoutV35&&Artwork().LockCutoutV35->GetName()==TEXT("M_LockCoverage_V38"));
            Language(false);Select(EFrontlineFaction::British,9);Outcome(0);Check(TEXT("final_campaign_win_has_no_nonexistent_unlock"),!NextAwardedOperationV38(B)&&AwardedAttachmentsV38(B).IsEmpty());
            Select(EFrontlineFaction::British,2);
            Check(TEXT("next_units_and_support_remain_locked_before_victory"),!B->IsRoleUnlocked(EFrontlineRole::Marksman)&&!B->IsSupportUnlocked(EFrontlineSupport::Gas)&&!B->IsSupportUnlocked(EFrontlineSupport::AntiTank));
            Outcome(0);const auto A=AwardedAttachmentsV38(B);
            Check(TEXT("actual_first_win_already_selects_next_operation"),B->GetCurrentCampaign()->CompletedMissions==3&&B->GetSelectedCampaignMission()->Id==3&&NextAwardedOperationV38(B));
            Check(TEXT("victory_announces_marksmen_two_supports_and_armour"),A.Num()==4&&A.ContainsByPredicate([](const FAttachmentV38& E){return E.Kind==EAttachmentKindV38::Infantry&&E.Id==int32(EFrontlineRole::Marksman);})&&A.ContainsByPredicate([](const FAttachmentV38& E){return E.Kind==EAttachmentKindV38::Armour&&E.Id==int32(EFrontlineVehicle::MarkIVMale);}));
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            Delay=1;break;
        }
        case 1:Shot(TEXT("01-british-victory-next-attachments"));break;
        case 2:Language(true);V28ReviewPointer=FVector2D(1128,551);break;
        case 3:Shot(TEXT("02-unlocked-attachment-description-zh"));break;
        case 4:
        {
            Language(false);V28ReviewPointer=FVector2D(1790,110);HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));
            const auto L=FrontlineCampaignV32::GetLoadout(B);
            Check(TEXT("continue_opens_the_awarded_operation_with_new_roster"),B->Phase==EFrontlinePhase::Briefing&&B->GetSelectedCampaignMission()->Id==3&&(L.EligibleRoles&(1u<<int32(EFrontlineRole::Marksman)))&&(L.EligibleSupports&(1u<<int32(EFrontlineSupport::Gas)))&&UFrontlineVehicleSubsystem::Get(B->GetWorld())->IsUnlocked(EFrontlineVehicle::MarkIVMale));break;
        }
        case 5:Shot(TEXT("03-next-operation-war-room"));break;
        case 6:S.UnitIntroduction=true;S.UnitIntroductionPage=0;break;
        case 7:Shot(TEXT("04-next-operation-unit-introduction"));break;
        case 8:Select(EFrontlineFaction::German,0);Outcome(0);Check(TEXT("german_victory_announces_machinegun_and_smoke"),AwardedAttachmentsV38(B).Num()==2);break;
        case 9:Shot(TEXT("05-german-victory-attachments"));break;
        case 10:Clear();B->ContinueCampaign();B->SelectCampaignMission(0);Outcome(0);Check(TEXT("replay_win_does_not_reannounce_existing_unlocks"),!B->WasCampaignMissionAdvanced()&&AwardedAttachmentsV38(B).IsEmpty());break;
        case 11:Shot(TEXT("06-replay-has-no-new-attachments"));break;
        case 12:Clear();B->ContinueCampaign();B->SelectCampaignMission(1);Outcome(1);Check(TEXT("defeat_does_not_advance_or_announce_unlocks"),B->GetCurrentCampaign()->CompletedMissions==1&&AwardedAttachmentsV38(B).IsEmpty());break;
        case 13:Shot(TEXT("07-defeat-has-no-new-attachments"));break;
        case 14:Select(EFrontlineFaction::British,0);B->BeginBattle();B->Elapsed=130;B->SetActorTickEnabled(false);FFrontlineReviewV32::Camera(PC,0,0,5300,220);break;
        case 15:Shot(TEXT("08-framed-locks-gameplay-scale"));break;
        case 16:V38LockCoverageReview=true;break;
        case 17:Shot(TEXT("09-lock-opaque-metal-clear-background"));break;
        case 18:
            V38LockCoverageReview=false;Check(TEXT("actual_idle_and_supported_prone_fixture"),FrontlineMotionV38::PrepareIdleReview(B));
            R->Idle=true;FFrontlineReviewV32::Camera(PC,B->Trenches[FrontlineBattlefield::CentreTrench].X+250,0,2350,40);Delay=2.4;break;
        case 19:Shot(TEXT("10-idle-and-supported-prone-early"));Delay=4.4;break;
        case 20:
        {
            int32 Crew=0,Frames=0,Personal=0,Invalid=0;float Grip=0,Leg=0;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&(U->SquadId>=381000&&U->SquadId<=381005||U->SquadId==381010))
                if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
                {
                    const auto D=A->GetCharacterDiagnostics();++Crew;Frames+=D.EvaluatedFrames;Personal+=D.PersonalLayerFrames;
                    Invalid+=D.InvalidBoneFrames+D.UnsafeTorsoHierarchyFrames;Grip=FMath::Max(Grip,D.MaximumPersonalGripDriftCm);Leg=FMath::Max(Leg,D.MaximumTerrainLegLengthErrorCm);
                }
            Check(TEXT("actual_idle_fixture_evaluates_independent_layers"),Crew==8&&Frames>0&&Personal>0);
            Check(TEXT("actual_idle_fixture_preserves_grips_and_limb_lengths"),Invalid==0&&Grip<.1f&&Leg<.1f);
            R->Report->SetNumberField(TEXT("idle_personal_layer_frames"),Personal);R->Report->SetNumberField(TEXT("idle_max_grip_drift_cm"),Grip);R->Report->SetNumberField(TEXT("idle_max_leg_length_error_cm"),Leg);
            Shot(TEXT("11-idle-and-supported-prone-later"));break;
        }
        case 21:Check(TEXT("actual_blast_core_fixture"),FrontlineImpactV38::SetupVisualFixture(B,.06f));FFrontlineReviewV32::Camera(PC,0,150,5000,180);Delay=.3;break;
        case 22:Shot(TEXT("12-blast-core-and-scale"));break;
        case 23:Check(TEXT("actual_blast_soil_fixture"),FrontlineImpactV38::SetupVisualFixture(B,.42f));Delay=.3;break;
        case 24:Shot(TEXT("13-blast-earth-and-fragments"));break;
        case 25:Check(TEXT("actual_blast_smoke_fixture"),FrontlineImpactV38::SetupVisualFixture(B,1.5f));Delay=.3;break;
        case 26:Shot(TEXT("14-blast-rolling-smoke"));break;
        default:
        {
            TArray<TSharedPtr<FJsonValue>> Typography;int32 Bad=0,Truncated=0;
            for(const auto& E:V36TypographyAudit){Typography.Add(MakeShared<FJsonValueObject>(E.Value));if(!E.Value->GetBoolField(TEXT("fits")))++Bad;if(E.Value->GetBoolField(TEXT("truncated")))++Truncated;}
            Check(TEXT("visible_typography_inside_bounds"),Bad==0&&Typography.Num()>35);Check(TEXT("no_paragraph_ellipsis"),Truncated==0);Check(TEXT("visible_English_Chinese_coverage"),FrontlineLocalizationV35::WriteCoverageReport());Check(TEXT("affected_screens_requested"),R->Shots==14);
            R->Report->SetArrayField(TEXT("typography"),Typography);R->Report->SetNumberField(TEXT("overflowing_text_boxes"),Bad);R->Report->SetNumberField(TEXT("ellipsized_paragraphs"),Truncated);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetNumberField(TEXT("screenshots_requested"),R->Shots);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),TEXT("Changed combat/motion/FX checks plus actual first-victory next-operation announcements, continued loadout, replay/defeat/final safeguards and opaque lock contrast. Two time-separated idle/prone frames and three real cosmetic blast ages. Fourteen focused captures; isolated volatile profile. No normal save changed and no full campaign or broad performance claim."));
            FString Out;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(R->Directory/TEXT("review.json")));
            V28ReviewPointer.Reset();V27ImageAuditActive=V36TypographyAuditActive=V38LockCoverageReview=false;FPlatformMisc::RequestExit(false);return false;
        }
        }
        ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
    }));
}
}
