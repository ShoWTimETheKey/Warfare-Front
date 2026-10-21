#pragma once
#include "CoreMinimal.h"
class USkeletalMesh;
class UStaticMeshComponent;
class AWarfareFrontlineSoldier;
namespace FrontlineCharacterV34
{
    // Returns only the new asset. Callers preserve their original fallback.
    USkeletalMesh* TryLoad(const FString& OriginalPath);
    UStaticMeshComponent* ReleaseWeapon(AWarfareFrontlineSoldier* Unit,bool Explosive,const FVector& ForceDirection);
    void ConsumeReleasedWeapon(AWarfareFrontlineSoldier* Unit);
}
