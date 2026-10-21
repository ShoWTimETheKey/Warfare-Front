#include "WarfareFrontlineCombatV26.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineGrenade.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include <limits>

TSharedRef<FJsonObject> RunFrontlineV26CombatChecks(AWarfareFrontlineGameMode* B)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;
    auto Check=[&](const TCHAR* Name,bool bPassed,const FString& Detail=FString())
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),bPassed);J->SetStringField(TEXT("detail"),Detail);
        Checks.Add(MakeShared<FJsonValueObject>(J));
        UE_LOG(LogTemp,Display,TEXT("WF_V26_COMBAT %s %s %s"),Name,bPassed?TEXT("PASS"):TEXT("FAIL"),*Detail);
    };
    ON_SCOPE_EXIT
    {
        int32 Passed=0;for(const auto& C:Checks)if(C->AsObject()->GetBoolField(TEXT("passed")))++Passed;
        Report->SetArrayField(TEXT("checks"),Checks);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Checks.Num());
    };
    if(!B || !FParse::Param(FCommandLine::Get(),TEXT("WFV26Review")) || !B->bProgressionVolatile)
    { Check(TEXT("isolated_review_required"),false,TEXT("Requires -WFV26Review and volatile progression."));return Report; }
    const auto SavedRuns=B->CampaignRuns;const auto SavedActive=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
    const auto SavedBritish=B->ArmyProgress[0],SavedGerman=B->ArmyProgress[1];const auto SavedRewards=B->RewardedEncounterIds;
    const bool bWasTicking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
    ON_SCOPE_EXIT
    {
        B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedActive;B->PlayerFaction=SavedFaction;
        B->ArmyProgress[0]=SavedBritish;B->ArmyProgress[1]=SavedGerman;B->RewardedEncounterIds=SavedRewards;
        B->ResetEncounter();B->SetActorTickEnabled(bWasTicking);
    };
    Report->SetStringField(TEXT("scope"),TEXT("Explicit stationary fixtures; real hit, Fire and ResolveStrike paths. No natural battle or new animation certification."));
    auto Setup=[&](EFrontlineFaction Faction)
    {
        B->ResetEncounter();B->StartNewCampaign(Faction);B->BeginBattle();
        const bool Spawned=B->SpawnSquad(0,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(0))
            && B->SpawnSquad(1,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(1))
            && B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(0))
            && B->SpawnSquad(1,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(1));
        for(AWarfareFrontlineSoldier* U:B->Units){U->SetActorTickEnabled(false);U->bMoving=false;U->Suppression=0;U->SetActorLocation(FVector(0,6000+U->SquadId*800+U->MemberIndex*80,1288));}
        return Spawned && B->Phase==EFrontlinePhase::Fighting;
    };
    auto Find=[&](int32 Team,EFrontlineRole Role,int32 Member)->AWarfareFrontlineSoldier*
    {for(AWarfareFrontlineSoldier* U:B->Units)if(U->Team==Team && U->Role==Role && U->MemberIndex==Member)return U;return nullptr;};
    auto Place=[&](AWarfareFrontlineSoldier* U,float X,float Y=0.f){U->SetActorLocation(FVector(X,Y,1288));};
    auto SeedFor=[&](float Probability,bool bHit)
    {
        for(int32 Seed=1;Seed<100000;++Seed){FRandomStream Probe(Seed);if((Probe.FRand()<Probability)==bHit){B->Random.Initialize(Seed);return true;}}
        return false;
    };
    if(!Setup(EFrontlineFaction::British)){Check(TEXT("combat_fixture_ready"),false,B->AssetFailure);return Report;}
    auto* Rifle=Find(0,EFrontlineRole::Rifle,0);auto* Enemy=Find(1,EFrontlineRole::Rifle,0);
    auto* GrenadeVictim=Find(1,EFrontlineRole::Rifle,1);auto* Friendly=Find(0,EFrontlineRole::Rifle,1);
    auto* Gun=Find(0,EFrontlineRole::Lewis,0);auto* Bearer=Find(0,EFrontlineRole::Lewis,1);auto* GermanGun=Find(1,EFrontlineRole::Lewis,0);
    if(!Rifle || !Enemy || !GrenadeVictim || !Friendly || !Gun || !Bearer || !GermanGun){Check(TEXT("combat_fixture_ready"),false,TEXT("Missing required actual roles."));return Report;}
    Check(TEXT("combat_fixture_ready"),true);
    Enemy->ReceiveHit(0,.1f,0);Enemy->ReceiveHit(-1,.1f,0);
    Enemy->ReceiveHit(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),0);
    Enemy->ReceiveHit(std::numeric_limits<float>::infinity(),-.9f,0);
    Check(TEXT("zero_negative_nonfinite_hit_never_wounds_or_heals"),Enemy->Health==100.f && FMath::IsNearlyEqual(Enemy->Suppression,.2f) && B->Casualties[1]==0);
    Enemy->ReceiveHit(1.e-12f,0,0,FVector(1,0,0));
    Check(TEXT("tiny_positive_bullet_hit_is_immediately_fatal"),!Enemy->IsAlive() && Enemy->Health==0 && Enemy->GetDeathState().bStarted && B->Casualties[1]==1);
    Enemy->ReceiveHit(1000,.9f,0);
    Check(TEXT("dead_soldier_cannot_be_killed_or_counted_twice"),B->Casualties[1]==1 && B->DamageDealt[0]==100);
    GrenadeVictim->ReceiveHit(1.e-12f,0,0,FVector(1,0,0),EFrontlineDamageKind::Grenade,.01f);
    Friendly->ReceiveHit(1.e-12f,0,0,FVector(1,0,0),EFrontlineDamageKind::Artillery,.01f);
    Check(TEXT("tiny_explosion_hits_kill_both_enemy_and_friendly"),GrenadeVictim->Health==0 && Friendly->Health==0 && B->FriendlyFireDamage==100
        && GrenadeVictim->GetDeathState().Kind==EFrontlineDamageKind::Grenade && Friendly->GetDeathState().Kind==EFrontlineDamageKind::Artillery);

    Enemy=Find(1,EFrontlineRole::Rifle,2);if(!Enemy){Check(TEXT("remaining_rifle_fixture_ready"),false);return Report;}
    Place(Rifle,1000);Place(Enemy,2500);Enemy->Suppression=0;
    const float Open=B->BulletHitChance(Rifle,Enemy);Place(Enemy,4000);const float Far=B->BulletHitChance(Rifle,Enemy);
    Place(Enemy,2500);Rifle->Suppression=.8f;const float Suppressed=B->BulletHitChance(Rifle,Enemy);Rifle->Suppression=0;
    const float TrenchX=B->Trenches[B->GetHomeTrenchIndex(1)].X;Place(Rifle,TrenchX-1500);Place(Enemy,TrenchX);
    const float Covered=B->BulletHitChance(Rifle,Enemy);Place(Rifle,1000);Place(Enemy,2500);
    Check(TEXT("range_suppression_and_trench_reduce_hit_probability"),Open>0 && Open<1 && Far<Open && Suppressed<Open && Covered<Open,
        FString::Printf(TEXT("open=%.6f far=%.6f suppressed=%.6f covered=%.6f"),Open,Far,Suppressed,Covered));
    const auto& MG=FrontlineWeapons::ProfileFor(Gun);const auto& MGDE=FrontlineWeapons::ProfileFor(GermanGun);const auto& Escort=FrontlineWeapons::ProfileFor(Bearer);
    Check(TEXT("national_mg_cadence_and_rifle_escort_remain_distinct"),Gun->IsMachineGunner() && !Bearer->IsAutomaticWeapon()
        && MG.HitProbabilityScale<Escort.HitProbabilityScale && MG.FireRangeCm>Escort.FireRangeCm && MG.RoundIntervalSeconds==.100f
        && MGDE.RoundIntervalSeconds==.120f && MG.RecoveryMin>=2.6f && Gun->GetMagazineCapacity()==47 && GermanGun->GetMagazineCapacity()==100);
    Check(TEXT("rifle_expected_shots_include_v38_accuracy_uplift"),FMath::IsNearlyEqual(1.f/Open,4.f/((.78f-1500.f/15000.f)*1.20f),.001f));
    const int32 BeforeShots=B->ShotsFired;const bool MissSeed=SeedFor(Open,false);B->Fire(Rifle,Enemy);
    Check(TEXT("real_fire_miss_leaves_full_alive_marker_and_only_pressure"),MissSeed && Enemy->Health==100 && Enemy->Suppression>0 && B->ShotsFired==BeforeShots+1);
    const bool HitSeed=SeedFor(Open,true);B->Fire(Rifle,Enemy);
    Check(TEXT("real_fire_landed_round_enters_natural_death_path"),HitSeed && Enemy->Health==0 && Enemy->GetDeathState().bStarted && B->ShotsFired==BeforeShots+2);

    if(!Setup(EFrontlineFaction::German)){Check(TEXT("explosion_fixture_ready"),false,B->AssetFailure);return Report;}
    auto* Target=Find(1,EFrontlineRole::Rifle,0);Friendly=Find(0,EFrontlineRole::Rifle,0);Gun=Find(0,EFrontlineRole::Lewis,0);
    if(!Target || !Friendly || !Gun){Check(TEXT("explosion_fixture_ready"),false);return Report;}
    Place(Target,1200);Place(Friendly,1200,40);Place(Gun,-8000,6000);
    FFrontlineStrike Strike;Strike.Team=0;Strike.Target=Target->Feet();Strike.Radius=220;Strike.bGrenade=true;
    const float Base=B->ExplosionHitChance(Strike,Target);Place(Target,1310);const float Edge=B->ExplosionHitChance(Strike,Target);Place(Target,1200);
    Check(TEXT("explosion_exposure_reduces_hit_chance_not_survivor_health"),Base>0 && Base<=1 && Edge<Base && FrontlineGrenade::HitChanceFalloff(220,220)==0);
    // Keep only one active target within the actual resolution radius, so the
    // stream seed selects its roll rather than another squad member's roll.
    Place(Friendly,5000,5000);const bool BlastMiss=SeedFor(Base,false);B->ResolveStrike(Strike);
    Check(TEXT("actual_grenade_miss_preserves_health_but_applies_pressure"),BlastMiss && Target->Health==100 && Target->Suppression>0 && Strike.bResolved);
    Strike.bResolved=false;const bool BlastHit=SeedFor(B->ExplosionHitChance(Strike,Target),true);B->ResolveStrike(Strike);
    Check(TEXT("actual_grenade_hit_is_fatal_without_fractional_survivor"),BlastHit && Target->Health==0 && Target->GetDeathState().Kind==EFrontlineDamageKind::Grenade);
    FFrontlineStrike Shell;Shell.Team=0;Shell.Target=Friendly->Feet();Shell.Radius=220;
    const float ShellChance=B->ExplosionHitChance(Shell,Friendly);B->ResolveStrike(Shell);const int32 AfterFriendly=B->Casualties[0];B->ResolveStrike(Shell);
    Check(TEXT("actual_shell_friendly_hit_is_fatal_and_resolution_idempotent"),ShellChance==1.f && Friendly->Health==0 && B->FriendlyFireDamage==100 && AfterFriendly==B->Casualties[0]);
    auto* MortarTarget=Find(1,EFrontlineRole::Rifle,1);if(!MortarTarget){Check(TEXT("mortar_fixture_ready"),false);return Report;}
    Place(MortarTarget,1500);FFrontlineStrike Mortar;Mortar.Team=0;Mortar.bMortar=true;Mortar.Target=MortarTarget->Feet();Mortar.Radius=200;
    const bool MortarHit=SeedFor(B->ExplosionHitChance(Mortar,MortarTarget),true);B->ResolveStrike(Mortar);
    Check(TEXT("actual_mortar_hit_is_fatal"),MortarHit && MortarTarget->Health==0 && B->MortarRoundsResolved==1);

    Place(Gun,B->Trenches[B->GetHomeTrenchIndex()].X,0);auto* Attacker=Find(1,EFrontlineRole::Lewis,1);if(!Attacker){Check(TEXT("protection_fixture_ready"),false);return Report;}
    Place(Attacker,Gun->Feet().X+1500);const float Unfortified=B->BulletHitChance(Attacker,Gun);
    B->ActiveResearchMutable().TechnologyMask|=1u<<7;
    const float Fortified=B->BulletHitChance(Attacker,Gun);
    Check(TEXT("fortification_research_reduces_hit_chance_instead_of_lethal_damage"),Unfortified>0 && FMath::IsNearlyEqual(Fortified,Unfortified*.75f,.00001f));
    Gun->ReceiveHit(.001f,0,1);
    Check(TEXT("fortified_machinegunner_still_dies_to_landed_hit"),Gun->Health==0 && !Gun->IsAlive());
    bool Binary=true;for(const AWarfareFrontlineSoldier* U:B->Units)Binary&=U->Health==0 || U->Health==100;
    Check(TEXT("all_fixture_soldiers_end_alive_or_dead_without_injury_state"),Binary);
    return Report;
}
