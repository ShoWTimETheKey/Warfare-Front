// Included in WarfareFRRUWorldV82. The pause belongs to one real encounter
// and local order; it never adjusts native clocks, deadlines or receipts.
const wfexp::tactical::StoryDefinition* PendingStory(const AWarfareFrontlineGameMode* B,const FWorld& R)
{
    const auto* M=Definition(B);if(!M||R.State.objective.index>=M->stages.size())return nullptr;
    const auto& Stage=M->stages[R.State.objective.index];
    for(const auto& Story:M->stories)if(Story.stageId==Stage.id
        &&B->Elapsed>=R.State.objective.activatedAt+Story.triggerAfter
        &&std::none_of(R.State.choices.begin(),R.State.choices.end(),[&](const auto& Choice){return Choice.storyId==Story.id;}))return &Story;
    return nullptr;
}
bool PauseForPendingStory(AWarfareFrontlineGameMode* B,bool TakeOverOwnedPause)
{
    if(!B||!IsInGameThread())return false;
    auto* Found=Worlds.Find(TWeakObjectPtr<AWarfareFrontlineGameMode>(B));
    if(!Found)return false;auto& R=**Found;
    if(!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting
        ||!SameDeployment(B,R)||!Ready(B)||R.Finished||R.State.objective.finished)
    {R.StoryPause={};return false;}
    const auto* Story=PendingStory(B,R);const bool SameEncounter=R.StoryPause.Active&&R.StoryPause.Encounter==B->GetEncounterId();
    if(!Story)
    {
        const bool Resume=SameEncounter&&!R.StoryPause.RestorePaused;R.StoryPause={};
        if(Resume)UGameplayStatics::SetGamePaused(B,false);
        return false;
    }
    const FString StoryId=wfexpue::Wide(Story->id);
    if(!SameEncounter||R.StoryPause.StoryId!=StoryId)
    {
        const bool Restore=SameEncounter?R.StoryPause.RestorePaused:UGameplayStatics::IsGamePaused(B);
        R.StoryPause={B->GetEncounterId(),StoryId,true,Restore};
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_STORY_PAUSE encounter=%s story=%s elapsed=%.3f restore_paused=%d"),
            *B->GetEncounterId().ToString(),*StoryId,B->Elapsed,int32(Restore));
    }
    if(TakeOverOwnedPause)R.StoryPause.RestorePaused=false;
    if(!UGameplayStatics::IsGamePaused(B))UGameplayStatics::SetGamePaused(B,true);return true;
}

void ResumeAfterOwnedPause(AWarfareFrontlineGameMode* B)
{
    if(B&&IsInGameThread()&&!PauseForPendingStory(B,true))B->SetPaused(false);
}

bool ChooseStory(AWarfareFrontlineGameMode* B,const FString& StoryId,const FString& ChoiceId)
{
    if(!B||!IsInGameThread()||!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting)return false;
    if(!PauseForPendingStory(B))return false;const auto V=View(B);
    if(!V.StoryPending||V.StoryId!=StoryId||!V.ChoiceIds.Contains(ChoiceId))return false;
    auto& R=WorldFor(B);const auto Owner=R.StoryPause;
    // No yield, asynchronous operation or Tick occurs between resume and the
    // original atomic command. Snapshot.paused describes the real world.
    if(!UGameplayStatics::SetGamePaused(B,false)||UGameplayStatics::IsGamePaused(B))
    {UGameplayStatics::SetGamePaused(B,true);return false;}
    const bool Accepted=Command(B,EFRRUTacticalCommandV82::ChooseStory,{},FString(),StoryId,ChoiceId);
    if(!SameDeployment(B,R)||B->GetEncounterId()!=Owner.Encounter)
    {R.StoryPause={};return false;}
    if(!Accepted)
    {UGameplayStatics::SetGamePaused(B,true);return false;}
    R.StoryPause={};UGameplayStatics::SetGamePaused(B,Owner.RestorePaused);
    // Multiple orders at one stage retain the first owner's manual pause.
    PauseForPendingStory(B);
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_STORY_CHOICE encounter=%s story=%s choice=%s elapsed=%.3f paused=%d"),
        *Owner.Encounter.ToString(),*StoryId,*ChoiceId,B->Elapsed,int32(UGameplayStatics::IsGamePaused(B)));
    return true;
}

#if WITH_EDITOR
TOptional<bool> ReviewStoryChoiceDwell(AWarfareFrontlineGameMode* B,const FString& StoryId,const FString& ChoiceId)
{
    if(!CandidateReview()||!PauseForPendingStory(B))return false;
    const auto V=View(B);if(V.StoryId!=StoryId||!V.ChoiceIds.Contains(ChoiceId))return false;
    auto& R=WorldFor(B);auto& Probe=R.StoryPauseProbe;
    if(Probe.Encounter!=B->GetEncounterId()||Probe.StoryId!=StoryId)
    {
        Probe={};Probe.Encounter=B->GetEncounterId();Probe.StoryId=StoryId;Probe.Started=FPlatformTime::Seconds();
        Probe.Elapsed=B->Elapsed;Probe.HostFrame=R.HostFrame;Probe.NativeSignature=wfexp::tactical::StateSignature(R.State);Probe.Snapshot=Capture(B,R);
        Probe.UnitCount=B->Units.Num();Probe.SquadCount=B->Squads.Num();Probe.StrikeCount=B->Strikes.Num();
        for(AWarfareFrontlineSoldier* U:B->Units)if(U)Probe.Feet.Add(TWeakObjectPtr<AWarfareFrontlineSoldier>(U),U->Feet());
        return TOptional<bool>();
    }
    const double Wall=FPlatformTime::Seconds()-Probe.Started;if(Wall<3.)return TOptional<bool>();
    const auto After=Capture(B,R);
    bool Frozen=UGameplayStatics::IsGamePaused(B)&&B->Elapsed==Probe.Elapsed&&R.HostFrame==Probe.HostFrame
        &&wfexp::tactical::StateSignature(R.State)==Probe.NativeSignature
        &&B->Units.Num()==Probe.UnitCount&&B->Squads.Num()==Probe.SquadCount&&B->Strikes.Num()==Probe.StrikeCount;
    bool Cooldowns=After.supportReadyAt==Probe.Snapshot.supportReadyAt&&After.supportRoundBudget==Probe.Snapshot.supportRoundBudget
        &&After.supportSalvoCount==Probe.Snapshot.supportSalvoCount;
    for(const auto& Foot:Probe.Feet){const auto* U=Foot.Key.Get();Frozen&=U&&U->Feet().Equals(Foot.Value,.001f);}
    const bool InvalidRejected=!ChooseStory(B,StoryId,TEXT("__invalid_story_choice_v82__"));
    const bool InvalidFrozen=UGameplayStatics::IsGamePaused(B)&&B->Elapsed==Probe.Elapsed&&R.HostFrame==Probe.HostFrame
        &&wfexp::tactical::StateSignature(R.State)==Probe.NativeSignature;
    const bool RestorePaused=R.StoryPause.RestorePaused;const double Deadline=R.State.objective.deadline;
    const bool Accepted=Frozen&&Cooldowns&&InvalidRejected&&InvalidFrozen&&ChooseStory(B,StoryId,ChoiceId);
    const bool Restored=Accepted&&B->Elapsed==Probe.Elapsed&&R.HostFrame==Probe.HostFrame
        &&R.State.objective.deadline==Deadline&&UGameplayStatics::IsGamePaused(B)==(RestorePaused||View(B).StoryPending);
    const bool Passed=Accepted&&Restored;
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_STORY_DWELL_REVIEW passed=%d story=%s wall_seconds=%.3f elapsed_before=%.6f elapsed_after=%.6f host_frame_before=%llu host_frame_after=%llu units=%d feet_frozen=%d support_frozen=%d invalid_rejected=%d invalid_frozen=%d valid_restored=%d"),
        int32(Passed),*StoryId,Wall,Probe.Elapsed,B->Elapsed,Probe.HostFrame,R.HostFrame,Probe.UnitCount,int32(Frozen),int32(Cooldowns),int32(InvalidRejected),int32(InvalidFrozen),int32(Restored));
    return Passed;
}
#endif
