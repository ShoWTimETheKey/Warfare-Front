#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineGameplayV51.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

struct FFrontlineMotionV40Access
{
    struct FWeaponCycleV851
    {
        EFrontlineWeaponPhase Phase=EFrontlineWeaponPhase::Ready;
        float Remaining=0,Position=0,Rate=1;
        bool Valid=false,Looping=false;
        TWeakObjectPtr<UAnimationAsset> Clip;
    };
    static void SuspendWeaponV851(AWarfareFrontlineSoldier* U,FWeaponCycleV851& Saved)
    {
        if(Saved.Valid)return;
        Saved.Phase=U->WeaponPhase;Saved.Remaining=U->FirePoseClock;
        Saved.Valid=Saved.Phase!=EFrontlineWeaponPhase::Ready;
        if(auto* Node=U->Soldier->GetSingleNodeInstance())
        {Saved.Clip=Node->GetCurrentAsset();Saved.Position=Node->GetCurrentTime();Saved.Rate=Node->GetPlayRate();Saved.Looping=Node->IsLooping();}
        // Pause the unfinished weapon action. In particular, donning a mask
        // neither fills a magazine nor silently finishes its interrupted reload.
        U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->FirePoseClock=0;U->BurstRemaining=0;
    }
    static void RestoreWeaponV851(AWarfareFrontlineSoldier* U,FWeaponCycleV851& Saved)
    {
        if(!Saved.Valid)return;
        U->WeaponPhase=Saved.Phase;U->FirePoseClock=Saved.Remaining;
        if(auto* Clip=Saved.Clip.Get())
        {
            U->SetMotion(Clip,Saved.Looping);
            if(auto* Node=U->Soldier->GetSingleNodeInstance())
            {Node->SetPosition(Saved.Position,false);Node->SetPlayRate(Saved.Rate);}
        }
        Saved.Valid=false;
    }
    static void SeedWeaponV851(AWarfareFrontlineSoldier* U,EFrontlineWeaponPhase Phase)
    {U->WeaponPhase=Phase;U->RoundsRemaining=1;U->FirePoseClock=8.f;U->ShotClock=100.f;}
    static float RemainingWeaponV851(const AWarfareFrontlineSoldier* U){return U->FirePoseClock;}
    static void Motion(AWarfareFrontlineSoldier* U,UAnimationAsset* Clip){U->SetMotion(Clip);}
    static void Pose(AWarfareFrontlineSoldier* U,UAnimationAsset* Clip,float Time=-1.f,float Rate=1.f)
    {
        U->bMoving=U->bEngaging=U->bWantedMovement=false;U->MotionSpeed=U->RequestedTravelSpeedV43=0;U->FocusTarget.Reset();U->BurstRemaining=0;
        U->SetMotion(Clip,false);
        if(auto* N=U->Soldier->GetSingleNodeInstance()){N->SetPlayRate(Rate);if(Time>=0)N->SetPosition(Time,false);}
    }
    static void Approach(AWarfareFrontlineSoldier* U,const FVector& Before,float DT)
    {
        const float Travel=FVector::Dist2D(Before,U->Feet());U->DistanceTravelled+=Travel;
        U->MotionSpeed=Travel/FMath::Max(DT,SMALL_NUMBER);U->RequestedTravelSpeedV43=0;
        U->bMoving=U->MotionSpeed>8.f;U->bWantedMovement=true;U->bEngaging=false;U->FocusTarget.Reset();
        if(U->bMoving)U->SetActorRotation(FMath::RInterpTo(U->GetActorRotation(),(U->Feet()-Before).GetSafeNormal2D().Rotation(),DT,6.f));
        U->SetMotion(U->bMoving?U->GetWalkMotion():U->GetIdleMotion());
        if(U->bMoving)if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlayRate(FMath::Clamp(U->MotionSpeed/210.f,.08f,1.45f));
    }
    static void Escape(AWarfareFrontlineSoldier* U,const FVector& Before,float DT)
    {
        const float Travel=FVector::Dist2D(Before,U->Feet());U->DistanceTravelled+=Travel;
        U->MotionSpeed=Travel/FMath::Max(DT,SMALL_NUMBER);U->RequestedTravelSpeedV43=0;
        U->bMoving=U->MotionSpeed>8.f;U->bWantedMovement=U->bMoving;U->bEngaging=false;U->FocusTarget.Reset();U->BurstRemaining=0;
        if(U->bMoving)U->SetActorRotation((U->Feet()-Before).GetSafeNormal2D().Rotation());
        U->SetMotion(U->bMoving?(U->GetRunMotion()?U->GetRunMotion():U->GetWalkMotion()):U->GetAimMotion());
        if(U->bMoving)if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlayRate(FMath::Clamp(U->MotionSpeed/(U->GetRunMotion()?390.f:210.f),.55f,1.35f));
    }
    static void PlaceSupported(AWarfareFrontlineSoldier* U,const FVector& Point,float DT)
    {
        const float Surface=FrontlineInfantryV45::SurfaceHeight(U->Battle,Point);
        float Z=Surface;
        if(!WarfareFRRUCampaignV82::Active(U->Battle)&&Surface<U->Feet().Z-3.f)
        {
            Z=U->Feet().Z;
            for(float Left=DT;Left>0.f;)
            {
                const float Step=FMath::Min(Left,1.f/120.f);Left-=Step;
                U->TrenchFallSpeedV69=FMath::Min(U->TrenchFallSpeedV69+980.f*Step,650.f);
                Z=FMath::Max(Surface,Z-U->TrenchFallSpeedV69*Step);
                if(Z<=Surface){U->TrenchFallSpeedV69=0.f;break;}
            }
        }
        else U->TrenchFallSpeedV69=0.f;
        U->SetActorLocation(FVector(Point.X,Point.Y,Z+88.f));
    }
    static FFrontlineArmyProgress& Research(AWarfareFrontlineGameMode* B){return B->ActiveResearchMutable();}
    static int32 Mission(AWarfareFrontlineGameMode* B,int32 Next){const int32 Old=B->BoundCampaignMission;B->BoundCampaignMission=Next;return Old;}
};

namespace FrontlineMotionV40
{
namespace
{
enum class EAction:uint8{None,Don,Doff,ApproachWire,Cut,MineWork,MineRise};
struct FState
{
    EAction Action=EAction::None;
    bool Sealed=false,CompletedCut=false,Warned=false;
    float Time=0,Duration=3,ClearFor=0,Dose=0,Threshold=0,CompleteHold=0;
    float Concentration=0,DoseRate=0,DistressCheckAt=0;
    int32 GasSource=INDEX_NONE;
    bool DistressHeard=false;
    bool EscapeAttemptedV8216=false;
    float EscapeRemainingV8216=0;
    FVector EscapeDirectionV8216=FVector::ZeroVector;
    int32 EscapeStartsV836=0,EscapeFramesV836=0,EscapePackFramesV836=0,EscapeRunFramesV836=0,EscapeBlockedFramesV836=0;
    float EscapeTravelV836=0,EscapeBelowSurfaceV836=0;
    FVector Wire=FVector::ZeroVector;
    TWeakObjectPtr<UAnimationAsset> Clip;
    TWeakObjectPtr<UStaticMeshComponent> Tools[2];
    int32 DonCompleted=0,CutsCompleted=0;
    FFrontlineMotionV40Access::FWeaponCycleV851 SuspendedWeaponV851;
    TWeakObjectPtr<UStaticMeshComponent> MineProbeV835;
    FVector MineCentreV835=FVector::ZeroVector,MineContactV835=FVector::ZeroVector;
    float MineStartedAtV835=0;
    bool MineWeaponSavedV835=false,MineWeaponWasHiddenV835=false;
    int32 MineStartsV835=0,MineWorkingFramesV835=0;
};
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FState> States;
TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> ReviewUnits;
float ReviewTime=0;
TWeakObjectPtr<UStaticMeshComponent> ReviewWire;
bool ReviewWireConsumed=false;
float Ease(float T){T=FMath::Clamp(T,0.f,1.f);return T*T*(3-2*T);}
FState& State(AWarfareFrontlineSoldier* U)
{
    auto& S=States.FindOrAdd(U);
    if(S.Threshold==0){const uint32 Identity=uint32(U->SquadId)*73856093u^uint32(U->MemberIndex)*19349663u^uint32(U->Team)*83492791u;S.Threshold=12.f+(Identity%501)/100.f;}
    return S;
}
UAnimationAsset* Load(const FString& Name)
{return LoadObject<UAnimationAsset>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V40/Animations/%s.%s"),*Name,*Name));}
UAnimationAsset* MaskClip(AWarfareFrontlineSoldier* U,bool Don)
{
    const TCHAR* Family=FrontlineFieldV32::IsFlameOperator(U)?TEXT("Flame"):U->IsCommissionedOfficer()?TEXT("Officer"):U->IsMachineGunner()?TEXT("MG"):TEXT("Rifle");
    return Load(FString::Printf(TEXT("A_Mask%s_%s_V40"),Don?TEXT("Don"):TEXT("Doff"),Family));
}
float Concentration(const AWarfareFrontlineSoldier* U,int32& Source,bool& Warning)
{
    Source=INDEX_NONE;Warning=false;float Max=0;
    const float Clock=U->Battle->CommittedSupportTimeV8217();
    for(const auto& Z:U->Battle->SmokeZones)if(Z.bGas&&Clock>=Z.StartedAt&&Clock<Z.ExpiresAt)
    {
        const FVector D=U->Feet()-Z.Centre;const float R=FMath::Max(1.f,Z.Radius);
        const float Q=FMath::Sqrt(FMath::Square(D.X/R)+FMath::Square(D.Y/(R*.758f)));
        Warning|=Q<1.20f;if(Q>=1.f)continue;
        const float C=FMath::Lerp(1.f,.55f,Ease(Q));
        if(C>Max){Max=C;Source=Z.Team;}
    }
    return Max;
}
void Voice(AWarfareFrontlineSoldier* U,EFrontlineVoiceEvent Event)
{if(auto* A=U->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->PlayVoice(Event,U->GetFaction()==EFrontlineFaction::German,U->GetActorLocation(),U->SquadId);}
void HideTools(FState& S)
{for(auto& T:S.Tools)if(T.IsValid())T->SetVisibility(false);if(S.MineProbeV835.IsValid())S.MineProbeV835->SetVisibility(false);}
void RestoreMineEquipmentV835(AWarfareFrontlineSoldier* U,FState& S)
{
    if(S.MineProbeV835.IsValid())S.MineProbeV835->SetVisibility(false);
    // A fatal release owns the hidden weapon thereafter. Never resurrect it
    // on a corpse, or unhide a weapon that was hidden before this work action.
    if(S.MineWeaponSavedV835&&U&&U->IsAlive()&&U->Soldier&&!S.MineWeaponWasHiddenV835)
        U->Soldier->UnHideBoneByName(TEXT("weapon_rifle"));
    S.MineWeaponSavedV835=false;
}
void MineProbePoseV835(AWarfareFrontlineSoldier* U,FState& S,const FVector& Contact,bool Visible)
{
    auto* Probe=S.MineProbeV835.Get();
    if(!Probe)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        auto* Steel=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Characters/V40/Tools/M_WireCutter_Steel_V40.M_WireCutter_Steel_V40"));
        if(!Mesh||!Steel)return;
        Probe=NewObject<UStaticMeshComponent>(U,TEXT("MineProbeV835"));U->AddInstanceComponent(Probe);
        Probe->SetupAttachment(U->GetRootComponent());Probe->SetStaticMesh(Mesh);Probe->SetMaterial(0,Steel);
        Probe->SetCollisionEnabled(ECollisionEnabled::NoCollision);Probe->SetCanEverAffectNavigation(false);
        Probe->ComponentTags.Add(TEXT("WF_V835_MINE_PROBE"));Probe->RegisterComponent();S.MineProbeV835=Probe;
    }
    const FVector Hand=U->Soldier->GetBoneLocation(TEXT("hand_r"));
    const FVector Axis=Hand-Contact;const float Length=Axis.Size();
    const bool Safe=Visible&&!Hand.ContainsNaN()&&!Contact.ContainsNaN()&&Length>8.f&&Length<125.f;
    Probe->SetVisibility(Safe);S.MineContactV835=Contact;
    if(!Safe)return;
    Probe->SetWorldLocation((Hand+Contact)*.5f);
    Probe->SetWorldRotation(FRotationMatrix::MakeFromZ(Axis).Rotator());
    Probe->SetWorldScale3D(FVector(.008f,.008f,Length/100.f));
}
void ToolPose(AWarfareFrontlineSoldier* U,FState& S)
{
    const float T=S.Time/FMath::Max(.1f,S.Duration),Visible=Ease(T/.24f)*(1-Ease((T-.80f)/.2f));
    const float Close=Ease((T-.38f)/.19f)*(1-Ease((T-.70f)/.13f));
    for(int32 I=0;I<2;++I)
    {
        auto* C=S.Tools[I].Get();if(!C)
        {
            auto* M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Characters/V40/Tools/SM_WireCutterHalf_V40.SM_WireCutterHalf_V40"));if(!M)continue;
            C=NewObject<UStaticMeshComponent>(U);C->SetupAttachment(U->GetRootComponent());C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);U->AddInstanceComponent(C);C->RegisterComponent();S.Tools[I]=C;
        }
        C->SetVisibility(Visible>.5f);
        const FVector F=U->GetActorForwardVector(),R=U->GetActorRightVector();
        // Mesh hinge is at origin; the two beveled blades close around the wire,
        // while the authored wrists compress its separate long handles.
        const float HandleSign=C->GetStaticMesh()->GetBounds().Origin.Y>=0?1.f:-1.f;
        const FQuat Q=FRotationMatrix::MakeFromY(-F*HandleSign).ToQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians((I?1:-1)*FMath::Lerp(21.f,10.f,Close)));
        C->SetWorldLocationAndRotation(U->Feet()+F*67+FVector(0,0,58)+R*(I?.55f:-.55f),Q);
    }
}
bool Begin(AWarfareFrontlineSoldier* U,FState& S,EAction Action,UAnimationAsset* Clip)
{
    auto* Sequence=Cast<UAnimSequenceBase>(Clip);if(!Sequence)return false;
    S.Action=Action;S.Time=0;S.Duration=Sequence->GetPlayLength();S.Clip=Clip;
    FFrontlineMotionV40Access::Pose(U,Clip,0);return true;
}
bool PackForMask(AWarfareFrontlineSoldier* U,float DT)
{
    if(auto* Gun=FrontlineMachinegunsV30::Get(U);Gun&&Gun->State!=EFrontlineGunStanceV30::Carry)
    {Gun->Update(DT,true);if(auto* A=Gun->TransitionMotion())FFrontlineMotionV40Access::Pose(U,A,-1,Gun->TransitionRate());return Gun->State!=EFrontlineGunStanceV30::Carry;}
    return false;
}
bool Eligible(const AWarfareFrontlineSoldier* U)
{return U&&U->IsActiveCombatant()&&U->Battle&&U->Battle->Phase==EFrontlinePhase::Fighting;}
bool MineDangerV835(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle)return true;
    for(const AWarfareFrontlineSoldier* Enemy:U->Battle->Units)if(IsValid(Enemy)&&Enemy->Team!=U->Team&&Enemy->IsActiveCombatant()
        &&FVector::DistSquared2D(U->Feet(),Enemy->Feet())<FMath::Square(650.f))return true;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(const AFrontlineTank* Tank:Fleet->GetVehicles())
        if(IsValid(Tank)&&Tank->Team!=U->Team&&Tank->IsOperational()&&Tank->HasEnteredField()&&!Tank->IsHidden()
            &&FVector::DistSquared2D(U->Feet(),Tank->GrenadeAimPointFor(U->Feet()))<=FMath::Square(650.f))return true;
    return false;
}
bool Isolated(const AWarfareFrontlineGameMode* B)
{
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV8215Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))return FrontlineEffectsReviewSafetyV8216::Allowed(B);
    FString UserDir;return B&&B->GetWorld()&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
AWarfareFrontlineSoldier* Make(AWarfareFrontlineGameMode* B,int32 Id,EFrontlineRole Role,FVector P)
{
    P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88;
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator::ZeroRotator);if(!U)return nullptr;
    FFrontlineSquad S;S.Id=Id;S.Role=Role;S.Team=0;S.bHoldPosition=true;S.Members.Add(U);B->Squads.Add(S);
    U->Initialize(B,Id,0,Role,0);U->SetActorTickEnabled(false);B->Units.Add(U);return U;
}
}
void Reset(AWarfareFrontlineGameMode* B)
{for(auto I=States.CreateIterator();I;++I)if(!I.Key().IsValid()||I.Key()->Battle==B)
    {if(auto* U=I.Key().Get())RestoreMineEquipmentV835(U,I.Value());I.RemoveCurrent();}ReviewUnits.Reset();}
bool IsAction(const UAnimationAsset* Asset)
{if(!Asset)return false;const FString N=Asset->GetName();return N.StartsWith(TEXT("A_Mask"))||N.StartsWith(TEXT("A_WireCut"))||N.StartsWith(TEXT("A_Gas"));}
bool IsBusy(const AWarfareFrontlineSoldier* U){const auto* S=States.Find(const_cast<AWarfareFrontlineSoldier*>(U));return S&&(S->Action!=EAction::None||S->CompletedCut);}
bool HasIssuedMask(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!FrontlineGameplayV51::MaskEligibleRole(U->Role))return false;
    // TacticalV2 has no national respirator-issue policy. Reserved descriptor
    // indices are not dates, and its smoke option is not Classic technology 16.
    if(WarfareFRRUCampaignV82::Active(U->Battle))return false;
    if(U->Team==0)return U->Battle->HasTechnology(16,0);
    const auto* M=U->Battle->GetEncounterCampaignMission();
    return M&&M->Id>=U->Battle->SupportUnlockMission(EFrontlineSupport::Gas)+3;
}
bool MaskSealed(const AWarfareFrontlineSoldier* U){const auto* S=States.Find(const_cast<AWarfareFrontlineSoldier*>(U));return HasIssuedMask(U)&&S&&S->Sealed;}
bool MaskPresentation(const AWarfareFrontlineSoldier* U,FVector& Offset)
{
    Offset=FVector::ZeroVector;const auto* S=States.Find(const_cast<AWarfareFrontlineSoldier*>(U));if(!S)return false;
    if(S->Action==EAction::Don||S->Action==EAction::Doff)
    {
        const float T=S->Action==EAction::Don?S->Time/S->Duration:1-S->Time/S->Duration;
        const float Raise=Ease((T-.18f)/.19f);
        Offset=U->Soldier->GetComponentTransform().InverseTransformVectorNoScale(U->GetActorForwardVector()*8.f-FVector::UpVector*37.f)*(1-Raise);
        return T>.18f;
    }
    return S->Sealed;
}
void Abort(AWarfareFrontlineSoldier* U)
{if(auto* S=States.Find(U))
{
    RestoreMineEquipmentV835(U,*S);S->Action=EAction::None;S->CompletedCut=false;S->Time=0;S->SuspendedWeaponV851.Valid=false;HideTools(*S);
    // Death or the battle outro owns the body from this point onward. Keep the
    // committed-attempt flag, but never leave a live escape budget on a corpse.
    if(!U->IsAlive()||!U->Battle||U->Battle->Phase!=EFrontlinePhase::Fighting)S->EscapeRemainingV8216=0.f;
}}
TSharedRef<FJsonObject> MineWorkReviewStateV835(const AWarfareFrontlineSoldier* U)
{
    auto J=MakeShared<FJsonObject>();const auto* S=States.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    J->SetBoolField(TEXT("working"),S&&S->Action==EAction::MineWork);
    J->SetBoolField(TEXT("rising"),S&&S->Action==EAction::MineRise);
    J->SetNumberField(TEXT("work_starts"),S?S->MineStartsV835:0);
    J->SetNumberField(TEXT("working_frames"),S?S->MineWorkingFramesV835:0);
    J->SetNumberField(TEXT("work_elapsed"),S?S->Time:0);
    const auto* Probe=S?S->MineProbeV835.Get():nullptr;
    J->SetBoolField(TEXT("probe_visible"),Probe&&Probe->IsVisible());
    J->SetBoolField(TEXT("rifle_hidden"),U&&U->Soldier&&U->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle")));
    if(Probe&&S&&U&&U->Soldier)
    {
        const FVector A=Probe->GetComponentTransform().TransformPosition(FVector(0,0,-50));
        const FVector Z=Probe->GetComponentTransform().TransformPosition(FVector(0,0,50));
        J->SetNumberField(TEXT("probe_ground_gap_cm"),FVector::Dist(A,S->MineContactV835));
        J->SetNumberField(TEXT("probe_grip_gap_cm"),FVector::Dist(Z,U->Soldier->GetBoneLocation(TEXT("hand_r"))));
        J->SetNumberField(TEXT("probe_ground_height_gap_cm"),FMath::Abs(float(A.Z)-FrontlineInfantryV45::SurfaceHeight(U->Battle.Get(),A)));
    }
    if(U&&U->Soldier)
    {J->SetNumberField(TEXT("pelvis_above_feet_cm"),U->Soldier->GetBoneLocation(TEXT("pelvis")).Z-U->Feet().Z);}
    return J;
}
FGasExposureV8215 GasExposureV8215(const AWarfareFrontlineSoldier* U)
{
    FGasExposureV8215 Result;const auto* S=States.Find(const_cast<AWarfareFrontlineSoldier*>(U));if(!S)return Result;
    Result.Dose=S->Dose;Result.Threshold=S->Threshold;Result.Concentration=S->Concentration;
    Result.Rate=S->DoseRate;Result.SourceTeam=S->GasSource;Result.Sealed=MaskSealed(U);
    Result.Escaping=U&&U->IsAlive()&&S->EscapeRemainingV8216>0;
    Result.Distress=U&&U->IsAlive()?Ease((S->Dose/FMath::Max(1.f,S->Threshold)-.18f)/.62f):0.f;
    return Result;
}
void TickExposure(AWarfareFrontlineSoldier* U,float DT)
{
    if(!U||!U->IsActiveCombatant()||!U->Battle||!U->Battle->CanResolveCommittedSupportV8217()||!FMath::IsFinite(DT)||DT<=0)return;auto& S=State(U);int32 Source;bool Warning;
    const float C=Concentration(U,Source,Warning);
    S.Concentration=C;S.GasSource=Source;S.DoseRate=0;
    S.ClearFor=Warning?0:S.ClearFor+DT;
    if(Warning&&!S.Warned){Voice(U,EFrontlineVoiceEvent::GasAlarm);S.Warned=true;}
    if(S.ClearFor>14.f)S.Warned=false;
    // Overlapping puff/round emitters describe one concentration, not stacked
    // random damage rolls. Low ground retains more gas; a sealed mask protects.
    if(C>0)
    {
        const bool Sealed=MaskSealed(U);
        // The sealed respirator keeps its accepted dose rate; bare lungs take
        // a faster dose without stacking the overlapping canister emitters.
        S.DoseRate=(Sealed?2.10f:3.00f)*C*(U->Battle->CoverFor(U)>0?1.20f:1.f)*(Sealed?FrontlineGameplayV51::SealedGasDoseScale:1.f);
        S.Dose=FMath::Min(S.Threshold,S.Dose+DT*S.DoseRate);
        U->Suppression=FMath::Min(1.f,U->Suppression+DT*C*(MaskSealed(U)?.035f:.085f));
        if(!S.DistressHeard&&S.Dose>=S.Threshold*.30f&&U->Battle->CommittedSupportTimeV8217()>=S.DistressCheckAt)
        {
            S.DistressCheckAt=U->Battle->CommittedSupportTimeV8217()+2.f;
            if(auto* A=U->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
                S.DistressHeard=A->PlayGasDistressV8215(U->GetFaction()==EFrontlineFaction::German,U->GetActorLocation(),U->SquadId*16+U->MemberIndex,U->SquadId);
        }
        if(S.Dose>=S.Threshold)U->ReceiveHit(1,.45f,Source,FVector::ZeroVector,EFrontlineDamageKind::Gas,.05f,FFrontlineKillSourceV65::FireSupport(Source,int32(EFrontlineSupport::Gas)));
    }
    else {S.Dose=FMath::Max(0.f,S.Dose-DT*.30f);if(S.Dose<=.01f)S.DistressHeard=false;}
}
bool PrepareGasEscapeTimeBudgetReviewV8219(AWarfareFrontlineSoldier* U)
{
    if(!U||!FrontlineEffectsReviewSafetyV8216::Allowed(U->Battle.Get())
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"))||!Eligible(U))return false;
    auto& S=State(U);S.EscapeAttemptedV8216=true;S.EscapeRemainingV8216=1.2f;
    S.EscapeDirectionV8216=FVector(0,1,0);return true;
}
bool TickGasEscapeV8216(AWarfareFrontlineSoldier* U,float DT)
{
    if(!Eligible(U)||!FMath::IsFinite(DT)||DT<=0)return false;
    auto* S=States.Find(U);if(!S)return false;
    if(HasIssuedMask(U)&&!S->Sealed)
    {
        int32 Source;bool Warning;Concentration(U,Source,Warning);
        // An issued mask must get its first response to a cloud which appeared
        // around the man. The panic escape path used to pre-empt that action
        // forever once an interrupted reload had allowed the dose to build.
        if(Warning||S->Action==EAction::Don){S->EscapeRemainingV8216=0;return false;}
    }
    static const bool ReviewV836=FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"));
    // Bare lungs need enough remaining time to lift an already planted gun
    // before the fatal dose. A sealed mask retains its accepted trigger.
    const float EscapeDoseFractionV836=MaskSealed(U)?.76f:.65f;
    if(!S->EscapeAttemptedV8216&&S->Concentration>.25f&&S->Dose>S->Threshold*EscapeDoseFractionV836
        &&S->Action==EAction::None&&U->WeaponPhase==EFrontlineWeaponPhase::Ready)
    {
        // A finite staggered rush precedes collapse. The committed escape can
        // pack a supported gun once instead of leaving its crew pinned prone.
        S->EscapeAttemptedV8216=true;
        if(ReviewV836)++S->EscapeStartsV836;
        const uint32 Key=uint32(U->SquadId)*73856093u^uint32(U->MemberIndex+1)*19349663u;
        FVector Away=FVector::ZeroVector;
        for(const auto& Z:U->Battle->SmokeZones)if(Z.bGas&&U->Battle->Elapsed>=Z.StartedAt&&U->Battle->Elapsed<Z.ExpiresAt)
        {
            const FVector Delta=(U->Feet()-Z.Centre).GetSafeNormal2D();
            if(FVector::Dist2D(U->Feet(),Z.Centre)<Z.Radius)Away+=Delta;
        }
        if(Away.IsNearlyZero())Away=-U->GetActorForwardVector()+U->GetActorRightVector()*(Key&1u?.7f:-.7f);
        S->EscapeDirectionV8216=Away.GetSafeNormal2D();
        S->EscapeRemainingV8216=.65f+float(Key%51u)*.01f;
    }
    if(S->EscapeRemainingV8216<=0)return false;
    if(PackForMask(U,DT)){if(ReviewV836)++S->EscapePackFramesV836;return true;}
    const FVector Before=U->Feet();const float EscapeTime=FMath::Min(DT,S->EscapeRemainingV8216);
    S->EscapeRemainingV8216=FMath::Max(0.f,S->EscapeRemainingV8216-EscapeTime);
    const int32 Substeps=FMath::Max(1,FMath::CeilToInt(EscapeTime/.05f));
    const float Step=EscapeTime/Substeps;
    for(int32 I=0;I<Substeps;++I)
    {
        const FVector StepBefore=U->Feet();
        const FVector Goal=StepBefore+S->EscapeDirectionV8216*370.f;
        const FVector Next=FrontlineMobilityV41::Step(U,Goal,370.f*Step,false,Step);
        FFrontlineMotionV40Access::PlaceSupported(U,Next,Step);
        if(FVector::DistSquared2D(StepBefore,U->Feet())<.01f)
        {S->EscapeRemainingV8216=0;break;}
    }
    FFrontlineMotionV40Access::Escape(U,Before,DT);
    // A blocked man braces once. Keeping a run clip at zero velocity was the
    // original visual defect; never force movement through a trench or ally.
    if(FVector::DistSquared2D(Before,U->Feet())<.01f)S->EscapeRemainingV8216=0;
    if(ReviewV836)
    {
        ++S->EscapeFramesV836;S->EscapeTravelV836+=FVector::Dist2D(Before,U->Feet());
        if(auto* Node=U->Soldier->GetSingleNodeInstance();U->bMoving&&Node&&Node->GetCurrentAsset()==U->GetRunMotion())++S->EscapeRunFramesV836;
        if(FVector::DistSquared2D(Before,U->Feet())<.01f)++S->EscapeBlockedFramesV836;
        S->EscapeBelowSurfaceV836=FMath::Max(S->EscapeBelowSurfaceV836,
            float(FrontlineInfantryV45::SurfaceHeight(U->Battle,U->Feet())-U->Feet().Z));
    }
    return true;
}
TSharedRef<FJsonObject> GasLocomotionReviewStateV836(const AWarfareFrontlineSoldier* U)
{
    auto J=MakeShared<FJsonObject>();
    const bool Allowed=U&&Isolated(U->Battle.Get())&&FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"));
    J->SetBoolField(TEXT("allowed"),Allowed);if(!Allowed)return J;
    const auto* S=States.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    J->SetBoolField(TEXT("has_exposure_state"),S!=nullptr);if(!S)return J;
    J->SetNumberField(TEXT("escape_starts"),S->EscapeStartsV836);
    J->SetNumberField(TEXT("escape_frames"),S->EscapeFramesV836);
    J->SetNumberField(TEXT("pack_frames"),S->EscapePackFramesV836);
    J->SetNumberField(TEXT("run_frames"),S->EscapeRunFramesV836);
    J->SetNumberField(TEXT("blocked_frames"),S->EscapeBlockedFramesV836);
    J->SetNumberField(TEXT("actual_travel_cm"),S->EscapeTravelV836);
    J->SetNumberField(TEXT("max_feet_below_surface_cm"),S->EscapeBelowSurfaceV836);
    J->SetNumberField(TEXT("completed_don_actions"),S->DonCompleted);
    J->SetBoolField(TEXT("mask_sealed"),MaskSealed(U));
    return J;
}
bool TickAction(AWarfareFrontlineSoldier* U,float DT)
{
    if(!Eligible(U)){if(U)Abort(U);return false;}auto& S=State(U);int32 Source;bool Warning;Concentration(U,Source,Warning);
    FFrontlineMineWorkStateV835 Mine;
    const bool HasMine=!WarfareFRRUCampaignV82::Active(U->Battle)&&FrontlineFieldV32::QueryMineWorkV835(U,Mine);
    const bool MineWorking=HasMine&&Mine.Phase==EFrontlineMineWorkPhaseV835::Working;
    const bool MaskPriority=HasIssuedMask(U)&&Warning&&!S.Sealed;
    if(MaskPriority&&S.Action==EAction::None)
        FFrontlineMotionV40Access::SuspendWeaponV851(U,S.SuspendedWeaponV851);
    if(S.Action==EAction::MineWork&&(!MineWorking||MaskPriority||!S.MineCentreV835.Equals(Mine.MineCentre,.1f)))
    {
        RestoreMineEquipmentV835(U,S);
        // A cleared charge permits the authored final rise. An interrupted job
        // (threat, loading, hull yielding or a changed order) returns immediately
        // to the ordinary movement/weapon path instead of holding that danger.
        const auto* Section=U->Battle->FindSquad(U->SquadId);
        const bool SafeFinish=!HasMine&&!MaskPriority&&Section&&Section->bAdvancing&&!Section->bHoldPosition
            &&U->WeaponPhase==EFrontlineWeaponPhase::Ready
            &&!MineDangerV835(U);
        S.Action=SafeFinish?EAction::MineRise:EAction::None;S.Time=0;
        if(!SafeFinish)FFrontlineMotionV40Access::Motion(U,U->GetAimMotion());
    }
    if(S.Action==EAction::MineRise)
    {
        const auto* Section=U->Battle->FindSquad(U->SquadId);
        FVector Yield;
        if(MaskPriority||U->WeaponPhase!=EFrontlineWeaponPhase::Ready||!Section||!Section->bAdvancing||Section->bHoldPosition
            ||MineDangerV835(U)
            ||FrontlineMobilityV41::ArmourYieldDestinationV8218(U,Yield))
        {S.Action=EAction::None;FFrontlineMotionV40Access::Motion(U,U->GetAimMotion());}
        else
        {
            S.Time+=DT;const float RiseSeconds=.8f;
            FFrontlineMotionV40Access::Pose(U,S.Clip.Get(),S.Duration*(.80f+.20f*FMath::Clamp(S.Time/RiseSeconds,0.f,1.f)));
            if(S.Time<RiseSeconds)return true;
            S.Action=EAction::None;FFrontlineMotionV40Access::Motion(U,U->GetIdleMotion());
        }
    }
    if(MineWorking&&!MaskPriority&&(S.Action==EAction::None||S.Action==EAction::MineWork))
    {
        if(S.Action!=EAction::MineWork)
        {
            auto* Clip=Load(TEXT("A_WireCut_V40"));if(!Begin(U,S,EAction::MineWork,Clip))return false;
            S.MineCentreV835=Mine.MineCentre;S.MineStartedAtV835=Mine.HostElapsed;++S.MineStartsV835;
            S.MineWeaponWasHiddenV835=U->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle"));S.MineWeaponSavedV835=true;
            U->Soldier->HideBoneByName(TEXT("weapon_rifle"),EPhysBodyOp::PBO_None);
        }
        S.Time=FMath::Max(0.f,Mine.HostElapsed-S.MineStartedAtV835);
        // Reuse one authored descent, then sample only its fully crouched work
        // interval. Personal wrist motion stays inside that same settled pose;
        // the full descent/rise sequence never loops while the mine is live.
        const float Settled=.8f;
        const float WorkSample=S.Duration*(.38f+.06f*(.5f+.5f*FMath::Sin(S.Time*1.15f+U->GetMotionPhase()*2.f*PI)));
        FFrontlineMotionV40Access::Pose(U,S.Clip.Get(),S.Time<Settled?S.Duration*.30f*S.Time/Settled:WorkSample);
        U->SetActorRotation(FMath::RInterpTo(U->GetActorRotation(),Mine.Direction.Rotation(),DT,6.f));
        MineProbePoseV835(U,S,Mine.ToolContact,S.Time>=Settled*.8f);++S.MineWorkingFramesV835;
        return true;
    }
    if(S.CompletedCut)
    {
        if(MaskPriority){S.CompletedCut=false;S.CompleteHold=0;}
        else
        {
        S.CompleteHold+=DT;
        // The field's quarter-second hazard pass consumes the real contact
        // result. Do not walk out of range between completion and that pass.
        // A shell/other pioneer may already have removed it: expire that orphan.
        if(S.CompleteHold<=.55f){FFrontlineMotionV40Access::Pose(U,U->GetIdleMotion());return true;}
        S.CompletedCut=false;S.CompleteHold=0;
        }
    }
    const bool Trained=HasIssuedMask(U);
    if(S.Action==EAction::ApproachWire||S.Action==EAction::Cut)
    {
        if((Warning&&Trained&&!S.Sealed)||U->WeaponPhase!=EFrontlineWeaponPhase::Ready){Abort(U);U->ReturnToIdle();}
    }
    if(S.Action==EAction::None)
    {
        if(U->WeaponPhase!=EFrontlineWeaponPhase::Ready)return false;
        FVector Flame;if(!MaskPriority&&FrontlineFieldV32::FlameBraceDirectionV33(U,Flame))return false;
        if(Trained&&Warning&&!S.Sealed)
        {if(PackForMask(U,DT))return true;if(!Begin(U,S,EAction::Don,MaskClip(U,true)))
            {FFrontlineMotionV40Access::RestoreWeaponV851(U,S.SuspendedWeaponV851);return false;}}
        else return false;
    }
    if(S.Action==EAction::Doff&&Warning)
    {
        // A returning cloud reverses the same trajectory. No mask teleports and
        // no second free seal: an opened mask must finish the full don phase.
        const float Reverse=S.Duration-S.Time;Begin(U,S,EAction::Don,MaskClip(U,true));S.Time=Reverse;
    }
    if(S.Action==EAction::ApproachWire)
    {
        const FVector F=(S.Wire-U->Feet()).GetSafeNormal2D();const FVector Goal=S.Wire-F*67.f;
        FVector D=Goal-U->Feet();D.Z=0;const float Dist=D.Size2D();
        if(FVector::Dist2D(U->Feet(),S.Wire)>190.f){Abort(U);U->ReturnToIdle();return false;}
        if(Dist>1.f)
        {
            const FVector Before=U->Feet();
            const FVector Next=FrontlineMobilityV41::Step(U,Goal,FMath::Min(Dist,95.f*DT),false,DT);
            FFrontlineMotionV40Access::PlaceSupported(U,Next,DT);
            FFrontlineMotionV40Access::Approach(U,Before,DT);return true;
        }
        if(!F.IsNearlyZero())U->SetActorRotation(F.Rotation());
        Begin(U,S,EAction::Cut,Load(TEXT("A_WireCut_V40")));
    }
    if(S.Action==EAction::Cut&&FVector::Dist2D(U->Feet()+U->GetActorForwardVector()*67.f,S.Wire)>8.f)
    {Abort(U);U->ReturnToIdle();return false;}
    S.Time+=DT;
    if(S.Action==EAction::Doff&&S.Time/S.Duration>.18f)S.Sealed=false;
    FFrontlineMotionV40Access::Pose(U,S.Clip.Get(),FMath::Min(S.Time,S.Duration));
    if(S.Action==EAction::Cut)ToolPose(U,S);
    if(S.Time>=S.Duration)
    {
        const EAction Done=S.Action;S.Action=EAction::None;HideTools(S);
        if(Done==EAction::Don){S.Sealed=true;++S.DonCompleted;Voice(U,EFrontlineVoiceEvent::MaskSealed);}
        else if(Done==EAction::Doff)S.Sealed=false;
        else if(Done==EAction::Cut){S.CompletedCut=true;S.CompleteHold=0;++S.CutsCompleted;Voice(U,EFrontlineVoiceEvent::WireCut);}
        U->ReturnToIdle();
        if(Done==EAction::Don)FFrontlineMotionV40Access::RestoreWeaponV851(U,S.SuspendedWeaponV851);
    }
    return true;
}
bool RequestWireCut(AWarfareFrontlineSoldier* U,const FVector& Wire)
{
    if(!Eligible(U)||U->Role!=EFrontlineRole::Pioneer||U->WeaponPhase!=EFrontlineWeaponPhase::Ready||FVector::Dist2D(U->Feet(),Wire)>270.f)return false;
    // One cutter can work under distant covering fire instead of every engineer
    // remaining stuck in a rifle duel beside the wire. Immediate threats win.
    if(U->Battle)for(const AWarfareFrontlineSoldier* Enemy:U->Battle->Units)
        if(Enemy&&Enemy->Team!=U->Team&&Enemy->IsActiveCombatant()&&FVector::DistSquared2D(Enemy->Feet(),U->Feet())<FMath::Square(650.f))return false;
    auto& S=State(U);if(S.CompletedCut)return false;if(S.Action!=EAction::None)return FVector::Dist2D(S.Wire,Wire)<1.f;
    if(!Load(TEXT("A_WireCut_V40")))return false;S.Wire=Wire;S.Action=EAction::ApproachWire;return true;
}
bool ConsumeWireCut(AWarfareFrontlineSoldier* U,const FVector& Wire)
{
    auto* S=States.Find(U);if(!Eligible(U)||!S||!S->CompletedCut||FVector::Dist2D(S->Wire,Wire)>1.f||FVector::Dist2D(U->Feet(),Wire)>90.f)return false;
    S->CompletedCut=false;return true;
}
UAnimationAsset* GasDeathMotion(AWarfareFrontlineSoldier* U,bool Prone)
{
    if(U&&U->Battle&&!WarfareFRRUCampaignV82::Active(U->Battle))
    {
        const TCHAR* Name=Prone?TEXT("A_GasProneCollapse_V836"):TEXT("A_GasCollapse_V836");
        if(auto* Refined=LoadObject<UAnimationAsset>(nullptr,*FString::Printf(
            TEXT("/Game/Warfare/Characters/V836/Animations/%s.%s"),Name,Name)))return Refined;
    }
    return Load(Prone?TEXT("A_GasProneCollapse_V40"):TEXT("A_GasCollapse_V40"));
}

void RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!Isolated(B)){Check(TEXT("v40_isolated_field_action_review"),false);return;}
    auto SavedUnits=B->Units;auto SavedSquads=B->Squads;auto SavedZones=B->SmokeZones;auto SavedResearch=FFrontlineMotionV40Access::Research(B);const auto Phase=B->Phase;const float Time=B->Elapsed;
    const int32 OldCasualties[2]={B->Casualties[0],B->Casualties[1]},OldOutro=B->BattleOutroWinner;const float OldMorale[2]={B->Morale[0],B->Morale[1]},OldDamage[2]={B->DamageDealt[0],B->DamageDealt[1]},OldFriendly=B->FriendlyFireDamage,OldPeak=B->PeakSuppression,OldOutroTime=B->BattleOutroElapsed;const FString OldMessage=B->LastMessage,OldReason=B->EndReason;
    TArray<AWarfareFrontlineSoldier*> Made;
    ON_SCOPE_EXIT{for(auto* U:Made){States.Remove(U);U->Destroy();}B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedZones;FFrontlineMotionV40Access::Research(B)=SavedResearch;B->Phase=Phase;B->Elapsed=Time;for(int32 I=0;I<2;++I){B->Casualties[I]=OldCasualties[I];B->Morale[I]=OldMorale[I];B->DamageDealt[I]=OldDamage[I];}B->FriendlyFireDamage=OldFriendly;B->PeakSuppression=OldPeak;B->BattleOutroWinner=OldOutro;B->BattleOutroElapsed=OldOutroTime;B->LastMessage=OldMessage;B->EndReason=OldReason;FrontlineMoraleV36::Reset(B);};
    B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Phase=EFrontlinePhase::Fighting;
    auto* Bare=Make(B,401001,EFrontlineRole::Rifle,FVector(1700,-500,0));auto* Masked=Make(B,401002,EFrontlineRole::Mills,FVector(1700,0,0));auto* Cutter=Make(B,401003,EFrontlineRole::Pioneer,FVector(1700,500,0));
    for(auto* U:{Bare,Masked,Cutter})if(U)Made.Add(U);if(Made.Num()!=3){Check(TEXT("v40_actual_fixture_actors"),false);return;}
    FFrontlineSmokeZone Z;Z.bGas=true;Z.Team=1;Z.Centre=Masked->Feet();Z.Radius=1200;Z.StartedAt=Time;Z.ExpiresAt=Time+100;B->SmokeZones.Add(Z);
    // The very same state functions used by soldier Tick are advanced here.
    auto& Research=FFrontlineMotionV40Access::Research(B);Research.TechnologyMask&=~(1u<<16);
    const int32 BeforeMission=FFrontlineMotionV40Access::Mission(B,0);Masked->Team=1;const bool EarlyUnissued=!HasIssuedMask(Masked);
    FFrontlineMotionV40Access::Mission(B,B->SupportUnlockMission(EFrontlineSupport::Gas)+3);const bool LaterIssued=HasIssuedMask(Masked);
    FFrontlineMotionV40Access::Mission(B,BeforeMission);Masked->Team=0;
    Check(TEXT("v40_enemy_mask_issue_follows_campaign_not_player_research"),EarlyUnissued&&LaterIssued&&!HasIssuedMask(Masked));
    float BareSecondsV836=0,BareRateV836=0;
    for(int32 I=0;I<320&&Bare->IsAlive();++I)
    {TickAction(Bare,.1f);TickExposure(Bare,.1f);BareSecondsV836+=.1f;if(I==0)BareRateV836=State(Bare).DoseRate;}
    const bool UnmaskedFatal=!Bare->IsAlive()&&Bare->GetDeathState().Kind==EFrontlineDamageKind::Gas&&Bare->GetDeathState().bAuthoredFall;
    const float ExpectedBareSecondsV836=State(Bare).Threshold/FMath::Max(BareRateV836,.001f);
    const float LegacyBareSecondsV836=ExpectedBareSecondsV836*(3.00f/2.10f);
    Check(FString::Printf(TEXT("v836_actual_unmasked_fatal_dose_%.2fs_before_legacy_%.2fs"),BareSecondsV836,LegacyBareSecondsV836),
        UnmaskedFatal&&BareRateV836>0&&BareSecondsV836>=ExpectedBareSecondsV836-.001f
        &&BareSecondsV836<=ExpectedBareSecondsV836+.101f&&BareSecondsV836<LegacyBareSecondsV836-.2f);
    Research.TechnologyMask|=1u<<16;
    TickAction(Masked,.01f);TickExposure(Masked,.01f);
    Check(TEXT("v40_researched_respirator_does_not_seal_instantly"),IsBusy(Masked)&&!MaskSealed(Masked)&&State(Masked).Dose>0);
    for(int32 I=0;I<32;++I){TickAction(Masked,.1f);TickExposure(Masked,.1f);}
    Check(TEXT("v40_actual_don_clip_completes_before_protection"),MaskSealed(Masked)&&State(Masked).DonCompleted==1&&Masked->IsAlive());
    for(int32 I=0;I<80;++I){TickAction(Masked,.1f);TickExposure(Masked,.1f);}
    Check(TEXT("v8215_short_exposure_kills_bare_man_but_sealed_mask_delays_fatal_dose"),UnmaskedFatal&&Masked->IsAlive()&&GasExposureV8215(Masked).Dose>0);
    const float BeforeOverlap=State(Masked).Dose,BeforeRate=State(Masked).DoseRate;
    Check(TEXT("v836_sealed_respirator_retains_accepted_exposure_rate"),FMath::IsNearlyEqual(BeforeRate,
        2.10f*State(Masked).Concentration*(B->CoverFor(Masked)>0?1.20f:1.f)*FrontlineGameplayV51::SealedGasDoseScale,.00001f));
    const float SavedSealedDoseV836=State(Masked).Dose;
    State(Masked).Dose=State(Masked).Threshold*.76f;
    Check(TEXT("v836_sealed_escape_gate_remains_strictly_above_seventy_six_percent"),
        MaskSealed(Masked)&&!TickGasEscapeV8216(Masked,.001f)&&!State(Masked).EscapeAttemptedV8216);
    State(Masked).Dose=SavedSealedDoseV836;
    B->SmokeZones.Add(Z);TickExposure(Masked,.1f);
    Check(TEXT("v8215_overlapping_canisters_do_not_stack_gas_dose"),Masked->IsAlive()&&FMath::IsNearlyEqual(State(Masked).Dose-BeforeOverlap,BeforeRate*.1f,.002f));
    B->SmokeZones.RemoveAt(B->SmokeZones.Num()-1);
    const int32 Before=B->Casualties[0];TickExposure(Bare,100);Check(TEXT("v40_dead_man_does_not_reenter_exposure_or_die_twice"),!Bare->IsAlive()&&B->Casualties[0]==Before);
    B->SmokeZones.Reset();for(int32 I=0;I<73;++I)TickExposure(Masked,.1f);TickAction(Masked,.1f);
    Check(TEXT("v836_mask_remains_sealed_after_seven_seconds_of_clear_air"),!IsBusy(Masked)&&MaskSealed(Masked));for(int32 I=0;I<33;++I)TickAction(Masked,.1f);
    FVector RetainedMaskOffset;
    Check(TEXT("v836_clear_air_never_repeats_don_or_starts_doff"),!IsBusy(Masked)&&MaskSealed(Masked)
        &&State(Masked).DonCompleted==1&&MaskPresentation(Masked,RetainedMaskOffset)&&RetainedMaskOffset.IsNearlyZero());
    B->SmokeZones.Add(Z);
    for(int32 I=0;I<400&&Masked->IsAlive();++I)TickExposure(Masked,.1f);
    const auto* GasNode=Masked->Soldier->GetSingleNodeInstance();
    Check(TEXT("v8215_sustained_dense_gas_kills_masked_man_with_full_nonlooping_fall"),
        !Masked->IsAlive()&&Masked->GetDeathState().Kind==EFrontlineDamageKind::Gas&&Masked->GetDeathState().bAuthoredFall
        &&GasNode&&!GasNode->IsLooping()&&GasNode->GetPlayRate()>=1.10f&&GasNode->GetPlayRate()<=1.28f);
    B->SmokeZones.Reset();
    // This is the bare-lungs escape case. The preceding mask test researched
    // its equipment; leaving that bit set would correctly start a don action
    // after the escape and invalidate this fixture's no-action postcondition.
    Research.TechnologyMask&=~(1u<<16);
    auto* Escapee=Make(B,401004,EFrontlineRole::Rifle,FVector(2500,750,0));if(Escapee)Made.Add(Escapee);
    if(Escapee)
    {
        auto& EscapeState=State(Escapee);EscapeState.Dose=EscapeState.Threshold*.6501f;
        Z.Centre=Escapee->Feet();B->SmokeZones.Add(Z);TickExposure(Escapee,.001f);
        const bool ActualTickV836=FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"));
        const auto AdvanceEscape=[&]()
        {
            if(!ActualTickV836)return TickGasEscapeV8216(Escapee,.05f);
            // Use the ordinary soldier early gate, including real field work,
            // packing, swept travel and the selected evaluated locomotion.
            Escapee->Tick(.05f);
            Escapee->Soldier->HandleExistingParallelEvaluationTask(true,true);
            Escapee->Soldier->TickAnimation(.05f,false);Escapee->Soldier->RefreshBoneTransforms();
            return EscapeState.EscapeAttemptedV8216;
        };
        const FVector BeforeEscape=Escapee->Feet();const bool Began=AdvanceEscape();
        const bool Advanced=FVector::DistSquared2D(BeforeEscape,Escapee->Feet())>.01f;
        auto* EscapeNode=Escapee->Soldier->GetSingleNodeInstance();
        Check(TEXT("v8216_gas_escape_uses_measured_travel_full_locomotion_and_never_runs_in_place"),Began&&EscapeState.EscapeAttemptedV8216
            &&EscapeNode&&(Advanced?EscapeNode->GetCurrentAsset()==Escapee->GetRunMotion():EscapeState.EscapeRemainingV8216==0));
        for(int32 I=0;I<30;++I)AdvanceEscape();
        Check(TEXT("v8216_gas_escape_is_finite_and_does_not_restart_or_toggle_stance"),EscapeState.EscapeRemainingV8216==0
            &&!TickGasEscapeV8216(Escapee,.05f)&&EscapeState.Action==EAction::None);
        if(ActualTickV836)
        {
            const auto Actual=GasLocomotionReviewStateV836(Escapee);
            Check(FString::Printf(TEXT("v836_gas_escape_actual_metrics_alive_%d_starts_%d_frames_%d_run_%d_travel_%.2f_remaining_%.3f_action_%d"),
                Escapee->IsAlive(),EscapeState.EscapeStartsV836,EscapeState.EscapeFramesV836,EscapeState.EscapeRunFramesV836,
                EscapeState.EscapeTravelV836,EscapeState.EscapeRemainingV8216,int32(EscapeState.Action)),true);
            Check(TEXT("v836_actual_soldier_tick_commits_one_finite_gas_escape_with_real_run_travel"),
                Actual->GetBoolField(TEXT("allowed"))&&Actual->GetBoolField(TEXT("has_exposure_state"))
                &&Actual->GetNumberField(TEXT("escape_starts"))==1&&Actual->GetNumberField(TEXT("escape_frames"))>0
                &&Actual->GetNumberField(TEXT("run_frames"))>0&&Actual->GetNumberField(TEXT("actual_travel_cm"))>1.f
                &&Actual->GetNumberField(TEXT("max_feet_below_surface_cm"))<=.1f&&EscapeState.EscapeRemainingV8216==0);
            UE_LOG(LogTemp,Display,TEXT("WF_V836_ACTUAL_GAS starts=%d frames=%d run=%d blocked=%d travel_cm=%.3f below_surface_cm=%.3f"),
                EscapeState.EscapeStartsV836,EscapeState.EscapeFramesV836,EscapeState.EscapeRunFramesV836,
                EscapeState.EscapeBlockedFramesV836,EscapeState.EscapeTravelV836,EscapeState.EscapeBelowSurfaceV836);
        }
        B->SmokeZones.Reset();
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))
    {
        // A late, lethal exposure must end the already committed body action,
        // too. Use the ordinary actor path rather than editing its budget.
        auto* FatalEscapeV836=Make(B,401006,EFrontlineRole::Rifle,FVector(2500,-750,0));
        if(FatalEscapeV836)Made.Add(FatalEscapeV836);
        bool DeathCancelledEscapeV836=false;
        if(FatalEscapeV836)
        {
            auto& FatalStateV836=State(FatalEscapeV836);FatalStateV836.Dose=FatalStateV836.Threshold*.85f;
            Z.Centre=FatalEscapeV836->Feet()-FVector(0,100.f,0);B->SmokeZones.Add(Z);
            FatalEscapeV836->Tick(.05f);
            const bool CommittedLiveBudgetV836=FatalEscapeV836->IsAlive()&&FatalStateV836.EscapeStartsV836==1
                &&FatalStateV836.EscapeRemainingV8216>0.f&&FatalStateV836.EscapeTravelV836>1.f;
            for(int32 I=0;I<30&&FatalEscapeV836->IsAlive();++I)FatalEscapeV836->Tick(.05f);
            DeathCancelledEscapeV836=CommittedLiveBudgetV836&&!FatalEscapeV836->IsAlive()
                &&FatalEscapeV836->GetDeathState().Kind==EFrontlineDamageKind::Gas
                &&FatalStateV836.EscapeRemainingV8216==0.f&&FatalStateV836.EscapeStartsV836==1
                &&!TickGasEscapeV8216(FatalEscapeV836,.05f)&&FatalStateV836.Action==EAction::None;
            Check(FString::Printf(TEXT("v836_fatal_escape_actual_live_budget_%d_dead_%d_starts_%d_travel_%.1f_remaining_%.3f"),
                CommittedLiveBudgetV836,!FatalEscapeV836->IsAlive(),FatalStateV836.EscapeStartsV836,
                FatalStateV836.EscapeTravelV836,FatalStateV836.EscapeRemainingV8216),true);
            B->SmokeZones.Reset();
        }
        Check(TEXT("v836_actual_fatal_gas_ends_committed_escape_budget_without_restart"),DeathCancelledEscapeV836);
        // Begin with a real deployed gun, then use ordinary soldier Tick to
        // finish one authored pack and perform collision-constrained travel.
        Research.TechnologyMask&=~(1u<<16);
        auto* GunnerV836=Make(B,401005,EFrontlineRole::Lewis,FVector(1700,-950,0));
        if(GunnerV836)Made.Add(GunnerV836);
        auto* GunV836=GunnerV836?FrontlineMachinegunsV30::Get(GunnerV836):nullptr;
        const int32 HomeV836=B->GetHomeTrenchIndex(0);
        const bool HeldBayV836=GunnerV836&&B->Trenches.IsValidIndex(HomeV836)&&B->Trenches[HomeV836].Owner==0;
        if(HeldBayV836)
        {
            // An unassigned squad's real destination is the far field edge,
            // even if bHoldPosition is set. Give this fixture an actual owned
            // firing bay; do not force the component into Supported state.
            auto* SectionV836=B->FindSquad(GunnerV836->SquadId);
            SectionV836->Trench=SectionV836->Destination=HomeV836;SectionV836->Slot=1;
            const FVector BayV836=FrontlineMachinegunsV30::TrenchFiringStepV36(GunnerV836,B->SoldierDestination(GunnerV836));
            GunnerV836->SetActorLocation(FVector(BayV836.X,BayV836.Y,FrontlineInfantryV45::SurfaceHeight(B,BayV836)+88.f));
            GunnerV836->bFieldEntered=true;GunnerV836->ReturnToIdle();
        }
        Check(TEXT("v836_bare_gas_machinegun_fixture_has_actual_owned_firing_bay"),HeldBayV836);
        if(GunV836&&HeldBayV836)for(int32 I=0;I<120&&!GunV836->IsSupported();++I)
        {
            GunnerV836->Tick(.05f);GunnerV836->Soldier->HandleExistingParallelEvaluationTask(true,true);
            GunnerV836->Soldier->TickAnimation(.05f,false);GunnerV836->Soldier->RefreshBoneTransforms();
            GunV836->TickComponent(.05f,LEVELTICK_All,nullptr);
        }
        const bool WasSupportedV836=GunV836&&GunV836->IsSupported()&&GunV836->Deployments==1;
        if(GunnerV836&&GunV836)Check(FString::Printf(TEXT("v836_gas_machinegun_setup_actual_state_%d_deploy_%d_pack_%d_moving_%d_destination_gap_%.1f"),
            int32(GunV836->State),GunV836->Deployments,GunV836->Packs,GunnerV836->bMoving,
            FVector::Dist2D(GunnerV836->Feet(),B->SoldierDestination(GunnerV836))),true);
        Check(TEXT("v836_bare_gas_escape_fixture_starts_with_actual_supported_machinegun"),WasSupportedV836);
        if(WasSupportedV836)
        {
            // A nearby canister gives a real lateral escape gradient along the
            // trench floor instead of asking the man to run through its wall.
            auto& GasStateV836=State(GunnerV836);Z.Centre=GunnerV836->Feet()-FVector(0,100.f,0);B->SmokeZones.Add(Z);
            TickExposure(GunnerV836,.001f);GasStateV836.Dose=GasStateV836.Threshold*.65f;
            Check(TEXT("v836_bare_escape_gate_is_strictly_above_sixty_five_percent"),
                !MaskSealed(GunnerV836)&&!TickGasEscapeV8216(GunnerV836,.001f)&&!GasStateV836.EscapeAttemptedV8216);
            GasStateV836.Dose=GasStateV836.Threshold*.6501f;
            bool AliveAtFirstStepV836=false;
            for(int32 I=0;I<80&&GunnerV836->IsAlive();++I)
            {
                GunnerV836->Tick(.05f);GunnerV836->Soldier->HandleExistingParallelEvaluationTask(true,true);
                GunnerV836->Soldier->TickAnimation(.05f,false);GunnerV836->Soldier->RefreshBoneTransforms();
                if(GasStateV836.EscapeTravelV836>1.f&&GunnerV836->IsAlive())AliveAtFirstStepV836=true;
                if(GasStateV836.EscapeAttemptedV8216&&GasStateV836.EscapeRemainingV8216<=0)break;
            }
            Check(FString::Printf(TEXT("v836_supported_machinegun_gas_pack_%d_frames_then_real_escape_%.1fcm"),
                GasStateV836.EscapePackFramesV836,GasStateV836.EscapeTravelV836),
                AliveAtFirstStepV836&&GasStateV836.EscapeStartsV836==1&&GunV836->Packs==1
                &&GasStateV836.EscapePackFramesV836>0&&GasStateV836.EscapeRunFramesV836>0
                &&GasStateV836.EscapeTravelV836>1.f&&GasStateV836.EscapeBelowSurfaceV836<=.1f
                &&GasStateV836.EscapeRemainingV8216==0);
            B->SmokeZones.Reset();
        }
    }
    const FVector Wire=Cutter->Feet()+FVector(67,0,0);Check(TEXT("v40_wire_rejects_distant_magic_cutting"),!RequestWireCut(Cutter,Wire+FVector(400,0,0))&&!ConsumeWireCut(Cutter,Wire));
    RequestWireCut(Cutter,Wire);TickAction(Cutter,.1f);Check(TEXT("v40_wire_no_progress_before_authored_tool_stroke"),IsBusy(Cutter)&&!ConsumeWireCut(Cutter,Wire));
    for(int32 I=0;I<32;++I)TickAction(Cutter,.1f);
    Check(TEXT("v40_wire_completed_stroke_consumed_once_at_real_contact"),ConsumeWireCut(Cutter,Wire)&&!ConsumeWireCut(Cutter,Wire));
    RequestWireCut(Cutter,Wire);TickAction(Cutter,.2f);Abort(Cutter);Check(TEXT("v40_interrupted_wire_action_grants_no_clearance"),!ConsumeWireCut(Cutter,Wire));
    RequestWireCut(Cutter,Wire);for(int32 I=0;I<32;++I)TickAction(Cutter,.1f);const bool Held=State(Cutter).CompletedCut&&IsBusy(Cutter);TickAction(Cutter,.60f);
    Check(TEXT("v40_orphaned_completed_wire_result_releases_operator_after_bounded_hold"),Held&&!IsBusy(Cutter)&&!ConsumeWireCut(Cutter,Wire));
    bool Clips=true;for(const TCHAR* Family:{TEXT("Rifle"),TEXT("MG"),TEXT("Officer"),TEXT("Flame")})for(const TCHAR* Action:{TEXT("Don"),TEXT("Doff")})Clips&=Load(FString::Printf(TEXT("A_Mask%s_%s_V40"),Action,Family))!=nullptr;
    Check(TEXT("v40_eight_equipment_specific_mask_clips_and_two_gas_falls_load"),Clips&&GasDeathMotion(Bare,false)&&GasDeathMotion(Bare,true));
}
void RunFocusedChecksV8215(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    RunFocusedChecks(B,Check);
    auto* Gas=FrontlineSmokeVolumeV35::GasMaterialV8215();
    Check(TEXT("v8215_gas_uses_actual_local_volume_derivative_with_fixed_cloud_budget"),
        Gas&&Gas->GetBaseMaterial()&&Gas->GetBaseMaterial()->GetName()==TEXT("M_GasVolume_V8215")&&FrontlineSmokeVolumeV35::CloudsPerMission==6);
}
void RunGasMaskChecksV851(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))||FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused")))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
    Check(TEXT("v851_mask_exact_disposable_ukde_production_world"),Safe);if(!Safe)return;
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedZones=B->SmokeZones;
    const auto SavedResearch=FFrontlineMotionV40Access::Research(B);const auto Phase=B->Phase;
    const float Time=B->Elapsed;const int32 Winner=B->BattleOutroWinner;const auto SavedStates=States;
    TArray<AWarfareFrontlineSoldier*> Made;
    ON_SCOPE_EXIT
    {
        for(auto* U:Made)if(IsValid(U))U->Destroy();States=SavedStates;B->Units=SavedUnits;B->Squads=SavedSquads;
        B->SmokeZones=SavedZones;B->Phase=Phase;B->Elapsed=Time;B->BattleOutroWinner=Winner;
        FFrontlineMotionV40Access::Research(B)=SavedResearch;
    };
    B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    FFrontlineMotionV40Access::Research(B).TechnologyMask|=1u<<16;
    int32 Id=851000,Trials=0;
    for(auto Role:{EFrontlineRole::Mills,EFrontlineRole::Pioneer,EFrontlineRole::Flamethrower})
        for(auto Weapon:{EFrontlineWeaponPhase::Ready,EFrontlineWeaponPhase::Reload,EFrontlineWeaponPhase::Recoil,EFrontlineWeaponPhase::BoltCycle,EFrontlineWeaponPhase::GrenadeThrow})
    {
        B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Elapsed=Time;
        auto* U=Make(B,++Id,Role,FVector(1700,0,0));if(!U){Check(TEXT("v851_mask_fixture_created"),false);continue;}Made.Add(U);
        FFrontlineMotionV40Access::SeedWeaponV851(U,Weapon);
        FFrontlineMotionV40Access::Motion(U,Weapon==EFrontlineWeaponPhase::Reload?U->GetReloadMotion():U->GetAimMotion());
        const float WeaponRemaining=FFrontlineMotionV40Access::RemainingWeaponV851(U);const int32 Rounds=U->RoundsRemaining;
        FFrontlineSmokeZone Z;Z.bGas=true;Z.Team=1;Z.Centre=U->Feet();Z.Radius=1200;Z.StartedAt=B->Elapsed;Z.ExpiresAt=B->Elapsed+100;B->SmokeZones.Add(Z);
        U->Tick(.05f);
        const bool Started=State(U).Action==EAction::Don&&!MaskSealed(U);
        for(int32 I=0;I<80&&U->IsAlive()&&!MaskSealed(U);++I)
        {B->Elapsed+=.05f;U->Tick(.05f);U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(.05f,false);U->Soldier->RefreshBoneTransforms();}
        const FString Name=FString::Printf(TEXT("v851_mask_cloud_appears_on_role%d_weapon%d"),int32(Role),int32(Weapon));
        Check(Name+TEXT("_first_tick_owns_actual_don_clip"),Started);
        Check(Name+TEXT("_seals_before_fatal_dose_with_no_free_ammunition"),U->IsAlive()&&MaskSealed(U)&&State(U).DonCompleted==1&&U->RoundsRemaining==Rounds);
        Check(Name+TEXT("_unfinished_weapon_cycle_resumes_without_instant_reload"),Weapon==EFrontlineWeaponPhase::Ready||
            (U->WeaponPhase==Weapon&&FMath::IsNearlyEqual(FFrontlineMotionV40Access::RemainingWeaponV851(U),WeaponRemaining,.001f)));
        Check(Name+TEXT("_panic_escape_does_not_preempt_mask"),!State(U).EscapeAttemptedV8216);++Trials;
    }
    B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();
    auto* Rifle=Make(B,++Id,EFrontlineRole::Rifle,FVector(1700,0,0));if(Rifle)Made.Add(Rifle);
    auto* MG=Make(B,++Id,EFrontlineRole::Lewis,FVector(1700,350,0));if(MG)Made.Add(MG);
    Check(TEXT("v851_mask_limited_issue_policy_preserved_for_ordinary_rifle_and_mg"),Rifle&&MG&&!HasIssuedMask(Rifle)&&!HasIssuedMask(MG));
    Check(TEXT("v851_mask_fifteen_actual_soldier_tick_interruptions_executed"),Trials==15);
}
void PrepareFieldActionReview(AWarfareFrontlineGameMode* B)
{
    if(!Isolated(B))return;FFrontlineMotionV40Access::Research(B).TechnologyMask|=1u<<16;
    for(AWarfareFrontlineSoldier* U:B->Units)if(U)U->Destroy();FrontlineMoraleV36::Reset(B);
    ReviewUnits.Reset();ReviewTime=0;ReviewWire.Reset();ReviewWireConsumed=false;B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Phase=EFrontlinePhase::Fighting;
    for(int32 I=0;I<4;++I)
    {
        auto* U=Make(B,402100+I,I==2?EFrontlineRole::Pioneer:EFrontlineRole::Rifle,FVector(-650+I*410.f,0,0));if(!U)continue;ReviewUnits.Add(U);
        auto& S=State(U);if(I==0)Begin(U,S,EAction::Don,MaskClip(U,true));else if(I==1){S.Sealed=true;Begin(U,S,EAction::Doff,MaskClip(U,false));}else if(I==2)
        {
            S.Wire=U->Feet()+FVector(67,0,0);Begin(U,S,EAction::Cut,Load(TEXT("A_WireCut_V40")));
            if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_FieldWire_Coil.SM_FieldWire_Coil")))
            {auto* C=NewObject<UStaticMeshComponent>(U);C->SetupAttachment(U->GetRootComponent());C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);U->AddInstanceComponent(C);C->RegisterComponent();C->SetWorldLocationAndRotation(S.Wire,FRotator(0,90,0));ReviewWire=C;}
        }
        else U->ReceiveHit(1,.4f,1,FVector::ZeroVector,EFrontlineDamageKind::Gas,.05f);
        // The review owns one clock. Component evaluation still runs, but its
        // asset player must not add unrelated world/asset-loading time to it.
        if(auto* N=U->Soldier->GetSingleNodeInstance())N->SetPlaying(false);
    }
}
void TickFieldActionReview(AWarfareFrontlineGameMode* B,float DT)
{
    if(!Isolated(B))return;
    // Core-ticker DT includes the preceding synchronous fixture/asset load.
    // That must not consume an entire three-second action before its first frame.
    DT=FMath::Clamp(DT,0.f,.05f);
    ReviewTime+=DT;for(const auto& P:ReviewUnits)if(auto* U=P.Get())
    {
        if(U->IsAlive())
        {
            TickAction(U,DT);
            if(U->Role==EFrontlineRole::Pioneer&&!ReviewWireConsumed)
                if(auto* S=States.Find(U);S&&ConsumeWireCut(U,S->Wire))
                {ReviewWireConsumed=true;if(ReviewWire.IsValid())ReviewWire->SetVisibility(false);}
            if(auto* N=U->Soldier->GetSingleNodeInstance())
            {
                N->SetPlaying(false);
                // TickAction already returns completed actions to actual idle.
                // Sample its breathing loop with the same review clock too.
                if(!IsAction(N->GetAnimationAsset())&&N->GetLength()>.01f)
                    N->SetPosition(FMath::Fmod(FMath::Max(0.f,ReviewTime-3.f),N->GetLength()),false);
            }
            U->Soldier->HandleExistingParallelEvaluationTask(true,true);
            U->Soldier->TickAnimation(0.f,false);U->Soldier->RefreshBoneTransforms();
        }
        else
        {
            const auto& Death=U->GetDeathState();
            if(!Death.bPhysicsStarted)if(auto* N=U->Soldier->GetSingleNodeInstance())
            {
                N->SetPlaying(false);N->SetPosition(FMath::Min(ReviewTime,Death.AuthoredClipSeconds),false);
                U->Soldier->HandleExistingParallelEvaluationTask(true,true);
                U->Soldier->TickAnimation(0.f,false);U->Soldier->RefreshBoneTransforms();
            }
            U->TickDeath(DT);
        }
        FrontlineFieldV32::TickUnit(U,DT);
    }
}
TSharedRef<FJsonObject> FieldActionReviewState(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("review_game_seconds"),ReviewTime);TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& P:ReviewUnits)if(auto* U=P.Get())
    {
        const auto* S=States.Find(U);auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("alive"),U->IsAlive());R->SetNumberField(TEXT("action"),S?int32(S->Action):0);R->SetNumberField(TEXT("action_seconds"),S?S->Time:0);R->SetBoolField(TEXT("mask_sealed"),MaskSealed(U));
        if(auto* N=U->Soldier->GetSingleNodeInstance()){R->SetStringField(TEXT("actual_animation"),GetPathNameSafe(N->GetAnimationAsset()));R->SetNumberField(TEXT("animation_seconds"),N->GetCurrentTime());}
        R->SetNumberField(TEXT("right_wrist_relative_head_cm"),U->Soldier->GetBoneLocation(TEXT("hand_r")).Z-U->Soldier->GetBoneLocation(TEXT("Head")).Z);
        R->SetNumberField(TEXT("left_wrist_relative_head_cm"),U->Soldier->GetBoneLocation(TEXT("hand_l")).Z-U->Soldier->GetBoneLocation(TEXT("Head")).Z);
        R->SetNumberField(TEXT("completed_don_actions"),S?S->DonCompleted:0);R->SetNumberField(TEXT("completed_cut_actions"),S?S->CutsCompleted:0);
        bool VisibleMask=false;TInlineComponentArray<USkeletalMeshComponent*> Meshes(U);
        for(auto* M:Meshes)if(M!=U->Soldier&&M->GetSkeletalMeshAsset()&&M->GetSkeletalMeshAsset()->GetName().Contains(TEXT("Respirator")))VisibleMask|=M->IsVisible();
        R->SetBoolField(TEXT("mask_component_visible"),VisibleMask);
        if(!U->IsAlive())
        {
            const auto& Death=U->GetDeathState();R->SetNumberField(TEXT("death_age_seconds"),Death.Age);R->SetNumberField(TEXT("authored_fall_seconds"),Death.AuthoredClipSeconds);
            R->SetBoolField(TEXT("gas_authored_fall_complete"),Death.bAuthoredFall&&ReviewTime>=Death.AuthoredClipSeconds-.001f);
            R->SetBoolField(TEXT("gas_physics_started"),Death.bPhysicsStarted);R->SetBoolField(TEXT("gas_settled"),Death.bSettled);
            R->SetBoolField(TEXT("gas_physics_finite"),!Death.bMissingPhysics&&!Death.bNonFinite);R->SetNumberField(TEXT("gas_body_speed_cm_s"),Death.CurrentBodySpeedCmS);
        }
        Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("actors"),Rows);
    J->SetBoolField(TEXT("wire_completed_and_consumed"),ReviewWireConsumed);
    J->SetBoolField(TEXT("wire_removed_after_completed_action"),ReviewWireConsumed&&ReviewWire.IsValid()&&!ReviewWire->IsVisible());
    if(ReviewUnits.Num()==4&&ReviewUnits[0].IsValid()&&ReviewUnits[1].IsValid())
    {
        J->SetBoolField(TEXT("don_complete_and_sealed"),State(ReviewUnits[0].Get()).DonCompleted==1&&!IsBusy(ReviewUnits[0].Get())&&MaskSealed(ReviewUnits[0].Get()));
        J->SetBoolField(TEXT("doff_complete_and_removed"),!IsBusy(ReviewUnits[1].Get())&&!MaskSealed(ReviewUnits[1].Get()));
    }
    return J;
}
}
