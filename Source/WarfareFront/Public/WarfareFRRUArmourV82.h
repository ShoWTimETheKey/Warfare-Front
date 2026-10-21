#pragma once
#include "CoreMinimal.h"
#include "FRRU/WarfareFRRUIntegration.h"
class AWarfareFrontlineGameMode;
class AFrontlineTank;
class AWarfareFrontlineSoldier;
class UFrontlineVehicleSubsystem;
class FJsonObject;
class USkeletalMesh;
struct FFrontlineKillSourceV65;
struct FFRRUWorkStateV82;
struct FFRRUArmourEffectiveSpecV87
{
    float CannonCycleScale=1,CannonAimRateScale=1,MachinegunRecoveryScale=1,MachinegunAimRateScale=1;
    float RepairSeconds=12,PreparationScale=1;
    bool Crew=false,Repair=false,Prepare=false;
};
struct FFRRUArmourDefinitionV82
{
    FString Id,Token,Name,Description;
    EFRRUNationV81 Nation=EFRRUNationV81::Invalid;
    int32 FirstDate=0,LastDate=0;
    float Cooldown=0,Speed=0,Hull=0,Range=0,ShellRadius=0,ShellEnergy=0;
    FVector Minimum,Maximum;
    bool Wheeled=false,Rail=false;
};
enum class EFRRUTrainPassengerStageV82 : uint8 { Seated,Queue,Sill,Steps,GroundClear,Field,Lost,Standing,Aisle };
struct FFRRUTrainPassengerStateV82
{
    int32 CarrierSerial=0,SquadId=0,MemberIndex=0,Car=0,Seat=0;
    EFRRUTrainPassengerStageV82 Stage=EFRRUTrainPassengerStageV82::Seated;
    FVector LocalFeet=FVector::ZeroVector;
    float StageStartedAt=0,Progress=0;
    FVector WorldFeet=FVector::ZeroVector,Facing=FVector::ForwardVector;
    // Live carrier transform of this carriage's outside landing, before the
    // ordinary native ground/contact projection. This is not a movement grant.
    FVector CarrierExitFeet=FVector::ZeroVector;
    FVector CarrierClearanceCorner=FVector::ZeroVector;
    bool Alive=false,Embarked=false;
};
namespace WarfareFRRUArmourV82
{
    WARFAREFRONT_API const TArray<FFRRUArmourDefinitionV82>& Definitions();
    WARFAREFRONT_API const FFRRUArmourDefinitionV82* Definition(const FString& Id);
    WARFAREFRONT_API const FFRRUArmourDefinitionV82* Definition(const AFrontlineTank* Vehicle);
    WARFAREFRONT_API FString Id(const AFrontlineTank* Vehicle);
    WARFAREFRONT_API bool National(const AFrontlineTank* Vehicle);
    // Only current encounter actors physically retained in this carrier count.
    // No copied model, artificial recruitment or visual-only passenger exists.
    WARFAREFRONT_API bool Embarked(const AWarfareFrontlineSoldier* Soldier);
    WARFAREFRONT_API int32 Passengers(const AFrontlineTank* Vehicle);
    WARFAREFRONT_API bool PassengerState(const AWarfareFrontlineSoldier* Soldier,FFRRUTrainPassengerStateV82& State);
    // This authority exists only for an actual living passenger in the same
    // encounter's destroyed carrier; ordinary inactive infantry remain immune.
    WARFAREFRONT_API bool DestroyedCarrierPassenger(const AWarfareFrontlineSoldier* Soldier);
    WARFAREFRONT_API void CrewLost(AFrontlineTank* Vehicle,const FFrontlineKillSourceV65& Source);
    WARFAREFRONT_API bool Eligible(const AWarfareFrontlineGameMode* Battle,const FString& Id,int32 Side);
    WARFAREFRONT_API int32 FirstMissionForArmour(const AWarfareFrontlineGameMode* Battle,const FString& Id);
    WARFAREFRONT_API bool AssetsReady(const FString& Id,FString& Error);
    WARFAREFRONT_API bool Accepted(const AWarfareFrontlineGameMode* Battle,const FString& Id);
    WARFAREFRONT_API FString BlockReason(const AWarfareFrontlineGameMode* Battle,const FString& Id,int32 Side=0);
    WARFAREFRONT_API TArray<FString> Roster(const AWarfareFrontlineGameMode* Battle,int32 Side=0);
    WARFAREFRONT_API void PreparationMeshes(const AWarfareFrontlineGameMode* Battle,TArray<USkeletalMesh*>& OutMeshes);
    WARFAREFRONT_API bool Recruit(AWarfareFrontlineGameMode* Battle,const FString& Id,int32 Side=0);
    // Consumes only this encounter's opening reserve, before any dispatch on
    // this side. Subsequent model-specific workshop cycles cannot be released.
    WARFAREFRONT_API bool ReleaseOpeningReadinessV87(AWarfareFrontlineGameMode* Battle,int32 Side=0);
    // A dispatch occupies the shared workshop, but each model has its own
    // completion time. Empty Id retains the last-dispatched cycle readback.
    WARFAREFRONT_API float CooldownFraction(const AWarfareFrontlineGameMode* Battle,int32 Side=0,const FString& Id=FString());
    WARFAREFRONT_API bool StageRailway(AWarfareFrontlineGameMode* Battle,FString& Error);
    WARFAREFRONT_API void ClearRailway(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool WalkSurface(const AWarfareFrontlineGameMode* Battle,float X,float Y,float NativeHeight,float& Height);
    WARFAREFRONT_API void Reset(UFrontlineVehicleSubsystem* Fleet);
    WARFAREFRONT_API void TickEnemy(UFrontlineVehicleSubsystem* Fleet);
    WARFAREFRONT_API void Simulate(AFrontlineTank* Vehicle,float DeltaSeconds);
    WARFAREFRONT_API bool Entered(const AFrontlineTank* Vehicle);
    WARFAREFRONT_API FVector Muzzle(const AFrontlineTank* Vehicle,int32 Index,bool Cannon);
    // Actual national chassis range, with the shared mounted-MG cap and weather.
    WARFAREFRONT_API float WeaponRangeV87(const AFrontlineTank* Vehicle,bool Cannon);
    WARFAREFRONT_API bool OccupiesNode(const AFrontlineTank* Vehicle,const FVector& Centre,float Radius);
    WARFAREFRONT_API bool Contains(const AFrontlineTank* Vehicle,const FVector& Point,float Margin);
    WARFAREFRONT_API FVector GrenadeAim(const AFrontlineTank* Vehicle,const FVector& Shooter);
    WARFAREFRONT_API float HalfLength(const AFrontlineTank* Vehicle);
    // Native option values are frozen for this actual vehicle at dispatch.
    WARFAREFRONT_API FFRRUArmourEffectiveSpecV87 EffectiveSpecV87(const AFrontlineTank* Vehicle);
    WARFAREFRONT_API bool RepairDestinationV87(const AWarfareFrontlineSoldier* Soldier,FVector& Destination);
    WARFAREFRONT_API bool RepairWorkStateV87(const AWarfareFrontlineSoldier* Soldier,FFRRUWorkStateV82& Out);
#if WITH_EDITOR
    // Read-only FT-only candidate evidence. This never grants deployment.
    WARFAREFRONT_API TSharedRef<FJsonObject> GroundPilotDiagnosis(const AFrontlineTank* Vehicle);
#endif
    // Run by the exact isolated world review. This does not grant deployment.
    WARFAREFRONT_API TSharedRef<FJsonObject> RunAssetChecks(AWarfareFrontlineGameMode* Battle);
}
