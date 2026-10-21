bool ChallengeReviewAllowedV84415(AWarfareFrontlineGameMode* B)
{
    FString Dir;
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84415/Runtime"));
    FPaths::NormalizeDirectoryName(Root);
    const bool V851=B&&FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)
        &&Dir.StartsWith(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V851/Runtime/")),ESearchCase::IgnoreCase);
    const bool New=B&&FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)
        &&Dir.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase);
    return B&&B->GetWorld()&&B->IsProgressionVolatile()&&DisposableAccount()
        &&B->GetCurrentCampaign()&&!WarfareFRRUCampaignV82::Active(B)
        &&(New||V851||FrontlineGameplayFocusedV84412::Allowed(B));
}
void RunChallengeChecksV84415(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=ChallengeReviewAllowedV84415(B);
    Check(TEXT("v84415_challenges_disposable_ukde_only"),Safe);if(!Safe)return;
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Prior=Encounters;const auto PriorAccount=Account;
    const auto Records0=B->CombatRecordV65[0],Records1=B->CombatRecordV65[1];
    const auto Phase=B->Phase;const int32 Winner=B->BattleOutroWinner;
    const float Morale0=B->Morale[0],Morale1=B->Morale[1];
    const float Damage0=B->DamageDealt[0],Damage1=B->DamageDealt[1];
    const float Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
    const FString Message=B->LastMessage,Failure=B->AssetFailure;
    const auto Casualties0=B->Casualties[0],Casualties1=B->Casualties[1];
    FFrontlineAchievementReviewV76 Campaign(B);FTransientReviewV81 Transient;
    TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
    TArray<TWeakObjectPtr<AActor>> Actors;
    ON_SCOPE_EXIT
    {
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        B->Units=Units;B->Squads=Squads;Encounters=Prior;Account=PriorAccount;
        B->CombatRecordV65[0]=Records0;B->CombatRecordV65[1]=Records1;
        B->Phase=Phase;B->BattleOutroWinner=Winner;
        B->Morale[0]=Morale0;B->Morale[1]=Morale1;B->Casualties[0]=Casualties0;B->Casualties[1]=Casualties1;
        B->DamageDealt[0]=Damage0;B->DamageDealt[1]=Damage1;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
        B->LastMessage=Message;B->AssetFailure=Failure;
    };
    Campaign.Campaign(EFrontlineFaction::German);
    B->Units.Reset();B->Squads.Reset();B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    ResetEncounter(B);auto& S=State(B);S.Started=true;S.Campaign=B->GetCurrentCampaign()->Id;
    auto Section=[&](EFrontlineRole Role,int32 Id)->AWarfareFrontlineSoldier*
    {
        FFrontlineSquad Q;Q.Id=Id;Q.Team=0;Q.Role=Role;
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return nullptr;
        U->Initialize(B,Id,0,Role,0);U->SetActorTickEnabled(false);U->SetActorHiddenInGame(true);
        U->bFieldEntered=true;U->SetActorLocation(FVector(0,1400+B->Squads.Num()*170,88));
        Q.Members.Add(U);B->Squads.Add(Q);B->Units.Add(U);Actors.Add(U);
        Recruited(B,int32(Role),0);return U;
    };
    auto* Gun=Section(EFrontlineRole::Lewis,8441501);auto* AT=Section(EFrontlineRole::AntiTankRifle,8441502);
    Check(TEXT("v84415_challenges_ordinary_role_actors"),Gun&&AT);if(!Gun||!AT)return;
    const auto GunSource=FFrontlineKillSourceV65::Soldier(Gun);
    for(int32 K=0;K<11;++K)FrontlineCombatRecordV65::InfantryLost(B,1,GunSource);
    Check(TEXT("v84415_one_gun_eleven_does_not_qualify"),!ChallengeConditionV84415(B,3));
    FrontlineCombatRecordV65::InfantryLost(B,1,GunSource);
    Check(TEXT("v84415_one_gun_twelve_qualifies"),ChallengeConditionV84415(B,3));
    Gun->Health=0;
    Check(TEXT("v84415_one_gun_deed_is_not_lost_with_gunner"),ChallengeConditionV84415(B,3));
    Campaign.Run->Difficulty=EFrontlineDifficulty::Rookie;
    Check(TEXT("v84415_one_gun_rookie_is_excluded"),!ChallengeConditionV84415(B,3));
    Campaign.Run->Difficulty=EFrontlineDifficulty::Survivor;
    Check(TEXT("v84415_one_gun_survivor_is_eligible"),ChallengeConditionV84415(B,3));
    auto* Mobile=Section(EFrontlineRole::MobileGun,8441503);
    Check(TEXT("v851_one_gun_second_researched_alternative_keeps_personal_deed"),Mobile&&ChallengeConditionV84415(B,3));
    if(!Mobile)return;
    S.GunSectionKillsV851.Reset();B->FindSquad(8441503)->KillsV65.Infantry=12;
    Check(TEXT("v851_one_gun_fabricated_statistics_do_not_replace_death_receipts"),!ChallengeConditionV84415(B,3));
    const auto MobileSource=FFrontlineKillSourceV65::Soldier(Mobile);
    for(int32 K=0;K<6;++K){FrontlineCombatRecordV65::InfantryLost(B,1,GunSource);FrontlineCombatRecordV65::InfantryLost(B,1,MobileSource);}
    Check(TEXT("v851_one_gun_two_six_kill_sections_cannot_pool"),!ChallengeConditionV84415(B,3));
    for(int32 K=0;K<6;++K)FrontlineCombatRecordV65::InfantryLost(B,1,MobileSource);
    Check(TEXT("v851_one_gun_mobile_section_twelve_real_deaths_is_equivalent"),ChallengeConditionV84415(B,3));
    B->Squads.RemoveAll([](const FFrontlineSquad& Q){return Q.Id==8441501||Q.Id==8441503;});
    Check(TEXT("v851_one_gun_deed_survives_actual_section_removal"),ChallengeConditionV84415(B,3));
    S.GunSectionKillsV851.Reset();
    for(int32 K=0;K<11;++K)FrontlineCombatRecordV65::InfantryLost(B,1,MobileSource);
    Check(TEXT("v851_one_gun_last_round_after_section_cleanup_has_eleven_receipts"),!ChallengeConditionV84415(B,3)&&S.GunSectionKillsV851.FindRef(8441503)==11);
    FrontlineCombatRecordV65::InfantryLost(B,1,MobileSource);
    Check(TEXT("v851_one_gun_last_round_after_section_cleanup_completes_personal_deed"),ChallengeConditionV84415(B,3));
    S.GunSectionKillsV851.Reset();
    for(int32 K=0;K<12;++K){FrontlineCombatRecordV65::InfantryLost(B,0,MobileSource);FrontlineCombatRecordV65::InfantryLost(B,1,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Artillery)));}
    Check(TEXT("v851_one_gun_friendly_fire_and_support_cannot_claim_section_deed"),!ChallengeConditionV84415(B,3));
    // Fabricated totals cannot claim a victim's class. Track hits cannot either.
    B->FindSquad(8441502)->KillsV65.Armour=20;
    Check(TEXT("v84415_david_statistics_without_real_victim_do_not_qualify"),!ChallengeConditionV84415(B,6));
    const auto ATSource=FFrontlineKillSourceV65::Soldier(AT);
    auto Target=[&](EFrontlineVehicle Type,int32 Serial,int32 Team=1)->AFrontlineTank*
    {
        auto* T=B->GetWorld()->SpawnActor<AFrontlineTank>();if(!T)return nullptr;Actors.Add(T);
        if(!T->Configure(B,Type,Team,Serial))return nullptr;
        T->SetActorTickEnabled(false);T->SetActorHiddenInGame(true);return T;
    };
    auto* Light=Target(EFrontlineVehicle::Whippet,8441511);
    auto* Heavy=Target(EFrontlineVehicle::MarkIVMale,8441512);
    Check(TEXT("v84415_david_actual_light_and_heavy_models_loaded"),Light&&Heavy);if(!Light||!Heavy)return;
    Light->ReceiveModuleHit(EFrontlineTankModule::Hull,100000,0,ATSource);
    Check(TEXT("v84415_david_light_armour_death_does_not_qualify"),!ChallengeConditionV84415(B,6));
    Heavy->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,100000,0,ATSource);
    Check(TEXT("v84415_david_heavy_track_loss_does_not_qualify"),Heavy->IsOperational()&&!ChallengeConditionV84415(B,6));
    Heavy->ReceiveModuleHit(EFrontlineTankModule::Hull,100000,0,ATSource);
    const int32 Receipts=S.HeavyRifleVictimsV84415.Num();
    Check(TEXT("v84415_david_real_heavy_hull_fatal_route_qualifies"),!Heavy->IsOperational()&&ChallengeConditionV84415(B,6)&&Receipts==1);
    FrontlineCombatRecordV65::ArmourDestroyedV8220(B,Heavy,ATSource);
    Check(TEXT("v84415_david_duplicate_death_receipt_is_idempotent"),S.HeavyRifleVictimsV84415.Num()==Receipts);
    Campaign.Run->Difficulty=EFrontlineDifficulty::Rookie;
    Check(TEXT("v84415_david_rookie_is_excluded"),!ChallengeConditionV84415(B,6));
    Campaign.Run->Difficulty=EFrontlineDifficulty::Veteran;
    const auto BeforeVictims=S.HeavyRifleVictimsV84415;S.HeavyRifleVictimsV84415.Reset();
    auto* ArtilleryHeavy=Target(EFrontlineVehicle::MarkVMale,8441513);
    if(ArtilleryHeavy)ArtilleryHeavy->ReceiveModuleHit(EFrontlineTankModule::Hull,100000,0,FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::AntiTank)));
    Check(TEXT("v84415_david_support_finish_cannot_claim_rifle_deed"),ArtilleryHeavy&&!ChallengeConditionV84415(B,6));
    S.HeavyRifleVictimsV84415=BeforeVictims;
    const auto BeforeDates=Account.Dates;const auto BeforeMask=Account.EarnedMask;
    S.Eligible=true;S.SettlementCaptured=false;Account.Loaded=true;Account.Tainted.Add(S.Campaign);
    B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;Finish(B,0);
    Check(TEXT("v84415_challenges_assisted_archive_cannot_settle"),!State(B).SettlementCaptured&&Account.EarnedMask==BeforeMask);
    bool SameDates=Account.Dates.Num()==BeforeDates.Num();
    for(const auto& Date:BeforeDates){const auto* Current=Account.Dates.Find(Date.Key);SameDates&=Current&&*Current==Date.Value;}
    Check(TEXT("v84415_challenge_revision_retains_existing_dates_and_mask"),Account.EarnedMask==BeforeMask&&SameDates);
    // This is a criterion/death-route boundary review. The controlled exchanges
    // below use unchanged projectiles and live retaliation for combat feasibility.
}
