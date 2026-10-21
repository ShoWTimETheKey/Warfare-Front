#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
namespace FrontlineCombatV38
{
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
