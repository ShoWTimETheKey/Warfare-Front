#pragma once
#include "CoreMinimal.h"
#include "FRRUVehicleNavV87.h"

class AWarfareFrontlineGameMode;
class AActor;
class AFrontlineTank;

// Isolated Editor query/guidance evidence. Guidance supplies heading only;
// the original armour/FT Plan remains the sole movement/phase consumer.
namespace FRRUVehicleNavCandidateV87
{
using EStatus=FRRUVehicleNavV87::EStatus;
struct FProof
{
    EStatus Status=EStatus::Failed;
    FString Reason,Path;
};
using FGuidance=FRRUVehicleNavV87::FGuidance;
bool Requested();
FProof Begin(AWarfareFrontlineGameMode* Battle,AActor* SceneActor);
FProof PollProof(AWarfareFrontlineGameMode* Battle);
// Pending while active/waiting; Success only for actual enemy-end handoff;
// Failed exposes terminal follow errors independently of the warm query proof.
FProof FollowStatus(AWarfareFrontlineGameMode* Battle);
FGuidance FollowHeading(AWarfareFrontlineGameMode* Battle,AFrontlineTank* Vehicle);
void ObserveMovement(AWarfareFrontlineGameMode* Battle,AFrontlineTank* Vehicle,const FVector& TickStart,float ReportedTravel,bool HaltForWeapon);
void Clear(AWarfareFrontlineGameMode* Battle);
}
