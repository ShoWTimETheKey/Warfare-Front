// Disposable render-only trajectory. The real movement planner has its own
// V842 checks; moving these unregistered fixtures is not navigation evidence.
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineLightingV42.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

namespace {
void StartFrontlineStencilReviewV842(AWarfareFrontlineController* Controller)
{
 FString User;
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV842StencilReview"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
  ||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun {
  int32 Stage=0,MoveFrame=0,Shots=0;bool Good=true;
  uint64 LastFrame=MAX_uint64;double Started=FPlatformTime::Seconds(),Next=Started+2,Prepared=0;
  FString Dir;TArray<TWeakObjectPtr<AFrontlineTank>> Tanks;TArray<FVector> Starts;
  TArray<TSharedPtr<FJsonValue>> Checks,Positions,Bindings;
 };
 auto R=MakeShared<FRun>();R->Dir=User;
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float){
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV842StencilBaseline"));
  const auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J){FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/Name));};
  const auto Finish=[&](){V28ReviewPointer.Reset();auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetBoolField(TEXT("baseline"),Baseline);J->SetArrayField(TEXT("checks"),R->Checks);J->SetArrayField(TEXT("positions"),R->Positions);J->SetArrayField(TEXT("material_bindings"),R->Bindings);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("motion_steps"),R->MoveFrame);J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);J->SetStringField(TEXT("scope"),TEXT("Actual fixed 4K Quality/High/RR-on/FG-off rendered Mark IV, Whippet and Mark V fixtures, each moving 8 cm per sampled engine frame for 400 cm. Not navigation, combat or a full temporal-art acceptance claim."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExit(false);return false;};
  if(!B||!PC||!PC->BattleCamera){if(Now-R->Started<40)return true;Check(TEXT("stencil_world_ready"),false);return Finish();}
  if(Now-R->Started>150){Check(TEXT("stencil_bounded_150_seconds"),false);return Finish();}
  if(!B->IsProgressionVolatile()||!FrontlineEffectsReviewSafetyV8216::Allowed(B)){Check(TEXT("stencil_disposable_world_guard"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  const auto Clear=[&](){S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;MedalWallV8215::View(PC).Wall=MedalWallV8215::View(PC).Inspect=false;V28ReviewPointer=FVector2D(-10000,-10000);PC->CancelTargeting();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);};
  if(R->Stage==0)
  {
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetRayReconstruction(true,false);}
   Check(TEXT("stencil_british_volatile_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=19;P->WonOperationMaskV52=(1u<<19)-1;P->DispatchResolvedMaskV52=(1u<<20)-1;}
   Check(TEXT("stencil_actual_operation_13"),B->SelectCampaignMission(13));S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;R->Next=Now+.1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<75){R->Next=Now+.1;return true;}
   Check(TEXT("stencil_actual_world_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   Clear();B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);
   // Pause simulation before exposing the render fixtures: no AI deployment,
   // random combat, public-roster insertion or incidental weapon flashes.
   FrontlineLightingV42::ObserveReviewWorld(B->GetWorld());UGameplayStatics::SetGamePaused(PC,true);
   const float Mid=B->Trenches.IsEmpty()?0.f:B->Trenches[B->Trenches.Num()/2].X;
   const EFrontlineVehicle Types[]={EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::Whippet,EFrontlineVehicle::MarkVMale};
   int32 Stencils=0;
   for(int32 I=0;I<3;++I)
   {
    auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>();const bool Configured=V&&V->Configure(B,Types[I],0,84210+I);
    Check(FString::Printf(TEXT("stencil_model%d_actual_assets"),I),Configured&&V->AreAssetsReady());if(!Configured)continue;
    const FVector P(Mid-1750.f+I*1750.f-200.f,-300.f,0);
    V->SetActorTransform(V->GroundPoseForContactV8218(P,0,0));V->SetActorTickEnabled(false);R->Tanks.Add(V);R->Starts.Add(P);
    auto Actor=MakeShared<FJsonObject>();Actor->SetNumberField(TEXT("model"),int32(Types[I]));Actor->SetNumberField(TEXT("serial"),V->Serial);Actor->SetStringField(TEXT("actor"),V->GetPathName());Actor->SetStringField(TEXT("initial_position"),V->GetActorLocation().ToString());
    TArray<TSharedPtr<FJsonValue>> Components;TInlineComponentArray<UStaticMeshComponent*> Meshes(V);
    for(auto* C:Meshes)if(C&&C->GetStaticMesh()&&(C==V->GetRootComponent()||C->GetName().StartsWith(TEXT("PaintedCrewIdentity"))))
    {
     auto Component=MakeShared<FJsonObject>();Component->SetStringField(TEXT("component"),C->GetName());Component->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());Component->SetStringField(TEXT("world_transform"),C->GetComponentTransform().ToString());
     TArray<TSharedPtr<FJsonValue>> Slots;
     for(int32 J=0;J<C->GetNumMaterials();++J){auto Slot=MakeShared<FJsonObject>();auto* M=C->GetMaterial(J);Slot->SetNumberField(TEXT("slot"),J);Slot->SetStringField(TEXT("material"),GetPathNameSafe(M));Slot->SetStringField(TEXT("base"),M?GetPathNameSafe(M->GetBaseMaterial()):TEXT("None"));Slots.Add(MakeShared<FJsonValueObject>(Slot));}
     Component->SetArrayField(TEXT("slots"),Slots);Components.Add(MakeShared<FJsonValueObject>(Component));
     if(C->GetName().StartsWith(TEXT("PaintedCrewIdentity"))){++Stencils;auto* M=C->GetMaterial(0);const FString Base=M?GetPathNameSafe(M->GetBaseMaterial()):FString();const bool OldParent=Base.Contains(TEXT("/Vehicles/V837/Materials/M_Vehicle_Stencil"))||Base.Contains(TEXT("/Vehicles/V37/M_Vehicle_Stencil"));Check(FString::Printf(TEXT("stencil_model%d_actual_expected_parent"),I),M!=nullptr&&(Baseline?OldParent:Base.Contains(TEXT("/Vehicles/V842/Materials/M_Vehicle_Stencil_V842"))));}
    }
    Actor->SetArrayField(TEXT("components"),Components);R->Bindings.Add(MakeShared<FJsonValueObject>(Actor));
   }
   Check(TEXT("stencil_three_real_actors_and_three_paint_receivers"),R->Tanks.Num()==3&&Stencils==3);if(!R->Good)return Finish();
   const FVector Focus(Mid,-200,130);PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=6000;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);
   FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,6000,3840);
   R->Stage=2;R->Next=Now+7;return true;
  }
  if(R->Stage==2)
  {
   // One transform update per distinct actual engine frame. Screenshot readback
   // may add a held frame; recorded frame IDs make this explicit rather than
   // presenting interpolated offline images as a continuous renderer sample.
   if(R->LastFrame==GFrameCounter)return true;R->LastFrame=GFrameCounter;
   auto Frame=MakeShared<FJsonObject>();Frame->SetNumberField(TEXT("motion_step"),R->MoveFrame);Frame->SetNumberField(TEXT("engine_frame"),double(GFrameCounter));TArray<TSharedPtr<FJsonValue>> Poses;
   bool Finite=true;
   for(int32 I=0;I<R->Tanks.Num();++I)
   {
    auto* V=R->Tanks[I].Get();if(!V){Finite=false;continue;}
    FVector P=R->Starts[I];P.X+=8.f*R->MoveFrame;V->SetActorTransform(V->GroundPoseForContactV8218(P,0,1.f/60));
    Finite&=!V->GetActorTransform().ContainsNaN();auto Pose=MakeShared<FJsonObject>();Pose->SetNumberField(TEXT("model"),int32(V->Type));Pose->SetNumberField(TEXT("serial"),V->Serial);Pose->SetStringField(TEXT("position"),V->GetActorLocation().ToString());Pose->SetStringField(TEXT("rotation"),V->GetActorRotation().ToString());Poses.Add(MakeShared<FJsonValueObject>(Pose));
   }
   Frame->SetArrayField(TEXT("actual_poses"),Poses);R->Positions.Add(MakeShared<FJsonValueObject>(Frame));
   if(!Finite){Check(TEXT("stencil_finite_motion"),false);return Finish();}
   if(R->MoveFrame%10==0){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/FString::Printf(TEXT("stencil-motion-%02d.png"),R->MoveFrame),true,false);++R->Shots;}
   if(R->MoveFrame==50){Check(TEXT("stencil_real_400cm_motion_and_six_renderer_captures"),R->Shots==6&&R->Positions.Num()==51);R->Stage=3;R->Next=Now+.1;return true;}
   ++R->MoveFrame;return true;
  }
  return Finish();
 }),0.f);
}
}
