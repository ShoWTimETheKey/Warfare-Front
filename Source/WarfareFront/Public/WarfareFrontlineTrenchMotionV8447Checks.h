#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class FJsonObject;
namespace FrontlineTrenchMotionV8447
{
TSharedRef<FJsonObject> RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
// Three paired real-Tick contact cases only; requires a disposable UK/DE review.
TSharedRef<FJsonObject> RunExistingContactChecksV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
// Held MG reload, settled helper and reload-paused transit helper continuity;
// six current-terrain real-Tick cases only, with guarded actor/state receipts.
TSharedRef<FJsonObject> RunMachinegunContinuityChecksV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
// Four full-phase Ready/Reload diagnostics, using only actual ray segments.
// The 6 cm lowering gate is unchanged; when it is absent, no obstacle or
// production contract test is created and the hypothesis is explicitly unproved.
TSharedRef<FJsonObject> RunMachinegunEngagementChecksV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
// Two Editor-only friendly-hull/held-MG-crew cases with actual phase, movement,
// physical support bay, generic formation bay and pure live lease receipts.
TSharedRef<FJsonObject> RunMachinegunLeaseChecksV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
// A persistent, disposable production section for paired captures. Reload=true
// reaches its real mid-reload phase through Tick, then freezes that evaluated
// pose until Cleanup. Both captures share identical actors, feet and camera focus.
bool PreparePoseV8447(AWarfareFrontlineGameMode* Battle,int32 Team,bool Reload,FVector& Focus,TFunctionRef<void(const FString&,bool)> Check,bool Machinegun=false);
void CleanupPoseV8447(AWarfareFrontlineGameMode* Battle);
}
