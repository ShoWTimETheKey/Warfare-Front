#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineGameplayV43.h"
namespace
{
void StartFrontlineReviewV8445(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8445Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto Run=MakeShared<FRun>();Run->Dir=User;TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(Run->Dir/Name));};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Run->Good);J->SetArrayField(TEXT("checks"),Run->Checks);J->SetNumberField(TEXT("captures_requested"),0);J->SetNumberField(TEXT("wall_seconds"),Now-Run->Began);J->SetStringField(TEXT("scope"),TEXT("Disposable UK/DE production vehicle Tick/Advance and real engineer release on current operation terrain, thirteen armour health definitions, and support cooldown. Focused actor fixtures; not a complete campaign or performance sweep."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-Run->Began<35)return true;Check(TEXT("v8445_world_ready"),false);return Finish();}
  if(Now-Run->Began>150){Check(TEXT("v8445_bounded_review"),false);return Finish();}
  auto& Screen=ScreenState(PC);
  if(Run->Stage==0)
  {
   Check(TEXT("v8445_disposable_ukde_world"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));if(!Run->Good)return Finish();
   Screen.Menu=Screen.Help=Screen.Settings=Screen.Technology=Screen.AchievementsV76=Screen.Deployment=Screen.UnitIntroduction=Screen.HistoricalDispatch=Screen.CampaignEndingV51=Screen.Credits=Screen.ExitConfirmV61=false;
   Screen.bPausedByOverlay=Screen.bPausedForAuxOverlay=false;Screen.PhotoTransitionV37=Screen.SmallTransitionV35=false;Screen.FactionSelectionV82=FFrontlineFactionSelectionV82();UGameplayStatics::SetGamePaused(PC,false);
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);}
   Check(TEXT("v8445_british_fixture_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* R=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){R->CompletedMissions=17;R->WonOperationMaskV52=(1u<<17)-1;R->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v8445_actual_current_operation_selected"),B->SelectCampaignMission(17));if(!Run->Good)return Finish();
   Screen.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);Run->Prepared=Now;Run->Stage=1;return true;
  }
  if(Run->Stage==1)
  {
   if(Screen.Preparing&&Screen.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-Run->Prepared<75)return true;
   Check(TEXT("v8445_actual_current_terrain_ready"),B->Phase==EFrontlinePhase::Fighting&&!Screen.Preparing&&B->AssetFailure.IsEmpty()&&Screen.PreparationError.IsEmpty());if(!Run->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);Run->Stage=2;return true;
  }
  if(Run->Stage==2)
  {
   auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v8445_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet)return Finish();
   auto Mobility=Fleet->RunMobilityChecksV8445();Write(TEXT("armour-mobility.json"),Mobility);
   const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;const bool Listed=Mobility->TryGetArrayField(TEXT("checks"),Values)&&Values&&!Values->IsEmpty();Check(TEXT("v8445_mobility_checks_actually_executed"),Listed);
   if(Listed)for(const auto& V:*Values){const auto J=V->AsObject();Check(J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));}
   Check(TEXT("v8445_mobility_report_passed"),Mobility->GetBoolField(TEXT("success")));
   auto Escort=FrontlineArmourV63::RunPioneerEscortChecksV8445(B,Check);Write(TEXT("escort-release.json"),Escort);Check(TEXT("v8445_escort_report_passed"),Escort->GetBoolField(TEXT("success")));
   auto Support=MakeShared<FJsonObject>();const float Base=B->SupportCooldownSeconds(EFrontlineSupport::AntiTank);Support->SetNumberField(TEXT("baseline_seconds"),95);Support->SetNumberField(TEXT("new_base_seconds"),Base);
   Check(TEXT("v8445_antitank_base_cooldown_reduced_exactly_twentyfive_percent"),FMath::IsNearlyEqual(Base,95.f*.75f,.00001f));
   Write(TEXT("support-cooldown.json"),Support);
   return Finish();
  }
  Check(TEXT("v8445_known_stage"),false);return Finish();
 }),0.f);
}
}
