#include "WarfareFrontline.h"
#include "WarfareFrontlinePacingV24.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AWarfareFrontlineGameMode::RunV24PacingChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    // One small synchronous addition to the already isolated V24 officer fixture.
    // Readiness/positions are explicit fixtures, not a simulated natural battle.
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVerify"));
    Check(TEXT("pacing_fixture_has_explicit_opt_in"),Allowed);if(!Allowed)return;
    const bool PriorVolatile=bProgressionVolatile;bProgressionVolatile=true;
    const EFrontlineFaction PriorFaction=PlayerFaction;
    // V59 supersedes the original frame-counter prices. Check the tactical
    // opportunity cost, not historical constants which no longer ship.
    const float RifleRate=RoleSize(EFrontlineRole::Rifle,0)/RoleRecruitSeconds(EFrontlineRole::Rifle);
    bool Rates=true;
    for(EFrontlineRole TestedRole:{EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Pioneer})
    {
        const float Seconds=RoleRecruitSeconds(TestedRole),Rate=RoleSize(TestedRole,0)/Seconds;
        Rates&=FMath::IsFinite(Seconds)&&Seconds>0&&RifleRate>Rate;
        UE_LOG(LogTemp,Display,TEXT("WF_V24_RECRUIT_SECONDS role=%d actual=%.3f men_per_second=%.3f"),int32(TestedRole),Seconds,Rate);
    }
    const float GunRate=RoleSize(EFrontlineRole::Lewis,0)/RoleRecruitSeconds(EFrontlineRole::Lewis);
    Check(TEXT("v59_rifles_keep_manpower_advantage_without_fourfold_mg_opportunity_cost"),Rates&&RifleRate/GunRate<3.f);
    Check(TEXT("v59_rifle_section_size_is_symmetric"),RoleSize(EFrontlineRole::Rifle,0)==6&&RoleSize(EFrontlineRole::Rifle,1)==6);
    Check(TEXT("grenade_readiness_rate_is_seventy_percent"),FMath::IsNearlyEqual(FrontlinePacingV24::GrenadeReadinessRate,.70f));
    SetBattleSpeed(2.f);
    Check(TEXT("legacy_two_speed_request_uses_one_point_five_world_clock"),BattleSpeed==1.5f && UGameplayStatics::GetGlobalTimeDilation(this)==1.5f);
    SetBattleSpeed(1.f);
    bool SpawnBounds=true;
    for(int32 Direction:{-1,1})for(float Centre:{-10800.f,0.f,10800.f})for(float Width:{3400.f,6000.f,9000.f})
    {
        const float LegalCentre=FMath::Clamp(Centre,-FrontlineBattlefield::SupportHalfLength+Width*.2f,FrontlineBattlefield::SupportHalfLength-Width*.2f);
        float Previous=0;
        for(int32 Member=0;Member<6;++Member)
        {
            const float X=FrontlinePacingV24::ReinforcementStartX(Direction,Centre,Width,Member,.5f);
            SpawnBounds&=X*Direction<-FrontlineBattlefield::SpawnHalfLength;
            SpawnBounds&=(X-LegalCentre)*Direction<-Width*.5f-300.f;
            if(Member)SpawnBounds&=(X-Previous)*Direction<=-100.f;
            Previous=X;
        }
    }
    Check(TEXT("both_faction_rear_edges_hide_creation_and_stagger_members"),SpawnBounds);
    for(EFrontlineFaction Faction:{EFrontlineFaction::British,EFrontlineFaction::German})
    {
        ResetEncounter();SelectPlayerFaction(Faction);BeginBattle();
        const FString Prefix=Faction==EFrontlineFaction::British?TEXT("british_"):TEXT("german_");
        bool Recruited=true;
        for(int32 Team=0;Team<2;++Team)for(int32 N=0;N<9;++N)
        {
            RecruitReadyAt[Team][int32(EFrontlineRole::Marksman)]=Elapsed;
            Recruited&=Recruit(EFrontlineRole::Marksman,Team);
        }
        Check(Prefix+TEXT("nine_squads_per_side_recruit_without_replacement_cap"),Recruited && ActiveSquads(0)==9 && ActiveSquads(1)==9);
        bool Capacity=true;
        for(int32 Team=0;Team<2;++Team)for(int32 Trench=0;Trench<Trenches.Num();++Trench)
        {
            TSet<int32> Seats;int32 Count=0;
            for(const auto& Squad:Squads)if(Squad.Team==Team && Squad.LivingCount()>0 &&
                (Squad.Trench==Trench || (Squad.bAdvancing && Squad.Destination==Trench)))
            {++Count;Seats.Add(Squad.Slot);}
            Capacity&=Count<=FrontlineBattlefield::TrenchCapacity && Seats.Num()==Count;
        }
        Check(Prefix+TEXT("unlimited_recruitment_preserves_unique_three_seat_reservations"),Capacity);
        AWarfareFrontlineSoldier* A=nullptr;AWarfareFrontlineSoldier* B=nullptr;bool Entering=true;
        for(AWarfareFrontlineSoldier* Unit:Units)if(Unit)
        {
            Unit->SetActorTickEnabled(false);
            Entering&=!Unit->IsActiveCombatant() && Unit->Feet().X*GetTeamDirection(Unit->Team)<-FrontlineBattlefield::SpawnHalfLength;
            if(Unit->Team==0 && !A)A=Unit;else if(Unit->Team==1 && !B)B=Unit;
        }
        Check(Prefix+TEXT("all_recruits_start_outside_entry_line_as_reserves"),Entering && A && B);
        if(A && B)
        {
            const float Health=A->Health,Pressure=A->Suppression;
            A->ReceiveHit(500.f,.9f,1,FVector(1,0,0),EFrontlineDamageKind::Artillery,1.f);
            const bool Protected=A->Health==Health && A->Suppression==Pressure && !HasSight(A,B) && !AcquireTarget(A);
            Fire(B,A);const bool StillProtected=A->Health==Health;
            Check(Prefix+TEXT("off_field_reserves_cannot_target_or_take_direct_and_explosive_hits"),Protected && StillProtected);
            const int32 Direction=GetTeamDirection(A->Team);
            FVector At=A->GetActorLocation();At.X=Direction*(-FrontlineBattlefield::SpawnHalfLength+2.f);A->SetActorLocation(At);
            A->Tick(.001f); // Actual entry transition with an explicit 1 ms fixture step.
            Check(Prefix+TEXT("physical_entry_activates_combatant_on_actual_soldier_tick"),A->IsActiveCombatant() && !B->IsActiveCombatant());
        }
    }
    ResetEncounter();SelectPlayerFaction(PriorFaction);bProgressionVolatile=PriorVolatile;
}
