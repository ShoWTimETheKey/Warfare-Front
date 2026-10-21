#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class UAnimationAsset;
class FJsonObject;
namespace FrontlineMotionV40
{
    struct FGasExposureV8215
    {
        float Dose=0,Threshold=0,Concentration=0,Rate=0,Distress=0;
        int32 SourceTeam=INDEX_NONE;
        bool Sealed=false;
        bool Escaping=false;
    };
    // Read-only presentation/review state; emitters never apply another damage roll.
    FGasExposureV8215 GasExposureV8215(const AWarfareFrontlineSoldier* Unit);
    void Reset(AWarfareFrontlineGameMode* Battle);
    // Exposure runs once per soldier game tick, independently of support-cloud emitters.
    void TickExposure(AWarfareFrontlineSoldier* Unit,float DT);
    // One physically swept, full-body escape attempt at a dangerous gas dose.
    // Returning true owns this tick's movement; exposure remains counted once.
    bool TickGasEscapeV8216(AWarfareFrontlineSoldier* Unit,float DT);
    // Exact disposable-profile fixture for the real gas locomotion branch.
    bool PrepareGasEscapeTimeBudgetReviewV8219(AWarfareFrontlineSoldier* Unit);
    bool TickAction(AWarfareFrontlineSoldier* Unit,float DT);
    bool IsAction(const UAnimationAsset* Asset);
    bool IsBusy(const AWarfareFrontlineSoldier* Unit);
    bool MaskSealed(const AWarfareFrontlineSoldier* Unit);
    bool HasIssuedMask(const AWarfareFrontlineSoldier* Unit);
    bool MaskPresentation(const AWarfareFrontlineSoldier* Unit,FVector& LocalOffset);
    bool RequestWireCut(AWarfareFrontlineSoldier* Unit,const FVector& Wire);
    bool ConsumeWireCut(AWarfareFrontlineSoldier* Unit,const FVector& Wire);
    UAnimationAsset* GasDeathMotion(AWarfareFrontlineSoldier* Unit,bool Prone);
    void Abort(AWarfareFrontlineSoldier* Unit);
    // Read-only evidence for the existing disposable mine-clearance fixture.
    TSharedRef<FJsonObject> MineWorkReviewStateV835(const AWarfareFrontlineSoldier* Unit);
    // Read-only counters from ordinary gas-escape ticks in a disposable review.
    TSharedRef<FJsonObject> GasLocomotionReviewStateV836(const AWarfareFrontlineSoldier* Unit);
    void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunFocusedChecksV8215(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunGasMaskChecksV851(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void PrepareFieldActionReview(AWarfareFrontlineGameMode* Battle);
    void TickFieldActionReview(AWarfareFrontlineGameMode* Battle,float DT);
    TSharedRef<FJsonObject> FieldActionReviewState(AWarfareFrontlineGameMode* Battle);
}
