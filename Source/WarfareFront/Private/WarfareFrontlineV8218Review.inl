#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineV8218CombatReview.inl"

namespace FrontlineReviewV8218
{
struct FCanvasProbe
{
 bool Enabled=false,Done=false,FacilityPointer=false;
 TFunction<void(const FString&,bool)> Check;
};
TSharedPtr<FCanvasProbe> CanvasProbe;
// Root may add a changed-art check without editing this bounded sequence.
TFunction<void(AWarfareFrontlineGameMode*,AWarfareFrontlineController*,TFunctionRef<void(const FString&,bool)>)> AdditionalChecks;
}

// DrawHUD calls this after constructing its actual Canvas. The local value
// probe shares the same surface, scale and projection; it never moves an OS
// cursor or records a hit box. Only the explicitly disposable review can enter.
namespace
{
void ProbeFrontlineCanvasV8218Review(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,float Top)
{
 const auto P=FrontlineReviewV8218::CanvasProbe;
 if(!P||!P->Enabled||!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review"))||B->Phase!=EFrontlinePhase::Fighting)return;
 if(!P->Done)
 {
  FFrontlineCanvas Probe=UI;Probe.HoverShortcut.Empty();Probe.TooltipStateV83=nullptr;
  MedalUIReviewV8218::WithCanvas(Probe,B,PC,Top,[P](const FString& Name,bool Pass){if(P->Check)P->Check(Name,Pass);});P->Done=true;
 }
 if(P->FacilityPointer)
 {
  for(const auto& Site:FrontlineFieldSitesV59::Views(B))
  {
   FVector2D Point;if(!FieldHoverV8218::SitePoint(UI,PC,Site,Top,Point))continue;
   FVector2D FlagPoint;UTexture2D* Flag=nullptr;float Height=0;
   V28ReviewPointer=FieldHoverV8218::FlagPoint(UI,B,PC,Site,Top,FlagPoint,Flag,Height)?FlagPoint-FVector2D(0,Height*.5f):Point;
   return;
  }
 }
}

void StartFrontlineV8218Review(AWarfareFrontlineController* Controller)
{
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  int32 Stage=0,Shots=0,ArtPhase=0;bool Passed=true;double Started=FPlatformTime::Seconds(),Next=Started+2.,Prepared=0;
  FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;
  TArray<TWeakObjectPtr<AFrontlineTank>> ArtVehicles;
  TSharedPtr<FrontlineReviewV8218::FCanvasProbe> Probe=MakeShared<FrontlineReviewV8218::FCanvasProbe>();
 };
 auto R=MakeShared<FRun>();R->Dir=User;FrontlineReviewV8218::CanvasProbe=R->Probe;
 TWeakPtr<FRun> WeakRun(R);R->Probe->Check=[WeakRun](const FString& Name,bool Pass){if(const auto Run=WeakRun.Pin()){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Passed&=Pass;}};
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak](float)
 {
  auto* PC=Weak.Get();auto* B=PC?FrontlineFor(PC):nullptr;auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
  const double Now=FPlatformTime::Seconds();auto Check=[R](const FString& Name,bool Pass){R->Probe->Check(Name,Pass);};
  auto Finish=[&]()
  {
   R->Probe->Enabled=false;V28ReviewPointer.Reset();FrontlineReviewV8218::CanvasProbe.Reset();
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Passed);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);
   J->SetStringField(TEXT("scope"),TEXT("V82.18 changed-path checks in an actual isolated Shipping world: two normally prepared UK/DE facilities scenes, armour configuration, combat, real Canvas hover geometry, dark medals and joined ribbons. 4K SDR High. Existing desktop user instances retained. No French/Russian campaign, whole-campaign, HDR or performance certification."));
   FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("checks.json")));FPlatformMisc::RequestExit(false);return false;
  };
  if(!B||!HUD||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v8218_actual_world_has_strict_disposable_profile"),false);return Finish();}
  if(Now-R->Started>150.){Check(TEXT("v8218_changed_path_review_bounded_150_seconds"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   UGameplayStatics::SetGamePaused(PC,false);
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;TypographyReviewPageV8210=INDEX_NONE;
   MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;
   R->Probe->Enabled=R->Probe->FacilityPointer=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);
  };
  auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
  auto Prepare=[&](EFrontlineFaction Nation)
  {
   Clear();B->Phase=EFrontlinePhase::Briefing;B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);
   const bool Created=B->StartNewCampaign(Nation,EFrontlineDifficulty::Veteran);
   if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
   {C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;C->ArmourResearchMaskV63=65535;}
   const bool Selected=Created&&B->SelectCampaignMission(7);Check(Nation==EFrontlineFaction::German?TEXT("v8218_german_facilities_mission_selected"):TEXT("v8218_british_facilities_mission_selected"),Selected);
   S.Dossier=1917;S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;
  };
  auto Ready=[&](const TCHAR* Name)
  {
   if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<65.)return 0;
   const bool Prepared=B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty();Check(Name,Prepared);
   if(!Prepared)return -1;B->SetActorTickEnabled(false);Clear();return 1;
  };
  auto FreezeBattle=[&]()
  {
   Clear();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->SetActorTickEnabled(false);
   const auto Sites=FrontlineFieldSitesV59::Views(B);const auto* Site=Sites.FindByPredicate([](const auto& V){return V.Kind!=FrontlineFieldSitesV59::EKind::FieldWorks||V.Work>0||V.Integrity<=0;});
   Check(TEXT("v8218_prepared_map_contains_presentable_real_facility"),Site!=nullptr);
   if(Site)FFrontlineReviewV32::Camera(PC,float(Site->Position.X),float(Site->Position.Y),6500,250);else FFrontlineReviewV32::Camera(PC,0,0,6500,250);
  };
  auto ArtCamera=[&](bool Close)
  {
   if(R->ArtVehicles.Num()!=2||!R->ArtVehicles[0].IsValid()||!R->ArtVehicles[1].IsValid())return;
   const auto A=R->ArtVehicles[0]->GetActorLocation(),C=R->ArtVehicles[1]->GetActorLocation();const auto Focus=Close?A:(A+C)*.5f;
   FFrontlineReviewV32::Camera(PC,float(Focus.X),float(Focus.Y),Close?2300.f:FMath::Max(3200.f,float(FMath::Abs(A.X-C.X))+2000.f),180);
  };
  auto LatePage=[&](EFrontlineFaction Nation,bool Technology)
  {
   Clear();B->Phase=EFrontlinePhase::Briefing;
   const bool Created=B->StartNewCampaign(Nation,EFrontlineDifficulty::Veteran);
   if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign()))
   {C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;C->ArmourResearchMaskV63=65535;}
   Check(Nation==EFrontlineFaction::German?TEXT("v8218_german_late_ui_fixture"):TEXT("v8218_british_late_ui_fixture"),Created&&B->SelectCampaignMission(19));
   B->SetActorTickEnabled(false);S.Dossier=1917;S.Technology=Technology;S.TechnologyPage=2;S.TechnologyFocus=INDEX_NONE;S.Deployment=!Technology;S.DeploymentTabV63=1;
   V28ReviewPointer=Technology?FVector2D(1025,818):FVector2D(-10000,-10000);
  };
  double Delay=.72;
  switch(R->Stage)
  {
  case 0:
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}
   FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Prepare(EFrontlineFaction::German);Delay=.1;break;
  case 1:{const int32 ReadyState=Ready(TEXT("v8218_german_real_preparation_ready"));if(ReadyState==0){R->Next=Now+.1;return true;}if(ReadyState<0)return Finish();RunFrontlineCombatChecksV8218(B,Check);FrontlineArmourV63::RunChecksV8218(B,Check);FrontlineCombatRecordV65::RunSourceCodecChecksV8218(Check);Check(TEXT("v8218_configuration_fixture_restores_real_prepared_german_mission"),B->GetEncounterCampaignMission()&&B->GetEncounterCampaignMission()->Id==7&&B->GetPlayerFaction()==EFrontlineFaction::German);if(FrontlineReviewV8218::AdditionalChecks)FrontlineReviewV8218::AdditionalChecks(B,PC,Check);const auto Tanks=FFrontlineCombatV8218Review::ArtFixture(B,false);Check(TEXT("v8218_actual_new_german_art_pair_on_prepared_ground"),Tanks.Num()==2);for(auto* T:Tanks)R->ArtVehicles.Add(T);R->ArtPhase=1;ArtCamera(false);Delay=1.1;break;}
  case 2:if(R->ArtPhase==1){Shot(TEXT("00a-new-german-beute-a7v-paint-real-ground"));R->ArtPhase=2;R->Next=Now+.35;return true;}if(R->ArtPhase==2){FFrontlineCombatV8218Review::ClearArtFixture(B);R->ArtVehicles.Empty();R->ArtPhase=0;FreezeBattle();R->Probe->Enabled=R->Probe->FacilityPointer=true;R->Next=Now+1.1;return true;}Check(TEXT("v8218_actual_canvas_hook_executed"),R->Probe->Done);Shot(TEXT("01-german-facility-hover-en"));break;
  case 3:R->Probe->FacilityPointer=false;V28ReviewPointer=FVector2D(-10000,-10000);break;
  case 4:Shot(TEXT("02-central-medal-rack-no-lower-emblem"));break;
  case 5:Clear();B->Phase=EFrontlinePhase::Briefing;FrontlineAchievementsV76::RunCheatGrantChecksV8218(B,Check);Check(TEXT("v8218_actual_medal_wall_isolated_fixture"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));for(int32 N=0;N<4;++N)MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81(N)),false);S.AchievementsV76=true;MedalWallV8215::View(PC).Wall=true;MedalWallV8215::View(PC).Nation=1;Delay=1.;break;
  case 6:if(!FrontlineUIAssetsV8213::PreloadSettled()){R->Next=Now+.1;return true;}MedalUIReviewV8218::Checks(B,PC,HUD,Check);Shot(TEXT("03-german-tight-medal-wall-en"));break;
  case 7:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);break;
  case 8:Shot(TEXT("04-german-tight-medal-wall-zh"));break;
  case 9:MedalWallV8215::View(PC).Nation=2;break;
  case 10:Shot(TEXT("05-france-unearned-dark-medals-zh"));break;
  case 11:LatePage(EFrontlineFaction::German,true);FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
  case 12:Shot(TEXT("06-german-four-by-four-armour-school-en"));break;
  case 13:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);break;
  case 14:Shot(TEXT("07-german-four-by-four-armour-school-zh"));break;
  case 15:Clear();B->Phase=EFrontlinePhase::Briefing;S.Dossier=1917;S.Deployment=true;S.DeploymentTabV63=1;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
  case 16:Shot(TEXT("08-german-two-platform-configurations-en"));break;
  case 17:Clear();B->Phase=EFrontlinePhase::Briefing;S.Dossier=1917;V28ReviewPointer=FVector2D(1361,104);break;
  case 18:Shot(TEXT("09-german-warroom-ribbon-hover-en"));break;
  case 19:Prepare(EFrontlineFaction::British);Delay=.1;break;
  case 20:{const int32 ReadyState=Ready(TEXT("v8218_british_real_preparation_ready"));if(ReadyState==0){R->Next=Now+.1;return true;}if(ReadyState<0)return Finish();const auto Tanks=FFrontlineCombatV8218Review::ArtFixture(B,true);Check(TEXT("v8218_actual_new_british_art_pair_on_prepared_ground"),Tanks.Num()==2);for(auto* T:Tanks)R->ArtVehicles.Add(T);R->ArtPhase=1;ArtCamera(false);FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);Delay=1.1;break;}
  case 21:if(R->ArtPhase==1){Shot(TEXT("00b-new-british-austin-star-real-ground"));R->ArtPhase=2;R->Next=Now+.35;return true;}if(R->ArtPhase==2){ArtCamera(true);R->ArtPhase=3;R->Next=Now+.8;return true;}if(R->ArtPhase==3){Shot(TEXT("00c-new-austin-twin-turrets-close"));R->ArtPhase=4;R->Next=Now+.35;return true;}if(R->ArtPhase==4){FFrontlineCombatV8218Review::ClearArtFixture(B);R->ArtVehicles.Empty();R->ArtPhase=0;FreezeBattle();R->Probe->Enabled=R->Probe->FacilityPointer=true;R->Next=Now+1.1;return true;}Shot(TEXT("10-british-facility-hover-zh"));break;
  case 22:Clear();B->Phase=EFrontlinePhase::Briefing;S.AchievementsV76=true;MedalWallV8215::View(PC).Wall=true;MedalWallV8215::View(PC).Nation=0;break;
  case 23:Shot(TEXT("11-british-tight-medal-wall-zh"));break;
  case 24:LatePage(EFrontlineFaction::British,true);FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);break;
  case 25:Shot(TEXT("12-british-four-by-four-armour-school-en"));break;
  case 26:FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);break;
  case 27:Shot(TEXT("13-british-four-by-four-armour-school-zh"));break;
  case 28:Clear();B->Phase=EFrontlinePhase::Briefing;S.Dossier=1917;S.Deployment=true;S.DeploymentTabV63=1;break;
  case 29:Shot(TEXT("14-british-five-platforms-zh"));break;
  default:Clear();Check(TEXT("v8218_changed_art_texture_preload_resolved"),FrontlineUIAssetsV8213::PreloadReady());return Finish();
  }
  ++R->Stage;R->Next=Now+Delay;return true;
 }),0.f);
}
}
