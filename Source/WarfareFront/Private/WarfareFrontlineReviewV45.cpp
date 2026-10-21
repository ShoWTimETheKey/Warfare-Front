#include "WarfareFrontlineReviewV45.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineHUDV45.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineWeather.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/HUD.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "RHI.h"
#include "RHIStats.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace
{
bool ReviewShadersPending45()
{
#if WITH_EDITOR
    return GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
#else
    return false;
#endif
}
bool Allowed45(const AWarfareFrontlineGameMode* B)
{
    FString Dir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
TSharedRef<FJsonObject> Distribution45(TArray<double> Values)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());J->SetBoolField(TEXT("available"),!Values.IsEmpty());
    if(Values.IsEmpty())return J;double Sum=0;for(double V:Values)Sum+=V;Values.Sort();
    J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());J->SetNumberField(TEXT("max_ms"),Values.Last());
    auto Percentile=[&](double P){return Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*P)-1,0,Values.Num()-1)];};
    J->SetNumberField(TEXT("p50_ms"),Percentile(.50));J->SetNumberField(TEXT("p95_ms"),Percentile(.95));J->SetNumberField(TEXT("p99_ms"),Percentile(.99));
    int32 Over33=0,Over50=0;for(double V:Values){Over33+=V>33.3;Over50+=V>50.;}J->SetNumberField(TEXT("over_33_3_ms"),Over33);J->SetNumberField(TEXT("over_50_ms"),Over50);return J;
}
struct FSample45
{
    TArray<double> Wall,GPU,Game,Render,RHI;double Seconds=0;int32 Unfocused=0;
    TSharedRef<FJsonObject> Describe()const
    {
        auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Distribution45(Wall));J->SetObjectField(TEXT("gpu"),Distribution45(GPU));
        J->SetObjectField(TEXT("game_thread"),Distribution45(Game));J->SetObjectField(TEXT("render_thread"),Distribution45(Render));J->SetObjectField(TEXT("rhi_thread"),Distribution45(RHI));
        J->SetNumberField(TEXT("sample_seconds"),Seconds);J->SetNumberField(TEXT("render_fps"),Seconds>0?Wall.Num()/Seconds:0);J->SetNumberField(TEXT("unfocused_frames"),Unfocused);
        J->SetStringField(TEXT("scope"),TEXT("One eight-second fixed normal wet view: six real friendly squads, active soldier/weather ticks, held combat director. Whole-frame delayed RHI GPU timer and actual game/render/RHI timers; no screenshot readback during sampling. Not a full-combat benchmark or per-pass GPU profile."));return J;
    }
};
TSharedRef<FJsonObject> Terrain45(AWarfareFrontlineGameMode* B)
{
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
    {
        if(!IsValid(*It)||It->IsActorBeingDestroyed())continue;
        TSharedPtr<FJsonObject> J;const FString Source=It->DescribeTerrainV43();
        if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),J)&&J.IsValid())
        {J->SetStringField(TEXT("actor"),It->GetPathName());J->SetBoolField(TEXT("parsed_actual_actor_json"),true);return J.ToSharedRef();}
    }
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("parsed_actual_actor_json"),false);return J;
}
bool BakedTerrain45(const TSharedRef<FJsonObject>& J)
{
    bool Complete=false;if(!J->TryGetBoolField(TEXT("baked_complete"),Complete)||!Complete)return false;
    const TSharedPtr<FJsonObject>* Water=nullptr;
    if(!J->TryGetObjectField(TEXT("basin_water_v44"),Water)||!Water||!Water->IsValid())return false;
    bool WaterShader=false,WaterCollision=false;double WaterTriangles=0;
    if(!(*Water)->TryGetBoolField(TEXT("material_shader_valid"),WaterShader)||!WaterShader
        ||!(*Water)->TryGetBoolField(TEXT("collision_disabled"),WaterCollision)||!WaterCollision
        ||!(*Water)->TryGetNumberField(TEXT("triangles"),WaterTriangles)||WaterTriangles<=0)return false;
    const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;if(!J->TryGetArrayField(TEXT("visible_tiles"),Tiles)||!Tiles||Tiles->Num()!=20)return false;
    for(const auto& Value:*Tiles)
    {
        const auto Row=Value->AsObject();bool DF=false,Shader=false;double Triangles=0,Cards=0,Clusters=0;
        if(!Row.IsValid()||!Row->TryGetBoolField(TEXT("distance_field"),DF)||!DF
            ||!Row->TryGetBoolField(TEXT("material_shader_valid"),Shader)||!Shader
            ||!Row->TryGetNumberField(TEXT("triangles"),Triangles)||Triangles<=0
            ||!Row->TryGetNumberField(TEXT("lumen_cards"),Cards)||Cards<=0
            ||!Row->TryGetNumberField(TEXT("nanite_clusters"),Clusters)||Clusters<=0)return false;
    }
    return true;
}
TSharedRef<FJsonObject> Render45(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
    J->SetBoolField(TEXT("fg_off_requested_and_effective"),R&&R->GetFrameGeneration()==EFrontlineFrameGeneration::Off&&R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
    J->SetBoolField(TEXT("quality_sr"),R&&R->GetQuality()==EFrontlineRenderQuality::Quality);J->SetBoolField(TEXT("rr_requested"),R&&R->GetRayReconstruction());
    J->SetBoolField(TEXT("no_rr_diagnostic_requested"),FParse::Param(FCommandLine::Get(),TEXT("WFV44NoRayReconstruction")));
    J->SetBoolField(TEXT("rr_enabled_api"),R&&R->GetDiagnostics()->GetBoolField(TEXT("rr_enabled_api")));
    if(R){J->SetObjectField(TEXT("runtime_diagnostics"),R->GetDiagnostics());J->SetNumberField(TEXT("production_frame_cap"),R->GetFrameRateLimit());}
    auto CV=MakeShared<FJsonObject>();for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("r.PathTracing"),TEXT("r.RayTracing"),TEXT("r.Nanite")})
        if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))CV->SetStringField(Name,V->GetString());
    J->SetObjectField(TEXT("render_cvars"),CV);return J;
}
int32 Living45(const AWarfareFrontlineGameMode* B)
{int32 Count=0;for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->IsAlive())++Count;return Count;}
}

struct FFrontlineReviewV45
{
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Operation)
    {
        if(!Allowed45(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,Operation))return false;
        B->BeginBattle();B->SetBattleSpeed(1);B->Elapsed=20;B->SetActorTickEnabled(false);
        B->Random.Initialize(451917+int32(F));B->EnemyDecisionRandom.Initialize(45+int32(F));
        bool Good=B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
        for(int32 Line=0;Line<2;++Line)
        {
            const int32 Trench=Line?B->GetHomeTrenchIndex(0):FrontlineBattlefield::CentreTrench;B->Trenches[Trench].Owner=0;
            for(int32 Slot=0;Slot<3;++Slot)
                Good&=B->SpawnSquad(0,Slot==1&&B->IsRoleUnlocked(EFrontlineRole::Lewis)?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Trench);
        }
        for(auto& S:B->Squads)S.bHoldPosition=true;
        B->Morale[0]=B->Morale[1]=72;return Good&&B->Squads.Num()==6;
    }
    static void StartOutcome(AWarfareFrontlineGameMode* B)
    {
        if(!Allowed45(B))return;
        FrontlineMotionV41::PrepareActionReview(B,2);
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        {Audio->SetBattleActive(false);Audio->PlayOutcomeV32(B->GetFactionForTeam(0)==EFrontlineFaction::German,FVector::ZeroVector);}
    }
    static bool LegacyWater(AWarfareFrontlineGameMode* B,float Value)
    {
        if(!Allowed45(B))return false;
        for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
            if(It->Surface)if(auto* MID=Cast<UMaterialInstanceDynamic>(It->Surface->GetMaterial(0)))
            {
                float Current=-1;
                if(!MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("LegacyWaterMask")),Current))continue;
                MID->SetScalarParameterValue(TEXT("LegacyWaterMask"),Value);
                return MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("LegacyWaterMask")),Current)&&FMath::IsNearlyEqual(Current,Value);
            }
        return false;
    }
};

namespace
{
TSharedRef<FJsonObject> Scene45(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();int32 Actors=0,Components=0,Volumes=0,Invalid=0,Secondary=0;
    TArray<TSharedPtr<FJsonValue>> Proxies,Impacts;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(!IsValid(*It)||It->IsActorBeingDestroyed())continue;++Actors;
        const FTransform T=It->GetActorTransform();if(T.ContainsNaN()||!T.IsValid())++Invalid;
        const auto* Blast=Cast<AWarfareFrontlineImpact>(*It);
        const bool FX=Blast||Cast<AFrontlineSupportMissionV28>(*It);
        if(Blast)
        {
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("actor"),It->GetPathName());
            R->SetNumberField(TEXT("profile"),Blast->GetBlastProfileIndex());R->SetNumberField(TEXT("age"),Blast->GetVisualAgeSeconds());
            R->SetBoolField(TEXT("ready"),Blast->IsVisualReady());R->SetBoolField(TEXT("flash"),Blast->IsFlashVisible());
            R->SetNumberField(TEXT("smoke_components"),Blast->GetVisibleSmokeCount());R->SetNumberField(TEXT("fragments"),Blast->GetBallisticFragmentCount());
            Impacts.Add(MakeShared<FJsonValueObject>(R));
        }
        if(!FX)continue;TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
        for(auto* C:Meshes)
        {
            if(!C||!C->IsVisible()||C->bHiddenInGame||!C->GetStaticMesh())continue;++Components;
            if(C->GetComponentTransform().ContainsNaN()||!C->GetComponentTransform().IsValid())++Invalid;
            if(C->GetStaticMesh()->GetName()!=TEXT("Cube"))continue;++Volumes;
            const bool Obstructs=C->bVisibleInRayTracing||C->bAffectDistanceFieldLighting||C->CastShadow;
            Secondary+=Obstructs;
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("component"),C->GetPathName());
            R->SetBoolField(TEXT("ray_tracing"),C->bVisibleInRayTracing);R->SetBoolField(TEXT("distance_field"),C->bAffectDistanceFieldLighting);
            R->SetBoolField(TEXT("casts_shadow"),C->CastShadow);R->SetStringField(TEXT("material"),GetPathNameSafe(C->GetMaterial(0)));
            R->SetStringField(TEXT("scale"),C->GetComponentScale().ToString());Proxies.Add(MakeShared<FJsonValueObject>(R));
        }
    }
    J->SetNumberField(TEXT("actors"),Actors);J->SetNumberField(TEXT("visible_fx_components"),Components);J->SetNumberField(TEXT("visible_volume_proxies"),Volumes);
    J->SetNumberField(TEXT("invalid_transforms"),Invalid);J->SetNumberField(TEXT("secondary_obstruction_proxies"),Secondary);
    J->SetArrayField(TEXT("actual_volume_proxies"),Proxies);J->SetArrayField(TEXT("actual_impacts"),Impacts);
    return J;
}
bool ProfileMatches45(AWarfareFrontlineGameMode* B,const TSharedRef<FJsonObject>& J)
{
    const auto* M=B->GetEncounterCampaignMission();if(!M)return false;
    const FString Depth=FString::Printf(TEXT("_D%d"),FMath::RoundToInt(FrontlineGameplayV33::OperationDepthScale(M->Id)*1000000.f));
    const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;if(!J->TryGetArrayField(TEXT("visible_tiles"),Tiles)||!Tiles||Tiles->Num()!=20)return false;
    for(const auto& V:*Tiles){const auto R=V->AsObject();FString Mesh;if(!R||!R->TryGetStringField(TEXT("mesh"),Mesh)||!Mesh.Contains(Depth))return false;}
    const TSharedPtr<FJsonObject>* Water=nullptr;FString Mesh;
    return J->TryGetObjectField(TEXT("basin_water_v44"),Water)&&Water&&(*Water)->TryGetStringField(TEXT("mesh"),Mesh)&&Mesh.Contains(Depth);
}
}

void StartFrontlineV45Review(AWarfareFrontlineGameMode* Battle)
{
    if(!Allowed45(Battle))return;
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> B;TWeakObjectPtr<AFrontlineSupportMissionV28> Gas;
        FString Dir;int32 Stage=0,Phase=0,ActionMode=-1,W=3840,H=2160,Living=0;
        bool Passed=true,Sampling=false,Stress=false,Audit=false,CameraQueued=false;double Start=FPlatformTime::Seconds(),Next=Start+2,Last=Start,StageStart=Start,NextBlast=0;
        FVector QueuedFocus=FVector::ZeroVector;float QueuedWidth=6000;
        float Width=6000;FVector Focus=FVector(0,0,650);FSample45 Sample;
        TArray<FString> Shots;TArray<TSharedPtr<FJsonValue>> Checks,Views;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->B=Battle;FParse::Value(FCommandLine::Get(),TEXT("ResX="),R->W);FParse::Value(FCommandLine::Get(),TEXT("ResY="),R->H);
    R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V45/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();if(!B)return false;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        if(!PC||!PC->GetHUD())
        {
            if(Now-R->Start<20)return true;
            R->Report->SetBoolField(TEXT("success"),false);R->Report->SetStringField(TEXT("error"),TEXT("No player controller/HUD within twenty seconds"));
            FString S;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(R->Dir/TEXT("review.json")));FPlatformMisc::RequestExit(false);return false;
        }
        auto Camera=[&](float X,float Y,float Width,float Z,bool Cut=true)
        {
            // RequestScreenshot reads the next rendered view, not the view at
            // this call. Never replace its camera in the same ticker callback.
            if(FScreenshotRequest::IsScreenshotRequested()){R->QueuedFocus=FVector(X,Y,Z);R->QueuedWidth=Width;R->CameraQueued=true;return;}
            R->Focus=FVector(X,Y,Z);R->Width=Width;FrontlineReviewUIV41::Camera(PC,X,Y,Width,Z,Cut);
        };
        if(R->CameraQueued&&!FScreenshotRequest::IsScreenshotRequested())
        {const FVector F=R->QueuedFocus;R->CameraQueued=false;Camera(F.X,F.Y,R->QueuedWidth,F.Z);}
        auto Shot=[&](const FString& Name)
        {
            const FString Path=R->Dir/(Name+TEXT(".png"));R->Shots.Add(Path);FScreenshotRequest::RequestScreenshot(Path,true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("capture"),Name);J->SetNumberField(TEXT("width_cm"),R->Width);J->SetStringField(TEXT("focus"),R->Focus.ToString());
            J->SetNumberField(TEXT("battle_seconds"),B->Elapsed);J->SetNumberField(TEXT("faction"),int32(B->GetPlayerFaction()));
            J->SetObjectField(TEXT("actual_scene"),Scene45(B));
            if(R->ActionMode>=0)J->SetObjectField(TEXT("actual_motion"),FrontlineMotionV41::ActionReviewState(B));
            if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())J->SetObjectField(TEXT("actual_outcome_audio"),A->DescribeOutcomeV45());
            if(PC->BattleCamera){J->SetStringField(TEXT("actual_camera"),PC->BattleCamera->GetActorTransform().ToString());J->SetNumberField(TEXT("actual_ortho_width"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);}
            R->Views.Add(MakeShared<FJsonValueObject>(J));
        };
        auto Finish=[&]()
        {
            R->Sampling=R->Stress=false;B->SetActorTickEnabled(false);int32 W=0,H=0;PC->GetViewportSize(W,H);
            Check(TEXT("requested_actual_viewport"),W==R->W&&H==R->H);
            bool Written=R->Shots.Num()==19;for(const auto& P:R->Shots)Written&=IFileManager::Get().FileSize(*P)>1024;Check(TEXT("eighteen_changed_area_and_one_legacy_water_AB_captures_written"),Written);
            if(R->Audit){auto J=FrontlineHUDV45::EndAudit();R->Report->SetObjectField(TEXT("chinese_HUD_audit"),J);Check(TEXT("chinese_HUD_actual_draws_and_measurement"),J->GetBoolField(TEXT("success")));R->Audit=false;}
            const auto UI=FrontlineReviewUIV41::EndAudit();R->Report->SetObjectField(TEXT("actual_UI_layout"),UI);bool Fits=false;UI->TryGetBoolField(TEXT("success"),Fits);Check(TEXT("actual_UI_boxes_fit"),Fits);
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("captures"),R->Views);
            R->Report->SetObjectField(TEXT("final_render"),Render45(B));R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("rhi"),GDynamicRHI?GDynamicRHI->GetName():TEXT("unavailable"));R->Report->SetStringField(TEXT("adapter"),GRHIAdapterName);
            R->Report->SetNumberField(TEXT("width"),W);R->Report->SetNumberField(TEXT("height"),H);R->Report->SetBoolField(TEXT("audio_output_enabled"),!FParse::Param(FCommandLine::Get(),TEXT("nosound")));
            R->Report->SetStringField(TEXT("scope"),TEXT("V45 local focused review only. Actual shortened early British and middle German fields, 20 baked tiles and matching basin depth keys; Chinese HUD and three real tooltips; current trench and outcome poses; three frozen explosion profiles at four ages; live gas/blast camera-pan stress. One eight-second fixed-field sample and one eight-second FX/pan sample, Quality SR/RR and FG off. No historical suites, path-tracing requirement, automatic artistic approval or claims about untested GPUs. Audio scheduler data is not subjective listening. Explicit isolated volatile UserDir; bounded 90-second self-exit."));
            FString S;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Start>90){Check(TEXT("bounded_ninety_second_review"),false);Finish();return false;}
        const float Step=FMath::Clamp(DT,0.f,.05f);
        if(R->ActionMode>=0)FrontlineMotionV41::TickActionReview(B,Step);
        else if(R->Stage>0&&B->Phase==EFrontlinePhase::Fighting&&!B->IsActorTickEnabled())B->Elapsed+=Step;
        if(R->Stress)
        {
            const float T=float(Now-R->StageStart),Limit=FrontlineTerrainV42::CameraVerticalLimitV43(6000,float(R->W)/R->H);
            Camera(FMath::Sin(T*.53f)*650,FMath::Sin(T*.71f)*Limit*.9f,6000,350,false);
            if(Now>=R->NextBlast)
            {
                R->NextBlast=Now+1.7;const int32 Kind=int32(T/1.7f)%3;const float X=-1300+Kind*1300;
                if(auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(FVector(X,400,AWarfareFrontlineGround::HeightAt(X,400)),FRotator::ZeroRotator))A->ConfigureBlast(Kind==0,Kind==1,FVector(.5,.2,-1));
            }
        }
        if(R->Stage>0&&PC->BattleCamera)FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),R->Focus,R->Width,R->W);
        if(R->Sampling)
        {
            const double Ms=(Now-R->Last)*1000;R->Last=Now;if(Ms>0){R->Sample.Wall.Add(Ms);R->Sample.Seconds+=Ms*.001;}
            auto Add=[](TArray<double>& A,double V){if(FMath::IsFinite(V)&&V>0)A.Add(V);};
            Add(R->Sample.GPU,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(R->Sample.Game,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(R->Sample.Render,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(R->Sample.RHI,FPlatformTime::ToMilliseconds(GRHIThreadTime));if(!FApp::HasFocus())++R->Sample.Unfocused;
        }
        if(Now<R->Next)return true;
        auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        switch(R->Stage)
        {
        case 0:
            if(ReviewShadersPending45()){R->Next=Now+.5;break;}
            Check(TEXT("isolated_volatile_review_profile"),Allowed45(B));FrontlineReviewUIV41::BeginAudit();FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(B,false);
            if(Render){Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetFrameRateSlider(1,false);}
            if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))V->Set(0,ECVF_SetByCode);
            FrontlineGameplayV33::RunDepthChecksV45(B,Check);if(Audio)Audio->RunV45OutcomeChecks(Check);else Check(TEXT("audio_subsystem_available"),false);
            Check(TEXT("isolated_late_operation_for_infantry_checks"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,19));
            FrontlineInfantryV45::RunChecks(B,Check); // Includes RunOutcomeChecksV45 once, via its actual actor fixture.
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
            Check(TEXT("early_british_wet_field"),FFrontlineReviewV45::Field(B,EFrontlineFaction::British,0));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);
            Check(TEXT("actual_early_weather_prepared"),FrontlineLightingV42::PrepareReviewWeather(B));R->Living=Living45(B);R->Stage=1;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+5;break;
        case 1:
        {
            const auto J=Terrain45(B);const auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());
            if((ReviewShadersPending45()||!BakedTerrain45(J)||!Weather||!Weather->IsReady())&&Now-R->StageStart<25){R->Next=Now+.5;break;}
            Check(TEXT("early_baked_twenty_tiles_and_basin_ready"),BakedTerrain45(J));Check(TEXT("early_runtime_depth_matches_actual_baked_meshes"),ProfileMatches45(B,J));
            Check(TEXT("actual_weather_ready"),Weather&&Weather->IsReady());R->Report->SetObjectField(TEXT("early_terrain"),J);
            R->Report->SetObjectField(TEXT("actual_lighting"),FrontlineLightingV42::DescribeLiveReflectionBindingsV44(B));
            Check(TEXT("quality_sr_rr_and_no_frame_generation"),Render&&Render->GetQuality()==EFrontlineRenderQuality::Quality&&Render->GetRayReconstruction()&&Render->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            R->Sample=FSample45();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Next=R->Last+8;R->Stage=2;break;
        }
        case 2:
            R->Sampling=false;R->Report->SetObjectField(TEXT("fixed_early_field_performance"),R->Sample.Describe());
            Check(TEXT("eight_second_normal_sample_stable_real_squads"),R->Sample.Seconds>=7.9&&R->Sample.GPU.Num()>30&&Living45(B)==R->Living);
            Shot(TEXT("01-early-british-wet-normal-6000"));R->Stage=3;R->Next=Now+.6;break;
        case 3:Camera(0,0,3400,350);R->Stage=4;R->Next=Now+1;break;
        case 4:Shot(TEXT("02-early-british-wet-zoom-3400"));R->Stage=41;R->Next=Now+.5;break;
        case 41:Check(TEXT("isolated_legacy_water_AB_enabled"),FFrontlineReviewV45::LegacyWater(B,1));R->Stage=42;R->Next=Now+.8;break;
        case 42:Shot(TEXT("02b-legacy-vertex-water-diagnostic"));R->Stage=43;R->Next=Now+.5;break;
        case 43:Check(TEXT("production_uses_only_basin_water_surface"),FFrontlineReviewV45::LegacyWater(B,0));Camera(0,0,9000,650);R->Stage=5;R->Next=Now+1;break;
        case 5:Shot(TEXT("03-early-british-wet-wide-9000"));Camera(0,FrontlineTerrainV42::CameraVerticalLimitV43(9000,float(R->W)/R->H),9000,650);R->Stage=6;R->Next=Now+1;break;
        case 6:Shot(TEXT("04-wide-positive-vertical-limit"));Camera(0,-FrontlineTerrainV42::CameraVerticalLimitV43(9000,float(R->W)/R->H),9000,650);R->Stage=7;R->Next=Now+1;break;
        case 7:Shot(TEXT("05-wide-negative-vertical-limit"));R->Stage=8;R->Next=Now+.6;break;
        case 8:
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::Chinese);FrontlineHUDV45::BeginAudit();R->Audit=true;
            Check(TEXT("german_middle_wet_field"),FFrontlineReviewV45::Field(B,EFrontlineFaction::German,5));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);
            FrontlineLightingV42::PrepareReviewWeather(B);R->Stage=9;R->Next=FPlatformTime::Seconds()+2;break;
        case 9:
            R->Report->SetObjectField(TEXT("german_terrain"),Terrain45(B));Check(TEXT("german_depth_matches_actual_baked_meshes"),ProfileMatches45(B,Terrain45(B)));
            Shot(TEXT("06-german-chinese-hud"));R->Stage=10;R->Next=Now+.6;break;
        case 10:
            Check(TEXT("late_guide_operation_prepared"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::German,19));
            FrontlineReviewUIV41::Deployment(PC);FrontlineReviewUIV41::Pointer(FVector2D(510,390));R->Stage=11;R->Next=Now+.75;break;
        case 11:Shot(TEXT("07-chinese-unit-tooltip"));R->Stage=110;R->Next=Now+.4;break;
        case 110:FrontlineReviewUIV41::Pointer(FVector2D(641,648));R->Stage=12;R->Next=Now+.75;break;
        case 12:Shot(TEXT("08-chinese-support-tooltip"));R->Stage=120;R->Next=Now+.4;break;
        case 120:FrontlineReviewUIV41::Technology(PC,1,24);FrontlineReviewUIV41::Pointer(FVector2D(486,752.5));R->Stage=13;R->Next=Now+.75;break;
        case 13:
        {
            Shot(TEXT("09-chinese-technology-tooltip"));const auto J=FrontlineHUDV45::EndAudit();R->Audit=false;R->Report->SetObjectField(TEXT("chinese_HUD_audit"),J);
            Check(TEXT("chinese_HUD_and_three_tooltips_actual_draw_pass"),J->GetBoolField(TEXT("success")));R->Stage=14;R->Next=Now+.6;break;
        }
        case 14:
            Check(TEXT("early_field_for_changed_motion"),FFrontlineReviewV45::Field(B,EFrontlineFaction::British,0));FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Pointer(FVector2D(1850,400));
            FrontlineMotionV41::PrepareActionReview(B,1);R->ActionMode=1;Camera(0,0,2800,100);R->Stage=15;R->Next=Now+.05;break;
        case 15:
            if(FrontlineMotionV41::ActionReviewState(B)->GetNumberField(TEXT("review_game_seconds"))<3.3){R->Next=Now+.02;break;}
            Shot(TEXT("10-trench-waiting-and-earthwork-contact"));R->ActionMode=-1;R->Stage=16;R->Next=Now+.5;break;
        case 16:FFrontlineReviewV45::StartOutcome(B);R->ActionMode=2;R->Phase=0;R->Stage=17;R->Next=Now+.02;break;
        case 17:
        {
            const float Times[]={.35f,1.80f,7.30f};const auto J=FrontlineMotionV41::ActionReviewState(B);
            if(J->GetNumberField(TEXT("review_game_seconds"))<Times[R->Phase]){R->Next=Now+.02;break;}
            // The result sheet otherwise hides the second motion cycle. This
            // affects only the isolated capture, never a player's ending.
            if(R->Phase==2)PC->GetHUD()->bShowHUD=false;
            Shot(FString::Printf(TEXT("%02d-outcome-%.2f-seconds"),11+R->Phase,Times[R->Phase]));++R->Phase;
            if(R->Phase==3){R->Stage=18;R->Next=Now+.1;}else R->Next=Now+.02;break;
        }
        case 18:
            PC->GetHUD()->bShowHUD=true;
            if(Audio&&Audio->DescribeOutcomeV45()->GetNumberField(TEXT("age_seconds"))<13.1){R->Next=Now+.1;break;}
            if(Audio){const auto J=Audio->DescribeOutcomeV45();R->Report->SetObjectField(TEXT("outcome_after_tails"),J);Check(TEXT("actual_response_six_admissions_and_no_missing_clips"),J->GetNumberField(TEXT("played"))==6&&J->GetNumberField(TEXT("missing"))==0);}
            R->ActionMode=-1;Check(TEXT("wet_field_for_fx_stress"),FFrontlineReviewV45::Field(B,EFrontlineFaction::British,5));FrontlineReviewUIV41::Clear(PC);
            if(Audio)Check(TEXT("actual_redeployment_cancels_outcome_queue"),!Audio->DescribeOutcomeV45()->GetBoolField(TEXT("pending"))&&Audio->DescribeOutcomeV45()->GetNumberField(TEXT("played"))==0);
            Camera(0,150,5000,180);R->Phase=0;R->Stage=19;R->Next=FPlatformTime::Seconds()+1;break;
        case 19:
        {
            const float Ages[]={.03f,.12f,.35f,1.f};Check(FString::Printf(TEXT("three_production_blast_profiles_at_%.2f"),Ages[R->Phase]),FrontlineImpactV40::SetupVisualFixture(B,Ages[R->Phase]));
            R->Stage=20;R->Next=Now+.25;break;
        }
        case 20:
        {
            const float Ages[]={.03f,.12f,.35f,1.f};Shot(FString::Printf(TEXT("%02d-grenade-mortar-shell-%.2f"),14+R->Phase,Ages[R->Phase]));
            const auto J=Scene45(B);Check(FString::Printf(TEXT("finite_nonobstructing_blast_phase_%d"),R->Phase),J->GetNumberField(TEXT("invalid_transforms"))==0&&J->GetNumberField(TEXT("secondary_obstruction_proxies"))==0);
            ++R->Phase;R->Stage=R->Phase<4?19:21;R->Next=Now+.45;break;
        }
        case 21:
        {
            for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)It->Destroy();
            auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();R->Gas=Gas;const FVector Target(0,300,AWarfareFrontlineGround::HeightAt(0,300));
            Check(TEXT("actual_committed_gas_mission"),Gas&&Gas->Configure(B,EFrontlineSupport::Gas,Target,0));
            B->Elapsed+=B->FireSupportDelayRange(EFrontlineSupport::Gas,Target,0).Y+6.f;if(Gas)Gas->Tick(.01f);
            Camera(0,0,6000,350);R->Stress=true;R->StageStart=Now;R->NextBlast=Now;R->Stage=22;R->Next=Now+2;break;
        }
        case 22:R->Sample=FSample45();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Stage=23;R->Next=R->Last+8;break;
        case 23:
        {
            R->Sampling=R->Stress=false;const auto Perf=R->Sample.Describe();Perf->SetStringField(TEXT("scope"),TEXT("Eight-second live six-cloud gas mission, periodic production blast actors and smooth horizontal/vertical camera pans. Actual delayed whole-frame GPU/CPU timings, no screenshot readbacks during interval. Includes game-scale overdraw/streaming; not a per-pass profiler or pure FX subtraction."));
            R->Report->SetObjectField(TEXT("gas_blast_pan_performance"),Perf);const auto J=Scene45(B);R->Report->SetObjectField(TEXT("stress_actual_scene"),J);
            Check(TEXT("stress_has_real_volume_proxies_without_RT_DF_blocks_or_invalid_transforms"),J->GetNumberField(TEXT("visible_volume_proxies"))>=6&&J->GetNumberField(TEXT("secondary_obstruction_proxies"))==0&&J->GetNumberField(TEXT("invalid_transforms"))==0);
            Check(TEXT("eight_second_active_fx_sample"),R->Sample.Seconds>=7.9&&R->Sample.GPU.Num()>30);Shot(TEXT("18-live-gas-blasts-and-camera-pan"));R->Stage=24;R->Next=Now+.8;break;
        }
        default:Finish();return false;
        }
        return true;
    }));
}

