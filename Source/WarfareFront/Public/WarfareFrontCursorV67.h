#pragma once
#include "CoreMinimal.h"
class APlayerController;

enum class EFrontlineCursorV67 : uint8 { Idle,Hover,Target,Disabled };
namespace FrontlineCursorV67
{
    // Uses viewport-local hardware cursors. The OS cursor is restored on exit
    // from the client area by UGameViewportClient's normal MouseLeave path.
    WARFAREFRONT_API bool Install(APlayerController* Player);
    WARFAREFRONT_API void SetState(APlayerController* Player,EFrontlineCursorV67 State);
}
