#pragma once
#include "CoreMinimal.h"
namespace FrontlineMovementV28
{
// Continuous translation target: the old 400cm switch removed almost half the
// speed in open ground. Only the final foot placement brakes for arrival.
inline float TravelSpeed(float Remaining,float Pressure,bool CanRun,float StopDistance=8.f)
{
    // Pressure affects observation and firing. Movement retains its ordinary
    // gait; terrain, carried equipment and arrival braking own pace.
    const float Cruise=CanRun?390.f:210.f;
    return FMath::Min(Cruise,FMath::Sqrt(FMath::Max(0.f,2.f*950.f*(Remaining-StopDistance))));
}
}
