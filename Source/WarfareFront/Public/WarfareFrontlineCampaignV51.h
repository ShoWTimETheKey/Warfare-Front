#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class UTexture2D;
namespace FrontlineCampaignV51
{
void ApplyCampaign(TArray<FFrontlineCampaignMission>& Missions,bool German);
bool CampaignCompleted(const AWarfareFrontlineGameMode* Battle);
bool CanOpenEnding(const AWarfareFrontlineGameMode* Battle);
bool IsFinalVictory(const AWarfareFrontlineGameMode* Battle);
const TCHAR* Copy(const TCHAR* Key);
UTexture2D* Photo(int32 Slot);
void AppendUIPreloadPathsV8213(TArray<FString>& Paths);
int32 PhotoForOperation(int32 Operation);
bool PrepareEndingReview(AWarfareFrontlineGameMode* Battle,EFrontlineFaction Faction);
void RunChecksV51(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
namespace FrontlineReviewUIV51
{
void Ending(AWarfareFrontlineController* Controller);
}
