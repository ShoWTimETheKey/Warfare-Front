#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineBattlefield.h"

namespace FrontlinePacingV24
{
inline constexpr float TempoIntervalScale=1.f/1.15f;
inline constexpr float BaseRecruitScale=.85f*TempoIntervalScale;
inline constexpr float GrenadeReadinessRate=.70f;
// One operation-relative boundary is shared by camera, ingress and combat.
// The old fixed 14,350 cm authoring spawn left shortened maps with a visible but
// untargetable rear approach and made reinforcement travel take half a minute.
inline float CombatHalfLength() { return FrontlineBattlefield::GoalHalfLength+1100.f; }
inline constexpr float IngressSpeedCm=390.f;
inline constexpr float IngressTravelSeconds=2.45f;
inline float ReinforcementStartX(int32 Direction,int32 Member,float PersonalPhase)
{
    return -Direction*(CombatHalfLength()+IngressSpeedCm*IngressTravelSeconds+Member*110.f+FMath::Clamp(PersonalPhase,0.f,1.f)*25.f);
}
// Retain the old review-call signature; camera arguments have no simulation role.
inline float ReinforcementStartX(int32 Direction,float CameraX,float CameraWidth,int32 Member,float PersonalPhase)
{
    return ReinforcementStartX(Direction,Member,PersonalPhase);
}
inline bool HasCrossedEntry(float X,int32 Direction)
{ return FMath::IsFinite(X) && X*Direction>=-CombatHalfLength(); }
}
