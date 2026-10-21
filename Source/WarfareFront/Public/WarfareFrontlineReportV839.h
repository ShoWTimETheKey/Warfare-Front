#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineCombatRecordV65.h"
#include "FRRU/WarfareFRRUIntegration.h"
class APlayerController;
class AWarfareFrontlineController;
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AFrontlineTank;
class FJsonObject;

struct FFrontlineReportDataV839
{
    FGuid Encounter;
    FFrontlineCombatRecordV65 Armies[2];
    int32 Survivors[2]={},Morale[2]={};
    float Seconds=0;
    bool CampaignV86=false,bEarlierAttemptsUnrecordedV86=false;
    int32 BattlesV86=0,VictoriesV86=0,DefeatsV86=0;
    /** Built/sorted once per campaign content revision; valid role rows for paged drawing. */
    TArray<FFrontlineSquadReportV839> SortedSquadsV86[2];
};
namespace FrontlineReportV839
{
    /** Call after fresh attempt records are cleared; never changes campaign access. */
    void Begin(AWarfareFrontlineGameMode* Battle);
    void RegisterSquad(AWarfareFrontlineGameMode* Battle,int32 Team,int32 Id,int32 Role,int32 Strength=0);
    void InfantryDeath(AWarfareFrontlineGameMode* Battle,int32 VictimTeam,const AWarfareFrontlineSoldier* Victim,const FFrontlineKillSourceV65& Source);
    void ArmourDeath(AWarfareFrontlineGameMode* Battle,const AFrontlineTank* Victim,const FFrontlineKillSourceV65& Source);
    /** One successfully accepted support order, not an arrival-time estimate. */
    void SupportDelivered(AWarfareFrontlineGameMode* Battle,int32 Team,int32 Support,int32 Count=1);
    /** Actual committed discharges/releases, never the planned salvo size. */
    void SupportRounds(AWarfareFrontlineGameMode* Battle,int32 Team,int32 Support,int32 Count=1);
    /** Positive applied integrity loss, clamped by the authoritative receiver. */
    void ArmourDamage(AWarfareFrontlineGameMode* Battle,int32 VictimTeam,const FFrontlineKillSourceV65& Source,float HullApplied,float TrackApplied);
    FFrontlineReportDataV839 GetReportData(const AWarfareFrontlineGameMode* Battle);
    /** Borrowed cache view until this battle's next report read; copy only to retain a snapshot. */
    const FFrontlineReportDataV839& GetCampaignReportV86(const AWarfareFrontlineGameMode* Battle);
    /** Positive changes after a committed return; does not count a second battle. */
    FFrontlineCombatRecordV65 CombatTailV87(const FFrontlineCombatRecordV65& Now,const FFrontlineCombatRecordV65& Before);
    /** Replace this encounter's frozen snapshot; never adds its totals a second time. */
    bool CaptureCampaignBattleV86(FFrontlineCombatRecordV65& Campaign,const AWarfareFrontlineGameMode* Battle,int32 Mission,int32 Winner);
    void OpenCampaignV86(APlayerController* Player,const AWarfareFrontlineGameMode* Battle);
    void RunCampaignChecksV86(TFunctionRef<void(const FString&,bool)> Check);
    /** Strict disposable BattleReady writer probe: primes the actual getter, calls
     * both armies' event writers, and restores records/campaign/temporary actors.
     * No manual revision invalidation, settlement, save, or full projectile claim.
     */
    void RunLiveCacheWriterChecksV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    /** Strict no-save controller routing fixture; replaces only the disposable run. */
    void RunCampaignUIRouteChecksV86(AWarfareFrontlineGameMode* Battle,AWarfareFrontlineController* Player,TFunctionRef<void(const FString&,bool)> Check);
    /** Disposable synthetic layout only; no settlement, reward or profile writes. */
    bool PrepareCampaignVisualV86(AWarfareFrontlineGameMode* Battle,APlayerController* Player,int32 Tab,int32 Team=0);
#if WITH_EDITOR
    /** Explicit isolated synthetic national return; no natural completion claim. */
    bool PrepareNationalCampaignVisualV87(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation,TFunctionRef<void(const FString&,bool)> Check);
#endif
    FFrontlineDetailedRecordV839 Tail(const FFrontlineDetailedRecordV839& Now,const FFrontlineDetailedRecordV839& Before);
    void WriteArchive(FJsonObject& Json,const FFrontlineDetailedRecordV839& Record);
    bool ReadArchive(const FJsonObject& Json,FFrontlineDetailedRecordV839& Record,bool Earlier);
    void RunCodecChecks(TFunctionRef<void(const FString&,bool)> Check);

    struct FView {FGuid Encounter,Campaign;bool Open=false,CampaignV86=false;int32 Tab=0,Team=0,Offset=0;};
    FView& View(APlayerController* Player);
    bool Active(APlayerController* Player,const AWarfareFrontlineGameMode* Battle);
    bool Close(APlayerController* Player,const AWarfareFrontlineGameMode* Battle);
    bool Click(APlayerController* Player,AWarfareFrontlineGameMode* Battle,FName Command);
    bool Wheel(APlayerController* Player,const AWarfareFrontlineGameMode* Battle,int32 Direction);
}
/** Strict disposable real-receiver probe; restores its actors and encounter state. */
TSharedRef<FJsonObject> RunFrontlineReportV839Review(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
/** Strict disposable screenshot fixture; does not settle or save an outcome. */
bool PrepareFrontlineReportVisualV839(AWarfareFrontlineGameMode* Battle);
