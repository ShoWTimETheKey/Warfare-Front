#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class FJsonObject;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;
class UStaticMeshComponent;

/** Explicit accepted derivative inputs. Empty/unaccepted descriptors never load assets.
 * Native65 and the old67 sources stay separate. No guessed 68-bone contract or rest conversion.
 */
namespace FrontlineTripoCharacterV87
{
    struct FDescriptor
    {
        FString Id, CandidateRoot, AcceptanceReceipt;
        bool bSkinAccepted=false, bDerivedRigAccepted=false, bWeaponFrameAccepted=false;
        FString Body, Skeleton, BasePhysics, CasualtyBody, DetachedLimb, DroppedWeapon;
        FName CasualtyCutBone=NAME_None;
        TArray<FName> ExpectedBoneNames;
        TArray<int32> ExpectedParentIndices;
        FTransform MeshRelativeTransform=FTransform::Identity;
        // Idle/Aim/Reload are required for the first article. Other legacy actions
        // need their own entry before they can execute on this independent skeleton.
        TMap<FName,FString> Motions;
    };

    bool Allowed(const AWarfareFrontlineGameMode* Battle);
    bool IsBound(const AWarfareFrontlineSoldier* Unit);
    TSharedRef<FJsonObject> Bind(AWarfareFrontlineSoldier* Unit,const FDescriptor& Descriptor,
        TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<FJsonObject> Restore(AWarfareFrontlineSoldier* Unit,
        TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<FJsonObject> Describe(const AWarfareFrontlineSoldier* Unit);

    /** Empty registry/Shipping: preserve the exact old pointer. Bound but unsupported:
     * return null, never play an old-skeleton clip on the candidate body.
     */
    UAnimationAsset* RemapMotion(const AWarfareFrontlineSoldier* Unit,UAnimationAsset* Legacy);
    /** Recover old classification identity for stride/rate/fire/fatal/gesture logic. */
    UAnimationAsset* LegacyEquivalent(const AWarfareFrontlineSoldier* Unit,UAnimationAsset* Asset);
    UAnimationAsset* Motion(const AWarfareFrontlineSoldier* Unit,FName Semantic);
    bool IsSemantic(const AWarfareFrontlineSoldier* Unit,const UAnimationAsset* Asset,FName Semantic);
    USkeletalMesh* CasualtyBody(const AWarfareFrontlineSoldier* Unit);
    /** The first article supplies one actual matching missing-segment/fragment pair.
     * It cannot fall back to an old67 casualty or advertise all limb variants.
     */
    bool ResolveCasualty(const AWarfareFrontlineSoldier* Unit,USkeletalMesh*& Body,UStaticMesh*& Limb,FName& CutBone);
    /** True means this registry owns the request, even if the validated drop cannot execute. */
    bool TryReleaseWeapon(AWarfareFrontlineSoldier* Unit,bool Explosive,const FVector& ForceDirection,
        UStaticMeshComponent*& Out);

    /** Requires the real getter/SetMotion hooks and a caller-created disposable
     * UK NCO + clear forward enemy in the existing scoped combat review.
     * Runs actual StartReload/Actor Tick; no pose-only substitution.
     */
    TSharedRef<FJsonObject> RunAimReloadChecks(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Enemy,
        TFunctionRef<void(const FString&,bool)> Check);
}
