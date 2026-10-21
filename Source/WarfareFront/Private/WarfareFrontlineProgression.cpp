#include "WarfareFrontlineProgression.h"
#include "WarfareFrontlineEnemyPolicyV84415.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineNewUnlockV8215.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "FRRU/WarfareFRRUCampaignInternalV82.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Containers/StringConv.h"

namespace
{
constexpr int64 ProgressionMaxBytesV86=64ll*1024*1024;
bool ProgressionBytesV86(int64 Bytes){return Bytes>=0&&Bytes<=ProgressionMaxBytesV86;}
int64 ProgressionUTF8BytesV86(const FString& Json){FTCHARToUTF8 Encoded(*Json);return Encoded.Length();}
struct FRecordedOutcomeV8217{FGuid Encounter,Campaign;FFrontlineCombatRecordV65 Captured;bool Persisted=false;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FRecordedOutcomeV8217> RecordedOutcomesV8217;
FFrontlineCombatRecordV65 CombatTailV8217(const FFrontlineCombatRecordV65& Now,const FFrontlineCombatRecordV65& Before)
{
 FFrontlineCombatRecordV65 Delta;auto Tally=[](const FFrontlineKillTallyV65& N,const FFrontlineKillTallyV65& P){FFrontlineKillTallyV65 D;D.Infantry=FMath::Max(0,N.Infantry-P.Infantry);D.Armour=FMath::Max(0,N.Armour-P.Armour);return D;};
 Delta.Total=Tally(Now.Total,Before.Total);Delta.OtherSources=Tally(Now.OtherSources,Before.OtherSources);
 for(int32 I=0;I<UE_ARRAY_COUNT(Delta.InfantrySources);++I)Delta.InfantrySources[I]=Tally(Now.InfantrySources[I],Before.InfantrySources[I]);
 for(int32 I=0;I<UE_ARRAY_COUNT(Delta.ArmourSources);++I)Delta.ArmourSources[I]=Tally(Now.ArmourSources[I],Before.ArmourSources[I]);
 for(int32 I=0;I<UE_ARRAY_COUNT(Delta.SupportSources);++I)Delta.SupportSources[I]=Tally(Now.SupportSources[I],Before.SupportSources[I]);
 for(const auto& Item:Now.NationalArmourSourcesV82)Delta.NationalArmourSourcesV82.Add(Item.Key,Tally(Item.Value,Before.NationalArmourSourcesV82.FindRef(Item.Key)));
 Delta.InfantryLost=FMath::Max(0,Now.InfantryLost-Before.InfantryLost);Delta.ArmourLost=FMath::Max(0,Now.ArmourLost-Before.ArmourLost);Delta.DetailsV839=FrontlineReportV839::Tail(Now.DetailsV839,Before.DetailsV839);return Delta;
}
constexpr uint32 LegacyTechnologyMask=(1u<<6)-1;
constexpr uint32 KnownTechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;
bool ValidFaction(EFrontlineFaction Faction) { return uint8(Faction)<=uint8(EFrontlineFaction::German); }
bool ValidTechnology(int32 Id) { return Id>=0 && Id<int32(EFrontlineTechnology::Count); }
const TCHAR* ArmyKey(int32 Index) { return Index==0?TEXT("British"):TEXT("German"); }
bool ReadBoundedInteger(const FJsonObject& Object,const TCHAR* Key,int32 Minimum,int32 Maximum,int32& Value)
{
    double Number=0;
    if (!Object.TryGetNumberField(Key,Number) || !FMath::IsFinite(Number) || Number<Minimum || Number>Maximum || Number!=FMath::FloorToDouble(Number)) return false;
    Value=int32(Number); return true;
}
}

EFrontlineFaction AWarfareFrontlineSoldier::GetFaction() const
{ return Battle?Battle->GetFactionForTeam(Team):(Team==0?EFrontlineFaction::British:EFrontlineFaction::German); }

int32 AWarfareFrontlineSoldier::GetVisualRoleIndex() const
{
    // Legacy weapon-family index remains stable. Scoped/pistol identities select
    // their actual mesh and motion set separately; escorts retain service rifles.
    const int32 Visual=IsMachineGunner()?1:(Role==EFrontlineRole::Mills || IsSectionLeader()?2:0);
    return int32(GetFaction())*3+Visual;
}

EFrontlineFaction AWarfareFrontlineGameMode::GetFactionForTeam(int32 Team) const
{ return Team==1?(PlayerFaction==EFrontlineFaction::British?EFrontlineFaction::German:EFrontlineFaction::British):PlayerFaction; }

const TCHAR* AWarfareFrontlineGameMode::GetFactionName(int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))switch(WarfareFRRUCampaignV82::Nation(this,Team))
    {
        case EFRRUNationV81::French:return TEXT("FRENCH ARMY");
        case EFRRUNationV81::Russian:return TEXT("RUSSIAN ARMY");
        case EFRRUNationV81::AustroHungarian:return TEXT("AUSTRO-HUNGARIAN ARMY");
        case EFRRUNationV81::German:return TEXT("GERMAN ARMY");
        default:return TEXT("UNAVAILABLE");
    }
    return GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("英军"):TEXT("德军");
}

const TCHAR* AWarfareFrontlineGameMode::GetRecruitName(EFrontlineRole InRole,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::RoleName(this,InRole,Team);
    if(InRole==EFrontlineRole::MobileGun)return GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("Lewis 机动枪组"):TEXT("MG 08/15 伴随枪组");
    if(InRole==EFrontlineRole::Signals)return GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("炮兵前沿观测组"):TEXT("Artilleriebeobachter 观测组");
    if(InRole==EFrontlineRole::RifleGrenadier)return TEXT("No.23 枪榴弹组");
    if(InRole==EFrontlineRole::AntiTankRifle)return TEXT("Tankgewehr 反坦克步枪组");
    if(InRole==EFrontlineRole::Flamethrower)return GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("Schilt 法军喷火分遣队"):TEXT("Wex 喷火工兵");
    if(InRole==EFrontlineRole::Pioneer)return GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("皇家工兵清障组"):TEXT("Pioniere 排障组");
    if (InRole==EFrontlineRole::Marksman && HasScopedMarksmen(Team)) return TEXT("SMLE 狙击手");
    if (InRole==EFrontlineRole::Officer && HasCommissionedOfficers(Team))
        return GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("Webley 军官观察组"):TEXT("P08 突击军官组");
    if (InRole==EFrontlineRole::Mills && HasStormtroopers(Team)) return TEXT("MP 18 突击队");
    if (GetFactionForTeam(Team)==EFrontlineFaction::British) return RoleName(InRole);
    switch (InRole)
    {
        case EFrontlineRole::Lewis:return TEXT("MG 08/15 机枪组");
        case EFrontlineRole::Mills:return TEXT("Stielhandgranate 突击班");
        case EFrontlineRole::Marksman:return TEXT("JÄGER PATROL");
        case EFrontlineRole::Officer:return TEXT("德军军士组");
        default:return TEXT("Gewehr 98 步枪班");
    }
}

int32 AWarfareFrontlineGameMode::GetSquadSize(EFrontlineRole InRole,int32 Team) const
{ if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::RoleSize(this,InRole,Team);return InRole==EFrontlineRole::Marksman&&GetFactionForTeam(Team)==EFrontlineFaction::German?3:RoleSize(InRole,GetFactionIndexForTeam(Team)); }

const TArray<FFrontlineTechnologyNode>& AWarfareFrontlineGameMode::GetTechnologyNodes() const
{ return GetTechnologyNodesForFaction(PlayerFaction); }

const TArray<FFrontlineTechnologyNode>& AWarfareFrontlineGameMode::GetTechnologyNodesForFaction(EFrontlineFaction Faction)
{
    // Four readable columns; each gun/artillery branch has its own root. IDs are
    // deliberately not row-major: changing old save bits would erase paid training.
    static const TArray<FFrontlineTechnologyNode> British={
        {0,-1,0,0,1,TEXT("靶场操练"),TEXT("步枪命中率提高 10%；距离、掩体与压制仍影响射击。"),-1,TEXT("British_00"),TEXT("步兵协同")},
        {1,0,0,1,2,TEXT("快速装填"),TEXT("各类武器装填时间缩短 15%。"),-1,TEXT("British_01"),TEXT("步兵协同")},
        {2,-1,1,0,1,TEXT("刘易斯压制射击"),TEXT("机枪造成的压制提高 18%。"),-1,TEXT("British_02"),TEXT("机枪支援")},
        {3,-1,3,0,2,TEXT("炮兵联络线"),TEXT("野炮整备时间缩短 15%。呼叫后的落弹延迟不变。"),-1,TEXT("British_03"),TEXT("步炮联络")},
        {4,-1,2,0,1,TEXT("投弹操练"),TEXT("手雷命中率提高 12%，落点散布不变。"),-1,TEXT("British_04"),TEXT("突击整备")},
        {5,4,2,1,2,TEXT("前沿补给"),TEXT("所有步兵小队整备时间缩短 10%。"),-1,TEXT("British_05"),TEXT("突击整备")},
        {6,1,1,2,3,TEXT("交替掩护"),TEXT("正在射击的驻壕机枪附近，推进步兵承受的枪弹压制降低 35%。"),2,TEXT("British_06"),TEXT("步兵协同")},
        {7,2,1,1,2,TEXT("火力分配"),TEXT("机枪压制更大范围的邻近敌军，射程不变。"),-1,TEXT("British_07"),TEXT("机枪支援")},
        {8,7,1,3,3,TEXT("枪组接替"),TEXT("存活同伴靠近后可接替阵亡机枪手，继续使用刘易斯与剩余弹药。"),5,TEXT("British_08"),TEXT("机枪支援")},
        {9,5,2,2,3,TEXT("战地委任"),TEXT("军士升级为军官：指挥命中加成由 25% 升至 50%，范围扩大，重整间隔由 8 秒减至 6 秒。"),1,TEXT("British_09"),TEXT("突击整备")},
        {10,3,3,1,2,TEXT("前沿观察"),TEXT("指挥组与一名存活同伴停稳 2 秒、看清目标后，野炮与迫击炮首轮提前 2.5 秒，散布缩小 30%。"),-1,TEXT("British_10"),TEXT("步炮联络")},
        {11,10,3,2,4,TEXT("烟幕后续射"),TEXT("向己方烟幕区域呼叫野炮时，追加一轮补射。"),4,TEXT("British_11"),TEXT("步炮联络")},
        {12,1,0,2,3,TEXT("狙击学校"),TEXT("精确射手换装镜瞄 SMLE，射程与精准度提高。"),10,TEXT("British_12"),TEXT("狙击操练")},
        {13,12,0,3,3,TEXT("指挥目标"),TEXT("狙击手优先瞄准可见的敌军指挥者，其次为机枪手。"),-1,TEXT("British_13"),TEXT("狙击操练")},
        {14,9,2,3,3,TEXT("战地重整"),TEXT("近期收到附近指挥组命令的推进步兵，承受的枪弹压制短暂降低 30%。"),6,TEXT("British_14"),TEXT("突击整备")},
        {15,1,3,3,2,TEXT("间歇补弹"),TEXT("驻壕士兵在低压制、5 秒无目标且弹药不足半数时主动装填。"),5,TEXT("British_15"),TEXT("步炮联络")}
    };
    static const TArray<FFrontlineTechnologyNode> German={
        {0,-1,0,0,1,TEXT("瞄准操练"),TEXT("步枪命中率提高 10%；距离、掩体与压制仍影响射击。"),-1,TEXT("German_00"),TEXT("纵深防御")},
        {1,0,0,1,2,TEXT("装填规程"),TEXT("各类武器装填时间缩短 15%。"),-1,TEXT("German_01"),TEXT("纵深防御")},
        {2,-1,1,0,1,TEXT("受控点射"),TEXT("机枪造成的压制提高 18%。"),-1,TEXT("German_02"),TEXT("机枪阵地")},
        {3,-1,3,0,2,TEXT("炮兵联络所"),TEXT("野炮整备时间缩短 15%。呼叫后的落弹延迟不变。"),-1,TEXT("German_03"),TEXT("突击火力")},
        {4,-1,2,0,1,TEXT("壕战投弹"),TEXT("手雷命中率提高 12%，落点散布不变。"),-1,TEXT("German_04"),TEXT("突击分队")},
        {5,4,2,1,2,TEXT("前运补给"),TEXT("所有步兵小队整备时间缩短 10%。"),-1,TEXT("German_05"),TEXT("突击分队")},
        {6,1,0,2,3,TEXT("前线军官"),TEXT("士官升级为军官：指挥命中加成由 25% 升至 50%，每 6 秒重整友军，行进中仍可下令。"),2,TEXT("German_06"),TEXT("纵深防御")},
        {7,2,1,1,2,TEXT("隐蔽枪位"),TEXT("驻壕机枪手遭枪弹命中的机会与承受的压制均降低 25%。"),-1,TEXT("German_07"),TEXT("机枪阵地")},
        {8,7,1,2,3,TEXT("留守掩护"),TEXT("整壕冲锋时，有枪手的机枪组留下掩护。前方16米内有己方冲锋步兵时，驻壕机枪命中率提高25%，点射恢复时间缩短35%。壕内只剩机枪组后，可再次下令前进。"),5,TEXT("German_08"),TEXT("机枪阵地")},
        {9,5,2,2,3,TEXT("烟幕渗透"),TEXT("己方烟幕中的推进突击队承受的枪弹压制减半，并接近至更短距离开火。"),1,TEXT("German_09"),TEXT("突击分队")},
        {10,3,3,1,2,TEXT("炮位测地"),TEXT("迫击炮散布缩小。指挥组与一名存活同伴停稳 2 秒、看清目标后，野炮与迫击炮首轮另提前 2.5 秒，散布再缩小 30%。"),-1,TEXT("German_10"),TEXT("突击火力")},
        {11,10,3,2,4,TEXT("压制突击"),TEXT("手雷对深受压制的驻壕敌军命中率提高 35%。"),4,TEXT("German_11"),TEXT("突击火力")},
        {12,9,2,3,3,TEXT("风暴突击队"),TEXT("突击班换装 MP 18，获得近距离自动火力。1918 年行动可用。"),4,TEXT("German_12"),TEXT("突击分队")},
        {13,8,1,3,3,TEXT("交叉火力"),TEXT("分散驻壕、同时射击的机枪形成交叉火力，压制提高 40%。"),6,TEXT("German_13"),TEXT("机枪阵地")},
        {14,12,3,3,3,TEXT("壕内自动火力"),TEXT("MP 18 点射也会压制目标附近的守军。"),11,TEXT("German_14"),TEXT("突击火力")},
        {15,12,0,3,2,TEXT("备用弹鼓"),TEXT("风暴队驻壕、低压制、5 秒无目标且弹药不足半数时主动换弹。"),5,TEXT("German_15"),TEXT("纵深防御")}
    };
    static const TArray<FFrontlineTechnologyNode> ExpandedBritish=[](){auto N=British;FrontlineCampaignV32::AppendTechnologyNodes(N,false);FrontlineCampaignV41::AppendTechnologyNodes(N,false);FrontlineGameplayV52::AppendTechnologyNodes(N,false);return N;}();
    static const TArray<FFrontlineTechnologyNode> ExpandedGerman=[](){auto N=German;FrontlineCampaignV32::AppendTechnologyNodes(N,true);FrontlineCampaignV41::AppendTechnologyNodes(N,true);FrontlineGameplayV52::AppendTechnologyNodes(N,true);return N;}();
    return Faction==EFrontlineFaction::German?ExpandedGerman:ExpandedBritish;
}

bool AWarfareFrontlineGameMode::HasTechnology(int32 Id,int32 Team) const
{
    // The FR/RU archive owns tactical options, never the legacy percent tree.
    if(WarfareFRRUCampaignV82::Active(this))return false;
    if(Team==1&&FrontlineUltimateArchiveV8215::FullArmory(this,Team))
        return GetTechnologyNodesForFaction(GetFactionForTeam(Team)).IsValidIndex(Id);
    if(Team==1)return FrontlineEnemyPolicyV84415::ResearchAvailable(this,Id);
    // Each army has a separate player record; the opponent never inherits another
    // campaign's upgrades merely because the player changes national equipment.
    return Team==0 && ValidTechnology(Id) && (GetActiveResearch().TechnologyMask&(1u<<Id))!=0;
}

FString AWarfareFrontlineGameMode::TechnologyBlockReason(int32 Id) const
{
    if (!ValidTechnology(Id)) return TEXT("项目不可用");
    if (!GetCurrentCampaign()) return TEXT("请先选择战役");
    if (Phase==EFrontlinePhase::Fighting) return TEXT("战后整备时研习");
    if (bProgressionReadOnly) return TEXT("无法读取存档，原文件已保留。");
    if (HasTechnology(Id)) return TEXT("已研习");
    const FString Assignment=TechnologyRoleBlockReason(Id);if(!Assignment.IsEmpty())return Assignment;
    const FString Modern=FrontlineCampaignV32::NewTechnologyBlockReason(this,Id);if(!Modern.IsEmpty())return Modern;
    const auto& Node=GetTechnologyNodes()[Id];
    for (int32 Prerequisite:{Node.Prerequisite,Node.SecondaryPrerequisite})
        if (Prerequisite!=INDEX_NONE && !HasTechnology(Prerequisite))
            return FString::Printf(TEXT("需要：%s"),GetTechnologyNodes()[Prerequisite].Title);
    if (GetResearchPoints()<Node.Cost) return TEXT("战绩点不足");
    return FString();
}

int32 AWarfareFrontlineGameMode::GetResearchPoints() const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::ResearchPoints(this);
    return FrontlineDebugV37::UnlimitedResearch(this)?99999:GetActiveResearch().ResearchPoints;
}

bool AWarfareFrontlineGameMode::PurchaseTechnology(int32 Id)
{
    if(WarfareFRRUCampaignV82::Active(this))return false;
    const FString Reason=TechnologyBlockReason(Id);
    if (!Reason.IsEmpty()) { LastMessage=Reason; return false; }
    auto& Army=ActiveResearchMutable();
    const FFrontlineArmyProgress Before=Army;
    Army.TechnologyMask|=1u<<Id;
    if(!FrontlineDebugV37::UnlimitedResearch(this))Army.ResearchPoints-=GetTechnologyNodes()[Id].Cost;
    if (!SaveProgression()) { Army=Before; LastMessage=ProgressionSaveStatus; return false; }
    if (Phase==EFrontlinePhase::Briefing)
    {
        // Apply pre-battle artillery preparation immediately; other bonuses are
        // consumed by actual fire, reload, grenade, and recruitment paths.
        const auto* Mission=GetSelectedCampaignMission();
        ArtilleryReadyAt[0]=EffectiveArtillerySeconds(0)*(Mission?Mission->PlayerArtilleryDelay/SupportCooldownSeconds(EFrontlineSupport::Artillery):1.f);
    }
    LastMessage=FString::Printf(TEXT("%s · %s"),GetTechnologyNodes()[Id].Title,GetTechnologyNodes()[Id].Effect);
    return true;
}

bool AWarfareFrontlineGameMode::SelectPlayerFaction(EFrontlineFaction Faction)
{
    if (!ValidFaction(Faction) || Phase==EFrontlinePhase::Fighting) return false;
    if(WarfareFRRUCampaignV82::Active(this)&&!WarfareFRRUCampaignInternalV82::RetireEncounter(this))return false;
    if (Faction==PlayerFaction){if(WarfareFRRUCampaignV82::Active(this)){WarfareFRRUCampaignV82::Deactivate(this);ResetEncounter();}return true;}
    const auto Before=PlayerFaction; const FGuid BeforeCampaign=ActiveCampaignId;
    PlayerFaction=Faction; ActiveCampaignId.Invalidate();
    if (!bProgressionReadOnly && !SaveProgression()) { PlayerFaction=Before; ActiveCampaignId=BeforeCampaign; LastMessage=ProgressionSaveStatus; return false; }
    WarfareFRRUCampaignV82::Deactivate(this);ResetEncounter();
    LastMessage=FString::Printf(TEXT("%s · 准备出战"),GetFactionName());
    return true;
}

float AWarfareFrontlineGameMode::WeaponReloadScale(int32 Team) const
{ return HasTechnology(int32(EFrontlineTechnology::ReloadDrill),Team)?.85f:1.f; }
float AWarfareFrontlineGameMode::RifleAccuracyScale(int32 Team) const
{ return HasTechnology(int32(EFrontlineTechnology::RifleDrill),Team)?1.10f:1.f; }
float AWarfareFrontlineGameMode::MachinegunPressureScale(int32 Team) const
{ return HasTechnology(int32(EFrontlineTechnology::SupportingFire),Team)?1.18f:1.f; }
float AWarfareFrontlineGameMode::GrenadeHitChanceScale(int32 Team) const
{ return HasTechnology(int32(EFrontlineTechnology::GrenadeDrill),Team)?1.12f:1.f; }
float AWarfareFrontlineGameMode::EffectiveRecruitSeconds(EFrontlineRole InRole,int32 Team) const
{
    if(WarfareFRRUCampaignV82::Active(this))
    {const float Native=WarfareFRRUEquipmentV82::ReadinessSeconds(this,InRole,Team);return Native==MAX_flt?MAX_flt:FrontlineGameplayV43::PreparationSeconds(Native,Team,GetCampaignDifficulty());}
    // Both six-man rifle sections use the same manpower budget. The former
    // German five-man draft discount must not survive the size normalization.
    return FrontlineGameplayV43::PreparationSeconds(RoleRecruitSeconds(InRole)*(HasTechnology(int32(EFrontlineTechnology::FieldSupply),Team)?.90f:1.f)*(HasTechnology(int32(EFrontlineTechnology::VeteranReserves),Team)?.92f:1.f),Team,GetCampaignDifficulty());
}
float AWarfareFrontlineGameMode::EffectiveArtillerySeconds(int32 Team) const
{ if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUEquipmentV82::SupportCooldown(this,EFrontlineSupport::Artillery,Team);return FrontlineGameplayV43::PreparationSeconds(SupportCooldownSeconds(EFrontlineSupport::Artillery)*(HasTechnology(int32(EFrontlineTechnology::ArtilleryLiaison),Team)?.85f:1.f),Team,GetCampaignDifficulty()); }

bool AWarfareFrontlineGameMode::HasFactionTechnology(EFrontlineFaction Faction,int32 Id,int32 Team) const
{ return GetFactionForTeam(Team)==Faction && HasTechnology(Id,Team); }

bool AWarfareFrontlineGameMode::IsInFriendlySmoke(const FVector& Point,int32 Team) const
{
    for (const FFrontlineSmokeZone& Zone:SmokeZones)
        if (!Zone.bGas && Zone.Team==Team && Elapsed>=Zone.StartedAt && Elapsed<Zone.ExpiresAt && FVector::DistSquared2D(Point,Zone.Centre)<=FMath::Square(Zone.Radius)) return true;
    return false;
}

bool AWarfareFrontlineGameMode::HasForwardObserver(const FVector& Point,int32 Team) const
{
    if(FrontlineFieldV32::HasAirObservation(this,Team,Point))return true;
    for (const AWarfareFrontlineSoldier* Unit:Units)
    {
        if (Unit->Team!=Team || !Unit->IsActiveCombatant() || Unit->bMoving || Unit->Suppression>.55f
            || Unit->Role!=EFrontlineRole::Marksman
            || FVector::DistSquared2D(Unit->Feet(),Point)>FMath::Square((Unit->IsCommissionedOfficer() && Unit->GetFaction()==EFrontlineFaction::British?6500.f:5000.f)*EncounterVisibilityScale())
            || DirectFireMultiplier(Unit->Feet(),Point)<1.f) continue;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(FrontlineObserver),true);
        for (const AWarfareFrontlineSoldier* Other:Units) Params.AddIgnoredActor(Other);
        if (!GetWorld()->LineTraceTestByChannel(Unit->Feet()+FVector(0,0,145),Point+FVector(0,0,135),ECC_Visibility,Params)) return true;
    }
    return false;
}

float AWarfareFrontlineGameMode::IncomingBulletPressureScale(const AWarfareFrontlineSoldier* Unit) const
{
    float Scale=FrontlineFieldV32::IsFlameOperator(Unit)?.90f:1.f;
    if (HasFactionTechnology(EFrontlineFaction::German,7,Unit->Team) && Unit->IsMachineGunner() && CoverFor(Unit)>0.f) Scale*=.75f;
    if (HasFactionTechnology(EFrontlineFaction::German,9,Unit->Team) && Unit->Role==EFrontlineRole::Mills && Unit->bMoving
        && IsInFriendlySmoke(Unit->Feet(),Unit->Team)) Scale*=.5f;
    if (Unit->bMoving && HasFactionTechnology(EFrontlineFaction::British,14,Unit->Team))
        for (const AWarfareFrontlineSoldier* Leader:Units)
            if (Leader->Team==Unit->Team && Leader->IsActiveCombatant() && Leader->IsSectionLeader()
                && Elapsed-Leader->LastSectionOrderAt<=8.f
                && FVector::DistSquared2D(Leader->Feet(),Unit->Feet())<=FMath::Square(650.f)) { Scale*=.70f; break; }
    if (HasFactionTechnology(EFrontlineFaction::British,6,Unit->Team) && Unit->bMoving)
        for (const AWarfareFrontlineSoldier* Gun:Units)
            if (Gun->Team==Unit->Team && Gun->IsAlive() && Gun->IsMachineGunner() && !Gun->bMoving && CoverFor(Gun)>0.f
                && Elapsed-Gun->LastAutomaticFireAt<=2.f
                && FVector::DistSquared2D(Gun->Feet(),Unit->Feet())<=FMath::Square(1600.f)) return Scale*.65f;
    return Scale;
}

float AWarfareFrontlineGameMode::IncomingBulletHitChanceScale(const AWarfareFrontlineSoldier* Unit) const
{
    const float Protection=FrontlineFieldV32::IsFlameOperator(Unit)?.88f:1.f;
    return Protection*FrontlineFieldSitesV59::BulletCover(Unit)*(HasFactionTechnology(EFrontlineFaction::German,7,Unit->Team)&&Unit->IsMachineGunner()&&CoverFor(Unit)>0.f?.75f:1.f);
}

float AWarfareFrontlineGameMode::OutgoingPressureScale(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target) const
{
    float Scale=Shooter->IsMachineGunner()?MachinegunPressureScale(Shooter->Team):1.f;
    if (!Shooter->IsAutomaticWeapon() && Target->bMoving && CoverFor(Shooter)>0.f
        && HasFactionTechnology(EFrontlineFaction::German,6,Shooter->Team)) Scale*=1.4f;
    if (Shooter->IsMachineGunner() && CoverFor(Shooter)>0.f && !Shooter->bMoving
        && HasFactionTechnology(EFrontlineFaction::German,13,Shooter->Team))
        for (const AWarfareFrontlineSoldier* Gun:Units)
            if (Gun!=Shooter && Gun->Team==Shooter->Team && Gun->IsAlive() && Gun->IsMachineGunner()
                && !Gun->bMoving && CoverFor(Gun)>0.f && Elapsed-Gun->LastAutomaticFireAt<=2.f
                && FVector::DistSquared2D(Gun->Feet(),Shooter->Feet())>FMath::Square(300.f)
                && FrontlineWeapons::DirectFireRangeDistanceCm(Gun,Target->Feet())<=FrontlineWeapons::ProfileFor(Gun).FireRangeCm*EncounterVisibilityScale()
                && HasSight(Gun,Target)) { Scale*=1.4f; break; }
    return Scale;
}

float AWarfareFrontlineGameMode::AdjacentPressureRadius(const AWarfareFrontlineSoldier* Shooter) const
{
    if(!Shooter||!Shooter->IsMachineGunner()||!HasFactionTechnology(EFrontlineFaction::British,7,Shooter->Team))return 180.f;
    extern bool FrontlineUseLegacyBalanceV8447();
    if(FrontlineUseLegacyBalanceV8447())return 480.f;
    // A fixed 480 cm budget was below the Lewis profile's existing 540 cm
    // beaten zone, so the final max() silently erased this paid upgrade.
    return FrontlineWeapons::ProfileFor(Shooter).AdjacentRadiusCm*1.25f;
}

float AWarfareFrontlineGameMode::AssaultContactScale(const AWarfareFrontlineSoldier* Unit) const
{ return Unit->Role==EFrontlineRole::Mills && HasFactionTechnology(EFrontlineFaction::German,9,Unit->Team)
    && IsInFriendlySmoke(Unit->Feet(),Unit->Team)?.6f:1.f; }

void AWarfareFrontlineGameMode::LoadProgression(bool bKeepAssignedFile)
{
    PlayerFaction=EFrontlineFaction::British; ArmyProgress[0]={}; ArmyProgress[1]={};
    RewardedEncounterIds.Empty(); bProgressionReadOnly=false;
    CampaignRuns.Empty(); ActiveCampaignId.Invalidate(); BoundCampaignId.Invalidate(); BoundCampaignEncounterId.Invalidate();
    BoundCampaignMission=INDEX_NONE; bCampaignMissionAdvanced=false;
    const bool FeatureReview=FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineV13Verify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview"));
    FString ReviewUserDir;
    const bool IsolatedNoSave=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        && FParse::Value(FCommandLine::Get(),TEXT("UserDir="),ReviewUserDir)
        && !ReviewUserDir.IsEmpty()&&!FPaths::IsRelative(ReviewUserDir);
    if (!bKeepAssignedFile) bProgressionVolatile=IsolatedNoSave||WarfareFRRUCampaignV82::PersistenceReviewRequestedV82(true)
        || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFDistributionCheck"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineProfile"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineMotionReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineLookReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineV14Verify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV14ManualReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineV15TechnologyVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV15UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV16UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV17UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV18UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV19WeatherReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV20Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV21Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV22Benchmark"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV22ImageReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV23MechanicsVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV23Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVisual"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV24GrenadeReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV24Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV23FieldReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV23DisplayReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV24DisplayReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV25HDRReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV26Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV28Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV28UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV28FieldReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV29Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV30Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV31Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV32Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV33Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV34Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV35Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV36Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV37Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV38Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV39Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFPromoV49"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV45Review")) || FParse::Param(FCommandLine::Get(),TEXT("WFV46Review")) || FParse::Param(FCommandLine::Get(),TEXT("WFV47Review")) || FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV49Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))
        || (FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))||(FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV63Review")))) || FParse::Param(FCommandLine::Get(),TEXT("WFV52Balance"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV33Benchmark"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV30TacticsVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV27Review"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV27UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV27RenderReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV27PathReference"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV26UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV24UIReview"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV18AgileVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV18AudioVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV17AgileVerify"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFProgressionVerify"));
    if (bProgressionVolatile) { ProgressionSaveStatus=TEXT("独立验证档案"); return; }
    if (FeatureReview && !bKeepAssignedFile)
    {
        ProgressionFile=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V13/feature-profile.json"));
        if (FParse::Param(FCommandLine::Get(),TEXT("WFV13FreshProfile"))) { SaveProgression(); return; }
    }
    if (ProgressionFile.IsEmpty()) ProgressionFile=FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("Warfare1917"),TEXT("Progression-v13.json"));
    if (!IFileManager::Get().FileExists(*ProgressionFile)) { SaveProgression(); return; }
    FString Json; TSharedPtr<FJsonObject> Root;
    bool Valid=ProgressionBytesV86(IFileManager::Get().FileSize(*ProgressionFile)) && FFileHelper::LoadFileToString(Json,*ProgressionFile)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) && Root.IsValid();
    int32 Schema=0,Faction=0;
    FFrontlineArmyProgress Loaded[2]; TArray<FString> LoadedReceipts;
    TArray<FFrontlineCampaignProgress> LoadedCampaigns; FGuid LoadedActiveCampaign;
    Valid=Valid && ReadBoundedInteger(*Root,TEXT("schema"),1,6,Schema) && ReadBoundedInteger(*Root,TEXT("player_faction"),0,1,Faction);
    for (int32 I=0;Valid && I<2;++I)
    {
        const TSharedPtr<FJsonObject>* Army=nullptr; int32 Mask=0;
        Valid=Root->TryGetObjectField(ArmyKey(I),Army) && Army && Army->IsValid()
            && ReadBoundedInteger(**Army,TEXT("research_points"),0,99999,Loaded[I].ResearchPoints)
            && ReadBoundedInteger(**Army,TEXT("technology_mask"),0,int32(Schema==1?LegacyTechnologyMask:Schema<4?((1u<<12)-1):KnownTechnologyMask),Mask)
            && ReadBoundedInteger(**Army,TEXT("victories"),0,999999,Loaded[I].Victories)
            && ReadBoundedInteger(**Army,TEXT("defeats"),0,999999,Loaded[I].Defeats);
        Loaded[I].TechnologyMask=uint32(Mask);
        if (Schema==1)
        {
            // Validate the original graph before migration. All original purchases,
            // army balances and reward receipts survive; no new nodes are granted.
            for (int32 Id:{1,3,5}) if ((Mask&(1<<Id)) && !(Mask&(1<<(Id-1)))) Valid=false;
        }
        else for (const auto& Node:GetTechnologyNodesForFaction(EFrontlineFaction(I)))
            for (int32 Required:{Node.Prerequisite,Node.SecondaryPrerequisite})
                if ((Mask&(1<<Node.Id)) && !FrontlineGameplayV51::PaidPrerequisiteSatisfied(Node.Id,Required,uint32(Mask))) Valid=false;
    }
    const TArray<TSharedPtr<FJsonValue>>* Receipts=nullptr;
    Valid=Valid && Root->TryGetArrayField(TEXT("rewarded_encounters"),Receipts) && Receipts && Receipts->Num()<=128;
    if (Valid) for (const auto& Entry:*Receipts)
    {
        FString Receipt; FGuid Parsed;
        if (!Entry.IsValid() || !Entry->TryGetString(Receipt) || !FGuid::ParseExact(Receipt,EGuidFormats::Digits,Parsed) || !Parsed.IsValid()) { Valid=false; break; }
        LoadedReceipts.AddUnique(Receipt);
    }
    if (Valid && Schema>=3)
    {
        const TArray<TSharedPtr<FJsonValue>>* Runs=nullptr; FString Active;
        // Allow the two legacy-faction archives added during a full-archive
        // migration. NewCampaign still refuses to exceed its existing 256 cap.
        Valid=Root->TryGetArrayField(TEXT("campaigns"),Runs) && Runs && Runs->Num()<=(Schema>=4?258:256)
            && Root->TryGetStringField(TEXT("active_campaign"),Active)
            && (Active.IsEmpty() || (FGuid::ParseExact(Active,EGuidFormats::Digits,LoadedActiveCampaign) && LoadedActiveCampaign.IsValid()));
        if (Valid) for (const auto& Entry:*Runs)
        {
            const TSharedPtr<FJsonObject>* Object=nullptr;
            FFrontlineCampaignProgress Run; FString Id; int32 National=0; FDateTime Created,Updated;
            const bool Good=Entry.IsValid() && Entry->TryGetObject(Object) && Object && Object->IsValid()
                && (*Object)->TryGetStringField(TEXT("id"),Id) && FGuid::ParseExact(Id,EGuidFormats::Digits,Run.Id) && Run.Id.IsValid()
                && !LoadedCampaigns.ContainsByPredicate([&Run](const FFrontlineCampaignProgress& Other){ return Other.Id==Run.Id; })
                && ReadBoundedInteger(**Object,TEXT("faction"),0,1,National)
                && ReadBoundedInteger(**Object,TEXT("completed_missions"),0,Schema<4?3:Schema<5?10:GetCampaignMissionsForFaction(EFrontlineFaction(National)).Num(),Run.CompletedMissions)
                && ReadBoundedInteger(**Object,TEXT("selected_mission"),0,Schema<4?2:Schema<5?9:GetCampaignMissionsForFaction(EFrontlineFaction(National)).Num()-1,Run.SelectedMission)
                && (Schema>=6 || Run.SelectedMission<=Run.CompletedMissions)
                && (*Object)->TryGetStringField(TEXT("created_utc"),Run.CreatedUtc) && Run.CreatedUtc.Len()<=64
                && (*Object)->TryGetStringField(TEXT("updated_utc"),Run.UpdatedUtc) && Run.UpdatedUtc.Len()<=64
                && FDateTime::ParseIso8601(*Run.CreatedUtc,Created) && FDateTime::ParseIso8601(*Run.UpdatedUtc,Updated) && Updated>=Created;
            if (!Good) { Valid=false; break; }
            Run.Faction=EFrontlineFaction(National);
            // Optional within schema 4: older builds can still open an archive;
            // pre-V30 archives inherit the original Rookie rules.
            if(!FrontlineDifficultyV30::ReadArchiveDifficulty(**Object,Run.Difficulty)){Valid=false;break;}
            auto ReadOptionalMask=[&](const TCHAR* Key,int32 Max,uint32& Target)
            {
                if(!(*Object)->HasField(Key))return true;
                int32 Value=0;if(!ReadBoundedInteger(**Object,Key,0,Max,Value))return false;
                Target=uint32(Value);return true;
            };
            if((*Object)->HasField(TEXT("armour_selection_explicit_v84413"))
                &&!(*Object)->TryGetBoolField(TEXT("armour_selection_explicit_v84413"),Run.bArmourSelectionExplicitV84413))
            {Valid=false;break;}
            if(!ReadOptionalMask(TEXT("role_selection"),(1<<int32(EFrontlineRole::Count))-1,Run.RoleSelectionMask)
                || !ReadOptionalMask(TEXT("support_selection"),(1<<int32(EFrontlineSupport::Count))-1,Run.SupportSelectionMask)
                || !ReadOptionalMask(TEXT("armour_research_v63"),65535,Run.ArmourResearchMaskV63)
                || !ReadOptionalMask(TEXT("armour_selection_v63"),(1<<int32(EFrontlineVehicle::Count))-1,Run.ArmourSelectionMaskV63)
                || !ReadOptionalMask(TEXT("armour_configuration_v836"),13,Run.ArmourConfigurationMaskV836)
                || (Run.ArmourConfigurationMaskV836&~13u)
                || !ReadOptionalMask(TEXT("pioneer_orders_v63"),(1<<20)-1,Run.PioneerOrdersMaskV63)
                || !ReadOptionalMask(TEXT("smoke_orders_v63"),(1<<20)-1,Run.SmokeOrdersMaskV63)
                || !ReadOptionalMask(TEXT("armour_orders_v63"),(1<<20)-1,Run.ArmourOrdersMaskV63)
                || !ReadOptionalMask(TEXT("requisition_paid_v836"),(1<<20)-1,Run.RequisitionPaidMaskV836)
                || (Run.RequisitionPaidMaskV836&~(Run.PioneerOrdersMaskV63|Run.SmokeOrdersMaskV63|Run.ArmourOrdersMaskV63))
                || FrontlineCampaignV32::BitCount(Run.ArmourSelectionMaskV63)>2
                || (Run.PioneerOrdersMaskV63&Run.SmokeOrdersMaskV63) || (Run.PioneerOrdersMaskV63&Run.ArmourOrdersMaskV63) || (Run.SmokeOrdersMaskV63&Run.ArmourOrdersMaskV63)
                || !ReadOptionalMask(TEXT("acknowledged_events"),Schema<5?1023:(1<<GetCampaignMissionsForFaction(Run.Faction).Num())-1,Run.AcknowledgedEventMask)
                || FrontlineCampaignV32::BitCount(Run.RoleSelectionMask)>5 || FrontlineCampaignV32::BitCount(Run.SupportSelectionMask)>6)
            {Valid=false;break;}
            static const int32 ArmourPreLegacy[]={-1,0,0,2,1,3,4,4,5};
            static const int32 ArmourPreCurrent[]={-1,-1,-1,2,0,10,-1,6,4,1,9,3,11,14,7,8};
            auto PrerequisitesValid=[&](const int32* Prerequisites,int32 Count)
            {
                for(int32 I=0;I<Count;++I)if((Run.ArmourResearchMaskV63&(1u<<I))&&Prerequisites[I]>=0
                    &&!(Run.ArmourResearchMaskV63&(1u<<Prerequisites[I])))return false;
                return true;
            };
            const bool LegacyArmour=Run.ArmourResearchMaskV63<=511&&PrerequisitesValid(ArmourPreLegacy,9);
            if(!LegacyArmour&&!PrerequisitesValid(ArmourPreCurrent,16))Valid=false;
            if(!Valid)break;
            if((*Object)->HasField(TEXT("achievement_eligible_v76"))&&!(*Object)->TryGetBoolField(TEXT("achievement_eligible_v76"),Run.bAchievementEligibleV76)){Valid=false;break;}
            if(!(*Object)->HasField(TEXT("armour_configuration_v836")))
            {
                if(Run.ArmourSelectionMaskV63&(1u<<int32(EFrontlineVehicle::MarkIVFemale)))Run.ArmourConfigurationMaskV836|=1u<<int32(EFrontlineVehicle::MarkIVMale);
                if(Run.ArmourSelectionMaskV63&(1u<<int32(EFrontlineVehicle::BeuteMarkIVFemale)))Run.ArmourConfigurationMaskV836|=1u<<int32(EFrontlineVehicle::BeuteMarkIV);
                if(Run.ArmourSelectionMaskV63&(1u<<int32(EFrontlineVehicle::A7VGretchen)))Run.ArmourConfigurationMaskV836|=1u<<int32(EFrontlineVehicle::A7V);
            }
            // Ordinary legacy assignments were always paid. Assisted archives
            // lack payment provenance, so cancellation must not invent a refund.
            if(!(*Object)->HasField(TEXT("requisition_paid_v836"))&&Run.bAchievementEligibleV76)
                Run.RequisitionPaidMaskV836=Run.PioneerOrdersMaskV63|Run.SmokeOrdersMaskV63|Run.ArmourOrdersMaskV63;
            if((*Object)->HasField(TEXT("mobile_gun_doctrine"))&&!(*Object)->TryGetBoolField(TEXT("mobile_gun_doctrine"),Run.bMobileGunV52)){Valid=false;break;}
            FrontlineGameplayV52::NormalizeLegacyLoadout(Run);
            if (Schema<4) Run.Research=Loaded[National];
            else
            {
                const TSharedPtr<FJsonObject>* Research=nullptr; int32 Mask=0;
                Valid=(*Object)->TryGetObjectField(TEXT("research"),Research) && Research && Research->IsValid()
                    && ReadBoundedInteger(**Research,TEXT("research_points"),0,99999,Run.Research.ResearchPoints)
                    && ReadBoundedInteger(**Research,TEXT("technology_mask"),0,int32(KnownTechnologyMask),Mask)
                    && ReadBoundedInteger(**Research,TEXT("victories"),0,999999,Run.Research.Victories)
                    && ReadBoundedInteger(**Research,TEXT("defeats"),0,999999,Run.Research.Defeats);
                Run.Research.TechnologyMask=uint32(Mask);
                for (const auto& Node:GetTechnologyNodesForFaction(Run.Faction))
                    for (int32 Required:{Node.Prerequisite,Node.SecondaryPrerequisite})
                        if ((Mask&(1<<Node.Id)) && !FrontlineGameplayV51::PaidPrerequisiteSatisfied(Node.Id,Required,uint32(Mask))) Valid=false;
                if (!Valid) break;
            }
            Run.RouteRevision=0;
            if(Schema>=5 && !ReadBoundedInteger(**Object,TEXT("route_revision"),41,41,Run.RouteRevision)){Valid=false;break;}
            if(!FrontlineCampaignV41::MigrateLegacy(Run)){Valid=false;break;}
            if((Schema>=6&&!(*Object)->HasField(TEXT("strategy_revision")))||!FrontlineCampaignV52::ReadArchive(**Object,Run)){Valid=false;break;}
            if(!FrontlineCombatRecordV65::ReadArchive(**Object,Run.CombatRecordV65,Run.Research.Victories+Run.Research.Defeats>0)){Valid=false;break;}
            LoadedCampaigns.Add(MoveTemp(Run));
        }
        if (Valid && LoadedActiveCampaign.IsValid())
            Valid=LoadedCampaigns.ContainsByPredicate([&LoadedActiveCampaign,Faction](const FFrontlineCampaignProgress& Run)
                { return Run.Id==LoadedActiveCampaign && int32(Run.Faction)==Faction; });
    }
    if (!Valid)
    {
        bProgressionReadOnly=true; ProgressionSaveStatus=TEXT("无法读取存档，原文件已保留。");
        UE_LOG(LogTemp,Warning,TEXT("V13 progression invalid or unsupported; original preserved: %s"),*ProgressionFile); return;
    }
    // Older faction-wide research is snapshotted into each existing archive.
    // A faction without any archive gets a named-by-date legacy route only when
    // it contains paid or earned progress. New campaigns never copy this state.
    if (Schema<4)
    {
        for (int32 I=0;I<2;++I)
            if (!LoadedCampaigns.ContainsByPredicate([I](const FFrontlineCampaignProgress& R){return int32(R.Faction)==I;})
                && (Loaded[I].TechnologyMask!=0 || Loaded[I].ResearchPoints!=2 || Loaded[I].Victories!=0 || Loaded[I].Defeats!=0))
            {
                FFrontlineCampaignProgress Legacy; Legacy.Id=FGuid::NewGuid(); Legacy.Faction=EFrontlineFaction(I);
                Legacy.Research=Loaded[I]; Legacy.CombatRecordV65.bIncludesEarlierUnrecordedBattles=Loaded[I].Victories+Loaded[I].Defeats>0; Legacy.CreatedUtc=Legacy.UpdatedUtc=FDateTime::UtcNow().ToIso8601();
                LoadedCampaigns.Add(Legacy);
                if (!LoadedActiveCampaign.IsValid() && I==Faction) LoadedActiveCampaign=Legacy.Id;
            }
        const FString MigrationBackup=ProgressionFile+TEXT(".pre-v23");
        if (!IFileManager::Get().FileExists(*MigrationBackup)
            && IFileManager::Get().Copy(*MigrationBackup,*ProgressionFile,false,false)!=COPY_OK)
        { bProgressionReadOnly=true; ProgressionSaveStatus=TEXT("无法备份旧存档，原文件已保留。"); return; }
    }
    if(Schema<5)
    {
        const FString MigrationBackup=ProgressionFile+TEXT(".pre-v41");
        if(!IFileManager::Get().FileExists(*MigrationBackup) && IFileManager::Get().Copy(*MigrationBackup,*ProgressionFile,false,false)!=COPY_OK)
        {bProgressionReadOnly=true;ProgressionSaveStatus=TEXT("无法备份旧存档，原文件已保留。");return;}
    }
    PlayerFaction=EFrontlineFaction(Faction); ArmyProgress[0]=Loaded[0]; ArmyProgress[1]=Loaded[1];
    CampaignRuns=MoveTemp(LoadedCampaigns); ActiveCampaignId=LoadedActiveCampaign;
    RewardedEncounterIds=MoveTemp(LoadedReceipts); ProgressionSaveStatus=TEXT("档案已载入");
    if (Schema<6)
    {
        const FString MigrationBackup=ProgressionFile+TEXT(".pre-v52");
        if(!IFileManager::Get().FileExists(*MigrationBackup)&&IFileManager::Get().Copy(*MigrationBackup,*ProgressionFile,false,false)!=COPY_OK)
        {bProgressionReadOnly=true;ProgressionSaveStatus=TEXT("无法备份旧存档，原文件已保留。");return;}
        if (SaveProgression()) ProgressionSaveStatus=TEXT("战役存档已更新。");
        else bProgressionReadOnly=true;
    }
}

bool AWarfareFrontlineGameMode::SaveProgression()
{
    if(FrontlineUltimateArchiveV8215::Active(this))return true;
    if (bProgressionVolatile) { ProgressionSaveStatus=TEXT("独立验证档案"); return true; }
    if (bProgressionReadOnly || ProgressionFile.IsEmpty()) return false;
    const auto Root=MakeShared<FJsonObject>(); Root->SetNumberField(TEXT("schema"),6);
    Root->SetNumberField(TEXT("player_faction"),int32(PlayerFaction));
    Root->SetStringField(TEXT("updated_utc"),FDateTime::UtcNow().ToIso8601());
    for (int32 I=0;I<2;++I)
    {
        const auto Army=MakeShared<FJsonObject>(); const auto& State=ArmyProgress[I];
        Army->SetNumberField(TEXT("research_points"),State.ResearchPoints);
        Army->SetNumberField(TEXT("technology_mask"),State.TechnologyMask);
        Army->SetNumberField(TEXT("victories"),State.Victories); Army->SetNumberField(TEXT("defeats"),State.Defeats);
        Root->SetObjectField(ArmyKey(I),Army);
    }
    TArray<TSharedPtr<FJsonValue>> Receipts;
    for (const auto& Id:RewardedEncounterIds) Receipts.Add(MakeShared<FJsonValueString>(Id));
    Root->SetArrayField(TEXT("rewarded_encounters"),Receipts);
    TArray<TSharedPtr<FJsonValue>> Runs;
    for (const auto& Run:CampaignRuns)
    {
        const auto Item=MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("id"),Run.Id.ToString(EGuidFormats::Digits));
        Item->SetNumberField(TEXT("faction"),int32(Run.Faction));
        Item->SetNumberField(TEXT("difficulty"),int32(Run.Difficulty));
        Item->SetNumberField(TEXT("completed_missions"),Run.CompletedMissions);
        Item->SetNumberField(TEXT("selected_mission"),Run.SelectedMission);
        Item->SetNumberField(TEXT("route_revision"),Run.RouteRevision);
        Item->SetNumberField(TEXT("role_selection"),Run.RoleSelectionMask);
        Item->SetNumberField(TEXT("support_selection"),Run.SupportSelectionMask);
        Item->SetBoolField(TEXT("achievement_eligible_v76"),Run.bAchievementEligibleV76);
        Item->SetNumberField(TEXT("armour_research_v63"),Run.ArmourResearchMaskV63);
        Item->SetNumberField(TEXT("armour_selection_v63"),Run.ArmourSelectionMaskV63);
        Item->SetBoolField(TEXT("armour_selection_explicit_v84413"),Run.bArmourSelectionExplicitV84413);
        Item->SetNumberField(TEXT("armour_configuration_v836"),Run.ArmourConfigurationMaskV836);
        Item->SetNumberField(TEXT("pioneer_orders_v63"),Run.PioneerOrdersMaskV63);
        Item->SetNumberField(TEXT("smoke_orders_v63"),Run.SmokeOrdersMaskV63);
        Item->SetNumberField(TEXT("armour_orders_v63"),Run.ArmourOrdersMaskV63);
        Item->SetNumberField(TEXT("requisition_paid_v836"),Run.RequisitionPaidMaskV836);
        Item->SetNumberField(TEXT("acknowledged_events"),Run.AcknowledgedEventMask);
        Item->SetBoolField(TEXT("mobile_gun_doctrine"),Run.bMobileGunV52);
        FrontlineCampaignV52::WriteArchive(*Item,Run);
        FrontlineCombatRecordV65::WriteArchive(*Item,Run.CombatRecordV65);
        Item->SetStringField(TEXT("created_utc"),Run.CreatedUtc); Item->SetStringField(TEXT("updated_utc"),Run.UpdatedUtc);
        const auto Research=MakeShared<FJsonObject>();
        Research->SetNumberField(TEXT("research_points"),Run.Research.ResearchPoints);
        Research->SetNumberField(TEXT("technology_mask"),Run.Research.TechnologyMask);
        Research->SetNumberField(TEXT("victories"),Run.Research.Victories);
        Research->SetNumberField(TEXT("defeats"),Run.Research.Defeats);
        Item->SetObjectField(TEXT("research"),Research);
        Runs.Add(MakeShared<FJsonValueObject>(Item));
    }
    Root->SetArrayField(TEXT("campaigns"),Runs);
    Root->SetStringField(TEXT("active_campaign"),ActiveCampaignId.IsValid()?ActiveCampaignId.ToString(EGuidFormats::Digits):FString());
    FString Json;const bool Serialized=FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
    // Reject before creating pending/backup files. Never write a profile which
    // our own bounded loader would reject, and never discard ledger entries.
    if(!Serialized||!ProgressionBytesV86(ProgressionUTF8BytesV86(Json)))
    {ProgressionSaveStatus=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?TEXT("档案数据超过容量限制，原档案已保留。"):TEXT("Archive data exceeds the storage limit. The original record has been preserved.");UE_LOG(LogTemp,Warning,TEXT("V86 progression serialization/capacity check failed; existing profile retained."));return false;}
    const FString Pending=ProgressionFile+TEXT(".pending");
    const FString Backup=ProgressionFile+TEXT(".backup");
    const bool HadProfile=IFileManager::Get().FileExists(*ProgressionFile);
    const bool Prepared=IFileManager::Get().MakeDirectory(*FPaths::GetPath(ProgressionFile),true)
        && FFileHelper::SaveStringToFile(Json,*Pending,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && (!HadProfile || IFileManager::Get().Copy(*Backup,*ProgressionFile,true,false)==COPY_OK);
    const bool Saved=Prepared && IFileManager::Get().Move(*ProgressionFile,*Pending,true,false,false,true);
    if (Prepared && !Saved && HadProfile && !IFileManager::Get().FileExists(*ProgressionFile))
        IFileManager::Get().Copy(*ProgressionFile,*Backup,true,false);
    ProgressionSaveStatus=Saved?TEXT("档案已保存"):TEXT("档案保存失败，请检查磁盘权限");
    if (!Saved) UE_LOG(LogTemp,Warning,TEXT("V13 progression save failed; existing profile retained where possible: %s"),*ProgressionFile);
    return Saved;
}

void AWarfareFrontlineGameMode::AwardEncounterResearch(int32 WinningTeam)
{
    if(FrontlineUltimateArchiveV8215::Active(this))
    {bEncounterRewarded=true;LastResearchReward=0;bCampaignMissionAdvanced=false;return;}
    if(WarfareFRRUCampaignV82::Active(this))return;
    if (bEncounterRewarded || !EncounterId.IsValid() || (WinningTeam!=0 && WinningTeam!=1)) return;
    const FString Receipt=EncounterId.ToString(EGuidFormats::Digits);
    if (RewardedEncounterIds.Contains(Receipt)) { bEncounterRewarded=true; return; }
    bEncounterRewarded=true;
    // Restart/abandon never calls this path. A short loss or an untouched match
    // cannot be repeatedly converted into research points.
    const bool MeaningfulBattle=Elapsed>=45.f && ShotsFired>=8;
    LastResearchReward=WinningTeam==0?3:(MeaningfulBattle?1:0);
    auto& Army=ActiveResearchMutable();
    Army.ResearchPoints=FMath::Min(99999,Army.ResearchPoints+LastResearchReward);
    if (WinningTeam==0) Army.Victories=FMath::Min(999999,Army.Victories+1);
    else Army.Defeats=FMath::Min(999999,Army.Defeats+1);
    bCampaignMissionAdvanced=false;
    if (BoundCampaignEncounterId==EncounterId && BoundCampaignId.IsValid())
    {
        auto* Run=CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& Item){ return Item.Id==BoundCampaignId; });
        if (Run && Run->Faction==PlayerFaction && GetCampaignMissionsForFaction(Run->Faction).IsValidIndex(BoundCampaignMission))
        {
            Run->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
            Run->CombatRecordV65.Add(CombatRecordV65[0]);
            FrontlineReportV839::CaptureCampaignBattleV86(Run->CombatRecordV65,this,BoundCampaignMission,WinningTeam);
            RecordedOutcomesV8217.Add(this,FRecordedOutcomeV8217{EncounterId,Run->Id,CombatRecordV65[0]});
            Run->CombatRecordV65.BattlesRecorded=FMath::Min(999999,Run->CombatRecordV65.BattlesRecorded+1);
            if (WinningTeam==0 && FrontlineCampaignV52::RecordVictory(*Run,BoundCampaignMission))
            {
                bCampaignMissionAdvanced=true;
                FrontlineNewUnlockV8215::Capture(this,BoundCampaignMission);
            }
        }
    }
    RewardedEncounterIds.Add(Receipt);
    while (RewardedEncounterIds.Num()>128) RewardedEncounterIds.RemoveAt(0);
    const bool OutcomeSaved=SaveProgression();
    if(auto* Snapshot=RecordedOutcomesV8217.Find(this);Snapshot&&Snapshot->Encounter==EncounterId)Snapshot->Persisted=OutcomeSaved;
    FrontlineAchievementsV76::LegacyOutcomePersistedV83(this,WinningTeam,OutcomeSaved);
    UE_LOG(LogTemp,Display,TEXT("V13 research receipt=%s faction=%d winner=%d elapsed=%.3f shots=%d award=%d total=%d"),
        *Receipt,int32(PlayerFaction),WinningTeam,Elapsed,ShotsFired,LastResearchReward,Army.ResearchPoints);
}

bool AWarfareFrontlineGameMode::FinalizeCampaignCombatRecordV8217()
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::FinalizeCampaignCombatRecordV87(this);
 auto* Snapshot=RecordedOutcomesV8217.Find(this);
 if(!Snapshot||Snapshot->Encounter!=EncounterId||WarfareFRRUCampaignV82::Active(this)||FrontlineUltimateArchiveV8215::Active(this)||!bEncounterRewarded||(Phase!=EFrontlinePhase::Victory&&Phase!=EFrontlinePhase::Defeat))return true;
 auto* Run=CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& R){return R.Id==Snapshot->Campaign;});if(!Run)return false;
 const auto Delta=CombatTailV8217(CombatRecordV65[0],Snapshot->Captured);
 const auto Before=Run->CombatRecordV65;const FString BeforeUpdated=Run->UpdatedUtc;
 const bool ReturnChanged=FrontlineReportV839::CaptureCampaignBattleV86(Run->CombatRecordV65,this,BoundCampaignMission,Phase==EFrontlinePhase::Victory?0:1);
 if(!Delta.Total.Total()&&!Delta.InfantryLost&&!Delta.ArmourLost&&!Delta.DetailsV839.HasCounts()&&!ReturnChanged&&Snapshot->Persisted)return true;
 Run->CombatRecordV65.Add(Delta);Run->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
 if(!SaveProgression()){Run->CombatRecordV65=Before;Run->UpdatedUtc=BeforeUpdated;return false;}
 Snapshot->Captured=CombatRecordV65[0];Snapshot->Persisted=true;
 FrontlineAchievementsV76::LegacyOutcomePersistedV83(this,Phase==EFrontlinePhase::Victory?0:1,true);return true;
}

bool FrontlineProgressionReviewV8219::InitialOutcomePending(const AWarfareFrontlineGameMode* B)
{
 const auto* Snapshot=B?RecordedOutcomesV8217.Find(const_cast<AWarfareFrontlineGameMode*>(B)):nullptr;
 return Snapshot&&Snapshot->Encounter==B->GetEncounterId()&&!Snapshot->Persisted;
}

void FrontlineProgressionReviewV8219::RunCapacityChecksV86(TFunctionRef<void(const FString&,bool)> Check)
{
 Check(TEXT("v86_profile_read_write_share_exact_64mib_limit"),ProgressionMaxBytesV86==64ll*1024*1024&&ProgressionBytesV86(ProgressionMaxBytesV86)&&!ProgressionBytesV86(ProgressionMaxBytesV86+1)&&!ProgressionBytesV86(-1));
 FString Sample=FString::ChrN(1024*1024+1,TEXT('x'));Sample+=TEXT("汉");const int64 Encoded=ProgressionUTF8BytesV86(Sample);
 Check(TEXT("v86_profile_over_old_1mib_limit_is_allowed_and_uses_utf8_bytes"),Encoded==1024*1024+4&&ProgressionBytesV86(Encoded)&&Encoded>Sample.Len());
 const auto Codec=MakeShared<FJsonObject>();Codec->SetStringField(TEXT("label"),TEXT("汉"));FString Json;const bool Serialized=FJsonSerializer::Serialize(Codec,TJsonWriterFactory<>::Create(&Json));
 Check(TEXT("v86_profile_capacity_check_is_read_only_serialized_text"),Serialized&&ProgressionBytesV86(ProgressionUTF8BytesV86(Json))&&Json.Contains(TEXT("label")));
}

void AWarfareFrontlineGameMode::BeginProgressionVerification()
{
    // Explicit, isolated fixture: tests real recruitment and profile I/O but uses
    // labelled synthetic match outcomes to exercise rewards without farming the
    // player's archive. It is separate from natural combat verification.
    const auto Report=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Checks; int32 Passed=0;
    auto Check=[&](const TCHAR* Name,bool Result)
    {
        const auto Item=MakeShared<FJsonObject>(); Item->SetStringField(TEXT("name"),Name); Item->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(Item)); if (Result) ++Passed;
        UE_LOG(LogTemp,Display,TEXT("V13 progression check %s: %s"),Name,Result?TEXT("PASS"):TEXT("FAIL"));
    };
    const FString Run=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ProgressionFile=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V13"),FString::Printf(TEXT("progression-fixture-%s.json"),*Run));
    bProgressionVolatile=false; bProgressionReadOnly=false; PlayerFaction=EFrontlineFaction::British;
    ArmyProgress[0]={}; ArmyProgress[1]={}; RewardedEncounterIds.Empty(); ResetEncounter();
    Check(TEXT("starter_points_are_separate_and_prerequisites_enforced"),GetResearchPoints()==2 && !PurchaseTechnology(1));
    Check(TEXT("first_training_persists_and_duplicate_purchase_blocked"),PurchaseTechnology(0) && GetResearchPoints()==1 && HasTechnology(0) && !PurchaseTechnology(0));
    Check(TEXT("insufficient_points_do_not_mutate_training"),!PurchaseTechnology(1) && GetResearchPoints()==1 && !HasTechnology(1));
    Check(TEXT("british_accuracy_bonus_is_player_only"),FMath::IsNearlyEqual(RifleAccuracyScale(0),1.1f) && RifleAccuracyScale(1)==1.f);
    Check(TEXT("german_faction_switch_rebuilds_correct_armies"),SelectPlayerFaction(EFrontlineFaction::German)
        && GetResearchPoints()==2 && !HasTechnology(0) && GetFactionForTeam(1)==EFrontlineFaction::British
        && GetSquadSize(EFrontlineRole::Rifle,0)==6 && GetSquadSize(EFrontlineRole::Rifle,1)==6);
    bool NationalModels=true;
    for (const AWarfareFrontlineSoldier* Unit:Units)
    {
        const bool German=Unit->Team==0;
        NationalModels&=Unit->Soldier->GetSkeletalMeshAsset() && Unit->Soldier->GetSkeletalMeshAsset()->GetName().Contains(German?TEXT("German"):TEXT("British"))
            && Unit->GetMagazineCapacity()==(German?5:10);
    }
    Check(TEXT("spawned_meshes_and_rifle_magazines_follow_faction"),NationalModels);
    BeginBattle();
    Check(TEXT("mid_battle_faction_and_training_changes_blocked"),!SelectPlayerFaction(EFrontlineFaction::British) && !PurchaseTechnology(2));
    Check(TEXT("german_machinegun_can_be_recruited"),Recruit(EFrontlineRole::Lewis,0));
    bool GermanMG=false;
    for (const AWarfareFrontlineSoldier* Unit:Units) if (Unit->Team==0 && Unit->IsAutomaticWeapon())
    {
        const bool CorrectCapacity=Unit->GetMagazineCapacity()==100;
        const bool CorrectReload=GermanMGReloadAnimation && Unit->GetReloadMotion()==GermanMGReloadAnimation;
        const bool CorrectMesh=V16SoldierMeshes.IsValidIndex(7) && V16SoldierMeshes[7]
            && Unit->GetPresentationRoleIndex()==7 && Unit->Soldier->GetSkeletalMeshAsset()==V16SoldierMeshes[7];
        GermanMG=CorrectCapacity && CorrectReload && CorrectMesh;
        UE_LOG(LogTemp,Display,TEXT("WF_V16_PROGRESSION_GERMAN_MG capacity=%d reload=%d identity_mesh=%d rounds=%d mesh=%s"),
            int32(CorrectCapacity),int32(CorrectReload),int32(CorrectMesh),Unit->GetMagazineCapacity(),*GetNameSafe(Unit->Soldier->GetSkeletalMeshAsset()));
    }
    Check(TEXT("german_machinegun_has_100_rounds_and_its_own_reload"),GermanMG);
    const int32 Before=GetResearchPoints(); RestartBattle();
    Check(TEXT("abandon_does_not_award_points"),GetResearchPoints()==Before && GetLastResearchReward()==0);
    BeginBattle(); Elapsed=46; ShotsFired=8; FinishBattle(0,TEXT("V13 isolated outcome fixture"));
    const FGuid PaidId=EncounterId;
    Check(TEXT("victory_awards_three_once"),GetResearchPoints()==Before+3 && GetLastResearchReward()==3);
    AwardEncounterResearch(0); bEncounterRewarded=false; AwardEncounterResearch(0);
    Check(TEXT("duplicate_outcome_and_receipt_do_not_award_twice"),GetResearchPoints()==Before+3);
    Check(TEXT("german_machinegun_and_artillery_roots_apply_independent_bonuses"),PurchaseTechnology(2) && PurchaseTechnology(3)
        && FMath::IsNearlyEqual(MachinegunPressureScale(0),1.18f) && FMath::IsNearlyEqual(EffectiveArtillerySeconds(),SupportCooldownSeconds(EFrontlineSupport::Artillery)*.85f));
    Check(TEXT("german_independent_rifle_branch"),PurchaseTechnology(0) && GetResearchPoints()==1);
    RestartBattle(); BeginBattle(); Elapsed=46; ShotsFired=8; FinishBattle(1,TEXT("V13 isolated defeat fixture"));
    Check(TEXT("engaged_defeat_awards_one"),GetLastResearchReward()==1 && GetResearchPoints()==2);
    Check(TEXT("reload_training_uses_real_duration_modifier"),PurchaseTechnology(1) && FMath::IsNearlyEqual(WeaponReloadScale(0),.85f) && WeaponReloadScale(1)==1.f);
    const int32 AfterReload=GetResearchPoints();
    RestartBattle(); BeginBattle(); Elapsed=4; ShotsFired=0; FinishBattle(1,TEXT("V13 isolated early loss fixture"));
    Check(TEXT("short_unengaged_defeat_cannot_farm_points"),GetLastResearchReward()==0 && GetResearchPoints()==AfterReload);
    RestartBattle(); BeginBattle(); Elapsed=46; ShotsFired=8; FinishBattle(0,TEXT("V13 isolated supply fixture"));
    Check(TEXT("assault_and_supply_branch_have_actual_modifiers"),PurchaseTechnology(4) && PurchaseTechnology(5)
        && FMath::IsNearlyEqual(GrenadeHitChanceScale(0),1.12f)
        && FMath::IsNearlyEqual(EffectiveRecruitSeconds(EFrontlineRole::Lewis),RoleRecruitSeconds(EFrontlineRole::Lewis)*.90f));
    RestartBattle(); BeginBattle();
    const bool Recruited=Recruit(EFrontlineRole::Rifle); bool Shared=Recruited;
    for (int32 I=0;I<RoleCount;++I) Shared&=FMath::IsNearlyEqual(RecruitCooldown(EFrontlineRole(I)),RoleRecruitSeconds(EFrontlineRole(I))*.90f,.001f);
    Check(TEXT("supply_bonus_applies_to_all_shared_readiness_counters"),Shared);
    const bool InitialArtillery=FMath::IsNearlyEqual(ArtilleryCooldown(),EffectiveArtillerySeconds(),.001f);
    ArtilleryReadyAt[0]=Elapsed; // Isolated readiness fixture; does not change the production cooldown.
    Check(TEXT("artillery_training_applies_to_initial_and_next_preparation"),InitialArtillery && RequestArtillery(FVector::ZeroVector)
        && FMath::IsNearlyEqual(ArtilleryCooldown(),EffectiveArtillerySeconds(),.001f));
    RestartBattle(); Check(TEXT("switch_back_restores_british_training_only"),SelectPlayerFaction(EFrontlineFaction::British)
        && GetResearchPoints()==1 && HasTechnology(0) && !HasTechnology(2) && WeaponReloadScale(0)==1.f);
    SelectPlayerFaction(EFrontlineFaction::German); const uint32 SavedMask=ArmyProgress[1].TechnologyMask;
    FString Stored; TSharedPtr<FJsonObject> StoredJson;
    const bool StoredValid=FFileHelper::LoadFileToString(Stored,*ProgressionFile) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Stored),StoredJson);
    Check(TEXT("isolated_profile_file_contains_both_armies_and_receipts"),StoredValid && StoredJson->HasTypedField<EJson::Object>(TEXT("British"))
        && StoredJson->HasTypedField<EJson::Object>(TEXT("German")) && RewardedEncounterIds.Contains(PaidId.ToString(EGuidFormats::Digits)) && SavedMask==LegacyTechnologyMask);
    LoadProgression(true);
    Check(TEXT("saved_profile_reloads_faction_both_armies_and_legacy_six_nodes"),!bProgressionReadOnly && PlayerFaction==EFrontlineFaction::German
        && ArmyProgress[1].TechnologyMask==SavedMask && ArmyProgress[0].TechnologyMask==1 && ArmyProgress[0].ResearchPoints==1
        && RewardedEncounterIds.Contains(PaidId.ToString(EGuidFormats::Digits)));
    Report->SetStringField(TEXT("run_id"),Run); Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
    Report->SetBoolField(TEXT("synthetic_outcome_fixture"),true); Report->SetBoolField(TEXT("player_profile_touched"),false);
    Report->SetBoolField(TEXT("accelerated_artillery_readiness_fixture"),true);
    Report->SetStringField(TEXT("isolated_profile"),FPaths::ConvertRelativePathToFull(ProgressionFile));
    Report->SetArrayField(TEXT("checks"),Checks); Report->SetNumberField(TEXT("passed"),Passed); Report->SetNumberField(TEXT("total"),Checks.Num());
    Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());
    FString Output; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output));
    FFileHelper::SaveStringToFile(Output,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V13/progression-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FPlatformMisc::RequestExit(false);
}
