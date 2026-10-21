#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
enum class EFrontlineRole : uint8;
enum class EFrontlineSupport : uint8;

namespace FrontlineCampaignV32
{
    struct FLoadout
    {
        uint32 EligibleRoles=0, EligibleSupports=0, SelectedRoles=0, SelectedSupports=0;
        int32 RoleCapacity=5, SupportCapacity=6;
        bool bSelectRoles=false, bSelectSupports=false;
    };
    struct FHistoricalEvent
    {
        int32 Id=INDEX_NONE;
        const TCHAR* Title=TEXT("");
        const TCHAR* Date=TEXT("");
        const TCHAR* Body=TEXT("");
        const TCHAR* Consequence=TEXT("");
        const TCHAR* ArtKey=TEXT("Dispatch");
        int32 ResearchAward=0;
    };
    // An acknowledged dispatch remains in the campaign archive. Its practical
    // supply order applies only to its named pair of operations, once per battle.
    struct FDispatchConditionsV50
    {
        // SmokeAdvance is reserved for the first accepted smoke order's rearm;
        // these operations already make their opening smoke available at once.
        float ArtilleryAdvance=0.f, MortarAdvance=0.f, SmokeAdvance=0.f, MoraleReserve=0.f;
        bool IsActive()const{return ArtilleryAdvance!=0.f||MortarAdvance!=0.f||SmokeAdvance!=0.f||MoraleReserve!=0.f;}
    };
    WARFAREFRONT_API FDispatchConditionsV50 DispatchConditionsV50(int32 MissionId,uint32 AcknowledgedEvents);
    WARFAREFRONT_API const FFrontlineCampaignMission* CurrentOperation(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FLoadout GetLoadout(const AWarfareFrontlineGameMode* Battle);
    // The battle rail and requests share the actual selected support mask.
    // Unused rail slots are drawn as locked by the presentation layer.
    WARFAREFRONT_API TArray<EFrontlineSupport> GetEquippedSupports(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool RoleSelected(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Team=0);
    WARFAREFRONT_API bool SupportSelected(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Team=0);
    WARFAREFRONT_API FString RoleBlockReason(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Team=0);
    WARFAREFRONT_API FString SupportBlockReason(const AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support,int32 Team=0);
    WARFAREFRONT_API bool SetMobileGunV52(AWarfareFrontlineGameMode* Battle,bool Enabled);
    WARFAREFRONT_API bool ToggleRole(AWarfareFrontlineGameMode* Battle,EFrontlineRole Role);
    WARFAREFRONT_API bool ToggleSupport(AWarfareFrontlineGameMode* Battle,EFrontlineSupport Support);
    WARFAREFRONT_API FHistoricalEvent EventForMission(EFrontlineFaction Faction,int32 MissionId);
    WARFAREFRONT_API FHistoricalEvent PendingEvent(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool AcknowledgeEvent(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FString NewTechnologyBlockReason(const AWarfareFrontlineGameMode* Battle,int32 Id);
    WARFAREFRONT_API void ApplyMissionExtensions(TArray<FFrontlineCampaignMission>& Missions,bool German);
    WARFAREFRONT_API void AppendTechnologyNodes(TArray<FFrontlineTechnologyNode>& Nodes,bool German);
    WARFAREFRONT_API int32 BitCount(uint32 Mask);
    WARFAREFRONT_API void RunFocusedChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API void RunLoadoutChecksV60(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
    WARFAREFRONT_API void RunSupportChecksV70(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
