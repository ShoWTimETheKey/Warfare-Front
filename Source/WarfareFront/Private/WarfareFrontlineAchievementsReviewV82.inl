// Deterministic event and settlement fixtures. These values are synthetic;
// genuine national battle acceptance is performed by the separate host review.
// Callers already snapshot/restore the disposable account and its mirror.
static void RunAccountSchemaChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile()||!DisposableAccount())return;
    const FString Path=AccountPath();IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".bak")));
    Account=FAccount();Account.Loaded=true;Account.EarnedMask=AllHonoursMaskV82;Account.Routes=15;
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)
    {Account.CrossNationsV83[Feat]=CrossRequiredNationsV83[Feat];for(int32 Nation=0;Nation<4;++Nation)if(CrossRequiredNationsV83[Feat]&(1u<<Nation))Account.CrossDatesV83.Add(CrossDateKeyV83(Feat,Nation),FString::Printf(TEXT("2026-10-%02dT00:00:00Z"),Nation+1));}
    FAccount Current;
    Check(TEXT("v836_schema8_thirty_bits_round_trip_without_overflow"),Save()&&ReadFile(Path,Current)&&Current.EarnedMask==AllHonoursMaskV82&&Current.CrossNationsV83[1]==3);
    FString FullText;FFileHelper::LoadFileToString(FullText,*Path);TSharedPtr<FJsonObject> FullJson;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FullText),FullJson);
    if(!FullJson.IsValid())return;
    const auto CurrentMedalRows=FullJson->GetArrayField(TEXT("medal_stats_v8215"));
    Check(TEXT("v87_schema7_fixture_starts_from_four_current_national_medal_rows"),CurrentMedalRows.Num()==4);if(CurrentMedalRows.Num()!=4)return;
    TArray<TSharedPtr<FJsonValue>> LegacyMedalRows={CurrentMedalRows[0],CurrentMedalRows[1]};
    FullJson->SetArrayField(TEXT("medal_stats_v8215"),LegacyMedalRows);
    FullJson->SetNumberField(TEXT("schema"),7);FullJson->SetNumberField(TEXT("earned"),double((1u<<TributeStartV836)-1u));
    FString PreviousText;FJsonSerializer::Serialize(FullJson,TJsonWriterFactory<>::Create(&PreviousText));FAccount Previous;
    Check(TEXT("v836_existing_schema7_preserves_original_twenty_eight_bits"),Decode(PreviousText,Previous)&&Previous.EarnedMask==(1u<<TributeStartV836)-1u);
    FullJson->SetNumberField(TEXT("earned"),double(1u<<TributeStartV836));FString WrongSchema;FJsonSerializer::Serialize(FullJson,TJsonWriterFactory<>::Create(&WrongSchema));
    Check(TEXT("v836_schema7_cannot_smuggle_new_tribute_bits"),!Decode(WrongSchema,Previous));
    FullJson->SetNumberField(TEXT("earned"),0);FullJson->SetNumberField(TEXT("cheat_honours_v8218"),double(1u<<TributeStartV836));
    FString WrongCheatSchema;FJsonSerializer::Serialize(FullJson,TJsonWriterFactory<>::Create(&WrongCheatSchema));
    Check(TEXT("v836_schema7_keeps_original_cheat_mask_boundary"),!Decode(WrongCheatSchema,Previous));
    FullJson->SetNumberField(TEXT("cheat_honours_v8218"),0);FullJson->SetNumberField(TEXT("schema"),AccountSchemaV82);
    FullJson->SetArrayField(TEXT("medal_stats_v8215"),CurrentMedalRows);
    FullJson->SetNumberField(TEXT("earned"),double(AllHonoursMaskV82)+1);FString Beyond;FJsonSerializer::Serialize(FullJson,TJsonWriterFactory<>::Create(&Beyond));
    Check(TEXT("v836_schema8_bit_outside_thirty_catalog_is_rejected"),!Decode(Beyond,Previous));
}
void RunNationalIsolatedChecksV82(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=B&&B->IsProgressionVolatile()&&DisposableAccount();
    Check(TEXT("v82_national_event_fixture_requires_disposable_account"),Allowed);if(!Allowed)return;
    FTransientReviewV81 Transient;
    const FString Path=AccountPath(),Backup=Path+TEXT(".bak");
    const bool HadPrimary=IFileManager::Get().FileExists(*Path),HadBackup=IFileManager::Get().FileExists(*Backup);
    TArray<uint8> Primary,Mirror;
    const bool Snapshot=(!HadPrimary||FFileHelper::LoadFileToArray(Primary,*Path))&&(!HadBackup||FFileHelper::LoadFileToArray(Mirror,*Backup));
    Check(TEXT("v82_national_fixture_snapshots_disposable_account"),Snapshot);if(!Snapshot)return;
    const FAccount BeforeAccount=Account;const auto BeforeAwardBattle=IsolatedAwardBattle;
    ON_SCOPE_EXIT
    {
        Account=BeforeAccount;IsolatedAwardBattle=BeforeAwardBattle;
        if(HadPrimary)FFileHelper::SaveArrayToFile(Primary,*Path);else IFileManager::Get().Delete(*Path);
        if(HadBackup)FFileHelper::SaveArrayToFile(Mirror,*Backup);else IFileManager::Get().Delete(*Backup);
        TArray<uint8> AfterPrimary,AfterMirror;
        Check(TEXT("v82_national_fixture_restores_existing_disposable_bytes"),
            (HadPrimary?FFileHelper::LoadFileToArray(AfterPrimary,*Path)&&AfterPrimary==Primary:!IFileManager::Get().FileExists(*Path))
            &&(HadBackup?FFileHelper::LoadFileToArray(AfterMirror,*Backup)&&AfterMirror==Mirror:!IFileManager::Get().FileExists(*Backup)));
    };
    IsolatedAwardBattle=B;
    const auto Clear=[&]()
    {
        IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*Backup);Account=FAccount();Account.Loaded=true;
        NotificationsV81.Empty();SettlementsV81.Empty();RetryTaintedV81.Empty();NextRetryV81=0;
        ReviewFailSaveV81=ReviewFailLockV81=ReviewFailMirrorV81=false;
    };
    const TCHAR* Missions[]={TEXT("fr.mort_homme"),TEXT("fr.vaux"),TEXT("fr.douaumont"),TEXT("fr.malmaison"),TEXT("fr.champagne_defence"),TEXT("fr.soissons"),
        TEXT("ru.galicia"),TEXT("ru.przemysl"),TEXT("ru.gorlice"),TEXT("ru.lutsk"),TEXT("ru.kovel"),TEXT("ru.riga")};
    const auto Proof=[&](int32 Index)
    {
        FNationalProofV82 P;P.Mission=P.StartedMission=Missions[Index-12];
        const auto* Definition=wfexp::tactical::FindMission(TCHAR_TO_UTF8(*P.Mission));
        P.Nation=P.StartedNation=Index<18?EFRRUNationV81::French:EFRRUNationV81::Russian;
        P.Date=P.StartedDate=Definition?Definition->date:0;P.Difficulty=P.StartedDifficulty=1;P.Alive=8;
        P.CampaignMatches=P.Eligible=P.Saved=P.Won=P.Cleared=P.TimelyBatteryV84=P.FT=true;
        FEncounter Events;Events.NationV82=P.Nation;Events.MissionV82=P.Mission;Events.DateV82=P.Date;
        if(Definition)for(const auto& Stage:Definition->stages)RecordNativeStageV82(Events,UTF8_TO_TCHAR(Stage.id.c_str()),false);
        P.Stages=Events.NativeStagesV82;
        for(int32 Role=0;Role<int32(EFrontlineRole::Count);++Role)
        {
            const auto* Equipment=wfexp::RoleFor(static_cast<wfexp::Faction>(uint8(P.Nation)),Role,P.Date);if(!Equipment||!Equipment->available)continue;
            FFrontlineKillSourceV65 Source;Source.Team=0;Source.SquadId=8200+Role;Source.Role=Role;
            Source.NationalSourceNationV82=int32(P.Nation);Source.NationalWeaponIdV82=UTF8_TO_TCHAR(Equipment->weaponId);
            for(int32 Kill=0;Kill<24;++Kill)RecordNationalLossV82(Events,1,Source,false);
            FNationalSectionV82 Row;Row.Squad=Source.SquadId;Row.Role=Role;Row.Alive=Equipment->size;Row.Weapon=Source.NationalWeaponIdV82;
            if(const auto* WeaponKills=Events.WeaponKillsV82.Find(Row.Squad))Row.Kills=WeaponKills->FindRef(Row.Weapon);
            Row.OriginalGunnerAlive=true;Row.SectionKillsV84=24;Row.OriginalSizeV84=Equipment->size;Row.FirstSectionV84=true;P.Sections.Add(Row);
        }
        return P;
    };
    uint32 NationalMask=0;
    for(int32 Index=12;Index<CrossFactionStartV83;++Index)
    {
        const auto P=Proof(Index);const FString Prefix=FString::Printf(TEXT("v82_%s_"),Catalog[Index].Id);
        const uint32 Expected=1u<<Index;
        Check(Prefix+TEXT("native_registered_date_and_exact_nation_events_qualify"),NationalMaskV82(P)==Expected);
        NationalMask|=NationalMaskV82(P);
        auto Negative=P;Negative.Nation=Negative.StartedNation=Index<18?EFRRUNationV81::Russian:EFRRUNationV81::French;
        Check(Prefix+TEXT("cross_faction_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.StartedNation=EFRRUNationV81::British;
        Check(Prefix+TEXT("changed_campaign_nation_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.CampaignMatches=false;
        Check(Prefix+TEXT("different_campaign_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Eligible=false;
        Check(Prefix+TEXT("cheat_assistance_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Saved=false;
        Check(Prefix+TEXT("unsaved_outcome_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Won=false;
        Check(Prefix+TEXT("failed_outcome_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Difficulty=Negative.StartedDifficulty=0;
        Check(Prefix+TEXT("rookie_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.StartedDifficulty=2;
        Check(Prefix+TEXT("changed_difficulty_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Date=Negative.StartedDate=P.Date-1;
        Check(Prefix+TEXT("incorrect_mission_date_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Stages.Remove(P.Mission+TEXT(".stage.3"));
        Check(Prefix+TEXT("missing_final_native_objective_cannot_qualify"),NationalMaskV82(Negative)==0);
        Negative=P;Negative.Fallback=true;
        Check(Prefix+TEXT("real_recovery_completion_remains_eligible"),NationalMaskV82(Negative)==Expected);
    }
    Check(TEXT("v82_twelve_new_national_bits_keep_original_homeward_index"),NationalMask==0x00fff000u&&HomewardIndexV82==11);
    Check(TEXT("v82_national_unentered_off_field_reinforcement_is_not_a_survivor"),!NationalSurvivorPresenceV82(0,true,false,false));
    Check(TEXT("v82_national_entered_living_soldier_remains_a_survivor_after_embarking"),NationalSurvivorPresenceV82(0,true,true,false));
    Check(TEXT("v82_national_dead_or_hostile_soldier_cannot_increase_survivors"),!NationalSurvivorPresenceV82(0,false,true,false)&&!NationalSurvivorPresenceV82(1,true,true,true));
    for(const int32 Index:{13,18,23})
    {
        const EFrontlineRole Role=Index==18?EFrontlineRole::Pioneer:EFrontlineRole::Signals;
        auto P=Proof(Index);
        for(auto& Row:P.Sections)if(Row.Role==int32(Role))Row.Alive=Row.OriginalSizeV84-1;
        Check(FString::Printf(TEXT("v84_%s_one_missing_original_member_blocks_intact_section"),Catalog[Index].Id),NationalMaskV82(P)==0);
        FNationalSectionV82 Replacement;Replacement.Role=int32(Role);Replacement.Alive=Replacement.OriginalSizeV84=Index==18?4:3;Replacement.FirstSectionV84=false;P.Sections.Add(Replacement);
        Check(FString::Printf(TEXT("v84_%s_intact_late_replacement_cannot_substitute_first_section"),Catalog[Index].Id),NationalMaskV82(P)==0);
        for(auto& Row:P.Sections)if(Row.Role==int32(Role)&&Row.FirstSectionV84)Row.Alive=Row.OriginalSizeV84-1+int32(NationalSurvivorPresenceV82(0,true,false,false));
        Check(FString::Printf(TEXT("v84_%s_unentered_reserve_is_not_last_original_survivor"),Catalog[Index].Id),NationalMaskV82(P)==0);
        for(auto& Row:P.Sections)if(Row.Role==int32(Role)&&Row.FirstSectionV84)Row.Alive=Row.OriginalSizeV84-1+int32(NationalSurvivorPresenceV82(0,true,true,false));
        Check(FString::Printf(TEXT("v84_%s_entered_embarked_original_survivor_keeps_section_intact"),Catalog[Index].Id),NationalMaskV82(P)==(1u<<Index));
        for(auto& Row:P.Sections)if(Row.Role==int32(Role))Row.OriginalSizeV84=0;
        Check(FString::Printf(TEXT("v84_%s_missing_original_size_is_not_an_intact_section"),Catalog[Index].Id),NationalMaskV82(P)==0);
    }
    for(const int32 Index:{12,20})
    {
        auto P=Proof(Index);P.Lost=4;P.Alive=0;
        Check(FString::Printf(TEXT("v84_%s_four_losses_meet_the_single_preservation_budget"),Catalog[Index].Id),NationalMaskV82(P)==(1u<<Index));
        P.Lost=5;Check(FString::Printf(TEXT("v84_%s_fifth_loss_blocks_preservation_budget"),Catalog[Index].Id),NationalMaskV82(P)==0);
    }
    const int32 WeaponTargets[][3]={{14,7,12},{16,1,18},{22,1,18}};
    for(const auto& Target:WeaponTargets)
    {
        auto P=Proof(Target[0]);for(auto& Row:P.Sections)if(Row.Role==Target[1])Row.Kills=Target[2]-1;
        Check(FString::Printf(TEXT("v84_%s_one_missing_exact_weapon_kill_blocks_target"),Catalog[Target[0]].Id),NationalMaskV82(P)==0);
    }
    const int32 SectionTargets[][3]={{19,0,16},{21,2,16}};
    for(const auto& Target:SectionTargets)
    {
        auto P=Proof(Target[0]);for(auto& Row:P.Sections)if(Row.Role==Target[1])Row.SectionKillsV84=Target[2]-1;
        Check(FString::Printf(TEXT("v84_%s_one_missing_section_kill_blocks_target"),Catalog[Target[0]].Id),NationalMaskV82(P)==0);
        FNationalSectionV82 Other;Other.Role=Target[1];Other.SectionKillsV84=1;P.Sections.Add(Other);
        Check(FString::Printf(TEXT("v84_%s_separate_sections_cannot_pool_required_tally"),Catalog[Target[0]].Id),NationalMaskV82(P)==0);
    }
    for(const int32 Index:{16,22})
    {
        auto P=Proof(Index);for(auto& Row:P.Sections)if(Row.Role==int32(EFrontlineRole::Lewis))Row.OriginalGunnerAlive=false;
        Check(FString::Printf(TEXT("v84_%s_original_gunner_must_survive"),Catalog[Index].Id),NationalMaskV82(P)==0);
    }
    auto NoBridge=Proof(18);NoBridge.Cleared=false;
    Check(TEXT("v84_galicia_first_engineers_need_actual_bridge_clearance"),NationalMaskV82(NoBridge)==0);
    auto BadBattery=Proof(15);BadBattery.TimelyBatteryV84=false;
    Check(TEXT("v84_malmaison_needs_the_timely_twelve_kill_fire_mission"),NationalMaskV82(BadBattery)==0);
    auto BadFT=Proof(17);BadFT.FT=false;
    Check(TEXT("v84_soissons_needs_one_eighteen_kill_half_hull_ft"),NationalMaskV82(BadFT)==0);
    FAccount Codec;
    Check(TEXT("v82_schema2_original_account_encoding_remains_valid"),Decode(TEXT("{\"schema\":2,\"earned\":1,\"routes\":1,\"tainted_campaigns\":[],\"dates\":{\"wire_crossing\":\"2026-09-01T00:00:00Z\"}}"),Codec)&&Codec.EarnedMask==1&&Codec.Dates.FindRef(TEXT("wire_crossing"))==TEXT("2026-09-01T00:00:00Z"));
    Check(TEXT("v82_schema2_twenty_four_bit_ceiling_is_safe"),Decode(TEXT("{\"schema\":2,\"earned\":16777215,\"routes\":15,\"tainted_campaigns\":[],\"dates\":{}}"),Codec)&&Codec.EarnedMask==LegacyAccountMaskV83);
    Check(TEXT("v82_schema2_out_of_catalog_bit_is_rejected"),!Decode(TEXT("{\"schema\":2,\"earned\":16777216,\"routes\":15,\"tainted_campaigns\":[],\"dates\":{}}"),Codec));

    // These run the same discharge-source counter used by the live Loss hook.
    FEncounter VB;VB.NationV82=EFRRUNationV81::French;VB.MissionV82=TEXT("fr.douaumont");VB.DateV82=19161024;
    FFrontlineKillSourceV65 Shot;Shot.Team=0;Shot.Role=int32(EFrontlineRole::RifleGrenadier);Shot.SquadId=827;
    Shot.NationalSourceNationV82=int32(EFRRUNationV81::French);Shot.NationalWeaponIdV82=TEXT("fr.vb");
    for(int32 I=0;I<8;++I)RecordNationalLossV82(VB,1,Shot,false);
    Check(TEXT("v82_eight_exact_vb_discharge_sources_count"),VB.WeaponKillsV82.FindChecked(827).FindRef(TEXT("fr.vb"))==8);
    const auto InvalidShot=[&](int32 Victim,int32 Nation,const TCHAR* Weapon,bool Armour)
    {auto Source=Shot;Source.NationalSourceNationV82=Nation;Source.NationalWeaponIdV82=Weapon;RecordNationalLossV82(VB,Victim,Source,Armour);};
    InvalidShot(0,2,TEXT("fr.vb"),false);InvalidShot(1,3,TEXT("fr.vb"),false);InvalidShot(1,2,TEXT("ru.mosin"),false);
    InvalidShot(1,2,TEXT("fr.vb"),true);InvalidShot(1,2,TEXT(""),false);
    InvalidShot(1,2,TEXT("fr.lebel"),false);
    Check(TEXT("v82_friendly_armour_foreign_and_unidentified_sources_do_not_add_vb_kills"),VB.WeaponKillsV82.FindChecked(827).FindRef(TEXT("fr.vb"))==8&&VB.WeaponKillsV82.FindChecked(827).FindRef(TEXT("fr.lebel"))==1);
    VB.DateV82=19160601;RecordNationalLossV82(VB,1,Shot,false);
    Check(TEXT("v82_vb_source_before_dated_issue_cannot_count"),VB.WeaponKillsV82.FindChecked(827).FindRef(TEXT("fr.vb"))==8);
    auto Split=Proof(14);for(auto& Row:Split.Sections)if(Row.Role==int32(EFrontlineRole::RifleGrenadier))Row.Kills=6;
    FNationalSectionV82 Second;Second.Role=int32(EFrontlineRole::RifleGrenadier);Second.Weapon=TEXT("fr.vb");Second.Kills=6;Second.Alive=2;Split.Sections.Add(Second);
    Check(TEXT("v84_separate_vb_teams_cannot_pool_twelve_kills"),NationalMaskV82(Split)==0);
    auto Dead=Proof(16);for(auto& Row:Dead.Sections)if(Row.Role==int32(EFrontlineRole::Lewis))Row.OriginalGunnerAlive=false;
    Check(TEXT("v82_replacement_hotchkiss_gunner_cannot_substitute_original"),NationalMaskV82(Dead)==0);

    FEncounter Kovel;Kovel.MissionV82=TEXT("ru.kovel");
    const bool FirstStage=RecordNativeStageV82(Kovel,TEXT("ru.kovel.stage.1"),false);
    Check(TEXT("v82_duplicate_and_foreign_native_stage_receipts_are_rejected"),FirstStage&&!RecordNativeStageV82(Kovel,TEXT("ru.kovel.stage.1"),false)&&!RecordNativeStageV82(Kovel,TEXT("fr.vaux.stage.3"),false)&&Kovel.NativeStagesV82.Num()==1);
    Check(TEXT("v82_clearance_tracks_authored_rail_and_rejects_duplicate_or_non_clearance_node"),RecordNativePassageV82(Kovel,TEXT("rail"))&&!RecordNativePassageV82(Kovel,TEXT("rail"))&&!RecordNativePassageV82(Kovel,TEXT("home"))&&Kovel.NativeClearancesV82.Num()==1);
    auto NoRail=Proof(22);NoRail.Cleared=false;
    Check(TEXT("v84_kovel_has_no_hidden_rail_clearance_condition"),NationalMaskV82(NoRail)==(1u<<22));
    FEncounter Clock;Clock.MissionV82=TEXT("fr.malmaison");Clock.Batteries.FindOrAdd(8201).Enemy=11;
    RecordNativeStageV82(Clock,TEXT("fr.malmaison.stage.2"),false);Clock.Batteries.FindChecked(8201).Enemy=12;
    Check(TEXT("v84_malmaison_twelfth_kill_after_approach_does_not_meet_clock_rule"),!Clock.Batteries.FindChecked(8201).BeforeNationalObjectiveV82);
    FEncounter BeforeClock;BeforeClock.MissionV82=TEXT("fr.malmaison");BeforeClock.Batteries.FindOrAdd(8201).Enemy=12;
    RecordNativeStageV82(BeforeClock,TEXT("fr.malmaison.stage.2"),false);
    Check(TEXT("v84_malmaison_twelve_kill_battery_is_frozen_at_actual_approach_receipt"),BeforeClock.Batteries.FindChecked(8201).BeforeNationalObjectiveV82);

    // Account writes below are scoped synthetic settlements, never battle wins.
    // The account mutex, merge, atomic primary/backup and original-date rules are
    // exactly the production code path, including the highest new bit (23).
    Clear();Account.EarnedMask=1;Account.Routes=1;
    const FString OldDate=TEXT("2026-09-01T00:00:00Z"),NewDate=TEXT("2026-10-03T00:00:00Z");
    const FGuid OldTaint=FGuid::NewGuid(),Campaign=FGuid::NewGuid();Account.Dates.Add(Catalog[0].Id,OldDate);Account.Tainted.Add(OldTaint);
    Check(TEXT("v82_national_settlement_preserves_original_schema2_seed"),Save());
    auto& Pending=SettlementsV81.FindOrAdd(Campaign);Pending.Mask=NationalMask;Pending.NotifyMask=1u<<23;
    for(int32 I=12;I<CrossFactionStartV83;++I)Pending.Dates.Add(Catalog[I].Id,NewDate);
    ReviewFailSaveV81=true;PersistSettlementsV81();
    Check(TEXT("v82_national_failed_atomic_write_reports_no_earned_bits_or_banner"),(Account.EarnedMask&NationalMask)==0&&NotificationsV81.IsEmpty()&&HasPendingV81());
    ReviewFailSaveV81=false;PersistSettlementsV81();FAccount PrimaryRead,MirrorRead;
    Check(TEXT("v82_national_atomic_save_preserves_original_bits_routes_taint_dates_and_highest_bit"),ReadFile(Path,PrimaryRead)&&ReadFile(Backup,MirrorRead)&&PrimaryRead.EarnedMask==(NationalMask|1u)&&MirrorRead.EarnedMask==PrimaryRead.EarnedMask&&PrimaryRead.Routes==1&&PrimaryRead.Tainted.Contains(OldTaint)&&PrimaryRead.Dates.FindRef(Catalog[0].Id)==OldDate&&PrimaryRead.Dates.FindRef(Catalog[23].Id)==NewDate);
    Check(TEXT("v82_national_committed_settlement_emits_highest_index_once"),TakeNotificationV81()==23&&TakeNotificationV81()==INDEX_NONE&&!HasPendingV81());
    PersistSettlementsV81();Account=FAccount();Load();
    Check(TEXT("v82_national_restart_read_retains_thirteen_awards_and_original_dates"),EarnedCount()==13&&Earned(23)&&!Earned(HomewardIndexV82)&&Account.Dates.FindRef(Catalog[23].Id)==NewDate&&Account.Dates.FindRef(Catalog[0].Id)==OldDate);
    auto& Again=SettlementsV81.FindOrAdd(Campaign);Again.Mask=1u<<23;Again.NotifyMask=1u<<23;Again.Dates.Add(Catalog[23].Id,TEXT("2026-10-04T00:00:00Z"));PersistSettlementsV81();
    Check(TEXT("v82_national_duplicate_settlement_keeps_first_date_without_second_banner"),Account.Dates.FindRef(Catalog[23].Id)==NewDate&&TakeNotificationV81()==INDEX_NONE);
    auto& Tainted=SettlementsV81.FindOrAdd(OldTaint);Tainted.Mask=1u<<1;Tainted.Routes=8;Tainted.Dates.Add(Catalog[1].Id,NewDate);PersistSettlementsV81();
    Check(TEXT("v82_national_saved_cheat_taint_blocks_pending_award_and_route"),!Earned(1)&&Account.Routes==1&&!HasPendingV81());

    // Cross-army values below are deterministic event-proof fixtures. They do
    // not manufacture a live battle or claim that a natural run earned a feat.
    const auto CrossProof=[](EFRRUNationV81 Nation)
    {
        FCrossBattleProofV83 P;P.Nation=P.StartedNation=Nation;P.Difficulty=P.StartedDifficulty=1;P.Date=P.StartedDate=19170409;
        P.CampaignMatches=P.Eligible=P.Saved=P.Won=P.Breakthrough=true;P.InfantryKills=30;P.FireKills=20;
        P.Alive=24;P.EnteredInfantryV842=24;P.MinimumMorale=.1f;
        for(const int32 Role:{int32(EFrontlineRole::Rifle),int32(EFrontlineRole::Lewis),int32(EFrontlineRole::Pioneer)})
            P.RoleKillsV84[Role]=10;
        return P;
    };
    const EFRRUNationV81 Nations[]={EFRRUNationV81::British,EFRRUNationV81::German,EFRRUNationV81::French,EFRRUNationV81::Russian};
    for(const auto Nation:Nations)
    {
        const auto P=CrossProof(Nation);const uint8 Expected=Nation==EFRRUNationV81::British||Nation==EFRRUNationV81::German?15:13;
        const FString Prefix=FString::Printf(TEXT("v83_cross_nation%d_"),int32(Nation));
        Check(Prefix+TEXT("real_counter_shaped_qualifying_proof"),CrossBattleFeatsV83(P)==Expected);
        auto Negative=P;Negative.Saved=false;Check(Prefix+TEXT("unsaved_outcome_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.Won=false;Check(Prefix+TEXT("defeat_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.Eligible=false;Check(Prefix+TEXT("cheat_assisted_campaign_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.CampaignMatches=false;Check(Prefix+TEXT("other_campaign_or_encounter_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.StartedNation=EFRRUNationV81::Invalid;Check(Prefix+TEXT("changed_nation_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.Difficulty=Negative.StartedDifficulty=0;Check(Prefix+TEXT("rookie_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.StartedDifficulty=2;Check(Prefix+TEXT("changed_difficulty_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.StartedDate=P.Date-1;Check(Prefix+TEXT("changed_battle_date_records_no_feat"),CrossBattleFeatsV83(Negative)==0);
        Negative=P;Negative.Difficulty=Negative.StartedDifficulty=2;Check(Prefix+TEXT("second_hard_difficulty_remains_eligible"),CrossBattleFeatsV83(Negative)==Expected);
    }
    auto CrossNegative=CrossProof(EFRRUNationV81::AustroHungarian);
    Check(TEXT("v83_cross_non_player_nation_cannot_add_progress"),CrossBattleFeatsV83(CrossNegative)==0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.Lost=8;CrossNegative.EnteredInfantryV842=32;
    Check(TEXT("v842_roll_call_eight_losses_with_twenty_four_returns_qualify"),(CrossBattleFeatsV83(CrossNegative)&1u)!=0);
    CrossNegative.EnteredInfantryV842=33;
    Check(TEXT("v842_roll_call_more_than_one_quarter_lost_remains_locked"),(CrossBattleFeatsV83(CrossNegative)&1u)==0);
    CrossNegative.EnteredInfantryV842=0;
    Check(TEXT("v842_roll_call_no_participation_roster_never_defaults_to_no_losses"),(CrossBattleFeatsV83(CrossNegative)&1u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.InfantryKills=0;CrossNegative.FireKills=0;
    Check(TEXT("v84_roll_call_is_preservation_not_a_hidden_kill_quota"),(CrossBattleFeatsV83(CrossNegative)&1u)!=0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.Alive=23+int32(NationalSurvivorPresenceV82(0,true,false,false));
    Check(TEXT("v84_pending_reinforcement_cannot_complete_twenty_four_survivors"),(CrossBattleFeatsV83(CrossNegative)&1u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.Alive=23+int32(NationalSurvivorPresenceV82(0,true,true,false));
    Check(TEXT("v84_entered_embarked_survivor_can_complete_twenty_four"),(CrossBattleFeatsV83(CrossNegative)&1u)!=0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.MinimumMorale=.11f;
    Check(TEXT("v84_both_sides_needs_ten_percent_morale_low"),(CrossBattleFeatsV83(CrossNegative)&2u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.Breakthrough=false;
    Check(TEXT("v83_both_sides_other_victory_is_not_a_breakthrough"),(CrossBattleFeatsV83(CrossNegative)&2u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::British);CrossNegative.Alive=1;CrossNegative.Lost=30;CrossNegative.InfantryKills=0;CrossNegative.FireKills=0;
    Check(TEXT("v84_both_sides_has_no_hidden_survivor_loss_or_kill_quotas"),(CrossBattleFeatsV83(CrossNegative)&2u)!=0);
    CrossNegative=CrossProof(EFRRUNationV81::French);CrossNegative.RoleKillsV84[int32(EFrontlineRole::Pioneer)]=9;
    Check(TEXT("v84_each_has_a_part_needs_ten_kills_from_third_role"),(CrossBattleFeatsV83(CrossNegative)&4u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::French);CrossNegative.RoleKillsV84[int32(EFrontlineRole::Pioneer)]=0;CrossNegative.RoleKillsV84[int32(EFrontlineRole::Rifle)]=100;
    Check(TEXT("v84_extra_kills_from_one_role_cannot_substitute_a_third_role"),(CrossBattleFeatsV83(CrossNegative)&4u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::French);CrossNegative.Alive=0;CrossNegative.Lost=30;
    Check(TEXT("v84_role_diversity_has_no_hidden_clearance_or_survivor_quotas"),(CrossBattleFeatsV83(CrossNegative)&4u)!=0);
    CrossNegative=CrossProof(EFRRUNationV81::Russian);CrossNegative.InfantryKills=19;
    Check(TEXT("v84_guns_and_men_cannot_replace_twenty_infantry_kills_with_fire"),(CrossBattleFeatsV83(CrossNegative)&8u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::Russian);CrossNegative.FireKills=19;
    Check(TEXT("v84_guns_and_men_cannot_replace_twenty_fire_mission_kills_with_infantry"),(CrossBattleFeatsV83(CrossNegative)&8u)==0);
    CrossNegative=CrossProof(EFRRUNationV81::Russian);CrossNegative.Alive=0;CrossNegative.Lost=30;
    Check(TEXT("v84_combined_arms_has_no_hidden_preservation_quota"),(CrossBattleFeatsV83(CrossNegative)&8u)!=0);

    // Upgrade the existing path and mutex, so a twenty-four-bit old reader sees
    // a newer schema and preserves it instead of recovering an older backup.
    Clear();const FString Legacy=TEXT("{\"schema\":2,\"earned\":16777215,\"routes\":15,\"tainted_campaigns\":[],\"dates\":{\"wire_crossing\":\"2026-09-01T00:00:00Z\",\"ru_riga\":\"2026-09-02T00:00:00Z\"}}");
    const bool LegacyWritten=FFileHelper::SaveStringToFile(Legacy,*Path)&&FFileHelper::SaveStringToFile(Legacy,*Backup);Account=FAccount();Load();
    Check(TEXT("v83_schema2_upgrade_preserves_all_original_twenty_four_bits_and_dates"),LegacyWritten&&EarnedCount()==24&&Account.EarnedMask==LegacyAccountMaskV83&&Account.Routes==15&&Account.Dates.FindRef(Catalog[0].Id)==OldDate&&Account.Dates.FindRef(Catalog[23].Id)==TEXT("2026-09-02T00:00:00Z")&&CompletedFeatNationMaskV83(24)==0);
    Check(TEXT("v83_cross_status_names_completed_and_remaining_required_nations"),Status(24,false).Contains(TEXT("Completed: None"))&&Status(24,false).Contains(TEXT("Britain, Germany, France, Russia"))&&Status(25,true).Contains(TEXT("英军、德军"))&&!Status(25,true).Contains(TEXT("俄军")));
    // Only the still-missing, currently closed nations delay a shared honour.
    {
        const FAccount BeforeStatus=Account;Account.EarnedMask=0;
        const bool FrenchClosed=!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French);
        const bool RussianClosed=!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian);
        Account.CrossNationsV83[0]=3;
        Check(TEXT("v87_cross_status_waits_for_missing_closed_nation"),Status(24,false).StartsWith(TEXT("Not yet available"))==(FrenchClosed||RussianClosed));
        Account.CrossNationsV83[0]=7;
        Check(TEXT("v87_cross_status_completed_france_only_missing_russia"),Status(24,false).StartsWith(TEXT("Not yet available"))==RussianClosed);
        Account.CrossNationsV83[0]=11;
        Check(TEXT("v87_cross_status_completed_russia_only_missing_france"),Status(24,true).StartsWith(TEXT("暂未开放"))==FrenchClosed);
        Account.CrossNationsV83[1]=0;
        Check(TEXT("v87_ukde_only_honour_does_not_wait_for_france_or_russia"),Status(25,false).StartsWith(TEXT("Not earned")));
        Account.Routes=7;
        Check(TEXT("v87_route_status_only_missing_russia"),Status(HomewardIndexV82,false).StartsWith(TEXT("Not yet available"))==RussianClosed);
        Account.Routes=11;
        Check(TEXT("v87_route_status_only_missing_france"),Status(HomewardIndexV82,true).StartsWith(TEXT("暂未开放"))==FrenchClosed);
        Account.CrossNationsV83[0]=15;Account.Routes=15;Account.EarnedMask=(1u<<24)|(1u<<HomewardIndexV82);
        Account.Dates.Add(Catalog[24].Id,OldDate);Account.Dates.Add(Catalog[HomewardIndexV82].Id,OldDate);
        Check(TEXT("v87_earned_cross_honour_precedes_campaign_gates"),Status(24,false).StartsWith(TEXT("Earned · ")));
        Check(TEXT("v87_earned_route_honour_precedes_campaign_gates"),Status(HomewardIndexV82,true).StartsWith(TEXT("达成 · ")));
        Account=BeforeStatus;
    }
    const bool Migrated=Save();FString MigratedText;TSharedPtr<FJsonObject> MigratedJson;FAccount MigratedPrimary,MigratedBackup;
    Check(TEXT("v87_upgrade_writes_current_schema_primary_and_backup_without_old_record_reset"),Migrated&&FFileHelper::LoadFileToString(MigratedText,*Path)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(MigratedText),MigratedJson)&&MigratedJson->GetIntegerField(TEXT("schema"))==AccountSchemaV82&&ReadFile(Path,MigratedPrimary)&&ReadFile(Backup,MigratedBackup)&&MigratedPrimary.EarnedMask==LegacyAccountMaskV83&&MigratedBackup.EarnedMask==LegacyAccountMaskV83&&MigratedPrimary.Dates.FindRef(Catalog[0].Id)==OldDate);
    if(!MigratedJson.IsValid())return;
    auto MissingLedger=MigratedJson;MissingLedger->RemoveField(TEXT("cross_feats"));FString MissingText;FJsonSerializer::Serialize(MissingLedger,TJsonWriterFactory<>::Create(&MissingText));FAccount InvalidCross;
    Check(TEXT("v83_schema3_missing_feat_ledger_is_rejected"),!Decode(MissingText,InvalidCross));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(MigratedText),MigratedJson);MigratedJson->SetNumberField(TEXT("earned"),AllHonoursMaskV82);
    FString UnprovedText;FJsonSerializer::Serialize(MigratedJson,TJsonWriterFactory<>::Create(&UnprovedText));
    Check(TEXT("v83_schema3_cross_earned_bits_need_all_required_nation_proofs"),!Decode(UnprovedText,InvalidCross));

    const auto Seed=[&]()
    {Clear();Account.EarnedMask=1;Account.Routes=1;Account.Dates.Add(Catalog[0].Id,OldDate);Check(TEXT("v83_cross_fixture_original_record_seed_saved"),Save());};
    const auto NationDate=[](int32 Nation){return FString::Printf(TEXT("2026-10-%02dT00:00:00Z"),Nation+1);};
    Seed();Account.EarnedMask|=1u<<25;Account.CrossNationsV83[1]=3;Account.CrossNationsV83[2]=1;
    Account.CrossDatesV83.Add(CrossDateKeyV83(1,0),OldDate);Account.CrossDatesV83.Add(CrossDateKeyV83(1,1),OldDate);Account.CrossDatesV83.Add(CrossDateKeyV83(2,0),OldDate);Account.Dates.Add(Catalog[25].Id,OldDate);
    const bool HistoricalSaved=Save();Account=FAccount();Load();
    auto NewRuleMiss=CrossProof(EFRRUNationV81::French);NewRuleMiss.Lost=1;NewRuleMiss.Alive=0;NewRuleMiss.InfantryKills=NewRuleMiss.FireKills=0;FMemory::Memzero(NewRuleMiss.RoleKillsV84,sizeof(NewRuleMiss.RoleKillsV84));
    CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),CrossBattleFeatsV83(NewRuleMiss),4,NationDate(2),true);PersistSettlementsV81();
    Check(TEXT("v84_rule_revision_keeps_historical_medal_date_and_partial_nation_proof"),HistoricalSaved&&Earned(25)&&Account.Dates.FindRef(Catalog[25].Id)==OldDate&&CompletedFeatNationMaskV83(25)==3&&CompletedFeatNationMaskV83(26)==1&&TakeNotificationV81()==INDEX_NONE);
    for(int32 Feat=0;Feat<CrossFactionCountV83;++Feat)
    {
        Seed();const int32 Index=CrossFactionStartV83+Feat;uint8 Collected=0;
        for(int32 Nation=0;Nation<4;++Nation)if(CrossRequiredNationsV83[Feat]&(1u<<Nation))
        {
            const uint8 Bit=uint8(1u<<Nation);const FGuid RunId=FGuid::NewGuid();
            CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(RunId),uint8(1u<<Feat),Bit,NationDate(Nation),true);PersistSettlementsV81();Collected|=Bit;
            const bool Complete=Collected==CrossRequiredNationsV83[Feat];
            Check(FString::Printf(TEXT("v83_%s_nation%d_only_saved_required_progress_is_visible"),Catalog[Index].Id,Nation),CompletedFeatNationMaskV83(Index)==Collected&&RequiredFeatNationMaskV83(Index)==CrossRequiredNationsV83[Feat]&&Earned(Index)==Complete&&Account.Routes==1&&!Earned(HomewardIndexV82));
            Check(FString::Printf(TEXT("v83_%s_nation%d_banner_only_when_all_required_feats_commit"),Catalog[Index].Id,Nation),TakeNotificationV81()==(Complete?Index:INDEX_NONE)&&TakeNotificationV81()==INDEX_NONE);
            if(Complete)Check(FString::Printf(TEXT("v83_%s_completion_date_is_last_required_nation"),Catalog[Index].Id),Account.Dates.FindRef(Catalog[Index].Id)==NationDate(Nation));
            else
            {
                CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),uint8(1u<<Feat),Bit,TEXT("2026-10-10T00:00:00Z"),true);PersistSettlementsV81();
                Check(FString::Printf(TEXT("v83_%s_repeat_same_nation_cannot_substitute_another_army"),Catalog[Index].Id),CompletedFeatNationMaskV83(Index)==Collected&&!Earned(Index)&&TakeNotificationV81()==INDEX_NONE);
            }
            Account=FAccount();Load();Check(FString::Printf(TEXT("v83_%s_nation%d_restart_retains_partial_or_complete_progress"),Catalog[Index].Id,Nation),CompletedFeatNationMaskV83(Index)==Collected&&Earned(Index)==Complete&&Account.Dates.FindRef(Catalog[0].Id)==OldDate);
        }
        const FString Completion=Account.Dates.FindRef(Catalog[Index].Id);
        CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),uint8(1u<<Feat),1,TEXT("2026-10-12T00:00:00Z"),true);PersistSettlementsV81();
        Check(FString::Printf(TEXT("v83_%s_duplicate_keeps_earliest_award_without_new_banner"),Catalog[Index].Id),Account.Dates.FindRef(Catalog[Index].Id)==Completion&&TakeNotificationV81()==INDEX_NONE&&EarnedCount()==2);
    }

    Seed();const FGuid BritishProof=FGuid::NewGuid();CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(BritishProof),1,1,NationDate(0),true);PersistSettlementsV81();
    for(int32 Nation=1;Nation<4;++Nation)CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),1,uint8(1u<<Nation),NationDate(Nation),true);
    ReviewFailSaveV81=true;PersistSettlementsV81();
    Check(TEXT("v83_failed_atomic_save_keeps_cross_progress_and_award_invisible"),CompletedFeatNationMaskV83(24)==1&&!Earned(24)&&TakeNotificationV81()==INDEX_NONE&&HasPendingV81());
    ReviewFailSaveV81=false;PersistSettlementsV81();FAccount CrossPrimary,CrossMirror;
    Check(TEXT("v83_retry_commits_all_cross_proofs_atomically_with_original_records"),ReadFile(Path,CrossPrimary)&&ReadFile(Backup,CrossMirror)&&CrossPrimary.CrossNationsV83[0]==15&&CrossMirror.CrossNationsV83[0]==15&&Earned(24)&&Account.Routes==1&&Account.Dates.FindRef(Catalog[0].Id)==OldDate&&TakeNotificationV81()==24&&TakeNotificationV81()==INDEX_NONE);

    Seed();for(int32 Nation=0;Nation<3;++Nation)CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),8,uint8(1u<<Nation),NationDate(Nation),true);PersistSettlementsV81();
    CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),8,8,NationDate(3),true);ReviewFailMirrorV81=true;PersistSettlementsV81();
    Check(TEXT("v83_primary_only_highest_bit_commit_defers_cross_banner"),ReadFile(Path,CrossPrimary)&&CrossPrimary.EarnedMask==(1u|(1u<<27))&&CrossPrimary.CrossNationsV83[3]==15&&CompletedFeatNationMaskV83(27)==7&&!Earned(27)&&TakeNotificationV81()==INDEX_NONE&&HasPendingV81());
    ReviewFailMirrorV81=false;PersistSettlementsV81();Account=FAccount();Load();
    Check(TEXT("v83_highest_cross_bit_restart_and_mirror_retry_banner_are_exact_once"),Earned(27)&&CompletedFeatNationMaskV83(27)==15&&Account.Dates.FindRef(Catalog[27].Id)==NationDate(3)&&TakeNotificationV81()==27&&TakeNotificationV81()==INDEX_NONE&&!HasPendingV81());

    Seed();const FGuid PartialRun=FGuid::NewGuid();CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(PartialRun),2,1,NationDate(0),true);
    ReviewFailMirrorV81=true;PersistSettlementsV81();ReviewFailMirrorV81=false;
    Check(TEXT("v83_partial_primary_commit_keeps_no_unearned_medal_banner_receipt"),SettlementsV81.FindChecked(PartialRun).CommittedNotifyMask==0&&!Earned(25)&&TakeNotificationV81()==INDEX_NONE);
    const auto LocalPending=SettlementsV81;SettlementsV81.Empty();Account=FAccount();Load();
    CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),2,2,NationDate(1),true);PersistSettlementsV81();
    Check(TEXT("v83_other_instance_completes_partial_mirror_commit_once"),Earned(25)&&TakeNotificationV81()==25&&TakeNotificationV81()==INDEX_NONE);
    SettlementsV81=LocalPending;PersistSettlementsV81();
    Check(TEXT("v83_partial_mirror_retry_cannot_repeat_other_instance_completion_banner"),Earned(25)&&CompletedFeatNationMaskV83(25)==3&&TakeNotificationV81()==INDEX_NONE&&!HasPendingV81());

    Seed();CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),4,1,NationDate(2),true);PersistSettlementsV81();const FAccount StalePartial=Account;
    CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),4,2,NationDate(1),true);PersistSettlementsV81();
    Account=StalePartial;CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),4,4,NationDate(2),true);
    CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),4,1,NationDate(0),true);PersistSettlementsV81();
    Check(TEXT("v83_concurrent_instance_or_merge_preserves_earliest_per_nation_dates"),CompletedFeatNationMaskV83(26)==7&&!Earned(26)&&Account.CrossDatesV83.FindRef(CrossDateKeyV83(2,0))==NationDate(0)&&Account.CrossDatesV83.FindRef(CrossDateKeyV83(2,1))==NationDate(1)&&TakeNotificationV81()==INDEX_NONE);
    const FAccount BeforeOtherCompletes=Account;CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),4,8,NationDate(3),true);PersistSettlementsV81();TakeNotificationV81();
    Account=BeforeOtherCompletes;CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),4,8,TEXT("2026-10-12T00:00:00Z"),true);PersistSettlementsV81();
    Check(TEXT("v83_other_instance_completed_cross_award_does_not_repeat_banner_or_date"),Earned(26)&&CompletedFeatNationMaskV83(26)==15&&Account.Dates.FindRef(Catalog[26].Id)==NationDate(3)&&TakeNotificationV81()==INDEX_NONE&&!HasPendingV81());

    Seed();CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(FGuid::NewGuid()),2,1,NationDate(0),true);PersistSettlementsV81();const FGuid TaintedRun=FGuid::NewGuid();
    CaptureCrossSettlementV83(SettlementsV81.FindOrAdd(TaintedRun),2,2,NationDate(1),true);SettlementsV81.FindChecked(TaintedRun).Routes=8;RetryTaintedV81.Add(TaintedRun);PersistSettlementsV81();
    Check(TEXT("v83_tainted_pending_campaign_cannot_add_cross_nation_feat_or_route"),CompletedFeatNationMaskV83(25)==1&&!Earned(25)&&Account.Tainted.Contains(TaintedRun)&&Account.Routes==1&&TakeNotificationV81()==INDEX_NONE&&!HasPendingV81());
    auto& WrongNation=SettlementsV81.FindOrAdd(FGuid::NewGuid());CaptureCrossSettlementV83(WrongNation,2,4,NationDate(2),true);PersistSettlementsV81();
    Check(TEXT("v83_france_cannot_substitute_for_britain_or_germany_in_both_sides"),CompletedFeatNationMaskV83(25)==1&&!Earned(25));

    RunAccountSchemaChecksV836(B,Check);
}
