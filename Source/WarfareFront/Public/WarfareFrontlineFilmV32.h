#pragma once

struct FPostProcessSettings;
namespace FrontlineFilmV32
{
    bool IsEnabled();
    void SetEnabled(bool bEnabled,bool bSave=true);
    void Apply(FPostProcessSettings& Settings);
}
