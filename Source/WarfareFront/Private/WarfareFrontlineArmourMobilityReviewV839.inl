// Included in the existing friend. No separate movement model or render clock.
static TSharedRef<FJsonObject> RunMobilityV839(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    FString User;
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV839Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
    Assert(TEXT("v839_mobility_requires_disposable_ukde_world"),Safe);
    auto* Fleet=Safe?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
    if(!Fleet){Report->SetBoolField(TEXT("success"),false);return Report;}
    const auto Vehicles=Fleet->Vehicles;const auto Units=B->Units;const auto Squads=B->Squads;
    const auto Phase=B->Phase;const int32 Winner=B->BattleOutroWinner,Serial=Fleet->NextSerial;
    const float Clock=B->Elapsed,EnemyAt=Fleet->NextEnemyAt;const FGuid Encounter=Fleet->LastEncounter;
    const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;const FString Message=B->LastMessage;
    const int32 Deployment[]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
    const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AFrontlineTank* V:Vehicles)Hide(V);for(AWarfareFrontlineSoldier* U:Units)Hide(U);
    B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Tracers.Reset();Fleet->Vehicles.Reset();
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;Fleet->LastEncounter=B->GetEncounterId();Fleet->NextEnemyAt=MAX_flt;
    UGameplayStatics::SetGamePaused(B,false);
    const auto Clear=[&](){for(AFrontlineTank* V:Fleet->Vehicles)if(IsValid(V))V->Destroy();Fleet->Vehicles.Reset();};
    ON_SCOPE_EXIT
    {
        Clear();Fleet->Vehicles=Vehicles;B->Units=Units;B->Squads=Squads;B->SmokeZones=Smoke;B->Tracers=Tracers;
        B->Phase=Phase;B->BattleOutroWinner=Winner;B->Elapsed=Clock;B->LastMessage=Message;
        Fleet->NextSerial=Serial;Fleet->NextEnemyAt=EnemyAt;Fleet->LastEncounter=Encounter;
        for(int32 T=0;T<2;++T)Fleet->DeploymentCountV37[T]=Deployment[T];
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        UGameplayStatics::SetGamePaused(B,Paused);
        Report->SetArrayField(TEXT("cases"),Rows);Report->SetBoolField(TEXT("success"),Good);
        Report->SetStringField(TEXT("scope"),TEXT("Ten current UK/DE chassis plus two Pioneer aliases on this prepared authored terrain. Actual fleet Tick consumes bounded world delta; physical occupancy, track phase, pitch, roll, speed bounds and clear-bank progress are checked. Three close live-neighbour routes retain collision. No FR/RU campaign, all-map claim, visual-quality score or frame-generation acceptance."));
    };
    const auto Place=[&](AFrontlineTank* V,const FVector& P)
    {
        if(!V)return;
        V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;
        V->RouteDecisionAtV37=0;V->RouteStallSecondsV837=0;V->bEntered=true;V->FitGroundV35(P,0);
    };
    static const EFrontlineVehicle Types[]={EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918,
        EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkIVFemale,EFrontlineVehicle::MarkVMale,EFrontlineVehicle::MarkVStar,
        EFrontlineVehicle::A7V,EFrontlineVehicle::A7VGretchen,EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::BeuteMarkIVFemale,
        EFrontlineVehicle::MarkIVPioneer,EFrontlineVehicle::BeutePioneer};
    const int32 Centre=B->Trenches.Num()/2;
    Assert(TEXT("v839_prepared_world_has_real_trench_bank"),B->Trenches.IsValidIndex(Centre));
    if(!B->Trenches.IsValidIndex(Centre))return Report;
    const bool BypassOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV839MobilityBypassOnly"));
    for(int32 I=0;I<UE_ARRAY_COUNT(Types)&&!BypassOnly;++I)
    {
        Clear();const EFrontlineVehicle Type=Types[I];const bool German=GermanVehicle(Type);
        const int32 Team=(B->GetFactionForTeam(0)==EFrontlineFaction::German)==German?0:1,Dir=B->GetTeamDirection(Team);
        auto* V=Fleet->Spawn(Type,Team,true);
        const FString Prefix=FString::Printf(TEXT("v839_model%d_"),int32(Type));
        Assert(Prefix+TEXT("actual_assets_present"),V&&V->AreAssetsReady());if(!V)continue;
        const float Lane=(I%2?-300.f:300.f),Bank=FrontlineFieldV32::TrenchCentreX(Centre,Lane);
        Place(V,FVector(Bank-Dir*900.f,Lane,0));const FVector Start=V->GetActorLocation();
        const float InitialPhase=V->GetTrackTravelPhase();float Travel=0,MaximumStep=0,MaximumTurn=0;int32 Stationary=0;
        bool Finite=true,Bounded=true,ClearHull=true;float MaximumPitch=0,MaximumRoll=0;
        // The subsystem applies the real production substep budget, rather
        // than actor tick (tank actors intentionally do not tick themselves).
        for(int32 F=0;F<400&&(V->GetActorLocation().X-Start.X)*Dir<1400.f;++F)
        {
            const FVector Before=V->GetActorLocation();const float Heading=V->HeadingYawV37;
            B->Elapsed+=.05f;Fleet->Tick(.05f);
            const FVector After=V->GetActorLocation();const FRotator Rotation=V->GetActorRotation();
            const float Step=FVector::Dist2D(Before,After),Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(Heading,V->HeadingYawV37));
            Travel+=Step;Stationary+=Step<.01f;MaximumStep=FMath::Max(MaximumStep,Step);MaximumTurn=FMath::Max(MaximumTurn,Turn);
            MaximumPitch=FMath::Max(MaximumPitch,float(FMath::Abs(Rotation.Pitch)));MaximumRoll=FMath::Max(MaximumRoll,float(FMath::Abs(Rotation.Roll)));
            Finite&=!V->GetActorTransform().ContainsNaN();ClearHull&=V->CanOccupyV48(After,V->HeadingYawV37);
            const float Scale=FrontlineArmourV63::TravelSpeedScaleV8218(B,Team);
            Bounded&=Step<=UFrontlineVehicleSubsystem::Definition(Type).SpeedCm*Scale*.0501f+.01f
                &&(After.X-Before.X)*Dir>=-.01f&&FMath::Abs(After.Y)<=980.1f;
        }
        const float Progress=(V->GetActorLocation().X-Start.X)*Dir;
        Assert(Prefix+TEXT("continuous_healthy_bank_crossing"),Finite&&Bounded&&ClearHull&&V->IsOperational()&&!V->IsImmobilized()&&Progress>800.f);
        Assert(Prefix+TEXT("actual_support_keeps_bounded_pitch_roll"),MaximumPitch<=12.01f&&MaximumRoll<=8.01f);
        Assert(Prefix+TEXT("actual_fleet_step_clock_and_running_gear_advance"),V->MobilityStepV839>.049f&&V->MobilityStepV839<.051f
            &&V->MobilityHostTimeV839==B->Elapsed&&V->DistanceTravelled>0.f
            &&(V->IsWheeledV8218()||V->GetTrackTravelPhase()!=InitialPhase));
        auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("healthy_real_bank_crossing"));
        J->SetNumberField(TEXT("travel_cm"),Travel);J->SetNumberField(TEXT("progress_cm"),Progress);J->SetNumberField(TEXT("stationary_steps"),Stationary);
        J->SetNumberField(TEXT("maximum_step_cm"),MaximumStep);J->SetNumberField(TEXT("maximum_turn_deg"),MaximumTurn);
        J->SetNumberField(TEXT("maximum_pitch_deg"),MaximumPitch);J->SetNumberField(TEXT("maximum_roll_deg"),MaximumRoll);Rows.Add(MakeShared<FJsonValueObject>(J));
    }
    for(const EFrontlineVehicle Type:{EFrontlineVehicle::Austin1918,EFrontlineVehicle::Whippet,EFrontlineVehicle::MarkIVMale})
    {
        Clear();const int32 Team=B->GetFactionForTeam(0)==EFrontlineFaction::British?0:1,Dir=B->GetTeamDirection(Team);
        auto* Moving=Fleet->Spawn(Type,Team,true);auto* Neighbour=Fleet->Spawn(EFrontlineVehicle::MarkIVMale,Team,true);
        const FString Prefix=FString::Printf(TEXT("v839_model%d_close_neighbour_"),int32(Type));
        Assert(Prefix+TEXT("actual_assets_present"),Moving&&Neighbour);if(!Moving||!Neighbour)continue;
        const float X=FrontlineFieldV32::TrenchCentreX(Centre,300.f)-Dir*1500.f;
        Place(Moving,FVector(X,300,0));Place(Neighbour,FVector(X+Dir*(Moving->FootprintHalfLengthV37()+Neighbour->FootprintHalfLengthV37()+20.f),300,0));
        Neighbour->TrackIntegrity[0]=0;Moving->RouteDecisionAtV37=.45f;
        const FVector Start=Moving->GetActorLocation();bool ClearHull=true,Bounded=true,ReverseGear=true;float RecoveryClock=0;int32 BackingSteps=0;
        for(int32 F=0;F<300;++F)
        {
            const FVector Before=Moving->GetActorLocation();const float GearBefore=Moving->TrackOffset;
            B->Elapsed+=.05f;Fleet->Tick(.05f);const FVector After=Moving->GetActorLocation();
            ClearHull&=Moving->CanOccupyV48(After,Moving->HeadingYawV37);
            const float Step=FVector::Dist2D(Before,After);const bool Backing=Moving->bBackingStepV839;
            const bool PermittedBacking=Backing&&Moving->BypassNeighbourV839.Get()==Neighbour&&Neighbour->Team==Moving->Team
                &&Neighbour->IsOperational()&&Moving->BypassBackingTotalV839<=Moving->BypassBackingLimitV839+.01f;
            Bounded&=((After.X-Before.X)*Dir>=-.01f||PermittedBacking)&&FMath::Abs(After.Y)<=980.1f&&!After.ContainsNaN()
                &&Step<=UFrontlineVehicleSubsystem::Definition(Type).SpeedCm*FrontlineArmourV63::TravelSpeedScaleV8218(B,Team)*.0501f+.01f;
            if(Backing)
            {
                ++BackingSteps;
                const float Period=Moving->IsWheeledV8218()?2.f*PI*48.f:Moving->PathLength;
                float Expected=FMath::Fmod(GearBefore-Step,Period);if(!Moving->IsWheeledV8218()&&Expected<0.f)Expected+=Period;
                ReverseGear&=FMath::IsNearlyEqual(Expected,Moving->TrackOffset,.002f);
            }
            RecoveryClock=FMath::Max(RecoveryClock,Moving->RouteStallSecondsV837);
        }
        const float Progress=(Moving->GetActorLocation().X-Start.X)*Dir;
        Assert(Prefix+TEXT("bounded_live_hull_bypass_keeps_collision"),ClearHull&&Bounded&&Progress>100.f&&Moving->IsOperational()&&!Moving->IsImmobilized());
        Assert(Prefix+TEXT("checked_short_backing_has_matching_reverse_running_gear"),BackingSteps>0&&ReverseGear
            &&Moving->BypassBackingTotalV839>0&&Moving->BypassBackingTotalV839<=Moving->BypassBackingLimitV839+.01f);
        auto J=Moving->DescribeMobilityV839();J->SetStringField(TEXT("case"),TEXT("close_disabled_live_neighbour"));
        J->SetNumberField(TEXT("progress_cm"),Progress);J->SetNumberField(TEXT("maximum_stall_feedback_seconds"),RecoveryClock);
        J->SetNumberField(TEXT("actual_backing_steps"),BackingSteps);J->SetBoolField(TEXT("reverse_running_gear_matches_travel"),ReverseGear);
        J->SetBoolField(TEXT("all_accepted_hulls_clear"),ClearHull);Rows.Add(MakeShared<FJsonValueObject>(J));
    }
    return Report;
}
