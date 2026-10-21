// Included inside the existing review friend. This is a static training-target
// attack-chain check, not a natural victory or tank-retaliation balance claim.
static TSharedRef<FJsonObject> RunATProjectilesV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Result=MakeShared<FJsonObject>();bool Good=true;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    const bool Safe=FrontlineGameplayFocusedV84412::Allowed(B)&&ReadbackAllowedV833(B);
    Assert(TEXT("v84412_at_projectiles_strict_disposable_ukde"),Safe);
    if(!Safe){Result->SetBoolField(TEXT("success"),false);return Result;}
    const double Began=FPlatformTime::Seconds();
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Runs=B->CampaignRuns;
    const auto Faction=B->PlayerFaction;const auto Active=B->ActiveCampaignId;
    const auto Encounter=B->EncounterId,Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
    const int32 BoundMission=B->BoundCampaignMission;const auto Phase=B->Phase;const int32 Outro=B->BattleOutroWinner;
    const float Clock=B->Elapsed,Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
    const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;
    const int32 Shots=B->ShotsFired,Grenades=B->GrenadesThrown;const FString Message=B->LastMessage,Failure=B->AssetFailure;
    const auto Records0=B->CombatRecordV65[0],Records1=B->CombatRecordV65[1];
    const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]};
    const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
    const bool HadMission=Missions.Contains(B);const FMission SavedMission=Missions.FindRef(B);
    TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
        if(Cast<AWarfareFrontlineSoldier>(*It)||Cast<AFrontlineTank>(*It))
        {Hidden.Add({*It,It->IsHidden(),It->GetActorEnableCollision()});It->SetActorHiddenInGame(true);It->SetActorEnableCollision(false);}
    ON_SCOPE_EXIT
    {
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        B->Units=Units;B->Squads=Squads;B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;
        B->EncounterId=Encounter;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=BoundMission;
        B->Phase=Phase;B->BattleOutroWinner=Outro;B->Elapsed=Clock;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
        B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;B->ShotsFired=Shots;B->GrenadesThrown=Grenades;
        B->LastMessage=Message;B->AssetFailure=Failure;B->CombatRecordV65[0]=Records0;B->CombatRecordV65[1]=Records1;
        for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];}
        if(HadMission)Missions.Add(B,SavedMission);else Missions.Remove(B);
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        Result->SetBoolField(TEXT("success"),Good);
        Result->SetNumberField(TEXT("total_cpu_seconds"),FPlatformTime::Seconds()-Began);
    };
    B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();
    B->PlayerFaction=EFrontlineFaction::German;B->EncounterId=FGuid::NewGuid();
    FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=EFrontlineFaction::German;
    Run.SelectedMission=Run.CompletedMissions=17;Run.Difficulty=EFrontlineDifficulty::Veteran;
    B->CampaignRuns={Run};B->ActiveCampaignId=B->BoundCampaignId=Run.Id;B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=17;
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;B->ShotsFired=0;
    B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};B->FriendlyFireDamage=0;B->PeakSuppression=0;
    for(int32 T=0;T<2;++T){B->Casualties[T]=0;B->Morale[T]=100;B->DamageDealt[T]=0;}
    Mission(B).Rounds.Reset();Mission(B).Readback={};
    Result->SetStringField(TEXT("scope"),TEXT("One real German Tankgewehr crew against two sequential full-health stationary UK training targets. Actual deployment, DischargeWeapon, recoil/reload, physical trace, unchanged scatter/penetration and deferred Tick damage. No direct module damage, forced hit roll, fabricated kill count, target retaliation, natural-victory or whole-campaign claim."));
    const auto Ground=[&](const FVector& P){return FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P));};
    const int32 Dir=B->GetTeamDirection(0),CrewId=8441241;
    FFrontlineSquad Crew;Crew.Id=CrewId;Crew.Team=0;Crew.Role=EFrontlineRole::AntiTankRifle;Crew.Trench=Crew.Destination=INDEX_NONE;
    B->Squads.Add(Crew);TArray<AWarfareFrontlineSoldier*> Men;
    for(int32 M=0;M<2;++M)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
        U->Initialize(B,CrewId,0,EFrontlineRole::AntiTankRifle,M);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;
        U->SetActorLocation(Ground(FVector(-Dir*(2500.f+M*90.f),1100.f+M*100.f,0))+FVector(0,0,88));
        U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));U->MotionRandom.Initialize(8441241+M*97);
        U->ReturnToIdle();U->AimSettleClock=U->DepartureClock=0;U->SetMotion(U->GetAimMotion());
        B->Units.Add(U);B->FindSquad(CrewId)->Members.Add(U);Men.Add(U);
    }
    auto* Gunner=Men.Num()==2?Men[0]:nullptr;auto* Gun=Gunner?FrontlineMachinegunsV30::Get(Gunner):nullptr;
    Assert(TEXT("v84412_at_projectiles_actual_two_man_tankgewehr_crew"),Gunner&&Gun&&Gun->HasAssets());if(!Gunner||!Gun||!Gun->HasAssets())return Result;
    const auto Pose=[&](float DT)
    {Gunner->Soldier->HandleExistingParallelEvaluationTask(true,true);Gunner->Soldier->TickAnimation(DT,false);Gunner->Soldier->RefreshBoneTransforms();};
    for(int32 I=0;I<100&&!Gun->CanFire();++I){B->Elapsed+=.05f;Gun->Update(.05f,false);Pose(.05f);}
    Gunner->SetMotion(Gunner->GetAimMotion());Pose(.35f);
    Assert(TEXT("v84412_at_projectiles_real_supported_deployment_completed"),Gun->CanFire());if(!Gun->CanFire())return Result;
    const double SimulationBegan=FPlatformTime::Seconds();const float SimulationStart=B->Elapsed;
    constexpr int32 MaximumShots=192;TArray<TSharedPtr<FJsonValue>> Cases;TArray<TSharedPtr<FJsonValue>> ShotRows;
    bool BudgetOkay=true,ScheduleOkay=true,AllSources=true,Deferred=true;float LastShot=-1,PreviousReload=0;
    const auto Advance=[&](float DT)
    {
        B->Elapsed+=DT;Gunner->ShotClock=FMath::Max(0.f,Gunner->ShotClock-DT);Gunner->AimSettleClock=FMath::Max(0.f,Gunner->AimSettleClock-DT);
        Gunner->UpdateWeaponCycle(DT);Gun->Update(DT,false);FrontlineSpecialistsV41::Tick(B);
    };
    const EFrontlineVehicle Models[]={EFrontlineVehicle::Whippet,EFrontlineVehicle::MarkVMale};
    for(int32 Model=0;Model<UE_ARRAY_COUNT(Models);++Model)
    {
        auto Case=MakeShared<FJsonObject>();Case->SetStringField(TEXT("model"),Model?TEXT("Mark V Male"):TEXT("Whippet"));
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>(Ground(FVector(0,1100,0)),FRotator::ZeroRotator);
        const bool Configured=Tank&&Tank->Configure(B,Models[Model],1,84412410+Model);
        Assert(FString::Printf(TEXT("v84412_at_full_health_target_%d_configured"),Model),Configured);
        if(!Configured){Cases.Add(MakeShared<FJsonValueObject>(Case));break;}
        Tank->SetActorTickEnabled(false);
        // Enter the real field once with no firing targets during placement.
        const auto CrewUnits=B->Units;B->Units.Reset();Tank->Simulate(.001f);B->Units=CrewUnits;
        const float InitialHull=Tank->GetHullIntegrity(),Expected=UFrontlineVehicleSubsystem::Definition(Models[Model]).Hull;
        Case->SetNumberField(TEXT("initial_hull"),InitialHull);Case->SetNumberField(TEXT("base_hull"),Expected);
        Case->SetBoolField(TEXT("full_health"),FMath::IsNearlyEqual(InitialHull,Tank->GetMaximumHullIntegrity(),.001f));
        Assert(FString::Printf(TEXT("v84412_at_target_%d_has_current_full_health"),Model),InitialHull>=Expected-.001f&&FMath::IsNearlyEqual(InitialHull,Tank->GetMaximumHullIntegrity(),.001f));
        const auto Before=Mission(B).Readback;const float Started=B->Elapsed;const int32 PriorKills=B->FindSquad(CrewId)->KillsV65.Armour;
        const FVector Original=Tank->GetActorLocation();const int32 PriorShots=B->ShotsFired;
        Tank->SetActorLocation(Ground(Gunner->Feet()+FVector(Dir*(FrontlineWeapons::ProfileFor(Gunner).FireRangeCm*B->EncounterVisibilityScale()+900.f),0,0)));
        FRandomStream RangeProbe(8441299+Model);const bool OutOfRange=FrontlineSpecialistsV41::Fire(Gunner,nullptr,Tank,RangeProbe);
        Assert(FString::Printf(TEXT("v84412_at_target_%d_out_of_range_spends_no_round"),Model),!OutOfRange&&B->ShotsFired==PriorShots&&Mission(B).Rounds.IsEmpty());Tank->SetActorLocation(Original);
        int32 NoShot=0;
        while(Tank->IsOperational()&&Gunner->IsAlive()&&B->ShotsFired<MaximumShots)
        {
            if(FPlatformTime::Seconds()-SimulationBegan>2.6){BudgetOkay=false;break;}
            for(int32 PhaseStep=0;PhaseStep<8&&Gunner->WeaponPhase!=EFrontlineWeaponPhase::Ready;++PhaseStep)
                Advance(FMath::Max(.02f,Gunner->FirePoseClock+.001f));
            if(Gunner->WeaponPhase!=EFrontlineWeaponPhase::Ready)break;
            Advance(FMath::Max(Gunner->ShotClock,Gunner->AimSettleClock)+.001f);
            Gunner->SetMotion(Gunner->GetAimMotion());Pose(.1f);
            const float HullBefore=Tank->GetHullIntegrity();const int32 FiredBefore=B->ShotsFired;
            const auto BeforeShot=Mission(B).Readback;Gunner->DischargeWeapon(nullptr,Tank);
            if(B->ShotsFired==FiredBefore){if(++NoShot>=4)break;Advance(.1f);continue;}NoShot=0;
            const auto Round=Mission(B).Rounds.IsEmpty()?FAPRound{}:Mission(B).Rounds.Last();
            Deferred&=Round.Arrival>B->Elapsed&&Tank->GetHullIntegrity()==HullBefore;
            AllSources&=Round.SourceV65.Team==0&&Round.SourceV65.SquadId==CrewId&&Round.SourceV65.Role==int32(EFrontlineRole::AntiTankRifle);
            if(LastShot>=0)ScheduleOkay&=B->Elapsed-LastShot+ .002f>=PreviousReload;
            LastShot=B->Elapsed;PreviousReload=FrontlineSpecialistsV41::ReloadSeconds(Gunner)*B->WeaponReloadScale(0);
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("model_index"),Model);Row->SetNumberField(TEXT("host_time"),B->Elapsed);
            Row->SetNumberField(TEXT("range_cm"),FVector::Dist(Gunner->GetMuzzleLocation(),Tank->ObservedArmourAimPointV837(Gunner->Feet())));
            Row->SetNumberField(TEXT("penetration_probability"),Round.Chance);Row->SetNumberField(TEXT("unmodified_roll"),Round.Roll);
            Row->SetBoolField(TEXT("physical_target_hit"),Round.Tank.Get()==Tank);Row->SetNumberField(TEXT("flight_seconds"),Round.Arrival-B->Elapsed);
            Advance(FMath::Max(.013f,Round.Arrival-B->Elapsed+.001f));
            Row->SetNumberField(TEXT("hull_damage"),HullBefore-Tank->GetHullIntegrity());Row->SetNumberField(TEXT("applied_hits"),Mission(B).Readback.AppliedHits-BeforeShot.AppliedHits);
            ShotRows.Add(MakeShared<FJsonValueObject>(Row));
        }
        const auto After=Mission(B).Readback;const int32 Credited=B->FindSquad(CrewId)->KillsV65.Armour-PriorKills;
        Case->SetNumberField(TEXT("shots"),After.Fired-Before.Fired);Case->SetNumberField(TEXT("physical_hits"),After.PhysicalHullHits-Before.PhysicalHullHits);
        Case->SetNumberField(TEXT("applied_hits"),After.AppliedHits-Before.AppliedHits);Case->SetNumberField(TEXT("world_interceptions"),After.WorldInterceptions-Before.WorldInterceptions);
        Case->SetNumberField(TEXT("hull_damage"),After.HullDamage-Before.HullDamage);Case->SetNumberField(TEXT("track_damage"),After.TrackDamage-Before.TrackDamage);
        Case->SetNumberField(TEXT("remaining_hull"),Tank->GetHullIntegrity());Case->SetNumberField(TEXT("sim_seconds"),B->Elapsed-Started);
        Case->SetNumberField(TEXT("same_section_actual_armour_kills"),Credited);Case->SetBoolField(TEXT("destroyed"),!Tank->IsOperational());
        Assert(FString::Printf(TEXT("v84412_at_full_health_target_%d_destroyed_by_actual_projectiles"),Model),!Tank->IsOperational()&&Credited==1&&After.AppliedHits>Before.AppliedHits);
        Cases.Add(MakeShared<FJsonValueObject>(Case));Tank->Destroy();if(!BudgetOkay)break;
    }
    const auto* ActualCrew=B->FindSquad(CrewId);const auto& Record=B->CombatRecordV65[0];
    Assert(TEXT("v84412_at_two_actual_armour_kills_same_living_section"),Cases.Num()==2&&ActualCrew&&ActualCrew->KillsV65.Armour==2&&Gunner->IsAlive()&&Record.Total.Armour==2&&Record.InfantrySources[int32(EFrontlineRole::AntiTankRifle)].Armour==2&&Record.OtherSources.Armour==0);
    Assert(TEXT("v84412_at_projectile_actual_source_and_delayed_damage"),AllSources&&Deferred);
    Assert(TEXT("v84412_at_projectile_real_reload_schedule_no_free_round"),ScheduleOkay&&Gunner->ReloadsCompleted>=Gunner->ShotsDischarged-1);
    Assert(TEXT("v84412_at_projectile_simulation_bounded_three_cpu_seconds"),BudgetOkay&&FPlatformTime::Seconds()-SimulationBegan<3.);
    Result->SetArrayField(TEXT("cases"),Cases);Result->SetArrayField(TEXT("shot_receipts"),ShotRows);
    Result->SetNumberField(TEXT("shots"),B->ShotsFired);Result->SetNumberField(TEXT("hits"),Mission(B).Readback.AppliedHits);
    Result->SetNumberField(TEXT("sim_seconds"),B->Elapsed-SimulationStart);Result->SetNumberField(TEXT("simulation_cpu_seconds"),FPlatformTime::Seconds()-SimulationBegan);
    Result->SetNumberField(TEXT("same_section_actual_armour_kills"),ActualCrew?ActualCrew->KillsV65.Armour:0);
    return Result;
}
