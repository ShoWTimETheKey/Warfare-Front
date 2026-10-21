#pragma once
#include "CoreMinimal.h"
class UAnimationAsset;
namespace FrontlineMotionV34
{
    /** Same authored contact/event timing; repaired anatomical arm poses. */
    UAnimationAsset* Load(const FString& OriginalPath);
    UAnimationAsset* Flame(const TCHAR* Phase);
}
