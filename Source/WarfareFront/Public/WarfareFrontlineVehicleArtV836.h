#pragma once
#include "CoreMinimal.h"
class UStaticMesh;
class UMaterialInterface;
namespace FrontlineVehicleArtV836
{
    FString EffectiveHullMaterialKeyV8310(const FString& Model,const FString& Key);
    UStaticMesh* ResolveMesh(const FString& Name);
    UMaterialInterface* ResolveMaterial(const FString& Key);
    UMaterialInterface* ResolveModelMaterialV838(const FString& Model,const FString& Key);
    UMaterialInterface* ResolveWreckMaterialV838(const FString& Model,const FString& Key);
    void HoldMaterialDetailV837(UMaterialInterface* Material);
}
