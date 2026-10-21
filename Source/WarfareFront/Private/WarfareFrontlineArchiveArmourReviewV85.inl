// Queue acceptance uses the real Archive roster, Tick and Recruit. Retired
// fixture actors are removed after each receipt; this is not combat acceptance.
TSharedRef<FJsonObject> FFrontlineArmourApproachReviewV85::RunArchiveAllocationV85(UFrontlineVehicleSubsystem* Self)
{
 auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
 const auto Check=[&](const FString& Name,bool Pass){auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));Good&=Pass;};
 auto* B=Self?Self->Battle():nullptr;
 const bool Safe=B&&FrontlineVisualV85::Allowed(B)&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
  &&Self->Vehicles.IsEmpty()&&B->Units.IsEmpty();
 Check(TEXT("v85_archive_queue_exact_disposable_empty_fixture"),Safe);
 if(Safe)
 {
  const bool Paused=UGameplayStatics::IsGamePaused(B->GetWorld());UGameplayStatics::SetGamePaused(B,false);
  ON_SCOPE_EXIT{FrontlineUltimateArchiveV8215::Leave(B);Self->ClearEncounter();UGameplayStatics::SetGamePaused(B,Paused);};
  for(int32 Nation=0;Nation<2;++Nation)for(int32 Difficulty=0;Difficulty<3;++Difficulty)
  {
   B->Phase=EFrontlinePhase::Briefing;
   Check(TEXT("v85_archive_queue_public_begin"),FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction(Nation),0,EFrontlineDifficulty(Difficulty)));
   B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;Self->ClearEncounter();Self->LastEncounter=B->GetEncounterId();
   const auto R=Self->GetEquippedRoster(1);Check(TEXT("v85_archive_opponent_full_roster"),R.Num()>2);
   int32 Cannon=0,Support=0;TSet<int32> Models;TArray<TSharedPtr<FJsonValue>> Waves;
   for(int32 Wave=0;Wave<9;++Wave)
   {
    const int32 Before=Self->DeploymentCountV37[1];int32 Attempts=0;
    while(Self->DeploymentCountV37[1]==Before&&Attempts++<4)
    {B->Elapsed=FMath::Max(B->Elapsed,Self->NextEnemyAt)+.001f;Self->Tick(.001f);}
    const bool Spawned=Self->DeploymentCountV37[1]==Before+1&&Self->Vehicles.Num()==1;
    Check(FString::Printf(TEXT("v85_archive_enemy_%d_diff%d_wave%d_actual_recruit"),Nation,Difficulty,Wave),Spawned);
    if(!Spawned)break;
    AFrontlineTank* V=Self->Vehicles[0].Get();const auto Type=V->Type;const auto& D=Self->Definition(Type);Models.Add(int32(Type));
    if(D.CannonCount>0)++Cannon;else ++Support;
    auto W=MakeShared<FJsonObject>();W->SetNumberField(TEXT("type"),int32(Type));W->SetNumberField(TEXT("host_clock"),B->Elapsed);
    W->SetNumberField(TEXT("next_decision"),Self->NextEnemyAt);W->SetNumberField(TEXT("preparation_seconds"),Self->ArmourCycleDurationV84413[1][int32(Type)]);
    Waves.Add(MakeShared<FJsonValueObject>(W));
    const int32 Next=ArchiveArmourReserveV85(R,Self->DeploymentCountV37[1],false,[&](EFrontlineVehicle Candidate){return Self->IsDeploymentEligibleV837(Candidate,1);});
    Check(TEXT("v85_archive_waits_for_reserved_model_clock"),R.IsValidIndex(Next)&&FMath::IsNearlyEqual(Self->NextEnemyAt,FMath::Max(B->Elapsed+.5f,Self->ReadyAt[1][int32(R[Next].Type)]),.01f));
    float Earliest=MAX_flt;for(const auto& Entry:R)Earliest=FMath::Min(Earliest,Self->ReadyAt[1][int32(Entry.Type)]);
    // A short preparation finishing early cannot restart the heavy queue.
    if(Earliest+.02f<Self->NextEnemyAt){B->Elapsed=Earliest+.01f;Self->Tick(.001f);Check(TEXT("v85_archive_short_cycle_cannot_starve_reservation"),Self->DeploymentCountV37[1]==Before+1);}
    V->Destroy();Self->Vehicles.Reset();
   }
   Check(FString::Printf(TEXT("v85_archive_%d_diff%d_actual_two_cannon_one_support_variety"),Nation,Difficulty),Cannon==6&&Support==3&&Models.Num()>=4);
   for(int32 Wave=0;Wave<9;++Wave){const int32 Pick=ArchiveArmourReserveV85(R,Wave,true,[&](EFrontlineVehicle Type){return Self->IsDeploymentEligibleV837(Type,1);});Check(TEXT("v85_archive_seen_armour_commits_legal_cannon"),R.IsValidIndex(Pick)&&R[Pick].CannonCount>0);}
   auto C=MakeShared<FJsonObject>();C->SetNumberField(TEXT("player_faction"),Nation);C->SetNumberField(TEXT("difficulty"),Difficulty);C->SetNumberField(TEXT("cannon_deployments"),Cannon);C->SetNumberField(TEXT("support_deployments"),Support);C->SetNumberField(TEXT("distinct_models"),Models.Num());C->SetArrayField(TEXT("waves"),Waves);Cases.Add(MakeShared<FJsonValueObject>(C));
   FrontlineUltimateArchiveV8215::Leave(B);
  }
 }
 J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("cases"),Cases);
 J->SetStringField(TEXT("scope"),TEXT("Actual UK/DE opponent Archive roster, production Tick/Recruit and untouched per-model preparation clocks, all three difficulties, nine deployments each. Empty-field queue fixture retires recruited actors after receipts. This does not claim battle win rates or FR/RU campaign acceptance."));return J;
}
TSharedRef<FJsonObject> RunFrontlineArchiveArmourV85Review(UFrontlineVehicleSubsystem* Fleet)
{return FFrontlineArmourApproachReviewV85::RunArchiveAllocationV85(Fleet);}
