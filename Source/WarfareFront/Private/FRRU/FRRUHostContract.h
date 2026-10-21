#pragma once
#include "Native/TacticalRuntime.h"
#include "Native/UE/ExpansionTacticalBridge.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace wfexp::hostv81 {
// Until actual UE art/world/Shipping checks pass, no production recruitment,
// save, order or faction path consumes this pilot.
inline constexpr bool Enabled=false;
inline constexpr tactical::Profile TechnologyMode=tactical::Profile::TacticalV2;
inline constexpr const char* IntegrationRevision="v81.frru.pilot.3";
struct Identity {
    std::string missionId,sceneId;
    std::uint32_t sceneRevision=0;
    int calendarDate=0,playerSide=-1;
    Theatre theatre=Theatre::Western;
    std::array<Faction,2> nations{{Faction::Invalid,Faction::Invalid}};
    std::array<int,2> directions{{0,0}};
};
inline Result BindIdentity(std::string_view missionId,int playerSide,int playerDirection,Identity& output) {
    const auto* base=wfexp::FindMission(missionId);
    const auto* rule=tactical::FindMission(missionId);
    const auto* ground=scene::ForMission(missionId);
    if(!base||!rule||!ground||!IsSelectable(base->faction)||(playerSide!=0&&playerSide!=1)||(playerDirection!=1&&playerDirection!=-1))return {false,"Invalid mission, selectable nation or encounter side/direction"};
    if(base->faction!=rule->player||base->enemy!=rule->enemy||base->date!=rule->date||ground->revision!=rule->sceneRevision||rule->sceneId!=ground->id)return {false,"Catalogue, tactical and physical identities disagree"};
    Identity candidate;candidate.missionId=std::string(missionId);candidate.sceneId=ground->id;candidate.sceneRevision=ground->revision;candidate.calendarDate=base->date;candidate.theatre=base->theatre;candidate.playerSide=playerSide;
    candidate.nations[playerSide]=base->faction;candidate.nations[1-playerSide]=base->enemy;candidate.directions[playerSide]=playerDirection;candidate.directions[1-playerSide]=-playerDirection;
    output=std::move(candidate);return {true,{}};
}

// These are descriptor lookups, not permission to spawn an unverified asset.
// Legacy British/German stat and asset resolvers are deliberately not called.
inline Result ResolveEquipment(const Identity& identity,int side,int roleId,int memberIndex,const Weapon*& output) {
    if(side<0||side>1||memberIndex<0)return {false,"Invalid encounter side or squad member"};
    const auto* mission=wfexp::FindMission(identity.missionId);
    if(!mission||identity.calendarDate!=mission->date||identity.playerSide<0||identity.playerSide>1||identity.nations[identity.playerSide]!=mission->faction||identity.nations[1-identity.playerSide]!=mission->enemy)return {false,"Encounter identity was altered"};
    const auto* role=RoleFor(identity.nations[side],roleId,identity.calendarDate);
    if(!role||memberIndex>=role->size||(side==identity.playerSide&&!RoleAllowed(*mission,roleId)))return {false,"National role is unavailable at this mission date"};
    const auto* weapon=WeaponFor(identity.nations[side],roleId,memberIndex,identity.calendarDate);
    if(!weapon||weapon->faction!=identity.nations[side]||identity.calendarDate<weapon->fromDate||identity.calendarDate>weapon->toDate)return {false,"No era-safe national weapon"};
    output=weapon;return {true,{}};
}

// The host has already applied its current firearm/balance multiplier. Passing
// that final probability through the pack helper again would apply x1.20/x1.35
// twice. TacticalV2 has no classic percentage modifier layer.
// Compatibility/preparation assertion only; do not validate an archive per shot.
inline Result RetainHostHitChance(const tactical::Campaign& campaign,float hostFinalProbability,float& output) {
    const auto legal=tactical::ValidateCampaign(campaign);
    if(!legal||campaign.profile!=TechnologyMode||tactical::UsesClassicModifiers(campaign))return {false,"Only the exclusive TacticalV2 profile is accepted"};
    if(!std::isfinite(hostFinalProbability)||hostFinalProbability<0||hostFinalProbability>1)return {false,"Invalid final host hit probability"};
    output=hostFinalProbability;return {true,{}};
}

// The sample comes from the existing host Elapsed; this object neither advances
// time nor reads wall time. Pause/noncombat is a successful no-op.
struct ClockSample { std::uint64_t hostFrame=0; double elapsed=0; bool fighting=false,paused=false; };
class ClockCursor {
public:
    Result RestoreCommitted(const tactical::ClockReceipt& clock) {
        if(clock.receipt.id.empty()||clock.hostFrame==0)return {true,{}};
        if(!std::isfinite(clock.capturedElapsed)||clock.capturedElapsed<0||clock.capturedElapsed>86400)return {false,"Invalid saved combat clock watermark"};
        if(bound_&&clock.hostFrame<frame_)return {true,{}};
        if(bound_&&clock.hostFrame==frame_)return clock.capturedElapsed==elapsed_?Result{true,{}}:Result{false,"Committed clock disagrees with captured frame"};
        bound_=true;frame_=clock.hostFrame;elapsed_=clock.capturedElapsed;return {true,{}};
    }
    Result Capture(const ClockSample& sample,bool& publish) {
        publish=false;
        if(!std::isfinite(sample.elapsed)||sample.elapsed<0||sample.elapsed>86400)return {false,"Invalid host combat clock"};
        if(bound_&&(sample.hostFrame<frame_||sample.elapsed<elapsed_))return {false,"Host frame or clock moved backwards without a new encounter"};
        if(bound_&&sample.hostFrame==frame_)return sample.elapsed==elapsed_?Result{true,{}}:Result{false,"Same host frame changed combat time"};
        frame_=sample.hostFrame;elapsed_=sample.elapsed;bound_=true;
        publish=sample.fighting&&!sample.paused;return {true,{}};
    }
    void Reset() { bound_=false;frame_=0;elapsed_=0; }
    double Elapsed() const { return elapsed_; }
private:
    bool bound_=false;std::uint64_t frame_=0;double elapsed_=0;
};

// One AdvanceTime receipt per captured host frame. Failed atomic preflight or
// commit leaves both reducer and cursor retryable. No timer or wall-clock read.
class HostFrameBridge {
public:
    Result Publish(tactical::State& state,tactical::Snapshot snapshot,const ClockSample& sample,tactical::AtomicHostBatch& host,bool& advanced) {
        advanced=false;
        if(state.playerSide<0||state.playerSide>1)return {false,"Invalid encounter side"};
        if(snapshot.elapsed!=sample.elapsed||snapshot.fighting!=sample.fighting||snapshot.paused!=sample.paused)return {false,"Snapshot did not use the single captured host clock"};
        auto candidate=clock_;const auto restored=candidate.RestoreCommitted(state.latestClock);if(!restored)return restored;
        bool publish=false;const auto captured=candidate.Capture(sample,publish);
        if(!captured)return captured;
        if(!publish){clock_=candidate;return {true,{}};}
        tactical::Command command;command.kind=tactical::CommandKind::AdvanceTime;command.side=state.playerSide;command.missionId=state.missionId;command.encounterId=state.encounterId;command.expectedRevision=state.revision;command.sequence=state.nextSequence[state.playerSide];command.hostFrame=sample.hostFrame;command.receiptId=std::string(IntegrationRevision)+".frame."+std::to_string(sample.hostFrame);
        const auto applied=tactical::Execute(state,snapshot,command,host);
        if(!applied)return applied;
        clock_=candidate;advanced=true;return {true,{}};
    }
    void Reset() {clock_.Reset();}
private:
    ClockCursor clock_;
};

// Registration accepts the real host plan's exact round manifest, not a count
// inferred from a timer. UE integration must call this only after atomic host
// commit succeeds, and Resolve only from actual ResolveStrike observations.
struct RoundKey {
    int salvo=-1,round=-1;
    bool operator<(const RoundKey& rhs)const {return salvo<rhs.salvo||(salvo==rhs.salvo&&round<rhs.round);}
};
struct PlannedRound {RoundKey key;double impactAt=0;};
struct HostPlan {
    std::string planId,encounterId;
    int side=-1,hostFireMissionId=-1,roundBudget=0;
    double committedAt=0;
    std::vector<PlannedRound> rounds;
};
class SupportJournal {
public:
    Result Register(const tactical::Snapshot& snapshot,const tactical::Intent& intent,const HostPlan& plan) {
        if(intent.kind!=tactical::IntentKind::RequestSupport||intent.supportId!=0||intent.salvos!=4||intent.side<0||intent.side>1)return {false,"Pilot accepts one four-salvo artillery intent"};
        if(plan.planId!=intent.id||plan.encounterId!=snapshot.encounterId||plan.side!=intent.side||plan.hostFireMissionId<0||plan.committedAt!=snapshot.elapsed||plan.roundBudget!=intent.roundBudget||plan.roundBudget!=snapshot.supportRoundBudget[intent.side][0]||snapshot.supportSalvoCount[intent.side][0]!=4||snapshot.supportReadyAt[intent.side][0]>snapshot.elapsed||snapshot.paused||!snapshot.fighting)return {false,"Registered plan differs from captured host entitlement"};
        if(plan.roundBudget<4||plan.roundBudget>10000||plan.rounds.size()!=static_cast<std::size_t>(plan.roundBudget)||!std::isfinite(plan.committedAt)||plan.committedAt<0)return {false,"Existing single-call shell budget is not closed"};
        std::set<RoundKey> keys;std::array<std::set<int>,4> rounds;
        for(const auto& round:plan.rounds) {
            if(round.key.salvo<0||round.key.salvo>=4||round.key.round<0||round.key.round>=10000||!std::isfinite(round.impactAt)||round.impactAt<intent.notBefore||!keys.insert(round.key).second)return {false,"Invalid, early or repeated shell in actual host plan"};
            rounds[round.key.salvo].insert(round.key.round);
        }
        for(const auto& group:rounds){if(group.empty())return {false,"Each registered salvo needs actual shells"};int ordinal=0;for(int value:group)if(value!=ordinal++)return {false,"Round ordinals must be contiguous within salvo"};}
        if(plans_.find(plan.planId)!=plans_.end())return {false,"Plan is already registered"};
        for(const auto& old:plans_)if(old.second.plan.encounterId==plan.encounterId&&old.second.plan.side==plan.side&&old.second.plan.hostFireMissionId==plan.hostFireMissionId)return {false,"Host fire mission already belongs to another plan"};
        Entry entry;entry.plan=plan;plans_.emplace(plan.planId,std::move(entry));return {true,{}};
    }
    Result Resolve(const std::string& planId,const std::string& encounterId,int side,int hostFireMissionId,RoundKey key,double hostElapsed,bool actualStrikeResolved,tactical::SupportReceipt& output,bool& emitted) {
        emitted=false;auto found=plans_.find(planId);if(found==plans_.end())return {false,"Unregistered plan"};auto& entry=found->second;
        if(entry.plan.encounterId!=encounterId||entry.plan.side!=side||entry.plan.hostFireMissionId!=hostFireMissionId||!actualStrikeResolved||!std::isfinite(hostElapsed)||hostElapsed<entry.plan.committedAt)return {false,"No matching actual resolved host strike"};
        const PlannedRound* expected=nullptr;for(const auto& round:entry.plan.rounds)if(round.key.salvo==key.salvo&&round.key.round==key.round){expected=&round;break;}
        if(!expected||hostElapsed<expected->impactAt)return {false,"Shell is absent or has not physically resolved"};
        // Receipt arrival may be out of order; each physical event retains its
        // original host timestamp. Duplicate observations never refresh it.
        entry.resolved.emplace(key,hostElapsed);
        double completedAt=entry.plan.committedAt;
        for(int salvo=0;salvo<4;++salvo){
            bool all=true;double at=completedAt;
            for(const auto& round:entry.plan.rounds)if(round.key.salvo==salvo){const auto resolved=entry.resolved.find(round.key);if(resolved==entry.resolved.end()){all=false;break;}at=std::max(at,resolved->second);}
            if(!all)break;completedAt=at;
            if(!entry.complete[salvo]){entry.complete[salvo]=true;entry.proofs[salvo]={entry.plan.planId,entry.plan.encounterId,entry.plan.side,salvo+1,completedAt};}
        }
        // Advisory return is retryable. Only explicit consumer acknowledgement
        // marks delivery; all active proof remains available afterwards.
        for(int salvo=0;salvo<4;++salvo)if(entry.complete[salvo]&&!entry.acknowledged[salvo]){output=entry.proofs[salvo];emitted=true;break;}
        return {true,{}};
    }
    Result PendingReceipts(const std::string& planId,const std::string& encounterId,std::vector<tactical::SupportReceipt>& output)const {
        const auto found=plans_.find(planId);if(found==plans_.end()||found->second.plan.encounterId!=encounterId)return {false,"No active matching support plan"};
        std::vector<tactical::SupportReceipt> candidate;for(int salvo=0;salvo<4;++salvo)if(found->second.complete[salvo]&&!found->second.acknowledged[salvo])candidate.push_back(found->second.proofs[salvo]);output=std::move(candidate);return {true,{}};
    }
    Result ActiveReceipts(const std::string& planId,const std::string& encounterId,std::vector<tactical::SupportReceipt>& output)const {
        const auto found=plans_.find(planId);if(found==plans_.end()||found->second.plan.encounterId!=encounterId)return {false,"No active matching support plan"};
        std::vector<tactical::SupportReceipt> candidate;for(int salvo=0;salvo<4;++salvo)if(found->second.complete[salvo])candidate.push_back(found->second.proofs[salvo]);output=std::move(candidate);return {true,{}};
    }
    Result Acknowledge(const tactical::SupportReceipt& receipt) {
        auto found=plans_.find(receipt.planId);if(found==plans_.end()||found->second.plan.encounterId!=receipt.encounterId||receipt.side!=found->second.plan.side||receipt.resolvedSalvos<1||receipt.resolvedSalvos>4)return {false,"No matching durable support receipt"};
        auto& entry=found->second;const int index=receipt.resolvedSalvos-1;if(!entry.complete[index]||receipt.resolvedAt!=entry.proofs[index].resolvedAt)return {false,"Consumer acknowledgement changed actual completion proof"};entry.acknowledged[index]=true;return {true,{}};
    }
    void RetirePlan(const std::string& planId,const std::string& encounterId){auto found=plans_.find(planId);if(found!=plans_.end()&&found->second.plan.encounterId==encounterId)plans_.erase(found);}
    void RetireEncounter(const std::string& encounterId) {for(auto it=plans_.begin();it!=plans_.end();)if(it->second.plan.encounterId==encounterId)it=plans_.erase(it);else ++it;}
    std::size_t RegisteredPlans()const {return plans_.size();}
private:
    struct Entry {HostPlan plan;std::map<RoundKey,double> resolved;std::array<bool,4> complete{},acknowledged{};std::array<tactical::SupportReceipt,4> proofs{};};
    std::map<std::string,Entry> plans_;
};
} // namespace wfexp::hostv81
