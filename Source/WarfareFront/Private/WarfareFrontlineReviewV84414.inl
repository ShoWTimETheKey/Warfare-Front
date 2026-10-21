namespace FrontlineMedalsV8215 { void RunAwardChecksV84414(TFunctionRef<void(const FString&,bool)>); }
namespace
{
void StartFrontlineReviewV84414(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV84414Focused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Page=0,Captures=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0,Next=0;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Captures);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V84.4.14 actual UK/DE armour contact fixtures and production medal award rules; six affected bilingual 4K honours captures. Disposable account only. No FR/RU campaign, full campaign, HDR/FG or concurrent-game performance claim."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay())return true;
  if(Now-R->Began>230.){Check(TEXT("v84414_bounded_runtime"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<35)return true;
   Check(TEXT("v84414_resident_core_before_draw"),FrontlineUIAssetsV8213::PreloadReady());if(!R->Good)return Finish();Clear();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
   Check(TEXT("v84414_disposable_british_campaign_only"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v84414_actual_terrain_selected"),B->SelectCampaignMission(17));if(!R->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<80)return true;
   Check(TEXT("v84414_actual_terrain_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v84414_actual_fleet_exists"),Fleet!=nullptr);if(!R->Good)return Finish();
   TSharedPtr<FJsonObject> Contact;
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{Contact=Fleet->RunContactChecksV84414();});});});
   Check(TEXT("v84414_actual_contact_executed"),Contact.IsValid());if(!R->Good)return Finish();Write(TEXT("contact.json"),Contact.ToSharedRef());
   for(const auto& Value:Contact->GetArrayField(TEXT("checks"))){auto J=Value->AsObject();if(J.IsValid())Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
   Check(TEXT("v84414_actual_contact_cases_pass"),Contact->GetBoolField(TEXT("success")));
   FrontlineMedalsV8215::RunAwardChecksV84414(Check);
   FrontlineAchievementsV76::RunAwardReceiptChecksV84414(B,Check);
   for(int32 I=37;I<FrontlineMedalsV8215::Count;++I)if(FrontlineMedalsV8215::Definition(I).Ribbon)
   {const FString Stem=FString::Printf(TEXT("T_Ribbon_%d_V84414"),I);auto* T=FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V84414/Honours/")+Stem+TEXT(".")+Stem);Check(Stem+TEXT("_resident_original_extent"),T&&T->GetSizeX()==768&&T->GetSizeY()==192);}
   R->Stage=2;return true;
  }
  const TCHAR* Shots[]={TEXT("01-british-wall-zh.png"),TEXT("02-german-wall-en.png"),TEXT("03-dsc-inspect-zh.png"),TEXT("04-dfm-inspect-en.png"),TEXT("05-french-detail-zh.png"),TEXT("06-russian-detail-en.png")};
  if(R->Stage==2)
  {
   if(R->Page>=UE_ARRAY_COUNT(Shots)){TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(R->Dir/TEXT("Captures/*.png")),true,false);Check(TEXT("v84414_six_affected_captures_written"),Files.Num()==6&&R->Captures==6);return Finish();}
   Clear();B->Phase=EFrontlinePhase::Briefing;PC->SetActorTickEnabled(true);
   const int32 Nation=R->Page==1?1:R->Page>=4?R->Page-2:0;
   Check(TEXT("v84414_disposable_display_fixture"),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,EFRRUNationV81(Nation),3));
   const auto Indices=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Nation));MedalWallV8215::PreloadVariants(Indices,false);
   S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Nation;M.Inspect=false;
   const int32 Focus=R->Page==2?37:R->Page==3?39:Nation==1?42:Nation==2?46:Nation==3?50:37;
   M.Focus=Focus;Check(TEXT("v84414_matching_award_preview"),MedalWallV8215::SelectInspectionStateV84413(PC,Focus,1,R->Page==2?2:R->Page==3?1:0,R->Page==2||R->Page==3?uint16(FrontlineMedalsV8215::EAttachment::Bar):0));
   if(R->Page==2||R->Page==3)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));
   FrontlineLocalizationV35::SetLanguage(R->Page%2==0?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   R->Stage=3;R->Next=Now+1.;return true;
  }
  if(R->Stage==3)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<215)return true;
   int32 W=0,H=0;PC->GetViewportSize(W,H);Check(FString(Shots[R->Page])+TEXT("_4k"),W==3840&&H==2160);if(W!=3840||H!=2160)return Finish();
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/Shots[R->Page],true,false);++R->Captures;++R->Page;R->Stage=2;R->Next=Now+.1;return true;
  }
  return true;
 }));
}
}
