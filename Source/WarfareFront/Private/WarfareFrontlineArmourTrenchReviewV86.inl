// Full held sections, real Soldier Tick and production vehicle Simulate.
// Initial placement is the ordinary authored bay; no routes are searched for
// test success and no departure order is issued to release a blocked hull.
TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunTrenchMobilityChecksV86()
{
    auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](FString Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    ON_SCOPE_EXIT{R->SetBoolField(TEXT("success"),Good);R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("cases"),Cases);};
    auto* B=Battle();FString User;
    const bool Safe=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
    Check(TEXT("v86_trench_exact_disposable_ukde_world"),Safe);if(!Safe)return R;
    const auto OldVehicles=Vehicles;const auto OldUnits=B->Units;const auto OldSquads=B->Squads;const auto OldTrenches=B->Trenches;
    const auto OldSmoke=B->SmokeZones;const auto OldTracers=B->Tracers;const auto OldPhase=B->Phase;const auto OldEncounter=LastEncounter;
    const int32 OldSerial=NextSerial,OldWinner=B->BattleOutroWinner,OldAdvances=B->AdvanceCommands;
    const float OldTime=B->Elapsed,OldEnemy=NextEnemyAt,OldOutro=B->BattleOutroElapsed;const bool OldPause=UGameplayStatics::IsGamePaused(B);
    TArray<AWarfareFrontlineSoldier*> People;TArray<AFrontlineTank*> Tanks;
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(const auto& V:OldVehicles)Hide(V.Get());for(const auto& U:OldUnits)Hide(U.Get());
    const auto Clear=[&]{for(auto* U:People)if(IsValid(U))U->Destroy();for(auto* V:Tanks)if(IsValid(V))V->Destroy();People.Reset();Tanks.Reset();Vehicles.Reset();B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Tracers.Reset();B->Trenches=OldTrenches;B->Elapsed=1000.f;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;NextEnemyAt=MAX_flt;NextSerial=8600;LastEncounter=B->GetEncounterId();FrontlineMobilityV41::Reset(B);};
    ON_SCOPE_EXIT{Clear();Vehicles=OldVehicles;B->Units=OldUnits;B->Squads=OldSquads;B->Trenches=OldTrenches;B->SmokeZones=OldSmoke;B->Tracers=OldTracers;B->Phase=OldPhase;B->Elapsed=OldTime;B->BattleOutroElapsed=OldOutro;B->BattleOutroWinner=OldWinner;B->AdvanceCommands=OldAdvances;NextEnemyAt=OldEnemy;NextSerial=OldSerial;LastEncounter=OldEncounter;for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}UGameplayStatics::SetGamePaused(B,OldPause);};
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();};
    const auto Infantry=[&](float DT){FFrontlineArmourMobilityAccessV84411::Schedule(B);for(auto* U:People)if(U->IsAlive()){U->Tick(DT);Pose(U,DT);if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const double StartWall=FPlatformTime::Seconds();int32 Trials=0,MatrixTrial=0;
    for(int32 Team=0;Team<2;++Team)for(int32 Mix=0;Mix<2;++Mix)
        for(auto Type:{EFrontlineVehicle::MarkVStar,EFrontlineVehicle::Whippet,EFrontlineVehicle::A7V,EFrontlineVehicle::Austin1918})
    {
        ++MatrixTrial;FString Focus;const FString CurrentCase=FString::Printf(TEXT("v86_t%d_m%d_mix%d"),Team,int32(Type),Mix);
        if(FParse::Value(FCommandLine::Get(),TEXT("WFV86TrenchCase="),Focus)&&!Focus.IsEmpty()&&Focus!=CurrentCase)continue;
        if(FPlatformTime::Seconds()-StartWall>160.){Check(TEXT("v86_trench_bounded_review_wall_budget"),false);R->SetNumberField(TEXT("completed_trials"),Trials);return R;}
        Clear();UGameplayStatics::SetGamePaused(B,false);++Trials;
        const int32 Dir=B->GetTeamDirection(Team),Trench=B->Trenches.Num()/2;const FString Name=FString::Printf(TEXT("v86_t%d_m%d_mix%d"),Team,int32(Type),Mix);
        Check(Name+TEXT("_authored_trench"),B->Trenches.IsValidIndex(Trench));if(!B->Trenches.IsValidIndex(Trench))continue;
        B->Trenches[Trench].Owner=Team;
        for(int32 Slot=0;Slot<FFrontlineTrench::Capacity;++Slot)
        {
            FFrontlineSquad Q;Q.Id=860000+MatrixTrial*10+Slot;Q.Team=Team;Q.Role=Mix?EFrontlineRole::Lewis:EFrontlineRole::Rifle;
            Q.Trench=Q.Destination=Q.LastPassedTrench=Trench;Q.Slot=Slot;Q.bHoldPosition=true;B->Squads.Add(Q);
            for(int32 Member=0;Member<B->GetSquadSize(Q.Role,Team);++Member)
            {
                auto* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
                U->Initialize(B,Q.Id,Team,Q.Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;FFrontlineArmourMobilityAccessV84411::DelayWeapons(U);U->ReturnToIdle();
                People.Add(U);B->Units.Add(U);B->FindSquad(Q.Id)->Members.Add(U);
                FVector P=B->SoldierDestination(U);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));Pose(U,0);
            }
        }
        for(int32 F=0;F<60;++F){B->Elapsed+=1.f/15.f;Infantry(1.f/15.f);}
        auto* V=GetWorld()->SpawnActor<AFrontlineTank>();const bool Configured=V&&V->Configure(B,Type,Team,++NextSerial);
        Check(Name+TEXT("_actual_chassis_and_full_held_squads"),Configured&&V->AreAssetsReady()&&B->Squads.Num()==FFrontlineTrench::Capacity&&People.Num()>=9);
        if(!Configured){if(V)V->Destroy();continue;}Tanks.Add(V);Vehicles.Add(V);V->SetActorTickEnabled(false);V->bEntered=true;
        const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,0);
        V->HeadingYawV37=Dir>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=0;V->FitGroundV35(FVector(Bank-Dir*1500.f,0,0),0);
        const float ForwardYaw=V->HeadingYawV37;
        V->HeadingYawV37=ForwardYaw+180.f;
        Check(Name+TEXT("_passing_certificate_rejects_reverse_heading"),!V->StraightInfantryPassWithinFieldV86(People[0]));
        V->HeadingYawV37=ForwardYaw;
        float Time=0,Idle=0,MaxIdle=0,MaxStep=0,Reverse=0;int32 Intersections=0,Requested=0;bool Finite=true,Bounded=true,CertificateProof=true,WholeBodyInField=true;
        TSharedPtr<FJsonObject> FirstMotionViolation;bool StoppedAfterDeadlock=false;
        TArray<TSharedPtr<FJsonValue>> Samples;
        while(Time<100.f&&(V->GetActorLocation().X-Bank)*Dir<1100.f)
        {
            const float DT=MatrixTrial%2?1.f/30.f:1.f/15.f;const FVector Before=V->GetActorLocation();const float OldYaw=V->HeadingYawV37;
            B->Elapsed+=DT;Time+=DT;Infantry(DT);V->Simulate(DT);
            const float Moved=FVector::Dist2D(Before,V->GetActorLocation());MaxStep=FMath::Max(MaxStep,Moved);
            const bool Wants=V->MobilityRequestedV839>.01f;Requested+=Wants;Idle=Wants&&Moved<.05f?Idle+DT:0;MaxIdle=FMath::Max(MaxIdle,Idle);
            const float Back=float((Before.X-V->GetActorLocation().X)*Dir);if(Back>0)Reverse+=Back;
            const float Cap=V->SteeringRateV8311(Moved>.01f?V->MobilityRequestedV839/DT:0.f,Moved<=.01f);
            const bool FiniteFrame=!V->GetActorTransform().ContainsNaN();
            const bool TravelBound=Moved<=V->MobilityRequestedV839+.01f;
            const float Turn=float(FMath::Abs(FMath::FindDeltaAngleDegrees(OldYaw,V->HeadingYawV37)));
            const bool TurnBound=Turn<=Cap*DT+.01f;
            Finite&=FiniteFrame;Bounded&=TravelBound&&TurnBound;
            if(!FirstMotionViolation&&(!FiniteFrame||!TravelBound||!TurnBound))
            {
                FirstMotionViolation=MakeShared<FJsonObject>();FirstMotionViolation->SetNumberField(TEXT("relative_seconds"),Time);
                FirstMotionViolation->SetBoolField(TEXT("finite"),FiniteFrame);FirstMotionViolation->SetBoolField(TEXT("travel_bound"),TravelBound);FirstMotionViolation->SetBoolField(TEXT("turn_bound"),TurnBound);
                FirstMotionViolation->SetNumberField(TEXT("old_heading_deg"),OldYaw);FirstMotionViolation->SetNumberField(TEXT("new_heading_deg"),V->HeadingYawV37);
                FirstMotionViolation->SetNumberField(TEXT("turn_deg"),Turn);FirstMotionViolation->SetNumberField(TEXT("moved_cm"),Moved);
                FirstMotionViolation->SetNumberField(TEXT("requested_cm"),V->MobilityRequestedV839);FirstMotionViolation->SetNumberField(TEXT("host_seconds"),DT);
                FirstMotionViolation->SetNumberField(TEXT("steering_cap_deg_s"),Cap);FirstMotionViolation->SetNumberField(TEXT("bypass_phase"),V->BypassPhaseV839);
                FirstMotionViolation->SetStringField(TEXT("position"),V->GetActorLocation().ToString());
            }
            const FBox ActualBody=V->Body->Bounds.GetBox();
            WholeBodyInField&=ActualBody.IsValid&&ActualBody.Min.Y>=-FrontlineBattlefield::HalfFrontage
                &&ActualBody.Max.Y<=FrontlineBattlefield::HalfFrontage;
            if(V->bBypassInfantryPassingV84412)
            {
                const auto* Owner=V->BypassInfantryV84411.Get();
                CertificateProof&=IsValid(Owner)&&V->StraightInfantryPassWithinFieldV86(Owner)
                    &&FVector::DistSquared2D(Owner->Feet(),V->BypassPassFeetV86)<=FMath::Square(15.f)
                    &&FMath::Abs(FMath::FindDeltaAngleDegrees(V->BypassPassHeadingV86,V->HeadingYawV37))<=.5f;
            }
            for(auto* U:People)Intersections+=V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37,34.f,0.f,U);
            if(Samples.IsEmpty()||Time>=Samples.Num()*1.5f){auto S=V->DescribeMobilityV839();S->SetNumberField(TEXT("relative_seconds"),Time);Samples.Add(MakeShared<FJsonValueObject>(S));}
            // The pass requirement is MaxIdle<5. Once ten seconds have
            // actually elapsed the case is irreversibly failed; preserve the
            // same failure and physical evidence without repeating the same
            // rejected production plan for the rest of the 100-second budget.
            if(MaxIdle>=10.f){StoppedAfterDeadlock=true;break;}
        }
        auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),Name);J->SetNumberField(TEXT("bank_progress_cm"),(V->GetActorLocation().X-Bank)*Dir);J->SetNumberField(TEXT("duration_seconds"),Time);J->SetNumberField(TEXT("longest_requested_stall_seconds"),MaxIdle);J->SetNumberField(TEXT("actual_infantry_hull_intersections"),Intersections);J->SetNumberField(TEXT("actual_reverse_cm"),Reverse);J->SetNumberField(TEXT("maximum_step_cm"),MaxStep);J->SetNumberField(TEXT("requested_frames"),Requested);J->SetArrayField(TEXT("samples"),Samples);
        if(FirstMotionViolation)J->SetObjectField(TEXT("first_motion_violation"),FirstMotionViolation);
        J->SetBoolField(TEXT("stopped_after_irreversible_deadlock"),StoppedAfterDeadlock);
        TArray<TSharedPtr<FJsonValue>> Blockers;
        for(auto* U:People)if(V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation()+FRotator(0,V->HeadingYawV37,0).Vector()*V->MobilityRequestedV839,V->HeadingYawV37,34.f,V->MobilityStepV839,U))
        {
            auto E=MakeShared<FJsonObject>();E->SetNumberField(TEXT("squad"),U->SquadId);E->SetNumberField(TEXT("member"),U->MemberIndex);E->SetStringField(TEXT("feet"),U->Feet().ToString());E->SetBoolField(TEXT("main_gunner"),U->IsMachineGunner());
            const FVector LocalFeet=V->Body->GetComponentTransform().InverseTransformPosition(U->Feet());
            const FVector OldAhead=V->GetActorLocation()+FRotator(0,V->HeadingYawV37,0).Vector()*FMath::Clamp(float(LocalFeet.X),0.f,390.f);
            E->SetStringField(TEXT("hull_local_feet"),LocalFeet.ToString());E->SetStringField(TEXT("old_nose_prediction"),OldAhead.ToString());
            E->SetBoolField(TEXT("old_nose_prediction_blocks"),V->BlocksInfantryAtV8218(U->Feet(),OldAhead,V->HeadingYawV37,50.f,0.f,U));
            E->SetBoolField(TEXT("actual_current_body_blocks"),V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37,34.f,0.f,U));
            E->SetBoolField(TEXT("actual_next_supported_body_blocks"),V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation()+FRotator(0,V->HeadingYawV37,0).Vector()*V->MobilityRequestedV839,V->HeadingYawV37,34.f,V->MobilityStepV839,U));
            if(const FVector* Attempt=V->BypassInfantryAttemptsV84411.Find(U))
            {E->SetStringField(TEXT("last_yield_attempt_feet"),Attempt->ToString());E->SetNumberField(TEXT("distance_from_last_attempt_cm"),FVector::Dist2D(U->Feet(),*Attempt));}
            const float ReverseStep=FMath::Min(V->MobilityRequestedV839,20.f);
            const FVector Rear=V->GetActorLocation()-FRotator(0,V->HeadingYawV37,0).Vector()*ReverseStep;
            E->SetBoolField(TEXT("next_bounded_reverse_clear"),V->CanOccupyV48(Rear,V->HeadingYawV37,V->MobilityStepV839));
            E->SetNumberField(TEXT("actual_motion_cm_s"),U->GetMotionSpeed());
            FVector Q;E->SetBoolField(TEXT("yield_endpoint_available"),V->InfantryYieldPointV8218(U,Q));E->SetStringField(TEXT("yield_endpoint"),Q.ToString());if(auto Step=FrontlineReadArmourStepV84411(U))E->SetObjectField(TEXT("actual_step"),Step);Blockers.Add(MakeShared<FJsonValueObject>(E));
        }
        J->SetArrayField(TEXT("last_actual_body_blockers"),Blockers);Cases.Add(MakeShared<FJsonValueObject>(J));
        Check(Name+TEXT("_complete_held_trench_crossing_without_departure"),(V->GetActorLocation().X-Bank)*Dir>1000.f&&B->AdvanceCommands==OldAdvances);
        Check(Name+TEXT("_physical_contact_speed_turn_and_field_bounds"),Finite&&Bounded&&Intersections==0&&FMath::Abs(V->GetActorLocation().Y)<=980.1f);
        Check(Name+TEXT("_whole_rendered_body_stays_within_authored_field"),WholeBodyInField);
        Check(Name+TEXT("_no_sustained_requested_deadlock"),MaxIdle<5.f);
        Check(Name+TEXT("_retained_certificate_direction_station_and_complete_tail_domain"),CertificateProof);
    }
    R->SetNumberField(TEXT("completed_trials"),Trials);R->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-StartWall);
    R->SetStringField(TEXT("scope"),TEXT("Current authored UK/DE middle trench; complete held rifle or supported machine-gun sections; Mark V*, Whippet, A7V and Austin; actual squad schedule, rendered infantry poses and vehicle Simulate at 15/30 host Hz. No public departure, collision bypass, success placement search, altered travel budget or FR/RU acceptance."));
    return R;
}
