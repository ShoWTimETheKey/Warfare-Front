#include "WarfareBattleSubsystem.h"
#include "Engine/World.h"

bool UWarfareBattleSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UWarfareBattleSubsystem::StartBattle(FName InScenarioId, int32 InSeed)
{
    if (bBattleActive || InScenarioId.IsNone())
    {
        return false;
    }

    ScenarioId = InScenarioId;
    BattleSeed = InSeed;
    bBattleActive = true;
    return true;
}

void UWarfareBattleSubsystem::EndBattle()
{
    bBattleActive = false;
    ScenarioId = NAME_None;
    BattleSeed = 0;
}

void UWarfareBattleSubsystem::Deinitialize()
{
    EndBattle();
    Super::Deinitialize();
}
