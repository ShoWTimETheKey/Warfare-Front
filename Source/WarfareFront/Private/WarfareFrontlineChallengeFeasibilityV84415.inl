// Controlled force placement is the sole scene fixture. Cadence, ammunition,
// acquisition, traces, penetration, damage, casualties and retaliation all use
// unchanged production functions. This cannot establish a natural mission win.
struct FFrontlineChallengeFeasibilityV84415
{
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto Report=MakeShared<FJsonObject>();bool Good=true;
        const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
        auto* Fleet=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
        const bool Safe=Fleet&&FrontlineAchievementsV76::ChallengeReviewAllowedV84415(B);
        Assert(TEXT("v84415_challenge_exchange_disposable_scope"),Safe);
        if(!Safe){Report->SetBoolField(TEXT("success"),false);return Report;}
        const auto Units=B->Units;const auto Squads=B->Squads;const auto Vehicles=Fleet->Vehicles;
        const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;
        const auto Phase=B->Phase;const auto Outro=B->BattleOutroWinner;
        const float Time=B->Elapsed;const int32 NextSquad=B->NextSquadId,Shots=B->ShotsFired,Recruits=B->RecruitCommands;
        const int32 Grenades=B->GrenadesThrown;const auto Records0=B->CombatRecordV65[0],Records1=B->CombatRecordV65[1];
        const int32 Casualties0=B->Casualties[0],Casualties1=B->Casualties[1];
        const float Morale0=B->Morale[0],Morale1=B->Morale[1],Damage0=B->DamageDealt[0],Damage1=B->DamageDealt[1];
        const float Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;const auto Message=B->LastMessage,Failure=B->AssetFailure;
        float Ready[2][int32(EFrontlineRole::Count)];
        float SupportReady[2][3];int32 SupportCommands[3];
        for(int32 T=0;T<2;++T)for(int32 S=0;S<3;++S)SupportReady[T][S]=B->ExtendedSupportReadyAt[T][S];
        for(int32 S=0;S<3;++S)SupportCommands[S]=B->ExtendedSupportCommands[S];
        for(int32 T=0;T<2;++T)for(int32 R=0;R<int32(EFrontlineRole::Count);++R)Ready[T][R]=B->RecruitReadyAt[T][R];
        TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
        struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(Cast<AWarfareFrontlineSoldier>(*It)||Cast<AFrontlineTank>(*It))
        {Hidden.Add({*It,It->IsHidden(),It->GetActorEnableCollision()});It->SetActorHiddenInGame(true);It->SetActorEnableCollision(false);}
        FFrontlineAchievementReviewV76 Campaign(B);
        ON_SCOPE_EXIT
        {
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            B->Units=Units;B->Squads=Squads;Fleet->Vehicles=Vehicles;
            B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;B->Phase=Phase;B->BattleOutroWinner=Outro;
            B->Elapsed=Time;B->NextSquadId=NextSquad;B->ShotsFired=Shots;B->RecruitCommands=Recruits;B->GrenadesThrown=Grenades;
            B->CombatRecordV65[0]=Records0;B->CombatRecordV65[1]=Records1;
            B->Casualties[0]=Casualties0;B->Casualties[1]=Casualties1;B->Morale[0]=Morale0;B->Morale[1]=Morale1;
            B->DamageDealt[0]=Damage0;B->DamageDealt[1]=Damage1;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
            B->LastMessage=Message;B->AssetFailure=Failure;
            for(int32 T=0;T<2;++T)for(int32 R=0;R<int32(EFrontlineRole::Count);++R)B->RecruitReadyAt[T][R]=Ready[T][R];
            for(int32 T=0;T<2;++T)for(int32 S=0;S<3;++S)B->ExtendedSupportReadyAt[T][S]=SupportReady[T][S];
            for(int32 S=0;S<3;++S)B->ExtendedSupportCommands[S]=SupportCommands[S];
            for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        };
        const double CPUStart=FPlatformTime::Seconds();TArray<TSharedPtr<FJsonValue>> Cases;
        int32 GunSuccesses=0,DavidSuccesses=0;
        const auto Ground=[&](FVector P){P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);return P;};
        const auto Clear=[&]
        {
            for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
            B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();
            B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;B->AssetFailure.Reset();
            B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};B->Casualties[0]=B->Casualties[1]=0;
            B->ShotsFired=B->RecruitCommands=0;B->FriendlyFireDamage=B->PeakSuppression=0;B->DamageDealt[0]=B->DamageDealt[1]=0;
            for(int32 S=0;S<3;++S)B->ExtendedSupportCommands[S]=0;
            for(int32 T=0;T<2;++T){B->Morale[T]=100;for(int32 R=0;R<int32(EFrontlineRole::Count);++R)B->RecruitReadyAt[T][R]=0;}
            FrontlineSpecialistsV41::Reset(B);FrontlineAchievementsV76::ResetEncounter(B);
        };
        // Late-operation history allows these actual rosters and research. No
        // battle kill, survival, damage or award evidence is part of that history.
        for(int32 Case=0;Case<4;++Case)
        {
            Clear();const bool David=Case>=2;const int32 Variant=Case%2;
            Campaign.Campaign(David?EFrontlineFaction::German:EFrontlineFaction::British);
            Campaign.Run->CompletedMissions=19;Campaign.Run->SelectedMission=19;
            Campaign.Run->Research.TechnologyMask=(1u<<0)|(1u<<1)|(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)
                |(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9)|(1u<<10)|(1u<<12)|(1u<<16)|(1u<<17)|(1u<<24)|(1u<<25);
            Campaign.Run->RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Lewis))
                |(1u<<int32(EFrontlineRole::Officer))|(1u<<int32(EFrontlineRole::AntiTankRifle));
            Campaign.Run->SupportSelectionMask=1u<<int32(EFrontlineSupport::AntiTank);
            B->NextSquadId=8441540+Case*100;
            auto& Encounter=FrontlineAchievementsV76::State(B);Encounter.Started=true;Encounter.Campaign=Campaign.Run->Id;
            const bool Assisted=David&&Variant;
            const int32 Dir=B->GetTeamDirection(0);const float Y=Assisted?350.f:Variant?1100.f:-1100.f;
            const auto PlaceSection=[&](FFrontlineSquad& Q,const FVector& Base)
            {
                Q.Trench=Q.Destination=INDEX_NONE;Q.bAdvancing=Q.bReinforcement=false;Q.bHoldPosition=true;
                for(int32 M=0;M<Q.Members.Num();++M)if(auto* U=Q.Members[M].Get())
                {
                    U->bFieldEntered=true;U->bMoving=false;U->SetActorTickEnabled(false);
                    U->SetActorLocation(Ground(Base+FVector(0,(M-(Q.Members.Num()-1)*.5f)*110,0))+FVector(0,0,88));
                    U->SetActorRotation(FRotator(0,B->GetTeamDirection(Q.Team)>0?0.f:180.f,0));
                    U->ReturnToIdle();U->AimSettleClock=U->DepartureClock=0;
                }
            };
            const auto Section=[&](EFrontlineRole Role,int32 Team,const FVector& Base)->int32
            {
                if(!B->SpawnSquad(Team,Role))return INDEX_NONE;
                auto& Q=B->Squads.Last();PlaceSection(Q,Base);
                FrontlineAchievementsV76::Recruited(B,int32(Role),Team);return Q.Id;
            };
            const int32 Goal=Section(David?EFrontlineRole::AntiTankRifle:EFrontlineRole::Lewis,0,FVector(-Dir*2000.f,Y,0));
            // Ordinary rifles shield the specialist's approach. Their real fire
            // may take kills; their bodies and ammunition receive no protection.
            Section(EFrontlineRole::Rifle,0,FVector(-Dir*1400.f,Y+(Assisted?760.f:260.f),0));
            Section(EFrontlineRole::Officer,0,FVector(-Dir*2300.f,Y-(Assisted?800.f:260.f),0));
            if(David)Section(EFrontlineRole::Rifle,0,FVector(-Dir*900.f,Y-(Assisted?760.f:260.f),0));
            AFrontlineTank* Tank=nullptr;
            if(David)
            {
                // A screened rifle position must still be inside its actual
                // weather-limited acquisition range; no range value is changed.
                Tank=B->GetWorld()->SpawnActor<AFrontlineTank>(Ground(FVector(Dir*(Assisted?2200.f:3700.f),Y,0)),FRotator::ZeroRotator);
                const auto Model=Variant?EFrontlineVehicle::MarkVMale:EFrontlineVehicle::MarkIVFemale;
                if(Tank&&Tank->Configure(B,Model,1,84415400+Case))
                {Tank->FitGroundV35(Tank->GetActorLocation(),0);Tank->SetActorTickEnabled(false);Tank->bEntered=true;Fleet->Vehicles.Add(Tank);}
                else{if(Tank)Tank->Destroy();Tank=nullptr;}
            }
            else Section(EFrontlineRole::Rifle,1,FVector(Dir*1200.f,Y,0));
            Assert(FString::Printf(TEXT("v84415_exchange_%d_legal_goal_role"),Case),Goal!=INDEX_NONE&&B->IsRoleUnlocked(David?EFrontlineRole::AntiTankRifle:EFrontlineRole::Lewis,0));
            auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("case"),Case);J->SetStringField(TEXT("challenge"),David?TEXT("iron_answer"):TEXT("gun_watch"));
            J->SetNumberField(TEXT("goal_section"),Goal);J->SetNumberField(TEXT("difficulty"),int32(B->GetCampaignDifficulty()));
            J->SetNumberField(TEXT("initial_enemy_hull"),Tank?Tank->GetHullIntegrity():0);
            const auto PositionJson=[](const FVector& P){auto V=MakeShared<FJsonObject>();V->SetNumberField(TEXT("x"),P.X);V->SetNumberField(TEXT("y"),P.Y);V->SetNumberField(TEXT("z"),P.Z);return V;};
            if(Tank)J->SetObjectField(TEXT("initial_tank_position"),PositionJson(Tank->GetActorLocation()));
            FVector RifleStation(-Dir*2000.f,Y,0);
            if(Assisted&&Tank)
            {
                auto* Original=B->FindSquad(Goal);
                auto* Lead=Original&&!Original->Members.IsEmpty()?Original->Members[0].Get():nullptr;
                const auto EvaluatePose=[&](AWarfareFrontlineSoldier* U)
                {
                    if(!U)return;
                    for(int32 Step=0;Step<160;++Step)
                    {
                        auto* Gun=FrontlineMachinegunsV30::Get(U);if(Gun)Gun->Update(.1f,false);
                        if(Gun&&Gun->IsSupported())U->SetMotion(U->GetAimMotion());
                        U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(.1f,false);U->Soldier->RefreshBoneTransforms();
                        if(Gun)Gun->TickComponent(.1f,LEVELTICK_All,nullptr);
                        if(Gun&&Gun->IsSupported()&&Step>=20)break;
                    }
                };
                const auto Probe=[&]()->TSharedRef<FJsonObject>
                {
                    auto P=MakeShared<FJsonObject>();if(!Lead)return P;
                    const FVector Muzzle=Lead->GetMuzzleLocation();const FVector Aim=Tank->AimPoint();
                    auto* Acquired=Fleet->AcquireInfantryTarget(Lead);
                    FCollisionQueryParams Query(SCENE_QUERY_STAT(V84415ActualDischargeGate),true);Query.AddIgnoredActor(Tank);
                    for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Query.AddIgnoredActor(U);
                    FHitResult Hit;const bool Blocked=B->GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Aim,ECC_Visibility,Query);
                    P->SetObjectField(TEXT("feet"),PositionJson(Lead->Feet()));P->SetObjectField(TEXT("muzzle"),PositionJson(Muzzle));
                    P->SetObjectField(TEXT("observed_armour_point"),PositionJson(Tank->ObservedArmourAimPointV837(Lead->Feet())));P->SetObjectField(TEXT("discharge_aim_point"),PositionJson(Aim));
                    P->SetNumberField(TEXT("visibility_scale"),B->EncounterVisibilityScale());P->SetNumberField(TEXT("effective_fire_range"),FrontlineWeapons::ProfileFor(Lead).FireRangeCm*B->EncounterVisibilityScale());
                    P->SetBoolField(TEXT("acquisition"),Acquired==Tank);P->SetBoolField(TEXT("production_firing_line"),Fleet->HasInfantryFiringLineV828(Lead,Tank));
                    P->SetBoolField(TEXT("discharge_muzzle_line"),!Blocked);P->SetBoolField(TEXT("supported_weapon_ready"),FrontlineMachinegunsV30::CanFire(Lead));
                    if(Blocked){P->SetStringField(TEXT("blocker_actor"),GetNameSafe(Hit.GetActor()));P->SetStringField(TEXT("blocker_component"),GetNameSafe(Hit.GetComponent()));P->SetObjectField(TEXT("blocker_point"),PositionJson(Hit.ImpactPoint));}
                    return P;
                };
                EvaluatePose(Lead);J->SetObjectField(TEXT("original_station_diagnostic"),Probe());
                const float XOffsets[]={0,-200,200,-400,400,-700,700};const float YOffsets[]={0,-250,250,-500,500,-750,750};
                bool Found=false;int32 Candidates=0;
                for(float X:XOffsets)
                {
                    for(float OffsetY:YOffsets)
                    {
                        const FVector Candidate=RifleStation+FVector(X,OffsetY,0);
                        if(FMath::Abs(Candidate.Y)+55.f>FrontlineBattlefield::HalfFrontage||FMath::Abs(Candidate.X)>FrontlineBattlefield::SupportHalfLength)continue;
                        if(!Original||!Lead)break;
                        PlaceSection(*Original,Candidate);EvaluatePose(Lead);auto P=Probe();++Candidates;
                        if(P->GetBoolField(TEXT("acquisition"))&&P->GetBoolField(TEXT("production_firing_line"))&&P->GetBoolField(TEXT("discharge_muzzle_line"))&&P->GetBoolField(TEXT("supported_weapon_ready")))
                        {RifleStation=Candidate;Found=true;J->SetObjectField(TEXT("selected_station_diagnostic"),P);break;}
                    }
                    if(Found)break;
                }
                J->SetNumberField(TEXT("ground_station_candidates_tested"),Candidates);J->SetBoolField(TEXT("legal_fixed_ground_station_found"),Found);
                J->SetObjectField(TEXT("selected_fixed_rifle_station"),PositionJson(Ground(RifleStation)));
                Assert(TEXT("v84415_david_fixed_ground_station_passes_all_production_rays"),Found);
            }
            bool SupportRequested=false;int32 SupportRounds=0;FVector SupportTarget=FVector::ZeroVector;
            TArray<TSharedPtr<FJsonValue>> RifleReplacements;
            const float Began=B->Elapsed;float NextWave=Began+25;bool Budget=true;
            while(B->Elapsed-Began<180.f&&!FrontlineAchievementsV76::ChallengeConditionV84415(B,David?6:3))
            {
                if(FPlatformTime::Seconds()-CPUStart>35.){Budget=false;break;}
                constexpr float DT=.1f;B->Elapsed+=DT;
                if(Assisted&&!SupportRequested&&Tank&&Tank->IsOperational()&&B->Elapsed-Began>=8.f)
                {
                    // A fixed order along the outer running gear, not a tracked
                    // weak-hull setup. Six real rounds retain their full scatter.
                    SupportTarget=Ground(Tank->GetActorLocation()+FVector(0,350.f,0));
                    SupportRequested=B->RequestSupport(EFrontlineSupport::AntiTank,SupportTarget,0);
                }
                bool HasLiveRifleGunner=false;
                if(Assisted)for(const AWarfareFrontlineSoldier* U:B->Units)
                    if(IsValid(U)&&U->Team==0&&U->Role==EFrontlineRole::AntiTankRifle&&U->MemberIndex==0&&U->IsActiveCombatant()){HasLiveRifleGunner=true;break;}
                if(Assisted&&!HasLiveRifleGunner)
                {
                    // Check the ordinary shared cooldown every host tick. A
                    // lost gunner is replaced by a newly recruited section,
                    // never revived and never granted an earlier ready time.
                    if(B->Recruit(EFrontlineRole::AntiTankRifle,0))
                    {
                        auto& Fresh=B->Squads.Last();PlaceSection(Fresh,RifleStation);
                        auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("section"),Fresh.Id);
                        Entry->SetNumberField(TEXT("at_sim_seconds"),B->Elapsed-Began);
                        Entry->SetObjectField(TEXT("position"),PositionJson(Ground(RifleStation)));
                        RifleReplacements.Add(MakeShared<FJsonValueObject>(Entry));HasLiveRifleGunner=true;
                    }
                }
                if((!David||Assisted)&&B->Elapsed>=NextWave)
                {
                    NextWave=B->Elapsed+25.f;
                    const int32 Team=Assisted?0:1;
                    if((!Assisted||HasLiveRifleGunner)&&B->Recruit(EFrontlineRole::Rifle,Team))PlaceSection(B->Squads.Last(),Assisted
                        ?FVector(-Dir*900.f,Y+(B->Squads.Num()%2?760.f:-760.f),0)
                        :FVector(Dir*1200.f,Y+((B->Squads.Num()%3)-1)*160.f,0));
                }
                // Static defensive placement omits travel only. Each actor still
                // acquires its own legal target and consumes its real feed cycle.
                for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->IsActiveCombatant())
                {
                    U->ShotClock-=DT;U->AimSettleClock-=DT;
                    U->Suppression=FMath::Max(0.f,U->Suppression-DT*.065f);U->UpdateWeaponCycle(DT);
                    if(auto* Gun=FrontlineMachinegunsV30::Get(U))Gun->Update(DT,false);
                    if(U->ShotClock>0||U->AimSettleClock>0||U->WeaponPhase!=EFrontlineWeaponPhase::Ready)continue;
                    auto* Armour=Fleet->AcquireInfantryTarget(U);auto* Infantry=B->AcquireTarget(U);
                    if(!Armour&&!Infantry)continue;
                    U->SetMotion(U->GetAimMotion());
                    U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
                    U->DischargeWeapon(Infantry,Armour);
                }
                if(Tank&&Tank->IsOperational())Tank->Simulate(DT);
                for(TActorIterator<AFrontlineSupportMissionV28> It(B->GetWorld());It;++It)
                    if(!Existing.Contains(*It)&&!It->IsActorBeingDestroyed()){It->Tick(DT);SupportRounds=FMath::Max(SupportRounds,It->RoundsFired);}
                FrontlineSpecialistsV41::Tick(B);B->TickCommittedSupportV8217();
                for(int32 I=B->Tracers.Num()-1;I>=0;--I)if((B->Tracers[I].Life-=DT)<=0)B->Tracers.RemoveAtSwap(I);
                // No survivors means a failed tactic, not a reason to revive men.
                if(B->LivingSoldiers(0)==0)break;
            }
            const auto* Q=B->FindSquad(Goal);const bool Qualified=FrontlineAchievementsV76::ChallengeConditionV84415(B,David?6:3);
            int32 EnemyShots=0;for(const AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->Team==1)EnemyShots+=U->ShotsDischarged;
            const bool Retaliated=David?Tank&&(Tank->MachinegunShots+Tank->CannonShots)>0:EnemyShots>0;
            J->SetNumberField(TEXT("sim_seconds"),B->Elapsed-Began);J->SetBoolField(TEXT("budget_met"),Budget);
            J->SetBoolField(TEXT("predicate_met"),Qualified);J->SetBoolField(TEXT("enemy_retaliated"),Retaliated);
            J->SetNumberField(TEXT("enemy_infantry_shots"),EnemyShots);J->SetNumberField(TEXT("enemy_tank_shots"),Tank?Tank->MachinegunShots+Tank->CannonShots:0);
            J->SetNumberField(TEXT("enemy_remaining_hull"),Tank?Tank->GetHullIntegrity():0);
            J->SetNumberField(TEXT("goal_infantry_kills"),Q?Q->KillsV65.Infantry:0);J->SetNumberField(TEXT("goal_armour_kills"),Q?Q->KillsV65.Armour:0);
            J->SetNumberField(TEXT("goal_living_members"),Q?Q->LivingCount():0);J->SetNumberField(TEXT("player_infantry_losses"),B->CombatRecordV65[0].InfantryLost);
            J->SetNumberField(TEXT("heavy_rifle_receipts"),FrontlineAchievementsV76::State(B).HeavyRifleVictimsV84415.Num());
            J->SetArrayField(TEXT("rifle_replacements"),RifleReplacements);
            int32 RifleArmourKills=0;TArray<TSharedPtr<FJsonValue>> ActualRifleSections;
            if(David)for(const auto& Actual:B->Squads)if(Actual.Team==0&&Actual.Role==EFrontlineRole::AntiTankRifle)
            {RifleArmourKills+=Actual.KillsV65.Armour;auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("section"),Actual.Id);Entry->SetNumberField(TEXT("armour_kills"),Actual.KillsV65.Armour);Entry->SetNumberField(TEXT("living_members"),Actual.LivingCount());int32 Discharges=0;for(const auto& M:Actual.Members)if(const auto* U=M.Get();U&&U->MemberIndex==0)Discharges+=U->ShotsDischarged;Entry->SetNumberField(TEXT("actual_heavy_rifle_shots"),Discharges);ActualRifleSections.Add(MakeShared<FJsonValueObject>(Entry));}
            J->SetNumberField(TEXT("all_actual_rifle_armour_kills"),RifleArmourKills);J->SetArrayField(TEXT("actual_rifle_sections"),ActualRifleSections);
            const auto AP=FrontlineSpecialistsV41::ReadArmourRoundsV833(B);auto APJson=MakeShared<FJsonObject>();
            APJson->SetNumberField(TEXT("fired"),AP.Fired);APJson->SetNumberField(TEXT("physical_hull_hits"),AP.PhysicalHullHits);APJson->SetNumberField(TEXT("world_interceptions"),AP.WorldInterceptions);APJson->SetNumberField(TEXT("applied_hits"),AP.AppliedHits);APJson->SetNumberField(TEXT("hull_damage"),AP.HullDamage);APJson->SetNumberField(TEXT("track_damage"),AP.TrackDamage);
            J->SetObjectField(TEXT("production_ap_round_readback"),APJson);
            J->SetBoolField(TEXT("support_requested"),SupportRequested);J->SetStringField(TEXT("support_type"),Assisted?TEXT("AntiTank"):TEXT("none"));
            J->SetNumberField(TEXT("support_actual_rounds"),SupportRounds);J->SetNumberField(TEXT("support_armour_kills"),B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::AntiTank)].Armour);
            if(Assisted)J->SetObjectField(TEXT("fixed_support_order"),PositionJson(SupportTarget));
            if(Tank){J->SetObjectField(TEXT("final_tank_position"),PositionJson(Tank->GetActorLocation()));J->SetNumberField(TEXT("left_track"),Tank->GetTrackIntegrity(0));J->SetNumberField(TEXT("right_track"),Tank->GetTrackIntegrity(1));J->SetStringField(TEXT("mobility_gate"),Tank->MobilityGateV839);}
            // A female tank can legitimately die outside its mounted MG range.
            // It remains a range-control observation; the Mark V must return fire.
            if(Case!=2)Assert(FString::Printf(TEXT("v84415_exchange_%d_enemy_uses_actual_retaliation"),Case),Retaliated);
            if(Assisted)Assert(TEXT("v84415_david_combined_arms_support_is_actual_order"),SupportRequested&&SupportRounds>0);
            Assert(FString::Printf(TEXT("v84415_exchange_%d_bounded_180_sim_seconds"),Case),Budget&&B->Elapsed-Began<180.2f);
            // David's actual condition is a legitimate heavy-hull rifle kill.
            // An enemy mounted MG returning fire is not an extra player rule.
            if(Qualified){if(David)++DavidSuccesses;else if(Retaliated)++GunSuccesses;}Cases.Add(MakeShared<FJsonValueObject>(J));
        }
        Assert(TEXT("v84415_one_gun_has_retaliating_attainable_exchange"),GunSuccesses>=1);
        Assert(TEXT("v84415_david_has_attainable_legitimate_heavy_hull_kill"),DavidSuccesses>=1);
        Report->SetStringField(TEXT("scope"),TEXT("Four controlled late-1918 force placements, two per challenge, maximum 180 host seconds each. Actual target acquisition, weapon feeds, terrain traces, unmodified probability/penetration, tank movement and enemy retaliation. No revival, forced damage, fabricated kills, forced victory or natural-campaign attainment claim."));
        Report->SetArrayField(TEXT("cases"),Cases);Report->SetNumberField(TEXT("gun_successes"),GunSuccesses);Report->SetNumberField(TEXT("david_successes"),DavidSuccesses);
        Report->SetNumberField(TEXT("cpu_seconds"),FPlatformTime::Seconds()-CPUStart);Report->SetBoolField(TEXT("success"),Good);return Report;
    }
};
namespace FrontlineAchievementsV76
{
TSharedRef<FJsonObject> RunChallengeFeasibilityV84415(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    WithScopedCombatLedgerV84412(B,[&]{Result=FFrontlineChallengeFeasibilityV84415::Run(B,Check);});return Result;
}
}
