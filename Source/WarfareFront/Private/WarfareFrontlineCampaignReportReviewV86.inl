#include "WarfareFrontlineUIReviewAccessV8447.h"

namespace FrontlineReportV839
{
void RunCampaignUIRouteChecksV86(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
    const bool Allowed=B&&PC&&HUD&&FrontlineFor(PC)==B&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))||FParse::Param(FCommandLine::Get(),TEXT("WFV86Review")))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(User)&&!FrontlineUltimateArchiveV8215::Active(B)&&!WarfareFRRUCampaignV82::Active(B);
    Check(TEXT("v86_campaign_controller_requires_strict_disposable_no_save_profile"),Allowed);if(!Allowed)return;
    // A labelled, disposable completed-record fixture. No natural campaign is
    // played and no real profile is loaded. The final outcome and UI calls below
    // use the ordinary settlement/controller code, not simulated command flags.
    auto& S=ScreenState(PC);
    S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=S.Preparing=false;
    S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.Dossier=1917;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;
    MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;PC->CancelTargeting();
    B->AbandonBattleToOperations();
    const bool New=B->StartNewCampaign(B->GetPlayerFaction(),EFrontlineDifficulty::Veteran);
    Check(TEXT("v86_campaign_controller_private_run_created"),New);if(!New)return;
    const bool Fixture=PrepareCampaignVisualV86(B,PC,5,0);
    Check(TEXT("v86_campaign_controller_twenty_attempt_fixture"),Fixture);if(!Fixture)return;
    Close(PC,B);auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!Run)return;
    const int32 Last=B->GetCampaignMissions().Num()-1;
    Check(TEXT("v86_campaign_controller_fixture_mission_mask_is_bounded"),Last>=0&&Last<31);if(Last<0||Last>=31)return;
    Run->CompletedMissions=Run->SelectedMission=Last;Run->WonOperationMaskV52=(1u<<Last)-1u;Run->DispatchResolvedMaskV52=(1u<<(Last+1))-1u;
    B->BindCampaignEncounter();const auto* Mission=B->GetEncounterCampaignMission();
    Check(TEXT("v86_campaign_controller_real_final_mission_bound"),Mission&&Mission->Id==Last);if(!Mission||Mission->Id!=Last)return;
    const auto Before=GetCampaignReportV86(B);const FGuid Encounter=B->GetEncounterId();
    for(auto& R:B->CombatRecordV65)R={};Begin(B);
    for(int32 Team=0;Team<2;++Team)
    {
        RegisterSquad(B,Team,77,0,Team==0?6:8);auto& R=B->CombatRecordV65[Team];
        R.Total={Team==0?3:2,1};R.InfantrySources[0].Infantry=R.Total.Infantry;R.SupportSources[0].Armour=1;
        R.InfantryLost=Team==0?2:3;R.ArmourLost=1;R.DetailsV839.LostByRole[0]=R.InfantryLost;
        R.DetailsV839.ArmourLostByModel[0]=1;R.DetailsV839.ArmourDestroyedByModel[0]=1;
        auto& Section=R.DetailsV839.Squads[77];Section.Lost=R.InfantryLost;Section.Kills=R.InfantrySources[0];
    }
    B->Phase=EFrontlinePhase::Fighting;B->Elapsed=300;B->ShotsFired=20;FFrontlineUIV8447Review::Finish(B,0,TEXT("V86 labelled final-return controller fixture"));
    const int32 Attempts=Run->CombatRecordV65.BattlesRecorded;
    Check(TEXT("v86_campaign_controller_real_settlement_adds_one_both_army_return"),Attempts==Before.BattlesV86+1&&Run->CombatRecordV65.BattlesV86.Num()==21&&!Run->CombatRecordV65.BattlesV86.IsEmpty()&&Run->CombatRecordV65.BattlesV86.Last().Encounter==Encounter);
    // Two late support casualties and one late friendly loss belong to this same
    // encounter; no second victory, battle or research reward is created.
    B->CombatRecordV65[0].Total.Infantry+=2;B->CombatRecordV65[0].SupportSources[0].Infantry+=2;
    B->CombatRecordV65[0].InfantryLost+=1;B->CombatRecordV65[0].DetailsV839.LostByRole[0]+=1;
    B->CombatRecordV65[0].DetailsV839.Squads[77].Lost+=1;
    B->CombatRecordV65[1].Total.Infantry+=1;B->CombatRecordV65[1].InfantrySources[0].Infantry+=1;
    B->CombatRecordV65[1].InfantryLost+=2;B->CombatRecordV65[1].DetailsV839.LostByRole[0]+=2;
    B->CombatRecordV65[1].DetailsV839.Squads[77].Lost+=2;B->CombatRecordV65[1].DetailsV839.Squads[77].Kills.Infantry+=1;
    B->BattleOutroElapsed=7.1f;
    HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));const auto Settled=GetCampaignReportV86(B);
    Check(TEXT("v86_campaign_controller_after_battle_opens_total_report_on_ending_page"),Active(PC,B)&&View(PC).CampaignV86&&S.CampaignEndingV51&&HUD->GetPresentationPage()==18);
    Check(TEXT("v86_campaign_controller_tail_and_enemy_deployment_losses_accumulate_once"),Settled.BattlesV86==Attempts&&Settled.Armies[0].Total.Infantry==Before.Armies[0].Total.Infantry+5&&Settled.Armies[0].InfantryLost==Before.Armies[0].InfantryLost+3&&Settled.Armies[1].Total.Infantry==Before.Armies[1].Total.Infantry+3&&Settled.Armies[1].InfantryLost==Before.Armies[1].InfantryLost+5&&Settled.Armies[1].DetailsV839.DeployedByRole[0]==Before.Armies[1].DetailsV839.DeployedByRole[0]+8);
    PC->BeginBattle();HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));const auto Again=GetCampaignReportV86(B);
    Check(TEXT("v86_campaign_controller_enter_and_stale_settlement_do_not_recount_or_leave"),Active(PC,B)&&B->GetEncounterId()==Encounter&&Again.BattlesV86==Attempts&&Again.Armies[0].Total.Infantry==Settled.Armies[0].Total.Infantry);
    HUD->NotifyHitBoxClick(TEXT("ReportTabV839_5"));const float Width=FFrontlineUIV8447Review::Width(PC);
    FFrontlineUIV8447Review::Wheel(PC,true);const bool Down=View(PC).Offset==1;FFrontlineUIV8447Review::Wheel(PC,false);
    Check(TEXT("v86_campaign_controller_wheel_scrolls_report_without_camera_zoom"),Down&&View(PC).Offset==0&&FFrontlineUIV8447Review::Width(PC)==Width&&Active(PC,B));
    PC->ToggleMenu();Check(TEXT("v86_campaign_controller_escape_returns_to_ending_only"),!Active(PC,B)&&S.CampaignEndingV51&&HUD->GetPresentationPage()==18&&B->GetEncounterId()==Encounter);
    HUD->NotifyHitBoxClick(TEXT("CampaignDetailedReportV86"));Check(TEXT("v86_campaign_controller_ending_button_reopens_report"),Active(PC,B)&&View(PC).CampaignV86&&S.CampaignEndingV51);
    HUD->NotifyHitBoxClick(TEXT("ReportBackV839"));Check(TEXT("v86_campaign_controller_back_button_returns_to_ending"),!Active(PC,B)&&S.CampaignEndingV51);
    HUD->NotifyHitBoxClick(TEXT("V51EndingOperations"));HUD->NotifyHitBoxClick(TEXT("V51CampaignRecord"));HUD->NotifyHitBoxClick(TEXT("CampaignDetailedReportV86"));
    Check(TEXT("v86_campaign_controller_completed_record_reopens_after_return_to_operations"),B->Phase==EFrontlinePhase::Briefing&&Active(PC,B)&&S.CampaignEndingV51&&HUD->GetPresentationPage()==18&&GetCampaignReportV86(B).BattlesV86==Attempts);
    Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!Run)return;
    const auto Preserved=Run->CombatRecordV65;Run->CombatRecordV65.BattlesV86.Reset();const auto Old=GetCampaignReportV86(B);Run->CombatRecordV65=Preserved;
    Check(TEXT("v86_campaign_controller_legacy_missing_enemy_data_stays_explicitly_unrecorded"),Old.bEarlierAttemptsUnrecordedV86&&Old.Armies[0].Total.Infantry==Preserved.Total.Infantry&&Old.Armies[1].BattlesRecorded==0&&!Old.Armies[1].DetailsV839.bRecorded&&Old.Armies[1].DetailsV839.bIncludesEarlierUnrecordedBattles&&Run->CombatRecordV65.BattlesV86.Num()==21);
    Run->CombatRecordV65={};Run->CombatRecordV65.bIncludesEarlierUnrecordedBattles=true;const auto Missing=GetCampaignReportV86(B);Run->CombatRecordV65=Preserved;
    Check(TEXT("v86_campaign_controller_pre_v65_known_outcomes_do_not_fabricate_kills_or_details"),Missing.bEarlierAttemptsUnrecordedV86&&Missing.BattlesV86==Run->Research.Victories+Run->Research.Defeats&&Missing.Armies[0].BattlesRecorded==0&&Missing.Armies[0].Total.Total()==0&&!Missing.Armies[0].DetailsV839.bRecorded&&Missing.Armies[1].Total.Total()==0);
    Check(TEXT("v86_campaign_controller_retained_return_numbering_preserves_known_older_attempts"),ReportUIV839::ReturnOrdinalV86(21,1,0)==21&&ReportUIV839::ReturnOrdinalV86(20,20,0)==1&&ReportUIV839::ReturnOrdinalV86(20,20,19)==20&&ReportUIV839::ReturnOrdinalV86(23,20,19)==23);
    // Keep page 18 active for the caller's next rendered frame. Its normal
    // EndingVisibleAtV83 clock is checked after the ordinary 0.7 s capture wait.
    S.EndingVisibleAtV83=-1;Check(TEXT("v86_campaign_controller_total_report_owns_page18_number_timeline"),Active(PC,B)&&HUD->GetPresentationPage()==18&&S.CampaignEndingV51);
}
}
