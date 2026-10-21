#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "WarfareFrontlineSandbagDescriptorV87.generated.h"

class UStaticMesh;
class UMaterialInterface;

/** Runtime provenance; ordinary AssetUserData survives Cook, unlike editor tags. */
UCLASS(meta=(ScriptName="SandbagDescriptorDataV87"))
class WARFAREFRONT_API UFrontlineSandbagDescriptorDataV87 : public UAssetUserData
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,Category="Descriptor") int32 DescriptorVersion=1;
    UPROPERTY(EditAnywhere,Category="Descriptor") int32 VariantIndex=INDEX_NONE;
    UPROPERTY(EditAnywhere,Category="Descriptor") FString GeometryHash;
    UPROPERTY(EditAnywhere,Category="Descriptor") FString SupportHash;
    UPROPERTY(EditAnywhere,Category="Descriptor") TArray<FString> LodGeometryHashes;
};

namespace FrontlineSandbagDescriptorV87
{
    /** One accepted A mesh; instance rotation varies its orientation. Failure leaves outputs empty for the original six-variant fallback. */
    bool TryLoad(TArray<UStaticMesh*>& Meshes,UMaterialInterface*& Material);
    int32 Variant(int32 OriginalVariant);
    const FVector* Feet(int32 VariantIndex);
    bool TopHeight(int32 VariantIndex,const FTransform& Transform,float X,float Y,float& Height);
    bool IsMaterial(UMaterialInterface* Material);
}
