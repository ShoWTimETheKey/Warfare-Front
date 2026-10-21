#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Focused, isolated review only. Begin after selecting Chinese, render the real
// battle HUD and localized tooltips, then End without changing the language.
namespace FrontlineHUDV45
{
    void BeginAudit();
    TSharedRef<FJsonObject> EndAudit();
}
