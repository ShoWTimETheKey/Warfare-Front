#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "WarfareFrontlineAnimation.generated.h"

/** Per-instance observations from the actual animation evaluation, for focused character review. */
struct FFrontlineAnimationV29Diagnostics
{
    int32 EvaluatedFrames=0,PersonalLayerFrames=0,InterruptedTransitions=0,SnapshotRemaps=0;
    int32 InvalidBoneFrames=0,UnsafeTorsoHierarchyFrames=0,PoleFallbackFrames=0;
    int32 TerrainFootFrames=0;
    int32 InertialBlendFramesV65=0,BurningDistressFramesV65=0;
    int32 GasDistressFramesV8215=0,HeavyRecoilFramesV8215=0;
    float MaximumInertialAngleV65=0;
    float MaximumPersonalGripDriftCm=0,MaximumArmLengthErrorCm=0,LastPersonalLeanDegrees=0;
    float MaximumTerrainLegLengthErrorCm=0;
    int32 TrenchReloadFramesV8447=0,TrenchReloadRejectedFramesV8447=0;
    float LastTrenchReloadShelterV8447=0,LastTrenchReloadLowerCmV8447=0;
    float MaximumTrenchReloadFootDriftCmV8447=0,MaximumTrenchReloadGripDriftCmV8447=0,MaximumTrenchReloadLegLengthErrorCmV8447=0;
    int32 MountedReloadFramesV8447=0,MountedReloadReducedFramesV8447=0;
    float MaximumMountedReloadWeaponAnchorDriftCmV8447=0,MaximumMountedReloadBoltAnchorDriftCmV8447=0,MaximumMountedReloadArmLengthErrorCmV8447=0;
    FVector LastMountedReloadWeaponAnchorCSV8447=FVector::ZeroVector;
    int32 OverheadPoseFramesV45=0,OverheadRejectedFramesV45=0;
    float MaximumOverheadGripErrorCmV45=0,MaximumOverheadShoulderDegreesV45=0,MaximumOverheadElbowDegreesV45=0;
    int32 NationalCarryGripFramesV82=0,NationalCarryGripRejectedFramesV82=0;
    float MaximumNationalCarryGunShiftCmV82=0,MaximumNationalBothGripResidualCmV82=0;
    int32 NationalGroundGunFramesV82=0,NationalGroundGunRejectedFramesV82=0;
    float MaximumNationalGroundGunShiftCmV82=0,MaximumNationalGroundGunTiltDegreesV82=0;
    float MaximumNationalGroundFootErrorCmV82=0,MaximumNationalGroundGripErrorCmV82=0;
    // Contribution of actual walk/run endpoints, including snapshot transitions.
    float LocomotionPoseWeightV46=0;
};

/** Plays the approved clips with a brief transition from the last displayed pose. */
UCLASS(Transient, NotBlueprintable)
class WARFAREFRONT_API UWarfareFrontlineAnimation : public UAnimSingleNodeInstance
{
    GENERATED_BODY()
public:
    void TransitionTo(UAnimationAsset* Asset,bool bLoop,float Rate,float StartPosition,float BlendSeconds);
    int32 GetGripConstraintFrames();
    float GetMaximumGripResidualCm();
    float GetMaximumGripCorrectionCm();
    FFrontlineAnimationV29Diagnostics GetCharacterDiagnostics();
    void FreezeForRagdoll();
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};
