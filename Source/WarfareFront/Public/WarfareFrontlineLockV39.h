#pragma once
#include "CoreMinimal.h"
class UCanvas;
class UTexture2D;
class UWorld;
class FJsonObject;

// One painted atlas sample and one coverage equation for both SDR and HDR UI.
namespace FrontlineLockV39
{
    void Draw(UCanvas* Canvas,UTexture2D* Atlas,const FVector2D& Position,const FVector2D& Size);
    TSharedRef<FJsonObject> RunCoverageChecks(UWorld* World);
}
