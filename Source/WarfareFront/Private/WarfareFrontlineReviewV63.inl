// Focused V63 integration review. Runs only with an isolated volatile profile.
struct FFrontlineReviewV63 : public TSharedFromThis<FFrontlineReviewV63>
{
 TWeakObjectPtr<AWarfareFrontlineController> PC;
 int32 Stage=0,CapturedStage=-1;double Next=0,Started=0;bool Good=true;TArray<TSharedPtr<FJsonValue>> Checks;FString Dir;
 TArray<TWeakObjectPtr<AFrontlineTank>> ArtTanks;
 double SampleAt=0,SampleLast=0,WallSum=0,GPUSum=0,GameSum=0,RenderSum=0;int32 SampleCount=0;
 TSharedPtr<FJsonObject> RenderSample;
 void Check(const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V63_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
 void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/Name,true,false);}
 bool Capture(const TCHAR* Name){if(CapturedStage==Stage)return false;Shot(Name);CapturedStage=Stage;return true;}
 void Finish(){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);if(RenderSample.IsValid())J->SetObjectField(TEXT("render_sample"),RenderSample);J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("review.json")));FPlatformMisc::RequestExit(false);}
 bool Tick(float)
 {
  const double Now=FPlatformTime::Seconds();if(Now<Next)return true;
  auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;if(!B||!B->LoadedSoldier||!C->GetHUD()){if(Now-Started>100){Check(TEXT("startup_ready"),false);Finish();return false;}return true;}
  if(Now-Started>180){Check(FString::Printf(TEXT("timeout_stage_%d"),Stage),false);Finish();return false;}
  auto* HUD=Cast<AWarfareFrontlineHUD>(C->GetHUD());auto& S=ScreenState(C);
  auto Report=[&](const FString& N,bool P){Check(N,P);};
  Next=Now+1.1;
  switch(Stage)
  {
  case 0:
   Check(TEXT("isolated_volatile_profile"),B->IsProgressionVolatile());
   Check(TEXT("german_campaign_fixture"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::German,19));
   B->SetActorTickEnabled(false);B->bProgressionReadOnly=false;B->CampaignRuns[0].CreatedUtc=B->CampaignRuns[0].UpdatedUtc=FDateTime::UtcNow().ToIso8601();
   FrontlineReviewUIV41::Clear(C);S.Dossier=0;
   Check(TEXT("latest_archive_selects_german_cover"),MenuCoverV63(B)&&MenuCoverV63(B)->GetName()==TEXT("T_MainMenu_German_V63"));
   ++Stage;Next=Now+3;break;
  case 1:
   if(Capture(TEXT("01-german-cover.png")))return true;
   Check(TEXT("british_campaign_fixture"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,14));B->bProgressionReadOnly=false;B->CampaignRuns[0].Research.ResearchPoints=100;
   B->CampaignRuns[0].CreatedUtc=B->CampaignRuns[0].UpdatedUtc=FDateTime::UtcNow().ToIso8601();
   FrontlineReviewUIV41::Technology(C,2,-1);
   for(int32 Id=0;Id<8;++Id)HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("V63ArmourBuy%d"),Id)));
   Check(TEXT("all_eight_technologies_buy_through_actual_ui"),B->CampaignRuns[0].ArmourResearchMaskV63==255&&B->GetResearchPoints()==78);
   {const int32 Points=B->GetResearchPoints();HUD->NotifyHitBoxClick(TEXT("V63ArmourBuy0"));Check(TEXT("duplicate_research_not_charged"),B->GetResearchPoints()==Points);}
   FrontlineArmourV63::RunChecks(B,Report);++Stage;break;
  case 2:
   if(Capture(TEXT("02-armour-research.png")))return true;S.Technology=false;S.Deployment=true;S.DeploymentTabV63=1;V28ReviewPointer=FVector2D(1870,1030);++Stage;break;
  case 3:
   if(Capture(TEXT("03-two-slot-armour-selection.png")))return true;
   HUD->NotifyHitBoxClick(TEXT("V63ArmourSelect0"));
   Check(TEXT("actual_loadout_click_removes_existing_armour"),!FrontlineArmourV63::IsSelected(B,EFrontlineVehicle::MarkIVMale));
   HUD->NotifyHitBoxClick(TEXT("V63ArmourSelect4"));
   Check(TEXT("actual_loadout_click_replaces_with_female"),FrontlineArmourV63::IsSelected(B,EFrontlineVehicle::MarkIVFemale)&&FrontlineArmourV63::IsSelected(B,EFrontlineVehicle::Whippet));
   {const uint32 Before=B->CampaignRuns[0].ArmourSelectionMaskV63;HUD->NotifyHitBoxClick(TEXT("V63ArmourSelect6"));Check(TEXT("third_armour_selection_rejected"),B->CampaignRuns[0].ArmourSelectionMaskV63==Before&&!FrontlineArmourV63::IsSelected(B,EFrontlineVehicle::MarkVMale));}
   HUD->NotifyHitBoxClick(TEXT("V63Requisition0"));Check(TEXT("operation_requisition_is_purchased"),FrontlineArmourV63::Requisition(B)==0);
   {const int32 Points=B->GetResearchPoints();HUD->NotifyHitBoxClick(TEXT("V63Requisition1"));Check(TEXT("one_requisition_per_operation_no_second_charge"),FrontlineArmourV63::Requisition(B)==0&&B->GetResearchPoints()==Points);}
   Check(TEXT("cambrai_real_loading_fixture"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8));
   B->bProgressionReadOnly=false;B->CampaignRuns[0].CreatedUtc=B->CampaignRuns[0].UpdatedUtc=FDateTime::UtcNow().ToIso8601();
   FrontlineReviewUIV41::Dossier(C);S.AcceptedLoadoutMission=INDEX_NONE;
   {auto& Map=WarRoom52::State(C);WarRoom52::Synchronize(Map,B);Map.Preview=8;Map.Stage=WarRoom52::Stage(8);Map.WasActive=true;WarRoom52::FocusOperation(Map,B,WarRoom52::UV(FrontlineCampaignV52::GetNode(B,8).LonLat),true,3.2f);}
   ++Stage;Next=Now+4;break;
  case 4:
   Check(TEXT("map_active_before_real_begin_click"),FrontlineMapSceneV61::IsActive(C));HUD->NotifyHitBoxClick(TEXT("V52Begin"));
   Check(TEXT("loading_snapshot_captured_before_intermediate_pages"),FrontlineMapSceneV61::LoadingView(C)!=nullptr);++Stage;break;
  case 5:
   if(S.HistoricalDispatch)HUD->NotifyHitBoxClick(TEXT("V52DispatchChoice0"));
   ++Stage;break;
  case 6:
   Check(TEXT("real_deployment_page_hides_map"),S.Deployment&&!FrontlineMapSceneV61::IsActive(C));
   Check(TEXT("snapshot_survives_deployment_player_tick"),FrontlineMapSceneV61::LoadingView(C)!=nullptr);
   HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));++Stage;break;
  case 7:
   if(S.UnitIntroduction){HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));Next=Now+.35;break;}
   Check(TEXT("preparation_entered_through_actual_introduction_flow"),S.Preparing);++Stage;Next=Now+.35;break;
  case 8:
   Check(TEXT("loading_background_retained_after_map_restore"),S.Preparing&&FrontlineMapSceneV61::LoadingView(C)!=nullptr);
   Shot(TEXT("04-real-loading-background.png"));++Stage;Next=Now+1;break;
  case 9:
   if(S.Preparing){Next=Now+.4;break;}
   Check(TEXT("preparation_finishes_without_blank_retry_page"),B->Phase==EFrontlinePhase::Fighting&&S.PreparationError.IsEmpty());B->SetPaused(false);
   FrontlineFieldSitesV59::RunChecksV60(B,Report);FrontlineCombatV63::RunChecks(B,Report);
   if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->RunConfigurationChecksV63(Report);
   // Exercise the right-click input route and the ordinary shared queue.
   {B->Elapsed=1000;for(float& T:B->RecruitReadyAt[0])T=0;S.Menu=S.Settings=S.Help=S.Technology=false;
    const auto Roles=FieldRolesV32(B);const int32 Before=B->Squads.Num();V28ReviewPointer=FVector2D(72,DockTop+85);C->RightClickOrdersV63();
    TickAutoOrdersV63(C,B);Check(TEXT("right_click_recruits_selected_section"),!Roles.IsEmpty()&&S.AutoRoleV63==int32(Roles[0])&&B->Squads.Num()==Before+1);
    TickAutoOrdersV63(C,B);Check(TEXT("repeat_order_obeys_shared_readiness"),B->Squads.Num()==Before+1);
    for(float& T:B->RecruitReadyAt[0])T=0;TickAutoOrdersV63(C,B);Check(TEXT("repeat_order_resumes_when_ready"),B->Squads.Num()==Before+2);
    C->RightClickOrdersV63();Check(TEXT("second_right_click_cancels_repeat"),S.AutoRoleV63==INDEX_NONE);}
   for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   {int32 I=0;for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){FVector P(-2000+(I%6)*210,-320+(I/6)*190,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P)+88;U->SetActorLocation(P);U->bFieldEntered=true;++I;}}
   {int32 I=0;for(auto T:{EFrontlineVehicle::MarkVMale,EFrontlineVehicle::MarkIVPioneer,EFrontlineVehicle::A7VGretchen,EFrontlineVehicle::BeutePioneer})
    {auto* A=B->GetWorld()->SpawnActor<AFrontlineTank>();if(A&&A->Configure(B,T,I>=2?1:0,63080+I)){const float X=-3400+I*2200;FVector P(X,250,AWarfareFrontlineGround::HeightAt(X,250));A->SetActorLocation(P);ArtTanks.Add(A);}++I;}}
   FrontlineReviewUIV41::Camera(C,0,0,13000,650,true);V28ReviewPointer=FVector2D(1870,830);C->SupportTargetMode=1;V28ReviewAim=FVector(0,0,0);
   C->bPreviewValid=true;C->ArtilleryPreview=FVector(0,0,AWarfareFrontlineGround::HeightAt(0,0));S.AutoRoleV63=int32(EFrontlineRole::Rifle);++Stage;Next=Now+4;break;
  case 10:
   if(Capture(TEXT("05-armour-and-support-sight.png")))return true;C->SupportTargetMode=0;V28ReviewAim.Reset();++Stage;Next=Now+2;break;
  case 11:
   Next=0;
   if(SampleAt<=0){RenderSample=MakeShared<FJsonObject>();if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))RenderSample->SetObjectField(TEXT("settings"),R->GetDiagnostics());SampleAt=SampleLast=FPlatformTime::Seconds();break;}
   WallSum+=(Now-SampleLast)*1000.;SampleLast=Now;GPUSum+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());GameSum+=FPlatformTime::ToMilliseconds(GGameThreadTime);RenderSum+=FPlatformTime::ToMilliseconds(GRenderThreadTime);++SampleCount;
   if(Now-SampleAt<6.)break;
   RenderSample->SetNumberField(TEXT("frames"),SampleCount);RenderSample->SetNumberField(TEXT("seconds"),Now-SampleAt);RenderSample->SetNumberField(TEXT("mean_wall_ms"),WallSum/SampleCount);RenderSample->SetNumberField(TEXT("mean_gpu_ms"),GPUSum/SampleCount);RenderSample->SetNumberField(TEXT("mean_game_ms"),GameSum/SampleCount);RenderSample->SetNumberField(TEXT("mean_render_ms"),RenderSum/SampleCount);
   RenderSample->SetStringField(TEXT("scope"),TEXT("Six seconds, warmed fixed tactical camera with four V63 armour configurations. Battle director and soldier simulation held for repeatable presentation; FG disabled, offscreen. Wall scheduling includes frame cap; delayed RHI GPU/thread samples are not matched per-pass timings. This is not a combat FPS or hardware compatibility guarantee."));++Stage;break;
  default:Finish();return false;
  }
  return true;
 }
};
void StartFrontlineV63Review(AWarfareFrontlineController* Controller)
{
 FString UserDir;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV63Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return;
 auto R=MakeShared<FFrontlineReviewV63>();R->PC=Controller;R->Started=FPlatformTime::Seconds();R->Next=R->Started+5;
 R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V63/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
