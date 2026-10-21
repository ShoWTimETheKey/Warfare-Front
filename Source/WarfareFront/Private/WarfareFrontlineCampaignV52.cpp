#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUnlocksV53.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr uint32 MissionMask=(1u<<20)-1;
constexpr int32 Forks[]={2,6,12,17};
FString L(const TCHAR* English,const TCHAR* Chinese)
{return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?Chinese:English;}
int32 ForkFirst(int32 Id){for(int32 F:Forks)if(Id==F||Id==F+1)return F;return INDEX_NONE;}
uint32 Prefix(int32 Count){return Count>0?(1u<<FMath::Clamp(Count,0,20))-1:0;}
uint32 Wins(const FFrontlineCampaignProgress& R){return R.StrategicRevisionV52==52?R.WonOperationMaskV52:Prefix(R.CompletedMissions);}
uint32 Bypasses(const FFrontlineCampaignProgress& R){return R.StrategicRevisionV52==52?R.BypassedOperationMaskV52:0;}
int32 DispatchOrder(const FFrontlineCampaignProgress& R,int32 Id)
{
    if(Id>=0&&Id<20&&(R.DispatchOrderMaskV52&(1u<<Id)))return Id;
    // The late railway/city alternatives draw on the same allocation. Switching
    // an uncommitted assignment cannot collect a second research grant.
    if(Id==17||Id==18){const int32 Other=35-Id;if(R.DispatchOrderMaskV52&(1u<<Other))return Other;}
    return INDEX_NONE;
}
bool Available(const FFrontlineCampaignProgress& R,int32 Id)
{
    if(Id<0||Id>=20||(Bypasses(R)&(1u<<Id)))return false;
    if(Wins(R)&(1u<<Id))return true;
    if(Id==R.CompletedMissions)return true;
    // Only the uncommitted alternative at the current fork is selectable.
    return ForkFirst(R.CompletedMissions)==R.CompletedMissions&&Id==R.CompletedMissions+1;
}
bool Integer(const FJsonObject& O,const TCHAR* Key,uint32& Out)
{
    double V=0;if(!O.TryGetNumberField(Key,V)||!FMath::IsFinite(V)||V<0||V>MissionMask||V!=FMath::FloorToDouble(V))return false;
    Out=uint32(V);return true;
}
struct FRuntime {FGuid Encounter;bool Delivered=false;int32 LoanRecruits=0;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FRuntime> Runtime;
}

struct FFrontlineCampaignV52Access
{
    static bool Resolve(AWarfareFrontlineGameMode* B,int32 Choice)
    {
        using namespace FrontlineCampaignV52;
        if(!B||B->Phase!=EFrontlinePhase::Briefing||B->bProgressionReadOnly)return false;
        const auto D=Dispatch(B);if(!D.bPending||!D.Choices.IsValidIndex(Choice))return false;
        auto* R=B->CampaignRuns.FindByPredicate([B](const auto& V){return V.Id==B->ActiveCampaignId;});if(!R)return false;
        const auto Before=*R;Initialize(*R);const uint32 Bit=1u<<D.Id;
        R->DispatchResolvedMaskV52|=Bit;R->DispatchOrderMaskV52|=Bit;
        if(Choice)R->DispatchChoiceMaskV52|=Bit;else R->DispatchChoiceMaskV52&=~Bit;
        R->AcknowledgedEventMask|=Bit;
        R->Research.ResearchPoints=FMath::Min(99999,R->Research.ResearchPoints+D.Choices[Choice].Research);
        R->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
        if(!B->SaveProgression()){*R=Before;B->LastMessage=B->ProgressionSaveStatus;return false;}
        B->LastMessage=D.Choices[Choice].Effect;return true;
    }
    static void Deliver(AWarfareFrontlineGameMode* B)
    {
        using namespace FrontlineCampaignV52;
        if(!B||B->Phase!=EFrontlinePhase::Fighting||B->Elapsed<8.f)return;
        const auto* M=B->GetEncounterCampaignMission();const auto* R=B->GetCurrentCampaign();
        if(!M||!R||R->StrategicRevisionV52!=52)return;
        const int32 Order=DispatchOrder(*R,M->Id);if(Order==INDEX_NONE)return;
        for(auto It=Runtime.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
        auto& S=Runtime.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){S={};S.Encounter=B->GetEncounterId();}
        if(S.Delivered)return;
        const auto D=Dispatch(B,M->Id);const int32 Choice=(R->DispatchChoiceMaskV52&(1u<<Order))?1:0;
        if(!D.Choices.IsValidIndex(Choice)){S.Delivered=true;return;}
        const int32 Role=D.Choices[Choice].ReinforcementRole;
        if(Role==INDEX_NONE){S.Delivered=true;return;}
        // The ordinary spawn path brings a real squad through the field edge.
        // No trench placement, no cooldown reduction and no duplicate wave.
        if(B->SpawnSquad(0,EFrontlineRole(Role)))
        {S.Delivered=true;B->LastMessage=L(TEXT("The attached section is coming up the approach road."),TEXT("配属小队正沿后方接近路赶来。"));}
    }
    static void Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("v52_dispatch_checks_require_isolated_profile"),false);return;}
        const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId;const auto Faction=B->PlayerFaction;
        const bool Ticking=B->IsActorTickEnabled(),ReadOnly=B->bProgressionReadOnly;
        ON_SCOPE_EXIT{B->AbandonBattleToOperations();B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;B->bProgressionReadOnly=ReadOnly;Runtime.Remove(B);B->ResetEncounter();B->SetActorTickEnabled(Ticking);};
        B->SetActorTickEnabled(false);B->AbandonBattleToOperations();B->bProgressionReadOnly=false;
        bool Choice=true,Delivery=true,Once=true,Archive=true,NoSaveMutation=true,Loan=true;
        for(int32 National=0;National<2;++National)
        {
            Choice&=B->StartNewCampaign(EFrontlineFaction(National));
            const int32 Points=B->GetResearchPoints();
            Choice&=Resolve(B,1)&&B->GetResearchPoints()==Points+2&&!Resolve(B,0)&&!Resolve(B,1);
            auto& R=B->CampaignRuns.Last();Archive&=(R.DispatchOrderMaskV52&1u)&&(R.DispatchChoiceMaskV52&1u);
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;B->Elapsed=10;
            const int32 Squads=B->Squads.Num();Deliver(B);Delivery&=B->Squads.Num()==Squads;
            B->AbandonBattleToOperations();Choice&=B->StartNewCampaign(EFrontlineFaction(National));
            B->bProgressionReadOnly=true;const auto Before=*B->GetCurrentCampaign();
            NoSaveMutation&=!Resolve(B,0)&&B->GetCurrentCampaign()->DispatchOrderMaskV52==Before.DispatchOrderMaskV52;
            B->bProgressionReadOnly=false;Choice&=Resolve(B,0);
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;B->Elapsed=7.9f;
            const int32 BeforeSquads=B->Squads.Num();const float Ready=B->RecruitReadyAt[0][0];
            Deliver(B);Delivery&=B->Squads.Num()==BeforeSquads;
            B->Elapsed=8.1f;Deliver(B);Delivery&=B->Squads.Num()==BeforeSquads+1&&B->RecruitReadyAt[0][0]==Ready;
            if(B->Squads.Num()>BeforeSquads)
            {const auto& S=B->Squads.Last();Delivery&=S.Team==0&&S.Trench==INDEX_NONE&&S.bReinforcement&&S.Role==EFrontlineRole::Lewis;}
            Deliver(B);Once&=B->Squads.Num()==BeforeSquads+1;
            const uint32 Research=B->GetActiveResearch().TechnologyMask;const int32 Stage=B->GetCurrentCampaign()->CompletedMissions;
            Loan&=B->IsRoleUnlocked(EFrontlineRole::Lewis)&&B->Recruit(EFrontlineRole::Lewis,0);
            for(float& T:B->RecruitReadyAt[0])T=0;
            Loan&=!B->Recruit(EFrontlineRole::Lewis,0)&&!FrontlineCampaignV52::LoanRecruitBlockReason(B,EFrontlineRole::Lewis,0).IsEmpty()
                &&B->GetCurrentCampaign()->CompletedMissions==Stage&&B->GetActiveResearch().TechnologyMask==Research;
            B->AbandonBattleToOperations();
        }
        Check(TEXT("v52_dispatch_choice_grants_points_once_and_excludes_reinforcement"),Choice&&Archive);
        Check(TEXT("v52_selected_attachment_uses_normal_edge_entry_without_recruit_cooldown"),Delivery);
        Check(TEXT("v52_attachment_arrives_once_per_attempt_and_never_during_preparation"),Once);
        Check(TEXT("v52_readonly_archive_cannot_commit_dispatch_choice"),NoSaveMutation);
        Check(TEXT("v59_loan_has_one_real_replacement_without_permanent_unlock_or_research"),Loan);
    }
};

namespace FrontlineCampaignV52
{
const FTheatre& Theatre()
{
    static FTheatre T;T.Title=L(TEXT("WESTERN FRONT"),TEXT("西线战场"));return T;
}
void Initialize(FFrontlineCampaignProgress& R)
{
    if(R.StrategicRevisionV52==52)return;
    R.StrategicRevisionV52=52;R.WonOperationMaskV52=Prefix(R.CompletedMissions);R.BypassedOperationMaskV52=0;
    // Previously collected dispatches stay collected, with no invented choice
    // and no retroactive reinforcement/research award.
    R.DispatchResolvedMaskV52=R.AcknowledgedEventMask;R.DispatchOrderMaskV52=0;R.DispatchChoiceMaskV52=0;
}
bool ReadArchive(const FJsonObject& O,FFrontlineCampaignProgress& R)
{
    if(!O.HasField(TEXT("strategy_revision"))){Initialize(R);return Available(R,R.SelectedMission);}
    double Revision=0;FString TheatreId;
    if(!O.TryGetNumberField(TEXT("strategy_revision"),Revision)||Revision!=52||!O.TryGetStringField(TEXT("theatre_id"),TheatreId)||TheatreId!=TEXT("western_front"))return false;
    if(!Integer(O,TEXT("won_operations"),R.WonOperationMaskV52)||!Integer(O,TEXT("bypassed_operations"),R.BypassedOperationMaskV52)
        ||!Integer(O,TEXT("dispatch_resolved"),R.DispatchResolvedMaskV52)||!Integer(O,TEXT("dispatch_orders"),R.DispatchOrderMaskV52)||!Integer(O,TEXT("dispatch_choices"),R.DispatchChoiceMaskV52))return false;
    R.StrategicRevisionV52=52;
    if((R.WonOperationMaskV52&R.BypassedOperationMaskV52)||((R.WonOperationMaskV52|R.BypassedOperationMaskV52)!=Prefix(R.CompletedMissions))
        ||(R.DispatchOrderMaskV52&~R.DispatchResolvedMaskV52)||(R.DispatchChoiceMaskV52&~R.DispatchOrderMaskV52))return false;
    for(int32 I=0;I<20;++I)if(R.BypassedOperationMaskV52&(1u<<I))
    {const int32 F=ForkFirst(I),Other=I==F?F+1:F;if(F==INDEX_NONE||!(R.WonOperationMaskV52&(1u<<Other)))return false;}
    return Available(R,R.SelectedMission);
}
void WriteArchive(FJsonObject& O,const FFrontlineCampaignProgress& Original)
{
    auto R=Original;Initialize(R);
    O.SetNumberField(TEXT("strategy_revision"),52);O.SetStringField(TEXT("theatre_id"),TEXT("western_front"));
    O.SetNumberField(TEXT("won_operations"),R.WonOperationMaskV52);O.SetNumberField(TEXT("bypassed_operations"),R.BypassedOperationMaskV52);
    O.SetNumberField(TEXT("dispatch_resolved"),R.DispatchResolvedMaskV52);O.SetNumberField(TEXT("dispatch_orders"),R.DispatchOrderMaskV52);O.SetNumberField(TEXT("dispatch_choices"),R.DispatchChoiceMaskV52);
}
bool RecordVictory(FFrontlineCampaignProgress& R,int32 Id)
{
    if(!Available(R,Id)||(Wins(R)&(1u<<Id)))return false;
    Initialize(R);R.WonOperationMaskV52|=1u<<Id;
    const int32 F=ForkFirst(Id);
    if(F!=INDEX_NONE){const int32 Other=Id==F?F+1:F;if(!(R.WonOperationMaskV52&(1u<<Other)))R.BypassedOperationMaskV52|=1u<<Other;}
    const uint32 Resolved=R.WonOperationMaskV52|R.BypassedOperationMaskV52;
    while(R.CompletedMissions<20&&(Resolved&(1u<<R.CompletedMissions)))++R.CompletedMissions;
    R.SelectedMission=FMath::Min(19,R.CompletedMissions);return true;
}
FString BlockReason(const FFrontlineCampaignProgress& R,int32 Id)
{
    if(Id<0||Id>=20)return L(TEXT("This operation is not part of the campaign."),TEXT("这项行动不在本战役命令中。"));
    if(Bypasses(R)&(1u<<Id))return L(TEXT("This assignment went to a neighbouring formation. The company follows its chosen route."),TEXT("此任务已交友邻，本连继续原定路线。"));
    return Available(R,Id)?FString():L(TEXT("Available after the current assignment."),TEXT("完成当前任务后开放。"));
}
bool CanStart(const AWarfareFrontlineGameMode* B,int32 Id)
{return B&&B->GetCurrentCampaign()&&B->CampaignMissionBlockReason(Id).IsEmpty();}
bool Select(AWarfareFrontlineGameMode* B,int32 Id){return B&&B->SelectCampaignMission(Id);}
int32 ActualVictories(const FFrontlineCampaignProgress& R){return FrontlineCampaignV32::BitCount(Wins(R));}
int32 RouteOperationCount(const FFrontlineCampaignProgress& R)
{
    int32 Count=16;for(int32 F:Forks)if((Wins(R)&(1u<<F))&&!(Bypasses(R)&(1u<<(F+1)))&&R.CompletedMissions>F)++Count;
    return Count;
}
FNode GetNode(const AWarfareFrontlineGameMode* B,int32 Id)
{
    FNode N;if(!B||!B->GetCampaignMissions().IsValidIndex(Id))return N;
    const auto& M=B->GetCampaignMissions()[Id];const auto* R=B->GetCurrentCampaign();
    N.Mission=&M;N.Id=Id;N.LonLat=M.MapPosition;N.Date=M.CalendarDateV41;
    // Amiens is the operation's name, not the August assault's start position:
    // place its pin east of the city, on the Villers/Marcelcave approach sector.
    if(Id==14)N.LonLat=FVector2D(2.57,49.86);
    N.Stage=Id;for(int32 F:Forks)if(Id>F)--N.Stage;
    const int32 F=ForkFirst(Id);if(F!=INDEX_NONE)N.Alternative=Id==F?F+1:F;
    if(R)
    {
        const uint32 Bit=1u<<Id;
        N.State=(Wins(*R)&Bit)?ENodeState::Completed:(Bypasses(*R)&Bit)?ENodeState::Bypassed:Available(*R,Id)?(R->SelectedMission==Id?ENodeState::Selected:ENodeState::Available):ENodeState::Locked;
    }
    static const TCHAR* En[]={TEXT("Awaiting orders"),TEXT("Assignment available"),TEXT("Selected assignment"),TEXT("Completed"),TEXT("Assigned elsewhere")};
    static const TCHAR* Zh[]={TEXT("等待命令"),TEXT("可选择任务"),TEXT("当前任务"),TEXT("已经完成"),TEXT("由友邻承担")};
    N.StateText=L(En[int32(N.State)],Zh[int32(N.State)]);
    if(F!=INDEX_NONE)
    {
        N.BranchTitle=F==2?L(TEXT("ARTOIS OR FLANDERS"),TEXT("阿图瓦，或佛兰德")):F==6?L(TEXT("THE YPRES ASSIGNMENT"),TEXT("伊普尔的两份命令")):F==12?L(TEXT("THE SPRING EMERGENCY"),TEXT("春季告急")):L(TEXT("THE LAST APPROACH"),TEXT("最后的接近线"));
        switch(Id)
        {
        case 2:N.Assignment=L(TEXT("Bullecourt: assault wire and deep trenches, with central stores to contest. Messines passes to a neighbouring company."),TEXT("布勒库尔：攻打铁丝与纵深壕，争夺中央补给湾。选择此地后，梅西讷由友邻承担。"));break;
        case 3:N.Assignment=L(TEXT("Messines: cross the mine craters and clear the ridge guns. Bullecourt passes to a neighbouring company."),TEXT("梅西讷：越过爆破坑岸，清除山脊机枪。选择此地后，布勒库尔由友邻承担。"));break;
        case 6:N.Assignment=L(TEXT("Langemarck: approach concrete posts along narrow paths through floodwater. Menin Road passes to neighbouring troops."),TEXT("朗厄马尔克：沿积水间的窄路接近混凝土据点。选择此地后，梅宁路由友邻承担。"));break;
        case 7:N.Assignment=L(TEXT("Menin Road: concentrate fire along firmer woodland ground and face the reserves. Langemarck passes to neighbouring troops."),TEXT("梅宁路：在较干的林缘集中火力，准备迎击预备队。选择此地后，朗厄马尔克由友邻承担。"));break;
        case 12:N.Assignment=L(TEXT("The Lys: recover the flooded lowland roads and reconnect the flank. Villers-Bretonneux passes to another formation."),TEXT("利斯河：夺回积水低地的道路，接通侧翼。选择此地后，维莱布勒托讷由友邻承担。"));break;
        case 13:N.Assignment=L(TEXT("Villers-Bretonneux: tanks engage on the village approaches. The Lys passes to a neighbouring formation."),TEXT("维莱布勒托讷：坦克在村外接近线交战。选择此地后，利斯河由友邻承担。"));break;
        case 17:N.Assignment=L(TEXT("The Selle: pioneers and anti-armour fire contest the railway cutting. Valenciennes passes to neighbouring troops."),TEXT("塞勒河：工兵与反装甲火力争夺铁路切口。选择此地后，瓦朗谢讷由友邻承担。"));break;
        case 18:N.Assignment=L(TEXT("Valenciennes: take the outer approaches without gas or aerial bombing. The Selle railway passes to neighbouring troops."),TEXT("瓦朗谢讷：夺取城郊通路，禁用毒气与空中轰炸。选择此地后，塞勒河铁路由友邻承担。"));break;
        }
    }
    else N.Assignment=FrontlineLocalizationV35::Text(M.Briefing);
    N.Opposition=L(TEXT("Rifle sections."),TEXT("步枪小队。"));
    auto AppendOppositionV841=[&](const FString& Sentence){const FString Clean=Sentence.TrimStartAndEnd();if(Clean.IsEmpty())return;if(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English&&!N.Opposition.IsEmpty())N.Opposition+=TEXT(" ");N.Opposition+=Clean;};
    if(M.EnemyRoleOrder.Contains(EFrontlineRole::Lewis))AppendOppositionV841(L(TEXT(" Machine-gun posts cover the approaches."),TEXT("机枪阵位封锁接近线。")));
    if(M.EnemyRoleOrder.Contains(EFrontlineRole::Mills))AppendOppositionV841(L(TEXT(" Grenadiers contest the trench corners."),TEXT("投弹手争夺壕角。")));
    if(M.EnemyRoleOrder.Contains(EFrontlineRole::Marksman))AppendOppositionV841(M.bEnemyScopedMarksmen?
        L(TEXT(" Scoped riflemen watch exposed gun crews."),TEXT("镜瞄射手盯住暴露的枪组。")):L(TEXT(" Selected riflemen cover longer approaches."),TEXT("精确射手看守较远接近线。")));
    if(M.EnemyRoleOrder.Contains(EFrontlineRole::Flamethrower))AppendOppositionV841(L(TEXT(" Flame parties threaten nearby cover."),TEXT("喷火小队威胁近处掩体。")));
    if(M.EnemyRoleOrder.Contains(EFrontlineRole::AntiTankRifle))AppendOppositionV841(L(TEXT(" Tankgewehr teams threaten armour."),TEXT("Tankgewehr组威胁装甲。")));
    if(M.bCounterstrokeV41)AppendOppositionV841(L(TEXT(" Reserves counterattack when the central post falls."),TEXT("中央支点失守后，预备队会反击。")));
    if(M.bEnemyStormtroopers)AppendOppositionV841(L(TEXT(" Assault troops carry automatic weapons."),TEXT("突击队携带自动武器。")));
    if(M.bArmoredTrain && M.TrainTeam==1)AppendOppositionV841(L(TEXT(" Railway armour supports the German line."),TEXT("铁路装甲火力支援德军阵线。")));
    if(M.WireDensity>=.65f)AppendOppositionV841(L(TEXT(" Dense wire restricts movement."),TEXT("密集铁丝限制通行。")));
    if(M.MineDensity>=.30f)AppendOppositionV841(L(TEXT(" Suspect ground needs a clearance team."),TEXT("可疑雷区需要工兵检查。")));
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
    {
        FString Tanks;
        for(const auto& Entry:Vehicles->GetRoster(1))
        {
            const int32 Arrival=Vehicles->UnlockMission(Entry.Type,1);
            const bool BritishMarkIV=Entry.Type==EFrontlineVehicle::MarkIVMale||Entry.Type==EFrontlineVehicle::MarkIVFemale||Entry.Type==EFrontlineVehicle::MarkIVPioneer;
            const bool Eligible=M.bArmourAllowedV41&&(BritishMarkIV?(M.Year>1917||Id>=Arrival):(M.Year>=1918&&Id>=Arrival));
            if(Eligible){if(!Tanks.IsEmpty())Tanks+=TEXT(" / ");Tanks+=FrontlineLocalizationV35::Text(Entry.Name);}
        }
        if(!Tanks.IsEmpty())AppendOppositionV841(L(TEXT("Armour may reinforce: "),TEXT("可能增援的装甲："))+Tanks+TEXT("."));
    }
    int32 First=F==INDEX_NONE?Id:F;
    if(R&&Available(*R,Id)&&!(Wins(*R)&(1u<<Id)))
    {
        int32 Previous=INDEX_NONE;for(int32 I=0;I<Id;++I)if(Wins(*R)&(1u<<I))Previous=I;
        First=Previous+1;
    }
    for(int32 Role=0;Role<9;++Role)
    {
        const auto Unit=EFrontlineRole(Role);const int32 Arrival=FrontlineCampaignV41::RoleArrival(B->GetPlayerFaction(),Unit);
        if(Arrival>=First&&Arrival<=Id){if(!N.NewAttachments.IsEmpty())N.NewAttachments+=TEXT(" · ");N.NewAttachments+=FrontlineLocalizationV35::Text(B->GetRecruitName(Unit));}
    }
    for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)
    {
        const int32 Arrival=B->SupportUnlockMission(EFrontlineSupport(I));
        if(Arrival>=First&&Arrival<=Id&&!(M.DisabledSupportMask&(1u<<I)))
        {if(!N.NewAttachments.IsEmpty())N.NewAttachments+=TEXT(" · ");N.NewAttachments+=FrontlineLocalizationV35::Text(B->GetSupportName(EFrontlineSupport(I)));}
    }
    if(M.bArmourAllowedV41)if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& Entry:Vehicles->GetRoster(0))
    {
        const int32 Arrival=Vehicles->UnlockMission(Entry.Type,0);
        if(Arrival>=First&&Arrival<=Id){if(!N.NewAttachments.IsEmpty())N.NewAttachments+=TEXT(" · ");N.NewAttachments+=FrontlineLocalizationV35::Text(Entry.Name);}
    }
    if(N.NewAttachments.IsEmpty())N.NewAttachments=L(TEXT("No new attachments."),TEXT("本次无新增配属。"));
    return N;
}
TArray<FNode> Nodes(const AWarfareFrontlineGameMode* B)
{TArray<FNode> Out;if(B)for(int32 I=0;I<B->GetCampaignMissions().Num();++I)Out.Add(GetNode(B,I));return Out;}

FFrontline Frontline(const AWarfareFrontlineGameMode* B,int32 Id)
{
    FFrontline F;const auto* M=B?(B->GetCampaignMissions().IsValidIndex(Id)?&B->GetCampaignMissions()[Id]:B->GetSelectedCampaignMission()):nullptr;
    const int32 Date=M?M->CalendarDateV41:19170409;
    // Authored strategic sketches, not surveyed trench traces. Snapshot dates
    // prevent a later offensive's gains from leaking into an earlier preview.
    if(Date<19180321)
    {
        F.Date=19170401;
        // The red trace stays the same authored 1917 strategic sketch. This
        // caption names the selected operation's period, not a new survey.
        F.Label=Date<19170601?L(TEXT("SPRING 1917 · ARRAS FRONT"),TEXT("1917年春 · 阿拉斯战线"))
            :Date<19170701?L(TEXT("JUNE 1917 · MESSINES"),TEXT("1917年6月 · 梅西讷"))
            :Date<19171001?L(TEXT("SUMMER 1917 · THIRD YPRES"),TEXT("1917年夏 · 第三次伊普尔"))
            :Date<19180101?L(TEXT("NOVEMBER 1917 · CAMBRAI"),TEXT("1917年11月 · 康布雷"))
            :L(TEXT("WINTER 1918 · WESTERN FRONT"),TEXT("1918年冬 · 西线"));
        F.Points={{2.74,51.15},{2.86,50.89},{2.94,50.73},{2.89,50.49},{2.88,50.28},{3.05,50.10},{3.29,49.86},{3.34,49.63},{3.59,49.47},{4.30,49.21}};
    }
    else if(Date<19180409)
    {F.Date=19180321;F.Label=L(TEXT("21 MARCH 1918 · OPENING OF MICHAEL"),TEXT("1918年3月21日 · 米夏埃尔攻势开始"));F.Points={{2.74,51.15},{2.99,50.91},{2.94,50.73},{2.89,50.49},{2.91,50.28},{3.14,50.10},{3.28,49.86},{3.34,49.63},{3.59,49.47},{4.30,49.21}};}
    else if(Date<19180808)
    {F.Date=19180409;F.Label=L(TEXT("APRIL 1918 · THE SPRING CRISIS"),TEXT("1918年4月 · 春季危机"));F.Points={{2.74,51.15},{2.88,50.88},{2.64,50.73},{2.58,50.61},{2.78,50.43},{2.76,50.26},{2.54,50.02},{2.53,49.87},{2.69,49.66},{3.02,49.54},{3.59,49.47},{4.30,49.21}};}
    else if(Date<19180929)
    {F.Date=19180808;F.Label=L(TEXT("8 AUGUST 1918 · THE HUNDRED DAYS"),TEXT("1918年8月8日 · 百日攻势"));F.Points={{2.74,51.15},{2.91,50.87},{2.66,50.70},{2.65,50.57},{2.90,50.31},{2.67,50.09},{2.57,49.91},{2.71,49.67},{3.14,49.49},{3.55,49.33},{4.30,49.21}};}
    else if(Date<19181017)
    {F.Date=19180929;F.Label=L(TEXT("29 SEPTEMBER 1918 · THE HINDENBURG ASSAULT"),TEXT("1918年9月29日 · 突击兴登堡防线"));F.Points={{2.95,51.06},{3.08,50.90},{3.17,50.73},{3.18,50.47},{3.18,50.27},{3.26,50.04},{3.30,49.87},{3.48,49.64},{3.76,49.48},{4.45,49.25}};}
    else
    {F.Date=19181017;F.Label=L(TEXT("OCTOBER 1918 · THE FINAL ADVANCE"),TEXT("1918年10月 · 最后的推进"));F.Points={{3.21,51.28},{3.34,51.06},{3.30,50.85},{3.40,50.57},{3.51,50.36},{3.55,50.08},{3.55,49.86},{3.69,49.66},{4.10,49.48},{4.83,49.30}};}
    F.Points.Append({{5.10,49.25},{5.57,49.05},{5.79,48.88},{6.45,48.88},{6.94,48.57},{7.10,48.20},{7.15,47.80},{7.20,47.51}});
    return F;
}

FDispatch Dispatch(const AWarfareFrontlineGameMode* B,int32 Id)
{
    FDispatch D;if(!B)return D;
    const auto* R=B->GetCurrentCampaign();if(!R)return D;
    if(Id==INDEX_NONE)Id=R->SelectedMission;
    const auto E=FrontlineCampaignV32::EventForMission(R->Faction,Id);
    if(E.Id==INDEX_NONE&&(Id<0||Id>=8))return D;
    D.Id=Id;
    if(E.Id!=INDEX_NONE){D.Title=FrontlineLocalizationV35::Text(E.Title);D.Date=E.Date;D.Body=FrontlineLocalizationV35::Text(E.Body);}
    else
    {
        D.Title=L(TEXT("Temporary attachment"),TEXT("临时配属"));
        const auto& Missions=B->GetCampaignMissions();if(Missions.IsValidIndex(Id))D.Date=Missions[Id].DateLabel;
        D.Body=L(TEXT("A neighbouring formation lends a specialist section for this action. It returns afterwards."),TEXT("友邻借调一支专门分队参加本次行动，战后归建。"));
    }
    const uint32 Bit=1u<<Id,Resolved=R->StrategicRevisionV52==52?R->DispatchResolvedMaskV52:R->AcknowledgedEventMask;
    const int32 Order=DispatchOrder(*R,Id);
    D.bPending=Available(*R,Id)&&!(Wins(*R)&Bit)&&!(Resolved&Bit)&&Order==INDEX_NONE;
    if(Order!=INDEX_NONE)D.SelectedChoice=(R->DispatchChoiceMaskV52&(1u<<Order))?1:0;
    int32 Role=Id>=15?int32(EFrontlineRole::Pioneer):Id>=3?int32(EFrontlineRole::Lewis):int32(EFrontlineRole::Rifle);
    static constexpr EFrontlineRole EarlyLoans[]={EFrontlineRole::Lewis,EFrontlineRole::Pioneer,EFrontlineRole::Marksman,EFrontlineRole::Officer,
        EFrontlineRole::Pioneer,EFrontlineRole::Flamethrower,EFrontlineRole::Marksman,EFrontlineRole::Officer};
    if(Id>=0&&Id<UE_ARRAY_COUNT(EarlyLoans))Role=int32(EarlyLoans[Id]);
    if(Id==10||Id==11)Role=int32(EFrontlineRole::Mills);
    if(Id==19)Role=int32(EFrontlineRole::Rifle);
    const FString Name=FrontlineLocalizationV35::Text(B->GetRecruitName(EFrontlineRole(Role)));
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    const FString AttachmentEffect=Chinese?FString::Printf(TEXT("一支%s于开战 8 秒后抵达，不占用整备。\n若尚未正式配属，本关可按正常整备再征募一队。借调部队战后归建。"),*Name):FString::Printf(TEXT("One %s section arrives after 8 seconds without using readiness.\nIf not permanently assigned, one replacement can be recruited with normal readiness. The attachment returns after the action."),*Name);
    D.Choices.Add({L(TEXT("ACCEPT THE ATTACHED SECTION"),TEXT("接收配属小队")),AttachmentEffect,Role,0});
    const int32 Points=Id>=14?3:2;
    const FString ResearchEffect=Chinese?FString::Printf(TEXT("获得 %d 研究点。借调分队转交友邻。"),Points):FString::Printf(TEXT("Gain %d research points. The loaned section goes to a neighbouring formation."),Points);
    D.Choices.Add({L(TEXT("RETAIN THE TRAINING ALLOCATION"),TEXT("保留整训配额")),ResearchEffect,INDEX_NONE,Points});
    return D;
}
bool ResolveDispatch(AWarfareFrontlineGameMode* B,int32 Choice){return FFrontlineCampaignV52Access::Resolve(B,Choice);}
bool IsLoanRole(const AWarfareFrontlineGameMode* B,EFrontlineRole Role)
{
    if(!B||!FrontlineCampaignV41::RoleAllowed(B->GetPlayerFaction(),Role))return false;
    const auto* R=B->GetCurrentCampaign();const auto* M=FrontlineCampaignV32::CurrentOperation(B);
    if(!R||!M||R->StrategicRevisionV52!=52)return false;
    const int32 Order=DispatchOrder(*R,M->Id);if(Order==INDEX_NONE||(R->DispatchChoiceMaskV52&(1u<<Order)))return false;
    const auto D=Dispatch(B,M->Id);
    return D.Choices.Num()>0&&D.Choices[0].ReinforcementRole==int32(Role);
}
FString LoanRecruitBlockReason(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Team)
{
    if(Team!=0||!IsLoanRole(B,Role)||FrontlineUnlocksV53::Stage(B,0)>=B->RoleUnlockMission(Role,0))return FString();
    const auto* S=Runtime.Find(B);
    return S&&S->Encounter==B->GetEncounterId()&&S->LoanRecruits>=1?
        L(TEXT("The operation's single replacement attachment has been used."),TEXT("本次行动的一队借调补充已用完。")):FString();
}
void OnRecruit(AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Team)
{
    if(Team!=0||!IsLoanRole(B,Role)||FrontlineUnlocksV53::Stage(B,0)>=B->RoleUnlockMission(Role,0))return;
    auto& S=Runtime.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){S={};S.Encounter=B->GetEncounterId();}++S.LoanRecruits;
}
void Tick(AWarfareFrontlineGameMode* B){FFrontlineCampaignV52Access::Deliver(B);}

void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    bool Branches=true,Chronology=true,Roundtrip=true,Once=true;
    for(int32 Faction=0;Faction<2;++Faction)for(int32 Pick=0;Pick<16;++Pick)
    {
        FFrontlineCampaignProgress R;R.Faction=EFrontlineFaction(Faction);Initialize(R);int32 Previous=0;
        const auto& Route=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(R.Faction);
        while(R.CompletedMissions<20)
        {
            int32 Id=R.CompletedMissions;for(int32 I=0;I<4;++I)if(Id==Forks[I]&&(Pick&(1<<I)))++Id;
            Chronology&=Route[Id].CalendarDateV41>=Previous;Previous=Route[Id].CalendarDateV41;
            R.SelectedMission=Id;auto O=MakeShared<FJsonObject>();WriteArchive(*O,R);auto Loaded=R;Loaded.WonOperationMaskV52=0;
            FString Json;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&Json));TSharedPtr<FJsonObject> Parsed;
            Roundtrip&=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Parsed)&&Parsed.IsValid()&&ReadArchive(*Parsed,Loaded)&&Loaded.SelectedMission==Id&&Loaded.WonOperationMaskV52==R.WonOperationMaskV52;
            Branches&=RecordVictory(R,Id);const auto Before=R;Once&=!RecordVictory(R,Id)&&R.WonOperationMaskV52==Before.WonOperationMaskV52;
        }
        Branches&=R.CompletedMissions==20&&ActualVictories(R)==16&&FrontlineCampaignV32::BitCount(R.BypassedOperationMaskV52)==4&&RouteOperationCount(R)==16;
        for(int32 I=0;I<20;++I)if(R.BypassedOperationMaskV52&(1u<<I))Branches&=!Available(R,I);
    }
    Check(TEXT("v52_all_32_faction_branch_routes_reach_ending_with_16_actual_wins"),Branches);
    Check(TEXT("v52_every_selectable_route_preserves_historical_chronology"),Chronology);
    Check(TEXT("v52_archive_roundtrip_preserves_uncommitted_second_branch_selection"),Roundtrip);
    Check(TEXT("v52_duplicate_victory_does_not_repeat_route_progress"),Once);
    bool Legacy=true;for(int32 C=0;C<=20;++C)
    {
        FFrontlineCampaignProgress R;R.CompletedMissions=C;R.SelectedMission=FMath::Min(C,19);R.Research.ResearchPoints=17;R.Research.TechnologyMask=0x345;
        Initialize(R);Legacy&=ActualVictories(R)==C&&R.BypassedOperationMaskV52==0&&R.SelectedMission==FMath::Min(C,19)&&R.Research.ResearchPoints==17&&R.Research.TechnologyMask==0x345;
    }
    Check(TEXT("v52_legacy_prefix_wins_and_research_survive_without_invented_branch_choices"),Legacy);
    FFrontlineCampaignProgress R;Initialize(R);FJsonObject O;WriteArchive(O,R);O.SetNumberField(TEXT("bypassed_operations"),1);
    Check(TEXT("v52_invalid_bypass_archive_is_rejected"),!ReadArchive(O,R));
    bool Geo=true;if(B)for(const auto& N:Nodes(B))Geo&=N.Mission&&N.LonLat.X>=-2.5&&N.LonLat.X<=10&&N.LonLat.Y>=46.4&&N.LonLat.Y<=52;
    Check(TEXT("v52_twenty_real_operations_project_inside_only_playable_western_theatre"),B&&Nodes(B).Num()==20&&Geo&&Theatre().bPlayable);
    FFrontlineCampaignV52Access::Checks(B,Check);
}
}
