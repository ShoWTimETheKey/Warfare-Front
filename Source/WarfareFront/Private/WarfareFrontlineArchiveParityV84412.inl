void FFrontlineUltimateArchiveV8215Access::ParityChecksV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineGameplayFocusedV84412::Allowed(B)&&!FrontlineUltimateArchiveV8215::Active(B);
    Check(TEXT("v84412_archive_parity_strict_disposable_nonarchive_profile"),Safe);if(!Safe)return;
    const auto Runs=B->CampaignRuns;const FGuid Active=B->ActiveCampaignId;const auto Faction=B->PlayerFaction;
    const auto Phase=B->Phase;const bool ReadOnly=B->bProgressionReadOnly,Volatile=B->bProgressionVolatile;
    const FString Message=B->LastMessage,SaveStatus=B->ProgressionSaveStatus;
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    Check(TEXT("v84412_archive_parity_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return;
    ON_SCOPE_EXIT
    {
        Sessions.Remove(B);B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;B->Phase=Phase;
        B->bProgressionReadOnly=ReadOnly;B->bProgressionVolatile=Volatile;B->LastMessage=Message;B->ProgressionSaveStatus=SaveStatus;
    };
    const uint8 AccountRoutes=FrontlineAchievementsV76::CompletedCampaignRouteMaskV82();
    for(const auto Player:{EFrontlineFaction::British,EFrontlineFaction::German})
    {
        FSession Session;Session.Replay=FGuid::NewGuid();Session.Qualified=true;
        FFrontlineCampaignProgress Run;Run.Id=Session.Replay;Run.Faction=Player;Run.SelectedMission=0;
        Run.CompletedMissions=B->GetCampaignMissionsForFaction(Player).Num();Run.bMobileGunV52=true;
        Run.Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1u;Run.ArmourResearchMaskV63=65535;
        B->CampaignRuns={Run};B->ActiveCampaignId=Run.Id;B->PlayerFaction=Player;B->Phase=EFrontlinePhase::Briefing;
        B->bProgressionReadOnly=false;B->bProgressionVolatile=true;Sessions.Add(B,Session);
        const FString Prefix=FString::Printf(TEXT("v84412_archive_%s_"),Player==EFrontlineFaction::German?TEXT("de"):TEXT("uk"));
        Check(Prefix+TEXT("full_armory_requires_exact_replay_both_sides"),FrontlineUltimateArchiveV8215::FullArmory(B,0)&&FrontlineUltimateArchiveV8215::FullArmory(B,1)
            &&!FrontlineUltimateArchiveV8215::FullArmory(B,-1)&&!FrontlineUltimateArchiveV8215::FullArmory(B,2));
        bool Roles=true,Tech=true,ArmourTech=true,Support=true,National=true,Variants=true;
        TSet<EFrontlineRole> ExpectedRoles,ObservedRoles;
        for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
        {
            const auto Role=EFrontlineRole(I);const bool Allowed=FrontlineCampaignV41::RoleAllowed(B->GetFactionForTeam(1),Role);
            Roles&=B->IsRoleUnlocked(Role,1)==Allowed&&FrontlineGameplayV33::EnemyRoleAvailable(B,Role)==Allowed;
            if(Allowed)ExpectedRoles.Add(Role);
        }
        for(int32 I=0;I<ExpectedRoles.Num();++I)ObservedRoles.Add(FrontlineArchiveEnemyRoleV84412(B,2+3*I));
        Check(Prefix+TEXT("enemy_real_role_gate_ignores_early_mission_locks"),Roles);
        Check(Prefix+TEXT("enemy_reserve_selector_visits_full_legal_national_roster"),ExpectedRoles.Difference(ObservedRoles).IsEmpty()
            &&ObservedRoles.Difference(ExpectedRoles).IsEmpty()&&FrontlineArchiveEnemyRoleV84412(B,0)==EFrontlineRole::Count);
        for(int32 I=0;I<int32(EFrontlineTechnology::Count);++I)Tech&=B->HasTechnology(I,0)&&B->HasTechnology(I,1);
        for(int32 I=0;I<16;++I)ArmourTech&=FrontlineArmourV63::Has(B,I,0)&&FrontlineArmourV63::Has(B,I,1);
        for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)
            Support&=B->IsSupportUnlocked(EFrontlineSupport(I),1)==(EFrontlineSupport(I)!=EFrontlineSupport::Rally)
                &&FrontlineCampaignV32::SupportBlockReason(B,EFrontlineSupport(I),1).IsEmpty();
        Check(Prefix+TEXT("enemy_all_actual_faction_infantry_and_armour_technologies"),Tech&&ArmourTech);
        Check(Prefix+TEXT("enemy_all_nine_fire_support_gates_available"),Support);
        const auto Enemy=B->GetFactionForTeam(1);
        Variants=Enemy==EFrontlineFaction::German?B->HasStormtroopers(1)&&!B->HasScopedMarksmen(1):B->HasScopedMarksmen(1)&&!B->HasStormtroopers(1);
        Check(Prefix+TEXT("enemy_sniper_and_mp18_variant_follow_actual_faction"),Variants);
        const auto Roster=Fleet->GetEquippedRoster(1);int32 ExpectedVehicles=0;
        for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
        {
            const auto Type=EFrontlineVehicle(I);const bool Allowed=Fleet->IsUnlocked(Type,1);
            const bool Present=Roster.ContainsByPredicate([&](const auto& D){return D.Type==Type;});
            if(Allowed)++ExpectedVehicles;National&=Allowed==Present;
        }
        Check(Prefix+TEXT("enemy_full_legal_armour_catalogue_enters_actual_ai_roster"),National&&Roster.Num()==ExpectedVehicles&&Roster.Num()>2);
        Check(Prefix+TEXT("player_handicap_does_not_disable_enemy_technology"),FrontlineUltimateArchiveV8215::SetTechnologyEnabled(B,17,false)
            &&!B->HasTechnology(17,0)&&B->HasTechnology(17,1)&&FrontlineUltimateArchiveV8215::SetArmourTechnologyEnabled(B,5,false)
            &&!FrontlineArmourV63::Has(B,5,0)&&FrontlineArmourV63::Has(B,5,1));
        const auto UI=FrontlineUIAssetsV8213::Summary();const double Sync=UI->GetNumberField(TEXT("synchronous_requests")),Hits=UI->GetNumberField(TEXT("cache_hits"));
        bool Pictures=FrontlineUIAssetsV8213::PreloadReady();
        const double Started=FPlatformTime::Seconds();
        for(int32 Pass=0;Pass<2;++Pass)for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Pictures&=Fleet->GetPortrait(EFrontlineVehicle(I))!=nullptr;
        const double LookupMs=(FPlatformTime::Seconds()-Started)*1000.;
        Check(Prefix+TEXT("first_and_repeat_all_armour_cards_reuse_resident_cache_without_sync_io"),Pictures
            &&FrontlineUIAssetsV8213::Summary()->GetNumberField(TEXT("synchronous_requests"))==Sync
            &&FrontlineUIAssetsV8213::Summary()->GetNumberField(TEXT("cache_hits"))>=Hits+2*int32(EFrontlineVehicle::Count));
        Check(Prefix+FString::Printf(TEXT("all_13_portrait_lookup_cpu_ms_%.3f"),LookupMs),FMath::IsFinite(LookupMs));
        B->ActiveCampaignId=FGuid::NewGuid();
        Check(Prefix+TEXT("stale_session_cannot_unlock_a_regular_campaign"),!FrontlineUltimateArchiveV8215::FullArmory(B,0)&&!FrontlineUltimateArchiveV8215::FullArmory(B,1));
        B->ActiveCampaignId=Run.Id;Sessions.Remove(B);
        Check(Prefix+TEXT("leaving_archive_restores_normal_enemy_upgrade_rules"),!FrontlineUltimateArchiveV8215::FullArmory(B,1)&&!B->HasTechnology(17,1));
    }
    Check(TEXT("v84412_archive_parity_never_awards_or_opens_locked_national_campaigns"),FrontlineAchievementsV76::CompletedCampaignRouteMaskV82()==AccountRoutes
        &&!FrontlineUltimateArchiveV8215::Available(EFrontlineFaction(2))&&!FrontlineUltimateArchiveV8215::Available(EFrontlineFaction(3)));
}
