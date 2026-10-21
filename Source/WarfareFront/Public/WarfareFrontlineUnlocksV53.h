#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"

class AWarfareFrontlineGameMode;

namespace FrontlineUnlocksV53
{
// Completed campaign orders can advance while the preceding action is still
// visible. Keep that action's roster until the player returns to preparation.
int32 Stage(const AWarfareFrontlineGameMode* Battle,int32 Team=0);
int32 IntroductionFirstOperation(const AWarfareFrontlineGameMode* Battle);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
bool PrepareVictoryReview(AWarfareFrontlineGameMode* Battle,EFrontlineFaction Faction,int32 Operation=0);
}
