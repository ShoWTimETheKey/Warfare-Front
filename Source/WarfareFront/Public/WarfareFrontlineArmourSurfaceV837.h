#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineArmourSurfaceV837.generated.h"

class UDecalComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class AFrontlineTank;

UCLASS()
class WARFAREFRONT_API AFrontlineArmourSurfaceV837:public AActor
{
    GENERATED_BODY()
public:
    AFrontlineArmourSurfaceV837();
    virtual void BeginPlay()override;
    virtual void Tick(float DT)override;
    static AFrontlineArmourSurfaceV837* Get(UWorld* World);
    void EmitContact(const FVector& Position,const FVector& Normal,float Strength,bool Explosive);
    void Clear();
    bool IsReady()const;
    TSharedRef<class FJsonObject> Describe()const;
    void RunTracksChecksV8311(TFunctionRef<void(const FString&,bool)> Check)const;
    // Disposable renderer diagnosis only: 1 high-contrast, 2 hidden, 0 restore.
    TSharedRef<class FJsonObject> SetImprintDiagnosticV8311(int32 Mode);
private:
    static constexpr int32 TrackBudget=768;
    static constexpr int32 ScarBudget=24;
    static constexpr int32 SpallBudget=64;
    static constexpr int32 DustBudget=8;
    UPROPERTY() TArray<TObjectPtr<UDecalComponent>> Tracks;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> TrackMaterialsV838;
    UPROPERTY() TArray<TObjectPtr<UDecalComponent>> Scars;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> ScarMaterials;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> DustMaterials;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Dust;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Spall;
    struct FFragment{FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector,Scale=FVector::OneVector;FQuat Rotation=FQuat::Identity;float Age=10,Life=.45f,Heat=0;bool Visible=false;};
    struct FCloud{FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector;float Age=10,Seed=0,Strength=0,RadiusScale=1,HeightScale=1;};
    struct FScar{float Age=100;};
    struct FTrail{FVector Position=FVector::ZeroVector;float Distance=0,SidePhase[2]={0,0};bool Valid=false;};
    FFragment Fragments[SpallBudget];FCloud Clouds[DustBudget];FScar ScarState[ScarBudget];
    TMap<TWeakObjectPtr<AFrontlineTank>,FTrail> Last;
    FGuid Encounter;
    int32 NextTrack=0,NextScar=0,NextSpall=0,NextDust=0;
    int32 TrackPairs=0,Contacts=0,AcceptedScars=0,RejectedGround=0,DustEmissions=0;
    int32 SoftGroundImprintsV8311=0,HardGroundImprintsV8311=0,SnowImprintsV8311=0;
    float TrackClock=0,SurfaceWetnessV8311=.65f;
    FRandomStream Random{837042};
    void TickVehicles(float DT);
    void StampTracks(AFrontlineTank* Tank,const FVector& Before,const FVector& After,float Travel);
    void EmitDust(const FVector& At,const FVector& Velocity,float Strength,float RadiusScale=1,float HeightScale=1);
};
