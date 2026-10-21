// Included by CombatRecordV65.cpp to restore its private armour serial receipts.
struct FFrontlineReportReviewV839
{static void TickCommitted(AWarfareFrontlineGameMode* B){B->TickCommittedSupportV8217();}};
namespace
{
bool ReportReviewAllowedV839(AWarfareFrontlineGameMode* B)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV839Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FrontlineUltimateArchiveV8215::ReviewUserDir(UserDir))return false;
    // ProjectSavedDir is redirected below -UserDir; anchor this guard in the
    // source-tree review root, then retain the exact V83.9 timestamp boundary.
    const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V839/Runtime/"));
    const FString Stamp=UserDir.StartsWith(Root,ESearchCase::IgnoreCase)?UserDir.Mid(Root.Len()):FString();
    return Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
}
bool MakeReportReceiverFixtureV839(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check,bool Keep)
{
    if(!ReportReviewAllowedV839(B)){Check(TEXT("v839_report_disposable_receiver_guard"),false);return false;}
    const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldStrikes=B->Strikes;const auto OldPhase=B->Phase;
    const auto OldSmokeZones=B->SmokeZones;const bool OldPaused=UGameplayStatics::IsGamePaused(B);
    const float OldArtilleryReady[2]={B->ArtilleryReadyAt[0],B->ArtilleryReadyAt[1]};const int32 OldArtilleryCommands=B->ArtilleryCommands;
    const int32 OldWinner=B->BattleOutroWinner,OldCasualties[2]={B->Casualties[0],B->Casualties[1]};
    const float OldOutro=B->BattleOutroElapsed,OldElapsed=B->Elapsed,OldFriendly=B->FriendlyFireDamage,OldPeak=B->PeakSuppression;
    const float OldDamage[2]={B->DamageDealt[0],B->DamageDealt[1]},OldMorale[2]={B->Morale[0],B->Morale[1]};
    const FFrontlineCombatRecordV65 OldRecords[2]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const FString OldMessage=B->LastMessage,OldReason=B->EndReason;
    const bool HadDeaths=ArmourDeathsV8220.Contains(B);const auto OldDeaths=ArmourDeathsV8220.FindRef(B);
    TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
    bool Good=true,Completed=false;
    auto C=[&](const FString& Key,bool Passed){Good&=Passed;Check(Key,Passed);};
    ON_SCOPE_EXIT
    {
        if(HadDeaths)ArmourDeathsV8220.Add(B,OldDeaths);else ArmourDeathsV8220.Remove(B);
        if(!Keep||!Completed||!Good)
        {
            TArray<AActor*> NewActors;for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))NewActors.Add(*It);
            for(auto* Actor:NewActors)if(IsValid(Actor))Actor->Destroy();
            B->Units=OldUnits;B->Squads=OldSquads;B->Strikes=OldStrikes;B->Phase=OldPhase;B->BattleOutroWinner=OldWinner;B->BattleOutroElapsed=OldOutro;
            B->SmokeZones=OldSmokeZones;B->ArtilleryCommands=OldArtilleryCommands;for(int32 T=0;T<2;++T)B->ArtilleryReadyAt[T]=OldArtilleryReady[T];UGameplayStatics::SetGamePaused(B,OldPaused);
            B->Elapsed=OldElapsed;B->FriendlyFireDamage=OldFriendly;B->PeakSuppression=OldPeak;B->LastMessage=OldMessage;B->EndReason=OldReason;
            for(int32 T=0;T<2;++T){B->CombatRecordV65[T]=OldRecords[T];B->Casualties[T]=OldCasualties[T];B->DamageDealt[T]=OldDamage[T];B->Morale[T]=OldMorale[T];}
        }
    };
    FrontlineAchievementsV76::WithScopedCombatEventsV839(B,[&]
    {FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]
    {FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]
    {
        B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=240;
        B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};B->Casualties[0]=B->Casualties[1]=0;B->DamageDealt[0]=B->DamageDealt[1]=0;B->Morale[0]=B->Morale[1]=100;B->FriendlyFireDamage=B->PeakSuppression=0;
        FrontlineReportV839::Begin(B);int32 NextId=8390000;
        bool FreshCoverage=true;for(const auto& R:B->CombatRecordV65)for(const auto& S:R.DetailsV839.Support)FreshCoverage&=S.bDeliveryRecorded&&S.bRoundsRecorded&&S.bDamageRecorded&&S.Delivered==0&&S.Rounds==0&&S.EnemyHullDamage==0&&S.FriendlyHullDamage==0;
        C(TEXT("v839_report_fresh_pipeline_knows_zero_activity"),FreshCoverage);
        auto Man=[&](int32 Team,EFrontlineRole Role)
        {
            const int32 Id=NextId++;FVector Position((Team?1.f:-1.f)*450.f,(Id%8-4)*150.f,0);Position.Z=AWarfareFrontlineGround::HeightAt(Position.X,Position.Y)+88;
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Position,FRotator(0,Team?180.f:0.f,0.f));if(!U)return U;
            U->Initialize(B,Id,Team,Role,0);U->bFieldEntered=true;U->SetActorTickEnabled(false);B->Units.Add(U);
            FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=Role;Q.Members.Add(U);B->Squads.Add(Q);FrontlineReportV839::RegisterSquad(B,Team,Id,int32(Role),1);return U;
        };
        auto Hit=[&](AWarfareFrontlineSoldier* U,const FFrontlineKillSourceV65& Source,EFrontlineDamageKind Kind=EFrontlineDamageKind::Bullet)
        {if(U)U->ReceiveHit(1,0,Source.Team,FVector::ForwardVector,Kind,.1f,Source);};
        auto* Shooter=Man(0,EFrontlineRole::Rifle);auto* Enemy=Man(1,EFrontlineRole::Rifle);auto* Delayed=Man(1,EFrontlineRole::Rifle);auto* Friendly=Man(0,EFrontlineRole::Rifle);
        if(!Shooter||!Enemy||!Delayed||!Friendly){C(TEXT("v839_report_real_infantry_spawn"),false);return;}
        const auto Rifle=FFrontlineKillSourceV65::Soldier(Shooter);const int32 ShooterId=Shooter->SquadId;
        Hit(Enemy,Rifle);Hit(Enemy,Rifle);
        C(TEXT("v839_report_real_fatal_receiver_and_duplicate_once"),!Enemy->IsAlive()&&B->CombatRecordV65[0].Total.Infantry==1&&B->CombatRecordV65[0].DetailsV839.Squads.FindChecked(ShooterId).Kills.Infantry==1);
        Shooter->Role=EFrontlineRole::Marksman;Hit(Shooter,FFrontlineKillSourceV65::FireSupport(1,int32(EFrontlineSupport::Artillery)));Shooter->Destroy();Hit(Delayed,Rifle,EFrontlineDamageKind::Grenade);Hit(Friendly,Rifle,EFrontlineDamageKind::Artillery);
        const auto& Section=B->CombatRecordV65[0].DetailsV839.Squads.FindChecked(ShooterId);
        C(TEXT("v839_report_dead_source_preserves_original_section_and_discharge_role"),!Delayed->IsAlive()&&Section.OriginalRole==int32(EFrontlineRole::Rifle)&&Section.Kills.Infantry==2&&B->CombatRecordV65[0].InfantrySources[int32(EFrontlineRole::Rifle)].Infantry==2&&B->CombatRecordV65[0].InfantrySources[int32(EFrontlineRole::Marksman)].Infantry==0);
        C(TEXT("v839_report_friendly_casualty_is_separate_from_enemy_kills"),!Friendly->IsAlive()&&B->CombatRecordV65[0].Total.Infantry==2&&B->CombatRecordV65[0].DetailsV839.FriendlyFire.Infantry==1&&Section.FriendlyFire.Infantry==1);
        const auto Battery=FFrontlineKillSourceV65::FireSupport(0,int32(EFrontlineSupport::Artillery));auto* SupportVictim=Man(1,EFrontlineRole::Lewis);Hit(SupportVictim,Battery,EFrontlineDamageKind::Artillery);
        auto* FriendlySupport=Man(0,EFrontlineRole::Lewis);Hit(FriendlySupport,Battery,EFrontlineDamageKind::Artillery);
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();const bool Configured=Tank&&Tank->Configure(B,EFrontlineVehicle::A7V,1,8390001);
        C(TEXT("v839_report_real_support_victim_and_armour_spawn"),SupportVictim&&!SupportVictim->IsAlive()&&Configured);
        if(!Configured)return;
        Tank->SetActorTickEnabled(false);Tank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,8.f,0,Battery);
        C(TEXT("v839_report_track_damage_is_not_armour_kill"),B->CombatRecordV65[0].Total.Armour==0);
        Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,Battery);Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,10000,0,Battery);
        C(TEXT("v839_report_real_support_hull_death_and_model_once"),B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::Artillery)].Armour==1&&B->CombatRecordV65[0].DetailsV839.ArmourDestroyedByModel[int32(EFrontlineVehicle::A7V)]==1&&B->CombatRecordV65[1].DetailsV839.ArmourLostByModel[int32(EFrontlineVehicle::A7V)]==1);
        C(TEXT("v839_report_actual_applied_integrity_damage"),B->CombatRecordV65[0].DetailsV839.Support[int32(EFrontlineSupport::Artillery)].EnemyHullDamage>0&&B->CombatRecordV65[0].DetailsV839.Support[int32(EFrontlineSupport::Artillery)].EnemyTrackDamage>0);
        B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;B->BattleOutroElapsed=2;const float Morale=B->Morale[1];const auto BeforeOutro=B->CombatRecordV65[0];
        auto* Outro=Man(1,EFrontlineRole::Mills);Hit(Outro,Battery,EFrontlineDamageKind::Artillery);Hit(Outro,Battery,EFrontlineDamageKind::Artillery);
        auto* ClosedOrdinary=Man(1,EFrontlineRole::Mills);Hit(ClosedOrdinary,Rifle);
        C(TEXT("v839_report_outro_committed_support_once_ordinary_fire_closed"),Outro&&!Outro->IsAlive()&&ClosedOrdinary&&ClosedOrdinary->IsAlive()&&B->CombatRecordV65[0].Total.Infantry==BeforeOutro.Total.Infantry+1&&B->Morale[1]==Morale);
        C(TEXT("v839_report_support_friendly_and_enemy_tallies_do_not_overlap"),B->CombatRecordV65[0].SupportSources[0].Infantry==2&&B->CombatRecordV65[0].DetailsV839.FriendlySupport[0].Infantry==1&&B->CombatRecordV65[0].Total.Infantry==4);
        FJsonObject J;FrontlineCombatRecordV65::WriteArchive(J,B->CombatRecordV65[0]);FFrontlineCombatRecordV65 Read;
        C(TEXT("v839_report_full_optional_archive_roundtrip"),FrontlineCombatRecordV65::ReadArchive(J,Read,false)&&Read.DetailsV839.bRecorded&&Read.DetailsV839.FriendlyFire.Infantry==2&&Read.DetailsV839.ArmourDestroyedByModel[int32(EFrontlineVehicle::A7V)]==1);
        J.GetObjectField(TEXT("combat_record_v65"))->RemoveField(TEXT("detailed_report_v839"));
        C(TEXT("v839_report_old_record_retains_known_source_totals"),FrontlineCombatRecordV65::ReadArchive(J,Read,true)&&!Read.DetailsV839.bRecorded&&Read.DetailsV839.bIncludesEarlierUnrecordedBattles&&Read.Total.Infantry==4&&Read.Total.Armour==1);
        FrontlineReportV839::RunCodecChecks(C);
        if(!Keep)
        {
            B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=FMath::Max(240.f,B->ArtilleryReadyAt[0]+.01f);UGameplayStatics::SetGamePaused(B,false);
            const bool Issued=B->RequestArtillery(FVector::ZeroVector,0);const auto& Telemetry=B->CombatRecordV65[0].DetailsV839.Support[int32(EFrontlineSupport::Artillery)];
            C(TEXT("v839_report_public_artillery_acceptance_is_one_order_zero_rounds"),Issued&&Telemetry.Delivered==1&&Telemetry.Rounds==0);
            if(Issued)
            {
                float FirstLaunch=MAX_flt;for(const auto& Strike:B->Strikes)if(Strike.SourceV65.Team==0&&Strike.SourceV65.Support==int32(EFrontlineSupport::Artillery))FirstLaunch=FMath::Min(FirstLaunch,Strike.LaunchedAt);
                C(TEXT("v839_report_public_artillery_has_actual_launch_schedule"),FMath::IsFinite(FirstLaunch)&&FirstLaunch<MAX_flt);
                if(FirstLaunch<MAX_flt)
                {
                    B->Elapsed=FirstLaunch-.01f;FFrontlineReportReviewV839::TickCommitted(B);C(TEXT("v839_report_before_launch_has_no_discharge"),Telemetry.Rounds==0);
                    B->Elapsed=FirstLaunch;FFrontlineReportReviewV839::TickCommitted(B);C(TEXT("v839_report_first_actual_launch_is_one_round"),Telemetry.Rounds==1);
                    FFrontlineReportReviewV839::TickCommitted(B);C(TEXT("v839_report_repeated_launch_tick_does_not_double_count"),Telemetry.Delivered==1&&Telemetry.Rounds==1);
                }
            }
        }
        B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;
        B->BattleOutroElapsed=7.01f;B->EndReason=OldReason;B->LastMessage=TEXT("V83.9 disposable receiver fixture");Completed=true;
    });});});
    return Good&&Completed;
}
}
TSharedRef<FJsonObject> RunFrontlineReportV839Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    auto C=[&](const FString& Name,bool Passed){auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(Row));Check(Name,Passed);};
    const bool Good=MakeReportReceiverFixtureV839(B,C,false);J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Rows);
    J->SetStringField(TEXT("scope"),TEXT("Strict NoSave receiver fixture: actual spawned infantry fatal hits, captured dead-source credit, friendly fire, support hull/track damage, actual armour death, committed outro, optional codec, rich tail, public artillery acceptance and first-launch receipt. Restores battle data, readiness, pause state, private serial receipts and achievement/morale/field event maps. No full campaign, complete projectile flight or balance proof."));return J;
}
bool PrepareFrontlineReportVisualV839(AWarfareFrontlineGameMode* B)
{return MakeReportReceiverFixtureV839(B,[](const FString&,bool){},true);}
