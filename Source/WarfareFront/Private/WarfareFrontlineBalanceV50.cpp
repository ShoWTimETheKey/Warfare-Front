#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineGameplayV43.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"

namespace { bool LegacyContactComparisonV835=false,LegacyBalanceComparisonV8447=false; }
// Only the disposable actor review below binds the exact preceding balance.
bool FrontlineUseLegacyBalanceV8447(){return LegacyBalanceComparisonV8447;}
// False throughout normal play. Only the disposable synchronous review below
// can bind the exact previous contact rules, then restores this flag on exit.
bool FrontlineUseLegacyCombatV835(){return LegacyContactComparisonV835;}

struct FFrontlineContactV835Review
{
#include "WarfareFrontlineContactReviewV835.inl"
};
TSharedRef<FJsonObject> RunFrontlineContactV835Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineContactV835Review::Run(B,Check);}

namespace FrontlineBalanceV50
{
float TrenchReloadShelterBlendV8447(const AWarfareFrontlineSoldier* Unit)
{
    if(!Unit||!Unit->IsActiveCombatant()||Unit->bMoving||Unit->WeaponPhase!=EFrontlineWeaponPhase::Reload)return 0.f;
    const float Phase=Unit->GetWeaponReloadNormalizedPhase();
    const float Edge=FMath::Min(FMath::Clamp(Phase/.12f,0.f,1.f),FMath::Clamp((1.f-Phase)/.18f,0.f,1.f));
    return Edge*Edge*(3.f-2.f*Edge);
}
float TargetExposure(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target,float Cover)
{
    const float Base=Cover>0.f?FMath::Clamp(1.f-Cover*(.8f/.75f),.12f,.34f):1.f;
    // Short automatic bursts can pick out the exposed trench lip after the
    // section has closed. Sight and range checks still reject solid banks.
    if(!LegacyBalanceComparisonV8447&&Shooter&&Target&&Cover>0.f&&Shooter->IsStormtrooper())
    {
        const float Close=FMath::Clamp((1600.f-FVector::Dist2D(Shooter->Feet(),Target->Feet()))/600.f,0.f,1.f);
        return FMath::Min(.65f,Base+Cover*.08f*Close);
    }
    // A deliberate shot finds a gunner's exposed head and shoulders. This is
    // not wall penetration: target selection and discharge still require sight.
    const bool Jaeger=Shooter&&Shooter->Role==EFrontlineRole::Marksman&&Shooter->GetFaction()==EFrontlineFaction::German;
    if(!Shooter||!Target||Cover<=0||Shooter->Role!=EFrontlineRole::Marksman||(!Jaeger&&Shooter->MemberIndex!=0)||Shooter->bMoving
        ||Target->bMoving||Shooter->Suppression>=.60f||FVector::DistSquared2D(Shooter->Feet(),Target->Feet())<FMath::Square(1200.f))return Base;
    const float Reveal=Target->IsMachineGunner()?(Jaeger?.36f:.24f):Target->IsSectionLeader()?(Jaeger?.16f:.08f):Jaeger?.10f:0.f;
    return FMath::Min(.65f,Base+Cover*Reveal);
}
float ShooterReadiness(const AWarfareFrontlineSoldier* Shooter)
{
    return FrontlineGameplayV52::CommandAccuracyScale(Shooter);
}
bool HasCoveringFireV8215(const AWarfareFrontlineSoldier* Shooter)
{
    if(!Shooter||!Shooter->Battle||!Shooter->IsActiveCombatant()||!Shooter->IsMachineGunner()||Shooter->bMoving
        ||!Shooter->Battle->HasFactionTechnology(EFrontlineFaction::German,8,Shooter->Team))return false;
    const auto* B=Shooter->Battle.Get();const auto* Station=B->FindSquad(Shooter->SquadId);
    if(!Station||Station->bAdvancing||!B->Trenches.IsValidIndex(Station->Trench)||B->Trenches[Station->Trench].Owner!=Shooter->Team||B->CoverFor(Shooter)<=0)return false;
    for(const AWarfareFrontlineSoldier* Ally:B->Units)if(Ally&&Ally!=Shooter&&Ally->Team==Shooter->Team&&Ally->IsActiveCombatant())
    {
        const auto* Section=B->FindSquad(Ally->SquadId);
        if(Section&&Section->bAdvancing&&!Section->bHoldPosition&&(Ally->Feet().X-Shooter->Feet().X)*B->GetTeamDirection(Shooter->Team)>0
            &&FVector::DistSquared2D(Ally->Feet(),Shooter->Feet())<=FMath::Square(1600.f))return true;
    }
    return false;
}
float CoveringFireHitScaleV8215(const AWarfareFrontlineSoldier* Shooter){return HasCoveringFireV8215(Shooter)?1.25f:1.f;}
float CoveringFireRecoveryScaleV8215(const AWarfareFrontlineSoldier* Shooter){return HasCoveringFireV8215(Shooter)?.65f:1.f;}
float OrderInterval(bool Officer){return Officer?6.f:8.f;}
float OrderRange(bool Officer,bool German){return Officer?(German?2100.f:2300.f):1900.f;}
float OrderRelief(bool Officer,bool German){return Officer?(German?.30f:.34f):.24f;}
}

struct FFrontlineBalanceV50Review
{
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("v50_balance_requires_isolated_profile"),false);return;}
        const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedPhase=B->Phase;const auto SavedSmoke=B->SmokeZones;
        const int32 SavedOutro=B->BattleOutroWinner;
        const auto SavedResearch=B->ActiveResearchMutable();const float SavedClock=B->Elapsed;
        const int32 OldOrders0=B->LeaderOrders[0],OldOrders1=B->LeaderOrders[1];
        const float OldRelief0=B->LeaderSuppressionRemoved[0],OldRelief1=B->LeaderSuppressionRemoved[1];
        TArray<AWarfareFrontlineSoldier*> Created;B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Elapsed=100;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        B->ActiveResearchMutable().TechnologyMask=0;
        ON_SCOPE_EXIT
        {
            for(auto* U:Created)if(IsValid(U))U->Destroy();
            B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedSmoke;B->Phase=SavedPhase;B->BattleOutroWinner=SavedOutro;B->Elapsed=SavedClock;B->ActiveResearchMutable()=SavedResearch;
            B->LeaderOrders[0]=OldOrders0;B->LeaderOrders[1]=OldOrders1;
            B->LeaderSuppressionRemoved[0]=OldRelief0;B->LeaderSuppressionRemoved[1]=OldRelief1;
        };
        auto Make=[&](int32 Team,EFrontlineRole Role,int32 Id,FVector Position)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,Id,Team,Role,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->Health=100;
            U->Suppression=0;U->bMoving=false;U->LastSectionOrderAt=-100;U->CommandReadyAt=0;U->SetActorLocation(Position+FVector(0,0,88));
            B->Units.Add(U);Created.Add(U);FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=Role;S.Trench=INDEX_NONE;S.bAdvancing=true;S.Members.Add(U);B->Squads.Add(S);return U;
        };
        auto* NCO=Make(0,EFrontlineRole::Officer,950001,FVector(0,0,10000));
        auto* Rifle=Make(0,EFrontlineRole::Rifle,950002,FVector(1000,0,10000));
        auto* Marksman=Make(0,EFrontlineRole::Marksman,950003,FVector(-1800,0,10000));
        auto* Gun=Make(1,EFrontlineRole::Lewis,950004,FVector(1800,0,10000));
        auto* Enemy=Make(1,EFrontlineRole::Rifle,950005,FVector(100,0,10000));
        Check(TEXT("v50_specialist_real_actors_ready"),NCO&&Rifle&&Marksman&&Gun&&Enemy);if(!NCO||!Rifle||!Marksman||!Gun||!Enemy)return;
        using namespace FrontlineBalanceV50;
        const float CoveredRifle=TargetExposure(Rifle,Gun,.75f),CoveredMarksman=TargetExposure(Marksman,Gun,.75f);
        Check(TEXT("v50_marksman_has_visible_stationary_gunner_counter_without_ignoring_cover"),CoveredMarksman>CoveredRifle*1.8f&&CoveredMarksman<.5f);
        Marksman->bMoving=true;const bool Moving=TargetExposure(Marksman,Gun,.75f)==CoveredRifle;Marksman->bMoving=false;
        Marksman->Suppression=.61f;const bool Pinned=TargetExposure(Marksman,Gun,.75f)==CoveredRifle;Marksman->Suppression=0;
        Gun->bMoving=true;const bool MovingTarget=TargetExposure(Marksman,Gun,.75f)==CoveredRifle;Gun->bMoving=false;
        Check(TEXT("v50_marksman_advantage_requires_still_clear_aim_and_never_buffs_open_targets"),Moving&&Pinned&&MovingTarget&&TargetExposure(Marksman,Gun,0)==1.f);
        const FVector GunBefore=Gun->GetActorLocation(),MarksmanBefore=Marksman->GetActorLocation(),EnemyBefore=Enemy->GetActorLocation();
        auto* GunSection=B->FindSquad(Gun->SquadId);const int32 EnemyHome=B->GetHomeTrenchIndex(1);
        const int32 OldOwner=B->Trenches[EnemyHome].Owner;B->Trenches[EnemyHome].Owner=1;GunSection->Trench=EnemyHome;
        Gun->SetActorLocation(FVector(B->Trenches[EnemyHome].X,0,10088));
        const int32 Dir=B->GetTeamDirection(0);Marksman->SetActorLocation(Gun->GetActorLocation()-FVector(Dir*2200,0,0));
        Enemy->SetActorLocation(Marksman->GetActorLocation()+FVector(Dir*1500,0,0));
        const float Cover=B->CoverFor(Gun),Exposure=TargetExposure(Marksman,Gun,Cover);
        const auto& P=FrontlineWeapons::ProfileFor(Marksman);
        const float OldChance=FrontlineWeapons::FirearmHitChance((P.AccuracyNear-2200.f/P.AccuracyFalloffCm)*P.HitProbabilityScale
            *FMath::Clamp(1.f-Cover*(.8f/.75f),.12f,.34f)*B->IncomingBulletHitChanceScale(Gun),false);
        Check(TEXT("v50_actual_marksman_target_and_bullet_paths_counter_gun_in_cover"),Cover>0&&Exposure<.65f&&B->AcquireTarget(Marksman)==Gun&&B->BulletHitChance(Marksman,Gun)>OldChance*1.5f);
        GunSection->Trench=INDEX_NONE;B->Trenches[EnemyHome].Owner=OldOwner;
        Gun->SetActorLocation(GunBefore);Marksman->SetActorLocation(MarksmanBefore);Enemy->SetActorLocation(EnemyBefore);
        Rifle->Suppression=.60f;Enemy->Suppression=.60f;const float HP=Rifle->Health;
        B->UpdateSectionOrders();const float Relieved=Rifle->Suppression;const int32 Orders=B->LeaderOrders[0];
        Check(TEXT("v50_nco_rallies_neighboring_section_in_open_field"),FMath::IsNearlyEqual(Relieved,.36f,.00001f)&&Rifle->Health==HP&&Enemy->Suppression==.6f&&Orders==OldOrders0+1);
        Check(TEXT("v50_actual_received_order_gives_six_second_steady_aim"),ShooterReadiness(Rifle)==1.10f&&ShooterReadiness(Enemy)==1.f);
        const float Commanded=B->BulletHitChance(Rifle,Enemy);Rifle->LastSectionOrderAt=-100;
        const float Uncommanded=B->BulletHitChance(Rifle,Enemy);Rifle->LastSectionOrderAt=B->Elapsed;
        Check(TEXT("v50_actual_rallied_fire_uses_the_bounded_aim_bonus"),Uncommanded>0&&FMath::IsNearlyEqual(Commanded/Uncommanded,1.10f,.0001f));
        NCO->CommandReadyAt=0;B->UpdateSectionOrders();
        Check(TEXT("v50_orders_cannot_stack_to_erase_suppression"),Rifle->Suppression==Relieved&&B->LeaderOrders[0]==Orders);
        B->Elapsed+=6.01f;Check(TEXT("v50_order_aim_bonus_expires"),ShooterReadiness(Rifle)==1.f);
        B->Elapsed+=2.1f;NCO->Health=0;B->UpdateSectionOrders();
        Check(TEXT("v50_dead_leader_grants_no_orders"),Rifle->Suppression==Relieved&&B->LeaderOrders[0]==Orders);
        const int32 German=B->GetFactionForTeam(0)==EFrontlineFaction::German?0:1,British=1-German;
        const float G=B->EffectiveRecruitSeconds(EFrontlineRole::Rifle,German)/FrontlineGameplayV43::PreparationSeconds(1.f,German,B->GetCampaignDifficulty());
        const float UK=B->EffectiveRecruitSeconds(EFrontlineRole::Rifle,British)/FrontlineGameplayV43::PreparationSeconds(1.f,British,B->GetCampaignDifficulty());
        Check(TEXT("v59_equal_rifle_sections_have_equal_readiness_before_explicit_difficulty"),G>0&&UK>0&&FMath::IsNearlyEqual(G,UK,.001f)
            &&B->GetSquadSize(EFrontlineRole::Rifle,German)==B->GetSquadSize(EFrontlineRole::Rifle,British));
        auto ManpowerRate=[&](EFrontlineRole Role){return B->RoleSize(Role,0)/B->RoleRecruitSeconds(Role);};
        Check(TEXT("v59_specialists_retain_lower_manpower_throughput_and_distinct_roles"),
            B->RoleRecruitSeconds(EFrontlineRole::Marksman)<B->RoleRecruitSeconds(EFrontlineRole::Lewis)
            &&ManpowerRate(EFrontlineRole::Officer)<ManpowerRate(EFrontlineRole::Rifle)
            &&ManpowerRate(EFrontlineRole::Pioneer)<ManpowerRate(EFrontlineRole::Rifle)
            &&B->RoleSize(EFrontlineRole::Pioneer,0)==4&&B->RoleSize(EFrontlineRole::Pioneer,1)==4);
        Check(TEXT("v50_officer_upgrades_preserve_national_roles_and_bounded_order_rates"),OrderRange(true,false)>OrderRange(true,true)&&OrderInterval(true)<OrderInterval(false)&&OrderRelief(true,false)>OrderRelief(true,true)&&OrderRelief(false,false)==OrderRelief(false,true));
    }
};
void FrontlineBalanceV50::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){FFrontlineBalanceV50Review::Run(B,Check);}

struct FFrontlineBalanceV8447Review
{
#include "WarfareFrontlineBalanceV8447Review.inl"
};
TSharedRef<FJsonObject> RunFrontlineBalanceV8447Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile())return FFrontlineBalanceV8447Review::Run(B,Check);
    auto Result=MakeShared<FJsonObject>();
    FrontlineAchievementsV76::WithScopedCombatEventsV839(B,[&]
    {FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]
    {FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]
    {Result=FFrontlineBalanceV8447Review::Run(B,Check);});});});
    return Result;
}
