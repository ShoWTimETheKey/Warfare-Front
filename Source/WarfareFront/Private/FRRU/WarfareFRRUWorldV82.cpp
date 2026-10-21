#include "WarfareFRRUWorldV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUTextV82.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineCombatRecordV65.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "FRRUHostContract.h"
#include "WarfareFRRUCampaignInternalV82.h"
#include "Native/UE/ExpansionArchiveUE.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "WarfareFrontlineGround.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "WarfareFRRUContentV82.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFrontlineVehiclesV28.h"
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <queue>

// Narrow encounter outcome access. A native finished result and its transactional
// campaign submission still pass ResolveOutcome before the host publishes it.
struct FFRRUWorldOutcomeAccessV82
{
    static void Finish(AWarfareFrontlineGameMode* B,bool Success)
    {if(B)B->FinishBattle(Success?0:1,Success?TEXT("The assigned position has been secured."):TEXT("The operation could not be completed."));}
};
namespace WarfareFRRUWorldV82
{
namespace
{
// Switch individual consumers only when their real host hook is connected.
// The locked V81 native pilot's Enabled flag is never a production switch.
constexpr bool ActualHostHooksConnected=true;
bool CandidateReview()
{
#if WITH_EDITOR
    // A real campaign outcome may be reviewed only in the same disposable
    // repository boundary already used by campaign and account persistence.
    // Invalid manual-review arguments never fall through to the no-save route.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview")))
    {FString PersistenceUser;return WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(PersistenceUser);}
#endif
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    FString User;const bool Flags=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!FPaths::IsRelative(User);
    FPaths::NormalizeDirectoryName(User);
    return Flags&&Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)&&User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase);
}
bool ReadNaturalEnemySeedGuidV87(const AWarfareFrontlineGameMode* B,FGuid& SeedGuid,FString& Error)
{
    const TCHAR* Arguments=FCommandLine::Get();const FString Key=TEXT("-FRRUNaturalEnemySeedGuid");FString Token,Value;int32 Requests=0;
    while(FParse::Token(Arguments,Token,false))
    {
        if(Token.Equals(Key,ESearchCase::IgnoreCase)){++Requests;Value.Empty();}
        else if(Token.StartsWith(Key+TEXT("="),ESearchCase::IgnoreCase)){++Requests;Value=Token.Mid(Key.Len()+1).TrimQuotes();}
    }
    if(Requests==0)return true; // The ordinary encounter-seeded path is unchanged.
    const auto Reject=[&](const TCHAR* Reason){Error=Reason;return false;};
#if WITH_EDITOR
    if(Requests!=1||Value.Len()!=32||!FGuid::ParseExact(Value,EGuidFormats::Digits,SeedGuid)||!SeedGuid.IsValid())
        return Reject(TEXT("Enemy-sequence replay requires one nonzero 32-hex FRRUNaturalEnemySeedGuid."));
    FString Case,User;
    if(!B||!B->IsProgressionVolatile()||FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))||!CandidateReview()
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))||FParse::Param(FCommandLine::Get(),TEXT("FRRUBlockedFireProbe"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV826FormationJoinOnly"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalCase="),Case)||Case!=TEXT("chemin-vb")
        ||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.chemin_dames")
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))
        return Reject(TEXT("Enemy-sequence replay is restricted to the volatile NoSave chemin-vb Editor role review."));
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);FPaths::NormalizeDirectoryName(User);
    const FString Prefix=Project+TEXT("/Saved/FRRU/V82NaturalReview/chemin-vb/");FGuid UserGuid;
    if(!User.StartsWith(Prefix,ESearchCase::IgnoreCase))return Reject(TEXT("Enemy-sequence replay UserDir is outside the exact chemin-vb review boundary."));
    const FString Suffix=User.Mid(Prefix.Len());
    if(Suffix.Len()!=37||!Suffix.Mid(32).Equals(TEXT("/User"),ESearchCase::IgnoreCase)
        ||!FGuid::ParseExact(Suffix.Left(32),EGuidFormats::Digits,UserGuid)||!UserGuid.IsValid())
        return Reject(TEXT("Enemy-sequence replay UserDir must be the existing project chemin-vb/<nonzero-32hex-GUID>/User boundary."));
    return true;
#else
    (void)B;(void)SeedGuid;
    return Reject(TEXT("Enemy-sequence replay is unavailable outside Editor builds."));
#endif
}
struct FRoute
{
    FString Current,Target;
    std::vector<std::string> Edges;
    int32 Step=0;
    std::map<int32,int32> MemberWaypoints;
    bool CarrierDelivery=false;
    FVector CarrierDeliveryCorner=FVector::ZeroVector;
    int32 EntryLane=INDEX_NONE;
    uint64 ArrivalSequence=0;
    int32 TransitCheckpointStep=INDEX_NONE;
    FVector TransitCheckpointOffset=FVector::ZeroVector;
    float TransitCorridorRetryAt=0;bool TransitCorridorWaiting=false;
    float TransitRetryAt=0;
    FString LastTransitRejection;
    FVector TransitRejectedFrom=FVector::ZeroVector,TransitRejectedTo=FVector::ZeroVector;
};
using FFormationCells=std::map<std::string,std::array<std::map<int,int>,2>>;
bool MayReserveHomeIngressV87(const AWarfareFrontlineGameMode* B,int32 Squad,const FFormationCells& Cells);
struct FWorld
{
    FGuid Encounter;
    bool SceneStaged=false,Started=false,Finished=false,PendingEnd=false;
    bool LocalResult=false,LocalSuccess=false,BreakthroughVictory=false;
    bool HonourResultPending=false;int32 HonourWinner=INDEX_NONE;
    struct FStoryPause
    {
        FGuid Encounter;FString StoryId;
        bool Active=false,RestorePaused=false;
    } StoryPause;
#if WITH_EDITOR
    float TrainDeliveryDiagnosticAt=-1;
    struct FActualStepV87
    {
        float At=0;FVector From,To,Guide;double TravelCm=0;uint64 Steps=0;
    };
    std::map<int,FActualStepV87> ActualStepsV87;
    struct FActualBendBypassV87
    {
        int32 EdgeStep=INDEX_NONE,Checkpoint=INDEX_NONE,Last=INDEX_NONE,AdvancedTo=INDEX_NONE;
        FString Node;float SelectedAt=0,AdvancedAt=-1,ReachedAt=-1;
        FVector Bend=FVector::ZeroVector,Chosen=FVector::ZeroVector,AdvancedFeet=FVector::ZeroVector,ReachedFeet=FVector::ZeroVector;
    };
    std::map<int,FActualBendBypassV87> ActualBendBypassesV87;
    struct FStoryPauseProbe
    {
        FGuid Encounter;FString StoryId;double Started=0;
        float Elapsed=0;uint64 HostFrame=0;int32 UnitCount=0,SquadCount=0,StrikeCount=0;
        std::string NativeSignature;
        wfexp::tactical::Snapshot Snapshot;
        TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FVector> Feet;
    } StoryPauseProbe;
#endif
    uint64 HostFrame=0;
    FString Mission,Scene,Notice;
    wfexp::hostv81::Identity Identity;
    wfexp::tactical::State State;
    struct FOrderPreview
    {
        double At=-1;
        wfexp::tactical::State State;
        wfexp::tactical::Snapshot Snapshot;
        std::map<std::string,FString> Reasons;
    } OrderPreview;
    wfexp::hostv81::HostFrameBridge Clock;
    wfexp::hostv81::SupportJournal Journal;
    std::array<std::string,2> ActivePlans;
    std::map<int,std::string> FirePlans;
    std::map<int,FRoute> Routes;
    struct FLocalApproach
    {
        enum class EPathKindV87:uint8 { None,TemporaryPortal,CompleteBodyGraph,RegisteredNativeRejoin };
        FString Node;int32 Side=INDEX_NONE,Cell=INDEX_NONE,Index=0;
        FVector Goal=FVector::ZeroVector;
        TArray<FVector> Points;
        EPathKindV87 PathKindV87=EPathKindV87::None;
        float ProgressAt=0,BestRemaining=MAX_flt,RetryAt=0,RevalidateAt=0,StalledMovementAttemptsV87=0;
        float CanonicalBestRemainingV87=MAX_flt,CanonicalNoProgressAttemptsV87=0;
        int32 TransitBody=INDEX_NONE;float TransitPassingSide=0;
        bool FinalJoinBodyPortal=false,BodyGraphRequiredV87=false;
        bool BendEndpointBoundV87=false;
        FVector BendEndpointV87=FVector::ZeroVector;
        float BendFinalLegValidatedAtV87=-1,BendArrivalRadiusV87=10,BendStopDistanceV87=8;
        uint64 BendFinalLegValidatedFrameV87=0;
        FString LastRejection;
        FVector RejectedFrom=FVector::ZeroVector,RejectedTo=FVector::ZeroVector;
        void ClearPathV87(){Points.Reset();Index=0;PathKindV87=EPathKindV87::None;}
    };
    std::map<int,FLocalApproach> Approaches;
    FFormationCells Cells;
    struct FPostAdmissionV827 {int32 Owner=INDEX_NONE,InheritedFromV87=INDEX_NONE;bool Transit=false;std::vector<int32> Waiting;};
    std::map<std::string,std::array<FPostAdmissionV827,2>> PostAdmissionsV827;
    std::map<std::string,int> Owners;
    std::set<std::string> Cleared;
    std::map<int,wfexp::tactical::MovementReceipt> Arrivals;
    struct FCoverProbe {FVector Feet,Bearing;float HeadHeight=0,At=-1,Value=0;};
    std::map<int,FCoverProbe> CoverProbes;
    std::array<std::vector<int>,2> Observers;
    double ObserveAt=0;
    double EnemyDecisionAt=3,EnemySupportAt=22;
    double EnemyRecruitAt=0,EnemyAssaultAt=9;
    int32 EnemyRecruitStep=0,EnemyWaveRecruitCount=0;
    int32 EnemyRosterIndexV827=0;
    double OrdinaryAdvanceAtV827=0;
    bool OrdinaryOrderV827=false;
    TSet<int32> OrdinarySectionsV827,HeldOrdinarySectionsV827;
    TMap<int32,double> OrdinaryReadyAtV827;
    EFrontlineSupport EnemyPendingSupport=EFrontlineSupport::Count;
    FVector EnemySupportTarget=FVector::ZeroVector;
    double EnemySupportExecuteAt=0;
    TWeakObjectPtr<AWarfareFrontlineSoldier> EnemySupportContact;
    FRandomStream EnemyRandom;
    int32 EnemyOpeningIndex=0,EnemyRecruitAttempts=0;
    EFrontlineRole EnemyRecruitRole=EFrontlineRole::Count;
    bool EnemyRecruitFromOpening=false;
    TMap<int32,FString> EnemyRecruitFailures;
    FString EnemyRecruitBlock;
    struct FLegacySurface {TWeakObjectPtr<UPrimitiveComponent> Component;bool Visible=false;ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;};
    TArray<FLegacySurface> HiddenLegacy;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TSharedPtr<FWorld>> Worlds;
bool MandatoryIngressSectionV87(const FWorld& R,const wfexp::tactical::StageDefinition& Stage,const FFrontlineSquad& Group);
bool ReservedByTacticsV827(const AWarfareFrontlineGameMode* B,const FWorld& R,int32 Id);
FString OrderNotice(const std::string& Reason)
{
    // Transaction diagnostics belong in the log, not in the commander's report.
    const FString Detail=wfexpue::Wide(Reason);
    UE_LOG(LogTemp,Verbose,TEXT("FRRU order refused: %s"),*Detail);
    const bool English=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English;
    if(Detail.Contains(TEXT("observer"))||Detail.Contains(TEXT("observation"))||Detail.Contains(TEXT("observed")))
        return English?TEXT("The observers have lost sight of the target. Move them into view."):TEXT("观察哨看不清目标。请将观察组移至能看见目标的位置。");
    if(Detail.Contains(TEXT("reload")))
        return English?TEXT("The gun crews are reloading."):TEXT("炮手正在装填。");
    if(Detail.Contains(TEXT("squad slot"))||Detail.Contains(TEXT("capacity")))
        return English?TEXT("There is no room for another section at that position."):TEXT("这处阵地已容不下另一支分队。");
    if(Detail.Contains(TEXT("route"))||Detail.Contains(TEXT("source/target")))
        return English?TEXT("The route is blocked. Choose another approach."):TEXT("前路受阻。请另选进攻路线。");
    if(Detail.Contains(TEXT("Support unavailable"))||Detail.Contains(TEXT("round budget")))
        return English?TEXT("The batteries are not ready to fire."):TEXT("炮兵尚未准备好开火。");
    if(Detail.Contains(TEXT("Current hold")))
        return English?TEXT("Hold this position until the order is fulfilled."):TEXT("请守住这处阵地，完成当前命令。");
    if(Detail.Contains(TEXT("Reserve"))||Detail.Contains(TEXT("reserve"))||Detail.Contains(TEXT("echelon")))
        return English?TEXT("The reserves must assemble before they can advance."):TEXT("预备队须先完成集结，才能投入战斗。");
    if(Detail.Contains(TEXT("not purchased")))
        return English?TEXT("This command has not been issued to the company."):TEXT("连队尚未掌握这项战术。");
    return English?TEXT("The section cannot carry out that order from its present position."):TEXT("分队无法在当前位置执行这道命令。");
}
TWeakObjectPtr<AWarfareFrontlineGameMode> PhysicalOwner;
FWorld& WorldFor(const AWarfareFrontlineGameMode* B)
{
    const TWeakObjectPtr<AWarfareFrontlineGameMode> Key(const_cast<AWarfareFrontlineGameMode*>(B));
    auto* Found=Worlds.Find(Key);if(!Found){for(auto It=Worlds.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();Found=&Worlds.Add(Key,MakeShared<FWorld>());}
    return **Found;
}
const wfexp::scene::SceneDefinition* Geometry(const AWarfareFrontlineGameMode* B)
{return B&&WarfareFRRUCampaignV82::Active(B)?wfexp::scene::ForMission(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(B))):nullptr;}
const wfexp::tactical::MissionDefinition* Definition(const AWarfareFrontlineGameMode* B)
{return B?wfexp::tactical::FindMission(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(B))):nullptr;}
const wfexp::tactical::NodeDefinition* Node(const wfexp::tactical::MissionDefinition& M,const std::string& Id)
{for(const auto& N:M.nodes)if(N.id==Id)return &N;return nullptr;}
const wfexp::tactical::EdgeDefinition* Edge(const wfexp::tactical::MissionDefinition& M,const std::string& Id)
{for(const auto& E:M.edges)if(E.id==Id)return &E;return nullptr;}
FVector Vector(const wfexp::scene::Point& P){return FVector(P.x,P.y,P.z);}
wfexp::scene::Point Point(const FVector& P){return {P.X,P.Y,P.Z};}
wfexp::Result Reject(const char* Why){return {false,Why};}
bool HooksConnected(){return ActualHostHooksConnected||CandidateReview();}
bool ReadyForWorldTick(const AWarfareFrontlineGameMode* B)
{
    if(!B||!HooksConnected()||!WarfareFRRUCampaignV82::Active(B))return false;
    const auto& R=WorldFor(B);const auto* G=Geometry(B);
    // Reuse only the current frame's full scene inspection. Public Ready and
    // explicit probes remain strict; a new frame or scene rechecks the field.
    return G&&R.SceneStaged&&R.Mission==WarfareFRRUCampaignV82::MissionId(B)&&R.Encounter==B->GetEncounterId()
        &&WarfareFRRUContentV82::SceneStagedForWater(B,R.Scene,G->revision);
}
bool EnemyRearHasRoomV827(AWarfareFrontlineGameMode* B,const FWorld& R);
int32 EnemyEchelonSizeV827(const FWorld& R);
FVector2D EnemyEchelonRestV827(const FWorld& R);
void TickEnemyRecruitment(AWarfareFrontlineGameMode* B,FWorld& R)
{
    if(B->Elapsed<R.EnemyRecruitAt||!EnemyRearHasRoomV827(B,R))return;
    auto Block=[&](const FString& Reason)
    {
        const FString Key=FString::Printf(TEXT("%d|%s"),int32(R.EnemyRecruitRole),*Reason);
        if(R.EnemyRecruitBlock!=Key)
        {
            R.EnemyRecruitBlock=Key;
            UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_V82_ENEMY_RECRUIT_BLOCKED mission=%s t=%.3f role=%d reason=%s"),
                *R.Mission,B->Elapsed,int32(R.EnemyRecruitRole),*Reason);
        }
    };
    const auto* Operation=B->GetEncounterCampaignMission();
    if(!Operation){Block(TEXT("bound national mission descriptor is missing"));return;}
    // A global host failure cannot be bypassed by substituting another role.
    if(!B->AssetFailure.IsEmpty()){Block(TEXT("host asset failure: ")+B->AssetFailure);return;}
    auto ClearCommit=[&]()
    {R.EnemyRecruitRole=EFrontlineRole::Count;R.EnemyRecruitFromOpening=false;R.EnemyRecruitAttempts=0;};
    auto RejectRole=[&](const FString& Reason)
    {
        Block(Reason);R.EnemyRecruitFailures.Add(int32(R.EnemyRecruitRole),Reason);
        if(R.EnemyRecruitFromOpening)++R.EnemyOpeningIndex;
        ClearCommit();
    };
    if(R.EnemyRecruitRole==EFrontlineRole::Count)
    {
        while(Operation->EnemyOpeningRoles.IsValidIndex(R.EnemyOpeningIndex))
        {
            const auto Role=Operation->EnemyOpeningRoles[R.EnemyOpeningIndex];
            FString Reason;
            if(uint8(Role)>=uint8(EFrontlineRole::Count))Reason=TEXT("invalid authored opening role");
            else if(!WarfareFRRUEquipmentV82::RoleAvailable(B,Role,1))Reason=TEXT("authored opening role unavailable for nation/date");
            else if(const auto* Failed=R.EnemyRecruitFailures.Find(int32(Role)))Reason=*Failed;
            if(Reason.IsEmpty()){R.EnemyRecruitRole=Role;R.EnemyRecruitFromOpening=true;break;}
            UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_V82_ENEMY_OPENING_SKIPPED mission=%s t=%.3f index=%d role=%d reason=%s"),
                *R.Mission,B->Elapsed,R.EnemyOpeningIndex,int32(Role),*Reason);
            ++R.EnemyOpeningIndex;
        }
        if(R.EnemyRecruitRole==EFrontlineRole::Count)
        {
            // The opening has a separate cursor. Thereafter each operation's
            // authored order supplies its composition, including repeated rifle
            // or gun entries. A blocked shared cycle retains that exact role.
            // Replays open the date-qualified national roster for both armies.
            // Keep the authored opening, then use the existing legal weighted
            // reserve selection rather than repeating this early mission's
            // restricted ordinary-campaign order forever.
            const int32 Count=WarfareFRRUCampaignV82::ArchiveFullArmoryV87(B,1)?0:Operation->EnemyRoleOrder.Num();
            for(int32 Offset=0;Offset<Count;++Offset)
            {
                const auto Role=Operation->EnemyRoleOrder[(R.EnemyRosterIndexV827+Offset)%Count];
                if(uint8(Role)>=uint8(EFrontlineRole::Count)
                    ||!WarfareFRRUEquipmentV82::RoleAvailable(B,Role,1)
                    ||R.EnemyRecruitFailures.Contains(int32(Role)))continue;
                R.EnemyRecruitRole=Role;R.EnemyRosterIndexV827=(R.EnemyRosterIndexV827+Offset+1)%Count;break;
            }
        }
        if(R.EnemyRecruitRole==EFrontlineRole::Count)
        {
            TArray<EFrontlineRole> Available;
            constexpr float Weights[11]={1.0f,1.1f,.75f,.7f,.45f,.4f,.4f,.5f,.3f,.7f,.35f};float Total=0;
            // Eligibility is nation/date, not current readiness. Preserve the
            // chosen specialist while its ordinary shared cycle completes.
            for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
                if(WarfareFRRUEquipmentV82::RoleAvailable(B,EFrontlineRole(I),1)&&!R.EnemyRecruitFailures.Contains(I))
                {Available.Add(EFrontlineRole(I));Total+=Weights[I];}
            if(Available.IsEmpty()){Block(TEXT("no legal national enemy role remains after recorded failures"));return;}
            R.EnemyRecruitRole=Available.Last();
            float Roll=R.EnemyRandom.FRandRange(0.f,Total);
            for(auto Role:Available){Roll-=Weights[int32(Role)];if(Roll<=0){R.EnemyRecruitRole=Role;break;}}
        }
        R.EnemyRecruitBlock.Reset();R.EnemyRecruitAttempts=0;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_ENEMY_RECRUIT_COMMITTED mission=%s t=%.3f role=%d opening=%d cooldown=%.3f"),
            *R.Mission,B->Elapsed,int32(R.EnemyRecruitRole),int32(R.EnemyRecruitFromOpening),B->RecruitCooldown(R.EnemyRecruitRole,1));
    }
    const auto Role=R.EnemyRecruitRole;
    if(!WarfareFRRUEquipmentV82::RoleAvailable(B,Role,1))
    {RejectRole(TEXT("committed role no longer available for nation/date"));return;}
    FString Error;
    if(!WarfareFRRUPresentationV82::RoleAssetsReady(B,Role,1,Error))
    {RejectRole(TEXT("national role asset preparation failed: ")+Error);return;}
    const float Cooldown=B->RecruitCooldown(Role,1);
    if(!FMath::IsFinite(Cooldown)||Cooldown==MAX_flt)
    {RejectRole(TEXT("host returned invalid recruit cooldown"));return;}
    if(Cooldown>.01f)return;
    const FString Reason=B->RecruitBlockReason(Role,1);
    if(!Reason.IsEmpty()){RejectRole(TEXT("ready committed role refused by host policy: ")+Reason);return;}
    if(B->Recruit(Role,1))
    {
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_ENEMY_RECRUITED mission=%s t=%.3f role=%d opening=%d"),
            *R.Mission,B->Elapsed,int32(Role),int32(R.EnemyRecruitFromOpening));
        if(R.EnemyRecruitFromOpening)++R.EnemyOpeningIndex;
        ClearCommit();R.EnemyRecruitBlock.Reset();
        const auto& Doctrine=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
        ++R.EnemyRecruitStep;
        R.EnemyRecruitAt=B->Elapsed+FrontlineGameplayV43::PreparationSeconds(
            R.EnemyRandom.FRandRange(1.6f,3.2f)*Doctrine.DecisionScale*FrontlinePacingV24::TempoIntervalScale,1,B->GetCampaignDifficulty());
        if(++R.EnemyWaveRecruitCount>=EnemyEchelonSizeV827(R))
        {
            R.EnemyWaveRecruitCount=0;
            const bool Opening=FrontlineGameplayV33::IsOpeningAssembly(Operation->Id,R.EnemyRecruitStep,
                B->LivingSoldiers(1),B->Elapsed,!FrontlineDifficultyV30::State(B).Contacts.IsEmpty());
            const auto Rest=EnemyEchelonRestV827(R);
            const float NextWave=B->Elapsed+FrontlineGameplayV43::PreparationSeconds(
                R.EnemyRandom.FRandRange(Opening?6.f:float(Rest.X),Opening?9.f:float(Rest.Y))*Doctrine.WaveRestScale*FrontlinePacingV24::TempoIntervalScale,1,B->GetCampaignDifficulty());
            R.EnemyRecruitAt=FMath::Max(R.EnemyRecruitAt,double(NextWave));
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_ENEMY_ECHELON mission=%s t=%.3f next_at=%.3f recruited=%d"),
                *R.Mission,B->Elapsed,NextWave,R.EnemyRecruitStep);
        }
        return;
    }
    // SpawnSquad owns its rollback. Do not claim a more specific failure than
    // the bool-only Recruit API exposes, or loop forever on this same role.
    ++R.EnemyRecruitAttempts;
    const FString Failure=FString::Printf(TEXT("host Recruit returned false after ready/asset preflight (attempt %d/2)"),R.EnemyRecruitAttempts);
    if(R.EnemyRecruitAttempts>=2)RejectRole(Failure);else Block(Failure);
}
bool RegisteredWorker(const FWorld& R,const wfexp::tactical::MissionDefinition& M,const FFrontlineSquad& Group,const std::string& Target)
{
    const auto& Gate=M.stages[R.State.objective.index];if(Group.Team!=R.State.playerSide||Group.Role!=EFrontlineRole::Pioneer||Gate.kind!=wfexp::tactical::GateKind::ClearPassage)return false;
    const bool RegisteredFallback=std::find(Gate.nodes.begin(),Gate.nodes.end(),Gate.fallbackNode)!=Gate.nodes.end()||std::find(Gate.alternateNodes.begin(),Gate.alternateNodes.end(),Gate.fallbackNode)!=Gate.alternateNodes.end();
    const bool Frozen=std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Group.Id)!=R.State.objective.fallbackWorkers.end();
    return Target==R.State.objective.boundObstacle&&(!R.State.objective.fallback||!RegisteredFallback||Frozen)
        ||R.State.objective.fallback&&RegisteredFallback&&Target==Gate.fallbackNode&&Frozen;
}
bool FrozenHold(const FWorld& R,const wfexp::tactical::MissionDefinition& M,const FFrontlineSquad& Group,const std::string& Target)
{
    const auto& Gate=M.stages[R.State.objective.index];
    return Group.Team==R.State.playerSide&&!R.State.objective.fallback&&R.State.withdrawPolicyActive&&R.State.withdrawPolicy=="hold_current_until_gate_complete"
        &&Gate.kind==wfexp::tactical::GateKind::HoldNode&&std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Group.Id)!=R.State.objective.holdParticipants.end()
        &&Target!=R.State.objective.boundObstacle;
}
bool SameDeployment(const AWarfareFrontlineGameMode* B,const FWorld& R)
{return B&&R.Started&&R.Encounter==B->GetEncounterId()&&R.State.encounterId==wfexpue::Narrow(R.Encounter.ToString(EGuidFormats::DigitsWithHyphensLower));}
bool InTransit(const FRoute& Route)
{return !Route.Target.IsEmpty()&&(Route.Step<int32(Route.Edges.size())||Route.Current!=Route.Target);}
bool IsIntermediateTransitLeg(const FRoute& Route)
{return Route.Step>=0&&Route.Step+1<int32(Route.Edges.size());}
void RetireStalePlans(FWorld& R)
{
    for(int32 Side=0;Side<2;++Side)if(!R.ActivePlans[Side].empty()&&R.State.fire[Side].planId!=R.ActivePlans[Side])
    {
        const std::string Retired=R.ActivePlans[Side];R.Journal.RetirePlan(Retired,R.State.encounterId);R.ActivePlans[Side].clear();
        for(auto It=R.FirePlans.begin();It!=R.FirePlans.end();)if(It->second==Retired)It=R.FirePlans.erase(It);else ++It;
    }
}
bool AtNode(const FFrontlineSquad& S,const wfexp::tactical::NodeDefinition& N,bool NeedStationary)
{
    bool Any=false;
    for(const auto& W:S.Members)if(const auto* U=W.Get();U&&U->IsAlive())
    {Any=true;if(!U->bFieldEntered||FVector::Dist2D(U->Feet(),Vector(N.position))>N.radius||(NeedStationary&&(U->bMoving||U->GetMotionSpeed()>.5f)))return false;}
    return Any;
}
const wfexp::tactical::NodeDefinition* OccupiedNode(const FFrontlineSquad& S,const wfexp::tactical::MissionDefinition& M)
{
    const wfexp::tactical::NodeDefinition* Found=nullptr;double Distance=1.e30;
    for(const auto& N:M.nodes)if(AtNode(S,N,false))
    {const double D=FVector::DistSquared2D(S.Centre(),Vector(N.position));if(D<Distance){Found=&N;Distance=D;}}
    return Found;
}
std::string NextNode(const FRoute& Route,const wfexp::tactical::MissionDefinition& M)
{
    if(Route.Step>=int32(Route.Edges.size()))return wfexpue::Narrow(Route.Target);
    const auto* Link=Edge(M,Route.Edges[Route.Step]);if(!Link)return {};
    const auto Current=wfexpue::Narrow(Route.Current);
    if(Link->from==Current)return Link->to;
    return Link->bidirectional&&Link->to==Current?Link->from:std::string();
}
bool RouteDepartureAllowed(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,const FWorld& R,const FFrontlineSquad& Group,const FRoute& Route)
{
    if(Route.Step<0||Route.Step>=int32(Route.Edges.size()))return false;
    const auto* Link=Edge(M,Route.Edges[Route.Step]);const auto* From=Node(M,wfexpue::Narrow(Route.Current));
    const std::string Next=NextNode(Route,M);const auto* To=Node(M,Next);const auto* G=Geometry(B);
    const auto Owner=R.Owners.find(Next);const bool Work=Next==wfexpue::Narrow(Route.Target)&&RegisteredWorker(R,M,Group,Next);
    return G&&Link&&From&&To&&(Link->from==From->id||(Link->bidirectional&&Link->to==From->id))
        &&!(Group.Team==R.State.playerSide&&Link->approach=="enemy")
        &&(Link->clearanceGate.empty()||R.Cleared.count(Link->clearanceGate)||(Work&&Link->clearanceGate==Next))
        &&(Owner==R.Owners.end()||Owner->second!=1-Group.Team||Next==wfexpue::Narrow(Route.Target))
        &&wfexp::scene::CanTraverseRoute(*G,Link->id,From->position,To->position);
}
bool TravelCheckpoint(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,const FRoute& Route,int Member,int Index,FVector& Out,int& Last)
{
    if(Route.Step<0||Route.Step>=int32(Route.Edges.size()))return false;
    const auto* G=Geometry(B);const auto* From=Node(M,wfexpue::Narrow(Route.Current));const auto* To=Node(M,NextNode(Route,M));
    if(!G||!From||!To)return false;const auto* Physical=wfexp::scene::FindRoute(*G,Route.Edges[Route.Step]);
    // Only scenes without authored polylines retain their existing, verified
    // straight route. A missing binding in a curved scene is not a shortcut.
    if(!Physical&&!G->routes.empty())return false;
    const int Count=Physical?int(Physical->points.size()):2;if(Count<2||Count>128)return false;
    Last=Count-1;Index=FMath::Clamp(Index,1,Last);const bool Reverse=Physical&&wfexpue::Narrow(Route.Current)!=Physical->from;
    const auto At=[&](int P){return Physical?Vector(Physical->points[Reverse?Count-1-P:P]):Vector(P==0?From->position:To->position);};
    const FVector Forward=(At(Index)-At(Index-1)).GetSafeNormal2D();if(Forward.IsNearlyZero())return false;
    const FVector Right(-Forward.Y,Forward.X,0);
    Out=At(Index)+Right*((Member%3-1)*68.)-Forward*((Member/3)*75.);
    if(Index==Last&&Route.TransitCheckpointStep==Route.Step)Out+=Route.TransitCheckpointOffset;
    float Floor=0;if(!TryHeight(B,Out.X,Out.Y,Floor))return false;Out.Z=Floor;return true;
}
FVector FormationPoint(const wfexp::tactical::NodeDefinition& N,int Side,int Cell,int Member)
{
    // An independent physical bay is reserved for every squad. Opposing
    // squads occupy distinct sides of a post rather than sharing destinations.
    // Three columns and two rows leave a real outside approach to every
    // member's post. A middle man must not be enclosed by an arrived third row.
    const double CellY=N.capacity>1?(Cell-(N.capacity-1)*.5)*180.:0.;
    const double SideX=N.capacity>1?(Side==0?-130.:130.):0.;
    const double SideY=N.capacity==1?(Side==0?-85.:85.):0.;
    return Vector(N.position)+FVector(SideX+(Member%3-1)*72.,CellY+SideY+(Member/3?36.:-36.),0);
}
// A planted soldier may fire before his last centimetres of formation steering.
// Arrival and join precedence share a tolerance smaller than the body radius;
// precise gun support and actual node/route/contact checks remain independent.
constexpr double FormationArrivalToleranceCm=20.;
bool FormationSettled(const FFrontlineSquad& Group,const wfexp::tactical::NodeDefinition& N,int Cell)
{
    bool Any=false;
    for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
    {
        Any=true;
        if(!Member->bFieldEntered||Member->bMoving||Member->GetMotionSpeed()>.5f
            ||FVector::Dist2D(Member->Feet(),FormationPoint(N,Group.Team,Cell,Member->MemberIndex))>FormationArrivalToleranceCm)return false;
    }
    return Any;
}
bool CellFits(const AWarfareFrontlineGameMode* B,const wfexp::tactical::NodeDefinition& N,int Side,int Cell,int Squad=INDEX_NONE)
{
    if(Side<0||Side>1||Cell<0||Cell>=N.capacity||N.capacity>3)return false;
    const auto* G=Geometry(B);const auto* Group=B?B->FindSquad(Squad):nullptr;
    if(!G||(Squad!=INDEX_NONE&&(!Group||Group->Team!=Side||Group->LivingCount()<=0)))return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82FormationCell),true);for(const auto& U:B->Units)if(U)Query.AddIgnoredActor(U);
    // A bay belongs to the actual surviving section. Gun crews and depleted
    // sections must not be rejected by terrain under nonexistent ranks.
    // Every occupied slot retains its native footing and capsule proof.
    TSet<int32> Ranks;
    if(Group)for(const auto& Weak:Group->Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())Ranks.Add(Member->MemberIndex);
    // The explicit geometry review has no actors and still probes the entire
    // six-man bay. Runtime reservations always supply their actual section.
    if(Squad==INDEX_NONE)for(int32 Member=0;Member<6;++Member)Ranks.Add(Member);
    for(const int32 Member:Ranks)
    {
        if(Member<0||Member>=6)return false;
        FVector Feet=FormationPoint(N,Side,Cell,Member);const auto Footing=wfexp::scene::FootingAt(*G,Feet.X,Feet.Y);
        if(!Footing.valid||!Footing.walkable||FVector::Dist2D(Feet,Vector(N.position))>N.radius||!wfexp::scene::CanTraverseSegment(*G,N.position,Point(Feet)))return false;
        FVector Centre,ContactFeet;if(!GroundContact(B,Feet,ContactFeet,Centre))return false;
        if(B->GetWorld()->OverlapBlockingTestByChannel(Centre,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),Query))return false;
    }
    return true;
}
bool CellLeavesTransitSpaceV827(const AWarfareFrontlineGameMode* B,const wfexp::tactical::NodeDefinition& N,int Side,int Cell,int Squad)
{
    const auto* Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B)));
    const auto* M=Definition(B);if(!Found||!M||!SameDeployment(B,**Found))return true;
    const auto* Joining=B->FindSquad(Squad);if(!Joining||Joining->Team!=Side||Joining->LivingCount()<=0)return false;
    for(const auto& Pair:(**Found).Routes)
    {
        const auto* Group=B->FindSquad(Pair.first);const auto& Path=Pair.second;
        if(!Group||Group->Id==Squad||Group->Team!=Side||Group->LivingCount()<=0||!Group->bAdvancing||Group->bHoldPosition
            ||!IsIntermediateTransitLeg(Path)||NextNode(Path,*M)!=N.id||Path.TransitCheckpointStep!=Path.Step)continue;
        // A frozen guide is only a future-space promise while this section
        // still owns the actual admission and may traverse the current leg.
        // Real bodies remain subject to the unchanged movement/capsule gates.
        const auto Admission=(**Found).PostAdmissionsV827.find(N.id);
        if(Admission==(**Found).PostAdmissionsV827.end()||Admission->second[Side].Owner!=Group->Id
            ||!RouteDepartureAllowed(B,*M,**Found,*Group,Path))continue;
        for(const auto& Weak:Group->Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
        {
            FVector Passing;int Last=0;if(!TravelCheckpoint(B,*M,Path,Member->MemberIndex,128,Passing,Last))return false;
            for(const auto& JoiningWeak:Joining->Members)if(const auto* Rank=JoiningWeak.Get();Rank&&Rank->IsAlive())
                if(FVector::Dist2D(Passing,FormationPoint(N,Side,Cell,Rank->MemberIndex))<56.5+FormationArrivalToleranceCm)return false;
        }
    }
    return true;
}
// Allocation is independent of its physical witness. Production supplies its
// actual living section and live transit check; the Editor-only geometric
// review supplies an explicit complete six-man bay without inventing soldiers.
bool AllocateFormationBayV87(const wfexp::tactical::NodeDefinition& N,int Side,int Owner,
    FFormationCells& Cells,TFunctionRef<bool(int32)> PhysicalWitness)
{
    if(Side<0||Side>1)return false;auto& Bays=Cells[N.id][Side];if(Bays.count(Owner))return true;
    for(int Cell=0;Cell<N.capacity;++Cell)
    {
        bool Used=false;for(const auto& Bay:Bays)if(Bay.second==Cell){Used=true;break;}
        if(!Used&&PhysicalWitness(Cell)){Bays.emplace(Owner,Cell);return true;}
    }
    return false;
}
bool ReserveCell(const AWarfareFrontlineGameMode* B,const wfexp::tactical::NodeDefinition& N,int Side,int Squad,FFormationCells& Cells)
{
    if(Side<0||Side>1)return false;auto& Bays=Cells[N.id][Side];if(Bays.count(Squad))return true;
    if(N.id=="home"&&Side==0&&!MayReserveHomeIngressV87(B,Squad,Cells))return false;
    return AllocateFormationBayV87(N,Side,Squad,Cells,[&](int32 Cell)
        {return CellFits(B,N,Side,Cell,Squad)&&CellLeavesTransitSpaceV827(B,N,Side,Cell,Squad);});
}
void ReleaseVacatedCells(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,FWorld& R)
{
    for(auto& NodeCells:R.Cells)for(int Side=0;Side<2;++Side)for(auto It=NodeCells.second[Side].begin();It!=NodeCells.second[Side].end();)
    {
        const auto* Group=B->FindSquad(It->first);const auto Path=R.Routes.find(It->first);const auto* N=Node(M,NodeCells.first);
        bool Keep=Group&&Group->LivingCount()>0&&N;
        if(Keep)
        {
            Keep=Path!=R.Routes.end()&&!IsIntermediateTransitLeg(Path->second)&&NextNode(Path->second,M)==N->id;
            // The old bay remains occupied until every living member has
            // physically left; giving it away at order time causes collisions.
            for(const auto& W:Group->Members)if(const auto* U=W.Get();U&&U->IsAlive()&&FVector::Dist2D(U->Feet(),Vector(N->position))<=N->radius+56)Keep=true;
        }
        if(!Keep)It=NodeCells.second[Side].erase(It);else ++It;
    }
    for(auto It=R.Routes.begin();It!=R.Routes.end();)if(const auto* Group=B->FindSquad(It->first);!Group||Group->LivingCount()<=0)It=R.Routes.erase(It);else ++It;
    for(auto It=R.CoverProbes.begin();It!=R.CoverProbes.end();)
    {const auto* Group=B->FindSquad(It->first/16);if(!Group||Group->LivingCount()<=0)It=R.CoverProbes.erase(It);else ++It;}
    for(auto It=R.Approaches.begin();It!=R.Approaches.end();)
    {
        const auto* Group=B->FindSquad(It->first/16);bool Present=false;
        if(Group)for(const auto& Weak:Group->Members)if(const auto* Member=Weak.Get();Member&&Member->MemberIndex==It->first%16&&Member->IsAlive()){Present=true;break;}
        if(!Present)It=R.Approaches.erase(It);else ++It;
    }

}
// A shared route checkpoint is a physical receiving corridor, not a bay.
// Only one section joins it at a time; following sections retain their actual
// feet and routes. Final joins use the same queue but keep their separate bays.
static bool BodyInPostV827(const FFrontlineSquad& Group,const wfexp::tactical::NodeDefinition& Post);
static bool HostileIntermediateReceiverBlockedV87(const FWorld& R,const wfexp::tactical::MissionDefinition& M,
    const FFrontlineSquad& Group,const std::string& Post)
{
    const auto Path=R.Routes.find(Group.Id);const auto* Receiver=Node(M,Post);const auto Owner=R.Owners.find(Post);
    // A route waiting for another section to secure an intermediate post
    // must not reserve the very receiving lane needed by that final assault.
    // A frozen plan is not physical occupancy. Actual feet already inside
    // the receiver retain their token until the existing departure clears it.
    return Receiver&&Path!=R.Routes.end()&&InTransit(Path->second)&&IsIntermediateTransitLeg(Path->second)
        &&NextNode(Path->second,M)==Post&&Owner!=R.Owners.end()&&Owner->second==1-Group.Team
        &&!BodyInPostV827(Group,*Receiver);
}
static bool AdmissionRequestV827(const FWorld& R,const wfexp::tactical::MissionDefinition& M,
    const FFrontlineSquad& Group,const std::string& Post)
{
    const auto Path=R.Routes.find(Group.Id);
    return Group.LivingCount()>0&&Group.bAdvancing&&!Group.bHoldPosition&&Path!=R.Routes.end()
        &&InTransit(Path->second)&&NextNode(Path->second,M)==Post
        &&!HostileIntermediateReceiverBlockedV87(R,M,Group,Post)
        &&(IsIntermediateTransitLeg(Path->second)||(R.Cells.count(Post)&&R.Cells.at(Post)[Group.Team].count(Group.Id)));
}
static bool BodyInPostV827(const FFrontlineSquad& Group,const wfexp::tactical::NodeDefinition& Post)
{
    for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive()
        &&Member->bFieldEntered&&FVector::Dist2D(Member->Feet(),Vector(Post.position))<=Post.radius+56.5)return true;
    return false;
}
// A paused section already executing its admitted corridor still physically
// owns that service. Retain the exact frozen legal leg if its live feet occupy
// the receiver or have completely cleared the original source footprint.
// Unentered bookings, missing nodes and changed/invalid routes cannot retain it.
static bool OccupiedFrozenTransitAdmissionV87(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    const FWorld& R,const FFrontlineSquad& Group,const std::string& Post,const FWorld::FPostAdmissionV827& Admission)
{
    const auto Path=R.Routes.find(Group.Id);const auto* Receiver=Node(M,Post);
    if(Admission.Owner!=Group.Id||!Admission.Transit||Group.LivingCount()<=0||!Group.bAdvancing
        ||!Receiver||Path==R.Routes.end())return false;
    const auto* Source=Node(M,wfexpue::Narrow(Path->second.Current));
    if(!Source||!InTransit(Path->second)||!IsIntermediateTransitLeg(Path->second)
        ||NextNode(Path->second,M)!=Post||Path->second.TransitCheckpointStep!=Path->second.Step
        ||!RouteDepartureAllowed(B,M,R,Group,Path->second))return false;
    if(BodyInPostV827(Group,*Receiver))return true;
    // Use the shared actual vacated-post predicate, never a larger receiving
    // radius. Off-field/pending members are not proof that a source was left.
    if(BodyInPostV827(Group,*Source))return false;
    for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
        if(!Member->bFieldEntered)return false;
    return true;
}
static void HoldGroupAdmissionsV87(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    FWorld& R,const FFrontlineSquad& Group)
{
    for(auto& Entry:R.PostAdmissionsV827)for(int32 Side=0;Side<2;++Side)
    {
        auto& Admission=Entry.second[Side];
        if(Side==Group.Team&&OccupiedFrozenTransitAdmissionV87(B,M,R,Group,Entry.first,Admission))
        {
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_HOLD_KEEP_OCCUPIED_ADMISSION mission=%s post=%s side=%d squad=%d step=%d host=%.3f arrival_granted=false"),
                *R.Mission,*wfexpue::Wide(Entry.first),Side,Group.Id,R.Routes.at(Group.Id).Step,B->Elapsed);
            continue; // Preserve the existing owner, FIFO position and corridor.
        }
        if(Admission.Owner==Group.Id){Admission.Owner=INDEX_NONE;Admission.InheritedFromV87=INDEX_NONE;}
        Admission.Waiting.erase(std::remove(Admission.Waiting.begin(),Admission.Waiting.end(),Group.Id),Admission.Waiting.end());
    }
}
static void ReleaseGroupAdmissionsV827(FWorld& R,int32 Id)
{
    for(auto& Entry:R.PostAdmissionsV827)for(auto& Admission:Entry.second)
    {
        if(Admission.Owner==Id){Admission.Owner=INDEX_NONE;Admission.InheritedFromV87=INDEX_NONE;}
        Admission.Waiting.erase(std::remove(Admission.Waiting.begin(),Admission.Waiting.end(),Id),Admission.Waiting.end());
    }
}
static bool FreezeTransitCorridorV827(AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    const FWorld& R,const FFrontlineSquad& Group,FRoute& Route,const std::string& Next,std::vector<int32>* NeededFinalJoinsV87=nullptr)
{
    if(NeededFinalJoinsV87)NeededFinalJoinsV87->clear();
    if(!IsIntermediateTransitLeg(Route)){Route.TransitCorridorWaiting=false;return true;}
    if(Route.TransitCheckpointStep==Route.Step){Route.TransitCorridorWaiting=false;return true;}
    if(B->Elapsed<Route.TransitCorridorRetryAt)return false;
    Route.TransitCorridorWaiting=true;Route.TransitCorridorRetryAt=B->Elapsed+.35f;
    const auto* Post=Node(M,Next);const auto* Ground=Geometry(B);if(!Post||!Ground)return false;
    FVector Left,RightTarget;int Last=0;
    if(!TravelCheckpoint(B,M,Route,0,128,Left,Last)||!TravelCheckpoint(B,M,Route,2,128,RightTarget,Last))return false;
    const FVector Across=(RightTarget-Left).GetSafeNormal2D();if(Across.IsNearlyZero())return false;
    const FVector Forward(Across.Y,-Across.X,0);
    struct FSpaceBody {FVector Position;double Clearance;int32 FutureFinalOwner=INDEX_NONE;};TArray<FSpaceBody> Bodies;
    // A reserved formation is a physical promise. Plan around both its real
    // settled bodies and its fixed slots, so a later friendly join cannot
    // occupy this leg's already chosen passing corridor.
    if(const auto Cells=R.Cells.find(Next);Cells!=R.Cells.end())for(const auto& Bay:Cells->second[Group.Team])
    {
        const auto* Occupant=B->FindSquad(Bay.first);if(!Occupant||Occupant->Id==Group.Id||Occupant->LivingCount()<=0)continue;
        const auto Pending=R.Routes.find(Occupant->Id);
        const bool FutureFinal=Pending!=R.Routes.end()&&!IsIntermediateTransitLeg(Pending->second)
            &&AdmissionRequestV827(R,M,*Occupant,Next)&&!BodyInPostV827(*Occupant,*Post);
        for(const auto& Weak:Occupant->Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
        {
            Bodies.Add({FormationPoint(*Post,Group.Team,Bay.second,Member->MemberIndex),56.5+FormationArrivalToleranceCm,FutureFinal?Occupant->Id:INDEX_NONE});
            if(Member->bFieldEntered)Bodies.Add({Member->Feet(),56.5});
        }
    }
    for(const AWarfareFrontlineSoldier* Other:B->Units)if(IsValid(Other)&&Other->SquadId!=Group.Id&&Other->IsAlive()
        &&Other->bFieldEntered&&!WarfareFRRUArmourV82::Embarked(Other)
        &&FVector::Dist2D(Other->Feet(),Vector(Post->position))<=Post->radius+300.)Bodies.Add({Other->Feet(),56.5});
    const auto SegmentGap=[](const FVector& P,const FVector& A,const FVector& Z)
    {
        FVector Delta=Z-A;Delta.Z=0;const double Length=Delta.SizeSquared();
        const double T=Length>.0001?FMath::Clamp(FVector::DotProduct((P-A)*FVector(1,1,0),Delta)/Length,0.,1.):0.;
        return FVector::Dist2D(P,A+Delta*T);
    };
    // Five fixed corridors within the same receiving node, never a new route
    // around wire or an enemy position. One is frozen for this route leg.
    for(const FVector Offset:{FVector::ZeroVector,Across*180.,-Across*180.,Forward*180.,-Forward*180.})
    {
        FRoute Candidate=Route;Candidate.TransitCheckpointStep=Candidate.Step;Candidate.TransitCheckpointOffset=Offset;
        bool Valid=true;std::set<int32> FutureBlockers;
        for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
        {
            FVector Goal,Previous;int Count=0;
            if(!TravelCheckpoint(B,M,Candidate,Member->MemberIndex,128,Goal,Count)
                ||!TravelCheckpoint(B,M,Candidate,Member->MemberIndex,FMath::Max(1,Count-1),Previous,Count)
                ||FVector::Dist2D(Goal,Vector(Post->position))+56.5>Post->radius){Valid=false;break;}
            // A two-point edge starts at its actual source formation. Longer
            // authored polylines keep the preceding exact per-rank bend.
            if(Count==1)Previous=Member->Feet();
            const auto Footing=wfexp::scene::FootingAt(*Ground,Goal.X,Goal.Y);
            FVector Feet,Capsule;
            if(!Footing.valid||!Footing.walkable||!GroundContact(B,Goal,Feet,Capsule)
                ||!CanTraverse(B,Previous,Goal)||!AllowsStep(Member,Previous,Goal)){Valid=false;break;}
            for(const auto& Body:Bodies)if(SegmentGap(Body.Position,Previous,Goal)<Body.Clearance)
            {
                if(Body.FutureFinalOwner==INDEX_NONE){Valid=false;break;}
                FutureBlockers.insert(Body.FutureFinalOwner);
            }
            if(!Valid)break;
        }
        if(!Valid)continue;
        if(!FutureBlockers.empty())
        {
            // This candidate remains blocked: a reservation is never waived.
            // Record only the final bookings whose future footprint prevents
            // an otherwise physically valid corridor, so their real joins can
            // inherit service before this transit instead of waiting on it.
            if(NeededFinalJoinsV87)for(const int32 Id:FutureBlockers)
                if(std::find(NeededFinalJoinsV87->begin(),NeededFinalJoinsV87->end(),Id)==NeededFinalJoinsV87->end())NeededFinalJoinsV87->push_back(Id);
            continue;
        }
        Route.TransitCheckpointStep=Route.Step;Route.TransitCheckpointOffset=Offset;
        Route.TransitCorridorWaiting=false;Route.TransitCorridorRetryAt=0;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V827_TRANSIT_CORRIDOR mission=%s squad=%d post=%s step=%d offset=%s t=%.3f"),
            *R.Mission,Group.Id,*wfexpue::Wide(Next),Route.Step,*Offset.ToString(),B->Elapsed);
        return true;
    }
    return false; // An occupied or unwalkable local corridor is a real wait.
}
static void PrunePostAdmissionsV827(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,FWorld& R)
{
    for(auto& Entry:R.PostAdmissionsV827)for(int32 Side=0;Side<2;++Side)
    {
        auto& Admission=Entry.second[Side];const auto* Post=Node(M,Entry.first);
        Admission.Waiting.erase(std::remove_if(Admission.Waiting.begin(),Admission.Waiting.end(),[&](int32 Id)
        {const auto* Group=B->FindSquad(Id);return !Post||!Group||Group->Team!=Side
            ||(!AdmissionRequestV827(R,M,*Group,Entry.first)
                &&!(Group->bHoldPosition&&OccupiedFrozenTransitAdmissionV87(B,M,R,*Group,Entry.first,Admission)));}),Admission.Waiting.end());
        if(Admission.Owner==INDEX_NONE)continue;
        const auto* Group=B->FindSquad(Admission.Owner);const auto Path=R.Routes.find(Admission.Owner);
        bool Keep=Post&&Group&&Group->Team==Side&&Group->LivingCount()>0;
        if(Keep&&Admission.InheritedFromV87!=INDEX_NONE&&!BodyInPostV827(*Group,*Post))
        {
            const auto* Source=B->FindSquad(Admission.InheritedFromV87);
            if(!Source||!AdmissionRequestV827(R,M,*Source,Entry.first))Keep=false;
        }
        if(Keep&&Admission.Transit)
            Keep=(Group->bHoldPosition&&OccupiedFrozenTransitAdmissionV87(B,M,R,*Group,Entry.first,Admission))
                ||(!Group->bHoldPosition&&Group->bAdvancing&&Path!=R.Routes.end()
                    &&(AdmissionRequestV827(R,M,*Group,Entry.first)
                        ||(InTransit(Path->second)&&BodyInPostV827(*Group,*Post))));
        else if(Keep)
        {
            const auto Cells=R.Cells.find(Entry.first);bool Settled=false;
            if(Cells!=R.Cells.end())if(const auto Bay=Cells->second[Side].find(Group->Id);Bay!=Cells->second[Side].end())Settled=FormationSettled(*Group,*Post,Bay->second);
            // A completed final join releases the corridor, never its occupied
            // formation cell. An explicit hold/aborted route cannot own a queue.
            Keep=!Settled&&AdmissionRequestV827(R,M,*Group,Entry.first);
        }
        if(!Keep){Admission.Owner=INDEX_NONE;Admission.InheritedFromV87=INDEX_NONE;}
    }
}
static bool LeavingOwnedPostV827(const AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    const FWorld& R,const FFrontlineSquad& Group,const std::string& Next)
{
    for(const auto& Entry:R.PostAdmissionsV827)if(Entry.first!=Next)
    {const auto& Admission=Entry.second[Group.Team];const auto* Post=Node(M,Entry.first);
        if(Admission.Owner==Group.Id&&Admission.Transit&&Post&&BodyInPostV827(Group,*Post))return true;}
    return false;
}
static bool AdjacentPostHandoffV827(const wfexp::tactical::MissionDefinition& M,const FWorld& R,
    const FFrontlineSquad& Group,const FRoute& Route,const std::string& Next)
{
    const auto* Receiver=Node(M,Next);if(!Receiver||!BodyInPostV827(Group,*Receiver))return false;
    // The leading feet may already lie inside both adjacent receiving circles.
    // Acquire this exact free next corridor while the tail still owns the old
    // footprint. Never preclaim a third post or a remote future destination.
    const wfexp::tactical::NodeDefinition* Previous=nullptr;
    for(const auto& Entry:R.PostAdmissionsV827)if(Entry.first!=Next)
    {
        const auto& Admission=Entry.second[Group.Team];const auto* Post=Node(M,Entry.first);
        if(Admission.Owner!=Group.Id||!Admission.Transit||!Post||!BodyInPostV827(Group,*Post))continue;
        if(Previous||Entry.first!=wfexpue::Narrow(Route.Current))return false;
        Previous=Post;
    }
    return Previous!=nullptr&&NextNode(Route,M)==Next;
}
static bool AdmitPostV827(AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    FWorld& R,const FFrontlineSquad& Group,FRoute& Route,const std::string& Next)
{
    auto& Admission=R.PostAdmissionsV827[Next][Group.Team];
    if(HostileIntermediateReceiverBlockedV87(R,M,Group,Next))
    {
        Admission.Waiting.erase(std::remove(Admission.Waiting.begin(),Admission.Waiting.end(),Group.Id),Admission.Waiting.end());
        if(Admission.Owner==Group.Id){Admission.Owner=INDEX_NONE;Admission.InheritedFromV87=INDEX_NONE;}
        // Keep the requested route, bay reservations and measured checkpoints.
        // Once this post is secured, ordinary admission can resume that route.
        Route.TransitCorridorWaiting=true;return false;
    }
    if(std::find(Admission.Waiting.begin(),Admission.Waiting.end(),Group.Id)==Admission.Waiting.end())Admission.Waiting.push_back(Group.Id);
    if(Admission.Owner==Group.Id)return FreezeTransitCorridorV827(B,M,R,Group,Route,Next);
    if(Admission.Owner!=INDEX_NONE||Admission.Waiting.empty()||Admission.Waiting.front()!=Group.Id)return false;
    // A distant receiver is not reserved until the old footprint clears. An
    // overlapping adjacent receiver can hand over at its actual boundary;
    // the old token remains until the last living feet have physically left.
    if(LeavingOwnedPostV827(B,M,R,Group,Next)&&!AdjacentPostHandoffV827(M,R,Group,Route,Next))return false;
    std::vector<int32> NeededFinalJoins;
    if(!FreezeTransitCorridorV827(B,M,R,Group,Route,Next,&NeededFinalJoins))
    {
        // A transit at the head can wait on a later final booking's promised
        // footprint, while that booking waits on this same FIFO. Inherit only
        // along that measured dependency. Leave the queue order unchanged;
        // after the exact booking joins, ordinary FIFO resumes automatically.
        const auto Cells=R.Cells.find(Next);const auto* Receiver=Node(M,Next);
        if(IsIntermediateTransitLeg(Route)&&Receiver&&Cells!=R.Cells.end()&&!NeededFinalJoins.empty())
            for(size_t Index=1;Index<Admission.Waiting.size();++Index)
            {
                const int32 Id=Admission.Waiting[Index];auto* Joiner=B->FindSquad(Id);const auto Joining=R.Routes.find(Id);
                if(std::find(NeededFinalJoins.begin(),NeededFinalJoins.end(),Id)==NeededFinalJoins.end()||!Joiner||Joining==R.Routes.end()
                    ||!Cells->second[Group.Team].count(Id)||IsIntermediateTransitLeg(Joining->second)
                    ||!AdmissionRequestV827(R,M,*Joiner,Next)||BodyInPostV827(*Joiner,*Receiver))continue;
                if(LeavingOwnedPostV827(B,M,R,*Joiner,Next)&&!AdjacentPostHandoffV827(M,R,*Joiner,Joining->second,Next))continue;
                if(!FreezeTransitCorridorV827(B,M,R,*Joiner,Joining->second,Next))continue;
                Admission.Owner=Id;Admission.Transit=false;Admission.InheritedFromV87=Group.Id;
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ADMISSION_INHERIT mission=%s post=%s side=%d from_transit=%d booked_final=%d host=%.3f"),
                    *R.Mission,*wfexpue::Wide(Next),Group.Team,Group.Id,Id,B->Elapsed);
                break; // One transit-to-final dependency; never recurse through another queue.
            }
        return false;
    }
    Admission.Owner=Group.Id;Admission.Transit=IsIntermediateTransitLeg(Route);Admission.InheritedFromV87=INDEX_NONE;return true;
}
wfexp::tactical::Snapshot Capture(AWarfareFrontlineGameMode* B,FWorld& R,bool PruneDeadArrivals=true)
{
    wfexp::tactical::Snapshot S;const auto* M=Definition(B);
    S.missionId=R.State.missionId;S.encounterId=R.State.encounterId;S.sceneId=R.Scene.IsEmpty()?std::string():wfexpue::Narrow(R.Scene);
    S.sceneRevision=M?M->sceneRevision:0;S.stateRevision=R.State.revision;
    S.elapsed=B->Elapsed;S.fighting=B->Phase==EFrontlinePhase::Fighting;S.paused=UGameplayStatics::IsGamePaused(B);
    S.geometryReady=R.SceneStaged;S.hostOrdersAtomic=HooksConnected();
    S.nations=R.Identity.nations;S.directions=R.Identity.directions;
    const auto* Physical=Geometry(B);if(!M||!Physical)return S;
    for(int32 Side=0;Side<2;++Side)for(int32 Slot=0;Slot<10;++Slot)
    {
        const auto Support=EFrontlineSupport(Slot);int32 Resolved=-1;
        if(WarfareFRRUEquipmentV82::ResolveSupport(Support,Resolved)&&Resolved==Slot&&WarfareFRRUEquipmentV82::SupportAvailable(B,Support,Side))
        {
            const float Ready=B->SupportCooldown(Support,Side);S.supportReadyAt[Side][Slot]=FMath::IsFinite(Ready)?B->Elapsed+Ready:86400.;
            if(Slot==0||Slot==3){S.supportRoundBudget[Side][Slot]=FrontlineFireSupportV26::Salvos*FrontlineFireSupportV26::NormalRoundsPerSalvo;S.supportSalvoCount[Side][Slot]=4;}
            else if(Slot==1){S.supportRoundBudget[Side][Slot]=FrontlineFireSupportV26::SmokeRoundsV37;S.supportSalvoCount[Side][Slot]=2;}
            else if(Slot==8){S.supportRoundBudget[Side][Slot]=6;S.supportSalvoCount[Side][Slot]=2;}
        }
    }
    S.squads.reserve(B->Squads.Num());
    for(const auto& Group:B->Squads)
    {
        if(Group.LivingCount()<=0)continue;
        const AWarfareFrontlineSoldier* Anchor=nullptr;bool Entered=true,Stationary=true,Reloading=false;
        for(const auto& W:Group.Members)if(const auto* U=W.Get();U&&U->IsAlive())
        {if(!Anchor||U->MemberIndex<Anchor->MemberIndex)Anchor=U;Entered&=U->bFieldEntered;Stationary&=!U->bMoving&&U->GetMotionSpeed()<=.5f;Reloading|=U->WeaponPhase==EFrontlineWeaponPhase::Reload;}
        // Offstage reinforcements have no native field presence until their
        // actual members enter; no camera observation is used as evidence.
        if(!Anchor||!Entered)continue;
        const auto Foot=Point(Anchor->Feet());if(!wfexp::scene::Contains(*Physical,Foot))continue;
        wfexp::tactical::SquadSnapshot Q;Q.id=Group.Id;Q.side=Group.Team;Q.roleId=int32(Group.Role);Q.living=Group.LivingCount();
        Q.entered=true;Q.stationary=Stationary;Q.advancing=Group.bAdvancing;Q.reloading=Reloading;Q.holding=Group.bHoldPosition;Q.position=Foot;
        const auto Path=R.Routes.find(Group.Id);
        if(Path==R.Routes.end()||!InTransit(Path->second))if(const auto* N=OccupiedNode(Group,*M))
        {
            const auto Bays=R.Cells.find(N->id);
            if(Bays!=R.Cells.end())if(const auto Bay=Bays->second[Group.Team].find(Group.Id);Bay!=Bays->second[Group.Team].end()&&FormationSettled(Group,*N,Bay->second))Q.nodeId=N->id;
        }
        S.squads.push_back(std::move(Q));
    }
    for(const auto& N:M->nodes)
    {
        // Clearance belongs to a registered edge. A gated front approach must
        // not close a different, ungated approach into the same physical post.
        // Native Route and each host departure still check that exact edge.
        auto I=R.Owners.find(N.id);wfexp::tactical::NodeSnapshot Post{N.id,I==R.Owners.end()?-1:I->second,false};
        // An entire real footprint contests a position. A squad anchor outside
        // the circle cannot conceal another member or an operational hull.
        for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team>=0&&U->Team<2&&U->IsActiveCombatant()
            &&FVector::DistSquared2D(U->Feet(),Vector(N.position))<=N.radius*N.radius)Post.contestedSidesMask|=std::uint8_t(1u<<U->Team);
        if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* V:Fleet->GetVehicles())
            if(V&&V->Team>=0&&V->Team<2&&WarfareFRRUArmourV82::OccupiesNode(V,Vector(N.position),float(N.radius)))Post.contestedSidesMask|=std::uint8_t(1u<<V->Team);
        S.nodes.push_back(std::move(Post));
    }
    // A receipt describes an existing physical squad in this snapshot. A
    // destroyed squad cannot leave a proof that rejects every later frame.
    std::set<int> Present;for(const auto& Q:S.squads)Present.insert(Q.id);
    for(auto It=R.Arrivals.begin();It!=R.Arrivals.end();)
    {
        const auto* Group=B->FindSquad(It->first);
        if(!Group||Group->LivingCount()<=0){if(PruneDeadArrivals)It=R.Arrivals.erase(It);else ++It;continue;}
        if(Present.count(It->first))S.movements.push_back(It->second);++It;
    }
    for(int32 Side=0;Side<2;++Side)if(!R.ActivePlans[Side].empty())
    {std::vector<wfexp::tactical::SupportReceipt> Proofs;if(R.Journal.ActiveReceipts(R.ActivePlans[Side],S.encounterId,Proofs))S.supportReceipts.insert(S.supportReceipts.end(),Proofs.begin(),Proofs.end());}
    FCollisionQueryParams Trace(SCENE_QUERY_STAT(FRRUV82Observation),true);for(const auto& U:B->Units)if(U)Trace.AddIgnoredActor(U);
    for(const auto& Q:S.squads)if(Q.stationary&&!Q.advancing&&(Q.roleId==3||Q.roleId==4||Q.roleId==10))for(const auto& N:M->nodes)
    {
        const FVector Eye=Vector(Q.position)+FVector(0,0,135),Target=Vector(N.position)+FVector(0,0,135);
        const bool Native=wfexp::scene::LineOfSight(*Geometry(B),Point(Eye),Point(Target));
        if(!Native||B->GetWorld()->LineTraceTestByChannel(Eye,Target,ECC_Visibility,Trace))continue;
        double Transmission=1.;
        const FVector Delta=Target-Eye;const double Den=Delta.SizeSquared2D();
        for(const auto& Smoke:B->SmokeZones)if(B->Elapsed>=Smoke.StartedAt&&B->Elapsed<Smoke.ExpiresAt&&Smoke.Radius>0)
        {const double T=Den>0?FMath::Clamp(FVector::DotProduct(Smoke.Centre-Eye,Delta)/Den,0.,1.):0.;if(FVector::Dist2D(Eye+Delta*T,Smoke.Centre)<Smoke.Radius)Transmission*=.2;}
        S.observations.push_back({Q.id,N.id,S.elapsed,Transmission,Point(Eye),true});
    }
    return S;
}
wfexp::tactical::Command MakeOrder(const wfexp::tactical::State& State,EFRRUTacticalCommandV82 Kind,const TArray<int32>& SquadIds,const FString& TargetNode,const FString& OptionOrStoryId,const FString& ChoiceId,int32 Variant,int32 Side,const FString& AlternateNode)
{
    wfexp::tactical::Command Order;Order.kind=wfexp::tactical::CommandKind(uint8(Kind)+1);
    Order.side=Side;Order.missionId=State.missionId;Order.encounterId=State.encounterId;Order.expectedRevision=State.revision;Order.sequence=State.nextSequence[Side];
    Order.receiptId="v82.order."+std::to_string(Side)+"."+std::to_string(Order.sequence);Order.targetNode=wfexpue::Narrow(TargetNode);Order.variant=Variant;
    Order.alternateNode=wfexpue::Narrow(AlternateNode);
    for(const int32 Id:SquadIds)Order.squadIds.push_back(Id);
    if(Kind==EFRRUTacticalCommandV82::ChooseStory){Order.storyId=wfexpue::Narrow(OptionOrStoryId);Order.choiceId=wfexpue::Narrow(ChoiceId);}
    if(Kind==EFRRUTacticalCommandV82::UseOption)Order.optionId=wfexpue::Narrow(OptionOrStoryId);
    if(Kind==EFRRUTacticalCommandV82::PlanFire)Order.alternateNode=wfexpue::Narrow(OptionOrStoryId);
    return Order;
}
struct FReservedSupport
{
    wfexp::hostv81::HostPlan Plan;
    wfexp::tactical::Intent Intent;
    EFrontlineSupport Kind=EFrontlineSupport::Artillery;
    bool HasJournal=false;
    TArray<FFrontlineStrike> Strikes;
    TOptional<FFrontlineSmokeZone> Smoke;
};
class FBatch final:public wfexp::tactical::AtomicHostBatch
{
    AWarfareFrontlineGameMode* B;FWorld& R;
    std::vector<FReservedSupport> Support;
    std::map<int,FRoute> Routes;
    std::map<int,std::string> AutomaticPostHandoversV87;
    std::map<int,FRoute> SourceHandoverRoutesV87,HandoverRoutesV87;
    uint64 HandoverRevisionV87=0;
    size_t HandoverStageV87=0;
    bool HandoverFallbackV87=false,HandoverFallbackArrivedV87=false;
    FFormationCells Cells,SourceCells;
    std::map<int,bool> Holds;
    std::set<std::string> Clears;
    TArray<TWeakObjectPtr<AActor>> ReservedActors;
    bool Complete=false,Success=false;
    bool Published=false;
    void DiscardActors(){for(const auto& Actor:ReservedActors)if(Actor.IsValid())Actor->Destroy();ReservedActors.Reset();}
public:
    FBatch(AWarfareFrontlineGameMode* In,FWorld& State):B(In),R(State){}
    ~FBatch(){if(!Published)DiscardActors();}
    void StageAutomaticPostHandoverV87(int32 Id,const std::string& Post){AutomaticPostHandoversV87.emplace(Id,Post);}
    wfexp::Result Preflight(const wfexp::tactical::Snapshot& S,const std::vector<wfexp::tactical::Intent>& Intents)override
    {
        DiscardActors();Support.clear();Routes.clear();Holds.clear();Clears.clear();SourceHandoverRoutesV87.clear();HandoverRoutesV87.clear();SourceCells=R.Cells;Cells=SourceCells;Complete=false;Published=false;
        if(!IsInGameThread()||!SameDeployment(B,R)||S.elapsed!=B->Elapsed||B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B))return Reject("Host deployment changed before preflight");
        const auto SnapshotValid=wfexp::tactical::ValidateSnapshot(R.State,S);if(!SnapshotValid)return SnapshotValid;
        const auto* M=Definition(B);if(!M||!R.SceneStaged)return Reject("Shared physical scene is not staged");
        if(!AutomaticPostHandoversV87.empty())
        {
            // Amend only an unarrived automatic promise in one of the two
            // measured post dependencies, alongside the resident's real move.
            // No currently occupied physical bay is released by this batch.
            if(!R.OrdinaryOrderV827||AutomaticPostHandoversV87.size()!=1
                ||Intents.size()!=1||Intents[0].kind!=wfexp::tactical::IntentKind::AdvanceExistingSquad
                ||R.State.objective.finished||R.State.objective.index>=M->stages.size())return Reject("Automatic bridge handover context changed");
            const auto& Stage=M->stages[R.State.objective.index];const auto& Request=*AutomaticPostHandoversV87.begin();
            const auto* Donor=B->FindSquad(Request.first);const auto* Resident=B->FindSquad(Intents[0].squadId);
            const auto* Post=Node(*M,Request.second);const auto* Goal=Node(*M,Intents[0].targetNode);
            const bool Galicia=R.Mission==TEXT("ru.galicia")&&!R.State.objective.fallback&&Request.second=="bridge";
            const bool MortRecovery=R.Mission==TEXT("fr.mort_homme")&&Stage.id=="fr.mort_homme.stage.2"
                &&R.State.objective.fallback&&!R.State.objective.fallbackArrived&&Request.second=="forest"
                &&Intents[0].targetNode=="front_right"&&Intents[0].targetNode==Stage.fallbackNode;
            if(!Galicia&&!MortRecovery)return Reject("Automatic bridge handover context changed");
            const auto Original=R.Routes.find(Request.first);const auto ResidentPath=Resident?R.Routes.find(Resident->Id):R.Routes.end();
            const auto Protected=[&](const FFrontlineSquad* Group)
            {return !Group||Group->Team!=0||Group->LivingCount()<=0||Group->bHoldPosition
                ||!R.OrdinarySectionsV827.Contains(Group->Id)||R.HeldOrdinarySectionsV827.Contains(Group->Id)
                ||ReservedByTacticsV827(B,R,Group->Id)
                ||std::find(R.State.objective.withdrawalParticipants.begin(),R.State.objective.withdrawalParticipants.end(),Group->Id)!=R.State.objective.withdrawalParticipants.end()
                ||(MortRecovery&&(std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Group->Id)!=R.State.objective.holdParticipants.end()
                    ||std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Group->Id)!=R.State.objective.fallbackWorkers.end()));};
            if(Stage.kind!=wfexp::tactical::GateKind::HoldNode||!Post||Post->capacity!=1||!Goal
                ||Goal->id!=(MortRecovery?Stage.fallbackNode:R.State.objective.boundObstacle)||Donor==Resident||Protected(Donor)||Protected(Resident)
                ||Resident->LivingCount()<Stage.minimumLiving||(Stage.requiredRole>=0&&int32(Resident->Role)!=Stage.requiredRole)
                ||std::find(Stage.mustOwn.begin(),Stage.mustOwn.end(),Post->id)!=Stage.mustOwn.end()
                ||Resident->bAdvancing||(ResidentPath!=R.Routes.end()&&InTransit(ResidentPath->second))
                ||!AtNode(*Resident,*Post,true)||!Donor->bAdvancing||Original==R.Routes.end()
                ||Original->second.Target!=wfexpue::Wide(Goal->id)||!IsIntermediateTransitLeg(Original->second)
                ||NextNode(Original->second,*M)!=Post->id||!Original->second.TransitCorridorWaiting
                ||Original->second.TransitCheckpointStep==Original->second.Step||Original->second.CarrierDelivery||BodyInPostV827(*Donor,*Post)
                ||BodyInPostV827(*Donor,*Goal))return Reject("Automatic bridge handover participants changed");
            const auto AtPost=Cells.find(Post->id);
            if(AtPost==Cells.end()||!AtPost->second[0].count(Resident->Id)
                ||!FormationSettled(*Resident,*Post,AtPost->second[0].at(Resident->Id)))return Reject("Automatic bridge guard is not in its actual bay");
            // The future receiver is checked in a copy, with only the departing
            // guard's booking excluded. Publish retains his real occupied bay
            // until ReleaseVacatedCells observes every living foot outside it.
            auto FutureCells=Cells;FutureCells[Post->id][0].erase(Resident->Id);FutureCells[Post->id][0].erase(Donor->Id);
            if(!AllocateFormationBayV87(*Post,0,Donor->Id,FutureCells,[&](int32 Cell)
                {return CellFits(B,*Post,0,Cell,Donor->Id)&&CellLeavesTransitSpaceV827(B,*Post,0,Cell,Donor->Id);}))return Reject("Future automatic bridge bay has no physical witness");
            FRoute Redirect=Original->second;Redirect.Target=wfexpue::Wide(Post->id);Redirect.Edges.resize(size_t(Redirect.Step)+1);
            Redirect.TransitCheckpointStep=INDEX_NONE;Redirect.TransitCheckpointOffset=FVector::ZeroVector;
            Redirect.TransitCorridorRetryAt=0;Redirect.TransitCorridorWaiting=false;Redirect.TransitRetryAt=0;
            Redirect.LastTransitRejection.Empty();Redirect.TransitRejectedFrom=FVector::ZeroVector;Redirect.TransitRejectedTo=FVector::ZeroVector;
            SourceHandoverRoutesV87.emplace(Donor->Id,Original->second);HandoverRoutesV87.emplace(Donor->Id,std::move(Redirect));
            if(auto Booking=Cells.find(Goal->id);Booking!=Cells.end())Booking->second[0].erase(Donor->Id);
            HandoverRevisionV87=R.State.revision;HandoverStageV87=R.State.objective.index;
            HandoverFallbackV87=R.State.objective.fallback;HandoverFallbackArrivedV87=R.State.objective.fallbackArrived;
        }
        std::set<int> Movers;for(const auto& Intent:Intents)if(Intent.kind==wfexp::tactical::IntentKind::AdvanceExistingSquad)Movers.insert(Intent.squadId);
        // Route.Target retains the atomic final-destination slot promise.
        // Physical formation bays belong to current bodies and the next post.
        std::array<std::map<std::string,int>,2> Reservations;
        for(const auto& Group:B->Squads)if(Group.LivingCount()>0)
        {
            if(Movers.count(Group.Id)||Group.Team<0||Group.Team>1)continue;
            const auto Redirect=HandoverRoutesV87.find(Group.Id);const auto Route=R.Routes.find(Group.Id);
            if(Redirect!=HandoverRoutesV87.end())++Reservations[Group.Team][wfexpue::Narrow(Redirect->second.Target)];
            else if(Route!=R.Routes.end()&&!Route->second.Target.IsEmpty()&&Group.bAdvancing)++Reservations[Group.Team][wfexpue::Narrow(Route->second.Target)];
            else if(const auto* N=OccupiedNode(Group,*M))++Reservations[Group.Team][N->id];
        }
        for(const auto& Pair:HandoverRoutesV87)
        {
            const auto Target=wfexpue::Narrow(Pair.second.Target);const auto* Post=Node(*M,Target);
            if(!Post||Reservations[0][Target]>Post->capacity)return Reject("Automatic bridge replacement destination is already reserved");
        }
        for(const auto& Intent:Intents)
        {
            if(Intent.side<0||Intent.side>1)return Reject("Invalid intent side");
            if(Intent.kind==wfexp::tactical::IntentKind::AdvanceExistingSquad)
            {
                const auto* Group=B->FindSquad(Intent.squadId);const auto* Target=Node(*M,Intent.targetNode);const auto* Here=Group?OccupiedNode(*Group,*M):nullptr;
                if(!Group||Group->Team!=Intent.side||Group->LivingCount()<=0||!Here||!AtNode(*Group,*Here,true)||Group->bAdvancing||!Target||Intent.routeEdges.empty()||Routes.count(Group->Id))return Reject("Movement does not own an entered stationary squad");
                if(++Reservations[Intent.side][Intent.targetNode]>Target->capacity)return Reject("Normal node capacity was reserved by another squad");
                if(FrozenHold(R,*M,*Group,Intent.targetNode))return Reject("The held objective must finish before withdrawal");
                std::string Cursor=Here->id;
                for(const auto& EdgeId:Intent.routeEdges)
                {
                    const auto* Link=Edge(*M,EdgeId);if(!Link)return Reject("Movement route contains an unregistered edge");
                    std::string Next;if(Link->from==Cursor)Next=Link->to;else if(Link->bidirectional&&Link->to==Cursor)Next=Link->from;else return Reject("Movement edges are discontinuous");
                    const auto* A=Node(*M,Cursor);const auto* Z=Node(*M,Next);
                    if(!A||!Z||!wfexp::scene::CanTraverseRoute(*Geometry(B),Link->id,A->position,Z->position))return Reject("Movement route crosses an unwalkable physical scene");
                    if(Intent.side==R.State.playerSide&&Link->approach=="enemy")return Reject("Enemy reinforcement axis is not a player approach");
                    const auto Owner=R.Owners.find(Next);if(Owner!=R.Owners.end()&&Owner->second==1-Intent.side&&Next!=Intent.targetNode)return Reject("Movement bypasses an opposed intermediate position");
                    const bool Worker=Next==Intent.targetNode&&RegisteredWorker(R,*M,*Group,Next);
                    if(!Link->clearanceGate.empty()&&!R.Cleared.count(Link->clearanceGate)&&!(Worker&&Link->clearanceGate==Next))return Reject("Movement route crosses uncleared wire");
                    Cursor=Next;
                }
                if(Cursor!=Intent.targetNode)return Reject("Movement route misses its reserved destination");
                FRoute Route;Route.Current=wfexpue::Wide(Here->id);Route.Target=wfexpue::Wide(Intent.targetNode);Route.Edges=Intent.routeEdges;
                if(auto Previous=R.Routes.find(Group->Id);Previous!=R.Routes.end())Route.ArrivalSequence=Previous->second.ArrivalSequence;
                auto FinalCells=Cells;
                if(!ReserveCell(B,*Target,Group->Team,Group->Id,FinalCells))return Reject("Destination has no clear physical formation bay");
                // Validate the final physical bay without occupying a future
                // intermediate section's exit. Its capacity promise above remains.
                if(const auto* Next=Node(*M,NextNode(Route,*M)))
                {
                    if(!IsIntermediateTransitLeg(Route)&&Next->id==Target->id)Cells=std::move(FinalCells);
                    // Transit uses the registered travel corridor. Only the
                    // terminal formation claims a physical bay.
                }
                Routes.emplace(Group->Id,std::move(Route));
            }
            else if(Intent.kind==wfexp::tactical::IntentKind::SetHold)
            {const auto* Group=B->FindSquad(Intent.squadId);if(!Group||Group->Team!=Intent.side||Group->LivingCount()<=0)return Reject("Hold order has no existing squad");if(!Intent.hold&&FrozenHold(R,*M,*Group,std::string()))return Reject("Current hold participants remain committed");Holds[Group->Id]=Intent.hold;}
            else if(Intent.kind==wfexp::tactical::IntentKind::RequestSupport)
            {
                if(Intent.supportId!=0&&Intent.supportId!=1)return Reject("Support intent has no actual host consumer");
                const auto Kind=EFrontlineSupport(Intent.supportId);
                if(!Support.empty()||!B->SupportBlockReason(Kind,Intent.side).IsEmpty()||Intent.salvos!=S.supportSalvoCount[Intent.side][Intent.supportId]
                    ||Intent.roundBudget!=S.supportRoundBudget[Intent.side][Intent.supportId]||Intent.salvoTargets.size()!=size_t(Intent.salvos))return Reject("Support entitlement changed before reservation");
                FReservedSupport Draft;Draft.Intent=Intent;Draft.Kind=Kind;
                Draft.Plan={Intent.id,S.encounterId,Intent.side,100000+B->ArtilleryCommands,Intent.roundBudget,S.elapsed,{}};
                if(Kind==EFrontlineSupport::Smoke)
                {
                    if(Intent.salvos!=2||Intent.roundBudget!=FrontlineFireSupportV26::SmokeRoundsV37)return Reject("Smoke intent is not the existing two-group ammunition budget");
                    const auto* G=Geometry(B);FVector Position=Vector(Intent.target);double Ground=0;
                    if(!G||!wfexp::scene::HeightAt(*G,Position.X,Position.Y,Ground))return Reject("Smoke target lies outside the shared scene");Position.Z=Ground;
                    const float ArrivalAt=float(FMath::Max(Intent.notBefore,S.elapsed+WarfareFRRUEquipmentV82::SupportArrival(B,Kind,Intent.side)));
                    auto* Visual=B->GetWorld()->SpawnActor<AWarfareFrontlineSmoke>(Position,FRotator::ZeroRotator);
                    if(Visual)ReservedActors.Add(Visual);
                    if(!IsValid(Visual)||!Visual->bVisualReady)return Reject("Smoke visual could not be reserved");
                    Visual->SetActorTickEnabled(false);Visual->SetActorHiddenInGame(true);Visual->DeployAt=ArrivalAt;
                    Visual->SetLifeSpan(0.f);
                    for(int32 Round=0;Round<Intent.roundBudget;++Round)
                    {
                        FVector Landing=Position+FVector((Round%3-1)*220.f+(Round>=3?55.f:0.f),(Round%3-1)*120.f+(Round>=3?-45.f:0.f),0);
                        Landing.X=FrontlineFireSupportV26::ReflectInside(Landing.X,float(G->maxX));Landing.Y=FrontlineFireSupportV26::ReflectInside(Landing.Y,float(G->maxY));
                        if(!wfexp::scene::HeightAt(*G,Landing.X,Landing.Y,Ground))return Reject("Smoke ammunition lands outside the shared scene");Landing.Z=Ground;
                        auto* Ammunition=B->GetWorld()->SpawnActor<AFrontlineSupportArrivalV30>();if(Ammunition)ReservedActors.Add(Ammunition);
                        if(!IsValid(Ammunition)||!Ammunition->Configure(B,Landing,Intent.side,ArrivalAt+Round*FrontlineFireSupportV26::SmokeBurstSpacing,true))return Reject("Smoke ammunition could not be reserved");
                        Ammunition->SetActorTickEnabled(false);Ammunition->SetActorHiddenInGame(true);
                    }
                    FFrontlineSmokeZone Zone;Zone.Team=Intent.side;Zone.Centre=Position;Zone.Radius=FrontlineFireSupportV26::SmokeRadius;Zone.StartedAt=ArrivalAt;
                    Zone.ExpiresAt=ArrivalAt+FrontlineFireSupportV26::SmokeDuration;Draft.Smoke=Zone;Support.push_back(std::move(Draft));continue;
                }
                if(Intent.salvos!=4||Intent.roundBudget!=12)return Reject("Artillery intent is not the single normal four-salvo entitlement");
                const auto Plan=FrontlineFireSupportV26::Plan(false,191726+B->ArtilleryCommands*1237+Intent.side*131);
                if(Plan.Num()!=Intent.roundBudget)return Reject("Host shell plan differs from reserved budget");
                Draft.HasJournal=S.nations[Intent.side]==wfexp::Faction::French;
                for(const auto& Round:Plan)
                {
                    FFrontlineStrike Strike;Strike.SourceV65=FFrontlineKillSourceV65::FireSupport(Intent.side,int32(EFrontlineSupport::Artillery));Strike.Team=Intent.side;
                    Strike.Target=Vector(Intent.salvoTargets[Round.Salvo])+FVector(Round.Offset.X,Round.Offset.Y,0);
                    const auto* G=Geometry(B);Strike.Target.X=FrontlineFireSupportV26::ReflectInside(Strike.Target.X,float(G->maxX));Strike.Target.Y=FrontlineFireSupportV26::ReflectInside(Strike.Target.Y,float(G->maxY));
                    double Ground=0;if(!wfexp::scene::HeightAt(*G,Strike.Target.X,Strike.Target.Y,Ground))return Reject("Support target lies outside the shared scene");Strike.Target.Z=Ground;
                    Strike.Radius=FrontlineFireSupportV26::FieldGunBlastRadius;Strike.Origin=Strike.Target+FVector(-B->GetTeamDirection(Intent.side)*1300,0,1800);
                    Strike.ImpactAt=float(FMath::Max(Intent.notBefore,S.elapsed+WarfareFRRUEquipmentV82::SupportArrival(B,EFrontlineSupport::Artillery,Intent.side))+Round.ImpactDelay-Plan[0].ImpactDelay);
                    Strike.LaunchedAt=Strike.ImpactAt-Round.TerminalFlight;Strike.FireMissionId=Draft.Plan.hostFireMissionId;Strike.SalvoIndex=Round.Salvo;Strike.RoundIndex=Round.Round;Strike.WarningLeadSeconds=Round.WarningLead;
                    Draft.Strikes.Add(Strike);Draft.Plan.rounds.push_back({{Round.Salvo,Round.Round},Strike.ImpactAt});
                }
                if(Draft.HasJournal){auto CandidateJournal=R.Journal;const auto Registered=CandidateJournal.Register(S,Intent,Draft.Plan);if(!Registered)return Registered;}
                Support.push_back(std::move(Draft));
            }
            else if(Intent.kind==wfexp::tactical::IntentKind::ClearObstacle)
            {if(!Node(*M,Intent.targetNode))return Reject("Obstacle is not a registered scene node");Clears.insert(Intent.targetNode);}
            else if(Intent.kind==wfexp::tactical::IntentKind::LocalObjectiveResult)
            {if(Complete)return Reject("Multiple local results in one batch");Complete=true;Success=Intent.success;}
            else if(Intent.kind!=wfexp::tactical::IntentKind::NotifyTelegraph&&Intent.kind!=wfexp::tactical::IntentKind::ObjectiveStageComplete)return Reject("Unimplemented actual host intent");
        }
        return {true,{}};
    }
    wfexp::Result Commit(const wfexp::tactical::Snapshot& S,const std::vector<wfexp::tactical::Intent>& Intents)override
    {
        if(!SameDeployment(B,R)||S.elapsed!=B->Elapsed||UGameplayStatics::IsGamePaused(B))return Reject("Host context changed before atomic commit");
        // The native transaction may be retried. Revalidate its actual host
        // participants before publishing any reserved order or cooldown.
        const auto* Mission=Definition(B);if(!Mission)return Reject("Mission disappeared before commit");
        if(R.Cells!=SourceCells)return Reject("Physical formation reservations changed before commit");
        if(!HandoverRoutesV87.empty())
        {
            if(!R.OrdinaryOrderV827||R.State.revision!=HandoverRevisionV87||R.State.objective.index!=HandoverStageV87
                ||R.State.objective.finished||R.State.objective.fallback!=HandoverFallbackV87
                ||R.State.objective.fallbackArrived!=HandoverFallbackArrivedV87)return Reject("Automatic bridge handover stage changed before commit");
            const auto MortFrozen=[&](int32 Id)
            {return HandoverFallbackV87&&(std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Id)!=R.State.objective.holdParticipants.end()
                ||std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Id)!=R.State.objective.fallbackWorkers.end()
                ||std::find(R.State.objective.withdrawalParticipants.begin(),R.State.objective.withdrawalParticipants.end(),Id)!=R.State.objective.withdrawalParticipants.end());};
            for(const auto& Pair:SourceHandoverRoutesV87)
            {
                const auto* Group=B->FindSquad(Pair.first);const auto Actual=R.Routes.find(Pair.first);const auto& Before=Pair.second;
                if(!Group||Group->Team!=0||Group->LivingCount()<=0||!Group->bAdvancing||Group->bHoldPosition
                    ||!R.OrdinarySectionsV827.Contains(Group->Id)||R.HeldOrdinarySectionsV827.Contains(Group->Id)
                    ||ReservedByTacticsV827(B,R,Group->Id)||MortFrozen(Group->Id)||Actual==R.Routes.end()
                    ||Actual->second.Current!=Before.Current||Actual->second.Target!=Before.Target||Actual->second.Edges!=Before.Edges
                    ||Actual->second.Step!=Before.Step||Actual->second.MemberWaypoints!=Before.MemberWaypoints
                    ||Actual->second.ArrivalSequence!=Before.ArrivalSequence||!Actual->second.TransitCorridorWaiting
                    ||Actual->second.EntryLane!=Before.EntryLane||Actual->second.CarrierDelivery!=Before.CarrierDelivery
                    ||Actual->second.TransitCheckpointStep!=Before.TransitCheckpointStep
                    ||!Actual->second.TransitCheckpointOffset.Equals(Before.TransitCheckpointOffset,.001))return Reject("Automatic bridge route changed before commit");
            }
            for(const auto& Pair:Routes)if(const auto* Group=B->FindSquad(Pair.first);!Group||Group->bHoldPosition
                ||!R.OrdinarySectionsV827.Contains(Pair.first)||R.HeldOrdinarySectionsV827.Contains(Pair.first)
                ||ReservedByTacticsV827(B,R,Pair.first)||MortFrozen(Pair.first))return Reject("Automatic bridge guard changed before commit");
        }
        for(const auto& Pair:Routes){const auto* Group=B->FindSquad(Pair.first);const auto* Here=Node(*Mission,wfexpue::Narrow(Pair.second.Current));if(!Group||!Here||Group->LivingCount()<=0||Group->bAdvancing||!AtNode(*Group,*Here,true))return Reject("Movement participant changed before commit");}
        for(const auto& Pair:Holds)if(const auto* Group=B->FindSquad(Pair.first);!Group||Group->LivingCount()<=0)return Reject("Hold participant disappeared before commit");
        for(const auto& Actor:ReservedActors)if(!Actor.IsValid())return Reject("Reserved support actor disappeared before commit");
        // No SpawnActor, callbacks, external file I/O or potentially failing
        // actor action occurs after publication begins. Journal registration is
        // fully reserved in a copy before any order or shared cooldown changes.
        auto CandidateJournal=R.Journal;
        for(const auto& Draft:Support)if(Draft.HasJournal){auto Result=CandidateJournal.Register(S,Draft.Intent,Draft.Plan);if(!Result)return Result;}
        B->Strikes.Reserve(B->Strikes.Num()+int32(Support.size())*12);
        B->SmokeZones.Reserve(B->SmokeZones.Num()+int32(Support.size()));
        R.Journal=std::move(CandidateJournal);
        R.Cells=std::move(Cells);
        for(auto& Pair:HandoverRoutesV87)
        {
            ReleaseGroupAdmissionsV827(R,Pair.first);R.Routes[Pair.first]=std::move(Pair.second);
            for(auto It=R.Approaches.begin();It!=R.Approaches.end();)if(It->first/16==Pair.first)It=R.Approaches.erase(It);else ++It;
        }
        for(auto& Pair:Routes)
        {
            ReleaseGroupAdmissionsV827(R,Pair.first);
            R.Routes[Pair.first]=std::move(Pair.second);auto* Group=B->FindSquad(Pair.first);
            for(auto It=R.Approaches.begin();It!=R.Approaches.end();)
                if(It->first/16==Pair.first)It=R.Approaches.erase(It);else ++It;
            if(Group->Team==0&&!R.OrdinaryOrderV827){R.OrdinarySectionsV827.Remove(Pair.first);R.HeldOrdinarySectionsV827.Remove(Pair.first);}
            Group->bHoldPosition=false;Group->bAdvancing=true;Group->Trench=INDEX_NONE;Group->Destination=INDEX_NONE;
        }
        for(const auto& Pair:Holds)
        {
            auto* Group=B->FindSquad(Pair.first);
            if(Pair.second)HoldGroupAdmissionsV87(B,*Mission,R,*Group);
            Group->bHoldPosition=Pair.second;
            if(Group->Team==0)
            {
                if(Pair.second)
                {if(R.OrdinarySectionsV827.Contains(Pair.first))R.HeldOrdinarySectionsV827.Add(Pair.first);R.OrdinarySectionsV827.Remove(Pair.first);}
                else if(R.HeldOrdinarySectionsV827.Remove(Pair.first)>0)
                {R.OrdinarySectionsV827.Add(Pair.first);R.OrdinaryReadyAtV827.FindOrAdd(Pair.first)=B->Elapsed+.8;}
            }
            auto Route=R.Routes.find(Pair.first);Group->bAdvancing=Route!=R.Routes.end()&&InTransit(Route->second);
        }
        std::set<std::string> NewlyCleared;for(const auto& Id:Clears)if(!R.Cleared.count(Id))NewlyCleared.insert(Id);
        R.Cleared.insert(Clears.begin(),Clears.end());
        for(const auto& Draft:Support)
        {
            if(Draft.Smoke.IsSet())
            {
                B->SmokeZones.Add(Draft.Smoke.GetValue());B->SmokeReadyAt[Draft.Intent.side]=B->Elapsed+WarfareFRRUEquipmentV82::SupportCooldown(B,Draft.Kind,Draft.Intent.side);++B->SmokeCommands;
                continue;
            }
            B->Strikes.Append(Draft.Strikes);B->ArtilleryReadyAt[Draft.Plan.side]=B->Elapsed+WarfareFRRUEquipmentV82::SupportCooldown(B,EFrontlineSupport::Artillery,Draft.Plan.side);
            ++B->ArtilleryCommands;if(Draft.HasJournal){R.ActivePlans[Draft.Plan.side]=Draft.Plan.planId;R.FirePlans[Draft.Plan.hostFireMissionId]=Draft.Plan.planId;}
        }
        for(const auto& Actor:ReservedActors){Actor->SetActorHiddenInGame(false);Actor->SetActorTickEnabled(true);if(auto* Visual=Cast<AWarfareFrontlineSmoke>(Actor.Get()))Visual->SetLifeSpan(Visual->DeployAt-B->Elapsed+FrontlineFireSupportV26::SmokeDuration+.2f);}
        Published=true;
        // Announce only a committed real route, using the existing national
        // actor lookup and shared speech budgets. No preflight or tick repeats.
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())for(const auto& Pair:Routes)
            if(const auto* Group=B->FindSquad(Pair.first))Audio->PlayVoice(EFrontlineVoiceEvent::Advance,
                WarfareFRRUCampaignV82::Nation(B,Group->Team)==EFRRUNationV81::German,Group->Centre(),Group->Id);
        for(const auto& Intent:Intents)if(Intent.kind==wfexp::tactical::IntentKind::ClearObstacle&&NewlyCleared.count(Intent.targetNode))
            if(const auto* ClearedNode=Node(*Mission,Intent.targetNode))FrontlineAchievementsV76::NativePassageClearedV82(B,Intent.side,wfexpue::Wide(Intent.targetNode),Vector(ClearedNode->position));
        for(const auto& Intent:Intents)if(Intent.kind==wfexp::tactical::IntentKind::ObjectiveStageComplete&&Intent.side==R.State.playerSide)
            FrontlineAchievementsV76::NativeObjectiveStageV82(B,wfexpue::Wide(Intent.stageId),R.State.objective.fallback);
        if(Complete){R.LocalResult=true;R.LocalSuccess=Success;}
        for(const auto& Intent:Intents)if(Intent.kind==wfexp::tactical::IntentKind::NotifyTelegraph)
        {
            const bool English=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English;
            if(Intent.side==R.State.playerSide&&!Intent.targetNode.empty()&&!Intent.alternateNode.empty())
            {
                // The Russian route-comparison order has already proved fresh
                // observation of both posts. Report that snapshot's ownership;
                // no hidden actor count, reserve or future command is consulted.
                auto Report=[&](const std::string& Id)
                {
                    static const char* Keys[]={"home","assembly","ridge","forest","front_left","front_right","relay","bridge","wire","rail","objective","exit","enemy_assembly"};
                    static const TCHAR* En[]={TEXT("Rear line"),TEXT("Assembly"),TEXT("Ridge"),TEXT("Wood line"),TEXT("Left approach"),TEXT("Right approach"),TEXT("Liaison post"),TEXT("Crossing"),TEXT("Wire gap"),TEXT("Rail cutting"),TEXT("Forward position"),TEXT("Withdrawal route"),TEXT("Enemy reserve")};
                    static const TCHAR* Zh[]={TEXT("后方阵地"),TEXT("集合点"),TEXT("山脊"),TEXT("林缘"),TEXT("左侧接近路"),TEXT("右侧接近路"),TEXT("联络岗"),TEXT("渡口"),TEXT("铁丝网缺口"),TEXT("铁路切口"),TEXT("前沿阵地"),TEXT("撤退通路"),TEXT("敌军集结点")};
                    FString Name;for(int32 I=0;I<UE_ARRAY_COUNT(Keys);++I)if(Id==Keys[I]){Name=English?En[I]:Zh[I];break;}
                    if(Id=="bridge"&&R.Mission==TEXT("fr.mort_homme"))Name=English?TEXT("Flank firing step"):TEXT("侧坡火力点");
                    const auto Post=std::find_if(S.nodes.begin(),S.nodes.end(),[&](const auto& N){return N.id==Id;});
                    if(Post==S.nodes.end())return FString();
                    const bool Contested=(Post->contestedSidesMask&3u)==3u;
                    const TCHAR* Status=Contested?(English?TEXT("contested"):TEXT("争夺中")):
                        Post->owner==R.State.playerSide?(English?TEXT("held by us"):TEXT("我军控制")):
                        Post->owner>=0?(English?TEXT("enemy-held"):TEXT("敌军控制")):(English?TEXT("unoccupied"):TEXT("尚未占领"));
                    return Name+(English?TEXT(": "):TEXT("："))+Status;
                };
                R.Notice=Report(Intent.targetNode)+(English?TEXT("; "):TEXT("；"))+Report(Intent.alternateNode)+(English?TEXT("."):TEXT("。"));
            }
            else R.Notice=English?TEXT("Enemy movement observed. Hold the approaches."):TEXT("发现敌军调动。守住各处入口。");
        }
        return {true,{}};
    }
};
}

FString SceneId(const AWarfareFrontlineGameMode* B){const auto* G=Geometry(B);return G?wfexpue::Wide(G->id):FString();}
bool SceneInfo(const AWarfareFrontlineGameMode* B,FString& Id,uint32& Revision,int32& SolidCount,FVector2D& Minimum,FVector2D& Maximum)
{const auto* G=Geometry(B);if(!G)return false;Id=wfexpue::Wide(G->id);Revision=G->revision;SolidCount=int32(G->solids.size());Minimum={G->minX,G->minY};Maximum={G->maxX,G->maxY};return true;}
bool NodePose(const AWarfareFrontlineGameMode* B,const FString& NodeId,FVector& Position,float& Radius)
{const auto* M=Definition(B);const auto* N=M?Node(*M,wfexpue::Narrow(NodeId)):nullptr;if(!N)return false;Position=Vector(N->position);Radius=float(N->radius);return true;}
bool TryHeight(const AWarfareFrontlineGameMode* B,float X,float Y,float& Height)
{
    const auto* G=Geometry(B);if(!G)return false;
    // The only extension is the three-second offstage ingress apron. Its level
    // is the actual boundary sample; it cannot create a combat/objective node.
    if(X<G->minX-2000||X>G->maxX+2000||Y<G->minY||Y>G->maxY)return false;
    double H=0;if(!wfexp::scene::HeightAt(*G,FMath::Clamp(double(X),G->minX,G->maxX),Y,H))return false;Height=float(H);WarfareFRRUArmourV82::WalkSurface(B,X,Y,Height,Height);return true;
}
bool TryGlobalHeight(float X,float Y,float& Height){return TryHeight(PhysicalOwner.Get(),X,Y,Height);}
bool GroundContact(const AWarfareFrontlineGameMode* B,const FVector& Position,FVector& Feet,FVector& CapsuleCentre)
{
    const auto* G=Geometry(B);if(!G||Position.ContainsNaN())return false;float Floor=0;if(!TryHeight(B,Position.X,Position.Y,Floor))return false;
    double CentreZ=0,NativeFloor=0;
    if(Position.X<G->minX||Position.X>G->maxX)CentreZ=Floor+90.; // flat offstage ingress apron, no objective/ground collision geometry
    else
    {if(!wfexp::scene::HeightAt(*G,Position.X,Position.Y,NativeFloor)||!wfexp::scene::GroundCapsuleCentre(*G,Position.X,Position.Y,25,85,5,CentreZ))return false;
        CentreZ+=Floor-NativeFloor;} // only the registered real running-rail contact can raise the foot surface
    const FVector CandidateFeet(Position.X,Position.Y,Floor),CandidateCentre(Position.X,Position.Y,CentreZ);
    if(CandidateFeet.ContainsNaN()||CandidateCentre.ContainsNaN())return false;Feet=CandidateFeet;CapsuleCentre=CandidateCentre;return true;
}
bool GroundContactPath(const AWarfareFrontlineGameMode* B,const FVector& From,const FVector& To,TArray<FVector>& Out)
{
    const auto* G=Geometry(B);if(!G||From.ContainsNaN()||To.ContainsNaN())return false;
    const double Length=FVector::Dist2D(From,To);if(!FMath::IsFinite(Length)||Length>100000)return false;
    // Seven centimetres also bounds the known scene gradient's chord error
    // below the unchanged skin. Actual capsule sweeps still own acceptance.
    TArray<double> Cuts;const int32 Samples=FMath::Max(1,FMath::CeilToInt(Length/7.));for(int32 I=0;I<=Samples;++I)Cuts.Add(double(I)/Samples);
    auto AddCrossings=[&](double Start,double End)
    {const double Delta=End-Start;if(FMath::Abs(Delta)<1.e-12)return;for(int32 Line=FMath::CeilToInt(FMath::Min(Start,End));Line<=FMath::FloorToInt(FMath::Max(Start,End));++Line)
        {const double T=(Line-Start)/Delta;if(T>0&&T<1)Cuts.Add(T);}};
    const double DX=(G->maxX-G->minX)/(G->columns-1),DY=(G->maxY-G->minY)/(G->rows-1);
    const double AX=(From.X-G->minX)/DX,AY=(From.Y-G->minY)/DY,ZX=(To.X-G->minX)/DX,ZY=(To.Y-G->minY)/DY;
    AddCrossings(AX,ZX);AddCrossings(AY,ZY);AddCrossings(AX-AY,ZX-ZY);Cuts.Sort();
    TArray<FVector> Candidate;double Previous=-1;
    for(const double T:Cuts){if(T-Previous<1.e-9)continue;Previous=T;FVector Feet,Centre;if(!GroundContact(B,FMath::Lerp(From,To,T),Feet,Centre))return false;Candidate.Add(Centre);}
    if(Candidate.IsEmpty())return false;Out=MoveTemp(Candidate);return true;
}
bool PlaceOnGround(AWarfareFrontlineSoldier* U,const FVector& Position)
{
    if(!U||!U->Battle||!WarfareFRRUCampaignV82::Active(U->Battle))return false;FVector Feet,Centre;
    if(!GroundContact(U->Battle,Position,Feet,Centre)||!U->Capsule||!U->Soldier)return false;
    // FollowSupport already integrated a short vertical fall. Preserve that
    // physical height rather than snapping its authored feet to the floor.
    const double Above=FMath::Max(0.0,Position.Z-Feet.Z);Feet.Z+=Above;Centre.Z+=Above;
    U->GroundContactOffsetV82=float(Centre.Z-Feet.Z);U->Capsule->SetCapsuleSize(25,85,false);U->SetActorLocation(Centre);
    U->Soldier->SetRelativeLocation(FVector(0,0,-U->GroundContactOffsetV82));return true;
}
bool TryWater(const AWarfareFrontlineGameMode* B,float X,float Y,float& Level,float& Depth,float* GroupIdentity)
{
    if(!B||PhysicalOwner.Get()!=B||!WarfareFRRUCampaignV82::Active(B))return false;
    const auto* G=Geometry(B);const auto* M=Definition(B);const auto& R=WorldFor(B);
    if(!G||!M||!R.SceneStaged||R.Encounter!=B->GetEncounterId()||R.Mission!=WarfareFRRUCampaignV82::MissionId(B)
        ||R.Scene!=wfexpue::Wide(G->id)||M->sceneId!=G->id||M->sceneRevision!=G->revision
        ||!WarfareFRRUContentV82::SceneStagedForWater(B,R.Scene,G->revision))return false;
    wfexp::scene::WaterSurface Surface;if(!wfexp::scene::QueryWaterSurface(*G,X,Y,Surface))return false;
    if(Surface.sceneId!=G->id||Surface.revision!=G->revision)return false;
    Level=float(Surface.levelCm);Depth=float(Surface.depthCm);if(GroupIdentity)*GroupIdentity=Surface.groupIndex/255.f;return true;
}
bool TryGlobalWater(float X,float Y,float& Level,float& Depth,float* GroupIdentity)
{return TryWater(PhysicalOwner.Get(),X,Y,Level,Depth,GroupIdentity);}
bool PhysicalSceneActive(){return PhysicalOwner.IsValid()&&WarfareFRRUCampaignV82::Active(PhysicalOwner.Get());}
float TravelScale(const AWarfareFrontlineGameMode* B,const FVector& P)
{const auto* G=Geometry(B);if(!G)return 1.f;if((P.X<G->minX||P.X>G->maxX)&&P.X>=G->minX-2000&&P.X<=G->maxX+2000&&P.Y>=G->minY&&P.Y<=G->maxY)return 1.f;const auto Foot=wfexp::scene::FootingAt(*G,P.X,P.Y);return Foot.valid&&Foot.walkable?float(Foot.movementScale):0.f;}
float GlobalTravelScale(const FVector& P){return TravelScale(PhysicalOwner.Get(),P);}
bool CanTraverse(const AWarfareFrontlineGameMode* B,const FVector& A,const FVector& Z)
{
    const auto* G=Geometry(B);if(!G)return false;
    if((A.X<G->minX||A.X>G->maxX)&&A.X>=G->minX-2000&&A.X<=G->maxX+2000)
    {float H=0;return FMath::Abs(Z.X)<=FMath::Abs(A.X)&&TryHeight(B,Z.X,Z.Y,H);}
    return wfexp::scene::CanTraverseSegment(*G,Point(A),Point(Z));
}
bool AllowsStep(const AWarfareFrontlineSoldier* U,const FVector& A,const FVector& Z)
{
    if(!U||!U->Battle||!WarfareFRRUCampaignV82::Active(U->Battle))return true;
    if(!CanTraverse(U->Battle,A,Z))return false;TArray<FVector> Centres;if(!GroundContactPath(U->Battle,A,Z,Centres))return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82Capsule),true,U);for(const auto& Other:U->Battle->Units)if(Other)Query.AddIgnoredActor(Other);
    for(int32 I=1;I<Centres.Num();++I)if(U->GetWorld()->SweepTestByChannel(Centres[I-1],Centres[I],FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),Query))return false;return true;
}
bool HasSight(const AWarfareFrontlineGameMode* B,const FVector& A,const FVector& Z)
{const auto* G=Geometry(B);return G&&wfexp::scene::LineOfSight(*G,Point(A),Point(Z));}
bool FiringLine(const AWarfareFrontlineSoldier* From,const AWarfareFrontlineSoldier* To)
{
    if(!From||!To||!From->Battle||From->Battle!=To->Battle||!From->IsActiveCombatant()||!To->IsActiveCombatant())return false;
    const auto* B=From->Battle.Get();const FVector Muzzle=From->GetMuzzleLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82ActualDischarge),true);for(const auto& U:B->Units)if(U)Query.AddIgnoredActor(U);
    const FVector Torso=To->Soldier?To->Soldier->GetBoneLocation(TEXT("spine_03")):To->Feet()+FVector(0,0,95);
    const FVector Head=To->Soldier?To->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):To->Feet()+FVector(0,0,145);
    for(const FVector& Target:{Torso,Head})if(HasSight(B,Muzzle,Target)&&!B->GetWorld()->LineTraceTestByChannel(Muzzle,Target,ECC_Visibility,Query))return true;
    return false;
}
float CoverFrom(const AWarfareFrontlineSoldier* U,const FVector& Source)
{
    if(!U||!U->Battle||!U->IsActiveCombatant())return 0.f;const auto* B=U->Battle.Get();const auto* G=Geometry(B);if(!G||!wfexp::scene::Contains(*G,Point(U->Feet())))return 0.f;
    // Node ownership grants no protection. Probe the real earth, masonry and
    // trunks toward the opposing line at the soldier's current body height.
    const FVector Feet=U->Feet();const FVector Bearing=(Source-Feet).GetSafeNormal2D();if(Bearing.IsNearlyZero()||Source.ContainsNaN())return 0.f;const float HeadHeight=U->Soldier?float(U->Soldier->GetBoneLocation(TEXT("Head")).Z-Feet.Z):145.f;
    auto& R=WorldFor(B);const int32 Person=U->SquadId*16+U->MemberIndex;auto Existing=R.CoverProbes.find(Person);
    if(Existing!=R.CoverProbes.end()&&B->Elapsed-Existing->second.At<=.12f&&FVector::DistSquared(Feet,Existing->second.Feet)<=16&&FMath::Abs(HeadHeight-Existing->second.HeadHeight)<=4&&FVector::DotProduct(Bearing,Existing->second.Bearing)>.995)return Existing->second.Value;
    const auto Footing=wfexp::scene::FootingAt(*G,Feet.X,Feet.Y);float Value=Footing.surface==wfexp::scene::Surface::Forest?.18f:0.f;
    const FVector Origin=Feet+FVector(0,0,FMath::Clamp(HeadHeight*.65f,40.f,98.f));const FVector Across(-Bearing.Y,Bearing.X,0);int32 Screens=0;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82PhysicalCover),true);for(const auto& Actor:B->Units)if(Actor)Query.AddIgnoredActor(Actor);
    for(float Lateral:{-110.f,0.f,110.f})
    {
        const FVector End=Origin+Bearing*240+Across*Lateral;if(!wfexp::scene::Contains(*G,Point(End)))continue;
        if(!HasSight(B,Origin,End)||B->GetWorld()->LineTraceTestByChannel(Origin,End,ECC_Visibility,Query))++Screens;
    }
    Value=FMath::Max(Value,float(Screens)*.24f);if(Value>0&&HeadHeight<105.f)Value=FMath::Min(.78f,Value+.06f);
    R.CoverProbes[Person]={Feet,Bearing,HeadHeight,B->Elapsed,Value};return Value;
}
float Cover(const AWarfareFrontlineSoldier* U)
{return U&&U->Battle?CoverFrom(U,U->Feet()+FVector(U->Battle->GetTeamDirection(U->Team)*1000,0,0)):0.f;}
bool EarthworkPose(const AWarfareFrontlineSoldier* U,FFRRUEarthworkPoseV82& Out)
{
    if(!U||!U->Battle||!U->IsActiveCombatant()||U->bMoving||U->GetMotionSpeed()>.5f||!WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* B=U->Battle.Get();const auto* M=Definition(B);const auto* G=Geometry(B);auto& R=WorldFor(B);const auto* Group=B->FindSquad(U->SquadId);
    if(!M||!G||!Group||Group->bAdvancing)return false;
    const auto Route=R.Routes.find(Group->Id);if(Route==R.Routes.end()||InTransit(Route->second))return false;
    const auto* Post=Node(*M,wfexpue::Narrow(Route->second.Current));if(!Post||!AtNode(*Group,*Post,true))return false;
    const auto Cells=R.Cells.find(Post->id);if(Cells==R.Cells.end())return false;const auto Bay=Cells->second[U->Team].find(Group->Id);if(Bay==Cells->second[U->Team].end())return false;
    const FVector Feet=U->Feet(),Facing(B->GetTeamDirection(U->Team),0,0);double Floor=0;
    if(!wfexp::scene::HeightAt(*G,Feet.X,Feet.Y,Floor)||FMath::Abs(Feet.Z-Floor)>32.)return false;
    // A gun rests on an actual raised earth lip, never on a node's ownership
    // label. Destroyed/flat positions fall back to ordinary ground deployment.
    for(float Distance=40;Distance<=240;Distance+=20)
    {
        const FVector Sample=Feet+Facing*Distance;double Lip=0;if(!wfexp::scene::HeightAt(*G,Sample.X,Sample.Y,Lip))continue;
        if(Lip<Floor+36.||Lip>Floor+140.)continue;
        FFRRUEarthworkPoseV82 Candidate;Candidate.NodeId=wfexpue::Wide(Post->id);Candidate.Bay=Bay->second;
        Candidate.Feet=Feet;Candidate.Support=FVector(Sample.X,Sample.Y,Lip+7.);Candidate.Facing=Facing;Candidate.CoverDepth=float(Lip-Floor);
        Out=MoveTemp(Candidate);return true;
    }
    return false;
}
bool WorkState(const AWarfareFrontlineSoldier* U,FFRRUWorkStateV82& Out)
{
    if(!U||!U->Battle||!U->IsActiveCombatant()||!WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* B=U->Battle.Get();const auto Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B)));
    const auto* M=Definition(B);const auto* Group=B->FindSquad(U->SquadId);
    if(!Found||!M||!Group||!SameDeployment(B,**Found)||Group->bHoldPosition||Group->bAdvancing||Group->Team!=(*Found)->State.playerSide)return false;
    const auto& R=**Found;if(R.State.objective.finished||R.State.objective.index>=M->stages.size())return false;const auto& Stage=M->stages[R.State.objective.index];
    // Only the surviving working member holds the actual tools. Other members
    // remain ordinary escorts; they do not all mime the same telephone/cutter.
    const AWarfareFrontlineSoldier* Worker=nullptr;for(const auto& Weak:Group->Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive()&&(!Worker||Member->MemberIndex<Worker->MemberIndex))Worker=Member;
    if(Worker!=U)return false;
    FFRRUWorkStateV82 Value;Value.Encounter=R.Encounter;Value.StageId=wfexpue::Wide(Stage.id);Value.SquadId=U->SquadId;Value.MemberIndex=U->MemberIndex;
    Value.HostElapsed=B->Elapsed;Value.Paused=UGameplayStatics::IsGamePaused(B);Value.StartedAt=R.State.objective.activatedAt;Value.EndsAt=R.State.objective.deadline;
    if(Group->Role==EFrontlineRole::Pioneer&&Stage.kind==wfexp::tactical::GateKind::ClearPassage)
    {
        const auto* Post=Node(*M,R.State.objective.boundObstacle);
        if(!Post||Group->LivingCount()<Stage.minimumLiving||!AtNode(*Group,*Post,Stage.stationary)||!RegisteredWorker(R,*M,*Group,Post->id))return false;
        for(const auto& Enemy:B->Units)if(Enemy&&Enemy->Team==1-Group->Team&&Enemy->IsActiveCombatant()&&FVector::Dist2D(Enemy->Feet(),Vector(Post->position))<=Post->radius)return false;
        if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* Vehicle:Fleet->GetVehicles())
            if(Vehicle&&Vehicle->Team==1-Group->Team&&WarfareFRRUArmourV82::OccupiesNode(Vehicle,Vector(Post->position),float(Post->radius)))return false;
        Value.Semantic=EFRRUWorkSemanticV82::ClearPassage;Value.NodeId=wfexpue::Wide(Post->id);Value.Target=Vector(Post->position);
        Value.ProgressSeconds=R.State.objective.progress;Value.RequiredSeconds=Stage.duration;Value.Timed=true;
        Value.NormalizedPhase=Stage.duration>0?float(FMath::Clamp(Value.ProgressSeconds/Stage.duration,0.,1.)):0;
        Value.CompletedOnce=R.Cleared.count(Post->id)>0;
    }
    else if(Group->Role==EFrontlineRole::Signals)
    {
        const auto* Post=Node(*M,"relay");if(!Post||!AtNode(*Group,*Post,true))return false;
        Value.Semantic=EFRRUWorkSemanticV82::SignalsRelay;Value.NodeId=TEXT("relay");Value.Target=U->Feet()+FVector(B->GetTeamDirection(U->Team)*75,0,75);
        const auto& Fire=R.State.fire[Group->Team];
        // A relay is an occupied communications post. Only an actual assigned
        // French observation has a timed progress window; Russian relay proof
        // grants no invented work timer or completion receipt.
        if((Fire.observerSquad==Group->Id||Fire.secondaryObserver==Group->Id)
            &&(Fire.phase==wfexp::tactical::FirePhase::Observing||Fire.phase==wfexp::tactical::FirePhase::Observed)&&B->Elapsed-Fire.lastObservation<=1.01)
        {Value.StartedAt=Fire.observationStart;Value.EndsAt=Fire.observationStart+Fire.observationRequired;Value.ProgressSeconds=Fire.lastObservation-Fire.observationStart;Value.RequiredSeconds=Fire.observationRequired;Value.NormalizedPhase=float(FMath::Clamp(Value.ProgressSeconds/Fire.observationRequired,0.,1.));Value.Timed=true;}
    }
    else return WarfareFRRUArmourV82::RepairWorkStateV87(U,Out);
    Value.Facing=(Value.Target-U->Feet()).GetSafeNormal2D();if(Value.Facing.IsNearlyZero())Value.Facing=FVector(B->GetTeamDirection(U->Team),0,0);
    Value.Active=!Value.Paused&&!Value.CompletedOnce;Out=Value;return true;
}
bool PassageCleared(const AWarfareFrontlineGameMode* B,const FString& NodeId)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return false;const auto Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B)));
    return Found&&SameDeployment(B,**Found)&&(*Found)->Cleared.count(wfexpue::Narrow(NodeId))>0;
}
bool PrepareScene(AWarfareFrontlineGameMode* B,FString& Error)
{
    Error=WarfareFRRUCampaignV82::DeploymentBlockReason(B);if(!Error.IsEmpty())return false;
    if(!CanPrepare(B)||B->Phase==EFrontlinePhase::Fighting){Error=TEXT("The operation is not ready to stage.");return false;}
    auto& R=WorldFor(B);const auto* G=Geometry(B);const auto* M=Definition(B);if(!G||!M||G->revision!=M->sceneRevision||M->sceneId!=G->id)return false;
    // Publish only the selected physical context before staging. Water remains
    // closed until all scene/encounter checks pass, including on a failed import.
    PhysicalOwner=B;
    if(!WarfareFRRUContentV82::StageScene(B,wfexpue::Wide(G->id),FTransform::Identity,int32(G->solids.size()),Error))return false;
    if(!WarfareFRRUArmourV82::StageRailway(B,Error))return false;
    if(R.HiddenLegacy.IsEmpty())for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(It->ActorHasTag(TEXT("FRRU_V82Scene")))continue;
        const bool LegacyOwner=It->ActorHasTag(TEXT("WarfareFrontline"));
        TArray<UPrimitiveComponent*> Parts;It->GetComponents(Parts);
        for(auto* C:Parts)
        {
            const bool LegacyGround=Cast<AWarfareFrontlineGround>(*It)!=nullptr;
            const FString Name=C->GetName();
            const bool LegacyField=Name.Contains(TEXT("Trench"))||Name.Contains(TEXT("Sandbag"))||Name.Contains(TEXT("Backdrop"))||Name.Contains(TEXT("Distant"));
            const auto* Static=Cast<UStaticMeshComponent>(C);const UStaticMesh* Mesh=Static?Static->GetStaticMesh():nullptr;
            const FString MeshPath=Mesh?Mesh->GetPathName():FString();
            // Preplaced V18 earthworks use the generic component name. These
            // exact six assets belong to the old battlefield, not this native
            // physical scene; preserve their state for the UK/DE return path.
            const bool LegacyPreplaced=It->ActorHasTag(TEXT("WF_V18_FieldProp"))&&(
                MeshPath==TEXT("/Game/Warfare/Field/V12/Meshes/SM_TrenchEarth_A.SM_TrenchEarth_A")
                ||MeshPath==TEXT("/Game/Warfare/Field/V12/Meshes/SM_TrenchEarth_B.SM_TrenchEarth_B")
                ||MeshPath==TEXT("/Game/Warfare/Field/V12/Meshes/SM_TrenchEarth_C.SM_TrenchEarth_C")
                ||MeshPath==TEXT("/Game/Warfare/Field/V12/Meshes/SM_BrokenRevetment_A.SM_BrokenRevetment_A")
                ||MeshPath==TEXT("/Game/Warfare/Field/V12/Meshes/SM_BrokenRevetment_B.SM_BrokenRevetment_B")
                ||MeshPath==TEXT("/Game/Warfare/Field/V12/Meshes/SM_BrokenRevetment_C.SM_BrokenRevetment_C"));
            if(!LegacyGround&&!LegacyField&&!LegacyOwner&&!LegacyPreplaced)continue;
            R.HiddenLegacy.Add({C,C->IsVisible(),C->GetCollisionEnabled()});C->SetVisibility(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    R.SceneStaged=true;R.Mission=WarfareFRRUCampaignV82::MissionId(B);R.Scene=wfexpue::Wide(G->id);R.Encounter=B->GetEncounterId();PhysicalOwner=B;return true;
}
bool Ready(const AWarfareFrontlineGameMode* B)
{if(!B||!HooksConnected()||!WarfareFRRUCampaignV82::Active(B))return false;const auto& R=WorldFor(B);const auto* G=Geometry(B);return G&&R.SceneStaged&&R.Mission==WarfareFRRUCampaignV82::MissionId(B)&&R.Encounter==B->GetEncounterId()&&WarfareFRRUContentV82::SceneStaged(B,R.Scene,G->revision);}
bool HostHooksConnected(){return HooksConnected();}
bool CanPrepare(const AWarfareFrontlineGameMode* B)
{return B&&WarfareFRRUCampaignV82::CampaignsAvailable(WarfareFRRUCampaignV82::Nation(B))&&HooksConnected()&&WarfareFRRUCampaignV82::Active(B)&&Geometry(B)&&Definition(B);}
FString BlockReason(const AWarfareFrontlineGameMode* B)
{const FString RosterReason=WarfareFRRUCampaignV82::DeploymentBlockReason(B);return !RosterReason.IsEmpty()?RosterReason:CanPrepare(B)?FString():TEXT("This operation is awaiting its field inspection.");}
bool Begin(AWarfareFrontlineGameMode* B,FString& Error)
{
    if(!WarfareFRRUCampaignV82::CampaignsAvailable(WarfareFRRUCampaignV82::Nation(B))){Error=WarfareFRRUCampaignV82::WorldBlockReason(B);return false;}
    Error=WarfareFRRUCampaignV82::DeploymentBlockReason(B);if(!Error.IsEmpty())return false;
    if(!Ready(B)||B->Phase!=EFrontlinePhase::Briefing)return false;
    FGuid NaturalEnemySeedGuidV87;if(!ReadNaturalEnemySeedGuidV87(B,NaturalEnemySeedGuidV87,Error))return false;
    auto& R=WorldFor(B);
    const auto Bound=wfexp::hostv81::BindIdentity(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(B)),0,B->GetTeamDirection(0),R.Identity);
    if(!Bound){Error=wfexpue::Wide(Bound.reason);return false;}
    if(!WarfareFRRUCampaignInternalV82::OpenEncounter(B,R.State,Error))return false;
    R.Started=true;R.Finished=R.PendingEnd=R.LocalResult=false;R.StoryPause={};R.HostFrame=0;R.Clock.Reset();
    R.HonourResultPending=false;R.HonourWinner=INDEX_NONE;
    R.Owners.clear();R.Cleared.clear();R.Routes.clear();R.Cells.clear();R.PostAdmissionsV827.clear();R.Approaches.clear();R.Arrivals.clear();R.CoverProbes.clear();R.Journal=wfexp::hostv81::SupportJournal();R.ActivePlans={};R.FirePlans.clear();R.Observers={};
#if WITH_EDITOR
    R.ActualStepsV87.clear();
#endif
    R.EnemyDecisionAt=3;R.EnemySupportAt=22;R.EnemyRandom.Initialize(GetTypeHash(R.Encounter));
#if WITH_EDITOR
    if(NaturalEnemySeedGuidV87.IsValid())
    {
        const uint32 ActualSeed=GetTypeHash(NaturalEnemySeedGuidV87);R.EnemyRandom.Initialize(static_cast<int32>(ActualSeed));
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_NATURAL_ENEMY_SEED actual_encounter=%s seed_source_guid=%s actual_uint_seed=%u scope=volatile_nosave_chemin_vb_editor_only encounter_identity_unchanged=true"),
            *R.Encounter.ToString(EGuidFormats::Digits),*NaturalEnemySeedGuidV87.ToString(EGuidFormats::Digits),ActualSeed);
    }
#endif
    R.EnemyRecruitAt=FrontlineGameplayV43::PreparationSeconds(R.EnemyRandom.FRandRange(2.6f,4.4f),1,B->GetCampaignDifficulty());
    R.EnemyRecruitStep=R.EnemyWaveRecruitCount=0;
    R.EnemyRosterIndexV827=0;R.OrdinaryAdvanceAtV827=0;R.OrdinaryOrderV827=false;
    R.OrdinarySectionsV827.Reset();R.HeldOrdinarySectionsV827.Reset();R.OrdinaryReadyAtV827.Reset();
    if(const auto* Operation=B->GetEncounterCampaignMission())R.EnemyAssaultAt=Operation->EnemyAssaultDelay*FrontlinePacingV24::TempoIntervalScale;
    R.EnemyPendingSupport=EFrontlineSupport::Count;R.EnemySupportTarget=FVector::ZeroVector;
    R.EnemySupportExecuteAt=0;R.EnemySupportContact.Reset();
    R.EnemyOpeningIndex=R.EnemyRecruitAttempts=0;R.EnemyRecruitRole=EFrontlineRole::Count;
    R.EnemyRecruitFromOpening=false;R.EnemyRecruitFailures.Reset();R.EnemyRecruitBlock.Reset();
    // Native support arrival and preparation remain real host seconds. No
    // classic technology or legacy mission-index multiplier is applied here.
    for(int32 Side=0;Side<2;++Side)
    {
        B->ArtilleryReadyAt[Side]=WarfareFRRUEquipmentV82::SupportCooldown(B,EFrontlineSupport::Artillery,Side);
        B->SmokeReadyAt[Side]=WarfareFRRUEquipmentV82::SupportCooldown(B,EFrontlineSupport::Smoke,Side);
        B->RallyReadyAt[Side]=WarfareFRRUEquipmentV82::SupportCooldown(B,EFrontlineSupport::Rally,Side);
        B->MortarReadyAt[Side]=WarfareFRRUEquipmentV82::SupportCooldown(B,EFrontlineSupport::Mortar,Side);
    }
    for(const auto& N:Definition(B)->nodes)R.Owners[N.id]=N.id=="home"||N.id=="exit"||N.id=="assembly"||N.id=="relay"?0:-1;
    return true;
}
bool Reset(AWarfareFrontlineGameMode* B)
{
    if(!B)return false;if(WarfareFRRUCampaignV82::Active(B)&&!WarfareFRRUCampaignInternalV82::RetireEncounter(B))return false;
    auto& R=WorldFor(B);for(const auto& Hidden:R.HiddenLegacy)if(auto* C=Hidden.Component.Get()){C->SetVisibility(Hidden.Visible);C->SetCollisionEnabled(Hidden.Collision);}
    WarfareFRRUArmourV82::ClearRailway(B);WarfareFRRUContentV82::ClearScene(B);WarfareFRRUCampaignInternalV82::ClearBinding(B);Worlds.Remove(TWeakObjectPtr<AWarfareFrontlineGameMode>(B));if(PhysicalOwner==B)PhysicalOwner.Reset();return true;
}
void Recruited(AWarfareFrontlineGameMode* B,int32 SquadId)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return;auto& R=WorldFor(B);auto* Group=B->FindSquad(SquadId);if(!Group)return;
    FrontlineMobilityV41::InvalidateRoster(B);
    FRoute Route;Route.Target=Group->Team==0?TEXT("home"):TEXT("enemy_assembly");
    for(int32 Order=0;Order<31;++Order)
    {const int32 Lane=15+(Order%2?-(Order+1)/2:Order/2);bool Used=false;for(const auto& Existing:R.Routes)if(const auto* Other=B->FindSquad(Existing.first);Other&&Other->Team==Group->Team&&Existing.second.Current.IsEmpty()&&Existing.second.EntryLane==Lane){Used=true;break;}if(!Used){Route.EntryLane=Lane;break;}}
    const auto* M=Definition(B);if(const auto* Target=M?Node(*M,wfexpue::Narrow(Route.Target)):nullptr)ReserveCell(B,*Target,Group->Team,SquadId,R.Cells);
    R.Routes[SquadId]=std::move(Route);
    if(Group->Team==0)R.OrdinarySectionsV827.Add(SquadId);
    Group->Trench=Group->Destination=INDEX_NONE;Group->bAdvancing=true;
}
bool Disembarked(AWarfareFrontlineGameMode* B,int32 SquadId,const AWarfareFrontlineSoldier* FirstOutside)
{
    if(!B||!FirstOutside||FirstOutside->Battle.Get()!=B||FirstOutside->SquadId!=SquadId||!FirstOutside->IsAlive()
        ||!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting)return false;
    FFRRUTrainPassengerStateV82 Passenger;
    if(!WarfareFRRUArmourV82::PassengerState(FirstOutside,Passenger)||Passenger.Stage!=EFRRUTrainPassengerStageV82::GroundClear)return false;
    auto& R=WorldFor(B);const auto* M=Definition(B);auto* Group=B->FindSquad(SquadId);
    if(!M||!Group||!SameDeployment(B,R))return false;
    if(R.Routes.count(SquadId)){FrontlineMobilityV41::InvalidateRoster(B);return true;}
    const FVector From=FirstOutside->Feet();const auto* G=Geometry(B);
    if(!G||!wfexp::scene::Contains(*G,Point(From)))return false;
    FVector DeliveryCorner,DeliveryCentre;
    if(!GroundContact(B,Passenger.CarrierClearanceCorner,DeliveryCorner,DeliveryCentre)
        ||!wfexp::scene::Contains(*G,Point(DeliveryCorner)))return false;
    TArray<TPair<const AWarfareFrontlineSoldier*,FVector>> Landings;
    for(const auto& Weak:Group->Members)if(const auto* U=Weak.Get();U&&U->IsAlive())
    {
        FFRRUTrainPassengerStateV82 Member;
        if(!WarfareFRRUArmourV82::PassengerState(U,Member)||Member.CarrierSerial!=Passenger.CarrierSerial
            ||Member.MemberIndex<0||Member.MemberIndex>=6||(Member.Car!=0&&Member.Car!=2))return false;
        const FVector Landing=Member.Stage==EFRRUTrainPassengerStageV82::GroundClear||Member.Stage==EFRRUTrainPassengerStageV82::Field
            ?U->Feet():Member.CarrierExitFeet;
        FVector Feet,Centre;
        if(!GroundContact(B,Landing,Feet,Centre)||!wfexp::scene::Contains(*G,Point(Feet)))return false;
        Landings.Emplace(U,Feet);
    }
    if(Landings.IsEmpty()||Landings.Num()>6)return false;
    bool TraceDelivery=false;
#if WITH_EDITOR
    TraceDelivery=CandidateReview()&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))
        &&B->Elapsed>=R.TrainDeliveryDiagnosticAt+5.f;
    if(TraceDelivery)R.TrainDeliveryDiagnosticAt=B->Elapsed;
#endif
    TArray<const wfexp::tactical::NodeDefinition*> Candidates;
    for(const auto& N:M->nodes)if(N.capacity>0&&N.id!="home"&&N.id!="assembly"&&N.id!="relay"&&N.id!="exit")Candidates.Add(&N);
    Candidates.Sort([&](const auto& A,const auto& Z){return FVector::DistSquared2D(From,Vector(A.position))<FVector::DistSquared2D(From,Vector(Z.position));});
    for(const auto* N:Candidates)
    {
        // The delivery approach goes around the live carrier's end. A direct
        // line to the receiving post crosses its solid carriages. Validate
        // both real legs for every passenger before publishing any reservation.
        for(int Cell=0;Cell<N->capacity;++Cell)
        {
            auto Cells=R.Cells;auto& Bays=Cells[N->id][Group->Team];bool Used=false;
            for(const auto& Bay:Bays)if(Bay.first!=SquadId&&Bay.second==Cell){Used=true;break;}
            if(Used||!CellFits(B,*N,Group->Team,Cell,SquadId))
            {if(TraceDelivery)UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_TRAIN_ROUTE squad=%d node=%s cell=%d rejection=%s"),SquadId,*wfexpue::Wide(N->id),Cell,Used?TEXT("occupied_bay"):TEXT("bay_contact"));continue;}
            // Both carriage exits must reach every living member's own post.
            // Validate before publishing cells or route, and try another free
            // bay if the first fitting bay has an inaccessible approach.
            bool AllReach=true;
            for(const auto& Landing:Landings)
            {
                FVector Target=FormationPoint(*N,Group->Team,Cell,Landing.Key->MemberIndex);float Floor=0;
                if(!TryHeight(B,Target.X,Target.Y,Floor)){AllReach=false;break;}Target.Z=Floor;
                const bool FirstLeg=AllowsStep(Landing.Key,Landing.Value,DeliveryCorner);
                const bool LastLeg=FirstLeg&&AllowsStep(Landing.Key,DeliveryCorner,Target);
                if(!FirstLeg||!LastLeg)
                {
                    if(TraceDelivery)UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_TRAIN_ROUTE squad=%d node=%s cell=%d member=%d rejection=%s from=%s to=%s"),
                        SquadId,*wfexpue::Wide(N->id),Cell,Landing.Key->MemberIndex,FirstLeg?TEXT("corner_to_post"):TEXT("exit_to_corner"),*Landing.Value.ToString(),*Target.ToString());
                    AllReach=false;break;
                }
            }
            if(!AllReach)continue;Bays[SquadId]=Cell;
            FRoute Route;Route.Target=wfexpue::Wide(N->id);Route.CarrierDelivery=true;Route.CarrierDeliveryCorner=DeliveryCorner;
            R.Cells=MoveTemp(Cells);R.Routes[SquadId]=MoveTemp(Route);
            Group->bAdvancing=true;Group->bReinforcement=false;Group->bHoldPosition=false;
            Group->Trench=Group->Destination=INDEX_NONE;FrontlineMobilityV41::InvalidateRoster(B);return true;
        }
    }
    return false;
}
static double BodySegmentGap(const FVector& Body,const FVector& A,const FVector& Z)
{
    const FVector Delta=(Z-A)*FVector(1,1,0);const double Length2=Delta.SizeSquared2D();
    const double Fraction=Length2>.0001?FMath::Clamp(FVector::DotProduct((Body-A)*FVector(1,1,0),Delta)/Length2,0.,1.):0.;
    return FVector::Dist2D(A+Delta*Fraction,Body);
}
static bool PhysicalFieldBodyV87(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->IsAlive()||WarfareFRRUArmourV82::Embarked(U))return false;
    if(U->bFieldEntered)return true;
    const auto* Ground=Geometry(U->Battle);if(!Ground)return false;const FVector Feet=U->Feet();
    // Entry visibility changes on the next actor tick. A real body whose
    // clearance already reaches the field cannot disappear from this step's
    // geometry merely because its centre is still just outside that boundary.
    // Distant offstage spawn groups retain their existing non-field status.
    return Feet.X>=Ground->minX-56.5&&Feet.X<=Ground->maxX+56.5
        &&Feet.Y>=Ground->minY-56.5&&Feet.Y<=Ground->maxY+56.5;
}
static bool LocalBodyLeg(const AWarfareFrontlineSoldier* U,const FVector& A,const FVector& Z)
{
    return FrontlineMobilityV41::VisitSweptBodies(U,A,Z,56.5,[&](const AWarfareFrontlineSoldier* Other)
    {
        if(Other==U||!PhysicalFieldBodyV87(Other))return true;
        const FVector Body=Other->Feet();const double Initial=FVector::Dist2D(A,Body);
        if(Initial<56.5)
        {
            // A pre-existing overlap may separate; it may not deepen or cut
            // through the other body while escaping its clearance circle.
            if(FVector::DotProduct((A-Body)*FVector(1,1,0),(Z-A)*FVector(1,1,0))<0||FVector::Dist2D(Z,Body)<=Initial+.01)return false;
        }
        else if(BodySegmentGap(Body,A,Z)<56.5)return false;
        return true;
    });
}
constexpr int32 MaximumBodyGuideVerticesV87=32;
// One read-only body visibility graph serves ordinary bypasses and registered
// rejoin sublegs. Planning changes no actor, route, clock or arrival receipt.
static bool BuildLocalBodyPathV87(const AWarfareFrontlineSoldier* U,const FVector& From,const FVector& Goal,
    const FVector& GuideCentre,double Limit,TArray<FVector>& Points,FString& Rejection,FVector& RejectedFrom,FVector& RejectedTo)
{
    Points.Reset();Rejection.Reset();RejectedFrom=From;RejectedTo=Goal;
    TArray<FVector> Neighbours;
    // Every body intersecting the actual swept leg lies no farther from Goal
    // than its remaining length plus the two-body cluster clearance. A fixed
    // 500 cm disc previously omitted real blockers near the approach start.
    const double CandidateReach=FMath::Max(500.,FVector::Dist2D(From,Goal)+113.);
    for(const auto& Other:U->Battle->Units)if(Other!=U&&PhysicalFieldBodyV87(Other)
        &&FVector::Dist2D(Other->Feet(),Goal)<=CandidateReach)Neighbours.Add(Other->Feet());
    Neighbours.Sort([&](const FVector& A,const FVector& Z){return BodySegmentGap(A,From,Goal)<BodySegmentGap(Z,From,Goal);});
    if(Neighbours.Num()>32)Neighbours.SetNum(32);
    TArray<int32> Cluster;
    for(int32 I=0;I<Neighbours.Num();++I)if(BodySegmentGap(Neighbours[I],From,Goal)<56.5)Cluster.Add(I);
    for(int32 At=0;At<Cluster.Num();++At)for(int32 I=0;I<Neighbours.Num();++I)
        if(!Cluster.Contains(I)&&FVector::Dist2D(Neighbours[I],Neighbours[Cluster[At]])<=113.)Cluster.Add(I);
    if(Cluster.IsEmpty()){Rejection=TEXT("body_seed_missing");RejectedFrom=From;RejectedTo=Goal;return false;}
    FVector Low=Neighbours[Cluster[0]],High=Low;
    for(const int32 I:Cluster){Low.X=FMath::Min(Low.X,Neighbours[I].X);Low.Y=FMath::Min(Low.Y,Neighbours[I].Y);High.X=FMath::Max(High.X,Neighbours[I].X);High.Y=FMath::Max(High.Y,Neighbours[I].Y);}
    // The graph goes around a 70 cm expanded hull, so a nearby body can seal
    // that hull's corners without touching the seed's 113 cm body cluster.
    // Close over those actual body circles too, using the same bounded
    // neighbours and clearances; never route through a body omitted from the
    // hull. The original fourteen graph candidates remain below.
    for(int32 Pass=0;Pass<Neighbours.Num();++Pass)
    {
        bool Grew=false;
        for(int32 I=0;I<Neighbours.Num();++I)if(!Cluster.Contains(I))
        {
            const FVector& Body=Neighbours[I];
            const double GapX=FMath::Max3(Low.X-70.-Body.X,0.,Body.X-High.X-70.);
            const double GapY=FMath::Max3(Low.Y-70.-Body.Y,0.,Body.Y-High.Y-70.);
            if(GapX*GapX+GapY*GapY>=56.5*56.5)continue;
            Cluster.Add(I);Grew=true;
            Low.X=FMath::Min(Low.X,Body.X);Low.Y=FMath::Min(Low.Y,Body.Y);
            High.X=FMath::Max(High.X,Body.X);High.Y=FMath::Max(High.Y,Body.Y);
        }
        if(!Grew)break;
    }
    TArray<FVector> Vertices{From,Goal};
    for(const FVector& Corner:{FVector(Low.X-70,Low.Y-70,0),FVector(Low.X-70,High.Y+70,0),FVector(High.X+70,Low.Y-70,0),FVector(High.X+70,High.Y+70,0)})
    {float Floor=0;if(FVector::Dist2D(Corner,From)>10.&&FVector::Dist2D(Corner,GuideCentre)<=Limit&&TryHeight(U->Battle,Corner.X,Corner.Y,Floor))Vertices.Add(FVector(Corner.X,Corner.Y,Floor));}
    // A soldier can already be between settled formation rows inside this
    // cluster's bounding box. Every diagonal to a corner can cross a body even
    // when a straight outward exit exists. Connect his actual position to the
    // same four expanded box sides, then use the unchanged visibility graph.
    // These are candidate points only: every complete leg still requires real
    // body clearance, walkable floor, ground contact and a swept capsule.
    for(const FVector& Exit:{FVector(Low.X-70,From.Y,0),FVector(High.X+70,From.Y,0),FVector(From.X,Low.Y-70,0),FVector(From.X,High.Y+70,0)})
    {float Floor=0;if(FVector::Dist2D(Exit,From)>10.&&FVector::Dist2D(Exit,GuideCentre)<=Limit&&TryHeight(U->Battle,Exit.X,Exit.Y,Floor))Vertices.Add(FVector(Exit.X,Exit.Y,Floor));}
    // The final leg enters the row from outside, rather than a bbox corner
    // cutting diagonally through the already settled neighbour's body.
    for(const FVector& Entry:{Goal+FVector(0,-70,0),Goal+FVector(0,70,0),Goal+FVector(-70,0,0),Goal+FVector(70,0,0)})
    {float Floor=0;if(FVector::Dist2D(Entry,From)>10.&&FVector::Dist2D(Entry,GuideCentre)<=Limit&&TryHeight(U->Battle,Entry.X,Entry.Y,Floor))Vertices.Add(FVector(Entry.X,Entry.Y,Floor));}
    // Axis exits can all cross settled ranks while an oblique gap between
    // two actual bodies remains open. Add that gap's medial point and its
    // perpendicular exits from the same expanded box, without moving a rank
    // or changing the checkpoint. The same 32 neighbours bound all pairs.
    struct FGapCandidateV87 {FVector Point;double Distance=0;bool BodyReachable=false,Reachable=false;};
    TArray<FGapCandidateV87> GapCandidates;
    const auto AddGapCandidate=[&](FVector Candidate)
    {
        float Floor=0;
        if(FVector::Dist2D(Candidate,From)<=10.||FVector::Dist2D(Candidate,GuideCentre)>Limit
            ||!TryHeight(U->Battle,Candidate.X,Candidate.Y,Floor))return;
        Candidate.Z=Floor;if(!LocalBodyLeg(U,Candidate,Candidate))return;
        for(const FVector& Existing:Vertices)if(Existing.Equals(Candidate,.01))return;
        for(const auto& Existing:GapCandidates)if(Existing.Point.Equals(Candidate,.01))return;
        FGapCandidateV87 Item;Item.Point=Candidate;Item.Distance=FVector::Dist2D(From,Candidate);
        Item.BodyReachable=LocalBodyLeg(U,From,Candidate);GapCandidates.Add(Item);
    };
    for(int32 A=0;A<Neighbours.Num();++A)for(int32 Z=A+1;Z<Neighbours.Num();++Z)
    {
        FVector Axis=Neighbours[Z]-Neighbours[A];Axis.Z=0;const double Width=Axis.Size2D();
        if(Width<=2.*56.5)continue;Axis/=Width;
        const FVector Middle=(Neighbours[A]+Neighbours[Z])*.5;
        if(Middle.X<Low.X-70.||Middle.X>High.X+70.||Middle.Y<Low.Y-70.||Middle.Y>High.Y+70.)continue;
        AddGapCandidate(Middle);const FVector Across(-Axis.Y,Axis.X,0);
        for(double Sign:{-1.,1.})
        {
            const FVector Direction=Across*Sign;double Exit=MAX_dbl;
            if(FMath::Abs(Direction.X)>1.e-6)Exit=FMath::Min(Exit,((Direction.X>0?High.X+70.:Low.X-70.)-Middle.X)/Direction.X);
            if(FMath::Abs(Direction.Y)>1.e-6)Exit=FMath::Min(Exit,((Direction.Y>0?High.Y+70.:Low.Y-70.)-Middle.Y)/Direction.Y);
            if(FMath::IsFinite(Exit)&&Exit>=0&&Exit<=2.*Limit)AddGapCandidate(Middle+Direction*(Exit+1.));
        }
    }
    // At most 496 pairs supply three candidates each. Keep only the closest
    // body-reachable candidates before the expensive real start-leg probes;
    // the final graph still contains at most 32 vertices including its base.
    GapCandidates.Sort([](const FGapCandidateV87& A,const FGapCandidateV87& Z)
    {return A.BodyReachable!=Z.BodyReachable?A.BodyReachable:A.Distance<Z.Distance;});
    if(GapCandidates.Num()>96)GapCandidates.SetNum(96);
    for(auto& Candidate:GapCandidates)Candidate.Reachable=Candidate.BodyReachable
        &&CanTraverse(U->Battle,From,Candidate.Point)&&AllowsStep(U,From,Candidate.Point);
    GapCandidates.Sort([](const FGapCandidateV87& A,const FGapCandidateV87& Z)
    {return A.Reachable!=Z.Reachable?A.Reachable:A.Distance<Z.Distance;});
    for(const auto& Candidate:GapCandidates)
    {if(Vertices.Num()>=MaximumBodyGuideVerticesV87)break;Vertices.Add(Candidate.Point);}
    bool Links[MaximumBodyGuideVerticesV87][MaximumBodyGuideVerticesV87]={};
    for(int32 A=0;A<Vertices.Num();++A)for(int32 Z=0;Z<Vertices.Num();++Z)if(A!=Z)
        Links[A][Z]=LocalBodyLeg(U,Vertices[A],Vertices[Z])&&CanTraverse(U->Battle,Vertices[A],Vertices[Z]);
    bool HasUsableStart=false;
    for(int32 Z=1;Z<Vertices.Num();++Z)if(Links[0][Z])
    {
        if(AllowsStep(U,From,Vertices[Z])){HasUsableStart=true;break;}
        Links[0][Z]=Links[Z][0]=false;
    }
    if(!HasUsableStart){Rejection=TEXT("body_graph_no_usable_start_leg");return false;}
    // Thirty-two bounded vertices remain a local body bypass, not a battlefield navigator.
    // Validate the selected complete route against unchanged actual capsules.
    for(int32 Attempt=0;Attempt<8;++Attempt)
    {
        std::array<double,MaximumBodyGuideVerticesV87> Costs;Costs.fill(1.e30);Costs[0]=0.;
        std::array<int32,MaximumBodyGuideVerticesV87> Previous;Previous.fill(INDEX_NONE);
        std::array<bool,MaximumBodyGuideVerticesV87> Visited{};
        for(int32 Visit=0;Visit<Vertices.Num();++Visit)
        {
            int32 Best=INDEX_NONE;for(int32 I=0;I<Vertices.Num();++I)if(!Visited[I]&&(Best==INDEX_NONE||Costs[I]<Costs[Best]))Best=I;
            if(Best==INDEX_NONE||Costs[Best]>=1.e29)break;Visited[Best]=true;
            for(int32 I=0;I<Vertices.Num();++I)if(Links[Best][I]&&!Visited[I])
            {const double Cost=Costs[Best]+FVector::Dist2D(Vertices[Best],Vertices[I]);if(Cost<Costs[I]){Costs[I]=Cost;Previous[I]=Best;}}
        }
        if(Previous[1]==INDEX_NONE)break;
        TArray<int32> Route;for(int32 I=1;I!=0&&I!=INDEX_NONE;I=Previous[I])Route.Insert(I,0);
        int32 Start=0;bool Valid=true;
        for(int32 End:Route)if(!AllowsStep(U,Vertices[Start],Vertices[End]))
        {Rejection=TEXT("planned_leg_capsule_or_contact");RejectedFrom=Vertices[Start];RejectedTo=Vertices[End];
            Links[Start][End]=Links[End][Start]=false;Valid=false;break;}else Start=End;
        if(!Valid)continue;
        for(int32 I:Route)Points.Add(Vertices[I]);return true;
    }
    return false;
}
static bool FormationJoinWaiting(const AWarfareFrontlineSoldier* U,const wfexp::tactical::NodeDefinition& N,int Cell,const FWorld& R)
{
    const auto* Group=U->Battle->FindSquad(U->SquadId);const auto* M=Definition(U->Battle);if(!Group||!M)return false;
    // A fully checked return to the registered approach already owns this
    // member's local motion. Parking a flank exactly at the join circle can
    // erase that chain across the middle man's route and make both wait.
    // Keep only this still-valid chain; every actual leg retains its body,
    // native and capsule checks. Arrival, invalidation or a new order restores
    // ordinary rank precedence rather than retaining a second motion owner.
    if(Cell!=INDEX_NONE&&Group->bAdvancing&&!Group->bHoldPosition&&!U->bEngaging)
    {
        const auto Local=R.Approaches.find(U->SquadId*16+U->MemberIndex);
        const auto Path=R.Routes.find(U->SquadId);const auto Bays=R.Cells.find(N.id);
        const auto Admission=R.PostAdmissionsV827.find(N.id);FVector Canonical=FormationPoint(N,U->Team,Cell,U->MemberIndex);float Floor=0;
        if(Local!=R.Approaches.end()&&Local->second.PathKindV87==FWorld::FLocalApproach::EPathKindV87::RegisteredNativeRejoin
            &&Local->second.Node==wfexpue::Wide(N.id)&&Local->second.Side==U->Team&&Local->second.Cell==Cell
            &&Local->second.Index>=0&&Local->second.Index<Local->second.Points.Num()
            &&Path!=R.Routes.end()&&InTransit(Path->second)&&!IsIntermediateTransitLeg(Path->second)
            &&Path->second.Target==wfexpue::Wide(N.id)&&NextNode(Path->second,*M)==N.id
            &&Bays!=R.Cells.end()&&Bays->second[U->Team].count(U->SquadId)&&Bays->second[U->Team].at(U->SquadId)==Cell
            &&Admission!=R.PostAdmissionsV827.end()&&Admission->second[U->Team].Owner==U->SquadId
            &&TryHeight(U->Battle,Canonical.X,Canonical.Y,Floor))
        {
            Canonical.Z=Floor;
            if(Local->second.Goal.Equals(Canonical,.01)&&FVector::Dist2D(U->Feet(),Canonical)>FormationArrivalToleranceCm)return false;
        }
    }
    const auto Rank=[&](int Member){const int Column=Member%3;return Column==1?0:Column==(Group->Team==0?2:0)?1:2;};
    const auto StationaryContact=[](const AWarfareFrontlineSoldier* Member)
    {return Member->bFieldEntered&&!Member->bMoving&&Member->GetMotionSpeed()<=.5f&&Member->bEngaging;};
    // Settle the middle pair before its flanking bodies close their approach.
    // A man stopped to fight is no longer joining; let others approach through
    // the unchanged body, native route and capsule checks around his real feet.
    // Dead members never retain precedence. Waiting keeps the route and bay;
    // Destination returns the actual feet so ordinary movement becomes idle.
    for(const auto& Weak:Group->Members)if(const auto* Other=Weak.Get();Other&&Other!=U&&Other->IsAlive()&&Rank(Other->MemberIndex)<Rank(U->MemberIndex))
        if(!StationaryContact(Other)&&(!Other->bFieldEntered||Other->bMoving||Other->GetMotionSpeed()>.5f
            ||FVector::Dist2D(Other->Feet(),FormationPoint(N,Group->Team,Cell,Other->MemberIndex))>FormationArrivalToleranceCm))return true;
    // Cross-section precedence belongs to the receiving admission queue.
    // A later reserved bay must not park men across the admitted join corridor.
    return false;
}
static bool LocalApproachDestination(const AWarfareFrontlineSoldier* U,const wfexp::tactical::NodeDefinition& N,int Cell,FWorld& R,const FVector& Goal,FVector& Out,bool IntermediateLeg=false)
{
    const int Person=U->SquadId*16+U->MemberIndex;const FVector From=U->Feet();const float Now=U->Battle->Elapsed;
    const double JoinLimit=N.radius+250.;
    // Waiting bodies stand at the join boundary. The admitted middle man
    // still needs a physical way around their outside edge, rather than
    // falling back to direct steering just outside that boundary.
    const double Limit=IntermediateLeg?850.:JoinLimit+200.;
    const FVector GuideCentre=IntermediateLeg?From:Vector(N.position);
    // A waiting flank may block a joining man behind the outer node circle.
    // Acquire within the same bounded node guide used by detour vertices; an
    // unrelated 650cm distance to his far-side slot excluded exactly those men.
    // Every actual leg still checks native floor, bodies and swept capsules.
    if(FVector::Dist2D(From,GuideCentre)>Limit)
    {R.Approaches.erase(Person);return false;}
    if(!IntermediateLeg&&FVector::Dist2D(From,Vector(N.position))<=JoinLimit&&FormationJoinWaiting(U,N,Cell,R))
    {R.Approaches.erase(Person);Out=From;return true;}
    auto& Guide=R.Approaches[Person];
    if(Guide.Node!=wfexpue::Wide(N.id)||Guide.Side!=U->Team||Guide.Cell!=Cell||!Guide.Goal.Equals(Goal,.01))
    {Guide={};Guide.Node=wfexpue::Wide(N.id);Guide.Side=U->Team;Guide.Cell=Cell;Guide.Goal=Goal;Guide.ProgressAt=Now;
        Guide.CanonicalBestRemainingV87=float(FVector::Dist2D(From,Goal));}
    // A changed goal clears the bound endpoint above. A wait or failed guide
    // cannot retain the last tick's precise-arrival lease.
    Guide.BendFinalLegValidatedAtV87=-1;
    const auto RememberRejection=[&]()
    {
        const auto Path=R.Routes.find(U->SquadId);if(Path==R.Routes.end()||!InTransit(Path->second))return;
        Path->second.LastTransitRejection=Guide.LastRejection;
        Path->second.TransitRejectedFrom=Guide.RejectedFrom;Path->second.TransitRejectedTo=Guide.RejectedTo;
    };
    // A final join can encounter another admitted rank after a safe corner.
    // Two unarrived men can cover one another's slots; requiring a complete
    // route to either occupied endpoint leaves both unable to step aside.
    // Admit the existing short body portal for that actual same-bay join too.
    // Every partial step still checks the unchanged real body/native/capsule
    // leg; only each man's actual feet at his own slot prove final arrival.
    if(!IntermediateLeg&&!Guide.FinalJoinBodyPortal&&Cell!=INDEX_NONE
        &&Guide.Points.IsEmpty()&&!U->bEngaging)
    {
        const auto Path=R.Routes.find(U->SquadId);const auto Bays=R.Cells.find(N.id);
        const bool RegisteredFinal=Path!=R.Routes.end()&&InTransit(Path->second)
            &&!IsIntermediateTransitLeg(Path->second)&&Path->second.Target==wfexpue::Wide(N.id)
            &&Bays!=R.Cells.end()&&Bays->second[U->Team].count(U->SquadId)
            &&Bays->second[U->Team].at(U->SquadId)==Cell
            &&FVector::DistSquared2D(Goal,FormationPoint(N,U->Team,Cell,U->MemberIndex))<=.0001;
        bool JoiningBodyBlocks=false;
        if(RegisteredFinal)FrontlineMobilityV41::VisitSweptBodies(U,From,Goal,56.5,[&](const AWarfareFrontlineSoldier* Other)
        {
            if(!Other||Other==U||Other->SquadId!=U->SquadId||Other->Team!=U->Team
                ||!Other->IsAlive()||!Other->bFieldEntered||Other->bMoving||Other->GetMotionSpeed()>.5f
                ||WarfareFRRUArmourV82::Embarked(Other)||FVector::Dist2D(Other->Feet(),Vector(N.position))>JoinLimit)return true;
            const bool Waiting=FormationJoinWaiting(Other,N,Cell,R);
            const bool Settled=!Other->bEngaging&&FVector::Dist2D(Other->Feet(),
                FormationPoint(N,Other->Team,Cell,Other->MemberIndex))<=FormationArrivalToleranceCm;
            const bool Joining=!Other->bEngaging&&FVector::Dist2D(Other->Feet(),
                FormationPoint(N,Other->Team,Cell,Other->MemberIndex))>FormationArrivalToleranceCm;
            if(!Waiting&&!Settled&&!Joining)return true;
            const FVector Body=Other->Feet();const double Initial=FVector::Dist2D(From,Body);
            JoiningBodyBlocks=Initial<56.5
                ?FVector::DotProduct((From-Body)*FVector(1,1,0),Goal-From)<0||FVector::Dist2D(Goal,Body)<=Initial+.01
                :BodySegmentGap(Body,From,Goal)<56.5;
            return !JoiningBodyBlocks;
        });
        if(JoiningBodyBlocks){Guide.FinalJoinBodyPortal=true;Guide.ClearPathV87();}
    }
    const bool ShortBodyPortal=IntermediateLeg||Guide.FinalJoinBodyPortal;
    if(Now<Guide.RetryAt){Out=From;return true;}
    // A temporary 80 cm tangent can be reached repeatedly while the same
    // authored checkpoint remains behind the same settled row. Its travelled
    // distance is not checkpoint progress. Escalate once after two seconds of
    // actual locomotion without 10 cm net improvement, and retain the existing
    // complete body graph until this real checkpoint/formation goal changes.
    // Do not discard a graph already taking a legitimate outward detour.
    if((ShortBodyPortal||Guide.PathKindV87==FWorld::FLocalApproach::EPathKindV87::TemporaryPortal)
        &&Guide.PathKindV87!=FWorld::FLocalApproach::EPathKindV87::RegisteredNativeRejoin
        &&!Guide.BodyGraphRequiredV87&&Guide.CanonicalNoProgressAttemptsV87>=2.f)
    {
        Guide.BodyGraphRequiredV87=true;
        if(Guide.PathKindV87!=FWorld::FLocalApproach::EPathKindV87::CompleteBodyGraph)Guide.ClearPathV87();
        Guide.LastRejection=TEXT("body_portal_no_canonical_progress");
        Guide.RejectedFrom=From;Guide.RejectedTo=Goal;RememberRejection();
    }
    // The stored path owns its meaning. Crossing from an ingress/transit guide
    // into the final-join domain cannot turn an old temporary portal into the
    // final bay. Consume the portal by its creation kind, then query the same
    // canonical goal again. Complete graph paths retain their real endpoint.
    if(Guide.PathKindV87==FWorld::FLocalApproach::EPathKindV87::TemporaryPortal
        &&!Guide.Points.IsEmpty()&&Guide.Index==Guide.Points.Num()-1
        &&FVector::Dist2D(From,Guide.Points.Last())<=10.)Guide.ClearPathV87();
    FVector BodyBypassGoal=Goal;
    int32 RegisteredBendIndexV87=INDEX_NONE,RegisteredBendLastV87=INDEX_NONE;
    const auto PureRegisteredBendV87=[&]()
    {return RegisteredBendIndexV87>=1&&RegisteredBendIndexV87<RegisteredBendLastV87;};
    const auto BindBendEndpointV87=[&](const FVector& Endpoint)
    {
        if(!PureRegisteredBendV87())return;
        Guide.BendEndpointBoundV87=true;Guide.BendEndpointV87=Endpoint;
    };
    const auto CacheBendFinalLegV87=[&](const FVector& Endpoint)
    {
        if(!PureRegisteredBendV87()||!Guide.BendEndpointBoundV87||!U->bFieldEntered
            ||U->Battle->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(U->Battle)
            ||!Endpoint.Equals(Guide.BendEndpointV87,.01))return;
        const double Offset=FVector::Dist2D(Endpoint,Goal),Margin=56.5-Offset;
        if(Offset<=44.01||Margin<=0.)return;
        // The caller has checked this actual last leg's bodies, native floor,
        // ground contact and capsule. Keep half the geometric margin unused.
        Guide.BendArrivalRadiusV87=float(FMath::Min(10.,Margin*.5));
        Guide.BendStopDistanceV87=FMath::Min(8.f,Guide.BendArrivalRadiusV87*.5f);
        Guide.BendFinalLegValidatedAtV87=Now;
        Guide.BendFinalLegValidatedFrameV87=GFrameCounter;
    };
#if WITH_EDITOR
    int32 RegisteredBendEdgeV87=INDEX_NONE;
    const auto RememberBendBypassV87=[&](const FVector& Chosen)
    {
        // Called only after the complete selected legs passed the existing
        // body/native/floor/capsule gates. This never advances the route.
        if(RegisteredBendIndexV87<1||RegisteredBendIndexV87>RegisteredBendLastV87||Chosen.Equals(Goal,.01)
            ||!U->Battle->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))||!CandidateReview())return;
        auto& Proof=R.ActualBendBypassesV87[Person];
        if(Proof.EdgeStep==RegisteredBendEdgeV87&&Proof.Checkpoint==RegisteredBendIndexV87&&Proof.Node==wfexpue::Wide(N.id)
            &&Proof.Bend.Equals(Goal,.01)&&Proof.Chosen.Equals(Chosen,.01))return;
        Proof={};Proof.EdgeStep=RegisteredBendEdgeV87;Proof.Checkpoint=RegisteredBendIndexV87;Proof.Last=RegisteredBendLastV87;
        Proof.Node=wfexpue::Wide(N.id);Proof.SelectedAt=Now;Proof.Bend=Goal;Proof.Chosen=Chosen;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_BEND_BYPASS_SELECTED squad=%d member=%d edge=%d P=%d last=%d host=%.3f bend=(%s) chosen=(%s) body=true native=true floor=true capsule=true guide_only=true"),
            U->SquadId,U->MemberIndex,Proof.EdgeStep,Proof.Checkpoint,Proof.Last,Now,*Goal.ToString(),*Chosen.ToString());
    };
#endif
    if(IntermediateLeg&&!U->bEngaging)
    {
        const auto* B=U->Battle.Get();const auto* M=Definition(B);const auto* Group=B?B->FindSquad(U->SquadId):nullptr;
        const auto Path=R.Routes.find(U->SquadId);FVector CanonicalCheckpoint;int Last=0;
        bool RegisteredCheckpoint=false;
        if(M&&Group&&Group->Team==U->Team&&Group->bAdvancing&&!Group->bHoldPosition
            &&Path!=R.Routes.end()&&InTransit(Path->second)
            &&NextNode(Path->second,*M)==N.id)
        {
            const auto Progress=Path->second.MemberWaypoints.find(U->MemberIndex);
            RegisteredCheckpoint=Progress!=Path->second.MemberWaypoints.end()
                &&TravelCheckpoint(B,*M,Path->second,U->MemberIndex,Progress->second,CanonicalCheckpoint,Last)
                &&Progress->second>=1&&Progress->second<=Last
                &&(Progress->second<Last||IsIntermediateTransitLeg(Path->second))&&Goal.Equals(CanonicalCheckpoint,.01);
            if(RegisteredCheckpoint)
            {
                RegisteredBendIndexV87=Progress->second;RegisteredBendLastV87=Last;
#if WITH_EDITOR
                RegisteredBendEdgeV87=Path->second.Step;
#endif
            }
        }
        bool SettledBodyCoversGoal=false,SettledBodyBlocksEntry=false;
        if(RegisteredCheckpoint)FrontlineMobilityV41::VisitSweptBodies(U,From,Goal,56.5,[&](const AWarfareFrontlineSoldier* Other)
        {
            if(!Other||Other==U||Other->Battle.Get()!=B||Other->Team!=U->Team
                ||!Other->IsAlive()||!Other->bFieldEntered||Other->bMoving||Other->GetMotionSpeed()>.5f
                ||Other->bEngaging||WarfareFRRUArmourV82::Embarked(Other))return true;
            // A route bend is a geometric guide disk. Every real stationary
            // friendly body can obstruct it, including a legally stopped
            // section whose different route has an enemy-owned intermediate.
            // This selects only U's guide and never moves or releases Other.
            const FVector Body=Other->Feet();const double Initial=FVector::Dist2D(From,Body);
            SettledBodyCoversGoal=FVector::Dist2D(Body,Goal)<56.5;
            SettledBodyBlocksEntry=Initial<56.5
                ?FVector::DotProduct((From-Body)*FVector(1,1,0),Goal-From)<0||FVector::Dist2D(Goal,Body)<=Initial+.01
                :BodySegmentGap(Body,From,Goal)<56.5;
            return !SettledBodyCoversGoal&&!SettledBodyBlocksEntry;
        });
        if(Guide.BendEndpointBoundV87&&PureRegisteredBendV87())
        {
            if(LocalBodyLeg(U,Guide.BendEndpointV87,Guide.BendEndpointV87))BodyBypassGoal=Guide.BendEndpointV87;
            else {Guide.BendEndpointBoundV87=false;Guide.ClearPathV87();}
        }
        if((SettledBodyCoversGoal||SettledBodyBlocksEntry)&&!Guide.BendEndpointBoundV87)
        {
            // A reached rank can cover a registered checkpoint or its entry
            // tangent. Aim inside that same unchanged checkpoint disk;
            // this does not apply the transit tolerance to a final bay slot.
            // 44 cm leaves the normal 10 cm movement stop inside 56.5; only real feet prove arrival.
            // A reached rank can cover the centre and the eight coarse angles
            // while leaving a narrow real opening inside the same arrival disk.
            // The endpoint must be free, but its approach need not be straight:
            // let the existing bounded body graph find the actual clear legs.
            double BestDistance=MAX_dbl,BestEndpoint=MAX_dbl;FVector Approach=Goal;
            for(const double Radius:{44.,54.})
            {
                // The original ring remains the fast path. Only a pure
                // registered bend may use the outer ring inside the same
                // unchanged actual 56.5 cm arrival disk.
                if(Radius>44.&&!PureRegisteredBendV87())break;
                for(int Index=0;Index<32;++Index)
                {
                    const double Angle=Index*PI/16.;FVector Candidate=Goal+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
                    float Floor=0;if(!TryHeight(B,Candidate.X,Candidate.Y,Floor))continue;Candidate.Z=Floor;
                    const double Distance=FVector::Dist2D(From,Candidate);
                    if(!LocalBodyLeg(U,Candidate,Candidate))continue;
                    if(Distance<BestEndpoint){BestEndpoint=Distance;BodyBypassGoal=Candidate;}
                    if(Distance>=BestDistance||!LocalBodyLeg(U,From,Candidate)||!CanTraverse(B,From,Candidate)
                        ||!AllowsStep(U,From,Candidate))continue;
                    BestDistance=Distance;Approach=Candidate;
                }
                if(BestEndpoint<MAX_dbl)break;
            }
            if(BestDistance<MAX_dbl)
            {
#if WITH_EDITOR
                RememberBendBypassV87(Approach);
#endif
                BindBendEndpointV87(Approach);CacheBendFinalLegV87(Approach);
                Guide.ClearPathV87();Out=Approach;return true;
            }
        }
    }
    if(!Guide.Points.IsEmpty())
    {
        bool ConsumedWaypoint=false;
        // Ordinary route movement stops within 10 cm. Only consume local
        // intermediate corners at that same distance; the real final bay
        // keeps the soldier's normal arrival and precise gun-placement rules.
        while(Guide.Index+1<Guide.Points.Num()&&FVector::Dist2D(From,Guide.Points[Guide.Index])<=10.)
        {
            // Reaching a corner's 10 cm arrival disk does not prove that the
            // actual stopped feet can cut directly onto its outgoing leg.
            const FVector Next=Guide.Points[Guide.Index+1];FString Rejection;
            if(!LocalBodyLeg(U,From,Next))Rejection=TEXT("corner_shortcut_body");
            else if(!CanTraverse(U->Battle,From,Next))Rejection=TEXT("corner_shortcut_native");
            else if(!AllowsStep(U,From,Next))Rejection=TEXT("corner_shortcut_capsule_or_contact");
            if(!Rejection.IsEmpty())
            {
                Guide.LastRejection=Rejection;Guide.RejectedFrom=From;Guide.RejectedTo=Next;
                RememberRejection();
                Guide.ClearPathV87();Guide.RetryAt=Now+.10f;Out=From;return true;
            }
            ++Guide.Index;Guide.ProgressAt=Now;Guide.BestRemaining=MAX_flt;ConsumedWaypoint=true;
        }
        if(Guide.Index>=Guide.Points.Num()){Guide.ClearPathV87();return false;}
        const FVector Waypoint=Guide.Points[Guide.Index];const float Remaining=float(FVector::Dist2D(From,Waypoint));
        if(Remaining<Guide.BestRemaining-.25f){Guide.BestRemaining=Remaining;Guide.ProgressAt=Now;Guide.StalledMovementAttemptsV87=0;}
        // Physical circling is not progress. The remaining distance to this
        // fixed waypoint must decrease; otherwise hold and retry the same bay.
        // Packing, reloading and turning legitimately keep the feet planted.
        // Count only failed locomotion attempts reported after real placement,
        // rather than expiring a safe guide during an authored gun action.
        if(Guide.StalledMovementAttemptsV87>=.75f){Guide.ClearPathV87();Guide.StalledMovementAttemptsV87=0;Guide.RetryAt=Now+.35f;Out=From;return true;}
        if(!LocalBodyLeg(U,From,Waypoint))
        {Guide.LastRejection=TEXT("current_leg_body");Guide.RejectedFrom=From;Guide.RejectedTo=Waypoint;RememberRejection();Guide.ClearPathV87();Guide.RetryAt=Now+.10f;Out=From;return true;}
        const bool PreciseBendLastLegV87=PureRegisteredBendV87()&&Guide.BendEndpointBoundV87
            &&Guide.Index==Guide.Points.Num()-1&&Waypoint.Equals(Guide.BendEndpointV87,.01)
            &&FVector::Dist2D(Waypoint,Goal)>44.01;
        if(ConsumedWaypoint||Now>=Guide.RevalidateAt||PreciseBendLastLegV87)
        {
            Guide.RevalidateAt=Now+.5f;
            if(!AllowsStep(U,From,Waypoint))
            {Guide.LastRejection=CanTraverse(U->Battle,From,Waypoint)?TEXT("current_leg_capsule_or_contact"):TEXT("current_leg_native");
                Guide.RejectedFrom=From;Guide.RejectedTo=Waypoint;RememberRejection();Guide.ClearPathV87();Guide.RetryAt=Now+.35f;Out=From;return true;}
        }
        if(PreciseBendLastLegV87)CacheBendFinalLegV87(Waypoint);
        Out=Waypoint;return true;
    }
    // A body detour can leave the final straight approach on a crater bank.
    // Return only to this same registered two-point leg and unchanged bay;
    // its receiver centre is an optional approach. This is a bounded recovery of an
    // admitted final join, not a replacement route or an arrival receipt.
    if(Cell!=INDEX_NONE&&!U->bEngaging&&!CanTraverse(U->Battle,From,Goal))
    {
        const auto* Group=U->Battle->FindSquad(U->SquadId);const auto* M=Definition(U->Battle);
        const auto Path=R.Routes.find(U->SquadId);const auto Bays=R.Cells.find(N.id);
        const auto Admissions=R.PostAdmissionsV827.find(N.id);
        const bool RegisteredFinal=Group&&M&&U->bFieldEntered&&U->Battle->Phase==EFrontlinePhase::Fighting
            &&!UGameplayStatics::IsGamePaused(U->Battle)&&Group->bAdvancing&&!Group->bHoldPosition
            &&Path!=R.Routes.end()&&InTransit(Path->second)&&!IsIntermediateTransitLeg(Path->second)
            &&Path->second.Step>=0&&Path->second.Step+1==int32(Path->second.Edges.size())&&!Path->second.CarrierDelivery
            &&NextNode(Path->second,*M)==N.id&&Path->second.Target==wfexpue::Wide(N.id)
            &&Bays!=R.Cells.end()&&Bays->second[U->Team].count(U->SquadId)&&Bays->second[U->Team].at(U->SquadId)==Cell
            &&Admissions!=R.PostAdmissionsV827.end()&&Admissions->second[U->Team].Owner==U->SquadId
            &&FVector::DistSquared2D(Goal,FormationPoint(N,U->Team,Cell,U->MemberIndex))<=.0001;
        const auto* Ground=RegisteredFinal?Geometry(U->Battle):nullptr;
        const auto* Physical=Ground?wfexp::scene::FindRoute(*Ground,Path->second.Edges[Path->second.Step]):nullptr;
        if(Physical&&Physical->points.size()==2&&RouteDepartureAllowed(U->Battle,*M,R,*Group,Path->second))
        {
            const bool Forward=wfexpue::Narrow(Path->second.Current)==Physical->from;
            const FVector Start=Vector(Physical->points[Forward?0:1]),End=Vector(Physical->points[Forward?1:0]);
            FVector Axis=End-Start;Axis.Z=0;const double Length=Axis.Size2D();
            FVector Centre=Vector(N.position);float Floor=0;
            const auto ClearLeg=[&](const FVector& A,const FVector& Z)
            {return LocalBodyLeg(U,A,Z)&&CanTraverse(U->Battle,A,Z)&&AllowsStep(U,A,Z);};
            const auto BuildRejoinLeg=[&](const FVector& A,const FVector& Z,TArray<FVector>& Points)
            {
                Points.Reset();
                // The registered line candidate must already have legal native
                // terrain. Reuse the body graph only for actual body blockage.
                if(!CanTraverse(U->Battle,A,Z))return false;
                if(LocalBodyLeg(U,A,Z))
                {if(!AllowsStep(U,A,Z))return false;Points.Add(Z);return true;}
                FString Rejection;FVector RejectedFrom,RejectedTo;
                return BuildLocalBodyPathV87(U,A,Z,GuideCentre,Limit,Points,Rejection,RejectedFrom,RejectedTo);
            };
            bool UsedTriangleTurnV87=false;FVector SelectedTriangleTurnV87=FVector::ZeroVector;
            const auto BuildInitialRejoinLeg=[&](const FVector& Rejoin,TArray<FVector>& Points)
            {
                UsedTriangleTurnV87=false;SelectedTriangleTurnV87=FVector::ZeroVector;
                // Preserve the existing direct/native/body path. A failed body
                // or capsule leg never enables an alternate terrain search.
                if(CanTraverse(U->Battle,From,Rejoin))return BuildRejoinLeg(From,Rejoin,Points);
                Points.Reset();
                // The actual 00->11 heightfield triangle supplies at most three
                // existing terrain corners. This is one local turn back to the
                // same registered line, not another route or a new arrival.
                if(!Ground||Ground->columns<2||Ground->rows<2
                    ||!wfexp::scene::Contains(*Ground,Point(From)))return false;
                const double Width=Ground->maxX-Ground->minX,Height=Ground->maxY-Ground->minY;
                if(!FMath::IsFinite(Width)||!FMath::IsFinite(Height)||Width<=0.||Height<=0.)return false;
                const double GX=(From.X-Ground->minX)/Width*(Ground->columns-1);
                const double GY=(From.Y-Ground->minY)/Height*(Ground->rows-1);
                if(!FMath::IsFinite(GX)||!FMath::IsFinite(GY))return false;
                const int32 IX=FMath::Min(Ground->columns-2,int32(GX)),IY=FMath::Min(Ground->rows-2,int32(GY));
                const double TX=GX-IX,TY=GY-IY;
                const double DX=Width/(Ground->columns-1),DY=Height/(Ground->rows-1);
                const FVector Corners[3]={
                    FVector(Ground->minX+IX*DX,Ground->minY+IY*DY,0),
                    FVector(Ground->minX+(IX+1)*DX,Ground->minY+(IY+1)*DY,0),
                    FVector(Ground->minX+(IX+(TX>=TY?1:0))*DX,Ground->minY+(IY+(TX>=TY?0:1))*DY,0)};
                for(FVector Turn:Corners)
                {
                    float TurnFloor=0;
                    if(FVector::Dist2D(Turn,From)<=10.||FVector::Dist2D(Turn,GuideCentre)>Limit
                        ||!TryHeight(U->Battle,Turn.X,Turn.Y,TurnFloor))continue;
                    Turn.Z=TurnFloor;TArray<FVector> Head,Tail;
                    if(!BuildRejoinLeg(From,Turn,Head)||Head.IsEmpty()||!Head.Last().Equals(Turn,.01)
                        ||!BuildRejoinLeg(Turn,Rejoin,Tail)||Head.Num()+Tail.Num()>MaximumBodyGuideVerticesV87)continue;
                    // Keep the triangle vertex between independently validated
                    // legs. The ordinary corner consumer may pass it only when
                    // the real feet are near and the outgoing physical leg is clear.
                    Points=MoveTemp(Head);Points.Append(Tail);
                    UsedTriangleTurnV87=true;SelectedTriangleTurnV87=Turn;return true;
                }
                return false;
            };
            if(Length>.01&&FVector::DistSquared2D(End,Centre)<=.0001&&TryHeight(U->Battle,Centre.X,Centre.Y,Floor))
            {
                Centre.Z=Floor;Axis/=Length;
                const double Projection=FMath::Clamp(FVector::DotProduct((From-Start)*FVector(1,1,0),Axis),0.,Length);
                for(int32 Sample=0;Sample<8;++Sample)
                {
                    FVector Rejoin=Start+Axis*FMath::Max(0.,Projection-Sample*80.);
                    if(FVector::Dist2D(Rejoin,From)<=10.||FVector::Dist2D(Rejoin,GuideCentre)>Limit
                        ||!TryHeight(U->Battle,Rejoin.X,Rejoin.Y,Floor))continue;
                    Rejoin.Z=Floor;TArray<FVector> Chain;
                    if(!BuildInitialRejoinLeg(Rejoin,Chain))continue;
                    const auto FinishAtSlot=[&](const FVector& Anchor,TArray<FVector>& Tail)
                    {
                        if(BuildRejoinLeg(Anchor,Goal,Tail))return true;
                        for(const FVector& Offset:{FVector(0,-70,0),FVector(0,70,0),FVector(-70,0,0),FVector(70,0,0)})
                        {
                            FVector Entry=Goal+Offset;
                            if(FVector::Dist2D(Entry,Centre)>Limit||!TryHeight(U->Battle,Entry.X,Entry.Y,Floor))continue;
                            Entry.Z=Floor;TArray<FVector> EntryPath,SlotPath;
                            if(!BuildRejoinLeg(Anchor,Entry,EntryPath)||!BuildRejoinLeg(Entry,Goal,SlotPath)
                                ||EntryPath.Num()+SlotPath.Num()>MaximumBodyGuideVerticesV87)continue;
                            Tail=MoveTemp(EntryPath);Tail.Append(SlotPath);return true;
                        }
                        return false;
                    };
                    // A two-point final approach proves arrival at its actual
                    // bay, not at the node centre. Keep a fully checked direct
                    // finish before considering that optional centre anchor.
                    TArray<FVector> Tail;bool Complete=FinishAtSlot(Rejoin,Tail);
                    if(!Complete)
                    {
                        TArray<FVector> CentrePath,SlotPath;
                        if(BuildRejoinLeg(Rejoin,Centre,CentrePath)&&FinishAtSlot(Centre,SlotPath)
                            &&CentrePath.Num()+SlotPath.Num()<=MaximumBodyGuideVerticesV87)
                        {Tail=MoveTemp(CentrePath);Tail.Append(SlotPath);Complete=true;}
                    }
                    if(!Complete||Chain.Num()+Tail.Num()>MaximumBodyGuideVerticesV87)continue;
                    Chain.Append(Tail);bool EntireChainClear=true;FVector Previous=From;
                    for(const FVector& Point:Chain)
                    {if(!ClearLeg(Previous,Point)){EntireChainClear=false;break;}Previous=Point;}
                    if(!EntireChainClear)continue;
                    Guide.ClearPathV87();FVector Prior=From;
                    for(const FVector& Point:Chain)if(!Point.Equals(Prior,.01)){Guide.Points.Add(Point);Prior=Point;}
                    if(Guide.Points.IsEmpty())continue;
                    Guide.PathKindV87=FWorld::FLocalApproach::EPathKindV87::RegisteredNativeRejoin;
                    Guide.ProgressAt=Now;Guide.StalledMovementAttemptsV87=0;Guide.RetryAt=0;
                    Guide.BestRemaining=float(FVector::Dist2D(From,Guide.Points[0]));Guide.RevalidateAt=Now+.5f;
                    Guide.LastRejection.Empty();Out=Guide.Points[0];
                    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_REGISTERED_NATIVE_REJOIN squad=%d member=%d edge=%s host=%.3f from=(%s) rejoin=(%s) receiver=(%s) slot=(%s) legs=%d terrain_triangle_turn_used=%d terrain_triangle_turn=(%s) complete_chain_cached=true body_native_capsule=true arrival_granted=false"),
                        U->SquadId,U->MemberIndex,*wfexpue::Wide(Path->second.Edges[Path->second.Step]),Now,*From.ToString(),*Rejoin.ToString(),*Centre.ToString(),*Goal.ToString(),Guide.Points.Num(),UsedTriangleTurnV87,*SelectedTriangleTurnV87.ToString());
                    return true;
                }
            }
            Guide.LastRejection=TEXT("registered_native_rejoin_no_safe_chain");Guide.RejectedFrom=From;Guide.RejectedTo=Goal;
            RememberRejection();Guide.RetryAt=Now+.35f;Out=From;return true;
        }
    }
    if(ShortBodyPortal&&!Guide.BodyGraphRequiredV87)
    {
        // Registered bends and admitted final joins move around real bodies
        // in sequence. Only the next maximum physical step needs a clear body
        // leg; the ordinary movement resolver retains every actual step's
        // native, ground-contact, capsule and body tests.
        FVector Forward=Goal-From;Forward.Z=0;const double Remaining=Forward.Size2D();
        if(Remaining<=.01)return false;Forward/=Remaining;
        const double LookAhead=FMath::Min(Remaining,80.);const FVector ShortGoal=From+Forward*LookAhead;
        if(LocalBodyLeg(U,From,ShortGoal))return false;
        FVector BlockingBody=FVector::ZeroVector;double BlockingDistance=MAX_dbl;int32 BlockingPerson=INDEX_NONE;bool HasBlocker=false;
        FrontlineMobilityV41::VisitSweptBodies(U,From,ShortGoal,56.5,[&](const AWarfareFrontlineSoldier* Other)
        {
            if(Other==U||!PhysicalFieldBodyV87(Other))return true;
            const FVector Body=Other->Feet();const double Initial=FVector::Dist2D(From,Body);
            const bool Blocks=Initial<56.5
                ?FVector::DotProduct((From-Body)*FVector(1,1,0),ShortGoal-From)<0||FVector::Dist2D(ShortGoal,Body)<=Initial+.01
                :BodySegmentGap(Body,From,ShortGoal)<56.5;
            if(Blocks&&Initial<BlockingDistance)
            {BlockingBody=Body;BlockingDistance=Initial;BlockingPerson=Other->SquadId*16+Other->MemberIndex;HasBlocker=true;}return true;
        });
        const FVector Across(-Forward.Y,Forward.X,0);TArray<FVector,TInlineAllocator<6>> Headings;
        for(double Turn:{.70,-.70,1.65,-1.65})Headings.Add((Forward+Across*Turn).GetSafeNormal());
        FVector Away=FVector::ZeroVector;
        if(HasBlocker)
        {
            Away=From-BlockingBody;Away.Z=0;Away=Away.GetSafeNormal();
            if(!Away.IsNearlyZero())
            {const FVector Tangent(-Away.Y,Away.X,0);Headings.Add((Tangent+Away*.10).GetSafeNormal());Headings.Add((-Tangent+Away*.10).GetSafeNormal());}
        }
        const bool BoundPassingSide=HasBlocker&&Guide.TransitBody==BlockingPerson&&Guide.TransitPassingSide!=0;
        double BestRemaining=MAX_dbl,RejectedRemaining=MAX_dbl;FVector Portal=FVector::ZeroVector,RejectedPortal=ShortGoal;
        FString PortalRejection=TEXT("body_bypass_no_safe_local_step");bool PortalOnPassingSide=false;float PortalSide=0;
        // At most six candidates, each at most 80 cm. A portal is usable only
        // with its complete real body/native/capsule leg, not a future path
        // through every body's present position up to the distant checkpoint.
        for(const FVector& Heading:Headings)
        {
            FVector Candidate=From+Heading*LookAhead;float Floor=0;
            FString Rejection;
            if(!TryHeight(U->Battle,Candidate.X,Candidate.Y,Floor))Rejection=TEXT("local_portal_missing_native_floor");else Candidate.Z=Floor;
            const double CandidateRemaining=FVector::Dist2D(Candidate,Goal);
            const double Around=Away.X*Heading.Y-Away.Y*Heading.X;
            const bool OnPassingSide=!BoundPassingSide||Around*Guide.TransitPassingSide>=-.001;
            if((PortalOnPassingSide&&!OnPassingSide)||(PortalOnPassingSide==OnPassingSide&&CandidateRemaining>=BestRemaining))continue;
            if(Rejection.IsEmpty()&&!LocalBodyLeg(U,From,Candidate))Rejection=TEXT("local_portal_body");
            if(Rejection.IsEmpty()&&!CanTraverse(U->Battle,From,Candidate))Rejection=TEXT("local_portal_native");
            if(Rejection.IsEmpty()&&!AllowsStep(U,From,Candidate))Rejection=TEXT("local_portal_capsule_or_contact");
            if(!Rejection.IsEmpty())
            {if(CandidateRemaining<RejectedRemaining){RejectedRemaining=CandidateRemaining;RejectedPortal=Candidate;PortalRejection=Rejection;}continue;}
            BestRemaining=CandidateRemaining;Portal=Candidate;PortalOnPassingSide=OnPassingSide;PortalSide=Around>0?1.f:Around<0?-1.f:0.f;
        }
        if(BestRemaining<MAX_dbl)
        {
            // Keep the same body's passing side while it has a safe leg. Only
            // a truly blocked preferred side considers the opposite side.
            Guide.TransitBody=BlockingPerson;Guide.TransitPassingSide=PortalSide;
            Guide.Points.Add(Portal);Guide.Index=0;Guide.ProgressAt=Now;Guide.StalledMovementAttemptsV87=0;
            Guide.PathKindV87=FWorld::FLocalApproach::EPathKindV87::TemporaryPortal;
            Guide.BestRemaining=float(FVector::Dist2D(From,Portal));Guide.RevalidateAt=Now+.5f;Out=Portal;return true;
        }
        Guide.LastRejection=PortalRejection;Guide.RejectedFrom=From;Guide.RejectedTo=RejectedPortal;
        RememberRejection();
        // Short tangents cannot escape every real body cluster. Fall through
        // to the existing bounded graph; every selected leg keeps its native,
        // floor, body and swept capsule checks.
    }
    if(LocalBodyLeg(U,From,BodyBypassGoal))
    {
        if(BodyBypassGoal.Equals(Goal,.01))return false;
        // Never fall back to the occupied canonical centre after selecting an
        // alternate endpoint. It is usable only with this complete real leg.
        if(CanTraverse(U->Battle,From,BodyBypassGoal)&&AllowsStep(U,From,BodyBypassGoal))
        {
#if WITH_EDITOR
            RememberBendBypassV87(BodyBypassGoal);
#endif
            BindBendEndpointV87(BodyBypassGoal);CacheBendFinalLegV87(BodyBypassGoal);
            Guide.ClearPathV87();Out=BodyBypassGoal;return true;
        }
        Out=From;return true;
    }
    TArray<FVector> BodyPoints;FString BodyRejection;FVector BodyRejectedFrom,BodyRejectedTo;
    const bool BodyPathBuilt=BuildLocalBodyPathV87(U,From,BodyBypassGoal,GuideCentre,Limit,BodyPoints,BodyRejection,BodyRejectedFrom,BodyRejectedTo);
    if(!BodyRejection.IsEmpty())
    {Guide.LastRejection=BodyRejection;Guide.RejectedFrom=BodyRejectedFrom;Guide.RejectedTo=BodyRejectedTo;RememberRejection();}
    if(BodyPathBuilt)
    {
#if WITH_EDITOR
        RememberBendBypassV87(BodyBypassGoal);
#endif
        BindBendEndpointV87(BodyBypassGoal);
        Guide.ClearPathV87();Guide.Points=MoveTemp(BodyPoints);Guide.ProgressAt=Now;Guide.StalledMovementAttemptsV87=0;
        Guide.PathKindV87=FWorld::FLocalApproach::EPathKindV87::CompleteBodyGraph;
        Guide.BestRemaining=float(FVector::Dist2D(From,Guide.Points[0]));Guide.RevalidateAt=Now+.5f;
        if(Guide.Points.Num()==1)CacheBendFinalLegV87(Guide.Points[0]);
        Out=Guide.Points[0];return true;
    }
    if(Guide.LastRejection.IsEmpty()){Guide.LastRejection=TEXT("body_bypass_no_safe_route");Guide.RejectedFrom=From;Guide.RejectedTo=BodyBypassGoal;}
    RememberRejection();Guide.RetryAt=Now+.35f;Out=From;return true;
}
bool RegisteredBendArrivalV87(const AWarfareFrontlineSoldier* U,const FVector& Destination,float& ArrivalRadius,float& StopDistance)
{
    // Read only the last leg validated by this tick's single Destination call.
    // No graph, body visit, route mutation or arrival acknowledgement occurs.
    if(!U||!U->Battle||!U->IsAlive()||!U->bFieldEntered||U->bEngaging
        ||U->Battle->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(U->Battle)
        ||!WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(U->Battle.Get()));
    if(!Found||!SameDeployment(U->Battle,**Found))return false;
    const auto Local=(**Found).Approaches.find(U->SquadId*16+U->MemberIndex);
    if(Local==(**Found).Approaches.end())return false;const auto& Guide=Local->second;
    if(!Guide.BendEndpointBoundV87||Guide.BendFinalLegValidatedAtV87!=U->Battle->Elapsed
        ||Guide.BendFinalLegValidatedFrameV87!=GFrameCounter
        ||!Destination.Equals(Guide.BendEndpointV87,.01))return false;
    ArrivalRadius=Guide.BendArrivalRadiusV87;StopDistance=Guide.BendStopDistanceV87;return true;
}
void RecordMovementV87(const AWarfareFrontlineSoldier* U,const FVector& From,const FVector& ActualTo,const FVector& Guide,float AttemptSeconds)
{
    if(!U||!U->Battle||!WarfareFRRUCampaignV82::Active(U->Battle)||From.ContainsNaN()||ActualTo.ContainsNaN())return;
    auto& R=WorldFor(U->Battle);if(!SameDeployment(U->Battle,R))return;
    const double Travel=FVector::Dist2D(From,ActualTo);
    if(const auto Local=R.Approaches.find(U->SquadId*16+U->MemberIndex);Local!=R.Approaches.end())
    {
        if(Travel>.0001)Local->second.StalledMovementAttemptsV87=0;
        else if(FMath::IsFinite(AttemptSeconds)&&AttemptSeconds>0)
            Local->second.StalledMovementAttemptsV87+=AttemptSeconds;
        const auto* Group=U->Battle->FindSquad(U->SquadId);
        if(FMath::IsFinite(AttemptSeconds)&&AttemptSeconds>0&&Group&&!Group->bHoldPosition&&!U->bEngaging)
        {
            auto& Approach=Local->second;const float Remaining=float(FVector::Dist2D(ActualTo,Approach.Goal));
            // This goal is the unchanged registered checkpoint or real bay,
            // never the temporary portal passed as the movement guide. Packing,
            // turning, hold and fighting have no eligible movement attempt.
            if(Remaining<=Approach.CanonicalBestRemainingV87-10.f)
            {Approach.CanonicalBestRemainingV87=Remaining;Approach.CanonicalNoProgressAttemptsV87=0;}
            else Approach.CanonicalNoProgressAttemptsV87+=AttemptSeconds;
        }
    }
#if WITH_EDITOR
    if(!U->Battle->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))||!CandidateReview())return;
    auto& Step=R.ActualStepsV87[U->SquadId*16+U->MemberIndex];
    Step.At=U->Battle->Elapsed;Step.From=From;Step.To=ActualTo;Step.Guide=Guide;
    if(Travel>.0001){Step.TravelCm+=Travel;++Step.Steps;}
    if(const auto Selected=R.ActualBendBypassesV87.find(U->SquadId*16+U->MemberIndex);Selected!=R.ActualBendBypassesV87.end())
    {
        auto& Proof=Selected->second;const auto Path=R.Routes.find(U->SquadId);const auto* M=Definition(U->Battle);
        if(M&&Proof.ReachedAt<0&&Proof.Checkpoint==Proof.Last&&Path!=R.Routes.end()
            &&Path->second.Step==Proof.EdgeStep&&IsIntermediateTransitLeg(Path->second)
            &&Proof.Node==wfexpue::Wide(NextNode(Path->second,*M))&&U->bFieldEntered
            &&FVector::Dist2D(ActualTo,Proof.Bend)<=56.5)
        {
            Proof.ReachedAt=U->Battle->Elapsed;Proof.ReachedFeet=ActualTo;
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_BEND_BYPASS_ACTUAL_REACHED squad=%d member=%d edge=%d P=%d host=%.3f actual_feet=(%s) original_checkpoint=(%s) actual_distance=%.3f unchanged_arrival_limit=56.5 arrival_granted=false"),
                U->SquadId,U->MemberIndex,Proof.EdgeStep,Proof.Checkpoint,Proof.ReachedAt,*ActualTo.ToString(),*Proof.Bend.ToString(),FVector::Dist2D(ActualTo,Proof.Bend));
        }
    }
#endif
}
#if WITH_EDITOR
TSharedPtr<FJsonObject> SupportDiagnosis(const AWarfareFrontlineGameMode* B)
{
    auto D=MakeShared<FJsonObject>();D->SetBoolField(TEXT("readOnly"),true);
    const auto* Found=B?Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B))):nullptr;
    if(!Found||!SameDeployment(B,**Found)){D->SetBoolField(TEXT("contextValid"),false);return D;}
    auto& R=**Found;D->SetBoolField(TEXT("contextValid"),true);
    D->SetStringField(TEXT("encounterId"),R.Encounter.ToString(EGuidFormats::Digits));
    // Read the actual reducer owner, never infer a fire policy from UI prose.
    const auto& Fire=R.State.fire[R.State.playerSide];
    D->SetNumberField(TEXT("firePhaseV87"),int32(Fire.phase));D->SetStringField(TEXT("planIdV87"),wfexpue::Wide(Fire.planId));
    D->SetStringField(TEXT("fireTargetV87"),wfexpue::Wide(Fire.node));D->SetNumberField(TEXT("observerSquadV87"),Fire.observerSquad);
    D->SetNumberField(TEXT("observationStartV87"),Fire.observationStart);D->SetNumberField(TEXT("lastObservationV87"),Fire.lastObservation);
    D->SetNumberField(TEXT("observationRequiredV87"),Fire.observationRequired);D->SetNumberField(TEXT("actualCommitAtV87"),Fire.commitAt);
    D->SetNumberField(TEXT("followupPhaseV87"),Fire.followupPhase);D->SetBoolField(TEXT("followupUsedV87"),Fire.followupUsed);
    D->SetBoolField(TEXT("followupPolicyActiveV87"),R.State.followupPolicyActive);D->SetStringField(TEXT("followupPolicyV87"),wfexpue::Wide(R.State.followupPolicy));
    TArray<TSharedPtr<FJsonValue>> Followups,Holds;
    for(const int32 Id:Fire.followupSquads)Followups.Add(MakeShared<FJsonValueNumber>(Id));
    for(const auto& Hold:R.State.arrivalHolds)
    {auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("squad"),Hold.first);Row->SetStringField(TEXT("target"),wfexpue::Wide(Hold.second));Holds.Add(MakeShared<FJsonValueObject>(Row));}
    D->SetArrayField(TEXT("assignedFollowupSquadsV87"),Followups);D->SetArrayField(TEXT("arrivalHoldsV87"),Holds);
    TArray<TSharedPtr<FJsonValue>> Receipts;
    for(int32 Side=0;Side<2;++Side)if(!R.ActivePlans[Side].empty()&&R.State.fire[Side].planId==R.ActivePlans[Side])
    {
        std::vector<wfexp::tactical::SupportReceipt> Actual;
        if(!R.Journal.ActiveReceipts(R.ActivePlans[Side],R.State.encounterId,Actual))continue;
        for(const auto& Receipt:Actual)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("planId"),wfexpue::Wide(Receipt.planId));
            Row->SetStringField(TEXT("encounterId"),wfexpue::Wide(Receipt.encounterId));Row->SetNumberField(TEXT("side"),Receipt.side);
            Row->SetNumberField(TEXT("resolvedSalvos"),Receipt.resolvedSalvos);Row->SetNumberField(TEXT("resolvedAt"),Receipt.resolvedAt);
            Receipts.Add(MakeShared<FJsonValueObject>(Row));
        }
    }
    D->SetArrayField(TEXT("activeActualPhaseReceipts"),Receipts);return D;
}
TSharedPtr<FJsonObject> ObjectiveDiagnosis(const AWarfareFrontlineGameMode* B)
{
    auto D=MakeShared<FJsonObject>();D->SetBoolField(TEXT("readOnly"),true);
    const auto* Found=B?Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B))):nullptr;
    const auto* M=B?Definition(B):nullptr;
    if(!Found||!M||!SameDeployment(B,**Found)||(**Found).State.objective.index>=M->stages.size())
    {D->SetBoolField(TEXT("contextValid"),false);return D;}
    auto& R=**Found;const auto& O=R.State.objective;const auto& Stage=M->stages[O.index];
    const std::string Target=O.fallback&&!O.fallbackArrived?Stage.fallbackNode:
        O.boundObstacle;
    // No pruning, route query, admission or clock publication is performed.
    const auto Snapshot=Capture(const_cast<AWarfareFrontlineGameMode*>(B),R,false);
    const auto Valid=wfexp::tactical::ValidateSnapshot(R.State,Snapshot);
    D->SetBoolField(TEXT("contextValid"),true);D->SetBoolField(TEXT("snapshotValid"),bool(Valid));
    D->SetStringField(TEXT("snapshotRejection"),wfexpue::Wide(Valid.reason));
    D->SetStringField(TEXT("stage"),wfexpue::Wide(Stage.id));D->SetNumberField(TEXT("kind"),int32(Stage.kind));
    D->SetStringField(TEXT("target"),wfexpue::Wide(Target));D->SetNumberField(TEXT("hostElapsed"),B->Elapsed);
    D->SetNumberField(TEXT("activatedAt"),O.activatedAt);D->SetNumberField(TEXT("deadline"),O.deadline);
    D->SetNumberField(TEXT("progressSeconds"),O.progress);D->SetNumberField(TEXT("requiredSeconds"),Stage.duration);
    D->SetNumberField(TEXT("minimumLiving"),Stage.minimumLiving);D->SetNumberField(TEXT("requiredRole"),Stage.requiredRole);
    D->SetBoolField(TEXT("stationaryRequired"),Stage.stationary);D->SetBoolField(TEXT("finished"),O.finished);
    D->SetBoolField(TEXT("success"),O.success);D->SetBoolField(TEXT("fallback"),O.fallback);
    D->SetBoolField(TEXT("fallbackArrived"),O.fallbackArrived);
    TArray<TSharedPtr<FJsonValue>> Withdrawal;
    for(const int32 Id:O.withdrawalParticipants)Withdrawal.Add(MakeShared<FJsonValueNumber>(Id));
    D->SetArrayField(TEXT("frozenWithdrawalParticipantsV87"),Withdrawal);
    TArray<TSharedPtr<FJsonValue>> Choices;
    for(const auto& Choice:R.State.choices)
    {auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("story"),wfexpue::Wide(Choice.storyId));Row->SetStringField(TEXT("choice"),wfexpue::Wide(Choice.choiceId));Choices.Add(MakeShared<FJsonValueObject>(Row));}
    D->SetArrayField(TEXT("actualStoryChoices"),Choices);
    D->SetNumberField(TEXT("playerMorale"),B->Morale[0]);D->SetNumberField(TEXT("enemyMorale"),B->Morale[1]);
    TArray<TSharedPtr<FJsonValue>> Posts,Sections;
    for(const auto& Post:Snapshot.nodes)if(Post.id==Target||std::find(Stage.mustOwn.begin(),Stage.mustOwn.end(),Post.id)!=Stage.mustOwn.end())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),wfexpue::Wide(Post.id));Row->SetNumberField(TEXT("owner"),Post.owner);
        Row->SetNumberField(TEXT("actualContestedSidesMask"),Post.contestedSidesMask);Posts.Add(MakeShared<FJsonValueObject>(Row));
    }
    for(const auto& Q:Snapshot.squads)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("id"),Q.id);Row->SetNumberField(TEXT("side"),Q.side);
        Row->SetNumberField(TEXT("role"),Q.roleId);Row->SetNumberField(TEXT("living"),Q.living);Row->SetStringField(TEXT("nativeNode"),wfexpue::Wide(Q.nodeId));
        Row->SetBoolField(TEXT("entered"),Q.entered);Row->SetBoolField(TEXT("stationary"),Q.stationary);
        Row->SetBoolField(TEXT("advancing"),Q.advancing);Row->SetBoolField(TEXT("holding"),Q.holding);
        if(const auto* Post=Node(*M,Target))Row->SetNumberField(TEXT("anchorDistanceToTargetCm"),FVector::Dist2D(Vector(Q.position),Vector(Post->position)));
        uint64 Watermark=0;for(const auto& At:O.arrivalSequences)if(At.first==Q.id)Watermark=FMath::Max(Watermark,uint64(At.second));
        Row->SetNumberField(TEXT("activationArrivalSequence"),double(Watermark));
        if(const auto Arrival=R.Arrivals.find(Q.id);Arrival!=R.Arrivals.end())
        {Row->SetStringField(TEXT("arrivalEdge"),wfexpue::Wide(Arrival->second.edgeId));Row->SetNumberField(TEXT("arrivalSequence"),double(Arrival->second.sequence));Row->SetNumberField(TEXT("arrivedAt"),Arrival->second.arrivedAt);}
        Sections.Add(MakeShared<FJsonValueObject>(Row));
    }
    D->SetArrayField(TEXT("requiredPosts"),Posts);D->SetArrayField(TEXT("actualNativeSections"),Sections);return D;
}
TSharedPtr<FJsonObject> MovementDiagnosis(const AWarfareFrontlineSoldier* U,bool TestClearance)
{
    auto D=MakeShared<FJsonObject>();D->SetBoolField(TEXT("readOnly"),true);
    const auto* B=U?U->Battle.Get():nullptr;
    const auto* Group=B?B->FindSquad(U->SquadId):nullptr;
    const auto* Found=B?Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B))):nullptr;
    const auto* M=B?Definition(B):nullptr;
    if(!U||!Group||!Found||!M||!SameDeployment(B,**Found)){D->SetBoolField(TEXT("contextValid"),false);return D;}
    const auto& R=**Found;D->SetBoolField(TEXT("contextValid"),true);
    D->SetStringField(TEXT("actualNativeNode"),FString());
    const auto ActualRoute=R.Routes.find(Group->Id);
    if(ActualRoute==R.Routes.end()||!InTransit(ActualRoute->second))if(const auto* ActualNode=OccupiedNode(*Group,*M))
    {const auto ActualCells=R.Cells.find(ActualNode->id);if(ActualCells!=R.Cells.end())
        if(const auto ActualBay=ActualCells->second[Group->Team].find(Group->Id);ActualBay!=ActualCells->second[Group->Team].end()
            &&FormationSettled(*Group,*ActualNode,ActualBay->second))D->SetStringField(TEXT("actualNativeNode"),wfexpue::Wide(ActualNode->id));}
    D->SetBoolField(TEXT("ordinaryOwnsSection"),R.OrdinarySectionsV827.Contains(Group->Id));
    D->SetBoolField(TEXT("ordinaryHoldWasRemembered"),R.HeldOrdinarySectionsV827.Contains(Group->Id));
    D->SetBoolField(TEXT("holding"),Group->bHoldPosition);D->SetBoolField(TEXT("advancing"),Group->bAdvancing);
    D->SetBoolField(TEXT("engaging"),U->bEngaging);
    D->SetNumberField(TEXT("weaponPhase"),int32(U->WeaponPhase));
    if(const auto* Gun=FrontlineMachinegunsV30::Get(U))
    {D->SetNumberField(TEXT("gunState"),int32(Gun->State));D->SetNumberField(TEXT("gunPacks"),Gun->Packs);
        D->SetNumberField(TEXT("gunDeployments"),Gun->Deployments);D->SetNumberField(TEXT("gunStanceSeconds"),Gun->StanceClock);}
    if(const auto Actual=R.ActualStepsV87.find(U->SquadId*16+U->MemberIndex);Actual!=R.ActualStepsV87.end())
    {
        const auto& Step=Actual->second;auto Proof=MakeShared<FJsonObject>();
        Proof->SetNumberField(TEXT("hostElapsed"),Step.At);Proof->SetStringField(TEXT("from"),Step.From.ToString());
        Proof->SetStringField(TEXT("actualTo"),Step.To.ToString());Proof->SetStringField(TEXT("guide"),Step.Guide.ToString());
        Proof->SetNumberField(TEXT("acceptedTravelCm"),FVector::Dist2D(Step.From,Step.To));
        Proof->SetNumberField(TEXT("cumulativeAcceptedTravelCm"),Step.TravelCm);Proof->SetNumberField(TEXT("acceptedSteps"),double(Step.Steps));
        if(const auto Bend=R.ActualBendBypassesV87.find(U->SquadId*16+U->MemberIndex);Bend!=R.ActualBendBypassesV87.end())
        {
            const auto& Witness=Bend->second;auto BendProof=MakeShared<FJsonObject>();
            BendProof->SetNumberField(TEXT("edgeIndex"),Witness.EdgeStep);BendProof->SetStringField(TEXT("node"),Witness.Node);
            BendProof->SetNumberField(TEXT("registeredCheckpointIndex"),Witness.Checkpoint);BendProof->SetNumberField(TEXT("lastCheckpointIndex"),Witness.Last);
            BendProof->SetNumberField(TEXT("selectedAtHost"),Witness.SelectedAt);BendProof->SetStringField(TEXT("originalBend"),Witness.Bend.ToString());
            BendProof->SetStringField(TEXT("chosenGuideEndpoint"),Witness.Chosen.ToString());BendProof->SetNumberField(TEXT("chosenOffsetCm"),FVector::Dist2D(Witness.Bend,Witness.Chosen));
            BendProof->SetBoolField(TEXT("completeSelectedLegsBodyNativeFloorCapsulePassed"),true);BendProof->SetBoolField(TEXT("guideOnlyNoArrivalGrant"),true);
            BendProof->SetBoolField(TEXT("originalCheckpointActuallyAdvanced"),Witness.AdvancedAt>=0);BendProof->SetNumberField(TEXT("advancedAtHost"),Witness.AdvancedAt);
            BendProof->SetBoolField(TEXT("originalCheckpointActuallyReached"),Witness.ReachedAt>=0||Witness.AdvancedAt>=0);
            BendProof->SetNumberField(TEXT("reachedAtHost"),Witness.ReachedAt);BendProof->SetStringField(TEXT("actualFeetWhenReached"),Witness.ReachedFeet.ToString());
            BendProof->SetNumberField(TEXT("advancedToCheckpointIndex"),Witness.AdvancedTo);BendProof->SetStringField(TEXT("actualFeetWhenAdvanced"),Witness.AdvancedFeet.ToString());
            if(Witness.AdvancedAt>=0)BendProof->SetNumberField(TEXT("actualDistanceToOriginalBendWhenAdvancedCm"),FVector::Dist2D(Witness.AdvancedFeet,Witness.Bend));
            Proof->SetObjectField(TEXT("registeredBendBypassV87"),BendProof);
        }
        Proof->SetBoolField(TEXT("afterActualActorPlacement"),true);D->SetObjectField(TEXT("guideExecutionV87"),Proof);
    }
    D->SetBoolField(TEXT("waitingToJoinFormation"),WaitingToJoinFormation(U));
    const auto It=R.Routes.find(Group->Id);D->SetBoolField(TEXT("hasRoute"),It!=R.Routes.end());
    const auto CurrentAdmission=R.PostAdmissionsV827.find(It!=R.Routes.end()?NextNode(It->second,*M):std::string());
    bool AdmissionWaiting=false;
    if(It!=R.Routes.end()&&InTransit(It->second)&&CurrentAdmission!=R.PostAdmissionsV827.end())
    {const auto& Admission=CurrentAdmission->second[Group->Team];
        D->SetNumberField(TEXT("receivingAdmissionOwner"),Admission.Owner);
        D->SetNumberField(TEXT("receivingAdmissionInheritedFromV87"),Admission.InheritedFromV87);
        const auto Position=std::find(Admission.Waiting.begin(),Admission.Waiting.end(),Group->Id);
        // An empty receiver with this section at the queue head is not a
        // legitimate traffic wait. Keep a self-lock visible to the unchanged
        // physical no-progress review instead of hiding it until timeout.
        AdmissionWaiting=Admission.Owner!=Group->Id&&(Admission.Owner!=INDEX_NONE
            ||(Position!=Admission.Waiting.end()&&Position!=Admission.Waiting.begin()));
        AdmissionWaiting|=It->second.TransitCorridorWaiting;
        D->SetNumberField(TEXT("receivingQueuePosition"),Position==Admission.Waiting.end()?-1:int32(Position-Admission.Waiting.begin()));}
    D->SetBoolField(TEXT("receivingAdmissionWaiting"),AdmissionWaiting);
    if(It==R.Routes.end())return D;const auto& Route=It->second;
    D->SetStringField(TEXT("currentNode"),Route.Current);D->SetStringField(TEXT("targetNode"),Route.Target);
    D->SetNumberField(TEXT("edgeIndex"),Route.Step);D->SetNumberField(TEXT("entryLane"),Route.EntryLane);
    D->SetStringField(TEXT("lastTransitRejection"),Route.LastTransitRejection);
    D->SetStringField(TEXT("lastTransitRejectedFrom"),Route.TransitRejectedFrom.ToString());D->SetStringField(TEXT("lastTransitRejectedTo"),Route.TransitRejectedTo.ToString());
    const std::string Next=NextNode(Route,*M);D->SetStringField(TEXT("nextNode"),wfexpue::Wide(Next));
    D->SetBoolField(TEXT("hostileIntermediateReceiverWaitingV87"),HostileIntermediateReceiverBlockedV87(R,*M,*Group,Next));
    D->SetBoolField(TEXT("transitCorridorWaiting"),Route.TransitCorridorWaiting);
    D->SetBoolField(TEXT("transitCheckpointFrozen"),Route.TransitCheckpointStep==Route.Step);
    D->SetStringField(TEXT("transitCheckpointOffset"),Route.TransitCheckpointOffset.ToString());
    const auto* N=Node(*M,Next);if(!N)return D;
    D->SetNumberField(TEXT("nodeRadius"),N->radius);D->SetNumberField(TEXT("distanceToNode"),FVector::Dist2D(U->Feet(),Vector(N->position)));
    const bool Transit=IsIntermediateTransitLeg(Route);D->SetBoolField(TEXT("intermediateTransit"),Transit);
    const auto Cells=R.Cells.find(Next);int Cell=INDEX_NONE;
    if(Cells!=R.Cells.end()){const auto Bay=Cells->second[Group->Team].find(Group->Id);if(Bay!=Cells->second[Group->Team].end())Cell=Bay->second;}
    D->SetNumberField(TEXT("reservedCell"),Cell);D->SetBoolField(TEXT("hasReservedCell"),Cell!=INDEX_NONE);
    const bool IngressWait=Route.Current.IsEmpty()&&Route.Target==TEXT("home")&&Group->Team==0&&Cell==INDEX_NONE;
    D->SetBoolField(TEXT("waitingForIngressBayV87"),IngressWait);
    D->SetBoolField(TEXT("mandatoryIngressParticipantV87"),R.State.objective.index<M->stages.size()
        &&MandatoryIngressSectionV87(R,M->stages[R.State.objective.index],*Group));
    if(IngressWait)D->SetStringField(TEXT("ingressWaitReasonV87"),TEXT("No free legal physical home bay; waiting in the actual field ingress lane."));
    TArray<TSharedPtr<FJsonValue>> BayOwners;
    bool AllIdleAutomatic=true;
    if(Cells!=R.Cells.end())for(const auto& Bay:Cells->second[Group->Team])
    {
        auto Owner=MakeShared<FJsonObject>();Owner->SetNumberField(TEXT("squad"),Bay.first);Owner->SetNumberField(TEXT("cell"),Bay.second);
        const auto* Occupant=B->FindSquad(Bay.first);Owner->SetNumberField(TEXT("living"),Occupant?Occupant->LivingCount():0);
        const bool Automatic=Occupant&&R.OrdinarySectionsV827.Contains(Occupant->Id)&&!Occupant->bHoldPosition;
        const bool Idle=Occupant&&!Occupant->bAdvancing&&FormationSettled(*Occupant,*N,Bay.second);
        const bool Mandatory=Occupant&&R.State.objective.index<M->stages.size()&&MandatoryIngressSectionV87(R,M->stages[R.State.objective.index],*Occupant);
        Owner->SetBoolField(TEXT("automatic"),Automatic);Owner->SetBoolField(TEXT("idle"),Idle);
        Owner->SetBoolField(TEXT("mandatoryParticipant"),Mandatory);
        Owner->SetBoolField(TEXT("held"),Occupant&&Occupant->bHoldPosition);Owner->SetBoolField(TEXT("advancing"),Occupant&&Occupant->bAdvancing);
        AllIdleAutomatic&=Automatic&&Idle&&!Mandatory;
        BayOwners.Add(MakeShared<FJsonValueObject>(Owner));
    }
    D->SetArrayField(TEXT("nextNodeBayOwners"),BayOwners);
    D->SetBoolField(TEXT("allIngressBayOwnersIdleAutomaticV87"),!BayOwners.IsEmpty()&&AllIdleAutomatic);
    if(!Transit&&Cell==INDEX_NONE)return D;
    FVector Goal;float Floor=0;
    if(Transit)
    {
        const auto Progress=Route.MemberWaypoints.find(U->MemberIndex);int Last=0;
        if(!TravelCheckpoint(B,*M,Route,U->MemberIndex,Progress==Route.MemberWaypoints.end()?1:Progress->second,Goal,Last))return D;
        D->SetStringField(TEXT("transitCheckpoint"),Goal.ToString());D->SetNumberField(TEXT("transitRetryAt"),Route.TransitRetryAt);
        D->SetNumberField(TEXT("transitCheckpointIndex"),Progress==Route.MemberWaypoints.end()?1:Progress->second);
        D->SetNumberField(TEXT("transitLastCheckpointIndex"),Last);
        D->SetBoolField(TEXT("actualFinalTransitCheckpointReached"),U->bFieldEntered&&Progress!=Route.MemberWaypoints.end()
            &&Progress->second>=Last&&FVector::Dist2D(U->Feet(),Goal)<=56.5);
    }
    else {Goal=FormationPoint(*N,Group->Team,Cell,U->MemberIndex);if(TryHeight(B,Goal.X,Goal.Y,Floor))Goal.Z=Floor;D->SetStringField(TEXT("formationGoal"),Goal.ToString());}
    D->SetNumberField(TEXT("distanceToGoal"),FVector::Dist2D(U->Feet(),Goal));
    D->SetBoolField(TEXT("directBodyLegClear"),LocalBodyLeg(U,U->Feet(),Goal));
    D->SetNumberField(TEXT("joinWaitRadiusCm"),N->radius+250.);D->SetNumberField(TEXT("localBodyBypassRadiusCm"),N->radius+450.);
    D->SetBoolField(TEXT("formationJoinWaiting"),!Transit&&FVector::Dist2D(U->Feet(),Vector(N->position))<=N->radius+250.&&FormationJoinWaiting(U,*N,Cell,R));
    float Nearest=MAX_flt;int NearestPerson=INDEX_NONE;
    for(const auto& Other:B->Units)if(Other!=U&&PhysicalFieldBodyV87(Other))
    {const float Gap=float(FVector::Dist2D(U->Feet(),Other->Feet()));if(Gap<Nearest){Nearest=Gap;NearestPerson=Other->SquadId*16+Other->MemberIndex;}}
    D->SetNumberField(TEXT("nearestLivingPerson"),NearestPerson);if(NearestPerson!=INDEX_NONE)D->SetNumberField(TEXT("nearestBodyGap"),Nearest);
    const auto Approach=R.Approaches.find(U->SquadId*16+U->MemberIndex);D->SetBoolField(TEXT("hasLocalApproach"),Approach!=R.Approaches.end());
    if(Approach!=R.Approaches.end())
    {
        const auto& A=Approach->second;D->SetNumberField(TEXT("waypointIndex"),A.Index);D->SetNumberField(TEXT("waypointCount"),A.Points.Num());
        D->SetNumberField(TEXT("stalledActualMovementAttemptSecondsV87"),A.StalledMovementAttemptsV87);
        D->SetNumberField(TEXT("canonicalBestRemainingV87"),A.CanonicalBestRemainingV87);
        D->SetNumberField(TEXT("canonicalNoProgressAttemptSecondsV87"),A.CanonicalNoProgressAttemptsV87);
        D->SetBoolField(TEXT("completeBodyGraphRequiredV87"),A.BodyGraphRequiredV87);
        D->SetBoolField(TEXT("registeredBendEndpointBoundV87"),A.BendEndpointBoundV87);
        if(A.BendEndpointBoundV87)D->SetStringField(TEXT("registeredBendEndpointV87"),A.BendEndpointV87.ToString());
        D->SetNumberField(TEXT("registeredBendFinalLegValidatedAtV87"),A.BendFinalLegValidatedAtV87);
        D->SetNumberField(TEXT("registeredBendArrivalRadiusV87"),A.BendArrivalRadiusV87);
        D->SetNumberField(TEXT("registeredBendStopDistanceV87"),A.BendStopDistanceV87);
        D->SetBoolField(TEXT("completeBodyGraphRouteV87"),A.PathKindV87==FWorld::FLocalApproach::EPathKindV87::CompleteBodyGraph);
        D->SetBoolField(TEXT("registeredNativeRejoinRouteV87"),A.PathKindV87==FWorld::FLocalApproach::EPathKindV87::RegisteredNativeRejoin);
        D->SetBoolField(TEXT("temporaryBodyPortalV87"),A.PathKindV87==FWorld::FLocalApproach::EPathKindV87::TemporaryPortal);
        D->SetBoolField(TEXT("finalJoinBodyPortal"),A.FinalJoinBodyPortal);
        D->SetNumberField(TEXT("progressAt"),A.ProgressAt);D->SetNumberField(TEXT("retryAt"),A.RetryAt);D->SetNumberField(TEXT("bestRemaining"),A.BestRemaining);
        D->SetStringField(TEXT("lastLocalLegRejection"),A.LastRejection);D->SetStringField(TEXT("lastRejectedFrom"),A.RejectedFrom.ToString());
        D->SetStringField(TEXT("lastRejectedTo"),A.RejectedTo.ToString());
        if(A.Points.IsValidIndex(A.Index)){D->SetStringField(TEXT("waypoint"),A.Points[A.Index].ToString());D->SetNumberField(TEXT("distanceToWaypoint"),FVector::Dist2D(U->Feet(),A.Points[A.Index]));}
    }
    const bool StalledApproach=R.Mission==TEXT("ru.galicia")&&Group->bAdvancing&&!Group->bHoldPosition&&!U->bEngaging
        &&Approach!=R.Approaches.end()&&Approach->second.Points.IsEmpty()
        &&FVector::Dist2D(U->Feet(),Goal)>FormationArrivalToleranceCm
        &&B->Elapsed-Approach->second.ProgressAt>=2.f;
    D->SetBoolField(TEXT("actualStalledLocalApproach"),StalledApproach);
    if(TestClearance||StalledApproach)
    {
        D->SetBoolField(TEXT("actualDirectNativeLegClear"),CanTraverse(B,U->Feet(),Goal));
        D->SetBoolField(TEXT("actualDirectCapsulePathClear"),AllowsStep(U,U->Feet(),Goal));
        if(Approach!=R.Approaches.end()&&Approach->second.Points.IsValidIndex(Approach->second.Index))
        {
            const FVector Waypoint=Approach->second.Points[Approach->second.Index];
            D->SetBoolField(TEXT("actualWaypointBodyLegClear"),LocalBodyLeg(U,U->Feet(),Waypoint));
            D->SetBoolField(TEXT("actualWaypointNativeLegClear"),CanTraverse(B,U->Feet(),Waypoint));
            D->SetBoolField(TEXT("actualWaypointCapsulePathClear"),AllowsStep(U,U->Feet(),Waypoint));
        }
        TArray<TSharedPtr<FJsonValue>> Bodies;
        for(const auto& Other:B->Units)if(Other!=U&&PhysicalFieldBodyV87(Other)
            &&FVector::Dist2D(Other->Feet(),Vector(N->position))<=N->radius+1000.)
        {
            auto Body=MakeShared<FJsonObject>();Body->SetNumberField(TEXT("person"),Other->SquadId*16+Other->MemberIndex);
            Body->SetNumberField(TEXT("side"),Other->Team);Body->SetStringField(TEXT("feet"),Other->Feet().ToString());
            Body->SetBoolField(TEXT("moving"),Other->bMoving);Body->SetBoolField(TEXT("engaging"),Other->bEngaging);
            Body->SetBoolField(TEXT("formationJoinWaiting"),WaitingToJoinFormation(Other));
            Bodies.Add(MakeShared<FJsonValueObject>(Body));
        }
        D->SetArrayField(TEXT("actualNearbyLivingBodies"),Bodies);
        TArray<TSharedPtr<FJsonValue>> BlockingBodies;
        const FVector From=U->Feet(),Delta=(Goal-From)*FVector(1,1,0);const double Length2=Delta.SizeSquared2D();
        for(const auto& Other:B->Units)if(Other!=U&&PhysicalFieldBodyV87(Other))
        {
            const FVector Feet=Other->Feet();const double Initial=FVector::Dist2D(From,Feet),Gap=BodySegmentGap(Feet,From,Goal);
            const bool Blocks=Initial<56.5
                ?FVector::DotProduct((From-Feet)*FVector(1,1,0),Goal-From)<0||FVector::Dist2D(Goal,Feet)<=Initial+.01
                :Gap<56.5;
            if(!Blocks)continue;
            auto Body=MakeShared<FJsonObject>();Body->SetNumberField(TEXT("person"),Other->SquadId*16+Other->MemberIndex);
            Body->SetNumberField(TEXT("side"),Other->Team);Body->SetStringField(TEXT("feet"),Feet.ToString());
            Body->SetNumberField(TEXT("initialGapCm"),Initial);Body->SetNumberField(TEXT("segmentGapCm"),Gap);
            Body->SetNumberField(TEXT("goalGapCm"),FVector::Dist2D(Goal,Feet));
            Body->SetNumberField(TEXT("segmentFraction"),Length2>.0001?FMath::Clamp(FVector::DotProduct((Feet-From)*FVector(1,1,0),Delta)/Length2,0.,1.):0.);
            BlockingBodies.Add(MakeShared<FJsonValueObject>(Body));
        }
        D->SetArrayField(TEXT("actualDirectBlockingBodies"),BlockingBodies);
        TArray<FVector> Centres;const bool HasContacts=GroundContactPath(B,From,Goal,Centres);
        D->SetBoolField(TEXT("actualDirectGroundContactPathValid"),HasContacts);
        if(HasContacts)
        {
            FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82CapsuleReadback),true,U);
            for(const auto& Other:B->Units)if(Other)Query.AddIgnoredActor(Other);
            for(int32 I=1;I<Centres.Num();++I)
            {
                FHitResult Hit;
                if(!U->GetWorld()->SweepSingleByChannel(Hit,Centres[I-1],Centres[I],FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),Query))continue;
                auto Block=MakeShared<FJsonObject>();Block->SetNumberField(TEXT("contactLegIndex"),I);
                Block->SetStringField(TEXT("fromCentre"),Centres[I-1].ToString());Block->SetStringField(TEXT("toCentre"),Centres[I].ToString());
                Block->SetStringField(TEXT("hitActor"),Hit.GetActor()?Hit.GetActor()->GetPathName():FString());
                Block->SetStringField(TEXT("hitComponent"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():FString());
                Block->SetStringField(TEXT("impact"),Hit.ImpactPoint.ToString());Block->SetStringField(TEXT("normal"),Hit.ImpactNormal.ToString());
                Block->SetNumberField(TEXT("hitFraction"),Hit.Time);Block->SetBoolField(TEXT("initialPenetration"),Hit.bStartPenetrating);
                D->SetObjectField(TEXT("actualDirectCapsuleFirstBlocker"),Block);break;
            }
        }
    }
    return D;
}
#endif
bool WaitingToJoinFormation(const AWarfareFrontlineSoldier* U)
{
    const auto* B=U?U->Battle.Get():nullptr;
    if(!B||!WarfareFRRUCampaignV82::Active(B)||U->Team<0||U->Team>1)return false;
    const auto* Group=B->FindSquad(U->SquadId);const auto* M=Definition(B);
    const auto* Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B)));
    if(!Group||!M||!Found||!SameDeployment(B,**Found)||Group->bHoldPosition||!Group->bAdvancing)return false;
    const auto& R=**Found;const auto Route=R.Routes.find(Group->Id);
    if(Route==R.Routes.end()||!InTransit(Route->second)||IsIntermediateTransitLeg(Route->second))return false;
    const std::string Next=NextNode(Route->second,*M);const auto* N=Node(*M,Next);
    const auto Cells=R.Cells.find(Next);if(!N||Cells==R.Cells.end())return false;
    const auto Bay=Cells->second[Group->Team].find(Group->Id);if(Bay==Cells->second[Group->Team].end())return false;
    // A planted final bay still supports ordinary deployment. This is
    // only the existing join pause before this man reaches his real slot.
    return FVector::Dist2D(U->Feet(),Vector(N->position))<=N->radius+250.
        &&FVector::Dist2D(U->Feet(),FormationPoint(*N,Group->Team,Bay->second,U->MemberIndex))>10.
        &&FormationJoinWaiting(U,*N,Bay->second,R);
}
bool ShouldFinishFriendlyPostEntry(const AWarfareFrontlineSoldier* U)
{
    const auto* B=U?U->Battle.Get():nullptr;
    if(!B||!WarfareFRRUCampaignV82::Active(B)||U->Team<0||U->Team>1)return false;
    const auto* Group=B->FindSquad(U->SquadId);const auto* M=Definition(B);
    const auto* Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B)));
    if(!Group||!M||!Found||!SameDeployment(B,**Found)||!Group->bAdvancing||Group->bHoldPosition)return false;
    const auto& R=**Found;const auto Path=R.Routes.find(Group->Id);
    if(Path==R.Routes.end()||!InTransit(Path->second)||IsIntermediateTransitLeg(Path->second))return false;
    const std::string Next=NextNode(Path->second,*M);const auto* N=Node(*M,Next);
    const auto Owner=R.Owners.find(Next);const auto Cells=R.Cells.find(Next);
    if(!N||Owner==R.Owners.end()||Owner->second!=Group->Team||Cells==R.Cells.end())return false;
    const auto Bay=Cells->second[Group->Team].find(Group->Id);
    if(Bay==Cells->second[Group->Team].end()
        ||FVector::Dist2D(U->Feet(),FormationPoint(*N,Group->Team,Bay->second,U->MemberIndex))>430.)return false;
    // Never suppress genuine close contact or entry into an opposed post.
    // This reads native ownership and reservation; it grants no arrival,
    // changes no route, and all ordinary floor/body checks still run.
    for(const AWarfareFrontlineSoldier* Other:B->Units)
        if(IsValid(Other)&&Other->Team!=U->Team&&Other->IsActiveCombatant()
            &&(FVector::DistSquared2D(Other->Feet(),U->Feet())<FMath::Square(600.f)
                ||FVector::DistSquared2D(Other->Feet(),Vector(N->position))<FMath::Square(float(N->radius))))return false;
    return true;
}
bool Destination(const AWarfareFrontlineSoldier* U,FVector& Out)
{
    if(!U||!U->Battle||!WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* Group=U->Battle->FindSquad(U->SquadId);if(!Group){Out=U->Feet();return true;}
    auto& R=WorldFor(U->Battle);const auto* M=Definition(U->Battle);const auto I=R.Routes.find(Group->Id);
    if(const auto Local=R.Approaches.find(U->SquadId*16+U->MemberIndex);Local!=R.Approaches.end())
        Local->second.BendFinalLegValidatedAtV87=-1;
    if(!M||I==R.Routes.end()||Group->bHoldPosition){R.Approaches.erase(U->SquadId*16+U->MemberIndex);Out=U->Feet();return true;}
    auto& Route=I->second;const std::string Next=NextNode(Route,*M);
    const auto* N=Node(*M,Next);if(!N){Out=U->Feet();return true;}
    const auto NodeCells=R.Cells.find(Next);const bool Transit=IsIntermediateTransitLeg(Route);
    const bool HasCell=NodeCells!=R.Cells.end()&&NodeCells->second[Group->Team].count(Group->Id);
    // One living pioneer may service a disabled friendly vehicle locally from
    // his existing post. This is a physical guide, never a node arrival/order;
    // manual holds, in-transit sections and current clearance work keep priority.
    if(!InTransit(Route)&&!Group->bAdvancing&&HasCell&&Group->Role==EFrontlineRole::Pioneer
        &&R.State.objective.index<M->stages.size()&&M->stages[R.State.objective.index].requiredRole!=int32(EFrontlineRole::Pioneer)
        &&!FrozenHold(R,*M,*Group,std::string())
        &&std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Group->Id)==R.State.objective.holdParticipants.end())
    {
        const AWarfareFrontlineSoldier* Worker=nullptr;
        for(const auto& Weak:Group->Members)if(const auto* Person=Weak.Get();Person&&Person->IsAlive()&&(!Worker||Person->MemberIndex<Worker->MemberIndex))Worker=Person;
        FVector Goal;
        if(Worker==U&&WarfareFRRUArmourV82::RepairDestinationV87(U,Goal)
            &&FVector::Dist2D(Goal,Vector(N->position))<=N->radius+850.
            &&FVector::Dist2D(U->Feet(),Vector(N->position))<=N->radius+850.)
        {
            const int32 Cell=NodeCells->second[Group->Team].at(Group->Id);
            if(LocalApproachDestination(U,*N,Cell,R,Goal,Out,true))return true;
        }
    }
    if(InTransit(Route)&&Group->bAdvancing&&(Transit||HasCell)&&!AdmitPostV827(U->Battle,*M,R,*Group,Route,Next))
    {
        const bool Leaving=LeavingOwnedPostV827(U->Battle,*M,R,*Group,Next)
            &&FVector::Dist2D(U->Feet(),Vector(N->position))>N->radius+56.5;
        if(!Leaving)
        {
            R.Approaches.erase(U->SquadId*16+U->MemberIndex);
            // Reinforcement visibility does not depend on a free receiving
            // corridor. Enter normally into the independent rear waiting lane.
            if(Route.Current.IsEmpty()&&Route.EntryLane!=INDEX_NONE&&!U->bFieldEntered)
            {const auto* G=Geometry(U->Battle);Out=FVector(Group->Team==0?G->minX+300:G->maxX-300,(Route.EntryLane-15)*330+(U->MemberIndex/2-1)*72,0);
                Out.X+=(U->MemberIndex%2?36:-36)*U->Battle->GetTeamDirection(Group->Team);float Floor=0;if(TryHeight(U->Battle,Out.X,Out.Y,Floor))Out.Z=Floor;}
            else Out=U->Feet();
            return true;
        }
    }
    if(!Transit&&!HasCell)
    {
        R.Approaches.erase(U->SquadId*16+U->MemberIndex);
        // Reinforcements enter the visible field in about three seconds even
        // when their receiving post is full, then wait in a separate rear lane.
        if(Route.Current.IsEmpty()&&Route.EntryLane!=INDEX_NONE)
        {const auto* G=Geometry(U->Battle);Out=FVector(Group->Team==0?G->minX+300:G->maxX-300,(Route.EntryLane-15)*330+(U->MemberIndex/2-1)*72,0);Out.X+=(U->MemberIndex%2?36:-36)*U->Battle->GetTeamDirection(Group->Team);}
        else{Out=U->Feet();return true;}
    }
    else
    {
        if(Transit&&!RouteDepartureAllowed(U->Battle,*M,R,*Group,Route)){Out=U->Feet();return true;}
        const int Cell=Transit?INDEX_NONE:NodeCells->second[Group->Team].at(Group->Id);
        if(Route.CarrierDelivery)
        {
            int32& Progress=Route.MemberWaypoints[U->MemberIndex];
            if(Progress==0)
            {
                FVector Final=FormationPoint(*N,Group->Team,Cell,U->MemberIndex);float Floor=0;
                if(!TryHeight(U->Battle,Final.X,Final.Y,Floor)){Out=U->Feet();return true;}Final.Z=Floor;
                if(FVector::Dist2D(U->Feet(),Route.CarrierDeliveryCorner)<=56.5&&AllowsStep(U,U->Feet(),Final))
                {Progress=1;R.Approaches.erase(U->SquadId*16+U->MemberIndex);}
                else
                {
                    Out=Route.CarrierDeliveryCorner;
                    const FVector Goal=Out;LocalApproachDestination(U,*N,Cell,R,Goal,Out,true);return true;
                }
            }
        }
        if(Route.Step<int32(Route.Edges.size()))
        {
            const auto* G=Geometry(U->Battle);
            const auto* Physical=G?wfexp::scene::FindRoute(*G,Route.Edges[Route.Step]):nullptr;
            if(Transit||(Physical&&Physical->points.size()>2))
            {
                int32& Progress=Route.MemberWaypoints[U->MemberIndex];Progress=FMath::Max(Progress,1);
                FVector Waypoint;int Last=0;
                if(!TravelCheckpoint(U->Battle,*M,Route,U->MemberIndex,Progress,Waypoint,Last)){Out=U->Feet();return true;}
                while(Progress<Last)
                {
                    // A travel bend is a clearance-sized guide, not an exact
                    // occupied formation slot. Requiring a smaller disk than
                    // the two-body clearance can put both men's turn targets
                    // inside each other's bodies. The outgoing leg still
                    // uses the bounded real-body/native/capsule bypass below.
                    if(FVector::Dist2D(U->Feet(),Waypoint)<=56.5)
                    {
                        ++Progress;
#if WITH_EDITOR
                        if(const auto Bend=R.ActualBendBypassesV87.find(U->SquadId*16+U->MemberIndex);Bend!=R.ActualBendBypassesV87.end())
                        {
                            auto& Proof=Bend->second;
                            if(Proof.AdvancedAt<0&&Proof.EdgeStep==Route.Step&&Proof.Checkpoint==Progress-1
                                &&Proof.Node==wfexpue::Wide(N->id)&&Proof.Bend.Equals(Waypoint,.01))
                            {
                                Proof.AdvancedAt=U->Battle->Elapsed;Proof.AdvancedTo=Progress;Proof.AdvancedFeet=U->Feet();
                                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_BEND_BYPASS_ACTUAL_PROGRESS squad=%d member=%d edge=%d P=%d nextP=%d host=%.3f actual_feet=(%s) original_bend=(%s) actual_distance=%.3f unchanged_arrival_limit=56.5"),
                                    U->SquadId,U->MemberIndex,Route.Step,Proof.Checkpoint,Progress,Proof.AdvancedAt,*Proof.AdvancedFeet.ToString(),*Proof.Bend.ToString(),FVector::Dist2D(Proof.AdvancedFeet,Proof.Bend));
                            }
                        }
#endif
                        if(!TravelCheckpoint(U->Battle,*M,Route,U->MemberIndex,Progress,Waypoint,Last)){Out=U->Feet();return true;}continue;
                    }
                    // Authored bends are physical travel, not intermediate
                    // capture objectives. Each man keeps his own progress.
                    Out=Waypoint;float Height=0;if(TryHeight(U->Battle,Out.X,Out.Y,Height))Out.Z=Height;
                    // A registered route bend needs the same bounded body
                    // bypass as the final bay. Returning the bend directly
                    // trapped middle ranks outside the node's join guide;
                    // their waiting squad then held the whole ingress queue.
                    // Keep this exact checkpoint, physical floor/capsules and
                    // native segment checks; only route around real bodies.
                    const FVector Goal=Out;LocalApproachDestination(U,*N,Cell,R,Goal,Out,true);
                    return true;
                }
                if(Transit)
                {
                    // An occupied intermediate post is a travel checkpoint,
                    // not a requested formation. Every living man must reach
                    // his real endpoint before UpdateSquads changes the edge.
                    // A reached rank must not freeze short across an unarrived
                    // neighbour's slot. While the section is incomplete, finish
                    // a clear straight leg toward his own normal 10 cm target.
                    // Never circle a reached rank or weaken the 56.5 cm proof.
                    if(U->bFieldEntered&&Progress>=Last&&FVector::Dist2D(U->Feet(),Waypoint)<=56.5)
                    {
                        bool AllReached=true;
                        for(const auto& Weak:Group->Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
                        {
                            FVector Final;int MemberLast=0;const auto At=Route.MemberWaypoints.find(Member->MemberIndex);
                            if(!Member->bFieldEntered||!TravelCheckpoint(U->Battle,*M,Route,Member->MemberIndex,128,Final,MemberLast)
                                ||At==Route.MemberWaypoints.end()||At->second<MemberLast||FVector::Dist2D(Member->Feet(),Final)>56.5){AllReached=false;break;}
                        }
                        R.Approaches.erase(U->SquadId*16+U->MemberIndex);
                        Out=!AllReached&&FVector::Dist2D(U->Feet(),Waypoint)>10.&&LocalBodyLeg(U,U->Feet(),Waypoint)
                            &&CanTraverse(U->Battle,U->Feet(),Waypoint)&&AllowsStep(U,U->Feet(),Waypoint)?Waypoint:U->Feet();
                        return true;
                    }
                    Out=Waypoint;const FVector Goal=Out;LocalApproachDestination(U,*N,INDEX_NONE,R,Goal,Out,true);return true;
                }
            }
        }
        Out=FormationPoint(*N,Group->Team,Cell,U->MemberIndex);
        float H=0;if(TryHeight(U->Battle,Out.X,Out.Y,H))Out.Z=H;
        // Every registered last leg can meet bodies before the post's join
        // domain, including a withdrawal to an already reserved rear bay.
        // Execution has one steering owner and cannot invent a second detour
        // after this guide returns its direct goal. Keep the existing bounded
        // transit guide outside the join domain, then ordinary ranked joining
        // inside it. Admission, real bay arrival and all physical legs retain
        // their existing checks; this does not grant arrival at the post.
        const bool ApproachingFinalPostV87=InTransit(Route)&&Group->bAdvancing&&!Transit&&HasCell
            &&FVector::Dist2D(U->Feet(),Vector(N->position))>N->radius+450.;
#if WITH_EDITOR
        const bool TraceFinalPostApproachV87=ApproachingFinalPostV87&&!R.Approaches.count(U->SquadId*16+U->MemberIndex)
            &&U->Battle->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))&&CandidateReview();
#endif
        const FVector Goal=Out;LocalApproachDestination(U,*N,Cell,R,Goal,Out,ApproachingFinalPostV87);
#if WITH_EDITOR
        if(TraceFinalPostApproachV87&&R.Approaches.count(U->SquadId*16+U->MemberIndex))
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_FINAL_POST_APPROACH_GUIDE squad=%d member=%d node=%s host=%.3f actual_feet=(%s) canonical_bay=(%s) checked_guide=(%s) node_distance=%.3f unchanged_join_domain=%.3f"),
                U->SquadId,U->MemberIndex,*wfexpue::Wide(N->id),U->Battle->Elapsed,*U->Feet().ToString(),*Goal.ToString(),*Out.ToString(),
                FVector::Dist2D(U->Feet(),Vector(N->position)),N->radius+450.);
#endif
        return true;
    }
    float H=0;if(TryHeight(U->Battle,Out.X,Out.Y,H))Out.Z=H;return true;
}
static bool ParkTransitAtReceiverV87(AWarfareFrontlineGameMode* B,FWorld& R,const FFrontlineSquad& Group,
    FRoute& Path,const wfexp::tactical::NodeDefinition& Receiver,bool PreserveWaypoints,const TCHAR* Reason)
{
    const auto Owner=R.Owners.find(Receiver.id);auto ParkingCells=R.Cells;
    if((Owner!=R.Owners.end()&&Owner->second==1-Group.Team)
        ||!ReserveCell(B,Receiver,Group.Team,Group.Id,ParkingCells))return false;
    const FString FormerTarget=Path.Target;
    R.Cells=std::move(ParkingCells);Path.Target=wfexpue::Wide(Receiver.id);
    Path.Edges.resize(size_t(Path.Step)+1);if(!PreserveWaypoints)Path.MemberWaypoints.clear();
    Path.TransitCheckpointStep=INDEX_NONE;Path.TransitCheckpointOffset=FVector::ZeroVector;
    Path.TransitCorridorRetryAt=0;Path.TransitCorridorWaiting=false;Path.TransitRetryAt=0;
    auto& Admission=R.PostAdmissionsV827[Receiver.id][Group.Team];
    // This only books an ordinary final join. The source's real occupied bay
    // remains until its last living feet leave; no arrival is issued here.
    if(Admission.Owner==Group.Id){Admission.Transit=false;Admission.InheritedFromV87=INDEX_NONE;}
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ROUTE_PARK_%s mission=%s squad=%d side=%d post=%s former_target=%s bay=%d host=%.3f"),
        Reason,*R.Mission,Group.Id,Group.Team,*Path.Target,*FormerTarget,R.Cells.at(Receiver.id)[Group.Team].at(Group.Id),B->Elapsed);
    return true;
}
static bool ParkStaleAutomaticRouteAtSourceV87(AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    FWorld& R,FFrontlineSquad& Group,FRoute& Path,const TCHAR*& Rejection)
{
    if(B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B)
        ||R.State.objective.finished||R.State.objective.index>=M.stages.size()||Group.Team!=R.State.playerSide
        ||!Group.bAdvancing||Group.bHoldPosition||!IsIntermediateTransitLeg(Path)||Path.CarrierDelivery
        ||!R.OrdinarySectionsV827.Contains(Group.Id)||R.HeldOrdinarySectionsV827.Contains(Group.Id)
        ||ReservedByTacticsV827(B,R,Group.Id)
        ||std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Group.Id)!=R.State.objective.holdParticipants.end()
        ||std::find(R.State.objective.withdrawalParticipants.begin(),R.State.objective.withdrawalParticipants.end(),Group.Id)!=R.State.objective.withdrawalParticipants.end()
        ||std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Group.Id)!=R.State.objective.fallbackWorkers.end()
        ||!HostileIntermediateReceiverBlockedV87(R,M,Group,NextNode(Path,M)))return false;
    const auto* Source=Node(M,wfexpue::Narrow(Path.Current));
    const auto Owner=Source?R.Owners.find(Source->id):R.Owners.end();
    // Ownership can change after the previous checkpoint was passed. Only
    // feet still entirely inside the current friendly post may settle there;
    // a section already out on its next leg must retain that physical route.
    if(!Source||Owner==R.Owners.end()||Owner->second!=Group.Team||!AtNode(Group,*Source,true))return false;
    double MaximumSourceDistance=0.;
    for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
    {
        if(Member->bEngaging)return false;
        MaximumSourceDistance=FMath::Max(MaximumSourceDistance,FVector::Dist2D(Member->Feet(),Vector(Source->position)));
    }
    auto ParkingCells=R.Cells;
    if(!ReserveCell(B,*Source,Group.Team,Group.Id,ParkingCells))
    {Rejection=TEXT("registered_current_route_blocked_source_bay_unavailable");return false;}
    const FString FormerTarget=Path.Target;const int32 FormerStep=Path.Step;
    FRoute Parking;Parking.Current=Path.Current;Parking.Target=Path.Current;Parking.ArrivalSequence=Path.ArrivalSequence;
    R.Cells=std::move(ParkingCells);ReleaseGroupAdmissionsV827(R,Group.Id);
    for(auto It=R.Approaches.begin();It!=R.Approaches.end();)if(It->first/16==Group.Id)It=R.Approaches.erase(It);else ++It;
    Path=std::move(Parking);R.OrderPreview.At=-1;
    // Remain advancing until the unchanged final formation/body/ground join
    // actually settles. The ordinary planner can then issue a fresh public
    // move; no arrival sequence, native receipt or post ownership is granted.
    Group.bAdvancing=true;
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ROUTE_PARK_STALE_SOURCE mission=%s squad=%d side=%d post=%s former_target=%s former_step=%d bay=%d host=%.3f maximum_source_distance=%.3f source_radius=%.3f arrival_granted=false"),
        *R.Mission,Group.Id,Group.Team,*Path.Current,*FormerTarget,FormerStep,R.Cells.at(Source->id)[Group.Team].at(Group.Id),B->Elapsed,MaximumSourceDistance,Source->radius);
    return true;
}
static bool ParkReciprocalAutomaticTransitV87(AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    FWorld& R,const FFrontlineSquad& Group,FRoute& Path)
{
    if(B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B)
        ||R.State.objective.finished||R.State.objective.index>=M.stages.size()||Group.Team!=R.State.playerSide
        ||!Group.bAdvancing||Group.bHoldPosition||!IsIntermediateTransitLeg(Path)||!Path.TransitCorridorWaiting
        ||Path.TransitCheckpointStep==Path.Step||Path.CarrierDelivery)return false;
    const auto& Stage=M.stages[R.State.objective.index];
    const auto Protected=[&](const FFrontlineSquad& Section)
    {return !R.OrdinarySectionsV827.Contains(Section.Id)||R.HeldOrdinarySectionsV827.Contains(Section.Id)
        ||ReservedByTacticsV827(B,R,Section.Id)
        ||std::find(R.State.objective.holdParticipants.begin(),R.State.objective.holdParticipants.end(),Section.Id)!=R.State.objective.holdParticipants.end()
        ||std::find(R.State.objective.withdrawalParticipants.begin(),R.State.objective.withdrawalParticipants.end(),Section.Id)!=R.State.objective.withdrawalParticipants.end()
        ||std::find(R.State.objective.fallbackWorkers.begin(),R.State.objective.fallbackWorkers.end(),Section.Id)!=R.State.objective.fallbackWorkers.end();};
    if(Protected(Group))return false;
    const auto* Source=Node(M,wfexpue::Narrow(Path.Current));const auto Next=NextNode(Path,M);const auto* Receiver=Node(M,Next);
    const auto Owner=R.Owners.find(Next);const auto SourceCells=Source?R.Cells.find(Source->id):R.Cells.end();
    if(!Source||!Receiver||Source->id==Receiver->id||Owner==R.Owners.end()||Owner->second!=Group.Team
        ||std::find(Stage.mustOwn.begin(),Stage.mustOwn.end(),Source->id)!=Stage.mustOwn.end()
        ||BodyInPostV827(Group,*Receiver))return false;
    const bool OwnSourceBay=SourceCells!=R.Cells.end()&&SourceCells->second[Group.Team].count(Group.Id);
    const bool SettledAtSource=OwnSourceBay&&AtNode(Group,*Source,true)
        &&FormationSettled(Group,*Source,SourceCells->second[Group.Team].at(Group.Id));
    // A reciprocal wait can form after both sections have left their source
    // bays. An already vacated bay must not become a prerequisite for parking
    // at the same registered receiver. Require the original physical footprint
    // to be entirely clear and its actual bay already released; keep every
    // passed checkpoint and let ordinary body/ground joining prove arrival.
    const bool DepartedSource=!OwnSourceBay&&!BodyInPostV827(Group,*Source);
    if(!SettledAtSource&&!DepartedSource)return false;
    for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
        if(Member->bEngaging||(DepartedSource&&(!Member->bFieldEntered||Member->bMoving||Member->GetMotionSpeed()>.5f)))return false;
    const auto Head=[&](const std::string& Post,int32 Id)
    {
        const auto Found=R.PostAdmissionsV827.find(Post);if(Found==R.PostAdmissionsV827.end())return false;
        const auto& Admission=Found->second[Group.Team];
        return Admission.Owner==INDEX_NONE&&!Admission.Waiting.empty()&&Admission.Waiting.front()==Id;
    };
    if(!Head(Next,Group.Id))return false;
    for(const auto& Other:B->Squads)if(Other.Id!=Group.Id&&Other.Team==Group.Team&&Other.LivingCount()>0
        &&Other.bAdvancing&&!Other.bHoldPosition&&!Protected(Other))
    {
        const auto Reverse=R.Routes.find(Other.Id);if(Reverse==R.Routes.end())continue;const auto& Opposite=Reverse->second;
        if(!IsIntermediateTransitLeg(Opposite)||Opposite.Current!=wfexpue::Wide(Next)||NextNode(Opposite,M)!=Source->id
            ||!Opposite.TransitCorridorWaiting||Opposite.TransitCheckpointStep==Opposite.Step||Opposite.CarrierDelivery
            ||!Head(Source->id,Other.Id)||BodyInPostV827(Other,*Source))continue;
        bool Still=true,BlocksReceivingLeg=false;
        for(const auto& Weak:Other.Members)if(const auto* Peer=Weak.Get();Peer&&Peer->IsAlive())
        {
            if(!Peer->bFieldEntered||Peer->bMoving||Peer->GetMotionSpeed()>.5f||Peer->bEngaging){Still=false;break;}
            // The reverse queue must have real bodies on this receiving leg,
            // not merely a distant destination or a future slot reservation.
            for(const auto& Own:Group.Members)if(const auto* Member=Own.Get();Member&&Member->IsAlive())
            {
                FVector Goal,Previous;int Last=0;
                if(!TravelCheckpoint(B,M,Path,Member->MemberIndex,128,Goal,Last))continue;
                if(Last==1)Previous=Member->Feet();
                else if(!TravelCheckpoint(B,M,Path,Member->MemberIndex,Last-1,Previous,Last))continue;
                if(BodySegmentGap(Peer->Feet(),Previous,Goal)<56.5)BlocksReceivingLeg=true;
            }
        }
        if(!Still||!BlocksReceivingLeg)continue;
        // Preserve every already passed edge/checkpoint. Only shorten this
        // automatic promise to its same existing receiver when a real bay is
        // available there; final joining executes all ordinary physical gates.
        if(ParkTransitAtReceiverV87(B,R,Group,Path,*Receiver,true,TEXT("RECIPROCAL_WAIT")))return true;
    }
    return false;
}
bool UpdateSquads(AWarfareFrontlineGameMode* B)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return false;auto& R=WorldFor(B);const auto* M=Definition(B);if(!M)return true;
    ReleaseVacatedCells(B,*M,R);PrunePostAdmissionsV827(B,*M,R);
    for(auto& Group:B->Squads)if(Group.LivingCount()>0)
    {
        auto Route=R.Routes.find(Group.Id);if(Route==R.Routes.end()||Group.bHoldPosition)continue;auto& Path=Route->second;
        if(IsIntermediateTransitLeg(Path))
        {
            if(B->Elapsed<Path.TransitRetryAt)continue;
            const auto RejectTransit=[&](const TCHAR* Reason,const FRoute& Leg)
            {
                Path.LastTransitRejection=Reason;Path.TransitRetryAt=B->Elapsed+.35f;
                const auto* A=Node(*M,wfexpue::Narrow(Leg.Current));const auto* Z=Node(*M,NextNode(Leg,*M));
                Path.TransitRejectedFrom=A?Vector(A->position):FVector::ZeroVector;Path.TransitRejectedTo=Z?Vector(Z->position):FVector::ZeroVector;
            };
            if(!RouteDepartureAllowed(B,*M,R,Group,Path))
            {
                const TCHAR* ParkingRejection=nullptr;
                if(ParkStaleAutomaticRouteAtSourceV87(B,*M,R,Group,Path,ParkingRejection))continue;
                RejectTransit(ParkingRejection?ParkingRejection:TEXT("registered_current_route_blocked"),Path);continue;
            }
            if(ParkReciprocalAutomaticTransitV87(B,*M,R,Group,Path))continue;
            bool Reached=true;
            for(const auto& Weak:Group.Members)if(const auto* Member=Weak.Get();Member&&Member->IsAlive())
            {
                FVector Checkpoint;int Last=0;const auto Progress=Path.MemberWaypoints.find(Member->MemberIndex);
                if(!Member->bFieldEntered||!TravelCheckpoint(B,*M,Path,Member->MemberIndex,128,Checkpoint,Last)
                    ||Progress==Path.MemberWaypoints.end()||Progress->second<Last
                    ||FVector::Dist2D(Member->Feet(),Checkpoint)>56.5){Reached=false;break;}
            }
            if(!Reached)continue;
            const std::string Next=NextNode(Path,*M);FRoute Outgoing=Path;
            Outgoing.Current=wfexpue::Wide(Next);++Outgoing.Step;Outgoing.MemberWaypoints.clear();
            if(!RouteDepartureAllowed(B,*M,R,Group,Outgoing))
            {
                RejectTransit(TEXT("registered_outgoing_route_blocked"),Outgoing);
                // Finishing this actual leg must not depend on permission to
                // depart on a later one. An intermediate post may have changed
                // hands while these men were walking. Park at the friendly
                // receiver through its normal, physically checked final bay;
                // otherwise this completed corridor seals every following
                // section even when the receiver has an available formation.
                const auto* Receiver=Node(*M,Next);
                if(Receiver)ParkTransitAtReceiverV87(B,R,Group,Path,*Receiver,false,TEXT("BLOCKED_OUTGOING"));
                continue;
            }
            const auto* Following=Node(*M,NextNode(Outgoing,*M));if(!Following)continue;
            const bool FollowingTransit=IsIntermediateTransitLeg(Outgoing);
            if(!FollowingTransit)
            {
                // The final post still requires its own free, physically
                // checked bay. A full terminal destination is ordinary wait.
                if(!ReserveCell(B,*Following,Group.Team,Group.Id,R.Cells)){RejectTransit(TEXT("terminal_formation_bay_full_or_invalid"),Outgoing);continue;}
            }
            // The entire squad has physically reached this checkpoint. Do
            // not require every rank's future leg to be clear at once: rear
            // ranks legitimately wait for the front ranks to move. Destination
            // keeps each exact guide and bounded body bypass; every actual
            // step still checks the unchanged native floor and swept capsule.
            Path.Current=Outgoing.Current;Path.Step=Outgoing.Step;Path.MemberWaypoints.clear();
            Path.TransitRetryAt=0;
            // Transit is physical progress only: no bay, arrival proof, node
            // capture, or ArriveTrench voice is granted at this checkpoint.
            continue;
        }
        if(const auto* Next=Node(*M,NextNode(Path,*M)))ReserveCell(B,*Next,Group.Team,Group.Id,R.Cells);
        const auto* Occupied=OccupiedNode(Group,*M);if(!Occupied||!AtNode(Group,*Occupied,true))continue;
        const auto Bays=R.Cells.find(Occupied->id);
        if(Bays==R.Cells.end()||!Bays->second[Group.Team].count(Group.Id)
            ||!FormationSettled(Group,*Occupied,Bays->second[Group.Team].at(Group.Id)))continue;
        if(Path.Edges.empty())
        {if(wfexpue::Wide(Occupied->id)==Path.Target){Path.Current=Path.Target;Group.bAdvancing=false;Group.bReinforcement=false;}continue;}
        if(Path.Step>=int32(Path.Edges.size()))continue;
        const auto* Link=Edge(*M,Path.Edges[Path.Step]);const std::string Next=Link&&Link->from==wfexpue::Narrow(Path.Current)?Link->to:Link?Link->from:std::string();
        // Recheck the next segment at its real departure point. Ownership may
        // change while the preceding leg is being walked; a stopped route is
        // cancelled here rather than bypassing the contested intermediate post.
        if(wfexpue::Wide(Occupied->id)==Path.Current)
        {
            const bool Valid=RouteDepartureAllowed(B,*M,R,Group,Path);
            if(!Valid){Path.Edges.clear();Path.Step=0;Path.Target=Path.Current;Group.bAdvancing=false;Group.bHoldPosition=true;R.Notice=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?TEXT("The route is blocked. The section is holding its last position."):TEXT("前路受阻。分队已停在上一处阵地。");continue;}
        }
        if(Occupied->id!=Next)continue;
        Path.Current=wfexpue::Wide(Next);++Path.Step;++Path.ArrivalSequence;Path.MemberWaypoints.clear();
        auto& Receipt=R.Arrivals[Group.Id];Receipt={"v82.arrival."+std::to_string(Group.Id)+"."+std::to_string(Path.ArrivalSequence),R.State.encounterId,Link->id,Group.Id,Path.ArrivalSequence,B->Elapsed};
        if(Path.Step==int32(Path.Edges.size()))
        {
            Group.bAdvancing=false;Group.bReinforcement=false;Path.Target=Path.Current;
            if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayVoice(EFrontlineVoiceEvent::ArriveTrench,
                WarfareFRRUCampaignV82::Nation(B,Group.Team)==EFRRUNationV81::German,Group.Centre(),Group.Id);
        }
    }
    for(const auto& N:M->nodes)
    {
        bool Here[2]={};const auto Bays=R.Cells.find(N.id);
        for(const auto& Group:B->Squads)if(Group.Team>=0&&Group.Team<2&&Group.LivingCount()>0&&AtNode(Group,N,true)&&Bays!=R.Cells.end())
        {
            const auto Path=R.Routes.find(Group.Id);const auto Bay=Bays->second[Group.Team].find(Group.Id);
            if((Path==R.Routes.end()||!InTransit(Path->second))&&Bay!=Bays->second[Group.Team].end()&&FormationSettled(Group,N,Bay->second))Here[Group.Team]=true;
        }
        if(Here[0]!=Here[1])
        {
            const int Previous=R.Owners.count(N.id)?R.Owners[N.id]:-1;const int Owner=Here[0]?0:1;R.Owners[N.id]=Owner;
            if(Previous!=Owner)
            {
                FrontlineMoraleV36::OnCaptureSector(B,wfexpue::Wide(N.id),Previous,Owner);
                if(N.id=="objective"&&Previous==1&&Owner==0)
                    FrontlineAchievementsV76::EnemyTrenchCaptured(B,Previous,Owner);
            }
        }
    }
    return true;
}
FVector MovementStep(const AWarfareFrontlineSoldier* U,const FVector& Proposed,float Distance)
{
    if(!U||!U->Battle)return FVector::ZeroVector;const FVector From=U->Feet();const auto* G=Geometry(U->Battle);
    if(!G||Proposed.ContainsNaN()||!FMath::IsFinite(Distance))return From;
    FVector Delta=Proposed-From;Delta.Z=0;const double Span=Delta.Size();if(Span<=.001||Distance<=0)return From;
    FVector Next=From+Delta/Span*FMath::Min3(float(Span),Distance,80.f);float Height=0;
    // Destination already owns the registered checkpoint/local body portal.
    // Execute that exact leg once. A second avoidance planner used a smaller
    // radius and could steer back inside this planner's margin indefinitely.
    // Existing overlap may separate monotonically; no body/capsule is ignored.
    if(!CanTraverse(U->Battle,From,Next)||!LocalBodyLeg(U,From,Next)
        ||!AllowsStep(U,From,Next)||!TryHeight(U->Battle,Next.X,Next.Y,Height))return From;
    Next.Z=Height;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(const AFrontlineTank* Tank:Fleet->GetVehicles())
        if(IsValid(Tank)&&Tank->IsOperational()&&!Tank->IsHidden()
            &&FVector::DistSquared2D(From,Tank->GetActorLocation())<FMath::Square(3600.f)
            &&Tank->InfantrySweepClearanceV8218(From,Next,34.f,U)<1.f)return From;
    return Next;
}
void ResolvedStrike(AWarfareFrontlineGameMode* B,const FFrontlineStrike& Strike)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B)||!Strike.bResolved)return;auto& R=WorldFor(B);if(!SameDeployment(B,R)||R.Finished)return;
    const auto Plan=R.FirePlans.find(Strike.FireMissionId);if(Plan==R.FirePlans.end())return;
    wfexp::tactical::SupportReceipt Receipt;bool Emitted=false;
    const auto Result=R.Journal.Resolve(Plan->second,R.State.encounterId,Strike.Team,Strike.FireMissionId,{Strike.SalvoIndex,Strike.RoundIndex},B->Elapsed,true,Receipt,Emitted);
    if(!Result)R.Notice=OrderNotice(Result.reason);
}
bool Command(AWarfareFrontlineGameMode* B,EFRRUTacticalCommandV82 Kind,const TArray<int32>& SquadIds,const FString& TargetNode,const FString& OptionOrStoryId,const FString& ChoiceId,int32 Variant,int32 Side,const FString& AlternateNode)
{
    if(!B||!Ready(B)||B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B)||Side<0||Side>1)return false;
    auto& R=WorldFor(B);if(!SameDeployment(B,R)||R.Finished)return false;
    const auto Snapshot=Capture(B,R);const auto Order=MakeOrder(R.State,Kind,SquadIds,TargetNode,OptionOrStoryId,ChoiceId,Variant,Side,AlternateNode);
    FBatch Batch(B,R);const auto Result=wfexp::tactical::Execute(R.State,Snapshot,Order,Batch);
    if(!Result){R.Notice=OrderNotice(Result.reason);return false;}
    R.OrderPreview.At=-1;
    // PlanFire/CancelFire may replace the native proof owner. Retire the old
    // journal before the next Capture rather than after a rejected clock tick.
    RetireStalePlans(R);
    if(Kind==EFRRUTacticalCommandV82::Observe)R.Observers[Side]=Order.squadIds;
    return true;
}
FString CommandBlockReason(AWarfareFrontlineGameMode* B,EFRRUTacticalCommandV82 Kind,const TArray<int32>& SquadIds,const FString& TargetNode,const FString& OptionOrStoryId,const FString& ChoiceId,int32 Variant,int32 Side,const FString& AlternateNode)
{
    if(!B||!Ready(B)||B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B)||Side<0||Side>1)return TEXT("Orders unavailable while the battle is paused");
    auto& R=WorldFor(B);if(!SameDeployment(B,R)||R.Finished)return TEXT("Orders unavailable outside this encounter");
    auto& Preview=R.OrderPreview;const double Now=FPlatformTime::Seconds();
    // A coherent state/snapshot pair is shared by all option rows. Never trace
    // every observer-to-post line once per row or mutate arrival housekeeping.
    if(Preview.At<0||Now-Preview.At>=.25)
    {Preview.Snapshot=Capture(B,R,false);Preview.State=R.State;Preview.At=Now;Preview.Reasons.clear();}
    const auto Order=MakeOrder(Preview.State,Kind,SquadIds,TargetNode,OptionOrStoryId,ChoiceId,Variant,Side,AlternateNode);
    std::string Key=std::to_string(uint8(Kind))+"|"+std::to_string(Side)+"|"+std::to_string(Variant)+"|"+Order.targetNode+"|"+Order.alternateNode+"|"+Order.optionId+"|"+Order.storyId+"|"+Order.choiceId;
    for(int Id:Order.squadIds)Key+="|"+std::to_string(Id);
    if(const auto Found=Preview.Reasons.find(Key);Found!=Preview.Reasons.end())return Found->second;
    const auto Decision=wfexp::tactical::Evaluate(Preview.State,Preview.Snapshot,Order);
    const FString Reason=Decision.allowed?FString():wfexpue::Wide(Decision.reason);
    Preview.Reasons.emplace(std::move(Key),Reason);return Reason;
}
namespace
{
// One exact graph query is shared by public moves and automatic route scoring.
// It reads scene/date/ownership/clearance; formation and body reservations still
// belong to the normal atomic move transaction.
std::vector<std::string> PhysicalOrderRouteV87(AWarfareFrontlineGameMode* B,const FWorld& R,
    const FFrontlineSquad& Section,const wfexp::tactical::NodeDefinition& Source,const std::string& Target)
{
    const auto* M=Definition(B);const auto* G=Geometry(B);const auto* Group=&Section;const auto* Here=&Source;
    if(!M||!G||Target==Source.id||!Node(*M,Target))return {};
    struct FStep {int Cost;std::string At;std::vector<std::string> Edges;};
    auto Later=[](const FStep& A,const FStep& Z){return A.Cost>Z.Cost;};std::priority_queue<FStep,std::vector<FStep>,decltype(Later)> Queue(Later);std::map<std::string,int> Costs;
    Queue.push({0,Here->id,{}});Costs[Here->id]=0;
    const std::string PreferredApproach=Group->Team==R.State.playerSide?R.State.approach:std::string("primary");
    while(!Queue.empty())
    {
        auto Current=Queue.top();Queue.pop();if(Current.At==Target)return std::move(Current.Edges);if(Costs[Current.At]!=Current.Cost)continue;
        for(const auto& Link:M->edges)
        {
            std::string Next;if(Link.from==Current.At)Next=Link.to;else if(Link.bidirectional&&Link.to==Current.At)Next=Link.from;else continue;
            if(Group->Team==R.State.playerSide&&Link.approach=="enemy")continue;
            const bool Work=Next==Target&&RegisteredWorker(R,*M,*Group,Next);
            if(!Link.clearanceGate.empty()&&!R.Cleared.count(Link.clearanceGate)&&!(Work&&Link.clearanceGate==Next))continue;
            auto Owner=R.Owners.find(Next);if(Owner!=R.Owners.end()&&Owner->second==1-Group->Team&&Next!=Target)continue;
            const auto* A=Node(*M,Current.At);const auto* Z=Node(*M,Next);if(!A||!Z||!wfexp::scene::CanTraverseRoute(*G,Link.id,A->position,Z->position))continue;
            const bool Branch=Link.approach=="primary"||Link.approach=="alternate";const int Cost=Current.Cost+(Branch&&Link.approach!=PreferredApproach?100:1);
            auto Prior=Costs.find(Next);if(Prior!=Costs.end()&&Prior->second<=Cost)continue;Costs[Next]=Cost;auto Edges=Current.Edges;Edges.push_back(Link.id);Queue.push({Cost,Next,std::move(Edges)});
        }
    }
    return {};
}
double PhysicalOrderLengthV87(AWarfareFrontlineGameMode* B,const wfexp::tactical::MissionDefinition& M,
    const FFrontlineSquad& Group,const wfexp::tactical::NodeDefinition& Here,const std::vector<std::string>& Edges)
{
    const auto* G=Geometry(B);if(!G)return MAX_dbl;
    double Length=FVector::Dist2D(Group.Centre(),Vector(Here.position));
    for(const auto& Id:Edges)
    {
        const auto* Road=wfexp::scene::FindRoute(*G,Id);
        if(Road&&Road->points.size()>1)for(size_t I=1;I<Road->points.size();++I)Length+=FVector::Dist(Vector(Road->points[I-1]),Vector(Road->points[I]));
        else
        {
            const wfexp::tactical::EdgeDefinition* Edge=nullptr;for(const auto& Link:M.edges)if(Link.id==Id){Edge=&Link;break;}
            if(!Edge)return MAX_dbl;const auto* From=Node(M,Edge->from);const auto* To=Node(M,Edge->to);
            if(!From||!To)return MAX_dbl;Length+=FVector::Dist(Vector(From->position),Vector(To->position));
        }
    }
    return Length;
}
}
bool MoveSquad(AWarfareFrontlineGameMode* B,int32 SquadId,const FString& TargetNode)
{
    if(!B||!Ready(B)||B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B))return false;auto& R=WorldFor(B);const auto* M=Definition(B);const auto* G=Geometry(B);const auto* Group=B->FindSquad(SquadId);
    const auto* Here=M&&Group?OccupiedNode(*Group,*M):nullptr;const auto Target=wfexpue::Narrow(TargetNode);
    if(!M||!G||!Group||!Here||Here->id==Target||!AtNode(*Group,*Here,true)||Group->bAdvancing||!Node(*M,Target)||FrozenHold(R,*M,*Group,Target))return false;
    auto Route=PhysicalOrderRouteV87(B,R,*Group,*Here,Target);
    if(Route.empty())return false;
    wfexp::tactical::Intent Intent;Intent.id="v82.move."+std::to_string(SquadId);Intent.kind=wfexp::tactical::IntentKind::AdvanceExistingSquad;Intent.side=Group->Team;Intent.squadId=SquadId;Intent.targetNode=Target;Intent.routeEdges=std::move(Route);Intent.target=Node(*M,Target)->position;
    const auto Snapshot=Capture(B,R);FBatch Batch(B,R);const auto Preflight=Batch.Preflight(Snapshot,{Intent});
#if WITH_EDITOR
    if(!Preflight&&CandidateReview()&&R.Mission==TEXT("ru.galicia"))
    {
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_MOVE_REFUSAL squad=%d side=%d target=%s host=%.3f reason=%s"),SquadId,Group->Team,*TargetNode,B->Elapsed,*wfexpue::Wide(Preflight.reason));
        for(const auto& Other:B->Squads)if(Other.Id!=SquadId&&Other.Team==Group->Team&&Other.LivingCount()>0)
        {
            const auto Path=R.Routes.find(Other.Id);const auto* At=OccupiedNode(Other,*M);
            if((Path!=R.Routes.end()&&Other.bAdvancing&&Path->second.Target==TargetNode)||(!Other.bAdvancing&&At&&At->id==Target))
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_MOVE_TARGET_CLAIM squad=%d target=%s advancing=%d living=%d"),Other.Id,*TargetNode,Other.bAdvancing,Other.LivingCount());
        }
    }
#endif
    const bool Accepted=Preflight&&Batch.Commit(Snapshot,{Intent});
    if(Accepted)R.OrderPreview.At=-1;return Accepted;
}
bool HoldSquad(AWarfareFrontlineGameMode* B,int32 SquadId,bool Hold)
{
    if(!B||!Ready(B)||B->Phase!=EFrontlinePhase::Fighting||UGameplayStatics::IsGamePaused(B))return false;auto& R=WorldFor(B);auto* Group=B->FindSquad(SquadId);
    if(!Group||Group->Team!=0||Group->LivingCount()<=0)return false;
    wfexp::tactical::Intent Intent;Intent.id="v82.hold."+std::to_string(SquadId);Intent.kind=wfexp::tactical::IntentKind::SetHold;Intent.side=0;Intent.squadId=SquadId;Intent.hold=Hold;
    const auto Snapshot=Capture(B,R);FBatch Batch(B,R);const bool Accepted=Batch.Preflight(Snapshot,{Intent})&&Batch.Commit(Snapshot,{Intent});
    if(Accepted)R.OrderPreview.At=-1;return Accepted;
}
#include "FRRUCampaignPlayV827.inl"
#include "FRRUEnemySupportV87.inl"
void Tick(AWarfareFrontlineGameMode* B)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return;auto& R=WorldFor(B);
    if(B->Phase!=EFrontlinePhase::Fighting)
    {
        if(R.HonourResultPending&&WarfareFRRUCampaignInternalV82::RetryOutcome(B)
            &&WarfareFRRUCampaignInternalV82::OutcomeReceiptCommittedV826(B))
        {R.HonourResultPending=false;FrontlineAchievementsV76::Finish(B,R.HonourWinner);}return;
    }
    if(!SameDeployment(B,R)||!ReadyForWorldTick(B)||R.Finished||UGameplayStatics::IsGamePaused(B))return;
    RetireStalePlans(R);UpdateSquads(B);const auto Snapshot=Capture(B,R);FBatch Batch(B,R);bool Advanced=false;
    const auto Result=R.Clock.Publish(R.State,Snapshot,{++R.HostFrame,B->Elapsed,true,false},Batch,Advanced);
    if(!Result){R.Notice=OrderNotice(Result.reason);return;}
    // A newly activated stage can reveal its local order in this same frame.
    // Freeze before AI, reserve or support work can continue behind the modal.
    if(PauseForPendingStory(B))return;
    TickOrdinaryAdvanceV827(B,R);
    for(int32 Side=0;Side<2;++Side)if(!R.ActivePlans[Side].empty())
    {
        std::vector<wfexp::tactical::SupportReceipt> Pending;if(R.Journal.PendingReceipts(R.ActivePlans[Side],R.State.encounterId,Pending))for(const auto& Receipt:Pending)R.Journal.Acknowledge(Receipt);
    }
    if(!R.State.objective.finished&&B->Elapsed>=R.ObserveAt)
    {
        R.ObserveAt=B->Elapsed+.25;
        for(int32 Side=0;Side<2;++Side)if(!R.Observers[Side].empty()&&R.State.fire[Side].phase!=wfexp::tactical::FirePhase::Idle&&R.State.fire[Side].phase!=wfexp::tactical::FirePhase::Committed)
        {TArray<int32> Squads;for(const int Id:R.Observers[Side])Squads.Add(Id);Command(B,EFRRUTacticalCommandV82::Observe,Squads,FString(),FString(),FString(),0,Side);}
    }
    // Enemy decisions use the public mission and actual contact observations.
    // They never inspect player orders, cursor, recruitment selection or plans.
    if(!R.State.objective.finished&&B->Elapsed>=R.EnemyDecisionAt)
    {
        const auto& Doctrine=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
        R.EnemyDecisionAt=B->Elapsed+R.EnemyRandom.FRandRange(1.3f,2.4f)*Doctrine.DecisionScale;
        TArray<AWarfareFrontlineSoldier*> Seen;for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&FrontlineDifficultyV30::CanObserve(B,U))Seen.Add(U);
        FrontlineDifficultyV30::Observe(B,Seen);
        TickEnemyRecruitment(B,R);
        if(B->Elapsed>=R.EnemyAssaultAt)
        {
            R.EnemyAssaultAt=B->Elapsed+R.EnemyRandom.FRandRange(5.f,8.f)*Doctrine.AssaultIntervalScale*FrontlinePacingV24::TempoIntervalScale;
            TickEnemyAdvanceV827(B,R);
        }
        if(R.State.counter[1].pending&&B->Elapsed>=R.State.counter[1].warnedAt+4)Command(B,EFRRUTacticalCommandV82::AdvanceCounter,{},FString(),FString(),FString(),0,1);
        TickEnemySupportV87(B,R,Seen);
    }
    if(R.LocalResult){R.PendingEnd=true;FFRRUWorldOutcomeAccessV82::Finish(B,R.LocalSuccess);}
}
bool ResolveOutcome(AWarfareFrontlineGameMode* B,int32 Winner)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return false;auto& R=WorldFor(B);if(!SameDeployment(B,R)||R.Finished)return false;
    if(Winner==0&&(!R.State.objective.finished||!R.State.objective.success))return false;
    if(!R.State.objective.finished)
    {
        if(Winner!=1)return false;auto Candidate=R.State;const auto Result=wfexp::RecordDefeat(Candidate.campaign.progression,Candidate.missionId,Candidate.encounterId);if(!Result)return false;
        Candidate.objective.finished=true;Candidate.objective.success=false;R.State=std::move(Candidate);
    }
    // A held post, completed clearance or withdrawal is not a breakthrough.
    // Capture this before retirement: later animations cannot change the result.
    R.BreakthroughVictory=false;
    if(Winner==0)
    {
        const auto* M=Definition(B);const auto* End=M?Node(*M,"enemy_assembly"):nullptr;
        if(M&&End&&R.State.objective.index<M->stages.size())
        {
            const auto& Final=M->stages[R.State.objective.index];
            const bool EnemyDeployment=Final.kind==wfexp::tactical::GateKind::TraverseNode&&Final.finalStage
                &&std::find(Final.nodes.begin(),Final.nodes.end(),End->id)!=Final.nodes.end();
            int32 Formations=0,Living=0;bool Clear=true;
            if(EnemyDeployment)for(const auto& Group:B->Squads)if(Group.LivingCount()>0)
            {
                if(Group.Team==0&&AtNode(Group,*End,true)){++Formations;Living+=Group.LivingCount();}
                if(Group.Team==1)for(const auto& Member:Group.Members)
                    if(const AWarfareFrontlineSoldier* U=Member.Get();U&&U->IsActiveCombatant()
                        &&FVector::Dist2D(U->Feet(),Vector(End->position))<=End->radius+500.)Clear=false;
            }
            R.BreakthroughVictory=EnemyDeployment&&Formations>=2&&Living>=FMath::Max(2,Final.minimumLiving)&&Clear;
        }
    }
    FString Error;const bool Saved=WarfareFRRUCampaignInternalV82::SubmitOutcome(B,R.State,B->CombatRecordV65[0].Total.Infantry,B->CombatRecordV65[0].Total.Armour,Error);
    R.HonourResultPending=!Saved&&WarfareFRRUCampaignInternalV82::HasPendingOutcome(B);R.HonourWinner=Winner;
    R.Finished=true;R.Journal.RetireEncounter(R.State.encounterId);if(!Saved)R.Notice=Error;return true;
}
bool OutcomePersisted(const AWarfareFrontlineGameMode* B)
{return B&&WarfareFRRUCampaignV82::Active(B)&&WorldFor(B).Finished
    &&WarfareFRRUCampaignInternalV82::OutcomeReceiptCommittedV826(B);}
bool IsBreakthroughVictory(const AWarfareFrontlineGameMode* B)
{return OutcomePersisted(B)&&WorldFor(B).State.objective.success&&WorldFor(B).BreakthroughVictory;}
FFRRUWorldViewV82 View(const AWarfareFrontlineGameMode* B)
{
    FFRRUWorldViewV82 V;if(!B||!WarfareFRRUCampaignV82::Active(B))return V;auto& R=WorldFor(B);const auto* M=Definition(B);if(!M)return V;
    V.MissionId=wfexpue::Wide(M->id);const auto* G=Geometry(B);V.SceneId=G?wfexpue::Wide(G->id):FString();V.Notice=R.Notice;
    for(const auto& N:M->nodes){FFRRUNodeV82 Row;Row.Id=wfexpue::Wide(N.id);Row.Position=Vector(N.position);Row.Radius=float(N.radius);Row.Capacity=N.capacity;auto I=R.Owners.find(N.id);Row.Owner=I==R.Owners.end()?-1:I->second;Row.Blocked=false;V.Nodes.Add(std::move(Row));}
    if(!R.Started)return V;
    const auto& Stage=M->stages[R.State.objective.index];V.StageId=wfexpue::Wide(Stage.id);
    V.FallbackAwaitingArrival=R.State.objective.fallback&&!R.State.objective.fallbackArrived;
    V.TargetNode=wfexpue::Wide(V.FallbackAwaitingArrival?Stage.fallbackNode:R.State.objective.boundObstacle);V.RequiredRole=Stage.requiredRole;
    V.FirePhase=int32(R.State.fire[0].phase);V.FireTarget=wfexpue::Wide(R.State.fire[0].node);V.AlternateTarget=wfexpue::Wide(R.State.fire[0].alternateNode);V.ReservePending=R.State.reserve[0].released<R.State.reserve[0].squads.size();
    if(!R.ActivePlans[0].empty()){std::vector<wfexp::tactical::SupportReceipt> Receipts;if(R.Journal.ActiveReceipts(R.ActivePlans[0],R.State.encounterId,Receipts))for(const auto& Receipt:Receipts)V.ResolvedSalvos=FMath::Max(V.ResolvedSalvos,Receipt.resolvedSalvos);}
    V.Progress=Stage.duration>0?float(R.State.objective.progress/Stage.duration):0;V.Remaining=float(FMath::Max(0.,R.State.objective.deadline-B->Elapsed));
    V.Fighting=B->Phase==EFrontlinePhase::Fighting;V.Fallback=R.State.objective.fallback;V.Finished=R.State.objective.finished;V.Success=R.State.objective.success;
    for(const auto& Story:M->stories)if(Story.stageId==Stage.id&&B->Elapsed>=R.State.objective.activatedAt+Story.triggerAfter&&std::none_of(R.State.choices.begin(),R.State.choices.end(),[&](const auto& Choice){return Choice.storyId==Story.id;}))
    {
        V.StoryPending=true;V.StoryId=wfexpue::Wide(Story.id);V.StoryTitle=WarfareFRRUTextV82::Resolve(V.StoryId,TEXT("title"));V.StoryBody=WarfareFRRUTextV82::Resolve(V.StoryId,TEXT("body"));
        for(const auto& Choice:Story.choices){const FString ChoiceId=wfexpue::Wide(Choice.id);V.ChoiceIds.Add(ChoiceId);V.ChoiceTitles.Add(WarfareFRRUTextV82::Resolve(V.StoryId+TEXT(".")+ChoiceId,TEXT("title")));V.ChoiceDescriptions.Add(WarfareFRRUTextV82::Resolve(V.StoryId+TEXT(".")+ChoiceId,TEXT("description")));}break;
    }
    return V;
}
#include "WarfareFRRUStoryPauseV82.inl"
#include "WarfareFRRUWorldGeometryReviewV82.inl"
}
