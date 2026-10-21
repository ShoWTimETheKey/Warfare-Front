#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
WARFAREFRONT_API bool SetupV36TrenchCrew(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check,int32 Team=0);
WARFAREFRONT_API void InspectV36TrenchCrew(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
