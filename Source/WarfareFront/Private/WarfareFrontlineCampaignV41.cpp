#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineUIAssetsV8213.h"
#include "WarfareFrontlineCampaignV51.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

namespace
{
bool UIPerfAllowedV8213()
{
    FString User;const TCHAR* Cmd=FCommandLine::Get();
    if(!FParse::Param(Cmd,TEXT("WFUIPerfV8213"))||!FParse::Param(Cmd,TEXT("WFRenderNoSave"))
        ||!FParse::Value(Cmd,TEXT("UserDir="),User)||User.Contains(TEXT("..")))return false;
    FPaths::NormalizeDirectoryName(User);
    return !FPaths::IsRelative(User)&&User.StartsWith(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8213/Transitions/")),ESearchCase::IgnoreCase);
}
constexpr int32 LegacyMap[]={0,3,5,8,11,14,15,16,17,19};
struct FAuthoredV41
{
    const TCHAR *Key,*BritishTitle,*GermanTitle,*History,*BritishOrders,*GermanOrders,*Terrain;
    int32 Date,Art;EFrontlineMissionTerrainV32 Style;
    float Wet,Rain,Width,Wire,Mines;int32 Lines;bool Counter,Depot;
};
#include "WarfareFrontlineCampaignDataV41.inl"
struct FRuntimeV41
{
    FGuid Encounter;bool CentreSeen=false,DepotDelivered=false,DispatchApplied=false;
    float SmokeRearmReserve=0.f,LastPlayerSmokeReady=0.f;int32 ObservedSmokeCommands=0;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FRuntimeV41> StatesV41;
int32 ObjectiveTrenchV50(const AWarfareFrontlineGameMode* B,int32 Team=0)
{
    const int32 Count=FrontlineBattlefield::TrenchCount;
    if(Count%2)return FrontlineBattlefield::CentreTrench;
    // An even line count has no neutral middle trench. Its contested objective
    // is the first position on the opposing half, mirrored with the army.
    return B&&B->GetTeamDirection(Team)<0?Count/2-1:Count/2;
}
UTexture2D* TextureV41(const FString& Path)
{
    static TMap<FString,TStrongObjectPtr<UTexture2D>> Cache;
    if(auto* Existing=Cache.Find(Path))return Existing->Get();
    auto* Texture=FrontlineUIAssetsV8213::Texture(Path);
    if(Texture)Cache.Add(Path,TStrongObjectPtr<UTexture2D>(Texture));
    return Texture;
}
}

// This adapter alone mutates enemy planning. Observing a lost, owned position is
// legitimate battlefield information; it never consumes the player's commands.
struct FFrontlineCampaignV41Access
{
    static void DispatchChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!B||!B->IsProgressionVolatile()||!((FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV66Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))||(FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV53Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV52Balance"))))return;
        const auto Saved=B->CampaignRuns;const auto Id=B->ActiveCampaignId;const auto F=B->PlayerFaction;
        const bool Ticking=B->IsActorTickEnabled();
        ON_SCOPE_EXIT{B->AbandonBattleToOperations();B->CampaignRuns=Saved;B->ActiveCampaignId=Id;B->PlayerFaction=F;B->ResetEncounter();B->SetActorTickEnabled(Ticking);};
        B->SetActorTickEnabled(false);bool Applied=true,Once=true,Absent=true,SmokeRearm=true,EnemySmokeIsolated=true;
        for(int32 Faction=0;Faction<2;++Faction)for(int32 Index:{3,5,8,11,14,16,18})
        {
            Applied&=Select(B,EFrontlineFaction(Faction),Index);
            B->CampaignRuns.Last().AcknowledgedEventMask=1u<<Index;
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            B->ArtilleryReadyAt[0]=65;B->MortarReadyAt[0]=35;B->SmokeReadyAt[0]=12;B->Morale[0]=100;
            const auto D=FrontlineCampaignV32::DispatchConditionsV50(Index,1u<<Index);
            FrontlineCampaignV41::TickObjective(B,0);
            const float A=B->ArtilleryReadyAt[0],M=B->MortarReadyAt[0],S=B->SmokeReadyAt[0],C=B->Morale[0];
            Applied&=FMath::IsNearlyEqual(A,65-D.ArtilleryAdvance)&&FMath::IsNearlyEqual(M,FMath::Max(0.f,35-D.MortarAdvance))
                &&S==12&&FMath::IsNearlyEqual(C,100+D.MoraleReserve);
            FrontlineCampaignV41::TickObjective(B,0);
            Once&=A==B->ArtilleryReadyAt[0]&&M==B->MortarReadyAt[0]&&S==B->SmokeReadyAt[0]&&C==B->Morale[0];
            // The smoke counter is shared by both armies. An enemy order must
            // not consume the player's one-time reserve or alter their timer.
            ++B->SmokeCommands;B->SmokeReadyAt[1]=75;FrontlineCampaignV41::TickObjective(B,0);
            EnemySmokeIsolated&=B->SmokeReadyAt[0]==12&&B->SmokeReadyAt[1]==75;
            ++B->SmokeCommands;B->SmokeReadyAt[0]=80;FrontlineCampaignV41::TickObjective(B,0);
            SmokeRearm&=FMath::IsNearlyEqual(B->SmokeReadyAt[0],80-D.SmokeAdvance)&&B->SmokeCommands==2;
            FrontlineCampaignV41::TickObjective(B,0);
            SmokeRearm&=FMath::IsNearlyEqual(B->SmokeReadyAt[0],80-D.SmokeAdvance);
            ++B->SmokeCommands;B->SmokeReadyAt[0]=90;FrontlineCampaignV41::TickObjective(B,0);
            SmokeRearm&=B->SmokeReadyAt[0]==90&&B->SmokeCommands==3;
            Select(B,EFrontlineFaction(Faction),Index);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            B->ArtilleryReadyAt[0]=65;B->MortarReadyAt[0]=35;B->SmokeReadyAt[0]=12;B->Morale[0]=100;
            FrontlineCampaignV41::TickObjective(B,0);
            Absent&=B->ArtilleryReadyAt[0]==65&&B->MortarReadyAt[0]==35&&B->SmokeReadyAt[0]==12&&B->Morale[0]==100;
        }
        bool Mirrored=true;
        for(int32 Faction=0;Faction<2;++Faction)for(int32 Index:{1,8,10,16})
        {
            Select(B,EFrontlineFaction(Faction),Index);
            const int32 Target=ObjectiveTrenchV50(B);
            Mirrored&=B->Trenches.IsValidIndex(Target)&&B->GetTeamDirection(0)*B->Trenches[Target].X>0.f;
        }
        Check(TEXT("v50_even_line_supply_and_counterstroke_targets_mirror_with_faction"),Mirrored);
        Check(TEXT("v50_dispatch_receipts_change_actual_readiness_and_morale_both_factions"),Applied);
        Check(TEXT("v50_dispatch_conditions_apply_once_per_encounter"),Once);
        Check(TEXT("v50_unacknowledged_dispatch_does_not_grant_supply_effects"),Absent);
        Check(TEXT("v50_dispatch_smoke_relief_applies_to_first_rearm_only_without_free_orders"),SmokeRearm);
        Check(TEXT("v50_enemy_smoke_cannot_consume_player_dispatch_reserve"),EnemySmokeIsolated);
    }
    static bool Select(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index)
    {
        FString UserDir;if(!B||!B->IsProgressionVolatile()||!(((((((((UIPerfAllowedV8213()||FParse::Param(FCommandLine::Get(),TEXT("WFV76Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75UIReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV74Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV73Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV72Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview"))||FParse::Param(FCommandLine::Get(),TEXT("WFV71Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV70Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV69Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview"))||FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV66Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV49Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFPromoV49"))||FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))||(FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))||(FParse::Param(FCommandLine::Get(),TEXT("WFV63Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV64Review"))))||FParse::Param(FCommandLine::Get(),TEXT("WFV52Balance")))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()||Index<0||Index>=20||uint8(F)>1)return false;
        B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.Difficulty=EFrontlineDifficulty::Veteran;
        R.CompletedMissions=Index;R.SelectedMission=Index;R.Research.TechnologyMask=(1u<<27)-1;R.Research.ResearchPoints=17;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->ResetEncounter();return true;
    }
    static bool SelectArtV71(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index,bool Research)
    {
        if(!(((((((UIPerfAllowedV8213()||FParse::Param(FCommandLine::Get(),TEXT("WFV76Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75UIReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV74Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV73Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV72Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview"))||FParse::Param(FCommandLine::Get(),TEXT("WFV71Review")))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!Select(B,F,Index))return false;
        B->CampaignRuns[0].Research.TechnologyMask=Research?(1u<<28)-1:0;
        B->ResetEncounter();return true;
    }
    static bool ArtFieldV73(AWarfareFrontlineGameMode* B)
    {
        if(!B||!B->IsProgressionVolatile()||!((((FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75UIReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV74Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV73Review")))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
        const int32 C=FrontlineBattlefield::CentreTrench;
        const bool Rifle=B->SpawnSquad(0,EFrontlineRole::Rifle,C-1);
        const bool MG=B->SpawnSquad(1,EFrontlineRole::Lewis,C+1);
        const bool Marksman=B->SpawnSquad(0,EFrontlineRole::Marksman,C);
        return Rifle&&MG&&Marksman;
    }
    static void ObjectiveChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("v41_objective_checks_require_isolated_profile"),false);return;}
        const auto Saved=B->CampaignRuns;const auto Id=B->ActiveCampaignId;const auto F=B->PlayerFaction;const bool Ticking=B->IsActorTickEnabled();
        ON_SCOPE_EXIT{B->AbandonBattleToOperations();B->CampaignRuns=Saved;B->ActiveCampaignId=Id;B->PlayerFaction=F;B->ResetEncounter();B->SetActorTickEnabled(Ticking);};
        B->SetActorTickEnabled(false);bool Limited=true,Depth=true,Supply=true,Telephone=true;
        for(int32 TeamFaction=0;TeamFaction<2;++TeamFaction)
        {
            Select(B,EFrontlineFaction(TeamFaction),0);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            const int32 Centre=FrontlineBattlefield::CentreTrench,Next=Centre+B->GetTeamDirection(0);
            B->Trenches[Centre].Owner=B->Trenches[Next].Owner=0;
            Limited&=!FrontlineCampaignV41::HasObjectiveVictory(B,0);
            Limited&=B->SpawnSquad(0,EFrontlineRole::Rifle,Next)&&!FrontlineCampaignV41::HasObjectiveVictory(B,0);
            B->Trenches[Centre].Owner=1;Limited&=!FrontlineCampaignV41::HasObjectiveVictory(B,0);B->Trenches[Centre].Owner=0;
            Select(B,EFrontlineFaction(TeamFaction),8);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            const int32 End=Centre+2*B->GetTeamDirection(0),Near=Centre+B->GetTeamDirection(0);
            B->Trenches[Centre].Owner=B->Trenches[Near].Owner=0;B->SpawnSquad(0,EFrontlineRole::Rifle,Near);
            Depth&=!FrontlineCampaignV41::HasObjectiveVictory(B,0);B->Trenches[End].Owner=0;B->SpawnSquad(0,EFrontlineRole::Rifle,End);
            Depth&=!FrontlineCampaignV41::HasObjectiveVictory(B,0);
            auto& Advancing=B->Squads.Last();Advancing.bAdvancing=true;
            for(const auto& Member:Advancing.Members)if(auto* U=Member.Get())
                U->SetActorLocation(FVector(B->GetTeamDirection(0)*(FrontlineBattlefield::GoalHalfLength+50.f),U->Feet().Y,200.f));
            Depth&=FrontlineCampaignV41::HasObjectiveVictory(B,0);
            Telephone&=FrontlineCampaignV41::TelephoneResponseRelief(B,0)==2.f;B->Trenches[Near].Owner=1;
            Telephone&=FrontlineCampaignV41::TelephoneResponseRelief(B,0)==0.f;
        }
        Select(B,EFrontlineFaction::British,2);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;B->Elapsed=100;
        const int32 Centre=FrontlineBattlefield::CentreTrench;B->Trenches[Centre].Owner=0;B->SpawnSquad(0,EFrontlineRole::Rifle,Centre);
        B->RecruitReadyAt[0][0]=140;B->MortarReadyAt[0]=160;B->SmokeReadyAt[0]=170;
        const int32 UnitsBefore=B->Units.Num();FrontlineCampaignV41::TickObjective(B,0);
        Supply&=B->GetEncounterCampaignMission()->bSupplyBayV41&&B->RecruitReadyAt[0][0]==132&&B->MortarReadyAt[0]==148&&B->SmokeReadyAt[0]==158;
        FrontlineCampaignV41::TickObjective(B,0);Supply&=B->RecruitReadyAt[0][0]==132&&B->Units.Num()==UnitsBefore;
        Check(TEXT("v43_captured_forward_post_alone_does_not_end_battle"),Limited);
        Check(TEXT("v43_deep_operation_requires_crossing_enemy_end_both_factions"),Depth);
        Check(TEXT("v41_supply_capture_applies_once_without_free_soldiers"),Supply);
        Check(TEXT("v41_forward_telephone_needs_occupied_enemy_post"),Telephone);
        bool Restricted=true;
        for(int32 Faction=0;Faction<2;++Faction)for(int32 Index:{10,18,19})
        {
            Select(B,EFrontlineFaction(Faction),Index);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            const uint32 Mask=B->GetEncounterCampaignMission()->DisabledSupportMask;
            for(int32 Team=0;Team<2;++Team)for(int32 Role=0;Role<int32(EFrontlineSupport::Count);++Role)
                if(Mask&(1u<<Role))Restricted&=!B->IsSupportUnlocked(EFrontlineSupport(Role),Team)&&!B->SupportBlockReason(EFrontlineSupport(Role),Team).IsEmpty();
        }
        Check(TEXT("v41_chapter_support_restrictions_apply_to_both_armies"),Restricted);
    }
    static void Counterstroke(AWarfareFrontlineGameMode* B)
    {
        B->EnemyCounterattackTrench=ObjectiveTrenchV50(B);
        B->EnemyCounterattackUntil=B->Elapsed+55.f;
        B->EnemyRecruitAt=FMath::Min(B->EnemyRecruitAt,B->Elapsed+2.f);
        B->EnemyAssaultAt=FMath::Min(B->EnemyAssaultAt,B->Elapsed+3.f);
        // Readiness and reinforcement entry remain authoritative: no free men,
        // instant cooldown completion or troop generation behind the camera.
    }
};

namespace FrontlineCampaignV41
{
bool PrepareReviewOperation(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index)
{return FFrontlineCampaignV41Access::Select(B,F,Index);}
bool PrepareArtReviewOperationV71(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index,bool Research)
{return FFrontlineCampaignV41Access::SelectArtV71(B,F,Index,Research);}
bool PopulateArtReviewFieldV73(AWarfareFrontlineGameMode* B)
{return FFrontlineCampaignV41Access::ArtFieldV73(B);}
int32 RemapLegacyMission(int32 OldIndex)
{return OldIndex==10?20:OldIndex>=0&&OldIndex<10?LegacyMap[OldIndex]:INDEX_NONE;}
int32 TrenchCount(int32 MissionIndex)
{
    return FrontlineBattlefield::OperationTrenchCount(MissionIndex);
}
int32 LegacyStage(int32 MissionIndex)
{
    if(MissionIndex<0)return INDEX_NONE;if(MissionIndex>=20)return 10;
    for(int32 I=9;I>=0;--I)if(MissionIndex>=LegacyMap[I])return I;
    return 0;
}
bool MigrateLegacy(FFrontlineCampaignProgress& R)
{
    if(R.RouteRevision==RouteRevision)return true;
    if(R.RouteRevision!=0 || R.CompletedMissions<0 || R.CompletedMissions>10 || R.SelectedMission<0
        || R.SelectedMission>=10 || R.SelectedMission>R.CompletedMissions || (R.AcknowledgedEventMask&~1023u))return false;
    R.CompletedMissions=RemapLegacyMission(R.CompletedMissions);R.SelectedMission=RemapLegacyMission(R.SelectedMission);
    uint32 Events=0;for(int32 I=0;I<10;++I)if(R.AcknowledgedEventMask&(1u<<I))Events|=1u<<LegacyMap[I];
    R.AcknowledgedEventMask=Events;R.RouteRevision=RouteRevision;return true;
}
bool RoleAllowed(EFrontlineFaction F,EFrontlineRole R)
{return R==EFrontlineRole::Signals?false:R==EFrontlineRole::RifleGrenadier?F==EFrontlineFaction::British:R==EFrontlineRole::AntiTankRifle?F==EFrontlineFaction::German:uint8(R)<uint8(EFrontlineRole::Count);}
int32 RoleArrival(EFrontlineFaction F,EFrontlineRole R)
{
    if(!RoleAllowed(F,R))return MAX_int32;
    // The first eight assignments introduce usable combined-arms choices.
    // Existing 1917 equipment is allocated earlier; MP18 and Tankgewehr retain
    // their independent 1918 gates. Schilt remains an allied flame attachment.
    static constexpr int32 UK[]={0,1,1,3,4,6,2,2,MAX_int32,1,MAX_int32};
    static constexpr int32 DE[]={0,1,0,3,4,6,2,MAX_int32,14,1,MAX_int32};
    return F==EFrontlineFaction::British?UK[int32(R)]:DE[int32(R)];
}
void Expand(TArray<FFrontlineCampaignMission>& Missions,bool G)
{
    const auto Legacy=Missions;check(Legacy.Num()==10);Missions.Reset(MissionCount);
    static const TCHAR* Dates[]={TEXT("9 APRIL 1917"),TEXT("12 APRIL 1917"),TEXT("3 MAY 1917"),TEXT("7 JUNE 1917"),TEXT("9 JUNE 1917"),TEXT("31 JULY 1917"),TEXT("16 AUGUST 1917"),TEXT("20 SEPTEMBER 1917"),TEXT("20 NOVEMBER 1917"),TEXT("30 NOVEMBER 1917"),TEXT("12 FEBRUARY 1918"),TEXT("21 MARCH 1918"),TEXT("9 APRIL 1918"),TEXT("24 APRIL 1918"),TEXT("8 AUGUST 1918"),TEXT("26 AUGUST 1918"),TEXT("29 SEPTEMBER 1918"),TEXT("17 OCTOBER 1918"),TEXT("1 NOVEMBER 1918"),TEXT("4 NOVEMBER 1918")};
    static constexpr int32 UKPortrait[]={0,0,2,1,1,2,2,1,3,3,3,4,6,5,5,6,7,8,6,9};
    static constexpr int32 DEPortrait[]={0,0,6,1,1,2,2,1,3,3,3,4,10,5,5,6,7,8,9,9};
    static constexpr float Depth[]={.88f,.89f,.91f,.93f,.91f,.95f,.92f,.93f,.99f,.94f,.87f,.97f,.94f,.98f,1.f,.98f,.98f,.96f,.92f,.96f};
    static const FVector2D PlacesXY[]={{2.777,50.292},{2.896,50.296},{2.927,50.193},{2.895,50.765},{2.897,50.770},{2.882,50.884},{2.924,50.913},{2.982,50.828},{3.234,50.176},{3.117,50.177},{3.071,50.112},{3.181,49.861},{2.697,50.645},{2.519,49.867},{2.296,49.895},{2.931,50.301},{3.260,49.963},{3.495,50.068},{3.524,50.354},{3.638,50.043}};
    static const TCHAR* Places[]={TEXT("法国 · 阿拉斯"),TEXT("法国 · 勒厄"),TEXT("法国 · 布勒库尔"),TEXT("比利时 · 梅西讷"),TEXT("比利时 · 梅西讷"),TEXT("比利时 · 皮尔克姆"),TEXT("比利时 · 朗厄马尔克"),TEXT("比利时 · 梅宁路"),TEXT("法国 · 康布雷"),TEXT("法国 · 布尔隆"),TEXT("法国 · 康布雷战区"),TEXT("法国 · 圣康坦以西"),TEXT("法国 · 利斯河"),TEXT("法国 · 维莱布勒托讷"),TEXT("法国 · 亚眠"),TEXT("法国 · 斯卡尔普河"),TEXT("法国 · 圣康坦运河"),TEXT("法国 · 塞勒河"),TEXT("法国 · 瓦朗谢讷"),TEXT("法国 · 桑布尔与勒凯努瓦")};
    for(int32 I=0;I<MissionCount;++I)
    {
        const auto& A=AuthoredV41[I];FFrontlineCampaignMission M=Legacy[A.Art];
        M.Id=I;M.LegacyArtV41=A.Art;M.CalendarDateV41=A.Date;M.Year=A.Date/10000;M.DateLabel=Dates[I];
        if(G && I==8){M.CalendarDateV41=19171130;M.DateLabel=TEXT("30 NOVEMBER 1917");}
        M.MapPosition=PlacesXY[I];M.MapKey=A.Key;M.Title=G?A.GermanTitle:A.BritishTitle;M.Location=Places[I];M.HistoricalContext=A.History;
        M.Briefing=G?A.GermanOrders:A.BritishOrders;M.TerrainNotes=A.Terrain;
        M.TerrainStyleV32=I==0?EFrontlineMissionTerrainV32::Ridge:I==19?EFrontlineMissionTerrainV32::Canal:A.Style;
        // Rain is authored independently of wet ground: the shell holes at
        // Messines remain wet after the shower, while Amiens begins in dry fog.
        static constexpr float Rain[]={.18f,.07f,.03f,.01f,.02f,.82f,.68f,.04f,0.f,.24f,.04f,.015f,.56f,.06f,0.f,.025f,.09f,.49f,.12f,.16f};
        static constexpr float Sight[]={.91f,.96f,1.f,.99f,.86f,.80f,.82f,.96f,.98f,.91f,.79f,.74f,.83f,.94f,.78f,1.f,.85f,.87f,.96f,.90f};
        M.Wetness=A.Wet;M.RainBase=Rain[I];M.RainVariation=Rain[I]>.4f?.19f:Rain[I]>.10f?.08f:.025f;
        M.VisibilityScale=Sight[I];
        M.MudMoveScale=A.Wet>.85f?.88f:.96f;M.BattlefieldHalfWidth=A.Width;M.WireDensity=A.Wire;M.MineDensity=A.Mines;
        M.ObjectiveLinesV41=((I==11&&!G)||(G&&I>=14))?2:A.Lines;M.Objective=EFrontlineCampaignObjective::Breakthrough;M.HoldCenterSeconds=0;
        M.ObjectiveText=M.ObjectiveLinesV41==2?TEXT("肃清敌军后沿并整队突破，或击溃敌军士气。"):TEXT("肃清敌军后沿并整队突破，或击溃敌军士气。");
        M.bCounterstrokeV41=A.Counter;M.bSupplyBayV41=A.Depot;M.DepthScaleV41=Depth[I];M.RandomSeed=41000+(G?701:0)+I*97;
        M.CommanderPortraitV41[0]=UKPortrait[I];M.CommanderPortraitV41[1]=DEPortrait[I];
        M.EnemyMorale=I<3?96.f+I*2:103.f+FMath::Min(I,14)*.5f;M.PlayerMorale=100;
        M.EnemyRecruitScale=I<3?1.13f-I*.02f:1.05f-FMath::Min(I,16)*.006f;
        M.EnemyAssaultDelay=I<3?12.f:9.f;M.PlayerArtilleryDelay=65.f;M.EnemyArtilleryDelay=I<3?125.f:95.f;
        M.PlayerSmokeDelay=I?0.f:12.f;M.PlayerMortarDelay=I<5?35.f:25.f;
        M.DisabledRoleMask=0;M.DisabledSupportMask=0;M.bArmourAllowedV41=I!=10;M.RestrictionText=TEXT("");M.AlliedContext=TEXT("");
        for(int32 R=0;R<int32(EFrontlineRole::Count);++R)if(I<RoleArrival(G?EFrontlineFaction::German:EFrontlineFaction::British,EFrontlineRole(R)))M.DisabledRoleMask|=1u<<R;
        M.EnemyRoleOrder.Reset();M.EnemyOpeningRoles.Reset();
        const EFrontlineFaction EF=G?EFrontlineFaction::British:EFrontlineFaction::German;
        M.EnemyRoleOrder.Add(EFrontlineRole::Rifle);
        for(EFrontlineRole R:{EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Pioneer,EFrontlineRole::Flamethrower,EFrontlineRole::RifleGrenadier,EFrontlineRole::AntiTankRifle,EFrontlineRole::MobileGun,EFrontlineRole::Signals})
            if(I>=RoleArrival(EF,R))M.EnemyRoleOrder.Add(R);
        M.EnemyOpeningRoles={EFrontlineRole::Rifle,EFrontlineRole::Rifle};
        if(I>=RoleArrival(EF,EFrontlineRole::Lewis))M.EnemyOpeningRoles.Insert(EFrontlineRole::Lewis,1);
        M.bEnemyScopedMarksmen=G&&I>=3;M.bEnemyStormtroopers=!G&&I>=11;M.bEnemyCommissionedOfficers=I>=4;
        M.bArmoredTrain=I==17;M.TrainTeam=G?0:1;
        if(I==10)
        {
            for(int32 S=0;S<int32(EFrontlineSupport::Count);++S)if(EFrontlineSupport(S)!=EFrontlineSupport::Smoke)M.DisabledSupportMask|=1u<<S;
            M.DisabledRoleMask|=1u<<int32(EFrontlineRole::Flamethrower);
            M.RestrictionText=TEXT("冬季巡逻只配属轻装分队与烟幕，不接通重炮、机队和装甲。");
        }
        if(I>=18){M.DisabledSupportMask=(1u<<int32(EFrontlineSupport::Gas))|(1u<<int32(EFrontlineSupport::AirBombing));M.RestrictionText=TEXT("居民区邻段不批准毒气与空中轰炸。");}
        if(M.bSupplyBayV41)M.AlliedContext=TEXT("夺下中央补给湾后，携弹队可缩短下一轮整备。");
        if(M.bCounterstrokeV41)M.AlliedContext=TEXT("中央支点易手后，敌军预备队会沿原通路组织反击。");
        Missions.Add(M);
    }
    FrontlinePlayerGuideV43::ApplyCampaign(Missions,G);
    FrontlineCampaignV51::ApplyCampaign(Missions,G);
}
bool HasObjectiveVictory(const AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||Team<0||Team>1||B->Phase!=EFrontlinePhase::Fighting)return false;
    for(const auto& Squad:B->Squads)if(Squad.Team==Team&&FrontlineGameplayV43::HasCrossedEnemyEnd(B,Squad))return true;
    return false;
}
void TickObjective(AWarfareFrontlineGameMode* B,float)
{
    if(!B||B->Phase!=EFrontlinePhase::Fighting)return;
    FrontlineCampaignV52::Tick(B);
    const auto* M=B->GetEncounterCampaignMission();const int32 ObjectiveTrench=ObjectiveTrenchV50(B);
    if(!M||!B->Trenches.IsValidIndex(ObjectiveTrench))return;
    for(auto It=StatesV41.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& S=StatesV41.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){S={};S.Encounter=B->GetEncounterId();}
    if(!S.DispatchApplied)
    {
        S.DispatchApplied=true;
        if(const auto* Run=B->GetCurrentCampaign();Run&&Run->StrategicRevisionV52!=52)
        {
            const auto D=FrontlineCampaignV32::DispatchConditionsV50(M->Id,Run->AcknowledgedEventMask);
            B->ArtilleryReadyAt[0]=FMath::Max(B->Elapsed,B->ArtilleryReadyAt[0]-D.ArtilleryAdvance);
            B->MortarReadyAt[0]=FMath::Max(B->Elapsed,B->MortarReadyAt[0]-D.MortarAdvance);
            S.SmokeRearmReserve=D.SmokeAdvance;S.LastPlayerSmokeReady=M->PlayerSmokeDelay;
            B->Morale[0]+=D.MoraleReserve;
        }
    }
    // Opening smoke is already available in these operations. Reserve the
    // ammunition benefit for the first successful player smoke rearm instead.
    // Both a new command and the player's own timer transition are required:
    // enemy smoke, gas clouds and passive tank smoke cannot spend this reserve.
    if(S.SmokeRearmReserve>0.f&&B->SmokeCommands>S.ObservedSmokeCommands
        &&B->SmokeReadyAt[0]>S.LastPlayerSmokeReady+.5f&&B->SmokeReadyAt[0]>B->Elapsed)
    {
        B->SmokeReadyAt[0]=FMath::Max(B->Elapsed,B->SmokeReadyAt[0]-S.SmokeRearmReserve);
        S.SmokeRearmReserve=0.f;
    }
    S.ObservedSmokeCommands=B->SmokeCommands;S.LastPlayerSmokeReady=B->SmokeReadyAt[0];
    if(B->Trenches[ObjectiveTrench].Owner!=0||B->TrenchSquadCount(ObjectiveTrench,0)==0)return;
    if(!S.CentreSeen){S.CentreSeen=true;if(M->bCounterstrokeV41){FFrontlineCampaignV41Access::Counterstroke(B);B->LastMessage=TEXT("中央支点已夺取。留一队掩护，敌军预备队正在接近。");}}
    if(M->bSupplyBayV41&&!S.DepotDelivered)
    {
        S.DepotDelivered=true;for(int32 R=0;R<int32(EFrontlineRole::Count);++R)B->RecruitReadyAt[0][R]=FMath::Max(B->Elapsed,B->RecruitReadyAt[0][R]-8.f);
        B->MortarReadyAt[0]=FMath::Max(B->Elapsed,B->MortarReadyAt[0]-12.f);B->SmokeReadyAt[0]=FMath::Max(B->Elapsed,B->SmokeReadyAt[0]-12.f);
        B->LastMessage=TEXT("中央补给湾接通。携弹队正在为下一轮出击整备。");
    }
}
float TelephoneResponseRelief(const AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||Team<0||Team>1||!B->HasTechnology(26,Team))return 0;
    const int32 I=FrontlineBattlefield::TrenchCount%2?
        FrontlineBattlefield::CentreTrench+B->GetTeamDirection(Team):ObjectiveTrenchV50(B,Team);
    return B->Trenches.IsValidIndex(I)&&B->Trenches[I].Owner==Team&&B->TrenchSquadCount(I,Team)>0?2.f:0.f;
}
FString ObjectiveConclusion(const AWarfareFrontlineGameMode* B,int32 Team)
{return Team==0?TEXT("本连的通路已经接通，接替部队可以通过。"):TEXT("敌军切断了本连最后的交通线。");}
UTexture2D* NewMap(const FFrontlineCampaignMission& M)
{
    if(FString(M.MapKey)==TEXT("arras")||FString(M.MapKey)==TEXT("messines")||FString(M.MapKey)==TEXT("ypres")||FString(M.MapKey)==TEXT("cambrai")||FString(M.MapKey)==TEXT("michael")||FString(M.MapKey)==TEXT("amiens")||(M.Id>=15&&M.Id!=18))return nullptr;
    return TextureV41(FString::Printf(TEXT("/Game/Warfare/UI/V41/Maps/T_Map_%s"),M.MapKey));
}
UTexture2D* QuastPortrait(){return TextureV41(TEXT("/Game/Warfare/UI/V41/Commanders/T_Commander_Quast"));}
UTexture2D* RolePortrait(EFrontlineRole R)
{return R==EFrontlineRole::RifleGrenadier?TextureV41(TEXT("/Game/Warfare/UI/V41/T_Portrait_British_RifleGrenadier")):R==EFrontlineRole::AntiTankRifle?TextureV41(TEXT("/Game/Warfare/UI/V41/T_Portrait_German_AntiTankRifle")):nullptr;}
UTexture2D* TechnologyPortrait(int32 Id,EFrontlineFaction F)
{return Id>=24&&Id<27?TextureV41(FString::Printf(TEXT("/Game/Warfare/UI/V41/Technology/T_Tech_%s_%d"),F==EFrontlineFaction::British?TEXT("British"):TEXT("German"),Id)):nullptr;}
void AppendUIPreloadPathsV8213(TArray<FString>& Paths)
{
    // The active campaign catalogue supplies the same ten map bindings as
    // NewMap. Existing late maps and the restored Riqueval survey have their
    // own accepted bindings and are not substituted here.
    for(int32 I=0;I<UE_ARRAY_COUNT(AuthoredV41);++I)
    {
        const FString Key=AuthoredV41[I].Key;
        if(Key==TEXT("arras")||Key==TEXT("messines")||Key==TEXT("ypres")||Key==TEXT("cambrai")||Key==TEXT("michael")||Key==TEXT("amiens")||(I>=15&&I!=18))continue;
        Paths.Add(FString::Printf(TEXT("/Game/Warfare/UI/V41/Maps/T_Map_%s"),*Key));
    }
    Paths.Add(TEXT("/Game/Warfare/UI/V41/Commanders/T_Commander_Quast"));
    Paths.Add(TEXT("/Game/Warfare/UI/V41/T_Portrait_British_RifleGrenadier"));
    Paths.Add(TEXT("/Game/Warfare/UI/V41/T_Portrait_German_AntiTankRifle"));
    for(int32 Id=24;Id<27;++Id)Paths.Add(FString::Printf(TEXT("/Game/Warfare/UI/V41/Technology/T_Tech_British_%d"),Id));
    // German 24/26 are supplied by the accepted V71 atlas. Only 25 still
    // resolves to the original V41 painting in ordinary production UI.
    Paths.Add(TEXT("/Game/Warfare/UI/V41/Technology/T_Tech_German_25"));
}
void AppendTechnologyNodes(TArray<FFrontlineTechnologyNode>& N,bool G)
{
    N.Append({
        {24,G?10:4,0,6,2,G?TEXT("Tankgewehr 支撑架"):TEXT("No.23 枪榴弹操练"),G?TEXT("反坦克步枪散布缩小 15%，命中后的击穿机会提高。"):TEXT("枪榴弹落点散布半径缩小 15%。"),-1,G?TEXT("German_24"):TEXT("British_24"),TEXT("专门分队")},
        {25,24,1,6,2,G?TEXT("反坦克枪副射手"):TEXT("枪榴弹携弹手"),G?TEXT("专门射手附近有存活的同队携弹手时，装填时间缩短 15%。"):TEXT("专门射手附近有存活的同队携弹手时，装填时间缩短 15%。"),5,G?TEXT("German_25"):TEXT("British_25"),TEXT("携弹联络")},
        {26,3,2,6,3,G?TEXT("前沿电话所"):TEXT("前沿电话线"),TEXT("驻守中央之后的第一道敌壕时，间接火力首轮提前 2 秒抵达。"),20,G?TEXT("German_26"):TEXT("British_26"),TEXT("前沿电话")}
    });
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    bool Routes=true,Dates=true,Unique=true,Profiles=true;
    for(int32 F=0;F<2;++F)
    {
        const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction(F));TSet<FString> Titles;
        Routes&=Missions.Num()==20;int32 Date=0,Limited=0,Counter=0,Depot=0;
        for(const auto& M:Missions){Dates&=M.CalendarDateV41>=Date;Date=M.CalendarDateV41;Titles.Add(M.Title);Limited+=M.ObjectiveLinesV41==2;Counter+=M.bCounterstrokeV41;Depot+=M.bSupplyBayV41;Profiles&=M.ObjectiveLinesV41>=2&&M.ObjectiveLinesV41<=3&&M.WireDensity<=1&&M.DepthScaleV41>=.85f;}
        Unique&=Titles.Num()==20;Profiles&=Limited>=8&&Counter>=4&&Depot>=4;
    }
    Check(TEXT("v41_two_twenty_operation_routes"),Routes);Check(TEXT("v41_historical_dates_chronological"),Dates);Check(TEXT("v41_forty_faction_orders_and_unique_titles"),Unique);Check(TEXT("v41_distinct_limited_counterstroke_supply_profiles"),Profiles);
    bool Migration=true;
    for(int32 C=0;C<=10;++C)for(int32 S=0;S<=FMath::Min(C,9);++S)
    {
        FFrontlineCampaignProgress R;R.RouteRevision=0;R.CompletedMissions=C;R.SelectedMission=S;R.AcknowledgedEventMask=1023;R.Research.ResearchPoints=17;R.Research.TechnologyMask=0x345;R.RoleSelectionMask=13;const auto Id=R.Id=FGuid::NewGuid();
        Migration&=MigrateLegacy(R)&&R.CompletedMissions==RemapLegacyMission(C)&&R.SelectedMission==RemapLegacyMission(S)&&R.Id==Id&&R.Research.ResearchPoints==17&&R.Research.TechnologyMask==0x345&&R.RoleSelectionMask==13;
        const auto Before=R;Migration&=MigrateLegacy(R)&&R.CompletedMissions==Before.CompletedMissions&&R.AcknowledgedEventMask==Before.AcknowledgedEventMask;
    }
    Check(TEXT("v41_all_sixty_five_legacy_boundaries_migrate_once_without_rewards"),Migration);
    Check(TEXT("v41_faction_specialist_slots_stable_and_exclusive"),int32(EFrontlineRole::RifleGrenadier)==7&&int32(EFrontlineRole::AntiTankRifle)==8&&!RoleAllowed(EFrontlineFaction::German,EFrontlineRole::RifleGrenadier)&&!RoleAllowed(EFrontlineFaction::British,EFrontlineRole::AntiTankRifle));
    bool Graphs=true,Art=true;
    for(int32 F=0;F<2;++F)
    {
        const auto& Nodes=AWarfareFrontlineGameMode::GetTechnologyNodesForFaction(EFrontlineFaction(F));Graphs&=Nodes.Num()==27;
        for(const auto& N:Nodes)Graphs&=N.Id>=0&&N.Id<27&&N.Prerequisite<N.Id&&N.SecondaryPrerequisite<N.Id&&N.Branch>=0&&N.Branch<4;
        for(int32 Id=24;Id<27;++Id)Art&=TechnologyPortrait(Id,EFrontlineFaction(F))!=nullptr;
    }
    int32 NewMaps=0;for(const auto& M:AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British))if(NewMap(M))++NewMaps;
    Art&=NewMaps==10&&QuastPortrait()&&RolePortrait(EFrontlineRole::RifleGrenadier)&&RolePortrait(EFrontlineRole::AntiTankRifle);
    Check(TEXT("v41_twenty_seven_acyclic_technology_nodes_each_faction"),Graphs);
    Check(TEXT("v41_nineteen_native_campaign_portrait_technology_textures"),Art);
    FFrontlineCampaignV41Access::ObjectiveChecks(B,Check);
}
void RunChecksV50(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    bool Dates=true,Unlocks=true,Copy=true,Orders=true,Isolation=true;TSet<FString> Briefings,Premises;TSet<int32> Counts,Styles;
    int32 Dispatches=0;
    for(int32 F=0;F<2;++F)
    {
        const auto Faction=EFrontlineFaction(F);int32 PreviousDate=0;
        const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(Faction);
        Copy&=Missions.Num()==MissionCount;
        for(const auto& M:Missions)
        {
            Dates&=M.CalendarDateV41>=PreviousDate;PreviousDate=M.CalendarDateV41;
            Counts.Add(TrenchCount(M.Id));Styles.Add(int32(M.TerrainStyleV32));
            Briefings.Add(M.Briefing);Premises.Add(M.AlliedContext);
            Copy&=FCString::Strlen(M.Briefing)>100&&FCString::Strlen(M.AlliedContext)>55;
            const auto Enemy=F?EFrontlineFaction::British:EFrontlineFaction::German;
            for(const auto R:M.EnemyRoleOrder)Unlocks&=RoleAllowed(Enemy,R)&&RoleArrival(Enemy,R)<=M.Id;
            for(const auto R:M.EnemyOpeningRoles)Unlocks&=M.EnemyRoleOrder.Contains(R);
            const auto E=FrontlineCampaignV32::EventForMission(Faction,M.Id);
            if(E.Id!=INDEX_NONE)
            {
                ++Dispatches;
                for(const TCHAR* Text:{E.Title,E.Body,E.Consequence})
                    Copy&=*Text&&FrontlineLocalizationV35::English(Text)!=Text;
            }
            const auto None=FrontlineCampaignV32::DispatchConditionsV50(M.Id,0);
            Isolation&=!None.IsActive();
        }
    }
    for(int32 Origin:{3,5,8,11,14,16,18})
    {
        for(int32 I=0;I<MissionCount;++I)
            Orders&=FrontlineCampaignV32::DispatchConditionsV50(I,1u<<Origin).IsActive()==(I==Origin||I==Origin+1);
    }
    Check(TEXT("v50_forty_distinct_bilingual_orders_and_tactical_premises"),Copy&&Briefings.Num()==40&&Premises.Num()==40&&Dispatches==24);
    Check(TEXT("v50_chronology_and_historically_available_enemy_attachments"),Dates&&Unlocks);
    Check(TEXT("v50_routes_use_two_three_four_positions_and_six_terrain_families"),Counts.Num()==3&&Counts.Contains(2)&&Counts.Contains(3)&&Counts.Contains(4)&&Styles.Num()==6);
    Check(TEXT("v50_dispatch_orders_cover_only_two_linked_operations"),Orders&&Isolation);
    FFrontlineCampaignV41Access::DispatchChecks(B,Check);
}
}
