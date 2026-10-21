#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;

// Opt-in review fixture only; invoke once on the isolated V30 review world.
// Stages: 0 carry, 1 ground deployment, 2 ground fire, 3 actual feed reload,
// 4 pack-up, 5 both faction crews at their own home trench firing step.
void BeginFrontlineV30MachinegunReview(AWarfareFrontlineGameMode* Battle,float GroundCenterX);
void SetFrontlineV30MachinegunReviewStage(AWarfareFrontlineGameMode* Battle,int32 Stage);
void TickFrontlineV30MachinegunReview(AWarfareFrontlineGameMode* Battle,float DT);
TSharedRef<FJsonObject> GetFrontlineV30MachinegunReviewReport(AWarfareFrontlineGameMode* Battle);
