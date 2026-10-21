#include "WarfareFrontlineChoiceTargetsV78.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "Components/BoxComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

namespace FrontlineChoiceTargetsV78
{
namespace
{
struct FScore {TWeakObjectPtr<AWarfareFrontlineSoldier> Target;float Value=0;FVector Position=FVector::ZeroVector;bool Moving=false;};
struct FChoice
{
    FGuid Encounter;float At=-1000,Range=0,Suppression=0;uint64 Frame=0;int32 Team=0,Member=0;EFrontlineRole Role=EFrontlineRole::Rifle;
    FName Weapon;bool ImprovedScatter=false;
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;FVector Position=FVector::ZeroVector;
    TWeakObjectPtr<AWarfareFrontlineSoldier> Selected;TArray<FScore,TInlineAllocator<8>> Scores;
};
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FChoice> LastChoices;
float UrgentRange(const AWarfareFrontlineSoldier*){return 900.f;}
float FireRange(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U)
{return FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();}
}
void Reset(const AWarfareFrontlineGameMode* Battle)
{
    for(auto It=LastChoices.CreateIterator();It;++It)if(!It.Key().IsValid()||!It.Value().Battle.IsValid()||It.Value().Battle==Battle)It.RemoveCurrent();
}
bool IsAreaOperator(const AWarfareFrontlineSoldier* U)
{return FrontlineSpecialistsV41::IsRifleGrenadier(U);}
EFrontlineRole PrecisionCounter(EFrontlineFaction Faction)
{return Faction==EFrontlineFaction::British?EFrontlineRole::RifleGrenadier:EFrontlineRole::Mills;}

AWarfareFrontlineSoldier* Pick(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U,AWarfareFrontlineSoldier* Nearest)
{
    if(!B||!U||!Nearest||!IsAreaOperator(U)||!U->IsActiveCombatant())return Nearest;
    const float Range=FireRange(B,U);
    struct FContact{AWarfareFrontlineSoldier* Unit;float Distance;};
    constexpr int32 ContactLimit=8;
    TArray<FContact,TInlineAllocator<ContactLimit>> Nearby,Visible;
    // Candidate storage and subsequent visibility traces are capped. The ordinary
    // acquisition above already supplies a valid fallback if nearer men are hidden.
    for(AWarfareFrontlineSoldier* Other:B->Units)if(Other&&Other->Team!=U->Team&&Other->IsActiveCombatant())
    {
        const float Distance=FVector::Dist2D(U->Feet(),Other->Feet());if(FrontlineWeapons::DirectFireRangeDistanceCm(U,Other->Feet())>Range)continue;
        int32 At=0;while(At<Nearby.Num()&&Nearby[At].Distance<=Distance)++At;
        if(At>=ContactLimit)continue;
        if(Nearby.Num()==ContactLimit)Nearby.RemoveAt(ContactLimit-1);
        Nearby.Insert({Other,Distance},At);
    }
    for(const auto& C:Nearby)if(B->DirectFireMultiplier(U->Feet(),C.Unit->Feet())>=.95f&&B->HasSight(U,C.Unit))Visible.Add(C);
    for(const auto& C:Visible)if(C.Distance<=UrgentRange(U))return C.Unit;
    auto Benefit=[&](const FContact& Aim)
    {
        const float Risk=FrontlineGrenade::BlastRadiusCm+FrontlineSpecialistsV41::RifleGrenadeScatterRadius(Aim.Distance,U->Suppression,B->HasTechnology(24,U->Team));
        for(AWarfareFrontlineSoldier* Friendly:B->Units)if(Friendly&&Friendly->Team==U->Team&&Friendly->IsActiveCombatant()
            &&FVector::DistSquared2D(Friendly->Feet(),Aim.Unit->Feet())<FMath::Square(Risk))return 0.f;
        float Value=0;
        for(const auto& C:Visible)if(C.Unit!=Aim.Unit)
            Value+=FMath::Max(0.f,1.f-FVector::Dist2D(C.Unit->Feet(),Aim.Unit->Feet())/FrontlineGrenade::BlastRadiusCm);
        return FMath::Min(750.f,Value*450.f);
    };
    AWarfareFrontlineSoldier* Best=Nearest;
    float BestScore=FVector::Dist2D(U->Feet(),Nearest->Feet())+(Nearest->bMoving?-250.f:0.f);
    for(auto It=LastChoices.CreateIterator();It;++It)
    {
        const auto& V=It.Value();const auto* Owner=It.Key().Get();const auto* Battle=V.Battle.Get();
        if(!Owner||!Owner->IsActiveCombatant()||!Battle||V.Encounter!=Battle->GetEncounterId()||V.At>Battle->Elapsed||Battle->Elapsed-V.At>.5f)It.RemoveCurrent();
    }
    if(LastChoices.Num()>=128)LastChoices.Empty();
    auto& Choice=LastChoices.FindOrAdd(const_cast<AWarfareFrontlineSoldier*>(U));Choice.Encounter=B->GetEncounterId();Choice.At=B->Elapsed;Choice.Scores.Empty();
    Choice.Battle=const_cast<AWarfareFrontlineGameMode*>(B);Choice.Frame=GFrameCounter;Choice.Range=Range;Choice.Position=U->Feet();
    Choice.Team=U->Team;Choice.Member=U->MemberIndex;Choice.Role=U->Role;Choice.Weapon=FName(FrontlineWeapons::ProfileFor(U).WeaponId);
    Choice.Suppression=U->Suppression;Choice.ImprovedScatter=B->HasTechnology(24,U->Team);
    for(const auto& C:Visible)
    {const float Score=C.Distance+(C.Unit->bMoving?-250.f:0.f)-Benefit(C);Choice.Scores.Add({C.Unit,Score,C.Unit->Feet(),C.Unit->bMoving});if(C.Unit==Nearest)BestScore=Score;}
    for(const auto& C:Choice.Scores)
    {
        if(C.Value<BestScore-180.f){Best=C.Target.Get();BestScore=C.Value;}
    }
    Choice.Selected=Best;
    return Best;
}
bool ShouldRetarget(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U,AWarfareFrontlineSoldier* Candidate,AWarfareFrontlineSoldier* Current)
{
    if(!B||!U||!Candidate||!Current||Candidate==Current||!IsAreaOperator(U)||!U->IsActiveCombatant()||!Candidate->IsActiveCombatant()||!Current->IsActiveCombatant()
        ||U->Battle!=B||Candidate->Battle!=B||Current->Battle!=B||Candidate->Team==U->Team||Current->Team==U->Team)return false;
    if(FVector::Dist2D(U->Feet(),Candidate->Feet())<=UrgentRange(U))return B->DirectFireMultiplier(U->Feet(),Candidate->Feet())>=.95f&&B->HasSight(U,Candidate);
    if(FVector::Dist2D(U->Feet(),Current->Feet())<=UrgentRange(U))return false;
    // Reuse this acquisition's eight scores instead of tracing the same men again.
    const auto* Choice=LastChoices.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!Choice||Choice->Encounter!=B->GetEncounterId()||Choice->Battle!=B||Choice->Selected!=Candidate||Choice->Frame!=GFrameCounter
        ||!FMath::IsNearlyEqual(Choice->At,B->Elapsed,.00001f)||Choice->Team!=U->Team||Choice->Member!=U->MemberIndex||Choice->Role!=U->Role
        ||Choice->Weapon!=FName(FrontlineWeapons::ProfileFor(U).WeaponId)||Choice->ImprovedScatter!=B->HasTechnology(24,U->Team)
        ||!FMath::IsNearlyEqual(Choice->Suppression,U->Suppression,.00001f)||!FMath::IsNearlyEqual(Choice->Range,FireRange(B,U),.01f)||!Choice->Position.Equals(U->Feet(),.01f))return false;
    for(const auto& Score:Choice->Scores)
    {
        const auto* Contact=Score.Target.Get();
        if(!Contact||!Contact->IsActiveCombatant()||Contact->Battle!=B||Contact->Team==U->Team||Contact->bMoving!=Score.Moving||!Score.Position.Equals(Contact->Feet(),.01f))return false;
    }
    if(FrontlineSpecialistsV41::IsRifleGrenadier(U))
    {
        const float Risk=FrontlineGrenade::BlastRadiusCm+FrontlineSpecialistsV41::RifleGrenadeScatterRadius(FVector::Dist2D(U->Feet(),Candidate->Feet()),U->Suppression,B->HasTechnology(24,U->Team));
        for(AWarfareFrontlineSoldier* Friendly:B->Units)if(Friendly&&Friendly->Team==U->Team&&Friendly->IsActiveCombatant()
            &&FVector::DistSquared2D(Friendly->Feet(),Candidate->Feet())<FMath::Square(Risk))return false;
    }
    const auto* Wanted=Choice->Scores.FindByPredicate([Candidate](const FScore& S){return S.Target==Candidate;});
    const auto* Prior=Choice->Scores.FindByPredicate([Current](const FScore& S){return S.Target==Current;});
    return Wanted&&Prior&&Wanted->Position.Equals(Candidate->Feet(),.01f)&&Prior->Position.Equals(Current->Feet(),.01f)
        &&Wanted->Value<Prior->Value-180.f&&B->DirectFireMultiplier(U->Feet(),Candidate->Feet())>=.95f&&B->HasSight(U,Candidate);
}
}

struct FFrontlineChoiceTargetsV78Review
{
    static void CounterChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        const auto OldRuns=B->CampaignRuns;const auto OldFaction=B->PlayerFaction;const auto OldId=B->ActiveCampaignId;
        const auto OldEncounter=B->EncounterId,OldBoundId=B->BoundCampaignId,OldBoundEncounter=B->BoundCampaignEncounterId;
        const int32 OldMission=B->BoundCampaignMission,OldStage=B->BoundAttachmentStageV53;
        const float OldRecruit=B->EnemyRecruitAt,OldAssault=B->EnemyAssaultAt,OldAssess=B->EnemyFireAssessAt,OldClock=B->Elapsed;
        const int32 OldStep=B->EnemyRecruitStep,OldCounter=B->EnemyCounterattackTrench,OldCommands=B->RecruitCommands;
        const int32 OldNextId=B->NextSquadId;const bool OldAdvanced=B->bCampaignMissionAdvanced;
        const float OldMorale[]={B->Morale[0],B->Morale[1]};
        const auto OldRole=B->EnemyPlannedRole;const bool OldCommitted=B->bEnemyRoleCommitted,OldPending=B->bEnemyFirePending;
        const auto OldRandom=B->EnemyDecisionRandom,OldCombatRandom=B->Random;const auto OldState=FrontlineDifficultyV30::State(B);
        const int32 OldWave=B->EnemyWaveRecruitCount;const float OldWaveAt=B->EnemyWaveStartedAt;
        float OldReady[2][AWarfareFrontlineGameMode::RoleCount];
        for(int32 T=0;T<2;++T)for(int32 R=0;R<AWarfareFrontlineGameMode::RoleCount;++R)OldReady[T][R]=B->RecruitReadyAt[T][R];
        ON_SCOPE_EXIT
        {
            for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){U->SetActorEnableCollision(false);U->Destroy();}B->Units.Empty();B->Squads.Empty();
            B->CampaignRuns=OldRuns;B->PlayerFaction=OldFaction;B->ActiveCampaignId=OldId;B->EncounterId=OldEncounter;
            B->BoundCampaignId=OldBoundId;B->BoundCampaignEncounterId=OldBoundEncounter;B->BoundCampaignMission=OldMission;B->BoundAttachmentStageV53=OldStage;
            B->EnemyRecruitAt=OldRecruit;B->EnemyAssaultAt=OldAssault;B->EnemyFireAssessAt=OldAssess;B->Elapsed=OldClock;
            B->EnemyRecruitStep=OldStep;B->EnemyCounterattackTrench=OldCounter;B->EnemyPlannedRole=OldRole;
            B->bEnemyRoleCommitted=OldCommitted;B->bEnemyFirePending=OldPending;B->EnemyDecisionRandom=OldRandom;B->Random=OldCombatRandom;
            B->RecruitCommands=OldCommands;B->EnemyWaveRecruitCount=OldWave;B->EnemyWaveStartedAt=OldWaveAt;
            B->NextSquadId=OldNextId;B->bCampaignMissionAdvanced=OldAdvanced;B->Morale[0]=OldMorale[0];B->Morale[1]=OldMorale[1];
            FrontlineDifficultyV30::State(B)=OldState;
            for(int32 T=0;T<2;++T)for(int32 R=0;R<AWarfareFrontlineGameMode::RoleCount;++R)B->RecruitReadyAt[T][R]=OldReady[T][R];
        };
        for(int32 Case=0;Case<3;++Case)
        {
            for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){U->SetActorEnableCollision(false);U->Destroy();}B->Units.Empty();B->Squads.Empty();
            FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=Case==1?EFrontlineFaction::German:EFrontlineFaction::British;
            Run.Difficulty=EFrontlineDifficulty::Survivor;Run.SelectedMission=Run.CompletedMissions=16;
            B->CampaignRuns={Run};B->PlayerFaction=Run.Faction;B->ActiveCampaignId=Run.Id;B->EncounterId=FGuid::NewGuid();
            B->BindCampaignEncounter();FrontlineDifficultyV30::Reset(B);B->Elapsed=100;
            B->Morale[0]=B->Morale[1]=100;
            B->EnemyRecruitAt=0;B->EnemyAssaultAt=B->EnemyFireAssessAt=MAX_flt;B->EnemyCounterattackTrench=INDEX_NONE;
            B->bEnemyRoleCommitted=false;B->bEnemyFirePending=false;B->EnemyWaveRecruitCount=0;
            const auto* Mission=B->GetEncounterCampaignMission();
            if(!Mission||Mission->EnemyRoleOrder.IsEmpty()){Check(TEXT("v78_counter_fixture_has_authored_opposition"),false);return;}
            B->EnemyRecruitStep=Mission->EnemyRoleOrder.Num()*10;
            for(int32 R=0;R<AWarfareFrontlineGameMode::RoleCount;++R)B->RecruitReadyAt[1][R]=0;
            auto Make=[&](int32 Team,EFrontlineRole Role,int32 Id,const FVector& P,int32 Member)
            {
                auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
                U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;
                U->SetActorLocation(P+FVector(0,0,88));B->Units.Add(U);
                FFrontlineSquad S;S.Team=Team;S.Role=Role;S.Id=Id;S.Members.Add(U);B->Squads.Add(S);return U;
            };
            const int32 Dir=B->GetTeamDirection(1);const FVector Origin(0,0,10000);
            auto* Gun=Make(1,EFrontlineRole::Lewis,781000+Case*10,Origin,0);
            auto* Lead=Make(0,EFrontlineRole::Marksman,781001+Case*10,Origin+FVector(Dir*2000,-60,0),0);
            auto* Escort=Make(0,EFrontlineRole::Marksman,781002+Case*10,Origin+FVector(Dir*2000,60,0),1);
            Check(FString::Printf(TEXT("v78_counter_real_actors_case%d"),Case),Gun&&Lead&&Escort);if(!Gun||!Lead||!Escort)return;
            Check(FString::Printf(TEXT("v78_counter_fixture_initial_physical_sight_is_clear_case%d"),Case),B->HasSight(Gun,Lead)&&B->HasSight(Gun,Escort));
            AActor* Wall=nullptr;
            if(Case==2)
            {
                Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall){Check(TEXT("v78_counter_wall_spawn"),false);return;}
                auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);
                Box->SetBoxExtent(FVector(35,500,220));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
                Box->RegisterComponent();Wall->SetActorLocation(Origin+FVector(Dir*1000,0,100));
            }
            const bool Observed=FrontlineDifficultyV30::CanObserve(B,Lead)&&FrontlineDifficultyV30::CanObserve(B,Escort);
            const float ObservationRange=FMath::Max(4800.f,FrontlineWeapons::ProfileFor(Gun).FireRangeCm)*B->EncounterVisibilityScale();
            Check(FString::Printf(TEXT("v78_counter_fixture_contacts_are_in_real_observation_range_case%d"),Case),FVector::Dist2D(Gun->Feet(),Lead->Feet())<ObservationRange&&FVector::Dist2D(Gun->Feet(),Escort->Feet())<ObservationRange);
            Check(FString::Printf(TEXT("v78_counter_fixture_physical_sight_matches_wall_case%d"),Case),(B->HasSight(Gun,Lead)&&B->HasSight(Gun,Escort))==(Case!=2));
            Check(FString::Printf(TEXT("v78_counter_uses_physical_observation_case%d"),Case),Observed==(Case!=2));
            const int32 Before=B->Squads.Num();B->ThinkEnemy();
            const auto Wanted=Case==2?EFrontlineRole::Rifle:FrontlineChoiceTargetsV78::PrecisionCounter(B->GetFactionForTeam(1));
            Check(FString::Printf(TEXT("v78_observed_precision_counter_recruits_legal_actual_squad_case%d"),Case),B->Squads.Num()==Before+1&&B->Squads.Last().Team==1
                &&B->Squads.Last().Role==Wanted&&B->IsRoleUnlocked(Wanted,1)&&B->RecruitCooldown(Wanted,1)>0);
            if(Wall){Wall->SetActorEnableCollision(false);Wall->Destroy();}
        }
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        const bool Safe=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
        Check(TEXT("v78_choice_fixture_requires_disposable_profile"),Safe);if(!Safe)return;
        const bool Empty=B->Units.IsEmpty()&&B->Squads.IsEmpty();Check(TEXT("v78_choice_fixture_starts_in_empty_encounter"),Empty);if(!Empty)return;
        // The calling review abandons this disposable encounter after the checks.
        const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldSmoke=B->SmokeZones;const auto OldStrikes=B->Strikes;
        const int32 OldShots=B->ShotsFired,OldGrenades=B->GrenadesThrown;
        const auto OldPhase=B->Phase;const int32 OldOutro=B->BattleOutroWinner;
        const auto OldRandom=B->Random;const auto OldMessage=B->LastMessage;
        const auto OldBoundId=B->BoundCampaignId,OldBoundEncounter=B->BoundCampaignEncounterId;
        const int32 OldMission=B->BoundCampaignMission,OldStage=B->BoundAttachmentStageV53;const bool OldAdvanced=B->bCampaignMissionAdvanced;
        TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Fixtures;
        ON_SCOPE_EXIT
        {
            for(const auto& W:Fixtures)if(auto* U=W.Get();IsValid(U)){U->SetActorEnableCollision(false);U->Destroy();}
            for(const auto& S:B->Strikes)if(S.Projectile.IsValid()&&!OldStrikes.ContainsByPredicate([&](const FFrontlineStrike& Prior){return Prior.Projectile==S.Projectile;}))S.Projectile->Destroy();
            B->Strikes=OldStrikes;B->ShotsFired=OldShots;B->GrenadesThrown=OldGrenades;
            B->Units=OldUnits;B->Squads=OldSquads;B->SmokeZones=OldSmoke;B->Phase=OldPhase;B->BattleOutroWinner=OldOutro;
            B->Random=OldRandom;B->LastMessage=OldMessage;B->BoundCampaignId=OldBoundId;B->BoundCampaignEncounterId=OldBoundEncounter;
            B->BoundCampaignMission=OldMission;B->BoundAttachmentStageV53=OldStage;B->bCampaignMissionAdvanced=OldAdvanced;
            FrontlineChoiceTargetsV78::Reset(B);
        };
        B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        B->BindCampaignEncounter();
        int32 Serial=780000;
        auto Make=[&](int32 Team,EFrontlineRole Role,const FVector& P,int32 Member=0)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,Serial++,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;
            U->SetActorLocation(P+FVector(0,0,88));B->Units.Add(U);Fixtures.Add(U);return U;
        };
        // The rejected combined candidate also changed flame aim points. Verify
        // both factions now use normal acquisition even beside a visible group.
        for(int32 Team=0;Team<2;++Team)
        {
            const int32 Dir=B->GetTeamDirection(Team);const FVector Origin(0,Team*3000.f,10000.f);
            auto* Flame=Make(Team,EFrontlineRole::Flamethrower,Origin);
            if(!Flame){Check(TEXT("v78_flame_regression_operator_spawn"),false);return;}
            const float Range=FMath::Min(FrontlineWeapons::ProfileFor(Flame).FireRangeCm*B->EncounterVisibilityScale(),B->HasTechnology(18,Team)?1450.f:1250.f);
            auto* Lone=Make(1-Team,EFrontlineRole::Rifle,Origin+FVector(Dir*Range*.75f,-Range*.4f,0));
            auto* Group=Make(1-Team,EFrontlineRole::Rifle,Origin+FVector(Dir*Range*.94f,0,0));
            auto* Left=Make(1-Team,EFrontlineRole::Rifle,Group?Group->Feet()+FVector(0,-70,0):Origin);
            auto* Right=Make(1-Team,EFrontlineRole::Rifle,Group?Group->Feet()+FVector(0,70,0):Origin);
            Check(FString::Printf(TEXT("v78_flame_regression_has_real_actors_team%d"),Team),Flame&&Lone&&Group&&Left&&Right);
            if(!Lone||!Group||!Left||!Right)return;
            Check(FString::Printf(TEXT("v78_flame_regression_has_visible_reachable_alternatives_team%d"),Team),B->HasSight(Flame,Lone)&&B->HasSight(Flame,Group)
                &&FVector::Dist2D(Flame->Feet(),Lone->Feet())<Range&&FVector::Dist2D(Flame->Feet(),Group->Feet())<Range);
            Check(FString::Printf(TEXT("v78_flame_primary_keeps_normal_acquisition_team%d"),Team),FrontlineFieldV32::IsFlameOperator(Flame)
                &&!FrontlineChoiceTargetsV78::IsAreaOperator(Flame)&&B->AcquireTarget(Flame)==Lone&&FrontlineChoiceTargetsV78::Pick(B,Flame,Lone)==Lone);
            Check(FString::Printf(TEXT("v78_flame_primary_does_not_force_aim_replacement_team%d"),Team),!FrontlineChoiceTargetsV78::ShouldRetarget(B,Flame,Group,Lone));
            Lone->SetActorLocation(Origin+FVector(Dir*300.f,0,88));
            Check(FString::Printf(TEXT("v78_flame_near_contact_uses_existing_acquisition_team%d"),Team),B->AcquireTarget(Flame)==Lone
                &&!FrontlineChoiceTargetsV78::ShouldRetarget(B,Flame,Lone,Group));
            Flame->MemberIndex=1;
            Check(FString::Printf(TEXT("v78_flame_rifle_escort_keeps_normal_acquisition_team%d"),Team),!FrontlineFieldV32::IsFlameOperator(Flame)
                &&!FrontlineChoiceTargetsV78::IsAreaOperator(Flame)&&B->AcquireTarget(Flame)==Lone&&!FrontlineChoiceTargetsV78::ShouldRetarget(B,Flame,Group,Lone));
            for(const auto& W:Fixtures)if(auto* U=W.Get();IsValid(U)){U->SetActorEnableCollision(false);U->Destroy();B->Units.Remove(U);}
            Fixtures.Empty();
        }
        for(int32 Team=0;Team<2;++Team)
        {
            const int32 Dir=B->GetTeamDirection(Team);const FVector Origin(0,Team*3000.f,10000.f);
            const bool Grenadier=B->GetFactionForTeam(Team)==EFrontlineFaction::British;
            if(!Grenadier)continue;
            auto* Shooter=Make(Team,EFrontlineRole::RifleGrenadier,Origin);
            if(!Shooter){Check(TEXT("v78_area_operator_spawn"),false);return;}
            const float FireRange=FrontlineWeapons::ProfileFor(Shooter).FireRangeCm*B->EncounterVisibilityScale();
            const float IsolatedX=FMath::Min(1600.f,FireRange*.82f);
            const float Side=FMath::Min(600.f,FireRange*.30f),ClusterX=FMath::Min(1850.f,FireRange*.96f);
            auto* Isolated=Make(1-Team,EFrontlineRole::Rifle,Origin+FVector(Dir*IsolatedX,-Side,0));
            auto* Cluster=Make(1-Team,EFrontlineRole::Rifle,Origin+FVector(Dir*ClusterX,0,0));
            auto* Left=Make(1-Team,EFrontlineRole::Rifle,Cluster?Cluster->Feet()+FVector(0,-70,0):Origin);
            auto* Right=Make(1-Team,EFrontlineRole::Rifle,Cluster?Cluster->Feet()+FVector(0,70,0):Origin);
            Check(FString::Printf(TEXT("v78_area_fixture_actors_team%d"),Team),Shooter&&Isolated&&Cluster&&Left&&Right);
            if(!Shooter||!Isolated||!Cluster||!Left||!Right)return;
            using namespace FrontlineChoiceTargetsV78;
            auto* Selected=B->AcquireTarget(Shooter);
            const bool Group=Selected==Cluster||Selected==Left||Selected==Right;
            Check(FString::Printf(TEXT("v78_actual_area_acquisition_prefers_visible_group_team%d"),Team),Group);
            auto* ExpectedContact=FVector::Dist2D(Shooter->Feet(),Isolated->Feet())<=FrontlineWeapons::AdvanceContactRangeFor(Shooter)*B->EncounterVisibilityScale()?Isolated:nullptr;
            Check(FString::Printf(TEXT("v78_area_retains_normal_advance_contact_team%d"),Team),B->AcquireTarget(Shooter,true)==ExpectedContact);
            Check(FString::Printf(TEXT("v78_area_reconsiders_a_lone_aim_point_team%d"),Team),Group&&ShouldRetarget(B,Shooter,Selected,Isolated));
            const float SelectedHealth=Selected?Selected->Health:0;
            if(Selected)Selected->Health=0;
            Check(FString::Printf(TEXT("v78_area_cache_rejects_dead_target_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));
            if(Selected)Selected->Health=SelectedHealth;
            const FVector SelectedPosition=Selected?Selected->GetActorLocation():Origin;
            if(Selected)Selected->SetActorLocation(SelectedPosition+FVector(0,8,0));
            Check(FString::Printf(TEXT("v78_area_cache_rejects_moved_target_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));
            if(Selected)Selected->SetActorLocation(SelectedPosition);
            const float OldElapsed=B->Elapsed;B->Elapsed-=1;
            Check(FString::Printf(TEXT("v78_area_cache_rejects_clock_rollback_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));B->Elapsed=OldElapsed;
            const auto OldEncounter=B->EncounterId;B->EncounterId=FGuid::NewGuid();
            Check(FString::Printf(TEXT("v78_area_cache_rejects_another_encounter_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));B->EncounterId=OldEncounter;
            auto* OtherOperator=Make(Team,Shooter->Role,Origin+FVector(0,30,0));
            Check(FString::Printf(TEXT("v78_area_cache_is_not_shared_between_operators_team%d"),Team),OtherOperator&&!ShouldRetarget(B,OtherOperator,Selected,Isolated));
            if(OtherOperator){OtherOperator->Health=0;OtherOperator->SetActorEnableCollision(false);B->Units.Remove(OtherOperator);}
            const float OldSuppression=Shooter->Suppression;Shooter->Suppression+=.1f;
            Check(FString::Printf(TEXT("v78_area_cache_rejects_changed_suppression_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));Shooter->Suppression=OldSuppression;
            FFrontlineSmokeZone Smoke;Smoke.Centre=Origin+FVector(Dir*ClusterX*.7f,0,0);Smoke.Radius=60;Smoke.StartedAt=B->Elapsed;Smoke.ExpiresAt=B->Elapsed+10;
            B->SmokeZones.Add(Smoke);
            Check(FString::Printf(TEXT("v78_area_cache_rechecks_new_smoke_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));B->SmokeZones.Empty();
            Isolated->SetActorLocation(Origin+FVector(Dir*700.f,0,88));
            Check(FString::Printf(TEXT("v78_area_near_threat_stays_urgent_team%d"),Team),B->AcquireTarget(Shooter)==Isolated);
            Isolated->SetActorLocation(Origin+FVector(Dir*IsolatedX,-Side,88));
            auto* Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall){Check(TEXT("v78_area_wall_spawn"),false);return;}
            auto* Box=NewObject<UBoxComponent>(Wall);
            Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(35,90,220));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            Box->RegisterComponent();Wall->SetActorLocation(Origin+FVector(Dir*ClusterX*.7f,0,100));
            Check(FString::Printf(TEXT("v78_area_cache_rechecks_new_physical_obstruction_team%d"),Team),!ShouldRetarget(B,Shooter,Selected,Isolated));
            Check(FString::Printf(TEXT("v78_area_wall_preserves_physical_visibility_team%d"),Team),!B->HasSight(Shooter,Cluster)&&B->AcquireTarget(Shooter)==Isolated);
            Wall->SetActorEnableCollision(false);Wall->Destroy();
            if(Grenadier)
            {
                Selected=B->AcquireTarget(Shooter);
                auto* Friend=Make(Team,EFrontlineRole::Rifle,Cluster->Feet()+FVector(0,120,0));
                Check(TEXT("v78_grenadier_cache_rechecks_new_friendly_risk"),Friend&&!ShouldRetarget(B,Shooter,Selected,Isolated));
                Check(TEXT("v78_grenadier_friendly_risk_removes_cluster_preference"),Friend&&B->AcquireTarget(Shooter)==Isolated);
                if(Friend){Friend->Health=0;Friend->SetActorEnableCollision(false);B->Units.Remove(Friend);}
                Shooter->MemberIndex=1;
                Check(TEXT("v78_grenadier_rifle_escort_keeps_normal_targeting"),B->AcquireTarget(Shooter)==Isolated&&!IsAreaOperator(Shooter));Shooter->MemberIndex=0;
                const int32 Shots=Shooter->ShotsDischarged,Strikes=B->Strikes.Num();Shooter->DischargeWeapon(B->AcquireTarget(Shooter));
                Check(TEXT("v78_cluster_choice_uses_real_delayed_round_and_ammo"),Shooter->ShotsDischarged==Shots+1&&Shooter->RoundsRemaining==0&&B->Strikes.Num()==Strikes+1
                    &&B->Strikes.Last().bRifleLaunched&&B->Strikes.Last().ImpactAt>B->Elapsed&&B->Strikes.Last().Radius==FrontlineGrenade::BlastRadiusCm);
            }
            Selected=B->AcquireTarget(Shooter);
            const TWeakObjectPtr<AWarfareFrontlineSoldier> WeakShooter=Shooter,WeakSelected=Selected,WeakIsolated=Isolated;
            Check(FString::Printf(TEXT("v78_reopen_fixture_starts_with_a_live_comparable_choice_team%d"),Team),ShouldRetarget(B,Shooter,Selected,Isolated));
            B->Phase=EFrontlinePhase::Briefing;B->BeginBattle();
            Check(FString::Printf(TEXT("v78_actual_begin_battle_discards_previous_choice_team%d"),Team),B->Phase==EFrontlinePhase::Fighting&&WeakShooter.IsValid()&&WeakSelected.IsValid()&&WeakIsolated.IsValid()
                &&!ShouldRetarget(B,WeakShooter.Get(),WeakSelected.Get(),WeakIsolated.Get()));
            for(const auto& W:Fixtures)if(auto* U=W.Get();IsValid(U)){U->Health=0;U->SetActorEnableCollision(false);B->Units.Remove(U);}
        }
        for(const auto& W:Fixtures)if(auto* U=W.Get();IsValid(U)){U->SetActorEnableCollision(false);U->Destroy();}
        Fixtures.Empty();
        CounterChecks(B,Check);
    }
};
void FrontlineChoiceTargetsV78::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineChoiceTargetsV78Review::Run(B,Check);}
