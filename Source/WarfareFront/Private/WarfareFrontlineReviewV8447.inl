namespace
{
void StartFrontlineReviewV8447(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Page=0,Captures=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+.5,Prepared=0;uint64 Frame=0;FVector Focus;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto Run=MakeShared<FRun>();Run->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Run->Dir/Name));};
  auto Finish=[&](){if(B)FrontlineTrenchMotionV8447::CleanupPoseV8447(B);auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Run->Good);J->SetArrayField(TEXT("checks"),Run->Checks);J->SetNumberField(TEXT("captures_requested"),Run->Captures);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Run->Began);J->SetStringField(TEXT("scope"),TEXT("Isolated real actor cover/weapon cycles, trench movement/reload pose, picker wheel dispatch and current map binding. Clear-plane probability probes are separated from grounded tests. No full campaign or sustained performance acceptance."));Write(TEXT("checks.json"),J);V27ImageAuditActive=PageCanvasV73::Audit=false;FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-Run->Began<35)return true;Check(TEXT("v8447_world_ready"),false);return Finish();}
  if(Now-Run->Began>230){Check(TEXT("v8447_bounded_review"),false);return Finish();}
  if(Now<Run->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& S=ScreenState(PC);
  auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;S.Preparing=false;MedalMountV8216::CloseV836(PC);UGameplayStatics::SetGamePaused(PC,false);V28ReviewPointer=FVector2D(-10000,-10000);};
  if(Run->Stage==0)
  {
   Check(TEXT("v8447_disposable_ukde_only"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));Clear();
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetHDREnabled(false,false);R->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);}
   Check(TEXT("v8447_british_actual_test_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* R=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){R->CompletedMissions=17;R->WonOperationMaskV52=(1u<<17)-1;R->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v8447_exact_disposable_british_decoration_seed"),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81::British,3));
   Check(TEXT("v8447_actual_operation_geometry_selected"),B->SelectCampaignMission(17));if(!Run->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);Run->Prepared=Now;Run->Stage=1;return true;
  }
  if(Run->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-Run->Prepared<75)return true;
   Check(TEXT("v8447_actual_terrain_and_assets_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!Run->Good)return Finish();B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);Run->Stage=2;return true;
  }
  if(Run->Stage==2)
  {
   auto Balance=RunFrontlineBalanceV8447Checks(B,Check);Write(TEXT("balance.json"),Balance);Check(TEXT("v8447_balance_report_passed"),Balance->GetBoolField(TEXT("success")));
   auto Motion=FrontlineTrenchMotionV8447::RunChecks(B,Check);Write(TEXT("motion.json"),Motion);Check(TEXT("v8447_motion_report_passed"),Motion->GetBoolField(TEXT("success")));
   auto Legacy=FrontlineTrenchMotionV8444::RunChecks(B,Check);Write(TEXT("motion-compatibility.json"),Legacy);Check(TEXT("v8447_existing_physical_entry_advance_checks_passed"),Legacy->GetBoolField(TEXT("success")));
   MedalMountV8216::RunPickerChecksV8447(Check);FrontlineLoadingPageV82::RunMapChecksV8447(Check);if(!Run->Good)return Finish();Run->Stage=3;Run->Next=FPlatformTime::Seconds()+.1;return true;
  }
  if(Run->Stage==3)
  {
   if(Run->Page==5||Run->Page==8){PC->MenuBackV61();Check(TEXT("v8447_escape_closes_picker_and_restores_camera_zoom"),!MedalMountV8216::View(PC).Picker);const float Width=FFrontlineUIV8447Review::Width(PC);FFrontlineUIV8447Review::Wheel(PC,true);Check(TEXT("v8447_camera_zoom_works_after_picker_closes"),FFrontlineUIV8447Review::Width(PC)>Width);}
   Clear();B->Phase=EFrontlinePhase::Fighting;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
   if(Run->Page<4||Run->Page>=8)
   {
    const bool Machinegun=Run->Page>=8;int32 Team=(Machinegun?Run->Page-8:Run->Page)/2;const bool Reload=(Run->Page%2)!=0;
    if(Machinegun&&B->GetPlayerFaction()==EFrontlineFaction::German)Team=1-Team;
    Check(TEXT("v8447_actual_paired_live_pose_prepared"),FrontlineTrenchMotionV8447::PreparePoseV8447(B,Team,Reload,Run->Focus,Check,Machinegun));FFrontlineUIV8447Review::Camera(PC,Run->Focus,3400.f);
   }
   else if(Run->Page<6)
   {
    FrontlineTrenchMotionV8447::CleanupPoseV8447(B);FFrontlineUIV8447Review::Camera(PC,FVector(0,0,150),6000.f);FrontlineLocalizationV35::SetLanguage(Run->Page==5?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
    PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalMount"));auto& Picker=MedalMountV8216::View(PC);const auto Awards=MedalMountV8216::AwardedV8447(Picker.Nation);const float Maximum=MedalMountV8216::PickerLayoutV8447::Maximum(Awards.Num());
    Check(TEXT("v8447_actual_picker_opens_with_scrollable_earned_roster"),Picker.Picker&&Awards.Num()>8&&Maximum>0);
    const float Width=FFrontlineUIV8447Review::Width(PC);V28ReviewPointer=FVector2D(500,400);for(int32 N=0;N<20;++N)FFrontlineUIV8447Review::Wheel(PC,true);
    Check(TEXT("v8447_picker_actual_wheel_scrolls_and_never_zooms"),FMath::IsNearlyEqual(Picker.CatalogueV8447.Target,Maximum,.001f)&&FFrontlineUIV8447Review::Width(PC)==Width);
    V28ReviewPointer=FVector2D(10,10);FFrontlineUIV8447Review::Wheel(PC,true);Check(TEXT("v8447_picker_backdrop_wheel_is_consumed_without_zoom_or_scroll"),FFrontlineUIV8447Review::Width(PC)==Width&&Picker.CatalogueV8447.Target==Maximum);
    V28ReviewPointer=FVector2D(980,880);FFrontlineUIV8447Review::Wheel(PC,false);Check(TEXT("v8447_picker_fixed_footer_wheel_is_consumed"),FFrontlineUIV8447Review::Width(PC)==Width&&Picker.CatalogueV8447.Target==Maximum);
    if(Run->Page==4){V28ReviewPointer=FVector2D(500,400);for(int32 N=0;N<20;++N)FFrontlineUIV8447Review::Wheel(PC,false);Check(TEXT("v8447_picker_actual_wheel_returns_to_first_row"),Picker.CatalogueV8447.Target==0&&FFrontlineUIV8447Review::Width(PC)==Width);}
    V28ReviewPointer=FVector2D(-10000,-10000);PageCanvasV73::Audit=true;PageCanvasV73::Hits.Empty();
   }
   else
   {
    FrontlineTrenchMotionV8447::CleanupPoseV8447(B);const int32 Mission=Run->Page==6?17:10;const auto Faction=Run->Page==6?EFrontlineFaction::British:EFrontlineFaction::German;
    Check(TEXT("v8447_actual_report_encounter_bound"),FFrontlineUIV8447Review::Report(B,Faction,Mission));Clear();FrontlineLocalizationV35::SetLanguage(Run->Page==7?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);V27ImageAuditActive=true;V27ImageAudit.Empty();
   }
   if(!Run->Good)return Finish();Run->Frame=GFrameCounter;Run->Stage=4;Run->Next=Now+.65;return true;
  }
  if(Run->Stage==4)
  {
   if(GFrameCounter<=Run->Frame+2)return true;int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("v8447_actual_4k_output"),W==3840&&H==2160);
   if(Run->Page==4||Run->Page==5)
   {
    bool NoPaging=true;for(const auto& P:PageCanvasV73::Hits){const auto Name=P.Value->GetStringField(TEXT("command"));NoPaging&=Name!=TEXT("V8216MountPrev")&&Name!=TEXT("V8216MountNext");}Check(TEXT("v8447_actual_picker_has_no_paging_buttons"),NoPaging);
    const auto& P=MedalMountV8216::View(PC);Check(TEXT("v8447_actual_picker_scroll_offset_reaches_requested_row"),FMath::Abs(P.CatalogueV8447.Offset-P.CatalogueV8447.Target)<1.f);
   }
   if(Run->Page==6||Run->Page==7)
   {
    const auto* M=B->GetEncounterCampaignMission();auto* T=FrontlineLoadingPageV82::OperationMapV8447(B,M);int32 Match=0;
    for(const auto& P:V27ImageAudit)if(T&&P.Value->GetStringField(TEXT("texture"))==T->GetPathName()&&P.Value->GetNumberField(TEXT("draw_x"))>1000&&P.Value->GetNumberField(TEXT("draw_y"))>=474&&P.Value->GetNumberField(TEXT("draw_y"))<660)++Match;
    Check(TEXT("v8447_actual_after_battle_draw_uses_loading_operation_map"),T&&Match==1&&M->Id==(Run->Page==6?17:10));
   }
   if(!Run->Good)return Finish();static const TCHAR* Names[]={TEXT("01-uk-aim"),TEXT("02-uk-reload"),TEXT("03-de-aim"),TEXT("04-de-reload"),TEXT("05-picker-top-en"),TEXT("06-picker-bottom-zh"),TEXT("07-uk-late-report-map"),TEXT("08-de-report-map-zh"),TEXT("09-uk-mg-aim"),TEXT("10-uk-mg-reload"),TEXT("11-de-mg-aim"),TEXT("12-de-mg-reload")};FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/(FString(Names[Run->Page])+TEXT(".png")),true,false);++Run->Captures;
   if(Run->Captures==12)Run->Stage=5;
   else {Run->Page=Run->Page==5?8:Run->Page==11?6:Run->Page+1;Run->Stage=3;}
   Run->Next=Now+.1;return true;
  }
  if(Run->Stage==5){TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Run->Dir/TEXT("Captures/*.png")),true,false);bool Good=Files.Num()==12;for(const auto& F:Files)Good&=IFileManager::Get().FileSize(*(Run->Dir/TEXT("Captures")/F))>1024;Check(TEXT("v8447_all_affected_page_captures_written"),Good&&Run->Captures==12);return Finish();}
  Check(TEXT("v8447_known_stage"),false);return Finish();
 }),0.f);
}
}
