// Synchronous, disposable production-actor review. No alternate damage model.
static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    FString User;
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
    Assert(TEXT("v836_armour_strict_disposable_ukde_world"),Safe);
    auto* Fleet=Safe?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
    if(!Fleet){Report->SetBoolField(TEXT("success"),false);return Report;}
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTrenches=B->Trenches;
    const auto SavedVehicles=Fleet->Vehicles;const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;
    const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
    const int32 BoundMission=B->BoundCampaignMission,NextSquad=B->NextSquadId,Serial=Fleet->NextSerial;
    const auto Phase=B->Phase;const int32 Winner=B->BattleOutroWinner,Shots=B->ShotsFired,Thrown=B->GrenadesThrown,Commands=B->RecruitCommands;
    const auto Strikes=B->Strikes;const auto Tracers=B->Tracers;const auto Smoke=B->SmokeZones;const auto Random=B->Random;
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
    const auto Encounter=Fleet->LastEncounter;const float Clock=B->Elapsed,EnemyAt=Fleet->NextEnemyAt,Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
    const bool Legacy=LegacyNo44V836;
    const int32 Deployment[]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
    float Ready[2][int32(EFrontlineVehicle::Count)],Recruit[2][AWarfareFrontlineGameMode::RoleCount];
    const float Cycle[]={Fleet->ArmourCycleSecondsV40[0],Fleet->ArmourCycleSecondsV40[1]},RailReady[]={Fleet->RailReadyAtV75[0],Fleet->RailReadyAtV75[1]};
    const float Morale[]={B->Morale[0],B->Morale[1]},Damage[]={B->DamageDealt[0],B->DamageDealt[1]};const int32 Casualties[]={B->Casualties[0],B->Casualties[1]};
    for(int32 T=0;T<2;++T){for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Ready[T][I]=Fleet->ReadyAt[T][I];for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)Recruit[T][I]=B->RecruitReadyAt[T][I];}
    TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AWarfareFrontlineSoldier* U:SavedUnits)Hide(U);for(AFrontlineTank* V:SavedVehicles)Hide(V);
    int32 CaseOrdinalV836=0;
    const auto Clear=[&]()
    {
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->Tracers.Reset();B->SmokeZones.Reset();Fleet->Vehicles.Reset();
        FrontlineSpecialistsV41::Reset(B);FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        B->Elapsed=1000;B->BattleOutroWinner=INDEX_NONE;B->Phase=EFrontlinePhase::Fighting;
        B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};B->Casualties[0]=B->Casualties[1]=0;
        B->Morale[0]=B->Morale[1]=100;B->ShotsFired=B->GrenadesThrown=0;B->Random.Initialize(836);
        // Death records deduplicate Encounter+Serial. Each disposable case must
        // receive distinct victim identities, exactly as public spawns do.
        Fleet->NextSerial=836000+(++CaseOrdinalV836)*100;B->NextSquadId=8360000+CaseOrdinalV836*100;Fleet->DeploymentCountV37[0]=Fleet->DeploymentCountV37[1]=0;
        Fleet->LastEncounter=B->GetEncounterId();Fleet->NextEnemyAt=MAX_flt;
        for(int32 T=0;T<2;++T){for(float& At:Fleet->ReadyAt[T])At=0;for(float& At:B->RecruitReadyAt[T])At=0;Fleet->RailReadyAtV75[T]=0;}
    };
    ON_SCOPE_EXIT
    {
        Clear();B->Units=SavedUnits;B->Squads=SavedSquads;B->Trenches=SavedTrenches;Fleet->Vehicles=SavedVehicles;
        B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=BoundMission;
        B->Phase=Phase;B->BattleOutroWinner=Winner;B->Elapsed=Clock;B->ShotsFired=Shots;B->GrenadesThrown=Thrown;B->RecruitCommands=Commands;
        B->Strikes=Strikes;B->Tracers=Tracers;B->SmokeZones=Smoke;B->Random=Random;B->LastMessage=Message;
        B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->NextSquadId=NextSquad;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
        Fleet->LastEncounter=Encounter;Fleet->NextSerial=Serial;Fleet->NextEnemyAt=EnemyAt;LegacyNo44V836=Legacy;
        for(int32 T=0;T<2;++T){Fleet->DeploymentCountV37[T]=Deployment[T];Fleet->ArmourCycleSecondsV40[T]=Cycle[T];Fleet->RailReadyAtV75[T]=RailReady[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];B->Casualties[T]=Casualties[T];
            for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Fleet->ReadyAt[T][I]=Ready[T][I];for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)B->RecruitReadyAt[T][I]=Recruit[T][I];}
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        Report->SetArrayField(TEXT("cases"),Rows);Report->SetBoolField(TEXT("success"),Good);
        Report->SetStringField(TEXT("scope"),TEXT("Bounded actual UK/DE vehicle Simulate, mounted sight, public recruitment readiness, live hull sweeps, temporary two-slot rail allocation and paid British rifle-grenade combat on the one prepared authored terrain. No full campaign, all-machine performance or aesthetic acceptance claim. No.23 control changes only the strict encounter-local No.44 issue flag; infantry, tank and terrain rules otherwise match. Range checks retain physical world rays and supported weapon readiness."));
    };
    const auto Campaign=[&](EFrontlineFaction Nation,int32 Mission)
    {
        B->PlayerFaction=Nation;FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=Nation;R.CompletedMissions=19;R.SelectedMission=Mission;
        R.WonOperationMaskV52=(1u<<19)-1;R.DispatchResolvedMaskV52=(1u<<20)-1;R.RoleSelectionMask=(1u<<int32(EFrontlineRole::Count))-1;
        R.ArmourResearchMaskV63=65535;
        B->CampaignRuns={R};B->ActiveCampaignId=B->BoundCampaignId=R.Id;B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=Mission;
    };
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const auto Make=[&](int32 Team,EFrontlineRole Role,int32 Member,int32 Id,const FVector& P,int32 Seed)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;
        U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88));
        U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0.f:180.f,0));U->MotionRandom.Initialize(Seed);U->ReturnToIdle();
        U->SetMotion(U->GetAimMotion());U->ShotClock=U->DepartureClock=U->AimSettleClock=0;B->Units.Add(U);Pose(U,.6f);return U;
    };
    const auto Place=[&](AFrontlineTank* V,const FVector& P)
    {if(V){V->SetActorTickEnabled(false);V->bEntered=true;V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->FitGroundV35(P,0);}};
    // A real public recruit sets the common queue to the dispatched model's
    // duration. Card masks sharing that queue are not evidence of identical CDs.
    for(int32 Nation=0;Nation<2;++Nation)
    {
        Clear();Campaign(Nation?EFrontlineFaction::German:EFrontlineFaction::British,19);
        const auto Roster=Fleet->GetRoster(0);TArray<float> Durations;
        for(const auto& D:Roster)
        {
            Clear();B->CampaignRuns[0].ArmourSelectionMaskV63=1u<<int32(D.Type);
            const bool Recruited=Fleet->Recruit(D.Type,0);const float Seconds=Fleet->ReadyAt[0][0]-B->Elapsed;
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_model_recruit"));J->SetNumberField(TEXT("army"),Nation);J->SetNumberField(TEXT("model"),int32(D.Type));J->SetBoolField(TEXT("recruited"),Recruited);J->SetNumberField(TEXT("queue_seconds"),Seconds);Rows.Add(MakeShared<FJsonValueObject>(J));
            const float Expected=FrontlineGameplayV43::PreparationSeconds(D.Cooldown*FrontlinePacingV24::TempoIntervalScale*FrontlineArmourV63::ReadinessScale(B,D.Type,0),0,B->GetCampaignDifficulty());
            Assert(FString::Printf(TEXT("v836_army%d_model%d_actual_shared_queue_uses_model_duration"),Nation,int32(D.Type)),Recruited&&FMath::IsNearlyEqual(Seconds,Expected,.001f));if(Recruited)Durations.Add(Seconds);
        }
        float Shortest=MAX_flt,Longest=0;for(float Seconds:Durations){Shortest=FMath::Min(Shortest,Seconds);Longest=FMath::Max(Longest,Seconds);}
        Assert(FString::Printf(TEXT("v836_army%d_live_recruit_cycles_are_not_identical"),Nation),Durations.Num()>=2&&Longest>Shortest+.1f);
    }
    for(int32 Nation=0;Nation<2;++Nation)
    {
        Clear();Campaign(Nation?EFrontlineFaction::German:EFrontlineFaction::British,19);Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();Fleet->NextEnemyAt=MAX_flt;B->Elapsed=0;
        const auto Roster=Fleet->GetRoster(0);int32 Fast=INDEX_NONE,Slow=INDEX_NONE;float First=MAX_flt,Last=0;
        for(int32 I=0;I<Roster.Num();++I){const float At=Fleet->ReadyAt[0][int32(Roster[I].Type)];if(At<First){First=At;Fast=I;}if(At>Last){Last=At;Slow=I;}}
        bool BeforeBlocked=false,Typed=false,Dispatched=false,Shared=false;
        if(Fast!=INDEX_NONE&&Slow!=INDEX_NONE)
        {
            const auto F=Roster[Fast].Type,S=Roster[Slow].Type;B->CampaignRuns[0].ArmourSelectionMaskV63=(1u<<int32(F))|(1u<<int32(S));
            B->Elapsed=FMath::Max(0.f,First-.05f);BeforeBlocked=!Fleet->Recruit(F,0);
            B->Elapsed=First+.01f;Typed=Fleet->RecruitBlockReason(F,0).IsEmpty()&&!Fleet->RecruitBlockReason(S,0).IsEmpty();Dispatched=Fleet->Recruit(F,0);
            Shared=Dispatched&&!Fleet->Recruit(S,0);for(float At:Fleet->ReadyAt[0])Shared&=FMath::IsNearlyEqual(At,Fleet->ReadyAt[0][0],.001f);
        }
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_first_allocation_readiness"));J->SetNumberField(TEXT("army"),Nation);J->SetNumberField(TEXT("first_model_ready_seconds"),First);J->SetNumberField(TEXT("last_model_ready_seconds"),Last);J->SetBoolField(TEXT("typed_first_ready_gate"),Typed);J->SetBoolField(TEXT("single_dispatch_then_shared_queue"),Shared);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_army%d_initial_model_preparation_has_real_different_public_gate"),Nation),BeforeBlocked&&Typed&&Last>First+.1f);
        Assert(FString::Printf(TEXT("v836_army%d_first_dispatch_closes_all_other_vehicle_cards_atomically"),Nation),Dispatched&&Shared&&Fleet->Vehicles.Num()==1);
    }
    // Austin's mounted Vickers must hold a genuine >32 m exposed contact, even
    // when a separate heavy hull is visible. Discovery never grants wall sight.
    for(int32 Held=0;Held<2;++Held)
    {
        Clear();Campaign(EFrontlineFaction::British,19);const int32 Dir=B->GetTeamDirection(0),Home=B->GetHomeTrenchIndex(0);
        const float Base=B->Trenches[Home].X;
        auto* Car=Fleet->Spawn(EFrontlineVehicle::Austin1918,0,true);auto* Heavy=Fleet->Spawn(EFrontlineVehicle::A7V,1,true);
        Place(Car,FVector(Base+Dir*600,300,0));Place(Heavy,FVector(Base+Dir*4100,-900,0));
        AWarfareFrontlineSoldier* Foe=nullptr;FVector Point;bool Mounted=false;float Range=0;
        for(float Lane:{300.f,0.f,700.f,-400.f})
        {
            if(Foe){B->Units.Remove(Foe);Foe->Destroy();}
            const int32 EnemyHome=B->GetHomeTrenchIndex(1);
            const float X=Held?B->Trenches[EnemyHome].X:Base+Dir*4200;
            Foe=Make(1,EFrontlineRole::Rifle,0,836100+Held,FVector(X,Lane,0),8361);
            Range=Car&&Foe?FVector::Dist2D(Car->GetActorLocation(),Foe->Feet()):0;
            if(Car&&Foe)for(int32 I=0;I<Car->GunMounts.Num()&&!Mounted;++I)Mounted=Car->InfantryFiringSolutionV828(Foe,I,false,Point);
            if(Mounted)break;
        }
        const FVector Start=Car?Car->GetActorLocation():FVector::ZeroVector;
        for(int32 F=0;F<240&&Car&&Foe&&Car->MachinegunShots==0;++F){B->Elapsed+=.05f;Car->Simulate(.05f);Pose(Foe,.05f);}
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("austin_mounted_infantry"));J->SetBoolField(TEXT("held_trench"),Held!=0);J->SetNumberField(TEXT("initial_range_cm"),Range);J->SetBoolField(TEXT("initial_real_mount_clear"),Mounted);J->SetNumberField(TEXT("rounds"),Car?Car->MachinegunShots:0);J->SetNumberField(TEXT("actual_travel_cm"),Car?FVector::Dist2D(Start,Car->GetActorLocation()):0);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_austin_context%d_fires_at_a_real_mounted_infantry_contact"),Held),Car&&Foe&&Car->MachinegunShots>0);
        if(!Held)Assert(TEXT("v836_austin_visible_heavy_does_not_force_close_approach_to_clear_infantry"),Mounted&&Range>3200&&FVector::Dist2D(Start,Car->GetActorLocation())<300);
    }
    // A German supported rifle must acquire and discharge beyond the actual
    // opposing armour MG envelope. Search only grounded authored locations;
    // a clear observation never substitutes for a blocked deployed muzzle.
    for(int32 GermanTeam=0;GermanTeam<2;++GermanTeam)
    {
        Clear();Campaign(GermanTeam?EFrontlineFaction::British:EFrontlineFaction::German,19);
        const int32 Dir=B->GetTeamDirection(GermanTeam);
        auto* Tank=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,1-GermanTeam,true);
        AWarfareFrontlineSoldier* Gunner=nullptr;bool Acquire=false,MuzzleClear=false,Supported=false,Mounted=false;float Distance=0;int32 Locations=0;
        const float MGRange=MountedInfantryRangeV836(B,EFrontlineVehicle::MarkIVMale,false);
        // Visibility scales the company rifle range. Use the genuine interval
        // between the MG and supported-rifle stopping distance when available.
        const float Gap=FMath::Min(5500.f,5600.f*B->EncounterVisibilityScale()-100.f);
        for(float Centre:{-1800.f,0.f,1800.f})
        {
            for(float Lane:{-900.f,900.f,-300.f,300.f})
            {
                ++Locations;
                if(Gunner){B->Units.Remove(Gunner);Gunner->Destroy();}
                Place(Tank,FVector(Centre+Dir*Gap*.5f,Lane,0));
                Gunner=Make(GermanTeam,EFrontlineRole::AntiTankRifle,0,836500+GermanTeam,FVector(Centre-Dir*Gap*.5f,Lane,0),836500+Locations);
                if(auto* Gun=FrontlineMachinegunsV30::Get(Gunner))
                    for(int32 F=0;F<100&&!Gun->IsSupported();++F){Gun->Update(.05f,false);Pose(Gunner,.05f);}
                auto* Gun=FrontlineMachinegunsV30::Get(Gunner);
                Supported=Gun&&Gun->IsSupported()&&Gun->CanFire();
                Acquire=Gunner&&Fleet->AcquireInfantryTarget(Gunner)==Tank;
                MuzzleClear=Acquire&&Supported&&Fleet->HasInfantryFiringLineV828(Gunner,Tank);
                Distance=Gunner&&Tank?FVector::Dist2D(Gunner->Feet(),Tank->AimPoint()):0;
                Mounted=false;FVector Aim;
                if(Gunner&&Tank)for(int32 I=0;I<Tank->GunMounts.Num()&&!Mounted;++I)Mounted=Tank->InfantryFiringSolutionV828(Gunner,I,false,Aim);
                if(MuzzleClear&&Distance>MGRange&& !Mounted)break;
            }
            if(MuzzleClear&&Distance>MGRange&&!Mounted)break;
        }
        const float Begin=B->Elapsed;
        for(int32 F=0;F<180&&Gunner&&MuzzleClear&&FrontlineSpecialistsV41::ReadArmourRoundsV833(B).Fired==0;++F)
        {B->Elapsed+=.05f;Gunner->Tick(.05f);Pose(Gunner,.05f);FrontlineSpecialistsV41::Tick(B);}
        const auto Rounds=FrontlineSpecialistsV41::ReadArmourRoundsV833(B);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_tankgewehr_range_advantage"));J->SetNumberField(TEXT("german_team"),GermanTeam);J->SetNumberField(TEXT("grounded_locations_examined"),Locations);
        J->SetNumberField(TEXT("actual_distance_cm"),Distance);J->SetNumberField(TEXT("actual_opposing_mg_range_cm"),MGRange);J->SetNumberField(TEXT("visibility_scale"),B->EncounterVisibilityScale());J->SetNumberField(TEXT("actual_profile_range_cm"),Gunner?FrontlineWeapons::ProfileFor(Gunner).FireRangeCm:0);
        J->SetBoolField(TEXT("real_observation_acquired"),Acquire);J->SetBoolField(TEXT("supported_muzzle_clear"),MuzzleClear);J->SetBoolField(TEXT("tank_mg_firing_solution"),Mounted);J->SetNumberField(TEXT("actual_ap_shots"),Rounds.Fired);J->SetNumberField(TEXT("physical_hull_hits"),Rounds.PhysicalHullHits);J->SetNumberField(TEXT("seconds"),B->Elapsed-Begin);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_german_team%d_supported_atr_real_clear_shot_exceeds_armour_mg_envelope"),GermanTeam),Tank&&Gunner&&Supported&&Acquire&&MuzzleClear&&Distance>MGRange&&!Mounted&&Rounds.Fired>0);
        Assert(FString::Printf(TEXT("v836_german_team%d_actual_atr_profile_is_70m_and_cannon_range_unchanged"),GermanTeam),Gunner&&FrontlineWeapons::ProfileFor(Gunner).FireRangeCm==7000.f
            &&FMath::IsNearlyEqual(MGRange,FMath::Min(5100.f,UFrontlineVehicleSubsystem::Definition(Tank->Type).RangeCm)*B->EncounterVisibilityScale()*.9f,.01f)
            &&FMath::IsNearlyEqual(MountedInfantryRangeV836(B,Tank->Type,true),FMath::Min(6500.f,UFrontlineVehicleSubsystem::Definition(Tank->Type).RangeCm*B->EncounterVisibilityScale()),.01f));
    }
    // Empty approaches contain real banks, rather than a synthetic flat floor.
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();Campaign(EFrontlineFaction::British,17);const int32 Dir=B->GetTeamDirection(Team),Centre=B->Trenches.Num()/2;const float X=B->Trenches[Centre].X;
        auto* V=Fleet->Spawn(Team?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,Team,true);Place(V,FVector(X-Dir*1200,300,0));
        const FVector Start=V?V->GetActorLocation():FVector::ZeroVector;float Travel=0;int32 Still=0;bool Finite=true;
        for(int32 F=0;F<300&&V;++F){const FVector Before=V->GetActorLocation();B->Elapsed+=.05f;V->Simulate(.05f);const float Moved=FVector::Dist2D(Before,V->GetActorLocation());Travel+=Moved;Still+=Moved<.01f;Finite&=!V->GetActorTransform().ContainsNaN();}
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("healthy_empty_trench_crossing"));J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("travel_cm"),Travel);J->SetNumberField(TEXT("progress_cm"),V?(V->GetActorLocation().X-Start.X)*Dir:0);J->SetNumberField(TEXT("stationary_frames"),Still);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_team%d_healthy_hull_crosses_empty_authored_bank"),Team),V&&Finite&&V->IsOperational()&&!V->IsImmobilized()&&Travel>1500);
    }
    // Real mutual acquisition, fitting and mount traverse; no forced fire.
    for(int32 FirstTeam=0;FirstTeam<2;++FirstTeam)for(int32 Passed=0;Passed<2;++Passed)
    {
        Clear();Campaign(EFrontlineFaction::British,17);const int32 Dir=B->GetTeamDirection(FirstTeam);
        auto* A=Fleet->Spawn(EFrontlineVehicle::A7V,FirstTeam,true);auto* M=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,1-FirstTeam,true);
        const float X=Passed?700.f:-1200.f,Y=Passed?700.f:300.f;
        Place(A,FVector(X*Dir,-Y,0));Place(M,FVector(-X*Dir,Y,0));
        float FirstA=-1,FirstM=-1;bool ClearHulls=true,Crossed=false;
        for(int32 F=0;F<180&&A&&M;++F)
        {
            B->Elapsed+=.05f;A->Simulate(.05f);M->Simulate(.05f);
            if(FirstA<0&&A->CannonShots)FirstA=(F+1)*.05f;if(FirstM<0&&M->CannonShots)FirstM=(F+1)*.05f;
            ClearHulls&=A->CanOccupyV48(A->GetActorLocation(),A->HeadingYawV37)&&M->CanOccupyV48(M->GetActorLocation(),M->HeadingYawV37);
            if(!Passed)Crossed|=(M->GetActorLocation().X-A->GetActorLocation().X)*Dir<0;
        }
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_opposing_armour"));J->SetNumberField(TEXT("a7v_team"),FirstTeam);J->SetBoolField(TEXT("already_passed"),Passed!=0);
        J->SetNumberField(TEXT("a7v_first_cannon_seconds"),FirstA);J->SetNumberField(TEXT("mark_iv_first_cannon_seconds"),FirstM);J->SetBoolField(TEXT("live_hull_clear"),ClearHulls);J->SetBoolField(TEXT("uncontested_crossing"),Crossed);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_armour_team%d_passed%d_both_real_mounts_fire_without_overlap"),FirstTeam,Passed),A&&M&&FirstA>=0&&FirstM>=0&&ClearHulls&&!Crossed);
    }
    // Equal physical impacts distinguish support power from hull absorption.
    float DamageAT=0,DamageOrdinary=0,DamageVehicle=0;
    for(int32 Mode=0;Mode<3;++Mode)
    {
        Clear();Campaign(EFrontlineFaction::British,17);auto* V=Fleet->Spawn(EFrontlineVehicle::A7V,1,true);Place(V,FVector(0,300,0));
        const float Before=V?V->GetHullIntegrity():0;
        const auto Credit=Mode==2?FFrontlineKillSourceV65::Vehicle(V):FFrontlineKillSourceV65::FireSupport(0,Mode==0?int32(EFrontlineSupport::AntiTank):int32(EFrontlineSupport::Mortar));
        if(V)Fleet->ApplyBlast(V->GetActorTransform().TransformPosition(FVector(0,0,160)),500.f,260.f,0,Credit);
        const float Loss=V?Before-V->GetHullIntegrity():0;auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_support_hull_application"));J->SetNumberField(TEXT("mode"),Mode);J->SetNumberField(TEXT("hull_loss"),Loss);J->SetBoolField(TEXT("still_operational"),V&&V->IsOperational());Rows.Add(MakeShared<FJsonValueObject>(J));
        if(Mode==0){DamageAT=Loss;Assert(TEXT("v836_260_at_contact_cannot_single_hit_destroy_heavy_hull"),V&&V->IsOperational()&&Loss>0&&Loss<Before);}
        else if(Mode==1)DamageOrdinary=Loss;else DamageVehicle=Loss;
    }
    Assert(TEXT("v836_ordinary_support_hull_energy_is_eighty_percent_without_weakening_at_or_vehicle_shell"),DamageAT>0&&FMath::IsNearlyEqual(DamageOrdinary,DamageVehicle*.80f,.02f)&&FMath::IsNearlyEqual(DamageAT,DamageVehicle,.02f));
    for(int32 Nation=0;Nation<2;++Nation)
    {
        Clear();Campaign(Nation?EFrontlineFaction::German:EFrontlineFaction::British,17);const int32 Team=Nation?0:1;
        B->Phase=EFrontlinePhase::Briefing;
        const bool Eligible=Fleet->RailAvailableV75(Team);
        const auto Roster=Fleet->GetRoster(Team);
        bool Selected=true;
        if(Team==0)
        {
            const auto Original=Fleet->GetEquippedRoster(Team);
            if(Original.Num()==2)Selected=FrontlineArmourV63::ToggleSelection(B,Original.Last().Type);
            Selected&=FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::RailConvoy);
        }
        const auto Allocated=Fleet->GetEquippedRoster(Team);
        const bool TwoSlots=Allocated.Num()==2&&Allocated.ContainsByPredicate([](const auto& D){return D.Type==EFrontlineVehicle::RailConvoy;});
        const bool Introduction=Fleet->UnlockMission(EFrontlineVehicle::RailConvoy,Team)==B->GetSelectedCampaignMission()->Id
            &&!FString(FrontlineArmourV63::VehicleDescriptionV8218(EFrontlineVehicle::RailConvoy)).IsEmpty();
        Assert(FString::Printf(TEXT("v836_rail_team%d_temporary_roster_and_normal_two_slot_allocation"),Team),Eligible&&Roster.Num()==3&&Selected&&TwoSlots&&Introduction);
        Assert(FString::Printf(TEXT("v836_rail_team%d_normal_portrait_asset_present"),Team),Fleet->GetPortrait(EFrontlineVehicle::RailConvoy)!=nullptr);
        B->Phase=EFrontlinePhase::Fighting;
        const FString RequestReason=Fleet->RecruitBlockReason(EFrontlineVehicle::RailConvoy,Team);
        bool Requested=false;
        if(Team==1){Fleet->NextEnemyAt=B->Elapsed;Fleet->Tick(.05f);Requested=Fleet->Vehicles.ContainsByPredicate([](const TObjectPtr<AFrontlineTank>& V){return V&&V->Type==EFrontlineVehicle::RailConvoy&&V->IsRailConvoyV32();});}
        else Requested=Fleet->Recruit(EFrontlineVehicle::RailConvoy,Team);
        AFrontlineTank* Train=nullptr;for(AFrontlineTank* V:Fleet->Vehicles)if(V&&V->IsRailConvoyV32())Train=V;
        bool Shared=Requested&&Fleet->DeploymentCountV37[Team]==1;
        for(float At:Fleet->ReadyAt[Team])Shared&=FMath::IsNearlyEqual(At,Fleet->ReadyAt[Team][0],.001f)&&At>B->Elapsed;
        for(const auto& D:Allocated)if(D.Type!=EFrontlineVehicle::RailConvoy)Shared&=!Fleet->Recruit(D.Type,Team);
        Assert(FString::Printf(TEXT("v836_rail_team%d_dispatch_uses_same_shared_armour_queue"),Team),Shared);
        const FVector Start=Train?Train->GetActorLocation():FVector::ZeroVector;
        for(int32 F=0;F<100&&Train;++F){B->Elapsed+=.05f;Train->Simulate(.05f);}
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("actual_german_rail_normal_recruitment"));J->SetNumberField(TEXT("campaign_army"),Nation);J->SetNumberField(TEXT("team"),Team);J->SetBoolField(TEXT("eligible"),Eligible);J->SetNumberField(TEXT("allocation_slots"),Allocated.Num());J->SetBoolField(TEXT("shared_queue"),Shared);J->SetStringField(TEXT("initial_public_block_reason"),RequestReason);J->SetBoolField(TEXT("request_accepted"),Requested);J->SetBoolField(TEXT("entered"),Train&&Train->HasEnteredField());J->SetNumberField(TEXT("travel_cm"),Train?FVector::Dist2D(Start,Train->GetActorLocation()):0);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_german_rail_%d_normal_recruit_enters_real_end_route"),Team),Eligible&&Requested&&Train&&Train->Type==EFrontlineVehicle::RailConvoy&&Train->HasEnteredField()&&FVector::Dist2D(Start,Train->GetActorLocation())>800);
        Assert(FString::Printf(TEXT("v836_rail_%d_common_long_cd_cannot_duplicate_live_train"),Team),!Fleet->Recruit(EFrontlineVehicle::RailConvoy,Team));
        FVector Shoulder=FVector::ZeroVector;AWarfareFrontlineSoldier* Friend=nullptr;bool Yield=false;float FriendTravel=0;bool BodiesClear=true;
        if(Train&&Train->Body&&Train->Body->GetStaticMesh())
        {
            const auto Bounds=Train->Body->GetStaticMesh()->GetBoundingBox();
            const FVector Approach=Train->Body->GetComponentTransform().TransformPosition(FVector(Bounds.Max.X+100.f,0,0));
            Friend=Make(Team,EFrontlineRole::Rifle,0,836170+Nation,Approach,836170);
            Yield=Train->InfantryYieldPointV8218(Friend,Shoulder);
            for(int32 F=0;F<100&&Friend&&Yield;++F)
            {
                const FVector Before=Friend->Feet(),Next=FrontlineMobilityV41::Step(Friend,Shoulder,8.f,false);
                FriendTravel+=FVector::Dist2D(Before,Next);Friend->SetActorLocation(Next+FVector(0,0,88));
                BodiesClear&=!Train->BlocksInfantryAtV8218(Friend->Feet(),Train->GetActorLocation(),Train->HeadingYawV37,34.f);
            }
        }
        auto Y=MakeShared<FJsonObject>();Y->SetStringField(TEXT("kind"),TEXT("rail_live_friendly_shoulder"));Y->SetNumberField(TEXT("team"),Team);Y->SetBoolField(TEXT("real_shoulder_exists"),Yield);Y->SetNumberField(TEXT("actual_body_travel_cm"),FriendTravel);Y->SetBoolField(TEXT("body_hull_clear"),BodiesClear);Rows.Add(MakeShared<FJsonValueObject>(Y));
        Assert(FString::Printf(TEXT("v836_rail_team%d_friendly_body_has_actual_in_domain_shoulder_without_hull_bypass"),Team),Yield&&FriendTravel>100&&BodiesClear&&FMath::Abs(Shoulder.Y)<FrontlineBattlefield::HalfFrontage-35.f);
        // Eligibility is the selected operation, not a permanently researched
        // chassis. A retained choice bit must not make the next nonrail battle legal.
        B->Phase=EFrontlinePhase::Briefing;B->CampaignRuns[0].SelectedMission=19;
        const auto NextRoster=Fleet->GetRoster(Team);
        Assert(FString::Printf(TEXT("v836_rail_team%d_allocation_expires_on_nonrail_operation_even_with_retained_choice"),Team),
            !Fleet->RailAvailableV75(Team)&&!Fleet->IsUnlocked(EFrontlineVehicle::RailConvoy,Team)
            &&!NextRoster.ContainsByPredicate([](const auto& D){return D.Type==EFrontlineVehicle::RailConvoy;})
            &&Fleet->UnlockMission(EFrontlineVehicle::RailConvoy,Team)==INDEX_NONE);
    }
    int32 RealNo44Shots=0,RealNo44HitCases=0,ActualInfantryKills=0;
    for(int32 LegacyMode=0;LegacyMode<2;++LegacyMode)for(int32 Model=0;Model<2;++Model)for(int32 Seed=0;Seed<2;++Seed)
    {
        Clear();Campaign(EFrontlineFaction::British,17);LegacyNo44V836=LegacyMode!=0;
        // The normal five-slot selection retains the first five supplied bits.
        // Equip the two actual roles under test, rather than a truncated all-role mask.
        B->CampaignRuns[0].RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::RifleGrenadier));
        const int32 Home=B->GetHomeTrenchIndex(0),Dir=B->GetTeamDirection(0);const float X=B->Trenches[Home].X;
        TArray<AWarfareFrontlineSoldier*> Men;TArray<TSharedPtr<FJsonValue>> RecruitReasons;float Paid=0;bool Recruited=true;
        for(int32 Section=0;Section<3;++Section)
        {
            const auto Role=Section==0?EFrontlineRole::Rifle:EFrontlineRole::RifleGrenadier;const int32 Before=B->Squads.Num();
            RecruitReasons.Add(MakeShared<FJsonValueString>(B->RecruitBlockReason(Role,0)));
            Recruited&=B->Recruit(Role,0)&&B->Squads.Num()==Before+1;
            if(B->Squads.Num()==Before)continue;
            auto& Squad=B->Squads.Last();Squad.bReinforcement=Squad.bAdvancing=false;Squad.Trench=Home;Squad.Destination=INDEX_NONE;Squad.Slot=Section;
            int32 M=0;for(const auto& W:Squad.Members)if(auto* U=W.Get())
            {
                const FVector P=Section==0?FVector(X+Dir*700.f,(M-2.5f)*160.f,0):FVector(X,(Section==1?-1:1)*420.f+(M-1)*105.f,0);
                U->SetActorTickEnabled(false);U->bFieldEntered=true;U->MotionRandom.Initialize(83600+Seed*197+Section*37+M*11);
                U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88));U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));U->ReturnToIdle();U->SetMotion(U->GetAimMotion());U->DepartureClock=U->AimSettleClock=0;Pose(U,.6f);Men.Add(U);++M;
            }
            if(Section<2){const auto NextRole=EFrontlineRole::RifleGrenadier;const float Wait=FMath::Max(0.f,B->RecruitReadyAt[0][int32(NextRole)]-B->Elapsed);Paid+=Wait;B->Elapsed+=Wait;}
        }
        auto* Tank=Fleet->Spawn(Model?EFrontlineVehicle::BeuteMarkIVFemale:EFrontlineVehicle::A7V,1,true);Place(Tank,FVector(X+Dir*2300.f,0,0));
        if(Tank){Tank->Random.Initialize(836100+Seed*131);Tank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,Tank->GetTrackIntegrity(0)+1);}
        const float Begin=B->Elapsed,Hull=Tank?Tank->GetHullIntegrity():0;int32 No44=0,Released=0;float FirstFire=-1;bool ClearBodies=true;
        for(int32 F=0;F<800&&Tank&&Tank->IsOperational();++F)
        {
            B->Elapsed+=.05f;Tank->Simulate(.05f);
            for(auto* U:Men){U->Tick(.05f);Pose(U,.05f);if(U->IsAlive())ClearBodies&=!Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation(),Tank->GetActorRotation().Yaw,48.f);}
            for(const auto& Strike:B->Strikes)if(Strike.bGrenade&&!Strike.bResolved&&Strike.ImpactAt<=B->Elapsed){++Released;No44+=Strike.bAntiTankRifleGrenadeV836;}
            B->TickCommittedSupportV8217();FrontlineSpecialistsV41::Tick(B);if(FirstFire<0&&B->GrenadesThrown>0)FirstFire=B->Elapsed-Begin;
        }
        const auto Proven=FrontlineSpecialistsV41::ReadArmourRoundsV833(B);int32 Alive=0;for(auto* U:Men)Alive+=U->IsAlive();
        const bool InfantryKill=Tank&&!Tank->IsOperational()&&B->CombatRecordV65[0].InfantrySources[int32(EFrontlineRole::RifleGrenadier)].Armour>0;
        if(!LegacyMode){RealNo44Shots+=No44;RealNo44HitCases+=Proven.GrenadeHullDamage>0;ActualInfantryKills+=InfantryKill;}
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("paid_uk_rifle_grenades_vs_armour"));J->SetStringField(TEXT("mode"),LegacyMode?TEXT("no23_control"):TEXT("no44_1918"));J->SetNumberField(TEXT("model"),Model);J->SetNumberField(TEXT("seed"),Seed);J->SetBoolField(TEXT("actual_recruits"),Recruited);J->SetNumberField(TEXT("paid_queue_seconds_before_third_section"),Paid);
        J->SetArrayField(TEXT("actual_recruit_block_reasons"),RecruitReasons);
        J->SetNumberField(TEXT("seconds"),B->Elapsed-Begin);J->SetNumberField(TEXT("initial_hull"),Hull);J->SetNumberField(TEXT("net_hull_damage"),Tank?Hull-Tank->GetHullIntegrity():0);J->SetNumberField(TEXT("actual_infantry_grenade_hull_damage"),Proven.GrenadeHullDamage);J->SetNumberField(TEXT("actual_infantry_grenade_track_damage"),Proven.GrenadeTrackDamage);
        J->SetNumberField(TEXT("released_grenades"),Released);J->SetNumberField(TEXT("no44_rounds_resolved"),No44);J->SetNumberField(TEXT("first_fire_seconds"),FirstFire);J->SetNumberField(TEXT("infantry_survivors"),Alive);J->SetBoolField(TEXT("infantry_attributed_armour_kill"),InfantryKill);J->SetBoolField(TEXT("actual_body_contact_clear"),ClearBodies);Rows.Add(MakeShared<FJsonValueObject>(J));
        Assert(FString::Printf(TEXT("v836_uk_no44_mode%d_model%d_seed%d_paid_real_sections_and_body_clear"),LegacyMode,Model,Seed),Recruited&&Paid>0&&ClearBodies);
        Assert(FString::Printf(TEXT("v836_uk_no44_mode%d_model%d_seed%d_no_misattributed_net_damage"),LegacyMode,Model,Seed),Tank&&Proven.GrenadeHullDamage<=Hull-Tank->GetHullIntegrity()+.01f);
    }
    Assert(TEXT("v836_no44_actual_releases_exist"),RealNo44Shots>=4);Assert(TEXT("v836_no44_hull_damage_has_actual_infantry_provenance"),RealNo44HitCases>=2);
    Assert(TEXT("v836_paid_british_no44_sections_can_earn_actual_infantry_armour_kill"),ActualInfantryKills>0);
    Report->SetNumberField(TEXT("no44_actual_resolved_rounds"),RealNo44Shots);Report->SetNumberField(TEXT("no44_actual_hull_damage_cases"),RealNo44HitCases);Report->SetNumberField(TEXT("no44_infantry_attributed_armour_kills"),ActualInfantryKills);
    Clear();Campaign(EFrontlineFaction::British,17);auto* Wreck=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,0,true);Place(Wreck,FVector(0,300,0));
    if(Wreck){Wreck->ReceiveModuleHit(EFrontlineTankModule::Hull,Wreck->GetMaximumHullIntegrity()+1);for(int32 I=0;I<1600;++I){B->Elapsed+=.1f;Wreck->Simulate(.1f);}}
    Assert(TEXT("v836_destroyed_hull_survives_fire_and_150_second_cleanup_timers"),IsValid(Wreck)&&!Wreck->IsOperational()&&!Wreck->GetActorEnableCollision());
    return Report;
}

// Explicitly retained only for the root's disposable real-window captures.
static bool PrepareVisual(AWarfareFrontlineGameMode* B,bool German)
{
    FString User;
    if(!B||!B->GetWorld()||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||User.IsEmpty()||FPaths::IsRelative(User))return false;
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Fleet)return false;
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->Tracers.Reset();B->SmokeZones.Reset();Fleet->ClearEncounter();
    B->PlayerFaction=German?EFrontlineFaction::German:EFrontlineFaction::British;
    FFrontlineCampaignProgress Progress;Progress.Id=FGuid::NewGuid();Progress.Faction=B->PlayerFaction;Progress.CompletedMissions=19;Progress.SelectedMission=17;
    Progress.WonOperationMaskV52=(1u<<19)-1;
    if(German)Progress.ArmourSelectionMaskV63=(1u<<int32(EFrontlineVehicle::A7V))|(1u<<int32(EFrontlineVehicle::RailConvoy));
    B->CampaignRuns={Progress};B->ActiveCampaignId=B->BoundCampaignId=Progress.Id;
    B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=17;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    B->Elapsed=1000;Fleet->LastEncounter=B->GetEncounterId();Fleet->NextEnemyAt=MAX_flt;
    const auto PlaceVisual=[&](AFrontlineTank* V,const FVector& P)
    {if(V){V->bEntered=true;V->SetActorTickEnabled(false);V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->FitGroundV35(P,0);}};
    auto* Main=Fleet->Spawn(German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkVStar,0,true);
    auto* Secondary=Fleet->Spawn(German?EFrontlineVehicle::RailConvoy:EFrontlineVehicle::Austin1918,0,true);
    auto* Broken=Fleet->Spawn(German?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::Whippet,0,true);
    PlaceVisual(Main,FVector(2000,450,0));PlaceVisual(Secondary,FVector(German?-4000.f:-900.f,German?-1130.f:-600.f,0));PlaceVisual(Broken,FVector(-1800,900,0));
    // Captures retain only these vehicles; public rail allocation and normal
    // recruitment are tested above. No second AI/legacy rail request is issued.
    Fleet->RailReadyAtV75[0]=Fleet->RailReadyAtV75[1]=MAX_flt;
    const bool ServiceAssets=Main&&Secondary&&Broken&&Main->AreAssetsReady()&&Secondary->AreAssetsReady()&&Broken->AreAssetsReady();
    if(Broken)Broken->ReceiveModuleHit(EFrontlineTankModule::Hull,Broken->GetMaximumHullIntegrity()+1);
    if(!ServiceAssets)return false;
    // AreAssetsReady validates the operational body. A destroyed vehicle now
    // intentionally replaces that body with its exact damaged derivative; keep
    // the full live check above and validate the actual wreck contract below.
    const auto WreckAssets=FFrontlineArmourWreckV8220Access::Describe(Broken);
    const bool BrokenAssets=WreckAssets->GetBoolField(TEXT("transition"))&&!WreckAssets->GetBoolField(TEXT("operational"))
        &&WreckAssets->GetBoolField(TEXT("body_geometry_replaced"))&&WreckAssets->GetBoolField(TEXT("surface_coverage_complete"))
        &&WreckAssets->GetBoolField(TEXT("body_surface_complete"))&&WreckAssets->GetBoolField(TEXT("persistent_wreck_parameters"))
        &&WreckAssets->GetBoolField(TEXT("legacy_damage_zero"))&&WreckAssets->GetBoolField(TEXT("removed_from_live_material_updates"))
        &&WreckAssets->GetBoolField(TEXT("physical_components_collision_disabled"))&&!WreckAssets->GetBoolField(TEXT("actor_collision"))
        &&WreckAssets->GetBoolField(TEXT("loose_steel_nonblocking_and_shadowed"))&&WreckAssets->GetBoolField(TEXT("track_cache_matches_actual_instances"))
        &&WreckAssets->GetBoolField(TEXT("thrown_chain_has_ground_contact"));
    return Main->AreAssetsReady()&&Secondary->AreAssetsReady()&&BrokenAssets;
}
