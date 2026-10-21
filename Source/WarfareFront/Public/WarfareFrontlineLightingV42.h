#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class UDirectionalLightComponent;
class USkyLightComponent;
class UWorld;
class FJsonObject;
struct FPostProcessSettings;

namespace FrontlineLightingV42
{
    bool IsBaseline();
    bool IsV84Baseline();
    bool IsV842Baseline();
    void ApplyGrade(FPostProcessSettings& Settings);
    void ApplyLights(UDirectionalLightComponent* Sun,USkyLightComponent* Sky,float Storm);
    float BackdropGain(float Storm);
    void ObserveReviewWorld(UWorld* World);
    bool PrepareReviewWeather(AWarfareFrontlineGameMode* Battle);
    TSharedRef<FJsonObject> Describe(AWarfareFrontlineGameMode* Battle);
    TSharedRef<FJsonObject> DescribeV84(AWarfareFrontlineGameMode* Battle);
    void RunV84Checks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    void RunV842Checks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    // One isolated post-sample readback; no renderer, material or CVar mutation.
    TSharedRef<FJsonObject> DescribeLiveReflectionBindingsV44(AWarfareFrontlineGameMode* Battle);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);

    // Final, isolated, static ground-truth capture only. This freezes the review
    // world and never enables the progressive path tracer during gameplay.
    bool BeginStaticReference(AWarfareFrontlineGameMode* Battle);
    TSharedRef<FJsonObject> DescribeReference(AWarfareFrontlineGameMode* Battle);
}
