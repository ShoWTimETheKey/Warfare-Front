#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
namespace FrontlineNewUnlockV8215
{
struct FCard{int32 Kind=0,Index=0;FString Title,Description;};
void Capture(AWarfareFrontlineGameMode* Battle,int32 WonMission);
const TArray<FCard>& Cards(const AWarfareFrontlineGameMode* Battle);
bool Pending(const AWarfareFrontlineGameMode* Battle);
/** Release the captured next-assignment cards only after the report returns to operations. */
bool ReleaseAfterReportV8217(AWarfareFrontlineGameMode* Battle);
void Dismiss(AWarfareFrontlineGameMode* Battle);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
