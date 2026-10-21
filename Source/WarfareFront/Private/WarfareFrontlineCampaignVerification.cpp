#include "WarfareFrontline.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::RunV17CampaignChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    // Targeted, labelled fixture. Outcomes below are synthetic: this checks the
    // real finish/reward/save routes, not whether an empty battle can be won.
    // The parent agile review separately exercises the live objective and AI.
    if (!FParse::Param(FCommandLine::Get(),TEXT("WFV17AgileVerify")))
    { Check(TEXT("campaign_fixture_requires_isolated_flag"),false); return; }
    const FString OriginalFile=ProgressionFile,OriginalStatus=ProgressionSaveStatus;
    const bool OriginalVolatile=bProgressionVolatile,OriginalReadOnly=bProgressionReadOnly;
    const auto OriginalBritish=ArmyProgress[0],OriginalGerman=ArmyProgress[1];
    const auto OriginalCampaigns=CampaignRuns; const auto OriginalReceipts=RewardedEncounterIds;
    const FGuid OriginalActive=ActiveCampaignId; const EFrontlineFaction OriginalFaction=PlayerFaction;
    const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V17"));
    IFileManager::Get().MakeDirectory(*Directory,true);
    ProgressionFile=FPaths::Combine(Directory,TEXT("campaign-fixture-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json"));
    bProgressionVolatile=false; bProgressionReadOnly=false; CampaignRuns.Empty(); ActiveCampaignId.Invalidate(); RewardedEncounterIds.Empty();
    ArmyProgress[0]={}; ArmyProgress[1]={}; PlayerFaction=EFrontlineFaction::British; ResetEncounter();
    ArmyProgress[0].ResearchPoints=11; ArmyProgress[0].TechnologyMask=63; ArmyProgress[0].Victories=4;
    ArmyProgress[1].ResearchPoints=7; ArmyProgress[1].TechnologyMask=1; ArmyProgress[1].Defeats=2;
    bool LegacySafe=SaveProgression();
    FString Saved; TSharedPtr<FJsonObject> Json;
    LegacySafe=LegacySafe && FFileHelper::LoadFileToString(Saved,*ProgressionFile)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Saved),Json) && Json.IsValid();
    if (LegacySafe)
    {
        Json->RemoveField(TEXT("campaigns")); Json->RemoveField(TEXT("active_campaign"));
        for (int32 Schema:{1,2})
        {
            Json->SetNumberField(TEXT("schema"),Schema); FString Legacy;
            FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Legacy));
            LegacySafe=FFileHelper::SaveStringToFile(Legacy,*ProgressionFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) && LegacySafe;
            LoadProgression(true);
            LegacySafe=LegacySafe && !bProgressionReadOnly && CampaignRuns.IsEmpty() && !ActiveCampaignId.IsValid()
                && ArmyProgress[0].ResearchPoints==11 && ArmyProgress[0].TechnologyMask==63 && ArmyProgress[0].Victories==4
                && ArmyProgress[1].ResearchPoints==7 && ArmyProgress[1].TechnologyMask==1 && ArmyProgress[1].Defeats==2;
        }
    }
    Check(TEXT("campaign_schema_1_and_2_preserve_national_archives_without_inventing_missions"),LegacySafe);
    const bool Created=StartNewCampaign(EFrontlineFaction::British);
    const FGuid BritishId=ActiveCampaignId;
    Check(TEXT("campaign_new_british_preserves_technology_and_has_no_initial_squads"),Created && CampaignRuns.Num()==1
        && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==0 && Units.IsEmpty() && Squads.IsEmpty()
        && ArmyProgress[0].ResearchPoints==11 && ArmyProgress[0].TechnologyMask==63 && ArmyProgress[1].ResearchPoints==7);
    const auto BeforeTraining=ArmyProgress[0]; const float PreparedArtillery=ArtilleryReadyAt[0];
    const bool UnrelatedTraining=PurchaseTechnology(6);
    UE_LOG(LogTemp,Display,TEXT("V17 campaign preparation before=%.9f after=%.9f"),PreparedArtillery,ArtilleryReadyAt[0]);
    Check(TEXT("campaign_unrelated_training_keeps_operation_artillery_preparation"),UnrelatedTraining
        && FMath::IsNearlyEqual(ArtilleryReadyAt[0],PreparedArtillery,.0001f));
    ArmyProgress[0]=BeforeTraining; SaveProgression(); // Restore the labelled legacy-balance fixture.
    Check(TEXT("campaign_locked_and_out_of_range_missions_cannot_start"),!SelectCampaignMission(1) && !SelectCampaignMission(2)
        && !SelectCampaignMission(-1) && !SelectCampaignMission(3) && GetCurrentCampaign() && GetCurrentCampaign()->SelectedMission==0);
    BeginBattle();
    Check(TEXT("campaign_battle_binds_route_and_blocks_midbattle_switch"),GetEncounterCampaignMission() && GetEncounterCampaignMission()->Id==0
        && !StartNewCampaign(EFrontlineFaction::German) && !ContinueCampaign(BritishId) && !SelectCampaignMission(0));
    FinishBattle(0,TEXT("V17 fixture · synthetic first-operation victory"));
    const int32 AfterFirstPoints=GetResearchPoints();
    Check(TEXT("campaign_finish_victory_unlocks_exactly_one_and_rewards_once"),GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==1
        && GetCurrentCampaign()->SelectedMission==1 && WasCampaignMissionAdvanced() && AfterFirstPoints==14
        && GetEncounterCampaignMission() && GetEncounterCampaignMission()->Id==0);
    AwardEncounterResearch(0); bEncounterRewarded=false; AwardEncounterResearch(0);
    Check(TEXT("campaign_duplicate_encounter_receipt_does_not_advance_or_reward"),GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==1
        && GetResearchPoints()==AfterFirstPoints);
    RestartBattle(); BeginBattle(); RestartBattle();
    Check(TEXT("campaign_abandon_does_not_advance_or_reward"),GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==1 && GetResearchPoints()==AfterFirstPoints);
    BeginBattle(); FinishBattle(1,TEXT("V17 fixture · synthetic short defeat"));
    Check(TEXT("campaign_defeat_keeps_current_operation_locked_successor_and_no_short_loss_reward"),GetCurrentCampaign()
        && GetCurrentCampaign()->CompletedMissions==1 && GetCurrentCampaign()->SelectedMission==1 && !SelectCampaignMission(2) && GetResearchPoints()==AfterFirstPoints);
    const bool Replay=SelectCampaignMission(0); BeginBattle(); FinishBattle(0,TEXT("V17 fixture · completed-operation replay"));
    Check(TEXT("campaign_completed_replay_does_not_skip_next_operation"),Replay && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==1 && !WasCampaignMissionAdvanced());
    bool FinishedRoute=true;
    for (int32 MissionIndex=1;MissionIndex<3;++MissionIndex)
    {
        FinishedRoute=SelectCampaignMission(MissionIndex) && FinishedRoute; BeginBattle();
        FinishBattle(0,TEXT("V17 fixture · synthetic sequential-operation victory"));
        FinishedRoute=FinishedRoute && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==MissionIndex+1;
    }
    Check(TEXT("campaign_three_victories_complete_route_without_fourth_index"),FinishedRoute && GetCurrentCampaign()
        && GetCurrentCampaign()->CompletedMissions==3 && GetCurrentCampaign()->SelectedMission==2 && !SelectCampaignMission(3));
    const int32 BritishPoints=ArmyProgress[0].ResearchPoints;
    const bool GermanCreated=StartNewCampaign(EFrontlineFaction::German); const FGuid GermanId=ActiveCampaignId;
    Check(TEXT("campaign_new_german_is_independent_and_preserves_completed_british"),GermanCreated && CampaignRuns.Num()==2
        && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==0 && GetPlayerFaction()==EFrontlineFaction::German
        && GetResearchPoints()==7 && HasTechnology(0) && ArmyProgress[0].ResearchPoints==BritishPoints && CampaignRuns[0].CompletedMissions==3);
    const bool Again=StartNewCampaign(EFrontlineFaction::British); const FGuid SecondBritishId=ActiveCampaignId;
    Check(TEXT("campaign_new_same_faction_appends_without_erasing_old_route"),Again && CampaignRuns.Num()==3 && SecondBritishId!=BritishId
        && CampaignRuns[0].CompletedMissions==3 && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==0 && GetResearchPoints()==BritishPoints);
    const bool Continued=ContinueCampaign(GermanId); SaveProgression();
    FString BeforeReadback; FFileHelper::LoadFileToString(BeforeReadback,*ProgressionFile);
    LoadProgression(true);
    Check(TEXT("campaign_disk_readback_preserves_three_lines_active_selection_and_both_armies"),Continued && !bProgressionReadOnly
        && CampaignRuns.Num()==3 && ActiveCampaignId==GermanId && GetPlayerFaction()==EFrontlineFaction::German
        && CampaignRuns[0].Id==BritishId && CampaignRuns[0].CompletedMissions==3 && CampaignRuns[2].Id==SecondBritishId
        && ArmyProgress[0].TechnologyMask==63 && ArmyProgress[0].ResearchPoints==BritishPoints && ArmyProgress[1].ResearchPoints==7);
    const bool OldContinued=ContinueCampaign(BritishId);
    Check(TEXT("campaign_continue_specific_old_line_restores_last_selected_unlocked_mission"),OldContinued
        && GetSelectedCampaignMission() && GetSelectedCampaignMission()->Id==2 && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==3);
    const FGuid ActiveBeforeInvalid=ActiveCampaignId; const int32 CountBeforeInvalid=CampaignRuns.Num();
    Check(TEXT("campaign_unknown_line_and_invalid_faction_do_not_mutate_archive"),!ContinueCampaign(FGuid::NewGuid())
        && !StartNewCampaign(EFrontlineFaction(2)) && ActiveCampaignId==ActiveBeforeInvalid && CampaignRuns.Num()==CountBeforeInvalid);
    const FString Invalid=TEXT("{\"schema\":999,\"campaigns\":[]}");
    FFileHelper::SaveStringToFile(Invalid,*ProgressionFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    LoadProgression(true); const bool Refused=bProgressionReadOnly && !StartNewCampaign(EFrontlineFaction::British) && !SaveProgression();
    FString AfterInvalid; FFileHelper::LoadFileToString(AfterInvalid,*ProgressionFile);
    Check(TEXT("campaign_unsupported_archive_is_preserved_and_never_overwritten_by_new_campaign"),Refused && AfterInvalid==Invalid);
    FFileHelper::SaveStringToFile(BeforeReadback,*ProgressionFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM); LoadProgression(true);
    UE_LOG(LogTemp,Display,TEXT("V17 campaign isolated fixture archive: %s"),*ProgressionFile);
    ProgressionFile=OriginalFile; bProgressionVolatile=OriginalVolatile; bProgressionReadOnly=OriginalReadOnly;
    ArmyProgress[0]=OriginalBritish; ArmyProgress[1]=OriginalGerman; CampaignRuns=OriginalCampaigns; RewardedEncounterIds=OriginalReceipts;
    ActiveCampaignId=OriginalActive; PlayerFaction=OriginalFaction; BoundCampaignId.Invalidate(); BoundCampaignEncounterId.Invalidate(); BoundCampaignMission=INDEX_NONE;
    ResetEncounter();
    ProgressionSaveStatus=OriginalStatus;
}
