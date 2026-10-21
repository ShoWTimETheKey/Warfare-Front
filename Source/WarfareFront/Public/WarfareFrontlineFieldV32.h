#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class UAnimationAsset;
class FJsonObject;
struct FFrontlineStrike;
enum class EFrontlineDamageKind : uint8;
enum class EFrontlineMissionTerrainV32 : uint8;
struct FFrontlineFlameStateV82
{
    float HostElapsed=0,StartedAt=0,JetEndsAt=0,ReadyAt=0,FuelCharges=0;
    float BurnSeconds=0,BurnAge=0,FuelBurstAt=-1;
    FVector Direction=FVector::ZeroVector;
    bool Alive=false,Bracing=false,JetActive=false,FuelIgnited=false,FuelSpent=false;
};
enum class EFrontlineMineWorkPhaseV835 : uint8 { None,Approaching,Waiting,Working };
struct FFrontlineMineWorkStateV835
{
    EFrontlineMineWorkPhaseV835 Phase=EFrontlineMineWorkPhaseV835::None;
    FVector MineCentre=FVector::ZeroVector,StandPoint=FVector::ZeroVector;
    FVector ToolContact=FVector::ZeroVector,Direction=FVector::ForwardVector;
    float HostElapsed=0,WorkSeconds=0,Progress=0;
    bool DesignatedWorker=false;
};
namespace FrontlineFieldV32
{
    /** In-memory report probe only; restores private unit/field event state. */
    void WithScopedCombatEventsV839(AWarfareFrontlineGameMode* Battle,TFunctionRef<void()> Work);
    TArray<FVector> MineBeltPointsV59(const AWarfareFrontlineGameMode* Battle);
    float FuelCookoffRemainingV41(const AWarfareFrontlineSoldier* Unit,bool& Ignited,bool& Spent);
    WARFAREFRONT_API void Reset(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void Begin(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void Tick(AWarfareFrontlineGameMode* Battle,float DT);
    WARFAREFRONT_API void InitializeUnit(AWarfareFrontlineSoldier* Unit);
    WARFAREFRONT_API void TickUnit(AWarfareFrontlineSoldier* Unit,float DT);
    WARFAREFRONT_API float MovementScale(AWarfareFrontlineSoldier* Unit);
    bool HasReachableHazardV52(const AWarfareFrontlineSoldier* Unit);
    bool MineWorkDestinationV8215(const AWarfareFrontlineSoldier* Unit,FVector& Destination);
    // Local mine work already supplies a stable physical point; squad terrain
    // detours must not offset it a second time.
    bool HasMineApproachV833(const AWarfareFrontlineSoldier* Unit);
    bool IsMineWorkingV8215(const AWarfareFrontlineSoldier* Unit);
    // Presentation observes the existing service job and host clock. The probe
    // touches nearby ground; it does not imply reaching the distant charge.
    bool QueryMineWorkV835(const AWarfareFrontlineSoldier* Unit,FFrontlineMineWorkStateV835& State);
    WARFAREFRONT_API void RunEngineerMineChecksV833(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunClearanceChecksV8311(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    AWarfareFrontlineSoldier* PrepareClearanceViewV8311(AWarfareFrontlineGameMode* Battle,float WorkSeconds);
    // Last stage of the disposable review only; the host captures this actual
    // working actor, holds its clock, then exits without resuming the fixture.
    WARFAREFRONT_API AWarfareFrontlineSoldier* PrepareMineWorkVisualReviewV835(AWarfareFrontlineGameMode* Battle);
    void RunPioneerChecksV52(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API bool FlameDischarge(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Target);
    WARFAREFRONT_API bool FlameBraceDirectionV33(AWarfareFrontlineSoldier* Unit,FVector& Direction);
    WARFAREFRONT_API bool IsFlameOperator(const AWarfareFrontlineSoldier* Unit);
    // Read-only host times; presentation never advances fuel or the fire cycle.
    WARFAREFRONT_API bool QueryFlameStateV82(const AWarfareFrontlineSoldier* Unit,FFrontlineFlameStateV82& State);
    WARFAREFRONT_API void OnFatalHitV33(AWarfareFrontlineSoldier* Unit,EFrontlineDamageKind Kind,int32 AttackingTeam,const FVector& ForceDirection=FVector::ZeroVector);
    WARFAREFRONT_API UAnimationAsset* BurningFallV33(AWarfareFrontlineSoldier* Unit);
    WARFAREFRONT_API float GasHitScale(AWarfareFrontlineSoldier* Unit);
    WARFAREFRONT_API void OnBlast(AWarfareFrontlineGameMode* Battle,const FFrontlineStrike& Strike);
    void RunBlastMineChecksV84411(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API void BeginOutcome(AWarfareFrontlineGameMode* Battle,int32 Winner);
    WARFAREFRONT_API float TrenchCentreX(int32 Index,float Y);
    WARFAREFRONT_API float TerrainLift(float X,float Y,float BaseHeight);
    void TerrainProfileV43(int32& OperationIndex,EFrontlineMissionTerrainV32& Style,bool bSet=false);
    // Highest actual bank triangle at XY. The caller combines this with terrain.
    WARFAREFRONT_API bool TryEarthworkSurfaceV36(AWarfareFrontlineGameMode* Battle,const FVector& WorldPoint,float& OutHeight);
    WARFAREFRONT_API void RefreshBankContactsV42(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API TSharedRef<FJsonObject> RunBankContactChecksV42(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool HasAirObservation(const AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& Point);
    WARFAREFRONT_API void SetAirObservation(AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& Point,float Until);
    WARFAREFRONT_API TSharedRef<FJsonObject> Review(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API void RunFocusedChecksV33(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API void RunMineChecksV8214(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API void RunMineRulesChecksV8215(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunWorkTimeBudgetChecksV8219(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
