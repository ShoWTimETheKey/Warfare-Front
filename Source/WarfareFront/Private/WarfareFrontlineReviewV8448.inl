#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "RenderingThread.h"
namespace
{
TSharedRef<FJsonObject> TimberTimingV8448(TArray<double> Values)
{
 auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());
 if(Values.IsEmpty())return J;
 double Sum=0;for(double Value:Values)Sum+=Value;Values.Sort();
 J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());
 J->SetNumberField(TEXT("p95_ms"),Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*.95)-1,0,Values.Num()-1)]);
 return J;
}
void StartFrontlineReviewV8448(AWarfareFrontlineController* Controller)
{
 FString User;
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8448Review"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  FString Dir;int32 Stage=0,Scene=0,Page=0,Captures=0;bool Good=true,Baseline=false;
  double Began=FPlatformTime::Seconds(),Next=Began+.5,Prepared=0,SampleStart=0,LastFrame=0;
  uint64 Frame=0;FVector Focus;TArray<double> Wall,GPU,Game,Render;
  TArray<TSharedPtr<FJsonValue>> Checks;
 };
 auto Run=MakeShared<FRun>();Run->Dir=User;Run->Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV8448TreeBaseline"));
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V8448_CHECK %s %d"),*Name,int32(Pass));};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Run->Dir/Name));};
  auto Finish=[&]()
  {
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Run->Good);J->SetArrayField(TEXT("checks"),Run->Checks);
   J->SetNumberField(TEXT("captures_requested"),Run->Captures);J->SetNumberField(TEXT("wall_seconds"),Now-Run->Began);J->SetBoolField(TEXT("tree_baseline"),Run->Baseline);
   J->SetStringField(TEXT("scope"),TEXT("Disposable British/German production terrain, continuous actual Fleet movement and contact fixtures, installed photographed timber bounds/material bindings, normal-lit 4K paired wide/close views. Short warmed rendering samples are observations, not whole-game performance acceptance."));
   Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:1);return false;
  };
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-Run->Began<35)return true;Check(TEXT("v8448_world_ready"),false);return Finish();}
  if(Now-Run->Began>285){Check(TEXT("v8448_bounded_review"),false);return Finish();}
  if(Now<Run->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;S.Preparing=false;
   MedalMountV8216::CloseV836(PC);UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);
  };
  if(Run->Stage==0)
  {
   // Preparation advances in Controller Tick. Restore it after the preceding
   // fixed-camera scene; a frozen review must not freeze the next load.
   B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);
   Check(TEXT("v8448_strict_disposable_ukde_only"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));Clear();
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetHDREnabled(false,false);R->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);}
   if(B->Phase!=EFrontlinePhase::Briefing)B->AbandonBattleToOperations();
   const EFrontlineFaction Faction=Run->Scene==2?EFrontlineFaction::German:EFrontlineFaction::British;
   const int32 Mission=Run->Scene==0?7:Run->Scene==1?17:10;
   Check(TEXT("v8448_actual_campaign_prepared"),B->StartNewCampaign(Faction,EFrontlineDifficulty::Veteran));
   if(auto* R=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){R->CompletedMissions=19;R->WonOperationMaskV52=(1u<<19)-1;R->DispatchResolvedMaskV52=(1u<<20)-1;}
   const bool Selected=B->SelectCampaignMission(Mission);Check(TEXT("v8448_actual_operation_selected"),Selected);if(!Selected)return Finish();
   S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);Run->Prepared=Now;Run->Stage=1;return true;
  }
  if(Run->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-Run->Prepared<75)return true;
   const bool Ready=B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty();Check(TEXT("v8448_actual_terrain_assets_ready"),Ready);if(!Ready)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   Clear();Run->Stage=2;return true;
  }
  if(Run->Stage==2)
  {
   if(Run->Scene==1&&!Run->Baseline)
   {
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v8448_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return Finish();
    auto Mobility=Fleet->RunMobilityChecksV8448();Write(TEXT("armour-mobility.json"),Mobility);
    const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;const bool Listed=Mobility->TryGetArrayField(TEXT("checks"),Values)&&Values&&!Values->IsEmpty();Check(TEXT("v8448_dynamic_mobility_fixtures_executed"),Listed);
    if(Listed)for(const auto& V:*Values){const auto J=V->AsObject();Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
    Check(TEXT("v8448_dynamic_mobility_report_passed"),Mobility->GetBoolField(TEXT("success")));
   }
   auto Field=FrontlineEnvironmentV42::RunChecks(B);Write(FString::Printf(TEXT("field-%d.json"),Run->Scene),Field);
   const bool FieldReady=Field->GetBoolField(TEXT("success"));Check(TEXT("v8448_installed_environment_contacts_and_materials"),FieldReady);
   // A mobility failure remains failed in the final aggregate, but must not
   // discard the independent tree captures and other affected environments.
   if(!FieldReady)return Finish();Run->Page=0;Run->Stage=3;return true;
  }
  if(Run->Stage==3)
  {
   Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
   // Both comparison legs use the same physical instance location, not a
   // showroom camera or independently stretched source mesh.
   FVector Focus(0,0,180);float Width=9000;
   if(Run->Page==1)
   {
    double Best=DBL_MAX;bool Found=false;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
     TInlineComponentArray<UInstancedStaticMeshComponent*> Components(*It);
     for(auto* C:Components)if(C&&C->IsVisible()&&C->GetStaticMesh()&&C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_SplinteredTrunk_")))
     for(int32 I=0;I<C->GetInstanceCount();++I)
     {
      FTransform T;if(!C->GetInstanceTransform(I,T,true))continue;
      const double Score=FVector2D(T.GetLocation().X,T.GetLocation().Y-2800).SizeSquared();
      if(Score<Best){Best=Score;Focus=T.GetLocation()+FVector(0,0,150);Found=true;}
     }
    }
    Check(TEXT("v8448_actual_instanced_tree_close_camera"),Found);Width=2600;
   }
   if(Run->Baseline)
   {
    FString Source;FParse::Value(FCommandLine::Get(),TEXT("WFV8448CameraSource="),Source);Source=FPaths::ConvertRelativePathToFull(Source);FPaths::NormalizeDirectoryName(Source);
    const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8448/Runtime/"));
    const FString Stamp=Source.StartsWith(Root,ESearchCase::IgnoreCase)?Source.Mid(Root.Len()):FString();
    const bool Safe=Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')&&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric()&&!Source.Contains(TEXT(".."));
    FString Text;TSharedPtr<FJsonObject> Camera;
    const bool Loaded=Safe&&FFileHelper::LoadFileToString(Text,*(Source/FString::Printf(TEXT("camera-%d-%d.json"),Run->Scene,Run->Page)))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Camera)&&Camera;
    Check(TEXT("v8448_baseline_uses_exact_candidate_camera"),Loaded);if(!Loaded)return Finish();
    Focus=FVector(Camera->GetNumberField(TEXT("x")),Camera->GetNumberField(TEXT("y")),Camera->GetNumberField(TEXT("z")));Width=Camera->GetNumberField(TEXT("width"));
   }
   Run->Focus=Focus;FFrontlineUIV8447Review::Camera(PC,Focus,Width);
   if(PC->BattleCamera)FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,Width,3840);
   Run->Frame=GFrameCounter;Run->Stage=4;Run->Next=Now+2.;return true;
  }
  if(Run->Stage==4)
  {
   if(GFrameCounter<=Run->Frame+2)return true;int32 W=0,H=0;PC->GetViewportSize(W,H);const bool CorrectSize=W==3840&&H==2160;Check(TEXT("v8448_actual_4k_render"),CorrectSize);if(!CorrectSize)return Finish();
   auto Camera=MakeShared<FJsonObject>();Camera->SetNumberField(TEXT("x"),Run->Focus.X);Camera->SetNumberField(TEXT("y"),Run->Focus.Y);Camera->SetNumberField(TEXT("z"),Run->Focus.Z);Camera->SetNumberField(TEXT("width"),Run->Page?2600:9000);
   Write(FString::Printf(TEXT("camera-%d-%d.json"),Run->Scene,Run->Page),Camera);
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/FString::Printf(TEXT("%02d-%s-%s.png"),Run->Scene*2+Run->Page+1,Run->Scene==0?TEXT("uk-menin"):Run->Scene==1?TEXT("uk-rail"):TEXT("de-late"),Run->Page?TEXT("timber"):TEXT("wide")),true,false);++Run->Captures;
   if(Run->Page==0){Run->SampleStart=Run->LastFrame=Now;Run->Wall.Empty();Run->GPU.Empty();Run->Game.Empty();Run->Render.Empty();Run->Stage=5;}
   else {Run->Stage=6;Run->Next=Now+.1;}return true;
  }
  if(Run->Stage==5)
  {
   if(Now-Run->LastFrame>0){Run->Wall.Add((Now-Run->LastFrame)*1000);Run->GPU.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Run->Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));Run->Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));}
   Run->LastFrame=Now;if(Now-Run->SampleStart<4.)return true;
   auto Timing=MakeShared<FJsonObject>();Timing->SetObjectField(TEXT("wall"),TimberTimingV8448(Run->Wall));Timing->SetObjectField(TEXT("gpu"),TimberTimingV8448(Run->GPU));Timing->SetObjectField(TEXT("game_thread"),TimberTimingV8448(Run->Game));Timing->SetObjectField(TEXT("render_thread"),TimberTimingV8448(Run->Render));
   Timing->SetNumberField(TEXT("sample_seconds"),Now-Run->SampleStart);Timing->SetBoolField(TEXT("tree_baseline"),Run->Baseline);Timing->SetStringField(TEXT("scope"),TEXT("Short warmed fixed-camera SDR/FG-off 4K observation; delayed RHI timings. Consult launch.json for competing user games."));Write(FString::Printf(TEXT("timing-%d.json"),Run->Scene),Timing);
   Run->Page=1;Run->Stage=3;return true;
  }
  if(Run->Stage==6)
  {
   if(++Run->Scene<3){Run->Stage=0;return true;}
   TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Run->Dir/TEXT("Captures/*.png")),true,false);bool Good=Files.Num()==6;for(const auto& F:Files)Good&=IFileManager::Get().FileSize(*(Run->Dir/TEXT("Captures")/F))>1024;
   Check(TEXT("v8448_all_affected_scene_captures_written"),Good&&Run->Captures==6);return Finish();
  }
  Check(TEXT("v8448_known_stage"),false);return Finish();
 }),0.f);
}
}
