#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineSoldier;
class AWarfareFrontlineGameMode;
class AFrontlineTank;
class FJsonObject;
struct FFrontlineKillSourceV65;
enum class EFrontlineRole:uint8;
namespace FrontlineSpecialistsV41
{
#if WITH_EDITOR
    TSharedRef<FJsonObject> RunFrenchVBMechanicsV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(AWarfareFrontlineSoldier*)> Observe);
#endif
    /** Encounter-local readback, populated only by the strict disposable V833 probe. */
    struct FArmourRoundReadbackV833
    {
        int32 Fired=0,PhysicalHullHits=0,AppliedHits=0,WorldInterceptions=0,GrenadeAppliedHits=0;
        float HullDamage=0,TrackDamage=0,GrenadeHullDamage=0,GrenadeTrackDamage=0;
    };
    FArmourRoundReadbackV833 ReadArmourRoundsV833(AWarfareFrontlineGameMode* Battle);
    void NoteGrenadeArmourV833(AWarfareFrontlineGameMode* Battle,const FFrontlineKillSourceV65& Source,int32 VictimTeam,float HullDamage,float TrackDamage);
    bool IsRifleGrenadier(const AWarfareFrontlineSoldier* Unit);
    bool HasAntiTankRifleGrenadeV836(const AWarfareFrontlineSoldier* Unit);
    // A committed HE rifle bomb stays a shorter-range weapon than Tankgewehr,
    // but should not close below the ordinary rifle-bomb envelope to engage plate.
    constexpr float AntiTankRifleGrenadeRangeCmV836=5000.f;
    constexpr float AntiTankRifleGrenadeContactCmV8310=4800.f;
    bool IsAntiTankGunner(const AWarfareFrontlineSoldier* Unit);
    bool IsSpecialist(const AWarfareFrontlineSoldier* Unit);
    float ReloadSeconds(const AWarfareFrontlineSoldier* Unit);
    float RifleGrenadeScatterRadius(float RangeCm,float Suppression,bool Trained);
    FQuat RifleGrenadeRotation(const FVector& Velocity,bool FrenchVB=false);
    bool Fire(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Target,AFrontlineTank* Tank,FRandomStream& Random);
    void Tick(AWarfareFrontlineGameMode* Battle);
    void Reset(AWarfareFrontlineGameMode* Battle);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
void RunChecksV75(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
AWarfareFrontlineSoldier* SetupVisualFixture(AWarfareFrontlineGameMode* Battle,EFrontlineRole Role);
}
