#include "WarfareFrontline.h"
#include "WarfareFrontlineChoiceTargetsV78.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineBattlefield.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineEnemyPolicyV84415.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineCampaignV32.h"
#include "Engine/World.h"

namespace FrontlineEnemyPolicyV84415
{
EFrontlineRole CounterRole(const FRecruitSituation& S)
{
    auto Can=[&](EFrontlineRole R){return int32(R)<11&&(S.AvailableRoles&(1u<<int32(R)))!=0;};
    auto Count=[&](EFrontlineRole R){return S.OwnRoles[int32(R)];};
    const bool Experienced=S.Difficulty!=EFrontlineDifficulty::Rookie;
    const int32 GunGoal=FMath::Clamp(1+S.SeenRifles/9,1,S.Difficulty==EFrontlineDifficulty::Survivor?4:3);
    // An archive grants equipment, not random recruitment. A current threat
    // precedes the reserve rotation just as it does in an ordinary operation.
    if(Experienced&&S.SeenArmour>0)
    {
        const auto AT=S.Faction==EFrontlineFaction::German?EFrontlineRole::AntiTankRifle:EFrontlineRole::RifleGrenadier;
        if(Can(AT)&&Count(AT)<FMath::Min(2,S.SeenArmour))return AT;
        if(Can(EFrontlineRole::Mills)&&Count(EFrontlineRole::Mills)==0)return EFrontlineRole::Mills;
    }
    if(S.SeenRifles>=6&&S.OwnGuns<GunGoal&&Can(EFrontlineRole::Lewis))return EFrontlineRole::Lewis;
    const auto Precision=FrontlineChoiceTargetsV78::PrecisionCounter(S.Faction);
    if(S.SeenMarksmen>=2&&Count(Precision)==0&&Can(Precision))return Precision;
    if(S.Counterattack&&Count(EFrontlineRole::Mills)<2&&Can(EFrontlineRole::Mills))return EFrontlineRole::Mills;
    if(Experienced&&S.OwnLiving>=9&&Count(EFrontlineRole::Officer)==0&&Can(EFrontlineRole::Officer))return EFrontlineRole::Officer;
    if(S.SeenEntrenched>=3)
    {
        if(Count(EFrontlineRole::Mills)==0&&Can(EFrontlineRole::Mills))return EFrontlineRole::Mills;
        if(Experienced&&S.NearestContact<4200&&Count(EFrontlineRole::Flamethrower)==0&&Can(EFrontlineRole::Flamethrower))return EFrontlineRole::Flamethrower;
        if(Experienced&&Count(EFrontlineRole::RifleGrenadier)==0&&Can(EFrontlineRole::RifleGrenadier))return EFrontlineRole::RifleGrenadier;
    }
    if(Experienced&&S.OwnLiving>=6&&(S.MineDensity>=.30f||S.WireDensity>=.70f)
        &&Count(EFrontlineRole::Pioneer)==0&&Can(EFrontlineRole::Pioneer))return EFrontlineRole::Pioneer;
    if(S.SeenGuns>0&&Count(EFrontlineRole::Marksman)==0&&Can(EFrontlineRole::Marksman))return EFrontlineRole::Marksman;
    if(S.OwnGuns==0&&Can(EFrontlineRole::Lewis))return EFrontlineRole::Lewis;
    return EFrontlineRole::Count;
}
namespace
{
int32 EarliestResearch(EFrontlineFaction F,int32 Id)
{
    auto Arrival=[&](EFrontlineRole R){return FrontlineCampaignV41::RoleArrival(F,R);};
    switch(Id)
    {
        case 2:case 7:case 8:case 13:return Id==13&&F==EFrontlineFaction::British?Arrival(EFrontlineRole::Marksman):Arrival(EFrontlineRole::Lewis);
        case 4:case 5:return Arrival(EFrontlineRole::Mills);
        case 6:return F==EFrontlineFaction::German?Arrival(EFrontlineRole::Officer):1;
        case 9:return F==EFrontlineFaction::British?Arrival(EFrontlineRole::Officer):Arrival(EFrontlineRole::Mills);
        case 12:case 14:case 15:return F==EFrontlineFaction::German?11:Id==12?Arrival(EFrontlineRole::Marksman):4;
        case 16:return 7;
        case 17:return Arrival(EFrontlineRole::Pioneer);
        case 18:return Arrival(EFrontlineRole::Flamethrower);
        case 19:case 21:return 8;
        case 20:return 13;
        case 22:return 17;
        case 23:return 4;
        case 24:return F==EFrontlineFaction::German?Arrival(EFrontlineRole::AntiTankRifle):Arrival(EFrontlineRole::RifleGrenadier);
        case 25:return Arrival(EFrontlineRole::Lewis);
        case 26:return Arrival(EFrontlineRole::Officer);
        case 27:return Arrival(EFrontlineRole::MobileGun);
        case 3:case 10:case 11:return 2;
        default:return 0;
    }
}
}
uint32 ResearchMask(EFrontlineFaction F,int32 Operation,EFrontlineDifficulty Difficulty)
{
    if(Operation<0||Operation>=20||uint8(F)>1||uint8(Difficulty)>2||Difficulty==EFrontlineDifficulty::Rookie)return 0;
    // Same two-point initial purse and earned-operation scale as a company
    // record. Survivor spends three per completed operation, Veteran two.
    // This fixed syllabus never inherits the player's choices or earned points.
    const int32 Key=Operation+int32(F)*20+int32(Difficulty)*40;
    static TMap<int32,uint32> Cached;
    if(const auto* Found=Cached.Find(Key))return *Found;
    int32 Budget=2+Operation*(Difficulty==EFrontlineDifficulty::Survivor?3:2);
    const auto& Nodes=AWarfareFrontlineGameMode::GetTechnologyNodesForFaction(F);
    static constexpr int32 Order[]={0,2,4,1,5,3,7,10,6,8,9,12,13,11,14,15,17,18,19,24,25,26,22,23,27,20,21,16};
    // Previously issued doctrine stays issued when a new specialist arrives.
    // Rebuilding a greedy purse from zero would quietly revoke older training.
    uint32 Mask=Operation>0?ResearchMask(F,Operation-1,Difficulty):0;
    for(const auto& N:Nodes)if(Mask&(1u<<N.Id))Budget-=N.Cost;
    bool Changed=true;
    while(Changed)
    {
        Changed=false;
        for(int32 Id:Order)
        {
            if(!Nodes.IsValidIndex(Id)||Nodes[Id].Id!=Id||(Mask&(1u<<Id))||Operation<EarliestResearch(F,Id))continue;
            const auto& N=Nodes[Id];
            if(N.Cost<=0||N.Cost>Budget||(N.Prerequisite>=0&&!(Mask&(1u<<N.Prerequisite)))
                ||(N.SecondaryPrerequisite>=0&&!(Mask&(1u<<N.SecondaryPrerequisite))))continue;
            // The MP18 is a 1918 attachment even when earlier research branches
            // could afford it. UK issued sniper equipment keeps its role gate.
            if(F==EFrontlineFaction::German&&(Id==12||Id==14||Id==15))
            {
                const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
                if(!Missions.IsValidIndex(Operation)||Missions[Operation].CalendarDateV41<19180321)continue;
            }
            Mask|=1u<<Id;Budget-=N.Cost;Changed=true;
        }
    }
    Cached.Add(Key,Mask);return Mask;
}
bool ResearchAvailable(const AWarfareFrontlineGameMode* B,int32 Id)
{
    if(!B||Id<0||Id>=int32(EFrontlineTechnology::Count))return false;
    const auto* M=B->GetEncounterCampaignMission();if(!M)M=B->GetSelectedCampaignMission();
    return M&&(ResearchMask(B->GetFactionForTeam(1),M->Id,B->GetCampaignDifficulty())&(1u<<Id))!=0;
}
bool RetainAnchor(EFrontlineDifficulty D,int32 Friendly,int32 Anchor,float Observed,float Remembered,bool Threat,bool RearRelief)
{
    if(D==EFrontlineDifficulty::Rookie||RearRelief)return false;
    // A safe vacant forward position does not require the final whole section
    // to remain at the last bay forever. Known defenders still retain a gun.
    if(!Threat&&Observed<=0&&Remembered<.5f&&Friendly>=6&&Friendly<=Anchor)return false;
    return true;
}
float WaveRestScale(EFrontlineDifficulty D,int32 Operation,bool Threatened)
{
    if(D==EFrontlineDifficulty::Rookie||Operation<3)return 1.f;
    if(D==EFrontlineDifficulty::Veteran)return Threatened?.88f:1.f;
    return Threatened?.72f:Operation>=11?.88f:.94f;
}
}

namespace
{
bool ObserveArmourV84415(const AWarfareFrontlineGameMode* B,const AFrontlineTank* Tank)
{
    if(!B||!Tank||Tank->Team!=0||!Tank->IsOperational()||!Tank->HasEnteredField())return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(V84415ObservedArmour),true,Tank);
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Query.AddIgnoredActor(U);
    for(const AWarfareFrontlineSoldier* Observer:B->Units)if(Observer&&Observer->Team==1&&Observer->IsActiveCombatant())
    {
        const auto Point=Tank->ObservedArmourAimPointV837(Observer->Feet());FHitResult Hit;
        if(FVector::Dist2D(Observer->Feet(),Point)<=6000.f*B->EncounterVisibilityScale()
            &&B->DirectFireMultiplier(Observer->Feet(),Point)>=.9f
            &&!B->GetWorld()->LineTraceSingleByChannel(Hit,Observer->Feet()+FVector(0,0,145),Point,ECC_Visibility,Query))return true;
    }
    return false;
}
struct FFirePlanV84415
{
    FGuid Encounter;bool Pending=false;
    EFrontlineSupport Type=EFrontlineSupport::Artillery;
    FVector Point=FVector::ZeroVector;float ExecuteAt=0,ObservedAt=0,AssessAt=0;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FFirePlanV84415> FirePlansV84415;
FFirePlanV84415& FirePlanV84415(AWarfareFrontlineGameMode* B,float InitialAssessment=0.f)
{
    for(auto I=FirePlansV84415.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
    auto& S=FirePlansV84415.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){S=FFirePlanV84415();S.Encounter=B->GetEncounterId();S.AssessAt=InitialAssessment;}return S;
}
void ServiceFireV84415(AWarfareFrontlineGameMode* B,const TArray<AFrontlineTank*,TInlineAllocator<8>>& Armour,float InitialAssessment,FRandomStream& Random)
{
    auto& S=FirePlanV84415(B,InitialAssessment);auto& T=FrontlineDifficultyV30::State(B);const auto& D=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
    const float Now=B->Elapsed;
    if(S.Pending)
    {
        if(Now<S.ExecuteAt)return;
        if(Now-S.ObservedAt<D.MemorySeconds&&Now-T.LastSupportAt>=D.FireCoordinationGap
            &&B->IsFireSupportAreaSafe(S.Point,S.Type,1)&&B->RequestSupport(S.Type,S.Point,1))
        {T.LastSupportAt=Now;++T.PlannedFireMissions;}
        S.Pending=false;S.AssessAt=Now+Random.FRandRange(D.AssessMin,D.AssessMax);return;
    }
    if(Now<S.AssessAt||Now-T.LastSupportAt<D.FireCoordinationGap)return;
    S.AssessAt=Now+Random.FRandRange(D.AssessMin,D.AssessMax);
    float Best=D.MinimumTargetValue;FVector Point=FVector::ZeroVector;float ObservedAt=0;EFrontlineSupport Type=EFrontlineSupport::Artillery;
    FrontlineDifficultyV30::FContact RegisteredContact;
    auto Offer=[&](EFrontlineSupport Kind,const FVector& P,float Score,float SeenAt,const FrontlineDifficultyV30::FContact* Observed=nullptr)
    {
        if(Score>Best&&B->SupportBlockReason(Kind,1).IsEmpty()&&B->IsFireSupportAreaSafe(P,Kind,1))
        {Best=Score;Point=P;Type=Kind;ObservedAt=SeenAt;RegisteredContact=Observed?*Observed:FrontlineDifficultyV30::FContact();RegisteredContact.Position=P;RegisteredContact.LastSeen=SeenAt;}
    };
    for(const auto* Tank:Armour)Offer(EFrontlineSupport::AntiTank,Tank->ObservedArmourAimPointV837(FVector::ZeroVector),6.f,Now);
    for(const auto& C:T.Contacts)
    {
        const float Confidence=FrontlineDifficultyV30::Confidence(C,Now,D);if(Confidence<=.35f)continue;
        float Fixed=0,Moving=0,Exposed=0;
        for(const auto& N:T.Contacts)
        {
            const float Certainty=FrontlineDifficultyV30::Confidence(N,Now,D);
            if(FMath::Abs(N.Position.X-C.Position.X)>1000||FMath::Abs(N.Position.Y-C.Position.Y)>650)continue;
            if(N.Moving)Moving+=Certainty;else Fixed+=Certainty;
            if(!N.Entrenched)Exposed+=Certainty;
        }
        const float Value=FrontlineDifficultyV30::TargetScore(C,T.Contacts,Now,D);
        Offer(EFrontlineSupport::Mortar,C.Position,Value+(C.Entrenched?.35f:0.f),C.LastSeen,&C);
        Offer(EFrontlineSupport::Artillery,C.Position,Value+(C.Machinegun?.25f:0.f),C.LastSeen,&C);
        if(Fixed>=4)Offer(EFrontlineSupport::Gas,C.Position,(Fixed*.90f+(C.Entrenched?1.f:0.f))*Confidence,C.LastSeen,&C);
        if(Exposed>=4)Offer(EFrontlineSupport::AirRecon,C.Position,Exposed*1.05f*Confidence,C.LastSeen,&C);
        if(Moving>=4)Offer(EFrontlineSupport::AirStrike,C.Position,Moving*1.10f*Confidence,C.LastSeen,&C);
        if(Fixed>=4)Offer(EFrontlineSupport::CreepingBarrage,C.Position,(Fixed*.85f+(C.Machinegun?1.f:0.f))*Confidence,C.LastSeen,&C);
        if(Fixed+Moving>=6)Offer(EFrontlineSupport::AirBombing,C.Position,(Fixed+Moving)*.95f*Confidence,C.LastSeen,&C);
        // A prepared direct-fire battery is useful against a held infantry gun
        // even if the opposing company has never sent armour into view.
        if(Fixed>=3)Offer(EFrontlineSupport::AntiTank,C.Position,Fixed*.75f*Confidence,C.LastSeen,&C);
    }
    if(Best>D.MinimumTargetValue)
    {
        S.Point=Point;S.Type=Type;S.ObservedAt=ObservedAt;S.ExecuteAt=Now+Random.FRandRange(D.ReactionMin,D.ReactionMax);S.Pending=true;
        // Registration retains real error and modest observed lead. No actor
        // is read after commitment and no strike follows a live target.
        S.Point=FrontlineDifficultyV30::RegisteredPoint(RegisteredContact,Now,D,Random);
    }
}
}

EFrontlineRole FrontlineArchiveEnemyRoleV84412(const AWarfareFrontlineGameMode* B,int32 RecruitStep)
{
    if(!FrontlineUltimateArchiveV8215::FullArmory(B,1)||RecruitStep<2||RecruitStep%3!=2)return EFrontlineRole::Count;
    TArray<EFrontlineRole,TInlineAllocator<10>> Roles;
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
        if(FrontlineGameplayV33::EnemyRoleAvailable(B,EFrontlineRole(I)))Roles.Add(EFrontlineRole(I));
    return Roles.IsEmpty()?EFrontlineRole::Count:Roles[((RecruitStep-2)/3)%Roles.Num()];
}

bool AWarfareFrontlineGameMode::CanEnemyObserve(const AWarfareFrontlineSoldier* Unit) const
{
    return FrontlineDifficultyV30::CanObserve(this,Unit);
}

void AWarfareFrontlineGameMode::ThinkEnemy()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_EnemyDecision);
    if (Phase!=EFrontlinePhase::Fighting) return;
    const auto& Doctrine=FrontlineDifficultyV30::Doctrine(GetCampaignDifficulty());
    auto& Tactics=FrontlineDifficultyV30::State(this);
    TArray<AWarfareFrontlineSoldier*> Seen;
    int32 RoleCounts[RoleCount]={}; int32 SeenGuns=0,SeenEntrenched=0,SeenRifles=0,SeenMarksmen=0,OwnGuns=0;
    float OwnPressure=0; int32 OwnLiving=0;
    for (AWarfareFrontlineSoldier* U:Units) if (U->IsAlive())
    {
        if (U->Team==1) { OwnPressure+=U->Suppression; ++OwnLiving;OwnGuns+=U->IsMachineGunner()?1:0; }
        else if (CanEnemyObserve(U)) { Seen.Add(U); SeenGuns+=U->IsAutomaticWeapon()?1:0; SeenEntrenched+=CoverFor(U)>0?1:0;SeenRifles+=U->Role==EFrontlineRole::Rifle?1:0;SeenMarksmen+=U->Role==EFrontlineRole::Marksman?1:0; }
    }
    FrontlineDifficultyV30::Observe(this,Seen);
    for (const auto& S:Squads) if (S.Team==1 && S.LivingCount()>0) ++RoleCounts[int32(S.Role)];
    TArray<AFrontlineTank*,TInlineAllocator<8>> SeenArmour;
    if(GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie)
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld()))
            for(AFrontlineTank* Tank:Fleet->GetVehicles())if(ObserveArmourV84415(this,Tank))SeenArmour.Add(Tank);
    const auto* Mission=GetEncounterCampaignMission();
    const bool Counterattack=EnemyCounterattackTrench!=INDEX_NONE && Elapsed<EnemyCounterattackUntil;
    // A role is committed until it assembles. Rifle availability cannot starve
    // the longer specialist preparation. Contact changes the next commitment.
    if (Elapsed>=EnemyRecruitAt)
    {
        if (!bEnemyRoleCommitted)
        {
            const EFrontlineRole PrecisionCounter=FrontlineChoiceTargetsV78::PrecisionCounter(GetFactionForTeam(1));
            const bool Archive=FrontlineUltimateArchiveV8215::FullArmory(this,1);
            const auto ArchiveReserve=FrontlineArchiveEnemyRoleV84412(this,EnemyRecruitStep);
            FrontlineEnemyPolicyV84415::FRecruitSituation Situation;Situation.Difficulty=GetCampaignDifficulty();Situation.Faction=GetFactionForTeam(1);
            Situation.OwnLiving=OwnLiving;Situation.OwnGuns=OwnGuns;Situation.SeenRifles=SeenRifles;Situation.SeenMarksmen=SeenMarksmen;
            Situation.SeenGuns=SeenGuns;Situation.SeenEntrenched=SeenEntrenched;Situation.SeenArmour=SeenArmour.Num();Situation.Counterattack=Counterattack;Situation.Archive=Archive;
            for(int32 I=0;I<RoleCount;++I){Situation.OwnRoles[I]=RoleCounts[I];if(FrontlineGameplayV33::EnemyRoleAvailable(this,EFrontlineRole(I)))Situation.AvailableRoles|=1u<<I;}
            if(Mission){Situation.WireDensity=Mission->WireDensity;Situation.MineDensity=Mission->MineDensity;}
            for(const auto* Contact:Seen)for(const AWarfareFrontlineSoldier* Own:Units)if(Own&&Own->Team==1&&Own->IsActiveCombatant())
                Situation.NearestContact=FMath::Min(Situation.NearestContact,float(FVector::Dist2D(Contact->Feet(),Own->Feet())));
            const auto CounterRole=FrontlineEnemyPolicyV84415::CounterRole(Situation);
            if (!Archive&&Mission && Mission->EnemyOpeningRoles.IsValidIndex(EnemyRecruitStep)) EnemyPlannedRole=Mission->EnemyOpeningRoles[EnemyRecruitStep];
            else if(CounterRole!=EFrontlineRole::Count&&OwnLiving>0&&(GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie||Archive))EnemyPlannedRole=CounterRole;
            else if(ArchiveReserve!=EFrontlineRole::Count)EnemyPlannedRole=ArchiveReserve;
            else if (OwnLiving==0) EnemyPlannedRole=EFrontlineRole::Rifle;
            else if (SeenRifles>=6 && OwnGuns<FMath::Clamp(1+SeenRifles/9,1,3)
                && FrontlineGameplayV33::EnemyRoleAvailable(this,EFrontlineRole::Lewis)) EnemyPlannedRole=EFrontlineRole::Lewis;
            else if (SeenMarksmen>=2 && RoleCounts[int32(PrecisionCounter)]==0 && FrontlineGameplayV33::EnemyRoleAvailable(this,PrecisionCounter)) EnemyPlannedRole=PrecisionCounter;
            else if (Counterattack && RoleCounts[int32(EFrontlineRole::Mills)]<2) EnemyPlannedRole=EFrontlineRole::Mills;
            else if (Counterattack && RoleCounts[int32(EFrontlineRole::Officer)]==0) EnemyPlannedRole=EFrontlineRole::Officer;
            else if (Counterattack && RoleCounts[int32(EFrontlineRole::MobileGun)]==0 && FrontlineGameplayV33::EnemyRoleAvailable(this,EFrontlineRole::MobileGun)) EnemyPlannedRole=EFrontlineRole::MobileGun;
            else if (SeenEntrenched>=3 && OwnLiving>=6 && RoleCounts[int32(EFrontlineRole::Officer)]==0
                && FrontlineGameplayV33::EnemyRoleAvailable(this,EFrontlineRole::Officer) && IsSupportUnlocked(EFrontlineSupport::Mortar,1)) EnemyPlannedRole=EFrontlineRole::Officer;
            else if (SeenGuns>0 && RoleCounts[int32(EFrontlineRole::Marksman)]==0) EnemyPlannedRole=EFrontlineRole::Marksman;
            else if (RoleCounts[int32(EFrontlineRole::Lewis)]+RoleCounts[int32(EFrontlineRole::MobileGun)]==0) EnemyPlannedRole=EFrontlineRole::Lewis;
            else if ((OwnPressure/FMath::Max(1,OwnLiving)>.4f || Morale[1]<.62f*FrontlineMoraleV36::StartingMaximum(this,1)) && RoleCounts[int32(EFrontlineRole::Officer)]==0) EnemyPlannedRole=EFrontlineRole::Officer;
            else if (SeenEntrenched>=3 && RoleCounts[int32(EFrontlineRole::Mills)]==0) EnemyPlannedRole=EFrontlineRole::Mills;
            else if(Archive)
            {
                TArray<EFrontlineRole,TInlineAllocator<10>> Roles;
                for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
                    if(FrontlineGameplayV33::EnemyRoleAvailable(this,EFrontlineRole(I)))Roles.Add(EFrontlineRole(I));
                EnemyPlannedRole=Roles.IsEmpty()?EFrontlineRole::Rifle:Roles[EnemyRecruitStep%Roles.Num()];
            }
            else if (Mission && Mission->EnemyRoleOrder.Num()) EnemyPlannedRole=Mission->EnemyRoleOrder[EnemyRecruitStep%Mission->EnemyRoleOrder.Num()];
            else EnemyPlannedRole=EnemyDecisionRandom.FRand()<.30f?EFrontlineRole::Mills:EFrontlineRole::Rifle;
            // The introductory opponent has an authored company assignment,
            // independent of a replaying player's later research/loadout. A
            // tactical preference cannot bring an unintroduced specialist in.
            if(!FrontlineGameplayV33::EnemyRoleAvailable(this,EnemyPlannedRole))
            {
                EnemyPlannedRole=EFrontlineRole::Rifle;
                if(Mission)for(int32 N=0;N<Mission->EnemyRoleOrder.Num();++N)
                {
                    const auto Candidate=Mission->EnemyRoleOrder[(EnemyRecruitStep+N)%Mission->EnemyRoleOrder.Num()];
                    if(FrontlineGameplayV33::EnemyRoleAvailable(this,Candidate)){EnemyPlannedRole=Candidate;break;}
                }
            }
            bEnemyRoleCommitted=true;
        }
        if (RecruitBlockReason(EnemyPlannedRole,1).IsEmpty() && Recruit(EnemyPlannedRole,1))
        {
            ++EnemyRecruitStep; bEnemyRoleCommitted=false;
            EnemyRecruitAt=Elapsed+FrontlineGameplayV43::PreparationSeconds(EnemyDecisionRandom.FRandRange(1.6f,3.2f)*Doctrine.DecisionScale*FrontlinePacingV24::TempoIntervalScale,1,GetCampaignDifficulty());
            ++EnemyWaveRecruitCount;
            if (EnemyWaveRecruitCount>=3)
            {
                EnemyWaveRecruitCount=0;
                const bool Opening=FrontlineGameplayV33::IsOpeningAssembly(Mission?Mission->Id:INDEX_NONE,EnemyRecruitStep,
                    LivingSoldiers(1),Elapsed,!Tactics.Contacts.IsEmpty());
                EnemyWaveStartedAt=Elapsed+FrontlineGameplayV43::PreparationSeconds(EnemyDecisionRandom.FRandRange(Counterattack?5.f:Opening?6.f:14.f,
                    Counterattack?8.f:Opening?9.f:20.f)*Doctrine.WaveRestScale*FrontlinePacingV24::TempoIntervalScale
                    *FrontlineEnemyPolicyV84415::WaveRestScale(GetCampaignDifficulty(),Mission?Mission->Id:INDEX_NONE,!Seen.IsEmpty()||Counterattack),1,GetCampaignDifficulty());
                EnemyRecruitAt=FMath::Max(EnemyRecruitAt,EnemyWaveStartedAt);
                UE_LOG(LogTemp,Display,TEXT("V18 echelon next_at=%.2f counterattack=%d recruited=%d"),EnemyWaveStartedAt,int32(Counterattack),EnemyRecruitStep);
            }
        }
    }
    if (Elapsed>=EnemyAssaultAt)
    {
        EnemyAssaultAt=Elapsed+EnemyDecisionRandom.FRandRange(5.f,8.f)*Doctrine.AssaultIntervalScale*FrontlinePacingV24::TempoIntervalScale;
        const int32 EnemyDir=GetTeamDirection(1);
        // Consider the foremost occupied position first in either physical direction.
        for (int32 Index=EnemyDir<0?0:Trenches.Num()-1;Trenches.IsValidIndex(Index);Index-=EnemyDir) if (Trenches[Index].Owner==1)
        {
            int32 Friendly=0,Opposition=0; float Pressure=0.f; bool Gun=false;
            bool KeepReserve=GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie;
            const FFrontlineSquad* Anchor=nullptr;bool AnchorGun=false;
            const int32 NextIndex=Index+EnemyDir;
            const float TargetX=Trenches.IsValidIndex(NextIndex)?Trenches[NextIndex].X:EnemyDir*FrontlineBattlefield::GoalHalfLength;
            for (const auto& S:Squads) if (S.Team==1 && S.Trench==Index && S.LivingCount()>0)
            {
                Friendly+=S.LivingCount();bool LivingGun=false;
                for(const auto& Member:S.Members)if(Member.IsValid()&&Member->IsAlive()&&Member->IsMachineGunner())LivingGun=true;
                if(!Anchor||(LivingGun&&!AnchorGun)||(LivingGun==AnchorGun&&S.LivingCount()>Anchor->LivingCount()))
                {Anchor=&S;AnchorGun=LivingGun;}
            }
            if (!Friendly) continue;
            for (auto* U:Seen) if (FMath::Abs(U->Feet().X-TargetX)<1100
                ||((U->Feet().X-Trenches[Index].X)*EnemyDir>=-400.f&&(U->Feet().X-TargetX)*EnemyDir<=700.f))
            { ++Opposition; Pressure+=U->Suppression; Gun|=U->IsAutomaticWeapon(); }
            const bool LocalCounterattack=Counterattack && NextIndex==EnemyCounterattackTrench;
            const float Remembered=FrontlineDifficultyV30::RememberedStrength(Tactics.Contacts,Elapsed,Doctrine,TargetX);
            // A gun keeps the threatened front, not an abandoned rear bay.
            // Reposition only through an already friendly next trench and when
            // neither current observation nor remembered contact challenges it.
            bool LocalThreat=false;
            for(auto* Contact:Seen)LocalThreat|=FMath::Abs(Contact->Feet().X-Trenches[Index].X)<1600.f;
            const bool RearRelief=Trenches.IsValidIndex(NextIndex)&&Trenches[NextIndex].Owner==1
                &&Opposition==0&&Remembered<.5f&&!LocalThreat;
            KeepReserve=FrontlineEnemyPolicyV84415::RetainAnchor(GetCampaignDifficulty(),Friendly,Anchor?Anchor->LivingCount():0,
                Opposition,Remembered,LocalThreat,RearRelief);
            const int32 Available=Friendly-(KeepReserve&&Anchor?Anchor->LivingCount():0);
            const bool CanPush=RearRelief||FrontlineDifficultyV30::CanAssault(Available,Opposition,Remembered,
                Pressure/FMath::Max(1,Opposition),Gun,LocalCounterattack);
            if (!CanPush) continue;
            // A command post knows its own committed barrage. Do not send a
            // wave into shells already on the way to its next position.
            bool OwnBarrage=false;
            for(const auto& Strike:Strikes)if(Strike.Team==1 && !Strike.bResolved && !Strike.bGrenade
                && FMath::Abs(Strike.Target.X-TargetX)<Strike.Radius+550 && Strike.ImpactAt>Elapsed)OwnBarrage=true;
            if(OwnBarrage && GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie)continue;
            TArray<int32> Movers;
            for (const auto& S:Squads) if (S.Team==1 && S.Trench==Index && S.LivingCount()>0 && !S.bHoldPosition)
            {
                if(KeepReserve&&Anchor&&S.Id==Anchor->Id)continue;
                bool LivingGun=false;
                for (const auto& Member:S.Members) if (Member.IsValid() && Member->IsAlive() && Member->IsMachineGunner()) LivingGun=true;
                if (!LivingGun || !KeepReserve || (Opposition==0&&Available>=8)) Movers.Add(S.Id);
            }
            int32 Moved=0; for (int32 Id:Movers) if (AdvanceSquad(Index,Id,1)) ++Moved;
            if (Moved>0) break;
        }
    }
    AWarfareFrontlineSoldier* Forward=nullptr;
    for (AWarfareFrontlineSoldier* U:Units) if (U->Team==1 && U->IsAlive() && (!Forward || U->Feet().X*GetTeamDirection(1)>Forward->Feet().X*GetTeamDirection(1))) Forward=U;
    if (Forward && !Seen.IsEmpty() && SmokeBlockReason(1).IsEmpty()
        && ((Forward->bMoving && Forward->Suppression>Doctrine.SmokePressure) || (SeenMarksmen>=2 && Forward->Suppression>.12f)))
    {
        const FVector Screen=Forward->Feet()+FVector(GetTeamDirection(1)*600.f,0,0);
        if (DirectFireMultiplier(Forward->Feet(),Screen)>=.95f) RequestSmoke(Screen,1);
    }
    auto SafeTarget=[this](const FVector& Target,EFrontlineSupport Type)
    {return IsFireSupportAreaSafe(Target,Type,1);};
    if(GetCampaignDifficulty()!=EFrontlineDifficulty::Rookie)
    {
        // One observed policy selects among every currently ready support.
        // Existing extended batteries share LastSupportAt, so none may mirror
        // a player call or overlap this commitment without the same pause.
        bEnemyFirePending=false;ServiceFireV84415(this,SeenArmour,EnemyFireAssessAt,EnemyDecisionRandom);return;
    }
    // This fire-control layer consumes last observed positions. It never reads
    // player support selections, cooldowns, commands or unseen actor state.
    if (bEnemyFirePending && Elapsed>=EnemyFireExecuteAt)
    {
        const EFrontlineSupport Type=bEnemyPendingMortar?EFrontlineSupport::Mortar:EFrontlineSupport::Artillery;
        if (Elapsed-Tactics.LastSupportAt>=Doctrine.FireCoordinationGap && SafeTarget(EnemyObservedFireTarget,Type))
        {
            const bool Fired=bEnemyPendingMortar?RequestMortar(EnemyObservedFireTarget,1):RequestArtillery(EnemyObservedFireTarget,1);
            if (Fired)
            {
                Tactics.LastSupportAt=Elapsed;++Tactics.PlannedFireMissions;
                UE_LOG(LogTemp,Display,TEXT("V30 observed fire difficulty=%d t=%.2f mortar=%d x=%.0f y=%.0f"),int32(GetCampaignDifficulty()),Elapsed,bEnemyPendingMortar,EnemyObservedFireTarget.X,EnemyObservedFireTarget.Y);
            }
        }
        bEnemyFirePending=false; EnemyFireAssessAt=Elapsed+EnemyDecisionRandom.FRandRange(Doctrine.AssessMin,Doctrine.AssessMax);
    }
    if (!bEnemyFirePending && Elapsed>=EnemyFireAssessAt)
    {
        EnemyFireAssessAt=Elapsed+EnemyDecisionRandom.FRandRange(Doctrine.AssessMin,Doctrine.AssessMax);
        if(Elapsed-Tactics.LastSupportAt<Doctrine.FireCoordinationGap)return;
        const bool CannonReady=ArtilleryBlockReason(1).IsEmpty(),MortarReady=MortarBlockReason(1).IsEmpty();
        if (!CannonReady && !MortarReady) return;
        const FrontlineDifficultyV30::FContact* Best=nullptr;float BestScore=Doctrine.MinimumTargetValue;
        bool BestMortar=false;
        for (const auto& Candidate:Tactics.Contacts)
        {
            const bool PreferMortar=MortarReady && (Candidate.Machinegun || Candidate.Entrenched || !CannonReady);
            const auto Type=PreferMortar?EFrontlineSupport::Mortar:EFrontlineSupport::Artillery;
            if(!SafeTarget(Candidate.Position,Type))continue;
            const float Score=FrontlineDifficultyV30::TargetScore(Candidate,Tactics.Contacts,Elapsed,Doctrine);
            if(Score>BestScore){Best=&Candidate;BestScore=Score;BestMortar=PreferMortar;}
        }
        if (Best)
        {
            bEnemyPendingMortar=BestMortar;
            EnemyObservedFireTarget=FrontlineDifficultyV30::RegisteredPoint(*Best,Elapsed,Doctrine,EnemyDecisionRandom);
            EnemyFireExecuteAt=Elapsed+EnemyDecisionRandom.FRandRange(Doctrine.ReactionMin,Doctrine.ReactionMax);
            bEnemyFirePending=true;
        }
    }
}

float AWarfareFrontlineGameMode::GetCampaignObjectiveProgress() const
{
    // Retain the API for existing save/review consumers. Timed centre control
    // is no longer a campaign victory condition or a displayed percentage.
    return 0.f;
}

void AWarfareFrontlineGameMode::UpdateCampaignObjective(float DT)
{
    // Trench captures create staging posts and authored counterstrokes. Victory
    // requires crossing the enemy end line; no capture count or hidden hold timer.
    CampaignHoldElapsed=0.f;
    FrontlineCampaignV41::TickObjective(this,DT);
    for(int32 Team=0;Team<2;++Team)if(FrontlineCampaignV41::HasObjectiveVictory(this,Team))
    {
        FinishBattle(Team,FrontlineCampaignV41::ObjectiveConclusion(this,Team));return;
    }
}
#include "WarfareFrontlineEnemyChecksV84415.inl"
