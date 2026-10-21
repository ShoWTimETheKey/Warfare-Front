// Bounded editor-only natural controller review. Campaign prerequisites are
// explicitly synthetic; no encounter clock, actor position or cooldown is set.
#if WITH_EDITOR
#include "FRRU/WarfareFRRUCampaignInternalV82.h"
#include "FRRU/Native/SceneRuntime.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineBlockedFireProbeV87.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineMovementV28.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlinePacingV24.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Serialization/Archive.h"
namespace FRRUNaturalReviewV82
{
enum class EProbe : uint8 { Rifle, Hotchkiss, FT, Reserve, Train, Austin, Schilt, VB, CA1, Saint, Ordinary, FrenchPlan };
struct FScenario {const TCHAR* Name;const TCHAR* Mission;EProbe Probe;bool WholeOperation=false;};
const FScenario Scenarios[]={
    {TEXT("mort-rifle"),TEXT("fr.mort_homme"),EProbe::Rifle},
    {TEXT("vaux-rifle"),TEXT("fr.vaux"),EProbe::Rifle},
    {TEXT("naroch-rifle"),TEXT("ru.naroch"),EProbe::Rifle},
    {TEXT("mort-hotchkiss"),TEXT("fr.mort_homme"),EProbe::Hotchkiss},
    {TEXT("matz-ft"),TEXT("fr.matz"),EProbe::FT},
    {TEXT("galicia-reserve"),TEXT("ru.galicia"),EProbe::Reserve},
    {TEXT("czernowitz-train"),TEXT("ru.czernowitz"),EProbe::Train},
    {TEXT("riga-austin"),TEXT("ru.riga"),EProbe::Austin},
    {TEXT("late-schilt"),TEXT("fr.sommepy"),EProbe::Schilt},
    {TEXT("chemin-vb"),TEXT("fr.chemin_dames"),EProbe::VB},
    {TEXT("chemin-plan"),TEXT("fr.chemin_dames"),EProbe::FrenchPlan},
    {TEXT("malmaison-ca1"),TEXT("fr.malmaison"),EProbe::CA1},
    {TEXT("malmaison-saint"),TEXT("fr.malmaison"),EProbe::Saint},
    {TEXT("mort-ordinary"),TEXT("fr.mort_homme"),EProbe::Ordinary},
    {TEXT("galicia-ordinary"),TEXT("ru.galicia"),EProbe::Ordinary},
    {TEXT("malmaison-ordinary"),TEXT("fr.malmaison"),EProbe::Ordinary},
    {TEXT("riga-ordinary"),TEXT("ru.riga"),EProbe::Ordinary},
    {TEXT("mort-full"),TEXT("fr.mort_homme"),EProbe::Ordinary,true},
    {TEXT("vaux-full"),TEXT("fr.vaux"),EProbe::Ordinary,true},
    {TEXT("douaumont-full"),TEXT("fr.douaumont"),EProbe::Ordinary,true},
    {TEXT("malmaison-full"),TEXT("fr.malmaison"),EProbe::Ordinary,true},
    {TEXT("matz-full"),TEXT("fr.matz"),EProbe::Ordinary,true},
    {TEXT("champagne-full"),TEXT("fr.champagne_defence"),EProbe::Ordinary,true},
    {TEXT("soissons-full"),TEXT("fr.soissons"),EProbe::Ordinary,true},
    {TEXT("sommepy-full"),TEXT("fr.sommepy"),EProbe::Ordinary,true},
    {TEXT("kovel-full"),TEXT("ru.kovel"),EProbe::Ordinary,true},
    {TEXT("louvemont-full"),TEXT("fr.louvemont"),EProbe::Ordinary,true},
    {TEXT("chemin-full"),TEXT("fr.chemin_dames"),EProbe::Ordinary,true}};
bool Isolation(FString& User)
{
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))return false;
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
    return User.StartsWith(Project+TEXT("/Saved/FRRU/V82NaturalReview/"),ESearchCase::IgnoreCase);
}
struct FScheduler : TSharedFromThis<FScheduler>
{
    enum class EStep:uint8 { Setup, Cover, WarRoom, Deployment, Introduction, Loading, WarmFrame, Reframed, Recruit, Entry, Pause, Hold, Resume, Specialist, Combat, FinalCapture };
    TWeakObjectPtr<AWarfareFrontlineController> Controller;
    const FScenario* Scenario=nullptr;
    FString User,FirstFailure,FailureClassV87,OrdinaryWithdrawalStageV87;
    EFrontlineDifficulty RequestedDifficultyV87=EFrontlineDifficulty::Veteran;
    FString RequestedDifficultyLabelV87=TEXT("Veteran");
    bool NaturalFastPlayRequestedV87=false,NaturalFastPlayHudClickAttemptedV87=false,NaturalFastPlayHudClickAppliedV87=false;
    float NaturalFastPlayActualSpeedV87=1.f,NaturalFastPlayClickedAtHostV87=-1.f;
    EStep Step=EStep::Setup;
    double Started=FPlatformTime::Seconds(),StepAt=Started,NextSample=0,NextOrders=0;
    double WallFrameMs=0,DeadlineSeconds=720;FDateTime WallDeadlineUtc;
    FString ReportName=TEXT("natural-combat.json"),ReportRevision=TEXT("v82.frru.natural.7");
    bool FixtureSynthetic=true;TFunction<void(FJsonObject&)> ReportDetail;
    TSharedPtr<FJsonObject> BlockedFireEvidenceV87;
    float PauseElapsed=0,EntryOrderedAt=0,ReserveHeldAt=-1,ReserveReleasedAt=-1;
    int32 Lead=INDEX_NONE,Second=INDEX_NONE,Observer=INDEX_NONE,Hostile=INDEX_NONE;
    int32 SchiltPioneerV87=INDEX_NONE;
    FString SchiltContactOrderV87;float SchiltContactRetryAtV87=0;
    FString OrdinaryFirstStageV827,OrdinaryFirstTargetV827;
    bool OrdinaryAdvanceSeenV827=false,OrdinaryHoldPreservedV827=false,OrdinaryFirstCompletedV827=false,OrdinaryWithdrawalArrivedV827=false;
    int32 OrdinaryRecruitsV827=0,OrdinaryArtilleryV827=0,OrdinaryRallyV827=0;double OrdinaryRecruitRetryV827=0,OrdinarySupportRetryV827=0;float OrdinaryHoldAtV827=-1;
    bool HoldDepartureProbeV87=false,HoldDepartureCheckpointV87=false,HoldDepartureArrivedV87=false;
    int32 HoldDeparturePhaseV87=0,HoldDepartureLivingV87=0,HoldDepartureEdgeV87=INDEX_NONE;
    FString HoldDepartureSourceV87,HoldDepartureReceiverV87,HoldDepartureTargetV87,HoldDepartureOffsetV87;
    float HoldDepartureReleasedAtV87=-1;
    TSharedPtr<FJsonObject> HoldDepartureEvidenceV87=MakeShared<FJsonObject>();
    int32 VauxSmokeCallsV87=0,VauxMortarCallsV87=0,VauxRecruitDeferralsV87=0,VauxMortarRouteDeclinesV87=0;
    float VauxNextRecruitHostV87=0;
    bool VauxManualHoldIssuedV87=false;
    int32 VauxPublicFireRequestAttemptsV87=0,VauxPublicHoldAttemptsV87=0,VauxDirectSafeFireWindowsV87=0,VauxHeldFireWindowsV87=0;
    TMap<FString,int32> VauxFireRefusalsV87;
    EFrontlineSupport OrdinaryFireWindowSupportV87=EFrontlineSupport::Artillery;
    int32 OrdinaryArtilleryRouteDeclinesV87=0;
    int32 KnownPositionFireAttemptsV87=0,KnownPositionFireAcceptedV87=0,KnownPositionFutureRouteRiskV87=0;
    int32 KnownPositionCurrentFriendlyRefusalsV87=0,KnownPositionNoEnemyPostV87=0;
    // Enabled only by the strict normal public fresh-start lifecycle owner.
    // Short role/natural reviews retain their historical bounded budgets.
    bool NormalCampaignCompletionV87=false;
    int32 NormalCompletionRecruitDeferralsV87=0,NormalCompletionActiveSoftLimitV87=0;
    int32 NormalCompletionLiveSectionsV87=0,NormalCompletionLivingPeopleV87=0,NormalCompletionHomeRelatedV87=0;
    float NormalCompletionLastReadinessHostV87=-1;
    TMap<FString,int32> NormalCompletionReadinessSamplesV87,NormalCompletionDeferralReasonsV87,NormalCompletionCapacityBasisV87;
    TMap<FString,FString> NormalCompletionLastBlockReasonsV87;
    bool OrdinaryFireWindowUsedV87=false;
    FGuid OrdinaryFireWindowEncounterV87;
    int32 OrdinaryFireWindowMissionV87=INDEX_NONE;
    TSet<int32> OrdinaryFireWindowHoldsV87,OrdinaryFireWindowTerminalHoldsV87;
    TMap<uint64,bool> OrdinaryFireWindowRoundsV87;
    TMap<int32,FVector> OrdinaryHoldFeetV827;TSet<int32> OrdinaryWithdrawalIdsV827;
    struct FTrafficWatchV827 {FString Route;int32 Living=0;double Best=0,At=0;};
    TMap<int32,FTrafficWatchV827> OrdinaryTrafficV827;double OrdinaryTrafficSampleV827=-1;
    TMap<int32,FTrafficWatchV827> OrdinaryIngressV87;
    int32 OrdinaryReturnStepV87=0,OrdinaryReturnWonV87=INDEX_NONE,OrdinaryOpeningResearchV87=0;double OrdinaryReturnAtV87=0;
    FFRRUCampaignSummaryV82 OrdinaryReturnRecordV87;bool OrdinaryReturnCardsV87=false;
    FString OrdinaryGuardStageV87,OrdinaryGuardTargetV87;
    TSet<int32> OrdinaryHoldGuardsV87;int32 OrdinaryGuardSamplesV87=0,OrdinaryMaxHoldResidentsV87=0;
    int32 FTObjectiveSection=INDEX_NONE,FTObjectiveRecruits=0;
    TSet<int32> FTObjectiveSections;
    double FTObjectiveRetryAt=0;
    FString FTSmokeApproach;
    float LeadBeforeMove=0;FVector LastLeadFeet=FVector::ZeroVector;
    bool RifleEntry=false,HoldChecked=false,ReserveRegistered=false,ReserveReleased=false;
    bool ClearanceCompleted=false;FString ClearanceNode;
    FString LastObjectiveOrder;double NextObjectiveAttemptAt=0;
    bool SpecialRecruited=false,ObserverRecruited=false,HostileRecruited=false;
    bool GunCarry=false,GunDeploy=false,GunSupported=false,GunPack=false;
    bool GunShots=false,GunReload=false,GunReloadDone=false,FourSalvos=false,Followup=false;
    bool VehicleRolled=false,VehicleShot=false,VehicleSharedCD=false,VehicleSelected=false;
    bool VehicleCannonShot=false,VehicleMachinegunShot=false;
    bool RealStraddler=false,RealHullContest=false,NegativeRecruitChecked=false;
    bool FlameBrace=false,FlameJet=false,FlameRecovery=false,FlameEnemyBurn=false,FlameIgnited=false,FlameSpent=false,FlameEmpty=false;
    bool FlamePaused=false,FlamePauseDone=false;double FlamePauseUntil=0;FVector FlamePauseFeet=FVector::ZeroVector;
    FFrontlineFlameStateV82 PausedFlame;
    int32 FlameShotsObserved=0;float LastFlameStarted=-1,PreviousFlameFuel=12;
    bool VBLoaded=false,VBEmpty=false,VBPairFinite=false,VBProjectile=false,VBRestored=false;
    int32 VBPioneerV87=INDEX_NONE;
    bool VBPioneerRecruitedV87=false,VBPioneerReturnedV87=false,VBContactSeenV87=false;
    FString VBPioneerOrderV87,VBAdvanceOrderV87;
    float VBPioneerRetryAtV87=0,VBAdvanceRetryAtV87=0,VBNoShotSinceV87=-1;
    TSharedPtr<FJsonObject> VBNoShotEvidenceV87;
    int32 VBReloadSamples=0;float VBReloadStartedAt=-1;
    int32 MaximumResolved=0;float ResolvedFourAt=-1;
    TArray<TSharedPtr<FJsonValue>> Checks,Samples,Commands,Captures;
    // One un-serialized frame for the unchanged all-member liveness watch.
    TArray<TSharedPtr<FJsonValue>> OrdinaryTrafficSectionsV87;
    FString PendingCapture,PendingCaptureName;double CaptureRequestedAt=0;int32 CaptureRequestedPage=INDEX_NONE;
    FIntPoint CaptureRequestedViewport=FIntPoint::ZeroValue;
    TSet<FString> CompletedCaptures;
    bool LoadingCaptured=false,LoadingNarrativeCheckedV826=false;
    int32 LoadingClicksBefore=INDEX_NONE;
    bool TheatreCaptured=false;
    bool FTNavigationProofDoneV87=false;FString FTNavigationProofPathV87;
    int32 TerrainBufferStage=0;double TerrainBufferStarted=0;
    int32 VisualCameraStage=0,VisualCloseFramedStageV87=INDEX_NONE;
    double PreviousFTPhase=-1,PreviousFTTravel=-1;bool FTPhaseWrapObserved=false;
    FVector OriginalCameraFocus=FVector::ZeroVector;float OriginalCameraWidth=6000;
    TMap<int32,FVector> PausedFeet,EntryStartingFeet;
    TMap<int32,float> MemberEntryTimes;
    int32 PauseStrikes=0,PauseResolved=0;
    bool Finished=false,FailureDeepSampledV87=false;
    // Review-only state. Every elapsed value is read from the real host.
    int32 RuptureSquad=INDEX_NONE,RuptureBefore=-1,RuptureAt=-1;
    TWeakObjectPtr<AWarfareFrontlineSoldier> RuptureActor;
    float RuptureHitAt=-1,RuptureStableSince=-1,RuptureDeathAt=-1;
    bool EmptyRefusalChecked=false,RuptureHitIssued=false,RuptureRepeatIssued=false,RuptureChecked=false;
    bool SecondReserveReleased=false,SecondReserveEarlyChecked=false,ReserveAxesChecked=false;
    bool FirstReserveEarlyChecked=false,ReserveIdentityChecked=false;
    float SecondReserveReleasedAt=-1;
    int32 ReserveUnitCount=0,ReserveSquadCount=0,ReserveRecruitCount=0;
    bool MaximReloadChecked=false,M95ReloadChecked=false;
    TSet<int32> MaximLoadingActors,M95LoadingActors;
    bool ReserveRidgeVacated=false,ReserveLeadHeld=false,ReserveLeadReleased=false;
    bool ReservePioneerScreenHeld=false,ReservePioneerScreenReleased=false;
    float ReserveTacticalRetryAt=0;
    int32 ReviewVehicleSerial=INDEX_NONE;
    TWeakObjectPtr<AFrontlineTank> ReviewVehicle;
    bool VehiclePoseChecked=false,VehicleTrackChecked=false,VehicleGunChecked=false,FTBlastFootprintCheckedV826=false;
    int32 TrackStage=0,BrokenMGShots=0;float TrackAt=-1,GunDisabledAt=-1,VehicleLastMGAt=-1;
    int32 VehicleLastMGShots=0;FVector TrackStoppedPosition=FVector::ZeroVector;
    bool DisabledGunHasTarget=false;float DisabledGunEligibleAt=-1;
    struct FObservedPassengerWindow
    {
        TWeakObjectPtr<AWarfareFrontlineSoldier> Actor;
        int32 Serial=INDEX_NONE,Squad=INDEX_NONE,Member=INDEX_NONE,Car=INDEX_NONE,Seat=INDEX_NONE,LastStage=INDEX_NONE;
        uint32 Stages=0,MotionStages=0;bool ContactValid=true,PoseValid=true,IdentityValid=true,EverActiveWhileAboard=false,AliveAtFirstField=false;
        FVector PreviousFeet=FVector::ZeroVector;float PreviousAt=-1,FirstAt=-1,FirstFieldAt=-1;
    };
    TMap<int32,FObservedPassengerWindow> NaturalPassengers,WreckPassengers;
    TWeakObjectPtr<AFrontlineTank> RetryTrain;
    int32 RetryTrainSerial=INDEX_NONE,GroundSurvivorPerson=INDEX_NONE,GroundSurvivorShots=0;
    int32 FirstNaturalTrainSerial=INDEX_NONE,TrainContactSquad=INDEX_NONE,TrainContactLeg=0;
    static constexpr int32 TrainContactRecruitLimit=3;
    int32 TrainContactRecruits=0,TrainCombatSerial=INDEX_NONE;
    TSet<int32> TrainContactSquads;
    TWeakObjectPtr<AFrontlineTank> TrainCombatCarrier;
    double TrainContactRetryAt=0;bool TrainContactHeld=false,TrainContactRailHeld=false,TrainContactRidgeHeld=false,TrainContactExhausted=false;
    bool TrainCombatRecruitAttempted=false,TrainCombatFireRecorded=false;
    bool NaturalSixPassengerChecked=false,WreckRetryChecked=false,CollisionFixtureCreated=false,WreckHitIssued=false,WreckDuplicateChecked=false;
    float TrainParkedAt=-1,GroundWaitAt=-1,WreckAt=-1,WreckMoraleLossBefore=0,WreckMoraleLossAfter=0;
    FVector GroundSurvivorFeet=FVector::ZeroVector;
    TArray<TWeakObjectPtr<AActor>> ReceivingBayFixtures;
    TArray<TSharedPtr<FJsonValue>> RequiredWindows;
    bool IsVehicleCase()const
    {return Scenario->Probe==EProbe::FT||Scenario->Probe==EProbe::Austin||Scenario->Probe==EProbe::Train||Scenario->Probe==EProbe::CA1||Scenario->Probe==EProbe::Saint;}
    bool RequiresVehicleCannon()const
    {return Scenario->Probe==EProbe::FT||Scenario->Probe==EProbe::CA1||Scenario->Probe==EProbe::Saint;}
    bool RequiresVehicleMachinegun()const
    {return Scenario->Probe==EProbe::Austin||Scenario->Probe==EProbe::Train||Scenario->Probe==EProbe::CA1||Scenario->Probe==EProbe::Saint;}
    int32 WeaponReviewSerial()const
    {return Scenario->Probe==EProbe::Train?TrainCombatSerial:ReviewVehicleSerial;}
    AFrontlineTank* WeaponReviewVehicle()const
    {return Scenario->Probe==EProbe::Train?TrainCombatCarrier.Get():ReviewVehicle.Get();}
    void ObserveVehicleWeapons(const AFrontlineTank* Vehicle)
    {
        if(!Vehicle||Vehicle->Serial!=WeaponReviewSerial())return;
        VehicleRolled|=Vehicle->DistanceTravelled>600;
        VehicleCannonShot|=Vehicle->CannonShots>0;VehicleMachinegunShot|=Vehicle->MachinegunShots>0;
        VehicleShot=(!RequiresVehicleCannon()||VehicleCannonShot)&&(!RequiresVehicleMachinegun()||VehicleMachinegunShot);
    }
    FString VehicleCaseId()const
    {return Scenario->Probe==EProbe::FT?TEXT("fr.ft"):Scenario->Probe==EProbe::Train?TEXT("at-hu.armoured_train"):Scenario->Probe==EProbe::CA1?TEXT("fr.schneider"):Scenario->Probe==EProbe::Saint?TEXT("fr.saint_chamond"):TEXT("ru.austin_series1");}
    void RequiredEvent(const FString& Name,const TCHAR* Scope,TSharedRef<FJsonObject> Evidence)
    {
        Evidence->SetStringField(TEXT("window"),Name);Evidence->SetStringField(TEXT("scope"),Scope);
        if(const auto* NBattle=Battle())Evidence->SetNumberField(TEXT("hostElapsed"),NBattle->Elapsed);
        RequiredWindows.Add(MakeShared<FJsonValueObject>(Evidence));
    }
    bool PrepareReserveResearch()
    {
        auto* NBattle=Battle();FString Error;
        // This existing isolated helper records two preceding operations through
        // BeginEncounter/RecordVictory. Select the earlier operation for replay.
        if(!WarfareFRRUCampaignInternalV82::PrepareReviewMission(NBattle,TEXT("ru.gorlice"),Error,RequestedDifficultyV87))return false;
        wfexp::tactical::Campaign Expected,Actual;
        if(!wfexp::tactical::NewTacticalCampaign(wfexp::Faction::Russian,Expected))return false;
        const auto* FixtureFrontier=wfexp::FindMission("ru.gorlice");if(!FixtureFrontier)return false;
        for(const auto& Prior:wfexp::Missions())if(Prior.faction==wfexp::Faction::Russian&&Prior.order<FixtureFrontier->order)
        {
            const std::string Token=wfexpue::Narrow(FGuid::NewGuid().ToString(EGuidFormats::Digits));
            if(!wfexp::BeginEncounter(Expected.progression,Prior.id,Token)||!wfexp::RecordVictory(Expected.progression,Prior.id,Token))return false;
        }
        if(!WarfareFRRUCampaignInternalV82::CopyCampaign(NBattle,Actual))return false;
        Check(TEXT("reserve_fixture_points_come_from_public_record_victory"),Actual.progression.researchPoints==Expected.progression.researchPoints
            &&Actual.progression.victories==Expected.progression.victories&&Actual.progression.wonMissionIds==Expected.progression.wonMissionIds);
        const int32 Before=Actual.progression.researchPoints;
        const bool Premature=WarfareFRRUCampaignV82::PurchaseOption(NBattle,TEXT("ru.tech.6"));
        wfexp::tactical::Campaign Refused;
        Check(TEXT("reserve_tech6_requires_tech2_without_partial_purchase"),!Premature&&WarfareFRRUCampaignInternalV82::CopyCampaign(NBattle,Refused)
            &&Refused.progression.researchPoints==Before&&Refused.purchasedOptions==Actual.purchasedOptions);
        const bool First=WarfareFRRUCampaignV82::PurchaseOption(NBattle,TEXT("ru.tech.2"));
        const bool SecondPurchase=First&&WarfareFRRUCampaignV82::PurchaseOption(NBattle,TEXT("ru.tech.6"));
        const bool ExpectedFirst=bool(wfexp::tactical::PurchaseOption(Expected,"ru.tech.2"));
        const bool ExpectedSecond=ExpectedFirst&&bool(wfexp::tactical::PurchaseOption(Expected,"ru.tech.6"));
        Check(TEXT("reserve_two_options_bought_by_real_campaign_api"),First&&SecondPurchase&&ExpectedSecond
            &&WarfareFRRUCampaignV82::ResearchPoints(NBattle)==Expected.progression.researchPoints
            &&WarfareFRRUCampaignV82::OwnsOption(NBattle,TEXT("ru.tech.2"))&&WarfareFRRUCampaignV82::OwnsOption(NBattle,TEXT("ru.tech.6")));
        auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("pointsBefore"),Before);
        Evidence->SetNumberField(TEXT("pointsAfter"),WarfareFRRUCampaignV82::ResearchPoints(NBattle));
        Evidence->SetNumberField(TEXT("publicVictoryCount"),Expected.progression.victories);
        Evidence->SetBoolField(TEXT("pointsAssignedDirectly"),false);Evidence->SetBoolField(TEXT("priorVictoriesSynthetic"),true);
        RequiredEvent(TEXT("ru_research_prerequisite_and_reward"),TEXT("synthetic prior progression, actual public purchases"),Evidence);
        return First&&SecondPurchase&&WarfareFRRUCampaignV82::SelectMission(NBattle,TEXT("ru.galicia"))&&FirstFailure.IsEmpty();
    }
    void TickReloadWindows()
    {
        auto* NBattle=Battle();if(!NBattle||Scenario->Probe!=EProbe::Reserve)return;
        for(AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit&&Unit->IsAlive())
        {
            const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Unit);if(!Equipment)continue;
            const bool Maxim=Equipment->WeaponId==TEXT("ru.maxim")&&Unit->Team==0;
            const bool M95=Equipment->WeaponId==TEXT("at-hu.m95")&&Unit->Team==1;
            if(!Maxim&&!M95)continue;
            const int32 Person=Unit->SquadId*16+Unit->MemberIndex;auto& Loading=Maxim?MaximLoadingActors:M95LoadingActors;
            if(Unit->ShotsDischarged>0&&Unit->ReloadsStarted>0&&Unit->GetWeaponReloadNormalizedPhase()>0&&Unit->GetWeaponReloadNormalizedPhase()<1)Loading.Add(Person);
            if(Loading.Contains(Person)&&Unit->ReloadsCompleted>0&&Unit->RoundsRemaining==Equipment->Capacity)
            {
                bool& Done=Maxim?MaximReloadChecked:M95ReloadChecked;if(Done)continue;
                Done=true;Check(Maxim?TEXT("actual_maxim_fired_then_completed_live_reload"):TEXT("actual_m95_fired_then_completed_live_reload"),true);
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("person"),Person);Evidence->SetStringField(TEXT("weapon"),Equipment->WeaponId);
                Evidence->SetNumberField(TEXT("shots"),Unit->ShotsDischarged);Evidence->SetNumberField(TEXT("reloadsStarted"),Unit->ReloadsStarted);
                Evidence->SetNumberField(TEXT("reloadsCompleted"),Unit->ReloadsCompleted);Evidence->SetNumberField(TEXT("restoredRounds"),Unit->RoundsRemaining);
                RequiredEvent(TEXT("actual_weapon_reload"),TEXT("natural weapon cycle, no ammunition or pose grants"),Evidence);
            }
        }
    }
    void TickReserveOrders()
    {
        auto* NBattle=Battle();if(!NBattle||Scenario->Probe!=EProbe::Reserve)return;
        if(!ReserveRegistered&&Second!=INDEX_NONE&&At(Second,TEXT("home"))&&At(Lead,TEXT("assembly")))
        {
            ReserveUnitCount=NBattle->Units.Num();ReserveSquadCount=NBattle->Squads.Num();ReserveRecruitCount=NBattle->RecruitCommands;
            ReserveRegistered=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::RegisterReserve,{Second,Lead},TEXT("assembly"));
            Event(TEXT("register_two_existing_real_reserve_echelons"),ReserveRegistered);if(!ReserveRegistered)return;
            ReserveHeldAt=NBattle->Elapsed;
            ReserveAxesChecked=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::UseOption,{},TEXT("assembly"),TEXT("ru.tech.2"),FString(),1,0,TEXT("ridge"));
            Event(TEXT("real_bought_tech2_assigns_two_registered_axes"),ReserveAxesChecked);
            const auto* Pioneer=NBattle->FindSquad(Second);const auto* Rifle=NBattle->FindSquad(Lead);
            Check(TEXT("reserve_two_axes_require_actual_existing_pioneer_and_rifle"),ReserveAxesChecked&&Pioneer&&Rifle&&Pioneer->Role==EFrontlineRole::Pioneer&&Rifle->Role==EFrontlineRole::Rifle);
            const bool Early=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::ReleaseReserve);
            FirstReserveEarlyChecked=!Early&&NBattle->Units.Num()==ReserveUnitCount&&NBattle->Squads.Num()==ReserveSquadCount&&NBattle->RecruitCommands==ReserveRecruitCount;
            Check(TEXT("first_reserve_release_before_four_host_seconds_refuses_without_recruitment"),FirstReserveEarlyChecked);
            return;
        }
        if(ReserveRegistered&&!ReserveReleased&&NBattle->Elapsed>=ReserveHeldAt+4.05f)
        {
            const int32 UnitCount=NBattle->Units.Num(),SquadCount=NBattle->Squads.Num(),RecruitCount=NBattle->RecruitCommands;
            ReserveReleased=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::ReleaseReserve);
            Event(TEXT("first_real_reserve_release_after_four_host_seconds"),ReserveReleased);if(!ReserveReleased)return;
            ReserveReleasedAt=NBattle->Elapsed;
            const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Member(Second),false);
            Check(TEXT("first_existing_echelon_moves_to_registered_assembly_axis"),NBattle->Elapsed-ReserveHeldAt>=4.f
                &&Diagnosis->GetBoolField(TEXT("contextValid"))&&Diagnosis->GetStringField(TEXT("targetNode"))==TEXT("assembly")
                &&NBattle->Units.Num()==UnitCount&&NBattle->Squads.Num()==SquadCount&&NBattle->RecruitCommands==RecruitCount);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("existingSquad"),Second);
            Evidence->SetNumberField(TEXT("registeredAt"),ReserveHeldAt);Evidence->SetNumberField(TEXT("releasedAt"),ReserveReleasedAt);
            Evidence->SetNumberField(TEXT("actualAssemblySeconds"),ReserveReleasedAt-ReserveHeldAt);Evidence->SetStringField(TEXT("axis"),TEXT("assembly"));
            RequiredEvent(TEXT("ru_first_real_echelon"),TEXT("public commands on real entered squads; unique host clock, no copied recruitment"),Evidence);
            return;
        }
        if(ReserveReleased&&!SecondReserveEarlyChecked&&NBattle->Elapsed>=ReserveReleasedAt+1.f&&NBattle->Elapsed<ReserveReleasedAt+6.f)
        {
            const int32 UnitCount=NBattle->Units.Num(),SquadCount=NBattle->Squads.Num(),RecruitCount=NBattle->RecruitCommands;
            const auto Before=WarfareFRRUWorldV82::MovementDiagnosis(Member(Lead),false);
            const bool Early=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::ReleaseReserve);
            const auto After=WarfareFRRUWorldV82::MovementDiagnosis(Member(Lead),false);
            SecondReserveEarlyChecked=!Early&&NBattle->Units.Num()==UnitCount&&NBattle->Squads.Num()==SquadCount&&NBattle->RecruitCommands==RecruitCount
                &&Before->GetStringField(TEXT("targetNode"))==After->GetStringField(TEXT("targetNode"));
            Check(TEXT("second_echelon_before_six_real_host_seconds_refuses_without_partial_route"),SecondReserveEarlyChecked);
            return;
        }
        if(ReserveReleased&&!SecondReserveReleased&&NBattle->Elapsed>=ReserveReleasedAt+6.05f)
        {
            const int32 UnitCount=NBattle->Units.Num(),SquadCount=NBattle->Squads.Num(),RecruitCount=NBattle->RecruitCommands;
            SecondReserveReleased=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::ReleaseReserve);
            Event(TEXT("second_real_reserve_release_after_six_host_seconds"),SecondReserveReleased);if(!SecondReserveReleased)return;
            SecondReserveReleasedAt=NBattle->Elapsed;const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Member(Lead),false);
            ReserveIdentityChecked=SecondReserveEarlyChecked&&Diagnosis->GetBoolField(TEXT("contextValid"))&&Diagnosis->GetStringField(TEXT("targetNode"))==TEXT("ridge")
                &&NBattle->Units.Num()==UnitCount&&NBattle->Squads.Num()==SquadCount&&NBattle->RecruitCommands==RecruitCount;
            Check(TEXT("second_existing_echelon_keeps_identity_and_bought_axis_after_six_seconds"),ReserveIdentityChecked&&SecondReserveReleasedAt-ReserveReleasedAt>=6.f);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("existingSquad"),Lead);
            Evidence->SetNumberField(TEXT("firstReleasedAt"),ReserveReleasedAt);Evidence->SetNumberField(TEXT("secondReleasedAt"),SecondReserveReleasedAt);
            Evidence->SetNumberField(TEXT("actualEchelonSeconds"),SecondReserveReleasedAt-ReserveReleasedAt);Evidence->SetStringField(TEXT("axis"),TEXT("ridge"));
            RequiredEvent(TEXT("ru_second_real_echelon"),TEXT("real six-second boundary and accepted public move; no host-clock or points writes"),Evidence);
        }
        if(SecondReserveReleased&&!ObserverRecruited&&NBattle->RecruitBlockReason(EFrontlineRole::Lewis,0).IsEmpty())
        {Observer=Recruit(EFrontlineRole::Lewis,0);ObserverRecruited=Observer!=INDEX_NONE;}
        if(NBattle->Elapsed<ReserveTacticalRetryAt)return;ReserveTacticalRetryAt=NBattle->Elapsed+1.f;
        const auto ReadyAt=[&](int32 Squad,const FString& Node)
        {const auto* Group=NBattle->FindSquad(Squad);return Group&&!Group->bAdvancing&&!Group->bHoldPosition&&At(Squad,Node);};
        // The bought ridge axis and section identity have already been read
        // back. Vacate that real checkpoint through an ordinary registered
        // route before asking the following pioneer section to pass through.
        if(ReserveIdentityChecked&&!ReserveRidgeVacated&&ReadyAt(Lead,TEXT("ridge")))
        {
            ReserveRidgeVacated=WarfareFRRUWorldV82::MoveSquad(NBattle,Lead,TEXT("relay"));
            Event(TEXT("reserve_rifle_public_vacate_ridge_to_relay"),ReserveRidgeVacated);
        }
        if(ReserveRidgeVacated&&!ReserveLeadHeld&&!ReserveLeadReleased&&ReadyAt(Lead,TEXT("relay")))
        {
            ReserveLeadHeld=WarfareFRRUWorldV82::HoldSquad(NBattle,Lead,true);
            Event(TEXT("reserve_rifle_public_hold_while_maxim_establishes_fire"),ReserveLeadHeld);
        }
        // The earlier run exposed the work party before its covering gun
        // arrived. Hold the same existing echelon at its real assembly post,
        // using only public orders, until the Maxim discharges a real round.
        if(ReserveIdentityChecked&&!ReservePioneerScreenHeld&&ReadyAt(Second,TEXT("assembly")))
        {
            ReservePioneerScreenHeld=WarfareFRRUWorldV82::HoldSquad(NBattle,Second,true);
            Event(TEXT("reserve_pioneer_public_wait_for_covering_gun"),ReservePioneerScreenHeld);
        }
        if(ReservePioneerScreenHeld&&!ReservePioneerScreenReleased)
        {
            bool CoveringGunFired=false;
            if(const auto* Group=NBattle->FindSquad(Observer))for(const auto& Weak:Group->Members)
                if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->IsMachineGunner()&&Unit->ShotsDischarged>0)CoveringGunFired=true;
            if(CoveringGunFired)
            {
                ReservePioneerScreenReleased=WarfareFRRUWorldV82::HoldSquad(NBattle,Second,false);
                Event(TEXT("reserve_pioneer_public_advance_under_real_maxim_fire"),ReservePioneerScreenReleased);
            }
        }
        const bool BridgeCleared=WarfareFRRUWorldV82::PassageCleared(NBattle,TEXT("bridge"));
        if(BridgeCleared&&ReadyAt(Second,TEXT("bridge")))
            Event(TEXT("reserve_pioneer_public_vacate_cleared_bridge"),WarfareFRRUWorldV82::MoveSquad(NBattle,Second,TEXT("assembly")));
        if(ReadyAt(Observer,TEXT("home")))
            Event(TEXT("maxim_natural_frontline_route"),WarfareFRRUWorldV82::MoveSquad(NBattle,Observer,TEXT("front_left")));
        bool MaximHasContact=false;
        if(const auto* Group=NBattle->FindSquad(Observer))for(const auto& Weak:Group->Members)
            if(auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->IsMachineGunner())
                if(auto* Gun=FrontlineMachinegunsV30::Get(Unit))
                    for(const AWarfareFrontlineSoldier* Enemy:NBattle->Units)
                        if(Enemy&&Enemy->Team!=Unit->Team&&Enemy->IsActiveCombatant()
                            &&FVector::Dist2D(Unit->Feet(),Enemy->Feet())<=FrontlineWeapons::ProfileFor(Unit).FireRangeCm*NBattle->EncounterVisibilityScale()
                            &&NBattle->HasSight(Unit,Enemy)&&WarfareFRRUWorldV82::FiringLine(Unit,Enemy)&&Gun->HasInfantryFiringLine(Enemy))MaximHasContact=true;
        // The gun covers the southern work party through its actual route.
        // Move to the genuinely cleared bridge when no live contact is present;
        // neither targets nor rounds nor an artificial firing window are given.
        if(BridgeCleared&&!MaximReloadChecked&&!MaximHasContact&&ReadyAt(Observer,TEXT("front_left")))
            Event(TEXT("maxim_public_approach_to_cleared_bridge"),WarfareFRRUWorldV82::MoveSquad(NBattle,Observer,TEXT("bridge")));
        if((!MaximReloadChecked||!M95ReloadChecked)
            &&(ReadyAt(Hostile,TEXT("enemy_assembly"))||ReadyAt(Hostile,TEXT("objective"))))
            Event(TEXT("m95_public_existing_section_bridge_approach"),WarfareFRRUWorldV82::MoveSquad(NBattle,Hostile,TEXT("bridge")));
        if(MaximReloadChecked&&M95ReloadChecked&&ReserveLeadHeld&&!ReserveLeadReleased)
        {
            if(ReadyAt(Observer,TEXT("bridge"))&&!WarfareFRRUWorldV82::MoveSquad(NBattle,Observer,TEXT("objective")))return;
            ReserveLeadReleased=WarfareFRRUWorldV82::HoldSquad(NBattle,Lead,false);
            Event(TEXT("reserve_rifle_public_resume_after_real_weapon_cycles"),ReserveLeadReleased);
        }
    }
    void TickFlameWindows()
    {
        auto* NBattle=Battle();if(!NBattle||Scenario->Probe!=EProbe::Schilt||UGameplayStatics::IsGamePaused(NBattle))return;
        if(!EmptyRefusalChecked&&FlameShotsObserved==12)
        {
            auto* Operator=Member(Second);FFrontlineFlameStateV82 Before;
            if(Operator&&Operator->IsActiveCombatant()&&!Operator->bMoving&&Operator->GetMotionSpeed()<=.5f
                &&Operator->WeaponPhase==EFrontlineWeaponPhase::Ready&&NBattle->Phase==EFrontlinePhase::Fighting
                &&FrontlineMachinegunsV30::CanFire(Operator)
                &&FrontlineFieldV32::QueryFlameStateV82(Operator,Before)&&Before.FuelCharges==0&&NBattle->Elapsed>=Before.ReadyAt)
            {
                const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Operator);AWarfareFrontlineSoldier* Target=nullptr;
                for(AWarfareFrontlineSoldier* Candidate:NBattle->Units)if(Candidate&&Candidate->Team!=Operator->Team&&Candidate->IsActiveCombatant()
                    &&Equipment&&Equipment->WeaponId==TEXT("fr.schilt")&&FMath::IsFinite(Equipment->Profile.FireRangeCm)&&Equipment->Profile.FireRangeCm>0
                    &&FMath::IsFinite(Equipment->Profile.AimedCycleSeconds)&&Equipment->Profile.AimedCycleSeconds>=1.33f
                    &&FVector::Dist2D(Operator->Feet(),Candidate->Feet())<=Equipment->Profile.FireRangeCm
                    &&FrontlineWeapons::DirectFireRangeDistanceCm(Operator,Candidate->Feet())<=Equipment->Profile.FireRangeCm*NBattle->EncounterVisibilityScale()
                    &&NBattle->HasSight(Operator,Candidate)&&WarfareFRRUWorldV82::FiringLine(Operator,Candidate)){Target=Candidate;break;}
                if(Target)
                {
                    const int32 OwnShots=Operator->ShotsDischarged,AllShots=NBattle->ShotsFired;
                    FrontlineFieldV32::FlameDischarge(Operator,Target);FFrontlineFlameStateV82 After;
                    const bool Stable=FrontlineFieldV32::QueryFlameStateV82(Operator,After)&&Operator->ShotsDischarged==OwnShots&&NBattle->ShotsFired==AllShots
                        &&After.FuelCharges==0&&Before.StartedAt==After.StartedAt&&Before.JetEndsAt==After.JetEndsAt&&Before.ReadyAt==After.ReadyAt;
                    Check(TEXT("twelve_natural_schilt_discharges_then_empty_valid_target_refusal"),Stable&&OwnShots==12);
                    EmptyRefusalChecked=Stable&&OwnShots==12;auto Evidence=MakeShared<FJsonObject>();
                    Evidence->SetNumberField(TEXT("operatorPerson"),Operator->SquadId*16+Operator->MemberIndex);Evidence->SetNumberField(TEXT("targetPerson"),Target->SquadId*16+Target->MemberIndex);
                    Evidence->SetNumberField(TEXT("normalDischarges"),OwnShots);Evidence->SetBoolField(TEXT("validTargetAndReadyClock"),true);
                    Evidence->SetBoolField(TEXT("actualNonFuelDischargeGuardsChecked"),true);
                    Evidence->SetBoolField(TEXT("clockFuelAndShotsUnchanged"),Stable);
                    RequiredEvent(TEXT("schilt_empty_refusal"),TEXT("twelve natural discharges plus public discharge refusal query"),Evidence);
                }
            }
        }
        if(!EmptyRefusalChecked)return;
        if(RuptureSquad==INDEX_NONE&&NBattle->RecruitBlockReason(EFrontlineRole::Flamethrower,0).IsEmpty())RuptureSquad=Recruit(EFrontlineRole::Flamethrower,0);
        if(!RuptureHitIssued)
        {
            auto* Carrier=Member(RuptureSquad);FFrontlineFlameStateV82 Before;
            if(!Carrier||!At(RuptureSquad,TEXT("home"))||!Carrier->IsActiveCombatant()||!FrontlineFieldV32::QueryFlameStateV82(Carrier,Before))return;
            Check(TEXT("independent_schilt_carrier_starts_with_twelve_unspent_charges"),RuptureSquad!=Second&&Carrier->ShotsDischarged==0&&Before.FuelCharges==12&&!Before.FuelIgnited&&!Before.FuelSpent);
            if(!FirstFailure.IsEmpty())return;
            RuptureActor=Carrier;RuptureBefore=int32(FrontlineFieldV32::Review(NBattle)->GetNumberField(TEXT("fuel_bursts_v33")));RuptureHitAt=NBattle->Elapsed;
            Carrier->ReceiveHit(1.f,.8f,1,FVector(1,0,0),EFrontlineDamageKind::Grenade,.7f);RuptureHitIssued=true;
            FFrontlineFlameStateV82 After;const bool Ignited=FrontlineFieldV32::QueryFlameStateV82(Carrier,After)&&!Carrier->IsAlive()&&After.FuelIgnited&&!After.FuelSpent&&After.FuelCharges==12&&After.FuelBurstAt>RuptureHitAt;
            Check(TEXT("public_lethal_blast_ignites_independent_remaining_fuel_with_real_delay"),Ignited);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("person"),Carrier->SquadId*16+Carrier->MemberIndex);
            Evidence->SetNumberField(TEXT("hitAt"),RuptureHitAt);Evidence->SetNumberField(TEXT("scheduledBurstAt"),After.FuelBurstAt);
            Evidence->SetNumberField(TEXT("beforeFuel"),Before.FuelCharges);Evidence->SetBoolField(TEXT("publicReceiveHit"),true);
            RequiredEvent(TEXT("schilt_independent_reservoir_hit"),TEXT("controlled damage to naturally recruited carrier; no health/fuel writes"),Evidence);
        }
        if(auto* Carrier=RuptureActor.Get();Carrier&&RuptureHitIssued&&!RuptureChecked)
        {
            FFrontlineFlameStateV82 State;if(!FrontlineFieldV32::QueryFlameStateV82(Carrier,State))return;
            const int32 Bursts=int32(FrontlineFieldV32::Review(NBattle)->GetNumberField(TEXT("fuel_bursts_v33")));const auto& Death=Carrier->GetDeathState();
            if(!State.FuelSpent||!Death.bFuelRuptureApplied||Death.PublishedPoseSamples==0)return;
            if(!RuptureRepeatIssued)
            {
                Check(TEXT("one_actual_reservoir_burst_is_owned_by_same_carrier"),Bursts==RuptureBefore+1&&State.FuelCharges==0&&Death.bFuelRupture&&Death.bFuelRuptureApplied);
                RuptureAt=Bursts;RuptureDeathAt=Death.FuelRuptureAt;RuptureStableSince=NBattle->Elapsed;
                Carrier->ReceiveHit(1.f,.8f,1,FVector(1,0,0),EFrontlineDamageKind::Grenade,.7f);RuptureRepeatIssued=true;return;
            }
            if(NBattle->Elapsed<RuptureStableSince+2.f)return;
            RuptureChecked=Bursts==RuptureAt&&Bursts==RuptureBefore+1&&State.FuelCharges==0&&State.FuelSpent&&Death.FuelRuptureAt==RuptureDeathAt;
            Check(TEXT("spent_reservoir_rehit_and_two_real_seconds_do_not_repeat_burst"),RuptureChecked);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("burstCounterBefore"),RuptureBefore);Evidence->SetNumberField(TEXT("burstCounterAfter"),Bursts);
            Evidence->SetNumberField(TEXT("stableObservedHostSeconds"),NBattle->Elapsed-RuptureStableSince);Evidence->SetNumberField(TEXT("sameCarrierRuptureAge"),Death.FuelRuptureAt);
            Evidence->SetBoolField(TEXT("actualChaosImpulseApplied"),Death.bFuelRuptureApplied);Evidence->SetNumberField(TEXT("publishedPoseSamples"),Death.PublishedPoseSamples);
            RequiredEvent(TEXT("schilt_independent_once_only_rupture"),TEXT("controlled public rehit; natural host tick and actual published physics"),Evidence);
        }
    }
    FTransform VehicleReferenceFrame(const FReferenceSkeleton& Ref,int32 Index)const
    {
        FTransform Frame=Ref.GetRefBonePose()[Index];
        for(int32 Parent=Ref.GetParentIndex(Index);Parent!=INDEX_NONE;Parent=Ref.GetParentIndex(Parent))Frame=Frame*Ref.GetRefBonePose()[Parent];
        return Frame;
    }
    void CheckVehiclePose(AFrontlineTank* Vehicle)
    {
        const auto* D=WarfareFRRUArmourV82::Definition(Vehicle);if(!D)return;
        TArray<UPoseableMeshComponent*> Parts;Vehicle->GetComponents(Parts);UPoseableMeshComponent* Pose=nullptr;
        for(auto* Part:Parts)if(Part&&Part->GetFName()==TEXT("NationalArmouredModel"))Pose=Part;
        auto* Mesh=Pose?Cast<USkeletalMesh>(Pose->GetSkinnedAsset()):nullptr;
        // PoseableMesh inherits USkinnedMeshComponent; read its real asset via
        // the base API instead of assuming SkeletalMeshComponent methods.
        if(!Mesh){Check(TEXT("vehicle_pose_fixture_has_actual_national_mesh"),false);return;}
        const auto& Ref=Mesh->GetRefSkeleton();const FTransform ActorBefore=Vehicle->GetActorTransform();
        const auto Name=[](const TCHAR* Suffix){return FName(Suffix);};
        struct FMountProbe{FName Yaw,Pitch,Muzzle;int32 Index=0;bool Cannon=true;};
        TArray<FMountProbe> Mounts;
        if(D->Id==TEXT("fr.schneider"))Mounts.Add({Name(TEXT("short_75_right_yaw")),Name(TEXT("short_75_right_pitch")),Name(TEXT("short_75_right_muzzle")),0,true});
        else if(D->Id==TEXT("fr.saint_chamond"))Mounts.Add({Name(TEXT("frontal_75_yaw")),Name(TEXT("frontal_75_pitch")),Name(TEXT("frontal_75_muzzle")),0,true});
        else if(D->Id==TEXT("fr.ft"))Mounts.Add({Name(TEXT("turret_yaw")),Name(TEXT("SA18_pitch")),Name(TEXT("SA18_muzzle")),0,true});
        else if(D->Id==TEXT("ru.austin_series1"))
        {
            Mounts.Add({Name(TEXT("turret_-1_yaw")),Name(TEXT("Maxim_-1_pitch")),Name(TEXT("Maxim_-1_muzzle")),0,false});
            Mounts.Add({Name(TEXT("turret_1_yaw")),Name(TEXT("Maxim_1_pitch")),Name(TEXT("Maxim_1_muzzle")),1,false});
        }
        bool Passed=true;auto Evidence=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Poses;
        if(D->Rail)
        {
            const FName Port=Name(TEXT("car_0_port_-1_-849"));const int32 BoneIndex=Ref.FindBoneIndex(Port);
            Passed=Ref.GetNum()==41&&BoneIndex!=INDEX_NONE;
            // FBX reflects source Y once. The displayed component undoes that
            // basis, so the authored port's 31cm terminal extends raw +Y.
            const FVector PortEnd=Passed?Pose->GetBoneTransformByName(Port,EBoneSpaces::ComponentSpace).TransformPosition(FVector(0,31,0)):FVector::ZeroVector;
            for(float Angle:{90.f,180.f})
            {
                Vehicle->SetActorRotation(FRotator(0,Angle,0));Pose->UpdateComponentToWorld();
                const FVector Expected=Pose->GetComponentTransform().TransformPosition(PortEnd);
                const FVector Actual=WarfareFRRUArmourV82::Muzzle(Vehicle,0,false);const double Error=FVector::Dist(Expected,Actual);
                Passed&=Error<=.1;auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("actorYawDegrees"),Angle);
                Row->SetStringField(TEXT("fixedPortBone"),Port.ToString());Row->SetStringField(TEXT("posedPortEnd"),Expected.ToString());
                Row->SetStringField(TEXT("queriedMuzzle"),Actual.ToString());Row->SetNumberField(TEXT("errorCm"),Error);Poses.Add(MakeShared<FJsonValueObject>(Row));
            }
            Vehicle->SetActorRotation(ActorBefore.GetRotation());Pose->UpdateComponentToWorld();
            Evidence->SetBoolField(TEXT("fixedPortsNotTurrets"),true);
        }
        for(const auto& Mount:Mounts)
        {
            const int32 PitchIndex=Ref.FindBoneIndex(Mount.Pitch),YawIndex=Ref.FindBoneIndex(Mount.Yaw);
            if(PitchIndex==INDEX_NONE||YawIndex==INDEX_NONE||Ref.FindBoneIndex(Mount.Muzzle)==INDEX_NONE){Passed=false;continue;}
            const FTransform OldYaw=Pose->GetBoneTransformByName(Mount.Yaw,EBoneSpaces::ComponentSpace);
            const FTransform OldPitch=Pose->GetBoneTransformByName(Mount.Pitch,EBoneSpaces::ComponentSpace);
            for(float Angle:{90.f,180.f})
            {
                FTransform NewYaw=OldYaw;NewYaw.SetRotation(FRotator(0,Angle,0).Quaternion()*OldYaw.GetRotation());
                Pose->SetBoneTransformByName(Mount.Yaw,NewYaw,EBoneSpaces::ComponentSpace);Pose->RefreshBoneTransforms();
                FTransform NewPitch=Pose->GetBoneTransformByName(Mount.Pitch,EBoneSpaces::ComponentSpace);
                NewPitch.SetRotation(FRotator(10,Angle,0).Quaternion());Pose->SetBoneTransformByName(Mount.Pitch,NewPitch,EBoneSpaces::ComponentSpace);Pose->RefreshBoneTransforms();
                const FTransform ActualBone=Pose->GetBoneTransformByName(Mount.Pitch,EBoneSpaces::ComponentSpace);
                const FVector Expected=Pose->GetComponentTransform().TransformPosition(Pose->GetBoneLocationByName(Mount.Muzzle,EBoneSpaces::ComponentSpace));
                const FVector Actual=WarfareFRRUArmourV82::Muzzle(Vehicle,Mount.Index,Mount.Cannon);const double Error=FVector::Dist(Expected,Actual);
                Passed&=Error<=.1&&!ActualBone.ContainsNaN();auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("controlledYawDegrees"),Angle);
                Row->SetNumberField(TEXT("controlledPitchDegrees"),10);Row->SetStringField(TEXT("actualPitchBone"),Mount.Pitch.ToString());
                Row->SetStringField(TEXT("actualBoneTransform"),ActualBone.ToString());Row->SetStringField(TEXT("expectedMuzzle"),Expected.ToString());
                Row->SetStringField(TEXT("queriedMuzzle"),Actual.ToString());Row->SetNumberField(TEXT("errorCm"),Error);Poses.Add(MakeShared<FJsonValueObject>(Row));
            }
            Pose->SetBoneTransformByName(Mount.Yaw,OldYaw,EBoneSpaces::ComponentSpace);Pose->RefreshBoneTransforms();
            Pose->SetBoneTransformByName(Mount.Pitch,OldPitch,EBoneSpaces::ComponentSpace);Pose->RefreshBoneTransforms();
            Passed&=Pose->GetBoneTransformByName(Mount.Yaw,EBoneSpaces::ComponentSpace).Equals(OldYaw,.001)
                &&Pose->GetBoneTransformByName(Mount.Pitch,EBoneSpaces::ComponentSpace).Equals(OldPitch,.001);
        }
        Passed&=Vehicle->GetActorTransform().Equals(ActorBefore,.001);
        Check(TEXT("actual_vehicle_90_180_posed_bone_muzzle_and_complete_restore"),Passed,D->Id);
        Evidence->SetStringField(TEXT("vehicle"),D->Id);Evidence->SetNumberField(TEXT("serial"),Vehicle->Serial);Evidence->SetArrayField(TEXT("poses"),Poses);
        Evidence->SetBoolField(TEXT("actorLocationWritten"),false);Evidence->SetBoolField(TEXT("legalHistoricalTraverseClaimed"),false);
        Evidence->SetBoolField(TEXT("boneAndActorPoseRestoredSameFrame"),Passed);
        RequiredEvent(TEXT("actual_vehicle_muzzle_pose"),TEXT("controlled actual pose queries, no fire/time/position grant"),Evidence);VehiclePoseChecked=Passed;
    }
    void TickVehicleWindows()
    {
        auto* NBattle=Battle();auto* Vehicle=WeaponReviewVehicle();if(!NBattle||!Vehicle||!IsVehicleCase()||UGameplayStatics::IsGamePaused(NBattle))return;
        const auto* D=WarfareFRRUArmourV82::Definition(Vehicle);if(!D||!Vehicle->IsOperational())return;
        if(Vehicle->MachinegunShots>VehicleLastMGShots){VehicleLastMGAt=NBattle->Elapsed;VehicleLastMGShots=Vehicle->MachinegunShots;}
        // Natural roll/fire must be observed BEFORE controlled damage/pose.
        ObserveVehicleWeapons(Vehicle);
        if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82FTContactOnly")))return;
        if(!VehicleRolled||!VehicleShot||!Vehicle->HasEnteredField())return;
        if(D->Id==TEXT("fr.ft")&&!FTBlastFootprintCheckedV826)
        {
            auto* Fleet=NBattle->GetWorld()->GetSubsystem<UFrontlineVehicleSubsystem>();
            if(!Fleet){Check(TEXT("ft_boundary_has_real_vehicle_subsystem"),false);return;}
            const float Hull=Vehicle->GetHullIntegrity(),Left=Vehicle->GetTrackIntegrity(0),Right=Vehicle->GetTrackIntegrity(1);
            FFrontlineKillSourceV65 Credit;Credit.Team=1-Vehicle->Team;
            TArray<TSharedPtr<FJsonValue>> Points;
            for(const FVector& Local:{FVector(0,280,35),FVector(D->Maximum.X+193.,0,35)})
            {
                const FVector Near(FMath::Clamp(Local.X,D->Minimum.X,D->Maximum.X),FMath::Clamp(Local.Y,D->Minimum.Y,D->Maximum.Y),35);
                const float Gap=FVector::Dist2D(Local,Near);
                const FVector World=Vehicle->GetActorTransform().TransformPosition(Local);
                Fleet->ApplyGrenadeBlast(World,170,Credit.Team,Credit);
                Fleet->ApplyBlast(World,170,50,Credit.Team,Credit);
                const bool Unchanged=Vehicle->GetHullIntegrity()==Hull&&Vehicle->GetTrackIntegrity(0)==Left&&Vehicle->GetTrackIntegrity(1)==Right;
                Check(TEXT("ft_outside_actual_footprint_ignores_public_blast_and_grenade"),Gap>170&&Unchanged,Local.ToString());
                auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("actualVehicleLocalPoint"),Local.ToString());P->SetNumberField(TEXT("actualFootprintGapCm"),Gap);
                P->SetBoolField(TEXT("hullAndTracksUnchanged"),Unchanged);Points.Add(MakeShared<FJsonValueObject>(P));
            }
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetArrayField(TEXT("outsideRealHullPoints"),Points);
            Evidence->SetNumberField(TEXT("serial"),Vehicle->Serial);Evidence->SetBoolField(TEXT("vehiclePositionWritten"),false);
            RequiredEvent(TEXT("ft_actual_blast_footprint_v826"),TEXT("controlled public explosion calls on a naturally entered and firing FT; outside actual asymmetric hull"),Evidence);
            FTBlastFootprintCheckedV826=true;if(!FirstFailure.IsEmpty())return;
        }
        if(Scenario->Probe==EProbe::Train&&!TrainCombatFireRecorded)
        {
            TrainCombatFireRecorded=true;auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("serial"),Vehicle->Serial);
            Evidence->SetNumberField(TEXT("actualTravelCm"),Vehicle->DistanceTravelled);Evidence->SetNumberField(TEXT("actualMachinegunShots"),Vehicle->MachinegunShots);
            Evidence->SetNumberField(TEXT("firstPassengerProofSerial"),FirstNaturalTrainSerial);Evidence->SetNumberField(TEXT("controlledWreckProofSerial"),RetryTrainSerial);
            Evidence->SetBoolField(TEXT("weaponCountersInheritedFromOtherCarrier"),false);
            RequiredEvent(TEXT("train_third_carrier_natural_roll_and_machinegun_fire"),TEXT("separate ordinary public combat recruit; actual travel, targets and discharge counters"),Evidence);
        }
        if(!VehiclePoseChecked){CheckVehiclePose(Vehicle);if(!FirstFailure.IsEmpty())return;}
        if(!VehicleTrackChecked)
        {
            if(D->Rail)
            {
                const float HullBefore=Vehicle->GetHullIntegrity(),LeftBefore=Vehicle->GetTrackIntegrity(0),RightBefore=Vehicle->GetTrackIntegrity(1);
                const bool LeftRepair=Vehicle->RepairTrackV60(0),RightRepair=Vehicle->RepairTrackV60(1);
                VehicleTrackChecked=!LeftRepair&&!RightRepair&&Vehicle->GetHullIntegrity()==HullBefore&&Vehicle->GetTrackIntegrity(0)==LeftBefore&&Vehicle->GetTrackIntegrity(1)==RightBefore;
                Check(TEXT("train_running_gear_repair_is_not_falsely_unlocked"),VehicleTrackChecked);
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("serial"),Vehicle->Serial);Evidence->SetBoolField(TEXT("leftRepairAccepted"),LeftRepair);
                Evidence->SetBoolField(TEXT("rightRepairAccepted"),RightRepair);Evidence->SetBoolField(TEXT("integrityUnchanged"),VehicleTrackChecked);
                RequiredEvent(TEXT("train_running_gear_public_repair_refusal"),TEXT("same separately recruited combat carrier; real public repair calls"),Evidence);
            }
            else if(TrackStage==0||TrackStage==2)
            {
                const int32 Side=TrackStage/2;const float HullBefore=Vehicle->GetHullIntegrity();
                Vehicle->ReceiveModuleHit(Side?EFrontlineTankModule::RightTrack:EFrontlineTankModule::LeftTrack,Vehicle->GetTrackIntegrity(Side)+1);
                Check(TEXT("actual_running_gear_hit_immobilizes_without_hull_change"),Vehicle->IsImmobilized()&&Vehicle->GetTrackIntegrity(Side)==0&&Vehicle->GetHullIntegrity()==HullBefore);
                TrackAt=NBattle->Elapsed;TrackStoppedPosition=Vehicle->GetActorLocation();++TrackStage;
            }
            else if(NBattle->Elapsed>=TrackAt+1.f)
            {
                const int32 Side=TrackStage/2;const float HullBefore=Vehicle->GetHullIntegrity();
                const bool Stopped=FVector::Dist2D(TrackStoppedPosition,Vehicle->GetActorLocation())<=.01;
                const bool Available=Vehicle->CanRepairTrackV60(Side);const bool Repaired=Vehicle->RepairTrackV60(Side);
                const float Restored=Vehicle->GetTrackIntegrity(Side);const bool Duplicate=Vehicle->RepairTrackV60(Side);
                const bool Passed=Stopped&&Available&&Repaired&&!Duplicate&&Restored>0&&Vehicle->GetTrackIntegrity(Side)==Restored&&Vehicle->GetHullIntegrity()==HullBefore;
                Check(TEXT("one_real_second_stopped_then_public_one_time_running_gear_repair"),Passed);
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("vehicle"),D->Id);Evidence->SetNumberField(TEXT("side"),Side);
                Evidence->SetNumberField(TEXT("stoppedHostSeconds"),NBattle->Elapsed-TrackAt);Evidence->SetBoolField(TEXT("noTranslationWhileBroken"),Stopped);
                Evidence->SetBoolField(TEXT("firstRepairAccepted"),Repaired);Evidence->SetBoolField(TEXT("duplicateRefused"),!Duplicate);
                Evidence->SetNumberField(TEXT("hullBeforeRepair"),HullBefore);Evidence->SetNumberField(TEXT("hullAfterRepair"),Vehicle->GetHullIntegrity());
                RequiredEvent(TEXT("vehicle_running_gear"),TEXT("controlled public module hit and repair; natural tick displacement"),Evidence);
                if(!Passed)return;++TrackStage;VehicleTrackChecked=TrackStage==4;
            }
            return;
        }
        const int32 Count=D->Id==TEXT("fr.schneider")?2:D->Id==TEXT("fr.saint_chamond")?4:D->Id==TEXT("ru.austin_series1")?2:D->Rail?12:0;
        if(Count==0&&!VehicleGunChecked)
        {
            const float HullBefore=Vehicle->GetHullIntegrity();const int32 Shots=Vehicle->MachinegunShots;
            Vehicle->ReceiveModuleHit(EFrontlineTankModule::Machinegun,66,0);const bool Repair=Vehicle->RepairMachinegunV63(0);
            VehicleGunChecked=!Repair&&Vehicle->GetHullIntegrity()==HullBefore&&Vehicle->MachinegunShots==Shots&&Vehicle->GetMachinegunIntegrity(0)==0;
            Check(TEXT("ft_sa18_has_no_invented_machinegun_or_machinegun_repair"),VehicleGunChecked);return;
        }
        if(Count>0&&GunDisabledAt<0&&VehicleLastMGAt>=0&&NBattle->Elapsed-VehicleLastMGAt<1.f)
        {
            const float HullBefore=Vehicle->GetHullIntegrity();bool AllBroken=true;
            for(int32 Index=0;Index<Count;++Index){Vehicle->ReceiveModuleHit(EFrontlineTankModule::Machinegun,Vehicle->GetMachinegunIntegrity(Index)+1,Index);AllBroken&=Vehicle->GetMachinegunIntegrity(Index)==0;}
            Check(TEXT("actual_all_mg_modules_disabled_without_hull_damage"),AllBroken&&Vehicle->GetHullIntegrity()==HullBefore);
            bool AnyRepair=false;for(int32 Index=0;Index<Count;++Index)AnyRepair|=Vehicle->RepairMachinegunV63(Index);
            Check(TEXT("tactical_v2_does_not_gain_classic_mg_repair"),!AnyRepair&&Vehicle->GetHullIntegrity()==HullBefore);
            GunDisabledAt=NBattle->Elapsed;BrokenMGShots=Vehicle->MachinegunShots;return;
        }
        if(GunDisabledAt>=0&&!VehicleGunChecked)
        {
            bool EligibleNow=false;
            for(AWarfareFrontlineSoldier* Target:NBattle->Units)if(Target&&Target->IsActiveCombatant()&&Target->Team!=Vehicle->Team)
                for(int32 Index=0;Index<Count;++Index)
                {
                    const FVector From=WarfareFRRUArmourV82::Muzzle(Vehicle,Index,false),To=Target->Feet()+FVector(0,0,100);
                    if(FVector::Dist2D(Vehicle->GetActorLocation(),To)>=D->Range||!WarfareFRRUWorldV82::HasSight(NBattle,From,To))continue;
                    // Reproduce the declared mount's actual target sector,
                    // rather than treating any visible enemy as permission.
                    const int32 Sign=Index<Count/2?-1:1;FVector Origin;float Base=Sign*90.f,Arc=45.f;
                    if(D->Id==TEXT("fr.schneider"))Origin=FVector(-58,Sign*164.,148);
                    else if(D->Id==TEXT("fr.saint_chamond"))Origin=FVector(Index%2?174:-210,Sign*186.,146);
                    else if(D->Id==TEXT("ru.austin_series1")){Origin=FVector(24,Sign*48.,216);Base=Sign*35.f;Arc=145.f;}
                    else {const int32 Within=Index%(Count/2),Car=Within/3;Origin=FVector((Car==0?-849.:511.)+(Within%3)*169.,Sign*143.,214);Arc=48.f;}
                    const FRotator Wanted=(Vehicle->GetActorTransform().InverseTransformPosition(To)-Origin).Rotation();
                    if(FMath::Abs(FMath::FindDeltaAngleDegrees(Base,float(Wanted.Yaw)))>Arc||Wanted.Pitch< -10||Wanted.Pitch>20)continue;
                    FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUNaturalDisabledGun),true);Query.AddIgnoredActor(Vehicle);Query.AddIgnoredActor(Target);
                    for(AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit)Query.AddIgnoredActor(Unit);
                    EligibleNow|=!NBattle->GetWorld()->LineTraceTestByChannel(From,To,ECC_Visibility,Query);
                }
            DisabledGunHasTarget|=EligibleNow;
            if(!EligibleNow)DisabledGunEligibleAt=-1;else if(DisabledGunEligibleAt<0)DisabledGunEligibleAt=NBattle->Elapsed;
            if(DisabledGunEligibleAt>=0&&NBattle->Elapsed>=DisabledGunEligibleAt+3.f)
            {
                VehicleGunChecked=Vehicle->MachinegunShots==BrokenMGShots;
                Check(TEXT("disabled_actual_mg_modules_stay_silent_across_three_real_seconds_with_visible_enemy"),VehicleGunChecked);
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("vehicle"),D->Id);Evidence->SetNumberField(TEXT("moduleCount"),Count);
                Evidence->SetNumberField(TEXT("serial"),Vehicle->Serial);
                Evidence->SetNumberField(TEXT("beforeShots"),BrokenMGShots);Evidence->SetNumberField(TEXT("afterShots"),Vehicle->MachinegunShots);
                Evidence->SetNumberField(TEXT("observedHostSeconds"),NBattle->Elapsed-DisabledGunEligibleAt);Evidence->SetBoolField(TEXT("actualVisibleEnemyObserved"),DisabledGunHasTarget);
                Evidence->SetBoolField(TEXT("continuousTargetInActualMountSectorAndPitch"),true);
                RequiredEvent(TEXT("vehicle_disabled_guns"),TEXT("controlled damage after natural fire; real target/LOS and natural host clock"),Evidence);
            }
        }
    }
    void RemoveReceivingBayFixtures()
    {
        for(const auto& Weak:ReceivingBayFixtures)if(auto* Actor=Weak.Get())Actor->Destroy();
        ReceivingBayFixtures.Reset();CollisionFixtureCreated=false;
    }
    bool CreateReceivingBayFixtures()
    {
        auto* NBattle=Battle();FString ExactUser;
        if(!NBattle||!IsInGameThread()||!Isolation(ExactUser)||ExactUser!=User||!NBattle->IsProgressionVolatile()
            ||NBattle->Phase!=EFrontlinePhase::Fighting)return false;
        RemoveReceivingBayFixtures();const auto View=WarfareFRRUWorldV82::View(NBattle);
        for(const auto& Node:View.Nodes)if(Node.Capacity>0&&Node.Id!=TEXT("home")&&Node.Id!=TEXT("assembly")&&Node.Id!=TEXT("relay")&&Node.Id!=TEXT("exit"))
        {
            float Floor=0;if(!WarfareFRRUWorldV82::TryHeight(NBattle,Node.Position.X,Node.Position.Y,Floor)){RemoveReceivingBayFixtures();return false;}
            auto* Fixture=NBattle->GetWorld()->SpawnActor<AActor>();if(!Fixture){RemoveReceivingBayFixtures();return false;}
            ReceivingBayFixtures.Add(Fixture);Fixture->Tags.Add(TEXT("FRRU_V82ControlledCollisionReview"));Fixture->SetActorHiddenInGame(true);
            auto* Box=NewObject<UBoxComponent>(Fixture,TEXT("ActualReceivingBayBlocker"));if(!Box){RemoveReceivingBayFixtures();return false;}
            Fixture->AddInstanceComponent(Box);Fixture->SetRootComponent(Box);Box->SetBoxExtent(FVector(Node.Radius+120,Node.Radius+120,400));
            Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->RegisterComponent();Fixture->SetActorLocation(FVector(Node.Position.X,Node.Position.Y,Floor+250));
        }
        CollisionFixtureCreated=!ReceivingBayFixtures.IsEmpty();
        if(CollisionFixtureCreated)
        {
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetBoolField(TEXT("controlledCollisionFixture"),true);
            Evidence->SetNumberField(TEXT("registeredReceivingBayBoxes"),ReceivingBayFixtures.Num());Evidence->SetBoolField(TEXT("registryWritten"),false);
            Evidence->SetBoolField(TEXT("nativeGeometryWritten"),false);Evidence->SetBoolField(TEXT("actorsTeleported"),false);
            RequiredEvent(TEXT("wreck_receiving_bays_physically_blocked"),TEXT("controlled QueryOnly WorldStatic collision fixture in exact disposable NoSave review"),Evidence);
        }
        return CollisionFixtureCreated;
    }
    void ObservePassengers(AFrontlineTank* Vehicle,TMap<int32,FObservedPassengerWindow>& Windows,bool NaturalFirst)
    {
        auto* NBattle=Battle();if(!NBattle||!Vehicle)return;
        for(AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit)
        {
            FFRRUTrainPassengerStateV82 State;if(!WarfareFRRUArmourV82::PassengerState(Unit,State)||State.CarrierSerial!=Vehicle->Serial)continue;
            const int32 Person=State.SquadId*16+State.MemberIndex;auto& Window=Windows.FindOrAdd(Person);
            if(Window.Serial==INDEX_NONE)
            {
                Window.Actor=Unit;Window.Serial=State.CarrierSerial;Window.Squad=State.SquadId;Window.Member=State.MemberIndex;
                Window.Car=State.Car;Window.Seat=State.Seat;Window.FirstAt=NBattle->Elapsed;
            }
            Window.IdentityValid&=Window.Actor.Get()==Unit&&Window.Serial==State.CarrierSerial&&Window.Squad==Unit->SquadId
                &&Window.Member==Unit->MemberIndex&&Window.Car==State.Car&&Window.Seat==State.Seat;
            const int32 Stage=int32(State.Stage);Window.Stages|=1u<<Stage;
            Window.EverActiveWhileAboard|=State.Embarked&&Unit->IsActiveCombatant();
            UAnimSingleNodeInstance* PoseNode=Unit->Soldier?Unit->Soldier->GetSingleNodeInstance():nullptr;
            UAnimSequence* ActualClip=PoseNode?Cast<UAnimSequence>(PoseNode->GetCurrentAsset()):nullptr;
            const bool InteriorPhase=State.Stage==EFRRUTrainPassengerStageV82::Seated||State.Progress>.01f&&State.Progress<.99f;
            if(State.Embarked&&InteriorPhase&&State.Stage!=EFRRUTrainPassengerStageV82::GroundClear)
            {
                const FString Semantic=State.Stage==EFRRUTrainPassengerStageV82::Seated?TEXT("Seated"):State.Stage==EFRRUTrainPassengerStageV82::Standing?TEXT("Stand")
                    :State.Stage==EFRRUTrainPassengerStageV82::Steps?TEXT("Stairs"):TEXT("CarryWalk");
                const FString ClipName=TEXT("A_AH_Train_")+Semantic+TEXT("67_V82");
                const FString ExpectedPath=TEXT("/Game/FRRUIntegration/FormalV82/Motions/Train67Correction1/")+ClipName+TEXT(".")+ClipName;
                bool Finite=Unit->Soldier&&Unit->Soldier->GetSkeletalMeshAsset()&&Unit->Soldier->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum()==67;
                for(const TCHAR* Bone:{TEXT("pelvis"),TEXT("head"),TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r")})
                    Finite&=Unit->Soldier&&!Unit->Soldier->GetBoneTransform(FName(Bone),RTS_World).ContainsNaN();
                const bool Accepted=ActualClip&&ActualClip->GetPathName()==ExpectedPath&&ActualClip->IsCompressedDataValid()&&Finite;
                Window.PoseValid&=Accepted;if(Accepted)Window.MotionStages|=Semantic==TEXT("Seated")?1u:Semantic==TEXT("Stand")?2u:Semantic==TEXT("CarryWalk")?4u:8u;
            }
            if(State.Stage!=EFRRUTrainPassengerStageV82::Field&&State.Stage!=EFRRUTrainPassengerStageV82::Lost)
                Window.ContactValid&=FVector::Dist2D(Unit->Feet(),State.WorldFeet)<=.1&&FMath::Abs(Unit->Feet().Z-State.WorldFeet.Z)<=.1;
            if(State.Stage==EFRRUTrainPassengerStageV82::GroundClear
                ||State.Stage==EFRRUTrainPassengerStageV82::Field&&Window.FirstFieldAt<0)
            {
                float Floor=0;const bool HasFloor=WarfareFRRUWorldV82::TryHeight(NBattle,Unit->Feet().X,Unit->Feet().Y,Floor);
                Window.ContactValid&=HasFloor&&Unit->Feet().Z>=Floor-.1&&Unit->Feet().Z<=Floor+3.1;
            }
            if(Stage!=Window.LastStage)
            {
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_TRAIN_STAGE serial=%d member=%d car=%d seat=%d stage=%d host=%.3f feet=%s pose=%d contact=%d"),
                    Vehicle->Serial,Unit->MemberIndex,State.Car,State.Seat,Stage,NBattle->Elapsed,*Unit->Feet().ToString(),int(Window.PoseValid),int(Window.ContactValid));
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("serial"),Vehicle->Serial);Evidence->SetNumberField(TEXT("person"),Person);
                Evidence->SetNumberField(TEXT("car"),State.Car);Evidence->SetNumberField(TEXT("seat"),State.Seat);Evidence->SetNumberField(TEXT("stage"),Stage);
                Evidence->SetStringField(TEXT("actor"),Unit->GetPathName());Evidence->SetStringField(TEXT("localFeet"),State.LocalFeet.ToString());
                Evidence->SetStringField(TEXT("worldFeet"),State.WorldFeet.ToString());Evidence->SetStringField(TEXT("actualFeet"),Unit->Feet().ToString());
                Evidence->SetNumberField(TEXT("phase"),State.Progress);Evidence->SetNumberField(TEXT("stageStartedAt"),State.StageStartedAt);
                Evidence->SetBoolField(TEXT("alive"),State.Alive);Evidence->SetBoolField(TEXT("embarked"),State.Embarked);
                Evidence->SetBoolField(TEXT("activeCombatant"),Unit->IsActiveCombatant());Evidence->SetBoolField(TEXT("actualFieldEntered"),Unit->bFieldEntered);
                Evidence->SetStringField(TEXT("actualDisplayedClip"),ActualClip?ActualClip->GetPathName():FString());
                Evidence->SetNumberField(TEXT("actualDisplayedClipTime"),PoseNode?PoseNode->GetCurrentTime():-1.f);
                Evidence->SetBoolField(TEXT("allObservedActualPoseFiniteAndDeclared"),Window.PoseValid);
                RequiredEvent(TEXT("actual_train_passenger_stage"),NaturalFirst?TEXT("natural first carrier: real six actors and authoritative host phases")
                    :TEXT("second naturally recruited carrier; controlled receiving-bay collision/damage window"),Evidence);
                Window.LastStage=Stage;
            }
            if(State.Stage==EFRRUTrainPassengerStageV82::Field&&Window.FirstFieldAt<0)
            {Window.FirstFieldAt=NBattle->Elapsed;Window.AliveAtFirstField=Unit->IsAlive();}
            Window.PreviousFeet=Unit->Feet();Window.PreviousAt=NBattle->Elapsed;
        }
    }
    void TickTrainWindows()
    {
        auto* NBattle=Battle();auto* First=ReviewVehicle.Get();if(Scenario->Probe!=EProbe::Train||!NBattle)return;
        // A completed first-carrier lifecycle survives its normal return and
        // destruction. It must not block the later, independently recruited car.
        if(!NaturalSixPassengerChecked&&!First)return;
        if(First)
        {
            if(FirstNaturalTrainSerial==INDEX_NONE)FirstNaturalTrainSerial=First->Serial;
            ObservePassengers(First,NaturalPassengers,true);
            if(TrainParkedAt<0&&FMath::Abs(First->GetActorLocation().X-7150)<.1)TrainParkedAt=NBattle->Elapsed;
        }
        const uint32 RequiredStages=(1u<<int32(EFRRUTrainPassengerStageV82::Seated))|(1u<<int32(EFRRUTrainPassengerStageV82::Standing))
            |(1u<<int32(EFRRUTrainPassengerStageV82::Aisle))|(1u<<int32(EFRRUTrainPassengerStageV82::Queue))|(1u<<int32(EFRRUTrainPassengerStageV82::Sill))
            |(1u<<int32(EFRRUTrainPassengerStageV82::Steps))|(1u<<int32(EFRRUTrainPassengerStageV82::GroundClear))|(1u<<int32(EFRRUTrainPassengerStageV82::Field));
        if(!NaturalSixPassengerChecked&&First&&NaturalPassengers.Num()==6)
        {
            bool AllField=true,Identities=true,Contacts=true,AboardInactive=true;TSet<TWeakObjectPtr<AWarfareFrontlineSoldier>> Unique;
            for(const auto& Entry:NaturalPassengers)
            {
                const auto& Window=Entry.Value;AllField&=(Window.Stages&RequiredStages)==RequiredStages&&Window.FirstFieldAt>=0;
                // A real passenger must leave the train alive. A later combat
                // casualty does not erase the completed physical lifecycle.
                Identities&=Window.IdentityValid&&Window.AliveAtFirstField;Contacts&=Window.ContactValid&&Window.PoseValid&&Window.MotionStages==15u;
                AboardInactive&=!Window.EverActiveWhileAboard;Unique.Add(Window.Actor);
            }
            if(AllField)
            {
                NaturalSixPassengerChecked=Identities&&Contacts&&AboardInactive&&Unique.Num()==6&&WarfareFRRUArmourV82::Passengers(First)==0;
                Check(TEXT("natural_train_six_unique_same_actors_seat_door_steps_ground_field"),NaturalSixPassengerChecked);
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("serial"),First->Serial);Evidence->SetNumberField(TEXT("uniqueActors"),Unique.Num());
                Evidence->SetNumberField(TEXT("parkedAtHost"),TrainParkedAt);Evidence->SetBoolField(TEXT("allRequiredActualStagesObserved"),AllField);
                Evidence->SetBoolField(TEXT("contactStateEqualsRealFeet"),Contacts);Evidence->SetBoolField(TEXT("noActiveCombatantAboard"),AboardInactive);
                Evidence->SetBoolField(TEXT("allSixAliveOnActualFirstFieldTransition"),Identities);
                RequiredEvent(TEXT("natural_train_first_six_lifecycle"),TEXT("natural physical lifecycle; no passenger pose, position, clock or victory grants"),Evidence);
                if(!NaturalSixPassengerChecked)return;
            }
        }
        if(!NaturalSixPassengerChecked||!FirstFailure.IsEmpty())return;
        // The second carrier's completed proof remains valid after its wreck
        // actor expires. Do not recruit another carrier or recreate its fixture.
        if(WreckRetryChecked)return;
        if(!RetryTrain.IsValid())
        {
            if(!WarfareFRRUArmourV82::BlockReason(NBattle,TEXT("at-hu.armoured_train"),1).IsEmpty())return;
            auto* Fleet=UFrontlineVehicleSubsystem::Get(NBattle->GetWorld());if(!Fleet)return;TSet<int32> Before;
            for(AFrontlineTank* Vehicle:Fleet->GetVehicles())if(Vehicle)Before.Add(Vehicle->Serial);
            const bool Recruited=WarfareFRRUArmourV82::Recruit(NBattle,TEXT("at-hu.armoured_train"),1);Event(TEXT("second_real_train_for_controlled_wreck_window"),Recruited);
            if(!Recruited)return;
            for(AFrontlineTank* Vehicle:Fleet->GetVehicles())if(Vehicle&&!Before.Contains(Vehicle->Serial)&&WarfareFRRUArmourV82::Id(Vehicle)==TEXT("at-hu.armoured_train"))
            {RetryTrain=Vehicle;RetryTrainSerial=Vehicle->Serial;break;}
            Check(TEXT("second_train_is_a_real_new_serial_not_copied_carrier"),RetryTrain.IsValid()&&RetryTrainSerial!=FirstNaturalTrainSerial);
            if(RetryTrain.IsValid())Check(TEXT("controlled_receiving_bay_fixture_created"),CreateReceivingBayFixtures());
            return;
        }
        auto* Carrier=RetryTrain.Get();ObservePassengers(Carrier,WreckPassengers,false);
        if(!CollisionFixtureCreated&&!WreckHitIssued)return;
        if(!WreckHitIssued)
        {
            AWarfareFrontlineSoldier* Survivor=nullptr;int32 Aboard=0;
            for(const auto& Entry:WreckPassengers)if(auto* Unit=Entry.Value.Actor.Get();Unit&&Unit->IsAlive())
            {FFRRUTrainPassengerStateV82 State;if(!WarfareFRRUArmourV82::PassengerState(Unit,State))continue;
                if(State.Stage==EFRRUTrainPassengerStageV82::GroundClear&&!Survivor)Survivor=Unit;
                if(State.Embarked&&State.Stage!=EFRRUTrainPassengerStageV82::GroundClear)++Aboard;}
            if(!Survivor||Aboard<=0)return;
            if(GroundWaitAt<0){GroundWaitAt=NBattle->Elapsed;GroundSurvivorPerson=Survivor->SquadId*16+Survivor->MemberIndex;GroundSurvivorFeet=Survivor->Feet();GroundSurvivorShots=Survivor->ShotsDischarged;return;}
            if(NBattle->Elapsed<GroundWaitAt+.75f)return;
            const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Survivor,false);
            Check(TEXT("ground_clear_actual_collision_refusal_has_no_field_route_or_firing"),!Diagnosis->GetBoolField(TEXT("hasRoute"))
                &&Survivor->ShotsDischarged==GroundSurvivorShots&&!Survivor->bFieldEntered&&FVector::Dist2D(Survivor->Feet(),GroundSurvivorFeet)<.1);
            const float MoraleBefore=NBattle->Morale[Carrier->Team];const int32 CasualtiesBefore=NBattle->Casualties[Carrier->Team];
            WreckMoraleLossBefore=FrontlineMoraleV36::Telemetry(NBattle).Loss[Carrier->Team][6];
            FFrontlineKillSourceV65 Source;Source.Team=0;Carrier->ReceiveModuleHit(EFrontlineTankModule::Hull,Carrier->GetHullIntegrity()+1,0,Source);
            WreckAt=NBattle->Elapsed;WreckHitIssued=true;WreckMoraleLossAfter=FrontlineMoraleV36::Telemetry(NBattle).Loss[Carrier->Team][6];
            const bool Alive=Survivor->IsAlive();FFRRUTrainPassengerStateV82 State;
            const bool SameGround=Alive&&WarfareFRRUArmourV82::PassengerState(Survivor,State)&&State.CarrierSerial==RetryTrainSerial
                &&State.Stage==EFRRUTrainPassengerStageV82::GroundClear&&!State.Embarked;
            Check(TEXT("actual_destroyed_carrier_kills_only_remaining_aboard_and_retains_ground_survivor"),!Carrier->IsOperational()&&SameGround
                &&NBattle->Casualties[Carrier->Team]-CasualtiesBefore==Aboard);
            const float ExpectedShock=FMath::Min(24.f,FMath::Max(0.f,MoraleBefore-Aboard*FrontlineMoraleV36::CasualtyLoss));
            // OnArmourDestroyed applies 24 for an occupied train, capped at
            // morale remaining after the actual passenger casualties.
            Check(TEXT("actual_train_wreck_high_morale_loss_once"),FMath::IsNearlyEqual(WreckMoraleLossAfter-WreckMoraleLossBefore,ExpectedShock,.001f));
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetBoolField(TEXT("controlledCollisionFixture"),true);Evidence->SetBoolField(TEXT("controlledPublicHullModuleHit"),true);
            Evidence->SetNumberField(TEXT("serial"),Carrier->Serial);Evidence->SetNumberField(TEXT("remainingAboardBeforeWreck"),Aboard);
            Evidence->SetNumberField(TEXT("survivingPerson"),GroundSurvivorPerson);Evidence->SetNumberField(TEXT("refusedHostSeconds"),WreckAt-GroundWaitAt);
            Evidence->SetNumberField(TEXT("armourDestroyedMoraleLoss"),WreckMoraleLossAfter-WreckMoraleLossBefore);
            Evidence->SetNumberField(TEXT("expectedCappedArmourDestroyedMoraleLoss"),ExpectedShock);
            RequiredEvent(TEXT("actual_wreck_retains_unregistered_ground_survivor"),TEXT("controlled collision and public destruction; real existing actor, actual casualties and morale"),Evidence);
            return;
        }
        const auto* SurvivorWindow=WreckPassengers.Find(GroundSurvivorPerson);auto* Survivor=SurvivorWindow?SurvivorWindow->Actor.Get():nullptr;
        if(!Survivor)return;
        FFRRUTrainPassengerStateV82 State;if(!WarfareFRRUArmourV82::PassengerState(Survivor,State))return;
        if(!WreckDuplicateChecked)
        {
            const float Before=FrontlineMoraleV36::Telemetry(NBattle).Loss[Carrier->Team][6];const int32 CasualtiesBefore=NBattle->Casualties[Carrier->Team];
            Carrier->ReceiveModuleHit(EFrontlineTankModule::Hull,1);
            WreckDuplicateChecked=FrontlineMoraleV36::Telemetry(NBattle).Loss[Carrier->Team][6]==Before&&NBattle->Casualties[Carrier->Team]==CasualtiesBefore;
            Check(TEXT("destroyed_carrier_repeat_hit_does_not_repeat_crew_loss_or_morale"),WreckDuplicateChecked);
        }
        if(CollisionFixtureCreated&&NBattle->Elapsed>=WreckAt+.75f)
        {
            const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Survivor,false);
            Check(TEXT("wreck_keeps_actual_ground_survivor_pending_until_real_bay_available"),State.Alive&&State.Stage==EFRRUTrainPassengerStageV82::GroundClear
                &&!State.Embarked&&!Diagnosis->GetBoolField(TEXT("hasRoute"))
                &&FVector::Dist2D(Survivor->Feet(),GroundSurvivorFeet)<.1);
            RemoveReceivingBayFixtures();Event(TEXT("remove_controlled_receiving_bay_colliders"),true);return;
        }
        if(!CollisionFixtureCreated&&!WreckRetryChecked&&State.Stage==EFRRUTrainPassengerStageV82::Field)
        {
            const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Survivor,false);
            WreckRetryChecked=State.Alive&&State.CarrierSerial==RetryTrainSerial&&Survivor->SquadId*16+Survivor->MemberIndex==GroundSurvivorPerson
                &&Diagnosis->GetBoolField(TEXT("hasRoute"))&&WreckDuplicateChecked;
            Check(TEXT("wreck_retries_real_registration_and_publishes_same_ground_actor_field"),WreckRetryChecked);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("serial"),RetryTrainSerial);Evidence->SetNumberField(TEXT("sameSurvivorPerson"),GroundSurvivorPerson);
            Evidence->SetStringField(TEXT("sameActor"),Survivor->GetPathName());Evidence->SetNumberField(TEXT("actualHostSecondsAfterWreck"),NBattle->Elapsed-WreckAt);
            Evidence->SetBoolField(TEXT("actualFieldRouteRegistered"),Diagnosis->GetBoolField(TEXT("hasRoute")));Evidence->SetBoolField(TEXT("retryCountInvented"),false);
            RequiredEvent(TEXT("actual_wreck_ground_registration_retry"),TEXT("natural tick retry after reversible collision fixture removal; no actor or native registry write"),Evidence);
        }
    }

    bool FinalContestCleared=false;
    bool GermanA7VCore=false,GermanBeuteCore=false;
    TSet<int32> GermanContactSerials;
    bool JoinMotionObservedV826=false,JoinMotionValidV826=true;
    int32 JoinMotionSamplesV826=0,JoinDeploymentsV826=0,JoinPacksV826=0;
    float JoinStartedV826=-1.f;
    FVector JoinFeetV826=FVector::ZeroVector;
    TWeakObjectPtr<AWarfareFrontlineSoldier> JoinGunnerV826;
    bool FormationJoinOnlyV826()const
    {return Scenario&&Scenario->Probe==EProbe::Reserve&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV826FormationJoinOnly"));}
    void ObserveFormationJoinV826()
    {
        if(!FormationJoinOnlyV826()||JoinMotionObservedV826)return;
        auto* Unit=Member(Observer);auto* NBattle=Battle();
        auto* Gun=Unit?FrontlineMachinegunsV30::Get(Unit):nullptr;
        if(!Unit||!Unit->IsAlive()||!Unit->IsMachineGunner()||!NBattle||!Gun)return;
        auto* Pose=Unit->Soldier?Unit->Soldier->GetSingleNodeInstance():nullptr;
        const UAnimationAsset* Clip=Pose?Pose->GetCurrentAsset():nullptr;
        const bool Waiting=WarfareFRRUWorldV82::WaitingToJoinFormation(Unit);
        if(Waiting&&Gun->State==EFrontlineGunStanceV30::Carry&&!Unit->bEngaging&&!Unit->bMoving)
        {
            if(JoinStartedV826<0)
            {JoinStartedV826=NBattle->Elapsed;JoinGunnerV826=Unit;JoinFeetV826=Unit->Feet();JoinDeploymentsV826=Gun->Deployments;JoinPacksV826=Gun->Packs;}
            if(NBattle->Elapsed-JoinStartedV826<.35f)return;
            ++JoinMotionSamplesV826;
            JoinMotionValidV826&=JoinGunnerV826.Get()==Unit&&FVector::Dist(Unit->Feet(),JoinFeetV826)<.1f
                &&Gun->Deployments==JoinDeploymentsV826&&Gun->Packs==JoinPacksV826
                &&Clip==WarfareFRRUPresentationV82::Motion(Unit,TEXT("Idle"));
        }
        else if(JoinMotionSamplesV826>=4&&JoinGunnerV826.Get()==Unit&&!Waiting&&Unit->bMoving
            &&Gun->State==EFrontlineGunStanceV30::Carry&&FVector::Dist2D(Unit->Feet(),JoinFeetV826)>25.f)
        {
            JoinMotionObservedV826=true;
            const bool TravelClip=Clip==Unit->GetWalkMotion()||Clip==Unit->GetRunMotion();
            Check(TEXT("maxim_actual_join_wait_carry_idle_then_same_actor_real_travel"),JoinMotionValidV826&&TravelClip);
            auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("sameActor"),Unit->GetPathName());
            E->SetNumberField(TEXT("waitSamples"),JoinMotionSamplesV826);E->SetNumberField(TEXT("travelCm"),FVector::Dist2D(Unit->Feet(),JoinFeetV826));
            E->SetBoolField(TEXT("waitPoseAndStanceStable"),JoinMotionValidV826);E->SetBoolField(TEXT("actualLocomotionClip"),TravelClip);
            RequiredEvent(TEXT("formation_join_carry_idle"),TEXT("actual registered join wait, ordinary public route release and accepted same-actor travel; no state or clock injection"),E);
        }
    }
    bool GermanCoreOnly()const
    {return Scenario&&Scenario->Probe==EProbe::FT&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanContactCoreOnly"));}
    TArray<TSharedPtr<FJsonValue>> MissingRareWindows()const
    {
        TArray<TSharedPtr<FJsonValue>> Missing;auto Need=[&](const TCHAR* Name,bool Complete){if(!Complete)Missing.Add(MakeShared<FJsonValueString>(Name));};
        if(!Scenario)return Missing;
        if(HoldDepartureProbeV87)
        {Need(TEXT("departed_frozen_owner_held_and_released"),OrdinaryHoldPreservedV827);
            Need(TEXT("original_registered_receiver_checkpoint_physically_completed"),HoldDepartureCheckpointV87);
            Need(TEXT("original_target_actual_arrival_receipt"),HoldDepartureArrivedV87);return Missing;}
        if(Scenario->Probe==EProbe::FrenchPlan)
        {
            Need(TEXT("chemin_actual_observation_commit_and_early_refusals"),PlanReviewV87.Step>=2&&PlanReviewV87.EarlyFollowupRefused);
            Need(TEXT("chemin_four_actual_salvo_receipts"),PlanReviewV87.Receipts.Num()==4);
            Need(TEXT("chemin_chosen_policy_assigns_two_legal_real_routes"),PlanReviewV87.RoutesAccepted);
            Need(TEXT("chemin_both_released_sections_physically_move"),PlanReviewV87.Moved);return Missing;
        }
        if(Scenario->Probe==EProbe::Ordinary)
        {Need(TEXT("ordinary_public_recruitment_leaves_home_without_move_order"),OrdinaryAdvanceSeenV827);
            Need(TEXT("ordinary_manual_hold_remains_authoritative"),OrdinaryHoldPreservedV827);
            Need(TEXT("ordinary_first_stage_actual_completion"),OrdinaryFirstCompletedV827);
            if(FString(Scenario->Mission)==TEXT("ru.riga"))Need(TEXT("ordinary_withdrawal_actual_participant_arrival"),OrdinaryWithdrawalArrivedV827);
            return Missing;}
        if(FormationJoinOnlyV826())
        {Need(TEXT("actual_maxim_registered_wait_then_carry_travel"),JoinMotionObservedV826&&JoinMotionValidV826);return Missing;}
        if(GermanCoreOnly())
        {Need(TEXT("german_a7v_actual_contact_travel_cannon_and_machinegun"),GermanA7VCore);
            Need(TEXT("german_beute_actual_contact_travel_cannon_and_machinegun"),GermanBeuteCore);return Missing;}
        if(Scenario->Probe==EProbe::Schilt)
        {Need(TEXT("schilt_natural_brace_jet_recovery_enemy_burn_pause"),FlameBrace&&FlameJet&&FlameRecovery&&FlameEnemyBurn&&FlamePauseDone);
            Need(TEXT("schilt_twelve_natural_discharges_and_empty_refusal"),FlameShotsObserved==12&&EmptyRefusalChecked);Need(TEXT("schilt_independent_real_once_only_reservoir_burst"),RuptureChecked);}
        if(Scenario->Probe==EProbe::Reserve)
        {Need(TEXT("ru_two_public_prerequisite_purchases"),ReserveAxesChecked);Need(TEXT("ru_first_real_four_second_echelon"),FirstReserveEarlyChecked&&ReserveReleased);
            Need(TEXT("ru_second_real_six_second_echelon"),SecondReserveEarlyChecked&&SecondReserveReleased&&ReserveIdentityChecked);Need(TEXT("ru_actual_clearance_completion"),ClearanceCompleted);
            Need(TEXT("ru_maxim_natural_reload"),MaximReloadChecked);Need(TEXT("ah_m95_natural_reload"),M95ReloadChecked);}
        if(IsVehicleCase())
        {Need(TEXT("vehicle_natural_roll_and_fire"),VehicleRolled&&VehicleShot);
            if(RequiresVehicleCannon())Need(TEXT("vehicle_natural_actual_cannon_fire"),VehicleCannonShot);
            if(RequiresVehicleMachinegun())Need(TEXT("vehicle_natural_actual_machinegun_fire"),VehicleMachinegunShot);
            Need(TEXT("vehicle_actual_same_frame_90_180_bone_muzzle"),VehiclePoseChecked);
            Need(TEXT("vehicle_applicable_running_gear_public_repair_or_correct_refusal"),VehicleTrackChecked);Need(TEXT("vehicle_applicable_mg_disable_and_no_classic_repair"),VehicleGunChecked);}
        if(Scenario->Probe==EProbe::Train)
        {Need(TEXT("train_first_six_unique_actual_lifecycle"),NaturalSixPassengerChecked);Need(TEXT("train_actual_ground_survivor_wreck_retry"),WreckRetryChecked);}
        if(Scenario->Probe==EProbe::FT)
        {Need(TEXT("actual_final_straddler_blocks"),RealStraddler);Need(TEXT("actual_final_operational_hostile_hull_blocks"),RealHullContest);Need(TEXT("actual_final_contest_then_cleared"),FinalContestCleared);
            Need(TEXT("ft_actual_asymmetric_public_explosion_footprint"),FTBlastFootprintCheckedV826);}
        return Missing;
    }
    AWarfareFrontlineGameMode* Battle()const
    {auto* NController=Controller.Get();return NController?FrontlineFor(NController):nullptr;}
    void Check(const FString& Name,bool Passed,const FString& Detail=FString())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Passed);Row->SetStringField(TEXT("detail"),Detail);
        Checks.Add(MakeShared<FJsonValueObject>(Row));
        if(!Passed&&FirstFailure.IsEmpty())FirstFailure=Name+TEXT(": ")+Detail;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_NATURAL_CHECK %s %s %s"),*Name,Passed?TEXT("PASS"):TEXT("FAIL"),*Detail);
    }
    bool Fail(const FString& Name,const FString& Detail)
    {
        if(Name==TEXT("ordinary_actual_route_makes_no_physical_progress")||Name==TEXT("mandatory_ingress_starved_by_idle_home_residents"))FailureClassV87=TEXT("physical_liveness_failure");
        else if(Name==TEXT("chemin_plan_worker_lost_before_clearance")||Name==TEXT("chemin_plan_participant_lost_before_commit")||Name==TEXT("chemin_plan_participant_lost_before_followup")||Name==TEXT("vb_actual_operator_lost_before_role_cycle_complete")||Name==TEXT("vb_actual_pioneer_lost_before_clearance"))FailureClassV87=TEXT("natural_combat_window_loss");
        else if(Name==TEXT("chemin_plan_window_lost_to_native_outcome"))FailureClassV87=TEXT("native_operation_defeat");
        else if(Name==TEXT("chemin_followup_route_did_not_start_real_movement"))FailureClassV87=TEXT("physical_liveness_failure");
        else if(Name==TEXT("ordinary_objective_scope_not_completed"))
        {
            auto* B=Battle();const auto V=WarfareFRRUWorldV82::View(B);const auto D=WarfareFRRUWorldV82::ObjectiveDiagnosis(B);
            FailureClassV87=V.Finished&&!V.Success?TEXT("native_operation_defeat"):TEXT("unexpected_operation_phase");
            double Deadline=0;if(V.Finished&&!V.Success&&B&&D->TryGetNumberField(TEXT("deadline"),Deadline)&&B->Elapsed>=Deadline)FailureClassV87=TEXT("native_stage_deadline");
        }
        else if(Name==TEXT("vb_natural_contact_inconclusive"))FailureClassV87=TEXT("inconclusive_natural_contact");
        else if(Name==TEXT("vb_stationary_visible_in_range_without_first_shot"))FailureClassV87=TEXT("observed_weapon_window_no_discharge");
        else FailureClassV87=TEXT("review_or_asset_failure");
        Check(Name,false,Detail);Finish();return !Finished;
    }
    void SetStep(EStep Next){Step=Next;StepAt=FPlatformTime::Seconds();}
    void Event(const FString& Name,bool Accepted)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("command"),Name);Row->SetBoolField(TEXT("accepted"),Accepted);
        Row->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-Started);if(auto* NBattle=Battle())Row->SetNumberField(TEXT("hostElapsed"),NBattle->Elapsed);
        Commands.Add(MakeShared<FJsonValueObject>(Row));
        if(Scenario->Probe==EProbe::Reserve)
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_NATURAL_EVENT command=%s accepted=%d host=%.3f"),*Name,Accepted,Battle()?Battle()->Elapsed:0.f);
    }
    bool FrameTripoCloseV87(int32 Solid,double Now)
    {
        auto* PC=Controller.Get();auto* B=Battle();
        auto* Camera=PC&&PC->BattleCamera?PC->BattleCamera->GetCameraComponent():nullptr;
        if(!PC||!B||!Camera||Camera->ProjectionMode!=ECameraProjectionMode::Orthographic)
            return Fail(TEXT("tripo_close_actual_camera_available"),TEXT("The existing orthographic battle camera is required."));
        // First settle the requested width; after the one analytic correction,
        // retain the existing 1.5 second dwell before projecting/capturing again.
        if(Now-StepAt<1.5||FMath::Abs(PC->CameraWidth-1800.f)>.25f
            ||FMath::Abs(Camera->OrthoWidth-1800.f)>.25f)return false;
        UStaticMeshComponent* Subject=nullptr;int32 Matches=0;
        const FName ClusterName(*FString::Printf(TEXT("VauxTripoClusterSolid%d"),Solid));
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("FRRU_V87TripoRuin")))
        {
            TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
            for(auto* Component:Components)if(Component&&Component->IsRegistered()&&Component->IsVisible()
                &&(Component->GetFName()==ClusterName||(Solid==16&&Component->GetFName()==TEXT("VauxTripoRuin02"))))
            {Subject=Component;++Matches;}
        }
        if(Matches!=1||!Subject||Subject->Bounds.Origin.ContainsNaN())
            return Fail(TEXT("tripo_close_exact_visible_subject"),FString::Printf(TEXT("solid=%d matches=%d"),Solid,Matches));
        int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);FVector2D Projected;
        if(VX<=0||VY<=0||!PC->ProjectWorldLocationToScreen(Subject->Bounds.Origin,Projected))
            return Fail(TEXT("tripo_close_actual_subject_projection"),FString::Printf(TEXT("solid=%d"),Solid));
        // Same scaled field rectangle as PlayerTick's camera input: top 78,
        // bottom DockTop, including the current viewport's vertical letterbox.
        const float UIScale=FMath::Min(VX/1920.f,VY/1080.f),OffsetY=(VY-1080.f*UIScale)*.5f;
        const FVector2D Desired(VX*.5,OffsetY+(78.f+DockTop)*UIScale*.5f);
        const FVector2D Error=Projected-Desired;
        if(VisualCloseFramedStageV87==VisualCameraStage)
        {
            if(Error.Size()>2.)return Fail(TEXT("tripo_close_framing_settled"),
                FString::Printf(TEXT("solid=%d actual=(%.3f,%.3f) desired=(%.3f,%.3f)"),Solid,Projected.X,Projected.Y,Desired.X,Desired.Y));
            return true;
        }
        FVector Eye;FRotator Rotation;PC->GetPlayerViewPoint(Eye,Rotation);
        const FRotationMatrix Basis(Rotation);const double CmPerPixel=double(Camera->OrthoWidth)/VX;
        const FVector Shift=Basis.GetUnitAxis(EAxis::Y)*(Error.X*CmPerPixel)
            -Basis.GetUnitAxis(EAxis::Z)*(Error.Y*CmPerPixel);
        if(Shift.ContainsNaN())return Fail(TEXT("tripo_close_finite_camera_correction"),FString());
        PC->CameraFocus+=Shift;PC->CameraPanVelocity=PC->CameraVerticalVelocity=0;
        VisualCloseFramedStageV87=VisualCameraStage;StepAt=Now;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_TRIPO_CLOSE_FRAME solid=%d anchor=(%s) before=(%.3f,%.3f) desired=(%.3f,%.3f) shift=(%s) width=%.3f"),
            Solid,*Subject->Bounds.Origin.ToString(),Projected.X,Projected.Y,Desired.X,Desired.Y,*Shift.ToString(),Camera->OrthoWidth);
        return false;
    }
    bool Capture(const FString& Name)
    {
        if(CompletedCaptures.Contains(Name))return true;
        const auto* NController=Controller.Get();const auto* HUD=NController?Cast<AWarfareFrontlineHUD>(NController->GetHUD()):nullptr;
        if(PendingCaptureName==Name)
        {
            if(FScreenshotRequest::IsScreenshotRequested()||FPlatformTime::Seconds()-CaptureRequestedAt<.4)return false;
            const int64 Bytes=IFileManager::Get().FileSize(*PendingCapture);FIntPoint Pixels=FIntPoint::ZeroValue;
            TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*PendingCapture));
            if(Reader&&Reader->TotalSize()>=24)
            {
                uint8 PngHeader[24]={};Reader->Serialize(PngHeader,24);
                const uint8 PngSignature[8]={137,80,78,71,13,10,26,10};
                if(!Reader->IsError()&&FMemory::Memcmp(PngHeader,PngSignature,8)==0&&FMemory::Memcmp(PngHeader+12,"IHDR",4)==0)
                {
                    const uint32 PngWidth=(uint32(PngHeader[16])<<24)|(uint32(PngHeader[17])<<16)|(uint32(PngHeader[18])<<8)|PngHeader[19];
                    const uint32 PngHeight=(uint32(PngHeader[20])<<24)|(uint32(PngHeader[21])<<16)|(uint32(PngHeader[22])<<8)|PngHeader[23];
                    if(PngWidth<=MAX_int32&&PngHeight<=MAX_int32)Pixels=FIntPoint(int32(PngWidth),int32(PngHeight));
                }
            }
            const bool Saved=Bytes>10000&&Pixels==FIntPoint(3840,2160)&&CaptureRequestedViewport==FIntPoint(3840,2160);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetStringField(TEXT("path"),PendingCapture);
            Row->SetNumberField(TEXT("bytes"),Bytes);Row->SetBoolField(TEXT("saved"),Saved);Row->SetNumberField(TEXT("requestedPage"),CaptureRequestedPage);
            Row->SetNumberField(TEXT("pngWidth"),Pixels.X);Row->SetNumberField(TEXT("pngHeight"),Pixels.Y);
            Row->SetNumberField(TEXT("actualViewportWidth"),CaptureRequestedViewport.X);Row->SetNumberField(TEXT("actualViewportHeight"),CaptureRequestedViewport.Y);
            Row->SetNumberField(TEXT("completedPage"),HUD?HUD->GetPresentationPage():INDEX_NONE);Row->SetBoolField(TEXT("artAccepted"),false);
            if(Name==TEXT("05-first-warmed-battle")||Name==TEXT("07-final-battle-state"))if(auto* NBattle=Battle())
            {
                Row->SetObjectField(TEXT("sceneRenderingAtCaptureCompletion"),WarfareFRRUContentV82::DescribeSceneRendering(NBattle));
                for(int32 Side=0;Side<2;++Side)
                {
                    Row->SetNumberField(FString::Printf(TEXT("morale%d"),Side),NBattle->Morale[Side]);
                    Row->SetNumberField(FString::Printf(TEXT("moraleMaximum%d"),Side),FrontlineMoraleV36::StartingMaximum(NBattle,Side));
                }
                FrontlineFrontsV76::FReadout Display;
                Row->SetBoolField(TEXT("actualFrontsDisplayAvailable"),FrontlineFrontsV76::Peek(NBattle,Display));
                const int32 ConsoleNation=int32(FrontlineConsoleV79::ResolveDisplayNationV82(NBattle,false));
                const bool GermanConsole=ConsoleNation==1;
                Row->SetBoolField(TEXT("actualGermanConsole"),GermanConsole);
                Row->SetNumberField(TEXT("actualConsoleNation"),ConsoleNation);
                for(int32 Part:{5,6})
                {
                    auto Texture=MakeShared<FJsonObject>();const auto* T=Artwork().ConsoleV79[FMath::Clamp(ConsoleNation,0,3)][Part].Get();
                    Texture->SetNumberField(TEXT("part"),Part);Texture->SetStringField(TEXT("uri"),T?T->GetPathName():FString());
                    Texture->SetBoolField(TEXT("ready"),FrontlineTextureReady(T));Texture->SetBoolField(TEXT("hasResource"),T&&T->GetResource());
                    Texture->SetNumberField(TEXT("width"),T?T->GetSizeX():0);Texture->SetNumberField(TEXT("height"),T?T->GetSizeY():0);
                    Row->SetObjectField(Part==5?TEXT("actualConsoleGauge"):TEXT("actualConsoleFill"),Texture);
                }
                for(int32 Side=0;Side<2;++Side)
                {
                    auto Gauge=MakeShared<FJsonObject>();Gauge->SetNumberField(TEXT("advance"),Display.Advance[Side]);Gauge->SetNumberField(TEXT("advanceDisplay"),Display.AdvanceDisplay[Side]);
                    Gauge->SetNumberField(TEXT("morale"),Display.Morale[Side]);Gauge->SetNumberField(TEXT("moraleDisplay"),Display.MoraleDisplay[Side]);
                    Gauge->SetNumberField(TEXT("advanceWidth"),698*Display.AdvanceDisplay[Side]);Gauge->SetNumberField(TEXT("moraleWidth"),698*Display.MoraleDisplay[Side]);
                    Gauge->SetNumberField(TEXT("advanceUVStartY"),.12);Gauge->SetNumberField(TEXT("moraleUVStartY"),.60);Gauge->SetNumberField(TEXT("uvHeight"),.28);
                    Gauge->SetNumberField(TEXT("drawHeight"),9);Row->SetObjectField(FString::Printf(TEXT("actualGaugeSide%d"),Side),Gauge);
                }
                Row->SetStringField(TEXT("cameraFocusAtCompletion"),NController?NController->CameraFocus.ToString():FString());
            }
            Captures.Add(MakeShared<FJsonValueObject>(Row));Check(TEXT("actual_4k_screenshot_saved.")+Name,Saved,
                FString::Printf(TEXT("viewport=%dx%d PNG=%dx%d %s"),CaptureRequestedViewport.X,CaptureRequestedViewport.Y,Pixels.X,Pixels.Y,*PendingCapture));
            CompletedCaptures.Add(Name);PendingCapture.Reset();PendingCaptureName.Reset();return true;
        }
        if(!PendingCaptureName.IsEmpty()||FScreenshotRequest::IsScreenshotRequested())return false;
        PendingCapture=FPaths::Combine(User,TEXT("FRRUReview/captures"),Name+TEXT(".png"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(PendingCapture),true);PendingCaptureName=Name;
        CaptureRequestedAt=FPlatformTime::Seconds();CaptureRequestedPage=HUD?HUD->GetPresentationPage():INDEX_NONE;
        const auto* NBattle=Battle();const auto* NViewport=NBattle?NBattle->GetWorld()->GetGameViewport():nullptr;
        CaptureRequestedViewport=NViewport&&NViewport->Viewport?NViewport->Viewport->GetSizeXY():FIntPoint::ZeroValue;
        FScreenshotRequest::RequestScreenshot(PendingCapture,true,false);return false;
    }
    int32 Recruit(EFrontlineRole Role,int32 Side)
    {
        auto* NBattle=Battle();if(!NBattle)return INDEX_NONE;
        TSet<int32> Existing;for(const auto& Group:NBattle->Squads)Existing.Add(Group.Id);
        const bool Accepted=NBattle->Recruit(Role,Side);Event(FString::Printf(TEXT("recruit.%d.side%d"),int32(Role),Side),Accepted);
        if(!Accepted)return INDEX_NONE;
        for(const auto& Group:NBattle->Squads)if(Group.Team==Side&&!Existing.Contains(Group.Id))return Group.Id;
        Fail(TEXT("accepted_recruit_has_exact_new_squad"),TEXT("Recruit returned true without a new real squad."));return INDEX_NONE;
    }
    int32 FTObjectiveSquad(const FFRRUWorldViewV82& View)
    {
        auto* NBattle=Battle();if(!NBattle)return INDEX_NONE;
        const EFrontlineRole Role=View.RequiredRole==int32(EFrontlineRole::Pioneer)?EFrontlineRole::Pioneer:EFrontlineRole::Rifle;
        const int32 Original=Role==EFrontlineRole::Pioneer?Second:Lead;
        const auto* OriginalGroup=NBattle->FindSquad(Original);
        if(OriginalGroup&&OriginalGroup->LivingCount()>0)return Original;
        const auto* Replacement=NBattle->FindSquad(FTObjectiveSection);
        if(Replacement&&Replacement->LivingCount()>0&&Replacement->Role==Role)return FTObjectiveSection;
        if(FTObjectiveRecruits>=6||FPlatformTime::Seconds()<FTObjectiveRetryAt||!NBattle->RecruitBlockReason(Role,0).IsEmpty())return INDEX_NONE;
        FTObjectiveRetryAt=FPlatformTime::Seconds()+2.;
        FTObjectiveSection=Recruit(Role,0);
        if(FTObjectiveSection!=INDEX_NONE)
        {
            ++FTObjectiveRecruits;FTObjectiveSections.Add(FTObjectiveSection);
            Commands.Last()->AsObject()->SetBoolField(TEXT("objectiveReplacementAfterActualSectionLoss"),true);
            Commands.Last()->AsObject()->SetNumberField(TEXT("actualSquad"),FTObjectiveSection);
            Commands.Last()->AsObject()->SetNumberField(TEXT("publicReplacementOrdinal"),FTObjectiveRecruits);
        }
        return FTObjectiveSection;
    }
    void TickTrainContactOrders()
    {
        auto* NBattle=Battle();if(!NBattle||Scenario->Probe!=EProbe::Train||UGameplayStatics::IsGamePaused(NBattle))return;
        const double Now=FPlatformTime::Seconds();if(Now<TrainContactRetryAt)return;
        if(TrainContactSquad!=INDEX_NONE)
        {
            const auto* ContactGroup=NBattle->FindSquad(TrainContactSquad);
            if(!ContactGroup||ContactGroup->LivingCount()<=0)
            {
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("squad"),TrainContactSquad);
                Evidence->SetNumberField(TEXT("publicContactRecruits"),TrainContactRecruits);Evidence->SetNumberField(TEXT("lastAcceptedRouteLeg"),TrainContactLeg);
                Evidence->SetBoolField(TEXT("heldAtObjective"),TrainContactHeld);Evidence->SetNumberField(TEXT("actualLivingMembers"),0);
                RequiredEvent(TEXT("train_actual_contact_section_lost"),TEXT("actual ordinary casualties; no resurrection or actor replacement in place"),Evidence);
                TrainContactSquad=INDEX_NONE;TrainContactLeg=0;TrainContactHeld=false;TrainContactRailHeld=false;TrainContactRidgeHeld=false;
            }
        }
        if(TrainContactSquad==INDEX_NONE)
        {
            if(TrainContactRecruits>=TrainContactRecruitLimit)
            {
                if(!TrainContactExhausted)
                {
                    TrainContactExhausted=true;auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("publicContactRecruits"),TrainContactRecruits);
                    Evidence->SetNumberField(TEXT("recruitLimit"),TrainContactRecruitLimit);
                    RequiredEvent(TEXT("train_bounded_contact_recruit_budget_exhausted"),TEXT("three ordinary public sections lost; missing combat windows remain unpassed"),Evidence);
                }
                return;
            }
            if(!NBattle->RecruitBlockReason(EFrontlineRole::Rifle,0).IsEmpty())return;
            TrainContactSquad=Recruit(EFrontlineRole::Rifle,0);TrainContactRetryAt=Now+2.;
            if(TrainContactSquad!=INDEX_NONE)
            {
                ++TrainContactRecruits;TrainContactSquads.Add(TrainContactSquad);TrainContactRidgeHeld=false;
                Check(TEXT("train_contact_is_independent_public_recruit"),TrainContactSquad!=Lead);
                Commands.Last()->AsObject()->SetNumberField(TEXT("actualSquad"),TrainContactSquad);
                Commands.Last()->AsObject()->SetNumberField(TEXT("contactRecruitOrdinal"),TrainContactRecruits);
                auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("squad"),TrainContactSquad);
                Evidence->SetNumberField(TEXT("publicRecruitOrdinal"),TrainContactRecruits);Evidence->SetNumberField(TEXT("recruitLimit"),TrainContactRecruitLimit);
                Evidence->SetBoolField(TEXT("replacementAfterActualSectionLoss"),TrainContactRecruits>1);
                RequiredEvent(TEXT("train_bounded_public_contact_recruit"),TEXT("normal Russian rifle recruitment and shared cooldown; no actor, health or clock grants"),Evidence);
            }
            return;
        }
        if(TrainContactHeld)return;
        // Keep the ordinary section in its safe rear bay until dispatch. The
        // northern forest contains an actual Schwarzlose and the first train's
        // dismounted section; use existing adjacent southern posts instead.
        const TCHAR* Posts[]={TEXT("home"),TEXT("assembly"),TEXT("ridge"),TEXT("front_left"),TEXT("objective")};
        if(TrainContactLeg>=4)
        {
            if(At(TrainContactSquad,Posts[4]))
            {
                TrainContactHeld=WarfareFRRUWorldV82::HoldSquad(NBattle,TrainContactSquad,true);
                Event(TEXT("train_contact_public_hold_at_objective"),TrainContactHeld);TrainContactRetryAt=Now+2.;
                Commands.Last()->AsObject()->SetNumberField(TEXT("actualSquad"),TrainContactSquad);
            }
            return;
        }
        if(At(TrainContactSquad,Posts[TrainContactLeg]))
        {
            if(TrainContactLeg==1&&!TrainContactRailHeld)
            {
                TrainContactRailHeld=WarfareFRRUWorldV82::HoldSquad(NBattle,TrainContactSquad,true);
                Event(TEXT("train_contact_public_hold_at_rear_assembly"),TrainContactRailHeld);TrainContactRetryAt=Now+2.;
                Commands.Last()->AsObject()->SetNumberField(TEXT("actualSquad"),TrainContactSquad);return;
            }
            // Wait for the publicly recruited replacement train to reach its rail post.
            if(TrainContactLeg==1&&(!WreckRetryChecked||NBattle->Elapsed<335.f))return;
            if(TrainContactLeg==2)
            {
                if(!TrainContactRidgeHeld)
                {
                    TrainContactRidgeHeld=WarfareFRRUWorldV82::HoldSquad(NBattle,TrainContactSquad,true);
                    Event(TEXT("train_contact_public_hold_at_ridge_for_actual_third_train"),TrainContactRidgeHeld);TrainContactRetryAt=Now+2.;
                    Commands.Last()->AsObject()->SetNumberField(TEXT("actualSquad"),TrainContactSquad);return;
                }
                // Keep the real contact section alive in its rear bay until the
                // same publicly recruited combat train actually reaches its rail post.
                const auto* Carrier=TrainCombatCarrier.Get();
                if(!Carrier||Carrier->Serial!=TrainCombatSerial||!Carrier->IsOperational()
                    ||!WarfareFRRUArmourV82::Entered(Carrier)||FMath::Abs(Carrier->GetActorLocation().X-7150)>=.1f)return;
            }
            const bool Accepted=WarfareFRRUWorldV82::MoveSquad(NBattle,TrainContactSquad,Posts[TrainContactLeg+1]);
            Event(TEXT("train_contact_southern_public_route_leg"),Accepted);
            Commands.Last()->AsObject()->SetStringField(TEXT("destination"),Posts[TrainContactLeg+1]);
            Commands.Last()->AsObject()->SetNumberField(TEXT("actualSquad"),TrainContactSquad);
            if(Accepted){++TrainContactLeg;TrainContactRailHeld=false;}TrainContactRetryAt=Now+2.;
        }
    }
    void TickTrainCombatRecruit()
    {
        auto* NBattle=Battle();if(!NBattle||Scenario->Probe!=EProbe::Train||TrainCombatRecruitAttempted
            ||!NaturalSixPassengerChecked||!WreckRetryChecked||CollisionFixtureCreated||!ReceivingBayFixtures.IsEmpty()
            ||TrainContactLeg<2||UGameplayStatics::IsGamePaused(NBattle))return;
        const auto* ContactGroup=NBattle->FindSquad(TrainContactSquad);if(!ContactGroup||ContactGroup->LivingCount()<=0)return;
        const int32 LivingContactMembers=ContactGroup->LivingCount();
        if(!WarfareFRRUArmourV82::BlockReason(NBattle,TEXT("at-hu.armoured_train"),1).IsEmpty())return;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(NBattle->GetWorld());if(!Fleet)return;TSet<int32> Before;
        for(AFrontlineTank* Vehicle:Fleet->GetVehicles())if(Vehicle)Before.Add(Vehicle->Serial);
        TrainCombatRecruitAttempted=true;
        const bool Recruited=WarfareFRRUArmourV82::Recruit(NBattle,TEXT("at-hu.armoured_train"),1);
        Event(TEXT("single_third_public_train_for_natural_combat"),Recruited);
        if(!Recruited){Fail(TEXT("contact_ready_public_combat_train_recruit"),TEXT("Ordinary armour recruitment refused despite a clear public block reason; no second attempt or cooldown write."));return;}
        int32 NewCarrierCount=0;
        for(AFrontlineTank* Vehicle:Fleet->GetVehicles())if(Vehicle&&!Before.Contains(Vehicle->Serial)&&Vehicle->Team==1
            &&WarfareFRRUArmourV82::Id(Vehicle)==TEXT("at-hu.armoured_train"))
        {++NewCarrierCount;TrainCombatCarrier=Vehicle;TrainCombatSerial=Vehicle->Serial;}
        const bool Distinct=NewCarrierCount==1&&TrainCombatCarrier.IsValid()&&TrainCombatSerial!=FirstNaturalTrainSerial&&TrainCombatSerial!=RetryTrainSerial;
        Check(TEXT("combat_train_is_one_distinct_actual_public_recruit"),Distinct);
        auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("serial"),TrainCombatSerial);
        Evidence->SetNumberField(TEXT("newActualCarrierCount"),NewCarrierCount);
        Evidence->SetNumberField(TEXT("firstPassengerProofSerial"),FirstNaturalTrainSerial);Evidence->SetNumberField(TEXT("controlledWreckProofSerial"),RetryTrainSerial);
        Evidence->SetNumberField(TEXT("actualContactSquad"),TrainContactSquad);Evidence->SetNumberField(TEXT("actualLivingContactMembers"),LivingContactMembers);
        Evidence->SetNumberField(TEXT("publicContactRecruits"),TrainContactRecruits);Evidence->SetNumberField(TEXT("acceptedSouthernRouteLeg"),TrainContactLeg);
        Evidence->SetBoolField(TEXT("actualAllLivingContactAtObjective"),At(TrainContactSquad,TEXT("objective")));
        Evidence->SetBoolField(TEXT("publicContactHoldAccepted"),TrainContactHeld);Evidence->SetBoolField(TEXT("receivingCollisionFixtureRecreated"),false);
        Evidence->SetBoolField(TEXT("firstSixAndSecondWreckProofsAlreadyCompleted"),NaturalSixPassengerChecked&&WreckRetryChecked);
        RequiredEvent(TEXT("train_separate_third_public_combat_carrier"),TEXT("one ordinary public recruit while the real southern-route section approaches the front; no target, position, health or clock grants"),Evidence);
    }
    bool At(int32 SquadId,const FString& NodeId,bool Still=true)const
    {
        const auto* NBattle=Battle();const auto* Group=NBattle?NBattle->FindSquad(SquadId):nullptr;if(!Group)return false;
        FVector Centre;float Radius=0;if(!WarfareFRRUWorldV82::NodePose(NBattle,NodeId,Centre,Radius))return false;bool Any=false;
        for(const auto& WeakMember:Group->Members)if(const auto* Unit=WeakMember.Get();Unit&&Unit->IsAlive())
        {Any=true;if(!Unit->bFieldEntered||FVector::Dist2D(Unit->Feet(),Centre)>Radius||Still&&(Unit->bMoving||Unit->GetMotionSpeed()>.5f))return false;}
        return Any;
    }
    AWarfareFrontlineSoldier* Member(int32 SquadId,int32 Index=0)const
    {
        const auto* NBattle=Battle();const auto* Group=NBattle?NBattle->FindSquad(SquadId):nullptr;if(!Group)return nullptr;
        for(const auto& Weak:Group->Members)if(auto* Unit=Weak.Get();Unit&&Unit->MemberIndex==Index&&Unit->IsAlive())return Unit;return nullptr;
    }
    void TickVBPublicOrdersV87()
    {
        auto* NBattle=Battle();if(!NBattle||Scenario->Probe!=EProbe::VB)return;
        if(VBPioneerV87==INDEX_NONE&&NBattle->RecruitBlockReason(EFrontlineRole::Pioneer,0).IsEmpty())
        {VBPioneerV87=Recruit(EFrontlineRole::Pioneer,0);VBPioneerRecruitedV87=VBPioneerV87!=INDEX_NONE;}
        const auto View=WarfareFRRUWorldV82::View(NBattle);
        const auto Move=[&](int32 Id,const FString& Target,FString& Last,float& Retry)
        {
            const auto* Group=NBattle->FindSquad(Id);const AWarfareFrontlineSoldier* Witness=nullptr;FString From;
            if(Group)for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsActiveCombatant()&&Unit->bFieldEntered){Witness=Unit;break;}
            if(!Group||!Witness||Group->bAdvancing||Group->bHoldPosition||NBattle->Elapsed<Retry
                ||!WarfareFRRUWorldV82::MovementDiagnosis(Witness,false)->TryGetStringField(TEXT("actualNativeNode"),From)
                ||From.IsEmpty()||From==Target||!At(Id,From))return;
            const FString Key=View.StageId+TEXT("|")+FString::FromInt(Id)+TEXT("|")+From+TEXT("|")+Target;
            if(Last==Key)return;
            const bool Accepted=WarfareFRRUWorldV82::MoveSquad(NBattle,Id,Target);
            Event(Id==VBPioneerV87?TEXT("vb_pioneer_public_route"):TEXT("vb_public_natural_contact_route"),Accepted);
            const auto Evidence=Commands.Last()->AsObject();Evidence->SetNumberField(TEXT("actualSquad"),Id);
            Evidence->SetStringField(TEXT("actualSourceNode"),From);Evidence->SetStringField(TEXT("actualTargetNode"),Target);
            Evidence->SetStringField(TEXT("actualStage"),View.StageId);
            if(!Accepted)Evidence->SetStringField(TEXT("actualRejection"),WarfareFRRUWorldV82::View(NBattle).Notice);
            Retry=NBattle->Elapsed+2.f;if(Accepted)Last=Key;
        };
        const bool Cleared=WarfareFRRUWorldV82::PassageCleared(NBattle,TEXT("wire"));
        if(!Cleared&&View.RequiredRole==int32(EFrontlineRole::Pioneer)&&View.TargetNode==TEXT("wire"))
            Move(VBPioneerV87,TEXT("wire"),VBPioneerOrderV87,VBPioneerRetryAtV87);
        if(Cleared&&!VBPioneerReturnedV87)
        {
            Move(VBPioneerV87,TEXT("assembly"),VBPioneerOrderV87,VBPioneerRetryAtV87);
            const auto* Group=NBattle->FindSquad(VBPioneerV87);const AWarfareFrontlineSoldier* Witness=nullptr;FString Native;
            if(Group)for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsActiveCombatant()&&Unit->bFieldEntered){Witness=Unit;break;}
            if(Group&&!Group->bAdvancing&&!Group->bHoldPosition&&Witness&&At(VBPioneerV87,TEXT("assembly"))
                &&WarfareFRRUWorldV82::MovementDiagnosis(Witness,false)->TryGetStringField(TEXT("actualNativeNode"),Native)&&Native==TEXT("assembly"))
            {VBPioneerReturnedV87=true;Event(TEXT("vb_pioneer_actual_return_to_assembly"),true);}
        }
        // Wait at a rear post without occupying the one-slot front-left work route.
        if(Second!=INDEX_NONE)Move(Second,VBPioneerReturnedV87?TEXT("objective"):TEXT("assembly"),VBAdvanceOrderV87,VBAdvanceRetryAtV87);
    }
    void SampleVB(AWarfareFrontlineSoldier* Unit,FJsonObject& Person)
    {
        if(Scenario->Probe!=EProbe::VB||!Unit||Unit->SquadId!=Second||Unit->MemberIndex!=0)return;
        auto* NBattle=Battle();const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Unit);if(!NBattle||!Equipment)return;
        // Observe this actual operator without selecting or replacing a target.
        // A nearest-enemy candidate explains an empty focus; it is never an order.
        auto FireReadback=MakeShared<FJsonObject>();FireReadback->SetBoolField(TEXT("readOnly"),true);
        FireReadback->SetNumberField(TEXT("hostElapsed"),NBattle->Elapsed);FireReadback->SetStringField(TEXT("primaryActor"),Unit->GetPathName());
        FireReadback->SetNumberField(TEXT("shotClock"),Unit->ShotClock);FireReadback->SetNumberField(TEXT("aimSettleClock"),Unit->AimSettleClock);
        FireReadback->SetNumberField(TEXT("firePoseClock"),Unit->FirePoseClock);FireReadback->SetNumberField(TEXT("departureClock"),Unit->DepartureClock);
        FireReadback->SetNumberField(TEXT("targetReviewClock"),Unit->TargetReviewClock);
        FireReadback->SetBoolField(TEXT("atDestination"),Unit->bAtDestinationV43);FireReadback->SetBoolField(TEXT("wasSettled"),Unit->bWasSettled);
        FireReadback->SetBoolField(TEXT("wantedMovement"),Unit->bWantedMovement);FireReadback->SetBoolField(TEXT("hadAdvanceContact"),Unit->bHadAdvanceContact);
        const FVector Muzzle=Unit->GetMuzzleLocation(),Forward=Unit->GetActorForwardVector();
        const FVector MuzzleForward=WarfareFRRUPresentationV82::MuzzleForward(Unit);
        FireReadback->SetStringField(TEXT("actualMuzzle"),Muzzle.ToString());FireReadback->SetStringField(TEXT("actualActorForward"),Forward.ToString());
        FireReadback->SetStringField(TEXT("actualMuzzleForward"),MuzzleForward.ToString());
        FireReadback->SetBoolField(TEXT("primaryActive"),Unit->IsActiveCombatant());FireReadback->SetBoolField(TEXT("primaryMoving"),Unit->bMoving);
        FireReadback->SetNumberField(TEXT("primarySpeedCm"),Unit->GetMotionSpeed());FireReadback->SetNumberField(TEXT("weaponPhase"),int32(Unit->WeaponPhase));
        FireReadback->SetNumberField(TEXT("visibilityScale"),NBattle->EncounterVisibilityScale());
        const double FireRange=FrontlineWeapons::ProfileFor(Unit).FireRangeCm*NBattle->EncounterVisibilityScale();
        FireReadback->SetNumberField(TEXT("actualFireRangeLimitCm"),FireRange);
        const AWarfareFrontlineSoldier* Focus=Unit->GetObservedInfantryTargetV844();
        const AWarfareFrontlineSoldier* DiagnosticTarget=IsValid(Focus)?Focus:nullptr;
        FireReadback->SetBoolField(TEXT("actualFocusPresent"),IsValid(Focus));
        FireReadback->SetStringField(TEXT("targetSource"),IsValid(Focus)?TEXT("actual_focus"):TEXT("nearest_active_enemy_without_focus"));
        if(!DiagnosticTarget)
        {
            double Nearest=MAX_dbl;
            for(const AWarfareFrontlineSoldier* Candidate:NBattle->Units)if(IsValid(Candidate)&&Candidate->Battle.Get()==NBattle
                &&Candidate->Team!=Unit->Team&&Candidate->IsActiveCombatant())
            {const double Distance=FVector::Dist2D(Unit->Feet(),Candidate->Feet());if(Distance<Nearest){Nearest=Distance;DiagnosticTarget=Candidate;}}
        }
        if(IsValid(DiagnosticTarget))
        {
            FireReadback->SetStringField(TEXT("targetActor"),DiagnosticTarget->GetPathName());FireReadback->SetNumberField(TEXT("targetSquad"),DiagnosticTarget->SquadId);
            FireReadback->SetNumberField(TEXT("targetMember"),DiagnosticTarget->MemberIndex);FireReadback->SetNumberField(TEXT("targetTeam"),DiagnosticTarget->Team);
            FireReadback->SetBoolField(TEXT("targetAlive"),DiagnosticTarget->IsAlive());FireReadback->SetBoolField(TEXT("targetActive"),DiagnosticTarget->IsActiveCombatant());
            FireReadback->SetBoolField(TEXT("targetSameEncounter"),DiagnosticTarget->Battle.Get()==NBattle);
            FireReadback->SetStringField(TEXT("targetFeet"),DiagnosticTarget->Feet().ToString());
            const double RangeDistance=FrontlineWeapons::DirectFireRangeDistanceCm(Unit,DiagnosticTarget->Feet());
            FireReadback->SetNumberField(TEXT("distance2DCm"),FVector::Dist2D(Unit->Feet(),DiagnosticTarget->Feet()));
            FireReadback->SetNumberField(TEXT("actualFireRangeDistanceCm"),RangeDistance);FireReadback->SetBoolField(TEXT("actualInFireRange"),RangeDistance<=FireRange);
            FireReadback->SetBoolField(TEXT("observationSight"),NBattle->HasSight(Unit,DiagnosticTarget));
            FireReadback->SetBoolField(TEXT("worldDischargeLineClear"),WarfareFRRUWorldV82::FiringLine(Unit,DiagnosticTarget));
            const FVector Direction=(DiagnosticTarget->Feet()-Unit->Feet()).GetSafeNormal2D();
            FireReadback->SetNumberField(TEXT("actorForwardDotTarget"),FVector::DotProduct(Forward.GetSafeNormal2D(),Direction));
            FireReadback->SetNumberField(TEXT("muzzleForwardDotTarget"),FVector::DotProduct(MuzzleForward.GetSafeNormal2D(),Direction));
            FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUNaturalVBReadbackV87),true);
            for(const AWarfareFrontlineSoldier* Other:NBattle->Units)if(Other)Query.AddIgnoredActor(Other);
            TArray<TSharedPtr<FJsonValue>> Rays;
            const auto Trace=[&](const TCHAR* Semantic,const FVector& Target)
            {
                auto Ray=MakeShared<FJsonObject>();Ray->SetStringField(TEXT("targetSemantic"),Semantic);Ray->SetStringField(TEXT("target"),Target.ToString());
                Ray->SetBoolField(TEXT("nativeLineClear"),WarfareFRRUWorldV82::HasSight(NBattle,Muzzle,Target));
                FHitResult Hit;const bool Blocked=Unit->GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Target,ECC_Visibility,Query);
                Ray->SetBoolField(TEXT("physicsBlocked"),Blocked);
                if(Blocked)
                {
                    Ray->SetStringField(TEXT("hitActor"),Hit.GetActor()?Hit.GetActor()->GetPathName():FString());
                    Ray->SetStringField(TEXT("hitComponent"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():FString());
                    Ray->SetStringField(TEXT("impact"),Hit.ImpactPoint.ToString());Ray->SetNumberField(TEXT("hitFraction"),Hit.Time);
                    Ray->SetBoolField(TEXT("initialPenetration"),Hit.bStartPenetrating);
                }
                Rays.Add(MakeShared<FJsonValueObject>(Ray));
            };
            Trace(TEXT("world_torso"),DiagnosticTarget->Soldier?DiagnosticTarget->Soldier->GetBoneLocation(TEXT("spine_03")):DiagnosticTarget->Feet()+FVector(0,0,95));
            Trace(TEXT("shared_head"),DiagnosticTarget->Soldier?DiagnosticTarget->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):DiagnosticTarget->Feet()+FVector(0,0,145));
            FireReadback->SetArrayField(TEXT("actualDischargeRays"),Rays);
        }
        Person.SetObjectField(TEXT("actualVBFireReadbackV87"),FireReadback);
        if(Step==EStep::Combat&&!FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture")))
        {
            const auto* Group=NBattle->FindSquad(Second);
            const bool Window=Unit->IsActiveCombatant()&&!Unit->bMoving&&Unit->GetMotionSpeed()<=.5f
                &&Unit->WeaponPhase==EFrontlineWeaponPhase::Ready&&Group&&!Group->bHoldPosition
                &&IsValid(DiagnosticTarget)&&DiagnosticTarget->Battle.Get()==NBattle&&DiagnosticTarget->Team!=Unit->Team
                &&DiagnosticTarget->IsActiveCombatant()&&FrontlineWeapons::DirectFireRangeDistanceCm(Unit,DiagnosticTarget->Feet())<=FireRange
                &&FireReadback->GetBoolField(TEXT("observationSight"));
            VBContactSeenV87|=Window;
            if(Window&&Unit->ShotsDischarged==0)
            {if(VBNoShotSinceV87<0)VBNoShotSinceV87=NBattle->Elapsed;VBNoShotEvidenceV87=FireReadback;}
            else VBNoShotSinceV87=-1;
        }
        Person.SetStringField(TEXT("actualWeaponId"),Equipment->WeaponId);Person.SetNumberField(TEXT("catalogReloadSeconds"),Equipment->ReloadSeconds);
        Person.SetNumberField(TEXT("actualCapacity"),Equipment->Capacity);Person.SetNumberField(TEXT("actualRoundsRemaining"),Unit->RoundsRemaining);
        TArray<UStaticMeshComponent*> StaticParts;Unit->GetComponents(StaticParts);UStaticMeshComponent* Loaded=nullptr;
        for(auto* Part:StaticParts)if(Part&&Part->GetFName()==TEXT("NationalLoadedVBV82"))Loaded=Part;
        const bool Visible=Loaded&&Loaded->IsVisible();Person.SetBoolField(TEXT("actualLoadedRoundVisible"),Visible);
        VBLoaded|=Unit->ShotsDischarged==0&&Unit->RoundsRemaining==1&&Visible;
        VBEmpty|=Unit->ShotsDischarged>0&&Unit->RoundsRemaining==0&&!Visible;
        const float Phase=Unit->GetWeaponReloadNormalizedPhase();
        if(Unit->WeaponPhase==EFrontlineWeaponPhase::Reload)
        {
            if(VBReloadStartedAt<0)VBReloadStartedAt=NBattle->Elapsed-Phase*Equipment->ReloadSeconds;
            ++VBReloadSamples;TArray<USkeletalMeshComponent*> Parts;Unit->GetComponents(Parts);USkeletalMeshComponent* Pair=nullptr;
            for(auto* Part:Parts)if(Part&&Part->GetFName()==TEXT("NationalReloadPairV82"))Pair=Part;
            auto* Mesh=Pair?Pair->GetSkeletalMeshAsset():nullptr;auto* Node=Pair?Pair->GetSingleNodeInstance():nullptr;
            auto* Clip=Node?Cast<UAnimSequence>(Node->GetCurrentAsset()):nullptr;
            auto* PairAsset=Node?Node->GetCurrentAsset():nullptr;Person.SetBoolField(TEXT("actualPairNodePresent"),Node!=nullptr);
            Person.SetStringField(TEXT("actualPairAssetPath"),PairAsset?PairAsset->GetPathName():FString());
            Person.SetStringField(TEXT("actualPairAssetClass"),PairAsset?PairAsset->GetClass()->GetName():FString());
            Person.SetNumberField(TEXT("actualPairClipLengthSeconds"),Clip?Clip->GetPlayLength():-1);
            Person.SetNumberField(TEXT("actualPairPlayRate"),Node?Node->GetPlayRate():-1);Person.SetBoolField(TEXT("actualPairIsPlaying"),Node&&Node->IsPlaying());
            const TCHAR* Names[]={TEXT("root"),TEXT("weapon_body"),TEXT("weapon_bolt"),TEXT("feed"),TEXT("top_cover"),TEXT("grenade")};
            const int32 Parents[]={INDEX_NONE,0,1,1,1,1};bool Finite=Mesh&&Mesh->GetRefSkeleton().GetNum()==6;
            TArray<TSharedPtr<FJsonValue>> Bones;
            for(int32 I=0;Finite&&I<6;++I)
            {
                const auto& Ref=Mesh->GetRefSkeleton();Finite=Ref.GetBoneName(I)==FName(Names[I])&&Ref.GetParentIndex(I)==Parents[I];
                const FTransform Pose=Pair->GetBoneTransform(FName(Names[I]),RTS_Component);Finite&=!Pose.ContainsNaN();
                auto Bone=MakeShared<FJsonObject>();Bone->SetStringField(TEXT("name"),Names[I]);Bone->SetStringField(TEXT("actualComponentTransform"),Pose.ToString());Bones.Add(MakeShared<FJsonValueObject>(Bone));
            }
            const float Sought=Clip&&Clip->GetPlayLength()>0?Node->GetCurrentTime()/Clip->GetPlayLength():-1;
            const float FrameAllowance=float(FApp::GetDeltaTime())*NBattle->BattleSpeed/4.2f+.005f;
            const bool Seek=Clip&&Clip->IsCompressedDataValid()&&Sought>=0&&FMath::Abs(Sought-Phase)<=FrameAllowance;
            Person.SetArrayField(TEXT("actualVBPairBones"),Bones);Person.SetBoolField(TEXT("actualSixBonePairFinite"),Finite);
            Person.SetBoolField(TEXT("actualPairVisible"),Pair&&Pair->IsVisible());Person.SetNumberField(TEXT("actualPairSeekPhase"),Sought);
            Person.SetNumberField(TEXT("actualPairHostPhaseError"),FMath::Abs(Sought-Phase));Person.SetNumberField(TEXT("actualSingleFrameSeekAllowance"),FrameAllowance);
            if(Phase>.05f)
            {
                VBPairFinite|=Finite&&Seek&&Pair&&Pair->IsVisible();
                Check(TEXT("live_vb_six_bone_pair_seeks_authoritative_reload_clock"),Finite&&Seek&&Pair&&Pair->IsVisible());
                Check(TEXT("live_vb_reload_remains_empty"),Unit->RoundsRemaining==0&&!Visible);
            }
        }
        if(Unit->ReloadsCompleted>0&&VBReloadStartedAt>=0)
        {
            VBRestored|=Unit->RoundsRemaining==1&&Visible;
            Person.SetNumberField(TEXT("actualReloadObservedSeconds"),NBattle->Elapsed-VBReloadStartedAt);
            Check(TEXT("live_vb_catalog_single_round_4_2s_no_early_refill"),Equipment->WeaponId==TEXT("fr.vb")&&Equipment->Capacity==1
                &&FMath::IsNearlyEqual(Equipment->ReloadSeconds,4.2f,.001f)&&NBattle->Elapsed-VBReloadStartedAt>=4.2f-float(FApp::GetDeltaTime())*NBattle->BattleSpeed-.005f);
        }
        for(const auto& Strike:NBattle->Strikes)if(Strike.bFrenchVBRound&&Strike.bRifleLaunched&&Strike.SourceV65.SquadId==Second&&!Strike.bResolved&&Strike.Projectile.IsValid())
        {
            const float Flight=Strike.ImpactAt-Strike.LaunchedAt;if(Flight<=0)continue;
            const float Age=FMath::Clamp(NBattle->Elapsed-Strike.LaunchedAt,0.f,Flight);
            const FVector Tangent=(Strike.Target-Strike.Origin)/Flight+FVector(0,0,490.f*Flight-980.f*Age);
            const float Dot=FVector::DotProduct(Strike.Projectile->GetActorQuat().RotateVector(FVector(0,-1,0)),Tangent.GetSafeNormal());
            Person.SetNumberField(TEXT("actualVBProjectileMinusYDotFlightTangent"),Dot);Person.SetNumberField(TEXT("actualVBFlightSeconds"),Flight);
            Person.SetStringField(TEXT("actualCommittedLaunchOrigin"),Strike.Origin.ToString());
            Check(TEXT("live_vb_projectile_minus_y_axis_tracks_committed_ballistic_tangent"),Dot>.995f);VBProjectile|=Dot>.995f;
        }
    }
    void Sample(bool TestMovementClearance=false)
    {
        auto* NBattle=Battle();if(!NBattle)return;
        const auto View=WarfareFRRUWorldV82::View(NBattle);auto Row=MakeShared<FJsonObject>();
        if(TestMovementClearance)FailureDeepSampledV87=true;
        Row->SetBoolField(TEXT("fullFailureMovementReadbackV87"),TestMovementClearance);
        Row->SetNumberField(TEXT("hostElapsed"),NBattle->Elapsed);Row->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-Started);
        Row->SetNumberField(TEXT("gpuFrameMs"),FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
        Row->SetNumberField(TEXT("gameThreadMs"),FPlatformTime::ToMilliseconds(GGameThreadTime));
        Row->SetNumberField(TEXT("renderThreadMs"),FPlatformTime::ToMilliseconds(GRenderThreadTime));
        Row->SetNumberField(TEXT("rhiThreadMs"),FPlatformTime::ToMilliseconds(GRHIThreadTime));
        Row->SetNumberField(TEXT("appFrameMs"),FApp::GetDeltaTime()*1000.);
        Row->SetNumberField(TEXT("wallFrameMs"),WallFrameMs);
        Row->SetStringField(TEXT("stage"),View.StageId);Row->SetStringField(TEXT("target"),View.TargetNode);Row->SetStringField(TEXT("notice"),View.Notice);
        Row->SetBoolField(TEXT("fallback"),View.Fallback);Row->SetBoolField(TEXT("fallbackAwaitingArrival"),View.FallbackAwaitingArrival);
        Row->SetNumberField(TEXT("progress"),View.Progress);Row->SetNumberField(TEXT("firePhase"),View.FirePhase);Row->SetNumberField(TEXT("resolvedSalvos"),View.ResolvedSalvos);
        Row->SetBoolField(TEXT("reservePending"),View.ReservePending);Row->SetBoolField(TEXT("paused"),UGameplayStatics::IsGamePaused(NBattle));
        Row->SetBoolField(TEXT("objectiveFinished"),View.Finished);Row->SetNumberField(TEXT("units"),NBattle->Units.Num());Row->SetNumberField(TEXT("strikes"),NBattle->Strikes.Num());
        if(Scenario->Probe==EProbe::Ordinary)Row->SetObjectField(TEXT("actualObjectiveEligibility"),WarfareFRRUWorldV82::ObjectiveDiagnosis(NBattle));
        if(Scenario->Probe==EProbe::Train)
        {
            Row->SetNumberField(TEXT("actualContactSquad"),TrainContactSquad);Row->SetNumberField(TEXT("publicContactRecruits"),TrainContactRecruits);
            Row->SetNumberField(TEXT("acceptedSouthernRouteLeg"),TrainContactLeg);Row->SetBoolField(TEXT("contactHeldAtAssembly"),TrainContactRailHeld);
            Row->SetBoolField(TEXT("contactHeldAtObjective"),TrainContactHeld);Row->SetNumberField(TEXT("weaponReviewSerial"),TrainCombatSerial);
        }
        if(View.RequiredRole==int32(EFrontlineRole::Pioneer))ClearanceNode=View.TargetNode;
        if(!ClearanceNode.IsEmpty()&&WarfareFRRUWorldV82::PassageCleared(NBattle,ClearanceNode))ClearanceCompleted=true;
        Row->SetBoolField(TEXT("actualPassageCleared"),ClearanceCompleted);Row->SetStringField(TEXT("actualClearanceNode"),ClearanceNode);
        if(Scenario->Probe==EProbe::Ordinary)
        {
            FVector Target;float Radius=0;const bool TargetKnown=WarfareFRRUWorldV82::NodePose(NBattle,View.TargetNode,Target,Radius);
            TArray<const FFrontlineSquad*> Candidates;
            for(const auto& Group:NBattle->Squads)if(Group.LivingCount()>0)Candidates.Add(&Group);
            TSet<int32> PartialIngressOwnersV87;
            for(const auto* Group:Candidates)
            {
                const AWarfareFrontlineSoldier* First=nullptr;bool Entered=false,Unentered=false;
                for(const auto& Weak:Group->Members)if(const auto* U=Weak.Get();U&&U->IsAlive())
                {if(!First)First=U;Entered|=U->bFieldEntered;Unentered|=!U->bFieldEntered;}
                if(!First||!Entered||!Unentered||!Group->bAdvancing||Group->bHoldPosition)continue;
                const auto D=WarfareFRRUWorldV82::MovementDiagnosis(First,false);FString Current,Post;double Owner=-1;bool Bay=false;
                D->TryGetStringField(TEXT("currentNode"),Current);D->TryGetStringField(TEXT("targetNode"),Post);
                D->TryGetNumberField(TEXT("receivingAdmissionOwner"),Owner);D->TryGetBoolField(TEXT("hasReservedCell"),Bay);
                if(Current.IsEmpty()&&Post==(Group->Team==0?TEXT("home"):TEXT("enemy_assembly"))&&Bay&&int32(Owner)==Group->Id)
                    PartialIngressOwnersV87.Add(Group->Id);
            }
            Candidates.Sort([&](const FFrontlineSquad& A,const FFrontlineSquad& Z)
                {const bool AH=PartialIngressOwnersV87.Contains(A.Id),ZH=PartialIngressOwnersV87.Contains(Z.Id);
                return AH!=ZH?AH:FVector::DistSquared2D(A.Centre(),Target)<FVector::DistSquared2D(Z.Centre(),Target);});
            TArray<TSharedPtr<FJsonValue>> Sections,Posts;OrdinaryTrafficSectionsV87.Reset();
            for(const auto& Post:View.Nodes)
            {auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("id"),Post.Id);Item->SetNumberField(TEXT("owner"),Post.Owner);
                Item->SetBoolField(TEXT("blocked"),Post.Blocked);Posts.Add(MakeShared<FJsonValueObject>(Item));}
            Row->SetArrayField(TEXT("actualNodeOwners"),Posts);
            for(int32 Index=0;Index<FMath::Min(32,Candidates.Num());++Index)
            {
                const auto* Group=Candidates[Index];auto Section=MakeShared<FJsonObject>();
                Section->SetNumberField(TEXT("id"),Group->Id);Section->SetNumberField(TEXT("side"),Group->Team);Section->SetNumberField(TEXT("role"),int32(Group->Role));
                Section->SetNumberField(TEXT("living"),Group->LivingCount());Section->SetBoolField(TEXT("held"),Group->bHoldPosition);Section->SetBoolField(TEXT("advancing"),Group->bAdvancing);
                Section->SetBoolField(TEXT("partialFirstIngressOwnerV87"),PartialIngressOwnersV87.Contains(Group->Id));
                Section->SetBoolField(TEXT("allLivingAtCurrentTarget"),TargetKnown&&At(Group->Id,View.TargetNode));
                TArray<TSharedPtr<FJsonValue>> Members,TrafficMembers;
                int32 Entered=0,Moving=0,Engaging=0,Shots=0,Within=0;float Travel=0;
                for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive())
                {
                    Entered+=Unit->bFieldEntered?1:0;Moving+=Unit->bMoving?1:0;Engaging+=Unit->bEngaging?1:0;Shots+=Unit->ShotsDischarged;Travel+=Unit->DistanceTravelled;
                    Within+=TargetKnown&&FVector::Dist2D(Unit->Feet(),Target)<=Radius?1:0;
                    // One real representative keeps the route/guide history at
                    // regular cadence. Failure retains every living member and
                    // full static/body clearance once, instead of duplicating
                    // neighbours, bay owners and ground probes every half second.
                    auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("member"),Unit->MemberIndex);Item->SetStringField(TEXT("feet"),Unit->Feet().ToString());
                    Item->SetBoolField(TEXT("entered"),Unit->bFieldEntered);Item->SetBoolField(TEXT("moving"),Unit->bMoving);Item->SetBoolField(TEXT("engaging"),Unit->bEngaging);
                    Item->SetNumberField(TEXT("speed"),Unit->GetMotionSpeed());Item->SetNumberField(TEXT("shots"),Unit->ShotsDischarged);
                    Item->SetBoolField(TEXT("withinCurrentTarget"),TargetKnown&&FVector::Dist2D(Unit->Feet(),Target)<=Radius);
                    auto Movement=WarfareFRRUWorldV82::MovementDiagnosis(Unit,TestMovementClearance);
                    auto Internal=MakeShared<FJsonObject>(*Item);Internal->SetObjectField(TEXT("movement"),Movement);TrafficMembers.Add(MakeShared<FJsonValueObject>(Internal));
                    if(!TestMovementClearance&&!Members.IsEmpty())continue;
                    if(!TestMovementClearance)
                    {
                        auto Compact=MakeShared<FJsonObject>();
                        for(const TCHAR* Key:{TEXT("contextValid"),TEXT("actualNativeNode"),TEXT("ordinaryOwnsSection"),TEXT("holding"),TEXT("advancing"),TEXT("engaging"),TEXT("weaponPhase"),TEXT("gunState"),TEXT("guideExecutionV87"),TEXT("waitingToJoinFormation"),TEXT("currentNode"),TEXT("targetNode"),TEXT("nextNode"),TEXT("edgeIndex"),TEXT("hostileIntermediateReceiverWaitingV87"),TEXT("receivingAdmissionWaiting"),TEXT("receivingAdmissionOwner"),TEXT("receivingQueuePosition"),TEXT("transitCorridorWaiting"),TEXT("hasReservedCell"),TEXT("reservedCell"),TEXT("distanceToNode"),TEXT("distanceToWaypoint"),TEXT("stalledActualMovementAttemptSecondsV87"),TEXT("waitingForIngressBayV87"),TEXT("mandatoryIngressParticipantV87"),TEXT("lastTransitRejection"),TEXT("lastLocalLegRejection")})
                            if(const auto* Value=Movement->Values.Find(Key))Compact->Values.Add(Key,*Value);
                        Movement=Compact;
                    }
                    Item->SetObjectField(TEXT("movement"),Movement);Members.Add(MakeShared<FJsonValueObject>(Item));
                }
                Section->SetNumberField(TEXT("enteredMembers"),Entered);Section->SetNumberField(TEXT("movingMembers"),Moving);Section->SetNumberField(TEXT("engagingMembers"),Engaging);
                Section->SetNumberField(TEXT("membersWithinCurrentTarget"),Within);Section->SetNumberField(TEXT("totalLivingShots"),Shots);Section->SetNumberField(TEXT("totalLivingActualTravelCm"),Travel);
                auto InternalSection=MakeShared<FJsonObject>(*Section);InternalSection->SetArrayField(TEXT("allLivingMembers"),TrafficMembers);
                OrdinaryTrafficSectionsV87.Add(MakeShared<FJsonValueObject>(InternalSection));
                Section->SetBoolField(TEXT("allLivingMembersIncludedV87"),TestMovementClearance);
                Section->SetArrayField(TestMovementClearance?TEXT("allLivingMembers"):TEXT("representativeMembersV87"),Members);Sections.Add(MakeShared<FJsonValueObject>(Section));
            }
            Row->SetArrayField(TEXT("ordinaryActualSections"),Sections);Row->SetBoolField(TEXT("ordinaryDiagnosticsBoundedTo32Sections"),true);
        }
        TArray<TSharedPtr<FJsonValue>> People;
        Row->SetNumberField(TEXT("actualFTObjectiveSection"),FTObjectiveSection);Row->SetNumberField(TEXT("publicFTObjectiveRecruits"),FTObjectiveRecruits);
        // Reserve ingress readback includes the real preceding bay occupants.
        TSet<int32> ReserveBayOwners;
        if(Scenario->Probe==EProbe::Reserve)if(const auto* Unit=Member(Hostile))
        {
            const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Unit,false);
            const TArray<TSharedPtr<FJsonValue>>* Bays=nullptr;
            if(Diagnosis->TryGetArrayField(TEXT("nextNodeBayOwners"),Bays))for(const auto& Bay:*Bays)
                ReserveBayOwners.Add(int32(Bay->AsObject()->GetNumberField(TEXT("squad"))));
        }
        if(Scenario->Probe==EProbe::Reserve&&WarfareFRRUCampaignV82::MissionId(NBattle)==TEXT("ru.galicia"))
        {
            // Keep existing bridge claimants visible after leaving the ingress bay (about 40s); passive readback only.
            int32 AdditionalClaimantPeople=0;
            for(const auto& Group:NBattle->Squads)
            {
                if(Group.Team!=1||Group.LivingCount()<=0||ReserveBayOwners.Contains(Group.Id)
                    ||Group.Members.IsEmpty()||Group.Members.Num()>6||AdditionalClaimantPeople+Group.Members.Num()>12)continue;
                for(const auto& Weak:Group.Members)if(const auto* Unit=Weak.Get();IsValid(Unit)&&Unit->IsAlive()
                    &&Unit->Battle.Get()==NBattle&&Unit->Team==1&&Unit->SquadId==Group.Id)
                {
                    const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Unit,false);FString Target;
                    if(Diagnosis->GetBoolField(TEXT("contextValid"))&&Diagnosis->GetBoolField(TEXT("hasRoute"))
                        &&Diagnosis->TryGetStringField(TEXT("targetNode"),Target)&&Target==TEXT("bridge"))
                    {ReserveBayOwners.Add(Group.Id);AdditionalClaimantPeople+=Group.Members.Num();}
                    break;
                }
            }
        }
        // End reserve ingress readback.
        // The VB cycle watches at most one actual hostile receiving owner.
        // Keep one representative at the existing cadence; the existing
        // failure sample retains that same section's living members once.
        int32 VBReceivingOwnerV87=INDEX_NONE;
        if(Scenario->Probe==EProbe::VB&&WarfareFRRUCampaignV82::MissionId(NBattle)==FString(Scenario->Mission))
            if(const auto* Source=Member(Hostile);Source&&Source->Battle.Get()==NBattle)
            {
                const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Source,false);double Owner=INDEX_NONE;
                if(Diagnosis->GetBoolField(TEXT("contextValid"))&&Diagnosis->TryGetNumberField(TEXT("receivingAdmissionOwner"),Owner)
                    &&Owner>0&&Owner<MAX_int32&&Owner==FMath::FloorToDouble(Owner))
                    if(const auto* OwnerSection=NBattle->FindSquad(int32(Owner));OwnerSection&&OwnerSection->Team==Source->Team
                        &&OwnerSection->LivingCount()>0&&OwnerSection->Members.Num()<=6)
                        VBReceivingOwnerV87=OwnerSection->Id;
            }
        if(Scenario->Probe==EProbe::VB)Row->SetNumberField(TEXT("actualVBReceivingOwnerV87"),VBReceivingOwnerV87);
        for(AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit&&(Unit->SquadId==Lead||Unit->SquadId==Second||Unit->SquadId==Observer||Unit->SquadId==Hostile||(Scenario->Probe==EProbe::Schilt&&Unit->SquadId==SchiltPioneerV87)||(Scenario->Probe==EProbe::VB&&Unit->SquadId==VBPioneerV87)||(Scenario->Probe==EProbe::FrenchPlan&&Unit->SquadId==PlanReviewV87.Worker)||ReserveBayOwners.Contains(Unit->SquadId)||TrainContactSquads.Contains(Unit->SquadId)||FTObjectiveSections.Contains(Unit->SquadId)||(Unit->SquadId==VBReceivingOwnerV87&&Unit->IsAlive()&&(TestMovementClearance||Unit->MemberIndex==0))))
        {
            auto Person=MakeShared<FJsonObject>();Person->SetNumberField(TEXT("squad"),Unit->SquadId);Person->SetNumberField(TEXT("member"),Unit->MemberIndex);
            Person->SetStringField(TEXT("feet"),Unit->Feet().ToString());Person->SetNumberField(TEXT("speed"),Unit->GetMotionSpeed());Person->SetBoolField(TEXT("entered"),Unit->bFieldEntered);
            Person->SetBoolField(TEXT("moving"),Unit->bMoving);Person->SetBoolField(TEXT("alive"),Unit->IsAlive());
            Person->SetObjectField(TEXT("movement"),WarfareFRRUWorldV82::MovementDiagnosis(Unit,TestMovementClearance));
            Person->SetNumberField(TEXT("shots"),Unit->ShotsDischarged);Person->SetNumberField(TEXT("reloadStarted"),Unit->ReloadsStarted);Person->SetNumberField(TEXT("reloadCompleted"),Unit->ReloadsCompleted);
            Person->SetNumberField(TEXT("reloadPhase"),Unit->GetWeaponReloadNormalizedPhase());Person->SetBoolField(TEXT("active"),Unit->IsActiveCombatant());
            if(Unit->IsAlive()&&Unit->Role==EFrontlineRole::Rifle)
                Person->SetObjectField(TEXT("actualOrdinaryWeaponCycle"),WarfareFRRUPresentationV82::ProbeUnit(Unit,false));
            SampleVB(Unit,*Person);
            FFrontlineFlameStateV82 FlameState;
            if(Unit->SquadId==Second&&FrontlineFieldV32::QueryFlameStateV82(Unit,FlameState))
            {
                Person->SetNumberField(TEXT("flameStartedAt"),FlameState.StartedAt);Person->SetNumberField(TEXT("flameUntil"),FlameState.JetEndsAt);
                Person->SetNumberField(TEXT("flameReadyAt"),FlameState.ReadyAt);Person->SetNumberField(TEXT("fuelCharges"),FlameState.FuelCharges);
                Person->SetBoolField(TEXT("valveBrace"),FlameState.Bracing);Person->SetBoolField(TEXT("jetActive"),FlameState.JetActive);
                Person->SetBoolField(TEXT("fuelIgnited"),FlameState.FuelIgnited);Person->SetBoolField(TEXT("fuelSpent"),FlameState.FuelSpent);
                FlameBrace|=FlameState.Bracing&&!FlameState.JetActive;FlameJet|=FlameState.JetActive;
                FlameRecovery|=FlameState.Alive&&FlameState.StartedAt>0&&FlameState.HostElapsed>=FlameState.JetEndsAt&&FlameState.HostElapsed<FlameState.ReadyAt;
                FlameIgnited|=FlameState.FuelIgnited;FlameSpent|=FlameState.FuelSpent;FlameEmpty|=FlameState.FuelCharges==0;
                if(FlameState.JetActive)
                {
                    const FVector Forward=WarfareFRRUPresentationV82::MuzzleForward(Unit).GetSafeNormal2D();
                    Check(TEXT("live_schilt_nozzle_tracks_host_jet_direction"),!Forward.IsNearlyZero()&&FVector::DotProduct(Forward,FlameState.Direction.GetSafeNormal2D())>.96,
                        FString::Printf(TEXT("dot=%.6f"),FVector::DotProduct(Forward,FlameState.Direction.GetSafeNormal2D())));
                }
                if(Unit->ShotsDischarged>FlameShotsObserved)
                {
                    if(LastFlameStarted>=0)Check(TEXT("schilt_catalog_cycle_owns_real_burst_clock"),FlameState.StartedAt-LastFlameStarted>=4.54f);
                    Check(TEXT("schilt_one_real_discharge_consumes_one_finite_charge"),FlameState.FuelCharges==PreviousFlameFuel-1&&Unit->ShotsDischarged==FlameShotsObserved+1);
                    Check(TEXT("schilt_cycle_is_catalog_4_55_without_classic_reserve"),FMath::IsNearlyEqual(FlameState.ReadyAt-(FlameState.StartedAt-.18f),4.55f,.005f));
                    LastFlameStarted=FlameState.StartedAt;PreviousFlameFuel=FlameState.FuelCharges;FlameShotsObserved=Unit->ShotsDischarged;
                }
            }
            if(auto* Gun=FrontlineMachinegunsV30::Get(Unit))
            {
                Person->SetNumberField(TEXT("gunState"),int32(Gun->State));Person->SetNumberField(TEXT("deployments"),Gun->Deployments);Person->SetNumberField(TEXT("packs"),Gun->Packs);
                Person->SetBoolField(TEXT("gunTrench"),Gun->IsTrench());Person->SetBoolField(TEXT("bipodContact"),Gun->HasBipodContactV36());
                if(Unit->IsMachineGunner())
                {
                    Person->SetBoolField(TEXT("actualGunCanFire"),Gun->CanFire());
                    Person->SetBoolField(TEXT("actualGunAssetsReady"),Gun->HasAssets());
                    Person->SetNumberField(TEXT("actualGunLockedYaw"),Gun->FormationYawV50());
                    Person->SetNumberField(TEXT("actualGunActorYaw"),Unit->GetActorRotation().Yaw);
                    Person->SetNumberField(TEXT("actualGunYawErrorDegrees"),FMath::Abs(FMath::FindDeltaAngleDegrees(Gun->FormationYawV50(),Unit->GetActorRotation().Yaw)));
                    Person->SetNumberField(TEXT("actualGunWeaponPhase"),int32(Unit->WeaponPhase));
                    Person->SetNumberField(TEXT("actualGunBlockedShotAttempts"),Gun->BlockedShotAttempts);
                    // Passive candidate readback. Never acquire/change a focus
                    // target or call the wrapper that counts blocked attempts.
                    int32 InRangeEnemies=0,ClearMuzzleEnemies=0;double NearestEnemy=DBL_MAX,NearestInRange=DBL_MAX;
                    const AWarfareFrontlineSoldier* DiagnosticEnemy=nullptr;
                    const double FireRange=FrontlineWeapons::ProfileFor(Unit).FireRangeCm*NBattle->EncounterVisibilityScale();
                    for(const AWarfareFrontlineSoldier* Enemy:NBattle->Units)if(Enemy&&Enemy->Team!=Unit->Team&&Enemy->IsActiveCombatant())
                    {
                        const double Distance=FVector::Dist2D(Unit->Feet(),Enemy->Feet());NearestEnemy=FMath::Min(NearestEnemy,Distance);
                        if(Distance<=FireRange)
                        {
                            ++InRangeEnemies;
                            if(Distance<NearestInRange){NearestInRange=Distance;DiagnosticEnemy=Enemy;}
                            if(WarfareFRRUWorldV82::FiringLine(Unit,Enemy)&&Gun->HasInfantryFiringLine(Enemy))++ClearMuzzleEnemies;
                        }
                    }
                    Person->SetNumberField(TEXT("actualGunFireRangeCm"),FireRange);
                    Person->SetNumberField(TEXT("actualGunNearestEnemyCm"),NearestEnemy==DBL_MAX?-1:NearestEnemy);
                    Person->SetNumberField(TEXT("actualGunInRangeEnemies"),InRangeEnemies);
                    Person->SetNumberField(TEXT("actualGunClearMuzzleEnemies"),ClearMuzzleEnemies);
                    if(Scenario->Probe==EProbe::Reserve&&DiagnosticEnemy)
                    {
                        // Read the same live muzzle, target alternatives and
                        // ignored infantry as the actual discharge checks.
                        // These traces neither acquire a focus nor grant fire.
                        auto Readback=MakeShared<FJsonObject>();Readback->SetBoolField(TEXT("readOnly"),true);
                        Readback->SetNumberField(TEXT("targetPerson"),DiagnosticEnemy->SquadId*16+DiagnosticEnemy->MemberIndex);
                        Readback->SetStringField(TEXT("targetFeet"),DiagnosticEnemy->Feet().ToString());
                        Readback->SetNumberField(TEXT("distanceCm"),NearestInRange);
                        const FVector Muzzle=Unit->GetMuzzleLocation();Readback->SetStringField(TEXT("muzzle"),Muzzle.ToString());
                        Readback->SetBoolField(TEXT("worldDischargeLineClear"),WarfareFRRUWorldV82::FiringLine(Unit,DiagnosticEnemy));
                        Readback->SetBoolField(TEXT("gunInfantryLineClear"),Gun->HasInfantryFiringLine(DiagnosticEnemy));
                        FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUNaturalGunReadbackV82),true);
                        for(const AWarfareFrontlineSoldier* Other:NBattle->Units)if(Other)Query.AddIgnoredActor(Other);
                        const auto* Native=wfexp::scene::ForMission(wfexpue::Narrow(View.MissionId));
                        const FVector Torso=DiagnosticEnemy->Soldier?DiagnosticEnemy->Soldier->GetBoneLocation(TEXT("spine_03")):DiagnosticEnemy->Feet()+FVector(0,0,95);
                        const FVector Head=DiagnosticEnemy->Soldier?DiagnosticEnemy->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):DiagnosticEnemy->Feet()+FVector(0,0,145);
                        TArray<TSharedPtr<FJsonValue>> Rays;
                        const auto Trace=[&](const TCHAR* Semantic,const FVector& Target)
                        {
                            auto Ray=MakeShared<FJsonObject>();Ray->SetStringField(TEXT("targetSemantic"),Semantic);Ray->SetStringField(TEXT("target"),Target.ToString());
                            Ray->SetBoolField(TEXT("nativeLineClear"),WarfareFRRUWorldV82::HasSight(NBattle,Muzzle,Target));
                            FHitResult Hit;const bool Blocked=Unit->GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Target,ECC_Visibility,Query);
                            Ray->SetBoolField(TEXT("physicsBlocked"),Blocked);
                            if(Blocked)
                            {
                                Ray->SetStringField(TEXT("hitActor"),Hit.GetActor()?Hit.GetActor()->GetPathName():FString());
                                Ray->SetStringField(TEXT("hitComponent"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():FString());
                                Ray->SetStringField(TEXT("impact"),Hit.ImpactPoint.ToString());Ray->SetStringField(TEXT("normal"),Hit.ImpactNormal.ToString());
                                Ray->SetNumberField(TEXT("hitFraction"),Hit.Time);Ray->SetBoolField(TEXT("initialPenetration"),Hit.bStartPenetrating);
                            }
                            const bool NativeBound=Native&&View.SceneId==wfexpue::Wide(Native->id);
                            Ray->SetBoolField(TEXT("nativeSceneBound"),NativeBound);
                            if(NativeBound)
                            {
                                // Exactly the production LOS terrain sample
                                // spacing/height boundary; solids remain the
                                // separate nativeLineClear and physics reads.
                                const double Length=FVector::Dist2D(Muzzle,Target);
                                if(FMath::IsFinite(Length)&&Length<=100000.)
                                {
                                    const int32 Steps=FMath::Max(1,FMath::CeilToInt(Length/40.));
                                    for(int32 I=1;I<Steps;++I)
                                    {
                                        const double T=double(I)/Steps;const FVector At=Muzzle+(Target-Muzzle)*T;double Floor=0;
                                        const bool Height=wfexp::scene::HeightAt(*Native,At.X,At.Y,Floor);
                                        if(!Height||At.Z<=Floor+2.)
                                        {
                                            Ray->SetNumberField(TEXT("firstNativeTerrainBlockedSample"),I);
                                            Ray->SetNumberField(TEXT("nativeTerrainFraction"),T);Ray->SetStringField(TEXT("nativeTerrainRayPoint"),At.ToString());
                                            Ray->SetBoolField(TEXT("nativeTerrainHeightAvailable"),Height);if(Height)Ray->SetNumberField(TEXT("nativeTerrainFloorCm"),Floor);break;
                                        }
                                    }
                                }
                            }
                            Rays.Add(MakeShared<FJsonValueObject>(Ray));
                        };
                        Trace(TEXT("world_torso"),Torso);Trace(TEXT("shared_head"),Head);
                        Trace(TEXT("gun_sight_target"),FrontlineMachinegunsV30::SightTarget(DiagnosticEnemy));
                        Readback->SetArrayField(TEXT("rays"),Rays);Person->SetObjectField(TEXT("actualGunLineReadback"),Readback);
                    }
                    if(auto* PoseNode=Unit->Soldier?Unit->Soldier->GetSingleNodeInstance():nullptr)
                    {
                        Person->SetStringField(TEXT("actualGunClip"),PoseNode->GetCurrentAsset()?PoseNode->GetCurrentAsset()->GetPathName():FString());
                        Person->SetNumberField(TEXT("actualGunClipSeconds"),PoseNode->GetCurrentTime());
                    }
                    int32 Contacts=0;Person->SetBoolField(TEXT("actualGunSupportReady"),WarfareFRRUPresentationV82::GunSupportContacts(Unit,Contacts));Person->SetNumberField(TEXT("actualGunSupportContacts"),Contacts);
                    if(Unit->SquadId==Second||(Scenario->Probe==EProbe::Reserve&&Unit->SquadId==Observer))
                        Person->SetObjectField(TEXT("actualGunSupportReadback"),WarfareFRRUPresentationV82::GunSupportReadback(Unit));
                    if(const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Unit))Person->SetStringField(TEXT("actualGunWeaponId"),Equipment->WeaponId);
                    if(auto* Animation=Cast<UWarfareFrontlineAnimation>(Unit->Soldier->GetAnimInstance()))
                    {
                        const auto D=Animation->GetCharacterDiagnostics();Person->SetNumberField(TEXT("nationalCarryGripFrames"),D.NationalCarryGripFramesV82);
                        Person->SetNumberField(TEXT("nationalCarryGripRejectedFrames"),D.NationalCarryGripRejectedFramesV82);
                        Person->SetNumberField(TEXT("nationalGroundGunFrames"),D.NationalGroundGunFramesV82);
                        Person->SetNumberField(TEXT("nationalGroundGunRejectedFrames"),D.NationalGroundGunRejectedFramesV82);
                        Person->SetNumberField(TEXT("maximumNationalGroundGunShiftCm"),D.MaximumNationalGroundGunShiftCmV82);
                        Person->SetNumberField(TEXT("maximumNationalGroundGunTiltDegrees"),D.MaximumNationalGroundGunTiltDegreesV82);
                        Person->SetNumberField(TEXT("maximumNationalGroundFootErrorCm"),D.MaximumNationalGroundFootErrorCmV82);
                        Person->SetNumberField(TEXT("maximumNationalGroundGripErrorCm"),D.MaximumNationalGroundGripErrorCmV82);
                        Person->SetNumberField(TEXT("maximumNationalCarryGunShiftCm"),D.MaximumNationalCarryGunShiftCmV82);
                        Person->SetNumberField(TEXT("maximumNationalBothGripResidualCm"),D.MaximumNationalBothGripResidualCmV82);
                    }
                }
                if(Unit->SquadId==Second&&Unit->IsMachineGunner())
                {GunCarry|=Gun->State==EFrontlineGunStanceV30::Carry;GunDeploy|=Gun->State==EFrontlineGunStanceV30::Deploy;GunSupported|=Gun->IsSupported();GunPack|=Gun->Packs>0;
                    GunShots|=Unit->ShotsDischarged>0;GunReload|=Unit->ReloadsStarted>0;GunReloadDone|=Unit->ReloadsCompleted>0;}
            }
            FFRRUEarthworkPoseV82 Earthwork;
            const bool Supported=WarfareFRRUWorldV82::EarthworkPose(Unit,Earthwork);Person->SetBoolField(TEXT("nativeEarthworkValid"),Supported);
            if(Supported){Person->SetStringField(TEXT("earthworkNode"),Earthwork.NodeId);Person->SetStringField(TEXT("support"),Earthwork.Support.ToString());}
            People.Add(MakeShared<FJsonValueObject>(Person));
        }
        Row->SetArrayField(TEXT("people"),People);
        if(Scenario->Probe==EProbe::Schilt)for(const auto& Unit:NBattle->Units)
            if(Unit&&Unit->Team==1&&!Unit->IsAlive()&&Unit->FatalSourceV65.SquadId==Second&&Unit->GetDeathState().Kind==EFrontlineDamageKind::Fire)
                FlameEnemyBurn|=Unit->GetDeathState().bStarted&&Unit->GetDeathState().PublishedPoseSamples>0;
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(NBattle->GetWorld()))
        {
            TArray<TSharedPtr<FJsonValue>> Machines;
            for(const auto& Weak:Fleet->GetVehicles())if(auto* Vehicle=Weak.Get();Vehicle&&(WarfareFRRUArmourV82::National(Vehicle)||Vehicle->UsesNationalGroundContactV82()))
            {
                if(!WarfareFRRUArmourV82::National(Vehicle))
                {
                    auto Machine=MakeShared<FJsonObject>();Machine->SetStringField(TEXT("id"),UFrontlineVehicleSubsystem::Definition(Vehicle->Type).AssetKey);
                    Machine->SetNumberField(TEXT("serial"),Vehicle->Serial);Machine->SetNumberField(TEXT("side"),Vehicle->Team);
                    Machine->SetStringField(TEXT("position"),Vehicle->GetActorLocation().ToString());Machine->SetStringField(TEXT("rotation"),Vehicle->GetActorRotation().ToString());
                    Machine->SetNumberField(TEXT("travelled"),Vehicle->DistanceTravelled);Machine->SetNumberField(TEXT("trackPhaseCm"),Vehicle->GetTrackTravelPhase());
                    Machine->SetNumberField(TEXT("cannonShots"),Vehicle->CannonShots);Machine->SetNumberField(TEXT("mgShots"),Vehicle->MachinegunShots);
                    Machine->SetNumberField(TEXT("hull"),Vehicle->GetHullIntegrity());Machine->SetNumberField(TEXT("trackLeft"),Vehicle->GetTrackIntegrity(0));Machine->SetNumberField(TEXT("trackRight"),Vehicle->GetTrackIntegrity(1));
                    Machine->SetBoolField(TEXT("entered"),Vehicle->HasEnteredField());Machine->SetBoolField(TEXT("operational"),Vehicle->IsOperational());
                    if(const auto Contact=Vehicle->DescribeNationalGroundContactV82();Contact.IsValid())
                    {
                        Machine->SetObjectField(TEXT("germanStaticContact"),Contact);
                        bool Passed=false,HullPassed=false,Complete=false;double Gap=-DBL_MAX;
                        if(Contact->TryGetBoolField(TEXT("passed"),Passed)&&Passed
                            &&Contact->TryGetBoolField(TEXT("completeHullPassed"),HullPassed)&&HullPassed
                            &&Contact->TryGetBoolField(TEXT("finiteFaceScanComplete"),Complete)&&Complete
                            &&Contact->TryGetNumberField(TEXT("minimumFiniteFaceGapCm"),Gap)&&Gap>=-.1)
                            GermanContactSerials.Add(Vehicle->Serial);
                    }
                    if(GermanCoreOnly()&&Vehicle->Team==1&&Vehicle->DistanceTravelled>=600.f
                        &&Vehicle->CannonShots>0&&Vehicle->MachinegunShots>0&&GermanContactSerials.Contains(Vehicle->Serial))
                    {
                        const bool Heavy=Vehicle->Type==EFrontlineVehicle::A7V;
                        bool& Complete=Heavy?GermanA7VCore:GermanBeuteCore;
                        if(!Complete&&(Heavy||Vehicle->Type==EFrontlineVehicle::BeuteMarkIV))
                        {Complete=true;Check(Heavy?TEXT("german_a7v_actual_contact_travel_cannon_and_machinegun"):TEXT("german_beute_actual_contact_travel_cannon_and_machinegun"),true,
                            FString::Printf(TEXT("same real serial=%d travel=%.1f cannon=%d machinegun=%d; checked faces/support/full hull; no position or clock writes"),Vehicle->Serial,Vehicle->DistanceTravelled,Vehicle->CannonShots,Vehicle->MachinegunShots));}
                    }
                    FVector Centre;float Radius=0;const bool TargetKnown=WarfareFRRUWorldV82::NodePose(NBattle,View.TargetNode,Centre,Radius);
                    Machine->SetBoolField(TEXT("currentTargetHullOccupies"),TargetKnown&&WarfareFRRUArmourV82::OccupiesNode(Vehicle,Centre,Radius));
                    Machines.Add(MakeShared<FJsonValueObject>(Machine));continue;
                }
                auto Machine=MakeShared<FJsonObject>();Machine->SetStringField(TEXT("id"),WarfareFRRUArmourV82::Id(Vehicle));Machine->SetStringField(TEXT("position"),Vehicle->GetActorLocation().ToString());
                Machine->SetNumberField(TEXT("serial"),Vehicle->Serial);Machine->SetNumberField(TEXT("side"),Vehicle->Team);
                Machine->SetStringField(TEXT("rotation"),Vehicle->GetActorRotation().ToString());
                Machine->SetObjectField(TEXT("ftGroundPilot"),WarfareFRRUArmourV82::GroundPilotDiagnosis(Vehicle));
                if(Scenario->Probe==EProbe::FT&&Vehicle->Serial==ReviewVehicleSerial)
                {
                    const auto Contact=WarfareFRRUArmourV82::GroundPilotDiagnosis(Vehicle);
                    const TSharedPtr<FJsonObject>* Attempt=nullptr;bool Passed=false;double Phase=0;
                    if(Contact->TryGetObjectField(TEXT("lastAttempt"),Attempt)&&Attempt&&Attempt->IsValid()
                        &&(*Attempt)->TryGetBoolField(TEXT("passed"),Passed)&&Passed
                        &&(*Attempt)->TryGetNumberField(TEXT("prospectivePhase"),Phase))
                    {
                        // The sampled window can skip the end/start quarters of
                        // this short clip. Forward travel plus a phase decrease
                        // observes a real wrap without changing the host clock.
                        FTPhaseWrapObserved|=PreviousFTPhase>=0&&Vehicle->DistanceTravelled>PreviousFTTravel&&Phase<PreviousFTPhase;
                        PreviousFTPhase=Phase;PreviousFTTravel=Vehicle->DistanceTravelled;
                    }
                }
                Machine->SetNumberField(TEXT("travelled"),Vehicle->DistanceTravelled);Machine->SetNumberField(TEXT("cannonShots"),Vehicle->CannonShots);Machine->SetNumberField(TEXT("mgShots"),Vehicle->MachinegunShots);
                Machine->SetNumberField(TEXT("hull"),Vehicle->GetHullIntegrity());Machine->SetNumberField(TEXT("trackLeft"),Vehicle->GetTrackIntegrity(0));Machine->SetNumberField(TEXT("trackRight"),Vehicle->GetTrackIntegrity(1));
                Machine->SetStringField(TEXT("muzzle0"),WarfareFRRUArmourV82::Muzzle(Vehicle,0,Scenario->Probe==EProbe::FT).ToString());
                Machine->SetBoolField(TEXT("entered"),Vehicle->HasEnteredField());
                ObserveVehicleWeapons(Vehicle);
                if(Vehicle->Serial==WeaponReviewSerial())
                {Machine->SetBoolField(TEXT("applicableCannonObserved"),!RequiresVehicleCannon()||VehicleCannonShot);Machine->SetBoolField(TEXT("applicableMachinegunObserved"),!RequiresVehicleMachinegun()||VehicleMachinegunShot);}
                Machines.Add(MakeShared<FJsonValueObject>(Machine));
            }
            Row->SetArrayField(TEXT("vehicles"),Machines);
        }
        MaximumResolved=FMath::Max(MaximumResolved,View.ResolvedSalvos);
        if(View.ResolvedSalvos==4&&ResolvedFourAt<0)ResolvedFourAt=NBattle->Elapsed;
        FourSalvos|=View.ResolvedSalvos==4;
        // A naturally observed straddler is evidence only if its anchor is outside
        // and another real living member is inside the current final target.
        const auto* Native=wfexp::tactical::FindMission(wfexpue::Narrow(View.MissionId));
        const bool Final=Native&&View.StageId==wfexpue::Wide(Native->stages.back().id);
        bool ActualPlayerCandidate=false;
        if(Final)for(const auto& Group:NBattle->Squads)if(Group.Team==0&&Group.LivingCount()>=Native->stages.back().minimumLiving
            &&(Native->stages.back().requiredRole<0||int32(Group.Role)==Native->stages.back().requiredRole))ActualPlayerCandidate|=At(Group.Id,View.TargetNode);
        if(Final&&ActualPlayerCandidate)
        {
            FVector Centre;float Radius=0;if(WarfareFRRUWorldV82::NodePose(NBattle,View.TargetNode,Centre,Radius))
            {
                for(const auto& Group:NBattle->Squads)if(Group.Team==1&&Group.LivingCount()>0)
                {
                    const AWarfareFrontlineSoldier* Anchor=nullptr;bool Inside=false;
                    for(const auto& Weak:Group.Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsActiveCombatant())
                    {if(!Anchor||Unit->MemberIndex<Anchor->MemberIndex)Anchor=Unit;Inside|=FVector::Dist2D(Unit->Feet(),Centre)<=Radius;}
                    if(Anchor&&FVector::Dist2D(Anchor->Feet(),Centre)>Radius&&Inside){RealStraddler=true;Check(TEXT("actual_straddler_cannot_finish_objective"),!View.Finished);}
                }
                if(auto* Fleet=UFrontlineVehicleSubsystem::Get(NBattle->GetWorld()))for(const auto& Weak:Fleet->GetVehicles())
                    if(const auto* Vehicle=Weak.Get();Vehicle&&Vehicle->Team==1&&WarfareFRRUArmourV82::OccupiesNode(Vehicle,Centre,Radius))
                    {RealHullContest=true;Check(TEXT("actual_hostile_hull_cannot_finish_objective"),!View.Finished);}
            }
        }
        Samples.Add(MakeShared<FJsonValueObject>(Row));
    }
    void Finish()
    {
        if(Finished||Step==EStep::FinalCapture)return;
        if(HoldDepartureProbeV87&&HoldDeparturePhaseV87==1)
        {Event(TEXT("public_release_departure_probe_on_finish"),WarfareFRRUWorldV82::HoldSquad(Battle(),Lead,false));HoldDeparturePhaseV87=2;}
        if(FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate")))FRRUVehicleNavCandidateV87::Clear(Battle());
        if(!FirstFailure.IsEmpty()&&!FailureDeepSampledV87)Sample(true);
        ReleaseOrdinaryFireWindowV87(TEXT("review_finish"));
        RemoveReceivingBayFixtures();
        auto* NBattle=Battle();auto* NController=Controller.Get();
        if(NBattle&&NController&&CompletedCaptures.Contains(TEXT("05-first-warmed-battle")))
        {
            // Only the viewing camera is reframed. No actor, battle phase,
            // objective, order or simulation clock is changed for this image.
            if(IsVehicleCase()&&WeaponReviewVehicle())NController->CameraFocus=WeaponReviewVehicle()->GetActorLocation();
            else if(const auto* Group=NBattle->FindSquad(Lead))NController->CameraFocus=Group->Centre();
            else {FVector Centre;float Radius=0;if(WarfareFRRUWorldV82::NodePose(NBattle,WarfareFRRUWorldV82::View(NBattle).TargetNode,Centre,Radius))NController->CameraFocus=Centre;}
            NController->TargetCameraWidth=6000;SetStep(EStep::FinalCapture);return;
        }
        WriteReport();
    }
    bool ObserveOrdinaryTrafficV827()
    {
        auto* NBattle=Battle();if(!NBattle||Samples.IsEmpty()||UGameplayStatics::IsGamePaused(NBattle))return true;
        const auto Row=Samples.Last()->AsObject();const double Host=Row->GetNumberField(TEXT("hostElapsed"));
        if(Host<=OrdinaryTrafficSampleV827)return true;OrdinaryTrafficSampleV827=Host;
        const auto Eligibility=Row->GetObjectField(TEXT("actualObjectiveEligibility"));
        if(Eligibility->GetBoolField(TEXT("contextValid"))&&int32(Eligibility->GetNumberField(TEXT("kind")))==0
            &&!Eligibility->GetBoolField(TEXT("finished")))
        {
            const FString Stage=Eligibility->GetStringField(TEXT("stage")),Target=Eligibility->GetStringField(TEXT("target"));
            if(Stage!=OrdinaryGuardStageV87||Target!=OrdinaryGuardTargetV87)
            {OrdinaryGuardStageV87=Stage;OrdinaryGuardTargetV87=Target;OrdinaryHoldGuardsV87.Reset();}
            for(const int32 Id:OrdinaryHoldGuardsV87)if(const auto* Guard=NBattle->FindSquad(Id);Guard&&Guard->LivingCount()>0&&Guard->bAdvancing)
            {
                const AWarfareFrontlineSoldier* Person=nullptr;for(const auto& Weak:Guard->Members)if(const auto* U=Weak.Get();U&&U->IsAlive()){Person=U;break;}
                const auto Movement=WarfareFRRUWorldV82::MovementDiagnosis(Person,false);bool Ordinary=false;FString Destination;
                Movement->TryGetBoolField(TEXT("ordinaryOwnsSection"),Ordinary);Movement->TryGetStringField(TEXT("targetNode"),Destination);
                if(Ordinary&&!Destination.IsEmpty()&&Destination!=Target)
                    return Fail(TEXT("ordinary_hold_guard_not_evicted_for_rear_gun"),FString::Printf(TEXT("Arrived guard %d left %s for %s during the same hold stage."),Id,*Target,*Destination));
            }
            int32 Residents=0;
            for(const auto& Value:Eligibility->GetArrayField(TEXT("actualNativeSections")))
            {
                const auto Section=Value->AsObject();if(Section->GetNumberField(TEXT("side"))!=0
                    ||Section->GetStringField(TEXT("nativeNode"))!=Target||!Section->GetBoolField(TEXT("stationary"))
                    ||Section->GetBoolField(TEXT("advancing"))||Section->GetNumberField(TEXT("living"))<Eligibility->GetNumberField(TEXT("minimumLiving")))continue;
                OrdinaryHoldGuardsV87.Add(int32(Section->GetNumberField(TEXT("id"))));++Residents;
            }
            if(Residents>0)++OrdinaryGuardSamplesV87;OrdinaryMaxHoldResidentsV87=FMath::Max(OrdinaryMaxHoldResidentsV87,Residents);
        }
        // All real members remain monitored. Only the durable JSON is slim;
        // the single current readback frame is released at the next sample.
        const auto* Sections=&OrdinaryTrafficSectionsV87;if(Sections->IsEmpty())return true;
        TSet<int32> Seen;
        for(const auto& Value:*Sections)
        {
            const auto Section=Value->AsObject();const int32 Id=int32(Section->GetNumberField(TEXT("id")));
            const int32 Side=int32(Section->GetNumberField(TEXT("side")));
            // The existing private Mort probe also observes autonomous enemy
            // traffic. A queued section or real firefight keeps the same
            // exclusions; this adds no order, movement or combat authority.
            const bool EnemyTrafficProbeV87=Side==1&&FrontlineBlockedFireProbeV87::Enabled(NBattle);
            if((Side!=0&&!EnemyTrafficProbeV87)||!Section->GetBoolField(TEXT("advancing"))||Section->GetBoolField(TEXT("held")))continue;
            const auto* Group=NBattle->FindSquad(Id);if(!Group||Group->LivingCount()<=0)continue;
            const auto& Living=Section->GetArrayField(TEXT("allLivingMembers"));
            if(!Living.IsEmpty())
            {
                const auto Entry=Living[0]->AsObject()->GetObjectField(TEXT("movement"));
                bool Wait=false,Mandatory=false,IdleOwners=false;
                Entry->TryGetBoolField(TEXT("waitingForIngressBayV87"),Wait);Entry->TryGetBoolField(TEXT("mandatoryIngressParticipantV87"),Mandatory);
                Entry->TryGetBoolField(TEXT("allIngressBayOwnersIdleAutomaticV87"),IdleOwners);
                if(Wait&&Mandatory&&IdleOwners)
                {
                    FString Owners;
                    for(const auto& Owner:Entry->GetArrayField(TEXT("nextNodeBayOwners")))Owners+=FString::SanitizeFloat(Owner->AsObject()->GetNumberField(TEXT("squad")))+TEXT("/");
                    auto* Watch=OrdinaryIngressV87.Find(Id);
                    if(!Watch||Watch->Route!=Owners)
                    {FTrafficWatchV827 Fresh;Fresh.Route=Owners;Fresh.At=Host;OrdinaryIngressV87.Add(Id,Fresh);}
                    else if(Host-Watch->At>=45.)return Fail(TEXT("mandatory_ingress_starved_by_idle_home_residents"),FString::Printf(
                        TEXT("Required section %d waited %.1f host seconds without a physical bay while the same automatic home residents stayed idle. No manual Hold is being overridden."),Id,Host-Watch->At));
                    continue;
                }
                OrdinaryIngressV87.Remove(Id);
            }
            const bool PartialIngressOwnerV87=Section->GetBoolField(TEXT("partialFirstIngressOwnerV87"));
            bool Eligible=true,PhysicalBlock=PartialIngressOwnerV87;double Remaining=0;int32 Count=0;
            FString Route=PartialIngressOwnerV87?TEXT("actual_first_ingress_owner/"):FString();
            for(const auto& Person:Section->GetArrayField(TEXT("allLivingMembers")))
            {
                const auto Member=Person->AsObject();const auto D=Member->GetObjectField(TEXT("movement"));
                bool Waiting=false,Ordinary=false,RegisteredRoute=false;D->TryGetBoolField(TEXT("receivingAdmissionWaiting"),Waiting);D->TryGetBoolField(TEXT("ordinaryOwnsSection"),Ordinary);
                D->TryGetBoolField(TEXT("hasRoute"),RegisteredRoute);
                const bool MonitoredRouteV87=Ordinary||(EnemyTrafficProbeV87&&RegisteredRoute);
                double Distance=0;if((!Member->GetBoolField(TEXT("entered"))&&!PartialIngressOwnerV87)||Member->GetBoolField(TEXT("engaging"))
                    ||Waiting||!MonitoredRouteV87||!D->TryGetNumberField(TEXT("distanceToGoal"),Distance)){Eligible=false;break;}
                Remaining+=Distance;++Count;
                bool Clear=true,Approach=false;double Waypoints=0;D->TryGetBoolField(TEXT("directBodyLegClear"),Clear);
                D->TryGetBoolField(TEXT("hasLocalApproach"),Approach);D->TryGetNumberField(TEXT("waypointCount"),Waypoints);
                PhysicalBlock|=!Clear||(Approach&&Waypoints>0&&Distance>56.5);
                Route+=D->GetStringField(TEXT("currentNode"))+TEXT("/")+D->GetStringField(TEXT("targetNode"))+TEXT("/")
                    +D->GetStringField(TEXT("nextNode"))+FString::SanitizeFloat(D->GetNumberField(TEXT("edgeIndex")));
                double Checkpoint=0;if(D->TryGetNumberField(TEXT("transitCheckpointIndex"),Checkpoint))Route+=FString::SanitizeFloat(Checkpoint);
            }
            // A firefight or an explicit FIFO wait is not a stuck-route failure.
            // The watch uses actual checkpoint distances; historical rejection
            // strings alone are not evidence of a current blocking body.
            for(const AWarfareFrontlineSoldier* Enemy:NBattle->Units)
                if(Enemy&&Enemy->Team!=Group->Team&&Enemy->IsActiveCombatant()&&FVector::Dist2D(Enemy->Feet(),Group->Centre())<1200.){Eligible=false;break;}
            if(!Eligible||!PhysicalBlock||Count<=0){OrdinaryTrafficV827.Remove(Id);continue;}
            Seen.Add(Id);Remaining/=Count;auto* Watch=OrdinaryTrafficV827.Find(Id);
            if(!Watch||Watch->Route!=Route||Watch->Living!=Count)
            {FTrafficWatchV827 Fresh;Fresh.Route=Route;Fresh.Living=Count;Fresh.Best=Remaining;Fresh.At=Host;OrdinaryTrafficV827.Add(Id,MoveTemp(Fresh));continue;}
            if(Remaining<Watch->Best-20.){Watch->Best=Remaining;Watch->At=Host;continue;}
            if(Host-Watch->At>=45.)
                return Fail(TEXT("ordinary_actual_route_makes_no_physical_progress"),FString::Printf(
                    TEXT("Section %d (side %d) has %d living men and made no checkpoint progress for %.1f host seconds (mean remaining %.1f cm). No member was engaging and this was not a receiving FIFO wait. Inspect the retained actual member/queue diagnostics."),
                    Id,Group->Team,Count,Host-Watch->At,Remaining));
        }
        for(auto It=OrdinaryTrafficV827.CreateIterator();It;++It)if(!Seen.Contains(It.Key()))It.RemoveCurrent();
        return true;
    }
#include "FRRUFrenchPlanReviewV87.inl"
    bool TickOrdinaryReturnV87(double Now)
    {
        auto* B=Battle();auto* PC=Controller.Get();auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        if(!B||!PC||!HUD)return Fail(TEXT("ordinary_return_live_context"),TEXT("The real winning instance or HUD was lost."));
        if(OrdinaryReturnStepV87==1)
        {
            if(B->Phase!=EFrontlinePhase::Victory||B->IsBattleOutroActive())return true;
            if(!FrontlineAchievementsV76::FinalizeBattleKillsV8217(B))return true;
            const auto* Won=B->GetEncounterCampaignMission();OrdinaryReturnWonV87=Won?Won->Id:INDEX_NONE;
            Check(TEXT("ordinary_return_has_committed_real_first_win"),Won&&WarfareFRRUCampaignV82::HasNewAssignmentReceiptV87(B,Won->Id)
                &&WarfareFRRUCampaignV82::CurrentRecord(B,OrdinaryReturnRecordV87)&&OrdinaryReturnRecordV87.ResearchPoints==OrdinaryOpeningResearchV87+2);
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            RequiredEvent(TEXT("ordinary_actual_win_before_public_return"),TEXT("This encounter's real native win and committed first-win token; prior fixture victories are not this result"),WarfareFRRUWorldV82::ObjectiveDiagnosis(B).ToSharedRef());
            HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));Event(TEXT("ordinary_public_report_return_to_operations"),B->Phase==EFrontlinePhase::Briefing);
            if(B->Phase!=EFrontlinePhase::Briefing)return Fail(TEXT("ordinary_public_report_return_to_operations"),B->LastMessage);
            const auto Cards=UnitIntroductionCards(B);auto& S=ScreenState(PC);OrdinaryReturnCardsV87=!Cards.IsEmpty();
            Check(TEXT("ordinary_next_assignment_matches_actual_dated_cards"),S.UnitIntroduction==OrdinaryReturnCardsV87&&S.NationalAssignmentNoticeV87==OrdinaryReturnCardsV87&&!S.Preparing);
            OrdinaryReturnAtV87=Now;OrdinaryReturnStepV87=2;
        }
        if(Now-OrdinaryReturnAtV87<.6||!Capture(TEXT("08-real-report-return-assignment")))return true;
        auto& S=ScreenState(PC);
        if(S.UnitIntroduction){HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));return true;}
        FFRRUCampaignSummaryV82 After;const bool Found=WarfareFRRUCampaignV82::CurrentRecord(B,After);const auto* Next=B->GetSelectedCampaignMission();
        Check(TEXT("ordinary_real_return_preserves_reward_next_mission_and_campaign"),Found&&After.Id==OrdinaryReturnRecordV87.Id
            &&After.ResearchPoints==OrdinaryReturnRecordV87.ResearchPoints&&After.Victories==OrdinaryReturnRecordV87.Victories&&After.Defeats==OrdinaryReturnRecordV87.Defeats
            &&After.MissionId==OrdinaryReturnRecordV87.MissionId&&Next&&WarfareFRRUCampaignV82::CalendarDate(B)==Next->CalendarDateV41
            &&WarfareFRRUCampaignV82::MissionBlockReason(B,After.MissionId).IsEmpty());
        Check(TEXT("ordinary_assignment_ack_returns_to_room_without_deploying"),B->Phase==EFrontlinePhase::Briefing&&!S.UnitIntroduction&&!S.NationalAssignmentNoticeV87&&!S.Preparing
            &&(!OrdinaryReturnCardsV87||(S.AcceptedNationalIntroCampaignV827==After.Id&&S.AcceptedNationalIntroMissionV827==After.MissionId))
            &&!WarfareFRRUCampaignV82::HasNewAssignmentReceiptV87(B,OrdinaryReturnWonV87));
        HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));
        Check(TEXT("ordinary_repeated_report_return_cannot_grant_reward_or_deploy"),B->Phase==EFrontlinePhase::Briefing&&WarfareFRRUCampaignV82::ResearchPoints(B)==After.ResearchPoints&&!ScreenState(PC).UnitIntroduction&&!ScreenState(PC).Preparing);
        auto Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("actualNextMission"),After.MissionId);Evidence->SetNumberField(TEXT("actualResearchPoints"),After.ResearchPoints);
        Evidence->SetBoolField(TEXT("actualNewAssignmentCardsShown"),OrdinaryReturnCardsV87);Evidence->SetBoolField(TEXT("usedLiveNaturalWin"),true);
        RequiredEvent(TEXT("ordinary_live_first_win_return_completed"),TEXT("Public report return and real card acknowledgements; no restored encounter or result injection"),Evidence);
        WriteReport();return false;
    }
    bool FrenchWholeOperationCompletionV87()const
    {
        if(!Scenario||Scenario->Probe!=EProbe::Ordinary||!Scenario->WholeOperation
            ||!FString(Scenario->Name).EndsWith(TEXT("-full")))return false;
        const auto* Mission=wfexp::tactical::FindMission(wfexpue::Narrow(FString(Scenario->Mission)));
        return Mission&&Mission->player==wfexp::Faction::French;
    }
    bool PublicCompletionPolicyV87()const
    {return NormalCampaignCompletionV87||FrenchWholeOperationCompletionV87();}
    bool KnownPositionFirePolicyV87()const
    {
        if(!PublicCompletionPolicyV87()||!Scenario||Scenario->Probe!=EProbe::Ordinary||VauxTacticsV87()||HoldDepartureProbeV87)return false;
        const auto* Mission=wfexp::tactical::FindMission(wfexpue::Narrow(FString(Scenario->Mission)));
        return Mission&&Mission->player==wfexp::Faction::French;
    }
    void ObserveNormalCompletionReadinessV87(const FString& Kind,const FString& Reason)
    {
        if(!PublicCompletionPolicyV87())return;
        ++NormalCompletionReadinessSamplesV87.FindOrAdd(Kind+TEXT("|")+(Reason.IsEmpty()?TEXT("ready"):Reason));
        NormalCompletionLastBlockReasonsV87.FindOrAdd(Kind)=Reason;
        if(const auto* LiveBattleV87=Battle())NormalCompletionLastReadinessHostV87=LiveBattleV87->Elapsed;
    }
    FString NormalCompletionRecruitDeferralV87(AWarfareFrontlineGameMode* LiveBattleV87,const FFRRUWorldViewV82& LiveViewV87)
    {
        NormalCompletionActiveSoftLimitV87=NormalCompletionLiveSectionsV87=NormalCompletionLivingPeopleV87=NormalCompletionHomeRelatedV87=0;
        NormalCompletionCapacityBasisV87.Reset();
        const auto* NativeMissionV87=wfexp::tactical::FindMission(wfexpue::Narrow(LiveViewV87.MissionId));
        if(!NativeMissionV87)return TEXT("native_capacity_scope_unavailable");
        TSet<FString> ScheduledPostsV87;ScheduledPostsV87.Add(TEXT("home"));ScheduledPostsV87.Add(TEXT("assembly"));ScheduledPostsV87.Add(LiveViewV87.TargetNode);
        for(const auto& NativeStageV87:NativeMissionV87->stages)if(wfexpue::Wide(NativeStageV87.id)==LiveViewV87.StageId)
            for(const auto& RequiredPostV87:NativeStageV87.mustOwn)ScheduledPostsV87.Add(wfexpue::Wide(RequiredPostV87));
        const wfexp::tactical::NodeDefinition* NativeHomeV87=nullptr;
        for(const auto& NativePostV87:NativeMissionV87->nodes)
        {
            if(NativePostV87.id=="home")NativeHomeV87=&NativePostV87;
            if(NativePostV87.capacity>0&&ScheduledPostsV87.Contains(wfexpue::Wide(NativePostV87.id)))
            {NormalCompletionActiveSoftLimitV87+=NativePostV87.capacity;NormalCompletionCapacityBasisV87.Add(wfexpue::Wide(NativePostV87.id),NativePostV87.capacity);}
        }
        if(!NativeHomeV87||NativeHomeV87->capacity<=0||NormalCompletionActiveSoftLimitV87<=0)return TEXT("native_home_capacity_unavailable");
        const FVector NativeHomePointV87(NativeHomeV87->position.x,NativeHomeV87->position.y,NativeHomeV87->position.z);
        for(const auto& ActualGroupV87:LiveBattleV87->Squads)if(ActualGroupV87.Team==0&&ActualGroupV87.LivingCount()>0)
        {
            ++NormalCompletionLiveSectionsV87;NormalCompletionLivingPeopleV87+=ActualGroupV87.LivingCount();
            const AWarfareFrontlineSoldier* ActualWitnessV87=nullptr;bool BodyAtHomeV87=false;
            for(const auto& WeakMemberV87:ActualGroupV87.Members)if(const auto* ActualMemberV87=WeakMemberV87.Get();ActualMemberV87&&ActualMemberV87->IsAlive())
            {if(!ActualWitnessV87)ActualWitnessV87=ActualMemberV87;
                BodyAtHomeV87|=ActualMemberV87->bFieldEntered&&FVector::Dist2D(ActualMemberV87->Feet(),NativeHomePointV87)<=NativeHomeV87->radius+56.5;}
            FString ActualCurrentV87,ActualTargetV87,ActualNextV87;bool IngressWaitingV87=false;
            if(ActualWitnessV87)
            {const auto ReadbackV87=WarfareFRRUWorldV82::MovementDiagnosis(ActualWitnessV87,false);
                ReadbackV87->TryGetStringField(TEXT("currentNode"),ActualCurrentV87);ReadbackV87->TryGetStringField(TEXT("targetNode"),ActualTargetV87);
                ReadbackV87->TryGetStringField(TEXT("nextNode"),ActualNextV87);ReadbackV87->TryGetBoolField(TEXT("receivingAdmissionWaiting"),IngressWaitingV87);}
            if(BodyAtHomeV87||(ActualCurrentV87.IsEmpty()&&ActualTargetV87==TEXT("home"))||(ActualNextV87==TEXT("home")&&IngressWaitingV87))++NormalCompletionHomeRelatedV87;
        }
        if(NormalCompletionHomeRelatedV87>=NativeHomeV87->capacity)return TEXT("actual_home_ingress_busy");
        return FString();
    }
    bool VauxTacticsV87()const
    {return Scenario&&Scenario->Probe==EProbe::Ordinary&&FString(Scenario->Name)==TEXT("vaux-full")
        &&FParse::Param(FCommandLine::Get(),TEXT("FRRUVauxTactics"));}
    const TCHAR* OrdinaryCasePolicyNameV87()const
    {
        if(VauxTacticsV87())return TEXT("vaux_public_support_staged_recruit_v87");
        if(Scenario&&Scenario->Probe==EProbe::Ordinary)
        {
            if(FString(Scenario->Mission)==TEXT("fr.mort_homme"))return TEXT("mort_rifle_hotchkiss_3to1_contact_rally_v87");
            if(FString(Scenario->Mission)==TEXT("ru.galicia"))return TEXT("galicia_one_pioneer_maxim_rifle_1to3_contact_rally_v87");
            if(FString(Scenario->Name)==TEXT("vaux-full"))return TEXT("vaux_shared_recruits_contact_rally_v87");
        }
        return TEXT("legacy_shared_cycle_v827");
    }
    bool RefuseVauxFireV87(EFrontlineSupport Kind,const TCHAR* Reason)
    {if(VauxTacticsV87())++VauxFireRefusalsV87.FindOrAdd(FString::Printf(TEXT("%d/%s"),int32(Kind),Reason));return false;}
    bool OrdinaryArtilleryRouteSafeV87(AWarfareFrontlineGameMode* NBattle,const FVector& Target,EFrontlineSupport Support=EFrontlineSupport::Artillery,TArray<int32>* FutureHoldsV87=nullptr)
    {
        if(!NBattle->IsFireSupportAreaSafe(Target,Support,0))
        {if(KnownPositionFirePolicyV87()&&Support==EFrontlineSupport::Artillery)++KnownPositionCurrentFriendlyRefusalsV87;
            return RefuseVauxFireV87(Support,TEXT("current_support_area_not_safe"));}
        // Review strategy only. A normal fixed four-group mission remains
        // capable of friendly fire after troops enter its ordered ground.
        // Avoid firing into an already known friendly destination which could
        // be reached before the final group, including an optimistic allowance
        // for a current traffic wait clearing. This grants no movement/arrival.
        const bool Mortar=Support==EFrontlineSupport::Mortar;
        const auto Delay=NBattle->FireSupportDelayRange(Support,Target,0);
        const double LastImpact=Delay.Y+(FrontlineFireSupportV26::Salvos-1)*(Mortar?6.2:5.8)
            +(Mortar?FrontlineFireSupportV26::NormalRoundsPerSalvo:FrontlineFireSupportV26::NormalRoundsPerSalvo-1)*(Mortar?1.05:.85)+2.5;
        const auto Extent=NBattle->FireSupportScatterExtent(Support,0);
        const double Margin=(Mortar?FrontlineFireSupportV26::MortarBlastRadius:FrontlineFireSupportV26::FieldGunBlastRadius)+160.;
        const auto Decline=[&]()
        {
            // Public fixed-ground fire may precede friendly arrival. This
            // forecast is diagnostic only in the scoped French completion
            // policy; current friendly footprint was still checked above.
            if(KnownPositionFirePolicyV87()&&Support==EFrontlineSupport::Artillery&&!FutureHoldsV87)
            {++KnownPositionFutureRouteRiskV87;return true;}
            if(Mortar)++VauxMortarRouteDeclinesV87;else ++OrdinaryArtilleryRouteDeclinesV87;return false;
        };
        const auto Inside=[&](const FVector& Point,float Radius)
        {return FMath::Abs(Point.X-Target.X)<=Extent.X+Margin+Radius&&FMath::Abs(Point.Y-Target.Y)<=Extent.Y+Margin+Radius;};
        for(const auto& Group:NBattle->Squads)if(Group.Team==0&&Group.LivingCount()>0&&Group.bAdvancing&&!Group.bHoldPosition)
        {
            const AWarfareFrontlineSoldier* ForecastRouteWitnessV87=nullptr;
            for(const auto& Weak:Group.Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->bFieldEntered){ForecastRouteWitnessV87=Unit;break;}
            if(!ForecastRouteWitnessV87)continue;const auto Movement=WarfareFRRUWorldV82::MovementDiagnosis(ForecastRouteWitnessV87,false);
            FString Next,Destination;Movement->TryGetStringField(TEXT("nextNode"),Next);Movement->TryGetStringField(TEXT("targetNode"),Destination);
            if(VauxTacticsV87())
            {
                FString Current;Movement->TryGetStringField(TEXT("currentNode"),Current);
                const auto* Ground=wfexp::scene::ForMission("fr.vaux");const TCHAR* Risk=nullptr;
                const wfexp::scene::RoutePolyline* CurrentLeg=nullptr;
                if(Ground)for(const auto& Route:Ground->routes)if((Current==UTF8_TO_TCHAR(Route.from)&&Next==UTF8_TO_TCHAR(Route.to))
                    ||(Current==UTF8_TO_TCHAR(Route.to)&&Next==UTF8_TO_TCHAR(Route.from))){CurrentLeg=&Route;break;}
                for(const auto& Weak:Group.Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->bFieldEntered)
                {
                    // Original gait cruise, original maximal load/pace and legal
                    // Rally boost bound reach. Terrain and hazards only reduce
                    // these Vaux Rifle/MG/Mills/Pioneer speeds. No clock is changed.
                    // Both gaits reach cruise before 89 cm; avoid overflow in
                    // the public braking formula's float squared-speed term.
                    const double Cruise=FrontlineMovementV28::TravelSpeed(1000.f,0.f,Unit->GetRunMotion()!=nullptr);
                    const double Speed=Cruise*Unit->GetMovementScale()*1.08*1.035*1.2;
                    const double ReachTimeBudget=Speed*LastImpact;double Budget=ReachTimeBudget;
                    const FVector2D Actual(Unit->Feet());
                    if(CurrentLeg&&CurrentLeg->points.size()>=2)
                    {
                        const bool Forward=Current==UTF8_TO_TCHAR(CurrentLeg->from);
                        const auto At=[&](int32 I){const auto& P=CurrentLeg->points[Forward?I:int32(CurrentLeg->points.size())-1-I];return FVector2D(P.x,P.y);};
                        double Best=MAX_dbl;int32 Segment=1;FVector2D Projection=Actual;
                        for(int32 I=1;I<int32(CurrentLeg->points.size());++I)
                        {const FVector2D A=At(I-1),Delta=At(I)-A;const double Len2=Delta.SizeSquared();
                            const double T=Len2>1.e-8?FMath::Clamp(FVector2D::DotProduct(Actual-A,Delta)/Len2,0.,1.):0.;
                            const FVector2D P=A+Delta*T;const double D=FVector2D::DistSquared(Actual,P);
                            if(D<Best){Best=D;Segment=I;Projection=P;}}
                        const FVector2D Reach=Extent+FVector2D(Margin+850.+FMath::Sqrt(Best));
                        const FBox2D Area(FVector2D(Target)-Reach,FVector2D(Target)+Reach);
                        const auto PrefixHit=[&](const FVector2D& A,const FVector2D& Z)
                        {const double Length=FVector2D::Distance(A,Z);if(Budget<=0.)return false;
                            const FVector2D End=Length>1.e-8?A+(Z-A)*FMath::Min(1.,Budget/Length):Z;
                            Budget=FMath::Max(0.,Budget-Length);return FMath::LineBoxIntersection2D(Area,A,End);};
                        bool Hit=PrefixHit(Actual,Projection);FVector2D Previous=Projection;
                        for(int32 I=Segment;!Hit&&Budget>0.&&I<int32(CurrentLeg->points.size());++I)
                        {const FVector2D Point=At(I);Hit=PrefixHit(Previous,Point);Previous=Point;}
                        if(Hit)Risk=TEXT("current_leg_within_last_impact");
                    }
                    else if(!Current.IsEmpty()&&Current!=Next)Risk=TEXT("unknown_current_registered_leg");
                    for(const FString& Id:{Next,Destination})
                    {FVector Post;float Radius=0;if(!Id.IsEmpty()&&WarfareFRRUWorldV82::NodePose(NBattle,Id,Post,Radius)
                        &&Inside(Post,Radius)&&FVector::Dist2D(Unit->Feet(),Post)<=ReachTimeBudget+Radius)Risk=TEXT("known_post_within_last_impact");}
                    if(Risk)break;
                }
                if(Risk)
                {if(FutureHoldsV87)FutureHoldsV87->AddUnique(Group.Id);
                    else {Decline();return RefuseVauxFireV87(Support,Risk);}}
                continue;
            }
            for(const FString& Id:{Next,Destination})
            {
                FVector Post;float Radius=0;if(Id.IsEmpty()||!WarfareFRRUWorldV82::NodePose(NBattle,Id,Post,Radius)||!Inside(Post,Radius))continue;
                for(const auto& Weak:Group.Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->bFieldEntered)
                {
                    const double OptimisticSpeed=FrontlinePacingV24::IngressSpeedCm*2.*FMath::Max(1.f,Unit->GetMovementScale());
                    if(FVector::Dist2D(Unit->Feet(),Post)<=OptimisticSpeed*LastImpact+Radius)
                    return Decline();
                }
            }
        }
        return !FutureHoldsV87||FutureHoldsV87->IsEmpty();
    }
    static uint64 OrdinaryFireRoundKeyV87(const FFrontlineStrike& Strike)
    {return (uint64(uint32(Strike.FireMissionId))<<32)|(uint64(uint16(Strike.SalvoIndex))<<16)|uint16(Strike.RoundIndex);}
    bool OrdinaryObservedEnemyV87(AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* Enemy)const
    {
        if(!B||!Enemy||Enemy->Team!=1||!Enemy->IsActiveCombatant())return false;
        for(const AWarfareFrontlineSoldier* Friendly:B->Units)if(Friendly&&Friendly->Team==0&&Friendly->IsActiveCombatant()
            &&FVector::Dist2D(Friendly->Feet(),Enemy->Feet())<=FrontlineWeapons::AdvanceContactRangeFor(Friendly)*B->EncounterVisibilityScale()
            &&B->HasSight(Friendly,Enemy))return true;
        return false;
    }
    bool TickVauxPublicTacticsV87(double Now,const FFRRUWorldViewV82& View)
    {
        auto* B=Battle();if(!B)return true;
        if((PublicCompletionPolicyV87()||OrdinaryRecruitsV827<24)&&Now>=OrdinaryRecruitRetryV827&&B->Elapsed>=VauxNextRecruitHostV87)
        {
            OrdinaryRecruitRetryV827=Now+1.;int32 Active=0,HomeRelated=0;bool PioneerAlive=false;
            FVector Home;float HomeRadius=0;const bool HasHome=WarfareFRRUWorldV82::NodePose(B,TEXT("home"),Home,HomeRadius);
            for(const auto& Group:B->Squads)if(Group.Team==0&&Group.LivingCount()>0)
            {
                ++Active;PioneerAlive|=Group.Role==EFrontlineRole::Pioneer;
                const AWarfareFrontlineSoldier* Witness=nullptr;bool BodyAtHome=false;
                for(const auto& Weak:Group.Members)if(const auto* U=Weak.Get();U&&U->IsAlive())
                {if(!Witness)Witness=U;BodyAtHome|=HasHome&&U->bFieldEntered&&FVector::Dist2D(U->Feet(),Home)<=HomeRadius+56.5;}
                FString Current,Target;
                if(Witness){const auto D=WarfareFRRUWorldV82::MovementDiagnosis(Witness,false);
                    D->TryGetStringField(TEXT("currentNode"),Current);D->TryGetStringField(TEXT("targetNode"),Target);}
                HomeRelated+=((BodyAtHome||(Current.IsEmpty()&&Target==TEXT("home")))?1:0);
            }
            if(HasHome&&(PublicCompletionPolicyV87()||Active<8)&&HomeRelated<2)
            {
                const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Rifle,
                    EFrontlineRole::Mills,EFrontlineRole::Rifle,EFrontlineRole::Lewis};
                const auto Role=View.RequiredRole==int32(EFrontlineRole::Pioneer)&&!PioneerAlive?EFrontlineRole::Pioneer:Roles[OrdinaryRecruitsV827%UE_ARRAY_COUNT(Roles)];
                const FString PublicReasonV87=B->RecruitBlockReason(Role,0);
                ObserveNormalCompletionReadinessV87(FString::Printf(TEXT("recruit.%d"),int32(Role)),PublicReasonV87);
                if(PublicReasonV87.IsEmpty())
                {
                    const int32 Id=Recruit(Role,0);if(Id!=INDEX_NONE)
                    {++OrdinaryRecruitsV827;VauxNextRecruitHostV87=B->Elapsed+18.f;
                        const auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("actualLivingPlayerSectionsBefore"),Active);
                        Row->SetNumberField(TEXT("actualHomeRelatedSectionsBefore"),HomeRelated);Row->SetStringField(TEXT("casePolicyV87"),OrdinaryCasePolicyNameV87());}
                }
            }
            else
            {
                ++VauxRecruitDeferralsV87;
                if(PublicCompletionPolicyV87())
                {++NormalCompletionRecruitDeferralsV87;++NormalCompletionDeferralReasonsV87.FindOrAdd(HasHome?TEXT("actual_home_ingress_busy"):TEXT("native_home_capacity_unavailable"));}
            }
        }
        if(Now<OrdinarySupportRetryV827)return true;OrdinarySupportRetryV827=Now+2.;
        FVector Objective;float Radius=0;if(!WarfareFRRUWorldV82::NodePose(B,View.TargetNode,Objective,Radius))return true;
        const AWarfareFrontlineSoldier* ArtilleryPoint=nullptr;const AWarfareFrontlineSoldier* MortarPoint=nullptr;
        const AWarfareFrontlineSoldier* SmokePoint=nullptr;const AWarfareFrontlineSoldier* RallyPoint=nullptr;
        double ArtilleryScore=MAX_dbl,MortarScore=MAX_dbl,SmokeScore=-MAX_dbl,RallyScore=-MAX_dbl;
        bool ObservedThreat=false,OwnFirePending=false;
        for(const auto& Strike:B->Strikes)if(Strike.Team==0&&!Strike.bResolved
            &&(Strike.SourceV65.Support==int32(EFrontlineSupport::Artillery)||Strike.SourceV65.Support==int32(EFrontlineSupport::Mortar)))OwnFirePending=true;
        const FString ArtilleryBlockV87=B->SupportBlockReason(EFrontlineSupport::Artillery,0);
        const FString MortarBlockV87=B->SupportBlockReason(EFrontlineSupport::Mortar,0);
        ObserveNormalCompletionReadinessV87(TEXT("support.artillery"),ArtilleryBlockV87);
        ObserveNormalCompletionReadinessV87(TEXT("support.mortar"),MortarBlockV87);
        const bool ArtilleryReady=!OwnFirePending&&(PublicCompletionPolicyV87()||OrdinaryArtilleryV827<6)&&ArtilleryBlockV87.IsEmpty();
        const bool MortarReady=!OwnFirePending&&MortarBlockV87.IsEmpty();
        if(!OwnFirePending)
        {if(!ArtilleryReady)RefuseVauxFireV87(EFrontlineSupport::Artillery,!PublicCompletionPolicyV87()&&OrdinaryArtilleryV827>=6?TEXT("review_artillery_cap"):TEXT("public_support_not_ready"));
            if(!MortarReady)RefuseVauxFireV87(EFrontlineSupport::Mortar,TEXT("public_support_not_ready"));}
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==1&&U->IsActiveCombatant()
            &&FVector::Dist2D(U->Feet(),Objective)<=4500.&&OrdinaryObservedEnemyV87(B,U))
        {
            ObservedThreat=true;const double Score=FVector::DistSquared2D(U->Feet(),Objective);
            if(ArtilleryReady&&Score<ArtilleryScore)
            {if(B->IsFireSupportAreaSafe(U->Feet(),EFrontlineSupport::Artillery,0)){ArtilleryPoint=U;ArtilleryScore=Score;}
                else RefuseVauxFireV87(EFrontlineSupport::Artillery,TEXT("current_friendly_footprint"));}
            if(MortarReady&&Score<MortarScore)
            {if(B->IsFireSupportAreaSafe(U->Feet(),EFrontlineSupport::Mortar,0)){MortarPoint=U;MortarScore=Score;}
                else RefuseVauxFireV87(EFrontlineSupport::Mortar,TEXT("current_friendly_footprint"));}
        }
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsActiveCombatant())
        {
            const auto* Group=B->FindSquad(U->SquadId);const double Distance=FVector::Dist2D(U->Feet(),Objective);
            const bool Stationed=!U->bMoving&&Distance<=Radius;
            const bool Approach=Group&&Group->bAdvancing&&!Group->bHoldPosition&&U->bMoving&&Distance<=3000.;
            const double Score=(U->bEngaging?2000.:0.)+U->Suppression*1000.-Distance;
            if(ObservedThreat&&(U->bEngaging||Stationed||Approach)&&!B->IsInFriendlySmoke(U->Feet(),0)&&Score>SmokeScore)
            {SmokePoint=U;SmokeScore=Score;}
            if((U->bEngaging||Stationed)&&U->Suppression>0.f&&!U->IsRallied()&&Score>RallyScore)
            {RallyPoint=U;RallyScore=Score;}
        }
        const auto Request=[&](EFrontlineSupport Support,const AWarfareFrontlineSoldier* Target,const TCHAR* Name,int32& Count)
        {
            if(!Target)return false;
            const FString PublicReasonV87=B->SupportBlockReason(Support,0);
            ObserveNormalCompletionReadinessV87(Support==EFrontlineSupport::Smoke?TEXT("support.smoke"):TEXT("support.rally"),PublicReasonV87);
            if(!PublicReasonV87.IsEmpty())return false;
            const float SuppressionBefore=Target->Suppression;
            const bool Accepted=B->RequestSupport(Support,Target->Feet(),0);Event(Name,Accepted);
            const auto Row=Commands.Last()->AsObject();Row->SetStringField(TEXT("casePolicyV87"),OrdinaryCasePolicyNameV87());
            Row->SetStringField(TEXT("actualTargetActor"),Target->GetPathName());Row->SetStringField(TEXT("actualRequestedPoint"),Target->Feet().ToString());
            Row->SetNumberField(TEXT("actualTargetSquad"),Target->SquadId);Row->SetNumberField(TEXT("suppressionBeforeRequest"),SuppressionBefore);
            Row->SetBoolField(TEXT("targetIsOwnActualUnit"),Target->Team==0);
            if(Accepted)++Count;return Accepted;
        };
        // Serial actual public Hold/fire/release windows, never a direct bypass.
        if(!ObservedThreat)RefuseVauxFireV87(EFrontlineSupport::Artillery,TEXT("no_observed_enemy"));
        if(ObservedThreat&&!OwnFirePending&&!ArtilleryPoint&&!MortarPoint)
            RefuseVauxFireV87(EFrontlineSupport::Artillery,TEXT("no_ready_current_safe_target"));
        if(OwnFirePending)RefuseVauxFireV87(EFrontlineSupport::Artillery,TEXT("existing_own_pending_impacts"));
        // Prefer the nearer legal observed target. Equal target scores favor
        // the support with fewer actual successful calls; neither type owns
        // a permanent priority, and the other's legal window is still tried.
        const bool MortarFirst=MortarPoint&&(!ArtilleryPoint||MortarScore<ArtilleryScore
            ||(MortarScore==ArtilleryScore&&VauxMortarCallsV87<OrdinaryArtilleryV827));
        for(const auto Kind:{MortarFirst?EFrontlineSupport::Mortar:EFrontlineSupport::Artillery,
            MortarFirst?EFrontlineSupport::Artillery:EFrontlineSupport::Mortar})
        {const auto* Point=Kind==EFrontlineSupport::Mortar?MortarPoint:ArtilleryPoint;
            if(Point&&TryOrdinaryFireWindowV87(B,Point,Kind))return true;}
        Request(EFrontlineSupport::Smoke,SmokePoint,TEXT("vaux_public_smoke_actual_contact_or_post"),VauxSmokeCallsV87);
        if(PublicCompletionPolicyV87()||OrdinaryRallyV827<12)Request(EFrontlineSupport::Rally,RallyPoint,TEXT("vaux_public_rally_actual_suppression"),OrdinaryRallyV827);
        return true;
    }
    void ReleaseOrdinaryFireWindowV87(const TCHAR* Reason)
    {
        if(OrdinaryFireWindowHoldsV87.IsEmpty())return;
        auto* B=Battle();if(B&&B->GetEncounterId()==OrdinaryFireWindowEncounterV87&&UGameplayStatics::IsGamePaused(B))return;
        TArray<int32> Ids=OrdinaryFireWindowHoldsV87.Array();Ids.Sort();
        for(const int32 Id:Ids)
        {
            const auto* Group=B&&B->GetEncounterId()==OrdinaryFireWindowEncounterV87?B->FindSquad(Id):nullptr;
            const bool Gone=!Group||Group->LivingCount()<=0||!Group->bHoldPosition;
            const bool Terminal=!B||B->GetEncounterId()!=OrdinaryFireWindowEncounterV87||B->Phase!=EFrontlinePhase::Fighting;
            const bool Released=!Gone&&!Terminal&&WarfareFRRUWorldV82::HoldSquad(B,Id,false);
            Event(TEXT("ordinary_public_artillery_window_release"),Gone||Released);
            auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("squad"),Id);Row->SetNumberField(TEXT("fireMissionId"),OrdinaryFireWindowMissionV87);
            if(VauxTacticsV87())Row->SetNumberField(TEXT("actualSupportType"),int32(OrdinaryFireWindowSupportV87));
            Row->SetStringField(TEXT("reason"),Reason);Row->SetBoolField(TEXT("actualPublicRelease"),Released);Row->SetBoolField(TEXT("alreadyDeadOrUnheld"),Gone);
            Row->SetBoolField(TEXT("terminalPublicApiUnavailable"),!Gone&&Terminal);
            if(Gone||Released||Terminal)OrdinaryFireWindowHoldsV87.Remove(Id);
            if(!Gone&&Terminal)OrdinaryFireWindowTerminalHoldsV87.Add(Id);
        }
    }
    bool TickOrdinaryFireWindowV87()
    {
        if(OrdinaryFireWindowRoundsV87.IsEmpty())
        {if(VauxTacticsV87()&&!OrdinaryFireWindowHoldsV87.IsEmpty())
            {ReleaseOrdinaryFireWindowV87(TEXT("retry_failed_window_public_release"));return !OrdinaryFireWindowHoldsV87.IsEmpty();}return false;}
        auto* B=Battle();if(!B||B->GetEncounterId()!=OrdinaryFireWindowEncounterV87||B->Phase!=EFrontlinePhase::Fighting)
        {ReleaseOrdinaryFireWindowV87(TEXT("encounter_ended_no_hidden_state_write"));OrdinaryFireWindowRoundsV87.Reset();return false;}
        if(UGameplayStatics::IsGamePaused(B))return true;
        int32 Resolved=0,LeftActive=0,Pending=0;
        for(auto& Round:OrdinaryFireWindowRoundsV87)
        {
            const FFrontlineStrike* Active=B->Strikes.FindByPredicate([&](const FFrontlineStrike& Strike)
                {return Strike.Team==0&&Strike.SourceV65.Support==int32(OrdinaryFireWindowSupportV87)&&OrdinaryFireRoundKeyV87(Strike)==Round.Key;});
            if(Active&&Active->bResolved){Round.Value=true;++Resolved;}
            else if(!Active){Round.Value=true;++LeftActive;}
            else {++Pending;}
        }
        if(Pending>0)return true;
        Event(TEXT("ordinary_artillery_window_actual_rounds_finished"),true);
        auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("fireMissionId"),OrdinaryFireWindowMissionV87);
        if(VauxTacticsV87())Row->SetNumberField(TEXT("actualSupportType"),int32(OrdinaryFireWindowSupportV87));
        Row->SetNumberField(TEXT("trackedActualRounds"),OrdinaryFireWindowRoundsV87.Num());Row->SetNumberField(TEXT("resolvedInActiveArray"),Resolved);Row->SetNumberField(TEXT("leftActiveArray"),LeftActive);
        ReleaseOrdinaryFireWindowV87(TEXT("all_tracked_actual_rounds_resolved_or_left_active"));
        if(!OrdinaryFireWindowHoldsV87.IsEmpty())return true;
        OrdinaryFireWindowRoundsV87.Reset();return false;
    }
    bool TryOrdinaryFireWindowV87(AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* Enemy,EFrontlineSupport FireSupport=EFrontlineSupport::Artillery)
    {
        const auto Refuse=[&](const TCHAR* Reason){return RefuseVauxFireV87(FireSupport,Reason);};
        if(FireSupport!=EFrontlineSupport::Artillery&&(!VauxTacticsV87()||FireSupport!=EFrontlineSupport::Mortar))return Refuse(TEXT("unsupported_window_type"));
        if(OrdinaryFireWindowUsedV87&&!VauxTacticsV87()&&!PublicCompletionPolicyV87())return Refuse(TEXT("legacy_single_window_already_used"));
        if(!OrdinaryFireWindowHoldsV87.IsEmpty()||!OrdinaryFireWindowRoundsV87.IsEmpty())return Refuse(TEXT("active_public_window_or_release_pending"));
        if(!Scenario||FString(Scenario->Name)!=TEXT("vaux-full")
            ||!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B))return Refuse(TEXT("scope_not_ready"));
        if(!B->SupportBlockReason(FireSupport,0).IsEmpty())return Refuse(TEXT("public_support_not_ready"));
        if(!OrdinaryObservedEnemyV87(B,Enemy))return Refuse(TEXT("no_current_actual_observer"));
        const auto View=WarfareFRRUWorldV82::View(B);
        if(View.MissionId!=Scenario->Mission||View.StoryPending)return Refuse(TEXT("mission_or_story_scope_changed"));
        const FVector Target=Enemy->Feet();const auto Extent=B->FireSupportScatterExtent(FireSupport,0);
        const bool Mortar=FireSupport==EFrontlineSupport::Mortar;
        const double Margin=(Mortar?FrontlineFireSupportV26::MortarBlastRadius:FrontlineFireSupportV26::FieldGunBlastRadius)+160.;
        const auto Inside=[&](const FVector& At,float Radius)
            {return FMath::Abs(At.X-Target.X)<=Extent.X+Margin+Radius&&FMath::Abs(At.Y-Target.Y)<=Extent.Y+Margin+Radius;};
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsAlive()&&Inside(U->Feet(),0))return Refuse(TEXT("current_friendly_footprint"));
        const auto Delay=B->FireSupportDelayRange(FireSupport,Target,0);
        const double LastImpact=Delay.Y+(FrontlineFireSupportV26::Salvos-1)*(Mortar?6.2:5.8)
            +(Mortar?FrontlineFireSupportV26::NormalRoundsPerSalvo:FrontlineFireSupportV26::NormalRoundsPerSalvo-1)*(Mortar?1.05:.85)+2.5;
        TArray<int32> ToHold;
        if(VauxTacticsV87())
        {
            for(const auto& Strike:B->Strikes)if(Strike.Team==0&&!Strike.bResolved
                &&(Strike.SourceV65.Support==int32(EFrontlineSupport::Artillery)||Strike.SourceV65.Support==int32(EFrontlineSupport::Mortar)))return Refuse(TEXT("existing_own_pending_impacts"));
            const bool ForecastSafeV87=OrdinaryArtilleryRouteSafeV87(B,Target,FireSupport,&ToHold);
            if(!ForecastSafeV87&&ToHold.IsEmpty())return Refuse(TEXT("pre_request_safety_unresolved"));
            for(const int32 Id:ToHold)
            {const auto* Group=B->FindSquad(Id);const AWarfareFrontlineSoldier* Witness=nullptr;
                if(Group)for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->bFieldEntered){Witness=Unit;break;}
                if(!Group||Group->bHoldPosition||!Witness)return Refuse(TEXT("hold_participant_not_available"));
                const auto D=WarfareFRRUWorldV82::MovementDiagnosis(Witness,false);
                if(!D->GetBoolField(TEXT("ordinaryOwnsSection"))||D->GetBoolField(TEXT("ordinaryHoldWasRemembered")))return Refuse(TEXT("hold_participant_not_ordinary"));}
        }
        else for(const auto& Group:B->Squads)if(Group.Team==0&&Group.LivingCount()>0&&Group.bAdvancing&&!Group.bHoldPosition)
        {
            const AWarfareFrontlineSoldier* Witness=nullptr;
            for(const auto& Weak:Group.Members)if(const auto* U=Weak.Get();U&&U->IsAlive()&&U->bFieldEntered){Witness=U;break;}
            if(!Witness)continue;const auto D=WarfareFRRUWorldV82::MovementDiagnosis(Witness,false);bool Future=false;
            FString Next,Destination;D->TryGetStringField(TEXT("nextNode"),Next);D->TryGetStringField(TEXT("targetNode"),Destination);
            for(const FString& Id:{Next,Destination})
            {
                FVector Post;float Radius=0;if(Id.IsEmpty()||!WarfareFRRUWorldV82::NodePose(B,Id,Post,Radius)||!Inside(Post,Radius))continue;
                for(const auto& Weak:Group.Members)if(const auto* U=Weak.Get();U&&U->IsAlive()&&U->bFieldEntered)
                    Future|=FVector::Dist2D(U->Feet(),Post)<=FrontlinePacingV24::IngressSpeedCm*2.*FMath::Max(1.f,U->GetMovementScale())*LastImpact+Radius;
            }
            if(!Future)continue;
            // Plain non-fallback holds cannot freeze advancing work/withdrawal
            // participants; active fire/reserve/arrival tasks were excluded above.
            if(!D->GetBoolField(TEXT("ordinaryOwnsSection"))||D->GetBoolField(TEXT("ordinaryHoldWasRemembered")))return false;
            ToHold.Add(Group.Id);
        }
        const bool NeedsPublicHoldV87=!ToHold.IsEmpty();
        if(!NeedsPublicHoldV87&&!VauxTacticsV87())return false;
        if(NeedsPublicHoldV87)
        {
            const auto* Gate=wfexp::tactical::FindMission(wfexpue::Narrow(View.MissionId));
            bool PlainHold=false;if(Gate)for(const auto& Stage:Gate->stages)if(wfexpue::Wide(Stage.id)==View.StageId)
                PlainHold=Stage.kind==wfexp::tactical::GateKind::HoldNode&&Stage.requiredRole<0;
            const auto Support=WarfareFRRUWorldV82::SupportDiagnosis(B);
            if(!OrdinaryHoldPreservedV827||!PlainHold||View.Fallback||View.ReservePending||View.FirePhase!=0||!Support->GetBoolField(TEXT("contextValid"))
                ||!Support->GetArrayField(TEXT("arrivalHoldsV87")).IsEmpty()||!Support->GetArrayField(TEXT("assignedFollowupSquadsV87")).IsEmpty())return Refuse(TEXT("native_hold_participants_or_task_protected"));
        }
        OrdinaryFireWindowSupportV87=FireSupport;OrdinaryFireWindowEncounterV87=B->GetEncounterId();OrdinaryFireWindowMissionV87=INDEX_NONE;
        for(const int32 Id:ToHold)
        {
            if(VauxTacticsV87())++VauxPublicHoldAttemptsV87;
            const bool Held=WarfareFRRUWorldV82::HoldSquad(B,Id,true);if(Held){OrdinaryFireWindowHoldsV87.Add(Id);if(VauxTacticsV87())VauxManualHoldIssuedV87=true;}
            Event(TEXT("ordinary_public_hold_for_fixed_artillery"),Held);Commands.Last()->AsObject()->SetNumberField(TEXT("squad"),Id);
            Commands.Last()->AsObject()->SetNumberField(TEXT("actualSupportType"),int32(FireSupport));
            if(!Held){ReleaseOrdinaryFireWindowV87(TEXT("public_hold_refused"));return Refuse(TEXT("public_hold_refused"));}
        }
        if(!OrdinaryObservedEnemyV87(B,Enemy)||!OrdinaryArtilleryRouteSafeV87(B,Target,FireSupport))
        {ReleaseOrdinaryFireWindowV87(TEXT("original_safety_or_current_observation_refused"));return Refuse(TEXT("post_hold_safety_or_observation_refused"));}
        TSet<int32> Before;for(const auto& Strike:B->Strikes)Before.Add(Strike.FireMissionId);
        if(VauxTacticsV87())++VauxPublicFireRequestAttemptsV87;
        const bool Fired=B->RequestSupport(FireSupport,Target,0);
        Event(VauxTacticsV87()&&!NeedsPublicHoldV87?TEXT("vaux_public_safe_fire_without_temporary_hold"):TEXT("ordinary_public_artillery_after_temporary_hold"),Fired);
        if(!Fired){ReleaseOrdinaryFireWindowV87(TEXT("actual_support_request_refused"));return Refuse(TEXT("public_support_request_refused"));}
        if(NeedsPublicHoldV87)OrdinaryFireWindowUsedV87=true;
        if(Mortar)++VauxMortarCallsV87;else ++OrdinaryArtilleryV827;
        if(VauxTacticsV87()){if(NeedsPublicHoldV87)++VauxHeldFireWindowsV87;else ++VauxDirectSafeFireWindowsV87;}
        auto Row=Commands.Last()->AsObject();Row->SetStringField(TEXT("fixedObservedTarget"),Target.ToString());Row->SetNumberField(TEXT("actualSupportType"),int32(FireSupport));
        Row->SetNumberField(TEXT("conservativeLastImpactDelayUpperBound"),LastImpact);Row->SetBoolField(TEXT("actualPublicHoldWindow"),NeedsPublicHoldV87);
        TArray<TSharedPtr<FJsonValue>> Rounds;bool OneMission=true;
        for(const auto& Strike:B->Strikes)if(Strike.Team==0&&Strike.SourceV65.Support==int32(FireSupport)
            &&Strike.FireMissionId!=INDEX_NONE&&!Before.Contains(Strike.FireMissionId))
        {
            if(OrdinaryFireWindowMissionV87==INDEX_NONE)OrdinaryFireWindowMissionV87=Strike.FireMissionId;
            OneMission&=OrdinaryFireWindowMissionV87==Strike.FireMissionId;OrdinaryFireWindowRoundsV87.Add(OrdinaryFireRoundKeyV87(Strike),false);
            auto Round=MakeShared<FJsonObject>();Round->SetNumberField(TEXT("fireMissionId"),Strike.FireMissionId);Round->SetNumberField(TEXT("salvo"),Strike.SalvoIndex);
            Round->SetNumberField(TEXT("round"),Strike.RoundIndex);Round->SetNumberField(TEXT("actualImpactAt"),Strike.ImpactAt);Rounds.Add(MakeShared<FJsonValueObject>(Round));
        }
        Row->SetNumberField(TEXT("fireMissionId"),OrdinaryFireWindowMissionV87);Row->SetArrayField(TEXT("actualNewRounds"),Rounds);
        Check(VauxTacticsV87()&&!NeedsPublicHoldV87?TEXT("vaux_safe_window_tracks_exact_new_public_fire_mission"):TEXT("ordinary_hold_window_tracks_exact_new_public_fire_mission"),OneMission&&!Rounds.IsEmpty()&&OrdinaryFireWindowRoundsV87.Num()==Rounds.Num());
        if(!FirstFailure.IsEmpty())Finish();
        return true;
    }
    TSharedPtr<FJsonObject> HoldDepartureSnapshotV87()const
    {
        auto E=MakeShared<FJsonObject>();const auto* B=Battle();E->SetNumberField(TEXT("hostElapsed"),B?B->Elapsed:0);
        TArray<TSharedPtr<FJsonValue>> Feet;
        if(const auto* G=B?B->FindSquad(Lead):nullptr)for(const auto& W:G->Members)if(const auto* U=W.Get())
        {auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("member"),U->MemberIndex);R->SetBoolField(TEXT("alive"),U->IsAlive());
            R->SetStringField(TEXT("actor"),U->GetPathName());R->SetStringField(TEXT("feet"),U->Feet().ToString());
            R->SetBoolField(TEXT("moving"),U->bMoving);R->SetNumberField(TEXT("speed"),U->GetMotionSpeed());
            R->SetObjectField(TEXT("movement"),WarfareFRRUWorldV82::MovementDiagnosis(U,false));Feet.Add(MakeShared<FJsonValueObject>(R));}
        E->SetArrayField(TEXT("members"),Feet);E->SetObjectField(TEXT("actualObjective"),WarfareFRRUWorldV82::ObjectiveDiagnosis(B));return E;
    }
    bool TickHoldDepartureProbeV87()
    {
        auto* B=Battle();const auto* G=B?B->FindSquad(Lead):nullptr;
        if(!G||G->LivingCount()<=0||(HoldDeparturePhaseV87>0&&G->LivingCount()!=HoldDepartureLivingV87))
        {HoldDepartureEvidenceV87->SetObjectField(TEXT("actualLoss"),HoldDepartureSnapshotV87());Sample(true);
            return Fail(TEXT("hold_departure_probe_actual_section_loss"),TEXT("Actual combat loss; no replacement or arrival is granted."));}
        const auto* U=Member(Lead);if(!U)return true;
        if(!B->IsProgressionVolatile())return Fail(TEXT("hold_departure_probe_requires_volatile_progression"),TEXT("No persistent profile may enter this diagnostic."));
        const auto D=WarfareFRRUWorldV82::MovementDiagnosis(U,false);
        if(HoldDeparturePhaseV87==0)
        {
            const FString From=D->GetStringField(TEXT("currentNode")),Next=D->GetStringField(TEXT("nextNode"));
            // This narrow first-leg window has no clearance/enemy-approach exception.
            if(From!=TEXT("home")||Next!=TEXT("assembly")||!D->GetBoolField(TEXT("intermediateTransit"))
                ||!D->GetBoolField(TEXT("transitCheckpointFrozen"))||D->GetBoolField(TEXT("hostileIntermediateReceiverWaitingV87"))
                ||!D->GetBoolField(TEXT("ordinaryOwnsSection"))||int32(D->GetNumberField(TEXT("receivingAdmissionOwner")))!=Lead)return true;
            FVector A,Z;float AR=0,ZR=0;
            if(!WarfareFRRUWorldV82::NodePose(B,From,A,AR)||!WarfareFRRUWorldV82::NodePose(B,Next,Z,ZR))return true;
            const auto* Scene=wfexp::scene::ForMission("fr.vaux");
            if(!Scene||!wfexp::scene::CanTraverseRoute(*Scene,"home__assembly",{A.X,A.Y,A.Z},{Z.X,Z.Y,Z.Z}))return true;
            for(const auto& W:G->Members)if(const auto* V=W.Get();V&&V->IsAlive())
                if(!V->bFieldEntered||FVector::Dist2D(V->Feet(),A)<=AR+56.5f||FVector::Dist2D(V->Feet(),Z)<=ZR+56.5f)return true;
            HoldDepartureSourceV87=From;HoldDepartureReceiverV87=Next;HoldDepartureTargetV87=D->GetStringField(TEXT("targetNode"));
            HoldDepartureEdgeV87=int32(D->GetNumberField(TEXT("edgeIndex")));HoldDepartureOffsetV87=D->GetStringField(TEXT("transitCheckpointOffset"));
            HoldDepartureLivingV87=G->LivingCount();OrdinaryHoldFeetV827.Reset();
            for(const auto& W:G->Members)if(const auto* V=W.Get();V&&V->IsAlive())OrdinaryHoldFeetV827.Add(V->MemberIndex,V->Feet());
            HoldDepartureEvidenceV87->SetObjectField(TEXT("beforePublicHold"),HoldDepartureSnapshotV87());
            const bool Held=WarfareFRRUWorldV82::HoldSquad(B,Lead,true);Event(TEXT("public_hold_departed_frozen_owner"),Held);
            if(!Held)return Fail(TEXT("hold_departure_probe_public_hold"),B->LastMessage);
            OrdinaryAdvanceSeenV827=true;OrdinaryHoldAtV827=B->Elapsed;HoldDeparturePhaseV87=1;return true;
        }
        if(HoldDeparturePhaseV87==1)
        {
            bool Kept=G->bHoldPosition&&D->GetStringField(TEXT("currentNode"))==HoldDepartureSourceV87
                &&D->GetStringField(TEXT("nextNode"))==HoldDepartureReceiverV87&&D->GetStringField(TEXT("targetNode"))==HoldDepartureTargetV87
                &&int32(D->GetNumberField(TEXT("edgeIndex")))==HoldDepartureEdgeV87&&D->GetBoolField(TEXT("transitCheckpointFrozen"))
                &&D->GetStringField(TEXT("transitCheckpointOffset"))==HoldDepartureOffsetV87
                &&int32(D->GetNumberField(TEXT("receivingAdmissionOwner")))==Lead;
            for(const auto& W:G->Members)if(const auto* V=W.Get();V&&V->IsAlive())
            {const auto* F=OrdinaryHoldFeetV827.Find(V->MemberIndex);Kept&=F&&FVector::Dist2D(*F,V->Feet())<4.f&&!V->bMoving&&V->GetMotionSpeed()<=.5f;}
            if(!Kept){HoldDepartureEvidenceV87->SetObjectField(TEXT("actualHoldFailure"),HoldDepartureSnapshotV87());
                WarfareFRRUWorldV82::HoldSquad(B,Lead,false);Sample(true);return Fail(TEXT("hold_departure_probe_owner_corridor_or_feet_changed"),TEXT("Original owner, frozen leg and every live foot must remain stable under public Hold."));}
            if(B->Elapsed-OrdinaryHoldAtV827<2.f)return true;
            HoldDepartureEvidenceV87->SetObjectField(TEXT("afterTwoHostSecondHold"),HoldDepartureSnapshotV87());
            const bool Released=WarfareFRRUWorldV82::HoldSquad(B,Lead,false);Event(TEXT("public_release_departed_frozen_owner"),Released);
            const auto R=WarfareFRRUWorldV82::MovementDiagnosis(U,false);
            OrdinaryHoldPreservedV827=Released&&int32(R->GetNumberField(TEXT("receivingAdmissionOwner")))==Lead
                &&R->GetBoolField(TEXT("transitCheckpointFrozen"))&&R->GetStringField(TEXT("transitCheckpointOffset"))==HoldDepartureOffsetV87;
            HoldDepartureEvidenceV87->SetObjectField(TEXT("afterPublicRelease"),HoldDepartureSnapshotV87());
            Check(TEXT("departed_frozen_owner_public_hold_release_preserves_service"),OrdinaryHoldPreservedV827);
            if(!OrdinaryHoldPreservedV827){Sample(true);Finish();return !Finished;}
            HoldDepartureReleasedAtV87=B->Elapsed;HoldDeparturePhaseV87=2;return true;
        }
        if(!HoldDepartureCheckpointV87&&(D->GetStringField(TEXT("currentNode"))==HoldDepartureReceiverV87
            ||int32(D->GetNumberField(TEXT("edgeIndex")))>HoldDepartureEdgeV87)&&D->GetStringField(TEXT("targetNode"))==HoldDepartureTargetV87)
        {HoldDepartureCheckpointV87=true;HoldDepartureEvidenceV87->SetObjectField(TEXT("originalCheckpointCompleted"),HoldDepartureSnapshotV87());}
        const auto* Mission=wfexp::tactical::FindMission("fr.vaux");
        const auto Objective=WarfareFRRUWorldV82::ObjectiveDiagnosis(B);
        for(const auto& Q:Objective->GetArrayField(TEXT("actualNativeSections")))
        {const auto R=Q->AsObject();if(int32(R->GetNumberField(TEXT("id")))!=Lead)continue;
            FString Edge;double AtHost=0;if(!R->TryGetStringField(TEXT("arrivalEdge"),Edge)||!R->TryGetNumberField(TEXT("arrivedAt"),AtHost)||AtHost<HoldDepartureReleasedAtV87)continue;
            if(Mission&&D->GetStringField(TEXT("currentNode"))==HoldDepartureTargetV87&&At(Lead,HoldDepartureTargetV87,false))for(const auto& L:Mission->edges)if(wfexpue::Wide(L.id)==Edge&&(wfexpue::Wide(L.to)==HoldDepartureTargetV87
                ||L.bidirectional&&wfexpue::Wide(L.from)==HoldDepartureTargetV87))HoldDepartureArrivedV87=true;}
        if(HoldDepartureCheckpointV87&&HoldDepartureArrivedV87)
        {HoldDepartureEvidenceV87->SetObjectField(TEXT("actualTargetArrival"),HoldDepartureSnapshotV87());Sample(true);
            Check(TEXT("departed_hold_original_registered_route_and_actual_target_arrival"),true);Finish();return !Finished;}
        return true;
    }
    bool TickOrdinaryV827(double Now)
    {
        const bool WaitingForPublicFireV87=TickOrdinaryFireWindowV87();
        if(OrdinaryReturnStepV87)return TickOrdinaryReturnV87(Now);
        auto* NBattle=Battle();const auto View=WarfareFRRUWorldV82::View(NBattle);
        if(NaturalFastPlayRequestedV87&&!NaturalFastPlayHudClickAttemptedV87
            &&NBattle->Phase==EFrontlinePhase::Fighting&&!View.StoryPending&&!UGameplayStatics::IsGamePaused(NBattle))
        {
            // Select the player's public 1.5x control once; host time and all
            // physical, weapon and native outcome authority remain unchanged.
            auto* PC=Controller.Get();auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
            if(!HUD)return Fail(TEXT("natural_fast_play_actual_hud"),TEXT("The loaded battle HUD is unavailable."));
            NaturalFastPlayHudClickAttemptedV87=true;const float Before=NBattle->BattleSpeed;
            HUD->NotifyHitBoxClick(TEXT("BattleRate2"));
            NaturalFastPlayActualSpeedV87=NBattle->BattleSpeed;NaturalFastPlayClickedAtHostV87=NBattle->Elapsed;
            NaturalFastPlayHudClickAppliedV87=FMath::IsNearlyEqual(NaturalFastPlayActualSpeedV87,1.5f);
            Event(TEXT("natural_public_hud_fast_play_1_5x"),NaturalFastPlayHudClickAppliedV87);
            auto Evidence=Commands.Last()->AsObject();Evidence->SetStringField(TEXT("actualHitBox"),TEXT("BattleRate2"));
            Evidence->SetNumberField(TEXT("battleSpeedBefore"),Before);Evidence->SetNumberField(TEXT("battleSpeedAfter"),NaturalFastPlayActualSpeedV87);
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_NATURAL_FAST_PLAY hitBox=BattleRate2 before=%.3f after=%.3f host=%.3f applied=%d"),
                Before,NaturalFastPlayActualSpeedV87,NaturalFastPlayClickedAtHostV87,NaturalFastPlayHudClickAppliedV87);
            Check(TEXT("natural_fast_play_public_hud_applies_one_and_half_speed"),NaturalFastPlayHudClickAppliedV87);
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
        }
        if(!ObserveOrdinaryTrafficV827())return false;
        if(OrdinaryFirstStageV827.IsEmpty())
        {OrdinaryFirstStageV827=View.StageId;OrdinaryFirstTargetV827=View.TargetNode;}
        if(View.StageId!=OrdinaryFirstStageV827&&!OrdinaryFirstCompletedV827)
        {
            OrdinaryFirstCompletedV827=true;
            const bool Clearance=FString(Scenario->Mission)==TEXT("ru.galicia")||FString(Scenario->Mission)==TEXT("fr.malmaison");
            Check(TEXT("ordinary_first_stage_completed_by_real_public_gameplay"),
                !Clearance||WarfareFRRUWorldV82::PassageCleared(NBattle,OrdinaryFirstTargetV827),View.StageId);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("completedStage"),OrdinaryFirstStageV827);
            Evidence->SetStringField(TEXT("followingStage"),View.StageId);Evidence->SetNumberField(TEXT("hostElapsed"),NBattle->Elapsed);
            Evidence->SetBoolField(TEXT("noManualMoveCommand"),true);Evidence->SetBoolField(TEXT("clearanceActuallyCompleted"),Clearance);
            RequiredEvent(TEXT("ordinary_stage_receipt"),TEXT("Public recruitment and normal story choice; actual host clock, routes and native objective receipt"),Evidence);
        }
        const bool Offensive=FString(Scenario->Mission)==TEXT("fr.malmaison");
        const bool Retreat=FString(Scenario->Mission)==TEXT("ru.riga");
        const bool FullClearance=FString(Scenario->Mission)==TEXT("ru.galicia");
        // The explicit diagnostic observes beyond the opening-stage receipt;
        // actual encounter gates and deadlines remain unchanged.
        const bool Whole=Scenario->WholeOperation||FullClearance||Retreat||FrontlineBlockedFireProbeV87::Enabled(NBattle);
        const auto* Mission=wfexp::tactical::FindMission(wfexpue::Narrow(FString(Scenario->Mission)));
        const wfexp::tactical::StageDefinition* CurrentGate=nullptr;bool HasWithdrawal=false;
        if(Mission)for(const auto& Gate:Mission->stages)
        {HasWithdrawal|=Gate.kind==wfexp::tactical::GateKind::WithdrawToNode;if(wfexpue::Wide(Gate.id)==View.StageId)CurrentGate=&Gate;}
        if(Whole&&CurrentGate&&CurrentGate->kind==wfexp::tactical::GateKind::WithdrawToNode&&OrdinaryWithdrawalStageV87!=View.StageId)
        {
            OrdinaryWithdrawalStageV87=View.StageId;OrdinaryWithdrawalIdsV827.Reset();
            const auto Actual=WarfareFRRUWorldV82::ObjectiveDiagnosis(NBattle);
            for(const auto& Id:Actual->GetArrayField(TEXT("frozenWithdrawalParticipantsV87")))OrdinaryWithdrawalIdsV827.Add(int32(Id->AsNumber()));
            Event(TEXT("observe_actual_withdrawal_activation_participants"),!OrdinaryWithdrawalIdsV827.IsEmpty());
        }
        if(Whole&&CurrentGate&&CurrentGate->kind==wfexp::tactical::GateKind::WithdrawToNode&&!OrdinaryWithdrawalIdsV827.IsEmpty())
            for(const int32 Id:OrdinaryWithdrawalIdsV827)if(const auto* Group=NBattle->FindSquad(Id);Group&&Group->LivingCount()>=CurrentGate->minimumLiving&&At(Id,View.TargetNode))
                OrdinaryWithdrawalArrivedV827=true;
        const bool ScopeComplete=OrdinaryFirstCompletedV827&&OrdinaryAdvanceSeenV827&&OrdinaryHoldPreservedV827
            &&(Scenario->WholeOperation||!Offensive||View.StageId.EndsWith(TEXT(".stage.3")))&&(!Whole||View.Finished&&View.Success)
            &&(!Whole||!HasWithdrawal||OrdinaryWithdrawalArrivedV827);
        if(ScopeComplete&&!HoldDepartureProbeV87)
        {
            if(Whole&&!WarfareFRRUWorldV82::OutcomePersisted(NBattle))return true;
            Check(TEXT("ordinary_advance_keeps_explicit_hold_and_resumes_after_release"),true);
            if(Whole)Check(TEXT("whole_operation_native_success_and_actual_outcome_committed"),true,View.StageId);
            if(OrdinaryGuardSamplesV87>0)Check(TEXT("ordinary_actual_hold_guards_retain_their_post"),true,FString::Printf(TEXT("%d actual hold samples; maximum %d resident sections."),OrdinaryGuardSamplesV87,OrdinaryMaxHoldResidentsV87));
            if(Offensive)Check(TEXT("ordinary_offensive_clearance_then_actual_objective_traverse"),true,View.StageId);
            if(Retreat)Check(TEXT("ordinary_withdrawal_success_keeps_actual_activation_participant"),true);
            if(Whole&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ReturnAfterNaturalWin")))
            {OrdinaryReturnStepV87=1;return TickOrdinaryReturnV87(Now);}
            Finish();return !Finished;
        }
        if(NBattle->Phase!=EFrontlinePhase::Fighting||View.Finished)
        {
            // Retain the actual gate/occupancy/clock evidence in the log too.
            // A failed encounter is not automatically a movement failure, and
            // a later cache cleanup must not leave only its generic end label.
            FString Actual;const auto Writer=TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Actual);
            FJsonSerializer::Serialize(WarfareFRRUWorldV82::ObjectiveDiagnosis(NBattle).ToSharedRef(),Writer);
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ORDINARY_FINAL_OBJECTIVE %s"),*Actual);
            return Fail(TEXT("ordinary_objective_scope_not_completed"),NBattle->EndReason+TEXT(" | ")+View.StageId);
        }
        if(WaitingForPublicFireV87)return true; // The host and opponent continue; this player is waiting for his actual rounds.
        if(Lead==INDEX_NONE)
        {
            if(!NBattle->RecruitBlockReason(EFrontlineRole::Rifle,0).IsEmpty())return true;
            Lead=Recruit(EFrontlineRole::Rifle,0);if(Lead==INDEX_NONE)return Fail(TEXT("ordinary_initial_public_recruit"),NBattle->LastMessage);
            ++OrdinaryRecruitsV827;EntryOrderedAt=NBattle->Elapsed;
            if(const auto* Group=NBattle->FindSquad(Lead))for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive())
            {EntryStartingFeet.Add(Unit->MemberIndex,Unit->Feet());Check(TEXT("ordinary_recruit_starts_at_real_offstage_entry_")+FString::FromInt(Unit->MemberIndex),!Unit->bFieldEntered);}
            return true;
        }
        if(HoldDepartureProbeV87)return TickHoldDepartureProbeV87();
        if(!OrdinaryAdvanceSeenV827)if(const auto* Unit=Member(Lead))
        {
            const auto Diagnosis=WarfareFRRUWorldV82::MovementDiagnosis(Unit);
            FString Target;Diagnosis->TryGetStringField(TEXT("targetNode"),Target);
            if(Unit->bFieldEntered&&!Target.IsEmpty()&&Target!=TEXT("home"))
            {
                OrdinaryAdvanceSeenV827=true;Check(TEXT("ordinary_recruit_leaves_home_without_manual_move"),true,Target);
                Event(TEXT("public_hold_ordinary_section"),WarfareFRRUWorldV82::HoldSquad(NBattle,Lead,true));OrdinaryHoldAtV827=NBattle->Elapsed;
            }
        }
        if(OrdinaryHoldAtV827>=0&&!OrdinaryHoldPreservedV827)
        {
            const auto* Group=NBattle->FindSquad(Lead);
            if(!Group||Group->LivingCount()<=0)return Fail(TEXT("ordinary_hold_same_section_survives"),TEXT("Lead section lost before the rear hold window."));
            if(NBattle->Elapsed-OrdinaryHoldAtV827>=.5f&&OrdinaryHoldFeetV827.IsEmpty())
                for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive())OrdinaryHoldFeetV827.Add(Unit->MemberIndex,Unit->Feet());
            if(NBattle->Elapsed-OrdinaryHoldAtV827<2.f)return true;
            bool Preserved=Group->bHoldPosition&&!OrdinaryHoldFeetV827.IsEmpty();
            for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive())
            {const auto* Feet=OrdinaryHoldFeetV827.Find(Unit->MemberIndex);Preserved&=Feet&&FVector::Dist2D(*Feet,Unit->Feet())<4.f;}
            Check(TEXT("ordinary_auto_route_does_not_override_public_hold"),Preserved);
            const bool Released=WarfareFRRUWorldV82::HoldSquad(NBattle,Lead,false);Event(TEXT("public_release_ordinary_section"),Released);
            Check(TEXT("ordinary_public_hold_release_accepted"),Released);OrdinaryHoldPreservedV827=Preserved&&Released;
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
        }
        // The review has a bounded recruit budget, not a production army cap.
        // It only clicks legal normal recruitment; no route or receipt is issued.
        if(VauxTacticsV87())return TickVauxPublicTacticsV87(Now,View);
        const bool MortCasePolicyV87=FString(Scenario->Mission)==TEXT("fr.mort_homme");
        const bool GaliciaCasePolicyV87=FString(Scenario->Mission)==TEXT("ru.galicia");
        const bool CaseRallyPolicyV87=MortCasePolicyV87||GaliciaCasePolicyV87||FString(Scenario->Name)==TEXT("vaux-full");
        const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Pioneer,EFrontlineRole::Lewis,
            EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Pioneer,EFrontlineRole::Rifle,EFrontlineRole::Lewis,
            EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Rifle,EFrontlineRole::Pioneer};
        if((PublicCompletionPolicyV87()||OrdinaryRecruitsV827<24)&&Now>=OrdinaryRecruitRetryV827)
        {
            auto Role=Roles[OrdinaryRecruitsV827%UE_ARRAY_COUNT(Roles)];
            // Case tactics use the same public selection, readiness and shared
            // recruitment queue. Mort has no clearance task; Galicia retains
            // its first real pioneer, then reinforces with guns and riflemen.
            if(MortCasePolicyV87)Role=OrdinaryRecruitsV827%4==1?EFrontlineRole::Lewis:EFrontlineRole::Rifle;
            else if(GaliciaCasePolicyV87)Role=OrdinaryRecruitsV827==1?EFrontlineRole::Pioneer:
                (OrdinaryRecruitsV827-2)%4==0?EFrontlineRole::Lewis:EFrontlineRole::Rifle;
            else if(View.RequiredRole!=int32(EFrontlineRole::Pioneer)&&OrdinaryRecruitsV827%UE_ARRAY_COUNT(Roles)==1)Role=EFrontlineRole::Lewis;
            else if(View.RequiredRole!=int32(EFrontlineRole::Pioneer)&&OrdinaryRecruitsV827%UE_ARRAY_COUNT(Roles)==2)Role=EFrontlineRole::Pioneer;
            OrdinaryRecruitRetryV827=Now+1.;
            const FString PublicRecruitBlockV87=NBattle->RecruitBlockReason(Role,0);
            ObserveNormalCompletionReadinessV87(FString::Printf(TEXT("recruit.%d"),int32(Role)),PublicRecruitBlockV87);
            const FString SchedulingDeferralV87=PublicCompletionPolicyV87()?NormalCompletionRecruitDeferralV87(NBattle,View):FString();
            if(PublicCompletionPolicyV87()&&PublicRecruitBlockV87.IsEmpty()&&!SchedulingDeferralV87.IsEmpty())
            {++NormalCompletionRecruitDeferralsV87;++NormalCompletionDeferralReasonsV87.FindOrAdd(SchedulingDeferralV87);}
            if(PublicRecruitBlockV87.IsEmpty()&&SchedulingDeferralV87.IsEmpty())
            {const int32 Id=Recruit(Role,0);if(Id!=INDEX_NONE)++OrdinaryRecruitsV827;}
        }
        if(Now>=OrdinarySupportRetryV827)
        {
            OrdinarySupportRetryV827=Now+2.;FVector Objective;float Radius=0;
            if(PublicCompletionPolicyV87())
            {ObserveNormalCompletionReadinessV87(TEXT("support.artillery"),NBattle->SupportBlockReason(EFrontlineSupport::Artillery,0));
                ObserveNormalCompletionReadinessV87(TEXT("support.rally"),NBattle->SupportBlockReason(EFrontlineSupport::Rally,0));}
            if(WarfareFRRUWorldV82::NodePose(NBattle,View.TargetNode,Objective,Radius))
            {
                const AWarfareFrontlineSoldier* HostilePoint=nullptr;const AWarfareFrontlineSoldier* FriendlyPoint=nullptr;
                double EnemyScore=MAX_dbl,HoldEnemyScoreV87=MAX_dbl;const AWarfareFrontlineSoldier* HoldEnemyV87=nullptr;
                float Suppression=-1,CaseRallyPriorityV87=-MAX_flt;FString RallyWindowV87;
                const bool KnownPositionPolicyV87=KnownPositionFirePolicyV87();
                for(const AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit&&(!KnownPositionPolicyV87||Unit->Team==0)&&Unit->IsActiveCombatant())
                {
                    if(Unit->Team==1&&FVector::Dist2D(Unit->Feet(),Objective)<4500.)
                    {const double Score=FVector::DistSquared2D(Unit->Feet(),Objective);
                        if(Score<EnemyScore&&OrdinaryArtilleryRouteSafeV87(NBattle,Unit->Feet())){HostilePoint=Unit;EnemyScore=Score;}
                        else if(FString(Scenario->Name)==TEXT("vaux-full")&&!OrdinaryFireWindowUsedV87&&Score<HoldEnemyScoreV87
                            &&NBattle->SupportBlockReason(EFrontlineSupport::Artillery,0).IsEmpty()&&OrdinaryObservedEnemyV87(NBattle,Unit))
                        {HoldEnemyV87=Unit;HoldEnemyScoreV87=Score;}}
                    if(Unit->Team==0&&FVector::Dist2D(Unit->Feet(),Objective)<4500.)
                    {
                        if(CaseRallyPolicyV87)
                        {
                            const auto* RallyGroupV87=NBattle->FindSquad(Unit->SquadId);
                            const float ForwardDistanceV87=float(FVector::Dist2D(Unit->Feet(),Objective));
                            const bool ActualContactWindowV87=Unit->bEngaging;
                            const bool ActualAdvanceWindowV87=RallyGroupV87&&RallyGroupV87->bAdvancing&&!RallyGroupV87->bHoldPosition
                                &&Unit->bFieldEntered&&Unit->bMoving&&Unit->GetMotionSpeed()>.5f&&ForwardDistanceV87<=2400.f;
                            const float PriorityV87=(ActualContactWindowV87?100000.f:0.f)+Unit->Suppression*1000.f-ForwardDistanceV87;
                            if(!Unit->IsRallied()&&(ActualContactWindowV87||ActualAdvanceWindowV87)&&PriorityV87>CaseRallyPriorityV87)
                            {FriendlyPoint=Unit;Suppression=Unit->Suppression;CaseRallyPriorityV87=PriorityV87;
                                RallyWindowV87=ActualContactWindowV87?TEXT("actual_live_contact"):TEXT("actual_forward_motion_within_2400cm");}
                        }
                        else if(Unit->Suppression>Suppression){FriendlyPoint=Unit;Suppression=Unit->Suppression;}
                    }
                }
                if(KnownPositionPolicyV87&&NBattle->SupportBlockReason(EFrontlineSupport::Artillery,0).IsEmpty())
                {
                    // Nodes/owners are already exposed by the public tactical
                    // map. Do not acquire, track or score hidden enemy actors.
                    FString KnownNodeV87;FVector KnownPointV87=FVector::ZeroVector;double KnownScoreV87=MAX_dbl;
                    for(const auto& PostV87:View.Nodes)if(PostV87.Owner==1&&!PostV87.Blocked)
                    {
                        FVector PostPointV87;float PostRadiusV87=0;
                        if(!WarfareFRRUWorldV82::NodePose(NBattle,PostV87.Id,PostPointV87,PostRadiusV87))continue;
                        const double ScoreV87=FVector::DistSquared2D(PostPointV87,Objective);
                        if(ScoreV87<FMath::Square(4500.)&&ScoreV87<KnownScoreV87)
                        {KnownNodeV87=PostV87.Id;KnownPointV87=PostPointV87;KnownScoreV87=ScoreV87;}
                    }
                    if(KnownNodeV87.IsEmpty())++KnownPositionNoEnemyPostV87;
                    else if(OrdinaryArtilleryRouteSafeV87(NBattle,KnownPointV87))
                    {
                        ++KnownPositionFireAttemptsV87;
                        const bool AcceptedV87=NBattle->RequestSupport(EFrontlineSupport::Artillery,KnownPointV87,0);
                        Event(TEXT("ordinary_public_artillery_preparation_known_enemy_post"),AcceptedV87);
                        auto KnownEventV87=Commands.Last()->AsObject();KnownEventV87->SetStringField(TEXT("publicKnownNode"),KnownNodeV87);
                        KnownEventV87->SetStringField(TEXT("fixedRequestedPoint"),KnownPointV87.ToString());KnownEventV87->SetNumberField(TEXT("publicNodeOwner"),1);
                        KnownEventV87->SetBoolField(TEXT("currentEnemyActorOrSightRequired"),false);KnownEventV87->SetBoolField(TEXT("futureFriendlyFireRiskRemains"),true);
                        if(AcceptedV87){++OrdinaryArtilleryV827;++KnownPositionFireAcceptedV87;}
                    }
                }
                if(!KnownPositionPolicyV87&&!HostilePoint&&HoldEnemyV87&&(PublicCompletionPolicyV87()||OrdinaryArtilleryV827<6)&&TryOrdinaryFireWindowV87(NBattle,HoldEnemyV87))return !Finished;
                if(!KnownPositionPolicyV87&&HostilePoint&&(PublicCompletionPolicyV87()||OrdinaryArtilleryV827<6)&&NBattle->SupportBlockReason(EFrontlineSupport::Artillery,0).IsEmpty())
                {const bool Accepted=NBattle->RequestSupport(EFrontlineSupport::Artillery,HostilePoint->Feet(),0);Event(TEXT("ordinary_public_artillery_against_onfield_enemy"),Accepted);if(Accepted)++OrdinaryArtilleryV827;}
                if(FriendlyPoint&&(PublicCompletionPolicyV87()||OrdinaryRallyV827<12)&&(CaseRallyPolicyV87?!RallyWindowV87.IsEmpty():Suppression>5.f)
                    &&NBattle->SupportBlockReason(EFrontlineSupport::Rally,0).IsEmpty())
                {
                    const bool Accepted=NBattle->RequestSupport(EFrontlineSupport::Rally,FriendlyPoint->Feet(),0);
                    Event(TEXT("ordinary_public_rally_own_forward_section"),Accepted);
                    auto RallyEventV87=Commands.Last()->AsObject();RallyEventV87->SetStringField(TEXT("casePolicyV87"),OrdinaryCasePolicyNameV87());
                    RallyEventV87->SetStringField(TEXT("actualTacticalWindowV87"),CaseRallyPolicyV87?RallyWindowV87:TEXT("legacy_suppression_above_5"));
                    RallyEventV87->SetNumberField(TEXT("actualSquad"),FriendlyPoint->SquadId);
                    RallyEventV87->SetNumberField(TEXT("actualMember"),FriendlyPoint->MemberIndex);
                    RallyEventV87->SetNumberField(TEXT("suppressionBeforeRequest"),Suppression);
                    if(Accepted)++OrdinaryRallyV827;
                }
            }
        }
        return true;
    }
    FString StoryChoiceV87(const FFRRUWorldViewV82& Story)const
    {
        FString Requested;
        if((Scenario->Probe==EProbe::Ordinary||Scenario->Probe==EProbe::FrenchPlan)&&FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalChoice="),Requested))
            return Story.ChoiceIds.Contains(Requested)?Requested:FString();
        return Story.ChoiceIds.IsEmpty()?FString():Story.ChoiceIds[0];
    }
    void WriteReport()
    {
        if(Finished)return;RemoveReceivingBayFixtures();Finished=true;
        if(FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")))FRRUVehicleNavCandidateV87::Clear(Battle());
        if(auto* NBattle=Battle())if(UGameplayStatics::IsGamePaused(NBattle))NBattle->SetPaused(false);
        ReleaseOrdinaryFireWindowV87(TEXT("final_report_cleanup"));
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("revision"),ReportRevision);Report->SetStringField(TEXT("scenario"),Scenario?Scenario->Name:TEXT("unbound"));
        Report->SetStringField(TEXT("firstFailure"),FirstFailure);Report->SetBoolField(TEXT("allPassed"),FirstFailure.IsEmpty());
        Report->SetStringField(TEXT("requestedDifficulty"),RequestedDifficultyLabelV87);Report->SetNumberField(TEXT("requestedDifficultyValue"),int32(RequestedDifficultyV87));
        Report->SetStringField(TEXT("failureClassV87"),FailureClassV87);
        Report->SetBoolField(TEXT("progressionFixtureSynthetic"),FixtureSynthetic);Report->SetBoolField(TEXT("simulationClockArtificiallyAdvanced"),false);
        Report->SetBoolField(TEXT("actorsTeleported"),false);Report->SetBoolField(TEXT("artAccepted"),false);Report->SetBoolField(TEXT("shippingAccepted"),false);
        Report->SetBoolField(TEXT("visualCaptureOnly"),FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly")));
        if(FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate"))||FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")))
        {
            Report->SetBoolField(TEXT("editorNavigationQueryOnlyV87"),!FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")));
            Report->SetBoolField(TEXT("editorNavigationGuidedTravelCandidateV87"),FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")));
            Report->SetStringField(TEXT("navigationProofPathV87"),FTNavigationProofPathV87);
            Report->SetBoolField(TEXT("ftPhysicalTraversalAccepted"),false);
        }
        Report->SetBoolField(TEXT("combatAccepted"),false);
        Report->SetBoolField(TEXT("holdDepartureProbeV87"),HoldDepartureProbeV87);
        if(HoldDepartureProbeV87)
        {Report->SetObjectField(TEXT("holdDepartureEvidenceV87"),HoldDepartureEvidenceV87);
            Report->SetStringField(TEXT("acceptanceScopeV87"),TEXT("Public Hold/release of an actually departed original frozen admission owner, original physical checkpoint and actual target arrival only"));
            Report->SetBoolField(TEXT("wholeOperationAccepted"),false);Report->SetBoolField(TEXT("campaignAccepted"),false);}
        Report->SetBoolField(TEXT("blockedFireProbeRequestedV87"),FrontlineBlockedFireProbeV87::Requested());
        Report->SetBoolField(TEXT("blockedFireProbeIsolationAcceptedV87"),FrontlineBlockedFireProbeV87::Isolation());
        Report->SetBoolField(TEXT("blockedFireProbeReproducedV87"),BlockedFireEvidenceV87.IsValid());
        if(BlockedFireEvidenceV87)Report->SetObjectField(TEXT("blockedFireProbeEvidenceV87"),BlockedFireEvidenceV87);
        Report->SetBoolField(TEXT("ordinaryAdvanceReview"),Scenario&&Scenario->Probe==EProbe::Ordinary);
        Report->SetBoolField(TEXT("naturalFastPlayRequestedV87"),NaturalFastPlayRequestedV87);
        Report->SetBoolField(TEXT("naturalFastPlayHudClickAttemptedV87"),NaturalFastPlayHudClickAttemptedV87);
        Report->SetBoolField(TEXT("naturalFastPlayHudClickAppliedV87"),NaturalFastPlayHudClickAppliedV87);
        Report->SetNumberField(TEXT("naturalFastPlayActualSpeedV87"),NaturalFastPlayActualSpeedV87);
        Report->SetNumberField(TEXT("naturalFastPlayClickedAtHostV87"),NaturalFastPlayClickedAtHostV87);
        if(Scenario&&Scenario->Probe==EProbe::FrenchPlan)Report->SetObjectField(TEXT("frenchPlanReviewV87"),FrenchPlanReportV87());
        if(Scenario&&Scenario->Probe==EProbe::Ordinary)
        {
            const bool Whole=Scenario->WholeOperation||FString(Scenario->Mission)==TEXT("ru.galicia")||FString(Scenario->Mission)==TEXT("ru.riga");
            Report->SetStringField(TEXT("expectedObjectiveScopeV87"),FrontlineBlockedFireProbeV87::Isolation()?TEXT("diagnostic_only_observe_until_blocked_fire_or_native_terminal"):Whole?TEXT("whole_operation_native_success"):FString(Scenario->Mission)==TEXT("fr.malmaison")?TEXT("clearance_and_enter_final_stage"):TEXT("opening_stage_completion"));
            Report->SetStringField(TEXT("ordinaryCasePolicyV87"),OrdinaryCasePolicyNameV87());
            Report->SetStringField(TEXT("ordinaryStrategyV87"),VauxTacticsV87()?FString::Printf(TEXT("Explicit Vaux public-tactics review fixture: alternative story, staged Rifle/MG/Mills recruitment (%s), unchanged automatic advance, four normally selected public supports, observed safe fire and actual-contact smoke/rally. Serial public fire windows: safe forecasts request normally without Hold; only at-risk ordinary participants use explicit public Hold/fire/release. No extra Move, actor, clock, RNG or outcome grants. The policy is not proof of campaign completion."),PublicCompletionPolicyV87()?TEXT("public readiness until the original Native/wall deadline; no cumulative review cap"):TEXT("original 24-review budget")):
                FString::Printf(TEXT("Review policy %s; public case recruitment with the unchanged 24-section budget; normal story branch; automatic task/cover movement; safe hostile-contact artillery and public rally during actual contact/advance windows. No scripted position, clock or arrival grants."),OrdinaryCasePolicyNameV87()));
        }
        Report->SetBoolField(TEXT("frenchWholeOperationCompletionPolicyV87"),FrenchWholeOperationCompletionV87());
        Report->SetBoolField(TEXT("reviewCumulativeRecruitSupportCapsAppliedV87"),!PublicCompletionPolicyV87());
        Report->SetStringField(TEXT("reviewCompletionProvenanceV87"),NormalCampaignCompletionV87?TEXT("normal_public_fresh_start"):
            FrenchWholeOperationCompletionV87()?TEXT("fixture_prior_french_whole_operation"):TEXT("original_bounded_natural_or_specialist"));
        if(FrenchWholeOperationCompletionV87()&&!NormalCampaignCompletionV87)
        {
            Report->SetStringField(TEXT("ordinaryStrategyV87"),TEXT("French prior-state whole-operation review fixture: original role strategy, story, automatic tasks and actual movement; public recruitment and support readiness until the unchanged Native result or original wall deadline. No cumulative recruit/Artillery/Rally cap and no total-living-section cap. Mortar/Smoke already had no cumulative count cap. Real home ingress scheduling, public BlockReason/CD, observed targets and fire safety remain authoritative. Existing explicit Vaux public Hold/fire/release stays separately reported. This is not normal fresh-start provenance or whole-campaign acceptance."));
            Report->SetBoolField(TEXT("wholeOperationCompletionCumulativeReviewCapsAppliedV87"),false);
            Report->SetNumberField(TEXT("wholeOperationCompletionRecruitScheduleDeferralsV87"),NormalCompletionRecruitDeferralsV87);
            Report->SetStringField(TEXT("wholeOperationCompletionHomeScheduleV87"),VauxTacticsV87()?TEXT("Existing Vaux review homeRelated<2; total live count diagnostic only"):
                TEXT("Existing actual ingress/home occupancy against native home capacity; total live count diagnostic only"));
            auto PublicSamplesV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionReadinessSamplesV87)PublicSamplesV87->SetNumberField(ItemV87.Key,ItemV87.Value);
            auto LastReasonsV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionLastBlockReasonsV87)LastReasonsV87->SetStringField(ItemV87.Key,ItemV87.Value);
            auto DeferralsV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionDeferralReasonsV87)DeferralsV87->SetNumberField(ItemV87.Key,ItemV87.Value);
            Report->SetObjectField(TEXT("wholeOperationCompletionPublicBlockReasonSamplesV87"),PublicSamplesV87);
            Report->SetObjectField(TEXT("wholeOperationCompletionLastPublicBlockReasonsV87"),LastReasonsV87);
            Report->SetObjectField(TEXT("wholeOperationCompletionScheduleDeferralReasonsV87"),DeferralsV87);
        }
        Report->SetBoolField(TEXT("normalLifecycleCompletionPolicyV87"),NormalCampaignCompletionV87);
        if(NormalCampaignCompletionV87)
        {
            Report->SetStringField(TEXT("ordinaryStrategyV87"),TEXT("Strict normal public fresh-start completion: original case roles, automatic tasks and guards; public recruitment/Artillery/Rally readiness until the unchanged native result or original wall deadline. No cumulative review recruit/support count cap. Only actual home ingress congestion defers ready recruitment. Living-section counts and capacity totals are diagnostics, never a global recruitment limit or production resource. No additional Move/Hold, actor, clock or outcome grants."));
            Report->SetBoolField(TEXT("normalCompletionCumulativeReviewCapsAppliedV87"),false);
            Report->SetNumberField(TEXT("normalCompletionRecruitScheduleDeferralsV87"),NormalCompletionRecruitDeferralsV87);
            Report->SetNumberField(TEXT("normalCompletionLastNativeCapacitySoftLimitV87"),NormalCompletionActiveSoftLimitV87);
            Report->SetNumberField(TEXT("normalCompletionLastLivingSectionsV87"),NormalCompletionLiveSectionsV87);
            Report->SetNumberField(TEXT("normalCompletionLastLivingPeopleV87"),NormalCompletionLivingPeopleV87);
            Report->SetNumberField(TEXT("normalCompletionLastHomeRelatedSectionsV87"),NormalCompletionHomeRelatedV87);
            Report->SetNumberField(TEXT("normalCompletionLastReadinessSampleAtHostV87"),NormalCompletionLastReadinessHostV87);
            auto PublicSamplesV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionReadinessSamplesV87)PublicSamplesV87->SetNumberField(ItemV87.Key,ItemV87.Value);
            auto LastReasonsV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionLastBlockReasonsV87)LastReasonsV87->SetStringField(ItemV87.Key,ItemV87.Value);
            auto DeferralsV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionDeferralReasonsV87)DeferralsV87->SetNumberField(ItemV87.Key,ItemV87.Value);
            auto CapacityV87=MakeShared<FJsonObject>();for(const auto& ItemV87:NormalCompletionCapacityBasisV87)CapacityV87->SetNumberField(ItemV87.Key,ItemV87.Value);
            Report->SetObjectField(TEXT("normalCompletionPublicBlockReasonSamplesV87"),PublicSamplesV87);
            Report->SetObjectField(TEXT("normalCompletionLastPublicBlockReasonsV87"),LastReasonsV87);
            Report->SetObjectField(TEXT("normalCompletionScheduleDeferralReasonsV87"),DeferralsV87);
            Report->SetObjectField(TEXT("normalCompletionNativeCapacityBasisV87"),CapacityV87);
        }
        if(HoldDepartureProbeV87)
        {Report->SetStringField(TEXT("expectedObjectiveScopeV87"),TEXT("departed_frozen_owner_public_hold_release_original_checkpoint_and_actual_target_arrival_only"));
            Report->SetStringField(TEXT("reviewCompletionProvenanceV87"),TEXT("fixture_prior_bounded_hold_departure_probe"));
            Report->SetStringField(TEXT("ordinaryStrategyV87"),TEXT("One normally recruited Rifle section follows its original automatic registered route. Delay only the original public Hold until every live foot clears home and remains outside assembly, with the original frozen owner. Public Hold two host seconds, read each foot/owner/frozen corridor, release, then observe the original physical checkpoint and actual target arrival. No Move, actor, clock, enemy sequence, support or outcome grant. This is not whole-operation acceptance."));}
        Report->SetBoolField(TEXT("knownPositionPublicPreparationFireV87"),KnownPositionFirePolicyV87());
        if(KnownPositionFirePolicyV87())
        {
            Report->SetStringField(TEXT("ordinaryStrategyV87"),TEXT("French public completion review: existing recruitment, tasks and Rally; Artillery may prepare a public enemy-held map post within the existing objective sector without a current enemy actor or sight. Current friendly footprint is still refused; future route forecasts are diagnostic only. Fixed public fire retains real CD, delay, scatter and friendly damage. No extra Hold/Move, actor, clock, Native outcome or damage grants. This is not zero-friendly-fire or complete campaign acceptance."));
            Report->SetNumberField(TEXT("knownPositionPublicFireAttemptsV87"),KnownPositionFireAttemptsV87);Report->SetNumberField(TEXT("knownPositionPublicFireAcceptedV87"),KnownPositionFireAcceptedV87);
            Report->SetNumberField(TEXT("knownPositionFutureRouteRiskDiagnosticCountV87"),KnownPositionFutureRouteRiskV87);
            Report->SetNumberField(TEXT("knownPositionCurrentFriendlyFootprintRefusalsV87"),KnownPositionCurrentFriendlyRefusalsV87);
            Report->SetNumberField(TEXT("knownPositionNoPublicEnemyPostV87"),KnownPositionNoEnemyPostV87);
        }
        Report->SetBoolField(TEXT("vauxPublicTacticsFixtureV87"),VauxTacticsV87());
        if(VauxTacticsV87())
        {
            Report->SetNumberField(TEXT("vauxTacticsActualSupportSelectionV87"),WarfareFRRUCampaignV82::SupportSelection(Battle()));
            Report->SetNumberField(TEXT("vauxTacticsLivingSectionLimitV87"),PublicCompletionPolicyV87()?0:8);
            Report->SetBoolField(TEXT("vauxTacticsLivingSectionLimitAppliedV87"),!PublicCompletionPolicyV87());Report->SetNumberField(TEXT("vauxTacticsHomeRelatedLimitV87"),2);
            Report->SetNumberField(TEXT("vauxTacticsMinimumRecruitHostIntervalV87"),18);Report->SetNumberField(TEXT("vauxTacticsRecruitDeferralsV87"),VauxRecruitDeferralsV87);
            Report->SetNumberField(TEXT("vauxTacticsActualSmokeCallsV87"),VauxSmokeCallsV87);Report->SetNumberField(TEXT("vauxTacticsActualMortarCallsV87"),VauxMortarCallsV87);
            Report->SetNumberField(TEXT("vauxTacticsMortarRouteDeclinesV87"),VauxMortarRouteDeclinesV87);
            Report->SetBoolField(TEXT("vauxTacticsAdditionalMoveOrHoldIssuedV87"),VauxManualHoldIssuedV87);
            Report->SetBoolField(TEXT("vauxTacticsManualHoldIssuedV87"),VauxManualHoldIssuedV87);Report->SetBoolField(TEXT("vauxTacticsAdditionalMoveIssuedV87"),false);
            Report->SetNumberField(TEXT("vauxTacticsLastAttemptedWindowSupportTypeV87"),int32(OrdinaryFireWindowSupportV87));
            Report->SetNumberField(TEXT("vauxTacticsPublicFireWindowCountV87"),OrdinaryArtilleryV827+VauxMortarCallsV87);
            Report->SetNumberField(TEXT("vauxTacticsPublicFireRequestAttemptsV87"),VauxPublicFireRequestAttemptsV87);
            Report->SetNumberField(TEXT("vauxTacticsPublicHoldAttemptsV87"),VauxPublicHoldAttemptsV87);
            Report->SetNumberField(TEXT("vauxTacticsDirectSafeFireWindowsV87"),VauxDirectSafeFireWindowsV87);
            Report->SetNumberField(TEXT("vauxTacticsHeldFireWindowsV87"),VauxHeldFireWindowsV87);
            auto Refusals=MakeShared<FJsonObject>();for(const auto& Pair:VauxFireRefusalsV87)Refusals->SetNumberField(Pair.Key,Pair.Value);
            Report->SetObjectField(TEXT("vauxTacticsFireWindowRefusalCountsV87"),Refusals);Report->SetBoolField(TEXT("vauxTacticsProductionTuningChangedV87"),false);
            Report->SetStringField(TEXT("vauxTacticsFireSafetyScopeV87"),TEXT("Actual observed enemies only; original friendly scatter/blast gate, current registered-leg prefixes bounded by original legal maximum gait and LastImpact, plus time-reachable next/final posts. Safe forecasts use a public request without Hold; only at-risk ordinary squads are actually held through exact public rounds and released. Existing own pending fire refuses another request. Read-only abstention, not a guarantee against future unobserved orders. Own actual pending rounds defer another fixed fire mission."));
        }
        Report->SetBoolField(TEXT("ordinaryReviewHasNoManualMoveCommand"),Scenario&&Scenario->Probe==EProbe::Ordinary);
        Report->SetBoolField(TEXT("ordinaryFirstStageActuallyCompleted"),OrdinaryFirstCompletedV827);
        Report->SetBoolField(TEXT("ordinaryPublicHoldPreserved"),OrdinaryHoldPreservedV827);
        Report->SetNumberField(TEXT("ordinaryPublicRecruits"),OrdinaryRecruitsV827);
        Report->SetNumberField(TEXT("ordinaryPublicArtilleryCalls"),OrdinaryArtilleryV827);Report->SetNumberField(TEXT("ordinaryPublicRallyCalls"),OrdinaryRallyV827);
        Report->SetNumberField(TEXT("ordinaryReviewArtilleryRouteDeclinesV87"),OrdinaryArtilleryRouteDeclinesV87);
        Report->SetBoolField(TEXT("ordinaryOnePublicArtilleryHoldWindowUsedV87"),OrdinaryFireWindowUsedV87);
        Report->SetNumberField(TEXT("ordinaryArtilleryHoldWindowMissionV87"),OrdinaryFireWindowMissionV87);
        Report->SetNumberField(TEXT("ordinaryArtilleryWindowUnreleasedLiveHoldsV87"),OrdinaryFireWindowHoldsV87.Num());
        TArray<TSharedPtr<FJsonValue>> TerminalHoldsV87;for(const int32 Id:OrdinaryFireWindowTerminalHoldsV87)TerminalHoldsV87.Add(MakeShared<FJsonValueNumber>(Id));
        Report->SetArrayField(TEXT("ordinaryArtilleryWindowTerminalPublicReleaseUnavailableIdsV87"),TerminalHoldsV87);
        Report->SetStringField(TEXT("ordinaryReviewArtilleryForecastScopeV87"),KnownPositionFirePolicyV87()
            ?TEXT("French public known-post preparation: current friendly footprint remains a cautious policy gate; future next/final route overlap is diagnostic, never a veto. Fixed impacts can cause future friendly fire. Public support CD/delay/scatter/damage unchanged.")
            :TEXT("Review-only conservative abstention: actual known friendly next/final route posts, real first-impact delay and the fixed four-group window. Production friendly fire and fixed shell targets are unchanged."));
        Report->SetNumberField(TEXT("actualHoldGuardSamplesV87"),OrdinaryGuardSamplesV87);Report->SetNumberField(TEXT("actualMaximumHoldResidentSectionsV87"),OrdinaryMaxHoldResidentsV87);
        if(Scenario&&Scenario->Probe==EProbe::Ordinary)if(auto* NBattle=Battle())
            Report->SetObjectField(TEXT("finalActualObjectiveEligibility"),WarfareFRRUWorldV82::ObjectiveDiagnosis(NBattle));
        Report->SetBoolField(TEXT("ftContactOnly"),FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82FTContactOnly")));
        Report->SetBoolField(TEXT("germanHostArmourReview"),FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview")));
        Report->SetBoolField(TEXT("germanContactCoreOnly"),GermanCoreOnly());
        Report->SetBoolField(TEXT("formationJoinOnly"),FormationJoinOnlyV826());
        Report->SetBoolField(TEXT("actualFTPhaseWrapObserved"),FTPhaseWrapObserved);
        Report->SetBoolField(TEXT("historicalPlayerSaveAccepted"),false);Report->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-Started);
        Report->SetNumberField(TEXT("configuredWallDeadlineSeconds"),DeadlineSeconds);
        if(WallDeadlineUtc.GetTicks()>0)Report->SetStringField(TEXT("configuredWallDeadlineUtc"),WallDeadlineUtc.ToIso8601());
        Report->SetArrayField(TEXT("requiredWindows"),RequiredWindows);
        Report->SetArrayField(TEXT("missingRequiredWindows"),MissingRareWindows());
        if(Scenario&&Scenario->Probe==EProbe::Train)
        {
            auto Serials=MakeShared<FJsonObject>();Serials->SetNumberField(TEXT("naturalSixPassengerCarrier"),FirstNaturalTrainSerial);
            Serials->SetNumberField(TEXT("controlledWreckCarrier"),RetryTrainSerial);Serials->SetNumberField(TEXT("separateNaturalCombatCarrier"),TrainCombatSerial);
            Serials->SetBoolField(TEXT("combatRecruitAttemptedExactlyOnce"),TrainCombatRecruitAttempted);
            Serials->SetNumberField(TEXT("publicContactRecruits"),TrainContactRecruits);Serials->SetNumberField(TEXT("contactRecruitLimit"),TrainContactRecruitLimit);
            Serials->SetBoolField(TEXT("contactRecruitBudgetExhausted"),TrainContactExhausted);
            Report->SetObjectField(TEXT("trainProofCarrierSerials"),Serials);
        }
        Report->SetBoolField(TEXT("controlledCollisionFixtureUsed"),RetryTrainSerial!=INDEX_NONE);
        Report->SetBoolField(TEXT("controlledVehiclePoseAndModuleFixtureUsed"),VehiclePoseChecked);
        Report->SetBoolField(TEXT("controlledSchiltPublicDamageFixtureUsed"),RuptureHitIssued);
        Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("samples"),Samples);Report->SetArrayField(TEXT("commands"),Commands);Report->SetArrayField(TEXT("captures"),Captures);
        Report->SetStringField(TEXT("pendingCapture"),PendingCapture);Report->SetBoolField(TEXT("cameraReframedForCapture"),CompletedCaptures.Contains(TEXT("06-battle-moved-zoomed")));
        Report->SetBoolField(TEXT("finalCameraReframedWithoutActorTeleport"),CompletedCaptures.Contains(TEXT("07-final-battle-state")));
        Report->SetBoolField(TEXT("actualStraddlerWindowSeen"),RealStraddler);Report->SetBoolField(TEXT("actualHostileHullWindowSeen"),RealHullContest);
        if(Scenario&&Scenario->Probe==EProbe::Schilt)
        {
            Report->SetNumberField(TEXT("actualSchiltPioneerSquadV87"),SchiltPioneerV87);
            Report->SetBoolField(TEXT("actualFlameBraceSeen"),FlameBrace);Report->SetBoolField(TEXT("actualFlameJetSeen"),FlameJet);
            Report->SetBoolField(TEXT("actualFlameRecoverySeen"),FlameRecovery);Report->SetBoolField(TEXT("actualFlameEnemyBurnSeen"),FlameEnemyBurn);
            Report->SetBoolField(TEXT("actualFlameIgnitionSeen"),FlameIgnited);Report->SetBoolField(TEXT("actualFlameFuelRuptureSeen"),FlameSpent);
            Report->SetBoolField(TEXT("actualFlameEmptySeen"),FlameEmpty);Report->SetBoolField(TEXT("actualFlamePauseChecked"),FlamePauseDone);
        }
        if(Scenario&&Scenario->Probe==EProbe::VB)
        {
            Report->SetBoolField(TEXT("actualVBInitialRoundVisible"),VBLoaded);Report->SetBoolField(TEXT("actualVBRoundAbsentAfterShot"),VBEmpty);
            Report->SetBoolField(TEXT("actualVBSixBonePairSeekObserved"),VBPairFinite);Report->SetNumberField(TEXT("actualVBReloadSamples"),VBReloadSamples);
            Report->SetBoolField(TEXT("actualVBProjectileTangentObserved"),VBProjectile);Report->SetBoolField(TEXT("actualVBLoadedRoundRestored"),VBRestored);
            Report->SetBoolField(TEXT("vbControlledExistingHostileRouteIssuedV87"),false);
            Report->SetNumberField(TEXT("vbControlledExistingHostileSquadV87"),INDEX_NONE);
            Report->SetNumberField(TEXT("vbNaturalPioneerSquadV87"),VBPioneerV87);
            Report->SetBoolField(TEXT("vbNaturalPioneerPubliclyRecruitedV87"),VBPioneerRecruitedV87);
            Report->SetBoolField(TEXT("vbNaturalPioneerReturnedToAssemblyV87"),VBPioneerReturnedV87);
            Report->SetBoolField(TEXT("vbNaturalVisibleInRangeWindowSeenV87"),VBContactSeenV87);
            Report->SetBoolField(TEXT("vbNaturalContactInconclusiveV87"),!FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))&&!VBContactSeenV87&&!FirstFailure.IsEmpty());
            Report->SetNumberField(TEXT("vbNaturalNoShotWindowStartedAtV87"),VBNoShotSinceV87);
            if(VBNoShotEvidenceV87.IsValid())Report->SetObjectField(TEXT("vbNaturalNoShotEvidenceV87"),VBNoShotEvidenceV87);
            Report->SetStringField(TEXT("vbContactScopeV87"),FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))?TEXT("Separate controlled VB mechanical fixture; see vbMechanicalFixtureV87 and explicit placement/clock scope flags."):TEXT("Normal public French pioneer recruitment, real wire clearance, actual return to assembly, then public VB movement toward objective. No enemy recruitment or orders, actor placement, health, weapon or clock grants. Bounded at 180 host seconds or the earlier existing deadline; an unobserved contact is inconclusive, never passed. Not whole-operation or enemy-AI balance acceptance."));
            Report->SetBoolField(TEXT("actualVBLaunchHumanMuzzleContactAccepted"),false);Report->SetBoolField(TEXT("actualVBReloadHandContactsAccepted"),false);
        }
        if(ReportDetail)ReportDetail(*Report);
        const FString Path=FPaths::Combine(User,TEXT("FRRUReview"),ReportName);IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
        FString Text;const bool Encoded=FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
        const bool Written=Encoded&&FFileHelper::SaveStringToFile(Text,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_NATURAL_FINISHED scenario=%s failure=%s receipt=%s"),Scenario?Scenario->Name:TEXT("unbound"),*FirstFailure,*Path);
        FPlatformMisc::RequestExitWithStatus(false,Written&&FirstFailure.IsEmpty()?0:3);
    }
    bool Tick(float DeltaTime)
    {
        WallFrameMs=double(DeltaTime)*1000.;
        auto* NController=Controller.Get();auto* NBattle=Battle();if(Finished)return false;
        if(!NController||!NBattle||!NBattle->HasActorBegunPlay())
        {if(Step==EStep::FinalCapture){Check(TEXT("actual_beginplay_controller_remains_valid"),false,TEXT("The context disappeared before the final screenshot."));WriteReport();return false;}return Fail(TEXT("actual_beginplay_controller_remains_valid"),TEXT("The actual controller or begun-play game mode disappeared."));}
        const double Now=FPlatformTime::Seconds();auto& Screen=ScreenState(NController);
        if(Step==EStep::FinalCapture)
        {
            if(Now-StepAt>15){Check(TEXT("final_actual_capture_completed_before_exit"),false,TEXT("The final screenshot did not complete within 15 wall seconds."));WriteReport();return false;}
            if(Now-StepAt<1.5)return true;
            if(!PendingCaptureName.IsEmpty()&&PendingCaptureName!=TEXT("07-final-battle-state")){Capture(PendingCaptureName);return true;}
            if(!Capture(TEXT("07-final-battle-state")))return true;
            Sample();WriteReport();return false;
        }
        if(FTNavigationProofDoneV87&&FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")))
        {
            const auto Follow=FRRUVehicleNavCandidateV87::FollowStatus(NBattle);
            if(Follow.Status==FRRUVehicleNavCandidateV87::EStatus::Failed)
            {Sample(true);return Fail(TEXT("actual_ft_navigation_guided_travel_failure_v87"),Follow.Reason);}
        }
        if(const auto Evidence=FrontlineBlockedFireProbeV87::Failure(NBattle))
        {
            BlockedFireEvidenceV87=Evidence;Sample(true);
            return Fail(TEXT("blocked_contact_no_actual_discharge_v87"),TEXT("An actual advancing MG-crew member retained stopping contact for 10 host seconds without increasing ShotsDischarged. Inspect blockedFireProbeEvidenceV87; this diagnostic threshold does not itself establish a gameplay bug."));
        }
        if(HoldDepartureProbeV87&&NBattle->Elapsed>=180.f)
        {HoldDepartureEvidenceV87->SetObjectField(TEXT("actualLimitState"),HoldDepartureSnapshotV87());Sample(true);
            return Fail(TEXT("hold_departure_probe_inconclusive"),TEXT("Explicit 180 host-second probe bound elapsed; missing live window or physical completion is not passed."));}
        if(Now-Started>DeadlineSeconds||(WallDeadlineUtc.GetTicks()>0&&FDateTime::UtcNow()>=WallDeadlineUtc))
        {Sample(true);if(Scenario->Probe==EProbe::VB&&!FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))&&!VBContactSeenV87)
            return Fail(TEXT("vb_natural_contact_inconclusive"),TEXT("The existing wall deadline elapsed without a stationary visible in-range VB opportunity. No contact or role window is granted."));
            return Fail(TEXT("bounded_natural_scenario"),TEXT("The configured wall deadline elapsed; retain the actual samples and final capture. Unobserved required windows are not passed."));}
        if(Scenario->Probe==EProbe::VB&&!FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))&&NBattle->Elapsed>=180.f)
        {Sample(true);return Fail(VBContactSeenV87?TEXT("vb_natural_role_cycle_incomplete"):TEXT("vb_natural_contact_inconclusive"),TEXT("The 180 host-second natural VB limit elapsed. Retain the actual recruitment, movement and fire readback; incomplete required windows are not passed."));}
        if(!Screen.PreparationError.IsEmpty())return Fail(TEXT("actual_loading_preparation"),Screen.PreparationError);
        if(!NBattle->AssetFailure.IsEmpty())return Fail(TEXT("actual_asset_binding"),NBattle->AssetFailure);
        if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))&&Step>=EStep::WarmFrame)
            Sample(); // Retain every warmed/moving frame; no outlier filtering.
        if(Step>=EStep::WarmFrame&&!Screen.Preparing&&!Screen.Menu&&!Screen.Help&&!Screen.Settings&&!Screen.Technology&&!Screen.AchievementsV76)
        {
            const auto Story=WarfareFRRUWorldV82::View(NBattle);
            if(Story.StoryPending&&!Story.ChoiceIds.IsEmpty())
            {
                const FString Choice=StoryChoiceV87(Story);
                if(Choice.IsEmpty())return Fail(TEXT("requested_natural_story_choice_exists"),TEXT("The explicit review branch is not one of this story's public choices."));
                const auto Result=WarfareFRRUWorldV82::ReviewStoryChoiceDwell(NBattle,Story.StoryId,Choice);
                if(!Result.IsSet())return true;
                Check(TEXT("actual_story_dwell_freezes_host.")+Story.StoryId,Result.GetValue());
                Event(TEXT("actual_story_choice"),Result.GetValue());
                if(!Result.GetValue())return Fail(TEXT("actual_story_pause_and_choice"),TEXT("Real story pause, invalid choice or normal resume failed; inspect STORY_DWELL_REVIEW."));
            }
        }
        if(Step==EStep::Setup)
        {
            FString Error;
            if(!(Scenario->Probe==EProbe::Reserve?PrepareReserveResearch():WarfareFRRUCampaignInternalV82::PrepareReviewMission(NBattle,Scenario->Mission,Error,RequestedDifficultyV87)))return Fail(TEXT("synthetic_progression_real_repository_fixture"),Error);
            Check(TEXT("synthetic_progression_real_repository_fixture"),true,TEXT("Prior victories are fixture data; this encounter is not advanced synthetically."));
            Check(TEXT("requested_public_difficulty_is_actual_campaign_difficulty"),NBattle->GetCampaignDifficulty()==RequestedDifficultyV87,RequestedDifficultyLabelV87);
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            if(Scenario->Probe==EProbe::Ordinary)OrdinaryOpeningResearchV87=WarfareFRRUCampaignV82::ResearchPoints(NBattle);
            if(Scenario->Probe==EProbe::FrenchPlan&&!PrepareFrenchPlanResearchV87())return Fail(TEXT("chemin_plan_exact_public_research_budget"),WarfareFRRUCampaignV82::Status(NBattle));
            uint32 Roles=(1u<<0)|(1u<<1)|(1u<<3)|(1u<<6);if(Scenario->Probe==EProbe::Ordinary)Roles|=1u<<2;uint32 Supports=Scenario->Probe==EProbe::FT?3u:Scenario->Probe==EProbe::Ordinary?5u:1u;
            const auto* Mission=wfexp::FindMission(wfexpue::Narrow(FString(Scenario->Mission)));if(!Mission)return Fail(TEXT("registered_mission"),TEXT("Stable mission ID not found."));
            if(VauxTacticsV87())Supports=15u;
            Roles&=Mission->roleMask;Supports&=Mission->supportMask;
            if(VauxTacticsV87()&&Supports!=15u)return Fail(TEXT("vaux_tactics_four_supports_are_legally_available"),TEXT("The current Vaux mission must normally allow Artillery, Smoke, Rally and Mortar; unavailable selection is not granted."));
            if(Scenario->Probe==EProbe::Schilt)Roles|=(1u<<int32(EFrontlineRole::Flamethrower))|(1u<<int32(EFrontlineRole::Pioneer));
            if(Scenario->Probe==EProbe::VB)Roles|=1u<<int32(EFrontlineRole::RifleGrenadier);
            if(!WarfareFRRUCampaignV82::SetSelection(NBattle,Roles,Supports))return Fail(TEXT("actual_role_support_selection"),WarfareFRRUCampaignV82::Status(NBattle));
            if(IsVehicleCase()&&Scenario->Probe!=EProbe::Train)
            {
                const TArray<FString> Chosen=Scenario->Probe==EProbe::FT?TArray<FString>{TEXT("fr.ft"),TEXT("fr.schneider")}:Scenario->Probe==EProbe::Austin?TArray<FString>{TEXT("ru.austin_series1")}:TArray<FString>{TEXT("fr.schneider"),TEXT("fr.saint_chamond")};
                VehicleSelected=WarfareFRRUCampaignV82::SetArmourSelection(NBattle,Chosen);if(!VehicleSelected)return Fail(TEXT("two_slot_stable_armour_selection"),WarfareFRRUCampaignV82::Status(NBattle));
            }
            Screen.Dossier=0;Screen.Menu=false;SetStep(EStep::Cover);return true;
        }
        if(Step==EStep::Cover)
        {
            if(Now-StepAt<1.5||!Capture(TEXT("01-main-menu")))return true;
            auto* HUD=Cast<AWarfareFrontlineHUD>(NController->GetHUD());if(!HUD)return Fail(TEXT("actual_hud_exists"),TEXT("Missing actual HUD."));
            HUD->NotifyHitBoxClick(TEXT("ContinueCampaign"));
            Check(TEXT("real_continue_opens_national_warroom"),Screen.Dossier==1917&&WarfareFRRUCampaignV82::MissionId(NBattle)==Scenario->Mission);
            SetStep(EStep::WarRoom);return true;
        }
        if(Step==EStep::WarRoom)
        {
            if(!TheatreCaptured)
            {
                if(Now-StepAt<1.5||Screen.PhotoTransitionV37||!Capture(TEXT("02-warroom-map")))return true;
                TheatreCaptured=true;
                if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly")))
                {
                    auto* HUD=Cast<AWarfareFrontlineHUD>(NController->GetHUD());
                    if(!HUD)return Fail(TEXT("actual_hud_exists"),TEXT("Missing actual HUD."));
                    const auto Before=FRRUWarRoomV82::CaptureNavigation(NController);
                    HUD->NotifyHitBoxClick(TEXT("FRRUMap:operation"));
                    const auto After=FRRUWarRoomV82::CaptureNavigation(NController);
                    NController->MenuBackV61();
                    Check(TEXT("actual_national_map_back_restores_view"),FRRUWarRoomV82::SameNavigation(Before,FRRUWarRoomV82::CaptureNavigation(NController)));
                    NController->MenuForwardV61();
                    Check(TEXT("actual_national_map_forward_restores_view"),FRRUWarRoomV82::SameNavigation(After,FRRUWarRoomV82::CaptureNavigation(NController)));
                    if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
                    StepAt=Now;return true;
                }
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly")))
                if(Now-StepAt<1.5||!Capture(TEXT("02b-operation-area")))return true;
            NController->BeginBattle();Check(TEXT("controller_opens_real_deployment"),Screen.Deployment);
            if(!Screen.Deployment)return Fail(TEXT("controller_deployment_gate"),NBattle->LastMessage);
            SetStep(EStep::Deployment);return true;
        }
        if(Step==EStep::Deployment)
        {
            if(Now-StepAt<.8||!Capture(TEXT("03-deployment-briefing")))return true;
            auto* HUD=Cast<AWarfareFrontlineHUD>(NController->GetHUD());
            if(!HUD)return Fail(TEXT("actual_loading_hud_exists"),TEXT("Missing actual HUD."));
            LoadingClicksBefore=NavigationV61::EmittedClicksV64;
            const auto Cards=UnitIntroductionCards(NBattle);
            const auto* Mission=NBattle->GetSelectedCampaignMission();const auto& Missions=WarfareFRRUCampaignV82::Missions(NBattle);
            const int32 Order=Mission?Missions.IndexOfByPredicate([&](const FFrontlineCampaignMission& M){return M.Id==Mission->Id;}):INDEX_NONE;
            int32 ExpectedCards=0;bool CurrentAttachments=Order!=INDEX_NONE;
            for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(WarfareFRRUEquipmentV82::FirstMissionForRole(NBattle,EFrontlineRole(I),0)==Order
                &&WarfareFRRUEquipmentV82::RoleAvailable(NBattle,EFrontlineRole(I),0))++ExpectedCards;
            for(const auto Kind:FrontlineSupportCards)if(WarfareFRRUEquipmentV82::FirstMissionForSupport(NBattle,Kind,0)==Order
                &&WarfareFRRUEquipmentV82::SupportAvailable(NBattle,Kind,0))++ExpectedCards;
            for(const auto& Id:WarfareFRRUArmourV82::Roster(NBattle))if(WarfareFRRUArmourV82::FirstMissionForArmour(NBattle,Id)==Order
                &&WarfareFRRUArmourV82::Accepted(NBattle,Id))++ExpectedCards;
            for(const auto& Card:Cards)
            {
                CurrentAttachments&=!Card.Heading.IsEmpty()&&!Card.Title.IsEmpty()&&!Card.Context.IsEmpty()&&!Card.Tactic.IsEmpty();
                if(Card.Role!=EFrontlineRole::Count)CurrentAttachments&=WarfareFRRUEquipmentV82::FirstMissionForRole(NBattle,Card.Role,0)==Order
                    &&WarfareFRRUEquipmentV82::RoleAvailable(NBattle,Card.Role,0);
                else if(Card.Support!=EFrontlineSupport::Count)CurrentAttachments&=WarfareFRRUEquipmentV82::FirstMissionForSupport(NBattle,Card.Support,0)==Order
                    &&WarfareFRRUEquipmentV82::SupportAvailable(NBattle,Card.Support,0);
                else if(!Card.NationalArmour.IsEmpty())CurrentAttachments&=WarfareFRRUArmourV82::FirstMissionForArmour(NBattle,Card.NationalArmour)==Order
                    &&WarfareFRRUArmourV82::Eligible(NBattle,Card.NationalArmour,0)&&WarfareFRRUArmourV82::Accepted(NBattle,Card.NationalArmour);
                else CurrentAttachments=false;
            }
            Check(TEXT("actual_introduction_contains_only_current_dated_native_attachments"),CurrentAttachments&&Cards.Num()==ExpectedCards);
            if(FString(Scenario->Mission)==TEXT("fr.malmaison"))
                Check(TEXT("malmaison_introduces_saint_chamond_without_early_ft_or_repeat_schneider"),
                    Cards.ContainsByPredicate([](const FUnitIntroductionCard& C){return C.NationalArmour==TEXT("fr.saint_chamond");})
                    &&!Cards.ContainsByPredicate([](const FUnitIntroductionCard& C){return C.NationalArmour==TEXT("fr.ft")||C.NationalArmour==TEXT("fr.schneider");}));
            if(FString(Scenario->Mission)==TEXT("ru.riga"))
                Check(TEXT("riga_does_not_repeat_the_warsaw_armoured_car_introduction"),
                    !Cards.ContainsByPredicate([](const FUnitIntroductionCard& C){return C.NationalArmour==TEXT("ru.austin_series1");}));
            HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));
            if(!Cards.IsEmpty())
            {
                Check(TEXT("controller_opens_actual_introduction_before_loading"),Screen.UnitIntroduction&&!Screen.Preparing
                    &&NBattle->Phase==EFrontlinePhase::Briefing&&Screen.UnitIntroductionPage==0);
                if(!Screen.UnitIntroduction)return Fail(TEXT("controller_introduction_gate"),NBattle->LastMessage);
                SetStep(EStep::Introduction);return true;
            }
            Event(TEXT("controller.begin_loading"),Screen.Preparing);
            if(!Screen.Preparing)return Fail(TEXT("controller_loading_gate"),NBattle->LastMessage);
            SetStep(EStep::Loading);return true;
        }
        if(Step==EStep::Introduction)
        {
            if(Now-StepAt<.4)return true;
            const auto Cards=UnitIntroductionCards(NBattle);const int32 Page=Screen.UnitIntroductionPage;
            if(!Screen.UnitIntroduction||!Cards.IsValidIndex(Page))return Fail(TEXT("actual_introduction_page_exists"),TEXT("Missing current native attachment page."));
            if(Page==0&&!Capture(TEXT("03b-unit-introduction")))return true;
            if(!Cards[Page].NationalArmour.IsEmpty()&&!Capture(TEXT("03c-armour-introduction")))return true;
            auto* HUD=Cast<AWarfareFrontlineHUD>(NController->GetHUD());
            if(!HUD)return Fail(TEXT("actual_introduction_hud_exists"),TEXT("Missing actual HUD."));
            const bool Last=Page+1==Cards.Num();if(Last)LoadingClicksBefore=NavigationV61::EmittedClicksV64;
            HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));
            Event(TEXT("controller.confirm_attachment_page"),Last?Screen.Preparing:Screen.UnitIntroduction&&Screen.UnitIntroductionPage==Page+1);
            if(!Last)
            {
                Check(TEXT("actual_introduction_next_page_keeps_host_briefing"),Screen.UnitIntroduction&&Screen.UnitIntroductionPage==Page+1
                    &&!Screen.Preparing&&NBattle->Phase==EFrontlinePhase::Briefing);
                StepAt=Now;return true;
            }
            Check(TEXT("actual_last_introduction_confirmation_enters_loading"),!Screen.UnitIntroduction&&Screen.Preparing
                &&Screen.AcceptedNationalIntroCampaignV827==WarfareFRRUCampaignV82::CampaignId(NBattle)
                &&Screen.AcceptedNationalIntroMissionV827==WarfareFRRUCampaignV82::MissionId(NBattle));
            if(!Screen.Preparing)return Fail(TEXT("controller_loading_after_introduction_gate"),NBattle->LastMessage);
            NController->BeginBattle();
            Check(TEXT("actual_repeated_enter_does_not_reopen_introduction_or_deployment"),Screen.Preparing&&!Screen.UnitIntroduction&&!Screen.Deployment);
            SetStep(EStep::Loading);return true;
        }
        if(Step==EStep::Loading)
        {
            if(!LoadingNarrativeCheckedV826)
            {
                LoadingNarrativeCheckedV826=true;
                const auto* M=NBattle->GetSelectedCampaignMission();
                const FString Key=FrontlineLoadingPageV82::MissionKey(NBattle,M);
                const FrontlineLoadingPageV82::FLoadingNarrativeV82* Authored=nullptr;
                for(const auto& Row:FrontlineLoadingPageV82::LoadingNarrativesV82)if(Key==Row.Key){Authored=&Row;break;}
                const FString Narrative=FrontlineLoadingPageV82::Narrative(NBattle,M);
                const FString Displayed=TextReviewV829::Loading(TEXT("narrative"),Narrative);
                Check(TEXT("actual_loading_uses_current_localized_authored_diary"),M&&Authored
                    &&Authored->En&&*Authored->En&&Authored->Zh&&*Authored->Zh
                    &&Narrative==WarRoom52::L(Authored->En,Authored->Zh)&&!Displayed.TrimStartAndEnd().IsEmpty()
                    &&!Displayed.Contains(TEXT("TODO"))&&!Displayed.Contains(TEXT("placeholder"),ESearchCase::IgnoreCase)
                    &&!Displayed.Contains(TEXT("待补"))&&!Displayed.Contains(TEXT("制作说明")));
            }
            if(NavigationV61::EmittedClicksV64!=LoadingClicksBefore
                ||(NavigationV61::ActiveClickV82.IsValid()&&NavigationV61::ActiveClickV82->IsPlaying()))
                return Fail(TEXT("actual_loading_button_audio_silent"),TEXT("A real button voice was emitted or retained during loading."));
            if(Now-StepAt>120)return Fail(TEXT("actual_loading_warm_frames"),FString::Printf(TEXT("painted=%d frames=%d phase=%d"),Screen.PreparationPainted,Screen.PreparationSceneFrames,int32(NBattle->Phase)));
            if(!LoadingCaptured&&(Screen.PreparationPainted||PendingCaptureName==TEXT("04-loading")))
                LoadingCaptured=Capture(TEXT("04-loading"));
            if(Screen.Preparing||NBattle->Phase!=EFrontlinePhase::Fighting)return true;
            if(!LoadingCaptured)return Fail(TEXT("actual_loading_capture_completed"),TEXT("Loading ended before its screenshot was confirmed."));
            Check(TEXT("actual_loading_button_audio_silent"),true,TEXT("Actual HUD confirm and each painted loading frame preserve the click count and retain no playing button component."));
            Check(TEXT("actual_controller_loading_resumes_real_battle"),!UGameplayStatics::IsGamePaused(NBattle)&&WarfareFRRUWorldV82::Ready(NBattle));SetStep(EStep::WarmFrame);return true;
        }
        if(Step==EStep::WarmFrame)
        {
            V28ReviewPointer=FVector2D(-10000,-10000);
            if((FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate"))||FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")))&&!FTNavigationProofDoneV87)
            {
                const auto Proof=FRRUVehicleNavCandidateV87::PollProof(NBattle);FTNavigationProofPathV87=Proof.Path;
                if(Proof.Status==FRRUVehicleNavCandidateV87::EStatus::Pending)return true;
                if(!FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate")))FRRUVehicleNavCandidateV87::Clear(NBattle);
                FTNavigationProofDoneV87=true;
                if(Proof.Status!=FRRUVehicleNavCandidateV87::EStatus::Success)
                    return Fail(TEXT("actual_transverse_clearance_ft_navigation_query_v87"),Proof.Reason);
                Check(TEXT("actual_transverse_clearance_ft_navigation_query_v87"),true,Proof.Reason);
            }
            if(Now-StepAt<1.||!Capture(TEXT("05-first-warmed-battle")))return true;
            if(FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture")))
            {
                const auto Mechanical=FrontlineSpecialistsV41::RunFrenchVBMechanicsV87(NBattle,[this](AWarfareFrontlineSoldier* Operator)
                {Second=Operator->SquadId;Sample();});
                ReportDetail=[Mechanical](FJsonObject& Report)
                {
                    Report.SetBoolField(TEXT("controlledVBMechanicsFixtureV87"),true);Report.SetStringField(TEXT("acceptanceScope"),TEXT("Controlled French VB mechanical consumer checks only; not natural recruitment, campaign or combat acceptance."));
                    Report.SetBoolField(TEXT("actorsPlaced"),Mechanical->GetBoolField(TEXT("actorsPlaced")));Report.SetBoolField(TEXT("actorsTeleported"),Mechanical->GetBoolField(TEXT("actorsTeleported")));
                    Report.SetBoolField(TEXT("simulationClockFixture"),Mechanical->GetBoolField(TEXT("simulationClockFixture")));
                    Report.SetBoolField(TEXT("simulationClockArtificiallyAdvanced"),Mechanical->GetBoolField(TEXT("simulationClockFixture")));
                    Report.SetBoolField(TEXT("actualDamageResolution"),false);Report.SetBoolField(TEXT("combatAccepted"),false);Report.SetBoolField(TEXT("worldAccepted"),false);
                    Report.SetBoolField(TEXT("campaignAccepted"),false);Report.SetBoolField(TEXT("historicalPlayerSaveAccepted"),false);Report.SetBoolField(TEXT("shippingAccepted"),false);
                    Report.SetBoolField(TEXT("finalCameraReframedWithoutActorTeleport"),false);Report.SetObjectField(TEXT("vbMechanicalFixtureV87"),Mechanical);
                };
                Check(TEXT("controlled_french_vb_mechanical_consumer"),Mechanical->GetBoolField(TEXT("allPassed")));
                Check(TEXT("controlled_vb_actual_single_round_pair_projectile_and_restore"),VBLoaded&&VBEmpty&&VBPairFinite&&VBProjectile&&VBRestored&&VBReloadSamples>=3);
                Finish();return !Finished;
            }
            if(auto* Audio=NBattle->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunFRRUV82AudioChecks([&](const FString& Name,bool Pass){Check(TEXT("actual_national_audio.")+Name,Pass);});
            OriginalCameraFocus=NController->CameraFocus;OriginalCameraWidth=NController->TargetCameraWidth;
            NController->CameraFocus.X=FMath::Clamp(NController->CameraFocus.X+1250.,-9000.,9000.);
            NController->CameraFocus.Y=FMath::Clamp(NController->CameraFocus.Y+600.,-3000.,3000.);
            NController->TargetCameraWidth=FMath::Max(3400.f,NController->TargetCameraWidth-700.f);SetStep(EStep::Reframed);return true;
        }
        if(Step==EStep::Reframed)
        {
            if(Now-StepAt<1.||!Capture(TEXT("06-battle-moved-zoomed")))return true;
            if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82TerrainBuffers")))
            {
                auto* Viewport=NBattle->GetWorld()->GetGameViewport();
                auto* Target=IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
                const bool ExactTerrain=(Scenario->Probe==EProbe::Rifle&&(FString(Scenario->Mission)==TEXT("fr.mort_homme")||FString(Scenario->Mission)==TEXT("fr.vaux")||FString(Scenario->Mission)==TEXT("ru.naroch")))
                    ||(Scenario->Probe==EProbe::FT&&FString(Scenario->Mission)==TEXT("fr.matz"));
                auto* Lumen=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Visualize"));
                if(!Viewport||!Target||!Lumen||!ExactTerrain||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly")))
                    return Fail(TEXT("terrain_buffer_exact_context"),TEXT("Read-only material diagnosis requires isolated Mort or Matz capture-only scope."));
                const TCHAR* Buffers[]={TEXT("BaseColor"),TEXT("Roughness"),TEXT("WorldNormal")};
                const TCHAR* Names[]={TEXT("06a-terrain-base-colour"),TEXT("06b-terrain-roughness"),TEXT("06c-terrain-world-normal")};
                if(TerrainBufferStage<3)
                {
                    if(TerrainBufferStarted==0)
                    {Target->Set(Buffers[TerrainBufferStage],ECVF_SetByCode);Viewport->SetViewMode(VMI_VisualizeBuffer);TerrainBufferStarted=Now;return true;}
                    if(Now-TerrainBufferStarted<.8||!Capture(Names[TerrainBufferStage]))return true;
                    Check(FString(TEXT("actual_terrain_buffer."))+Buffers[TerrainBufferStage],Viewport->ViewModeIndex==VMI_VisualizeBuffer&&Target->GetString()==Buffers[TerrainBufferStage]);
                    ++TerrainBufferStage;TerrainBufferStarted=0;return true;
                }
                if(TerrainBufferStage<5)
                {
                    const int32 Mode=TerrainBufferStage==3?5:13;
                    const TCHAR* Name=TerrainBufferStage==3?TEXT("06d-lumen-surface-cache"):TEXT("06e-lumen-indirect-lighting");
                    if(TerrainBufferStarted==0)
                    {Target->Set(TEXT("None"),ECVF_SetByCode);Viewport->SetViewMode(VMI_Lit);Lumen->Set(Mode,ECVF_SetByCode);TerrainBufferStarted=Now;return true;}
                    if(Now-TerrainBufferStarted<1.5||!Capture(Name))return true;
                    Check(Name,Lumen->GetInt()==Mode);++TerrainBufferStage;TerrainBufferStarted=0;return true;
                }
                if(TerrainBufferStage==5)
                {Lumen->Set(0,ECVF_SetByCode);Target->Set(TEXT("None"),ECVF_SetByCode);Viewport->SetViewMode(VMI_Lit);++TerrainBufferStage;TerrainBufferStarted=Now;return true;}
                if(Now-TerrainBufferStarted<.8)return true;
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly")))
            {
                // A story may become available after the first warm frame.
                // Use its normal command before any later art capture too.
                const auto VisualStory=WarfareFRRUWorldV82::View(NBattle);
                if(VisualStory.StoryPending&&!VisualStory.ChoiceIds.IsEmpty())
                    Event(TEXT("visual_public_story_choice"),WarfareFRRUWorldV82::ChooseStory(NBattle,VisualStory.StoryId,VisualStory.ChoiceIds[0]));
                if(VisualCameraStage==0)
                {
                    const auto Story=WarfareFRRUWorldV82::View(NBattle);
                    if(Story.StoryPending&&!Story.ChoiceIds.IsEmpty())
                        Event(TEXT("visual_public_story_choice"),WarfareFRRUWorldV82::ChooseStory(NBattle,Story.StoryId,Story.ChoiceIds[0]));
                    NController->CameraFocus=FVector::ZeroVector;NController->TargetCameraWidth=9000;VisualCameraStage=1;StepAt=Now;return true;
                }
                if(VisualCameraStage==1)
                {
                    if(Now-StepAt<1.5||!Capture(TEXT("07-central-battlefield")))return true;
                    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87TripoRuin")))
                    {
                        // A private inspection only. Keep the ordinary camera
                        // and every neighbouring native building unchanged.
                        // Seed the existing close view; the actual rendered
                        // component is framed after its orthographic width settles.
                        Sample();NController->CameraFocus=FVector(1050,800,-400);NController->TargetCameraWidth=1800;
                        VisualCameraStage=3;StepAt=Now;return true;
                    }
                    Sample();NController->CameraFocus=FVector(6500,0,0);NController->TargetCameraWidth=6000;VisualCameraStage=2;StepAt=Now;return true;
                }
                if(VisualCameraStage==3)
                {
                    if(!FrameTripoCloseV87(16,Now))return !Finished;
                    if(!Capture(TEXT("09-tripo-ruin-close")))return true;
                    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87TripoCluster")))
                    {
                        Sample();NController->CameraFocus=FVector(2000,800,-400);NController->TargetCameraWidth=1800;
                        VisualCameraStage=4;StepAt=Now;return true;
                    }
                    Sample();NController->CameraFocus=FVector(6500,0,0);NController->TargetCameraWidth=6000;VisualCameraStage=2;StepAt=Now;return true;
                }
                if(VisualCameraStage==4)
                {
                    if(!FrameTripoCloseV87(18,Now))return !Finished;
                    if(!Capture(TEXT("10-tripo-stable18-close")))return true;
                    Sample();NController->CameraFocus=FVector(1050,1750,-400);NController->TargetCameraWidth=1800;
                    VisualCameraStage=5;StepAt=Now;return true;
                }
                if(VisualCameraStage==5)
                {
                    if(!FrameTripoCloseV87(20,Now))return !Finished;
                    if(!Capture(TEXT("11-tripo-workshop20-close")))return true;
                    Sample();NController->CameraFocus=FVector(6500,0,0);NController->TargetCameraWidth=6000;VisualCameraStage=2;StepAt=Now;return true;
                }
                if(Now-StepAt<1.5||!Capture(TEXT("08-eastern-approach")))return true;
                Sample();WriteReport();return false;
            }
            NController->CameraFocus=OriginalCameraFocus;NController->TargetCameraWidth=OriginalCameraWidth;
            SetStep(EStep::Recruit);return true;
        }
        if(Now>=NextSample){Sample();ObserveFormationJoinV826();NextSample=Now+(Scenario->Probe==EProbe::Schilt||Scenario->Probe==EProbe::VB?.10:.5);}
        if(FormationJoinOnlyV826()&&JoinMotionObservedV826)
        {Finish();return !Finished;}
        if(Scenario->Probe==EProbe::FrenchPlan)return TickFrenchPlanV87(Now);
        if(Scenario->Probe==EProbe::Ordinary)return TickOrdinaryV827(Now);
        if(Scenario->Probe==EProbe::VB&&VBPioneerRecruitedV87&&VBPioneerV87!=INDEX_NONE
            &&!WarfareFRRUWorldV82::PassageCleared(NBattle,TEXT("wire")))
            if(const auto* Group=NBattle->FindSquad(VBPioneerV87);Group&&Group->Team==0
                &&Group->Role==EFrontlineRole::Pioneer&&Group->LivingCount()==0)
            {
                bool ConfirmedDeaths=!Group->Members.IsEmpty();TArray<TSharedPtr<FJsonValue>> Deaths;
                for(const auto& Weak:Group->Members)
                {
                    const auto* Unit=Weak.Get();
                    if(!IsValid(Unit)||Unit->Battle.Get()!=NBattle||Unit->SquadId!=VBPioneerV87||Unit->Team!=0
                        ||Unit->Role!=EFrontlineRole::Pioneer||Unit->IsAlive()||!Unit->GetDeathState().bStarted)
                    {ConfirmedDeaths=false;continue;}
                    const auto& Source=Unit->FatalSourceV65;const auto& Death=Unit->GetDeathState();
                    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("actualActor"),Unit->GetPathName());
                    Row->SetNumberField(TEXT("actualMember"),Unit->MemberIndex);Row->SetNumberField(TEXT("deathStateAge"),Death.Age);
                    Row->SetNumberField(TEXT("fatalDamageKind"),int32(Death.Kind));Row->SetNumberField(TEXT("fatalSourceTeam"),Source.Team);
                    Row->SetNumberField(TEXT("fatalSourceSquad"),Source.SquadId);Row->SetNumberField(TEXT("fatalSourceRole"),Source.Role);
                    Row->SetNumberField(TEXT("fatalSupport"),Source.Support);Row->SetNumberField(TEXT("fatalFireMission"),Source.FireMissionId);
                    Row->SetNumberField(TEXT("fatalVehicleSerial"),Source.VehicleSerial);Deaths.Add(MakeShared<FJsonValueObject>(Row));
                }
                if(ConfirmedDeaths)
                {
                    Sample(true);Event(TEXT("vb_actual_pioneer_lost_before_clearance"),false);
                    const auto Evidence=Commands.Last()->AsObject();Evidence->SetNumberField(TEXT("actualSquad"),VBPioneerV87);
                    Evidence->SetNumberField(TEXT("observedHostElapsed"),NBattle->Elapsed);Evidence->SetBoolField(TEXT("publicRecruitConfirmed"),true);
                    Evidence->SetBoolField(TEXT("actualWireCleared"),false);Evidence->SetNumberField(TEXT("actualLivingCount"),0);
                    Evidence->SetArrayField(TEXT("actualMemberDeaths"),Deaths);
                    return Fail(TEXT("vb_actual_pioneer_lost_before_clearance"),TEXT("The actual publicly recruited pioneer section has no living members and every original member has a confirmed death record before wire clearance. Clearance, return and the dependent VB approach cannot complete in this bounded attempt; retain deep sampling and actual fatal sources. No replacement or rule change is issued."));
                }
            }
        if(NBattle->Phase!=EFrontlinePhase::Fighting)
        {
            const auto Terminal=WarfareFRRUWorldV82::View(NBattle);
            FinalContestCleared=Scenario->Probe==EProbe::FT&&RealStraddler&&RealHullContest&&Terminal.Finished&&Terminal.Success;
            if(FinalContestCleared&&VehicleRolled&&VehicleShot&&VehiclePoseChecked&&VehicleTrackChecked&&VehicleGunChecked)
            {Check(TEXT("actual_final_contest_blocked_then_cleared_by_real_outcome"),true);Finish();return !Finished;}
            return Fail(TEXT("natural_window_not_lost_to_early_resolution"),NBattle->EndReason);
        }
        if(Step==EStep::Recruit)
        {
            // Test only an actually unavailable binding. No asset readiness flag,
            // elapsed time, cooldown array or actor state is edited to force it.
            FString Error;const EFrontlineRole Rejected=EFrontlineRole::Lewis;
            if(!WarfareFRRUPresentationV82::RoleAssetsReady(NBattle,Rejected,0,Error)&&NBattle->RecruitCooldown(Rejected,0)<=0)
            {
                const int32 UnitCount=NBattle->Units.Num(),SquadCount=NBattle->Squads.Num(),CommandsBefore=NBattle->RecruitCommands;
                const float Before=NBattle->RecruitCooldown(EFrontlineRole::Rifle,0);const bool Accepted=NBattle->Recruit(Rejected,0);
                Check(TEXT("unready_binding_recruit_rejected_without_publication"),!Accepted&&NBattle->Units.Num()==UnitCount&&NBattle->Squads.Num()==SquadCount&&NBattle->RecruitCommands==CommandsBefore
                    &&FMath::IsNearlyEqual(Before,NBattle->RecruitCooldown(EFrontlineRole::Rifle,0),.001f),Error);NegativeRecruitChecked=true;
                if(Accepted)return Fail(TEXT("unready_binding_cannot_create_live_meshless_actors"),Error);
            }
            Lead=Recruit(EFrontlineRole::Rifle,0);if(Lead==INDEX_NONE)return Fail(TEXT("real_rifle_recruit_after_negative_attempt"),NBattle->LastMessage);
            EntryOrderedAt=NBattle->Elapsed;
            if(const auto* Group=NBattle->FindSquad(Lead))for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive())
            {EntryStartingFeet.Add(Unit->MemberIndex,Unit->Feet());Check(TEXT("recruit_member_starts_outside_real_entry_")+FString::FromInt(Unit->MemberIndex),!Unit->bFieldEntered&&!FrontlinePacingV24::HasCrossedEntry(Unit->Feet().X,NBattle->GetTeamDirection(0)));}
            SetStep(EStep::Entry);return true;
        }
        if(Step==EStep::Entry)
        {
            if(const auto* Group=NBattle->FindSquad(Lead))for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->bFieldEntered&&!MemberEntryTimes.Contains(Unit->MemberIndex))
            {
                const float Seconds=NBattle->Elapsed-EntryOrderedAt;MemberEntryTimes.Add(Unit->MemberIndex,Seconds);
                const FVector* Start=EntryStartingFeet.Find(Unit->MemberIndex);
                Check(TEXT("member_natural_entry_time_")+FString::FromInt(Unit->MemberIndex),Start&&Seconds>=1.5f&&Seconds<=5.5f&&FVector::Dist2D(*Start,Unit->Feet())>500,
                    FString::Printf(TEXT("observed %.3fs; no instant entry and real movement from an offstage start"),Seconds));
            }
            if(!RifleEntry&&MemberEntryTimes.Num()==EntryStartingFeet.Num()&&!EntryStartingFeet.IsEmpty())
            {RifleEntry=true;Check(TEXT("all_living_recruits_crossed_real_entry_independently"),true);}
            if(!At(Lead,TEXT("home")))
            {
                // Keep the real all-member stationary/entry gate intact. A
                // blocked approach must leave an actual state and camera
                // receipt instead of waiting for the external process limit.
                if(NBattle->Elapsed-EntryOrderedAt>=45.f)
                {Sample(true);return Fail(TEXT("all_living_recruits_reach_reserved_home_bay"),TEXT("45 real host seconds passed without all living members settling in the home post. See member movement diagnostics and final camera capture."));}
                return true;
            }
            if(auto* Unit=Member(Lead))
            {const auto Probe=WarfareFRRUPresentationV82::ProbeUnit(Unit);Check(TEXT("actual_rifle_body_weapon_binding"),Probe->GetBoolField(TEXT("actualBinding")));}
            PauseElapsed=NBattle->Elapsed;PauseStrikes=NBattle->Strikes.Num();PauseResolved=WarfareFRRUWorldV82::View(NBattle).ResolvedSalvos;
            PausedFeet.Empty();for(AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit)PausedFeet.Add(Unit->SquadId*16+Unit->MemberIndex,Unit->Feet());
            NBattle->SetPaused(true);SetStep(EStep::Pause);return true;
        }
        if(Step==EStep::Pause)
        {
            if(Now-StepAt<2)return true;bool Same=FMath::IsNearlyEqual(PauseElapsed,NBattle->Elapsed,.0001f)&&PauseStrikes==NBattle->Strikes.Num()&&PauseResolved==WarfareFRRUWorldV82::View(NBattle).ResolvedSalvos;
            for(AWarfareFrontlineSoldier* Unit:NBattle->Units)if(Unit)if(const FVector* Previous=PausedFeet.Find(Unit->SquadId*16+Unit->MemberIndex))Same&=FVector::Dist(*Previous,Unit->Feet())<.001;
            Check(TEXT("pause_keeps_single_host_clock_positions_and_support"),Same);NBattle->SetPaused(false);
            if(!WarfareFRRUWorldV82::MoveSquad(NBattle,Lead,TEXT("assembly")))return Fail(TEXT("ordinary_real_registered_route"),WarfareFRRUWorldV82::View(NBattle).Notice);
            SetStep(EStep::Hold);return true;
        }
        if(Step==EStep::Hold)
        {
            auto* Unit=Member(Lead);if(!Unit)return Fail(TEXT("lead_survives_rear_route"),TEXT("No living lead remains."));
            if(Unit->DistanceTravelled<600||Unit->GetMotionSpeed()<1)return true;
            LeadBeforeMove=Unit->DistanceTravelled;LastLeadFeet=Unit->Feet();
            if(!WarfareFRRUWorldV82::HoldSquad(NBattle,Lead,true))return Fail(TEXT("hold_existing_in_transit_squad"),WarfareFRRUWorldV82::View(NBattle).Notice);
            HoldChecked=true;SetStep(EStep::Resume);return true;
        }
        if(Step==EStep::Resume)
        {
            if(Now-StepAt<1.5)return true;
            if(HoldChecked)
            {
                auto* Unit=Member(Lead);Check(TEXT("hold_does_not_advance_or_discard_physical_route"),Unit&&FVector::Dist2D(LastLeadFeet,Unit->Feet())<8);
                if(!WarfareFRRUWorldV82::HoldSquad(NBattle,Lead,false))return Fail(TEXT("resume_retains_real_route_and_bay"),WarfareFRRUWorldV82::View(NBattle).Notice);
                HoldChecked=false;StepAt=Now;return true;
            }
            if(auto* Unit=Member(Lead);Unit&&Unit->DistanceTravelled>LeadBeforeMove+120){Check(TEXT("resume_moves_by_actual_displacement"),true);SetStep(EStep::Specialist);}return true;
        }
        if(Step==EStep::Specialist)
        {
            if(Scenario->Probe==EProbe::VB)
            {TickVBPublicOrdersV87();if(VBPioneerV87==INDEX_NONE)return true;}
            if(Scenario->Probe==EProbe::Rifle){SetStep(EStep::Combat);return true;}
            if(Scenario->Probe==EProbe::Train)
            {
                TickTrainContactOrders();
                // Wait in real time until 30s, before the ordinary enemy
                // armour assessment at 35s. The contact section follows its
                // own route while the carrier enters; neither clock is edited.
                if(TrainContactSquad==INDEX_NONE||NBattle->Elapsed<30.f)return true;
            }
            if((Scenario->Probe==EProbe::CA1||Scenario->Probe==EProbe::Saint)&&Second==INDEX_NONE)
            {
                if(!NBattle->RecruitBlockReason(EFrontlineRole::Pioneer,0).IsEmpty())return true;
                Second=Recruit(EFrontlineRole::Pioneer,0);
                if(Second==INDEX_NONE)return Fail(TEXT("real_armour_window_pioneer_recruit"),NBattle->LastMessage);
                Check(TEXT("armour_clearance_uses_real_shared_cooldown_pioneer"),true);
            }
            if(Scenario->Probe==EProbe::Hotchkiss||Scenario->Probe==EProbe::Reserve||Scenario->Probe==EProbe::Schilt||Scenario->Probe==EProbe::VB)
            {
                const EFrontlineRole Role=Scenario->Probe==EProbe::Hotchkiss?EFrontlineRole::Lewis:Scenario->Probe==EProbe::Schilt?EFrontlineRole::Flamethrower:Scenario->Probe==EProbe::VB?EFrontlineRole::RifleGrenadier:EFrontlineRole::Pioneer;
                if(!SpecialRecruited&&NBattle->RecruitBlockReason(Role,0).IsEmpty())
                {Second=Recruit(Role,0);if(Second==INDEX_NONE)return Fail(TEXT("real_specialist_recruit"),NBattle->LastMessage);SpecialRecruited=true;}
            }
            else if(!SpecialRecruited)
            {
                const FString VehicleId=VehicleCaseId();
                const int32 Side=Scenario->Probe==EProbe::Train?1:0;
                const FString PreparationBlock=WarfareFRRUArmourV82::BlockReason(NBattle,VehicleId,Side);
                if(!PreparationBlock.IsEmpty())
                {
                    // Wait on this exact model's real encounter queue. A zero
                    // cooldown with another refusal remains a genuine failure.
                    if(WarfareFRRUArmourV82::CooldownFraction(NBattle,Side,VehicleId)>0.f)return true;
                    return Fail(TEXT("actual_national_armour_recruit"),PreparationBlock);
                }
                auto* ActualFleet=UFrontlineVehicleSubsystem::Get(NBattle->GetWorld());TSet<int32> ExistingSerials;
                if(ActualFleet)for(AFrontlineTank* ActualVehicle:ActualFleet->GetVehicles())if(ActualVehicle)ExistingSerials.Add(ActualVehicle->Serial);
                const bool Accepted=WarfareFRRUArmourV82::Recruit(NBattle,VehicleId,Side);Event(TEXT("national_armour.")+VehicleId,Accepted);
                if(!Accepted)return Fail(TEXT("actual_national_armour_recruit"),WarfareFRRUArmourV82::BlockReason(NBattle,VehicleId,Side));
                if(ActualFleet)for(AFrontlineTank* ActualVehicle:ActualFleet->GetVehicles())if(ActualVehicle&&!ExistingSerials.Contains(ActualVehicle->Serial)&&ActualVehicle->Team==Side&&WarfareFRRUArmourV82::Id(ActualVehicle)==VehicleId)
                {ReviewVehicle=ActualVehicle;ReviewVehicleSerial=ActualVehicle->Serial;break;}
                Check(TEXT("accepted_vehicle_recruit_binds_exact_new_actual_serial"),ReviewVehicle.IsValid());
                SpecialRecruited=true;
                if(Side==0){const int32 Before=UFrontlineVehicleSubsystem::Get(NBattle->GetWorld())->GetVehicles().Num();
                    const bool Repeated=WarfareFRRUArmourV82::Recruit(NBattle,VehicleId,Side);VehicleSharedCD=!Repeated&&Before==UFrontlineVehicleSubsystem::Get(NBattle->GetWorld())->GetVehicles().Num();Check(TEXT("national_shared_armour_cooldown_has_no_second_spawn"),VehicleSharedCD);}
            }
            if(SpecialRecruited)SetStep(EStep::Combat);return true;
        }
        if(Step==EStep::Combat)
        {
            if(Scenario->Probe==EProbe::VB&&Second!=INDEX_NONE
                &&!(VBLoaded&&VBEmpty&&VBPairFinite&&VBProjectile&&VBRestored&&VBReloadSamples>=3))
                if(const auto* Group=NBattle->FindSquad(Second))for(const auto& Weak:Group->Members)
                    if(const auto* Unit=Weak.Get();IsValid(Unit)&&Unit->Battle.Get()==NBattle&&Unit->SquadId==Second
                        &&Unit->MemberIndex==0&&Unit->Role==EFrontlineRole::RifleGrenadier&&!Unit->IsAlive()&&Unit->GetDeathState().bStarted)
                    {
                        Sample(true);if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
                        if(VBLoaded&&VBEmpty&&VBPairFinite&&VBProjectile&&VBRestored&&VBReloadSamples>=3)
                        {Check(TEXT("real_vb_single_round_fire_empty_reload_pair_and_restoration"),true);Finish();return !Finished;}
                        TArray<TSharedPtr<FJsonValue>> Missing;FString MissingNames;
                        const auto MissingWindow=[&](const TCHAR* Name,bool Passed)
                        {if(!Passed){Missing.Add(MakeShared<FJsonValueString>(Name));if(!MissingNames.IsEmpty())MissingNames+=TEXT(",");MissingNames+=Name;}};
                        MissingWindow(TEXT("initial_loaded_round"),VBLoaded);MissingWindow(TEXT("empty_after_shot"),VBEmpty);
                        MissingWindow(TEXT("six_bone_reload_pair"),VBPairFinite);MissingWindow(TEXT("projectile_tangent"),VBProjectile);
                        MissingWindow(TEXT("loaded_round_restored"),VBRestored);MissingWindow(TEXT("three_reload_samples"),VBReloadSamples>=3);
                        const auto& Source=Unit->FatalSourceV65;const auto& Death=Unit->GetDeathState();
                        Event(TEXT("vb_actual_operator_lost_before_role_cycle_complete"),false);
                        const auto Evidence=Commands.Last()->AsObject();Evidence->SetStringField(TEXT("actualActor"),Unit->GetPathName());
                        Evidence->SetNumberField(TEXT("actualSquad"),Second);Evidence->SetNumberField(TEXT("actualMember"),Unit->MemberIndex);
                        Evidence->SetNumberField(TEXT("observedHostElapsed"),NBattle->Elapsed);Evidence->SetNumberField(TEXT("actualDeathAge"),Death.Age);
                        Evidence->SetNumberField(TEXT("fatalDamageKind"),int32(Death.Kind));Evidence->SetNumberField(TEXT("fatalSourceTeam"),Source.Team);
                        Evidence->SetNumberField(TEXT("fatalSourceSquad"),Source.SquadId);Evidence->SetNumberField(TEXT("fatalSourceRole"),Source.Role);
                        Evidence->SetNumberField(TEXT("fatalSupport"),Source.Support);Evidence->SetNumberField(TEXT("fatalFireMission"),Source.FireMissionId);
                        Evidence->SetNumberField(TEXT("fatalVehicleSerial"),Source.VehicleSerial);Evidence->SetArrayField(TEXT("missingRequiredWindows"),Missing);
                        return Fail(TEXT("vb_actual_operator_lost_before_role_cycle_complete"),FString::Printf(
                            TEXT("Actual operator %s squad=%d member=0 died before VB cycle completion; observed_host=%.3f source_side=%d source_squad=%d source_role=%d missing=%s"),
                            *Unit->GetPathName(),Second,NBattle->Elapsed,Source.Team,Source.SquadId,Source.Role,*MissingNames));
                    }
            if(Scenario->Probe==EProbe::Schilt&&FlamePaused)
            {
                if(Now<FlamePauseUntil)return true;
                FFrontlineFlameStateV82 State;auto* Unit=Member(Second);
                const bool Stable=Unit&&FrontlineFieldV32::QueryFlameStateV82(Unit,State)&&State.HostElapsed==PausedFlame.HostElapsed
                    &&State.StartedAt==PausedFlame.StartedAt&&State.JetEndsAt==PausedFlame.JetEndsAt&&State.ReadyAt==PausedFlame.ReadyAt
                    &&State.FuelCharges==PausedFlame.FuelCharges&&Unit->Feet().Equals(FlamePauseFeet,.001);
                Check(TEXT("pause_during_live_schilt_keeps_authoritative_clock_fuel_and_pose_origin"),Stable);
                NBattle->SetPaused(false);FlamePaused=false;FlamePauseDone=true;
            }
            TickReloadWindows();TickFlameWindows();TickVehicleWindows();TickTrainWindows();
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            if(Scenario->Probe==EProbe::VB&&VBNoShotSinceV87>=0&&NBattle->Elapsed-VBNoShotSinceV87>=5.f)
            {
                Sample(true);Event(TEXT("vb_stationary_visible_in_range_without_first_shot"),false);
                const auto Evidence=Commands.Last()->AsObject();Evidence->SetNumberField(TEXT("windowStartedAtHost"),VBNoShotSinceV87);
                Evidence->SetNumberField(TEXT("windowDurationHostSeconds"),NBattle->Elapsed-VBNoShotSinceV87);
                if(VBNoShotEvidenceV87.IsValid())Evidence->SetObjectField(TEXT("actualPassiveFireReadback"),VBNoShotEvidenceV87);
                return Fail(TEXT("vb_stationary_visible_in_range_without_first_shot"),TEXT("The actual alive, Ready, unheld stationary VB operator observed an active enemy in real fire range for five host seconds without its first discharge. Inspect passive focus/clocks/muzzle/rays and the full failure sample; this does not bypass any guard or itself establish a production bug."));
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82FTContactOnly"))&&VehicleRolled&&FTPhaseWrapObserved)
            {Check(TEXT("natural_ft_roll_600cm_and_compressed_phase_wrap"),true,TEXT("Only natural locomotion/contact; weapon, objective and Shipping acceptance remain separate."));Finish();return !Finished;}
            // Observation stays fresh at four real calls per second. An editor
            // ticker never manufactures hundreds of duplicate player commands.
            if(Now<NextOrders)return true;NextOrders=Now+.25;
            const auto View=WarfareFRRUWorldV82::View(NBattle);
            TickTrainContactOrders();
            TickTrainCombatRecruit();
            TickReserveOrders();if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            if(View.StoryPending&&!View.ChoiceIds.IsEmpty())
                Event(TEXT("actual_story_choice"),WarfareFRRUWorldV82::ChooseStory(NBattle,View.StoryId,View.ChoiceIds[0]));
            if(Scenario->Probe==EProbe::Schilt&&View.RequiredRole==int32(EFrontlineRole::Pioneer)
                &&SchiltPioneerV87==INDEX_NONE&&NBattle->RecruitBlockReason(EFrontlineRole::Pioneer,0).IsEmpty())
            {
                SchiltPioneerV87=Recruit(EFrontlineRole::Pioneer,0);
                if(SchiltPioneerV87==INDEX_NONE)return Fail(TEXT("schilt_actual_pioneer_recruit"),NBattle->LastMessage);
                Check(TEXT("schilt_clearance_uses_independent_public_pioneer"),SchiltPioneerV87!=Second);
            }
            // A role-specific objective is assigned to the actual specialist,
            // never to the rifle lead merely because it arrived first.
            const int32 ObjectiveSquad=Scenario->Probe==EProbe::VB?INDEX_NONE:Scenario->Probe==EProbe::FT?FTObjectiveSquad(View):View.RequiredRole==int32(EFrontlineRole::Pioneer)?
                (Scenario->Probe==EProbe::VB?INDEX_NONE:Scenario->Probe==EProbe::Schilt?SchiltPioneerV87:Second):Lead;
            const FString SmokeApproach=View.StageId+TEXT("|")+FString::FromInt(ObjectiveSquad)+TEXT("|")+View.TargetNode;
            if(Scenario->Probe==EProbe::FT&&ObjectiveSquad!=INDEX_NONE&&NBattle->Elapsed>=50.f
                &&FTSmokeApproach!=SmokeApproach&&!At(ObjectiveSquad,View.TargetNode)
                &&NBattle->SupportBlockReason(EFrontlineSupport::Smoke,0).IsEmpty())
            {
                FVector Cover;float Radius=0;
                if(WarfareFRRUWorldV82::NodePose(NBattle,View.TargetNode,Cover,Radius))
                {
                    const bool Accepted=NBattle->RequestSupport(EFrontlineSupport::Smoke,Cover,0);
                    Event(TEXT("ordinary_smoke_for_live_objective_approach"),Accepted);if(Accepted)FTSmokeApproach=SmokeApproach;
                }
            }
            // The faster generic objective poll must not bypass the public
            // pioneer hold while its covering Maxim is still establishing fire.
            if((Scenario->Probe!=EProbe::Reserve||(SecondReserveReleased
                &&(ObjectiveSquad!=Second||ReservePioneerScreenReleased)))&&ObjectiveSquad!=INDEX_NONE&&NBattle->Elapsed>=NextObjectiveAttemptAt)
            {
                const auto* Group=NBattle->FindSquad(ObjectiveSquad);FString From;
                if(Group&&!Group->bAdvancing&&!Group->bHoldPosition&&Group->LivingCount()>0)
                    for(const auto& Weak:Group->Members)if(const auto* U=Weak.Get();U&&U->IsAlive()&&U->bFieldEntered)
                    {WarfareFRRUWorldV82::MovementDiagnosis(U)->TryGetStringField(TEXT("currentNode"),From);break;}
                const FString Key=View.StageId+TEXT("|")+FString::FromInt(ObjectiveSquad)+TEXT("|")+From+TEXT("|")+View.TargetNode+TEXT("|")+FString::FromInt(int32(View.FallbackAwaitingArrival));
                if(!From.IsEmpty()&&From!=View.TargetNode&&At(ObjectiveSquad,From)&&LastObjectiveOrder!=Key)
                {
                    const bool Accepted=WarfareFRRUWorldV82::Command(NBattle,View.FallbackAwaitingArrival?EFRRUTacticalCommandV82::PolicyOrder:EFRRUTacticalCommandV82::AdvanceToObjective,{ObjectiveSquad},View.TargetNode);
                    Event(TEXT("actual_role_objective_order"),Accepted);
                    Commands.Last()->AsObject()->SetStringField(TEXT("fromNode"),From);Commands.Last()->AsObject()->SetStringField(TEXT("stage"),View.StageId);
                    if(Scenario->Probe==EProbe::FT&&View.TargetNode==TEXT("assembly")&&From==TEXT("objective"))Event(TEXT("actual_ft_return_to_rear_objective"),Accepted);
                    NextObjectiveAttemptAt=NBattle->Elapsed+2.;if(Accepted)LastObjectiveOrder=Key;
                }
            }
            if(Scenario->Probe==EProbe::Hotchkiss&&Second!=INDEX_NONE&&At(Second,TEXT("home")))
                Event(TEXT("gun_real_route_to_objective"),WarfareFRRUWorldV82::MoveSquad(NBattle,Second,View.TargetNode));
            if(Scenario->Probe==EProbe::Schilt&&Second!=INDEX_NONE)
            {
                if(!ClearanceNode.IsEmpty()&&WarfareFRRUWorldV82::PassageCleared(NBattle,ClearanceNode)
                    &&View.RequiredRole!=int32(EFrontlineRole::Pioneer)&&!View.TargetNode.IsEmpty()
                    &&NBattle->Elapsed>=SchiltContactRetryAtV87)
                {
                    const auto* Group=NBattle->FindSquad(Second);auto* Operator=Member(Second);FFrontlineFlameStateV82 State;
                    if(Group&&Group->LivingCount()>0&&!Group->bAdvancing&&!Group->bHoldPosition&&Operator&&Operator->IsActiveCombatant()
                        &&FrontlineFieldV32::QueryFlameStateV82(Operator,State)&&!State.Bracing)
                    {
                        FString From;const auto Actual=WarfareFRRUWorldV82::MovementDiagnosis(Operator,false);
                        const FString Key=View.StageId+TEXT("|")+FString::FromInt(Second)+TEXT("|");
                        if(Actual->TryGetStringField(TEXT("actualNativeNode"),From)&&!From.IsEmpty()&&From!=View.TargetNode&&At(Second,From))
                        {
                            const FString Order=Key+From+TEXT("|")+View.TargetNode;
                            if(SchiltContactOrderV87!=Order)
                            {
                                const bool Accepted=WarfareFRRUWorldV82::MoveSquad(NBattle,Second,View.TargetNode);
                                Event(TEXT("schilt_real_near_contact_route"),Accepted);
                                const auto Evidence=Commands.Last()->AsObject();Evidence->SetStringField(TEXT("actualSourceNode"),From);
                                Evidence->SetStringField(TEXT("actualTargetNode"),View.TargetNode);Evidence->SetStringField(TEXT("actualStage"),View.StageId);
                                Evidence->SetNumberField(TEXT("actualSquad"),Second);
                                SchiltContactRetryAtV87=NBattle->Elapsed+2.f;if(Accepted)SchiltContactOrderV87=Order;
                            }
                        }
                    }
                }
                if(!FlamePauseDone)if(auto* Unit=Member(Second);Unit&&FrontlineFieldV32::QueryFlameStateV82(Unit,PausedFlame)&&PausedFlame.JetActive)
                {FlamePauseFeet=Unit->Feet();NBattle->SetPaused(true);FlamePaused=true;FlamePauseUntil=Now+1.5;return true;}
                if(FlameBrace&&FlameJet&&FlameRecovery&&FlameEnemyBurn&&FlamePauseDone&&FlameShotsObserved==12&&EmptyRefusalChecked&&RuptureChecked)
                {Check(TEXT("schilt_all_natural_and_independent_damage_windows_complete"),true);Finish();return !Finished;}
            }
            bool ContactReadyForReviewV87=Scenario->Probe!=EProbe::VB;
            if(Scenario->Probe==EProbe::VB&&Second!=INDEX_NONE)
            {
                TickVBPublicOrdersV87();
                if(VBLoaded&&VBEmpty&&VBPairFinite&&VBProjectile&&VBRestored&&VBReloadSamples>=3)
                {Check(TEXT("real_vb_single_round_fire_empty_reload_pair_and_restoration"),true);Finish();return !Finished;}
            }
            if(ContactReadyForReviewV87&&!HostileRecruited&&NBattle->RecruitBlockReason(EFrontlineRole::Rifle,1).IsEmpty())
            {Hostile=Recruit(EFrontlineRole::Rifle,1);HostileRecruited=Hostile!=INDEX_NONE;}
            if(ContactReadyForReviewV87&&Hostile!=INDEX_NONE&&At(Hostile,TEXT("enemy_assembly")))WarfareFRRUWorldV82::MoveSquad(NBattle,Hostile,View.TargetNode);
            if(Scenario->Probe==EProbe::Hotchkiss||Scenario->Probe==EProbe::FT)
            {
                if(!ObserverRecruited&&NBattle->RecruitBlockReason(EFrontlineRole::Marksman,0).IsEmpty())
                {Observer=Recruit(EFrontlineRole::Marksman,0);ObserverRecruited=Observer!=INDEX_NONE;}
                if(Observer!=INDEX_NONE&&At(Observer,TEXT("home")))WarfareFRRUWorldV82::MoveSquad(NBattle,Observer,TEXT("relay"));
                const bool CanReplanFT=Scenario->Probe==EProbe::FT&&View.FirePhase==4
                    &&WarfareFRRUWorldV82::CommandBlockReason(NBattle,EFRRUTacticalCommandV82::PlanFire,{},View.TargetNode).IsEmpty();
                if(At(Observer,TEXT("relay"))&&(View.FirePhase==0||CanReplanFT))
                    Event(TEXT("real_fire_plan"),WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::PlanFire,{},View.TargetNode));
                if(View.FirePhase==1||View.FirePhase==2)WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::Observe,{Observer});
                if(View.FirePhase==3)Event(TEXT("real_commit_with_host_rounds"),WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::CommitFire));
                if(FourSalvos&&!Followup&&ResolvedFourAt>=0&&NBattle->Elapsed<ResolvedFourAt+8)
                    Followup=WarfareFRRUWorldV82::Command(NBattle,EFRRUTacticalCommandV82::Followup,{Scenario->Probe==EProbe::FT?ObjectiveSquad:Lead});
                if(Scenario->Probe==EProbe::Hotchkiss&&GunReloadDone&&!GunPack&&At(Second,View.TargetNode))WarfareFRRUWorldV82::MoveSquad(NBattle,Second,TEXT("assembly"));
                if(Scenario->Probe==EProbe::Hotchkiss&&GunCarry&&GunDeploy&&GunSupported&&GunPack&&GunShots&&GunReload&&GunReloadDone&&FourSalvos&&Followup)
                {Check(TEXT("real_hotchkiss_carry_deploy_fire_reload_pack_and_four_salvo_followup"),true);Finish();return !Finished;}
            }
            int32 LeadShots=0;if(const auto* Group=NBattle->FindSquad(Lead))for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get())LeadShots+=Unit->ShotsDischarged;
            if(Scenario->Probe==EProbe::Rifle&&LeadShots>0&&NBattle->Elapsed>EntryOrderedAt+35)
            {Check(TEXT("selected_real_rifle_squad_fired"),true,FString::FromInt(LeadShots)+TEXT(" own rounds, excludes all other squads and enemy fire; per-discharge target/LOS instrumentation remains separate."));Finish();return !Finished;}
            if(Scenario->Probe==EProbe::Reserve&&FirstReserveEarlyChecked&&ReserveReleased&&SecondReserveEarlyChecked&&SecondReserveReleased&&ReserveAxesChecked
                &&ReserveIdentityChecked&&ClearanceCompleted&&MaximReloadChecked&&M95ReloadChecked)
            {Check(TEXT("reserve_all_real_two_echelon_clearance_and_weapon_windows_complete"),true);Finish();return !Finished;}
            if(GermanCoreOnly()&&GermanA7VCore&&GermanBeuteCore)
            {Check(TEXT("german_contact_and_live_weapons_core_complete"),true,TEXT("Natural original A7V and Beute contact, travel and both weapon classes only; final objectives, art and Shipping remain separate."));Finish();return !Finished;}
            if(IsVehicleCase()&&VehicleRolled&&VehicleShot&&VehiclePoseChecked&&VehicleTrackChecked&&VehicleGunChecked
                &&(Scenario->Probe!=EProbe::Train||NaturalSixPassengerChecked&&WreckRetryChecked)
                &&(Scenario->Probe!=EProbe::FT||RealStraddler&&RealHullContest&&FinalContestCleared))
            {Check(TEXT("vehicle_all_applicable_actual_windows_complete"),true);Finish();return !Finished;}
            return true;
        }
        return true;
    }
};
FAutoConsoleCommand Command(TEXT("WF.FRRU.ReviewNaturalV82"),TEXT("Bounded isolated controller/loading/recruitment and natural battle review."),FConsoleCommandDelegate::CreateLambda([]()
{
    FString User,Case;if(!Isolation(User)||!FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalCase="),Case))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_NATURAL_REFUSED: exact isolated flags, fresh UserDir and named case required"));return;}
    const FScenario* Selected=nullptr;for(const auto& Candidate:Scenarios)if(Case==Candidate.Name){Selected=&Candidate;break;}
    if(!Selected){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_NATURAL_REFUSED: unknown case"));return;}
    const bool FastPlay=FParse::Param(FCommandLine::Get(),TEXT("FRRUNaturalFastPlay"));
    const bool HoldDepartureProbe=FParse::Param(FCommandLine::Get(),TEXT("FRRUHoldDepartureProbeV87"));
    if(HoldDepartureProbe)
    {
        FGuid Run;const FString RunDir=FPaths::GetPath(User),RunName=FPaths::GetCleanFilename(RunDir);
        FString Seed,ExpectedUser=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),TEXT("Saved/FRRU/V82NaturalReview/vaux-full"),RunName,TEXT("User")));
        FPaths::NormalizeDirectoryName(ExpectedUser);
        if(Case!=TEXT("vaux-full")||FastPlay||FParse::Param(FCommandLine::Get(),TEXT("FRRUVauxTactics"))
            ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ReturnAfterNaturalWin"))
            ||FrontlineBlockedFireProbeV87::Requested()||FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalEnemySeedGuid="),Seed)
            ||!FGuid::ParseExact(RunName,EGuidFormats::Digits,Run)||!Run.IsValid()
            ||!User.Equals(ExpectedUser,ESearchCase::IgnoreCase))
        {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_HOLD_DEPARTURE_PROBE_REFUSED: isolated GUID Vaux ordinary NoSave combat only; no tactics, replay, fast-play, visual or return modes"));return;}
    }
    FString VauxChoice;
    if(FParse::Param(FCommandLine::Get(),TEXT("FRRUVauxTactics"))&&(Case!=TEXT("vaux-full")||Selected->Probe!=EProbe::Ordinary||!Selected->WholeOperation
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))||FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate"))
        ||FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate"))||FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82FTContactOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview"))
        ||FrontlineBlockedFireProbeV87::Requested()||!FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalChoice="),VauxChoice)||VauxChoice!=TEXT("alternative")))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_VAUX_TACTICS_REFUSED: explicit isolated vaux-full normal combat with alternative story required"));return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavFollowCandidate"))&&(Case!=TEXT("matz-ft")||Selected->Probe!=EProbe::FT||FastPlay
        ||FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate"))||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82FTContactOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview"))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_FT_NAV_FOLLOW_REFUSED: exact isolated normal Matz FT candidate required"));return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate"))&&(Case!=TEXT("matz-ft")||Selected->Probe!=EProbe::FT
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))||FastPlay))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_FT_NAV_REFUSED: exact isolated Matz visual-only route query required"));return;}
    FString MechanicalSeed;
    if(FParse::Param(FCommandLine::Get(),TEXT("FRRUVBMechanicsFixture"))&&(Case!=TEXT("chemin-vb")||Selected->Probe!=EProbe::VB||FastPlay
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))||FrontlineBlockedFireProbeV87::Requested()
        ||FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalEnemySeedGuid="),MechanicalSeed)))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_VB_MECHANICS_REFUSED: explicit chemin-vb NoSave mechanics only; incompatible with visual/probe/fast-play/enemy-seed modes"));return;}
    if(FastPlay&&(!Selected->WholeOperation||Selected->Probe!=EProbe::Ordinary
        ||!FString(Selected->Mission).StartsWith(TEXT("fr."))||!Case.EndsWith(TEXT("-full"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82FTContactOnly"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanContactCoreOnly"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV826FormationJoinOnly"))||FrontlineBlockedFireProbeV87::Requested()))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_NATURAL_FAST_PLAY_REFUSED: French ordinary whole-operation *-full combat only; no capture-only or specialist windows"));return;}
    if(FrontlineBlockedFireProbeV87::Requested()&&!FrontlineBlockedFireProbeV87::Isolation())
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_BLOCKED_FIRE_REFUSED: explicit NoSave Mort case and exact disposable GUID/User are required"));return;}
    const FString Repository=FPaths::Combine(User,TEXT("FRRUReview/campaigns-tactical-v2.json"));
    if(IFileManager::Get().FileExists(*Repository)||IFileManager::Get().FileExists(*(Repository+TEXT(".bak"))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_NATURAL_REFUSED: national repository is not fresh"));return;}
    static bool Requested=false;if(Requested)return;Requested=true;
    auto Probe=MakeShared<FScheduler>();Probe->Scenario=Selected;Probe->User=User;Probe->NaturalFastPlayRequestedV87=FastPlay;Probe->HoldDepartureProbeV87=HoldDepartureProbe;
    FString Difficulty=TEXT("Veteran");FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalDifficulty="),Difficulty);
    if(FParse::Param(FCommandLine::Get(),TEXT("FRRUVauxTactics"))&&!Difficulty.Equals(TEXT("Veteran"),ESearchCase::IgnoreCase))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_VAUX_TACTICS_REFUSED: unchanged Veteran difficulty required"));return;}
    if(!Difficulty.Equals(TEXT("Rookie"),ESearchCase::IgnoreCase)&&!Difficulty.Equals(TEXT("Veteran"),ESearchCase::IgnoreCase)
        &&!Difficulty.Equals(TEXT("Survivor"),ESearchCase::IgnoreCase))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_NATURAL_REFUSED: difficulty must be Rookie, Veteran or Survivor"));return;}
    Probe->RequestedDifficultyV87=Difficulty.Equals(TEXT("Rookie"),ESearchCase::IgnoreCase)?EFrontlineDifficulty::Rookie:
        Difficulty.Equals(TEXT("Survivor"),ESearchCase::IgnoreCase)?EFrontlineDifficulty::Survivor:EFrontlineDifficulty::Veteran;
    Probe->RequestedDifficultyLabelV87=Probe->RequestedDifficultyV87==EFrontlineDifficulty::Rookie?TEXT("Rookie"):
        Probe->RequestedDifficultyV87==EFrontlineDifficulty::Survivor?TEXT("Survivor"):TEXT("Veteran");
    double Deadline=720;
    if(FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalDeadline="),Deadline))
    {
        if(!FMath::IsFinite(Deadline)||Deadline<60||Deadline>820)
        {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_NATURAL_REFUSED: invalid bounded deadline"));return;}
        Probe->DeadlineSeconds=Deadline;
    }
    FString DeadlineUtc;
    if(FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalDeadlineUtc="),DeadlineUtc)&&!FDateTime::ParseIso8601(*DeadlineUtc,Probe->WallDeadlineUtc))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_NATURAL_REFUSED: invalid wall deadline UTC"));return;}
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float Delta)
    {
        if(!Probe->Controller.IsValid())
        {
            if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World();World&&World->WorldType==EWorldType::Game)
                if(auto* Controller=Cast<AWarfareFrontlineController>(World->GetFirstPlayerController());Controller&&Controller->HasActorBegunPlay())
                    if(auto* Battle=FrontlineFor(Controller);Battle&&Battle->HasActorBegunPlay()&&Battle->IsProgressionVolatile()&&Battle->Phase==EFrontlinePhase::Briefing)Probe->Controller=Controller;
            if(!Probe->Controller.IsValid())
            {if(FPlatformTime::Seconds()-Probe->Started>45)return Probe->Fail(TEXT("actual_beginplay_initial_context"),TEXT("No begun-play volatile controller in 45 wall seconds."));return true;}
        }
        return Probe->Tick(Delta);
    }));
}));
}
#endif
