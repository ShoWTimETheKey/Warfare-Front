// One focused disposable run. CPU bounds are diagnostic only; the retained
// rendered frames, not a CPU-box assertion, inspect the skin-cache output.
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineTrenchMotionV8447Checks.h"
#include "WarfareFrontlineTripoPropsV87.h"
#include "WarfareFrontlineRenderSettingsV87Checks.h"
#include "WarfareFrontlineTerrainV42.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
namespace
{
void StartFrontlineReviewV86(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Wave=0,Shots=0,Art=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=0,Prepared=0;TArray<TSharedPtr<FJsonValue>> Checks;TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Actors;
 FVector PropsFocus=FVector::ZeroVector,PropsViewFocus=FVector::ZeroVector;double PropsSampleBegan=0,PropsLast=0;int32 PropsUnfocused=0;TArray<double> PropsWall,PropsGPU,PropsGame,PropsRender,PropsRHI;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V86_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));if(!FFileHelper::SaveStringToFile(T,*(R->Dir/Name)))R->Good=false;};
  const auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);
   const TCHAR* Scope=FParse::Param(FCommandLine::Get(),TEXT("WFV87FormalBags"))?TEXT("V87 formal Tripo resource closure and whole-descriptor binding, actual nine-foot contact checks and three 4K scene cameras. Editor only; no natural campaign or Shipping acceptance."):
    FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoProps"))?TEXT("V87 actual UK/DE scene sandbags before/candidate, five 4K cameras and short warmed fixed-camera cost. No production binding, natural campaign or Shipping acceptance."):
    FParse::Param(FCommandLine::Get(),TEXT("WFV87MGOnly"))?TEXT("V87 actual UK/DE machinegun reload and helper station continuity, six real Tick scenarios. Disposable actors; no full historical motion/campaign acceptance."):
    FParse::Param(FCommandLine::Get(),TEXT("WFV87MGDeepOnly"))?TEXT("V87 actual UK/DE machinegun Ready/Reload pose visibility diagnostics; no production contact contract exercised when the lowering hypothesis is absent."):
    FParse::Param(FCommandLine::Get(),TEXT("WFV87MGLeaseOnly"))?TEXT("V87 actual UK/DE helper station and armour passing-lease diagnostics in two real team directions. No production movement fix or natural full campaign claim."):
    FParse::Param(FCommandLine::Get(),TEXT("WFV87SettingsOnly"))?TEXT("V87 actual HUD settings dispatch and live setter idempotence, forced reapply, isolated scratch persistence. No player profile or SDK presentation safety claim."):
    FParse::Param(FCommandLine::Get(),TEXT("WFV87FocusedOnly"))?TEXT("V87 private UK/DE smoke contact old/new actual Tick comparison, campaign report cache on labelled 10240-row history, real report event writer invalidation; disposable profile. No new model visual or natural full-campaign acceptance."):
    TEXT("V86 held full-section trench crossing, previous armour contact regression, blood slot/lifetime and three actual frames, accumulated campaign report codec and bilingual pages, paired German archive art. UK/DE disposable profiles; no FRRU campaigns.");
   J->SetStringField(TEXT("scope"),Scope);if(B)if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){auto D=Render->GetDiagnostics();const auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.AsyncCompute"));D->SetNumberField(TEXT("skin_cache_async_compute"),C?C->GetInt():-1);Write(TEXT("render-diagnostics.json"),D);}Write(TEXT("checks.json"),J);V28ReviewPointer.Reset();FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay())return true;
  if(Now-R->Began>400){Check(TEXT("v86_bounded_runtime"),false);return Finish();}
  // Frozen PlayerTick must not freeze its ordinary orthographic streaming view.
  // Submit the same production projection throughout warm-up and captures.
  if(((R->Stage>=20&&R->Stage<=34)||(R->Stage>=50&&R->Stage<=55))&&PC->BattleCamera)
  {int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),R->PropsViewFocus,PC->BattleCamera->GetCameraComponent()->OrthoWidth,VX);}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  const auto ClearUI=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  const auto Camera=[&](FVector P,float Width){R->PropsViewFocus=P;PC->BattleCamera->SetActorLocation(P+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((P-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager){PC->PlayerCameraManager->SetGameCameraCutThisFrame();PC->PlayerCameraManager->UpdateCamera(0);}};
  const auto Shot=[&](const FString& Name){int32 W=0,H=0;PC->GetViewportSize(W,H);Check(Name+TEXT("_4k"),W==3840&&H==2160);FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;R->Next=Now+.3;};
  if(R->Stage==0)
  {
   if(!FrontlineUIAssetsV8213::PreloadReady()&&Now-R->Began<40)return true;
   Check(TEXT("v86_resident_ui"),FrontlineUIAssetsV8213::PreloadReady());ClearUI();
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);}
   const auto* Skin=IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.AsyncCompute"));Check(TEXT("v86_pc_supported_graphics_skin_cache_actual_cvar"),Skin&&Skin->GetInt()==0);
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV87SettingsOnly")))
   {S.Settings=true;S.SettingsTab=2;R->Stage=40;R->Next=Now+.5;return true;}
   if(!FParse::Param(FCommandLine::Get(),TEXT("WFV87FormalBags"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrenchOnly"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoProps"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV87MGOnly"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV87MGDeepOnly"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV87MGLeaseOnly")))FrontlineReportV839::RunCampaignChecksV86(Check);
   Check(TEXT("v86_challenge_ordinary_campaign"),B->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran));
   
   int32 Mission=17;FParse::Value(FCommandLine::Get(),TEXT("WFV86Mission="),Mission);Mission=FMath::Clamp(Mission,0,19);
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=Mission;P->WonOperationMaskV52=(1u<<Mission)-1;P->DispatchResolvedMaskV52=(1u<<(Mission+1))-1;}
   Check(FString::Printf(TEXT("v86_actual_terrain_operation_%d_selected"),Mission),B->SelectCampaignMission(Mission));ClearUI();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==40)
  {
   auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());auto* H=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
   Check(TEXT("v87_settings_real_hud_and_subsystem"),Render&&H);if(!Render||!H)return Finish();
   const auto Before=Render->GetDiagnostics();const auto Q=Render->GetQuality();const auto P=Render->GetGraphicsPreset();
   const auto FG=Render->GetFrameGeneration();const auto Reflex=Render->GetReflex();const bool RR=Render->GetRayReconstruction();
   for(int32 I=0;I<8;++I)
   {
    H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("GraphicsPreset%d"),int32(P))));
    H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("ImageQuality%d"),int32(Q))));
    H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("ImageGeneration%d"),int32(FG))));
    H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("ImageReflex%d"),int32(Reflex))));
    H->NotifyHitBoxClick(RR?TEXT("ImageReconstruction1"):TEXT("ImageReconstruction0"));
   }
   const auto After=Render->GetDiagnostics();
   for(const TCHAR* Key:{TEXT("settings_apply_calls_v87"),TEXT("settings_suspend_calls_v87"),TEXT("settings_save_flush_calls_v87"),TEXT("fg_resume_not_before_v844")})
    Check(FString(TEXT("v87_settings_actual_hud_same_value_"))+Key,FMath::IsNearlyEqual(Before->GetNumberField(Key),After->GetNumberField(Key),1.e-6));
   Check(TEXT("v87_settings_actual_hud_requested_state_preserved"),Render->GetQuality()==Q&&Render->GetGraphicsPreset()==P&&Render->GetFrameGeneration()==FG&&Render->GetReflex()==Reflex&&Render->GetRayReconstruction()==RR);
   // Zero work alone could also mean an earlier overlay swallowed every hit.
   // Prove the same-frame HUD route with a real changed preset and restoration.
   const auto Alternate=P==EFrontlineGraphicsPreset::High?EFrontlineGraphicsPreset::Medium:EFrontlineGraphicsPreset::High;
   H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("GraphicsPreset%d"),int32(Alternate))));const auto Routed=Render->GetDiagnostics();
   Check(TEXT("v87_settings_actual_hud_changed_preset_reaches_live_setter"),Render->GetGraphicsPreset()==Alternate
    &&Routed->GetNumberField(TEXT("settings_apply_calls_v87"))==After->GetNumberField(TEXT("settings_apply_calls_v87"))+1
    &&Routed->GetNumberField(TEXT("settings_suspend_calls_v87"))==After->GetNumberField(TEXT("settings_suspend_calls_v87"))+1);
   H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("GraphicsPreset%d"),int32(P))));const auto Restored=Render->GetDiagnostics();
   Check(TEXT("v87_settings_actual_hud_changed_preset_restores_original"),Render->GetGraphicsPreset()==P
    &&Restored->GetNumberField(TEXT("settings_apply_calls_v87"))==Routed->GetNumberField(TEXT("settings_apply_calls_v87"))+1);
   auto Dispatch=MakeShared<FJsonObject>();Dispatch->SetNumberField(TEXT("same_value_hud_dispatches"),40);Dispatch->SetNumberField(TEXT("changed_preset_route_witnesses"),2);Dispatch->SetObjectField(TEXT("before"),Before);Dispatch->SetObjectField(TEXT("after"),After);Dispatch->SetObjectField(TEXT("changed_preset"),Routed);Dispatch->SetObjectField(TEXT("restored_preset"),Restored);Write(TEXT("settings-hud-dispatch-v87.json"),Dispatch);
   Write(TEXT("settings-idempotence-v87.json"),FrontlineRenderSettingsV87::RunChecks(B,Check));return Finish();
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<80)return true;
   Check(TEXT("v86_actual_battle_ready"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(B->Phase!=EFrontlinePhase::Fighting||S.Preparing)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV87FormalBags")))
   {ClearUI();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);FactionSelectionV82::CoveredWorldV826().Update(PC,B);R->Stage=50;R->Next=Now+3;return true;}
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoProps")))
   {
    auto J=FrontlineTripoPropsV87::Prepare(B,Check);Write(TEXT("tripo-props-baseline.json"),J);
    const bool Ready=FrontlineTripoPropsV87::Focus(B,R->PropsFocus);Check(TEXT("v87_tripo_actual_scene_focus"),Ready);if(!Ready)return Finish();
    ClearUI();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);FactionSelectionV82::CoveredWorldV826().Update(PC,B);
    Camera(R->PropsFocus,1250);R->Stage=20;R->Next=Now+3;return true;
   }
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV87FocusedOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFV87MGOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFV87MGDeepOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFV87MGLeaseOnly")))
   {
    FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{
     if(FParse::Param(FCommandLine::Get(),TEXT("WFV87MGLeaseOnly"))){auto C=FrontlineTrenchMotionV8447::RunMachinegunLeaseChecksV87(B,Check);Write(TEXT("machinegun-lease-v87.json"),C);}
     else if(FParse::Param(FCommandLine::Get(),TEXT("WFV87MGDeepOnly"))){auto C=FrontlineTrenchMotionV8447::RunMachinegunEngagementChecksV87(B,Check);Write(TEXT("machinegun-engagement-v87.json"),C);}
     else if(FParse::Param(FCommandLine::Get(),TEXT("WFV87MGOnly"))){auto C=FrontlineTrenchMotionV8447::RunMachinegunContinuityChecksV87(B,Check);Write(TEXT("machinegun-continuity-v87.json"),C);}
     else {auto C=FrontlineTrenchMotionV8447::RunExistingContactChecksV87(B,Check);Write(TEXT("existing-contact-v87.json"),C);FrontlineReportV839::RunLiveCacheWriterChecksV87(B,Check);}
    });});});
    return Finish();
   }
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v86_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return Finish();
   FrontlineAchievementsV76::WithScopedCombatLedgerV84412(B,[&]{FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]{FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrenchOnly")))
    {auto C=RunFrontlineArmourApproachV85Review(Fleet);Write(TEXT("armour.json"),C);for(const auto& V:C->GetArrayField(TEXT("checks"))){auto J=V->AsObject();Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}}
    auto C=Fleet->RunTrenchMobilityChecksV86();Write(TEXT("armour-trench.json"),C);for(const auto& V:C->GetArrayField(TEXT("checks"))){auto J=V->AsObject();Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrenchOnly")))AWarfareFrontlineWoundFX::RunBloodChecksV86(B,Check);
   });});});
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrenchOnly")))return Finish();
   ClearUI();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);FactionSelectionV82::CoveredWorldV826().Update(PC,B);
   Camera(FVector(0,0,140),7600);R->Stage=2;return true;
  }
  if(R->Stage==50)
  {
   auto J=FrontlineEnvironmentV42::RunChecks(B);Write(TEXT("formal-bags-contact-v87.json"),J);
   auto P=J->GetObjectField(TEXT("field_props_v8449"));
   Check(TEXT("v87_formal_bags_actual_descriptor_bound"),P->GetBoolField(TEXT("formal_tripo_sandbags_v87")));
   Check(TEXT("v87_formal_bags_actual_contact_contract"),P->GetNumberField(TEXT("bag_ground_contact_errors"))==0&&P->GetNumberField(TEXT("bag_instances_v84412"))>0);
   Check(TEXT("v87_formal_bags_actual_shader_and_no_collision"),P->GetNumberField(TEXT("actual_render_shader_errors"))==0&&J->GetNumberField(TEXT("collision_components"))==0&&J->GetNumberField(TEXT("invalid_materials"))==0);
   const auto F=P->GetObjectField(TEXT("bag_camera_focus"));R->PropsFocus=FVector(F->GetNumberField(TEXT("x")),F->GetNumberField(TEXT("y")),F->GetNumberField(TEXT("z")));
   Camera(R->PropsFocus,1250);R->Stage=51;R->Next=Now+3;return true;
  }
  if(R->Stage==51){Shot(TEXT("formal-bags-close"));R->Stage=52;return true;}
  if(R->Stage==52){Camera(R->PropsFocus,6000);R->Stage=53;R->Next=Now+3;return true;}
  if(R->Stage==53){Shot(TEXT("formal-bags-tactical"));R->Stage=54;return true;}
  if(R->Stage==54){Camera(R->PropsFocus+FVector(180,90,0),2100);R->Stage=55;R->Next=Now+3;return true;}
  if(R->Stage==55){Shot(TEXT("formal-bags-moved"));R->Stage=56;return true;}
  if(R->Stage==56)return Finish();
  if(R->Stage>=20&&R->Stage<=34)
  {
   const auto ResetSample=[&](){R->PropsSampleBegan=R->PropsLast=0;R->PropsUnfocused=0;R->PropsWall.Reset();R->PropsGPU.Reset();R->PropsGame.Reset();R->PropsRender.Reset();R->PropsRHI.Reset();};
   const auto Sample=[&](){if(!R->PropsSampleBegan){R->PropsSampleBegan=R->PropsLast=Now;return false;}R->PropsWall.Add((Now-R->PropsLast)*1000);R->PropsLast=Now;const double G=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(G>0)R->PropsGPU.Add(G);const double GT=FPlatformTime::ToMilliseconds(GGameThreadTime),RT=FPlatformTime::ToMilliseconds(GRenderThreadTime),RH=FPlatformTime::ToMilliseconds(GRHIThreadTime);if(GT>0)R->PropsGame.Add(GT);if(RT>0)R->PropsRender.Add(RT);if(RH>0)R->PropsRHI.Add(RH);R->PropsUnfocused+=!FApp::HasFocus();return Now-R->PropsSampleBegan>=3;};
   const auto Stats=[&](TArray<double> V){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),V.Num());J->SetBoolField(TEXT("available"),!V.IsEmpty());if(!V.IsEmpty()){double Sum=0;for(double X:V)Sum+=X;V.Sort();J->SetNumberField(TEXT("mean_ms"),Sum/V.Num());J->SetNumberField(TEXT("p95_ms"),V[FMath::Clamp(FMath::CeilToInt(V.Num()*.95)-1,0,V.Num()-1)]);J->SetNumberField(TEXT("max_ms"),V.Last());}return J;};
   const auto SaveTimes=[&](const TCHAR* N){auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Stats(R->PropsWall));J->SetObjectField(TEXT("gpu"),Stats(R->PropsGPU));J->SetObjectField(TEXT("game"),Stats(R->PropsGame));J->SetObjectField(TEXT("render"),Stats(R->PropsRender));J->SetObjectField(TEXT("rhi"),Stats(R->PropsRHI));J->SetNumberField(TEXT("unfocused_frames"),R->PropsUnfocused);J->SetNumberField(TEXT("sample_seconds"),Now-R->PropsSampleBegan);J->SetStringField(TEXT("scope"),TEXT("Short warmed fixed actual battle camera, simulation frozen, Editor offscreen High SDR Quality SR RR on FG off. Local candidate cost only, not Shipping performance or active combat."));Write(N,J);};
   if(R->Stage==20){Shot(TEXT("tripo-baseline-close"));R->Stage=26;return true;}
   if(R->Stage==26){Camera(R->PropsFocus,6000);ResetSample();R->Stage=21;R->Next=Now+3;return true;}
   if(R->Stage==21){if(!Sample())return true;SaveTimes(TEXT("tripo-baseline-timings.json"));Shot(TEXT("tripo-baseline-tactical"));R->Stage=22;return true;}
   if(R->Stage==22){auto J=FrontlineTripoPropsV87::SwitchCandidate(B,Check);Write(TEXT("tripo-props-candidate.json"),J);if(!J->GetBoolField(TEXT("success"))){FrontlineTripoPropsV87::Restore(B);return Finish();}Camera(R->PropsFocus,1250);R->Stage=23;R->Next=Now+3;return true;}
   if(R->Stage==23){Write(TEXT("tripo-props-warmed-close.json"),FrontlineTripoPropsV87::DescribeCurrent(B));Shot(TEXT("tripo-candidate-close"));R->Stage=27;return true;}
   if(R->Stage==27){Camera(R->PropsFocus,6000);ResetSample();R->Stage=24;R->Next=Now+3;return true;}
   if(R->Stage==24){if(!Sample())return true;SaveTimes(TEXT("tripo-candidate-timings.json"));Shot(TEXT("tripo-candidate-tactical"));R->Stage=28;return true;}
   if(R->Stage==28){Camera(R->PropsFocus+FVector(180,90,0),2100);R->Stage=25;R->Next=Now+3;return true;}
   if(R->Stage==25){Write(TEXT("tripo-props-warmed-moved.json"),FrontlineTripoPropsV87::DescribeCurrent(B));Shot(TEXT("tripo-candidate-moved"));R->Stage=29;return true;}
   if(R->Stage==29&&FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoTextureWait")))
   {if(FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoSamplingFix"))){auto J=FrontlineTripoPropsV87::RepairCandidateSampling(B,Check);Write(TEXT("tripo-sampling-repair.json"),J);if(!J->GetBoolField(TEXT("success"))){FrontlineTripoPropsV87::Restore(B);return Finish();}}auto J=FrontlineTripoPropsV87::EnsureTextureReady(B,Check);Write(TEXT("tripo-texture-compile-ready.json"),J);if(!J->GetBoolField(TEXT("success"))){FrontlineTripoPropsV87::Restore(B);return Finish();}Camera(R->PropsFocus,1250);R->Stage=30;R->Next=Now+3;return true;}
   if(R->Stage==30){Write(TEXT("tripo-props-corrected-close.json"),FrontlineTripoPropsV87::DescribeCurrent(B));Shot(TEXT("tripo-corrected-close"));R->Stage=31;return true;}
   if(R->Stage==31){Camera(R->PropsFocus,6000);ResetSample();R->Stage=32;R->Next=Now+3;return true;}
   if(R->Stage==32){if(!Sample())return true;SaveTimes(TEXT("tripo-corrected-timings.json"));Write(TEXT("tripo-props-corrected-tactical.json"),FrontlineTripoPropsV87::DescribeCurrent(B));Shot(TEXT("tripo-corrected-tactical"));R->Stage=34;return true;}
   FrontlineTripoPropsV87::Restore(B);return Finish();
  }
  if(R->Stage==2)
  {
   FVector G=FVector::ZeroVector;auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld());Check(TEXT("v86_actual_blood_capture_prepared"),FX&&FX->ReviewBloodCaptureV86(G));if(!FX)return Finish();
   FX->SetActorTickEnabled(true);Camera(G,1450);R->Prepared=Now;R->Wave=0;R->Stage=3;return true;
  }
  if(R->Stage==3)
  {
   const float Times[]={.7f,3.0f,8.0f};if(Now-R->Prepared<Times[R->Wave])return true;
   Shot(FString::Printf(TEXT("blood-sequence-%d"),R->Wave));if(++R->Wave==3){if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld()))Write(TEXT("blood-visual.json"),FX->DescribeBloodV84411());R->Stage=4;}return true;
  }
  if(R->Stage==4)
  {
   ClearUI();PC->SetActorTickEnabled(true);B->Phase=EFrontlinePhase::Briefing;
   if(R->Art>=2){R->Art=0;R->Stage=6;return true;}
   auto* Campaign=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(Campaign)Campaign->Research.TechnologyMask&=~(1u<<19);
   FrontlineLocalizationV35::SetLanguage(R->Art?FrontlineLocalizationV35::ELanguage::English:FrontlineLocalizationV35::ELanguage::Chinese);
   S.Technology=true;S.TechnologyPage=1;S.TechnologyFocus=19;R->Stage=5;R->Next=Now+.7;return true;
  }
  if(R->Stage==5){if(S.PhotoTransitionV37)return true;Shot(FString::Printf(TEXT("german-barrage-archive-%s"),R->Art?TEXT("en"):TEXT("zh")));++R->Art;R->Stage=4;return true;}
  if(R->Stage==6)
  {
   if(R->Art>=12)return Finish();ClearUI();PC->SetActorTickEnabled(true);
   if(R->Art==0){FrontlineReportV839::RunCampaignUIRouteChecksV86(B,PC,Check);ClearUI();}
   FrontlineLocalizationV35::SetLanguage(R->Art<6?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   Check(TEXT("v86_actual_campaign_report_prepared"),FrontlineReportV839::PrepareCampaignVisualV86(B,PC,R->Art%6,0));if(R->Art%6==5)FrontlineReportV839::View(PC).Offset=12;S.CampaignEndingV51=true;S.EndingVisibleAtV83=-1;R->Stage=7;R->Next=Now+1.3;return true;
  }
  if(R->Stage==7){if(S.PhotoTransitionV37||S.EndingVisibleAtV83<0||Now-S.EndingVisibleAtV83<1.1)return true;Check(TEXT("v86_campaign_report_actual_number_timeline_started"),S.EndingVisibleAtV83>=0);Shot(FString::Printf(TEXT("campaign-report-%s-%d"),R->Art<6?TEXT("zh"):TEXT("en"),R->Art%6));++R->Art;R->Stage=6;return true;}
  return true;
 }));
}
}
