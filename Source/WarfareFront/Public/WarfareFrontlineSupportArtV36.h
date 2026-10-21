#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarfareFrontlineSupportArtV36.generated.h"
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;
class AWarfareFrontlineGameMode;
struct FFrontlineStrike;
/** Cosmetic ammunition uses the committed game's flight clock. No damage or targeting. */
UCLASS()
class WARFAREFRONT_API AFrontlineOrdnanceV36 : public AActor
{
    GENERATED_BODY()
public:
    AFrontlineOrdnanceV36();
    bool Configure(AWarfareFrontlineGameMode* B,const FVector& From,const FVector& To,float Launched,float Impact,bool Direct,bool AirBullet=false,bool Terminal=true,float TerminalSpeed=14500.f);
    virtual void Tick(float DeltaSeconds) override;
    void UpdateVisual(float Time);
    static FVector PositionAt(const FVector& From,const FVector& To,float Duration,float T,bool Direct);
    static FVector TerminalPositionV43(const FVector& From,const FVector& To,float SecondsToImpact,float Speed);
    bool IsVisualReady() const;
    void SetGroundImpactEnabled(bool Enabled){bGroundImpact=Enabled;}
    void SetHeavyBulletVisualV8216(){bHeavyBulletV8216=true;}
    void SetCommittedSupportV8217(bool Enabled=true){bCommittedSupportV8217=Enabled;}
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Wake;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WakeMaterial;
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    FGuid Encounter;
    FVector Origin=FVector::ZeroVector,Target=FVector::ZeroVector;
    float LaunchAt=0,ImpactAt=0,TerminalSpeedV43=14500.f;
    bool bDirect=false,bBullet=false,bTerminalV43=false,bGroundImpact=true;
    bool bHeavyBulletV8216=false;
    bool bCommittedSupportV8217=false;
};
namespace FrontlineSupportArtV36
{
    UStaticMesh* AircraftFor(const AWarfareFrontlineGameMode* Battle,int32 Side);
    void PrewarmVisuals();
    UStaticMesh* Aircraft(bool German);
    bool PrepareAircraftReview(bool Report=false);
    AFrontlineOrdnanceV36* SpawnShell(AWarfareFrontlineGameMode* Battle,const FFrontlineStrike& Strike);
    void StrafeRound(AWarfareFrontlineGameMode* Battle,const FVector& Muzzle,const FVector& Ground,float Duration=.085f,bool GroundImpact=true,bool CommittedSupport=false);
    void StrafeRoundAtV8220(AWarfareFrontlineGameMode* Battle,const FVector& Muzzle,const FVector& Ground,float LaunchAt,float ImpactAt);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
    void SetupVisualFixture(AWarfareFrontlineGameMode* Battle);
}
namespace FrontlineSupportArtV43
{
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    bool SetupTerminalFixture(AWarfareFrontlineGameMode* Battle,float SecondsBeforeImpact);
}
namespace FrontlineSupportArtV40
{
    struct FFlight {FVector Position,Velocity;FRotator Rotation;};
    FFlight AttackFlight(const FVector& Target,int32 Direction,float PassAge,int32 PassIndex);
    FFlight TransitFlight(const FVector& Target,int32 Direction,float Age);
    bool ForwardGroundShot(const FVector& Muzzle,const FVector& Forward,FVector& OutPoint,float& OutDuration);
    void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const TCHAR*,bool)> Check);
    bool SetupFlightFixture(AWarfareFrontlineGameMode* Battle,float PassAge);
}
namespace FrontlineSupportArtV8220
{
    // Flight speed is independent of the fire mission's release interval.
    inline constexpr float BomberSpeed=2300.f,BomberHeight=1550.f;
    FrontlineSupportArtV40::FFlight BomberFlight(const FVector& Target,int32 Direction,float Age);
}
