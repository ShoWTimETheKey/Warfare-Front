#include "WarfareFrontlineMotionV38.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

namespace
{
float Ease(float X){X=FMath::Clamp(X,0.f,1.f);return X*X*(3.f-2.f*X);}
FFrontlineIdlePoseV38 Weighted(const FFrontlineIdlePoseV38& Peak,float Weight)
{
    FFrontlineIdlePoseV38 P=Peak;P.Torso=Peak.Torso*Weight;P.Head=Peak.Head*Weight;P.Weight=Weight;
    if(Weight<=0)P.Intent=EFrontlineIdleIntentV38::None;
    return P;
}
}

FFrontlineIdlePoseV38 FrontlineMotionV38::UpdateIdle(FFrontlineIdleStateV38& S,const AWarfareFrontlineSoldier* U,const UAnimationAsset* Asset,float DT)
{
    if(!U||!FMath::IsFinite(DT)||DT<0)return {};
    // No frame-number seed, actor pointer seed, world-global timer or battle RNG.
    const uint32 Identity=uint32(U->SquadId)*92821u^uint32(U->MemberIndex+1)*68917u^uint32(U->Team+1)*283923u^381917u;
    if(!S.Initialized||S.Identity!=Identity)
    {
        S=FFrontlineIdleStateV38();S.Initialized=true;S.Identity=Identity;S.Random.Initialize(int32(Identity));S.Wait=S.Random.FRandRange(1.3f,7.4f);
    }
    if(DT==0)return S.Current;
    const auto* B=U->Battle.Get();const auto* Gun=FrontlineMachinegunsV30::Get(U);
    const bool InReadyPose=Asset&&Asset==U->GetIdleMotion()&&U->WeaponPhase==EFrontlineWeaponPhase::Ready;
    const bool Eligible=B&&B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE&&U->IsActiveCombatant()
        &&InReadyPose&&!U->bMoving&&!U->bEngaging&&U->Suppression<.32f&&B->CoverFor(U)>0.f&&(!Gun||!Gun->TransitionMotion());
    if(!Eligible)
    {
        // The endpoint snapshot already retains the displayed old gesture when
        // a real clip changes. Never layer a return gesture over a feed, throw,
        // death or locomotion clip. For a same-ready-pose suppression change,
        // return gently instead of snapping the man's head to neutral.
        if(InReadyPose&&!U->bMoving&&U->IsActiveCombatant()&&S.Current.IsActive())
        {
            if(!S.Returning){S.Peak=S.Current;S.ReturnWeight=1.f;S.Returning=true;}
            S.ReturnWeight=FMath::Max(0.f,S.ReturnWeight-DT/.26f);S.Current=Weighted(S.Peak,Ease(S.ReturnWeight));
        }
        else {S.Current={};S.Returning=false;}
        if(S.WasEligible)S.Wait=S.Random.FRandRange(1.8f,5.6f);
        S.WasEligible=false;S.Age=S.Duration=0;return S.Current;
    }
    if(S.Returning)
    {
        // A short suppression/engagement fluctuation must finish the same
        // return, not abruptly remove or resurrect the half-returned glance.
        S.ReturnWeight=FMath::Max(0.f,S.ReturnWeight-DT/.26f);S.Current=Weighted(S.Peak,Ease(S.ReturnWeight));
        if(S.ReturnWeight<=0){S.Returning=false;S.Current={};}
        S.WasEligible=true;return S.Current;
    }
    S.WasEligible=true;
    if(S.Duration<=0)
    {
        S.Wait-=DT;if(S.Wait>0){S.Current={};return S.Current;}
        int32 Pick=S.Random.RandRange(1,4);
        if(Pick==int32(S.LastIntent))Pick=Pick%4+1;
        S.Peak={};S.Peak.Intent=EFrontlineIdleIntentV38(Pick);S.LastIntent=S.Peak.Intent;
        const float Side=S.Random.RandRange(0,1)?1.f:-1.f,Variation=S.Random.FRandRange(.83f,1.12f);
        const int32 Role=U->GetPresentationRoleIndex()%6;
        switch(S.Peak.Intent)
        {
        case EFrontlineIdleIntentV38::NearWatch:S.Peak.Head=FRotator(-1.4f,Side*10.f,-.8f);S.Peak.Torso=FRotator(0,Side*.8f,.35f);break;
        case EFrontlineIdleIntentV38::FarWatch:S.Peak.Head=FRotator(.8f,Side*(Role==3?14.f:11.5f),.5f);S.Peak.Torso=FRotator(0,Side*1.4f,-.5f);break;
        case EFrontlineIdleIntentV38::SettleLoad:S.Peak.Head=FRotator(-1.3f,Side*3.f,.3f);S.Peak.Torso=FRotator(Side*.65f,0,Side*1.45f);break;
        case EFrontlineIdleIntentV38::ChinTuck:S.Peak.Head=FRotator(0,Side*2.5f,-3.2f);S.Peak.Torso=FRotator(0,0,.35f);break;
        default:break;
        }
        S.Peak.Head*=Variation;S.Peak.Torso*=Variation;
        // The supported gun and its arms never move away from the bipod.
        // Crew resting at the ground gun use the same contact restriction.
        if(Gun&&Gun->IsSupported()) {S.Peak.Torso=FRotator::ZeroRotator;S.Peak.Head*=.45f;}
        S.Attack=S.Random.FRandRange(.75f,1.15f);S.Release=S.Random.FRandRange(.75f,1.05f);
        S.Duration=S.Attack+S.Release+S.Random.FRandRange(.55f,1.35f);S.Age=0;++S.GesturesStarted;
    }
    S.Age+=DT;
    const float Weight=Ease(S.Age/S.Attack)*Ease((S.Duration-S.Age)/S.Release);
    S.Current=Weighted(S.Peak,Weight);
    if(S.Age>=S.Duration){S.Current={};S.Duration=0;S.Wait=S.Random.FRandRange(4.5f,11.f);}
    return S.Current;
}

namespace
{
bool IsolatedV38(AWarfareFrontlineGameMode* B)
{
    FString UserDir;return B&&B->GetWorld()&&FParse::Param(FCommandLine::Get(),TEXT("WFV38Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
AWarfareFrontlineSoldier* MakeWaiter(AWarfareFrontlineGameMode* B,int32 Id,int32 Team,EFrontlineRole Role,int32 Member,int32 Trench,float Y,bool Ground)
{
    float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);if(Ground)X+=B->GetTeamDirection(Team)*660.f;
    else if((Role==EFrontlineRole::Lewis||Role==EFrontlineRole::MobileGun)&&Member==0)X+=B->GetTeamDirection(Team)*125.f;
    FVector P(X,Y,AWarfareFrontlineGround::HeightAt(X,Y));
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P+FVector(0,0,88),FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
    if(!U)return nullptr;U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->bEngaging=false;
    B->Units.Add(U);U->ReturnToIdle();return U;
}
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>>> IdleReviews;
}

bool FrontlineMotionV38::PrepareIdleReview(AWarfareFrontlineGameMode* B)
{
    if(!IsolatedV38(B)||!B->Trenches.IsValidIndex(FrontlineBattlefield::CentreTrench))return false;
    auto& Crew=IdleReviews.FindOrAdd(B);Crew.Empty();const int32 Trench=FrontlineBattlefield::CentreTrench;
    B->Trenches[Trench].Owner=0;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->SetActorTickEnabled(false);
    const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Rifle,EFrontlineRole::Lewis};
    for(int32 I=0;I<6;++I)
    {
        FFrontlineSquad S;S.Id=381000+I;S.Team=0;S.Role=Roles[I];S.Trench=S.Destination=Trench;S.Slot=1;
        if(auto* U=MakeWaiter(B,S.Id,0,Roles[I],0,Trench,-700.f+I*260.f,false)){S.Members.Add(U);Crew.Add(U);}
        B->Squads.Add(S);
    }
    FFrontlineSquad S;S.Id=381010;S.Team=0;S.Role=EFrontlineRole::Lewis;S.Trench=INDEX_NONE;S.Destination=Trench;
    for(int32 I=0;I<2;++I)if(auto* U=MakeWaiter(B,S.Id,0,S.Role,I,Trench,-260.f+I*110.f,true)){S.Members.Add(U);Crew.Add(U);}
    B->Squads.Add(S);return Crew.Num()==8;
}
void FrontlineMotionV38::TickIdleReview(AWarfareFrontlineGameMode* B,float DT)
{
    if(!IsolatedV38(B))return;const auto* Crew=IdleReviews.Find(B);if(!Crew)return;
    for(const auto& W:*Crew)if(auto* U=W.Get())if(auto* Gun=FrontlineMachinegunsV30::Get(U))
    {
        const bool Transition=Gun->Update(DT,false);
        if(auto* Anim=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
        {
            if(Transition)
            {
                UAnimationAsset* Clip=Gun->TransitionMotion();
                if(Clip&&Anim->GetCurrentAsset()!=Clip)Anim->TransitionTo(Clip,false,Gun->TransitionRate(),0,.10f);
                else if(Clip)Anim->SetPlayRate(Gun->TransitionRate());
            }
            else if(Anim->GetCurrentAsset()!=U->GetIdleMotion())U->ReturnToIdle();
        }
    }
}

TSharedRef<FJsonObject> FrontlineMotionV38::RunFocusedChecks(AWarfareFrontlineGameMode* B)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const TCHAR* Name,bool Pass){auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);Rows.Add(MakeShared<FJsonValueObject>(Row));};
    ON_SCOPE_EXIT{int32 Passed=0;for(const auto& Row:Rows)Passed+=Row->AsObject()->GetBoolField(TEXT("passed"));Report->SetArrayField(TEXT("checks"),Rows);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Rows.Num());Report->SetBoolField(TEXT("success"),Rows.Num()>0&&Passed==Rows.Num());};
    if(!IsolatedV38(B)){Check(TEXT("isolated_v38_motion_review_required"),false);return Report;}
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTrenches=B->Trenches;const auto Phase=B->Phase;const int32 Outro=B->BattleOutroWinner;
    TArray<AWarfareFrontlineSoldier*> Units;
    ON_SCOPE_EXIT{for(auto* U:Units)if(IsValid(U))U->Destroy();B->Units=SavedUnits;B->Squads=SavedSquads;B->Trenches=SavedTrenches;B->Phase=Phase;B->BattleOutroWinner=Outro;};
    B->Units.Empty();B->Squads.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    const int32 Trench=B->GetHomeTrenchIndex(0);if(!B->Trenches.IsValidIndex(Trench)){Check(TEXT("authored_trench_available"),false);return Report;}B->Trenches[Trench].Owner=0;
    for(int32 I=0;I<6;++I)
    {
        FFrontlineSquad S;S.Id=382000+I;S.Team=0;S.Role=I==5?EFrontlineRole::Lewis:EFrontlineRole::Rifle;S.Trench=S.Destination=Trench;
        if(auto* U=MakeWaiter(B,S.Id,0,S.Role,0,Trench,-500.f+I*180.f,false)){S.Members.Add(U);Units.Add(U);}B->Squads.Add(S);
    }
    Check(TEXT("six_actual_idle_characters_and_accepted_clips"),Units.Num()==6&&B->IdleAnimation!=nullptr);if(Units.Num()!=6)return Report;
    TArray<FFrontlineIdleStateV38> States;States.SetNum(6);TSet<int32> FirstStarts,Intents;float MaxHead=0,MaxTorso=0;bool Bounded=true,ReturningNeutral=true;
    for(int32 Step=0;Step<1200;++Step)for(int32 I=0;I<6;++I)
    {
        const int32 Before=States[I].GesturesStarted;const auto P=UpdateIdle(States[I],Units[I],Units[I]->GetIdleMotion(),.05f);
        if(Before==0&&States[I].GesturesStarted>0)FirstStarts.Add(Step);
        if(P.IsActive())Intents.Add(int32(P.Intent));
        MaxHead=FMath::Max(MaxHead,float(P.Head.GetManhattanDistance(FRotator::ZeroRotator)));MaxTorso=FMath::Max(MaxTorso,float(P.Torso.GetManhattanDistance(FRotator::ZeroRotator)));
        Bounded&=P.Weight>=0&&P.Weight<=1.f&&FMath::Abs(P.Head.Yaw)<16.f&&FMath::Abs(P.Torso.Yaw)<1.6f&&FMath::Abs(P.Torso.Roll)<1.7f;
        // IsActive is a visibility threshold, not the envelope's exact end.
        // Near-zero fade samples may correctly retain sub-0.02 degree turns.
        ReturningNeutral&=P.Weight>0.f||(P.Head.IsNearlyZero(.001f)&&P.Torso.IsNearlyZero(.001f));
    }
    Check(TEXT("independent_waits_do_not_restart_the_section_in_unison"),FirstStarts.Num()>=5);
    Check(TEXT("four_scheduled_idle_intents_have_bounded_motion_and_neutral_pauses"),Intents.Num()==4&&Bounded&&ReturningNeutral&&MaxHead>8&&MaxTorso>1);
    Report->SetNumberField(TEXT("distinct_first_gesture_times"),FirstStarts.Num());Report->SetNumberField(TEXT("maximum_idle_head_euler_sum_degrees"),MaxHead);
    Report->SetNumberField(TEXT("observed_idle_intents"),Intents.Num());Report->SetNumberField(TEXT("maximum_idle_torso_euler_sum_degrees"),MaxTorso);
    Report->SetBoolField(TEXT("idle_motion_inside_anatomical_bounds"),Bounded);Report->SetBoolField(TEXT("completed_idle_envelopes_return_exactly_to_neutral"),ReturningNeutral);
    auto* U=Units[0];bool Gated=true;
    for(EFrontlineWeaponPhase Action:{EFrontlineWeaponPhase::Recoil,EFrontlineWeaponPhase::BoltCycle,EFrontlineWeaponPhase::Reload,EFrontlineWeaponPhase::GrenadeThrow,EFrontlineWeaponPhase::OfficerCommand})
    {U->WeaponPhase=Action;Gated&=!UpdateIdle(States[0],U,U->GetIdleMotion(),.1f).IsActive();}
    U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->bMoving=true;Gated&=!UpdateIdle(States[0],U,U->GetWalkMotion(),.1f).IsActive();U->bMoving=false;U->Health=0;Gated&=!UpdateIdle(States[0],U,U->GetIdleMotion(),.1f).IsActive();U->Health=100;
    Check(TEXT("idle_layer_never_changes_reload_bolt_throw_locomotion_or_death"),Gated);
    auto* Gunner=Units[5];auto* Gun=FrontlineMachinegunsV30::Get(Gunner);bool Timing=Gun&&Gun->HasAssets(),FireGate=true,HeadOnly=true;
    if(Gun)
    {
        for(int32 Context=0;Context<2;++Context)
        {
            auto* S=B->FindSquad(Gunner->SquadId);S->Trench=Context==0?INDEX_NONE:Trench;Gun->State=EFrontlineGunStanceV30::Carry;Gunner->WeaponPhase=EFrontlineWeaponPhase::Ready;
            Gunner->SetActorRotation(FRotator(0,B->GetTeamDirection(0)>0?0:180,0));
            Gun->Update(.001f,false);const auto* Deploy=Cast<UAnimSequenceBase>(Gun->TransitionMotion());
            const float Expected=Deploy?Deploy->GetPlayLength()/1.30f:0;Timing&=Deploy&&Gun->State==EFrontlineGunStanceV30::Deploy&&FMath::IsNearlyEqual(Gun->TransitionDurationV38(),Expected,.0001f);FireGate&=!Gun->CanFire();
            if(Expected>0){Gun->Update(Expected*.90f,false);FireGate&=!Gun->CanFire();Gun->Update(Expected*.11f,false);Timing&=Gun->IsSupported()&&Gun->CanFire();}
            if(Context==1)
            {
                FFrontlineIdleStateV38 State;
                for(int32 I=0;I<700;++I){const auto P=UpdateIdle(State,Gunner,Gunner->GetIdleMotion(),.05f);HeadOnly&=P.Torso.IsNearlyZero(.0001f);}
                HeadOnly&=State.GesturesStarted>=2;
            }
            Gun->Update(.001f,true);const auto* Pack=Cast<UAnimSequenceBase>(Gun->TransitionMotion());const float Rise=Pack?Pack->GetPlayLength()/(1.35f*1.30f):0;
            Timing&=Pack&&FMath::IsNearlyEqual(Gun->TransitionDurationV38(),Rise,.0001f);FireGate&=!Gun->CanFire();
            if(Rise>0){FireGate&=Gun->Update(Rise*.9f,true);Gun->Update(Rise*.11f,true);Timing&=Gun->State==EFrontlineGunStanceV30::Carry;}
        }
    }
    Check(TEXT("actual_prone_and_rise_state_durations_are_thirty_percent_faster"),Timing);
    Check(TEXT("supported_fire_and_movement_still_wait_for_complete_transition"),FireGate);
    Check(TEXT("supported_machinegun_idle_keeps_torso_and_bipod_fixed"),HeadOnly);
    Report->SetStringField(TEXT("scope"),TEXT("Actual idle actors/clip eligibility, deterministic per-actor scheduler and actual MG transition state machine. Integrated worker-side grip/foot preservation and visible asynchronous motion are checked in the caller's two timed captures."));
    return Report;
}
