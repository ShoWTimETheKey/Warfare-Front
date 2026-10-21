// Member of the existing private review friend. Public orders pay the real
// shared queue; no friendly actor is teleported into a firing position.
static TSharedRef<FJsonObject> RunNo44RotationV8310(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 auto Result=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;
 const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
 const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&!WarfareFRRUCampaignV82::Active(B)
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV8310UKRifleGrenades"));
 Assert(TEXT("v8310_no44_rotation_exact_disposable_ukde_guard"),Safe);
 auto* Fleet=Safe?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;
 if(!Fleet){Result->SetBoolField(TEXT("success"),false);return Result;}
 const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;const auto Vehicles=Fleet->Vehicles;
 const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;const auto Active=B->ActiveCampaignId;
 const auto Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId,Encounter=B->EncounterId;
 const int32 BoundMission=B->BoundCampaignMission,NextId=B->NextSquadId,Serial=Fleet->NextSerial;
 const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;const auto Phase=B->Phase;const auto Random=B->Random;
 const float Clock=B->Elapsed,Peak=B->PeakSuppression,Friendly=B->FriendlyFireDamage;
 const int32 Winner=B->BattleOutroWinner,Shots=B->ShotsFired,Thrown=B->GrenadesThrown,Recruited=B->RecruitCommands,Advanced=B->AdvanceCommands,Captured=B->TrenchCaptures;
 const float Outro=B->BattleOutroElapsed,EnemyRecruit=B->EnemyRecruitAt,CounterUntil=B->EnemyCounterattackUntil,WaveAt=B->EnemyWaveStartedAt;
 const int32 Counter=B->EnemyCounterattackTrench,WaveCount=B->EnemyWaveRecruitCount;const bool CommittedRole=B->bEnemyRoleCommitted;
 const bool Rewarded=B->bEncounterRewarded,AdvancedCampaign=B->bCampaignMissionAdvanced;
 const int32 Reward=B->LastResearchReward,DisplayedReward=B->ResearchBeforeOutcomeV828;const auto RewardIds=B->RewardedEncounterIds;
 const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage,Reason=B->EndReason,Failure=B->AssetFailure;
 const int32 Casualties[]={B->Casualties[0],B->Casualties[1]};const float Morale[]={B->Morale[0],B->Morale[1]},Damage[]={B->DamageDealt[0],B->DamageDealt[1]};
 float Ready[2][AWarfareFrontlineGameMode::RoleCount];for(int32 T=0;T<2;++T)for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)Ready[T][I]=B->RecruitReadyAt[T][I];
 TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
 struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
 const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
 for(AWarfareFrontlineSoldier* U:Units)Hide(U);for(AFrontlineTank* V:Vehicles)Hide(V);
 const auto Clear=[&]()
 {
  TArray<AActor*> Created;for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))Created.Add(*It);
  for(auto* A:Created)if(IsValid(A))A->Destroy();
  B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();Fleet->Vehicles.Reset();
  FrontlineSpecialistsV41::Reset(B);FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
 };
 ON_SCOPE_EXIT
 {
  Clear();B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;Fleet->Vehicles=Vehicles;Fleet->NextSerial=Serial;
  B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->EncounterId=Encounter;B->BoundCampaignMission=BoundMission;
  B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;B->Phase=Phase;B->Random=Random;B->Elapsed=Clock;B->PeakSuppression=Peak;B->FriendlyFireDamage=Friendly;
  B->BattleOutroWinner=Winner;B->BattleOutroElapsed=Outro;B->ShotsFired=Shots;B->GrenadesThrown=Thrown;B->RecruitCommands=Recruited;B->AdvanceCommands=Advanced;B->TrenchCaptures=Captured;B->NextSquadId=NextId;
  B->EnemyRecruitAt=EnemyRecruit;B->EnemyCounterattackUntil=CounterUntil;B->EnemyCounterattackTrench=Counter;B->EnemyWaveStartedAt=WaveAt;B->EnemyWaveRecruitCount=WaveCount;B->bEnemyRoleCommitted=CommittedRole;
  B->bEncounterRewarded=Rewarded;B->bCampaignMissionAdvanced=AdvancedCampaign;B->LastResearchReward=Reward;B->ResearchBeforeOutcomeV828=DisplayedReward;B->RewardedEncounterIds=RewardIds;
  B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;B->EndReason=Reason;B->AssetFailure=Failure;
  for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)B->RecruitReadyAt[T][I]=Ready[T][I];}
  for(const auto& H:Hidden)if(H.Actor.IsValid()){H.Actor->SetActorHiddenInGame(H.Hidden);H.Actor->SetActorEnableCollision(H.Collision);}
  Result->SetArrayField(TEXT("cases"),Rows);Result->SetBoolField(TEXT("success"),Good);
 };
 FrontlineAchievementsV76::WithScopedCombatEventsV839(B,[&]
 {FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]
 {FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]
 {
  for(int32 Seed=0;Seed<3;++Seed)
  {
   Clear();B->Trenches=Trenches;B->PlayerFaction=EFrontlineFaction::British;
   FFrontlineCampaignProgress Progress;Progress.Id=FGuid::NewGuid();Progress.Faction=EFrontlineFaction::British;Progress.Difficulty=EFrontlineDifficulty::Veteran;
   Progress.SelectedMission=Progress.CompletedMissions=19;Progress.StrategicRevisionV52=52;
   Progress.BypassedOperationMaskV52=(1u<<3)|(1u<<7)|(1u<<13)|(1u<<17);Progress.WonOperationMaskV52=((1u<<19)-1u)&~Progress.BypassedOperationMaskV52;
   Progress.DispatchResolvedMaskV52=(1u<<20)-1u;Progress.RoleSelectionMask=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::RifleGrenadier));
   Progress.SupportSelectionMask=0;Progress.ArmourSelectionMaskV63=0;Progress.Research.TechnologyMask=0;
   B->CampaignRuns={Progress};B->ActiveCampaignId=B->BoundCampaignId=Progress.Id;B->EncounterId=FGuid::NewGuid();B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=19;
   B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;B->Elapsed=0;B->NextSquadId=83103100+Seed*100;Fleet->NextSerial=8310310+Seed*10;
   B->Random.Initialize(831031+Seed*1549);B->PeakSuppression=B->FriendlyFireDamage=0;B->ShotsFired=B->GrenadesThrown=B->RecruitCommands=B->AdvanceCommands=B->TrenchCaptures=0;
   for(int32 T=0;T<2;++T){B->CombatRecordV65[T]={};B->Casualties[T]=0;B->DamageDealt[T]=0;B->Morale[T]=100;for(float& At:B->RecruitReadyAt[T])At=0;}
   // Fresh encounter readiness is zero once. Nothing clears or shortens it
   // after the first accepted command. The hostile tank enters through the
   // genuine vehicle entry, with its configured full hull and both tracks.
   auto* Tank=Fleet->Spawn(EFrontlineVehicle::A7V,1,true);
   const bool Full=Tank&&Tank->AreAssetsReady()&&Tank->IsOperational()&&Tank->HullIntegrity==Tank->MaximumHullIntegrity&&Tank->TrackIntegrity[0]>0&&Tank->TrackIntegrity[1]>0;
   const FString Prefix=FString::Printf(TEXT("v8310_no44_rotation_seed%d_"),Seed);Assert(Prefix+TEXT("full_mobile_hostile_heavy_armour"),Full);if(!Full)continue;
   Tank->Random.Initialize(831039+Seed*1553);const float Hull=Tank->HullIntegrity,Left=Tank->TrackIntegrity[0],Right=Tank->TrackIntegrity[1];
   TArray<TSharedPtr<FJsonValue>> Orders,Samples;TSet<int32> OwnedSections;int32 RifleSections=0,BombSections=0;float NextAllowed=0,LastAccepted=-1,ElapsedToKill=-1;
   bool QueueValid=true,Finite=true,BodiesClear=true;float FirstEntry=-1,FirstArmourShot=-1;constexpr float DT=.05f,Limit=120.f;
   const auto Pose=[](AWarfareFrontlineSoldier* U,float Delta)
   {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(Delta,false);U->Soldier->RefreshBoneTransforms();if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(Delta,LEVELTICK_All,nullptr);}};
   for(int32 Frame=0;Frame<2400&&Tank->IsOperational()&&B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE&&B->Morale[0]>0;++Frame)
   {
    B->Elapsed=(Frame+1)*DT;
    if(RifleSections<1||BombSections<6)
    {
     const auto Role=RifleSections<1?EFrontlineRole::Rifle:EFrontlineRole::RifleGrenadier;
     const float Cooldown=B->RecruitCooldown(Role,0);const FString Block=B->RecruitBlockReason(Role,0);
     if(Block.IsEmpty()&&B->Recruit(Role,0))
     {
      QueueValid&=Cooldown<=.01f&&B->Elapsed+.01f>=NextAllowed;
      const float Paid=LastAccepted<0?0:B->Elapsed-LastAccepted;LastAccepted=B->Elapsed;
      auto& Q=B->Squads.Last();OwnedSections.Add(Q.Id);RifleSections+=Role==EFrontlineRole::Rifle;BombSections+=Role==EFrontlineRole::RifleGrenadier;
      for(const auto& W:Q.Members)if(auto* U=W.Get()){U->SetActorTickEnabled(false);U->MotionRandom.Initialize(831031+Seed*1549+Q.Id*31+U->MemberIndex*101);}
      NextAllowed=B->RecruitReadyAt[0][int32(EFrontlineRole::RifleGrenadier)];
      auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("host_seconds"),B->Elapsed);O->SetNumberField(TEXT("role"),int32(Role));O->SetNumberField(TEXT("squad"),Q.Id);O->SetNumberField(TEXT("cooldown_before_command"),Cooldown);
      O->SetNumberField(TEXT("actual_seconds_since_previous_dispatch"),Paid);O->SetNumberField(TEXT("next_rifle_grenade_ready_at"),NextAllowed);O->SetNumberField(TEXT("original_members"),Q.Members.Num());Orders.Add(MakeShared<FJsonValueObject>(O));
     }
    }
    for(const auto& Q:B->Squads)if(Q.Team==0&&OwnedSections.Contains(Q.Id)&&Q.LivingCount()>0&&!Q.bAdvancing&&B->Trenches.IsValidIndex(Q.Trench))B->AdvanceSquad(Q.Trench,Q.Id,0);
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))
    {
     U->Tick(DT);Pose(U,DT);Finite&=!U->Feet().ContainsNaN();
     if(U->IsActiveCombatant()&&U->bFieldEntered){if(FirstEntry<0)FirstEntry=B->Elapsed;BodiesClear&=!Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation(),Tank->HeadingYawV37);}
     if(U->Role==EFrontlineRole::RifleGrenadier&&U->MemberIndex==0&&U->ShotsDischarged>0&&FirstArmourShot<0)FirstArmourShot=B->Elapsed;
    }
    B->TickCommittedSupportV8217();FrontlineSpecialistsV41::Tick(B);Tank->Simulate(DT);B->UpdateSquads();FrontlineMoraleV36::Tick(B,DT);
    if(Frame%100==0)
    {
     auto S=MakeShared<FJsonObject>();S->SetNumberField(TEXT("host_seconds"),B->Elapsed);S->SetNumberField(TEXT("hull"),Tank->HullIntegrity);S->SetNumberField(TEXT("left_track"),Tank->TrackIntegrity[0]);S->SetNumberField(TEXT("right_track"),Tank->TrackIntegrity[1]);
     S->SetNumberField(TEXT("active_infantry"),B->LivingSoldiers(0));S->SetNumberField(TEXT("rifle_grenade_sections_dispatched"),BombSections);S->SetNumberField(TEXT("morale"),B->Morale[0]);S->SetNumberField(TEXT("tank_mg_shots"),Tank->MachinegunShots);Samples.Add(MakeShared<FJsonValueObject>(S));
    }
    if(!Tank->IsOperational()){ElapsedToKill=B->Elapsed;break;}
    const bool PendingBomb=B->Strikes.ContainsByPredicate([](const FFrontlineStrike& S){return S.Team==0&&S.bGrenade&&!S.bResolved;});
    if(BombSections==6&&B->LivingSoldiers(0)==0&&!PendingBomb)break;
   }
   const auto Proven=FrontlineSpecialistsV41::ReadArmourRoundsV833(B);const auto InfantryCredit=B->CombatRecordV65[0].InfantrySources[int32(EFrontlineRole::RifleGrenadier)].Armour;
   const bool Kill=!Tank->IsOperational()&&InfantryCredit>=1&&FMath::IsNearlyEqual(Proven.GrenadeHullDamage,Hull,.02f);
   Assert(Prefix+TEXT("every_actual_recruit_paid_the_shared_queue"),QueueValid&&RifleSections==1&&BombSections>=2&&BombSections<=6);
   Assert(Prefix+TEXT("actual_edge_entry_and_animated_rifle_bomb_discharge"),FirstEntry>=0&&FirstArmourShot>=FirstEntry&&B->GrenadesThrown>=4);
   Assert(Prefix+TEXT("actual_infantry_only_full_hull_kill_is_not_a_tail_credit"),Kill);
   Assert(Prefix+TEXT("finite_real_footing_and_no_live_hull_overlap"),Finite&&BodiesClear);
   auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("seed"),Seed);Row->SetBoolField(TEXT("success"),QueueValid&&Kill&&Finite&&BodiesClear);
   Row->SetNumberField(TEXT("initial_full_hull"),Hull);Row->SetNumberField(TEXT("initial_left_track"),Left);Row->SetNumberField(TEXT("initial_right_track"),Right);Row->SetNumberField(TEXT("final_hull"),Tank->HullIntegrity);
   Row->SetNumberField(TEXT("elapsed_seconds"),B->Elapsed);Row->SetNumberField(TEXT("kill_seconds"),ElapsedToKill);Row->SetNumberField(TEXT("first_real_field_entry_seconds"),FirstEntry);Row->SetNumberField(TEXT("first_real_rifle_bomb_discharge_seconds"),FirstArmourShot);
   Row->SetNumberField(TEXT("rifle_cover_sections"),RifleSections);Row->SetNumberField(TEXT("rifle_grenade_sections"),BombSections);Row->SetNumberField(TEXT("actual_rifle_bombs_released"),B->GrenadesThrown);
   Row->SetNumberField(TEXT("actual_no44_hull_damage"),Proven.GrenadeHullDamage);Row->SetNumberField(TEXT("actual_no44_track_damage"),Proven.GrenadeTrackDamage);Row->SetNumberField(TEXT("actual_no44_armour_kill_credit"),InfantryCredit);
   Row->SetNumberField(TEXT("infantry_losses"),B->CombatRecordV65[0].InfantryLost);Row->SetNumberField(TEXT("surviving_infantry"),B->LivingSoldiers(0));Row->SetNumberField(TEXT("player_morale"),B->Morale[0]);Row->SetNumberField(TEXT("tank_machinegun_shots"),Tank->MachinegunShots);Row->SetNumberField(TEXT("tank_cannon_shots"),Tank->CannonShots);
   Row->SetArrayField(TEXT("accepted_public_recruit_orders"),Orders);Row->SetArrayField(TEXT("five_second_samples"),Samples);Rows.Add(MakeShared<FJsonValueObject>(Row));
  }
 });});});
 Result->SetStringField(TEXT("scope"),TEXT("Three seeded controlled heavy-armour approaches on the already prepared authored terrain. Fresh late British allocation, Veteran, no purchased infantry technology, one full configured mobile A7V entering by its genuine edge route. A public command policy pays the ordinary shared queue for one rifle cover section and at most six No.44 sections, with actual edge ingress and public advances. Ordinary soldier/animation, committed ballistic impacts, hostile tank movement/fire, squad arrivals and morale run in a120s actor timeline; an exhausted six-section force with no living man or pending bomb fails promptly. No friendly armour, support, invulnerability, forced hits, teleports into combat, hull/track subtraction or mid-battle cooldown reset. A pass requires all original hostile hull loss credited to infantry bombs and the real infantry armour kill ledger. This is not a full enemy army encounter, natural campaign win, enjoyment or cross-national equality proof."));
 return Result;
}
