#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "WarfareTacticalSlice.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USkeletalMesh;
class UAnimationAsset;
class UMaterialInterface;
class ACameraActor;
class USoundBase;
class UAudioComponent;
class AWarfareTacticalGameMode;

/** Height-field terrain, shared by the rendered surface and infantry movement. */
UCLASS()
class WARFAREFRONT_API AWarfareTacticalGround : public AActor
{
    GENERATED_BODY()
public:
    AWarfareTacticalGround();
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Tactical") void RebuildTerrain();
    UFUNCTION(BlueprintPure, Category="Tactical") static float HeightAt(float X, float Y);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tactical") TObjectPtr<UInstancedStaticMeshComponent> Sandbags;
    UPROPERTY(EditAnywhere, Category="Tactical") TObjectPtr<UMaterialInterface> GroundMaterial;
};

/** Uses an imported soldier mesh only. No geometric replacement soldier exists. */
UCLASS()
class WARFAREFRONT_API AWarfareTacticalUnit : public AActor
{
    GENERATED_BODY()
public:
    AWarfareTacticalUnit();
    virtual void Tick(float DeltaSeconds) override;
    void Initialize(AWarfareTacticalGameMode* InBattle, int32 InTeam, int32 InSquad);
    bool MoveTo(const FVector& Destination);
    void ReceiveShot(float Damage, float Pressure);
    bool IsAlive() const { return Health > 0.f; }
    bool IsMoving() const { return RouteIndex < Route.Num(); }
    FVector Feet() const { return GetActorLocation() - FVector(0, 0, 88); }
    FString Status() const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Capsule;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Soldier;
    UPROPERTY(Transient) TObjectPtr<AWarfareTacticalGameMode> Battle;
    int32 Team = 0;
    int32 Squad = 0;
    float Health = 100;
    float Suppression = 0;
    float DistanceTravelled = 0;
    float AnimationTimeAdvanced = 0;
    bool bFireEnabled = true;
private:
    TArray<FVector> Route;
    int32 RouteIndex = 0;
    float ShotClock = 0;
    float DecisionClock = 0;
    float FirePoseClock = 0;
    float LastAnimationTime = 0;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> Playing;
    void SetMotion(UAnimationAsset* Animation, bool bLoop = true);
};

struct FWarfareTacticalTracer
{
    FVector Start;
    FVector End;
    float Life = .13f;
};

/** One local 14-soldier encounter. Both teams use the shared scan-based study character. */
UCLASS()
class WARFAREFRONT_API AWarfareTacticalGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AWarfareTacticalGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void ResetEncounter();
    bool BuildRoute(const FVector& From, const FVector& To, TArray<FVector>& OutRoute) const;
    bool HasSight(const AWarfareTacticalUnit* From, const AWarfareTacticalUnit* To) const;
    float CoverAt(const FVector& Position) const;
    AWarfareTacticalUnit* AcquireTarget(const AWarfareTacticalUnit* Unit) const;
    void Fire(AWarfareTacticalUnit* Shooter, AWarfareTacticalUnit* Target);
    void ThinkEnemy(AWarfareTacticalUnit* Unit);
    void OrderSquad(int32 Squad, const FVector& Target);
    void ToggleFire(int32 Squad);
    int32 AliveCount(int32 Team, int32 Squad = INDEX_NONE) const;
    void PlayOrderSound(bool bMove);
    UPROPERTY(EditDefaultsOnly, Category="Presentation") float MeshYawOffset = -90.f;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") TSoftObjectPtr<USkeletalMesh> SoldierAsset;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") TSoftObjectPtr<UAnimationAsset> IdleAsset;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") TSoftObjectPtr<UAnimationAsset> WalkAsset;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") TSoftObjectPtr<UAnimationAsset> RunAsset;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") TSoftObjectPtr<UAnimationAsset> AimAsset;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") TSoftObjectPtr<UAnimationAsset> FireAsset;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> LoadedSoldier;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> IdleAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> WalkAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> RunAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> AimAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> FireAnimation;
    UPROPERTY(Transient) TArray<TObjectPtr<AWarfareTacticalUnit>> Units;
    UPROPERTY(Transient) TObjectPtr<ACameraActor> BattleCamera;
    TArray<FWarfareTacticalTracer> Tracers;
    FVector Objective = FVector(0, 750, 0);
    float HeldSeconds = 0;
    float ElapsedSeconds = 0;
    bool bEncounterEnded = false;
    bool bWon = false;
    FString AssetFailure;
    FString LastOrder;
    int32 ShotsFired = 0;
    float DamageApplied = 0;
    float PeakSuppression = 0;
private:
    UPROPERTY(Transient) TObjectPtr<USoundBase> RifleSound;
    UPROPERTY(Transient) TObjectPtr<USoundBase> RifleSoundAlternate;
    UPROPERTY(Transient) TObjectPtr<USoundBase> SelectSound;
    UPROPERTY(Transient) TObjectPtr<USoundBase> MoveSound;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> WindAudio;
    TArray<FBox> CoverBoxes;
    TArray<FVector> CoverPositions;
    FRandomStream Random;
    bool IsBlocked(const FVector& Position) const;
    void BeginVerification();
};

UCLASS()
class WARFAREFRONT_API AWarfareTacticalController : public APlayerController
{
    GENERATED_BODY()
public:
    AWarfareTacticalController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    int32 SelectedSquad = 0;
    FVector OrderMarker = FVector::ZeroVector;
    float OrderMarkerTime = 0;
    // Mouse/keyboard input and the opt-in regression path use these same commands.
    void SelectSquad(int32 Squad);
    void CommandMove(const FVector& Destination);
    void TogglePause();
    void Restart();
private:
    FVector CameraFocus = FVector(0, -1100, 80);
    float CameraDistance = 6000;
    void SelectAtCursor();
    void MoveAtCursor();
    void SelectAlpha();
    void SelectBravo();
    void ToggleFire();
    void ZoomIn();
    void ZoomOut();
};

UCLASS()
class WARFAREFRONT_API AWarfareTacticalHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void WorldRing(const FVector& Position, float Radius, FLinearColor Color, float Width = 1.5f);
};
