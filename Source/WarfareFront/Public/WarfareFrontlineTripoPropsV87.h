#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class FJsonObject;

/** Private disposable UK/DE scene review only; production never loads Tripo. */
namespace FrontlineTripoPropsV87
{
    bool Allowed(const AWarfareFrontlineGameMode* Battle);
    /** Snapshot the six existing sandbag groups; retain the actual scene baseline. */
    TSharedRef<FJsonObject> Prepare(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    /** Install only the candidate visuals and explicit weather on those groups. */
    TSharedRef<FJsonObject> SwitchCandidate(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    /** Read-only post-warm sample; no asset loads, setters or compilation work. */
    TSharedRef<FJsonObject> DescribeCurrent(const AWarfareFrontlineGameMode* Battle);
    /** Explicit private corrective pass: wait only this candidate's used textures. */
    TSharedRef<FJsonObject> EnsureTextureReady(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    /** Explicit private UV0 atlas metadata repair only when actual sampling is missing. */
    TSharedRef<FJsonObject> RepairCandidateSampling(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    /** Same world-space focus for paired baseline/candidate camera captures. */
    bool Focus(const AWarfareFrontlineGameMode* Battle,FVector& OutCentre);
    /** Restore captured meshes/materials/instance transforms without save writes. */
    void Restore(AWarfareFrontlineGameMode* Battle);
}
