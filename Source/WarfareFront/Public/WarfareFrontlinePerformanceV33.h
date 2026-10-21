#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
// Opt-in, isolated, self-terminating real-time measurement. Never called for a player session.
WARFAREFRONT_API void StartFrontlineV33Performance(AWarfareFrontlineGameMode* Battle);
