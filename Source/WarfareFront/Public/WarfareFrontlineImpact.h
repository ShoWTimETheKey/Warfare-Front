#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineImpact.generated.h"
class UStaticMeshComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class AWarfareFrontlineGameMode;
class AWarfareFrontlineGround;
class UFrontlineBlastCompositionV8221;
class FJsonObject;
struct FFrontlineImpactV38Review;
struct FFrontlineBlastReviewV831;
struct FFrontlineScorchStampV8449
{
    FVector Origin=FVector::ZeroVector;
    int32 Profile=0,Seed=0,Triangles=0,ClippedTriangles=0,WaterSamplesPruned=0,SourceTriangles=0;
    float Radius=0.f,MaximumSupportError=0.f,MaximumBuildMilliseconds=0.f;
    float MinimumClearance=0.f,MaximumPenetration=0.f;
    float ProjectionHalfDepthV84411=0.f;
    bool bClockwiseVerified=false;
    bool bSnow=false;
};
/** A bounded, cosmetic ground receiver. It neither excavates nor changes cover. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineScorchV8449 : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineScorchV8449();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    static AWarfareFrontlineScorchV8449* Get(UWorld* World,bool bCreate=true);
    void QueueImpact(const FVector& Point,int32 Profile);
    TSharedRef<FJsonObject> Describe() const;
    /** Only a disposable V8449 fixture can explicitly drain its small queue. */
    static void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    static constexpr int32 StampBudget=32;
private:
    bool BuildStamp(const FFrontlineScorchStampV8449& Request);
    void ClearStamps();
    UPROPERTY() TObjectPtr<UMaterialInterface> ScorchBase;
    UPROPERTY() TArray<TObjectPtr<UDecalComponent>> Stamps;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> StampMaterials;
    TArray<FFrontlineScorchStampV8449> Pending,State;
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    TWeakObjectPtr<AWarfareFrontlineGround> Ground;
    int32 Cursor=0,Queued=0,RejectedAir=0,RejectedWater=0,RejectedContact=0;
    bool bLegacyDryTraceChecked=false,bLegacyDryTraceHit=false;
    float LegacyDryTraceNormalZ=0.f,LegacyDryTraceErrorCm=0.f;
    int32 BoundOperation=INDEX_NONE;
    float LastHostTime=0.f,BuildWait=0.f;
};
struct FFrontlineSoilFragment
{
    FVector Velocity=FVector::ZeroVector,Position=FVector::ZeroVector,AngularVelocity=FVector::ZeroVector,Scale=FVector::OneVector;
    FRotator Rotation=FRotator::ZeroRotator;
    bool bLanded=false;
    int32 Bounces=0;
};
/** Authored volume bakes, flash and instanced ballistic soil; cosmetic only. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineImpact : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineImpact();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    /** Cosmetic profile only. Called immediately after spawning; never changes damage. */
    void ConfigureBlast(bool bGrenade,bool bMortar,const FVector& IncomingDirection=FVector::ZeroVector);
    void ConfigureMineV8214(float FootprintRadiusCm=600.f);
    /** A single strafing bullet hitting soil: independent budget, no flash. */
    void ConfigureBulletStrike();
    void ConfigureAirburstV64(float FootprintRadius=1200.f,const FVector& IncomingDirection=FVector::ZeroVector);
    void SetCasualtyDustV8211(int32 Casualties);
    void SetCasualtyDustV8216(const TArray<FVector>& CasualtyFeet);
    void ConfigureStrafeStrike(const FVector& IncomingDirection);
    /** Tankgewehr soil/armour impact; no explosive radius or explosive flash. */
    void ConfigureHeavyBulletV8215(const FVector& IncomingDirection,bool bArmour=false);
    bool IsVisualReady() const;
    bool IsFlashVisible() const;
    int32 GetVisibleSmokeCount() const;
    float GetVisualAgeSeconds() const { return Age; }
    int32 GetBlastProfileIndex() const { return BlastKind; }
    int32 GetBallisticFragmentCount() const { return Soil.Num(); }
    bool HasFastFragmentsV835() const;
    float GetMineFootprintRadiusCmV8214() const { return MineRadiusV8214; }
    int32 GetConfiguredVolumeCountV8214() const { return VolumePuffsV40; }
    static void RunChecksV8216(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<FJsonObject> DescribeCompositionV8221() const;
    void CheckCompositionV8221(TFunctionRef<void(const FString&,bool)> Check) const;
    static void RunReconfigurationChecksV8221(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    static void RunDebrisChecksV835(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    static TSharedRef<FJsonObject> DescribeFragmentShutterV835(UWorld* World);
    static constexpr int32 HeavyImpactBudget=12;
    static constexpr int32 BulletImpactBudget=16;
    static constexpr int32 FlashLightBudget=3;
    static constexpr int32 CardBudget=16;
    static constexpr int32 VolumeBudgetV40=8;
private:
    friend struct FFrontlineImpactV38Review;
    friend struct FFrontlineBlastReviewV831;
    friend struct FFrontlineEffectsSafetyV844Review;
    void ConfigureProfile(int32 Profile);
    int32 ReservedVolumeCountV8216() const;
    void UpdateFlash(const FRotator& Billboard);
    void UpdateSoil(float DeltaSeconds);
    void CommitFragmentTransformsV835();
    void UpdateClodTrails(const FRotator& Billboard);
    void UpdateMineV8214(float DeltaSeconds,const FRotator& Billboard);
    float FitWaterContactV831(int32 PuffIndex,const FVector& Centre,const FVector& Dimensions,float VisualDelta);
    UPROPERTY() TObjectPtr<UMaterialInterface> EjectaBase;
    UPROPERTY() TObjectPtr<UMaterialInterface> RollingBase;
    UPROPERTY() TObjectPtr<UMaterialInterface> VolumeBaseV40;
    UPROPERTY() TObjectPtr<UMaterialInterface> PenetrationBaseV8215;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PenetrationMaterialV8215;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Puffs;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Flash;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Fragments;
    UPROPERTY() TObjectPtr<UPointLightComponent> FlashLight;
    UPROPERTY() TObjectPtr<UFrontlineBlastCompositionV8221> CompositionV8221;
    TArray<FFrontlineSoilFragment> Soil;
    TArray<FTransform> FragmentTransformsV835,PreviousFragmentTransformsV835;
    int32 FragmentUploadBatchesV835=0;
    bool bReviewLegacyDebrisV835=false;
    TArray<FVector> PuffDrift;
    TArray<float> PuffVariation;
    // Cosmetic contact response only; reset on profile binding. It shares
    // the impact's visual clock and is never saved or used by gameplay.
    TArray<float> WaterContactLiftV831,WaterContactTargetV831;
    TArray<uint8> WaterContactInitializedV831;
    // Profile-local ownership; zero retains this slot's exact old producer.
    TArray<uint8> ParticulateSlotsV84412;
    TWeakObjectPtr<AWarfareFrontlineGround> WaterContactGroundV831;
    float Age=0;
    double ConfiguredAt=0;
    bool bFirstTick=true;
    bool bProfileConfigured=false;
    bool bFlashLightAllocated=false;
    bool bReviewLegacyV831=false;
    FVector StrikeDirection=FVector::ZeroVector;
    TArray<FVector> ShrapnelGroundV8211;
    TArray<float> ShrapnelArrivalV8211;
    TArray<FVector> CasualtyDustSourcesV8216;
    float ShrapnelRadiusV8211=1200.f,BloodLoadV8211=0.f;
    float MineRadiusV8214=600.f;
    bool bHeavyBulletV8215=false,bArmourBulletV8215=false,bStrafeBulletV8215=false;
    int32 BlastKind=0,EarthPuffs=4,WashPuffs=4,CorePuffs=12,ActivePuffs=16;
    int32 VolumePuffsV40=0;
};

namespace FrontlineImpactV38
{
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
/** Isolated, cosmetic phase capture of the actual production actors; never applies damage. */
bool SetupVisualFixture(AWarfareFrontlineGameMode* Battle,float PhaseSeconds);
}
namespace FrontlineImpactV40
{
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
bool SetupVisualFixture(AWarfareFrontlineGameMode* Battle,float PhaseSeconds);
}
namespace FrontlineImpactV831
{
/** Exact production profiles, frozen at deterministic visual ages. Strict
 * disposable review only; no damage, combat units or host-clock mutation.
 * Appends one actor; callers explicitly ClearVisualFixture between phases. */
AWarfareFrontlineImpact* SetupVisualFixture(AWarfareFrontlineGameMode* Battle,int32 Profile,float PhaseSeconds,bool bLegacy,const FVector& Origin,int32 Casualties=2);
TSharedRef<FJsonObject> DescribeVisualFixture(AWarfareFrontlineGameMode* Battle);
void ClearVisualFixture(AWarfareFrontlineGameMode* Battle);
/** Isolation only: hide flat puff cards (1), retained puff volumes (2), or
 * short composition volume proxies (3). Recreate the fixture between calls.
 * Returns the number of previously visible components hidden, -1 if refused. */
int32 HideVisualFixtureLayer(AWarfareFrontlineGameMode* Battle,int32 Layer);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
