#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
// Root's single isolated -WFV35Review serial runner. Does not advance the world
// or write saves; creates and retires only its own explicit combat fixtures.
void RunV35CombatChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
// Aged production smoke/gas volumes for one normal-camera inspection. This is
// explicitly a cosmetic fixture; it is not a certification of arrival timing.
bool SetupV35CombatVisualFixture(AWarfareFrontlineGameMode* Battle);
