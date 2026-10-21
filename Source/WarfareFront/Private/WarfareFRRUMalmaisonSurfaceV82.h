#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UFrontlineTerrainMaterialV47;
class FJsonObject;

namespace WarfareFRRUMalmaisonSurfaceV82
{
bool ReviewEnabled(const FString& SceneId);
const TCHAR* InstancePath();
const TCHAR* BasePath();
UFrontlineTerrainMaterialV47* Apply(const FString& SceneId,UStaticMeshComponent* Component,
    UMaterialInterface* Source,FString& Error);
bool AppliedTo(const UStaticMeshComponent* Component);
TSharedPtr<FJsonObject> Describe(const UStaticMeshComponent* Component);
}
