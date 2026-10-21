static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> ExternalCheck)
{
    auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows,Checks;bool Good=true;
    const auto Check=[&](const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;ExternalCheck(Name,Pass);};
    FString User;const bool CommandSafe=B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User)&&!WarfareFRRUCampaignV82::Active(B);
    FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8447/Runtime"));
    FPaths::NormalizeDirectoryName(Root);Root+=TEXT("/");
    const FString Stamp=User.StartsWith(Root,ESearchCase::IgnoreCase)?User.Mid(Root.Len()):FString();
    const bool Safe=CommandSafe&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    Check(TEXT("v8447_balance_disposable_ukde_only"),Safe);
    if(!Safe){R->SetBoolField(TEXT("success"),false);R->SetArrayField(TEXT("checks"),Checks);return R;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;
    const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;
    const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;
    const int32 BoundMission=B->BoundCampaignMission,NextId=B->NextSquadId,Winner=B->BattleOutroWinner,Shots=B->ShotsFired,Thrown=B->GrenadesThrown;
    const auto Random=B->Random;const auto Phase=B->Phase;const float Clock=B->Elapsed,Peak=B->PeakSuppression,Friendly=B->FriendlyFireDamage;
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
    const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]},SmokeShots[2]={B->SmokeAffectedShots[0],B->SmokeAffectedShots[1]};
    const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
    const bool Legacy=LegacyBalanceComparisonV8447;
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
        Clear();LegacyBalanceComparisonV8447=Legacy;
        B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;
        B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=BoundMission;
        B->NextSquadId=NextId;B->BattleOutroWinner=Winner;B->ShotsFired=Shots;B->GrenadesThrown=Thrown;B->Random=Random;B->Phase=Phase;B->Elapsed=Clock;B->PeakSuppression=Peak;B->FriendlyFireDamage=Friendly;
        B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;
        for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];B->SmokeAffectedShots[T]=SmokeShots[T];}
        for(const auto& H:Hidden)if(auto* U=H.U.Get()){U->SetActorHiddenInGame(H.Hidden);U->SetActorEnableCollision(H.Collision);}
        R->SetBoolField(TEXT("success"),Good);R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("cases"),Rows);
        R->SetStringField(TEXT("scope"),TEXT("Matched v84.4.6/current UK/DE real-actor Fire and weapon-cycle probes. Clear high plane isolates exact probability/cover/smoke/range. Grounded 30-second held-section Tick uses evaluated poses and real supported MG, ammunition and reload clocks. Fixture targets alone are replenished between rounds and never tick death physics; this is sustained-fire measurement, not a duel or campaign win-rate."));
    };
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();if(auto* C=FrontlineMachinegunsV30::Get(U))C->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const auto Ground=[&](const FVector& P){return FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P));};
    const auto Begin=[&](int32 Mode,EFrontlineFaction Army)
    {
        Clear();LegacyBalanceComparisonV8447=Mode==0;B->Trenches=Trenches;B->PlayerFaction=Army;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;
        FFrontlineCampaignProgress C;C.Id=FGuid::NewGuid();C.Faction=Army;C.SelectedMission=C.CompletedMissions=19;
        B->CampaignRuns={C};B->ActiveCampaignId=B->BoundCampaignId=C.Id;B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=19;
        B->Random.Initialize(844701);B->ShotsFired=B->GrenadesThrown=0;
        for(int32 T=0;T<2;++T){B->Casualties[T]=0;B->Morale[T]=100;B->DamageDealt[T]=0;B->CombatRecordV65[T]={};}
    };
    const auto Make=[&](int32 Team,int32 Id,EFrontlineRole Role,int32 Member,const FVector& Feet,bool OnGround)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
        U->MotionRandom.Initialize(844711+Member*101+Team*197);U->ShotClock=U->GrenadeClock=10000;U->DepartureClock=U->ReactionSeconds=0;
        U->SetActorLocation((OnGround?Ground(Feet):Feet)+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));U->SetMotion(U->GetAimMotion());
        B->Units.Add(U);Pose(U,.2f);return U;
    };
    // The old clamp turns deep .82 cover into .1253 exposure. Seven percentage
    // points less standing shelter restore .20 exposure while reload keeps .82.
    for(int32 Army=0;Army<2;++Army)for(int32 Depth=0;Depth<3;++Depth)for(int32 Reload=0;Reload<2;++Reload)
    {
        float Chance[2]={},Cover[2]={};int32 Hits[2]={};
        for(int32 Mode=0;Mode<2;++Mode)
        {
            Begin(Mode,EFrontlineFaction(Army));const int32 Dir=B->GetTeamDirection(1);
            const int32 Trench=Depth==0?B->GetHomeTrenchIndex(0):Depth==1?FrontlineBattlefield::CentreTrench:1;
            B->Trenches[Trench].Owner=0;const FVector P(FrontlineFieldV32::TrenchCentreX(Trench,0),0,10000);
            auto* Target=Make(0,844701,EFrontlineRole::Rifle,0,P,false);auto* Gun=Make(1,844702,EFrontlineRole::Lewis,0,P-FVector(Dir*2500,0,0),false);
            Check(TEXT("v8447_cover_probe_real_actors"),Target&&Gun);if(!Target||!Gun)continue;
            FFrontlineSquad S;S.Id=Target->SquadId;S.Team=0;S.Role=Target->Role;S.Trench=Trench;S.Members.Add(Target);B->Squads.Add(S);
            if(Reload){Target->WeaponPhase=EFrontlineWeaponPhase::Reload;Target->ReloadDurationSecondsV82=2;Target->FirePoseClock=1;}
            Cover[Mode]=B->CoverFor(Target);Chance[Mode]=B->BulletHitChance(Gun,Target);
            const bool Sight=B->HasSight(Gun,Target);Check(TEXT("v8447_cover_probe_actual_sight"),Sight);
            for(int32 Shot=0;Shot<256;++Shot)
            {
                Target->Health=100;Target->Suppression=0;Target->DeathState={};B->Morale[0]=100;
                B->Fire(Gun,Target);if(!Target->IsAlive())++Hits[Mode];
            }
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("real_fire_probability_probe"));J->SetStringField(TEXT("mode"),Mode?TEXT("v8447"):TEXT("v8446"));
            J->SetStringField(TEXT("defender_army"),Army?TEXT("de"):TEXT("uk"));J->SetNumberField(TEXT("trench"),Trench);J->SetBoolField(TEXT("target_reloading"),Reload!=0);
            J->SetNumberField(TEXT("cover"),Cover[Mode]);J->SetNumberField(TEXT("target_exposure"),FrontlineBalanceV50::TargetExposure(Gun,Target,Cover[Mode]));
            const auto& Profile=FrontlineWeapons::ProfileFor(Gun);J->SetStringField(TEXT("attacker_weapon"),Profile.WeaponId);
            J->SetNumberField(TEXT("physical_distance_cm"),FVector::Dist2D(Gun->Feet(),Target->Feet()));J->SetNumberField(TEXT("resident_fire_range_cm"),Profile.FireRangeCm);
            J->SetNumberField(TEXT("visibility_scale"),B->EncounterVisibilityScale());J->SetNumberField(TEXT("smoke_factor"),B->DirectFireMultiplier(Gun->Feet(),Target->Feet()));
            J->SetNumberField(TEXT("incoming_bullet_factor"),B->IncomingBulletHitChanceScale(Target));J->SetNumberField(TEXT("prepared_works_factor"),FrontlineFieldSitesV59::PreparedFrontCoverV60(Gun,Target));
            J->SetNumberField(TEXT("terrain_factor"),FrontlineTerrainTacticsV54::DirectFireHitScale(Gun->Feet(),Target->Feet(),Target->bMoving));
            J->SetNumberField(TEXT("attacker_firing_position_factor"),B->CoverFor(Gun)>0.f?1.3f:1.f);
            J->SetNumberField(TEXT("per_round_probability"),Chance[Mode]);J->SetNumberField(TEXT("attempts"),256);J->SetNumberField(TEXT("actual_fire_calls"),B->ShotsFired);J->SetNumberField(TEXT("actual_fatal_hits"),Hits[Mode]);Rows.Add(MakeShared<FJsonValueObject>(J));
        }
        Check(FString::Printf(TEXT("v8447_army%d_depth%d_reload%d_cover_probability"),Army,Depth,Reload),Chance[0]>0&&Chance[1]>0
            &&(Reload?FMath::IsNearlyEqual(Cover[0],Cover[1],.00001f)&&FMath::IsNearlyEqual(Chance[0],Chance[1],.00001f):FMath::IsNearlyEqual(Cover[0]-Cover[1],.07f,.00001f)&&Chance[1]>=Chance[0]*1.10f));
    }
    Begin(1,EFrontlineFaction::German);
    const int32 Trench=B->GetHomeTrenchIndex(0),Dir=B->GetTeamDirection(0);B->Trenches[Trench].Owner=0;
    const FVector P(FrontlineFieldV32::TrenchCentreX(Trench,0),0,10000);
    auto* Defender=Make(0,844720,EFrontlineRole::Lewis,0,P,false);auto* Rifle=Make(1,844721,EFrontlineRole::Rifle,0,P+FVector(Dir*2500,0,0),false);
    if(Defender&&Rifle)
    {
        FFrontlineSquad S;S.Id=Defender->SquadId;S.Team=0;S.Role=Defender->Role;S.Trench=Trench;S.Members.Add(Defender);B->Squads.Add(S);
        const float Base=B->BulletHitChance(Rifle,Defender);B->ActiveResearchMutable().TechnologyMask=1u<<7;
        Check(TEXT("v8447_german_gun_protection_remains_exactly_bounded"),FMath::IsNearlyEqual(B->BulletHitChance(Rifle,Defender)/Base,.75f,.0001f));
        Defender->WeaponPhase=EFrontlineWeaponPhase::Reload;Defender->ReloadDurationSecondsV82=2;
        Defender->FirePoseClock=2;const float Start=FrontlineBalanceV50::TrenchReloadShelterBlendV8447(Defender);
        Defender->FirePoseClock=1;const float Mid=FrontlineBalanceV50::TrenchReloadShelterBlendV8447(Defender);
        Defender->FirePoseClock=.02f;const float End=FrontlineBalanceV50::TrenchReloadShelterBlendV8447(Defender);
        Defender->bMoving=true;const float Moving=FrontlineBalanceV50::TrenchReloadShelterBlendV8447(Defender);Defender->bMoving=false;
        Check(TEXT("v8447_reload_cover_uses_same_smooth_pose_envelope"),Start==0&&Mid==1&&End>0&&End<.02f&&Moving==0);
    }
    // Exercise the real neighbour-pressure loop, including its final radius
    // max, on each UK gun profile. A formula-only check misses the old no-op.
    for(EFrontlineRole Role:{EFrontlineRole::Lewis,EFrontlineRole::MobileGun})
    {
        float Radius[2]={},Pressure[2]={};
        for(int32 Mode=0;Mode<2;++Mode)
        {
            Begin(Mode,EFrontlineFaction::British);B->ActiveResearchMutable().TechnologyMask=1u<<7;
            const int32 Direction=B->GetTeamDirection(0);const FVector Point(0,0,10000);
            auto* Gun=Make(0,844722,Role,0,Point,false);auto* Enemy=Make(1,844723,EFrontlineRole::Rifle,0,Point+FVector(Direction*1600,0,0),false);
            const float Gap=Role==EFrontlineRole::Lewis?600.f:500.f;
            auto* Neighbor=Make(1,844724,EFrontlineRole::Rifle,0,Point+FVector(Direction*1600,Gap,0),false);
            if(!Gun||!Enemy||!Neighbor){Check(TEXT("v8447_fire_distribution_actual_actors"),false);continue;}
            const auto& Profile=FrontlineWeapons::ProfileFor(Gun);Radius[Mode]=FMath::Max(Profile.AdjacentRadiusCm,B->AdjacentPressureRadius(Gun));
            B->Fire(Gun,Enemy);Pressure[Mode]=Neighbor->Suppression;
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("uk_fire_distribution_real_neighbor_pressure"));J->SetStringField(TEXT("mode"),Mode?TEXT("v8447"):TEXT("v8446"));
            J->SetStringField(TEXT("section"),Role==EFrontlineRole::Lewis?TEXT("fixed_lewis"):TEXT("mobile_lewis"));J->SetNumberField(TEXT("profile_radius_cm"),Profile.AdjacentRadiusCm);
            J->SetNumberField(TEXT("final_pressure_radius_cm"),Radius[Mode]);J->SetNumberField(TEXT("actual_neighbor_gap_cm"),Gap);J->SetNumberField(TEXT("actual_neighbor_suppression"),Pressure[Mode]);J->SetNumberField(TEXT("actual_fire_calls"),B->ShotsFired);
            Rows.Add(MakeShared<FJsonValueObject>(J));
        }
        Check(Role==EFrontlineRole::Lewis?TEXT("v8447_fixed_lewis_paid_radius_upgrade_now_has_real_effect"):TEXT("v8447_mobile_lewis_paid_radius_upgrade_is_profile_relative"),
            Radius[1]>Radius[0]&&Pressure[0]==0&&Pressure[1]>0);
    }
    // A close MP18 shot exposes a trench lip; this is an observed hit-chance
    // bonus, never a discharge permission through a wall or beyond its range.
    for(float Distance:{900.f,1400.f,2500.f})
    {
        float Chance[2]={},Exposure[2]={};
        for(int32 Mode=0;Mode<2;++Mode)
        {
            Begin(Mode,EFrontlineFaction::German);B->ActiveResearchMutable().TechnologyMask=1u<<12;
            const int32 Home=B->GetHomeTrenchIndex(1),Direction=B->GetTeamDirection(0);B->Trenches[Home].Owner=1;
            const FVector Point(FrontlineFieldV32::TrenchCentreX(Home,0),0,10000);
            auto* Enemy=Make(1,844725,EFrontlineRole::Rifle,0,Point,false);auto* Storm=Make(0,844726,EFrontlineRole::Mills,0,Point-FVector(Direction*Distance,0,0),false);
            if(!Enemy||!Storm){Check(TEXT("v8447_close_assault_actual_actors"),false);continue;}
            FFrontlineSquad S;S.Id=Enemy->SquadId;S.Team=1;S.Role=Enemy->Role;S.Trench=Home;S.Members.Add(Enemy);B->Squads.Add(S);
            Chance[Mode]=B->BulletHitChance(Storm,Enemy);Exposure[Mode]=FrontlineBalanceV50::TargetExposure(Storm,Enemy,B->CoverFor(Enemy));
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("covered_mp18_assault_probability"));J->SetStringField(TEXT("mode"),Mode?TEXT("v8447"):TEXT("v8446"));
            J->SetNumberField(TEXT("distance_cm"),Distance);J->SetNumberField(TEXT("exposure"),Exposure[Mode]);J->SetNumberField(TEXT("per_round_probability"),Chance[Mode]);J->SetBoolField(TEXT("actual_mp18"),Storm->IsStormtrooper());
            Rows.Add(MakeShared<FJsonValueObject>(J));
            Check(TEXT("v8447_mp18_keeps_historical_capacity_range_and_cadence"),Storm->IsStormtrooper()&&Storm->GetMagazineCapacity()==32
                &&FrontlineWeapons::ProfileFor(Storm).FireRangeCm==2800&&FrontlineWeapons::ProfileFor(Storm).RoundIntervalSeconds==.15f);
        }
        Check(FString::Printf(TEXT("v8447_mp18_covered_distance%d_improves"),int32(Distance)),Chance[0]>0&&Chance[1]>Chance[0]*1.18f&&Exposure[1]>Exposure[0]);
    }
    // Whole sections are settled through their actual terrain/stance Tick.
    // Thirty seconds suffices to include MP18 reloads and multiple MG bursts.
    for(int32 RoleIndex=0;RoleIndex<2;++RoleIndex)for(int32 DistanceIndex=0;DistanceIndex<2;++DistanceIndex)
    {
        const EFrontlineRole Role=RoleIndex?EFrontlineRole::Lewis:EFrontlineRole::Mills;
        const float Distance=DistanceIndex?2500.f:900.f;int32 Totals[2]={};float Expected[2]={};
        for(int32 Mode=0;Mode<2;++Mode)
        {
            Begin(Mode,EFrontlineFaction::German);B->ActiveResearchMutable().TechnologyMask=1u<<12;
            const int32 Home=B->GetHomeTrenchIndex(0),Direction=B->GetTeamDirection(0);B->Trenches[Home].Owner=0;
            FFrontlineSquad S;S.Id=844730;S.Team=0;S.Role=Role;S.Trench=S.Destination=S.LastPassedTrench=Home;S.Slot=1;S.bHoldPosition=true;B->Squads.Add(S);
            TArray<AWarfareFrontlineSoldier*> Men;
            for(int32 M=0;M<B->GetSquadSize(Role,0);++M)if(auto* U=Make(0,S.Id,Role,M,FVector(B->Trenches[Home].X,0,0),true)){B->FindSquad(S.Id)->Members.Add(U);Men.Add(U);}
            for(auto* U:Men){bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);U->SetActorLocation(Ground(Goal)+FVector(0,0,88));Pose(U,.1f);}
            for(int32 F=0;F<100;++F){B->Elapsed+=.05f;for(auto* U:Men){U->Tick(.05f);Pose(U,.05f);}}
            Check(TEXT("v8447_grounded_real_section_ready"),Men.Num()==B->GetSquadSize(Role,0));if(Men.IsEmpty())continue;
            auto* Target=Make(1,844731,EFrontlineRole::Rifle,0,Men[0]->Feet()+FVector(Direction*Distance,0,0),true);if(!Target){Check(TEXT("v8447_grounded_target"),false);continue;}
            auto* Gun=FrontlineMachinegunsV30::Get(Men[0]);bool Sight=false;
            for(float Across:{0.f,180.f,-180.f,350.f,-350.f})
            {
                Target->SetActorLocation(Ground(Men[0]->Feet()+FVector(Direction*Distance,Across,0))+FVector(0,0,88));Pose(Target,.1f);
                Sight=B->HasSight(Men[0],Target)&&(!Gun||Gun->HasInfantryFiringLine(Target));if(Sight)break;
            }
            Check(TEXT("v8447_grounded_section_has_real_firing_line"),Sight&&(!RoleIndex||(Gun&&Gun->IsSupported()&&Gun->IsTrench())));
            for(auto* U:Men){U->ShotClock=0;U->GrenadeClock=10000;U->DepartureClock=0;}
            int32 Fatal=0;const int32 Before=B->ShotsFired;float ProbabilitySum=0;
            for(int32 F=0;F<600;++F)
            {
                B->Elapsed+=.05f;
                for(auto* U:Men)
                {
                    Target->Health=100;Target->Suppression=0;Target->DeathState={};B->Morale[1]=100;
                    const int32 Prior=U->ShotsDischarged;const float Probability=B->BulletHitChance(U,Target);
                    U->Tick(.05f);Pose(U,.05f);
                    if(U->ShotsDischarged>Prior)ProbabilitySum+=Probability;
                    if(!Target->IsAlive())++Fatal;
                }
            }
            Totals[Mode]=B->ShotsFired-Before;Expected[Mode]=ProbabilitySum;
            int32 Reloads=0,Bursts=0;TArray<TSharedPtr<FJsonValue>> MemberRows;
            for(auto* U:Men){Reloads+=U->ReloadsStarted;Bursts+=U->BurstsStarted;auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("member"),U->MemberIndex);M->SetBoolField(TEXT("mp18"),U->IsStormtrooper());M->SetBoolField(TEXT("mg"),U->IsMachineGunner());M->SetNumberField(TEXT("shots"),U->ShotsDischarged);M->SetNumberField(TEXT("reloads"),U->ReloadsStarted);MemberRows.Add(MakeShared<FJsonValueObject>(M));}
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),TEXT("grounded_held_section_tick"));J->SetStringField(TEXT("mode"),Mode?TEXT("v8447"):TEXT("v8446"));J->SetStringField(TEXT("section"),RoleIndex?TEXT("mg0815_and_rifle_assistants"):TEXT("four_mp18"));
            J->SetNumberField(TEXT("longitudinal_distance_cm"),Distance);J->SetNumberField(TEXT("physical_distance_cm"),FVector::Dist2D(Men[0]->Feet(),Target->Feet()));J->SetNumberField(TEXT("seconds"),30);
            J->SetBoolField(TEXT("actual_sight"),Sight);J->SetNumberField(TEXT("actual_rounds"),Totals[Mode]);J->SetNumberField(TEXT("sum_actual_shot_probability"),Expected[Mode]);J->SetNumberField(TEXT("actual_fatal_hits"),Fatal);J->SetNumberField(TEXT("reloads"),Reloads);J->SetNumberField(TEXT("bursts"),Bursts);
            J->SetNumberField(TEXT("section_recruit_seconds"),B->EffectiveRecruitSeconds(Role,0));J->SetArrayField(TEXT("members"),MemberRows);Rows.Add(MakeShared<FJsonValueObject>(J));
            Check(TEXT("v8447_grounded_section_actually_fires"),Totals[Mode]>0);
            if(!RoleIndex)Check(TEXT("v8447_storm_section_has_four_actual_mp18_carriers_and_reload"),Men.Num()==4&&Men.ContainsByPredicate([](const auto* U){return U->IsStormtrooper()&&U->ReloadsStarted>0;}));
            const int32 Old=Men[0]->ShotsDischarged;Target->Health=100;Target->SetActorLocation(Ground(Men[0]->Feet()+FVector(Direction*(FrontlineWeapons::ProfileFor(Men[0]).FireRangeCm*B->EncounterVisibilityScale()+100.f),0,0))+FVector(0,0,88));
            Men[0]->ReturnToIdle();Men[0]->DischargeWeapon(Target);Check(TEXT("v8447_original_weapon_range_still_blocks_round_spending"),Men[0]->ShotsDischarged==Old);
        }
        if(!RoleIndex)Check(FString::Printf(TEXT("v8447_storm_distance%d_sustained_fire_improves"),DistanceIndex),Expected[1]>Expected[0]*1.12f&&Totals[1]>=Totals[0]);
    }
    return R;
}
