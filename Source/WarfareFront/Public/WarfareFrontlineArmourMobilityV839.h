#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
// Strict disposable UK/DE fixture. Calls the production fleet clock and
// real actors on the currently prepared terrain; never changes map content.
TSharedRef<FJsonObject> RunFrontlineArmourMobilityV839Review(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
