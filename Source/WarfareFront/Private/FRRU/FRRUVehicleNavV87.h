#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AActor;
class AFrontlineTank;
class FJsonObject;
namespace FRRUVehicleNavV87
{
enum class EStatus:uint8 { Pending,Success,Failed };
struct FResult { EStatus Status=EStatus::Failed;FString Reason; };
struct FGuidance
{
    bool Active=false,Pending=false,StopAtActualGoal=false,HasHeading=false;
    float Heading=0;FString Reason;
};
bool EligibleScene(const AWarfareFrontlineGameMode* Battle);
FResult Begin(AWarfareFrontlineGameMode* Battle,AActor* SceneActor);
FResult Poll(AWarfareFrontlineGameMode* Battle);
FGuidance FollowHeading(AWarfareFrontlineGameMode* Battle,AFrontlineTank* Vehicle);
void ObserveMovement(AWarfareFrontlineGameMode* Battle,AFrontlineTank* Vehicle,const FVector& TickStart,float ReportedTravel,bool HaltForWeapon);
FResult FollowStatus(AWarfareFrontlineGameMode* Battle);
void Clear(AWarfareFrontlineGameMode* Battle);
#if WITH_EDITOR
// Called only by the strict isolated Editor adapter. Query keeps its old native
// objective proof; follow exercises the same enemy-end controller as runtime.
FResult BeginReview(AWarfareFrontlineGameMode* Battle,AActor* SceneActor,bool QueryOnly);
TSharedPtr<FJsonObject> ReviewSnapshot(AWarfareFrontlineGameMode* Battle,bool ReleaseAndClear=false);
#endif
}
