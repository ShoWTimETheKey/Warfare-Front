#include "WarfareFrontlineArmourMobilityV839.h"
#include "WarfareFrontlineReportV839.h"
#include "Components/LightComponent.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "RenderingThread.h"
TSharedRef<FJsonObject> RunFrontlineMuzzleFlashV839Review(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
TSharedRef<FJsonObject> RunFrontlineReportV839Review(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>);
bool PrepareFrontlineReportVisualV839(AWarfareFrontlineGameMode*);
namespace
{
void StartFrontlineReviewV839(AWarfareFrontlineController* Controller)
{
 FString User;if(!Controller||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun{int32 Stage=0,Shots=0,Art=0,Page=0;bool Good=true;double Began=FPlatformTime::Seconds(),Next=Began+2,Prepared=0;
  FString Dir;TArray<TSharedPtr<FJsonValue>> Checks;TWeakObjectPtr<AFrontlineTank> Subject;};
 auto R=MakeShared<FRun>();R->Dir=User;IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  const auto Check=[R](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V839_CHECK %s %d"),*Name,int32(Pass));};
  const auto Write=[&](const TCHAR* Name,TSharedRef<FJsonObject> J){FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(R->Dir/Name));};
  const auto Finish=[&](){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),R->Good);J->SetArrayField(TEXT("checks"),R->Checks);J->SetNumberField(TEXT("captures_requested"),R->Shots);J->SetNumberField(TEXT("wall_seconds"),Now-R->Began);J->SetStringField(TEXT("scope"),TEXT("Disposable UK/DE actual receivers, vehicle subsystem contact, normal-lit late-operation material views, source-bound fire lights, bilingual return-sheet and loading pages. No national campaign or whole-game/perfection claim."));Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,R->Good?0:1);return false;};
  if(!B||!PC||!PC->BattleCamera){if(Now-R->Began<30)return true;Check(TEXT("v839_world_ready"),false);return Finish();}
  if(Now-R->Began>420){Check(TEXT("v839_review_bound"),false);return Finish();}
  if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;auto& S=ScreenState(PC);
  const auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(R->Dir/TEXT("Captures")/(Name+TEXT(".png")),true,false);++R->Shots;};
  const auto Clear=[&](){UGameplayStatics::SetGamePaused(PC,false);S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Preparing=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.PhotoTransitionV37=S.SmallTransitionV35=false;PC->CancelTargeting();FrontlineMapSceneV61::Update(PC,false,FVector2D(.5,.5),1,1,0,0,0);};
  const auto Camera=[&](const FVector& Focus,float Width){PC->BattleCamera->SetActorLocation(Focus+FVector(0,8500,2950));PC->BattleCamera->SetActorRotation((Focus-PC->BattleCamera->GetActorLocation()).Rotation());PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,Width,3840);};
  if(R->Stage==0)
  {
   if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);}
   Check(TEXT("v839_start_disposable_british_campaign"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){P->CompletedMissions=19;P->WonOperationMaskV52=(1u<<19)-1;P->DispatchResolvedMaskV52=(1u<<20)-1;P->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;P->ArmourResearchMaskV63=65535;P->SelectedMission=17;}
   // Apply the selected mission through the production path. Mutating only the
   // disposable progress row leaves the old operation-depth profile installed.
   Check(TEXT("v839_selected_operation_uses_public_reset"),B->SelectCampaignMission(17));
   S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);R->Prepared=FPlatformTime::Seconds();R->Stage=1;R->Next=R->Prepared+.1;return true;
  }
  if(R->Stage==1)
  {
   if((S.Preparing||UGameplayStatics::IsGamePaused(B))&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-R->Prepared<70){R->Next=Now+.1;return true;}
   Check(TEXT("v839_actual_operation_loaded"),B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&B->AssetFailure.IsEmpty());
   if(!R->Good)return Finish();Clear();Camera(FVector(0,0,200),7000);R->Next=Now+.5;R->Stage=2;return true;
  }
  if(R->Stage==2)
  {
   Clear();PC->SetActorTickEnabled(false);B->SetActorTickEnabled(false);
   int32 FieldLights=0;TArray<TSharedPtr<FJsonValue>> Lights;
   for(TActorIterator<AActor> It(B->GetWorld());It;++It){TInlineComponentArray<ULightComponent*> Components(*It);for(const auto* C:Components){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("owner"),It->GetName());J->SetStringField(TEXT("component"),C->GetName());J->SetBoolField(TEXT("visible"),C->IsVisible());J->SetNumberField(TEXT("intensity"),C->Intensity);Lights.Add(MakeShared<FJsonValueObject>(J));if(C->IsVisible()&&C->Intensity>0&&!It->GetName().Contains(TEXT("Map")))++FieldLights;}}
   Check(TEXT("v839_actual_battle_view_target"),PC->GetViewTarget()==PC->BattleCamera);Check(TEXT("v839_actual_world_lighting_restored"),FieldLights>0);
   auto Lighting=MakeShared<FJsonObject>();Lighting->SetArrayField(TEXT("lights"),Lights);Lighting->SetNumberField(TEXT("visible_field_lights"),FieldLights);Write(TEXT("normal-lighting.json"),Lighting);
   for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)FFileHelper::SaveStringToFile(It->DescribeTerrainV43(),*(R->Dir/TEXT("terrain-live.json")));
   auto Surfaces=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> SurfaceRows;
   for(TActorIterator<AActor> It(B->GetWorld());It;++It){TInlineComponentArray<UStaticMeshComponent*> Cs(*It);for(const auto* C:Cs)if(C&&C->IsVisible()&&C->GetStaticMesh())for(int32 I=0;I<C->GetNumMaterials();++I)if(const auto* M=C->GetMaterial(I)){const FString Name=M->GetPathName();if(!Name.Contains(TEXT("Bank"))&&!Name.Contains(TEXT("Ground"))&&!Name.Contains(TEXT("Water"))&&!Name.Contains(TEXT("Sandbag")))continue;auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("actor"),It->GetName());J->SetStringField(TEXT("component"),C->GetName());J->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());J->SetStringField(TEXT("material"),Name);J->SetStringField(TEXT("base_material"),M->GetMaterial()->GetPathName());J->SetNumberField(TEXT("slot"),I);J->SetBoolField(TEXT("cast_shadow"),C->CastShadow);SurfaceRows.Add(MakeShared<FJsonValueObject>(J));}}
   Surfaces->SetArrayField(TEXT("surfaces"),SurfaceRows);Write(TEXT("surfaces-live.json"),Surfaces);
   Shot(TEXT("late-ground-before"));R->Next=Now+.1;R->Stage=FParse::Param(FCommandLine::Get(),TEXT("WFV839EnvironmentOnly"))?20:3;return true;
  }
  if(R->Stage==3)
  {
   if(!FParse::Param(FCommandLine::Get(),TEXT("WFV839ReportsOnly")))
   {Write(TEXT("mobility.json"),RunFrontlineArmourMobilityV839Review(B,Check));Write(TEXT("muzzle.json"),RunFrontlineMuzzleFlashV839Review(B,Check));}
   Write(TEXT("report-receivers.json"),RunFrontlineReportV839Review(B,Check));
   if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>()){Audio->RunArmourAudioChecksV839(Check);Write(TEXT("contact-audio.json"),Audio->DescribeArmourContactV839());}
   R->Stage=FParse::Param(FCommandLine::Get(),TEXT("WFV839CoreOnly"))||FParse::Param(FCommandLine::Get(),TEXT("WFV839ReportsOnly"))?6:4;R->Next=FPlatformTime::Seconds()+.1;return true;
  }
  if(R->Stage==4)
  {
   Clear();R->Subject=PrepareFrontlineArmourArtV838(B,R->Art);Check(TEXT("v839_actual_vehicle_assets_")+FString::FromInt(R->Art),R->Subject.IsValid());
   Camera(R->Art==10?FVector(0,-300,210):FVector(0,-650,140),R->Art==10?7800.f:4100.f);R->Prepared=FPlatformTime::Seconds();R->Next=R->Prepared+12.5;R->Stage=5;return true;
  }
  if(R->Stage==5)
  {
   if(auto* V=R->Subject.Get())Write(*FString::Printf(TEXT("vehicle-%02d.json"),R->Art),V->DescribeMobilityV839());
   if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld())){Fire->RunEffectsChecksV839(Check);Write(*FString::Printf(TEXT("fire-%02d.json"),R->Art),Fire->DescribeEffectsV839());}
   if(auto* Surface=AFrontlineArmourSurfaceV837::Get(B->GetWorld()))Write(*FString::Printf(TEXT("materials-%02d.json"),R->Art),Surface->Describe());
   Shot(FString::Printf(TEXT("armour-%02d"),R->Art));++R->Art;R->Stage=R->Art<11?4:6;R->Next=Now+.1;return true;
  }
  if(R->Stage==6)
  {
   Clear();Check(TEXT("v839_real_receiver_report_display_fixture"),PrepareFrontlineReportVisualV839(B));
   auto& V=FrontlineReportV839::View(PC);V.Encounter=B->GetEncounterId();V.Open=false;V.Team=0;V.Tab=0;V.Offset=0;R->Page=-2;R->Stage=7;R->Next=FPlatformTime::Seconds()+.6;return true;
  }
  if(R->Stage==7)
  {
   const bool ZH=R->Page%2!=0;FrontlineLocalizationV35::SetLanguage(ZH?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
   auto& V=FrontlineReportV839::View(PC);V.Open=R->Page>=0;V.Tab=FMath::Max(0,R->Page/2);V.Offset=0;R->Next=Now+.3;R->Stage=8;return true;
  }
  if(R->Stage==8)
  {Shot(R->Page<0?FString::Printf(TEXT("main-report-%s"),R->Page%2?TEXT("zh"):TEXT("en")):FString::Printf(TEXT("report-%d-%s"),R->Page/2,R->Page%2?TEXT("zh"):TEXT("en")));++R->Page;R->Stage=R->Page<10?7:9;R->Next=Now+.1;return true;}
  if(R->Stage==9)
  {FrontlineReportV839::View(PC).Open=false;S.Preparing=true;S.PreparationError.Empty();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);R->Stage=10;R->Next=Now+.5;return true;}
  if(R->Stage==10)
  {Shot(TEXT("loading-late-en"));R->Stage=11;R->Next=Now+.1;return true;}
  if(R->Stage==11)
  {FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);R->Stage=12;R->Next=Now+.5;return true;}
  if(R->Stage==12)
  {Shot(TEXT("loading-late-zh"));R->Stage=13;R->Next=Now+.1;return true;}
  if(R->Stage==13){S.Preparing=false;return Finish();}
  if(R->Stage==20)
  {
   constexpr int32 Modes[]={3,0,1,2};const int32 Mode=Modes[R->Page];int32 Receivers=0;
   for(TActorIterator<AActor> It(B->GetWorld());It;++It){TInlineComponentArray<UStaticMeshComponent*> Cs(*It);for(auto* C:Cs)if(C)for(int32 I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))if(M->GetBaseMaterial()->GetPathName().Contains(TEXT("M_EarthBank_V839"))){M->SetScalarParameterValue(TEXT("BankReviewModeV839"),float(Mode));++Receivers;}}
   Check(FString::Printf(TEXT("v839_bank_actual_ab_receivers_mode%d"),Mode),Receivers>0);
   if(R->Art!=-1){R->Art=-1;R->Prepared=Now;R->Next=Now+4.;return true;}
   TSet<UMaterialInterface*> Materials;
   for(TActorIterator<AActor> It(B->GetWorld());It;++It){TInlineComponentArray<UStaticMeshComponent*> Cs(*It);for(auto* C:Cs)if(C&&C->IsVisible())for(int32 I=0;I<C->GetNumMaterials();++I)if(auto* M=C->GetMaterial(I))if(M->GetBaseMaterial()->GetPathName().Contains(TEXT("M_EarthBank_V839")))Materials.Add(M);}
   TArray<TSharedPtr<FJsonValue>> Proxies;bool Warm=true;
   for(auto* M:Materials)
   {
    struct FResult {bool Complete=false,Valid=false,Same=false;FString Name;};FResult Result;FResult* Output=&Result;
    const FMaterialRenderProxy* Assigned=M->GetRenderProxy();const auto Feature=B->GetWorld()->GetFeatureLevel();
    ENQUEUE_RENDER_COMMAND(WFReadBankV839)([Assigned,Feature,Output](FRHICommandListImmediate&){const FMaterialRenderProxy* Effective=Assigned;const FMaterial& Actual=Assigned->GetMaterialWithFallback(Feature,Effective);Output->Name=Actual.GetFriendlyName();Output->Same=Effective==Assigned;Output->Complete=Actual.IsRenderingThreadShaderMapComplete();const auto* Map=Actual.GetRenderingThreadShaderMap();Output->Valid=Map&&Map->IsValidForRendering();});
    FlushRenderingCommands();
    const bool Ready=Result.Complete&&Result.Valid&&Result.Same&&Result.Name.Contains(TEXT("EarthBank_V839"));Warm&=Ready;
    if(Ready||Now-R->Prepared>=15.)Check(TEXT("v839_bank_actual_render_proxy_not_fallback"),Ready);
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("assigned_mid"),M->GetPathName());J->SetStringField(TEXT("actual_shader"),Result.Name);J->SetBoolField(TEXT("render_shader_map_valid"),Result.Valid);J->SetBoolField(TEXT("assigned_proxy_used"),Result.Same);float Parameter=-1;const bool HasMode=M->GetScalarParameterValue(TEXT("BankReviewModeV839"),Parameter);J->SetNumberField(TEXT("actual_mode"),Parameter);Check(TEXT("v839_bank_actual_scalar_matches_mode"),HasMode&&FMath::IsNearlyEqual(Parameter,float(Mode)));Proxies.Add(MakeShared<FJsonValueObject>(J));
   }
   auto Description=MakeShared<FJsonObject>();Description->SetArrayField(TEXT("materials"),Proxies);Write(*FString::Printf(TEXT("bank-mode%d-proxies.json"),Mode),Description);
   if(!Warm&&Now-R->Prepared<15.){R->Next=Now+.25;return true;}
   Camera(FVector(FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,-650),-650,160),4100);
   if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();R->Art=0;R->Stage=21;R->Next=Now+2.;return true;
  }
  if(R->Stage==21)
  {constexpr int32 Modes[]={3,0,1,2};Shot(FString::Printf(TEXT("bank-mode%d-view%d"),Modes[R->Page],R->Art));++R->Art;if(R->Art<2){Camera(FVector(FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,-650),-650,80),2200);if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();R->Next=Now+2.;return true;}++R->Page;const int32 Cases=FParse::Param(FCommandLine::Get(),TEXT("WFV839BankOnly"))?2:4;R->Stage=R->Page<Cases?20:22;R->Next=Now+.1;return true;}
  if(R->Stage==22){if(FParse::Param(FCommandLine::Get(),TEXT("WFV839BankOnly")))return Finish();R->Page=0;R->Stage=23;R->Next=Now+.1;return true;}
  if(R->Stage==23)
  {
   constexpr int32 Operations[]={17,3,10};B->AbandonBattleToOperations();
   Check(TEXT("v839_ground_comparison_public_operation_selection"),B->SelectCampaignMission(Operations[R->Page]));B->BeginBattle();B->SetActorTickEnabled(false);Clear();
   // Reset the bank to its production candidate before the independent ground
   // unit comparison; the source palette, weather and physical tiles stay put.
   for(TActorIterator<AActor> It(B->GetWorld());It;++It){TInlineComponentArray<UStaticMeshComponent*> Cs(*It);for(auto* C:Cs)if(C)for(int32 I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))if(M->GetBaseMaterial()->GetPathName().Contains(TEXT("M_EarthBank_V839")))M->SetScalarParameterValue(TEXT("BankReviewModeV839"),0.f);}
   Camera(FVector(0,-650,140),5100);R->Art=0;R->Stage=24;R->Next=FPlatformTime::Seconds()+2.;return true;
  }
  if(R->Stage==24)
  {Check(TEXT("v839_actual_ground_unit_parent_swap"),FrontlineTerrainV42::SetGroundUnitsReviewV839(B,R->Art>0));if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();R->Stage=25;R->Next=Now+2.;return true;}
  if(R->Stage==25)
  {
   constexpr int32 Operations[]={17,3,10};Shot(FString::Printf(TEXT("ground-op%d-units%d"),Operations[R->Page],R->Art));
   for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)FFileHelper::SaveStringToFile(It->DescribeTerrainV43(),*(R->Dir/FString::Printf(TEXT("ground-op%d-units%d.json"),Operations[R->Page],R->Art)));
   ++R->Art;if(R->Art<2){R->Stage=24;R->Next=Now+.1;return true;}++R->Page;R->Stage=R->Page<3?23:26;R->Next=Now+.1;return true;
  }
  if(R->Stage==26)return Finish();
  return Finish();
 }),0.f);
}
}
