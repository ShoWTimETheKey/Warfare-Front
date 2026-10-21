#pragma once

#include "CoreMinimal.h"
#include "WarfareFrontlineCombatRecordV65.h"

/** National equipment is independent from the left/right command team. */
enum class EFrontlineFaction : uint8 { British, German };
/** Stable archive values; archives written before V30 remain Rookie. */
enum class EFrontlineDifficulty : uint8 { Rookie=0, Veteran=1, Survivor=2 };

/** Paid IDs and prices stay stable. UK9 / DE6 also commission the existing leader in V24. */
enum class EFrontlineTechnology : uint8
{
    RifleDrill=0, ReloadDrill=1, SupportingFire=2, ArtilleryLiaison=3,
    GrenadeDrill=4, FieldSupply=5, SectionTactics=6, GunPositions=7,
    FireRelief=8, AssaultCoordination=9, Observation=10, CombinedArms=11,
    SpecialistTraining=12, SpecialistTactics=13, AssaultLeadership=14, PreparedReload=15,
    RespiratorCare=16, PioneerClearance=17, FlameSection=18, CreepingBarrage=19,
    ContactPatrol=20, SOSBarrage=21, RailLiaison=22, VeteranReserves=23,
    SpecialistSection=24, AmmunitionCrew=25, FieldTelephone=26, MobileGunDoctrine=27, Count=28
};

struct FFrontlineTechnologyNode
{
    int32 Id=0, Prerequisite=INDEX_NONE, Branch=0, Tier=0, Cost=1;
    const TCHAR* Title=TEXT("");
    const TCHAR* Effect=TEXT("");
    int32 SecondaryPrerequisite=INDEX_NONE;
    const TCHAR* ArtKey=TEXT("");
    const TCHAR* BranchTitle=TEXT("");
};

struct FFrontlineArmyProgress
{
    int32 ResearchPoints=2;
    uint32 TechnologyMask=0;
    int32 Victories=0, Defeats=0;
};

enum class EFrontlineRole : uint8;
enum class EFrontlineCampaignObjective : uint8 { Breakthrough, HoldCenter };
enum class EFrontlineMissionTerrainV32 : uint8 { Farmland, Ridge, Marsh, Canal, RailCutting, Village };

/** Authored historical conditions on the shipped battlefield. */
struct FFrontlineCampaignMission
{
    int32 Id=0;
    const TCHAR* Title=TEXT("");
    const TCHAR* Location=TEXT("");
    const TCHAR* DateLabel=TEXT("");
    const TCHAR* Briefing=TEXT("");
    const TCHAR* ObjectiveText=TEXT("");
    int32 Year=1917;
    float RainBase=.30f, RainVariation=.20f, Wetness=.65f, VisibilityScale=1.f, MudMoveScale=1.f;
    const TCHAR* HistoricalContext=TEXT("");
    const TCHAR* TerrainNotes=TEXT("");
    const TCHAR* MapKey=TEXT("");
    FVector2D MapPosition=FVector2D::ZeroVector;
    bool bEnemyScopedMarksmen=false, bEnemyStormtroopers=false;
    bool bEnemyCommissionedOfficers=false;
    EFrontlineCampaignObjective Objective=EFrontlineCampaignObjective::Breakthrough;
    float HoldCenterSeconds=0.f;
    float PlayerMorale=100.f, EnemyMorale=100.f;
    float EnemyRecruitScale=1.f, EnemyAssaultDelay=9.f;
    float PlayerArtilleryDelay=114.285714f, EnemyArtilleryDelay=132.f;
    float PlayerSmokeDelay=12.f, PlayerMortarDelay=45.f;
    int32 RandomSeed=191708;
    TArray<EFrontlineRole> EnemyRoleOrder;
    // Priority recruitment after battle starts, with normal readiness and entry.
    // These roles never create soldiers during preparation or occupy a trench.
    TArray<EFrontlineRole> EnemyOpeningRoles;
    EFrontlineMissionTerrainV32 TerrainStyleV32=EFrontlineMissionTerrainV32::Farmland;
    float BattlefieldHalfWidth=900.f, WireDensity=.35f, MineDensity=0.f;
    bool bArmoredTrain=false;
    int32 TrainTeam=1;
    uint32 DisabledRoleMask=0, DisabledSupportMask=0;
    const TCHAR* RestrictionText=TEXT("");
    const TCHAR* AlliedContext=TEXT("");
    // Route identity is independent of the established art/attachment epochs.
    int32 LegacyArtV41=0, CalendarDateV41=19170409, ObjectiveLinesV41=3;
    int32 CommanderPortraitV41[2]={0,0};
    bool bCounterstrokeV41=false, bSupplyBayV41=false, bArmourAllowedV41=true;
    float DepthScaleV41=1.f;
};

/** Route, research and earned points belong to this archive alone. */
struct FFrontlineCampaignProgress
{
    FGuid Id;
    EFrontlineFaction Faction=EFrontlineFaction::British;
    EFrontlineDifficulty Difficulty=EFrontlineDifficulty::Rookie;
    int32 CompletedMissions=0;
    int32 SelectedMission=0;
    int32 RouteRevision=41;
    FString CreatedUtc, UpdatedUtc;
    FFrontlineArmyProgress Research;
    // Zero selection is the compatible automatic loadout for pre-V32 archives.
    uint32 RoleSelectionMask=0, SupportSelectionMask=0;
    uint32 ArmourResearchMaskV63=0,ArmourSelectionMaskV63=0;
    bool bArmourSelectionExplicitV84413=false;
    // Configuration belongs to the platform even when that platform is deselected.
    uint32 ArmourConfigurationMaskV836=0;
    // Only paid attachments on the current operation are refundable.
    uint32 RequisitionPaidMaskV836=0;
    uint32 PioneerOrdersMaskV63=0,SmokeOrdersMaskV63=0,ArmourOrdersMaskV63=0;
    uint32 AcknowledgedEventMask=0;
    // V52 keeps the chronological frontier above, and records actual victories
    // separately: a branch not assigned to this company is never a fictitious win.
    int32 StrategicRevisionV52=0;
    uint32 WonOperationMaskV52=0,BypassedOperationMaskV52=0;
    uint32 DispatchResolvedMaskV52=0,DispatchOrderMaskV52=0,DispatchChoiceMaskV52=0;
    bool bMobileGunV52=false;
    bool bAchievementEligibleV76=true;
    FFrontlineCombatRecordV65 CombatRecordV65;
};

class AWarfareFrontlineGameMode;
namespace FrontlineProgressionReviewV8219
{
/** Read-only receipt diagnostic for the disposable initial-save retry fixture. */
bool InitialOutcomePending(const AWarfareFrontlineGameMode* Battle);
/** In-memory UTF-8 and size-boundary checks; no profile or disk access. */
void RunCapacityChecksV86(TFunctionRef<void(const FString&,bool)> Check);
}
