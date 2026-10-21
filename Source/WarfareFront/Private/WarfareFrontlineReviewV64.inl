// One disposable integration run; exact runtime captures are reviewed separately.
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif
struct FFrontlineReviewV64 : public TSharedFromThis<FFrontlineReviewV64>
{
 TWeakObjectPtr<AWarfareFrontlineController> PC;
 int32 Stage=0,Captured=-1,ClickStart=0,Frames=0;bool Good=true,Baseline=false;
 double Next=0,Started=0,SampleStart=0,SampleLast=0,Wall=0,GPU=0,Game=0,Render=0;
 FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TSharedPtr<FJsonObject> Performance;
 void Check(const FString& N,bool P){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("pass"),P);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=P;UE_LOG(LogTemp,Display,TEXT("WF_V64_CHECK %s %s"),*N,P?TEXT("PASS"):TEXT("FAIL"));}
 bool Shot(const TCHAR* Name){if(Captured==Stage)return false;FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/Name,true,false);Captured=Stage;return true;}
 void Finish(){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetBoolField(TEXT("baseline"),Baseline);J->SetArrayField(TEXT("checks"),Checks);if(Performance)J->SetObjectField(TEXT("render_sample"),Performance);J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/TEXT("review.json")));FPlatformMisc::RequestExit(false);}
 bool Tick(float)
 {
  const double Now=FPlatformTime::Seconds();if(Now<Next)return true;
  auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
  if(!B||!B->LoadedSoldier||!C->GetHUD()){if(Now-Started>100){Check(TEXT("startup"),false);Finish();return false;}return true;}
  if(Now-Started>600){Check(FString::Printf(TEXT("timeout_stage_%d"),Stage),false);Finish();return false;}
#if WITH_EDITOR
  if(Stage>0&&(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))){Next=Now+1;return true;}
#endif
  if(Stage>0&&PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.5;return true;}
  auto* HUD=Cast<AWarfareFrontlineHUD>(C->GetHUD());auto& S=ScreenState(C);auto& Map=WarRoom52::State(C);
  auto Report=[&](const FString& N,bool P){Check(N,P);};Next=Now+.7;
  auto Battle=[&](int32 Op)
  {
   B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);
   Check(FString::Printf(TEXT("fixture_operation_%d"),Op),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,Op));
   B->SetActorTickEnabled(false);B->BeginBattle();B->SetPaused(false);S.Dossier=1917;
   Check(TEXT("environment_fixture_enters_fighting"),B->Phase==EFrontlinePhase::Fighting);
   // Fixed-camera helper suspends PlayerTick. Restore the ordinary map-owned
   // lighting before freezing that tick, exactly as production PlayerTick does.
   FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
   FrontlineReviewUIV41::Camera(C,0,0,8600,650,true);V28ReviewPointer=FVector2D(1890,810);
  };
  switch(Stage)
  {
  case 0:
   Check(TEXT("disposable_profile"),B->IsProgressionVolatile());
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV64CombatOnly"))){Battle(8);Stage=90;Next=Now+1;break;}
   if(FParse::Param(FCommandLine::Get(),TEXT("WFV69SitesOnly"))){Stage=17;break;}
   if(Baseline){Stage=10;break;}
   Check(TEXT("map_fixture"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8));B->SetActorTickEnabled(false);
   FrontlineReviewUIV41::Dossier(C);WarRoom52::Synchronize(Map,B);WarRoom52::BeginRoute(Map,B);Map.WasActive=true;
   ++Stage;Next=Now+6;break;
  case 1:
   if(!FrontlineMapSceneV61::IsAtlasReadyV64(C)){Next=Now+1;break;}
   Check(TEXT("atlas_runtime_blocks_and_resolution_ready"),true);
   Check(TEXT("single_geographic_document"),FrontlineMapSceneV61::IsSingleDocumentV64(C));
   if(Shot(TEXT("01-theatre.png")))return true;
   {const auto P=WarRoom52::AtlasPoint(Map,WarRoom52::UV(FVector2D(3.22,49.98)));
    Check(TEXT("sector_interior_click_resolves_without_label"),WarRoom52::AtlasSectorAtPointV61(Map,P)==3);}
   HUD->NotifyHitBoxClick(TEXT("V60Sector3"));++Stage;Next=Now+3.6;break;
  case 2:
   Check(TEXT("sector_flight_keeps_same_document"),Map.AtlasMode==1&&FrontlineMapSceneV61::IsSingleDocumentV64(C));
   if(Shot(TEXT("02-sector.png")))return true;
   Map.Preview=8;Map.Stage=WarRoom52::Stage(8);WarRoom52::FocusOperation(Map,B,WarRoom52::UV(FrontlineCampaignV52::GetNode(B,8).LonLat),false,2.5f);
   ++Stage;Next=Now+3.3;break;
  case 3:
   Check(TEXT("close_flight_finishes_on_same_document"),Map.OperationSheet&&FrontlineMapSceneV61::IsSingleDocumentV64(C));
   {const auto P=WarRoom52::UV(FrontlineCampaignV52::GetNode(B,8).LonLat);Check(TEXT("geographic_draw_and_pick_agree"),FVector2D::Distance(P,FrontlineMapSceneV61::Unproject(C,FrontlineMapSceneV61::Project(C,P)))<.00001);}
   if(Shot(TEXT("03-operation.png")))return true;
   ClickStart=NavigationV61::EmittedClicksV64;HUD->NotifyHitBoxClick(TEXT("V52Begin"));
   Check(TEXT("actual_begin_captures_background"),FrontlineMapSceneV61::LoadingView(C)!=nullptr);++Stage;break;
  case 4:
   if(S.HistoricalDispatch){HUD->NotifyHitBoxClick(TEXT("V52DispatchChoice0"));break;}
   if(S.Deployment){HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));break;}
   if(S.UnitIntroduction){HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));Next=Now+.2;break;}
   Check(TEXT("actual_orders_enter_preparation"),S.Preparing);++Stage;Next=Now+.2;break;
  case 5:
   Check(TEXT("loading_retains_real_camera_background"),S.Preparing&&FrontlineMapSceneV61::LoadingView(C)!=nullptr);
   Shot(TEXT("04-loading.png"));HUD->NotifyHitBoxClick(TEXT("Menu"));
   Check(TEXT("loading_click_is_silent_and_does_not_open_menu"),NavigationV61::EmittedClicksV64==ClickStart&&!S.Menu);++Stage;break;
  case 6:
   if(S.Preparing){Next=Now+.3;break;}
   Check(TEXT("loading_reaches_battle"),B->Phase==EFrontlinePhase::Fighting&&S.PreparationError.IsEmpty());
   {FString Response;FrontlineDebugV37::Execute(B,TEXT("status"),Response);FrontlineDebugV37::Execute(B,TEXT("speed 4"),Response);}
   for(int32 I=0;I<3;++I){HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("BattleRate%d"),I)));FrontlineDebugV37::Tick(B);const float V[]={.5f,1,1.5f};Check(FString::Printf(TEXT("actual_speed_switch_%d_after_console"),I),FMath::IsNearlyEqual(B->BattleSpeed,V[I]));}B->SetBattleSpeed(1);
   FrontlineFieldSitesV59::RunFlagChecksV64(B,Report);
   FrontlineCombatV64::RunChecks(B,Report);B->AbandonBattleToOperations();Stage=10;break;
  case 10:
   Battle(7);++Stage;Next=Now+7;break;
  case 11:
   if(!Baseline)UFrontlineWaterResponseV51::RunChecksV64(B->GetWorld(),Report);
   if(Shot(TEXT("05-menin-wide.png")))return true;
   FrontlineReviewUIV41::Camera(C,0,0,4200,650,true);++Stage;Next=Now+4;break;
  case 12:
   if(Shot(TEXT("06-menin-close.png")))return true;
   FrontlineReviewUIV41::Camera(C,0,0,8600,650,true);++Stage;Next=Now+3;break;
  case 13:
   Next=0;if(SampleStart==0){Performance=MakeShared<FJsonObject>();if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))Performance->SetObjectField(TEXT("settings"),R->GetDiagnostics());SampleStart=SampleLast=Now;break;}
   Wall+=(Now-SampleLast)*1000.;SampleLast=Now;GPU+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());Game+=FPlatformTime::ToMilliseconds(GGameThreadTime);Render+=FPlatformTime::ToMilliseconds(GRenderThreadTime);++Frames;
   if(Now-SampleStart<6.)break;
   Performance->SetNumberField(TEXT("frames"),Frames);Performance->SetNumberField(TEXT("seconds"),Now-SampleStart);Performance->SetNumberField(TEXT("mean_wall_ms"),Wall/Frames);Performance->SetNumberField(TEXT("mean_gpu_ms"),GPU/Frames);Performance->SetNumberField(TEXT("mean_game_ms"),Game/Frames);Performance->SetNumberField(TEXT("mean_render_ms"),Render/Frames);
   Performance->SetStringField(TEXT("scope"),TEXT("Paired six-second 4K fixed Menin environment; director disabled, DLSS Quality, FG off. Measures environment cost, not combat performance or other hardware."));++Stage;break;
  case 14:
   Battle(12);++Stage;Next=Now+7;break;
  case 15:
   if(!Baseline)UFrontlineWaterResponseV51::RunChecksV64(B->GetWorld(),Report);
   if(Shot(TEXT("07-lys-wide.png")))return true;
   FrontlineReviewUIV41::Camera(C,0,0,4200,650,true);++Stage;Next=Now+4;break;
  case 16:
   if(Shot(TEXT("08-lys-close.png")))return true;
   if(Baseline){Finish();return false;}
   Battle(0);++Stage;Next=Now+5;break;
  case 17:
   if(!FParse::Param(FCommandLine::Get(),TEXT("WFV69SitesOnly")))if(Shot(TEXT("09-locked-console.png")))return true;
   Battle(8);
   for(int32 Team=0;Team<2;++Team)
   {
    const int32 Trench=Team==0?B->Trenches.Num()/2:B->GetHomeTrenchIndex(1);
    B->Trenches[Trench].Owner=Team;
    Check(FString::Printf(TEXT("capture_visual_garrison_%d"),Team),B->SpawnSquad(Team,EFrontlineRole::Rifle,Trench));
    auto& Q=B->Squads.Last();Q.Trench=Trench;Q.bAdvancing=Q.bReinforcement=false;
    int32 I=0;for(const auto& W:Q.Members)if(auto* U=W.Get())
    {const float Y=-800+I++*135.f,X=FrontlineFieldV32::TrenchCentreX(Trench,Y);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));}
   }
   for(int32 I=0;I<52;++I){B->Elapsed+=.25f;FrontlineFieldSitesV59::Tick(B,.25f);}
   {int32 Owned=0;for(const auto& V:FrontlineFieldSitesV59::Views(B))if(V.Kind!=FrontlineFieldSitesV59::EKind::FieldWorks&&V.Owner>=0)++Owned;Check(TEXT("two_posts_captured_by_actual_garrisons"),Owned>=2);}
   C->SupportTargetMode=FParse::Param(FCommandLine::Get(),TEXT("WFV69SitesOnly"))?0:1;C->bPreviewValid=true;C->bTargetingArtillery=true;
   C->ArtilleryPreview=FVector(1000,400,AWarfareFrontlineGround::HeightAt(1000,400));
   FrontlineReviewUIV41::Camera(C,800,-300,6800,650,true);++Stage;Next=Now+5;break;
  case 18:
   if(Captured!=Stage&&FParse::Param(FCommandLine::Get(),TEXT("WFV69SitesOnly")))FrontlineFieldSitesV59::RunFlagChecksV64(B,Report);
   if(Shot(TEXT("10-owned-flags-and-sight.png")))return true;
   Finish();return false;
  case 90:
   FrontlineCombatV64::RunChecks(B,Report);Finish();return false;
  default:Finish();return false;
  }
  return true;
 }
};
void StartFrontlineV64Review(AWarfareFrontlineController* Controller)
{
 FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV64Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
 auto R=MakeShared<FFrontlineReviewV64>();R->PC=Controller;R->Started=FPlatformTime::Seconds();R->Next=R->Started+5;R->Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline"));
 R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V64/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
