#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineWoundFX.generated.h"

class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;
class USkeletalMesh;
struct FStreamableHandle;
class UPrimitiveComponent;
class FJsonObject;

struct FFrontlineBloodBurst
{
    FVector Position=FVector::ZeroVector,Direction=FVector::ZeroVector;
    FVector Velocity=FVector::ZeroVector,LandingPosition=FVector::ZeroVector;
    float Age=10,Size=1,Seed=0;
    float LandingAge=1,MarkSize=0,MarkOpacity=0;
    bool bDepositPending=false,bFatalDeposit=false,bBlast=false;
};
struct FFrontlineBloodMark
{
    float Age=200,Opacity=0,SpreadSeconds=0,MassV84411=0,SpreadV84411=0;
    float RetirementAgeV86=-1.f;
    uint32 IdentityV84411=0;bool bMassDrivenV84411=false;
};
struct FFrontlineBloodRunoffV828
{
    FVector Start=FVector::ZeroVector,End=FVector::ZeroVector;
    float Age=200,Delay=0,Duration=1,Mass=0;
    bool bWaterEntry=false,bDelivered=false,bTerminal=false;
    int32 NextSlotV84411=INDEX_NONE;uint32 ChainV84411=0;
    float FilmMassV84411=0;bool bStartedV84411=false;
    float RetirementAgeV86=-1.f;
};
struct FFrontlineCorpseSeepV53
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;
    FVector LastBone=FVector::ZeroVector;
    float Age=0.f,Stable=0.f,Remaining=1.2f,DepositAge=0.f;
    bool bGroundMarked=false,bDryDrain=false;
    int32 GroundMarkV84411=INDEX_NONE;uint32 GroundMarkIdentityV84411=0;
    FVector GroundDepositV84411=FVector::ZeroVector;
};
struct FFrontlineDetachedPart
{
    FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector,AngularVelocity=FVector::ZeroVector;
    FQuat Rotation=FQuat::Identity;
    float Age=100,Radius=8;
    bool bLanded=false;
};

/** One bounded world pool: 24 short bursts, 64 stains and 8 single limb pieces. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineWoundFX : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineWoundFX();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    static AWarfareFrontlineWoundFX* Get(UWorld* World);
    void FatalHit(AWarfareFrontlineSoldier* Unit,const FVector& Direction,bool bBlast);
    void FatalHeavyBulletV75(AWarfareFrontlineSoldier* Unit,const FVector& Direction);
    bool ShowBlastLoss(AWarfareFrontlineSoldier* Unit);
    void PrepareRoleV8216(USkeletalMesh* Original);
    bool WaitForPreparedRoleV8216(USkeletalMesh* Original);
    bool ResetFocusedReviewV8216();
    bool HasLimbLossV8216(const AWarfareFrontlineSoldier* Unit) const;
    FString LossVariantV8216(const AWarfareFrontlineSoldier* Unit) const;
    int32 ActiveBodyMistV8216() const;
    static void RunChecksV828(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    bool ReviewDepositsV828(FVector& Ground,const FVector& Burst);
    static void RunBloodChecksV84411(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    TSharedRef<FJsonObject> DescribeBloodV84411() const;
    bool ReviewBloodDepositsV84411(FVector& Ground);
    static void RunBloodChecksV86(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    bool ReviewBloodCaptureV86(FVector& Ground);
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Mists;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> MistMaterials;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BlastCloudsV828;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> BlastCloudMaterialsV828;
    TArray<FFrontlineBloodBurst> BlastCloudStateV828;
    int32 BlastCloudCursorV828=0;
    UPROPERTY() TArray<TObjectPtr<UDecalComponent>> Marks;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> MarkMaterials;
    UPROPERTY() TArray<TObjectPtr<UDecalComponent>> RunoffDecalsV828;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> RunoffMaterialsV828;
    TArray<FFrontlineBloodRunoffV828> RunoffsV828;
    int32 RunoffCursorV828=0;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> CasualtyViews;
    TMap<FName,TSharedPtr<FStreamableHandle>> LimbPreloadsV8216;
    UPROPERTY() TArray<TObjectPtr<UPrimitiveComponent>> LimbPSOWarmupV8216;
    TArray<FFrontlineBloodBurst> Bursts;
    TArray<FFrontlineBloodMark> MarkState;
    TArray<FFrontlineDetachedPart> PartState;
    TArray<FFrontlineCorpseSeepV53> CorpseSeepsV53;
    TSet<TWeakObjectPtr<AWarfareFrontlineSoldier>> RegisteredCorpseSeepsV86;
    FGuid SeepEncounterV53;
    float SeepClockV53=0.f;
    int32 BurstCursor=0,MarkCursor=0,FatalMarkCursor=0,HitsShown=0,LossesShown=0;
    float LastBattleTime=0;
    bool bReady=false;
    uint32 BloodMarkIdentityV84411=0,BloodChainIdentityV84411=0;
    int32 AddMark(const FVector& Position,const FVector& Direction,float Size,float Opacity,int32 Seed,bool bFatalDeposit=false,bool bSeep=false,float PoolMassV84411=0);
    void ClearPresentation();
    void TickCorpseSeepsV53(float DeltaSeconds);
    float AddRunoffV828(const FVector& Position,int32 Seed,float Mass,bool* bWaitingForCapacityV86=nullptr);
    void TickRunoffsV828(float DeltaSeconds);
    bool IsMarkSourceOwnedV86(int32 Slot) const;
    int32 ReserveMarkV86(bool bFatalDeposit);
    void TickMarksV86(float DeltaSeconds);
};

