#include "WarfareFrontlineUnlocksV53.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

namespace
{
bool IsolatedReview(const AWarfareFrontlineGameMode* B)
{
    FString UserDir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
struct FUnlockSnapshot
{
    uint32 Roles[2]{},Supports[2]{},Vehicles[2]{};
    FrontlineCampaignV32::FLoadout Loadout;
    uint32 Technologies=0;
    int32 Year=0;
    FString WeaponDateGate;
    TArray<float> Readiness;
};
FUnlockSnapshot Snapshot(const AWarfareFrontlineGameMode* B)
{
    FUnlockSnapshot S;S.Loadout=FrontlineCampaignV32::GetLoadout(B);
    S.Technologies=B->GetActiveResearch().TechnologyMask;S.Year=B->GetEncounterYear();S.WeaponDateGate=B->TechnologyRoleBlockReason(12);
    for(int32 T=0;T<2;++T)
    {
        for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
        {if(B->IsRoleUnlocked(EFrontlineRole(I),T))S.Roles[T]|=1u<<I;S.Readiness.Add(B->RecruitCooldown(EFrontlineRole(I),T));}
        for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)
        {if(B->IsSupportUnlocked(EFrontlineSupport(I),T))S.Supports[T]|=1u<<I;S.Readiness.Add(B->SupportCooldown(EFrontlineSupport(I),T));}
        if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& E:V->GetRoster(T))
            if(V->IsUnlocked(E.Type,T))S.Vehicles[T]|=1u<<int32(E.Type);
    }
    return S;
}
bool SameRoster(const FUnlockSnapshot& A,const FUnlockSnapshot& B)
{
    for(int32 T=0;T<2;++T)if(A.Roles[T]!=B.Roles[T]||A.Supports[T]!=B.Supports[T]||A.Vehicles[T]!=B.Vehicles[T])return false;
    return A.Loadout.EligibleRoles==B.Loadout.EligibleRoles&&A.Loadout.SelectedRoles==B.Loadout.SelectedRoles
        &&A.Loadout.EligibleSupports==B.Loadout.EligibleSupports&&A.Loadout.SelectedSupports==B.Loadout.SelectedSupports;
}
}

struct FFrontlineUnlocksV53Access
{
    static int32 Stage(const AWarfareFrontlineGameMode* B,int32 Team)
    {
        if(!B||Team<0||Team>1)return INDEX_NONE;
        if(B->Phase==EFrontlinePhase::Fighting||B->Phase==EFrontlinePhase::Victory||B->Phase==EFrontlinePhase::Defeat)
            if(const auto* M=B->GetEncounterCampaignMission())
                return Team==1?M->Id:FMath::Max(M->Id,B->BoundAttachmentStageV53);
        const auto* R=B->GetCurrentCampaign();
        return R?(Team==1?R->SelectedMission:FMath::Max(R->CompletedMissions,R->SelectedMission)):MAX_int32;
    }

    // Installs only isolated test data. Binding and awards below use the actual
    // production APIs, including the archive receipt and branch advance.
    static bool Fixture(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Completed,int32 Selected,bool Legacy=false)
    {
        if(!IsolatedReview(B)||!B->GetCampaignMissionsForFaction(F).IsValidIndex(Selected))return false;
        FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.CompletedMissions=Completed;R.SelectedMission=Selected;
        R.Research.ResearchPoints=17;R.Research.TechnologyMask=(1u<<2)|(1u<<12)|(1u<<27);R.bMobileGunV52=true;
        if(Legacy){FJsonObject Old;if(!FrontlineCampaignV52::ReadArchive(Old,R))return false;}
        else FrontlineCampaignV52::Initialize(R);
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->Phase=EFrontlinePhase::Briefing;
        B->EncounterId=FGuid::NewGuid();B->BoundCampaignEncounterId.Invalidate();B->BoundCampaignId.Invalidate();
        B->BoundCampaignMission=B->BoundAttachmentStageV53=INDEX_NONE;B->bCampaignMissionAdvanced=false;B->bEncounterRewarded=false;
        B->BattleOutroWinner=INDEX_NONE;B->Elapsed=90;B->ShotsFired=8;
        return true;
    }

    static void Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!IsolatedReview(B)){Check(TEXT("v53_unlock_checks_require_isolated_profile"),false);return;}
        const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId;const auto Faction=B->PlayerFaction;
        const auto Receipts=B->RewardedEncounterIds;const bool ReadOnly=B->bProgressionReadOnly,Ticking=B->IsActorTickEnabled();
        ON_SCOPE_EXIT{B->AbandonBattleToOperations();B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;
            B->RewardedEncounterIds=Receipts;B->bProgressionReadOnly=ReadOnly;B->ResetEncounter();B->SetActorTickEnabled(Ticking);};
        B->SetActorTickEnabled(false);B->AbandonBattleToOperations();B->bProgressionReadOnly=false;
        bool Victory=true,Readiness=true,Training=true,Next=true,Defeat=true,Branches=true,Introductions=true,Legacy=true;
        int32 NextRoleDeltas=0,NextSupportDeltas=0;
        for(int32 Nation=0;Nation<2;++Nation)
        {
            const auto F=EFrontlineFaction(Nation);
            if(!Fixture(B,F,0,0)){Victory=false;continue;}
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;const auto Before=Snapshot(B);
            B->FinishBattle(0,TEXT("V53 isolated unlock regression"));const auto After=Snapshot(B);
            Victory&=B->Phase==EFrontlinePhase::Victory&&B->GetCurrentCampaign()->CompletedMissions==1
                &&B->GetCurrentCampaign()->SelectedMission==1&&Stage(B,0)==0&&SameRoster(Before,After);
            Readiness&=Before.Readiness==After.Readiness;
            Training&=Before.Technologies==After.Technologies&&Before.Year==After.Year&&Before.WeaponDateGate==After.WeaponDateGate;
            Introductions&=FrontlineUnlocksV53::IntroductionFirstOperation(B)==INDEX_NONE;
            Next&=B->ContinueCampaign()&&B->Phase==EFrontlinePhase::Briefing&&Stage(B,0)==1;
            const auto Prepared=Snapshot(B);NextRoleDeltas+=Prepared.Roles[0]!=Before.Roles[0];NextSupportDeltas+=Prepared.Supports[0]!=Before.Supports[0];
            Introductions&=FrontlineUnlocksV53::IntroductionFirstOperation(B)==1;
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;const auto BeforeLoss=Snapshot(B);
            B->FinishBattle(1,TEXT("V53 isolated defeat regression"));
            Defeat&=Stage(B,0)==1&&B->GetCurrentCampaign()->CompletedMissions==1&&SameRoster(BeforeLoss,Snapshot(B));

            for(int32 Fork:{2,6,12,17})for(int32 Pick=0;Pick<2;++Pick)
            {
                const int32 Id=Fork+Pick;if(!Fixture(B,F,Fork,Id)){Branches=false;continue;}
                B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;const auto Start=Snapshot(B);
                B->FinishBattle(0,TEXT("V53 isolated branch regression"));
                Branches&=B->GetCurrentCampaign()->CompletedMissions==Fork+2&&B->GetCurrentCampaign()->SelectedMission==Fork+2
                    &&Stage(B,0)==Id&&SameRoster(Start,Snapshot(B))&&B->GetEncounterCampaignMission()->Id==Id;
                // Returning to preparation must reveal the selected next order,
                // including an arrival date that lay in a bypassed operation.
                B->Phase=EFrontlinePhase::Briefing;
                Branches&=Stage(B,0)==Fork+2&&FrontlineCampaignV32::CurrentOperation(B)->Id==Fork+2;
                const auto PreparedBranch=Snapshot(B);NextRoleDeltas+=PreparedBranch.Roles[0]!=Start.Roles[0];NextSupportDeltas+=PreparedBranch.Supports[0]!=Start.Supports[0];
                Introductions&=FrontlineUnlocksV53::IntroductionFirstOperation(B)==Id+1;
                B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
                Branches&=Stage(B,0)==Fork+2&&B->GetEncounterCampaignMission()->Id==Fork+2;
            }

            // Old saves retain the earned roster and paid doctrine when an
            // earlier completed operation is replayed. No new schema is needed.
            if(!Fixture(B,F,15,0,true)){Legacy=false;continue;}
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;const auto Earned=Snapshot(B);
            const auto Paid=B->GetActiveResearch().TechnologyMask;
            B->FinishBattle(0,TEXT("V53 isolated legacy replay regression"));
            Legacy&=Stage(B,0)==15&&B->GetCurrentCampaign()->CompletedMissions==15&&!B->WasCampaignMissionAdvanced()
                &&SameRoster(Earned,Snapshot(B))&&B->GetActiveResearch().TechnologyMask==Paid;

            // Crossing into 1918 in the archive cannot apply MP18 availability
            // or next-year metadata while the 1917 outcome is still visible.
            if(!Fixture(B,F,9,9)){Training=false;continue;}
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;const auto BeforeYear=Snapshot(B);
            B->FinishBattle(0,TEXT("V53 isolated year-boundary regression"));const auto AfterYear=Snapshot(B);
            Training&=BeforeYear.Year==1917&&AfterYear.Year==1917&&BeforeYear.WeaponDateGate==AfterYear.WeaponDateGate
                &&BeforeYear.Technologies==AfterYear.Technologies&&SameRoster(BeforeYear,AfterYear);
        }
        Check(TEXT("v53_victory_keeps_both_armies_bound_infantry_support_armour_and_loadout"),Victory);
        Check(TEXT("v53_victory_does_not_reset_or_reveal_next_roster_cooldowns"),Readiness);
        Check(TEXT("v53_results_keep_paid_training_and_bound_historical_weapon_dates"),Training);
        Check(TEXT("v53_next_preparation_reveals_new_attachments_only_after_continue"),Next&&NextRoleDeltas>0&&NextSupportDeltas>0);
        Check(TEXT("v53_defeat_keeps_current_roster_without_advancing"),Defeat);
        Check(TEXT("v53_all_16_faction_branch_outcomes_keep_old_context_until_next_entry"),Branches);
        Check(TEXT("v53_intro_context_suppressed_in_results_and_covers_bypassed_arrival_dates"),Introductions);
        Check(TEXT("v53_legacy_replays_preserve_earned_units_and_paid_research"),Legacy);
    }

    static bool VictoryReview(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Id)
    {
        if(!IsolatedReview(B)||!B->GetCampaignMissionsForFaction(F).IsValidIndex(Id))return false;
        B->AbandonBattleToOperations();
        if(!Fixture(B,F,Id,Id))return false;
        // First-action screenshots use an ordinary new company's research.
        auto& R=B->CampaignRuns[0];R.Research={};R.bMobileGunV52=false;
        B->ResetEncounter();B->BeginBattle();if(B->Phase!=EFrontlinePhase::Fighting)return false;
        B->Elapsed=90;B->ShotsFired=8;B->FinishBattle(0,TEXT("The enemy has withdrawn. The position is secure."));
        return B->Phase==EFrontlinePhase::Victory&&B->GetEncounterCampaignMission()&&B->GetEncounterCampaignMission()->Id==Id;
    }
};

namespace FrontlineUnlocksV53
{
int32 Stage(const AWarfareFrontlineGameMode* B,int32 Team){return FFrontlineUnlocksV53Access::Stage(B,Team);}
int32 IntroductionFirstOperation(const AWarfareFrontlineGameMode* B)
{
    if(!B||B->Phase!=EFrontlinePhase::Briefing)return INDEX_NONE;
    const auto* M=B->GetSelectedCampaignMission();const auto* R=B->GetCurrentCampaign();if(!M||!R)return INDEX_NONE;
    if(R->StrategicRevisionV52!=52||(R->WonOperationMaskV52&(1u<<M->Id)))return M->Id;
    int32 Previous=INDEX_NONE;for(int32 I=0;I<M->Id;++I)if(R->WonOperationMaskV52&(1u<<I))Previous=I;
    return Previous+1;
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check){FFrontlineUnlocksV53Access::Checks(B,Check);}
bool PrepareVictoryReview(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Id){return FFrontlineUnlocksV53Access::VictoryReview(B,F,Id);}
}
