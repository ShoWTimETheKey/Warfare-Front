#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AActor;
class FJsonObject;
namespace FrontlineEnvironmentV42
{
    // Undo the previous pack before FieldV32 maps the next operation's earthworks.
    void Reset(AWarfareFrontlineGameMode* Battle);
    // Complete versioned pack, with no changes to terrain or contact geometry.
    bool Build(AWarfareFrontlineGameMode* Battle,AActor* Owner);
    // Strict disposable-review baseline. Never a release/player option.
    bool FieldBaselineV8449(const AWarfareFrontlineGameMode* Battle);
    // Call after TerrainV42's final per-operation MID installation.
    void FinishContactPropsV8449(AWarfareFrontlineGameMode* Battle);
    TSharedRef<FJsonObject> RunChecks(AWarfareFrontlineGameMode* Battle);
}
