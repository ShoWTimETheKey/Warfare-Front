#pragma once
#include "CoreMinimal.h"
class USkeletalMesh;
class UMaterialInterface;
namespace FrontlineSoldierSurfaceV67
{
    USkeletalMesh* ResolveMesh(USkeletalMesh* Source);
    UMaterialInterface* ResolveMaterial(UMaterialInterface* Source);
    FString SourceMeshName(const FString& Name);
}
