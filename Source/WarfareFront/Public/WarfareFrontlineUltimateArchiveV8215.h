#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
#include "FRRU/WarfareFRRUIntegration.h"
class AWarfareFrontlineGameMode;
namespace FrontlineUltimateArchiveV8215
{
bool Available(EFrontlineFaction Faction);
/** Four-army entry points; never reinterpret a nation as an old faction. */
bool NationAvailableV87(const AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation);
const TArray<FFrontlineCampaignMission>& NationMissionsV87(const AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation);
bool BeginNationV87(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation,int32 Mission,EFrontlineDifficulty Difficulty);
bool Active(const AWarfareFrontlineGameMode* Battle);
bool ReplayEligible(const AWarfareFrontlineGameMode* Battle);
bool FullArmory(const AWarfareFrontlineGameMode* Battle,int32 Team=0);
/** Optional replay handicap. All technologies are enabled by default. */
bool SetTechnologyEnabled(AWarfareFrontlineGameMode* Battle,int32 Technology,bool Enabled);
bool SetArmourTechnologyEnabled(AWarfareFrontlineGameMode* Battle,int32 Technology,bool Enabled);
bool Begin(AWarfareFrontlineGameMode* Battle,EFrontlineFaction Faction,int32 Mission,EFrontlineDifficulty Difficulty);
void Leave(AWarfareFrontlineGameMode* Battle);
void MarkAssisted(AWarfareFrontlineGameMode* Battle);
/** Explicit review only. Invalid review parameters fail closed for every account path. */
bool ReviewRequested();
bool ReviewUserDir(FString& Out);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
