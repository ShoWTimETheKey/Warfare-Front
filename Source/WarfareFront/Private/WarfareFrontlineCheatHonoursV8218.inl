bool GrantAllForCheatV8218(AWarfareFrontlineGameMode* B,bool Honours,bool Medals,FString& Response)
{
 const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 auto Say=[&](const TCHAR* EN,const TCHAR* CN){Response=ZH?CN:EN;};
 const bool Replay=FrontlineUltimateArchiveV8215::Active(B)&&!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
 if(!B||(!Honours&&!Medals)||B->IsProgressionReadOnly()||(B->IsProgressionVolatile()&&!IsolatedAwardAccountV8217()&&!Replay))
 {Say(TEXT("Cheat grants require a writable game account."),TEXT("当前账户不可写入，未授予。"));return false;}
 MarkCheat(B);State(B).Eligible=false;Load();
 if(Account.Corrupt||Account.Unsupported){Say(TEXT("Account records are unavailable; no grants saved."),TEXT("账户记录不可用，未授予。"));return false;}
 FAccountLock Lock;if(!Lock.Valid()||!MergeDisk()){Say(TEXT("Account is busy; retry the command."),TEXT("账户正在使用，请重试命令。"));return false;}
 const FAccount Before=Account;const auto* Run=B->GetCurrentCampaign();const FGuid Campaign=Run&&Run->Id.IsValid()?Run->Id:FGuid::NewGuid();
 if(Run&&Run->Id.IsValid())Account.Tainted.Add(Run->Id);
 const FString Date=FDateTime::UtcNow().ToIso8601();
 if(Honours)Account.CheatHonoursV8218|=AllHonoursMaskV82;
 if(Medals)for(int32 I=0;I<FrontlineMedalsV8215::Count;++I)
 {
  if(I==4)continue;const auto& D=FrontlineMedalsV8215::Definition(I);auto A=FrontlineMedalsV8215::CompleteCheatAwardV8218(I,Campaign,Date);
  if(!FrontlineMedalsV8215::ValidAward(I,A)){Account=Before;Say(TEXT("Decoration catalogue rejected a grant; no grants saved."),TEXT("勋章授予格式不符，未保存。"));return false;}
  if(const auto* Prior=Account.CheatMedalAwardsV8222.Find(D.Id)){A.Repeats=FMath::Max(A.Repeats,Prior->Repeats);if(!Prior->Date.IsEmpty())A.Date=Prior->Date;}
  Account.CheatMedalAwardsV8222.Add(D.Id,MoveTemp(A));Account.CheatMedalsV8218.Add(D.Id);
 }
 bool PrimaryCommitted=false;if(!SaveLocked(&PrimaryCommitted))
 {
  if(!PrimaryCommitted)Account=Before;
  if(PrimaryCommitted)Say(TEXT("Cheat grants saved to primary; backup pending. Retry the command."),TEXT("作弊授予已写入主记录，备份未完成。请重试命令。"));else Say(TEXT("Cheat grants were not saved. Retry the command."),TEXT("作弊授予未保存，请重试命令。"));return false;
 }
 // Explicit cheated records are displayed separately from naturally earned
 // routes and feats. No campaign progress or normal award banners are forged.
 Say(TEXT("Cheat grants saved. Campaign achievement eligibility is disabled."),TEXT("作弊授予已保存。本战役不再获得正常战功。"));return true;
}
void RunCheatGrantChecksV8218(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&B->Phase==EFrontlinePhase::Briefing;
 Check(TEXT("v8218_cheat_grants_exact_disposable_account"),Safe);if(!Safe)return;
 FTransientReviewV81 Transient;FFrontlineAchievementReviewV76 RunScope(B);Load();const auto BeforeAccount=Account;const auto BeforeEncounter=State(B);const auto BeforeAwardBattle=IsolatedAwardBattle;
 const FString Path=AccountPath(),Backup=Path+TEXT(".bak");TArray<uint8> MainBytes,BackupBytes;
 const bool HadMain=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
 if((HadMain&&!FFileHelper::LoadFileToArray(MainBytes,*Path))||(HadBackup&&!FFileHelper::LoadFileToArray(BackupBytes,*Backup))){Check(TEXT("v8218_cheat_account_snapshot"),false);return;}
 ON_SCOPE_EXIT
 {
  Account=BeforeAccount;Encounters.Add(B,BeforeEncounter);IsolatedAwardBattle=BeforeAwardBattle;
  if(HadMain)FFileHelper::SaveArrayToFile(MainBytes,*Path);else IFileManager::Get().Delete(*Path);
  if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
  TArray<uint8> M,K;Check(TEXT("v8218_cheat_fixture_restores_account_bytes"),(HadMain?FFileHelper::LoadFileToArray(M,*Path)&&M==MainBytes:!IFileManager::Get().FileExists(*Path))&&(HadBackup?FFileHelper::LoadFileToArray(K,*Backup)&&K==BackupBytes:!IFileManager::Get().FileExists(*Backup)));
 };
 IsolatedAwardBattle=B;Account=FAccount();Account.Loaded=true;{FAccountLock Lock;Check(TEXT("v8218_cheat_empty_fixture_saved"),Lock.Valid()&&SaveLocked());}
 FString Response;ReviewFailSaveV81=true;const bool Failed=!GrantAllForCheatV8218(B,true,true,Response);ReviewFailSaveV81=false;
 Check(TEXT("v8218_cheat_failed_save_does_not_claim_grants"),Failed&&Account.CheatHonoursV8218==0&&Account.CheatMedalsV8218.IsEmpty());
 const auto NaturalRoutes=Account.Routes;const auto NaturalMask=Account.EarnedMask;const auto CountersUK=Account.Medals[0],CountersDE=Account.Medals[1];
 const bool Granted=GrantAllForCheatV8218(B,true,true,Response);FAccount Disk;const bool Saved=ReadFile(Path,Disk);
 bool Exact=true;int32 Categories=0,Forms=0;
 for(int32 N=0;N<4;++N)for(int32 I:FrontlineMedalsV8215::NationIndices(EFRRUNationV81(N)))
 {const auto A=FrontlineMedalsV8215::Award(I);Exact&=FrontlineMedalsV8215::ValidAward(I,A)&&A.Grade==FrontlineMedalsV8215::Variants(I).Num()&&CheatGrantedMedalV8218(FrontlineMedalsV8215::Definition(I).Id);++Categories;Forms+=A.Grade;}
 Check(TEXT("v84412_cheat_all_52_categories_95_real_forms_only"),Granted&&Saved&&Exact&&Categories==52&&Forms==95&&Disk.CheatMedalsV8218.Num()==52&&!Disk.MedalAwardsV8216.Contains(FrontlineMedalsV8215::Definition(4).Id));
 Check(TEXT("v8218_cheat_all_honours_marked_without_natural_route_forgery"),Disk.CheatHonoursV8218==AllHonoursMaskV82&&EarnedCount()==Count&&Disk.EarnedMask==NaturalMask&&Disk.Routes==NaturalRoutes&&Status(0,false).Contains(TEXT("cheat")));
 Check(TEXT("v8218_cheat_does_not_inflate_kills_or_unlock_closed_campaigns"),Account.Medals[0].Infantry==CountersUK.Infantry&&Account.Medals[1].Infantry==CountersDE.Infantry&&Account.Medals[0].Victories==CountersUK.Victories&&Account.Medals[1].Victories==CountersDE.Victories&&!FrontlineMedalsV8215::Available(EFRRUNationV81::French)&&!FrontlineMedalsV8215::Available(EFRRUNationV81::Russian));
 Check(TEXT("v8218_cheat_grants_do_not_emit_natural_award_banners"),TakeNotificationV81()==INDEX_NONE&&TakeMedalNotificationV8217().Index==INDEX_NONE);
 const auto Prior=Account.CheatMedalAwardsV8222;const bool Repeated=GrantAllForCheatV8218(B,true,true,Response);bool Same=Repeated&&Prior.Num()==Account.CheatMedalAwardsV8222.Num();for(const auto& Entry:Prior){const auto A=Account.CheatMedalAwardsV8222.FindRef(Entry.Key);Same&=A.Grade==Entry.Value.Grade&&A.Repeats==Entry.Value.Repeats&&A.Attachments==Entry.Value.Attachments;}
 Check(TEXT("v8218_cheat_repeat_command_does_not_create_extra_bars"),Same&&Account.Routes==NaturalRoutes);
}
