#pragma once

#include "CoreMinimal.h"

class UAnimationAsset;

/** Impact category controls a fatal hit's physical response, never survivable injury. */
enum class EFrontlineDamageKind : uint8
{
    Bullet,
    Grenade,
    Artillery,
    Gas,
    Fire,
    HeavyBullet
};

inline bool IsFrontlineExplosiveDamage(EFrontlineDamageKind Kind)
{return Kind==EFrontlineDamageKind::Grenade || Kind==EFrontlineDamageKind::Artillery;}

struct FFrontlineDeathVisualBinding
{
    FName Bone,PhysicsBody;
    FVector BodyLocalPoint=FVector::ZeroVector;
};

/** Measured on actual Chaos bodies, including unclamped values for detecting unstable solves. */
struct FFrontlineDeathState
{
    EFrontlineDamageKind Kind=EFrontlineDamageKind::Bullet;
    bool bStarted=false,bSettled=false,bMissingPhysics=false,bNonFinite=false;
    bool bAuthoredFall=false,bPhysicsStarted=false,bSeverAttempted=false;
    bool bFuelRupture=false,bFuelRuptureApplied=false,bFuelInterruptedAuthoredFall=false;
    float FuelRuptureAt=-1,FuelRupturePoseSeconds=0;
    FVector FuelRuptureDeltaV=FVector::ZeroVector;
    // Unauthored deaths freeze immediately, then create bodies on a later actor
    // PrePhysics tick, before this mesh's prerequisite-ordered evaluation.
    uint64 PendingPhysicsFrame=MAX_uint64;
    float Age=0,QuietSeconds=0,SettledAt=-1;
    float PhysicsStartedAt=-1,AuthoredClipSeconds=0,LastDiagnosticAt=0,ForceScale=1;
    float PredictedUnfittedClearanceCm=0,TerrainFitAngleDegrees=0,TerrainFitLiftCm=0,HandoffTerrainClearanceCm=0;
    float MaximumHandoffPosePredictionErrorCm=0;
    float InitializationBoneShiftCm=0,InitializedBodyToBoneErrorCm=0;
    float MaximumRightElbowLimitExcessDegrees=0;
    float FirstPhysicalPoseErrorCm=-1,MaximumPhysicalPoseErrorCm=0;
    int32 PhysicalPoseSamples=0;
    // The original actor-tick probe remains intact. This second probe runs after
    // the mesh publishes its completed physics blend, not before that blend.
    float FirstPublishedPoseErrorCm=-1,MaximumPublishedPoseErrorCm=0,LastPublishedPoseErrorCm=-1;
    int32 PublishedPoseSamples=0,NonPhysicsFinalizations=0,FirstPublishedTickGroup=-1;
    uint64 LastPublishedFrame=MAX_uint64;
    FDelegateHandle BoneFinalizedHandle;
    TWeakObjectPtr<UAnimationAsset> AuthoredAnimation;
    int32 AuthoredVariantIndex=INDEX_NONE;
    TArray<FFrontlineDeathVisualBinding> VisualBindings;
    FName HandoffLowestBody;
    FName PeakAngularBody;
    FTransform AuthoredBaseTransform=FTransform::Identity;
    FVector TerrainPivot=FVector::ZeroVector;
    FQuat TerrainFitRotation=FQuat::Identity;
    FVector PredictedFittedHead=FVector::ZeroVector,PredictedFittedPelvis=FVector::ZeroVector;
    float MaximumBodySpeedCmS=0,MaximumAngularSpeedRadS=0,MaximumJointSeparationCm=0;
    float MaximumPelvisTravelCm=0,MaximumPelvisRiseCm=0,MaximumSettledDriftCm=0;
    float CurrentBodySpeedCmS=0,CurrentAngularSpeedRadS=0;
    // Diagnostic readback only; these fields never grant death settlement.
    bool bProneAtHit=false,bInitialNativeGroundRead=false;
    float InitialPelvisTerrainClearanceCm=MAX_flt,InitialChestTerrainClearanceCm=MAX_flt;
    FString ActualEntryClip;
    int32 BodyCount=0,ConstraintCount=0,VelocityLimitEvents=0,Frames=0;
    FVector InitialPelvis=FVector::ZeroVector,SettledPelvis=FVector::ZeroVector;
    FVector IncomingDirection=FVector::ZeroVector;
    FVector InheritedVelocity=FVector::ZeroVector;
};
