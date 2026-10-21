#pragma once

#include "CoreMinimal.h"

class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class FJsonObject;

namespace FrontlineWeapons
{
// Engagement distances and timing are deliberately compressed game values.
// Feed systems retain the physical 10/5/32/47/100-round distinction in Soldier.
struct FProfile
{
    const TCHAR* WeaponId;
    float FireRangeCm;
    float AdvanceContactCm;
    // Per-round hit probability weight, not a damage/health quantity.
    float HitProbabilityScale;
    float DirectPressure;
    float AdjacentPressure;
    float AdjacentRadiusCm;
    float AccuracyNear;
    float AccuracyFloor;
    float AccuracyFalloffCm;
    float AimedCycleSeconds;
    float RoundIntervalSeconds;
    int32 BurstMin;
    int32 BurstMax;
    float RecoveryMin;
    float RecoveryMax;
};

// Apply the relative V38 firearm uplift after range, cover, smoke and research.
// Automatic refers to the weapon actually fired, including vehicle/aircraft MGs.
float FirearmHitChance(float BaseProbability,bool bAutomatic);

const FProfile& ProfileFor(const AWarfareFrontlineSoldier* Soldier);
// Ammunition bearers halt with their living gunner, but fire using their own rifle.
float AdvanceContactRangeFor(const AWarfareFrontlineSoldier* Soldier);
// The heavier issued rifle bomb owns only armour stopping distance; its No.23
// infantry profile and firing stance are unchanged.
float AdvanceArmourContactRangeForV8310(const AWarfareFrontlineSoldier* Soldier);
// Depth is visual staging for ordinary line fire; physical traces and hit
// falloff continue to use the real world endpoints.
float DirectFireRangeDistanceCm(const AWarfareFrontlineSoldier* Soldier,const FVector& TargetFeet);
float GrenadierPreparationRateBoostV835(const AWarfareFrontlineSoldier* Soldier,const AWarfareFrontlineSoldier* Target,bool ObservedThisTick);
}

// Synchronous, opt-in QA: uses existing live squads and restores all fixture positions.
TSharedRef<FJsonObject> RunFrontlineV21WeaponChecks(AWarfareFrontlineGameMode* Battle);
