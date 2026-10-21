#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "WarfareFrontlineDeath.h"
#include "WarfareFrontlineProgression.h"
#include "WarfareFrontline.generated.h"

class USkeletalMeshComponent;
class UCapsuleComponent;
class USkeletalMesh;
class UAnimationAsset;
class ACameraActor;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AWarfareFrontlineController;
class AFrontlineTank;
class UPhysicsAsset;
class FJsonObject;
struct FFrontlineMissionIntroduction;
namespace FRRUNaturalReviewV82 { struct FScheduler; }

enum class EFrontlinePhase : uint8 { Briefing, Fighting, Victory, Defeat };
// Append new roles: the original three indexes remain compatible with V13 art and reports.
enum class EFrontlineRole : uint8 { Rifle, Lewis, Mills, Marksman, Officer, Flamethrower, Pioneer, RifleGrenadier, AntiTankRifle, MobileGun, Signals, Count };
enum class EFrontlineSupport : uint8 { Artillery, Smoke, Rally, Mortar, Gas, AntiTank, AirStrike, CreepingBarrage, AirRecon, AirBombing, Count };
enum class EFrontlineWeaponPhase : uint8 { Ready, Recoil, BoltCycle, Reload, GrenadeThrow, OfficerCommand };

struct FFrontlineSmokeZone
{
    bool bGas=false;
    int32 Team=0;
    FVector Centre=FVector::ZeroVector;
    float Radius=850;
    float StartedAt=0,ExpiresAt=0;
    float NextVoiceCheckAt=0;
    bool bCoverCallMade=false;
};

/** World-space smoke using the same authored dust material as impacts. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineSmoke : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineSmoke();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    bool bVisualReady=false;
    float DeployAt=0; // V26: game-clock arrival, not an immediate smoke screen.
private:
    float Age=0;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Puffs;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
};

struct FFrontlineTrench
{
    float X=0;
    int32 Owner=INDEX_NONE;
    bool bBypass=false;
    static constexpr int32 Capacity=3;
};

struct FFrontlineSquad
{
    int32 Id=INDEX_NONE;
    int32 Team=0;
    EFrontlineRole Role=EFrontlineRole::Rifle;
    int32 Trench=INDEX_NONE;
    int32 Destination=INDEX_NONE;
    int32 LastPassedTrench=INDEX_NONE;
    int32 Slot=0;
    bool bAdvancing=false;
    bool bReinforcement=false;
    bool bHoldPosition=false;
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Members;
    FFrontlineKillTallyV65 KillsV65;
    int32 LivingCount() const;
    FVector Centre() const;
};

struct FFrontlineStrike
{
    FFrontlineKillSourceV65 SourceV65;
    int32 Team=0;
    FVector Target=FVector::ZeroVector;
    FVector Origin=FVector::ZeroVector;
    float LaunchedAt=0;
    float ImpactAt=0;
    float Radius=360;
    TWeakObjectPtr<AActor> Projectile;
    bool bDirectFire=false;
    float ArmorPower=0.f;
    bool bGrenade=false;
    bool bOrdinaryHandGrenadeV835=false; // Captured at release; transient strike state, never a player archive field.
    bool bAntiTankRifleGrenadeV836=false; // No.44 payload is fixed when the grenade leaves the rifle.
    bool bMineV8214=false; // Runtime blast profile; never serialized into player archives.
    bool bRifleLaunched=false;
    bool bFrenchVBRound=false; // Runtime projectile axis only; no archive field.
    bool bMortar=false;
    bool bAirDropped=false;
    bool bShrapnelV64=false; // Timed airburst; lethal fragments, no armour/earth blast.
    bool bResolved=false;
    bool bReportRoundRecordedV839=false; // Encounter-local discharge receipt; never serialized.
    bool bIncomingSoundPlayed=false;
    int32 FireMissionId=INDEX_NONE,SalvoIndex=0,RoundIndex=0;
    float WarningLeadSeconds=1.2f;
};

struct FFrontlineTracer
{
    FVector Start=FVector::ZeroVector;
    FVector End=FVector::ZeroVector;
    float Life=.05f,Duration=.05f;
};

UCLASS()
class WARFAREFRONT_API AWarfareFrontlineSoldier : public AActor
{
    GENERATED_BODY()
public:
    AWarfareFrontlineSoldier();
    virtual void Tick(float DeltaSeconds) override;
    void Initialize(AWarfareFrontlineGameMode* InBattle,int32 InSquad,int32 InTeam,EFrontlineRole InRole,int32 InMember);
    // Positive finite Impact means a landed, fatal hit. Zero means pressure only.
    void ReceiveHit(float Impact,float Pressure,int32 AttackingTeam,const FVector& ForceDirection=FVector::ZeroVector,EFrontlineDamageKind Kind=EFrontlineDamageKind::Bullet,float ForceScale=1.f,const FFrontlineKillSourceV65& SourceV65={});
    void TickDeath(float DT);
    const FFrontlineDeathState& GetDeathState() const { return DeathState; }
    void ReturnToIdle();
    bool IsAlive() const { return Health>0; }
    bool IsActiveCombatant() const;
    // Direct in-field fixture/level spawns remain active. Normal reinforcements
    // set this false until they physically cross their faction's entry line.
    bool bFieldEntered=true;
    bool IsRallied() const;
    // National contact moves the real clearance capsule while keeping its
    // authored feet on the ground. The original actor/mesh offset stays 88.
    float GroundContactOffsetV82=88.f;
    FVector Feet() const { return GetActorLocation()-FVector(0,0,GroundContactOffsetV82); }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Capsule;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Soldier;
    UPROPERTY(Transient) TObjectPtr<AWarfareFrontlineGameMode> Battle;
    int32 SquadId=INDEX_NONE,Team=0,MemberIndex=0;
    EFrontlineRole Role=EFrontlineRole::Rifle;
    // Compatibility marker only: 100 alive, 0 dead. No wounds or health recovery.
    float Health=100,Suppression=0,DistanceTravelled=0,AnimationTimeAdvanced=0;
    // An operator's first landed firearm round spends this once-only protection.
    // It never creates a fractional health/wound state or regenerates in cover.
    bool bFlameBulletProtectionSpentV48=false;
    FFrontlineKillSourceV65 FatalSourceV65;
    float RallyUntil=0;
    bool bMoving=false,bEngaging=false;
    FVector FormationOffset=FVector::ZeroVector;
    float FirstMovementAt=-1,FirstShotAt=-1;
    float LastAutomaticFireAt=-1000.f;
    int32 MotionTransitions=0,ShotsDischarged=0,ShotsWhileMoving=0;
    float GetMotionSpeed() const { return MotionSpeed; }
    float GetMovementScale() const { return MovementScale; }
    float GetMotionPhase() const { return MotionPhase; }
    AWarfareFrontlineSoldier* GetObservedInfantryTargetV844() const { return FocusTarget.Get(); }
    bool IsMachineGunner() const { return (Role==EFrontlineRole::Lewis || Role==EFrontlineRole::MobileGun) && (MemberIndex==0 || bRecoveredMachineGun); }
    bool IsScopedMarksman() const { return bScopedWeapon; }
    bool IsCommissionedOfficer() const { return bCommissionedOfficer; }
    void ConfigureOfficerEquipment();
    bool BeginOfficerCommand();
    bool bOfficerSeenReported=false;
    bool IsStormtrooper() const { return bStormWeapon; }
    bool IsAutomaticWeapon() const { return IsMachineGunner() || IsStormtrooper(); }
    float HeavyRecoilAgeV8215() const { return Role==EFrontlineRole::AntiTankRifle&&MemberIndex==0?MuzzleAge:10.f; }
    void ConfigureSpecialistEquipment();
    bool RecoverMachineGunFrom(AWarfareFrontlineSoldier* FallenGunner);
    bool UpdateMachineGunRecovery(float DT);
    bool bRecoveredMachineGun=false;
    float GunRecoveryReadyAt=0.f;
    bool IsSectionLeader() const { return Role==EFrontlineRole::Officer && MemberIndex==0; }
    int32 GetVisualRoleIndex() const;
    int32 GetPresentationRoleIndex() const;
    float CommandReadyAt=0;
    float LastSectionOrderAt=-12.f;
    EFrontlineFaction GetFaction() const;
    int32 GetMagazineCapacity() const;
    UAnimationAsset* GetIdleMotion() const;
    UAnimationAsset* GetAimMotion() const;
    UAnimationAsset* GetReloadMotion() const;
    float GetWeaponReloadNormalizedPhase() const;
    UAnimationAsset* GetWalkMotion() const;
    UAnimationAsset* GetRunMotion() const;
    FVector GetMuzzleLocation() const;
    void ShowWeaponDischarge();
    void ShowBulletImpact(const FVector& Position);
    // A spent fuel pack affects its existing casualty, without registering a second death.
    void OnFuelRuptureV41(float RemainingFuel);
    FVector GetGrenadeReleaseLocation() const;
    void ReleaseGrenade();
    void StartGrenadeThrow(AWarfareFrontlineSoldier* Target,AFrontlineTank* VehicleTarget=nullptr);
    int32 GrenadeThrowsStarted=0,GrenadesReleased=0;
    int32 RoundsRemaining=0,ReloadsStarted=0,ReloadsCompleted=0,BoltCyclesCompleted=0,BurstsStarted=0;
    int32 ShotsDuringReload=0;
    EFrontlineWeaponPhase WeaponPhase=EFrontlineWeaponPhase::Ready;
    TArray<float> ShotTimes;
private:
    FFrontlineDeathState DeathState;
#if WITH_EDITOR
    friend struct FFrontlineTripoCharacterAccessV87;
    friend struct FRRUNaturalReviewV82::FScheduler;
#endif
    friend class AFrontlineTank;
    friend struct FFrontlineEnemyChecksV84415;
    friend struct FFrontlineChallengeFeasibilityV84415;
    friend struct FFrontlineRailGameplayV837Review;
    friend struct FFrontlineArmourArtV838Review;
    friend struct FFrontlineMovementV35Review;
    friend struct FFrontlineCombatV63Review;
    friend struct FFrontlineCombatV64Review;
    friend struct FFrontlineTrenchCombatV69Review;
    friend struct FFrontlineTrenchMotionV8444Review;
    friend struct FFrontlineTrenchMotionV8447Review;
    friend struct FFrontlineMachinegunV8450Review;
    friend struct FFrontlineCrewV84411Review;
    friend struct FFrontlineArmourMobilityAccessV84411;
    friend struct FFrontlineBalanceV8447Review;
    friend struct FFrontlineUIV8447Review;
    friend struct FFrontlineContactV835Review;
    friend struct FFrontlineArmourV836Review;
    friend struct FFrontlineGameplayReviewV75;
    friend struct FFrontlineChoiceTargetsV78Review;
    friend struct FFrontlineMoraleV76Access;
    friend struct FFrontlineReviewV76;
    friend struct FFrontlineArmourV63Access;
    friend struct FFrontlineArmourConfigurationV63Review;
    friend struct FFrontlineReviewV63;
    friend struct FFrontlineReviewV64;
    friend struct FFrontlineReviewV65;
    friend struct FFrontlineReviewV49;
    friend struct FFrontlineReviewV50;
    friend struct FFrontlineReviewV51;
    friend struct FFrontlineReviewV52;
    friend struct FFrontlineReviewV53;
    friend struct FFrontlineGameplayV43Review;
    friend struct FFrontlineInfantryV45Review;
    friend class UFrontlineMachinegunV30;
    friend struct FFrontlineMGReviewV36;
    friend struct FFrontlineMotionV40Access;
    friend struct FFrontlineSpecialistsV41Review;
    friend struct FFrontlineMotionV41Access;
    friend struct FFRRUPresentationAccessV82;
    friend void StartFrontlineV34Review(AWarfareFrontlineController* Controller);
    friend void StartFrontlineV29Review(AWarfareFrontlineController* Controller);
    friend struct FFrontlineMGReviewV30;
    friend struct FFrontlineStanceV8216Review;
    friend struct FFrontlineCombatV8217Review;
    friend struct FFrontlineCombatV8218Review;
    friend struct FFrontlineFireControlV831Review;
    friend struct FFrontlineTimeBudgetV8219Review;
    friend TSharedRef<FJsonObject> RunFrontlineV28Checks(AWarfareFrontlineGameMode* Battle);
    friend TSharedRef<FJsonObject> RunFrontlineV27CampaignChecks(AWarfareFrontlineGameMode* Battle);
    void BeginDeathPhysics();
    void ApplyFuelRuptureImpulseV41();
    UPROPERTY(Transient) TObjectPtr<UPhysicsAsset> DeathPhysicsAsset;
    int32 BurstRemaining=0;
    float ArmourObstructionSecondsV828=0.f;
    FVector ArmourRecoveryOriginV828=FVector::ZeroVector;
    float NativeInfantryObstructionSecondsV87=0.f;
    FVector NativeInfantryRecoveryOriginV87=FVector::ZeroVector;
    void UpdateWeaponCycle(float DT);
    void DischargeWeapon(AWarfareFrontlineSoldier* Target,AFrontlineTank* VehicleTarget=nullptr);
    void StartReload();
    FVector GrenadeAimPoint=FVector::ZeroVector;
        bool GrenadeReleased=false;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> HeldGrenade;
    void InitializeWeaponEffects();
    void TickWeaponEffects(float DT);
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MuzzleFlash;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> MuzzleLight;
    float WeaponHeat=0;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MuzzleSmoke;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BulletDust;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BulletDustMaterial;
    FVector LastBulletImpact=FVector::ZeroVector;
    float BulletDustAge=10;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MuzzleFlashMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MuzzleSmokeMaterial;
    float MuzzleAge=10;
    FVector LastMuzzlePosition=FVector::ZeroVector;
    float ShotClock=0,GrenadeClock=0,FirePoseClock=0,LastAnimationTime=0;
    float ArmourGrenadeReadyAtV833=-1.f;
    float ReloadDurationSecondsV82=0;
    float MotionSpeed=0,MotionPhase=0,MovementScale=1,CadenceScale=1,BreathingRate=1;
    float RequestedTravelSpeedV43=0;
    float TrenchFallSpeedV69=0;
    bool bAtDestinationV43=false;
    float DepartureClock=0,ReactionSeconds=0,AimSettleClock=0,ContactHoldClock=0,ContactReactionClock=0,TargetReviewClock=0;
    bool bWantedMovement=false,bWasSettled=true,bHadAdvanceContact=false;
    FRandomStream MotionRandom;
    TWeakObjectPtr<AWarfareFrontlineSoldier> FocusTarget;
    TWeakObjectPtr<AWarfareFrontlineSoldier> RecoveryGunner;
    bool bScopedWeapon=false, bStormWeapon=false;
    bool bCommissionedOfficer=false;
    // Low ready, aim, walk, run, fire, national reload, national command.
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimationAsset>> OfficerMotions;
    bool bSniperReadyCalled=false, bStormAdvanceCalled=false;
    float QuietReloadClock=0.f;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> StormFireMotion;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> StormReloadMotion;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> Playing;
    void SetMotion(UAnimationAsset* Animation,bool bLoop=true,bool bAlreadyResolved=false);
};

/** One-axis trench battle with shared infantry readiness and independent artillery preparation. */
UCLASS()
class WARFAREFRONT_API AWarfareFrontlineGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AWarfareFrontlineGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void BeginBattle();
    bool RestartBattle();
    void AbandonBattleToOperations();
    bool Recruit(EFrontlineRole InRole,int32 Team=0);
    int32 RoleUnlockMission(EFrontlineRole Role,int32 Team=0) const;
    bool IsRoleUnlocked(EFrontlineRole Role,int32 Team=0) const;
    FString RoleUnlockReason(EFrontlineRole Role,int32 Team=0) const;
    FString TechnologyRoleBlockReason(int32 TechnologyId) const;
    FFrontlineMissionIntroduction GetMissionIntroduction() const;
    bool AdvanceTrench(int32 TrenchIndex,int32 Team=0);
    TArray<int32> GetTrenchSquadSlots(int32 TrenchIndex,int32 Team=0) const;
    FString SquadAdvanceBlockReason(int32 TrenchIndex,int32 SquadId,int32 Team=0) const;
    bool AdvanceSquad(int32 TrenchIndex,int32 SquadId,int32 Team=0);
    bool SetSquadHold(int32 TrenchIndex,int32 SquadId,bool bHold,int32 Team=0);
    void BeginV17AgileVerification();
    bool StartNewCampaign(EFrontlineFaction Faction,EFrontlineDifficulty Difficulty=EFrontlineDifficulty::Rookie);
    EFrontlineDifficulty GetCampaignDifficulty() const;
    bool ContinueCampaign(const FGuid& CampaignId=FGuid());
    /** Save only the already-settled encounter's final combat-record delta. */
    bool FinalizeCampaignCombatRecordV8217();
    bool DeleteCampaignArchives(const TArray<FGuid>& CampaignIds);
    bool SelectCampaignMission(int32 MissionId);
    const TArray<FFrontlineCampaignProgress>& GetCampaignRuns() const { return CampaignRuns; }
    const FFrontlineCampaignProgress* GetCurrentCampaign() const;
    void MarkAchievementAssistedV76();
    const TArray<FFrontlineCampaignMission>& GetCampaignMissions() const;
    static const TArray<FFrontlineCampaignMission>& GetCampaignMissionsForFaction(EFrontlineFaction Faction);
    const FFrontlineCampaignMission* GetSelectedCampaignMission() const;
    const FFrontlineCampaignMission* GetEncounterCampaignMission() const;
    FString CampaignMissionBlockReason(int32 MissionId) const;
    void BindCampaignEncounter();
    bool WasCampaignMissionAdvanced() const { return bCampaignMissionAdvanced; }
    float GetCampaignObjectiveProgress() const;
    void RunV17CampaignChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV18BattlefieldChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV20TrenchChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV21FactionChecks(TFunctionRef<void(const FString&,bool)> Check);
    void BeginV18AgileVerification();
    bool RequestArtillery(const FVector& Target,int32 Team=0);
    bool RequestSmoke(const FVector& Target,int32 Team=0);
    bool SetTrenchBypass(int32 TrenchIndex,bool bBypass,int32 Team=0);
    bool RequestRally(const FVector& Target,int32 Team=0);
    bool RequestMortar(const FVector& Target,int32 Team=0);
    bool RequestSupport(EFrontlineSupport Support,const FVector& Target,int32 Team=0);
    const TCHAR* GetSupportName(EFrontlineSupport Support,int32 Team=0) const;
    const TCHAR* GetSupportDescription(EFrontlineSupport Support,int32 Team=0) const;
    int32 SupportUnlockMission(EFrontlineSupport Support,int32 Team=0) const;
    bool IsSupportUnlocked(EFrontlineSupport Support,int32 Team=0) const;
    FString SupportUnlockReason(EFrontlineSupport Support,int32 Team=0) const;
    float SupportCooldown(EFrontlineSupport Support,int32 Team=0) const;
    FString SupportBlockReason(EFrontlineSupport Support,int32 Team=0) const;
    void SetBattleSpeed(float Multiplier);
    void SetPaused(bool bPaused);
    void BeginVerification(); // Implemented independently in WarfareFrontlineVerification.cpp.
    void BeginMotionReview(); // Optional close-camera frame capture in the actual encounter.
    void BeginPerformanceReview(); // Actual encounter, no screenshot readback during sampling.
    void BeginV22PerformanceReview();
    void BeginV22ImageReview();
    void BeginV23MechanicsVerification();
    EFrontlineFaction GetPlayerFaction() const { return PlayerFaction; }
    FGuid GetEncounterId() const { return EncounterId; }
    EFrontlineFaction GetFactionForTeam(int32 Team) const;
    int32 GetFactionIndexForTeam(int32 Team) const { return int32(GetFactionForTeam(Team)); }
    int32 GetTeamDirection(int32 Team) const;
    int32 GetHomeTrenchIndex(int32 Team=0) const;
    int32 GetGoalTrenchIndex(int32 Team=0) const;
    float GetHomeCameraFocusX() const;
    const TCHAR* GetFactionName(int32 Team=0) const;
    const TCHAR* GetRecruitName(EFrontlineRole Role,int32 Team=0) const;
    int32 GetSquadSize(EFrontlineRole Role,int32 Team=0) const;
    bool SelectPlayerFaction(EFrontlineFaction Faction);
    const TArray<FFrontlineTechnologyNode>& GetTechnologyNodes() const;
    static const TArray<FFrontlineTechnologyNode>& GetTechnologyNodesForFaction(EFrontlineFaction Faction);
    bool HasTechnology(int32 Id,int32 Team=0) const;
    FString TechnologyBlockReason(int32 Id) const;
    bool PurchaseTechnology(int32 Id);
    const FFrontlineArmyProgress& GetActiveResearch() const;
    int32 GetResearchPoints() const;
    bool HasScopedMarksmen(int32 Team=0) const;
    bool HasCommissionedOfficers(int32 Team=0) const;
    void ReportOfficerSpotted(const AWarfareFrontlineSoldier* Observer,AWarfareFrontlineSoldier* Officer) const;
    void BeginV24OfficerVerification();
    void BeginV24OfficerVisual();
    void RunV24PacingChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV35MovementChecks(TFunctionRef<void(const FString&,bool)> Check);
    void RunV38CombatChecks(TFunctionRef<void(const FString&,bool)> Check);
    bool SetupV35MovementVisualFixture();
    void BeginV24GrenadeReview();
    bool HasStormtroopers(int32 Team=0) const;
    int32 GetEncounterYear() const;
    float EncounterVisibilityScale() const;
    float EncounterMudMoveScale() const;
    int32 GetLastResearchReward() const { return LastResearchReward; }
    int32 GetDisplayedResearchPointsV828() const { return ResearchBeforeOutcomeV828>=0?ResearchBeforeOutcomeV828:GetResearchPoints(); }
    const FString& GetProgressionSaveStatus() const { return ProgressionSaveStatus; }
    bool IsProgressionVolatile() const { return bProgressionVolatile; }
    // Restricted disposable review: never uses the ordinary player save root.
    bool PrepareContinuePersistenceReviewV8214();
    bool IsProgressionReadOnly() const { return bProgressionReadOnly; }
    float WeaponReloadScale(int32 Team) const;
    float EffectiveRecruitSeconds(EFrontlineRole Role,int32 Team=0) const;
    float EffectiveArtillerySeconds(int32 Team=0) const;
    void BeginProgressionVerification();
    void BeginV15TechnologyVerification();
    bool HasFactionTechnology(EFrontlineFaction Faction,int32 Id,int32 Team=0) const;
    bool IsInFriendlySmoke(const FVector& Point,int32 Team) const;
    bool HasForwardObserver(const FVector& Point,int32 Team) const;
    float IncomingBulletPressureScale(const AWarfareFrontlineSoldier* Unit) const;
    float IncomingBulletHitChanceScale(const AWarfareFrontlineSoldier* Unit) const;
    float BulletHitChance(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target) const;
    float ExplosionHitChance(const FFrontlineStrike& Strike,const AWarfareFrontlineSoldier* Target) const;
    float OutgoingPressureScale(const AWarfareFrontlineSoldier* Shooter,const AWarfareFrontlineSoldier* Target) const;
    float AdjacentPressureRadius(const AWarfareFrontlineSoldier* Shooter) const;
    float AssaultContactScale(const AWarfareFrontlineSoldier* Unit) const;
    void BeginV13FeatureReview();
    void BeginV14GameplayVerification();
    float RecruitCooldown(EFrontlineRole InRole,int32 Team=0) const;
    float ArtilleryCooldown(int32 Team=0) const;
    float SmokeCooldown(int32 Team=0) const;
    float RallyCooldown(int32 Team=0) const;
    float MortarCooldown(int32 Team=0) const;
    float Income(int32 Team=0) const;
    float CommandIncome(int32 Team=0) const;
    FString RecruitBlockReason(EFrontlineRole InRole,int32 Team=0) const;
    FString AdvanceBlockReason(int32 TrenchIndex,int32 Team=0) const;
    FString ArtilleryBlockReason(int32 Team=0) const;
    FString SmokeBlockReason(int32 Team=0) const;
    FString RallyBlockReason(int32 Team=0) const;
    FString MortarBlockReason(int32 Team=0) const;
    int32 ActiveSquads(int32 Team) const;
    int32 LivingSoldiers(int32 Team) const;
    int32 TrenchSquadCount(int32 TrenchIndex,int32 Team) const;
    FFrontlineSquad* FindSquad(int32 Id);
    const FFrontlineSquad* FindSquad(int32 Id) const;
    FVector SoldierDestination(const AWarfareFrontlineSoldier* Unit) const;
    bool ShouldFinishFriendlyTrenchEntryV69(const AWarfareFrontlineSoldier* Unit) const;
    AWarfareFrontlineSoldier* AcquireTarget(const AWarfareFrontlineSoldier* Unit,bool bAdvanceContact=false) const;
    bool HasSight(const AWarfareFrontlineSoldier* From,const AWarfareFrontlineSoldier* To) const;
    float CoverFor(const AWarfareFrontlineSoldier* Unit) const;
    void Fire(AWarfareFrontlineSoldier* Shooter,AWarfareFrontlineSoldier* Target);
    void ThrowGrenade(AWarfareFrontlineSoldier* Shooter,AWarfareFrontlineSoldier* Target);
    void ThrowGrenadeAt(AWarfareFrontlineSoldier* Shooter,const FVector& Target);
    void RegisterCasualty(AWarfareFrontlineSoldier* Unit,int32 AttackingTeam);
    void ResolveFieldStrikeV32(FFrontlineStrike& Strike) { ResolveStrike(Strike); }
    static const TCHAR* RoleName(EFrontlineRole Role);
    static int32 RoleSize(EFrontlineRole Role,int32 Team=0);
    static float RoleRecruitSeconds(EFrontlineRole Role);
    static float RoleFireRange(EFrontlineRole Role);
    static float RoleAdvanceContactRange(EFrontlineRole Role);
    static float RoleCost(EFrontlineRole InRole);
    static float SupportCost(EFrontlineSupport Support);
    static float SupportCooldownSeconds(EFrontlineSupport Support);
    FVector2D FireSupportScatterExtent(EFrontlineSupport Support,int32 Team=0) const;
    FVector2D FireSupportDelayRange(EFrontlineSupport Support,const FVector& Target,int32 Team=0) const;
    bool IsFireSupportAreaSafe(const FVector& Target,EFrontlineSupport Support,int32 Team) const;
    float DirectFireMultiplier(const FVector& From,const FVector& To) const;

    EFrontlinePhase Phase=EFrontlinePhase::Briefing;
    float BattleOutroElapsed=0.f;
    int32 BattleOutroWinner=INDEX_NONE;
    bool IsBattleOutroActive() const { return BattleOutroWinner>=0 && BattleOutroElapsed<7.f; }
    bool CanResolveCommittedSupportV8217() const { return Phase==EFrontlinePhase::Fighting || ((Phase==EFrontlinePhase::Victory || Phase==EFrontlinePhase::Defeat) && BattleOutroWinner>=0); }
    float CommittedSupportTimeV8217() const { return Elapsed+((Phase==EFrontlinePhase::Victory || Phase==EFrontlinePhase::Defeat) && BattleOutroWinner>=0?BattleOutroElapsed:0.f); }
    bool IsResolvingCommittedSupportV8217() const { return bResolvingCommittedSupportV8217; }
    TArray<FFrontlineTrench> Trenches;
    TArray<FFrontlineSquad> Squads;
    TArray<FFrontlineStrike> Strikes;
    TArray<FFrontlineTracer> Tracers;
    TArray<FFrontlineSmokeZone> SmokeZones;
    UPROPERTY(Transient) TArray<TObjectPtr<AWarfareFrontlineSoldier>> Units;
    float Morale[2]={100,100};
    // Legacy reporting fields remain for V09 compatibility; V10 has no wallets.
    float Supplies[2]={0,0},CommandPoints[2]={0,0};
    static constexpr float SuppliesCap=600;
    static constexpr float CommandCap=100;
    static constexpr int32 RoleCount=int32(EFrontlineRole::Count);
    float RecruitReadyAt[2][RoleCount]={};
    float ArtilleryReadyAt[2]={4000.f/35.f,4000.f/35.f},SmokeReadyAt[2]={0,0},RallyReadyAt[2]={0,0};
    float MortarReadyAt[2]={45,45};
    float ExtendedSupportReadyAt[2][3]={{52,65,80},{74,85,105}};
    int32 ExtendedSupportCommands[3]={};
    float ExtendedSupportThinkAt=8.f;
    float Elapsed=0,BattleSpeed=1;
    FString LastMessage,AssetFailure,EndReason;
    int32 ShotsFired=0,RecruitCommands=0,AdvanceCommands=0,ArtilleryCommands=0,GrenadesThrown=0,TrenchCaptures=0;
    int32 Casualties[2]={0,0};
    FFrontlineCombatRecordV65 CombatRecordV65[2];
    float DamageDealt[2]={0,0},FriendlyFireDamage=0,PeakSuppression=0;
    int32 SmokeCommands=0,RallyCommands=0,SmokeAffectedShots[2]={0,0};
    int32 MortarCommands=0,MortarRoundsResolved=0,LeaderOrders[2]={0,0};
    float LeaderSuppressionRemoved[2]={0,0};
    float SuppliesEarned[2]={0,0},SuppliesSpent[2]={0,0},CommandEarned[2]={0,0},CommandSpent[2]={0,0};
    float RallySuppressionRemoved[2]={0,0},RallyMoraleRestored[2]={0,0};
    UPROPERTY(EditDefaultsOnly, Category="Presentation") float MeshYawOffset=-90.f;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> LoadedSoldier;
    // Faction * 3 + visual role. Automatic-weapon assistants carry their faction's rifle.
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMesh>> RoleSoldierMeshes;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMesh>> V16SoldierMeshes;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> IdleAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> WalkAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> RunAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> AimAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> FireAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> BoltAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> TrenchCycleAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> ReloadAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> AutomaticFireAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> MGIdleAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> MGAimAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> MGWalkAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> MGRunAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> LewisReloadAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> GermanMGReloadAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> GrenadeAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> DeathAnimation;
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimationAsset>> DeathAnimations;
private:
    EFrontlineFaction PlayerFaction=EFrontlineFaction::British;
    friend struct FFrontlineArmourContactReviewV84414;
    friend struct FFrontlineArmourApproachReviewV85;
    friend struct FFrontlineEnemyChecksV84415;
    friend struct FFrontlineChallengeFeasibilityV84415;
    friend struct FFRRUWorldOutcomeAccessV82;
    friend struct FFRRUArmourAccessV82;
    friend struct FFrontlineCampaignV32Access;
    friend struct FFrontlineAchievementReviewV76;
    friend struct FFrontlineRailGameplayV837Review;
    friend struct FFrontlineArmourV836Review;
    friend struct FFrontlineArmourArtV838Review;
    friend struct FFrontlineCheatCampaignReviewV8222;
    friend struct FFrontlineCampaignV41Access;
    friend struct FFrontlineCampaignV51Access;
    friend struct FFrontlineCampaignV52Access;
    friend struct FFrontlineGameplayV33Access;
    friend struct FFrontlineTrenchMotionV8444Review;
    friend struct FFrontlineTrenchMotionV8447Review;
    friend struct FFrontlineMachinegunV8450Review;
    friend struct FFrontlineCrewV84411Review;
    friend struct FFrontlineArmourMobilityAccessV84411;
    friend struct FFrontlineBalanceV8447Review;
    friend struct FFrontlineUIV8447Review;
    friend struct FFrontlineGameplayV43Review;
    friend struct FFrontlineReadinessChecksV84413;
    friend struct FFrontlineInfantryV45Review;
    friend struct FFrontlinePerformanceV33Access;
    friend struct FFrontlineEnvironmentReviewV42;
    friend struct FFrontlineReviewV43;
    friend struct FFrontlineReviewV45;
    friend struct FFrontlineReviewV46;
    friend struct FFrontlineReviewV47;
    friend struct FFrontlineReviewV48;
    friend struct FFrontlinePromoV49;
    friend struct FFrontlineCombatV63Review;
    friend struct FFrontlineCombatV64Review;
    friend struct FFrontlineTrenchCombatV69Review;
    friend struct FFrontlineContactV835Review;
    friend struct FFrontlineGameplayReviewV75;
    friend struct FFrontlineMoraleV76Access;
    friend struct FFrontlineReviewV76;
    friend struct FFrontlineArmourV63Access;
    friend struct FFrontlineArmourConfigurationV63Review;
    friend struct FFrontlineReviewV63;
    friend struct FFrontlineReviewV64;
    friend struct FFrontlineReviewV65;
    friend struct FFrontlineReviewV49;
    friend struct FFrontlineReportReviewV839;
    friend struct FFrontlineBalanceV50Review;
    friend struct FFrontlineBalanceV52Review;
    friend struct FFrontlineChoiceTargetsV78Review;
    friend struct FFrontlineReviewV50;
    friend struct FFrontlineReviewV51;
    friend struct FFrontlineReviewV52;
    friend struct FFrontlineReviewV53;
    friend struct FFrontlineCombatV35Review;
    friend struct FFrontlineMoraleV36Review;
    friend struct FFrontlineSpecialistsV41Review;
    friend struct FFrontlineMotionV40Access;
    friend struct FFrontlineMGReviewV36;
    friend class AFrontlineTank;
    friend struct FFrontlineReviewV32;
    FFrontlineArmyProgress ArmyProgress[2];
    FString ProgressionFile,ProgressionSaveStatus;
    TArray<FString> RewardedEncounterIds;
    FGuid EncounterId;
    int32 LastResearchReward=0;
    int32 ResearchBeforeOutcomeV828=INDEX_NONE;
    bool bEncounterRewarded=false,bProgressionReadOnly=false,bProgressionVolatile=false;
    void LoadProgression(bool bKeepAssignedFile=false);
    FFrontlineArmyProgress& ActiveResearchMutable();
    bool SaveProgression();
    void AwardEncounterResearch(int32 WinningTeam);
    float RifleAccuracyScale(int32 Team) const;
    float MachinegunPressureScale(int32 Team) const;
    float GrenadeHitChanceScale(int32 Team) const;
    int32 NextSquadId=1,EnemyRecruitStep=0;
    friend struct FFrontlineFireControlV831Review;
    float EnemyThinkAt=0,EnemyAssaultAt=10;
    float EnemyWaveStartedAt=0;
    int32 EnemyWaveRecruitCount=0;
    int32 EnemyCounterattackTrench=INDEX_NONE;
    float EnemyCounterattackUntil=0;
    float TrenchCoverValue(int32 Index) const;
    TArray<FFrontlineCampaignProgress> CampaignRuns;
    FGuid ActiveCampaignId, BoundCampaignId, BoundCampaignEncounterId;
    int32 BoundCampaignMission=INDEX_NONE;
    int32 BoundAttachmentStageV53=INDEX_NONE;
    friend struct FFrontlineUnlocksV53Access;
    friend struct FFrontlineUltimateArchiveV8215Access;
    friend struct FFrontlineNationalArchiveAccessV87;
    friend struct FFrontlineCombatV8217Review;
    friend struct FFrontlineCombatV8218Review;
    friend struct FFrontlineTimeBudgetV8219Review;
    friend struct FFrontlineFieldSitesV59Access;
    friend struct FFrontlineArmourObservationV8219Review;
    friend struct FFrontlineAirSupportV8220Review;
    friend struct FFrontlineArmourKillTallyV8220Review;
    friend struct FFrontlineWreckVisualV8220Review;
    bool bCampaignMissionAdvanced=false;
    float CampaignHoldElapsed=0.f;
    void UpdateCampaignObjective(float DT);
    float EnemyRecruitAt=3,EnemyFireAssessAt=8,EnemyFireExecuteAt=0;
    EFrontlineRole EnemyPlannedRole=EFrontlineRole::Rifle;
    bool bEnemyRoleCommitted=false,bEnemyFirePending=false,bEnemyPendingMortar=false;
    FVector EnemyObservedFireTarget=FVector::ZeroVector;
    FRandomStream EnemyDecisionRandom;
    bool CanEnemyObserve(const AWarfareFrontlineSoldier* Unit) const;
    FRandomStream Random;
    void ResetEncounter();
    friend void StartFrontlineV34Review(AWarfareFrontlineController* Controller);
    friend void StartFrontlineV23FieldReview(AWarfareFrontlineController* Controller);
    friend void StartFrontlineV27RenderReview(AWarfareFrontlineController* Controller);
    friend void StartFrontlineV28Review(AWarfareFrontlineController* Controller);
    friend void StartFrontlineV29Review(AWarfareFrontlineController* Controller);
    friend TSharedRef<FJsonObject> RunFrontlineV28Checks(AWarfareFrontlineGameMode* Battle);
    friend TSharedRef<FJsonObject> RunFrontlineV26CombatChecks(AWarfareFrontlineGameMode* Battle);
    friend TSharedRef<FJsonObject> RunFrontlineV27CampaignChecks(AWarfareFrontlineGameMode* Battle);
    bool SpawnSquad(int32 Team,EFrontlineRole InRole,int32 InitialTrench=INDEX_NONE);
    void UpdateSquads();
    void ThinkEnemy();
    void UpdateSectionOrders();
    void ResolveStrike(FFrontlineStrike& Strike);
    void TickCommittedSupportV8217();
    bool bResolvingCommittedSupportV8217=false;
    void CaptureTrench(int32 Index,int32 Team);
    void FinishBattle(int32 WinningTeam,const FString& Reason);
    int32 FirstAvailableTrench(int32 Team) const;
    int32 FreeSlot(int32 Trench,int32 Team,int32 IgnoreSquad=INDEX_NONE) const;
};

UCLASS()
class WARFAREFRONT_API AWarfareFrontlineController : public APlayerController
{
    GENERATED_BODY()
public:
    AWarfareFrontlineController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    int32 SelectedTrench=INDEX_NONE;
    bool bTargetingArtillery=false,bPreviewValid=false;
    int32 SupportTargetMode=0;
    FVector ArtilleryPreview=FVector::ZeroVector;
    UPROPERTY(Transient) TObjectPtr<ACameraActor> BattleCamera;
    void BeginBattle();
    void RightClickOrdersV63();
    void RecruitRifle();
    void RecruitLewis();
    void RecruitMills();
    void RecruitMarksman();
    void RecruitOfficer();
    void ReleaseSelected();
    void ToggleArtillery();
    void ToggleSmoke();
    void ToggleRally();
    void ToggleMortar();
    void ToggleMenu();
    void MenuBackV61();
    void MenuForwardV61();
    void ToggleHelp();
    void ToggleSettings();
    void TogglePerformanceMonitor();
    void ToggleTechnology();
    void ToggleAchievementsV76();
    void ConfirmSupportAtWorldPoint(const FVector& Point);
    void ResetCamera();
    void TogglePause();
    void ToggleSpeed();
    void Restart();
    void SelectTrench(int32 Trench);
    void CancelTargeting();
    void RunV21CameraChecks(TFunctionRef<void(const FString&,bool)> Check);
private:
    FVector CameraFocus=FVector(-1300,0,0);
    friend struct FFrontlineUIV8447Review;
    friend struct FFrontlineWreckVisualV8220Review;
    // Isolated editor capture changes camera targets only, never actor routes.
    friend struct FRRUNaturalReviewV82::FScheduler;
    friend struct FFrontlineReviewV32;
    friend struct FFrontlinePromoV49;
    TWeakObjectPtr<AActor> DistantMatte;
    float CameraWidth=6000,TargetCameraWidth=6000,CameraPanVelocity=0,CameraVerticalVelocity=0;
    void ClickAtCursor();
    void ZoomIn();
    void ZoomOut();
    bool GroundAtCursor(FVector& Out) const;
};

UCLASS()
class WARFAREFRONT_API AWarfareFrontlineHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    bool IsPointerOverInterface() const;
    int32 GetPresentationPage() const;
};
