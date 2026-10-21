#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AFrontlineTank;
class FJsonObject;
enum class EFrontlineVehicle:uint8;
namespace FrontlineArmourV63
{
constexpr int32 ResearchCountV8218=16;
struct FResearch { int32 Id,Prerequisite,Cost; FString Title,Description; int32 Branch=0,Tier=0; FString TitleEN,TitleZH; };
FString L(const TCHAR* English,const TCHAR* Chinese);
TArray<FResearch> Research(const AWarfareFrontlineGameMode* B);
bool Has(const AWarfareFrontlineGameMode* B,int32 Id,int32 Team=0);
FString ResearchBlock(const AWarfareFrontlineGameMode* B,int32 Id);
bool BuyResearch(AWarfareFrontlineGameMode* B,int32 Id);
uint32 GetSelectedMask(const AWarfareFrontlineGameMode* B,uint32 Eligible);
bool ForcedAllocationV842(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,int32 Team=0);
bool IsSelected(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Type);
bool ToggleSelection(AWarfareFrontlineGameMode* B,EFrontlineVehicle Type);
EFrontlineVehicle PlatformV8218(EFrontlineVehicle Type);
EFrontlineVehicle ConfigurationV8218(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Platform,int32 Team=0);
bool ToggleConfigurationV8218(AWarfareFrontlineGameMode* B,EFrontlineVehicle Platform);
const TCHAR* VehicleDescriptionV8218(EFrontlineVehicle Type);
float ReadinessScale(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,int32 Team=0);
float TrackScale(const AWarfareFrontlineGameMode* B,int32 Team=0);
float GunIntegrityScale(const AWarfareFrontlineGameMode* B,int32 Team=0);
float CannonCycleScale(const AWarfareFrontlineGameMode* B,int32 Team=0);
float HullScaleV8218(const AWarfareFrontlineGameMode* B,int32 Team=0);
float TravelSpeedScaleV8218(const AWarfareFrontlineGameMode* B,int32 Team=0);
float MachinegunCycleScaleV8218(const AWarfareFrontlineGameMode* B,int32 Team=0);
float ArmourShellDamageScaleV8218(const AWarfareFrontlineGameMode* B,int32 Team=0);
float InfantryShellRadiusScaleV8218(const AWarfareFrontlineGameMode* B,int32 Team=0);
float CloseGunAccuracyScaleV8218(const AWarfareFrontlineGameMode* B,float DistanceCm,int32 Team=0);
float RepairRestoredFractionV8218(const AWarfareFrontlineGameMode* B,int32 Team=0);
bool ReviewRequestedV8218(const AWarfareFrontlineGameMode* B);
void RunChecksV8218(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
FString RequisitionTitle(int32 Id);
FString RequisitionDescription(int32 Id);
int32 Requisition(const AWarfareFrontlineGameMode* B);
bool CanRequisition(const AWarfareFrontlineGameMode* B,int32 Id);
bool BuyRequisition(AWarfareFrontlineGameMode* B,int32 Id);
void Tick(AWarfareFrontlineGameMode* B);
void SpawnPioneerEscort(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank);
// Detaches the existing living fitters once. Never creates replacement men.
// Call after the destroyed hull has changed to its wreck and disabled collision.
int32 ReleasePioneerEscortV8445(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank);
// A displaced survivor walks out through ordinary infantry Tick; release never
// teleports ground fitters. This temporary exit precedes normal squad goals.
bool PioneerEgressDestinationV8445(const AWarfareFrontlineSoldier* Unit,FVector& Destination);
TSharedRef<FJsonObject> RunPioneerEscortChecksV8445(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
AFrontlineTank* EscortFor(const AWarfareFrontlineSoldier* Unit);
bool EscortDestination(const AWarfareFrontlineSoldier* Unit,FVector& Destination);
bool SquadIsEscorting(const AWarfareFrontlineGameMode* B,int32 SquadId);
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
void RunAllocationChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
void RunRailAllocationChecksV842(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
}
