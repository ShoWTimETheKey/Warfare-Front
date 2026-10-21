void RunHonoursLifecycleChecksV83(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User;
    const bool Safe=B&&B->IsProgressionVolatile()&&!B->IsProgressionReadOnly()
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(User)&&!WarfareFRRUCampaignV82::Active(B)
        &&!FrontlineUltimateArchiveV8215::Active(B)&&!FrontlineDebugV37::HasCheatState(B);
    Check(TEXT("v83_honours_lifecycle_strict_disposable_account"),Safe);if(!Safe)return;
    FHonoursFileSnapshotV83 Original;const bool Snapshotted=Original.Capture(true);
    Check(TEXT("v83_honours_lifecycle_snapshots_all_four_files"),Snapshotted);if(!Snapshotted)return;
    Load();const auto BeforeAccount=Account;const auto BeforeEncounters=Encounters;const auto BeforeIsolated=IsolatedAwardBattle;
    const auto BeforeRecord0=B->CombatRecordV65[0],BeforeRecord1=B->CombatRecordV65[1];
    const bool BeforeTallyFault=ReviewFailTallyResetV83;
    FTransientReviewV81 QueueScope;
    ON_SCOPE_EXIT
    {
        ReviewFailSaveV81=ReviewFailMirrorV81=ReviewFailTallyResetV83=ReviewFailTallyMirrorV83=false;
        FString Response;FrontlineDebugV37::Execute(B,TEXT("reset"),Response);
        Account=BeforeAccount;Encounters=BeforeEncounters;IsolatedAwardBattle=BeforeIsolated;
        B->CombatRecordV65[0]=BeforeRecord0;B->CombatRecordV65[1]=BeforeRecord1;
        Check(TEXT("v83_honours_lifecycle_restores_disposable_account_bytes"),Original.Restore()&&Original.Matches());
        ReviewFailTallyResetV83=BeforeTallyFault;
    };
    FFrontlineCheatCampaignReviewV8222 ArchiveScope(B);
    ArchiveScope.Empty();IsolatedAwardBattle=B;Account=FAccount();Account.Loaded=true;
    {FAccountLock Lock;Check(TEXT("v83_honours_empty_fixture_saved"),Lock.Valid()&&SaveLocked());}
    const bool NaturalStarted=B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);
    const FGuid Natural=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    Account.EarnedMask=1;Account.Routes=1;Account.Dates.Add(Catalog[0].Id,TEXT("2026-10-06T00:00:00Z"));
    Account.CrossNationsV83[0]=1;Account.CrossDatesV83.Add(CrossDateKeyV83(0,0),TEXT("2026-10-06T00:00:00Z"));
    Account.Medals[0].Infantry=777;Account.Medals[0].Armour=12;Account.Medals[0].Victories=3;Account.Medals[0].CleanVictories=2;Account.Medals[0].NaturalRoute=1;
    const FGuid NaturalReceipt=FGuid::NewGuid();Account.MedalReceiptsV8215.Add(NaturalReceipt);
    const FString DCM(TEXT("uk.distinguished_conduct"));FrontlineMedalsV8215::FAward Genuine;
    Genuine.Grade=1;Genuine.Campaign=Natural;Genuine.Date=TEXT("2026-10-06T00:00:00Z");Account.MedalAwardsV8216.Add(DCM,Genuine);
    ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();ArmourTallyLedgerV8220.Loaded=true;
    const FGuid NaturalDeed=FGuid::NewGuid();ArmourTallyLedgerV8220.Totals[0]=1;ArmourTallyLedgerV8220.Receipts.Add(NaturalDeed,0);
    bool TallyPrimary=false;
    {FAccountLock Lock;Check(TEXT("v83_natural_award_and_deed_fixture_saved"),NaturalStarted&&Natural.IsValid()&&Lock.Valid()&&FrontlineMedalsV8215::ValidAward(0,Genuine)&&SaveLocked()&&SaveArmourTallyLockedV8220(TallyPrimary));}
    auto NaturalRetained=[&]()
    {
        const auto A=Account.MedalAwardsV8216.FindRef(DCM);
        return Account.EarnedMask==1&&Account.Routes==1&&Account.Dates.FindRef(Catalog[0].Id)==Genuine.Date
            &&Account.CrossNationsV83[0]==1&&Account.CrossDatesV83.FindRef(CrossDateKeyV83(0,0))==Genuine.Date
            &&Account.Medals[0].Infantry==777&&Account.Medals[0].Armour==12&&Account.Medals[0].Victories==3
            &&Account.Medals[0].CleanVictories==2&&Account.MedalReceiptsV8215.Contains(NaturalReceipt)
            &&A.Grade==1&&A.Campaign==Natural&&A.Date==Genuine.Date&&FrontlineArmourKillTallyV8220::Count(EFrontlineFaction::British)==1;
    };
    FString Response;const bool CheatStarted=B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran);
    const FGuid Assisted=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    const bool Granted=FrontlineDebugV37::Execute(B,TEXT("honours all"),Response);
    Check(TEXT("v83_overlay_fixture_leaves_natural_progress_untouched"),CheatStarted&&Granted&&NaturalRetained()&&Account.CheatHonoursV8218==AllHonoursMaskV82&&Account.CheatMedalAwardsV8222.Num()==36);
    const auto GrantedAccount=Account;FHonoursFileSnapshotV83 BeforeDelete;Check(TEXT("v83_delete_failure_snapshot"),BeforeDelete.Capture(false));
    const int32 Archives=B->GetCampaignRuns().Num();ReviewFailSaveV81=true;
    const bool FailedPrimary=!B->DeleteCampaignArchives({Assisted});ReviewFailSaveV81=false;
    Check(TEXT("v83_archive_delete_primary_failure_retains_archives_and_exact_honours"),FailedPrimary&&BeforeDelete.Matches()&&B->GetCampaignRuns().Num()==Archives&&Account.CheatHonoursV8218==AllHonoursMaskV82&&NaturalRetained());
    ReviewFailMirrorV81=true;const bool FailedMirror=!B->DeleteCampaignArchives({Assisted});ReviewFailMirrorV81=false;
    Check(TEXT("v83_archive_delete_primary_only_failure_rolls_back_both_files"),FailedMirror&&BeforeDelete.Matches()&&B->GetCampaignRuns().Num()==Archives&&NaturalRetained());
    int32 CommitCalls=0;
    const bool FailedArchive=!CommitArchiveDeletionV83(B,[&](){++CommitCalls;return false;},Response);
    Check(TEXT("v83_archive_save_failure_restores_cheat_overlays"),FailedArchive&&CommitCalls==1&&BeforeDelete.Matches()&&Account.CheatMedalAwardsV8222.Num()==36&&NaturalRetained());
    const bool Deleted=B->DeleteCampaignArchives({Assisted});
    Check(TEXT("v83_archive_delete_revokes_all_cheat_grants_preserves_legitimate_honours"),Deleted&&Account.CheatHonoursV8218==0&&Account.CheatMedalsV8218.IsEmpty()&&Account.CheatMedalAwardsV8222.IsEmpty()&&NaturalRetained()&&Account.Tainted.Contains(Assisted));
    Account=FAccount();Load();ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();
    Check(TEXT("v83_revocation_and_legitimate_progress_survive_reload"),NaturalRetained()&&Account.CheatHonoursV8218==0&&Account.CheatMedalAwardsV8222.IsEmpty());
    Account=GrantedAccount;const bool StaleSave=Save();
    Check(TEXT("v83_older_window_cannot_resurrect_revoked_cheat_overlay"),StaleSave&&Account.CheatHonoursV8218==0&&Account.CheatMedalAwardsV8222.IsEmpty()&&NaturalRetained());
    const bool LastStarted=B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran);
    const FGuid Last=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    const bool LastGranted=FrontlineDebugV37::Execute(B,TEXT("honours all"),Response);
    const bool DeletedAll=B->DeleteCampaignArchives({Natural,Last});
    Check(TEXT("v83_last_archive_delete_revokes_cheats_and_keeps_natural_account"),LastStarted&&LastGranted&&DeletedAll&&B->GetCampaignRuns().IsEmpty()&&Account.CheatHonoursV8218==0&&Account.CheatMedalAwardsV8222.IsEmpty()&&NaturalRetained()&&Account.Tainted.Contains(Last));
    const bool ResetRoute=B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);
    const FGuid ResetCampaign=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    FrontlineDebugV37::Execute(B,TEXT("honours all"),Response);
    const auto PreResetAccount=Account;FHonoursFileSnapshotV83 PreResetFiles;Check(TEXT("v83_reset_failure_snapshot"),PreResetFiles.Capture(true));
    const FGuid PendingCampaign=FGuid::NewGuid();auto& Pending=SettlementsV81.FindOrAdd(PendingCampaign);
    Pending.CampaignV8215=PendingCampaign;Pending.Mask=2;Pending.Dates.Add(Catalog[1].Id,TEXT("2026-10-06T01:00:00Z"));
    NotificationsV81.Add(1);MedalNotificationsV8217.Add(FMedalNotificationV8217());ArmourKillNotificationsV8220.Add(FrontlineArmourKillTallyV8220::FNotification());
    ReviewFailSaveV81=true;const bool ResetPrimaryFailed=!ResetAllHonoursV83(B,Response);ReviewFailSaveV81=false;
    Check(TEXT("v83_reset_primary_failure_rolls_back_all_progress_and_pending"),ResetPrimaryFailed&&PreResetFiles.Matches()&&NaturalRetained()&&SettlementsV81.Contains(PendingCampaign)&&!NotificationsV81.IsEmpty());
    ReviewFailMirrorV81=true;const bool ResetMirrorFailed=!ResetAllHonoursV83(B,Response);ReviewFailMirrorV81=false;
    Check(TEXT("v83_reset_primary_only_failure_rolls_back_all_four_files"),ResetMirrorFailed&&PreResetFiles.Matches()&&Account.ResetGenerationV83==PreResetAccount.ResetGenerationV83&&NaturalRetained());
    ReviewFailTallyResetV83=true;const bool ResetTallyFailed=!ResetAllHonoursV83(B,Response);ReviewFailTallyResetV83=false;
    Check(TEXT("v83_reset_tally_failure_rolls_back_account_and_deed_progress"),ResetTallyFailed&&PreResetFiles.Matches()&&Account.ResetGenerationV83==PreResetAccount.ResetGenerationV83&&NaturalRetained());
    ReviewFailTallyMirrorV83=true;const bool ResetTallyMirrorFailed=!ResetAllHonoursV83(B,Response);ReviewFailTallyMirrorV83=false;
    Check(TEXT("v83_reset_committed_tally_primary_mirror_failure_restores_all_four_files"),ResetTallyMirrorFailed&&PreResetFiles.Matches()&&Account.ResetGenerationV83==PreResetAccount.ResetGenerationV83&&NaturalRetained());
    const bool DamagedPrimary=FFileHelper::SaveStringToFile(TEXT("{broken-primary-v83"),*AccountPath());Account=FAccount();Load();
    const bool Recovered=DamagedPrimary&&Account.Recovered&&!Account.Corrupt;FHonoursFileSnapshotV83 RecoveredFiles;
    const bool RecoverySnapshot=RecoveredFiles.Capture(true);ReviewFailTallyMirrorV83=true;
    const bool RecoveredResetFailed=!ResetAllHonoursV83(B,Response);ReviewFailTallyMirrorV83=false;
    Check(TEXT("v83_recovered_backup_is_retained_when_reset_later_fails"),Recovered&&RecoverySnapshot&&RecoveredResetFailed&&RecoveredFiles.Matches()&&Account.Recovered&&NaturalRetained());
    Check(TEXT("v83_recovered_fixture_restores_supported_primary"),PreResetFiles.Restore());Account=PreResetAccount;
    const bool Reset=FrontlineDebugV37::Execute(B,TEXT("honours reset"),Response);
    const uint32 Generation=Account.ResetGenerationV83;
    Check(TEXT("v83_console_reset_clears_all_honours_progress_but_not_cheat_taints"),ResetRoute&&Reset&&Generation==PreResetAccount.ResetGenerationV83+1&&EarnedCount()==0&&Account.Routes==0&&Account.Dates.IsEmpty()&&Account.CrossDatesV83.IsEmpty()&&Account.MedalAwardsV8216.IsEmpty()&&Account.CheatMedalAwardsV8222.IsEmpty()&&Account.MedalReceiptsV8215.IsEmpty()&&Account.Medals[0].Infantry==0&&Account.Medals[0].Armour==0&&Account.Tainted.Contains(ResetCampaign)&&CampaignAssistedV8222(ResetCampaign)&&FrontlineArmourKillTallyV8220::Count(EFrontlineFaction::British)==0);
    bool NoCrossProgress=true;for(uint8 Mask:Account.CrossNationsV83)NoCrossProgress&=Mask==0;
    Check(TEXT("v83_console_reset_cancels_frozen_proofs_and_banners"),NoCrossProgress&&SettlementsV81.IsEmpty()&&PendingArmourTallyV8220.IsEmpty()&&NotificationsV81.IsEmpty()&&MedalNotificationsV8217.IsEmpty()&&ArmourKillNotificationsV8220.IsEmpty()&&!State(B).Eligible);
    Account=FAccount();Load();ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();
    Check(TEXT("v83_reset_survives_relaunch_without_progress_regrant"),Account.ResetGenerationV83==Generation&&EarnedCount()==0&&Account.Medals[0].Infantry==0&&FrontlineArmourKillTallyV8220::Count(EFrontlineFaction::British)==0&&Account.Tainted.Contains(ResetCampaign));
    const FString TallyPath=ArmourTallyPathV8220();bool OldTallyCopied=false;
    for(const auto& File:PreResetFiles.Files)if(File.Path==TallyPath)OldTallyCopied=FFileHelper::SaveArrayToFile(File.Bytes,*TallyPath);
    ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();
    Check(TEXT("v83_interrupted_tally_rewrite_cannot_restore_pre_reset_deeds"),OldTallyCopied&&FrontlineArmourKillTallyV8220::Count(EFrontlineFaction::British)==0&&ArmourTallyLedgerV8220.ResetGenerationV83==Generation);
    Account=PreResetAccount;auto& Stale=SettlementsV81.FindOrAdd(PendingCampaign);Stale.Mask=2;
    const bool Refreshed=Save();
    Check(TEXT("v83_stale_instance_reset_generation_drops_old_bits_and_settlements"),Refreshed&&Account.ResetGenerationV83==Generation&&EarnedCount()==0&&Account.Medals[0].Infantry==0&&SettlementsV81.IsEmpty()&&Account.Tainted.Contains(ResetCampaign));
    FString Encoded;TSharedPtr<FJsonObject> Legacy;FAccount Decoded;
    const bool Parsed=FFileHelper::LoadFileToString(Encoded,*AccountPath())&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Encoded),Legacy)&&Legacy.IsValid();
    bool LegacyRead=false;if(Parsed){Legacy->SetNumberField(TEXT("schema"),6);Legacy->RemoveField(TEXT("reset_generation_v83"));FJsonSerializer::Serialize(Legacy.ToSharedRef(),TJsonWriterFactory<>::Create(&Encoded));LegacyRead=Decode(Encoded,Decoded);}
    Check(TEXT("v83_schema6_account_reads_without_invented_reset_history"),LegacyRead&&Decoded.ResetGenerationV83==0&&Decoded.EarnedMask==Account.EarnedMask&&Decoded.Tainted.Num()==Account.Tainted.Num());
}
