#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
enum class EFrontlineRole : uint8;

namespace FrontlineEnemyPolicyV84415
{
// Inputs are a commander's own company and physically observed enemy contacts.
// No player selections, pending orders, research or cooldowns enter this policy.
struct FRecruitSituation
{
    EFrontlineDifficulty Difficulty=EFrontlineDifficulty::Rookie;
    EFrontlineFaction Faction=EFrontlineFaction::German;
    uint32 AvailableRoles=0;
    int32 OwnRoles[11]={};
    int32 OwnLiving=0,OwnGuns=0,SeenRifles=0,SeenMarksmen=0,SeenGuns=0,SeenEntrenched=0,SeenArmour=0;
    int32 RecruitStep=0;
    float NearestContact=MAX_flt,WireDensity=0,MineDensity=0;
    bool Counterattack=false,Archive=false;
};
EFrontlineRole CounterRole(const FRecruitSituation& Situation);
uint32 ResearchMask(EFrontlineFaction Faction,int32 Operation,EFrontlineDifficulty Difficulty);
bool ResearchAvailable(const AWarfareFrontlineGameMode* Battle,int32 Id);
bool RetainAnchor(EFrontlineDifficulty Difficulty,int32 Friendly,int32 AnchorLiving,
    float Observed,float Remembered,bool LocalThreat,bool RearRelief);
float WaveRestScale(EFrontlineDifficulty Difficulty,int32 Operation,bool Threatened);
}
// Production ThinkEnemy actions and catalogue checks; explicit disposable profile only.
void RunFrontlineEnemyChecksV84415(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
