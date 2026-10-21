// V68 isolated art review. Each scene is prepared once, frozen, then captured
// for every requested variant in the same process: identical camera, pose,
// weather clock, exposure, resolution and SR/RR/FG. Disposable profile only.
#include "WarfareFrontlineArtV68.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "SceneViewExtension.h"
namespace
{
// The review pauses the world to freeze poses, particles and the weather clock.
// A paused family makes UE keep read-only temporal history; the next variant
// would then blend against the previous material. Keep history live, exactly
// as the V42 static reference does for its paused frame.
class FArtReviewViewV68 final:public FWorldSceneViewExtension
{
public:
    FArtReviewViewV68(const FAutoRegister& Register,UWorld* World):FWorldSceneViewExtension(Register,World){}
    virtual void SetupViewFamily(FSceneViewFamily& Family) override{Family.bWorldIsPaused=false;}
    virtual void SetupView(FSceneViewFamily&,FSceneView&) override{}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override{}
};
struct FArtVariantV68
{
    FString Name;int32 Soldiers=0,Terrain=0,Light=0;
    float Yaw=0,Pitch=0,Sun=1,Sky=1;bool bLightParams=false;
};
struct FFrontlineArtReviewV68 : public TSharedFromThis<FFrontlineArtReviewV68>
{
 TWeakObjectPtr<AWarfareFrontlineController> PC;
 TSharedPtr<FArtReviewViewV68,ESPMode::ThreadSafe> View;
 TArray<FString> Scenes;TArray<FArtVariantV68> Variants;
 int32 Scene=0,Variant=0,Phase=0;bool Good=true;
 double Started=0,Next=0,SettleFrom=0,GpuMs=0,GameMs=0,RenderMs=0;int32 Samples=0;
 float FinalYaw=0,FinalPitch=0,FinalSun=1,FinalSky=1;
 FString Dir;TArray<TSharedPtr<FJsonValue>> Checks,Captures;
 void Check(const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V68_CHECK %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));}
 static void SetCVar(const TCHAR* Name,float Value){if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))V->Set(*FString::SanitizeFloat(Value),ECVF_SetByConsole);}
 void Finish()
 {
  auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("captures"),Captures);
  J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
  if(auto* C=PC.Get())if(auto* R=UFrontlineRenderSettingsSubsystem::Get(C->GetWorld()))J->SetObjectField(TEXT("renderer"),R->GetDiagnostics());
  J->SetStringField(TEXT("scope"),TEXT("Same-process V67/V68 art variants at frozen fixed cameras. Captures need visual judgement; no performance, audio, campaign or save claim."));
  FString T;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&T));FFileHelper::SaveStringToFile(T,*(Dir/TEXT("review.json")));
  if(auto* C=PC.Get())UGameplayStatics::SetGamePaused(C,false);
  View.Reset();FPlatformMisc::RequestExit(false);
 }
 void ParseOptions()
 {
  // Lists contain commas: FParse::Value must not stop at the first separator.
  FString List=TEXT("v36normal,v36close,trench,figures,field,wide,aircraft");FParse::Value(FCommandLine::Get(),TEXT("WFV68Scenes="),List,false);List.ParseIntoArray(Scenes,TEXT(","));
  // "v68" means the shipped defaults unless a probe overrides them.
  auto Default=[](const TCHAR* N,float F){const auto* V=IConsoleManager::Get().FindConsoleVariable(N);return V?V->GetFloat():F;};
  FinalYaw=Default(TEXT("wf.Art.V68.SunYawOffset"),0);FinalPitch=Default(TEXT("wf.Art.V68.SunPitchOffset"),0);
  FinalSun=Default(TEXT("wf.Art.V68.SunScale"),1);FinalSky=Default(TEXT("wf.Art.V68.SkyScale"),1);
  FParse::Value(FCommandLine::Get(),TEXT("WFV68SunYaw="),FinalYaw);FParse::Value(FCommandLine::Get(),TEXT("WFV68SunPitch="),FinalPitch);
  FParse::Value(FCommandLine::Get(),TEXT("WFV68SunScale="),FinalSun);FParse::Value(FCommandLine::Get(),TEXT("WFV68SkyScale="),FinalSky);
  FString Names=TEXT("v67,v68");FParse::Value(FCommandLine::Get(),TEXT("WFV68Variants="),Names,false);TArray<FString> Parsed;Names.ParseIntoArray(Parsed,TEXT(","));
  for(const FString& N:Parsed)
  {
   FArtVariantV68 V;V.Name=N;
   if(N==TEXT("v68")){V.Soldiers=V.Terrain=V.Light=1;}
   else if(N==TEXT("soldiers"))V.Soldiers=1;
   else if(N==TEXT("terrain"))V.Terrain=1;
   else if(N==TEXT("light"))V.Light=1;
   else if(N.StartsWith(TEXT("yaw"))){V.Light=1;V.bLightParams=true;V.Yaw=FCString::Atof(*N.Mid(3));}
   else if(N.StartsWith(TEXT("pitch"))){V.Light=1;V.bLightParams=true;V.Pitch=FCString::Atof(*N.Mid(5));}
   else if(N.StartsWith(TEXT("sky"))){V.Light=1;V.bLightParams=true;V.Sky=FCString::Atof(*N.Mid(3));}
   else if(N!=TEXT("v67"))continue;
   Variants.Add(V);
  }
 }
 void ApplyVariant(const FArtVariantV68& V)
 {
  SetCVar(TEXT("wf.Art.V68.Soldiers"),V.Soldiers);SetCVar(TEXT("wf.Art.V68.Terrain"),V.Terrain);SetCVar(TEXT("wf.Art.V68.Light"),V.Light);
  SetCVar(TEXT("wf.Art.V68.SunYawOffset"),V.bLightParams?V.Yaw:FinalYaw);SetCVar(TEXT("wf.Art.V68.SunPitchOffset"),V.bLightParams?V.Pitch:FinalPitch);
  SetCVar(TEXT("wf.Art.V68.SunScale"),V.bLightParams?V.Sun:FinalSun);SetCVar(TEXT("wf.Art.V68.SkyScale"),V.bLightParams?V.Sky:FinalSky);
  if(auto* C=PC.Get())FrontlineArtV68::ApplyWorld(C->GetWorld());
 }
 static void Freeze(AWarfareFrontlineSoldier* U,UAnimationAsset* Motion,float Phase)
 {
  if(!U||!U->Soldier)return;U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;
  if(auto* A=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))if(Motion)A->TransitionTo(Motion,true,1.f,Phase,0.f);
  U->Soldier->TickAnimation(.05f,false);U->Soldier->RefreshBoneTransforms();
 }
 bool Battle(AWarfareFrontlineController* C,AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Op)
 {
  UGameplayStatics::SetGamePaused(C,false);B->AbandonBattleToOperations();FrontlineReviewUIV41::Clear(C);
  const bool Ready=FrontlineCampaignV41::PrepareReviewOperation(B,F,Op);
  B->SetActorTickEnabled(false);B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);
  auto& S=ScreenState(C);S.Preparing=S.Deployment=S.HistoricalDispatch=S.UnitIntroduction=false;S.Dossier=1917;
  auto& Map=WarRoom52::State(C);FrontlineMapSceneV61::Update(C,false,Map.Centre,Map.Zoom,1,Map.AtlasSector,Map.Preview,Map.Detail);
  return Ready&&B->Phase==EFrontlinePhase::Fighting;
 }
 void Trench(AWarfareFrontlineGameMode* B,int32 Team,EFrontlineRole Role,int32 Trench,int32 Slot)
 {
  B->Trenches[Trench].Owner=Team;FFrontlineReviewV32::Spawn(B,Role,Team,Trench);if(B->Squads.IsEmpty())return;
  auto& S=B->Squads.Last();S.Slot=Slot;S.bHoldPosition=true;
  for(const auto& W:S.Members)if(auto* U=W.Get())
  {
   FVector P=FrontlineMachinegunsV30::TrenchFiringStepV36(U,B->SoldierDestination(U));
   U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
   if(auto* Gun=FrontlineMachinegunsV30::Get(U))for(int32 I=0;I<70&&!Gun->IsSupported();++I)Gun->Update(.1f,false);
   Freeze(U,U->GetAimMotion(),.35f+.07f*U->MemberIndex);
  }
 }
 AWarfareFrontlineSoldier* Figure(AWarfareFrontlineGameMode* B,int32 Team,EFrontlineRole Role,const FVector& At,float Yaw,int32 Pose)
 {
  FVector P=At;P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88;
  auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P,FRotator(0,Yaw,0));if(!U)return nullptr;
  U->Initialize(B,68000+B->Units.Num(),Team,Role,0);B->Units.Add(U);
  Freeze(U,Pose==0?U->GetIdleMotion():Pose==1?U->GetAimMotion():U->GetWalkMotion(),Pose==2?.30f:.20f);return U;
 }
 bool SetupScene(AWarfareFrontlineController* C,AWarfareFrontlineGameMode* B,const FString& Name)
 {
  const float TrenchX=FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,0);
  auto FreezeAll=[&](){for(const auto& U:B->Units)if(U&&U->IsAlive())Freeze(U,nullptr,0);};
  if(Name==TEXT("v36normal")||Name==TEXT("v36close"))
  {
   // The exact V36 fixture and cameras behind the V67 Shipping evidence
   // (18-...-normal: British crew at 3400; 19-...-close: German crew at 1800).
   // Must precede campaign scenes: it uses the startup encounter.
   UGameplayStatics::SetGamePaused(C,false);FrontlineReviewUIV41::Clear(C);
   const bool German=Name==TEXT("v36close");
   const bool Ready=SetupV36TrenchCrew(B,[&](const FString& N,bool P){Check(Name+TEXT("_")+N,P);},German?1:0);
   FreezeAll();FFrontlineReviewV32::Camera(C,0,0,German?1800:3400,80);return Ready;
  }
  if(Name==TEXT("trench"))
  {
   // British Lewis section and rifle section in Arras's real centre trench.
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   Trench(B,0,EFrontlineRole::Lewis,FrontlineBattlefield::CentreTrench,1);Trench(B,0,EFrontlineRole::Rifle,FrontlineBattlefield::CentreTrench,3);
   FFrontlineReviewV32::Camera(C,TrenchX,0,3400,80);return Ready;
  }
  if(Name==TEXT("trenchde"))
  {
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   Trench(B,1,EFrontlineRole::Lewis,FrontlineBattlefield::CentreTrench,1);Trench(B,1,EFrontlineRole::Rifle,FrontlineBattlefield::CentreTrench,3);
   FFrontlineReviewV32::Camera(C,TrenchX,0,1800,80);return Ready;
  }
  if(Name==TEXT("figures"))
  {
   // One representative British and German rifleman, idle/aim/walk, on real
   // Arras ground beside the centre trench. Close tactical zoom.
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   const float X=FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,0)+620.f;
   const FVector Spots[4]={FVector(X-150,-60,0),FVector(X-20,90,0),FVector(X+150,-40,0),FVector(X+280,110,0)};
   Figure(B,0,EFrontlineRole::Rifle,Spots[0],-25,0);Figure(B,0,EFrontlineRole::Rifle,Spots[1],20,1);
   Figure(B,1,EFrontlineRole::Rifle,Spots[2],200,0);Figure(B,1,EFrontlineRole::Rifle,Spots[3],160,2);
   // Frame the figures on their actual ground, wherever the fixture stands.
   float Ground=0;for(const FVector& S:Spots)Ground+=AWarfareFrontlineGround::HeightAt(S.X,S.Y)*.25f;
   FFrontlineReviewV32::Camera(C,X+65,20,1200,Ground+95);return Ready;
  }
  if(Name==TEXT("poses"))
  {
   // Changed garment silhouettes: weapon, arm and kit contacts in the actual
   // action library. Aim, reload, run and grenade throw per faction, plus one
   // fatal casualty per faction (casualty body pair, wound/weapon release).
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   const float X=FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,0)+900.f;
   float Ground=0;int32 N=0;
   for(int32 Team=0;Team<2;++Team)for(int32 K=0;K<5;++K)
   {
    const FVector At(X-340+K*170,Team?120:-120,0);Ground+=AWarfareFrontlineGround::HeightAt(At.X,At.Y);++N;
    auto* U=Figure(B,Team,EFrontlineRole::Rifle,At,Team?200:-20,0);if(!U)continue;
    UAnimationAsset* Motion=K==0?U->GetAimMotion():K==1?U->GetReloadMotion():K==2?U->GetRunMotion():K==3?B->GrenadeAnimation.Get():nullptr;
    if(K<4){Freeze(U,Motion?Motion:U->GetIdleMotion(),K==3?.55f:.40f);}
    // British: rifle hit ragdoll. German: grenade, exercising the blast-loss
    // casualty body pair, wound cap and released weapon.
    else{U->SetActorTickEnabled(true);U->ReceiveHit(1,0,1-Team,FVector(Team?-1:1,0,0),Team?EFrontlineDamageKind::Grenade:EFrontlineDamageKind::Bullet,Team?.6f:.4f);}
   }
   FFrontlineReviewV32::Camera(C,X,0,1400,Ground/FMath::Max(1,N)+95);return Ready;
  }
  if(Name==TEXT("field"))
  {
   // The ordinary 6000 cm battle width: soldiers at their real play scale.
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   Trench(B,0,EFrontlineRole::Lewis,FrontlineBattlefield::CentreTrench,1);Trench(B,0,EFrontlineRole::Rifle,FrontlineBattlefield::CentreTrench,0);
   const float X=FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,0);
   Figure(B,0,EFrontlineRole::Rifle,FVector(X+700,-150,0),0,2);Figure(B,0,EFrontlineRole::Rifle,FVector(X+820,60,0),0,2);
   Figure(B,1,EFrontlineRole::Rifle,FVector(X+1500,-80,0),180,1);Figure(B,1,EFrontlineRole::Rifle,FVector(X+1650,120,0),180,0);
   FFrontlineReviewV32::Camera(C,X+500,0,6000,650);return Ready;
  }
  if(Name==TEXT("wide"))
  {
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   FFrontlineReviewV32::Camera(C,0,0,8600,650);return Ready;
  }
  if(Name==TEXT("ypres"))
  {
   const bool Ready=Battle(C,B,EFrontlineFaction::British,5);
   Trench(B,0,EFrontlineRole::Rifle,FrontlineBattlefield::CentreTrench,0);
   FFrontlineReviewV32::Camera(C,0,0,4200,650);return Ready;
  }
  if(Name==TEXT("menin"))
  {
   const bool Ready=Battle(C,B,EFrontlineFaction::British,7);
   FFrontlineReviewV32::Camera(C,0,0,6000,650);return Ready;
  }
  if(Name==TEXT("armour"))
  {
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   AFrontlineTank* Tank=FrontlineCombatRecordV65::PrepareArmourReview(B);
   FFrontlineReviewV32::Camera(C,0,0,2400,AWarfareFrontlineGround::HeightAt(0,0)+150);return Ready&&Tank!=nullptr;
  }
  if(Name==TEXT("aircraft"))
  {
   const bool Ready=Battle(C,B,EFrontlineFaction::British,0);
   FrontlineSupportArtV36::PrewarmVisuals();FrontlineSupportArtV36::SetupVisualFixture(B);
   FFrontlineReviewV32::Camera(C,0,0,5500,450);return Ready;
  }
  return false;
 }
 bool Busy() const
 {
#if WITH_EDITOR
  if(FAssetCompilingManager::Get().GetNumRemainingAssets()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()))return true;
#endif
  return PipelineStateCache::GetNumActivePipelinePrecompileTasks()>0;
 }
 bool Tick(float)
 {
  const double Now=FPlatformTime::Seconds();if(Now<Next)return true;
  auto* C=PC.Get();auto* B=C?FrontlineFor(C):nullptr;
  if(!B||!B->LoadedSoldier||!C->GetHUD()){if(Now-Started>150){Check(TEXT("startup"),false);Finish();return false;}Next=Now+.25;return true;}
  if(Now-Started>1500){Check(FString::Printf(TEXT("timeout_scene_%d_variant_%d"),Scene,Variant),false);Finish();return false;}
  if(Scene>=Scenes.Num()){Finish();return false;}
  if(Busy()){Next=Now+.25;SettleFrom=0;return true;}
  const FString& Name=Scenes[Scene];
  if(Phase==0)
  {
   if(Scene==0){Check(TEXT("disposable_profile"),B->IsProgressionVolatile());if(!B->IsProgressionVolatile()){Finish();return false;}}
   // Scene fixtures are built with the V67 bindings, then every variant is
   // applied in place. Fresh spawns and in-place rebinding share resolvers.
   ApplyVariant(FArtVariantV68{});
   Check(TEXT("scene_")+Name,SetupScene(C,B,Name));
   // Terrain tiles, texture streaming and Lumen scene capture settle once.
   Phase=1;Variant=0;Next=Now+8.0;return true;
  }
  if(Phase==1)
  {
   // Freeze simulation, particles and the weather clock for all variants.
   UGameplayStatics::SetGamePaused(C,true);ApplyVariant(Variants[Variant]);
   Phase=2;SettleFrom=0;Next=Now+.5;return true;
  }
  if(Phase==2)
  {
   // Shader compilation of a first-seen permutation must finish, then Lumen,
   // DLSS RR and virtual shadows need a stable interval on the frozen frame.
   if(SettleFrom==0){SettleFrom=Now;GpuMs=GameMs=RenderMs=0;Samples=0;}
   // Warmed indicative frame cost on the frozen frame (last 2 s of settling).
   if(Now-SettleFrom>1.5){GpuMs+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());GameMs+=FPlatformTime::ToMilliseconds(GGameThreadTime);RenderMs+=FPlatformTime::ToMilliseconds(GRenderThreadTime);++Samples;}
   if(Now-SettleFrom<3.5){Next=Now+.25;return true;}
   const FString File=FString::Printf(TEXT("%02d-%s-%s.png"),Scene,*Name,*Variants[Variant].Name);
   FScreenshotRequest::RequestScreenshot(Dir/TEXT("captures")/File,true,false);
   auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),File);J->SetStringField(TEXT("scene"),Name);J->SetStringField(TEXT("variant"),Variants[Variant].Name);
   if(Samples>0){J->SetNumberField(TEXT("gpu_ms"),GpuMs/Samples);J->SetNumberField(TEXT("game_thread_ms"),GameMs/Samples);J->SetNumberField(TEXT("render_thread_ms"),RenderMs/Samples);J->SetNumberField(TEXT("timing_samples"),Samples);}
   J->SetBoolField(TEXT("soldiers_v68"),FrontlineArtV68::SoldiersEnabled());J->SetBoolField(TEXT("terrain_v68"),FrontlineArtV68::TerrainEnabled());J->SetBoolField(TEXT("light_v68"),FrontlineArtV68::LightEnabled());
   if(auto* W=AWarfareFrontlineWeather::Get(B->GetWorld()))
   {J->SetNumberField(TEXT("sun_intensity"),W->GetSunIntensity());J->SetNumberField(TEXT("sky_intensity"),W->GetSkyIntensity());J->SetNumberField(TEXT("wetness"),W->GetWetness());J->SetNumberField(TEXT("cloud"),W->GetCloudCover());}
   TArray<TSharedPtr<FJsonValue>> Bodies;TSet<FString> Seen;
   for(const auto& U:B->Units)if(U&&U->Soldier&&U->Soldier->GetSkeletalMeshAsset())
   {
    const FString Mesh=U->Soldier->GetSkeletalMeshAsset()->GetPathName();if(Seen.Contains(Mesh))continue;Seen.Add(Mesh);
    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("mesh"),Mesh);TArray<TSharedPtr<FJsonValue>> Mats;
    for(int32 I=0;I<U->Soldier->GetNumMaterials();++I)if(auto* M=U->Soldier->GetMaterial(I))Mats.Add(MakeShared<FJsonValueString>(GetPathNameSafe(M->GetMaterial())));
    R->SetArrayField(TEXT("base_materials"),Mats);Bodies.Add(MakeShared<FJsonValueObject>(R));
   }
   J->SetArrayField(TEXT("bound_bodies"),Bodies);Captures.Add(MakeShared<FJsonValueObject>(J));
   Phase=3;Next=Now+.6;return true;
  }
  // Phase 3: capture written; continue with the next variant or scene.
  if(++Variant<Variants.Num()){Phase=1;return true;}
  UGameplayStatics::SetGamePaused(C,false);Phase=0;++Scene;Next=Now+.3;return true;
 }
};
}
void StartFrontlineV68ArtReview(AWarfareFrontlineController* C)
{
 FString U;if(!FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),U)||U.IsEmpty())return;
 auto R=MakeShared<FFrontlineArtReviewV68>();R->PC=C;R->Started=FPlatformTime::Seconds();R->Next=R->Started+5;
 R->Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V68/Review");IFileManager::Get().MakeDirectory(*(R->Dir/TEXT("captures")),true);
 R->ParseOptions();if(R->Variants.IsEmpty()||R->Scenes.IsEmpty()){UE_LOG(LogTemp,Error,TEXT("WF_V68_REVIEW_NO_WORK"));return;}
 R->View=FSceneViewExtensions::NewExtension<FArtReviewViewV68>(C->GetWorld());
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT){return R->Tick(DT);}),0.f);
}
