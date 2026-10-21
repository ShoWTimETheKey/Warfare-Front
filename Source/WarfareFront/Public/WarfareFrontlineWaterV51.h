#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WarfareFrontlineWaterV51.generated.h"

class AWarfareFrontlineGround;
class AWarfareFrontlineWeather;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UMaterialInterface;
class FJsonObject;

struct FFrontlineNativeWaterBindingV82
{
    UStaticMeshComponent* Surface=nullptr;
    int32 MaterialSlot=INDEX_NONE;
    UMaterialInterface* Material=nullptr;
};

/** Analytic puddle response on the already clipped, physically shaded surface. */
UCLASS()
class WARFAREFRONT_API UFrontlineWaterResponseV51 : public UActorComponent
{
    GENERATED_BODY()
public:
    UFrontlineWaterResponseV51();
    static void Bind(AWarfareFrontlineGround* Ground);
    // Receiver scene slots share the existing bounded ripple/blood sources.
    // Their shader consumes the exact native floor, never legacy vertex data.
    static bool BindNativeV82(AActor* SceneOwner,
        const TArray<FFrontlineNativeWaterBindingV82>& Bindings,const FString& SceneId,uint32 Revision);
    static void UnbindNativeV82(AActor* SceneOwner);
    // Explicit callers force the complete scene check; only the ordinary water tick reuses this frame.
    bool IsNativeBindingCurrentV82(bool ReuseFrameValidation=false) const;
    static bool NotifyImpact(UWorld* World,const FVector& Position,float Radius,float Strength);
    static bool NotifyBlood(UWorld* World,const FVector& Position,float Mass,float Radius=165.f,bool bCorpseSeep=false);
    static void RunChecks(UWorld* World,TFunctionRef<void(const FString&,bool)> Check);
    static void RunChecksV53(UWorld* World,TFunctionRef<void(const FString&,bool)> Check);
    static void RunChecksV64(UWorld* World,TFunctionRef<void(const FString&,bool)> Check);
    static void RunLifecycleChecksV8219(UWorld* World,TFunctionRef<void(const FString&,bool)> Check);
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    TSharedRef<FJsonObject> Describe() const;
private:
    struct FImpulse { FVector2D Position=FVector2D::ZeroVector;float Began=-100.f,Radius=0.f,Strength=0.f,Group=-1.f,Level=0.f;bool Contact=false; };
    FImpulse Impulses[8];
    TMap<TWeakObjectPtr<AActor>,FVector> ContactPositionsV64;
    int32 ContactCursorV64=0,FootContactsV64=0,TrackContactsV64=0;
    struct FBloodSource
    {
        FVector2D Position=FVector2D::ZeroVector;
        float Began=-1000.f,Group=-1.f,Level=0.f,Mass=0.f,Radius=165.f;
    };
    FBloodSource Blood[8];
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WaterMID;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WaterSurface;
    int32 WaterMaterialSlotV82=0;
    FString NativeSceneIdV82;
    uint32 NativeRevisionV82=0;
    TWeakObjectPtr<AWarfareFrontlineWeather> Weather;
    float Rain=0.f,WeatherAge=1.f;
    int32 MaxImpulses=4,Accepted=0,Rejected=0;
    int32 MaxBloodSources=8,BloodDeposits=0,BloodRejected=0,BloodRunoffDeposits=0,BloodMerged=0;
    int32 CorpseSeepDeposits=0;
    FGuid EncounterV53;
    void Rebind(AWarfareFrontlineGround* Ground);
    void ResetBindingV82();
    bool HasBoundSurfaceV8219() const;
    void SetScalarV82(FName Name,float Value);
    void SetVectorV82(FName Name,const FLinearColor& Value);
    bool Admit(const FVector& Position,float Radius,float Strength,bool Contact=false);
    void SampleContactsV64();
    bool AdmitBlood(const FVector& Position,float Mass,float Radius);
    bool FindBloodEntry(const FVector& Position,FVector& Wet,float& Group,bool& Runoff) const;
    void ClearBlood();
    void UploadBlood(float Now);
    void Upload(float Now);
};
