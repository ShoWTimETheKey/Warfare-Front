#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
TSharedRef<FJsonObject> RunFrontlineArmourMotionV8311Review(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
