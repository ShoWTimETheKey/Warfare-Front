#include "WarfareFrontlineFilmV32.h"
#include "Engine/Scene.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace FrontlineFilmV32
{
namespace { bool bLoaded=false,bGrain=true; }
bool IsEnabled()
{
    if(!bLoaded)
    {
        bLoaded=true;
        if(GConfig)GConfig->GetBool(TEXT("Warfare.Presentation"),TEXT("FilmGrain"),bGrain,GGameUserSettingsIni);
    }
    return bGrain;
}
void SetEnabled(bool bEnabled,bool bSave)
{
    bLoaded=true;bGrain=bEnabled;
    if(bSave && GConfig && !FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {
        GConfig->SetBool(TEXT("Warfare.Presentation"),TEXT("FilmGrain"),bGrain,GGameUserSettingsIni);
        GConfig->Flush(false,GGameUserSettingsIni);
    }
}
void Apply(FPostProcessSettings& S)
{
    // The native tonemapper modulates scene luminance before the separately
    // composed HUD. No translucent full-screen cards, no noise on type.
    S.bOverride_FilmGrainIntensity=true;S.FilmGrainIntensity=IsEnabled()?.018f:0.f;
    S.bOverride_FilmGrainIntensityShadows=true;S.FilmGrainIntensityShadows=.65f;
    S.bOverride_FilmGrainIntensityMidtones=true;S.FilmGrainIntensityMidtones=.70f;
    S.bOverride_FilmGrainIntensityHighlights=true;S.FilmGrainIntensityHighlights=.16f;
    S.bOverride_FilmGrainShadowsMax=true;S.FilmGrainShadowsMax=.10f;
    S.bOverride_FilmGrainHighlightsMin=true;S.FilmGrainHighlightsMin=.70f;
    S.bOverride_FilmGrainHighlightsMax=true;S.FilmGrainHighlightsMax=3.f;
}
}
