static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;
    int32 CurrentGrenadeLandings=0,CurrentBuiltBankLandings=0,LegacyEmbeddedLandings=0;
    const auto Assert=[&](const FString& N,bool P){Good&=P;Check(N,P);};
    FString User;const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV835Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User)
        &&!WarfareFRRUCampaignV82::Active(B);
    Assert(TEXT("v835_contact_disposable_ukde_world"),Safe);if(!Safe){R->SetBoolField(TEXT("success"),false);return R;}
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());bool EmptyFleet=true;
    if(Fleet)for(const auto& V:Fleet->GetVehicles())if(IsValid(V.Get())&&V->IsOperational())EmptyFleet=false;
    Assert(TEXT("v835_contact_starts_without_live_armour"),EmptyFleet);if(!EmptyFleet){R->SetBoolField(TEXT("success"),false);return R;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;
    const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;
    const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
    const int32 BoundMission=B->BoundCampaignMission,NextId=B->NextSquadId,Winner=B->BattleOutroWinner,Shots=B->ShotsFired,Thrown=B->GrenadesThrown;
    const auto Random=B->Random;const auto Phase=B->Phase;const float Clock=B->Elapsed,Peak=B->PeakSuppression,Friendly=B->FriendlyFireDamage;
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
    const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]};const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
    const bool OldComparison=LegacyContactComparisonV835;
    TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
    struct FHidden{TWeakObjectPtr<AWarfareFrontlineSoldier> U;bool Hidden,Collision;};TArray<FHidden> Hidden;
    for(AWarfareFrontlineSoldier* U:Units)if(IsValid(U)){Hidden.Add({U,U->IsHidden(),U->GetActorEnableCollision()});U->SetActorHiddenInGame(true);U->SetActorEnableCollision(false);}
    const auto Clear=[&]()
    {
        for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
        B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();
        FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
    };
    ON_SCOPE_EXIT
    {
        Clear();LegacyContactComparisonV835=OldComparison;
        B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;
        B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=BoundMission;
        B->NextSquadId=NextId;B->BattleOutroWinner=Winner;B->ShotsFired=Shots;B->GrenadesThrown=Thrown;B->Random=Random;B->Phase=Phase;B->Elapsed=Clock;B->PeakSuppression=Peak;B->FriendlyFireDamage=Friendly;
        B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;
        for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];}
        for(const auto& H:Hidden)if(auto* U=H.U.Get()){U->SetActorHiddenInGame(H.Hidden);U->SetActorEnableCollision(H.Collision);}
        R->SetBoolField(TEXT("success"),Good);R->SetArrayField(TEXT("cases"),Rows);
        R->SetStringField(TEXT("scope"),TEXT("UK/DE current authored ground: held rifle/MG crews at home and centre trenches; matched 30-second rifle/grenadier attacks in open, held-trench and smoke approaches. Only fixture infantry Tick, evaluated skeletal poses and committed hand grenades advance. Frozen trench targets establish firing eligibility, not duelling balance. Actual damage, throws, losses and shared-queue readiness are recorded; no whole-campaign claim."));
    };
    const auto Ground=[&](const FVector& P){return FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P));};
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();if(auto* C=FrontlineMachinegunsV30::Get(U))C->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const auto Make=[&](int32 Team,int32 Id,EFrontlineRole Role,int32 Member,const FVector& P,int32 Seed)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
        U->MotionRandom.Initialize(Seed+Member*101+Team*197);U->SetActorLocation(Ground(P)+FVector(0,0,88));
        U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));U->SetMotion(U->GetAimMotion());
        B->Units.Add(U);Pose(U,.3f);return U;
    };
    const auto BeginCase=[&](int32 Seed)
    {
        Clear();B->Trenches=Trenches;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;B->Random.Initialize(Seed);
        B->ShotsFired=B->GrenadesThrown=0;B->FriendlyFireDamage=0;for(int32 T=0;T<2;++T){B->Morale[T]=100;B->Casualties[T]=0;B->DamageDealt[T]=0;B->CombatRecordV65[T]={};}
    };
    // Comparison modes use identical actor IDs, private shot/throw streams and
    // production geometry. They are labelled even when an unchanged rule gives
    // identical results; this is never a second event-generating acquisition.
    for(int32 Mode=0;Mode<2;++Mode)for(int32 Nation=0;Nation<2;++Nation)
    {
        LegacyContactComparisonV835=Mode==0;B->PlayerFaction=Nation?EFrontlineFaction::German:EFrontlineFaction::British;
        FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=B->PlayerFaction;Run.CompletedMissions=Run.SelectedMission=19;
        B->CampaignRuns={Run};B->ActiveCampaignId=B->BoundCampaignId=Run.Id;B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=19;
        const int32 Dir=B->GetTeamDirection(0),Home=B->GetHomeTrenchIndex(0),Centre=B->Trenches.Num()/2;
        // Isolate the range formula from terrain while retaining the real
        // acquisition, shot and collision routes. This high clear plane is a
        // rule probe, never represented as grounded encounter acceptance.
        BeginCase(83511);
        auto* Near=Make(0,835001+Nation*10,EFrontlineRole::Rifle,0,FVector(0,550,0),83511);
        auto* Far=Make(0,835002+Nation*10,EFrontlineRole::Rifle,0,FVector(0,-550,0),83511);
        auto* Target=Make(1,835003+Nation*10,EFrontlineRole::Rifle,0,FVector(0,550,0),83511);
        if(Near&&Far&&Target)
        {
            const float Range=FrontlineWeapons::ProfileFor(Near).FireRangeCm*B->EncounterVisibilityScale();
            Near->SetActorLocation(FVector(0,550,10088));Far->SetActorLocation(FVector(0,-550,10088));Target->SetActorLocation(FVector(Dir*Range*.99f,550,10088));
            for(auto* U:{Near,Far,Target})Pose(U,.1f);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("isolated_range_rule"));J->SetStringField(TEXT("mode"),Mode?TEXT("current"):TEXT("v834_legacy"));J->SetStringField(TEXT("army"),Nation?TEXT("de"):TEXT("uk"));
            J->SetNumberField(TEXT("along_front_cm"),Range*.99f);J->SetNumberField(TEXT("near_physical_distance_cm"),FVector::Dist2D(Near->Feet(),Target->Feet()));J->SetNumberField(TEXT("far_physical_distance_cm"),FVector::Dist2D(Far->Feet(),Target->Feet()));
            J->SetBoolField(TEXT("near_acquires"),B->AcquireTarget(Near)==Target);J->SetBoolField(TEXT("far_acquires"),B->AcquireTarget(Far)==Target);
            J->SetNumberField(TEXT("near_hit_chance"),B->BulletHitChance(Near,Target));J->SetNumberField(TEXT("far_hit_chance"),B->BulletHitChance(Far,Target));
            Assert(FString::Printf(TEXT("v835_mode%d_%s_range_rule_obeys_its_exact_route"),Mode,Nation?TEXT("de"):TEXT("uk")),
                B->AcquireTarget(Near)==Target&&(Mode?(B->AcquireTarget(Far)==Target&&B->BulletHitChance(Far,Target)>0.f):!B->AcquireTarget(Far)));
            auto* Blocker=B->GetWorld()->SpawnActor<AActor>();auto* Wall=Blocker?NewObject<UBoxComponent>(Blocker):nullptr;
            if(Wall){Blocker->SetRootComponent(Wall);Wall->SetBoxExtent(FVector(15,1800,230));Wall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Wall->SetCollisionResponseToAllChannels(ECR_Ignore);Wall->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Wall->RegisterComponent();Blocker->SetActorLocation(FVector(Dir*Range*.5f,0,10140));}
            Assert(FString::Printf(TEXT("v835_mode%d_%s_range_probe_keeps_actual_wall_occlusion"),Mode,Nation?TEXT("de"):TEXT("uk")),Wall&&!B->HasSight(Near,Target)&&!B->HasSight(Far,Target)&&!B->AcquireTarget(Near)&&!B->AcquireTarget(Far));
            Rows.Add(MakeShared<FJsonValueObject>(J));
        }
        if(Mode==1&&B->Trenches.IsValidIndex(Home))
        {
            BeginCase(83521);const int32 CrossId=835250+Nation;B->Trenches[Home].Owner=0;B->Trenches[Home].bBypass=true;
            FFrontlineSquad Crossing;Crossing.Id=CrossId;Crossing.Team=0;Crossing.Role=EFrontlineRole::Lewis;Crossing.Trench=INDEX_NONE;Crossing.Destination=INDEX_NONE;Crossing.bAdvancing=true;Crossing.Slot=1;B->Squads.Add(Crossing);
            const FVector FloorPoint(FrontlineFieldV32::TrenchCentreX(Home,0),0,0);
            auto* CrossingGunner=Make(0,CrossId,EFrontlineRole::Lewis,0,FloorPoint,83521);
            auto* CrossingEnemy=Make(1,CrossId+500,EFrontlineRole::Rifle,0,FloorPoint+FVector(Dir*650.f,0,0),83561);
            if(CrossingGunner){CrossingGunner->ShotClock=10000;B->FindSquad(CrossId)->Members.Add(CrossingGunner);}
            auto* CrossingGun=FrontlineMachinegunsV30::Get(CrossingGunner);bool SawTrench=false,SawProne=false;
            const bool ActualContact=CrossingGunner&&CrossingEnemy&&B->HasSight(CrossingGunner,CrossingEnemy)&&B->AcquireTarget(CrossingGunner,true)==CrossingEnemy;
            if(CrossingGun){for(int32 F=0;F<6;++F)CrossingGun->Update(.1f,true);Assert(FString::Printf(TEXT("v835_%s_true_movement_intent_never_deploys_in_a_passed_trench"),Nation?TEXT("de"):TEXT("uk")),CrossingGun->State==EFrontlineGunStanceV30::Carry);}
            auto* BankMan=Make(0,CrossId+1,EFrontlineRole::Lewis,0,FloorPoint+FVector(Dir*300.f,0,0),83551);
            auto* BankGun=FrontlineMachinegunsV30::Get(BankMan);if(BankGun){for(int32 F=0;F<6;++F)BankGun->Update(.1f,false);Assert(FString::Printf(TEXT("v835_%s_actual_parapet_is_not_classified_as_trench_floor"),Nation?TEXT("de"):TEXT("uk")),!BankGun->IsTrench());}
            if(ActualContact)for(int32 F=0;F<120;++F){B->Elapsed+=.05f;CrossingGunner->Tick(.05f);Pose(CrossingGunner,.05f);if(CrossingGun->IsSupported()){SawTrench|=CrossingGun->IsTrench();SawProne|=CrossingGun->IsProne();}}
            auto Cross=MakeShared<FJsonObject>();Cross->SetStringField(TEXT("kind"),TEXT("bypassed_trench_pose"));Cross->SetStringField(TEXT("army"),Nation?TEXT("de"):TEXT("uk"));Cross->SetBoolField(TEXT("actual_contact"),ActualContact);Cross->SetBoolField(TEXT("supported_in_trench"),SawTrench);Cross->SetBoolField(TEXT("ever_supported_prone"),SawProne);
            Cross->SetNumberField(TEXT("assignment_trench"),B->FindSquad(CrossId)->Trench);Cross->SetBoolField(TEXT("still_advancing"),B->FindSquad(CrossId)->bAdvancing);Cross->SetBoolField(TEXT("bypass_unchanged"),B->Trenches[Home].bBypass);Rows.Add(MakeShared<FJsonValueObject>(Cross));
            Assert(FString::Printf(TEXT("v835_%s_actual_bypassed_trench_contact_uses_trench_pose_without_assigning_a_slot"),Nation?TEXT("de"):TEXT("uk")),ActualContact&&CrossingGun&&SawTrench&&!SawProne&&B->FindSquad(CrossId)->Trench==INDEX_NONE&&B->FindSquad(CrossId)->bAdvancing&&B->Trenches[Home].bBypass);
            BeginCase(83523);const int32 Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=835300+Nation;
            const FVector Station((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
            FFrontlineSquad Crew;Crew.Id=Id;Crew.Team=0;Crew.Role=EFrontlineRole::Lewis;Crew.bAdvancing=true;Crew.Destination=Next;Crew.Slot=1;B->Squads.Add(Crew);
            TArray<AWarfareFrontlineSoldier*> Men;for(int32 M=0;M<3;++M)if(auto* U=Make(0,Id,EFrontlineRole::Lewis,M,Station+FVector(-Dir*M*40.f,M==1?320.f:M==2?-320.f:0.f,0),83523))
            {U->ShotClock=10000;B->FindSquad(Id)->Members.Add(U);Men.Add(U);}
            auto* Foe=Make(1,Id+500,EFrontlineRole::Rifle,0,Station+FVector(Dir*650.f,0,0),83563);
            for(int32 F=0;F<180;++F){B->Elapsed+=.05f;for(auto* U:Men){U->Tick(.05f);Pose(U,.05f);}}
            auto* Gun=Men.Num()==3?FrontlineMachinegunsV30::Get(Men[0]):nullptr;
            const bool Planted=Foe&&Gun&&Gun->IsSupported()&&B->HasSight(Men[0],Foe);
            int32 Running=0,Reversing=0,BadFacing=0,Moving=0,Backsteps=0;float FinalGap=-1,ActualTravel=0,BodyGap=MAX_flt,NewStationForward=-MAX_flt,OldStationForward=0;bool Reform=false;
            const bool ForwardStationV836=FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"));
            if(Planted)
            {
                auto* Bearer=Men[1];FVector Goal=FrontlineMachinegunsV30::Destination(Bearer,B->SoldierDestination(Bearer),Reform);
                // Displace only the actor; the existing accepted station and
                // gun placement remain authoritative. Real body/ground checks
                // decide whether and how he returns to that same station.
                Bearer->SetActorLocation(Ground(Goal+FVector(Dir*240.f,0,0))+FVector(0,0,88));Pose(Bearer,.1f);
                OldStationForward=(Goal.X-Bearer->Feet().X)*Dir;
                if(ForwardStationV836){Goal=FrontlineMachinegunsV30::Destination(Bearer,B->SoldierDestination(Bearer),Reform);NewStationForward=(Goal.X-Bearer->Feet().X)*Dir;}
                for(int32 F=0;F<160;++F)
                {
                    const FVector Before=Bearer->Feet();B->Elapsed+=.05f;for(auto* U:Men){U->Tick(.05f);Pose(U,.05f);}
                    ActualTravel+=FVector::Dist2D(Before,Bearer->Feet());Backsteps+=(Bearer->Feet().X-Before.X)*Dir<-.05f;
                    for(auto* U:Men)if(U!=Bearer)BodyGap=FMath::Min(BodyGap,float(FVector::Dist2D(U->Feet(),Bearer->Feet())));
                    if(Bearer->bMoving){++Moving;Running+=Bearer->Playing==Bearer->GetRunMotion();BadFacing+=FMath::Abs(FMath::FindDeltaAngleDegrees(Bearer->GetActorRotation().Yaw,Dir>0?0.f:180.f))>100.f;
                        if(auto* Node=Bearer->Soldier->GetSingleNodeInstance())Reversing+=Node->GetPlayRate()<0;}
                }
                FinalGap=FVector::Dist2D(Bearer->Feet(),Goal);
            }
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("ground_crew_reform"));J->SetStringField(TEXT("mode"),TEXT("current"));J->SetStringField(TEXT("army"),Nation?TEXT("de"):TEXT("uk"));
            J->SetBoolField(TEXT("actual_supported_contact"),Planted);J->SetNumberField(TEXT("moving_frames"),Moving);J->SetNumberField(TEXT("run_frames"),Running);J->SetNumberField(TEXT("reverse_walk_frames"),Reversing);J->SetNumberField(TEXT("rear_facing_frames"),BadFacing);J->SetNumberField(TEXT("final_station_gap_cm"),FinalGap);
            J->SetNumberField(TEXT("actual_travel_cm"),ActualTravel);J->SetNumberField(TEXT("backstep_frames"),Backsteps);J->SetNumberField(TEXT("minimum_crew_body_gap_cm"),BodyGap<MAX_flt?BodyGap:-1.f);
            if(ForwardStationV836){J->SetNumberField(TEXT("old_station_forward_cm"),OldStationForward);J->SetNumberField(TEXT("new_station_forward_cm"),NewStationForward);}
            J->SetNumberField(TEXT("deployments"),Gun?Gun->Deployments:0);J->SetNumberField(TEXT("packs"),Gun?Gun->Packs:0);Rows.Add(MakeShared<FJsonValueObject>(J));
            Assert(FString::Printf(TEXT("v835_%s_crew_reform_is_a_real_contact_fixture"),Nation?TEXT("de"):TEXT("uk")),Planted);
            if(ForwardStationV836)Assert(FString::Printf(TEXT("v836_%s_passed_crew_station_chooses_real_forward_safe_position_without_forced_retrace"),Nation?TEXT("de"):TEXT("uk")),
                Planted&&OldStationForward<-200.f&&NewStationForward>=-8.f&&Backsteps==0&&Reversing==0&&Running==0&&BadFacing==0&&FinalGap>=0.f&&FinalGap<=64.f&&BodyGap>=90.f&&Gun->Packs==0);
            else Assert(FString::Printf(TEXT("v835_%s_displaced_crew_returns_by_real_forward_facing_reverse_walk"),Nation?TEXT("de"):TEXT("uk")),
                Planted&&ActualTravel>100.f&&Backsteps>0&&Reversing>0&&Running==0&&BadFacing==0&&FinalGap>=0.f&&FinalGap<=64.f&&BodyGap>=90.f&&Gun->Packs==0);
            if(ForwardStationV836)
            {
                // Remove only the actual contact, then let the same planted
                // section receive its existing forward destination. Do not
                // set gun states, clocks or pose to manufacture a transition.
                if(Foe){B->Units.Remove(Foe);Foe->Destroy();}
                bool SawPack=false,FinishedPack=false,LocomotionOnFinish=false;
                float FinishTravel=0,FinishReaction=-1,ClearLeg=0,FinishClearLeg=0;
                int32 PackTicks=0;const int32 PacksBefore=Gun?Gun->Packs:0;
                if(Planted)
                {
                    auto* Lead=Men[0];const FVector Goal=B->SoldierDestination(Lead);
                    ClearLeg=FVector::Dist2D(Lead->Feet(),FrontlineMobilityV41::Step(Lead,Goal,20.f,false));
                    for(int32 F=0;F<180&&!FinishedPack;++F)
                    {
                        const auto BeforeState=Gun->State;const FVector BeforeFeet=Lead->Feet();
                        B->Elapsed+=.05f;for(auto* U:Men){U->Tick(.05f);Pose(U,.05f);}
                        SawPack|=Gun->State==EFrontlineGunStanceV30::Pack;PackTicks+=Gun->State==EFrontlineGunStanceV30::Pack;
                        if(BeforeState==EFrontlineGunStanceV30::Pack&&Gun->State==EFrontlineGunStanceV30::Carry)
                        {
                            FinishedPack=true;FinishTravel=FVector::Dist2D(BeforeFeet,Lead->Feet());FinishReaction=Lead->DepartureClock;
                            FinishClearLeg=FVector::Dist2D(Lead->Feet(),FrontlineMobilityV41::Step(Lead,B->SoldierDestination(Lead),20.f,false));
                            auto* Node=Lead->Soldier->GetSingleNodeInstance();
                            LocomotionOnFinish=Lead->bMoving&&Node&&(Node->GetCurrentAsset()==Lead->GetWalkMotion()||Node->GetCurrentAsset()==Lead->GetRunMotion());
                        }
                    }
                }
                auto Resume=MakeShared<FJsonObject>();Resume->SetStringField(TEXT("kind"),TEXT("actual_supported_pack_to_forward_locomotion"));
                Resume->SetStringField(TEXT("army"),Nation?TEXT("de"):TEXT("uk"));Resume->SetBoolField(TEXT("initially_supported"),Planted);
                Resume->SetBoolField(TEXT("saw_authored_pack"),SawPack);Resume->SetBoolField(TEXT("pack_finished"),FinishedPack);
                Resume->SetNumberField(TEXT("pack_frames"),PackTicks);Resume->SetNumberField(TEXT("packs_started"),Gun?Gun->Packs-PacksBefore:0);
                Resume->SetNumberField(TEXT("initial_actual_clear_leg_cm"),ClearLeg);Resume->SetNumberField(TEXT("finish_actual_clear_leg_cm"),FinishClearLeg);
                Resume->SetBoolField(TEXT("physically_blocked_on_finish"),FinishedPack&&FinishTravel<=.01f&&FinishClearLeg<=.01f);
                Resume->SetNumberField(TEXT("finish_tick_travel_cm"),FinishTravel);Resume->SetNumberField(TEXT("finish_tick_departure_seconds"),FinishReaction);
                Resume->SetBoolField(TEXT("finish_tick_uses_locomotion"),LocomotionOnFinish);Rows.Add(MakeShared<FJsonValueObject>(Resume));
                Assert(FString::Printf(TEXT("v836_%s_pack_fixture_starts_supported_with_real_clear_forward_leg"),Nation?TEXT("de"):TEXT("uk")),Planted&&ClearLeg>1.f);
                Assert(FString::Printf(TEXT("v836_%s_authored_rise_completes_once_then_moves_without_second_reaction_wait"),Nation?TEXT("de"):TEXT("uk")),
                    SawPack&&FinishedPack&&PackTicks>0&&Gun&&Gun->Packs-PacksBefore==1&&FinishReaction==0.f&&FinishTravel>.01f&&LocomotionOnFinish);
            }
        }
        for(int32 Depth=0;Depth<2;++Depth)for(int32 RoleIndex=0;RoleIndex<2;++RoleIndex)
        {
            BeginCase(83519);const int32 Trench=Depth?Centre:Home,Id=835100+Nation*100+Depth*20+RoleIndex;
            if(!B->Trenches.IsValidIndex(Trench)){Assert(TEXT("v835_contact_has_actual_trench"),false);continue;}
            B->Trenches[Trench].Owner=0;const EFrontlineRole Role=RoleIndex?EFrontlineRole::Lewis:EFrontlineRole::Rifle;
            FFrontlineSquad Q;Q.Id=Id;Q.Team=0;Q.Role=Role;Q.Trench=Q.Destination=Trench;Q.Slot=1;Q.bHoldPosition=true;B->Squads.Add(Q);
            TArray<AWarfareFrontlineSoldier*> Men;for(int32 M=0;M<B->RoleSize(Role,0);++M)
                if(auto* U=Make(0,Id,Role,M,FVector(B->Trenches[Trench].X,(M-2.f)*140.f,0),83519)){B->FindSquad(Id)->Members.Add(U);Men.Add(U);}
            const float ShortStart=RoleIndex&&Depth==0?20.f:0.f;
            for(auto* U:Men){bool Reform=false;const FVector P=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);U->SetActorLocation(Ground(P-FVector(Dir*ShortStart,0,0))+FVector(0,0,88));Pose(U,.1f);}
            for(int32 M=0;M<12;++M)Make(1,Id+800+M,EFrontlineRole::Rifle,0,FVector(B->Trenches[Trench].X+Dir*(950.f+(M%3)*160.f),(M/3-1.5f)*470.f,0),83619+M);
            TArray<int32> TraceClear,Acquired,Ready,MuzzleClear,EyeVisible;TraceClear.Init(0,Men.Num());Acquired=TraceClear;Ready=TraceClear;MuzzleClear=TraceClear;EyeVisible=TraceClear;
            TArray<int32> Unfulfilled,StateChanges;Unfulfilled=TraceClear;StateChanges=TraceClear;TArray<float> Travel;Travel.Init(0,Men.Num());
            TArray<EFrontlineGunStanceV30> PreviousStance;PreviousStance.Init(EFrontlineGunStanceV30::Carry,Men.Num());
            TArray<TSharedPtr<FJsonValue>> BlockedSamples;
            for(int32 F=0;F<280;++F)
            {
                B->Elapsed+=.05f;for(int32 M=0;M<Men.Num();++M)
                {
                    auto* U=Men[M];const FVector Before=U->Feet();U->Tick(.05f);Pose(U,.05f);auto* T=B->AcquireTarget(U);auto* G=FrontlineMachinegunsV30::Get(U);
                    const float Moved=FVector::Dist2D(Before,U->Feet());Travel[M]+=Moved;Unfulfilled[M]+=U->bWantedMovement&&Moved<.05f;
                    if(G){StateChanges[M]+=G->State!=PreviousStance[M];PreviousStance[M]=G->State;}
                    if(T){++Acquired[M];TraceClear[M]+=B->HasSight(U,T);MuzzleClear[M]+=!G||G->HasInfantryFiringLine(T);}
                    FCollisionQueryParams Qry(SCENE_QUERY_STAT(V835ContactProbe),true);for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Qry.AddIgnoredActor(Other);
                    for(AWarfareFrontlineSoldier* Other:B->Units)if(Other&&Other->Team!=U->Team&&Other->IsActiveCombatant())
                    {
                        const FVector Head=Other->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5);FHitResult Hit;
                        const bool EyeClear=!B->GetWorld()->LineTraceSingleByChannel(Hit,U->Feet()+FVector(0,0,145),Head,ECC_Visibility,Qry);
                        if(EyeClear){++EyeVisible[M];if(!T&&BlockedSamples.Num()<3)
                        {
                            const FVector Origin=FrontlineMachinegunsV30::SightOrigin(U);const bool Blocked=B->GetWorld()->LineTraceSingleByChannel(Hit,Origin,Head,ECC_Visibility,Qry);
                            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("member"),M);J->SetNumberField(TEXT("at_seconds"),F*.05f);J->SetBoolField(TEXT("origin_blocked"),Blocked);
                            J->SetStringField(TEXT("origin"),Origin.ToString());J->SetStringField(TEXT("head"),Head.ToString());J->SetStringField(TEXT("muzzle"),U->GetMuzzleLocation().ToString());
                            J->SetStringField(TEXT("hit_actor"),Blocked?GetNameSafe(Hit.GetActor()):TEXT("none"));J->SetStringField(TEXT("hit_component"),Blocked?GetNameSafe(Hit.GetComponent()):TEXT("none"));J->SetStringField(TEXT("hit"),Blocked?Hit.ImpactPoint.ToString():TEXT("none"));
                            BlockedSamples.Add(MakeShared<FJsonValueObject>(J));
                        }break;}
                    }
                    Ready[M]+=U->WeaponPhase==EFrontlineWeaponPhase::Ready&&!U->bMoving&&U->AimSettleClock<=0;
                }
            }
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("kind"),TEXT("held_fire"));Row->SetStringField(TEXT("mode"),Mode?TEXT("current"):TEXT("v834_legacy"));Row->SetStringField(TEXT("army"),Nation?TEXT("de"):TEXT("uk"));
            Row->SetStringField(TEXT("role"),RoleIndex?TEXT("machinegun"):TEXT("rifle"));Row->SetNumberField(TEXT("trench"),Trench);Row->SetNumberField(TEXT("seconds"),14);
            Row->SetNumberField(TEXT("initial_short_of_firing_step_cm"),ShortStart);
            TArray<TSharedPtr<FJsonValue>> Members;for(int32 M=0;M<Men.Num();++M)
            {
                auto* U=Men[M];auto* G=FrontlineMachinegunsV30::Get(U);auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("member"),M);J->SetNumberField(TEXT("shots"),U->ShotsDischarged);
                J->SetNumberField(TEXT("acquired_frames"),Acquired[M]);J->SetNumberField(TEXT("sight_frames"),TraceClear[M]);J->SetNumberField(TEXT("muzzle_clear_frames"),MuzzleClear[M]);J->SetNumberField(TEXT("ready_frames"),Ready[M]);
                J->SetNumberField(TEXT("standing_eye_visible_frames"),EyeVisible[M]);
                J->SetNumberField(TEXT("actual_travel_cm"),Travel[M]);J->SetNumberField(TEXT("unfulfilled_movement_frames"),Unfulfilled[M]);J->SetNumberField(TEXT("stance_changes"),StateChanges[M]);
                J->SetNumberField(TEXT("deployments"),G?G->Deployments:0);J->SetNumberField(TEXT("packs"),G?G->Packs:0);J->SetStringField(TEXT("feet"),U->Feet().ToString());Members.Add(MakeShared<FJsonValueObject>(J));
                Assert(FString::Printf(TEXT("v835_%d_%d_%d_%d_member%d_finite"),Mode,Nation,Depth,RoleIndex,M),!U->Feet().ContainsNaN());
                Assert(FString::Printf(TEXT("v835_%d_%d_%d_%d_member%d_held_clear_contact_fires"),Mode,Nation,Depth,RoleIndex,M),Acquired[M]>0&&U->ShotsDischarged>0);
            }
            Row->SetArrayField(TEXT("members"),Members);Row->SetArrayField(TEXT("standing_eye_without_acquisition_samples"),BlockedSamples);Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        for(int32 Context=0;Context<3;++Context)for(int32 RoleIndex=0;RoleIndex<2;++RoleIndex)
        for(int32 SeedIndex=0;SeedIndex<(Context==1&&RoleIndex==1?3:1);++SeedIndex)
        {
            const int32 DuelSeed=83537+SeedIndex*23;
            BeginCase(DuelSeed);const int32 Id=835700+Nation*100+Context*20+RoleIndex+SeedIndex*10000,EnemyId=Id+1000;
            const EFrontlineRole Role=RoleIndex?EFrontlineRole::Mills:EFrontlineRole::Rifle;const bool Held=Context!=0;
            if(!B->Trenches.IsValidIndex(Centre)){Assert(TEXT("v835_duel_has_actual_central_trench"),false);continue;}
            B->Trenches[Centre].Owner=1;
            FFrontlineSquad A;A.Id=Id;A.Team=0;A.Role=Role;A.bAdvancing=true;A.Destination=Centre;A.Slot=1;B->Squads.Add(A);
            FFrontlineSquad E;E.Id=EnemyId;E.Team=1;E.Role=EFrontlineRole::Rifle;E.Trench=E.Destination=Held?Centre:INDEX_NONE;E.Slot=1;E.bHoldPosition=Held;E.bAdvancing=!Held;B->Squads.Add(E);
            const FVector ObjectivePoint(B->Trenches[Centre].X,0,0);TArray<AWarfareFrontlineSoldier*> Attackers,Defenders;
            for(int32 M=0;M<B->RoleSize(Role,0);++M)if(auto* U=Make(0,Id,Role,M,ObjectivePoint+FVector(-Dir*(2600.f+(M%2)*80.f),(M-(B->RoleSize(Role,0)-1)*.5f)*130.f,0),DuelSeed))
            {B->FindSquad(Id)->Members.Add(U);Attackers.Add(U);}
            for(int32 M=0;M<B->RoleSize(EFrontlineRole::Rifle,1);++M)if(auto* U=Make(1,EnemyId,EFrontlineRole::Rifle,M,ObjectivePoint+FVector(0,(M-2.5f)*130.f,0),DuelSeed+400))
            {B->FindSquad(EnemyId)->Members.Add(U);Defenders.Add(U);}
            for(auto* U:Defenders){const FVector P=Held?B->SoldierDestination(U):U->Feet();U->SetActorLocation(Ground(P)+FVector(0,0,88));if(!Held){U->bAtDestinationV43=true;U->ShotClock=0;}Pose(U,.1f);}
            if(Mode==1&&Context==1&&RoleIndex==1&&SeedIndex==0&&!Attackers.IsEmpty()&&!Defenders.IsEmpty())
            {
                // Pure probability probes use this real, assigned trench man.
                // They neither resolve a strike nor consume the combat RNG.
                auto* ProbeVictim=Defenders[0];FFrontlineStrike ProbeBomb;
                ProbeBomb.SourceV65=FFrontlineKillSourceV65::Soldier(Attackers[0]);ProbeBomb.Team=0;
                ProbeBomb.bGrenade=true;ProbeBomb.bOrdinaryHandGrenadeV835=true;ProbeBomb.Radius=FrontlineGrenade::BlastRadiusCm;
                ProbeBomb.Target=Ground(ProbeVictim->Feet()+FVector(Dir*175.f,0,0))+FVector(0,0,3);
                const float ProbeDistance=FVector::Dist2D(ProbeBomb.Target,ProbeVictim->Feet());
                const float ProbeExposure=FrontlineGrenade::BlastExposure(B,ProbeBomb.Target,ProbeVictim);
                const float CurrentChance=B->ExplosionHitChance(ProbeBomb,ProbeVictim);
                LegacyContactComparisonV835=true;const float LegacyChance=B->ExplosionHitChance(ProbeBomb,ProbeVictim);LegacyContactComparisonV835=false;
                FFrontlineStrike StormBomb=ProbeBomb;StormBomb.bOrdinaryHandGrenadeV835=false;
                FFrontlineStrike FriendlyBomb=ProbeBomb;FriendlyBomb.Team=ProbeVictim->Team;FriendlyBomb.SourceV65.Team=ProbeVictim->Team;
                const float FriendlyChance=B->ExplosionHitChance(FriendlyBomb,ProbeVictim);
                const float EnemyScale=B->GrenadeHitChanceScale(ProbeBomb.Team),FriendlyScale=B->GrenadeHitChanceScale(FriendlyBomb.Team);
                const float ProbeLinear=1.f-FMath::Clamp((ProbeDistance-FrontlineGrenade::FullExposureRadiusCm)
                    /(ProbeBomb.Radius-FrontlineGrenade::FullExposureRadiusCm),0.f,1.f);
                const float ExpectedWithRealScreen=.82f*EnemyScale*ProbeLinear*ProbeExposure*(1.f-B->CoverFor(ProbeVictim)*.30f);
                FFrontlineStrike OutsideBomb=ProbeBomb;OutsideBomb.Target=ProbeVictim->Feet()+FVector(Dir*(ProbeBomb.Radius+1.f),0,0);
                Assert(FString::Printf(TEXT("v835_%s_ordinary_bomb_keeps_275cm_radius"),Nation?TEXT("de"):TEXT("uk")),
                    FMath::IsNearlyEqual(ProbeBomb.Radius,275.f)&&B->ExplosionHitChance(OutsideBomb,ProbeVictim)==0.f);
                Assert(FString::Printf(TEXT("v835_%s_covered_bomb_linear_improves_same_real_landing"),Nation?TEXT("de"):TEXT("uk")),
                    B->CoverFor(ProbeVictim)>0.f&&CurrentChance>LegacyChance&&LegacyChance>0.f);
                Assert(FString::Printf(TEXT("v835_%s_covered_bomb_keeps_real_screen_and_friend_symmetry"),Nation?TEXT("de"):TEXT("uk")),
                    ProbeVictim->Suppression<.55f&&ProbeExposure>0.f&&ProbeExposure<=1.f
                    &&FMath::IsNearlyEqual(CurrentChance,ExpectedWithRealScreen,.00001f)
                    &&FMath::IsNearlyEqual(CurrentChance/EnemyScale,FriendlyChance/FriendlyScale,.00001f));
                Assert(FString::Printf(TEXT("v835_%s_captured_storm_bomb_retains_old_squared_profile"),Nation?TEXT("de"):TEXT("uk")),
                    FMath::IsNearlyEqual(B->ExplosionHitChance(StormBomb,ProbeVictim),LegacyChance,.00001f));
                auto Probe=MakeShared<FJsonObject>();Probe->SetNumberField(TEXT("distance_cm"),ProbeDistance);Probe->SetNumberField(TEXT("actual_exposure"),ProbeExposure);
                Probe->SetNumberField(TEXT("current_chance"),CurrentChance);Probe->SetNumberField(TEXT("legacy_chance"),LegacyChance);Probe->SetNumberField(TEXT("friendly_chance"),FriendlyChance);
                R->SetObjectField(Nation?TEXT("grenade_profile_probe_de"):TEXT("grenade_profile_probe_uk"),Probe);
            }
            if(Context==2){FFrontlineSmokeZone Z;Z.Team=0;Z.Centre=ObjectivePoint+FVector(-Dir*1300.f,0,0);Z.Radius=1750.f;Z.StartedAt=999;Z.ExpiresAt=1040;B->SmokeZones.Add(Z);}
            TArray<float> FirstPhysicalRange,FirstPrepared,DeathAt,MinimumNearClock,InitialClock;
            TArray<int32> PhysicalRangeFrames,MovingRangeFrames,PreparedBlockedFrames;
            FirstPhysicalRange.Init(-1.f,Attackers.Num());FirstPrepared.Init(-1.f,Attackers.Num());DeathAt.Init(-1.f,Attackers.Num());
            MinimumNearClock.Init(MAX_flt,Attackers.Num());PhysicalRangeFrames.Init(0,Attackers.Num());MovingRangeFrames.Init(0,Attackers.Num());PreparedBlockedFrames.Init(0,Attackers.Num());
            for(const auto* U:Attackers)InitialClock.Add(U->GrenadeClock);
            TArray<TSharedPtr<FJsonValue>> GrenadeImpactEvidence;
            int32 Landings=0,BuiltBankLandings=0,EmbeddedLandings=0;float MaximumBurial=0,MaximumBankLift=0;
            float FirstThrow=-1,Nearest=MAX_flt;for(int32 F=0;F<600;++F)
            {
                B->Elapsed+=.05f;for(auto* U:Attackers)if(IsValid(U)){U->Tick(.05f);Pose(U,.05f);}for(auto* U:Defenders)if(IsValid(U)){U->Tick(.05f);Pose(U,.05f);}
                for(int32 M=0;M<Attackers.Num();++M)
                {
                    const auto* U=Attackers[M];
                    if(!U->IsAlive()){if(DeathAt[M]<0.f)DeathAt[M]=B->Elapsed-1000.f;continue;}
                    float NearestPhysical=MAX_flt;
                    for(const auto* V:Defenders)if(V->IsAlive())NearestPhysical=FMath::Min(NearestPhysical,float(FVector::Dist2D(U->Feet(),V->Feet())));
                    if(NearestPhysical<=FrontlineGrenade::MaxRangeCm)
                    {
                        if(FirstPhysicalRange[M]<0.f)FirstPhysicalRange[M]=B->Elapsed-1000.f;
                        ++PhysicalRangeFrames[M];MovingRangeFrames[M]+=U->bMoving;MinimumNearClock[M]=FMath::Min(MinimumNearClock[M],U->GrenadeClock);
                        if(FirstPrepared[M]<0.f&&(U->GrenadeClock<=0.f||U->GrenadeThrowsStarted>0))FirstPrepared[M]=B->Elapsed-1000.f;
                        if(U->GrenadeClock<=0.f&&(U->FirePoseClock>0.f||U->ShotClock>0.f||U->AimSettleClock>0.f||U->WeaponPhase!=EFrontlineWeaponPhase::Ready))++PreparedBlockedFrames[M];
                    }
                }
                // A newly committed hand bomb is observed once, in its real
                // release tick, before the ordinary support resolver runs.
                for(const auto& S:B->Strikes)if(S.bGrenade&&FMath::IsNearlyEqual(S.LaunchedAt,B->Elapsed,.001f))
                {
                    const float Surface=FrontlineInfantryV45::SurfaceHeight(B,S.Target);
                    const float Base=AWarfareFrontlineGround::HeightAt(S.Target.X,S.Target.Y);
                    const float Burial=FMath::Max(0.f,Surface+3.f-float(S.Target.Z));
                    ++Landings;BuiltBankLandings+=Surface>Base+1.f;EmbeddedLandings+=Burial>.1f;
                    MaximumBurial=FMath::Max(MaximumBurial,Burial);MaximumBankLift=FMath::Max(MaximumBankLift,Surface-Base);
                }
                // Inspect the real committed landing immediately before its
                // ordinary resolver. Probability/exposure queries are pure;
                // the fixture never supplies a hit or consumes a random draw.
                for(const auto& S:B->Strikes)if(S.bGrenade&&!S.bResolved&&S.ImpactAt<=B->Elapsed)
                {
                    const AWarfareFrontlineSoldier* NearestVictim=nullptr;float ClosestVictimDistance=MAX_flt,ExpectedEnemyHits=0.f;int32 InRadius=0;
                    for(const auto* V:Defenders)if(V->IsActiveCombatant())
                    {
                        const float VictimDistance=FVector::Dist2D(V->Feet(),S.Target);
                        if(VictimDistance<ClosestVictimDistance){ClosestVictimDistance=VictimDistance;NearestVictim=V;}
                        if(VictimDistance<=S.Radius){++InRadius;ExpectedEnemyHits+=B->ExplosionHitChance(S,V);}
                    }
                    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("impact_seconds"),B->Elapsed-1000.f);J->SetStringField(TEXT("target"),S.Target.ToString());
                    J->SetNumberField(TEXT("nearest_enemy_distance_cm"),NearestVictim?ClosestVictimDistance:-1.f);J->SetNumberField(TEXT("enemies_inside_radius"),InRadius);
                    J->SetNumberField(TEXT("nearest_enemy_exposure"),NearestVictim&&ClosestVictimDistance<=S.Radius?FrontlineGrenade::BlastExposure(B,S.Target,NearestVictim):-1.f);
                    J->SetNumberField(TEXT("nearest_enemy_hit_chance"),NearestVictim?B->ExplosionHitChance(S,NearestVictim):0.f);J->SetNumberField(TEXT("expected_enemy_hits"),ExpectedEnemyHits);
                    GrenadeImpactEvidence.Add(MakeShared<FJsonValueObject>(J));
                }
                B->TickCommittedSupportV8217();for(auto* U:Attackers)if(U->IsAlive())for(auto* V:Defenders)if(V->IsAlive())Nearest=FMath::Min(Nearest,float(FVector::Dist2D(U->Feet(),V->Feet())));
                if(FirstThrow<0&&B->GrenadesThrown>0)FirstThrow=B->Elapsed-1000;
            }
            int32 Alive=0,EnemyAlive=0,RoundCount=0,Throws=0,BombKills=0,RifleKills=0;
            for(auto* U:Attackers){Alive+=U->IsAlive();RoundCount+=U->ShotsDischarged;Throws+=U->GrenadeThrowsStarted;}
            for(auto* U:Defenders){EnemyAlive+=U->IsAlive();if(!U->IsAlive()&&U->FatalSourceV65.Team==0&&U->FatalSourceV65.SquadId==Id){BombKills+=U->GetDeathState().Kind==EFrontlineDamageKind::Grenade;RifleKills+=U->GetDeathState().Kind==EFrontlineDamageKind::Bullet;}}
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("kind"),TEXT("rifle_duel"));Row->SetStringField(TEXT("mode"),Mode?TEXT("current"):TEXT("v834_legacy"));Row->SetStringField(TEXT("army"),Nation?TEXT("de"):TEXT("uk"));
            Row->SetStringField(TEXT("role"),RoleIndex?TEXT("grenadiers"):TEXT("rifle"));Row->SetStringField(TEXT("context"),Context==0?TEXT("open"):Context==1?TEXT("held_trench"):TEXT("smoke_approach"));
            Row->SetNumberField(TEXT("seed"),DuelSeed);Row->SetNumberField(TEXT("grenade_enemy_kills"),BombKills);Row->SetNumberField(TEXT("rifle_enemy_kills"),RifleKills);
            Row->SetNumberField(TEXT("observed_grenade_landings"),Landings);Row->SetNumberField(TEXT("built_bank_landings"),BuiltBankLandings);Row->SetNumberField(TEXT("embedded_grenade_landings"),EmbeddedLandings);
            Row->SetNumberField(TEXT("maximum_burial_cm"),MaximumBurial);Row->SetNumberField(TEXT("maximum_built_bank_lift_cm"),MaximumBankLift);
            Row->SetNumberField(TEXT("seconds"),30);Row->SetNumberField(TEXT("attacker_alive"),Alive);Row->SetNumberField(TEXT("defender_alive"),EnemyAlive);Row->SetNumberField(TEXT("rounds"),RoundCount);Row->SetNumberField(TEXT("throws_started"),Throws);Row->SetNumberField(TEXT("grenades_released"),B->GrenadesThrown);
            Row->SetNumberField(TEXT("first_grenade_seconds"),FirstThrow);Row->SetNumberField(TEXT("nearest_live_contact_cm"),Nearest<MAX_flt?Nearest:-1.f);Row->SetNumberField(TEXT("readiness_seconds"),B->EffectiveRecruitSeconds(Role,0));Row->SetNumberField(TEXT("enemy_readiness_seconds"),B->EffectiveRecruitSeconds(EFrontlineRole::Rifle,1));
            Row->SetNumberField(TEXT("enemy_kills"),B->Casualties[1]);Row->SetNumberField(TEXT("friendly_kills"),B->Casualties[0]);
            TArray<TSharedPtr<FJsonValue>> ApproachMembers;
            if(RoleIndex)for(int32 M=0;M<Attackers.Num();++M)
            {
                const auto* U=Attackers[M];auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("member"),M);
                J->SetNumberField(TEXT("initial_grenade_clock"),InitialClock[M]);J->SetNumberField(TEXT("first_physical_range_seconds"),FirstPhysicalRange[M]);
                J->SetNumberField(TEXT("first_prepared_in_range_seconds"),FirstPrepared[M]);J->SetNumberField(TEXT("death_seconds"),DeathAt[M]);
                J->SetNumberField(TEXT("minimum_live_clock_in_physical_range"),MinimumNearClock[M]<MAX_flt?MinimumNearClock[M]:-1.f);
                J->SetNumberField(TEXT("physical_range_frames"),PhysicalRangeFrames[M]);J->SetNumberField(TEXT("moving_range_frames"),MovingRangeFrames[M]);J->SetNumberField(TEXT("prepared_weapon_clock_blocked_frames"),PreparedBlockedFrames[M]);
                J->SetNumberField(TEXT("throws_started"),U->GrenadeThrowsStarted);J->SetNumberField(TEXT("grenades_released"),U->GrenadesReleased);ApproachMembers.Add(MakeShared<FJsonValueObject>(J));
            }
            Row->SetArrayField(TEXT("approach_members"),ApproachMembers);Row->SetArrayField(TEXT("grenade_impact_evidence"),GrenadeImpactEvidence);Rows.Add(MakeShared<FJsonValueObject>(Row));
            Assert(FString::Printf(TEXT("v835_%d_%d_%d_%d_seed%d_duel_uses_real_mortal_rosters"),Mode,Nation,Context,RoleIndex,DuelSeed),Attackers.Num()==B->RoleSize(Role,0)&&Defenders.Num()==6&&B->Casualties[1]>=0&&B->Casualties[1]<=6&&B->Casualties[0]>=0&&B->Casualties[0]<=Attackers.Num());
            Assert(FString::Printf(TEXT("v835_%d_%d_%d_%d_seed%d_each_committed_bomb_is_inspected_at_most_once"),Mode,Nation,Context,RoleIndex,DuelSeed),GrenadeImpactEvidence.Num()<=Landings);
            if(Mode)
            {
                CurrentGrenadeLandings+=Landings;CurrentBuiltBankLandings+=BuiltBankLandings;
                Assert(FString::Printf(TEXT("v835_%s_%d_%d_seed%d_committed_bombs_rest_on_actual_surface"),Nation?TEXT("de"):TEXT("uk"),Context,RoleIndex,DuelSeed),EmbeddedLandings==0&&MaximumBurial<=.1f);
            }
            else LegacyEmbeddedLandings+=EmbeddedLandings;
        }
    }
    R->SetNumberField(TEXT("current_observed_grenade_landings"),CurrentGrenadeLandings);R->SetNumberField(TEXT("current_built_bank_landings"),CurrentBuiltBankLandings);R->SetNumberField(TEXT("legacy_embedded_grenade_landings"),LegacyEmbeddedLandings);
    Assert(TEXT("v835_actual_committed_bomb_sample_includes_real_built_bank_landings"),CurrentGrenadeLandings>0&&CurrentBuiltBankLandings>0);
    return R;
}
