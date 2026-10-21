#pragma once
#include "CoreMinimal.h"
#include "FRRU/WarfareFRRUIntegration.h"
#include "WarfareFrontlineProgression.h"
#include "WarfareFrontlineCombatRecordV65.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class AWarfareFrontlineSoldier;
class FJsonObject;

struct FFRRUCampaignSummaryV82
{
    FGuid Id;
    EFRRUNationV81 PlayerNation=EFRRUNationV81::Invalid;
    EFrontlineDifficulty Difficulty=EFrontlineDifficulty::Rookie;
    FString MissionId,Title,UpdatedUtc;
    int32 Victories=0,Defeats=0,Completed=0,ResearchPoints=2,InfantryKills=0,ArmourKills=0,Orders=0;
    FFrontlineCombatRecordV65 CombatRecord;
    bool HonourEligible=true;
};
struct FFRRUTacticalOptionV82
{
    FString Id,Title,Description,Tradeoff,BlockReason;
    int32 Cost=0,Index=0;bool Owned=false;
};

// Nation is an equipment/campaign identity. Side stays the original two-team
// battle index. Neither nation values nor stable IDs enter the old UK/DE file.
namespace WarfareFRRUCampaignV82
{
    // Independent national release gates. Only accepted French/Russian player
    // campaigns may open; unknown/enemy nations never inherit a shared gate.
    WARFAREFRONT_API bool CampaignsAvailable(EFRRUNationV81 Nation);
    // Read-only, exact disposable-user guard shared by campaign/account I/O.
    // Failure leaves User unchanged; it never selects a production fallback.
    WARFAREFRONT_API bool TryPersistenceReviewUserV82(FString& User);
    WARFAREFRONT_API bool PersistenceReviewRequestedV82(bool PublicOnly=false);
    WARFAREFRONT_API bool Active(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool Active(const AWarfareFrontlineController* Controller);
    WARFAREFRONT_API EFRRUNationV81 Nation(const AWarfareFrontlineGameMode* Battle,int32 Side=0);
    WARFAREFRONT_API EFRRUNationV81 Nation(const AWarfareFrontlineController* Controller,int32 Side=0);
    WARFAREFRONT_API EFRRUNationV81 Nation(const AWarfareFrontlineSoldier* Soldier);
    WARFAREFRONT_API int32 RoleId(const AWarfareFrontlineSoldier* Soldier);
    WARFAREFRONT_API int32 CalendarDate(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API int32 CalendarDate(const AWarfareFrontlineController* Controller);
    WARFAREFRONT_API FString MissionId(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FGuid CampaignId(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API EFrontlineDifficulty Difficulty(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API const TArray<FFrontlineCampaignMission>& Missions(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API const FFrontlineCampaignMission* SelectedMission(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API const FFrontlineCampaignMission* EncounterMission(const AWarfareFrontlineGameMode* Battle);
    // Read before leaving the report. Only a committed first victory in this
    // bound encounter can announce newly assigned equipment after the return.
    WARFAREFRONT_API bool HasNewAssignmentReceiptV87(const AWarfareFrontlineGameMode* Battle,int32 CompletedHostMissionId);
    WARFAREFRONT_API const TArray<FFRRUCampaignSummaryV82>& Archives(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool CurrentRecord(const AWarfareFrontlineGameMode* Battle,FFRRUCampaignSummaryV82& Out);
    // Borrowed until the next repository commit; use within the current call.
    WARFAREFRONT_API const FFrontlineCombatRecordV65* CampaignCombatRecordV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool CampaignReportMetadataV87(const AWarfareFrontlineGameMode* Battle,FGuid& Id,int32& Victories,int32& Defeats);
    // Commit only the additional casualties/support rounds after the outcome.
    WARFAREFRONT_API bool FinalizeCampaignCombatRecordV87(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool AvailableArchiveReplayV87(const AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation);
    WARFAREFRONT_API const TArray<FFrontlineCampaignMission>& ArchiveMissionsV87(const AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation);
    WARFAREFRONT_API bool BeginArchiveReplayV87(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation,int32 MissionOrder,EFrontlineDifficulty Level);
    WARFAREFRONT_API bool ArchiveReplayActiveV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool ArchiveReplayEligibleV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool ArchiveFullArmoryV87(const AWarfareFrontlineGameMode* Battle,int32 Side=0);
    WARFAREFRONT_API EFRRUNationV81 ArchiveReplayNationV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API int32 ArchiveReplayMissionV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API EFrontlineDifficulty ArchiveReplayDifficultyV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void LeaveArchiveReplayV87(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void MarkArchiveReplayAssistedV87(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool SetArchiveOptionEnabledV87(AWarfareFrontlineGameMode* Battle,const FString& Id,bool Enabled);
    WARFAREFRONT_API bool SuspendForLegacyArchiveV87(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void RestoreAfterLegacyArchiveV87(AWarfareFrontlineGameMode* Battle);
#if WITH_EDITOR
    // Scoped synthetic account qualification is supplied by the private caller.
    WARFAREFRONT_API void RunArchiveReplayChecksV87(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API bool PrepareParityCampaignReportV87(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 Nation,const FFrontlineCombatRecordV65& Record);
#endif
    WARFAREFRONT_API bool CanOpenEnding(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool HonourEligible(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool MarkHonourAssisted(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool Start(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 PlayerNation,EFrontlineDifficulty Level);
    WARFAREFRONT_API bool Continue(AWarfareFrontlineGameMode* Battle,const FGuid& Id=FGuid());
    WARFAREFRONT_API bool SelectMission(AWarfareFrontlineGameMode* Battle,const FString& StableId);
    WARFAREFRONT_API bool Delete(AWarfareFrontlineGameMode* Battle,const TArray<FGuid>& Ids);
    WARFAREFRONT_API void Deactivate(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FString Status(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FString MissionBlockReason(const AWarfareFrontlineGameMode* Battle,const FString& StableId);
    WARFAREFRONT_API int32 ResearchPoints(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool PurchaseOption(AWarfareFrontlineGameMode* Battle,const FString& StableId);
    WARFAREFRONT_API bool OwnsOption(const AWarfareFrontlineGameMode* Battle,const FString& StableId);
    WARFAREFRONT_API TArray<FFRRUTacticalOptionV82> Options(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API uint32 RoleSelection(const AWarfareFrontlineGameMode* Battle);
    // Real stage requirements, including requirements after the opening stage.
    WARFAREFRONT_API uint32 RequiredRoleMask(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FString DeploymentBlockReason(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API uint32 SupportSelection(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool SetSelection(AWarfareFrontlineGameMode* Battle,uint32 Roles,uint32 Supports);
    WARFAREFRONT_API TArray<FString> ArmourSelection(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool SetArmourSelection(AWarfareFrontlineGameMode* Battle,const TArray<FString>& StableIds);
    // Host-owned attachments use stable national mission IDs, never legacy masks.
    WARFAREFRONT_API int32 RequisitionV87(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool CanRequisitionV87(const AWarfareFrontlineGameMode* Battle,int32 Kind);
    WARFAREFRONT_API bool BuyRequisitionV87(AWarfareFrontlineGameMode* Battle,int32 Kind);
    WARFAREFRONT_API bool FrozenRequisitionV87(const AWarfareFrontlineGameMode* Battle,int32& Kind,float& SmokeReadyAt);
    // This remains closed until the real actor/geometry/order backend passes.
    WARFAREFRONT_API bool WorldReady(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FString WorldBlockReason(const AWarfareFrontlineGameMode* Battle);
}
