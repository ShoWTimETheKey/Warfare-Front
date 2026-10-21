#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;

// Synchronous opt-in fixture. Restores campaign/research records and leaves an
// empty briefing; the root review owns subsequent screenshots and shutdown.
TSharedRef<FJsonObject> RunFrontlineV26CombatChecks(AWarfareFrontlineGameMode* Battle);
