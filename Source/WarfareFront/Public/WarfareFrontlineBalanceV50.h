#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
namespace FrontlineBalanceV50
{
    // Tactical modifiers affect a hit roll, never injury points or armour damage.
    float TargetExposure(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target,float Cover);
    // The reload posture and its restored protection share this phase envelope.
    float TrenchReloadShelterBlendV8447(const AWarfareFrontlineSoldier* Unit);
    float ShooterReadiness(const AWarfareFrontlineSoldier* Shooter);
    bool HasCoveringFireV8215(const AWarfareFrontlineSoldier* Shooter);
    float CoveringFireHitScaleV8215(const AWarfareFrontlineSoldier* Shooter);
    float CoveringFireRecoveryScaleV8215(const AWarfareFrontlineSoldier* Shooter);
    float OrderInterval(bool Officer);
    float OrderRange(bool Officer,bool German);
    float OrderRelief(bool Officer,bool German);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
class FJsonObject;
TSharedRef<FJsonObject> RunFrontlineBalanceV8447Checks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
