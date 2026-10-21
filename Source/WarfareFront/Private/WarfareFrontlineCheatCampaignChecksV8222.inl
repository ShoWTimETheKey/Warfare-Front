bool SeedAssistedArchiveReviewV8222(AWarfareFrontlineGameMode* B)
{
    FString Dir;
    if(!B||!B->IsProgressionVolatile()||B->IsProgressionReadOnly()
        ||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)||WarfareFRRUCampaignV82::Active(B)
        ||FrontlineUltimateArchiveV8215::Active(B)||!B->GetCurrentCampaign())return false;
    const FGuid Campaign=B->GetCurrentCampaign()->Id;if(!Campaign.IsValid())return false;
    const auto Before=IsolatedAwardBattle;IsolatedAwardBattle=B;
    ON_SCOPE_EXIT{IsolatedAwardBattle=Before;};
    MarkCheat(B);const auto* Run=B->GetCurrentCampaign();
    return Run&&Run->Id==Campaign&&!Run->bAchievementEligibleV76&&CampaignAssistedV8222(Campaign);
}
void RunCheatCampaignIsolationChecksV8222(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString Dir;
    const bool Safe=B&&B->IsProgressionVolatile()&&!B->IsProgressionReadOnly()
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&!WarfareFRRUCampaignV82::Active(B)
        &&!FrontlineUltimateArchiveV8215::Active(B)&&!FrontlineDebugV37::HasCheatState(B);
    Check(TEXT("v8222_cheat_campaign_checks_exact_disposable_account"),Safe);if(!Safe)return;
    const FString Path=AccountPath(),Backup=Path+TEXT(".bak");TArray<uint8> MainBytes,BackupBytes;
    const bool HadMain=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
    const bool Snapshot=(!HadMain||FFileHelper::LoadFileToArray(MainBytes,*Path))&&(!HadBackup||FFileHelper::LoadFileToArray(BackupBytes,*Backup));
    Check(TEXT("v8222_cheat_campaign_snapshots_disposable_files"),Snapshot);if(!Snapshot)return;
    Load();const auto BeforeAccount=Account;const auto BeforeAwardBattle=IsolatedAwardBattle;const auto BeforeEncounter=State(B);
    FTransientReviewV81 PendingScope;FFrontlineCheatCampaignReviewV8222 ArchiveScope(B);
    ON_SCOPE_EXIT
    {
        FString Response;FrontlineDebugV37::Execute(B,TEXT("reset"),Response);
        IsolatedAwardBattle=BeforeAwardBattle;Account=BeforeAccount;Encounters.Add(B,BeforeEncounter);
        if(HadMain)FFileHelper::SaveArrayToFile(MainBytes,*Path);else IFileManager::Get().Delete(*Path);
        if(HadBackup)FFileHelper::SaveArrayToFile(BackupBytes,*Backup);else IFileManager::Get().Delete(*Backup);
        TArray<uint8> Main,Mirror;
        Check(TEXT("v8222_cheat_campaign_restores_disposable_account_bytes"),
            (HadMain?FFileHelper::LoadFileToArray(Main,*Path)&&Main==MainBytes:!IFileManager::Get().FileExists(*Path))
            &&(HadBackup?FFileHelper::LoadFileToArray(Mirror,*Backup)&&Mirror==BackupBytes:!IFileManager::Get().FileExists(*Backup)));
    };
    ArchiveScope.Empty();IsolatedAwardBattle=B;Account=FAccount();Account.Loaded=true;
    {FAccountLock Lock;Check(TEXT("v8222_cheat_campaign_empty_account_saved"),Lock.Valid()&&SaveLocked());}
    FString Response;const bool MadeA=B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);
    const FGuid A=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    FrontlineDebugV37::Execute(B,TEXT("nocd on"),Response);FrontlineDebugV37::Execute(B,TEXT("research on"),Response);
    B->Phase=EFrontlinePhase::Fighting;FrontlineDebugV37::Execute(B,TEXT("speed 4"),Response);B->Phase=EFrontlinePhase::Briefing;
    Check(TEXT("v8222_cheat_campaign_A_marked_only_own_guid"),MadeA&&A.IsValid()&&CampaignAssistedV8222(A)
        &&B->GetCurrentCampaign()&&!B->GetCurrentCampaign()->bAchievementEligibleV76&&FrontlineDebugV37::HasCheatState(B));
    const bool MadeB=B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran);
    const FGuid Clean=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    const float Ready=B->Elapsed+40;B->RecruitReadyAt[0][0]=Ready;FrontlineDebugV37::Tick(B);BeginEncounter(B);
    Check(TEXT("v8222_cheat_campaign_B_drops_switches_rate_and_can_earn"),MadeB&&Clean.IsValid()&&Clean!=A&&State(B).Eligible
        &&!CampaignAssistedV8222(Clean)&&!FrontlineDebugV37::HasCheatState(B)&&!FrontlineDebugV37::UnlimitedResearch(B)
        &&B->BattleSpeed==1.f&&B->RecruitReadyAt[0][0]==Ready);
    const bool BackA=B->ContinueCampaign(A);FrontlineDebugV37::Execute(B,TEXT("reset"),Response);BeginEncounter(B);
    Check(TEXT("v8222_cheat_campaign_reset_and_resume_cannot_launder_A"),BackA&&CampaignAssistedV8222(A)&&!State(B).Eligible&&!FrontlineDebugV37::HasCheatState(B));
    FrontlineDebugV37::Execute(B,TEXT("research on"),Response);
    const bool RemovedA=B->DeleteCampaignArchives({A});BeginEncounter(B);
    Check(TEXT("v8222_cheat_campaign_delete_active_A_rebinds_clean_B"),RemovedA&&B->GetCurrentCampaign()&&B->GetCurrentCampaign()->Id==Clean
        &&State(B).Eligible&&!FrontlineDebugV37::HasCheatState(B)&&!FrontlineDebugV37::UnlimitedResearch(B)&&!CampaignAssistedV8222(Clean));
    Account=FAccount();Load();Check(TEXT("v8222_cheat_campaign_reload_preserves_taint_without_global_veto"),
        Account.Tainted.Contains(A)&&!Account.Tainted.Contains(Clean)&&CampaignAssistedV8222(A)&&!CampaignAssistedV8222(Clean));

    // A new tainted route fails to save its account marker. Its saved campaign
    // flag and pending GUID still block only that route, never another one.
    B->Phase=EFrontlinePhase::Briefing;const bool MadeFailure=B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);
    const FGuid Failure=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();ReviewFailSaveV81=true;
    FrontlineDebugV37::Execute(B,TEXT("research on"),Response);
    Check(TEXT("v8222_cheat_campaign_failed_marker_write_stays_campaign_scoped"),MadeFailure&&CampaignAssistedV8222(Failure)
        &&RetryTaintedV81.Contains(Failure)&&B->GetCurrentCampaign()&&!B->GetCurrentCampaign()->bAchievementEligibleV76);
    ReviewFailSaveV81=false;NextRetryV81=0;PersistSettlementsV81();const bool RestoredClean=B->ContinueCampaign(Clean);BeginEncounter(B);
    Check(TEXT("v8222_cheat_campaign_failed_marker_retry_does_not_block_clean_route"),RestoredClean&&State(B).Eligible&&!FrontlineDebugV37::HasCheatState(B));

    // Explicit all-grants are a separate presentation ledger. A later natural
    // victory must start its real decoration at its actual base class.
    const bool AllGranted=GrantAllForCheatV8218(B,true,true,Response);
    Check(TEXT("v8222_cheat_grants_have_no_natural_award_records"),AllGranted&&Account.MedalAwardsV8216.IsEmpty()
        &&Account.CheatMedalAwardsV8222.Num()==36&&Account.EarnedMask==0);
    B->Phase=EFrontlinePhase::Briefing;const bool MadeNatural=B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran);
    const FGuid Natural=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();ArchiveScope.ReadyForWin();ResetEncounter(B);BeginEncounter(B);
    for(int32 I=0;I<3;++I)ObstacleCleared(B,0,FVector(I*100,0,0));B->CombatRecordV65[0].Total.Infantry=600;
    B->Phase=EFrontlinePhase::Victory;Finish(B,0);FAccount Disk;const bool Persisted=ReadFile(Path,Disk);
    const FString DCM(TEXT("uk.distinguished_conduct"));
    Check(TEXT("v8222_clean_campaign_naturally_earns_achievement_after_all_cheat"),MadeNatural&&Persisted&&(Disk.EarnedMask&1)
        &&!CheatGrantedAchievementV8218(0)&&Status(0,false).StartsWith(TEXT("Earned"))&&TakeNotificationV81()==0);
    const auto NaturalAward=MedalAwardV8216(DCM);
    bool HasNaturalBanner=false;for(int32 I=0;I<FrontlineMedalsV8215::Count;++I){const auto Notice=TakeMedalNotificationV8217();if(Notice.Index==0)HasNaturalBanner=true;}
    Check(TEXT("v8222_clean_campaign_naturally_earns_base_medal_and_banner_after_all_cheat"),Persisted&&Disk.MedalAwardsV8216.FindRef(DCM).Grade==1
        &&NaturalAward.Grade==1&&NaturalAward.Repeats==0&&NaturalAward.Campaign==Natural&&!CheatGrantedMedalV8218(DCM)&&HasNaturalBanner);
    Account=FAccount();Load();Check(TEXT("v8222_clean_natural_and_cheat_ledgers_reload_independently"),Account.EarnedMask&1
        &&Account.CheatMedalAwardsV8222.Num()==36&&Account.MedalAwardsV8216.FindRef(DCM).Grade==1&&!CheatGrantedMedalV8218(DCM));
    const auto BeforeNatural=Account.MedalAwardsV8216.FindRef(DCM);const auto BeforeNaturalStats=Account.Medals[0];
    const bool GrantAfterNatural=GrantAllForCheatV8218(B,true,true,Response);
    Check(TEXT("v8222_later_cheat_grant_retains_already_natural_class_date_and_stats"),GrantAfterNatural
        &&Account.MedalAwardsV8216.FindRef(DCM).Grade==BeforeNatural.Grade&&Account.MedalAwardsV8216.FindRef(DCM).Date==BeforeNatural.Date
        &&Account.Medals[0].Infantry==BeforeNaturalStats.Infantry&&!CheatGrantedAchievementV8218(0)&&!CheatGrantedMedalV8218(DCM));

    // Recreate a schema-5 explicit grant using the production codec. Its art is
    // preserved as an overlay, while natural counters are preserved verbatim.
    FString Encoded;FFileHelper::LoadFileToString(Encoded,*Path);TSharedPtr<FJsonObject> Legacy;
    const bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Encoded),Legacy)&&Legacy.IsValid();
    FAccount Migrated;bool MigratedOK=false;
    if(Parsed)
    {
        Legacy->SetNumberField(TEXT("schema"),5);const auto Cheats=Legacy->TryGetField(TEXT("cheat_medal_awards_v8222"));
        if(Cheats){Legacy->SetField(TEXT("medal_awards_v8216"),Cheats);Legacy->RemoveField(TEXT("cheat_medal_awards_v8222"));FString Text;FJsonSerializer::Serialize(Legacy.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));MigratedOK=Decode(Text,Migrated);}
    }
    Check(TEXT("v8222_schema5_cheat_grants_migrate_without_forging_natural_awards"),MigratedOK&&Migrated.CheatMedalAwardsV8222.Num()==36
        &&Migrated.MedalAwardsV8216.IsEmpty()&&Migrated.Medals[0].Infantry==Account.Medals[0].Infantry&&Migrated.EarnedMask==Account.EarnedMask);
}
