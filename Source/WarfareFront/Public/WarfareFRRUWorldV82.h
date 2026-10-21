#pragma once
#include "CoreMinimal.h"

class AWarfareFrontlineGameMode;
class AWarfareFrontlineSoldier;
class FJsonObject;
struct FFrontlineStrike;
enum class EFrontlineSupport : uint8;

enum class EFRRUTacticalCommandV82 : uint8
{
    ChooseStory,PlanFire,Observe,CommitFire,CancelFire,Followup,
    RegisterReserve,ReleaseReserve,CounterOrder,AdvanceCounter,UseOption,PolicyOrder,AdvanceToObjective
};
struct FFRRUNodeV82
{
    FString Id;
    FVector Position=FVector::ZeroVector;
    float Radius=0;
    int32 Capacity=0,Owner=INDEX_NONE;
    bool Blocked=false;
};
struct FFRRUWorldViewV82
{
    FString MissionId,SceneId,StageId,TargetNode,StoryId,StoryTitle,StoryBody,Notice;
    TArray<FString> ChoiceIds,ChoiceTitles,ChoiceDescriptions;
    TArray<FFRRUNodeV82> Nodes;
    float Progress=0,Remaining=0;
    int32 FirePhase=0,ResolvedSalvos=0,RequiredRole=INDEX_NONE;
    FString FireTarget,AlternateTarget;
    bool ReservePending=false;
    bool Fighting=false,Fallback=false,FallbackAwaitingArrival=false,StoryPending=false,Finished=false,Success=false;
};
struct FFRRUEarthworkPoseV82
{
    FString NodeId;int32 Bay=INDEX_NONE;
    FVector Feet=FVector::ZeroVector,Support=FVector::ZeroVector,Facing=FVector::ForwardVector;
    float CoverDepth=0;
};
enum class EFRRUWorkSemanticV82:uint8 { ClearPassage,SignalsRelay };
struct FFRRUWorkStateV82
{
    FGuid Encounter;FString StageId,NodeId;int32 SquadId=INDEX_NONE,MemberIndex=INDEX_NONE;
    EFRRUWorkSemanticV82 Semantic=EFRRUWorkSemanticV82::ClearPassage;
    FVector Target=FVector::ZeroVector,Facing=FVector::ForwardVector;
    // Stage window and accumulated native work, not a second animation clock.
    double HostElapsed=0,StartedAt=0,EndsAt=0,ProgressSeconds=0,RequiredSeconds=0;
    float NormalizedPhase=0;bool Active=false,Paused=false,CompletedOnce=false,Timed=false;
};
namespace WarfareFRRUWorldV82
{
    WARFAREFRONT_API FString SceneId(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool SceneInfo(const AWarfareFrontlineGameMode* Battle,FString& Id,uint32& Revision,int32& SolidCount,FVector2D& Minimum,FVector2D& Maximum);
    WARFAREFRONT_API bool NodePose(const AWarfareFrontlineGameMode* Battle,const FString& NodeId,FVector& Position,float& Radius);
    WARFAREFRONT_API bool TryHeight(const AWarfareFrontlineGameMode* Battle,float X,float Y,float& Height);
    WARFAREFRONT_API bool GroundContact(const AWarfareFrontlineGameMode* Battle,const FVector& Position,FVector& Feet,FVector& CapsuleCentre);
    WARFAREFRONT_API bool GroundContactPath(const AWarfareFrontlineGameMode* Battle,const FVector& From,const FVector& To,TArray<FVector>& CapsuleCentres);
    WARFAREFRONT_API bool PlaceOnGround(AWarfareFrontlineSoldier* Soldier,const FVector& Position);
    WARFAREFRONT_API bool TryGlobalHeight(float X,float Y,float& Height);
    // Encoded group 1/255 matches the one authored connected R3 water patch.
    // A false national query must never fall through to UK/DE basin caches.
    WARFAREFRONT_API bool TryWater(const AWarfareFrontlineGameMode* Battle,float X,float Y,float& Level,float& Depth,float* GroupIdentity=nullptr);
    WARFAREFRONT_API bool TryGlobalWater(float X,float Y,float& Level,float& Depth,float* GroupIdentity=nullptr);
    WARFAREFRONT_API bool PhysicalSceneActive();
    WARFAREFRONT_API float TravelScale(const AWarfareFrontlineGameMode* Battle,const FVector& Position);
    WARFAREFRONT_API float GlobalTravelScale(const FVector& Position);
    WARFAREFRONT_API bool CanTraverse(const AWarfareFrontlineGameMode* Battle,const FVector& From,const FVector& To);
    WARFAREFRONT_API bool AllowsStep(const AWarfareFrontlineSoldier* Soldier,const FVector& From,const FVector& To);
    WARFAREFRONT_API bool HasSight(const AWarfareFrontlineGameMode* Battle,const FVector& Eye,const FVector& Target);
    WARFAREFRONT_API bool FiringLine(const AWarfareFrontlineSoldier* From,const AWarfareFrontlineSoldier* To);
    WARFAREFRONT_API float Cover(const AWarfareFrontlineSoldier* Soldier);
    WARFAREFRONT_API float CoverFrom(const AWarfareFrontlineSoldier* Soldier,const FVector& Source);
    WARFAREFRONT_API bool EarthworkPose(const AWarfareFrontlineSoldier* Soldier,FFRRUEarthworkPoseV82& Out);
    WARFAREFRONT_API bool WorkState(const AWarfareFrontlineSoldier* Soldier,FFRRUWorkStateV82& Out);
    WARFAREFRONT_API bool PassageCleared(const AWarfareFrontlineGameMode* Battle,const FString& NodeId);
    WARFAREFRONT_API bool PrepareScene(AWarfareFrontlineGameMode* Battle,FString& Error);
    // Read the existing production/isolated-review gate without opening a campaign.
    WARFAREFRONT_API bool HostHooksConnected();
    WARFAREFRONT_API bool CanPrepare(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool Ready(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FString BlockReason(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool Begin(AWarfareFrontlineGameMode* Battle,FString& Error);
    WARFAREFRONT_API bool Reset(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API void Tick(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool UpdateSquads(AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API bool Destination(const AWarfareFrontlineSoldier* Soldier,FVector& Out);
    // O(1) read of this tick's physically validated pure-bend final leg only.
    WARFAREFRONT_API bool RegisteredBendArrivalV87(const AWarfareFrontlineSoldier* Soldier,const FVector& Destination,float& ArrivalRadius,float& StopDistance);
    // Finish a real reserved friendly bay before deploying on its parapet.
    WARFAREFRONT_API bool ShouldFinishFriendlyPostEntry(const AWarfareFrontlineSoldier* Soldier);
    // Read the existing route/bay wait without changing steering or arrival.
    WARFAREFRONT_API bool WaitingToJoinFormation(const AWarfareFrontlineSoldier* Soldier);
#if WITH_EDITOR
    WARFAREFRONT_API TSharedPtr<FJsonObject> SupportDiagnosis(const AWarfareFrontlineGameMode* Battle);
    // Read actual reducer eligibility and host occupancy without issuing orders.
    WARFAREFRONT_API TSharedPtr<FJsonObject> ObjectiveDiagnosis(const AWarfareFrontlineGameMode* Battle);
    // A read-only snapshot of the actual reserved bay and local approach.
    // It never calls Destination or changes reservations/steering.
    WARFAREFRONT_API TSharedPtr<FJsonObject> MovementDiagnosis(const AWarfareFrontlineSoldier* Soldier,bool TestClearance=false);
#endif
    WARFAREFRONT_API FVector MovementStep(const AWarfareFrontlineSoldier* Soldier,const FVector& Proposed,float Distance);
    // Accepted placement owns retry time. Gun actions/turning pass zero attempt time.
    WARFAREFRONT_API void RecordMovementV87(const AWarfareFrontlineSoldier* Soldier,const FVector& From,const FVector& ActualTo,const FVector& Guide,float AttemptSeconds);
    WARFAREFRONT_API void Recruited(AWarfareFrontlineGameMode* Battle,int32 SquadId);
    // Reserve a real node bay for this existing carrier squad from its actual
    // dry exit. This grants neither an entry nor an objective arrival receipt.
    WARFAREFRONT_API bool Disembarked(AWarfareFrontlineGameMode* Battle,int32 SquadId,const AWarfareFrontlineSoldier* FirstOutside);
    WARFAREFRONT_API void ResolvedStrike(AWarfareFrontlineGameMode* Battle,const FFrontlineStrike& Strike);
    // Reading a local order freezes the real host world. Only an existing
    // loading/menu owner may transfer its pause through TakeOverOwnedPause.
    WARFAREFRONT_API bool PauseForPendingStory(AWarfareFrontlineGameMode* Battle,bool TakeOverOwnedPause=false);
    WARFAREFRONT_API void ResumeAfterOwnedPause(AWarfareFrontlineGameMode* Battle);
    // The synchronous choice briefly resumes the real world for the unchanged
    // native command checks, without running a tick or changing combat time.
    WARFAREFRONT_API bool ChooseStory(AWarfareFrontlineGameMode* Battle,const FString& StoryId,const FString& ChoiceId);
#if WITH_EDITOR
    // Bounded real-world dwell probe used by the existing isolated play review.
    // Unset means the three wall seconds have not elapsed yet.
    WARFAREFRONT_API TOptional<bool> ReviewStoryChoiceDwell(AWarfareFrontlineGameMode* Battle,const FString& StoryId,const FString& ChoiceId);
#endif
    WARFAREFRONT_API bool Command(AWarfareFrontlineGameMode* Battle,EFRRUTacticalCommandV82 Kind,const TArray<int32>& SquadIds={},const FString& TargetNode=FString(),const FString& OptionOrStoryId=FString(),const FString& ChoiceId=FString(),int32 Variant=0,int32 Side=0,const FString& AlternateNode=FString());
    // Read-only legality preview. An empty reason means the native reducer
    // accepts these exact inputs; clicking still validates fresh host state.
    WARFAREFRONT_API FString CommandBlockReason(AWarfareFrontlineGameMode* Battle,EFRRUTacticalCommandV82 Kind,const TArray<int32>& SquadIds={},const FString& TargetNode=FString(),const FString& OptionOrStoryId=FString(),const FString& ChoiceId=FString(),int32 Variant=0,int32 Side=0,const FString& AlternateNode=FString());
    WARFAREFRONT_API bool MoveSquad(AWarfareFrontlineGameMode* Battle,int32 SquadId,const FString& TargetNode);
    WARFAREFRONT_API bool HoldSquad(AWarfareFrontlineGameMode* Battle,int32 SquadId,bool Hold);
    WARFAREFRONT_API bool ResolveOutcome(AWarfareFrontlineGameMode* Battle,int32 Winner);
    WARFAREFRONT_API bool OutcomePersisted(const AWarfareFrontlineGameMode* Battle);
    // Frozen from the physical final objective, never inferred from UI prose.
    WARFAREFRONT_API bool IsBreakthroughVictory(const AWarfareFrontlineGameMode* Battle);
    WARFAREFRONT_API FFRRUWorldViewV82 View(const AWarfareFrontlineGameMode* Battle);
}
