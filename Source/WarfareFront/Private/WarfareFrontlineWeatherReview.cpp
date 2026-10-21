#include "WarfareFrontlineWeather.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlinePrecipitation.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

// Isolated presentation fixture, enabled only by the review launcher.
void StartFrontlineV19WeatherReview(AWarfareFrontlineController* Controller)
{
    if (!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV19WeatherReview"))) return;
    struct FRun
    {
        int32 Stage=0,SampleMode=INDEX_NONE,PeakDrops=0,PeakSplashes=0,FixtureUnits=0;
        double Started=FPlatformTime::Seconds(),Next=Started+3.,LastFrame=0,SampleBegin=0,SampleEnd=0;
        FString Id=FGuid::NewGuid().ToString(),Directory,SettingsPath;
        bool Passed=true;float OvercastSun=0,OvercastSky=0,OvercastWetness=0;
        TArray<double> WallMs[2],GpuMs[2];
        TArray<FTransform> PausedTransforms;
        TWeakObjectPtr<UInstancedStaticMeshComponent> PausedRain;
        TArray<TSharedPtr<FJsonValue>> Checks,Images;
    };
    const auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V19/WeatherReview")/Run->Id);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if (!PC) return false;
        const double Now=FPlatformTime::Seconds();
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        auto* W=AWarfareFrontlineWeather::Get(PC->GetWorld());
        auto* Rain=W?W->GetPrecipitation():nullptr;
        // Every frame, not the half-second command cadence. Both sample windows
        // start after warm-up and end before a screenshot is requested.
        if (Run->SampleMode>=0 && Run->LastFrame>=Run->SampleBegin && Now<=Run->SampleEnd)
        {
            Run->WallMs[Run->SampleMode].Add((Now-Run->LastFrame)*1000.);
            const double GPU=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if (GPU>0) Run->GpuMs[Run->SampleMode].Add(GPU);
        }
        Run->LastFrame=Now;
        if (Rain)
        {
            Run->PeakDrops=FMath::Max(Run->PeakDrops,Rain->GetActiveDropCount());
            Run->PeakSplashes=FMath::Max(Run->PeakSplashes,Rain->GetActiveSplashCount());
        }
        if (Now<Run->Next) return true;
        Run->Next=Now+.5;
        auto Check=[&](const TCHAR* Name,bool Passed,const FString& Detail=FString())
        {
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("name"),Name);O->SetBoolField(TEXT("passed"),Passed);O->SetStringField(TEXT("detail"),Detail);
            Run->Checks.Add(MakeShared<FJsonValueObject>(O));Run->Passed&=Passed;
            UE_LOG(LogTemp,Display,TEXT("WF_V19_WEATHER %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
        };
        auto Command=[&](const TCHAR* Name){if(HUD) HUD->NotifyHitBoxClick(FName(Name));};
        auto Snap=[&](const TCHAR* Name,int32 Page)
        {
            const FString Path=Run->Directory/(FString(Name)+TEXT(".png"));
            FScreenshotRequest::RequestScreenshot(Path,true,false);
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("file"),Path);O->SetNumberField(TEXT("width"),Width);O->SetNumberField(TEXT("height"),Height);
            O->SetNumberField(TEXT("weather_mode"),W?int32(W->GetMode()):-1);O->SetNumberField(TEXT("rain"),W?W->GetRainAmount():-1);O->SetNumberField(TEXT("wetness"),W?W->GetWetness():-1);
            O->SetNumberField(TEXT("sun"),W?W->GetSunIntensity():-1);O->SetNumberField(TEXT("sky"),W?W->GetSkyIntensity():-1);
            O->SetNumberField(TEXT("camera_x"),PC->BattleCamera?PC->BattleCamera->GetActorLocation().X:0);
            O->SetNumberField(TEXT("camera_width"),PC->BattleCamera?PC->BattleCamera->GetCameraComponent()->OrthoWidth:0);
            Run->Images.Add(MakeShared<FJsonValueObject>(O));
            Check(Name,HUD && HUD->GetPresentationPage()==Page && B && B->AssetFailure.IsEmpty(),
                FString::Printf(TEXT("page=%d AssetFailure=%s"),HUD?HUD->GetPresentationPage():-1,B?*B->AssetFailure:TEXT("GameMode missing")));
        };
        auto BeginSample=[&](int32 Mode)
        {Run->SampleMode=Mode;Run->SampleBegin=Now+3.;Run->SampleEnd=Run->SampleBegin+7.;};
        if (Run->Stage<99 && ((!B || !HUD || !W) || !B->AssetFailure.IsEmpty() || Now-Run->Started>55.))
        {
            Check(TEXT("flow_resources_and_timeout"),false,FString::Printf(TEXT("B=%d HUD=%d Weather=%d age=%.2f AssetFailure=%s"),!!B,!!HUD,!!W,Now-Run->Started,B?*B->AssetFailure:TEXT("missing")));
            Run->Stage=99;Run->SampleMode=INDEX_NONE;
        }
        switch(Run->Stage++)
        {
        case 0:
            // Public actor tick control is confined to this opt-in fixture:
            // suppress AI recruitment/morale/objective progression, not weather.
            PC->Restart();B->SetActorTickEnabled(false);PC->ResetCamera();
            PC->SetMouseLocation(1280,1380);Command(TEXT("NewCampaign"));break;
        case 1: Command(TEXT("FactionBritish"));break;
        case 2: Command(TEXT("Begin"));break;
        case 3:
            if (B->Phase!=EFrontlinePhase::Fighting)
            {
                if (Now-Run->Started>22.) {Check(TEXT("real_operation_preparation_completed"),false,TEXT("Fighting phase not reached within 22 wall seconds"));Run->Stage=99;}
                else --Run->Stage;
                break;
            }
            Check(TEXT("real_operation_preparation_completed"),B->GetEncounterCampaignMission()!=nullptr && HUD->GetPresentationPage()==3);
            Check(TEXT("weather_materials_lights_and_precipitation_ready"),W->IsReady());
            Check(TEXT("empty_battlefield_before_fixture"),B->Units.IsEmpty() && B->Squads.IsEmpty());
            W->SetMode(EFrontlineWeatherMode::Overcast,false);W->SnapToWeatherForReview();PC->ResetCamera();Run->Next=Now+3.;break;
        case 4:
            Check(TEXT("v18_comparison_camera_preserved"),PC->BattleCamera && FMath::IsNearlyEqual(PC->BattleCamera->GetActorLocation().X,-6700.,1.) && FMath::IsNearlyEqual(PC->BattleCamera->GetCameraComponent()->OrthoWidth,6000.f,1.f));
            Snap(TEXT("01-empty-overcast-v18-camera"),3);Run->Next=Now+1.;break;
        case 5:
        {
            bool Recruited=true;
            for (int32 Team=0;Team<2;++Team)
                for (EFrontlineRole Role:{EFrontlineRole::Rifle,EFrontlineRole::Lewis})
                {
                    for (int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I) B->RecruitReadyAt[Team][I]=B->Elapsed;
                    Recruited&=B->Recruit(Role,Team);
                }
            int32 Slots[2]={0,0};
            for (auto& Squad:B->Squads)
            {
                Squad.Trench=Squad.Destination=Squad.Team;Squad.Slot=Slots[Squad.Team]++;
                Squad.bAdvancing=Squad.bReinforcement=false;Squad.bHoldPosition=true;
                B->Trenches[Squad.Trench].Owner=Squad.Team;
            }
            for (AWarfareFrontlineSoldier* Unit:B->Units)
                if (Unit) {Unit->SetActorLocation(B->SoldierDestination(Unit)+FVector(0,0,88));Unit->ReturnToIdle();Unit->SetActorTickEnabled(false);}
            Run->FixtureUnits=B->Units.Num();B->LastMessage.Empty();
            Check(TEXT("fixed_two_trench_roster_created_via_recruit"),Recruited && B->ActiveSquads(0)==2 && B->ActiveSquads(1)==2 && Run->FixtureUnits>0,
                FString::Printf(TEXT("%d soldiers; Rifle and Lewis/MG groups on each side; GameMode and Soldier actor ticks disabled, skeletal components continue"),Run->FixtureUnits));
            Run->OvercastSun=W->GetSunIntensity();Run->OvercastSky=W->GetSkyIntensity();Run->OvercastWetness=W->GetWetness();
            BeginSample(0);break;
        }
        case 6:
            if (Now<Run->SampleEnd) {--Run->Stage;break;}
            Run->SampleMode=INDEX_NONE;Snap(TEXT("02-roster-overcast"),3);Run->Next=Now+1.;break;
        case 7: Command(TEXT("Settings"));break;
        case 8: Command(TEXT("WeatherMode3"));break;
        case 9:
        {
            const FConfigBranch* Branch=GConfig?GConfig->FindBranchWithNoReload(FName(TEXT("GameUserSettings")),FString()):nullptr;
            Run->SettingsPath=FPaths::ConvertRelativePathToFull(Branch && !Branch->IniPath.IsEmpty()?Branch->IniPath:
                FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir()));
            FConfigFile Disk;Disk.Read(Run->SettingsPath);int32 SavedMode=-1;
            const bool OnDisk=Disk.GetInt(TEXT("WarfareFrontline.Weather"),TEXT("Mode"),SavedMode);
            Check(TEXT("settings_weather_click_applies_and_saves_physical_ini"),W->GetMode()==EFrontlineWeatherMode::Rain && OnDisk && SavedMode==3,
                FString::Printf(TEXT("runtime=%d physical_ini=%d explicit_key=%d file=%s"),int32(W->GetMode()),SavedMode,OnDisk,*Run->SettingsPath));
            Command(TEXT("DisplayBack"));W->SnapToWeatherForReview();BeginSample(1);break;
        }
        case 10:
        {
            if (Now<Run->SampleEnd) {--Run->Stage;break;}
            Run->SampleMode=INDEX_NONE;
            Check(TEXT("storm_lowers_sun_and_sky_and_increases_wetness"),W->GetSunIntensity()<Run->OvercastSun && W->GetSkyIntensity()<Run->OvercastSky && W->GetWetness()>Run->OvercastWetness,
                FString::Printf(TEXT("sun %.3f->%.3f sky %.3f->%.3f wetness %.3f->%.3f"),Run->OvercastSun,W->GetSunIntensity(),Run->OvercastSky,W->GetSkyIntensity(),Run->OvercastWetness,W->GetWetness()));
            int32 DropInstances=0,SplashInstances=0;
            if (Rain)
            {
                TArray<UInstancedStaticMeshComponent*> Components;Rain->GetComponents(Components);
                for (auto* C:Components) if (C)
                {if(C->GetName()==TEXT("RainLines")) DropInstances=C->GetInstanceCount();else if(C->GetName()==TEXT("RainContacts")) SplashInstances=C->GetInstanceCount();}
            }
            Check(TEXT("rain_and_splash_pools_are_live_and_bounded"),Rain && Rain->IsVisualReady() && Rain->GetActiveDropCount()>0 && Run->PeakDrops<=600 && Run->PeakSplashes<=96 && DropInstances>0 && DropInstances<=600 && SplashInstances>0 && SplashInstances<=96,
                FString::Printf(TEXT("active=%d peak_drops=%d peak_splashes=%d allocated=%d/%d"),Rain?Rain->GetActiveDropCount():0,Run->PeakDrops,Run->PeakSplashes,DropInstances,SplashInstances));
            Check(TEXT("roster_and_phase_identical_across_comparison"),B->Phase==EFrontlinePhase::Fighting && B->Units.Num()==Run->FixtureUnits && B->LivingSoldiers(0)+B->LivingSoldiers(1)==Run->FixtureUnits && B->ShotsFired==0);
            Snap(TEXT("03-roster-rain"),3);Run->Next=Now+1.;break;
        }
        case 11: Command(TEXT("Pause"));break;
        case 12:
            if (Rain)
            {
                TArray<UInstancedStaticMeshComponent*> Components;Rain->GetComponents(Components);
                for (auto* C:Components) if (C && C->GetName()==TEXT("RainLines"))
                {
                    Run->PausedRain=C;
                    for (int32 I=0;I<C->GetInstanceCount();++I) {FTransform T;if(C->GetInstanceTransform(I,T,true)) Run->PausedTransforms.Add(T);}
                }
            }
            Run->Next=Now+1.;break;
        case 13:
        {
            auto* C=Run->PausedRain.Get();bool Same=PC->GetWorld()->IsPaused() && C && Run->PausedTransforms.Num()>0 && C->GetInstanceCount()==Run->PausedTransforms.Num();
            if (Same) for (int32 I=0;I<Run->PausedTransforms.Num();++I)
            {FTransform T;Same&=C->GetInstanceTransform(I,T,true) && T.Equals(Run->PausedTransforms[I],.0001f);}
            Check(TEXT("paused_rain_instance_transforms_do_not_advance"),Same,FString::Printf(TEXT("%d actual world transforms compared over one paused wall second"),Run->PausedTransforms.Num()));
            Command(TEXT("Pause"));break;
        }
        case 14: Command(TEXT("Settings"));break;
        case 15:
            Check(TEXT("reopened_weather_settings_retain_rain_mode"),HUD->GetPresentationPage()==7 && W->GetMode()==EFrontlineWeatherMode::Rain);
            Snap(TEXT("04-settings-weather"),7);break;
        case 16: Command(TEXT("DisplayBack"));break;
        case 17: PC->Restart();break;
        case 18: Command(TEXT("MenuHome"));break;
        default:
        {
            auto Stats=[](TArray<double> Values,bool IncludeFrameRate)
            {
                auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("samples"),Values.Num());
                if (!Values.IsEmpty())
                {
                    double Sum=0;for(double X:Values)Sum+=X;Values.Sort();
                    O->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());if(IncludeFrameRate) O->SetNumberField(TEXT("fps_from_mean"),1000.*Values.Num()/Sum);
                    O->SetNumberField(TEXT("p99_ms"),Values[FMath::Min(Values.Num()-1,int32(Values.Num()*.99))]);O->SetNumberField(TEXT("max_ms"),Values.Last());
                }return O;
            };
            Check(TEXT("two_seven_second_render_samples_available"),Run->WallMs[0].Num()>30 && Run->WallMs[1].Num()>30 && Run->GpuMs[0].Num()>0 && Run->GpuMs[1].Num()>0,
                FString::Printf(TEXT("wall_samples=%d/%d gpu_samples=%d/%d"),Run->WallMs[0].Num(),Run->WallMs[1].Num(),Run->GpuMs[0].Num(),Run->GpuMs[1].Num()));
            int32 WrittenImages=0;for(const auto& Image:Run->Images)
                if(IFileManager::Get().FileSize(*Image->AsObject()->GetStringField(TEXT("file")))>0) ++WrittenImages;
            Check(TEXT("four_screenshots_written_to_disk"),WrittenImages==4 && Run->Images.Num()==4,FString::Printf(TEXT("written=%d requested=%d"),WrittenImages,Run->Images.Num()));
            const bool QuitReady=B && HUD && B->Phase==EFrontlinePhase::Briefing && HUD->GetPresentationPage()==0;
            Check(TEXT("main_menu_quit_route_available"),QuitReady);
            auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("run_id"),Run->Id);O->SetBoolField(TEXT("passed"),Run->Passed);
            O->SetArrayField(TEXT("checks"),Run->Checks);O->SetArrayField(TEXT("screenshots"),Run->Images);
            O->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);O->SetStringField(TEXT("adapter"),GRHIAdapterName);O->SetStringField(TEXT("settings_ini"),Run->SettingsPath);
            O->SetNumberField(TEXT("fixture_soldiers"),Run->FixtureUnits);O->SetNumberField(TEXT("peak_rain_drops"),Run->PeakDrops);O->SetNumberField(TEXT("peak_splashes"),Run->PeakSplashes);
            auto Samples=MakeShared<FJsonObject>();
            for(int32 I=0;I<2;++I){auto P=MakeShared<FJsonObject>();P->SetObjectField(TEXT("wall_frame"),Stats(Run->WallMs[I],true));P->SetObjectField(TEXT("rhi_gpu"),Stats(Run->GpuMs[I],false));Samples->SetObjectField(I?TEXT("rain"):TEXT("overcast"),P);}
            O->SetObjectField(TEXT("render_comparison"),Samples);
            auto Settings=MakeShared<FJsonObject>();
            for (const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("t.MaxFPS"),TEXT("r.VSync"),TEXT("t.IdleWhenNotForeground"),TEXT("sg.GlobalIlluminationQuality"),TEXT("r.Lumen.HardwareRayTracing")})
                if(const auto* V=IConsoleManager::Get().FindConsoleVariable(Name)) Settings->SetStringField(Name,V->GetString());
            O->SetObjectField(TEXT("live_settings"),Settings);
            O->SetStringField(TEXT("scope"),TEXT("Four actual screenshots and spaced HUD dispatches in real L_Frontline. Opt-in fixture disables GameMode and Soldier actor ticks, leaves skeletal components and weather rendering active, and places publicly recruited British/German Rifle plus Lewis/MG groups in trenches 0/1. Camera remains x=-6700, width=6000. Two seven-second rendering samples follow three-second warm-ups; screenshot frames and their following warm-up are excluded. Background-window, capped rendering comparison only, not combat performance or stress testing; delayed RHI GPU samples need not match the same wall frame. Weather states snap only in this fixture; natural transition quality is not asserted. Physical saved INI is read independently of modified GConfig memory; single-process check does not claim process-restart persistence. No visual-quality pass is inferred from API checks."));
            O->SetStringField(TEXT("exit_route"),QuitReady?TEXT("HUD QuitGame; parent must verify process exit"):TEXT("fixture failure fallback exit"));
            FString Json;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("weather-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V19/weather-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V19_WEATHER_REVIEW_COMPLETE Passed=%d %s"),Run->Passed,*Run->Directory);
            if(QuitReady) Command(TEXT("QuitGame"));else FPlatformMisc::RequestExit(false);
            return false;
        }
        }
        return true;
    }));
}
