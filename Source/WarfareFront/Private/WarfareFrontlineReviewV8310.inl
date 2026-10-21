#include "WarfareFrontlineRibbonRackV8222.h"
AFrontlineTank* PrepareFrontlineArmourArtV838(AWarfareFrontlineGameMode*,int32);
namespace
{
void StartFrontlineReviewV8310(AWarfareFrontlineController* Controller)
{
 int32 Feasibility=-1;if(FParse::Value(FCommandLine::Get(),TEXT("WFV838FeasibilityCase="),Feasibility)&&Feasibility>=2&&Feasibility<=3){StartFrontlineReviewV838(Controller);return;}
 FString User;if(!Controller||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{int32 Stage=0,Art=0,ArtEnd=10,Page=0,Shots=0,Samples=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2,Prepared=0,GameMS=0,RenderMS=0,GPUMS=0;FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;int32 ArtStart=-1;if(FParse::Value(FCommandLine::Get(),TEXT("WFV8310ArtStart="),ArtStart)&&ArtStart>=0&&ArtStart<=10)R->Art=ArtStart;int32 ArtEnd=10;if(FParse::Value(FCommandLine::Get(),TEXT("WFV8310ArtEnd="),ArtEnd)&&ArtEnd>=R->Art&&ArtEnd<=10)R->ArtEnd=ArtEnd;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  if(R->Stage==3&&Now-R->Prepared>=8.){++R->Samples;R->GameMS+=FPlatformTime::ToMilliseconds(GGameThreadTime);R->RenderMS+=FPlatformTime::ToMilliseconds(GRenderThreadTime);R->GPUMS+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());}
  const auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V8310_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(R->Dir/Name));};
  const auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("Disposable production armour/wreck scenes and bilingual medal-wall rendering. Artwork scenes do not establish gameplay balance or achievement feasibility."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!B||!PC||!PC->BattleCamera){if(Now-R->Began<30)return true;Check(TEXT("world_ready"),false);return Finish();}
  if(Now-R->Began>390){Check(TEXT("bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& S=ScreenState(PC);
  const auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;};
  const auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;PC->CancelTargeting();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);};
  const auto Camera=[&](const FVector& Focus,float Width){PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,Width,3840);};
  if(R->Stage==0)
  {
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);}
   Check(TEXT("disposable_british_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;R->Next=Now+.1;return true;
  }
  if(R->Stage==1)
  {
   if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<70){R->Next=Now+.1;return true;}
   Check(TEXT("actual_operation_loaded"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   Clear();PC->SetActorTickEnabled(false);B->SetActorTickEnabled(false);FrontlineRibbonRackV8222::RunChecks(Check);FrontlineAchievementsV76::RunAllQuietChecksV8310(B,Check);
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV8310UKRifleGrenades")))Write(TEXT("british-infantry-armour.json"),FrontlineCombatV64::RunAntiArmourChecksV833(B,Check));
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV8310UKRifleGrenades")))Write(TEXT("british-infantry-rotation.json"),FrontlineCombatV64::RunNo44RotationChecksV8310(B,Check));
   if(!FParse::Param(FCommandLine::Get(),TEXT("WFV8310UIOnly")))if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))Fire->RunWreckFlamePoolChecksV8310(B,Check);
   R->Stage=FParse::Param(FCommandLine::Get(),TEXT("WFV8310UIOnly"))?4:2;R->Next=FPlatformTime::Seconds()+.1;return true;
  }
  if(R->Stage==2)
  {
   Clear();auto* V=PrepareFrontlineArmourArtV838(B,R->Art);Check(TEXT("production_vehicle_assets_")+FString::FromInt(R->Art),V&&(R->Art==10?!V->IsOperational():V->AreAssetsReady()));if(!R->Good)return Finish();
   Clear();Camera(R->Art==10?FVector(0,-300,210):FVector(0,-650,140),R->Art==10?7800.f:4100.f);Check(TEXT("actual_battle_view_target"),PC->GetViewTarget()==PC->BattleCamera);R->Samples=0;R->GameMS=R->RenderMS=R->GPUMS=0;R->Prepared=FPlatformTime::Seconds();R->Next=R->Prepared+12.5;R->Stage=3;return true;
  }
  if(R->Stage==3)
  {
   auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld());Check(TEXT("production_fire_service_exists"),Fire!=nullptr);if(!Fire)return Finish();
   const auto Smoke=Fire->DescribeWreckSmokeV838();bool Mature=Smoke->GetNumberField(TEXT("active_sources"))>0;
   for(const auto& Row:Smoke->GetArrayField(TEXT("sources")))Mature&=Row->AsObject()->GetNumberField(TEXT("flow_age"))>=12.;
   if(!Mature&&Now-R->Prepared<45.){R->Next=Now+.2;return true;}
   Check(TEXT("wreck_actual_flow_clock_has_retired_damage_blast_")+FString::FromInt(R->Art),Mature);
   Fire->RunEffectsChecksV839(Check);Fire->RunWreckFlameChecksV8310(Check);
   const auto Flame=Fire->DescribeWreckFlameV8310();Check(TEXT("actual_visible_wreck_volume_source_" )+FString::FromInt(R->Art),Flame->GetNumberField(TEXT("visible_wreck_cores"))>0);
   Write(FString::Printf(TEXT("wreck-flame-%02d.json"),R->Art),Flame);Write(FString::Printf(TEXT("fire-%02d.json"),R->Art),Fire->DescribeEffectsV839());Write(FString::Printf(TEXT("smoke-%02d.json"),R->Art),Smoke);
   if(R->Art==0||(R->Art>=2&&R->Art<=5))
   {
    int32 Receivers=0;bool Correct=true;if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* V:Fleet->GetVehicles())if(V)if(auto* Hull=Cast<UStaticMeshComponent>(V->GetRootComponent()))if(Hull->GetStaticMesh())
    {const auto& Slots=Hull->GetStaticMesh()->GetStaticMaterials();for(int32 Slot=0;Slot<Slots.Num();++Slot)if(Slots[Slot].MaterialSlotName.ToString().Contains(TEXT("White"))){++Receivers;auto* M=Hull->GetMaterial(Slot);Correct&=M&&M->GetBaseMaterial()->GetName().Contains(TEXT("_British_"));}}
    Check(TEXT("inherited_white_geometry_uses_actual_model_paint_")+FString::FromInt(R->Art),Receivers>0&&Correct);
   }
   if(auto* Surface=AFrontlineArmourSurfaceV837::Get(B->GetWorld()))Write(FString::Printf(TEXT("surfaces-%02d.json"),R->Art),Surface->Describe());
   auto Cost=MakeShared<FJsonObject>();Cost->SetNumberField(TEXT("samples"),R->Samples);Cost->SetNumberField(TEXT("game_thread_mean_ms"),R->GameMS/FMath::Max(1,R->Samples));Cost->SetNumberField(TEXT("render_thread_mean_ms"),R->RenderMS/FMath::Max(1,R->Samples));Cost->SetNumberField(TEXT("gpu_mean_ms"),R->GPUMS/FMath::Max(1,R->Samples));Cost->SetStringField(TEXT("scope"),TEXT("Warm offscreen 4K Quality/RR fixed-camera live-and-wreck still-life cost diagnostic, FG off. Not foreground frame-generation or natural battle performance."));Write(FString::Printf(TEXT("cost-%02d.json"),R->Art),Cost);
   Shot(FString::Printf(TEXT("armour-%02d-live-wreck"),R->Art));
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV8310ArtOnly"))){R->Stage=30;R->Next=Now+.1;return true;}
   ++R->Art;R->Stage=R->Art<=R->ArtEnd?2:4;R->Next=Now+.1;return true;
  }
  if(R->Stage==4)
  {
   Clear();B->Phase=EFrontlinePhase::Briefing;Check(TEXT("medal_ui_fixture_in_disposable_account"),FrontlineAchievementsV76::SeedMedalReviewV8215(B,true,3));
   for(int32 Nation=0;Nation<4;++Nation)MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Nation)),true);
   R->Stage=5;R->Next=Now+1.;return true;
  }
  if(R->Stage==5)
  {
   Clear();FrontlineLocalizationV35::SetLanguage(R->Page%2?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   S.AchievementsV76=true;auto& M=MedalWallV8215::View(PC);M.Wall=true;M.Nation=R->Page/2;M.Row=0;const auto Items=MedalWallV8215::Items(EFRRUNationV81(M.Nation));if(!Items.IsEmpty())M.Focus=Items[0];
   const auto Ribbons=MedalWallV8215::OrderedRibbonItems(EFRRUNationV81(M.Nation));if(M.Nation<2)Check(TEXT("wall_has_multiline_ribbon_fixture_" )+FString::FromInt(M.Nation),Ribbons.Num()>3);
   R->Stage=6;R->Next=Now+.65;return true;
  }
  if(R->Stage==6)
  {Shot(FString::Printf(TEXT("medal-wall-%d-%s"),R->Page/2,R->Page%2?TEXT("zh"):TEXT("en")));++R->Page;R->Stage=R->Page<8?5:7;R->Next=Now+.1;return true;}
  if(R->Stage==30){Camera(FVector(1050,-650,210),1900);if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();R->Stage=31;R->Next=Now+2.;return true;}
  if(R->Stage==31){Shot(FString::Printf(TEXT("armour-%02d-wreck-detail"),R->Art));++R->Art;R->Stage=R->Art<=R->ArtEnd?2:7;R->Next=Now+.1;return true;}
  return Finish();
 }),0.f);
}
}
