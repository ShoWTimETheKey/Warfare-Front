#pragma once
// Additive rulesVersion2. Host owns actors, movement, ammunition and all damage.
#include "Expansion.h"
#include "SceneRuntime.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace wfexp::tactical {
enum class GateKind : std::uint8_t { HoldNode, ClearPassage, TraverseNode, WithdrawToNode };
enum class CommandKind : std::uint8_t { AdvanceTime, ChooseStory, PlanFire, Observe, CommitFire, CancelFire, Followup, RegisterReserve, ReleaseReserve, CounterOrder, AdvanceCounter, UseOption, PolicyOrder, AdvanceToObjective };
enum class IntentKind : std::uint8_t { RequestSupport, AdvanceExistingSquad, SetHold, NotifyTelegraph, ClearObstacle, ObjectiveStageComplete, LocalObjectiveResult };
enum class Profile : std::uint8_t { ClassicV1, TacticalV2 };
struct NodeDefinition { std::string id; scene::Point position; double radius=0; int capacity=0; };
struct EdgeDefinition { std::string id,from,to,approach,clearanceGate; bool bidirectional=false; };
struct StageDefinition {
    std::string id; GateKind kind=GateKind::HoldNode; std::vector<std::string> nodes,alternateNodes,mustOwn;
    double duration=0,timeLimit=0,fallbackDeadline=0; int minimumLiving=1,requiredRole=-1;
    bool stationary=false,timeoutFallback=false,finalStage=false; std::string choiceReceipt,fallbackNode;
    // Receiver-authored dependency on the preceding stage's actual clear site.
    bool followClearedPassageV87=false;
};
struct StoryEffect { std::string kind,value; };
struct StoryChoice { std::string id,title,description,targetNode; std::vector<StoryEffect> effects; };
struct StoryDefinition { std::string id,stageId,title,body,defaultChoice; double triggerAfter=0,deadline=0; std::vector<StoryChoice> choices; };
struct MissionDefinition { std::string id,sceneId; std::uint32_t sceneRevision=0; Faction player=Faction::Invalid,enemy=Faction::Invalid; int date=0; std::vector<NodeDefinition> nodes; std::vector<EdgeDefinition> edges; std::vector<StageDefinition> stages; std::vector<StoryDefinition> stories; };
struct OptionDefinition { std::string id,capability,command,label,description; Faction faction=Faction::Invalid; int index=0,legacyBit=0,cost=0; std::vector<std::string> prerequisites; bool implemented=false;
    // Receiver metadata, not saved fields. Existing tactical rows retain their
    // original meaning; new passive armour effects never claim Classic bits.
    int fromDate=19140101,toDate=19181231;double primaryEffect=1.,secondaryEffect=1.;
};
const std::vector<MissionDefinition>& Missions();
const std::vector<OptionDefinition>& Options();
const MissionDefinition* FindMission(std::string_view);
const OptionDefinition* FindOption(std::string_view);
const char* RulesDigest();
const char* SceneDigest();

// The caller supplies only publicly observed contacts. Readiness, ammunition,
// date eligibility, friendly-fire safety and delayed reaction stay in the host.
enum class ObservedFireKind : std::uint8_t { Mortar, FieldArtillery, Shrapnel };
struct ObservedFireTarget {
    double importance=0,confidence=0,nearbyWeight=0;
    bool moving=false,entrenched=false,firingMachinegun=false;
};
double ObservedSupportScore(ObservedFireKind,const ObservedFireTarget&) noexcept;

struct Campaign { int schemaVersion=2,rulesVersion=2; Profile profile=Profile::TacticalV2; Faction faction=Faction::Invalid; Archive progression; std::vector<std::string> purchasedOptions; };
Result ValidateCampaign(const Campaign&);
Result NewTacticalCampaign(Faction,Campaign&);
Result PurchaseOption(Campaign&,std::string_view);
// Explicitly reject classic percent application in this profile.
bool UsesClassicModifiers(const Campaign&) noexcept;

struct SquadSnapshot { int id=-1,side=-1,roleId=-1,living=0; bool entered=false,stationary=false,advancing=false,reloading=false,holding=false; std::string nodeId; scene::Point position; };
// V82 receiver amendment: actual member/hull occupancy is ephemeral, never saved.
struct NodeSnapshot { std::string id; int owner=-1; bool blocked=false; std::uint8_t contestedSidesMask=0; };
struct Observation { int observerSquad=-1; std::string targetNode; double observedAt=0,smokeTransmission=0; scene::Point eye; bool hostLineOfSight=false; };
struct MovementReceipt { std::string id,encounterId,edgeId; int squadId=-1; std::uint64_t sequence=0; double arrivedAt=0; };
struct SupportReceipt { std::string planId,encounterId; int side=-1,resolvedSalvos=0; double resolvedAt=0; };
struct Snapshot {
    std::string missionId,encounterId,sceneId; std::uint32_t sceneRevision=0; std::uint64_t stateRevision=0;
    double elapsed=0; bool fighting=true,paused=false,geometryReady=false,hostOrdersAtomic=false;
    std::array<Faction,2> nations{{Faction::Invalid,Faction::Invalid}}; std::array<int,2> directions{{1,-1}};
    std::array<std::array<double,10>,2> supportReadyAt{}; std::array<std::array<int,10>,2> supportRoundBudget{},supportSalvoCount{};
    std::vector<SquadSnapshot> squads; std::vector<NodeSnapshot> nodes; std::vector<Observation> observations; std::vector<MovementReceipt> movements; std::vector<SupportReceipt> supportReceipts;
    // Positions are canonical scene feet coordinates; host bridge mirrors world X
    // once using player travel direction. Scene geometry is not a faction array.
};
struct Command {
    std::string receiptId,missionId,encounterId,targetNode,alternateNode,optionId,storyId,choiceId;
    std::uint64_t sequence=0,expectedRevision=0,hostFrame=0; CommandKind kind=CommandKind::AdvanceTime;
    int side=-1,supportId=0,variant=0; std::vector<int> squadIds;
};
struct Intent {
    std::string id,targetNode,alternateNode,stageId,edgeId; IntentKind kind=IntentKind::NotifyTelegraph;
    int side=-1,squadId=-1,supportId=-1,roundBudget=0,salvos=0; bool hold=false,success=false;
    double notBefore=0; scene::Point target; std::vector<scene::Point> salvoTargets; std::vector<std::string> routeEdges;
};
enum class FirePhase : std::uint8_t { Idle, Planned, Observing, Observed, Committed };
struct FireState { FirePhase phase=FirePhase::Idle; std::string node,alternateNode,observerNode,planId; int supportId=0,observerSquad=-1,secondaryObserver=-1,roundBudget=0; double observationStart=0,lastObservation=0,commitAt=0,commitDelay=0,observationRequired=4; int stencil=0,followupPhase=4; bool followupUsed=false,waitReload=false,dualObservation=false; std::vector<int> followupSquads,reloadSquads; };
struct ReserveState { std::vector<int> squads; std::vector<std::string> axes; double heldAt=0,lastReleaseAt=-100; std::size_t released=0; bool waitReload=false,amended=false; };
struct CounterState { std::vector<int> squads; std::string node; double warnedAt=-100,readyAt=0; bool pending=false; };
struct StageState { std::size_t index=0; double activatedAt=0,lastElapsed=0,progress=0,deadline=0; bool fallback=false,fallbackArrived=false,finished=false,success=false; std::string boundObstacle; std::vector<int> withdrawalParticipants,holdParticipants,fallbackWorkers; std::vector<std::pair<int,std::uint64_t>> arrivalSequences; std::vector<std::string> completedStages,clearedObstacles,usedMovementReceipts; };
struct ChoiceRecord { std::string storyId,choiceId; };
struct Receipt { std::string id,payload; std::uint64_t sequence=0; int side=-1; };
// Integration amendment: one persistent, bounded AdvanceTime watermark.
// Non-clock commands retain the original transaction journal.
struct ClockReceipt {Receipt receipt;double capturedElapsed=0;std::uint64_t hostFrame=0;};
struct State {
    int rulesVersion=2,playerSide=0; std::string missionId,encounterId; Campaign campaign; std::uint64_t revision=0;
    // Host-bound volatile replay capability; never part of a campaign codec.
    bool fullArmoryReplay=false;
    double elapsed=0; std::array<std::uint64_t,2> nextSequence{{1,1}}; std::array<FireState,2> fire; std::array<ReserveState,2> reserve; std::array<CounterState,2> counter;
    StageState objective; std::vector<ChoiceRecord> choices; std::vector<Receipt> receipts;ClockReceipt latestClock;
    std::string approach="primary",followupPolicy="limited_objective_then_hold",withdrawPolicy="hold_current_until_gate_complete";
    std::vector<std::pair<std::string,std::string>> clearanceTargets;
    std::vector<std::pair<int,std::string>> arrivalHolds; bool followupPolicyActive=false,withdrawPolicyActive=false;
};
Result NewEncounter(const Campaign&,std::string_view mission,std::string_view encounter,int playerSide,double elapsed,State&);
Result ValidateSnapshot(const State&,const Snapshot&);
Result ValidateState(const State&);
std::string StateSignature(const State&);
struct Decision { bool allowed=false,duplicate=false; std::string reason; std::vector<Intent> intents; };
class Prepared {
public:
    const Decision& View() const noexcept { return decision_; }
private:
    Decision decision_; State candidate_; std::string sourceState_,sourceSnapshot_; Command command_;
    friend Prepared Prepare(const State&,const Snapshot&,const Command&);
    friend Result Acknowledge(State&,const Snapshot&,const Prepared&,const std::vector<std::string>&);
};
// Pure evaluation; UI and AI call exactly the same legality path as production.
Decision Evaluate(const State&,const Snapshot&,const Command&);
Prepared Prepare(const State&,const Snapshot&,const Command&);
// all intent IDs must be successfully prevalidated/executed as ONE host batch.
// Partial actor side effects are forbidden by hostOrdersAtomic capability.
Result Acknowledge(State&,const Snapshot&,const Prepared&,const std::vector<std::string>& successfulIntentIds);
} // namespace wfexp::tactical
