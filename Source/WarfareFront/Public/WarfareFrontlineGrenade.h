#pragma once

#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;

// Game-scale grenade tuning. No target actor is retained after EstimateAim.
namespace FrontlineGrenade
{
inline constexpr float MaxRangeCm=1848.f;
// Extending reach must not silently tighten throws at the former distances.
inline constexpr float TrajectoryReferenceRangeCm=1320.f;
inline constexpr float BlastRadiusCm=275.f;
inline constexpr float FullExposureRadiusCm=40.f;
inline constexpr float MinFlightSeconds=1.45f;
inline constexpr float FlightRangeSeconds=.45f;
inline constexpr float BaseScatterCm=65.f;
inline constexpr float RangeScatterCm=160.f;
inline constexpr float PressureScatterCm=95.f;
inline constexpr float TargetMovementScatterCm=55.f;
inline constexpr float ThrowerMovementScatterCm=40.f;
inline constexpr float ObscuredScatterCm=70.f;
inline constexpr float CoverScatterCm=30.f;
inline constexpr float MaxScatterCm=420.f;
inline constexpr float CrossRangeScale=.75f;
// V27: half the previous readiness rate; no per-frame repeated coin toss.
inline constexpr float ReadinessRateMultiplier=.5f;
// Half precision is represented by twice the linear dispersion, including its cap.
inline constexpr float AimScatterMultiplier=2.f;

float FlightSeconds(float RangeCm);
float HitChanceFalloff(float DistanceCm,float RadiusCm);
FVector EstimateAim(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target);
float BlastExposure(const AWarfareFrontlineGameMode* Battle,const FVector& Position,const AWarfareFrontlineSoldier* Unit);
}
