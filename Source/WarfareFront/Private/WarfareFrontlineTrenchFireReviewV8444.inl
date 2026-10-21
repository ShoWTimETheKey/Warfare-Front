// Included inside RunV844's existing disposable-profile world guard and restore.
// The obstacle witnesses use actual actor/skeleton rays and production Tick.
for(int32 Team=0;Team<2;++Team)for(int32 Depth=0;Depth<2;++Depth)for(int32 Heavy=0;Heavy<2;++Heavy)
{
    Clear();
    const int32 Dir=B->GetTeamDirection(Team),Trench=Depth?FrontlineBattlefield::CentreTrench:B->GetHomeTrenchIndex(Team);
    const FString Prefix=FString::Printf(TEXT("v8444_team%d_depth%d_%s"),Team,Depth,Heavy?TEXT("mg"):TEXT("rifle"));
    if(!B->Trenches.IsValidIndex(Trench)){Assert(Prefix+TEXT("_actual_trench"),false);continue;}
    B->Trenches[Trench].Owner=Team;
    const auto Section=[&](int32 Slot,EFrontlineRole Role,int32 Count)
    {
        FFrontlineSquad S;S.Id=++B->NextSquadId;S.Team=Team;S.Role=Role;S.Trench=S.Destination=S.LastPassedTrench=Trench;S.Slot=Slot;S.bHoldPosition=true;B->Squads.Add(S);
        TArray<AWarfareFrontlineSoldier*> Men;
        for(int32 M=0;M<Count;++M)if(auto* U=Make(Team,S.Id,Role,M,FVector(B->Trenches[Trench].X,(Slot-1)*630.f,0)))
        {B->FindSquad(S.Id)->Members.Add(U);Men.Add(U);}
        for(auto* U:Men){bool Reform=false;const FVector P=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);U->SetActorLocation(Ground(P)+FVector(0,0,88));Pose(U,.1f);}
        return Men;
    };
    auto Men=Section(1,Heavy?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Heavy?3:6);
    auto Upper=Section(2,EFrontlineRole::Rifle,1),Lower=Section(0,EFrontlineRole::Rifle,1);
    Assert(Prefix+TEXT("_complete_roster_and_two_resident_observers"),Men.Num()==(Heavy?3:6)&&Upper.Num()==1&&Lower.Num()==1);
    if(Men.Num()!=(Heavy?3:6)||Upper.Num()!=1||Lower.Num()!=1)continue;
    TArray<AWarfareFrontlineSoldier*> Residents=Men;Residents.Append(Upper);Residents.Append(Lower);
    for(int32 F=0;F<100;++F){B->Elapsed+=.05f;for(auto* U:Residents){U->Tick(.05f);Pose(U,.05f);}}
    auto* Subject=Men[0];auto* Gun=FrontlineMachinegunsV30::Get(Subject);
    const FVector Start=Subject->Feet();
    Assert(Prefix+TEXT("_settled_actual_supported_pose"),!Subject->bMoving&&(!Heavy||(Gun&&Gun->IsSupported()&&Gun->IsTrench())));
    if(!Heavy)
    {
        const FVector Goal=B->SoldierDestination(Subject);
        Assert(Prefix+TEXT("_real_cached_firing_step_identity_present"),FrontlineInfantryV45::HasValidatedFiringStepV831(Subject,Goal));
        Assert(Prefix+TEXT("_xy_slot_identity_survives_a_different_temporary_floor_height"),FrontlineInfantryV45::HasValidatedFiringStepV831(Subject,Goal+FVector(0,0,500.f)));
        Assert(Prefix+TEXT("_xy_slot_identity_keeps_original_tenth_cm_tolerance"),FrontlineInfantryV45::HasValidatedFiringStepV831(Subject,Goal+FVector(.09f,-.09f,500.f))
            &&!FrontlineInfantryV45::HasValidatedFiringStepV831(Subject,Goal+FVector(.11f,0,0))&&!FrontlineInfantryV45::HasValidatedFiringStepV831(Subject,Goal+FVector(0,.11f,0)));
        auto* Assignment=B->FindSquad(Subject->SquadId);const bool Advancing=Assignment->bAdvancing;Assignment->bAdvancing=true;
        const bool Released=!FrontlineInfantryV45::HasValidatedFiringStepV831(Subject,Goal+FVector(0,0,500.f));Assignment->bAdvancing=Advancing;
        Assert(Prefix+TEXT("_xy_slot_identity_does_not_survive_an_advancing_assignment"),Released);
    }
    auto* Near=Make(1-Team,++B->NextSquadId,EFrontlineRole::Rifle,0,Start+FVector(Dir*750.f,600.f,0));
    auto* Far=Make(1-Team,++B->NextSquadId,EFrontlineRole::Rifle,0,Start+FVector(Dir*1500.f,-350.f,0));
    if(!Near||!Far){Assert(Prefix+TEXT("_two_actual_contacts"),false);continue;}
    // Locate a clear pair on this operation's real bank geometry before adding
    // the deliberate asymmetric obstruction. No sight result is overridden.
    bool Baseline=false;
    for(float Along:{550.f,750.f,950.f})for(float Across:{600.f,800.f})
    {
        if(Baseline)break;Near->SetActorLocation(Ground(Start+FVector(Dir*Along,Across,0))+FVector(0,0,88));Pose(Near,.1f);
        for(float FarAlong:{1200.f,1500.f,1750.f})for(float FarAcross:{-350.f,-500.f})
        {
            Far->SetActorLocation(Ground(Start+FVector(Dir*FarAlong,FarAcross,0))+FVector(0,0,88));Pose(Far,.1f);
            Baseline=FVector::Dist2D(Start,Near->Feet())<FVector::Dist2D(Start,Far->Feet())
                &&B->HasSight(Subject,Near)&&B->HasSight(Subject,Far)&&B->HasSight(Upper[0],Near)&&B->HasSight(Lower[0],Far)
                &&(!Gun||(Gun->HasInfantryFiringLine(Near)&&Gun->HasInfantryFiringLine(Far)));
            if(Baseline)break;
        }
    }
    Assert(Prefix+TEXT("_real_nearest_and_farther_baseline"),Baseline);if(!Baseline)continue;
    const auto Observe=[&](AWarfareFrontlineSoldier* Observer,AWarfareFrontlineSoldier* Contact,AWarfareFrontlineSoldier* Other)
    {
        B->Units.Remove(Other);Observer->FocusTarget.Reset();Observer->TargetReviewClock=0;
        Observer->Tick(.05f);Pose(Observer,.05f);B->Units.Add(Other);
        return Observer->GetObservedInfantryTargetV844()==Contact;
    };
    Assert(Prefix+TEXT("_both_contacts_are_acquired_by_real_resident_ticks"),Observe(Upper[0],Near,Far)&&Observe(Lower[0],Far,Near));
    const auto Wall=[&](const FVector& Centre,const FVector& Extent)
    {
        auto* A=B->GetWorld()->SpawnActor<AActor>();if(!A)return A;Owned.Add(A);
        auto* Box=NewObject<UBoxComponent>(A);A->SetRootComponent(Box);A->AddInstanceComponent(Box);Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->RegisterComponent();A->SetActorLocation(Centre);return A;
    };
    const FVector Source=FrontlineMachinegunsV30::SightOrigin(Subject);
    auto* NearWall=Wall(FMath::Lerp(Source,FrontlineMachinegunsV30::SightTarget(Near),.5f),FVector(18,260,5000));
    auto* FarWall=Wall(Source,FVector(7,14,5000));
    if(!NearWall||!FarWall){Assert(Prefix+TEXT("_contact_walls_spawned"),false);continue;}
    FCollisionQueryParams Q(SCENE_QUERY_STAT(V8444NearestContactWitness),true);for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Q.AddIgnoredActor(U);
    const auto CandidateSees=[&](const AWarfareFrontlineSoldier* Contact)
    {
        const FVector Primary=FrontlineMachinegunsV30::SightTarget(Contact),Head=Contact->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5);
        const FVector EyeOffset=Source-Start;
        const FVector LocalMuzzle=Subject->GetActorQuat().UnrotateVector(EyeOffset);
        const int32 Count=Heavy?31:5;
        for(int32 I=0;I<Count;++I)for(float X:{125.f,101.f,77.f,53.f,29.f})
        {
            FVector P=Start;P.Y+=Heavy?(I-15)*24.f:float(I==0?0:I==1?-56:I==2?56:I==3?-112:112);
            P.X=FrontlineFieldV32::TrenchCentreX(Trench,P.Y)+Dir*X;P=Ground(P);
            if(FrontlineInfantryV45::BankIntrusion(Subject,P)>8.f)continue;
            bool Reserved=false;for(const auto* U:Residents)if(U!=Subject&&FVector::DistSquared2D(P,U->Feet())<FMath::Square(56.f)){Reserved=true;break;}
            if(Reserved)continue;
            const FVector Origin=Heavy?P+(Contact->Feet()-P).GetSafeNormal2D().Rotation().Quaternion().RotateVector(LocalMuzzle):P+EyeOffset;
            if(!B->GetWorld()->LineTraceTestByChannel(Origin,Primary,ECC_Visibility,Q)||!B->GetWorld()->LineTraceTestByChannel(Origin,Head,ECC_Visibility,Q))return true;
        }
        return false;
    };
    bool Witness=false;
    for(float Fraction:{.12f,.18f,.24f,.30f,.36f})
    {
        FarWall->SetActorLocation(FMath::Lerp(Source,FrontlineMachinegunsV30::SightTarget(Far),Fraction));
        Witness=!B->HasSight(Subject,Near)&&!B->HasSight(Subject,Far)&&B->HasSight(Upper[0],Near)&&B->HasSight(Lower[0],Far)
            &&!CandidateSees(Near)&&CandidateSees(Far);
        if(Witness)break;
    }
    TArray<AWarfareFrontlineSoldier*> Contacts;FrontlineInfantryV45::ObservedTrenchContactsV8444(Subject,Contacts);
    Assert(Prefix+TEXT("_legacy_nearest_only_search_has_no_candidate_but_second_observed_contact_has_one"),Witness&&Contacts.Num()>=2
        &&FrontlineInfantryV45::ObservedTrenchContactV844(Subject)==Near);
    if(!Witness)continue;
    const int32 Before=Subject->ShotsDischarged,BurstsBefore=Subject->BurstsStarted,DeployBefore=Gun?Gun->Deployments:0,PackBefore=Gun?Gun->Packs:0;
    const float Begin=B->Elapsed;Subject->ShotClock=0;float MaxStep=0;
    const int32 Needed=Heavy?12:2;
    // The German gun can legally fire fourteen rounds in its first burst.
    // A twelve-round threshold alone cannot witness the recovery/next burst.
    for(int32 F=0;F<400&&(Subject->ShotsDischarged<Before+Needed||(Heavy&&Subject->BurstsStarted<BurstsBefore+2));++F)
    {
        if(!Far->IsAlive())
        {
            const FVector At=Far->Feet();B->Units.Remove(Far);Far->Destroy();Far=Make(1-Team,++B->NextSquadId,EFrontlineRole::Rifle,0,At);
            if(!Far)break;Observe(Lower[0],Far,Near);
        }
        B->Elapsed+=.05f;
        for(auto* U:Residents){const FVector Old=U->Feet();U->Tick(.05f);Pose(U,.05f);MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Old,U->Feet())));}
    }
    const int32 Fired=Subject->ShotsDischarged-Before;
    Assert(Prefix+TEXT("_second_contact_produces_actual_discharge_across_weapon_cycles"),Fired>=Needed&&(!Heavy||Subject->BurstsStarted-BurstsBefore>=2));
    Assert(Prefix+TEXT("_refit_keeps_physical_step_and_bounded_pack_deploy"),MaxStep<30.f&&FrontlineInfantryV45::BankIntrusion(Subject,Subject->Feet())<=8.1f
        &&(!Gun||(Gun->Deployments-DeployBefore<=1&&Gun->Packs-PackBefore<=1)));
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("fixture"),Prefix);Row->SetBoolField(TEXT("legacy_nearest_has_no_candidate"),!CandidateSees(Near));
    Row->SetStringField(TEXT("start"),Start.ToString());Row->SetStringField(TEXT("finish"),Subject->Feet().ToString());Row->SetStringField(TEXT("nearest_contact"),Near->Feet().ToString());
    Row->SetStringField(TEXT("farther_contact"),Far?Far->Feet().ToString():TEXT("spawn_failed"));Row->SetNumberField(TEXT("real_contact_count"),Contacts.Num());
    Row->SetNumberField(TEXT("shots"),Fired);Row->SetNumberField(TEXT("bursts"),Subject->BurstsStarted-BurstsBefore);Row->SetNumberField(TEXT("seconds"),B->Elapsed-Begin);
    TArray<TSharedPtr<FJsonValue>> Times;float LongestInterval=0;
    for(int32 I=Before;I<Subject->ShotTimes.Num();++I)
    {Times.Add(MakeShared<FJsonValueNumber>(Subject->ShotTimes[I]));if(I>Before)LongestInterval=FMath::Max(LongestInterval,Subject->ShotTimes[I]-Subject->ShotTimes[I-1]);}
    Row->SetArrayField(TEXT("actual_shot_times_seconds"),Times);Row->SetNumberField(TEXT("longest_actual_intershot_seconds"),LongestInterval);
    Row->SetNumberField(TEXT("max_step_cm"),MaxStep);Row->SetNumberField(TEXT("extra_packs"),Gun?Gun->Packs-PackBefore:0);Row->SetNumberField(TEXT("extra_deployments"),Gun?Gun->Deployments-DeployBefore:0);
    if(Gun)Row->SetObjectField(TEXT("actual_gun_sector"),Gun->DescribeTrenchFiringSectorV844());Rows.Add(MakeShared<FJsonValueObject>(Row));
    // Re-evaluate contact lifecycle and range independently of a successful
    // placement cache; a shared observation cannot spend a round through cover.
    const int32 Count=Subject->ShotsDischarged,Rounds=Subject->RoundsRemaining,Combat=B->ShotsFired;
    Subject->ReturnToIdle();Subject->DischargeWeapon(Near);
    Assert(Prefix+TEXT("_nearest_opaque_contact_still_cannot_spend_ammunition"),Subject->ShotsDischarged==Count&&Subject->RoundsRemaining==Rounds&&B->ShotsFired==Combat);
    for(auto* U:Residents)if(U!=Subject)U->Health=0;
    FrontlineInfantryV45::ObservedTrenchContactsV8444(Subject,Contacts);
    Assert(Prefix+TEXT("_dead_observers_do_not_retain_shared_contacts"),!Contacts.Contains(Far)&&!Contacts.Contains(Near));
    for(auto* U:Residents)if(U!=Subject)U->Health=100;
    NearWall->SetActorEnableCollision(false);FarWall->SetActorEnableCollision(false);B->Units.Remove(Near);Near->Destroy();if(Far){B->Units.Remove(Far);Far->Destroy();}
    // Full rosters span distinct Y bays and start from differing actor headings.
    // These observations use evaluated idle/walk/aim assets and ordinary Tick.
    for(auto* U:Men)U->ShotClock=10000;
    for(auto* U:Men)
    {
        U->ReturnToIdle();U->SetActorRotation(FRotator(0,(Dir>0?0.f:180.f)+(U->MemberIndex%2?180.f:0.f),0));
        U->SetMotion(U->MemberIndex%2?U->GetWalkMotion():U->GetIdleMotion());Pose(U,.1f);
        auto* Target=Make(1-Team,++B->NextSquadId,EFrontlineRole::Rifle,0,U->Feet()+FVector(Dir*650.f,0,0));if(!Target){Assert(Prefix+TEXT("_roster_target"),false);continue;}
        const int32 Old=U->ShotsDischarged;U->ShotClock=0;
        for(int32 F=0;F<180&&U->ShotsDischarged==Old;++F){B->Elapsed+=.05f;U->Tick(.05f);Pose(U,.05f);}
        Assert(Prefix+FString::Printf(TEXT("_member%d_distinct_y_and_pose_really_fires"),U->MemberIndex),U->ShotsDischarged>Old&&!U->bMoving);
        U->ReturnToIdle();const float Range=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
        Target->Health=100;Target->SetActorLocation(Ground(U->Feet()+FVector(Dir*(Range+60.f),0,0))+FVector(0,0,88));Pose(Target,.1f);
        const int32 Out=U->ShotsDischarged;U->DischargeWeapon(Target);
        Assert(Prefix+FString::Printf(TEXT("_member%d_original_range_limit_remains"),U->MemberIndex),U->ShotsDischarged==Out);
        B->Units.Remove(Target);Target->Destroy();U->ShotClock=10000;
    }
}

// The first Shipping QA found no supported same-footpoint >75-degree bay.
// That speculative production extension was withdrawn; this is an explicit
// coverage report, not a passing check or a newly measured geometry scan.
auto AngleCoverage=MakeShared<FJsonObject>();
AngleCoverage->SetStringField(TEXT("status"),TEXT("not_exercised"));
AngleCoverage->SetBoolField(TEXT("production_extension_withdrawn"),true);
AngleCoverage->SetBoolField(TEXT("current_runtime_scan_performed"),false);
AngleCoverage->SetNumberField(TEXT("prior_physical_pairs"),0);
AngleCoverage->SetStringField(TEXT("prior_failed_receipt"),TEXT("Saved/BugAudit/V8444/Runtime/20261009-164353-484/trench-fire.json"));
AngleCoverage->SetStringField(TEXT("reason"),TEXT("The bounded actual UK/DE home/centre three-slot scan found no original-footpoint bipod-supported over-traverse candidate. No claim that this path reproduced the user's bug. Keep the established 70-degree in-place review and current fire arc."));
Result->SetObjectField(TEXT("over_traverse_coverage"),AngleCoverage);
Result->SetObjectField(TEXT("sector_candidate_selector"),RunFrontlineTrenchSectorSelectorChecksV8444(Assert));
