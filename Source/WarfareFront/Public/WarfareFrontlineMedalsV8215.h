#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineMedalRepeatLimitsV85.h"
#include "WarfareFrontlineProgression.h"
#include "FRRU/WarfareFRRUIntegration.h"
namespace FrontlineMedalsV8215
{
inline constexpr int32 Count=53;
struct FStats{int32 Infantry=0,Armour=0,Victories=0,CleanVictories=0;uint8 NaturalRoute=0;};
enum class EAttachment:uint16{None=0,Bar=1,OakLeaves=2,Palm=4,Star=8,Clasp=16,Rosette=32,Swords=64,Crown=128,Bow=256};
/** Grade is the catalogue's real class, never a virtual service level. */
struct FAward{uint8 Grade=0,Repeats=0;uint16 Attachments=0;FString Date;FGuid Campaign;TArray<FGuid> CampaignDeeds;};
/** Museum selection only: a real class with its documented repeat/citation devices. */
struct FInspectionStateV84413{FAward Form;bool Earned=false;};
struct FVariant{const TCHAR* Id;const TCHAR* NameEN;const TCHAR* NameZH;const TCHAR* ArtworkStem;const TCHAR* HistoricalQualification;};
struct FProof{int32 Nation=0,Date=0,Difficulty=0,Infantry=0,Armour=0,Losses=0,ArmourLosses=0,Obstacles=0,FriendlyFire=0,EnteredInfantryV842=0,ReturnedInfantryV842=0,AircraftInfantryV84412=0,AircraftArmourV84412=0,SupportArmourV84412=0;float MinimumMorale=1.f;FGuid Campaign;bool Won=false,Replay=false,Completed=false,Undefeated=false;};
struct FDefinition
{
 const TCHAR* Id;const TCHAR* NameEN;const TCHAR* NameZH;const TCHAR* HistoryEN;const TCHAR* HistoryZH;
 const TCHAR* RuleEN;const TCHAR* RuleZH;EFRRUNationV81 Nation;int32 Metric;int32 Threshold[3];bool Ribbon;
};
const FDefinition& Definition(int32 Index);
FStats Stats(EFrontlineFaction Faction);
FStats Stats(EFRRUNationV81 Nation);
int32 Value(int32 Index);
int32 Tier(int32 Index);
int32 NextTarget(int32 Index);
FString Progress(int32 Index,bool Chinese);
FString FlagTooltip(EFrontlineFaction Faction,bool Chinese);
FString FlagTooltip(EFRRUNationV81 Nation,bool Chinese);
TArray<int32> Ribbons(EFrontlineFaction Faction);
TArray<int32> Ribbons(EFRRUNationV81 Nation);
/** Previously earned breast ribbons remain in the service record after promotion. */
int32 RibbonGradeForHighestClassV8443(int32 Index,int32 HighestGrade);
FString ArtworkPath();
FString ArtworkPath(int32 Index);
FString VariantArtworkPath(int32 Index,int32 Grade);
/** Iron Cross I/Grand Cross retains the already-earned II ribbon. */
FString RibbonArtworkPath(int32 Index);
TArray<int32> NationIndices(EFRRUNationV81 Nation);
TArray<FVariant> Variants(int32 Index);
FString AwardLabel(int32 Index,bool Chinese);
/** Service-record presentation; storage remains in MedalAwardV8216 unchanged. */
FAward Award(int32 Index);
/** Display copy only: legacy raw repeats remain unchanged in the account. */
FAward PresentedAwardV85(int32 Index,const FAward& Recorded);
TArray<FInspectionStateV84413> InspectionStatesV84413(int32 Index,const FAward& Recorded);
inline TArray<FInspectionStateV84413> InspectionStatesV84413(int32 Index){return InspectionStatesV84413(Index,Award(Index));}
FString InspectionLabelV84413(int32 Index,const FAward& Form,bool Chinese);
void RunInspectionChecksV84413(TFunctionRef<void(const FString&,bool)> Check);
int32 IndexForId(const FString& Id);
bool ValidAward(int32 Index,const FAward& Award);
/** Pure receipt-bound evaluator; caller owns atomic account persistence. */
void ApplyAwards(TMap<FString,FAward>& Awards,const FStats& Cumulative,const FProof& Proof);
/** Actual entered roster only; pending reserves never improve the proportion. */
bool PreservedForceV842(int32 Entered,int32 Returned);
/** Actual catalogue classes/attachments, for explicitly marked cheat grants. */
FAward CompleteCheatAwardV8218(int32 Index,const FGuid& Campaign,const FString& Date);
void RunCatalogueChecks(TFunctionRef<void(const FString&,bool)> Check);
void RunExtensionChecksV84412(TFunctionRef<void(const FString&,bool)> Check);
/** Pure production rule checks; temporary maps only, no account or player files. */
void RunAwardChecksV84414(TFunctionRef<void(const FString&,bool)> Check);
bool Available(EFRRUNationV81 Nation);
}
