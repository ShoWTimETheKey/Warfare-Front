#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineCharacterV34.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUPresentationV82.h"
#if WITH_EDITOR
#include "WarfareFrontlineTripoCharacterV87.h"
#endif
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

struct FFrontlineMotionV41Access
{
    static UAnimationAsset* Play(AWarfareFrontlineSoldier* U,UAnimationAsset* Clip,float Rate=1.f,float Start=0.f,bool Restart=false)
    {
        if(!U||!U->Soldier||!Clip)return nullptr;
        // Resolve the actual display identity once for initial playback and
        // every repeated gesture. Restart must obey the same Skeleton gate.
        if(WarfareFRRUCampaignV82::Active(U->Battle))Clip=WarfareFRRUPresentationV82::RemapMotion(U,Clip);
#if WITH_EDITOR
        Clip=FrontlineTripoCharacterV87::RemapMotion(U,Clip);
#endif
        if(!Clip||!U->Soldier->GetSkeletalMeshAsset()||Clip->GetSkeleton()!=U->Soldier->GetSkeletalMeshAsset()->GetSkeleton())
        {UE_LOG(LogTemp,Error,TEXT("WF_OUTCOME_MOTION_REFUSED squad=%d member=%d actual=%s"),U->SquadId,U->MemberIndex,*GetNameSafe(Clip));return nullptr;}
        U->bMoving=U->bEngaging=U->bWantedMovement=false;U->MotionSpeed=U->RequestedTravelSpeedV43=0;U->FocusTarget.Reset();U->BurstRemaining=0;
        if(Restart)
        {
            if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
            {A->TransitionTo(Clip,false,Rate,Start,.22f);U->Playing=Clip;U->LastAnimationTime=Start;++U->MotionTransitions;}
        }
        else U->SetMotion(Clip,false,true);
        if(auto* N=U->Soldier->GetSingleNodeInstance();N&&N->GetAnimationAsset()==Clip)
        {N->SetPlayRate(Rate);N->SetPlaying(true);return Clip;}
        return nullptr;
    }
};

namespace FrontlineMotionV41
{
namespace
{
struct FPersonal
{
    FRandomStream Random;
    uint32 Identity=0;
    bool Initialized=false,Waiting=false,OutroPrepared=false,OutroStarted=false,Released=false;
    float NextAt=0,Until=0,LastWaitingAt=-1000,OutroStartedAt=0,Rate=1;
    int32 LastWait=INDEX_NONE,Waits=0,OutcomeTake=0,GestureCycles=0,HoldFrames=0;
    bool HoldingRaise=false,HeldRaise=false;
    float HoldStartedAt=0,HoldSeconds=0,ReleaseClipSeconds=-1;
    TWeakObjectPtr<UAnimationAsset> Clip,Outcome;
};
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FPersonal> People;
TMap<FString,TWeakObjectPtr<UAnimationAsset>> Clips;
uint32 Identity(const AWarfareFrontlineSoldier* U)
{return uint32(U->SquadId)*92821u^uint32(U->MemberIndex+1)*68917u^uint32(U->Team+1)*283923u^411917u;}
FPersonal& Person(AWarfareFrontlineSoldier* U)
{
    auto& S=People.FindOrAdd(U);const uint32 Key=Identity(U);
    if(!S.Initialized||S.Identity!=Key)
    {S=FPersonal();S.Identity=Key;S.Initialized=true;S.Random.Initialize(int32(Key));S.NextAt=U->Battle->Elapsed+S.Random.FRandRange(1.4f,7.8f);S.OutcomeTake=S.Random.RandRange(0,3);}
    return S;
}
UAnimationAsset* Load(const FString& Name)
{
    if(auto* A=Clips.FindRef(Name).Get())return A;
    const TCHAR* Revision=Name.EndsWith(TEXT("_V828"))?TEXT("V828"):TEXT("V41");
    auto* A=LoadObject<UAnimationAsset>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/%s/Animations/%s.%s"),Revision,*Name,*Name));
    if(A)Clips.Add(Name,A);return A;
}
float Length(UAnimationAsset* A){const auto* S=Cast<UAnimSequenceBase>(A);return S?S->GetPlayLength():0.f;}
const TCHAR* Family(const AWarfareFrontlineSoldier* U)
{return FrontlineFieldV32::IsFlameOperator(U)?TEXT("Flame"):U->IsCommissionedOfficer()?TEXT("Officer"):U->IsMachineGunner()?TEXT("MG"):TEXT("Rifle");}
bool CanWait(const AWarfareFrontlineSoldier* U,bool ClearContact)
{
    if(!U||!U->Battle||!U->IsActiveCombatant()||!ClearContact||U->bMoving||U->bEngaging||U->Suppression>=.32f
        ||U->WeaponPhase!=EFrontlineWeaponPhase::Ready||U->Battle->Phase!=EFrontlinePhase::Fighting
        ||U->Battle->BattleOutroWinner!=INDEX_NONE||U->Battle->CoverFor(U)<=0||FrontlineMotionV40::IsBusy(U))return false;
    const auto* Gun=FrontlineMachinegunsV30::Get(U);
    // A head glance remains safe at a planted gun; body shifts are carry-only.
    return !Gun||(!Gun->IsSupported()&&!Gun->TransitionMotion());
}
bool Isolated(AWarfareFrontlineGameMode* B)
{FString D;return B&&B->GetWorld()&&B->IsProgressionVolatile()
    &&(FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV828Review")))
    &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),D)&&!D.IsEmpty();}
}

void Reset(AWarfareFrontlineGameMode* B)
{for(auto I=People.CreateIterator();I;++I)if(!I.Key().IsValid()||I.Key()->Battle==B)I.RemoveCurrent();}

UAnimationAsset* FatalMotion(AWarfareFrontlineSoldier* U,EFrontlineDamageKind Kind,const FVector& Incoming,bool Prone,int32& Variant)
{
    if(!U||Prone||IsFrontlineExplosiveDamage(Kind)||Kind==EFrontlineDamageKind::Gas)return nullptr;
    const uint32 Key=Identity(U);Variant=int32(Key%3);
    if(Kind==EFrontlineDamageKind::Fire)
    {
        if(FrontlineFieldV32::IsFlameOperator(U)){Variant=2;return Load(TEXT("A_BurningFall_Fuel_V41"));}
        Variant=int32(Key%2);return Load(Variant?TEXT("A_BurningFall_Stagger_V41"):TEXT("A_BurningFall_Reel_V41"));
    }
    if(Kind!=EFrontlineDamageKind::Bullet&&Kind!=EFrontlineDamageKind::HeavyBullet)return nullptr;
    const float Side=FMath::Abs(float(FVector::DotProduct(Incoming.GetSafeNormal2D(),U->GetActorRightVector())));
    if(Side>.5f)Variant=2;
    static const TCHAR* Names[]={TEXT("A_Death_RecoilBack_V41"),TEXT("A_Death_BuckleFront_V41"),TEXT("A_Death_CrumpleSide_V41")};
    return Load(Names[Variant]);
}

void CancelWaiting(AWarfareFrontlineSoldier* U)
{
    if(auto* S=People.Find(U);S&&S->Waiting)
    {S->Waiting=false;S->Clip.Reset();S->NextAt=U->Battle->Elapsed+S->Random.FRandRange(2.8f,7.f);}
}
UAnimationAsset* CarryIdle(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle)return nullptr;
    if(const auto* G=FrontlineMachinegunsV30::Get(U);G&&G->State!=EFrontlineGunStanceV30::Carry)return nullptr;
    return Load(FString::Printf(TEXT("A_LowReady_%s_V41"),Family(U)));
}
UAnimationAsset* WaitingMotion(AWarfareFrontlineSoldier* U,float DT,bool ClearContact)
{
    if(!U||!U->Battle||DT<0||!FMath::IsFinite(DT))return nullptr;
    if(!CanWait(U,ClearContact)){CancelWaiting(U);return nullptr;}
    auto& S=Person(U);const float Now=U->Battle->Elapsed;
    if(S.Waiting&&Now-S.LastWaitingAt>.3f)CancelWaiting(U);
    S.LastWaitingAt=Now;
    if(S.Waiting)
    {
        if(Now<S.Until)return S.Clip.Get();
        S.Waiting=false;S.Clip.Reset();S.NextAt=Now+S.Random.FRandRange(5.4f,12.5f);return nullptr;
    }
    if(Now<S.NextAt)return nullptr;
    int32 Pick=S.Random.RandRange(0,1);if(Pick==S.LastWait)Pick=1-Pick;
    UAnimationAsset* A=Load(FString::Printf(TEXT("A_Wait_%s_%s_V41"),Family(U),Pick?TEXT("LookAlong"):TEXT("SettleKit")));
    if(!A){S.NextAt=Now+8.f;return nullptr;}
    S.Waiting=true;S.Clip=A;S.LastWait=Pick;S.Until=Now+Length(A);++S.Waits;
    return A;
}

void TickOutro(AWarfareFrontlineSoldier* U,float DT)
{
    if(!U||!U->Battle||!U->IsAlive()||U->Battle->BattleOutroWinner<0||!FMath::IsFinite(DT)||DT<=0)return;
    auto& S=Person(U);const float Now=U->Battle->BattleOutroElapsed;
    if(Now<.10f+U->GetMotionPhase()*.48f)return;
    const bool Won=U->Team==U->Battle->BattleOutroWinner;const FString F=Family(U);
    const bool CanRaise=F==TEXT("Rifle")&&U->Role!=EFrontlineRole::AntiTankRifle;
    const auto StartGesture=[&](bool First)
    {
        if(!First&&Won&&CanRaise)S.OutcomeTake=(S.OutcomeTake+S.Random.RandRange(1,3))%4;
        // Old kneeling/hopping assets remain preserved, but no live outcome
        // selects them. The two new rifle gestures retain one actual gun hand.
        static const TCHAR* RifleTakes[]={TEXT("RifleRaise"),TEXT("Fist"),TEXT("OneArmPump"),TEXT("OneArmSalute")};
        const bool OneArm=Won&&CanRaise&&S.OutcomeTake>=2;
        const TCHAR* Take=Won?(CanRaise?RifleTakes[S.OutcomeTake%4]:TEXT("Fist"))
            :(S.OutcomeTake%2==0?TEXT("HandsHigh"):TEXT("HandsNear"));
        const FString Name=FString::Printf(TEXT("A_%s_%s_%s_%s"),Won?TEXT("Victory"):TEXT("Surrender"),*F,Take,OneArm?TEXT("V828"):TEXT("V41"));
        auto* A=Load(Name);if(!A)return false;
        S.Rate=Won?S.Random.FRandRange(1.08f,1.26f):1.25f*S.Random.FRandRange(.92f,1.08f);
        auto* Displayed=FFrontlineMotionV41Access::Play(U,A,S.Rate,!Won&&!First?2.5f:0.f,!First);
        if(!Displayed)return false;
        S.OutroStarted=true;S.OutroStartedAt=Now;S.Outcome=Displayed;++S.GestureCycles;
        S.HoldingRaise=S.HeldRaise=false;S.HoldSeconds=S.Random.FRandRange(.60f,1.35f);
        // A surrendered man loops only the empty-handed settled section. He
        // never re-grasps a released gun or repeats the release transition.
        return true;
    };
    if(!S.OutroStarted)
    {
        if(!S.OutroPrepared){S.OutroPrepared=true;CancelWaiting(U);FrontlineMotionV40::Abort(U);U->ReturnToIdle();}
        // Existing full pack/rise animation precedes a standing gesture.
        if(auto* G=FrontlineMachinegunsV30::Get(U);G&&G->State!=EFrontlineGunStanceV30::Carry)
        {
            if(G->Update(DT,true)){if(auto* A=G->TransitionMotion())FFrontlineMotionV41Access::Play(U,A,G->TransitionRate());return;}
        }
        if(!StartGesture(true))return;
    }
    auto* Node=U->Soldier->GetSingleNodeInstance();if(!Node||Node->GetAnimationAsset()!=S.Outcome.Get())return;
    const float ClipTime=Node->GetCurrentTime();
    if(!Won&&!S.Released&&ClipTime>=.64f)
    {
        // Exactly the actual owner's gun, finish, bayonet and mass are released.
        // The hand opens at the authored release; the reservoir stays on the back.
        S.Released=FrontlineCharacterV34::ReleaseWeapon(U,false,FVector::ZeroVector)!=nullptr;
        if(S.Released)S.ReleaseClipSeconds=ClipTime;
    }
    // Keep the authored raised rifle/fist alive for a personal interval. Small
    // motion within the old raise plateau replaces a frozen final frame. The
    // new single-arm clips contain their own complete pumping rhythm.
    const bool AuthoredPump=Won&&CanRaise&&S.OutcomeTake>=2;
    if(Won&&!AuthoredPump&&!S.HeldRaise&&ClipTime>=.98f&&ClipTime<1.33f)
    {S.HoldingRaise=true;S.HeldRaise=true;S.HoldStartedAt=Now;}
    if(S.HoldingRaise)
    {
        if(Now-S.HoldStartedAt<S.HoldSeconds)
        {
            Node->SetPlaying(false);Node->SetPosition(.92f+.17f*(.5f+.5f*FMath::Sin((Now-S.HoldStartedAt)*3.1f+U->GetMotionPhase()*PI)),false);
            ++S.HoldFrames;return;
        }
        S.HoldingRaise=false;Node->SetPosition(1.33f,false);Node->SetPlaying(true);
    }
    if(Node->GetCurrentTime()>=Node->GetLength()-.035f)StartGesture(false);
}
UAnimationAsset* SpecialistAction(const AWarfareFrontlineSoldier* U,const TCHAR* Name,bool Trench)
{
    if(!U||U->MemberIndex!=0)return nullptr;
    if(U->Role==EFrontlineRole::RifleGrenadier)return Load(FString::Printf(TEXT("A_No23_%s_V41"),Name));
    if(U->Role!=EFrontlineRole::AntiTankRifle)return nullptr;
    return Load(FString::Printf(TEXT("A_AT_%s_%s_V41"),Trench?TEXT("Trench"):TEXT("Ground"),Name));
}

namespace
{
struct FReview {int32 Mode=0;float Time=0;TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Units;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FReview> Reviews;
AWarfareFrontlineSoldier* Make(AWarfareFrontlineGameMode* B,int32 Id,int32 Team,EFrontlineRole Role,FVector P,int32 Trench=INDEX_NONE)
{
    P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88;
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,Team?180:0,0));if(!U)return nullptr;
    U->Initialize(B,Id,Team,Role,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;B->Units.Add(U);
    FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=Role;S.Trench=Trench;S.bHoldPosition=true;S.Members.Add(U);B->Squads.Add(S);return U;
}
void Evaluate(AWarfareFrontlineSoldier* U)
{U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(0,false);U->Soldier->RefreshBoneTransforms();}
int32 ReleasedCount(AWarfareFrontlineSoldier* U)
{TInlineComponentArray<UStaticMeshComponent*> Parts(U);int32 Count=0;for(auto* C:Parts)if(C->ComponentHasTag(TEXT("WF_V34_RELEASED_WEAPON")))++Count;return Count;}
}

void RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!Isolated(B)){Check(TEXT("v41_motion_requires_isolated_volatile_profile"),false);return;}
    int32 Loaded=0,Expected=0;const auto Has=[&](const FString& Name){++Expected;auto* A=Load(Name);Loaded+=A&&Length(A)>.1f?1:0;};
    for(const TCHAR* F:{TEXT("Rifle"),TEXT("MG"),TEXT("Officer"),TEXT("Flame")})
    {
        for(const TCHAR* T:{TEXT("SettleKit"),TEXT("LookAlong")})Has(FString::Printf(TEXT("A_Wait_%s_%s_V41"),F,T));
        for(const TCHAR* T:{TEXT("HandsHigh"),TEXT("HandsNear"),TEXT("Kneel")})Has(FString::Printf(TEXT("A_Surrender_%s_%s_V41"),F,T));
        Has(FString::Printf(TEXT("A_Victory_%s_Fist_V41"),F));
        Has(FString::Printf(TEXT("A_LowReady_%s_V41"),F));
    }
    for(const TCHAR* N:{TEXT("A_Victory_Rifle_RifleRaise_V41"),TEXT("A_Victory_Rifle_ExhaustedHop_V41"),TEXT("A_Death_RecoilBack_V41"),TEXT("A_Death_BuckleFront_V41"),TEXT("A_Death_CrumpleSide_V41"),TEXT("A_BurningFall_Reel_V41"),TEXT("A_BurningFall_Stagger_V41"),TEXT("A_BurningFall_Fuel_V41")})Has(N);
    for(const TCHAR* C:{TEXT("Ground"),TEXT("Trench")})for(const TCHAR* T:{TEXT("Aim"),TEXT("Fire"),TEXT("Bolt"),TEXT("Reload"),TEXT("Deploy"),TEXT("Pack")})Has(FString::Printf(TEXT("A_AT_%s_%s_V41"),C,T));
    for(const TCHAR* A:{TEXT("Aim"),TEXT("Fire"),TEXT("Reload")})Has(FString::Printf(TEXT("A_No23_%s_V41"),A));
    Check(TEXT("v41_all51_authored_character_and_specialist_clips_load"),Loaded==51&&Expected==51);
    bool NewGestures=true;for(const TCHAR* N:{TEXT("A_Victory_Rifle_OneArmPump_V828"),TEXT("A_Victory_Rifle_OneArmSalute_V828")})
        {auto* A=Load(N);NewGestures&=A&&Length(A)>5.f;}
    Check(TEXT("v828_both_single_arm_rifle_clips_load"),NewGestures);
    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldTrenches=B->Trenches;const auto OldPhase=B->Phase;
    const float OldTime=B->Elapsed,OldOutroTime=B->BattleOutroElapsed;const int32 OldWinner=B->BattleOutroWinner;TArray<AWarfareFrontlineSoldier*> Made;
    ON_SCOPE_EXIT{for(auto* U:Made){People.Remove(U);if(IsValid(U))U->Destroy();}B->Units=OldUnits;B->Squads=OldSquads;B->Trenches=OldTrenches;B->Phase=OldPhase;B->Elapsed=OldTime;B->BattleOutroWinner=OldWinner;B->BattleOutroElapsed=OldOutroTime;};
    B->Units.Empty();B->Squads.Empty();B->Phase=EFrontlinePhase::Fighting;B->Elapsed=200;B->BattleOutroWinner=INDEX_NONE;
    const int32 T=FrontlineBattlefield::CentreTrench;
    if(!B->Trenches.IsValidIndex(T)){Check(TEXT("v41_motion_fixture_has_real_trench"),false);return;}B->Trenches[T].Owner=0;
    TSet<int32> Delays;for(int32 I=0;I<8;++I)
    {
        const float Y=-490+I*140;auto* U=Make(B,94100+I,0,EFrontlineRole::Rifle,FVector(FrontlineFieldV32::TrenchCentreX(T,Y),Y,0),T);
        if(U){Made.Add(U);Delays.Add(FMath::RoundToInt((Person(U).NextAt-B->Elapsed)*100));}
    }
    Check(TEXT("v41_waiting_schedules_are_independent_of_global_battle_randomness"),Made.Num()==8&&Delays.Num()>=7);
    if(Made.IsEmpty())return;auto* U=Made[0];auto& S=Person(U);S.NextAt=B->Elapsed;
    auto* First=WaitingMotion(U,.05f,true);Check(TEXT("v41_real_trench_wait_chooses_full_body_clip"),First&&First->GetName().StartsWith(TEXT("A_Wait_Rifle_")));
    U->bEngaging=true;Check(TEXT("v41_contact_interrupts_idle_without_delaying_weapon"),WaitingMotion(U,.05f,false)==nullptr&&!S.Waiting&&U->WeaponPhase==EFrontlineWeaponPhase::Ready);U->bEngaging=false;
    S.NextAt=B->Elapsed;auto* Second=WaitingMotion(U,.05f,true);
    Check(TEXT("v41_completed_waiting_intents_do_not_repeat_consecutively"),First&&Second&&First!=Second);
    U->WeaponPhase=EFrontlineWeaponPhase::Reload;Check(TEXT("v41_no_wait_pose_over_reload"),WaitingMotion(U,.05f,true)==nullptr&&!S.Waiting);U->WeaponPhase=EFrontlineWeaponPhase::Ready;
    int32 Variant=INDEX_NONE;auto* Bullet=FatalMotion(U,EFrontlineDamageKind::Bullet,U->GetActorRightVector(),false,Variant);
    Check(TEXT("v41_lateral_bullet_selects_grounded_side_collapse"),Bullet&&Variant==2&&Bullet->GetName()==TEXT("A_Death_CrumpleSide_V41"));
    Check(TEXT("v41_prone_explosion_and_gas_keep_their_existing_handoff"),!FatalMotion(U,EFrontlineDamageKind::Bullet,FVector::ForwardVector,true,Variant)&&!FatalMotion(U,EFrontlineDamageKind::Artillery,FVector::ForwardVector,false,Variant)&&!FatalMotion(U,EFrontlineDamageKind::Gas,FVector::ForwardVector,false,Variant));
    B->Phase=EFrontlinePhase::Defeat;B->BattleOutroWinner=1;B->BattleOutroElapsed=1.5f;TickOutro(U,.05f);
    auto& Outcome=Person(U);B->BattleOutroElapsed=Outcome.OutroStartedAt+.7f;
    if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPosition(.72f,false);Evaluate(U);TickOutro(U,.05f);TickOutro(U,.05f);
    Check(TEXT("v41_surrender_opens_hands_and_releases_one_actual_weapon"),Outcome.OutroStarted&&Outcome.Released&&ReleasedCount(U)==1&&Outcome.Outcome.IsValid()&&Outcome.Outcome->GetName().StartsWith(TEXT("A_Surrender_Rifle_")));
}

void RunOutcomeChecksV45(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!(FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV828Review")))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("v45_outcomes_require_isolated_volatile_profile"),false);return;}
    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldPhase=B->Phase;
    const float OldTime=B->Elapsed,OldOutro=B->BattleOutroElapsed;const int32 OldWinner=B->BattleOutroWinner;
    TArray<AWarfareFrontlineSoldier*> Made;
    ON_SCOPE_EXIT
    {
        for(auto* U:Made){People.Remove(U);if(IsValid(U))U->Destroy();}
        B->Units=OldUnits;B->Squads=OldSquads;B->Phase=OldPhase;B->Elapsed=OldTime;B->BattleOutroElapsed=OldOutro;B->BattleOutroWinner=OldWinner;
    };
    B->Units.Empty();B->Squads.Empty();B->Elapsed=200;B->BattleOutroElapsed=0;B->BattleOutroWinner=0;B->Phase=EFrontlinePhase::Victory;
    for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<5;++I)
    {
        auto* U=Make(B,94500+Team*8+I,Team,I==4?EFrontlineRole::Lewis:EFrontlineRole::Rifle,
            FVector((Team?1.f:-1.f)*FrontlineBattlefield::TrenchSpacing*.5f,-400+I*200,0));
        if(U){Made.Add(U);Person(U).OutcomeTake=I%4;Evaluate(U);}
    }
    Check(TEXT("v828_outcome_fixture_has_both_sides_four_rifle_takes_and_supported_weapon_family"),Made.Num()==10);if(Made.Num()!=10)return;
    float PeakRifle=-MAX_flt,PeakFist=-MAX_flt,MaxPlantedMotion=0;
    bool ReleaseAfterOpening=true,NoRetiredClips=true,SurrenderRate=true;int32 HoldSamples=0,OverheadSilhouetteFrames=0;
    int32 SingleArmFrames[2]={0,0};float SingleArmGripError=0;FTransform GripAnchors[2];bool GripSeen[2]={false,false};
    float PeakReceiver=-MAX_flt,PeakBothHands=-MAX_flt;
    float InitialFeet[10];for(int32 I=0;I<Made.Num();++I)InitialFeet[I]=Made[I]->Soldier->GetBoneLocation(TEXT("foot_l")).Z;
    TSet<int32> OnsetTimes;TSet<FString> WinnerClips;
    for(int32 Frame=0;Frame<320;++Frame)
    {
        B->Elapsed+=.05f;B->BattleOutroElapsed+=.05f;
        for(int32 I=0;I<Made.Num();++I)
        {
            auto* U=Made[I];const bool Started=Person(U).OutroStarted;
            U->Tick(.05f);U->Soldier->HandleExistingParallelEvaluationTask(true,true);
            U->Soldier->TickAnimation(.05f,false);U->Soldier->RefreshBoneTransforms();
            const auto& S=Person(U);auto* N=U->Soldier->GetSingleNodeInstance();if(!N||!S.OutroStarted)continue;
            const FString ActiveName=N->GetAnimationAsset()->GetName();NoRetiredClips&=!ActiveName.Contains(TEXT("ExhaustedHop"))&&!ActiveName.Contains(TEXT("Kneel"));
            if(!Started)OnsetTimes.Add(FMath::RoundToInt(S.OutroStartedAt*100));
            const float Foot=U->Soldier->GetBoneLocation(TEXT("foot_l")).Z-InitialFeet[I];
            if(U->Team==0)
            {
                WinnerClips.Add(N->GetAnimationAsset()->GetName());HoldSamples+=S.HoldingRaise;
                const FString Name=N->GetAnimationAsset()->GetName();const float Head=U->Soldier->GetBoneLocation(TEXT("Head")).Z;
                if(Name.Contains(TEXT("RifleRaise")))
                {
                    PeakRifle=FMath::Max(PeakRifle,float(U->GetMuzzleLocation().Z-Head));
                    const float Receiver=U->Soldier->GetBoneLocation(TEXT("weapon_rifle")).Z-Head;
                    const float Hands=FMath::Min(U->Soldier->GetBoneLocation(TEXT("hand_l")).Z,U->Soldier->GetBoneLocation(TEXT("hand_r")).Z)-Head;
                    PeakReceiver=FMath::Max(PeakReceiver,Receiver);PeakBothHands=FMath::Max(PeakBothHands,Hands);
                    OverheadSilhouetteFrames+=(Receiver>22.f&&Hands>20.f);
                }
                if(Name.Contains(TEXT("Fist")))PeakFist=FMath::Max(PeakFist,float(U->Soldier->GetBoneLocation(TEXT("hand_l")).Z-Head));
                if(Name.Contains(TEXT("OneArm"))&&N->GetCurrentTime()>.4f)
                {
                    const int32 Variant=Name.Contains(TEXT("Pump"))?0:1;
                    const auto Grip=U->Soldier->GetSocketTransform(TEXT("hand_r"),RTS_Component).GetRelativeTransform(U->Soldier->GetSocketTransform(TEXT("weapon_rifle"),RTS_Component));
                    if(!GripSeen[Variant]){GripAnchors[Variant]=Grip;GripSeen[Variant]=true;}
                    SingleArmGripError=FMath::Max(SingleArmGripError,float(FVector::Dist(Grip.GetLocation(),GripAnchors[Variant].GetLocation())));
                    SingleArmFrames[Variant]+=U->Soldier->GetBoneLocation(TEXT("hand_r")).Z>Head+8.f&&U->Soldier->GetBoneLocation(TEXT("hand_l")).Z<Head-20.f;
                }
                if(N->GetCurrentTime()>.4f)MaxPlantedMotion=FMath::Max(MaxPlantedMotion,FMath::Abs(Foot));
            }
            else
            {
                SurrenderRate&=S.Rate>=1.15f-UE_KINDA_SMALL_NUMBER&&S.Rate<=1.35f+UE_KINDA_SMALL_NUMBER&&FMath::IsNearlyEqual(N->GetPlayRate(),S.Rate);
                if(S.Released)ReleaseAfterOpening&=S.ReleaseClipSeconds>=.64f;
                else if(N->GetCurrentTime()<.60f)ReleaseAfterOpening&=ReleasedCount(U)==0;
            }
        }
    }
    bool Cycling=true,Surrender=true,BoundedArms=true;int32 SolvedFrames=0,RejectedFrames=0;float GripError=0;for(auto* U:Made)
    {
        const auto& S=Person(U);Cycling&=S.GestureCycles>=2;
        if(U->Team==1)Surrender&=S.Released&&ReleasedCount(U)==1;
        if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
        {
            const auto D=A->GetCharacterDiagnostics();SolvedFrames+=D.OverheadPoseFramesV45;RejectedFrames+=D.OverheadRejectedFramesV45;
            GripError=FMath::Max(GripError,D.MaximumOverheadGripErrorCmV45);
            BoundedArms&=D.MaximumArmLengthErrorCm<.05f&&D.MaximumOverheadShoulderDegreesV45<=170.f&&D.MaximumOverheadElbowDegreesV45<=155.f;
        }
    }
    Check(TEXT("v45_outcomes_stagger_and_continue_after_the_first_authored_take"),Cycling&&OnsetTimes.Num()>=4&&WinnerClips.Num()>=5);
    Check(TEXT("v45_rifles_and_empty_fists_raise_above_heads_without_freezing"),PeakRifle>8.f&&PeakFist>8.f&&HoldSamples>30);
    Check(TEXT("v45_overhead_receiver_and_both_hands_have_real_closed_grips_and_bounded_arms"),OverheadSilhouetteFrames>20&&SolvedFrames>40&&RejectedFrames==0&&GripError<.15f&&BoundedArms);
    Check(TEXT("v828_victory_never_hops_and_all_feet_remain_planted"),NoRetiredClips&&MaxPlantedMotion<5.f);
    Check(TEXT("v828_two_distinct_single_arm_rifle_gestures_retain_actual_gun_hand"),SingleArmFrames[0]>20&&SingleArmFrames[1]>20&&SingleArmGripError<.15f);
    Check(TEXT("v828_standing_surrender_plays_exactly_25_percent_faster"),NoRetiredClips&&SurrenderRate);
    Check(TEXT("v45_surrender_releases_once_at_the_published_open_hand_pose"),Surrender&&ReleaseAfterOpening);
    UE_LOG(LogTemp,Display,TEXT("WF_V828_OUTCOMES onset_times=%d clips=%d held_frames=%d rifle_above_head_cm=%.2f fist_above_head_cm=%.2f planted_motion_cm=%.2f single_arm_frames=%d,%d grip_error_cm=%.4f"),OnsetTimes.Num(),WinnerClips.Num(),HoldSamples,PeakRifle,PeakFist,MaxPlantedMotion,SingleArmFrames[0],SingleArmFrames[1],SingleArmGripError);
    UE_LOG(LogTemp,Display,TEXT("WF_V45_OVERHEAD receiver_above_head_cm=%.2f both_hands_above_head_cm=%.2f silhouette_frames=%d solved=%d rejected=%d grip_error_cm=%.4f"),PeakReceiver,PeakBothHands,OverheadSilhouetteFrames,SolvedFrames,RejectedFrames,GripError);
}

void PrepareActionReview(AWarfareFrontlineGameMode* B,int32 Mode)
{
    if(!Isolated(B))return;
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();Reset(B);
    B->SetActorTickEnabled(false);B->Phase=EFrontlinePhase::Fighting;B->Elapsed=200;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;
    auto& R=Reviews.FindOrAdd(B);R=FReview();R.Mode=Mode;
    if(Mode==1)
    {
        const int32 T=FrontlineBattlefield::CentreTrench;if(!B->Trenches.IsValidIndex(T))return;B->Trenches[T].Owner=0;
        for(int32 I=0;I<8;++I)
        {const float Y=-560+I*160;auto* U=Make(B,94200+I,0,I==2?EFrontlineRole::Mills:I==4?EFrontlineRole::Pioneer:EFrontlineRole::Rifle,FVector(FrontlineFieldV32::TrenchCentreX(T,Y),Y,0),T);if(U)R.Units.Add(U);}
    }
    else
    {
        const int32 Count=Mode==2?8:6;
        for(int32 I=0;I<Count;++I)
        {
            const int32 Team=Mode==2&&I>=4?1:0;const EFrontlineRole Role=Mode==0&&I==5?EFrontlineRole::Flamethrower:EFrontlineRole::Rifle;
            int32 Id=94300+I*31;if(Mode==0)while((uint32(Id)*92821u^68917u^283923u^411917u)%(I<3?3u:2u)!=uint32(I<3?I:(I-3)%2))++Id;
            auto* U=Make(B,Id,Team,Role,FVector((I-(Count-1)*.5f)*(Mode==2?280.f:370.f),Mode==2&&Team?140:-140,0));if(!U)continue;R.Units.Add(U);
            if(Mode==2){auto& S=Person(U);S.OutcomeTake=I%4;}
            else U->ReceiveHit(1,.1f,1,I==2?U->GetActorRightVector():-U->GetActorForwardVector(),I<3?EFrontlineDamageKind::Bullet:EFrontlineDamageKind::Fire,.10f);
            if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlaying(false);Evaluate(U);
        }
        if(Mode==2){B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;}
    }
}

void TickActionReview(AWarfareFrontlineGameMode* B,float DT)
{
    if(!Isolated(B)||!Reviews.Contains(B)||!FMath::IsFinite(DT))return;auto& R=Reviews.FindChecked(B);DT=FMath::Clamp(DT,0.f,.05f);R.Time+=DT;B->Elapsed=200+R.Time;
    if(R.Mode==2)B->BattleOutroElapsed=R.Time;else if(R.Mode==0)FrontlineFieldV32::Tick(B,DT);
    for(const auto& W:R.Units)if(auto* U=W.Get())
    {
        if(R.Mode==0)
        {
            FrontlineFieldV32::TickUnit(U,DT);
            const auto& D=U->GetDeathState();
            if(D.bAuthoredFall&&!D.bPhysicsStarted)
            {if(auto* N=U->Soldier->GetSingleNodeInstance()){N->SetPlaying(false);N->SetPosition(FMath::Min(R.Time*(D.Kind==EFrontlineDamageKind::Bullet?1.30f:1.f),D.AuthoredClipSeconds),false);}Evaluate(U);}
            U->TickDeath(DT);
        }
        else if(R.Mode==1)
        {
            auto* A=WaitingMotion(U,DT,true);if(A)FFrontlineMotionV41Access::Play(U,A);else U->ReturnToIdle();
            if(auto* N=U->Soldier->GetSingleNodeInstance())
            {const auto& S=Person(U);N->SetPlaying(false);N->SetPosition(A?FMath::Clamp(R.Time+200-(S.Until-Length(A)),0.f,Length(A)):FMath::Fmod(R.Time,FMath::Max(.1f,N->GetLength())),false);}Evaluate(U);
        }
        else
        {
            // This isolated fixture advances explicitly below. Freeze between
            // evaluations so the world's component tick cannot add another DT.
            if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlaying(!Person(U).HoldingRaise);
            TickOutro(U,DT);U->Soldier->HandleExistingParallelEvaluationTask(true,true);
            U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
            if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlaying(false);
        }
    }
}

TSharedRef<FJsonObject> ActionReviewState(AWarfareFrontlineGameMode* B)
{
    auto O=MakeShared<FJsonObject>();const auto* R=Reviews.Find(B);if(!R){O->SetBoolField(TEXT("prepared"),false);return O;}
    O->SetBoolField(TEXT("prepared"),true);O->SetNumberField(TEXT("mode"),R->Mode);O->SetNumberField(TEXT("review_game_seconds"),R->Time);TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& W:R->Units)if(auto* U=W.Get())
    {
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("squad"),U->SquadId);J->SetNumberField(TEXT("team"),U->Team);J->SetBoolField(TEXT("alive"),U->IsAlive());
        if(auto* N=U->Soldier->GetSingleNodeInstance()){J->SetStringField(TEXT("clip"),GetNameSafe(N->GetAnimationAsset()));J->SetNumberField(TEXT("clip_seconds"),N->GetCurrentTime());J->SetNumberField(TEXT("play_rate_v828"),N->GetPlayRate());}
        J->SetNumberField(TEXT("pelvis_above_feet_cm"),U->Soldier->GetBoneLocation(TEXT("pelvis")).Z-U->Feet().Z);J->SetNumberField(TEXT("left_foot_above_ground_cm"),U->Soldier->GetBoneLocation(TEXT("foot_l")).Z-U->Feet().Z);
        const auto& D=U->GetDeathState();J->SetBoolField(TEXT("authored_fall_complete"),D.bAuthoredFall&&D.Age>=D.AuthoredClipSeconds);J->SetBoolField(TEXT("physics_started"),D.bPhysicsStarted);J->SetBoolField(TEXT("physics_finite"),!D.bNonFinite&&!D.bMissingPhysics);J->SetBoolField(TEXT("settled"),D.bSettled);J->SetNumberField(TEXT("body_speed_cm_s"),D.CurrentBodySpeedCmS);J->SetNumberField(TEXT("maximum_pelvis_rise_cm"),D.MaximumPelvisRiseCm);
        J->SetBoolField(TEXT("fuel_rupture_interrupted_authored_fall"),D.bFuelInterruptedAuthoredFall);J->SetBoolField(TEXT("fuel_carrier_impulse_applied"),D.bFuelRuptureApplied);J->SetBoolField(TEXT("physics_handoff_queued"),D.PendingPhysicsFrame!=MAX_uint64);J->SetNumberField(TEXT("fuel_rupture_pose_seconds"),D.FuelRupturePoseSeconds);J->SetNumberField(TEXT("fuel_delta_speed_cm_s"),D.FuelRuptureDeltaV.Size());
        bool Ignited=false,Spent=false;const float FuelRemaining=FrontlineFieldV32::FuelCookoffRemainingV41(U,Ignited,Spent);J->SetBoolField(TEXT("fuel_ignited"),Ignited);J->SetBoolField(TEXT("fuel_spent"),Spent);J->SetNumberField(TEXT("fuel_cookoff_remaining_seconds"),FuelRemaining);
        if(const auto* S=People.Find(U)){J->SetNumberField(TEXT("waiting_takes_started"),S->Waits);J->SetBoolField(TEXT("waiting_active"),S->Waiting);J->SetBoolField(TEXT("outro_started"),S->OutroStarted);J->SetBoolField(TEXT("weapon_released"),S->Released);J->SetNumberField(TEXT("outcome_take"),S->OutcomeTake);J->SetNumberField(TEXT("gesture_cycles_v45"),S->GestureCycles);J->SetNumberField(TEXT("raised_hold_frames_v45"),S->HoldFrames);J->SetNumberField(TEXT("weapon_release_clip_seconds_v45"),S->ReleaseClipSeconds);}
        if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
        {
            const auto Dg=A->GetCharacterDiagnostics();J->SetNumberField(TEXT("invalid_bone_frames"),Dg.InvalidBoneFrames);J->SetNumberField(TEXT("maximum_grip_error_cm"),A->GetMaximumGripResidualCm());J->SetNumberField(TEXT("maximum_leg_length_error_cm"),Dg.MaximumTerrainLegLengthErrorCm);
            J->SetNumberField(TEXT("overhead_pose_frames_v45"),Dg.OverheadPoseFramesV45);J->SetNumberField(TEXT("overhead_rejected_frames_v45"),Dg.OverheadRejectedFramesV45);
            J->SetNumberField(TEXT("overhead_grip_error_cm_v45"),Dg.MaximumOverheadGripErrorCmV45);
            J->SetNumberField(TEXT("overhead_maximum_shoulder_degrees_v45"),Dg.MaximumOverheadShoulderDegreesV45);
            J->SetNumberField(TEXT("overhead_maximum_elbow_degrees_v45"),Dg.MaximumOverheadElbowDegreesV45);
            const float Head=U->Soldier->GetBoneLocation(TEXT("Head")).Z;
            J->SetNumberField(TEXT("right_hand_above_head_joint_cm_v828"),U->Soldier->GetBoneLocation(TEXT("hand_r")).Z-Head);
            J->SetNumberField(TEXT("left_hand_above_head_joint_cm_v828"),U->Soldier->GetBoneLocation(TEXT("hand_l")).Z-Head);
            J->SetNumberField(TEXT("receiver_above_head_joint_cm"),U->Soldier->GetBoneLocation(TEXT("weapon_rifle")).Z-Head);
            J->SetNumberField(TEXT("both_hands_above_head_joint_cm"),FMath::Min(U->Soldier->GetBoneLocation(TEXT("hand_l")).Z,U->Soldier->GetBoneLocation(TEXT("hand_r")).Z)-Head);
        }
        Rows.Add(MakeShared<FJsonValueObject>(J));
    }
    O->SetArrayField(TEXT("units"),Rows);return O;
}
}
