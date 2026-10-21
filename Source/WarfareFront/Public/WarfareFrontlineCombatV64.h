#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineCombatV64
{
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
TSharedRef<FJsonObject> RunAntiArmourChecksV833(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
TSharedRef<FJsonObject> RunNo44RotationChecksV8310(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
