#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;

// Isolated, finite balance experiment. Uses native world/actor ticking and the
// shipped recruitment, AI, combat and victory rules; it is not a combat model.
// Returns true only when -WFV52Balance owns this process. Do not start other
// review runners in the same process when true is returned.
WARFAREFRONT_API bool StartFrontlineBalanceV52(AWarfareFrontlineGameMode* Battle);
