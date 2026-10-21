#pragma once

#include "CoreMinimal.h"

// Battlefield rules sample the same authored landform masks as the visible
// terrain. These are local effects, not whole-operation difficulty modifiers.
namespace FrontlineTerrainTacticsV54
{
    WARFAREFRONT_API float InfantryTravelScale(const FVector& Position);
    WARFAREFRONT_API float VehicleTravelScale(const FVector& Position);
    // Called only after physical sight/range checks. Moving men get less shelter.
    WARFAREFRONT_API float DirectFireHitScale(const FVector& Shooter,const FVector& Target,bool bTargetMoving);
    WARFAREFRONT_API void RunChecks(TFunctionRef<void(const FString&,bool)> Check);
}
