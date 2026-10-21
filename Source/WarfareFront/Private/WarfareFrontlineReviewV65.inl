// Disposable changed-feature review. Synthetic layout figures are labelled;
// actual damage, persistence and animation assertions run separately below.
struct FFrontlineReviewV65 : public TSharedFromThis<FFrontlineReviewV65>
{
 TWeakObjectPtr<AWarfareFrontlineController> PC;
 TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Characters;
 TWeakObjectPtr<AFrontlineTank> Armour;
 FVector WetFixture=FVector::ZeroVector;
 int32 Stage=0,Captured=-1,Frames=0;bool Good=true,Baseline=false;
 double Next=0,Started=0,SampleStart=0,SampleLast=0,Wall=0,GPU=0,Game=0,Render=0;
 FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TSharedPtr<FJsonObject> Performance;
 void Check(const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V65_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
 bool Shot(const TCHAR* Name){if(Captured==Stage)return false;FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/Name,true,false);Captured=Stage;return true;}
 void Finish(){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetBoolField(TEXT("baseline"),Baseline);J->SetArrayField(TEXT("checks"),Checks);if(Performance)J->SetObjectField(TEXT("render_sample"),Performance);J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);J->SetStringField(TEXT("scope"),TEXT("Changed-feature isolated review. Campaign totals/tooltip values in layout captures are synthetic; damage and receipt assertions are separate. Not a full campaign or listening test."));FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/TEXT("review.json")));FPlatformMisc::RequestExit(false);}
 bool Tick(float DT)
 {
  const double Now=FPlatformTime::Seconds();auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
  if(B&&Stage>=8&&Stage<=11)B->Elapsed+=DT;
  if(Now<Next)return true;
  if(!B||!B->LoadedSoldier||!C->GetHUD()){if(Now-Started>100){Check(TEXT("startup"),false);Finish();return false;}return true;}
  if(Now-Started>420){Check(FString::Printf(TEXT("timeout_stage_%d"),Stage),false);Finish();return false;}
#if WITH_EDITOR
  if(Stage>0&&(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))){Next=Now+.5;return true;}
#endif
  if(Stage>0&&PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0){Next=Now+.5;return true;}
  auto& S=ScreenState(C);auto& Map=WarRoom52::State(C);auto Report=[&](const FString& N,bool P){Check(N,P);};Next=Now+.4;
  auto Battle=[&](int32 Op)
  {
   B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);
   Check(FString::Printf(TEXT("fixture_operation_%d"),Op),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,Op));
   B->SetActorTickEnabled(false);B->BeginBattle();B->SetPaused(false);S.Dossier=1917;
   FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
   FrontlineReviewUIV41::Camera(C,0,0,8600,650,true);V28ReviewPointer=FVector2D(1890,810);
  };
  switch(Stage)
  {
  case 0:Check(TEXT("disposable_profile"),B->IsProgressionVolatile());Battle(7);++Stage;Next=Now+9;break;
  case 1:
   for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
   {
    Check(TEXT("paired_soil_bound_to_visible_tiles"),It->Surface->GetMaterial(0)->GetBaseMaterial()->GetName()==(Baseline?TEXT("M_GroundMud_V59"):TEXT("M_GroundMud_V65")));
    bool Flat=It->BasinWaterV44&&It->BasinWaterV44->GetUpVector().Z>.99999;
#if WITH_EDITOR
    if(It->BasinWaterV44&&It->BasinWaterV44->GetStaticMesh())
    {
     const auto* D=It->BasinWaterV44->GetStaticMesh()->GetRenderData();
     if(D&&D->LODResources.Num())
     {
      const auto& V=D->LODResources[0].VertexBuffers;TMap<int32,FVector2f> Levels;
      if(V.PositionVertexBuffer.GetVertexData()&&V.ColorVertexBuffer.GetVertexData())
      {
       for(uint32 I=0;I<V.PositionVertexBuffer.GetNumVertices();++I)if(V.ColorVertexBuffer.VertexColor(I).B==0)
       {const int32 G=V.ColorVertexBuffer.VertexColor(I).G;const float Z=V.PositionVertexBuffer.VertexPosition(I).Z;if(auto* L=Levels.Find(G)){L->X=FMath::Min(L->X,Z);L->Y=FMath::Max(L->Y,Z);}else Levels.Add(G,FVector2f(Z,Z));}
       for(const auto& L:Levels)Flat&=L.Value.Y-L.Value.X<.02f;
       Check(TEXT("actual_baked_crater_groups_have_level_vertices"),Flat&&Levels.Num()>0);
      }
     }
    }
#endif
    Check(TEXT("water_component_has_no_tilt"),Flat);
   }
   if(!Baseline){UFrontlineWaterResponseV51::RunChecksV64(B->GetWorld(),Report);UFrontlineWaterResponseV51::RunChecksV53(B->GetWorld(),Report);}
   if(Shot(TEXT("01-menin-wide.png")))return true;
   FrontlineReviewUIV41::Camera(C,0,0,4200,650,true);++Stage;Next=Now+3;break;
  case 2:if(Shot(TEXT("02-menin-close.png")))return true;FrontlineReviewUIV41::Camera(C,0,0,8600,650,true);++Stage;Next=Now+3;break;
  case 3:
   Next=0;if(!SampleStart){Performance=MakeShared<FJsonObject>();if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))Performance->SetObjectField(TEXT("settings"),R->GetDiagnostics());SampleStart=SampleLast=Now;break;}
   Wall+=(Now-SampleLast)*1000;SampleLast=Now;GPU+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());Game+=FPlatformTime::ToMilliseconds(GGameThreadTime);Render+=FPlatformTime::ToMilliseconds(GRenderThreadTime);++Frames;
   if(Now-SampleStart<6)break;
   Performance->SetNumberField(TEXT("frames"),Frames);Performance->SetNumberField(TEXT("seconds"),Now-SampleStart);Performance->SetNumberField(TEXT("mean_wall_ms"),Wall/Frames);Performance->SetNumberField(TEXT("mean_gpu_ms"),GPU/Frames);Performance->SetNumberField(TEXT("mean_game_ms"),Game/Frames);Performance->SetNumberField(TEXT("mean_render_ms"),Render/Frames);
   Performance->SetStringField(TEXT("scope"),TEXT("Six-second fixed Menin environment, director disabled. Resolution/RHI given by launch receipt; not combat performance."));
   if(Baseline){Finish();return false;}Battle(12);++Stage;Next=Now+6;break;
  case 4:FrontlineReviewUIV41::Camera(C,0,0,4200,650,true);++Stage;Next=Now+3;break;
  case 5:if(Shot(TEXT("03-lys-water.png")))return true;Battle(0);++Stage;Next=Now+4;break;
  case 6:if(Shot(TEXT("04-locked-slots.png")))return true;FrontlineCombatRecordV65::RunFocusedChecks(B,Report);if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV65AudioChecks(Report);++Stage;break;
  case 7:
   for(auto* U:FrontlineCombatRecordV65::PrepareCharacterReview(B))Characters.Add(U);
   Check(TEXT("six_character_pose_fixtures"),Characters.Num()==6);FrontlineReviewUIV41::Camera(C,0,0,1750,230,true);++Stage;Next=Now+.65;break;
  case 8:Shot(TEXT("05-burning-reaction.png"));++Stage;Next=Now+.65;break;
  case 9:Shot(TEXT("06-burning-collapse.png"));++Stage;Next=Now+3;break;
  case 10:
   if(Characters.Num()&&Characters[0].IsValid())
   {FVector2D P;int32 W=0,H=0;C->GetViewportSize(W,H);C->ProjectWorldLocationToScreen(Characters[0]->Feet()+FVector(0,0,88),P);V28ReviewPointer=FVector2D(P.X*1920/FMath::Max(1,W),P.Y*1080/FMath::Max(1,H));}
   ++Stage;Next=Now+.5;break;
  case 11:if(Shot(TEXT("07-unit-record.png")))return true;Check(TEXT("player_squad_tooltip_visible"),V39LastStatus.Visible&&V39LastStatus.Team==0&&!V39LastStatus.Tank);++Stage;break;
  case 12:
   Battle(19);
   B->CombatRecordV65[0]={};B->CombatRecordV65[0].Total.Infantry=1643;B->CombatRecordV65[0].Total.Armour=39;
   B->CombatRecordV65[0].InfantrySources[0]={713,2};B->CombatRecordV65[0].ArmourSources[0]={487,27};B->CombatRecordV65[0].SupportSources[0]={443,10};B->CombatRecordV65[0].InfantryLost=589;B->CombatRecordV65[0].ArmourLost=16;
   {
    auto* Run=B->CampaignRuns.FindByPredicate([&](const auto& R){return R.Id==B->ActiveCampaignId;});Check(TEXT("campaign_record_fixture"),Run!=nullptr);
    if(Run){const auto Before=Run->CombatRecordV65.Total;B->AwardEncounterResearch(0);const auto Once=Run->CombatRecordV65;B->AwardEncounterResearch(0);Check(TEXT("completed_encounter_receipt_credits_once"),Run->CombatRecordV65.Total.Infantry==Before.Infantry+1643&&Run->CombatRecordV65.Total.Infantry==Once.Total.Infantry&&Run->CombatRecordV65.BattlesRecorded==Once.BattlesRecorded);}
   }
   S.CampaignEndingV51=true;FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);V28ReviewPointer=FVector2D(1830,950);FrontlineReviewUIV41::BeginAudit();++Stage;Next=Now+.8;break;
  case 13:
   if(Shot(TEXT("08-campaign-record-en.png")))return true;
   Check(TEXT("campaign_english_text_fits"),FrontlineReviewUIV41::EndAudit()->GetBoolField(TEXT("success")));
   FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);FrontlineReviewUIV41::BeginAudit();++Stage;Next=Now+.7;break;
  case 14:
   if(Shot(TEXT("09-campaign-record-zh.png")))return true;
   Check(TEXT("campaign_chinese_text_fits"),FrontlineReviewUIV41::EndAudit()->GetBoolField(TEXT("success")));
   Battle(0);FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
   Armour=FrontlineCombatRecordV65::PrepareArmourReview(B);
   Check(TEXT("armour_tooltip_fixture"),Armour.IsValid());FrontlineReviewUIV41::Camera(C,0,0,2400,150,true);++Stage;Next=Now+3;break;
  case 15:
   if(Armour.IsValid())
   {FVector2D P;int32 W=0,H=0;C->GetViewportSize(W,H);C->ProjectWorldLocationToScreen(Armour->AimPoint(),P);V28ReviewPointer=FVector2D(P.X*1920/FMath::Max(1,W),P.Y*1080/FMath::Max(1,H));}
   ++Stage;Next=Now+.5;break;
  case 16:
   if(Shot(TEXT("10-armour-record.png")))return true;
   Check(TEXT("player_armour_tooltip_visible"),V39LastStatus.Visible&&V39LastStatus.Team==0&&V39LastStatus.Tank);
   Battle(7);++Stage;Next=Now+4;break;
  case 17:
   {
    float Best=-MAX_flt;
    for(int32 Y=-1400;Y<=1400;Y+=50)for(int32 X=-1800;X<=1800;X+=50)
    {float Z=0,D=0;if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Z,D)&&D>3.f)
     {
      // The production side camera can hide a valid deep pool behind its lip.
      // Choose a source actually visible along this camera's viewing slope.
      bool Visible=true;for(float Offset=20;Offset<1500&&Visible;Offset+=20)
       Visible=AWarfareFrontlineGround::HeightAt(X,Y+Offset)<Z+2.f+Offset*(4650.f-Z-100.f)/14000.f;
      const float Score=FMath::Min(D,12.f)-.002f*FVector2D(X,Y).Size();
      if(Visible&&Score>Best){Best=Score;WetFixture=FVector(X,Y,Z);}
     }}
    Check(TEXT("blood_visual_uses_real_pool"),Best>-MAX_flt);
    Check(TEXT("blood_visual_real_source_admitted"),UFrontlineWaterResponseV51::NotifyBlood(B->GetWorld(),WetFixture,2.f,200.f,true));
    UFrontlineWaterResponseV51::NotifyImpact(B->GetWorld(),WetFixture,260.f,.5f);
    FrontlineReviewUIV41::Camera(C,WetFixture.X,WetFixture.Y,1900,WetFixture.Z+100,true);
   }
   ++Stage;Next=Now+8;break;
  case 18:if(Shot(TEXT("11-blood-in-water.png")))return true;Finish();return false;
  default:Finish();return false;
  }
  return true;
 }
};
void StartFrontlineV65Review(AWarfareFrontlineController* C)
{
 FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
 auto R=MakeShared<FFrontlineReviewV65>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+5;R->Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV65Baseline"));R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V65/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
