// Live production gameplay: this file never spawns, moves or damages an actor,
// seeds a combat counter, changes a random stream, or finishes a battle.
namespace FrontlineAchievementsV76
{
namespace
{
struct FFeasibilityV838
{
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    int32 Case=INDEX_NONE,First=INDEX_NONE,Original=0,PioneerAttempts=0;
    int32 HistoricalWins=0,HistoricalIncome=0,DispatchIncome=0,InfantryResearchCost=0,ArmourResearchCost=0,ResearchRemaining=0;
    uint32 BeforeEarned=0;
    float Started=0,NextAction=0,NextSample=0;
    float MaximumWorldDelta=0,MaximumWorldDilation=0;
    double WallStarted=0;
    bool StartedLive=false,CombinedDEV8311=false;
    TArray<TSharedPtr<FJsonValue>> Actions,Samples;
};
FFeasibilityV838 FeasibilityV838;
bool FeasibilitySafeV838(AWarfareFrontlineGameMode* B)
{
    FString User;
    return B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV838Review"))
            ||(FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(User)))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User)
        &&DisposableAccount()&&!FrontlineUltimateArchiveV8215::Active(B)&&!FrontlineDebugV37::HasCheatState(B);
}
const TCHAR* FeasibilityNameV838(int32 C)
{
    const TCHAR* Names[]={TEXT("uk_pioneers_veteran_op2"),TEXT("de_pioneers_veteran_op2"),
        TEXT("uk_roll_call_survivor_final19"),TEXT("de_roll_call_survivor_final19"),
        TEXT("uk_one_gun_veteran_op2"),TEXT("de_one_gun_veteran_op2")};
    return C>=0&&C<UE_ARRAY_COUNT(Names)?Names[C]:TEXT("invalid");
}
bool FeasibilityIntactV838(const AWarfareFrontlineGameMode* B,const FEncounter& E,EFrontlineRole Role,
    int32& Id,int32& Original,int32& Active)
{
    Id=E.FirstSectionsV84.Contains(int32(Role))?E.FirstSectionsV84.FindRef(int32(Role)):INDEX_NONE;
    Original=E.OriginalSectionSizesV84.FindRef(Id);Active=0;
    const auto* Q=E.FirstSectionsV84.Contains(int32(Role))?B->FindSquad(Id):nullptr;
    if(Q&&Q->Team==0&&Q->Role==Role)for(const auto& U:Q->Members)
        Active+=U.IsValid()&&U->Team==0&&U->bFieldEntered&&U->IsActiveCombatant();
    return Q&&Original>=2&&Q->Members.Num()==Original&&Active==Original;
}
TSharedRef<FJsonObject> FeasibilitySnapshotV838(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();const auto& E=State(B);
    int32 Pioneer=INDEX_NONE,PioneerOriginal=0,PioneerActive=0,Rifle=INDEX_NONE,RifleOriginal=0,RifleActive=0;
    const bool PIntact=FeasibilityIntactV838(B,E,EFrontlineRole::Pioneer,Pioneer,PioneerOriginal,PioneerActive);
    const bool RIntact=FeasibilityIntactV838(B,E,EFrontlineRole::Rifle,Rifle,RifleOriginal,RifleActive);
    const auto* Personal=E.PioneerClearancesV837.Find(Pioneer);
    J->SetNumberField(TEXT("elapsed"),B->Elapsed);J->SetNumberField(TEXT("phase"),int32(B->Phase));
    J->SetNumberField(TEXT("difficulty"),int32(B->GetCampaignDifficulty()));J->SetNumberField(TEXT("nation"),int32(B->GetPlayerFaction()));
    const auto* Mission=B->GetEncounterCampaignMission();J->SetNumberField(TEXT("bound_operation"),Mission?Mission->Id:INDEX_NONE);
    J->SetNumberField(TEXT("infantry_lost"),B->CombatRecordV65[0].InfantryLost);
    J->SetNumberField(TEXT("infantry_alive"),B->LivingSoldiers(0));J->SetNumberField(TEXT("enemy_infantry_alive"),B->LivingSoldiers(1));
    J->SetNumberField(TEXT("player_morale"),B->Morale[0]);J->SetNumberField(TEXT("enemy_morale"),B->Morale[1]);
    J->SetNumberField(TEXT("first_pioneer_id"),Pioneer);J->SetNumberField(TEXT("first_pioneer_original"),PioneerOriginal);
    J->SetNumberField(TEXT("first_pioneer_active"),PioneerActive);J->SetBoolField(TEXT("first_pioneer_intact"),PIntact);
    J->SetNumberField(TEXT("first_pioneer_personal_clearances"),Personal?Personal->Num():0);
    J->SetNumberField(TEXT("distinct_player_clearances"),E.Obstacles.Num());
    J->SetNumberField(TEXT("first_rifle_id"),Rifle);J->SetNumberField(TEXT("first_rifle_original"),RifleOriginal);
    J->SetNumberField(TEXT("first_rifle_active"),RifleActive);J->SetBoolField(TEXT("first_rifle_intact"),RIntact);
    TArray<TSharedPtr<FJsonValue>> FirstRifleMembers;
    if(const auto* Q=B->FindSquad(Rifle))for(const auto& W:Q->Members)if(const auto* U=W.Get())
    {
        auto M=MakeShared<FJsonObject>();const bool Dead=!U->IsAlive();const auto& Source=U->FatalSourceV65;
        M->SetNumberField(TEXT("member"),U->MemberIndex);M->SetBoolField(TEXT("alive"),U->IsAlive());M->SetBoolField(TEXT("active"),U->IsActiveCombatant());M->SetBoolField(TEXT("entered"),U->bFieldEntered);
        M->SetStringField(TEXT("feet"),U->Feet().ToString());M->SetNumberField(TEXT("fatal_kind"),Dead?int32(U->GetDeathState().Kind):INDEX_NONE);
        M->SetNumberField(TEXT("fatal_source_team"),Dead?Source.Team:INDEX_NONE);M->SetNumberField(TEXT("fatal_source_support"),Dead?Source.Support:INDEX_NONE);
        M->SetNumberField(TEXT("fatal_source_vehicle"),Dead?Source.VehicleSerial:INDEX_NONE);M->SetNumberField(TEXT("fatal_source_squad"),Dead?Source.SquadId:INDEX_NONE);
        FirstRifleMembers.Add(MakeShared<FJsonValueObject>(M));
    }
    J->SetArrayField(TEXT("first_rifle_members"),FirstRifleMembers);
    J->SetBoolField(TEXT("friendly_infantry_loss_recorded"),B->CombatRecordV65[0].DetailsV839.bRecorded);
    J->SetNumberField(TEXT("friendly_infantry_losses"),B->CombatRecordV65[0].DetailsV839.bRecorded?B->CombatRecordV65[0].DetailsV839.FriendlyFire.Infantry:INDEX_NONE);
    int32 Qualified=INDEX_NONE;J->SetBoolField(TEXT("hearts_live_predicate"),PioneerWorkCompletedV842(B,E,&Qualified));
    J->SetNumberField(TEXT("qualified_pioneer_section"),Qualified);
    J->SetStringField(TEXT("first_pioneer_fields_scope"),TEXT("Diagnostic original-section history only; revised goal may be earned by any actual clearing section."));
    const auto Returned=QuietReturnV8311(B,E);
    J->SetBoolField(TEXT("quiet_live_predicate"),Returned.Squad!=INDEX_NONE);
    J->SetNumberField(TEXT("quiet_qualified_squad"),Returned.Squad);
    J->SetNumberField(TEXT("quiet_original_members"),Returned.Original);
    J->SetNumberField(TEXT("quiet_returned_members"),Returned.Returned);
    J->SetNumberField(TEXT("quiet_returned_members_shots"),Returned.ReturnedShots);
    J->SetNumberField(TEXT("quiet_squad_kill_credit"),Returned.Kills);
    J->SetNumberField(TEXT("quiet_returned_categories_v844"),Returned.ClassesV844);
    J->SetNumberField(TEXT("quiet_required_categories_v844"),RequiredReturnClassesV844);
    J->SetNumberField(TEXT("quiet_returned_role_mask_v844"),Returned.RoleMaskV844);
    J->SetStringField(TEXT("quiet_rule_v844"),TEXT("Highest-difficulty final victory with entered survivors from three distinct infantry section classes. Weapon variants and rifle assistants do not create extra classes. No first-section, personal-shot or kill quota."));
    J->SetBoolField(TEXT("eligible"),E.Eligible);J->SetBoolField(TEXT("settlement_captured"),E.SettlementCaptured);
    J->SetBoolField(TEXT("has_cheat_state"),FrontlineDebugV37::HasCheatState(B));
    J->SetNumberField(TEXT("shots_fired"),B->ShotsFired);J->SetNumberField(TEXT("recruit_commands"),B->RecruitCommands);
    J->SetNumberField(TEXT("advance_commands"),B->AdvanceCommands);J->SetNumberField(TEXT("support_commands"),B->ArtilleryCommands+B->MortarCommands+B->SmokeCommands+B->RallyCommands);
    int32 BestGun=INDEX_NONE,BestGunKills=0;
    for(const auto& Section:E.GunSectionKillsV851)if(Section.Value>BestGunKills){BestGun=Section.Key;BestGunKills=Section.Value;}
    J->SetNumberField(TEXT("one_gun_best_section"),BestGun);J->SetNumberField(TEXT("one_gun_best_section_fatal_receipts"),BestGunKills);
    J->SetBoolField(TEXT("one_gun_live_predicate"),ChallengeConditionV84415(B,3));
    TArray<TSharedPtr<FJsonValue>> Squads;
    for(const auto& Q:B->Squads)if(Q.Team==0)
    {
        auto S=MakeShared<FJsonObject>();S->SetNumberField(TEXT("id"),Q.Id);S->SetNumberField(TEXT("role"),int32(Q.Role));
        S->SetNumberField(TEXT("living"),Q.LivingCount());S->SetNumberField(TEXT("trench"),Q.Trench);
        S->SetBoolField(TEXT("advancing"),Q.bAdvancing);S->SetBoolField(TEXT("hold"),Q.bHoldPosition);
        S->SetNumberField(TEXT("centre_x"),Q.Centre().X);S->SetNumberField(TEXT("kills"),Q.KillsV65.Infantry);
        const int32 Original=E.OriginalSectionSizesV84.FindRef(Q.Id);int32 Entered=0,ReturnedShots=0;
        for(const auto& U:Q.Members)if(U.IsValid()&&U->Team==0&&U->IsAlive()
            &&(U->bFieldEntered||U->IsActiveCombatant()))
        {++Entered;ReturnedShots+=U->ShotsDischarged;}
        S->SetNumberField(TEXT("original_members"),Original);S->SetNumberField(TEXT("entered_active_members"),Entered);
        S->SetNumberField(TEXT("returned_members_shots"),ReturnedShots);S->SetNumberField(TEXT("section_armour_kills"),Q.KillsV65.Armour);
        S->SetBoolField(TEXT("entered_survival_qualified"),Original>=1&&Q.Members.Num()==Original&&Entered>=1);
        if(Q.Role==EFrontlineRole::Lewis)
        {
            const AWarfareFrontlineSoldier* Gunner=nullptr;
            for(const auto& U:Q.Members)if(U.IsValid()&&U->IsMachineGunner())
            {if(!Gunner)Gunner=U.Get();if(U->IsActiveCombatant()){Gunner=U.Get();break;}}
            if(Gunner)
            {
                S->SetNumberField(TEXT("machinegunner_member"),Gunner->MemberIndex);
                S->SetNumberField(TEXT("machinegunner_shots"),Gunner->ShotsDischarged);
                S->SetBoolField(TEXT("machinegunner_entered_active"),Gunner->bFieldEntered&&Gunner->IsActiveCombatant());
            }
        }
        if(Q.Role==EFrontlineRole::Pioneer)
        {
            const auto* Work=E.PioneerClearancesV837.Find(Q.Id);
            S->SetNumberField(TEXT("personal_clearances"),Work?Work->Num():0);
            S->SetBoolField(TEXT("pioneer_work_qualified"),Work&&Work->Num()>=2&&Original>=2);
        }
        Squads.Add(MakeShared<FJsonValueObject>(S));
    }
    J->SetArrayField(TEXT("player_squads"),Squads);J->SetObjectField(TEXT("field"),FrontlineFieldV32::Review(B));
    TArray<TSharedPtr<FJsonValue>> Armour;int32 LiveEnemy=0;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const AFrontlineTank* V:Fleet->GetVehicles())if(IsValid(V))
    {
        auto A=MakeShared<FJsonObject>();A->SetNumberField(TEXT("serial"),V->Serial);A->SetNumberField(TEXT("team"),V->Team);
        A->SetNumberField(TEXT("model"),int32(V->Type));A->SetBoolField(TEXT("entered"),V->HasEnteredField());A->SetBoolField(TEXT("operational"),V->IsOperational());
        A->SetNumberField(TEXT("hull"),V->GetHullIntegrity());A->SetNumberField(TEXT("maximum_hull"),V->GetMaximumHullIntegrity());
        A->SetNumberField(TEXT("machinegun_shots"),V->MachinegunShots);A->SetNumberField(TEXT("cannon_shots"),V->CannonShots);
        A->SetNumberField(TEXT("infantry_kills"),V->KillsV65.Infantry);A->SetNumberField(TEXT("armour_kills"),V->KillsV65.Armour);
        A->SetNumberField(TEXT("x"),V->GetActorLocation().X);A->SetNumberField(TEXT("y"),V->GetActorLocation().Y);
        LiveEnemy+=V->Team==1&&V->IsOperational()&&V->HasEnteredField();Armour.Add(MakeShared<FJsonValueObject>(A));
    }
    J->SetNumberField(TEXT("enemy_armour_living_entered"),LiveEnemy);J->SetArrayField(TEXT("actual_armour"),Armour);
    return J;
}
void FeasibilityActionV838(AWarfareFrontlineGameMode* B,const FString& Action,int32 Subject=INDEX_NONE)
{
    auto& R=FeasibilityV838;if(R.Actions.Num()>=160)return;
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("elapsed"),B->Elapsed);J->SetStringField(TEXT("action"),Action);
    J->SetNumberField(TEXT("subject"),Subject);R.Actions.Add(MakeShared<FJsonValueObject>(J));
}
bool PrepareOneGunFeasibilityV851(AWarfareFrontlineGameMode* B,int32 CaseIndex)
{
 if(!FeasibilitySafeV838(B)||(CaseIndex!=4&&CaseIndex!=5))return false;
 B->AbandonBattleToOperations();const auto Nation=CaseIndex==4?EFrontlineFaction::British:EFrontlineFaction::German;
 if(!B->StartNewCampaign(Nation,EFrontlineDifficulty::Veteran))return false;
 auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!Run)return false;
 // Only two earlier operation wins form the historical fixture. The live
 // operation starts empty; ordinary shared queues and enemy AI are untouched.
 Run->SelectedMission=Run->CompletedMissions=2;Run->WonOperationMaskV52=3;Run->BypassedOperationMaskV52=0;
 Run->DispatchResolvedMaskV52=Run->DispatchOrderMaskV52=Run->AcknowledgedEventMask=3;Run->DispatchChoiceMaskV52=0;
 Run->Research.TechnologyMask=Run->ArmourResearchMaskV63=0;Run->Research.ResearchPoints=8;
 Run->RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis));
 Run->SupportSelectionMask=(1u<<int32(EFrontlineSupport::Smoke))|(1u<<int32(EFrontlineSupport::Mortar))|(1u<<int32(EFrontlineSupport::Artillery));
 Run->ArmourSelectionMaskV63=0;
 if(!B->SelectCampaignMission(2))return false;
 const int32 Before=B->GetResearchPoints();if(!FrontlineCampaignV52::ResolveDispatch(B,1))return false;
 const int32 Dispatch=B->GetResearchPoints()-Before;int32 Cost=0;
 for(const int32 Technology:{0,2,3,4,5})
 {const int32 Bank=B->GetResearchPoints();if(!B->PurchaseTechnology(Technology))return false;Cost+=Bank-B->GetResearchPoints();}
 FeasibilityV838=FFeasibilityV838();auto& R=FeasibilityV838;R.Battle=B;R.Case=CaseIndex;
 R.HistoricalWins=2;R.HistoricalIncome=8;R.DispatchIncome=Dispatch;R.InfantryResearchCost=Cost;R.ResearchRemaining=B->GetResearchPoints();
 if(R.ResearchRemaining!=8+Dispatch-Cost||R.ResearchRemaining<0)return false;
 Load();R.BeforeEarned=Account.EarnedMask;IsolatedAwardBattle=B;return true;
}
bool TickOneGunFeasibilityV851(AWarfareFrontlineGameMode* B,TSharedPtr<FJsonObject>& CompletedReport)
{
 auto& R=FeasibilityV838;const bool Safe=FeasibilitySafeV838(B);
 constexpr double WallLimit=310.;constexpr float SimulationLimit=450.f;
 if(Safe&&B->Phase==EFrontlinePhase::Fighting&&B->Elapsed-R.Started<SimulationLimit&&FPlatformTime::Seconds()-R.WallStarted<WallLimit)
 {
  if(B->Elapsed>=R.NextSample){R.Samples.Add(MakeShared<FJsonValueObject>(FeasibilitySnapshotV838(B)));R.NextSample=B->Elapsed+10.f;}
  if(B->Elapsed<R.NextAction)return false;R.NextAction=B->Elapsed+.5f;
  const bool Deed=ChallengeConditionV84415(B,3);
  int32 Guns=0,Rifles=0;for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0)
  {
   if(Q.Role==EFrontlineRole::Lewis||Q.Role==EFrontlineRole::MobileGun)
    for(const auto& U:Q.Members)if(U.IsValid()&&U->IsAlive()&&U->IsMachineGunner())
    {++Guns;break;} // Include incoming gunners; an assistant-only remnant cannot replace a gun.
   Rifles+=Q.Role==EFrontlineRole::Rifle;
  }
  if(Guns<(Deed?2:1)&&B->Recruit(EFrontlineRole::Lewis))FeasibilityActionV838(B,Deed?TEXT("normal_second_gun_reinforcement_after_personal_deed"):TEXT("recruit_or_replace_actual_machinegun_section"),B->Squads.Last().Id);
  if(Rifles<(Deed?4:2)&&B->Recruit(EFrontlineRole::Rifle))FeasibilityActionV838(B,Deed?TEXT("normal_rifle_reinforcement_after_personal_deed"):TEXT("recruit_two_rifle_screen_sections"),B->Squads.Last().Id);
  TSet<AWarfareFrontlineSoldier*> Observed;
  for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0)
  {
   bool Contact=false;
   for(const auto& U:Q.Members)if(U.IsValid()&&U->bFieldEntered&&U->IsActiveCombatant())
    if(auto* Target=B->AcquireTarget(U.Get(),false))
    {Contact=true;Observed.Add(Target);}
   if(!Contact&&B->Trenches.IsValidIndex(Q.Trench)&&!Q.bAdvancing&&B->AdvanceSquad(Q.Trench,Q.Id))
    FeasibilityActionV838(B,TEXT("public_advance_only_without_actual_section_contact"),Q.Id);
  }
  // Normal combined arms may relieve a visible concentration while infantry
  // are engaged. The personal gun deed never prohibits support or forces a
  // fighting rifle screen out of its cover.
  for(const auto Support:{EFrontlineSupport::Artillery,EFrontlineSupport::Mortar})
  {
   if(!B->SupportBlockReason(Support).IsEmpty())continue;
   AWarfareFrontlineSoldier* Target=nullptr;int32 Best=0;
   for(AWarfareFrontlineSoldier* U:Observed)if(U&&U->Team==1&&U->bFieldEntered&&U->IsActiveCombatant()&&B->IsFireSupportAreaSafe(U->Feet(),Support,0))
   {
    int32 Cluster=0;for(AWarfareFrontlineSoldier* V:B->Units)Cluster+=V&&V->Team==1&&V->bFieldEntered&&V->IsActiveCombatant()&&FVector::DistSquared2D(U->Feet(),V->Feet())<FMath::Square(1100.f);
    if(!Deed&&Cluster<5)continue;
    if(Cluster>Best){Best=Cluster;Target=U;}
   }
   if(Target&&B->RequestSupport(Support,Target->Feet()))FeasibilityActionV838(B,Deed?TEXT("normal_support_after_one_section_deed"):TEXT("normal_support_relieve_observed_front_pressure"),int32(Support));
  }
  return false;
 }
 auto J=FeasibilitySnapshotV838(B);const bool Victory=B->Phase==EFrontlinePhase::Victory;
 const bool Predicate=ChallengeConditionV84415(B,3);FAccount Disk;const bool DiskReadable=ReadFile(AccountPath(),Disk);
 const bool Saved=DiskReadable&&(Disk.EarnedMask&(1u<<3));
 J->SetStringField(TEXT("case"),FeasibilityNameV838(R.Case));J->SetNumberField(TEXT("case_index"),R.Case);
 J->SetBoolField(TEXT("success"),Safe&&Victory&&Predicate&&State(B).Eligible&&State(B).SettlementCaptured&&Saved);
 J->SetBoolField(TEXT("actual_objective_victory"),Victory);J->SetBoolField(TEXT("actual_account_readable"),DiskReadable);
 J->SetBoolField(TEXT("actual_account_has_target"),Saved);J->SetBoolField(TEXT("target_previously_earned"),(R.BeforeEarned&(1u<<3))!=0);
 J->SetBoolField(TEXT("target_newly_earned"),Saved&&!(R.BeforeEarned&(1u<<3)));
 J->SetStringField(TEXT("outcome"),!Safe?TEXT("review_guard_failed"):Victory?TEXT("victory"):B->Phase==EFrontlinePhase::Defeat?TEXT("defeat"):TEXT("bounded_timeout"));
 J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R.WallStarted);J->SetNumberField(TEXT("wall_limit_seconds"),WallLimit);
 J->SetNumberField(TEXT("host_elapsed"),B->Elapsed-R.Started);J->SetNumberField(TEXT("simulation_limit_seconds"),SimulationLimit);
 J->SetNumberField(TEXT("historical_prior_wins"),R.HistoricalWins);J->SetNumberField(TEXT("historical_research_income"),R.HistoricalIncome);
 J->SetNumberField(TEXT("actual_dispatch_income"),R.DispatchIncome);J->SetNumberField(TEXT("actual_research_cost"),R.InfantryResearchCost);
 J->SetNumberField(TEXT("research_remaining"),R.ResearchRemaining);J->SetArrayField(TEXT("accepted_public_actions"),R.Actions);J->SetArrayField(TEXT("samples"),R.Samples);
 J->SetStringField(TEXT("scope"),TEXT("One bounded normal-operation2 policy for this army on Veteran. Two earlier wins are a historical prerequisite fixture. Infantry enters from the real edge through shared cooldowns. Every section holds an actual member-acquired contact, and uses public advance only when no member has one. A live incoming or entered gunner reserves its section; assistant-only remnants do not prevent replacement. Begin with one gun and two rifle screening sections; after12 actual enemy deaths from one section, normal reinforcements may expand to two guns and four rifle sections through unchanged recruitment rules. No unavailable armour is unlocked. Normal AI/fire/hit rolls/casualties/morale. Normal safely targeted artillery may relieve an observed enemy concentration of at least5 during infantry contact before the deed; after the deed it may target any observed safe contact. No repositioned actors, fabricated casualty counters, immunity, forced outcome or claim of all-map human enjoyment."));
 R.StartedLive=false;IsolatedAwardBattle.Reset();CompletedReport=J;return true;
}
}
bool PrepareFeasibilityCaseV838(AWarfareFrontlineGameMode* B,int32 CaseIndex)
{
    if(CaseIndex==4||CaseIndex==5)return PrepareOneGunFeasibilityV851(B,CaseIndex);
    if(!FeasibilitySafeV838(B)||CaseIndex<0||CaseIndex>=4)return false;
    B->AbandonBattleToOperations();
    const auto Nation=CaseIndex%2?EFrontlineFaction::German:EFrontlineFaction::British;
    const bool Quiet=CaseIndex>=2;const int32 Mission=Quiet?19:2;
    const bool CombinedDE=FParse::Param(FCommandLine::Get(),TEXT("WFV8311CombinedDE"));
    if(CombinedDE)
    {
        FString ReviewDir;
        if(CaseIndex!=3||!FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))
            ||!FrontlineUltimateArchiveV8215::ReviewUserDir(ReviewDir))return false;
    }
    if(!B->StartNewCampaign(Nation,Quiet?EFrontlineDifficulty::Survivor:EFrontlineDifficulty::Veteran))return false;
    auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!C)return false;
    // Quiet: choose2/6/12/18 at the four forks, never both alternatives.
    // Engineers: only operations0/1 are historical; operation2 is current.
    // Earlier dispatches accepted their loan (zero research income); no loan
    // remains active from an earlier battle. No present-battle evidence is set.
    const uint32 EarlierBypasses=(1u<<3)|(1u<<7)|(1u<<13);
    C->CompletedMissions=Quiet?19:2;
    C->WonOperationMaskV52=Quiet?((((1u<<17)-1u)&~EarlierBypasses)|(1u<<18)):3u;
    C->BypassedOperationMaskV52=Quiet?(EarlierBypasses|(1u<<17)):0u;
    uint32 PastDispatches=0;for(int32 Id=0;Id<20;++Id)
        if((C->WonOperationMaskV52&(1u<<Id))&&(Id<8||FrontlineCampaignV32::EventForMission(Nation,Id).Id!=INDEX_NONE))PastDispatches|=1u<<Id;
    C->DispatchResolvedMaskV52=C->DispatchOrderMaskV52=C->AcknowledgedEventMask=PastDispatches;
    C->DispatchChoiceMaskV52=0;
    C->Research.TechnologyMask=C->ArmourResearchMaskV63=0;
    const int32 HistoricalWins=FrontlineCampaignV32::BitCount(C->WonOperationMaskV52);
    const int32 HistoricalIncome=2+3*HistoricalWins; // actual starting grant + actual victory reward
    C->Research.ResearchPoints=HistoricalIncome;
    C->RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis))
        |(1u<<int32(EFrontlineRole::Officer))|(1u<<int32(EFrontlineRole::Pioneer))|(1u<<int32(EFrontlineRole::Marksman));
    C->SupportSelectionMask=(1u<<int32(EFrontlineSupport::Artillery))|(1u<<int32(EFrontlineSupport::Smoke))
        |(1u<<int32(EFrontlineSupport::Rally))|(1u<<int32(EFrontlineSupport::Mortar))
        |(1u<<int32(EFrontlineSupport::CreepingBarrage))|(1u<<int32(EFrontlineSupport::AirRecon));
    C->ArmourSelectionMaskV63=!Quiet?0u:Nation==EFrontlineFaction::British?
        (1u<<int32(EFrontlineVehicle::MarkVMale))|(1u<<int32(EFrontlineVehicle::Whippet)):
        (1u<<int32(EFrontlineVehicle::A7V))|(1u<<int32(EFrontlineVehicle::BeuteMarkIV));
    if(!B->SelectCampaignMission(Mission))return false;
    // Buy a concrete affordable policy loadout through production prerequisite
    // and debit APIs. No unlimited research, direct researched bits or arbitrary
    // point grant. The bank above is solely the explicit historical fixture.
    const int32 BeforeDispatch=B->GetResearchPoints();
    if(!FrontlineCampaignV52::ResolveDispatch(B,1))return false;
    const int32 DispatchIncome=B->GetResearchPoints()-BeforeDispatch;
    int32 InfantryCost=0,ArmourCost=0;
    const TArray<int32> Infantry=Quiet?TArray<int32>{0,1,2,3,4,5,10,16,17,19}:TArray<int32>{0,2,3,4,5,17};
    for(const int32 Id:Infantry)
    {
        const int32 Before=B->GetResearchPoints();if(!B->PurchaseTechnology(Id))return false;
        InfantryCost+=Before-B->GetResearchPoints();
    }
    const TArray<int32> Armour=Quiet?TArray<int32>{0,1,2,3,9,10,11}:TArray<int32>{};
    for(const int32 Id:Armour)
    {
        const int32 Before=B->GetResearchPoints();if(!FrontlineArmourV63::BuyResearch(B,Id))return false;
        ArmourCost+=Before-B->GetResearchPoints();
    }
    if(Quiet&&Nation==EFrontlineFaction::British)
    {const int32 Before=B->GetResearchPoints();if(!FrontlineArmourV63::BuyResearch(B,5))return false;ArmourCost+=Before-B->GetResearchPoints();}
    if(CombinedDE)
    {
        if(B->GetResearchPoints()!=12)return false;
        for(const int32 Id:{6,7,8,24,25})
        {
            const int32 Before=B->GetResearchPoints();if(!B->PurchaseTechnology(Id))return false;
            InfantryCost+=Before-B->GetResearchPoints();
        }
        if(B->GetResearchPoints()!=0||!FrontlineCampaignV32::ToggleRole(B,EFrontlineRole::Marksman)
            ||!FrontlineCampaignV32::ToggleRole(B,EFrontlineRole::AntiTankRifle)
            ||!FrontlineCampaignV32::ToggleSupport(B,EFrontlineSupport::AntiTank))return false;
        const auto Loadout=FrontlineCampaignV32::GetLoadout(B);
        const uint32 Roles=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis))
            |(1u<<int32(EFrontlineRole::Officer))|(1u<<int32(EFrontlineRole::Pioneer))|(1u<<int32(EFrontlineRole::AntiTankRifle));
        if(Loadout.SelectedRoles!=Roles||FrontlineCampaignV32::BitCount(Loadout.SelectedSupports)!=6
            ||!(Loadout.SelectedSupports&(1u<<int32(EFrontlineSupport::AntiTank))))return false;
    }
    FeasibilityV838=FFeasibilityV838();FeasibilityV838.Battle=B;FeasibilityV838.Case=CaseIndex;
    FeasibilityV838.HistoricalWins=HistoricalWins;FeasibilityV838.HistoricalIncome=HistoricalIncome;
    FeasibilityV838.DispatchIncome=DispatchIncome;FeasibilityV838.InfantryResearchCost=InfantryCost;FeasibilityV838.ArmourResearchCost=ArmourCost;
    FeasibilityV838.ResearchRemaining=B->GetResearchPoints();FeasibilityV838.CombinedDEV8311=CombinedDE;
    if(InfantryCost!=(CombinedDE?32:Quiet?20:9)||ArmourCost!=(Quiet?(Nation==EFrontlineFaction::British?22:18):0)
        ||FeasibilityV838.ResearchRemaining!=HistoricalIncome+DispatchIncome-InfantryCost-ArmourCost)return false;
    Load();FeasibilityV838.BeforeEarned=Account.EarnedMask;
    // The normal BeginEncounter/Finish pipeline remains authoritative. Its
    // isolated account destination is guarded by all four flags above.
    IsolatedAwardBattle=B;return true;
}
bool StartFeasibilityCaseV838(AWarfareFrontlineGameMode* B,int32 CaseIndex)
{
    auto& R=FeasibilityV838;const auto* C=B?B->GetCurrentCampaign():nullptr;
    const auto* M=B?B->GetEncounterCampaignMission():nullptr;const bool Quiet=CaseIndex>=2&&CaseIndex<4;
    if(!FeasibilitySafeV838(B)||R.Battle.Get()!=B||R.Case!=CaseIndex||!C||!M||M->Id!=(Quiet?19:2)
        ||C->Faction!=(CaseIndex%2?EFrontlineFaction::German:EFrontlineFaction::British)
        ||C->Difficulty!=(Quiet?EFrontlineDifficulty::Survivor:EFrontlineDifficulty::Veteran)
        ||B->Phase!=EFrontlinePhase::Fighting||!B->Units.IsEmpty()||!B->Squads.IsEmpty()
        ||B->Elapsed>2.f||!State(B).Eligible||State(B).SettlementCaptured)return false;
    R.Started=B->Elapsed;R.WallStarted=FPlatformTime::Seconds();R.NextAction=R.Started;R.NextSample=R.Started;
    R.StartedLive=true;return true;
}
bool TickFeasibilityCaseV838(AWarfareFrontlineGameMode* B,TSharedPtr<FJsonObject>& CompletedReport)
{
    CompletedReport.Reset();auto& R=FeasibilityV838;
    if(!R.StartedLive||R.Battle.Get()!=B||!B)return false;
    if(R.Case==4||R.Case==5)return TickOneGunFeasibilityV851(B,CompletedReport);
    if(B->GetWorld()){R.MaximumWorldDelta=FMath::Max(R.MaximumWorldDelta,B->GetWorld()->GetDeltaSeconds());R.MaximumWorldDilation=FMath::Max(R.MaximumWorldDilation,UGameplayStatics::GetGlobalTimeDilation(B));}
    const bool Safe=FeasibilitySafeV838(B),Quiet=R.Case>=2;
    const double WallLimit=450.;
    const float SimulationLimit=Quiet?550.f:600.f;
    if(Safe&&B->Phase==EFrontlinePhase::Fighting&&B->Elapsed-R.Started<SimulationLimit&&FPlatformTime::Seconds()-R.WallStarted<WallLimit)
    {
        if(B->Elapsed>=R.NextSample){R.Samples.Add(MakeShared<FJsonValueObject>(FeasibilitySnapshotV838(B)));R.NextSample=B->Elapsed+15.f;}
        if(B->Elapsed<R.NextAction)return false;R.NextAction=B->Elapsed+.5f;
        const EFrontlineRole FirstRole=Quiet?EFrontlineRole::Rifle:EFrontlineRole::Pioneer;
        if(R.First==INDEX_NONE&&B->Recruit(FirstRole))
        {
            R.First=State(B).FirstSectionsV84.FindRef(int32(FirstRole));R.Original=State(B).OriginalSectionSizesV84.FindRef(R.First);
            R.PioneerAttempts+=!Quiet;
            FeasibilityActionV838(B,TEXT("recruit_original_goal_section"),R.First);
        }
        if(!Quiet&&R.First==INDEX_NONE)return false;
        if(!Quiet)
        {
            // An early dense-wire operation has no heavy-armour dependency.
            // Engineers take the short first crossing with rifle/gun cover,
            // before shellfire or enemy engineers remove every work opportunity.
            int32 Guns=0,Pioneers=0,Rifles=0;for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0)
            {Guns+=Q.Role==EFrontlineRole::Lewis;Pioneers+=Q.Role==EFrontlineRole::Pioneer;Rifles+=Q.Role==EFrontlineRole::Rifle;}
            if(Guns<1&&B->Recruit(EFrontlineRole::Lewis))FeasibilityActionV838(B,TEXT("recruit_cover_section"),int32(EFrontlineRole::Lewis));
            if(Rifles<3&&B->Recruit(EFrontlineRole::Rifle))FeasibilityActionV838(B,TEXT("recruit_cover_section"),int32(EFrontlineRole::Rifle));
            if(Pioneers<2&&R.PioneerAttempts<4&&!PioneerWorkCompletedV842(B,State(B))&&B->Recruit(EFrontlineRole::Pioneer))
            {++R.PioneerAttempts;FeasibilityActionV838(B,TEXT("recruit_replacement_engineers"),int32(EFrontlineRole::Pioneer));}
        }
        if(Quiet)
        {
            // Three ordinary rifle sections and an established machine-gun
            // section form the infantry line. No roster receives privileged
            // reserve protection for the achievement.
            if(R.CombinedDEV8311)
            {
                int32 Living[int32(EFrontlineRole::Count)]={};
                for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0)++Living[int32(Q.Role)];
                // Each accepted order consumes the actual shared queue. The
                // command and clearance sections replace extra rifle sections.
                for(const auto Role:{EFrontlineRole::Officer,EFrontlineRole::Lewis,EFrontlineRole::AntiTankRifle,EFrontlineRole::Pioneer,EFrontlineRole::Rifle})
                    if(Living[int32(Role)]<1&&B->Recruit(Role))FeasibilityActionV838(B,TEXT("recruit_combined_arms_section"),int32(Role));
            }
            else
            {
                int32 Guns=0,Rifles=0;
                for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0)
                {Guns+=Q.Role==EFrontlineRole::Lewis;Rifles+=Q.Role==EFrontlineRole::Rifle;}
                if(Guns<1&&B->Recruit(EFrontlineRole::Lewis))FeasibilityActionV838(B,TEXT("recruit_vulnerable_machinegun_cover"),int32(EFrontlineRole::Lewis));
                if(Rifles<3&&B->Recruit(EFrontlineRole::Rifle))FeasibilityActionV838(B,TEXT("recruit_vulnerable_rifle_cover"),int32(EFrontlineRole::Rifle));
            }
        }
        if(Quiet)if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
        {
            int32 Active=0;for(const auto& V:Fleet->GetVehicles())Active+=V&&V->Team==0&&V->IsOperational();
            if(Active<2)
            {
                const bool German=B->GetPlayerFaction()==EFrontlineFaction::German;
                const EFrontlineVehicle TypesUK[]={EFrontlineVehicle::MarkVMale,EFrontlineVehicle::Whippet};
                const EFrontlineVehicle TypesDE[]={EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIV};
                for(int32 I=0;I<2;++I)if(Fleet->Recruit(German?TypesDE[I]:TypesUK[I]))
                {FeasibilityActionV838(B,TEXT("recruit_supporting_armour"),int32(German?TypesDE[I]:TypesUK[I]));break;}
            }
        }
        // A resident MG covers a real visible weapon contact, not an arbitrary
        // home trench. Until then it advances through the same player order as
        // the rifles. No unexposed Hold handler or privileged reserve is used.
        for(const auto& Q:B->Squads)if(Q.Team==0&&Q.LivingCount()>0&&B->Trenches.IsValidIndex(Q.Trench)&&!Q.bAdvancing)
        {
            const auto* Work=State(B).PioneerClearancesV837.Find(Q.Id);
            const bool EngineerHome=!Quiet&&Q.Role==EFrontlineRole::Pioneer&&Work&&Work->Num()>=2;
            bool Hold=EngineerHome||Q.Role==EFrontlineRole::Lewis;
            if(Quiet&&Q.Role==EFrontlineRole::Lewis)
            {
                AWarfareFrontlineSoldier* Observer=nullptr;
                for(const auto& U:Q.Members)if(U.IsValid()&&U->Team==0&&U->bFieldEntered&&U->IsActiveCombatant())
                {if(!Observer)Observer=U.Get();if(U->IsMachineGunner()){Observer=U.Get();break;}}
                if(!Observer)continue; // Reinforcements have not entered yet.
                // The production query owns actual profile distance, weather,
                // smoke and physical sight. It does not alter the weapon state.
                const auto* Visible=B->AcquireTarget(Observer,false);
                Hold=Visible&&Visible->bFieldEntered&&Visible->IsActiveCombatant();
            }
            if(!Hold&&B->AdvanceSquad(Q.Trench,Q.Id))FeasibilityActionV838(B,
                Quiet&&Q.Role==EFrontlineRole::Lewis?TEXT("advance_machinegun_to_visible_fire_position"):TEXT("advance_section"),Q.Id);
        }
        // Choose an entered enemy cluster using public safety and cooldown
        // queries. Friendly-fire damage remains enabled after the order.
        const int32 Direction=B->GetTeamDirection(0);
        for(const auto Support:{EFrontlineSupport::Artillery,EFrontlineSupport::Mortar,EFrontlineSupport::CreepingBarrage})
        {
            if(!B->SupportBlockReason(Support).IsEmpty())continue;
            AWarfareFrontlineSoldier* Enemy=nullptr;int32 Best=0;
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==1&&U->bFieldEntered&&U->IsActiveCombatant()
                &&B->IsFireSupportAreaSafe(U->Feet(),Support,0))
            {
                if(!Quiet&&State(B).Obstacles.IsEmpty())
                {
                    const int32 Home=B->GetHomeTrenchIndex(0),Next=Home+Direction;
                    if(B->Trenches.IsValidIndex(Next)&&U->Feet().X*Direction<B->Trenches[Next].X*Direction+600.f)continue;
                }
                int32 Cluster=0;for(AWarfareFrontlineSoldier* V:B->Units)Cluster+=V&&V->Team==1&&V->bFieldEntered&&V->IsActiveCombatant()
                    &&FVector::DistSquared2D(U->Feet(),V->Feet())<FMath::Square(1100.f);
                if(Cluster>Best){Best=Cluster;Enemy=U;}
            }
            if(Enemy&&B->RequestSupport(Support,Enemy->Feet()))FeasibilityActionV838(B,TEXT("fire_support_on_entered_enemy_cluster"),int32(Support));
        }
        if(R.CombinedDEV8311&&B->SupportBlockReason(EFrontlineSupport::AntiTank).IsEmpty())
            if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
            {
                AFrontlineTank* Seen=nullptr;FVector Aim;double Nearest=MAX_dbl;
                for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->bFieldEntered&&U->IsActiveCombatant())
                    if(auto* V=Fleet->AcquireInfantryTarget(U);V&&V->Team==1&&V->IsOperational()&&V->HasEnteredField())
                    {
                        const FVector Point=V->ObservedArmourAimPointV837(U->Feet());const double Distance=FVector::DistSquared2D(U->Feet(),Point);
                        if(Distance<Nearest&&B->IsFireSupportAreaSafe(Point,EFrontlineSupport::AntiTank,0))
                        {Nearest=Distance;Seen=V;Aim=Point;}
                    }
                if(Seen&&B->RequestSupport(EFrontlineSupport::AntiTank,Aim))
                    FeasibilityActionV838(B,TEXT("anti_tank_fire_on_actual_observed_hull"),Seen->Serial);
            }
        if(!Quiet)
        {
            const FFrontlineSquad* Working=nullptr;float Front=-MAX_flt;int32 Returning=INDEX_NONE;
            PioneerWorkCompletedV842(B,State(B),&Returning);
            for(const auto& Q:B->Squads)if(Q.Team==0&&Q.Role==EFrontlineRole::Pioneer&&Q.LivingCount()>0)
            {
                if(Q.Id==Returning){Working=&Q;break;}
                if(!Q.bAdvancing)continue;
                bool Entered=false;for(const auto& U:Q.Members)Entered|=U.IsValid()&&U->bFieldEntered&&U->IsActiveCombatant();
                const int32 Home=B->GetHomeTrenchIndex(0);
                const bool Crossing=Entered&&B->Trenches.IsValidIndex(Home)
                    &&(Q.Centre().X-B->Trenches[Home].X)*Direction>100.f;
                if(Crossing&&Q.Centre().X*Direction>Front){Working=&Q;Front=Q.Centre().X*Direction;}
            }
            if(Working)
            {
                const bool ReturningSection=Working->Id==Returning;
                const FVector Screen=Working->Centre()+(ReturningSection?FVector::ZeroVector:FVector(Direction*850.f,0,0));
                if(B->RequestSmoke(Screen))FeasibilityActionV838(B,ReturningSection?TEXT("smoke_same_clearing_section"):TEXT("smoke_engineer_approach"),Working->Id);
            }
        }
        return false;
    }
    const int32 Target=Quiet?29:28;auto J=FeasibilitySnapshotV838(B);
    const bool Victory=B->Phase==EFrontlinePhase::Victory;
    const bool Predicate=J->GetBoolField(Quiet?TEXT("quiet_live_predicate"):TEXT("hearts_live_predicate"));
    FAccount Disk;const bool DiskReadable=ReadFile(AccountPath(),Disk);
    const bool ActualSave=DiskReadable&&(Disk.EarnedMask&(1u<<Target));
    const bool Before=R.BeforeEarned&(1u<<Target);
    J->SetStringField(TEXT("case"),FeasibilityNameV838(R.Case));J->SetNumberField(TEXT("case_index"),R.Case);
    J->SetBoolField(TEXT("combined_german_policy_v8311"),R.CombinedDEV8311);
    J->SetBoolField(TEXT("success"),Safe&&Victory&&Predicate&&State(B).Eligible&&State(B).SettlementCaptured&&ActualSave);
    J->SetBoolField(TEXT("actual_account_readable"),DiskReadable);
    J->SetBoolField(TEXT("actual_objective_victory"),Victory);J->SetBoolField(TEXT("actual_account_has_target"),ActualSave);
    J->SetBoolField(TEXT("target_previously_earned"),Before);J->SetBoolField(TEXT("target_newly_earned"),ActualSave&&!Before);
    J->SetStringField(TEXT("outcome"),!Safe?TEXT("review_guard_failed"):Victory?TEXT("victory"):B->Phase==EFrontlinePhase::Defeat?TEXT("defeat"):TEXT("bounded_timeout"));
    J->SetStringField(TEXT("end_reason"),B->EndReason);J->SetStringField(TEXT("last_message"),B->LastMessage);
    J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R.WallStarted);
    J->SetNumberField(TEXT("world_time_dilation"),UGameplayStatics::GetGlobalTimeDilation(B));
    J->SetNumberField(TEXT("largest_observed_world_delta_seconds"),R.MaximumWorldDelta);
    J->SetNumberField(TEXT("largest_observed_world_time_dilation"),R.MaximumWorldDilation);
    J->SetNumberField(TEXT("game_speed_setting"),B->BattleSpeed);
    J->SetNumberField(TEXT("simulation_seconds_limit"),SimulationLimit);J->SetNumberField(TEXT("pioneer_sections_requested"),R.PioneerAttempts);
    J->SetNumberField(TEXT("wall_seconds_limit"),WallLimit);
    J->SetStringField(TEXT("first_rifle_commit_policy"),Quiet?TEXT("Recruit the first rifle section at the opening. All rifle sections receive ordinary public advance orders; none is protected as the first-roster objective. Keep up to three living rifle sections, one machine-gun section and two armoured vehicles. An entered machine-gun section advances by public trench orders until its real gunner observes a production-range, physically visible enemy, then covers that contact. If the gunner is lost, the living entered crew use their own actual weapon observation. No reserve section or shot count is manufactured. The V84.4 honour requires survivors from three infantry classes. This legacy two-class rifle/MG policy is diagnostic only and cannot prove the revised goal; discharges and kills are not additional eligibility gates."):TEXT("Not a final-battle goal case."));
    if(R.CombinedDEV8311)J->SetStringField(TEXT("first_rifle_commit_policy"),TEXT("Last German-only tactical alternative: one rifle, one machine-gun, one actual commissioned-officer, one engineer and one Tankgewehr section, plus up to two armoured vehicles. Every recruit uses the shared cooldown; no section has first-roster protection. All sections advance through public trench orders; the MG only retains actual visible production-range contacts. Engineers use their unmodified real clearance task. Anti-tank support is ordered only on an entered operational hull observed through a living entered infantryman's production range and physical-sight query, with normal friendly-area safety and cooldown. No health, shots, casualty, cooldown or outcome is set."));
    auto Budget=MakeShared<FJsonObject>();Budget->SetNumberField(TEXT("historical_victories"),R.HistoricalWins);
    Budget->SetNumberField(TEXT("historical_income_starting2_plus_three_per_victory"),R.HistoricalIncome);
    Budget->SetNumberField(TEXT("current_dispatch_research_received_through_api"),R.DispatchIncome);
    Budget->SetNumberField(TEXT("infantry_research_cost_debited_through_api"),R.InfantryResearchCost);
    Budget->SetNumberField(TEXT("armour_research_cost_debited_through_api"),R.ArmourResearchCost);
    Budget->SetNumberField(TEXT("research_remaining_before_battle"),R.ResearchRemaining);
    if(const auto* C=B->GetCurrentCampaign()){Budget->SetNumberField(TEXT("infantry_researched_mask"),C->Research.TechnologyMask);Budget->SetNumberField(TEXT("armour_researched_mask"),C->ArmourResearchMaskV63);}
    J->SetObjectField(TEXT("affordable_research_prerequisites"),Budget);
    J->SetArrayField(TEXT("accepted_public_actions"),R.Actions);J->SetArrayField(TEXT("samples"),R.Samples);
    J->SetStringField(TEXT("fixture_prerequisites"),Quiet?TEXT("Fresh volatile UK/DE company; legal route chooses2/6/12/18 at exclusive forks. Fifteen prior victories before19; historical bank47; current training dispatch received through API. Actual purchases: infantry0,1,2,3,4,5,10,16,17,19 cost20; armour0,1,2,3,9,10,11 cost18 plus British5 cost4. No escort/recovery nodes or full research. Legal equipment choices filtered through operation eligibility. No present battle evidence seeded.")
        :TEXT("Fresh volatile UK/DE company on operation2, with only earlier operations0/1 won: historical bank2+2*3=8; current training dispatch2 received through API. Actual infantry purchases0,2,3,4,5,17 cost9 leave1. No armour research or allocation. Pioneer arrival2 and legal support/roster eligibility remain authoritative. No present battle evidence seeded."));
    if(R.CombinedDEV8311)J->SetStringField(TEXT("fixture_prerequisites"),TEXT("Strict NoSave German final-operation fixture. Same fifteen earlier legal-route victories and historical47 plus current dispatch3. Baseline infantry purchases cost20, armour cost18, leaving12. Actual prerequisite-checked purchases6/7/8/24/25 spend precisely12; balance zero. Public departure toggles remove Marksman and add Tankgewehr, retaining Rifle/MG/Officer/Pioneer. Public support selection adds the sixth eligible AntiTank slot; the retired Rally ordinal is normally filtered out. No extra research grant, direct researched mask, forced equipment, protected section or present-battle evidence."));
    J->SetStringField(TEXT("limits"),TEXT("One deterministic public-API action policy per army/goal on actual operations2/19 with a prerequisite-valid affordable research subset. Earlier victories are explicit historical fixtures, not newly played campaign acceptance. Ordinary world/actor ticks, weapon hit rolls, cooldowns, morale, field work, casualties, AI and objective settlement. The unexposed UK/DE Hold handler is not used. Final battle cases use the normal1.5x setting, ordinary infantry advances, an actually entered machine-gun section advancing to visible production-range contacts, and up to two armoured vehicles. This policy cannot prove human enjoyment, whole-campaign completion, all-map balance or performance."));
    R.StartedLive=false;IsolatedAwardBattle.Reset();CompletedReport=J;return true;
}
}
