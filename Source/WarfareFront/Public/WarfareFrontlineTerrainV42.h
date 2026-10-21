#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class UWorld;
class AActor;
namespace FrontlineTerrainV42 {
    void Apply(AWarfareFrontlineGameMode* Battle);
    // Disposable actual-camera A/B. Swaps only the current ground material,
    // retaining its instance parameters and every physical tile/contact.
    bool SetGroundUnitsReviewV839(AWarfareFrontlineGameMode* Battle,bool Enabled);
    void UpdateStreaming(UWorld* World,const FVector& Camera,const FVector& Focus,float OrthoWidth,int32 ViewportWidth);
    float CameraVerticalLimitV43(float Width,float Aspect=16.f/9.f);
    void PlaceBackdropV43(AActor* Matte,const FVector& Focus,float Width,float Aspect=16.f/9.f);
}
