void RunInitialOutcomeRetryChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&B->Units.IsEmpty()&&B->Phase==EFrontlinePhase::Briefing
  &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"));
 Check(TEXT("v8219_initial_retry_requires_empty_exact_disposable_profile"),Safe);if(!Safe)return;
 // One transient scope only. The serial driver calls this independently of
 // the restart fixture; account files are snapshotted and restored here.
 FTransientReviewV81 Scope;Load();const auto BeforeAccount=Account;const auto BeforeAward=IsolatedAwardBattle;
 const auto BeforeEncounters=Encounters;const auto Profile=FFrontlineAchievementReviewV76::InitialProfileV8219(B);
 const FString Path=AccountPath(),Backup=Path+TEXT(".bak");TArray<uint8> MainBytes,BackupBytes;
 const bool HadMain=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
 if((HadMain&&!FFileHelper::LoadFileToArray(MainBytes,*Path))||(HadBackup&&!FFileHelper::LoadFileToArray(BackupBytes,*Backup)))
 {Check(TEXT("v8219_initial_retry_account_snapshot"),false);return;}
 TArray<FString> CreatedProfiles;
 ON_SCOPE_EXIT
 {
  ReviewFailSaveV81=ReviewFailMirrorV81=ReviewFailLockV81=false;
  FFrontlineAchievementReviewV76::InitialProfileV8219(B,Profile);
  B->Phase=EFrontlinePhase::Briefing;B->RestartBattle();
  IsolatedAwardBattle=BeforeAward;Account=BeforeAccount;Encounters=BeforeEncounters;
  if(HadMain)FFileHelper::SaveArrayToFile(MainBytes,*Path);else IFileManager::Get().Delete(*Path);
  if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
  for(const auto& File:CreatedProfiles)
  {IFileManager::Get().Delete(*File);IFileManager::Get().Delete(*(File+TEXT(".pending")));IFileManager::Get().Delete(*(File+TEXT(".backup")));}
 };
 IsolatedAwardBattle=B;Account=FAccount();Account.Loaded=true;
 {FAccountLock Lock;Check(TEXT("v8219_initial_retry_empty_account_saved"),Lock.Valid()&&SaveLocked());}
 for(int32 N=0;N<2;++N)for(int32 Winner=0;Winner<2;++Winner)
 {
  FFrontlineAchievementReviewV76::InitialProfileV8219(B,Profile);B->Phase=EFrontlinePhase::Briefing;
  const FString Prefix=FString::Printf(TEXT("v8219_initial_nation_%d_winner_%d_"),N,Winner);
  const bool Seed=FFrontlineAchievementReviewV76::NewUnlockFixtureV8215(B,EFrontlineFaction(N),0);
  Check(Prefix+TEXT("real_campaign_bound"),Seed);if(!Seed)continue;
  auto TestProfile=FFrontlineAchievementReviewV76::InitialProfileV8219(B);
  TestProfile.File=FPaths::Combine(Dir,TEXT("AccountReview"),TEXT("initial-progression-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json"));
  TestProfile.Volatile=false;TestProfile.ReadOnly=true;CreatedProfiles.Add(TestProfile.File);
  FFrontlineAchievementReviewV76::InitialProfileV8219(B,TestProfile);
  B->Phase=EFrontlinePhase::Fighting;B->Elapsed=90;BeginEncounter(B);
  B->CombatRecordV65[0].Total.Infantry=11;B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Infantry=11;
  const int32 BeforeKills=Account.Medals[N].Infantry,BeforeVictories=Account.Medals[N].Victories;
  const auto BeforeAwards=Account.MedalAwardsV8216;const uint32 BeforeEarned=Account.EarnedMask;
  uint8 BeforeCross[CrossFactionCountV83];for(int32 I=0;I<CrossFactionCountV83;++I)BeforeCross[I]=Account.CrossNationsV83[I];
  if(Winner==0){State(B).MinimumMorale=.05f;B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0)*.05f;}
  FFrontlineAchievementReviewV76::FinishInitialReviewBattleV8219(B,Winner);
  auto& S=State(B);const auto Encounter=B->GetEncounterId();const auto Campaign=S.Campaign;const auto Phase=B->Phase;
  const int32 Points=B->GetResearchPoints(),Completed=B->GetCurrentCampaign()->CompletedMissions;
  const bool Frozen=S.DeferredOutcomeV8219.IsSet();FSettlementV81 Original;if(Frozen)Original=S.DeferredOutcomeV8219.GetValue();
  const auto* OriginalStats=Original.MedalOutcomesV8215.Find(Encounter);const auto* OriginalProof=Original.MedalProofsV8216.Find(Encounter);
  Check(Prefix+TEXT("original_cross_challenge_frozen_behind_actual_campaign_save"),Winner==1
   ||(Original.CrossNationsV83[1]==(N?2:1)&&OriginalProof&&OriginalProof->MinimumMorale<=.10001f));
  Check(Prefix+TEXT("initial_failure_freezes_result_and_keeps_it_out_of_account_queue"),Frozen&&S.SettlementCaptured
   &&FrontlineProgressionReviewV8219::InitialOutcomePending(B)&&!SettlementsV81.Contains(Campaign)
   &&OriginalStats&&OriginalStats->Value.Infantry==11&&Account.Medals[N].Infantry==BeforeKills
   &&(Winner==1||(OriginalProof&&OriginalProof->Infantry==11&&OriginalProof->Losses==0)));
  const bool ZeroFailed=FinalizeBattleKillsV8217(B);
  Check(Prefix+TEXT("zero_tail_still_retries_failed_original_progression_save"),!ZeroFailed&&B->Phase==Phase
   &&B->GetEncounterId()==Encounter&&FrontlineProgressionReviewV8219::InitialOutcomePending(B)&&State(B).DeferredOutcomeV8219.IsSet());
  B->CombatRecordV65[0].Total.Infantry+=7;B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Infantry+=7;
  B->CombatRecordV65[0].InfantryLost+=2;State(B).MinimumMorale=1.f;B->Morale[0]=FrontlineMoraleV36::StartingMaximum(B,0);Finish(B,Winner);
  if(Winner==0)
  {
   auto LateProof=CaptureCrossBattleProofV83(B,State(B),true,true);LateProof.Saved=true;
   Check(Prefix+TEXT("late_battle_state_no_longer_qualifies_original_cross_challenge"),(CrossBattleFeatsV83(LateProof)&(1u<<1))==0);
  }
  TestProfile.ReadOnly=false;FFrontlineAchievementReviewV76::InitialProfileV8219(B,TestProfile);
  ReviewFailSaveV81=true;const bool AccountFailed=FinalizeBattleKillsV8217(B);
  const auto* Pending=SettlementsV81.Find(Campaign);const auto* Released=Pending?Pending->MedalOutcomesV8215.Find(Encounter):nullptr;
  const auto* ReleasedProof=Pending?Pending->MedalProofsV8216.Find(Encounter):nullptr;
  Check(Prefix+TEXT("successful_progression_retry_releases_original_eleven_kill_proof"),!AccountFailed
   &&!FrontlineProgressionReviewV8219::InitialOutcomePending(B)&&!State(B).DeferredOutcomeV8219.IsSet()
   &&State(B).PendingInitialReceiptV8219&&Released&&Released->Value.Infantry==11
   &&Pending&&Pending->CrossNationsV83[1]==Original.CrossNationsV83[1]
   &&(Winner==1||(ReleasedProof&&OriginalProof&&ReleasedProof->Infantry==11&&ReleasedProof->Losses==0
    &&ReleasedProof->Difficulty==OriginalProof->Difficulty&&ReleasedProof->Date==OriginalProof->Date
    &&ReleasedProof->MinimumMorale==OriginalProof->MinimumMorale))
   &&B->GetCurrentCampaign()->CombatRecordV65.Total.Infantry==18&&B->Phase==Phase);
  auto ExpectedAwards=BeforeAwards;
  if(OriginalStats&&OriginalProof)
  {
   auto Cumulative=Account.Medals[N];const auto& D=OriginalStats->Value;
   Cumulative.Infantry+=D.Infantry;Cumulative.Armour+=D.Armour;Cumulative.Victories+=D.Victories;
   Cumulative.CleanVictories+=D.CleanVictories;Cumulative.NaturalRoute|=D.NaturalRoute;
   FrontlineMedalsV8215::ApplyAwards(ExpectedAwards,Cumulative,*OriginalProof);
  }
  ReviewFailSaveV81=false;const bool Retried=FinalizeBattleKillsV8217(B),Again=FinalizeBattleKillsV8217(B);FAccount Disk;
  Check(Prefix+TEXT("retry_saves_original_plus_tail_once_in_real_account_files"),Retried&&Again&&ReadFile(Path,Disk)
   &&Disk.Medals[N].Infantry==BeforeKills+18&&Disk.Medals[N].Victories==BeforeVictories+(Winner==0?1:0)
   &&Disk.MedalReceiptsV8215.Contains(Encounter)&&!State(B).PendingInitialReceiptV8219);
  bool SameAwards=Account.MedalAwardsV8216.Num()==ExpectedAwards.Num();
  for(const auto& Pair:ExpectedAwards)
  {const auto* A=Account.MedalAwardsV8216.Find(Pair.Key);SameAwards&=A&&A->Grade==Pair.Value.Grade&&A->Repeats==Pair.Value.Repeats
   &&A->Attachments==Pair.Value.Attachments&&A->Campaign==Pair.Value.Campaign&&A->CampaignDeeds==Pair.Value.CampaignDeeds;}
  uint32 ExpectedEarned=BeforeEarned|Original.Mask;
  for(int32 I=0;I<CrossFactionCountV83;++I)if((BeforeCross[I]|Original.CrossNationsV83[I])==CrossRequiredNationsV83[I])ExpectedEarned|=1u<<(CrossFactionStartV83+I);
  Check(Prefix+TEXT("late_support_does_not_recompute_honour_or_reward_proof"),SameAwards&&Account.EarnedMask==ExpectedEarned
   &&B->GetResearchPoints()==Points&&B->GetCurrentCampaign()->CompletedMissions==Completed);
  Check(Prefix+TEXT("real_account_preserves_original_cross_challenge_and_date"),Disk.CrossNationsV83[1]==(BeforeCross[1]|Original.CrossNationsV83[1])
   &&(Winner==1||Disk.CrossDatesV83.FindRef(CrossDateKeyV83(1,N))==Original.CrossDatesV83.FindRef(CrossDateKeyV83(1,N))));
  FString SavedJson;TSharedPtr<FJsonObject> SavedRoot;bool CampaignSaved=false;
  if(FFileHelper::LoadFileToString(SavedJson,*TestProfile.File)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(SavedJson),SavedRoot)&&SavedRoot)
  {
   const TArray<TSharedPtr<FJsonValue>>* Runs=nullptr;
   if(SavedRoot->TryGetArrayField(TEXT("campaigns"),Runs))for(const auto& Value:*Runs)
    if(const auto Row=Value->AsObject();Row&&Row->GetStringField(TEXT("id"))==Campaign.ToString(EGuidFormats::Digits))
    {FFrontlineCombatRecordV65 Record;CampaignSaved=FrontlineCombatRecordV65::ReadArchive(*Row,Record,true)
     &&Record.Total.Infantry==18&&Record.SupportSources[int32(EFrontlineSupport::Artillery)].Infantry==18
     &&Record.InfantryLost==2&&Record.BattlesRecorded==1;}
  }
  Check(Prefix+TEXT("real_progression_reload_contains_one_battle_and_eighteen_kills"),CampaignSaved);
  const bool Retired=Winner==0?B->ContinueCampaign():B->RestartBattle();
  Check(Prefix+TEXT("saved_result_retires_without_another_reward"),Retired&&B->Phase==EFrontlinePhase::Briefing
   &&B->GetEncounterId()!=Encounter&&B->GetResearchPoints()==Points&&Account.Medals[N].Infantry==BeforeKills+18);
 }
}
