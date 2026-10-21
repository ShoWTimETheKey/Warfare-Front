#include "WarfareFrontlineReviewV53.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineUnlocksV53.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineTerrainV42.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
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
bool Allowed53(AWarfareFrontlineGameMode* B)
{FString D;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),D)&&!D.IsEmpty()&&!FPaths::IsRelative(D);}
bool Compiled53()
{
#if WITH_EDITOR
 if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))return false;
#endif
 return GUsingNullRHI||PipelineStateCache::GetNumActivePipelinePrecompileTasks()==0;
}
UFrontlineWaterResponseV51* Water53(UWorld* W)
{for(TActorIterator<AWarfareFrontlineGround> It(W);It;++It)if(auto* R=It->FindComponentByClass<UFrontlineWaterResponseV51>())return R;return nullptr;}
const TCHAR* Shot53(int32 I)
{static const TCHAR* Names[]={TEXT("01-v53-victory-locked.png"),TEXT("02-v53-water-before.png"),TEXT("03-v53-casualty-seep.png"),TEXT("04-v53-rain-and-pressure.png"),TEXT("05-v53-battlefield-scale.png"),TEXT("06-v53-water-detail.png")};return Names[FMath::Clamp(I,0,5)];}
}
struct FFrontlineReviewV53
{
 static bool Ready(AWarfareFrontlineGameMode* B){return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
 static bool Field(AWarfareFrontlineGameMode* B)
 {
  if(!Allowed53(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,5))return false;
  B->BeginBattle();B->SetActorTickEnabled(false);B->Elapsed=10;
  for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))U->Destroy();B->Units.Empty();B->Squads.Empty();
  UGameplayStatics::SetGamePaused(B,false);return B->Phase==EFrontlinePhase::Fighting;
 }
 static bool Pool(FVector& P)
 {
  float Best=-1;
  for(float X=-2200;X<2200;X+=90)for(float Y=-600;Y<650;Y+=70)
  {
   float L,D;if(!AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,L,D)||D<3)continue;
   int32 Wet=0;for(int32 I=0;I<8;++I){float WL,WD;Wet+=AWarfareFrontlineGround::QueryWaterSurfaceV51(X+FMath::Cos(I*PI*.25f)*90,Y+FMath::Sin(I*PI*.25f)*90,WL,WD)&&FMath::Abs(WL-L)<1;}
   const float Score=Wet*100+FMath::Min(D,30.f)-FMath::Abs(Y)*.003f;
   if(Score>Best){Best=Score;P=FVector(X,Y,L);}
  }
  return Best>=700;
 }
 static bool Casualties(AWarfareFrontlineGameMode* B,const FVector& P)
 {
  bool Good=true;
  for(int32 I=0;I<2;++I)
  {
   const FVector Q=P+FVector(I?65.f:-55.f,I?30.f:-25.f,0);
   FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(FVector(Q.X,Q.Y,AWarfareFrontlineGround::HeightAt(Q.X,Q.Y)+88),FRotator(0,I?180:0,0),Params);
   if(!U){Good=false;continue;}U->Initialize(B,880+I,I,EFrontlineRole::Rifle,0);B->Units.Add(U);
   U->ReceiveHit(100,0,1-I,FVector(I?-1.f:1.f,.1f,0),EFrontlineDamageKind::Bullet,.8f);Good&=!U->IsAlive();
  }
  return Good;
 }
 static void SetClock(AWarfareFrontlineGameMode* B,float DT){if(B->Phase==EFrontlinePhase::Fighting)B->Elapsed+=FMath::Clamp(DT,0.f,.1f);}
};
void StartFrontlineV53Review(AWarfareFrontlineGameMode* B)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFV53Review")))return;
 struct FRun{TWeakObjectPtr<AWarfareFrontlineGameMode> B;double Start=FPlatformTime::Seconds(),Next=0,Quiet=0,SampleAt=0,SampleLast=0;int32 Scene=0,Step=0;bool Good=true,Checked=false;FVector Pool=FVector::ZeroVector;FString Dir,Shot;TArray<double> Frames,GPU;TArray<TSharedPtr<FJsonValue>> Checks,Scenes;};
 auto R=MakeShared<FRun>();R->B=B;R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V53/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
 if(FParse::Param(FCommandLine::Get(),TEXT("WFV53WaterOnly"))){R->Checked=true;R->Scene=1;}
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
 {
  auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& N,bool P){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),P);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=P;UE_LOG(LogTemp,Display,TEXT("WF_V53_CHECK %s %s"),P?TEXT("PASS"):TEXT("FAIL"),*N);};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();int32 Pass=0;for(auto& C:R->Checks)Pass+=C->AsObject()->GetBoolField(TEXT("passed"));J->SetBoolField(TEXT("success"),R->Good);J->SetNumberField(TEXT("passed"),Pass);J->SetNumberField(TEXT("total"),R->Checks.Num());J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("art_scenes"),R->Scenes);J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);J->SetStringField(TEXT("scope"),TEXT("V53 focused encounter unlock timing and native casualty/puddle material review. Fixed-camera samples are not live-combat benchmarks or fluid-volume certification."));FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);};
  if(!B||Now-R->Start>420){Check(TEXT("v53_bounded_review"),false);Finish();return false;}if(!FFrontlineReviewV53::Ready(B))return true;
  if(!Allowed53(B)){Check(TEXT("v53_requires_isolated_profile"),false);Finish();return false;}
  if(!R->Checked)
  {
   FrontlineUnlocksV53::RunChecks(B,Check);
   const bool Field=FFrontlineReviewV53::Field(B);Check(TEXT("v53_real_field_ready"),Field);if(Field)UFrontlineWaterResponseV51::RunChecksV53(B->GetWorld(),Check);
   R->Checked=true;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV53Art"))){Finish();return false;}return true;
  }
  if(GUsingNullRHI){Check(TEXT("v53_art_requires_renderer"),false);Finish();return false;}
  FFrontlineReviewV53::SetClock(B,DT);
  auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC){Check(TEXT("v53_controller"),false);Finish();return false;}
  if(R->Scene>=1)if(auto* View=Cast<ACameraActor>(PC->GetViewTarget())){int32 W=0,H=0;PC->GetViewportSize(W,H);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),View->GetActorLocation(),R->Pool,View->GetCameraComponent()->OrthoWidth,W);}
  if(R->Step==0)
  {
   if(R->Scene>=6){Finish();return false;}
   FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);bool Good=true;
   if(R->Scene==0){Good=FrontlineUnlocksV53::PrepareVictoryReview(B,EFrontlineFaction::British,0);FrontlineReviewUIV41::Clear(PC);}
   if(R->Scene==1){Good=FFrontlineReviewV53::Field(B)&&FFrontlineReviewV53::Pool(R->Pool);FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,R->Pool.X,R->Pool.Y,2700,R->Pool.Z+180,true);if(auto* W=AWarfareFrontlineWeather::Get(B->GetWorld()))W->SetMode(EFrontlineWeatherMode::Overcast,false);}
   if(R->Scene==2)Good=FFrontlineReviewV53::Casualties(B,R->Pool);
   if(R->Scene==3){if(auto* W=AWarfareFrontlineWeather::Get(B->GetWorld()))W->SetMode(EFrontlineWeatherMode::Rain,false);}
   if(R->Scene==4)FrontlineReviewUIV41::Camera(PC,R->Pool.X,R->Pool.Y,6500,650,true);
   if(R->Scene==5){FrontlineReviewUIV41::Camera(PC,R->Pool.X,R->Pool.Y,1200,R->Pool.Z,true);if(auto* View=Cast<ACameraActor>(PC->GetViewTarget())){const FVector From=R->Pool+FVector(0,3500,6000);View->SetActorLocation(From);View->SetActorRotation((R->Pool-From).Rotation());View->GetCameraComponent()->OrthoWidth=1200;}}
   if(auto* S=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){S->SetHDREnabled(false,false);S->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
   Check(FString::Printf(TEXT("v53_scene_%d_ready"),R->Scene),Good);if(!Good){Finish();return false;}
   R->Next=Now+(R->Scene==2?14.:4.);R->Quiet=R->SampleAt=0;R->Frames.Reset();R->GPU.Reset();R->Step=1;FrontlineReviewUIV41::BeginAudit();return true;
  }
  if(R->Step==1)
  {
   if(!Compiled53()){R->Quiet=0;return true;}if(!R->Quiet)R->Quiet=Now;if(Now<R->Next||Now-R->Quiet<.8)return true;
   if(R->Scene==3)
   {
    if(!R->SampleAt){Check(TEXT("v53_actual_pool_pressure_admitted"),UFrontlineWaterResponseV51::NotifyImpact(B->GetWorld(),R->Pool,560,1.f));R->SampleAt=Now;return true;}
    if(Now-R->SampleAt<.45)return true;
   }
   if(R->Scene==1||R->Scene==2)
   {
    if(!R->SampleAt){R->SampleAt=R->SampleLast=Now;return true;}R->Frames.Add((Now-R->SampleLast)*1000);R->SampleLast=Now;
    const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(GPU>0)R->GPU.Add(GPU);if(Now-R->SampleAt<2.)return true;
   }
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("scene"),R->Scene);J->SetStringField(TEXT("pool"),R->Pool.ToString());if(auto* W=Water53(B->GetWorld())){const auto State=W->Describe();J->SetObjectField(TEXT("water"),State);if(R->Scene==2)Check(TEXT("v53_native_casualties_actually_feed_water"),State->GetNumberField(TEXT("active_sources"))>0&&State->GetNumberField(TEXT("total_mass"))>.01&&State->GetNumberField(TEXT("blood_deposits"))>0&&State->GetNumberField(TEXT("corpse_seep_deposits"))>0);}
   if(R->Frames.Num()){double Sum=0,G=0;for(double V:R->Frames)Sum+=V;for(double V:R->GPU)G+=V;J->SetNumberField(TEXT("fixed_scene_frame_ms"),Sum/R->Frames.Num());if(R->GPU.Num())J->SetNumberField(TEXT("fixed_scene_gpu_ms"),G/R->GPU.Num());J->SetStringField(TEXT("timing_scope"),TEXT("2 seconds; fixed camera, director held, FG off; casualty physics added in second sample; not isolated shader cost."));}
   auto Audit=FrontlineReviewUIV41::EndAudit();J->SetObjectField(TEXT("ui_audit"),Audit);Check(FString::Printf(TEXT("v53_scene_%d_layout"),R->Scene),Audit->GetBoolField(TEXT("success")));
   R->Shot=R->Dir/Shot53(R->Scene);J->SetStringField(TEXT("capture"),R->Shot);R->Scenes.Add(MakeShared<FJsonValueObject>(J));FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Next=Now+1.;R->Step=2;return true;
  }
  if(Now<R->Next)return true;Check(FString::Printf(TEXT("v53_scene_%d_capture_written"),R->Scene),IFileManager::Get().FileSize(*R->Shot)>1024);++R->Scene;R->Step=0;return true;
 }));
}
