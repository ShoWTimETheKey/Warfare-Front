#pragma once
#include "WarfareFrontline.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
// No public gameplay command: the caller must enforce a disposable, no-save review directory.
struct FFrontlineUIV8447Review
{
 static float Width(const AWarfareFrontlineController* P){return P->TargetCameraWidth;}
 static void Wheel(AWarfareFrontlineController* P,bool Out){if(Out)P->ZoomOut();else P->ZoomIn();}
 static void Finish(AWarfareFrontlineGameMode* B,int32 Winner,const FString& Reason){B->FinishBattle(Winner,Reason);}
 static void Camera(AWarfareFrontlineController* P,const FVector& Focus,float Width)
 {
  P->CameraFocus=Focus;P->CameraWidth=P->TargetCameraWidth=Width;P->CameraPanVelocity=P->CameraVerticalVelocity=0;
  if(auto* C=P->BattleCamera.Get()){C->SetActorLocation(Focus+FVector(0,8500,2950));C->SetActorRotation((Focus-C->GetActorLocation()).Rotation());C->GetCameraComponent()->OrthoWidth=Width;P->SetViewTarget(C);if(P->PlayerCameraManager)P->PlayerCameraManager->UpdateCamera(0.f);}
 }
 static bool Report(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission)
 {
  B->AbandonBattleToOperations();if(!B->StartNewCampaign(F,EFrontlineDifficulty::Veteran))return false;
  if(auto* R=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){R->CompletedMissions=19;R->WonOperationMaskV52=(1u<<19)-1;R->DispatchResolvedMaskV52=(1u<<20)-1;}
  if(!B->SelectCampaignMission(Mission))return false;B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=INDEX_NONE;B->EndReason=TEXT("The enemy position has fallen.");B->SetActorTickEnabled(false);return B->GetEncounterCampaignMission()!=nullptr;
 }
};
