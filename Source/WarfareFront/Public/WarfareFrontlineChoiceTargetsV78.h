#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
enum class EFrontlineFaction : uint8;
enum class EFrontlineRole : uint8;
namespace FrontlineChoiceTargetsV78
{
    void Reset(const AWarfareFrontlineGameMode* Battle);
    bool IsAreaOperator(const AWarfareFrontlineSoldier* Unit);
    // Rifle-grenade operators compare bounded visible contacts; other weapons keep Nearest.
    AWarfareFrontlineSoldier* Pick(const AWarfareFrontlineGameMode* Battle,const AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Nearest);
    bool ShouldRetarget(const AWarfareFrontlineGameMode* Battle,const AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Candidate,AWarfareFrontlineSoldier* Current);
    EFrontlineRole PrecisionCounter(EFrontlineFaction Faction);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
