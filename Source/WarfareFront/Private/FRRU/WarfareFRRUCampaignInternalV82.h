#pragma once
#include "WarfareFRRUCampaignV82.h"
#include "Native/TacticalRuntime.h"

// The live reducer is not a save format. Only strategic state and a retired
// encounter token are saved; actors and delayed effects are never resurrected.
namespace WarfareFRRUCampaignInternalV82
{
    bool CopyCampaign(const AWarfareFrontlineGameMode* Battle,wfexp::tactical::Campaign& Out);
    bool OpenEncounter(AWarfareFrontlineGameMode* Battle,wfexp::tactical::State& Out,FString& Error);
    bool SubmitOutcome(AWarfareFrontlineGameMode* Battle,const wfexp::tactical::State& Resolved,int32 Kills,int32 ArmourKills,FString& Error);
    bool RetryOutcome(AWarfareFrontlineGameMode* Battle);
    bool HasPendingOutcome(const AWarfareFrontlineGameMode* Battle);
    bool OutcomeReceiptCommittedV826(const AWarfareFrontlineGameMode* Battle);
    bool RetireEncounter(AWarfareFrontlineGameMode* Battle);
    void ClearBinding(AWarfareFrontlineGameMode* Battle);
    // Disposable WorldReview only. Prior progression is explicitly synthetic;
    // the selected operation is then tested through real host consumers.
    bool PrepareReviewMission(AWarfareFrontlineGameMode* Battle,const FString& MissionId,FString& Error,EFrontlineDifficulty Level=EFrontlineDifficulty::Veteran);
}
