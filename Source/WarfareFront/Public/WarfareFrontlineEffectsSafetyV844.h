#pragma once
#include "CoreMinimal.h"
class AActor;
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineEffectsSafetyV844
{
    // Game-thread validation of the currently bound transient proxies. This
    // does not claim a render-thread fallback read or a DLSS crash diagnosis.
    void Guard(AActor* Effect);
    TSharedRef<FJsonObject> Describe(AActor* Effect);
    TSharedRef<FJsonObject> RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    // Six persistent real actors, revisited over GPU frames by the host review.
    // Stage 0: new bursts, 1: rebind, 2: .9s tick, 3: large-delta cleanup,
    // 4: reconfigure after cleanup, 5: advance the rebuilt burst.
    bool SetupFixture(AWarfareFrontlineGameMode* Battle,int32 Stage);
    bool SetFixtureLiveV844(AWarfareFrontlineGameMode* Battle,bool Live);
    TSharedRef<FJsonObject> DescribeFixture(AWarfareFrontlineGameMode* Battle);
    void ClearFixture(AWarfareFrontlineGameMode* Battle);
}
