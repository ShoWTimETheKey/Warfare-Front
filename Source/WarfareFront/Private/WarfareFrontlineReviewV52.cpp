#include "WarfareFrontlineReviewV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineWarRoomV52.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
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
bool Allowed52(AWarfareFrontlineGameMode* B)
{FString D;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),D)&&!D.IsEmpty()&&!FPaths::IsRelative(D);}
bool Compiled52()
{
#if WITH_EDITOR
 if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))return false;
#endif
 return GUsingNullRHI||PipelineStateCache::GetNumActivePipelinePrecompileTasks()==0;
}
const TCHAR* Shot52(int32 I)
{static const TCHAR* N[]={TEXT("01-v52-british-branch-orders.png"),TEXT("02-v52-german-enemy-zh.png"),TEXT("03-v52-locked-route-preview.png"),TEXT("04-v52-theatre-and-terrain.png"),TEXT("05-v52-dispatch-choice-zh.png"),TEXT("06-v52-machinegun-variant.png"),TEXT("07-v52-machinegun-research.png"),TEXT("08-v52-command-stars.png")};return N[FMath::Clamp(I,0,7)];}
FVector2D ResearchHover52(AWarfareFrontlineGameMode* B)
{
 // Centre of node 27's late-page card, matching DrawTechnology's grid.
 const auto& Nodes=B->GetTechnologyNodes();if(!Nodes.IsValidIndex(27))return FVector2D(-10000,-10000);
 return FVector2D(486.f+Nodes[27].Branch*286.f,318.f+(Nodes[27].Tier-4)*181.f+72.5f);
}
}
struct FFrontlineReviewV52
{
 static bool Ready(AWarfareFrontlineGameMode* B){return B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IdleAnimation&&B->AssetFailure.IsEmpty();}
 static bool Scene(AWarfareFrontlineGameMode* B,int32 I)
 {
  if(!Allowed52(B))return false;auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC)return false;
  FrontlineLocalizationV35::SetLanguage((I==1||I==4)?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
  const int32 Ops[]={2,14,2,16,0,8,8,15};const bool German=I==1;
  if(!FrontlineCampaignV41::PrepareReviewOperation(B,German?EFrontlineFaction::German:EFrontlineFaction::British,Ops[I]))return false;B->SetActorTickEnabled(false);
  if(I<4)return FrontlineWarRoomV52::PrepareReview(PC,I==2?19:Ops[I],I==1?2:I==2?3:I==3?1:0,I==3?1.f:2.3f);
  if(I==4){B->AbandonBattleToOperations();if(!B->StartNewCampaign(EFrontlineFaction::British))return false;return FrontlineWarRoomV52::PrepareReview(PC,0,0,2.f,true);}
  auto& Research=B->ActiveResearchMutable();Research.ResearchPoints=18;Research.TechnologyMask=(1u<<0)|(1u<<1)|(1u<<2)|(1u<<3)|(1u<<10)|(1u<<27);
  if(I==5){FrontlineReviewUIV41::Deployment(PC);FrontlineReviewUIV41::Pointer(FVector2D(1900,1060));return true;}
  if(I==6){Research.TechnologyMask&=~(1u<<27);FrontlineReviewUIV41::Technology(PC,1,27);FrontlineReviewUIV41::Pointer(ResearchHover52(B));return true;}
  Research.TechnologyMask|=1u<<9;B->BeginBattle();B->SetActorTickEnabled(false);B->Elapsed=10;
  const int32 Home=B->GetHomeTrenchIndex(0);
  const bool Spawned=B->SpawnSquad(0,EFrontlineRole::Officer,Home)&&B->SpawnSquad(0,EFrontlineRole::Rifle,Home)&&B->SpawnSquad(0,EFrontlineRole::Lewis,Home);
  FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,B->Trenches[Home].X,600,5400,650,true);return Spawned;
 }
};
void StartFrontlineV52Review(AWarfareFrontlineGameMode* B)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFV52Review")))return;
 struct FRun{TWeakObjectPtr<AWarfareFrontlineGameMode> B;double Start=FPlatformTime::Seconds(),Next=0,Quiet=0;int32 I=0,Last=7,RequestedScene=-1,Step=0;bool Good=true,Checked=false,SelectionValid=true;FString Dir,Shot;TArray<TSharedPtr<FJsonValue>> Checks,Scenes;};
 auto R=MakeShared<FRun>();R->B=B;R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V52/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
 if(FParse::Value(FCommandLine::Get(),TEXT("WFV52ReviewScene="),R->RequestedScene))
 {
  R->SelectionValid=R->RequestedScene>=0&&R->RequestedScene<=7&&FParse::Param(FCommandLine::Get(),TEXT("WFV52Art"));
  if(R->SelectionValid)R->I=R->Last=R->RequestedScene;
 }
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float)
 {
  auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V52_CHECK %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();int32 Passed=0;for(const auto& C:R->Checks)Passed+=C->AsObject()->GetBoolField(TEXT("passed"));J->SetBoolField(TEXT("success"),R->Good);J->SetNumberField(TEXT("passed"),Passed);J->SetNumberField(TEXT("total"),R->Checks.Num());J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("art_scenes"),R->Scenes);J->SetNumberField(TEXT("wall_seconds"),Now-R->Start);J->SetNumberField(TEXT("requested_scene"),R->RequestedScene);J->SetStringField(TEXT("scope"),R->RequestedScene>=0?TEXT("Isolated V52 selected art scene; production logic checks also run when WFV52Logic is requested. Not a general visual, balance or performance certificate."):TEXT("Isolated production V52 roster, command stars, branch/save/event and interactive map checks. Eight art scenes only when WFV52Art is requested; not a general balance or performance certificate."));FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);};
  if(!R->SelectionValid){Check(TEXT("v52_scene_selection_requires_art_and_index_0_to_7"),false);Finish();return false;}
  if(!B||Now-R->Start>480){Check(TEXT("v52_bounded_review"),false);Finish();return false;}if(!FFrontlineReviewV52::Ready(B))return true;
  if(!Allowed52(B)){Check(TEXT("v52_isolated_profile"),false);Finish();return false;}
  if(!FParse::Param(FCommandLine::Get(),TEXT("WFV52Art"))||(!R->Checked&&FParse::Param(FCommandLine::Get(),TEXT("WFV52Logic"))))
  {
   const bool Setup=FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,8);Check(TEXT("v52_production_world_ready"),Setup);
   if(Setup){B->BeginBattle();B->SetActorTickEnabled(false);FrontlineGameplayV52::RunChecks(B,Check);B->AbandonBattleToOperations();FrontlineCampaignV52::RunChecks(B,Check);FrontlineWarRoomV52::RunChecks(B,Check);}
   R->Checked=true;if(FParse::Param(FCommandLine::Get(),TEXT("WFV52Art")))return true;Finish();return false;
  }
  if(GUsingNullRHI){Check(TEXT("v52_art_requires_renderer"),false);Finish();return false;}
  if(R->Step==0){if(R->I>R->Last){Finish();return false;}const bool Ready=FFrontlineReviewV52::Scene(B,R->I);Check(FString::Printf(TEXT("v52_scene_%d_ready"),R->I),Ready);if(!Ready){Finish();return false;}FrontlineReviewUIV41::BeginAudit();R->Step=1;R->Next=Now+4.;R->Quiet=0;return true;}
  if(R->Step==1)
  {if(!Compiled52()){R->Quiet=0;return true;}if(!R->Quiet)R->Quiet=Now;if(Now<R->Next||Now-R->Quiet<1.)return true;
   auto S=MakeShared<FJsonObject>();const auto Audit=FrontlineReviewUIV41::EndAudit();S->SetObjectField(TEXT("ui_audit"),Audit);Check(FString::Printf(TEXT("v52_scene_%d_text_image_bounds"),R->I),Audit->GetBoolField(TEXT("success")));
   R->Shot=R->Dir/Shot52(R->I);S->SetStringField(TEXT("capture"),R->Shot);S->SetNumberField(TEXT("index"),R->I);R->Scenes.Add(MakeShared<FJsonValueObject>(S));
   // EndAudit clears the simulated pointer; keep the captured hover identical.
   FrontlineReviewUIV41::Pointer(R->I==6?ResearchHover52(B):FVector2D(1900,1060));
   FScreenshotRequest::RequestScreenshot(R->Shot,true,false);R->Next=Now+1.;R->Step=2;return true;}
  if(Now<R->Next)return true;Check(FString::Printf(TEXT("v52_scene_%d_capture_written"),R->I),IFileManager::Get().FileSize(*R->Shot)>1024);++R->I;R->Step=0;return true;
 }));
}
