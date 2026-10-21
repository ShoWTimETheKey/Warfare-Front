#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "WarfareBattleSubsystem.generated.h"

/** Per-world lifecycle boundary. Simulation and tactical decisions are not implemented yet. */
UCLASS()
class WARFAREFRONT_API UWarfareBattleSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Warfare|Battle")
    bool StartBattle(FName InScenarioId, int32 InSeed);

    UFUNCTION(BlueprintCallable, Category="Warfare|Battle")
    void EndBattle();

    UFUNCTION(BlueprintPure, Category="Warfare|Battle")
    bool IsBattleActive() const { return bBattleActive; }

    UFUNCTION(BlueprintPure, Category="Warfare|Battle")
    FName GetScenarioId() const { return ScenarioId; }

    UFUNCTION(BlueprintPure, Category="Warfare|Battle")
    int32 GetBattleSeed() const { return BattleSeed; }

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    UPROPERTY(Transient)
    FName ScenarioId;

    UPROPERTY(Transient)
    int32 BattleSeed = 0;

    UPROPERTY(Transient)
    bool bBattleActive = false;
};
