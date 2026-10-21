#pragma once
#include "CoreMinimal.h"
#include "FRRU/WarfareFRRUIntegration.h"
#include "WarfareFrontlineMedalsV8215.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AFrontlineTank;
class FJsonObject;
struct FFrontlineKillSourceV65;
namespace FrontlineAchievementsV76
{
inline constexpr int32 Count=30;
inline constexpr int32 TributeStartV836=28;
inline constexpr int32 RequiredReturnClassesV844=3;
inline constexpr int32 CrossFactionStartV83=24;
inline constexpr int32 CrossFactionCountV83=4;
inline constexpr int32 HomewardIndexV82=11;
inline constexpr uint32 AllHonoursMaskV82=(1u<<Count)-1u;
inline constexpr uint8 AllCampaignRoutesV82=15; // Britain, Germany, France, Russia.
struct FDefinition{const TCHAR* Id;const TCHAR* NameEN;const TCHAR* NameZH;const TCHAR* RuleEN;const TCHAR* RuleZH;const TCHAR* HistoryEN;const TCHAR* HistoryZH;EFRRUNationV81 Nation=EFRRUNationV81::Invalid;};
const FDefinition& Definition(int32 Index);
/** The two Great War tributes share one presentation category. */
bool SpecialHonourV844(int32 Index);
/** Common=0, France=1, Russia=2. The original twelve indices remain stable. */
int32 ChapterV82(int32 Index);
FString BadgeObjectPathV82(int32 Index);
bool Earned(int32 Index);
int32 EarnedCount();
/** Persisted route bits: Britain=1, Germany=2, France=4, Russia=8. */
uint8 CompletedCampaignRouteMaskV82();
/** Saved battlefield feats: Britain=1, Germany=2, France=4, Russia=8. */
uint8 CompletedFeatNationMaskV83(int32 Index);
uint8 RequiredFeatNationMaskV83(int32 Index);
FrontlineMedalsV8215::FStats MedalStatsV8215(EFrontlineFaction Faction);
/** National identity is independent of the two battlefield team indices. */
FrontlineMedalsV8215::FStats NationalMedalStatsV87(EFRRUNationV81 Nation);
#if WITH_EDITOR
/** Labelled disposable eligibility/award fixture. Its token restores the in-memory account. */
struct FNationalParityFixtureV87;
TSharedPtr<FNationalParityFixtureV87> BeginNationalParityFixtureV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
#endif
FrontlineMedalsV8215::FAward MedalAwardV8216(const FString& StableId);
bool CheatGrantedAchievementV8218(int32 Index);
bool CheatGrantedMedalV8218(const FString& StableId);
/** Explicit console cheat only; never manufactures campaign completion. */
bool GrantAllForCheatV8218(AWarfareFrontlineGameMode* Battle,bool Achievements,bool Medals,FString& Response);
/** Explicit field-console reset; keeps existing campaign cheat taints. */
bool ResetAllHonoursV83(AWarfareFrontlineGameMode* Battle,FString& Response);
/** Commit archive deletion and global cheat revocation as one guarded operation. */
bool CommitArchiveDeletionV83(AWarfareFrontlineGameMode* Battle,TFunctionRef<bool()> CommitDeletion,FString& Response);
/** Strict disposable account checks; never resets a real player profile. */
void RunHonoursLifecycleChecksV83(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
void RunCheatGrantChecksV8218(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Persisted campaign-only assistance; does not disable an entire account. */
bool CampaignAssistedV8222(const FGuid& Campaign,bool SavedEligible=true);
/** Actual archive switching, delete/reload and natural award checks; disposable UserDir only. */
void RunCheatCampaignIsolationChecksV8222(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Assisted archive screenshot fixture; exact NoSave UserDir only. */
bool SeedAssistedArchiveReviewV8222(AWarfareFrontlineGameMode* Battle);
void RunMedalChecksV8215(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
bool SeedMedalReviewV8215(AWarfareFrontlineGameMode* Battle,bool Completed,int32 Level=0);
/** V8443 only: disposable empty briefing UI account. German stages 0/1/2
 * contain six/seven/seven advanced decorations; stage 3 is a transient
 * all-category national artwork fixture and never saves its display overlay. */
bool SeedRibbonReviewV8443(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation,int32 Stage);
/** Strict V837 disposable UI fixture only: nine national decorations and five
 * synthetic deed receipts, in memory. No save, campaign proof or award events. */
bool SeedHonoursPresentationV837(AWarfareFrontlineGameMode* Battle,EFrontlineFaction Faction);
/** Disposable report/operations fixture; unlocks release only after returning. */
bool SeedNewUnlockReviewV8215(AWarfareFrontlineGameMode* Battle,EFrontlineFaction Faction,int32 WonMission=0,bool ReturnToOperations=true);
/** Consume only a newly persisted award; independent of paused battle time. */
int32 TakeNotificationV81();
/** A newly saved real decoration, class or attachment; no virtual levels. */
struct FMedalNotificationV8217{int32 Index=INDEX_NONE;uint8 Grade=0,Repeats=0;uint16 Attachments=0;FGuid Receipt;};
FMedalNotificationV8217 TakeMedalNotificationV8217();
/** Atomic-save and deferred-notice fixtures, disposable absolute profile only. */
void RunMedalNotificationChecksV8217(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** DSC/DFM durable receipts and cheat rejection; exact disposable profile only. */
void RunAwardReceiptChecksV84414(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Actual durable disposable settlement, for timed earned-decoration captures. */
bool SeedMedalNotificationReviewV8217(AWarfareFrontlineGameMode* Battle,EFrontlineFaction Nation=EFrontlineFaction::British,bool Advance=false);
FString Status(int32 Index,bool Chinese);
void ResetEncounter(AWarfareFrontlineGameMode* Battle);
/** In-memory report probe only; restores private encounter event state. */
void WithScopedCombatEventsV839(AWarfareFrontlineGameMode* Battle,TFunctionRef<void()> Work);
/** Strict V84412 NoSave fixture only; restores account, encounters and transient award ledgers. */
void WithScopedCombatLedgerV84412(AWarfareFrontlineGameMode* Battle,TFunctionRef<void()> Work);
void BeginEncounter(AWarfareFrontlineGameMode* Battle);
void Tick(AWarfareFrontlineGameMode* Battle,float Seconds);
void Recruited(AWarfareFrontlineGameMode* Battle,int32 Role,int32 Team);
void Loss(AWarfareFrontlineGameMode* Battle,int32 VictimTeam,const FFrontlineKillSourceV65& Source,bool Armour);
void OfficerOrder(AWarfareFrontlineGameMode* Battle,AWarfareFrontlineSoldier* Officer,int32 Squad,float Relief=0.f);
void ObstacleCleared(AWarfareFrontlineGameMode* Battle,int32 Team,const FVector& Position,int32 ClearingSquad=INDEX_NONE);
void EnemyTrenchCaptured(AWarfareFrontlineGameMode* Battle,int32 PreviousOwner,int32 Team);
/** A native stage receipt, emitted only after a successful atomic acknowledgement. */
void NativeObjectiveStageV82(AWarfareFrontlineGameMode* Battle,const FString& StageId,bool Fallback);
/** A newly committed clearance of an authored node, after real engineer work. */
void NativePassageClearedV82(AWarfareFrontlineGameMode* Battle,int32 Team,const FString& NodeId,const FVector& Position);
void RailSectionDeployed(AWarfareFrontlineGameMode* Battle,AFrontlineTank* Train,int32 Role,int32 SquadId);
void MarkCheat(AWarfareFrontlineGameMode* Battle);
/** Actual legacy profile save result for this bound ended encounter only. */
void LegacyOutcomePersistedV83(AWarfareFrontlineGameMode* Battle,int32 Winner,bool Saved);
void Finish(AWarfareFrontlineGameMode* Battle,int32 Winner);
/** Append real outro kill deltas once, without repeating any award evaluation. */
bool FinalizeBattleKillsV8217(AWarfareFrontlineGameMode* Battle);
/** Real restart and retry boundary, restricted to a disposable account. */
void RunRestartTailChecksV8219(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Initial progression failure, frozen proof release and tail retry; exact V8219 disposable profile only. */
void RunInitialOutcomeRetryChecksV8219(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Scoped event/settlement fixtures; strictly disposable review account only. */
void RunIsolatedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Scoped final-rifle criterion boundaries only: no account loads/writes or settlement. */
void RunAllQuietChecksV8310(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Scoped actual actor/event and pure award boundaries; strict disposable UK/DE only. */
void RunBalanceChecksV842(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
void RunChallengeChecksV84412(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Future settlements only: one deployed MG section, or a real heavy-tank victim.
 * Pure predicate; it neither finishes a battle nor grants an account award. */
bool ChallengeConditionV84415(const AWarfareFrontlineGameMode* Battle,int32 Index);
/** Accepted idempotent fatal-hull route; never inferred from damage or kill totals. */
void ArmourDestroyedV84415(AWarfareFrontlineGameMode* Battle,const AFrontlineTank* Victim,const FFrontlineKillSourceV65& Source);
/** Focused criterion/event guard checks; disposable UK/DE review only. */
void RunChallengeChecksV84415(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Four bounded production exchanges with enemy retaliation. Setup/history are
 * controlled; no hit, damage, kill or victory evidence is fabricated. */
TSharedRef<FJsonObject> RunChallengeFeasibilityV84415(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Pure national event/settlement fixtures; no live progression is manufactured. */
void RunNationalIsolatedChecksV82(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Four disposable UK/DE live gameplay cases. Prepare before normal world
 * preparation; Start after BeginBattle; Tick while ordinary engine ticks run.
 * History/research/loadout prerequisites are explicit, never goal evidence. */
bool PrepareFeasibilityCaseV838(AWarfareFrontlineGameMode* Battle,int32 CaseIndex);
bool StartFeasibilityCaseV838(AWarfareFrontlineGameMode* Battle,int32 CaseIndex);
/** Returns true at victory, defeat or the 600-simulation-second bound.
 * CompletedReport is populated only then; a timeout is never an award pass. */
bool TickFeasibilityCaseV838(AWarfareFrontlineGameMode* Battle,TSharedPtr<FJsonObject>& CompletedReport);
}
