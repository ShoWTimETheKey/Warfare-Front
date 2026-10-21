#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WarfareFrontlineAppearanceV29.generated.h"

class AWarfareFrontlineSoldier;
class USkeletalMesh;
class USkeletalMeshComponent;

/** Small expressions on the authored face shell, independent for every soldier. */
UCLASS()
class WARFAREFRONT_API UFrontlineAppearanceV29 : public UActorComponent
{
    GENERATED_BODY()
public:
    UFrontlineAppearanceV29();
    void InitializeAppearance(AWarfareFrontlineSoldier* InSoldier);
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    int32 GetIdentitySeed() const { return IdentitySeed; }
    bool HasAuthoredExpressions() const { return bHasMorphs; }
    int32 GetCompletedBlinkCount() const { return CompletedBlinks; }
    float GetNextBlinkTime() const { return NextBlink; }
private:
    UPROPERTY(Transient) TObjectPtr<AWarfareFrontlineSoldier> OwnerSoldier;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> BoundAsset;
    FRandomStream FaceRandom;
    int32 IdentitySeed=0,CompletedBlinks=0;
    float Age=0,NextBlink=1, BlinkStarted=-100, BlinkDuration=.18f;
    float RestLid=.08f, RestBrow=.2f, Lean=.3f, Broad=.3f;
    float BrowWeight=0,JawWeight=0,DeathAge=0;
    bool bHasMorphs=false,bWasVisible=false;
    FName ResolvedNames[6];
    void BindMesh(USkeletalMeshComponent* Mesh);
    void Apply(USkeletalMeshComponent* Mesh,int32 Channel,float Weight);
};

/** Call once at the end of the soldier's equipment/animation initialization. */
WARFAREFRONT_API void InitializeFrontlineAppearanceV29(AWarfareFrontlineSoldier* Soldier);
/** Preserve the individual's head when the visible casualty follower is created. */
WARFAREFRONT_API void InitializeFrontlineCasualtyAppearanceV29(USkeletalMeshComponent* Source,USkeletalMeshComponent* Follower);
