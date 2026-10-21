// UK/DE production scheduling regression. Only the disposable V84411 runner
// can enter; the normal vehicle tick, rendered infantry poses and body sweeps
// remain authoritative. No actors are teleported during the trajectory.
bool FrontlineArmourYieldGoalBodyClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Destination);
TSharedPtr<FJsonObject> FrontlineReadArmourStepV84411(const AWarfareFrontlineSoldier* U);
struct FFrontlineArmourMobilityAccessV84411
{
    static void Schedule(AWarfareFrontlineGameMode* B){B->UpdateSquads();}
    static void DelayWeapons(AWarfareFrontlineSoldier* U){U->ShotClock=U->GrenadeClock=10000.f;U->DepartureClock=U->ReactionSeconds=0;}
    static bool WantsMovement(AWarfareFrontlineSoldier* U){return U->bWantedMovement;}
    static float PlanningSpeed(AWarfareFrontlineSoldier* U){return U->RequestedTravelSpeedV43;}
    static FRandomStream SnapshotRandom(const AWarfareFrontlineGameMode* B){return B->Random;}
    static void RestoreRandom(AWarfareFrontlineGameMode* B,const FRandomStream& R){B->Random=R;}
};
TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunMobilityChecksV84411()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;};
    ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);Report->SetBoolField(TEXT("success"),Good);};
    auto* B=Battle();FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84411/Runtime"));FPaths::NormalizeDirectoryName(Root);
    const FString Stamp=User.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)?User.Mid(Root.Len()+1):FString();
    const bool Safe=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&!FPaths::IsRelative(User)&&!User.Contains(TEXT(".."))&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    Check(TEXT("v84411_armour_strict_disposable_ukde_profile"),Safe);if(!Safe)return Report;
    const auto OriginalFleet=Vehicles;const auto OriginalUnits=B->Units;const auto OriginalSquads=B->Squads;const auto OriginalTrenches=B->Trenches;
    const int32 OriginalShots=B->ShotsFired,OriginalCasualties[2]={B->Casualties[0],B->Casualties[1]};
    const float OriginalMorale[2]={B->Morale[0],B->Morale[1]},OriginalDamage[2]={B->DamageDealt[0],B->DamageDealt[1]},OriginalFriendlyFire=B->FriendlyFireDamage;
    const FFrontlineCombatRecordV65 OriginalCombat[2]={B->CombatRecordV65[0],B->CombatRecordV65[1]};
    const FString OriginalMessage=B->LastMessage,OriginalEnd=B->EndReason;
    TSet<const AWarfareFrontlineImpact*> ExistingImpact;for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)ExistingImpact.Add(*It);
    const auto OriginalSmoke=B->SmokeZones;const auto OriginalTracers=B->Tracers;const auto OriginalPhase=B->Phase;const auto Encounter=LastEncounter;
    const float Clock=B->Elapsed,EnemyAt=NextEnemyAt,OutroTime=B->BattleOutroElapsed;const int32 Serial=NextSerial,Outro=B->BattleOutroWinner,Advances=B->AdvanceCommands;
    const bool Paused=UGameplayStatics::IsGamePaused(GetWorld());TArray<AWarfareFrontlineSoldier*> People;TArray<AFrontlineTank*> Tanks;
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    for(AFrontlineTank* V:Vehicles)if(IsValid(V)){Hidden.Add({V,V->IsHidden(),V->GetActorEnableCollision()});V->SetActorHiddenInGame(true);V->SetActorEnableCollision(false);}
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){Hidden.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
    const auto Clear=[&](){for(auto* U:People)if(IsValid(U))U->Destroy();People.Reset();B->Units.Reset();B->Squads.Reset();
        for(auto* V:Tanks)if(IsValid(V))V->Destroy();Tanks.Reset();Vehicles.Reset();B->Trenches=OriginalTrenches;B->SmokeZones.Reset();B->Tracers.Reset();
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=100.f;LastEncounter=B->GetEncounterId();NextEnemyAt=MAX_flt;
        FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);UGameplayStatics::SetGamePaused(GetWorld(),false);};
    ON_SCOPE_EXIT{FrontlineSetLegacyArmourYieldV84411(false);Clear();Vehicles=OriginalFleet;B->Units=OriginalUnits;B->Squads=OriginalSquads;B->Trenches=OriginalTrenches;B->SmokeZones=OriginalSmoke;B->Tracers=OriginalTracers;
        B->Phase=OriginalPhase;B->Elapsed=Clock;B->BattleOutroWinner=Outro;B->BattleOutroElapsed=OutroTime;B->AdvanceCommands=Advances;LastEncounter=Encounter;NextEnemyAt=EnemyAt;NextSerial=Serial;
        B->ShotsFired=OriginalShots;B->FriendlyFireDamage=OriginalFriendlyFire;B->LastMessage=OriginalMessage;B->EndReason=OriginalEnd;
        for(int32 Team=0;Team<2;++Team){B->Morale[Team]=OriginalMorale[Team];B->DamageDealt[Team]=OriginalDamage[Team];B->Casualties[Team]=OriginalCasualties[Team];B->CombatRecordV65[Team]=OriginalCombat[Team];}
        for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)if(!ExistingImpact.Contains(*It))It->Destroy();
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}UGameplayStatics::SetGamePaused(GetWorld(),Paused);};
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();};
    const auto InfantryTick=[&](float DT){FFrontlineArmourMobilityAccessV84411::Schedule(B);for(auto* U:People)if(U->IsAlive()){U->Tick(DT);Pose(U,DT);if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const auto NewTank=[&](EFrontlineVehicle Type,int32 Team,FVector P){auto* V=GetWorld()->SpawnActor<AFrontlineTank>();if(!V||!V->Configure(B,Type,Team,++NextSerial)){if(V)V->Destroy();return (AFrontlineTank*)nullptr;}
        Tanks.Add(V);Vehicles.Add(V);V->HeadingYawV37=B->GetTeamDirection(Team)>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->bEntered=true;V->FitGroundV35(P,0);
        for(float& C:V->CannonClock)C=10000.f;for(float& C:V->GunClock)C=10000.f;return V;};
    const auto Section=[&](int32 Team,int32 Trench,int32 Slot,EFrontlineRole Role,int32 Id){FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=Role;Q.Trench=Q.Destination=Q.LastPassedTrench=Trench;Q.Slot=Slot;Q.bHoldPosition=true;B->Squads.Add(Q);
        for(int32 M=0;M<B->GetSquadSize(Role,Team);++M){auto* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;U->Initialize(B,Id,Team,Role,M);U->SetActorTickEnabled(false);U->bFieldEntered=true;FFrontlineArmourMobilityAccessV84411::DelayWeapons(U);U->ReturnToIdle();
            People.Add(U);B->Units.Add(U);B->FindSquad(Id)->Members.Add(U);FVector P=B->SoldierDestination(U);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0.f:180.f,0));Pose(U,0);}};
    const bool RegressionReview=FParse::Param(FCommandLine::Get(),TEXT("WFV84412ArmourRegressionReview"));
    const bool FocusDiagnostic=!RegressionReview&&FParse::Param(FCommandLine::Get(),TEXT("WFV84411ArmourFocus"));
    int32 FocusTeam=1,FocusContext=0,FocusModel=int32(EFrontlineVehicle::MarkIVMale);
    FParse::Value(FCommandLine::Get(),TEXT("WFV84411ArmourTeam="),FocusTeam);
    FParse::Value(FCommandLine::Get(),TEXT("WFV84411ArmourContext="),FocusContext);
    FParse::Value(FCommandLine::Get(),TEXT("WFV84411ArmourModel="),FocusModel);
    FocusTeam=FMath::Clamp(FocusTeam,0,1);FocusContext=FMath::Clamp(FocusContext,0,2);
    if(FocusModel!=int32(EFrontlineVehicle::MarkIVMale)&&FocusModel!=int32(EFrontlineVehicle::Whippet)
        &&FocusModel!=int32(EFrontlineVehicle::Austin1918))FocusModel=int32(EFrontlineVehicle::MarkIVMale);
    Report->SetBoolField(TEXT("partial_diagnostic_only"),FocusDiagnostic||RegressionReview);
    Report->SetBoolField(TEXT("focused_regression_suite"),RegressionReview);
    int32 LegacyDeadlocks=0;const double Started=FPlatformTime::Seconds();
    for(int32 Team=0;Team<2;++Team)for(int32 Context=0;Context<3;++Context)for(EFrontlineVehicle Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918})for(bool Legacy:{true,false})
    {
        if(FocusDiagnostic&&(Team!=FocusTeam||Context!=FocusContext||int32(Type)!=FocusModel))continue;
        if(RegressionReview&&(Legacy||Team!=1||!((Context==0&&Type==EFrontlineVehicle::Whippet)
            ||(Context==2&&Type==EFrontlineVehicle::MarkIVMale))))continue;
        Clear();FrontlineSetLegacyArmourYieldV84411(Legacy);const int32 Dir=B->GetTeamDirection(Team),Trench=Context==2?B->GetHomeTrenchIndex(Team):B->Trenches.Num()/2;
        NextSerial=8441100+Team*100+Context*20+int32(Type)*2;
        const FString Name=FString::Printf(TEXT("v84411_armour_t%d_context%d_model%d_%s"),Team,Context,int32(Type),Legacy?TEXT("prior"):TEXT("current"));
        Check(Name+TEXT("_authored_trench"),B->Trenches.IsValidIndex(Trench));if(!B->Trenches.IsValidIndex(Trench))continue;
        B->Trenches[Trench].Owner=Team;
        for(int32 S=0;S<FFrontlineTrench::Capacity;++S)Section(Team,Trench,S,Context==1||Context==2&&S==1?EFrontlineRole::Lewis:EFrontlineRole::Rifle,8441100+Team*100+Context*10+S);
        for(int32 F=0;F<200;++F){B->Elapsed+=.05f;InfantryTick(.05f);}const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,0);
        auto* V=NewTank(Type,Team,FVector(Bank-Dir*1600.f,0,0));Check(Name+TEXT("_actual_chassis_and_complete_held_sections"),V&&V->AreAssetsReady()&&B->Squads.Num()==3&&People.Num()>=9);if(!V)continue;
        // The only difference among contexts is the actual held section mix,
        // authored trench and a live disabled hull/wreck. No advance is sent.
        if(Context==2){auto* Other=NewTank(EFrontlineVehicle::A7V,Team,FVector(Bank+Dir*150.f,620.f,0));if(Other)Other->TrackIntegrity[0]=0;
            auto* Wreck=NewTank(EFrontlineVehicle::Whippet,Team,FVector(Bank+Dir*1000.f,-620.f,0));if(Wreck){Wreck->HullIntegrity=0;Wreck->WreckAge=20;}}
        const FVector Start=V->GetActorLocation();float Time=0,Stall=0,MaxStall=0,MaxStep=0;int32 Blocked=0,Overlap=0,GateChanged=0;FString LastGate;TArray<TSharedPtr<FJsonValue>> Samples;
        TSharedPtr<FJsonObject> AssistantWait;TArray<TSharedPtr<FJsonValue>> FirstPhysicalBlock,LastPhysicalBlock;
        TMap<AWarfareFrontlineSoldier*,FVector> ActiveLeases,PreviousFeet;for(auto* U:People)PreviousFeet.Add(U,U->Feet());
        int32 LeaseAcquired=0,LeaseReleased=0,LeaseGoalChanged=0,RearSteps=0,RearReturns=0;float RearDistance=0;bool Traced=false;
        while(Time<40.f&&(V->GetActorLocation().X-Bank)*Dir<1100.f){const float DT=Samples.Num()%11==0?.10f:1.f/30.f;const FVector Before=V->GetActorLocation();B->Elapsed+=DT;Time+=DT;InfantryTick(DT);Tick(DT);
            for(auto* U:People){FVector Lease;const bool Held=FrontlineReadArmourYieldV84411(U,Lease);const FVector* Old=ActiveLeases.Find(U);
                if(Held&&!Old)++LeaseAcquired;else if(!Held&&Old)++LeaseReleased;else if(Held&&Old&&FVector::Dist2D(*Old,Lease)>.01f)++LeaseGoalChanged;
                const float Rear=float((U->Feet().X-PreviousFeet[U].X)*Dir);if(Rear<-.5f){++RearSteps;RearDistance-=Rear;if(!Held)++RearReturns;}
                PreviousFeet[U]=U->Feet();if(Held)ActiveLeases.Add(U,Lease);else ActiveLeases.Remove(U);}
            const float Move=FVector::Dist2D(Before,V->GetActorLocation()),Forward=(V->GetActorLocation().X-Before.X)*Dir;MaxStep=FMath::Max(MaxStep,Move);
            const bool Requested=V->MobilityRequestedV839>0.f;Stall=Requested&&Forward<.05f?Stall+DT:0.f;MaxStall=FMath::Max(MaxStall,Stall);
            const FString Gate=V->MobilityGateV839;Blocked+=Gate==TEXT("physical_route_blocked");if(Gate!=LastGate){++GateChanged;LastGate=Gate;}
            if(Gate==TEXT("physical_route_blocked")&&(FirstPhysicalBlock.IsEmpty()||Time>=Samples.Num()*.5f))
            {
              LastPhysicalBlock.Reset();for(auto* U:People)
              {
                const FVector Position=V->GetActorLocation(),Facing=FRotator(0,V->HeadingYawV37,0).Vector();
                const FVector Next=Position+Facing*V->MobilityRequestedV839;
                const bool Stops=V->BlocksInfantryAtV8218(U->Feet(),Next,V->HeadingYawV37,34.f,DT,U);
                if(!Stops)continue;
                auto E=MakeShared<FJsonObject>();FVector SidePoint=FVector::ZeroVector;const bool Yields=V->InfantryYieldPointV8218(U,SidePoint);
                E->SetNumberField(TEXT("at_seconds"),Time);E->SetNumberField(TEXT("squad"),U->SquadId);E->SetNumberField(TEXT("member"),U->MemberIndex);E->SetNumberField(TEXT("role"),int32(U->Role));
                E->SetStringField(TEXT("feet"),U->Feet().ToString());E->SetStringField(TEXT("head"),U->Soldier->GetBoneLocation(TEXT("Head")).ToString());
                E->SetStringField(TEXT("next_hull_pose"),V->GroundPoseForContactV8218(Next,V->HeadingYawV37,DT).ToString());
                E->SetBoolField(TEXT("current_hull_overlap"),V->BlocksInfantryAtV8218(U->Feet(),Position,V->HeadingYawV37,34.f,0.f,U));
                E->SetBoolField(TEXT("moving"),U->bMoving);E->SetBoolField(TEXT("wants_movement"),FFrontlineArmourMobilityAccessV84411::WantsMovement(U));E->SetBoolField(TEXT("is_main_gunner"),U->IsMachineGunner());
                if(auto Receipt=FrontlineReadArmourStepV84411(U))E->SetObjectField(TEXT("actual_step_receipt"),Receipt);
                const auto* G=FrontlineMachinegunsV30::Get(U);E->SetBoolField(TEXT("is_supported"),G&&G->IsSupported());E->SetBoolField(TEXT("yield_available"),Yields);E->SetStringField(TEXT("yield_candidate"),SidePoint.ToString());
                E->SetBoolField(TEXT("yield_candidate_allows_trench_step"),!SidePoint.IsNearlyZero()&&FrontlineInfantryV45::AllowsTrenchStep(U,U->Feet(),SidePoint));
                FVector LeasePoint;const bool HasLease=FrontlineReadArmourYieldV84411(U,LeasePoint);E->SetBoolField(TEXT("active_yield_lease"),HasLease);E->SetStringField(TEXT("actual_yield_lease_point"),LeasePoint.ToString());E->SetStringField(TEXT("actor_rotation"),U->GetActorRotation().ToString());
                if(HasLease)
                {
                    E->SetBoolField(TEXT("cached_goal_current_hull_clear"),!V->BlocksInfantryAtV8218(LeasePoint,Position,float(V->GetActorRotation().Yaw),34.f,0.f,U));
                    E->SetBoolField(TEXT("cached_goal_future_clear"),V->IsInfantryYieldGoalClearV84411(U,LeasePoint));
                    E->SetBoolField(TEXT("cached_goal_body_clear"),FrontlineArmourYieldGoalBodyClearV84411(U,LeasePoint));
                    if(const auto* Held=B->FindSquad(U->SquadId);Held&&B->Trenches.IsValidIndex(Held->Trench))
                    {
                        FVector OldFloor=U->Feet(),NewFloor=LeasePoint;
                        OldFloor.X=FrontlineFieldV32::TrenchCentreX(Held->Trench,OldFloor.Y);
                        NewFloor.X=FrontlineFieldV32::TrenchCentreX(Held->Trench,NewFloor.Y);
                        E->SetNumberField(TEXT("actual_floor_height_cm"),U->Feet().Z-FrontlineInfantryV45::SurfaceHeight(B,OldFloor));
                        E->SetNumberField(TEXT("cached_goal_floor_height_cm"),LeasePoint.Z-FrontlineInfantryV45::SurfaceHeight(B,NewFloor));
                    }
                    const double PriorSide=FVector::DotProduct(LeasePoint-U->Feet(),V->GetActorRightVector());
                    E->SetNumberField(TEXT("cached_to_fresh_side_dot"),FVector::DotProduct(SidePoint-U->Feet(),V->GetActorRightVector())*(PriorSide>0.?1.:PriorSide<0.?-1.:0.));
                }
                TArray<TSharedPtr<FJsonValue>> Neighbours;for(const auto* Other:People)if(Other!=U&&Other->IsAlive()&&FVector::Dist2D(U->Feet(),Other->Feet())<340.f){auto N=MakeShared<FJsonObject>();N->SetNumberField(TEXT("squad"),Other->SquadId);N->SetNumberField(TEXT("member"),Other->MemberIndex);N->SetStringField(TEXT("feet"),Other->Feet().ToString());N->SetNumberField(TEXT("gap_cm"),FVector::Dist2D(U->Feet(),Other->Feet()));Neighbours.Add(MakeShared<FJsonValueObject>(N));}E->SetArrayField(TEXT("actual_nearby_people"),Neighbours);
                if(FocusDiagnostic&&!Legacy&&!Traced&&Stall>8.f){Traced=true;FrontlineMobilityV41::TraceNextStepV64(U);UE_LOG(LogTemp,Display,TEXT("V84411_ACTUAL_BLOCKER_TRACE case=%s squad=%d member=%d elapsed=%.3f activelease=%d lease=%s"),*Name,U->SquadId,U->MemberIndex,Time,HasLease,*LeasePoint.ToString());}
                if(const auto* Q=B->FindSquad(U->SquadId);Q&&B->Trenches.IsValidIndex(Q->Trench)&&!SidePoint.IsNearlyZero())
                {FVector Floor=SidePoint;Floor.X=FrontlineFieldV32::TrenchCentreX(Q->Trench,Floor.Y)+U->Feet().X-FrontlineFieldV32::TrenchCentreX(Q->Trench,U->Feet().Y);Floor.Z=FrontlineInfantryV45::SurfaceHeight(B,Floor);
                    E->SetStringField(TEXT("contour_preserving_yield_candidate"),Floor.ToString());E->SetBoolField(TEXT("contour_yield_allows_trench_step"),FrontlineInfantryV45::AllowsTrenchStep(U,U->Feet(),Floor));}
                LastPhysicalBlock.Add(MakeShared<FJsonValueObject>(E));
              }
              if(FirstPhysicalBlock.IsEmpty())FirstPhysicalBlock=LastPhysicalBlock;
            }
            if(Legacy&&!AssistantWait&&Gate==TEXT("physical_route_blocked"))for(auto* U:People)
            {
                const auto* G=FrontlineMachinegunsV30::Get(U);if(!G||!G->IsSupported()||U->IsMachineGunner())continue;
                FVector OldPoint,NewPoint;const bool Original=V->InfantryYieldPointV8218(U,OldPoint);
                FrontlineSetLegacyArmourYieldV84411(false);const bool Revised=V->InfantryYieldPointV8218(U,NewPoint);FrontlineSetLegacyArmourYieldV84411(true);
                if(!Original&&Revised&&!V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37,50.f,0.f,U))
                {
                    AssistantWait=MakeShared<FJsonObject>();AssistantWait->SetNumberField(TEXT("at_seconds"),Time);AssistantWait->SetStringField(TEXT("feet"),U->Feet().ToString());
                    AssistantWait->SetStringField(TEXT("hull"),V->GetActorTransform().ToString());AssistantWait->SetNumberField(TEXT("squad"),U->SquadId);AssistantWait->SetNumberField(TEXT("member"),U->MemberIndex);
                    AssistantWait->SetStringField(TEXT("clear_side_destination"),NewPoint.ToString());AssistantWait->SetBoolField(TEXT("current_hull_overlap"),false);
                    AssistantWait->SetBoolField(TEXT("old_supported_assistant_yields"),false);AssistantWait->SetBoolField(TEXT("revised_supported_assistant_yields"),true);break;
                }
            }
            for(auto* U:People)Overlap+=U->IsAlive()&&V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37,34.f,0.f,U);
            if(Samples.IsEmpty()||Time>=Samples.Num()*.5f){auto S=V->DescribeMobilityV839();S->SetNumberField(TEXT("relative_seconds"),Time);S->SetNumberField(TEXT("net_forward_cm"),(V->GetActorLocation().X-Start.X)*Dir);Samples.Add(MakeShared<FJsonValueObject>(S));}}
        auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("duration"),Time);J->SetNumberField(TEXT("bank_progress_cm"),(V->GetActorLocation().X-Bank)*Dir);J->SetNumberField(TEXT("longest_requested_stall_seconds"),MaxStall);J->SetNumberField(TEXT("physical_blocked_frames"),Blocked);J->SetNumberField(TEXT("actual_body_overlaps"),Overlap);J->SetNumberField(TEXT("gate_changes"),GateChanged);J->SetNumberField(TEXT("maximum_step_cm"),MaxStep);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
        J->SetBoolField(TEXT("legacy_trench_yield_rule"),Legacy);
        J->SetNumberField(TEXT("yield_lease_acquisitions"),LeaseAcquired);J->SetNumberField(TEXT("yield_lease_releases"),LeaseReleased);J->SetNumberField(TEXT("active_yield_goal_changes"),LeaseGoalChanged);
        J->SetNumberField(TEXT("infantry_rear_steps"),RearSteps);J->SetNumberField(TEXT("infantry_rear_return_steps_without_lease"),RearReturns);J->SetNumberField(TEXT("infantry_rear_distance_cm"),RearDistance);
        J->SetArrayField(TEXT("first_actual_forward_body_blockers"),FirstPhysicalBlock);
        J->SetArrayField(TEXT("last_actual_forward_body_blockers"),LastPhysicalBlock);
        if(AssistantWait)J->SetObjectField(TEXT("exact_supported_assistant_wait_witness"),AssistantWait);
        if(Legacy){LegacyDeadlocks+=(V->GetActorLocation().X-Bank)*Dir<=650.f||MaxStall>=3.f;}
        else{
            Check(Name+TEXT("_crosses_occupied_trench_without_public_departure"),(V->GetActorLocation().X-Bank)*Dir>650.f&&B->AdvanceCommands==Advances);
            Check(Name+TEXT("_no_sustained_physical_deadlock_or_infantry_clipping"),MaxStall<3.f&&Overlap==0&&!V->GetActorTransform().ContainsNaN());
        }
    }
    // Two active friends use normal fleet routing. The faster rear chassis
    // must find a passing lane while the complete held sections stay put.
    if(!FocusDiagnostic)for(int32 Team=0;Team<2;++Team)
    {
        Clear();FrontlineSetLegacyArmourYieldV84411(false);NextSerial=8441170+Team*10;const int32 Dir=B->GetTeamDirection(Team),Trench=B->Trenches.Num()/2;
        B->Trenches[Trench].Owner=Team;for(int32 S=0;S<FFrontlineTrench::Capacity;++S)Section(Team,Trench,S,EFrontlineRole::Rifle,8441180+Team*10+S);
        for(int32 F=0;F<200;++F){B->Elapsed+=.05f;InfantryTick(.05f);}const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,0);
        auto* Lead=NewTank(EFrontlineVehicle::MarkIVMale,Team,FVector(Bank-Dir*1300.f,0,0));auto* Follower=NewTank(EFrontlineVehicle::Whippet,Team,FVector(Bank-Dir*2700.f,0,0));
        const FString Name=FString::Printf(TEXT("v84411_two_live_friendly_t%d"),Team);Check(Name+TEXT("_real_chassis"),Lead&&Follower&&Lead->AreAssetsReady()&&Follower->AreAssetsReady());if(!Lead||!Follower)continue;
        float Time=0,Stall[2]={},MaxStall[2]={},MaxSeparationY=0;int32 BodyOverlap=0;TArray<TSharedPtr<FJsonValue>> Samples;
        while(Time<60.f&&(FMath::Min(float(Lead->GetActorLocation().X*Dir),float(Follower->GetActorLocation().X*Dir))-Bank*Dir)<650.f)
        {
            const float DT=1.f/30.f;const FVector Before[2]={Lead->GetActorLocation(),Follower->GetActorLocation()};B->Elapsed+=DT;Time+=DT;InfantryTick(DT);Tick(DT);
            AFrontlineTank* Pair[2]={Lead,Follower};for(int32 I=0;I<2;++I){const float Forward=float((Pair[I]->GetActorLocation().X-Before[I].X)*Dir);
                Stall[I]=Pair[I]->MobilityRequestedV839>0&&Forward<.05f?Stall[I]+DT:0.f;MaxStall[I]=FMath::Max(MaxStall[I],Stall[I]);
                for(auto* U:People)BodyOverlap+=U->IsAlive()&&Pair[I]->BlocksInfantryAtV8218(U->Feet(),Pair[I]->GetActorLocation(),Pair[I]->HeadingYawV37,34.f,0.f,U);}
            MaxSeparationY=FMath::Max(MaxSeparationY,float(FMath::Abs(Lead->GetActorLocation().Y-Follower->GetActorLocation().Y)));
            if(Samples.IsEmpty()||Time>=Samples.Num()){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("seconds"),Time);J->SetObjectField(TEXT("lead"),Lead->DescribeMobilityV839());J->SetObjectField(TEXT("follower"),Follower->DescribeMobilityV839());Samples.Add(MakeShared<FJsonValueObject>(J));}
        }
        const float LeadProgress=float((Lead->GetActorLocation().X-Bank)*Dir),FollowerProgress=float((Follower->GetActorLocation().X-Bank)*Dir);auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("duration"),Time);
        J->SetNumberField(TEXT("lead_bank_progress_cm"),LeadProgress);J->SetNumberField(TEXT("follower_bank_progress_cm"),FollowerProgress);J->SetNumberField(TEXT("lead_requested_stall_seconds"),MaxStall[0]);J->SetNumberField(TEXT("follower_requested_stall_seconds"),MaxStall[1]);
        J->SetNumberField(TEXT("maximum_actual_lateral_separation_cm"),MaxSeparationY);J->SetNumberField(TEXT("actual_body_overlaps"),BodyOverlap);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
        for(AFrontlineTank* V:{Lead,Follower})
        {
            TArray<TSharedPtr<FJsonValue>> Blockers;const FVector P=V->GetActorLocation();
            const FVector Next=P+FRotator(0,V->HeadingYawV37,0).Vector()*V->MobilityRequestedV839;
            for(auto* U:People)if(U->IsAlive()&&V->BlocksInfantryAtV8218(U->Feet(),Next,V->HeadingYawV37,34.f,1.f/30.f,U))
            {
                auto E=MakeShared<FJsonObject>();E->SetNumberField(TEXT("squad"),U->SquadId);E->SetNumberField(TEXT("member"),U->MemberIndex);
                E->SetStringField(TEXT("feet"),U->Feet().ToString());E->SetBoolField(TEXT("moving"),U->bMoving);
                E->SetBoolField(TEXT("wants_movement"),FFrontlineArmourMobilityAccessV84411::WantsMovement(U));
                if(auto R=FrontlineReadArmourStepV84411(U))E->SetObjectField(TEXT("actual_step_receipt"),R);
                FVector Q;const bool Lease=FrontlineReadArmourYieldV84411(U,Q);E->SetBoolField(TEXT("active_yield_lease"),Lease);
                if(Lease){E->SetStringField(TEXT("actual_yield_lease_point"),Q.ToString());E->SetBoolField(TEXT("cached_direct_route_clear"),FrontlineArmourYieldDirectPathClearV84411(U,Q));}
                Blockers.Add(MakeShared<FJsonValueObject>(E));
            }
            J->SetArrayField(V==Lead?TEXT("lead_last_actual_forward_body_blockers"):TEXT("follower_last_actual_forward_body_blockers"),Blockers);
        }
        Check(Name+TEXT("_both_cross_held_trench_without_departure"),LeadProgress>650.f&&FollowerProgress>650.f&&B->AdvanceCommands==Advances);
        Check(Name+TEXT("_no_sustained_requested_deadlock_or_body_overlap"),MaxStall[0]<3.f&&MaxStall[1]<3.f&&BodyOverlap==0);
        J->SetBoolField(TEXT("opened_full_lateral_passing_clearance"),MaxSeparationY>Lead->FootprintHalfWidthV8311()+Follower->FootprintHalfWidthV8311());
    }
    // Opposing heavy tanks acquire real mounted arcs, stop for discharge and
    // apply in-flight shells to the actual receivers. Stop before a wreck is
    // required, so this does not invoke an unrelated victory/casualty fixture.
    Clear();FrontlineSetLegacyArmourYieldV84411(false);NextSerial=8441190;
    if(!FocusDiagnostic&&!RegressionReview)
    {
        const int32 Trench=B->Trenches.Num()/2;const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,0);
        auto* UK=NewTank(EFrontlineVehicle::MarkIVMale,0,FVector(Bank-1800.f,0,0));auto* DE=NewTank(EFrontlineVehicle::A7V,1,FVector(Bank+1800.f,0,0));
        Check(TEXT("v84411_opposing_live_armour_real_chassis"),UK&&DE&&UK->AreAssetsReady()&&DE->AreAssetsReady());
        if(UK&&DE){for(float& C:UK->CannonClock)C=0.f;for(float& C:DE->CannonClock)C=0.f;
            const float Initial[2]={UK->HullIntegrity+UK->TrackIntegrity[0]+UK->TrackIntegrity[1],DE->HullIntegrity+DE->TrackIntegrity[0]+DE->TrackIntegrity[1]};
            float Time=0,Stall[2]={},MaxStall[2]={};bool HaltedForMount[2]={};TArray<TSharedPtr<FJsonValue>> Samples;
            while(Time<60.f&&UK->IsOperational()&&DE->IsOperational())
            {
                const float DT=1.f/30.f;B->Elapsed+=DT;Time+=DT;Tick(DT);AFrontlineTank* Pair[2]={UK,DE};
                for(int32 I=0;I<2;++I){const auto* V=Pair[I];Stall[I]=V->MobilityRequestedV839>0&&V->MobilityMovedV839<.05f?Stall[I]+DT:0.f;MaxStall[I]=FMath::Max(MaxStall[I],Stall[I]);
                    HaltedForMount[I]|=FCString::Strcmp(V->MobilityGateV839,TEXT("observed_mounted_contact"))==0;}
                if(Samples.IsEmpty()||Time>=Samples.Num()*.5f){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("seconds"),Time);J->SetObjectField(TEXT("uk"),UK->DescribeMobilityV839());J->SetObjectField(TEXT("de"),DE->DescribeMobilityV839());Samples.Add(MakeShared<FJsonValueObject>(J));}
                const float AppliedUK=Initial[0]-UK->HullIntegrity-UK->TrackIntegrity[0]-UK->TrackIntegrity[1],AppliedDE=Initial[1]-DE->HullIntegrity-DE->TrackIntegrity[0]-DE->TrackIntegrity[1];
                if(UK->CannonShots>0&&DE->CannonShots>0&&AppliedUK>0&&AppliedDE>0)break;
            }
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),TEXT("v84411_opposing_live_armour"));J->SetNumberField(TEXT("duration"),Time);J->SetObjectField(TEXT("uk"),UK->DescribeMobilityV839());J->SetObjectField(TEXT("de"),DE->DescribeMobilityV839());
            const float AppliedUK=Initial[0]-UK->HullIntegrity-UK->TrackIntegrity[0]-UK->TrackIntegrity[1],AppliedDE=Initial[1]-DE->HullIntegrity-DE->TrackIntegrity[0]-DE->TrackIntegrity[1];
            J->SetNumberField(TEXT("applied_damage_to_uk"),AppliedUK);J->SetNumberField(TEXT("applied_damage_to_de"),AppliedDE);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
            Check(TEXT("v84411_opposing_live_armour_both_mounted_fire_and_actual_shell_damage"),UK->CannonShots>0&&DE->CannonShots>0&&AppliedUK>0&&AppliedDE>0);
            Check(TEXT("v84411_opposing_live_armour_legitimate_firing_stop_not_route_deadlock"),HaltedForMount[0]&&HaltedForMount[1]&&MaxStall[0]<3.f&&MaxStall[1]<3.f);
        }
    }
    Report->SetNumberField(TEXT("prior_rule_deadlock_witnesses"),LegacyDeadlocks);Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
    if(!RegressionReview)Check(TEXT("v84411_prior_trench_yield_rule_has_real_deadlock_witness"),LegacyDeadlocks>0);
    Report->SetStringField(TEXT("scope"),RegressionReview?TEXT("Focused four-case regression only: current UK and DE two-live-friendly pairs, current DE Whippet at the central held rifle trench, and current DE Mark IV at the home mixed rifle/MG trench with disabled friendly armour and wreck. The original crossing, requested-stall below 3 seconds, body-overlap, no-public-departure and finite-pose gates remain unchanged. No prior branch or opposing duel is repeated; this is not full mobility acceptance."):FocusDiagnostic?*FString::Printf(TEXT("Partial diagnosis only: prior/current team %d context %d model %d occupied authored-trench fixture with actual blocked-step receipts. This is not full mobility acceptance."),FocusTeam,FocusContext,FocusModel):TEXT("18 paired UK/DE actual chassis/fleet-Tick trajectories against three complete normally scheduled held rifle/MG sections, natural skeletal evaluation and supported poses, central/home authored banks, disabled friendly armour and persistent wreck. Prior trench-yield rule is isolated to the before branch. Two active same-direction friendly pairs and one opposing heavy-armour duel use real fleet routing and mounted shells. No public infantry departure or in-trajectory teleport; movement fixtures delay damage clocks. Not a full campaign, all-map or art acceptance claim."));
    return Report;
}
