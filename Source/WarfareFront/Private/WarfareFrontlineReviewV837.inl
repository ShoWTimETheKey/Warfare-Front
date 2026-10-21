TSharedRef<FJsonObject> RunFrontlineRailGameplayV837Review(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
bool PrepareFrontlineRailVisualV837(AWarfareFrontlineGameMode*,bool);
namespace
{
void StartFrontlineReviewV837(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV837Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{int32 Stage=0,Shots=0,Pending=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2,Prepared=0;FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TWeakObjectPtr<AFrontlineTank> Train;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V837_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J){FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/Name));};
  const auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("Disposable UK/DE rail readiness, six difficulty/army scenarios, actual passenger/weapon/module/mobility paths, honours/rack rules and retained account bits. Actual 4K new train, wreck, surface and honours captures. UI award seed is synthetic in-memory evidence, not earned-gameplay proof. No FR/RU campaign development, whole-campaign balance or foreground-performance guarantee."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExit(false);return false;};
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v837_disposable_world"),false);return Finish();}
  if(Now-R->Began>150){Check(TEXT("v837_bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);double Delay=.30;
  const auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;S.FactionSelectionV82=FFrontlineFactionSelectionV82();PC->CancelTargeting();MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::CloseV836(PC);UltimateArchiveUIV8215::Close(PC);V28ReviewPointer=FVector2D(-10000,-10000);};
  const auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
  const auto Camera=[&](const FVector& Focus,float Width){PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);};
  const auto Visual=[&](bool German){Clear();B->AbandonBattleToOperations();Check(TEXT("v837_actual_rail_visual_fixture_")+FString::FromInt(German),PrepareFrontlineRailVisualV837(B,German));B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);R->Train.Reset();if(auto* F=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:F->GetVehicles())if(T&&T->IsOperational()&&T->IsRailConvoyV32()){R->Train=T;break;}Check(TEXT("v837_visual_live_train_found"),R->Train.IsValid());Camera(FVector(0,-350,170),7500);};
  if(R->Pending==1){R->Pending=0;Camera(FVector(1000,-1130,160),4000);R->Next=Now+.7;return true;}
  if(R->Pending==2){R->Pending=0;Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);S.AchievementsV76=true;HonoursListV82::Reveal(S,29,true);R->Next=Now+.5;return true;}
  if(R->Pending==3){R->Pending=0;Camera(FVector(-1300,-1130,160),4200);R->Next=Now+.7;return true;}
  switch(R->Stage)
  {
   case 0:
   {
    Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v837_real_british_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
    if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=19;P->WonOperationMaskV52=(1u<<19)-1;P->DispatchResolvedMaskV52=(1u<<20)-1;P->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;P->ArmourResearchMaskV63=0xffff;P->AcknowledgedEventMask=0xffffffffu;}
    if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);}
    Check(TEXT("v837_prepared_rail_operation"),B->SelectCampaignMission(17));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;break;
   }
   case 1:
    if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<65){R->Next=Now+.2;return true;}
    Check(TEXT("v837_actual_battle_loaded"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
    if(!R->Good){auto D=MakeShared<FJsonObject>();D->SetNumberField(TEXT("phase"),int32(B->Phase));D->SetBoolField(TEXT("preparing"),S.Preparing);D->SetBoolField(TEXT("paused"),UGameplayStatics::IsGamePaused(B));D->SetStringField(TEXT("preparation_error"),S.PreparationError);D->SetStringField(TEXT("asset_failure"),B->AssetFailure);D->SetStringField(TEXT("message"),B->LastMessage);Write(TEXT("startup-failure.json"),D);return Finish();}B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);Clear();break;
   case 2:
    Write(TEXT("rail-gameplay-v837.json"),RunFrontlineRailGameplayV837Review(B,Check));
    FrontlineAchievementsV76::RunIsolatedChecks(B,Check);FrontlineArmourValourV8221::RunChecks(Check);MedalWallV8215::RunDeedGeometryChecksV83(Check);FrontlineRibbonRackV8222::RunChecks(Check);
    for(int32 I=28;I<30;++I)Check(TEXT("v837_tribute_cooked_")+FString::FromInt(I),LoadObject<UTexture2D>(nullptr,*FrontlineAchievementsV76::BadgeObjectPathV82(I))!=nullptr);
    if(auto* Surface=AFrontlineArmourSurfaceV837::Get(B->GetWorld()))Check(TEXT("v837_surface_assets_ready"),Surface->IsReady());else Check(TEXT("v837_surface_service"),false);
    if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Check(TEXT("v837_contact_audio_assets_ready"),Audio->ArmourContactAssetsReadyV837());else Check(TEXT("v837_contact_audio_service"),false);
    Visual(false);Delay=1.7;break;
   case 3:Shot(TEXT("01-uk-rail-confrontation-normal-view"));break;
   case 4:Visual(true);Delay=1.7;break;
   case 5:
    Shot(TEXT("02-de-rail-detachment-normal-view"));
    if(auto* T=R->Train.Get())for(int32 I=0;I<90;++I){B->Elapsed+=.05f;T->Simulate(.05f);}
    R->Pending=1;Delay=.1;break;
   case 6:
    if(auto* Surface=AFrontlineArmourSurfaceV837::Get(B->GetWorld()))Write(TEXT("live-materials-v837.json"),Surface->Describe());
    if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))Write(TEXT("renderer-v837.json"),Render->GetDiagnostics());
    Shot(TEXT("03-train-engine-paint-and-bogie-detail"));R->Pending=3;Delay=.1;break;
   case 7:Shot(TEXT("04-train-carriages-and-weapons"));break;
   case 8:
   {
    if(auto* T=R->Train.Get())
    {
     Check(TEXT("v837_live_train_has_new_body"),T->AreAssetsReady());
     const FVector Point=T->ObservedArmourAimPointV837(FVector(0,300,140));
     if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))Fire->EmitArmourContactV836(Point,FVector(0,1,0),1.f,false);
     Shot(TEXT("05-small-armour-penetration-effect"));
    }
    break;
   }
   case 9:
    if(auto* T=R->Train.Get()){T->ReceiveModuleHit(EFrontlineTankModule::Hull,T->GetMaximumHullIntegrity()+10);Check(TEXT("v837_actual_train_wreck_non_operational"),!T->IsOperational());Write(TEXT("train-wreck-v837.json"),FFrontlineArmourWreckV8220Access::Describe(T));}Delay=.7;break;
   case 10:
    if(auto* Surface=AFrontlineArmourSurfaceV837::Get(B->GetWorld()))Write(TEXT("surface-v837.json"),Surface->Describe());Shot(TEXT("06-destroyed-train-and-surface-interaction"));break;
   case 11:
    Clear();Check(TEXT("v837_isolated_honours_ui_seed"),FrontlineAchievementsV76::SeedHonoursPresentationV837(B,EFrontlineFaction::German));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);S.AchievementsV76=true;{auto& W=MedalWallV8215::View(PC);W.Wall=true;W.Nation=1;W.Focus=MedalWallV8215::DeedBaseV8222+1;W.Row=2;}Delay=.5;break;
   case 12:Shot(TEXT("07-de-chest-badge-and-tight-ribbon-wall-zh"));break;
   case 13:Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);S.AchievementsV76=true;HonoursListV82::Reveal(S,28,true);Delay=.5;break;
   case 14:Shot(TEXT("08-valiant-hearts-tribute-zh"));R->Pending=2;Delay=.1;break;
   case 15:Shot(TEXT("09-all-quiet-tribute-en"));break;
   case 16:Clear();B->Phase=EFrontlinePhase::Fighting;V28ReviewPointer=FVector2D(1870,25);Delay=.7;break;
   case 17:Shot(TEXT("10-battle-ribbon-rack-tooltip"));break;
   case 18:
    Clear();B->AbandonBattleToOperations();PC->SetActorTickEnabled(true);
    {auto& W=WarRoom52::State(PC);WarRoom52::Synchronize(W,B);W.FlightStart=Now-10;WarRoom52::AtlasTick(W,Now);W.WasActive=true;}
    R->Prepared=Now;Delay=.4;break;
   case 19:
   {
    const bool Ready=FrontlineMapSceneV61::ContinuePreloadReadyV8214(PC)&&FrontlineMapSceneV61::IsAtlasReadyV64(PC)&&FrontlineMapSceneV61::ContinueViewWarmedV8214(PC);
    if(!Ready&&Now-R->Prepared<15){R->Next=Now+.2;return true;}
    Check(TEXT("v837_headquarters_rack_capture_has_ready_actual_map"),Ready);Shot(TEXT("11-headquarters-ribbon-rack"));break;
   }
   default:Clear();return Finish();
  }
  ++R->Stage;R->Next=FPlatformTime::Seconds()+Delay;return true;
 }),0.f);
}
}
