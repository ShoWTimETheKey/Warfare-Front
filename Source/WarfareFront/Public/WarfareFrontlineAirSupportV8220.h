#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;

namespace FrontlineAirSupportReviewV8220
{
    // Shipping keeps this evidence independent of compiled-out UE_LOG calls.
    bool AppendMetric(AWarfareFrontlineGameMode* B,const TCHAR* Group,TSharedRef<FJsonObject> Metric);
}

// Both entries require the exact disposable V8220 review profile.
void RunAirSupportChecksV8220(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
bool SetupAirSupportCaptureV8220(AWarfareFrontlineGameMode* B,int32 Mode);

namespace FrontlineSupportV32
{
    void RunAirBombingChecksV8220(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check,TFunctionRef<void()> Resolve);
    bool SetupAirBombingCaptureV8220(AWarfareFrontlineGameMode* B);
}
