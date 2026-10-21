#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
namespace FrontlineCombatV63
{
    // Disposable production-actor regressions, called by the existing review runner.
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
