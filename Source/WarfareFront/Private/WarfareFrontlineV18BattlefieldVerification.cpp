#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "Engine/World.h"

void AWarfareFrontlineGameMode::RunV18BattlefieldChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    // Short, isolated logic fixtures: explicit deployment, relocation, cooldown
    // and objective-time setup. No assertion of natural match duration or physics.
    Check(TEXT("v18_battlefield_fixture_is_volatile"),bProgressionVolatile);
    if (!bProgressionVolatile) return;
    const auto SavedCampaigns=CampaignRuns; const auto SavedReceipts=RewardedEncounterIds;
    const auto SavedBritish=ArmyProgress[0],SavedGerman=ArmyProgress[1];
    const auto SavedFaction=PlayerFaction; const FGuid SavedActive=ActiveCampaignId;
    const FString SavedStatus=ProgressionSaveStatus;
    CampaignRuns.Empty(); ActiveCampaignId.Invalidate(); ArmyProgress[0]={}; ArmyProgress[1]={};
    PlayerFaction=EFrontlineFaction::British; ResetEncounter();
    bool Geometry=Trenches.Num()==5;
    for (int32 I=0;I<Trenches.Num();++I)
        Geometry=Geometry && FMath::IsNearlyEqual(Trenches[I].X,(I-2)*4000.f)
            && Trenches[I].Owner==(I<2?0:(I>2?1:INDEX_NONE));
    Check(TEXT("v18_five_trenches_span_160_metres_with_empty_second_lines"),Geometry && Units.IsEmpty() && Squads.IsEmpty());
    bool GroundMatches=true;
    for (const auto& Trench:Trenches)
    {
        const float Z=AWarfareFrontlineGround::HeightAt(Trench.X,0.f);
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(V18Terrain),true);
        const bool HitGround=GetWorld()->LineTraceSingleByChannel(Hit,FVector(Trench.X,0,Z+1000),FVector(Trench.X,0,Z-500),ECC_Visibility,Params);
        GroundMatches=GroundMatches && HitGround && Cast<AWarfareFrontlineGround>(Hit.GetActor()) && FMath::Abs(Hit.ImpactPoint.Z-Z)<1.5f
            && AWarfareFrontlineGround::HeightAt(Trench.X+280,0.f)-Z>55.f;
    }
    Check(TEXT("v18_all_five_cut_surfaces_have_real_collision_at_shared_height"),GroundMatches);
    BeginBattle();
    const bool PlayerDeployed=Recruit(EFrontlineRole::Rifle,0),EnemyDeployed=Recruit(EFrontlineRole::Rifle,1);
    bool RearEntry=PlayerDeployed && EnemyDeployed;
    for (const auto& S:Squads) RearEntry=RearEntry && S.Trench==INDEX_NONE && S.bReinforcement && S.Destination==(S.Team==0?0:4);
    for (const AWarfareFrontlineSoldier* U:Units) RearEntry=RearEntry && FMath::Abs(U->Feet().X)>11000.f;
    Check(TEXT("v18_both_armies_recruit_from_new_rear_boundaries_without_front_spawn"),RearEntry);

    ResetEncounter(); BeginBattle(); TArray<int32> Garrison;
    for (int32 I=0;I<3;++I) { if (SpawnSquad(0,EFrontlineRole::Rifle,0)) Garrison.Add(Squads.Last().Id); }
    bool Capacity=Garrison.Num()==3 && FreeSlot(0,0)==INDEX_NONE;
    Capacity=Capacity && Recruit(EFrontlineRole::Rifle,0) && Squads.Last().Destination==1;
    Check(TEXT("v18_fourth_reinforcement_routes_beyond_full_three_slot_home_trench"),Capacity);
    if (Garrison.Num()==3)
    {
        // The fourth reinforcement already reserves one second-line slot.
        SetSquadHold(0,Garrison[1],true);
        const bool Ordered=AdvanceTrench(0,0);
        Check(TEXT("v18_whole_trench_order_respects_hold_and_next_line_reservations"),Ordered
            && FindSquad(Garrison[1])->Trench==0 && FindSquad(Garrison[1])->bHoldPosition
            && FreeSlot(1,0)==INDEX_NONE && !AdvanceSquad(0,Garrison[1],0));
    }
    ResetEncounter(); BeginBattle(); const bool Walker=SpawnSquad(0,EFrontlineRole::Rifle,0);
    int32 WalkerId=Walker?Squads.Last().Id:INDEX_NONE; bool Traversed=Walker;
    for (int32 Index=0;Traversed && Index<4;++Index)
    {
        Traversed=AdvanceSquad(Index,WalkerId,0);
        auto* S=FindSquad(WalkerId);
        if (!Traversed || !S) break;
        for (const auto& Ref:S->Members) if (Ref.IsValid())
        {
            const FVector P=SoldierDestination(Ref.Get()); Ref->SetActorLocation(P+FVector(0,0,88));
        }
        UpdateSquads(); S=FindSquad(WalkerId);
        Traversed=Traversed && S && S->Trench==Index+1 && !S->bAdvancing && Phase==EFrontlinePhase::Fighting;
    }
    Check(TEXT("v18_each_of_four_depth_advances_stops_in_requested_trench"),Traversed && AdvanceCommands==4);
    if (Traversed)
    {
        AdvanceSquad(4,WalkerId,0); auto* S=FindSquad(WalkerId);
        for (const auto& Ref:S->Members) if (Ref.IsValid()) Ref->SetActorLocation(FVector(11750,Ref->Feet().Y,88));
        UpdateSquads(); const bool OldThresholdPassed=Phase==EFrontlinePhase::Fighting;
        for (const auto& Ref:S->Members) if (Ref.IsValid()) Ref->SetActorLocation(FVector(11900,Ref->Feet().Y,88));
        UpdateSquads(); Check(TEXT("v18_breakthrough_requires_new_118_metre_endline"),OldThresholdPassed && Phase==EFrontlinePhase::Victory);
    }

    ResetEncounter(); BeginBattle();
    bool DifferentCover=true;
    for (int32 Index:{0,1,2})
    {
        Trenches[Index].Owner=0;
        const bool Spawned=SpawnSquad(0,EFrontlineRole::Rifle,Index);
        DifferentCover=DifferentCover && Spawned;
        if (Spawned)
        {
            const auto* U=Squads.Last().Members[0].Get();
            const float Expected=Index==0?.75f:(Index==1?.82f:.66f);
            DifferentCover=DifferentCover && U && FMath::IsNearlyEqual(CoverFor(U),Expected);
        }
    }
    Check(TEXT("v18_rear_second_line_and_battered_key_position_have_distinct_actual_cover"),DifferentCover);
    ResetEncounter(); BeginBattle(); ArtilleryReadyAt[0]=SmokeReadyAt[0]=MortarReadyAt[0]=0;
    const bool InvalidFire=!RequestArtillery(FVector(12001,0,0)) && !RequestSmoke(FVector(0,651,0)) && !RequestMortar(FVector(-12001,0,0));
    const bool ValidFire=RequestArtillery(FVector(11000,0,0)) && RequestSmoke(FVector(-11000,0,0)) && RequestMortar(FVector(10500,0,0));
    bool InBounds=true; for (const auto& Strike:Strikes) InBounds=InBounds && FMath::Abs(Strike.Target.X)<=12000 && FMath::Abs(Strike.Target.Y)<=650;
    Check(TEXT("v18_support_reaches_extended_field_and_rejects_outside_coordinates"),InvalidFire && ValidFire && InBounds);

    ResetEncounter(); BeginBattle(); SpawnSquad(1,EFrontlineRole::Rifle,4);
    const int32 BeforeCounter=ActiveSquads(1); CaptureTrench(3,0);
    Check(TEXT("v18_lost_second_line_queues_counterattack_without_spawning_free_units"),EnemyCounterattackTrench==3
        && EnemyCounterattackUntil>Elapsed && ActiveSquads(1)==BeforeCounter);
    for (int32 I=0;I<RoleCount;++I) RecruitReadyAt[1][I]=Elapsed+10;
    Elapsed=4; ThinkEnemy(); const bool Waited=ActiveSquads(1)==BeforeCounter;
    Elapsed=11; ThinkEnemy();
    Check(TEXT("v18_counterattack_obeys_normal_readiness_and_rear_entry"),Waited && ActiveSquads(1)==BeforeCounter+1
        && Squads.Last().Role==EFrontlineRole::Mills && Squads.Last().Trench==INDEX_NONE && Squads.Last().Centre().X>11000);
    EnemyCounterattackUntil=0; EnemyWaveRecruitCount=2; EnemyRecruitAt=Elapsed; bEnemyRoleCommitted=false;
    for (int32 I=0;I<RoleCount;++I) RecruitReadyAt[1][I]=0;
    ThinkEnemy();
    Check(TEXT("v18_third_echelon_recruit_starts_bounded_reassembly_pause"),EnemyWaveRecruitCount==0 && EnemyRecruitAt>=Elapsed+14 && EnemyRecruitAt<=Elapsed+20);

    ResetEncounter(); const bool Campaign=StartNewCampaign(EFrontlineFaction::German); BeginBattle();
    const bool Occupied=SpawnSquad(0,EFrontlineRole::Rifle,2); CaptureTrench(2,0);
    UpdateCampaignObjective(40.f);
    Check(TEXT("v18_hold_mission_uses_real_third_trench_and_longer_ninety_second_goal"),Campaign && Occupied
        && GetEncounterCampaignMission() && GetEncounterCampaignMission()->HoldCenterSeconds==90 && CampaignHoldElapsed==40 && Phase==EFrontlinePhase::Fighting);
    SpawnSquad(1,EFrontlineRole::Rifle,2); UpdateCampaignObjective(1.f);
    const bool Interrupted=CampaignHoldElapsed==0;
    for (AWarfareFrontlineSoldier* U:Units) if (U->Team==1) U->SetActorLocation(FVector(7000,U->Feet().Y,88));
    UpdateCampaignObjective(89.f); const bool NotEarly=Phase==EFrontlinePhase::Fighting;
    UpdateCampaignObjective(1.1f);
    Check(TEXT("v18_contested_key_resets_timer_then_complete_hold_advances_campaign"),Interrupted && NotEarly && Phase==EFrontlinePhase::Victory
        && GetCurrentCampaign() && GetCurrentCampaign()->CompletedMissions==1);

    CampaignRuns=SavedCampaigns; RewardedEncounterIds=SavedReceipts; ArmyProgress[0]=SavedBritish; ArmyProgress[1]=SavedGerman;
    PlayerFaction=SavedFaction; ActiveCampaignId=SavedActive; ResetEncounter(); ProgressionSaveStatus=SavedStatus;
}
