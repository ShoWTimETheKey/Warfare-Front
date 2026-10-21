#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WarfareFrontlineMachinegunsV30.generated.h"

class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class UAnimationAsset;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UBoxComponent;
class AFrontlineTank;
class FJsonObject;

enum class EFrontlineGunStanceV30 : uint8 { Carry, Deploy, Supported, Pack };

/** A real movement / weapon gate; no timer grants a standing gunner free fire. */
UCLASS(Transient)
class WARFAREFRONT_API UFrontlineMachinegunV30 : public UActorComponent
{
    GENERATED_BODY()
public:
    UFrontlineMachinegunV30();
    void Initialize(AWarfareFrontlineSoldier* Unit);
    bool Update(float DT,bool bWantsMovement);
    virtual void TickComponent(float DT,ELevelTick Tick,FActorComponentTickFunction* ThisTick) override;
    UAnimationAsset* TransitionMotion() const;
    UAnimationAsset* Action(const TCHAR* Name) const;
    bool CanFire() const;
    bool CoversYaw(float WorldYaw) const;
    // Section observation is independent of the moving reload/deploy muzzle.
    // This contact never authorizes a bullet through an obstructed firing line.
    AFrontlineTank* ObserveArmourV48();
    bool HasArmourFiringLineV48(const AFrontlineTank* Tank) const;
    // A gun looking over a firing lip observes from its supported firing pose,
    // not the gunner's lower head or an animated reload muzzle.
    FVector TrenchObservationOrigin() const;
    bool HasInfantryFiringLine(const AWarfareFrontlineSoldier* Target) const;
    void ReviewTrenchFiringSectorV75();
    TSharedRef<FJsonObject> DescribeTrenchFiringSectorV844() const;
    bool IsProne() const { return !bTrench && State!=EFrontlineGunStanceV30::Carry; }
    bool IsSupported() const { return State==EFrontlineGunStanceV30::Supported; }
    bool IsTrench() const { return bTrench; }
    bool WantsRelocationV81() const { return bWantsRelocationV81; }
    float FormationYawV50() const { return bHasReviewedTrenchSectorV844?ReviewedTrenchYawV844:LockedYaw; }
    bool HasAssets() const { return bAssetsReady; }
    bool HasBipodContactV36() const { return bNationalSupportsV82?bNationalSupportReadyV82:BipodContactCountV36==2; }
    // Read only the last Ready/Aim weapon transform with two actual contacts.
    // Reload feed transforms remain relative to this accepted physical mount.
    bool ReadSupportedWeaponAnchorV8447(FTransform& WorldTarget) const;
    TSharedRef<FJsonObject> DescribeSupportContactsV8447() const;
    float TransitionRate() const;
    float TransitionDurationV38() const { return ActionSeconds; }
    int32 Deployments=0,Packs=0,BlockedShotAttempts=0;
    FString LastPackReasonV8450;
    float StanceClock=0,LastDeploymentSeconds=0;
    EFrontlineGunStanceV30 State=EFrontlineGunStanceV30::Carry;
private:
    UPROPERTY(Transient) TObjectPtr<AWarfareFrontlineSoldier> OwnerUnit;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UAnimationAsset>> Motions;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> BipodLegs;
    UPROPERTY(Transient) TObjectPtr<UBoxComponent> ProneSelection;
    UPROPERTY(Transient) TWeakObjectPtr<AFrontlineTank> ObservedArmourV48;
    bool bTrench=false,bAssetsReady=false,bWasGunner=false,bWantsRelocationV81=false;
    bool bLowTrenchPoseV8216=false,bPackingForRelocationV8216=false;
    bool bNationalSupportsV82=false,bNationalSupportReadyV82=false;
    float LockedYaw=0,ActionSeconds=0;
    float MovementIntentSecondsV43=0,StillSecondsV43=0,SectorChangeSecondsV43=0;
    int32 BipodContactCountV36=0;
    FVector SupportedObservationLocal=FVector::ZeroVector;
    bool bHasSupportedObservation=false;
    FTransform SupportedWeaponActorV8447=FTransform::Identity;
    FGuid SupportedWeaponEncounterV8447;
    bool bHasSupportedWeaponV8447=false;
    TSharedPtr<FJsonObject> SupportGeometryReviewV8447;
    float FiringBayReviewAtV75=0.f;
    float FiringBayBlockedSinceV8216=-1.f;
    float ReviewedTrenchYawV844=0.f;
    bool bReviewedTrenchYawV844=false;
    bool bHasReviewedTrenchSectorV844=false;
    void Start(EFrontlineGunStanceV30 Next,bool bInTrench);
    float DeploymentYawV61();
    void UpdateSupports();
};

namespace FrontlineMachinegunsV30
{
void Initialize(AWarfareFrontlineSoldier* Unit);
UFrontlineMachinegunV30* Get(const AWarfareFrontlineSoldier* Unit);
AWarfareFrontlineSoldier* LivingGunner(const AWarfareFrontlineSoldier* Unit);
bool ValidContactV61(const AWarfareFrontlineSoldier* Unit,const AWarfareFrontlineSoldier* Target);
float TargetScoreV828(const AWarfareFrontlineSoldier* Unit,const AWarfareFrontlineSoldier* Target);
bool ShouldRetargetV828(const AWarfareFrontlineSoldier* Unit,const AWarfareFrontlineSoldier* Candidate,const AWarfareFrontlineSoldier* Current);
bool HasGroundStationV828(const AWarfareFrontlineSoldier* Unit);
FVector Destination(AWarfareFrontlineSoldier* Unit,const FVector& Default,bool& bReform);
UAnimationAsset* Action(const AWarfareFrontlineSoldier* Unit,const TCHAR* Name);
bool CanFire(AWarfareFrontlineSoldier* Unit);
FVector SightOrigin(const AWarfareFrontlineSoldier* Unit);
FVector SightTarget(const AWarfareFrontlineSoldier* Unit);
FVector TrenchFiringStepV36(const AWarfareFrontlineSoldier* Unit,const FVector& Default);
bool UsesLowTrenchPoseV64(const AWarfareFrontlineSoldier* Unit);
bool HasValidatedTrenchBayV69(const AWarfareFrontlineSoldier* Unit);
void RunStanceChecksV8216(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
