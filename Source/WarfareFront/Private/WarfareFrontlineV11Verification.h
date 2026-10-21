#pragma once

#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class FJsonObject;

/** Passive observer. Use separate instances for an untouched match and arranged fixtures. */
class FFrontlineV11Verification
{
public:
    FFrontlineV11Verification();
    void Sample(const AWarfareFrontlineGameMode* Battle);
    TSharedRef<FJsonObject> BuildReport() const;
    bool AreObservedDeathsSettled() const;
private:
    struct FImpl;
    TSharedPtr<FImpl> Impl;
};
