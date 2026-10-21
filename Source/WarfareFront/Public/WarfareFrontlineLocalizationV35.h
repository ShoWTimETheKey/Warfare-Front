#pragma once
#include "CoreMinimal.h"

// Authored whole messages and explicit printf-style message templates.
// Gameplay identifiers, saves and voice-bank selection never depend on locale.
namespace FrontlineLocalizationV35
{
    enum class ELanguage : uint8 { English, Chinese };
    ELanguage GetLanguage();
    bool SetLanguage(ELanguage Language);
    FString Text(const FString& Source);
    FString English(const FString& Source);
    const TCHAR* Literal(const TCHAR* Source);
    bool WriteCoverageReport();
    void RunFormationNameChecksV70(TFunctionRef<void(const FString&,bool)> Check);
    void RunCopyChecksV66(TFunctionRef<void(const FString&,bool)> Check);
}
