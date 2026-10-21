#include "WarfareFrontlineTrenchMotionV8444Checks.h"
namespace
{
void StartFrontlineReviewV8444(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Page=0,Shots=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+1,Prepared=0;uint64 Frame=0;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto Run=MakeShared<FRun>();Run->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Good&=Passed;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(Run->Dir/Name));};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Run->Good);J->SetArrayField(TEXT("checks"),Run->Checks);J->SetNumberField(TEXT("captures_requested"),Run->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-Run->Began);J->SetStringField(TEXT("scope"),TEXT("Disposable UK/DE trench collision, production-actor firing/movement and existing medal UI. Synthetic fixtures, not complete campaign or sustained performance acceptance; paused campaigns unchanged."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-Run->Began<35)return true;Check(TEXT("v8444_world_ready"),false);return Finish();}
  if(Now-Run->Began>180){Check(TEXT("v8444_bounded_review"),false);return Finish();}
  if(Now<Run->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& Screen=ScreenState(PC);
  auto Clear=[&](){Screen.Menu=Screen.Help=Screen.Settings=Screen.Technology=Screen.AchievementsV76=Screen.Deployment=Screen.UnitIntroduction=Screen.HistoricalDispatch=Screen.CampaignEndingV51=Screen.Credits=Screen.ExitConfirmV61=false;Screen.bPausedByOverlay=Screen.bPausedForAuxOverlay=false;Screen.PhotoTransitionV37=Screen.SmallTransitionV35=false;Screen.FactionSelectionV82=FFrontlineFactionSelectionV82();Screen.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::CloseV836(PC);UltimateArchiveUIV8215::Close(PC);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);UGameplayStatics::SetGamePaused(PC,false);};
  if(Run->Stage==0)
  {
   Check(TEXT("v8444_disposable_non_national_world"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));if(!Run->Good)return Finish();
   Clear();if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);}
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV8444UIOnly"))){Run->Stage=3;return true;}
   Check(TEXT("v8444_british_physical_test_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v8444_existing_operation_geometry_selected"),B->SelectCampaignMission(17));if(!Run->Good)return Finish();Screen.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);Run->Prepared=Now;Run->Stage=1;Run->Next=Now+.1;return true;
  }
  if(Run->Stage==1)
  {
   if(Screen.Preparing&&Screen.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-Run->Prepared<75){Run->Next=Now+.1;return true;}
   Check(TEXT("v8444_actual_map_prepared"),B->Phase==EFrontlinePhase::Fighting&&!Screen.Preparing&&B->AssetFailure.IsEmpty()&&Screen.PreparationError.IsEmpty());if(!Run->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& Unit:B->Units)if(IsValid(Unit))Unit->SetActorTickEnabled(false);Run->Stage=2;Run->Next=Now+.1;return true;
  }
  if(Run->Stage==2)
  {
   Write(TEXT("trench-fire.json"),FrontlineInfantryV45::RunTrenchFireChecksV844(B,Check));
   Write(TEXT("trench-motion.json"),FrontlineTrenchMotionV8444::RunChecks(B,Check));
   if(!Run->Good)return Finish();Run->Stage=3;Run->Next=FPlatformTime::Seconds()+.1;return true;
  }
  if(Run->Stage==3)
  {
   B->AbandonBattleToOperations();Check(TEXT("v8444_return_to_empty_operations_fixture"),B->Phase==EFrontlinePhase::Briefing&&B->Units.IsEmpty());Clear();Check(TEXT("v8444_german_ui_campaign_only"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);
   Check(TEXT("v8444_exact_disposable_ribbon_seed"),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81::German,2));MedalWallV8215::RunRibbonUIChecksV8444(Check);MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81::German),true);
   if(!Run->Good)return Finish();Run->Stage=4;Run->Next=Now+.5;return true;
  }
  if(Run->Stage==4)
  {
   Clear();V27ImageAuditActive=true;V27ImageAudit.Empty();FrontlineLocalizationV35::SetLanguage(Run->Page>=4?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);B->Phase=EFrontlinePhase::Briefing;
   const auto Command=MedalWallV8215::WarRoomRibbonNameV8444(EFRRUNationV81::German,18);
   auto& WarRoom=WarRoom52::State(PC);WarRoom52::Synchronize(WarRoom,B);WarRoom52::BeginRoute(WarRoom,B);WarRoom.WasActive=true;WarRoom.FlightStart=Now-10;WarRoom52::AtlasTick(WarRoom,Now);WarRoom.Tab=0;PageCanvasV73::Audit=true;PageCanvasV73::Hits.Empty();
   if(Run->Page==0){const auto Items=MedalWallV8215::OrderedRibbonItems(EFRRUNationV81::German);const auto Cells=FrontlineRibbonRackV8222::Layout(Items.Num(),52,79,240,42,false);const int32 Slot=Items.Find(18);Check(TEXT("v8444_real_operations_ribbon_present"),Cells.IsValidIndex(Slot));if(Cells.IsValidIndex(Slot))V28ReviewPointer=Cells[Slot].GetCenter();}
   else
   {
    PC->GetHUD()->NotifyHitBoxClick(Command);auto& M=MedalWallV8215::View(PC);Check(FString::Printf(TEXT("v8444_page%d_actual_ribbon_click_opens_correct_wall"),Run->Page),Screen.AchievementsV76&&M.Wall&&M.Nation==1&&M.Focus==18);
    if(Run->Page==2||Run->Page==3||Run->Page==5){PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));if(Run->Page==3)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalVariantPrev"));}
   }
   if(!Run->Good)return Finish();Run->Frame=GFrameCounter;Run->Stage=5;Run->Next=Now+.6;return true;
  }
  if(Run->Stage==5)
  {
   if(GFrameCounter<=Run->Frame+2)return true;int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);Check(TEXT("v8444_actual_4k_ui_output"),Width==3840&&Height==2160);
   if(Run->Page==0){int32 Hits=0;for(const auto& Hit:PageCanvasV73::Hits)if(Hit.Value->GetStringField(TEXT("command")).StartsWith(TEXT("V8444WarRoomMedalWall")))++Hits;Check(TEXT("v8444_seven_visible_ribbons_register_seven_navigation_hits"),Hits==7);}
   const bool Inspect=Run->Page==2||Run->Page==3||Run->Page==5;
   if(Inspect)Check(TEXT("v8444_actual_inspect_is_open_with_expected_class"),MedalWallV8215::View(PC).Inspect&&MedalWallV8215::View(PC).PreviewGrade==(Run->Page==3?2:3)&&FrontlineMedalsV8215::Tier(18)==3);
   if(Run->Page>0)
   {
    int32 Drawn=0;for(const auto& Placement:V27ImageAudit){const auto& Image=Placement.Value;if(Image->GetStringField(TEXT("texture")).Contains(TEXT("T_RibbonAtlas_de_V8217"))&&FMath::IsNearlyEqual(float(Image->GetNumberField(TEXT("draw_y"))),Inspect?841.f:612.f,.01f)&&FMath::IsNearlyEqual(float(Image->GetNumberField(TEXT("draw_width"))),Inspect?224.f:172.f,.01f)&&FMath::IsNearlyEqual(float(Image->GetNumberField(TEXT("draw_height"))),Inspect?56.f:43.f,.01f))++Drawn;}
    Check(FString::Printf(TEXT("v8444_page%d_corresponding_ribbon_was_actually_drawn"),Run->Page),Drawn==1);
   }
   if(!Run->Good)return Finish();static const TCHAR* Names[]={TEXT("01-warroom-ribbon-link-en"),TEXT("02-wall-ribbon-detail-en"),TEXT("03-inspect-high-class-ribbon-en"),TEXT("04-inspect-lower-class-ribbon-en"),TEXT("05-wall-ribbon-detail-zh"),TEXT("06-inspect-ribbon-zh")};FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/(FString(Names[Run->Page])+TEXT(".png")),true,false);++Run->Shots;++Run->Page;Run->Stage=Run->Page<6?4:6;Run->Next=Now+.1;return true;
  }
  if(Run->Stage==6){int32 Files=0;TArray<FString> Captures;IFileManager::Get().FindFiles(Captures,*(Run->Dir/TEXT("Captures/*.png")),true,false);for(const auto& Name:Captures)if(IFileManager::Get().FileSize(*(Run->Dir/TEXT("Captures")/Name))>1024)++Files;Check(TEXT("v8444_all_targeted_ui_captures_written"),Run->Shots==6&&Files==6);V27ImageAuditActive=false;return Finish();}
  Check(TEXT("v8444_known_review_stage"),false);return Finish();
 }),0.f);
}
}
