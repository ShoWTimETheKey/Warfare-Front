#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WarfareFrontlineBlastCompositionV8221.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class FJsonObject;

/** Short physical layers surrounding the retained smoke; never owns damage or the battle clock. */
UCLASS()
class WARFAREFRONT_API UFrontlineBlastCompositionV8221 : public UActorComponent
{
    GENERATED_BODY()
public:
    UFrontlineBlastCompositionV8221();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    void Configure(int32 Profile,const FVector& Origin,float Radius=1200.f,bool bUseArtResetV831=true);
    void StartIndependent(float FuelStrength);
    /** Releases only this standalone fuel impulse, including its registered render components. */
    void ReleaseIndependent();
    void SetCasualties(const TArray<FVector>& Feet,int32 Count);
    void UpdateAt(float Age,const FVector& ViewForward);
    bool IsReady() const { return Ready; }
    bool UsesArtResetV831() const { return ArtResetV831; }
    bool ReplacesFlatFlash() const { return Ready&&Kind!=3; }
    TSharedRef<FJsonObject> Describe() const;
    void Check(TFunctionRef<void(const FString&,bool)> Check) const;
    static UStaticMesh* SoilMesh();
    static UMaterialInterface* SoilMaterial();
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Cores;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CoreMaterials;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Aerosols;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> AerosolMaterials;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> EarthJets;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> EarthMaterials;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Splinters;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SplinterMaterial;
    TArray<FVector> FragmentVelocity,FragmentAngular;
    TArray<FVector> FragmentScale;
    TArray<FVector> CasualtyFeet;
    FVector BlastOrigin=FVector::ZeroVector;
    float PhaseAge=0.f,FootprintRadius=1200.f,Seed=0.f;
    int32 Kind=0,CasualtyCount=0;
    bool Ready=false;
    bool ArtResetV831=false;
    bool ParticulatesV84412=false,BaselineV84412=false,LightingAppliedV84412=false;
    FLinearColor LightDirectionV84412=FLinearColor::Black,KeyRadianceV84412=FLinearColor::Black,AmbientRadianceV84412=FLinearColor::Black;
    bool Independent=false,FirstIndependentTick=true;
    float IndependentAge=0.f,FuelGain=1.f;
    double IndependentBornAt=0.;
};

namespace FrontlineBlastCompositionV8221
{
WARFAREFRONT_API void Prewarm();
/** Supplement only the fast three-dimensional impulse of a genuine fuel rupture.
 * Existing FireV33 persistent flames and smoke remain their original producers. */
WARFAREFRONT_API void EmitFuelBurst(AActor* Owner,const FVector& Origin,float Fuel);
/** Called by the persistent fire service on reset; leaves all other fire and smoke ownership intact. */
WARFAREFRONT_API void ClearFuelBursts(AActor* Owner);
}
