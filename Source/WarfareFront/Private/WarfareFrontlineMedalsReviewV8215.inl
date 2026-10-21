bool SeedRibbonReviewV8443(AWarfareFrontlineGameMode* B,EFRRUNationV81 Nation,int32 Stage)
{
 FString Dir;
 if(!B||!B->IsProgressionVolatile()||!B->GetWorld()||!B->GetWorld()->IsGameWorld()
  ||B->Phase!=EFrontlinePhase::Briefing||!B->Units.IsEmpty()||WarfareFRRUCampaignV82::Active(B)
  ||(!FParse::Param(FCommandLine::Get(),TEXT("WFV8443RibbonReview"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV8446Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV84412GameplayFocused"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused")))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!DisposableAccount()
  ||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)||Dir.IsEmpty()||FPaths::IsRelative(Dir)
  ||int32(Nation)<0||int32(Nation)>3||Stage<0||Stage>3
  ||(Stage<3&&Nation!=EFRRUNationV81::German))return false;
 const FString Expected=FPaths::Combine(Dir,TEXT("AccountReview/achievements-v2.json"));
 if(!FPaths::IsSamePath(AccountPath(),Expected))return false;
 // No encounter, campaign completion or public award event is manufactured.
 FAccount Display;Display.Loaded=true;
 TArray<int32> Indices=Stage==3?FrontlineMedalsV8215::NationIndices(Nation):TArray<int32>{3,18,19,20,21,22};
 if(Stage==1||Stage==2)Indices.Add(23);
 const FGuid Campaign(0x84430000,uint32(int32(Nation)+1),1,1);const FString Date=TEXT("2026-10-09T00:00:00Z");
 for(int32 I:Indices)
 {
  auto A=FrontlineMedalsV8215::CompleteCheatAwardV8218(I,Campaign,Date);
  if(Stage<2){A.Grade=1;A.Repeats=0;A.Attachments=0;}
  if(!FrontlineMedalsV8215::ValidAward(I,A))return false;
  const FString Id(FrontlineMedalsV8215::Definition(I).Id);
  if(Stage==3){Display.CheatMedalAwardsV8222.Add(Id,A);Display.CheatMedalsV8218.Add(Id);}
  else Display.MedalAwardsV8216.Add(Id,A);
 }
 Account=MoveTemp(Display);ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();ArmourTallyLedgerV8220.Loaded=true;
 NotificationsV81.Empty();MedalNotificationsV8217.Empty();ArmourKillNotificationsV8220.Empty();
 PendingArmourTallyV8220.Empty();SettlementsV81.Empty();NextRetryV81=ArmourTallyRetryV8220=0;
 // FR/RU artwork parity stays a temporary display overlay. No nation is
 // started and no cheat flag or account record from stage 3 is persisted.
 if(Stage==3)return true;
 FAccountLock Lock;if(!Lock.Valid()||!SaveLocked())return false;
 Account=FAccount();Load();
 return !Account.Corrupt&&!Account.Unsupported&&Account.MedalAwardsV8216.Num()==Indices.Num()
  &&Account.CheatMedalAwardsV8222.IsEmpty();
}
bool SeedMedalReviewV8215(AWarfareFrontlineGameMode* B,bool Completed,int32 Level)
{
 FString Dir;if(!B||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir))return false;
 Account=FAccount();Account.Loaded=true;Account.Routes=Completed?3:0;
 for(int32 N=0;N<2;++N)
 {auto& M=Account.Medals[N];M.Infantry=Level>=3?8200:Level==2?4100:Level==1?1600:0;M.Armour=Level>=3?150:Level==2?55:Level==1?25:0;M.Victories=Completed?40:0;M.CleanVictories=Completed?40:0;M.NaturalRoute=Completed?3:0;
  if(Level>0){FrontlineMedalsV8215::FProof P;P.Nation=N;P.Date=19171120;P.Difficulty=2;P.Won=true;P.Completed=Completed;P.Undefeated=Completed;P.Armour=4;P.Obstacles=3;P.MinimumMorale=.1f;for(int32 Deed=0;Deed<Level;++Deed){P.Campaign=FGuid::NewGuid();FrontlineMedalsV8215::ApplyAwards(Account.MedalAwardsV8216,M,P);}}}
 FAccountLock Lock;return Lock.Valid()&&SaveLocked();
}
bool SeedNewUnlockReviewV8215(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission,bool ReturnToOperations)
{
 if(!FFrontlineAchievementReviewV76::NewUnlockFixtureV8215(B,F,Mission))return false;
 FFrontlineAchievementReviewV76::NewUnlockVictoryV8215(B);B->BattleOutroElapsed=7.01f;
 if(FrontlineNewUnlockV8215::Pending(B))return false;
 return !ReturnToOperations||(B->ContinueCampaign()&&FrontlineNewUnlockV8215::ReleaseAfterReportV8217(B)&&FrontlineNewUnlockV8215::Pending(B));
}
void RunMedalChecksV8215(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&B->Units.IsEmpty()&&B->Phase==EFrontlinePhase::Briefing;
 FrontlineMedalsV8215::RunCatalogueChecks(Check);
 Check(TEXT("v8215_archive_medal_review_absolute_disposable_briefing"),Safe);if(!Safe)return;
 FTransientReviewV81 QueueScope;Load();const FAccount BeforeAccount=Account;const auto BeforeEncounter=State(B);
 const auto BeforePhase=B->Phase;const int32 BeforeOutro=B->BattleOutroWinner;const FString BeforeReason=B->EndReason;const auto BeforeRecord=B->CombatRecordV65[0];
 const FString Path=AccountPath(),Backup=Path+TEXT(".bak");TArray<uint8> PrimaryBytes,BackupBytes;
 const bool PrimaryHad=IFileManager::Get().FileExists(*Path),BackupHad=IFileManager::Get().FileExists(*Backup);
 if((PrimaryHad&&!FFileHelper::LoadFileToArray(PrimaryBytes,*Path))||(BackupHad&&!FFileHelper::LoadFileToArray(BackupBytes,*Backup))){Check(TEXT("v8215_medal_disposable_snapshot"),false);return;}
 ON_SCOPE_EXIT
 {
  FrontlineUltimateArchiveV8215::Leave(B);Account=BeforeAccount;Encounters.Add(B,BeforeEncounter);B->Phase=BeforePhase;B->BattleOutroWinner=BeforeOutro;B->EndReason=BeforeReason;B->CombatRecordV65[0]=BeforeRecord;
  if(PrimaryHad)FFileHelper::SaveArrayToFile(PrimaryBytes,*Path);else IFileManager::Get().Delete(*Path);
  if(BackupHad)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
 };
 const auto Fingerprint=[&](){FString Result;for(const auto& R:B->GetCampaignRuns())Result+=FString::Printf(TEXT("%s:%d:%d:%d:%u:%d:%d:%u:%u;%s;%s"),*R.Id.ToString(),int32(R.Faction),int32(R.Difficulty),R.CompletedMissions,R.Research.TechnologyMask,R.Research.ResearchPoints,R.SelectedMission,R.WonOperationMaskV52,R.ArmourResearchMaskV63,*R.CreatedUtc,*R.UpdatedUtc);return Result;};
 Check(TEXT("v8215_medal_fixture_empty_account_saved"),SeedMedalReviewV8215(B,false,0));
 Check(TEXT("v8215_archive_closed_without_completed_route"),!FrontlineUltimateArchiveV8215::Available(EFrontlineFaction::British)&&!FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction::British,0,EFrontlineDifficulty::Veteran));
 Check(TEXT("v8215_medal_fixture_completed_account_saved"),SeedMedalReviewV8215(B,true,0));
 const FString RunsBefore=Fingerprint();const auto* Original=B->GetCurrentCampaign();const FGuid OriginalId=Original?Original->Id:FGuid();
 const bool Began=FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction::British,0,EFrontlineDifficulty::Veteran);
 Check(TEXT("v8215_archive_completed_route_can_replay_first_operation"),Began);
 if(Began)
 {
  const auto* R=B->GetCurrentCampaign();const FGuid ReplayId=R?R->Id:FGuid();
  Check(TEXT("v8215_archive_unique_campaign_full_research"),R&&ReplayId!=OriginalId&&R->ArmourResearchMaskV63==511&&R->Research.TechnologyMask==((1u<<int32(EFrontlineTechnology::Count))-1u));
  const FString ReplayBeforeWrites=Fingerprint();const bool WritesBlocked=!B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Rookie)&&!B->ContinueCampaign()&&!B->DeleteCampaignArchives(TArray<FGuid>{ReplayId});Check(TEXT("v8215_archive_normal_campaign_mutators_fail_closed"),WritesBlocked&&Fingerprint()==ReplayBeforeWrites);
  bool Roles=true,Support=true;for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(FrontlineGameplayV52::SelectableRole(EFrontlineRole(I))&&FrontlineCampaignV41::RoleAllowed(B->GetPlayerFaction(),EFrontlineRole(I)))Roles&=B->IsRoleUnlocked(EFrontlineRole(I));
  for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)if(EFrontlineSupport(I)!=EFrontlineSupport::Rally)Support&=B->IsSupportUnlocked(EFrontlineSupport(I));
  Check(TEXT("v8215_archive_existing_player_units_and_support_available"),Roles&&Support&&FrontlineUltimateArchiveV8215::FullArmory(B,0)&&!FrontlineUltimateArchiveV8215::FullArmory(B,1));
  const int32 ResearchBefore=B->GetResearchPoints();const uint32 TechBefore=B->GetActiveResearch().TechnologyMask;
  const bool Disabled=FrontlineUltimateArchiveV8215::SetTechnologyEnabled(B,0,false)&&!B->HasTechnology(0);
  const bool Reenabled=FrontlineUltimateArchiveV8215::SetTechnologyEnabled(B,0,true)&&B->HasTechnology(0);
  const bool ArmourDisabled=FrontlineUltimateArchiveV8215::SetArmourTechnologyEnabled(B,8,false)&&!FrontlineArmourV63::Has(B,8);
  const bool ArmourEnabled=FrontlineUltimateArchiveV8215::SetArmourTechnologyEnabled(B,8,true)&&FrontlineArmourV63::Has(B,8);
  Check(TEXT("v8216_archive_optional_tech_disabled_and_restored_without_cost"),Disabled&&Reenabled&&ArmourDisabled&&ArmourEnabled&&B->GetResearchPoints()==ResearchBefore&&B->GetActiveResearch().TechnologyMask==TechBefore);
  auto ChooseRole=[&](EFrontlineRole Role){auto L=FrontlineCampaignV32::GetLoadout(B);if(FrontlineCampaignV32::RoleSelected(B,Role))return true;for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if((L.SelectedRoles&(1u<<I))&&FrontlineCampaignV32::BitCount(L.SelectedRoles)>=L.RoleCapacity){if(!FrontlineCampaignV32::ToggleRole(B,EFrontlineRole(I)))return false;break;}return FrontlineCampaignV32::ToggleRole(B,Role)&&FrontlineCampaignV32::RoleSelected(B,Role);};
  auto ChooseSupport=[&](EFrontlineSupport Kind){auto L=FrontlineCampaignV32::GetLoadout(B);if(FrontlineCampaignV32::SupportSelected(B,Kind))return true;for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)if((L.SelectedSupports&(1u<<I))&&FrontlineCampaignV32::BitCount(L.SelectedSupports)>=L.SupportCapacity){if(!FrontlineCampaignV32::ToggleSupport(B,EFrontlineSupport(I)))return false;break;}return FrontlineCampaignV32::ToggleSupport(B,Kind)&&FrontlineCampaignV32::SupportSelected(B,Kind);};
  Check(TEXT("v8216_archive_late_rosters_can_be_selected_for_first_operation"),ChooseRole(EFrontlineRole::Flamethrower)&&ChooseSupport(EFrontlineSupport::AirBombing));
  bool MissionGate=true;for(const auto& M:B->GetCampaignMissions())MissionGate&=B->CampaignMissionBlockReason(M.Id).IsEmpty();Check(TEXT("v8215_archive_all_operation_choices_open"),MissionGate);
  bool Bound=true;for(const auto& M:B->GetCampaignMissions())Bound&=FFrontlineAchievementReviewV76::ArchiveBindV8215(B,M.Id);Bound&=FFrontlineAchievementReviewV76::ArchiveBindV8215(B,0);Check(TEXT("v8215_archive_all_operation_encounters_bind_without_fake_wins"),Bound);
  const int32 Points=R?R->Research.ResearchPoints:-1,Completed=R?R->CompletedMissions:-1;const bool Saved=FFrontlineAchievementReviewV76::ArchiveRewardAndSaveV8215(B);Check(TEXT("v8215_archive_no_campaign_or_research_reward_mutation"),B->GetCurrentCampaign()->Research.ResearchPoints==Points&&B->GetCurrentCampaign()->CompletedMissions==Completed&&B->GetLastResearchReward()==0&&Saved);
  BeginEncounter(B);B->Phase=EFrontlinePhase::Victory;B->EndReason=TEXT("突破");B->BattleOutroWinner=0;
  B->CombatRecordV65[0].Total.Infantry=31;B->CombatRecordV65[0].Total.Armour=2;
  State(B).GasEnemy=10;State(B).GasFriendly=0;Finish(B,0);
  const auto SavedFirst=Account.Medals[0];Check(TEXT("v8215_archive_legitimate_war_record_and_kills_settle"),Earned(8)&&SavedFirst.Infantry==31&&SavedFirst.Armour==2&&SavedFirst.Victories==40&&SavedFirst.NaturalRoute==3);
  Finish(B,0);Check(TEXT("v8215_archive_encounter_receipt_prevents_double_counts"),Account.Medals[0].Infantry==31&&Account.Medals[0].Armour==2&&Account.MedalReceiptsV8215.Num()==1);
  FFrontlineAchievementReviewV76::ArchiveResetV8215(B);BeginEncounter(B);MarkCheat(B);B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;
  B->CombatRecordV65[0].Total.Infantry=47;B->CombatRecordV65[0].Total.Armour=3;State(B).GasEnemy=10;Finish(B,0);
  Check(TEXT("v8215_archive_assistance_revokes_replay_awards"),!FrontlineUltimateArchiveV8215::ReplayEligible(B)&&!State(B).Eligible&&Account.Medals[0].Infantry==31&&Account.Medals[0].Armour==2&&Account.MedalReceiptsV8215.Num()==1);
  FrontlineUltimateArchiveV8215::Leave(B);Check(TEXT("v8215_archive_original_campaign_snapshot_restored"),Fingerprint()==RunsBefore&&(!OriginalId.IsValid()||(B->GetCurrentCampaign()&&B->GetCurrentCampaign()->Id==OriginalId)));
 }
 B->Phase=EFrontlinePhase::Briefing;
 Check(TEXT("v8215_medal_fixture_all_levels_saved"),SeedMedalReviewV8215(B,true,3));
 Account=FAccount();Load();Check(TEXT("v8216_real_grades_bars_survive_account_reload"),FrontlineMedalsV8215::Tier(0)==1&&FrontlineMedalsV8215::Award(0).Repeats==2&&FrontlineMedalsV8215::Tier(1)==1&&FrontlineMedalsV8215::Award(1).Repeats==2&&FrontlineMedalsV8215::Tier(2)==1&&FrontlineMedalsV8215::Award(2).Repeats==1&&FrontlineMedalsV8215::Tier(3)==3&&FrontlineMedalsV8215::Award(5).Attachments==uint16(FrontlineMedalsV8215::EAttachment::OakLeaves));
 Check(TEXT("v8216_campaign_deed_deduplication_survives_account_reload"),FrontlineMedalsV8215::Award(2).CampaignDeeds.Num()==2&&FrontlineMedalsV8215::Award(5).CampaignDeeds.Num()==2&&FrontlineMedalsV8215::Award(21).CampaignDeeds.Num()==2);
 Check(TEXT("v8215_medal_national_filters_do_not_unlock_paused_campaigns"),FrontlineMedalsV8215::Tier(6)==0&&FrontlineMedalsV8215::Tier(11)==0&&!FrontlineMedalsV8215::Available(EFRRUNationV81::French)&&!FrontlineMedalsV8215::Available(EFRRUNationV81::Russian));
 const auto UK=FrontlineMedalsV8215::Ribbons(EFrontlineFaction::British),DE=FrontlineMedalsV8215::Ribbons(EFrontlineFaction::German);Check(TEXT("v8443_ribbon_precedence_retains_previously_earned_chest_classes"),UK==TArray<int32>{2,12,0,1,13,16,17}&&DE==TArray<int32>{3,21,22,20,18,19,24,23});
 const auto Highest=Account.MedalAwardsV8216;
 for(int32 I:FrontlineMedalsV8215::NationIndices(EFRRUNationV81::German))
  if(auto* A=Account.MedalAwardsV8216.Find(FrontlineMedalsV8215::Definition(I).Id))A->Grade=1;
 Check(TEXT("v8443_low_and_high_german_grades_keep_the_same_ribbon_ids"),FrontlineMedalsV8215::Ribbons(EFrontlineFaction::German)==DE);
 Account.MedalAwardsV8216=Highest;
 FString Encoded;TSharedPtr<FJsonObject> LegacyJSON;if(FFileHelper::LoadFileToString(Encoded,*Path)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Encoded),LegacyJSON)&&LegacyJSON.IsValid())
 {LegacyJSON->SetNumberField(TEXT("schema"),4);LegacyJSON->RemoveField(TEXT("medal_awards_v8216"));FJsonSerializer::Serialize(LegacyJSON.ToSharedRef(),TJsonWriterFactory<>::Create(&Encoded));FAccount Legacy;Check(TEXT("v8216_schema4_preserves_counters_without_fake_grades"),Decode(Encoded,Legacy)&&Legacy.Medals[0].Infantry==8200&&Legacy.MedalAwardsV8216.IsEmpty());}
 else Check(TEXT("v8216_schema4_preserves_counters_without_fake_grades"),false);
 FFrontlineAchievementReviewV76::NewUnlockChecksV8215(B,Check);Check(TEXT("v8215_next_unlock_review_restores_original_runs"),Fingerprint()==RunsBefore);
}
