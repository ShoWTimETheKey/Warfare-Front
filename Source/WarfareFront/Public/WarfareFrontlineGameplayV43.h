#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
struct FFrontlineSquad;

namespace FrontlineGameplayV43
{
// Rookie retains the teaching concession; Survivor uses the authored supply
// interval. Difficulty never changes player preparation or weapon lethality.
inline constexpr float EnemyAvailabilityRate=.75f;
inline float AvailabilityRate(EFrontlineDifficulty Difficulty)
{return Difficulty==EFrontlineDifficulty::Survivor?1.f:Difficulty==EFrontlineDifficulty::Veteran?.90f:EnemyAvailabilityRate;}
inline float PreparationSeconds(float Seconds,int32 Team,EFrontlineDifficulty Difficulty=EFrontlineDifficulty::Rookie)
{return Team==1?Seconds/AvailabilityRate(Difficulty):Seconds;}
bool HasCrossedEnemyEnd(const AWarfareFrontlineGameMode* Battle,const FFrontlineSquad& Squad);
void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
