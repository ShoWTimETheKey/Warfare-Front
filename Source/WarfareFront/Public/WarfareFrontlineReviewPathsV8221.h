#pragma once
#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

// Keep review roots exact while avoiding personal directory literals in Shipping.
// Callers supply only the original fixed suffixes; these are never user inputs.
namespace FrontlineReviewPathsV8221
{
inline FString HomePath(const TCHAR* ExactSuffix)
{
    FString Home=FPlatformProcess::UserHomeDir();
    FPaths::NormalizeFilename(Home);
    // This invalid extended-device root is nonempty and absolute, so an
    // unavailable home cannot accidentally turn a prefix guard into a match.
    if(Home.IsEmpty()||FPaths::IsRelative(Home))
        return TEXT("//?/WarfareFrontInvalidReviewRootV8221/");
    FString Path=FPaths::ConvertRelativePathToFull(FPaths::Combine(Home,ExactSuffix));
    FPaths::NormalizeFilename(Path);
    if(FString(ExactSuffix).EndsWith(TEXT("/"))&&!Path.EndsWith(TEXT("/")))Path+=TEXT("/");
    return Path;
}
}
