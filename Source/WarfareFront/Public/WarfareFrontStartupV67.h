#pragma once

#include "CoreMinimal.h"

// Native launch artwork, followed by a render-acknowledged handoff to the menu.
// No UObject/assets are required before the engine finishes initialization.
namespace FrontlineStartupV67
{
    struct FWindowDiagnostics
    {
        bool SplashObserved=false,SplashExcludedFromTaskbar=false,GameObserved=false;
        bool GameShellStylePreserved=false,InputRestored=false,GameUncloaked=false;
        bool GameForegroundV75=false;
        bool FocusCompletedV8310=false,FocusCancelledV8310=false;
        bool SplashInitiallyForegroundV845=false,SplashForegroundBeforeCloseV845=false,ForegroundRequestSucceededV845=false;
        bool GameCoversMonitorV8310=false,TaskbarAboveGameV8310=false;
        int32 ForegroundRequestsV8310=0;
        bool SplashFactionVerifiedV79=false;
        int32 SplashFactionV79=0;
        int32 MaximumTaskbarEligibleWindows=0;
    };
    WARFAREFRONT_API void Begin();
    WARFAREFRONT_API void AttachSlate();
    WARFAREFRONT_API void NotifyMainMenuDrawn(bool bArtworkAndFontsReady);
    WARFAREFRONT_API void Shutdown();
    WARFAREFRONT_API bool IsWaitingForMenu();
    WARFAREFRONT_API FWindowDiagnostics WindowDiagnostics();
}
