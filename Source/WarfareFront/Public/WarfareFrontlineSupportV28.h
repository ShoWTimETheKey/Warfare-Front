#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineSupportV28.generated.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class FJsonObject;
enum class EFrontlineSupport : uint8;
namespace FrontlineGasV35
{
    // Hazard risk is separate from equipment protection. Preserve the prior
    // unresearched exposure budget without equipping invisible default masks.
    inline constexpr float CoveredExposureChance=.05f,OpenExposureChance=.028f;
    inline constexpr float ExposureIntervalSeconds=2.8f;
}
/** Committed orders observe game time and expire with their encounter. */
UCLASS()
class WARFAREFRONT_API AFrontlineSupportMissionV28 : public AActor
{
    GENERATED_BODY()
public:
    AFrontlineSupportMissionV28();
    bool Configure(AWarfareFrontlineGameMode* InBattle,EFrontlineSupport InSupport,const FVector& InTarget,int32 InTeam);
    virtual void Tick(float DeltaTime) override;
    int32 RoundsFired=0;
private:
    friend struct FFrontlineSupportV836Review;
    friend struct FFrontlineAirSupportV8220Review;
    friend struct FFrontlineCombatV8217Review;
    friend struct FFrontlineGameplayReviewV75;
    friend struct FFrontlineCombatV63Review;
    friend struct FFrontlineSupportV40Review;
    friend struct FFrontlineRulesV8215Review;
    friend struct FFrontlineCombatV35Review;
    friend TSharedRef<FJsonObject> RunFrontlineV28SupportChecks(AWarfareFrontlineGameMode* Battle);
    friend void RunFrontlineSupportChecksV37(AWarfareFrontlineGameMode*,TFunctionRef<void(const TCHAR*,bool)>);
    void EmitGas(float Time);
    void FireAntitank(float Time);
    void Strafe(float Time,float ObservationSeconds=0.f);
    void ResolveStrafeRounds();
    struct FStrafeImpactV37 {FVector Origin,Point;float ImpactAt=0;int32 Seed=0;float LaunchedAtV8220=0;};
    TArray<FStrafeImpactV37> PendingStrafe;
    UPROPERTY() TObjectPtr<AWarfareFrontlineGameMode> Battle;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Visuals;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    EFrontlineSupport Support;
    FVector Target=FVector::ZeroVector;
    FVector GunOriginV8215=FVector::ZeroVector;
    FGuid Encounter;
    int32 Team=0;
    float RequestedAt=0,Arrival=0,NextAction=0,LastTime=0;
    bool Warned=false;
    bool bDrivenOffV59=false;
    float DrivenOffAtV59=-1;
    uint8 GasBurstMask=0;
    int32 StrafeSlotsV8220[2]={0,0};
    FRandomStream Random{281917};
};
void TickFrontlineExtendedSupportAI(AWarfareFrontlineGameMode* Battle);
void StartFrontlineV28Review(AWarfareFrontlineController* Controller);
TSharedRef<FJsonObject> RunFrontlineV28SupportChecks(AWarfareFrontlineGameMode* Battle);
void RunFrontlineSupportChecksV37(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
void RunFrontlineSupportChecksV40(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
void RunFrontlineSupportRulesChecksV8215(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
TSharedRef<FJsonObject> RunFrontlineSupportV836Review(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
bool PrepareFrontlineGasVisualV836(AWarfareFrontlineGameMode* Battle,const FVector& Point);
