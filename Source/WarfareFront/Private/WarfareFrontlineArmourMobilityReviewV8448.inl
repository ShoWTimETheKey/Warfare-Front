// Real production actor trajectories, with strictly isolated old-rule witnesses.
TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunMobilityChecksV8448()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;};
    ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);Report->SetBoolField(TEXT("success"),Good);};
    auto* B=Battle();FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
    FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8448/Runtime"));FPaths::NormalizeDirectoryName(Root);
    const FString Stamp=User.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)?User.Mid(Root.Len()+1):FString();
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8448Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&!User.IsEmpty()&&!FPaths::IsRelative(User)&&!User.Contains(TEXT(".."))&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    Check(TEXT("v8448_mobility_strict_disposable_ukde_profile"),Safe);if(!Safe)return Report;
    const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTrenches=B->Trenches;
    const auto SavedSmoke=B->SmokeZones;const auto SavedTracers=B->Tracers;const auto SavedPhase=B->Phase;
    const auto SavedEncounter=LastEncounter;const float Clock=B->Elapsed,EnemyAt=NextEnemyAt,OutroClock=B->BattleOutroElapsed;
    const int32 SavedSerial=NextSerial,Winner=B->BattleOutroWinner,Advances=B->AdvanceCommands,Count[2]={DeploymentCountV37[0],DeploymentCountV37[1]};
    const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());const FString Message=B->LastMessage;
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AFrontlineTank* V:SavedVehicles)Hide(V);for(AWarfareFrontlineSoldier* U:SavedUnits)Hide(U);
    TArray<AFrontlineTank*> Fixtures;TArray<AWarfareFrontlineSoldier*> People;
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Tracers.Empty();Vehicles.Empty();
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;LastEncounter=B->GetEncounterId();NextEnemyAt=MAX_flt;
    UGameplayStatics::SetGamePaused(B,false);
    const auto Clear=[&](){for(auto* U:People)if(IsValid(U))U->Destroy();People.Empty();B->Units.Empty();B->Squads.Empty();
        for(auto* V:Fixtures)if(IsValid(V))V->Destroy();Fixtures.Empty();Vehicles.Empty();B->Trenches=SavedTrenches;
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0.f;B->Elapsed=Clock;B->LastMessage=Message;
        LastEncounter=B->GetEncounterId();NextEnemyAt=MAX_flt;UGameplayStatics::SetGamePaused(B,false);
        FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);};
    ON_SCOPE_EXIT
    {
        LegacyContactTurnV8448=LegacyFixedLanesV8448=LegacyStandingBodyV8448=false;
        Clear();Vehicles=SavedVehicles;B->Units=SavedUnits;B->Squads=SavedSquads;B->Trenches=SavedTrenches;
        B->SmokeZones=SavedSmoke;B->Tracers=SavedTracers;B->Phase=SavedPhase;B->BattleOutroWinner=Winner;B->BattleOutroElapsed=OutroClock;B->AdvanceCommands=Advances;
        B->Elapsed=Clock;B->LastMessage=Message;LastEncounter=SavedEncounter;NextSerial=SavedSerial;NextEnemyAt=EnemyAt;
        for(int32 T=0;T<2;++T)DeploymentCountV37[T]=Count[T];
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        UGameplayStatics::SetGamePaused(B,Paused);
    };
    const auto NewTank=[&](EFrontlineVehicle Type,int32 Team)
    {
        auto* V=GetWorld()->SpawnActor<AFrontlineTank>();if(!V)return V;
        if(!V->Configure(B,Type,Team,++NextSerial)){V->Destroy();return (AFrontlineTank*)nullptr;}
        Fixtures.Add(V);Vehicles.Add(V);for(float& C:V->CannonClock)C=10000.f;for(float& C:V->GunClock)C=10000.f;return V;
    };
    const auto Place=[&](AFrontlineTank* V,FVector P)
    {
        const int32 Dir=B->GetTeamDirection(V->Team);V->HeadingYawV37=Dir>0?0.f:180.f;
        V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->RouteDecisionAtV37=0;V->RouteStallSecondsV837=0;
        V->ArmourContactV48.Reset();V->ArmourContactAgeV48=2;V->InfantryContactV8448.Reset();V->bEntered=true;V->FitGroundV35(P,0);
    };
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();}};
    const auto NewPerson=[&](int32 Team,FVector P,EFrontlineRole Role=EFrontlineRole::Rifle,int32 Id=8448000,int32 Member=0)
    {
        auto* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
        P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0.f:180.f,0));
        People.Add(U);B->Units.Add(U);Pose(U,0);return U;
    };
    const auto TickPerson=[&](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->IsAlive()){U->Tick(DT);Pose(U,DT);if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const int32 Trench=B->Trenches.Num()/2;Check(TEXT("v8448_authored_trench_present"),B->Trenches.IsValidIndex(Trench));if(!B->Trenches.IsValidIndex(Trench))return Report;
    const auto RunTrajectory=[&](const FString& Name,AFrontlineTank* V,float Bank,float Duration,bool Dynamic,bool Legacy,int32 ReleaseId=INDEX_NONE)
    {
        LegacyContactTurnV8448=LegacyFixedLanesV8448=LegacyStandingBodyV8448=Legacy;
        const FVector Start=V->GetActorLocation();const int32 Dir=B->GetTeamDirection(V->Team);
        float Travel=0,Requested=0,Idle=0,MaxIdle=0,NoProgress=0,MaxNoProgress=0,UnreadyHalt=0,MaxUnreadyHalt=0;bool Finite=true,ClearBody=true,Bounded=true;
        int32 FiringFrames=0,TraverseFrames=0,RouteFrames=0,PackFrames=0,CarryFrames=0,ConsumedSteps=0,ActualSteps=0;bool Released=false;
        float MinHead=MAX_flt,MaxHead=0.f;TArray<TSharedPtr<FJsonValue>> Samples,ContactEvents;
        const auto ContactStage=[&](const TCHAR* Stage,int32 Frame)
        {
            for(auto* U:People)if(U->IsAlive()&&V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37,34.f,0.f,U))
            {
                ClearBody=false;if(ContactEvents.Num()>=24)continue;
                auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("stage"),Stage);E->SetNumberField(TEXT("frame"),Frame);
                E->SetNumberField(TEXT("host_seconds"),B->Elapsed);E->SetNumberField(TEXT("member"),U->MemberIndex);E->SetNumberField(TEXT("squad"),U->SquadId);
                E->SetStringField(TEXT("feet"),U->Feet().ToString());E->SetStringField(TEXT("hull"),V->GetActorTransform().ToString());
                if(U->Soldier)E->SetStringField(TEXT("head"),U->Soldier->GetBoneLocation(TEXT("Head")).ToString());
                ContactEvents.Add(MakeShared<FJsonValueObject>(E));
            }
        };
        const int32 Frames=FMath::CeilToInt(Duration/.05f);
        for(int32 F=0;F<Frames;++F)
        {
            const float DT=F%19==0?.10f:F%13==0?1.f/60.f:.05f;const FVector Before=V->GetActorLocation();
            B->Elapsed+=DT;
            ContactStage(TEXT("before_infantry_tick"),F);
            if(ReleaseId!=INDEX_NONE&&F==15)Released=B->AdvanceSquad(Trench,ReleaseId,V->Team);
            if(Dynamic)for(auto* U:People)
            {
                TickPerson(U,DT);
                if(ReleaseId!=INDEX_NONE&&U->SquadId==ReleaseId)
                {
                    if(auto* G=FrontlineMachinegunsV30::Get(U)){PackFrames+=G->State==EFrontlineGunStanceV30::Pack;CarryFrames+=G->State==EFrontlineGunStanceV30::Carry;}
                    if(U->Soldier){const float Head=float(U->Soldier->GetBoneLocation(TEXT("Head")).Z-U->Feet().Z);MinHead=FMath::Min(MinHead,Head);MaxHead=FMath::Max(MaxHead,Head);}
                }
            }
            ContactStage(TEXT("after_infantry_pose_before_fleet"),F);
            Tick(DT);++ActualSteps;
            ContactStage(TEXT("after_fleet_commit"),F);
            const bool Consumed=FMath::IsNearlyEqual(V->MobilityHostTimeV839,B->Elapsed,.002f)&&V->MobilityStepV839>0.f;
            ConsumedSteps+=Consumed;
            const FVector After=V->GetActorLocation();const float Moved=float(FVector::Dist2D(Before,After));
            Travel+=Moved;Requested+=V->MobilityRequestedV839;
            Idle=Moved<.01f?Idle+DT:0.f;MaxIdle=FMath::Max(MaxIdle,Idle);
            NoProgress=V->MobilityRequestedV839>0.f&&(After.X-Before.X)*Dir<.5f?NoProgress+DT:0.f;MaxNoProgress=FMath::Max(MaxNoProgress,NoProgress);
            const FString Gate=V->MobilityGateV839;
            const bool ContactHalt=Gate==TEXT("feasible_mounted_traverse")||Gate==TEXT("remembered_mounted_contact");
            UnreadyHalt=ContactHalt&&!V->bMobilityFiringReadyV8448&&V->MobilityRequestedV839==0.f&&(After.X-Before.X)*Dir<.5f?UnreadyHalt+DT:0.f;
            MaxUnreadyHalt=FMath::Max(MaxUnreadyHalt,UnreadyHalt);
            FiringFrames+=V->bMobilityFiringReadyV8448;TraverseFrames+=V->bMobilityTraversingV8448;
            RouteFrames+=FString(V->MobilityGateV839)==TEXT("physical_route_blocked");
            Finite&=!V->GetActorTransform().ContainsNaN();
            Bounded&=Moved<=Definition(V->Type).SpeedCm*FrontlineArmourV63::TravelSpeedScaleV8218(B,V->Team)*DT+.02f&&FMath::Abs(After.Y)<=980.1f;
            for(auto* U:People)if(U->IsAlive())ClearBody&=!V->BlocksInfantryAtV8218(U->Feet(),After,V->HeadingYawV37,34.f,0.f,U);
            if(F%4==0||F<4){auto S=V->DescribeMobilityV839();S->SetNumberField(TEXT("relative_seconds"),B->Elapsed-(Clock));S->SetBoolField(TEXT("actual_fleet_step_consumed"),Consumed);
                S->SetNumberField(TEXT("net_forward_cm"),(After.X-Start.X)*Dir);S->SetNumberField(TEXT("total_travel_cm"),Travel);Samples.Add(MakeShared<FJsonValueObject>(S));}
            // Stop after the authored bank has genuinely been passed, before
            // a fast chassis can trigger the campaign's real end-line victory.
            // The next fixture must not inherit its outro or a stale snapshot.
            if((After.X-Bank)*Dir>1500.f)break;
        }
        auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),Name);J->SetBoolField(TEXT("legacy_rule_counterfactual"),Legacy);
        J->SetNumberField(TEXT("net_forward_cm"),(V->GetActorLocation().X-Start.X)*Dir);J->SetNumberField(TEXT("bank_progress_cm"),(V->GetActorLocation().X-Bank)*Dir);
        J->SetNumberField(TEXT("total_travel_cm"),Travel);J->SetNumberField(TEXT("requested_total_cm"),Requested);J->SetNumberField(TEXT("max_idle_seconds"),MaxIdle);
        J->SetNumberField(TEXT("max_requested_without_net_progress_seconds"),MaxNoProgress);J->SetNumberField(TEXT("firing_ready_frames"),FiringFrames);
        J->SetNumberField(TEXT("max_unready_contact_halt_seconds"),MaxUnreadyHalt);J->SetArrayField(TEXT("body_contact_events"),ContactEvents);
        J->SetNumberField(TEXT("traverse_only_frames"),TraverseFrames);J->SetNumberField(TEXT("physical_route_blocked_frames"),RouteFrames);
        J->SetBoolField(TEXT("real_departure_order_accepted"),Released);J->SetNumberField(TEXT("actual_pack_pose_frames"),PackFrames);J->SetNumberField(TEXT("actual_carry_pose_frames"),CarryFrames);
        J->SetNumberField(TEXT("actual_fleet_steps"),ActualSteps);J->SetNumberField(TEXT("consumed_fleet_steps"),ConsumedSteps);
        if(ReleaseId!=INDEX_NONE){J->SetNumberField(TEXT("actual_min_head_height_cm"),MinHead);J->SetNumberField(TEXT("actual_max_head_height_cm"),MaxHead);}
        J->SetBoolField(TEXT("finite"),Finite);J->SetBoolField(TEXT("all_poses_clear_actual_bodies"),ClearBody);J->SetBoolField(TEXT("bounded_travel"),Bounded);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
        Check(Name+TEXT("_every_actual_fleet_step_is_consumed"),ActualSteps>0&&ConsumedSteps==ActualSteps);
        LegacyContactTurnV8448=LegacyFixedLanesV8448=LegacyStandingBodyV8448=false;return J;
    };
    int32 LaneWitnesses=0;
    for(int32 Team=0;Team<2;++Team)for(auto Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::A7V,EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918})
    {
        const int32 Dir=B->GetTeamDirection(Team);const FString Prefix=FString::Printf(TEXT("v8448_t%d_model%d_"),Team,int32(Type));
        float LegacyProgress=0,NewProgress=0;
        for(bool Legacy:{true,false})
        {
            Clear();auto* V=NewTank(Type,Team);Check(Prefix+TEXT("production_assets"),V&&V->AreAssetsReady());if(!V)continue;
            const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,300.f);
            // Four real standing men cover the four old lanes. A middle
            // physical corridor still exists and must be considered early.
            for(float Y:{-900.f,-300.f,300.f,900.f})NewPerson(Team,FVector(Bank+Dir*100.f,Y,0));
            Place(V,FVector(Bank-Dir*1300.f,300.f,0));
            auto J=RunTrajectory(Prefix+(Legacy?TEXT("four_fixed_lanes_old"):TEXT("middle_corridor_new")),V,Bank,28.f,false,Legacy);
            if(Legacy)LegacyProgress=J->GetNumberField(TEXT("bank_progress_cm"));
            else
            {
                NewProgress=J->GetNumberField(TEXT("bank_progress_cm"));
                Check(Prefix+TEXT("middle_corridor_passes_live_bodies_without_stall_or_overlap"),J->GetBoolField(TEXT("finite"))&&J->GetBoolField(TEXT("bounded_travel"))
                    &&J->GetBoolField(TEXT("all_poses_clear_actual_bodies"))&&NewProgress>700.f&&J->GetNumberField(TEXT("max_requested_without_net_progress_seconds"))<3.f);
            }
        }
        LaneWitnesses+=LegacyProgress<500.f&&NewProgress>700.f;
    }
    Report->SetNumberField(TEXT("fixed_lane_deadlock_witnesses"),LaneWitnesses);
    Check(TEXT("v8448_fixed_lane_omission_has_real_old_rule_witness"),LaneWitnesses>0);
    // Natural held MG section: animation and movement are evaluated before
    // each actual fleet tick. Low bodies are never frozen in a fake pose.
    int32 LowBodyWitnesses=0;
    for(int32 Team=0;Team<2;++Team)for(auto Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::Whippet})
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team),Id=8448100+Team*10+int32(Type);B->Trenches[Trench].Owner=Team;
        FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Trench=Q.Destination=Q.LastPassedTrench=Trench;Q.Slot=1;Q.bHoldPosition=true;
        for(int32 M=0;M<3;++M)if(auto* U=NewPerson(Team,FVector(B->Trenches[Trench].X,-500.f+M*160.f,0),EFrontlineRole::Lewis,Id,M))Q.Members.Add(U);
        B->Squads.Add(Q);
        for(int32 F=0;F<260;++F){B->Elapsed+=.05f;for(auto* U:People)TickPerson(U,.05f);}
        auto* G=People.IsEmpty()?nullptr:FrontlineMachinegunsV30::Get(People[0]);
        const FString Prefix=FString::Printf(TEXT("v8448_dynamic_mg_t%d_model%d_"),Team,int32(Type));
        Check(Prefix+TEXT("real_tick_establishes_supported_trench_gun"),G&&G->IsSupported()&&G->IsTrench());
        auto* V=NewTank(Type,Team);if(!V||People.IsEmpty()){Check(Prefix+TEXT("actors_present"),false);continue;}
        const float Lane=People[0]->Feet().Y,Bank=FrontlineFieldV32::TrenchCentreX(Trench,Lane);Place(V,FVector(Bank-Dir*1250.f,Lane,0));
        auto J=RunTrajectory(Prefix+TEXT("actual_section_and_fleet_tick"),V,Bank,30.f,true,false);
        Check(Prefix+TEXT("actual_low_section_is_not_a_permanent_standing_wall"),J->GetBoolField(TEXT("finite"))&&J->GetBoolField(TEXT("bounded_travel"))
            &&J->GetBoolField(TEXT("all_poses_clear_actual_bodies"))&&J->GetNumberField(TEXT("bank_progress_cm"))>700.f);
        // Same actual final pose: show whether the old 145 cm body creates
        // a contact that is absent for this rendered supported crew.
        for(auto* U:People)
        {
            const FVector P(U->Feet().X,U->Feet().Y,0);const FTransform Saved=V->GetActorTransform();const float Heading=V->HeadingYawV37;
            V->HeadingYawV37=Dir>0?0.f:180.f;V->FitGroundV35(P,0);
            const bool Standing=V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37);
            const bool Posed=V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37,34.f,0.f,U);
            LowBodyWitnesses+=Standing&&!Posed;V->SetActorTransform(Saved);V->HeadingYawV37=Heading;
        }
    }
    Report->SetNumberField(TEXT("rendered_low_body_standing_envelope_witnesses"),LowBodyWitnesses);
    // The same supported formation must remain reciprocal while a real
    // AdvanceSquad order packs the gun and evaluates its standing/run clips.
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team),Id=8448200+Team;B->Trenches[Trench].Owner=Team;
        FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Trench=Q.Destination=Q.LastPassedTrench=Trench;Q.Slot=1;Q.bHoldPosition=true;
        for(int32 M=0;M<3;++M)if(auto* U=NewPerson(Team,FVector(B->Trenches[Trench].X,-500.f+M*160.f,0),EFrontlineRole::Lewis,Id,M))Q.Members.Add(U);
        B->Squads.Add(Q);for(int32 F=0;F<260;++F){B->Elapsed+=.05f;for(auto* U:People)TickPerson(U,.05f);}
        auto* G=People.IsEmpty()?nullptr:FrontlineMachinegunsV30::Get(People[0]);auto* V=NewTank(EFrontlineVehicle::Whippet,Team);
        const FString Prefix=FString::Printf(TEXT("v8448_real_mg_departure_t%d_"),Team);
        Check(Prefix+TEXT("supported_crew_and_vehicle_ready"),G&&G->IsSupported()&&V);if(!G||!V)continue;
        const float Lane=People[0]->Feet().Y,Bank=FrontlineFieldV32::TrenchCentreX(Trench,Lane);Place(V,FVector(Bank-Dir*650.f,Lane,0));
        const FVector CrewStart=People[0]->Feet();auto J=RunTrajectory(Prefix+TEXT("pack_to_stand_during_bank_crossing"),V,Bank,30.f,true,false,Id);
        Check(Prefix+TEXT("actual_pack_then_carry_motion_observed"),J->GetBoolField(TEXT("real_departure_order_accepted"))&&J->GetNumberField(TEXT("actual_pack_pose_frames"))>0
            &&J->GetNumberField(TEXT("actual_carry_pose_frames"))>0&&(People[0]->Feet().X-CrewStart.X)*Dir>100.f);
        Check(Prefix+TEXT("posture_changes_preserve_body_clearance_and_forward_resolution"),J->GetBoolField(TEXT("finite"))&&J->GetBoolField(TEXT("bounded_travel"))
            &&J->GetBoolField(TEXT("all_poses_clear_actual_bodies"))&&J->GetNumberField(TEXT("bank_progress_cm"))>700.f);
    }
    // Live disabled armor and a persistent wreck are present together at
    // the bank. This is an ordinary fleet Tick, not a detached Advance call.
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team);const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,300.f);
        auto* V=NewTank(EFrontlineVehicle::MarkIVMale,Team);auto* Disabled=NewTank(EFrontlineVehicle::A7V,Team);auto* Wreck=NewTank(EFrontlineVehicle::Whippet,Team);
        const FString Prefix=FString::Printf(TEXT("v8448_live_friend_and_wreck_t%d_"),Team);Check(Prefix+TEXT("actual_fleet_present"),V&&Disabled&&Wreck);if(!V||!Disabled||!Wreck)continue;
        Place(Disabled,FVector(Bank-Dir*120.f,300.f,0));Disabled->TrackIntegrity[0]=0.f;
        Place(Wreck,FVector(Bank+Dir*800.f,-300.f,0));Wreck->HullIntegrity=0.f;Wreck->WreckAge=130.f;
        Place(V,FVector(Bank-Dir*1700.f,300.f,0));const FVector DisabledStart=Disabled->GetActorLocation(),WreckStart=Wreck->GetActorLocation();
        auto J=RunTrajectory(Prefix+TEXT("simultaneous_bank_bypass"),V,Bank,34.f,false,false);
        Check(Prefix+TEXT("bank_bypass_is_bounded_and_resolves_forward"),J->GetBoolField(TEXT("finite"))&&J->GetBoolField(TEXT("bounded_travel"))
            &&J->GetNumberField(TEXT("bank_progress_cm"))>1200.f&&J->GetNumberField(TEXT("max_requested_without_net_progress_seconds"))<3.f);
        Check(Prefix+TEXT("disabled_friend_and_wreck_remain_stationary"),Disabled->GetActorLocation().Equals(DisabledStart,.001f)&&Wreck->GetActorLocation().Equals(WreckStart,.001f));
    }
    // Grounded hull-eye-only sightings: the previous rule may turn the hull
    // despite having no reachable muzzle/elevation solution at that position.
    int32 UnusableSightWitnesses=0;
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();auto* A=NewTank(EFrontlineVehicle::MarkIVMale,Team);if(!A)continue;
        const int32 Dir=B->GetTeamDirection(Team);auto* U=NewPerson(1-Team,FVector(B->Trenches[Trench].X,300.f,0));if(!U)continue;
        bool Captured=false;
        for(float Offset:{200.f,350.f,500.f,750.f,1100.f,1500.f})for(float Across:{-700.f,-350.f,0.f,350.f,700.f})
        {
            const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,300.f);Place(A,FVector(Bank-Dir*Offset,300.f,0));
            FVector P(FrontlineFieldV32::TrenchCentreX(Trench,300.f+Across)-Dir*100.f,300.f+Across,0);
            P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));Pose(U,0);
            bool Mounted=false;FVector Point;for(int32 I=0;I<A->CannonMounts.Num()&&!Mounted;++I)Mounted=A->InfantryFiringSolutionV828(U,I,true,Point);
            for(int32 I=0;I<A->GunMounts.Num()&&!Mounted;++I)Mounted=A->InfantryFiringSolutionV828(U,I,false,Point);
            const FVector Head=U->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5);
            const bool Seen=B->DirectFireMultiplier(A->AimPoint(),Head)>=.9f&&ClearSight(GetWorld(),A,U,A->AimPoint(),Head);
            float Heading=0;const bool Feasible=A->MountedHeadingV8448(Head,Heading,false,U);
            const FTransform Original=A->GetActorTransform();const float Yaw=A->HeadingYawV37;
            LegacyContactTurnV8448=true;A->FaceArmourV48(U->Feet()+FVector(0,0,105),.05f,false,U);
            const bool OldTurn=FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,A->HeadingYawV37))>.01f;
            LegacyContactTurnV8448=false;A->HeadingYawV37=Yaw;A->SetActorTransform(Original);
            if(Seen&&!Mounted&&!Feasible&&OldTurn)
            {
                ++UnusableSightWitnesses;
                if(!Captured)
                {
                    const FString Prefix=FString::Printf(TEXT("v8448_hull_eye_only_t%d_"),Team);
                    const FVector Start=A->GetActorLocation();
                    auto Old=RunTrajectory(Prefix+TEXT("old_turn_gate"),A,Bank,10.f,false,true);
                    Place(A,Start);auto New=RunTrajectory(Prefix+TEXT("verified_muzzle_gate"),A,Bank,10.f,false,false);
                    Check(Prefix+TEXT("new_rule_preserves_grounded_valid_progress_or_real_firing_halt"),New->GetBoolField(TEXT("finite"))&&New->GetBoolField(TEXT("bounded_travel"))
                        &&New->GetBoolField(TEXT("all_poses_clear_actual_bodies"))&&New->GetNumberField(TEXT("max_requested_without_net_progress_seconds"))<3.f
                        &&New->GetNumberField(TEXT("max_unready_contact_halt_seconds"))<2.5f
                        &&(New->GetNumberField(TEXT("net_forward_cm"))>200.f
                            ||(A->bMobilityFiringReadyV8448&&!A->bMobilityTraversingV8448&&A->MobilityRequestedV839==0.f)));
                    auto Proof=MakeShared<FJsonObject>();Proof->SetStringField(TEXT("case"),Prefix+TEXT("exact_unusable_sight_counterexample"));
                    Proof->SetStringField(TEXT("initial_hull"),Start.ToString());Proof->SetStringField(TEXT("real_target_feet"),U->Feet().ToString());
                    Proof->SetBoolField(TEXT("hull_eye_seen"),Seen);Proof->SetBoolField(TEXT("actual_mount_ready"),Mounted);Proof->SetBoolField(TEXT("feasible_final_muzzle_heading"),Feasible);
                    Proof->SetBoolField(TEXT("old_rule_turns_and_would_halt"),OldTurn);Cases.Add(MakeShared<FJsonValueObject>(Proof));Captured=true;
                }
            }
        }
    }
    Report->SetNumberField(TEXT("unusable_hull_eye_turn_witnesses"),UnusableSightWitnesses);
    Check(TEXT("v8448_hull_eye_only_parapet_contact_has_real_old_rule_witness"),UnusableSightWitnesses>0);
    // A fixed side gun must obtain its firing arc through actual rolling
    // steering and then fire from its real muzzle, rather than merely pass.
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team);const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,300.f);
        auto* A=NewTank(EFrontlineVehicle::MarkIVMale,Team);auto* E=NewTank(EFrontlineVehicle::A7V,1-Team);
        const FString Prefix=FString::Printf(TEXT("v8448_fixed_sponson_t%d_"),Team);Check(Prefix+TEXT("actual_armour_present"),A&&E);if(!A||!E)continue;
        const FVector Start(Bank-Dir*2200.f,Team==0?-500.f:500.f,0);Place(A,Start);
        E->HullIntegrity=100000.f;E->TrackIntegrity[0]=0.f;for(float& C:A->CannonClock)C=0.f;
        FVector P;bool Ready=false,AimReady=false,Found=false;float FutureYaw=0.f;
        for(float Ahead:{50.f,100.f,150.f,250.f})for(float Across:{700.f,800.f,900.f})if(!Found)
        {
            Place(E,FVector(Start.X+Dir*Ahead,Team==0?Across:-Across,0));Ready=AimReady=false;
            for(int32 I=0;I<A->CannonMounts.Num();++I)
            {Ready|=A->ArmourFiringSolutionV8219(E,I,P);AimReady|=A->ArmourFiringSolutionV8219(E,I,P,true);}
            FVector Observed;const bool Seen=A->ArmourSightV48(E,Observed);
            const bool Future=Seen&&A->MountedHeadingV8448(Observed,FutureYaw,true,E);
            const float ForwardYaw=Dir>0?0.f:180.f;
            Found=!Ready&&!AimReady&&Future&&FMath::Abs(FMath::FindDeltaAngleDegrees(A->HeadingYawV37,FutureYaw))>.1f
                &&FMath::Abs(FMath::FindDeltaAngleDegrees(ForwardYaw,FutureYaw))<=60.f;
        }
        Check(Prefix+TEXT("initial_contact_needs_real_chassis_acquisition"),Found&&!Ready&&!AimReady);
        if(!Found)continue;
        const int32 Shots=A->CannonShots;auto J=RunTrajectory(Prefix+TEXT("real_rolling_acquisition_and_discharge"),A,Bank,20.f,false,false);
        Check(Prefix+TEXT("live_target_is_acquired_and_actually_fired_on"),A->CannonShots>Shots&&E->HullIntegrity<100000.f
            &&J->GetNumberField(TEXT("net_forward_cm"))>20.f
            &&J->GetBoolField(TEXT("finite"))&&J->GetBoolField(TEXT("bounded_travel"))&&J->GetBoolField(TEXT("all_poses_clear_actual_bodies"))
            &&J->GetNumberField(TEXT("max_unready_contact_halt_seconds"))<2.5f);
        J->SetBoolField(TEXT("initial_actual_mount_ready"),Ready);J->SetBoolField(TEXT("initial_current_pose_aim_ready"),AimReady);
        J->SetNumberField(TEXT("initial_proved_future_heading_deg"),FutureYaw);J->SetStringField(TEXT("initial_target_pose"),E->GetActorTransform().ToString());
        J->SetNumberField(TEXT("target_remaining_hull"),E->HullIntegrity);
    }
    // Mounted clear firing positions and disabled running gear still stop.
    Clear();auto* V=NewTank(EFrontlineVehicle::MarkIVMale,0);if(V)
    {
        const int32 Dir=B->GetTeamDirection(0);const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,300.f);
        Place(V,FVector(Bank-Dir*2500.f,300.f,0));auto* U=NewPerson(1,FVector(V->GetActorLocation().X+Dir*1000.f,600.f,0));bool Mounted=false;
        for(float Ahead:{700.f,1000.f,1500.f})for(float Across:{-600.f,-300.f,0.f,300.f,600.f})if(U&&!Mounted)
        {
            FVector P=V->GetActorLocation()+FVector(Dir*Ahead,Across,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));Pose(U,0);FVector Point;
            for(int32 I=0;I<V->CannonMounts.Num()&&!Mounted;++I)Mounted=V->InfantryFiringSolutionV828(U,I,true,Point);
            for(int32 I=0;I<V->GunMounts.Num()&&!Mounted;++I)Mounted=V->InfantryFiringSolutionV828(U,I,false,Point);
        }
        const FVector Halt=V->GetActorLocation();B->Elapsed+=.05f;Tick(.05f);
        Check(TEXT("v8448_legitimate_observed_gun_position_remains_stationary"),Mounted&&V->bMobilityFiringReadyV8448&&V->MobilityRequestedV839==0.f&&V->GetActorLocation().Equals(Halt,.001f));
        // A present aiming position also finishes real barrel alignment and
        // discharges; muzzle-ready/aiming bookkeeping alone cannot pass this.
        for(float& C:V->CannonClock)C=0.f;for(float& C:V->GunClock)C=0.f;
        const int32 Fired=V->CannonShots+V->MachinegunShots;
        for(int32 F=0;F<80&&V->CannonShots+V->MachinegunShots==Fired;++F){B->Elapsed+=.05f;Tick(.05f);}
        Check(TEXT("v8448_present_aiming_position_finishes_alignment_and_real_discharge"),V->CannonShots+V->MachinegunShots>Fired);
        V->TrackIntegrity[0]=0.f;const FVector Disabled=V->GetActorLocation();B->Elapsed+=.05f;Tick(.05f);
        Check(TEXT("v8448_running_gear_damage_still_stops_vehicle"),FString(V->MobilityGateV839)==TEXT("running_gear_disabled")&&V->GetActorLocation().Equals(Disabled,.001f));
    }
    Report->SetStringField(TEXT("scope"),TEXT("UK/DE current production terrain and real fleet Tick: paired fixed-lane counterfactuals, standing-body shoulders, natural supported MG sections, real AdvanceSquad packing/standing transitions and skeletal evaluation, simultaneous disabled friend and wreck, mixed step sizes, net forward progress, body clearance, mounted firing stops and track damage. Not an all-map, full-campaign or visual acceptance claim."));
    return Report;
}
