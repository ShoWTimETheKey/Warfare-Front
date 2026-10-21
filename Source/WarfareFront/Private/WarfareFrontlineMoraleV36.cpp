#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineFrontsV76.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

struct FFrontlineMoraleV76Access
{
    static void Set(AWarfareFrontlineSoldier* U,bool Officer,EFrontlineDamageKind Kind){U->bCommissionedOfficer=Officer;U->DeathState.Kind=Kind;}
};
namespace FrontlineMoraleV36
{
float CapacityMultiplierV85(const AWarfareFrontlineGameMode* B)
{
    return FrontlineUltimateArchiveV8215::Active(B)?1.50f:1.25f;
}
namespace
{
struct FState
{
    FGuid Encounter;
    FTelemetry Events;
    float Ceiling[2]={100,100}, LastCasualty[2]={-1000,-1000}, FriendlyShockAt[2]={-1000,-1000};
    float Frontier[2]={0,0}, CohesionSpent[2]={0,0}, PendingSeconds=0;
    uint32 Captured[2]={0,0}, Lost[2]={0,0};
    TSet<FString> CapturedSectors[2],LostSectors[2];
    uint16 AdvanceMilestones[2]={0,0};float RallySpent[2]={0,0};
    TSet<TWeakObjectPtr<AWarfareFrontlineSoldier>> Casualties;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FState> States;
float HomeAlong(const AWarfareFrontlineGameMode* B,int32 Team)
{
    FString Scene;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
    if(WarfareFRRUWorldV82::SceneInfo(B,Scene,Revision,Solids,Minimum,Maximum))
        return B->GetTeamDirection(Team)>0?Minimum.X:-Maximum.X;
    if(B->Trenches.IsEmpty())return -FMath::Abs(FrontlineBattlefield::TrenchX(0));
    const int32 Index=B->GetTeamDirection(Team)>0?0:B->Trenches.Num()-1;
    return (B->Trenches.IsValidIndex(Index)?B->Trenches[Index].X:FrontlineBattlefield::TrenchX(Index))*B->GetTeamDirection(Team);
}
float EnemyRearAlong(const AWarfareFrontlineGameMode* B,int32 Team)
{
    FString Scene;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
    if(WarfareFRRUWorldV82::SceneInfo(B,Scene,Revision,Solids,Minimum,Maximum))
        return B->GetTeamDirection(Team)>0?Maximum.X:-Minimum.X;
    if(B->Trenches.IsEmpty())return FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
    const int32 Index=B->GetTeamDirection(Team)>0?B->Trenches.Num()-1:0;
    return (B->Trenches.IsValidIndex(Index)?B->Trenches[Index].X:FrontlineBattlefield::TrenchX(Index))*B->GetTeamDirection(Team);
}
float ForwardStep(const AWarfareFrontlineGameMode* B,int32 Team)
{
    // Eight milestones between the actual home lines: adding/removing an
    // intermediate trench cannot manufacture extra morale at deployment.
    return FMath::Max(100.f,(EnemyRearAlong(B,Team)-HomeAlong(B,Team))/8.f);
}
FState& State(AWarfareFrontlineGameMode* B)
{
    for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& S=States.FindOrAdd(B);
    if(B && S.Encounter!=B->GetEncounterId())
    {
        S=FState();S.Encounter=B->GetEncounterId();
        for(int32 Team=0;Team<2;++Team)
        {
            S.Ceiling[Team]=FMath::Max(1.f,B->Morale[Team]);
            S.Frontier[Team]=HomeAlong(B,Team);
        }
    }
    return S;
}
bool ValidTeam(int32 Team){return Team==0 || Team==1;}
void Change(AWarfareFrontlineGameMode* B,int32 Team,float Amount,ECause Cause)
{
    if(!B || !ValidTeam(Team) || !FMath::IsFinite(Amount) || Amount==0)return;
    auto& S=State(B);const float Before=B->Morale[Team];
    // Zero morale is a terminal state. A recovery on the same tick must never
    // resurrect a collapsing army; territorial success is evaluated first.
    if(Amount>0 && Before<=0)return;
    B->Morale[Team]=FMath::Clamp(Before+Amount,0.f,FMath::Max(S.Ceiling[Team],Before));
    const float Applied=B->Morale[Team]-Before;if(FMath::Abs(Applied)<.00001f)return;
    if(Applied<0)S.Events.Loss[Team][Cause==ECause::OfficerLost?5:Cause==ECause::ArmourDestroyed?6:FMath::Clamp(int32(Cause),0,4)]-=Applied;
    else S.Events.Recovery[Team][Cause==ECause::Rally?3:FMath::Clamp(int32(Cause)-5,0,2)]+=Applied;
    S.Events.LastCause[Team]=Cause;S.Events.LastEventAt[Team]=B->Elapsed;
}
void AdvanceGrant(AWarfareFrontlineGameMode* B,int32 Team,float Progress)
{
    auto& S=State(B);
    for(int32 I=0;I<10;++I){const float Threshold=I==9?.95f:(I+1)*.1f;const uint16 Bit=1u<<I;
        if(Progress>=Threshold&&!(S.AdvanceMilestones[Team]&Bit)){S.AdvanceMilestones[Team]|=Bit;Change(B,Team,.6f,ECause::Advance);}}
}
bool ClearTankSight(const AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U,const AFrontlineTank* T,const FCollisionQueryParams& Query)
{
    if(B->DirectFireMultiplier(U->Feet(),T->GetActorLocation())<.95f)return false;
    return !B->GetWorld()->LineTraceTestByChannel(U->Feet()+FVector(0,0,135),T->AimPoint(),ECC_Visibility,Query);
}
}
void Reset(AWarfareFrontlineGameMode* B){States.Remove(B);State(B);}
void WithScopedCombatEventsV839(AWarfareFrontlineGameMode* B,TFunctionRef<void()> Work)
{if(!B||!B->IsProgressionVolatile())return;const auto Before=States;ON_SCOPE_EXIT{States=Before;};Work();}
float StartingMaximum(AWarfareFrontlineGameMode* B,int32 Team){return B&&ValidTeam(Team)?State(B).Ceiling[Team]:100.f;}
void OnArmourDestroyed(AWarfareFrontlineGameMode* B,int32 Team,bool Rail,bool Aboard)
{if(B&&B->Phase==EFrontlinePhase::Fighting)Change(B,Team,-(Rail?(Aboard?24.f:20.f):8.f),ECause::ArmourDestroyed);}
void Rally(AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||!ValidTeam(Team))return;auto& S=State(B);const float Before=B->Morale[Team];
    Change(B,Team,FMath::Max(0.f,FMath::Min(5.f,10.f-S.RallySpent[Team])),ECause::Rally);
    S.RallySpent[Team]+=FMath::Max(0.f,B->Morale[Team]-Before);
}
const FTelemetry& Telemetry(AWarfareFrontlineGameMode* B){return State(B).Events;}
const TCHAR* CauseLabel(ECause Cause)
{
    switch(Cause)
    {
        case ECause::FriendlyFire:return TEXT("Friendly shellfire");
        case ECause::Gas:return TEXT("Gas exposure");
        case ECause::Armour:return TEXT("Facing enemy armour");
        case ECause::LostTrench:return TEXT("Trench lost");
        case ECause::CapturedTrench:return TEXT("Enemy trench captured");
        case ECause::Advance:return TEXT("Forward progress");
        case ECause::Cohesion:return TEXT("Mutual support");
        case ECause::OfficerLost:return TEXT("Officer lost");
        case ECause::ArmourDestroyed:return TEXT("Armour destroyed");
        case ECause::Rally:return TEXT("Orders restored");
        default:return TEXT("Casualties");
    }
}
bool OnCasualty(AWarfareFrontlineGameMode* B,AWarfareFrontlineSoldier* U,int32 AttackingTeam)
{
    if(!B || !U || !ValidTeam(U->Team))return false;
    auto& S=State(B);if(S.Casualties.Contains(U))return false;S.Casualties.Add(U);S.LastCasualty[U->Team]=B->Elapsed;
    if(B->Phase!=EFrontlinePhase::Fighting)return true;
    Change(B,U->Team,-CasualtyLoss,ECause::Casualties);
    if(U->IsCommissionedOfficer())Change(B,U->Team,-2.5f,ECause::OfficerLost);
    if(AttackingTeam==U->Team && IsFrontlineExplosiveDamage(U->GetDeathState().Kind))
    {
        // A battery landing amongst its own men is worse than ordinary losses.
        // The first hit adds a local shock; successive shells cannot retrigger
        // that shock every frame, but each friendly casualty still matters.
        const bool Shock=B->Elapsed-S.FriendlyShockAt[U->Team]>=8.f;
        Change(B,U->Team,-(.75f+(Shock?1.25f:0.f)),ECause::FriendlyFire);
        if(Shock)S.FriendlyShockAt[U->Team]=B->Elapsed;
    }
    return true;
}
void OnCapture(AWarfareFrontlineGameMode* B,int32 Trench,int32 PreviousOwner,int32 Team)
{
    if(!B || !ValidTeam(Team) || !B->Trenches.IsValidIndex(Trench) || !ValidTeam(PreviousOwner) || PreviousOwner==Team)return;
    auto& S=State(B);const uint32 Bit=1u<<Trench;
    // A trench can be fought over, but repeated transfer cannot manufacture an
    // endless stream of morale. Each side receives its first capture/loss only.
    if(!(S.Captured[Team]&Bit)){S.Captured[Team]|=Bit;Change(B,Team,3.f,ECause::CapturedTrench);}
    if(!(S.Lost[PreviousOwner]&Bit)){S.Lost[PreviousOwner]|=Bit;Change(B,PreviousOwner,-3.f,ECause::LostTrench);}
}
void OnCaptureSector(AWarfareFrontlineGameMode* B,const FString& SectorId,int32 PreviousOwner,int32 Team)
{
    if(!B||!ValidTeam(Team)||!ValidTeam(PreviousOwner)||PreviousOwner==Team
        ||SectorId.IsEmpty()||SectorId.Len()>128)return;
    auto& S=State(B);
    // Stable native node IDs share the same bounded first-transfer reward as
    // classic trenches, without inventing legacy trench array indices.
    if(!S.CapturedSectors[Team].Contains(SectorId))
    {S.CapturedSectors[Team].Add(SectorId);Change(B,Team,3.f,ECause::CapturedTrench);}
    if(!S.LostSectors[PreviousOwner].Contains(SectorId))
    {S.LostSectors[PreviousOwner].Add(SectorId);Change(B,PreviousOwner,-3.f,ECause::LostTrench);}
}
bool HasTerritorialVictory(const AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B || !ValidTeam(Team) || B->Phase!=EFrontlinePhase::Fighting)return false;
    const int32 Dir=B->GetTeamDirection(Team),Rear=Dir>0?B->Trenches.Num()-1:0;
    const int32 First=Dir>0?B->Trenches.Num()/2:(B->Trenches.Num()-1)/2;
    for(int32 Index=First;B->Trenches.IsValidIndex(Index);Index+=Dir)
        if(B->Trenches[Index].Owner!=Team)return false;
    if(!B->Trenches.IsValidIndex(Rear) || B->TrenchSquadCount(Rear,Team)==0)return false;
    int32 Garrison=0;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U && U->IsActiveCombatant())
    {
        const float Separation=FMath::Abs(U->Feet().X-B->Trenches[Rear].X);
        if(U->Team!=Team && Separation<800.f)return false;
        if(U->Team==Team && Separation<550.f)++Garrison;
    }
    return Garrison>=2;
}
void Tick(AWarfareFrontlineGameMode* B,float Seconds)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_ContextMorale);
    if(!B || B->Phase!=EFrontlinePhase::Fighting || !FMath::IsFinite(Seconds) || Seconds<=0)return;
    auto& S=State(B);S.PendingSeconds+=Seconds;if(S.PendingSeconds<1.f)return;
    // Sample real positions once per second. A pause/hitch cannot retroactively
    // apply a minute of exposure to a position occupied only on this frame.
    const float DT=FMath::Min(2.f,S.PendingSeconds);S.PendingSeconds=0;
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    FCollisionQueryParams SightQuery(SCENE_QUERY_STAT(FrontlineMoraleTankSight),true);
    for(const auto& U:B->Units)if(U)SightQuery.AddIgnoredActor(U.Get());
    if(Fleet)for(const auto& T:Fleet->GetVehicles())if(T)SightQuery.AddIgnoredActor(T.Get());
    float GasRate[2]={0,0},TankRate[2]={0,0},Forward[2]={S.Frontier[0],S.Frontier[1]};
    TArray<const FFrontlineSquad*> Cohesive[2];
    for(const auto& Squad:B->Squads)if(ValidTeam(Squad.Team) && Squad.LivingCount()>=2)
    {
        int32 Active=0;float Pressure=0;
        for(const auto& Member:Squad.Members)if(Member.IsValid() && Member->IsActiveCombatant()){++Active;Pressure+=Member->Suppression;}
        if(Active<2)continue;
        const float Along=Squad.Centre().X*B->GetTeamDirection(Squad.Team);
        Forward[Squad.Team]=FMath::Max(Forward[Squad.Team],Along);
        const float CohesionLine=HomeAlong(B,Squad.Team)+ForwardStep(B,Squad.Team)*.20f;
        if(Along>CohesionLine && Pressure/Active<.28f)Cohesive[Squad.Team].Add(&Squad);
    }
    for(AWarfareFrontlineSoldier* U:B->Units)if(U && U->IsActiveCombatant() && ValidTeam(U->Team))
    {
        bool InGas=false;
        for(const auto& Zone:B->SmokeZones)
            if(Zone.bGas && B->Elapsed>=Zone.StartedAt && B->Elapsed<Zone.ExpiresAt && FVector::DistSquared2D(U->Feet(),Zone.Centre)<=FMath::Square(Zone.Radius))
                {InGas=true;break;}
        if(InGas)GasRate[U->Team]+=.045f*(FrontlineFieldV32::GasHitScale(U)<.5f?.15f:1.f);
        bool EnemyArmour=false,FriendlyArmour=false;
        if(Fleet)for(const auto& T:Fleet->GetVehicles())if(T && T->IsOperational() && T->HasEnteredField())
        {
            const float Distance=FVector::DistSquared2D(U->Feet(),T->GetActorLocation());
            if(T->Team==U->Team){if(Distance<FMath::Square(1800.f))FriendlyArmour=true;continue;}
            if(Distance<FMath::Square(2200.f) && ClearTankSight(B,U,T.Get(),SightQuery))EnemyArmour=true;
        }
        if(EnemyArmour)TankRate[U->Team]+=.026f*(B->CoverFor(U)>0?.35f:1.f)*(FriendlyArmour?.35f:1.f);
    }
    for(int32 Team=0;Team<2;++Team)
    {
        Change(B,Team,-FMath::Min(.42f,GasRate[Team])*DT,ECause::Gas);
        Change(B,Team,-FMath::Min(.16f,TankRate[Team])*DT,ECause::Armour);
        AdvanceGrant(B,Team,FrontlineFrontsV76::AdvanceFraction(B,Team));
        bool CloseSections=false;
        for(int32 A=0;A<Cohesive[Team].Num() && !CloseSections;++A)
            for(int32 C=A+1;C<Cohesive[Team].Num();++C)
                if(FVector::DistSquared2D(Cohesive[Team][A]->Centre(),Cohesive[Team][C]->Centre())<FMath::Square(1250.f)){CloseSections=true;break;}
        if(CloseSections && GasRate[Team]==0 && TankRate[Team]==0 && B->Elapsed-S.LastCasualty[Team]>=8.f && S.CohesionSpent[Team]<CohesionBudget)
        {
            const float Grant=FMath::Min(.035f*DT,CohesionBudget-S.CohesionSpent[Team]);
            const float Before=B->Morale[Team];Change(B,Team,Grant,ECause::Cohesion);
            S.CohesionSpent[Team]+=FMath::Max(0.f,B->Morale[Team]-Before);
        }
    }
}
void RunChecksV76(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))){Check(TEXT("v76_morale_isolated"),false);return;}
    const auto Old=State(B);const float M0=B->Morale[0],M1=B->Morale[1];const int32 C0=B->Casualties[0],C1=B->Casualties[1];const FString Message=B->LastMessage;
    auto* Victim=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();auto* Officer=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    ON_SCOPE_EXIT{if(IsValid(Victim))Victim->Destroy();if(IsValid(Officer))Officer->Destroy();B->Morale[0]=M0;B->Morale[1]=M1;B->Casualties[0]=C0;B->Casualties[1]=C1;B->LastMessage=Message;States.Add(B,Old);};
    if(!Victim||!Officer){Check(TEXT("v76_morale_real_actor_fixture"),false);return;}
    Victim->SetActorTickEnabled(false);Officer->SetActorTickEnabled(false);Victim->Team=Officer->Team=0;
    FFrontlineMoraleV76Access::Set(Victim,false,EFrontlineDamageKind::Artillery);FFrontlineMoraleV76Access::Set(Officer,true,EFrontlineDamageKind::Bullet);
    B->Morale[0]=100;Reset(B);const int32 Before=B->Casualties[0];B->RegisterCasualty(Victim,0);const float After=B->Morale[0];B->RegisterCasualty(Victim,0);
    Check(TEXT("v76_friendly_blast_casualty_and_shock_count_once"),B->Casualties[0]==Before+1&&FMath::IsNearlyEqual(B->Morale[0],After)&&FMath::IsNearlyEqual(Telemetry(B).Loss[0][1],2.f));
    B->Morale[0]=50;AdvanceGrant(B,0,.41f);const float Advance=B->Morale[0];AdvanceGrant(B,0,.05f);AdvanceGrant(B,0,.41f);
    Check(TEXT("v76_retreat_and_readvance_cannot_repeat_milestone_grant"),FMath::IsNearlyEqual(Advance,52.4f,.001f)&&FMath::IsNearlyEqual(B->Morale[0],Advance));
    const float BeforeOfficer=B->Morale[0];B->RegisterCasualty(Officer,1);
    Check(TEXT("v76_officer_loss_retains_distinct_bounded_penalty"),FMath::IsNearlyEqual(BeforeOfficer-B->Morale[0],3.2f,.001f)&&FMath::IsNearlyEqual(Telemetry(B).Loss[0][5],2.5f));
    B->Morale[0]=B->Morale[1]=100;Reset(B);B->Morale[0]=B->Morale[1]=70;
    OnCaptureSector(B,TEXT("native.bridge"),1,0);OnCaptureSector(B,TEXT("native.bridge"),1,0);
    Check(TEXT("v82_native_sector_capture_has_one_bounded_gain_and_loss"),B->Morale[0]==73&&B->Morale[1]==67);
    OnCaptureSector(B,TEXT("native.bridge"),0,1);OnCaptureSector(B,TEXT("native.bridge"),1,0);
    Check(TEXT("v82_sector_recapture_cannot_farm_morale"),B->Morale[0]==70&&B->Morale[1]==70);
    OnCaptureSector(B,TEXT("native.ridge"),-1,0);OnCaptureSector(B,FString(),1,0);
    Check(TEXT("v82_neutral_or_invalid_sector_cannot_manufacture_morale"),B->Morale[0]==70&&B->Morale[1]==70);
    B->Morale[0]=0;Rally(B,0);AdvanceGrant(B,0,1.f);FrontlineFrontsV76::Read(B,.1f);
    Check(TEXT("v76_zero_morale_is_terminal_despite_recovery_and_hud_animation"),B->Morale[0]==0);
}
void RunLayoutChecksV50(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("v50_morale_layout_review_isolated"),false);return;}
    const auto OriginalTrenches=B->Trenches;const auto* Existing=States.Find(B);const bool HadState=Existing!=nullptr;
    const FState OriginalState=Existing?*Existing:FState();
    ON_SCOPE_EXIT{B->Trenches=OriginalTrenches;if(HadState)States.Add(B,OriginalState);else States.Remove(B);};
    bool HomeCorrect=true,EqualBudget=true,Symmetric=true;
    for(int32 Count:{2,3,4})
    {
        B->Trenches.SetNum(Count);
        for(int32 I=0;I<Count;++I)B->Trenches[I].X=-3000.f+6000.f*I/(Count-1);
        if(Count==3)B->Trenches[1].X=210.f;
        States.Remove(B);const auto& S=State(B);
        for(int32 Team=0;Team<2;++Team)
        {
            HomeCorrect&=FMath::IsNearlyEqual(S.Frontier[Team],-3000.f);
            EqualBudget&=FMath::IsNearlyEqual((EnemyRearAlong(B,Team)-S.Frontier[Team])/ForwardStep(B,Team),8.f);
            const int32 Dir=B->GetTeamDirection(Team),First=Dir>0?Count/2:(Count-1)/2;
            int32 Captures=0;for(int32 I=First;B->Trenches.IsValidIndex(I);I+=Dir)++Captures;
            Symmetric&=Captures==(Count+1)/2;
        }
    }
    Check(TEXT("v50_morale_progress_starts_at_real_home_no_deployment_windfall"),HomeCorrect);
    Check(TEXT("v50_two_three_four_trenches_preserve_same_six_point_progress_budget"),EqualBudget);
    Check(TEXT("v50_territorial_half_checks_mirror_for_even_and_odd_layouts"),Symmetric);
}
}
