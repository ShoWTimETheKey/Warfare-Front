#pragma once
#include "CoreMinimal.h"

// Generated from ArtSource/UI/V85/Medals/repeat-policy.json.
// Per-award game boundaries use sourced Great War forms, not statutory maxima.
namespace FrontlineMedalsV8215
{
// Physical renderer capacity also covers unrelated devices and six real classes.
inline constexpr int32 MaximumRepeatsV85=5;
inline int32 RepeatLimitV85(int32 Index)
{
    switch(Index)
    {
    case 0:return 2; // uk.distinguished_conduct
    case 1:return 3; // uk.military_cross
    case 2:return 1; // uk.victoria_cross
    case 12:return 3; // uk.distinguished_service_order
    case 13:return 3; // uk.military_medal
    case 37:return 2; // uk.distinguished_service_cross
    case 38:return 2; // uk.distinguished_flying_cross
    case 39:return 1; // uk.distinguished_flying_medal
    default:return 0;
    }
}
inline int32 PresentedRepeatsV85(int32 Index,int32 Recorded)
{return FMath::Clamp(Recorded,0,RepeatLimitV85(Index));}
}
