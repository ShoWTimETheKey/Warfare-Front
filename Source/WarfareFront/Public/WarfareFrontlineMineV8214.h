#pragma once
#include "CoreMinimal.h"

namespace FrontlineMineV8214
{
inline constexpr float TriggerRadiusCm=155.f;
inline constexpr float BlastRadiusCm=600.f;
inline constexpr float FullImpactRadiusCm=175.f;
inline constexpr float WorkStandOffCmV833=320.f;
inline constexpr float WorkReachCmV833=390.f;
inline constexpr float SearchFrontageCmV833=400.f;
inline constexpr float WorkSecondsV833=4.f;

inline float HitChanceFalloff(float DistanceCm,float RadiusCm=BlastRadiusCm)
{
    if(!FMath::IsFinite(DistanceCm)||!FMath::IsFinite(RadiusCm)||DistanceCm<0.f||RadiusCm<=0.f||DistanceCm>=RadiusCm)return 0.f;
    const float Core=FMath::Min(FullImpactRadiusCm,RadiusCm);
    if(DistanceCm<=Core)return 1.f;
    const float T=(DistanceCm-Core)/(RadiusCm-Core);
    return 1.f-T*T*(3.f-2.f*T);
}
}
