#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class FJsonObject;

namespace FrontlineTrenchMotionV8444
{
enum class EBankInvalidation : uint8 { None, GoalChanged, AssignmentChanged, BayUnvalidated, OutsideLocalScope, WaypointReached, AttemptStalled };
struct FBankRouteEvidence
{
    bool Active=false;
    uint32 Decision=0;
    int32 Trench=INDEX_NONE;
    FVector Goal=FVector::ZeroVector,Waypoint=FVector::ZeroVector,ProgressFeet=FVector::ZeroVector,LastAttemptGoal=FVector::ZeroVector;
    float AttemptAt=0.f,StalledSeconds=0.f,TotalAttemptSeconds=0.f;
    EBankInvalidation LastInvalidation=EBankInvalidation::None;
    uint32 InvalidatedDecision=0;
    FVector InvalidatedGoal=FVector::ZeroVector,ReplacementGoal=FVector::ZeroVector;
    float InvalidatedAt=0.f,InvalidatingAttemptSeconds=0.f;
    bool HeldAtInvalidation=false,ValidatedAtInvalidation=false;
};
// Read-only evidence: it neither refreshes the local field nor requests a route.
bool ReadBankRoute(const AWarfareFrontlineSoldier* Unit,FBankRouteEvidence& Out);
TSharedRef<FJsonObject> RunChecks(AWarfareFrontlineGameMode* Battle,
    TFunctionRef<void(const FString&,bool)> Check);
}
