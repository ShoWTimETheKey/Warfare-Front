#include "WarfareFrontlineBalanceV46.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

void FrontlineBalanceV46::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV46Review"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("v46_balance_requires_isolated_volatile_profile"),false);return;}

    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedStrikes=B->Strikes;
    const auto SavedTrenches=B->Trenches;const auto SavedSmoke=B->SmokeZones;const auto SavedPhase=B->Phase;
    const int32 SavedOutro=B->BattleOutroWinner,SavedShots=B->ShotsFired,SavedGrenades=B->GrenadesThrown;
    const FString SavedMessage=B->LastMessage;TArray<AWarfareFrontlineSoldier*> Created;
    B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    ON_SCOPE_EXIT
    {
        for(auto& S:B->Strikes)if(S.Projectile.IsValid())S.Projectile->Destroy();
        for(auto* U:Created)if(IsValid(U))U->Destroy();
        B->Units=SavedUnits;B->Squads=SavedSquads;B->Strikes=SavedStrikes;B->SmokeZones=SavedSmoke;B->Trenches=SavedTrenches;
        B->Phase=SavedPhase;B->BattleOutroWinner=SavedOutro;B->ShotsFired=SavedShots;B->GrenadesThrown=SavedGrenades;B->LastMessage=SavedMessage;
    };
    auto Make=[&](int32 Team,EFrontlineRole Role,int32 Member,int32 SquadId)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        Created.Add(U);U->Initialize(B,SquadId,Team,Role,Member);U->SetActorTickEnabled(false);
        U->bFieldEntered=true;U->Health=100;U->Suppression=0;U->bMoving=false;
        U->SetActorLocation(FVector(50000+Created.Num()*5000,0,12000));B->Units.Add(U);return U;
    };
    AWarfareFrontlineSoldier* Gunners[2]={};AWarfareFrontlineSoldier* Escorts[2]={};
    for(int32 Team=0;Team<2;++Team)
    {
        Gunners[Team]=Make(Team,EFrontlineRole::Lewis,0,74600+Team);Escorts[Team]=Make(Team,EFrontlineRole::Lewis,1,74600+Team);
        FFrontlineSquad S;S.Id=74600+Team;S.Team=Team;S.Role=EFrontlineRole::Lewis;S.Trench=INDEX_NONE;S.Members={Gunners[Team],Escorts[Team]};B->Squads.Add(S);
    }
    const bool CrewReady=Gunners[0]&&Gunners[1]&&Escorts[0]&&Escorts[1];Check(TEXT("v46_real_british_and_german_gun_crews_available"),CrewReady);if(!CrewReady)return;
    bool Prep=true;
    for(int32 Team=0;Team<2;++Team)
        Prep&=FMath::IsNearlyEqual(B->EffectiveRecruitSeconds(EFrontlineRole::RifleGrenadier,Team)/B->EffectiveRecruitSeconds(EFrontlineRole::AntiTankRifle,Team),(27.f/1.30f)/(32.f*.70f),.00001f);
    Check(TEXT("v46_rifle_grenadier_preparation_rate_is_130_percent_after_shared_modifiers"),Prep
        &&FMath::IsNearlyEqual(B->RoleRecruitSeconds(EFrontlineRole::RifleGrenadier),15.351170f,.0001f)
        &&FMath::IsNearlyEqual(B->RoleRecruitSeconds(EFrontlineRole::AntiTankRifle),32.f*.70f*FrontlinePacingV24::BaseRecruitScale,.0001f));

    for(int32 Team=0;Team<2;++Team)
    {
        auto* Gun=Gunners[Team];auto* Escort=Escorts[Team];auto* Target=Escorts[1-Team];
        const bool British=Gun->GetFaction()==EFrontlineFaction::British;const FString Prefix=British?TEXT("v46_lewis_"):TEXT("v46_mg0815_");
        const auto& P=FrontlineWeapons::ProfileFor(Gun);const auto& R=FrontlineWeapons::ProfileFor(Escort);
        Check(Prefix+TEXT("range_hit_weight_and_rifle_escort_are_correct"),
            FMath::IsNearlyEqual(P.FireRangeCm,British?4632.5f:4930.f,.01f)&&FMath::IsNearlyEqual(P.AdvanceContactCm,British?3995.f:4122.5f,.01f)
            &&FMath::IsNearlyEqual(P.HitProbabilityScale,British?.0828f:.0966f,.000001f)
            &&FMath::IsNearlyEqual(R.FireRangeCm,British?4100.f:4300.f,.01f)&&R.HitProbabilityScale==.25f&&!Escort->IsAutomaticWeapon());
        Check(Prefix+TEXT("national_burst_feed_and_companion_contact_preserved"),Gun->GetMagazineCapacity()==(British?47:100)
            &&P.BurstMin==(British?8:10)&&P.BurstMax==(British?11:14)&&FMath::IsNearlyEqual(P.RoundIntervalSeconds,British?.100f:.120f)
            &&FMath::IsNearlyEqual(FrontlineWeapons::AdvanceContactRangeFor(Escort),P.AdvanceContactCm));
        for(int32 I=0;I<Created.Num();++I)Created[I]->SetActorLocation(FVector(50000+I*6000,0,12000));
        const int32 Dir=B->GetTeamDirection(Team);Gun->SetActorLocation(FVector(0,0,12000));
        Target->SetActorLocation(FVector(Dir*1000,0,12000));
        const float OldWeight=British?.072f:.084f;
        const float Baseline=FrontlineWeapons::FirearmHitChance((.74f-1000.f/18000.f)*OldWeight*B->DirectFireMultiplier(Gun->Feet(),Target->Feet())*B->IncomingBulletHitChanceScale(Target),true);
        Check(Prefix+TEXT("actual_bullet_hit_probability_increases_by_fifteen_percent"),Baseline>0&&FMath::IsNearlyEqual(B->BulletHitChance(Gun,Target)/Baseline,1.15f,.0001f));
        const float Effective=P.FireRangeCm*B->EncounterVisibilityScale();Target->SetActorLocation(FVector(Dir*(Effective-20.f),0,12000));
        const bool Inside=B->AcquireTarget(Gun)==Target&&B->BulletHitChance(Gun,Target)>0;
        Target->SetActorLocation(FVector(Dir*(Effective+20.f),0,12000));
        Check(Prefix+TEXT("actual_target_and_hit_paths_obey_reduced_weather_scaled_range"),Inside&&B->AcquireTarget(Gun)==nullptr&&B->BulletHitChance(Gun,Target)==0);
    }

    bool Spread=true;
    for(float Range:{0.f,1500.f,3000.f})for(float Suppression:{0.f,.8f})for(bool Trained:{false,true})
    {
        const float Previous=(90.f+Range*.050f+Suppression*170.f)*(Trained?.85f:1.f);
        Spread&=FMath::IsNearlyEqual(FrontlineSpecialistsV41::RifleGrenadeScatterRadius(Range,Suppression,Trained)/Previous,1.20f,.00001f);
    }
    Check(TEXT("v46_rifle_grenade_scatter_radius_is_twenty_percent_wider_at_range_and_under_suppression"),Spread);
    const int32 BritishTeam=B->GetFactionForTeam(0)==EFrontlineFaction::British?0:1;
    auto* Grenadier=Make(BritishTeam,EFrontlineRole::RifleGrenadier,0,74620);if(!Grenadier){Check(TEXT("v46_rifle_grenadier_fixture_exists"),false);return;}
    FFrontlineSquad Specialist;Specialist.Id=74620;Specialist.Team=BritishTeam;Specialist.Role=EFrontlineRole::RifleGrenadier;Specialist.Trench=INDEX_NONE;Specialist.Members={Grenadier};B->Squads.Add(Specialist);
    Check(TEXT("v46_roster_uplift_does_not_accelerate_single_round_reload"),FMath::IsNearlyEqual(FrontlineSpecialistsV41::ReloadSeconds(Grenadier),7.f)&&Grenadier->GetMagazineCapacity()==1);
    auto* GrenadeTarget=Escorts[1-BritishTeam];const int32 GrenadeDir=B->GetTeamDirection(BritishTeam);
    Grenadier->SetActorLocation(FVector(0,0,12000));GrenadeTarget->SetActorLocation(FVector(GrenadeDir*1200,0,12000));Grenadier->Suppression=.4f;
    FRandomStream Shot(460917),Expected(460917);const float Angle=Expected.FRandRange(0,2*PI);
    const float Radius=FMath::Sqrt(Expected.FRand())*FrontlineSpecialistsV41::RifleGrenadeScatterRadius(1200,.4f,B->HasTechnology(24,BritishTeam));
    const FVector Planned=GrenadeTarget->Feet()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius*.85f,0);
    const bool Fired=FrontlineSpecialistsV41::Fire(Grenadier,GrenadeTarget,nullptr,Shot);
    Check(TEXT("v46_actual_rifle_grenade_commits_the_wider_seeded_scatter"),Fired&&B->Strikes.Num()==1&&B->Strikes[0].bRifleLaunched&&FVector::DistSquared2D(B->Strikes[0].Target,Planned)<.01f);

    const float OldEnd=10856.f*FrontlineBattlefield::OperationDepthScale;
    Check(TEXT("v46_authoritative_deeper_goal_remains_supported_and_on_authored_ground"),FMath::IsNearlyEqual(FrontlineBattlefield::GoalHalfLength,OldEnd+.20f*FrontlineBattlefield::TrenchSpacing,.02f)
        &&FrontlineBattlefield::GoalHalfLength>OldEnd&&FrontlineBattlefield::SupportHalfLength>FrontlineBattlefield::GoalHalfLength
        &&FrontlineBattlefield::GoalHalfLength+300.f<FrontlineBattlefield::GroundHalfLength);
    for(int32 Team=0;Team<2;++Team)
    {
        auto& S=B->Squads[Team];const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(1-Team);
        for(auto& T:B->Trenches)T.Owner=Team;
        S.bAdvancing=true;S.Trench=INDEX_NONE;S.Destination=B->GetGoalTrenchIndex(Team);S.LastPassedTrench=Home;
        auto Put=[&](float X){for(const auto& W:S.Members)if(auto* U=W.Get()){U->Health=100;U->bFieldEntered=true;U->SetActorLocation(FVector(Dir*X,U->Feet().Y,12000));}};
        Put(OldEnd+1.f);const bool OldDoesNotWin=!FrontlineGameplayV43::HasCrossedEnemyEnd(B,S);
        Put(FrontlineBattlefield::GoalHalfLength-1.f);const bool InsideDoesNotWin=!FrontlineGameplayV43::HasCrossedEnemyEnd(B,S);
        Put(FrontlineBattlefield::GoalHalfLength+1.f);const bool PassedWins=FrontlineGameplayV43::HasCrossedEnemyEnd(B,S);
        S.Destination=Home;const bool MisroutedDoesNotWin=!FrontlineGameplayV43::HasCrossedEnemyEnd(B,S);S.Destination=B->GetGoalTrenchIndex(Team);
        Gunners[Team]->bFieldEntered=false;const bool ReserveDoesNotWin=!FrontlineGameplayV43::HasCrossedEnemyEnd(B,S);Gunners[Team]->bFieldEntered=true;
        Put(FrontlineBattlefield::GoalHalfLength+20.f);Escorts[Team]->SetActorLocation(FVector(Dir*(FrontlineBattlefield::GoalHalfLength-50.f),0,12000));
        const bool CentreStillRequired=!FrontlineGameplayV43::HasCrossedEnemyEnd(B,S);
        const float Destination=B->SoldierDestination(Gunners[Team]).X*Dir;
        Check(FString::Printf(TEXT("v46_team_%d_only_active_goal_bound_section_crossing_deep_end_wins"),Team),OldDoesNotWin&&InsideDoesNotWin&&PassedWins&&MisroutedDoesNotWin&&ReserveDoesNotWin&&CentreStillRequired&&Destination>FrontlineBattlefield::GoalHalfLength);
    }
}
