#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
enum class EFrontlineRole : uint8;

namespace FrontlineGameplayV51
{
    constexpr float ObservationSettleSeconds=2.f;
    constexpr float SealedGasDoseScale=.22f;
    bool MaskEligibleRole(EFrontlineRole Role);
    bool PaidPrerequisiteSatisfied(int32 NodeId,int32 Required,uint32 Mask);
    void Tick(AWarfareFrontlineGameMode* Battle,float DT);
    bool HasObservation(const AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& Point);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunObservationTimeBudgetChecksV8219(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
