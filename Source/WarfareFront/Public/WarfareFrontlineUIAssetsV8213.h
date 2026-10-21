#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class FJsonObject;

// Game-thread UI image lifecycle. Requests never wait for asset I/O or RHI work.
// Exact authored object paths and the imported texture settings are retained.
namespace FrontlineUIAssetsV8213
{
    WARFAREFRONT_API bool BaselineMode();
    WARFAREFRONT_API void PreloadCore();
    WARFAREFRONT_API void PreloadTextures(const TArray<FString>& Paths);
    WARFAREFRONT_API UTexture2D* Texture(const FString& Path);
    // Settled includes load/RHI failures confirmed after a nonblocking resource
    // initialization fence, so startup can present its error page. Ready excludes them.
    WARFAREFRONT_API bool PreloadSettled();
    WARFAREFRONT_API bool PreloadReady();
    WARFAREFRONT_API TSharedRef<FJsonObject> Summary();
}
