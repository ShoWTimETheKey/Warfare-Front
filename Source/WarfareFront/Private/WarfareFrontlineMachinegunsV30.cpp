#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMotionV34.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineMobilityV41.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// Narrow fixture access; its only caller is the disposable-profile-guarded
// stance check below. No production firing or roster API is exposed.
struct FFrontlineStanceV8216Review
{
    static TArray<TObjectPtr<AFrontlineTank>>& Vehicles(UFrontlineVehicleSubsystem* Fleet){return Fleet->Vehicles;}
    static void DelayFire(AWarfareFrontlineSoldier* U){U->ShotClock=U->GrenadeClock=10000.f;}
    static UAnimationAsset* Asset(const AWarfareFrontlineSoldier* U){return U->Playing.Get();}
};

namespace
{
float Length(UAnimationAsset* A) { const auto* S=Cast<UAnimSequenceBase>(A);return S?S->GetPlayLength():0.f; }
bool Gunner(const AWarfareFrontlineSoldier* U){return U&&(U->IsMachineGunner()||FrontlineSpecialistsV41::IsAntiTankGunner(U));}
bool SupportedTeam(const AWarfareFrontlineSoldier* U){return U&&(U->Role==EFrontlineRole::Lewis||U->Role==EFrontlineRole::MobileGun||U->Role==EFrontlineRole::AntiTankRifle);}
bool PhysicalTrenchPoseV835(const AWarfareFrontlineSoldier* U,bool Retain)
{
    if(!U||!U->Battle||WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const FVector Feet=U->Feet();if(Feet.ContainsNaN()||FMath::Abs(Feet.Y)>FrontlineBattlefield::HalfFrontage-35.f)return false;
    // Passing a bypassed/full trench does not assign a formation bay. Posture
    // nevertheless belongs to the actual depression under the man's feet.
    // The existing owned firing-bay path remains the first choice below.
    bool CheckedFloor=false;
    for(int32 I=0;I<U->Battle->Trenches.Num();++I)
    {
        const float Centre=FrontlineFieldV32::TrenchCentreX(I,Feet.Y);
        if(FMath::Abs(Feet.X-Centre)>(Retain?140.f:115.f))continue;
        if(!CheckedFloor)
        {
            const float Floor=AWarfareFrontlineGround::HeightAt(Feet.X,Feet.Y);
            if(FMath::Abs(Feet.Z-FrontlineInfantryV45::SurfaceHeight(U->Battle,Feet))>8.f||Feet.Z>Floor+8.f)return false;
            CheckedFloor=true;
        }
        bool BothBanks=true;
        for(const float Side:{-1.f,1.f})
        {
            float BankTop=-MAX_flt;
            for(const float Offset:{250.f,300.f})
            {
                const FVector P(Centre+Side*Offset,Feet.Y,0);float Earth=0;
                float Height=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
                if(FrontlineFieldV32::TryEarthworkSurfaceV36(U->Battle,P,Earth))Height=FMath::Max(Height,Earth);
                BankTop=FMath::Max(BankTop,Height);
            }
            BothBanks&=BankTop>=Feet.Z+35.f;
        }
        if(BothBanks)return true;
    }
    return false;
}
struct FFiringBay
{
    FGuid Encounter;
    int32 Trench=INDEX_NONE;
    bool LowPoseV64=false,ValidatedV69=false;
    FVector Desired=FVector::ZeroVector,Position=FVector::ZeroVector;
    FVector FireTargetV75=FVector::ZeroVector,MeasuredMuzzleV75=FVector::ZeroVector;
    bool HasFireTargetV75=false,MeasuredLowV75=false,SectorValidatedV844=false;
};
TMap<TWeakObjectPtr<const AWarfareFrontlineSoldier>,FFiringBay> FiringBays;
float ExecutableTrenchSectorScoreV8444(const FVector& Candidate,const FVector& Feet,bool CandidateLow,bool CurrentLow,float LockedYaw,float Yaw)
{
    const float Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(LockedYaw,Yaw));
    // A same-footpoint candidate can only use the existing in-place traverse
    // and current stance. Reject an unusable cheap candidate before ranking;
    // it must not monopolise the search over a reachable relocated bay.
    if(FVector::DistSquared2D(Candidate,Feet)<=FMath::Square(12.f)&&(CandidateLow!=CurrentLow||Turn>70.f))return MAX_flt;
    return FVector::Dist2D(Candidate,Feet)+.1f*Turn;
}
struct FCrewStationV81
{
    FGuid Encounter;
    TWeakObjectPtr<AWarfareFrontlineSoldier> Gunner;
    FVector GunPosition=FVector::ZeroVector,Position=FVector::ZeroVector;
    FVector AcceptedForwardV87=FVector::ZeroVector;
    FVector ProgressFeet=FVector::ZeroVector;
    float ProgressAt=0,RetryAt=0;
    bool Valid=false,Arrived=false;
};
TMap<TWeakObjectPtr<const AWarfareFrontlineSoldier>,FCrewStationV81> CrewStationsV81;
}

// Pure numerical selector boundaries, called only by the guarded V8444 review.
// This does not manufacture a live collision, supported pose or fired round.
TSharedRef<FJsonObject> RunFrontlineTrenchSectorSelectorChecksV8444(TFunctionRef<void(const FString&,bool)> Check)
{
    auto Result=MakeShared<FJsonObject>();bool Good=true;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    const FVector Feet=FVector::ZeroVector,Relocated(24.f,0,0);
    const float LegacyFirst=8.f,LegacySecond=26.f;
    const float First=ExecutableTrenchSectorScoreV8444(Feet,Feet,false,false,0.f,80.f);
    const float Second=ExecutableTrenchSectorScoreV8444(Relocated,Feet,false,false,0.f,20.f);
    Assert(TEXT("v8444_mg_selector_legacy_cheap_unexecutable_contact_hides_relocated_second_contact"),LegacyFirst<LegacySecond&&First==MAX_flt&&Second<MAX_flt);
    float Best=MAX_flt;int32 Pick=INDEX_NONE;const float Scores[2]={First,Second};
    for(int32 I=0;I<2;++I)if(Scores[I]<Best){Best=Scores[I];Pick=I;}
    Assert(TEXT("v8444_mg_selector_skips_unexecutable_first_and_selects_24cm_second_bay"),Pick==1&&FMath::IsNearlyEqual(Best,LegacySecond));
    Assert(TEXT("v8444_mg_selector_keeps_original_seventy_degree_boundary"),ExecutableTrenchSectorScoreV8444(Feet,Feet,false,false,0.f,70.f)<MAX_flt
        &&ExecutableTrenchSectorScoreV8444(Feet,Feet,false,false,0.f,70.01f)==MAX_flt);
    Assert(TEXT("v8444_mg_selector_same_footpoint_pose_change_requires_a_real_relocation"),ExecutableTrenchSectorScoreV8444(Feet,Feet,true,false,0.f,20.f)==MAX_flt
        &&ExecutableTrenchSectorScoreV8444(Relocated,Feet,true,false,0.f,20.f)<MAX_flt);
    Assert(TEXT("v8444_mg_selector_keeps_twelve_cm_in_place_boundary"),ExecutableTrenchSectorScoreV8444(FVector(12.f,0,0),Feet,false,false,0.f,80.f)==MAX_flt
        &&ExecutableTrenchSectorScoreV8444(FVector(12.01f,0,0),Feet,false,false,0.f,80.f)<MAX_flt);
    Assert(TEXT("v8444_mg_selector_reversed_team_heading_has_the_same_admission_rule"),ExecutableTrenchSectorScoreV8444(Feet,Feet,false,false,180.f,260.f)==MAX_flt
        &&ExecutableTrenchSectorScoreV8444(Relocated,Feet,false,false,180.f,200.f)<MAX_flt);
    Assert(TEXT("v8444_mg_selector_wraparound_heading_retains_the_same_boundary"),ExecutableTrenchSectorScoreV8444(Feet,Feet,false,false,179.f,-111.f)<MAX_flt
        &&ExecutableTrenchSectorScoreV8444(Feet,Feet,false,false,179.f,-110.99f)==MAX_flt);
    Result->SetBoolField(TEXT("success"),Good);Result->SetNumberField(TEXT("legacy_first_score"),LegacyFirst);Result->SetNumberField(TEXT("legacy_second_score"),LegacySecond);
    Result->SetBoolField(TEXT("first_candidate_executable"),First<MAX_flt);Result->SetNumberField(TEXT("selected_candidate"),Pick);Result->SetNumberField(TEXT("selected_score"),Best);
    Result->SetStringField(TEXT("scope"),TEXT("Deterministic numerical checks of the same candidate-admission/scoring helper used by production MG sector selection. No claim of a live original-footpoint over-traverse geometry or actual pack, deployment or bullet."));
    return Result;
}

UFrontlineMachinegunV30::UFrontlineMachinegunV30()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
void UFrontlineMachinegunV30::Initialize(AWarfareFrontlineSoldier* Unit)
{
    OwnerUnit=Unit;bWasGunner=Gunner(Unit);Motions.Reset();bAssetsReady=true;
    const bool National=WarfareFRRUCampaignV82::Active(Unit->Battle);
    bNationalSupportsV82=National;
    if(National)
    {
        for(const TCHAR* A:{TEXT("Aim"),TEXT("Fire"),TEXT("Reload"),TEXT("Deploy"),TEXT("Pack")})
            bAssetsReady&=WarfareFRRUPresentationV82::SupportedMotion(Unit,A,false)!=nullptr;
    }
    else
    {
    for(const TCHAR* Context:{TEXT("Ground"),TEXT("Trench")})
        for(const TCHAR* Kind:{TEXT("MG"),TEXT("Bearer")})
        {
            if(FCString::Strcmp(Kind,TEXT("Bearer"))==0 && FCString::Strcmp(Context,TEXT("Trench"))==0)continue;
            const bool Gun=FCString::Strcmp(Kind,TEXT("MG"))==0;
            const TArray<FString> Actions=Gun?TArray<FString>{TEXT("Aim"),TEXT("Fire"),TEXT("LewisReload"),TEXT("MG0815Reload"),TEXT("Deploy"),TEXT("Pack")}
                :TArray<FString>{TEXT("Aim"),TEXT("Fire"),TEXT("Bolt"),TEXT("Reload"),TEXT("Deploy"),TEXT("Pack")};
            for(const FString& A:Actions)
            {
                const FString Key=FString::Printf(TEXT("%s_%s_%s"),Kind,Context,*A),Name=TEXT("A_")+Key+TEXT("_V30");
            UAnimationAsset* Clip=FrontlineMotionV34::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V30/Animations/%s.%s"),*Name,*Name));
            Motions.Add(Key,Clip);bAssetsReady &= Clip!=nullptr;
            if(Gun&&FCString::Strcmp(Context,TEXT("Trench"))==0)
            {
                const FString LowName=TEXT("A_MG_Trench_")+A+TEXT("_V36");
                auto* Low=LoadObject<UAnimationAsset>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V36/Animations/%s.%s"),*LowName,*LowName));
                Motions.Add(TEXT("MG_TrenchLow_")+A,Low);bAssetsReady&=Low!=nullptr;
            }
            }
        }
    if(FrontlineSpecialistsV41::IsAntiTankGunner(Unit))
        for(const TCHAR* Context:{TEXT("Ground"),TEXT("Trench")})for(const TCHAR* A:{TEXT("Aim"),TEXT("Fire"),TEXT("Bolt"),TEXT("Reload"),TEXT("Deploy"),TEXT("Pack")})
        {
            const FString Key=FString::Printf(TEXT("AT_%s_%s"),Context,A),Name=TEXT("A_")+Key+TEXT("_V41");
            auto* Clip=FrontlineMotionV34::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V41/Animations/%s.%s"),*Name,*Name));
            Motions.Add(Key,Clip);bAssetsReady&=Clip!=nullptr;
        }
    }
    if(!bAssetsReady)UE_LOG(LogTemp,Error,TEXT("WF_V30_SUPPORTED_ACTIONS_MISSING squad=%d member=%d; unsupported gun fire remains blocked"),Unit->SquadId,Unit->MemberIndex);
    if(!ProneSelection)
    {
        ProneSelection=NewObject<UBoxComponent>(Unit,TEXT("ProneSelectionV30"));Unit->AddInstanceComponent(ProneSelection);
        ProneSelection->SetupAttachment(Unit->GetRootComponent());ProneSelection->SetBoxExtent(FVector(115,36,34));
        ProneSelection->SetCollisionResponseToAllChannels(ECR_Ignore);ProneSelection->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        ProneSelection->SetCollisionEnabled(ECollisionEnabled::NoCollision);ProneSelection->SetCanEverAffectNavigation(false);ProneSelection->RegisterComponent();
    }
    if(!National&&BipodLegs.IsEmpty())
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        UMaterialInterface* Steel=nullptr;
        for(int32 I=0;I<Unit->Soldier->GetNumMaterials();++I)
            if(auto* M=Unit->Soldier->GetMaterial(I);M && M->GetBaseMaterial()->GetName().Contains(TEXT("Steel"))) { Steel=M;break; }
        for(int32 I=0;I<2;++I)
        {
            auto* Leg=NewObject<UStaticMeshComponent>(Unit);Unit->AddInstanceComponent(Leg);Leg->SetupAttachment(Unit->GetRootComponent());
            Leg->SetStaticMesh(Mesh);if(Steel)Leg->SetMaterial(0,Steel);Leg->SetCollisionEnabled(ECollisionEnabled::NoCollision);Leg->SetCanEverAffectNavigation(false);
            Leg->SetVisibility(false);Leg->RegisterComponent();BipodLegs.Add(Leg);
        }
    }
}
UAnimationAsset* UFrontlineMachinegunV30::Action(const TCHAR* Name) const
{
    if(!OwnerUnit)return nullptr;
    if(WarfareFRRUCampaignV82::Active(OwnerUnit->Battle))return WarfareFRRUPresentationV82::SupportedMotion(OwnerUnit,Name,bTrench);
    FString A=Name;
    if(FrontlineSpecialistsV41::IsAntiTankGunner(OwnerUnit))
    {const auto* Found=Motions.Find(FString::Printf(TEXT("AT_%s_%s"),bTrench?TEXT("Trench"):TEXT("Ground"),Name));return Found?Found->Get():nullptr;}
    if(A==TEXT("Reload") && OwnerUnit->IsMachineGunner())A=OwnerUnit->GetFaction()==EFrontlineFaction::British?TEXT("LewisReload"):TEXT("MG0815Reload");
    const auto* Squad=OwnerUnit->Battle?OwnerUnit->Battle->FindSquad(OwnerUnit->SquadId):nullptr;
    // A shallow battered position needs a lower supported stance; deep intact
    // trenches retain their standing firing step and accepted hand/feed actions.
    const bool Low=bTrench&&OwnerUnit->IsMachineGunner()&&Squad&&bLowTrenchPoseV8216;
    const FString Key=FString::Printf(TEXT("%s_%s_%s"),OwnerUnit->IsMachineGunner()?TEXT("MG"):TEXT("Bearer"),Low?TEXT("TrenchLow"):bTrench?TEXT("Trench"):TEXT("Ground"),*A);
    const auto* Found=Motions.Find(Key);return Found?Found->Get():nullptr;
}
UAnimationAsset* UFrontlineMachinegunV30::TransitionMotion() const
{
    return State==EFrontlineGunStanceV30::Deploy?Action(TEXT("Deploy")):State==EFrontlineGunStanceV30::Pack?Action(TEXT("Pack")):nullptr;
}
float UFrontlineMachinegunV30::TransitionRate() const
{
    const float Drill=OwnerUnit&&OwnerUnit->Role==EFrontlineRole::MobileGun?1.30f:1.f;
    const float Personal=OwnerUnit?.94f+OwnerUnit->GetMotionPhase()*.12f:1.f;
    return State==EFrontlineGunStanceV30::Pack?1.35f*1.30f*Drill*Personal:State==EFrontlineGunStanceV30::Deploy?1.30f*Drill*Personal:1.f;
}
void UFrontlineMachinegunV30::Start(EFrontlineGunStanceV30 Next,bool bInTrench)
{
    // A native gun being physically packed or planted has ended its previous
    // burst. Carrying that unfinished visual burst through deployment kept
    // the deploy clip at its last frame and prevented the real aim/support fit.
    // Ammunition and all weapon-cycle clocks remain owned by the weapon code.
    if(OwnerUnit&&WarfareFRRUCampaignV82::Active(OwnerUnit->Battle)
        &&(Next==EFrontlineGunStanceV30::Pack||Next==EFrontlineGunStanceV30::Deploy))OwnerUnit->BurstRemaining=0;
    if(Next==EFrontlineGunStanceV30::Deploy||Next==EFrontlineGunStanceV30::Pack||bTrench!=bInTrench)bHasSupportedWeaponV8447=false;
    if(Next==EFrontlineGunStanceV30::Pack||Next==EFrontlineGunStanceV30::Deploy)
        bReviewedTrenchYawV844=bHasReviewedTrenchSectorV844=false;
    if(bTrench!=bInTrench)bHasSupportedObservation=false;
    State=Next;bTrench=bInTrench;StanceClock=0;
    // A new bay is a complete placement, not a live swap between two body poses.
    // Keep the old pose and observed socket while lifting the existing gun.
    if(Next==EFrontlineGunStanceV30::Deploy)
    {
        bLowTrenchPoseV8216=bTrench&&OwnerUnit->IsMachineGunner()&&FrontlineMachinegunsV30::UsesLowTrenchPoseV64(OwnerUnit);
        bPackingForRelocationV8216=bWantsRelocationV81=false;
    }
    ActionSeconds=Length(TransitionMotion())/TransitionRate();
    if(Next==EFrontlineGunStanceV30::Deploy){LockedYaw=OwnerUnit->GetActorRotation().Yaw;++Deployments;}
    else if(Next==EFrontlineGunStanceV30::Pack)++Packs;
}
AFrontlineTank* UFrontlineMachinegunV30::ObserveArmourV48()
{
    if(!OwnerUnit || !OwnerUnit->Battle || !OwnerUnit->IsActiveCombatant()
        || !FrontlineSpecialistsV41::IsAntiTankGunner(OwnerUnit))
    {ObservedArmourV48.Reset();return nullptr;}
    auto* B=OwnerUnit->Battle.Get();auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());
    if(!Fleet){ObservedArmourV48.Reset();return nullptr;}
    // A section can continue watching a tank while its gunner lowers the rifle
    // to deploy or work the bolt. Testing that animated muzzle for *awareness*
    // made the same obstruction alternately order a pack and a deployment.
    // Use the ordinary standing observation height, with real terrain/wall and
    // smoke visibility. Actual discharge retains the separate muzzle sight test.
    const FVector Eye=OwnerUnit->Feet()+FVector(0,0,145.f);
    const float ProfileRange=FrontlineWeapons::ProfileFor(OwnerUnit).FireRangeCm;
    const float Range=(WarfareFRRUCampaignV82::Active(B)?FMath::Min(5400.f,ProfileRange):ProfileRange)*B->EncounterVisibilityScale();
    const auto Visible=[&](AFrontlineTank* Tank)
    {
        if(!IsValid(Tank)||Tank->Team==OwnerUnit->Team||!Tank->IsOperational()||!Tank->HasEnteredField()
            ||FVector::Dist2D(OwnerUnit->Feet(),Tank->AimPoint())>=Range
            ||B->DirectFireMultiplier(OwnerUnit->Feet(),Tank->AimPoint())<.9f)return false;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(V48ArmourObservation),true);Query.AddIgnoredActor(Tank);
        for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Query.AddIgnoredActor(Other);
        FHitResult Hit;return !GetWorld()->LineTraceSingleByChannel(Hit,Eye,Tank->AimPoint(),ECC_Visibility,Query);
    };
    // Keep one still-visible contact across nearest-distance ties. A true loss
    // of sight, range, smoke or destruction invalidates it without a blind timer.
    if(auto* Prior=ObservedArmourV48.Get();Visible(Prior))return Prior;
    AFrontlineTank* Best=nullptr;float Nearest=Range;
    for(AFrontlineTank* Tank:Fleet->GetVehicles())if(Visible(Tank))
    {const float D=FVector::Dist2D(OwnerUnit->Feet(),Tank->AimPoint());if(D<Nearest){Nearest=D;Best=Tank;}}
    ObservedArmourV48=Best;return Best;
}
bool UFrontlineMachinegunV30::HasArmourFiringLineV48(const AFrontlineTank* Tank) const
{
    if(!OwnerUnit||!OwnerUnit->Battle||!IsValid(Tank)||!Tank->IsOperational()||!Tank->HasEnteredField()
        ||Tank->Team==OwnerUnit->Team||!OwnerUnit->IsActiveCombatant())return false;
    const auto* B=OwnerUnit->Battle.Get();
    const float ProfileRange=FrontlineWeapons::ProfileFor(OwnerUnit).FireRangeCm;
    const float Range=(WarfareFRRUCampaignV82::Active(B)?FMath::Min(5400.f,ProfileRange):ProfileRange)*B->EncounterVisibilityScale();
    if(FVector::Dist2D(OwnerUnit->Feet(),Tank->AimPoint())>=Range||B->DirectFireMultiplier(OwnerUnit->Feet(),Tank->AimPoint())<.9f)return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(V48ArmourFiringLine),true);Query.AddIgnoredActor(Tank);
    for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Query.AddIgnoredActor(Other);
    FHitResult Hit;return !GetWorld()->LineTraceSingleByChannel(Hit,OwnerUnit->GetMuzzleLocation(),Tank->AimPoint(),ECC_Visibility,Query);
}
FVector UFrontlineMachinegunV30::TrenchObservationOrigin() const
{
    // A ready evaluated aim uses its current physical socket. Cached aim is
    // for feed/recoil continuity, not a permanently lagged observation ray
    // after traversing or a blended local stance has changed that socket.
    if(OwnerUnit&&State==EFrontlineGunStanceV30::Supported&&OwnerUnit->WeaponPhase==EFrontlineWeaponPhase::Ready
        &&OwnerUnit->Playing==Action(TEXT("Aim")))return OwnerUnit->GetMuzzleLocation();
    if(bHasSupportedObservation)
        return OwnerUnit->GetActorTransform().TransformPosition(SupportedObservationLocal);
    // Perception is the man's real standing eye until an evaluated supported
    // socket exists. Lowering the gun cannot erase the contact that placed it.
    // Every discharge still traces its animated muzzle independently.
    return OwnerUnit->Feet()+FVector(0,0,145.f);
}

TSharedRef<FJsonObject> UFrontlineMachinegunV30::DescribeTrenchFiringSectorV844() const
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("supported"),IsSupported());J->SetBoolField(TEXT("trench"),bTrench);
    J->SetBoolField(TEXT("measured_socket"),bHasSupportedObservation);J->SetBoolField(TEXT("reviewed_turn_pending"),bReviewedTrenchYawV844);
    J->SetNumberField(TEXT("reviewed_yaw"),ReviewedTrenchYawV844);J->SetNumberField(TEXT("deployment_yaw"),LockedYaw);
    J->SetStringField(TEXT("last_pack_reason"),LastPackReasonV8450);
    J->SetNumberField(TEXT("movement_intent_seconds"),MovementIntentSecondsV43);
    J->SetNumberField(TEXT("sector_change_seconds"),SectorChangeSecondsV43);
    if(OwnerUnit)
    {
        const auto* B=OwnerUnit->Battle.Get();const auto* T=OwnerUnit->FocusTarget.Get();const auto* Bay=FiringBays.Find(OwnerUnit);
        J->SetNumberField(TEXT("actual_yaw"),OwnerUnit->GetActorRotation().Yaw);J->SetNumberField(TEXT("weapon_phase"),int32(OwnerUnit->WeaponPhase));
        J->SetNumberField(TEXT("burst_rounds_remaining_v87"),OwnerUnit->BurstRemaining);
        J->SetNumberField(TEXT("native_infantry_obstruction_seconds_v87"),OwnerUnit->NativeInfantryObstructionSecondsV87);
        J->SetStringField(TEXT("native_infantry_recovery_origin_v87"),OwnerUnit->NativeInfantryRecoveryOriginV87.ToString());
        J->SetNumberField(TEXT("shot_clock"),OwnerUnit->ShotClock);J->SetNumberField(TEXT("aim_settle_clock"),OwnerUnit->AimSettleClock);J->SetNumberField(TEXT("fire_pose_clock"),OwnerUnit->FirePoseClock);
        J->SetStringField(TEXT("motion"),GetNameSafe(OwnerUnit->Playing));J->SetStringField(TEXT("actual_muzzle"),OwnerUnit->GetMuzzleLocation().ToString());
        J->SetStringField(TEXT("observation_origin"),TrenchObservationOrigin().ToString());J->SetNumberField(TEXT("cached_aim_socket_delta_cm"),bHasSupportedObservation?FVector::Dist(OwnerUnit->GetMuzzleLocation(),OwnerUnit->GetActorTransform().TransformPosition(SupportedObservationLocal)):-1.f);
        J->SetBoolField(TEXT("focus_present"),T!=nullptr);J->SetBoolField(TEXT("focus_sight"),B&&T&&B->HasSight(OwnerUnit,T));J->SetBoolField(TEXT("focus_muzzle_clear"),T&&HasInfantryFiringLine(T));
        J->SetBoolField(TEXT("candidate_validated"),Bay&&Bay->ValidatedV69);J->SetBoolField(TEXT("candidate_sector_validated"),Bay&&Bay->SectorValidatedV844);
        if(Bay){J->SetStringField(TEXT("candidate_position"),Bay->Position.ToString());J->SetStringField(TEXT("candidate_fire_target"),Bay->FireTargetV75.ToString());}
    }
    return J;
}
bool UFrontlineMachinegunV30::HasInfantryFiringLine(const AWarfareFrontlineSoldier* Target) const
{
    if(!OwnerUnit||!OwnerUnit->Battle||!Target||!Target->IsActiveCombatant()
        ||Target->Team==OwnerUnit->Team||!OwnerUnit->IsActiveCombatant())return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(MGInfantryFiringLine),true);
    for(const AWarfareFrontlineSoldier* Other:OwnerUnit->Battle->Units)if(Other)Query.AddIgnoredActor(Other);
    FHitResult Hit;
    const FVector Muzzle=OwnerUnit->GetMuzzleLocation();
    if(!GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,FrontlineMachinegunsV30::SightTarget(Target),ECC_Visibility,Query))return true;
    return Target->Soldier&&!GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,
        Target->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5),ECC_Visibility,Query);
}
void UFrontlineMachinegunV30::ReviewTrenchFiringSectorV75()
{
    if(OwnerUnit&&WarfareFRRUCampaignV82::Active(OwnerUnit->Battle))return;
    if(!OwnerUnit||!OwnerUnit->Battle||!OwnerUnit->IsMachineGunner()||!bTrench
        ||State!=EFrontlineGunStanceV30::Supported||OwnerUnit->WeaponPhase!=EFrontlineWeaponPhase::Ready
        ||!bHasSupportedObservation)return;
    auto* B=OwnerUnit->Battle.Get();if(B->Elapsed<FiringBayReviewAtV75)return;
    FiringBayReviewAtV75=B->Elapsed+.65f;
    const auto* Squad=B->FindSquad(OwnerUnit->SquadId);if(!Squad||Squad->bAdvancing)return;
    const float Range=FrontlineWeapons::ProfileFor(OwnerUnit).FireRangeCm*B->EncounterVisibilityScale();
    TArray<AWarfareFrontlineSoldier*> Contacts;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(V75GunBayAwareness),true);
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Q.AddIgnoredActor(U);
    for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team!=OwnerUnit->Team&&U->IsActiveCombatant())
    {
        if(FrontlineWeapons::DirectFireRangeDistanceCm(OwnerUnit,U->Feet())>=Range||B->DirectFireMultiplier(OwnerUnit->Feet(),U->Feet())<.9f)continue;
        if(B->HasSight(OwnerUnit,U)&&HasInfantryFiringLine(U)){FiringBayBlockedSinceV8216=-1.f;return;}
        const FVector Eye=OwnerUnit->Feet()+FVector(0,0,145.f),Head=U->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5);
        if(!GetWorld()->LineTraceTestByChannel(Eye,Head,ECC_Visibility,Q))Contacts.Add(U);
    }
    // An adjacent resident can see a changed diagonal sector that this gun's
    // own cached bay cannot. Retain the same measured-muzzle placement search;
    // sharing that observation never bypasses either discharge ray.
    TArray<AWarfareFrontlineSoldier*> Shared;FrontlineInfantryV45::ObservedTrenchContactsV8444(OwnerUnit,Shared);
    for(auto* Contact:Shared)Contacts.AddUnique(Contact);
    if(Contacts.IsEmpty())
    {FiringBayBlockedSinceV8216=-1.f;return;}
    if(auto* Focus=OwnerUnit->FocusTarget.Get();FrontlineMachinegunsV30::ValidContactV61(OwnerUnit,Focus)&&HasInfantryFiringLine(Focus))
    {FiringBayBlockedSinceV8216=-1.f;return;}
    if(FiringBayBlockedSinceV8216<0.f)FiringBayBlockedSinceV8216=B->Elapsed;
    if(B->Elapsed-FiringBayBlockedSinceV8216<1.3f)return;
    FiringBayReviewAtV75=B->Elapsed+2.5f;
    auto* Bay=FiringBays.Find(OwnerUnit);if(!Bay)return;
    // Refit a real blocked muzzle to the observed sector. The original search
    // only checked three forward rays once, so a neighbouring traverse or a
    // changed flank could permanently silence a correctly deployed gun.
    const FFiringBay Original=*Bay;FFiringBay BestBay=Original;float BestScore=MAX_flt,BestYaw=0;
    const FVector Default=B->SoldierDestination(OwnerUnit);
    for(auto* Seen:Contacts)
    {
        *Bay=Original;
        Bay->MeasuredMuzzleV75=OwnerUnit->GetActorTransform().TransformPosition(SupportedObservationLocal)-OwnerUnit->Feet();
        Bay->MeasuredMuzzleV75=OwnerUnit->GetActorQuat().UnrotateVector(Bay->MeasuredMuzzleV75);
        Bay->MeasuredLowV75=Original.LowPoseV64;
        Bay->FireTargetV75=Seen->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5);
        Bay->HasFireTargetV75=true;Bay->SectorValidatedV844=false;Bay->Encounter.Invalidate();
        const FVector Refitted=FrontlineMachinegunsV30::TrenchFiringStepV36(OwnerUnit,Default);
        if(!Bay->ValidatedV69||!Bay->SectorValidatedV844
            ||FrontlineWeapons::DirectFireRangeDistanceCm(OwnerUnit,Seen->Feet()+OwnerUnit->Feet()-Refitted)>Range)continue;
        const float Yaw=(Seen->Feet()-Refitted).Rotation().Yaw;
        const float Score=ExecutableTrenchSectorScoreV8444(Refitted,OwnerUnit->Feet(),Bay->LowPoseV64,bLowTrenchPoseV8216,LockedYaw,Yaw);
        if(Score<BestScore){BestScore=Score;BestBay=*Bay;BestYaw=Yaw;}
    }
    *Bay=BestScore<MAX_flt?BestBay:Original;
    if(BestScore==MAX_flt)return;
    // A valid local refit can keep the feet and require only traversing the
    // planted gun. Without this, its old blocked socket hid the contact which
    // would have ordered that turn, although the measured turned socket cleared.
    if(Bay->LowPoseV64==bLowTrenchPoseV8216&&FVector::DistSquared2D(Bay->Position,OwnerUnit->Feet())<=FMath::Square(12.f)
        &&FMath::Abs(FMath::FindDeltaAngleDegrees(LockedYaw,BestYaw))<=70.f)
    {ReviewedTrenchYawV844=BestYaw;bReviewedTrenchYawV844=bHasReviewedTrenchSectorV844=true;}
}
float UFrontlineMachinegunV30::DeploymentYawV61()
{
    auto* B=OwnerUnit->Battle.Get();
    auto* Target=OwnerUnit->FocusTarget.Get();
    if(!FrontlineMachinegunsV30::ValidContactV61(OwnerUnit,Target))
        OwnerUnit->FocusTarget=Target=B->AcquireTarget(OwnerUnit);
    float Yaw=Target?(Target->Feet()-OwnerUnit->Feet()).Rotation().Yaw:B->GetTeamDirection(OwnerUnit->Team)>0?0.f:180.f;
    // A gunner's last path correction is not an observed enemy bearing. In
    // particular his bearer must not inherit a temporary rearward walk heading.
    if(FrontlineSpecialistsV41::IsAntiTankGunner(OwnerUnit))
    {if(auto* Tank=ObserveArmourV48())Yaw=(Tank->AimPoint()-OwnerUnit->Feet()).Rotation().Yaw;}
    else if(!Target)
        if(auto* V=UFrontlineVehicleSubsystem::Get(GetWorld()))
            if(auto* Tank=V->AcquireInfantryTarget(OwnerUnit))Yaw=(Tank->AimPoint()-OwnerUnit->Feet()).Rotation().Yaw;
    return Yaw;
}
bool UFrontlineMachinegunV30::Update(float DT,bool bWantsMovement)
{
    if(!OwnerUnit || !OwnerUnit->IsActiveCombatant())return false;
    if(bReviewedTrenchYawV844&&State==EFrontlineGunStanceV30::Supported&&!bWantsMovement
        &&OwnerUnit->WeaponPhase==EFrontlineWeaponPhase::Ready)
    {
        OwnerUnit->SetActorRotation(FMath::RInterpTo(OwnerUnit->GetActorRotation(),FRotator(0,ReviewedTrenchYawV844,0),DT,6.f));
        if(FMath::Abs(FMath::FindDeltaAngleDegrees(OwnerUnit->GetActorRotation().Yaw,ReviewedTrenchYawV844))<2.f)bReviewedTrenchYawV844=false;
    }
    if(Gunner(OwnerUnit)!=bWasGunner)
    {
        State=EFrontlineGunStanceV30::Carry;bWasGunner=Gunner(OwnerUnit);bHasSupportedObservation=false;
        bReviewedTrenchYawV844=bHasReviewedTrenchSectorV844=false;
        bLowTrenchPoseV8216=bPackingForRelocationV8216=bWantsRelocationV81=false;
        MovementIntentSecondsV43=StillSecondsV43=SectorChangeSecondsV43=0.f;
    }
    auto* Battle=OwnerUnit->Battle.Get();
    const auto* Assigned=Battle?Battle->FindSquad(OwnerUnit->SquadId):nullptr;
    // Cover probability is not a posture boundary. Curved firing bays can lie
    // a few centimetres outside that narrow damage envelope while a gun is
    // still planted in its assigned trench. Only a real departure changes the
    // support context; hysteresis prevents the bank edge toggling two clips.
    const bool AssignedTrench=Assigned&&!Assigned->bAdvancing&&Battle->Trenches.IsValidIndex(Assigned->Trench)
        &&Battle->Trenches[Assigned->Trench].Owner==OwnerUnit->Team;
    const float BankDistance=AssignedTrench?FMath::Abs(OwnerUnit->Feet().X
        -FrontlineFieldV32::TrenchCentreX(Assigned->Trench,OwnerUnit->Feet().Y)):MAX_flt;
    bool Trench=AssignedTrench&&BankDistance<=(bTrench?185.f:155.f);
    if(!Trench&&!WarfareFRRUCampaignV82::Active(Battle))Trench=PhysicalTrenchPoseV835(OwnerUnit,bTrench);
    if(WarfareFRRUCampaignV82::Active(Battle))
    {
        FFRRUEarthworkPoseV82 Earth;Trench=WarfareFRRUWorldV82::EarthworkPose(OwnerUnit,Earth);
        if(Gunner(OwnerUnit)&&WarfareFRRUPresentationV82::SupportedMotion(OwnerUnit,TEXT("Aim"),true)
            ==WarfareFRRUPresentationV82::SupportedMotion(OwnerUnit,TEXT("Aim"),false))Trench=false;
    }
    const bool HasGun=Gunner(OwnerUnit) || FrontlineMachinegunsV30::LivingGunner(OwnerUnit)!=nullptr;
    const bool NeedsSupport=HasGun && (Gunner(OwnerUnit) || !Trench);
    const bool CanHandle=OwnerUnit->WeaponPhase==EFrontlineWeaponPhase::Ready;
    MovementIntentSecondsV43=bWantsMovement?MovementIntentSecondsV43+DT:0.f;
    StillSecondsV43=!bWantsMovement&&OwnerUnit->GetMotionSpeed()<8.f?StillSecondsV43+DT:0.f;
    StanceClock+=DT;
    if(State==EFrontlineGunStanceV30::Pack)
    {
        if(StanceClock<ActionSeconds)return true;
        State=EFrontlineGunStanceV30::Carry;
    }
    // Rifle escorts can cover a wider prone sector than the planted heavy gun.
    // Only a genuine change of flank makes them stand and deploy again.
    const bool ChangedSector=State==EFrontlineGunStanceV30::Supported
        &&!CoversYaw(OwnerUnit->GetActorRotation().Yaw);
    SectorChangeSecondsV43=ChangedSector?SectorChangeSecondsV43+DT:0.f;
    const bool StableDeparture=bWantsMovement&&MovementIntentSecondsV43>=.22f;
    const bool StableSectorChange=ChangedSector&&SectorChangeSecondsV43>=.65f;
    // Assistants retain their accepted station during an uncommitted movement
    // wish or a sector-only pack. A genuine departure releases it once.
    bWantsRelocationV81=bPackingForRelocationV8216||(State==EFrontlineGunStanceV30::Carry&&bWantsMovement);
    if(State!=EFrontlineGunStanceV30::Carry && (StableDeparture || Trench!=bTrench || !NeedsSupport || StableSectorChange))
    {
        if(CanHandle){LastPackReasonV8450=StableDeparture?TEXT("movement"):Trench!=bTrench?TEXT("context"):!NeedsSupport?TEXT("no_gunner"):TEXT("sector");bPackingForRelocationV8216=StableDeparture;bWantsRelocationV81=StableDeparture;Start(EFrontlineGunStanceV30::Pack,bTrench);return true;}
        return bWantsMovement; // Finish the actual feed/bolt action before lifting the gun.
    }
    if(State==EFrontlineGunStanceV30::Deploy)
    {
        // Finish the arrival turn while the weapon is being placed, before
        // locking its sector. The feet stay planted throughout this action.
        if(bTrench&&OwnerUnit->IsMachineGunner())
            OwnerUnit->SetActorRotation(FMath::RInterpTo(OwnerUnit->GetActorRotation(),FRotator(0,LockedYaw,0),DT,6.f));
        if(StanceClock<ActionSeconds||(bTrench&&OwnerUnit->IsMachineGunner()
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(OwnerUnit->GetActorRotation().Yaw,LockedYaw))>3.f))return true;
        LockedYaw=OwnerUnit->GetActorRotation().Yaw;
        LastDeploymentSeconds=StanceClock;State=EFrontlineGunStanceV30::Supported;
        UE_LOG(LogTemp,Verbose,TEXT("WF_V30_GUN_DEPLOYED squad=%d member=%d trench=%d seconds=%.3f"),OwnerUnit->SquadId,OwnerUnit->MemberIndex,bTrench,LastDeploymentSeconds);
        return bWantsMovement;
    }
    const auto* Squad=Battle?Battle->FindSquad(OwnerUnit->SquadId):nullptr;
    const bool TrenchArrival=Trench&&OwnerUnit->IsMachineGunner()&&Squad&&!Squad->bAdvancing&&!bWantsMovement;
    const float SettleSeconds=Gunner(OwnerUnit)?.30f:.30f+OwnerUnit->GetMotionPhase()*.34f;
    // Returning actual feet to wait for a join corridor is not an arrival.
    // Current infantry/armour contact retains its normal supported stance.
    const bool JoinPause=State==EFrontlineGunStanceV30::Carry&&!bWantsMovement&&!OwnerUnit->bEngaging
        &&WarfareFRRUWorldV82::WaitingToJoinFormation(OwnerUnit);
    if(State==EFrontlineGunStanceV30::Carry && !JoinPause && NeedsSupport && bAssetsReady && (TrenchArrival||StillSecondsV43>=SettleSeconds) && CanHandle)
    {
        const float Yaw=DeploymentYawV61();
        if(TrenchArrival)
        {
            // This frame already has a confirmed arrival, so blend directly
            // from the displayed walking pose into the complete deploy clip.
            // No intervening standing/low-ready clip or artificial .30s wait.
            Start(EFrontlineGunStanceV30::Deploy,true);LockedYaw=Yaw;
            OwnerUnit->SetActorRotation(FMath::RInterpTo(OwnerUnit->GetActorRotation(),FRotator(0,Yaw,0),DT,6.f));return true;
        }
        // Open-ground prone placement still requires a settled bearing.
        if(FMath::Abs(FMath::FindDeltaAngleDegrees(OwnerUnit->GetActorRotation().Yaw,Yaw))>3.f)
        {OwnerUnit->SetActorRotation(FMath::RInterpTo(OwnerUnit->GetActorRotation(),FRotator(0,Yaw,0),DT,6.f));return false;}
        Start(EFrontlineGunStanceV30::Deploy,Trench);return true;
    }
    // Debounce a transient desire to move without allowing a supported gun to
    // slide across the ground during the confirmation interval.
    return State!=EFrontlineGunStanceV30::Carry&&bWantsMovement;
}
bool UFrontlineMachinegunV30::CoversYaw(float WorldYaw) const
{
    return OwnerUnit&&State==EFrontlineGunStanceV30::Supported
        &&FMath::Abs(FMath::FindDeltaAngleDegrees(LockedYaw,WorldYaw))<=(Gunner(OwnerUnit)&&!bTrench?28.f:75.f);
}
bool UFrontlineMachinegunV30::CanFire() const
{
    if(OwnerUnit&&WarfareFRRUCampaignV82::Active(OwnerUnit->Battle)&&Gunner(OwnerUnit))
    {int32 Contacts=0;if(!WarfareFRRUPresentationV82::GunSupportContacts(OwnerUnit,Contacts))return false;}
    return bAssetsReady&&OwnerUnit&&CoversYaw(OwnerUnit->GetActorRotation().Yaw);
}
bool UFrontlineMachinegunV30::ReadSupportedWeaponAnchorV8447(FTransform& WorldTarget) const
{
    if(!OwnerUnit||!OwnerUnit->Battle||!OwnerUnit->IsActiveCombatant()||!OwnerUnit->IsMachineGunner()
        ||!bTrench||State!=EFrontlineGunStanceV30::Supported||!bHasSupportedWeaponV8447
        ||SupportedWeaponEncounterV8447!=OwnerUnit->Battle->GetEncounterId())return false;
    WorldTarget=SupportedWeaponActorV8447*OwnerUnit->GetActorTransform();return true;
}
TSharedRef<FJsonObject> UFrontlineMachinegunV30::DescribeSupportContactsV8447() const
{
    if(SupportGeometryReviewV8447.IsValid())return SupportGeometryReviewV8447.ToSharedRef();
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("scope"),TEXT("No guarded actual UpdateSupports contact sample."));return J;
}
void UFrontlineMachinegunV30::TickComponent(float DT,ELevelTick Tick,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DT,Tick,ThisTick);UpdateSupports();
}
void UFrontlineMachinegunV30::UpdateSupports()
{
    BipodContactCountV36=0;
    if(!OwnerUnit)return;
    const bool Alive=OwnerUnit->IsAlive(),Prone=Alive && IsProne();
    ProneSelection->SetCollisionEnabled(Prone?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
    if(Alive)OwnerUnit->Capsule->SetCollisionEnabled(Prone?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
    ProneSelection->SetRelativeLocation(FVector(-40,0,-56));
    const bool Visible=Alive && Gunner(OwnerUnit) && State==EFrontlineGunStanceV30::Supported;
    static const bool ReviewSupportsV8447=FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    TSharedPtr<FJsonObject> SupportReview;
    if(ReviewSupportsV8447)
    {
        SupportReview=MakeShared<FJsonObject>();SupportGeometryReviewV8447=SupportReview;
        SupportReview->SetStringField(TEXT("scope"),TEXT("The exact evaluated-world geometry and unchanged contact predicates in production UpdateSupports."));
        SupportReview->SetNumberField(TEXT("state"),int32(State));SupportReview->SetNumberField(TEXT("weapon_phase"),int32(OwnerUnit->WeaponPhase));
        SupportReview->SetBoolField(TEXT("visible_supported_gunner"),Visible);SupportReview->SetBoolField(TEXT("trench"),bTrench);
        SupportReview->SetBoolField(TEXT("actual_low_pose"),bLowTrenchPoseV8216);SupportReview->SetBoolField(TEXT("validated_bay"),FrontlineMachinegunsV30::HasValidatedTrenchBayV69(OwnerUnit));
        if(const auto* Bay=FiringBays.Find(OwnerUnit))
        {
            SupportReview->SetStringField(TEXT("bay_position"),Bay->Position.ToString());
            SupportReview->SetNumberField(TEXT("feet_to_bay_cm"),FVector::Dist2D(OwnerUnit->Feet(),Bay->Position));
            SupportReview->SetStringField(TEXT("bay_minus_feet"),(Bay->Position-OwnerUnit->Feet()).ToString());
        }
        SupportReview->SetBoolField(TEXT("arrival_latched"),OwnerUnit->bAtDestinationV43);SupportReview->SetBoolField(TEXT("moving"),OwnerUnit->bMoving);
        SupportReview->SetStringField(TEXT("feet"),OwnerUnit->Feet().ToString());SupportReview->SetNumberField(TEXT("actor_yaw"),OwnerUnit->GetActorRotation().Yaw);
        SupportReview->SetStringField(TEXT("playing_asset"),OwnerUnit->Playing?OwnerUnit->Playing->GetPathName():TEXT("none"));
        SupportReview->SetStringField(TEXT("aim_asset"),Action(TEXT("Aim"))?Action(TEXT("Aim"))->GetPathName():TEXT("none"));
        SupportReview->SetBoolField(TEXT("cached_supported_weapon"),bHasSupportedWeaponV8447);SupportReview->SetNumberField(TEXT("actual_contacts"),0);
    }
    for(const auto& L:BipodLegs)L->SetVisibility(Visible);
    if(!Alive)return;
    if(!Visible)return;
    if(bNationalSupportsV82)
    {
        bNationalSupportReadyV82=WarfareFRRUPresentationV82::GunSupportContacts(OwnerUnit,BipodContactCountV36);
        if(bTrench&&OwnerUnit->WeaponPhase==EFrontlineWeaponPhase::Ready&&OwnerUnit->Playing==Action(TEXT("Aim")))
        {SupportedObservationLocal=OwnerUnit->GetActorTransform().InverseTransformPosition(OwnerUnit->GetMuzzleLocation());bHasSupportedObservation=true;}
        return;
    }
    // Post-update runs after the evaluated skeleton. Keep the last genuine
    // supported aim socket through recoil and feed actions, in actor space so
    // traversing the planted gun preserves its observation sector.
    if(bTrench&&OwnerUnit->IsMachineGunner()&&OwnerUnit->WeaponPhase==EFrontlineWeaponPhase::Ready
        &&OwnerUnit->Playing==Action(TEXT("Aim")))
    {
        SupportedObservationLocal=OwnerUnit->GetActorTransform().InverseTransformPosition(OwnerUnit->GetMuzzleLocation());
        bHasSupportedObservation=true;
    }
    const FVector W=OwnerUnit->Soldier->GetBoneLocation(TEXT("weapon_rifle"));
    const FVector Forward=(OwnerUnit->GetMuzzleLocation()-W).GetSafeNormal(),Right=FVector::CrossProduct(FVector::UpVector,Forward).GetSafeNormal();
    const bool AntiTank=FrontlineSpecialistsV41::IsAntiTankGunner(OwnerUnit);
    const FVector Pivot=AntiTank&&OwnerUnit->Soldier->DoesSocketExist(TEXT("WF_Bipod"))
        ?OwnerUnit->Soldier->GetSocketLocation(TEXT("WF_Bipod")):W+Forward*(AntiTank?50.f:36.f);
    TArray<TSharedPtr<FJsonValue>> ContactRows;
    if(SupportReview)
    {
        SupportReview->SetStringField(TEXT("gun_bone"),W.ToString());SupportReview->SetStringField(TEXT("muzzle"),OwnerUnit->GetMuzzleLocation().ToString());
        SupportReview->SetStringField(TEXT("pivot"),Pivot.ToString());SupportReview->SetStringField(TEXT("forward"),Forward.ToString());
    }
    FVector StationFootOffsetV8447=FVector::ZeroVector;bool StationFootProjectionV8447=false;
    const auto* AssignmentV8447=OwnerUnit->Battle?OwnerUnit->Battle->FindSquad(OwnerUnit->SquadId):nullptr;
    if(bTrench&&OwnerUnit->IsMachineGunner()&&!OwnerUnit->bMoving&&AssignmentV8447&&!AssignmentV8447->bAdvancing)
        if(const auto* Bay=FiringBays.Find(OwnerUnit);Bay&&Bay->ValidatedV69&&Bay->Encounter==OwnerUnit->Battle->GetEncounterId()
            &&Bay->Trench==AssignmentV8447->Trench&&FVector::DistSquared2D(OwnerUnit->Feet(),Bay->Position)<=FMath::Square(12.f))
        {
            // The actor can safely stop a few centimetres from its exact bay.
            // On a steep bank, that small error moves an untargeted bipod foot
            // onto the low face. Keep the real gun/top planted and let only the
            // two feet lean toward their existing validated station's XY.
            // Every foot still samples the actual surface and keeps the same
            // 2 cm / 65 cm contact limits below; no predicted contact is used.
            StationFootOffsetV8447=Bay->Position-OwnerUnit->Feet();StationFootOffsetV8447.Z=0;
            StationFootProjectionV8447=true;
        }
    if(SupportReview)
    {
        SupportReview->SetBoolField(TEXT("actual_station_foot_projection"),StationFootProjectionV8447);
        SupportReview->SetStringField(TEXT("station_foot_xy_offset"),StationFootOffsetV8447.ToString());
        SupportReview->SetNumberField(TEXT("station_foot_xy_offset_cm"),StationFootOffsetV8447.Size2D());
    }
    // The same bipod seats on existing terrain at the firing lip. No personal
    // stack of bags is created in the trench, on departure or after a casualty.
    {
        for(int32 I=0;I<2;++I)
        {
            const float Side=I?1.f:-1.f;const FVector Top=Pivot+Right*Side*4.f;
            const FVector UnprojectedFoot=Pivot+Right*Side*14.f+Forward*6.f;
            FVector Foot=UnprojectedFoot+StationFootOffsetV8447;float Surface=AWarfareFrontlineGround::HeightAt(Foot.X,Foot.Y),Earthwork=0;
            const float GroundSurface=Surface;const bool EarthworkHit=bTrench&&FrontlineFieldV32::TryEarthworkSurfaceV36(OwnerUnit->Battle.Get(),Foot,Earthwork);
            if(EarthworkHit)Surface=FMath::Max(Surface,Earthwork);
            Foot.Z=Surface+1.5f;
            const FVector Axis=Top-Foot;
            // A bad contact must not turn a bipod into metre-long stilts.
            const bool Contact=Axis.Z>2.f && Axis.Size()<(bTrench?65.f:80.f);
            if(SupportReview)
            {
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("side"),I);R->SetStringField(TEXT("top"),Top.ToString());R->SetStringField(TEXT("foot"),Foot.ToString());
                R->SetStringField(TEXT("unprojected_foot_xy"),UnprojectedFoot.ToString());
                R->SetNumberField(TEXT("ground_surface_z"),GroundSurface);R->SetBoolField(TEXT("earthwork_hit"),EarthworkHit);R->SetNumberField(TEXT("earthwork_surface_z"),Earthwork);
                R->SetNumberField(TEXT("selected_surface_z"),Surface);R->SetNumberField(TEXT("axis_z_cm"),Axis.Z);R->SetNumberField(TEXT("axis_length_cm"),Axis.Size());
                R->SetBoolField(TEXT("contact"),Contact);ContactRows.Add(MakeShared<FJsonValueObject>(R));
            }
            BipodLegs[I]->SetVisibility(Contact);if(!Contact)continue;
            ++BipodContactCountV36;
            BipodLegs[I]->SetWorldLocation((Top+Foot)*.5f);
            BipodLegs[I]->SetWorldRotation(FRotationMatrix::MakeFromZ(Axis).Rotator());
            BipodLegs[I]->SetWorldScale3D(FVector(.012f,.012f,Axis.Size()/100.f));
        }
    }
    if(SupportReview){SupportReview->SetArrayField(TEXT("contacts"),ContactRows);SupportReview->SetNumberField(TEXT("actual_contacts"),BipodContactCountV36);}
    if(SupportReview)if(const auto* Bay=FiringBays.Find(OwnerUnit);Bay&&Bay->ValidatedV69)
    {
        // Read-only counterfactual: translate the evaluated rig to its already
        // assigned exact bay. This does not move the actor, change a slot or
        // count as actual support. It isolates stopping error from pose fit.
        const FVector Offset=Bay->Position-OwnerUnit->Feet(),BayPivot=Pivot+Offset;
        TArray<TSharedPtr<FJsonValue>> Projected;int32 ProjectedContacts=0;
        for(int32 I=0;I<2;++I)
        {
            const float Side=I?1.f:-1.f;const FVector Top=BayPivot+Right*Side*4.f;
            FVector Foot=BayPivot+Right*Side*14.f+Forward*6.f;float Surface=AWarfareFrontlineGround::HeightAt(Foot.X,Foot.Y),Earthwork=0;
            const float Ground=Surface;const bool Hit=bTrench&&FrontlineFieldV32::TryEarthworkSurfaceV36(OwnerUnit->Battle.Get(),Foot,Earthwork);
            if(Hit)Surface=FMath::Max(Surface,Earthwork);Foot.Z=Surface+1.5f;const FVector Axis=Top-Foot;
            const bool Contact=Axis.Z>2.f&&Axis.Size()<(bTrench?65.f:80.f);if(Contact)++ProjectedContacts;
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("top"),Top.ToString());R->SetStringField(TEXT("foot"),Foot.ToString());
            R->SetNumberField(TEXT("ground_surface_z"),Ground);R->SetBoolField(TEXT("earthwork_hit"),Hit);R->SetNumberField(TEXT("earthwork_surface_z"),Earthwork);
            R->SetNumberField(TEXT("axis_z_cm"),Axis.Z);R->SetNumberField(TEXT("axis_length_cm"),Axis.Size());R->SetBoolField(TEXT("projected_contact"),Contact);Projected.Add(MakeShared<FJsonValueObject>(R));
        }
        SupportReview->SetStringField(TEXT("exact_bay_projection_scope"),TEXT("Read-only translated evaluated rig at the existing assigned bay. These are predicted contacts, not actual contacts or a substitute for Tick arrival."));
        SupportReview->SetNumberField(TEXT("exact_bay_projected_contacts"),ProjectedContacts);SupportReview->SetArrayField(TEXT("exact_bay_projected_geometry"),Projected);
    }
    if(BipodContactCountV36==2&&bTrench&&OwnerUnit->IsMachineGunner()
        &&OwnerUnit->WeaponPhase==EFrontlineWeaponPhase::Ready&&OwnerUnit->Playing==Action(TEXT("Aim")))
    {
        SupportedWeaponActorV8447=OwnerUnit->Soldier->GetSocketTransform(TEXT("weapon_rifle"),RTS_World)
            .GetRelativeTransform(OwnerUnit->GetActorTransform());
        SupportedWeaponEncounterV8447=OwnerUnit->Battle->GetEncounterId();bHasSupportedWeaponV8447=true;
    }
}

namespace FrontlineMachinegunsV30
{
void Initialize(AWarfareFrontlineSoldier* U)
{
    if(!SupportedTeam(U) || Get(U))return;
    auto* C=NewObject<UFrontlineMachinegunV30>(U);U->AddInstanceComponent(C);C->RegisterComponent();C->AddTickPrerequisiteComponent(U->Soldier);C->Initialize(U);
}
UFrontlineMachinegunV30* Get(const AWarfareFrontlineSoldier* U){return U?U->FindComponentByClass<UFrontlineMachinegunV30>():nullptr;}
AWarfareFrontlineSoldier* LivingGunner(const AWarfareFrontlineSoldier* U)
{
    if(!SupportedTeam(U) || !U->Battle)return nullptr;
    if(const auto* S=U->Battle->FindSquad(U->SquadId))
        for(const auto& W:S->Members)if(auto* G=W.Get();G && G!=U && G->Team==U->Team && G->IsActiveCombatant() && Gunner(G))return G;
    return nullptr;
}
bool ValidContactV61(const AWarfareFrontlineSoldier* U,const AWarfareFrontlineSoldier* Target)
{
    return U&&U->Battle&&Target&&Target!=U&&Target->Battle==U->Battle&&Target->Team!=U->Team&&Target->IsActiveCombatant()
        &&FrontlineWeapons::DirectFireRangeDistanceCm(U,Target->Feet())<=FrontlineWeapons::ProfileFor(U).FireRangeCm*U->Battle->EncounterVisibilityScale()
        &&U->Battle->HasSight(U,Target);
}
float TargetScoreV828(const AWarfareFrontlineSoldier* U,const AWarfareFrontlineSoldier* Target)
{
    if(!U||!Target)return MAX_flt;
    const float Distance=FVector::Dist2D(U->Feet(),Target->Feet());
    float Score=Distance+(Target->bMoving?-250.f:0.f);
    if(!U->IsMachineGunner()||WarfareFRRUCampaignV82::Active(U->Battle))return Score;
    // Spread established guns across exposed threats after pressure has built.
    // Immediate close defence wins over area coverage, and actual fire retains
    // the existing muzzle/range gates.
    if(Distance>900.f)Score+=FMath::SmoothStep(.35f,.90f,Target->Suppression)*1400.f;
    if(const auto* Gun=Get(U);Gun&&Gun->IsSupported()&&!Gun->HasInfantryFiringLine(Target))Score+=3000.f;
    return Score;
}
bool ShouldRetargetV828(const AWarfareFrontlineSoldier* U,const AWarfareFrontlineSoldier* Candidate,const AWarfareFrontlineSoldier* Current)
{
    if(!U||!U->IsMachineGunner()||WarfareFRRUCampaignV82::Active(U->Battle)||!Candidate||Candidate==Current
        ||!Current||!ValidContactV61(U,Candidate))return false;
    if(const auto* Gun=Get(U);Gun&&Gun->IsSupported()
        &&Gun->CoversYaw((Candidate->Feet()-U->Feet()).Rotation().Yaw)&&Gun->HasInfantryFiringLine(Candidate)
        &&(!Gun->CoversYaw((Current->Feet()-U->Feet()).Rotation().Yaw)||!Gun->HasInfantryFiringLine(Current)))return true;
    // A complete burst belongs to one aim point. The margin prevents two
    // similarly suppressed men repeatedly resetting the gun's aim timer.
    return TargetScoreV828(U,Candidate)+280.f<TargetScoreV828(U,Current);
}
bool HasGroundStationV828(const AWarfareFrontlineSoldier* U)
{
    const auto* Station=CrewStationsV81.Find(U);
    if(!U||!U->Battle||!Station||Station->Encounter!=U->Battle->GetEncounterId())return false;
    const auto* G=Station->Gunner.Get();const auto* C=Get(G);
    return G&&G->IsActiveCombatant()&&C&&!C->WantsRelocationV81()&&!C->IsTrench()
        &&FVector::DistSquared2D(G->Feet(),Station->GunPosition)<=FMath::Square(20.f);
}
FVector Destination(AWarfareFrontlineSoldier* U,const FVector& Default,bool& bReform)
{
    bReform=false;if(!SupportedTeam(U))return Default;
    // Native World owns the actual exclusive bay and squad formation cells.
    if(WarfareFRRUCampaignV82::Active(U->Battle))return Default;
    if(Gunner(U))return TrenchFiringStepV36(U,Default);
    if(CrewStationsV81.Num()>256)for(auto It=CrewStationsV81.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    if(auto* G=LivingGunner(U);G && FVector::Dist2D(U->Feet(),G->Feet())<1500.f)
        if(auto* C=Get(G);C && !C->WantsRelocationV81() && !C->IsTrench())
        {
            auto* Prior=CrewStationsV81.Find(U);
            bool SameStation=Prior&&Prior->Encounter==U->Battle->GetEncounterId()&&Prior->Gunner.Get()==G
                &&FVector::DistSquared2D(Prior->GunPosition,G->Feet())<=FMath::Square(20.f);
            FVector FailedStation=FVector::ZeroVector;bool Failed=false;
            const FVector Front=FRotator(0,C->FormationYawV50(),0).Vector();
            // A helper who has already passed his old bay need not retrace the
            // approach while facing incoming fire. Select one physically legal
            // near-side station from his current frontage and then keep it.
            // Passing belongs to the frame in which this station was accepted.
            // A sector-only turn can reverse the gun's current frontage while a
            // reload-paused helper has not moved at all; it cannot pass his bay.
            const bool PassedStationV836=SameStation&&Prior->Valid&&C->IsSupported()
                &&FVector::DotProduct(Prior->Position-U->Feet(),Prior->AcceptedForwardV87)<-64.f;
            if(PassedStationV836){FailedStation=Prior->Position;Failed=true;SameStation=false;}
            if(SameStation)
            {
                const float Remaining=FVector::Dist2D(U->Feet(),Prior->Position);
                const float Arrival=Prior->Arrived?64.f:32.f;
                const auto* Helper=Get(U);
                const bool Handling=U->WeaponPhase!=EFrontlineWeaponPhase::Ready
                    ||(Helper&&(Helper->State==EFrontlineGunStanceV30::Deploy||Helper->State==EFrontlineGunStanceV30::Pack));
                // The 64 cm retention band belongs to an already seated man.
                // A blocked arrival at 33--64 cm has made no progress; treating
                // it as arrival refreshed this clock forever and retried the
                // same rearward goal every frame. Feed/deployment pauses do
                // not count as failed locomotion.
                if(FVector::DistSquared2D(U->Feet(),Prior->ProgressFeet)>FMath::Square(8.f)||Remaining<=Arrival||Handling)
                {Prior->ProgressFeet=U->Feet();Prior->ProgressAt=U->Battle->Elapsed;}
                // Reconsider a genuinely unreachable station, not a brief
                // deployment/weapon pause. Reservations otherwise stay fixed.
                if((!Prior->Valid&&U->Battle->Elapsed>=Prior->RetryAt)
                    ||(Remaining>Arrival&&U->Battle->Elapsed-Prior->ProgressAt>4.f))
                {FailedStation=Prior->Position;Failed=true;SameStation=false;}
            }
            if(!SameStation && C->State!=EFrontlineGunStanceV30::Deploy&&!C->IsSupported())return Default;
            const FRotationMatrix Station(FRotator(0,C->FormationYawV50(),0));
            const float Side=U->MemberIndex%2?1.f:-1.f;
            // The assistant feeds beside the gun, while the observer covers a
            // wider flank. Their ground stations do not orbit every aim change.
            const float Across=U->MemberIndex>=3?240.f:U->MemberIndex%2?120.f:178.f;
            const float Behind=U->MemberIndex>=3?215.f:U->MemberIndex%2?36.f:100.f;
            const FVector Rear=G->Feet()-Station.GetUnitAxis(EAxis::X)*Behind;
            const FVector Lateral=Station.GetUnitAxis(EAxis::Y)*Side*(Across+U->GetMotionPhase()*10.f);
            FVector P=Rear+Lateral;
            const float Frontage=FrontlineBattlefield::HalfFrontage-35.f;
            if(FMath::Abs(P.Y)>Frontage)
            {
                // An outer-edge gun must give its helper a reachable station.
                // Use the covering side inside the same movement frontage;
                // a near-lateral bearing can also put the rear offset outside.
                const FVector Opposite=Rear-Lateral;
                if(FMath::Abs(Opposite.Y)<FMath::Abs(P.Y))P=Opposite;
                P.Y=FMath::Clamp(P.Y,-double(Frontage),double(Frontage));
            }
            // Turning a planted gun is a new firing sector, not a new march
            // order for its section. Keep the actual support station through
            // Pack/Carry/Deploy; a real relocation or replacement invalidates it.
            if(SameStation)P=Prior->Position;
            else
            {
                const FVector Desired=P;float Best=MAX_flt;FVector Chosen=U->Feet();
                const bool ForwardStationV836=PassedStationV836
                    ||(C->IsSupported()&&FVector::DotProduct(Desired-U->Feet(),Front)<-64.f)
                    // A failed rearward reslot is not an order to retrace it
                    // through its blocker. Pick a checked near-side station;
                    // initial, reachable local backward deployment is retained.
                    ||(Failed&&FVector::DotProduct(Desired-U->Feet(),Front)<-8.f);
                // A helper beyond the gun's local 420 cm support area must
                // not march back to its old station. The same checked near-side
                // rule applies there too; if no forward support point exists,
                // keep his real feet instead of inventing a reverse march.
                FCollisionQueryParams Q(SCENE_QUERY_STAT(V828CrewStation),false);
                for(const AWarfareFrontlineSoldier* Other:U->Battle->Units)if(Other)Q.AddIgnoredActor(Other);
                const auto Consider=[&](FVector Candidate)
                {
                    if(FMath::Abs(Candidate.Y)>Frontage||FVector::DistSquared2D(Candidate,G->Feet())>FMath::Square(420.f)
                        ||(ForwardStationV836&&FVector::DotProduct(Candidate-U->Feet(),Front)<-8.f)
                        ||(Failed&&FVector::DistSquared2D(Candidate,FailedStation)<FMath::Square(65.f)))return;
                    Candidate.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Candidate);
                    if(FrontlineInfantryV45::BankIntrusion(U,Candidate)>8.f)return;
                    for(const AWarfareFrontlineSoldier* Other:U->Battle->Units)if(Other&&Other!=U&&Other->IsAlive())
                        if(FVector::DistSquared2D(Candidate,Other->Feet())<FMath::Square(90.f))return;
                    for(const auto& Pair:CrewStationsV81)if(Pair.Key.Get()!=U&&Pair.Key.IsValid()
                        &&Pair.Value.Encounter==U->Battle->GetEncounterId()&&Pair.Value.Gunner.Get()==G)
                        if(FVector::DistSquared2D(Candidate,Pair.Value.Position)<FMath::Square(100.f))return;
                    if(U->GetWorld()->OverlapBlockingTestByChannel(Candidate+FVector(0,0,88),FQuat::Identity,ECC_WorldStatic,
                        FCollisionShape::MakeCapsule(28.f,80.f),Q))return;
                    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(const AFrontlineTank* Tank:Fleet->GetVehicles())
                        if(IsValid(Tank)&&Tank->IsOperational()&&!Tank->IsHidden()
                            &&(Tank->BlocksInfantryAtV8218(Candidate,Tank->GetActorLocation(),Tank->GetActorRotation().Yaw,28.f)
                                ||Tank->InfantrySweepClearanceV8218(U->Feet(),Candidate)<1.f))return;
                    const float Travel=FVector::Dist2D(U->Feet(),Candidate);
                    FVector Last=U->Feet();const int32 Steps=FMath::Max(1,FMath::CeilToInt(Travel/20.f));
                    for(int32 I=1;I<=Steps;++I)
                    {
                        FVector Next=FMath::Lerp(U->Feet(),Candidate,double(I)/Steps);
                        Next.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);
                        if(FMath::Abs(Next.Z-Last.Z)>38.f||!FrontlineInfantryV45::AllowsTrenchStep(U,Last,Next))return;
                        Last=Next;
                    }
                    const float Score=FVector::Dist2D(Candidate,Desired)+Travel*.35f;
                    if(Score<Best){Best=Score;Chosen=Candidate;}
                };
                Consider(Desired);Consider(U->Feet());
                if(ForwardStationV836)for(float Ahead:{0.f,48.f,96.f})for(float AcrossOffset:{-120.f,-60.f,60.f,120.f})
                    Consider(U->Feet()+Front*Ahead+Station.GetUnitAxis(EAxis::Y)*AcrossOffset);
                for(float Radius:{90.f,170.f})for(int32 I=0;I<12;++I)
                    Consider(Desired+FVector(FMath::Cos(I*PI/6.f),FMath::Sin(I*PI/6.f),0)*Radius);
                auto& Saved=CrewStationsV81.FindOrAdd(U);Saved.Encounter=U->Battle->GetEncounterId();Saved.Gunner=G;
                Saved.GunPosition=G->Feet();Saved.Position=P=Chosen;Saved.ProgressFeet=U->Feet();
                Saved.AcceptedForwardV87=Front;
                Saved.ProgressAt=U->Battle->Elapsed;Saved.RetryAt=U->Battle->Elapsed+1.f;Saved.Valid=Best<MAX_flt;Saved.Arrived=false;
                Prior=&Saved;
            }
            P.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,P);
            const float Separation=FVector::Dist2D(U->Feet(),P);
            bReform=Separation>(Prior&&Prior->Arrived?64.f:32.f);
            if(Prior)Prior->Arrived=!bReform;return P;
        }
    CrewStationsV81.Remove(U);
    return Default;
}
FVector TrenchFiringStepV36(const AWarfareFrontlineSoldier* U,const FVector& Default)
{
    if(!U||!U->Battle||!Gunner(U))return Default;
    const auto* Squad=U->Battle->FindSquad(U->SquadId);
    if(!Squad||Squad->bAdvancing||!U->Battle->Trenches.IsValidIndex(Squad->Trench))return Default;
    // Move the actual soldier onto the existing inner firing step. The gun
    // projects across the earthen lip; the man stays inside the cover envelope.
    FVector Step=Default;
    Step.X=FrontlineFieldV32::TrenchCentreX(Squad->Trench,Step.Y)+U->Battle->GetTeamDirection(U->Team)*125.f;
    const FVector Safe=FrontlineInfantryV45::TrenchSlot(U,Squad->Trench,Step,true);
    if(!U->IsMachineGunner())return Safe;
    if(FiringBays.Num()>256)for(auto It=FiringBays.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Bay=FiringBays.FindOrAdd(U);
    if(Bay.Encounter==U->Battle->GetEncounterId()&&Bay.Trench==Squad->Trench&&Bay.Desired.Equals(Default,.01f))return Bay.Position;
    const FFiringBay PreviousBay=Bay;
    const bool ExistingPlacement=Bay.ValidatedV69&&Bay.Trench==Squad->Trench&&Bay.Desired.Equals(Default,.01f);
    const float Dir=U->Battle->GetTeamDirection(U->Team);
    const FVector Forward(Dir,0,0),Right(0,Dir,0);
    const bool PreferLow=FrontlineBattlefield::TrenchCount%2==1&&Squad->Trench==FrontlineBattlefield::CentreTrench;
    TArray<FVector,TInlineAllocator<24>> ReservedSlots;
    for(const AWarfareFrontlineSoldier* Other:U->Battle->Units)
    {
        if(!IsValid(Other)||Other==U||!Other->IsAlive()||Other->Team!=U->Team)continue;
        const auto* OtherSquad=U->Battle->FindSquad(Other->SquadId);
        if(!OtherSquad||(OtherSquad->bAdvancing?OtherSquad->Destination:OtherSquad->Trench)!=Squad->Trench)continue;
        // Fixed formation destinations, not moving bodies, keep the decision
        // stable. Otherwise the gun can choose its assistant's exact station
        // and both men orbit until neither can complete deployment.
        FVector Reserved=U->Battle->SoldierDestination(Other);
        if(OtherSquad->bAdvancing&&!Other->IsSectionLeader())
        {
            Reserved.X=FrontlineFieldV32::TrenchCentreX(Squad->Trench,Reserved.Y)+Dir*125.f;
            Reserved=FrontlineInfantryV45::TrenchSlot(Other,Squad->Trench,Reserved,true);
        }
        if(const auto* OtherBay=FiringBays.Find(Other);OtherBay&&OtherBay->Encounter==U->Battle->GetEncounterId()
            &&OtherBay->Trench==Squad->Trench)Reserved=OtherBay->Position;
        ReservedSlots.Add(Reserved);
    }
    FVector Best=Safe;float BestScore=MAX_flt;bool BestLow=false;
    // A low pose is only valid where the actual surviving lip is low enough.
    // Search both accepted stances across the local parapet, not a silently
    // blocked default bay selected by the trench's array index.
    for(int32 Pose=0;Pose<2;++Pose)for(int32 I=-15;I<=15;++I)
    {
        const bool Low=Pose==0;
        FVector P=Step;P.Y=Default.Y+I*24.f;
        if(FMath::Abs(P.Y)>FrontlineBattlefield::HalfFrontage-60.f)continue;
        P.X=FrontlineFieldV32::TrenchCentreX(Squad->Trench,P.Y)+Dir*125.f;
        P=FrontlineInfantryV45::TrenchSlot(U,Squad->Trench,P,true);
        if(FrontlineInfantryV45::BankIntrusion(U,P)>8.f)continue;
        bool Occupied=false;
        for(const FVector& Reserved:ReservedSlots)
            if(FVector::DistSquared2D(P,Reserved)<FMath::Square(96.f)){Occupied=true;break;}
        if(Occupied)continue;
        const FVector SectorForward=Bay.HasFireTargetV75?(Bay.FireTargetV75-P).GetSafeNormal2D():Forward;
        const FVector SectorRight=FVector::CrossProduct(FVector::UpVector,SectorForward);
        const FVector Pivot=P+SectorForward*54.16f+SectorRight*4.2f+FVector(0,0,Low?96.9f:131.9f);
        bool Contact=true;float MeanGap=0;
        for(const float Side:{-1.f,1.f})
        {
            FVector Foot=Pivot+SectorRight*Side*14.f+SectorForward*6.f;float Bank=0;
            if(!FrontlineFieldV32::TryEarthworkSurfaceV36(U->Battle,Foot,Bank)){Contact=false;break;}
            const float Gap=Pivot.Z-FMath::Max(Bank,AWarfareFrontlineGround::HeightAt(Foot.X,Foot.Y));
            Contact&=Gap>5.f&&Gap<58.f;MeanGap+=Gap*.5f;
        }
        if(!Contact)continue;
        FCollisionQueryParams Sight(SCENE_QUERY_STAT(V64FiringBay),true);
        for(const AWarfareFrontlineSoldier* Other:U->Battle->Units)if(Other)Sight.AddIgnoredActor(Other);
        FVector Muzzle=Pivot+Forward*55.f;
        if(Bay.HasFireTargetV75)
        {
            FVector Local=Bay.MeasuredMuzzleV75;Local.Z+=(Low?96.9f:131.9f)-(Bay.MeasuredLowV75?96.9f:131.9f);
            const FQuat Sector=(Bay.FireTargetV75-P).GetSafeNormal2D().Rotation().Quaternion();
            Muzzle=P+Sector.RotateVector(Local);
        }
        FHitResult Lip;
        // Check the central line and both flanks of this firing sector. A bay
        // aimed over a lip but into its neighbouring traverse is not a firing bay.
        bool Clear=true;
        if(Bay.HasFireTargetV75)
            Clear=!U->GetWorld()->LineTraceTestByChannel(Muzzle,Bay.FireTargetV75,ECC_Visibility,Sight);
        else for(const float Side:{-.30f,0.f,.30f})
            Clear&=!U->GetWorld()->LineTraceSingleByChannel(Lip,Muzzle,Muzzle+(Forward+Right*Side)*650.f,ECC_Visibility,Sight);
        if(!Clear)continue;
        const float Score=FVector::Dist2D(P,Step)+FMath::Abs(MeanGap-22.f)*.25f+(Low==PreferLow?0.f:65.f);
        if(Score<BestScore){BestScore=Score;Best=P;BestLow=Low;}
    }
    if(BestScore==MAX_flt&&ExistingPlacement)
    {
        // A temporarily blocked sector is not proof that the old physical bay
        // is invalid. Keep it until a fully checked replacement exists.
        Bay=PreviousBay;Bay.Encounter=U->Battle->GetEncounterId();return Bay.Position;
    }
    Bay.Encounter=U->Battle->GetEncounterId();Bay.Trench=Squad->Trench;Bay.Desired=Default;Bay.Position=Best;Bay.LowPoseV64=BestLow;
    Bay.ValidatedV69=BestScore<MAX_flt;Bay.SectorValidatedV844=Bay.ValidatedV69&&Bay.HasFireTargetV75;
    if(!Bay.ValidatedV69)UE_LOG(LogTemp,Warning,TEXT("WF_V69_MG_NO_VALID_BAY squad=%d trench=%d slot=%d fallback=%s reserved=%d"),
        U->SquadId,Squad->Trench,Squad->Slot,*Best.ToString(),ReservedSlots.Num());
    return Best;
}
bool HasValidatedTrenchBayV69(const AWarfareFrontlineSoldier* U)
{
    const auto* Bay=FiringBays.Find(U);
    return U&&U->Battle&&Bay&&Bay->Encounter==U->Battle->GetEncounterId()&&Bay->ValidatedV69;
}
bool UsesLowTrenchPoseV64(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle)return false;
    const auto* Bay=FiringBays.Find(U);return Bay&&Bay->Encounter==U->Battle->GetEncounterId()&&Bay->LowPoseV64;
}
UAnimationAsset* Action(const AWarfareFrontlineSoldier* U,const TCHAR* Name)
{
    const auto* C=Get(U);return C && C->IsSupported()?C->Action(Name):nullptr;
}
bool CanFire(AWarfareFrontlineSoldier* U)
{
    if(!SupportedTeam(U))return true;
    auto* C=Get(U);
    const bool NeedsSupport=Gunner(U) || (C && C->IsProne());
    const bool Ready=!NeedsSupport || (C && C->CanFire());
    if(!Ready && C)++C->BlockedShotAttempts;return Ready;
}
FVector SightOrigin(const AWarfareFrontlineSoldier* U)
{
    // A heavy-rifle section retains the same awareness point while lowering
    // its weapon to deploy/feed. Discharge separately traces its actual muzzle.
    if(FrontlineSpecialistsV41::IsAntiTankGunner(U))return U->Feet()+FVector(0,0,145.f);
    if(const auto* C=Get(U);C&&U->IsMachineGunner()&&C->IsTrench())return C->TrenchObservationOrigin();
    // The section retains the same tactical observation point while lowering
    // into open-ground support or feeding. An animated prone head must not
    // remove the very contact that ordered deployment and restart marching.
    // Every supported round separately traces its current physical muzzle.
    if(const auto* C=Get(U);C&&!C->IsTrench())return U->Feet()+FVector(0,0,145.f);
    if(const auto* C=Get(U);C && (C->IsProne()||(C->IsTrench()&&C->IsSupported())))return U->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,8.f);
    return U->Feet()+FVector(0,0,145.f);
}
FVector SightTarget(const AWarfareFrontlineSoldier* U)
{
    if(const auto* C=Get(U);C && (C->IsProne()||(C->IsTrench()&&C->IsSupported())))return U->Soldier->GetBoneLocation(TEXT("spine_03"))+FVector(0,0,6.f);
    return U->Feet()+FVector(0,0,135.f);
}

void RunStanceChecksV8216(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    const bool Isolated=B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
    Check(TEXT("v8216_supported_stance_requires_disposable_profile"),Isolated);if(!Isolated)return;
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTracers=B->Tracers;
    const auto SavedPhase=B->Phase;const float SavedClock=B->Elapsed;const int32 SavedOutro=B->BattleOutroWinner;
    const int32 SavedShots=B->ShotsFired,SavedCasualties[2]={B->Casualties[0],B->Casualties[1]};
    const float SavedMorale[2]={B->Morale[0],B->Morale[1]},SavedDamage[2]={B->DamageDealt[0],B->DamageDealt[1]},SavedPeak=B->PeakSuppression;
    const FFrontlineCombatRecordV65 SavedRecords[2]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    const auto SavedVehicles=Fleet?FFrontlineStanceV8216Review::Vehicles(Fleet):TArray<TObjectPtr<AFrontlineTank>>();
    struct FHidden {TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;bool Hidden,Collision;};TArray<FHidden> Hidden;
    for(AWarfareFrontlineSoldier* U:SavedUnits)if(IsValid(U))
    {Hidden.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
    TArray<AWarfareFrontlineSoldier*> Made;
    ON_SCOPE_EXIT
    {
        for(auto* U:Made)if(IsValid(U)){FiringBays.Remove(U);CrewStationsV81.Remove(U);U->Destroy();}
        B->Units=SavedUnits;B->Squads=SavedSquads;B->Tracers=SavedTracers;B->Phase=SavedPhase;B->Elapsed=SavedClock;B->BattleOutroWinner=SavedOutro;
        B->ShotsFired=SavedShots;B->PeakSuppression=SavedPeak;
        for(int32 Team=0;Team<2;++Team){B->Morale[Team]=SavedMorale[Team];B->Casualties[Team]=SavedCasualties[Team];B->DamageDealt[Team]=SavedDamage[Team];B->CombatRecordV65[Team]=SavedRecords[Team];}
        if(Fleet)FFrontlineStanceV8216Review::Vehicles(Fleet)=SavedVehicles;
        for(const auto& H:Hidden)if(auto* U=H.Unit.Get()){U->SetActorHiddenInGame(H.Hidden);U->SetActorEnableCollision(H.Collision);}
        FrontlineMobilityV41::Reset(B);
    };
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();};
    const auto Make=[&](int32 Id,int32 Team,EFrontlineRole Role,int32 Member,const FVector& P,bool Delay=true)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
        U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88.f));
        U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0.f:180.f,0));
        if(Delay)FFrontlineStanceV8216Review::DelayFire(U);B->Units.Add(U);Made.Add(U);Pose(U,0.f);return U;
    };
    for(int32 Team=0;Team<2;++Team)
    {
        for(auto* U:Made)if(IsValid(U)){FiringBays.Remove(U);CrewStationsV81.Remove(U);U->Destroy();}Made.Reset();
        B->Units.Reset();B->Squads.Reset();B->Tracers.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=300.f;
        if(Fleet)FFrontlineStanceV8216Review::Vehicles(Fleet).Reset();
        if(B->Trenches.Num()<2){Check(TEXT("v8216_stance_fixture_has_actual_trenches"),false);return;}
        const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1);
        const FVector Station((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
        auto* G=Make(9821600+Team,Team,EFrontlineRole::Lewis,0,Station);
        auto* Bearer=Make(9821600+Team,Team,EFrontlineRole::Lewis,1,Station+FVector(-Dir*40.f,320.f,0));
        auto* Observer=Make(9821600+Team,Team,EFrontlineRole::Lewis,2,Station+FVector(-Dir*90.f,-320.f,0));
        const float ContactGap=FMath::Min(650.f,FMath::Abs(B->Trenches[Home].X-B->Trenches[Next].X)*.24f);
        auto* Enemy=Make(9821690+Team,1-Team,EFrontlineRole::Rifle,0,Station+FVector(Dir*ContactGap,0,0));
        const FString Prefix=FString::Printf(TEXT("v8216_nation_%d_"),Team);
        Check(Prefix+TEXT("actual_section_actors_created"),G&&Bearer&&Observer&&Enemy);if(!G||!Bearer||!Observer||!Enemy)return;
        FFrontlineSquad Q;Q.Id=G->SquadId;Q.Team=Team;Q.Trench=INDEX_NONE;Q.Destination=Next;Q.bAdvancing=true;Q.bReinforcement=false;
        Q.Members.Add(G);Q.Members.Add(Bearer);Q.Members.Add(Observer);B->Squads.Add(Q);
        auto* Gun=Get(G);const bool Sight=Gun&&Gun->HasAssets()&&B->HasSight(G,Enemy)&&B->AcquireTarget(G,true)==Enemy;
        Check(Prefix+TEXT("uses_real_clear_contact_and_authored_support_clips"),Sight);if(!Sight)continue;
        FrontlineMobilityV41::Reset(B);
        const auto TickCrew=[&](float DT)
        {
            B->Elapsed+=DT;
            for(auto* U:{G,Bearer,Observer}){U->Tick(DT);Pose(U,DT);if(auto* C=Get(U))C->TickComponent(DT,LEVELTICK_All,nullptr);}
        };
        for(int32 I=0;I<180;++I)TickCrew(.05f);
        const bool Planted=Gun->IsSupported()&&Gun->Deployments==1&&Gun->Packs==0&&!G->bMoving;
        Check(Prefix+TEXT("ordinary_tick_plants_once_against_actual_contact"),Planted);
        const FVector BearerFeet=Bearer->Feet(),ObserverFeet=Observer->Feet();
        for(int32 I=0;I<60;++I)TickCrew(.05f);
        Check(Prefix+TEXT("both_escorts_keep_station_without_running_or_reverse_orbit"),
            !Bearer->bMoving&&!Observer->bMoving&&Bearer->GetMotionSpeed()==0.f&&Observer->GetMotionSpeed()==0.f
            &&FVector::Dist2D(BearerFeet,Bearer->Feet())<1.f&&FVector::Dist2D(ObserverFeet,Observer->Feet())<1.f
            &&FFrontlineStanceV8216Review::Asset(Bearer)!=Bearer->GetRunMotion()&&FFrontlineStanceV8216Review::Asset(Observer)!=Observer->GetRunMotion());
        bool Reformed=false;const FVector Accepted=Destination(Bearer,B->SoldierDestination(Bearer),Reformed);
        const int32 Deployments=Gun->Deployments,Packs=Gun->Packs;bool StableStation=true;
        for(int32 I=0;I<12;++I)
        {
            Gun->Update(.10f,true);
            StableStation&=!Gun->WantsRelocationV81()&&Destination(Bearer,B->SoldierDestination(Bearer),Reformed).Equals(Accepted,.01f);
            Gun->Update(.10f,false);
        }
        Check(Prefix+TEXT("transient_move_wish_never_releases_assistant_station_or_repacks"),
            StableStation&&Gun->IsSupported()&&Gun->Deployments==Deployments&&Gun->Packs==Packs);
        const float Locked=Gun->FormationYawV50();G->SetActorRotation(FRotator(0,Locked+45.f,0));
        Gun->Update(.30f,false);G->SetActorRotation(FRotator(0,Locked,0));Gun->Update(.30f,false);
        Check(Prefix+TEXT("transient_bearing_change_keeps_the_same_supported_placement"),Gun->IsSupported()&&Gun->Packs==Packs);
        B->Units.Remove(Enemy);Enemy->Destroy();FrontlineMobilityV41::Reset(B);const FVector BeforeDeparture=G->Feet();
        for(int32 I=0;I<100;++I)TickCrew(.05f);
        Check(Prefix+TEXT("real_contact_loss_still_packs_and_resumes_forward_travel"),
            Gun->Packs>Packs&&FVector::Dist2D(BeforeDeparture,G->Feet())>100.f&&(G->Feet().X-BeforeDeparture.X)*Dir>0.f);
        // Physical bank checks use the displayed operation's sampled triangles.
        // A curved wall that is worse after a pure side step is never exempted
        // just because the squad owns an Advance order.
        bool FoundWall=false,SideBlocked=false,ForwardAllowed=false;
        for(int32 Trench=0;Trench<B->Trenches.Num()&&!FoundWall;++Trench)
            for(int32 Y=-1300;Y<=1300&&!FoundWall;Y+=40)for(int32 X=-112;X<=112&&!FoundWall;X+=16)
            {
                FVector From(FrontlineFieldV32::TrenchCentreX(Trench,Y)+X,Y,0);From.Z=AWarfareFrontlineGround::HeightAt(From.X,From.Y);
                if(FrontlineInfantryV45::BankIntrusion(G,From)>8.f)continue;
                for(float Side:{-80.f,80.f})
                {
                    FVector To=From+FVector(0,Side,0);const float Intrusion=FrontlineInfantryV45::BankIntrusion(G,To);
                    if(Intrusion<=8.f)continue;FoundWall=true;
                    SideBlocked=!FrontlineInfantryV45::AllowsTrenchStep(G,From,To);
                    ForwardAllowed=FrontlineInfantryV45::AllowsTrenchStep(G,From,From+FVector(Dir*16.f,0,0));break;
                }
            }
        Check(Prefix+TEXT("advance_cannot_side_step_shoulders_through_actual_inner_bank"),FoundWall&&SideBlocked&&ForwardAllowed);

        // Separate owned-trench case: the open-ground fixture above cannot
        // establish that a real parapet supports sustained, unobstructed fire.
        // Initialize normal role clocks, use the production trench destinations,
        // and let ordinary Tick choose/place the supported weapon itself.
        for(auto* U:Made)if(IsValid(U)){FiringBays.Remove(U);CrewStationsV81.Remove(U);U->Destroy();}Made.Reset();
        B->Units.Reset();B->Squads.Reset();B->Tracers.Reset();FrontlineMobilityV41::Reset(B);
        FFrontlineSquad Held;Held.Id=9821620+Team;Held.Team=Team;Held.Role=EFrontlineRole::Lewis;
        Held.Trench=Held.Destination=Held.LastPassedTrench=Home;Held.Slot=1;Held.bHoldPosition=true;
        B->Squads.Add(Held);
        TArray<AWarfareFrontlineSoldier*> HeldCrew;
        for(int32 Member=0;Member<3;++Member)
        {
            const FVector Centre(FrontlineFieldV32::TrenchCentreX(Home,0),0,0);
            auto* U=Make(Held.Id,Team,EFrontlineRole::Lewis,Member,Centre,false);
            if(!U)continue;HeldCrew.Add(U);B->Squads.Last().Members.Add(U);
        }
        for(auto* U:HeldCrew)
        {
            FVector P=B->SoldierDestination(U);
            P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
            U->SetActorLocation(P+FVector(0,0,88));Pose(U,0.f);
        }
        // Real, mortal infantry contacts cover several lanes, as in V69's
        // production trench review. Neither LOS nor supported state is forged.
        for(int32 I=0;I<12;++I)
        {
            const float Y=(I/3-1.5f)*480.f+(I%3)*55.f;
            const FVector P(FrontlineFieldV32::TrenchCentreX(Home,Y)+Dir*(1050.f+(I%3)*95.f),Y,0);
            Make(9821680+Team*20+I,1-Team,EFrontlineRole::Rifle,0,P,false);
        }
        auto* HeldG=HeldCrew.Num()>0?HeldCrew[0]:nullptr;auto* HeldGun=Get(HeldG);
        const bool HeldFixture=HeldCrew.Num()==3&&HeldGun&&HeldGun->HasAssets()
            &&B->Trenches[Home].Owner==Team&&B->FindSquad(Held.Id)->Trench==Home&&!B->FindSquad(Held.Id)->bAdvancing;
        Check(Prefix+TEXT("home_mg_uses_owned_trench_and_production_member_destinations"),HeldFixture);
        bool StableFire=HeldFixture,EscortsStill=HeldFixture;float SupportedContactSeconds=0.f;
        int32 FirstDeployment=INDEX_NONE,FirstPack=INDEX_NONE;FVector EscortFeet[2];
        if(HeldFixture)for(int32 I=0;I<240;++I)
        {
            B->Elapsed+=.05f;
            for(auto* U:HeldCrew){U->Tick(.05f);Pose(U,.05f);if(auto* C=Get(U))C->TickComponent(.05f,LEVELTICK_All,nullptr);}
            const auto* Target=B->AcquireTarget(HeldG,true);
            const bool ClearContact=Target&&HeldGun->HasInfantryFiringLine(Target)&&B->HasSight(HeldG,Target);
            if(HeldGun->IsTrench()&&HeldGun->IsSupported()&&ClearContact)
            {
                if(FirstDeployment==INDEX_NONE)
                {FirstDeployment=HeldGun->Deployments;FirstPack=HeldGun->Packs;EscortFeet[0]=HeldCrew[1]->Feet();EscortFeet[1]=HeldCrew[2]->Feet();}
                SupportedContactSeconds+=.05f;
                StableFire&=HeldGun->Deployments==FirstDeployment&&HeldGun->Packs==FirstPack;
                for(int32 Escort=1;Escort<3;++Escort)
                    EscortsStill&=!HeldCrew[Escort]->bMoving&&HeldCrew[Escort]->GetMotionSpeed()==0.f
                        &&FVector::Dist2D(EscortFeet[Escort-1],HeldCrew[Escort]->Feet())<1.f
                        &&FFrontlineStanceV8216Review::Asset(HeldCrew[Escort])!=HeldCrew[Escort]->GetRunMotion();
            }
            else if(FirstDeployment!=INDEX_NONE&&Target)
                StableFire&=HeldGun->IsTrench()&&HeldGun->IsSupported();
        }
        Check(Prefix+TEXT("home_mg_really_fires_without_deploy_pack_cycles"),HeldFixture&&StableFire
            &&SupportedContactSeconds>=3.f&&HeldG->ShotsDischarged>0&&HeldGun->IsTrench()&&HeldGun->Deployments==1&&HeldGun->Packs==0);
        Check(Prefix+TEXT("home_mg_escorts_hold_real_station_without_running"),HeldFixture&&SupportedContactSeconds>=3.f&&EscortsStill);
        const auto* HeldTarget=HeldG?B->AcquireTarget(HeldG,true):nullptr;
        UE_LOG(LogTemp,Display,TEXT("WF_V8216_HOME_MG team=%d actors=%d shots=%d deployments=%d packs=%d trench=%d supported=%d clear_contact_seconds=%.2f escorts=%d stable=%d target=%d observation=%d muzzle=%d feet=%s"),
            Team,HeldCrew.Num(),HeldG?HeldG->ShotsDischarged:0,HeldGun?HeldGun->Deployments:0,HeldGun?HeldGun->Packs:0,
            HeldGun&&HeldGun->IsTrench(),HeldGun&&HeldGun->IsSupported(),SupportedContactSeconds,EscortsStill,StableFire,
            HeldTarget!=nullptr,HeldTarget&&B->HasSight(HeldG,HeldTarget),HeldTarget&&HeldGun&&HeldGun->HasInfantryFiringLine(HeldTarget),
            HeldG?*HeldG->Feet().ToString():TEXT("missing"));
    }
}
}

