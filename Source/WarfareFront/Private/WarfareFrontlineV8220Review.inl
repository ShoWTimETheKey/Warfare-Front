#include "WarfareFrontlineWreckVisualV8220Review.inl"

namespace
{
void StartFrontlineV8220Review(AWarfareFrontlineController* Controller)
{
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  int32 Stage=0,Shots=0,SamplePhase=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2.,Prepared=0,SampleBegan=0,LastSample=0;
  FString Dir;TArray<TSharedPtr<FJsonValue>> Checks,Samples;
 };
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V8220_CHECK %s %d"),*Name,int32(Pass));};
  auto Finish=[&](){FFrontlineWreckVisualV8220Review::Clear();V28ReviewPointer.Reset();auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("warmed_frame_samples"),R->Samples);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V82.20 actual isolated Shipping air-support schedules/hits/interception, unique infantry armour tally/persistence, UK/DE live/burning/cooled armour bindings and bilingual UI. Disposable fixtures; no French/Russian campaigns or whole-campaign/fun certification."));FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("checks.json")));FPlatformMisc::RequestExit(false);return false;};
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v8220_strict_disposable_world"),false);return Finish();}
  if(Now-R->Began>150.){Check(TEXT("v8220_bounded_review"),false);return Finish();}
  if(R->SampleBegan>0&&Now-R->SampleBegan>.7&&R->LastSample>0&&!FScreenshotRequest::IsScreenshotRequested())
  {
   auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("surface"),R->SamplePhase?TEXT("coherent_wreck"):TEXT("legacy_damage"));J->SetNumberField(TEXT("wall_ms"),(Now-R->LastSample)*1000.);J->SetNumberField(TEXT("game_ms"),FPlatformTime::ToMilliseconds(GGameThreadTime));J->SetNumberField(TEXT("render_ms"),FPlatformTime::ToMilliseconds(GRenderThreadTime));J->SetNumberField(TEXT("rhi_ms"),FPlatformTime::ToMilliseconds(GRHIThreadTime));J->SetNumberField(TEXT("gpu_ms"),FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));R->Samples.Add(MakeShared<FJsonValueObject>(J));
  }
  R->LastSample=Now;
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
  auto Prepare=[&](EFrontlineFaction Nation){Clear();FFrontlineWreckVisualV8220Review::Clear();B->Phase=EFrontlinePhase::Briefing;B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);Check(TEXT("v8220_campaign_fixture_created"),B->StartNewCampaign(Nation,EFrontlineDifficulty::Veteran));if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;C->ArmourResearchMaskV63=0;C->SupportSelectionMask=(1u<<int32(EFrontlineSupport::AirBombing))|(1u<<int32(EFrontlineSupport::AirStrike));}Check(TEXT("v8220_normal_operation_selected"),B->SelectCampaignMission(16));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=FPlatformTime::Seconds();};
  auto Ready=[&](){if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<65.)return 0;const bool Good=B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty();Check(TEXT("v8220_actual_prepared_battle"),Good);if(!Good)return -1;B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);Clear();return 1;};
  auto Wall=[&](bool ZH){Clear();FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=B->GetPlayerFaction()==EFrontlineFaction::German?1:0;};
  double Delay=.7;
  switch(R->Stage)
  {
   case 0:Clear();if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Prepare(EFrontlineFaction::British);Delay=.1;break;
   case 1:{const int32 State=Ready();if(State==0){R->Next=Now+.1;return true;}if(State<0)return Finish();RunAirSupportChecksV8220(B,Check);FrontlineArmourKillTallyV8220::RunChecks(B,Check);FFrontlineWreckVisualV8220Review::Setup(B,PC,R->Dir,Check);Check(TEXT("v8220_real_british_infantry_kill_notification"),FrontlineArmourKillTallyV8220::SeedNotificationReview(B));Delay=1.;break;}
   case 2:Shot(TEXT("01-british-live-burning-and-service-tally-en"));break;
   case 3:Wall(true);break;
   case 4:Shot(TEXT("02-british-service-wall-zh"));break;
   case 5:Clear();FFrontlineWreckVisualV8220Review::Cooled();FFrontlineWreckVisualV8220Review::Camera(PC);Delay=6.;break;
   case 6:{if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld())){const auto V=Fire->VisualStateV8214();Check(TEXT("v8220_cooled_wreck_fire_and_soot_expire_naturally"),V.Wreck==0&&V.Soot==0&&V.VolumeSootV8216==0);}FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Check(TEXT("v8220_same_geometry_legacy_surface_comparison"),FFrontlineWreckVisualV8220Review::LegacySurfaces(true));R->SamplePhase=0;R->SampleBegan=Now;Delay=4.;break;}
   case 7:R->SampleBegan=0;FFrontlineWreckVisualV8220Review::LegacySurfaces(false);R->SamplePhase=1;R->SampleBegan=Now;Delay=4.;break;
   case 8:R->SampleBegan=0;Shot(TEXT("03-british-live-versus-cooled-wreck-en"));break;
   case 9:Check(TEXT("v8220_actual_strafing_capture_prepared"),SetupAirSupportCaptureV8220(B,0));Delay=1.;break;
   case 10:Shot(TEXT("04-actual-strafing-airframe-and-earth-strikes-en"));break;
   case 11:Check(TEXT("v8220_actual_bomber_capture_prepared"),SetupAirSupportCaptureV8220(B,1));Delay=1.;break;
   case 12:Shot(TEXT("05-actual-staggered-bomber-formation-en"));break;
   case 13:Prepare(EFrontlineFaction::German);Delay=.1;break;
   case 14:{const int32 State=Ready();if(State==0){R->Next=Now+.1;return true;}if(State<0)return Finish();FFrontlineWreckVisualV8220Review::Setup(B,PC,R->Dir,Check);FFrontlineWreckVisualV8220Review::Cooled();Check(TEXT("v8220_real_german_infantry_kill_notification"),FrontlineArmourKillTallyV8220::SeedNotificationReview(B));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);Delay=1.;break;}
   case 15:Shot(TEXT("06-german-service-tally-zh"));break;
   case 16:Wall(false);break;
   case 17:Shot(TEXT("07-german-service-wall-en"));break;
   default:Clear();Check(TEXT("v8220_service_badge_rhi_ready"),FrontlineTextureReady(ArmourKillTallyUIV8220::Badge(EFRRUNationV81::British)));return Finish();
  }
  ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
 }),0.f);
}
}
