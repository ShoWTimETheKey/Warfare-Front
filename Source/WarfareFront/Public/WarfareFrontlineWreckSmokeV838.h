#pragma once
#include "CoreMinimal.h"
class UStaticMeshComponent;
class UMaterialInterface;
enum class EFrontlineVehicle : uint8;

/** Cosmetic source sites in the same local centimetres as the authored hull. */
struct FFrontlineWreckSmokeSiteV838
{
    FVector LocalOrigin=FVector::ZeroVector;
    float WidthCm=180,FlameWeight=1,SmokeWeight=1,FlameScale=1;
    const TCHAR* Name=TEXT("hull");
    bool Primary=false;
};
struct FFrontlineWreckSmokeProfileV838
{
    FString Name;
    float FootprintScale=1;
    TArray<FFrontlineWreckSmokeSiteV838> Sites;
};
struct FFrontlineWreckSmokeLayerV838
{
    FVector Centre=FVector::ZeroVector,Dimensions=FVector::ZeroVector;
    FLinearColor Colour=FLinearColor::Black;
    float Density=0,FieldAge=0;
};
namespace FrontlineWreckSmokeV838
{
    inline constexpr int32 LayersPerSource=3;
    inline constexpr float TailSeconds=2.4f;
    UMaterialInterface* Material();
    FFrontlineWreckSmokeProfileV838 Profile(EFrontlineVehicle Type,const UStaticMeshComponent* Hull=nullptr);
    FVector Wind(float GameTime);
    // Coherent lower combustion, rising body and diluted upper billow. The
    // existing transported density remains a true depth-tested volume.
    FFrontlineWreckSmokeLayerV838 Layer(const FVector& Source,float WidthCm,float Strength,float Heat,
        float SourcePhase,float FlowAge,float FeedAge,float GameTime,int32 Index);
}
