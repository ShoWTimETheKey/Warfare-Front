// Production public HUD, Enter, Escape and XBUTTON routes on a disposable UK/DE
// profile. Result phases are UI fixtures; this is not a campaign-win claim.
void RunArchiveFlowChecksV851(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=PC&&B&&B->GetWorld()&&B->IsProgressionVolatile()
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))
  &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&!WarfareFRRUCampaignV82::Active(B);
 Check(TEXT("v851_archive_navigation_strict_disposable_scope"),Safe);if(!Safe)return;
 auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());Check(TEXT("v851_archive_navigation_real_hud"),HUD!=nullptr);if(!HUD)return;
 auto& Screen=ScreenState(PC);
 const auto Clear=[&]()
 {
  if(FrontlineUltimateArchiveV8215::Active(B))FrontlineUltimateArchiveV8215::Leave(B);
  // Review starts on a live map. Leave through the ordinary non-awarding
  // abandonment API before creating the next disposable campaign fixture.
  B->AbandonBattleToOperations();
  UltimateArchiveUIV8215::Close(PC);UltimateArchiveUIV8215::View(PC).EntryV851.Reset();
  Screen.Menu=Screen.Help=Screen.Settings=Screen.Technology=Screen.AchievementsV76=Screen.Deployment=Screen.Preparing=false;
  Screen.UnitIntroduction=Screen.HistoricalDispatch=Screen.CampaignEndingV51=Screen.ExitConfirmV61=false;
  Screen.PhotoTransitionV37=Screen.SmallTransitionV35=Screen.bPausedByOverlay=Screen.bPausedForAuxOverlay=false;
  Screen.FactionSelectionV82=FFrontlineFactionSelectionV82();Screen.Dossier=0;PC->CancelTargeting();
  auto& History=NavigationV61::History(PC);History.Back.Empty();History.Forward.Empty();
  UGameplayStatics::SetGamePaused(B,false);MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::CloseV836(PC);
 };
 Clear();Check(TEXT("v851_archive_navigation_unlocked_review_only"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,0));
 for(int32 Origin=0;Origin<2;++Origin)for(int32 Route=0;Route<13;++Route)
 {
  Clear();const FString Prefix=FString::Printf(TEXT("v851_archive_origin%d_route%d_"),Origin,Route);
  if(!B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran)){Check(Prefix+TEXT("source_campaign"),false);continue;}
  const FGuid OriginalId=B->GetCurrentCampaign()->Id;Screen.Dossier=Origin?1917:0;
  HUD->NotifyHitBoxClick(TEXT("V8215ArchiveOpen"));
  Check(Prefix+TEXT("actual_open_captures_source_page"),UltimateArchiveUIV8215::Active(PC)
    &&UltimateArchiveUIV8215::View(PC).EntryV851.IsSet()
    &&UltimateArchiveUIV8215::View(PC).EntryV851.GetValue().Dossier==(Origin?1917:0));
  HUD->NotifyHitBoxClick(TEXT("V8215ArchiveFaction1"));HUD->NotifyHitBoxClick(TEXT("V8215ArchiveMission17"));
  HUD->NotifyHitBoxClick(TEXT("V8215ArchiveDifficulty2"));HUD->NotifyHitBoxClick(TEXT("V8215ArchiveBegin"));
  Check(Prefix+TEXT("actual_begin_uses_selected_german_replay"),FrontlineUltimateArchiveV8215::Active(B)
    &&B->GetPlayerFaction()==EFrontlineFaction::German&&Screen.Deployment&&!UltimateArchiveUIV8215::Active(PC));
  switch(Route)
  {
  case 0:HUD->NotifyHitBoxClick(TEXT("V32DeploymentBack"));break;
  case 1:PC->ToggleMenu();break;
  case 2:PC->MenuBackV61();break;
  case 3:Screen.Deployment=false;PC->ToggleMenu();break;
  case 4:Screen.Deployment=false;PC->MenuBackV61();break;
  case 5:Screen.Deployment=false;Screen.Preparing=true;HUD->NotifyHitBoxClick(TEXT("PreparationBack"));break;
  case 6:Screen.Deployment=false;B->Phase=EFrontlinePhase::Fighting;Screen.Menu=true;HUD->NotifyHitBoxClick(TEXT("AbandonBattle"));break;
  case 7:Screen.Deployment=false;B->Phase=EFrontlinePhase::Victory;HUD->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));break;
  case 8:Screen.Deployment=false;B->Phase=EFrontlinePhase::Defeat;PC->BeginBattle();break;
  case 9:Screen.Deployment=false;HUD->NotifyHitBoxClick(TEXT("MenuHome"));break;
  case 10:Screen.Deployment=false;HUD->NotifyHitBoxClick(TEXT("NewCampaign"));break;
  case 11:Screen.Deployment=false;HUD->NotifyHitBoxClick(TEXT("ContinueCampaign"));break;
  case 12:Screen.Deployment=false;HUD->NotifyHitBoxClick(TEXT("CampaignArchives"));break;
  }
  const auto& Selection=UltimateArchiveUIV8215::View(PC);
  Check(Prefix+TEXT("replay_retired_and_original_profile_restored"),!FrontlineUltimateArchiveV8215::Active(B)
    &&B->GetPlayerFaction()==EFrontlineFaction::British&&B->GetCurrentCampaign()&&B->GetCurrentCampaign()->Id==OriginalId);
  if(Route<9)
  {
   Check(Prefix+TEXT("return_opens_selector_with_last_army_operation_difficulty"),Selection.Open
      &&Selection.Faction==EFrontlineFaction::German&&Selection.Mission==17&&Selection.Difficulty==2
      &&!Screen.Menu&&!Screen.Deployment&&!Screen.Preparing&&!Screen.ExitConfirmV61);
   if(Route%3==0)HUD->NotifyHitBoxClick(TEXT("V8215ArchiveClose"));else if(Route%3==1)PC->ToggleMenu();else PC->MenuBackV61();
   Check(Prefix+TEXT("selector_close_returns_to_actual_source_not_hidden_campaign"),!UltimateArchiveUIV8215::Active(PC)
      &&Screen.Dossier==(Origin?1917:0)&&!Screen.ExitConfirmV61&&!Screen.Menu&&!Screen.Deployment);
   Check(Prefix+TEXT("selector_close_does_not_reinsert_hidden_replay_into_side_button_history"),NavigationV61::History(PC).Back.IsEmpty()&&NavigationV61::History(PC).Forward.IsEmpty());
  }
  else
  {
   const int32 Destination=Route==9?0:Route==10?1918:Route==11?1917:1919;
   Check(Prefix+TEXT("explicit_navigation_keeps_its_requested_destination"),!Selection.Open&&Screen.Dossier==Destination&&!Screen.Deployment);
  }
 }
 Clear();UltimateArchiveUIV8215::ReturnToSelectionV85(PC,EFrontlineFaction::German,17,EFrontlineDifficulty::Survivor);
 PC->ToggleMenu();Check(TEXT("v851_archive_selector_without_source_fails_safely_to_main_menu"),!UltimateArchiveUIV8215::Active(PC)&&Screen.Dossier==0);
}
