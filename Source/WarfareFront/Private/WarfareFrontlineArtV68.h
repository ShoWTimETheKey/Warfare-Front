#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGround;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMesh;
class UWorld;

// V69 production keeps V67 appearance. V68 art bindings are isolated to the
// -WFV68ArtReview harness; -WFV68Baseline also disables them there. The variables exist
// so an isolated review can compare V67 and V68 in one process: same frozen
// pose, camera, weather clock, exposure and SR/RR configuration.
namespace FrontlineArtV68
{
    bool SoldiersEnabled();
    bool TerrainEnabled();
    bool LightEnabled();
    // V68 and V67 appearance names map to the V34/V41 source contract name.
    FString SourceMeshName(const FString& Name);
    // V67 living/casualty body -> matching V68 body when the complete pair and
    // shared skeleton exist. Anything else is returned unchanged.
    USkeletalMesh* ResolveMesh(USkeletalMesh* Source);
    UMaterialInterface* ResolveMaterial(UMaterialInterface* Source);
    UMaterialInterface* ResolveBankMaterial(UMaterialInterface* Source);
    // Operation look -> V68 key/fill. Exposure, gamut, RT modes and quality are untouched.
    void AdjustLight(float& Pitch,float& Yaw,float& SunIntensity,float& SkyIntensity);
    // V68 mud material (identical to V67 when its facies amount is 0), or Source.
    UMaterialInterface* ResolveGroundMaterial(UMaterialInterface* Source);
    // Per-operation facies texture sampled from the same HeightAt function that
    // drives footing, collision and the baked tiles: R crater bowl, G raised
    // ejecta rim/crest, B steep cut face, A broad low ground. Binds it to the
    // ground MID when that material exposes the V68 parameters.
    void BindFieldFacies(AWarfareFrontlineGround* Ground,UMaterialInstanceDynamic* GroundMID);
    // Review-only in-place rebinding of existing soldiers, earth banks and light.
    void ApplyWorld(UWorld* World);
}
