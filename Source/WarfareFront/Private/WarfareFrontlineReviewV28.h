#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class FJsonObject;
TSharedRef<FJsonObject> RunFrontlineV28Checks(AWarfareFrontlineGameMode* Battle);
void StartFrontlineV28Review(AWarfareFrontlineController* Controller);
