#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineDeath.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class UAnimationAsset;
class FJsonObject;
namespace FrontlineMotionV41
{
    void Reset(AWarfareFrontlineGameMode* Battle);
    UAnimationAsset* FatalMotion(AWarfareFrontlineSoldier* Unit,EFrontlineDamageKind Kind,const FVector& Incoming,bool Prone,int32& Variant);
    // Called after contact and weapon decisions; never delays an order or shot.
    UAnimationAsset* WaitingMotion(AWarfareFrontlineSoldier* Unit,float DT,bool ClearContact);
    void CancelWaiting(AWarfareFrontlineSoldier* Unit);
    UAnimationAsset* CarryIdle(const AWarfareFrontlineSoldier* Unit);
    void TickOutro(AWarfareFrontlineSoldier* Unit,float DT);
    UAnimationAsset* SpecialistAction(const AWarfareFrontlineSoldier* Unit,const TCHAR* Name,bool Trench);
    void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunOutcomeChecksV45(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    // Modes: 0 bullet/fire casualties, 1 independent trench waiting, 2 outcome.
    void PrepareActionReview(AWarfareFrontlineGameMode* Battle,int32 Mode);
    void TickActionReview(AWarfareFrontlineGameMode* Battle,float DT);
    TSharedRef<FJsonObject> ActionReviewState(AWarfareFrontlineGameMode* Battle);
}
