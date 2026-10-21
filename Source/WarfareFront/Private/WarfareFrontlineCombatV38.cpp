#include "WarfareFrontlineCombatV38.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineGrenade.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include <limits>

void FrontlineCombatV38::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{if(B)B->RunV38CombatChecks(Check);else Check(TEXT("combat_fixture_exists"),false);}

void AWarfareFrontlineGameMode::RunV38CombatChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!bProgressionVolatile||!FParse::Param(FCommandLine::Get(),TEXT("WFV38Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("combat_requires_isolated_review"),false);return;}
    const auto Runs=CampaignRuns;const auto Active=ActiveCampaignId;const auto Faction=PlayerFaction;
    const auto British=ArmyProgress[0],German=ArmyProgress[1];const auto Rewards=RewardedEncounterIds;
    const bool Ticking=IsActorTickEnabled();SetActorTickEnabled(false);
    ON_SCOPE_EXIT{CampaignRuns=Runs;ActiveCampaignId=Active;PlayerFaction=Faction;ArmyProgress[0]=British;ArmyProgress[1]=German;RewardedEncounterIds=Rewards;ResetEncounter();SetActorTickEnabled(Ticking);};
    ResetEncounter();StartNewCampaign(EFrontlineFaction::British);BeginBattle();
    bool Spawned=true;
    for(int32 Team=0;Team<2;++Team)for(auto FixtureRole:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Marksman})
        Spawned&=SpawnSquad(Team,FixtureRole,GetHomeTrenchIndex(Team));
    for(AWarfareFrontlineSoldier* U:Units){U->SetActorTickEnabled(false);U->bMoving=U->bEngaging=false;U->Suppression=0;U->bFieldEntered=true;U->SetActorLocation(FVector(-9000,5000+U->SquadId*300+U->MemberIndex*30,1288));}
    auto Find=[&](int32 Team,EFrontlineRole FixtureRole,int32 Member=0)->AWarfareFrontlineSoldier*
    {for(AWarfareFrontlineSoldier* U:Units)if(U->Team==Team&&U->Role==FixtureRole&&U->MemberIndex==Member)return U;return nullptr;};
    auto* Rifle=Find(0,EFrontlineRole::Rifle);auto* Enemy=Find(1,EFrontlineRole::Rifle);
    auto* Gun=Find(0,EFrontlineRole::Lewis);auto* Escort=Find(0,EFrontlineRole::Lewis,1);auto* GermanGun=Find(1,EFrontlineRole::Lewis);
    auto* Assault=Find(0,EFrontlineRole::Mills);auto* GermanAssault=Find(1,EFrontlineRole::Mills);
    Check(TEXT("combat_actual_role_fixtures_ready"),Spawned&&Rifle&&Enemy&&Gun&&Escort&&GermanGun&&Assault&&GermanAssault&&Phase==EFrontlinePhase::Fighting);
    if(!Spawned||!Rifle||!Enemy||!Gun||!Escort||!GermanGun||!Assault||!GermanAssault)return;
    auto Place=[](AWarfareFrontlineSoldier* U,float X,float Y=0.f){U->SetActorLocation(FVector(X,Y,1288));};
    auto OpenChance=[&](AWarfareFrontlineSoldier* Shooter,AWarfareFrontlineSoldier* Target){Place(Shooter,1000);Place(Target,2500);const float P=BulletHitChance(Shooter,Target);Place(Shooter,-9000,5000);Place(Target,-9000,7000);return P;};
    const float RifleChance=OpenChance(Rifle,Enemy),EscortChance=OpenChance(Escort,Enemy);
    Check(TEXT("actual_rifle_and_rifle_escort_receive_twenty_percent"),!Escort->IsAutomaticWeapon()&&FMath::IsNearlyEqual(RifleChance,.204f,.00001f)&&FMath::IsNearlyEqual(EscortChance,.204f,.00001f));
    Check(TEXT("actual_british_and_german_guns_receive_thirty_five_percent"),Gun->IsAutomaticWeapon()&&GermanGun->IsAutomaticWeapon()
        &&FMath::IsNearlyEqual(OpenChance(Gun,Enemy),(.74f-1500.f/18000.f)*.072f*1.35f,.00001f)
        &&FMath::IsNearlyEqual(OpenChance(GermanGun,Rifle),(.74f-1500.f/18000.f)*.084f*1.35f,.00001f));
    auto* Marksman=Find(0,EFrontlineRole::Marksman);
    Check(TEXT("marksman_uses_nonautomatic_uplift"),Marksman&&!Marksman->IsAutomaticWeapon()&&FMath::IsNearlyEqual(OpenChance(Marksman,Enemy),(.91f-1500.f/28000.f)*.70f*1.20f,.00001f));
    Place(Rifle,1000);Place(Enemy,4000);const float Far=BulletHitChance(Rifle,Enemy);
    Place(Enemy,2500);Rifle->Suppression=.8f;const float Suppressed=BulletHitChance(Rifle,Enemy);Rifle->Suppression=0;
    const float TrenchX=Trenches[GetHomeTrenchIndex(1)].X;Place(Rifle,TrenchX-1500);Place(Enemy,TrenchX);const float Covered=BulletHitChance(Rifle,Enemy);
    Check(TEXT("cover_suppression_and_distance_still_reduce_accuracy"),Far<RifleChance&&Suppressed<RifleChance&&Covered<RifleChance&&Covered>0);
    Place(Rifle,1000);Place(Enemy,1000+FrontlineWeapons::ProfileFor(Rifle).FireRangeCm*EncounterVisibilityScale()+2);
    Check(TEXT("firearm_uplift_never_extends_rifle_fire_range"),BulletHitChance(Rifle,Enemy)==0);
    Place(Rifle,1000);Place(Enemy,2500);
    bool Seeded=false;for(int32 Seed=1;Seed<100000;++Seed){FRandomStream Probe(Seed);const float Roll=Probe.FRand();if(Roll>=.17f&&Roll<RifleChance){Random.Initialize(Seed);Seeded=true;break;}}
    const int32 Before=ShotsFired;Fire(Rifle,Enemy);
    Check(TEXT("real_fire_newly_successful_round_enters_fatal_hit_path"),Seeded&&Enemy->Health==0&&Enemy->GetDeathState().bStarted&&ShotsFired==Before+1);
    auto* ContactEnemy=Find(1,EFrontlineRole::Rifle,1);Place(Rifle,-9000,5000);Place(Assault,1000);Place(ContactEnemy,2000);
    Check(TEXT("assault_acquires_beyond_previous_contact_limit"),FrontlineWeapons::AdvanceContactRangeFor(Assault)==1155.f&&AcquireTarget(Assault,true)==ContactEnemy);
    Place(ContactEnemy,2250);Check(TEXT("assault_new_contact_limit_remains_bounded"),AcquireTarget(Assault,true)==nullptr);
    Check(TEXT("both_national_assault_profiles_gain_forty_percent_contact"),FrontlineWeapons::ProfileFor(Assault).AdvanceContactCm==1155.f&&FrontlineWeapons::ProfileFor(GermanAssault).AdvanceContactCm==1155.f);
    Place(ContactEnemy,2850);Assault->StartGrenadeThrow(ContactEnemy);Check(TEXT("grenade_outside_new_range_is_rejected"),Assault->GrenadeThrowsStarted==0);
    Place(ContactEnemy,2750);Assault->StartGrenadeThrow(ContactEnemy);
    Check(TEXT("grenade_1750cm_enters_actual_throw_animation"),FrontlineGrenade::MaxRangeCm==1848.f&&Assault->GrenadeThrowsStarted==1&&Assault->WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow&&Assault->GrenadesReleased==0);
    Check(TEXT("grenade_readiness_and_half_precision_policy_preserved"),FrontlineGrenade::ReadinessRateMultiplier==.5f&&FrontlineGrenade::AimScatterMultiplier==2.f&&FrontlineGrenade::TrajectoryReferenceRangeCm==1320.f
        &&FMath::IsNearlyEqual(FrontlineGrenade::FlightSeconds(1320.f),1.9f,.0001f)&&FrontlineGrenade::FlightSeconds(1848.f)>1.9f);
    Check(TEXT("firearm_chance_clamps_and_rejects_invalid_values"),FrontlineWeapons::FirearmHitChance(.99f,true)==1.f&&FrontlineWeapons::FirearmHitChance(-1.f,false)==0.f
        &&FrontlineWeapons::FirearmHitChance(std::numeric_limits<float>::quiet_NaN(),true)==0.f);
    const auto& MG=FrontlineWeapons::ProfileFor(Gun);const auto& DE=FrontlineWeapons::ProfileFor(GermanGun);
    Check(TEXT("national_burst_cadence_and_feed_systems_preserved"),MG.RoundIntervalSeconds==.100f&&DE.RoundIntervalSeconds==.120f&&MG.RecoveryMin==2.6f&&DE.RecoveryMin==3.f&&Gun->GetMagazineCapacity()==47&&GermanGun->GetMagazineCapacity()==100);
}
