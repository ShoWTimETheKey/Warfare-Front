#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineWreckSmokeV838.h"
#include "WarfareFrontlineArmourAudioV839.h"
#include "WarfareFrontlineFireV33.generated.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class FJsonObject;
struct FFrontlineFuelParticleV33
{
    FVector P=FVector::ZeroVector,V=FVector::ZeroVector;
    float Age=100,Life=1,Size=1,Stretch=1,Seed=0,Turn=0;
    // Layer 0 retains the accepted jet/rupture; 1 clothing, 2 wreck, 3 ember.
    uint8 Layer=0;
    float Growth=4.8f,FrameRate=13,Heat=1,Buoyancy=85,Drag=.44f,CurlRate=13,Spin=0;
    float AttachSeconds=0;
    // Selected attached tongues use a source-oriented ribbon instead of a
    // camera-locked silhouette. The remaining lobes supply changing depth.
    bool bSourceRibbonV8216=false;
    float RibbonBearingV8216=0;
    FVector LastAnchor=FVector::ZeroVector,AttachmentLocalOffset=FVector::ZeroVector;
    TWeakObjectPtr<USkeletalMeshComponent> AttachedMesh;
    FName AttachedBone=NAME_None;
    bool Smoke=false,Visible=false;
};
/** Read-only counts of the production pool for a short, affected-effects review. */
struct FFrontlineFuelVisualStatsV8214
{
    int32 Jet=0,Clothing=0,Wreck=0,Ember=0,Soot=0,AttachedClothing=0;
    int32 PoolUsed=0,PoolCapacity=0;
    float MinFrameRate=0,MaxFrameRate=0,MaxFlameWidthCm=0;
    int32 SourceRibbonsV8216=0,VolumeSootV8216=0;
    int32 FlameCoresV836=0,ArmourFlashesV836=0;
    int32 WreckSmokeSourcesV838=0,WreckSmokeVolumesV838=0;
};
struct FFrontlineFuelVolumeV8216
{
    FVector Source=FVector::ZeroVector;
    float Age=100,SourcePhase=0,Strength=0,Heat=0;
};
struct FFrontlineFlameCoreV836
{
    FVector Source=FVector::ZeroVector,Dimensions=FVector(90,80,140);
    float Age=100,Strength=0,Seed=0;
    TWeakObjectPtr<USkeletalMeshComponent> AttachedMesh;
    FName Bone=NAME_None;
    FVector LocalOffset=FVector::ZeroVector;
    // Wreck sources retain their hull/site identity; they cannot borrow a
    // nearby clothing core or another still-burning wreck's slot.
    TWeakObjectPtr<UStaticMeshComponent> WreckHullV8310;
    int32 WreckModelV8310=INDEX_NONE,WreckSiteV8310=INDEX_NONE;
    bool bWreckPrimaryV8310=false,bWreckRetiringV8310=false;
    bool bMeasuredOpeningV8311=false;
    FVector OpeningV8311=FVector::ZeroVector,LocalOpeningV8311=FVector::ZeroVector;
    uint64 FedFrameV8310=MAX_uint64,PresentedFrameV8310=MAX_uint64;
    uint64 FedFireTickV8310=MAX_uint64,FeedSequenceV8310=0,PresentedFeedSequenceV8310=0;
};
struct FFrontlineWreckSootV838
{
    FVector Source=FVector::ZeroVector;
    float FeedAge=100,FlowAge=0,Strength=0,Heat=0,Phase=0,WidthCm=180;
    int32 Site=INDEX_NONE,Model=INDEX_NONE;
    FString Profile,SiteName;
    TWeakObjectPtr<UStaticMeshComponent> Hull;
};
struct FFrontlineWreckLightV839
{
    FVector Source=FVector::ZeroVector;
    float FeedAge=100,Strength=0,Phase=0,Power=0;
    int32 Site=INDEX_NONE,Model=INDEX_NONE;
    TWeakObjectPtr<UStaticMeshComponent> Hull;
};
/** One bounded, reusable world service. Cosmetics never decide hits. */
UCLASS()
class WARFAREFRONT_API AFrontlineFuelFireV33:public AActor
{
    GENERATED_BODY()
public:
    AFrontlineFuelFireV33();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    static AFrontlineFuelFireV33* Get(UWorld* World);
    void EmitJet(const FVector& From,const FVector& Direction,float DT,float Seed);
    void EmitClothing(AWarfareFrontlineSoldier* Unit,float DT,float Strength);
    void EmitBurningWreck(const FVector& At,float DT,float Strength,float SourcePhase=0,bool bPrimary=true,float SmokeStrength=-1,float SourceScale=1,bool bLegacyLight=true,bool bDedicatedVolumeV8310=false);
    void EmitVehicleWreckV838(EFrontlineVehicle Type,const FTransform& Pose,float DT,float WreckAge,float Phase,UStaticMeshComponent* Hull=nullptr);
    void EmitRailCarWreckV838(const FTransform& Pose,float DT,int32 Car,float Phase,UStaticMeshComponent* Hull=nullptr);
    /** Actual component transforms and MID values for the focused Shipping review. */
    TSharedRef<FJsonObject> DescribeWreckSmokeV838() const;
    void RunWreckSmokeChecksV838(TFunctionRef<void(const FString&,bool)> Check) const;
    void FuelBurst(const FVector& At,float Fuel);
    void EmitArmourContactV836(const FVector& Position,const FVector& Normal,float Strength,bool bExplosive=true,
        EFrontlineArmourContactSoundV839 SoundKind=EFrontlineArmourContactSoundV839::Automatic);
    TSharedRef<FJsonObject> DescribeEffectsV839() const;
    void RunEffectsChecksV839(TFunctionRef<void(const FString&,bool)> Check) const;
    TSharedRef<FJsonObject> DescribeWreckFlameV8310() const;
    void RunWreckFlameChecksV8310(TFunctionRef<void(const FString&,bool)> Check) const;
    void RunWreckFlamePoolChecksV8310(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void Clear();
    int32 ActiveCount() const;
    FFrontlineFuelVisualStatsV8214 VisualStateV8214() const;
    bool IsReady() const;
    static constexpr int32 FlameBudget=192;
    static constexpr int32 SmokeBudget=112;
    static constexpr int32 LightBudget=2;
    static constexpr int32 WreckLightBudgetV839=24;
    static constexpr float WreckLightChangePerSecondV839=7200.f;
    static constexpr int32 VolumeBudgetV8216=6;
    static constexpr int32 FlameCoreBudgetV836=8;
    static constexpr int32 WreckFlameBudgetV8310=24;
    static constexpr int32 ContactBeginV8310=FlameCoreBudgetV836+WreckFlameBudgetV8310;
    static constexpr int32 ContactBudgetV836=4;
    static constexpr int32 WreckSmokeSourceBudgetV838=24;
    static constexpr int32 WreckSmokeVolumeBudgetV838=WreckSmokeSourceBudgetV838*FrontlineWreckSmokeV838::LayersPerSource;
private:
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Flames;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Smoke;
    UPROPERTY() TObjectPtr<UPointLightComponent> Lights[LightBudget];
    UPROPERTY() TObjectPtr<UPointLightComponent> WreckLightsV839[WreckLightBudgetV839];
    FFrontlineWreckLightV839 WreckLightStateV839[WreckLightBudgetV839];
    int32 WreckLightFeedsV839=0,WreckLightDropsV839=0,WreckLightAssignmentsV839=0;
    float MaximumWreckLightRateV839=0;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> VolumeSootV8216;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> VolumeMaterialsV8216;
    TArray<FFrontlineFuelVolumeV8216> VolumeStateV8216;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WreckSootV838;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> WreckSootMaterialsV838;
    TArray<FFrontlineWreckSootV838> WreckSootStateV838;
    int32 WreckSmokeFeedsV838=0,WreckSmokeEvictionsV838=0;
    int32 ActualFireTicksV838=0;
    float LastFireDeltaV838=0;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FlameCoresV836;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> FlameMaterialsV836;
    TArray<FFrontlineFlameCoreV836> FlameStatesV836;
    int32 WreckFlameFeedsV8310=0,WreckFlameDropsV8310=0,WreckFlameFallbacksV8310=0;
    uint64 WreckFireTickSequenceV8310=0,WreckFeedSequenceV8310=0;
    TArray<FFrontlineFuelParticleV33> Particles;
    int32 FlameCursor=0,SmokeCursor=0,EmissionSerial=0;
    bool bHasParticles=false;
    float LightEnergy[LightBudget]={},LightAge[LightBudget]={};
    FRandomStream Random{331918};
    FFrontlineFuelParticleV33* Add(const FVector& P,const FVector& V,float Life,float Size,float Stretch,bool bSmoke,float Turn=0);
    void Illuminate(const FVector& P,float Strength);
    void FeedWreckLightV839(const FVector& Source,EFrontlineVehicle Type,int32 Site,float Strength,float Phase,UStaticMeshComponent* Hull);
    bool TickWreckLightsV839(float DT);
    void FeedSootVolumeV8216(const FVector& Source,float Strength,float Heat,float SourcePhase);
    void TickSootVolumesV8216(float DT,const FVector& ViewForward);
    void FeedWreckSootV838(const FVector& Source,const FFrontlineWreckSmokeSiteV838& Site,const FString& Profile,
        EFrontlineVehicle Type,int32 SiteIndex,float Strength,float Heat,float Phase,UStaticMeshComponent* Hull);
    void TickWreckSootV838(float DT,const FVector& ViewForward);
    void FeedFlameCoreV836(const FVector& Source,const FVector& Size,float Strength,float Seed,
        USkeletalMeshComponent* Mesh=nullptr,FName Bone=NAME_None);
    void TickFlameCoresV836(float DT,const FVector& ViewForward);
    bool FeedWreckFlameV8310(const FVector& Source,const FVector& Size,float Strength,float Seed,
        UStaticMeshComponent* Hull,EFrontlineVehicle Type,int32 Site,bool bPrimary,
        const FVector& Opening=FVector::ZeroVector,bool MeasuredOpening=false);
    void PresentWreckFlameV8310(int32 Slot,const FVector& ViewForward);
};
