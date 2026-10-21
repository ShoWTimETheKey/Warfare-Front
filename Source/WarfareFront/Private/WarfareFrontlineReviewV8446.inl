#include "WarfareFrontlineMedalRibbonV8446.h"
namespace
{
void StartFrontlineReviewV8446(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8446Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FCase{int32 Nation,SeedNation,Seed,Index,Grade,Tier,Bow,Rose,Silver,Gold;bool Inspect,Chinese,Capture;const TCHAR* Name;};
 static constexpr FCase Cases[]={
  {2,2,3,8,5,5,0,1,0,1,false,false,true,TEXT("01-fr-detail-grand-cross")},
  {2,2,3,8,1,5,0,0,0,0,true,false,true,TEXT("02-fr-knight")},
  {2,2,3,8,2,5,0,1,0,0,true,false,true,TEXT("03-fr-officer")},
  {2,2,3,8,3,5,0,1,1,0,true,false,false,TEXT("fr-commander")},
  {2,2,3,8,4,5,0,1,1,1,true,false,true,TEXT("04-fr-grand-officer")},
  {2,2,3,8,5,5,0,1,0,1,true,false,true,TEXT("05-fr-grand-cross")},
  {3,3,3,9,1,4,0,0,0,0,true,false,true,TEXT("06-ru-fourth")},
  {3,3,3,9,2,4,1,0,0,0,true,false,true,TEXT("07-ru-third")},
  {3,3,3,9,3,4,0,0,0,0,true,false,false,TEXT("ru-second")},
  {3,3,3,9,4,4,1,0,0,0,true,false,false,TEXT("ru-first")},
  {1,1,2,3,1,3,0,0,0,0,true,false,false,TEXT("de-iron-cross-second")},
  {1,1,2,3,2,3,0,0,0,0,true,true,true,TEXT("08-de-iron-cross-first-no-bar")},
  {1,1,2,3,3,3,0,0,0,0,true,false,false,TEXT("de-iron-cross-grand")},
  {0,0,3,14,1,1,0,0,0,0,false,false,true,TEXT("09-uk-1914-star-rosette")},
  {2,1,0,8,3,0,0,1,1,0,true,true,true,TEXT("10-fr-locked-commander")},
  {3,1,0,10,2,0,1,0,0,0,true,true,true,TEXT("11-ru-locked-third-cross")}
 };
 struct FRun{FString Dir;int32 Stage=0,Page=0,Captures=0,SeedNation=INDEX_NONE,Seed=INDEX_NONE;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+.5;uint64 Frame=0;TArray<TSharedPtr<FJsonValue>> Checks,Images;};
 auto Run=MakeShared<FRun>();Run->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString S;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(Run->Dir/Name));};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Run->Good);J->SetArrayField(TEXT("checks"),Run->Checks);J->SetArrayField(TEXT("actual_device_draws"),Run->Images);J->SetNumberField(TEXT("captures_requested"),Run->Captures);J->SetNumberField(TEXT("wall_seconds"),Now-Run->Began);J->SetStringField(TEXT("scope"),TEXT("Disposable account display overlay; pure class/rack rules and actual HUD click, selected-ribbon texture/device draw placements and 4K captures. No campaign, gameplay, player record or performance acceptance."));Write(TEXT("checks.json"),J);V27ImageAuditActive=false;FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-Run->Began<35)return true;Check(TEXT("v8446_world_ready"),false);return Finish();}
  if(Now-Run->Began>180){Check(TEXT("v8446_bounded_review"),false);return Finish();}
  if(Now<Run->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& Screen=ScreenState(PC);
  if(Run->Stage==0)
  {
   Check(TEXT("v8446_disposable_ukde_host_no_national_campaign"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetHDREnabled(false,false);R->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);}
   Check(TEXT("v8446_temporary_german_host"),B->StartNewCampaign(EFrontlineFaction::German));B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);
   FrontlineMedalRibbonV8446::RunChecks(Check);MedalWallV8215::RunRibbonUIChecksV8444(Check);
   MedalWallV8215::PreloadVariants({3,8,9,10,14},true);if(!Run->Good)return Finish();Run->Stage=1;Run->Next=Now+1.;return true;
  }
  if(Run->Stage==1)
  {
   Screen.Menu=Screen.Help=Screen.Settings=Screen.Technology=Screen.Deployment=Screen.UnitIntroduction=Screen.HistoricalDispatch=Screen.CampaignEndingV51=Screen.Credits=Screen.ExitConfirmV61=false;
   Screen.AchievementsV76=true;Screen.PhotoTransitionV37=Screen.SmallTransitionV35=false;Screen.FactionSelectionV82=FFrontlineFactionSelectionV82();Screen.Dossier=1917;Screen.bPausedByOverlay=Screen.bPausedForAuxOverlay=false;
   UGameplayStatics::SetGamePaused(PC,false);B->Phase=EFrontlinePhase::Briefing;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Inspect=false;
   const auto& C=Cases[Run->Page];
   if(Run->SeedNation!=C.SeedNation||Run->Seed!=C.Seed){Check(FString(C.Name)+TEXT("_exact_disposable_account_seed"),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81(C.SeedNation),C.Seed));Run->SeedNation=C.SeedNation;Run->Seed=C.Seed;}
   FrontlineLocalizationV35::SetLanguage(C.Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   PC->GetHUD()->NotifyHitBoxClick(FName(*FString::Printf(TEXT("V8215MedalNation%d"),C.Nation)));
   PC->GetHUD()->NotifyHitBoxClick(FName(*FString::Printf(TEXT("V8216MedalSelect%d"),C.Index)));
   if(C.Inspect){PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));for(int32 N=0;M.PreviewGrade!=C.Grade&&N<10;++N)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalVariantPrev"));}
   Check(FString(C.Name)+TEXT("_actual_selection_and_class_clicks"),M.Nation==C.Nation&&M.Focus==C.Index&&M.Inspect==C.Inspect&&(!C.Inspect||M.PreviewGrade==C.Grade));
   Check(FString(C.Name)+TEXT("_preview_preserves_actual_award_class"),FrontlineMedalsV8215::Tier(C.Index)==C.Tier);
   if(!Run->Good)return Finish();V27ImageAuditActive=true;V27ImageAudit.Empty();V28ReviewPointer=FVector2D(-10000,-10000);Run->Frame=GFrameCounter;Run->Stage=2;Run->Next=Now+.55;return true;
  }
  if(Run->Stage==2)
  {
   if(GFrameCounter<=Run->Frame+2)return true;const auto& C=Cases[Run->Page];int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);Check(TEXT("v8446_actual_4k_output"),Width==3840&&Height==2160);
   const float Y=C.Inspect?841.f:612.f,H=C.Inspect?56.f:43.f;int32 Atlas=0,Bow=0,Rose=0,Silver=0,Gold=0,SilverRosette=0;
   auto Page=MakeShared<FJsonObject>();Page->SetStringField(TEXT("page"),C.Name);TArray<TSharedPtr<FJsonValue>> Draws;
   for(const auto& Pair:V27ImageAudit)
   {
    const auto& J=Pair.Value;const float DY=J->GetNumberField(TEXT("draw_y")),DH=J->GetNumberField(TEXT("draw_height"));
    if(DY<Y-.01f||DY+DH>Y+H+.01f)continue;const FString Path=J->GetStringField(TEXT("texture"));
    if(Path.Contains(TEXT("T_RibbonAtlas_"))&&FMath::IsNearlyEqual(DY,Y,.01f))++Atlas;
    if(Path.Contains(TEXT("T_StGeorgeBowV8221")))++Bow;
    if(Path.Contains(TEXT("legion_d_honneur_V8216_officier"))&&FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("uv_width"))),360.f/1024,.00001f))++Rose;
    if(Path.Contains(TEXT("T_MedalAttachment_SilverBar_V8216"))&&FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("uv_height"))),37.f/724,.00001f))++Silver;
    if(Path.Contains(TEXT("T_MedalAttachment_GoldBar_V8216"))&&FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("uv_height"))),37.f/836,.00001f))++Gold;
    if(Path.Contains(TEXT("T_MedalAttachment_SilverRosette_V8216")))++SilverRosette;
    if(Path.Contains(TEXT("T_RibbonAtlas_"))||Path.Contains(TEXT("Attachment"))||Path.Contains(TEXT("BowV8221"))||Path.Contains(TEXT("officier")))Draws.Add(MakeShared<FJsonValueObject>(J));
   }
   const int32 ExpectedAtlas=C.Index==3&&C.Grade==2?0:1;
   Check(FString(C.Name)+TEXT("_selected_textile_presence"),Atlas==ExpectedAtlas);
   Check(FString(C.Name)+TEXT("_actual_selected_class_device_draws"),Bow==C.Bow&&Rose==C.Rose&&Silver==C.Silver&&Gold==C.Gold);
   if(C.Index==14)Check(TEXT("v8446_1914_star_recorded_clasp_draws_one_ribbon_rosette"),SilverRosette==1);
   Check(FString(C.Name)+TEXT("_no_fr_ru_campaign_unlock"),!WarfareFRRUCampaignV82::Active(B)
    &&!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French)&&!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian)
    &&!FrontlineMedalsV8215::Available(EFRRUNationV81::French)&&!FrontlineMedalsV8215::Available(EFRRUNationV81::Russian));
   Page->SetArrayField(TEXT("placements"),Draws);Run->Images.Add(MakeShared<FJsonValueObject>(Page));if(!Run->Good)return Finish();
   if(C.Capture){FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/(FString(C.Name)+TEXT(".png")),true,false);++Run->Captures;}
   ++Run->Page;Run->Stage=Run->Page<UE_ARRAY_COUNT(Cases)?1:3;Run->Next=Now+.1;return true;
  }
  if(Run->Stage==3){TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Run->Dir/TEXT("Captures/*.png")),true,false);bool Written=Files.Num()==Run->Captures;for(const auto& F:Files)Written&=IFileManager::Get().FileSize(*(Run->Dir/TEXT("Captures")/F))>1024;Check(TEXT("v8446_targeted_actual_ui_captures_written"),Written&&Run->Captures==11);return Finish();}
  Check(TEXT("v8446_known_stage"),false);return Finish();
 }),0.f);
}
}
