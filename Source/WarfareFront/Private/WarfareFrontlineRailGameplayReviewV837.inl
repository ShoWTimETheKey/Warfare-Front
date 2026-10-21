// Bounded tests of production actors, clocks, gates and damage. No second model.
static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    FString User;const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV837Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
    Assert(TEXT("v837_rail_strict_disposable_ukde_review"),Safe);
    auto* Fleet=Safe?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;if(!Fleet){Report->SetBoolField(TEXT("success"),false);return Report;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Vehicles=Fleet->Vehicles;
    const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;
    const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
    const int32 Mission=B->BoundCampaignMission,NextSquad=B->NextSquadId,Serial=Fleet->NextSerial;
    const auto Phase=B->Phase;const int32 Winner=B->BattleOutroWinner,Shots=B->ShotsFired,Thrown=B->GrenadesThrown,Commands=B->RecruitCommands;
    const auto Strikes=B->Strikes;const auto Tracers=B->Tracers;const auto Smoke=B->SmokeZones;const auto Random=B->Random;
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
    const auto Encounter=Fleet->LastEncounter;const float Clock=B->Elapsed,EnemyAt=Fleet->NextEnemyAt,Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
    const int32 Deployment[]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
    const float Cycle[]={Fleet->ArmourCycleSecondsV40[0],Fleet->ArmourCycleSecondsV40[1]},RailReady[]={Fleet->RailReadyAtV75[0],Fleet->RailReadyAtV75[1]};
    const float Morale[]={B->Morale[0],B->Morale[1]},Damage[]={B->DamageDealt[0],B->DamageDealt[1]};const int32 Casualties[]={B->Casualties[0],B->Casualties[1]};
    float Ready[2][int32(EFrontlineVehicle::Count)],Recruit[2][AWarfareFrontlineGameMode::RoleCount];
    for(int32 T=0;T<2;++T){for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Ready[T][I]=Fleet->ReadyAt[T][I];for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)Recruit[T][I]=B->RecruitReadyAt[T][I];}
    const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());UGameplayStatics::SetGamePaused(B,false);
    TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AWarfareFrontlineSoldier* U:Units)Hide(U);for(AFrontlineTank* V:Vehicles)Hide(V);
    int32 Ordinal=0;
    const auto Clear=[&]()
    {
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->Tracers.Reset();B->SmokeZones.Reset();Fleet->Vehicles.Reset();
        FrontlineSpecialistsV41::Reset(B);FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        B->Elapsed=0;B->BattleOutroWinner=INDEX_NONE;B->Phase=EFrontlinePhase::Fighting;
        B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};B->Casualties[0]=B->Casualties[1]=0;B->Morale[0]=B->Morale[1]=100;
        B->NextSquadId=8370000+(++Ordinal)*100;B->Random.Initialize(837);Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();Fleet->NextSerial=837000+Ordinal*100;
    };
    ON_SCOPE_EXIT
    {
        Clear();B->Units=Units;B->Squads=Squads;Fleet->Vehicles=Vehicles;B->CampaignRuns=Runs;B->PlayerFaction=Faction;
        B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=Mission;
        B->NextSquadId=NextSquad;B->Elapsed=Clock;B->Phase=Phase;B->BattleOutroWinner=Winner;B->ShotsFired=Shots;B->GrenadesThrown=Thrown;B->RecruitCommands=Commands;
        B->Strikes=Strikes;B->Tracers=Tracers;B->SmokeZones=Smoke;B->Random=Random;B->LastMessage=Message;
        B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
        Fleet->LastEncounter=Encounter;Fleet->NextSerial=Serial;Fleet->NextEnemyAt=EnemyAt;
        for(int32 T=0;T<2;++T){Fleet->DeploymentCountV37[T]=Deployment[T];Fleet->ArmourCycleSecondsV40[T]=Cycle[T];Fleet->RailReadyAtV75[T]=RailReady[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];B->Casualties[T]=Casualties[T];
            for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Fleet->ReadyAt[T][I]=Ready[T][I];for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)B->RecruitReadyAt[T][I]=Recruit[T][I];}
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        UGameplayStatics::SetGamePaused(B,Paused);Report->SetArrayField(TEXT("cases"),Rows);Report->SetBoolField(TEXT("success"),Good);
        Report->SetStringField(TEXT("scope"),TEXT("Six bounded UK/DE rail allocations with genuine model cooldown and difficulty supply clocks, actual convoy/unloading and physical hull/door checks. Independent held-contact and live-neighbour movement cases on the prepared authored terrain. No FR/RU content, whole-campaign enjoyment, all-map balance or visual-quality score."));
    };
    const auto Campaign=[&](EFrontlineFaction Nation,EFrontlineDifficulty Difficulty,int32 Selected=17)
    {
        B->PlayerFaction=Nation;FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=Nation;R.Difficulty=Difficulty;R.CompletedMissions=19;R.SelectedMission=Selected;
        R.WonOperationMaskV52=(1u<<19)-1;R.DispatchResolvedMaskV52=(1u<<20)-1;R.RoleSelectionMask=(1u<<int32(EFrontlineRole::Count))-1;
        R.ArmourResearchMaskV63=65535;R.ArmourSelectionMaskV63=(1u<<int32(EFrontlineVehicle::RailConvoy))|(1u<<int32(EFrontlineVehicle::A7V));
        B->CampaignRuns={R};B->ActiveCampaignId=B->BoundCampaignId=R.Id;B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=Selected;
        Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();
    };
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();}};
    const auto Soldier=[&](int32 Team,const FVector& P,int32 Id)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;
        U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+U->GroundContactOffsetV82));U->ReturnToIdle();
        U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0.f:180.f,0));U->SetMotion(U->GetAimMotion());B->Units.Add(U);Pose(U,.4f);return U;
    };
    float InitialEnemyTimes[3]={0},Replenishment[3]={0};
    for(int32 Nation=0;Nation<2;++Nation)for(int32 Difficulty=0;Difficulty<3;++Difficulty)
    {
        Clear();Campaign(Nation?EFrontlineFaction::German:EFrontlineFaction::British,EFrontlineDifficulty(Difficulty));
        const int32 Team=Nation?0:1,Dir=B->GetTeamDirection(Team);
        const float ActualFirst=Fleet->ReadyAt[Team][int32(EFrontlineVehicle::RailConvoy)],AIAt=Fleet->NextEnemyAt;
        B->Elapsed=FMath::Max(ActualFirst,Team?AIAt:0.f)+.01f;
        bool Requested=false;if(Team==0)Requested=Fleet->Recruit(EFrontlineVehicle::RailConvoy,Team);else{Fleet->Tick(.05f);Requested=Fleet->Vehicles.ContainsByPredicate([](const auto& V){return V&&V->IsRailConvoyV32();});}
        AFrontlineTank* Train=nullptr;for(AFrontlineTank* V:Fleet->Vehicles)if(V&&V->IsRailConvoyV32())Train=V;
        const FString Prefix=FString::Printf(TEXT("v837_rail_army%d_difficulty%d_"),Nation,Difficulty);
        Assert(Prefix+TEXT("actual_public_allocation_after_authored_readiness"),Requested&&Train&&ActualFirst>0.f);
        if(!Train)continue;
        const float Queue=Fleet->ReadyAt[Team][0]-B->Elapsed;
        if(Team==1){InitialEnemyTimes[Difficulty]=FMath::Max(ActualFirst,AIAt);Replenishment[Difficulty]=Queue;}
        Train->SetActorTickEnabled(false);Train->bEntered=true;
        const float ActualHome=B->Trenches[B->GetHomeTrenchIndex(Team)].X*Dir;
        const float RailheadProgress=FMath::Max(ActualHome+900.f,-FrontlinePacingV24::CombatHalfLength()+1550.f);
        Train->FitGroundV35(FVector(Dir*RailheadProgress,-1130.f,28.f),0);
        const FVector FrontDoor=Train->GetActorTransform().TransformPosition(FVector(1320,0,0));
        const FVector RearDoor=Train->GetActorTransform().TransformPosition(FVector(-1400,0,0));
        const bool DoorsAtRailhead=RailheadProgress>=ActualHome+700.f
            &&FMath::Abs(FrontDoor.X)<FrontlinePacingV24::CombatHalfLength()-100.f
            &&FMath::Abs(RearDoor.X)<FrontlinePacingV24::CombatHalfLength()-100.f;
        Assert(Prefix+TEXT("fixture_reaches_actual_railhead_with_both_doors_in_physical_field"),DoorsAtRailhead);
        bool Physical=true,Staged=false,Stopped=false;float FirstRelease=-1,LastRelease=-1;
        int32 Visible=0;const FVector Start=Train->GetActorLocation();
        for(int32 F=0;F<140&&!Train->RailHasUnloadedV75();++F)
        {
            B->Elapsed+=.05f;Train->Simulate(.05f);int32 Present=0;
            const auto Members=B->Units;
            for(AWarfareFrontlineSoldier* U:Members)if(IsValid(U)&&!U->IsHidden()&&U->IsAlive())
            {
                ++Present;Physical&=!Train->BlocksInfantryAtV8218(U->Feet(),Train->GetActorLocation(),Train->HeadingYawV37)
                    &&FMath::Abs(U->Feet().Y)<FrontlineBattlefield::HalfFrontage-35.f;
                U->Tick(.05f);Pose(U,.05f);
            }
            if(Present>Visible){if(FirstRelease<0)FirstRelease=(F+1)*.05f;LastRelease=(F+1)*.05f;Visible=Present;}
            Staged|=Present>0&&Present<10;Stopped|=Present<10&&FVector::Dist2D(Start,Train->GetActorLocation())<.01f;
        }
        Assert(Prefix+TEXT("physical_stopped_doors_release_two_real_sections_in_stages"),Train->RailHasUnloadedV75()&&Train->RailPassengersV75()==0&&Visible==10&&Staged&&Stopped&&Physical&&LastRelease>FirstRelease+.5f);
        Assert(Prefix+TEXT("twenty_independent_bogie_wheels_four_doors_two_cupolas"),Train->RailBogieWheelsV837.Num()==20&&Train->RailDoorsV837.Num()==4&&Train->RailCupolasV837.Num()==2);
        bool PhysicalRailMuzzles=true;
        for(int32 Gun=0;Gun<Train->Cannons.Num();++Gun)
        {
            const auto* Barrel=Train->Cannons[Gun].Get();const FVector Muzzle=Barrel->GetComponentTransform().InverseTransformPosition(Train->WeaponMuzzle(Gun,true));
            const FBox Geometry=Barrel->GetStaticMesh()?Barrel->GetStaticMesh()->GetBoundingBox():FBox(ForceInit);
            PhysicalRailMuzzles&=Barrel->GetStaticMesh()&&Barrel->GetStaticMesh()->GetName()==TEXT("SM_RailCannon_V837")
                &&Geometry.IsValid&&Geometry.Max.X>=216.f&&FMath::IsNearlyEqual(float(Muzzle.X),217.f,.01f)
                &&Muzzle.X>130.f&&Muzzle.X<=Geometry.Max.X+2.f;
        }
        Assert(Prefix+TEXT("actual_exposed_rail_barrel_and_flash_share_measured_mesh_end"),PhysicalRailMuzzles);
        // The ordinary common queue may finish, but the single live train stays
        // ineligible. Its queue is never delayed or reset by that grey state.
        B->Elapsed=Fleet->ReadyAt[Team][0]+.01f;
        const float ReadyBefore=Fleet->ReadyAt[Team][0];
        const bool Grey=!Fleet->IsDeploymentEligibleV837(EFrontlineVehicle::RailConvoy,Team)
            &&Fleet->CooldownFraction(EFrontlineVehicle::RailConvoy,Team)==0.f&&!Fleet->Recruit(EFrontlineVehicle::RailConvoy,Team);
        Assert(Prefix+TEXT("one_live_special_stays_blocked_after_cooldown_without_clock_mutation"),Grey&&Fleet->ReadyAt[Team][0]==ReadyBefore);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_rail_supply_and_disembark"));J->SetNumberField(TEXT("player_army"),Nation);J->SetNumberField(TEXT("difficulty"),Difficulty);
        J->SetNumberField(TEXT("first_model_ready_seconds"),ActualFirst);J->SetNumberField(TEXT("enemy_decision_ready_seconds"),AIAt);J->SetNumberField(TEXT("real_shared_replenishment_seconds"),Queue);
        J->SetNumberField(TEXT("first_release_seconds"),FirstRelease);J->SetNumberField(TEXT("last_release_seconds"),LastRelease);J->SetNumberField(TEXT("visible_real_passengers"),Visible);J->SetBoolField(TEXT("grounded_hull_safe_release"),Physical);
        J->SetNumberField(TEXT("actual_home_progress_cm"),ActualHome);J->SetNumberField(TEXT("fixture_railhead_progress_cm"),RailheadProgress);J->SetNumberField(TEXT("combat_half_length_cm"),FrontlinePacingV24::CombatHalfLength());
        J->SetBoolField(TEXT("both_actual_doors_inside_at_railhead"),DoorsAtRailhead);J->SetNumberField(TEXT("actual_unload_clock_seconds"),Train->RailDisembarkClockV75);J->SetNumberField(TEXT("actual_door_phase"),Train->RailDoorPhaseV837);
        J->SetNumberField(TEXT("actual_section_mask"),Train->RailSectionMaskV837);J->SetNumberField(TEXT("actual_forward_members_released"),Train->RailReleasedV837[0]);J->SetNumberField(TEXT("actual_rear_members_released"),Train->RailReleasedV837[1]);J->SetNumberField(TEXT("actual_materialized_squads"),B->Squads.Num());
        J->SetBoolField(TEXT("grey_despite_ready_queue"),Grey);Rows.Add(MakeShared<FJsonValueObject>(J));
        // Four real local hull contacts, not a ghost HP bonus: loss of the
        // front gun car and engine leaves the opposing firepower/crew intact.
        const auto Credit=FFrontlineKillSourceV65::FireSupport(1-Team,int32(EFrontlineSupport::AntiTank));
        const float Hull=Train->GetHullIntegrity();Train->ReceiveRailContactV837(Train->GetActorTransform().TransformPosition(FVector(2210,160,240)),400.f,Credit);
        Assert(Prefix+TEXT("front_gun_car_failure_keeps_rear_gun_alive_and_chassis_hp_unchanged"),!Train->RailCannonOperationalV837(0)&&Train->RailCannonOperationalV837(1)&&Train->GetHullIntegrity()==Hull);
        Train->ReceiveRailContactV837(Train->GetActorTransform().TransformPosition(FVector(300,140,220)),600.f,Credit);
        const FVector Disabled=Train->GetActorLocation();Train->Simulate(.1f);
        Assert(Prefix+TEXT("targeted_engine_hit_stops_healthy_chassis_without_silencing_rear_gun"),Train->IsOperational()&&Train->IsImmobilized()&&Train->RailEngineIntegrityV837()==0.f&&Train->RailCannonOperationalV837(1)&&Train->GetActorLocation().Equals(Disabled,.01f));
        Train->ReceiveModuleHit(EFrontlineTankModule::Hull,Train->GetHullIntegrity()+1.f,0,Credit);
        Assert(Prefix+TEXT("actual_wreck_releases_single_live_gate_without_resetting_cooldown"),Fleet->IsDeploymentEligibleV837(EFrontlineVehicle::RailConvoy,Team)&&Fleet->ReadyAt[Team][0]==ReadyBefore);
    }
    Assert(TEXT("v837_rail_enemy_supply_reaction_scales_by_difficulty_without_hp_or_weapon_buffs"),InitialEnemyTimes[0]>InitialEnemyTimes[1]&&InitialEnemyTimes[1]>InitialEnemyTimes[2]
        &&Replenishment[0]>Replenishment[1]&&Replenishment[1]>Replenishment[2]);
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();Campaign(Team?EFrontlineFaction::British:EFrontlineFaction::German,EFrontlineDifficulty::Veteran,19);
        B->CampaignRuns[0].ArmourSelectionMaskV63=1u<<int32(EFrontlineVehicle::A7V);
        B->CampaignRuns[0].ArmourConfigurationMaskV836=1u<<int32(EFrontlineVehicle::A7V);
        auto* Gretchen=Fleet->Spawn(EFrontlineVehicle::A7VGretchen,Team,true);
        B->Elapsed=Fleet->ReadyAt[Team][int32(EFrontlineVehicle::A7VGretchen)]+.01f;
        const bool LiveBlocked=Gretchen&&!Fleet->IsDeploymentEligibleV837(EFrontlineVehicle::A7VGretchen,Team)
            &&Fleet->CooldownFraction(EFrontlineVehicle::A7VGretchen,Team)==0.f;
        if(Gretchen)Gretchen->ReceiveModuleHit(EFrontlineTankModule::Hull,Gretchen->GetHullIntegrity()+1);
        UGameplayStatics::SetGamePaused(B,true);
        const bool Freed=Fleet->IsDeploymentEligibleV837(EFrontlineVehicle::A7VGretchen,Team);
        UGameplayStatics::SetGamePaused(B,false);
        Assert(FString::Printf(TEXT("v837_gretchen_team%d_ready_live_block_and_wreck_eligibility_independent_of_pause"),Team),LiveBlocked&&Freed);
    }
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();Campaign(Team?EFrontlineFaction::British:EFrontlineFaction::German,EFrontlineDifficulty::Veteran);
        auto* Train=Fleet->Spawn(EFrontlineVehicle::RailConvoy,Team,true);if(!Train){Assert(TEXT("v837_rail_mount_observation_assets"),false);continue;}
        const int32 Dir=B->GetTeamDirection(Team);Train->SetActorTickEnabled(false);
        // The nose/gun car has naturally crossed the entry while its engine
        // remains in the rear approach, as in normal 2.8-second recruitment.
        Train->FitGroundV35(FVector(-Dir*(FrontlinePacingV24::CombatHalfLength()+600.f),-1130,28),0);
        Train->bEntered=Train->HasEnteredField();
        const float Range=MountedInfantryRangeV836(B,EFrontlineVehicle::RailConvoy,false);
        AWarfareFrontlineSoldier* Target=nullptr;bool ClearMount=false;float CentreDistance=0,MuzzleDistance=0;
        for(float Lane:{-700.f,-300.f,300.f})
        {
            if(Target){B->Units.Remove(Target);Target->Destroy();}
            Target=Soldier(1-Team,Train->GetActorLocation()+FVector(Dir*(Range+450.f),Lane+1130.f,0),837950+Team);
            if(!Target)continue;FVector Point;
            CentreDistance=FVector::Dist2D(Train->GetActorLocation(),Target->Feet());
            for(int32 Gun=0;Gun<Train->GunMounts.Num();++Gun)
                if(Train->InfantryFiringSolutionV828(Target,Gun,false,Point))
                {ClearMount=true;MuzzleDistance=FVector::Dist2D(Train->WeaponMuzzle(Gun,false),Point);break;}
            if(ClearMount)break;
        }
        const int32 BeforeShots=Train->MachinegunShots;
        for(int32 F=0;F<100&&Target&&Train->MachinegunShots==BeforeShots;++F){B->Elapsed+=.05f;Train->Simulate(.05f);Pose(Target,.05f);}
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("rail_real_muzzle_outside_central_engine_range"));J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("centre_distance_cm"),CentreDistance);J->SetNumberField(TEXT("mount_distance_cm"),MuzzleDistance);J->SetNumberField(TEXT("mounted_range_cm"),Range);J->SetNumberField(TEXT("actual_mg_shots"),Train->MachinegunShots-BeforeShots);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v837_rail_team%d_end_car_fires_from_real_muzzle_outside_engine_centre_range"),Team),ClearMount&&CentreDistance>Range&&MuzzleDistance<Range&&Train->MachinegunShots>BeforeShots);
    }
    // British opponent must select a cannon counter from actually observed
    // German rail armour. Rookie retains its simple ordinary roster rotation.
    for(int32 Difficulty=0;Difficulty<3;++Difficulty)
    {
        Clear();Campaign(EFrontlineFaction::German,EFrontlineDifficulty(Difficulty));
        const float CurrentRailReady=Fleet->ReadyAt[0][int32(EFrontlineVehicle::RailConvoy)];B->Elapsed=CurrentRailReady+.01f;
        const bool Deployed=Fleet->Recruit(EFrontlineVehicle::RailConvoy,0);AFrontlineTank* Train=Deployed?Fleet->Vehicles.Last().Get():nullptr;
        if(Train){Train->SetActorTickEnabled(false);Train->bEntered=true;Train->FitGroundV35(FVector(0,-1130,28),0);}
        auto* Observer=Soldier(1,FVector(-1800.f,-700.f,0),837880+Difficulty);bool Seen=Train&&Observer&&ClearSight(B->GetWorld(),Observer,Train,Observer->Feet()+FVector(0,0,145),Train->AimPoint());
        B->Elapsed=FMath::Max(Fleet->NextEnemyAt,Fleet->ReadyAt[1][0])+.01f;Fleet->Tick(.05f);
        AFrontlineTank* Counter=nullptr;for(AFrontlineTank* V:Fleet->Vehicles)if(V&&V->Team==1)Counter=V;
        const float CounterQueueSeconds=Fleet->ReadyAt[1][0]-B->Elapsed,ExchangeStart=B->Elapsed;
        const float HullBefore=Train?Train->GetHullIntegrity():0.f;
        float CarBefore=0.f;if(Train)for(int32 Car=0;Car<5;++Car)CarBefore+=Train->RailCarIntegrityV837(Car);
        for(int32 F=0;F<500&&Train&&Counter&&Train->IsOperational()&&Counter->IsOperational();++F)
        {B->Elapsed+=.05f;Train->Simulate(.05f);Counter->Simulate(.05f);}
        float CarAfter=0.f;if(Train)for(int32 Car=0;Car<5;++Car)CarAfter+=Train->RailCarIntegrityV837(Car);
        const float ActualHullDamage=Train?HullBefore-Train->GetHullIntegrity():0.f,ActualCarDamage=CarBefore-CarAfter;
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_enemy_observed_rail_counter"));J->SetNumberField(TEXT("difficulty"),Difficulty);J->SetBoolField(TEXT("observer_actual_hull_line"),Seen);
        J->SetNumberField(TEXT("actual_enemy_model"),Counter?int32(Counter->Type):-1);J->SetNumberField(TEXT("actual_enemy_cannon_count"),Counter?Counter->Cannons.Num():0);
        J->SetNumberField(TEXT("actual_counter_cannon_shots"),Counter?Counter->CannonShots:0);J->SetNumberField(TEXT("actual_rail_hull_damage"),ActualHullDamage);J->SetNumberField(TEXT("actual_rail_carriage_damage"),ActualCarDamage);
        J->SetNumberField(TEXT("actual_counter_hull_remaining"),Counter?Counter->GetHullIntegrity():0.f);
        J->SetNumberField(TEXT("ordinary_model_replenishment_seconds"),CounterQueueSeconds);J->SetNumberField(TEXT("actual_exchange_seconds"),B->Elapsed-ExchangeStart);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v837_difficulty%d_enemy_dispatches_actual_rail_counter_from_seen_hull"),Difficulty),Deployed&&Seen&&Counter&&(Difficulty==0||!Counter->Cannons.IsEmpty()));
        if(Difficulty>0)Assert(FString::Printf(TEXT("v837_difficulty%d_real_counter_fires_and_damages_convoy_in_bounded_live_exchange"),Difficulty),Counter&&Counter->CannonShots>0&&(ActualHullDamage>0||ActualCarDamage>0));
    }
    // Empty approaches and a near live neighbour, in both army directions.
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();Campaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran,19);
        auto* Moving=Fleet->Spawn(Team?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkVStar,Team,true);
        auto* Neighbour=Fleet->Spawn(Team?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,Team,true);
        if(!Moving||!Neighbour){Assert(TEXT("v837_neighbour_contact_assets"),false);continue;}
        const int32 Dir=B->GetTeamDirection(Team);const float X=B->Trenches[B->Trenches.Num()/2].X-Dir*900.f;
        for(auto* V:{Moving,Neighbour}){V->SetActorTickEnabled(false);V->bEntered=true;V->HeadingYawV37=Dir>0?0.f:180.f;}
        Moving->PreferredLaneV37=Moving->DesiredLaneV37=300;Moving->FitGroundV35(FVector(X,300,0),0);
        Neighbour->PreferredLaneV37=Neighbour->DesiredLaneV37=900;Neighbour->FitGroundV35(FVector(X+Dir*1100.f,900,0),0);
        const FVector Start=Moving->GetActorLocation();bool SafeSteps=true;int32 StoppedFrames=0;
        for(int32 F=0;F<240;++F)
        {
            B->Elapsed+=.05f;const FVector Before=Moving->GetActorLocation();Moving->Simulate(.05f);
            const FVector After=Moving->GetActorLocation();const float Travel=FVector::Dist2D(Before,After);
            SafeSteps&=Travel<=Fleet->Definition(Moving->Type).SpeedCm*.051f*FrontlineArmourV63::TravelSpeedScaleV8218(B,Team)
                &&(After.X-Before.X)*Dir>=-.01f&&!After.ContainsNaN()&&Moving->CanOccupyV48(After,Moving->HeadingYawV37);StoppedFrames+=Travel<.01f;
        }
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_empty_approach_with_live_neighbour"));J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("travel_cm"),FVector::Dist2D(Start,Moving->GetActorLocation()));J->SetNumberField(TEXT("stopped_frames"),StoppedFrames);J->SetBoolField(TEXT("bounded_no_reverse_no_overlap"),SafeSteps);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v837_army%d_healthy_hull_makes_bounded_forward_travel_around_live_neighbour"),Team),SafeSteps&&(Moving->GetActorLocation().X-Start.X)*Dir>500.f&&StoppedFrames<200);
    }
    // A real short-range sight obstruction: pivot rays pass beside two narrow
    // wall piers, while actual protruding sponson muzzles are screened. Hull
    // observation must not turn that unusable contact into an indefinite stop.
    Clear();Campaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran,19);
    auto* Screened=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,0,true);
    auto* CloseHull=Fleet->Spawn(EFrontlineVehicle::A7V,1,true);
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    TArray<AActor*> Piers;TArray<FRotator> OriginalCannonPose;
    if(Screened&&CloseHull&&Cube)
    {
        for(auto* V:{Screened,CloseHull}){V->SetActorTickEnabled(false);V->bEntered=true;V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;}
        Screened->PreferredLaneV37=Screened->DesiredLaneV37=0;Screened->FitGroundV35(FVector(-1000,0,0),0);
        CloseHull->PreferredLaneV37=CloseHull->DesiredLaneV37=470;CloseHull->FitGroundV35(FVector(-600,470,0),0);
        for(int32 Mount=0;Mount<Screened->CannonMounts.Num();++Mount)
        {
            OriginalCannonPose.Add(Screened->Cannons[Mount]->GetRelativeRotation());
            const FVector Muzzle=Screened->WeaponMuzzle(Mount,true);
            FVector Toward=CloseHull->AimPoint()-Muzzle;Toward.Z=0;Toward.Normalize();
            auto* Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall)continue;
            auto* Solid=NewObject<UStaticMeshComponent>(Wall);Wall->SetRootComponent(Solid);Solid->SetStaticMesh(Cube);
            Solid->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Solid->SetCollisionObjectType(ECC_WorldStatic);
            Solid->SetCollisionResponseToAllChannels(ECR_Ignore);Solid->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            Solid->RegisterComponent();Solid->SetWorldLocation(Muzzle+Toward*40.f);Solid->SetWorldScale3D(FVector(.16,.16,4.f));Piers.Add(Wall);
        }
    }
    FVector SeenPoint;const bool Observed=Screened&&CloseHull&&Screened->ArmourSightV48(CloseHull,SeenPoint);
    bool Usable=false;int32 ActualBlockedMuzzles=0;
    if(Screened&&CloseHull)for(int32 Mount=0;Mount<Screened->CannonMounts.Num();++Mount)
    {FVector P;Usable|=Screened->ArmourFiringSolutionV8219(CloseHull,Mount,P);
        ActualBlockedMuzzles+=!ClearSight(B->GetWorld(),Screened,CloseHull,Screened->WeaponMuzzle(Mount,true),CloseHull->AimPoint());}
    const FVector SightStart=Screened?Screened->GetActorLocation():FVector::ZeroVector;
    const float ShortRange=Screened&&CloseHull?FVector::Dist2D(Screened->AimPoint(),CloseHull->AimPoint()):0.f;
    if(Screened)for(int32 F=0;F<12;++F){B->Elapsed+=.05f;Screened->Simulate(.05f);}
    const float Reposition=Screened?FVector::Dist2D(SightStart,Screened->GetActorLocation()):0.f;
    Assert(TEXT("v837_actual_short_wall_counterexample_has_observation_but_no_mounted_solution"),Piers.Num()==2&&Observed&&!Usable&&ActualBlockedMuzzles>0&&ShortRange<700.f);
    Assert(TEXT("v837_actual_short_wall_contact_does_not_force_healthy_hull_permanent_hold"),Screened&&Screened->IsOperational()&&!Screened->IsImmobilized()&&Reposition>.1f&&Screened->CanOccupyV48(Screened->GetActorLocation(),Screened->HeadingYawV37));
    // Smoke removes that unusable hull-eye contact for a short interval. Only a
    // formerly real mounted firing position may own the existing 1.5 s hold.
    bool RestoredMounted=false,ProductionRemembered=false,ProductionUsable=true;float SmokeReposition=0,SmokeVisibility=1;
    float SmokeHeadingBefore=0,SmokeHeadingAfter=0,SmokeAcrossGap=0,SmokeForwardProgress=0;bool SmokeStartingHullClear=false,SmokeStepsSafe=true,SmokeMemoryUnused=true;
    int32 SmokeStoppedFrames=0;
    TArray<TSharedPtr<FJsonValue>> SmokeSteps;
    if(Screened&&CloseHull)
    {
        // Revisit the same controlled actual obstruction and let production
        // observation record its failed mounted status; do not seed a memory.
        Screened->HeadingYawV37=B->GetTeamDirection(0)>0?0.f:180.f;Screened->FitGroundV35(SightStart,0);
        for(int32 Mount=0;Mount<OriginalCannonPose.Num();++Mount)Screened->Cannons[Mount]->SetRelativeRotation(OriginalCannonPose[Mount]);
        FVector RestoredPoint;
        for(int32 Mount=0;Mount<Screened->CannonMounts.Num();++Mount)RestoredMounted|=Screened->ArmourFiringSolutionV8219(CloseHull,Mount,RestoredPoint);
        Assert(TEXT("v837_actual_wall_memory_fixture_restores_original_physical_barrel_pose"),!RestoredMounted);
        B->Elapsed+=.05f;Screened->Simulate(.05f);
        ProductionRemembered=Screened->ArmourContactV48.Get()==CloseHull;ProductionUsable=Screened->bArmourContactUsableV837;
        Assert(TEXT("v837_actual_unusable_contact_does_not_acquire_firing_position_memory"),ProductionRemembered&&!ProductionUsable);
        FFrontlineSmokeZone Curtain;Curtain.Centre=(Screened->AimPoint()+CloseHull->AimPoint())*.5;Curtain.Radius=950;
        Curtain.StartedAt=B->Elapsed-2.f;Curtain.ExpiresAt=B->Elapsed+20.f;B->SmokeZones.Add(Curtain);
        const FVector BeforeSmoke=Screened->GetActorLocation();SmokeHeadingBefore=Screened->HeadingYawV37;
        SmokeAcrossGap=FMath::Abs(CloseHull->GetActorLocation().Y-BeforeSmoke.Y);SmokeStartingHullClear=Screened->CanOccupyV48(BeforeSmoke,Screened->HeadingYawV37);
        // Give the real steering mechanism 1.25 s. A safe bounded turn is a
        // legitimate zero-translation frame; it is not a cached-contact hold.
        // All original smoke, hull, forward-only and contact rules remain live.
        for(int32 Frame=0;Frame<25;++Frame)
        {
            const FVector Before=Screened->GetActorLocation();const float Heading=Screened->HeadingYawV37;
            B->Elapsed+=.05f;Screened->Simulate(.05f);
            const FVector After=Screened->GetActorLocation();const float Travel=FVector::Dist2D(Before,After);
            SmokeStoppedFrames+=Travel<=.01f;
            const float Progress=float((After.X-Before.X)*B->GetTeamDirection(Screened->Team));
            const bool Occupancy=Screened->CanOccupyV48(After,Screened->HeadingYawV37);
            const float Visibility=B->DirectFireMultiplier(Screened->AimPoint(),CloseHull->AimPoint());
            const bool MemoryUnused=!Screened->bArmourContactUsableV837;
            SmokeStepsSafe&=Occupancy&&Progress>=-.01f&&!After.ContainsNaN()
                &&Travel<=Fleet->Definition(Screened->Type).SpeedCm*.051f*FrontlineArmourV63::TravelSpeedScaleV8218(B,Screened->Team)
                &&FMath::Abs(FMath::FindDeltaAngleDegrees(Heading,Screened->HeadingYawV37))<=2.401f;
            SmokeMemoryUnused&=MemoryUnused&&Visibility<.9f;
            auto Step=MakeShared<FJsonObject>();Step->SetNumberField(TEXT("seconds"),(Frame+1)*.05f);
            Step->SetNumberField(TEXT("heading_before_deg"),Heading);Step->SetNumberField(TEXT("heading_after_deg"),Screened->HeadingYawV37);
            Step->SetNumberField(TEXT("travel_cm"),Travel);Step->SetNumberField(TEXT("forward_progress_cm"),Progress);
            Step->SetBoolField(TEXT("actual_hull_occupancy_clear"),Occupancy);Step->SetBoolField(TEXT("unusable_contact_memory_unused"),MemoryUnused);Step->SetNumberField(TEXT("smoke_visibility_multiplier"),Visibility);
            SmokeSteps.Add(MakeShared<FJsonValueObject>(Step));
        }
        SmokeHeadingAfter=Screened->HeadingYawV37;SmokeVisibility=B->DirectFireMultiplier(Screened->AimPoint(),CloseHull->AimPoint());
        SmokeReposition=FVector::Dist2D(BeforeSmoke,Screened->GetActorLocation());SmokeForwardProgress=float((Screened->GetActorLocation().X-BeforeSmoke.X)*B->GetTeamDirection(Screened->Team));
        Assert(TEXT("v837_recent_unusable_observation_does_not_hold_through_actual_smoke"),SmokeVisibility<.9f&&SmokeReposition>1.f&&SmokeForwardProgress>1.f&&SmokeStepsSafe&&SmokeMemoryUnused);
        Assert(TEXT("v837_smoke_clear_forward_arc_has_no_recurring_stall_wait"),SmokeForwardProgress>40.f&&SmokeStoppedFrames<=8);
    }
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("short_actual_wall_no_muzzle_solution"));J->SetNumberField(TEXT("range_cm"),ShortRange);
    J->SetBoolField(TEXT("actual_pivot_observation"),Observed);J->SetBoolField(TEXT("actual_mounted_solution"),Usable);J->SetNumberField(TEXT("blocked_muzzle_rays"),ActualBlockedMuzzles);
    J->SetNumberField(TEXT("actual_reposition_cm"),Reposition);Rows.Add(MakeShared<FJsonValueObject>(J));
    J->SetBoolField(TEXT("restored_pose_mounted_solution"),RestoredMounted);J->SetBoolField(TEXT("actual_contact_memory_present"),ProductionRemembered);J->SetBoolField(TEXT("actual_firing_position_memory"),ProductionUsable);
    J->SetNumberField(TEXT("actual_smoke_visibility_multiplier"),SmokeVisibility);J->SetNumberField(TEXT("actual_smoke_reposition_cm"),SmokeReposition);
    J->SetNumberField(TEXT("actual_smoke_heading_before_deg"),SmokeHeadingBefore);J->SetNumberField(TEXT("actual_smoke_heading_after_deg"),SmokeHeadingAfter);
    J->SetNumberField(TEXT("actual_smoke_lateral_hull_gap_cm"),SmokeAcrossGap);J->SetBoolField(TEXT("actual_smoke_initial_hull_occupancy_clear"),SmokeStartingHullClear);
    J->SetNumberField(TEXT("actual_smoke_observation_seconds"),1.25f);J->SetNumberField(TEXT("actual_smoke_forward_progress_cm"),SmokeForwardProgress);
    J->SetNumberField(TEXT("actual_smoke_stopped_frames"),SmokeStoppedFrames);
    J->SetBoolField(TEXT("actual_smoke_steps_bounded_forward_and_collision_safe"),SmokeStepsSafe);J->SetBoolField(TEXT("actual_smoke_failed_position_memory_stays_unused"),SmokeMemoryUnused);J->SetArrayField(TEXT("actual_smoke_steering_steps"),SmokeSteps);
    return Report;
}

// One final disposable scene, using the current operation's normal terrain,
// roster and actor construction. Root owns camera, timing and screenshot QA.
static bool PrepareVisual(AWarfareFrontlineGameMode* B,bool German)
{
    FString User;
    if(!B||!B->GetWorld()||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV837Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||User.IsEmpty()||FPaths::IsRelative(User))return false;
    if(!B->StartNewCampaign(German?EFrontlineFaction::German:EFrontlineFaction::British,EFrontlineDifficulty::Veteran))return false;
    auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!Run)return false;
    Run->CompletedMissions=19;Run->SelectedMission=17;Run->WonOperationMaskV52=(1u<<19)-1;Run->DispatchResolvedMaskV52=(1u<<20)-1;
    Run->ArmourResearchMaskV63=65535;Run->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;
    Run->ArmourSelectionMaskV63=German?(1u<<int32(EFrontlineVehicle::A7V))|(1u<<int32(EFrontlineVehicle::RailConvoy)):
        (1u<<int32(EFrontlineVehicle::MarkVStar))|(1u<<int32(EFrontlineVehicle::Austin1918));
    B->ResetEncounter();B->BeginBattle();if(B->Phase!=EFrontlinePhase::Fighting||!B->AssetFailure.IsEmpty())return false;
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return false;Fleet->LastEncounter=B->GetEncounterId();
    B->Elapsed=120.f;Fleet->NextEnemyAt=MAX_flt;
    const int32 RailTeam=German?0:1;
    auto* Train=Fleet->Spawn(EFrontlineVehicle::RailConvoy,RailTeam,true);
    auto* Main=Fleet->Spawn(German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkVStar,0,true);
    auto* Second=Fleet->Spawn(German?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::Austin1918,0,true);
    auto* Wreck=Fleet->Spawn(German?EFrontlineVehicle::BeuteMarkIVFemale:EFrontlineVehicle::Whippet,0,true);
    const auto Place=[&](AFrontlineTank* V,const FVector& P)
    {if(V){V->SetActorTickEnabled(false);V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->FitGroundV35(P,0);V->bEntered=V->HasEnteredField();}};
    Place(Train,FVector(0,-1130,28));Place(Main,FVector(1800,450,0));Place(Second,FVector(-1500,-300,0));Place(Wreck,FVector(-700,900,0));
    const bool Ready=Train&&Main&&Second&&Wreck&&Train->AreAssetsReady()&&Main->AreAssetsReady()&&Second->AreAssetsReady()&&Wreck->AreAssetsReady();
    if(Wreck)Wreck->ReceiveModuleHit(EFrontlineTankModule::Hull,Wreck->GetHullIntegrity()+1);
    // Doors and soldiers proceed through Simulate; no scripted discharge,
    // instant unloading, immortal targets or falsely reported combat result.
    return Ready;
}
