#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineRenderSettingsV87
{
// Editor-only live setters; explicit V87/NoSave gate and unique scratch INI.
TSharedRef<FJsonObject> RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
