#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontline.h"
#include "Misc/DateTime.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

struct FFrontlineCampaignV32Access
{
    static void Check(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Report);
    static void CheckLoadoutV60(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Report);
    static void CheckSupportsV70(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Report);
    static bool Mutate(AWarfareFrontlineGameMode* B,TFunctionRef<void(FFrontlineCampaignProgress&)> Change)
    {
        if(!B || B->Phase==EFrontlinePhase::Fighting || B->bProgressionReadOnly)return false;
        auto* R=B->CampaignRuns.FindByPredicate([B](const FFrontlineCampaignProgress& V){return V.Id==B->ActiveCampaignId;});
        if(!R)return false;
        const FFrontlineCampaignProgress Before=*R;
        Change(*R);R->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
        if(B->SaveProgression())return true;
        *R=Before;B->LastMessage=B->ProgressionSaveStatus;return false;
    }
};

namespace FrontlineCampaignV32
{
int32 BitCount(uint32 Mask){int32 N=0;for(;Mask;Mask&=Mask-1)++N;return N;}
const FFrontlineCampaignMission* CurrentOperation(const AWarfareFrontlineGameMode* B)
{
    if(!B)return nullptr;
    if(B->Phase==EFrontlinePhase::Fighting || B->Phase==EFrontlinePhase::Victory || B->Phase==EFrontlinePhase::Defeat)
        if(const auto* M=B->GetEncounterCampaignMission())return M;
    return B->GetSelectedCampaignMission();
}
static uint32 FirstBits(uint32 Eligible,int32 Capacity)
{
    uint32 Result=0;
    for(int32 I=0;I<31 && Capacity>0;++I)if(Eligible&(1u<<I)){Result|=1u<<I;--Capacity;}
    return Result;
}
FLoadout GetLoadout(const AWarfareFrontlineGameMode* B)
{
    FLoadout L;if(!B)return L;
    if(WarfareFRRUCampaignV82::Active(B))
    {
        for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(B->IsRoleUnlocked(EFrontlineRole(I)))L.EligibleRoles|=1u<<I;
        for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)if(B->IsSupportUnlocked(EFrontlineSupport(I)))L.EligibleSupports|=1u<<I;
        L.bSelectRoles=L.EligibleRoles!=0;L.bSelectSupports=BitCount(L.EligibleSupports)>L.SupportCapacity;
        const uint32 Roles=WarfareFRRUCampaignV82::RoleSelection(B),Supports=WarfareFRRUCampaignV82::SupportSelection(B);
        if(Roles)L.SelectedRoles=FirstBits(Roles&L.EligibleRoles,L.RoleCapacity);
        else
        {
            // A default roster must carry the operation's real work party.
            // Explicit saved choices remain choices; deployment validates them.
            const uint32 Required=WarfareFRRUCampaignV82::RequiredRoleMask(B)&L.EligibleRoles;
            L.SelectedRoles=FirstBits(Required,L.RoleCapacity);
            L.SelectedRoles|=FirstBits(L.EligibleRoles&~L.SelectedRoles,L.RoleCapacity-BitCount(L.SelectedRoles));
        }
        L.SelectedSupports=FirstBits(Supports?Supports&L.EligibleSupports:L.EligibleSupports,L.SupportCapacity);
        if(!L.SelectedRoles)L.SelectedRoles=FirstBits(L.EligibleRoles,1);if(!L.SelectedSupports)L.SelectedSupports=FirstBits(L.EligibleSupports,1);return L;
    }
    const auto* R=B->GetCurrentCampaign();const auto* M=CurrentOperation(B);
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
        if(FrontlineGameplayV52::SelectableRole(EFrontlineRole(I)) && B->IsRoleUnlocked(EFrontlineRole(I))
            && (!M || !(M->DisabledRoleMask&(1u<<I)) || FrontlineUltimateArchiveV8215::FullArmory(B) || FrontlineCampaignV52::IsLoanRole(B,EFrontlineRole(I))))L.EligibleRoles|=1u<<I;
    for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)
        if(B->IsSupportUnlocked(EFrontlineSupport(I)) && (!M || !(M->DisabledSupportMask&(1u<<I)) || FrontlineUltimateArchiveV8215::FullArmory(B)))L.EligibleSupports|=1u<<I;
    L.bSelectRoles=R && L.EligibleRoles!=0;
    L.bSelectSupports=R && BitCount(L.EligibleSupports)>L.SupportCapacity;
    L.SelectedRoles=R?FirstBits(R->RoleSelectionMask?R->RoleSelectionMask&L.EligibleRoles:L.EligibleRoles,L.RoleCapacity):L.EligibleRoles;
    L.SelectedSupports=L.bSelectSupports?FirstBits(R->SupportSelectionMask?R->SupportSelectionMask&L.EligibleSupports:L.EligibleSupports,L.SupportCapacity):L.EligibleSupports;
    // Preserve every still-eligible saved choice, including rifle-free rosters.
    // An older/earlier operation may invalidate all choices; provide one legal
    // section then, without rewriting the archive or filling unused slots.
    if(!L.SelectedRoles&&L.EligibleRoles)L.SelectedRoles=FirstBits(L.EligibleRoles,1);
    // A temporary attachment expands eligibility, not mandatory roster slots.
    // Its dispatched relief squad still arrives through the event's own path.
    if(!L.SelectedSupports && L.EligibleSupports)L.SelectedSupports=FirstBits(L.EligibleSupports,L.SupportCapacity);
    return L;
}
TArray<EFrontlineSupport> GetEquippedSupports(const AWarfareFrontlineGameMode* B)
{
    TArray<EFrontlineSupport> Supports;const auto L=GetLoadout(B);
    if(B&&WarfareFRRUCampaignV82::Active(B))
    {for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)if((L.SelectedSupports&(1u<<I))&&Supports.Num()<L.SupportCapacity)Supports.Add(EFrontlineSupport(I));return Supports;}
    // Preserve the established console order, but never substitute the original
    // six-card catalogue when the campaign has fewer than six eligible tasks.
    // Shrapnel retains the legacy AirRecon ordinal for archive compatibility.
    static constexpr EFrontlineSupport Order[]={EFrontlineSupport::Artillery,EFrontlineSupport::Smoke,
        EFrontlineSupport::Mortar,EFrontlineSupport::Gas,EFrontlineSupport::AntiTank,EFrontlineSupport::AirStrike,
        EFrontlineSupport::CreepingBarrage,EFrontlineSupport::AirRecon,EFrontlineSupport::AirBombing};
    for(const auto Kind:Order)
        if((L.SelectedSupports&(1u<<uint8(Kind)))&&Supports.Num()<L.SupportCapacity)Supports.Add(Kind);
    return Supports;
}
FString RoleBlockReason(const AWarfareFrontlineGameMode* B,EFrontlineRole R,int32 Team)
{
    if(uint8(R)>=uint8(EFrontlineRole::Count))return TEXT("此分队不存在。");
    if(B&&WarfareFRRUCampaignV82::Active(B))return Team==0&&!(GetLoadout(B).SelectedRoles&(1u<<uint8(R)))?(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?TEXT("本次出发未编入此分队；请回作战室调整配属。"):TEXT("This section is not assigned. Adjust the roster in the war room.")):FString();
    if(R==EFrontlineRole::Signals)return TEXT("观察联络已编入指挥分队。 ");
    if(!B || Team!=0)return FString();
    R=FrontlineGameplayV52::CanonicalRole(R);
    const auto* M=CurrentOperation(B);
    if(M && (M->DisabledRoleMask&(1u<<uint8(R)))&&!FrontlineUltimateArchiveV8215::FullArmory(B,Team)&&!FrontlineCampaignV52::IsLoanRole(B,R))return M->RestrictionText;
    const auto L=GetLoadout(B);
    return L.bSelectRoles && !(L.SelectedRoles&(1u<<uint8(R)))?TEXT("本次出发未编入此分队；回到作战室可调整配属。"):FString();
}
FString SupportBlockReason(const AWarfareFrontlineGameMode* B,EFrontlineSupport S,int32 Team)
{
    if(uint8(S)>=uint8(EFrontlineSupport::Count))return TEXT("此支援不存在。");
    if(!B || (Team!=0&&Team!=1))return FString();
    if(WarfareFRRUCampaignV82::Active(B))return Team==0&&!(GetLoadout(B).SelectedSupports&(1u<<uint8(S)))?(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?TEXT("本次行动未接通此支援；请回作战室调整联络。"):TEXT("This support is not assigned. Adjust support in the war room.")):FString();
    const auto* M=CurrentOperation(B);
    if(M && (M->DisabledSupportMask&(1u<<uint8(S)))&&!FrontlineUltimateArchiveV8215::FullArmory(B,Team))return M->RestrictionText;
    if(Team!=0)return FString();
    const auto L=GetLoadout(B);
    return L.bSelectSupports && !(L.SelectedSupports&(1u<<uint8(S)))?TEXT("本次出发未接通这项支援；回到作战室可调整联络。"):FString();
}
bool RoleSelected(const AWarfareFrontlineGameMode* B,EFrontlineRole R,int32 Team)
{return uint8(R)<uint8(EFrontlineRole::Count) && (Team!=0 || (B && (GetLoadout(B).SelectedRoles&(1u<<uint8(WarfareFRRUCampaignV82::Active(B)?R:FrontlineGameplayV52::CanonicalRole(R))))));}
bool SupportSelected(const AWarfareFrontlineGameMode* B,EFrontlineSupport S,int32 Team)
{return uint8(S)<uint8(EFrontlineSupport::Count) && (Team!=0 || (B && (GetLoadout(B).SelectedSupports&(1u<<uint8(S)))));}
bool ToggleRole(AWarfareFrontlineGameMode* B,EFrontlineRole Role)
{
    if(!B || (!WarfareFRRUCampaignV82::Active(B)&&!FrontlineGameplayV52::SelectableRole(Role))||uint8(Role)>=uint8(EFrontlineRole::Count))return false;
    const auto L=GetLoadout(B);const uint32 Bit=1u<<uint8(Role);
    if(!L.bSelectRoles || !(L.EligibleRoles&Bit))return false;
    const uint32 Next=L.SelectedRoles^Bit;
    if(!Next){B->LastMessage=TEXT("至少配属一支本次行动可用的步兵。");return false;}
    if(BitCount(Next)>L.RoleCapacity){B->LastMessage=TEXT("先留下一支分队，才能安排新的配属。");return false;}
    if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::SetSelection(B,Next,L.SelectedSupports);
    return FFrontlineCampaignV32Access::Mutate(B,[Next](FFrontlineCampaignProgress& R){R.RoleSelectionMask=Next;});
}
bool SetMobileGunV52(AWarfareFrontlineGameMode* B,bool Enabled)
{
    if(!B||B->Phase!=EFrontlinePhase::Briefing||!FrontlineGameplayV52::MobileAvailable(B))return false;
    // Changing the next deployment never rewrites living soldiers or readiness clocks.
    return FFrontlineCampaignV32Access::Mutate(B,[Enabled](FFrontlineCampaignProgress& R){R.bMobileGunV52=Enabled;});
}
bool ToggleSupport(AWarfareFrontlineGameMode* B,EFrontlineSupport Support)
{
    if(!B || uint8(Support)>=uint8(EFrontlineSupport::Count))return false;
    const auto L=GetLoadout(B);const uint32 Bit=1u<<uint8(Support);
    if(!L.bSelectSupports || !(L.EligibleSupports&Bit))return false;
    const uint32 Next=L.SelectedSupports^Bit;
    if(!Next){B->LastMessage=TEXT("至少保持一条后方联络。");return false;}
    if(BitCount(Next)>L.SupportCapacity){B->LastMessage=TEXT("先解除一条支援联络，再接通新的炮位或机队。");return false;}
    if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::SetSelection(B,L.SelectedRoles,Next);
    return FFrontlineCampaignV32Access::Mutate(B,[Next](FFrontlineCampaignProgress& R){R.SupportSelectionMask=Next;});
}

#include "WarfareFrontlineDispatchDataV50.inl"
FHistoricalEvent EventForMission(EFrontlineFaction Faction,int32 Id)
{return DispatchV50(Faction,Id);}
FDispatchConditionsV50 DispatchConditionsV50(int32 MissionId,uint32 AcknowledgedEvents)
{
    struct FOrder {int32 First,Last;FDispatchConditionsV50 Conditions;};
    static const FOrder Orders[]={
        {3,4,{22.f,0.f,8.f,0.f}},
        {5,6,{-24.f,18.f,0.f,0.f}},
        {8,9,{28.f,5.f,0.f,0.f}},
        {11,12,{20.f,0.f,12.f,6.f}},
        {14,15,{22.f,12.f,0.f,4.f}},
        {16,17,{12.f,8.f,12.f,0.f}},
        {18,19,{12.f,0.f,12.f,4.f}}
    };
    for(const auto& Order:Orders)
        if(MissionId>=Order.First&&MissionId<=Order.Last&&(AcknowledgedEvents&(1u<<Order.First)))return Order.Conditions;
    return {};
}
FHistoricalEvent PendingEvent(const AWarfareFrontlineGameMode* B)
{
    if(!B || B->Phase==EFrontlinePhase::Fighting)return {};
    const auto* R=B->GetCurrentCampaign();const auto* M=B->GetSelectedCampaignMission();
    if(!R || !M || M->Id<0 || M->Id>=31 || M->Id>R->CompletedMissions || (R->AcknowledgedEventMask&(1u<<M->Id)))return {};
    return EventForMission(R->Faction,M->Id);
}
bool AcknowledgeEvent(AWarfareFrontlineGameMode* B)
{
    const auto E=PendingEvent(B);if(E.Id==INDEX_NONE)return false;
    return FFrontlineCampaignV32Access::Mutate(B,[E](FFrontlineCampaignProgress& R)
    {R.AcknowledgedEventMask|=1u<<E.Id;R.Research.ResearchPoints=FMath::Min(99999,R.Research.ResearchPoints+E.ResearchAward);});
}
FString NewTechnologyBlockReason(const AWarfareFrontlineGameMode*,int32)
{
    // Doctrine follows earned research and prerequisites, not arbitrary chapter
    // locks. Actual armament/attachment dates remain in their deployment gates.
    return FString();
}

void AppendTechnologyNodes(TArray<FFrontlineTechnologyNode>& Nodes,bool G)
{
    Nodes.Append({
        {16,10,0,4,4,G?TEXT("滤罐气密检查"):TEXT("小盒式呼吸器"),G?TEXT("为投弹班、工兵与喷火分遣队配发呼吸器。完成佩戴后，毒气累积减少 78%；仍受压制，久留仍可能致命。其余分队需避开毒区。"):TEXT("为投弹班、工兵与喷火分遣队配发呼吸器。完成佩戴后，毒气累积减少 78%；仍受压制，久留仍可能致命。其余分队需避开毒区。"),-1,G?TEXT("German_16"):TEXT("British_16"),TEXT("前沿装备")},
        {17,5,1,4,2,G?TEXT("前沿清障操练"):TEXT("工兵开路"),TEXT("工兵排除爆炸物的速度提高 65%。"),-1,G?TEXT("German_17"):TEXT("British_17"),TEXT("工兵通路")},
        {18,4,2,4,3,G?TEXT("喷火组协同"):TEXT("Schilt 携行燃料"),G?TEXT("喷火射程略增；首次燃料耗尽后，可再喷射四次。"):TEXT("喷火射程略增；首次燃料耗尽后，可再喷射四次。"),17,G?TEXT("German_18"):TEXT("British_18"),TEXT("近壕清障")},
        {19,3,3,4,3,G?TEXT("徐进火墙"):TEXT("步炮协同"),TEXT("徐进弹幕各组推进间距缩短，覆盖更密集；炮弹总数不变。"),10,G?TEXT("German_19"):TEXT("British_19"),TEXT("诸兵种联络")},
        {20,3,0,5,2,G?TEXT("引信装定操练"):TEXT("榴霰弹引信"),TEXT("榴霰弹由两轮增至三轮，每轮仍为 3 发。"),-1,G?TEXT("German_20"):TEXT("British_20"),TEXT("前沿装备")},
        {21,3,1,5,3,G?TEXT("红星求援"):TEXT("SOS 预备火线"),TEXT("己方士气低于 35 时，徐进弹幕、榴霰弹与轰炸任务整备缩短 22%。"),10,G?TEXT("German_21"):TEXT("British_21"),TEXT("工兵通路")},
        {22,19,2,5,3,G?TEXT("铁路炮兵联络"):TEXT("铁路反装甲观察"),G?TEXT("装甲列车车体耐久提高 22%；铁路行动中的反坦克野炮整备缩短 18%。"):TEXT("铁路行动中，反坦克野炮整备时间缩短 18%。"),20,G?TEXT("German_22"):TEXT("British_22"),TEXT("近壕清障")},
        {23,5,3,5,3,G?TEXT("老兵接替"):TEXT("老兵带队"),TEXT("所有步兵整备再缩短 8%，可与补给训练叠加。"),1,G?TEXT("German_23"):TEXT("British_23"),TEXT("诸兵种联络")}
    });
}

void ApplyMissionExtensions(TArray<FFrontlineCampaignMission>& Missions,bool G)
{
    for(auto& M:Missions)
    {
        M.BattlefieldHalfWidth=M.Id==2?1180.f:1040.f;
        M.TerrainStyleV32=M.Id==1?EFrontlineMissionTerrainV32::Ridge:M.Id==2?EFrontlineMissionTerrainV32::Marsh:EFrontlineMissionTerrainV32::Farmland;
        M.WireDensity=M.Id==2?.65f:M.Id==3?.76f:.40f;
        M.MineDensity=M.Id==2?.18f:M.Id==3?.27f:0.f;
        M.DisabledRoleMask=(1u<<uint8(EFrontlineRole::Flamethrower))|(1u<<uint8(EFrontlineRole::Pioneer));
        M.RestrictionText=TEXT("此行动的命令尚未配属喷火与工兵分队。");
    }
    auto Add=[&](int32 Id,const TCHAR* Title,const TCHAR* Date,const TCHAR* Place,const TCHAR* Map,FVector2D Position,
        const TCHAR* History,const TCHAR* Orders,const TCHAR* Terrain,EFrontlineMissionTerrainV32 Style,float Wet,float Rain,float Sight)
    {
        FFrontlineCampaignMission M;M.Id=Id;M.Title=Title;M.DateLabel=Date;M.Location=Place;M.MapKey=Map;M.MapPosition=Position;M.Year=1918;
        M.HistoricalContext=History;M.Briefing=Orders;M.TerrainNotes=Terrain;M.TerrainStyleV32=Style;
        M.BattlefieldHalfWidth=Id==8?1300.f:1240.f;M.Wetness=Wet;M.RainBase=Rain;M.RainVariation=.10f;M.VisibilityScale=Sight;M.MudMoveScale=Wet>.7f?.90f:.97f;
        M.PlayerMorale=100;M.EnemyMorale=G?110:112;M.EnemyRecruitScale=.94f;M.EnemyAssaultDelay=8.5f;
        M.PlayerArtilleryDelay=65;M.EnemyArtilleryDelay=88;M.PlayerSmokeDelay=0;M.PlayerMortarDelay=28;M.RandomSeed=32000+(G?100:0)+Id*37;
        M.WireDensity=Id==7?.95f:Id==9?.38f:.70f;M.MineDensity=Id==7?.66f:Id==8?.54f:.38f;
        M.Objective=EFrontlineCampaignObjective::Breakthrough;M.HoldCenterSeconds=0.f;
        M.ObjectiveText=TEXT("突破敌方后沿，或击溃守军士气");
        M.EnemyRoleOrder={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Pioneer,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Flamethrower};
        M.EnemyOpeningRoles={EFrontlineRole::Lewis};M.bEnemyCommissionedOfficers=true;M.bEnemyScopedMarksmen=G;M.bEnemyStormtroopers=!G;
        Missions.Add(M);
    };
    Add(6,G?TEXT("斯卡尔普：铁丝后的后卫"):TEXT("越过白垩田野"),TEXT("26 AUGUST 1918"),TEXT("法国 · 斯卡尔普河战区"),TEXT("scarpe1918"),FVector2D(2.99,50.29),
        TEXT("亚眠之后，协约军继续扩大攻势。斯卡尔普河方向的推进越过旧阵地与白垩农田；德国后卫利用尚存的铁丝、村落与交通壕争取时间。"),
        G?TEXT("撤退道路重新穿过旧阵地，铁丝挡住了己方接近线。工兵开路，Wex 分队与机枪掩护局部反击。"):
        TEXT("斯卡尔普的旧铁丝从白垩土中露出。工兵检查未爆弹，步兵随徐进弹幕穿过缺口。"),
        TEXT("白垩土的壕肩呈浅灰断面，铁丝沿田界分布；可疑地段有通路，分散前进比挤在一起更安全。"),EFrontlineMissionTerrainV32::Farmland,.46f,.08f,.98f);
    Add(7,G?TEXT("运河下面仍有人"):TEXT("贝利库尔的地表"),TEXT("29 SEPTEMBER 1918"),TEXT("法国 · 圣康坦运河"),TEXT("hindenburg"),FVector2D(3.236,49.955),
        TEXT("圣康坦运河构成兴登堡防线的重要障碍。澳大利亚与美军在贝利库尔隧道上方作战，英国部队也在其他渡口推进；纵深防御让突破远非一条直线。"),
        G?TEXT("运河纤道仍能通行，两侧枪位保持联络。工兵照看通路，预备队准备夺回失守的中央阵位。"):
        TEXT("里克瓦尔的运河明渠横在前方。工兵沿破损坡岸接近，法军喷火分遣队清理近处掩体；烟幕掩护渡越。"),
        TEXT("深切运河与草坡、之字壕和密集铁丝交错。爆破障碍与弹坑挤压通路，烟幕应覆盖整段渡口。"),EFrontlineMissionTerrainV32::Canal,.64f,.13f,.89f);
    Add(8,G?TEXT("塞勒河：留住铁轨"):TEXT("切口里的钢铁"),TEXT("17 OCTOBER 1918"),TEXT("法国 · 塞勒河谷"),TEXT("selle"),FVector2D(3.54,50.105),
        TEXT("塞勒河战役中，英军与相邻法军继续向东推进，德军依靠河谷、铁路与村落组织后卫。后卫依靠沿河铁路输送弹药、撤离装备，列车火力正在掩护守军保住最后的通路。"),
        G?TEXT("塞勒河车站仍在装车，列车炮组留在切口。步兵掩护车载枪炮的侧翼，守住通往河岸的道路。"):
        TEXT("塞勒河切口里的列车仍在掩护撤运。工兵清出接近线，反装甲火力打击车载枪炮，步兵沿路堤展开。"),
        TEXT("铁路越过湿润河谷，路堤提供局部遮蔽。轨道旁散布障碍与废弃物，纵向展开的通路更宽。"),EFrontlineMissionTerrainV32::RailCutting,.76f,.25f,.93f);
    auto& Rail=Missions.Last();Rail.bArmoredTrain=true;Rail.TrainTeam=G?0:1;
    Add(9,G?TEXT("桑布尔：最后的后卫"):TEXT("停战以前的清晨"),TEXT("4 NOVEMBER 1918"),TEXT("法国 · 桑布尔与勒凯努瓦战区"),TEXT("sambre"),FVector2D(3.64,50.10),
        TEXT("桑布尔方向的攻势展开，新西兰部队在勒凯努瓦作战。奥匈帝国的停战已经生效，西线尚未停火；后卫阵地、运河与仍有居民的村镇继续阻挡前进。"),
        G?TEXT("南方停战的电报已到，西线仍在开火。机枪守住桑布尔渡口，预备队掩护最后的通路。城郊禁止毒气与空中轰炸。"):
        TEXT("十一月四日，新西兰部队逼近勒凯努瓦城墙。桑布尔渡口仍在交火；肃清后沿，掩护后续部队通过。城郊禁止毒气与空中轰炸。"),
        TEXT("破墙、果树与草沟分割射界，斜向交通壕通往运河。零散铁丝与可疑障碍仍能夺走生命。"),EFrontlineMissionTerrainV32::Village,.68f,.18f,.95f);
    auto& Last=Missions.Last();Last.DisabledSupportMask=(1u<<uint8(EFrontlineSupport::Gas))|(1u<<uint8(EFrontlineSupport::AirBombing));
    Last.RestrictionText=TEXT("本次命令不批准居民区邻段的毒气与空中轰炸。");
    Missions[6].AlliedContext=G?TEXT("工兵与 Wex 分队归建。"):TEXT("工兵配属，新的支援联络需要出发前安排。");
    if(!G){Missions[6].DisabledRoleMask=1u<<uint8(EFrontlineRole::Flamethrower);Missions[6].RestrictionText=TEXT("法军 Schilt 分遣队将在圣康坦运河行动加入。");}
    else Missions[6].EnemyRoleOrder.Remove(EFrontlineRole::Flamethrower);
    Missions[7].AlliedContext=G?TEXT("纵深守军仍与邻段保持联络。"):TEXT("邻段澳美军团作战；本连可接纳法军 Schilt 分遣队。");
    Missions[8].AlliedContext=G?TEXT("铁路后卫正在掩护撤运，装甲列车支援本段。"):TEXT("澳大利亚军团休整，英法军队继续推进。");
    Missions[9].AlliedContext=G?TEXT("奥匈帝国停战的消息已经传来。"):TEXT("新西兰部队正在勒凯努瓦方向作战。");
}

void RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCampaignV32Access::Check(B,Check);}
void RunLoadoutChecksV60(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCampaignV32Access::CheckLoadoutV60(B,Check);}
void RunSupportChecksV70(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCampaignV32Access::CheckSupportsV70(B,Check);}
}

#include "WarfareFrontlineSupportRosterReviewV70.inl"

void FFrontlineCampaignV32Access::CheckLoadoutV60(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Report)
{
    using namespace FrontlineCampaignV32;
    const bool Isolated=B&&B->bProgressionVolatile;
    Report(TEXT("v60_loadout_fixture_is_volatile"),Isolated);if(!Isolated)return;
    const auto OldRuns=B->CampaignRuns;const auto OldActive=B->ActiveCampaignId;const auto OldFaction=B->PlayerFaction;
    const auto OldPhase=B->Phase;const auto OldMessage=B->LastMessage;const auto OldStatus=B->ProgressionSaveStatus;const bool OldReadOnly=B->bProgressionReadOnly;
    ON_SCOPE_EXIT{B->CampaignRuns=OldRuns;B->ActiveCampaignId=OldActive;B->PlayerFaction=OldFaction;B->Phase=OldPhase;B->LastMessage=OldMessage;B->ProgressionSaveStatus=OldStatus;B->bProgressionReadOnly=OldReadOnly;};
    B->Phase=EFrontlinePhase::Briefing;B->bProgressionReadOnly=false;
    const uint32 Gun=1u<<uint8(EFrontlineRole::Lewis),Bombers=1u<<uint8(EFrontlineRole::Mills);
    for(EFrontlineFaction Faction:{EFrontlineFaction::British,EFrontlineFaction::German})
    {
        FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=Faction;R.CompletedMissions=20;R.SelectedMission=1;R.RoleSelectionMask=Gun|Bombers;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=Faction;
        const auto L=GetLoadout(B);
        Report(TEXT("v60_early_nonempty_saved_roster_keeps_rifles_deselected"),L.bSelectRoles&&L.SelectedRoles==(Gun|Bombers)&&B->CampaignRuns[0].RoleSelectionMask==(Gun|Bombers)
            &&!RoleBlockReason(B,EFrontlineRole::Rifle).IsEmpty()&&RoleBlockReason(B,EFrontlineRole::Lewis).IsEmpty());
        const bool Removed=ToggleRole(B,EFrontlineRole::Mills);
        const bool LastRefused=!ToggleRole(B,EFrontlineRole::Lewis);
        Report(TEXT("v60_only_last_available_infantry_choice_is_mandatory"),Removed&&LastRefused&&GetLoadout(B).SelectedRoles==Gun&&B->CampaignRuns[0].RoleSelectionMask==Gun);
        const bool Added=ToggleRole(B,EFrontlineRole::Rifle),RemovedRifle=ToggleRole(B,EFrontlineRole::Rifle);
        Report(TEXT("v60_rifle_card_uses_ordinary_reversible_selection"),Added&&RemovedRifle&&GetLoadout(B).SelectedRoles==Gun);
        B->CampaignRuns[0].SelectedMission=0;B->CampaignRuns[0].CompletedMissions=0;
        const auto Early=GetLoadout(B);
        Report(TEXT("v60_ineligible_replay_falls_back_without_overwriting_saved_choice"),Early.SelectedRoles!=0&&(Early.SelectedRoles&~Early.EligibleRoles)==0&&B->CampaignRuns[0].RoleSelectionMask==Gun);
        B->CampaignRuns[0].SelectedMission=7;B->CampaignRuns[0].CompletedMissions=20;
        Report(TEXT("v60_returning_to_later_operation_restores_saved_rifle_free_choice"),GetLoadout(B).SelectedRoles==Gun);
        B->Phase=EFrontlinePhase::Fighting;
        Report(TEXT("v60_battle_cannot_rewrite_departure_roster"),!ToggleRole(B,EFrontlineRole::Rifle)&&B->CampaignRuns[0].RoleSelectionMask==Gun);
        B->Phase=EFrontlinePhase::Briefing;
        auto& Late=B->CampaignRuns[0];Late.SelectedMission=19;Late.CompletedMissions=20;
        FrontlineCampaignV52::Initialize(Late);Late.DispatchOrderMaskV52|=1u<<19;Late.DispatchResolvedMaskV52|=1u<<19;Late.DispatchChoiceMaskV52&=~(1u<<19);
        Report(TEXT("v60_temporary_rifle_attachment_does_not_force_roster_slot"),FrontlineCampaignV52::IsLoanRole(B,EFrontlineRole::Rifle)
            &&GetLoadout(B).SelectedRoles==Gun&&!RoleBlockReason(B,EFrontlineRole::Rifle).IsEmpty());
    }
}

void FFrontlineCampaignV32Access::Check(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Report)
{
    using namespace FrontlineCampaignV32;
    const bool Isolated=B && B->bProgressionVolatile;
    Report(TEXT("campaign_review_never_writes_player_profile"),Isolated);if(!Isolated)return;
    const auto OldRuns=B->CampaignRuns;const auto OldActive=B->ActiveCampaignId;const auto OldFaction=B->PlayerFaction;
    const auto OldPhase=B->Phase;const auto OldMessage=B->LastMessage;const auto OldStatus=B->ProgressionSaveStatus;
    const bool OldReadOnly=B->bProgressionReadOnly;B->bProgressionReadOnly=false;B->Phase=EFrontlinePhase::Briefing;
    bool Stable=true,Chronological=true,Graph=true;
    for(int32 F=0;F<2;++F)
    {
        const auto& M=B->GetCampaignMissionsForFaction(EFrontlineFaction(F));
        Stable&=M.Num()==10;
        static const TCHAR* Keys[]={TEXT("arras"),TEXT("messines"),TEXT("ypres"),TEXT("cambrai"),TEXT("michael"),TEXT("amiens")};
        for(int32 I=0;I<M.Num();++I){Stable&=M[I].Id==I;if(I<6)Stable&=FString(M[I].MapKey)==Keys[I];else Chronological&=M[I].Year==1918;}
        const auto& N=B->GetTechnologyNodesForFaction(EFrontlineFaction(F));Graph&=N.Num()==24;
        // A full DFS catches cycles even when paid IDs are not tier ordered.
        TArray<int32> Marks;Marks.Init(0,N.Num());TFunction<bool(int32)> Visit;
        Visit=[&](int32 Id){if(!N.IsValidIndex(Id))return false;if(Marks[Id]==1)return false;if(Marks[Id]==2)return true;Marks[Id]=1;
            for(int32 P:{N[Id].Prerequisite,N[Id].SecondaryPrerequisite})if(P!=INDEX_NONE && !Visit(P))return false;Marks[Id]=2;return true;};
        for(int32 I=0;I<N.Num();++I)Graph&=N[I].Id==I && Visit(I) && N[I].Cost>0;
    }
    Report(TEXT("ten_operations_preserve_original_six_save_indexes"),Stable);
    Report(TEXT("appended_operations_remain_in_late_1918"),Chronological);
    Report(TEXT("both_24_node_research_graphs_are_complete_and_acyclic"),Graph);
    FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=EFrontlineFaction::British;R.CompletedMissions=7;R.SelectedMission=7;
    R.CreatedUtc=R.UpdatedUtc=FDateTime::UtcNow().ToIso8601();B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=R.Faction;
    auto L=GetLoadout(B);
    Report(TEXT("overflow_loadout_keeps_five_roles_six_supports_and_rifle"),L.bSelectRoles && L.bSelectSupports && BitCount(L.SelectedRoles)==5 && BitCount(L.SelectedSupports)==6 && (L.SelectedRoles&1u));
    const uint32 Before=L.SelectedRoles;const bool Refused=!ToggleRole(B,EFrontlineRole::Pioneer);
    Report(TEXT("full_roster_rejects_extra_role_without_mutation"),Refused && GetLoadout(B).SelectedRoles==Before);
    const bool Exchanged=ToggleRole(B,EFrontlineRole::Marksman) && ToggleRole(B,EFrontlineRole::Pioneer);
    Report(TEXT("roster_exchange_reaches_actual_recruitment_gate"),Exchanged && RoleBlockReason(B,EFrontlineRole::Pioneer).IsEmpty() && !RoleBlockReason(B,EFrontlineRole::Marksman).IsEmpty()
        &&ToggleRole(B,EFrontlineRole::Rifle)&&!RoleSelected(B,EFrontlineRole::Rifle));
    const int32 Points=B->GetResearchPoints();const auto E=PendingEvent(B);const bool Ack=AcknowledgeEvent(B);
    Report(TEXT("historical_event_rewards_once_per_campaign_archive"),E.Id==7 && Ack && !AcknowledgeEvent(B) && B->GetResearchPoints()==Points+2 && PendingEvent(B).Id==INDEX_NONE);
    R.Id=FGuid::NewGuid();R.AcknowledgedEventMask=0;B->CampaignRuns.Add(R);B->ActiveCampaignId=R.Id;
    Report(TEXT("new_archive_does_not_inherit_event_receipts"),PendingEvent(B).Id==7 && B->GetResearchPoints()==2);
    auto& Active=B->CampaignRuns.Last();Active.CompletedMissions=9;Active.SelectedMission=9;Active.SupportSelectionMask=(1u<<int32(EFrontlineSupport::Count))-1;
    L=GetLoadout(B);
    const uint32 Forbidden=(1u<<uint8(EFrontlineSupport::Gas))|(1u<<uint8(EFrontlineSupport::AirBombing));
    Report(TEXT("civilian_sector_restrictions_override_saved_support_choices"),!(L.SelectedSupports&Forbidden) && !SupportBlockReason(B,EFrontlineSupport::Gas).IsEmpty());
    Active.SelectedMission=0;
    Report(TEXT("early_replay_does_not_deploy_late_specialists"),!RoleSelected(B,EFrontlineRole::Pioneer) && !RoleSelected(B,EFrontlineRole::Flamethrower));
    Report(TEXT("respirator_research_has_no_arbitrary_chapter_gate"),NewTechnologyBlockReason(B,16).IsEmpty());
    Active.SelectedMission=8;
    const auto* Train=B->GetSelectedCampaignMission();
    Report(TEXT("armored_train_is_authored_rail_sector_only"),Train && Train->bArmoredTrain && Train->TerrainStyleV32==EFrontlineMissionTerrainV32::RailCutting && Train->TrainTeam==1);
    B->CampaignRuns=OldRuns;B->ActiveCampaignId=OldActive;B->PlayerFaction=OldFaction;B->Phase=OldPhase;
    B->LastMessage=OldMessage;B->ProgressionSaveStatus=OldStatus;B->bProgressionReadOnly=OldReadOnly;
}
