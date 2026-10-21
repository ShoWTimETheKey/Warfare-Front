// Included inside the bounded natural scheduler. One real plan and one chosen
// story policy per process; prior campaign prerequisites alone are synthetic.
struct FFrenchPlanReviewStateV87
{
    int32 Step=0,Worker=INDEX_NONE;
    FString Choice,Target,ObserverPost,WorkerPost,Wait,PlanId;
    double NextOrder=0;
    float ObservationAt=-1,DraftAt=-1,CommitAt=-1,FourthAt=-1,FollowupAt=-1;
    bool WorkerApproachOrdered=false,WorkerLeaving=false,ObserverOrdered=false;
    bool FirstAssemblyOrdered=false,SecondAssemblyOrdered=false,FollowupsPrepared=false;
    bool EarlyFollowupRefused=false,RoutesAccepted=false,Moved=false;
    TMap<int32,double> Receipts;TMap<int32,FVector> StartFeet;
} PlanReviewV87;
bool PrepareFrenchPlanResearchV87()
{
    auto* B=Battle();if(!B)return false;
    const int32 Before=WarfareFRRUCampaignV82::ResearchPoints(B);
    const auto* First=wfexp::tactical::FindOption("fr.tech.2");const auto* Pair=wfexp::tactical::FindOption("fr.tech.6");
    if(!First||!Pair||Before<First->cost+Pair->cost)return false;
    const bool Accepted=WarfareFRRUCampaignV82::PurchaseOption(B,TEXT("fr.tech.2"))
        &&WarfareFRRUCampaignV82::PurchaseOption(B,TEXT("fr.tech.6"));
    const int32 After=WarfareFRRUCampaignV82::ResearchPoints(B);
    Check(TEXT("chemin_plan_three_points_use_actual_prerequisite_purchases"),Accepted&&Before-After==First->cost+Pair->cost
        &&First->cost==1&&Pair->cost==2&&WarfareFRRUCampaignV82::OwnsOption(B,TEXT("fr.tech.6")));
    auto Evidence=MakeShared<FJsonObject>();Evidence->SetNumberField(TEXT("actualMissionDate"),19170416);
    Evidence->SetNumberField(TEXT("fixturePointsBefore"),Before);Evidence->SetNumberField(TEXT("actualPointsAfter"),After);
    Evidence->SetStringField(TEXT("purchasedOptions"),TEXT("fr.tech.2 -> fr.tech.6; no phase-two follow-up option"));
    RequiredEvent(TEXT("chemin_plan_real_research_budget"),TEXT("prior victories are synthetic; both purchases use the ordinary campaign API and exact costs"),Evidence);
    return Accepted&&FirstFailure.IsEmpty();
}
bool FrenchPlanAtV87(int32 Id,const FString& NodeId)const
{
    const auto* B=Battle();const auto* Q=B?B->FindSquad(Id):nullptr;const auto* U=Member(Id);
    if(!Q||!U||Q->bAdvancing||!At(Id,NodeId))return false;
    FString NativeNode;return WarfareFRRUWorldV82::MovementDiagnosis(U,false)->TryGetStringField(TEXT("actualNativeNode"),NativeNode)&&NativeNode==NodeId;
}
TSharedRef<FJsonObject> FrenchPlanReportV87()const
{
    auto D=MakeShared<FJsonObject>();D->SetStringField(TEXT("scope"),TEXT("one real four-salvo plan and two-section policy; operation victory is not required"));
    D->SetStringField(TEXT("choice"),PlanReviewV87.Choice);D->SetStringField(TEXT("fireTarget"),PlanReviewV87.Target);
    D->SetStringField(TEXT("observerPost"),PlanReviewV87.ObserverPost);D->SetStringField(TEXT("latestWait"),PlanReviewV87.Wait);
    D->SetStringField(TEXT("followupAssemblyPost"),TEXT("assembly"));D->SetBoolField(TEXT("bothFollowupsPhysicallyPrepared"),PlanReviewV87.FollowupsPrepared);
    D->SetNumberField(TEXT("step"),PlanReviewV87.Step);D->SetNumberField(TEXT("worker"),PlanReviewV87.Worker);
    D->SetNumberField(TEXT("observer"),Observer);D->SetNumberField(TEXT("firstFollowup"),Lead);D->SetNumberField(TEXT("secondFollowup"),Second);
    D->SetNumberField(TEXT("firstObservationAt"),PlanReviewV87.ObservationAt);D->SetNumberField(TEXT("actualCommitAt"),PlanReviewV87.CommitAt);
    D->SetNumberField(TEXT("actualFourthSalvoAt"),PlanReviewV87.FourthAt);D->SetNumberField(TEXT("actualFollowupAt"),PlanReviewV87.FollowupAt);
    D->SetBoolField(TEXT("fourthPhaseEarlyRefusal"),PlanReviewV87.EarlyFollowupRefused);
    D->SetBoolField(TEXT("twoActualFollowupRoutes"),PlanReviewV87.RoutesAccepted);
    D->SetBoolField(TEXT("bothRoutesActuallyMoved"),PlanReviewV87.Moved);
    if(const auto* B=Battle())D->SetObjectField(TEXT("actualSupportState"),WarfareFRRUWorldV82::SupportDiagnosis(B));
    return D;
}
bool TickFrenchPlanV87(double Now)
{
    auto* B=Battle();if(!B||UGameplayStatics::IsGamePaused(B))return true;
    auto& P=PlanReviewV87;const auto View=WarfareFRRUWorldV82::View(B);
    if(B->Phase!=EFrontlinePhase::Fighting||View.Finished)
        return Fail(TEXT("chemin_plan_window_lost_to_native_outcome"),B->EndReason+TEXT(" | ")+P.Wait);
    if(Now<P.NextOrder)return true;P.NextOrder=Now+.10;
    const auto State=WarfareFRRUWorldV82::SupportDiagnosis(B);
    if(P.Choice.IsEmpty())
    {
        const auto Diagnosis=WarfareFRRUWorldV82::ObjectiveDiagnosis(B);const TArray<TSharedPtr<FJsonValue>>* Choices=nullptr;
        if(Diagnosis->TryGetArrayField(TEXT("actualStoryChoices"),Choices))for(const auto& Choice:*Choices)
            if(Choice->AsObject()->GetStringField(TEXT("story"))==TEXT("fr.chemin_dames.local_order"))P.Choice=Choice->AsObject()->GetStringField(TEXT("choice"));
        if(P.Choice.IsEmpty()){P.Wait=TEXT("actual public story selection");return true;}
        P.Target=P.Choice==TEXT("alternative")?TEXT("rail"):TEXT("wire");
        P.ObserverPost=TEXT("relay");
        P.WorkerPost=P.Choice==TEXT("alternative")?TEXT("front_right"):TEXT("front_left");
        const FString Policy=State->GetStringField(TEXT("followupPolicyV87"));
        Check(TEXT("chemin_actual_story_sets_requested_followup_policy"),State->GetBoolField(TEXT("followupPolicyActiveV87"))
            &&Policy==(P.Choice==TEXT("alternative")?TEXT("reserve_for_second_route"):TEXT("limited_objective_then_hold")));
        if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
    }
    if(P.Step==0)
    {
        // A dead role cannot arrive later. Retain genuine combat loss as such,
        // instead of waiting until the outer review deadline hides the cause.
        if((Observer!=INDEX_NONE&&!Member(Observer))||(Lead!=INDEX_NONE&&!Member(Lead))||(Second!=INDEX_NONE&&!Member(Second)))
            return Fail(TEXT("chemin_plan_participant_lost_before_commit"),P.Wait);
        // The work party starts first. Prepare the two real follow-up groups
        // at the registered rear assembly post before assigning the timed
        // policy. Neither a route allocation nor a home-bay hold proves that
        // its later FIFO/transit approach can actually execute.
        int32* Next=nullptr;EFrontlineRole Role=EFrontlineRole::Rifle;
        if(P.Worker==INDEX_NONE){Next=&P.Worker;Role=EFrontlineRole::Pioneer;}
        else if(Observer==INDEX_NONE){Next=&Observer;Role=EFrontlineRole::Marksman;}
        else if(Lead==INDEX_NONE)Next=&Lead;else if(Second==INDEX_NONE)Next=&Second;
        if(Next&&B->RecruitBlockReason(Role,0).IsEmpty())
        {
            *Next=Recruit(Role,0);if(*Next==INDEX_NONE)return Fail(TEXT("chemin_plan_public_recruit"),B->LastMessage);
        }
        for(const int32 Id:{Observer})if(auto* Q=B->FindSquad(Id);Q&&!Q->bHoldPosition&&FrenchPlanAtV87(Id,TEXT("home")))
            Event(TEXT("chemin_plan_hold_real_rear_arrival"),WarfareFRRUWorldV82::HoldSquad(B,Id,true));
        if(!P.WorkerApproachOrdered&&FrenchPlanAtV87(P.Worker,TEXT("home")))
        {P.WorkerApproachOrdered=WarfareFRRUWorldV82::MoveSquad(B,P.Worker,P.Target);Event(TEXT("chemin_public_worker_approaches_chosen_gap"),P.WorkerApproachOrdered);}
        // Observation travels in parallel with the work party; do not send a
        // two-man observer alone across the exposed ridge 150 seconds later.
        if(!P.ObserverOrdered&&FrenchPlanAtV87(Observer,TEXT("home")))
        {P.ObserverOrdered=WarfareFRRUWorldV82::MoveSquad(B,Observer,P.ObserverPost);Event(TEXT("chemin_actual_observer_early_relay_route"),P.ObserverOrdered);}
        if(P.ObserverOrdered&&FrenchPlanAtV87(Observer,P.ObserverPost))
            if(auto* Q=B->FindSquad(Observer);Q&&!Q->bHoldPosition)Event(TEXT("chemin_hold_actual_relay_observer"),WarfareFRRUWorldV82::HoldSquad(B,Observer,true));
        for(const int32 Id:{Lead,Second})if(Id!=INDEX_NONE)
        {
            bool& Ordered=Id==Lead?P.FirstAssemblyOrdered:P.SecondAssemblyOrdered;
            if(!Ordered&&FrenchPlanAtV87(Id,TEXT("home")))
            {Ordered=WarfareFRRUWorldV82::MoveSquad(B,Id,TEXT("assembly"));Event(TEXT("chemin_followup_public_rear_assembly_route"),Ordered);}
            if(Ordered&&FrenchPlanAtV87(Id,TEXT("assembly")))
                if(auto* Q=B->FindSquad(Id);Q&&!Q->bHoldPosition)
                    Event(TEXT("chemin_followup_holds_actual_assembly_bay"),WarfareFRRUWorldV82::HoldSquad(B,Id,true));
        }
        if(!WarfareFRRUWorldV82::PassageCleared(B,P.Target))
        {
            if(P.Worker!=INDEX_NONE&&!Member(P.Worker))return Fail(TEXT("chemin_plan_worker_lost_before_clearance"),P.Target);
            P.Wait=TEXT("real pioneer clears ")+P.Target;return true;
        }
        // Vacate the registered shell target through a real route. No soldier
        // is shifted, healed or removed to make the draft appear safe.
        if(Member(P.Worker)&&!P.WorkerLeaving&&FrenchPlanAtV87(P.Worker,P.Target))
        {P.WorkerLeaving=WarfareFRRUWorldV82::MoveSquad(B,P.Worker,P.WorkerPost);Event(TEXT("chemin_worker_leaves_actual_shell_target"),P.WorkerLeaving);}
        const bool WorkerAway=!Member(P.Worker)||P.WorkerLeaving&&FrenchPlanAtV87(P.Worker,P.WorkerPost);
        if(!WorkerAway){P.Wait=TEXT("worker physically vacates the cleared crossing");return true;}
        if(auto* Q=B->FindSquad(P.Worker);Q&&Q->LivingCount()>0&&!Q->bHoldPosition)
            Event(TEXT("chemin_worker_holds_safe_adjacent_post"),WarfareFRRUWorldV82::HoldSquad(B,P.Worker,true));
        if(P.ObserverOrdered&&FrenchPlanAtV87(Observer,P.ObserverPost))
        {
            if(auto* Q=B->FindSquad(Observer);Q&&!Q->bHoldPosition)Event(TEXT("chemin_hold_actual_observer"),WarfareFRRUWorldV82::HoldSquad(B,Observer,true));
        }
        else {P.Wait=TEXT("actual stationary forward observer at ")+P.ObserverPost;return true;}
        if(!P.FirstAssemblyOrdered||!P.SecondAssemblyOrdered||!FrenchPlanAtV87(Lead,TEXT("assembly"))||!FrenchPlanAtV87(Second,TEXT("assembly"))
            ||!B->FindSquad(Lead)->bHoldPosition||!B->FindSquad(Second)->bHoldPosition)
        {P.Wait=TEXT("two distinct follow-ups physically settle and hold their registered rear assembly bays");return true;}
        if(!P.FollowupsPrepared)
        {
            P.FollowupsPrepared=true;Check(TEXT("chemin_both_followups_physically_assemble_before_assignment"),true);
            auto Evidence=MakeShared<FJsonObject>();Evidence->SetObjectField(TEXT("firstActualBay"),WarfareFRRUWorldV82::MovementDiagnosis(Member(Lead),false));
            Evidence->SetObjectField(TEXT("secondActualBay"),WarfareFRRUWorldV82::MovementDiagnosis(Member(Second),false));
            RequiredEvent(TEXT("chemin_actual_rear_followup_assembly"),TEXT("two public Move orders, full real arrival at separate legal assembly bays, then public Hold; no relocation or arrival injection"),Evidence);
        }
        if(!B->SupportBlockReason(EFrontlineSupport::Artillery,0).IsEmpty())
        {P.Wait=B->SupportBlockReason(EFrontlineSupport::Artillery,0);return true;}
        const bool Planned=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::PlanFire,{},P.Target);
        Event(TEXT("chemin_fixed_four_salvo_plan"),Planned);if(!Planned){P.Wait=WarfareFRRUWorldV82::View(B).Notice;return true;}
        const int32 Shells=B->Strikes.Num(),Calls=B->ArtilleryCommands;const float Ready=B->SupportCooldown(EFrontlineSupport::Artillery,0);
        const bool EarlyCommit=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::CommitFire);
        Check(TEXT("chemin_unobserved_commit_rejected_without_ammo_or_cooldown"),!EarlyCommit&&B->Strikes.Num()==Shells&&B->ArtilleryCommands==Calls
            &&FMath::IsNearlyEqual(Ready,B->SupportCooldown(EFrontlineSupport::Artillery,0),.001f));
        const bool Assigned=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::UseOption,{Lead,Second},FString(),TEXT("fr.tech.6"));
        Event(TEXT("chemin_assign_two_existing_followups"),Assigned);
        if(!Assigned)return Fail(TEXT("chemin_existing_followup_assignment"),WarfareFRRUWorldV82::View(B).Notice);
        P.Step=1;P.DraftAt=B->Elapsed;P.Wait=TEXT("four continuous seconds of actual observer LOS");
    }
    if(P.Step==1)
    {
        if(!Member(Observer)||!Member(Lead)||!Member(Second))return Fail(TEXT("chemin_plan_participant_lost_before_commit"),P.Wait);
        const bool Observed=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::Observe,{Observer});
        if(!Observed){P.Wait=WarfareFRRUWorldV82::View(B).Notice;
            if(B->Elapsed-P.DraftAt>=12.f)return Fail(TEXT("chemin_stationary_relay_lacks_actual_observation_proof"),P.Wait);
            return true;}
        const auto Actual=WarfareFRRUWorldV82::SupportDiagnosis(B);P.ObservationAt=float(Actual->GetNumberField(TEXT("observationStartV87")));
        if(WarfareFRRUWorldV82::View(B).FirePhase!=3)return true;
        const double Duration=Actual->GetNumberField(TEXT("lastObservationV87"))-Actual->GetNumberField(TEXT("observationStartV87"));
        Check(TEXT("chemin_real_observer_completes_four_continuous_host_seconds"),Duration>=4.&&Duration<5.1
            &&Actual->GetNumberField(TEXT("observationRequiredV87"))==4.&&Actual->GetNumberField(TEXT("observerSquadV87"))==Observer);
        const int32 Shells=B->Strikes.Num(),Calls=B->ArtilleryCommands;
        const bool Commit=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::CommitFire);Event(TEXT("chemin_commit_actual_host_artillery"),Commit);
        if(!Commit)return Fail(TEXT("chemin_observed_commit"),WarfareFRRUWorldV82::View(B).Notice);
        P.CommitAt=B->Elapsed;const auto Committed=WarfareFRRUWorldV82::SupportDiagnosis(B);P.PlanId=Committed->GetStringField(TEXT("planIdV87"));
        TSet<int32> Phases;TSet<FString> Rounds;
        for(int32 I=Shells;I<B->Strikes.Num();++I){const auto& Shell=B->Strikes[I];Phases.Add(Shell.SalvoIndex);Rounds.Add(FString::Printf(TEXT("%d.%d"),Shell.SalvoIndex,Shell.RoundIndex));}
        Check(TEXT("chemin_one_commit_reserves_twelve_distinct_host_shells_four_salvos"),B->Strikes.Num()-Shells==12&&B->ArtilleryCommands==Calls+1&&Phases.Num()==4&&Rounds.Num()==12&&!P.PlanId.IsEmpty());
        const bool Early=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::Followup,{Lead,Second});
        const auto EarlyState=WarfareFRRUWorldV82::SupportDiagnosis(B);
        P.EarlyFollowupRefused=!Early&&!EarlyState->GetBoolField(TEXT("followupUsedV87"))&&B->FindSquad(Lead)->bHoldPosition&&B->FindSquad(Second)->bHoldPosition;
        Check(TEXT("chemin_followup_before_actual_fourth_receipt_is_rejected"),P.EarlyFollowupRefused);
        auto Evidence=MakeShared<FJsonObject>();Evidence->SetObjectField(TEXT("actualStateAfterCommit"),Committed);
        Evidence->SetNumberField(TEXT("continuousActualObservationSeconds"),Duration);Evidence->SetNumberField(TEXT("actualNewShells"),B->Strikes.Num()-Shells);
        RequiredEvent(TEXT("chemin_real_observation_and_commit"),TEXT("existing stationary national observer and actual scene/smoke LOS; no observation or ammunition injection"),Evidence);
        P.Step=2;P.Wait=TEXT("four actual three-round salvo completions");
    }
    if(P.Step==2)
    {
        const auto Actual=WarfareFRRUWorldV82::SupportDiagnosis(B);const TArray<TSharedPtr<FJsonValue>>* Receipts=nullptr;
        if(Actual->TryGetArrayField(TEXT("activeActualPhaseReceipts"),Receipts))for(const auto& Value:*Receipts)
        {
            const auto Receipt=Value->AsObject();if(Receipt->GetStringField(TEXT("planId"))!=P.PlanId||Receipt->GetNumberField(TEXT("side"))!=0)continue;
            const int32 Phase=int32(Receipt->GetNumberField(TEXT("resolvedSalvos")));const double At=Receipt->GetNumberField(TEXT("resolvedAt"));
            if(Phase<1||Phase>4||At<P.CommitAt||At>B->Elapsed)return Fail(TEXT("chemin_actual_salvo_receipt_identity_or_clock"),P.PlanId);
            if(!P.Receipts.Contains(Phase))
            {P.Receipts.Add(Phase,At);auto Evidence=MakeShared<FJsonObject>(*Receipt);RequiredEvent(FString::Printf(TEXT("chemin_actual_salvo_%d_completed"),Phase),TEXT("durable journal produced by actual resolved host shells"),Evidence);}
        }
        if(!P.Receipts.Contains(4))return true;
        P.FourthAt=float(P.Receipts.FindChecked(4));bool Ordered=P.Receipts.Num()==4;
        for(int32 Phase=2;Phase<=4;++Phase)Ordered&=P.Receipts.Contains(Phase-1)&&P.Receipts.FindChecked(Phase)>=P.Receipts.FindChecked(Phase-1);
        Check(TEXT("chemin_four_actual_salvo_receipts_have_same_plan_monotone_times"),Ordered);
        if(!Member(Lead)||!Member(Second))return Fail(TEXT("chemin_plan_participant_lost_before_followup"),P.Wait);
        const bool Accepted=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::Followup,{Lead,Second});Event(TEXT("chemin_public_followup_inside_actual_receipt_window"),Accepted);
        if(!Accepted)return Fail(TEXT("chemin_fourth_receipt_followup_requires_legal_real_routes"),WarfareFRRUWorldV82::View(B).Notice);
        P.FollowupAt=B->Elapsed;const auto* First=B->FindSquad(Lead);const auto* Next=B->FindSquad(Second);
        const auto FirstRoute=WarfareFRRUWorldV82::MovementDiagnosis(Member(Lead),false),SecondRoute=WarfareFRRUWorldV82::MovementDiagnosis(Member(Second),false);
        const FString ExpectedFirst=P.Choice==TEXT("alternative")?P.Target:TEXT("objective");
        const auto Followed=WarfareFRRUWorldV82::SupportDiagnosis(B);const TArray<TSharedPtr<FJsonValue>>* Holds=nullptr;int32 PolicyHolds=0;
        if(Followed->TryGetArrayField(TEXT("arrivalHoldsV87"),Holds))for(const auto& H:*Holds)
            if((H->AsObject()->GetNumberField(TEXT("squad"))==Lead||H->AsObject()->GetNumberField(TEXT("squad"))==Second)&&H->AsObject()->GetStringField(TEXT("target"))==TEXT("objective"))++PolicyHolds;
        P.RoutesAccepted=Lead!=Second&&First&&Next&&First->bAdvancing&&Next->bAdvancing&&!First->bHoldPosition&&!Next->bHoldPosition
            &&FirstRoute->GetStringField(TEXT("targetNode"))==ExpectedFirst&&SecondRoute->GetStringField(TEXT("targetNode"))==TEXT("objective")
            &&Followed->GetBoolField(TEXT("followupUsedV87"))&&PolicyHolds==(P.Choice==TEXT("alternative")?0:2);
        Check(TEXT("chemin_story_policy_assigns_two_distinct_real_routes_and_hold_rules"),P.RoutesAccepted);
        Check(TEXT("chemin_followup_is_within_eight_real_host_seconds_of_fourth_salvo"),P.FollowupAt>=P.FourthAt&&P.FollowupAt-P.FourthAt<=8.f);
        const bool Duplicate=WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::Followup,{Lead,Second});
        Check(TEXT("chemin_used_followup_cannot_release_the_pair_twice"),!Duplicate&&WarfareFRRUWorldV82::SupportDiagnosis(B)->GetBoolField(TEXT("followupUsedV87")));
        auto Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("choice"),P.Choice);Evidence->SetObjectField(TEXT("firstActualRoute"),FirstRoute);
        Evidence->SetObjectField(TEXT("secondActualRoute"),SecondRoute);Evidence->SetObjectField(TEXT("actualPolicyState"),Followed);
        Evidence->SetNumberField(TEXT("actualSecondsSinceFourthReceipt"),P.FollowupAt-P.FourthAt);
        RequiredEvent(TEXT("chemin_actual_two_section_followup_policy"),TEXT("actual public command, live physical bays/routes, policy-specific destinations and pending arrival holds"),Evidence);
        P.StartFeet.Add(Lead,Member(Lead)->Feet());P.StartFeet.Add(Second,Member(Second)->Feet());P.Step=3;P.Wait=TEXT("both released sections execute real first movement");
    }
    if(P.Step==3)
    {
        bool Both=true;for(const int32 Id:{Lead,Second})
        {const auto* U=Member(Id);Both&=U&&P.StartFeet.Contains(Id)&&FVector::Dist2D(U->Feet(),P.StartFeet.FindChecked(Id))>50.;}
        if(Both){P.Moved=true;Check(TEXT("chemin_both_followup_sections_physically_start_their_assigned_route"),true);Finish();return !Finished;}
        if(B->Elapsed-P.FollowupAt>=45.f)return Fail(TEXT("chemin_followup_route_did_not_start_real_movement"),P.Wait);
    }
    if(!FirstFailure.IsEmpty()){Finish();return !Finished;}return true;
}
