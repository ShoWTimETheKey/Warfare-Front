// Included by VehiclesV28: real recruitment and the ordinary profile codec,
// confined to the exact V84413 disposable account. No synthetic cooldown model.
void FrontlineRunArchiveAllocationChecksV84413(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
struct FFrontlineReadinessChecksV84413
{
 static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
 {
  auto Report=MakeShared<FJsonObject>();FString Dir;
  const bool Safe=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
   &&FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))
   &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&!FrontlineUltimateArchiveV8215::Active(B);
  Check(TEXT("v84413_readiness_exact_no_save_account_and_nonarchive_boundary"),Safe);if(!Safe)return Report;
  auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v84413_readiness_real_fleet_present"),Fleet!=nullptr);if(!Fleet)return Report;
  const auto Runs=B->CampaignRuns;const auto Army0=B->ArmyProgress[0],Army1=B->ArmyProgress[1];const auto Receipts=B->RewardedEncounterIds;
  const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId,Encounter=B->EncounterId;
  const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;const auto Units=B->Units;const auto Squads=B->Squads;const auto Random=B->Random;
  const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53,NextSquad=B->NextSquadId,Outro=B->BattleOutroWinner;
  const bool Volatile=B->bProgressionVolatile,ReadOnly=B->bProgressionReadOnly,Advanced=B->bCampaignMissionAdvanced,Rewarded=B->bEncounterRewarded,Paused=UGameplayStatics::IsGamePaused(B);
  const FString File=B->ProgressionFile,Status=B->ProgressionSaveStatus,Message=B->LastMessage,Failure=B->AssetFailure;const float Elapsed=B->Elapsed;
  const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];
  const auto OriginalVehicles=Fleet->Vehicles;const auto FleetEncounter=Fleet->LastEncounter;const int32 Serial=Fleet->NextSerial;
  const float EnemyAt=Fleet->NextEnemyAt;float Ready[2][int32(EFrontlineVehicle::Count)],Durations[2][int32(EFrontlineVehicle::Count)];
  FMemory::Memcpy(Ready,Fleet->ReadyAt,sizeof(Ready));FMemory::Memcpy(Durations,Fleet->ArmourCycleDurationV84413,sizeof(Durations));
  const float Cycles[2]={Fleet->ArmourCycleSecondsV40[0],Fleet->ArmourCycleSecondsV40[1]},Rails[2]={Fleet->RailReadyAtV75[0],Fleet->RailReadyAtV75[1]};
  const int32 Deployments[2]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
  TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
  ON_SCOPE_EXIT
  {
   for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
   B->CampaignRuns=Runs;B->ArmyProgress[0]=Army0;B->ArmyProgress[1]=Army1;B->RewardedEncounterIds=Receipts;
   B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;B->EncounterId=Encounter;
   B->PlayerFaction=Faction;B->Phase=Phase;B->Units=Units;B->Squads=Squads;B->Random=Random;B->NextSquadId=NextSquad;
   B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->BattleOutroWinner=Outro;B->Elapsed=Elapsed;
   B->bProgressionVolatile=Volatile;B->bProgressionReadOnly=ReadOnly;B->bCampaignMissionAdvanced=Advanced;B->bEncounterRewarded=Rewarded;
   B->ProgressionFile=File;B->ProgressionSaveStatus=Status;B->LastMessage=Message;B->AssetFailure=Failure;
   B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;UGameplayStatics::SetGamePaused(B,Paused);
   Fleet->Vehicles=OriginalVehicles;Fleet->LastEncounter=FleetEncounter;Fleet->NextSerial=Serial;Fleet->NextEnemyAt=EnemyAt;
   FMemory::Memcpy(Fleet->ReadyAt,Ready,sizeof(Ready));FMemory::Memcpy(Fleet->ArmourCycleDurationV84413,Durations,sizeof(Durations));
   for(int32 T=0;T<2;++T){Fleet->ArmourCycleSecondsV40[T]=Cycles[T];Fleet->RailReadyAtV75[T]=Rails[T];Fleet->DeploymentCountV37[T]=Deployments[T];}
  };
  B->Units.Empty();B->Squads.Empty();Fleet->Vehicles.Empty();B->bProgressionReadOnly=false;B->BattleOutroWinner=INDEX_NONE;
  UGameplayStatics::SetGamePaused(B,false);TArray<TSharedPtr<FJsonValue>> Actual;
  auto Setup=[&](EFrontlineFaction Nation,EFrontlineDifficulty Difficulty)
  {
   for(auto V:Fleet->Vehicles)if(IsValid(V))V->Destroy();Fleet->Vehicles.Empty();
   FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=Nation;Run.Difficulty=Difficulty;
   Run.SelectedMission=Run.CompletedMissions=19;Run.ArmourResearchMaskV63=65535;Run.Research.ResearchPoints=20;
   Run.CreatedUtc=Run.UpdatedUtc=FDateTime::UtcNow().ToIso8601();
   B->CampaignRuns={Run};B->ActiveCampaignId=B->BoundCampaignId=Run.Id;B->PlayerFaction=Nation;
   B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=19;
   B->Phase=EFrontlinePhase::Fighting;B->Elapsed=1000;Fleet->LastEncounter=B->GetEncounterId();Fleet->NextSerial=8441300;
   Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();
   Fleet->ResetReadinessForDebug(0);Fleet->ResetReadinessForDebug(1);
  };
  for(int32 Nation=0;Nation<2;++Nation)for(int32 Difficulty=0;Difficulty<3;++Difficulty)
  {
   Setup(EFrontlineFaction(Nation),EFrontlineDifficulty(Difficulty));auto& Run=B->CampaignRuns[0];
   const auto Fast=Nation==0?EFrontlineVehicle::Austin1918:EFrontlineVehicle::BeuteMarkIVFemale;
   const auto Slow=Nation==0?EFrontlineVehicle::MarkVStar:EFrontlineVehicle::A7V;
   Run.ArmourSelectionMaskV63=(1u<<int32(Fast))|(1u<<int32(Slow));Run.bArmourSelectionExplicitV84413=true;
   const FString Prefix=FString::Printf(TEXT("v84413_readiness_n%d_d%d_"),Nation,Difficulty);
   Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();
   const float InitialFast=Fleet->ReadyAt[0][int32(Fast)],InitialSlow=Fleet->ReadyAt[0][int32(Slow)];
   B->Elapsed=InitialFast*.5f;
   Check(Prefix+TEXT("actual_initial_workshop_setup_locks_model_specific_ui_periods"),InitialFast<InitialSlow&&FMath::IsNearlyEqual(Fleet->CooldownFraction(Fast),.5f,.001f)&&Fleet->CooldownFraction(Slow)>.5f);
   Fleet->ResetReadinessForDebug(0);B->Elapsed=1000;
   const bool Dispatched=Fleet->Recruit(Slow,0);Check(Prefix+TEXT("actual_slow_vehicle_recruit_succeeds"),Dispatched);if(!Dispatched)continue;
   const float Start=B->Elapsed,FastDuration=Fleet->ArmourCycleDurationV84413[0][int32(Fast)],SlowDuration=Fleet->ArmourCycleDurationV84413[0][int32(Slow)];
   const auto Expected=[&](EFrontlineVehicle Type){return FrontlineGameplayV43::PreparationSeconds(Fleet->Definition(Type).Cooldown*FrontlinePacingV24::TempoIntervalScale*FrontlineArmourV63::ReadinessScale(B,Type,0),0,B->GetCampaignDifficulty());};
   bool EveryModel=true;for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)EveryModel&=FMath::IsNearlyEqual(Fleet->ReadyAt[0][I],Start+Expected(EFrontlineVehicle(I)),.001f)
     &&FMath::IsNearlyEqual(Fleet->ArmourCycleDurationV84413[0][I],Expected(EFrontlineVehicle(I)),.001f);
   Check(Prefix+TEXT("all_model_deadlines_derive_from_same_dispatch_with_actual_model_and_research"),EveryModel&&FastDuration<SlowDuration);
   const int32 Before=Fleet->Vehicles.Num();Check(Prefix+TEXT("second_card_cannot_double_dispatch_at_start"),!Fleet->Recruit(Fast,0)&&Fleet->Vehicles.Num()==Before);
   B->Elapsed=Start+FastDuration*.5f;
   Check(Prefix+TEXT("authoritative_ui_fraction_differs_halfway_through_fast_model"),FMath::IsNearlyEqual(Fleet->CooldownFraction(Fast),.5f,.001f)&&Fleet->CooldownFraction(Slow)>.5f);
   float FrozenReady[int32(EFrontlineVehicle::Count)],FrozenDuration[int32(EFrontlineVehicle::Count)];
   FMemory::Memcpy(FrozenReady,Fleet->ReadyAt[0],sizeof(FrozenReady));FMemory::Memcpy(FrozenDuration,Fleet->ArmourCycleDurationV84413[0],sizeof(FrozenDuration));
   Run.ArmourResearchMaskV63=0;
   Check(Prefix+TEXT("research_change_does_not_rewrite_current_deadlines_or_ui_denominator"),FMemory::Memcmp(FrozenReady,Fleet->ReadyAt[0],sizeof(FrozenReady))==0
     &&FMemory::Memcmp(FrozenDuration,Fleet->ArmourCycleDurationV84413[0],sizeof(FrozenDuration))==0&&FMath::IsNearlyEqual(Fleet->CooldownFraction(Fast),.5f,.001f));
   Run.ArmourResearchMaskV63=65535;B->Elapsed=Start+FastDuration+.001f;
   Check(Prefix+TEXT("fast_model_really_ready_while_heavy_still_blocked"),Fleet->RecruitBlockReason(Fast,0).IsEmpty()&&!Fleet->RecruitBlockReason(Slow,0).IsEmpty()
     &&Fleet->CooldownFraction(Fast)==0&&Fleet->CooldownFraction(Slow)>0);
   const float Restart=B->Elapsed;const bool FastDispatched=Fleet->Recruit(Fast,0);
   Check(Prefix+TEXT("actual_fast_dispatch_restarts_both_model_cycles_with_own_periods"),FastDispatched
     &&FMath::IsNearlyEqual(Fleet->ReadyAt[0][int32(Fast)],Restart+FastDuration,.001f)
     &&FMath::IsNearlyEqual(Fleet->ReadyAt[0][int32(Slow)],Restart+SlowDuration,.001f));
   const auto EnemyRoster=Fleet->GetEquippedRoster(1);const bool EnemyRecruited=!EnemyRoster.IsEmpty()&&Fleet->Recruit(EnemyRoster[0].Type,1);
   bool EnemyPeriods=EnemyRecruited;
   for(const auto& D:EnemyRoster)
   {
    const float Duration=FrontlineGameplayV43::PreparationSeconds(D.Cooldown*FrontlinePacingV24::TempoIntervalScale*FrontlineArmourV63::ReadinessScale(B,D.Type,1),1,B->GetCampaignDifficulty());
    EnemyPeriods&=FMath::IsNearlyEqual(Fleet->ReadyAt[1][int32(D.Type)],B->Elapsed+Duration,.001f)&&FMath::IsNearlyEqual(Fleet->ArmourCycleDurationV84413[1][int32(D.Type)],Duration,.001f);
   }
   Check(Prefix+TEXT("opponent_real_recruit_uses_its_own_faction_and_model_readiness"),EnemyPeriods);
   auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("nation"),Nation);Row->SetNumberField(TEXT("difficulty"),Difficulty);
   Row->SetNumberField(TEXT("fast_seconds"),FastDuration);Row->SetNumberField(TEXT("heavy_seconds"),SlowDuration);Row->SetBoolField(TEXT("real_dispatches"),Dispatched&&FastDispatched);Actual.Add(MakeShared<FJsonValueObject>(Row));
  }
  Setup(EFrontlineFaction::British,EFrontlineDifficulty::Rookie);B->Phase=EFrontlinePhase::Briefing;auto& Run=B->CampaignRuns[0];
  Run.ArmourResearchMaskV63=0;
  Check(TEXT("v84413_unresearched_close_gun_accuracy_remains_one"),FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1200)==1.f);
  Run.ArmourResearchMaskV63=65535;
  Check(TEXT("v84413_researched_british_close_gun_accuracy_130_only_inside_1200"),FMath::IsNearlyEqual(FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1200),1.30f,.001f)&&FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1201)==1.f);
  Run.Faction=EFrontlineFaction::German;B->PlayerFaction=Run.Faction;
  Check(TEXT("v84413_researched_german_close_gun_accuracy_130_only_inside_1200"),FMath::IsNearlyEqual(FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1200),1.30f,.001f)&&FrontlineArmourV63::CloseGunAccuracyScaleV8218(B,1201)==1.f);
  Run.Faction=EFrontlineFaction::British;B->PlayerFaction=Run.Faction;
  uint32 Eligible=0;for(const auto& D:Fleet->GetRoster())if(Fleet->IsUnlocked(D.Type))Eligible|=1u<<int32(D.Type);
  Check(TEXT("v84413_legacy_zero_selection_still_has_automatic_compatible_loadout"),FrontlineArmourV63::GetSelectedMask(B,Eligible)!=0);
  const int32 Points=Run.Research.ResearchPoints;
  bool Cancelled=true;for(const auto& D:Fleet->GetEquippedRoster())Cancelled&=FrontlineArmourV63::ToggleSelection(B,D.Type);
  Check(TEXT("v84413_all_armour_cards_can_be_cancelled_without_reverting_to_default"),Cancelled&&Run.bArmourSelectionExplicitV84413&&Run.ArmourSelectionMaskV63==0&&Fleet->GetEquippedRoster().IsEmpty()&&Run.Research.ResearchPoints==Points);
  const bool Variant=FrontlineArmourV63::ToggleConfigurationV8218(B,EFrontlineVehicle::MarkIVMale);
  Check(TEXT("v84413_unselected_variant_changes_without_repopulating_empty_allocation"),Variant&&Fleet->GetEquippedRoster().IsEmpty()&&FrontlineArmourV63::ConfigurationV8218(B,EFrontlineVehicle::MarkIVMale)==EFrontlineVehicle::MarkIVFemale);
  Check(TEXT("v84413_empty_then_two_selections_remain_reversible_and_cost_free"),FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::MarkIVFemale)
   &&FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::Whippet)&&Fleet->GetEquippedRoster().Num()==2&&Run.Research.ResearchPoints==Points);
  const bool Paid=FrontlineArmourV63::BuyRequisition(B,0),Refund=FrontlineArmourV63::BuyRequisition(B,0);
  Check(TEXT("v84413_ordinary_special_allocation_second_click_refunds_exact_paid_three_points"),Paid&&Refund&&Run.Research.ResearchPoints==Points&&Run.RequisitionPaidMaskV836==0&&FrontlineArmourV63::Requisition(B)==-1);
  // Use the ordinary serializer and loader, with a new file beneath this exact
  // disposable account. The player's configured progression path is restored.
  const FString Probe=FPaths::Combine(Dir,TEXT("Saved/V84413/readiness-selection-"))+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json");
  Check(TEXT("v84413_selection_codec_new_file_within_disposable_root"),Probe.StartsWith(Dir+TEXT("/"),ESearchCase::IgnoreCase)&&!IFileManager::Get().FileExists(*Probe));
  B->ProgressionFile=Probe;B->bProgressionVolatile=false;B->ArmyProgress[0]={};B->ArmyProgress[1]={};B->RewardedEncounterIds.Empty();
  const uint32 Two=Run.ArmourSelectionMaskV63;bool Saved=B->SaveProgression();if(Saved)B->LoadProgression(true);
  Check(TEXT("v84413_real_profile_roundtrip_preserves_two_selected_platforms"),Saved&&!B->bProgressionReadOnly&&B->CampaignRuns.Num()==1&&B->CampaignRuns[0].bArmourSelectionExplicitV84413&&B->CampaignRuns[0].ArmourSelectionMaskV63==Two);
  if(B->CampaignRuns.Num()==1&&!B->bProgressionReadOnly)
  {
   B->CampaignRuns[0].ArmourSelectionMaskV63=0;Saved=B->SaveProgression();if(Saved)B->LoadProgression(true);
   Check(TEXT("v84413_real_profile_roundtrip_preserves_explicit_empty_selection"),Saved&&!B->bProgressionReadOnly&&B->CampaignRuns.Num()==1&&B->CampaignRuns[0].bArmourSelectionExplicitV84413&&Fleet->GetEquippedRoster().IsEmpty());
   FString Text;TSharedPtr<FJsonObject> Json;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
   const bool Parsed=FFileHelper::LoadFileToString(Text,*Probe)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json)&&Json.IsValid()
    &&Json->TryGetArrayField(TEXT("campaigns"),Rows)&&Rows&&Rows->Num()==1;
   if(Parsed){(*Rows)[0]->AsObject()->RemoveField(TEXT("armour_selection_explicit_v84413"));FJsonSerializer::Serialize(Json,TJsonWriterFactory<>::Create(&Text));Saved=FFileHelper::SaveStringToFile(Text,*Probe);if(Saved)B->LoadProgression(true);}
   Check(TEXT("v84413_old_profile_missing_optional_field_retains_automatic_selection"),Parsed&&Saved&&!B->bProgressionReadOnly&&B->CampaignRuns.Num()==1
    &&!B->CampaignRuns[0].bArmourSelectionExplicitV84413&&!Fleet->GetEquippedRoster().IsEmpty());
  }
  B->bProgressionVolatile=true;B->bProgressionReadOnly=false;B->ProgressionFile=File;
  FrontlineRunArchiveAllocationChecksV84413(B,Check);
  Report->SetArrayField(TEXT("actual_dispatch_cycles"),Actual);
  Report->SetStringField(TEXT("scope"),TEXT("Actual Recruit, RecruitBlockReason, CooldownFraction and ordinary disposable profile roundtrips. UK/DE, three difficulties; no movement, natural combat win, or FR/RU campaign acceptance claim."));
  return Report;
 }
};
TSharedRef<FJsonObject> FrontlineRunReadinessChecksV84413(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineReadinessChecksV84413::Run(B,Check);}
