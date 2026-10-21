#pragma once
#include "CoreMinimal.h"
#include "FRRU/WarfareFRRUIntegration.h"

class AWarfareFrontlineGameMode;
enum class EFrontlineFaction : uint8;
struct FFrontlineKillSourceV65;

/** Original unit-war-diary tally, distinct from historical decorations. */
namespace FrontlineArmourKillTallyV8220
{
struct FNotification
{
    EFRRUNationV81 Nation=EFRRUNationV81::Invalid;
    int32 Total=0;
    FGuid Encounter,Receipt;
};
int32 Count(EFRRUNationV81 Nation);
int32 Count(EFrontlineFaction Faction);
FString Description(EFRRUNationV81 Nation,bool Chinese);
/** Returns only a new record committed to both account files. */
FNotification TakeNotification();
/** Called only by the unique, real hull-destruction route. */
void RecordDestroyedArmour(AWarfareFrontlineGameMode* Battle,int32 VictimTeam,int32 VictimSerial,const FFrontlineKillSourceV65& Source);
/** Codec, real fatal-hit and save-retry checks in an allowlisted disposable account. */
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Generates one genuine fatal-hit record for a timed capture, disposable account only. */
bool SeedNotificationReview(AWarfareFrontlineGameMode* Battle);
}
