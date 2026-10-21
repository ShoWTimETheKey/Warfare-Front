#pragma once
#include "CoreMinimal.h"
class AActor;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
namespace FrontlineFlameVolumeV836
{
    UMaterialInterface* Material();
    UMaterialInterface* WreckMaterialV8311();
    UStaticMeshComponent* Create(AActor* Owner,FName Name,bool Wreck=false);
    void Update(UStaticMeshComponent* Proxy,UMaterialInstanceDynamic* Material,const FVector& Centre,
        const FVector& Dimensions,const FVector& ViewForward,float Strength,float Seed,float Clock,
        float Age=0.f,bool Contact=false,bool WorldClock=false);
}
