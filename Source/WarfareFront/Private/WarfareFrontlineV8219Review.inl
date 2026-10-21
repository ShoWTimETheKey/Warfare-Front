#include "WarfareFrontlineV8219TimeBudgetReview.inl"
#include "WarfareFrontlineArmourObservationChecksV8219.inl"
#include "WarfareFrontlineGameplayV51.h"

namespace
{
void RecordPreparedSurfaceBindingsV8219(AWarfareFrontlineGameMode* B,const FString& Dir,TFunctionRef<void(const FString&,bool)> Check)
{
 if(!FrontlineEffectsReviewSafetyV8216::Allowed(B))return;
 TArray<TSharedPtr<FJsonValue>> Rows;bool Found=false,Complete=true;
 for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
 {
  auto* G=*It;if(G->BakedTerrainV43.IsEmpty())continue;Found=true;
  auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("ground_material"),GetPathNameSafe(G->GroundMaterial));
  TArray<TSharedPtr<FJsonValue>> Tiles;UMaterialInterface* Expected=IsValid(G->BakedTerrainV43[0])?G->BakedTerrainV43[0]->GetMaterial(0):nullptr;
  for(UStaticMeshComponent* Tile:G->BakedTerrainV43)
  {
   if(!IsValid(Tile)){Complete=false;continue;}
   auto T=MakeShared<FJsonObject>();T->SetStringField(TEXT("mesh"),GetPathNameSafe(Tile->GetStaticMesh()));T->SetStringField(TEXT("displayed_material"),GetPathNameSafe(Tile->GetMaterial(0)));
   T->SetBoolField(TEXT("ray_tracing_visible"),Tile->bVisibleInRayTracing);Tiles.Add(MakeShared<FJsonValueObject>(T));Complete&=Tile->IsRegistered()&&Expected&&Tile->GetMaterial(0)==Expected;
  }
  J->SetArrayField(TEXT("displayed_tiles"),Tiles);J->SetStringField(TEXT("water_contract"),AWarfareFrontlineGround::DescribeWaterV72());
  if(IsValid(G->BasinWaterV44)){J->SetStringField(TEXT("water_mesh"),GetPathNameSafe(G->BasinWaterV44->GetStaticMesh()));J->SetStringField(TEXT("water_material"),GetPathNameSafe(G->BasinWaterV44->GetMaterial(0)));}
  Rows.Add(MakeShared<FJsonValueObject>(J));
 }
 Check(TEXT("v8219_actual_prepared_tiles_share_the_displayed_material"),Found&&Complete);
 auto Report=MakeShared<FJsonObject>();Report->SetArrayField(TEXT("grounds"),Rows);FString JSON;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&JSON));
 FFileHelper::SaveStringToFile(JSON,*(Dir/(B->GetPlayerFaction()==EFrontlineFaction::German?TEXT("german-surface-bindings.json"):TEXT("british-surface-bindings.json"))));
}
namespace ArmourProbeV8219
{bool Wanted=false,Done=false;TFunction<void(const FString&,bool)> Check;}
void ProbeFrontlineCanvasV8219Review(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 if(!ArmourProbeV8219::Wanted||ArmourProbeV8219::Done||!FrontlineEffectsReviewSafetyV8216::Allowed(B))return;
 const auto& Draw=ArmourVariantUIReviewV8219::Last;if(Draw.Canvas!=UI.Surface||Draw.Frame!=GFrameCounter)return;
 const auto* Button=Draw.Buttons.FindByPredicate([](const auto& P){return P.Target==EFrontlineVehicle::A7VGretchen&&!P.Enabled;});
 if(!Button)return;
 if(!Button->Hovered){V28ReviewPointer=FVector2D(Button->X+Button->W*.5f,Button->Y+Button->H*.5f);return;}
 if(ArmourProbeV8219::Check)ArmourVariantUIReviewV8219::WithCanvas(UI,B,Button->Target,ArmourProbeV8219::Check);
 ArmourProbeV8219::Done=true;
}
void StartFrontlineV8219Review(AWarfareFrontlineController* Controller)
{
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review")))return;
 FString User;if(!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{int32 Stage=0,Shots=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2.,Prepared=0;FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V8219_CHECK %s %d"),*Name,int32(Pass));};
  ArmourProbeV8219::Check=Check;
  auto Finish=[&](){V28ReviewPointer.Reset();auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("V82.19 actual isolated Shipping UK/DE time budgets, ended-encounter restart and failed-save retry, prepared terrain/materials, bilingual affected armour pages. No French/Russian campaigns or whole-campaign/performance certification."));FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("checks.json")));FPlatformMisc::RequestExit(false);return false;};
  if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("v8219_strict_disposable_world"),false);return Finish();}
  if(Now-R->Began>150.){Check(TEXT("v8219_bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;S.Dossier=1917;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;MedalMountV8216::View(PC).Picker=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);};
  auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(FString(Name)+TEXT(".png")),true,false);++R->Shots;};
  auto Prepare=[&](EFrontlineFaction Nation){Clear();B->Phase=EFrontlinePhase::Briefing;B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);Check(TEXT("v8219_campaign_fixture_created"),B->StartNewCampaign(Nation,EFrontlineDifficulty::Veteran));if(auto* C=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){C->CompletedMissions=19;C->WonOperationMaskV52=(1u<<19)-1;C->DispatchResolvedMaskV52=(1u<<20)-1;C->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;C->Research.ResearchPoints=60;C->ArmourResearchMaskV63=0;}Check(TEXT("v8219_normal_operation_selected"),B->SelectCampaignMission(7));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;};
  auto Ready=[&](){
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("phase"),int32(B->Phase));J->SetBoolField(TEXT("preparing"),S.Preparing);J->SetBoolField(TEXT("painted"),S.PreparationPainted);J->SetBoolField(TEXT("paused"),UGameplayStatics::IsGamePaused(B));J->SetBoolField(TEXT("controller_tick"),PC->IsActorTickEnabled());J->SetBoolField(TEXT("battle_tick"),B->IsActorTickEnabled());J->SetStringField(TEXT("preparation_error"),S.PreparationError);J->SetStringField(TEXT("asset_failure"),B->AssetFailure);J->SetNumberField(TEXT("textures"),S.PreparationTextures.Num());J->SetNumberField(TEXT("resident_textures"),ResidentOperationTextures(S));J->SetNumberField(TEXT("scene_frames"),S.PreparationSceneFrames);J->SetNumberField(TEXT("scene_seconds"),S.PreparationFirstSceneAt>0?Now-S.PreparationFirstSceneAt:0.);J->SetNumberField(TEXT("global_pending_pso"),PipelineStateCache::GetNumActivePipelinePrecompileTasks());J->SetBoolField(TEXT("world_render_disabled"),B->GetWorld()->GetGameViewport()->bDisableWorldRendering);FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/(B->GetPlayerFaction()==EFrontlineFaction::German?TEXT("german-preparation.json"):TEXT("british-preparation.json"))));
   if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<65.)return 0;const bool Good=B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty();Check(TEXT("v8219_actual_prepared_battle"),Good);if(!Good)return -1;B->SetActorTickEnabled(false);Clear();return 1;};
  auto Deployment=[&](bool Chinese){Clear();FrontlineLocalizationV35::SetLanguage(Chinese?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);B->Phase=EFrontlinePhase::Briefing;Check(TEXT("v8219_late_armour_page_without_automatic_research"),B->SelectCampaignMission(19));S.Deployment=true;S.DeploymentTabV63=1;V28ReviewPointer=FVector2D(1180,674);};
  double Delay=.75;
  switch(R->Stage)
  {
   case 0:Clear();B->SetActorTickEnabled(false);B->Phase=EFrontlinePhase::Briefing;FrontlineAchievementsV76::RunRestartTailChecksV8219(B,Check);FrontlineAchievementsV76::RunInitialOutcomeRetryChecksV8219(B,Check);if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);}FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Prepare(EFrontlineFaction::British);Delay=.1;break;
   case 1:{const int32 State=Ready();if(State==0){R->Next=Now+.1;return true;}if(State<0)return Finish();RunFrontlineTimeBudgetChecksV8219(B,Check);RunFrontlineArmourObservationChecksV8219(B,Check);FrontlineFieldSitesV59::RunAirCoverChecksV8219(B,Check);FrontlineFieldV32::RunWorkTimeBudgetChecksV8219(B,Check);FrontlineGameplayV51::RunObservationTimeBudgetChecksV8219(B,Check);UFrontlineWaterResponseV51::RunLifecycleChecksV8219(B->GetWorld(),Check);RecordPreparedSurfaceBindingsV8219(B,R->Dir,Check);PC->ResetCamera();Delay=1.;break;}
   case 2:Shot(TEXT("01-british-prepared-field-en"));break;
   case 3:Deployment(false);break;
   case 4:Shot(TEXT("02-british-configuration-en"));break;
   case 5:Deployment(true);break;
   case 6:Shot(TEXT("03-british-configuration-zh"));break;
   case 7:Clear();S.Technology=true;S.TechnologyPage=2;V28ReviewPointer=FVector2D(910,810);break;
   case 8:Shot(TEXT("04-british-armour-school-zh"));break;
   case 9:Prepare(EFrontlineFaction::German);Delay=.1;break;
   case 10:{const int32 State=Ready();if(State==0){R->Next=Now+.1;return true;}if(State<0)return Finish();RecordPreparedSurfaceBindingsV8219(B,R->Dir,Check);PC->ResetCamera();Delay=1.;break;}
   case 11:Shot(TEXT("05-german-prepared-field-zh"));break;
   case 12:Deployment(false);ArmourProbeV8219::Wanted=true;ArmourProbeV8219::Done=false;break;
   case 13:Check(TEXT("v8219_disabled_gretchen_tooltip_presented_en"),ArmourProbeV8219::Done);ArmourProbeV8219::Wanted=false;Shot(TEXT("06-german-configuration-en"));break;
   case 14:Deployment(true);ArmourProbeV8219::Wanted=true;ArmourProbeV8219::Done=false;break;
   case 15:Check(TEXT("v8219_disabled_gretchen_tooltip_presented_zh"),ArmourProbeV8219::Done);ArmourProbeV8219::Wanted=false;Shot(TEXT("07-german-configuration-zh"));break;
   case 16:Clear();S.Technology=true;S.TechnologyPage=2;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);V28ReviewPointer=FVector2D(910,810);break;
   case 17:Shot(TEXT("08-german-armour-school-en"));break;
   default:Clear();Check(TEXT("v8219_ui_resources_ready"),FrontlineUIAssetsV8213::PreloadReady());return Finish();
  }
  ++R->Stage;R->Next=Now+Delay;return true;
 }),0.f);
}
}
