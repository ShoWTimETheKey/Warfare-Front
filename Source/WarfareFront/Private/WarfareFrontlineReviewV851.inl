// One focused disposable run. CPU bounds are diagnostic only; the retained
// rendered frames, not a CPU-box assertion, inspect the skin-cache output.
#include "WarfareFrontlineMotionV40.h"
namespace
{
void StartFrontlineReviewV851(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Wave=0,Shots=0,Art=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=0,Prepared=0;TArray<TSharedPtr<FJsonValue>> Checks;TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Actors;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V851_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  const auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V85.1 actual archive navigation, gas-mask tick, armour Simulate contact, fatal-receipt achievement boundaries, first-visible living/casualty skeletal stress and German paired art. UK/DE disposable profiles; no FRRU campaigns, no exhaustive stability or performance claim."));if(B)if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){auto D=Render->GetDiagnostics();const auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.AsyncCompute"));D->SetNumberField(TEXT("skin_cache_async_compute"),C?C->GetInt():-1);Write(TEXT("render-diagnostics.json"),D);}Write(TEXT("checks.json"),J);V28ReviewPointer.Reset();FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay())return true;
  if(Now-R->Began>285){Check(TEXT("v851_bounded_runtime"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  const auto ClearUI=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  const auto Camera=[&](FVector P,float Width){PC->BattleCamera->SetActorLocation(P+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((P-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager){PC->PlayerCameraManager->SetGameCameraCutThisFrame();PC->PlayerCameraManager->UpdateCamera(0);}};
  const auto Shot=[&](const FString& Name){int32 W=0,H=0;PC->GetViewportSize(W,H);Check(Name+TEXT("_4k"),W==3840&&H==2160);FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;R->Next=Now+.3;};
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<40)return true;
   Check(TEXT("v851_resident_ui"),FrontlineUIAssetsV8213::PreloadReady());ClearUI();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);}
   const auto* Skin=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.AsyncCompute"));Check(TEXT("v851_pc_supported_graphics_skin_cache_actual_cvar"),Skin&&Skin->GetInt()==0);
   RunArchiveFlowChecksV851(PC,B,Check);
   Check(TEXT("v851_challenge_ordinary_campaign"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));
   FrontlineAchievementsV76::RunChallengeChecksV84415(B,Check);
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v851_actual_late_rail_terrain_selected"),B->SelectCampaignMission(17));ClearUI();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<80)return true;
   Check(TEXT("v851_actual_battle_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(B->Phase!=EFrontlinePhase::Fighting||S.Preparing)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v851_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return Finish();
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{
    auto C=RunFrontlineArmourApproachV85Review(Fleet);Write(TEXT("armour.json"),C);for(const auto& V:C->GetArrayField(TEXT("checks"))){auto J=V->AsObject();Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV851DenseDiagnostic")))FrontlineMotionV40::RunGasMaskChecksV851(B,Check);
   });});});
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV851DenseDiagnostic")))return Finish();
   ClearUI();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);FactionSelectionV82::CoveredWorldV826().Update(PC,B);
   Camera(FVector(0,0,140),7600);R->Stage=2;return true;
  }
  if(R->Stage==2)
  {
   if(R->Wave>=8){R->Stage=4;return true;}
   // Three new visible skin variants per wave; ordinary fatal hits create
   // replacement casualty meshes. Earlier corpses remain in view at rest.
   const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills};
   for(int32 I=0;I<3;++I)
   {
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();Check(TEXT("v851_stress_spawn"),U!=nullptr);if(!U)continue;
    U->Initialize(B,851000+R->Wave*3+I,R->Wave%2,Roles[I],0);U->bFieldEntered=true;
    const float X=-2400+R->Wave*650,Y=-1200+I*850;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));U->SetActorTickEnabled(true);B->Units.Add(U);R->Actors.Add(U);
   }
   R->Prepared=Now;R->Stage=3;R->Next=Now+.65;return true;
  }
  if(R->Stage==3)
  {
   if(Now-R->Prepared<.9)return true;
   const int32 Start=R->Actors.Num()-3;
   if(Now-R->Prepared<1.9)
   {
    for(int32 I=Start;I<R->Actors.Num();++I)if(auto* U=R->Actors[I].Get())if(U->IsAlive())
     U->ReceiveHit(100,1,1-U->Team,FVector(1,(I%3)-1,.45f),I%3==0?EFrontlineDamageKind::Artillery:EFrontlineDamageKind::Bullet,1.5f);
    R->Next=Now+1.2;return true;
   }
   const auto* Skin=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.AsyncCompute"));Check(TEXT("v851_pc_skin_cache_remains_graphics_after_casualty_streaming"),Skin&&Skin->GetInt()==0);
   bool Bounds=true;for(const auto& W:R->Actors)if(auto* U=W.Get())if(U->Soldier){const auto& A=U->Soldier->Bounds;Bounds&=!A.Origin.ContainsNaN()&&!A.BoxExtent.ContainsNaN()&&A.BoxExtent.GetMax()<1200;}
   Check(TEXT("v851_cpu_mesh_bounds_diagnostic_only"),Bounds);
   Shot(FString::Printf(TEXT("skin-streaming-%02d"),R->Wave));++R->Wave;R->Stage=2;return true;
  }
  if(R->Stage==4)
  {
   for(const auto& W:R->Actors)if(auto* U=W.Get())U->SetActorTickEnabled(false);
   ClearUI();PC->SetActorTickEnabled(true);B->Phase=EFrontlinePhase::Briefing;
   if(R->Art>=4)return Finish();
   auto* Campaign=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(Campaign){if(R->Art>=2)Campaign->Research.TechnologyMask|=1u<<19;else Campaign->Research.TechnologyMask&=~(1u<<19);}
   FrontlineLocalizationV35::SetLanguage(R->Art%2?FrontlineLocalizationV35::ELanguage::English:FrontlineLocalizationV35::ELanguage::Chinese);
   S.Technology=true;S.TechnologyPage=1;S.TechnologyFocus=19;R->Prepared=Now;R->Stage=5;R->Next=Now+1.2;return true;
  }
  if(R->Stage==5){Shot(FString::Printf(TEXT("german-barrage-%s-%s"),R->Art<2?TEXT("archive"):TEXT("colour"),R->Art%2?TEXT("en"):TEXT("zh")));++R->Art;R->Stage=4;return true;}
  return true;
 }));
}
}
