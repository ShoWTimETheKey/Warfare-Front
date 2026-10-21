#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
namespace FrontlineEquipmentV41
{
 void Initialize(AWarfareFrontlineSoldier* Unit);
 void Tick(AWarfareFrontlineSoldier* Unit);
 void RunChecks(AWarfareFrontlineSoldier* Unit,TFunctionRef<void(const FString&,bool)> Check);
}
