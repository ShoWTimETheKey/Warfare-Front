#include "WarfareGrenadeReleaseNotify.h"
#include "WarfareFrontline.h"
#include "Components/SkeletalMeshComponent.h"

void UWarfareGrenadeReleaseNotify::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp,Animation,EventReference);
    if (MeshComp) if (auto* Man=Cast<AWarfareFrontlineSoldier>(MeshComp->GetOwner())) Man->ReleaseGrenade();
}
