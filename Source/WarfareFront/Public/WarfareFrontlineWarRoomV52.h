#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineController;
class AWarfareFrontlineGameMode;
namespace FrontlineWarRoomV52
{
 bool PrepareMenuReviewV61(AWarfareFrontlineController* PC,bool Confirm);
 bool PrepareReview(AWarfareFrontlineController* PC,int32 Mission,int32 Tab=0,float Zoom=2.3f,bool Dispatch=false);
 // Deterministic production-map fixture. The real flight may be inspected
 // separately; this sets a precise point on that same geographic projection.
 bool PrepareReviewV54(AWarfareFrontlineController* PC,int32 Mission,float Zoom,bool OperationSheet,int32 Tab=0);
 bool PrepareLoadingReviewV54(AWarfareFrontlineController* PC);
 bool LoadingReviewSettledV59(AWarfareFrontlineController* PC);
 void FinishLoadingReviewV54(AWarfareFrontlineController* PC);
 void RunFlightChecksV54(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check);
 void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
 void RunEntryChecksV828(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check);
}
