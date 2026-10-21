#pragma once
#include "CoreMinimal.h"
class UWorld;
namespace FrontlineMedalMotionV8216
{
struct FShock { uint64 Serial=0; FVector Position=FVector::ZeroVector; float Strength=0; };
void NotifyBlast(UWorld* World,const FVector& Position,float Strength);
void ReadSince(UWorld* World,uint64& Serial,TArray<FShock,TInlineAllocator<16>>& Out);
struct FPendulum { float Angle=0,Velocity=0; void Step(float Delta,float Acceleration,float Impulse); };
void RunChecks(TFunctionRef<void(const FString&,bool)> Check);
}
