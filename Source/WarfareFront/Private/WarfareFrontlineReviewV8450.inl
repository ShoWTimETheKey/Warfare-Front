#include "WarfareFrontlineMachinegunReviewV8450.inl"

namespace
{
void StartFrontlineReviewV8450(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(R->Dir/Name));};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R->Began);Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-R->Began<35)return true;Check(TEXT("v8450_world_ready"),false);return Finish();}
  if(Now-R->Began>210){Check(TEXT("v8450_bounded_review"),false);return Finish();}
  auto& S=ScreenState(PC);
  if(R->Stage==0)
  {
   Check(TEXT("v8450_disposable_ukde_only"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;UGameplayStatics::SetGamePaused(PC,false);
   Check(TEXT("v8450_british_disposable_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v8450_actual_operation_geometry"),B->SelectCampaignMission(17));if(!R->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<75)return true;
   Check(TEXT("v8450_actual_terrain_and_assets_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   auto Result=FFrontlineMachinegunV8450Review::Run(B,Check);Write(TEXT("machinegun.json"),Result);return Finish();
  }
  return true;
 }));
}
}
