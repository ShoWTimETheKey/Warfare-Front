#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AActor;
class FJsonObject;
namespace FrontlineEnvironmentV40
{
    void Reset(AWarfareFrontlineGameMode* Battle);
    bool Build(AWarfareFrontlineGameMode* Battle,AActor* Owner);
    TSharedRef<FJsonObject> RunChecks(AWarfareFrontlineGameMode* Battle);
}
