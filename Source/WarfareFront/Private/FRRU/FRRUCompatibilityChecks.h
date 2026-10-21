#pragma once
#include "FRRUHostContract.h"
#include <cstdio>
#include <limits>

namespace wfexp::hostv81 {
struct CheckReport {
    int checks=0;
    std::vector<std::string> failures;
    void Check(bool value,const std::string& label) {++checks;if(!value)failures.push_back(label);}
    void OK(const Result& result,const std::string& label) {Check(result.ok,label+": "+result.reason);}
    bool Passed()const {return failures.empty();}
};
struct PilotFixture {
    tactical::State state;
    tactical::Snapshot snapshot;
    const tactical::MissionDefinition* definition=nullptr;
    int receipt=0;
    bool Init(const char* missionId,int side,CheckReport& report,bool buyOptions=false) {
        definition=tactical::FindMission(missionId);const auto* row=wfexp::FindMission(missionId);
        report.Check(definition&&row,"Fixture stable mission exists");if(!definition||!row)return false;
        tactical::Campaign campaign;const auto created=tactical::NewTacticalCampaign(row->faction,campaign);report.OK(created,"New TacticalV2 campaign");if(!created)return false;
        for(const auto& prior:wfexp::Missions())if(prior.faction==row->faction&&prior.order<row->order){
            char token[64];std::snprintf(token,sizeof(token),"00000000-0000-0000-0000-%012d",prior.order+1);
            report.OK(SelectMission(campaign.progression,prior.id),"Select prior stable mission");report.OK(BeginEncounter(campaign.progression,prior.id,token),"Begin prior encounter");report.OK(RecordVictory(campaign.progression,prior.id,token),"Prior completion");
        }
        if(buyOptions){campaign.progression.researchPoints=100;for(const auto& option:tactical::Options())if(option.faction==row->faction&&option.index<12)report.OK(tactical::PurchaseOption(campaign,option.id),"Purchase exclusive active tactical option");}
        const auto encounter=tactical::NewEncounter(campaign,missionId,"11111111-2222-3333-4444-555555555555",side,0,state);report.OK(encounter,"New scoped encounter");if(!encounter)return false;
        snapshot.missionId=missionId;snapshot.encounterId=state.encounterId;snapshot.sceneId=definition->sceneId;snapshot.sceneRevision=definition->sceneRevision;snapshot.geometryReady=true;snapshot.hostOrdersAtomic=true;snapshot.nations[side]=row->faction;snapshot.nations[1-side]=row->enemy;
        for(const auto& node:definition->nodes)snapshot.nodes.push_back({node.id,node.id=="enemy_assembly"?1-side:side,false});
        for(auto& budget:snapshot.supportRoundBudget)budget.fill(12);for(auto& salvos:snapshot.supportSalvoCount)salvos.fill(4);
        Add(10,0,"assembly",side);Add(12,RoleAllowed(*row,3)?3:6,"relay",side);Add(14,2,"home",side);
        const auto valid=tactical::ValidateSnapshot(state,snapshot);report.OK(valid,"Actual shared-ground snapshot");return valid.ok;
    }
    const tactical::NodeDefinition* Node(std::string_view id)const {for(const auto& node:definition->nodes)if(node.id==id)return &node;return nullptr;}
    void Add(int id,int role,const char* nodeId,int side){const auto* node=Node(nodeId);const auto* roleDef=RoleFor(snapshot.nations[side],role,definition->date);if(!node||!roleDef)return;tactical::SquadSnapshot squad;squad.id=id;squad.roleId=role;squad.side=side;squad.living=roleDef->size;squad.entered=true;squad.stationary=true;squad.nodeId=nodeId;squad.position=node->position;snapshot.squads.push_back(squad);}
    tactical::SquadSnapshot* Squad(int id){for(auto& squad:snapshot.squads)if(squad.id==id)return &squad;return nullptr;}
    void Time(double elapsed){snapshot.elapsed=elapsed;snapshot.stateRevision=state.revision;}
    tactical::Command Command(tactical::CommandKind kind,int side=-1){if(side<0)side=state.playerSide;tactical::Command command;command.kind=kind;command.side=side;command.missionId=state.missionId;command.encounterId=state.encounterId;command.expectedRevision=state.revision;command.sequence=state.nextSequence[side];command.receiptId="pilot."+std::to_string(++receipt);return command;}
    tactical::Decision Apply(const tactical::Command& command,CheckReport& report){snapshot.stateRevision=state.revision;const auto prepared=tactical::Prepare(state,snapshot,command);report.Check(prepared.View().allowed,"Prepare command "+std::to_string(static_cast<int>(command.kind))+": "+prepared.View().reason);if(!prepared.View().allowed)return prepared.View();std::vector<std::string> ids;for(const auto& intent:prepared.View().intents)ids.push_back(intent.id);report.OK(tactical::Acknowledge(state,snapshot,prepared,ids),"Acknowledge complete mock host batch");snapshot.stateRevision=state.revision;return prepared.View();}
    void Reject(const tactical::Command& command,CheckReport& report,const char* label){const auto before=tactical::StateSignature(state);report.Check(!tactical::Evaluate(state,snapshot,command).allowed,label);report.Check(tactical::StateSignature(state)==before,"Refused command preserves reducer");}
    void Proof(int id,const char* target){auto* squad=Squad(id);const auto* node=Node(target);if(!squad||!node)return;squad->nodeId=target;squad->position=node->position;auto eye=node->position;eye.z+=180;snapshot.observations.clear();snapshot.observations.push_back({id,target,snapshot.elapsed,1,eye,true});}
};
struct TestAtomicHost final:tactical::AtomicHostBatch {
    bool rejectPreflight=false,rejectCommit=false;
    int commits=0;
    Result Preflight(const tactical::Snapshot&,const std::vector<tactical::Intent>&)override {return {!rejectPreflight,"Fixture preflight refusal"};}
    Result Commit(const tactical::Snapshot&,const std::vector<tactical::Intent>&)override {if(rejectCommit)return {false,"Fixture atomic refusal"};++commits;return {true,{}};}
};

inline CheckReport RunNativeCompatibilityChecks() {
    CheckReport report;
    report.Check(!Enabled,"Expansion remains gated");
    report.Check(static_cast<int>(Faction::British)==0&&static_cast<int>(Faction::German)==1,"Legacy national ordinals unchanged");
    report.Check(!IsSelectable(Faction::AustroHungarian),"Austria-Hungary cannot be a player campaign");
    tactical::Campaign invalid;report.Check(!tactical::NewTacticalCampaign(Faction::AustroHungarian,invalid),"AH campaign creation refused");
    std::set<std::string> missionIds;
    for(const auto& mission:wfexp::Missions()){
        report.Check(missionIds.insert(mission.id).second,"Stable mission ID unique");
        for(int side=0;side<2;++side)for(int direction:{-1,1}){
            Identity identity;report.OK(BindIdentity(mission.id,side,direction,identity),"Bind explicit encounter identity");
            report.Check(identity.nations[side]==mission.faction&&identity.nations[1-side]==mission.enemy&&identity.directions[side]==direction&&identity.directions[1-side]==-direction,"Nationality independent from side and facing");
            report.Check(identity.calendarDate==mission.date&&identity.sceneId==tactical::FindMission(mission.id)->sceneId,"Exact date and stable physical scene preserved");
            const Weapon* weapon=nullptr;report.OK(ResolveEquipment(identity,side,0,0,weapon),"Era-safe primary service rifle");
            report.Check(weapon&&weapon->faction==mission.faction,"Primary weapon nationality is explicit");
            const Weapon* untouched=weapon;report.Check(!ResolveEquipment(identity,2,0,0,weapon)&&weapon==untouched,"Faction cannot be used as team array index");
            identity.calendarDate=19130101;report.Check(!ResolveEquipment(identity,side,0,0,weapon)&&weapon==untouched,"Date mutation cannot bypass mission eligibility");
        }
    }
    Identity identity;report.Check(!BindIdentity("fr.missing",0,1,identity),"Unknown mission refused");report.Check(!BindIdentity("ru.galicia",3,1,identity),"Nation ordinal is not a side");report.Check(!BindIdentity("ru.galicia",0,0,identity),"Direction only plus/minus one");
    report.Check(RoleFor(Faction::French,7,19160831)==nullptr&&RoleFor(Faction::French,7,19160901)!=nullptr,"VB rifle-grenade date gate exact");
    report.Check(RoleFor(Faction::Russian,9,19160101)==nullptr,"Russian tank/flamethrower fallback not borrowed from Germany");
    for(const auto& vehicle:Vehicles())report.Check(!vehicle.assetsReady&&!vehicle.deploymentVerified,"Vehicle art never opens gameplay deployment gate");
    for(Faction nation:{Faction::French,Faction::Russian}){
        tactical::Campaign campaign;report.OK(tactical::NewTacticalCampaign(nation,campaign),"Exclusive TacticalV2 profile");float result=-1;report.OK(RetainHostHitChance(campaign,.4f,result),"Retain host final hit probability");report.Check(result==.4f,"No second firearm accuracy multiplier");
        campaign.profile=tactical::Profile::ClassicV1;report.Check(!RetainHostHitChance(campaign,.4f,result),"Classic and TacticalV2 may not stack");campaign.profile=tactical::Profile::TacticalV2;campaign.progression.technologyMask=1;report.Check(!RetainHostHitChance(campaign,.4f,result),"Classic percentage mask refused");campaign.progression.technologyMask=0;report.Check(!RetainHostHitChance(campaign,std::numeric_limits<float>::quiet_NaN(),result),"Nonfinite hit probability refused");
    }
    {
        PilotFixture fixture;if(fixture.Init("fr.mort_homme",0,report)){
            auto command=fixture.Command(tactical::CommandKind::PlanFire);command.targetNode="ridge";fixture.Apply(command,report);
            for(int elapsed=0;elapsed<=4;++elapsed){fixture.Time(elapsed);fixture.Proof(12,"ridge");command=fixture.Command(tactical::CommandKind::Observe);command.squadIds={12};fixture.Apply(command,report);report.Check((fixture.state.fire[0].phase==tactical::FirePhase::Observed)==(elapsed==4),"Observation amendment requires four uninterrupted host seconds");if(elapsed==3){command=fixture.Command(tactical::CommandKind::CommitFire);fixture.Reject(command,report,"Three seconds cannot commit amended observation");}}
            command=fixture.Command(tactical::CommandKind::CommitFire);const auto prepared=tactical::Prepare(fixture.state,fixture.snapshot,command);report.Check(prepared.View().allowed&&prepared.View().intents.size()==1,"Exactly one artillery entitlement requested");
            if(prepared.View().intents.size()==1){const auto intent=prepared.View().intents[0];report.Check(intent.roundBudget==12&&intent.salvos==4,"Four salvos retain original 12-shell single-call budget");
                auto changed=fixture.snapshot;changed.supportRoundBudget[0][0]=16;report.Check(!tactical::Acknowledge(fixture.state,changed,prepared,{intent.id}),"Changed entitlement rejects prior prepared batch");
                SupportJournal journal;HostPlan plan;plan.planId=intent.id;plan.encounterId=fixture.snapshot.encounterId;plan.side=0;plan.hostFireMissionId=77;plan.roundBudget=12;plan.committedAt=fixture.snapshot.elapsed;
                const auto committedSnapshot=fixture.snapshot;
                for(int salvo=0;salvo<4;++salvo)for(int round=0;round<3;++round)plan.rounds.push_back({{salvo,round},10.0+salvo*2+round*.1});
                report.OK(journal.Register(fixture.snapshot,intent,plan),"Register exact actual host shell manifest");report.Check(!journal.Register(fixture.snapshot,intent,plan),"No duplicate support reservation");
                auto overspend=plan;overspend.planId="overspend";overspend.roundBudget=13;report.Check(!journal.Register(fixture.snapshot,intent,overspend),"Extra shells refused");
                tactical::SupportReceipt receipt;bool emitted=false;report.Check(!journal.Resolve(plan.planId,plan.encounterId,0,77,{0,0},20,false,receipt,emitted),"Timer alone never resolves artillery");
                report.Check(!journal.Resolve(plan.planId,plan.encounterId,0,77,{0,0},5,true,receipt,emitted),"Before-impact fake receipt refused");
                report.OK(tactical::Acknowledge(fixture.state,fixture.snapshot,prepared,{intent.id}),"Commit one mock host batch");fixture.snapshot.stateRevision=fixture.state.revision;
                command=fixture.Command(tactical::CommandKind::Followup);command.squadIds={14};fixture.Reject(command,report,"No actual salvo means no follow-on order");
                int published=0;for(int salvo=0;salvo<4;++salvo)for(int round=0;round<3;++round){report.OK(journal.Resolve(plan.planId,plan.encounterId,0,77,{salvo,round},20+salvo,true,receipt,emitted),"Actual shell resolved");report.Check(emitted==(round==2),"Only all actual shells complete a salvo");if(emitted){++published;report.Check(receipt.resolvedSalvos==salvo+1,"Contiguous salvo receipt phase");fixture.snapshot.supportReceipts.push_back(receipt);report.OK(journal.Acknowledge(receipt),"Consumer confirms delivery after retaining proof");}}
                report.Check(published==4,"Four actual completion receipts");report.OK(journal.Resolve(plan.planId,plan.encounterId,0,77,{3,2},23,true,receipt,emitted),"Duplicate shell observation harmless");report.Check(!emitted,"Duplicate does not publish more salvos");
                fixture.Time(23);fixture.Proof(12,"relay");command=fixture.Command(tactical::CommandKind::Followup);command.squadIds={14};const auto follow=fixture.Apply(command,report);report.Check(follow.intents.size()==1&&follow.intents[0].squadId==14,"Follow-on orders existing squad only");
                SupportJournal retry;report.OK(retry.Register(committedSnapshot,intent,plan),"Durable retry fixture plan");
                for(int salvo:{3,1,0,2})for(int round=2;round>=0;--round)report.OK(retry.Resolve(plan.planId,plan.encounterId,0,77,{salvo,round},10+salvo*2+round*.1,true,receipt,emitted),"Out-of-order actual event timestamps retained");
                std::vector<tactical::SupportReceipt> pending,active;report.OK(retry.PendingReceipts(plan.planId,plan.encounterId,pending),"Query durable pending receipts");report.Check(pending.size()==4,"Consumer refusal retains all four phase proofs");
                report.OK(retry.PendingReceipts(plan.planId,plan.encounterId,active),"Query after rejected/unacknowledged consumer");report.Check(active.size()==4&&active[1].resolvedAt==12.2,"Retry does not refresh second-salvo completion clock");
                if(pending.size()==4){auto wrong=pending[1];wrong.resolvedAt+=1;report.Check(!retry.Acknowledge(wrong),"Mutated timestamp acknowledgement refused");wrong=pending[1];wrong.side=1;report.Check(!retry.Acknowledge(wrong),"Wrong side acknowledgement refused");report.OK(retry.Acknowledge(pending[3]),"Out-of-order phase acknowledgement");report.OK(retry.Acknowledge(pending[1]),"Second phase acknowledged");report.OK(retry.Acknowledge(pending[1]),"Repeated acknowledgement harmless");}
                report.OK(retry.ActiveReceipts(plan.planId,plan.encounterId,active),"Active proof retained after delivery acknowledgement");report.Check(active.size()==4&&active[1].resolvedAt==12.2,"Acknowledgement preserves actual phase-two proof");
                report.OK(retry.Resolve(plan.planId,plan.encounterId,0,77,{1,2},100,true,receipt,emitted),"Late duplicate actual shell observation");report.OK(retry.ActiveReceipts(plan.planId,plan.encounterId,active),"Query after late duplicate");report.Check(active.size()==4&&active[1].resolvedAt==12.2,"Duplicate cannot extend eight-second follow-on window");
                auto secondPlan=plan;secondPlan.planId="second.plan";secondPlan.hostFireMissionId=78;secondPlan.committedAt=fixture.snapshot.elapsed;for(auto& round:secondPlan.rounds)round.impactAt=30+round.key.salvo*2+round.key.round*.1;auto secondIntent=intent;secondIntent.id=secondPlan.planId;secondIntent.notBefore=fixture.snapshot.elapsed;report.OK(retry.Register(fixture.snapshot,secondIntent,secondPlan),"Second host fire mission has its own durable journal");report.OK(retry.ActiveReceipts(secondPlan.planId,secondPlan.encounterId,active),"Second-plan active proof query");report.Check(active.empty(),"Old plan proof cannot satisfy next plan");retry.RetirePlan(plan.planId,plan.encounterId);report.Check(!retry.ActiveReceipts(plan.planId,plan.encounterId,active),"Retired plan proof no longer exposed");report.Check(!retry.Resolve(plan.planId,plan.encounterId,0,77,{1,2},101,true,receipt,emitted),"Retired-plan callback refused");
                journal.RetireEncounter(plan.encounterId);report.Check(journal.RegisteredPlans()==0,"Battle end retires journal");report.Check(!journal.Resolve(plan.planId,plan.encounterId,0,77,{3,2},24,true,receipt,emitted),"Late strike cannot reward restarted encounter");
            }
        }
    }
    {
        PilotFixture fixture;if(fixture.Init("ru.galicia",1,report)){
            auto command=fixture.Command(tactical::CommandKind::RegisterReserve);command.targetNode="ridge";command.squadIds={10};fixture.Apply(command,report);const auto count=fixture.snapshot.squads.size();fixture.Time(3);command=fixture.Command(tactical::CommandKind::ReleaseReserve);fixture.Reject(command,report,"Reserve needs four actual host seconds");fixture.Time(4);command=fixture.Command(tactical::CommandKind::ReleaseReserve);const auto release=fixture.Apply(command,report);report.Check(release.intents.size()==1&&release.intents[0].squadId==10&&fixture.snapshot.squads.size()==count,"Reserve release never recruits or clones");
            fixture.Add(90,0,"enemy_assembly",0);command=fixture.Command(tactical::CommandKind::CounterOrder,0);command.targetNode="objective";command.squadIds={90};fixture.Apply(command,report);command=fixture.Command(tactical::CommandKind::AdvanceCounter,0);fixture.Reject(command,report,"AH must display four-second warning");fixture.Time(8);command=fixture.Command(tactical::CommandKind::AdvanceCounter,0);fixture.Apply(command,report);
        }
    }
    {
        PilotFixture fixture;if(fixture.Init("ru.galicia",0,report)){
            HostFrameBridge clock;TestAtomicHost host;bool advanced=false;const auto before=tactical::StateSignature(fixture.state);
            fixture.snapshot.paused=true;report.OK(clock.Publish(fixture.state,fixture.snapshot,{1,0,true,true},host,advanced),"Paused host frame");report.Check(!advanced&&tactical::StateSignature(fixture.state)==before&&host.commits==0,"Pause does not advance reducer or host orders");
            fixture.snapshot.paused=false;fixture.Time(1);host.rejectPreflight=true;report.Check(!clock.Publish(fixture.state,fixture.snapshot,{2,1,true,false},host,advanced),"Atomic preflight refusal");report.Check(tactical::StateSignature(fixture.state)==before&&host.commits==0,"Failed preflight has no side effects");host.rejectPreflight=false;
            report.OK(clock.Publish(fixture.state,fixture.snapshot,{2,1,true,false},host,advanced),"Same host frame can retry refused preflight");report.Check(advanced&&fixture.state.elapsed==1&&host.commits==1,"One host elapsed drives reducer");fixture.snapshot.stateRevision=fixture.state.revision;const auto completed=tactical::StateSignature(fixture.state);
            report.OK(clock.Publish(fixture.state,fixture.snapshot,{2,1,true,false},host,advanced),"Duplicate host frame");report.Check(!advanced&&host.commits==1&&tactical::StateSignature(fixture.state)==completed,"Repeated frame cannot duplicate time/orders");
            report.Check(!clock.Publish(fixture.state,fixture.snapshot,{3,2,true,false},host,advanced),"Alternate elapsed/snapshot clock refused");fixture.Time(.5);report.Check(!clock.Publish(fixture.state,fixture.snapshot,{3,.5,true,false},host,advanced),"Backwards host elapsed refused");
            fixture.Time(1.5);host.rejectCommit=true;report.Check(!clock.Publish(fixture.state,fixture.snapshot,{3,1.5,true,false},host,advanced),"Atomic commit refusal");report.Check(tactical::StateSignature(fixture.state)==completed&&host.commits==1,"Commit refusal preserves pilot context");host.rejectCommit=false;
            report.OK(clock.Publish(fixture.state,fixture.snapshot,{3,1.5,true,false},host,advanced),"Half-speed host clock sample");report.Check(fixture.state.elapsed==1.5,"No additional native scheduler");fixture.Time(3.5);report.OK(clock.Publish(fixture.state,fixture.snapshot,{4,3.5,true,false},host,advanced),"Double-speed host clock sample");report.Check(fixture.state.elapsed==3.5,"Speed policy remains owned by host elapsed");
            report.Check(fixture.state.receipts.empty()&&!fixture.state.latestClock.receipt.id.empty(),"AdvanceTime uses one bounded persistent slot");
            const auto saved=fixture.state;HostFrameBridge restored;const auto stable=tactical::StateSignature(saved);report.OK(restored.Publish(fixture.state,fixture.snapshot,{4,3.5,true,false},host,advanced),"Bridge recovers committed frame watermark from State");report.Check(!advanced&&tactical::StateSignature(fixture.state)==stable,"Saved clock replay cannot repeat a commit");
            auto replay=fixture.Command(tactical::CommandKind::AdvanceTime);replay.hostFrame=4;replay.receiptId=fixture.state.latestClock.receipt.id;fixture.Time(4);fixture.Reject(replay,report,"Same clock token with different elapsed refused");
        }
    }
    {
        PilotFixture fixture;if(fixture.Init("ru.galicia",0,report)){
            fixture.Time(.25);auto legacy=fixture.Command(tactical::CommandKind::AdvanceTime);fixture.Apply(legacy,report);report.Check(fixture.state.latestClock.hostFrame==0,"Initial producer-style zero-frame clock remains legal");
            HostFrameBridge bridge;TestAtomicHost host;bool advanced=false;fixture.Time(1);report.OK(bridge.Publish(fixture.state,fixture.snapshot,{10,1,true,false},host,advanced),"Legacy clock upgrades to explicit host frame");
            legacy=fixture.Command(tactical::CommandKind::AdvanceTime);fixture.Reject(legacy,report,"Zero-frame command cannot erase nonzero watermark");
            HostFrameBridge fresh;const auto before=tactical::StateSignature(fixture.state);report.Check(!fresh.Publish(fixture.state,fixture.snapshot,{9,1,true,false},host,advanced)&&tactical::StateSignature(fixture.state)==before,"Fresh adapter still rejects an older frame after legacy refusal");
            fixture.Time(2);report.OK(fresh.Publish(fixture.state,fixture.snapshot,{11,2,true,false},host,advanced),"Later real frame remains legal after refusal");
            auto corrupted=fixture.state;++corrupted.latestClock.hostFrame;report.Check(!tactical::ValidateState(corrupted),"Frame field must match canonical clock payload");
            corrupted=fixture.state;--corrupted.latestClock.receipt.sequence;report.Check(!tactical::ValidateState(corrupted),"Otherwise in-range sequence must match clock payload");
            corrupted=fixture.state;corrupted.latestClock.receipt.payload="x";report.Check(!tactical::ValidateState(corrupted),"Opaque nonempty payload is not a clock proof");
            corrupted=fixture.state;corrupted.latestClock.capturedElapsed=1.5;report.Check(!tactical::ValidateState(corrupted),"Captured time participates in canonical clock proof");
            corrupted=fixture.state;corrupted.latestClock.receipt.id="changed.receipt";report.Check(!tactical::ValidateState(corrupted),"Receipt ID must match canonical proof");
            corrupted=fixture.state;corrupted.latestClock.receipt.side=1;report.Check(!tactical::ValidateState(corrupted),"Clock side cannot be replaced independently");
            fixture.Time(2);auto replay=fixture.Command(tactical::CommandKind::AdvanceTime);replay.receiptId=fixture.state.latestClock.receipt.id;replay.sequence=fixture.state.latestClock.receipt.sequence;replay.hostFrame=fixture.state.latestClock.hostFrame;const auto duplicate=tactical::Evaluate(fixture.state,fixture.snapshot,replay);report.Check(duplicate.allowed&&duplicate.duplicate,"Canonical exact clock replay remains idempotent");
        }
    }
    return report;
}
} // namespace wfexp::hostv81
