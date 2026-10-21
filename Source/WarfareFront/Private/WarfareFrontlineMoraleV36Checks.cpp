#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineGameplayV33.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

// Explicit scenario checks of the production state machine. These are not an
// autonomous campaign playthrough or a statistical estimate of player win rate.
struct FFrontlineMoraleV36Review
{
static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B || !B->bProgressionVolatile || !FParse::Param(FCommandLine::Get(),TEXT("WFV36Review")))
    {Check(TEXT("morale_review_requires_volatile_v36_session"),false);return;}
    const auto SavedRuns=B->CampaignRuns;const auto SavedId=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
    const auto SavedA=B->ArmyProgress[0],SavedB=B->ArmyProgress[1];const auto SavedRewards=B->RewardedEncounterIds;
    const bool WasTicking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
    TArray<TWeakObjectPtr<AActor>> Temporary;
    ON_SCOPE_EXIT
    {
        for(const auto& Actor:Temporary)if(Actor.IsValid())Actor->Destroy();
        B->AbandonBattleToOperations();B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedId;B->PlayerFaction=SavedFaction;
        B->ArmyProgress[0]=SavedA;B->ArmyProgress[1]=SavedB;B->RewardedEncounterIds=SavedRewards;
        B->ResetEncounter();B->SetActorTickEnabled(WasTicking);
    };
    auto Select=[&](EFrontlineFaction F,int32 Mission,EFrontlineDifficulty D=EFrontlineDifficulty::Rookie)
    {
        B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.Difficulty=D;
        R.CompletedMissions=9;R.SelectedMission=Mission;R.AcknowledgedEventMask=MAX_uint32;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->ResetEncounter();B->BeginBattle();
        B->EnemyRecruitAt=B->EnemyThinkAt=B->EnemyFireAssessAt=B->EnemyAssaultAt=B->ExtendedSupportThinkAt=MAX_flt;
    };
    auto Spawn=[&](int32 Team,int32 Trench)->int32
    {
        if(!B->SpawnSquad(Team,EFrontlineRole::Rifle,Trench))return INDEX_NONE;
        for(const auto& U:B->Squads.Last().Members)if(U.IsValid())U->SetActorTickEnabled(false);
        return B->Squads.Last().Id;
    };
    auto PlaceSquad=[&](int32 Id,const FVector& P)
    {
        if(auto* Squad=B->FindSquad(Id))for(const auto& U:Squad->Members)if(U.IsValid())
        {FVector Point=P+FVector((U->MemberIndex%2)*18,U->MemberIndex*45,0);Point.Z=AWarfareFrontlineGround::HeightAt(Point.X,Point.Y)+88;U->SetActorLocation(Point);}
    };
    auto* Notice=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    if(!Notice){Check(TEXT("morale_notification_fixture_created"),false);return;}
    Temporary.Add(Notice);Notice->SetActorTickEnabled(false);Notice->Battle=B;Notice->Team=1;Notice->Health=0;
    bool DoctrineOrdered=true,AllEased=true;
    const float OldRecruit[]={1.f,.93f,.86f};const float OldDecision[]={1.f,.80f,.65f};
    for(int32 D=0;D<3;++D)
    {
        const auto& Doctrine=FrontlineDifficultyV30::Doctrine(EFrontlineDifficulty(D));
        AllEased&=FMath::IsNearlyEqual(Doctrine.RecruitScale/OldRecruit[D],1.06f,.0001f)
            && FMath::IsNearlyEqual(Doctrine.DecisionScale/OldDecision[D],1.06f,.0001f);
        if(D)DoctrineOrdered&=Doctrine.RecruitScale<FrontlineDifficultyV30::Doctrine(EFrontlineDifficulty(D-1)).RecruitScale;
    }
    Check(TEXT("all_difficulties_eased_six_percent_without_reversing_order"),AllEased&&DoctrineOrdered);
    Check(TEXT("all_roster_readiness_uses_fifteen_percent_faster_cadence"),FMath::IsNearlyEqual(FrontlinePacingV24::BaseRecruitScale/.85f,1.f/1.15f));
    bool Depth=true,ObjectiveText=true;int32 MissionCount=0;
    for(auto F:{EFrontlineFaction::British,EFrontlineFaction::German})for(const auto& M:B->GetCampaignMissionsForFaction(F))
    {
        ++MissionCount;const float Previous=M.Id==0?.88f:M.Id==1?.91f:M.Id==2?.94f:1.f;
        // The multiply/divide round trip can differ by one float ULP. UE's
        // default 1e-8 tolerance is smaller than that at this magnitude.
        Depth&=FMath::IsNearlyEqual(FrontlineGameplayV33::OperationDepthScale(M.Id)/Previous,FrontlineGameplayV33::CampaignDepthScale,1.e-6f);
        ObjectiveText&=M.HoldCenterSeconds==0 && FString(M.ObjectiveText).Contains(TEXT("两道战壕"));
    }
    Check(TEXT("all_twenty_missions_have_shorter_depth_and_explicit_territorial_objective"),MissionCount==20&&Depth&&ObjectiveText);
    UE_LOG(LogTemp,Display,TEXT("WF_V36_MISSION_CONTRACT count=%d depth=%d objective=%d"),MissionCount,Depth,ObjectiveText);
    // Feed casualty notifications into the real production handler, then move
    // an actual spawned squad through normal AdvanceSquad/UpdateSquads capture
    // transitions. Positions are staged, not claimed as simulated combat wins.
    for(auto F:{EFrontlineFaction::British,EFrontlineFaction::German})for(int32 D=0;D<3;++D)
    {
        Select(F,0,EFrontlineDifficulty(D));const int32 Team=0,Dir=B->GetTeamDirection(Team);
        const FString Prefix=FString::Printf(TEXT("faction_%d_difficulty_%d_"),int32(F),D);
        const float Start=B->Morale[1];for(int32 N=0;N<59;++N)B->RegisterCasualty(Notice,0);
        const float AfterAttrition=B->Morale[1];
        Check(Prefix+TEXT("old_collapse_threshold_now_leaves_morale_for_a_territorial_assault"),FMath::IsNearlyEqual(AfterAttrition,Start-59*FrontlineMoraleV36::CasualtyLoss,.001f)&&AfterAttrition>45);
        const int32 Id=Spawn(0,B->GetHomeTrenchIndex(0));bool Traversed=Id!=INDEX_NONE;
        for(int32 Step=0;Step<4 && Traversed;++Step)
        {
            auto* Squad=B->FindSquad(Id);const int32 From=Squad?Squad->Trench:INDEX_NONE;
            Traversed=Squad && B->AdvanceSquad(From,Id,0);if(!Traversed)break;
            Squad=B->FindSquad(Id);
            for(const auto& U:Squad->Members)if(U.IsValid())U->SetActorLocation(B->SoldierDestination(U.Get())+FVector(0,0,88));
            B->UpdateSquads();Squad=B->FindSquad(Id);
            Traversed=Squad && Squad->Trench==From+Dir;
            B->UpdateCampaignObjective(0);
        }
        Check(Prefix+TEXT("actual_capture_state_machine_finishes_by_territory_with_enemy_morale_remaining"),Traversed
            && B->Phase==EFrontlinePhase::Victory && B->Morale[1]>0 && B->EndReason.Contains(TEXT("纵深防线")));
        UE_LOG(LogTemp,Display,TEXT("WF_V36_OUTCOME_SCENARIO faction=%d difficulty=%d scenario=staged_capture casualties=59 enemy_morale=%.2f territorial=%d"),int32(F),D,B->Morale[1],Traversed&&B->Phase==EFrontlinePhase::Victory);
    }
    Select(EFrontlineFaction::British,0);
    B->CampaignHoldElapsed=20000;B->UpdateCampaignObjective(20000);
    Check(TEXT("fresh_positions_and_legacy_hold_cannot_auto_win"),B->Phase==EFrontlinePhase::Fighting && B->CampaignHoldElapsed==0 && !FrontlineMoraleV36::HasTerritorialVictory(B,0)&&!FrontlineMoraleV36::HasTerritorialVictory(B,1));
    int32 Count=0;while(B->Phase==EFrontlinePhase::Fighting && Count<250){B->RegisterCasualty(Notice,0);++Count;B->Tick(0);}
    Check(TEXT("production_casualty_sequence_still_allows_morale_collapse"),Count>=100 && Count<250 && B->Phase==EFrontlinePhase::Victory && B->EndReason.Contains(TEXT("士气")));
    UE_LOG(LogTemp,Display,TEXT("WF_V36_OUTCOME_SCENARIO scenario=casualty_notifications count=%d morale_ending=%d"),Count,B->Phase==EFrontlinePhase::Victory);

    Select(EFrontlineFaction::British,7);B->Morale[0]=60;B->Morale[1]=80;
    const int32 First=Spawn(0,2),Second=Spawn(0,2);
    PlaceSquad(First,FVector(1600,-300,0));PlaceSquad(Second,FVector(1700,250,0));
    for(int32 N=0;N<260;++N){B->Elapsed+=1;FrontlineMoraleV36::Tick(B,1);}
    const auto Recovery=FrontlineMoraleV36::Telemetry(B);const float Cohesion=Recovery.Recovery[0][2],Advanced=Recovery.Recovery[0][1];
    const float BeforeFarm=B->Morale[0];
    for(int32 N=0;N<260;++N){B->Elapsed+=1;FrontlineMoraleV36::Tick(B,1);}
    Check(TEXT("nearby_forward_sections_recover_but_cannot_farm_passive_morale"),Cohesion>5.9f&&Cohesion<=6.001f&&Advanced>0&&FMath::IsNearlyEqual(B->Morale[0],BeforeFarm,.001f));
    B->CaptureTrench(3,0);const float FirstCapture=FrontlineMoraleV36::Telemetry(B).Recovery[0][0];
    B->CaptureTrench(3,1);B->CaptureTrench(3,0);
    Check(TEXT("enemy_trench_capture_recovers_once_and_repeated_exchange_cannot_farm"),FMath::IsNearlyEqual(FirstCapture,3.f)&&FrontlineMoraleV36::Telemetry(B).Recovery[0][0]==FirstCapture);
    B->Morale[0]=0;B->Elapsed+=1;FrontlineMoraleV36::Tick(B,1);B->Tick(0);
    Check(TEXT("zero_morale_cannot_be_resurrected_by_recovery_tick"),B->Morale[0]==0 && B->Phase==EFrontlinePhase::Defeat);

    Select(EFrontlineFaction::British,7);B->Morale[0]=80;const int32 GasId=Spawn(0,2);
    PlaceSquad(GasId,FVector(1700,0,0));B->Elapsed=100;
    FFrontlineSmokeZone Gas;Gas.bGas=true;Gas.Centre=FVector(1700,120,0);Gas.Radius=950;Gas.StartedAt=0;Gas.ExpiresAt=1000;Gas.Team=1;B->SmokeZones={Gas};
    FrontlineMoraleV36::Tick(B,1);const float Unmasked=FrontlineMoraleV36::Telemetry(B).Loss[0][2];
    B->ActiveResearchMutable().TechnologyMask|=1u<<16;B->Elapsed+=1;FrontlineMoraleV36::Tick(B,1);
    const float Masked=FrontlineMoraleV36::Telemetry(B).Loss[0][2]-Unmasked;
    B->SmokeZones[0].bGas=false;B->Elapsed+=1;FrontlineMoraleV36::Tick(B,1);
    Check(TEXT("actual_gas_zone_causes_shock_researched_donned_mask_reduces_it_smoke_does_not"),Unmasked>.2f && Masked>0 && Masked<Unmasked*.2f
        && FMath::IsNearlyEqual(FrontlineMoraleV36::Telemetry(B).Loss[0][2],Unmasked+Masked,.001f));

    Select(EFrontlineFaction::British,7);B->Morale[0]=80;const int32 Friendly=Spawn(0,2);
    auto* Victim=B->FindSquad(Friendly)?B->FindSquad(Friendly)->Members[0].Get():nullptr;
    const float BeforeHit=B->Morale[0];if(Victim)Victim->ReceiveHit(1,.5f,0,FVector(1,0,0),EFrontlineDamageKind::Artillery,.4f);
    Check(TEXT("actual_friendly_explosive_death_has_distinct_morale_penalty"),Victim && !Victim->IsAlive()
        && BeforeHit-B->Morale[0]>2.6f && FrontlineMoraleV36::Telemetry(B).Loss[0][1]>=1.99f);

    Select(EFrontlineFaction::British,7);B->Elapsed=500;B->Morale[0]=80;
    const int32 Exposed=Spawn(0,2);PlaceSquad(Exposed,FVector(1600,-150,0));
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    const bool TankMade=Fleet && Fleet->Recruit(EFrontlineVehicle::A7V,1);
    AFrontlineTank* Tank=TankMade?Fleet->GetVehicles().Last().Get():nullptr;
    if(Tank){Tank->SetActorLocation(FVector(2450,0,AWarfareFrontlineGround::HeightAt(2450,0)));Tank->Simulate(.01f);}
    FrontlineMoraleV36::Tick(B,1);const float ArmourShock=FrontlineMoraleV36::Telemetry(B).Loss[0][3];
    FFrontlineSmokeZone Screen;Screen.Centre=FVector(2050,0,0);Screen.Radius=550;Screen.StartedAt=0;Screen.ExpiresAt=1000;B->SmokeZones={Screen};
    B->Elapsed+=1;FrontlineMoraleV36::Tick(B,1);
    Check(TEXT("visible_operational_armour_pressures_exposed_infantry_smoke_blocks_observation"),Tank && Tank->HasEnteredField() && ArmourShock>0
        && FMath::IsNearlyEqual(FrontlineMoraleV36::Telemetry(B).Loss[0][3],ArmourShock,.001f));
    B->SmokeZones.Empty();B->Elapsed+=1;const float BeforeDestroy=FrontlineMoraleV36::Telemetry(B).Loss[0][3];
    if(Tank)Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,10000);FrontlineMoraleV36::Tick(B,1);
    Check(TEXT("destroyed_armour_cannot_keep_causing_fear"),Tank && !Tank->IsOperational() && FMath::IsNearlyEqual(FrontlineMoraleV36::Telemetry(B).Loss[0][3],BeforeDestroy,.001f));
}
};
void RunFrontlineMoraleV36Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineMoraleV36Review::Run(B,Check);}
