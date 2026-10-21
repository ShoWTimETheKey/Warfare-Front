TSharedRef<FJsonObject> RunFrontlineArmourApproachV85Review(UFrontlineVehicleSubsystem* Fleet);
TSharedRef<FJsonObject> RunFrontlineArchiveArmourV85Review(UFrontlineVehicleSubsystem* Fleet);
#include "WarfareFrontlineVisualV85.h"
bool PrepareFrontlineGasVisualV836(AWarfareFrontlineGameMode* B,const FVector& P);
namespace
{
void CheckMoraleAndArchiveV85(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 for(int32 Nation=0;Nation<2;++Nation)for(int32 Difficulty=0;Difficulty<3;++Difficulty)
 {
  Check(TEXT("v85_disposable_campaign_start"),B->StartNewCampaign(EFrontlineFaction(Nation),EFrontlineDifficulty(Difficulty)));
  if(auto* Run=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Run->CompletedMissions=B->GetCampaignMissions().Num();Run->WonOperationMaskV52=(1u<<20)-1;Run->DispatchResolvedMaskV52=(1u<<20)-1;}
  for(int32 Mission=0;Mission<B->GetCampaignMissions().Num();++Mission)
  {
   const bool Selected=B->SelectCampaignMission(Mission);const auto* M=B->GetSelectedCampaignMission();
   const auto Capacity=[&](float Scale){return M&&FMath::IsNearlyEqual(B->Morale[0],M->PlayerMorale*Scale,.001f)&&FMath::IsNearlyEqual(B->Morale[1],M->EnemyMorale*Scale,.001f)
     &&FMath::IsNearlyEqual(FrontlineMoraleV36::StartingMaximum(B,0),B->Morale[0],.001f)&&FMath::IsNearlyEqual(FrontlineMoraleV36::StartingMaximum(B,1),B->Morale[1],.001f);};
   const bool Initial=Selected&&Capacity(1.25f);B->Phase=EFrontlinePhase::Fighting;B->Morale[0]*=.40f;B->Morale[1]*=.65f;const bool Retried=B->RestartBattle();
   Check(FString::Printf(TEXT("v85_campaign_%d_%d_%d_capacity_both_sides_retry_no_stack"),Nation,Difficulty,Mission),Initial&&Retried&&Capacity(1.25f));
  }
 }
 Check(TEXT("v85_archive_qualification_disposable_only"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,0));
 for(int32 Nation=0;Nation<2;++Nation)for(int32 Difficulty=0;Difficulty<3;++Difficulty)
  for(int32 Mission=0;Mission<B->GetCampaignMissionsForFaction(EFrontlineFaction(Nation)).Num();++Mission)
  {
   const bool Entered=FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction(Nation),Mission,EFrontlineDifficulty(Difficulty));
   const auto* M=B->GetSelectedCampaignMission();
   const auto Capacity=[&](){return M&&FMath::IsNearlyEqual(B->Morale[0],M->PlayerMorale*1.50f,.001f)&&FMath::IsNearlyEqual(B->Morale[1],M->EnemyMorale*1.50f,.001f)
      &&FMath::IsNearlyEqual(FrontlineMoraleV36::StartingMaximum(B,0),B->Morale[0],.001f)&&FMath::IsNearlyEqual(FrontlineMoraleV36::StartingMaximum(B,1),B->Morale[1],.001f);};
   const bool Initial=Entered&&Capacity();if(Entered){B->Phase=EFrontlinePhase::Fighting;B->Morale[0]*=.40f;B->Morale[1]*=.65f;}const bool Retried=Entered&&B->RestartBattle();
   Check(FString::Printf(TEXT("v85_archive_%d_%d_%d_capacity_both_sides_retry_no_stack"),Nation,Difficulty,Mission),Initial&&Retried&&Capacity());
   FrontlineUltimateArchiveV8215::Leave(B);
   const auto* Restored=B->GetSelectedCampaignMission();
   Check(TEXT("v85_archive_leave_restores_ordinary_capacity"),Restored&&FMath::IsNearlyEqual(B->Morale[0],Restored->PlayerMorale*1.25f,.001f)&&FMath::IsNearlyEqual(B->Morale[1],Restored->EnemyMorale*1.25f,.001f));
  }
 Check(TEXT("v85_return_original_campaign_start"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
 const auto* Original=B->GetCurrentCampaign();const FGuid OriginalId=Original?Original->Id:FGuid();
 for(int32 Route=0;Route<2;++Route)
 {
  UltimateArchiveUIV8215::Close(PC);auto& State=ScreenState(PC);State.Menu=State.Help=State.Settings=State.Technology=State.AchievementsV76=State.Deployment=State.Preparing=false;
  Check(TEXT("v85_return_replay_german_op17"),FrontlineUltimateArchiveV8215::Begin(B,EFrontlineFaction::German,17,EFrontlineDifficulty::Survivor));
  B->Phase=Route?EFrontlinePhase::Defeat:EFrontlinePhase::Victory;
  if(Route)PC->BeginBattle();else PC->GetHUD()->NotifyHitBoxClick(TEXT("CampaignAfterBattle"));
  const bool Returned=!FrontlineUltimateArchiveV8215::Active(B);const auto& View=UltimateArchiveUIV8215::View(PC);
  Check(Route?TEXT("v85_repeated_archive_return_retains_selection"):TEXT("v85_archive_return_selector_retains_different_faction"),Returned&&View.Open&&View.Faction==EFrontlineFaction::German&&View.Mission==17&&View.Difficulty==2
   &&!FrontlineUltimateArchiveV8215::Active(B)&&B->GetPlayerFaction()==EFrontlineFaction::British&&B->GetCurrentCampaign()&&B->GetCurrentCampaign()->Id==OriginalId);
  UltimateArchiveUIV8215::Close(PC);UltimateArchiveUIV8215::Open(PC,B);
  Check(TEXT("v85_archive_reopen_retains_faction_and_operation"),UltimateArchiveUIV8215::View(PC).Faction==EFrontlineFaction::German&&UltimateArchiveUIV8215::View(PC).Mission==17);
 }
 UltimateArchiveUIV8215::Close(PC);
}
void StartFrontlineReviewV85(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Page=0,Captures=0,Visual=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0,Next=0,SampleAfter=0;TArray<TSharedPtr<FJsonValue>> Checks;TArray<double> GPU[6];TWeakObjectPtr<AFrontlineTank> Tank;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V85_CHECK %s %d"),*Name,int32(Pass));};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Captures);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V85 actual UK/DE morale capacities, archive return, armour natural approach and fixed-body recovery, actual Archive preparation queues, historically sourced medal forms and legacy-record retention; affected bilingual 4K captures and frozen visual comparison. Disposable account, FRRU campaigns closed."));Write(TEXT("checks.json"),J);V28ReviewPointer.Reset();FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay())return true;
  if(Now-R->Began>285.){Check(TEXT("v85_bounded_runtime"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<35)return true;
   Check(TEXT("v85_resident_core_before_draw"),FrontlineUIAssetsV8213::PreloadReady());if(!R->Good)return Finish();Clear();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
   CheckMoraleAndArchiveV85(PC,B,Check);
   FrontlineAchievementsV76::RunAwardReceiptChecksV84414(B,Check);
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v85_actual_terrain_selected"),B->SelectCampaignMission(17));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<80)return true;
   Check(TEXT("v85_actual_terrain_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v85_actual_fleet_exists"),Fleet!=nullptr);if(!R->Good)return Finish();
   TSharedPtr<FJsonObject> Contact;
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{Contact=RunFrontlineArmourApproachV85Review(Fleet);});});});
   Check(TEXT("v85_actual_contact_executed"),Contact.IsValid());if(!Contact.IsValid())return Finish();Write(TEXT("armour.json"),Contact.ToSharedRef());
   for(const auto& Value:Contact->GetArrayField(TEXT("checks"))){auto J=Value->AsObject();if(J.IsValid())Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
   auto Queue=RunFrontlineArchiveArmourV85Review(Fleet);Write(TEXT("archive-armour-queue.json"),Queue);
   for(const auto& Value:Queue->GetArrayField(TEXT("checks"))){auto J=Value->AsObject();if(J.IsValid())Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
   TSharedPtr<FJsonObject> Mobility;
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{Mobility=Fleet->RunMobilityChecksV85();});});});
   Write(TEXT("mobility.json"),Mobility.ToSharedRef());
   for(const auto& Value:Mobility->GetArrayField(TEXT("checks"))){auto J=Value->AsObject();if(J.IsValid())Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
   FrontlineMedalsV8215::RunAwardChecksV84414(Check);MedalWallV8215::RunInspectionUIChecksV84415(PC,Check);
   R->Stage=2;return true;
  }
  struct FShot{const TCHAR* Name;int32 Nation,Medal,Grade,Repeats;bool Inspect,ZH;};
  static const FShot Shots[]={
   {TEXT("01-german-first-class-zh.png"),1,3,1,0,true,true},
   {TEXT("02-german-last-class-en.png"),1,3,3,0,true,false},
   {TEXT("03-german-middle-class-zh.png"),1,20,3,0,false,true},
   {TEXT("04-german-sixth-class-en.png"),1,20,6,0,true,false},
   {TEXT("05-british-first-repeat-zh.png"),0,37,1,0,true,true},
   {TEXT("06-british-maximum-repeat-en.png"),0,37,1,FrontlineMedalsV8215::RepeatLimitV85(37),true,false},
   {TEXT("07-hohenzollern-knight-zh.png"),1,18,1,0,true,true},
   {TEXT("08-hohenzollern-commander-en.png"),1,18,2,0,true,false},
   {TEXT("09-hohenzollern-grand-commander-zh.png"),1,18,3,0,true,true},
   {TEXT("16-british-dfc-eagle-bars-zh.png"),0,38,1,FrontlineMedalsV8215::RepeatLimitV85(38),true,true}};
  if(R->Stage==2)
  {
   if(R->Page>=UE_ARRAY_COUNT(Shots)){Clear();R->Stage=4;return true;}
   Clear();B->Phase=EFrontlinePhase::Briefing;PC->SetActorTickEnabled(true);const auto& Shot=Shots[R->Page];
   Check(TEXT("v85_disposable_display_fixture"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,false,0));
   const auto Indices=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Shot.Nation));MedalWallV8215::PreloadVariants(Indices,false);
   S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=Shot.Nation;M.Inspect=false;M.Focus=Shot.Medal;
   Check(FString(Shot.Name)+TEXT("_preview_access"),MedalWallV8215::SelectInspectionStateV84413(PC,Shot.Medal,Shot.Grade,Shot.Repeats,Shot.Repeats?uint16(FrontlineMedalsV8215::EAttachment::Bar):0));
   if(Shot.Inspect)PC->GetHUD()->NotifyHitBoxClick(TEXT("V8216MedalInspect"));
   FrontlineLocalizationV35::SetLanguage(Shot.ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);R->Stage=3;R->Next=Now+1.;return true;
  }
  if(R->Stage==3)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<265)return true;
   int32 W=0,H=0;PC->GetViewportSize(W,H);Check(FString(Shots[R->Page].Name)+TEXT("_4k"),W==3840&&H==2160);if(W!=3840||H!=2160)return Finish();
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/Shots[R->Page].Name,true,false);++R->Captures;++R->Page;R->Stage=2;R->Next=Now+.1;return true;
  }
  if(R->Stage==4)
  {
   Clear();B->Phase=EFrontlinePhase::Fighting;
   MedalMountV8216::CloseV836(PC);UltimateArchiveUIV8215::Close(PC);
   // Return the presentation-owned environment lease before freezing this
   // independent scene. Medal pages may have retained the command-table view.
   FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);
   FactionSelectionV82::CoveredWorldV826().Update(PC,B);PC->SetActorTickEnabled(false);
   bool VisualReady=true;const auto VisualCheck=[&](const FString& Name,bool Pass){Check(Name,Pass);VisualReady&=Pass;};
   bool VisibleLight=false,VisibleGround=false;
   for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!It->IsHidden())
   {
    if(Cast<AWarfareFrontlineGround>(*It))VisibleGround=true;
    TInlineComponentArray<ULightComponentBase*> Lights(*It);
    for(auto* Light:Lights)VisibleLight|=Light&&Light->IsVisible()&&Light->Intensity>0;
   }
   VisualCheck(TEXT("v85_visual_command_table_released"),!FrontlineMapSceneV61::IsActive(PC));
   VisualCheck(TEXT("v85_visual_world_rendering_enabled"),PC->GetWorld()->GetGameViewport()&&!PC->GetWorld()->GetGameViewport()->bDisableWorldRendering);
   VisualCheck(TEXT("v85_visual_actual_environment_light_and_ground_visible"),VisibleLight&&VisibleGround);
   VisualCheck(TEXT("v85_visual_baseline_strict_profile"),FrontlineVisualV85::SetBaseline(B,true));
   R->Tank=B->GetWorld()->SpawnActor<AFrontlineTank>();
   VisualCheck(TEXT("v85_visual_actual_tank_configure"),R->Tank.IsValid()&&R->Tank->Configure(B,EFrontlineVehicle::MarkVStar,0,85000));
   VisualCheck(TEXT("v85_visual_frozen_actual_ground_pose"),FrontlineVisualV85::PlaceFrozenTank(B,R->Tank.Get(),FVector(2200,-1200,0)));
   VisualCheck(TEXT("v85_actual_mark_v_star_production_asset_ready"),R->Tank.IsValid()&&R->Tank->AreAssetsReady());
   VisualCheck(TEXT("v85_actual_six_release_gas_producer"),PrepareFrontlineGasVisualV836(B,FVector(-1200,700,AWarfareFrontlineGround::HeightAt(-1200,700))));
   // Earlier failures remain in R->Good and the final exit receipt. Independent
   // visual evidence must not be discarded when its own scene is valid.
   if(!VisualReady)return Finish();
   UGameplayStatics::SetGamePaused(B,true);R->Stage=5;return true;
  }
  if(R->Stage==5)
  {
   if(R->Visual>=6)
   {
    auto Perf=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Samples;
    for(int32 I=0;I<6;++I){auto J=MakeShared<FJsonObject>();auto V=R->GPU[I];V.Sort();double Total=0;for(double MS:V)Total+=MS;J->SetNumberField(TEXT("view"),I/2);J->SetBoolField(TEXT("baseline"),I%2==0);J->SetNumberField(TEXT("samples"),V.Num());J->SetNumberField(TEXT("mean_gpu_ms"),V.IsEmpty()?0:Total/V.Num());J->SetNumberField(TEXT("p95_gpu_ms"),V.IsEmpty()?0:V[FMath::Min(V.Num()-1,FMath::FloorToInt(V.Num()*.95))]);Samples.Add(MakeShared<FJsonValueObject>(J));}
    Perf->SetArrayField(TEXT("views"),Samples);Perf->SetStringField(TEXT("scope"),TEXT("Same-process frozen scene, actual 4K native High SDR, FG off. Warmed RHI whole-frame timing; not a battle-wide FPS guarantee."));Write(TEXT("visual-timing.json"),Perf);
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(R->Dir/TEXT("Captures/*.png")),true,false);Check(TEXT("v85_sixteen_affected_4k_captures_written"),Files.Num()==16&&R->Captures==16);FrontlineVisualV85::SetBaseline(B,false);return Finish();
   }
   const auto Rebound=FrontlineVisualV85::RebindFrozen(B,R->Visual%2==0);
   Check(FString::Printf(TEXT("v85_visual_%d_real_parent_bindings"),R->Visual),Rebound.Allowed&&Rebound.Ready&&Rebound.Gas>0&&Rebound.Armour>0);
   const FVector Focus=R->Visual<2?FVector(0,0,120):R->Visual<4?R->Tank->GetActorLocation()+FVector(0,0,150):FVector(-1000,700,400);
   PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=R->Visual<2?14000:R->Visual<4?4600:6200;
   FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,PC->BattleCamera->GetCameraComponent()->OrthoWidth,3840);
   PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager){PC->PlayerCameraManager->SetGameCameraCutThisFrame();PC->PlayerCameraManager->UpdateCamera(0.f);}
   Check(FString::Printf(TEXT("v85_visual_%d_actual_battle_camera"),R->Visual),PC->GetViewTarget()==PC->BattleCamera);
   R->Stage=6;R->SampleAfter=Now+2.;R->Next=0;R->Prepared=Now;return true;
  }
  if(R->Stage==6)
  {
   if(Now>=R->SampleAfter){const double MS=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(MS>0&&MS<500)R->GPU[R->Visual].Add(MS);}
   if(Now-R->Prepared<4.)return true;
   Check(FString::Printf(TEXT("v85_visual_%d_actual_gpu_samples"),R->Visual),R->GPU[R->Visual].Num()>=10);
   const FString Name=FString::Printf(TEXT("%02d-%s-%s.png"),R->Visual+10,R->Visual<2?TEXT("battle-scale"):R->Visual<4?TEXT("mark-v-star"):TEXT("gas-cloud"),R->Visual%2==0?TEXT("before"):TEXT("after"));
   FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/Name,true,false);++R->Captures;++R->Visual;R->Stage=5;R->Next=Now+.2;return true;
  }
  return true;
 }));
}
}
