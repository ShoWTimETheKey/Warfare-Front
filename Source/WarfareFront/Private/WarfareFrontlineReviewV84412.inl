#include "WarfareFrontlineGameplayFocusedV84412.h"
#include "WarfareFrontlineUIAssetsV8213.h"
namespace FrontlineFieldV32 { void RunEngineerFrontageChecksV84412(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>); }
namespace FrontlineUltimateArchiveV8215 { void RunParityChecksV84412(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>); }
namespace FrontlineVehicleArtV836 { void FrontlineRunArmourBalanceChecksV84412(TFunctionRef<void(const FString&,bool)>); }
namespace FrontlineMedalsV8215 { void RunExtensionChecksV84412(TFunctionRef<void(const FString&,bool)>); }
namespace FrontlineAchievementsV76 { void RunChallengeChecksV84412(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>); void WithScopedCombatLedgerV84412(AWarfareFrontlineGameMode*,TFunctionRef<void()>); }
TSharedRef<FJsonObject> FrontlineRunATProjectileChecksV84412(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
namespace
{
void StartFrontlineReviewV84412(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV84412GameplayFocused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  FString Dir;int32 Stage=0,Captures=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0,Next=0;
  double SyncBefore=0,MedalStarted=0;TArray<TSharedPtr<FJsonValue>> Checks;TArray<TWeakObjectPtr<AFrontlineTank>> Visuals;
 };
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  auto Finish=[&]()
  {
   for(const auto& W:R->Visuals)if(auto* V=W.Get())V->Destroy();V28ReviewPointer.Reset();
   if(B&&FrontlineUltimateArchiveV8215::Active(B))FrontlineUltimateArchiveV8215::Leave(B);
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);
   J->SetNumberField(TEXT("captures_requested"),R->Captures);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R->Began);
   J->SetStringField(TEXT("scope"),TEXT("Focused V84.4.12 only: real local engineer mine approach/work, UK/DE Archive enemy national arsenal and resident portrait I/O, 52 new armour balance/art checks, new medals and two challenge boundaries. Seven actual 4K captures: normal armour paint, first bilingual Archive deployment, and bilingual Mecklenburg first-class wall / second-class inspection. Medal display is explicitly a disposable award fixture, not natural award acceptance. No FR/RU, old full suite, HDR/FG or whole-campaign acceptance."));
   Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;
  };
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-R->Began<35)return true;Check(TEXT("v84412_world_ready"),false);return Finish();}
  if(!FrontlineGameplayFocusedV84412::Allowed(B)){Check(TEXT("v84412_strict_saved_review_profile"),false);return Finish();}
  if(Now-R->Began>150){Check(TEXT("v84412_focused_review_bounded"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;
   UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);
  };
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<35)return true;
   Check(TEXT("v84412_core_images_resident_before_first_interaction"),FrontlineUIAssetsV8213::PreloadReady());if(!R->Good)return Finish();
   Clear();if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
   Check(TEXT("v84412_starts_only_disposable_british_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v84412_actual_authored_ukde_geometry_selected"),B->SelectCampaignMission(17));if(!R->Good)return Finish();
   S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<75)return true;
   Check(TEXT("v84412_actual_terrain_preparation_succeeds"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   FrontlineFieldV32::RunEngineerFrontageChecksV84412(B,Check);
   FrontlineUltimateArchiveV8215::RunParityChecksV84412(B,Check);
   FrontlineVehicleArtV836::FrontlineRunArmourBalanceChecksV84412(Check);
   FrontlineMedalsV8215::RunExtensionChecksV84412(Check);
   FrontlineAchievementsV76::RunChallengeChecksV84412(B,Check);
   auto AT=MakeShared<FJsonObject>();AT->SetBoolField(TEXT("success"),false);
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{AT=FrontlineRunATProjectileChecksV84412(B,Check);});
   Write(TEXT("actual-at-projectiles.json"),AT);Check(TEXT("v84412_actual_at_projectile_chain_success"),AT->GetBoolField(TEXT("success")));
   Write(TEXT("ui-cache-after-checks.json"),FrontlineUIAssetsV8213::Summary());if(!R->Good)return Finish();
   Clear();
   const EFrontlineVehicle Models[]={EFrontlineVehicle::MarkVMale,EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIV};
   for(int32 I=0;I<3;++I)
   {
    FVector P(float(I-1)*1100.f,550.f,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
    auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>(P,FRotator::ZeroRotator);
    Check(FString::Printf(TEXT("v84412_actual_normal_armour_paint_%d"),I),V&&V->Configure(B,Models[I],I?1:0,844120+I));
    if(V){V->SetActorTransform(V->GroundPoseForContactV8218(P,V->GetActorRotation().Yaw,0.f));V->SetActorTickEnabled(false);R->Visuals.Add(V);}
   }
   if(!R->Good)return Finish();FFrontlineUIV8447Review::Camera(PC,FVector(0,550,120),4500);
   R->Stage=2;R->Next=FPlatformTime::Seconds()+2.;return true;
  }
  if(R->Stage==2)
  {
   bool Ready=R->Visuals.Num()==3;for(const auto& W:R->Visuals)Ready&=W.IsValid()&&W->AreAssetsReady();
   if(!Ready&&Now-R->Prepared<100)return true;Check(TEXT("v84412_three_normal_armour_materials_ready"),Ready);if(!R->Good)return Finish();
   Write(TEXT("armour-render.json"),FieldRenderInventoryV8449(B));
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures/01-armour-paint.png"),true,false);++R->Captures;R->Stage=3;R->Next=Now+.1;return true;
  }
  if(R->Stage==3)
  {
   for(const auto& W:R->Visuals)if(auto* V=W.Get())V->Destroy();R->Visuals.Reset();Clear();B->Phase=EFrontlinePhase::Briefing;
   Check(TEXT("v84412_archive_ui_unlock_is_disposable_account_only"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,0));
   R->SyncBefore=FrontlineUIAssetsV8213::Summary()->GetNumberField(TEXT("synchronous_requests"));
   const double Start=FPlatformTime::Seconds();Check(TEXT("v84412_real_first_archive_begin"),FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction::British,0,EFrontlineDifficulty::Veteran));
   S.Deployment=true;S.DeploymentTabV63=1;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("actual_archive_begin_cpu_ms"),(FPlatformTime::Seconds()-Start)*1000.);
   J->SetStringField(TEXT("timing_scope"),TEXT("Actual Begin/Reset command only, not screenshot or rendered frame latency."));Write(TEXT("archive-first-command.json"),J);
   FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);if(!R->Good)return Finish();R->Stage=4;R->Next=Now+1.;return true;
  }
  if(R->Stage==4)
  {
   Check(TEXT("v84412_first_archive_draw_has_no_synchronous_portrait_io"),FrontlineUIAssetsV8213::Summary()->GetNumberField(TEXT("synchronous_requests"))==R->SyncBefore);
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures/02-archive-first-zh.png"),true,false);++R->Captures;R->Stage=5;R->Next=Now+.1;return true;
  }
  if(R->Stage==5){FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);R->Stage=6;R->Next=Now+.5;return true;}
  if(R->Stage==6)
  {
   Write(TEXT("ui-cache-first-deployment.json"),FrontlineUIAssetsV8213::Summary());
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures/03-archive-first-en.png"),true,false);++R->Captures;R->Stage=7;R->Next=Now+.1;return true;
  }
  if(R->Stage==7)
  {
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());uint32 Eligible=0;
   if(Fleet)for(const auto& D:Fleet->GetRoster())if(Fleet->IsUnlocked(D.Type))Eligible|=1u<<int32(D.Type);
   const auto Before=FrontlineArmourV63::GetSelectedMask(B,Eligible);
   const double Start=FPlatformTime::Seconds();const bool Changed=FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::Whippet);
   const double FirstMs=(FPlatformTime::Seconds()-Start)*1000.;
   const bool Restored=FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::Whippet);
   Check(TEXT("v84412_first_real_archive_armour_selection_is_reversible"),Changed&&Restored&&FrontlineArmourV63::GetSelectedMask(B,Eligible)==Before);
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("first_actual_roster_mutation_cpu_ms"),FirstMs);J->SetStringField(TEXT("scope"),TEXT("Actual memory-only roster command, no screenshot frame timing or FPS guarantee."));Write(TEXT("archive-selection.json"),J);
   if(!R->Good)return Finish();R->Stage=8;return true;
  }
  if(R->Stage==8)
  {
   FrontlineUltimateArchiveV8215::Leave(B);Clear();B->Phase=EFrontlinePhase::Briefing;
   Check(TEXT("v84412_new_medal_display_fixture_has_no_live_units"),B->Units.IsEmpty());
   Check(TEXT("v84412_new_german_medals_seed_only_disposable_display"),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81::German,3));
   MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81::German),false);
   if(!R->Good)return Finish();R->MedalStarted=Now;R->Stage=9;return true;
  }
  auto MedalPage=[&](bool Chinese,bool Inspect)
  {
   S.AchievementsV76=true;FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   PC->GetHUD()->NotifyHitBoxClick(TEXT("V8215MedalNation1"));PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalSelect42"));
   auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Inspect=false;
   if(Inspect){PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));for(int32 N=0;M.PreviewGrade!=1&&N<8;++N)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalVariantPrev"));}
   Check(FString::Printf(TEXT("v84412_medal_%s_%s_actual_class_and_selection"),Chinese?TEXT("zh"):TEXT("en"),Inspect?TEXT("inspect"):TEXT("wall")),
    M.Wall&&M.Nation==1&&M.Focus==42&&M.Inspect==Inspect&&FrontlineMedalsV8215::Tier(42)==2&&(!Inspect||M.PreviewGrade==1));
  };
  if(R->Stage==9)
  {
   const bool Ready=FrontlineUIAssetsV8213::PreloadReady()&&FrontlineTextureReady(MedalWallV8215::Art(42,1))&&FrontlineTextureReady(MedalWallV8215::Art(42,2));
   if(!Ready&&Now-R->MedalStarted<30)return true;Check(TEXT("v84412_new_medal_both_real_class_textures_resident"),Ready);
   if(!R->Good)return Finish();MedalPage(true,false);if(!R->Good)return Finish();R->Stage=10;R->Next=Now+.5;return true;
  }
  if(R->Stage==10||R->Stage==12||R->Stage==14||R->Stage==16)
  {
   const TCHAR* Shot=R->Stage==10?TEXT("04-medal-wall-zh-class2.png"):R->Stage==12?TEXT("05-medal-inspect-zh-class1.png"):R->Stage==14?TEXT("06-medal-wall-en-class2.png"):TEXT("07-medal-inspect-en-class1.png");
   int32 W=0,H=0;PC->GetViewportSize(W,H);Check(FString(Shot)+TEXT("_actual_4k"),W==3840&&H==2160);if(!R->Good)return Finish();
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/Shot,true,false);++R->Captures;++R->Stage;R->Next=Now+.1;return true;
  }
  if(R->Stage==11||R->Stage==13||R->Stage==15)
  {
   MedalPage(R->Stage==11,R->Stage!=13);if(!R->Good)return Finish();++R->Stage;R->Next=Now+.5;return true;
  }
  if(R->Stage==17)
  {
   TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(R->Dir/TEXT("Captures/*.png")),true,false);
   Check(TEXT("v84412_seven_actual_4k_captures_written"),R->Captures==7&&Files.Num()==7);return Finish();
  }
  return true;
 }));
}
}
