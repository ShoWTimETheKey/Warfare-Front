#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
struct FFrontlineCampaignMission;
enum class EFrontlineRole : uint8;

namespace FrontlineGameplayV33
{
// Exact binary32 authoring value of the historical V44 base (.95f*.90f*.85f).
// A decimal product folds differently under /fp:fast, changing baked resource keys.
inline constexpr float CampaignDepthScale=0x1.741892p-1f;
float RelativeDepthScaleV45(int32 MissionId);
float OperationDepthScale(int32 MissionId);
void RunDepthChecksV45(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
bool EnemyRoleAvailable(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role);
// Only the opening assembly can skip the long post-contact echelon rest.
bool IsOpeningAssembly(int32 MissionId,int32 Recruited,int32 Living,float Elapsed,bool HasObservedContact);
void ApplyEarlyMissionPacing(TArray<FFrontlineCampaignMission>& Missions,bool GermanPlayer);
// Isolated live scenarios; refuses to mutate a player's normal campaign archive.
void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
