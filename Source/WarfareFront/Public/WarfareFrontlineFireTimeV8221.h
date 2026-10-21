#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;

// Exact disposable V8221 review only; creates and retires its own real fire actor.
void RunFireTimeChecksV8221(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);

