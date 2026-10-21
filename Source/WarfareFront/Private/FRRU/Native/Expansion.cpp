#include "Expansion.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include "Catalog.generated.inl"

namespace wfexp {
namespace {
template<class T> const T* find(View<T> rows,std::string_view id) {
    for (const auto& row:rows) if (row.id==id) return &row;
    return nullptr;
}
bool contains(const std::vector<std::string>& rows,std::string_view id) {
    return std::find(rows.begin(),rows.end(),id)!=rows.end();
}
bool listed(std::string_view list,std::string_view id) {
    if (id.empty()) return false;
    while (!list.empty()) {
        const auto end=list.find('|');
        if (list.substr(0,end)==id) return true;
        if (end==std::string_view::npos) break;
        list.remove_prefix(end+1);
    }
    return false;
}
bool inEra(int date,int from,int to) { return date>=from && date<=to; }
bool validDate(int date) {
    const int year=date/10000,month=(date/100)%100,day=date%100;
    if (year<1914 || year>1918 || month<1 || month>12 || day<1) return false;
    const int days[]={31,28+(year%4==0?1:0),31,30,31,30,31,31,30,31,30,31};
    return day<=days[month-1];
}
bool validToken(std::string_view id) {
    return !id.empty() && id.size()<=64 && id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")==std::string::npos;
}
Result fail(const char* reason) { return {false,reason}; }
Result success() { return {true,""}; }
bool unique(const std::vector<std::string>& ids) {
    std::unordered_set<std::string> seen;
    for (const auto& id:ids) if (!seen.insert(id).second) return false;
    return true;
}
void closeLoan(Archive& a) {
    if (!a.pendingLoanDispatchId.empty()) a.consumedLoanDispatchIds.push_back(a.pendingLoanDispatchId);
    a.pendingLoanRoleId.clear(); a.pendingLoanDispatchId.clear(); a.pendingLoanMissionId.clear();
}
}
View<Weapon> Weapons() { return {generated::weapons,sizeof(generated::weapons)/sizeof(Weapon)}; }
View<Role> Roles() { return {generated::roles,sizeof(generated::roles)/sizeof(Role)}; }
View<Support> Supports() { return {generated::supports,sizeof(generated::supports)/sizeof(Support)}; }
View<Technology> Technologies() { return {generated::technologies,sizeof(generated::technologies)/sizeof(Technology)}; }
View<Mission> Missions() { return {generated::missions,sizeof(generated::missions)/sizeof(Mission)}; }
View<Dispatch> Dispatches() { return {generated::dispatches,sizeof(generated::dispatches)/sizeof(Dispatch)}; }
View<FactionInfo> Factions() { return {generated::factions,sizeof(generated::factions)/sizeof(FactionInfo)}; }
View<Equipment> EquipmentItems() { return {generated::equipment,sizeof(generated::equipment)/sizeof(Equipment)}; }
View<VoiceCue> VoiceCues() { return {generated::voices,sizeof(generated::voices)/sizeof(VoiceCue)}; }
View<Vehicle> Vehicles() { return {generated::vehicles,sizeof(generated::vehicles)/sizeof(Vehicle)}; }
const char* CatalogDigest() { return generated::digest; }
Faction ParseFaction(std::string_view id) {
    if (id=="uk" || id=="British") return Faction::British;
    if (id=="de" || id=="German") return Faction::German;
    if (id=="fr") return Faction::French;
    if (id=="ru") return Faction::Russian;
    if (id=="at-hu") return Faction::AustroHungarian;
    return Faction::Invalid;
}
const char* FactionId(Faction faction) {
    switch (faction) {
    case Faction::British:return "uk"; case Faction::German:return "de";
    case Faction::French:return "fr"; case Faction::Russian:return "ru";
    case Faction::AustroHungarian:return "at-hu"; default:return "";
    }
}
bool IsSelectable(Faction faction) { return faction==Faction::French || faction==Faction::Russian; }
const Weapon* FindWeapon(std::string_view id) { return find(Weapons(),id); }
const Role* FindRole(std::string_view id) { return find(Roles(),id); }
const Support* FindSupport(std::string_view id) { return find(Supports(),id); }
const Technology* FindTechnology(std::string_view id) { return find(Technologies(),id); }
const Mission* FindMission(std::string_view id) { return find(Missions(),id); }
const Dispatch* FindDispatch(std::string_view id) { return find(Dispatches(),id); }
const Equipment* FindEquipment(std::string_view id) { return find(EquipmentItems(),id); }
const Vehicle* FindVehicle(std::string_view id) { return find(Vehicles(),id); }
const Role* RoleFor(Faction faction,int roleId,int date) {
    if (roleId<0 || roleId>=11 || !validDate(date)) return nullptr;
    for (const auto& role:Roles()) if (role.faction==faction && role.roleId==roleId && role.available && inEra(date,role.fromDate,role.toDate)) return &role;
    return nullptr;
}
const Weapon* WeaponFor(Faction faction,int roleId,int memberIndex,int date) {
    const auto* role=RoleFor(faction,roleId,date);
    if (!role || memberIndex<0 || memberIndex>=role->size) return nullptr;
    const auto* weapon=FindWeapon(memberIndex==0?role->weaponId:role->escortWeaponId);
    return weapon && weapon->faction==faction && inEra(date,weapon->fromDate,weapon->toDate)?weapon:nullptr;
}
const Support* SupportFor(Faction faction,int supportId,int date) {
    if (supportId<0 || supportId>=10 || !validDate(date)) return nullptr;
    for (const auto& support:Supports()) if (support.faction==faction && support.supportId==supportId && support.available && inEra(date,support.fromDate,support.toDate)) return &support;
    return nullptr;
}
bool RoleAllowed(const Mission& mission,int roleId) {
    return roleId>=0 && roleId<11 && (mission.roleMask&(1u<<roleId)) && RoleFor(mission.faction,roleId,mission.date);
}
bool SupportAllowed(const Mission& mission,int supportId) {
    return supportId>=0 && supportId<10 && (mission.supportMask&(1u<<supportId)) && SupportFor(mission.faction,supportId,mission.date);
}
bool ArmourAllowed(const Mission& mission,std::string_view armourId) {
    // Russia's campaign never inherits a German/British tank fallback.
    const auto* vehicle=FindVehicle(armourId);
    return mission.faction==Faction::French && vehicle && vehicle->faction==mission.faction &&
        validDate(mission.date) && inEra(mission.date,vehicle->fromDate,vehicle->toDate) &&
        vehicle->assetsReady && vehicle->deploymentVerified && vehicle->profileId[0] &&
        listed(vehicle->verifiedMissionIds,mission.id) && listed(mission.armourIds,armourId);
}
Result ValidateArchive(const Archive& a) {
    if (a.schemaVersion!=1 || a.contentRevision!=1 || a.packId!="wf.fr-ru") return fail("Unsupported expansion archive version");
    const auto faction=ParseFaction(a.factionId);
    if (!IsSelectable(faction)) return fail("Archive faction is not selectable");
    if (a.researchPoints<0 || a.researchPoints>100000 || a.victories<0 || a.defeats<0 || a.victories>100000 || a.defeats>100000) return fail("Archive counters out of bounds");
    if (a.wonMissionIds.size()>32 || a.boughtTechnologyIds.size()>28 || a.resolvedDispatchIds.size()>32 || a.choiceIds.size()!=a.resolvedDispatchIds.size()) return fail("Archive collection limit exceeded");
    if (a.defeatedEncounterIds.size()>100000 || a.resolvedEncounterIds.size()>200000 || a.consumedLoanDispatchIds.size()>32) return fail("Archive receipt limit exceeded");
    if (!unique(a.wonMissionIds) || !unique(a.boughtTechnologyIds) || !unique(a.resolvedDispatchIds) || !unique(a.choiceIds) || !unique(a.defeatedEncounterIds) || !unique(a.resolvedEncounterIds) || !unique(a.consumedLoanDispatchIds)) return fail("Duplicate archive IDs");
    if (a.defeatedEncounterIds.size()>100000 || a.defeats!=static_cast<int>(a.defeatedEncounterIds.size())) return fail("Defeat count differs from unique encounter events");
    const std::unordered_set<std::string> receipts(a.resolvedEncounterIds.begin(),a.resolvedEncounterIds.end());
    for (const auto& id:a.defeatedEncounterIds) {
        if (!validToken(id) || receipts.find(id)==receipts.end()) return fail("Invalid defeat encounter token");
    }
    if (a.resolvedEncounterIds.size()>200000 || a.resolvedEncounterIds.size()<a.defeatedEncounterIds.size()+a.wonMissionIds.size()) return fail("Outcome receipt count is inconsistent");
    for (const auto& id:a.resolvedEncounterIds) if (!validToken(id)) return fail("Invalid outcome receipt token");
    if (a.consumedLoanDispatchIds.size()>32) return fail("Too many consumed loans");
    for (const auto& id:a.wonMissionIds) {
        const auto* mission=FindMission(id);
        if (!mission || mission->faction!=faction) return fail("Unknown or cross-faction victory");
        // Campaign frontier is a chronological prefix, never fabricated wins.
        for (const auto& prior:Missions()) if (prior.faction==faction && prior.order<mission->order && !contains(a.wonMissionIds,prior.id)) return fail("Victory skips a required operation");
    }
    if (a.victories!=static_cast<int>(a.wonMissionIds.size())) return fail("Victory count differs from unique operations");
    std::uint32_t mask=0;
    for (const auto& id:a.boughtTechnologyIds) {
        const auto* tech=FindTechnology(id);
        if (!tech || tech->faction!=faction || tech->legacyId<0 || tech->legacyId>=28) return fail("Unknown or cross-faction technology");
        mask|=1u<<tech->legacyId;
        for (const auto& prior:Technologies()) if (listed(tech->prerequisiteIds,prior.id) && !contains(a.boughtTechnologyIds,prior.id)) return fail("Technology prerequisite missing");
    }
    if (a.technologyMask!=mask) return fail("Technology mask differs from stable IDs");
    const auto* selected=FindMission(a.selectedMissionId);
    if (!selected || selected->faction!=faction) return fail("Unknown or cross-faction selection");
    for (const auto& prior:Missions()) if (prior.faction==faction && prior.order<selected->order && !contains(a.wonMissionIds,prior.id)) return fail("Selected operation is locked");
    if (a.activeEncounterId.empty()!=a.activeEncounterMissionId.empty()) return fail("Partial active encounter");
    if (!a.activeEncounterId.empty() && (!validToken(a.activeEncounterId) || a.activeEncounterMissionId!=a.selectedMissionId || contains(a.resolvedEncounterIds,a.activeEncounterId))) return fail("Invalid or already resolved active encounter");
    for (std::size_t i=0;i<a.resolvedDispatchIds.size();++i) {
        const auto* dispatch=FindDispatch(a.resolvedDispatchIds[i]);
        if (!dispatch || dispatch->faction!=faction || !contains(a.wonMissionIds,dispatch->afterMissionId)) return fail("Dispatch has not been earned");
        bool valid=false;
        for (std::size_t j=0;j<dispatch->choiceCount;++j) if (dispatch->choices[j].id==a.choiceIds[i]) {
            valid=true;
            if (dispatch->choices[j].loanRoleId[0]) {
                const bool pending=a.pendingLoanDispatchId==dispatch->id;
                const bool closed=contains(a.consumedLoanDispatchIds,dispatch->id);
                if (pending==closed) return fail("Granted loan must be exactly pending or closed");
            }
        }
        if (!valid) return fail("Dispatch choice mismatch");
    }
    for (const auto& id:a.consumedLoanDispatchIds) {
        const auto at=std::find(a.resolvedDispatchIds.begin(),a.resolvedDispatchIds.end(),id);
        if (at==a.resolvedDispatchIds.end()) return fail("Consumed loan dispatch was not resolved");
        const auto* d=FindDispatch(id); const auto choiceIndex=static_cast<std::size_t>(at-a.resolvedDispatchIds.begin());
        bool loan=false;
        for (std::size_t j=0;j<d->choiceCount;++j) if (d->choices[j].id==a.choiceIds[choiceIndex] && d->choices[j].loanRoleId[0]) loan=true;
        if (!loan) return fail("Consumed dispatch did not grant a loan");
    }
    if (a.pendingLoanRoleId.empty()!=a.pendingLoanDispatchId.empty() || a.pendingLoanRoleId.empty()!=a.pendingLoanMissionId.empty()) return fail("Partial pending loan");
    if (!a.pendingLoanRoleId.empty()) {
        const auto* role=FindRole(a.pendingLoanRoleId);
        if (!role || role->faction!=faction || !role->available) return fail("Invalid loan role");
        bool earned=false;
        for (std::size_t i=0;i<a.resolvedDispatchIds.size();++i) {
            const auto* dispatch=FindDispatch(a.resolvedDispatchIds[i]);
            for (std::size_t j=0;j<dispatch->choiceCount;++j) if (dispatch->id==a.pendingLoanDispatchId && !contains(a.consumedLoanDispatchIds,dispatch->id) && dispatch->choices[j].id==a.choiceIds[i] && dispatch->choices[j].loanRoleId==a.pendingLoanRoleId) earned=true;
        }
        if (!earned) return fail("Loan was not granted by a resolved dispatch");
        const auto* loanMission=FindMission(a.pendingLoanMissionId);
        const auto* loanDispatch=FindDispatch(a.pendingLoanDispatchId);
        const auto* after=loanDispatch?FindMission(loanDispatch->afterMissionId):nullptr;
        if (!loanMission || !after || loanMission->faction!=faction || loanMission->order!=after->order+1 || !RoleAllowed(*loanMission,role->roleId) || contains(a.wonMissionIds,loanMission->id)) return fail("Loan is not bound to the next valid operation");
    }
    return success();
}
Result NewCampaign(Faction faction,Archive& output) {
    if (!IsSelectable(faction)) return fail("This faction cannot start an expansion campaign");
    Archive candidate; candidate.factionId=FactionId(faction);
    for (const auto& mission:Missions()) if (mission.faction==faction && mission.order==0) candidate.selectedMissionId=mission.id;
    const auto result=ValidateArchive(candidate); if (result) output=std::move(candidate); return result;
}
bool CanStart(const Archive& a,std::string_view missionId) {
    if (!ValidateArchive(a)) return false;
    const auto* m=FindMission(missionId);
    if (!m || m->faction!=ParseFaction(a.factionId)) return false;
    for (const auto& prior:Missions()) if (prior.faction==m->faction && prior.order<m->order && !contains(a.wonMissionIds,prior.id)) return false;
    return true;
}
Result SelectMission(Archive& a,std::string_view id) {
    if (!a.activeEncounterId.empty()) return fail("Cannot switch operation during an active encounter");
    if (!CanStart(a,id)) return fail("Operation is locked or belongs to another faction");
    a.selectedMissionId=id; return success();
}
Result BuyTechnology(Archive& a,std::string_view id) {
    const auto valid=ValidateArchive(a); if (!valid) return valid;
    if (!a.activeEncounterId.empty()) return fail("Research cannot change during an active encounter");
    const auto* tech=FindTechnology(id);
    if (!tech || tech->faction!=ParseFaction(a.factionId)) return fail("Invalid technology");
    if (contains(a.boughtTechnologyIds,id)) return fail("Technology already purchased");
    if (a.researchPoints<tech->cost) return fail("Insufficient research points");
    for (const auto& prior:Technologies()) if (listed(tech->prerequisiteIds,prior.id) && !contains(a.boughtTechnologyIds,prior.id)) return fail("Technology prerequisite missing");
    Archive candidate=a; candidate.researchPoints-=tech->cost; candidate.boughtTechnologyIds.emplace_back(id); candidate.technologyMask|=1u<<tech->legacyId;
    const auto result=ValidateArchive(candidate); if (result) a=std::move(candidate); return result;
}
Result BeginEncounter(Archive& a,std::string_view id,std::string_view encounterId) {
    if (!CanStart(a,id) || a.selectedMissionId!=id || !a.activeEncounterId.empty() || !validToken(encounterId) || contains(a.resolvedEncounterIds,encounterId) || a.resolvedEncounterIds.size()>=200000) return fail("Invalid, active, full or previously resolved encounter");
    a.activeEncounterId=encounterId;a.activeEncounterMissionId=id;return success();
}
Result AbortEncounter(Archive& a) {
    const auto valid=ValidateArchive(a);if (!valid)return valid;
    if (a.activeEncounterId.empty() || a.resolvedEncounterIds.size()>=200000)return fail("No active encounter or receipt limit reached");
    a.resolvedEncounterIds.push_back(a.activeEncounterId);a.activeEncounterId.clear();a.activeEncounterMissionId.clear();return success();
}
Result RecordVictory(Archive& a,std::string_view id,std::string_view encounterId) {
    if (!CanStart(a,id) || a.selectedMissionId!=id || a.activeEncounterId!=encounterId || a.activeEncounterMissionId!=id || encounterId.empty()) return fail("Victory does not match the active encounter");
    if (contains(a.wonMissionIds,id)) {
        if (a.resolvedEncounterIds.size()>=200000) return fail("Outcome receipt limit reached");
        a.resolvedEncounterIds.emplace_back(encounterId);a.activeEncounterId.clear();a.activeEncounterMissionId.clear();return success();
    }
    if (a.researchPoints>99998) return fail("Research counter would overflow");
    Archive candidate=a; candidate.wonMissionIds.emplace_back(id); ++candidate.victories; candidate.researchPoints+=2;
    candidate.resolvedEncounterIds.emplace_back(encounterId);candidate.activeEncounterId.clear();candidate.activeEncounterMissionId.clear();
    if (candidate.pendingLoanMissionId==id) closeLoan(candidate);
    const auto* m=FindMission(id);
    for (const auto& next:Missions()) if (next.faction==m->faction && next.order==m->order+1) candidate.selectedMissionId=next.id;
    const auto result=ValidateArchive(candidate); if (result) a=std::move(candidate); return result;
}
Result RecordDefeat(Archive& a,std::string_view id,std::string_view encounterId) {
    if (!CanStart(a,id) || a.selectedMissionId!=id || a.defeats>=100000 || a.activeEncounterId!=encounterId || a.activeEncounterMissionId!=id || encounterId.empty()) return fail("Invalid or duplicate defeat event");
    Archive candidate=a; ++candidate.defeats; candidate.defeatedEncounterIds.emplace_back(encounterId);
    candidate.resolvedEncounterIds.emplace_back(encounterId);candidate.activeEncounterId.clear();candidate.activeEncounterMissionId.clear();
    if (candidate.pendingLoanMissionId==id) closeLoan(candidate);
    const auto result=ValidateArchive(candidate); if (result) a=std::move(candidate); return result;
}
Result ResolveDispatch(Archive& a,std::string_view dispatchId,std::string_view choiceId) {
    const auto valid=ValidateArchive(a); if (!valid) return valid;
    if (!a.activeEncounterId.empty()) return fail("Dispatch cannot change during an active encounter");
    const auto* d=FindDispatch(dispatchId);
    if (!d || d->faction!=ParseFaction(a.factionId) || !contains(a.wonMissionIds,d->afterMissionId)) return fail("Dispatch is not earned");
    if (contains(a.resolvedDispatchIds,dispatchId)) return fail("Dispatch already resolved");
    const Choice* choice=nullptr;
    for (std::size_t i=0;i<d->choiceCount;++i) if (d->choices[i].id==choiceId) choice=&d->choices[i];
    if (!choice) return fail("Choice does not belong to dispatch");
    if (a.researchPoints>100000-choice->researchPoints) return fail("Research counter would overflow");
    if (choice->loanRoleId[0] && !a.pendingLoanRoleId.empty()) return fail("Resolve or consume the existing loan before accepting another");
    Archive candidate=a; candidate.researchPoints+=choice->researchPoints; candidate.resolvedDispatchIds.emplace_back(dispatchId); candidate.choiceIds.emplace_back(choiceId);
    if (choice->loanRoleId[0]) {
        const auto* after=FindMission(d->afterMissionId);const Mission* loanMission=nullptr;
        for (const auto& m:Missions()) if (m.faction==d->faction && after && m.order==after->order+1) loanMission=&m;
        const auto* role=FindRole(choice->loanRoleId);
        if (!loanMission || contains(a.wonMissionIds,loanMission->id) || !role || !RoleAllowed(*loanMission,role->roleId)) return fail("Loan opportunity has expired or has no valid next operation");
        candidate.pendingLoanRoleId=choice->loanRoleId; candidate.pendingLoanDispatchId=d->id;candidate.pendingLoanMissionId=loanMission->id;
    }
    const auto result=ValidateArchive(candidate); if (result) a=std::move(candidate); return result;
}
Result ConsumeLoan(Archive& a,std::string_view id) {
    const auto valid=ValidateArchive(a); if (!valid) return valid;
    const auto* m=FindMission(a.selectedMissionId); const auto* r=FindRole(id);
    if (a.pendingLoanRoleId!=id || a.pendingLoanMissionId!=a.selectedMissionId || a.activeEncounterMissionId!=a.pendingLoanMissionId || a.activeEncounterId.empty() || !r || !m || !RoleAllowed(*m,r->roleId)) return fail("Loan is unavailable for this active operation");
    closeLoan(a);return success();
}
Result ReleaseLoan(Archive& a) {
    const auto valid=ValidateArchive(a);if (!valid)return valid;
    if (!a.activeEncounterId.empty())return fail("Loan cannot be released during an active encounter");
    if (a.pendingLoanRoleId.empty())return fail("No pending loan");
    closeLoan(a);return success();
}
float FirearmHitChance(float base,bool automatic) {
    if (!std::isfinite(base)) return 0;
    return std::clamp(base*(automatic?1.35f:1.20f),0.f,1.f);
}
float BattleModifiers::Scale(std::string_view key) const {
    for (const auto& value:values) if (value.first==key)return value.second;
    return 1.f;
}
float RecruitmentSeconds(const Role& r,const BattleModifiers& m) {
    return r.readinessSeconds*m.Scale("recruit_readiness")*(r.roleId==2?m.Scale("grenade_readiness"):1.f);
}
float ReloadSeconds(const Weapon& w,const BattleModifiers& m) { return w.reloadSeconds*m.Scale("reload_time"); }
float SupportCooldownSeconds(const Support& s,const BattleModifiers& m) {
    return s.cooldownSeconds*(s.supportId==0?m.Scale("artillery_cooldown"):s.supportId==1?m.Scale("smoke_cooldown"):1.f);
}
float SupportArrivalSeconds(const Support& s,const BattleModifiers& m) {
    return s.arrivalSeconds*((s.supportId==0 || s.supportId==3)?m.Scale("support_arrival_time"):1.f);
}
Result BuildBattleModifiers(const Archive& a,BattleModifiers& output) {
    const auto valid=ValidateArchive(a);if (!valid)return valid;
    BattleModifiers candidate;
    auto apply=[&candidate](const char* key,float multiplier) {
        auto it=std::find_if(candidate.values.begin(),candidate.values.end(),[key](const auto& row){return row.first==key;});
        if(it==candidate.values.end())candidate.values.emplace_back(key,multiplier);else it->second*=multiplier;
    };
    for (const auto& id:a.boughtTechnologyIds) {
        const auto* t=FindTechnology(id);apply(t->effectKey,t->multiplier);
        for(std::size_t i=0;i<t->tradeoffCount;++i)apply(t->tradeoffs[i].key,t->tradeoffs[i].multiplier);
    }
    for(auto& value:candidate.values)value.second=std::clamp(value.second,.5f,1.5f);
    output=std::move(candidate);return success();
}
std::vector<int> RankRecruitment(const AIContext& c) {
    std::vector<int> result;
    if (c.occupiedSlots<0 || c.capacity<=0 || c.occupiedSlots>=c.capacity || c.capacity>3) return result;
    if (c.mission && (c.faction!=c.mission->faction && c.faction!=c.mission->enemy)) return result;
    std::vector<std::pair<float,int>> scored;
    for (int r=0;r<11;++r) {
        const auto* role=RoleFor(c.faction,r,c.date); if (!role) continue;
        if (c.mission) {
            if (c.date!=c.mission->date) continue;
            if (c.faction==c.mission->faction && !RoleAllowed(*c.mission,r)) continue;
            if (c.faction==c.mission->enemy && !listed(c.mission->enemyRoleOrder,std::to_string(r))) continue;
        }
        float score=r==0?10.f:1.f;
        if (r==1 && !c.hasMachinegun) score=c.defending?16.f:12.f;
        if (r==6 && c.needsClearance && !c.hasPioneer) score=18.f;
        if (r==2 && c.enemyEntrenched && !c.defending) score=13.f;
        if (r==4 && !c.hasCommand && c.occupiedSlots>0) score=11.f;
        if (r==9 && !c.defending && !c.hasMachinegun) score=12.5f;
        if (r==3 && c.defending) score=8.f;
        // No armour grants tank rifles to Russia or an unsupported new role.
        if (r==8 && c.enemyArmour) score=15.f;
        scored.emplace_back(score,r);
    }
    std::stable_sort(scored.begin(),scored.end(),[](const auto& a,const auto& b){return a.first>b.first;});
    for (const auto& row:scored) result.push_back(row.second);
    return result;
}
Result CheckHost(const HostCapabilities& h) {
    if (h.contractVersion!=1) return fail("Expansion host contract version must be 1");
    if (!h.explicitEnemyFaction) return fail("Host still infers enemy nationality from player faction");
    if (!h.equipmentResolver) return fail("Explicit faction equipment resolver is missing");
    if (!h.dateGates) return fail("Historical date gate is missing");
    if (!h.namespaceArchives) return fail("Expansion archive namespace is missing");
    if (!h.eraSafeEnemyRoster) return fail("Enemy roster still permits out-of-era specialists");
    if (!h.explicitTheatre) return fail("Eastern theatre mapping is missing");
    if (!h.explicitMissionIdentity) return fail("Host still uses Western operation numbers to choose scene assets");
    if (!h.allModifierKeysSupported) return fail("Some technology effects or tradeoffs have no host implementation");
    if (!h.assetsReady) return fail("Required expansion art/audio assets are not accepted");
    if (!h.oneCombatScheduler) return fail("Host must retain exactly one combat/support scheduler");
    return success();
}
}
