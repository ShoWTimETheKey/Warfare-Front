#pragma once
#include "CoreMinimal.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
namespace FrontlineSmokeVolumeV35
{
    inline constexpr int32 CloudsPerMission=6;
    UMaterialInterface* MaterialV40();
    UMaterialInterface* GasMaterialV8215();
    // Width/depth/height are world centimetres. The ray-marched density stays
    // in world space while the camera moves, including vertical camera pans.
    void Update(UStaticMeshComponent* Proxy,UMaterialInstanceDynamic* Material,
        const FVector& Centre,const FVector& Dimensions,const FVector& ViewForward,float Density,float Age);
}
