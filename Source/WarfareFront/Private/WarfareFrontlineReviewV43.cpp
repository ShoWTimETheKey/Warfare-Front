#include "WarfareFrontlineReviewV43.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGameplayV43.h"
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
bool ReviewShadersPending43()
{
#if WITH_EDITOR
    return GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
#else
    return false;
#endif
}
bool Allowed43(const AWarfareFrontlineGameMode* B)
{
    FString Dir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
TSharedRef<FJsonObject> Distribution43(TArray<double> Values)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());J->SetBoolField(TEXT("available"),!Values.IsEmpty());
    if(Values.IsEmpty())return J;double Sum=0;for(double V:Values)Sum+=V;Values.Sort();
    J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());J->SetNumberField(TEXT("max_ms"),Values.Last());
    auto Percentile=[&](double P){return Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*P)-1,0,Values.Num()-1)];};
    J->SetNumberField(TEXT("p50_ms"),Percentile(.50));J->SetNumberField(TEXT("p95_ms"),Percentile(.95));J->SetNumberField(TEXT("p99_ms"),Percentile(.99));
    int32 Over33=0,Over50=0;for(double V:Values){Over33+=V>33.3;Over50+=V>50.;}J->SetNumberField(TEXT("over_33_3_ms"),Over33);J->SetNumberField(TEXT("over_50_ms"),Over50);return J;
}
struct FSample43
{
    TArray<double> Wall,GPU,Game,Render,RHI;double Seconds=0;int32 Unfocused=0;
    TSharedRef<FJsonObject> Describe()const
    {
        auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Distribution43(Wall));J->SetObjectField(TEXT("gpu"),Distribution43(GPU));
        J->SetObjectField(TEXT("game_thread"),Distribution43(Game));J->SetObjectField(TEXT("render_thread"),Distribution43(Render));J->SetObjectField(TEXT("rhi_thread"),Distribution43(RHI));
        J->SetNumberField(TEXT("sample_seconds"),Seconds);J->SetNumberField(TEXT("render_fps"),Seconds>0?Wall.Num()/Seconds:0);J->SetNumberField(TEXT("unfocused_frames"),Unfocused);
        J->SetStringField(TEXT("scope"),TEXT("One eight-second fixed normal wet view: six real friendly squads, active soldier/weather ticks, held combat director. Whole-frame delayed RHI GPU timer and actual game/render/RHI timers; no screenshot readback during sampling. Not a full-combat benchmark or per-pass GPU profile."));return J;
    }
};
TSharedRef<FJsonObject> Terrain43(AWarfareFrontlineGameMode* B)
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
bool BakedTerrain43(const TSharedRef<FJsonObject>& J)
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
TSharedRef<FJsonObject> Render43(AWarfareFrontlineGameMode* B)
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
int32 Living43(const AWarfareFrontlineGameMode* B)
{int32 Count=0;for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->IsAlive())++Count;return Count;}
}

struct FFrontlineReviewV43
{
    static bool Guide(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,int32 Mode)
    {
        if(!Allowed43(B)||!PC||Mode<0||Mode>2)return false;
        if(Mode==0)
        {
            B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid(43,19,0,1);R.Faction=EFrontlineFaction::British;
            R.Difficulty=EFrontlineDifficulty::Veteran;R.CompletedMissions=R.SelectedMission=19;
            R.Research.TechnologyMask=(1u<<27)-1;R.Research.ResearchPoints=17;
            B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=R.Faction;B->ResetEncounter();B->SetActorTickEnabled(false);
        }
        if(Mode<2)
        {
            FrontlineReviewUIV41::Deployment(PC);
            // Actual deployment hit rectangles: British Lewis card and mortar.
            FrontlineReviewUIV41::Pointer(Mode==0?FVector2D(510,390):FVector2D(641,648));
        }
        else
        {
            FrontlineReviewUIV41::Technology(PC,1,24);FrontlineReviewUIV41::Pointer(FVector2D(486,752.5));
        }
        return B->Phase==EFrontlinePhase::Briefing&&B->GetSelectedCampaignMission()&&B->GetSelectedCampaignMission()->Id==19;
    }
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F)
    {
        if(!Allowed43(B))return false;
        B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid(43,1917,5,uint32(F)+1);
        R.Faction=F;R.Difficulty=EFrontlineDifficulty::Veteran;R.CompletedMissions=R.SelectedMission=5;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->ResetEncounter();B->BeginBattle();
        B->SetBattleSpeed(1);B->Elapsed=20;B->SetActorTickEnabled(false);B->NextSquadId=1;
        B->Random.Initialize(431917+int32(F));B->EnemyDecisionRandom.Initialize(43+int32(F));
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
    static bool Movement(AWarfareFrontlineGameMode* B,TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FVector>& Starts)
    {
        if(!Field(B,EFrontlineFaction::British))return false;Starts.Empty();
        for(auto& S:B->Squads)S.bHoldPosition=false;
        const bool Ordered=B->AdvanceTrench(FrontlineBattlefield::CentreTrench);
        for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))
        {U->SetActorTickEnabled(false);Starts.Add(U,U->Feet());}
        return Ordered&&!Starts.IsEmpty();
    }
    static bool Combat(AWarfareFrontlineGameMode* B)
    {
        if(!Field(B,EFrontlineFaction::British))return false;
        const int32 Enemy=FrontlineBattlefield::CentreTrench+1;B->Trenches[Enemy].Owner=1;bool Good=true;
        for(int32 I=0;I<3;++I)Good&=B->SpawnSquad(1,I==1?EFrontlineRole::Lewis:EFrontlineRole::Rifle,Enemy);
        for(auto& S:B->Squads)S.bHoldPosition=false;
        Good&=B->AdvanceTrench(FrontlineBattlefield::CentreTrench,0);Good&=B->AdvanceTrench(Enemy,1);
        // Only readiness is opened for the fixture. Actual order admission,
        // response delays, scatter, shell flights and impacts remain production.
        B->ArtilleryReadyAt[0]=B->Elapsed;Good&=B->RequestArtillery(FVector(B->Trenches[Enemy].X,0,0),0);
        B->SetActorTickEnabled(true);return Good;
    }
};

namespace
{
TSharedRef<FJsonObject> Movement43(AWarfareFrontlineGameMode* B,float Seconds,const TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FVector>& Starts)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("game_seconds"),Seconds);TArray<TSharedPtr<FJsonValue>> Rows;
    bool Finite=true;int32 Moving=0;double Total=0,Minimum=MAX_dbl;
    for(const auto& Entry:Starts)if(const auto* U=Entry.Key.Get())
    {
        const FVector P=U->Feet();const double D=FVector::Dist2D(P,Entry.Value);Finite&=!P.ContainsNaN()&&FMath::IsFinite(D)&&FMath::IsFinite(U->GetMotionSpeed());
        Moving+=U->bMoving?1:0;Total+=D;auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("actor"),U->GetPathName());
        Row->SetStringField(TEXT("feet"),P.ToString());Row->SetStringField(TEXT("destination"),B->SoldierDestination(U).ToString());
        Row->SetNumberField(TEXT("distance_from_start_cm"),D);Row->SetNumberField(TEXT("actual_motion_speed_cm_s"),U->GetMotionSpeed());
        Row->SetNumberField(TEXT("motion_transitions"),U->MotionTransitions);Row->SetBoolField(TEXT("moving"),U->bMoving);Rows.Add(MakeShared<FJsonValueObject>(Row));
        for(const auto& Other:Starts)if(const auto* V=Other.Key.Get();V&&V!=U)Minimum=FMath::Min(Minimum,double(FVector::Dist2D(P,V->Feet())));
    }
    J->SetArrayField(TEXT("actors"),Rows);J->SetBoolField(TEXT("finite"),Finite&&!Rows.IsEmpty());J->SetNumberField(TEXT("moving"),Moving);
    J->SetNumberField(TEXT("minimum_pair_separation_cm"),Minimum==MAX_dbl?0:Minimum);J->SetNumberField(TEXT("total_displacement_cm"),Total);return J;
}

TSharedRef<FJsonObject> GraphicsState43(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
    J->SetStringField(TEXT("adapter"),GRHIAdapterName);J->SetStringField(TEXT("rhi"),GDynamicRHI?GDynamicRHI->GetName():TEXT("unavailable"));
    J->SetNumberField(TEXT("feature_level"),int32(B->GetWorld()->GetFeatureLevel()));J->SetBoolField(TEXT("integrated_device"),GRHIDeviceIsIntegrated);
    if(Render){J->SetNumberField(TEXT("preset"),int32(Render->GetGraphicsPreset()));J->SetObjectField(TEXT("render_diagnostics"),Render->GetDiagnostics());}
    J->SetNumberField(TEXT("effects_budget"),UFrontlineRenderSettingsSubsystem::EffectsBudgetV43());
    auto CV=MakeShared<FJsonObject>();for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("t.MaxFPS"),TEXT("r.VSync"),TEXT("r.DynamicGlobalIlluminationMethod"),TEXT("r.ReflectionMethod"),
        TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.ShadowQuality"),TEXT("r.Shadow.MaxResolution"),
        TEXT("r.Shadow.CSM.MaxCascades"),TEXT("r.SSR.Quality"),TEXT("r.VolumetricFog"),TEXT("r.VolumetricFog.GridPixelSize"),TEXT("r.BloomQuality"),TEXT("r.MotionBlurQuality"),
        TEXT("r.MaxAnisotropy"),TEXT("r.Streaming.MipBias"),TEXT("r.Streaming.PoolSize"),TEXT("r.SkeletalMeshLODBias"),TEXT("r.MaterialQualityLevel"),TEXT("r.Nanite")})
        if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))CV->SetStringField(Name,V->GetString());
    J->SetObjectField(TEXT("actual_graphics_cvars"),CV);FTextureMemoryStats M;RHIGetTextureMemoryStats(M);auto VRAM=MakeShared<FJsonObject>();
    VRAM->SetNumberField(TEXT("dedicated_video_bytes"),double(M.DedicatedVideoMemory));VRAM->SetNumberField(TEXT("shared_system_bytes"),double(M.SharedSystemMemory));
    VRAM->SetNumberField(TEXT("total_graphics_budget_bytes"),double(M.TotalGraphicsMemory));VRAM->SetNumberField(TEXT("streaming_texture_bytes"),double(M.StreamingMemorySize));
    VRAM->SetNumberField(TEXT("nonstreaming_texture_bytes"),double(M.NonStreamingMemorySize));VRAM->SetNumberField(TEXT("texture_pool_bytes"),double(M.TexturePoolSize));
    VRAM->SetStringField(TEXT("source"),TEXT("Actual RHI FTextureMemoryStats; -1 hardware fields mean unavailable. Texture allocations are not total process VRAM."));J->SetObjectField(TEXT("rhi_memory"),VRAM);
    TArray<TSharedPtr<FJsonValue>> LOD;for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->Soldier)
    {auto L=MakeShared<FJsonObject>();L->SetStringField(TEXT("actor"),U->GetPathName());L->SetNumberField(TEXT("actual_predicted_lod"),U->Soldier->GetPredictedLODLevel());LOD.Add(MakeShared<FJsonValueObject>(L));}
    J->SetArrayField(TEXT("actual_skeletal_lods"),LOD);J->SetObjectField(TEXT("actual_terrain"),Terrain43(B));return J;
}

void StartGraphics43(AWarfareFrontlineGameMode* Battle)
{
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> B;FString Dir;int32 Stage=0,Tier=0,W=1920,H=1080,Living=0;bool Passed=true,Sampling=false,Combat=false,Recruit=false;
        double Start=FPlatformTime::Seconds(),Next=Start+2,Last=Start,StageStart=Start;float CombatStart=0;
        FVector Focus=FVector(0,0,650);TArray<EFrontlineGraphicsPreset> Tiers;FSample43 Sample;
        TArray<TSharedPtr<FJsonValue>> Checks,Results;TArray<FString> Shots;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->B=Battle;R->Tiers={EFrontlineGraphicsPreset::High,EFrontlineGraphicsPreset::Medium,EFrontlineGraphicsPreset::Low};
    FString Preset;if(FParse::Value(FCommandLine::Get(),TEXT("WFV43GraphicsPreset="),Preset))
    {
        if(Preset.Equals(TEXT("High"),ESearchCase::IgnoreCase))R->Tiers={EFrontlineGraphicsPreset::High};
        else if(Preset.Equals(TEXT("Medium"),ESearchCase::IgnoreCase))R->Tiers={EFrontlineGraphicsPreset::Medium};
        else if(Preset.Equals(TEXT("Low"),ESearchCase::IgnoreCase))R->Tiers={EFrontlineGraphicsPreset::Low};
    }
    FParse::Value(FCommandLine::Get(),TEXT("ResX="),R->W);FParse::Value(FCommandLine::Get(),TEXT("ResY="),R->H);
    R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V43/GraphicsReview"));IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();if(!B)return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC||!PC->GetHUD())return Now-R->Start<25;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const FString& Name){const FString P=R->Dir/(Name+TEXT(".png"));R->Shots.Add(P);FScreenshotRequest::RequestScreenshot(P,true,false);};
        auto Finish=[&]()
        {
            B->SetActorTickEnabled(false);int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("requested_actual_viewport"),W==R->W&&H==R->H);
            bool Files=R->Shots.Num()==R->Tiers.Num()+2;for(const auto& P:R->Shots)Files&=IFileManager::Get().FileSize(*P)>1024;Check(TEXT("graphics_and_settings_captures_written"),Files);
            const auto UI=FrontlineReviewUIV41::EndAudit();R->Report->SetObjectField(TEXT("actual_UI_layout"),UI);
            bool Fits=false;UI->TryGetBoolField(TEXT("success"),Fits);Check(TEXT("graphics_UI_text_and_images_fit"),Fits);
            Check(TEXT("localized_coverage_report_written"),FrontlineLocalizationV35::WriteCoverageReport());
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("tiers"),R->Results);
            R->Report->SetNumberField(TEXT("width"),W);R->Report->SetNumberField(TEXT("height"),H);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),TEXT("Graphics-only run: no gameplay/guide/audio historical checks. Same six-squad wet field, native 100% rendering and FG off for every tier, four-second transition settling and eight-second samples. A separate 18-second actual combat/recruit/artillery/continuous-camera interval measures hitches. RHI adapter and driver path are reported; this machine does not validate RTX 3060 or integrated-GPU performance."));
            FString S;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(R->Dir/TEXT("graphics-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Start>150){Check(TEXT("bounded_graphics_timeout"),false);Finish();return false;}
        if(R->Combat)
        {
            const float T=B->Elapsed-R->CombatStart,Span=FMath::Max(0.f,FrontlineBattlefield::GroundHalfLength-3300.f);
            R->Focus=FVector(FMath::Sin(T*.30f)*Span,FMath::Sin(T*.45f)*FrontlineTerrainV42::CameraVerticalLimitV43(6000,float(R->W)/R->H)*.65f,650);
            FrontlineReviewUIV41::Camera(PC,R->Focus.X,R->Focus.Y,6000,650,false);
            if(!R->Recruit&&T>1.f){R->Recruit=true;Check(TEXT("actual_mid_sample_recruit_order"),B->Recruit(EFrontlineRole::Rifle,0));}
        }
        else if(R->Stage>0&&B->Phase==EFrontlinePhase::Fighting&&!B->IsActorTickEnabled())B->Elapsed+=FMath::Clamp(DT,0.f,.1f);
        if(R->Stage>0&&PC->BattleCamera)FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),R->Focus,6000,R->W);
        if(R->Sampling)
        {
            const double Ms=(Now-R->Last)*1000.;R->Last=Now;if(Ms>0){R->Sample.Wall.Add(Ms);R->Sample.Seconds+=Ms*.001;}
            auto Add=[](TArray<double>& A,double V){if(FMath::IsFinite(V)&&V>0)A.Add(V);};
            Add(R->Sample.GPU,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(R->Sample.Game,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(R->Sample.Render,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(R->Sample.RHI,FPlatformTime::ToMilliseconds(GRHIThreadTime));if(!FApp::HasFocus())++R->Sample.Unfocused;
        }
        if(ReviewShadersPending43()&&!R->Sampling)
        {R->Start+=DT;R->Next=Now+4;R->StageStart=Now;return true;}
        if(Now<R->Next)return true;auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        switch(R->Stage)
        {
        case 0:
            Check(TEXT("isolated_graphics_profile"),Allowed43(B));FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);FrontlineReviewUIV41::BeginAudit();
            FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(PC,false);Check(TEXT("same_real_wet_field"),FFrontlineReviewV43::Field(B,EFrontlineFaction::British));FrontlineReviewUIV41::Clear(PC);
            if(!Render){Check(TEXT("render_subsystem_exists"),false);Finish();return false;}
            Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);Render->SetRayReconstruction(false,false);Render->SetFrameRateSlider(1,false);
            if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))V->Set(0,ECVF_SetByCode);
            FrontlineReviewUIV41::Camera(PC,0,0,6000,650);R->Living=Living43(B);R->Stage=1;R->Next=Now+.1;break;
        case 1:
            Check(FString::Printf(TEXT("set_graphics_tier_%d"),int32(R->Tiers[R->Tier])),Render&&Render->SetGraphicsPreset(R->Tiers[R->Tier],false));
            if(Render){Render->SetQuality(EFrontlineRenderQuality::Native,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);}
            B->Elapsed=20;FrontlineLightingV42::PrepareReviewWeather(B);FrontlineReviewUIV41::Camera(PC,0,0,6000,650);
            R->StageStart=FPlatformTime::Seconds();R->Stage=2;R->Next=R->StageStart+4;break;
        case 2:
        {
            const auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());
            if(R->Tier==0&&(!Weather||!Weather->IsReady())&&Now-R->StageStart<30){R->Next=Now+1;break;}
            Check(FString::Printf(TEXT("tier_%d_native_fg_off"),int32(R->Tiers[R->Tier])),Render&&Render->GetGraphicsPreset()==R->Tiers[R->Tier]&&Render->GetQuality()==EFrontlineRenderQuality::Native
                &&Render->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off&&Render->GetDiagnostics()->GetNumberField(TEXT("screen_percentage"))>=99.9);
            R->Sample=FSample43();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Next=R->Last+8;R->Stage=3;break;
        }
        case 3:
        {
            R->Sampling=false;auto J=GraphicsState43(B);auto P=R->Sample.Describe();P->SetStringField(TEXT("scope"),TEXT("Same fixed wet field and six friendly squads, eight seconds, native rendering, FG off. No screenshot work inside sample."));
            J->SetObjectField(TEXT("performance"),P);J->SetNumberField(TEXT("settle_seconds"),R->Last-R->StageStart-R->Sample.Seconds);R->Results.Add(MakeShared<FJsonValueObject>(J));
            Check(FString::Printf(TEXT("tier_%d_sample_and_stable_composition"),int32(R->Tiers[R->Tier])),R->Sample.Seconds>=7.9&&R->Sample.GPU.Num()>30&&Living43(B)==R->Living&&B->ShotsFired==0);
            const TCHAR* Name=R->Tiers[R->Tier]==EFrontlineGraphicsPreset::High?TEXT("high"):R->Tiers[R->Tier]==EFrontlineGraphicsPreset::Medium?TEXT("medium"):TEXT("low");
            Shot(FString::Printf(TEXT("%02d-%s-native-wet"),R->Tier+1,Name));++R->Tier;R->Stage=R->Tier<R->Tiers.Num()?1:4;R->Next=Now+.8;break;
        }
        case 4:
            // Compare hitches on the requested single tier, or High when all
            // three tiers are compared. Do not quietly benchmark Low as High.
            if(Render){Render->SetGraphicsPreset(R->Tiers.Num()==1?R->Tiers[0]:EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);}
            FrontlineReviewUIV41::Clear(PC);Check(TEXT("actual_combat_and_artillery_orders"),FFrontlineReviewV43::Combat(B));FrontlineReviewUIV41::Clear(PC);
            FrontlineReviewUIV41::Camera(PC,0,0,6000,650);R->Focus=FVector(0,0,650);R->Stage=5;R->Next=FPlatformTime::Seconds()+4;break;
        case 5:
            R->Combat=true;R->CombatStart=B->Elapsed;R->Sample=FSample43();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Next=R->Last+18;R->Stage=6;break;
        case 6:
        {
            R->Sampling=R->Combat=false;auto J=GraphicsState43(B);auto P=R->Sample.Describe();P->SetStringField(TEXT("scope"),TEXT("18-second actual combat with one player reinforcement order, real committed artillery and smooth camera traverse. Includes first-use/streaming spikes after warmup; no screenshot work inside sample."));
            J->SetObjectField(TEXT("performance"),P);J->SetNumberField(TEXT("shots_fired"),B->ShotsFired);J->SetNumberField(TEXT("artillery_orders"),B->ArtilleryCommands);J->SetNumberField(TEXT("recruit_orders"),B->RecruitCommands);
            J->SetNumberField(TEXT("casualties"),B->Casualties[0]+B->Casualties[1]);R->Report->SetObjectField(TEXT("combat_hitches"),J);
            Check(TEXT("actual_combat_sample_has_fire_and_artillery"),R->Sample.Seconds>=17.9&&B->ShotsFired>0&&B->ArtilleryCommands>0&&R->Recruit);
            B->SetActorTickEnabled(false);Shot(TEXT("04-actual-combat-traverse"));R->Stage=7;R->Next=Now+.8;break;
        }
        case 7:
            FrontlineReviewUIV41::Clear(PC);PC->ToggleSettings();if(auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD()))HUD->NotifyHitBoxClick(TEXT("SettingsImage"));
            FrontlineReviewUIV41::Pointer(FVector2D(1420,900));R->Stage=8;R->Next=Now+1;break;
        case 8:Shot(TEXT("05-production-graphics-settings"));R->Stage=9;R->Next=Now+1;break;
        default:Finish();return false;
        }
        return true;
    }));
}
}

void StartFrontlineV43Review(AWarfareFrontlineGameMode* Battle)
{
    if(!Allowed43(Battle))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV43GraphicsReview"))){StartGraphics43(Battle);return;}
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;FString Directory;int32 Stage=0,View=0,Living=0,StreamingViews=0;
        bool Passed=true,Sampling=false,Movement=false,DiagnosticOptionsApplied=false;double Start=FPlatformTime::Seconds(),Next=Start+2,StageStart=Start,Last=Start;
        FVector Focus=FVector(0,0,650);float Width=6000,MovementSeconds=0;FSample43 Samples;
        TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FVector> MovementStarts;
        TArray<TSharedPtr<FJsonValue>> Checks,Views;TArray<FString> Shots;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->Battle=Battle;R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V43/Review"));
    IFileManager::Get().MakeDirectory(*R->Directory,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->Battle.Get();const double Now=FPlatformTime::Seconds();if(!B)return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC||!PC->GetHUD())return Now-R->Start<25;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Camera=[&](float X,float Y,float Width,float Z){R->Focus=FVector(X,Y,Z);R->Width=Width;FrontlineReviewUIV41::Camera(PC,X,Y,Width,Z);};
        auto Shot=[&](const FString& Name)
        {
            const FString Path=R->Directory/(Name+TEXT(".png"));R->Shots.Add(Path);FScreenshotRequest::RequestScreenshot(Path,true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("capture"),Name);J->SetStringField(TEXT("focus"),R->Focus.ToString());J->SetNumberField(TEXT("width_cm"),R->Width);
            J->SetNumberField(TEXT("battle_seconds"),B->Elapsed);J->SetNumberField(TEXT("faction"),int32(B->GetPlayerFaction()));
            if(PC->BattleCamera){J->SetStringField(TEXT("actual_camera"),PC->BattleCamera->GetActorTransform().ToString());J->SetNumberField(TEXT("actual_ortho_width"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);}
            R->Views.Add(MakeShared<FJsonValueObject>(J));
        };
        auto Finish=[&]()
        {
            R->Sampling=R->Movement=false;int32 W=0,H=0;PC->GetViewportSize(W,H);Check(TEXT("actual_3840x2160_viewport"),W==3840&&H==2160);
            const bool FirstViewOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV44FirstViewOnly"));
            bool Written=R->Shots.Num()==(FirstViewOnly?1:13);for(const auto& P:R->Shots)Written&=IFileManager::Get().FileSize(*P)>1024;Check(FirstViewOnly?TEXT("one_diagnostic_capture_written"):TEXT("thirteen_focused_captures_written"),Written);
            const auto UI=FrontlineReviewUIV41::EndAudit();R->Report->SetObjectField(TEXT("actual_UI_layout"),UI);bool Fits=false;UI->TryGetBoolField(TEXT("success"),Fits);Check(TEXT("reviewed_UI_text_and_images_fit"),Fits);
            Check(TEXT("localized_coverage_report_written"),FrontlineLocalizationV35::WriteCoverageReport());
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("captures"),R->Views);
            R->Report->SetNumberField(TEXT("width"),W);R->Report->SetNumberField(TEXT("height"),H);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetBoolField(TEXT("first_view_only_diagnostic"),FirstViewOnly);
            R->Report->SetStringField(TEXT("scope"),FirstViewOnly?
                TEXT("First-view renderer diagnostic only: initial production checks, one wet 4K Quality SR FG-off eight-second sample, actual reflection bindings, and one normal-view capture. Requested RR/trace/binding overrides and effective readbacks are recorded. Later UI, faction, movement and projectile views were not run; this is not thirteen-view acceptance. Isolated volatile saves."):
                TEXT("V43-only production gameplay, guide, audio-bank and committed-projectile checks; actual baked terrain representation; one wet 4K Quality SR FG-off eight-second sample (RR enabled unless the explicit isolated diagnostic disables it); normal/zoom/vertical boundary and faction HUD views; three actual tooltips; two movement phases and two frozen terminal-projectile instants. Isolated volatile saves. No historical suites or path-tracer acceptance claim. Visual and audio artistic acceptance remain human review tasks."));
            FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Start>150){Check(TEXT("bounded_review_timeout"),false);Finish();return false;}
        if(R->Stage>0)
        {
            int32 W=0,H=0;PC->GetViewportSize(W,H);const auto* C=PC->BattleCamera?PC->BattleCamera->GetCameraComponent():nullptr;
            if(C){FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),R->Focus,C->OrthoWidth,W);++R->StreamingViews;}
        }
        if(R->Movement)
        {
            const float Step=FMath::Clamp(DT,0.f,.05f);R->MovementSeconds+=Step;B->Elapsed+=Step;
            for(const auto& Entry:R->MovementStarts)if(auto* U=Entry.Key.Get())U->Tick(Step);
        }
        else if(B->Phase==EFrontlinePhase::Fighting&&R->Stage>0)B->Elapsed+=FMath::Clamp(DT,0.f,.1f);
        if(R->Sampling)
        {
            const double Ms=(Now-R->Last)*1000.;R->Last=Now;if(Ms>0){R->Samples.Wall.Add(Ms);R->Samples.Seconds+=Ms*.001;}
            auto Add=[](TArray<double>& V,double X){if(FMath::IsFinite(X)&&X>0)V.Add(X);};
            Add(R->Samples.GPU,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(R->Samples.Game,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(R->Samples.Render,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(R->Samples.RHI,FPlatformTime::ToMilliseconds(GRHIThreadTime));if(!FApp::HasFocus())++R->Samples.Unfocused;
        }
        if(ReviewShadersPending43()&&!R->Sampling)
        {R->Start+=DT;R->Next=Now+4;R->StageStart=Now;return true;}
        if(Now<R->Next)return true;
        switch(R->Stage)
        {
        case 0:
        {
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);FrontlineReviewUIV41::BeginAudit();Check(TEXT("isolated_volatile_profile"),Allowed43(B));
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
            {Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);Render->SetFrameRateSlider(1,false);}
            if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))V->Set(0,ECVF_SetByCode);
            if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())A->RunV44AudioChecks(Check);else Check(TEXT("audio_subsystem_available"),false);
            FrontlineGameplayV43::RunFocusedChecks(B,Check);FrontlinePlayerGuideV43::RunChecks(B,Check);FrontlineSupportArtV43::RunChecks(B,Check);
            FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(PC,false);Check(TEXT("british_wet_production_scene"),FFrontlineReviewV43::Field(B,EFrontlineFaction::British));
            FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);Check(TEXT("authored_wet_weather_prepared"),FrontlineLightingV42::PrepareReviewWeather(B));
            R->Living=Living43(B);R->Stage=1;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+8;break;
        }
        case 1:
        {
            const auto Terrain=Terrain43(B);const auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());const bool Ready=BakedTerrain43(Terrain)&&Weather&&Weather->IsReady();
            if(!Ready&&Now-R->StageStart<35){R->Next=Now+1;break;}
            Check(TEXT("wet_field_baked_cards_distance_fields_nanite_ready"),BakedTerrain43(Terrain));Check(TEXT("wet_weather_ready"),Weather&&Weather->IsReady());
            // Shipping disables arbitrary ExecCmds. Explicit, volatile-review
            // switches must set and read back the actual diagnostic control.
            if(!R->DiagnosticOptionsApplied)
            {
            bool Changed=false;
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV44NoReflectionScreenTraces")))
            {
                Changed=true;
                auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.ScreenTraces"));
                if(V)V->SetWithCurrentPriority(0);
                Check(TEXT("requested_reflection_screen_trace_diagnostic_effective"),V&&V->GetInt()==0);
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV44ConventionalTerrain")))
            {
                Changed=true;
                auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite"));
                if(V)V->SetWithCurrentPriority(0);
                Check(TEXT("requested_conventional_terrain_diagnostic_effective"),V&&V->GetInt()==0);
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV44RefreshReflectionBindings")))
            {
                Changed=true;int32 Count=0;
                for(TActorIterator<AActor> It(B->GetWorld());It;++It)
                {
                    if(It->IsHidden())continue;TInlineComponentArray<UStaticMeshComponent*> Components(*It);
                    for(auto* C:Components)if(C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->bRenderInMainPass&&C->GetStaticMesh())
                    {C->RecreateRenderState_Concurrent();++Count;}
                }
                R->Report->SetNumberField(TEXT("visible_static_render_states_refreshed_v44"),Count);
                Check(TEXT("requested_reflection_binding_refresh_performed_once"),Count>0);
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV44SurfaceCacheReflections")))
            {
                Changed=true;auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing.LightingMode"));
                if(V)V->SetWithCurrentPriority(0);
                R->Report->SetNumberField(TEXT("surface_cache_reflection_lighting_mode_v44"),V?V->GetInt():-1);
                Check(TEXT("requested_surface_cache_reflections_effective"),V&&V->GetInt()==0);
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV44NoRayReconstruction")))
            {
                Changed=true;auto* Settings=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
                if(Settings)Settings->SetRayReconstruction(false,false);
                Check(TEXT("requested_no_ray_reconstruction_applied"),Settings&&!Settings->GetRayReconstruction());
            }
            R->DiagnosticOptionsApplied=true;
            // Let render-state replacements and temporal-upscaler transitions
            // settle before the fixed eight-second timing interval.
            if(Changed){R->Next=FPlatformTime::Seconds()+4;return true;}
            }
            const auto Lighting=FrontlineLightingV42::Describe(B);
            Check(TEXT("backdrop_keeps_primary_view_without_blocking_reflection_scene"),Lighting->GetBoolField(TEXT("backdrop_primary_only_flags_v44")));
            R->Report->SetObjectField(TEXT("wet_actual_terrain"),Terrain);R->Report->SetObjectField(TEXT("wet_lighting"),Lighting);R->Report->SetNumberField(TEXT("warmup_seconds"),Now-R->StageStart);
            const auto Render=Render43(B);R->Report->SetObjectField(TEXT("render_contract"),Render);
            const bool NoRR=FParse::Param(FCommandLine::Get(),TEXT("WFV44NoRayReconstruction"));
            Check(NoRR?TEXT("quality_sr_rr_disabled_fg_off_diagnostic"):TEXT("quality_sr_rr_fg_off"),Render->GetBoolField(TEXT("fg_off_requested_and_effective"))&&Render->GetBoolField(TEXT("quality_sr"))&&
                (NoRR?(!Render->GetBoolField(TEXT("rr_requested"))&&!Render->GetBoolField(TEXT("rr_enabled_api"))):Render->GetBoolField(TEXT("rr_requested"))));
            R->Samples=FSample43();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Next=R->Last+8;R->Stage=2;break;
        }
        case 2:
            R->Sampling=false;R->Report->SetObjectField(TEXT("wet_normal_4k_performance"),R->Samples.Describe());
            Check(TEXT("one_eight_second_gpu_and_cpu_sample"),R->Samples.Seconds>=7.9&&R->Samples.GPU.Num()>=60&&R->Samples.Game.Num()>=60);
            Check(TEXT("sample_composition_stayed_constant_without_combat"),Living43(B)==R->Living&&B->ShotsFired==0);
            R->Report->SetObjectField(TEXT("live_reflection_bindings_v44"),FrontlineLightingV42::DescribeLiveReflectionBindingsV44(B));
            R->Report->SetObjectField(TEXT("render_contract_at_capture"),Render43(B));
            Shot(TEXT("01-british-wet-normal-6000"));R->Stage=3;R->Next=Now+.7;break;
        case 3:
            if(FParse::Param(FCommandLine::Get(),TEXT("WFV44FirstViewOnly"))){R->Report->SetNumberField(TEXT("production_streaming_view_submissions"),R->StreamingViews);Finish();return false;}
            Camera(0,0,3400,650);R->Stage=4;R->Next=Now+2;break;
        case 4:Shot(TEXT("02-british-wet-zoom-3400"));R->Stage=5;R->Next=Now+.7;break;
        case 5:Camera(0,0,9000,650);R->Stage=6;R->Next=Now+2;break;
        case 6:Shot(TEXT("03-british-wet-wide-9000"));R->Stage=7;R->Next=Now+.7;break;
        case 7:Camera(0,FrontlineTerrainV42::CameraVerticalLimitV43(6000),6000,650);R->Stage=8;R->Next=Now+2;break;
        case 8:Shot(TEXT("04-british-wet-upper-pan-limit"));R->Stage=9;R->Next=Now+.7;break;
        case 9:Camera(0,-FrontlineTerrainV42::CameraVerticalLimitV43(6000),6000,650);R->Stage=10;R->Next=Now+2;break;
        case 10:Shot(TEXT("05-british-wet-lower-pan-limit"));R->Stage=11;R->Next=Now+.7;break;
        case 11:
            FrontlineReviewUIV41::Clear(PC);Check(TEXT("german_wet_production_scene"),FFrontlineReviewV43::Field(B,EFrontlineFaction::German));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);
            Check(TEXT("german_authored_weather_prepared"),FrontlineLightingV42::PrepareReviewWeather(B));R->Stage=12;R->Next=Now+4;break;
        case 12:
            R->Report->SetObjectField(TEXT("german_actual_terrain"),Terrain43(B));Check(TEXT("german_same_strength_morale_fixture"),B->Morale[0]==72&&B->Morale[1]==72);
            Shot(TEXT("06-german-normal-morale-symmetry"));R->Stage=13;R->Next=Now+.7;break;
        case 13:
            Check(TEXT("unit_guide_fixture"),FFrontlineReviewV43::Guide(B,PC,0));R->Stage=14;R->Next=Now+1.1;break;
        case 14:Shot(TEXT("07-unit-guide-tooltip"));R->Stage=15;R->Next=Now+.7;break;
        case 15:Check(TEXT("support_guide_fixture"),FFrontlineReviewV43::Guide(B,PC,1));R->Stage=16;R->Next=Now+1.1;break;
        case 16:Shot(TEXT("08-support-guide-tooltip"));R->Stage=17;R->Next=Now+.7;break;
        case 17:Check(TEXT("technology_guide_fixture"),FFrontlineReviewV43::Guide(B,PC,2));R->Stage=18;R->Next=Now+1.1;break;
        case 18:Shot(TEXT("09-technology-guide-tooltip"));R->Stage=19;R->Next=Now+.7;break;
        case 19:
            FrontlineReviewUIV41::Clear(PC);Check(TEXT("actual_movement_fixture"),FFrontlineReviewV43::Movement(B,R->MovementStarts));FrontlineReviewUIV41::Clear(PC);
            R->Movement=true;R->MovementSeconds=0;Camera(500,0,3400,120);R->Stage=20;R->Next=Now+.05;break;
        case 20:
        {
            const auto Movement=Movement43(B,R->MovementSeconds,R->MovementStarts);double Time=0;Movement->TryGetNumberField(TEXT("game_seconds"),Time);
            if(Time<.5){R->Next=Now+.05;break;}R->Report->SetObjectField(TEXT("movement_half_second"),Movement);Shot(TEXT("10-movement-half-second"));R->Stage=21;R->Next=Now+.05;break;
        }
        case 21:
        {
            const auto Movement=Movement43(B,R->MovementSeconds,R->MovementStarts);double Time=0;Movement->TryGetNumberField(TEXT("game_seconds"),Time);
            if(Time<3.5){R->Next=Now+.05;break;}R->Report->SetObjectField(TEXT("movement_three_seconds"),Movement);
            Check(TEXT("real_advance_actors_finite_and_progressing"),Movement->GetBoolField(TEXT("finite"))&&Movement->GetNumberField(TEXT("moving"))>0&&Movement->GetNumberField(TEXT("total_displacement_cm"))>100);
            Shot(TEXT("11-movement-three-seconds"));R->Movement=false;R->Stage=22;R->Next=Now+.7;break;
        }
        case 22:
            Camera(0,70,4200,1050);Check(TEXT("terminal_projectile_180ms_fixture"),FrontlineSupportArtV43::SetupTerminalFixture(B,.18f));R->Stage=23;R->Next=Now+.6;break;
        case 23:Shot(TEXT("12-terminal-shells-180ms-before-impact"));R->Stage=24;R->Next=Now+.7;break;
        case 24:Check(TEXT("terminal_projectile_35ms_fixture"),FrontlineSupportArtV43::SetupTerminalFixture(B,.035f));R->Stage=25;R->Next=Now+.6;break;
        case 25:Shot(TEXT("13-terminal-shells-35ms-before-impact"));R->Stage=26;R->Next=Now+.9;break;
        default:R->Report->SetNumberField(TEXT("production_streaming_view_submissions"),R->StreamingViews);Finish();return false;
        }
        return true;
    }));
}
