#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class FJsonObject;

struct FFrontlineMissionIntroduction
{
    EFrontlineRole Role{};
    FString Heading,Title,Context,Tactic;
};
TSharedRef<FJsonObject> RunFrontlineV27CampaignChecks(AWarfareFrontlineGameMode* Battle);
void StartFrontlineV27CampaignReview(AWarfareFrontlineController* Controller);
