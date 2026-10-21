#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;

namespace FrontlineMoraleV36
{
    enum class ECause : uint8 { Casualties, FriendlyFire, Gas, Armour, LostTrench, CapturedTrench, Advance, Cohesion, OfficerLost, ArmourDestroyed, Rally, Count };
    struct FTelemetry
    {
        float Loss[2][7]={{0}}, Recovery[2][4]={{0}};
        ECause LastCause[2]={ECause::Casualties,ECause::Casualties};
        float LastEventAt[2]={-1000,-1000};
    };
    inline constexpr float CasualtyLoss=.70f;
    inline constexpr float CohesionBudget=6.f;
    void Reset(AWarfareFrontlineGameMode* Battle);
    /** In-memory report probe only; restores private casualty/event receipts. */
    void WithScopedCombatEventsV839(AWarfareFrontlineGameMode* Battle,TFunctionRef<void()> Work);
    bool OnCasualty(AWarfareFrontlineGameMode* Battle,AWarfareFrontlineSoldier* Unit,int32 AttackingTeam);
    float StartingMaximum(AWarfareFrontlineGameMode* Battle,int32 Team);
    float CapacityMultiplierV85(const AWarfareFrontlineGameMode* Battle);
    void OnArmourDestroyed(AWarfareFrontlineGameMode* Battle,int32 Team,bool Rail,bool PassengersAboard);
    void Rally(AWarfareFrontlineGameMode* Battle,int32 Team);
    void RunChecksV76(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void OnCapture(AWarfareFrontlineGameMode* Battle,int32 Trench,int32 PreviousOwner,int32 Team);
    void OnCaptureSector(AWarfareFrontlineGameMode* Battle,const FString& SectorId,int32 PreviousOwner,int32 Team);
    void Tick(AWarfareFrontlineGameMode* Battle,float Seconds);
    bool HasTerritorialVictory(const AWarfareFrontlineGameMode* Battle,int32 Team);
    const FTelemetry& Telemetry(AWarfareFrontlineGameMode* Battle);
    const TCHAR* CauseLabel(ECause Cause);
    void RunLayoutChecksV50(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}

void RunFrontlineMoraleV36Checks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
