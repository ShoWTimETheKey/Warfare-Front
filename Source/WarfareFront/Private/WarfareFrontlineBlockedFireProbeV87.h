#pragma once
#include "CoreMinimal.h"

// Explicit disposable-editor diagnostics only. No gameplay decisions use this state.
#if WITH_EDITOR
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AFrontlineTank;
class FJsonObject;
namespace FrontlineBlockedFireProbeV87
{
bool Requested();
bool Isolation();
bool Enabled(const AWarfareFrontlineGameMode* Battle);
void Observe(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Contact,bool StopsForContact,
    bool WantsMovement,bool CrewReform,bool AtDestination,const FVector& Destination);
void TickGate(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Target,const TCHAR* Reason,
    float ShotClock,float AimSettleClock,float FirePoseClock,float DepartureClock);
void Discharge(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Target,AFrontlineTank* Vehicle,const TCHAR* Reason);
TSharedPtr<FJsonObject> Failure(const AWarfareFrontlineGameMode* Battle);
}
#endif
