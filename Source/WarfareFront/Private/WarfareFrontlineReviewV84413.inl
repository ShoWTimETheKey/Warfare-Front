TSharedRef<FJsonObject> FrontlineRunReadinessChecksV84413(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
namespace FrontlineMedalsV8215 { void RunInspectionChecksV84413(TFunctionRef<void(const FString&,bool)>); }
namespace
{
void StartFrontlineReviewV84413(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Page=0,Captures=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0,Next=0;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  auto Finish=[&](){if(B&&FrontlineUltimateArchiveV8215::Active(B))FrontlineUltimateArchiveV8215::Leave(B);auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Captures);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V84.4.13 UK/DE actual readiness/deployment and movement fixtures; medal award-state readback and actual UI selection, canonical dossier maps and modest armour-colour correction. Eight bilingual 4K affected captures. No real account, FR/RU campaign, full natural campaign, HDR/FG or performance guarantee."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay())return true;
  if(Now-R->Began>210.){Check(TEXT("v84413_bounded_runtime"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<35)return true;
   Check(TEXT("v84413_resident_core_before_draw"),FrontlineUIAssetsV8213::PreloadReady());if(!R->Good)return Finish();Clear();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
   Check(TEXT("v84413_disposable_british_campaign_only"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v84413_actual_late_terrain_selected"),B->SelectCampaignMission(17));if(!R->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<75)return true;
   Check(TEXT("v84413_actual_terrain_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   Write(TEXT("readiness.json"),FrontlineRunReadinessChecksV84413(B,Check));if(!R->Good)return Finish();
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v84413_actual_fleet_exists"),Fleet!=nullptr);if(!R->Good)return Finish();
   TSharedPtr<FJsonObject> Motion;
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{Motion=Fleet->RunMovementChecksV84413();});});});
   Check(TEXT("v84413_scoped_actual_movement_was_executed"),Motion.IsValid());if(!R->Good)return Finish();Write(TEXT("movement.json"),Motion.ToSharedRef());
   for(const auto& Value:Motion->GetArrayField(TEXT("checks"))){auto J=Value->AsObject();if(J.IsValid())Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
   Check(TEXT("v84413_actual_movement_cases_pass"),Motion->GetBoolField(TEXT("success")));
   FrontlineMedalsV8215::RunInspectionChecksV84413(Check);MedalWallV8215::RunInspectionUIChecksV84413(PC,Check);
   Check(TEXT("v84413_actual_chroma_material_not_fallback"),Artwork().ArchivePhotoMaterialV72&&Artwork().ArchivePhotoMaterialV72->GetName()==TEXT("M_ResearchPhoto_V84413"));
   FrontlineLoadingPageV82::RunMapChecksV8447(Check);R->Stage=2;return true;
  }
  const TCHAR* Shots[]={TEXT("01-armour-research-zh.png"),TEXT("02-armour-research-en.png"),TEXT("03-ground-early-zh.png"),TEXT("04-ground-late-en.png"),TEXT("05-dsc-initial-wall-zh.png"),TEXT("06-dsc-two-bars-inspect-zh.png"),TEXT("07-plm-initial-wall-en.png"),TEXT("08-plm-oakleaves-inspect-en.png")};
  if(R->Stage==2)
  {
   if(R->Page>=UE_ARRAY_COUNT(Shots)){TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(R->Dir/TEXT("Captures/*.png")),true,false);Check(TEXT("v84413_eight_affected_captures_written"),Files.Num()==8&&R->Captures==8);return Finish();}
   Clear();B->Phase=EFrontlinePhase::Briefing;PC->SetActorTickEnabled(true);
   if(R->Page<4)
   {
    const bool DE=R->Page==1||R->Page==3;
    Check(TEXT("v84413_capture_disposable_campaign"),B->StartNewCampaign(DE?EFrontlineFaction::German:EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
    auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(P){P->CompletedMissions=19;P->WonOperationMaskV52=(1u<<19)-1;P->DispatchResolvedMaskV52=(1u<<20)-1;P->ArmourResearchMaskV63=0x5555u|(1u<<8);}
    const int32 Mission=R->Page==2?0:17;Check(TEXT("v84413_capture_operation_selected"),B->SelectCampaignMission(Mission));B->Phase=EFrontlinePhase::Briefing;
    FrontlineLocalizationV35::SetLanguage(DE?FrontlineLocalizationV35::ELanguage::English:FrontlineLocalizationV35::ELanguage::Chinese);
    if(R->Page<2){S.Technology=true;S.TechnologyPage=2;S.TechnologyFocus=INDEX_NONE;}
    else{auto& W=WarRoom52::State(PC);WarRoom52::Synchronize(W,B);W.Preview=Mission;W.Tab=1;Check(TEXT("v84413_ground_map_resident_canonical"),FrontlineTextureReady(FrontlineLoadingPageV82::OperationMapV8447(B,B->GetSelectedCampaignMission())));}
   }
   else
   {
    const bool DE=R->Page>=6;const int32 Medal=DE?5:37;const bool Inspect=(R->Page%2)==1;
    Check(TEXT("v84413_medal_display_fixture_only"),FrontlineAchievementsV76::SeedRibbonReviewV8443(B,DE?EFRRUNationV81::German:EFRRUNationV81::British,3));
    MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(DE?EFRRUNationV81::German:EFRRUNationV81::British),false);
    S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=DE?1:0;M.Focus=Medal;M.Inspect=false;
    Check(TEXT("v84413_medal_preview_uses_exact_award_state"),MedalWallV8215::SelectInspectionStateV84413(PC,Medal,1,Inspect&&!DE?2:0,Inspect?uint16(DE?FrontlineMedalsV8215::EAttachment::OakLeaves:FrontlineMedalsV8215::EAttachment::Bar):0));
    if(Inspect)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));
    FrontlineLocalizationV35::SetLanguage(DE?FrontlineLocalizationV35::ELanguage::English:FrontlineLocalizationV35::ELanguage::Chinese);
   }
   R->Stage=3;R->Next=Now+((R->Page==2||R->Page==3)?4.:1.);return true;
  }
  if(R->Stage==3)
  {
   if(R->Page==2||R->Page==3){const auto& W=WarRoom52::State(PC);Check(TEXT("v84413_ground_actual_operation_dossier_open"),W.AtlasMode==2&&W.OperationSheet&&W.Tab==1&&W.FlightDuration<=0.);}
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<195)return true;
   int32 W=0,H=0;PC->GetViewportSize(W,H);Check(FString(Shots[R->Page])+TEXT("_4k"),W==3840&&H==2160);if(W!=3840||H!=2160)return Finish();
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/Shots[R->Page],true,false);++R->Captures;++R->Page;R->Stage=2;R->Next=Now+.1;return true;
  }
  return true;
 }));
}
}
