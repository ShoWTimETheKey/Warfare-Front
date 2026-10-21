#include "WarfareFrontline.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineGround.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::BeginV17AgileVerification()
{
    // Synchronous API fixtures only. Positions/readiness/decision timestamps are
    // explicitly arranged; no combat tick, screenshots, or historical QA suites.
    const double Started=FPlatformTime::Seconds();
    const auto Report=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Checks;
    int32 Passed=0;
    auto Check=[&](const FString& Name,bool Result)
    {
        const auto Entry=MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"),Name);Entry->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(Entry));Passed+=Result?1:0;
        UE_LOG(LogTemp,Display,TEXT("V17 agile %s: %s"),*Name,Result?TEXT("PASS"):TEXT("FAIL"));
    };
    auto Finish=[&]()
    {
        Phase=EFrontlinePhase::Briefing;
        Report->SetStringField(TEXT("scope"),TEXT("Synchronous real GameMode API fixtures; explicit spawn/relocation/readiness/time setup. Not a natural battle, rendering, performance or physics run."));
        Report->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());
        Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
        Report->SetNumberField(TEXT("total"),Checks.Num());Report->SetNumberField(TEXT("passed"),Passed);
        Report->SetNumberField(TEXT("failed_count"),Checks.Num()-Passed);
        Report->SetBoolField(TEXT("success"),Passed==Checks.Num());Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());Report->SetArrayField(TEXT("checks"),Checks);
        const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V17"));
        IFileManager::Get().MakeDirectory(*Directory,true);
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        const bool Saved=FFileHelper::SaveStringToFile(Json,*FPaths::Combine(Directory,TEXT("agile-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("WF_V17_AGILE_COMPLETE passed=%d total=%d failed=%d report_saved=%d"),Passed,Checks.Num(),Checks.Num()-Passed,int32(Saved));
        if(!Saved)UE_LOG(LogTemp,Error,TEXT("V17 agile receipt could not be saved"));
        FPlatformMisc::RequestExit(false);
    };
    Check(TEXT("verification_profile_is_volatile"),bProgressionVolatile);
    Check(TEXT("required_gameplay_assets_available"),AssetFailure.IsEmpty() && LoadedSoldier && IdleAnimation);
    if(!bProgressionVolatile || !AssetFailure.IsEmpty() || !LoadedSoldier || !IdleAnimation){Finish();return;}
    PlayerFaction=EFrontlineFaction::British;ArmyProgress[0]={};ArmyProgress[1]={};
    CampaignRuns.Empty();ActiveCampaignId.Invalidate();BoundCampaignId.Invalidate();BoundCampaignEncounterId.Invalidate();BoundCampaignMission=INDEX_NONE;

    ResetEncounter();
    Check(TEXT("briefing_starts_with_no_squads_on_either_side"),Units.IsEmpty() && Squads.IsEmpty() && ActiveSquads(0)==0 && ActiveSquads(1)==0);
    Check(TEXT("all_trench_views_expose_exactly_three_slots"),FFrontlineTrench::Capacity==3 && GetTrenchSquadSlots(0,0).Num()==3
        && GetTrenchSquadSlots(1,0).Num()==3 && GetTrenchSquadSlots(2,1).Num()==3);
    BeginBattle();
    Check(TEXT("begin_battle_does_not_spawn_initial_opponents"),Phase==EFrontlinePhase::Fighting && Units.IsEmpty() && Squads.IsEmpty());
    const bool PlayerRecruit=Recruit(EFrontlineRole::Marksman,0);
    Check(TEXT("player_recruit_adds_only_player_squad"),PlayerRecruit && ActiveSquads(0)==1 && ActiveSquads(1)==0 && LivingSoldiers(0)==GetSquadSize(EFrontlineRole::Marksman,0));
    const int32 PlayerBefore=LivingSoldiers(0);
    Check(TEXT("enemy_recruit_adds_only_enemy_squad"),Recruit(EFrontlineRole::Marksman,1) && ActiveSquads(1)==1 && LivingSoldiers(0)==PlayerBefore);

    ResetEncounter();BeginBattle();
    TArray<int32> Ids;bool Stationed=true;
    for(int32 Slot=0;Slot<3;++Slot)
    {
        const int32 Id=NextSquadId;
        Stationed&=SpawnSquad(0,EFrontlineRole::Marksman,0);Ids.Add(Id);
    }
    const TArray<int32> Slots=GetTrenchSquadSlots(0,0);
    bool ThreeSlots=Stationed && Slots.Num()==3 && TrenchSquadCount(0,0)==3 && FreeSlot(0,0)==INDEX_NONE;
    if(ThreeSlots)for(int32 I=0;I<3;++I)ThreeSlots&=Slots[I]==Ids[I];
    Check(TEXT("trench_has_three_distinct_occupied_slots"),ThreeSlots);
    if(Stationed)
    {
        const bool Single=AdvanceSquad(0,Ids[0],0);
        Check(TEXT("individual_advance_moves_exactly_one_squad"),Single && FindSquad(Ids[0])->bAdvancing && TrenchSquadCount(0,0)==2
            && !FindSquad(Ids[1])->bAdvancing && !FindSquad(Ids[2])->bAdvancing && AdvanceCommands==1);
        const bool Held=SetSquadHold(0,Ids[1],true,0);
        const bool Whole=AdvanceTrench(0,0);
        Check(TEXT("whole_trench_order_skips_held_squad"),Held && Whole && FindSquad(Ids[1])->Trench==0
            && FindSquad(Ids[1])->bHoldPosition && FindSquad(Ids[2])->bAdvancing && AdvanceCommands==2);
        Check(TEXT("all_remaining_held_blocks_whole_trench_order"),!AdvanceTrench(0,0) && AdvanceCommands==2);
        Check(TEXT("explicit_single_order_clears_hold"),AdvanceSquad(0,Ids[1],0) && !FindSquad(Ids[1])->bHoldPosition && FindSquad(Ids[1])->bAdvancing);
        TSet<int32> Reserved;
        for(int32 Id:Ids)if(const auto* Squad=FindSquad(Id))if(Squad->Destination==1 && Squad->bAdvancing)Reserved.Add(Squad->Slot);
        Check(TEXT("moving_squads_reserve_three_unique_destination_slots"),Reserved.Num()==3 && FreeSlot(1,0)==INDEX_NONE);
        const int32 FourthId=NextSquadId;const bool Fourth=SpawnSquad(0,EFrontlineRole::Marksman,0);
        Check(TEXT("fourth_advance_cannot_overbook_destination"),Fourth && !AdvanceSquad(0,FourthId,0)
            && FindSquad(FourthId)->Trench==0 && !FindSquad(FourthId)->bAdvancing && FreeSlot(1,0)==INDEX_NONE);
        Check(TEXT("enemy_side_cannot_issue_player_squad_order"),!AdvanceSquad(0,FourthId,1) && !SetSquadHold(0,FourthId,true,1));
    }

    // Reinforcements reserve seats through the real Recruit -> SpawnSquad path.
    // Only readiness is reset between the four synchronous recruitment calls.
    ResetEncounter();BeginBattle();bool Recruited=true;
    for(int32 I=0;I<4;++I){RecruitReadyAt[0][int32(EFrontlineRole::Marksman)]=Elapsed;Recruited&=Recruit(EFrontlineRole::Marksman,0);}
    int32 FirstReservations=0,NextReservations=0;TSet<int32> ReinforcementSlots;
    for(const auto& S:Squads)if(S.Team==0 && S.bReinforcement)
    {
        if(S.Destination==0){++FirstReservations;ReinforcementSlots.Add(S.Slot);}
        if(S.Destination==1)++NextReservations;
    }
    Check(TEXT("reinforcement_routing_respects_three_reserved_seats"),Recruited && FirstReservations==3 && ReinforcementSlots.Num()==3 && NextReservations==1 && ActiveSquads(1)==0);

    struct FAIResult{bool Ready=false;FVector Point=FVector::ZeroVector;float ExecuteAt=0;int32 Fired=0;};
    auto EnemyStrikeCount=[&]()
    {int32 Count=0;for(const auto& Strike:Strikes)if(Strike.Team==1)++Count;return Count;};
    auto RunAI=[&](const FString& Prefix,bool WithPlayerFire,bool MoveFriendlyIntoBlast)
    {
        FAIResult Result;ResetEncounter();BeginBattle();Elapsed=120.f;
        EnemyRecruitAt=EnemyAssaultAt=MAX_flt;EnemyFireAssessAt=Elapsed;
        ArtilleryReadyAt[0]=ArtilleryReadyAt[1]=0;MortarReadyAt[1]=MAX_flt;
        const bool PlayerSpawn=SpawnSquad(0,EFrontlineRole::Rifle,0);
        ThinkEnemy();
        Check(Prefix+TEXT("_no_observer_means_no_fire_order"),PlayerSpawn && !bEnemyFirePending && EnemyStrikeCount()==0);
        const bool EnemySpawn=SpawnSquad(1,EFrontlineRole::Marksman,2);
        AWarfareFrontlineSoldier* Observed=nullptr;AWarfareFrontlineSoldier* Observer=nullptr;
        auto Place=[&](float Lift)
        {
            for(AWarfareFrontlineSoldier* Unit:Units)
            {
                const float X=Unit->Team==0?-1800.f+float(Unit->MemberIndex%3)*42.f:-300.f+float(Unit->MemberIndex)*42.f;
                const float Y=-140.f+float(Unit->MemberIndex/3)*46.f;
                Unit->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88.f+Lift));
                Unit->bMoving=false;
                if(Unit->Team==0 && !Observed)Observed=Unit;
                if(Unit->Team==1 && !Observer)Observer=Unit;
            }
        };
        Place(0.f);float SightFixtureLift=0.f;
        if(Observed && Observer && !HasSight(Observer,Observed)){SightFixtureLift=300.f;Place(SightFixtureLift);}
        const bool Visible=PlayerSpawn && EnemySpawn && Observed && Observer && HasSight(Observer,Observed) && CanEnemyObserve(Observed);
        Check(Prefix+TEXT("_actual_world_observer_can_see_cluster"),Visible);
        Report->SetNumberField(Prefix+TEXT("_explicit_sight_fixture_lift_cm"),SightFixtureLift);
        if(!Visible)return Result;
        FFrontlineSmokeZone Screen;Screen.Team=0;Screen.Centre=FVector(-1050,-140,0);Screen.Radius=550;Screen.StartedAt=Elapsed;Screen.ExpiresAt=Elapsed+20;
        SmokeZones.Add(Screen);EnemyFireAssessAt=Elapsed;ThinkEnemy();
        Check(Prefix+TEXT("_smoke_blocks_observation_and_fire_commit"),!CanEnemyObserve(Observed) && !bEnemyFirePending && EnemyStrikeCount()==0);
        SmokeZones.Empty();EnemyDecisionRandom.Initialize(731917);EnemyFireAssessAt=Elapsed;
        const int32 DecisionSeed=EnemyDecisionRandom.GetCurrentSeed();const float AssessBefore=EnemyFireAssessAt;
        if(WithPlayerFire)
        {
            const bool Requested=RequestArtillery(FVector(-4800,-500,0),0);
            Check(Prefix+TEXT("_player_artillery_does_not_trigger_or_reseed_enemy"),Requested && !bEnemyFirePending && EnemyStrikeCount()==0
                && EnemyDecisionRandom.GetCurrentSeed()==DecisionSeed && EnemyFireAssessAt==AssessBefore);
        }
        ThinkEnemy();Result.Ready=bEnemyFirePending;Result.Point=EnemyObservedFireTarget;Result.ExecuteAt=EnemyFireExecuteAt;
        Check(Prefix+TEXT("_observed_cluster_queues_delayed_order"),Result.Ready && Result.ExecuteAt>=Elapsed+2.8f && Result.ExecuteAt<=Elapsed+5.8f && EnemyStrikeCount()==0);
        if(!Result.Ready)return Result;
        Elapsed=Result.ExecuteAt-.01f;ThinkEnemy();
        Check(Prefix+TEXT("_fire_does_not_execute_before_transmission_delay"),bEnemyFirePending && EnemyStrikeCount()==0);
        // Move the observed men after the point was recorded. Artillery must
        // retain the old XY; the normal API recomputes terrain height at launch.
        for(AWarfareFrontlineSoldier* Unit:Units)if(Unit->Team==0)Unit->AddActorWorldOffset(FVector(0,450,0));
        if(MoveFriendlyIntoBlast)
            Observer->SetActorLocation(FVector(Result.Point.X+250,Result.Point.Y,AWarfareFrontlineGround::HeightAt(Result.Point.X+250,Result.Point.Y)+88));
        Elapsed=Result.ExecuteAt+.01f;ThinkEnemy();Result.Fired=EnemyStrikeCount();
        if(MoveFriendlyIntoBlast)
            Check(Prefix+TEXT("_friendly_approach_cancels_pending_bombardment"),Result.Fired==0 && !bEnemyFirePending);
        else
        {
            bool Frozen=Result.Fired==1;
            for(const auto& Strike:Strikes)if(Strike.Team==1)Frozen&=FVector::Dist2D(Strike.Target,Result.Point)<.01f;
            Check(Prefix+TEXT("_delayed_order_fires_at_recorded_point"),Frozen && !bEnemyFirePending);
        }
        return Result;
    };
    const FAIResult Baseline=RunAI(TEXT("ai_baseline"),false,false);
    const FAIResult PlayerFire=RunAI(TEXT("ai_with_player_fire"),true,false);
    Check(TEXT("enemy_schedule_and_target_independent_of_player_artillery"),Baseline.Ready && PlayerFire.Ready && Baseline.Fired==1 && PlayerFire.Fired==1
        && FVector::Dist2D(Baseline.Point,PlayerFire.Point)<.01f && FMath::IsNearlyEqual(Baseline.ExecuteAt,PlayerFire.ExecuteAt,.0001f));
    RunAI(TEXT("ai_friendly_safety"),false,true);

    Check(TEXT("grenade_helper_retains_close_danger_and_zero_edge_damage"),FrontlineGrenade::HitChanceFalloff(0,220)==1.f
        && FrontlineGrenade::HitChanceFalloff(40,220)==1.f && FrontlineGrenade::HitChanceFalloff(100,220)<.5f && FrontlineGrenade::HitChanceFalloff(220,220)==0.f);
    Report->SetNumberField(TEXT("grenade_max_flight_seconds"),FrontlineGrenade::FlightSeconds(1200));
    Check(TEXT("grenade_helper_flight_is_bounded_and_grows_with_range"),FMath::IsNearlyEqual(FrontlineGrenade::FlightSeconds(0),1.45f,.0001f)
        && FMath::IsNearlyEqual(FrontlineGrenade::FlightSeconds(1200),1.9f,.0001f) && FrontlineGrenade::FlightSeconds(350)<FrontlineGrenade::FlightSeconds(1150));

    // Real objective accumulation/contestation/completion in three synchronous
    // updates. No synthetic FinishBattle call and no 45-second wall-clock wait.
    ResetEncounter();const bool HoldCampaign=StartNewCampaign(EFrontlineFaction::German);BeginBattle();
    const bool Defenders=SpawnSquad(0,EFrontlineRole::Marksman,1);Trenches[1].Owner=0;
    UpdateCampaignObjective(20.f);
    Check(TEXT("hold_center_accumulates_actual_twenty_seconds"),HoldCampaign && Defenders && Phase==EFrontlinePhase::Fighting
        && FMath::IsNearlyEqual(GetCampaignObjectiveProgress(),20.f/45.f,.0001f));
    const bool Contestants=SpawnSquad(1,EFrontlineRole::Marksman,2);
    for(AWarfareFrontlineSoldier* Unit:Units)if(Unit->Team==1)
    {
        const float X=Trenches[1].X,Y=float(Unit->MemberIndex)*80.f;
        Unit->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));
    }
    UpdateCampaignObjective(1.f);
    Check(TEXT("hold_center_enemy_contestation_resets_progress"),Contestants && Phase==EFrontlinePhase::Fighting && GetCampaignObjectiveProgress()==0.f);
    for(AWarfareFrontlineSoldier* Unit:Units)if(Unit->Team==1)
        Unit->SetActorLocation(FVector(2200,0,AWarfareFrontlineGround::HeightAt(2200,0)+88));
    UpdateCampaignObjective(45.f);
    const auto* HeldRun=GetCurrentCampaign();
    Check(TEXT("hold_center_actual_completion_wins_and_advances_campaign"),Phase==EFrontlinePhase::Victory && WasCampaignMissionAdvanced()
        && HeldRun && HeldRun->Faction==EFrontlineFaction::German && HeldRun->CompletedMissions==1);
    RunV17CampaignChecks(Check);
    Finish();
}
