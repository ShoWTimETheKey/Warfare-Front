#include "FRRUCompatibilityChecks.h"
#include <array>
#include <fstream>
#include <iostream>
#include <limits>
#include <utility>

using namespace wfexp;
using namespace wfexp::tactical;
using wfexp::hostv81::CheckReport;
using wfexp::hostv81::PilotFixture;
namespace {
void OptionCosts(CheckReport& report)
{
    constexpr std::array<int,12> french{{0,1,1,1,2,0,2,0,0,2,2,2}};
    constexpr std::array<int,12> russian{{0,1,1,1,2,2,2,0,2,2,2,2}};
    report.Check(Options().size()==30,"Existing 24 active identities plus six distinct passive armour identities");
    for(const auto& o:Options()){
        const auto* found=FindOption(o.id);
        report.Check(found==&o,"List and purchase lookup share one authoritative option: "+o.id);
        const auto& expected=o.faction==Faction::French?french:russian;
        if(o.index<12)report.Check(o.index>=0&&o.cost==expected[static_cast<std::size_t>(o.index)],"Unchanged active receiver cost: "+o.id);
        else report.Check(o.index>=12&&o.index<=14&&o.cost==(o.index==14?3:2)&&o.legacyBit==-1&&o.command=="PassiveArmour","Passive armour option has a real research cost and no Classic bit: "+o.id);
    }
    Campaign fr;report.OK(NewTacticalCampaign(Faction::French,fr),"French campaign for pricing");fr.progression.researchPoints=0;
    report.OK(PurchaseOption(fr,"fr.tech.0"),"Basic hold can be registered without research points");
    report.Check(fr.progression.researchPoints==0&&fr.purchasedOptions.size()==1,"Free order records its purchase marker without debit");
    report.OK(PurchaseOption(fr,"fr.tech.0"),"Free order purchase is idempotent");
    report.Check(fr.purchasedOptions.size()==1,"Repeated free purchase adds no second marker");
    const auto before=fr.purchasedOptions;
    report.Check(!PurchaseOption(fr,"fr.tech.5"),"Free redeployment retains its original prerequisite");
    report.Check(fr.purchasedOptions==before&&fr.progression.researchPoints==0,"Unqualified free purchase changes nothing");
    report.Check(!PurchaseOption(fr,"fr.tech.2"),"Distinct barrage allocation still costs research");
    fr.progression.researchPoints=1;report.OK(PurchaseOption(fr,"fr.tech.1"),"Paid reload coordination");
    report.OK(PurchaseOption(fr,"fr.tech.5"),"Qualified basic redeployment costs no further points");
    report.Check(fr.progression.researchPoints==0,"Only the distinctive paid option was debited");
    fr.progression.researchPoints=2;report.OK(PurchaseOption(fr,"fr.tech.4"),"Paid early follow-up retains rifle-guard prerequisite");
    report.OK(PurchaseOption(fr,"fr.tech.8"),"Explicit paired observer binding is a free qualified order");
    report.Check(fr.progression.researchPoints==0,"Paired observation does not bill a duplicate fire-plan ability");
    report.Check(fr.progression.technologyMask==0&&fr.progression.boughtTechnologyIds.empty()&&!UsesClassicModifiers(fr),"Free registration never enables Classic modifiers");
    Campaign ru;report.OK(NewTacticalCampaign(Faction::Russian,ru),"Russian campaign for pricing");ru.progression.researchPoints=0;
    report.OK(PurchaseOption(ru,"ru.tech.0"),"Basic reserve hold is free");
    report.Check(!PurchaseOption(ru,"ru.tech.7"),"Free Maxim movement retains artillery prerequisite");
    ru.progression.researchPoints=1;report.OK(PurchaseOption(ru,"ru.tech.3"),"Reserve artillery remains a paid tactical option");
    report.OK(PurchaseOption(ru,"ru.tech.7"),"Qualified Maxim movement is free");
    report.Check(ru.progression.researchPoints==0,"Russian shared order does not debit an extra point");
    auto invalid=ru;invalid.progression.technologyMask=1;
    report.Check(!ValidateCampaign(invalid),"Classic percentages are still rejected in TacticalV2");
}
void ArmourResearchV87(CheckReport& report)
{
    for(const auto Nation:{Faction::French,Faction::Russian})
    {
        const std::string Prefix=std::string(FactionId(Nation))+".armour.";
        const auto Crew=Prefix+"crew",Repair=Prefix+"repair",Prepare=Prefix+"prepare";
        const auto* Definition=FindOption(Crew);if(!Definition){report.Check(false,"National armour crew option exists");continue;}
        Campaign Early;report.OK(NewTacticalCampaign(Nation,Early),"New exclusive campaign for armour qualification");
        Early.progression.researchPoints=100;const auto BeforeEarly=Early;
        report.Check(!PurchaseOption(Early,Crew),"Research points cannot bypass the actual unlocked vehicle era: "+Crew);
        report.Check(Early.progression.researchPoints==BeforeEarly.progression.researchPoints&&Early.purchasedOptions==BeforeEarly.purchasedOptions,
            "Rejected early armour research neither bills points nor publishes an option");
        Campaign Campaign;report.OK(NewTacticalCampaign(Nation,Campaign),"Fresh campaign earns its actual armour-era frontier");
        const Mission* Frontier=nullptr;
        for(const auto& Mission:wfexp::Missions())if(Mission.faction==Nation)
        {
            if(Mission.date>=Definition->fromDate){Frontier=&Mission;break;}
            char Token[64];std::snprintf(Token,sizeof(Token),"a8700000-0000-0000-0000-%012d",100+static_cast<int>(Nation)*20+Mission.order);
            report.OK(SelectMission(Campaign.progression,Mission.id),"Select actual prior operation for armour frontier");
            report.OK(BeginEncounter(Campaign.progression,Mission.id,Token),"Open actual prior encounter token for armour frontier");
            report.OK(RecordVictory(Campaign.progression,Mission.id,Token),"Earn frontier using real progression rules");
        }
        report.Check(Frontier&&CanStart(Campaign.progression,Frontier->id),"First legal national vehicle-era operation is actually unlocked");
        const int Before=Campaign.progression.researchPoints;
        report.Check(!PurchaseOption(Campaign,Repair)&&!PurchaseOption(Campaign,Prepare),"Workshop and preparation require national crew training");
        report.Check(Campaign.progression.researchPoints==Before&&Campaign.purchasedOptions.empty(),"Missing prerequisites preserve earned research points");
        report.OK(PurchaseOption(Campaign,Crew),"Purchase national crew training through native research");
        report.OK(PurchaseOption(Campaign,Repair),"Purchase actual workshop research after its prerequisite");
        report.Check(Campaign.progression.researchPoints==Before-4,"Two research purchases debit exactly four earned points");
        if(Campaign.progression.researchPoints<3&&Frontier)
        {
            const auto Owned=Campaign.purchasedOptions;const int Points=Campaign.progression.researchPoints;
            report.Check(!PurchaseOption(Campaign,Prepare)&&Campaign.progression.researchPoints==Points&&Campaign.purchasedOptions==Owned,
                "Three-point preparation is not silently discounted or partially published");
            report.OK(SelectMission(Campaign.progression,Frontier->id),"Select frontier operation to earn remaining workshop budget");
            report.OK(BeginEncounter(Campaign.progression,Frontier->id,"a8700000-1111-2222-3333-444444444444"),"Begin frontier battle for remaining research");
            report.OK(RecordVictory(Campaign.progression,Frontier->id,"a8700000-1111-2222-3333-444444444444"),"Real frontier reward covers remaining research cost");
        }
        report.OK(PurchaseOption(Campaign,Prepare),"Purchase per-model preparation through native research");
        const auto* Opposite=FindOption(Nation==Faction::French?"ru.armour.crew":"fr.armour.crew");
        report.Check(Opposite&&!PurchaseOption(Campaign,Opposite->id),"Armour research cannot be borrowed from the opposite nation");
        const auto Owned=Campaign.purchasedOptions;const int Points=Campaign.progression.researchPoints;
        report.OK(PurchaseOption(Campaign,Crew),"Repeated passive purchase is idempotent");
        report.Check(Campaign.progression.researchPoints==Points&&Campaign.purchasedOptions==Owned,"Repeated passive purchase cannot debit or duplicate");
        const Mission* First=nullptr;for(const auto& M:wfexp::Missions())if(M.faction==Nation&&M.order==0){First=&M;break;}
        report.Check(First&&bool(SelectMission(Campaign.progression,First->id))&&bool(ValidateCampaign(Campaign)),
            "Previously earned armour research remains a valid old-profile record when revisiting an early date");
        report.Check(Campaign.progression.technologyMask==0&&Campaign.progression.boughtTechnologyIds.empty()&&!UsesClassicModifiers(Campaign),
            "Armour upgrades use stable purchasedOptions, never Classic percentage masks");
        // Explicit pure-rule fixture: all options share the unchanged 24-string
        // codec bound. It is not a UE campaign-win or material/effect proof.
        Campaign.progression.researchPoints=100;
        for(const auto& Option:Options())if(Option.faction==Nation&&Option.implemented)
            report.OK(PurchaseOption(Campaign,Option.id),"Full native profile owns each exclusive stable option");
        report.Check(Campaign.purchasedOptions.size()==15&&Campaign.purchasedOptions.size()<=24&&bool(ValidateCampaign(Campaign)),
            "Fifteen national options fit the unchanged codec and prerequisite contract");
        PilotFixture Fixture;
        if(Frontier&&Fixture.Init(Frontier->id,0,report))
        {
            State PassiveState;
            report.OK(NewEncounter(Campaign,Frontier->id,"a8700000-aaaa-bbbb-cccc-dddddddddddd",0,0,PassiveState),"Real exclusive profile opens a fresh encounter with armour research");
            Fixture.state=PassiveState;Fixture.snapshot.encounterId=PassiveState.encounterId;Fixture.snapshot.stateRevision=PassiveState.revision;
            const auto Signature=StateSignature(Fixture.state);auto Command=Fixture.Command(CommandKind::UseOption);Command.optionId=Crew;
            const auto Decision=Evaluate(Fixture.state,Fixture.snapshot,Command);
            report.Check(!Decision.allowed&&Decision.intents.empty()&&StateSignature(Fixture.state)==Signature,
                "Passive armour research cannot issue a tactical move, duplicate ammunition or active ability");
        }
    }
}

void PublicFireScores(CheckReport& report)
{
    const ObservedFireTarget moving{6,1,5,true,false,false};
    report.Check(ObservedSupportScore(ObservedFireKind::Shrapnel,moving)>ObservedSupportScore(ObservedFireKind::Mortar,moving),"Fresh observed marching group favours shrapnel over mortar");
    report.Check(ObservedSupportScore(ObservedFireKind::Shrapnel,moving)>ObservedSupportScore(ObservedFireKind::FieldArtillery,moving),"Fresh observed marching group favours shrapnel over field artillery");
    const ObservedFireTarget gun{6,1,1,false,true,true};
    report.Check(ObservedSupportScore(ObservedFireKind::FieldArtillery,gun)>ObservedSupportScore(ObservedFireKind::Mortar,gun),"Observed entrenched firing machine gun favours field artillery");
    report.Check(ObservedSupportScore(ObservedFireKind::FieldArtillery,gun)>ObservedSupportScore(ObservedFireKind::Shrapnel,gun),"Entrenchment does not attract open-ground shrapnel");
    const ObservedFireTarget single{4,1,1,false,false,false};
    report.Check(ObservedSupportScore(ObservedFireKind::Mortar,single)>ObservedSupportScore(ObservedFireKind::FieldArtillery,single),"Isolated static contact retains a useful mortar role");
    auto stale=moving;stale.confidence=.2;
    report.Check(ObservedSupportScore(ObservedFireKind::Shrapnel,stale)<ObservedSupportScore(ObservedFireKind::Mortar,stale),"Moving contact memory reduces shrapnel suitability");
    auto expired=moving;expired.confidence=0;
    for(auto kind:{ObservedFireKind::Mortar,ObservedFireKind::FieldArtillery,ObservedFireKind::Shrapnel})
        report.Check(ObservedSupportScore(kind,expired)==0,"Expired observation produces no target score");
    auto bad=moving;bad.importance=std::numeric_limits<double>::quiet_NaN();
    report.Check(ObservedSupportScore(ObservedFireKind::Mortar,bad)==0,"Nonfinite contact importance is rejected");
    bad=moving;bad.nearbyWeight=-1;report.Check(ObservedSupportScore(ObservedFireKind::Shrapnel,bad)==0,"Negative contact density is rejected");
    bad=moving;bad.confidence=1.1;report.Check(ObservedSupportScore(ObservedFireKind::Mortar,bad)==0,"Confidence outside the observed range is rejected");
    report.Check(ObservedSupportScore(static_cast<ObservedFireKind>(255),moving)==0,"Unknown support scoring kind is rejected");
}
void ReadOnlyOrders(CheckReport& report)
{
    PilotFixture f;if(!f.Init("fr.chemin_dames",0,report,true))return;
    auto hold=f.Command(CommandKind::UseOption);hold.optionId="fr.tech.0";hold.squadIds={10};hold.variant=1;
    const auto signature=StateSignature(f.state);const auto revision=f.state.revision;
    const auto held=Evaluate(f.state,f.snapshot,hold);
    report.Check(held.allowed&&held.intents.size()==1&&held.intents[0].kind==IntentKind::SetHold&&held.intents[0].hold,"Preview uses the actual stationary-rifle hold consumer");
    report.Check(StateSignature(f.state)==signature&&f.state.revision==revision,"An allowed preview never commits an order or advances the clock");
    hold.variant=0;const auto released=Evaluate(f.state,f.snapshot,hold);
    report.Check(released.allowed&&released.intents.size()==1&&!released.intents[0].hold,"Primary and alternate preserve their real release/hold semantics");
    hold.squadIds={12};report.Check(!Evaluate(f.state,f.snapshot,hold).allowed,"Marksman cannot impersonate the rifle-guard role");
    hold.squadIds={10};f.Squad(10)->stationary=false;f.Squad(10)->advancing=true;
    report.Check(!Evaluate(f.state,f.snapshot,hold).allowed,"Moving section is not offered a stationary order");
    f.Squad(10)->stationary=true;f.Squad(10)->advancing=false;
    auto allocation=f.Command(CommandKind::UseOption);allocation.optionId="fr.tech.2";allocation.targetNode="ridge";allocation.alternateNode="forest";allocation.variant=1;
    report.Check(!Evaluate(f.state,f.snapshot,allocation).allowed,"Owned paid option stays unavailable before a fire draft exists");
    auto plan=f.Command(CommandKind::PlanFire);plan.targetNode="ridge";f.Apply(plan,report);
    allocation=f.Command(CommandKind::UseOption);allocation.optionId="fr.tech.2";allocation.targetNode="ridge";allocation.alternateNode="forest";allocation.variant=1;
    report.Check(Evaluate(f.state,f.snapshot,allocation).allowed,"Barrage-allocation option becomes available for an editable draft and alternate target");
    allocation.alternateNode.clear();report.Check(!Evaluate(f.state,f.snapshot,allocation).allowed,"Missing alternate target disables the real barrage-allocation consumer");
    report.Check(f.state.fire[0].phase==FirePhase::Planned&&f.state.fire[0].stencil==0,"Preview changes neither the draft nor its salvo allocation");
    PilotFixture unowned;if(!unowned.Init("fr.mort_homme",0,report))return;
    hold=unowned.Command(CommandKind::UseOption);hold.optionId="fr.tech.0";hold.squadIds={10};
    report.Check(!Evaluate(unowned.state,unowned.snapshot,hold).allowed,"Free pricing does not bypass an option's registration marker");
    PilotFixture r;if(!r.Init("ru.warsaw_rearguard",0,report,true))return;
    hold=r.Command(CommandKind::UseOption);hold.optionId="ru.tech.0";hold.squadIds={10};hold.variant=1;
    report.Check(!Evaluate(r.state,r.snapshot,hold).allowed,"Russian reserve hold requires a registered reserve");
    auto reserve=r.Command(CommandKind::RegisterReserve);reserve.squadIds={10};reserve.targetNode="ridge";r.Apply(reserve,report);
    hold=r.Command(CommandKind::UseOption);hold.optionId="ru.tech.0";hold.squadIds={10};hold.variant=1;
    const auto before=StateSignature(r.state);report.Check(Evaluate(r.state,r.snapshot,hold).allowed,"Registered Russian reserve exposes the valid hold order");
    report.Check(StateSignature(r.state)==before,"Reserve preview neither releases nor moves troops");
    auto inspect=r.Command(CommandKind::UseOption);inspect.optionId="ru.tech.8";inspect.targetNode="relay";inspect.alternateNode="ridge";
    report.Check(!Evaluate(r.state,r.snapshot,inspect).allowed,"Route report cannot inspect two unobserved positions");
    r.Add(16,3,"ridge",0);r.Proof(12,"relay");auto eye=r.Node("ridge")->position;eye.z+=180;
    r.snapshot.observations.push_back({16,"ridge",r.snapshot.elapsed,1,eye,true});
    const auto observed=Evaluate(r.state,r.snapshot,inspect);
    report.Check(observed.allowed&&observed.intents.size()==1&&observed.intents[0].kind==IntentKind::NotifyTelegraph
        &&observed.intents[0].targetNode=="relay"&&observed.intents[0].alternateNode=="ridge","Only fresh observation issues the two-position report");
    r.snapshot.observations[1].hostLineOfSight=false;
    report.Check(!Evaluate(r.state,r.snapshot,inspect).allowed,"Losing sight of either position blocks the report");
}
void CampaignProgressionV827(CheckReport& report)
{
    std::set<std::string> EnemyOrders;
    for(Faction Nation:{Faction::French,Faction::Russian})
    {
        Archive Campaign;report.OK(NewCampaign(Nation,Campaign),"New chronological progression campaign");
        std::uint32_t PreviousRoles=0,PreviousSupports=0;int Count=0;
        for(const auto& Mission:wfexp::Missions())if(Mission.faction==Nation)
        {
            ++Count;const std::string Id=Mission.id;const auto* Rules=wfexp::tactical::FindMission(Id);
            report.Check(Rules&&Rules->date==Mission.date,"Catalog and tactical operation date agree: "+Id);
            report.Check((Mission.roleMask&PreviousRoles)==PreviousRoles,"Unit availability never disappears between operations: "+Id);
            report.Check((Mission.supportMask&PreviousSupports)==PreviousSupports,"Support availability never disappears between operations: "+Id);
            PreviousRoles=Mission.roleMask;PreviousSupports=Mission.supportMask;
            for(int RoleId=0;RoleId<11;++RoleId)if(Mission.roleMask&(1u<<RoleId))
            {
                const auto* Role=RoleFor(Nation,RoleId,Mission.date);
                report.Check(Role&&RoleAllowed(Mission,RoleId),"Enabled unit is legal on operation date: "+Id+" / "+std::to_string(RoleId));
                if(Role)for(int Member=0;Member<Role->size;++Member)
                    report.Check(WeaponFor(Nation,RoleId,Member,Mission.date)!=nullptr,"Enabled unit has date-correct primary and escort weapons: "+Id);
            }
            for(int SupportId=0;SupportId<10;++SupportId)if(Mission.supportMask&(1u<<SupportId))
                report.Check(SupportAllowed(Mission,SupportId),"Enabled support is legal and implemented on operation date: "+Id+" / "+std::to_string(SupportId));
            if(Rules)for(const auto& Stage:Rules->stages)if(Stage.requiredRole>=0)
                report.Check(RoleAllowed(Mission,Stage.requiredRole),"Required task participant is recruitable: "+Stage.id);
            for(const char* Order:{Mission.enemyRoleOrder,Mission.enemyOpeningRoles})
            {
                std::string Roles=Order;std::size_t Start=0;
                do
                {
                    const auto End=Roles.find('|',Start);const auto Token=Roles.substr(Start,End==std::string::npos?End:End-Start);
                    const int RoleId=Token.empty()?-1:std::stoi(Token);
                    report.Check(RoleFor(Mission.enemy,RoleId,Mission.date)!=nullptr,"Authored enemy unit resolves without unavailable placeholder: "+Id+" / "+Token);
                    if(End==std::string::npos)break;Start=End+1;
                }while(Start<=Roles.size());
            }
            EnemyOrders.insert(Mission.enemyRoleOrder);
            if(Mission.order==0)
            {
                report.Check(Mission.roleMask==71u,"First operation starts with rifle, gun, assault and required pioneer: "+Id);
                report.Check(Mission.supportMask==5u,"First operation reserves later fire support for later operations: "+Id);
            }
            char Token[64];std::snprintf(Token,sizeof(Token),"00000000-0000-0000-0000-%012d",100+static_cast<int>(Nation)*20+Mission.order);
            report.OK(SelectMission(Campaign,Id),"Select chronological operation: "+Id);
            report.OK(BeginEncounter(Campaign,Id,Token),"Begin real token-bound archive operation: "+Id);
            report.OK(RecordVictory(Campaign,Id,Token),"Victory advances the same chronological archive: "+Id);
            report.OK(ValidateArchive(Campaign),"Changed roster preserves completed archive validation: "+Id);
            for(const auto& Dispatch:wfexp::Dispatches())if(Dispatch.faction==Nation&&Id==Dispatch.afterMissionId)
                for(std::size_t ChoiceIndex=0;ChoiceIndex<Dispatch.choiceCount;++ChoiceIndex)
                {
                    Archive Copy=Campaign;const auto& Choice=Dispatch.choices[ChoiceIndex];
                    const auto Resolved=ResolveDispatch(Copy,Dispatch.id,Choice.id);
                    report.OK(Resolved,"Every earned dispatch choice still resolves: "+std::string(Choice.id));
                    if(!Resolved)continue;
                    report.OK(ValidateArchive(Copy),"Dispatch reward remains a valid archive: "+std::string(Choice.id));
                    if(Choice.loanRoleId[0])
                    {
                        const auto* Role=FindRole(Choice.loanRoleId);const auto* Next=wfexp::FindMission(Copy.pendingLoanMissionId);
                        report.Check(Role&&Next&&Next->order==Mission.order+1&&RoleAllowed(*Next,Role->roleId),
                            "Loan is legal in exactly the next operation: "+std::string(Choice.id));
                        if(Role&&Next)
                        {
                            report.OK(BeginEncounter(Copy,Next->id,"99999999-8888-7777-6666-555555555555"),"Open attachment's actual next operation");
                            report.OK(ConsumeLoan(Copy,Choice.loanRoleId),"Loan remains consumable after real encounter begins");
                            report.OK(ValidateArchive(Copy),"Consumed attachment preserves archive contract");
                        }
                    }
                }
        }
        report.Check(Count==10,"Ten unique chronological operations for each campaign");
    }
    report.Check(EnemyOrders.size()>=12,"Operations contain varied authored enemy compositions");
}
void IdentityDates(CheckReport& report)
{
    constexpr std::array<std::pair<const char*,int>,20> dates{{
        {"fr.mort_homme",19160306},{"fr.vaux",19160601},{"fr.douaumont",19161024},{"fr.louvemont",19161215},{"fr.chemin_dames",19170416},
        {"fr.malmaison",19171023},{"fr.matz",19180611},{"fr.champagne_defence",19180715},{"fr.soissons",19180718},{"fr.sommepy",19180926},
        {"ru.galicia",19140829},{"ru.przemysl",19150319},{"ru.gorlice",19150502},{"ru.warsaw_rearguard",19150805},{"ru.naroch",19160318},
        {"ru.lutsk",19160604},{"ru.czernowitz",19160618},{"ru.kovel",19160728},{"ru.kerensky",19170701},{"ru.riga",19170901}}};
    for(const auto& entry:dates){const auto* m=wfexp::tactical::FindMission(entry.first);report.Check(m&&m->date==entry.second,"Stable mission/date preserved: "+std::string(entry.first));}
}
void ArchiveEraQualificationV87(CheckReport& report)
{
    for(const char* Id:{"fr.mort_homme","ru.galicia"})
    {
        PilotFixture f;if(!f.Init(Id,0,report))continue;
        report.Check(!f.state.fullArmoryReplay,"Ordinary encounter has no replay capability: "+std::string(Id));
        f.Add(900,4,"assembly",0);
        report.Check(f.Squad(900)!=nullptr,"Replay test command section is real and date-qualified: "+std::string(Id));
        report.Check(!ValidateSnapshot(f.state,f.snapshot),"Ordinary early operation retains its roster restriction: "+std::string(Id));
        f.state.fullArmoryReplay=true;
        report.OK(ValidateSnapshot(f.state,f.snapshot),"Explicit replay permits a date-qualified additional national section: "+std::string(Id));
        if(auto* Section=f.Squad(900))
        {
            const auto Before=*Section;Section->roleId=9;
            report.Check(!ValidateSnapshot(f.state,f.snapshot),"Replay still refuses unavailable or not-yet-issued support weapons: "+std::string(Id));
            *Section=Before;Section->roleId=8;
            report.Check(!ValidateSnapshot(f.state,f.snapshot),"Replay cannot invent an unavailable anti-tank rifle section: "+std::string(Id));
            *Section=Before;
        }
        auto Foreign=f.snapshot;Foreign.nations[0]=Faction::British;
        report.Check(!ValidateSnapshot(f.state,Foreign),"Replay capability cannot substitute another army's identity: "+std::string(Id));
    }
    PilotFixture atomic;if(!atomic.Init("fr.mort_homme",0,report))return;
    auto Clock=atomic.Command(CommandKind::AdvanceTime);Clock.hostFrame=1;
    const auto Prepared=Prepare(atomic.state,atomic.snapshot,Clock);report.Check(Prepared.View().allowed,"Prepare ordinary atomic clock before replay-capability mutation");
    std::vector<std::string> Receipts;for(const auto& Intent:Prepared.View().intents)Receipts.push_back(Intent.id);
    atomic.state.fullArmoryReplay=true;
    report.Check(!Acknowledge(atomic.state,atomic.snapshot,Prepared,Receipts),"Replay capability is included in the atomic state signature");
}
void ClearedCrossingStageV87(CheckReport& report)
{
    struct Case {const char* mission;const char* choice;const char* cleared;const char* approach;};
    const Case Cases[]={{"ru.galicia","alternative","rail","front_right"},
        {"fr.louvemont","steady","wire","front_left"},{"ru.kovel","steady","rail","front_right"}};
    for(const auto& C:Cases)
    {
        PilotFixture f;if(!f.Init(C.mission,0,report))continue;const std::string Label=C.mission;
        for(auto& Node:f.snapshot.nodes)if(Node.id=="objective")Node.owner=1;
        f.Add(900,6,C.cleared,0);f.Time(4);
        auto Choose=f.Command(CommandKind::ChooseStory);Choose.storyId=Label+".local_order";Choose.choiceId=C.choice;f.Apply(Choose,report);
        const auto& First=f.definition->stages[0];const auto& Second=f.definition->stages[1];
        report.Check(First.kind==GateKind::ClearPassage&&Second.followClearedPassageV87,"Registered follow-cleared-crossing stage: "+Label);
        for(int Tick=4;Tick<180&&f.state.objective.index==0;++Tick)
        {f.Time(Tick);f.Apply(f.Command(CommandKind::AdvanceTime),report);}
        report.Check(f.state.objective.index==1&&f.state.objective.boundObstacle==C.cleared,"Second stage freezes the actual opened crossing: "+Label);
        report.Check(f.state.objective.clearedObstacles.size()==1&&f.state.objective.clearedObstacles.front()==C.cleared,
            "Only the actually worked crossing is cleared: "+Label);
        // These are explicitly synthetic native proof fixtures. They test the
        // reducer/registered graph, not UE movement, bodies or campaign wins.
        if(auto* Worker=f.Squad(900)){Worker->nodeId="home";Worker->position=f.Node("home")->position;}
        f.Add(910,0,C.approach,0);const double Activated=f.state.objective.activatedAt;
        f.Time(Activated+1);f.Apply(f.Command(CommandKind::AdvanceTime),report);
        report.Check(f.state.objective.index==1&&f.state.objective.progress==0,"Clearance does not fabricate next-stage arrival or hold time: "+Label);
        auto Wrong=f.Command(CommandKind::AdvanceToObjective);Wrong.squadIds={910};Wrong.targetNode=Second.nodes.front();
        report.Check(!Evaluate(f.state,f.snapshot,Wrong).allowed,"The old closed crossing is not the bound objective: "+Label);
        auto Advance=f.Command(CommandKind::AdvanceToObjective);Advance.squadIds={910};Advance.targetNode=C.cleared;
        const auto Move=f.Apply(Advance,report);
        const auto Actual=std::find_if(Move.intents.begin(),Move.intents.end(),[](const auto& I){return I.kind==IntentKind::AdvanceExistingSquad;});
        report.Check(Actual!=Move.intents.end()&&Actual->targetNode==C.cleared&&Actual->routeEdges.size()==1,
            "Following section receives the actual legal incoming crossing edge: "+Label);
        if(Actual==Move.intents.end())continue;
        auto* Following=f.Squad(910);Following->nodeId=C.cleared;Following->position=f.Node(C.cleared)->position;
        f.snapshot.movements.push_back({"v87.crossing.arrival."+Label,f.state.encounterId,Actual->routeEdges.back(),910,1,Activated+2});
        const int Duration=Second.kind==GateKind::HoldNode?int(Second.duration)+2:2;
        for(int Tick=2;Tick<Duration+3&&f.state.objective.index==1;++Tick)
        {f.Time(Activated+Tick);f.Apply(f.Command(CommandKind::AdvanceTime),report);}
        report.Check(f.state.objective.index==2&&!f.state.objective.finished,"Actual fresh-arrival/hold proof advances exactly one stage: "+Label);
    }
}
}
int main(int argc,char** argv)
{
    CheckReport report;OptionCosts(report);ArmourResearchV87(report);PublicFireScores(report);ReadOnlyOrders(report);IdentityDates(report);CampaignProgressionV827(report);ArchiveEraQualificationV87(report);ClearedCrossingStageV87(report);
    std::ostream* out=&std::cout;std::ofstream file;if(argc>1){file.open(argv[1]);if(!file)return 3;out=&file;}
    *out<<"{\"revision\":\"v826.native.orders-public-fire.1\",\"scope\":\"Focused native pricing, observed-contact scoring and reducer preview fixtures; no UE actor, art, performance or Shipping acceptance\",\"checks\":"<<report.checks<<",\"passed\":"<<(report.Passed()?"true":"false")<<",\"failures\":[";
    for(std::size_t i=0;i<report.failures.size();++i){if(i)*out<<',';*out<<'"';for(char c:report.failures[i]){if(c=='"'||c=='\\')*out<<'\\';*out<<c;}*out<<'"';}*out<<"]}\n";
    std::cout<<"V826 orders/public fire: "<<report.checks<<" checks, "<<report.failures.size()<<" failures\n";
    for(const auto& failure:report.failures)std::cerr<<failure<<'\n';return report.Passed()?0:1;
}
