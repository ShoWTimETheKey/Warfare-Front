#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class UAnimationAsset;
class FJsonObject;

enum class EFrontlineIdleIntentV38 : uint8 { None, NearWatch, FarWatch, SettleLoad, ChinTuck };

/** Plain snapshot: no UObject/world access in worker-side pose evaluation. */
struct FFrontlineIdlePoseV38
{
    FRotator Torso=FRotator::ZeroRotator,Head=FRotator::ZeroRotator;
    float Weight=0;
    EFrontlineIdleIntentV38 Intent=EFrontlineIdleIntentV38::None;
    bool IsActive() const { return Weight>.001f; }
};

/** Owned by one animation proxy. Randomness never consumes combat RNG. */
struct FFrontlineIdleStateV38
{
    FRandomStream Random{381917};
    uint32 Identity=0;
    bool Initialized=false,WasEligible=false,Returning=false;
    float Wait=0,Age=0,Duration=0,Attack=.8f,Release=.8f,ReturnWeight=0;
    int32 GesturesStarted=0;
    EFrontlineIdleIntentV38 LastIntent=EFrontlineIdleIntentV38::None;
    FFrontlineIdlePoseV38 Peak,Current;
};

namespace FrontlineMotionV38
{
    /** Game-thread call from animation PreUpdate, before its pose snapshot. */
    FFrontlineIdlePoseV38 UpdateIdle(FFrontlineIdleStateV38& State,const AWarfareFrontlineSoldier* Unit,const UAnimationAsset* CurrentAsset,float DeltaSeconds);
    /** Isolated actual actor/component checks; the caller owns runtime captures. */
    TSharedRef<FJsonObject> RunFocusedChecks(AWarfareFrontlineGameMode* Battle);
    bool PrepareIdleReview(AWarfareFrontlineGameMode* Battle);
    void TickIdleReview(AWarfareFrontlineGameMode* Battle,float DeltaSeconds);
}
