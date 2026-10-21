// Focused production API fixtures. This is not a second movement simulation.
TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunMobilityChecksV8445()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;};
    ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);Report->SetBoolField(TEXT("success"),Good);};
    auto* B=Battle();FString User;
    FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
    const bool Absolute=!User.IsEmpty()&&!FPaths::IsRelative(User);
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8445/Runtime"));FPaths::NormalizeDirectoryName(Root);
    const FString Stamp=User.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)?User.Mid(Root.Len()+1):FString();
    const bool DisposablePath=Absolute&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8445Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&DisposablePath;
    Check(TEXT("v8445_mobility_requires_disposable_ukde_world"),Safe);if(!Safe)return Report;
    const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;
    const auto SavedSmoke=B->SmokeZones;const auto SavedTracers=B->Tracers;const auto SavedPhase=B->Phase;
    const auto SavedEncounter=LastEncounter;const float Clock=B->Elapsed,EnemyAt=NextEnemyAt;
    const int32 SavedSerial=NextSerial,Winner=B->BattleOutroWinner,Count[2]={DeploymentCountV37[0],DeploymentCountV37[1]};
    const FString Message=B->LastMessage;const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AFrontlineTank* V:SavedVehicles)Hide(V);for(AWarfareFrontlineSoldier* U:SavedUnits)Hide(U);
    TSet<AActor*> OldEffects;for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)OldEffects.Add(*It);
    TArray<AFrontlineTank*> Fixtures;TArray<AWarfareFrontlineSoldier*> People;
    B->Units.Empty();B->Squads.Empty();B->SmokeZones.Empty();B->Tracers.Empty();Vehicles.Empty();
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;LastEncounter=B->GetEncounterId();NextEnemyAt=MAX_flt;
    UGameplayStatics::SetGamePaused(B,false);
    const auto Clear=[&](){for(auto* U:People)if(IsValid(U))U->Destroy();People.Empty();B->Units.Empty();
        for(auto* V:Fixtures)if(IsValid(V))V->Destroy();Fixtures.Empty();Vehicles.Empty();};
    ON_SCOPE_EXIT
    {
        Clear();Vehicles=SavedVehicles;B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedSmoke;B->Tracers=SavedTracers;
        B->Phase=SavedPhase;B->BattleOutroWinner=Winner;B->Elapsed=Clock;B->LastMessage=Message;
        LastEncounter=SavedEncounter;NextSerial=SavedSerial;NextEnemyAt=EnemyAt;
        for(int32 T=0;T<2;++T)DeploymentCountV37[T]=Count[T];
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)if(!OldEffects.Contains(*It))It->Destroy();
        UGameplayStatics::SetGamePaused(B,Paused);
    };
    const auto NewTank=[&](EFrontlineVehicle Type,int32 Team)
    {
        auto* V=GetWorld()->SpawnActor<AFrontlineTank>();
        if(!V)return (AFrontlineTank*)nullptr;
        if(!V->Configure(B,Type==EFrontlineVehicle::RailConvoy?EFrontlineVehicle::A7V:Type,Team,++NextSerial)
            ||(Type==EFrontlineVehicle::RailConvoy&&!V->ConfigureRailV32())){V->Destroy();return (AFrontlineTank*)nullptr;}
        Fixtures.Add(V);Vehicles.Add(V);
        // Keep the real observation and halt decision without killing fixtures.
        for(float& C:V->CannonClock)C=10000.f;for(float& C:V->GunClock)C=10000.f;
        return V;
    };
    const auto Place=[&](AFrontlineTank* V,const FVector& P)
    {
        const int32 Dir=B->GetTeamDirection(V->Team);V->HeadingYawV37=Dir>0?0.f:180.f;
        V->PreferredLaneV37=V->DesiredLaneV37=P.Y;V->RouteDecisionAtV37=0;V->RouteStallSecondsV837=0;
        V->bEntered=true;V->FitGroundV35(P,0);
    };
    const auto NewPerson=[&](int32 Team,const FVector& P)
    {
        auto* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return (AWarfareFrontlineSoldier*)nullptr;
        U->Initialize(B,8445000+People.Num(),Team,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
        U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88.f));
        People.Add(U);B->Units.Add(U);return U;
    };
    // The classification is mass/protection, independently of the old Heavy()
    // helper which only selects rhomboid track geometry (and excludes A7V).
    constexpr float OldHull[]={350,220,405,315,340,315,380,340,380,315,420,165,1450};
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
    {
        Clear();const auto Type=EFrontlineVehicle(I);auto* V=NewTank(Type,0);const bool Light=Type==EFrontlineVehicle::Whippet||Type==EFrontlineVehicle::Austin1918;
        const float ClassScale=Light?1.30f:1.60f;
        const float Research=Type==EFrontlineVehicle::RailConvoy?(B->HasTechnology(22,0)?1.22f:1.f):FrontlineArmourV63::HullScaleV8218(B,0);
        Check(FString::Printf(TEXT("v8445_model%d_actual_base_and_researched_hull"),I),V
            &&FMath::IsNearlyEqual(Definition(Type).Hull,OldHull[I]*ClassScale,.02f)
            &&FMath::IsNearlyEqual(V->MaximumHullIntegrity,OldHull[I]*ClassScale*Research,.02f)
            &&V->HullIntegrity==V->MaximumHullIntegrity);
        const float Track=Type==EFrontlineVehicle::RailConvoy?420.f:(Light&&Type==EFrontlineVehicle::Austin1918?95.f:180.f)*FrontlineArmourV63::TrackScale(B,0);
        Check(FString::Printf(TEXT("v8445_model%d_running_gear_hp_retained"),I),V&&FMath::IsNearlyEqual(V->TrackIntegrity[0],Track,.02f)&&V->TrackIntegrity[1]==V->TrackIntegrity[0]);
        if(V){auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("actual_hull_configuration"));
            J->SetStringField(TEXT("class"),Light?TEXT("light"):Type==EFrontlineVehicle::RailConvoy?TEXT("heavy_rail"):TEXT("heavy"));
            J->SetNumberField(TEXT("old_base_hull"),OldHull[I]);J->SetNumberField(TEXT("new_base_hull"),Definition(Type).Hull);
            J->SetNumberField(TEXT("research_multiplier"),Research);J->SetNumberField(TEXT("actual_maximum_hull"),V->MaximumHullIntegrity);Cases.Add(MakeShared<FJsonValueObject>(J));}
    }
    const int32 Trench=B->Trenches.Num()/2;Check(TEXT("v8445_authored_trench_present"),B->Trenches.IsValidIndex(Trench));
    if(!B->Trenches.IsValidIndex(Trench))return Report;
    int32 ReproducedClosestFaceStalls=0;
    for(int32 Team=0;Team<2;++Team)for(const auto Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::A7V,EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918})
    {
        Clear();auto* V=NewTank(Type,Team);const FString Prefix=FString::Printf(TEXT("v8445_model%d_direction%d_"),int32(Type),Team);
        Check(Prefix+TEXT("actual_assets_ready"),V&&V->AreAssetsReady());if(!V)continue;
        const int32 Dir=B->GetTeamDirection(Team);const float Lane=Team?-300.f:300.f;
        const float Bank=FrontlineFieldV32::TrenchCentreX(Trench,Lane);Place(V,FVector(Bank-Dir*900.f,Lane,0));
        const FVector Start=V->GetActorLocation();float Idle=0,MaxIdle=0,Travel=0,Requested=0;bool Finite=true,Bounded=true;
        for(int32 F=0;F<700&&(V->GetActorLocation().X-Bank)*Dir<1200.f;++F)
        {
            const FVector Before=V->GetActorLocation();B->Elapsed+=.05f;Tick(.05f);const float Step=FVector::Dist2D(Before,V->GetActorLocation());
            Idle=Step<.01f?Idle+.05f:0.f;MaxIdle=FMath::Max(MaxIdle,Idle);Travel+=Step;Requested+=V->MobilityRequestedV839;
            Finite&=!V->GetActorTransform().ContainsNaN();Bounded&=Step<=Definition(Type).SpeedCm*FrontlineArmourV63::TravelSpeedScaleV8218(B,Team)*.0501f+.01f
                &&(V->GetActorLocation().X-Before.X)*Dir>=-.01f&&FMath::Abs(V->GetActorLocation().Y)<=980.1f;
        }
        Check(Prefix+TEXT("no_enemy_actual_tick_crosses_bank_without_stall"),Finite&&Bounded&&MaxIdle<.5f&&(V->GetActorLocation().X-Bank)*Dir>1100.f);
        auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("no_enemy_production_tick_bank_crossing"));
        J->SetNumberField(TEXT("progress_cm"),(V->GetActorLocation().X-Start.X)*Dir);J->SetNumberField(TEXT("travel_cm"),Travel);
        J->SetNumberField(TEXT("requested_total_cm"),Requested);J->SetNumberField(TEXT("max_idle_seconds"),MaxIdle);Cases.Add(MakeShared<FJsonValueObject>(J));
        // Native standing feet at the approach deliberately seed an already
        // enclosed torso, the same condition ground/bay motion can introduce.
        Place(V,FVector(Bank-Dir*1200.f,Lane,0));V->RouteDecisionAtV37=100.f;
        auto* U=NewPerson(Team,V->GetActorLocation()-FVector(Dir*30.f,0,0));
        Check(Prefix+TEXT("seeded_existing_standing_body_overlap"),U&&V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37));
        if(U)
        {
            const FVector P=V->GetActorLocation(),Out=P+FVector(Dir*10.f,0,0),In=P-FVector(Dir*10.f,0,0);
            const bool Relieves=V->RelievesExistingInfantryContactV842(U->Feet(),Out,V->HeadingYawV37,34,.05f);
            // Read the previous minimum-face rule using these same real meshes,
            // native standing feet and the exact prospective support pose.
            const FTransform OldHullPose=V->Body->GetComponentTransform();
            const FTransform Local=OldHullPose.GetRelativeTransform(V->GetActorTransform());
            const FTransform NewHullPose=Local*V->GroundPoseForContactV8218(Out,V->HeadingYawV37,.05f);
            const auto BoxAt=[&](const FTransform& Hull)
            {
                const FVector Vertical=Hull.InverseTransformVector(FVector(0,0,72.5f)),Scale=Hull.GetScale3D().GetAbs();
                return V->Body->GetStaticMesh()->GetBoundingBox().ExpandBy(FVector(FMath::Abs(Vertical.X)+34.f/FMath::Max(.01,Scale.X),
                    FMath::Abs(Vertical.Y)+34.f/FMath::Max(.01,Scale.Y),FMath::Abs(Vertical.Z)));
            };
            const auto Depth=[](const FBox& Box,const FVector& Point,const FVector& Scale)
            {double Minimum=MAX_dbl;for(int32 Axis=0;Axis<3;++Axis)Minimum=FMath::Min(Minimum,FMath::Min(Point[Axis]-Box.Min[Axis],Box.Max[Axis]-Point[Axis])*Scale[Axis]);return Minimum;};
            const FVector BodyCentre=U->Feet()+FVector(0,0,72.5f);
            const double OldDepth=Depth(BoxAt(OldHullPose),OldHullPose.InverseTransformPosition(BodyCentre),OldHullPose.GetScale3D().GetAbs());
            const double NewDepth=Depth(BoxAt(NewHullPose),NewHullPose.InverseTransformPosition(BodyCentre),NewHullPose.GetScale3D().GetAbs());
            const bool OldRuleRelieves=NewDepth+.10<OldDepth;ReproducedClosestFaceStalls+=Relieves&&!OldRuleRelieves;
            auto Relief=V->DescribeMobilityV839();Relief->SetStringField(TEXT("case"),TEXT("grounded_existing_overlap_minimum_face_counterexample"));
            Relief->SetStringField(TEXT("standing_feet"),U->Feet().ToString());Relief->SetNumberField(TEXT("old_minimum_depth_cm"),OldDepth);
            Relief->SetNumberField(TEXT("candidate_minimum_depth_cm"),NewDepth);Relief->SetBoolField(TEXT("previous_rule_relief"),OldRuleRelieves);
            Relief->SetBoolField(TEXT("actual_horizontal_relief"),Relieves);Cases.Add(MakeShared<FJsonValueObject>(Relief));
            Check(Prefix+TEXT("horizontal_exit_does_not_wait_for_closest_floor_face"),Relieves&&V->CanOccupyV48(Out,V->HeadingYawV37,.05f));
            Check(Prefix+TEXT("existing_contact_inward_and_teleport_are_rejected"),!V->RelievesExistingInfantryContactV842(U->Feet(),In,V->HeadingYawV37,34,.05f)
                &&!V->RelievesExistingInfantryContactV842(U->Feet(),P+FVector(Dir*200.f,0,0),V->HeadingYawV37,34,.05f));
            const float Moved=V->AdvanceV37(10.f,.05f);
            Check(Prefix+TEXT("actual_advance_relieves_grounded_existing_contact"),Moved>9.9f&&(V->GetActorLocation().X-P.X)*Dir>9.9f);
        }
        for(auto* Person:People)if(IsValid(Person))Person->Destroy();People.Empty();B->Units.Empty();
        // Physical forward obstruction remains a real body; ordinary routing
        // must find the clear bank shoulder without creating a new overlap.
        Place(V,FVector(Bank-Dir*1300.f,Lane,0));U=NewPerson(Team,FVector(Bank-Dir*200.f,Lane,0));
        bool ClearBody=true;MaxIdle=Idle=0;
        for(int32 F=0;U&&F<900&&(V->GetActorLocation().X-Bank)*Dir<1000.f;++F)
        {
            const FVector Before=V->GetActorLocation();B->Elapsed+=.05f;Tick(.05f);const float Moved=FVector::Dist2D(Before,V->GetActorLocation());
            Idle=Moved<.01f?Idle+.05f:0;MaxIdle=FMath::Max(MaxIdle,Idle);
            ClearBody&=!V->BlocksInfantryAtV8218(U->Feet(),V->GetActorLocation(),V->HeadingYawV37);
        }
        Check(Prefix+TEXT("actual_tick_avoids_stationary_bank_body_without_new_overlap"),U&&ClearBody&&(V->GetActorLocation().X-Bank)*Dir>900.f&&MaxIdle<2.f);
        J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("stationary_grounded_bank_body"));
        J->SetNumberField(TEXT("bank_progress_cm"),(V->GetActorLocation().X-Bank)*Dir);J->SetNumberField(TEXT("max_idle_seconds"),MaxIdle);J->SetBoolField(TEXT("all_accepted_poses_clear"),ClearBody);Cases.Add(MakeShared<FJsonValueObject>(J));
        for(auto* Person:People)if(IsValid(Person))Person->Destroy();People.Empty();B->Units.Empty();
        // Select an actual usable mounted infantry sight on the clear approach.
        // A legitimate combat halt must remain even after the mobility repair.
        Place(V,FVector(Bank-Dir*2400.f,Lane,0));U=NewPerson(1-Team,V->GetActorLocation()+FVector(Dir*700.f,400.f,0));bool Mounted=false;
        for(float Ahead:{700.f,1000.f,1500.f})for(float Across:{-600.f,-300.f,0.f,300.f,600.f})
        {
            if(!U||Mounted)continue;const FVector P=V->GetActorLocation()+FVector(Dir*Ahead,Across,0);
            U->SetActorLocation(FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P)+88.f));FVector Point;
            for(int32 I=0;I<V->CannonMounts.Num()&&!Mounted;++I)Mounted=V->InfantryFiringSolutionV828(U,I,true,Point);
            for(int32 I=0;I<V->GunMounts.Num()&&!Mounted;++I)Mounted=V->InfantryFiringSolutionV828(U,I,false,Point);
        }
        const FVector Halt=V->GetActorLocation();B->Elapsed+=.05f;Tick(.05f);
        Check(Prefix+TEXT("observed_usable_mounted_target_retains_legitimate_combat_halt"),Mounted&&FString(V->MobilityGateV839)==TEXT("observed_mounted_contact")
            &&V->GetActorLocation().Equals(Halt,.001f)&&V->MobilityRequestedV839==0.f);
        J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("legitimate_observed_mounted_contact"));J->SetBoolField(TEXT("usable_mounted_target"),Mounted);Cases.Add(MakeShared<FJsonValueObject>(J));
        for(auto* Person:People)if(IsValid(Person))Person->Destroy();People.Empty();B->Units.Empty();
        V->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,V->TrackIntegrity[0]+1.f);const FVector Disabled=V->GetActorLocation();
        B->Elapsed+=.05f;Tick(.05f);
        Check(Prefix+TEXT("destroyed_running_gear_retains_legitimate_stop"),V->IsOperational()&&V->IsImmobilized()
            &&FString(V->MobilityGateV839)==TEXT("running_gear_disabled")&&V->GetActorLocation().Equals(Disabled,.001f));
    }
    Report->SetNumberField(TEXT("closest_face_stalls_reproduced"),ReproducedClosestFaceStalls);
    Check(TEXT("v8445_previous_closest_face_rule_actually_reproduces_grounded_stall"),ReproducedClosestFaceStalls>0);
    Report->SetStringField(TEXT("scope"),TEXT("Thirteen actual UK/DE hull configurations and unchanged running-gear HP; both travel directions for Mark IV, A7V, Whippet and Austin on the prepared authored trench. Actual fleet Tick, AdvanceV37, standing-body overlap/avoidance, legitimate mounted contact and disabled running gear. No FR/RU encounter, full campaign, all-map or visual acceptance."));
    return Report;
}

