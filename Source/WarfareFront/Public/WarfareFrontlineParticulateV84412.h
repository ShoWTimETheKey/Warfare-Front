#pragma once
#include "CoreMinimal.h"
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UWorld;

/** Non-smoke particulate bindings. Does not own emission, damage or clocks. */
namespace FrontlineParticulateV84412
{
struct FSceneLighting
{
    FLinearColor Direction=FLinearColor(0,0,1),Key=FLinearColor::Black,Ambient=FLinearColor::Black;
};
/** 0 mineral dust, 1 incident aerosol, 2 toxic gas. Null preserves the caller's
 * exact old material if absent or running the strict disposable A/B witness. */
WARFAREFRONT_API UMaterialInterface* Material(const UObject* WorldContext,int32 Profile=0);
WARFAREFRONT_API UStaticMesh* Cube();
WARFAREFRONT_API FSceneLighting SceneLighting(UWorld* World);
WARFAREFRONT_API void ApplySceneLighting(UWorld* World,UMaterialInstanceDynamic* Material);
WARFAREFRONT_API void Update(UStaticMeshComponent* Proxy,UMaterialInstanceDynamic* Material,
    const FVector& Centre,const FVector& Dimensions,const FVector& ViewForward,float Density,float Age,int32 Profile=0);
}
