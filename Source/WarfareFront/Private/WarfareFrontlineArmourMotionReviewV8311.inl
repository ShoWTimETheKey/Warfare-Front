// Receiver fixture uses the production fleet substep, terrain and art actors.
static TSharedRef<FJsonObject> RunMotionV8311(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Cases;bool Good=true;FString User;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
    Assert(TEXT("v8311_motion_disposable_ukde_receiver"),Safe);
    auto* Fleet=Safe?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
    if(!Fleet){Report->SetBoolField(TEXT("success"),false);return Report;}
    const auto SavedVehicles=Fleet->Vehicles;const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;
    const auto SavedPhase=B->Phase;const int32 SavedWinner=B->BattleOutroWinner,Serial=Fleet->NextSerial;
    const float Clock=B->Elapsed,EnemyAt=Fleet->NextEnemyAt;const FGuid Encounter=Fleet->LastEncounter;
    const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;const FString Message=B->LastMessage;
    const float Morale[]={B->Morale[0],B->Morale[1]};TSet<AActor*> ExistingImpacts;
    for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)ExistingImpacts.Add(*It);
    const int32 Deployment[]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
    const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AFrontlineTank* V:SavedVehicles)Hide(V);for(AWarfareFrontlineSoldier* U:SavedUnits)Hide(U);
    B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Tracers.Reset();Fleet->Vehicles.Reset();
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;Fleet->LastEncounter=B->GetEncounterId();Fleet->NextEnemyAt=MAX_flt;
    UGameplayStatics::SetGamePaused(B,false);
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> MadeInfantry;
    const auto Clear=[&](){for(AFrontlineTank* V:Fleet->Vehicles)if(IsValid(V))V->Destroy();Fleet->Vehicles.Reset();
        for(auto& W:MadeInfantry)if(auto* U=W.Get())U->Destroy();MadeInfantry.Reset();B->Units.Reset();B->Squads.Reset();};
    ON_SCOPE_EXIT
    {
        Clear();Fleet->Vehicles=SavedVehicles;B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=Smoke;B->Tracers=Tracers;
        B->Phase=SavedPhase;B->BattleOutroWinner=SavedWinner;B->Elapsed=Clock;B->LastMessage=Message;B->Morale[0]=Morale[0];B->Morale[1]=Morale[1];
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)if(!ExistingImpacts.Contains(*It))It->Destroy();
        Fleet->NextSerial=Serial;Fleet->NextEnemyAt=EnemyAt;Fleet->LastEncounter=Encounter;
        for(int32 T=0;T<2;++T)Fleet->DeploymentCountV37[T]=Deployment[T];
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        UGameplayStatics::SetGamePaused(B,Paused);Report->SetArrayField(TEXT("cases"),Cases);Report->SetBoolField(TEXT("success"),Good);
        Report->SetStringField(TEXT("scope"),TEXT("Actual fleet Tick: Austin, Whippet and long Mark V* pass live disabled hulls and wrecks near an authored trench; two moving friendly hulls pass, and opposing cannon hulls retain their real engagement. No substitute kinematics, all-map or human-play claim."));
    };
    const auto Place=[&](AFrontlineTank* V,const FVector& P)
    {
        if(!V)return;V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;
        V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->RouteDecisionAtV37=0;V->RouteStallSecondsV837=0;V->bEntered=true;V->FitGroundV35(P,0);
    };
    const int32 Centre=B->Trenches.Num()/2;
    Assert(TEXT("v8311_motion_prepared_real_bank"),B->Trenches.IsValidIndex(Centre));if(!B->Trenches.IsValidIndex(Centre))return Report;
    const int32 British=B->GetFactionForTeam(0)==EFrontlineFaction::British?0:1,Dir=B->GetTeamDirection(British);
    for(const auto Type:{EFrontlineVehicle::Austin1918,EFrontlineVehicle::Whippet,EFrontlineVehicle::MarkVStar})
    for(int32 Dead=0;Dead<2;++Dead)
    {
        Clear();auto* V=Fleet->Spawn(Type,British,true);auto* Other=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,British,true);
        const FString Prefix=FString::Printf(TEXT("v8311_model%d_%s_"),int32(Type),Dead?TEXT("wreck"):TEXT("live"));
        Assert(Prefix+TEXT("assets"),V&&Other&&V->AreAssetsReady()&&Other->AreAssetsReady());if(!V||!Other)continue;
        const float Bank=FrontlineFieldV32::TrenchCentreX(Centre,-300.f),BlockX=Bank-Dir*900.f;
        Place(V,FVector(BlockX-Dir*1800.f,-300,0));Place(Other,FVector(BlockX,-300,0));
        Other->TrackIntegrity[0]=0;
        if(Dead){Other->HullIntegrity=0;Other->Wreck();}
        const FVector Start=V->GetActorLocation();float MaxOverlap=0,MaxTurn=0,Travel=0,MaxAcross=0;int32 Still=0,OverlapSteps=0;
        bool Finite=true,WithinMotion=true,WheelSteers=false,Differential=false;TArray<TSharedPtr<FJsonValue>> Path;
        for(int32 Frame=0;Frame<600&&(V->GetActorLocation().X-Start.X)*Dir<3200.f;++Frame)
        {
            const FVector Before=V->GetActorLocation();const float Yaw=V->HeadingYawV37;
            B->Elapsed+=.05f;Fleet->Tick(.05f);
            const FVector After=V->GetActorLocation();const float Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,V->HeadingYawV37));
            const float Step=FVector::Dist2D(Before,After),Pen=V->ArmourOverlapV8311(After,V->HeadingYawV37,Other);
            MaxOverlap=FMath::Max(MaxOverlap,Pen);OverlapSteps+=Pen>.01f;MaxTurn=FMath::Max(MaxTurn,Turn);Travel+=Step;Still+=Step<.01f;
            MaxAcross=FMath::Max(MaxAcross,float(FMath::Abs(After.Y-Start.Y)));Finite&=!V->GetActorTransform().ContainsNaN();
            WithinMotion&=(After.X-Before.X)*Dir>=-.01f&&FMath::Abs(After.Y)<=980.1f
                &&Turn<=V->SteeringRateV8311(UFrontlineVehicleSubsystem::Definition(Type).SpeedCm*FrontlineArmourV63::TravelSpeedScaleV8218(B,V->Team),false)*.0501f+.01f;
            WheelSteers|=V->IsWheeledV8218()&&FMath::Abs(V->WheelSteerV8311)>1.f;
            Differential|=!V->IsWheeledV8218()&&FMath::Abs(V->TrackSideOffsetV8311[0]-V->TrackSideOffsetV8311[1])>10.f;
            if(Frame%10==0||Pen>.01f)
            {auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("step"),Frame);Row->SetNumberField(TEXT("host_seconds"),B->Elapsed);
                Row->SetNumberField(TEXT("x"),After.X);Row->SetNumberField(TEXT("y"),After.Y);Row->SetNumberField(TEXT("heading_deg"),V->HeadingYawV37);
                Row->SetNumberField(TEXT("chosen_lane_cm"),V->DesiredLaneV37);Row->SetNumberField(TEXT("overlap_cm"),Pen);Path.Add(MakeShared<FJsonValueObject>(Row));}
        }
        const float Progress=(V->GetActorLocation().X-Start.X)*Dir;
        Assert(Prefix+TEXT("finite_forward_grounded_progress"),Finite&&WithinMotion&&Progress>2400.f&&V->IsOperational()&&!V->IsImmobilized());
        Assert(Prefix+TEXT("clear_open_corridor_avoids_hull"),OverlapSteps==0&&MaxAcross>300.f);
        Assert(Prefix+TEXT("running_gear_expresses_real_turn"),V->IsWheeledV8218()?WheelSteers:Differential);
        auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),Dead?TEXT("soft_wreck"):TEXT("soft_live_hull"));
        J->SetNumberField(TEXT("progress_cm"),Progress);J->SetNumberField(TEXT("travel_cm"),Travel);J->SetNumberField(TEXT("maximum_overlap_cm"),MaxOverlap);
        J->SetNumberField(TEXT("overlap_steps"),OverlapSteps);J->SetNumberField(TEXT("stationary_steps"),Still);J->SetNumberField(TEXT("maximum_turn_step_deg"),MaxTurn);
        J->SetNumberField(TEXT("maximum_lane_change_cm"),MaxAcross);J->SetArrayField(TEXT("path"),Path);Cases.Add(MakeShared<FJsonValueObject>(J));
    }
    Clear();auto* Fast=Fleet->Spawn(EFrontlineVehicle::Whippet,British,true);auto* Lead=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,British,true);
    Assert(TEXT("v8311_friendly_moving_hulls_assets"),Fast&&Lead);if(Fast&&Lead)
    {
        const float Bank=FrontlineFieldV32::TrenchCentreX(Centre,-300.f);
        Place(Fast,FVector(Bank-Dir*3000.f,-300,0));Place(Lead,FVector(Bank-Dir*1700.f,-300,0));
        const FVector FastStart=Fast->GetActorLocation(),LeadStart=Lead->GetActorLocation();float MaxOverlap=0,MaxSeparation=0;
        bool Finite=true;
        for(int32 Frame=0;Frame<600;++Frame)
        {
            B->Elapsed+=.05f;Fleet->Tick(.05f);
            MaxOverlap=FMath::Max(MaxOverlap,Fast->ArmourOverlapV8311(Fast->GetActorLocation(),Fast->HeadingYawV37,Lead));
            MaxSeparation=FMath::Max(MaxSeparation,float(FMath::Abs(Fast->GetActorLocation().Y-Lead->GetActorLocation().Y)));
            Finite&=!Fast->GetActorTransform().ContainsNaN()&&!Lead->GetActorTransform().ContainsNaN();
        }
        const float FastProgress=(Fast->GetActorLocation().X-FastStart.X)*Dir,LeadProgress=(Lead->GetActorLocation().X-LeadStart.X)*Dir;
        Assert(TEXT("v8311_friendly_moving_hulls_pass_with_forward_progress"),Finite&&FastProgress>2500.f&&LeadProgress>1800.f&&MaxOverlap<=.01f&&MaxSeparation>300.f);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("case"),TEXT("two_friendly_moving_hulls"));
        J->SetNumberField(TEXT("follower_progress_cm"),FastProgress);J->SetNumberField(TEXT("leader_progress_cm"),LeadProgress);
        J->SetNumberField(TEXT("maximum_overlap_cm"),MaxOverlap);J->SetNumberField(TEXT("maximum_lateral_separation_cm"),MaxSeparation);Cases.Add(MakeShared<FJsonValueObject>(J));
    }
    Clear();auto* A=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,British,true);auto* E=Fleet->Spawn(EFrontlineVehicle::A7V,1-British,true);
    Assert(TEXT("v8311_opposing_hulls_assets"),A&&E);if(A&&E)
    {
        const float Bank=FrontlineFieldV32::TrenchCentreX(Centre,300.f);Place(A,FVector(Bank-Dir*1800.f,300,0));Place(E,FVector(Bank+Dir*1800.f,300,0));
        const FVector StartA=A->GetActorLocation(),StartE=E->GetActorLocation();int32 ObservedHalt=0;
        for(int32 F=0;F<120&&A->IsOperational()&&E->IsOperational();++F)
        {
            B->Elapsed+=.05f;Fleet->Tick(.05f);
            ObservedHalt+=FCString::Strcmp(A->MobilityGateV839,TEXT("observed_mounted_contact"))==0&&FCString::Strcmp(E->MobilityGateV839,TEXT("observed_mounted_contact"))==0;
        }
        Assert(TEXT("v8311_soft_avoidance_retains_opposing_cannon_engagement"),A->CannonShots>0&&E->CannonShots>0&&ObservedHalt>0);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("case"),TEXT("opposing_mounted_contact"));J->SetNumberField(TEXT("british_cannon_shots"),A->CannonShots);
        J->SetNumberField(TEXT("german_cannon_shots"),E->CannonShots);J->SetNumberField(TEXT("both_halted_steps"),ObservedHalt);
        J->SetNumberField(TEXT("british_travel_cm"),FVector::Dist2D(StartA,A->GetActorLocation()));J->SetNumberField(TEXT("german_travel_cm"),FVector::Dist2D(StartE,E->GetActorLocation()));Cases.Add(MakeShared<FJsonValueObject>(J));
    }
    const auto EnemyInfantry=[&](AFrontlineTank* V,const FVector& Feet,int32 SquadId)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        MadeInfantry.Add(U);U->Initialize(B,SquadId,1-V->Team,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;
        FVector P=Feet;P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));U->ReturnToIdle();B->Units.Add(U);
        FFrontlineSquad Q;Q.Id=SquadId;Q.Team=U->Team;Q.Role=EFrontlineRole::Rifle;Q.Members.Add(U);B->Squads.Add(Q);return U;
    };
    for(int32 Team=0;Team<2;++Team)
    {
        const int32 Direction=B->GetTeamDirection(Team);const float Lane=Direction>0?-300.f:300.f;
        const float Bank=FrontlineFieldV32::TrenchCentreX(Centre,Lane);const FString Prefix=FString::Printf(TEXT("v8311_austin_team%d_"),Team);
        Clear();auto* Car=Fleet->Spawn(EFrontlineVehicle::Austin1918,Team,true);
        Assert(Prefix+TEXT("front_rear_assets"),Car!=nullptr);if(!Car)continue;
        Place(Car,FVector(Bank-Direction*1700.f,Lane,0));const FVector Start=Car->GetActorLocation();
        auto* Rear=EnemyInfantry(Car,Start-FVector(Direction*650.f,0,0),8311200+Team*10);
        auto* Front=EnemyInfantry(Car,Start+FVector(Direction*1200.f,0,0),8311201+Team*10);
        int32 MountedHalt=0;float MoveBeforeShot=0;
        if(Front&&Rear)for(int32 F=0;F<160&&Car->MachinegunShots==0;++F)
        {
            B->Elapsed+=.05f;Fleet->Tick(.05f);MoveBeforeShot+=Car->MobilityMovedV839;
            MountedHalt+=FCString::Strcmp(Car->MobilityGateV839,TEXT("observed_mounted_contact"))==0;
        }
        Assert(Prefix+TEXT("rear_contact_cannot_displace_actual_front_mount"),Front&&Rear&&Car->MachinegunShots>0&&MountedHalt>0&&MoveBeforeShot<1.f);
        auto J=Car->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("austin_front_and_nearer_rear"));J->SetNumberField(TEXT("mounted_halt_steps"),MountedHalt);J->SetNumberField(TEXT("move_before_first_shot_cm"),MoveBeforeShot);Cases.Add(MakeShared<FJsonValueObject>(J));

        Clear();Car=Fleet->Spawn(EFrontlineVehicle::Austin1918,Team,true);if(!Car){Assert(Prefix+TEXT("edge_arc_assets"),false);continue;}
        Place(Car,FVector(Bank-Direction*1700.f,Lane,0));
        const float Bearing=FMath::DegreesToRadians(75.f);
        auto* Edge=EnemyInfantry(Car,Car->GetActorTransform().TransformPosition(Car->GunMounts[1]+FVector(FMath::Cos(Bearing)*1000.f,FMath::Sin(Bearing)*1000.f,-245.f)),8311202+Team*10);
        FVector Point;const bool ActualSolution=Edge&&Car->InfantryFiringSolutionV828(Edge,1,false,Point);
        const float ActualBearing=ActualSolution?FMath::Abs(FMath::FindDeltaAngleDegrees(Car->GunBaseYaw[1],float((Car->GetActorTransform().InverseTransformPosition(Point)-Car->GunMounts[1]).Rotation().Yaw))):0.f;
        const FVector EdgeStart=Car->GetActorLocation();const float EdgeYaw=Car->HeadingYawV37;
        MountedHalt=0;
        for(int32 F=0;F<160&&Car->MachinegunShots==0;++F){B->Elapsed+=.05f;Fleet->Tick(.05f);MountedHalt+=FCString::Strcmp(Car->MobilityGateV839,TEXT("observed_mounted_contact"))==0;}
        Assert(Prefix+TEXT("actual_55_to_60_degree_mount_does_not_require_hull_pivot"),ActualSolution&&ActualBearing>55.f&&ActualBearing<=60.f
            &&Car->MachinegunShots>0&&MountedHalt>0&&FVector::Dist2D(EdgeStart,Car->GetActorLocation())<1.f&&FMath::Abs(FMath::FindDeltaAngleDegrees(EdgeYaw,Car->HeadingYawV37))<.01f);
        J=Car->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("austin_real_mount_edge_band"));J->SetNumberField(TEXT("actual_mount_delta_deg"),ActualBearing);Cases.Add(MakeShared<FJsonValueObject>(J));

        Clear();Car=Fleet->Spawn(EFrontlineVehicle::Austin1918,Team,true);if(!Car){Assert(Prefix+TEXT("rear_end_assets"),false);continue;}
        const float End=FrontlineGameplayV49::BreakthroughLine()+Car->FootprintHalfLengthV37()+245.f;
        Place(Car,FVector(Direction*End,Lane,0));const FVector EndStart=Car->GetActorLocation();
        Rear=EnemyInfantry(Car,EndStart-FVector(Direction*1200.f,0,0),8311203+Team*10);
        const float EndBudget=FrontlineGameplayV49::BreakthroughLine()+FMath::Max(Car->FootprintHalfLengthV37(),float(EndStart.X*Direction)-Car->RearProgressV49())+240.f-EndStart.X*Direction;
        Assert(Prefix+TEXT("rear_fixture_really_defends_zero_forward_budget_end"),Rear&&FrontlineGameplayV49::EnemyEndDefended(B,Team)&&EndBudget<=0.f);
        bool GroundedFinite=true,NoStationaryPivot=true,WithinApron=true;float ArcTravel=0,MaxTurn=0;MountedHalt=0;TArray<TSharedPtr<FJsonValue>> ArcPath;
        for(int32 F=0;F<500&&Car->MachinegunShots==0;++F)
        {
            const FVector Before=Car->GetActorLocation();const float BeforeYaw=Car->HeadingYawV37;
            B->Elapsed+=.05f;Fleet->Tick(.05f);const FVector After=Car->GetActorLocation();const float Step=FVector::Dist2D(Before,After);
            const float Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(BeforeYaw,Car->HeadingYawV37));
            ArcTravel+=Step;MaxTurn=FMath::Max(MaxTurn,Turn);GroundedFinite&=!Car->GetActorTransform().ContainsNaN()&&Car->CanOccupyV48(After,Car->HeadingYawV37);
            NoStationaryPivot&=Turn<.001f||Step>.01f;
            WithinApron&=FMath::Abs(After.Y)<=980.1f&&FMath::Abs(After.X)<=FMath::Min(FrontlinePacingV24::CombatHalfLength()+550.f,FrontlineBattlefield::GroundHalfLength-600.f)+.1f;
            MountedHalt+=FCString::Strcmp(Car->MobilityGateV839,TEXT("observed_mounted_contact"))==0;
            if(F%10==0||Step<.01f)
            {auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("step"),F);Row->SetNumberField(TEXT("host_seconds"),B->Elapsed);
                Row->SetNumberField(TEXT("x"),After.X);Row->SetNumberField(TEXT("y"),After.Y);Row->SetNumberField(TEXT("heading_deg"),Car->HeadingYawV37);
                Row->SetNumberField(TEXT("moved_cm"),Step);Row->SetNumberField(TEXT("mg_shots"),Car->MachinegunShots);ArcPath.Add(MakeShared<FJsonValueObject>(Row));}
        }
        Assert(Prefix+TEXT("rear_end_defender_acquired_by_finite_grounded_rolling_arc"),Rear&&Car->MachinegunShots>0&&MountedHalt>0&&GroundedFinite&&WithinApron&&NoStationaryPivot&&ArcTravel>100.f);
        J=Car->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("austin_rear_only_defended_end"));J->SetNumberField(TEXT("rolling_arc_travel_cm"),ArcTravel);J->SetNumberField(TEXT("maximum_turn_step_deg"),MaxTurn);J->SetBoolField(TEXT("no_stationary_pivot"),NoStationaryPivot);J->SetArrayField(TEXT("path"),ArcPath);Cases.Add(MakeShared<FJsonValueObject>(J));
    }
    return Report;
}
