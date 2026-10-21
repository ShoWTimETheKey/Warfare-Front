#pragma once

#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
enum class EFrontlineFaction : uint8;
enum class EFRRUNationV81 : uint8;

// Presentation identity only. It never selects an army or changes a campaign.
namespace FrontlineConsoleV79
{
    WARFAREFRONT_API EFrontlineFaction ResolveDisplayFaction(const AWarfareFrontlineGameMode* Battle,bool bHome);
    WARFAREFRONT_API EFRRUNationV81 ResolveDisplayNationV82(const AWarfareFrontlineGameMode* Battle,bool bHome);
    // Pure policy fixtures: no actors, archive reads, writes or gameplay changes.
    WARFAREFRONT_API void RunPolicyChecks(TFunctionRef<void(const FString&,bool)> Check);
}
