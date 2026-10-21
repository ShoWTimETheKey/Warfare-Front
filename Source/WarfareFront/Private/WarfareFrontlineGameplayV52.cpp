#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

namespace FrontlineGameplayV52
{
bool SelectableRole(EFrontlineRole R)
{return uint8(R)<uint8(EFrontlineRole::Count)&&R!=EFrontlineRole::MobileGun&&R!=EFrontlineRole::Signals;}
EFrontlineRole CanonicalRole(EFrontlineRole R)
{return R==EFrontlineRole::MobileGun?EFrontlineRole::Lewis:R==EFrontlineRole::Signals?EFrontlineRole::Officer:R;}
void NormalizeLegacyLoadout(FFrontlineCampaignProgress& R)
{
    const uint32 Old=R.RoleSelectionMask;
    if(Old&(1u<<9))R.RoleSelectionMask|=1u<<1;
    if(Old&(1u<<10))R.RoleSelectionMask|=1u<<4;
    R.RoleSelectionMask&=~((1u<<9)|(1u<<10));
}
bool MobileAvailable(const AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||(Team!=0&&Team!=1))return false;
    if(Team==1)return B->IsRoleUnlocked(EFrontlineRole::MobileGun,1);
    return B->HasTechnology(27)&&B->IsRoleUnlocked(EFrontlineRole::Lewis,0);
}
EFrontlineRole RecruitVariant(const AWarfareFrontlineGameMode* B,EFrontlineRole R,int32 Team)
{
    if(!B||Team!=0||R!=EFrontlineRole::Lewis)return R;
    const auto* Run=B->GetCurrentCampaign();
    return Run&&Run->bMobileGunV52&&MobileAvailable(B)?EFrontlineRole::MobileGun:R;
}
bool SetMobileGun(AWarfareFrontlineGameMode* B,bool Enabled)
{return FrontlineCampaignV32::SetMobileGunV52(B,Enabled);}
bool IsCommandLeader(const AWarfareFrontlineSoldier* U)
{
    return U&&U->Battle&&U->IsSectionLeader()&&U->IsActiveCombatant()&&U->Suppression<.85f
        &&U->Battle->Phase==EFrontlinePhase::Fighting&&U->Battle->BattleOutroWinner==INDEX_NONE;
}
const AWarfareFrontlineSoldier* CommandLeader(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!U->IsActiveCombatant()||U->IsSectionLeader())return nullptr;
    const AWarfareFrontlineSoldier* Best=nullptr;double BestDistance=MAX_dbl;float BestScale=0;
    for(const AWarfareFrontlineSoldier* L:U->Battle->Units)if(L!=U&&L&&L->Team==U->Team&&IsCommandLeader(L))
    {
        const float Range=FrontlineBalanceV50::OrderRange(L->IsCommissionedOfficer(),L->GetFaction()==EFrontlineFaction::German);
        const double Distance=FVector::DistSquared2D(L->Feet(),U->Feet());
        const float Scale=L->IsCommissionedOfficer()?1.50f:1.25f;
        if(Distance<=FMath::Square(Range)&&(Scale>BestScale||(Scale==BestScale&&Distance<BestDistance)))
        {Best=L;BestScale=Scale;BestDistance=Distance;}
    }
    return Best;
}
float CommandAccuracyScale(const AWarfareFrontlineSoldier* U)
{const auto* L=CommandLeader(U);return L?(L->IsCommissionedOfficer()?1.50f:1.25f):1.f;}
bool ObservationTrained(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!U->IsSectionLeader())return false;
    if(U->Team==0)return U->Battle->HasTechnology(10);
    const auto* M=U->Battle->GetEncounterCampaignMission();
    return M&&M->Id>=8;
}
float PioneerContactRange(const AWarfareFrontlineSoldier* U)
{
    if(FrontlineFieldV32::HasMineApproachV833(U))return 650.f;
    return FrontlineFieldV32::HasReachableHazardV52(U)?1750.f:FrontlineWeapons::ProfileFor(U).AdvanceContactCm;
}
void AppendTechnologyNodes(TArray<FFrontlineTechnologyNode>& N,bool G)
{
    N.Add({27,2,3,6,2,G?TEXT("随步兵前进的机枪"):TEXT("刘易斯突击编组"),
        TEXT("在整备中切换机枪编组：保留固定枪位的远射压制，或换用携行更快的伴随机枪。两者共用一个岗位和整备周期，现役分队保持原编制。"),-1,
        G?TEXT("German_27"):TEXT("British_27"),TEXT("诸兵种联络")});
    // Stable paid IDs and costs, with one card per branch/tier on both existing
    // pages. Later equipment now follows the same four subjects as basic drill.
    static constexpr int32 LateRows[4][3]={{16,23,25},{27,20,26},{17,18,24},{19,21,22}};
    static const TCHAR* BranchNames[]={TEXT("步兵整备"),TEXT("火力与观察"),TEXT("突击与指挥"),TEXT("炮兵联络")};
    for(int32 Branch=0;Branch<4;++Branch)for(int32 Row=0;Row<3;++Row)
    {auto& Node=N[LateRows[Branch][Row]];Node.Branch=Branch;Node.Tier=4+Row;}
    for(auto& Node:N)Node.BranchTitle=BranchNames[Node.Branch];
    // Removing unrelated cross-branch requirements keeps every old paid graph
    // valid; the remaining parent was already required by the shipped graph.
    for(auto& Node:N)Node.SecondaryPrerequisite=INDEX_NONE;
    N[18].Prerequisite=17;N[19].Prerequisite=10;N[22].Prerequisite=19;N[26].Prerequisite=20;
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile()){Check(TEXT("v52_gameplay_checks_require_volatile_profile"),false);return;}
    bool Layout=true,NewTools=true;
    for(int32 F=0;F<2;++F)
    {
        TSet<int32> Cells;
        const auto& Nodes=B->GetTechnologyNodesForFaction(EFrontlineFaction(F));Layout&=Nodes.Num()==28;
        for(const auto& N:Nodes)
        {
            const int32 Cell=N.Tier*4+N.Branch;
            Layout&=!Cells.Contains(Cell)&&N.Branch>=0&&N.Branch<4&&N.Tier>=0&&N.Tier<7
                &&((N.Id<16)==(N.Tier<4))&&N.Prerequisite<N.Id&&N.SecondaryPrerequisite==INDEX_NONE;
            Cells.Add(Cell);
        }
        for(int32 Mission=0;Mission<8;++Mission)
        {
            bool New=Mission==0||Mission==1||Mission==5||Mission==7;
            for(int32 Role=0;Role<int32(EFrontlineRole::Count);++Role)
                New|=SelectableRole(EFrontlineRole(Role))&&FrontlineCampaignV41::RoleArrival(EFrontlineFaction(F),EFrontlineRole(Role))==Mission;
            NewTools&=New;
        }
    }
    Check(TEXT("v59_twenty_eight_stable_technology_ids_fit_both_pages_without_overlap"),Layout);
    Check(TEXT("v59_first_eight_operations_each_introduce_a_real_role_or_support"),NewTools);
    FrontlineFieldV32::RunPioneerChecksV52(B,Check);
    FFrontlineCampaignProgress Old;Old.RoleSelectionMask=(1u<<9)|(1u<<10)|1u;Old.Research.TechnologyMask=(1u<<2);Old.Research.ResearchPoints=7;
    NormalizeLegacyLoadout(Old);const auto Once=Old.RoleSelectionMask;NormalizeLegacyLoadout(Old);
    Check(TEXT("v52_legacy_roles_fold_into_existing_slots_without_research_grants"),Once==((1u<<1)|(1u<<4)|1u)&&Old.RoleSelectionMask==Once
        &&Old.Research.TechnologyMask==(1u<<2)&&Old.Research.ResearchPoints==7&&!Old.bMobileGunV52);
    Check(TEXT("v52_attachments_are_not_extra_roster_slots"),!SelectableRole(EFrontlineRole::Signals)&&!SelectableRole(EFrontlineRole::MobileGun)
        &&CanonicalRole(EFrontlineRole::MobileGun)==EFrontlineRole::Lewis&&!B->IsRoleUnlocked(EFrontlineRole::Signals));
    Check(TEXT("v52_both_machinegun_doctrines_share_replenishment_duration"),FMath::IsNearlyEqual(B->RoleRecruitSeconds(EFrontlineRole::MobileGun),B->RoleRecruitSeconds(EFrontlineRole::Lewis)));
    const auto SavedSquads=B->Squads;const auto SavedSmoke=B->SmokeZones;
    const auto SavedUnits=B->Units;const auto SavedPhase=B->Phase;const int32 SavedOutro=B->BattleOutroWinner;
    auto& Research=const_cast<FFrontlineArmyProgress&>(B->GetActiveResearch());const auto SavedResearch=Research;
    TArray<AWarfareFrontlineSoldier*> Spawned;
    ON_SCOPE_EXIT{for(auto* U:Spawned)if(IsValid(U))U->Destroy();B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedSmoke;B->Phase=SavedPhase;B->BattleOutroWinner=SavedOutro;Research=SavedResearch;};
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;Research.TechnologyMask=0;
    auto Make=[&](EFrontlineRole R,int32 Team,int32 Id,const FVector& P,int32 Member=0)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,R,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->SetActorLocation(P+FVector(0,0,88));
        U->bMoving=false;U->Suppression=0;U->WeaponPhase=EFrontlineWeaponPhase::Ready;B->Units.Add(U);Spawned.Add(U);return U;
    };
    auto* Rifle=Make(EFrontlineRole::Rifle,0,95200,FVector(0,0,2400));
    auto* Enemy=Make(EFrontlineRole::Rifle,1,95201,FVector(3500,0,2400));
    auto* Leader=Make(EFrontlineRole::Officer,0,95202,FVector(0,500,2400));
    Check(TEXT("v52_command_fixture_actors_spawned"),Rifle&&Enemy&&Leader);if(!Rifle||!Enemy||!Leader)return;
    Leader->SetActorLocation(FVector(0,4000,2488));const float Base=B->BulletHitChance(Rifle,Enemy);
    Leader->SetActorLocation(FVector(0,500,2488));const float Command=B->BulletHitChance(Rifle,Enemy);
    Check(TEXT("v52_nco_star_tracks_real_nonstacking_accuracy_aura"),IsCommandLeader(Leader)&&CommandLeader(Rifle)==Leader&&CommandLeader(Enemy)==nullptr
        &&FMath::IsNearlyEqual(CommandAccuracyScale(Rifle),1.25f)&&Command>Base);
    Research.TechnologyMask=1u<<(B->GetPlayerFaction()==EFrontlineFaction::British?9:6);
    auto* Officer=Make(EFrontlineRole::Officer,0,95203,FVector(0,-500,2400));
    Check(TEXT("v52_commissioned_officer_improves_actual_fire_without_stacking"),Officer&&Officer->IsCommissionedOfficer()
        &&FMath::IsNearlyEqual(CommandAccuracyScale(Rifle),1.50f)&&B->BulletHitChance(Rifle,Enemy)>Command);
    if(Officer)
    {
        const int32 Dir=B->GetTeamDirection(0);const auto OldEnemy=Enemy->GetActorTransform(),OldOfficer=Officer->GetActorTransform();
        auto* Escort=Make(EFrontlineRole::Officer,0,Officer->SquadId,FVector(Dir*200.f,0,10000),1);
        Check(TEXT("v52_command_screen_escort_fixture_ready"),Escort!=nullptr);
        if(Escort)
        {
            const float Screen=FrontlineWeapons::ProfileFor(Rifle).AdvanceContactCm;
            Officer->SetActorLocation(FVector(0,0,10088));Enemy->SetActorLocation(FVector(Dir*(Screen+100.f)*B->EncounterVisibilityScale(),0,10088));
            FFrontlineSquad Section;Section.Id=Officer->SquadId;Section.Team=0;Section.Role=EFrontlineRole::Officer;Section.Members={Officer,Escort};
            Section.Trench=INDEX_NONE;Section.Destination=B->GetHomeTrenchIndex(1);Section.LastPassedTrench=B->GetHomeTrenchIndex(0);Section.Slot=1;Section.bAdvancing=true;B->Squads.Add(Section);
            const bool Contact=B->AcquireTarget(Officer,true)==Enemy&&B->AcquireTarget(Escort,true)==Enemy;
            const bool NoPistolRange=B->AcquireTarget(Officer,false)==nullptr&&B->BulletHitChance(Officer,Enemy)==0.f;
            const float BeforeX=Officer->Feet().X;Officer->Tick(.05f);
            Check(TEXT("v52_real_command_contact_stops_leader_behind_rifle_screen_without_pistol_range_gain"),Contact&&NoPistolRange
                &&FrontlineWeapons::AdvanceContactRangeFor(Escort)==Screen&&FrontlineWeapons::AdvanceContactRangeFor(Officer)==Screen+200.f
                &&FMath::IsNearlyEqual(Officer->Feet().X,BeforeX,.01f)&&!Officer->bMoving);
            B->Squads.RemoveAll([&](const FFrontlineSquad& S){return S.Id==Officer->SquadId;});Escort->Health=0;
        }
        Officer->SetActorTransform(OldOfficer);Enemy->SetActorTransform(OldEnemy);
    }
    if(Officer)Officer->Health=0;Leader->Suppression=.90f;
    Check(TEXT("v52_dead_or_overwhelmed_commanders_lose_stars_and_aura"),CommandLeader(Rifle)==nullptr&&CommandAccuracyScale(Rifle)==1.f);
    Leader->Suppression=0;Leader->SetActorLocation(FVector(0,4000,2488));
    Check(TEXT("v52_leaving_command_radius_immediately_removes_silver_star"),CommandLeader(Rifle)==nullptr&&FMath::IsNearlyEqual(B->BulletHitChance(Rifle,Enemy),Base));
    Check(TEXT("v52_signals_cannot_be_recruited_through_legacy_role_id"),!B->Recruit(EFrontlineRole::Signals,0)&&!B->Recruit(EFrontlineRole::Signals,1));
    auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());
    Check(TEXT("v52_variant_fixture_has_real_campaign"),Run!=nullptr);if(!Run)return;
    const auto SavedRun=*Run;const int32 SavedRecruitCommands=B->RecruitCommands;const auto SavedMessage=B->LastMessage;
    float SavedReady[2][int32(EFrontlineRole::Count)];
    for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<int32(EFrontlineRole::Count);++I)SavedReady[Team][I]=B->RecruitReadyAt[Team][I];
    ON_SCOPE_EXIT
    {
        *Run=SavedRun;B->RecruitCommands=SavedRecruitCommands;B->LastMessage=SavedMessage;
        for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<int32(EFrontlineRole::Count);++I)B->RecruitReadyAt[Team][I]=SavedReady[Team][I];
        for(AWarfareFrontlineSoldier* U:B->Units)if(U&&!SavedUnits.Contains(U))Spawned.AddUnique(U);
    };
    Run->CompletedMissions=FMath::Max(3,Run->CompletedMissions);Run->RoleSelectionMask=3;Run->bMobileGunV52=false;
    Research.TechnologyMask=(1u<<2)|(1u<<27);
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)B->RecruitReadyAt[0][I]=B->Elapsed;
    const bool FixedRecruited=B->Recruit(EFrontlineRole::Lewis);
    const int32 FixedId=FixedRecruited?B->Squads.Last().Id:INDEX_NONE;
    const float ReadyBeforeSwitch=B->RecruitReadyAt[0][int32(EFrontlineRole::Lewis)];
    B->Phase=EFrontlinePhase::Briefing;const bool Switched=SetMobileGun(B,true);B->Phase=EFrontlinePhase::Fighting;
    const auto* Fixed=B->FindSquad(FixedId);
    Check(TEXT("v52_actual_loadout_switch_preserves_deployed_gun_and_shared_cooldown"),FixedRecruited&&Switched&&Fixed&&Fixed->Role==EFrontlineRole::Lewis
        &&B->RecruitReadyAt[0][int32(EFrontlineRole::Lewis)]==ReadyBeforeSwitch
        &&B->RecruitCooldown(EFrontlineRole::MobileGun)>0&&!B->Recruit(EFrontlineRole::Lewis));
    const auto Loadout=FrontlineCampaignV32::GetLoadout(B);
    Check(TEXT("v52_mobile_selection_occupies_only_the_existing_machinegun_slot"),(Loadout.SelectedRoles&(1u<<1))!=0
        &&(Loadout.EligibleRoles&((1u<<9)|(1u<<10)))==0&&RecruitVariant(B,EFrontlineRole::Lewis)==EFrontlineRole::MobileGun);
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)B->RecruitReadyAt[0][I]=B->Elapsed;
    const bool MobileRecruited=B->Recruit(EFrontlineRole::Lewis);
    const auto* Mobile=MobileRecruited?&B->Squads.Last():nullptr;
    Check(TEXT("v52_canonical_recruit_spawns_real_mobile_squad_and_locks_all_infantry"),Mobile&&Mobile->Role==EFrontlineRole::MobileGun&&Mobile->LivingCount()==4
        &&B->RecruitCooldown(EFrontlineRole::Rifle)>0&&FMath::IsNearlyEqual(B->RecruitCooldown(EFrontlineRole::Lewis),B->RecruitCooldown(EFrontlineRole::MobileGun)));
}
}
