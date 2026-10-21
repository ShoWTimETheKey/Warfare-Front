#include "WarfareFrontlineReviewV54.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineWarRoomV52.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContentStreaming.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PipelineStateCache.h"
#include "RHI.h"
#include "RenderTimer.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif

namespace
{
// Opt-in, fixed-camera comparison only. Keep the timing sources identical to
// V22; engine GPU/thread timings are delayed and are not matched wall frames.
struct FReviewPerformance54
{
 double Duration=6.,Started=0.,Last=0.,SettlingSince=0.;
 int32 Restarts=0;
 TArray<double> Wall,GPU,Game,Render;
 TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
 explicit FReviewPerformance54(double Seconds):Duration(Seconds)
 {
  // Avoid growing buffers inside the measured window, even on an uncapped GPU.
  const int32 Capacity=FMath::CeilToInt(Duration*1000.);
  Wall.Reserve(Capacity);GPU.Reserve(Capacity);Game.Reserve(Capacity);Render.Reserve(Capacity);
  Report->SetNumberField(TEXT("scene_index"),12);
  Report->SetNumberField(TEXT("requested_seconds"),Duration);
  Report->SetBoolField(TEXT("completed"),false);
  Report->SetBoolField(TEXT("editor_build"),WITH_EDITOR!=0);
  Report->SetBoolField(TEXT("shipping_build"),UE_BUILD_SHIPPING!=0);
  Report->SetStringField(TEXT("scope"),TEXT("Scene 12 fixed-camera terrain comparison with battle simulation disabled. Loading, compilation and screenshot capture excluded. Wall intervals include the configured cap and offscreen scheduling; delayed RHI GPU and engine thread timings are separate distributions. Editor/Development results are not Shipping or active-combat FPS guarantees, and do not measure generated/presented frames."));
 }
 void Reset()
 {
  if(Started>0.)++Restarts;
  Started=Last=0.;Wall.Reset();GPU.Reset();Game.Reset();Render.Reset();
 }
 static TSharedRef<FJsonObject> Stats(TArray<double> Values)
 {
  auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());
  J->SetBoolField(TEXT("available"),!Values.IsEmpty());
  if(!Values.IsEmpty())
  {
   double Sum=0.;for(double V:Values)Sum+=V;
   Values.Sort();
   auto Percentile=[&](double P){const double At=P*(Values.Num()-1);const int32 A=FMath::FloorToInt(At),B=FMath::CeilToInt(At);return FMath::Lerp(Values[A],Values[B],At-A);};
   J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());
   J->SetNumberField(TEXT("p50_ms"),Percentile(.50));
   J->SetNumberField(TEXT("p95_ms"),Percentile(.95));
   J->SetNumberField(TEXT("p99_ms"),Percentile(.99));
   J->SetNumberField(TEXT("max_ms"),Values.Last());
  }
  return J;
 }
 // Return true when the timed window is finished; the caller bounds settling.
 bool Tick(double Now,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
 {
  if(Started<=0.)
  {
   int32 W=0,H=0;PC->GetViewportSize(W,H);
   Report->SetNumberField(TEXT("viewport_width"),W);Report->SetNumberField(TEXT("viewport_height"),H);
   Report->SetStringField(TEXT("adapter"),GRHIAdapterName);
   if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
    Report->SetObjectField(TEXT("render_settings_start"),Settings->GetDiagnostics());
   // Diagnostics can synchronize with the renderer: begin after that work.
   Started=Last=FPlatformTime::Seconds();
   return false;
  }
  const double Interval=(Now-Last)*1000.;Last=Now;
  if(Interval>0.&&FMath::IsFinite(Interval))Wall.Add(Interval);
  const double GPUTime=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
  if(GPUTime>0.&&FMath::IsFinite(GPUTime))GPU.Add(GPUTime);
  const double GameTime=FPlatformTime::ToMilliseconds(GGameThreadTime);
  const double RenderTime=FPlatformTime::ToMilliseconds(GRenderThreadTime);
  if(GameTime>0.&&FMath::IsFinite(GameTime))Game.Add(GameTime);
  if(RenderTime>0.&&FMath::IsFinite(RenderTime))Render.Add(RenderTime);
  if(Now-Started<Duration)return false;
  Report->SetBoolField(TEXT("completed"),true);
  Report->SetNumberField(TEXT("sample_seconds"),Now-Started);
  Report->SetNumberField(TEXT("frame_count"),Wall.Num());
  Report->SetNumberField(TEXT("settling_restarts"),Restarts);
  Report->SetObjectField(TEXT("wall_frame_times"),Stats(Wall));
  Report->SetObjectField(TEXT("rhi_gpu_times"),Stats(GPU));
  Report->SetObjectField(TEXT("game_thread_times"),Stats(Game));
  Report->SetObjectField(TEXT("render_thread_times"),Stats(Render));
  if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
   Report->SetObjectField(TEXT("render_settings_end"),Settings->GetDiagnostics());
  return true;
 }
};
bool Allowed54(const AWarfareFrontlineGameMode* B)
{
 FString UserDir;
 return B&&B->GetWorld()&&B->IsProgressionVolatile()
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
  &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)
  &&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
}
bool Compiled54()
{
#if WITH_EDITOR
 if(FAssetCompilingManager::Get().GetNumRemainingAssets()
  ||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))return false;
#endif
 return GUsingNullRHI||PipelineStateCache::GetNumActivePipelinePrecompileTasks()==0;
}
bool Ready54(const AWarfareFrontlineGameMode* B)
{return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
const TCHAR* ShotName54(int32 Scene)
{
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV61Review"))&&Scene>=5)return Scene==5?TEXT("06-main-menu-v61.png"):TEXT("07-exit-confirmation-v61.png");
 static const TCHAR* Names[]={
  TEXT("01-western-front-overview.png"),TEXT("02-campaign-stage.png"),
  TEXT("03-registered-sector-map.png"),TEXT("04-operation-sheet.png"),
  TEXT("05-loading-sheet-carryover.png"),TEXT("06-messines-mine-ridge-landform.png"),
  TEXT("07-ypres-marsh-landform.png"),TEXT("08-st-quentin-canal-landform.png"),
  TEXT("09-cambrai-churned-approach.png"),TEXT("10-bourlon-ruined-cellar.png"),
  TEXT("11-selle-rail-cutting.png"),TEXT("12-winter-furrows.png"),
  TEXT("13-menin-road-gullies.png"),TEXT("14-lys-wash-fords.png"),
  TEXT("15-villers-village-crossroads.png")};
 return Scene>=0&&Scene<15?Names[Scene]:TEXT("invalid-scene.png");
}
float Feature54(int32 Scene,const FFrontlineTerrainFeaturesV54& F)
{
 if(Scene==5)return F.RidgeCrest;
 if(Scene==6)return F.MarshChannel;
 if(Scene==7)return F.CanalCut;
 if(Scene==8)return F.TankChurn;
 if(Scene==9)return F.RuinCellar;
 if(Scene==10)return F.RailShoulder;
 if(Scene==11)return F.SnowDrift;
 if(Scene==12)return F.WoodlandGully;
 if(Scene==13)return F.MarshChannel;
 if(Scene==14)return F.VillageRoad;
 return 0.f;
}
FVector FindLandform54(int32 Scene)
{
 float Best=0.f;FVector Point=FVector::ZeroVector;
 for(float X=-4500.f;X<=4500.f;X+=150.f)for(float Y=-1900.f;Y<=1300.f;Y+=130.f)
 {
  const float M=Feature54(Scene,AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y));
  if(M>Best){Best=M;Point=FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y));}
 }
 return Point;
}
bool Field54(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,int32 Scene,FVector& Focus)
{
 const int32 Op=Scene==5?3:Scene==6?5:Scene==7?16:Scene==8?8:Scene==9?9:Scene==10?17:Scene==12?7:Scene==13?12:Scene==14?13:10;
 if(Scene==5)FrontlineWarRoomV52::FinishLoadingReviewV54(PC);
 if(!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,Op))return false;
 B->BeginBattle();B->SetActorTickEnabled(false);UGameplayStatics::SetGamePaused(B,false);
 if(B->Phase!=EFrontlinePhase::Fighting||!B->AssetFailure.IsEmpty())return false;
 Focus=FindLandform54(Scene);
 if(Scene>=6&&Feature54(Scene,AWarfareFrontlineGround::QueryTerrainFeaturesV54(Focus.X,Focus.Y))<.12f)return false;
 // Messines is deliberately the normal broad side-on camera, rather than a
 // macro crop that could disguise the mined ridge with a backdrop.
 if(Scene==5)Focus=FVector(0,0,AWarfareFrontlineGround::HeightAt(0,0));
 if(Scene==12)Focus=FVector(0,0,AWarfareFrontlineGround::HeightAt(0,0));
 if(Scene==13)
 {
  const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
  Focus=FVector(-.45f*Home,0,AWarfareFrontlineGround::HeightAt(-.45f*Home,0));
 }
 if(Scene==14)
 {
  const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
  const float AtX=.40f*Home, RoadY=90.f+.085f*AtX+125.f*FMath::Sin(AtX/1590.f+.4f);
  Focus=FVector(AtX,RoadY-450.f,AWarfareFrontlineGround::HeightAt(AtX,RoadY-450.f));
 }
 float ReviewFocusX=0.f;
 if(FParse::Value(FCommandLine::Get(),TEXT("WFV56ReviewFocusX="),ReviewFocusX))
  Focus.X=ReviewFocusX;
 FrontlineReviewUIV41::Clear(PC);
 FrontlineReviewUIV41::Camera(PC,Focus.X,Focus.Y,Scene==5?8300.f:7000.f,650.f,true);
 return true;
}
TSharedRef<FJsonObject> FieldState54(AWarfareFrontlineGameMode* B,FVector Focus)
{
 auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("camera_focus"),Focus.ToString());
 int32 Tiles=0,Current=0;
 float MeninRoadDistance=MAX_flt,MeninGullyDistance=MAX_flt;
 int32 MeninRoadB=-1,MeninGullyB=-1;
 int32 MeninReadableTiles=0;
 FString MeninProbeUnavailableReason;
 const auto* Mission=B->GetSelectedCampaignMission();
 const bool ProbeMenin=Mission&&Mission->Id==7;
 const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
 const float ProbeX=-.36f*Home;
 float RoadY=-150.f+.115f*ProbeX+125.f*FMath::Sin(ProbeX/1450.f+.42f);
 int32 MeninTerrainVersion=0;
 for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
 {
  for(const UStaticMeshComponent* C:It->BakedTerrainV43)if(C&&C->GetStaticMesh()&&C->IsVisible())
  {
   ++Tiles;
   const FString Path=C->GetStaticMesh()->GetPathName();
   if(Path.StartsWith(TEXT("/Game/Warfare/Field/V54/Terrain/"))
    ||Path.StartsWith(TEXT("/Game/Warfare/Field/V57/Terrain/"))
    ||Path.StartsWith(TEXT("/Game/Warfare/Field/V58/Terrain/"))
    ||Path.StartsWith(TEXT("/Game/Warfare/Field/V59/Terrain/")))++Current;
   if(ProbeMenin&&MeninTerrainVersion==0)
   {
    MeninTerrainVersion=Path.StartsWith(TEXT("/Game/Warfare/Field/V59/Terrain/"))?59:
     Path.StartsWith(TEXT("/Game/Warfare/Field/V58/Terrain/"))?58:
     Path.StartsWith(TEXT("/Game/Warfare/Field/V57/Terrain/"))?57:0;
    if(MeninTerrainVersion==59)
     RoadY=-300.f+1.20f*ProbeX+145.f*FMath::Sin(ProbeX/1950.f+.42f);
   }
   if(UMaterialInterface* Material=C->GetMaterial(0))
   {
    if(const UMaterial* Base=Material->GetBaseMaterial())
     J->SetStringField(TEXT("ground_base_material"),Base->GetPathName());
    float FieldSeed=0.f;
    if(Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FieldSeed")),FieldSeed))
     J->SetNumberField(TEXT("ground_field_seed"),FieldSeed);
   }
   if(ProbeMenin)
   {
    const UStaticMesh* Mesh=C->GetStaticMesh();
    const auto* Render=Mesh->GetRenderData();
    const auto* LOD=Render&&Render->LODResources.Num()?&Render->LODResources[0]:nullptr;
    if(LOD)
    {
     const auto& Positions=LOD->VertexBuffers.PositionVertexBuffer;
     const auto& Colors=LOD->VertexBuffers.ColorVertexBuffer;
     // Cooked render buffers may retain their vertex count after releasing
     // CPU storage. This optional colour probe must not dereference them or
     // force twenty terrain partitions to keep an extra CPU copy for review.
     bool CPUAccessPermitted=Mesh->bAllowCPUAccess;
#if WITH_EDITOR
     CPUAccessPermitted=true;
#endif
     const bool CPUReadable=CPUAccessPermitted
      &&Positions.GetAllowCPUAccess()&&Colors.GetAllowCPUAccess()
      &&Positions.GetVertexData()!=nullptr&&Colors.GetVertexData()!=nullptr
      &&Positions.GetNumVertices()>0&&Colors.GetNumVertices()==Positions.GetNumVertices();
     if(CPUReadable)
     {
      ++MeninReadableTiles;
      for(uint32 V=0;V<Positions.GetNumVertices();++V)
      {
       const FVector3f P=Positions.VertexPosition(V);
       const float DX=P.X-ProbeX;
       const float RoadDistance=DX*DX+FMath::Square(P.Y-RoadY);
       const float GullyDistance=DX*DX+FMath::Square(P.Y-RoadY-970.f);
       if(RoadDistance<MeninRoadDistance){MeninRoadDistance=RoadDistance;MeninRoadB=Colors.VertexColor(V).B;}
       if(GullyDistance<MeninGullyDistance){MeninGullyDistance=GullyDistance;MeninGullyB=Colors.VertexColor(V).B;}
      }
     }
     else if(MeninProbeUnavailableReason.IsEmpty())
      MeninProbeUnavailableReason=CPUAccessPermitted
       ?TEXT("Terrain position/colour CPU buffers are not retained, resident, or equally sized; optional vertex-colour sampling is unavailable.")
       :TEXT("Cooked terrain does not promise CPU vertex access. Optional vertex-colour sampling is unavailable; GPU terrain and collision checks remain active.");
    }
    else if(MeninProbeUnavailableReason.IsEmpty())
     MeninProbeUnavailableReason=TEXT("Terrain has no readable LOD 0 render data for the optional vertex-colour probe.");
   }
  }
  TSharedPtr<FJsonObject> Terrain;
  if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(It->DescribeTerrainV43()),Terrain)&&Terrain)
   J->SetObjectField(TEXT("terrain"),Terrain);
 }
 J->SetNumberField(TEXT("visible_tiles"),Tiles);J->SetNumberField(TEXT("baked_visible_tiles"),Current);
 if(ProbeMenin)
 {
  const bool ProbeAvailable=Tiles>0&&MeninReadableTiles==Tiles&&MeninRoadB>=0&&MeninGullyB>=0;
  J->SetBoolField(TEXT("menin_vertex_probe_available"),ProbeAvailable);
  J->SetNumberField(TEXT("menin_vertex_probe_readable_tiles"),MeninReadableTiles);
  J->SetStringField(TEXT("menin_vertex_probe_status"),ProbeAvailable?TEXT("available"):TEXT("unavailable"));
  if(ProbeAvailable)
  {
   J->SetNumberField(TEXT("menin_road_b_unorm8"),MeninRoadB);
   J->SetNumberField(TEXT("menin_gully_b_unorm8"),MeninGullyB);
   J->SetNumberField(TEXT("menin_road_probe_distance_cm"),FMath::Sqrt(MeninRoadDistance));
   J->SetNumberField(TEXT("menin_gully_probe_distance_cm"),FMath::Sqrt(MeninGullyDistance));
  }
  else
  {
   J->SetStringField(TEXT("menin_vertex_probe_unavailable_reason"),MeninProbeUnavailableReason.IsEmpty()
    ?TEXT("No complete CPU-readable terrain tile set exists for this optional vertex-colour probe.")
    :MeninProbeUnavailableReason);
   for(const TCHAR* Name:{TEXT("menin_road_b_unorm8"),TEXT("menin_gully_b_unorm8"),
     TEXT("menin_road_probe_distance_cm"),TEXT("menin_gully_probe_distance_cm")})
    J->SetField(Name,MakeShared<FJsonValueNull>());
  }
  J->SetNumberField(TEXT("menin_visible_terrain_version"),MeninTerrainVersion);
  J->SetNumberField(TEXT("menin_road_axis_y_cm"),RoadY);
  J->SetObjectField(TEXT("menin_environment"),FrontlineEnvironmentV42::RunChecks(B));
 }
 return J;
}
bool RegionalMaps54()
{
 for(int32 I=0;I<5;++I)
 {
  const FString Name=FString::Printf(TEXT("T_Region_%d"),I);
  const FString Path=FString::Printf(TEXT("/Game/Warfare/Campaign/V54/%s.%s"),*Name,*Name);
  const UTexture2D* Map=LoadObject<UTexture2D>(nullptr,*Path);
  UE_LOG(LogTemp,Display,TEXT("WF_V54_MAP_TILE stage=%d loaded=%d size=%dx%d path=%s"),I,Map?1:0,
   Map?Map->GetSizeX():0,Map?Map->GetSizeY():0,*Path);
  // Runtime GetSizeX reports the 32px placeholder while the imported 4K
  // texture is still compiling/streaming. Source dimensions and source hash
  // are checked by the separate regional-map QA manifest before import.
  if(!Map)return false;
 }
 return true;
}
}

void StartFrontlineV54Review(AWarfareFrontlineGameMode* B)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFV54Review")))return;
 struct FRun
 {
  TWeakObjectPtr<AWarfareFrontlineGameMode> B;
  double Start=FPlatformTime::Seconds(),Next=0.,Quiet=0.;
  int32 Scene=0,EndScene=15,Step=0;bool Good=true,Checked=false,SingleScene=false;
  FVector Focus=FVector::ZeroVector;FString Dir,Shot;
  TArray<TSharedPtr<FJsonValue>> Checks,Scenes;
  TUniquePtr<FReviewPerformance54> Performance;
 };
 auto R=MakeShared<FRun>();R->B=B;
 const bool V61=FParse::Param(FCommandLine::Get(),TEXT("WFV61Review"));
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV60Review")))R->EndScene=5;
 if(V61)R->EndScene=7;
 int32 RequestedScene=INDEX_NONE;
 if(FParse::Value(FCommandLine::Get(),TEXT("WFV55ReviewScene="),RequestedScene)
  &&RequestedScene>=0&&RequestedScene<15)
 {R->Scene=RequestedScene;R->EndScene=RequestedScene+1;R->SingleScene=true;}
 float PerfSeconds=0.f;
 if(FParse::Value(FCommandLine::Get(),TEXT("WFReviewPerfSeconds="),PerfSeconds)
  &&FMath::IsFinite(PerfSeconds)&&PerfSeconds>0.f&&!GUsingNullRHI
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV54Art"))
  &&((V61&&R->Scene<=3&&R->EndScene>3)||(!V61&&R->Scene<=12&&R->EndScene>12)))
  R->Performance=MakeUnique<FReviewPerformance54>(FMath::Clamp(PerfSeconds,.5f,30.f));
 if(V61&&R->Performance){R->Performance->Report->SetNumberField(TEXT("scene_index"),3);R->Performance->Report->SetStringField(TEXT("scope"),TEXT("V61 warmed 4K main-view map-table rendering. Offscreen Editor scheduling and no generated frames; not a gameplay or hardware-compatibility benchmark."));}
 R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V54/Review"));
 IFileManager::Get().MakeDirectory(*R->Dir,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,V61](float)
 {
  auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();
  const auto Check=[&](const FString& Name,bool Passed)
  {
   auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);
   R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Passed;
   UE_LOG(LogTemp,Display,TEXT("WF_V54_CHECK %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Name);
  };
  const auto Finish=[&]()
  {
   auto J=MakeShared<FJsonObject>();int32 Passed=0;
   for(const auto& C:R->Checks)Passed+=C->AsObject()->GetBoolField(TEXT("passed"));
   J->SetBoolField(TEXT("success"),R->Good);J->SetNumberField(TEXT("passed"),Passed);J->SetNumberField(TEXT("total"),R->Checks.Num());
   J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);J->SetArrayField(TEXT("checks"),R->Checks);
   J->SetArrayField(TEXT("art_scenes"),R->Scenes);J->SetBoolField(TEXT("null_rhi_logic_only"),GUsingNullRHI);
   if(R->Performance)J->SetObjectField(TEXT("performance"),R->Performance->Report);
   J->SetStringField(TEXT("scope"),V61
    ?TEXT("V61 isolated map projection, menu navigation, modal and pause checks; seven production 4K map-table, preparation and main-menu captures. Player progress is not written. Visual captures require inspection; checks are not an aesthetic certification."):FParse::Param(FCommandLine::Get(),TEXT("WFV60Review"))
    ?TEXT("V60 isolated loadout, live engineering and gun-position checks; production atlas, mid-flight, dossier and preparation captures. Player progress is never written. Visual captures require human inspection; checks are not an aesthetic or whole-campaign balance certification."):R->SingleScene
    ?TEXT("Focused V55 single-scene 4K visual inspection with isolated V54 topographic rules. Screenshot evidence is not an autonomous aesthetic judgment or live-play benchmark.")
    :TEXT("Isolated topographic rules and fifteen changed-page captures. Same production map, operation sheet, loading page and baked combat terrain. Screenshots are visual review evidence, not an autonomous aesthetic judgment or live-play benchmark."));
   FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));
   FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
   UE_LOG(LogTemp,Display,TEXT("WF_V54_REVIEW_DONE success=%d passed=%d total=%d"),R->Good,Passed,R->Checks.Num());
   FPlatformMisc::RequestExit(false);
  };
  if(!B||Now-R->Start>600.){Check(TEXT("v54_bounded_review"),false);Finish();return false;}
  if(B->HasActorBegunPlay()&&!B->AssetFailure.IsEmpty())
  {Check(TEXT("v54_required_assets_loaded: ")+B->AssetFailure,false);Finish();return false;}
  if(!Ready54(B))return true;
  if(!Allowed54(B)){Check(TEXT("v54_requires_volatile_isolated_profile"),false);Finish();return false;}
  auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
  if(!PC){Check(TEXT("v54_player_controller"),false);Finish();return false;}
  if(!R->Checked)
  {
   const bool Field=FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,0);
   Check(TEXT("v54_production_campaign_fixture"),Field);
   if(Field)
   {
    B->BeginBattle();B->SetActorTickEnabled(false);
    Check(TEXT("v54_production_field_ready"),B->Phase==EFrontlinePhase::Fighting);
    if(V61)
    {B->AbandonBattleToOperations();if(FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8))FrontlineWarRoomV52::RunFlightChecksV54(PC,Check);}
    else if(FParse::Param(FCommandLine::Get(),TEXT("WFV60Review")))
    {
     FrontlineCampaignV32::RunLoadoutChecksV60(B,Check);
     B->AbandonBattleToOperations();
     const bool Fixture=FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8);
     Check(TEXT("v60_live_engineer_and_casemate_fixture"),Fixture);
     if(Fixture){B->BeginBattle();B->SetActorTickEnabled(false);FrontlineFieldSitesV59::RunChecksV60(B,Check);B->AbandonBattleToOperations();}
     if(FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8))FrontlineWarRoomV52::RunFlightChecksV54(PC,Check);
    }
    else
    {
    AWarfareFrontlineGround::RunChecksV54(Check);
    FrontlineTerrainTacticsV54::RunChecks(Check);
    FrontlineMobilityV41::RunTerrainRouteChecksV54(Check);
    B->AbandonBattleToOperations();
    // The live site checks need the actual depot and AA meshes from this
    // disposable operation, after Allowed54 has established the no-save gate.
    const bool Sites=FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,13);
    Check(TEXT("v59_live_site_operation_fixture"),Sites);
    if(Sites)
    {
     B->BeginBattle();B->SetActorTickEnabled(false);
     FrontlineFieldSitesV59::RunChecks(B,Check);
     B->AbandonBattleToOperations();
    }
    if(FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8))
     FrontlineWarRoomV52::RunFlightChecksV54(PC,Check);
    else Check(TEXT("v54_map_flight_fixture"),false);
    }
   }
   Check(TEXT("v54_all_five_registered_regional_maps_load"),RegionalMaps54());
   R->Checked=true;
   if(!FParse::Param(FCommandLine::Get(),TEXT("WFV54Art"))){Finish();return false;}
   if(GUsingNullRHI){Check(TEXT("v54_art_requires_renderer"),false);Finish();return false;}
   return true;
  }
  if(R->Step==0)
  {
   if(R->Scene>=R->EndScene){Finish();return false;}
   FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
   bool Setup=false;R->Focus=FVector::ZeroVector;
   if(R->Scene<=3)
   {
    int32 WarRoomMission=8;
    if(FParse::Value(FCommandLine::Get(),TEXT("WFV56ReviewWarRoomMission="),WarRoomMission))
     WarRoomMission=FMath::Clamp(WarRoomMission,0,19);
    Setup=FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,WarRoomMission);
    float ReviewZoom=R->Scene==0?1.f:R->Scene==1?1.85f:
     WarRoomMission<=1?60.f:WarRoomMission>=8&&WarRoomMission<=10?72.f:3.6f;
    FParse::Value(FCommandLine::Get(),TEXT("WFV57ReviewMapZoom="),ReviewZoom);
    if(Setup)Setup=FrontlineWarRoomV52::PrepareReviewV54(PC,WarRoomMission,ReviewZoom,
     R->Scene==3,R->Scene==1?1:0);
   }
   else if(R->Scene==4)Setup=FrontlineWarRoomV52::PrepareLoadingReviewV54(PC);
   else if(V61)Setup=FrontlineWarRoomV52::PrepareMenuReviewV61(PC,R->Scene==6);
   else Setup=Field54(B,PC,R->Scene,R->Focus);
   Check(FString::Printf(TEXT("v54_scene_%d_ready"),R->Scene),Setup);
   if(!Setup){Finish();return false;}
   if(R->Scene>=5&&!V61)
   {
    if(auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
    {Settings->SetHDREnabled(false,false);Settings->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
   }
   FrontlineReviewUIV41::BeginAudit();R->Quiet=0.;
   R->Next=Now+(R->Scene==4?.3: R->Scene>=5?4.5:1.8);R->Step=1;return true;
  }
  if(R->Scene>=5&&!V61)
  {
   if(auto* View=Cast<ACameraActor>(PC->GetViewTarget()))
   {
    int32 W=0,H=0;PC->GetViewportSize(W,H);
    FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),View->GetActorLocation(),R->Focus,
     View->GetCameraComponent()->OrthoWidth,W);
   }
  }
  if(R->Step==1)
  {
   // The preparation artwork exists for several seconds and must be captured
   // before its own render-warmup completes. Its textures were just shown on
   // the war-room sheet; only that scene bypasses the global shader drain.
   if(R->Scene!=4)
   {
    if(!Compiled54())
    {
     R->Quiet=0.;
     if(R->Scene==(V61?3:12)&&R->Performance)R->Performance->Reset();
     return true;
    }
    if(!R->Quiet)
    {
     R->Quiet=Now;
     // The first map draw can expose Unreal's 32px loading placeholder.
     // Audit the settled material/texture view that the screenshot records.
     FrontlineReviewUIV41::BeginAudit();
    }
    if(Now<R->Next||Now-R->Quiet<.7)return true;
   }
   else if(Now<R->Next||!FrontlineWarRoomV52::LoadingReviewSettledV59(PC))return true;
   if(R->Scene==(V61?3:12)&&R->Performance)
   {
    auto& Perf=*R->Performance;
    if(Perf.SettlingSince<=0.)Perf.SettlingSince=Now;
    const int32 Wanting=IStreamingManager::Get().GetNumWantingResources();
    if(Wanting>0)
    {
     Perf.Reset();R->Quiet=0.;
     if(Now-Perf.SettlingSince<15.)return true;
     Perf.Report->SetStringField(TEXT("failure"),TEXT("Texture streaming did not settle within the additional 15-second probe budget; no performance result accepted."));
     Perf.Report->SetNumberField(TEXT("resources_wanting_stream_in"),Wanting);
     Check(TEXT("v54_performance_textures_settled"),false);
    }
    else if(!Perf.Tick(Now,PC,B))return true;
   }
   if(R->Scene==0)
   {
    int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
    Check(TEXT("v54_visual_review_native_4k_viewport"),Width>=3840&&Height>=2160);
   }
   auto Scene=MakeShared<FJsonObject>();Scene->SetNumberField(TEXT("index"),R->Scene);
   if(const auto* M=B->GetSelectedCampaignMission())Scene->SetNumberField(TEXT("mission"),M->Id);
   Scene->SetStringField(TEXT("phase"),B->Phase==EFrontlinePhase::Fighting?TEXT("fighting"):TEXT("briefing"));
   if(R->Scene>=5&&!V61)
   {
    const auto Field=FieldState54(B,R->Focus);Scene->SetObjectField(TEXT("field"),Field);
    Scene->SetNumberField(TEXT("authored_landform_mask_at_focus"),
     Feature54(R->Scene,AWarfareFrontlineGround::QueryTerrainFeaturesV54(R->Focus.X,R->Focus.Y)));
    Check(FString::Printf(TEXT("v57_scene_%d_uses_visible_baked_meshes"),R->Scene),
     Field->GetNumberField(TEXT("visible_tiles"))>0
     &&Field->GetNumberField(TEXT("visible_tiles"))==Field->GetNumberField(TEXT("baked_visible_tiles")));
    if(R->Scene==12)
    {
     const auto Env=Field->GetObjectField(TEXT("menin_environment"));
     Check(TEXT("v59_menin_timber_dressing_preserves_road_and_trench_clearance"),
      Env->GetBoolField(TEXT("success"))
      &&Env->GetNumberField(TEXT("menin_road_or_trench_timber_obstructions"))==0);
    }
   }
   const auto Audit=FrontlineReviewUIV41::EndAudit();Scene->SetObjectField(TEXT("ui_audit"),Audit);
   Check(FString::Printf(TEXT("v54_scene_%d_layout"),R->Scene),Audit->GetBoolField(TEXT("success")));
   R->Shot=R->Dir/ShotName54(R->Scene);Scene->SetStringField(TEXT("capture"),R->Shot);
   R->Scenes.Add(MakeShared<FJsonValueObject>(Scene));FScreenshotRequest::RequestScreenshot(R->Shot,true,false);
   R->Next=Now+1.;R->Step=2;return true;
  }
  if(Now<R->Next)return true;
  Check(FString::Printf(TEXT("v54_scene_%d_capture_written"),R->Scene),IFileManager::Get().FileSize(*R->Shot)>1024);
  ++R->Scene;R->Step=0;return true;
 }));
}
