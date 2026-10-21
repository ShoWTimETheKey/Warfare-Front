#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
class UTexture2D;
namespace FrontlineCampaignV41
{
constexpr int32 RouteRevision=41, MissionCount=20;
int32 RemapLegacyMission(int32 OldIndex);
int32 LegacyStage(int32 MissionIndex);
int32 TrenchCount(int32 MissionIndex);
bool MigrateLegacy(FFrontlineCampaignProgress& Run);
void Expand(TArray<FFrontlineCampaignMission>& Missions,bool German);
bool RoleAllowed(EFrontlineFaction Faction,EFrontlineRole Role);
int32 RoleArrival(EFrontlineFaction Faction,EFrontlineRole Role);
bool HasObjectiveVictory(const AWarfareFrontlineGameMode* Battle,int32 Team);
void TickObjective(AWarfareFrontlineGameMode* Battle,float DeltaSeconds);
float TelephoneResponseRelief(const AWarfareFrontlineGameMode* Battle,int32 Team);
FString ObjectiveConclusion(const AWarfareFrontlineGameMode* Battle,int32 Team);
void AppendTechnologyNodes(TArray<FFrontlineTechnologyNode>& Nodes,bool German);
UTexture2D* NewMap(const FFrontlineCampaignMission& Mission);
UTexture2D* QuastPortrait();
UTexture2D* RolePortrait(EFrontlineRole Role);
UTexture2D* TechnologyPortrait(int32 Id,EFrontlineFaction Faction);
void AppendUIPreloadPathsV8213(TArray<FString>& Paths);
bool PrepareReviewOperation(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index);
bool PrepareArtReviewOperationV71(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index,bool Research);
bool PopulateArtReviewFieldV73(AWarfareFrontlineGameMode* B);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
void RunChecksV50(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
