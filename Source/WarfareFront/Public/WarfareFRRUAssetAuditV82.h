#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WarfareFRRUAssetAuditV82.generated.h"
class USkeletalMesh;
class UPhysicsAsset;
class UStaticMesh;
class UTexture2D;
class UAnimSequence;

// Actual Editor model snapshot used around metadata-only slot restoration.
// This does not approve skin deformation, camera appearance or compression.
UCLASS()
class WARFAREFRONT_API UWarfareFRRUAssetAuditV82 : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Complete the engine's Editor compression job before inspecting compressed
    // poses. Never substitutes raw animation or changes the shared Skeleton.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static bool PrepareAnimationCompression(UAnimSequence* Sequence);
    // Exact G98, MG 08 and MG 08/15 mechanical pairs; human67 stays separate.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static bool PrepareMechanicalAnimationCompression(UAnimSequence* Sequence,USkeletalMesh* Mesh);
    // Read-only check of the isolated Editor persistence boundary.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotPersistenceReviewBoundary();
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotSkeletalMesh(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotStaticMesh(UStaticMesh* Mesh);
    // Exact Vaux old/new foundation assets only; actual immutable collision
    // data, including fields deliberately unavailable to Python reflection.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotStaticCollisionV87(UStaticMesh* Mesh);
    // Inspect actual authored basis first; only eligible isolated candidates
    // may repair their bounded invalid corners. Null Candidate is read-only.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString RepairGermanIngressSourceBasisV82(UStaticMesh* Source,UStaticMesh* Candidate);
    // Editor technical preflight, exclusively for registered CorrectiveV1
    // world-render parts. Repairs invalid corner basis without changing the
    // physical scene, visible surface, healthy basis or material bindings.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString RepairWorldRenderBasisV826(UStaticMesh* Mesh);
    // One saved Vaux source and one new V87 namespace only. Null candidate is
    // read-only; no FBX roundtrip and no widening of the older repair gates.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString RepairVauxSourceBasisV87(UStaticMesh* Source,UStaticMesh* Candidate);
    // Only the exact 60 bad current dressings pinned by the V87 actual audit.
    // Null Candidate is read-only; old repair gates and scene bindings stay.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString RepairDressingSourceBasisV87(UStaticMesh* Source,UStaticMesh* Candidate);
    // Null candidate and Part=-1 return a read-only sixteen-metre Vaux plan.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString PartitionVauxSourceV87(UStaticMesh* Source,UStaticMesh* Candidate,int32 Part=-1);
    // Compare a bounded set of authored anchors with the actual built LOD0.
    // Reads render positions, vertex colours and UVs; never moves a mesh.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotStaticMeshCorrespondence(UStaticMesh* Mesh,const TArray<FVector>& AssetLocalPoints,double MaximumErrorCm);
    // Exact source/platform G16 lookup data; this does not certify water art.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotHeightfieldTexture(UTexture2D* Texture);
    // Read-only source/platform/mip evidence for isolated material derivatives.
    // Does not resize, reimport, rebuild, save or change the supplied texture.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotSurfaceTextureV87(UTexture2D* Texture);
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static bool AppendVerifiedUnusedStaticMaterialSlot(UStaticMesh* Mesh,FName Name,int64 SourceTriangles);
    // Read the engine's actual bodies and constraints; Python does not expose
    // these instanced arrays in UE 5.8. No objects are changed or saved.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static FString SnapshotPhysicsAsset(UPhysicsAsset* Asset,USkeletalMesh* Mesh);
    // Caller proves the actual FBX triangle count under pinned input gates.
    // Existing slots and nonzero counts are always rejected in this bridge.
    UFUNCTION(BlueprintCallable,Category="FRRU|Audit")
    static bool AppendVerifiedUnusedMaterialSlot(USkeletalMesh* Mesh,FName Name,int64 SourceTriangles);
};
