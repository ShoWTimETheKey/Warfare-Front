#include "WarfareFrontlineCrewReviewV84411.inl"
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFrontlineMaterialV27.h"

namespace
{
void StartFrontlineReviewV84411(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{FString Dir;int32 Stage=0,Captures=0;bool Good=true;double Began=FPlatformTime::Seconds(),Prepared=0,Next=0;FVector Focus;TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Visuals;TArray<TSharedPtr<FJsonValue>> Checks;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V84411_CHECK %s %d"),*Name,int32(Pass));};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(R->Dir/Name));};
  auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Captures);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-R->Began);J->SetStringField(TEXT("scope"),(FParse::Param(FCommandLine::Get(),TEXT("WFV84411ArmourFocus"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84412ArmourRegressionReview")))?TEXT("Partial diagnostic only: selected prior/current UK/DE armour and infantry physical Step tracing; not delivery acceptance."):TEXT("Focused disposable UK/DE actual crew and armour Tick, blast-mine accounting, loaded audio, conforming scorch and finite blood transport. Native SDR 4K captures; no whole-campaign, HDR, DLSS-FG or maximum-performance acceptance."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-R->Began<35)return true;Check(TEXT("v84411_world_ready"),false);return Finish();}
  if(Now-R->Began>600){Check(TEXT("v84411_bounded_review"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  if(R->Stage==0)
  {
   Check(TEXT("v84411_disposable_ukde_only"),B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B));
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;UGameplayStatics::SetGamePaused(PC,false);
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
   Check(TEXT("v84411_british_disposable_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=17;P->WonOperationMaskV52=(1u<<17)-1;P->DispatchResolvedMaskV52=(1u<<18)-1;}
   Check(TEXT("v84411_actual_operation_geometry"),B->SelectCampaignMission(17));if(!R->Good)return Finish();S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=Now;R->Stage=1;return true;
  }
  if(R->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<75)return true;
   Check(TEXT("v84411_actual_terrain_and_assets_prepared"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty());if(!R->Good)return Finish();
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   const bool ArmourFocus=FParse::Param(FCommandLine::Get(),TEXT("WFV84411ArmourFocus"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84412ArmourRegressionReview"));
   if(!ArmourFocus){Write(TEXT("machinegun.json"),FFrontlineMachinegunV8450Review::Run(B,Check));Write(TEXT("crew.json"),RunFrontlineCrewChecksV84411(B,Check));}
   if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld())){TSharedPtr<FJsonObject> J;FrontlineFieldV32::WithScopedCombatEventsV839(B,[&](){J=Fleet->RunMobilityChecksV84411();});Write(TEXT("armour.json"),J.ToSharedRef());Check(TEXT("v84411_actual_paired_armour_checks"),J->GetBoolField(TEXT("success")));}else Check(TEXT("v84411_vehicle_subsystem"),false);
   if(ArmourFocus)return Finish();
   if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()){Audio->RunArmourAudioChecksV839(Check);Write(TEXT("audio.json"),Audio->DescribeArmourContactV839());}else Check(TEXT("v84411_audio_subsystem"),false);
   FrontlineFieldV32::WithScopedCombatEventsV839(B,[&](){FFrontlineMachinegunV8450Review::FContext MineContext(B);
    const float FriendlyFireBefore=B->FriendlyFireDamage;
    ON_SCOPE_EXIT{B->FriendlyFireDamage=FriendlyFireBefore;};
    FFrontlineMachinegunV8450Review::Make(MineContext,8441190,0,0,FVector(560,800,0),true);
    FFrontlineMachinegunV8450Review::Make(MineContext,8441191,1,0,FVector(560,800,0),true);
    FrontlineFieldV32::RunBlastMineChecksV84411(B,Check);});
   {FFrontlineMachinegunV8450Review::FContext BurnContext(B);
    auto* U=FFrontlineMachinegunV8450Review::Make(BurnContext,8441180,0,0,FVector(0,0,0),true);
    Check(TEXT("v84411_charred_fixture_spawned"),U!=nullptr);if(U){U->ReceiveHit(1,0,1,FVector(1,0,0),EFrontlineDamageKind::Bullet);FrontlineMaterialV27::RunBurnChecksV84411(U,[&](const TCHAR* N,bool P){Check(N,P);});}}
   FrontlineFieldV32::WithScopedCombatEventsV839(B,[&]()
   {
    FFrontlineMachinegunV8450Review::FContext C(B);TArray<TSharedPtr<FJsonValue>> Cases;
    for(int32 Team:{0,1})for(int32 Kind=0;Kind<4;++Kind)
    {
     C.Clear();const bool Fuel=Kind==1;const auto Damage=Kind==0?EFrontlineDamageKind::Fire:Kind==1||Kind==3?EFrontlineDamageKind::Artillery:EFrontlineDamageKind::Bullet;
     auto* U=FFrontlineMachinegunV8450Review::Make(C,8441170+Team*10+Kind,Team,0,FVector(0,0,0),false,Fuel?EFrontlineRole::Flamethrower:EFrontlineRole::Rifle);
     const FString Name=FString::Printf(TEXT("v84411_burn_t%d_k%d_"),Team,Kind);Check(Name+TEXT("actual_unit"),U!=nullptr);if(!U)continue;
     U->ReceiveHit(1,0,1-Team,FVector(1,0,0),Damage);TArray<TSharedPtr<FJsonValue>> Samples;bool Finite=true,Early=true,Final=true;int32 Bound=0;
     for(int32 Frame=0;Frame<=220;++Frame)
     {
      if(Frame){B->Elapsed+=.05f;FrontlineFieldV32::Tick(B,.05f);FFrontlineMachinegunV8450Review::Tick(U,.05f);}
      float Highest=0,Lowest=1;Bound=0;for(int32 Slot=0;Slot<U->Soldier->GetNumMaterials();++Slot){float V=0;auto* M=U->Soldier->GetMaterial(Slot);if(M&&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BurnAmountV84411")),V)){++Bound;Highest=FMath::Max(Highest,V);Lowest=FMath::Min(Lowest,V);Finite&=FMath::IsFinite(V)&&V>=0&&V<=1;}}
      if(Frame<=60)Early&=Highest==0;if(Frame==220)Final=Kind<2?Lowest>.999f:Highest==0;
      if(Frame==0||Frame==60||Frame==148||Frame==220){auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("seconds"),Frame*.05f);J->SetNumberField(TEXT("minimum_burn"),Lowest);J->SetNumberField(TEXT("maximum_burn"),Highest);J->SetNumberField(TEXT("bound_materials"),Bound);Samples.Add(MakeShared<FJsonValueObject>(J));}
     }
     Check(Name+TEXT("actual_tick_preserves_initial_surface_and_final_state"),!U->IsAlive()&&Bound>=3&&Finite&&Early&&Final);
     auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("team"),Team);J->SetNumberField(TEXT("kind"),Kind);J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
    }
    auto J=MakeShared<FJsonObject>();J->SetArrayField(TEXT("cases"),Cases);Write(TEXT("burn.json"),J);
   });
   AWarfareFrontlineWoundFX::RunBloodChecksV84411(B,Check);
   AWarfareFrontlineScorchV8449::RunChecks(B,Check);
   if(auto* Scorch=AWarfareFrontlineScorchV8449::Get(B->GetWorld(),false)){auto J=Scorch->Describe();Check(TEXT("v84411_scorch_actual_native_ground_decal_material"),J->GetBoolField(TEXT("material_native_ground_decal_v84411"))&&J->GetBoolField(TEXT("material_shader_valid"))&&J->GetBoolField(TEXT("material_shader_complete")));Write(TEXT("scorch.json"),J);R->Focus=FVector(J->GetNumberField(TEXT("review_focus_x")),J->GetNumberField(TEXT("review_focus_y")),J->GetNumberField(TEXT("review_focus_z")));}
   S.Preparing=false;S.SmallTransitionV35=S.PhotoTransitionV37=false;PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);
   FFrontlineUIV8447Review::Camera(PC,R->Focus+FVector(0,0,90),2600);R->Stage=2;R->Next=FPlatformTime::Seconds()+12;return true;
  }
  if(R->Stage==2)
  {
   Write(TEXT("render.json"),FieldRenderInventoryV8449(B));FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures/01-scorch.png"),true,false);++R->Captures;R->Stage=3;R->Next=Now+.1;return true;
  }
  if(R->Stage==3)
  {
   if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld())){FVector P=R->Focus;Check(TEXT("v84411_blood_visible_ground_deposit_prepared"),FX->ReviewBloodDepositsV84411(P));R->Focus=P;FX->SetActorTickEnabled(true);FFrontlineUIV8447Review::Camera(PC,P+FVector(0,0,70),2600);}
   R->Stage=4;R->Next=Now+5;return true;
  }
  if(R->Stage==4)
  {
   if(auto* FX=AWarfareFrontlineWoundFX::Get(B->GetWorld()))Write(TEXT("blood.json"),FX->DescribeBloodV84411());FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures/02-blood.png"),true,false);++R->Captures;R->Stage=5;R->Next=Now+.1;return true;
  }
  if(R->Stage==5)
  {
   for(int32 Team:{0,1})
   {
    auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();Check(TEXT("v84411_charred_visual_actual_unit"),U!=nullptr);if(!U)continue;
    U->Initialize(B,8441160+Team,Team,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);FVector P=R->Focus+FVector(Team?220.f:-220.f,0,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);U->SetActorLocation(P+FVector(0,0,88));U->ReceiveHit(1,0,1-Team,FVector(1,0,0),EFrontlineDamageKind::Fire);
    for(int32 F=0;F<240;++F)FFrontlineMachinegunV8450Review::Tick(U,.05f);R->Visuals.Add(U);B->Units.Add(U);
   }
   R->Stage=6;R->Next=Now+3;return true;
  }
  if(R->Stage==6){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures/03-charred.png"),true,false);++R->Captures;R->Stage=7;R->Next=Now+.3;return true;}
  if(R->Stage==7){TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(R->Dir/TEXT("Captures/*.png")),true,false);Check(TEXT("v84411_focused_4k_capture_files_written"),Files.Num()==R->Captures);return Finish();}
  return true;
 }));
}
}

