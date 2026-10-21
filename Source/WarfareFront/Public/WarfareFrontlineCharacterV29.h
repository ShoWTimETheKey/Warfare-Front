#pragma once
#include "CoreMinimal.h"
class USkeletalMesh;
namespace FrontlineCharacterV29
{
    // Accepted source remains the fallback and can be selected for an isolated comparison.
    USkeletalMesh* Load(const FString& OriginalPath);
}
