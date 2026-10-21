#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineMaterialV27.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ActorComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
void ReviewVar(const TCHAR* Name,float Value)
{if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))V->SetWithCurrentPriority(Value);}
float ReadReviewVar(const TCHAR* Name)
{const auto* V=IConsoleManager::Get().FindConsoleVariable(Name);return V?V->GetFloat():-999.f;}
double ReviewMean(const TArray<double>& Values)
{double Sum=0;for(double V:Values)Sum+=V;return Values.IsEmpty()?0:Sum/Values.Num();}
double ReviewP95(TArray<double> Values)
{if(Values.IsEmpty())return 0;Values.Sort();return Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*.95)-1,0,Values.Num()-1)];}
}
void StartFrontlineV27RenderReview(AWarfareFrontlineController* Controller)
{
    const bool Path=FParse::Param(FCommandLine::Get(),TEXT("WFV27PathReference"));
    FString UserDir;
    if(!Controller||(!Path&&!FParse::Param(FCommandLine::Get(),TEXT("WFV27RenderReview"))))return;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty()
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    Controller->DisableInput(Controller);Controller->SetActorTickEnabled(false);
    struct FRun
    {
        int32 Stage=0,Active=INDEX_NONE;double Started=FPlatformTime::Seconds(),Next=Started+3,Last=0,Until=0;
        uint64 PathStart=0;bool Ready=true;FString Dir;
        TArray<double> Wall[2],GPU[2];TArray<TSharedPtr<FJsonValue>> Images;
        TSharedPtr<FJsonObject> Diagnostics;
    };
    auto R=MakeShared<FRun>();R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V27")/
        (Path?TEXT("PathReference"):TEXT("RenderReview"))/FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*R->Dir,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R,Weak,Path](float)
    {
        auto* PC=Weak.Get();auto* World=PC?PC->GetWorld():nullptr;
        auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
        if(!PC||!World||!B)return false;
        const double Now=FPlatformTime::Seconds();
        if(R->Active>=0&&R->Last>0&&Now<=R->Until)
        {
            R->Wall[R->Active].Add((Now-R->Last)*1000.);
            const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if(GPU>0)R->GPU[R->Active].Add(GPU);
        }
        R->Last=Now;if(Now<R->Next)return true;
        auto Snapshot=[&](const TCHAR* Name)
        {
            const FString File=R->Dir/(FString(Name)+TEXT(".png"));
            FScreenshotRequest::RequestScreenshot(File,true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),File);
            J->SetBoolField(TEXT("path_tracing_show_flag"),GEngine&&GEngine->GameViewport&&GEngine->GameViewport->EngineShowFlags.PathTracing);
            J->SetNumberField(TEXT("materials_v27"),ReadReviewVar(TEXT("wf.Materials.V27")));
            J->SetNumberField(TEXT("lighting_mode"),ReadReviewVar(TEXT("r.Lumen.HardwareRayTracing.LightingMode")));
            J->SetNumberField(TEXT("reflection_downsample"),ReadReviewVar(TEXT("r.Lumen.Reflections.DownsampleFactor")));
            J->SetNumberField(TEXT("reflection_bounces"),ReadReviewVar(TEXT("r.Lumen.Reflections.MaxBounces")));
            R->Images.Add(MakeShared<FJsonValueObject>(J));
        };
        auto Finish=[&]()
        {
            auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Samples;int32 Written=0;
            for(const auto& I:R->Images)Written+=IFileManager::Get().FileExists(*I->AsObject()->GetStringField(TEXT("file")))?1:0;
            for(int32 I=0;I<2;++I)
            {
                auto S=MakeShared<FJsonObject>();S->SetStringField(TEXT("profile"),I?TEXT("v27"):TEXT("v26_reference"));
                S->SetNumberField(TEXT("frames"),R->Wall[I].Num());S->SetNumberField(TEXT("wall_ms_mean"),ReviewMean(R->Wall[I]));
                S->SetNumberField(TEXT("wall_ms_p95"),ReviewP95(R->Wall[I]));S->SetNumberField(TEXT("gpu_ms_mean"),ReviewMean(R->GPU[I]));
                S->SetNumberField(TEXT("gpu_ms_p95"),ReviewP95(R->GPU[I]));Samples.Add(MakeShared<FJsonValueObject>(S));
            }
            const bool SamplesComplete=Path||(R->Wall[0].Num()>=30&&R->Wall[1].Num()>=30&&R->GPU[0].Num()>0&&R->GPU[1].Num()>0);
            J->SetBoolField(TEXT("success"),R->Ready&&SamplesComplete&&Written==R->Images.Num()&&Written>0);
            J->SetArrayField(TEXT("captures"),R->Images);J->SetArrayField(TEXT("samples"),Samples);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(World))R->Diagnostics=Render->GetDiagnostics();
            if(R->Diagnostics)J->SetObjectField(TEXT("renderer"),R->Diagnostics);
            J->SetNumberField(TEXT("path_frames"),R->PathStart?GFrameCounter-R->PathStart:0);
            J->SetStringField(TEXT("scope"),Path?
                TEXT("Actual UE Path Tracer on the existing battlefield, paused simulation, 64 spp/4 bounces, same camera/materials. Show flag and captured image require visual inspection; not realtime combat performance or path-traced DLSS RR."):
                TEXT("Two short fixed-camera samples on the real battlefield, prepositioned squads, simulation actors stopped while render components remain. Baseline and revised reflection/material profiles in one process. GPU counters are delayed; other running games may contend. Not whole-battle 4K90 certification."));
            FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
            FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("render-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V27_RENDER_REVIEW_COMPLETE ready=%d path=%d %s"),R->Ready,Path,*R->Dir);
            FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Started>90){R->Ready=false;Finish();return false;}
        switch(R->Stage)
        {
        case 0:
        {
            B->StartNewCampaign(EFrontlineFaction::British);B->BeginBattle();B->SetActorTickEnabled(false);
            const EFrontlineRole Roles[]={EFrontlineRole::Rifle,EFrontlineRole::Mills,EFrontlineRole::Lewis,EFrontlineRole::Officer};
            for(int32 Team=0;Team<2;++Team)for(int32 I=0;I<4;++I)R->Ready&=B->SpawnSquad(Team,Roles[I],INDEX_NONE);
            for(AWarfareFrontlineSoldier* U:B->Units)if(U)
            {
                const float X=-3600.f+U->Team*2900.f+int32(U->Role)*180.f+U->MemberIndex*76.f,Y=-150.f+int32(U->Role)*145.f;
                U->SetActorTickEnabled(false);U->ReturnToIdle();U->bFieldEntered=true;U->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88.f));
                U->SetActorRotation(FRotator(0,U->Team?180:0,0));
            }
            R->Ready&=B->Phase==EFrontlinePhase::Fighting&&PC->BattleCamera&&B->Units.Num()>=20;
            if(!R->Ready){Finish();return false;}
            auto* Camera=PC->BattleCamera.Get();const FVector Location(-1800,14000,4650),Focus(-1800,0,650);
            Camera->SetActorLocation(Location);Camera->SetActorRotation((Focus-Location).Rotation());Camera->GetCameraComponent()->OrthoWidth=6000;
            PC->SetViewTarget(Camera);
            for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            {It->SetActorLocation(FVector(-1080,-5200,-1500));It->SetActorScale3D(FVector(125,1,42));break;}
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(World))
            {Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(Path?EFrontlineRenderQuality::Native:EFrontlineRenderQuality::DLAA,false);if(!Path)Render->SetRayReconstruction(true,false);R->Diagnostics=Render->GetDiagnostics();}
            ReviewVar(TEXT("r.VSync"),0);ReviewVar(TEXT("t.MaxFPS"),0);
            if(Path)
            {
                ReviewVar(TEXT("wf.Materials.V27"),1);ReviewVar(TEXT("r.PathTracing.SamplesPerPixel"),64);
                FrontlineMaterialV27::ApplyWorld(World,true);
                ReviewVar(TEXT("r.PathTracing.MaxBounces"),4);ReviewVar(TEXT("r.PathTracing.Denoiser"),1);
                ReviewVar(TEXT("r.PathTracing.ProgressDisplay"),0);
                ReviewVar(TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB"),768);
                // Weather deliberately ticks during a player pause. Freeze its
                // light updates and component animation for an accumulating reference.
                for(TActorIterator<AActor> It(World);It;++It)
                {
                    It->SetActorTickEnabled(false);
                    TInlineComponentArray<UActorComponent*> Components;It->GetComponents(Components);
                    for(auto* C:Components)C->SetComponentTickEnabled(false);
                }
                UGameplayStatics::SetGamePaused(World,true);
                UKismetRenderingLibrary::EnablePathTracing(true);UKismetRenderingLibrary::RefreshPathTracingOutput();
                R->Ready&=ReadReviewVar(TEXT("r.PathTracing"))==1&&GEngine&&GEngine->GameViewport&&GEngine->GameViewport->EngineShowFlags.PathTracing;R->PathStart=GFrameCounter;
                R->Stage=10;R->Next=Now+3;
            }
            else
            {
                ReviewVar(TEXT("wf.Materials.V27"),0);ReviewVar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"),1);
                FrontlineMaterialV27::ApplyWorld(World,false);
                ReviewVar(TEXT("r.Lumen.Reflections.DownsampleFactor"),2);ReviewVar(TEXT("r.Lumen.Reflections.MaxBounces"),1);
                R->Stage=1;R->Next=Now+4;
            }
            break;
        }
        case 1:R->Active=0;R->Until=R->Next=Now+5;R->Stage=2;break;
        case 2:R->Active=INDEX_NONE;Snapshot(TEXT("01-v26-reference"));R->Next=Now+1;R->Stage=3;break;
        case 3:
            ReviewVar(TEXT("wf.Materials.V27"),1);ReviewVar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"),2);
            FrontlineMaterialV27::ApplyWorld(World,true);
            ReviewVar(TEXT("r.Lumen.Reflections.DownsampleFactor"),2);ReviewVar(TEXT("r.Lumen.Reflections.MaxBounces"),2);
            R->Stage=4;R->Next=Now+4;break;
        case 4:R->Active=1;R->Until=R->Next=Now+5;R->Stage=5;break;
        case 5:R->Active=INDEX_NONE;Snapshot(TEXT("02-v27-normal-view"));R->Next=Now+1;R->Stage=6;break;
        case 6:
            for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==0&&U->Role==EFrontlineRole::Rifle&&U->MemberIndex==1)
            {U->SetActorTickEnabled(true);U->ReceiveHit(1.f,.2f,1,FVector(-1,0,0));break;}
            PC->BattleCamera->SetActorLocation(FVector(-3150,14000,4650));
            PC->BattleCamera->SetActorRotation((FVector(-3150,0,180)-PC->BattleCamera->GetActorLocation()).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=1800;
            R->Next=Now+3;R->Stage=7;break;
        case 7:Snapshot(TEXT("03-v27-material-detail"));R->Next=Now+1;R->Stage=8;break;
        case 8:
            PC->BattleCamera->SetActorLocation(FVector(-1000,7000,4200));
            PC->BattleCamera->SetActorRotation((FVector(-1000,-850,65)-PC->BattleCamera->GetActorLocation()).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=2400;
            R->Next=Now+2;R->Stage=9;break;
        case 9:Snapshot(TEXT("04-v27-puddle-and-embers"));R->Next=Now+1;R->Stage=12;break;
        case 10:
            if(GFrameCounter-R->PathStart<128)return true;
            Snapshot(TEXT("04-path-traced-reference"));R->Stage=11;R->Next=Now+2;break;
        default:Finish();return false;
        }
        return true;
    }));
}
