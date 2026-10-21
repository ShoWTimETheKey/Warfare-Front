// Included inside FrontlineAchievementsV76: all writes target the explicitly
// allowlisted disposable account, and both files plus transient queues restore.
void RunMedalNotificationChecksV8217(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&B->Phase==EFrontlinePhase::Briefing;
 Check(TEXT("v8217_medal_notification_absolute_disposable_scope"),Safe);if(!Safe)return;
 FTransientReviewV81 Scope;Load();const auto BeforeAccount=Account;const auto BeforeAwardBattle=IsolatedAwardBattle;
 const auto BeforeEncounter=State(B);const auto BeforePhase=B->Phase;const auto BeforeRecord=B->CombatRecordV65[0];
 const FString Path=AccountPath(),Backup=Path+TEXT(".bak");TArray<uint8> MainBytes,BackupBytes;
 const bool HadMain=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
 if((HadMain&&!FFileHelper::LoadFileToArray(MainBytes,*Path))||(HadBackup&&!FFileHelper::LoadFileToArray(BackupBytes,*Backup))){Check(TEXT("v8217_medal_notification_snapshot"),false);return;}
 ON_SCOPE_EXIT
 {
  Account=BeforeAccount;IsolatedAwardBattle=BeforeAwardBattle;Encounters.Add(B,BeforeEncounter);B->Phase=BeforePhase;B->CombatRecordV65[0]=BeforeRecord;
  if(HadMain)FFileHelper::SaveArrayToFile(MainBytes,*Path);else IFileManager::Get().Delete(*Path);
  if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
  TArray<uint8> M,K;Check(TEXT("v8217_medal_notification_restores_account_bytes"),(HadMain?FFileHelper::LoadFileToArray(M,*Path)&&M==MainBytes:!IFileManager::Get().FileExists(*Path))&&(HadBackup?FFileHelper::LoadFileToArray(K,*Backup)&&K==BackupBytes:!IFileManager::Get().FileExists(*Backup)));
 };
 IsolatedAwardBattle=B;
 Check(TEXT("v8217_medal_fault_injection_uses_same_disposable_account_boundary"),IsolatedAwardAccountV8217()&&IsolatedAwardsAllowed(B));
 auto Empty=[&](){ReviewFailSaveV81=ReviewFailMirrorV81=ReviewFailLockV81=false;SettlementsV81.Empty();NotificationsV81.Empty();MedalNotificationsV8217.Empty();RetryTaintedV81.Empty();Account=FAccount();Account.Loaded=true;FAccountLock Lock;return Lock.Valid()&&SaveLocked();};
 auto Add=[&](int32 Nation,int32 Infantry,const FGuid& Campaign,const FGuid& Receipt,bool Notify,int32 Armour=0)
 {
  auto& Pending=SettlementsV81.FindOrAdd(Campaign);Pending.CampaignV8215=Campaign;Pending.NotifyMedalsV8217=Notify;
  FrontlineMedalsV8215::FStats Delta;Delta.Infantry=Infantry;Delta.Victories=1;Pending.MedalOutcomesV8215.Add(Receipt,TPair<int32,FrontlineMedalsV8215::FStats>(Nation,Delta));
  FrontlineMedalsV8215::FProof P;P.Campaign=Campaign;P.Nation=Nation;P.Date=19171120;P.Difficulty=1;P.Won=true;P.Armour=Armour;Pending.MedalProofsV8216.Add(Receipt,P);
 };
 auto Drain=[&](int32 Index){FMedalNotificationV8217 Found;for(int32 K=0;K<100;++K){const auto E=TakeMedalNotificationV8217();if(E.Index==INDEX_NONE)break;if(E.Index==Index)Found=E;}return Found;};
 Check(TEXT("v8217_medal_notification_empty_fixture"),Empty());const FGuid Campaign=FGuid::NewGuid(),Receipt=FGuid::NewGuid();
 Add(0,500,Campaign,Receipt,true);ReviewFailSaveV81=true;PersistSettlementsV81();
 Check(TEXT("v8217_medal_failed_primary_has_no_banner_or_award"),TakeMedalNotificationV8217().Index==INDEX_NONE&&!FrontlineMedalsV8215::Tier(0)&&HasPendingV81());
 ReviewFailSaveV81=false;NextRetryV81=0;const auto First=Drain(0);
 Check(TEXT("v8217_medal_first_persisted_class_emits_once"),First.Index==0&&First.Grade==1&&First.Repeats==0&&First.Receipt==Receipt&&!HasPendingV81()&&TakeMedalNotificationV8217().Index==INDEX_NONE);
 Add(0,500,Campaign,Receipt,true);PersistSettlementsV81();Check(TEXT("v8217_duplicate_medal_receipt_emits_nothing"),TakeMedalNotificationV8217().Index==INDEX_NONE&&Account.Medals[0].Infantry==500);
 const FGuid Repeat=FGuid::NewGuid();Add(0,1500,Campaign,Repeat,true);ReviewFailMirrorV81=true;PersistSettlementsV81();FAccount Primary;
 Check(TEXT("v8217_medal_primary_only_retains_notice_until_mirror"),ReadFile(Path,Primary)&&Primary.MedalAwardsV8216.FindRef(TEXT("uk.distinguished_conduct")).Repeats==1&&TakeMedalNotificationV8217().Index==INDEX_NONE&&HasPendingV81());
 ReviewFailMirrorV81=false;NextRetryV81=0;const auto Bar=Drain(0);
 Check(TEXT("v8217_medal_mirror_retry_emits_exact_real_bar_once"),Bar.Index==0&&Bar.Grade==1&&Bar.Repeats==1&&(Bar.Attachments&uint16(FrontlineMedalsV8215::EAttachment::Bar))&&Bar.Receipt==Repeat&&TakeMedalNotificationV8217().Index==INDEX_NONE&&Account.Medals[0].Infantry==2000);
 Check(TEXT("v8217_medal_class_fixture_empty"),Empty());const FGuid German=FGuid::NewGuid();Add(1,500,German,FGuid::NewGuid(),true);PersistSettlementsV81();const auto SecondClass=Drain(3);
 Add(1,0,German,FGuid::NewGuid(),true,4);PersistSettlementsV81();const auto FirstClass=Drain(3);
 Check(TEXT("v8217_medal_real_iron_cross_classes_notify_separately"),SecondClass.Index==3&&SecondClass.Grade==1&&FirstClass.Index==3&&FirstClass.Grade==2&&SecondClass.Receipt!=FirstClass.Receipt&&TakeMedalNotificationV8217().Index==INDEX_NONE);
 Check(TEXT("v8217_outro_kill_fixture_empty"),Empty());const FGuid TailCampaign=FGuid::NewGuid();const int32 TailNation=int32(B->GetPlayerFaction()),TailIndex=TailNation==0?0:3;Add(TailNation,500,TailCampaign,B->GetEncounterId(),true);PersistSettlementsV81();Drain(TailIndex);
 auto& E=State(B);E.Id=B->GetEncounterId();E.Campaign=TailCampaign;E.Started=E.Eligible=E.SettlementCaptured=true;E.CapturedInfantryKillsV8217=500;E.CapturedArmourKillsV8217=0;E.OutroKillsFinalizedV8217=false;
 B->Phase=EFrontlinePhase::Victory;B->CombatRecordV65[0].Total.Infantry=507;B->CombatRecordV65[0].Total.Armour=2;const auto AwardBeforeTail=FrontlineMedalsV8215::Award(TailIndex);
 FinalizeBattleKillsV8217(B);const auto AfterTail=Account.Medals[TailNation];const int32 TailReceipts=Account.MedalReceiptsV8215.Num();FinalizeBattleKillsV8217(B);
 Check(TEXT("v8217_outro_kills_add_once_without_new_awards_or_victories"),AfterTail.Infantry==507&&AfterTail.Armour==2&&AfterTail.Victories==1&&Account.Medals[TailNation].Infantry==507&&Account.MedalReceiptsV8215.Num()==TailReceipts&&TailReceipts==2&&FrontlineMedalsV8215::Award(TailIndex).Repeats==AwardBeforeTail.Repeats&&TakeMedalNotificationV8217().Index==INDEX_NONE);
 B->Phase=EFrontlinePhase::Briefing;
 Check(TEXT("v8217_medal_silent_fixture_empty"),Empty());Add(0,500,FGuid::NewGuid(),FGuid::NewGuid(),false);PersistSettlementsV81();Check(TEXT("v8217_silent_disposable_fixture_never_announces_award"),FrontlineMedalsV8215::Tier(0)==1&&TakeMedalNotificationV8217().Index==INDEX_NONE);
}

bool SeedMedalNotificationReviewV8217(AWarfareFrontlineGameMode* B,EFrontlineFaction Nation,bool Advance)
{
 FString Dir;if(!B||!B->IsProgressionVolatile()||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)||uint8(Nation)>1)return false;
 Load();if(!Advance){Account=FAccount();Account.Loaded=true;MedalNotificationsV8217.Empty();NotificationsV81.Empty();SettlementsV81.Empty();RetryTaintedV81.Empty();FAccountLock Lock;if(!Lock.Valid()||!SaveLocked())return false;}
 const FGuid Receipt=FGuid::NewGuid(),Campaign=FGuid::NewGuid();auto& P=SettlementsV81.FindOrAdd(Campaign);P.CampaignV8215=Campaign;P.NotifyMedalsV8217=true;
 FrontlineMedalsV8215::FStats D;D.Infantry=Advance?(Nation==EFrontlineFaction::British?1500:0):500;D.Victories=1;
 P.MedalOutcomesV8215.Add(Receipt,TPair<int32,FrontlineMedalsV8215::FStats>(int32(Nation),D));FrontlineMedalsV8215::FProof Proof;Proof.Campaign=Campaign;Proof.Nation=int32(Nation);Proof.Date=19171120;Proof.Difficulty=1;Proof.Won=true;Proof.Armour=Advance&&Nation==EFrontlineFaction::German?4:0;P.MedalProofsV8216.Add(Receipt,Proof);
 PersistSettlementsV81();return !HasPendingV81()&&Account.MedalReceiptsV8215.Contains(Receipt)&&!MedalNotificationsV8217.IsEmpty();
}
