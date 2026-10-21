#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
enum class EFrontlineSupport:uint8;
namespace FrontlineSupportV32
{
    struct FReconStatusV37
    {
        FVector Centre=FVector::ZeroVector;
        float SecondsRemaining=0;
        int32 Infantry=0,Machineguns=0,Armour=0;
    };
    WARFAREFRONT_API void Reset(AWarfareFrontlineGameMode* B);
    WARFAREFRONT_API void Tick(AWarfareFrontlineGameMode* B,float DT);
    WARFAREFRONT_API bool Request(AWarfareFrontlineGameMode* B,EFrontlineSupport S,const FVector& Target,int32 Team);
    WARFAREFRONT_API float Cooldown(const AWarfareFrontlineGameMode* B,EFrontlineSupport S,int32 Team);
    WARFAREFRONT_API bool GetReconStatusV37(const AWarfareFrontlineGameMode* B,int32 Team,FReconStatusV37& Status);
    WARFAREFRONT_API bool HasRegistrationV37(const AWarfareFrontlineGameMode* B,int32 Team,const FVector& Point);
    WARFAREFRONT_API void ResetReadinessForDebug(AWarfareFrontlineGameMode* B,int32 Team=0);
    WARFAREFRONT_API void RunV37Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check);
    WARFAREFRONT_API bool SetupReconReviewV37(AWarfareFrontlineGameMode* B);
}
