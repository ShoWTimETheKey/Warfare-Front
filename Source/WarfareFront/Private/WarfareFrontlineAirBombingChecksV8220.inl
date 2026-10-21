// Included inside FrontlineSupportV32 to inspect the actual registered sorties.
bool RequestBomberWithMetricsV8220(AWarfareFrontlineGameMode* B,const FVector& Target,const TCHAR* Stage)
{
    const auto Loadout=FrontlineCampaignV32::GetLoadout(B);
    const FString Block=B->SupportBlockReason(EFrontlineSupport::AirBombing,0);
    const float ReadyIn=B->SupportCooldown(EFrontlineSupport::AirBombing,0);
    const int32 PriorStrikes=B->Strikes.Num(),PriorAircraft=States.FindOrAdd(B).Air.Num();
    const bool Ordered=Request(B,EFrontlineSupport::AirBombing,Target,0);
    auto Metric=MakeShared<FJsonObject>();Metric->SetStringField(TEXT("stage"),Stage);Metric->SetStringField(TEXT("target"),Target.ToString());
    Metric->SetBoolField(TEXT("ordered"),Ordered);Metric->SetStringField(TEXT("support_block_reason_before_request"),Block);
    Metric->SetStringField(TEXT("support_block_reason_after_request"),B->SupportBlockReason(EFrontlineSupport::AirBombing,0));
    Metric->SetNumberField(TEXT("airplane_count_before"),PriorAircraft);Metric->SetNumberField(TEXT("airplane_count_after"),States.FindOrAdd(B).Air.Num());
    Metric->SetNumberField(TEXT("strike_count_before"),PriorStrikes);Metric->SetNumberField(TEXT("strike_count_after"),B->Strikes.Num());
    Metric->SetNumberField(TEXT("eligible_support_mask"),Loadout.EligibleSupports);Metric->SetNumberField(TEXT("selected_support_mask"),Loadout.SelectedSupports);
    Metric->SetNumberField(TEXT("support_capacity"),Loadout.SupportCapacity);Metric->SetNumberField(TEXT("cooldown_before_request"),ReadyIn);
    Metric->SetBoolField(TEXT("support_unlocked"),B->IsSupportUnlocked(EFrontlineSupport::AirBombing,0));Metric->SetBoolField(TEXT("game_paused"),UGameplayStatics::IsGamePaused(B));
    Metric->SetBoolField(TEXT("actual_aircraft_asset_ready"),FrontlineSupportArtV36::AircraftFor(B,0)!=nullptr);
    Metric->SetStringField(TEXT("asset_failure"),B->AssetFailure);Metric->SetNumberField(TEXT("unit_count"),B->Units.Num());Metric->SetNumberField(TEXT("squad_count"),B->Squads.Num());
    int32 Friendly=0;for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->IsActiveCombatant())++Friendly;
    Metric->SetNumberField(TEXT("active_friendly_unit_count"),Friendly);
    if(const auto* Campaign=B->GetCurrentCampaign())
    {Metric->SetStringField(TEXT("campaign_id"),Campaign->Id.ToString());Metric->SetNumberField(TEXT("selected_mission"),Campaign->SelectedMission);Metric->SetNumberField(TEXT("saved_support_selection_mask"),Campaign->SupportSelectionMask);}
    if(const auto* Mission=FrontlineCampaignV32::CurrentOperation(B))Metric->SetNumberField(TEXT("bound_operation_id"),Mission->Id);
    FrontlineAirSupportReviewV8220::AppendMetric(B,TEXT("bomber_requests"),Metric);return Ordered;
}
void RunAirBombingChecksV8220(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check,TFunctionRef<void()> Resolve)
{
    FString Dir;
    if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir))
    {Check(TEXT("v8220_bomber_requires_exact_disposable_profile"),false);return;}
    const FState Before=States.FindOrAdd(B);const auto Strikes=B->Strikes;
    const float Clock=B->Elapsed,Outro=B->BattleOutroElapsed;const int32 Winner=B->BattleOutroWinner;
    const auto Phase=B->Phase;const FString Message=B->LastMessage;
    States.FindOrAdd(B)=FState(); // StopPlanes below owns only this review's sorties.
    auto StopPlanes=[&]()
    {
        for(auto& A:States.FindOrAdd(B).Air)if(auto* Plane=A.Aircraft.Get())
        {if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->UpdateAircraftAudio(int32(Plane->GetUniqueID()),Plane->GetActorLocation(),0.f,false);Plane->Destroy();}
    };
    ON_SCOPE_EXIT{StopPlanes();States.FindOrAdd(B)=Before;B->Strikes=Strikes;B->Elapsed=Clock;B->Phase=Phase;B->BattleOutroElapsed=Outro;B->BattleOutroWinner=Winner;B->LastMessage=Message;};
    auto Ready=[&]()
    {
        StopPlanes();auto& State=States.FindOrAdd(B);State=FState();State.Encounter=B->GetEncounterId();State.EnemyThink=MAX_flt;
        B->Strikes.Empty();B->Elapsed=100.f;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;
    };
    Ready();const FVector Target(0,0,AWarfareFrontlineGround::HeightAt(0,0));
    const bool Ordered=RequestBomberWithMetricsV8220(B,Target,TEXT("production_checks"));
    UE_LOG(LogTemp,Display,TEXT("WF_V8220_BOMBER order=%d block=%s"),Ordered,*B->SupportBlockReason(EFrontlineSupport::AirBombing,0));
    auto& State=States.FindOrAdd(B);
    Check(TEXT("v8220_bomber_production_request_registers_three_real_four_bomb_aircraft"),Ordered&&State.Air.Num()==3&&B->Strikes.Num()==12);if(!Ordered||State.Air.Num()!=3||B->Strikes.Num()!=12)return;
    const auto Plan=B->Strikes;const auto Aircraft=State.Air;
    TSet<uint32> Keys;bool Assets=true,Origins=true,Spacing=true,Footprint=true,Flights=true;
    for(const auto& A:Aircraft)
    {
        auto* Plane=A.Aircraft.Get();auto* Mesh=Plane?Cast<UStaticMeshComponent>(Plane->GetRootComponent()):nullptr;
        Assets&=Mesh&&Mesh->IsRegistered()&&Mesh->GetStaticMesh()==FrontlineSupportArtV36::AircraftFor(B,0)&&A.RoundCount==4;
        if(Plane)Keys.Add(Plane->GetUniqueID());
    }
    for(int32 I=0;I<Plan.Num();++I)
    {
        const auto& Strike=Plan[I];const auto& A=Aircraft[I/4];B->Elapsed=Strike.LaunchedAt;Tick(B,.05f);
        const auto Flight=FrontlineSupportArtV8220::BomberFlight(Target,B->GetTeamDirection(0),Strike.LaunchedAt-A.Arrival);
        Origins&=A.Aircraft.IsValid()&&Strike.Origin.Equals(A.Aircraft->GetActorLocation(),.1f)&&Strike.Origin.Equals(Flight.Position,.1f)
            &&Strike.bAirDropped&&Strike.Radius==820.f&&Strike.ArmorPower==500.f&&Strike.ImpactAt>Strike.LaunchedAt&&Strike.FireMissionId==Aircraft[0].FireMission;
        if(I)Spacing&=FMath::IsNearlyEqual(Strike.LaunchedAt-Plan[I-1].LaunchedAt,.55f,.0001f);
        Footprint&=FMath::Abs(Strike.Target.X-Target.X)<=B->FireSupportScatterExtent(EFrontlineSupport::AirBombing).X&&FMath::Abs(Strike.Target.Y-Target.Y)<=770.f;
        Flights&=FMath::IsNearlyEqual(FMath::Abs(float(Flight.Velocity.X)),2300.f,.001f);
    }
    const auto Response=B->FireSupportDelayRange(EFrontlineSupport::AirBombing,Target,0);
    Check(TEXT("v8220_bomber_release_origins_follow_actual_three_aircraft_and_unique_audio_keys"),Assets&&Keys.Num()==3&&Origins);
    Check(TEXT("v8220_bomber_keeps_twelve_bombs_055_interval_first_impact_and_damage"),Spacing&&Plan[0].ImpactAt-100.f>=Response.X-.001f&&Plan[0].ImpactAt-100.f<=Response.Y+.001f&&Origins);
    Check(TEXT("v8220_bomber_actual_release_flight_is_2300cm_per_second_independent_of_interval"),Flights&&FrontlineSupportArtV8220::BomberSpeed>1250.f*.7f*2.f);
    Check(TEXT("v8220_bomber_actual_scatter_remains_inside_request_safety_footprint"),Footprint);
    bool NoUpward=true;
    for(const auto& Strike:Plan)
    {
        const float Duration=Strike.ImpactAt-Strike.LaunchedAt;
        const FVector InitialVelocity=(Strike.Target-Strike.Origin)/Duration+FVector(0,0,490.f*Duration);
        NoUpward&=FMath::Abs(InitialVelocity.Z)<.02f&&FVector::DotProduct(InitialVelocity,FVector(B->GetTeamDirection(0),0,0))>0;
        auto* Projectile=FrontlineSupportArtV36::SpawnShell(B,Strike);NoUpward&=Projectile&&Projectile->IsVisualReady();if(Projectile)Projectile->Destroy();
    }
    Check(TEXT("v8220_bomber_actual_airdrop_assets_use_gravity_without_upward_impulse"),NoUpward);
    // Real production arrival, real infantry and kill attribution; no forced hit
    // and no invented damage formula. A central physical bomb has near-one chance.
    const auto& First=Plan[0];auto* Victim=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    if(Victim){Victim->Initialize(B,822099,1,EFrontlineRole::Rifle,0);Victim->bFieldEntered=true;Victim->SetActorTickEnabled(false);Victim->SetActorLocation(First.Target+FVector(0,0,88));B->Units.Add(Victim);}
    B->Strikes=Plan;B->Elapsed=First.ImpactAt-.0001f;const int32 KillsBefore=B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::AirBombing)].Infantry;
    Resolve();const bool Early=Victim&&!Victim->IsAlive();
    B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;B->Elapsed=First.ImpactAt-.0001f;B->BattleOutroElapsed=.002f;Resolve();
    Check(TEXT("v8220_bomber_actual_first_arrival_remains_lethal_and_credited_during_ending"),Victim&&!Early&&!Victim->IsAlive()
        &&Victim->FatalSourceV65.Support==int32(EFrontlineSupport::AirBombing)&&Victim->FatalSourceV65.FireMissionId==First.FireMissionId
        &&B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::AirBombing)].Infantry==KillsBefore+1);
    Tick(B,.2f);bool EndingAudio=true;
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        for(const auto Key:Keys)EndingAudio&=Audio->DescribeAircraftAudioV8218(int32(Key))->GetBoolField(TEXT("playing"));
    else EndingAudio=false;
    Check(TEXT("v8220_bomber_actual_committed_flight_keeps_each_engine_audio_during_ending"),EndingAudio);
    B->Units.Empty();if(Victim)Victim->Destroy();
    B->BattleOutroElapsed=20.f;Resolve();
    Check(TEXT("v8220_bomber_empty_ground_committed_tail_resolves_without_freezing_bombs"),B->Strikes.IsEmpty());
    int32 Playing=0;if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        for(const auto& A:Aircraft)if(A.Aircraft.IsValid())Playing+=Audio->DescribeAircraftAudioV8218(int32(A.Aircraft->GetUniqueID()))->GetBoolField(TEXT("playing"))?1:0;
    Tick(B,.2f);bool Stopped=true;
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())for(const auto Key:Keys)Stopped&=!Audio->DescribeAircraftAudioV8218(int32(Key))->GetBoolField(TEXT("playing"));
    Check(TEXT("v8220_bomber_all_three_departure_audio_keys_are_stopped"),State.Air.IsEmpty()&&Stopped);
    UE_LOG(LogTemp,Display,TEXT("WF_V8220_BOMBER planes=3 bombs=%d release_interval=%.2f cruise_cm_s=%.1f first_effect=%.4f response=[%.4f,%.4f] audio_playing_before_departure=%d"),Plan.Num(),Plan[1].LaunchedAt-Plan[0].LaunchedAt,FrontlineSupportArtV8220::BomberSpeed,Plan[0].ImpactAt-100.f,Response.X,Response.Y,Playing);
    auto Metric=MakeShared<FJsonObject>();Metric->SetNumberField(TEXT("actual_aircraft"),Aircraft.Num());Metric->SetNumberField(TEXT("actual_bombs"),Plan.Num());
    Metric->SetNumberField(TEXT("release_interval_seconds"),Plan[1].LaunchedAt-Plan[0].LaunchedAt);Metric->SetNumberField(TEXT("cruise_cm_per_second"),FrontlineSupportArtV8220::BomberSpeed);
    Metric->SetNumberField(TEXT("first_effect_seconds"),Plan[0].ImpactAt-100.f);Metric->SetNumberField(TEXT("response_min_seconds"),Response.X);Metric->SetNumberField(TEXT("response_max_seconds"),Response.Y);
    Metric->SetBoolField(TEXT("release_origins_match_actual_aircraft"),Origins);Metric->SetBoolField(TEXT("released_bombs_have_no_upward_impulse"),NoUpward);
    Metric->SetBoolField(TEXT("ending_engine_audio_playing"),EndingAudio);Metric->SetBoolField(TEXT("departure_engine_audio_stopped"),Stopped);
    FrontlineAirSupportReviewV8220::AppendMetric(B,TEXT("bomber_outcomes"),Metric);
    Ready();bool InterceptionCase=false;
    const bool AARegistered=FrontlineFieldSitesV59::WithAirCoverReviewV8220(B,1,[&](const FVector& Gun)
    {
        const int32 Direction=B->GetTeamDirection(0);
        FVector Aim=Gun+FVector(Direction*4000.f,0,0);Aim.Z=AWarfareFrontlineGround::HeightAt(Aim.X,Aim.Y);
        if(FMath::Abs(Aim.X)>FrontlineBattlefield::SupportHalfLength||FMath::Abs(Aim.Y)>FrontlineBattlefield::HalfFrontage)return;
        if(!RequestBomberWithMetricsV8220(B,Aim,TEXT("actual_aa_interception_checks")))return;
        auto& AirState=States.FindOrAdd(B);if(AirState.Air.Num()!=3||B->Strikes.Num()!=12)return;
        const auto ActualPlan=B->Strikes;const auto Sorties=AirState.Air;
        for(int32 I=1;I<=30;++I){B->Elapsed=Sorties[0].Arrival+I*.05f;Tick(B,.05f);}
        int32 Driven=0,Cancelled=0;bool ReleasedRetained=true,OtherRetained=true;
        for(const auto& A:AirState.Air)if(A.DrivenOffAt>=0)++Driven;
        for(const auto& Strike:ActualPlan)
        {
            const bool Exists=B->Strikes.ContainsByPredicate([&](const FFrontlineStrike& S){return S.FireMissionId==Strike.FireMissionId&&S.RoundIndex==Strike.RoundIndex;});
            if(!Exists)++Cancelled;
            if(Strike.RoundIndex>=4)OtherRetained&=Exists;
            if(Strike.LaunchedAt<=Sorties[0].Arrival+1.1f+.0001f)ReleasedRetained&=Exists;
        }
        InterceptionCase=Driven==1&&Cancelled>0&&Cancelled<4&&ReleasedRetained&&OtherRetained;
        UE_LOG(LogTemp,Display,TEXT("WF_V8220_BOMBER_AA actual_sorties=%d driven_off=%d cancelled_unreleased=%d released_retained=%d other_aircraft_retained=%d"),AirState.Air.Num(),Driven,Cancelled,ReleasedRetained,OtherRetained);
        auto AAMetric=MakeShared<FJsonObject>();AAMetric->SetNumberField(TEXT("actual_sorties"),AirState.Air.Num());AAMetric->SetNumberField(TEXT("driven_off"),Driven);
        AAMetric->SetNumberField(TEXT("cancelled_unreleased"),Cancelled);AAMetric->SetBoolField(TEXT("released_retained"),ReleasedRetained);AAMetric->SetBoolField(TEXT("other_aircraft_retained"),OtherRetained);
        FrontlineAirSupportReviewV8220::AppendMetric(B,TEXT("bomber_aa_outcomes"),AAMetric);
    });
    Check(TEXT("v8220_bomber_actual_registered_aa_cancels_only_one_aircraft_unreleased_bombs"),AARegistered&&InterceptionCase);
}
bool SetupAirBombingCaptureV8220(AWarfareFrontlineGameMode* B)
{
    FString Dir;if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir))return false;
    const FName Tag(TEXT("V8220AirCapture"));for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(I->ActorHasTag(Tag))I->Destroy();
    const auto Before=States.FindOrAdd(B);const auto Strikes=B->Strikes;const auto Units=B->Units;
    const auto Phase=B->Phase;const float Clock=B->Elapsed;const FString Message=B->LastMessage;
    ON_SCOPE_EXIT{States.FindOrAdd(B)=Before;B->Strikes=Strikes;B->Units=Units;B->Phase=Phase;B->Elapsed=Clock;B->LastMessage=Message;};
    auto& State=States.FindOrAdd(B);State=FState();State.Encounter=B->GetEncounterId();State.EnemyThink=MAX_flt;
    B->Strikes.Empty();B->Units.Empty();B->Phase=EFrontlinePhase::Fighting;
    const FVector Target(2600,0,AWarfareFrontlineGround::HeightAt(2600,0));if(!RequestBomberWithMetricsV8220(B,Target,TEXT("capture")))return false;
    const float Time=State.Air[0].Arrival+1.925f;bool Ready=true;
    for(auto& A:State.Air)if(auto* Plane=A.Aircraft.Get())
    {
        Plane->Tags.Add(Tag);Plane->SetActorHiddenInGame(false);Plane->SetActorTickEnabled(false);Plane->SetLifeSpan(20.f);
        const auto Flight=FrontlineSupportArtV8220::BomberFlight(A.Target,B->GetTeamDirection(0),Time-A.Arrival);
        Plane->SetActorTransform(FTransform(Flight.Rotation,Flight.Position));
    }
    else Ready=false;
    for(const auto& Strike:B->Strikes)if(Strike.LaunchedAt<=Time&&Time<Strike.ImpactAt)
    {
        auto* Bomb=FrontlineSupportArtV36::SpawnShell(B,Strike);Ready&=Bomb!=nullptr;
        if(Bomb){Bomb->Tags.Add(Tag);Bomb->SetActorTickEnabled(false);Bomb->UpdateVisual(Time);Bomb->SetLifeSpan(20.f);}
    }
    auto Metric=MakeShared<FJsonObject>();Metric->SetBoolField(TEXT("prepared"),Ready&&State.Air.Num()==3);Metric->SetNumberField(TEXT("actual_aircraft"),State.Air.Num());Metric->SetNumberField(TEXT("actual_strikes"),B->Strikes.Num());
    FrontlineAirSupportReviewV8220::AppendMetric(B,TEXT("bomber_captures"),Metric);return Ready&&State.Air.Num()==3;
}
