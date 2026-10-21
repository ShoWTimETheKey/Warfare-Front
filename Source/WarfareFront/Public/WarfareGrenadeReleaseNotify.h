#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WarfareGrenadeReleaseNotify.generated.h"

/** Release on the authored hand gesture, through the animation event queue. */
UCLASS()
class WARFAREFRONT_API UWarfareGrenadeReleaseNotify : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
};
