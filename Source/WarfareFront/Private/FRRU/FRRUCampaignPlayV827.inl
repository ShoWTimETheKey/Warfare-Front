// Ordinary trench advance uses the same physical route/bay transaction as a
// player's order. The reducer still owns clearance, hold and arrival receipts.
namespace
{
struct FOperationPlayV827
{
    const TCHAR* Id;const char* EnemyJunction;int32 Echelon;float RestMin,RestMax;
};
const FOperationPlayV827& OperationPlayV827(const FWorld& R)
{
    static const FOperationPlayV827 Operations[]={
        {TEXT("fr.mort_homme"),"ridge",3,12,18},
        {TEXT("fr.vaux"),"objective",3,14,20},
        {TEXT("fr.douaumont"),"objective",2,16,22},
        {TEXT("fr.louvemont"),"objective",2,15,21},
        {TEXT("fr.chemin_dames"),"objective",3,17,23},
        {TEXT("fr.malmaison"),"objective",2,17,23},
        {TEXT("fr.matz"),"rail",3,13,19},
        {TEXT("fr.champagne_defence"),"front_left",4,13,19},
        {TEXT("fr.soissons"),"objective",2,16,22},
        {TEXT("fr.sommepy"),"objective",3,15,21},
        {TEXT("ru.galicia"),"objective",2,17,23},
        {TEXT("ru.przemysl"),"ridge",3,15,21},
        {TEXT("ru.gorlice"),"ridge",4,12,18},
        {TEXT("ru.warsaw_rearguard"),"bridge",3,12,18},
        {TEXT("ru.naroch"),"objective",2,18,24},
        {TEXT("ru.lutsk"),"objective",2,17,23},
        {TEXT("ru.czernowitz"),"objective",3,15,21},
        {TEXT("ru.kovel"),"bridge",3,16,22},
        {TEXT("ru.kerensky"),"ridge",3,14,20},
        {TEXT("ru.riga"),"bridge",4,12,18}};
    for(const auto& Operation:Operations)if(R.Mission==Operation.Id)return Operation;
    static const FOperationPlayV827 Fallback={TEXT(""),"objective",3,14,20};return Fallback;
}
int32 EnemyEchelonSizeV827(const FWorld& R){return OperationPlayV827(R).Echelon;}
FVector2D EnemyEchelonRestV827(const FWorld& R)
{const auto& Operation=OperationPlayV827(R);return FVector2D(Operation.RestMin,Operation.RestMax);}
bool RearPostV827(const std::string& Id)
{return Id=="home"||Id=="assembly"||Id=="relay"||Id=="exit"||Id=="enemy_assembly";}
int32 ClaimedPostsV827(const AWarfareFrontlineGameMode* B,const FWorld& R,
    const wfexp::tactical::MissionDefinition& M,const std::string& Id,int32 Side,int32 Except=INDEX_NONE)
{
    int32 Count=0;
    for(const auto& Group:B->Squads)if(Group.Id!=Except&&Group.Team==Side&&Group.LivingCount()>0)
    {
        const auto Route=R.Routes.find(Group.Id);
        if(Route!=R.Routes.end()&&Group.bAdvancing&&wfexpue::Narrow(Route->second.Target)==Id){++Count;continue;}
        if(const auto* Here=OccupiedNode(Group,M);Here&&Here->id==Id)++Count;
    }
    return Count;
}
bool EnemyRearHasRoomV827(AWarfareFrontlineGameMode* B,const FWorld& R)
{
    const auto* M=Definition(B);const auto* Rear=M?Node(*M,"enemy_assembly"):nullptr;
    // This is a real ingress bay promise, not a maximum active-army size.
    // Old sections at forward posts no longer block a new echelon.
    if(!Rear)return false;int32 Arrivals=0;const auto Cells=R.Cells.find(Rear->id);
    for(const auto& Group:B->Squads)if(Group.Team==1&&Group.LivingCount()>0)
    {
        const auto Route=R.Routes.find(Group.Id);
        const bool Bay=Cells!=R.Cells.end()&&Cells->second[1].count(Group.Id)>0;
        const bool Arriving=Route!=R.Routes.end()&&Group.bAdvancing&&wfexpue::Narrow(Route->second.Target)==Rear->id;
        if(Bay||Arriving||AtNode(Group,*Rear,false))++Arrivals;
    }
    return Arrivals<Rear->capacity;
}
bool SectionSettledV827(AWarfareFrontlineGameMode* B,const FWorld& R,
    const FFrontlineSquad& Group,const wfexp::tactical::NodeDefinition& Here)
{
    const auto Cells=R.Cells.find(Here.id);
    if(Cells==R.Cells.end())return false;const auto Bay=Cells->second[Group.Team].find(Group.Id);
    return Bay!=Cells->second[Group.Team].end()&&AtNode(Group,Here,true)&&FormationSettled(Group,Here,Bay->second);
}
bool ActualPostGuardV87(AWarfareFrontlineGameMode* B,const FWorld& R,
    const FFrontlineSquad& Group,const wfexp::tactical::NodeDefinition& Post)
{
    const auto Route=R.Routes.find(Group.Id);
    return Group.Team==0&&Group.LivingCount()>0&&(Route==R.Routes.end()||!InTransit(Route->second))
        &&SectionSettledV827(B,R,Group,Post);
}
bool ApproachingPostGuardV87(AWarfareFrontlineGameMode* B,const FWorld& R,
    const wfexp::tactical::MissionDefinition& M,const std::string& Post,int32 Except,int32 Side=0)
{
    // An accepted destination is already a capacity promise. Count it only
    // while its actual living section can still follow every remaining edge;
    // an interrupted/cancelled order must not suppress the next relief party.
    // This coordinates orders only: it grants no presence, ownership or time.
    for(const auto& Other:B->Squads)if(Other.Id!=Except&&Other.Team==Side&&Other.LivingCount()>0
        &&Other.bAdvancing&&!Other.bHoldPosition)
    {
        const auto Found=R.Routes.find(Other.Id);if(Found==R.Routes.end())continue;
        FRoute Remaining=Found->second;
        if(wfexpue::Narrow(Remaining.Target)!=Post||!InTransit(Remaining)||Remaining.Step<0
            ||Remaining.Step>=int32(Remaining.Edges.size()))continue;
        bool Valid=true;
        while(Remaining.Step<int32(Remaining.Edges.size()))
        {
            if(!RouteDepartureAllowed(B,M,R,Other,Remaining)){Valid=false;break;}
            Remaining.Current=wfexpue::Wide(NextNode(Remaining,M));++Remaining.Step;
        }
        if(Valid&&wfexpue::Narrow(Remaining.Current)==Post)return true;
    }
    return false;
}
bool TaskSectionV827(const FWorld& R,const wfexp::tactical::StageDefinition& Stage,const FFrontlineSquad& Group)
{
    if(Stage.requiredRole>=0&&int32(Group.Role)!=Stage.requiredRole)return false;
    if(Stage.kind==wfexp::tactical::GateKind::WithdrawToNode
        &&std::find(R.State.objective.withdrawalParticipants.begin(),R.State.objective.withdrawalParticipants.end(),Group.Id)
            ==R.State.objective.withdrawalParticipants.end())return false;
    if(R.State.objective.fallback
        &&Stage.kind==wfexp::tactical::GateKind::ClearPassage
        &&(std::find(Stage.nodes.begin(),Stage.nodes.end(),Stage.fallbackNode)!=Stage.nodes.end()
            ||std::find(Stage.alternateNodes.begin(),Stage.alternateNodes.end(),Stage.fallbackNode)!=Stage.alternateNodes.end())
        &&std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Group.Id)
            ==R.State.objective.fallbackWorkers.end())return false;
    return true;
}
bool MandatoryIngressSectionV87(const FWorld& R,const wfexp::tactical::StageDefinition& Stage,const FFrontlineSquad& Group)
{
    // Ordinary holds can use any section. Reserve priority is for a required
    // specialist or the already frozen recovery participant, not every recruit.
    return (Stage.requiredRole>=0||R.State.objective.fallback)&&Group.LivingCount()>=Stage.minimumLiving
        &&TaskSectionV827(R,Stage,Group);
}
bool WaitingMandatoryIngressV87(const AWarfareFrontlineGameMode* B,const FWorld& R,
    const wfexp::tactical::StageDefinition& Stage,const FFormationCells& Cells,int32 Except=INDEX_NONE)
{
    const auto Home=Cells.find("home");
    for(const auto& Group:B->Squads)if(Group.Id!=Except&&Group.Team==0&&R.OrdinarySectionsV827.Contains(Group.Id)
        &&!Group.bHoldPosition&&MandatoryIngressSectionV87(R,Stage,Group))
    {
        const auto Route=R.Routes.find(Group.Id);
        if(Route!=R.Routes.end()&&Route->second.Current.IsEmpty()&&Route->second.Target==TEXT("home")
            &&(Home==Cells.end()||!Home->second[0].count(Group.Id)))return true;
    }
    return false;
}
bool MayReserveHomeIngressV87(const AWarfareFrontlineGameMode* B,int32 Squad,const FFormationCells& Cells)
{
    const auto* Found=B?Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B))):nullptr;
    const auto* Mission=B?Definition(B):nullptr;const auto* Group=B?B->FindSquad(Squad):nullptr;
    if(!Found||!Mission||!Group||!SameDeployment(B,**Found))return true;
    const auto& R=**Found;if(R.State.objective.finished||R.State.objective.index>=Mission->stages.size())return true;
    const auto& Stage=Mission->stages[R.State.objective.index];
    // A public manual move retains its promise. A new automatic recruit has
    // no route yet; existing automatic ingress is explicitly owned by this set.
    if(R.Routes.count(Squad)&&!R.OrdinarySectionsV827.Contains(Squad))return true;
    return MandatoryIngressSectionV87(R,Stage,*Group)||!WaitingMandatoryIngressV87(B,R,Stage,Cells,Squad);
}
bool HasTaskGuardV827(AWarfareFrontlineGameMode* B,const FWorld& R,
    const wfexp::tactical::StageDefinition& Stage,
    const wfexp::tactical::NodeDefinition& Target)
{
    for(const auto& Other:B->Squads)if(Other.Team==0&&Other.LivingCount()>=Stage.minimumLiving
        &&TaskSectionV827(R,Stage,Other)&&SectionSettledV827(B,R,Other,Target))return true;
    return false;
}
bool FreshTaskArrivalV827(const FWorld& R,int32 Id)
{
    const auto Arrival=R.Arrivals.find(Id);if(Arrival==R.Arrivals.end())return false;
    uint64 Watermark=0;for(const auto& Previous:R.State.objective.arrivalSequences)
        if(Previous.first==Id)Watermark=FMath::Max(Watermark,uint64(Previous.second));
    return Arrival->second.arrivedAt>R.State.objective.activatedAt&&Arrival->second.sequence>Watermark
        &&std::find(R.State.objective.usedMovementReceipts.begin(),R.State.objective.usedMovementReceipts.end(),Arrival->second.id)
            ==R.State.objective.usedMovementReceipts.end();
}
bool ReservedByTacticsV827(const AWarfareFrontlineGameMode* B,const FWorld& R,int32 Id)
{
    const auto& Reserve=R.State.reserve[0];
    for(std::size_t I=Reserve.released;I<Reserve.squads.size();++I)if(Reserve.squads[I]==Id)return true;
    const auto& Fire=R.State.fire[0];
    if(Fire.phase!=wfexp::tactical::FirePhase::Idle&&Fire.phase!=wfexp::tactical::FirePhase::Committed)
        if(Fire.observerSquad==Id||Fire.secondaryObserver==Id
            ||std::find(Fire.reloadSquads.begin(),Fire.reloadSquads.end(),Id)!=Fire.reloadSquads.end()
            ||std::find(Fire.followupSquads.begin(),Fire.followupSquads.end(),Id)!=Fire.followupSquads.end())return true;
    if(Fire.phase==wfexp::tactical::FirePhase::Committed&&!Fire.followupUsed
        &&std::find(Fire.followupSquads.begin(),Fire.followupSquads.end(),Id)!=Fire.followupSquads.end())
    {
        // Assigned follow-up sections remain available until actual landed
        // salvos open and close the native eight-second follow-up window.
        std::vector<wfexp::tactical::SupportReceipt> Receipts;bool Expired=false;
        if(R.Journal.ActiveReceipts(Fire.planId,R.State.encounterId,Receipts))
            for(const auto& Receipt:Receipts)if(Receipt.side==0&&Receipt.resolvedSalvos==4
                &&B->Elapsed-Receipt.resolvedAt>8.)Expired=true;
        if(!Expired)return true;
    }
    for(const auto& Hold:R.State.arrivalHolds)if(Hold.first==Id)return true;
    return false;
}
bool OrdinaryMoveV827(AWarfareFrontlineGameMode* B,FWorld& R,int32 Id,const std::string& Target)
{
    const FString PreviousNotice=R.Notice;TGuardValue<bool> NaturalOrder(R.OrdinaryOrderV827,true);
    const bool Moved=MoveSquad(B,Id,wfexpue::Wide(Target));R.Notice=PreviousNotice;
    if(Moved)UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V827_ORDINARY_ADVANCE mission=%s squad=%d target=%s t=%.3f"),
        *R.Mission,Id,*wfexpue::Wide(Target),B->Elapsed);
    return Moved;
}
bool RelieveAutomaticBridgeReservationV87(AWarfareFrontlineGameMode* B,FWorld& R,
    const wfexp::tactical::MissionDefinition& M,const wfexp::tactical::StageDefinition& Stage,
    const FFrontlineSquad& Resident,const wfexp::tactical::NodeDefinition& Here,const wfexp::tactical::NodeDefinition& Goal)
{
    // Only the two measured single-bay dependencies qualify. Mort's recovery
    // uses its existing PolicyOrder target; no other fallback or post may
    // exchange promises. Occupied bays remain owned by the actual bodies.
    const bool Galicia=R.Mission==TEXT("ru.galicia")&&!R.State.objective.fallback&&Here.id=="bridge";
    const bool MortRecovery=R.Mission==TEXT("fr.mort_homme")&&Stage.id=="fr.mort_homme.stage.2"
        &&R.State.objective.fallback&&!R.State.objective.fallbackArrived&&Here.id=="forest"
        &&Goal.id=="front_right"&&Goal.id==Stage.fallbackNode;
    if((!Galicia&&!MortRecovery)||Stage.kind!=wfexp::tactical::GateKind::HoldNode
        ||Here.capacity!=1||Here.id==Goal.id||Resident.bAdvancing||Resident.bHoldPosition
        ||!R.OrdinarySectionsV827.Contains(Resident.Id)||ReservedByTacticsV827(B,R,Resident.Id)
        ||!TaskSectionV827(R,Stage,Resident)||Resident.LivingCount()<Stage.minimumLiving
        ||ClaimedPostsV827(B,R,M,Goal.id,0,Resident.Id)<Goal.capacity||HasTaskGuardV827(B,R,Stage,Goal)
        ||std::find(Stage.mustOwn.begin(),Stage.mustOwn.end(),Here.id)!=Stage.mustOwn.end())return false;
    const auto Owner=R.Owners.find(Here.id);const auto Waiting=R.PostAdmissionsV827.find(Here.id);
    if(Owner==R.Owners.end()||Owner->second!=0||Waiting==R.PostAdmissionsV827.end())return false;
    const auto& Admission=Waiting->second[0];
    if(Admission.Owner!=INDEX_NONE||Admission.Waiting.empty())return false;
    const int32 DonorId=Admission.Waiting.front();const auto* Donor=B->FindSquad(DonorId);const auto Found=R.Routes.find(DonorId);
    if(!Donor||DonorId==Resident.Id||Donor->Team!=0||Donor->LivingCount()<=0||!Donor->bAdvancing||Donor->bHoldPosition
        ||!R.OrdinarySectionsV827.Contains(DonorId)||ReservedByTacticsV827(B,R,DonorId)||Found==R.Routes.end())return false;
    if(MortRecovery)for(const int32 Id:{Resident.Id,DonorId})
        if(std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Id)!=R.State.objective.holdParticipants.end()
            ||std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Id)!=R.State.objective.fallbackWorkers.end())return false;
    const auto& Route=Found->second;
    if(Route.Target!=wfexpue::Wide(Goal.id)||!IsIntermediateTransitLeg(Route)||NextNode(Route,M)!=Here.id
        ||!Route.TransitCorridorWaiting||Route.TransitCheckpointStep==Route.Step||Route.CarrierDelivery
        ||BodyInPostV827(*Donor,Here)||BodyInPostV827(*Donor,Goal))return false;
    const auto Forward=PhysicalOrderRouteV87(B,R,Resident,Here,Goal.id);if(Forward.empty())return false;
    wfexp::tactical::Intent Intent;Intent.id="v87.auto.bridge.handover."+std::to_string(Resident.Id);
    Intent.kind=wfexp::tactical::IntentKind::AdvanceExistingSquad;Intent.side=0;Intent.squadId=Resident.Id;
    Intent.targetNode=Goal.id;Intent.target=Goal.position;Intent.routeEdges=Forward;
    const auto Snapshot=Capture(B,R,false);TGuardValue<bool> NaturalOrder(R.OrdinaryOrderV827,true);FBatch Batch(B,R);
    Batch.StageAutomaticPostHandoverV87(DonorId,Here.id);
    const bool Committed=Batch.Preflight(Snapshot,{Intent})&&Batch.Commit(Snapshot,{Intent});
    if(Committed)
    {
        R.OrderPreview.At=-1;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_AUTOMATIC_BRIDGE_HANDOVER mission=%s resident=%d donor=%d resident_from=%s resident_to=%s donor_to=%s donor_step=%d host=%.3f"),
            *R.Mission,Resident.Id,DonorId,*wfexpue::Wide(Here.id),*wfexpue::Wide(Goal.id),*wfexpue::Wide(Here.id),Route.Step,B->Elapsed);
    }
    return Committed;
}
bool GunCoverCanReachV87(AWarfareFrontlineGameMode* B,const FFrontlineSquad& Group,
    const wfexp::tactical::NodeDefinition& Post,const wfexp::tactical::NodeDefinition& Goal)
{
    const AWarfareFrontlineSoldier* Weapon=nullptr;bool LiveGun=false;double Range=0.;
    for(const auto& Weak:Group.Members)if(const auto* U=Weak.Get();U&&U->IsActiveCombatant())
    {
        const bool Gun=U->IsMachineGunner();const double Reach=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
        // A surviving ammunition bearer has his actual rifle, never the dead
        // operator's gun profile. Prefer the living/recovered gun if present.
        if(!Weapon||(Gun&&!LiveGun)||(Gun==LiveGun&&Reach>Range))
        {Weapon=U;Range=Reach;LiveGun=Gun;}
    }
    if(!Weapon||Range<=0)return false;
    const FVector Origin=Vector(Post.position)+FVector(0,0,FMath::Clamp(Weapon->GetMuzzleLocation().Z-Weapon->Feet().Z,45.,165.));
    const FVector Toward=(Vector(Post.position)-Vector(Goal.position)).GetSafeNormal2D();
    // Posts are footprints occupied by real men, not ground-level point targets.
    // Test centre and the near occupied edge at torso/head height; no extra
    // weapon range or visibility is granted to the actual firing code.
    for(const FVector Base:{Vector(Goal.position),Vector(Goal.position)+Toward*Goal.radius*.85})
    {
        if(FVector::Dist2D(Vector(Post.position),Base)>Range)continue;
        float Floor=0;if(!TryHeight(B,float(Base.X),float(Base.Y),Floor))continue;
        for(const double Height:{95.,145.})if(HasSight(B,Origin,FVector(Base.X,Base.Y,Floor+Height)))return true;
    }
    return false;
}
void CoverPostsV827(AWarfareFrontlineGameMode* B,const FWorld& R,
    const wfexp::tactical::MissionDefinition& M,const FFrontlineSquad& Group,
    const wfexp::tactical::NodeDefinition& Goal,TArray<const wfexp::tactical::NodeDefinition*>& Out)
{
    const auto* Here=OccupiedNode(Group,M);
    const bool ForwardPost=Here&&!RearPostV827(Here->id);
    const double Direction=Group.Team==0?1.:-1.;
    const double ExistingDistance=Here?FVector::Dist2D(Vector(Here->position),Vector(Goal.position)):MAX_dbl;
    bool ExistingContact=false;
    if(ForwardPost)for(const auto& Weak:Group.Members)if(const auto* U=Weak.Get();U&&U->IsActiveCombatant()&&U->bEngaging)ExistingContact=true;
    for(const auto& Candidate:M.nodes)
    {
        if(Candidate.id==Goal.id||RearPostV827(Candidate.id)||Candidate.id=="wire")continue;
        if(ClaimedPostsV827(B,R,M,Candidate.id,Group.Team,Group.Id)>=Candidate.capacity)continue;
        // Preserve a working firing post. A vacancy on the opposite branch is
        // not an order to pack up and march through the rear assembly again.
        // Only cover relocation has this guard: task/withdraw/manual orders and
        // the explicit rear-ingress release retain their normal legal routes.
        if(ForwardPost&&Candidate.id!=Here->id)
        {
            if(ExistingContact||FVector::Dist2D(Vector(Candidate.position),Vector(Goal.position))>=ExistingDistance-150.
                ||Candidate.position.x*Direction<Here->position.x*Direction-Here->radius)continue;
            const auto Route=PhysicalOrderRouteV87(B,R,Group,*Here,Candidate.id);if(Route.empty())continue;
            std::string Cursor=Here->id;bool RearDetour=false;
            for(const auto& Id:Route)
            {
                const wfexp::tactical::EdgeDefinition* Link=nullptr;for(const auto& E:M.edges)if(E.id==Id){Link=&E;break;}
                if(!Link){RearDetour=true;break;}
                Cursor=Link->from==Cursor?Link->to:Link->from;const auto* Post=Node(M,Cursor);
                if(!Post||RearPostV827(Cursor)||Post->position.x*Direction<Here->position.x*Direction-Here->radius)
                {RearDetour=true;break;}
            }
            if(RearDetour)continue;
        }
        // A gun covers the approach from its own side of the task. A rifle can
        // also contest the neighbouring entry; the actual route remains checked.
        const bool Gun=Group.Role==EFrontlineRole::Lewis||Group.Role==EFrontlineRole::MobileGun;
        if(Gun&&Candidate.position.x*Direction>Goal.position.x*Direction+650.)continue;
        if(Gun&&!(ExistingContact&&Here&&Candidate.id==Here->id)&&!GunCoverCanReachV87(B,Group,Candidate,Goal))continue;
        Out.Add(&Candidate);
    }
    const char* Preferred=Group.Team==0?(R.State.approach=="alternate"?"front_right":"front_left"):
        (Group.Id%2==0?"front_left":"front_right");
    Out.Sort([&](const auto& A,const auto& Z)
    {
        const auto Score=[&](const auto& N)
        {
            const double D=FVector::Dist2D(Vector(N.position),Vector(Goal.position));
            const bool Gun=Group.Role==EFrontlineRole::Lewis||Group.Role==EFrontlineRole::MobileGun;
            return D+(Gun&&D<650.?600.:0.)-(N.id==Preferred?180.:0.);
        };
        return Score(A)<Score(Z);
    });
}
void TickOrdinaryAdvanceV827(AWarfareFrontlineGameMode* B,FWorld& R)
{
    if(R.State.objective.finished||B->Elapsed<R.OrdinaryAdvanceAtV827)return;
    R.OrdinaryAdvanceAtV827=B->Elapsed+.65;const auto* M=Definition(B);
    if(!M||R.State.objective.index>=M->stages.size())return;
    const auto& Stage=M->stages[R.State.objective.index];
    const std::string GoalId=R.State.objective.fallback&&!R.State.objective.fallbackArrived?Stage.fallbackNode:
        R.State.objective.boundObstacle;
    const auto* Goal=Node(*M,GoalId);if(!Goal)return;
    TArray<int32> Sections;
    for(const auto& Group:B->Squads)if(Group.Team==0&&Group.LivingCount()>0&&R.OrdinarySectionsV827.Contains(Group.Id))Sections.Add(Group.Id);
    TMap<int32,double> TaskRouteCosts;
    for(const int32 Id:Sections)if(const auto* Group=B->FindSquad(Id);Group)
    {
        double Cost=MAX_dbl;
        if(const auto* Here=OccupiedNode(*Group,*M);Here)
        {
            if(Here->id==GoalId&&SectionSettledV827(B,R,*Group,*Here))Cost=0.;
            else if(!Group->bAdvancing&&!Group->bHoldPosition&&!ReservedByTacticsV827(B,R,Id))
            {const auto Route=PhysicalOrderRouteV87(B,R,*Group,*Here,GoalId);if(!Route.empty())Cost=PhysicalOrderLengthV87(B,*M,*Group,*Here,Route);}
        }
        TaskRouteCosts.Add(Id,Cost);
    }
    Sections.Sort([&](int32 A,int32 Z)
    {
        const auto* L=B->FindSquad(A);const auto* V=B->FindSquad(Z);
        const bool LP=TaskSectionV827(R,Stage,*L)&&L->LivingCount()>=Stage.minimumLiving;
        const bool VP=TaskSectionV827(R,Stage,*V)&&V->LivingCount()>=Stage.minimumLiving;
        if(LP!=VP)return LP;
        const double LC=TaskRouteCosts.FindChecked(A),VC=TaskRouteCosts.FindChecked(Z);
        if(LC!=VC)return LC<VC;
        // Role is only a tie-breaker after actual reachability and route length.
        const bool LG=Stage.kind==wfexp::tactical::GateKind::HoldNode&&(L->Role==EFrontlineRole::Lewis||L->Role==EFrontlineRole::MobileGun);
        const bool VG=Stage.kind==wfexp::tactical::GateKind::HoldNode&&(V->Role==EFrontlineRole::Lewis||V->Role==EFrontlineRole::MobileGun);
        return LG!=VG?LG:A<Z;
    });
    for(const int32 Id:Sections)
    {
        const auto* Group=B->FindSquad(Id);if(!Group||Group->bAdvancing||Group->bHoldPosition||ReservedByTacticsV827(B,R,Id))continue;
        const auto* Here=OccupiedNode(*Group,*M);if(!Here||!SectionSettledV827(B,R,*Group,*Here))continue;
        auto* ReadyAt=R.OrdinaryReadyAtV827.Find(Id);
        if(!ReadyAt){R.OrdinaryReadyAtV827.Add(Id,B->Elapsed+.8);continue;}if(B->Elapsed<*ReadyAt)continue;
        const bool Participant=TaskSectionV827(R,Stage,*Group);
        if(Here->id=="home"&&!MandatoryIngressSectionV87(R,Stage,*Group)
            &&WaitingMandatoryIngressV87(B,R,Stage,R.Cells,Id))
        {
            // Idle cover sections must not seal the receiving post while the
            // work party waits in the real ingress lane. Move only automatic
            // non-participants, through an ordinary capacity/ground transaction.
            bool Released=false;
            for(const char* Rear:{"assembly","relay","exit"})if(const auto* Post=Node(*M,Rear))
            {
                const auto Owner=R.Owners.find(Rear);
                // Distribute temporary guards instead of filling every bay
                // of the next transit post and sealing the worker's route.
                if((Owner!=R.Owners.end()&&Owner->second==1)||ClaimedPostsV827(B,R,*M,Rear,0,Id)>=FMath::Min(1,Post->capacity))continue;
                if(OrdinaryMoveV827(B,R,Id,Rear))
                {UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_REAR_RELEASE mission=%s squad=%d post=%s host=%.3f"),*R.Mission,Id,*wfexpue::Wide(Rear),B->Elapsed);Released=true;break;}
            }
            if(Released)continue;
        }
        if(Here->id==GoalId&&Participant)
        {
            // Keep an arrived guard at its post. A contested hold resets its
            // timer; that reset must not be interpreted as a request to evict
            // the rifle section for a gun which may still be in the rear.
            // Other real bays can accept reinforcements below. Replacing a
            // guard in a full position remains an explicit player order.
            const bool ArrivalTask=Stage.kind==wfexp::tactical::GateKind::TraverseNode||Stage.kind==wfexp::tactical::GateKind::WithdrawToNode;
            if(ArrivalTask&&(!R.State.objective.fallback||R.State.objective.fallbackArrived)&&!FreshTaskArrivalV827(R,Id))
            {
                // A prior stage's occupancy is not a new traverse receipt.
                // Leave by an actual adjacent edge and return ordinarily.
                // No arrival sequence, actor feet or stage clock is fabricated.
                for(const auto& Link:M->edges)
                {
                    std::string Away;if(Link.from==Here->id)Away=Link.to;else if(Link.bidirectional&&Link.to==Here->id)Away=Link.from;else continue;
                    if(Away=="enemy_assembly")continue;
                    if(OrdinaryMoveV827(B,R,Id,Away))break;
                }
            }
            continue;
        }
        bool Ordered=false;
        bool TaskInProgress=HasTaskGuardV827(B,R,Stage,*Goal);
        for(const auto& Other:B->Squads)if(Other.Team==0&&Other.Id!=Id&&Other.LivingCount()>=Stage.minimumLiving
            &&TaskSectionV827(R,Stage,Other))
        {const auto Path=R.Routes.find(Other.Id);if(Path!=R.Routes.end()&&Other.bAdvancing&&wfexpue::Narrow(Path->second.Target)==GoalId)TaskInProgress=true;}
        bool RetainCommunication=false;
        for(const auto& Required:Stage.mustOwn)if(const auto* Post=Node(*M,Required))
        {
            bool OtherGuard=false;for(const auto& Other:B->Squads)if(Other.Id!=Id&&ActualPostGuardV87(B,R,Other,*Post)){OtherGuard=true;break;}
            if(Here->id==Required&&!OtherGuard){RetainCommunication=true;break;}
            if(!OtherGuard&&!ApproachingPostGuardV87(B,R,*M,Required,Id)&&TaskInProgress&&Here->id!=Required
                &&OrdinaryMoveV827(B,R,Id,Required)){Ordered=true;break;}
        }
        if(RetainCommunication||Ordered)continue;
        if(Participant&&RelieveAutomaticBridgeReservationV87(B,R,*M,Stage,*Group,*Here,*Goal))continue;
        // The nearest legal available task sections fill the real free bays.
        // A distant low-ID gun cannot exclude a rifle already on this approach.
        if(Participant&&(Stage.kind!=wfexp::tactical::GateKind::HoldNode
                ||ClaimedPostsV827(B,R,*M,GoalId,0,Id)<Goal->capacity))
        {
            if(R.State.objective.fallback&&!R.State.objective.fallbackArrived)
            {
                const FString PreviousNotice=R.Notice;TGuardValue<bool> NaturalOrder(R.OrdinaryOrderV827,true);
                Ordered=Command(B,EFRRUTacticalCommandV82::PolicyOrder,{Id},wfexpue::Wide(GoalId));R.Notice=PreviousNotice;
            }
            else Ordered=OrdinaryMoveV827(B,R,Id,GoalId);
        }
        if(Ordered)continue;
        // Keep the hold's communications post manned where possible. It uses
        // another real section; this grants neither ownership nor objective time.
        for(const auto& Required:Stage.mustOwn)if(const auto* Post=Node(*M,Required))
        {
            bool Guard=false;for(const auto& Other:B->Squads)if(ActualPostGuardV87(B,R,Other,*Post)){Guard=true;break;}
            if(!Guard&&!ApproachingPostGuardV87(B,R,*M,Required,Id)&&Here->id!=Required
                &&OrdinaryMoveV827(B,R,Id,Required)){Ordered=true;break;}
        }
        if(Ordered)continue;
        TArray<const wfexp::tactical::NodeDefinition*> Cover;CoverPostsV827(B,R,*M,*Group,*Goal,Cover);
        for(const auto* Post:Cover)
        {
            if(Here->id==Post->id)break;
            if(OrdinaryMoveV827(B,R,Id,Post->id))break;
        }
    }
}
void TickEnemyAdvanceV827(AWarfareFrontlineGameMode* B,FWorld& R)
{
    const auto* M=Definition(B);if(!M)return;
    // Orders are the operation's own junction and fresh visible contacts.
    // A player stage transition, choice, exit order or selected unit is never
    // read here. Rear withdrawal nodes are not an enemy omniscient objective.
    const auto* Goal=Node(*M,OperationPlayV827(R).EnemyJunction);if(!Goal)return;
    const auto& Doctrine=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
    const auto& Contacts=FrontlineDifficultyV30::State(B).Contacts;
    const wfexp::tactical::NodeDefinition* SeenGoal=nullptr;double SeenScore=MAX_dbl;
    for(const auto& Contact:Contacts)
    {
        const double Confidence=FrontlineDifficultyV30::Confidence(Contact,B->Elapsed,Doctrine);if(Confidence<.45)continue;
        for(const auto& Post:M->nodes)if(!RearPostV827(Post.id)&&Post.id!="wire")
        {
            const double Distance=FVector::Dist2D(Contact.Position,Vector(Post.position));
            if(Distance<=Post.radius+650.&&Distance/Confidence<SeenScore){SeenGoal=&Post;SeenScore=Distance/Confidence;}
        }
    }
    // A rejected intermediate leg is a dependency, not permission to pass
    // through an enemy post. Only fresh enemy contact can turn that dependency
    // into an assault task for another real section still idle in its rear.
    TArray<const wfexp::tactical::NodeDefinition*> ObservedBlockedPostsV87;
    if(R.Identity.nations[0]==wfexp::Faction::French&&R.Identity.nations[1]==wfexp::Faction::German)
    for(const auto& Waiting:B->Squads)if(Waiting.Team==1&&Waiting.LivingCount()>0&&Waiting.bAdvancing&&!Waiting.bHoldPosition)
    {
        const auto Path=R.Routes.find(Waiting.Id);if(Path==R.Routes.end())continue;
        const auto Next=NextNode(Path->second,*M);const auto* Post=Node(*M,Next);
        if(!Post||RearPostV827(Next)||Next=="wire"||!HostileIntermediateReceiverBlockedV87(R,*M,Waiting,Next)
            ||RouteDepartureAllowed(B,*M,R,Waiting,Path->second))continue;
        for(const auto& Contact:Contacts)
            if(FrontlineDifficultyV30::Confidence(Contact,B->Elapsed,Doctrine)>=.45
                &&FVector::Dist2D(Contact.Position,Vector(Post->position))<=Post->radius+650.)
            {ObservedBlockedPostsV87.AddUnique(Post);break;}
    }
    for(const auto& Group:B->Squads)if(Group.Team==1&&Group.LivingCount()>0&&!Group.bAdvancing&&!Group.bHoldPosition)
    {
        const auto* Here=OccupiedNode(Group,*M);if(!Here||!SectionSettledV827(B,R,Group,*Here))continue;
        const bool Gun=Group.Role==EFrontlineRole::Lewis||Group.Role==EFrontlineRole::MobileGun;
        bool AssaultOrderedV87=false;
        if(Here->id=="enemy_assembly"&&(Group.Role==EFrontlineRole::Rifle||Group.Role==EFrontlineRole::Mills
            ||Group.Role==EFrontlineRole::RifleGrenadier))
            for(const auto* Post:ObservedBlockedPostsV87)
            {
                if(ApproachingPostGuardV87(B,R,*M,Post->id,Group.Id,1))continue;
                if(!MoveSquad(B,Group.Id,wfexpue::Wide(Post->id)))continue;
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ENEMY_BLOCKED_POST_ASSAULT mission=%s squad=%d target=%s host=%.3f observed=1 normal_move=1"),
                    *R.Mission,Group.Id,*wfexpue::Wide(Post->id),B->Elapsed);
                AssaultOrderedV87=true;break;
            }
        if(AssaultOrderedV87)continue;
        const auto* Target=!Gun&&SeenGoal?SeenGoal:Goal;
        if(Here->id==Target->id)continue;
        TArray<const wfexp::tactical::NodeDefinition*> Targets;Targets.Add(Target);
        CoverPostsV827(B,R,*M,Group,*Target,Targets);
        // Already settled guns retain their arc while rifle sections press the
        // junction. A new gun advances from its rear; it does not shuffle among
        // equally close posts every decision interval.
        if(Gun&&!RearPostV827(Here->id)&&FVector::Dist2D(Vector(Here->position),Vector(Goal->position))<3500.)continue;
        for(const auto* Post:Targets)
        {
            if(Here->id==Post->id)break;
            if(R.Identity.nations[1]==wfexp::Faction::AustroHungarian&&Here->id=="enemy_assembly")
            {
                if(R.State.counter[1].pending||B->Elapsed<R.State.counter[1].readyAt)break;
                const FString Notice=R.Notice;
                if(Command(B,EFRRUTacticalCommandV82::CounterOrder,{Group.Id},wfexpue::Wide(Post->id),FString(),FString(),0,1))break;
                R.Notice=Notice;
            }
            else if(MoveSquad(B,Group.Id,wfexpue::Wide(Post->id)))break;
        }
    }
}
}
