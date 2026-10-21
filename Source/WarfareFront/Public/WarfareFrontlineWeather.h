#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineWeather.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;
class AWarfareFrontlinePrecipitation;
class AWarfareFrontlineController;
enum class EFrontlineWeatherMode:uint8 { Campaign,Overcast,Drizzle,Rain };
void StartFrontlineV19WeatherReview(AWarfareFrontlineController* Controller);

struct FFrontlineWeatherLookV50
{
    const TCHAR* Name=TEXT("Broken overcast");
    float CloudBase=.58f,SunScale=1.f,SkyScale=1.f,Pitch=-28.f,Yaw=42.f,SourceAngle=4.f;
    float Mist=0.f,FogStart=14200.f,FogFalloff=.30f,BackdropScale=1.f;
    // V84.2 authors the key and fill for each available UK/DE operation.
    // Rain, visibility, wetness and the operation's fog remain authoritative.
    bool bAuthoredLightingV84=false,bCloudShadowV84=true;
    float CloudShadowScaleV84=1.f;
    const TCHAR* LightProfileKeyV842=TEXT("legacy");
    const TCHAR* LightPhaseV842=TEXT("Day");
    int32 LightSeasonMonthV842=0;
    FLinearColor SunTint=FLinearColor(.95f,.97f,1.f),SkyTint=FLinearColor(.93f,.96f,1.f),FogTint=FLinearColor(.060f,.069f,.077f);
};
FFrontlineWeatherLookV50 FrontlineWeatherLookV50(int32 Operation,float Wetness,float Visibility);
void RunFrontlineWeatherChecksV50(TFunctionRef<void(const FString&,bool)> Check);
void RunFrontlineWeatherChecksV842(TFunctionRef<void(const FString&,bool)> Check);

/** A bounded weather director; terrain, skies, lights and precipitation share one state. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineWeather:public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineWeather();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    static AWarfareFrontlineWeather* Get(UWorld* World);
    EFrontlineWeatherMode GetMode() const { return Mode; }
    void SetMode(EFrontlineWeatherMode Value,bool bSave=true);
    float GetRainAmount() const { return Rain; }
    float GetWetness() const { return Wetness; }
    float GetCloudCover() const { return Cloud; }
    float GetSunIntensity() const;
    float GetSkyIntensity() const;
    bool IsReady() const;
    AWarfareFrontlinePrecipitation* GetPrecipitation() const { return Precipitation; }
    void SnapToWeatherForReview();
    void RebindGroundMaterial(); // Reacquire the displayed MID after material A/B.
    bool UsesDisplayedGroundMaterial() const;
    FFrontlineWeatherLookV50 GetOperationLookV50() const { return OperationLook; }
private:
    UPROPERTY(Transient) TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY(Transient) TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY(Transient) TObjectPtr<UExponentialHeightFogComponent> Fog;
    UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Grade;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GroundMID;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BackdropMID;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CloudMID;
    UPROPERTY(Transient) TObjectPtr<AWarfareFrontlinePrecipitation> Precipitation;
    EFrontlineWeatherMode Mode=EFrontlineWeatherMode::Campaign;
    float Rain=.28f,Wetness=.68f,Cloud=.70f,TargetRain=.28f,TargetCloud=.70f;
    float Clock=0,UpdateClock=0;
    int32 LastMissionSeed=INDEX_NONE;
    bool bBound=false;
    FFrontlineWeatherLookV50 OperationLook;
    void BindScene();
    void EvaluateTarget();
    void ApplyWeather();
};
