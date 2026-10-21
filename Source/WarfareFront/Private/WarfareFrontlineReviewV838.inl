#include "GameFramework/WorldSettings.h"
AFrontlineTank* PrepareFrontlineArmourArtV838(AWarfareFrontlineGameMode*,int32);
namespace
{
void StartFrontlineReviewV838(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{int32 Stage=0,Art=0,Shots=0,Feasibility=-1,ArtCase=-1;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2,Prepared=0;
  FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TWeakObjectPtr<AFrontlineTank> Subject;};
 auto R=MakeShared<FRun>();R->Dir=User;FParse::Value(FCommandLine::Get(),TEXT("WFV838FeasibilityCase="),R->Feasibility);
 FParse::Value(FCommandLine::Get(),TEXT("WFV838ArtCase="),R->ArtCase);if(R->ArtCase>=0&&R->ArtCase<11)R->Art=R->ArtCase;
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V838_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J){FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/Name));};
  const auto Finish=[&](){if(B&&B->GetWorld())B->GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetNumberField(TEXT("feasibility_case"),R->Feasibility);Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!B||!PC||!PC->BattleCamera){if(Now-R->Began<30)return true;Check(TEXT("v838_world_ready"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& S=ScreenState(PC);
  const auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;};
  const auto Camera=[&](const FVector& Focus,float Width){PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);};
  if(R->Stage==0)
  {
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);}
   if(R->Feasibility>=0)Check(TEXT("v838_prepare_real_goal_case"),FrontlineAchievementsV76::PrepareFeasibilityCaseV838(B,R->Feasibility));
   else
   {
    Check(TEXT("v838_prepare_art_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
    if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=19;P->WonOperationMaskV52=(1u<<19)-1;P->DispatchResolvedMaskV52=(1u<<20)-1;P->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;P->ArmourResearchMaskV63=65535;P->SelectedMission=17;}
   }
   if(!R->Good)return Finish();
   // A cold editor preparation frame can otherwise advance the battle before
   // the live V851 policy starts. Freeze only this disposable setup, and resume
   // ordinary world ticks at the existing empty-start check below.
   if(R->Feasibility>=4&&FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused")))B->SetActorTickEnabled(false);
   S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;R->Next=Now+.1;return true;
  }
  if(R->Stage==1)
  {
   if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<70){R->Next=Now+.1;return true;}
   Check(TEXT("v838_actual_world_loaded"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&!UGameplayStatics::IsGamePaused(B)&&B->AssetFailure.IsEmpty());
   if(!R->Good)return Finish();PC->SetActorTickEnabled(false);
   if(R->Feasibility>=0){Check(TEXT("v838_real_goal_case_started_empty"),FrontlineAchievementsV76::StartFeasibilityCaseV838(B,R->Feasibility));if(!R->Good)return Finish();B->SetActorTickEnabled(true);B->SetBattleSpeed(1.5f);R->Stage=2;R->Next=Now+.025;return true;}
   R->Stage=3;
  }
  if(R->Stage==2)
  {
   TSharedPtr<FJsonObject> Outcome;
   if(!FrontlineAchievementsV76::TickFeasibilityCaseV838(B,Outcome)){R->Next=Now+.025;return true;}
   B->GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);B->SetActorTickEnabled(false);Check(TEXT("v838_actual_goal_earned_through_normal_settlement"),Outcome.IsValid()&&Outcome->GetBoolField(TEXT("success"))&&Outcome->GetBoolField(TEXT("target_newly_earned")));
   if(Outcome)Write(TEXT("achievement-feasibility-v838.json"),Outcome.ToSharedRef());Shot(TEXT("achievement-case-")+FString::FromInt(R->Feasibility));R->Stage=6;R->Next=Now+.4;return true;
  }
  if(R->Stage==3)
  {
   UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;PC->CancelTargeting();
   R->Subject=PrepareFrontlineArmourArtV838(B,R->Art);Check(TEXT("v838_actual_model_scene_")+FString::FromInt(R->Art),R->Subject.IsValid()&&(R->Art==10?!R->Subject->IsOperational():R->Subject->AreAssetsReady()));
   Camera(R->Art==10?FVector(0,-300,210):FVector(0,-650,140),R->Art==10?7800.f:4100.f);R->Stage=4;R->Prepared=FPlatformTime::Seconds();R->Next=R->Prepared+1.4;return true;
  }
  if(R->Stage==4)
  {
   if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))
   {
    auto Smoke=Fire->DescribeWreckSmokeV838();bool Warm=Smoke->GetNumberField(TEXT("active_sources"))>0;
    // Let the genuine damage blast retire before reviewing the continuing
    // wreck soot; source flow advances only through ordinary world ticks.
    for(const auto& Row:Smoke->GetArrayField(TEXT("sources")))Warm&=Row->AsObject()->GetNumberField(TEXT("flow_age"))>=12.;
    if(!Warm&&Now-R->Prepared<60){R->Next=Now+.15;return true;}
    Check(TEXT("v838_actual_wreck_flow_warmed_")+FString::FromInt(R->Art),Warm);
   }
   if(R->Art!=10)if(auto* Subject=R->Subject.Get())Subject->SetActorTickEnabled(false);
   if(auto* Surface=AFrontlineArmourSurfaceV837::Get(B->GetWorld()))
   {
    auto Description=Surface->Describe();Write(*FString::Printf(TEXT("model-%02d-v838.json"),R->Art),Description);
    if(R->Art!=10)for(const auto& Row:Description->GetArrayField(TEXT("vehicles")))
      for(const auto& Mat:Row->AsObject()->GetArrayField(TEXT("materials")))
        Check(TEXT("v838_actual_bespoke_receiver_")+FString::FromInt(R->Art),Mat->AsObject()->GetStringField(TEXT("base_material")).Contains(TEXT("/Vehicles/V838/")));
   }
   if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld())){Fire->RunWreckSmokeChecksV838(Check);Write(*FString::Printf(TEXT("wreck-%02d-v838.json"),R->Art),Fire->DescribeWreckSmokeV838());}
   Shot(FString::Printf(TEXT("armour-%02d-live-and-wreck"),R->Art));R->Stage=5;R->Next=Now+.15;return true;
  }
  if(R->Stage==5){++R->Art;R->Stage=R->ArtCase<0&&R->Art<11?3:6;R->Next=Now+.1;return true;}
  return Finish();
 }),0.f);
}
}
