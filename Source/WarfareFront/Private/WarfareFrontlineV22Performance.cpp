#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AWarfareFrontlineGameMode::BeginV22PerformanceReview()
{
    if(!bProgressionVolatile||!FParse::Param(FCommandLine::Get(),TEXT("WFV22Benchmark")))return;
    struct FSample
    {
        TArray<double> Wall,GPU,Game,Render,RHI;
        int32 MinLiving=MAX_int32,MaxLiving=0;
        double MinCameraX=MAX_dbl,MaxCameraX=-MAX_dbl,MinCameraWidth=MAX_dbl,MaxCameraWidth=0;
        bool Completed=false;
    };
    struct FRun
    {
        int32 Stage=0,Active=INDEX_NONE,Action=0,InitialUnits=0,ExpectedUnits=0,InitialVisibleUnits=0,Width=0,Height=0;
        double Started=FPlatformTime::Seconds(),Next=Started+3,Last=Started,From=0,Until=0,BattleStart=0;
        FSample Samples[2];FString Directory;
        bool Ready=true;TArray<TSharedPtr<FJsonValue>> Images,Events;
        FString Failure;
        TSharedPtr<FJsonObject> SteadyDiagnostics,BattleDiagnostics;
    };
    auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V22/Performance")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* B=Weak.Get();if(!B)return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());
        auto* Rendering=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        const double Now=FPlatformTime::Seconds();
        if(Run->Active>=0&&Run->Last>=Run->From&&Now<=Run->Until)
        {
            auto& S=Run->Samples[Run->Active];
            S.Wall.Add((Now-Run->Last)*1000);
            const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if(GPU>0)S.GPU.Add(GPU);
            S.Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
            S.Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
            S.RHI.Add(FPlatformTime::ToMilliseconds(GRHIThreadTime));
            const int32 Living=B->LivingSoldiers(0)+B->LivingSoldiers(1);
            S.MinLiving=FMath::Min(S.MinLiving,Living);S.MaxLiving=FMath::Max(S.MaxLiving,Living);
            if(PC&&PC->BattleCamera)
            {
                const double X=PC->BattleCamera->GetActorLocation().X,W=PC->BattleCamera->GetCameraComponent()->OrthoWidth;
                S.MinCameraX=FMath::Min(S.MinCameraX,X);S.MaxCameraX=FMath::Max(S.MaxCameraX,X);
                S.MinCameraWidth=FMath::Min(S.MinCameraWidth,W);S.MaxCameraWidth=FMath::Max(S.MaxCameraWidth,W);
            }
        }
        Run->Last=Now;
        if(Run->Active==1&&B->Phase==EFrontlinePhase::Fighting)
        {
            const double T=Now-Run->BattleStart;
            const double Times[]={3,7,10,15};
            if(Run->Action<4&&T>=Times[Run->Action])
            {
                const int32 Action=Run->Action++;
                auto Record=[&](const TCHAR* Name,int32 Team,const FVector& Target,bool Accepted)
                {
                    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("action"),Name);J->SetNumberField(TEXT("team"),Team);
                    J->SetNumberField(TEXT("battle_wall_seconds"),T);J->SetNumberField(TEXT("battle_game_seconds"),B->Elapsed);
                    J->SetNumberField(TEXT("target_x"),Target.X);J->SetNumberField(TEXT("target_y"),Target.Y);J->SetBoolField(TEXT("accepted"),Accepted);
                    Run->Events.Add(MakeShared<FJsonValueObject>(J));
                };
                if(Action==0){Record(TEXT("artillery"),0,FVector(-1200,0,0),B->RequestArtillery(FVector(-1200,0,0),0));Record(TEXT("artillery"),1,FVector(-3000,0,0),B->RequestArtillery(FVector(-3000,0,0),1));}
                if(Action==1){Record(TEXT("smoke"),0,FVector(-1800,300,0),B->RequestSmoke(FVector(-1800,300,0),0));Record(TEXT("smoke"),1,FVector(-2700,-300,0),B->RequestSmoke(FVector(-2700,-300,0),1));}
                if(Action==2){Record(TEXT("mortar"),0,FVector(-1000,0,0),B->RequestMortar(FVector(-1000,0,0),0));Record(TEXT("mortar"),1,FVector(-3300,0,0),B->RequestMortar(FVector(-3300,0,0),1));}
                if(Action==3){Record(TEXT("advance"),0,FVector(B->Trenches[0].X,0,0),B->AdvanceTrench(0,0));Record(TEXT("advance"),1,FVector(B->Trenches[4].X,0,0),B->AdvanceTrench(4,1));}
            }
        }
        if(Now<Run->Next)return true;Run->Next=Now+.25;
        if(!B->AssetFailure.IsEmpty()||Now-Run->Started>100)
        {Run->Failure=B->AssetFailure.IsEmpty()?TEXT("Required dependency or stage did not complete within 100 wall seconds"):B->AssetFailure;UE_LOG(LogTemp,Error,TEXT("V22_PERF_RESOURCES_OR_TIMEOUT %s"),*Run->Failure);Run->Ready=false;Run->Active=INDEX_NONE;Run->Stage=99;}
        // A missing dependency must not prevent the timeout report and exit.
        if((!PC||!HUD||!Weather||!Rendering)&&Run->Stage!=99)return true;
        auto Command=[&](const TCHAR* Name){HUD->NotifyHitBoxClick(FName(Name));};
        auto Snap=[&](const TCHAR* Name)
        {
            const FString File=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(File,true,false);
            Run->Images.Add(MakeShared<FJsonValueString>(File));
        };
        switch(Run->Stage++)
        {
        case 0:PC->Restart();Command(TEXT("NewCampaign"));break;
        case 1:Command(TEXT("FactionBritish"));break;
        case 2:Command(TEXT("Begin"));break;
        case 3:
        {
            if(B->Phase!=EFrontlinePhase::Fighting){--Run->Stage;break;}
            B->ResetEncounter();B->BeginBattle();B->SetBattleSpeed(1);
            const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer,EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills};
            B->Trenches[1].Owner=0;B->Trenches[2].Owner=1;
            for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<8;++I)
            {
                const int32 T=I<3?(Team?2:1):(I<6?(Team?4:0):INDEX_NONE);
                Run->ExpectedUnits+=B->GetSquadSize(Roles[I],Team);
                const int32 SquadsBefore=B->Squads.Num();
                if(!B->SpawnSquad(Team,Roles[I],T)||B->Squads.Num()!=SquadsBefore+1)
                {Run->Ready=false;Run->Failure=TEXT("A requested fixture squad did not spawn completely");continue;}
                auto& S=B->Squads.Last();
                Run->Ready&=S.LivingCount()==B->GetSquadSize(Roles[I],Team);
                if(T==INDEX_NONE)
                {
                    S.bAdvancing=true;S.bReinforcement=false;S.Trench=INDEX_NONE;S.Destination=Team?1:2;
                    S.LastPassedTrench=Team?2:1;S.Slot=I-6;
                }
                int32 M=0;
                for(const auto& Ref:S.Members)if(auto* U=Ref.Get())
                {
                    FVector At=T==INDEX_NONE?FVector(Team?-1200:-2800,(I-6)*480-380+M*65,0):B->SoldierDestination(U);
                    At.Z=AWarfareFrontlineGround::HeightAt(At.X,At.Y)+88;
                    U->SetActorLocation(At);U->ReturnToIdle();U->SetActorTickEnabled(false);++M;
                }
            }
            B->SetActorTickEnabled(false);PC->SelectTrench(1);
            Weather->SetMode(EFrontlineWeatherMode::Rain,false);Weather->SnapToWeatherForReview();
            for(int32 Team=0;Team<2;++Team){B->ArtilleryReadyAt[Team]=0;B->SmokeReadyAt[Team]=0;B->MortarReadyAt[Team]=0;}
            Run->InitialUnits=B->Units.Num();PC->GetViewportSize(Run->Width,Run->Height);
            Run->Ready&=Run->InitialUnits==Run->ExpectedUnits&&Run->ExpectedUnits==60&&Run->Width==3840&&Run->Height==2160;
            Run->Active=0;Run->From=Now+8;Run->Until=Run->From+12;
            UE_LOG(LogTemp,Display,TEXT("V22_PERF_STEADY_SETUP soldiers=%d viewport=%dx%d warmup=8 sample=12"),Run->InitialUnits,Run->Width,Run->Height);
            break;
        }
        case 4:
            if(Now<Run->Until){--Run->Stage;break;}
            Run->Samples[0].Completed=true;
            // Count projected living centres after camera settling, outside timed samples.
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->IsAlive())
            {
                FVector2D Pixel;
                if(PC->ProjectWorldLocationToScreen(U->GetActorLocation(),Pixel)&&Pixel.X>=0&&Pixel.X<Run->Width&&Pixel.Y>=0&&Pixel.Y<Run->Height)++Run->InitialVisibleUnits;
            }
            Run->Active=INDEX_NONE;Run->SteadyDiagnostics=Rendering->GetDiagnostics();
            Snap(TEXT("01-dense-rain-steady"));Run->Next=Now+1;break;
        case 5:
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)U->SetActorTickEnabled(true);
            B->SetActorTickEnabled(true);Run->Active=1;Run->BattleStart=Now;
            Run->From=Now+2;Run->Until=Run->From+28;
            UE_LOG(LogTemp,Display,TEXT("V22_PERF_BATTLE_BEGIN sample=28 natural_health=1 audio=1"));break;
        case 6:
            if(Now<Run->Until&&B->Phase==EFrontlinePhase::Fighting){--Run->Stage;break;}
            Run->Samples[1].Completed=Now>=Run->Until;
            if(!Run->Samples[1].Completed){Run->Ready=false;Run->Failure=TEXT("Encounter ended before the 28-second combat sample completed");}
            Run->Active=INDEX_NONE;Run->BattleDiagnostics=Rendering->GetDiagnostics();
            Snap(TEXT("02-battle-after-fire-support"));Run->Next=Now+1;break;
        default:
        {
            auto Stats=[](TArray<double> V,bool FrameRates=true)
            {
                auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),V.Num());
                if(V.IsEmpty())return J;
                V.Sort();double Sum=0;for(double X:V)Sum+=X;
                const int32 Tail=FMath::Max(1,FMath::CeilToInt(V.Num()*.01));double Slow=0;
                for(int32 I=V.Num()-Tail;I<V.Num();++I)Slow+=V[I];
                J->SetNumberField(TEXT("mean_ms"),Sum/V.Num());J->SetNumberField(TEXT("total_ms"),Sum);
                J->SetNumberField(TEXT("p95_ms"),V[FMath::Clamp(FMath::CeilToInt(V.Num()*.95)-1,0,V.Num()-1)]);
                J->SetNumberField(TEXT("p99_ms"),V[FMath::Clamp(FMath::CeilToInt(V.Num()*.99)-1,0,V.Num()-1)]);
                if(FrameRates&&Sum>0&&Slow>0){J->SetNumberField(TEXT("fps_from_mean"),1000/(Sum/V.Num()));J->SetNumberField(TEXT("one_percent_low_fps"),1000/(Slow/Tail));}
                J->SetNumberField(TEXT("max_ms"),V.Last());
                return J;
            };
            auto Report=MakeShared<FJsonObject>();
            Report->SetStringField(TEXT("adapter"),GRHIAdapterName);Report->SetNumberField(TEXT("viewport_width"),Run->Width);Report->SetNumberField(TEXT("viewport_height"),Run->Height);
            Report->SetNumberField(TEXT("initial_soldiers"),Run->InitialUnits);Report->SetNumberField(TEXT("expected_soldiers"),Run->ExpectedUnits);
            Report->SetNumberField(TEXT("steady_projected_living_centres_in_viewport"),Run->InitialVisibleUnits);Report->SetNumberField(TEXT("shots"),B->ShotsFired);
            Report->SetNumberField(TEXT("casualties"),B->Casualties[0]+B->Casualties[1]);Report->SetNumberField(TEXT("artillery_commands"),B->ArtilleryCommands);
            Report->SetNumberField(TEXT("smoke_commands"),B->SmokeCommands);Report->SetNumberField(TEXT("mortar_commands"),B->MortarCommands);
            Report->SetNumberField(TEXT("mortar_rounds_resolved"),B->MortarRoundsResolved);Report->SetArrayField(TEXT("scheduled_action_results"),Run->Events);
            Report->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);Report->SetArrayField(TEXT("screenshots"),Run->Images);
            for(int32 I=0;I<2;++I)
            {
                const auto& S=Run->Samples[I];auto J=MakeShared<FJsonObject>();
                J->SetObjectField(TEXT("wall_render_frames"),Stats(S.Wall));J->SetObjectField(TEXT("rhi_gpu"),Stats(S.GPU));
                J->SetObjectField(TEXT("game_thread_active"),Stats(S.Game,false));J->SetObjectField(TEXT("render_thread_active"),Stats(S.Render,false));J->SetObjectField(TEXT("rhi_thread_active"),Stats(S.RHI,false));
                J->SetNumberField(TEXT("min_living"),S.Wall.IsEmpty()?0:S.MinLiving);J->SetNumberField(TEXT("max_living"),S.MaxLiving);
                double Covered=0;for(double X:S.Wall)Covered+=X;
                const double Requested=I?28.:12.;const bool Coverage=S.Completed&&Covered>=Requested*950.&&S.GPU.Num()>0;
                J->SetNumberField(TEXT("requested_seconds"),Requested);J->SetNumberField(TEXT("sampled_wall_seconds"),Covered*.001);J->SetBoolField(TEXT("window_completed"),S.Completed);J->SetBoolField(TEXT("coverage_valid"),Coverage);
                const bool CameraRecorded=S.MinCameraX<=S.MaxCameraX;
                if(CameraRecorded)
                {
                    J->SetNumberField(TEXT("camera_x_min"),S.MinCameraX);J->SetNumberField(TEXT("camera_x_max"),S.MaxCameraX);
                    J->SetNumberField(TEXT("camera_width_min"),S.MinCameraWidth);J->SetNumberField(TEXT("camera_width_max"),S.MaxCameraWidth);
                }
                const bool CameraStable=CameraRecorded&&S.MaxCameraX-S.MinCameraX<=1.&&S.MaxCameraWidth-S.MinCameraWidth<=1.;
                J->SetBoolField(TEXT("fixed_camera_valid"),CameraStable);Run->Ready&=Coverage&&CameraStable;
                Report->SetObjectField(I?TEXT("combat"):TEXT("steady"),J);
            }
            const bool LiveEffects=B->ShotsFired>0&&B->ArtilleryCommands>0&&B->SmokeCommands>0&&B->MortarCommands>0&&B->MortarRoundsResolved>0;
            Report->SetBoolField(TEXT("live_fire_and_support_observed"),LiveEffects);Run->Ready&=LiveEffects;
            if(!Run->Ready&&Run->Failure.IsEmpty())Run->Failure=TEXT("Setup, sample coverage, fixed camera or actual combat/support evidence was incomplete; inspect named fields");
            Report->SetBoolField(TEXT("fixture_ready"),Run->Ready);Report->SetStringField(TEXT("failure"),Run->Failure);
            if(Run->SteadyDiagnostics)Report->SetObjectField(TEXT("steady_rendering"),Run->SteadyDiagnostics);
            if(Run->BattleDiagnostics)Report->SetObjectField(TEXT("combat_rendering"),Run->BattleDiagnostics);
            auto CVars=MakeShared<FJsonObject>();
            for(const TCHAR* N:{TEXT("r.ScreenPercentage"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("r.TSR.History.ScreenPercentage"),TEXT("r.Lumen.ScreenProbeGather.TracingOctahedronResolution"),TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Lumen.HardwareRayTracing.LightingMode"),TEXT("r.Lumen.Reflections.DownsampleFactor"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Streaming.PoolSize"),TEXT("r.NGX.DLSS.Enable"),TEXT("r.NGX.DLSS.DenoiserMode"),TEXT("t.MaxFPS"),TEXT("r.VSync")})
                if(const auto* C=IConsoleManager::Get().FindConsoleVariable(N))CVars->SetStringField(N,C->GetString());
            Report->SetObjectField(TEXT("live_cvars"),CVars);
            Report->SetStringField(TEXT("scope"),TEXT("Isolated current game with 60 real soldiers/eight squads per team at legal three-slot trench occupancy, explicitly prepositioned for repeatable load. Total scene soldiers are not all visible: steady_projected_living_centres_in_viewport counts projected centres, without testing occlusion or HUD coverage. Rain and skeletal animation throughout. Steady 12s sample after 8s warm-up with actor gameplay paused, followed by up to 28s live combat after 2s warm-up: normal health, animation/AI/physics/audio active, scheduled real fire-support APIs with initial cooldowns ready and acceptance recorded. Natural deaths, AI and engine randomness can change combat load between runs; living ranges and command counters must accompany comparisons. No screenshots during samples; captured afterwards. Not a naturally recruited campaign, camera-pan test or whole-match performance guarantee. Wall samples are CoreTicker engine-tick intervals, not independently observed presentations; plugin presentation count is separately labelled estimate, not measured FG timing. Thread counters are engine-provided durations, not additive frame components; zero can mean inactive/bypassed. Delayed GPU counters are not frame-aligned. p95/p99 use nearest rank; one-percent low FPS is the reciprocal of the mean slowest ceil(1%) intervals. Coverage requires completed windows and at least 95 percent of requested wall time plus positive GPU samples; fixed camera permits at most 1cm/1cm-width drift."));
            FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("performance.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V22/performance.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("V22_PERF_COMPLETE ready=%d shots=%d casualties=%d %s"),Run->Ready,B->ShotsFired,B->Casualties[0]+B->Casualties[1],*Run->Directory);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
