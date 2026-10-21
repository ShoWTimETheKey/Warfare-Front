#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AFrontlineTank;
class FJsonObject;

struct FFrontlineKillTallyV65
{
    int32 Infantry=0,Armour=0;
    int32 Total() const { return Infantry+Armour; }
    void Add(const FFrontlineKillTallyV65& Other);
};

/** Capture at discharge: a delayed round retains its unit even after that unit dies. */
struct FFrontlineKillSourceV65
{
    int32 Team=INDEX_NONE,SquadId=INDEX_NONE,Role=INDEX_NONE;
    int32 VehicleSerial=INDEX_NONE,VehicleType=INDEX_NONE,Support=INDEX_NONE;
    int32 FireMissionId=INDEX_NONE;
    FString NationalVehicleIdV82;
    FString NationalWeaponIdV82;
    int32 NationalSourceNationV82=INDEX_NONE;
    static FFrontlineKillSourceV65 Soldier(const AWarfareFrontlineSoldier* Unit);
    static FFrontlineKillSourceV65 Vehicle(const AFrontlineTank* Unit);
    static FFrontlineKillSourceV65 FireSupport(int32 Team,int32 Support);
};

/** Exact V83.9 event totals. Missing fields in older archives remain unrecorded. */
struct FFrontlineSupportReportV839
{
    int32 Delivered=0,Rounds=0;
    double EnemyHullDamage=0,EnemyTrackDamage=0,FriendlyHullDamage=0,FriendlyTrackDamage=0;
    bool bDeliveryRecorded=false,bRoundsRecorded=false,bDamageRecorded=false;
};
struct FFrontlineSquadReportV839
{
    int32 Id=INDEX_NONE,OriginalRole=INDEX_NONE,Deployed=0,Lost=0;
    int32 BattleNumberV86=0; // Display identity across completed attempts; never a runtime squad ID.
    FFrontlineKillTallyV65 Kills,FriendlyFire;
};
struct FFrontlineDetailedRecordV839
{
    bool bRecorded=false,bIncludesEarlierUnrecordedBattles=false;
    FFrontlineKillTallyV65 FriendlyFire,FriendlySupport[10];
    int32 DeployedByRole[11]={},LostByRole[11]={};
    int32 ArmourLostByModel[13]={},ArmourDestroyedByModel[13]={};
    TMap<FString,int32> NationalArmourLost,NationalArmourDestroyed;
    FFrontlineSupportReportV839 Support[10];
    // Current attempt only: archive aggregation must not merge reused section IDs.
    TMap<int32,FFrontlineSquadReportV839> Squads;
    void Add(const FFrontlineDetailedRecordV839& Other);
    bool HasCounts() const;
};

struct FFrontlineCombatRecordV65;
/** Immutable per-attempt snapshots. Child records never carry another campaign ledger. */
struct FFrontlineCampaignBattleV86
{
    FGuid Encounter;
    int32 Mission=INDEX_NONE,Winner=INDEX_NONE;
    float Seconds=0;
    TSharedPtr<FFrontlineCombatRecordV65> Armies[2];
};

/** Completed battle attempts, including defeats and replays; no inferred historical kills. */
struct FFrontlineCombatRecordV65
{
    /** Runtime-only content token. Copies retain identity; new/reset/load/event writes invalidate it.
     * Mutating a retained battle snapshot must go through CaptureCampaignBattleV86;
     * direct future record writers must call InvalidateReportV86 after changing content.
     * Not serialized: an old archive loaded under the same campaign ID gets a fresh token.
     */
    static uint64 NewReportRevisionV86();
    uint64 ReportRevisionV86=NewReportRevisionV86();
    void InvalidateReportV86(){ReportRevisionV86=NewReportRevisionV86();}
    FFrontlineKillTallyV65 Total,InfantrySources[11],ArmourSources[13],SupportSources[10],OtherSources;
    TMap<FString,FFrontlineKillTallyV65> NationalArmourSourcesV82;
    int32 InfantryLost=0,ArmourLost=0,BattlesRecorded=0;
    bool bIncludesEarlierUnrecordedBattles=false;
    FFrontlineDetailedRecordV839 DetailsV839;
    TArray<FFrontlineCampaignBattleV86> BattlesV86;
    void Add(const FFrontlineCombatRecordV65& Other);
};

namespace FrontlineCombatRecordV65
{
    void InfantryLost(AWarfareFrontlineGameMode* Battle,int32 VictimTeam,const FFrontlineKillSourceV65& Source,const AWarfareFrontlineSoldier* Victim=nullptr);
    void ArmourLost(AWarfareFrontlineGameMode* Battle,int32 VictimTeam,const FFrontlineKillSourceV65& Source);
    /** Real victim identity makes both ordinary statistics and repeatable marks idempotent. */
    void ArmourDestroyedV8220(AWarfareFrontlineGameMode* Battle,const AFrontlineTank* Victim,const FFrontlineKillSourceV65& Source);
    bool ReadArchive(const FJsonObject& Json,FFrontlineCombatRecordV65& Record,bool HasEarlierBattles);
    void WriteArchive(FJsonObject& Json,const FFrontlineCombatRecordV65& Record);
    void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    /** In-memory new-platform/legacy-array codec checks; no profile or world writes. */
    void RunSourceCodecChecksV8218(TFunctionRef<void(const FString&,bool)> Check);
    TArray<AWarfareFrontlineSoldier*> PrepareCharacterReview(AWarfareFrontlineGameMode* Battle);
    /** Isolated, static tooltip layout fixture; its kill totals are synthetic. */
    AFrontlineTank* PrepareArmourReview(AWarfareFrontlineGameMode* Battle);
}
