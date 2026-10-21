#pragma once
#include "CoreMinimal.h"

namespace FrontlineColourV73
{
// Display-authored artwork only. Mirrors the linear-sRGB UI material; the
// native startup bitmap has no scene tonemapper or HDR luminance to compensate.
inline FLinearColor Painting(const FLinearColor& Source)
{
    const float Y=.2126f*Source.R+.7152f*Source.G+.0722f*Source.B;
    const float Warm=FMath::SmoothStep(.012f,.12f,Source.R-Source.B)*FMath::SmoothStep(.004f,.08f,Source.R-Source.G);
    const float Chroma=.80f+.12f*Warm;
    const float Zone=(1-FMath::SmoothStep(.05f,.52f,Y))*FMath::Min(Y,.12f)*.12f;
    constexpr float BiasY=-.12f*.2126f+ .05f*.7152f+.18f*.0722f;
    return FLinearColor(FMath::Max(0.f,Y+(Source.R-Y)*Chroma+(-.12f-BiasY)*Zone),
        FMath::Max(0.f,Y+(Source.G-Y)*Chroma+(.05f-BiasY)*Zone),
        FMath::Max(0.f,Y+(Source.B-Y)*Chroma+(.18f-BiasY)*Zone),Source.A);
}
}
