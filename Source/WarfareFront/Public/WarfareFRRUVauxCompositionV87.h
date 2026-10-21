#pragma once
#include "CoreMinimal.h"
class AActor;class USceneComponent;class UStaticMeshComponent;class UMaterialInterface;
class AWarfareFrontlineGameMode;
namespace WarfareFRRUVauxCompositionV87
{
    WARFAREFRONT_API bool Enabled(const FString& Scene);
    WARFAREFRONT_API FVector CameraOffset(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API int32 ComponentCount(const FString& Scene);
    WARFAREFRONT_API bool Stage(AActor* Owner,USceneComponent* Root,const FString& Scene,FString& Error,
        bool(*MaterialReady)(UMaterialInterface*,FString&));
    WARFAREFRONT_API bool ValidateComponent(const FString& Scene,const UStaticMeshComponent* Component);
}
