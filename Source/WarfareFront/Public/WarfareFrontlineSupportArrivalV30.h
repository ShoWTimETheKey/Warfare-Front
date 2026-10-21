#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineSupportArrivalV30.generated.h"
class AWarfareFrontlineGameMode;
class UStaticMeshComponent;
struct FFrontlineStrike;
/** Scheduled ammunition carries smoke/gas to its release point. No damage. */
UCLASS()
class WARFAREFRONT_API AFrontlineSupportArrivalV30 : public AActor
{
    GENERATED_BODY()
public:
    AFrontlineSupportArrivalV30();
    bool Configure(AWarfareFrontlineGameMode* Battle,const FVector& Target,int32 Team,float ImpactAt,bool bImpactDust);
    virtual void Tick(float DeltaTime) override;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Shell;
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    FVector Origin=FVector::ZeroVector,Target=FVector::ZeroVector;
    FGuid Encounter;
    float LaunchAt=0,ImpactAt=0;
    int32 TeamV839=INDEX_NONE;
    bool bImpactDust=false;
    bool bVisualLaunched=false;
};
void EnsureFrontlineSupportProjectileV30(AWarfareFrontlineGameMode* Battle,FFrontlineStrike& Strike);
void ScheduleFrontlineSupportArrivalV30(AWarfareFrontlineGameMode* Battle,const FVector& Target,int32 Team,float ImpactAt,bool bImpactDust);
