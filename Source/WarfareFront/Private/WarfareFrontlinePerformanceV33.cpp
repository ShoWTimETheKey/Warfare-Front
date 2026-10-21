#include "WarfareFrontlinePerformanceV33.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInterface.h"
#include "RenderTimer.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FV33Samples
{
    TArray<double> Wall,GPU,Game,Render,RHI;
    int32 MinLiving=MAX_int32,MaxLiving=0,MinArmour=MAX_int32,MaxFire=0,Unfocused=0,MaxWreckSourcesV838=0,MaxWreckVolumesV838=0;
    double Seconds=0;
    FV33Samples(){Wall.Reserve(6000);GPU.Reserve(6000);Game.Reserve(6000);Render.Reserve(6000);RHI.Reserve(6000);}
};
struct FV33PerfRun
{
    double Started=FPlatformTime::Seconds(),WarmUntil=0,SampleStart=0,Last=0,ProfileUntil=0;
    float GameStart=0,GameEnd=0;
    int32 Stage=0,InitialUnits=0,ExpectedUnits=0,Operators=0,InitialArmour=0,ExpectedArmour=4,Width=0,Height=0,InitialVisible=0,ShotsAtStart=0;
    bool Ready=true,ConfigurationValid=true,SimulationActive=true;
    bool ProfileRequested=false,ProfileStarted=false,ProfileAvailable=false;
    int32 ShotsAtEnd=0;
    int32 InitialWrecksV838=0;
    bool WreckStressV838=false,WarmCaptureV838=false;
    FString Directory,Failure;
    FV33Samples All,Fixed,Pan;
    TSharedPtr<FJsonObject> Before,InitialField;
    TSet<FString> MeshPaths;
    TSet<FString> FireMaterialPaths;
    TWeakObjectPtr<AFrontlineFuelFireV33> Fire;
};
TSharedRef<FJsonObject> Distribution(TArray<double> Values)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());
    if(Values.IsEmpty()){J->SetBoolField(TEXT("available"),false);return J;}
    double Total=0;for(double V:Values)Total+=V;Values.Sort();
    J->SetBoolField(TEXT("available"),true);J->SetNumberField(TEXT("mean_ms"),Total/Values.Num());
    J->SetNumberField(TEXT("p50_ms"),Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*.50)-1,0,Values.Num()-1)]);
    J->SetNumberField(TEXT("p95_ms"),Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*.95)-1,0,Values.Num()-1)]);
    J->SetNumberField(TEXT("max_ms"),Values.Last());return J;
}
TSharedRef<FJsonObject> Summarize(const FV33Samples& S)
{
    auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Distribution(S.Wall));J->SetObjectField(TEXT("gpu"),Distribution(S.GPU));
    J->SetNumberField(TEXT("wreck_sources_max_v838"),S.MaxWreckSourcesV838);J->SetNumberField(TEXT("wreck_volumes_max_v838"),S.MaxWreckVolumesV838);
    J->SetObjectField(TEXT("game"),Distribution(S.Game));J->SetObjectField(TEXT("render"),Distribution(S.Render));J->SetObjectField(TEXT("rhi"),Distribution(S.RHI));
    J->SetNumberField(TEXT("sample_seconds"),S.Seconds);J->SetNumberField(TEXT("render_fps"),S.Seconds>0?S.Wall.Num()/S.Seconds:0);
    J->SetNumberField(TEXT("living_min"),S.MinLiving==MAX_int32?0:S.MinLiving);J->SetNumberField(TEXT("living_max"),S.MaxLiving);
    J->SetNumberField(TEXT("operational_armour_min"),S.MinArmour==MAX_int32?0:S.MinArmour);J->SetNumberField(TEXT("fire_particles_max"),S.MaxFire);
    J->SetNumberField(TEXT("unfocused_frames"),S.Unfocused);return J;
}
void CollectMeshes(AActor* A,TSet<FString>& Paths,TSet<FString>* Materials=nullptr)
{
    if(!A)return;
    TInlineComponentArray<UStaticMeshComponent*> Static(A);for(auto* C:Static)
    {
        if(C->GetStaticMesh())Paths.Add(C->GetStaticMesh()->GetPathName());
        if(Materials)for(int32 I=0;I<C->GetNumMaterials();++I)if(auto* M=C->GetMaterial(I))Materials->Add(M->GetPathName());
    }
    TInlineComponentArray<USkeletalMeshComponent*> Skeletal(A);for(auto* C:Skeletal)if(C->GetSkeletalMeshAsset())Paths.Add(C->GetSkeletalMeshAsset()->GetPathName());
}
void Camera(AWarfareFrontlineController* PC,double Seconds)
{
    if(!PC||!PC->BattleCamera)return;
    const float A=FMath::Clamp(float((Seconds-10.)/8.),0.f,1.f);
    const float X=900.f*A*A*(3.f-2.f);
    const FVector Focus(X,0,650),From(X,14000,4650);
    PC->BattleCamera->SetActorLocationAndRotation(From,(Focus-From).Rotation());
    PC->BattleCamera->GetCameraComponent()->SetOrthoWidth(6000.f);
    // Restart selected the operations camera before BeginBattle. Select the
    // actual battlefield view, then refresh it even during paused warm-up.
    PC->SetViewTarget(PC->BattleCamera);
    if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);
}
bool RenderContract(UFrontlineRenderSettingsSubsystem* R)
{
    if(!R)return false;const auto D=R->GetDiagnostics();
    return R->GetQuality()==EFrontlineRenderQuality::Quality && R->GetFrameGeneration()==EFrontlineFrameGeneration::Off
        && R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off && R->GetRayReconstruction()
        && D->GetBoolField(TEXT("dlss_enabled_api")) && D->GetBoolField(TEXT("rr_enabled_api"))
        && D->GetBoolField(TEXT("observed_orthographic")) && !R->GetHDRStatus().bOutputActive;
}
TSharedRef<FJsonObject> CostSettings()
{
    auto J=MakeShared<FJsonObject>();
    // A fixed public rendering whitelist, never an arbitrary config dump.
    const TCHAR* Names[]={TEXT("r.Lumen.HardwareRayTracing.LightingMode"),TEXT("r.Lumen.Reflections.MaxBounces"),
        TEXT("r.Lumen.Reflections.DownsampleFactor"),TEXT("r.Lumen.Reflections.AsyncCompute"),TEXT("r.Lumen.AsyncCompute"),
        TEXT("r.Lumen.Reflections.MaxRoughnessToTrace"),TEXT("r.Lumen.SceneLightingQuality"),TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"),
        TEXT("r.Lumen.ScreenProbeGather.TracingOctahedronResolution"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"),
        TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"),TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayDirectional"),
        TEXT("r.VolumetricFog.GridPixelSize"),TEXT("r.VolumetricFog.GridSizeZ"),TEXT("r.RayTracing.Culling"),
        TEXT("r.RayTracing.Geometry.SkeletalMeshes"),TEXT("r.RayTracing.Geometry.InstancedStaticMeshes.Culling"),TEXT("r.RayTracing.UseTextureLod"),
        TEXT("r.SkinCache.Mode"),TEXT("r.SkinCache.AsyncCompute"),TEXT("r.SkinCache.RecomputeTangents"),
        TEXT("r.SkinCache.RecomputeTangentsParallelDispatch"),TEXT("r.SkinCache.DefaultBehavior"),
        TEXT("r.Nanite"),TEXT("r.ScreenPercentage"),TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("t.IdleWhenNotForeground")};
    for(const TCHAR* Name:Names)if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))J->SetStringField(Name,C->GetString());
    return J;
}
}

struct FFrontlinePerformanceV33Access
{
    static void Start(AWarfareFrontlineGameMode* B)
    {
        if(!B||!FParse::Param(FCommandLine::Get(),TEXT("WFV33Benchmark")))return;
        FString UserDir;
        if(!B->bProgressionVolatile||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()
            ||FParse::Param(FCommandLine::Get(),TEXT("WFV33Review")))
        {UE_LOG(LogTemp,Error,TEXT("WF_V33_PERF rejected: requires standalone volatile archive and explicit UserDir"));FPlatformMisc::RequestExit(false);return;}
        auto Run=MakeShared<FV33PerfRun>();
        Run->ProfileRequested=FParse::Param(FCommandLine::Get(),TEXT("WFV33GPUProfile"));
        if(!FParse::Value(FCommandLine::Get(),TEXT("WFV33PerformanceOut="),Run->Directory))
            Run->Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V33/Performance")/FGuid::NewGuid().ToString();
        Run->Directory=FPaths::ConvertRelativePathToFull(Run->Directory);IFileManager::Get().MakeDirectory(*Run->Directory,true);
        TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(B);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
        {
            auto* Battle=Weak.Get();if(!Battle){FPlatformMisc::RequestExit(false);return false;}
            auto* PC=Cast<AWarfareFrontlineController>(Battle->GetWorld()->GetFirstPlayerController());
            auto* R=UFrontlineRenderSettingsSubsystem::Get(Battle->GetWorld());auto* V=UFrontlineVehicleSubsystem::Get(Battle->GetWorld());
            const double Now=FPlatformTime::Seconds();
            if(!Battle->AssetFailure.IsEmpty()||Now-Run->Started>65)
            {Run->Failure=Battle->AssetFailure.IsEmpty()?TEXT("65 second self-exit timeout"):Battle->AssetFailure;Run->Ready=false;Run->Stage=3;}
            if(Run->Stage==0 && Now-Run->Started>=3 && PC && PC->BattleCamera && R && V)
            {
                Battle->AbandonBattleToOperations();FFrontlineCampaignProgress Campaign;
                Campaign.Id=FGuid::NewGuid();Campaign.Faction=EFrontlineFaction::British;Campaign.Difficulty=EFrontlineDifficulty::Veteran;
                const bool RailSample=FParse::Param(FCommandLine::Get(),TEXT("WFV837RailBenchmark"));
                Campaign.CompletedMissions=RailSample?19:9;Campaign.SelectedMission=RailSample?17:8;Campaign.Research.TechnologyMask=(1u<<24)-1;
                if(RailSample){Campaign.WonOperationMaskV52=(1u<<19)-1;Campaign.DispatchResolvedMaskV52=(1u<<20)-1;}
                Campaign.RoleSelectionMask=0x7f;Campaign.SupportSelectionMask=0x3ff;Campaign.AcknowledgedEventMask=0xffffffffu;
                Battle->CampaignRuns={Campaign};Battle->ActiveCampaignId=Campaign.Id;Battle->PlayerFaction=Campaign.Faction;
                PC->Restart();Battle->BeginBattle();Battle->SetBattleSpeed(1.f);
                if(Battle->Phase!=EFrontlinePhase::Fighting){Run->Failure=TEXT("isolated live-combat deployment failed");Run->Ready=false;Run->Stage=3;}
                else
                {
                    const auto* Mission=Battle->GetEncounterCampaignMission();
                    Run->ExpectedArmour=Mission&&Mission->bArmoredTrain?5:4;
                    R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetQuality(EFrontlineRenderQuality::Quality,false);
                    R->SetRayReconstruction(true,false);R->SetHDREnabled(false,false);R->SetHDRCalibrationVisible(false);
                    // Preserve the production monitor-based cap, and report it. No fake uncapped claim.
                    R->SetFrameRateSlider(1.f,false);if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))C->Set(0,ECVF_SetByCode);
                    const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Rifle,EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Flamethrower};
                    for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<8;++I)
                    {
                        Run->ExpectedUnits+=Battle->GetSquadSize(Roles[I],Team);
                        if(!Battle->SpawnSquad(Team,Roles[I],INDEX_NONE)){Run->Ready=false;continue;}
                        auto& S=Battle->Squads.Last();S.bAdvancing=true;S.bReinforcement=false;S.Trench=INDEX_NONE;
                        S.Destination=2;S.LastPassedTrench=Team?3:1;S.Slot=I%3;
                        int32 M=0;for(const auto& Ref:S.Members)if(auto* U=Ref.Get())
                        {
                            const float Sign=Team?1.f:-1.f;
                            const float X=Sign*(I==7?550.f:1250.f+(I%3)*440.f+M*46.f);
                            const float Y=I==7?480.f+M*72.f:-850.f+(I/3)*470.f+M*95.f;
                            U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88));U->bFieldEntered=true;U->ReturnToIdle();++M;
                            Run->Operators+=FrontlineFieldV32::IsFlameOperator(U);CollectMeshes(U,Run->MeshPaths);
                        }
                    }
                    // Gas exposure follows the normal issue and donning paths;
                    // this read does not force an immediately sealed mask.
                    FFrontlineSmokeZone Zone;Zone.bGas=true;Zone.Centre=FVector(0,-500,0);Zone.Radius=2700;Zone.ExpiresAt=60;
                    Battle->SmokeZones.Add(Zone);for(AWarfareFrontlineSoldier* U:Battle->Units)if(U)FrontlineFieldV32::GasHitScale(U);
                    V->ResetBattle();V->LastEncounter=Battle->GetEncounterId();
                    for(int32 I=0;I<4;++I)if(auto* Tank=V->Spawn(EFrontlineVehicle(I),I<2?0:1,false))
                    {
                        const float X=(I<2?-1.f:1.f)*(I%2?2450.f:1850.f),Y=I%2?720.f:-600.f;
                        Tank->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)));CollectMeshes(Tank,Run->MeshPaths);
                        Run->Ready&=Tank->AreAssetsReady();++Run->InitialArmour;
                    }
                    if(Mission&&Mission->bArmoredTrain)if(auto* Train=V->SpawnScenarioRailV32(1)){Train->SetActorLocation(FVector(0,-1130,28));CollectMeshes(Train,Run->MeshPaths);Run->Ready&=Train->AreAssetsReady();++Run->InitialArmour;}
                    Run->WreckStressV838=FParse::Param(FCommandLine::Get(),TEXT("WFV838WreckBenchmark"));
                    if(Run->WreckStressV838)for(int32 I=0;I<2;++I)if(auto* Wreck=V->Spawn(I?EFrontlineVehicle::Austin1918:EFrontlineVehicle::Whippet,0,false))
                    {
                        const float X=I?650.f:-650.f,Y=I?800.f:250.f;
                        Wreck->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)));
                        Wreck->ReceiveModuleHit(EFrontlineTankModule::Hull,Wreck->GetMaximumHullIntegrity()+1);
                        CollectMeshes(Wreck,Run->MeshPaths);++Run->InitialWrecksV838;
                    }
                    PC->CancelTargeting();PC->SetActorTickEnabled(false);Camera(PC,0);PC->GetViewportSize(Run->Width,Run->Height);
                    if(auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD()))if(HUD->GetPresentationPage()!=3)
                    {Run->Failure=TEXT("benchmark battlefield HUD was not selected after Restart cleared overlays");Run->Ready=false;}
                    auto* Fire=AFrontlineFuelFireV33::Get(Battle->GetWorld());Run->Fire=Fire;Run->Ready&=Fire && Fire->IsReady();CollectMeshes(Fire,Run->MeshPaths,&Run->FireMaterialPaths);
                    Run->InitialUnits=Battle->Units.Num();Run->InitialField=FrontlineFieldV32::Review(Battle);
                    Run->Ready&=Run->InitialUnits==Run->ExpectedUnits && Run->InitialUnits>=58 && Run->InitialUnits<=68 && Run->Operators==2 && Run->InitialArmour==Run->ExpectedArmour;
                    Run->Ready&=Run->Width==3840 && Run->Height==2160;
                    for(AWarfareFrontlineSoldier* U:Battle->Units)if(U){FVector2D P;if(PC->ProjectWorldLocationToScreen(U->GetActorLocation(),P)&&P.X>=0&&P.X<Run->Width&&P.Y>=0&&P.Y<Run->Height)++Run->InitialVisible;}
                    Battle->SetPaused(true);Run->WarmUntil=FPlatformTime::Seconds()+5.;Run->Stage=1;
                    UE_LOG(LogTemp,Display,TEXT("WF_V33_PERF fixture soldiers=%d operators=%d armour=%d visible_centres=%d warm=5 live_sample=18"),Run->InitialUnits,Run->Operators,Run->InitialArmour,Run->InitialVisible);
                }
            }
            else if(Run->Stage==1 && Now>=Run->WarmUntil)
            {
                if(Run->WreckStressV838&&!Run->WarmCaptureV838)
                {FScreenshotRequest::RequestScreenshot(Run->Directory/TEXT("warmed-wreck-scene-v838.png"),true,false);Run->WarmCaptureV838=true;Run->WarmUntil=Now+.4;return true;}
                if(FScreenshotRequest::IsScreenshotRequested())return true;
                Run->Before=R->GetDiagnostics();Run->ConfigurationValid=RenderContract(R);
                // Complete resource inventory only outside the measurement window.
                for(AWarfareFrontlineSoldier* U:Battle->Units)CollectMeshes(U,Run->MeshPaths);
                // Projection uses the camera manager's last completed frame;
                // the initial setup frame still contains the operations camera.
                // Paused warm-up still renders, but does not tick its manager.
                // Refresh that same real camera before projecting the sample.
                if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);
                Run->InitialVisible=0;for(AWarfareFrontlineSoldier* U:Battle->Units)if(U)
                {
                    if(Run->WreckStressV838&&PC->BattleCamera->GetCameraComponent()->ProjectionMode==ECameraProjectionMode::Orthographic)
                    {
                        const auto* CameraComponent=PC->BattleCamera->GetCameraComponent();
                        const FVector P=CameraComponent->GetComponentTransform().InverseTransformPosition(U->GetActorLocation());
                        const float Width=CameraComponent->OrthoWidth,Height=Width*Run->Height/FMath::Max(1,Run->Width);
                        Run->InitialVisible+=P.X>0&&FMath::Abs(P.Y)<=Width*.5f&&FMath::Abs(P.Z)<=Height*.5f;
                    }
                    else{FVector2D P;if(PC->ProjectWorldLocationToScreen(U->GetActorLocation(),P)&&P.X>=0&&P.X<Run->Width&&P.Y>=0&&P.Y<Run->Height)++Run->InitialVisible;}
                }
                Run->ShotsAtStart=Battle->ShotsFired;Run->GameStart=Battle->Elapsed;Battle->SetPaused(false);
                // Authored contact order enters the production finite-fuel, hit, death and fire-pool paths.
                AWarfareFrontlineSoldier* Operators[2]={};for(AWarfareFrontlineSoldier* U:Battle->Units)if(FrontlineFieldV32::IsFlameOperator(U))Operators[U->Team]=U;
                if(Operators[0]&&Operators[1]){FrontlineFieldV32::FlameDischarge(Operators[0],Operators[1]);FrontlineFieldV32::FlameDischarge(Operators[1],Operators[0]);}
                Run->SampleStart=Run->Last=FPlatformTime::Seconds();Run->Stage=2;
            }
            else if(Run->Stage==2)
            {
                const double Elapsed=Now-Run->SampleStart,Ms=(Now-Run->Last)*1000.;Run->Last=Now;
                Camera(PC,Elapsed);int32 Living=0,Armour=0;for(AWarfareFrontlineSoldier* U:Battle->Units)Living+=U&&U->IsActiveCombatant();
                if(V)for(AFrontlineTank* Tank:V->GetVehicles())Armour+=Tank&&Tank->IsOperational();
                auto* Fire=Run->Fire.Get();const int32 Particles=Fire?Fire->ActiveCount():0;
                const auto Visual=Fire?Fire->VisualStateV8214():FFrontlineFuelVisualStatsV8214();
                const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
                const double Game=FPlatformTime::ToMilliseconds(GGameThreadTime),Render=FPlatformTime::ToMilliseconds(GRenderThreadTime),RHI=FPlatformTime::ToMilliseconds(GRHIThreadTime);
                auto Record=[&](FV33Samples& S){S.Wall.Add(Ms);if(GPU>0)S.GPU.Add(GPU);if(Game>0)S.Game.Add(Game);if(Render>0)S.Render.Add(Render);if(RHI>0)S.RHI.Add(RHI);
                    S.Seconds+=Ms/1000.;S.MinLiving=FMath::Min(S.MinLiving,Living);S.MaxLiving=FMath::Max(S.MaxLiving,Living);S.MinArmour=FMath::Min(S.MinArmour,Armour);S.MaxFire=FMath::Max(S.MaxFire,Particles);S.Unfocused+=!FApp::HasFocus();};
                Run->All.MaxWreckSourcesV838=FMath::Max(Run->All.MaxWreckSourcesV838,Visual.WreckSmokeSourcesV838);Run->All.MaxWreckVolumesV838=FMath::Max(Run->All.MaxWreckVolumesV838,Visual.WreckSmokeVolumesV838);
                Record(Run->All);Record(Elapsed<=10?Run->Fixed:Run->Pan);
                Run->SimulationActive&=!UGameplayStatics::IsGamePaused(Battle) && Battle->Phase==EFrontlinePhase::Fighting;
                if(Elapsed>=18.){Run->GameEnd=Battle->Elapsed;Run->ShotsAtEnd=Battle->ShotsFired;Run->Stage=3;}
            }
            if(Run->Stage==3 && Run->ProfileRequested && !Run->ProfileStarted && !Run->All.Wall.IsEmpty())
            {
                // ProfileGPU changes instrumentation and logs a detailed GPU
                // tree. Invoke strictly AFTER the timed run, never mid-sample.
                Run->ProfileStarted=true;Run->ProfileAvailable=IConsoleManager::Get().FindConsoleObject(TEXT("ProfileGPU"))!=nullptr;
                if(Run->ProfileAvailable && PC)
                {
                    const TCHAR* Disabled[]={TEXT("r.ProfileGPU.ShowUI"),TEXT("r.ProfileGPU.UnicodeOutput")};
                    for(const TCHAR* Name:Disabled)if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))C->Set(0,ECVF_SetByCode);
                    if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ProfileGPU.Sort")))C->Set(1,ECVF_SetByCode);
                    if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ProfileGPU.ThresholdPercent")))C->Set(.15f,ECVF_SetByCode);
                    UE_LOG(LogTemp,Display,TEXT("WF_V33_GPU_PROFILE after_timed_sample living=%d armour=%d"),Battle->LivingSoldiers(0)+Battle->LivingSoldiers(1),V?V->GetVehicles().Num():0);
                    PC->ConsoleCommand(TEXT("ProfileGPU"),true);Run->ProfileUntil=Now+3.;Run->Stage=4;
                }
                else UE_LOG(LogTemp,Warning,TEXT("WF_V33_GPU_PROFILE unavailable in this build; FPS samples remain valid"));
            }
            if(Run->Stage==4){if(Now<Run->ProfileUntil)return true;Run->Stage=3;}
            if(Run->Stage!=3)return true;
            auto J=MakeShared<FJsonObject>();auto After=R?R->GetDiagnostics():MakeShared<FJsonObject>();
            Run->ConfigurationValid&=RenderContract(R);const bool Complete=Run->All.Seconds>=18 && Run->All.Wall.Num()>100;
            const bool Live=Run->SimulationActive && Run->GameEnd-Run->GameStart>=15 && Run->ShotsAtEnd>Run->ShotsAtStart;
            const bool FireCovered=Run->All.MaxFire>0;
            const bool Success=Run->Ready && Complete && Live && FireCovered && Run->ConfigurationValid && Run->All.GPU.Num()>Run->All.Wall.Num()*.8;
            J->SetNumberField(TEXT("version"),33);J->SetBoolField(TEXT("success"),Success);J->SetStringField(TEXT("failure"),Run->Failure);
            J->SetBoolField(TEXT("foreground_entire_sample"),Run->All.Wall.Num()>0 && Run->All.Unfocused==0);
            J->SetStringField(TEXT("focus_scope"),Run->All.Unfocused?TEXT("Contains unfocused-window frames; these are real background-window measurements and must not be described as foreground play."):TEXT("Application foreground for every measured interval."));
            J->SetBoolField(TEXT("gpu_profile_requested"),Run->ProfileRequested);J->SetBoolField(TEXT("gpu_profile_command_available"),Run->ProfileAvailable);
            J->SetStringField(TEXT("gpu_profile_scope"),TEXT("Optional native UE ProfileGPU pass tree in runtime.log, requested only after all 18 seconds of FPS samples; diagnostic frame is not included in timing distributions."));
            J->SetObjectField(TEXT("render_cost_cvars"),CostSettings());
            J->SetStringField(TEXT("scope"),TEXT("Authored live mixed-armour stress encounter; 5 s paused renderer warm-up, then 18 s real combat, 10 s fixed normal view and 8 s short pan. Natural deaths and AI retained. No screenshot/readback during sampling."));
            J->SetStringField(TEXT("timing_method"),TEXT("Core-ticker wall intervals and Unreal RHI GPU/game/render/RHI thread counters. Thread counters are delayed and overlap; they must not be summed. FG is off; render_fps counts wall intervals, not generated or estimated presented frames."));
            J->SetStringField(TEXT("limit_method"),TEXT("Production cap at the current monitor's detected maximum slider value, VSync off. Cap reported in renderer diagnostics; results near it may be cap-limited."));
            J->SetStringField(TEXT("fixture_method"),Run->ExpectedArmour==5
                ?TEXT("Prepositioned production squads, four production tank models and one assigned rail convoy. Gas exposure follows normal researched equipment and donning animations; masks are not forced on at setup. Two finite-fuel opening flame orders use normal combat paths. No health, hit chance, AI budget or animation overrides.")
                :TEXT("Prepositioned production squads and four production tank models in a non-rail operation; no rail convoy is assigned. Gas exposure follows normal researched equipment and donning animations; masks are not forced on at setup. Two finite-fuel opening flame orders use normal combat paths. No health, hit chance, AI budget or animation overrides."));
            J->SetNumberField(TEXT("width"),Run->Width);J->SetNumberField(TEXT("height"),Run->Height);J->SetNumberField(TEXT("camera_ortho_width_cm"),6000);
            J->SetBoolField(TEXT("actual_view_target_is_battle_camera"),PC&&PC->BattleCamera&&PC->GetViewTarget()==PC->BattleCamera);
            J->SetNumberField(TEXT("camera_pan_cm"),900);J->SetNumberField(TEXT("initial_soldiers"),Run->InitialUnits);J->SetNumberField(TEXT("initial_visible_centres"),Run->InitialVisible);
            J->SetNumberField(TEXT("flame_operators"),Run->Operators);J->SetNumberField(TEXT("initial_armour_including_train"),Run->InitialArmour);
            J->SetBoolField(TEXT("wreck_stress_v838"),Run->WreckStressV838);J->SetNumberField(TEXT("initial_additional_wrecks_v838"),Run->InitialWrecksV838);
            if(Run->WreckStressV838)J->SetStringField(TEXT("initial_visible_centres_method_v838"),TEXT("Actual orthographic camera transform/width and viewport aspect; geometric forward frustum centres, not occlusion or per-pixel visibility. Warm-up capture precedes live sampling; additional wreck soot grows during the live sample."));
            J->SetNumberField(TEXT("expected_initial_armour"),Run->ExpectedArmour);J->SetBoolField(TEXT("rail_fixture_expected"),Run->ExpectedArmour==5);
            J->SetNumberField(TEXT("game_seconds_advanced"),Run->GameEnd-Run->GameStart);J->SetNumberField(TEXT("shots_during_sample"),Run->ShotsAtEnd-Run->ShotsAtStart);
            J->SetBoolField(TEXT("live_simulation"),Live);J->SetBoolField(TEXT("fire_covered"),FireCovered);J->SetBoolField(TEXT("render_contract"),Run->ConfigurationValid);
            J->SetObjectField(TEXT("all"),Summarize(Run->All));J->SetObjectField(TEXT("fixed"),Summarize(Run->Fixed));J->SetObjectField(TEXT("pan"),Summarize(Run->Pan));
            J->SetBoolField(TEXT("mean_render_fps_at_least_90"),Run->All.Seconds>0 && Run->All.Wall.Num()/Run->All.Seconds>=90);
            J->SetBoolField(TEXT("p95_frame_time_at_most_11_11_ms"),!Run->All.Wall.IsEmpty() && Distribution(Run->All.Wall)->GetNumberField(TEXT("p95_ms"))<=1000./90.);
            if(Run->Before)J->SetObjectField(TEXT("renderer_before"),Run->Before);J->SetObjectField(TEXT("renderer_after"),After);
            if(Run->InitialField)J->SetObjectField(TEXT("field_before"),Run->InitialField);J->SetObjectField(TEXT("field_after"),FrontlineFieldV32::Review(Battle));
            auto Budget=MakeShared<FJsonObject>();Budget->SetNumberField(TEXT("flame"),AFrontlineFuelFireV33::FlameBudget);Budget->SetNumberField(TEXT("smoke"),AFrontlineFuelFireV33::SmokeBudget);Budget->SetNumberField(TEXT("lights"),AFrontlineFuelFireV33::LightBudget);J->SetObjectField(TEXT("fire_pool_budget"),Budget);
            TArray<FString> Sorted=Run->MeshPaths.Array();Sorted.Sort();TArray<TSharedPtr<FJsonValue>> Paths;for(const auto& P:Sorted)Paths.Add(MakeShared<FJsonValueString>(P));J->SetArrayField(TEXT("actual_component_mesh_paths"),Paths);
            Sorted=Run->FireMaterialPaths.Array();Sorted.Sort();Paths.Reset();for(const auto& P:Sorted)Paths.Add(MakeShared<FJsonValueString>(P));J->SetArrayField(TEXT("actual_fire_material_paths"),Paths);
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            const bool Written=FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("performance.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V33_PERF complete valid=%d saved=%d frames=%d fps=%.2f report=%s"),Success,Written,Run->All.Wall.Num(),Run->All.Seconds>0?Run->All.Wall.Num()/Run->All.Seconds:0,*(Run->Directory/TEXT("performance.json")));
            FPlatformMisc::RequestExit(false);return false;
        }));
    }
};
void StartFrontlineV33Performance(AWarfareFrontlineGameMode* Battle){FFrontlinePerformanceV33Access::Start(Battle);}
