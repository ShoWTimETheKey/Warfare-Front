#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineFXV47
{
/** Load and retain the small shared FX assets during existing preparation. */
void Prewarm();
/** Actual production profiles at a reproducible age; isolated review only. */
bool SetupTemporalFixture(AWarfareFrontlineGameMode* Battle,float AgeSeconds);
TSharedRef<FJsonObject> Describe(AWarfareFrontlineGameMode* Battle);
void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
}
