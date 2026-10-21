#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineProgression.h"
class AWarfareFrontlineGameMode;
class FJsonObject;

namespace FrontlineCampaignV52
{
// Stable theatre identity supports later campaigns without advertising empty maps.
enum class ETheatre : uint8 { WesternFront=0,EasternFront=1,SouthernFront=2,Gallipoli=3 };
enum class ENodeState : uint8 { Locked,Available,Selected,Completed,Bypassed };
struct FTheatre
{
    ETheatre Id=ETheatre::WesternFront;
    FString Title;
    FVector2D MinLonLat=FVector2D(-2.5,46.4),MaxLonLat=FVector2D(10,52);
    bool bPlayable=true;
};
struct FNode
{
    const FFrontlineCampaignMission* Mission=nullptr;
    int32 Id=INDEX_NONE,Alternative=INDEX_NONE,Date=19170409,Stage=0;
    FVector2D LonLat=FVector2D::ZeroVector;
    ENodeState State=ENodeState::Locked;
    FString BranchTitle,Assignment,Opposition,NewAttachments,StateText;
};
struct FFrontline
{
    int32 Date=19170409;
    FString Label;
    // Longitude, latitude; ordered North Sea to the Swiss frontier.
    TArray<FVector2D> Points;
};
struct FDispatchChoice
{
    FString Title,Effect;
    int32 ReinforcementRole=INDEX_NONE,Research=0;
};
struct FDispatch
{
    int32 Id=INDEX_NONE;
    FString Title,Date,Body;
    TArray<FDispatchChoice> Choices;
    bool bPending=false;
    int32 SelectedChoice=INDEX_NONE;
};
const FTheatre& Theatre();
TArray<FNode> Nodes(const AWarfareFrontlineGameMode* Battle);
FNode GetNode(const AWarfareFrontlineGameMode* Battle,int32 MissionId);
FFrontline Frontline(const AWarfareFrontlineGameMode* Battle,int32 PreviewMissionId=INDEX_NONE);
bool CanStart(const AWarfareFrontlineGameMode* Battle,int32 MissionId);
bool Select(AWarfareFrontlineGameMode* Battle,int32 MissionId);
FString BlockReason(const FFrontlineCampaignProgress& Run,int32 MissionId);
int32 ActualVictories(const FFrontlineCampaignProgress& Run);
int32 RouteOperationCount(const FFrontlineCampaignProgress& Run);
FDispatch Dispatch(const AWarfareFrontlineGameMode* Battle,int32 MissionId=INDEX_NONE);
bool ResolveDispatch(AWarfareFrontlineGameMode* Battle,int32 Choice);
// One encounter-local replacement for an accepted, not-yet-permanent attachment.
bool IsLoanRole(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role);
FString LoanRecruitBlockReason(const AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Team);
void OnRecruit(AWarfareFrontlineGameMode* Battle,EFrontlineRole Role,int32 Team);
void Tick(AWarfareFrontlineGameMode* Battle);
// Archive helpers are transactional at the existing progression I/O boundary.
void Initialize(FFrontlineCampaignProgress& Run);
bool ReadArchive(const FJsonObject& Object,FFrontlineCampaignProgress& Run);
void WriteArchive(FJsonObject& Object,const FFrontlineCampaignProgress& Run);
bool RecordVictory(FFrontlineCampaignProgress& Run,int32 MissionId);
void RunChecks(AWarfareFrontlineGameMode* Battle,TFunctionRef<void(const FString&,bool)> Check);
}
