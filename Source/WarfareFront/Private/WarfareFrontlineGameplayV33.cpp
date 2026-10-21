#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace FrontlineGameplayV33
{
float RelativeDepthScaleV45(int32 MissionId)
{
    // Both twenty-operation routes: first five, middle thirteen, final pair.
    if(MissionId<0 || MissionId>=20)return 1.f;
    return MissionId<5?.65f:MissionId<18?.75f:1.f;
}
float OperationDepthScale(int32 MissionId)
{
    const auto& M=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    // Quantised terrain/water package names must agree between /fp:precise
    // Editor and /fp:fast Shipping. Reassociation crosses the Op6 key boundary.
    volatile float CampaignScaled=(M.IsValidIndex(MissionId)?M[MissionId].DepthScaleV41:1.f)*CampaignDepthScale;
    volatile float OperationScaled=CampaignScaled*RelativeDepthScaleV45(MissionId);
    return OperationScaled;
}
void RunDepthChecksV45(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    const bool Isolated=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    Check(TEXT("v45_depth_checks_use_isolated_volatile_world"),Isolated);if(!Isolated)return;
    const float OldScale=FrontlineBattlefield::OperationDepthScale;
    ON_SCOPE_EXIT{FrontlineBattlefield::ConfigureOperationDepth(OldScale);};
    const auto& UK=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    const auto& DE=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::German);
    bool Relative=UK.Num()==20&&DE.Num()==20,Mirrored=Relative,Geometry=Relative,FinalPair=Relative;
    for(int32 I=0;I<FMath::Min(UK.Num(),DE.Num());++I)
    {
        const float V44=UK[I].DepthScaleV41*CampaignDepthScale,Current=OperationDepthScale(I);
        Relative&=FMath::IsNearlyEqual(Current/V44,I<5?.65f:I<18?.75f:1.f,.00001f);
        Mirrored&=UK[I].DepthScaleV41==DE[I].DepthScaleV41&&UK[I].TerrainStyleV32==DE[I].TerrainStyleV32;
        if(I>=18)FinalPair&=Current==V44;
        FrontlineBattlefield::ConfigureOperationDepth(Current);
        using namespace FrontlineBattlefield;
        Geometry&=FMath::IsFinite(TrenchSpacing)&&FMath::IsNearlyEqual(TrenchSpacing,AuthoringTrenchSpacing*Current*(I<18?2.f:1.f),.005f)
            &&TrenchCount==(I<18?3:5)&&TrenchSpacing>1800.f&&FMath::IsNearlyZero(TrenchX(0)+TrenchX(TrenchCount-1))
            &&FMath::IsNearlyEqual(TrenchX(TrenchCount-1),2.f*AuthoringTrenchSpacing*Current,.005f)
            &&SpawnHalfLength>TrenchX(TrenchCount-1)&&GoalHalfLength>SpawnHalfLength&&SupportHalfLength>GoalHalfLength;
    }
    Check(TEXT("v45_first_five_35_percent_middle_thirteen_25_percent_relative_to_v44"),Relative);
    Check(TEXT("v45_final_two_route_depths_unchanged"),FinalPair);
    Check(TEXT("v45_both_armies_share_bake_profile_and_mirrored_geometry"),Mirrored);
    Check(TEXT("v45_shortened_depth_is_applied_without_fallback_or_overlapping_bank_footings"),Geometry);
    Check(TEXT("v45_invalid_operation_keeps_existing_default_scale"),OperationDepthScale(INDEX_NONE)==CampaignDepthScale
        &&RelativeDepthScaleV45(20)==1.f);
}
bool EnemyRoleAvailable(const AWarfareFrontlineGameMode* B,EFrontlineRole Role)
{
    if(!B || uint8(Role)>=uint8(EFrontlineRole::Count))return false;
    const auto* M=FrontlineCampaignV32::CurrentOperation(B);
    if(!FrontlineCampaignV41::RoleAllowed(B->GetFactionForTeam(1),Role))return false;
    if(FrontlineUltimateArchiveV8215::FullArmory(B,1))return true;
    if(!M)return true;
    return M->Id>=B->RoleUnlockMission(Role,1) && M->EnemyRoleOrder.Contains(Role);
}
bool IsOpeningAssembly(int32 MissionId,int32 Recruited,int32 Living,float Elapsed,bool HasObservedContact)
{
    return MissionId>=0 && MissionId<=2 && Recruited<=3 && Living<=18 && Elapsed<45.f && !HasObservedContact;
}
void ApplyEarlyMissionPacing(TArray<FFrontlineCampaignMission>& Missions,bool GermanPlayer)
{
    for(auto& M:Missions)
        M.ObjectiveText=TEXT("越过敌军最后一道战壕，深入后方阵地，或迫使敌军撤退");
    for(auto& M:Missions)if(M.Id>=0 && M.Id<=2)
    {
        M.EnemyAssaultDelay=M.Id==0?11.5f:M.Id==1?11.f:10.5f;
        // Earlier orders introduce the opposing company's weapons one at a
        // time. The later campaign keeps its original specialist compositions.
        if(M.Id==0)
        {
            M.EnemyOpeningRoles={EFrontlineRole::Rifle,EFrontlineRole::Rifle,EFrontlineRole::Rifle};
        }
        else if(M.Id==1)
        {
            M.EnemyOpeningRoles={EFrontlineRole::Lewis,EFrontlineRole::Rifle,EFrontlineRole::Mills};
        }
        else
        {
            M.EnemyOpeningRoles={EFrontlineRole::Lewis,EFrontlineRole::Pioneer,EFrontlineRole::Rifle};
        }
        M.bEnemyScopedMarksmen=false;
        M.bEnemyCommissionedOfficers=false;
    }
}
}

struct FFrontlineGameplayV33Access
{
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!B || !B->bProgressionVolatile || B->Phase==EFrontlinePhase::Fighting)
        {Check(TEXT("v33_gameplay_requires_isolated_idle_archive"),false);return;}
        const auto SavedRuns=B->CampaignRuns;const auto SavedId=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
        const auto SavedA=B->ArmyProgress[0],SavedB=B->ArmyProgress[1];const auto SavedRewards=B->RewardedEncounterIds;
        const bool WasTicking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
        ON_SCOPE_EXIT
        {
            B->AbandonBattleToOperations();B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedId;B->PlayerFaction=SavedFaction;
            B->ArmyProgress[0]=SavedA;B->ArmyProgress[1]=SavedB;B->RewardedEncounterIds=SavedRewards;
            B->ResetEncounter();B->SetActorTickEnabled(WasTicking);
        };
        auto Select=[&](EFrontlineFaction F,int32 Mission,EFrontlineDifficulty D=EFrontlineDifficulty::Rookie)
        {
            B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.Difficulty=D;
            R.CompletedMissions=9;R.SelectedMission=Mission;R.Research.TechnologyMask=(1u<<24)-1;R.Research.ResearchPoints=17;
            R.RoleSelectionMask=1u;R.SupportSelectionMask=1u;R.AcknowledgedEventMask=0x12;
            B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->ResetEncounter();
        };
        bool Depths=true,Mirrored=true,Reinforcement=true,Sequence=true,Later=true;
        float PreviousSpan=0;
        for(int32 M=0;M<10;++M)
        {
            Select(EFrontlineFaction::British,M);
            const float Span=B->Trenches.Last().X-B->Trenches[0].X;
            if(M<3)Depths&=Span<4.f*FrontlineBattlefield::AuthoringTrenchSpacing && Span>PreviousSpan;
            else Later&=FMath::IsNearlyEqual(Span,4.f*FrontlineBattlefield::AuthoringTrenchSpacing*FrontlineGameplayV33::CampaignDepthScale,.01f);
            PreviousSpan=Span;
            for(int32 I=0;I<B->Trenches.Num();++I)
            {
                Mirrored&=FMath::IsNearlyZero(B->Trenches[I].X+B->Trenches[B->Trenches.Num()-1-I].X);
                if(I)Sequence&=B->Trenches[I].X-B->Trenches[I-1].X>2700;
            }
            for(int32 Dir:{-1,1})
            {
                const float Camera=-Dir*(FrontlineBattlefield::SpawnHalfLength-1600.f);
                const float Start=FrontlinePacingV24::ReinforcementStartX(Dir,Camera,6000,2,.3f);
                Reinforcement&=!FrontlinePacingV24::HasCrossedEntry(Start,Dir) && (Start-Camera)*Dir<-3300;
                Reinforcement&=FrontlineBattlefield::GoalHalfLength>FrontlineBattlefield::SpawnHalfLength
                    && FrontlineBattlefield::SupportHalfLength>FrontlineBattlefield::GoalHalfLength;
            }
        }
        Check(TEXT("first_three_approaches_shorten_gradually_with_five_distinct_trenches"),Depths&&Sequence);
        Check(TEXT("later_operations_share_v37_depth_reduction"),Later);
        Check(TEXT("both_factions_keep_mirrored_goals_and_offscreen_walk_on_entries"),Mirrored&&Reinforcement);

        bool IntroRoles=true,ContinuesRecruiting=true,NoBlindFire=true;
        for(int32 F=0;F<2;++F)for(int32 M=0;M<3;++M)
        {
            Select(EFrontlineFaction(F),M);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            B->ArtilleryReadyAt[1]=B->MortarReadyAt[1]=0;
            // Replay has every technology unlocked. The opposing introductory
            // force must still follow this operation, not the player's archive.
            for(int32 R=0;R<int32(EFrontlineRole::Count);++R)
                IntroRoles&=B->IsRoleUnlocked(EFrontlineRole(R),1)==(M>=B->RoleUnlockMission(EFrontlineRole(R),1));
            for(int32 T=0;T<=55;++T){B->Elapsed=float(T);B->ThinkEnemy();}
            ContinuesRecruiting&=B->EnemyRecruitStep>=4;
            for(const auto& Squad:B->Squads)if(Squad.Team==1)IntroRoles&=FrontlineGameplayV33::EnemyRoleAvailable(B,Squad.Role);
            NoBlindFire&=B->ArtilleryCommands==0 && B->MortarCommands==0 && !B->bEnemyFirePending;
        }
        Check(TEXT("replaying_early_operations_keeps_opposing_specialist_introductions"),IntroRoles);
        Check(TEXT("both_factions_first_three_ai_assemble_multiple_waves_without_starvation"),ContinuesRecruiting);
        Check(TEXT("opponent_does_not_register_artillery_without_observed_contacts"),NoBlindFire);
        using namespace FrontlineGameplayV33;
        Check(TEXT("opening_short_rest_ends_after_contact_or_first_wave"),IsOpeningAssembly(0,3,15,24,false)
            && !IsOpeningAssembly(0,3,15,24,true) && !IsOpeningAssembly(0,6,15,30,false)
            && !IsOpeningAssembly(5,3,15,24,false) && !IsOpeningAssembly(0,3,15,60,false));

        bool Flanks=true;
        for(int32 D=0;D<3;++D)for(int32 Side:{-1,1})
        {
            FrontlineDifficultyV30::FContact C;C.Position=FVector(0,Side*1050,0);C.LastSeen=60;
            FRandomStream R(3317+D);const auto& Doctrine=FrontlineDifficultyV30::Doctrine(EFrontlineDifficulty(D));
            const FVector Registered=FrontlineDifficultyV30::RegisteredPoint(C,60,Doctrine,R);
            Flanks&=Registered.Y*Side>900 && FMath::Abs(Registered.Y)<FrontlineBattlefield::HalfFrontage;
        }
        Check(TEXT("registered_fire_can_reach_observed_outer_flanks_on_all_difficulties"),Flanks);

        Select(EFrontlineFaction::German,2);const auto Before=*B->GetCurrentCampaign();const int32 RewardCount=B->RewardedEncounterIds.Num();
        B->BeginBattle();const bool HadSquad=B->SpawnSquad(0,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(0));
        TWeakObjectPtr<AWarfareFrontlineSoldier> Soldier=B->Units.IsEmpty()?nullptr:B->Units[0].Get();
        AActor* Projectile=B->GetWorld()->SpawnActor<AActor>();FFrontlineStrike Strike;Strike.Projectile=Projectile;Strike.ImpactAt=2;
        B->Strikes.Add(Strike);B->SmokeZones.AddDefaulted();B->Tracers.AddDefaulted();
        TWeakObjectPtr<AActor> Committed=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const FGuid OldEncounter=B->GetEncounterId();B->BattleOutroWinner=1;B->BattleOutroElapsed=2;
        B->bEnemyFirePending=true;B->EnemyFireExecuteAt=4;B->Elapsed=120;
        auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        if(Audio){Audio->SetBattleActive(false);Audio->PlayOutcomeV32(true,FVector::ZeroVector);}
        UGameplayStatics::SetGamePaused(B,true);B->AbandonBattleToOperations();
        const auto* After=B->GetCurrentCampaign();
        const bool Retained=After && After->Id==Before.Id && After->SelectedMission==Before.SelectedMission
            && After->CompletedMissions==Before.CompletedMissions && After->Research.ResearchPoints==Before.Research.ResearchPoints
            && After->Research.TechnologyMask==Before.Research.TechnologyMask && After->Research.Victories==Before.Research.Victories
            && After->Research.Defeats==Before.Research.Defeats && After->RoleSelectionMask==Before.RoleSelectionMask
            && After->SupportSelectionMask==Before.SupportSelectionMask && After->AcknowledgedEventMask==Before.AcknowledgedEventMask;
        Check(TEXT("abandon_preserves_selected_campaign_research_unlocks_loadout_and_no_reward"),Retained
            && RewardCount==B->RewardedEncounterIds.Num() && !B->bEncounterRewarded && B->LastResearchReward==0);
        const bool Removed=HadSquad && (!Soldier.IsValid() || Soldier->IsActorBeingDestroyed()) && (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed())
            && (!Committed.IsValid() || Committed->IsActorBeingDestroyed());
        Check(TEXT("abandon_cancels_paused_battle_units_strikes_smoke_outro_and_observed_orders"),Removed
            && B->Phase==EFrontlinePhase::Briefing && !UGameplayStatics::IsGamePaused(B) && B->GetEncounterId()!=OldEncounter
            && B->Units.IsEmpty() && B->Squads.IsEmpty() && B->Strikes.IsEmpty() && B->SmokeZones.IsEmpty() && B->Tracers.IsEmpty()
            && B->BattleOutroWinner==INDEX_NONE && !B->bEnemyFirePending && !B->BoundCampaignEncounterId.IsValid());
        Check(TEXT("abandon_cancels_delayed_relief_and_battle_audio_even_when_already_inactive"),Audio && !Audio->IsBattleActive()
            && Audio->OutcomeAtV32<0 && Audio->GetActiveEffects()==0 && Audio->GetActiveVoices()==0);
        B->BeginBattle();Check(TEXT("redeployment_after_abandon_has_clean_binding_and_zero_initial_squads"),B->Phase==EFrontlinePhase::Fighting
            && B->GetEncounterCampaignMission() && B->GetEncounterCampaignMission()->Id==Before.SelectedMission && B->Units.IsEmpty()
            && B->Elapsed==0 && B->Casualties[0]==0 && B->Casualties[1]==0);
    }
};
void FrontlineGameplayV33::RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineGameplayV33Access::Run(B,Check);}
