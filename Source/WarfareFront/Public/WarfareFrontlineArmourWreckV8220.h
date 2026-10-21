#pragma once

#include "CoreMinimal.h"

class AFrontlineTank;
class FJsonObject;

/** One irreversible visual transition on hull destruction; no gameplay envelope. */
struct FFrontlineArmourWreckV8220Access
{
    static bool Warm();
    static void Apply(AFrontlineTank* Tank);
    static TSharedRef<FJsonObject> Describe(const AFrontlineTank* Tank);
};
