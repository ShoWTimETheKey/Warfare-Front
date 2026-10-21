#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
namespace FrontlineWaterReviewV83
{
enum class EContribution:uint8 { Baseline,Hidden,NoReflection,Candidate,Restore };
// Existing scene only. Root holds one camera at Focus for all four images.
bool SetContribution(AWarfareFrontlineGameMode* Battle,EContribution Mode,FVector& Focus);
FString Describe(AWarfareFrontlineGameMode* Battle);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
