#pragma once
#include "CoreMinimal.h"

namespace FrontlineUpdatesV36
{
enum class EStatus : uint8 { Idle, Checking, Current, Available, Offline, RateLimited };
struct FState
{
    EStatus Status=EStatus::Idle;
    FString LatestTag,CheckedUtc;
};
WARFAREFRONT_API const FState& Get();
WARFAREFRONT_API const TCHAR* CurrentTag();
WARFAREFRONT_API void Check();
WARFAREFRONT_API bool OpenReleasePage();
// Uses the same parser as the network response; no network or player writes.
WARFAREFRONT_API void RunChecks(TFunctionRef<void(const FString&,bool)> Check);
}
