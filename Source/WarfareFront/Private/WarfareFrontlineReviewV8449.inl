#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "StaticMeshResources.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "RenderingThread.h"
namespace
{
// Read the original material proxies in the real scene after its camera has
// rendered. A valid game-thread ShaderMap alone does not rule out fallback.
TSharedRef<FJsonObject> FieldRenderInventoryV8449(AWarfareFrontlineGameMode* B)
{
 auto J=MakeShared<FJsonObject>();if(!B)return J;
 struct FRow {const FMaterialRenderProxy* Proxy=nullptr;FString Path,Base,Actual;bool Same=false,Complete=false,Valid=false;};
 auto Rows=MakeShared<TArray<FRow>,ESPMode::ThreadSafe>();TSet<UMaterialInterface*> Seen;TArray<TStrongObjectPtr<UMaterialInterface>> KeepAlive;
 bool Geometry=true,PSO=true;int32 ScorchMeshes=0,ScorchDecals=0;
 for(TActorIterator<AActor> It(B->GetWorld());It;++It)
 {
  if(It->IsHidden())continue;TInlineComponentArray<UStaticMeshComponent*> Cs(*It);
  for(auto* C:Cs)if(C&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->bRenderInMainPass&&C->GetStaticMesh())
  {
   if(auto* ISM=Cast<UInstancedStaticMeshComponent>(C))if(ISM->GetInstanceCount()==0)continue;
   PSO&=!C->GetPSOPrecacheComponentData().UsePSOPrecacheFallbackMaterial();
   if(It->IsA<AWarfareFrontlineScorchV8449>())
   {
    ++ScorchMeshes;const auto* D=C->GetStaticMesh()->GetRenderData();
    Geometry&=C->GetSceneProxy()&&D&&!D->LODResources.IsEmpty()&&D->LODResources[0].IndexBuffer.GetNumIndices()>=3;
   }
   for(int32 I=0;I<C->GetNumMaterials();++I)if(auto* M=C->GetMaterial(I))if(!Seen.Contains(M))
   {Seen.Add(M);KeepAlive.Emplace(M);FRow R;R.Proxy=M->GetRenderProxy();R.Path=M->GetPathName();R.Base=GetPathNameSafe(M->GetBaseMaterial());Rows->Add(MoveTemp(R));}
  }
  if(It->IsA<AWarfareFrontlineScorchV8449>())
  {
   TInlineComponentArray<UDecalComponent*> Decals(*It);
   for(auto* C:Decals)if(C&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame)
   {
    ++ScorchDecals;Geometry&=C->SceneProxy!=nullptr&&!C->DecalSize.ContainsNaN()&&C->DecalSize.GetMin()>0;
    if(auto* M=C->GetDecalMaterial())if(!Seen.Contains(M))
    {Seen.Add(M);KeepAlive.Emplace(M);FRow R;R.Proxy=M->GetRenderProxy();R.Path=M->GetPathName();R.Base=GetPathNameSafe(M->GetBaseMaterial());Rows->Add(MoveTemp(R));}
   }
  }
 }
 const auto Feature=B->GetWorld()->GetFeatureLevel();
 ENQUEUE_RENDER_COMMAND(WFFieldOriginalProxiesV8449)([Rows,Feature](FRHICommandListImmediate&)
 {
  for(auto& R:*Rows)if(R.Proxy)
  {
   const FMaterialRenderProxy* Effective=R.Proxy;const auto& M=R.Proxy->GetMaterialWithFallback(Feature,Effective);
   R.Actual=M.GetFriendlyName();R.Same=Effective==R.Proxy;R.Complete=M.IsRenderingThreadShaderMapComplete();
   const auto* Map=M.GetRenderingThreadShaderMap();R.Valid=Map&&Map->IsValidForRendering();
  }
 });
 FlushRenderingCommands();TArray<TSharedPtr<FJsonValue>> Values;bool Good=!Rows->IsEmpty();
 for(const auto& R:*Rows)
 {
  auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("assigned_material"),R.Path);V->SetStringField(TEXT("parent"),R.Base);V->SetStringField(TEXT("actual_render_material"),R.Actual);
  V->SetBoolField(TEXT("assigned_proxy_used"),R.Same);V->SetBoolField(TEXT("render_shader_complete"),R.Complete);V->SetBoolField(TEXT("render_shader_valid"),R.Valid);
  Good&=R.Same&&R.Complete&&R.Valid;Values.Add(MakeShared<FJsonValueObject>(V));
 }
 J->SetArrayField(TEXT("materials"),Values);J->SetBoolField(TEXT("original_render_proxies_valid_no_fallback"),Good);J->SetBoolField(TEXT("components_no_pso_fallback_requested"),PSO);
 J->SetNumberField(TEXT("visible_scorch_meshes"),ScorchMeshes);J->SetBoolField(TEXT("scorch_actual_proxy_and_indices_present"),Geometry&&ScorchMeshes>0);
 J->SetNumberField(TEXT("visible_scorch_native_decals"),ScorchDecals);J->SetBoolField(TEXT("scorch_actual_receiver_proxy_present"),Geometry&&(ScorchMeshes+ScorchDecals)>0);
 J->SetStringField(TEXT("scope"),TEXT("Original material shader proxies of visible installed static/instanced scene meshes, read on RenderThread. This does not prove the final ray-tracing SBT selection or mesh-decal colour visibility; captured pixels require review."));return J;
}
TSharedRef<FJsonObject> FieldTimingV8449(TArray<double> Values)
{
 auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());if(Values.IsEmpty())return J;
 double Sum=0;for(double V:Values)Sum+=V;Values.Sort();
 J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());J->SetNumberField(TEXT("p95_ms"),Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*.95)-1,0,Values.Num()-1)]);return J;
}
void StartFrontlineReviewV8449(AWarfareFrontlineController* Controller)
{
 FString User;
 if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return;
 struct FRun
 {
  FString Dir;int32 Stage=0,Scene=0,Page=0,Captures=0,Effect=0;bool Good=true,Baseline=false,RRRequested=false,ArtFocused=false,ArtBaseline=false,PropsFocused=false;
  double Began=FPlatformTime::Seconds(),Next=Began+.5,Prepared=0,SampleStart=0,LastFrame=0;
  uint64 Frame=0;FVector Focus;float Width=9000;double RRFramesBefore=0;
  TArray<double> Wall,GPU,Game,Render;TArray<TSharedPtr<FJsonValue>> Checks;
  TSharedPtr<FJsonObject> RRProbe;
 };
 auto Run=MakeShared<FRun>();Run->Dir=User;
 Run->ArtFocused=FParse::Param(FCommandLine::Get(),TEXT("WFV84412FieldFocused"));
 Run->PropsFocused=FParse::Param(FCommandLine::Get(),TEXT("WFV84413PropsFocused"));
 Run->ArtBaseline=Run->ArtFocused&&(FParse::Param(FCommandLine::Get(),TEXT("WFV84412BagBaseline"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84412DustBaseline")));
 IFileManager::Get().MakeDirectory(*(User/TEXT("Captures")),true);
 TWeakObjectPtr<AWarfareFrontlineController> Owner(Controller);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Owner](float)
 {
  auto* PC=Owner.Get();auto* B=PC?FrontlineFor(PC):nullptr;const double Now=FPlatformTime::Seconds();
  auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V8449_CHECK %s %d"),*Name,int32(Pass));};
  auto Write=[&](const FString& Name,TSharedRef<FJsonObject> J){FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Run->Dir/Name));};
  auto Finish=[&]()
  {
   auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Run->Good);J->SetArrayField(TEXT("checks"),Run->Checks);
   J->SetNumberField(TEXT("captures_requested"),Run->Captures);J->SetNumberField(TEXT("wall_seconds"),Now-Run->Began);J->SetBoolField(TEXT("field_baseline"),Run->Baseline);
   J->SetBoolField(TEXT("v84412_focused_art_review"),Run->ArtFocused);J->SetBoolField(TEXT("v84412_art_baseline"),Run->ArtBaseline);
   J->SetBoolField(TEXT("v84413_props_focused_review"),Run->PropsFocused);
   J->SetStringField(TEXT("scope"),TEXT("Six disposable British production fields: chalk, marsh, mud, February night snow, town rubble and rail. Actual ground/timber/root/bag/site bindings and contacts, production scorch receivers, matching 4K wide/detail cameras, short Native SDR High FG-off samples. Extra Quality+RR image is a separate runtime probe; Path Tracing is not the production pipeline and is not accepted by this review."));
   if(Run->ArtFocused)J->SetStringField(TEXT("scope"),TEXT("Three existing UK production fields: chalk, winter night and wet rail. Actual sandbag geometry/material/contact, paired gameplay/detail views; six frozen genuine cosmetic blast profiles and four-impact warmed render observation. Candidate-only Quality+RR image. No campaign, damage, animation, HDR or frame-generation acceptance; captured pixels require root review."));
   if(Run->PropsFocused)J->SetStringField(TEXT("scope"),TEXT("Corrective sandbag pass: chalk and wet rail production wide/detail views, plus actual scorch enabled/hidden matched Quality+RR captures. No repeat of unchanged gas or dust; disposable UK scene only."));
   Write(TEXT("checks.json"),J);FPlatformMisc::RequestExitWithStatus(false,Run->Good?0:1);return false;
  };
  if(!PC||!B||!PC->HasActorBegunPlay()||!B->HasActorBegunPlay()){if(Now-Run->Began<25)return true;Check(TEXT("v8449_world_ready"),false);return Finish();}
  if(Now-Run->Began>150){Check(TEXT("v8449_bounded_review"),false);return Finish();}
  if(Now<Run->Next||FScreenshotRequest::IsScreenshotRequested())return true;
  auto& S=ScreenState(PC);
  auto Clear=[&]()
  {
   S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=S.Deployment=S.UnitIntroduction=S.HistoricalDispatch=S.CampaignEndingV51=S.Credits=S.ExitConfirmV61=false;
   S.PhotoTransitionV37=S.SmallTransitionV35=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.Dossier=1917;S.Preparing=false;
   MedalMountV8216::CloseV836(PC);UGameplayStatics::SetGamePaused(PC,false);PC->CancelTargeting();V28ReviewPointer=FVector2D(-10000,-10000);
  };
  static const int32 Missions[]={3,5,7,10,13,17};
  static const TCHAR* Names[]={TEXT("chalk"),TEXT("marsh"),TEXT("mud"),TEXT("winter-night"),TEXT("rubble"),TEXT("rail")};
  if(Run->Stage==0)
  {
   B->SetActorTickEnabled(true);PC->SetActorTickEnabled(true);Clear();
   const bool Strict=B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B);
   Check(TEXT("v8449_strict_disposable_ukde_only"),Strict);if(!Strict)return Finish();
   Run->Baseline=FrontlineEnvironmentV42::FieldBaselineV8449(B);
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
   {R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetHDREnabled(false,false);R->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);R->SetQuality(EFrontlineRenderQuality::Native,false);R->SetFrameRateLimit(0,false);}
   if(B->Phase!=EFrontlinePhase::Briefing)B->AbandonBattleToOperations();
   Check(TEXT("v8449_actual_campaign_prepared"),B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Veteran));
   if(auto* Progress=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign())){Progress->CompletedMissions=19;Progress->WonOperationMaskV52=(1u<<19)-1;Progress->DispatchResolvedMaskV52=(1u<<20)-1;}
   const bool Selected=B->SelectCampaignMission(Missions[Run->Scene]);Check(TEXT("v8449_actual_operation_selected"),Selected);if(!Selected)return Finish();
   S.AcceptedLoadoutMission=INDEX_NONE;BeginOperationPreparation(PC,B);Run->Prepared=Now;Run->Stage=1;return true;
  }
  if(Run->Stage==1)
  {
   if(S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty()&&Now-Run->Prepared<35)return true;
   const bool Ready=B->Phase==EFrontlinePhase::Fighting&&!S.Preparing&&S.PreparationError.IsEmpty()&&B->AssetFailure.IsEmpty();Check(TEXT("v8449_actual_terrain_assets_ready"),Ready);if(!Ready)return Finish();
   if(auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld())){Weather->SetActorTickEnabled(true);Weather->SnapToWeatherForReview();Weather->SetActorTickEnabled(false);}
   B->SetActorTickEnabled(false);PC->SetActorTickEnabled(false);for(const auto& U:B->Units)if(IsValid(U))U->SetActorTickEnabled(false);
   Clear();Run->Stage=2;return true;
  }
  if(Run->Stage==2)
  {
   auto Field=FrontlineEnvironmentV42::RunChecks(B);Write(FString::Printf(TEXT("field-%d.json"),Run->Scene),Field);
   Check(TEXT("v8449_environment_actual_binding_contact_route_clear"),Field->GetBoolField(TEXT("success")));
   if(Run->ArtFocused)
   {
    auto Props=Field->GetObjectField(TEXT("field_props_v8449"));
    Check(TEXT("v84412_actual_expected_sandbag_route"),Props->GetBoolField(TEXT("sandbags_v84412"))!=Run->ArtBaseline);
    Check(TEXT("v84412_actual_sandbag_contact_complete"),Props->GetNumberField(TEXT("bag_ground_contact_errors"))==0);
   }
   FrontlineLightingV42::RunV842Checks(B,Check);Write(FString::Printf(TEXT("actual-lighting-%d.json"),Run->Scene),FrontlineLightingV42::DescribeV84(B));
   AWarfareFrontlineGround* Ground=nullptr;for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It){Ground=*It;break;}
   Check(TEXT("v8449_actual_ground_present"),Ground&&Ground->Surface);if(!Ground||!Ground->Surface)return Finish();
   FString GroundText=Ground->DescribeTerrainV43();TSharedPtr<FJsonObject> G;
   bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(GroundText),G)&&G.IsValid();Check(TEXT("v8449_ground_readback_parsed"),Parsed);if(Parsed)Write(FString::Printf(TEXT("ground-%d.json"),Run->Scene),G.ToSharedRef());
   static const TCHAR* NewParents[]={TEXT("M_GroundChalk51_V8449"),TEXT("M_GroundMud76_V8449"),TEXT("M_GroundMud76_V8449"),TEXT("M_GroundWinter51_V8449"),TEXT("M_GroundRubble58_V8449"),TEXT("M_GroundRubble51_V8449")};
   static const TCHAR* OldParents[]={TEXT("M_GroundChalk51_V839"),TEXT("M_GroundMud_V76"),TEXT("M_GroundMud_V76"),TEXT("M_GroundWinter51_V839"),TEXT("M_GroundRubble58_V839"),TEXT("M_GroundRubble51_V839")};
   auto* Assigned=Ground->Surface->GetMaterial(0);const FString Base=Assigned?GetNameSafe(Assigned->GetBaseMaterial()):TEXT("missing");
   Check(TEXT("v8449_ground_actual_expected_parent"),Base==(Run->Baseline?OldParents[Run->Scene]:NewParents[Run->Scene]));
   bool SameTiles=!Ground->BakedTerrainV43.IsEmpty();for(const auto& C:Ground->BakedTerrainV43)SameTiles&=C&&C->GetMaterial(0)==Assigned;
   Check(TEXT("v8449_visible_tiles_share_actual_surface_mid"),SameTiles);
   if(Parsed)
   {const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;bool Valid=G->TryGetArrayField(TEXT("visible_tiles"),Rows)&&Rows&&!Rows->IsEmpty();if(Valid)for(const auto& V:*Rows)Valid&=V->AsObject()->GetBoolField(TEXT("material_shader_valid"));Check(TEXT("v8449_installed_ground_shader_complete_valid"),Valid);}
   auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());Check(TEXT("v8449_weather_bound_to_final_ground"),Weather&&Weather->IsReady()&&Weather->UsesDisplayedGroundMaterial());
   if(Weather)
   {
    const auto Look=Weather->GetOperationLookV50();auto W=MakeShared<FJsonObject>();W->SetStringField(TEXT("profile"),Look.LightProfileKeyV842);W->SetStringField(TEXT("phase"),Look.LightPhaseV842);W->SetNumberField(TEXT("season_month"),Look.LightSeasonMonthV842);
    W->SetNumberField(TEXT("sun_intensity"),Weather->GetSunIntensity());W->SetNumberField(TEXT("sky_intensity"),Weather->GetSkyIntensity());W->SetNumberField(TEXT("wetness"),Weather->GetWetness());W->SetNumberField(TEXT("rain"),Weather->GetRainAmount());W->SetNumberField(TEXT("pitch"),Look.Pitch);W->SetNumberField(TEXT("yaw"),Look.Yaw);
    Check(TEXT("v8449_light_and_weather_finite"),FMath::IsFinite(Look.Pitch)&&FMath::IsFinite(Look.Yaw)&&FMath::IsFinite(Weather->GetSunIntensity())&&FMath::IsFinite(Weather->GetSkyIntensity())&&Weather->GetSunIntensity()>=0&&Weather->GetSkyIntensity()>=0);
    if(Missions[Run->Scene]==10)Check(TEXT("v8449_winter_is_february_night_operation"),FCString::Strcmp(Look.LightPhaseV842,TEXT("Night"))==0&&Look.LightSeasonMonthV842==2);
    Write(FString::Printf(TEXT("weather-%d.json"),Run->Scene),W);
   }
   bool Finite=true;int32 Components=0,Instances=0;
   for(TActorIterator<AActor> It(B->GetWorld());It;++It)
   {
    TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
    for(auto* C:Meshes)if(C&&C->IsVisible()&&C->GetStaticMesh())
    {
     const FTransform T=C->GetComponentTransform();Finite&=!T.ContainsNaN()&&T.GetLocation().GetAbsMax()<1000000&&T.GetScale3D().GetAbsMax()<100000;++Components;
     if(auto* ISM=Cast<UInstancedStaticMeshComponent>(C))for(int32 I=0;I<ISM->GetInstanceCount();++I)
     {FTransform X;const bool Read=ISM->GetInstanceTransform(I,X,true);Finite&=Read&&!X.ContainsNaN()&&X.GetLocation().GetAbsMax()<1000000;++Instances;}
    }
   }
   auto F=MakeShared<FJsonObject>();F->SetBoolField(TEXT("finite"),Finite);F->SetNumberField(TEXT("components"),Components);F->SetNumberField(TEXT("instances"),Instances);Write(FString::Printf(TEXT("positions-%d.json"),Run->Scene),F);
   Check(TEXT("v8449_all_visible_mesh_and_instance_transforms_finite"),Finite&&Components>0);
   if(!Run->Baseline)
   {
    AWarfareFrontlineScorchV8449::RunChecks(B,Check);
    if(auto* Scorch=AWarfareFrontlineScorchV8449::Get(B->GetWorld(),false))
    {auto D=Scorch->Describe();Write(FString::Printf(TEXT("scorch-%d.json"),Run->Scene),D);Check(TEXT("v8449_scorch_actual_shader_complete_valid_deferred_decal"),D->GetBoolField(TEXT("material_shader_complete"))&&D->GetBoolField(TEXT("material_shader_valid"))&&D->GetBoolField(TEXT("material_deferred_decal")));}
   }
   Run->Page=0;Run->Stage=3;return true;
  }
  if(Run->Stage==3)
  {
   Clear();FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);FVector Focus(0,0,180);float Width=9000;
   if(Run->Page==1)
   {
    double Best=DBL_MAX;bool Found=false;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
     TInlineComponentArray<UInstancedStaticMeshComponent*> Cs(*It);
     for(auto* C:Cs)if(C&&C->IsVisible()&&C->GetStaticMesh()&&C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_SplinteredTrunk_")))for(int32 I=0;I<C->GetInstanceCount();++I)
     {FTransform T;if(!C->GetInstanceTransform(I,T,true))continue;double Score=FVector2D(T.GetLocation().X,T.GetLocation().Y-2800).SizeSquared();if(Score<Best){Best=Score;Focus=T.GetLocation()+FVector(0,0,120);Found=true;}}
    }
    if(!Run->Baseline)
    {
     const auto Sites=FrontlineFieldSitesV59::DescribeVisualsV8449(B);const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
     double Largest=-DBL_MAX;bool SiteFound=false;
     if(Sites->TryGetArrayField(TEXT("sites"),Rows))for(const auto& V:*Rows)
     {
      const auto J=V->AsObject();double X,Y,Z,Gap=0;
      if(J&&J->TryGetNumberField(TEXT("x"),X)&&J->TryGetNumberField(TEXT("y"),Y)&&J->TryGetNumberField(TEXT("z"),Z))
      {
       double Front=0;J->TryGetNumberField(TEXT("rear_embed_max_cm"),Gap);J->TryGetNumberField(TEXT("front_max_gap_cm"),Front);
       Gap=FMath::Max(Gap,Front);
       if(Gap>15.f&&Gap>Largest){Largest=Gap;Focus=FVector(X,Y,Z+100);SiteFound=true;}
      }
     }
     Check(TEXT("v8449_actual_root_or_doorway_ground_detail_camera"),SiteFound||Found);Found|=SiteFound;
    }
    else Check(TEXT("v8449_actual_root_and_ground_detail_camera"),Found);
    Width=2600;
   }
   if(Run->Baseline)
   {
    FString Source;FParse::Value(FCommandLine::Get(),TEXT("WFV8449CameraSource="),Source);Source=FPaths::ConvertRelativePathToFull(Source);FPaths::NormalizeDirectoryName(Source);
    const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8449/Runtime/"));
    const FString Stamp=Source.StartsWith(Root,ESearchCase::IgnoreCase)?Source.Mid(Root.Len()):FString();
    bool Safe=Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')&&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric()&&!Source.Contains(TEXT(".."));
    FString Text;TSharedPtr<FJsonObject> Camera;const bool Loaded=Safe&&FFileHelper::LoadFileToString(Text,*(Source/FString::Printf(TEXT("camera-%d-%d.json"),Run->Scene,Run->Page)))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Camera)&&Camera;
    Check(TEXT("v8449_baseline_exact_candidate_camera"),Loaded);if(!Loaded)return Finish();Focus=FVector(Camera->GetNumberField(TEXT("x")),Camera->GetNumberField(TEXT("y")),Camera->GetNumberField(TEXT("z")));Width=Camera->GetNumberField(TEXT("width"));
   }
   if(Run->ArtFocused)
   {
    const int32 T=FMath::Clamp(FrontlineBattlefield::CentreTrench,0,B->Trenches.Num()-1);
    const float Y=91.f;const float X=FrontlineFieldV32::TrenchCentreX(T,Y)+235.f;
    Focus=FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+95.f);Width=Run->Page?2600.f:6000.f;
    if(Run->ArtBaseline)
    {
     FString Source;FParse::Value(FCommandLine::Get(),TEXT("WFV8449CameraSource="),Source);Source=FPaths::ConvertRelativePathToFull(Source);FPaths::NormalizeDirectoryName(Source);
     const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8449/Runtime/"));
     const FString Stamp=Source.StartsWith(Root,ESearchCase::IgnoreCase)?Source.Mid(Root.Len()):FString();
     const bool Safe=Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')&&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric()&&!Source.Contains(TEXT(".."));
     FString Text;TSharedPtr<FJsonObject> C;const bool Loaded=Safe&&FFileHelper::LoadFileToString(Text,*(Source/FString::Printf(TEXT("camera-%d-%d.json"),Run->Scene,Run->Page)))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),C)&&C;
     Check(TEXT("v84412_baseline_exact_candidate_camera"),Loaded);if(!Loaded)return Finish();Focus=FVector(C->GetNumberField(TEXT("x")),C->GetNumberField(TEXT("y")),C->GetNumberField(TEXT("z")));Width=C->GetNumberField(TEXT("width"));
    }
   }
   Check(TEXT("v8449_camera_finite_within_gameplay_zoom"),!Focus.ContainsNaN()&&FMath::IsFinite(Width)&&Width>=2600&&Width<=9000);
   Run->Focus=Focus;Run->Width=Width;FFrontlineUIV8447Review::Camera(PC,Focus,Width);
   if(PC->BattleCamera)FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),Focus,Width,3840);
   Run->Frame=GFrameCounter;Run->Stage=4;Run->Next=Now+1.5;return true;
  }
  if(Run->Stage==4)
  {
   if(GFrameCounter<=Run->Frame+2)return true;int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("v8449_actual_4k_native_high_sdr_fg_off"),W==3840&&H==2160);
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
   {auto D=R->GetDiagnostics();Write(FString::Printf(TEXT("render-%d-%d.json"),Run->Scene,Run->Page),D);Check(TEXT("v8449_sample_pipeline_is_native_high_sdr_fg_off"),R->GetQuality()==EFrontlineRenderQuality::Native&&R->GetGraphicsPreset()==EFrontlineGraphicsPreset::High&&R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off&&!R->GetHDRStatus().bOutputActive&&!D->GetBoolField(TEXT("dlss_enabled_api")));}
   auto C=MakeShared<FJsonObject>();C->SetNumberField(TEXT("x"),Run->Focus.X);C->SetNumberField(TEXT("y"),Run->Focus.Y);C->SetNumberField(TEXT("z"),Run->Focus.Z);C->SetNumberField(TEXT("width"),Run->Width);Write(FString::Printf(TEXT("camera-%d-%d.json"),Run->Scene,Run->Page),C);
   if(Run->Page==0)
   {
    auto Inventory=FieldRenderInventoryV8449(B);Write(FString::Printf(TEXT("actual-render-proxies-%d.json"),Run->Scene),Inventory);
    Check(TEXT("v8449_visible_actual_original_render_shaders_no_fallback"),Inventory->GetBoolField(TEXT("original_render_proxies_valid_no_fallback"))&&Inventory->GetBoolField(TEXT("components_no_pso_fallback_requested")));
    if(!Run->Baseline)Check(TEXT("v8449_scorch_actual_render_receiver_proxy_present"),Inventory->GetBoolField(TEXT("scorch_actual_receiver_proxy_present")));
    Write(FString::Printf(TEXT("actual-lighting-bindings-%d.json"),Run->Scene),FrontlineLightingV42::DescribeLiveReflectionBindingsV44(B));
    // Report the actively bound High branch, rather than deriving quality
    // from default/placeholder texture dimensions during asynchronous init.
    AWarfareFrontlineGround* Ground=nullptr;for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It){Ground=*It;break;}
    if(Ground&&Ground->Surface)if(auto* M=Ground->Surface->GetMaterial(0))
    {
     TArray<UTexture*> Used;M->GetUsedTextures(Used,EMaterialQualityLevel::High,GMaxRHIShaderPlatform);TArray<TSharedPtr<FJsonValue>> Textures;
     for(auto* T:Used)if(auto* X=Cast<UTexture2D>(T))
     {auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("asset"),X->GetPathName());V->SetNumberField(TEXT("width"),X->GetSizeX());V->SetNumberField(TEXT("height"),X->GetSizeY());V->SetNumberField(TEXT("total_mips"),X->GetNumMips());V->SetNumberField(TEXT("resident_mips"),X->GetNumResidentMips());V->SetNumberField(TEXT("largest_resident_width"),X->GetNumResidentMips()>0?FMath::Max(1,X->GetSizeX()>>FMath::Clamp(X->GetNumMips()-X->GetNumResidentMips(),0,30)):0);V->SetBoolField(TEXT("pending_init_or_streaming"),X->HasPendingInitOrStreaming());Textures.Add(MakeShared<FJsonValueObject>(V));}
     auto D=MakeShared<FJsonObject>();D->SetStringField(TEXT("material"),M->GetPathName());D->SetStringField(TEXT("quality"),TEXT("High"));D->SetArrayField(TEXT("textures"),Textures);Write(FString::Printf(TEXT("actual-high-textures-%d.json"),Run->Scene),D);
    }
   }
   const TCHAR* Detail=!Run->Baseline?TEXT("contact-detail"):TEXT("root-detail");
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/FString::Printf(TEXT("%02d-%s-%s.png"),Run->Scene*2+Run->Page+1,Names[Run->Scene],Run->Page?Detail:TEXT("wide")),true,false);++Run->Captures;
   if(Run->Page==0){Run->SampleStart=Run->LastFrame=FPlatformTime::Seconds()+1.;Run->Wall.Empty();Run->GPU.Empty();Run->Game.Empty();Run->Render.Empty();Run->Stage=5;}
   else{Run->Stage=6;Run->Next=Now+.1;}return true;
  }
  if(Run->Stage==5)
  {
   if(Now<Run->SampleStart)return true;
   if(Now>Run->LastFrame){Run->Wall.Add((Now-Run->LastFrame)*1000);Run->GPU.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Run->Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));Run->Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));}
   Run->LastFrame=Now;if(Now-Run->SampleStart<3.)return true;
   auto T=MakeShared<FJsonObject>();T->SetObjectField(TEXT("wall"),FieldTimingV8449(Run->Wall));T->SetObjectField(TEXT("gpu"),FieldTimingV8449(Run->GPU));T->SetObjectField(TEXT("game_thread"),FieldTimingV8449(Run->Game));T->SetObjectField(TEXT("render_thread"),FieldTimingV8449(Run->Render));T->SetNumberField(TEXT("sample_seconds"),Now-Run->SampleStart);T->SetBoolField(TEXT("field_baseline"),Run->Baseline);
   T->SetStringField(TEXT("scope"),TEXT("Native 100% SDR High FG-off fixed-view short warmed observation; candidate/baseline use identical explicit settings. Not sustained battle FPS or HDR/FG acceptance."));Write(FString::Printf(TEXT("timing-%d.json"),Run->Scene),T);
   Run->Page=1;Run->Stage=3;return true;
  }
  if(Run->Stage==6)
  {
   if(Run->PropsFocused)
   {
    if(Run->Scene==0){Run->Scene=5;Run->Stage=0;return true;}
    Run->Stage=7;return true;
   }
   if(Run->ArtFocused)
   {
    if(Run->Scene==0){Run->Scene=3;Run->Stage=0;return true;}
    if(Run->Scene==3){Run->Scene=5;Run->Stage=0;return true;}
    Run->Effect=0;Run->Stage=12;return true;
   }
   if(++Run->Scene<6){Run->Stage=0;return true;}
   if(Run->Baseline){Run->Stage=11;return true;}
   Run->Stage=7;return true;
  }
  if(Run->Stage==7)
  {
   Run->RRProbe=MakeShared<FJsonObject>();auto& P=*Run->RRProbe;P.SetBoolField(TEXT("path_tracing_accepted"),false);P.SetStringField(TEXT("path_tracing_status"),TEXT("Not tested: actual production pipeline is Lit/Lumen, not progressive Path Tracing. No debug view override was applied."));
   auto* View=PC->GetWorld()->GetGameViewport();P.SetBoolField(TEXT("actual_path_tracing_showflag"),View&&View->EngineShowFlags.PathTracing);P.SetNumberField(TEXT("actual_view_mode"),View?View->ViewModeIndex:-1);
   if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.PathTracing")))P.SetNumberField(TEXT("path_tracing_cvar"),C->GetInt());
   P.SetBoolField(TEXT("mesh_decal_visibility_requires_image_review"),true);P.SetStringField(TEXT("scope"),TEXT("One separate Quality+Ray Reconstruction production camera probe. API enable alone is insufficient; actual RR view families and captured receiver appearance must be inspected."));
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
   {
    auto D=R->GetDiagnostics();Run->RRFramesBefore=D->GetNumberField(TEXT("observed_rr_frames"));P.SetObjectField(TEXT("before"),D);
    Run->RRRequested=R->SetQuality(EFrontlineRenderQuality::Quality,false)&&R->SetRayReconstruction(true,false);P.SetBoolField(TEXT("rr_request_accepted"),Run->RRRequested);
   }
   FVector Focus=Run->Focus;
   if(auto* Scorch=AWarfareFrontlineScorchV8449::Get(B->GetWorld(),false))
   {auto D=Scorch->Describe();P.SetObjectField(TEXT("scorch_before_rr"),D);double X,Y,Z;if(D->TryGetNumberField(TEXT("review_focus_x"),X)&&D->TryGetNumberField(TEXT("review_focus_y"),Y)&&D->TryGetNumberField(TEXT("review_focus_z"),Z))Focus=FVector(X,Y,Z+100);}
   Check(TEXT("v8449_rr_probe_camera_finite"),!Focus.ContainsNaN());FFrontlineUIV8447Review::Camera(PC,Focus,2600);Run->Stage=8;Run->Next=Now+4.;return true;
  }
  if(Run->Stage==8)
  {
   auto Inventory=FieldRenderInventoryV8449(B);Run->RRProbe->SetObjectField(TEXT("actual_render_inventory"),Inventory);Run->RRProbe->SetObjectField(TEXT("actual_lighting_bindings"),FrontlineLightingV42::DescribeLiveReflectionBindingsV44(B));
   bool Effective=false;if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
   {auto D=R->GetDiagnostics();Run->RRProbe->SetObjectField(TEXT("after"),D);const double Delta=D->GetNumberField(TEXT("observed_rr_frames"))-Run->RRFramesBefore;Run->RRProbe->SetNumberField(TEXT("observed_rr_view_family_delta"),Delta);Effective=Run->RRRequested&&D->GetBoolField(TEXT("rr_enabled_api"))&&Delta>2;}
   Run->RRProbe->SetBoolField(TEXT("rr_executed"),Effective);Run->RRProbe->SetStringField(TEXT("rr_status"),Effective?TEXT("Executed; image review still required"):TEXT("Unavailable or not executed; NOT accepted as an RR visibility test"));Write(TEXT("rr-probe.json"),Run->RRProbe.ToSharedRef());
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures/13-rail-scorch-rr-probe.png"),true,false);++Run->Captures;Run->Stage=9;Run->Next=Now+.1;return true;
  }
  if(Run->Stage==9)
  {
   auto* Scorch=AWarfareFrontlineScorchV8449::Get(B->GetWorld(),false);
   int32 Hidden=0;if(Scorch)
   {
    TInlineComponentArray<UStaticMeshComponent*> Components(Scorch);for(auto* C:Components)if(C&&C->IsVisible()&&C->GetStaticMesh()){C->SetVisibility(false);++Hidden;}
    TInlineComponentArray<UDecalComponent*> Decals(Scorch);for(auto* C:Decals)if(C&&C->IsVisible()&&C->GetDecalMaterial()){C->SetVisibility(false);++Hidden;}
   }
   Check(TEXT("v8449_scorch_paired_pixel_witness_hides_actual_meshes"),Hidden>0);
   Run->Stage=10;Run->Next=Now+4.;return true;
  }
  if(Run->Stage==10)
  {
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures/14-rail-scorch-hidden-rr-control.png"),true,false);++Run->Captures;
   Run->Stage=11;Run->Next=Now+.1;return true;
  }
  if(Run->Stage==11)
  {
   const int32 Expected=Run->PropsFocused?6:Run->Baseline?12:14;
   TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Run->Dir/TEXT("Captures/*.png")),true,false);bool Saved=Files.Num()==Expected;for(const auto& F:Files)Saved&=IFileManager::Get().FileSize(*(Run->Dir/TEXT("Captures")/F))>1024;
   Check(Run->PropsFocused?TEXT("v84413_corrective_props_and_scorch_capture_pairs_written"):TEXT("v8449_all_six_affected_field_capture_pairs_written"),Saved&&Run->Captures==Expected);return Finish();
  }
  if(Run->ArtFocused&&Run->Stage==12)
  {
   FrontlineImpactV831::ClearVisualFixture(B);
   static const int32 Profiles[]={2,1,0,5,4,0};static const float Ages[]={.12f,.28f,.42f,.28f,.15f,.42f};
   const FVector P(0.f,550.f,AWarfareFrontlineGround::HeightAt(0.f,550.f)+2.f);
   auto* A=FrontlineImpactV831::SetupVisualFixture(B,Profiles[Run->Effect],Ages[Run->Effect],false,P,2);
   Check(TEXT("v84412_actual_cosmetic_profile_frozen"),A&&A->IsVisualReady());if(!A)return Finish();
   Run->Focus=P+FVector(0,0,80);Run->Width=Run->Effect==0||Run->Effect==4?6000.f:Run->Effect==3||Run->Effect==5?2600.f:4000.f;
   FFrontlineUIV8447Review::Camera(PC,Run->Focus+(Run->Effect==5?FVector(0,280,120):FVector::ZeroVector),Run->Width);
   Run->Stage=13;Run->Next=Now+1.;return true;
  }
  if(Run->ArtFocused&&Run->Stage==13)
  {
   auto J=FrontlineImpactV831::DescribeVisualFixture(B);J->SetBoolField(TEXT("v84412_baseline"),Run->ArtBaseline);Write(FString::Printf(TEXT("effect-%d.json"),Run->Effect),J);
   const auto& Actors=J->GetArrayField(TEXT("actors"));bool Bound=!Actors.IsEmpty();
   for(const auto& V:Actors){const auto A=V->AsObject();Bound&=A->GetBoolField(TEXT("particulates_v84412"))!=Run->ArtBaseline;}
   Check(TEXT("v84412_actual_particulate_composition_matches_route"),Bound);
   auto Inventory=FieldRenderInventoryV8449(B);Check(TEXT("v84412_effect_actual_shaders_no_fallback"),Inventory->GetBoolField(TEXT("original_render_proxies_valid_no_fallback")));
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/FString::Printf(TEXT("dust-%02d.png"),Run->Effect),true,false);++Run->Captures;
   if(++Run->Effect<6){Run->Stage=12;return true;}Run->Stage=14;Run->Next=Now+.1;return true;
  }
  if(Run->ArtFocused&&Run->Stage==14)
  {
   FrontlineImpactV831::ClearVisualFixture(B);bool Ready=true;
   for(int32 I=0;I<4;++I){FVector P((I%2-.5f)*700.f,550.f+(I/2-.5f)*500.f,0);P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+2.f;Ready&=FrontlineImpactV831::SetupVisualFixture(B,I%2,.32f,false,P,2)!=nullptr;}
   Check(TEXT("v84412_four_actual_impacts_ready"),Ready);if(!Ready)return Finish();
   FFrontlineUIV8447Review::Camera(PC,FVector(0,550,180),6000);
   Run->Wall.Empty();Run->GPU.Empty();Run->Game.Empty();Run->Render.Empty();Run->SampleStart=Run->LastFrame=Now+2.;Run->Stage=15;return true;
  }
  if(Run->ArtFocused&&Run->Stage==15)
  {
   if(Now<Run->SampleStart)return true;
   if(Now>Run->LastFrame){Run->Wall.Add((Now-Run->LastFrame)*1000);Run->GPU.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Run->Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));Run->Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));}Run->LastFrame=Now;
   if(Now-Run->SampleStart<4.)return true;
   auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),FieldTimingV8449(Run->Wall));J->SetObjectField(TEXT("gpu"),FieldTimingV8449(Run->GPU));J->SetObjectField(TEXT("game_thread"),FieldTimingV8449(Run->Game));J->SetObjectField(TEXT("render_thread"),FieldTimingV8449(Run->Render));J->SetBoolField(TEXT("v84412_baseline"),Run->ArtBaseline);J->SetStringField(TEXT("scope"),TEXT("Four frozen actual production blast actors, paired Native SDR 4K High fixed camera after two-second warmup. Cosmetic GPU comparison only, not natural battle or FG FPS."));Write(TEXT("dust-timing.json"),J);
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures/dust-stress.png"),true,false);++Run->Captures;
   if(Run->ArtBaseline){Run->Stage=17;return true;}
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){auto D=R->GetDiagnostics();Run->RRFramesBefore=D->GetNumberField(TEXT("observed_rr_frames"));Run->RRRequested=R->SetQuality(EFrontlineRenderQuality::Quality,false)&&R->SetRayReconstruction(true,false);}
   Run->Stage=16;Run->Next=Now+4.;return true;
  }
  if(Run->ArtFocused&&Run->Stage==16)
  {
   auto J=MakeShared<FJsonObject>();bool RR=false;
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){auto D=R->GetDiagnostics();J->SetObjectField(TEXT("actual_render"),D);RR=Run->RRRequested&&D->GetBoolField(TEXT("rr_enabled_api"))&&D->GetNumberField(TEXT("observed_rr_frames"))>Run->RRFramesBefore+2.;}J->SetBoolField(TEXT("rr_executed"),RR);Write(TEXT("dust-rr.json"),J);
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures/dust-stress-quality-rr.png"),true,false);++Run->Captures;Run->Stage=17;Run->Next=Now+.2;return true;
  }
  if(Run->ArtFocused&&Run->Stage==17)
  {
   FrontlineImpactV831::ClearVisualFixture(B);
   if(auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld())){R->SetQuality(EFrontlineRenderQuality::Native,false);R->SetRayReconstruction(false,false);}
   const FVector P(0,550,AWarfareFrontlineGround::HeightAt(0,550));
   const bool Ready=PrepareFrontlineGasVisualV836(B,P);Check(TEXT("v84412_actual_gas_cosmetic_fixture"),Ready);if(!Ready)return Finish();
   Run->Page=0;FFrontlineUIV8447Review::Camera(PC,P+FVector(0,0,130),6000);Run->Next=Now+2.;Run->Stage=19;return true;
  }
  if(Run->ArtFocused&&Run->Stage==19)
  {
   auto Inventory=FieldRenderInventoryV8449(B);Write(FString::Printf(TEXT("gas-shaders-%d.json"),Run->Page),Inventory);
   Check(TEXT("v84412_gas_actual_shader_no_fallback"),Inventory->GetBoolField(TEXT("original_render_proxies_valid_no_fallback")));
   int32 BoundGas=0,VisibleGas=0;bool CorrectGas=true;TArray<TSharedPtr<FJsonValue>> GasBindings;
   for(TActorIterator<AFrontlineSupportMissionV28> It(B->GetWorld());It;++It)
   {
    TArray<UStaticMeshComponent*> Parts;It->GetComponents<UStaticMeshComponent>(Parts);
    for(auto* C:Parts)if(C&&C->GetMaterial(0))
    {const auto* Base=C->GetMaterial(0)->GetBaseMaterial();const bool New=Base&&Base->GetName()==TEXT("M_GasVolume_V84413");const bool Old=Base&&Base->GetName()==TEXT("M_GasVolume_V836");CorrectGas&=Run->ArtBaseline?Old:New;++BoundGas;VisibleGas+=C->IsVisible()?1:0;
     auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("component"),C->GetPathName());Row->SetStringField(TEXT("base"),Base?Base->GetPathName():TEXT("missing"));Row->SetBoolField(TEXT("visible"),C->IsVisible());GasBindings.Add(MakeShared<FJsonValueObject>(Row));}
   }
   // The production fixture is frozen 7.5 s after arrival. Releases are 1.7 s
   // apart: five clouds are visible, while the sixth still owns its real binding.
   auto GasScope=MakeShared<FJsonObject>();GasScope->SetArrayField(TEXT("components"),GasBindings);GasScope->SetNumberField(TEXT("bound"),BoundGas);GasScope->SetNumberField(TEXT("visible"),VisibleGas);GasScope->SetNumberField(TEXT("fixture_age_seconds"),7.5);Write(FString::Printf(TEXT("gas-bindings-%d.json"),Run->Page),GasScope);
   Check(TEXT("v84412_actual_gas_parent_matches_candidate_or_baseline"),CorrectGas&&BoundGas==FrontlineSmokeVolumeV35::CloudsPerMission&&VisibleGas==5);
   FScreenshotRequest::RequestScreenshot(Run->Dir/TEXT("Captures")/FString::Printf(TEXT("gas-%d.png"),Run->Page),true,false);++Run->Captures;
   if(Run->Page++==0){Run->Stage=20;return true;}
   Run->Stage=18;Run->Next=Now+.2;return true;
  }
  if(Run->ArtFocused&&Run->Stage==20)
  {
   FFrontlineUIV8447Review::Camera(PC,FVector(0,550,AWarfareFrontlineGround::HeightAt(0,550)+130),3200);Run->Next=Now+1.;Run->Stage=19;return true;
  }
  if(Run->ArtFocused&&Run->Stage==18)
  {
   TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Run->Dir/TEXT("Captures/*.png")),true,false);bool Saved=Files.Num()==Run->Captures&&Run->Captures==(Run->ArtBaseline?15:16);for(const auto& F:Files)Saved&=IFileManager::Get().FileSize(*(Run->Dir/TEXT("Captures")/F))>1024;
   Check(TEXT("v84412_focused_capture_files_written"),Saved);FrontlineImpactV831::ClearVisualFixture(B);return Finish();
  }
  Check(TEXT("v8449_known_stage"),false);return Finish();
 }),0.f);
}
}
