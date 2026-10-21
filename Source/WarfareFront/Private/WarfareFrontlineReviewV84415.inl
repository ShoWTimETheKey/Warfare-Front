#include "WarfareFrontlineEnemyPolicyV84415.h"
namespace
{
void StartFrontlineReviewV84415(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Page=0,Captures=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0,Next=0;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V84415_CHECK %s %d"),*Name,int32(Pass));};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  auto Result=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Captures);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R->Began);J->SetStringField(TEXT("scope"),TEXT("V84.4.15 UK/DE enemy eligibility and production decision fixtures, achievement feasibility and legal honours preview/placement; eight affected bilingual 4K captures. Disposable account only. No FR/RU campaign, exhaustive campaign win-rate, HDR/FG or concurrent-game performance claim."));return J;};
  auto Finish=[&](){Write(TEXT("checks.json"),Result());V28ReviewPointer.Reset();FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay())return true;
  if(Now-R->Began>270.){Check(TEXT("v84415_bounded_runtime"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<35)return true;
   Check(TEXT("v84415_resident_core_before_draw"),FrontlineUIAssetsV8213::PreloadReady());if(!R->Good)return Finish();Clear();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
   Check(TEXT("v84415_disposable_british_campaign_only"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v84415_production_terrain_selected"),B->SelectCampaignMission(17));if(!R->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<80)return true;
   Check(TEXT("v84415_production_terrain_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   MedalWallV8215::RunPresentationChecksV84415(Check);
   RunFrontlineEnemyChecksV84415(B,Check);
   FrontlineAchievementsV76::RunChallengeChecksV84415(B,Check);
   Write(TEXT("challenge-feasibility.json"),FrontlineAchievementsV76::RunChallengeFeasibilityV84415(B,Check));
   MedalWallV8215::RunInspectionUIChecksV84415(PC,Check);
   // Independent presentation checks still run when a bounded combat probe
   // fails; the final result retains every failure and cannot become a pass.
   Write(TEXT("checks-checkpoint.json"),Result());R->Stage=2;return true;
  }
  struct FShot{const TCHAR* Name;int32 Nation,Medal,Grade,Repeats;bool Inspect,ZH;};
  static const FShot Shots[]={
   {TEXT("01-dsc-four-bars-detail-zh.png"),0,37,1,4,false,true},
   {TEXT("02-dsc-four-bars-inspect-en.png"),0,37,1,4,true,false},
   {TEXT("03-dso-four-bars-inspect-zh.png"),0,12,1,4,true,true},
   {TEXT("04-dfm-eight-bars-inspect-en.png"),0,39,1,8,true,false},
   {TEXT("05-dsc-maximum-repeat-inspect-zh.png"),0,37,1,255,true,true},
   {TEXT("06-victoria-cross-single-bar-inspect-en.png"),0,2,1,1,true,false},
   {TEXT("07-unearned-french-highest-class-zh.png"),2,8,5,0,false,true},
   {TEXT("08-unearned-german-highest-class-en.png"),1,3,3,0,false,false}};
  if(R->Stage==2)
  {
   if(R->Page>=UE_ARRAY_COUNT(Shots)){TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(R->Dir/TEXT("Captures/*.png")),true,false);Check(TEXT("v84415_eight_affected_captures_written"),Files.Num()==UE_ARRAY_COUNT(Shots)&&R->Captures==UE_ARRAY_COUNT(Shots));return Finish();}
   Clear();B->Phase=EFrontlinePhase::Briefing;PC->SetActorTickEnabled(true);const auto& Shot=Shots[R->Page];
   Check(TEXT("v84415_disposable_display_fixture"),R->Page>=6?FrontlineAchievementsV76::SeedMedalReviewV8215(B,false,0):FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81(Shot.Nation),3));
   const auto Indices=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Shot.Nation));MedalWallV8215::PreloadVariants(Indices,false);
   S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Shot.Nation;M.Inspect=false;M.Focus=Shot.Medal;
   Check(FString(Shot.Name)+TEXT("_preview_access"),MedalWallV8215::SelectInspectionStateV84413(PC,Shot.Medal,Shot.Grade,Shot.Repeats,Shot.Repeats?uint16(FrontlineMedalsV8215::EAttachment::Bar):0));
   if(Shot.Inspect)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));
   FrontlineLocalizationV35::SetLanguage(Shot.ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);R->Stage=3;R->Next=Now+1.;return true;
  }
  if(R->Stage==3)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<255)return true;
   int32 W=0,H=0;PC->GetViewportSize(W,H);Check(FString(Shots[R->Page].Name)+TEXT("_4k"),W==3840&&H==2160);if(W!=3840||H!=2160)return Finish();
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/Shots[R->Page].Name,true,false);++R->Captures;++R->Page;R->Stage=2;R->Next=Now+.1;return true;
  }
  return true;
 }));
}
}
