#pragma once
#include "WarfareFrontlineProgression.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Expansion.h"

namespace wfexpue {
// Must live inside the HOST module; it references host types without a circular
// plugin dependency. Team directions are encounter sides, independent of nation.
struct FEncounter {
    FFrontlineCampaignMission Mission;
    wfexp::Faction Player=wfexp::Faction::Invalid,Enemy=wfexp::Faction::Invalid;
    wfexp::Theatre Theatre=wfexp::Theatre::Western;
    int32 Direction[2]={1,-1};
    FString StableMissionId,ArmourIds;
};
inline FFrontlineCampaignMission ConvertMission(const wfexp::Mission& Row) {
    FFrontlineCampaignMission Out;
    // Strings need durable TCHAR storage. The owner below holds every converted
    // string; never assign pointers returned by temporary UTF8_TO_TCHAR macros.
    // Reserved adapter IDs intentionally cannot collide with UK/DE 0..19.
    // A host without explicitMissionIdentity is refused by CheckHost.
    Out.Id=(Row.faction==wfexp::Faction::French?1000:2000)+Row.order;Out.Year=Row.date/10000;Out.CalendarDateV41=Row.date;
    Out.RainBase=Row.rainBase;Out.RainVariation=Row.rainVariation;Out.Wetness=Row.wetness;Out.VisibilityScale=Row.visibilityScale;Out.MudMoveScale=Row.mudMoveScale;
    Out.Objective=Row.objective==wfexp::Objective::HoldCenter?EFrontlineCampaignObjective::HoldCenter:EFrontlineCampaignObjective::Breakthrough;
    Out.HoldCenterSeconds=Row.holdSeconds;Out.PlayerMorale=Row.playerMorale;Out.EnemyMorale=Row.enemyMorale;Out.EnemyRecruitScale=Row.enemyRecruitScale;Out.EnemyAssaultDelay=Row.enemyAssaultDelay;
    Out.PlayerArtilleryDelay=Row.playerArtilleryDelay;Out.EnemyArtilleryDelay=Row.enemyArtilleryDelay;Out.RandomSeed=Row.seed;
    Out.BattlefieldHalfWidth=Row.battlefieldHalfWidth;Out.WireDensity=Row.wireDensity;Out.MineDensity=Row.mineDensity;
    Out.MapPosition=FVector2D(Row.longitude,Row.latitude);
    Out.TerrainStyleV32=static_cast<EFrontlineMissionTerrainV32>(static_cast<uint8>(Row.terrain));
    Out.DisabledRoleMask=((1u<<11)-1u)&~Row.roleMask;Out.DisabledSupportMask=((1u<<10)-1u)&~Row.supportMask;
    // Specialist firearm identities are not silently inherited from host dates.
    Out.bEnemyScopedMarksmen=false;Out.bEnemyStormtroopers=false;Out.bEnemyCommissionedOfficers=false;
    // Art/reference descriptors are not a deployment registry. All delivered
    // vehicles remain unaccepted; no mission conversion may open this gate.
    Out.bArmourAllowedV41=false;
    return Out;
}
// Non-copyable owner: the legacy mission has raw const TCHAR* fields. Move or
// resize of the storage after binding would invalidate those addresses.
class FBoundMission {
public:
    FBoundMission(const wfexp::Mission& Row,bool Chinese):Encounter{} {
        Encounter.Mission=ConvertMission(Row);Encounter.Player=Row.faction;Encounter.Enemy=Row.enemy;Encounter.Theatre=Row.theatre;
        Encounter.StableMissionId=UTF8_TO_TCHAR(Row.id);Encounter.ArmourIds=UTF8_TO_TCHAR(Row.armourIds);
        auto L=[Chinese](wfexp::Text Value){return FString(UTF8_TO_TCHAR(Chinese?Value.zh:Value.en));};
        Title=L(Row.title);Location=L(Row.location);Briefing=L(Row.briefing);ObjectiveText=L(Row.objectiveText);History=L(Row.history);TerrainNotes=L(Row.terrainNotes);
        Date=FString::Printf(TEXT("%04d-%02d-%02d"),Row.date/10000,(Row.date/100)%100,Row.date%100);
        MapKey=Encounter.StableMissionId;
        auto& M=Encounter.Mission;M.Title=*Title;M.Location=*Location;M.Briefing=*Briefing;M.ObjectiveText=*ObjectiveText;M.HistoricalContext=*History;M.TerrainNotes=*TerrainNotes;M.DateLabel=*Date;M.MapKey=*MapKey;
        FString Roles=UTF8_TO_TCHAR(Row.enemyRoleOrder);TArray<FString> Items;Roles.ParseIntoArray(Items,TEXT("|"),true);
        for(const auto& Item:Items){const int32 Role=FCString::Atoi(*Item);if(Role>=0&&Role<11)M.EnemyRoleOrder.Add(static_cast<EFrontlineRole>(Role));}
        // Preserve authored opening priorities, not a guessed first roster entry.
        FString Opening=UTF8_TO_TCHAR(Row.enemyOpeningRoles);Items.Reset();Opening.ParseIntoArray(Items,TEXT("|"),true);
        for(const auto& Item:Items){const int32 Role=FCString::Atoi(*Item);if(Role>=0&&Role<11)M.EnemyOpeningRoles.Add(static_cast<EFrontlineRole>(Role));}
    }
    FBoundMission(const FBoundMission&)=delete;FBoundMission& operator=(const FBoundMission&)=delete;
    FBoundMission(FBoundMission&&)=delete;FBoundMission& operator=(FBoundMission&&)=delete;
    const FEncounter& Get() const{return Encounter;}
private:
    FEncounter Encounter;
    FString Title,Location,Briefing,ObjectiveText,History,TerrainNotes,Date,MapKey;
};
inline bool ConvertWeapon(const wfexp::Weapon* Row,FString& OwnedId,FrontlineWeapons::FProfile& Out) {
    if(!Row)return false;OwnedId=UTF8_TO_TCHAR(Row->id);const auto& S=Row->stats;
    Out={*OwnedId,S.fireRangeCm,S.advanceContactCm,S.hitProbabilityScale,S.directPressure,S.adjacentPressure,S.adjacentRadiusCm,S.accuracyNear,S.accuracyFloor,S.accuracyFalloffCm,S.aimedCycleSeconds,S.roundIntervalSeconds,S.burstMin,S.burstMax,S.recoveryMin,S.recoveryMax};
    return true;
}
static_assert(static_cast<uint8>(EFrontlineFaction::British)==0 && static_cast<uint8>(EFrontlineFaction::German)==1,"Legacy faction values changed");
static_assert(static_cast<uint8>(EFrontlineMissionTerrainV32::Village)==static_cast<uint8>(wfexp::Terrain::Village),"Terrain enum mapping changed");
}
