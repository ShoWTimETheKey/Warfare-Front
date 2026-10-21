#include "WarfareFrontlineWaterReviewV72.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineWeather.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PipelineStateCache.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif

namespace
{
bool AllowedWater72(AWarfareFrontlineGameMode* B)
{
 FString D;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),D)
  &&!D.IsEmpty()&&!FPaths::IsRelative(D);
}
bool CompiledWater72()
{
#if WITH_EDITOR
 if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))return false;
#endif
 return !GUsingNullRHI&&PipelineStateCache::GetNumActivePipelinePrecompileTasks()==0;
}
UFrontlineWaterResponseV51* ResponseWater72(UWorld* W)
{for(TActorIterator<AWarfareFrontlineGround> It(W);It;++It)if(auto* R=It->FindComponentByClass<UFrontlineWaterResponseV51>())return R;return nullptr;}
TSharedPtr<FJsonObject> GeometryWater72()
{
 TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(AWarfareFrontlineGround::DescribeWaterV72()),J);
 return J;
}
bool FieldWater72(AWarfareFrontlineGameMode* B,int32 Op)
{
 if(!AllowedWater72(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,Op))return false;
 B->BeginBattle();B->SetActorTickEnabled(false);B->Elapsed=10;
 for(auto U:B->Units)if(IsValid(U.Get()))U->Destroy();B->Units.Empty();B->Squads.Empty();
 UGameplayStatics::SetGamePaused(B,false);return B->Phase==EFrontlinePhase::Fighting;
}
bool PoolWater72(FVector& Position)
{
 float Best=-MAX_flt;
 // Prefer the regional mud channels; retain crater pools as baseline fallback.
 for(float X=-6000;X<6500;X+=100)for(float Y=-3500;Y<1500;Y+=100)
 {
  float L=0,D=0,G=-1;if(!AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,L,D,&G)||D<3.f)continue;
  int32 Wet=0;for(int32 I=0;I<8;++I){float WL=0,WD=0,WG=-1;Wet+=AWarfareFrontlineGround::QueryWaterSurfaceV51(X+FMath::Cos(I*PI*.25f)*70,Y+FMath::Sin(I*PI*.25f)*70,WL,WD,&WG)&&FMath::Abs(WL-L)<.01f&&FMath::Abs(WG-G)<.001f;}
  const float Score=Wet*100+FMath::Min(D,60.f)-FMath::Abs(Y+1500.f)*.02f-FMath::Abs(X)*.002f;
  if(Score>Best){Best=Score;Position=FVector(X,Y,L);}
 }
 return Best>600.f;
}
struct FSceneWater72{int32 Op;const TCHAR* Name;float Width;int32 Motion;bool Response;};
const FSceneWater72 WaterScenes72[]={{5,TEXT("01-wide"),6500,0,false},{5,TEXT("02-close"),2200,0,false},
 {5,TEXT("03-blood-and-pressure"),2200,0,true},{5,TEXT("04-pan-AD"),6500,1,false},
 {5,TEXT("05-pan-WS"),6500,2,false},{12,TEXT("06-lys-wide"),6500,0,false},{16,TEXT("07-canal-wide"),6500,0,false},
 {5,TEXT("08-11500-WS-edges"),11500,3,false}};
}

void StartFrontlineWaterReviewV72(AWarfareFrontlineGameMode* B)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview")))return;
 struct FRun
 {
  TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;double Start=FPlatformTime::Seconds(),At=0,Quiet=0,Last=0;
  int32 OpCheck=0,Scene=0,Step=0,PanShot=0,FieldOp=-1;bool Good=true,Baseline=false,FocusedV75=false,DiagnosticV75=false,NoReflectionV75=false;
  FVector Pool=FVector::ZeroVector;FString Dir;TArray<FString> PendingShots;TArray<double> Frames,GPU;
  TArray<TSharedPtr<FJsonValue>> Checks,Profiles,Scenes;TSharedPtr<FJsonObject> Current;
 };
 auto R=MakeShared<FRun>();R->Battle=B;R->Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline"));
 R->FocusedV75=FParse::Param(FCommandLine::Get(),TEXT("WFV75WaterFocused"));
 R->DiagnosticV75=FParse::Param(FCommandLine::Get(),TEXT("WFV75WaterHidden"));
 R->NoReflectionV75=FParse::Param(FCommandLine::Get(),TEXT("WFV75WaterNoReflection"));
 if(R->DiagnosticV75){R->FocusedV75=true;R->OpCheck=2;R->Scene=1;}
 if(R->NoReflectionV75){R->FocusedV75=true;R->OpCheck=2;R->Scene=1;}
 R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V72/WaterReview"));IFileManager::Get().MakeDirectory(*R->Dir,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
 {
  auto* B=R->Battle.Get();const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& N,bool P){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),P);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=P;UE_LOG(LogTemp,Display,TEXT("WF_V72_WATER_CHECK %s %s"),P?TEXT("PASS"):TEXT("FAIL"),*N);};
  auto Finish=[&]()
  {
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetBoolField(TEXT("baseline"),R->Baseline);
   J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("actual_profiles"),R->Profiles);J->SetArrayField(TEXT("render_scenes"),R->Scenes);J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);
   J->SetStringField(TEXT("scope"),R->DiagnosticV75?TEXT("Operation 5 close view with only the actual basin-water component hidden; diagnostic comparison, FG off."):R->NoReflectionV75?TEXT("Operation 5 close and blood/pressure views with SingleLayerWater reflection disabled; diagnostic comparison, FG off."):R->FocusedV75?TEXT("V75 core/regional material correction on operations 5 and 16: wide, close, blood/pressure, AD/WS and canal. Geometry unchanged. Held director, FG off; visual acceptance remains manual."):TEXT("Actual V72 baked water geometry and shared queries on five affected operations; rendered 4K wide, close, blood/pressure and moving AD/WS frames. Director held, FG off; no fluid-volume, generated-present, live-combat FPS or full-campaign claim. Visual acceptance remains manual."));
   if(B)if(auto* S=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))J->SetObjectField(TEXT("render_diagnostics"),S->GetDiagnostics());
   if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.ScreenProbeGather.Temporal.DistanceThreshold")))J->SetNumberField(TEXT("lumen_temporal_distance_threshold"),V->GetFloat());
   FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
  };
  // The five real operation loads share this wall-clock bound with captures.
  // Editor static-mesh initialisation exceeded the previous 150-second cap.
  if(!B||Now-R->Start>240){Check(TEXT("v72_water_review_bounded"),false);Finish();return false;}
  if(!B->HasActorBegunPlay()||!B->LoadedSoldier||!B->IdleAnimation||!B->AssetFailure.IsEmpty())return true;
  if(!AllowedWater72(B)){Check(TEXT("v72_water_review_isolated_profile"),false);Finish();return false;}
  auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC)return true;
  if(R->OpCheck<(R->FocusedV75?2:5))
  {
   static const int32 Ops[]={5,6,12,16,19};static const int32 FocusedOps[]={5,16};const int32 Op=R->FocusedV75?FocusedOps[R->OpCheck++]:Ops[R->OpCheck++];
   const bool Ready=FieldWater72(B,Op);Check(FString::Printf(TEXT("v72_operation_%02d_loaded"),Op),Ready);if(!Ready){Finish();return false;}
   R->FieldOp=Op;FrontlineReviewUIV41::Clear(PC);auto Stats=GeometryWater72();Check(TEXT("v72_actual_geometry_report_readable"),Stats.IsValid());
   if(Stats){Stats->SetNumberField(TEXT("review_operation"),Op);R->Profiles.Add(MakeShared<FJsonValueObject>(Stats));}
   if(!R->Baseline)AWarfareFrontlineGround::RunChecksV72(Check);
   if(R->OpCheck==1){UFrontlineWaterResponseV51::RunChecksV64(B->GetWorld(),Check);UFrontlineWaterResponseV51::RunChecksV53(B->GetWorld(),Check);}
   return true;
  }
  if(R->FocusedV75&&R->Scene==5)R->Scene=6;
  if(R->Scene>=UE_ARRAY_COUNT(WaterScenes72)||(R->FocusedV75&&R->Scene>=7)||(R->DiagnosticV75&&R->Scene>=2)||(R->NoReflectionV75&&R->Scene>=3)){Finish();return false;}
  const auto& Scene=WaterScenes72[R->Scene];
  if(auto* View=Cast<ACameraActor>(PC->GetViewTarget())){int32 W=0,H=0;PC->GetViewportSize(W,H);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),View->GetActorLocation(),R->Pool,View->GetCameraComponent()->OrthoWidth,W);}
  if(R->Step==0)
  {
   bool Ready=true;if(R->FieldOp!=Scene.Op){Ready=FieldWater72(B,Scene.Op);R->FieldOp=Scene.Op;}
   Ready&=PoolWater72(R->Pool);Check(FString::Printf(TEXT("v72_%s_real_pool_ready"),Scene.Name),Ready);if(!Ready){Finish();return false;}
   FrontlineReviewUIV41::Clear(PC);
   int32 VW=0,VH=0;PC->GetViewportSize(VW,VH);
   const float VerticalLimit=FrontlineTerrainV42::CameraVerticalLimitV43(Scene.Width,VH>0?float(VW)/VH:16.f/9.f);
   FrontlineReviewUIV41::Camera(PC,Scene.Motion==3?0:R->Pool.X,Scene.Motion==3?-VerticalLimit:R->Pool.Y,Scene.Width,Scene.Motion==3?650:R->Pool.Z+180,true);
   if(auto* S=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){S->SetHDREnabled(false,false);S->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
   if(Scene.Op==5)if(auto* W=AWarfareFrontlineWeather::Get(B->GetWorld()))W->SetMode(EFrontlineWeatherMode::Rain,false);
   R->Current=MakeShared<FJsonObject>();R->Current->SetNumberField(TEXT("operation"),Scene.Op);R->Current->SetStringField(TEXT("scene"),Scene.Name);R->Current->SetStringField(TEXT("pool"),R->Pool.ToString());R->Current->SetNumberField(TEXT("ortho_width_cm"),Scene.Width);
   R->Current->SetNumberField(TEXT("analytical_bed_cm"),AWarfareFrontlineGround::HeightAt(R->Pool.X,R->Pool.Y));
   R->Current->SetBoolField(TEXT("water_hidden_diagnostic"),R->DiagnosticV75);
   if(R->NoReflectionV75)if(auto* Var=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.SingleLayer.Reflection")))Var->Set(0,ECVF_SetByCode);
   auto Vars=MakeShared<FJsonObject>();
   for(const TCHAR* Name:{TEXT("r.Water.SingleLayer"),TEXT("r.Water.SingleLayer.Reflection"),TEXT("r.Water.SingleLayer.DepthPrepass"),TEXT("r.Water.SingleLayer.RefractionDownsampleFactor"),TEXT("r.NGX.DLSS.DenoiserMode"),TEXT("r.Substrate")})
       if(auto* Var=IConsoleManager::Get().FindConsoleVariable(Name))Vars->SetNumberField(Name,Var->GetFloat());
   R->Current->SetObjectField(TEXT("water_render_cvars"),Vars);
   for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
   {
    TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);
    for(auto* Part:Parts)if(Part&&Part->GetMaterial(0)&&Part->GetMaterial(0)->GetMaterial()->GetName().StartsWith(TEXT("M_BasinWater")))
    {
     R->Current->SetStringField(TEXT("water_world_transform"),Part->GetComponentTransform().ToString());
     R->Current->SetStringField(TEXT("water_parent"),Part->GetMaterial(0)->GetMaterial()->GetPathName());
     R->Current->SetNumberField(TEXT("water_shading_models"),Part->GetMaterial(0)->GetMaterial()->GetShadingModels().GetShadingModelField());
     if(R->DiagnosticV75)Part->SetVisibility(false);
    }
   }
   if(Scene.Motion==3){R->Current->SetNumberField(TEXT("production_vertical_limit_cm"),VerticalLimit);R->Current->SetStringField(TEXT("camera_scope"),TEXT("Requested 11500 cm stress view; production vertical bounds and focus height. Current scroll-wheel maximum is 9000 cm."));}
   R->At=Now;R->Quiet=0;R->Frames.Reset();R->GPU.Reset();R->PendingShots.Reset();R->PanShot=0;R->Step=1;return true;
  }
  if(R->Step==1)
  {
   if(!CompiledWater72()){R->Quiet=0;return true;}if(!R->Quiet)R->Quiet=Now;
   if(Now-R->At<2.||Now-R->Quiet<.5)return true;
   if(Scene.Response){Check(TEXT("v72_real_water_pressure_admitted"),UFrontlineWaterResponseV51::NotifyImpact(B->GetWorld(),R->Pool,520,.8f));Check(TEXT("v72_real_water_blood_admitted"),UFrontlineWaterResponseV51::NotifyBlood(B->GetWorld(),R->Pool,.65f,165,true));}
   R->At=R->Last=Now;R->Step=2;return true;
  }
  if(R->Step==2)
  {
   const float T=FMath::Clamp(float((Now-R->At)/3.2),0.f,1.f);
   if(Scene.Motion)
   {
    float X=R->Pool.X+(Scene.Motion==1?(T-.5f)*850.f:0),Y=R->Pool.Y+(Scene.Motion==2?(T-.5f)*560.f:0),Height=R->Pool.Z+180;
    if(Scene.Motion==3){int32 W=0,H=0;PC->GetViewportSize(W,H);const float Limit=FrontlineTerrainV42::CameraVerticalLimitV43(Scene.Width,H>0?float(W)/H:16.f/9.f);X=0;Y=(2*T-1)*Limit;Height=650;}
    FrontlineReviewUIV41::Camera(PC,X,Y,Scene.Width,Height,false);
   }
   R->Frames.Add((Now-R->Last)*1000);R->Last=Now;const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(GPU>0)R->GPU.Add(GPU);
   const float NextShot=Scene.Motion==3?.5f*R->PanShot:Scene.Motion?(.25f+.25f*R->PanShot):.5f;
   const int32 Shots=Scene.Motion?3:1;
   if(R->PanShot<Shots&&T>=NextShot&&!FScreenshotRequest::IsScreenshotRequested())
   {
    const FString File=R->Dir/FString::Printf(TEXT("%s-%d.png"),Scene.Name,++R->PanShot);R->PendingShots.Add(File);FScreenshotRequest::RequestScreenshot(File,true,false);
   }
   if(T<1)return true;
   double Sum=0,G=0;for(double V:R->Frames)Sum+=V;for(double V:R->GPU)G+=V;
   R->Current->SetNumberField(TEXT("render_frame_ms"),Sum/FMath::Max(1,R->Frames.Num()));if(R->GPU.Num())R->Current->SetNumberField(TEXT("gpu_frame_ms"),G/R->GPU.Num());
   R->Current->SetStringField(TEXT("timing_scope"),TEXT("3.2-second rendered scene sample, held director; moving-camera frames for AD/WS, FG off; not live-combat benchmark."));
   if(auto* W=ResponseWater72(B->GetWorld()))R->Current->SetObjectField(TEXT("water_response"),W->Describe());
   R->At=Now;R->Step=3;return true;
  }
  if(Now-R->At<.8)return true;TArray<TSharedPtr<FJsonValue>> Captures;
  for(const auto& File:R->PendingShots){Check(FString::Printf(TEXT("v72_capture_%s_written"),*FPaths::GetBaseFilename(File)),IFileManager::Get().FileSize(*File)>1024);Captures.Add(MakeShared<FJsonValueString>(File));}
  Check(FString::Printf(TEXT("v72_%s_all_camera_frames_captured"),Scene.Name),R->PendingShots.Num()==(Scene.Motion?3:1));
  R->Current->SetArrayField(TEXT("captures"),Captures);R->Scenes.Add(MakeShared<FJsonValueObject>(R->Current));++R->Scene;R->Step=0;return true;
 }));
}
