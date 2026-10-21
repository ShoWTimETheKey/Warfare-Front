#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class FJsonObject;

struct FFRRUSceneSolidRayV82
{
    FString Label;
    FVector From=FVector::ZeroVector,To=FVector::ZeroVector;
    bool ExpectedBlocked=false;
};

// These functions inspect/stage real candidate meshes, shader maps and physics.
// They do not grant camera/art, native traversal/deck/LOS or Shipping acceptance.
namespace WarfareFRRUContentV82
{
    WARFAREFRONT_API FString WaterHeightTexturePathV82(const FString& NativeSceneId);
    WARFAREFRONT_API bool SceneAssetsReady(const FString& NativeSceneId,int32 ExpectedUCX,FString& Error);
    WARFAREFRONT_API bool StageScene(AWarfareFrontlineGameMode* Battle,const FString& NativeSceneId,
        const FTransform& Transform,int32 ExpectedUCX,FString& Error);
    WARFAREFRONT_API bool SceneStaged(const AWarfareFrontlineGameMode* Battle,const FString& NativeSceneId,uint32 NativeRevision);
    // Rain and surface probes share one full census per frame. Staging,
    // explicit readiness and diagnostic checks continue to use SceneStaged.
    WARFAREFRONT_API bool SceneStagedForWater(const AWarfareFrontlineGameMode* Battle,const FString& NativeSceneId,uint32 NativeRevision);
    // Read displayed component bindings and resident texture mips at the
    // current game frame. Does not compile, force residency or accept art.
    WARFAREFRONT_API TSharedPtr<FJsonObject> DescribeSceneRendering(const AWarfareFrontlineGameMode* Battle);
    // Drive only displayed scene materials that declare the shared weather
    // parameter. Returns the number of actual slots updated, not acceptance.
    WARFAREFRONT_API int32 ApplyWeatherWetness(const AWarfareFrontlineGameMode* Battle,float Wetness);
    WARFAREFRONT_API void ApplyWeatherDistant(const AWarfareFrontlineGameMode* Battle,float SkyGain,float CloudCover,float Clock);
    // Direct new-component traces, not traces satisfied by the legacy floor.
    // Host supplies deck/route samples and native solid rays. Dynamic wire has
    // its own host-owned body and must be probed before/after clearing there.
    WARFAREFRONT_API bool ProbeScene(const AWarfareFrontlineGameMode* Battle,
        const TArray<FVector2D>& AdditionalHeightSamples,const TArray<FFRRUSceneSolidRayV82>& SolidRays,
        TSharedPtr<FJsonObject>& Evidence,FString& Error);
    WARFAREFRONT_API void ClearScene(AWarfareFrontlineGameMode* Battle);
}
