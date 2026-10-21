#include "WarfareFrontlineBalanceV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr float Step52=1.f/35.f;
constexpr int32 ScenarioCount52=28,LegacyScenarioCount52=12,Operation52=15,V78ScenarioCount=16;
bool V78Balance(){return FParse::Param(FCommandLine::Get(),TEXT("WFV78Balance"));}
int32 ScenarioCount(){return V78Balance()?V78ScenarioCount:ScenarioCount52;}
const TCHAR* RoleKey52(EFrontlineRole R)
{
    static const TCHAR* Keys[]={TEXT("rifle"),TEXT("machinegun"),TEXT("assault"),TEXT("marksman"),TEXT("command"),
        TEXT("flamethrower"),TEXT("pioneer"),TEXT("rifle_grenadier"),TEXT("anti_tank_rifle"),TEXT("legacy_mobile_gun"),TEXT("legacy_observer")};
    return int32(R)>=0&&int32(R)<UE_ARRAY_COUNT(Keys)?Keys[int32(R)]:TEXT("unknown");
}
struct FScenario52
{
    EFrontlineFaction Faction=EFrontlineFaction::British;
    EFrontlineRole Focus=EFrontlineRole::Rifle;
    bool Defend=false,Commissioned=false;
    bool RushComparison=false,PureRifles=false,PureMarksmen=false;
    int32 Operation=Operation52,SeedOffset=0;
    EFrontlineDifficulty Difficulty=EFrontlineDifficulty::Veteran;
    FString Name;
    int32 Context78=INDEX_NONE;
    TArray<EFrontlineRole> Recipe78;
};
FScenario52 Scenario52(int32 Index)
{
    if(V78Balance())
    {
        FScenario52 S;const int32 Local=Index%8;S.Context78=Local/2;
        S.Faction=Index<8?EFrontlineFaction::British:EFrontlineFaction::German;
        S.Operation=S.Context78==1?15:S.Context78==2?16:14;
        S.Difficulty=EFrontlineDifficulty::Survivor;S.RushComparison=true;S.Commissioned=true;
        const bool Mixed=(Local%2)!=0;const bool UK=S.Faction==EFrontlineFaction::British;
        const TCHAR* Context[]={TEXT("field"),TEXT("wire"),TEXT("trench"),TEXT("armour")};
        if(S.Context78==2&&!Mixed){S.PureMarksmen=true;S.Focus=EFrontlineRole::Marksman;S.Recipe78={EFrontlineRole::Marksman};}
        else if(!Mixed&&S.Context78!=3){S.PureRifles=true;S.Recipe78={EFrontlineRole::Rifle};}
        else if(S.Context78==0)S.Recipe78={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Officer,EFrontlineRole::Marksman,EFrontlineRole::Rifle};
        else if(S.Context78==1){S.Focus=EFrontlineRole::Pioneer;S.Recipe78={EFrontlineRole::Rifle,EFrontlineRole::Pioneer,EFrontlineRole::Lewis,EFrontlineRole::Officer,EFrontlineRole::Mills,EFrontlineRole::Rifle};}
        else if(S.Context78==2){S.Focus=UK?EFrontlineRole::RifleGrenadier:EFrontlineRole::Flamethrower;
            S.Recipe78={EFrontlineRole::Lewis,EFrontlineRole::Marksman,UK?EFrontlineRole::RifleGrenadier:EFrontlineRole::Mills,EFrontlineRole::Officer,EFrontlineRole::Flamethrower,EFrontlineRole::Lewis};}
        else {S.Focus=Mixed?(UK?EFrontlineRole::RifleGrenadier:EFrontlineRole::AntiTankRifle):EFrontlineRole::Mills;
            S.Recipe78={EFrontlineRole::Rifle,EFrontlineRole::Lewis,S.Focus,EFrontlineRole::Officer,EFrontlineRole::Rifle,EFrontlineRole::Rifle};}
        S.Name=FString::Printf(TEXT("%s-survivor-%s-%s"),UK?TEXT("uk"):TEXT("de"),Context[S.Context78],Mixed?TEXT("specialist"):TEXT("control"));return S;
    }
    if(Index>=LegacyScenarioCount52)
    {
        const int32 Case=Index-LegacyScenarioCount52,Variant=Case%8;
        FScenario52 S;S.RushComparison=true;S.PureRifles=Variant%2==0;
        S.Faction=Variant>=4?EFrontlineFaction::German:EFrontlineFaction::British;
        S.Operation=(Variant/2)%2?16:2;S.SeedOffset=(Case/8)*997;
        S.Difficulty=EFrontlineDifficulty::Survivor;
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance"))&&S.Operation==16)
        {S.PureMarksmen=S.PureRifles;S.PureRifles=false;S.Focus=EFrontlineRole::Marksman;S.Commissioned=true;}
        S.Name=FString::Printf(TEXT("%s-survivor-op%d-%s-seed%d"),S.Faction==EFrontlineFaction::British?TEXT("uk"):TEXT("de"),
            S.Operation+1,S.PureMarksmen?TEXT("pure-marksmen"):S.PureRifles?TEXT("pure-rifles"):TEXT("mixed"),Case/8+1);return S;
    }
    FScenario52 S;S.Faction=Index>=6?EFrontlineFaction::German:EFrontlineFaction::British;
    const int32 Variant=Index%6;S.Defend=Variant>=4;S.Commissioned=S.Defend;
    S.Focus=Variant==1?EFrontlineRole::Marksman:Variant==2||Variant==5?EFrontlineRole::Officer:Variant==3?EFrontlineRole::Pioneer:EFrontlineRole::Rifle;
    S.Name=FString::Printf(TEXT("%s-%s-%s"),S.Faction==EFrontlineFaction::British?TEXT("uk"):TEXT("de"),
        S.Defend?TEXT("defend"):TEXT("attack"),RoleKey52(S.Focus));return S;
}
const EFrontlineRole* CounterplayRecipeV64(const FScenario52& S)
{
    // Six calls, at most five selected slots. Every choice is available in the
    // named operation; a missing role is a setup error, never silent rifle spam.
    static constexpr EFrontlineRole EarlyUK[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Pioneer,EFrontlineRole::Rifle,EFrontlineRole::RifleGrenadier};
    static constexpr EFrontlineRole EarlyDE[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Pioneer,EFrontlineRole::Rifle,EFrontlineRole::Mills};
    static constexpr EFrontlineRole LateUK[]={EFrontlineRole::Lewis,EFrontlineRole::Marksman,EFrontlineRole::RifleGrenadier,EFrontlineRole::Officer,EFrontlineRole::Flamethrower,EFrontlineRole::Lewis};
    static constexpr EFrontlineRole LateDE[]={EFrontlineRole::Lewis,EFrontlineRole::Marksman,EFrontlineRole::Mills,EFrontlineRole::Officer,EFrontlineRole::Flamethrower,EFrontlineRole::Lewis};
    return S.Operation==16?(S.Faction==EFrontlineFaction::British?LateUK:LateDE):(S.Faction==EFrontlineFaction::British?EarlyUK:EarlyDE);
}
uint32 CounterplayRosterV64(const FScenario52& S)
{uint32 Mask=0;const auto* Recipe=CounterplayRecipeV64(S);for(int32 I=0;I<6;++I)Mask|=1u<<int32(Recipe[I]);return Mask;}
struct FRoleSample52
{
    int32 Squads=0,Men=0,LastShots=0,Shots=0,PrimaryShots=0,EscortShots=0;
    float FirstRecruit=-1,FirstEntry=-1,AliveSeconds=0,SuppressionSeconds=0,CommandedSeconds=0;
    float CommittedWait=0,EnemyObservedSeconds=0;
};
struct FRun52
{
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    double Started=FPlatformTime::Seconds(),ScenarioStarted=0;
    int32 Index=0,ScenarioMask=(1<<LegacyScenarioCount52)-1,Seed=741521,PlayerRecruitStep=0;
    float Limit=420.f,NextOrders=0,NextSupport=0,NextSample=0,NextSnapshot=0,LastSample=0,MaxObjective=0;
    bool Active=false,Good=true;
    int32 MaximumEnemyArmour=0;
    FString Dir,Failure;
    TSet<int32> SeenSquads;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,int32> SeenShots;
    FRoleSample52 Roles[2][AWarfareFrontlineGameMode::RoleCount];
    TArray<TSharedPtr<FJsonValue>> Results,Timeline,Deployments;
    TSharedPtr<FJsonObject> Identity;
};
TSharedRef<FJsonObject> Snapshot52(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("seconds"),B->Elapsed);
    J->SetNumberField(TEXT("objective"),B->GetCampaignObjectiveProgress());
    for(int32 Team=0;Team<2;++Team)
    {
        auto T=MakeShared<FJsonObject>();T->SetNumberField(TEXT("morale"),B->Morale[Team]);T->SetNumberField(TEXT("casualties"),B->Casualties[Team]);
        T->SetNumberField(TEXT("living"),B->LivingSoldiers(Team));T->SetNumberField(TEXT("active_squads"),B->ActiveSquads(Team));
        T->SetNumberField(TEXT("leader_orders"),B->LeaderOrders[Team]);T->SetNumberField(TEXT("suppression_removed"),B->LeaderSuppressionRemoved[Team]);
        float Forward=-FrontlineBattlefield::SpawnHalfLength;int32 Held=0;
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==Team&&U->IsActiveCombatant())Forward=FMath::Max(Forward,float(U->Feet().X*B->GetTeamDirection(Team)));
        for(const auto& Trench:B->Trenches)Held+=Trench.Owner==Team;
        T->SetNumberField(TEXT("foremost_progress_cm"),Forward);T->SetNumberField(TEXT("trenches_owned"),Held);
        J->SetObjectField(Team?TEXT("enemy"):TEXT("player"),T);
    }
    J->SetNumberField(TEXT("shots"),B->ShotsFired);J->SetNumberField(TEXT("field_gun_orders"),B->ArtilleryCommands);
    J->SetNumberField(TEXT("mortar_orders"),B->MortarCommands);J->SetNumberField(TEXT("grenades"),B->GrenadesThrown);
    J->SetNumberField(TEXT("trench_captures"),B->TrenchCaptures);return J;
}
bool Visible52(AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* Enemy)
{
    if(!Enemy||Enemy->Team!=1||!Enemy->IsActiveCombatant())return false;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsActiveCombatant()
        &&FVector::DistSquared2D(U->Feet(),Enemy->Feet())<FMath::Square(6200.f*B->EncounterVisibilityScale())
        &&B->DirectFireMultiplier(U->Feet(),Enemy->Feet())>=.95f&&B->HasSight(U,Enemy))return true;
    return false;
}
void PlayerPolicy52(AWarfareFrontlineGameMode* B,FRun52& Run,const FScenario52& Scenario)
{
    // A committed choice waits for its real shared cooldown. It never falls
    // back to ready rifles merely because the chosen specialist takes longer.
    constexpr int32 Recipe[]={0,1,2,0,3,0,2,0};
    const int32 Pick=Recipe[Run.PlayerRecruitStep%UE_ARRAY_COUNT(Recipe)];
    EFrontlineRole Wanted=Pick==0?EFrontlineRole::Rifle:Pick==1?EFrontlineRole::Lewis:Pick==3?EFrontlineRole::Mills:Scenario.Focus;
    if(Scenario.RushComparison)
    {
        static constexpr EFrontlineRole Mixed[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Pioneer,EFrontlineRole::Rifle,EFrontlineRole::Officer};
        const bool V64=FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance"));
        Wanted=V78Balance()?Scenario.Recipe78[Run.PlayerRecruitStep%Scenario.Recipe78.Num()]:Scenario.PureMarksmen?EFrontlineRole::Marksman:Scenario.PureRifles?EFrontlineRole::Rifle:(V64?CounterplayRecipeV64(Scenario):Mixed)[Run.PlayerRecruitStep%UE_ARRAY_COUNT(Mixed)];
        const auto Loadout=FrontlineCampaignV32::GetLoadout(B);
        if(!(Loadout.SelectedRoles&(1u<<int32(Wanted))))
        {if(V64||V78Balance()){Run.Good=false;Run.Failure=TEXT("counterplay_recipe_role_unavailable");return;}Wanted=EFrontlineRole::Rifle;}
    }
    if(Wanted==EFrontlineRole::Officer)
        // Escorts surviving their commander do not supply an aura. A living
        // incoming commander does, however, already occupy this policy choice.
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsSectionLeader()&&U->IsAlive())
            {Wanted=(V78Balance()||FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance")))&&Scenario.Operation==16?EFrontlineRole::Lewis:EFrontlineRole::Rifle;break;}
    if(V78Balance()&&B->RecruitCooldown(Wanted,0)>0.f)Run.Roles[0][int32(Wanted)].CommittedWait+=Step52;
    // These are documented controller choices, not game-wide squad caps.
    const int32 PolicyCapacity=Scenario.RushComparison?MAX_int32:Scenario.Defend?3:6;
    if(B->ActiveSquads(0)<PolicyCapacity&&B->RecruitBlockReason(Wanted,0).IsEmpty()&&B->Recruit(Wanted,0))++Run.PlayerRecruitStep;
    if(B->Elapsed>=Run.NextOrders)
    {
        Run.NextOrders=B->Elapsed+2.f;
        if(!Scenario.Defend)
        {
            const int32 Dir=B->GetTeamDirection(0);
            for(int32 I=Dir>0?B->Trenches.Num()-1:0;B->Trenches.IsValidIndex(I);I-=Dir)
            {
                if(B->Trenches[I].Owner!=0)continue;
                const int32 Friendly=B->TrenchSquadCount(I,0);if(Friendly<2&&B->Elapsed<45)continue;
                if(Friendly>0)B->AdvanceTrench(I,0);
            }
        }
    }
    if(B->Elapsed<Run.NextSupport)return;Run.NextSupport=B->Elapsed+2.5f;
    TArray<AWarfareFrontlineSoldier*> Seen;
    for(AWarfareFrontlineSoldier* U:B->Units)if(Visible52(B,U))Seen.Add(U);
    AWarfareFrontlineSoldier* Target=nullptr;float Best=0;
    for(auto* U:Seen)
    {
        float Value=U->IsMachineGunner()?2.f:1.f;
        for(auto* V:Seen)if(V!=U&&FVector::Dist2D(U->Feet(),V->Feet())<900.f)Value+=1.f;
        if(Value>Best){Best=Value;Target=U;}
    }
    if(Target&&Best>=3.f)
    {
        const FVector P=Target->Feet();
        if(V78Balance()&&Best>=7.f&&B->SupportBlockReason(EFrontlineSupport::AirRecon,0).IsEmpty()&&B->IsFireSupportAreaSafe(P,EFrontlineSupport::AirRecon,0))B->RequestSupport(EFrontlineSupport::AirRecon,P,0);
        else if(B->ArtilleryBlockReason(0).IsEmpty()&&B->IsFireSupportAreaSafe(P,EFrontlineSupport::Artillery,0))B->RequestArtillery(P,0);
        else if(B->MortarBlockReason(0).IsEmpty()&&B->IsFireSupportAreaSafe(P,EFrontlineSupport::Mortar,0))B->RequestMortar(P,0);
    }
    AWarfareFrontlineSoldier* Pressed=nullptr;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsActiveCombatant()&&U->Suppression>.5f&&(!Pressed||U->Suppression>Pressed->Suppression))Pressed=U;
    if(Pressed&&B->RallyBlockReason(0).IsEmpty())B->RequestRally(Pressed->Feet(),0);
    if(Pressed&&Pressed->bMoving&&!Seen.IsEmpty()&&B->SmokeBlockReason(0).IsEmpty())B->RequestSmoke(Pressed->Feet()+FVector(B->GetTeamDirection(0)*550.f,0,0),0);
    // The same infantry-heavy policy can respond to observed armour. It does
    // not get coordinates from a tank hidden from every living friendly man.
    if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
        for(AFrontlineTank* T:V->GetVehicles())if(T&&T->Team==1&&T->IsOperational()&&T->HasEnteredField())
        {
            bool SeenTank=false;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(V52BalanceArmour),true);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)Params.AddIgnoredActor(U);Params.AddIgnoredActor(T);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsActiveCombatant()
                &&FVector::Dist2D(U->Feet(),T->AimPoint())<6000.f*B->EncounterVisibilityScale()
                &&B->DirectFireMultiplier(U->Feet(),T->AimPoint())>=.95f
                &&!B->GetWorld()->LineTraceTestByChannel(U->Feet()+FVector(0,0,145),T->AimPoint(),ECC_Visibility,Params)){SeenTank=true;break;}
            if(SeenTank&&B->SupportBlockReason(EFrontlineSupport::AntiTank,0).IsEmpty()&&B->IsFireSupportAreaSafe(T->AimPoint(),EFrontlineSupport::AntiTank,0))
            {B->RequestSupport(EFrontlineSupport::AntiTank,T->AimPoint(),0);break;}
        }
}
void Sample52(AWarfareFrontlineGameMode* B,FRun52& Run)
{
    for(const auto& S:B->Squads)if(!Run.SeenSquads.Contains(S.Id))
    {
        Run.SeenSquads.Add(S.Id);auto& R=Run.Roles[S.Team][int32(S.Role)];++R.Squads;R.Men+=S.Members.Num();
        if(R.FirstRecruit<0)R.FirstRecruit=B->Elapsed;
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("time"),B->Elapsed);J->SetNumberField(TEXT("team"),S.Team);
        J->SetNumberField(TEXT("squad_id"),S.Id);J->SetStringField(TEXT("role"),RoleKey52(S.Role));J->SetNumberField(TEXT("men"),S.Members.Num());
        J->SetNumberField(TEXT("shared_recruit_seconds"),B->EffectiveRecruitSeconds(S.Role,S.Team));Run.Deployments.Add(MakeShared<FJsonValueObject>(J));
    }
    if(B->Elapsed<Run.NextSample)return;
    const float DT=FMath::Max(0.f,B->Elapsed-Run.LastSample);Run.LastSample=B->Elapsed;Run.NextSample=B->Elapsed+1.f;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U)
    {
        auto& R=Run.Roles[U->Team][int32(U->Role)];int32& Prior=Run.SeenShots.FindOrAdd(U);
        const int32 NewShots=FMath::Max(0,U->ShotsDischarged-Prior);R.Shots+=NewShots;Prior=U->ShotsDischarged;
        if(U->MemberIndex==0||U->IsMachineGunner())R.PrimaryShots+=NewShots;else R.EscortShots+=NewShots;
        if(U->IsActiveCombatant())
        {
            if(R.FirstEntry<0)R.FirstEntry=B->Elapsed;R.AliveSeconds+=DT;R.SuppressionSeconds+=DT*U->Suppression;
            if(FrontlineGameplayV52::CommandAccuracyScale(U)>1.f)R.CommandedSeconds+=DT;
            if(V78Balance()&&U->Team==0&&FrontlineDifficultyV30::CanObserve(B,U))R.EnemyObservedSeconds+=DT;
        }
    }
    if(V78Balance())if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
    {int32 Count=0;for(AFrontlineTank* T:Fleet->GetVehicles())if(T&&T->Team==1&&T->IsOperational()&&T->HasEnteredField())++Count;Run.MaximumEnemyArmour=FMath::Max(Run.MaximumEnemyArmour,Count);}
    Run.MaxObjective=FMath::Max(Run.MaxObjective,B->GetCampaignObjectiveProgress());
    if(B->Elapsed>=Run.NextSnapshot){Run.NextSnapshot=B->Elapsed+30.f;Run.Timeline.Add(MakeShared<FJsonValueObject>(Snapshot52(B)));}
}
void Write52(FRun52& Run,bool Finished)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("status"),Finished?(Run.Good?TEXT("completed_exploratory_measurement"):TEXT("failed")):TEXT("running"));
    J->SetStringField(TEXT("scope"),TEXT("Native UE fixed-step actual battle actors, production recruitment/AI/support/terrain/morale/objectives. Scripted infantry-heavy player policy; no human pick-rate estimate or universal balance claim."));
    J->SetStringField(TEXT("limitations"),FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance"))?TEXT("V64 IDs 12-19 compare uncapped pure rifles (operation 3) or pure precision pairs (operation 17) against mixed sections on Survivor, both armies and the same seed. British operation 17 has scoped training in both policies. Identical support controller, research and real shared queues. Native ordering may vary; scripted use is not human pick rate. Time caps are unresolved; no visual, performance or universal balance claim."):TEXT("Legacy IDs 0-11 retain operation 16 Veteran and bounded mixed recipes. IDs 12-27 compare uncapped pure rifles against uncapped mixed sections on Survivor, operations 3 and 17, both armies, with two paired battle seeds. Both policies use the same support controller and research. Native actor ordering may vary. No performance, visual or universal-balance claim. Time caps are unresolved."));
    J->SetBoolField(TEXT("v64_counterplay_comparison"),FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance")));
    J->SetNumberField(TEXT("elapsed_wall_seconds"),FPlatformTime::Seconds()-Run.Started);J->SetNumberField(TEXT("seed"),Run.Seed);
    J->SetNumberField(TEXT("fixed_step_seconds"),Step52);J->SetNumberField(TEXT("max_game_seconds_per_scenario"),Run.Limit);
    TArray<TSharedPtr<FJsonValue>> Selected;
    for(int32 I=0;I<ScenarioCount();++I)if(Run.ScenarioMask&(1<<I))Selected.Add(MakeShared<FJsonValueNumber>(I));
    J->SetArrayField(TEXT("selected_scenario_ids"),Selected);
    J->SetStringField(TEXT("command_replacement_policy"),TEXT("replace_dead_commander_even_if_escorts_survive; living_inbound_commander_blocks_duplicate"));
    J->SetNumberField(TEXT("scenario_count_requested"),Selected.Num());J->SetNumberField(TEXT("scenario_count_completed"),Run.Results.Num());
    J->SetStringField(TEXT("failure"),Run.Failure);J->SetArrayField(TEXT("scenarios"),Run.Results);
    if(V78Balance())
    {
        J->SetNumberField(TEXT("review_version"),78);
        J->SetStringField(TEXT("limitations"),TEXT("Sixteen fixed-policy real battle scenarios: field, dense obstacles, trench and potential armour contacts, both factions. Operation terrain is intact; labels describe authored contexts, not forced geometry or guaranteed armour. Each external seed uses the same recipes and visible-contact support controller. Endogenous combat and native actor ordering vary. Time caps are unresolved, not wins. Role kills are production source attribution; suppression/command exposure is not kill credit. Shared preparation resets every role to its own duration after every recruit. No human pick-rate, enjoyment, full-campaign or performance claim."));
    }
    FString Out;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Out));FFileHelper::SaveStringToFile(Out,*(Run.Dir/TEXT("balance.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}

struct FFrontlineBalanceV52Review
{
    static bool Begin(AWarfareFrontlineGameMode* B,FRun52& Run)
    {
        const auto S=Scenario52(Run.Index);B->AbandonBattleToOperations();
        FFrontlineCampaignProgress Archive;Archive.Id=FGuid::NewGuid();Archive.Faction=S.Faction;Archive.Difficulty=S.Difficulty;
        Archive.SelectedMission=Archive.CompletedMissions=S.Operation;Archive.Research.ResearchPoints=0;
        // Same common paid training in each paired roster. The commissioned
        // defence pair both own the commission, even its no-officer control.
        Archive.Research.TechnologyMask=(1u<<0)|(1u<<1)|(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)|(1u<<10)|(1u<<17);
        if(S.RushComparison)Archive.Research.TechnologyMask=(1u<<0)|(1u<<1)|(1u<<2)|(1u<<4)|(1u<<5);
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance"))&&S.Operation==16&&S.Faction==EFrontlineFaction::British)
            Archive.Research.TechnologyMask|=(1u<<3)|(1u<<10)|(1u<<12); // Both compared rosters own the same paid prerequisite chain.
        if(S.Commissioned)Archive.Research.TechnologyMask|=1u<<(S.Faction==EFrontlineFaction::British?9:6);
        Archive.RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis))|(1u<<int32(EFrontlineRole::Mills))
            |(1u<<int32(EFrontlineRole::Officer))|(1u<<int32(S.Focus==EFrontlineRole::Marksman?EFrontlineRole::Marksman:EFrontlineRole::Pioneer));
        const bool V64=FParse::Param(FCommandLine::Get(),TEXT("WFV64Balance"))&&S.RushComparison;
        if(V64)Archive.RoleSelectionMask=CounterplayRosterV64(S);
        Archive.SupportSelectionMask=(1u<<int32(EFrontlineSupport::Artillery))|(1u<<int32(EFrontlineSupport::Mortar))|(1u<<int32(EFrontlineSupport::Smoke))
            |(1u<<int32(EFrontlineSupport::Rally))|(1u<<int32(EFrontlineSupport::AntiTank))|(1u<<int32(EFrontlineSupport::AirStrike));
        if(V78Balance())
        {
            Archive.Research.TechnologyMask=(1u<<0)|(1u<<1)|(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)|(1u<<10)|(1u<<17)|(1u<<18)
                |(1u<<(S.Faction==EFrontlineFaction::British?9:6));
            if(S.Faction==EFrontlineFaction::British)Archive.Research.TechnologyMask|=1u<<12;
            Archive.RoleSelectionMask=0;for(const auto R:S.Recipe78)Archive.RoleSelectionMask|=1u<<int32(R);
            Archive.SupportSelectionMask=(1u<<int32(EFrontlineSupport::Artillery))|(1u<<int32(EFrontlineSupport::Mortar))
                |(1u<<int32(EFrontlineSupport::Smoke))
                |(1u<<int32(EFrontlineSupport::AntiTank))|(1u<<int32(EFrontlineSupport::AirRecon));
        }
        B->CampaignRuns={Archive};B->ActiveCampaignId=Archive.Id;B->PlayerFaction=S.Faction;B->ResetEncounter();
        B->Random.Initialize(Run.Seed+S.SeedOffset);B->EnemyDecisionRandom.Initialize(Run.Seed+S.SeedOffset+540209);
        B->BeginBattle();B->SetActorTickEnabled(true);UGameplayStatics::SetGamePaused(B,false);
        if(B->Phase!=EFrontlinePhase::Fighting||!B->AssetFailure.IsEmpty())return false;
        Run.SeenSquads.Empty();Run.SeenShots.Empty();Run.Timeline.Empty();Run.Deployments.Empty();
        for(auto& Team:Run.Roles)for(auto& Role:Team)Role=FRoleSample52();
        Run.PlayerRecruitStep=0;Run.NextOrders=Run.NextSupport=Run.NextSample=Run.NextSnapshot=Run.LastSample=Run.MaxObjective=0;
        Run.MaximumEnemyArmour=0;
        Run.ScenarioStarted=FPlatformTime::Seconds();Run.Active=true;Run.Identity=MakeShared<FJsonObject>();
        Run.Identity->SetNumberField(TEXT("scenario_id"),Run.Index);
        Run.Identity->SetStringField(TEXT("name"),S.Name);Run.Identity->SetStringField(TEXT("faction"),S.Faction==EFrontlineFaction::British?TEXT("British"):TEXT("German"));
        Run.Identity->SetStringField(TEXT("policy"),S.RushComparison?(S.PureMarksmen?TEXT("uncapped_pure_marksmen_shared_support"):S.PureRifles?TEXT("uncapped_pure_rifles_shared_support"):TEXT("uncapped_mixed_shared_support")):
            S.Defend?TEXT("hold_home_max_three_squads"):TEXT("advance_in_sections_max_six_squads"));
        Run.Identity->SetNumberField(TEXT("battle_seed"),Run.Seed+S.SeedOffset);Run.Identity->SetNumberField(TEXT("difficulty"),int32(S.Difficulty));
        Run.Identity->SetBoolField(TEXT("uncapped_recruitment_policy"),S.RushComparison);
        Run.Identity->SetStringField(TEXT("focus_role"),RoleKey52(S.Focus));Run.Identity->SetBoolField(TEXT("commissioned"),S.Commissioned);
        Run.Identity->SetNumberField(TEXT("operation_zero_based"),S.Operation);Run.Identity->SetNumberField(TEXT("research_mask"),Archive.Research.TechnologyMask);
        Run.Identity->SetNumberField(TEXT("role_selection_mask"),Archive.RoleSelectionMask);Run.Identity->SetNumberField(TEXT("support_selection_mask"),Archive.SupportSelectionMask);
        const auto L=FrontlineCampaignV32::GetLoadout(B);Run.Identity->SetNumberField(TEXT("actual_role_selection_mask"),L.SelectedRoles);
        if(V64)
        {
            const bool Fits=L.SelectedRoles==Archive.RoleSelectionMask&&FrontlineCampaignV32::BitCount(Archive.RoleSelectionMask)<=L.RoleCapacity;
            Run.Identity->SetBoolField(TEXT("recipe_matches_actual_selected_slots"),Fits);if(!Fits)return false;
            TArray<TSharedPtr<FJsonValue>> Recipe;const auto* Roles=CounterplayRecipeV64(S);
            for(int32 I=0;I<6;++I)Recipe.Add(MakeShared<FJsonValueString>(RoleKey52(Roles[I])));
            Run.Identity->SetArrayField(TEXT("mixed_recruitment_recipe"),Recipe);
            Run.Identity->SetStringField(TEXT("live_commander_substitution"),S.Operation==16?TEXT("machinegun; present in the configured roster"):TEXT("not used in early roster"));
        }
        Run.Identity->SetNumberField(TEXT("actual_support_selection_mask"),L.SelectedSupports);Run.Identity->SetNumberField(TEXT("role_capacity"),L.RoleCapacity);
        if(V78Balance())
        {
            const bool Fits=L.SelectedRoles==Archive.RoleSelectionMask&&L.SelectedSupports==Archive.SupportSelectionMask
                &&FrontlineCampaignV32::BitCount(Archive.RoleSelectionMask)<=L.RoleCapacity;
            Run.Identity->SetBoolField(TEXT("recipe_matches_actual_selected_slots"),Fits);if(!Fits)return false;
            Run.Identity->SetNumberField(TEXT("context_index"),S.Context78);Run.Identity->SetNumberField(TEXT("paired_case"),(Run.Index%8)/2);
            Run.Identity->SetStringField(TEXT("policy"),S.Name);
            TArray<TSharedPtr<FJsonValue>> Recipe;for(const auto R:S.Recipe78)Recipe.Add(MakeShared<FJsonValueString>(RoleKey52(R)));
            Run.Identity->SetArrayField(TEXT("recruitment_recipe"),Recipe);
        }
        Run.Identity->SetNumberField(TEXT("initial_player_morale"),B->Morale[0]);Run.Identity->SetNumberField(TEXT("initial_enemy_morale"),B->Morale[1]);
        Run.Identity->SetNumberField(TEXT("trench_count"),B->Trenches.Num());Run.Identity->SetNumberField(TEXT("goal_half_length_cm"),FrontlineBattlefield::GoalHalfLength);
        Run.Identity->SetNumberField(TEXT("focus_recruit_seconds"),B->EffectiveRecruitSeconds(S.Focus));
        if(!(L.SelectedRoles&(1u<<int32(S.Focus))))return false;
        UE_LOG(LogTemp,Display,TEXT("V52_BALANCE_BEGIN index=%d name=%s seed=%d"),Run.Index,*S.Name,Run.Seed);return true;
    }
    static void Finish(AWarfareFrontlineGameMode* B,FRun52& Run)
    {
        Run.NextSample=0;Sample52(B,Run);auto J=Run.Identity.ToSharedRef();
        J->SetStringField(TEXT("result"),B->Phase==EFrontlinePhase::Victory?TEXT("player_victory"):B->Phase==EFrontlinePhase::Defeat?TEXT("player_defeat"):TEXT("unresolved_time_cap"));
        J->SetStringField(TEXT("end_reason"),B->EndReason);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Run.ScenarioStarted);
        J->SetNumberField(TEXT("maximum_objective_progress"),Run.MaxObjective);J->SetObjectField(TEXT("final"),Snapshot52(B));
        J->SetObjectField(TEXT("hazards"),FrontlineFieldV32::Review(B));
        J->SetArrayField(TEXT("timeline"),Run.Timeline);J->SetArrayField(TEXT("deployments"),Run.Deployments);
        TArray<TSharedPtr<FJsonValue>> Rows;
        for(int32 T=0;T<2;++T)for(int32 R=0;R<AWarfareFrontlineGameMode::RoleCount;++R)if(Run.Roles[T][R].Squads)
        {
            const auto& V=Run.Roles[T][R];auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),T);Row->SetStringField(TEXT("role"),RoleKey52(EFrontlineRole(R)));
            Row->SetNumberField(TEXT("squads_recruited"),V.Squads);Row->SetNumberField(TEXT("men_recruited"),V.Men);Row->SetNumberField(TEXT("shots"),V.Shots);
            Row->SetNumberField(TEXT("primary_weapon_shots"),V.PrimaryShots);Row->SetNumberField(TEXT("escort_weapon_shots"),V.EscortShots);
            Row->SetNumberField(TEXT("first_recruit_seconds"),V.FirstRecruit);Row->SetNumberField(TEXT("first_field_entry_seconds"),V.FirstEntry);
            Row->SetNumberField(TEXT("alive_man_seconds"),V.AliveSeconds);Row->SetNumberField(TEXT("suppression_man_seconds"),V.SuppressionSeconds);
            Row->SetNumberField(TEXT("commanded_man_seconds"),V.CommandedSeconds);Rows.Add(MakeShared<FJsonValueObject>(Row));
            if(V78Balance())
            {
                const auto& K=B->CombatRecordV65[T].InfantrySources[R];
                Row->SetNumberField(TEXT("attributed_infantry_kills"),K.Infantry);Row->SetNumberField(TEXT("attributed_armour_kills"),K.Armour);
                Row->SetNumberField(TEXT("committed_queue_wait_seconds"),V.CommittedWait);
                Row->SetNumberField(TEXT("enemy_observed_man_seconds"),V.EnemyObservedSeconds);
                Row->SetNumberField(TEXT("infantry_kills_per_recruited_squad"),V.Squads?float(K.Infantry)/V.Squads:0);
                Row->SetNumberField(TEXT("infantry_kills_per_nominal_preparation_second"),V.Squads?K.Infantry/(V.Squads*B->EffectiveRecruitSeconds(EFrontlineRole(R),T)):0);
            }
        }
        if(V78Balance())
        {
            auto Totals=MakeShared<FJsonObject>();FrontlineCombatRecordV65::WriteArchive(*Totals,B->CombatRecordV65[0]);J->SetObjectField(TEXT("player_combat_record"),Totals);
            auto Enemy=MakeShared<FJsonObject>();FrontlineCombatRecordV65::WriteArchive(*Enemy,B->CombatRecordV65[1]);J->SetObjectField(TEXT("enemy_combat_record"),Enemy);
            J->SetNumberField(TEXT("maximum_operational_enemy_armour"),Run.MaximumEnemyArmour);
            if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
            {int32 EnemyArmour=0;for(AFrontlineTank* T:Fleet->GetVehicles())if(T&&T->Team==1)++EnemyArmour;J->SetNumberField(TEXT("enemy_armour_actors_present_at_end"),EnemyArmour);}
        }
        J->SetArrayField(TEXT("role_usage"),Rows);Run.Results.Add(MakeShared<FJsonValueObject>(J));
        UE_LOG(LogTemp,Display,TEXT("V52_BALANCE_END index=%d seconds=%.1f phase=%d morale=%.2f/%.2f casualties=%d/%d shots=%d"),
            Run.Index,B->Elapsed,int32(B->Phase),B->Morale[0],B->Morale[1],B->Casualties[0],B->Casualties[1],B->ShotsFired);
        B->SetActorTickEnabled(false);B->Phase=EFrontlinePhase::Briefing;Run.Active=false;++Run.Index;Write52(Run,false);
    }
};

bool StartFrontlineBalanceV52(AWarfareFrontlineGameMode* B)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV52Balance")))return false;
    FString UserDir;
    const bool Allowed=B&&B->GetWorld()&&B->IsProgressionVolatile()&&GUsingNullRHI
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)
        &&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
    if(!Allowed){UE_LOG(LogTemp,Error,TEXT("V52 balance requires a volatile, absolute isolated UserDir, WFRenderNoSave and NullRHI."));FPlatformMisc::RequestExitWithStatus(false,2);return true;}
    auto Run=MakeShared<FRun52>();Run->Battle=B;Run->Dir=FPaths::ProjectSavedDir()/(V78Balance()?TEXT("Gameplay/V78/Balance"):TEXT("Gameplay/V52/Balance"));IFileManager::Get().MakeDirectory(*Run->Dir,true);
    FParse::Value(FCommandLine::Get(),TEXT("WFBalanceSeed="),Run->Seed);
    int32 Only=INDEX_NONE,Mask=(1<<(V78Balance()?V78ScenarioCount:LegacyScenarioCount52))-1;
    const bool HasOnly=FParse::Value(FCommandLine::Get(),TEXT("WFBalanceScenario="),Only);
    const bool HasMask=FParse::Value(FCommandLine::Get(),TEXT("WFBalanceScenarioMask="),Mask);
    if((HasOnly&&HasMask)||(HasOnly&&(Only<0||Only>=ScenarioCount()))||(HasMask&&(Mask<=0||(Mask&~((1<<ScenarioCount())-1))!=0)))
    {UE_LOG(LogTemp,Error,TEXT("V52 balance requires one valid scenario index (0-27) or a non-empty 28-bit mask, not both."));FPlatformMisc::RequestExitWithStatus(false,2);return true;}
    Run->ScenarioMask=HasOnly?1<<Only:Mask;
    float Limit=420;if(FParse::Value(FCommandLine::Get(),TEXT("WFBalanceSeconds="),Limit))Run->Limit=FMath::Clamp(Limit,60.f,420.f);
    FApp::SetFixedDeltaTime(Step52);FApp::SetUseFixedTimeStep(true);
    if(auto* MaxFPS=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))MaxFPS->Set(0.f,ECVF_SetByCode);
    B->SetActorTickEnabled(false);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float)
    {
        auto* Battle=Run->Battle.Get();
        if(!Battle||!Run->Good||FPlatformTime::Seconds()-Run->Started>1500.)
        {Run->Good=false;if(Run->Failure.IsEmpty())Run->Failure=Battle?TEXT("wall_timeout"):TEXT("battle_destroyed");Write52(*Run,true);FPlatformMisc::RequestExitWithStatus(false,2);return false;}
        if(!Run->Active)
        {
            while(Run->Index<ScenarioCount()&&!(Run->ScenarioMask&(1<<Run->Index)))++Run->Index;
            if(Run->Index>=ScenarioCount()){Write52(*Run,true);FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:2);return false;}
            if(!Battle->HasActorBegunPlay()||!Battle->LoadedSoldier||!Battle->IdleAnimation)return true;
            if(!FFrontlineBalanceV52Review::Begin(Battle,*Run))
            {Run->Good=false;Run->Failure=TEXT("scenario_setup_failed");Write52(*Run,true);FPlatformMisc::RequestExitWithStatus(false,2);return false;}
        }
        Sample52(Battle,*Run);
        if(Battle->Phase!=EFrontlinePhase::Fighting||Battle->Elapsed>=Run->Limit)
        {FFrontlineBalanceV52Review::Finish(Battle,*Run);return true;}
        PlayerPolicy52(Battle,*Run,Scenario52(Run->Index));return true;
    }));return true;
}
