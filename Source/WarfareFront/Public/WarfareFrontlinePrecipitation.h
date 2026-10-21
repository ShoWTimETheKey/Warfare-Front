#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlinePrecipitation.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
struct FFrontlinePrecipitationReadinessV42;

struct FFrontlineRainDrop
{
    FVector Position=FVector::ZeroVector;
    float Speed=900.f, Length=35.f, Width=2.f, Opacity=.7f;
    bool bActive=false;
};

struct FFrontlineRainContact
{
    FVector Position=FVector::ZeroVector,Normal=FVector::UpVector;
    float Age=0.f,Life=.4f,Size=14.f;
    bool bActive=false,bRipple=false;
};

/** World-space, cosmetic precipitation: two bounded instanced-quad pools. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlinePrecipitation : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlinePrecipitation();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    /** Amount is 0..1; wind is horizontal world centimetres/second. */
    void SetWeather(float Amount,const FVector& Wind);
    float GetRainAmount() const { return RainAmount; }
    int32 GetActiveDropCount() const { return ActiveDropCount; }
    int32 GetActiveSplashCount() const { return ActiveSplashCount; }
    FVector GetVisibleCentreV76() const { return VisibleCentre; }
    const TArray<FFrontlineRainContact>& GetRainContactsV76() const { return Contacts; }
    bool IsVisualReady() const { return bVisualReady; }
    bool IsAwaitingMaterialsV40() const { return !bVisualReady && !bInitializationFailedV40; }
    int32 GetReadinessAttemptsV40() const { return ReadinessAttemptsV40; }
    static constexpr int32 MaximumRainDrops=600;
    static constexpr int32 MaximumSplashes=96;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RainLines;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RainContacts;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RainMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ContactMaterial;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> RainPlaneV40;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> RainSourceV40;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ContactSourceV40;
    TArray<FFrontlineRainDrop> Drops;
    TArray<FFrontlineRainContact> Contacts;
    TArray<FTransform> RainTransforms,ContactTransforms;
    TArray<float> RainCustomData,ContactCustomData;
    FRandomStream Random;
    FVector WindVelocity=FVector::ZeroVector,VisibleCentre=FVector::ZeroVector;
    FVector ViewForward=FVector(0,-1,-.28f),ViewRight=FVector::ForwardVector;
    float RainAmount=0.f,VisualRainAmount=0.f,HalfWidth=3600.f,HalfDepthV76=2450.f,SplashCredit=0.f;
    int32 ActiveDropCount=0,ActiveSplashCount=0,NextContact=0;
    bool bVisualReady=false,bHasView=false;
    bool bInitializationFailedV40=false;
    double ReadinessStartV40=0,NextReadinessCheckV40=0;
    int32 ReadinessAttemptsV40=0;
    TSharedPtr<FFrontlinePrecipitationReadinessV42,ESPMode::ThreadSafe> MaterialReadinessV42;
    bool InitializeVisualsV40();
    bool UpdateView();
    void ResetDrop(FFrontlineRainDrop& Drop,bool bFillVolume);
    void AddContact(const FVector& Point);
};
