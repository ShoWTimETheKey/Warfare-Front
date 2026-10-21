#include "WarfareFrontlineReviewV46.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineBalanceV46.h"
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
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/HUD.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "Materials/Material.h"
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
bool ReviewShadersPending46()
{
#if WITH_EDITOR
    return GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
#else
    return false;
#endif
}
bool Allowed46(const AWarfareFrontlineGameMode* B)
{
    FString Dir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV46Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
TSharedRef<FJsonObject> Distribution46(TArray<double> Values)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());J->SetBoolField(TEXT("available"),!Values.IsEmpty());
    if(Values.IsEmpty())return J;double Sum=0;for(double V:Values)Sum+=V;Values.Sort();
    J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());J->SetNumberField(TEXT("max_ms"),Values.Last());
    auto Percentile=[&](double P){return Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*P)-1,0,Values.Num()-1)];};
    J->SetNumberField(TEXT("p50_ms"),Percentile(.50));J->SetNumberField(TEXT("p95_ms"),Percentile(.95));J->SetNumberField(TEXT("p99_ms"),Percentile(.99));
    int32 Over33=0,Over50=0;for(double V:Values){Over33+=V>33.3;Over50+=V>50.;}J->SetNumberField(TEXT("over_33_3_ms"),Over33);J->SetNumberField(TEXT("over_50_ms"),Over50);return J;
}
struct FSample46
{
    TArray<double> Wall,GPU,Game,Render,RHI;double Seconds=0;int32 Unfocused=0;
    TSharedRef<FJsonObject> Describe()const
    {
        auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Distribution46(Wall));J->SetObjectField(TEXT("gpu"),Distribution46(GPU));
        J->SetObjectField(TEXT("game_thread"),Distribution46(Game));J->SetObjectField(TEXT("render_thread"),Distribution46(Render));J->SetObjectField(TEXT("rhi_thread"),Distribution46(RHI));
        J->SetNumberField(TEXT("sample_seconds"),Seconds);J->SetNumberField(TEXT("render_fps"),Seconds>0?Wall.Num()/Seconds:0);J->SetNumberField(TEXT("unfocused_frames"),Unfocused);
        J->SetStringField(TEXT("scope"),TEXT("One eight-second fixed normal wet view: six real friendly squads, active soldier/weather ticks, held combat director. Whole-frame delayed RHI GPU timer and actual game/render/RHI timers; no screenshot readback during sampling. Not a full-combat benchmark or per-pass GPU profile."));return J;
    }
};
TSharedRef<FJsonObject> Terrain46(AWarfareFrontlineGameMode* B)
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
bool BakedTerrain46(const TSharedRef<FJsonObject>& J)
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
        const auto Row=Value->AsObject();bool DF=false,Shader=false,FullSource=false;double Triangles=0,Cards=0,Clusters=0;
        if(!Row.IsValid()||!Row->TryGetBoolField(TEXT("distance_field"),DF)||!DF
            ||!Row->TryGetBoolField(TEXT("full_source_raster_v46"),FullSource)||!FullSource
            ||!Row->TryGetBoolField(TEXT("material_shader_valid"),Shader)||!Shader
            ||!Row->TryGetNumberField(TEXT("triangles"),Triangles)||Triangles<=0
            ||!Row->TryGetNumberField(TEXT("lumen_cards"),Cards)||Cards<=0
            ||!Row->TryGetNumberField(TEXT("nanite_clusters"),Clusters)||Clusters<=0)return false;
    }
    return true;
}
TSharedRef<FJsonObject> Render46(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
    J->SetBoolField(TEXT("fg_off_requested_and_effective"),R&&R->GetFrameGeneration()==EFrontlineFrameGeneration::Off&&R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
    J->SetBoolField(TEXT("quality_sr"),R&&R->GetQuality()==EFrontlineRenderQuality::Quality);J->SetBoolField(TEXT("rr_requested"),R&&R->GetRayReconstruction());
    J->SetBoolField(TEXT("no_rr_diagnostic_requested"),FParse::Param(FCommandLine::Get(),TEXT("WFV44NoRayReconstruction")));
    J->SetBoolField(TEXT("rr_enabled_api"),R&&R->GetDiagnostics()->GetBoolField(TEXT("rr_enabled_api")));
    if(R){J->SetObjectField(TEXT("runtime_diagnostics"),R->GetDiagnostics());J->SetNumberField(TEXT("production_frame_cap"),R->GetFrameRateLimit());}
    auto CV=MakeShared<FJsonObject>();for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("r.PathTracing"),TEXT("r.RayTracing"),TEXT("r.Nanite"),TEXT("r.Lumen.DiffuseIndirect.Allow")})
        if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))CV->SetStringField(Name,V->GetString());
    J->SetObjectField(TEXT("render_cvars"),CV);return J;
}
int32 Living46(const AWarfareFrontlineGameMode* B)
{int32 Count=0;for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->IsAlive())++Count;return Count;}
}

struct FFrontlineReviewV46
{
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Operation)
    {
        if(!Allowed46(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,Operation))return false;
        B->BeginBattle();B->SetBattleSpeed(1);B->Elapsed=20;B->SetActorTickEnabled(false);
        B->Random.Initialize(461917+int32(F));B->EnemyDecisionRandom.Initialize(46+int32(F));
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
        if(!Allowed46(B))return;
        FrontlineMotionV41::PrepareActionReview(B,2);
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        {Audio->SetBattleActive(false);Audio->PlayOutcomeV32(B->GetFactionForTeam(0)==EFrontlineFaction::German,FVector::ZeroVector);}
    }

};

namespace
{
TSharedRef<FJsonObject> Scene46(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();int32 Actors=0,Components=0,Volumes=0,Invalid=0,Secondary=0;
    TArray<TSharedPtr<FJsonValue>> Proxies,Impacts;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(!IsValid(*It)||It->IsActorBeingDestroyed())continue;++Actors;
        const FTransform T=It->GetActorTransform();if(T.ContainsNaN()||!T.IsValid())++Invalid;
        const auto* Blast=Cast<AWarfareFrontlineImpact>(*It);
        const bool Screen=Cast<AWarfareFrontlineSmoke>(*It)!=nullptr;
        const bool Gas=Cast<AFrontlineSupportMissionV28>(*It)!=nullptr;
        const bool FX=Blast||Gas||Screen;
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
            const FVector Scale=C->GetComponentScale();R->SetStringField(TEXT("scale"),Scale.ToString());
            R->SetStringField(TEXT("family"),Screen?TEXT("smoke"):Gas?TEXT("gas"):TEXT("blast"));
            R->SetNumberField(TEXT("depth_width_ratio"),Scale.Y/Scale.X);R->SetNumberField(TEXT("height_width_ratio"),Scale.Z/Scale.X);
            R->SetStringField(TEXT("base_material"),GetPathNameSafe(C->GetMaterial(0)?C->GetMaterial(0)->GetMaterial():nullptr));Proxies.Add(MakeShared<FJsonValueObject>(R));
        }
    }
    J->SetNumberField(TEXT("actors"),Actors);J->SetNumberField(TEXT("visible_fx_components"),Components);J->SetNumberField(TEXT("visible_volume_proxies"),Volumes);
    J->SetNumberField(TEXT("invalid_transforms"),Invalid);J->SetNumberField(TEXT("secondary_obstruction_proxies"),Secondary);
    J->SetArrayField(TEXT("actual_volume_proxies"),Proxies);J->SetArrayField(TEXT("actual_impacts"),Impacts);
    return J;
}
bool ProfileMatches46(AWarfareFrontlineGameMode* B,const TSharedRef<FJsonObject>& J)
{
    const auto* M=B->GetEncounterCampaignMission();if(!M)return false;
    const FString Depth=FString::Printf(TEXT("_D%d"),FMath::RoundToInt(FrontlineGameplayV33::OperationDepthScale(M->Id)*1000000.f));
    const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;if(!J->TryGetArrayField(TEXT("visible_tiles"),Tiles)||!Tiles||Tiles->Num()!=20)return false;
    for(const auto& V:*Tiles){const auto R=V->AsObject();FString Mesh;if(!R||!R->TryGetStringField(TEXT("mesh"),Mesh)||!Mesh.Contains(Depth)||!Mesh.StartsWith(TEXT("/Game/Warfare/Field/V46/Terrain/")))return false;}
    const TSharedPtr<FJsonObject>* Water=nullptr;FString Mesh;
    return J->TryGetObjectField(TEXT("basin_water_v44"),Water)&&Water&&(*Water)->TryGetStringField(TEXT("mesh"),Mesh)&&Mesh.Contains(Depth)&&Mesh.StartsWith(TEXT("/Game/Warfare/Field/V46/WaterR2/"));
}
}

void StartFrontlineV46Review(AWarfareFrontlineGameMode* Battle)
{
    if(!Allowed46(Battle))return;
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> B;
        TWeakObjectPtr<AFrontlineSupportMissionV28> Gas;
        TWeakObjectPtr<AWarfareFrontlineSmoke> Smoke;
        FString Dir;int32 Stage=0,ActionMode=-1,W=3840,H=2160;
        bool Passed=true,Sampling=false,Stress=false,Orbit=false,CameraQueued=false;
        double Start=FPlatformTime::Seconds(),Next=Start+2,Last=Start,StageStart=Start,NextBlast=0;
        FVector Focus=FVector(0,0,650),QueuedFocus=FVector::ZeroVector;float Width=6000,QueuedWidth=6000;
        FSample46 Sample;TArray<FString> Shots;TArray<TSharedPtr<FJsonValue>> Checks,Views;
        TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->B=Battle;FParse::Value(FCommandLine::Get(),TEXT("ResX="),R->W);FParse::Value(FCommandLine::Get(),TEXT("ResY="),R->H);
    R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V46/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
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
            if(FScreenshotRequest::IsScreenshotRequested()){R->QueuedFocus=FVector(X,Y,Z);R->QueuedWidth=Width;R->CameraQueued=true;return;}
            R->Focus=FVector(X,Y,Z);R->Width=Width;FrontlineReviewUIV41::Camera(PC,X,Y,Width,Z,Cut);
        };
        if(R->CameraQueued&&!FScreenshotRequest::IsScreenshotRequested())
        {const FVector F=R->QueuedFocus;R->CameraQueued=false;Camera(F.X,F.Y,R->QueuedWidth,F.Z);}
        auto Shot=[&](const FString& Name)
        {
            const FString Path=R->Dir/(Name+TEXT(".png"));R->Shots.Add(Path);FScreenshotRequest::RequestScreenshot(Path,true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("capture"),Name);J->SetNumberField(TEXT("width_cm"),R->Width);J->SetStringField(TEXT("focus"),R->Focus.ToString());
            J->SetNumberField(TEXT("battle_seconds"),B->Elapsed);J->SetNumberField(TEXT("faction"),int32(B->GetPlayerFaction()));J->SetNumberField(TEXT("active_trenches"),B->Trenches.Num());
            J->SetObjectField(TEXT("actual_scene"),Scene46(B));
            if(R->ActionMode>=0)J->SetObjectField(TEXT("actual_motion"),FrontlineMotionV41::ActionReviewState(B));
            if(auto* A=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())J->SetObjectField(TEXT("actual_outcome_audio"),A->DescribeOutcomeV45());
            if(PC->BattleCamera){J->SetStringField(TEXT("actual_camera"),PC->BattleCamera->GetActorTransform().ToString());J->SetNumberField(TEXT("actual_ortho_width"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);}
            R->Views.Add(MakeShared<FJsonValueObject>(J));
        };
        auto Geometry=[&](const TCHAR* Key,int32 Expected)
        {
            const auto J=Terrain46(B);R->Report->SetObjectField(Key,J);
            Check(FString(Key)+TEXT("_active_trench_and_centre_restored"),B->Trenches.Num()==Expected&&FrontlineBattlefield::TrenchCount==Expected&&FrontlineBattlefield::CentreTrench==Expected/2);
            Check(FString(Key)+TEXT("_actual_twenty_tiles_basin_and_depth_key"),BakedTerrain46(J)&&ProfileMatches46(B,J));
#if WITH_EDITOR
            bool Tangents=true;const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;
            if(!J->TryGetArrayField(TEXT("visible_tiles"),Tiles)||!Tiles||Tiles->Num()!=20)Tangents=false;
            else for(const auto& V:*Tiles){const auto T=V->AsObject();bool Explicit=false;double Position=0,Normal=0,Tangent=0;
                Tangents&=T&&T->TryGetBoolField(TEXT("explicit_tangents"),Explicit)&&Explicit&&T->TryGetNumberField(TEXT("position_precision"),Position)&&Position==4
                    &&T->TryGetNumberField(TEXT("normal_precision"),Normal)&&Normal==12&&T->TryGetNumberField(TEXT("tangent_precision"),Tangent)&&Tangent==10;}
            Check(FString(Key)+TEXT("_actual_nanite_explicit_tangent_precision"),Tangents);
#endif
            bool Bound=false;
            for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(IsValid(*It)&&!It->IsActorBeingDestroyed()&&It->Surface)
            {
                UMaterialInterface* M=It->Surface->GetMaterial(0);
                for(int32 Depth=0;M&&Depth<8;++Depth){if(M->GetName()==TEXT("MI_GroundMud_V46")){Bound=true;break;}auto* I=Cast<UMaterialInstance>(M);M=I?I->Parent.Get():nullptr;}
            }
            Check(FString(Key)+TEXT("_v46_ground_instance_really_bound"),Bound);
        };
        auto VolumeChecks=[&](const TCHAR* Key,bool AllFamilies)
        {
            const auto J=Scene46(B);R->Report->SetObjectField(Key,J);bool Shape=true,Material=true;int32 Smoke=0,Gas=0,Blast=0;
            for(const auto& V:J->GetArrayField(TEXT("actual_volume_proxies")))
            {
                const auto P=V->AsObject();const FString Family=P->GetStringField(TEXT("family"));const double Z=P->GetNumberField(TEXT("height_width_ratio")),Y=P->GetNumberField(TEXT("depth_width_ratio"));
                const bool Screen=Family==TEXT("smoke"),Poison=Family==TEXT("gas");Smoke+=Screen;Gas+=Poison;Blast+=!Screen&&!Poison;
                Shape&=Y>=.85&&Y<=.99&&Z>=(Poison?.61:.77)&&(!Screen||Z<=.93)&&(!Poison||Z<=.74);
                Material&=P->GetStringField(TEXT("base_material")).Contains(TEXT("/Game/Warfare/Field/V46/Materials/M_SmokeVolume_V46"));
            }
            Check(FString(Key)+TEXT("_actual_rounded_world_volumes_use_v46_material"),Shape&&Material&&Smoke>0&&Gas>0&&(!AllFamilies||Blast>0));
            Check(FString(Key)+TEXT("_finite_no_ray_shadow_or_distance_field_occlusion"),J->GetNumberField(TEXT("invalid_transforms"))==0&&J->GetNumberField(TEXT("secondary_obstruction_proxies"))==0);
        };
        auto Finish=[&]()
        {
            R->Sampling=R->Stress=false;PC->SetActorTickEnabled(true);B->SetActorTickEnabled(false);int32 W=0,H=0;PC->GetViewportSize(W,H);
            Check(TEXT("requested_actual_viewport"),W==R->W&&H==R->H);bool Written=R->Shots.Num()==10;
            for(const auto& P:R->Shots)Written&=IFileManager::Get().FileSize(*P)>1024;Check(TEXT("ten_changed_area_captures_written"),Written);
            const auto UI=FrontlineReviewUIV41::EndAudit();R->Report->SetObjectField(TEXT("actual_UI_layout"),UI);bool Fits=false;UI->TryGetBoolField(TEXT("success"),Fits);Check(TEXT("actual_UI_boxes_fit"),Fits);
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("captures"),R->Views);
            R->Report->SetObjectField(TEXT("final_render"),Render46(B));R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("rhi"),GDynamicRHI?GDynamicRHI->GetName():TEXT("unavailable"));R->Report->SetStringField(TEXT("adapter"),GRHIAdapterName);
            R->Report->SetNumberField(TEXT("width"),W);R->Report->SetNumberField(TEXT("height"),H);R->Report->SetBoolField(TEXT("audio_output_enabled"),!FParse::Param(FCommandLine::Get(),TEXT("nosound")));
            R->Report->SetStringField(TEXT("scope"),TEXT("V46 local focused review: real balance paths and MG crew ticks; British first / German final / British first 3-5-3 trench reloads and matching V46 terrain, water and explicit Nanite tangents; normal/zoom/wide game views; one actual outcome ensemble per faction; production smoke, gas and blast volumes from two camera azimuths; one eight-second live FX and camera-pan sample. Ten affected-area captures. No historical infantry/CJK/water A-B suite, no path-tracing or unmeasured 90 FPS claim. Audio admissions are not subjective listening. Isolated volatile UserDir, bounded 98 seconds."));
            FString S;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Start>98){Check(TEXT("bounded_ninety_eight_second_review"),false);Finish();return false;}
        const float Step=FMath::Clamp(DT,0.f,.05f);
        if(R->ActionMode>=0)FrontlineMotionV41::TickActionReview(B,Step);
        else if(R->Stage>0&&B->Phase==EFrontlinePhase::Fighting&&!B->IsActorTickEnabled())B->Elapsed+=Step;
        if(R->Stress)
        {
            const float T=float(Now-R->StageStart),Limit=FrontlineTerrainV42::CameraVerticalLimitV43(6000,float(R->W)/R->H);
            Camera(FMath::Sin(T*.53f)*650,FMath::Sin(T*.71f)*Limit*.75f,6000,350,false);
            if(Now>=R->NextBlast){R->NextBlast=Now+1.7;const int32 Kind=int32(T/1.7f)%3;const float X=-900+Kind*900;
                if(auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(FVector(X,400,AWarfareFrontlineGround::HeightAt(X,400)),FRotator::ZeroRotator))A->ConfigureBlast(Kind==0,Kind==1,FVector(.5,.2,-1));}
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
            if(ReviewShadersPending46()){R->Next=Now+.5;break;}
            Check(TEXT("isolated_volatile_review_profile"),Allowed46(B));FrontlineReviewUIV41::BeginAudit();FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(B,false);
            if(Render){Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetFrameRateSlider(1,false);}
            if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))V->Set(0,ECVF_SetByCode);
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
            FrontlineGameplayV33::RunDepthChecksV45(B,Check);if(Audio)Audio->RunV46OutcomeChecks(Check);else Check(TEXT("audio_subsystem_available"),false);
            Check(TEXT("late_operation_for_actual_balance_and_crew_checks"),FrontlineCampaignV41::PrepareReviewOperation(B,EFrontlineFaction::British,19));
            B->BeginBattle();B->SetActorTickEnabled(false);FrontlineBalanceV46::RunChecks(B,Check);R->Report->SetObjectField(TEXT("machinegun_crew"),FrontlineInfantryV45::RunCrewChecksV46(B,Check));
            Check(TEXT("first_british_field"),FFrontlineReviewV46::Field(B,EFrontlineFaction::British,0));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);
            Check(TEXT("actual_early_weather_prepared"),FrontlineLightingV42::PrepareReviewWeather(B));R->Stage=1;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+5;break;
        case 1:
        {
            const auto J=Terrain46(B);const auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());
            if((ReviewShadersPending46()||!BakedTerrain46(J)||!Weather||!Weather->IsReady())&&Now-R->StageStart<22){R->Next=Now+.5;break;}
            Geometry(TEXT("first_british_terrain"),3);Check(TEXT("actual_weather_ready"),Weather&&Weather->IsReady());R->Report->SetObjectField(TEXT("actual_lighting"),FrontlineLightingV42::DescribeLiveReflectionBindingsV44(B));
            Check(TEXT("quality_sr_rr_and_no_frame_generation"),Render&&Render->GetQuality()==EFrontlineRenderQuality::Quality&&Render->GetRayReconstruction()&&Render->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
            Shot(TEXT("01-first-three-trenches-normal-6000"));R->Stage=2;R->Next=Now+.5;break;
        }
        case 2:
        {
            FString Diagnostic;FParse::Value(FCommandLine::Get(),TEXT("WFV46GroundDiagnostic="),Diagnostic);
            if(!Diagnostic.IsEmpty())
            {
                const TCHAR* Name=Diagnostic==TEXT("NoNanite")?TEXT("r.Nanite"):Diagnostic==TEXT("NoDiffuseGI")?TEXT("r.Lumen.DiffuseIndirect.Allow"):nullptr;
                if(Name)if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))V->Set(0,ECVF_SetByConsole);
                R->Report->SetStringField(TEXT("isolated_ground_diagnostic"),Diagnostic);
                R->Stage=90;R->Next=Now+5;break;
            }
            Camera(0,0,3400,350);R->Stage=3;R->Next=Now+1;break;
        }
        case 90:Shot(TEXT("02-isolated-ground-diagnostic"));R->Stage=91;R->Next=Now+.5;break;
        case 91:
        {
            R->Report->SetObjectField(TEXT("final_render"),Render46(B));R->Report->SetArrayField(TEXT("captures"),R->Views);
            R->Report->SetStringField(TEXT("scope"),TEXT("Same running field and camera, before and after one isolated render CVar. Diagnostic only; not the full acceptance suite."));
            FString S;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FPlatformMisc::RequestExit(false);return false;
        }
        case 3:Shot(TEXT("02-first-ground-zoom-3400"));R->Stage=4;R->Next=Now+.5;break;
        case 4:Camera(0,0,9000,650);R->Stage=5;R->Next=Now+1;break;
        case 5:Shot(TEXT("03-first-ground-wide-9000"));R->Stage=6;R->Next=Now+.5;break;
        case 6:Camera(0,0,3000,250);FFrontlineReviewV46::StartOutcome(B);R->ActionMode=2;R->Stage=7;R->Next=Now+.05;break;
        case 7:
            if(FrontlineMotionV41::ActionReviewState(B)->GetNumberField(TEXT("review_game_seconds"))<1.80){R->Next=Now+.02;break;}
            Shot(TEXT("04-british-outcome-and-ensemble"));
            if(Audio){const auto J=Audio->DescribeOutcomeV45();Check(TEXT("actual_british_outcome_uses_one_complete_ensemble"),J->GetNumberField(TEXT("expected_ensemble_starts"))==1&&J->GetNumberField(TEXT("played"))==1&&J->GetNumberField(TEXT("missing"))==0);}
            R->ActionMode=-1;R->Stage=8;R->Next=Now+.5;break;
        case 8:
            Check(TEXT("final_german_field_reload"),FFrontlineReviewV46::Field(B,EFrontlineFaction::German,19));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);FrontlineLightingV42::PrepareReviewWeather(B);
            R->Stage=9;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+4;break;
        case 9:
            if((ReviewShadersPending46()||!BakedTerrain46(Terrain46(B)))&&Now-R->StageStart<15){R->Next=Now+.5;break;}
            Geometry(TEXT("final_german_terrain"),5);Shot(TEXT("05-final-five-trenches-restored"));R->Stage=10;R->Next=Now+.5;break;
        case 10:Camera(0,0,3000,250);FFrontlineReviewV46::StartOutcome(B);R->ActionMode=2;R->Stage=11;R->Next=Now+.05;break;
        case 11:
            if(FrontlineMotionV41::ActionReviewState(B)->GetNumberField(TEXT("review_game_seconds"))<1.80){R->Next=Now+.02;break;}
            Shot(TEXT("06-german-outcome-and-ensemble"));
            if(Audio){const auto J=Audio->DescribeOutcomeV45();Check(TEXT("actual_german_outcome_uses_one_complete_ensemble"),J->GetNumberField(TEXT("expected_ensemble_starts"))==1&&J->GetNumberField(TEXT("played"))==1&&J->GetNumberField(TEXT("missing"))==0);}
            R->ActionMode=-1;R->Stage=12;R->Next=Now+.5;break;
        case 12:
            Check(TEXT("first_british_reloaded_after_final_operation"),FFrontlineReviewV46::Field(B,EFrontlineFaction::British,0));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);FrontlineLightingV42::PrepareReviewWeather(B);
            R->Stage=13;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+3;break;
        case 13:
            if((ReviewShadersPending46()||!BakedTerrain46(Terrain46(B)))&&Now-R->StageStart<12){R->Next=Now+.5;break;}
            Geometry(TEXT("restored_first_british_terrain"),3);Shot(TEXT("07-three-trenches-and-bakes-restored"));R->Stage=14;R->Next=Now+.5;break;
        case 14:
        {
            auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();R->Gas=Gas;
            const FVector Target(1600,350,AWarfareFrontlineGround::HeightAt(1600,350));Check(TEXT("production_gas_mission_for_volume_review"),Gas&&Gas->Configure(B,EFrontlineSupport::Gas,Target,0));
            B->Elapsed+=B->FireSupportDelayRange(EFrontlineSupport::Gas,Target,0).Y+11.f;if(Gas)Gas->Tick(.01f);
            auto* Smoke=B->GetWorld()->SpawnActor<AWarfareFrontlineSmoke>(FVector(-1600,350,AWarfareFrontlineGround::HeightAt(-1600,350)),FRotator::ZeroRotator);R->Smoke=Smoke;
            Check(TEXT("production_six_volume_smoke_actor"),Smoke&&Smoke->bVisualReady);if(Smoke){Smoke->DeployAt=B->Elapsed-5.f;Smoke->Tick(.01f);}
            Check(TEXT("production_three_blast_profiles_for_volume_view"),FrontlineImpactV40::SetupVisualFixture(B,.8f));Camera(0,150,6000,350);R->Stage=15;R->Next=Now+.6;break;
        }
        case 15:VolumeChecks(TEXT("normal_volume_bindings"),true);Shot(TEXT("08-smoke-gas-blasts-normal-azimuth"));R->Stage=16;R->Next=Now+.5;break;
        case 16:
            // Actual camera orbit tests volume depth, not a second billboard
            // facing the same view. Restore the normal controller before play.
            if(PC->BattleCamera){PC->SetActorTickEnabled(false);const FVector Offset=PC->BattleCamera->GetActorLocation()-R->Focus;
                const FVector Location=R->Focus+FQuat(FVector::UpVector,FMath::DegreesToRadians(22.f)).RotateVector(Offset);
                PC->BattleCamera->SetActorLocationAndRotation(Location,(R->Focus-Location).Rotation());
                if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);R->Orbit=true;}
            Check(TEXT("second_volume_view_is_actual_camera_orbit"),R->Orbit);R->Stage=17;R->Next=Now+.8;break;
        case 17:VolumeChecks(TEXT("orbited_volume_bindings"),true);Shot(TEXT("09-smoke-gas-blasts-orbit-22-degrees"));R->Stage=18;R->Next=Now+.5;break;
        case 18:
            PC->SetActorTickEnabled(true);R->Orbit=false;Camera(0,0,6000,350);
            for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)It->Destroy();
            R->Stress=true;R->StageStart=Now;R->NextBlast=Now;R->Stage=19;R->Next=Now+1.2;break;
        case 19:R->Sample=FSample46();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Stage=20;R->Next=R->Last+8;break;
        case 20:
        {
            R->Sampling=R->Stress=false;const auto Perf=R->Sample.Describe();Perf->SetStringField(TEXT("scope"),TEXT("One eight-second live production smoke/gas, periodic grenade/mortar/shell actors and smooth horizontal/vertical camera pans. Ordinary soldier and component ticks, held combat director. Actual delayed whole-frame RHI GPU/CPU timers, no screenshot readbacks during sample. Not a per-pass profiler, pure FX subtraction or general gameplay benchmark."));
            R->Report->SetObjectField(TEXT("fx_pan_performance"),Perf);VolumeChecks(TEXT("stress_actual_scene"),false);Check(TEXT("eight_second_active_fx_sample"),R->Sample.Seconds>=7.9&&R->Sample.GPU.Num()>30);
            Shot(TEXT("10-live-volumetric-fx-stress"));R->Stage=21;R->Next=Now+.8;break;
        }
        default:Finish();return false;
        }
        return true;
    }));
}

