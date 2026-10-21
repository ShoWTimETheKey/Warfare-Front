#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/AssetUserData.h"
#include "WarfareFrontlineGround.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
enum class EFrontlineMissionTerrainV32 : uint8;

/** Offline hydraulic labels travelling with their exact rendered water mesh. */
UCLASS()
class WARFAREFRONT_API UFrontlineWaterPlanesV72 : public UAssetUserData
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Schema=72;
    UPROPERTY() int32 Operation=-1;
    UPROPERTY() int32 Style=-1;
    UPROPERTY() int32 DepthKey=0;
    UPROPERTY() int32 GeometryVersion=0;
    UPROPERTY() FVector2D Origin=FVector2D::ZeroVector;
    UPROPERTY() float CellSize=40.f;
    UPROPERTY() int32 Columns=0;
    UPROPERTY() int32 Rows=0;
    UPROPERTY() TArray<int32> CellGroups;
    UPROPERTY() TArray<float> Levels;
    UPROPERTY() TArray<float> LegacyMeanLevels;
    UPROPERTY() TArray<float> SpillLevels;
    UPROPERTY() TArray<int32> GroupColours;
    // Empty on the accepted all-standing meshes. 0=standing, 1=flowing reach.
    UPROPERTY() TArray<int32> GroupKinds;
    UPROPERTY() int32 LysComposition=0;
    UPROPERTY() int32 WetCells=0;
    UPROPERTY() int32 RetainedCells=0;
    UPROPERTY() int32 SpillBoundarySamples=0;
    UPROPERTY() int32 PhysicalSpillSamples=0;
    UPROPERTY() int32 ClippedSubmergedBoundaries=0;
    UPROPERTY() bool bValid=false;
};

// Spatial masks describe the actual displaced landform after bank, rail and
// standing-water protection. They are shared with traversal and cover rules.
struct FFrontlineTerrainFeaturesV54
{
    int32 Operation=0;
    EFrontlineMissionTerrainV32 Style=static_cast<EFrontlineMissionTerrainV32>(0);
    float RidgeCrest=0,WoodlandGully=0,MarshChannel=0,CanalCut=0,RailShoulder=0,RailWash=0,Ruins=0,RuinCellar=0,VillageRoad=0,SnowDrift=0;
    // Farm operations: traversable negative drainage/lane relief, positive
    // broken boundary berm, and churned vehicle approaches respectively.
    float FieldDitch=0,FieldBoundary=0,TankChurn=0;
    float ReliefCm=0;
};

/** The trench profile is shared by rendering, collision and every soldier's feet. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineGround : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineGround();
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Frontline") void RebuildTerrain();
    UFUNCTION(BlueprintCallable, Category="Frontline|Terrain") static FString BakeTerrainProfileV43(int32 OperationIndex);
    UFUNCTION(BlueprintCallable, Category="Frontline|Terrain") static FString BakeWaterProfileV44(int32 OperationIndex);
    UFUNCTION(BlueprintCallable, Category="Frontline|Terrain") static FString CloneWaterEdgeCoverageV75(int32 OperationIndex);
    UFUNCTION(BlueprintCallable, Category="Frontline|Terrain") static FString CloneWaterEdgeCoverageV83(int32 OperationIndex);
    UFUNCTION(BlueprintPure, Category="Frontline|Terrain") FString DescribeTerrainV43() const;
    void SyncTerrainMaterialV43();
    bool BindBakedTerrainV43();
    void BindWaterV44();
    UFUNCTION(BlueprintPure, Category="Frontline") static float HeightAt(float X, float Y);
    static bool QueryWaterSurfaceV51(float X,float Y,float& Level,float& Depth,float* BasinIdentity=nullptr);
    static void RunChecksV51(TFunctionRef<void(const FString&,bool)> Check);
    static FString DescribeWaterV72();
    static void RunChecksV72(TFunctionRef<void(const FString&,bool)> Check);
    static FFrontlineTerrainFeaturesV54 QueryTerrainFeaturesV54(float X,float Y);
    static void RunChecksV54(TFunctionRef<void(const FString&,bool)> Check);
    static FTransform TrackSupportAtV35(const FVector& Position,const FRotator& Previous,float Yaw,float Rear,float Front,float HalfWidth,float SoleHeight,float Seconds);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Frontline") TObjectPtr<UInstancedStaticMeshComponent> Sandbags;
    UPROPERTY(EditAnywhere, Category="Frontline") TObjectPtr<UMaterialInterface> GroundMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> BakedTerrainV43;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BasinWaterV44;
};
