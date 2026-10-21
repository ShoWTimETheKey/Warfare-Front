// Fixed production Simulate starts, with a scoped old forward-only witness.
// No placement/yaw/weapon-clock changes after each case's initial state.
struct FFrontlineArmourApproachReviewV85
{
static TSharedRef<FJsonObject> RunArchiveAllocationV85(UFrontlineVehicleSubsystem* Self);
static TSharedRef<FJsonObject> Run(UFrontlineVehicleSubsystem* Self)
{
    const auto GetWorld=[Self]{return Self->GetWorld();};const auto Battle=[Self]{return Self->Battle();};
    auto& Vehicles=Self->Vehicles;auto& NextSerial=Self->NextSerial;auto& NextEnemyAt=Self->NextEnemyAt;auto& LastEncounter=Self->LastEncounter;
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](const FString& Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    ON_SCOPE_EXIT{LegacyForwardOnlyAcquisitionV85=false;Report->SetBoolField(TEXT("success"),Good);Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);};
    auto* B=Battle();FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V85/Runtime"));FPaths::NormalizeDirectoryName(Root);
    const FString Stamp=User.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)?User.Mid(Root.Len()+1):FString();
    FString NewUser;
    const bool OriginalReview=FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))
        &&!FPaths::IsRelative(User)&&!User.Contains(TEXT(".."))&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    const bool NewReview=FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(NewUser);
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&(OriginalReview||NewReview);
    Check(TEXT("v85_armour_exact_disposable_ukde_world"),Safe);if(!Safe)return Report;
    const bool DenseDiagnostic=NewReview&&FParse::Param(FCommandLine::Get(),TEXT("WFV851DenseDiagnostic"));
    const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;
    const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
    const auto Faction=B->PlayerFaction;const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53;
    const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;const auto Phase=B->Phase;
    const auto Random=B->Random;const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]};
    const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]},Friendly=B->FriendlyFireDamage;
    const FFrontlineCombatRecordV65 Combat[2]={B->CombatRecordV65[0],B->CombatRecordV65[1]};const FString End=B->EndReason,Failure=B->AssetFailure;
    const int32 Serial=NextSerial,Shots=B->ShotsFired,Winner=B->BattleOutroWinner;const float Clock=B->Elapsed,Outro=B->BattleOutroElapsed,EnemyAt=NextEnemyAt;
    const auto Encounter=LastEncounter;const FString Message=B->LastMessage;
    const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
    for(AFrontlineTank* V:SavedVehicles)if(IsValid(V)){Hidden.Add({V,V->IsHidden(),V->GetActorEnableCollision()});V->SetActorHiddenInGame(true);V->SetActorEnableCollision(false);}
    for(AWarfareFrontlineSoldier* U:SavedUnits)if(IsValid(U)){Hidden.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
    TSet<AActor*> Existing;for(TActorIterator<AActor> It(GetWorld());It;++It)Existing.Add(*It);
    const auto Clear=[&]{for(TActorIterator<AActor> It(GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();Vehicles.Reset();B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Tracers.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=1000;NextEnemyAt=MAX_flt;NextSerial=8500;LegacyForwardOnlyAcquisitionV85=false;LegacyContactChoiceV84414=false;FrontlineMobilityV41::Reset(B);};
    ON_SCOPE_EXIT
    {
        Clear();Vehicles=SavedVehicles;B->Units=SavedUnits;B->Squads=SavedSquads;B->CampaignRuns=Runs;B->ActiveCampaignId=Active;
        B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->PlayerFaction=Faction;B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;
        B->SmokeZones=Smoke;B->Tracers=Tracers;B->Phase=Phase;B->ShotsFired=Shots;B->BattleOutroWinner=Winner;B->BattleOutroElapsed=Outro;B->Elapsed=Clock;
        NextSerial=Serial;LastEncounter=Encounter;NextEnemyAt=EnemyAt;B->LastMessage=Message;
        B->Random=Random;B->FriendlyFireDamage=Friendly;B->EndReason=End;B->AssetFailure=Failure;
        for(int32 Team=0;Team<2;++Team){B->Morale[Team]=Morale[Team];B->Casualties[Team]=Casualties[Team];B->DamageDealt[Team]=Damage[Team];B->CombatRecordV65[Team]=Combat[Team];}
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}UGameplayStatics::SetGamePaused(B,Paused);
    };
    UGameplayStatics::SetGamePaused(B,false);LastEncounter=B->GetEncounterId();
    const auto Campaign=[&](int32 Nation,int32 Difficulty)
    {
        B->PlayerFaction=EFrontlineFaction(Nation);FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=B->PlayerFaction;R.Difficulty=EFrontlineDifficulty(Difficulty);
        R.SelectedMission=R.CompletedMissions=19;B->CampaignRuns={R};B->ActiveCampaignId=B->BoundCampaignId=R.Id;B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=19;
    };
    const auto Place=[&](AFrontlineTank* V,FVector P)
    {
        V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=P.Y;
        V->RouteDecisionAtV37=V->RouteStallSecondsV837=0;V->ArmourContactV48.Reset();V->ArmourContactAgeV48=2;V->InfantryContactV8448.Reset();V->bEntered=true;V->FitGroundV35(P,0);
    };
    const auto NewTank=[&](EFrontlineVehicle Type,int32 Team,FVector P)
    {
        auto* V=GetWorld()->SpawnActor<AFrontlineTank>();if(!V)return V;
        const bool Rail=Type==EFrontlineVehicle::RailConvoy;
        if(!V->Configure(B,Rail?EFrontlineVehicle::A7V:Type,Team,++NextSerial)
            ||(Rail&&!V->ConfigureRailV32())){V->Destroy();return (AFrontlineTank*)nullptr;}
        V->SetActorTickEnabled(false);Vehicles.Add(V);Place(V,P);V->bRailUnloadedV75=true;
        return V;
    };
    const double Began=FPlatformTime::Seconds();int32 Approaches=0,RearWitnesses=0;
    const EFrontlineVehicle CannonModels[]={EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkVMale,EFrontlineVehicle::MarkVStar,EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::RailConvoy};
    const auto Sample=[&](AFrontlineTank* V,AFrontlineTank* Other,float Time)
    {
        auto J=V->DescribeMobilityV839();J->SetNumberField(TEXT("time"),Time);J->SetNumberField(TEXT("shots"),V->CannonShots);
        J->SetNumberField(TEXT("other_hull"),Other->HullIntegrity);J->SetNumberField(TEXT("root_distance"),FVector::Dist2D(V->GetActorLocation(),Other->GetActorLocation()));
        J->SetNumberField(TEXT("heading"),V->HeadingYawV37);J->SetNumberField(TEXT("acquisition_yaw"),V->RollingAcquisitionYawV8448);
        J->SetBoolField(TEXT("rolling_acquisition"),V->bRollingAcquisitionV8448);J->SetBoolField(TEXT("selected_enemy"),V->ArmourContactV48.Get()==Other);
        float Heading=V->HeadingYawV37;const bool Feasible=V->MountedHeadingV8448(Other->AimPoint(),Heading,true,Other);
        J->SetBoolField(TEXT("physically_feasible_acquisition"),Feasible);J->SetNumberField(TEXT("physically_feasible_yaw"),Heading);
        bool Current=false,Aiming=false;FVector P;
        for(int32 I=0;I<V->CannonMounts.Num();++I){Current|=V->ArmourFiringSolutionV8219(Other,I,P);Aiming|=V->ArmourFiringSolutionV8219(Other,I,P,true);}
        J->SetBoolField(TEXT("actual_muzzle_solution"),Current);J->SetBoolField(TEXT("aiming_muzzle_solution"),Aiming);return J;
    };
    // Fixed starts deliberately include already stopped tanks and a gun aimed
    // away from a visible enemy. Older reviews searched for a mutually usable
    // muzzle first and used broad opposite lanes; that hid endpoint/aim gates.
    int32 PersistentContactTrialsV851=0;
    if(!DenseDiagnostic)for(int32 Nation=0;Nation<2;++Nation)for(auto Type:CannonModels)for(int32 Scenario=0;Scenario<3;++Scenario)
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);
        const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        const float Centre=Bank+(Scenario==2?0.f:1700.f),Half=Scenario==2?950.f:1800.f;
        const float Lane=Scenario==2?0.f:350.f;
        auto* A=NewTank(Type,0,FVector(Centre-Dir*Half,-Lane,0));
        auto* E=NewTank(Type==EFrontlineVehicle::A7V?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1,FVector(Centre+Dir*Half,Lane,0));
        const FString Name=FString::Printf(TEXT("v851_n%d_model%d_contact%d"),Nation,int32(Type),Scenario);
        Check(Name+TEXT("_production_meshes_created"),A&&E&&A->AreAssetsReady()&&E->AreAssetsReady());if(!A||!E)continue;
        if(Scenario==0)
        {
            // Immobilized opponents still possess working main guns. Track
            // damage grants no extra traverse or range and cannot deny fire.
            A->TrackIntegrity[0]=A->TrackIntegrity[1]=0;E->TrackIntegrity[0]=E->TrackIntegrity[1]=0;
        }
        if(Scenario==1)
        {
            for(int32 I=0;I<A->Cannons.Num();++I)A->Cannons[I]->SetRelativeRotation(FRotator(0,A->CannonBaseYaw[I]-50.f,0));
            for(int32 I=0;I<E->Cannons.Num();++I)E->Cannons[I]->SetRelativeRotation(FRotator(0,E->CannonBaseYaw[I]+50.f,0));
        }
        const float InitialA=A->HullIntegrity,InitialE=E->HullIntegrity;float FirstA=-1,FirstE=-1,Time=0;
        bool Finite=true;TArray<TSharedPtr<FJsonValue>> Samples;
        for(int32 Frame=0;Frame<1050&&(FirstA<0||FirstE<0||A->HullIntegrity==InitialA||E->HullIntegrity==InitialE);++Frame)
        {
            const float DT=Frame%9==0?.1f:1.f/30.f;Time+=DT;B->Elapsed+=DT;A->Simulate(DT);E->Simulate(DT);
            if(A->CannonShots>0&&FirstA<0)FirstA=Time;if(E->CannonShots>0&&FirstE<0)FirstE=Time;
            Finite&=!A->GetActorTransform().ContainsNaN()&&!E->GetActorTransform().ContainsNaN();
            if(Frame%30==0)
            {
                auto Detail=Sample(A,E,Time);Detail->SetObjectField(TEXT("enemy"),Sample(E,A,Time));
                TArray<TSharedPtr<FJsonValue>> Mounts;
                for(int32 I=0;I<A->Cannons.Num();++I)
                {
                    auto M=MakeShared<FJsonObject>();FVector P;float Distance=0;
                    const bool CanAim=A->ArmourFiringSolutionV8219(E,I,P,true,&Distance);
                    const bool CanShoot=A->ArmourFiringSolutionV8219(E,I,P,false);
                    M->SetNumberField(TEXT("mount"),I);M->SetBoolField(TEXT("future_muzzle_clear"),CanAim);M->SetBoolField(TEXT("actual_muzzle_clear"),CanShoot);
                    M->SetNumberField(TEXT("clock"),A->CannonClock[I]);M->SetNumberField(TEXT("future_distance"),Distance);
                    M->SetStringField(TEXT("rendered_gun_rotation"),A->Cannons[I]->GetRelativeRotation().ToString());
                    M->SetStringField(TEXT("actual_muzzle"),A->WeaponMuzzle(I,true).ToString());Mounts.Add(MakeShared<FJsonValueObject>(M));
                }
                Detail->SetArrayField(TEXT("mounts"),Mounts);Samples.Add(MakeShared<FJsonValueObject>(Detail));
            }
            if(!A->IsOperational()||!E->IsOperational())break;
        }
        Check(Name+TEXT("_both_real_guns_fire_and_flight_damages_hulls"),FirstA>0&&FirstE>0&&A->HullIntegrity<InitialA&&E->HullIntegrity<InitialE&&Finite);
        auto Detail=MakeShared<FJsonObject>();Detail->SetStringField(TEXT("name"),Name);Detail->SetNumberField(TEXT("first_player_shell"),FirstA);Detail->SetNumberField(TEXT("first_enemy_shell"),FirstE);
        Detail->SetNumberField(TEXT("player_hull_loss"),InitialA-A->HullIntegrity);Detail->SetNumberField(TEXT("enemy_hull_loss"),InitialE-E->HullIntegrity);
        Detail->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(Detail));++PersistentContactTrialsV851;
    }
    if(!DenseDiagnostic)Check(TEXT("v851_thirty_six_fixed_stopped_wrong_aim_and_same_lane_contacts_executed"),PersistentContactTrialsV851==36);
    // Dense formations retain their actual hull collision and independent
    // moving opponents. No target is shuffled, teleported or held by the test.
    // The fourth case is already passed: a friendly adjacent hull may veto the
    // otherwise legal stationary acquisition which sparse V85 pairs accepted.
    int32 DenseTrialsV851=0;
    for(int32 Nation=0;Nation<2;++Nation)for(int32 Scenario=DenseDiagnostic?3:0;Scenario<4;++Scenario)
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);
        const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        TArray<AFrontlineTank*> Dense;TArray<float> First;TArray<int32> Switches,LastSelected;
        for(int32 Team=0;Team<2;++Team)for(int32 Slot=0;Slot<3;++Slot)
        {
            const int32 Side=Team==0?-Dir:Dir;
            const float Along=Scenario==3?-(1700.f+Slot*80.f):Slot==0?1450.f:2500.f;
            const float Lane=Scenario==3?(Slot-1)*610.f:Slot==0?0.f:Slot==1?-640.f:640.f;
            EFrontlineVehicle Type=Slot==0?EFrontlineVehicle::A7V:Slot==1?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::MarkVStar;
            if(Scenario==1)Type=Slot==0?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V;
            if(Scenario==2&&Slot==0)Type=Team==0?EFrontlineVehicle::Austin1918:EFrontlineVehicle::Whippet;
            auto* V=NewTank(Type,Team,FVector(Bank+Side*Along,Lane,0));
            if(V){Dense.Add(V);First.Add(-1.f);Switches.Add(0);LastSelected.Add(INDEX_NONE);}
        }
        const FString Name=FString::Printf(TEXT("v851_n%d_dense%d"),Nation,Scenario);
        Check(Name+TEXT("_six_actual_hulls_created"),Dense.Num()==6);if(Dense.Num()!=6)continue;
        bool InitialClear=true,Finite=true;int32 Overlaps=0;float Time=0;
        for(int32 I=0;I<Dense.Num();++I)for(int32 J=I+1;J<Dense.Num();++J)
            InitialClear&=ArmourHullSeparatedV85(Dense[I],Dense[I]->GetActorTransform(),Dense[J],Dense[J]->GetActorTransform());
        TArray<TSharedPtr<FJsonValue>> Samples;
        // Six long hulls may need sequential real turning room. Keep natural
        // weapon clocks and steering caps; allow that physical manoeuvre rather
        // than moving fixtures or forcing a shot to satisfy a short deadline.
        for(int32 Frame=0;Frame<(Scenario==3?1500:840);++Frame)
        {
            const float DT=Frame%9==0?.1f:1.f/30.f;Time+=DT;B->Elapsed+=DT;
            for(int32 I=0;I<Dense.Num();++I)
            {
                auto* V=Dense[I];V->Simulate(DT);Finite&=!V->GetActorTransform().ContainsNaN();
                if(V->CannonShots>0&&First[I]<0)First[I]=Time;
                const auto* Contact=V->ArmourContactV48.Get();const int32 Selected=Contact?Contact->Serial:INDEX_NONE;
                if(LastSelected[I]!=INDEX_NONE&&Selected!=INDEX_NONE&&Selected!=LastSelected[I])++Switches[I];
                LastSelected[I]=Selected;
            }
            if(Frame%30==0)
            {
                auto FrameSample=MakeShared<FJsonObject>();FrameSample->SetNumberField(TEXT("time"),Time);TArray<TSharedPtr<FJsonValue>> Rows;
                for(int32 I=0;I<Dense.Num();++I)
                {
                    auto* V=Dense[I];auto J=V->DescribeMobilityV839();J->SetNumberField(TEXT("shots"),V->CannonShots);
                    J->SetBoolField(TEXT("acquisition_clearance"),V->bArmourClearanceV851);
                    J->SetBoolField(TEXT("acquisition_clearance_step"),V->bArmourClearanceStepV851);
                    if(V->bArmourClearanceV851)
                    {
                        const FVector P=V->GetActorLocation();const float Forward=B->GetTeamDirection(V->Team)>0?0.f:180.f;
                        const float Next=FMath::FixedTurn(V->HeadingYawV37,Forward,V->SteeringRateV8311(0.f,true)*DT);
                        const auto HullBlocker=[&](const FTransform& Pose)->int32
                        {
                            for(const AFrontlineTank* Other:Self->GetVehicles())
                                if(IsValid(Other)&&Other!=V&&Other->IsOperational()&&!Other->IsHidden()
                                    &&!ArmourHullSeparatedV85(V,Pose,Other,Other->GetActorTransform()))return Other->Serial;
                            return INDEX_NONE;
                        };
                        auto Recovery=MakeShared<FJsonObject>();Recovery->SetNumberField(TEXT("forward_heading"),Forward);
                        Recovery->SetNumberField(TEXT("next_heading"),Next);
                        Recovery->SetBoolField(TEXT("unwind_occupancy"),V->CanOccupyV48(P,Next,DT));
                        Recovery->SetNumberField(TEXT("unwind_hull_blocker"),HullBlocker(V->GroundPoseForContactV8218(P,Next,DT)));
                        Recovery->SetNumberField(TEXT("translation_lease_owner"),V->ArmourClearanceLeaseOwnerV851);
                        Recovery->SetNumberField(TEXT("backing_cm"),V->ArmourClearanceBackingV851);
                        Recovery->SetNumberField(TEXT("backing_limit_cm"),FMath::Max(300.f,V->FootprintHalfLengthV37()*2.f));
                        for(int32 Sign:{1,-1})
                        {
                            const FVector To=P+FRotator(0,V->HeadingYawV37,0).Vector()*Sign*12.f;
                            const FString Key=Sign>0?TEXT("forward"):TEXT("reverse");
                            Recovery->SetBoolField(Key+TEXT("_12cm_occupancy"),V->CanOccupyV48(To,V->HeadingYawV37,DT));
                            Recovery->SetBoolField(Key+TEXT("_12cm_inside_field"),FMath::Abs(To.Y)<=980.f);
                            Recovery->SetNumberField(Key+TEXT("_12cm_hull_blocker"),HullBlocker(V->GroundPoseForContactV8218(To,V->HeadingYawV37,DT)));
                        }
                        J->SetObjectField(TEXT("recovery_physical_diagnostic"),Recovery);
                    }
                    J->SetNumberField(TEXT("first_shell"),First[I]);J->SetNumberField(TEXT("global_target_switches"),Switches[I]);
                    const auto* Contact=V->ArmourContactV48.Get();J->SetNumberField(TEXT("global_target_serial"),Contact?Contact->Serial:INDEX_NONE);
                    if(Contact){FVector P;bool Future=false,Actual=false;for(int32 M=0;M<V->Cannons.Num();++M)
                        {Future|=V->ArmourFiringSolutionV8219(Contact,M,P,true);Actual|=V->ArmourFiringSolutionV8219(Contact,M,P);}
                        J->SetBoolField(TEXT("global_future_muzzle_clear"),Future);J->SetBoolField(TEXT("global_actual_muzzle_clear"),Actual);}
                    TArray<TSharedPtr<FJsonValue>> Mounts;
                    for(int32 M=0;M<V->Cannons.Num();++M)
                    {
                        const AFrontlineTank* Chosen=nullptr;FVector Target;float Best=FMath::Min(6500.f,UFrontlineVehicleSubsystem::Definition(V->Type).RangeCm*B->EncounterVisibilityScale());
                        for(auto* Other:Dense)if(Other!=V&&Other->Team!=V->Team&&Other->IsOperational())
                        {FVector P;float D=MAX_flt;if(V->ArmourFiringSolutionV8219(Other,M,P,true,&D)&&D<Best){Best=D;Chosen=Other;Target=P;}}
                        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("mount"),M);Row->SetNumberField(TEXT("target_serial"),Chosen?Chosen->Serial:INDEX_NONE);
                        Row->SetNumberField(TEXT("clock"),V->CannonClock[M]);Row->SetStringField(TEXT("rotation"),V->Cannons[M]->GetRelativeRotation().ToString());
                        if(Chosen){const FRotator Bearing=(V->GetActorTransform().InverseTransformPosition(Target)-V->CannonMounts[M]).Rotation();
                            Row->SetNumberField(TEXT("yaw_error"),FMath::Abs(FMath::FindDeltaAngleDegrees(V->Cannons[M]->GetRelativeRotation().Yaw,Bearing.Yaw)));
                            Row->SetNumberField(TEXT("pitch_error"),FMath::Abs(FMath::FindDeltaAngleDegrees(V->Cannons[M]->GetRelativeRotation().Pitch,Bearing.Pitch)));}
                        Mounts.Add(MakeShared<FJsonValueObject>(Row));
                    }
                    J->SetArrayField(TEXT("mounts"),Mounts);Rows.Add(MakeShared<FJsonValueObject>(J));
                }
                FrameSample->SetArrayField(TEXT("vehicles"),Rows);Samples.Add(MakeShared<FJsonValueObject>(FrameSample));
                for(int32 I=0;I<Dense.Num();++I)for(int32 J=I+1;J<Dense.Num();++J)
                    Overlaps+=!ArmourHullSeparatedV85(Dense[I],Dense[I]->GetActorTransform(),Dense[J],Dense[J]->GetActorTransform());
            }
            bool All=true;for(int32 I=0;I<Dense.Num();++I)if(!Dense[I]->Cannons.IsEmpty()&&First[I]<0&&Dense[I]->IsOperational())All=false;
            if(All&&Time>8.f)break;
        }
        bool BothTeamsFire[2]={false,false},AllLiveCannonsFire=true;
        for(int32 I=0;I<Dense.Num();++I){BothTeamsFire[Dense[I]->Team]|=First[I]>0;AllLiveCannonsFire&=Dense[I]->Cannons.IsEmpty()||First[I]>0||!Dense[I]->IsOperational();}
        Check(Name+TEXT("_finite_physical_formation_both_armies_commit_real_shells"),InitialClear&&Finite&&BothTeamsFire[0]&&BothTeamsFire[1]);
        Check(Name+TEXT("_every_surviving_cannon_reaches_a_real_firing_solution"),AllLiveCannonsFire);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("initial_hulls_clear"),InitialClear);
        J->SetNumberField(TEXT("sampled_hull_overlaps"),Overlaps);J->SetNumberField(TEXT("elapsed"),Time);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));++DenseTrialsV851;
    }
    Check(TEXT("v851_dense_natural_six_hull_formations_executed"),DenseTrialsV851==(DenseDiagnostic?2:8));
    if(DenseDiagnostic)
    {
        Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Began);
        Report->SetStringField(TEXT("scope"),TEXT("Exact original two UK/DE dense3 formations only; unchanged authored starts, mesh collision, game-time steps and production Simulate. Read-only pivot/translation blocker diagnostics."));
        return Report;
    }
    // Fixed authored terrain positions. Never search for a pre-aligned shot,
    // change cannon clocks, flatten a bank or assign a yaw after frame zero.
    for(int32 Nation=0;Nation<2;++Nation)for(int32 TerrainCase=0;TerrainCase<2;++TerrainCase)for(auto Type:CannonModels)
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);
        const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        const float Centre=Bank+(TerrainCase?1200.f:0.f),Separation=TerrainCase?4200.f:1600.f;
        auto* A=NewTank(Type,0,FVector(Centre-Dir*Separation,-700.f,0));
        auto* E=NewTank(Type==EFrontlineVehicle::A7V?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1,FVector(Centre+Dir*Separation,700.f,0));
        const FString Name=FString::Printf(TEXT("v85_n%d_terrain%d_model%d_natural_approach"),Nation,TerrainCase,int32(Type));
        Check(Name+TEXT("_actual_configured_meshes"),A&&E&&A->AreAssetsReady()&&E->AreAssetsReady());if(!A||!E)continue;
        ++Approaches;const FVector StartA=A->GetActorLocation(),StartE=E->GetActorLocation();const float HullA=A->HullIntegrity,HullE=E->HullIntegrity;
        const double InitialSide=(StartE.X-StartA.X)*Dir;float Time=0,FirstA=-1,FirstE=-1,HitA=-1,HitE=-1;
        bool CrossedBeforeFire=false,ClearHulls=true,Finite=true;TArray<TSharedPtr<FJsonValue>> Samples;
        for(int32 Frame=0;Frame<1500&&(HitA<0||HitE<0);++Frame)
        {
            const float DT=Frame%9==0?.10f:1.f/30.f;Time+=DT;B->Elapsed+=DT;A->Simulate(DT);E->Simulate(DT);
            if(A->CannonShots>0&&FirstA<0)FirstA=Time;if(E->CannonShots>0&&FirstE<0)FirstE=Time;
            if(A->HullIntegrity<HullA&&HitA<0)HitA=Time;if(E->HullIntegrity<HullE&&HitE<0)HitE=Time;
            CrossedBeforeFire|=(FirstA<0||FirstE<0)&&(E->GetActorLocation().X-A->GetActorLocation().X)*Dir*InitialSide<0;
            ClearHulls&=ArmourHullSeparatedV85(A,A->GetActorTransform(),E,E->GetActorTransform());
            Finite&=!A->GetActorTransform().ContainsNaN()&&!E->GetActorTransform().ContainsNaN();
            if(Frame%30==0){auto J=Sample(A,E,Time);J->SetObjectField(TEXT("enemy"),Sample(E,A,Time));Samples.Add(MakeShared<FJsonValueObject>(J));}
            if(!A->IsOperational()||!E->IsOperational())break;
        }
        Check(Name+TEXT("_both_production_guns_fire_and_flight_damages_opponent"),FirstA>0&&FirstE>0&&HitA>0&&HitE>0);
        Check(Name+TEXT("_no_uncontested_crossing_or_actual_hull_overlap"),!CrossedBeforeFire&&ClearHulls&&Finite);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("first_player_shell"),FirstA);J->SetNumberField(TEXT("first_enemy_shell"),FirstE);
        J->SetNumberField(TEXT("player_first_hull_damage"),HitA);J->SetNumberField(TEXT("enemy_first_hull_damage"),HitE);J->SetNumberField(TEXT("player_travel_cm"),FVector::Dist2D(StartA,A->GetActorLocation()));
        J->SetNumberField(TEXT("enemy_travel_cm"),FVector::Dist2D(StartE,E->GetActorLocation()));J->SetBoolField(TEXT("crossed_before_both_fired"),CrossedBeforeFire);J->SetBoolField(TEXT("actual_hulls_separated"),ClearHulls);
        J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
    }
    // Paired source witness: the same configured RNG, weapon clocks and fixed
    // post-pass positions use the old forward clamp or the bounded real pivot.
    for(int32 Nation=0;Nation<2;++Nation)for(auto Type:{EFrontlineVehicle::A7V,EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkVMale,EFrontlineVehicle::MarkVStar})
    {
        float OldShotsA=0,OldShotsE=0,OldTravelA=0;bool OldSelected=false;
        const FString Name=FString::Printf(TEXT("v85_n%d_model%d_passed_armour"),Nation,int32(Type));
        for(int32 Trial=0;Trial<2;++Trial)
        {
            Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);
            const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
            auto* A=NewTank(Type,0,FVector(Bank+Dir*1600.f,-700.f,0));
            auto* E=NewTank(Type==EFrontlineVehicle::A7V?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1,FVector(Bank-Dir*1600.f,700.f,0));
            Check(Name+FString::Printf(TEXT("_trial%d_assets"),Trial),A&&E&&A->AreAssetsReady()&&E->AreAssetsReady());if(!A||!E)continue;
            const FVector Start=A->GetActorLocation();const float HullA=A->HullIntegrity,HullE=E->HullIntegrity;float FirstA=-1,FirstE=-1,Time=0;
            bool Selected=false,ClearHulls=true,BoundedTurns=true;TArray<TSharedPtr<FJsonValue>> Samples;
            LegacyForwardOnlyAcquisitionV85=Trial==0;
            for(int32 Frame=0;Frame<660;++Frame)
            {
                const float DT=Frame%9==0?.10f:1.f/30.f;Time+=DT;B->Elapsed+=DT;
                const float YawA=A->HeadingYawV37,YawE=E->HeadingYawV37;A->Simulate(DT);E->Simulate(DT);
                if(A->CannonShots>0&&FirstA<0)FirstA=Time;if(E->CannonShots>0&&FirstE<0)FirstE=Time;
                Selected|=A->ArmourContactV48.Get()==E;
                ClearHulls&=ArmourHullSeparatedV85(A,A->GetActorTransform(),E,E->GetActorTransform());
                BoundedTurns&=FMath::Abs(FMath::FindDeltaAngleDegrees(YawA,A->HeadingYawV37))<=A->SteeringRateV8311(0,true)*DT+.01f
                    &&FMath::Abs(FMath::FindDeltaAngleDegrees(YawE,E->HeadingYawV37))<=E->SteeringRateV8311(0,true)*DT+.01f;
                if(Frame%30==0){auto J=Sample(A,E,Time);J->SetObjectField(TEXT("enemy"),Sample(E,A,Time));Samples.Add(MakeShared<FJsonValueObject>(J));}
                if(Trial==1&&A->HullIntegrity<HullA&&E->HullIntegrity<HullE)break;
            }
            LegacyForwardOnlyAcquisitionV85=false;
            if(Trial==0){OldShotsA=A->CannonShots;OldShotsE=E->CannonShots;OldTravelA=FVector::Dist2D(Start,A->GetActorLocation());OldSelected=Selected;}
            else
            {
                ++RearWitnesses;
                Check(Name+TEXT("_old_contact_selected_but_cannon_acquisition_cannot_complete"),OldSelected&&OldShotsA==0&&OldShotsE==0&&OldTravelA>100.f);
                Check(Name+TEXT("_actual_checked_pivots_reacquire_both_guns_and_real_damage"),FirstA>0&&FirstE>0&&A->HullIntegrity<HullA&&E->HullIntegrity<HullE);
                Check(Name+TEXT("_actual_hulls_separated_and_stationary_turn_rate_retained"),ClearHulls&&BoundedTurns);
            }
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("legacy_forward_only"),Trial==0);
            J->SetNumberField(TEXT("first_player_shell"),FirstA);J->SetNumberField(TEXT("first_enemy_shell"),FirstE);J->SetNumberField(TEXT("player_travel_cm"),FVector::Dist2D(Start,A->GetActorLocation()));
            J->SetNumberField(TEXT("player_hull_loss"),HullA-A->HullIntegrity);J->SetNumberField(TEXT("enemy_hull_loss"),HullE-E->HullIntegrity);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
        }
    }
    Check(TEXT("v85_two_directions_six_cannon_models_two_fixed_terrain_approaches"),Approaches==24);
    Check(TEXT("v85_four_tracked_models_two_directions_paired_rear_witnesses"),RearWitnesses==8);
    for(int32 Nation=0;Nation<2;++Nation)for(auto Type:{EFrontlineVehicle::Austin1918,EFrontlineVehicle::Whippet,EFrontlineVehicle::MarkIVFemale,EFrontlineVehicle::A7VGretchen})
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);
        auto* A=NewTank(Type,0,FVector(-Dir*2300.f,-700.f,0));auto* E=NewTank(EFrontlineVehicle::A7V,1,FVector(Dir*2300.f,700.f,0));
        if(!A||!E){Check(TEXT("v85_mg_only_assets"),false);continue;}
        const FVector Start=A->GetActorLocation();for(int32 F=0;F<60;++F){B->Elapsed+=.05f;A->Simulate(.05f);}
        Check(FString::Printf(TEXT("v85_n%d_model%d_real_mg_limitation_and_forward_travel"),Nation,int32(Type)),A->Cannons.IsEmpty()&&A->CannonShots==0
            &&FVector::Dist2D(Start,A->GetActorLocation())>100.f);
    }
    Report->SetNumberField(TEXT("approaches"),Approaches);Report->SetNumberField(TEXT("rear_witnesses"),RearWitnesses);Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Began);
    Report->SetStringField(TEXT("scope"),TEXT("Actual UK/DE production meshes, fixed current terrain starts, authored initial clocks and poses, simultaneous Simulate, legal muzzle gates and ordinary shell RNG/flight. Exact old/new rear-acquisition witness. Real mesh OBB separation, no forced hits, no campaign acceptance or FR/RU runs."));
    return Report;
}
};
TSharedRef<FJsonObject> RunFrontlineArmourApproachV85Review(UFrontlineVehicleSubsystem* Fleet)
{return FFrontlineArmourApproachReviewV85::Run(Fleet);}

