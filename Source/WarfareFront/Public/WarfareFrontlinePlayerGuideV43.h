#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
enum class EFrontlineSupport:uint8;
enum class EFrontlineVehicle:uint8;
namespace FrontlinePlayerGuideV43
{
// Design-space rectangles are mirrored about the actual 1920-wide console.
// Both labels use the same line height and measured alignment in the HUD.
struct FMoraleLabels { FBox2D Army,Value; };
FMoraleLabels MoraleLabels(bool Left);
TArray<EFrontlineRole> NewRoles(EFrontlineFaction Faction,const FFrontlineCampaignMission& Mission);
FString Unit(const AWarfareFrontlineGameMode* B,EFrontlineRole Role);
const TCHAR* Support(EFrontlineSupport Kind,EFrontlineFaction Faction);
const TCHAR* Armour(EFrontlineVehicle Type);
FString Technology(const AWarfareFrontlineGameMode* B,const FFrontlineTechnologyNode& Node);
int32 TechnologyArrival(EFrontlineFaction Faction,int32 Id);
EFrontlineRole TechnologyRole(EFrontlineFaction Faction,int32 Id);
int32 SupportArrival(EFrontlineSupport Kind);
void ApplyCampaign(TArray<FFrontlineCampaignMission>& Missions,bool German);
void RunChecks(const AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
}
