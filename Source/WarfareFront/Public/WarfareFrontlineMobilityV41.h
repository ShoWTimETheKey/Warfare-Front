#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineMobilityV41
{
    // Returns feet in world centimetres. Local avoidance never adds travel or
    // a reverse advance; it does not replace sight/contact or trench orders.
    // Actual steering time, never time since a previous call. Untimed review
    // projections keep the compatibility default and cannot age a stuck route.
    // Tick supplies RequiredFacing to turn toward the physically checked leg
    // before walking it. An opposed body keeps its actual feet until aligned;
    // the resolver never publishes that rejected candidate into the body field.
    FVector Step(AWarfareFrontlineSoldier* Unit,const FVector& Destination,float Distance,bool Advancing,float AttemptSeconds=0.f,FVector* RequiredFacing=nullptr);
    bool ArmourYieldDestinationV8218(AWarfareFrontlineSoldier* Unit,FVector& Destination);
    // Editor/NoSave/private MG lease review only. Reads the existing map without
    // selecting, refreshing, revoking or progressing a lease or physical route.
    TSharedPtr<FJsonObject> DescribeArmourYieldLeaseV87(const AWarfareFrontlineSoldier* Unit);
    // Validate the complete accepted step, including its actual support/fall
    // height, against live hulls before publishing a new infantry position.
    bool AllowsSupportedPlacementV8448(const AWarfareFrontlineSoldier* Unit,const FVector& From,const FVector& To);
    // Conservative broad phase over the existing live body field. Every bucket
    // in the expanded segment bounds is visited; the caller owns its exact
    // body/state predicates and may stop by returning false.
    bool VisitSweptBodies(const AWarfareFrontlineSoldier* Unit,const FVector& From,const FVector& To,double Clearance,
        TFunctionRef<bool(const AWarfareFrontlineSoldier*)> Visit);
    void InvalidateRoster(AWarfareFrontlineGameMode* Battle);
    void TraceNextStepV64(AWarfareFrontlineSoldier* Unit);
    void Reset(AWarfareFrontlineGameMode* Battle);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    // Pure tactical-route checks; V54's isolated review can run these without
    // spawning soldiers or altering campaign progression.
    void RunTerrainRouteChecksV54(TFunctionRef<void(const FString&,bool)> Check);
}
