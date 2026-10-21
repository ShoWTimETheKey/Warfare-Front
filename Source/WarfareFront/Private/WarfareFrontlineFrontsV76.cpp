#include "WarfareFrontlineFrontsV76.h"
#include "WarfareFrontlineUIMotionV83.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFRRUWorldV82.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
namespace FrontlineFrontsV76
{
namespace
{
struct FState{FGuid Encounter;FReadout R;float From[2]={0,0},Target[2]={0,0},Age[2]={3,3},Sample=1;FrontlineUIMotionV83::FMoraleMotion MoraleMotion[2];bool Initial=true;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FState> States;
}
float AdvanceFraction(const AWarfareFrontlineGameMode* B,int32 Team)
{
    if(!B||Team<0||Team>1)return 0;
    const float Half=FMath::Max(1.f,FrontlineBattlefield::GoalHalfLength);
    float Home=-Half,Away=Half;FString Scene;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
    if(WarfareFRRUWorldV82::SceneInfo(B,Scene,Revision,Solids,Minimum,Maximum))
    {
        const int32 Direction=B->GetTeamDirection(Team);
        Home=Direction>0?Minimum.X:-Maximum.X;Away=Direction>0?Maximum.X:-Minimum.X;
    }
    float Furthest=Home;bool Present=false;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==Team&&U->bFieldEntered&&U->IsActiveCombatant())
    {Furthest=FMath::Max(Furthest,float(U->Feet().X*B->GetTeamDirection(Team)));Present=true;}
    return Present?FMath::Clamp((Furthest-Home)/FMath::Max(1.f,Away-Home),0.f,1.f):0.f;
}
void Reset(AWarfareFrontlineGameMode* B){States.Remove(B);}
bool Peek(const AWarfareFrontlineGameMode* B,FReadout& Out)
{if(!B)return false;const auto* S=States.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B)));if(!S||S->Encounter!=B->GetEncounterId())return false;Out=S->R;return true;}
FReadout Read(AWarfareFrontlineGameMode* B,float DT)
{
    if(!B)return {};
    for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& S=States.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){S=FState();S.Encounter=B->GetEncounterId();}
    DT=FMath::Clamp(FMath::IsFinite(DT)?DT:0.f,0.f,.25f);S.Sample+=DT;
    for(int32 T=0;T<2;++T)
    {
        S.R.Advance[T]=AdvanceFraction(B,T);S.R.Morale[T]=FMath::Clamp(B->Morale[T]/FrontlineMoraleV36::StartingMaximum(B,T),0.f,1.f);
        if(S.Sample>=1.f&&FMath::Abs(S.Target[T]-S.R.Advance[T])>.00001f)
        {S.From[T]=S.R.AdvanceDisplay[T];S.Target[T]=S.R.Advance[T];S.Age[T]=0;}
        S.Age[T]=FMath::Min(3.f,S.Age[T]+DT);
        const float Ease=FMath::Sin((S.Age[T]/3.f)*HALF_PI);
        S.R.AdvanceDisplay[T]=FMath::Lerp(S.From[T],S.Target[T],Ease);
        // Display interpolation has no authority over casualties or victory.
        S.MoraleMotion[T].Step(S.R.Morale[T],DT);
        S.R.MoraleDisplay[T]=S.MoraleMotion[T].Display;
        S.R.MoraleGhost[T]=S.MoraleMotion[T].Ghost;
        S.R.MoraleGhostActive[T]=S.MoraleMotion[T].HasGhost;
    }
    if(S.Sample>=1.f)S.Sample=0;S.Initial=false;return S.R;
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check,TSharedPtr<FJsonObject>* Diagnostics)
{
    if(!B||!B->IsProgressionVolatile()){Check(TEXT("v76_fronts_isolated"),false);return;}
    Check(TEXT("v76_empty_armies_do_not_gain_progress"),AdvanceFraction(B,0)==0&&AdvanceFraction(B,1)==0);
    auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();auto* C=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    if(!A||!C){if(A)A->Destroy();if(C)C->Destroy();Check(TEXT("v76_fronts_real_actor_fixture"),false);return;}
    const auto Units=B->Units;ON_SCOPE_EXIT{A->Destroy();C->Destroy();B->Units=Units;};
    A->SetActorTickEnabled(false);C->SetActorTickEnabled(false);A->Team=0;C->Team=1;A->bFieldEntered=C->bFieldEntered=true;A->Health=C->Health=100;
    const float Goal=FrontlineBattlefield::GoalHalfLength;
    const FVector RequestedA(-Goal*.5f*B->GetTeamDirection(0),0,88),RequestedC(0,0,88);
    const bool PlacedA=A->SetActorLocation(RequestedA),PlacedC=C->SetActorLocation(RequestedC);B->Units.Add(A);B->Units.Add(C);
    const float AdvanceA=AdvanceFraction(B,0),AdvanceC=AdvanceFraction(B,1);
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("goal_half_length_cm"),Goal);J->SetNumberField(TEXT("army_unit_count"),B->Units.Num());
    TArray<TSharedPtr<FJsonValue>> Actors;
    for(auto* U:{A,C})
    {
        const int32 Dir=B->GetTeamDirection(U->Team);const double FeetX=U->Feet().X;
        auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("team"),U->Team);P->SetNumberField(TEXT("direction"),Dir);P->SetNumberField(TEXT("feet_x_cm"),FeetX);
        P->SetNumberField(TEXT("requested_x_cm"),U==A?RequestedA.X:RequestedC.X);P->SetBoolField(TEXT("set_location_returned"),U==A?PlacedA:PlacedC);
        P->SetBoolField(TEXT("entered"),U->bFieldEntered);P->SetBoolField(TEXT("active_combatant"),U->IsActiveCombatant());P->SetNumberField(TEXT("health"),U->Health);
        P->SetNumberField(TEXT("actual_advance"),U==A?AdvanceA:AdvanceC);P->SetNumberField(TEXT("expected_advance"),U==A?.25:.5);
        P->SetNumberField(TEXT("double_reference_from_actual_feet"),(FeetX*Dir+double(Goal))/(2.*double(Goal)));
        Actors.Add(MakeShared<FJsonValueObject>(P));
    }
    J->SetArrayField(TEXT("real_actors"),Actors);if(Diagnostics)*Diagnostics=J;
    UE_LOG(LogTemp,Display,TEXT("WF_V76_FRONTS goal=%.17g dir0=%d feet0=%.17g advance0=%.17g placed0=%d dir1=%d feet1=%.17g advance1=%.17g placed1=%d units=%d"),double(Goal),B->GetTeamDirection(0),A->Feet().X,double(AdvanceA),PlacedA,B->GetTeamDirection(1),C->Feet().X,double(AdvanceC),PlacedC,B->Units.Num());
    // UE's default float comparison is absolute 1e-8, below one binary32
    // step here. Shipping /fp:fast produced exactly one predecessor step at
    // .25 and .5 (both physical positions were correct). Compare at most one
    // adjacent representable float, not a map-independent decimal tolerance.
    // Production arithmetic and the authoritative win rules remain untouched.
    const auto SameProgress=[](float Actual,float Expected)
    {
        if(!FMath::IsFinite(Actual)||!FMath::IsFinite(Expected)||Actual<0||Actual>1||Expected<0||Expected>1)return false;
        if(Actual==Expected)return true;
        uint32 ABits=0,EBits=0;FMemory::Memcpy(&ABits,&Actual,sizeof(float));FMemory::Memcpy(&EBits,&Expected,sizeof(float));
        return (ABits>EBits?ABits-EBits:EBits-ABits)<=1u;
    };
    J->SetNumberField(TEXT("maximum_adjacent_binary32_steps"),1);
    J->SetNumberField(TEXT("quarter_lower_absolute_bound"),1.4901161193847656e-8);J->SetNumberField(TEXT("quarter_upper_absolute_bound"),2.9802322387695312e-8);
    J->SetNumberField(TEXT("half_lower_absolute_bound"),2.9802322387695312e-8);J->SetNumberField(TEXT("half_upper_absolute_bound"),5.9604644775390625e-8);
    Check(TEXT("v76_two_real_fronts_are_not_complementary"),SameProgress(AdvanceA,.25f)&&SameProgress(AdvanceC,.5f));
    uint32 TwoStepBits=0x3efffffeu;float TwoSteps=0;FMemory::Memcpy(&TwoSteps,&TwoStepBits,sizeof(float));
    Check(TEXT("v76_front_numeric_bound_rejects_two_ulp_error"),!SameProgress(TwoSteps,.5f));
    A->Health=0;Check(TEXT("v76_dead_foremost_actor_does_not_hold_progress"),AdvanceFraction(B,0)==0);A->Health=100;
    TArray<TSharedPtr<FJsonValue>> Scenarios;
    const auto Record=[&](const TCHAR* Name,float Expected0,float Expected1,bool Exact=false)
    {
        const float Actual0=AdvanceFraction(B,0),Actual1=AdvanceFraction(B,1);
        auto D=MakeShared<FJsonObject>();D->SetStringField(TEXT("name"),Name);D->SetNumberField(TEXT("actual_advance_0"),Actual0);D->SetNumberField(TEXT("actual_advance_1"),Actual1);
        D->SetNumberField(TEXT("expected_advance_0"),Expected0);D->SetNumberField(TEXT("expected_advance_1"),Expected1);D->SetBoolField(TEXT("exact_clamp_required"),Exact);
        D->SetNumberField(TEXT("actor_a_feet_x_cm"),A->Feet().X);D->SetNumberField(TEXT("actor_c_feet_x_cm"),C->Feet().X);D->SetNumberField(TEXT("actor_c_team"),C->Team);
        Scenarios.Add(MakeShared<FJsonValueObject>(D));Check(Name,Exact?(Actual0==Expected0&&Actual1==Expected1):(SameProgress(Actual0,Expected0)&&SameProgress(Actual1,Expected1)));
    };
    const auto Position=[&](AWarfareFrontlineSoldier* U,float DirectionalX)
    {U->SetActorLocation(FVector(double(DirectionalX)*B->GetTeamDirection(U->Team),0,88));};
    Position(A,0);Record(TEXT("v76_advancing_one_army_keeps_opponent_front_unchanged"),.5f,.5f);
    Position(C,-Goal*.5f);Record(TEXT("v76_opponent_retreat_changes_only_its_own_front"),.5f,.25f);
    A->Team=C->Team=0;Position(A,-Goal*.5f);Position(C,0);Record(TEXT("v76_same_army_uses_foremost_member_not_average"),.5f,0);
    Position(C,-Goal);Record(TEXT("v76_retreating_foremost_member_reveals_next_member"),.25f,0);
    Position(C,0);C->Health=0;Record(TEXT("v76_dead_front_member_cannot_hide_live_rear_member"),.25f,0);
    A->Health=0;Record(TEXT("v76_no_surviving_members_return_exact_zero"),0,0,true);
    A->Health=C->Health=100;C->Team=1;Position(A,-Goal);Position(C,-Goal);Record(TEXT("v76_both_home_boundaries_are_exact_zero"),0,0,true);
    Position(A,Goal);Position(C,Goal);Record(TEXT("v76_both_far_boundaries_are_one"),1,1);
    Position(A,-Goal*2.f);Position(C,Goal*2.f);Record(TEXT("v76_beyond_both_ends_clamps_exactly_zero_and_one"),0,1,true);
    Position(A,Goal);A->bFieldEntered=false;C->Health=0;Record(TEXT("v76_unentered_member_cannot_mark_front"),0,0,true);
    J->SetArrayField(TEXT("focused_real_actor_scenarios"),Scenarios);B->Units=Units;
    const float M0=B->Morale[0],M1=B->Morale[1];ON_SCOPE_EXIT{B->Morale[0]=M0;B->Morale[1]=M1;Reset(B);};
    B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0)*.5f;
    const auto R=Read(B,.05f);Check(TEXT("v76_morale_uses_own_starting_maximum"),FMath::IsNearlyEqual(R.Morale[0],.5f));
    Check(TEXT("v76_fronts_are_independent_not_complementary"),R.Advance[0]+R.Advance[1]==0);
    B->Morale[0]=0;Read(B,.01f);Check(TEXT("v76_display_never_resurrects_actual_zero_morale"),B->Morale[0]==0);
    // Drive the actual readout at fixed real-time steps, independent of the
    // battlefield clock. These fixtures remain in the volatile review world.
    Reset(B);const float Maximum=FrontlineMoraleV36::StartingMaximum(B,0);
    B->Morale[0]=Maximum;const auto InitialRead=Read(B,0);
    Check(TEXT("v83_real_read_first_frame_has_no_morale_ghost"),InitialRead.MoraleDisplay[0]==1.f&&!InitialRead.MoraleGhostActive[0]);
    B->Morale[0]=Maximum*.4f;Read(B,0);FReadout LossRead;
    for(int32 I=0;I<20;++I)LossRead=Read(B,.01f);
    Check(TEXT("v83_real_read_morale_display_95_percent_by_200ms"),(LossRead.MoraleDisplay[0]-.4f)/.6f<=.05001f);
    Check(TEXT("v83_real_read_ghost_holds_old_edge_through_200ms"),LossRead.MoraleGhostActive[0]&&LossRead.MoraleGhost[0]==1.f);
    for(int32 I=20;I<88;++I)LossRead=Read(B,.01f);
    Check(TEXT("v83_real_read_ghost_is_retracted_at_880ms"),!LossRead.MoraleGhostActive[0]&&LossRead.MoraleGhost[0]==LossRead.MoraleDisplay[0]);
    B->Morale[0]=Maximum*.2f;Read(B,0);B->Morale[0]=Maximum*.6f;const auto RecoveryRead=Read(B,0);
    Check(TEXT("v83_real_read_morale_recovery_has_no_ghost"),!RecoveryRead.MoraleGhostActive[0]);
}
}
