// A production Simulate fixture. The old selector is available only inside
// this scoped review; it is never an end-user command or combat setting.
struct FFrontlineArmourContactReviewV84414
{
static TSharedRef<FJsonObject> Run(UFrontlineVehicleSubsystem* Self)
{
    const auto GetWorld=[Self]{return Self->GetWorld();};const auto Battle=[Self]{return Self->Battle();};
    auto& Vehicles=Self->Vehicles;auto& NextSerial=Self->NextSerial;auto& NextEnemyAt=Self->NextEnemyAt;auto& LastEncounter=Self->LastEncounter;
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](const FString& Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    ON_SCOPE_EXIT{LegacyContactChoiceV84414=false;Report->SetBoolField(TEXT("success"),Good);Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);};
    auto* B=Battle();FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84414/Runtime"));FPaths::NormalizeDirectoryName(Root);
    const FString Stamp=User.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)?User.Mid(Root.Len()+1):FString();
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV84414Focused"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&!FPaths::IsRelative(User)&&!User.Contains(TEXT(".."))&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    Check(TEXT("v84414_contact_exact_disposable_ukde_world"),Safe);if(!Safe)return Report;
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
    const auto Clear=[&]{for(TActorIterator<AActor> It(GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();Vehicles.Reset();B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Tracers.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=1000;NextEnemyAt=MAX_flt;LegacyContactChoiceV84414=false;FrontlineMobilityV41::Reset(B);};
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
        for(float& C:V->CannonClock)C=0.f;for(float& C:V->GunClock)C=10000.f;return V;
    };
    const double Began=FPlatformTime::Seconds();int32 Duels=0;
    const EFrontlineVehicle Models[]={EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkVMale,EFrontlineVehicle::MarkVStar,EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::RailConvoy};
    for(int32 Nation=0;Nation<2;++Nation)for(int32 Difficulty=0;Difficulty<3;++Difficulty)for(auto Type:Models)
    {
        Clear();Campaign(Nation,Difficulty);const int32 Dir=B->GetTeamDirection(0);const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        auto* A=NewTank(Type,0,FVector(Bank-Dir*1800.f,-300.f,0));auto* E=NewTank(Type==EFrontlineVehicle::A7V?EFrontlineVehicle::MarkIVMale:EFrontlineVehicle::A7V,1,FVector(Bank+Dir*1800.f,300.f,0));
        const FString Name=FString::Printf(TEXT("v84414_n%d_d%d_model%d"),Nation,Difficulty,int32(Type));
        Check(Name+TEXT("_actual_meshes_and_cannons"),A&&E&&A->AreAssetsReady()&&E->AreAssetsReady()&&!A->Cannons.IsEmpty()&&!E->Cannons.IsEmpty());if(!A||!E)continue;
        bool Setup=false;FVector P;
        // Grounded field/trench approaches, not an alternate flat combat model.
        for(float Offset:{0.f,-1500.f,1500.f})for(float Separation:{1400.f,1800.f,2400.f})if(!Setup)
        {
            Place(A,FVector(Bank+Offset-Dir*Separation,-300.f,0));Place(E,FVector(Bank+Offset+Dir*Separation,300.f,0));bool ReadyA=false,ReadyE=false;
            for(int32 I=0;I<A->CannonMounts.Num();++I)ReadyA|=A->ArmourFiringSolutionV8219(E,I,P,true);
            for(int32 I=0;I<E->CannonMounts.Num();++I)ReadyE|=E->ArmourFiringSolutionV8219(A,I,P,true);
            Setup=ReadyA&&ReadyE&&A->CanOccupyV48(A->GetActorLocation(),A->HeadingYawV37)
                &&E->CanOccupyV48(E->GetActorLocation(),E->HeadingYawV37);
        }
        Check(Name+TEXT("_both_production_muzzles_have_real_firing_solution"),Setup);if(!Setup)continue;
        const FVector AStart=A->GetActorLocation(),EStart=E->GetActorLocation();float FirstA=-1,FirstE=-1;bool ClearBodies=true,Finite=true;
        TArray<TSharedPtr<FJsonValue>> Samples;
        for(int32 Frame=0;Frame<160&&(FirstA<0||FirstE<0);++Frame)
        {
            const float DT=Frame%7==0?.10f:1.f/30.f;B->Elapsed+=DT;A->Simulate(DT);E->Simulate(DT);
            if(A->CannonShots>0&&FirstA<0)FirstA=B->Elapsed-1000;if(E->CannonShots>0&&FirstE<0)FirstE=B->Elapsed-1000;
            ClearBodies&=A->CanOccupyV48(A->GetActorLocation(),A->HeadingYawV37)&&E->CanOccupyV48(E->GetActorLocation(),E->HeadingYawV37);
            Finite&=!A->GetActorTransform().ContainsNaN()&&!E->GetActorTransform().ContainsNaN();
            if(Frame%10==0){auto J=A->DescribeMobilityV839();J->SetNumberField(TEXT("time"),B->Elapsed-1000);Samples.Add(MakeShared<FJsonValueObject>(J));}
        }
        ++Duels;auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("first_player_shell"),FirstA);J->SetNumberField(TEXT("first_enemy_shell"),FirstE);
        J->SetNumberField(TEXT("player_travel_cm"),FVector::Dist2D(AStart,A->GetActorLocation()));J->SetNumberField(TEXT("enemy_travel_cm"),FVector::Dist2D(EStart,E->GetActorLocation()));J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
        Check(Name+TEXT("_both_hold_the_real_arc_and_commit_shells"),FirstA>=0&&FirstE>=0&&FVector::Dist2D(AStart,A->GetActorLocation())<1.f&&FVector::Dist2D(EStart,E->GetActorLocation())<1.f&&A->MobilityRequestedV839==0.f&&E->MobilityRequestedV839==0.f);
        Check(Name+TEXT("_finite_poses_and_physical_body_clearance"),Finite&&ClearBodies);
    }
    Check(TEXT("v84414_three_difficulties_two_armies_six_cannon_models"),Duels==36);
    for(int32 Nation=0;Nation<2;++Nation)
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        auto* A=NewTank(EFrontlineVehicle::A7V,0,FVector(Bank-Dir*1800.f,0,0));auto* Rear=NewTank(EFrontlineVehicle::Whippet,1,FVector(Bank-Dir*2600.f,300.f,0));auto* Front=NewTank(EFrontlineVehicle::MarkIVMale,1,FVector(Bank+Dir*600.f,300.f,0));
        const FString Name=FString::Printf(TEXT("v84414_n%d_nearest_unusable"),Nation);Check(Name+TEXT("_actual_three_hulls"),A&&Rear&&Front);if(!A||!Rear||!Front)continue;
        FVector Aim;float Distance=0;LegacyContactChoiceV84414=true;const auto* Old=A->SelectArmourContactV84414(Distance,Aim);LegacyContactChoiceV84414=false;const auto* New=A->SelectArmourContactV84414(Distance,Aim);
        Check(Name+TEXT("_old_rule_witness_and_usable_target_priority"),Old==Rear&&New==Front);
        for(int32 F=0;F<90&&A->CannonShots==0;++F){B->Elapsed+=.05f;A->Simulate(.05f);}
        Check(Name+TEXT("_production_simulate_stops_and_actually_fires_forward"),A->ArmourContactV48.Get()==Front&&A->CannonShots>0&&A->MobilityRequestedV839==0.f);
        Rear->Destroy();Vehicles.Remove(Rear);Front->AddActorWorldOffset(FVector(Dir*16000.f,0,0));const FVector RangeExit=A->GetActorLocation();
        for(int32 F=0;F<100;++F){B->Elapsed+=.05f;A->Simulate(.05f);}
        Check(Name+TEXT("_living_enemy_out_of_range_releases_firing_stop"),Front->IsOperational()&&(A->GetActorLocation().X-RangeExit.X)*Dir>100.f&&A->MobilityRequestedV839>0.f);
        // Remove real opposing actors from this fleet, then allow the existing
        // observation memory to expire: ordinary travel must resume naturally.
        Front->Destroy();Vehicles.Remove(Front);const FVector Before=A->GetActorLocation();
        for(int32 F=0;F<100;++F){B->Elapsed+=.05f;A->Simulate(.05f);}
        Check(Name+TEXT("_no_enemy_restores_normal_forward_travel"),(A->GetActorLocation().X-Before.X)*Dir>100.f&&A->MobilityRequestedV839>0.f);
    }
    for(int32 Nation=0;Nation<2;++Nation)
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        auto* A=NewTank(EFrontlineVehicle::MarkIVMale,0,FVector(Bank-Dir*2000.f,-500.f,0));auto* E=NewTank(EFrontlineVehicle::A7V,1,FVector(Bank-Dir*1900.f,800.f,0));
        auto* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();const FString Name=FString::Printf(TEXT("v84414_n%d_cannon_acquisition_priority"),Nation);
        Check(Name+TEXT("_actual_vehicle_and_infantry_actors"),A&&E&&U);if(!A||!E||!U)continue;
        U->Initialize(B,8441401,1,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;B->Units.Add(U);
        FFrontlineArmourMobilityAccessV84411::DelayWeapons(U);U->ReturnToIdle();
        // The existing loss of MG mounts leaves the real side cannons as the
        // available anti-infantry weapons: both contacts seek a different yaw.
        for(float& G:A->GunIntegrity)G=0;for(float& C:A->CannonClock)C=10000.f;
        bool Witness=false;float ArmourYaw=0,InfantryYaw=0;FVector Start=A->GetActorLocation(),EnemyStart=FVector::ZeroVector,PersonPoint;
        for(float Ahead:{50.f,100.f,150.f,250.f})for(float Across:{700.f,800.f,900.f})for(float PersonAhead:{0.f,50.f,100.f})if(!Witness)
        {
            Place(A,Start);EnemyStart=FVector(Start.X+Dir*Ahead,Across,0);Place(E,EnemyStart);
            PersonPoint=FVector(Start.X+Dir*PersonAhead,-900.f,0);PersonPoint.Z=FrontlineInfantryV45::SurfaceHeight(B,PersonPoint);U->SetActorLocation(PersonPoint+FVector(0,0,88));
            U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(.05f,false);U->Soldier->RefreshBoneTransforms();
            FVector Seen;bool PresentArmour=false,PresentInfantry=false;
            for(int32 I=0;I<A->CannonMounts.Num();++I){FVector P;PresentArmour|=A->ArmourFiringSolutionV8219(E,I,P,true);PresentInfantry|=A->InfantryFiringSolutionV828(U,I,true,P,nullptr,true);}
            Witness=!PresentArmour&&!PresentInfantry&&A->ArmourSightV48(E,Seen)&&A->MountedHeadingV8448(Seen,ArmourYaw,true,E)
                &&A->MountedHeadingV8448(U->Feet()+FVector(0,0,105),InfantryYaw,false,U)&&FMath::Abs(FMath::FindDeltaAngleDegrees(ArmourYaw,InfantryYaw))>3.f;
        }
        Check(Name+TEXT("_real_opposing_chassis_acquisition_yaws_found"),Witness);if(!Witness)continue;
        Place(A,Start);Place(E,EnemyStart);LegacyContactChoiceV84414=true;A->Simulate(.05f);const float OldYaw=A->RollingAcquisitionYawV8448;
        Place(A,Start);Place(E,EnemyStart);LegacyContactChoiceV84414=false;A->Simulate(.05f);const float NewYaw=A->RollingAcquisitionYawV8448;
        Check(Name+TEXT("_higher_priority_armour_yaw_survives_nearby_infantry"),A->ArmourContactV48.Get()==E&&A->bRollingAcquisitionV8448
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw,ArmourYaw))<.1f&&FMath::Abs(FMath::FindDeltaAngleDegrees(OldYaw,ArmourYaw))>3.f);
        const FVector AcquisitionStart=A->GetActorLocation();const float EnemyHull=E->HullIntegrity;bool ArmourShell=false;float ArmourAt=-1.f;
        for(float& C:A->CannonClock)C=0.f;
        // Continue that same unready real pose through ordinary movement,
        // traverse, muzzle alignment and shell flight. No orientation or
        // placement is assigned after the initial witness frame.
        for(int32 F=0;F<400&&!ArmourShell;++F)
        {
            const float DT=F%9==0?.10f:1.f/30.f;B->Elapsed+=DT;A->Simulate(DT);
            ArmourShell=E->HullIntegrity<EnemyHull;
            if(ArmourShell)ArmourAt=B->Elapsed-1000.f;
        }
        Check(Name+TEXT("_natural_rolling_acquisition_stops_fires_and_hits_actual_armour"),ArmourShell&&ArmourAt>=0.f
            &&A->ArmourContactV48.Get()==E&&A->MobilityRequestedV839==0.f&&FVector::Dist2D(AcquisitionStart,A->GetActorLocation())>1.f);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("proved_armour_heading"),ArmourYaw);J->SetNumberField(TEXT("proved_infantry_heading"),InfantryYaw);J->SetNumberField(TEXT("old_committed_heading"),OldYaw);J->SetNumberField(TEXT("current_committed_heading"),NewYaw);Cases.Add(MakeShared<FJsonValueObject>(J));
        J->SetNumberField(TEXT("armour_hit_at"),ArmourAt);J->SetNumberField(TEXT("armour_hull_loss"),EnemyHull-E->HullIntegrity);J->SetNumberField(TEXT("natural_chassis_travel_cm"),FVector::Dist2D(AcquisitionStart,A->GetActorLocation()));
        float OldMGRequested=0.f,OldMGTravel=0.f;
        for(int32 Trial=0;Trial<2;++Trial)
        {
            // A separate natural initial state: authored cannon/MG poses,
            // every MG intact, the same proved opposing vehicle and infantry.
            // The old trial proves the lower-priority infantry hold; only the
            // corrected trial continues into normal gun fire and shell flight.
            Clear();Campaign(Nation,1);
            auto* Player=NewTank(EFrontlineVehicle::MarkIVMale,0,Start);
            auto* Enemy=NewTank(EFrontlineVehicle::A7V,1,EnemyStart);
            auto* Person=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
            const FString MGName=Name+TEXT("_intact_mg");
            Check(MGName+TEXT("_actual_initial_actors"),Player&&Enemy&&Person);if(!Player||!Enemy||!Person)continue;
            Person->Initialize(B,8441402,1,EFrontlineRole::Rifle,0);Person->SetActorTickEnabled(false);Person->bFieldEntered=true;
            B->Units.Add(Person);FFrontlineArmourMobilityAccessV84411::DelayWeapons(Person);Person->ReturnToIdle();
            // The side position used by the competing-yaw trial can sit below
            // the physical MG pitch limit. Choose an actually exposed contact
            // from eight finite field positions, not an eye-only observation.
            FVector Point;bool CannonReady=false,MGReady=false;FVector MGFeet=FVector::ZeroVector;
            for(const FVector& Local:{FVector(800,0,0),FVector(1200,0,0),FVector(800,-600,0),FVector(800,600,0),
                FVector(0,-900,0),FVector(0,900,0),FVector(-800,600,0),FVector(-800,-600,0)})if(!MGReady)
            {
                MGFeet=Player->GetActorTransform().TransformPosition(Local);MGFeet.Z=FrontlineInfantryV45::SurfaceHeight(B,MGFeet);
                Person->SetActorLocation(MGFeet+FVector(0,0,Person->GroundContactOffsetV82));Person->Soldier->HandleExistingParallelEvaluationTask(true,true);
                Person->Soldier->TickAnimation(.05f,false);Person->Soldier->RefreshBoneTransforms();
                for(int32 I=0;I<Player->GunMounts.Num();++I)MGReady|=Player->InfantryFiringSolutionV828(Person,I,false,Point,nullptr,true);
            }
            for(int32 I=0;I<Player->CannonMounts.Num();++I)CannonReady|=Player->ArmourFiringSolutionV8219(Enemy,I,Point,true);
            float Yaw=0.f;Check(MGName+TEXT("_unready_cannon_feasible_roll_and_ready_mg"),
                !CannonReady&&MGReady&&Player->MountedHeadingV8448(Enemy->AimPoint(),Yaw,true,Enemy)
                &&Player->GunIntegrity.ContainsByPredicate([](float G){return G>0.f;}));
            for(float& Timer:Player->CannonClock)Timer=10000.f;
            const FVector Initial=Player->GetActorLocation();LegacyContactChoiceV84414=Trial==0;
            Player->Simulate(.05f);const float Travel=FVector::Dist2D(Initial,Player->GetActorLocation());
            if(Trial==0)
            {
                OldMGRequested=Player->MobilityRequestedV839;OldMGTravel=Travel;
                Check(MGName+TEXT("_old_lower_priority_infantry_hold_witness"),Player->bRollingAcquisitionV8448&&OldMGRequested==0.f&&OldMGTravel<.01f);
                LegacyContactChoiceV84414=false;continue;
            }
            Check(MGName+TEXT("_new_priority_permits_actual_rolling"),Player->bRollingAcquisitionV8448&&Player->MobilityRequestedV839>0.f&&Travel>.01f);
            for(float& GunTimer:Player->GunClock)GunTimer=0.f;
            for(int32 Frame=0;Frame<30&&Player->MachinegunShots==0;++Frame){B->Elapsed+=.05f;Player->Simulate(.05f);}
            for(float& Timer:Player->CannonClock)Timer=0.f;
            const float Hull=Enemy->HullIntegrity;float HitAt=-1.f;
            for(int32 Frame=0;Frame<400&&(HitAt<0.f||Player->MachinegunShots==0);++Frame)
            {
                const float DT=Frame%9==0?.10f:1.f/30.f;B->Elapsed+=DT;Player->Simulate(DT);
                if(Enemy->HullIntegrity<Hull&&HitAt<0.f)HitAt=B->Elapsed-1000.f;
            }
            Check(MGName+TEXT("_natural_independent_mg_fire_and_actual_armour_hit"),
                Player->MachinegunShots>0&&Player->CannonShots>0&&HitAt>=0.f&&Player->ArmourContactV48.Get()==Enemy);
            auto Detail=MakeShared<FJsonObject>();Detail->SetStringField(TEXT("name"),MGName);
            Detail->SetNumberField(TEXT("old_first_travel_cm"),OldMGTravel);Detail->SetNumberField(TEXT("old_requested_cm"),OldMGRequested);
            Detail->SetNumberField(TEXT("new_first_travel_cm"),Travel);Detail->SetNumberField(TEXT("new_armour_hit_at"),HitAt);
            Detail->SetStringField(TEXT("actual_exposed_mg_contact"),MGFeet.ToString());
            Detail->SetNumberField(TEXT("new_machinegun_shots"),Player->MachinegunShots);Detail->SetNumberField(TEXT("new_armour_hull_loss"),Hull-Enemy->HullIntegrity);
            Cases.Add(MakeShared<FJsonValueObject>(Detail));
        }
    }
    for(int32 Nation=0;Nation<2;++Nation)
    {
        Clear();Campaign(Nation,1);const int32 Dir=B->GetTeamDirection(0);const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        auto* A=NewTank(EFrontlineVehicle::RailConvoy,0,FVector(Bank-Dir*1800.f,-600.f,0));auto* E=NewTank(EFrontlineVehicle::A7V,1,FVector(Bank+Dir*1200.f,600.f,0));
        if(!A||!E){Check(TEXT("v84414_disabled_cannon_actual_actors"),false);continue;}
        A->RailCarIntegrityV837_[0]=A->RailCarIntegrityV837_[4]=0.f;const FVector Before=A->GetActorLocation();
        for(int32 F=0;F<100;++F){B->Elapsed+=.05f;A->Simulate(.05f);}
        Check(FString::Printf(TEXT("v84414_n%d_all_real_cannon_modules_destroyed_no_idle_lock"),Nation),!A->Cannons.IsEmpty()
            &&!A->RailCannonOperationalV837(0)&&!A->RailCannonOperationalV837(1)&&A->CannonShots==0
            &&(A->GetActorLocation().X-Before.X)*Dir>100.f&&A->MobilityRequestedV839>0.f);
    }
    // Machine-gun-only chassis retain their real limitation and normal travel;
    // the repair must not manufacture a cannon or permanent threat stop.
    for(auto Type:{EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918,EFrontlineVehicle::MarkIVFemale,EFrontlineVehicle::A7VGretchen})
    {
        Clear();Campaign(0,0);const int32 Dir=B->GetTeamDirection(0);const float Bank=B->Trenches.IsEmpty()?0.f:FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f);
        auto* A=NewTank(Type,0,FVector(Bank-Dir*2400.f,-600.f,0));auto* E=NewTank(EFrontlineVehicle::A7V,1,FVector(Bank+Dir*1200.f,600.f,0));if(!A||!E){Check(TEXT("v84414_machinegun_chassis_assets"),false);continue;}
        const FVector Before=A->GetActorLocation();for(int32 F=0;F<90;++F){B->Elapsed+=.05f;A->Simulate(.05f);}
        Check(FString::Printf(TEXT("v84414_model%d_no_invented_antiarmour_weapon_or_idle_lock"),int32(Type)),A->Cannons.IsEmpty()&&A->CannonShots==0&&(A->GetActorLocation().X-Before.X)*Dir>100.f);
    }
    Report->SetNumberField(TEXT("duels"),Duels);Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Began);
    Report->SetStringField(TEXT("scope"),TEXT("Actual UK/DE configured meshes, grounded production cannon arcs, simultaneous Simulate and committed shell flight. Three difficulties, six cannon models, nearest-unusable target witnesses, live-target removal and four machine-gun-only controls. No paused national campaign or complete-match balance claim."));
    return Report;
}
};
TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunContactChecksV84414()
{return FFrontlineArmourContactReviewV84414::Run(this);}
