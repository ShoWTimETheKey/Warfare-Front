// Included inside FrontlineAchievementsV76. All persistence is confined to the
// allowlisted disposable review profile; its bytes and transient state restore.
void RunAwardReceiptChecksV84414(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&!B->IsProgressionReadOnly()
  &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&!WarfareFRRUCampaignV82::Active(B)
  &&!FrontlineUltimateArchiveV8215::Active(B)&&!FrontlineDebugV37::HasCheatState(B);
 Check(TEXT("v84414_award_receipts_require_an_absolute_disposable_profile"),Safe);if(!Safe)return;
 FHonoursFileSnapshotV83 Files;const bool Snapshotted=Files.Capture(false);
 Check(TEXT("v84414_award_receipts_snapshot_disposable_account_and_mirror"),Snapshotted);if(!Snapshotted)return;
 Load();const auto BeforeAccount=Account;const auto BeforeBattle=IsolatedAwardBattle;
 FTransientReviewV81 Scope;
 ON_SCOPE_EXIT
 {
  Account=BeforeAccount;IsolatedAwardBattle=BeforeBattle;
  Check(TEXT("v84414_award_receipts_restore_disposable_account_bytes"),Files.Restore()&&Files.Matches());
 };
 IsolatedAwardBattle=B;Account=FAccount();Account.Loaded=true;
 {FAccountLock Lock;const bool Saved=Lock.Valid()&&SaveLocked();Check(TEXT("v84414_award_receipts_start_from_a_saved_empty_fixture"),Saved);if(!Saved)return;}
 const FString Service=FrontlineMedalsV8215::Definition(37).Id,Flying=FrontlineMedalsV8215::Definition(39).Id;
 auto Add=[&](const FGuid& Campaign,const FGuid& Receipt)
 {
  auto& Pending=SettlementsV81.FindOrAdd(Campaign);Pending.CampaignV8215=Campaign;Pending.NotifyMedalsV8217=true;
  FrontlineMedalsV8215::FStats Delta;Delta.Armour=2;Delta.Victories=1;
  Pending.MedalOutcomesV8215.Add(Receipt,TPair<int32,FrontlineMedalsV8215::FStats>(0,Delta));
  FrontlineMedalsV8215::FProof P;P.Campaign=Campaign;P.Nation=0;P.Date=19180603;
  P.Difficulty=1;P.Won=true;P.SupportArmourV84412=2;P.AircraftArmourV84412=1;P.AircraftInfantryV84412=24;
  Pending.MedalProofsV8216.Add(Receipt,P);
 };
 auto NoticeCounts=[&](int32& S,int32& F,int32* Total=nullptr)
 {
  S=F=0;if(Total)*Total=0;for(int32 N=0;N<100;++N)
  {const auto E=TakeMedalNotificationV8217();if(E.Index==INDEX_NONE)break;if(Total)++*Total;if(E.Index==37)++S;if(E.Index==39)++F;}
 };
 const FGuid Campaign=FGuid::NewGuid(),First=FGuid::NewGuid();Add(Campaign,First);PersistSettlementsV81();
 int32 ServiceNotices=0,FlyingNotices=0;NoticeCounts(ServiceNotices,FlyingNotices);
 Check(TEXT("v84414_actual_atomic_settlement_grants_both_deeds_without_a_career_gate"),!HasPendingV81()&&Account.MedalReceiptsV8215.Contains(First)
  &&Account.MedalAwardsV8216.FindRef(Service).Grade==1&&Account.MedalAwardsV8216.FindRef(Flying).Grade==1
  &&ServiceNotices==1&&FlyingNotices==1&&Account.Medals[0].Armour==2);
 Add(Campaign,First);PersistSettlementsV81();NoticeCounts(ServiceNotices,FlyingNotices);
 Check(TEXT("v84414_repeated_encounter_receipt_cannot_create_a_bar_or_a_second_banner"),Account.MedalAwardsV8216.FindRef(Service).Repeats==0
  &&Account.MedalAwardsV8216.FindRef(Flying).Repeats==0&&ServiceNotices==0&&FlyingNotices==0&&Account.Medals[0].Armour==2);
 const FGuid Repeat=FGuid::NewGuid();Add(Campaign,Repeat);PersistSettlementsV81();NoticeCounts(ServiceNotices,FlyingNotices);
 const auto NaturalService=Account.MedalAwardsV8216.FindRef(Service),NaturalFlying=Account.MedalAwardsV8216.FindRef(Flying);
 Check(TEXT("v84414_new_encounter_receipt_grants_exactly_one_bar_and_one_notice_each"),NaturalService.Repeats==1&&NaturalFlying.Repeats==1
  &&ServiceNotices==1&&FlyingNotices==1&&Account.Medals[0].Armour==4&&Account.MedalReceiptsV8215.Contains(Repeat));
 Add(Campaign,Repeat);PersistSettlementsV81();NoticeCounts(ServiceNotices,FlyingNotices);
 Check(TEXT("v84414_resubmitted_repeat_receipt_remains_exactly_once"),Account.MedalAwardsV8216.FindRef(Service).Repeats==1
  &&Account.MedalAwardsV8216.FindRef(Flying).Repeats==1&&ServiceNotices==0&&FlyingNotices==0&&Account.Medals[0].Armour==4);
 const FGuid Assisted=FGuid::NewGuid(),AssistedReceipt=FGuid::NewGuid();Add(Assisted,AssistedReceipt);
 RetryTaintedV81.Add(Assisted);PersistSettlementsV81();NoticeCounts(ServiceNotices,FlyingNotices);
 Check(TEXT("v84414_cheat_taint_blocks_even_a_fully_qualified_new_deed_and_its_counter"),Account.Tainted.Contains(Assisted)
  &&!Account.MedalReceiptsV8215.Contains(AssistedReceipt)&&Account.Medals[0].Armour==4
  &&Account.MedalAwardsV8216.FindRef(Service).Repeats==NaturalService.Repeats
  &&Account.MedalAwardsV8216.FindRef(Flying).Repeats==NaturalFlying.Repeats&&ServiceNotices==0&&FlyingNotices==0);
 Account=FAccount();Load();
 const auto ReloadedService=Account.MedalAwardsV8216.FindRef(Service),ReloadedFlying=Account.MedalAwardsV8216.FindRef(Flying);
 Check(TEXT("v84414_existing_natural_bars_and_cheat_boundary_survive_account_reload"),!Account.Corrupt&&!Account.Unsupported
  &&ReloadedService.Grade==NaturalService.Grade&&ReloadedService.Repeats==NaturalService.Repeats&&ReloadedService.Date==NaturalService.Date
  &&ReloadedFlying.Grade==NaturalFlying.Grade&&ReloadedFlying.Repeats==NaturalFlying.Repeats&&ReloadedFlying.Date==NaturalFlying.Date
  &&Account.Tainted.Contains(Assisted)&&Account.MedalReceiptsV8215.Contains(First)&&Account.MedalReceiptsV8215.Contains(Repeat));
 bool Ceiling=true;
 for(int32 R=2;R<=6;++R)
 {
  const FGuid Receipt=FGuid::NewGuid();Add(Campaign,Receipt);PersistSettlementsV81();NoticeCounts(ServiceNotices,FlyingNotices);
  Ceiling&=!HasPendingV81()&&Account.MedalReceiptsV8215.Contains(Receipt)
   &&Account.MedalAwardsV8216.FindRef(Service).Repeats==FMath::Min(R,FrontlineMedalsV8215::RepeatLimitV85(37))
   &&Account.MedalAwardsV8216.FindRef(Flying).Repeats==FMath::Min(R,FrontlineMedalsV8215::RepeatLimitV85(39))
   &&ServiceNotices==(R<=FrontlineMedalsV8215::RepeatLimitV85(37)?1:0)&&FlyingNotices==(R<=FrontlineMedalsV8215::RepeatLimitV85(39)?1:0);
 }
 Check(TEXT("v85_atomic_receipts_stop_at_each_sourced_boundary_and_extra_deed_emits_no_banner"),Ceiling);
 for(int32 R:{6,9,255})
 {
  // Every repeatable career condition is already met; Add supplies the air
  // and support deed. A skipped qualification cannot fake a cap pass.
  Account.Medals[0].Infantry=50000;Account.Medals[0].Armour=1000;Account.Medals[0].Victories=Account.Medals[0].CleanVictories=1000;
  for(int32 I:{0,1,12,13,37,38,39})
  {
   auto A=FrontlineMedalsV8215::CompleteCheatAwardV8218(I,FGuid::NewGuid(),TEXT("2026-10-09T00:00:00Z"));
   A.Repeats=uint8(R);Account.MedalAwardsV8216.Add(FrontlineMedalsV8215::Definition(I).Id,A);
  }
  const auto Legacy=Account.MedalAwardsV8216;
  bool Saved=false;{FAccountLock Lock;Saved=Lock.Valid()&&SaveLocked();}
  Account=FAccount();Load();bool Retained=Saved&&!Account.Corrupt&&!Account.Unsupported;
  auto Same=[&]()
  {
   bool Equal=true;for(int32 I:{0,1,12,13,37,38,39})
   {
    const FString Id=FrontlineMedalsV8215::Definition(I).Id;const auto A=Account.MedalAwardsV8216.FindRef(Id),Old=Legacy.FindRef(Id);
    Equal&=A.Grade==Old.Grade&&A.Repeats==Old.Repeats&&A.Attachments==Old.Attachments&&A.Date==Old.Date
     &&A.Campaign==Old.Campaign&&A.CampaignDeeds==Old.CampaignDeeds&&FrontlineMedalsV8215::Award(I).Repeats==FrontlineMedalsV8215::RepeatLimitV85(I);
   }
   return Equal;
  };
  Retained&=Same();const FGuid Receipt=FGuid::NewGuid();Add(Campaign,Receipt);PersistSettlementsV81();int32 TotalNotices=0;NoticeCounts(ServiceNotices,FlyingNotices,&TotalNotices);
  Retained&=Same()&&!HasPendingV81()&&TotalNotices==0&&ServiceNotices==0&&FlyingNotices==0;
  Account=FAccount();Load();Retained&=Same()&&!Account.Corrupt&&!Account.Unsupported;
  Check(FString::Printf(TEXT("v85_all_seven_legacy_repeat_%d_records_roundtrip_raw_and_emit_no_new_repeat_notice"),R),Retained);
 }
}
