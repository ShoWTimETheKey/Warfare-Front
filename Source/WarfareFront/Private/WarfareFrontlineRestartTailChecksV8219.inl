void RunRestartTailChecksV8219(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&B->Units.IsEmpty()&&B->Phase==EFrontlinePhase::Briefing
  &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"));
 Check(TEXT("v8219_restart_checks_require_empty_disposable_world"),Safe);if(!Safe)return;
 FTransientReviewV81 Scope;Load();const auto BeforeAccount=Account;const auto BeforeAward=IsolatedAwardBattle;
 const FString Path=AccountPath(),Backup=Path+TEXT(".bak");TArray<uint8> MainBytes,BackupBytes;
 const bool HadMain=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
 if((HadMain&&!FFileHelper::LoadFileToArray(MainBytes,*Path))||(HadBackup&&!FFileHelper::LoadFileToArray(BackupBytes,*Backup))){Check(TEXT("v8219_restart_account_snapshot"),false);return;}
 ON_SCOPE_EXIT
 {
  ReviewFailSaveV81=ReviewFailMirrorV81=ReviewFailLockV81=false;IsolatedAwardBattle=BeforeAward;Account=BeforeAccount;
  B->Phase=EFrontlinePhase::Briefing;B->RestartBattle();
  if(HadMain)FFileHelper::SaveArrayToFile(MainBytes,*Path);else IFileManager::Get().Delete(*Path);
  if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
  // This is an empty, volatile review before either real battlefield is prepared.
  // The next stage starts a new fixture through the normal campaign API.
 };
 IsolatedAwardBattle=B;Account=FAccount();Account.Loaded=true;
 {FAccountLock Lock;Check(TEXT("v8219_restart_empty_account_saved"),Lock.Valid()&&SaveLocked());}
 for(int32 N=0;N<2;++N)for(int32 Winner=0;Winner<2;++Winner)
 {
  B->Phase=EFrontlinePhase::Briefing;
  const FString Prefix=FString::Printf(TEXT("v8219_restart_nation_%d_winner_%d_"),N,Winner);
  const bool Seed=FFrontlineAchievementReviewV76::NewUnlockFixtureV8215(B,EFrontlineFaction(N),0);
  Check(Prefix+TEXT("real_campaign_bound"),Seed);if(!Seed)continue;
  B->Phase=EFrontlinePhase::Fighting;B->Elapsed=90;BeginEncounter(B);
  B->CombatRecordV65[0].Total.Infantry=11;B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Infantry=11;
  FFrontlineAchievementReviewV76::FinishReviewBattleV8219(B,Winner);
  const auto Encounter=B->GetEncounterId();const auto Phase=B->Phase;
  const int32 Points=B->GetResearchPoints(),Completed=B->GetCurrentCampaign()->CompletedMissions,Victories=Account.Medals[N].Victories;
  const auto Awards=Account.MedalAwardsV8216;const uint32 Earned=Account.EarnedMask;
  const int32 MedalsBefore=Account.Medals[N].Infantry;
  Check(Prefix+TEXT("zero_tail_can_be_flushed_without_closing_later_ranges"),FinalizeBattleKillsV8217(B));
  B->CombatRecordV65[0].Total.Infantry+=7;B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Infantry+=7;B->CombatRecordV65[0].InfantryLost+=2;
  ReviewFailSaveV81=true;const bool FailedRestart=B->RestartBattle();ReviewFailSaveV81=false;
  Check(Prefix+TEXT("failed_tail_save_preserves_result_and_encounter"),!FailedRestart&&B->Phase==Phase&&B->GetEncounterId()==Encounter&&B->CombatRecordV65[0].Total.Infantry==18);
  ReviewFailMirrorV81=true;const bool FailedMirror=B->RestartBattle();
  Check(Prefix+TEXT("primary_only_tail_save_keeps_result_for_mirror_retry"),!FailedMirror&&B->Phase==Phase&&B->GetEncounterId()==Encounter);
  B->CombatRecordV65[0].Total.Infantry+=3;B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Infantry+=3;B->CombatRecordV65[0].InfantryLost+=1;
  const bool FailedAgain=B->RestartBattle();ReviewFailMirrorV81=false;
  Check(Prefix+TEXT("second_mirror_failure_keeps_frozen_range_and_new_tail"),!FailedAgain&&B->Phase==Phase&&B->GetEncounterId()==Encounter&&B->CombatRecordV65[0].Total.Infantry==21);
  const bool Retired=Winner==0?B->ContinueCampaign():B->RestartBattle();const auto* R=B->GetCurrentCampaign();FAccount Disk;
  Check(Prefix+TEXT("retry_retires_once_and_keeps_campaign_source_losses"),Retired&&B->Phase==EFrontlinePhase::Briefing&&B->GetEncounterId()!=Encounter&&R&&R->CombatRecordV65.Total.Infantry==21&&R->CombatRecordV65.SupportSources[int32(EFrontlineSupport::Artillery)].Infantry==21&&R->CombatRecordV65.InfantryLost==3&&R->CombatRecordV65.BattlesRecorded==1);
  Check(Prefix+TEXT("real_account_reload_includes_tail_once"),ReadFile(Path,Disk)&&Disk.Medals[N].Infantry==MedalsBefore+10&&Disk.Medals[N].Victories==Victories);
  bool SameAwards=Account.MedalAwardsV8216.Num()==Awards.Num();for(const auto& Pair:Awards){const auto* A=Account.MedalAwardsV8216.Find(Pair.Key);SameAwards&=A&&A->Grade==Pair.Value.Grade&&A->Repeats==Pair.Value.Repeats&&A->Attachments==Pair.Value.Attachments&&A->Date==Pair.Value.Date&&A->Campaign==Pair.Value.Campaign&&A->CampaignDeeds==Pair.Value.CampaignDeeds;}
  Check(Prefix+TEXT("tail_does_not_repeat_points_route_or_honours"),R&&B->GetResearchPoints()==Points&&R->CompletedMissions==Completed&&Account.EarnedMask==Earned&&SameAwards);
  const int32 FinalKills=Account.Medals[N].Infantry;B->RestartBattle();Check(Prefix+TEXT("repeated_restart_is_idempotent"),Account.Medals[N].Infantry==FinalKills&&B->GetCurrentCampaign()->CombatRecordV65.Total.Infantry==21);
 }
}
