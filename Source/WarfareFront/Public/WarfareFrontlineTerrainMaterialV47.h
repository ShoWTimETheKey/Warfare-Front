#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WarfareFrontlineTerrainMaterialV47.generated.h"

// The ground's world-projected custom shader has no compiler-derived UV
// sampling entries. Keep its real sampling metadata on a transient instance;
// never edit the imported material or use a dummy texture parameter to make
// UE refresh its streaming cache.
UCLASS(Transient)
class WARFAREFRONT_API UFrontlineTerrainMaterialV47 : public UMaterialInstanceDynamic
{
    GENERATED_BODY()
public:
    static UFrontlineTerrainMaterialV47* CreateForGround(UMaterialInterface* Source,UObject* Outer)
    {
        if(!Source)return nullptr;
        auto* Instance=NewObject<UFrontlineTerrainMaterialV47>(Outer);
        auto* SourceInstance=Cast<UMaterialInstance>(Source);
        Instance->InitializeMID(SourceInstance&&SourceInstance->Parent?SourceInstance->Parent.Get():Source);
        Instance->CopyMaterialUniformParameters(Source);
        return Instance;
    }
    void SetWorldSampling(const TArray<FMaterialTextureInfo>& Entries)
    {
        SetTextureStreamingData(Entries);
        CacheTexturesSamplingInfo();
    }
};
