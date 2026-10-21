#pragma once
// Engine-independent content contract. No world ticks, I/O, engine singleton or
// original-game mutation. Distances are compressed gameplay centimetres.
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wfexp {
enum class Faction : std::uint8_t { British=0, German=1, French=2, Russian=3, AustroHungarian=4, Invalid=255 };
enum class Theatre : std::uint8_t { Western=0, Eastern=1 };
enum class Objective : std::uint8_t { Breakthrough, HoldCenter };
enum class Terrain : std::uint8_t { Farmland, Ridge, Marsh, Canal, RailCutting, Village };
struct Text { const char* zh; const char* en; };
struct FactionInfo { const char* id; Faction faction; bool selectable; Text title,name,identity,description; };
struct Equipment { const char* id; Faction faction; Text name,description; int fromDate,toDate; const char* kind;const char* slot;const char* artKey; };
struct VoiceCue { const char* id; Faction faction; const char* trigger;Text text;const char* nativeText; };
// Descriptors can exist before verified deployment and a host gameplay profile.
// Their presence in a registry is never permission to spawn a vehicle.
struct Vehicle { const char* id; Faction faction;Text name,description;const char* kind;int fromDate,toDate;const char* modelKey;bool assetsReady,deploymentVerified;const char* verifiedMissionIds;const char* profileId; };
struct WeaponStats {
    float fireRangeCm,advanceContactCm,hitProbabilityScale,directPressure,adjacentPressure,adjacentRadiusCm;
    float accuracyNear,accuracyFloor,accuracyFalloffCm,aimedCycleSeconds,roundIntervalSeconds;
    int burstMin,burstMax; float recoveryMin,recoveryMax;
};
struct Weapon {
    const char* id; Faction faction; Text name; int fromDate,toDate,capacity; float reloadSeconds;
    const char* kind;const char* reloadKind;
    WeaponStats stats;
};
struct Role {
    const char* id; Faction faction; int roleId; Text name,description;
    const char* weaponId; const char* escortWeaponId; int fromDate,toDate,size; float readinessSeconds;
    bool available;float costScale;
};
struct Support {
    const char* id; Faction faction; int supportId; Text name,description;
    int fromDate,toDate; float cooldownScale,costScale;
    // Authored cooldownSeconds already includes cooldownScale. Do not multiply
    // that authoring ratio a second time when resolving a national support.
    bool available;float cooldownSeconds,arrivalSeconds;
};
struct ModifierEffect { const char* key; float multiplier; };
struct Technology {
    const char* id; Faction faction; int legacyId,cost; Text name,description;
    const char* prerequisiteIds; const char* effectKey; float multiplier;
    const ModifierEffect* tradeoffs;std::size_t tradeoffCount;
};
struct Mission {
    const char* id; Faction faction,enemy; int order,date,seed; Text title,location,briefing,objectiveText,history,terrainNotes;
    Theatre theatre; float longitude,latitude; Objective objective; Terrain terrain;
    float holdSeconds,playerMorale,enemyMorale,enemyRecruitScale,enemyAssaultDelay;
    float playerArtilleryDelay,enemyArtilleryDelay,rainBase,rainVariation,wetness,visibilityScale,mudMoveScale;
    float wireDensity,mineDensity,battlefieldHalfWidth;
    std::uint32_t roleMask,supportMask; const char* armourIds; const char* enemyRoleOrder; const char* enemyOpeningRoles;
};
struct Choice {
    const char* id; Text title,description; int researchPoints; const char* loanRoleId;
};
struct Dispatch {
    const char* id; Faction faction; const char* afterMissionId; Text title,body;
    const Choice* choices; std::size_t choiceCount;
};
template<class T> struct View {
    const T* data=nullptr; std::size_t size=0;
    const T* begin() const { return data; }
    const T* end() const { return size?data+size:data; }
};
View<Weapon> Weapons(); View<Role> Roles(); View<Support> Supports();
View<FactionInfo> Factions(); View<Equipment> EquipmentItems();View<VoiceCue> VoiceCues();View<Vehicle> Vehicles();
View<Technology> Technologies(); View<Mission> Missions(); View<Dispatch> Dispatches();
Faction ParseFaction(std::string_view id);
const char* FactionId(Faction faction);
bool IsSelectable(Faction faction);
const Weapon* FindWeapon(std::string_view id);
const Role* FindRole(std::string_view id);
const Support* FindSupport(std::string_view id);
const Technology* FindTechnology(std::string_view id);
const Mission* FindMission(std::string_view id);
const Dispatch* FindDispatch(std::string_view id);
const Equipment* FindEquipment(std::string_view id);const Vehicle* FindVehicle(std::string_view id);
// Requiring an explicit national identity prevents a Team==0/1 shortcut.
const Role* RoleFor(Faction faction,int roleId,int date);
const Weapon* WeaponFor(Faction faction,int roleId,int memberIndex,int date);
const Support* SupportFor(Faction faction,int supportId,int date);
bool RoleAllowed(const Mission& mission,int roleId);
bool SupportAllowed(const Mission& mission,int supportId);
bool ArmourAllowed(const Mission& mission,std::string_view armourId);

struct Archive {
    int schemaVersion=1,contentRevision=1;
    std::string packId="wf.fr-ru",factionId,selectedMissionId;
    int researchPoints=2,victories=0,defeats=0;
    std::uint32_t technologyMask=0;
    std::vector<std::string> wonMissionIds,boughtTechnologyIds,resolvedDispatchIds;
    // Exactly one choice per dispatch; choices are persisted so a failed write
    // cannot be followed by applying a different reward from the same order.
    std::vector<std::string> choiceIds;
    // consumedLoanDispatchIds includes explicitly released/expired attachments.
    std::vector<std::string> defeatedEncounterIds,resolvedEncounterIds,consumedLoanDispatchIds;
    std::string pendingLoanRoleId,pendingLoanDispatchId,pendingLoanMissionId;
    std::string activeEncounterId,activeEncounterMissionId;
};
struct Result { bool ok=false; std::string reason; explicit operator bool() const { return ok; } };
Result ValidateArchive(const Archive& archive);
Result NewCampaign(Faction faction,Archive& output);
bool CanStart(const Archive& archive,std::string_view missionId);
Result SelectMission(Archive& archive,std::string_view missionId);
Result BuyTechnology(Archive& archive,std::string_view technologyId);
Result BeginEncounter(Archive& archive,std::string_view missionId,std::string_view encounterId);
// Cancel/recover an interrupted encounter without rewards or a fictitious loss.
// Retire its token so late callbacks cannot affect a restarted battle.
Result AbortEncounter(Archive& archive);
Result RecordVictory(Archive& archive,std::string_view missionId,std::string_view encounterId);
Result RecordDefeat(Archive& archive,std::string_view missionId,std::string_view encounterId);
Result ResolveDispatch(Archive& archive,std::string_view dispatchId,std::string_view choiceId);
// Loan is reserved/consumed only after the host has confirmed legal readiness,
// capacity and the normal physical entry path. No spawning occurs in this pack.
Result ConsumeLoan(Archive& archive,std::string_view roleId);
// Explicitly closes an unused attachment so it cannot block later orders.
Result ReleaseLoan(Archive& archive);
// No second combat scheduler: host delegates each call once. Probability is
// bounded; successful host hit remains a single fatal event, never fractional HP.
float FirearmHitChance(float baseProbability,bool automatic);
class BattleModifiers {
public:
    float Scale(std::string_view key) const;
private:
    std::vector<std::pair<std::string,float>> values;
    friend Result BuildBattleModifiers(const Archive&,BattleModifiers&);
};
// Validate once at encounter preparation, then use this immutable snapshot in
// the host's existing calculations. Never validate a whole save per shot/tick.
Result BuildBattleModifiers(const Archive& archive,BattleModifiers& output);
float RecruitmentSeconds(const Role& role,const BattleModifiers& modifiers);
float ReloadSeconds(const Weapon& weapon,const BattleModifiers& modifiers);
float SupportCooldownSeconds(const Support& support,const BattleModifiers& modifiers);
float SupportArrivalSeconds(const Support& support,const BattleModifiers& modifiers);

struct AIContext {
    Faction faction=Faction::Invalid; int date=0; const Mission* mission=nullptr;
    bool defending=false,enemyArmour=false,enemyEntrenched=false,needsClearance=false;
    bool hasMachinegun=false,hasPioneer=false,hasCommand=false;
    int occupiedSlots=0,capacity=3;
};
// Host retains recruitment clock, economy, unlocks, cap, target selection,
// navigation and casualty logic; this merely ranks legal existing roles.
std::vector<int> RankRecruitment(const AIContext& context);
struct HostCapabilities {
    int contractVersion=0;
    bool explicitEnemyFaction=false,equipmentResolver=false,dateGates=false,namespaceArchives=false;
    bool eraSafeEnemyRoster=false,explicitTheatre=false,assetsReady=false,oneCombatScheduler=false;
    bool explicitMissionIdentity=false,allModifierKeysSupported=false;
};
Result CheckHost(const HostCapabilities& host);
const char* CatalogDigest();
}
