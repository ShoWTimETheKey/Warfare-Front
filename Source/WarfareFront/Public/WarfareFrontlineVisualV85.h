#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AFrontlineTank;
class UMaterialInterface;
namespace FrontlineVisualV85
{
    // A/B controls exist only inside the strict disposable V85 focused run.
    bool Allowed(const AWarfareFrontlineGameMode* Battle);
    bool SetBaseline(const AWarfareFrontlineGameMode* Battle,bool Baseline);
    bool Baseline();
    // Optical authoring only; never a release-radius, age or damage gain.
    float GasOpticalGain(UMaterialInterface* Material);
    bool PlaceFrozenTank(AWarfareFrontlineGameMode* Battle,AFrontlineTank* Tank,const FVector& Position);
    struct FRebindResult { bool Allowed=false,Ready=false;int32 Gas=0,Armour=0; };
    // Already frozen real producers, same transforms/ages and camera. Parameter
    // copies preserve their physical envelope; only the owned parents change.
    FRebindResult RebindFrozen(AWarfareFrontlineGameMode* Battle,bool Baseline);
}
