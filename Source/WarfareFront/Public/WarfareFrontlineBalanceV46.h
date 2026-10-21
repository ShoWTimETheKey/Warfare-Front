#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
namespace FrontlineBalanceV46
{
    // Synchronous isolated fixture; restores the encounter and never writes saves.
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
