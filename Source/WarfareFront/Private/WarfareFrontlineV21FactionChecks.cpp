#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AWarfareFrontlineGameMode::RunV21FactionChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    if (!FParse::Param(FCommandLine::Get(),TEXT("WFV21Review")) || !bProgressionVolatile)
    {
        Check(TEXT("faction_fixture_requires_v21_review_and_volatile_profile"),false);
        return;
    }
    const bool German=GetPlayerFaction()==EFrontlineFaction::German;
    const int32 Home=German?4:0,EnemyHome=German?0:4,Dir=German?-1:1,Front=Home+Dir;
    const FString Prefix=German?TEXT("german_"):TEXT("british_");
    auto Verify=[&](const TCHAR* Name,bool Passed) {Check(Prefix+Name,Passed);};
    auto Fresh=[&]() {ResetEncounter();BeginBattle();};
    auto Relocate=[&](int32 Id,float X)
    {
        const auto* Squad=FindSquad(Id); if (!Squad) return;
        const double Shift=X-Squad->Centre().X;
        for (const auto& Member:Squad->Members) if (Member.IsValid())
            Member->SetActorLocation(Member->GetActorLocation()+FVector(Shift,0,0));
    };
    Fresh();
    Verify(TEXT("direction_home_goal_are_physical_faction_sides"),
        GetTeamDirection(0)==Dir && GetTeamDirection(1)==-Dir
        && GetHomeTrenchIndex()==Home && GetHomeTrenchIndex(1)==EnemyHome
        && GetGoalTrenchIndex()==(German?-2:5) && GetHomeCameraFocusX()*(German?1.f:-1.f)>6000.f);
    const bool Recruited=Recruit(EFrontlineRole::Rifle,0) && Recruit(EFrontlineRole::Rifle,1);
    bool SpawnAndFacing=Recruited,RearDestination=Recruited;
    for (const auto& Squad:Squads)
    {
        const int32 ExpectedHome=Squad.Team==0?Home:EnemyHome,ExpectedDir=Squad.Team==0?Dir:-Dir;
        RearDestination &= Squad.Trench==INDEX_NONE && Squad.Destination==ExpectedHome && Squad.bReinforcement;
        SpawnAndFacing &= FMath::Abs(Squad.Centre().X+ExpectedDir*FrontlineBattlefield::SpawnHalfLength)<200.f;
        for (const auto& Member:Squad.Members) if (Member.IsValid())
            SpawnAndFacing &= Member->GetActorForwardVector().X*ExpectedDir>.99f;
    }
    Verify(TEXT("real_recruits_spawn_at_opposite_rear_edges_facing_the_enemy"),SpawnAndFacing);
    Verify(TEXT("real_recruits_seek_their_own_home_trench"),RearDestination);

    Fresh();
    bool FullReady=true;
    for (int32 I=0;I<3;++I) FullReady &= SpawnSquad(0,EFrontlineRole::Rifle,Front);
    FullReady &= SpawnSquad(0,EFrontlineRole::Rifle,Home);
    const int32 Walker=Squads.IsEmpty()?INDEX_NONE:Squads.Last().Id;
    const bool Dispatched=FullReady && AdvanceSquad(Home,Walker,0);
    const auto* Moving=FindSquad(Walker);
    Verify(TEXT("single_section_exits_toward_centre_past_three_full_friendly_slots"),
        Dispatched && Moving && Moving->Destination==2 && TrenchSquadCount(Front,0)==3);
    // Explicit routing fixture: place the issued marcher beyond the full
    // trench's exit, then use the production passage/reservation update.
    Relocate(Walker,Trenches[Front].X+Dir*220.f);
    UpdateSquads(); Moving=FindSquad(Walker);
    Verify(TEXT("crossed_full_trench_preserves_three_residents_and_forward_destination"),
        Moving && Moving->bAdvancing && Moving->Trench==INDEX_NONE && Moving->LastPassedTrench==Front
        && Moving->Destination==2 && TrenchSquadCount(Front,0)==3);
    const bool EnemyReady=SpawnSquad(1,EFrontlineRole::Rifle,EnemyHome);
    const int32 EnemyId=EnemyReady?Squads.Last().Id:INDEX_NONE;
    const bool EnemyDispatched=EnemyReady && AdvanceSquad(EnemyHome,EnemyId,1);
    const auto* Enemy=FindSquad(EnemyId);
    Verify(TEXT("opponent_section_advances_in_the_opposite_world_direction"),
        EnemyDispatched && Enemy && Enemy->Destination==EnemyHome-Dir
        && (Trenches[Enemy->Destination].X-Enemy->Centre().X)*-Dir>0.f);

    // Final tableau is solely for the root's two faction UI/camera/profile
    // review. Both armies include a real gunner and his rifle-armed guards.
    Fresh();
    bool FinalReady=SpawnSquad(0,EFrontlineRole::Rifle,Home) && SpawnSquad(0,EFrontlineRole::Lewis,Home)
        && SpawnSquad(0,EFrontlineRole::Mills,Home) && SpawnSquad(1,EFrontlineRole::Lewis,EnemyHome)
        && SpawnSquad(1,EFrontlineRole::Rifle,EnemyHome);
    for (AWarfareFrontlineSoldier* Unit:Units) if (Unit) Unit->SetActorTickEnabled(false);
    EnemyRecruitAt=EnemyThinkAt=EnemyAssaultAt=EnemyFireAssessAt=MAX_flt;
    bEnemyFirePending=false;
    Verify(TEXT("review_tableau_has_three_player_and_two_opponent_sections"),
        FinalReady && TrenchSquadCount(Home,0)==3 && TrenchSquadCount(EnemyHome,1)==2);
    UE_LOG(LogTemp,Display,TEXT("V21_FACTION_REVIEW german=%d dir=%d home=%d enemyHome=%d profileVolatile=1 relocationFixture=1 soldierTicksFrozen=1"),
        German,Dir,Home,EnemyHome);
}
