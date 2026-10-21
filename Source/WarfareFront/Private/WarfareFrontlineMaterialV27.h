#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class UWorld;
namespace FrontlineMaterialV27
{
    void Apply(AWarfareFrontlineSoldier* Unit,bool bEnabled=true);
    void Fatal(AWarfareFrontlineSoldier* Unit,const FVector& IncomingDirection);
    void BurnV84411(AWarfareFrontlineSoldier* Unit,float Age,float Seconds);
    void RunBurnChecksV84411(AWarfareFrontlineSoldier* Fixture,TFunctionRef<void(const TCHAR*,bool)> Check);
    void ApplyWorld(UWorld* World,bool bEnabled);
}
