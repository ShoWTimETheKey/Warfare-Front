#include "TacticalRuntime.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace wfexp::tactical {
namespace {
#include "TacticalRules.generated.inl"
#include "WorldLayoutV82.generated.inl"
#include "StageBindingsV87.generated.inl"
#include "ArmourOptionsV87.generated.inl"
Result Good() { return {true,{}}; }
Result Bad(std::string why) { return {false,std::move(why)}; }
bool Finite(double v) { return std::isfinite(v); }
bool Time(double v) { return Finite(v) && v>=0 && v<=86400; }
bool PointOK(const scene::Point& p) { return Finite(p.x)&&Finite(p.y)&&Finite(p.z); }
double Distance(const scene::Point& a,const scene::Point& b) { return std::hypot(a.x-b.x,a.y-b.y); }
bool Id(std::string_view v) { if(v.empty()||v.size()>128)return false;for(char c:v)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;return true; }
bool Guid(std::string_view v) { if(v.size()!=36)return false;for(std::size_t i=0;i<v.size();++i){char c=v[i];if(i==8||i==13||i==18||i==23){if(c!='-')return false;}else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;}return true; }
template<class T> bool Has(const std::vector<T>& v,const T& x) { return std::find(v.begin(),v.end(),x)!=v.end(); }
const NodeDefinition* Node(const MissionDefinition& m,std::string_view id) { for(const auto& n:m.nodes)if(n.id==id)return &n;return nullptr; }
const EdgeDefinition* Edge(const MissionDefinition& m,std::string_view id) { for(const auto& e:m.edges)if(e.id==id)return &e;return nullptr; }
const SquadSnapshot* Squad(const Snapshot& s,int id) { for(const auto& q:s.squads)if(q.id==id)return &q;return nullptr; }
const NodeSnapshot* NodeState(const Snapshot& s,std::string_view id) { for(const auto& n:s.nodes)if(n.id==id)return &n;return nullptr; }
bool At(const SquadSnapshot& q,const NodeDefinition& n) { return q.nodeId==n.id&&Distance(q.position,n.position)<=n.radius; }
bool Living(const SquadSnapshot* q,int side) { return q&&q->side==side&&q->living>0&&q->entered; }
bool Bought(const State& s,int index) { return Has(s.campaign.purchasedOptions,std::string(FactionId(s.campaign.faction))+".tech."+std::to_string(index)); }
bool ObserverRole(int role) { return role==3||role==4||role==10; }
struct Writer {
    std::ostringstream out;
    Writer(){out.imbue(std::locale::classic());out<<std::setprecision(17);}
    void Add(std::string_view v){out<<v.size()<<':'<<v<<';';}
    void Add(const std::string& v){Add(std::string_view(v));}
    template<class T> void Add(T v){out<<v<<';';}
    void Point(const scene::Point& p){Add(p.x);Add(p.y);Add(p.z);}
    template<class T>void List(const std::vector<T>& a){Add(a.size());for(const auto& x:a)Add(x);}
};
void CampaignWrite(Writer& w,const Campaign& c){
    w.Add(c.schemaVersion);w.Add(c.rulesVersion);w.Add(static_cast<int>(c.profile));w.Add(static_cast<int>(c.faction));w.List(c.purchasedOptions);
    const auto& a=c.progression;w.Add(a.schemaVersion);w.Add(a.contentRevision);w.Add(a.packId);w.Add(a.factionId);w.Add(a.selectedMissionId);w.Add(a.researchPoints);w.Add(a.victories);w.Add(a.defeats);w.Add(a.technologyMask);w.List(a.wonMissionIds);w.List(a.boughtTechnologyIds);w.List(a.resolvedDispatchIds);w.List(a.choiceIds);w.List(a.defeatedEncounterIds);w.List(a.resolvedEncounterIds);w.List(a.consumedLoanDispatchIds);w.Add(a.pendingLoanRoleId);w.Add(a.pendingLoanDispatchId);w.Add(a.pendingLoanMissionId);w.Add(a.activeEncounterId);w.Add(a.activeEncounterMissionId);
}
std::string Payload(const Command& c){Writer w;w.Add(c.receiptId);w.Add(c.missionId);w.Add(c.encounterId);w.Add(c.sequence);w.Add(c.hostFrame);w.Add(static_cast<int>(c.kind));w.Add(c.side);w.Add(c.supportId);w.Add(c.variant);w.Add(c.targetNode);w.Add(c.alternateNode);w.Add(c.optionId);w.Add(c.storyId);w.Add(c.choiceId);w.List(c.squadIds);return w.out.str();}
std::string ClockPayload(const State& state,const ClockReceipt& clock){
    Command command;command.kind=CommandKind::AdvanceTime;command.receiptId=clock.receipt.id;command.missionId=state.missionId;command.encounterId=state.encounterId;command.sequence=clock.receipt.sequence;command.hostFrame=clock.hostFrame;command.side=clock.receipt.side;
    Writer proof;proof.Add(Payload(command));proof.Add(clock.capturedElapsed);return proof.out.str();
}
std::string SnapshotSignature(const Snapshot& s){Writer w;w.Add(s.missionId);w.Add(s.encounterId);w.Add(s.sceneId);w.Add(s.sceneRevision);w.Add(s.stateRevision);w.Add(s.elapsed);w.Add(s.fighting);w.Add(s.paused);w.Add(s.geometryReady);w.Add(s.hostOrdersAtomic);for(int side=0;side<2;++side){w.Add(static_cast<int>(s.nations[side]));w.Add(s.directions[side]);for(int i=0;i<10;++i){w.Add(s.supportReadyAt[side][i]);w.Add(s.supportRoundBudget[side][i]);w.Add(s.supportSalvoCount[side][i]);}}w.Add(s.squads.size());for(const auto& q:s.squads){w.Add(q.id);w.Add(q.side);w.Add(q.roleId);w.Add(q.living);w.Add(q.entered);w.Add(q.stationary);w.Add(q.advancing);w.Add(q.reloading);w.Add(q.holding);w.Add(q.nodeId);w.Point(q.position);}w.Add(s.nodes.size());for(const auto& n:s.nodes){w.Add(n.id);w.Add(n.owner);w.Add(n.blocked);w.Add(static_cast<int>(n.contestedSidesMask));}w.Add(s.observations.size());for(const auto& o:s.observations){w.Add(o.observerSquad);w.Add(o.targetNode);w.Add(o.observedAt);w.Add(o.smokeTransmission);w.Point(o.eye);w.Add(o.hostLineOfSight);}w.Add(s.movements.size());for(const auto& a:s.movements){w.Add(a.id);w.Add(a.encounterId);w.Add(a.edgeId);w.Add(a.squadId);w.Add(a.sequence);w.Add(a.arrivedAt);}w.Add(s.supportReceipts.size());for(const auto& a:s.supportReceipts){w.Add(a.planId);w.Add(a.encounterId);w.Add(a.side);w.Add(a.resolvedSalvos);w.Add(a.resolvedAt);}return w.out.str();}
bool Proof(const State& st,const Snapshot& s,int side,int squad,std::string_view target){
    const auto* q=Squad(s,squad);const auto* m=FindMission(st.missionId);const auto* n=m?Node(*m,target):nullptr;const auto* g=scene::ForMission(st.missionId);
    if(!Living(q,side)||!q->stationary||q->advancing||!ObserverRole(q->roleId)||!n||!g)return false;
    for(const auto& o:s.observations)if(o.observerSquad==squad&&o.targetNode==target&&s.elapsed-o.observedAt<=.25&&o.hostLineOfSight&&o.smokeTransmission>=.5&&Distance(o.eye,q->position)<=400&&o.eye.z>=q->position.z+50&&o.eye.z<=q->position.z+250){scene::Point endpoint=n->position;endpoint.z+=135;if(scene::LineOfSight(*g,o.eye,endpoint))return true;}return false;
}
bool AnyProof(const State& st,const Snapshot& s,int side,std::string_view target){for(const auto& q:s.squads)if(Proof(st,s,side,q.id,target))return true;return false;}
bool Cleared(const State& st,std::string_view n){return Has(st.objective.clearedObstacles,std::string(n));}
bool RegisteredWorkFallback(const State& st){const auto& d=FindMission(st.missionId)->stages[st.objective.index];return d.kind==GateKind::ClearPassage&&(Has(d.nodes,d.fallbackNode)||Has(d.alternateNodes,d.fallbackNode));}
bool FallbackWorker(const State& st,const SquadSnapshot* q,std::string_view target){const auto& d=FindMission(st.missionId)->stages[st.objective.index];return st.objective.fallback&&RegisteredWorkFallback(st)&&target==d.fallbackNode&&q&&q->roleId==6&&Has(st.objective.fallbackWorkers,q->id);}
// Receiver balance uses the published physical scene, after WorldLayout has
// replaced the prototype coordinates. This runs once, never extends a live
// deadline, and changes neither host elapsed time nor any completion gate.
struct PacingEdge { const EdgeDefinition* edge=nullptr; double seconds=0; };
double PacingRoute(const MissionDefinition& m,const std::vector<PacingEdge>& roads,
    std::string_view from,std::string_view to,const std::set<std::string>& cleared,
    std::string_view clearanceTarget,std::string_view axis)
{
    if(!Node(m,from)||!Node(m,to))return -1;
    if(from==to)return 0;
    struct Step { int cost; std::string node; double seconds; };
    auto later=[](const Step& a,const Step& b){return a.cost>b.cost;};
    std::priority_queue<Step,std::vector<Step>,decltype(later)> work(later);
    std::map<std::string,int> costs;work.push({0,std::string(from),0});costs[std::string(from)]=0;
    while(!work.empty())
    {
        const auto cur=work.top();work.pop();if(cur.node==to)return cur.seconds;
        if(cur.cost!=costs[cur.node])continue;
        for(const auto& road:roads)
        {
            const auto& e=*road.edge;std::string next;
            if(e.from==cur.node)next=e.to;else if(e.bidirectional&&e.to==cur.node)next=e.from;else continue;
            if(e.approach=="enemy"||(!e.clearanceGate.empty()&&!cleared.count(e.clearanceGate)
                &&!(next==to&&e.clearanceGate==clearanceTarget)))continue;
            const bool branch=e.approach=="primary"||e.approach=="alternate";
            const int cost=cur.cost+(branch&&e.approach!=axis?100:1);
            const auto old=costs.find(next);if(old!=costs.end()&&old->second<=cost)continue;
            costs[next]=cost;work.push({cost,next,cur.seconds+road.seconds});
        }
    }
    return -1; // An unavailable physical route never becomes zero travel.
}
double PacingTravel(const MissionDefinition& m,const std::vector<PacingEdge>& roads,
    const std::vector<std::string>& origins,const std::vector<std::string>& targets,
    const std::vector<std::set<std::string>>& clearStates,bool clearance)
{
    double longest=-1;
    for(const auto& cleared:clearStates)for(const auto& from:origins)for(const auto& to:targets)for(const auto* axis:{"primary","alternate"})
        longest=std::max(longest,PacingRoute(m,roads,from,to,cleared,clearance?std::string_view(to):std::string_view(),axis));
    return longest;
}
double PacingReadiness(const MissionDefinition& m,int role)
{
    const auto* r=wfexp::RoleFor(m.player,role,m.date);
    return r&&r->available&&std::isfinite(r->readinessSeconds)?std::max(0.,double(r->readinessSeconds)):0;
}
void ApplyReceiverPacingV82(std::vector<MissionDefinition>& missions)
{
    for(auto& m:missions)
    {
        const auto* ground=scene::ForMission(m.id);if(!ground)continue;
        std::vector<PacingEdge> roads;
        for(const auto& e:m.edges)
        {
            const auto* a=Node(m,e.from);const auto* b=Node(m,e.to);
            if(!a||!b||!scene::CanTraverseRoute(*ground,e.id,a->position,b->position))continue;
            double length=Distance(a->position,b->position);
            if(const auto* route=scene::FindRoute(*ground,e.id))
            {length=0;for(std::size_t p=1;p<route->points.size();++p)length+=Distance(route->points[p-1],route->points[p]);}
            // 350 cm/s ordinary marching plus measured formation/arrival cost.
            // Empty legacy route arrays retain CanTraverseRoute's segment gate.
            if(Finite(length))roads.push_back({&e,length/350.+12.});
        }
        std::vector<std::string> origins{"home"},coverOrigins{"home"};std::vector<std::set<std::string>> clearStates(1);
        int previousRole=-1;
        for(std::size_t index=0;index<m.stages.size();++index)
        {
            auto& d=m.stages[index];auto targets=d.nodes;
            if(d.kind==GateKind::ClearPassage)targets.insert(targets.end(),d.alternateNodes.begin(),d.alternateNodes.end());
            std::vector<std::string> coverTargets;
            for(const auto& target:targets)
                coverTargets.push_back(index==0?(m.player==Faction::Russian?"wire":"front_left")
                    :d.kind==GateKind::ClearPassage?(target=="rail"?"front_right":target=="wire"?"front_left":target):target);
            const bool newWorker=index>0&&d.requiredRole>=0&&d.requiredRole!=previousRole;
            const std::vector<std::string> workerOrigins=newWorker?std::vector<std::string>{"home"}:origins;
            const double worker=PacingTravel(m,roads,workerOrigins,targets,clearStates,d.kind==GateKind::ClearPassage);
            const double cover=PacingTravel(m,roads,coverOrigins,coverTargets,clearStates,false);
            const double workerQueue=d.requiredRole>=0&&(index==0||d.requiredRole!=previousRole)?PacingReadiness(m,d.requiredRole):0;
            // Recruitment shares one queue. A first gun then pioneer waits for
            // pioneer readiness; a first rifle then gun waits for gun readiness.
            const double gunQueue=index==0&&d.requiredRole<0?PacingReadiness(m,1):0;
            const double workerReady=worker+workerQueue+(index==0||newWorker?25.:0.);
            const double critical=std::max(workerReady,cover<0?workerReady:cover+gunQueue+(index==0?25.:0.));
            const bool rear=d.kind==GateKind::WithdrawToNode
                ||std::all_of(targets.begin(),targets.end(),[](const std::string& n){return n=="home"||n=="assembly"||n=="relay"||n=="exit";});
            const double combat=rear?30.:75.;
            const auto rounded=[](double seconds){return std::ceil(seconds/15.)*15.;};
            if(worker>=0)d.timeLimit=std::max(d.timeLimit,rounded(critical+combat+d.duration+15.));
            if(d.timeoutFallback&&!d.fallbackNode.empty())
            {
                // Recovery and the original guard can operate in parallel.
                // A registered clearance backup remains the same frozen worker.
                const std::vector<std::string> backup{d.fallbackNode};auto recoveryOrigins=origins;
                recoveryOrigins.insert(recoveryOrigins.end(),targets.begin(),targets.end());
                const bool backupWork=d.kind==GateKind::ClearPassage&&Has(targets,d.fallbackNode);
                const double recovery=PacingTravel(m,roads,recoveryOrigins,backup,clearStates,backupWork);
                if(recovery>=0)d.fallbackDeadline=std::max(d.fallbackDeadline,rounded(std::max(critical,recovery)+combat+d.duration+15.));
            }
            origins=targets;coverOrigins=coverTargets;previousRole=d.requiredRole;
            // A real clear opens one selected site, never both authored mouths.
            if(d.kind==GateKind::ClearPassage)
            {
                std::vector<std::set<std::string>> next;
                for(const auto& prior:clearStates)for(const auto& target:targets)
                {auto choice=prior;choice.insert(target);next.push_back(std::move(choice));}
                clearStates=std::move(next);
            }
        }
    }
}
const SquadSnapshot* RecoveryAtBackup(const State& st,const Snapshot& s,const MissionDefinition& m,const StageDefinition& d)
{
    const auto* n=Node(m,d.fallbackNode);const auto* ns=NodeState(s,d.fallbackNode);
    if(!n||!ns||ns->owner==1-st.playerSide||(ns->contestedSidesMask&(1u<<(1-st.playerSide))))return nullptr;
    for(const auto& enemy:s.squads)if(Living(&enemy,1-st.playerSide)&&At(enemy,*n))return nullptr;
    for(const auto& q:s.squads)
        if(Living(&q,st.playerSide)&&q.living>=d.minimumLiving&&q.stationary&&!q.advancing&&At(q,*n)
            &&(d.requiredRole<0||q.roleId==d.requiredRole)
            &&(!RegisteredWorkFallback(st)||FallbackWorker(st,&q,d.fallbackNode))
            &&(d.kind!=GateKind::WithdrawToNode||Has(st.objective.withdrawalParticipants,q.id)))return &q;
    return nullptr;
}
std::vector<std::string> Route(const State& st,const Snapshot& s,int side,std::string_view from,std::string_view to,std::string_view preferred={},bool approachingObstacle=false){
    const auto* m=FindMission(st.missionId);const auto* g=scene::ForMission(st.missionId);if(!m||!g||!Node(*m,from)||!Node(*m,to)||from==to)return {};
    const std::string axis=preferred.empty()?(side==st.playerSide?st.approach:std::string("primary")):std::string(preferred);
    // A registered nonpreferred branch remains available for mandatory spurs,
    // but a whole legal chosen approach wins over a slightly shorter other axis.
    struct Step{int cost;std::string node;std::vector<std::string> path;};
    auto later=[](const Step& a,const Step& b){return a.cost>b.cost;};
    std::priority_queue<Step,std::vector<Step>,decltype(later)> work(later);std::map<std::string,int> costs;work.push({0,std::string(from),{}});costs[std::string(from)]=0;
    while(!work.empty()){auto cur=work.top();work.pop();if(cur.node==to)return cur.path;if(cur.cost!=costs[cur.node])continue;
        for(const auto& e:m->edges){std::string next;if(e.from==cur.node)next=e.to;else if(e.bidirectional&&e.to==cur.node)next=e.from;else continue;
            if((side==st.playerSide&&e.approach=="enemy")||(!e.clearanceGate.empty()&&!Cleared(st,e.clearanceGate)&&!(approachingObstacle&&next==to&&e.clearanceGate==to)))continue;
            const auto* ns=NodeState(s,next);if(ns&&((ns->blocked&&!Cleared(st,next)&&!(approachingObstacle&&next==to))||(ns->owner==1-side&&next!=to)))continue;
            const auto* a=Node(*m,cur.node);const auto* b=Node(*m,next);if(!scene::CanTraverseRoute(*g,e.id,a->position,b->position))continue;
            const bool branch=e.approach=="primary"||e.approach=="alternate";int cost=cur.cost+(branch&&e.approach!=axis?100:1);auto old=costs.find(next);if(old!=costs.end()&&old->second<=cost)continue;costs[next]=cost;auto path=cur.path;path.push_back(e.id);work.push({cost,next,std::move(path)});
        }
    }return {};
}
void Emit(std::vector<Intent>& out,const Command& c,Intent i){i.id=c.receiptId+"."+std::to_string(out.size());out.push_back(std::move(i));}
Result Move(const State& st,const Snapshot& s,const Command& c,int id,std::string_view target,std::vector<Intent>& out,std::string_view preferred={}){
    const auto* q=Squad(s,id);const auto* m=FindMission(st.missionId);const auto* n=m?Node(*m,target):nullptr;
    if(!Living(q,c.side)||!q->stationary||q->advancing||!n||!Node(*m,q->nodeId)||!At(*q,*Node(*m,q->nodeId)))return Bad("Move requires one existing entered stationary squad and registered source/target");
    if(c.side==st.playerSide&&!st.objective.fallback&&st.withdrawPolicyActive&&st.withdrawPolicy=="hold_current_until_gate_complete"&&m->stages[st.objective.index].kind==GateKind::HoldNode&&Has(st.objective.holdParticipants,q->id)&&q->nodeId==st.objective.boundObstacle&&target!=q->nodeId)return Bad("Current hold gate must finish before tactical withdrawal");
    int occupied=0;for(const auto& a:s.squads)if(a.id!=id&&Living(&a,c.side)&&At(a,*n))++occupied;for(const auto& a:out)if(a.kind==IntentKind::AdvanceExistingSquad&&a.side==c.side&&a.targetNode==target&&a.squadId!=id)++occupied;
    if(occupied>=n->capacity)return Bad("Target node has no ordinary squad slot");
    const bool workerApproach=c.side==st.playerSide&&q->roleId==6&&m->stages[st.objective.index].kind==GateKind::ClearPassage&&((target==st.objective.boundObstacle&&(!st.objective.fallback||!RegisteredWorkFallback(st)||Has(st.objective.fallbackWorkers,q->id)))||FallbackWorker(st,q,target));
    auto route=Route(st,s,c.side,q->nodeId,target,preferred,workerApproach);if(route.empty())return Bad("No registered physical route; no teleport fallback");
    Intent i;i.kind=IntentKind::AdvanceExistingSquad;i.side=c.side;i.squadId=id;i.targetNode=std::string(target);i.target=n->position;i.routeEdges=std::move(route);Emit(out,c,std::move(i));return Good();
}
void Hold(const Command& c,int id,bool holding,std::vector<Intent>& out){Intent i;i.kind=IntentKind::SetHold;i.side=c.side;i.squadId=id;i.hold=holding;Emit(out,c,std::move(i));}
void Activate(State& st,const Snapshot* s,double elapsed){auto& o=st.objective;const auto& mission=*FindMission(st.missionId);const auto& d=mission.stages[o.index];const std::string previousClearSite=o.boundObstacle;o.activatedAt=elapsed;o.lastElapsed=elapsed;o.progress=0;o.deadline=elapsed+d.timeLimit;o.fallback=false;o.fallbackArrived=false;o.boundObstacle=d.nodes.front();for(const auto& b:st.clearanceTargets)if(b.first==d.id)o.boundObstacle=b.second;
    if(d.followClearedPassageV87&&o.index>0){const auto& prior=mission.stages[o.index-1];
        if(prior.kind==GateKind::ClearPassage&&Has(o.completedStages,prior.id)&&Cleared(st,previousClearSite)
            &&(Has(prior.nodes,previousClearSite)||Has(prior.alternateNodes,previousClearSite)))o.boundObstacle=previousClearSite;}
    o.withdrawalParticipants.clear();o.holdParticipants.clear();o.fallbackWorkers.clear();o.arrivalSequences.clear();if(s)for(const auto& a:s->movements){bool found=false;for(auto& p:o.arrivalSequences)if(p.first==a.squadId){p.second=std::max(p.second,a.sequence);found=true;}if(!found)o.arrivalSequences.emplace_back(a.squadId,a.sequence);}if(s&&d.kind==GateKind::WithdrawToNode)for(const auto& q:s->squads)if(Living(&q,st.playerSide))o.withdrawalParticipants.push_back(q.id);}
Result Story(State& st,const Snapshot& s,const StoryDefinition& story,const StoryChoice& choice){
    if(st.objective.index>=FindMission(st.missionId)->stages.size()||story.stageId!=FindMission(st.missionId)->stages[st.objective.index].id)return Bad("Story is not active");
    if(s.elapsed<st.objective.activatedAt+story.triggerAfter)return Bad("Story trigger has not occurred");
    for(const auto& r:st.choices)if(r.storyId==story.id)return r.choiceId==choice.id?Good():Bad("Story choice receipt already consumed");
    for(const auto& e:choice.effects){
        if(e.kind=="SetApproachConstraint"){if(e.value!="primary"&&e.value!="alternate")return Bad("Unknown approach");st.approach=e.value;}
        else if(e.kind=="BindClearanceObstacle"){auto split=e.value.find('|');if(split==std::string::npos)return Bad("Invalid obstacle binding");auto stage=e.value.substr(0,split),node=e.value.substr(split+1);bool found=false;for(auto& b:st.clearanceTargets)if(b.first==stage){b.second=node;found=true;}if(!found)st.clearanceTargets.emplace_back(stage,node);if(stage==FindMission(st.missionId)->stages[st.objective.index].id&&st.objective.boundObstacle!=node){st.objective.boundObstacle=node;st.objective.progress=0;}}
        else if(e.kind=="SelectFireStencil")st.fire[st.playerSide].stencil=e.value=="three_corridor_one_counterroute"?1:0;
        else if(e.kind=="SetFollowupPolicy"){st.followupPolicyActive=true;st.followupPolicy=e.value;}
        else if(e.kind=="SetWithdrawPolicy"){st.withdrawPolicyActive=true;st.withdrawPolicy=e.value;}
        else return Bad("Story effect has no native consumer");
    }st.choices.push_back({story.id,choice.id});return Good();
}
Result SupportIntent(const State& st,const Snapshot& s,const Command& c,std::string_view target,int salvos,std::vector<Intent>& out){
    const auto* m=FindMission(st.missionId);const auto* n=m?Node(*m,target):nullptr;const auto* base=wfexp::FindMission(st.missionId);
    if(c.supportId>=0&&c.supportId<10&&salvos==0)salvos=s.supportSalvoCount[c.side][c.supportId];
    if(!n||c.supportId<0||c.supportId>=10||salvos<1||s.supportSalvoCount[c.side][c.supportId]!=salvos||!SupportFor(s.nations[c.side],c.supportId,m->date)||(c.side==st.playerSide&&!st.fullArmoryReplay&&!SupportAllowed(*base,c.supportId))||s.supportReadyAt[c.side][c.supportId]>s.elapsed||s.supportRoundBudget[c.side][c.supportId]<salvos)return Bad("Support unavailable, era locked, not ready or lacks existing round budget");
    Intent i;i.kind=IntentKind::RequestSupport;i.side=c.side;i.supportId=c.supportId;i.targetNode=std::string(target);i.target=n->position;i.roundBudget=s.supportRoundBudget[c.side][c.supportId];i.salvos=salvos;i.notBefore=s.elapsed;for(int a=0;a<salvos;++a)i.salvoTargets.push_back(n->position);Emit(out,c,std::move(i));return Good();
}
Result Register(State& st,const Snapshot& s,const Command& c,std::vector<Intent>& out){auto& r=st.reserve[c.side];if(s.nations[c.side]!=Faction::Russian||c.side!=st.playerSide)return Bad("Reserve mechanism requires Russian player");if(r.released<r.squads.size())return Bad("Existing reserve must finish before registering another");const std::size_t cap=Bought(st,6)?2u:1u;if(c.squadIds.empty()||c.squadIds.size()>cap)return Bad("Reserve capacity exceeded");if(!Node(*FindMission(st.missionId),c.targetNode))return Bad("Unknown reserve axis");ReserveState fresh;fresh.heldAt=s.elapsed;fresh.waitReload=r.waitReload;for(int id:c.squadIds){const auto* q=Squad(s,id);if(!Living(q,c.side)||!q->stationary||q->advancing||q->nodeId.empty()||(q->nodeId!="home"&&q->nodeId!="assembly"&&q->nodeId!="exit"))return Bad("Reserve must be a real entered rear squad, no copied recruitment");fresh.squads.push_back(id);fresh.axes.push_back(c.targetNode);Hold(c,id,true,out);}r=std::move(fresh);return Good();}
Result Release(State& st,const Snapshot& s,const Command& c,std::vector<Intent>& out){auto& r=st.reserve[c.side];if(s.nations[c.side]!=Faction::Russian||c.side!=st.playerSide||r.released>=r.squads.size())return Bad("No unreleased existing Russian squad");if(s.elapsed<r.heldAt+4||s.elapsed<r.lastReleaseAt+6)return Bad("Assembly/echelon interval not ready");int id=r.squads[r.released];const auto* q=Squad(s,id);if(!Living(q,c.side)){++r.released;return Good();}if(r.waitReload&&q->reloading&&s.elapsed<r.heldAt+30)return Bad("Real weapon cycle still reloading");std::string target=r.axes[r.released];std::string preferred;if(st.followupPolicyActive&&st.followupPolicy=="reserve_for_second_route")preferred=r.released>0?"alternate":"primary";auto result=Move(st,s,c,id,target,out,preferred);if(!result)return result;if(st.followupPolicyActive&&st.followupPolicy=="limited_objective_then_hold")st.arrivalHolds.emplace_back(id,target);r.lastReleaseAt=s.elapsed;++r.released;return Good();}
Result Finish(State& st,const Command& c,bool success,std::vector<Intent>& out){auto result=success?RecordVictory(st.campaign.progression,st.missionId,st.encounterId):RecordDefeat(st.campaign.progression,st.missionId,st.encounterId);if(!result)return result;st.objective.finished=true;st.objective.success=success;Intent i;i.kind=IntentKind::LocalObjectiveResult;i.side=st.playerSide;i.success=success;Emit(out,c,std::move(i));return Good();}
const MovementReceipt* Arrival(const State& st,const Snapshot& s,int side,std::string_view target,int minimum,const std::vector<int>* participants=nullptr){const auto* m=FindMission(st.missionId);const auto* n=Node(*m,target);for(const auto& a:s.movements){const auto* q=Squad(s,a.squadId);const auto* e=Edge(*m,a.edgeId);if(!Living(q,side)||q->living<minimum||!At(*q,*n)||!e||a.arrivedAt<=st.objective.activatedAt||Has(st.objective.usedMovementReceipts,a.id)||(participants&&!Has(*participants,a.squadId)))continue;std::uint64_t previous=0;for(const auto& p:st.objective.arrivalSequences)if(p.first==a.squadId)previous=p.second;if(a.sequence<=previous||(!e->clearanceGate.empty()&&!Cleared(st,e->clearanceGate)&&!(side==st.playerSide&&!st.objective.fallbackArrived&&FallbackWorker(st,q,target)&&e->clearanceGate==target)))continue;if(e->to==target||(e->bidirectional&&e->from==target))return &a;}return nullptr;}
Result Tick(State& st,const Snapshot& s,const Command& c,std::vector<Intent>& out){
    const auto* m=FindMission(st.missionId);auto& o=st.objective;if(o.finished)return Bad("Encounter already resolved");
    for(auto it=st.arrivalHolds.begin();it!=st.arrivalHolds.end();){const auto* q=Squad(s,it->first);const auto* n=Node(*m,it->second);if(!Living(q,st.playerSide)){it=st.arrivalHolds.erase(it);continue;}if(n&&At(*q,*n)&&q->stationary&&!q->advancing){Command hold=c;hold.side=st.playerSide;Hold(hold,q->id,true,out);it=st.arrivalHolds.erase(it);}else ++it;}
    for(const auto& story:m->stories)if(story.stageId==m->stages[o.index].id&&s.elapsed>=o.activatedAt+story.triggerAfter+story.deadline){bool chosen=false;for(const auto& r:st.choices)if(r.storyId==story.id)chosen=true;if(!chosen){for(const auto& choice:story.choices)if(choice.id==story.defaultChoice){auto result=Story(st,s,story,choice);if(!result)return result;}}}
    // A held Russian squad must not receive an indefinite tactical freeze. At
    // 30s issue its ordinary movement order; a blocked route stays visibly held
    // and may be manually cancelled/reassigned, never spawned or teleported.
    if(s.nations[st.playerSide]==Faction::Russian){auto& r=st.reserve[st.playerSide];if(r.released<r.squads.size()&&s.elapsed>=r.heldAt+30&&s.elapsed>=r.lastReleaseAt+6){Command release=c;release.side=st.playerSide;auto result=Release(st,s,release,out);if(!result){int id=r.squads[r.released];Hold(release,id,false,out);++r.released;r.lastReleaseAt=s.elapsed;}}}
    const auto& d=m->stages[o.index];double dt=std::min(1.0,std::max(0.0,s.elapsed-o.lastElapsed));o.lastElapsed=s.elapsed;
    if(s.elapsed>=o.deadline){if(d.timeoutFallback&&!o.fallback){o.fallback=true;o.fallbackArrived=false;o.progress=0;o.deadline=s.elapsed+d.fallbackDeadline;o.activatedAt=s.elapsed;o.fallbackWorkers.clear();if(RegisteredWorkFallback(st))for(const auto& q:s.squads)if(Living(&q,st.playerSide)&&q.roleId==6)o.fallbackWorkers.push_back(q.id);o.arrivalSequences.clear();for(const auto& a:s.movements){bool found=false;for(auto& p:o.arrivalSequences)if(p.first==a.squadId){p.second=std::max(p.second,a.sequence);found=true;}if(!found)o.arrivalSequences.emplace_back(a.squadId,a.sequence);}return Good();}return Finish(st,c,false,out);}
    if(!d.choiceReceipt.empty()){bool chosen=false;for(const auto& r:st.choices)if(r.storyId==d.choiceReceipt)chosen=true;if(!chosen)return Good();}
    if(o.fallback&&!o.fallbackArrived)
    {
        // This is current actual occupancy, not a fabricated arrival. The host
        // node/stationary proof covers every living member and hostile hull.
        const auto* recovery=RecoveryAtBackup(st,s,*m,d);if(!recovery)return Good();
        const auto* a=Arrival(st,s,st.playerSide,d.fallbackNode,d.minimumLiving,RegisteredWorkFallback(st)?&o.fallbackWorkers:nullptr);
        if(a&&a->squadId==recovery->id)o.usedMovementReceipts.push_back(a->id);
        o.fallbackArrived=true;if(RegisteredWorkFallback(st))o.boundObstacle=d.fallbackNode;
        o.progress=0;o.lastElapsed=s.elapsed;return Good();
    }
    std::string target=o.boundObstacle;const auto* n=Node(*m,target);const auto* ns=NodeState(s,target);bool contested=ns&&(ns->contestedSidesMask&(1u<<(1-st.playerSide)));for(const auto& q:s.squads)if(Living(&q,1-st.playerSide)&&At(q,*n))contested=true;
    bool complete=false;
    if(d.kind==GateKind::HoldNode||d.kind==GateKind::ClearPassage){bool worker=false;int guard=-1;for(const auto& q:s.squads)if(Living(&q,st.playerSide)&&q.living>=d.minimumLiving&&At(q,*n)&&(!d.stationary||(q.stationary&&!q.advancing))&&(d.requiredRole<0||q.roleId==d.requiredRole)&&(d.kind!=GateKind::ClearPassage||!q.holding)&&(!(d.kind==GateKind::ClearPassage&&o.fallback&&RegisteredWorkFallback(st))||Has(o.fallbackWorkers,q.id))){worker=true;if(guard<0)guard=q.id;}bool owner=true;if(d.kind==GateKind::HoldNode){owner=ns&&ns->owner==st.playerSide;for(const auto& own:d.mustOwn){const auto* state=NodeState(s,own);owner=owner&&state&&state->owner==st.playerSide;}}bool active=worker&&owner&&!contested;if(active){o.progress+=dt;if(d.kind==GateKind::HoldNode&&o.holdParticipants.empty())o.holdParticipants.push_back(guard);}else if(d.kind==GateKind::HoldNode){o.progress=0;o.holdParticipants.clear();}complete=active&&o.progress>=d.duration;}
    else {const auto* a=Arrival(st,s,st.playerSide,target,d.minimumLiving,d.kind==GateKind::WithdrawToNode?&o.withdrawalParticipants:nullptr);if(a&&!contested){o.usedMovementReceipts.push_back(a->id);complete=true;}}
    if(!complete)return Good();
    if(d.kind==GateKind::ClearPassage){if(!Cleared(st,target))o.clearedObstacles.push_back(target);Intent clear;clear.kind=IntentKind::ClearObstacle;clear.side=st.playerSide;clear.targetNode=target;clear.stageId=d.id;Emit(out,c,std::move(clear));}
    o.completedStages.push_back(d.id);Intent stage;stage.kind=IntentKind::ObjectiveStageComplete;stage.side=st.playerSide;stage.stageId=d.id;Emit(out,c,std::move(stage));if(d.finalStage)return Finish(st,c,true,out);++o.index;Activate(st,&s,s.elapsed);return Good();
}
Result Option(State& st,const Snapshot& s,const Command& c,std::vector<Intent>& out){
    const auto* option=FindOption(c.optionId);if(!option||!option->implemented||option->faction!=s.nations[c.side]||c.side!=st.playerSide||!Has(st.campaign.purchasedOptions,c.optionId))return Bad("Option not purchased or no consumer");int index=option->index;const bool fr=option->faction==Faction::French;auto& f=st.fire[c.side];auto& r=st.reserve[c.side];const auto* m=FindMission(st.missionId);
    auto one=[&]()->const SquadSnapshot* {return c.squadIds.size()==1?Squad(s,c.squadIds.front()):nullptr;};
    if(index==0){const auto* q=one();if(!Living(q,c.side)||!q->stationary||q->advancing||(fr&&q->roleId!=0))return Bad("Hold option requires a real stationary rifle/reserve");if(!fr&&!Has(r.squads,q->id))return Bad("Russian hold option requires registered reserve");Hold(c,q->id,c.variant==1,out);return Good();}
    if(index==1){if(fr){if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed)return Bad("No editable fire draft");if(c.variant==1){if(c.squadIds.empty())return Bad("Reload alignment requires explicit real gun squads");for(int id:c.squadIds){const auto* gun=Squad(s,id);if(!Living(gun,c.side)||(gun->roleId!=1&&gun->roleId!=9))return Bad("Reload alignment needs existing gun crews");}}f.reloadSquads=c.squadIds;f.commitDelay=c.variant==1?2:0;f.waitReload=c.variant==1;}else{if(r.released>=r.squads.size())return Bad("No existing reserve");r.waitReload=c.variant==1;}return Good();}
    if(index==2){if(fr){if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||c.alternateNode.empty()||!Node(*m,c.alternateNode))return Bad("No editable counterroute stencil");f.alternateNode=c.alternateNode;f.stencil=c.variant;}else{if(r.released!=0||r.squads.empty()||!Node(*m,c.targetNode)||!Node(*m,c.alternateNode))return Bad("Axes may only change before first release");for(std::size_t i=0;i<r.axes.size();++i)r.axes[i]=(c.variant==1&&i>0)?c.alternateNode:c.targetNode;}return Good();}
    if(index==3){if(fr){if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||!Node(*m,c.targetNode)||!Node(*m,c.alternateNode)||!AnyProof(st,s,c.side,c.targetNode)||!AnyProof(st,s,c.side,c.alternateNode))return Bad("Both drafts require current visible proof");f.node=c.variant==1?c.alternateNode:c.targetNode;f.alternateNode=c.variant==1?c.targetNode:c.alternateNode;f.phase=FirePhase::Planned;f.observerSquad=-1;f.observationRequired=4;f.dualObservation=true;}else{if(!Has(r.axes,c.targetNode))return Bad("Support axis must match a registered real reserve axis");return SupportIntent(st,s,c,c.targetNode,0,out);}return Good();}
    if(index==4){if(fr){if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||c.squadIds.size()!=1)return Bad("Followup phase must be assigned before commitment");const auto* q=one();if(!Living(q,c.side)||(q->roleId!=2&&q->roleId!=7))return Bad("Phase followup requires legal existing assault/VB squad");f.followupPhase=c.variant==1?2:4;f.followupSquads=c.squadIds;}else{if(r.released!=0||r.squads.size()!=2)return Bad("Two unreleased real echelons required");const auto* a=Squad(s,r.squads[0]);const auto* b=Squad(s,r.squads[1]);int wanted=c.variant==1?2:0;if(!a||!b||(a->roleId!=wanted&&b->roleId!=wanted))return Bad("Requested real role absent");if(b->roleId==wanted){std::swap(r.squads[0],r.squads[1]);std::swap(r.axes[0],r.axes[1]);}}return Good();}
    if(index==5||index==7){const auto* q=one();if(!Living(q,c.side))return Bad("Existing squad required");if(index==7&&q->roleId!=1&&q->roleId!=9)return Bad("Anchor relocation requires an actual machinegun role");if(!fr&&index==5){auto it=std::find(r.squads.begin()+static_cast<std::ptrdiff_t>(r.released),r.squads.end(),q->id);if(it==r.squads.end()||r.amended)return Bad("Assembly relocation allowed once for an unreleased reserve");r.amended=true;}return Move(st,s,c,q->id,c.targetNode,out);}
    if(index==6){if(fr){if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||c.squadIds.empty()||c.squadIds.size()>2)return Bad("One or two existing followups must be assigned before commitment");for(int id:c.squadIds)if(!Living(Squad(s,id),c.side))return Bad("Missing existing followup squad");f.followupSquads=c.squadIds;}else{return Register(st,s,c,out);}return Good();}
    if(index==8){if(fr){if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||c.squadIds.size()!=2||!Node(*m,c.targetNode)||!Node(*m,c.alternateNode)||!Proof(st,s,c.side,c.squadIds[0],c.targetNode)||!Proof(st,s,c.side,c.squadIds[1],c.alternateNode))return Bad("Fork requires two actual observing squads");f.node=c.variant==1?c.alternateNode:c.targetNode;f.alternateNode=c.variant==1?c.targetNode:c.alternateNode;f.phase=FirePhase::Planned;f.observerSquad=-1;f.dualObservation=true;f.observationRequired=4;}else{if(!AnyProof(st,s,c.side,c.targetNode)||!AnyProof(st,s,c.side,c.alternateNode))return Bad("Inspect exposes only actually observed routes");Intent i;i.kind=IntentKind::NotifyTelegraph;i.side=c.side;i.targetNode=c.targetNode;i.alternateNode=c.alternateNode;Emit(out,c,std::move(i));}return Good();}
    if(index==9){Command smoke=c;smoke.supportId=1;return SupportIntent(st,s,smoke,c.variant==1?c.alternateNode:c.targetNode,0,out);}
    if(index==10){const StageDefinition* gate=nullptr;for(std::size_t i=st.objective.index;i<m->stages.size();++i){const auto& d=m->stages[i];if(d.kind==GateKind::ClearPassage&&(Has(d.nodes,c.targetNode)||Has(d.alternateNodes,c.targetNode))){gate=&d;break;}}const auto* q=one();if(!gate||!Living(q,c.side)||q->roleId!=6)return Bad("Only an authored clearance target and real pioneer may be bound");if(gate->id==m->stages[st.objective.index].id&&st.objective.fallback&&RegisteredWorkFallback(st)&&(c.targetNode!=gate->fallbackNode||!Has(st.objective.fallbackWorkers,q->id)))return Bad("Active recovery keeps its registered worksite and frozen pioneer identity");bool found=false;for(auto& a:st.clearanceTargets)if(a.first==gate->id){a.second=c.targetNode;found=true;}if(!found)st.clearanceTargets.emplace_back(gate->id,c.targetNode);if(gate->id==m->stages[st.objective.index].id&&st.objective.boundObstacle!=c.targetNode){st.objective.boundObstacle=c.targetNode;st.objective.progress=0;}return Good();}
    if(index==11){if(fr){const auto* q=one();if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||!q||q->id==f.observerSquad||!Proof(st,s,c.side,q->id,f.node))return Bad("Handoff requires another living valid observer before commit");f.observerSquad=q->id;f.observationStart=s.elapsed;f.lastObservation=s.elapsed;f.phase=FirePhase::Observing;}else{if(r.released!=1||r.squads.size()!=2||r.amended||!Node(*m,c.targetNode))return Bad("Only one unreleased second axis amendment");bool signal=false;const auto* relay=Node(*m,"relay");for(const auto& q:s.squads)if(Living(&q,c.side)&&q.roleId==10&&q.stationary&&relay&&At(q,*relay))signal=true;if(!signal)return Bad("A real signal squad must occupy relay");r.axes[1]=c.targetNode;r.amended=true;}return Good();}
    return Bad("Option has no native consumer");
}
Result Reduce(State& st,const Snapshot& s,const Command& c,std::vector<Intent>& out){
    if(st.objective.finished)return Bad("Resolved encounter cannot accept tactical commands");auto& f=st.fire[c.side];auto& counter=st.counter[c.side];const auto* m=FindMission(st.missionId);
    switch(c.kind){
    case CommandKind::AdvanceTime:if(c.side!=st.playerSide)return Bad("Only player objective clock advances");return Tick(st,s,c,out);
    case CommandKind::ChooseStory:{if(c.side!=st.playerSide)return Bad("Enemy cannot choose player story");for(const auto& story:m->stories)if(story.id==c.storyId){if(s.elapsed>st.objective.activatedAt+story.triggerAfter+story.deadline)return Bad("Choice deadline passed; default consumes the same receipt");for(const auto& choice:story.choices)if(choice.id==c.choiceId)return Story(st,s,story,choice);}return Bad("Unknown story choice");}
    case CommandKind::PlanFire:{bool retired=f.phase==FirePhase::Idle;if(f.phase==FirePhase::Committed)for(const auto& receipt:s.supportReceipts)if(receipt.side==c.side&&receipt.planId==f.planId&&receipt.resolvedSalvos==4&&(f.followupUsed||s.elapsed-receipt.resolvedAt>8))retired=true;if(s.nations[c.side]!=Faction::French||c.side!=st.playerSide||!retired||!Node(*m,c.targetNode)||c.supportId!=0)return Bad("One French fixed artillery corridor per side");std::vector<Intent> check;auto result=SupportIntent(st,s,c,c.targetNode,4,check);if(!result)return result;FireState draft;draft.phase=FirePhase::Planned;draft.node=c.targetNode;draft.alternateNode=c.alternateNode;draft.supportId=c.supportId;draft.stencil=f.stencil;draft.roundBudget=s.supportRoundBudget[c.side][c.supportId];f=std::move(draft);return Good();}
    case CommandKind::Observe:{if(s.nations[c.side]!=Faction::French||f.phase==FirePhase::Idle||f.phase==FirePhase::Committed||c.squadIds.size()!=(f.dualObservation?2u:1u))return Bad("No editable French draft");int id=c.squadIds[0];int secondary=f.dualObservation?c.squadIds[1]:-1;if(!Proof(st,s,c.side,id,f.node)||(f.dualObservation&&!Proof(st,s,c.side,secondary,f.alternateNode)))return Bad("Living observer, geometry LOS and smoke proof required");if(f.observerSquad!=id||f.secondaryObserver!=secondary||f.phase==FirePhase::Planned||s.elapsed-f.lastObservation>1.01){f.observationStart=s.elapsed;f.phase=FirePhase::Observing;}f.observerSquad=id;f.secondaryObserver=secondary;f.lastObservation=s.elapsed;if(s.elapsed-f.observationStart>=f.observationRequired)f.phase=FirePhase::Observed;return Good();}
    case CommandKind::CommitFire:{if(s.nations[c.side]!=Faction::French||f.phase!=FirePhase::Observed||!Proof(st,s,c.side,f.observerSquad,f.node)||(f.dualObservation&&!Proof(st,s,c.side,f.secondaryObserver,f.alternateNode))||c.supportId!=f.supportId||s.elapsed-f.lastObservation>1.01)return Bad("Fresh completed observation required");if(f.waitReload){for(int id:f.reloadSquads){const auto* gun=Squad(s,id);if(!Living(gun,c.side)||gun->reloading)return Bad("Frozen real gun crew reload must finish");}}auto result=SupportIntent(st,s,c,f.node,4,out);if(!result)return result;if(s.supportRoundBudget[c.side][c.supportId]!=f.roundBudget)return Bad("Draft round budget changed; prepare a new draft");if(f.stencil==1){const auto* alternate=Node(*m,f.alternateNode);if(!alternate||!AnyProof(st,s,c.side,f.alternateNode))return Bad("Counterroute salvo needs an actually observed registered target");out.back().salvoTargets.back()=alternate->position;}out.back().notBefore=s.elapsed+f.commitDelay;f.phase=FirePhase::Committed;f.planId=out.back().id;f.commitAt=s.elapsed;return Good();}
    case CommandKind::CancelFire:if(f.phase==FirePhase::Idle||f.phase==FirePhase::Committed)return Bad("Only an uncommitted draft can be cancelled");f=FireState{};return Good();
    case CommandKind::Followup:{if(f.phase!=FirePhase::Committed||f.followupUsed||c.side!=st.playerSide)return Bad("No unused committed followup window");bool opened=false;for(const auto& a:s.supportReceipts)if(a.side==c.side&&a.planId==f.planId&&a.resolvedSalvos>=f.followupPhase&&s.elapsed-a.resolvedAt<=8)opened=true;if(!opened)return Bad("Actual salvo phase receipt required");std::vector<int> ids=f.followupSquads.empty()?c.squadIds:f.followupSquads;if(ids.empty()||ids.size()>(Bought(st,6)?2u:1u))return Bad("Followup consumes real squad capacity");for(std::size_t index=0;index<ids.size();++index){std::string target=f.node;if(st.followupPolicyActive&&st.followupPolicy=="limited_objective_then_hold")target="objective";else if(st.followupPolicyActive&&st.followupPolicy=="reserve_for_second_route"&&index>0)target="objective";std::string preferred;if(st.followupPolicyActive&&st.followupPolicy=="reserve_for_second_route")preferred=index>0?"alternate":"primary";auto result=Move(st,s,c,ids[index],target,out,preferred);if(!result)return result;if(st.followupPolicyActive&&st.followupPolicy=="limited_objective_then_hold")st.arrivalHolds.emplace_back(ids[index],target);}f.followupUsed=true;return Good();}
    case CommandKind::RegisterReserve:return Register(st,s,c,out);
    case CommandKind::ReleaseReserve:return Release(st,s,c,out);
    case CommandKind::CounterOrder:{if(s.nations[c.side]!=Faction::AustroHungarian||c.side==st.playerSide||counter.pending||s.elapsed<counter.readyAt||c.squadIds.empty()||c.squadIds.size()>3||!Node(*m,c.targetNode))return Bad("AH counterattack requires existing enemy squads and cooldown");for(int id:c.squadIds){if(!Living(Squad(s,id),c.side)||Route(st,s,c.side,Squad(s,id)->nodeId,c.targetNode).empty())return Bad("Counter squad absent or no physical route");}counter.squads=c.squadIds;counter.node=c.targetNode;counter.warnedAt=s.elapsed;counter.pending=true;Intent i;i.kind=IntentKind::NotifyTelegraph;i.side=c.side;i.targetNode=c.targetNode;i.notBefore=s.elapsed+4;Emit(out,c,std::move(i));return Good();}
    case CommandKind::AdvanceCounter:{if(!counter.pending||s.nations[c.side]!=Faction::AustroHungarian||s.elapsed<counter.warnedAt+4)return Bad("Public counterattack warning has not elapsed");for(int id:counter.squads){if(!Living(Squad(s,id),c.side))continue;auto result=Move(st,s,c,id,counter.node,out);if(!result)return result;}counter.pending=false;counter.readyAt=s.elapsed+60;return Good();}
    case CommandKind::UseOption:return Option(st,s,c,out);
    case CommandKind::AdvanceToObjective:{const auto& gate=m->stages[st.objective.index];const std::string target=st.objective.boundObstacle;const auto* q=c.squadIds.size()==1?Squad(s,c.squadIds[0]):nullptr;if(c.side!=st.playerSide||!Living(q,c.side)||c.targetNode!=target||(gate.requiredRole>=0&&q->roleId!=gate.requiredRole)||(gate.kind==GateKind::WithdrawToNode&&!Has(st.objective.withdrawalParticipants,q->id)))return Bad("Objective approach requires the actual stage participant/role and bound target");return Move(st,s,c,q->id,target,out);}
    case CommandKind::PolicyOrder:{if(c.side!=st.playerSide||c.squadIds.empty()||(!st.objective.fallback&&(!st.withdrawPolicyActive||st.withdrawPolicy!="fallback_after_warning"||s.elapsed<st.objective.deadline-20)))return Bad("Fallback order requires active recovery or chosen public 20 second warning and existing squads");const auto& d=m->stages[st.objective.index];if(!c.targetNode.empty()&&c.targetNode!=d.fallbackNode)return Bad("Fallback order only accepts the registered recovery node");for(int id:c.squadIds){if(st.objective.fallback&&RegisteredWorkFallback(st)&&!FallbackWorker(st,Squad(s,id),d.fallbackNode))return Bad("Clearance recovery requires its frozen existing pioneer");auto result=Move(st,s,c,id,d.fallbackNode,out);if(!result)return result;}return Good();}
    }return Bad("Unknown command");
}
} // namespace

const std::vector<MissionDefinition>& Missions()
{
    static const auto ReceiverMissions=[]()
    {
        auto Result=MissionRows;
        for(auto& Mission:Result)if(Mission.id=="fr.mort_homme")
        {
            const auto* Ground=scene::ForMission(Mission.id);
            if(!Ground)continue;
            Mission.sceneId=Ground->id;Mission.sceneRevision=Ground->revision;
            for(auto& Node:Mission.nodes)
            {
                double Height=0.;
                if(scene::HeightAt(*Ground,Node.position.x,Node.position.y,Height))Node.position.z=Height;
            }
        }
        ApplyWorldLayoutV82(Result);ApplyReceiverStageBindingsV87(Result);ApplyReceiverPacingV82(Result);return Result;
    }();
    return ReceiverMissions;
}
const std::vector<OptionDefinition>& Options(){
    static const std::vector<OptionDefinition> ReceiverOptions=[] {
        auto rows=OptionRows;
        // These entries bind the existing hold/move commands, not a combat
        // improvement. Keep their identities and prerequisite markers intact.
        for(auto& o:rows){
            const bool basic=o.index==0||o.index==7||(o.faction==Faction::French&&(o.index==5||o.index==8));
            if(basic)o.cost=0;
        }
        AppendReceiverArmourOptionsV87(rows);
        return rows;
    }();
    return ReceiverOptions;
}
const MissionDefinition* FindMission(std::string_view id){for(const auto& m:Missions())if(m.id==id)return &m;return nullptr;}
const OptionDefinition* FindOption(std::string_view id){for(const auto& o:Options())if(o.id==id)return &o;return nullptr;}
const char* RulesDigest(){return RulesSHA;}
const char* SceneDigest(){return ScenesSHA;}
double ObservedSupportScore(ObservedFireKind kind,const ObservedFireTarget& t)noexcept{
    if(!Finite(t.importance)||!Finite(t.confidence)||!Finite(t.nearbyWeight)||t.importance<=0||t.confidence<=0||t.confidence>1||t.nearbyWeight<0)return 0;
    const double density=std::min(t.nearbyWeight,8.0);
    double suitability=0;
    switch(kind){
    case ObservedFireKind::Mortar:
        suitability=(t.moving?.70:1.0)+(t.entrenched?.10:0)+(t.firingMachinegun?.20:0);break;
    case ObservedFireKind::FieldArtillery:
        suitability=.82+(t.entrenched?.45:0)+(t.firingMachinegun?.50:0)+(density>=4?.15:0);break;
    case ObservedFireKind::Shrapnel:
        suitability=.50+(t.moving?.60:0)+(t.entrenched?-.35:.20)+std::min(density*.10,.55);
        // A remembered moving column is less useful to an airburst battery.
        suitability*=t.moving?t.confidence:1.;break;
    default:return 0;
    }
    return t.importance*suitability;
}
Result ValidateCampaign(const Campaign& c){if(c.schemaVersion!=2||c.rulesVersion!=2||c.profile!=Profile::TacticalV2||!IsSelectable(c.faction)||ParseFaction(c.progression.factionId)!=c.faction)return Bad("Unsupported tactical save schema/profile/faction");auto r=ValidateArchive(c.progression);if(!r)return r;if(c.progression.technologyMask!=0||!c.progression.boughtTechnologyIds.empty())return Bad("Classic percent technologies cannot be stacked with TacticalV2");if(c.purchasedOptions.size()>24)return Bad("Too many tactical options");std::set<std::string> seen;for(const auto& id:c.purchasedOptions){const auto* t=FindOption(id);if(!t||!t->implemented||t->faction!=c.faction||!seen.insert(id).second)return Bad("Unknown/duplicate tactical option");for(const auto& prerequisite:t->prerequisites)if(!Has(c.purchasedOptions,prerequisite))return Bad("Missing tactical option prerequisite");}return Good();}
Result NewTacticalCampaign(Faction f,Campaign& output){Campaign c;c.faction=f;auto r=NewCampaign(f,c.progression);if(!r)return r;r=ValidateCampaign(c);if(r)output=std::move(c);return r;}
Result PurchaseOption(Campaign& c,std::string_view id){auto r=ValidateCampaign(c);if(!r)return r;const auto* o=FindOption(id);if(!o||o->faction!=c.faction||!o->implemented)return Bad("Unknown or unsupported tactical option");if(Has(c.purchasedOptions,std::string(id)))return Good();if(!c.progression.activeEncounterId.empty())return Bad("Cannot purchase during an active encounter");
    // Research follows the actual unlocked campaign frontier, not the currently
    // selected replay date. An owned option stays valid when revisiting an
    // earlier mission; equipment still checks that mission's real vehicle era.
    int frontierDate=0;for(const auto& m:wfexp::Missions())if(m.faction==c.faction&&CanStart(c.progression,m.id))frontierDate=std::max(frontierDate,m.date);
    if(frontierDate<o->fromDate||frontierDate>o->toDate)return Bad("Armour research is not available at the unlocked campaign date");
    for(const auto& p:o->prerequisites)if(!Has(c.purchasedOptions,p))return Bad("Prerequisite not purchased");if(c.progression.researchPoints<o->cost)return Bad("Insufficient research points");Campaign candidate=c;candidate.progression.researchPoints-=o->cost;candidate.purchasedOptions.emplace_back(id);r=ValidateCampaign(candidate);if(r)c=std::move(candidate);return r;}
bool UsesClassicModifiers(const Campaign& c)noexcept{return c.profile==Profile::ClassicV1;}
Result NewEncounter(const Campaign& c,std::string_view mission,std::string_view encounter,int side,double elapsed,State& output){auto r=ValidateCampaign(c);if(!r)return r;const auto* m=FindMission(mission);if(!m||m->player!=c.faction||!Guid(encounter)||(side!=0&&side!=1)||!Time(elapsed))return Bad("Invalid tactical mission/encounter/side/time");State st;st.campaign=c;st.playerSide=side;st.missionId=std::string(mission);st.encounterId=std::string(encounter);st.elapsed=elapsed;r=SelectMission(st.campaign.progression,mission);if(!r)return r;r=BeginEncounter(st.campaign.progression,mission,encounter);if(!r)return r;Activate(st,nullptr,elapsed);r=ValidateState(st);if(r)output=std::move(st);return r;}
Result ValidateState(const State& st){auto r=ValidateCampaign(st.campaign);if(!r)return r;const auto* m=FindMission(st.missionId);if(st.rulesVersion!=2||!m||m->player!=st.campaign.faction||(st.playerSide!=0&&st.playerSide!=1)||!Guid(st.encounterId)||!Time(st.elapsed)||st.objective.index>=m->stages.size()||st.receipts.size()>200000||st.revision>=std::numeric_limits<std::uint64_t>::max()-1)return Bad("Invalid tactical state identity/bounds");if(!st.objective.finished&&(st.campaign.progression.activeEncounterId!=st.encounterId||st.campaign.progression.activeEncounterMissionId!=st.missionId))return Bad("Active encounter token mismatch");if(!Time(st.objective.activatedAt)||!Time(st.objective.lastElapsed)||!Time(st.objective.deadline)||!Time(st.objective.progress)||st.objective.activatedAt>st.elapsed||st.objective.lastElapsed>st.elapsed)return Bad("Invalid objective clock");if(!Node(*m,st.objective.boundObstacle))return Bad("Unknown bound obstacle");std::set<int> fallbackWorkers;for(int id:st.objective.fallbackWorkers)if(id<0||!fallbackWorkers.insert(id).second||fallbackWorkers.size()>4096)return Bad("Invalid frozen fallback worker identities");for(int side=0;side<2;++side){const auto& f=st.fire[side];const auto& rsv=st.reserve[side];const auto& counter=st.counter[side];if(st.nextSequence[side]==0||st.nextSequence[side]==std::numeric_limits<std::uint64_t>::max()||static_cast<int>(f.phase)>4||!Time(f.observationStart)||!Time(f.lastObservation)||!Time(f.commitAt)||!Time(f.commitDelay)||!Time(f.observationRequired)||f.roundBudget<0||f.roundBudget>10000||f.followupSquads.size()>2||f.reloadSquads.size()>3||rsv.squads.size()>2||rsv.axes.size()!=rsv.squads.size()||rsv.released>rsv.squads.size()||!Time(rsv.heldAt)||!Finite(rsv.lastReleaseAt)||counter.squads.size()>3||!Finite(counter.warnedAt)||!Time(counter.readyAt))return Bad("Invalid mechanism bounds");if(f.phase!=FirePhase::Idle&&!Node(*m,f.node))return Bad("Unknown fire node");for(const auto& a:rsv.axes)if(!Node(*m,a))return Bad("Unknown reserve axis");if(counter.pending&&!Node(*m,counter.node))return Bad("Unknown counter node");}std::set<std::string> receipts;for(const auto& a:st.receipts)if(!Id(a.id)||a.payload.empty()||a.side<0||a.side>1||a.sequence==0||!receipts.insert(a.id).second)return Bad("Invalid command receipt journal");
const auto& clock=st.latestClock;
if(clock.receipt.id.empty()){
    if(!clock.receipt.payload.empty()||clock.receipt.sequence!=0||clock.receipt.side!=-1||clock.capturedElapsed!=0||clock.hostFrame!=0)return Bad("Invalid empty clock watermark");
}else if(!Id(clock.receipt.id)||clock.receipt.payload.empty()||clock.receipt.side!=st.playerSide||clock.receipt.sequence==0||clock.receipt.sequence>=st.nextSequence[st.playerSide]||!Time(clock.capturedElapsed)||clock.capturedElapsed>st.elapsed||clock.hostFrame==std::numeric_limits<std::uint64_t>::max()||receipts.count(clock.receipt.id)||clock.receipt.payload!=ClockPayload(st,clock))return Bad("Invalid persistent clock watermark or noncanonical proof");
return Good();}
Result ValidateSnapshot(const State& st,const Snapshot& s){auto valid=ValidateState(st);if(!valid)return valid;const auto* m=FindMission(st.missionId);const auto* g=scene::ForMission(st.missionId);if(!g||s.missionId!=st.missionId||s.encounterId!=st.encounterId||s.sceneId!=m->sceneId||s.sceneRevision!=m->sceneRevision||s.stateRevision!=st.revision||!Time(s.elapsed)||s.elapsed<st.elapsed||!s.geometryReady||!s.hostOrdersAtomic||!s.fighting||s.paused)return Bad("Snapshot identity/revision/clock/host capability mismatch");if(s.nations[st.playerSide]!=m->player||s.nations[1-st.playerSide]!=m->enemy||(s.directions[0]!=1&&s.directions[0]!=-1)||s.directions[0]!=-s.directions[1])return Bad("Side and explicit national identity disagree");if(s.squads.size()>4096||s.observations.size()>8192||s.movements.size()>8192||s.supportReceipts.size()>1024||s.nodes.size()!=m->nodes.size())return Bad("Snapshot collection bounds");std::set<int> ids;for(const auto& q:s.squads){const auto* role=q.side>=0&&q.side<2?RoleFor(s.nations[q.side],q.roleId,m->date):nullptr;if(q.id<0||!ids.insert(q.id).second||!role||q.living<0||q.living>role->size||!PointOK(q.position)||!scene::Contains(*g,q.position)||(q.side==st.playerSide&&!st.fullArmoryReplay&&!RoleAllowed(*wfexp::FindMission(st.missionId),q.roleId)))return Bad("Invalid squad identity/role/date/position");if(q.living>0&&q.entered){auto footing=scene::FootingAt(*g,q.position.x,q.position.y);if(!footing.valid||!footing.walkable||std::abs(q.position.z-footing.heightCm)>32)return Bad("Living squad feet disagree with shared physical terrain");}if(!q.nodeId.empty()){const auto* n=Node(*m,q.nodeId);if(!n||!At(q,*n))return Bad("Claimed squad node is outside actual radius");}}std::set<std::string> nodes;for(const auto& n:s.nodes)if(!Node(*m,n.id)||!nodes.insert(n.id).second||n.owner< -1||n.owner>1||n.contestedSidesMask>3)return Bad("Invalid node snapshot");for(int side=0;side<2;++side)for(int i=0;i<10;++i)if(!Time(s.supportReadyAt[side][i])||s.supportRoundBudget[side][i]<0||s.supportRoundBudget[side][i]>10000||s.supportSalvoCount[side][i]<0||s.supportSalvoCount[side][i]>4||(s.supportRoundBudget[side][i]>0&&s.supportSalvoCount[side][i]==0))return Bad("Invalid support readiness/budget");for(const auto& o:s.observations)if(!Squad(s,o.observerSquad)||!Node(*m,o.targetNode)||!Time(o.observedAt)||o.observedAt>s.elapsed||!Finite(o.smokeTransmission)||o.smokeTransmission<0||o.smokeTransmission>1||!PointOK(o.eye))return Bad("Invalid observation record");std::set<std::string> moves;std::set<std::pair<int,std::uint64_t>> arrivalSequences;for(const auto& a:s.movements){const auto* e=Edge(*m,a.edgeId);const auto* q=Squad(s,a.squadId);if(!Id(a.id)||!moves.insert(a.id).second||!arrivalSequences.emplace(a.squadId,a.sequence).second||a.encounterId!=st.encounterId||!e||!q||a.sequence==0||!Time(a.arrivedAt)||a.arrivedAt>s.elapsed||!scene::CanTraverseRoute(*g,e->id,Node(*m,e->from)->position,Node(*m,e->to)->position))return Bad("Invalid physical movement receipt");}for(const auto& a:s.supportReceipts)if(a.encounterId!=st.encounterId||a.side<0||a.side>1||a.planId!=st.fire[a.side].planId||st.fire[a.side].phase!=FirePhase::Committed||a.resolvedSalvos<0||a.resolvedSalvos>4||!Time(a.resolvedAt)||a.resolvedAt<st.fire[a.side].commitAt||a.resolvedAt>s.elapsed)return Bad("Invalid actual support phase receipt");return Good();}
std::string StateSignature(const State& st){Writer w;w.Add(st.rulesVersion);w.Add(st.fullArmoryReplay);w.Add(st.playerSide);w.Add(st.missionId);w.Add(st.encounterId);w.Add(st.revision);w.Add(st.elapsed);CampaignWrite(w,st.campaign);for(int side=0;side<2;++side){w.Add(st.nextSequence[side]);const auto& f=st.fire[side];w.Add(static_cast<int>(f.phase));w.Add(f.node);w.Add(f.alternateNode);w.Add(f.observerNode);w.Add(f.planId);w.Add(f.supportId);w.Add(f.observerSquad);w.Add(f.secondaryObserver);w.Add(f.roundBudget);w.Add(f.observationStart);w.Add(f.lastObservation);w.Add(f.commitAt);w.Add(f.commitDelay);w.Add(f.observationRequired);w.Add(f.stencil);w.Add(f.followupPhase);w.Add(f.followupUsed);w.Add(f.waitReload);w.Add(f.dualObservation);w.List(f.followupSquads);w.List(f.reloadSquads);const auto& r=st.reserve[side];w.List(r.squads);w.List(r.axes);w.Add(r.heldAt);w.Add(r.lastReleaseAt);w.Add(r.released);w.Add(r.waitReload);w.Add(r.amended);const auto& c=st.counter[side];w.List(c.squads);w.Add(c.node);w.Add(c.warnedAt);w.Add(c.readyAt);w.Add(c.pending);}const auto& o=st.objective;w.Add(o.index);w.Add(o.activatedAt);w.Add(o.lastElapsed);w.Add(o.progress);w.Add(o.deadline);w.Add(o.fallback);w.Add(o.fallbackArrived);w.Add(o.finished);w.Add(o.success);w.Add(o.boundObstacle);w.List(o.withdrawalParticipants);w.List(o.holdParticipants);w.List(o.fallbackWorkers);w.Add(o.arrivalSequences.size());for(const auto& a:o.arrivalSequences){w.Add(a.first);w.Add(a.second);}w.List(o.completedStages);w.List(o.clearedObstacles);w.List(o.usedMovementReceipts);w.Add(st.choices.size());for(const auto& a:st.choices){w.Add(a.storyId);w.Add(a.choiceId);}w.Add(st.receipts.size());for(const auto& a:st.receipts){w.Add(a.id);w.Add(a.payload);w.Add(a.sequence);w.Add(a.side);}w.Add(st.latestClock.receipt.id);w.Add(st.latestClock.receipt.payload);w.Add(st.latestClock.receipt.sequence);w.Add(st.latestClock.receipt.side);w.Add(st.latestClock.capturedElapsed);w.Add(st.latestClock.hostFrame);w.Add(st.approach);w.Add(st.followupPolicy);w.Add(st.withdrawPolicy);w.Add(st.followupPolicyActive);w.Add(st.withdrawPolicyActive);w.Add(st.arrivalHolds.size());for(const auto& h:st.arrivalHolds){w.Add(h.first);w.Add(h.second);}w.Add(st.clearanceTargets.size());for(const auto& a:st.clearanceTargets){w.Add(a.first);w.Add(a.second);}return w.out.str();}
Prepared Prepare(const State& st,const Snapshot& s,const Command& c){
    Prepared p;p.command_=c;auto invalid=[&](const std::string& why){p.decision_.reason=why;return p;};
    auto r=ValidateSnapshot(st,s);if(!r)return invalid(r.reason);
    const bool clock=c.kind==CommandKind::AdvanceTime;
    if(!Id(c.receiptId)||c.missionId!=st.missionId||c.encounterId!=st.encounterId||c.side<0||c.side>1||c.variant<0||c.variant>1||c.supportId<0||c.supportId>=10||c.squadIds.size()>3||static_cast<int>(c.kind)>static_cast<int>(CommandKind::AdvanceToObjective)||(!clock&&c.hostFrame!=0)||(clock&&c.side!=st.playerSide)||c.hostFrame==std::numeric_limits<std::uint64_t>::max())return invalid("Invalid command envelope");
    if(clock&&(c.supportId!=0||c.variant!=0||!c.targetNode.empty()||!c.alternateNode.empty()||!c.optionId.empty()||!c.storyId.empty()||!c.choiceId.empty()||!c.squadIds.empty()))return invalid("Clock command contains unrelated gameplay fields");
    if(clock&&st.latestClock.hostFrame!=0&&c.hostFrame==0)return invalid("Legacy zero frame cannot erase established host watermark");
    std::set<int> ids;for(int id:c.squadIds)if(id<0||!ids.insert(id).second)return invalid("Duplicate/invalid real squad IDs");
    auto payload=Payload(c);if(clock){ClockReceipt proof;proof.receipt={c.receiptId,{},c.sequence,c.side};proof.hostFrame=c.hostFrame;proof.capturedElapsed=s.elapsed;payload=ClockPayload(st,proof);}
    const Receipt* replay=nullptr;
    if(st.latestClock.receipt.id==c.receiptId&&!c.receiptId.empty()){
        if(!clock||st.latestClock.capturedElapsed!=s.elapsed)return invalid("Clock replay changed kind or captured elapsed");
        replay=&st.latestClock.receipt;
    }
    for(const auto& receipt:st.receipts)if(receipt.id==c.receiptId){replay=&receipt;break;}
    if(replay){
        if(replay->payload!=payload)return invalid("Receipt replay has a different payload");
        p.decision_.allowed=true;p.decision_.duplicate=true;p.sourceState_=StateSignature(st);p.sourceSnapshot_=SnapshotSignature(s);return p;
    }
    if(clock&&c.hostFrame!=0&&st.latestClock.hostFrame!=0&&c.hostFrame<=st.latestClock.hostFrame)return invalid("Stale host frame watermark");
    if(c.expectedRevision!=st.revision||c.sequence!=st.nextSequence[c.side]||(!clock&&st.receipts.size()>=200000))return invalid("Stale, reordered or exhausted command journal");
    p.candidate_=st;r=Reduce(p.candidate_,s,c,p.decision_.intents);
    if(!r){p.decision_.intents.clear();return invalid(r.reason);}
    for(const auto& intent:p.decision_.intents)if(intent.kind==IntentKind::AdvanceExistingSquad){auto& holds=p.candidate_.arrivalHolds;holds.erase(std::remove_if(holds.begin(),holds.end(),[&](const auto& h){return h.first==intent.squadId&&h.second!=intent.targetNode;}),holds.end());}
    p.candidate_.elapsed=s.elapsed;++p.candidate_.revision;++p.candidate_.nextSequence[c.side];
    if(clock)p.candidate_.latestClock={{c.receiptId,std::move(payload),c.sequence,c.side},s.elapsed,c.hostFrame};
    else p.candidate_.receipts.push_back({c.receiptId,std::move(payload),c.sequence,c.side});
    r=ValidateState(p.candidate_);if(!r){p.decision_.intents.clear();return invalid(r.reason);}
    p.sourceState_=StateSignature(st);p.sourceSnapshot_=SnapshotSignature(s);p.decision_.allowed=true;return p;
}
Decision Evaluate(const State& st,const Snapshot& s,const Command& c){return Prepare(st,s,c).View();}
Result Acknowledge(State& st,const Snapshot& original,const Prepared& p,const std::vector<std::string>& successful){if(!p.decision_.allowed)return Bad("Cannot acknowledge rejected transaction");if(StateSignature(st)!=p.sourceState_||SnapshotSignature(original)!=p.sourceSnapshot_)return Bad("Atomic context changed between prepare and acknowledgement");std::set<std::string> ids(successful.begin(),successful.end());if(ids.size()!=successful.size()||ids.size()!=p.decision_.intents.size())return Bad("Host batch did not acknowledge every intent exactly once");for(const auto& i:p.decision_.intents)if(!ids.count(i.id))return Bad("Missing host intent acknowledgement");if(!p.decision_.duplicate)st=p.candidate_;return Good();}
} // namespace wfexp::tactical
