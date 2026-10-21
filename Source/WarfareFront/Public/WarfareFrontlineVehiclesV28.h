#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineCombatRecordV65.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "WarfareFrontlineVehiclesV28.generated.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class AWarfareFrontlineController;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;
class FJsonObject;
struct FCollisionQueryParams;
// Append new variants: the first four IDs are retained for existing campaigns.
enum class EFrontlineVehicle : uint8 { MarkIVMale, Whippet, A7V, BeuteMarkIV, MarkIVFemale, BeuteMarkIVFemale, MarkVMale, MarkIVPioneer, A7VGretchen, BeutePioneer, MarkVStar, Austin1918, RailConvoy, Count };
enum class EFrontlineTankModule : uint8 { Hull, LeftTrack, RightTrack, Machinegun };
struct FFrontlineVehicleEntry
{
    EFrontlineVehicle Type=EFrontlineVehicle::MarkIVMale;
    const TCHAR* Name=TEXT("");
    const TCHAR* Description=TEXT("");
    const TCHAR* AssetKey=TEXT("");
    float Cost=0, Cooldown=0, SpeedCm=0, Hull=0, RangeCm=0;
    int32 CannonCount=0,MachinegunCount=0;
};

/** Fixed superstructures and limited-traverse sponsons, never a modern turret. */
UCLASS()
class WARFAREFRONT_API AFrontlineTank : public AActor
{
    GENERATED_BODY()
public:
    AFrontlineTank();
    bool Configure(AWarfareFrontlineGameMode* InBattle,EFrontlineVehicle InType,int32 InTeam,int32 InSerial);
    bool ConfigureRailV32();
    bool IsRailConvoyV32() const { return bRailConvoyV32; }
    bool IsWheeledV8218() const;
    int32 RailPassengersV75() const;
    bool RailHasUnloadedV75() const { return bRailUnloadedV75; }
    float RailCarIntegrityV837(int32 Car) const { return Car>=0&&Car<5?RailCarIntegrityV837_[Car]:0.f; }
    float RailEngineIntegrityV837() const { return RailEngineIntegrityV837_; }
    bool RailCannonOperationalV837(int32 Mount) const;
    void ReceiveRailContactV837(const FVector& WorldImpact,float Energy,const FFrontlineKillSourceV65& SourceV65={});
    void Simulate(float Seconds);
    void ReceiveModuleHit(EFrontlineTankModule Module,float Energy,int32 GunIndex=0,const FFrontlineKillSourceV65& SourceV65={});
    void ReceiveImpact(const FVector& WorldImpact,float Energy,bool bExplosive,const FFrontlineKillSourceV65& SourceV65={},float ModulePressureScaleV844=1.f);
    bool IsOperational() const { return HullIntegrity>0 && !IsActorBeingDestroyed(); }
    bool IsImmobilized() const { return TrackIntegrity[0]<=0 || TrackIntegrity[1]<=0; }
    bool HasEnteredField() const;
    float RearProgressV49() const;
    bool HasClearedEnemyEndV49() const;
    float GetHullIntegrity() const { return HullIntegrity; }
    float GetMaximumHullIntegrity() const { return MaximumHullIntegrity; }
    float GetTrackTravelPhase() const { return TrackOffset; }
    float GetTrackIntegrity(int32 Side) const { return TrackIntegrity[FMath::Clamp(Side,0,1)]; }
    bool CanRepairTrackV60(int32 Side) const;
    bool RepairTrackV60(int32 Side);
    int32 RepairableMachinegunV63() const;
    bool RepairMachinegunV63(int32 Index);
    void SetFieldRepairHoldV63(bool Hold);
    bool IsHeldForFieldRepairV63() const;
    bool ReadCooperativeOpeningGuideV86(const AWarfareFrontlineSoldier* Unit,const FVector& Goal,FVector& Guide) const;
    void CancelCooperativeOpeningGuideV86(const AWarfareFrontlineSoldier* Unit,const FVector& Goal);
    bool HasInfantryRecoveryContactV86(const AWarfareFrontlineSoldier* Unit) const;
    bool IsInfantryRecoveryStationClearV86(const AWarfareFrontlineSoldier* Unit,const FVector& Station) const;
    float GetMachinegunIntegrity(int32 Index) const { return GunIntegrity.IsValidIndex(Index)?GunIntegrity[Index]:0; }
    int32 GetMachinegunCount() const { return GunIntegrity.Num(); }
    bool AreAssetsReady() const;
    // Exact receiver-only German host adapter; false outside its boxed review.
    bool UsesNationalGroundContactV82() const { return bNationalGermanGroundV82; }
    bool OccupiesNationalNodeV82(const FVector& Centre,float Radius) const;
    TSharedPtr<FJsonObject> DescribeNationalGroundContactV82() const;
    // Readback of the latest production subsystem step; it never advances it.
    TSharedRef<FJsonObject> DescribeMobilityV839() const;
    FVector AimPoint() const { return GetActorLocation()+FVector(0,0,160); }
    FVector ObservedArmourAimPointV837(const FVector& Observer) const;
    FVector GrenadeAimPointFor(const FVector& ShooterFeet) const;
    bool ContainsHullPointV75(const FVector& Point,float Margin=12.f) const;
    // One physical hull envelope for reciprocal infantry/vehicle contact.
    FTransform GroundPoseForContactV8218(const FVector& Position,float Yaw,float MotionSeconds=0.f,const FTransform* FromPose=nullptr) const;
    bool BlocksInfantryAtV8218(const FVector& Feet,const FVector& Position,float Yaw,float Radius=34.f,float MotionSeconds=0.f,const AWarfareFrontlineSoldier* Unit=nullptr,const FTransform* FromPose=nullptr,const FTransform* SupportedPose=nullptr) const;
    bool RelievesExistingInfantryContactV842(const FVector& Feet,const FVector& Position,float Yaw,float Radius=34.f,float MotionSeconds=0.f,const AWarfareFrontlineSoldier* Unit=nullptr,const FTransform* FromPose=nullptr,const FTransform* SupportedPose=nullptr) const;
    float InfantrySweepClearanceV8218(const FVector& From,const FVector& To,float Radius=34.f,const AWarfareFrontlineSoldier* Unit=nullptr,const FTransform* PreviewPose=nullptr) const;
    bool InfantryYieldPointV8218(const AWarfareFrontlineSoldier* Unit,FVector& Destination) const;
    bool IsInfantryYieldGoalClearV84411(const AWarfareFrontlineSoldier* Unit,const FVector& Destination) const;
    bool IsInfantryYieldStationSafeV86(const AWarfareFrontlineSoldier* Unit,const FVector& Destination) const;
    bool IsInfantryYieldNextStepClearV86(const AWarfareFrontlineSoldier* Unit,const FVector& Destination) const;
    bool HasInfantryYieldRecoveryV84411(const AWarfareFrontlineSoldier* Unit) const;
    FVector WeaponMuzzle(int32 Index,bool bCannon) const;
    EFrontlineVehicle Type=EFrontlineVehicle::MarkIVMale;
    int32 Team=0,Serial=0,CannonShots=0,MachinegunShots=0;
    float DistanceTravelled=0;
    FFrontlineKillTallyV65 KillsV65;
private:
    friend struct FFrontlineVisualV85Access;
    friend struct FFrontlineArmourApproachReviewV85;
    friend struct FFrontlineArmourContactReviewV84414;
    friend struct FFrontlineChallengeFeasibilityV84415;
    friend struct FFrontlineMuzzleFlashV839Review;
    friend class AFrontlineArmourSurfaceV837;
    friend struct FFrontlineRailGameplayV837Review;
    friend struct FFrontlineArmourArtV838Review;
    friend struct FFrontlineArmourV836Review;
    friend struct FFrontlineArmourWreckV8220Access;
    friend struct FFrontlineAirSupportV8220Review;
    friend struct FFrontlineArmourKillTallyV8220Review;
    friend struct FFrontlineWreckVisualV8220Review;
    friend struct FFRRUArmourAccessV82;
    friend struct FFrontlineRulesV8215Review;
    friend struct FFrontlineCombatV8217Review;
    friend struct FFrontlineCombatV8218Review;
    friend struct FFrontlineTimeBudgetV8219Review;
    friend struct FFrontlineArmourObservationV8219Review;
    friend struct FFrontlineGameplayReviewV75;
    friend struct FFrontlineCombatRecordReviewV65;
    friend struct FFrontlineCombatV63Review;
    friend struct FFrontlineCombatV64Review;
    friend struct FFrontlineArmourConfigurationV63Review;
    friend struct FFrontlineMovementV35Review;
    friend struct FFrontlineInfantryV45Review;
    friend struct FFrontlineGameplayV43Review;
    friend void StartFrontlineV28Review(AWarfareFrontlineController* Controller);
    friend class UFrontlineVehicleSubsystem;
    friend struct FFrontlineVehiclesV37Review;
    friend struct FFrontlineReviewV49;
    friend struct FFrontlineReviewV50;
    void UpdateTracks(float Distance,float TurnRadians=0.f);
    void FitGroundV35(const FVector& Position,float Seconds);
    bool CommitNationalGroundV82(const FVector& Position,float Heading,float Distance,bool Initial=false,bool Reverse=false);
    float AdvanceV37(float Distance,float Seconds);
    bool StraightInfantryPassWithinFieldV86(const AWarfareFrontlineSoldier* Unit,FVector* End=nullptr) const;
    void RefreshInfantryPassCertificateV86();
    float AdvanceArmourClearanceV851(float Distance,float Seconds);
    bool PrepareStaticBodyDetourV85(float Speed,float Seconds,AWarfareFrontlineSoldier* PassingOwner=nullptr);
    float AdvanceStaticBodyDetourV85(float Distance,float Seconds);
    bool StaticBodyDetourPoseClearV85(const FVector& Position,float Yaw,float Seconds,const FTransform* FromPose=nullptr,FTransform* OutSupportedPose=nullptr) const;
    float AdvanceCombatArcV8311(const FVector& Target,float Distance,float Seconds);
    float FootprintHalfLengthV37() const;
    float FootprintHalfWidthV8311() const;
    float SteeringRateV8311(float Speed,bool Stationary) const;
    float ArmourOverlapV8311(const FVector& Position,float Yaw,const AFrontlineTank* Other,float Margin=0.f) const;
    bool CanOccupyV48(const FVector& Position,float Yaw,float MotionSeconds=0.f,const FTransform* FromPose=nullptr,const FTransform* SupportedPose=nullptr) const;
    bool FaceArmourV48(const FVector& Target,float Seconds,bool CannonOnly=false,const AActor* TargetActor=nullptr);
    bool MountedHeadingV8448(const FVector& Target,float& OutHeading,bool CannonOnly,const AActor* TargetActor=nullptr) const;
    bool ArmourSightV48(const AFrontlineTank* Target,FVector& OutPoint) const;
    AFrontlineTank* SelectArmourContactV84414(float& OutDistance,FVector& OutPoint) const;
    bool ArmourFiringSolutionV8219(const AFrontlineTank* Target,int32 Mount,FVector& OutPoint,bool ForAiming=false,float* OutMuzzleDistance=nullptr) const;
    bool InfantryFiringSolutionV828(const AWarfareFrontlineSoldier* Target,int32 Mount,bool Cannon,FVector& OutPoint,const FCollisionQueryParams* Prepared=nullptr,bool ForAiming=false) const;
    void ApplySurfaceV37(UStaticMeshComponent* Component,float Dust,float Rust);
    void AddMarkingsV37();
    void UpdateEffects(float Seconds);
    void PresentMuzzleFlashV839(int32 Index);
    void UpdateShells(float Seconds);
    void ResolveShell(const FVector& Impact);
    void FireCannon(int32 Index,const FVector& Position,AFrontlineTank* TankTarget);
    void FireMachinegun(int32 Index,AWarfareFrontlineSoldier* Target);
    void Wreck();
    void DeployRailPassengersV75();
    void UpdateRailDisembarkationV837(float Seconds,bool CanBegin);
    bool RailEgressV837(int32 Car,int32 Member,FVector& Feet) const;
    UPROPERTY(Transient) TObjectPtr<AWarfareFrontlineGameMode> Battle;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TrackLinks;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Guns;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Cannons;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Flashes;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Smoke;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RailWheelsV33;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RailRodsV33;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RailBogieWheelsV837;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RailDoorsV837;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RailCupolasV837;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RoadWheelsV8218;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RoadTurretsV8218;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> MarkingsV37;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> BodyMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> SmokeMaterials;
    TArray<float> GunIntegrity,GunClock,FlashAge;
    TArray<int32> BurstRemaining;
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> GunTargetsV828;
    TArray<FVector> GunMounts,CannonMounts;
    TArray<float> GunBaseYaw,CannonBaseYaw;
    struct FShellInFlight
    {
        FVector From=FVector::ZeroVector,Impact=FVector::ZeroVector;
        float Age=0,Duration=.2f;
        TWeakObjectPtr<UStaticMeshComponent> Visual;
    };
    TArray<FShellInFlight> Shells;
    TArray<FVector2D> TrackPath;
    TArray<float> TrackSegments;
    TArray<FTransform> TrackTransformsV33;
    float TrackIntegrity[2]={180,180},CannonClock[2]={4,6},HullIntegrity=0,MaximumHullIntegrity=0,PathLength=0,TrackOffset=0,TrackUpdate=0;
    float PreferredLaneV37=0,DesiredLaneV37=0,RouteDecisionAtV37=0,HeadingYawV37=0,TrackDistanceV37=0;
    float TrackSideOffsetV8311[2]={0.f,0.f},TrackTurnV8311=0.f,WheelSteerV8311=0.f;
    float RoadCombatTurnSignV8311=0.f;
    TWeakObjectPtr<AWarfareFrontlineSoldier> RoadCombatTargetV8311;
    float FailedGermanLaneV82=0,FailedGermanLaneUntilHostV82=-1;
    float RouteStallSecondsV837=0,BlockedLaneV837=0,BlockedLaneUntilV837=-1;
    struct FStaticBodyDetourPoseV85 { FVector Position=FVector::ZeroVector;float Heading=0;bool Reverse=false; };
    TArray<FStaticBodyDetourPoseV85> StaticBodyDetourV85;
    int32 StaticBodyDetourIndexV85=0;
    float StaticBodyDetourBlockedV85=0,StaticBodyDetourRetryAtV85=-1;
    TWeakObjectPtr<AWarfareFrontlineSoldier> StaticBodyDetourOwnerV85;
    FVector StaticBodyDetourOwnerFeetV85=FVector::ZeroVector;
    int32 StaticBodyOpeningTrenchV86=INDEX_NONE;
    struct FCooperativeOpeningV86 { FVector From=FVector::ZeroVector,Leg=FVector::ZeroVector,Goal=FVector::ZeroVector,Guided=FVector::ZeroVector;int32 Trench=INDEX_NONE;bool Acknowledged=false;FGuid Encounter;TArray<FVector> FloorPath; };
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FCooperativeOpeningV86> CooperativeOpeningsV86;
    struct FInfantryRecoveryWitnessV86 { FTransform From=FTransform::Identity,Supported=FTransform::Identity;FGuid Encounter;float At=-1,Seconds=0;bool Valid=false;FVector2D LocalStep=FVector2D::ZeroVector;float Turn=0; };
    mutable FInfantryRecoveryWitnessV86 InfantryRecoveryWitnessV86;
    bool RefreshInfantryRecoveryWitnessV86() const;
    TSharedPtr<FJsonObject> StaticBodyDetourAttemptV86;
    float MobilityRequestedV839=0,MobilityMovedV839=0,MobilityStepV839=0,MobilityHostTimeV839=-1;
    const TCHAR* MobilityGateV839=TEXT("not_simulated");
    bool bAcquisitionFeasibleV8448=false,bMobilityFiringReadyV8448=false,bMobilityTraversingV8448=false;
    bool bRollingAcquisitionV8448=false;
    float RollingAcquisitionYawV8448=0.f;
    bool bArmourClearanceV851=false,bArmourClearanceStepV851=false;
    TWeakObjectPtr<AFrontlineTank> ArmourClearanceTargetV851;
    float ArmourClearanceBackingV851=0.f;
    int32 ArmourClearanceLeaseOwnerV851=INDEX_NONE;
    TWeakObjectPtr<AFrontlineTank> ArmourClearanceBudgetTargetV851;
    bool bArmourClearanceReverseV851=false;
    TWeakObjectPtr<AWarfareFrontlineSoldier> InfantryContactV8448;
    // A live friendly hull can leave too little room for the first turn.
    // Back only through a checked short corridor, then retain its bypass lane.
    uint8 BypassPhaseV839=0;
    float BypassLaneV839=0,BypassRemainingV839=0,BypassBlockedSecondsV839=0;
    float BypassBackingTotalV839=0,BypassBackingLimitV839=0;
    bool bBackingStepV839=false;
    bool bBypassInfantryPassingV84412=false;
    FVector BypassPassFeetV86=FVector::ZeroVector;
    float BypassPassHeadingV86=0.f;
    TWeakObjectPtr<AFrontlineTank> BypassNeighbourV839;
    TWeakObjectPtr<AWarfareFrontlineSoldier> BypassInfantryV84411;
    FVector BypassInfantryStartV84411=FVector::ZeroVector;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FVector> BypassInfantryAttemptsV84411;
    // Native A7V recovery owns its lane until the first actual forward commit.
    uint8 GermanRecoveryPhaseV826=0;
    float GermanTerrainStallSecondsV826=0,GermanRecoveryRemainingCmV826=0;
    float GermanRecoveryLaneV826=0,GermanRecoveryBlockedSecondsV826=0,GermanRecoveryReadyAtV826=-1;
    struct FGermanAcceptedPoseV826 {FVector Position;float Heading=0;};
    // Value-only history of successful contact plans; no live actor or render data.
    TArray<FGermanAcceptedPoseV826> GermanAcceptedRouteV826;
    TSharedPtr<FJsonObject> GermanCannonReadbackV82;
    TWeakObjectPtr<AFrontlineTank> ArmourContactV48;
    FVector LastArmourPointV48=FVector::ZeroVector;
    float ArmourContactAgeV48=2.f;
    bool bArmourContactUsableV837=false;
    float SmokeAge=0,WreckAge=0;
    bool bEntered=false;
    bool bNationalGermanGroundV82=false;
    uint8 RepairedTracksV60=0;
    bool bMachinegunRepairedV63=false;
    float FieldRepairHoldUntilV63=-1.f;
    bool bRailConvoyV32=false;
    bool bRailUnloadedV75=false;
    float RailDisembarkClockV75=0.f;
    uint8 RailSectionMaskV837=0;
    int32 RailSquadIdsV837[2]={INDEX_NONE,INDEX_NONE};
    int32 RailReleasedV837[2]={0,0};
    float RailDoorPhaseV837=0, RailReleaseClockV837=0;
    // Front gun, forward troop car, tender, rear troop car and rear gun.
    float RailCarIntegrityV837_[5]={260,220,190,220,260};
    float RailEngineIntegrityV837_=340;
    FFrontlineKillSourceV65 RailDestructionSourceV837;
    bool bArmorReported=false;
    bool bEnemyArmorReported=false;
    float NextVoiceCheck=0;
    FRandomStream Random{281917};
};

/** Separate vehicle roster; never changes five-slot infantry enums or saves. */
UCLASS()
class WARFAREFRONT_API UFrontlineVehicleSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    static UFrontlineVehicleSubsystem* Get(UWorld* World);
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void Deinitialize() override;
    void ResetBattle() { ClearEncounter(); }
    int32 GetDeploymentCountV76(int32 Team) const { return Team>=0&&Team<2?DeploymentCountV37[Team]:0; }
    TArray<FFrontlineVehicleEntry> GetRoster(int32 Team=0) const;
    TArray<FFrontlineVehicleEntry> GetEquippedRoster(int32 Team=0) const;
    static const FFrontlineVehicleEntry& Definition(EFrontlineVehicle Type);
    UTexture2D* GetPortrait(EFrontlineVehicle Type) const;
    bool IsUnlocked(EFrontlineVehicle Type,int32 Team=0) const;
    // Eligibility excludes cooldown and menu pause, so one-live cards stay
    // grey while their independent common readiness clock keeps advancing.
    bool IsDeploymentEligibleV837(EFrontlineVehicle Type,int32 Team=0) const;
    int32 UnlockMission(EFrontlineVehicle Type,int32 Team=0) const;
    FString RecruitBlockReason(EFrontlineVehicle Type,int32 Team=0) const;
    bool Recruit(EFrontlineVehicle Type,int32 Team=0);
    AFrontlineTank* SpawnScenarioRailV32(int32 Team);
    static constexpr float RailCooldownSecondsV75=145.f;
    bool RailAvailableV75(int32 Team=0) const;
    FString RailBlockReasonV75(int32 Team=0) const;
    bool RequestRailV75(int32 Team=0);
    float RailCooldownFractionV75(int32 Team=0) const;
    float CooldownFraction(EFrontlineVehicle Type,int32 Team=0) const;
    const TArray<TObjectPtr<AFrontlineTank>>& GetVehicles() const { return Vehicles; }
    void ApplyBlast(const FVector& Position,float Radius,float Power,int32 AttackingTeam,const FFrontlineKillSourceV65& SourceV65={});
    void ApplyGrenadeBlast(const FVector& Position,float Radius,int32 AttackingTeam,const FFrontlineKillSourceV65& SourceV65={});
    void ApplyAntiTankRifleGrenadeV836(const FVector& Position,float Radius,int32 AttackingTeam,const FFrontlineKillSourceV65& SourceV65={});
    AFrontlineTank* AcquireGrenadeTarget(const AWarfareFrontlineSoldier* Shooter,float MaxRange,FVector& OutAimPoint) const;
    void ResetReadinessForDebug(int32 Team=0);
    AFrontlineTank* AcquireInfantryTarget(const AWarfareFrontlineSoldier* Shooter) const;
    bool HasInfantryFiringLineV828(const AWarfareFrontlineSoldier* Shooter,const AFrontlineTank* Target) const;
    void ApplySmallArmsHit(AFrontlineTank* Target,const FVector& Impact,bool bAutomatic,int32 AttackingTeam);
    TSharedRef<FJsonObject> RunFocusedChecks();
    TSharedRef<FJsonObject> RunVehicleChecksV37();
    TSharedRef<FJsonObject> RunArmourChecksV40();
    TSharedRef<FJsonObject> RunMobilityChecksV41();
    TSharedRef<FJsonObject> RunMobilityChecksV842();
    TSharedRef<FJsonObject> RunMobilityChecksV85();
    TSharedRef<FJsonObject> RunTrenchMobilityChecksV86();
    TSharedRef<FJsonObject> RunMobilityChecksV8445();
    TSharedRef<FJsonObject> RunMobilityChecksV8448();
    TSharedRef<FJsonObject> RunMobilityChecksV84411();
    TSharedRef<FJsonObject> RunMovementChecksV84413();
    TSharedRef<FJsonObject> RunContactChecksV84414();
    TSharedRef<FJsonObject> RunSupportArmourBalanceChecksV844();
    TSharedRef<FJsonObject> RunArmourCombatChecksV48();
    void RunPlatformChecksV8218(TFunctionRef<void(const FString&,bool)> Check);
    void RunConfigurationChecksV63(TFunctionRef<void(const FString&,bool)> Check);
    TArray<AFrontlineTank*> SpawnVehicleArtReviewV37();
private:
    friend struct FFrontlineArmourContactReviewV84414;
    friend struct FFrontlineArmourApproachReviewV85;
    friend struct FFrontlineEnemyChecksV84415;
    friend struct FFrontlineChallengeFeasibilityV84415;
    friend struct FFrontlineRailGameplayV837Review;
    friend struct FFrontlineArmourArtV838Review;
    friend struct FFrontlineAirSupportV8220Review;
    friend struct FFrontlineArmourV836Review;
    friend struct FFrontlineArmourKillTallyV8220Review;
    friend struct FFrontlineWreckVisualV8220Review;
    friend struct FFRRUArmourAccessV82;
    friend struct FFrontlineCombatV8217Review;
    friend struct FFrontlineCombatV8218Review;
    friend struct FFrontlineTimeBudgetV8219Review;
    friend struct FFrontlineArmourObservationV8219Review;
    friend struct FFrontlineRulesV8215Review;
    friend struct FFrontlineGameplayReviewV75;
    friend struct FFrontlineCombatRecordReviewV65;
    friend struct FFrontlineCombatV63Review;
    friend struct FFrontlineCombatV64Review;
    friend struct FFrontlineArmourConfigurationV63Review;
    friend TSharedRef<FJsonObject> RunFrontlineV28SupportChecks(AWarfareFrontlineGameMode* Battle);
    friend struct FFrontlineInfantryV45Review;
    friend struct FFrontlineMovementV35Review;
    friend struct FFrontlinePerformanceV33Access;
    friend struct FFrontlineStanceV8216Review;
    friend struct FFrontlineFireControlV831Review;
    friend struct FFrontlineEngineerMineV833Review;
    friend struct FFrontlineReviewV49;
    friend struct FFrontlineReviewV50;
    friend struct FFrontlineGameplayV43Review;
    friend struct FFrontlineVehiclesV37Review;
    AWarfareFrontlineGameMode* Battle() const;
    void ClearEncounter();
    AFrontlineTank* Spawn(EFrontlineVehicle Type,int32 Team,bool bAtEntry);
    UPROPERTY(Transient) TArray<TObjectPtr<AFrontlineTank>> Vehicles;
    FGuid LastEncounter;
    float ReadyAt[2][int32(EFrontlineVehicle::Count)]={{0}},NextEnemyAt=70;
    float ArmourCycleSecondsV40[2]={50,50};
    float ArmourCycleDurationV84413[2][int32(EFrontlineVehicle::Count)]={{0}};
    friend struct FFrontlineReadinessChecksV84413;
    int32 NextSerial=0;
    int32 DeploymentCountV37[2]={0,0};
    float RailReadyAtV75[2]={42.f,42.f};
};
