#include "WarfareFrontlineReviewV47.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineFXV47.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineReviewV41.h"
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
#include "Engine/Texture2D.h"
#include "GameFramework/HUD.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
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
#include "RenderingThread.h"
#include "Shader.h"
#include "SceneInterface.h"
#include "TextureResource.h"
#include "VertexFactory.h"
#include "UObject/StrongObjectPtr.h"
#include "RHI.h"
#include "RHIStats.h"
#include "PixelFormat.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace
{
bool ReviewShadersPending47()
{
#if WITH_EDITOR
    return GShaderCompilingManager&&GShaderCompilingManager->IsCompiling();
#else
    return false;
#endif
}
bool Allowed47(const AWarfareFrontlineGameMode* B)
{
    FString Dir;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
TSharedRef<FJsonObject> Distribution47(TArray<double> Values)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),Values.Num());J->SetBoolField(TEXT("available"),!Values.IsEmpty());
    if(Values.IsEmpty())return J;double Sum=0;for(double V:Values)Sum+=V;Values.Sort();
    J->SetNumberField(TEXT("mean_ms"),Sum/Values.Num());J->SetNumberField(TEXT("max_ms"),Values.Last());
    auto Percentile=[&](double P){return Values[FMath::Clamp(FMath::CeilToInt(Values.Num()*P)-1,0,Values.Num()-1)];};
    J->SetNumberField(TEXT("p50_ms"),Percentile(.50));J->SetNumberField(TEXT("p95_ms"),Percentile(.95));J->SetNumberField(TEXT("p99_ms"),Percentile(.99));
    int32 Over33=0,Over50=0;for(double V:Values){Over33+=V>33.3;Over50+=V>50.;}J->SetNumberField(TEXT("over_33_3_ms"),Over33);J->SetNumberField(TEXT("over_50_ms"),Over50);return J;
}
struct FSample47
{
    TArray<double> Wall,GPU,Game,Render,RHI;double Seconds=0;int32 Unfocused=0;
    TSharedRef<FJsonObject> Describe()const
    {
        auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Distribution47(Wall));J->SetObjectField(TEXT("gpu"),Distribution47(GPU));
        J->SetObjectField(TEXT("game_thread"),Distribution47(Game));J->SetObjectField(TEXT("render_thread"),Distribution47(Render));J->SetObjectField(TEXT("rhi_thread"),Distribution47(RHI));
        J->SetNumberField(TEXT("sample_seconds"),Seconds);J->SetNumberField(TEXT("render_fps"),Seconds>0?Wall.Num()/Seconds:0);J->SetNumberField(TEXT("unfocused_frames"),Unfocused);
        J->SetStringField(TEXT("scope"),TEXT("One eight-second fixed normal wet view: six real friendly squads, active soldier/weather ticks, held combat director. Whole-frame delayed RHI GPU timer and actual game/render/RHI timers; no screenshot readback during sampling. Not a full-combat benchmark or per-pass GPU profile."));return J;
    }
};
TSharedRef<FJsonObject> Terrain47(AWarfareFrontlineGameMode* B)
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
bool BakedTerrain47(const TSharedRef<FJsonObject>& J)
{
    const bool ExpectedFullSource=!FParse::Param(FCommandLine::Get(),TEXT("WFV47NaniteGround"));
    bool Complete=false;if(!J->TryGetBoolField(TEXT("baked_complete"),Complete)||!Complete)return false;
    const TSharedPtr<FJsonObject>* Water=nullptr;
    if(!J->TryGetObjectField(TEXT("basin_water_v44"),Water)||!Water||!Water->IsValid())return false;
    bool WaterCollision=false;double WaterTriangles=0;
    if(!(*Water)->TryGetBoolField(TEXT("collision_disabled"),WaterCollision)||!WaterCollision
        ||!(*Water)->TryGetNumberField(TEXT("triangles"),WaterTriangles)||WaterTriangles<=0)return false;
    const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;if(!J->TryGetArrayField(TEXT("visible_tiles"),Tiles)||!Tiles||Tiles->Num()!=20)return false;
    for(const auto& Value:*Tiles)
    {
        const auto Row=Value->AsObject();bool DF=false,FullSource=false;double Triangles=0,Cards=0,Clusters=0;
        if(!Row.IsValid()||!Row->TryGetBoolField(TEXT("distance_field"),DF)||!DF
            ||!Row->TryGetBoolField(TEXT("full_source_raster_v46"),FullSource)||FullSource!=ExpectedFullSource
            ||!Row->TryGetNumberField(TEXT("triangles"),Triangles)||Triangles<=0
            ||!Row->TryGetNumberField(TEXT("lumen_cards"),Cards)||Cards<=0
            ||!Row->TryGetNumberField(TEXT("nanite_clusters"),Clusters)||Clusters<=0)return false;
    }
    return true;
}
TSharedRef<FJsonObject> Render47(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
    J->SetBoolField(TEXT("fg_off_requested_and_effective"),R&&R->GetFrameGeneration()==EFrontlineFrameGeneration::Off&&R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
    J->SetBoolField(TEXT("quality_sr"),R&&R->GetQuality()==EFrontlineRenderQuality::Quality);J->SetBoolField(TEXT("rr_requested"),R&&R->GetRayReconstruction());
    J->SetBoolField(TEXT("no_rr_diagnostic_requested"),FParse::Param(FCommandLine::Get(),TEXT("WFV44NoRayReconstruction")));
    J->SetBoolField(TEXT("rr_enabled_api"),R&&R->GetDiagnostics()->GetBoolField(TEXT("rr_enabled_api")));
    if(R){J->SetObjectField(TEXT("runtime_diagnostics"),R->GetDiagnostics());J->SetNumberField(TEXT("production_frame_cap"),R->GetFrameRateLimit());}
    auto CV=MakeShared<FJsonObject>();for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("r.PathTracing"),TEXT("r.RayTracing"),TEXT("r.Nanite"),TEXT("r.Lumen.DiffuseIndirect.Allow"),TEXT("r.Fog"),TEXT("r.VolumetricFog")})
        if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))CV->SetStringField(Name,V->GetString());
    J->SetObjectField(TEXT("render_cvars"),CV);return J;
}
}

struct FFrontlineReviewV47
{
    static bool Field(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Operation)
    {
        if(!Allowed47(B)||!FrontlineCampaignV41::PrepareReviewOperation(B,F,Operation))return false;
        B->BeginBattle();B->SetBattleSpeed(1);B->Elapsed=20;B->SetActorTickEnabled(false);
        B->Random.Initialize(471917+int32(F));B->EnemyDecisionRandom.Initialize(47+int32(F));
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

};

namespace
{
TSharedRef<FJsonObject> Scene47(AWarfareFrontlineGameMode* B)
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
            R->SetStringField(TEXT("base_material"),GetPathNameSafe(C->GetMaterial(0)?C->GetMaterial(0)->GetMaterial():nullptr));
            UTexture* DensityAtlas=nullptr;
            if(C->GetMaterial(0)&&C->GetMaterial(0)->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityVolume")),DensityAtlas))
            {
                R->SetStringField(TEXT("density_atlas"),GetPathNameSafe(DensityAtlas));
                if(auto* Texture=Cast<UTexture2D>(DensityAtlas))
                {R->SetNumberField(TEXT("atlas_width"),Texture->GetSizeX());R->SetNumberField(TEXT("atlas_height"),Texture->GetSizeY());R->SetBoolField(TEXT("atlas_render_resource"),Texture->GetResource()!=nullptr);}
            }
            Proxies.Add(MakeShared<FJsonValueObject>(R));
        }
    }
    J->SetNumberField(TEXT("actors"),Actors);J->SetNumberField(TEXT("visible_fx_components"),Components);J->SetNumberField(TEXT("visible_volume_proxies"),Volumes);
    J->SetNumberField(TEXT("invalid_transforms"),Invalid);J->SetNumberField(TEXT("secondary_obstruction_proxies"),Secondary);
    J->SetArrayField(TEXT("actual_volume_proxies"),Proxies);J->SetArrayField(TEXT("actual_impacts"),Impacts);
    return J;
}
bool ProfileMatches47(AWarfareFrontlineGameMode* B,const TSharedRef<FJsonObject>& J)
{
    const auto* M=B->GetEncounterCampaignMission();if(!M)return false;
    const FString Depth=FString::Printf(TEXT("_D%d"),FMath::RoundToInt(FrontlineGameplayV33::OperationDepthScale(M->Id)*1000000.f));
    const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;if(!J->TryGetArrayField(TEXT("visible_tiles"),Tiles)||!Tiles||Tiles->Num()!=20)return false;
    for(const auto& V:*Tiles){const auto R=V->AsObject();FString Mesh;if(!R||!R->TryGetStringField(TEXT("mesh"),Mesh)||!Mesh.Contains(Depth)||!Mesh.StartsWith(TEXT("/Game/Warfare/Field/V47/Terrain/")))return false;}
    const TSharedPtr<FJsonObject>* Water=nullptr;FString Mesh;
    return J->TryGetObjectField(TEXT("basin_water_v44"),Water)&&Water&&(*Water)->TryGetStringField(TEXT("mesh"),Mesh)&&Mesh.Contains(Depth)&&Mesh.StartsWith(TEXT("/Game/Warfare/Field/V47/Water/"));
}

bool HasMaterial47(UMaterialInterface* Material,const TCHAR* ObjectPath)
{
    // Inspect the assigned dynamic-instance ancestry, not a merely loaded asset.
    for(int32 Depth=0;Material&&Depth<8;++Depth)
    {
        if(Material->GetPathName()==ObjectPath)return true;
        auto* Instance=Cast<UMaterialInstance>(Material);Material=Instance?Instance->Parent.Get():nullptr;
    }
    return false;
}

TArray<double> ContactSamples47()
{
    // These are the authoritative movement heights at nine protected frontage
    // positions. The optional 0 -> 19 -> 0 reload must restore them exactly.
    TArray<double> Values;
    for(float X:{-1400.f,0.f,1400.f})for(float Y:{-600.f,0.f,600.f})
        Values.Add(AWarfareFrontlineGround::HeightAt(X,Y));
    return Values;
}

TSharedRef<FJsonObject> ObserveAssignedRasterMaterials47(AWarfareFrontlineGameMode* B)
{
    struct FObserved
    {
        const FMaterialRenderProxy* Proxy=nullptr;
        FString Component,Assigned,Kind,RenderMaterial;
        bool Original=false,Valid=false,Complete=false,Finalized=false,Vertex=false,Pixel=false,SingleLayerWater=false;
        int32 ShaderCount=0,PipelineCount=0,Quality=INDEX_NONE,ShadingModelBits=0;
        TArray<FString> RasterShaders;
    };
    auto Batch=MakeShared<TArray<FObserved>,ESPMode::ThreadSafe>();TArray<TStrongObjectPtr<UMaterialInterface>> KeepAlive;
    auto Add=[&](UStaticMeshComponent* Component,const TCHAR* Kind)
    {
        UMaterialInterface* Material=Component?Component->GetMaterial(0):nullptr;if(!Material)return;
        KeepAlive.Emplace(Material);FObserved Row;Row.Proxy=Material->GetRenderProxy();Row.Component=Component->GetPathName();
        Row.Assigned=Material->GetPathName();Row.Kind=Kind;Batch->Add(MoveTemp(Row));
    };
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
    {
        if(!IsValid(*It)||It->IsActorBeingDestroyed())continue;
        if(!It->BakedTerrainV43.IsEmpty())Add(It->BakedTerrainV43[0].Get(),TEXT("ground"));
        Add(It->BasinWaterV44.Get(),TEXT("water"));break;
    }
    const auto Feature=B->GetWorld()->GetFeatureLevel();
    ENQUEUE_RENDER_COMMAND(FrontlineV47ObserveAssignedGroundAndWater)([Batch,Feature](FRHICommandListImmediate&)
    {
        for(auto& Row:*Batch)
        {
            // Deliberately never call GetMaterialWithFallback. The assigned
            // material must supply the local-VF base-pass VS and PS itself.
            const FMaterial* Material=Row.Proxy?Row.Proxy->GetMaterialNoFallback(Feature):nullptr;
            Row.Original=Material!=nullptr;if(!Material)continue;
            Row.RenderMaterial=Material->GetFriendlyName();Row.Quality=int32(Material->GetQualityLevel());
            Row.Complete=Material->IsRenderingThreadShaderMapComplete();
            Row.ShadingModelBits=int32(Material->GetShadingModels().GetShadingModelField());
            Row.SingleLayerWater=Material->GetShadingModels().HasOnlyShadingModel(MSM_SingleLayerWater);
            const auto* Map=Material->GetRenderingThreadShaderMap();Row.Valid=Map&&Map->IsValidForRendering();Row.Finalized=Map&&Map->IsCompilationFinalized();
            if(!Map)continue;Map->CountNumShaders(Row.ShaderCount,Row.PipelineCount);
            TMap<FShaderId,TShaderRef<FShader>> Shaders;Map->GetShaderList(Shaders);
            for(const auto& Pair:Shaders)
            {
                const auto& Id=Pair.Key;if(!Id.Type||!Id.VFType||!Pair.Value.IsValid())continue;
                const FString VF=Id.VFType->GetName(),Type=Id.Type->GetName();
                if(VF!=TEXT("FLocalVertexFactory"))continue;
                const bool VS=Type==TEXT("TBasePassVSFNoLightMapPolicy")&&Id.Type->GetFrequency()==SF_Vertex;
                const bool PS=Type.StartsWith(TEXT("TBasePassPSFNoLightMapPolicy"))&&Id.Type->GetFrequency()==SF_Pixel;
                if(!VS&&!PS)continue;Row.Vertex|=VS;Row.Pixel|=PS;
                Row.RasterShaders.Add(FString::Printf(TEXT("%s | %s | permutation=%d"),*Type,*VF,Id.PermutationId));
            }
        }
    });
    // A single bounded two-material batch, outside every timed interval.
    // Strong GT references remain alive until the original proxies are read.
    FlushRenderingCommands();
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=Batch->Num()==2;
    for(const auto& O:*Batch)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("kind"),O.Kind);Row->SetStringField(TEXT("component"),O.Component);
        Row->SetStringField(TEXT("assigned_material"),O.Assigned);Row->SetStringField(TEXT("original_render_material"),O.RenderMaterial);
        Row->SetBoolField(TEXT("original_no_fallback_present"),O.Original);Row->SetBoolField(TEXT("render_map_valid"),O.Valid);
        Row->SetBoolField(TEXT("render_map_complete"),O.Complete);Row->SetBoolField(TEXT("render_compilation_finalized"),O.Finalized);
        Row->SetNumberField(TEXT("render_shader_count"),O.ShaderCount);Row->SetNumberField(TEXT("render_pipeline_count"),O.PipelineCount);Row->SetNumberField(TEXT("quality"),O.Quality);
        Row->SetBoolField(TEXT("local_vf_no_lightmap_basepass_vs"),O.Vertex);Row->SetBoolField(TEXT("local_vf_no_lightmap_basepass_ps"),O.Pixel);
        Row->SetBoolField(TEXT("single_layer_water_only"),O.SingleLayerWater);Row->SetNumberField(TEXT("shading_model_bit_field"),O.ShadingModelBits);
        TArray<TSharedPtr<FJsonValue>> Names;for(const auto& Name:O.RasterShaders)Names.Add(MakeShared<FJsonValueString>(Name));Row->SetArrayField(TEXT("available_required_raster_types"),Names);
        const bool Ready=O.Original&&O.Valid&&O.ShaderCount>0&&O.Vertex&&O.Pixel&&(O.Kind!=TEXT("water")||O.SingleLayerWater);
        Row->SetBoolField(TEXT("ready"),Ready);Good&=Ready;Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("materials"),Rows);
    J->SetStringField(TEXT("scope"),TEXT("Two actual assigned visible terrain/water materials: original no-fallback frozen render map must contain local-vertex-factory no-lightmap base-pass vertex and pixel shader families. Water must report only SingleLayerWater. All twenty tile assignments are independently checked. GT/RT whole-map completion remains diagnostic, not substituted for pass shader availability. This is shader-resource evidence, not draw-command/GPU pixel correctness or visual acceptance."));
    return J;
}

TSharedRef<FJsonObject> GroundTextureResidency47(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();UMaterialInterface* Material=nullptr;
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
    {
        if(!IsValid(*It)||It->IsActorBeingDestroyed()||It->BakedTerrainV43.IsEmpty()||!It->BakedTerrainV43[0])continue;
        Material=It->BakedTerrainV43[0]->GetMaterial(0);J->SetStringField(TEXT("component"),It->BakedTerrainV43[0]->GetPathName());break;
    }
    J->SetStringField(TEXT("assigned_material"),GetPathNameSafe(Material));if(!Material)return J;
    const EShaderPlatform Platform=B->GetWorld()->Scene?B->GetWorld()->Scene->GetShaderPlatform():GMaxRHIShaderPlatform;
    auto Scalars=MakeShared<FJsonObject>();
    for(const TCHAR* Name:{TEXT("SoilProjectionScaleV47"),TEXT("WeatherWetness")})
    {
        float Value=0;const bool Available=Material->GetScalarParameterValue(FMaterialParameterInfo(Name),Value);
        auto S=MakeShared<FJsonObject>();S->SetBoolField(TEXT("available"),Available);if(Available)S->SetNumberField(TEXT("value"),Value);Scalars->SetObjectField(Name,S);
    }
    J->SetObjectField(TEXT("actual_assigned_scalar_parameters"),Scalars);
    TArray<TSharedPtr<FJsonValue>> Sampling;
    for(const auto& Entry:Material->GetTextureStreamingData())
    {
        auto S=MakeShared<FJsonObject>();S->SetStringField(TEXT("texture_name"),Entry.TextureName.ToString());
        S->SetNumberField(TEXT("sampling_scale"),Entry.SamplingScale);S->SetNumberField(TEXT("uv_channel"),Entry.UVChannelIndex);
        Sampling.Add(MakeShared<FJsonValueObject>(S));
    }
    J->SetArrayField(TEXT("actual_material_streaming_sampling_entries"),Sampling);
    const auto SamplingCache=Material->GetCachedTexturesSamplingInfo();
    J->SetBoolField(TEXT("actual_material_sampling_cache_present"),SamplingCache.IsValid());
    J->SetNumberField(TEXT("actual_material_sampling_cache_texture_count"),SamplingCache?SamplingCache->TextureSamplingInfos.Num():0);
    const auto* Resource=Material->GetMaterialResource(Platform);const auto* Map=Resource?Resource->GetGameThreadShaderMap():nullptr;
    J->SetBoolField(TEXT("game_thread_resource_present"),Resource!=nullptr);
    J->SetBoolField(TEXT("game_thread_whole_map_complete"),Resource&&Resource->IsGameThreadShaderMapComplete());
    J->SetBoolField(TEXT("game_thread_map_render_valid"),Map&&Map->IsValidForRendering());
    J->SetBoolField(TEXT("game_thread_map_finalized"),Map&&Map->IsCompilationFinalized());
    TArray<UTexture*> Used;Material->GetUsedTextures(Used,{},Platform);TSet<UTexture*> Unique;
    struct FRHIObservation
    {
        UTexture2D* Texture=nullptr;bool Ready=false;int32 Width=0,Height=0,Mips=0,Format=PF_Unknown;
    };
    auto Actual=MakeShared<TArray<FRHIObservation>,ESPMode::ThreadSafe>();TArray<TStrongObjectPtr<UTexture2D>> KeepAlive;
    TArray<TSharedPtr<FJsonValue>> Rows;
    UTexture2D* Supplemental=FindObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Field/V47/Textures/T_FlowDensity64_V47.T_FlowDensity64_V47"));
    if(Supplemental)Used.AddUnique(Supplemental);
    for(UTexture* Texture:Used)
    {
        auto* T=Cast<UTexture2D>(Texture);if(!T||Unique.Contains(Texture))continue;Unique.Add(Texture);KeepAlive.Emplace(T);
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("asset"),T->GetPathName());const FIntPoint Imported=T->GetImportedSize();
        Row->SetNumberField(TEXT("imported_source_width"),Imported.X);Row->SetNumberField(TEXT("imported_source_height"),Imported.Y);
        Row->SetNumberField(TEXT("platform_width"),T->GetSizeX());Row->SetNumberField(TEXT("platform_height"),T->GetSizeY());
        Row->SetNumberField(TEXT("total_platform_mips"),T->GetNumMips());Row->SetNumberField(TEXT("resident_mips"),T->GetNumResidentMips());
        Row->SetNumberField(TEXT("maximum_allowed_mips"),T->GetNumMipsAllowed(true));Row->SetNumberField(TEXT("cached_lod_bias"),T->GetCachedLODBias());
        Row->SetBoolField(TEXT("pending_streaming"),T->HasPendingInitOrStreaming());Row->SetBoolField(TEXT("fully_streamed_within_current_limits"),T->IsFullyStreamedIn());
        const auto& State=T->GetStreamableResourceState();Row->SetBoolField(TEXT("streamable_resource_state_valid"),State.IsValid());
        if(State.IsValid())
        {
            Row->SetNumberField(TEXT("requested_mips"),State.NumRequestedLODs);Row->SetNumberField(TEXT("resource_resident_mips"),State.NumResidentLODs);
            Row->SetNumberField(TEXT("resource_maximum_mips"),State.MaxNumLODs);Row->SetNumberField(TEXT("asset_lod_bias"),State.AssetLODBias);
            Row->SetNumberField(TEXT("lod_bias_modifier"),State.LODBiasModifier);Row->SetBoolField(TEXT("supports_streaming"),State.bSupportsStreaming);
        }
        const int32 FirstResident=FMath::Clamp(T->GetNumMips()-T->GetNumResidentMips(),0,30);
        Row->SetNumberField(TEXT("largest_resident_width"),T->GetNumResidentMips()>0?FMath::Max(1,T->GetSizeX()>>FirstResident):0);
        Row->SetNumberField(TEXT("largest_resident_height"),T->GetNumResidentMips()>0?FMath::Max(1,T->GetSizeY()>>FirstResident):0);
        FRHIObservation Observation;Observation.Texture=T;Actual->Add(Observation);Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    ENQUEUE_RENDER_COMMAND(FrontlineV47ObserveGroundTextureDescriptors)([Actual](FRHICommandListImmediate&)
    {
        for(auto& O:*Actual)
        {
            const auto* TextureResource=O.Texture?O.Texture->GetResource():nullptr;const auto* TextureRHI=TextureResource?TextureResource->GetTexture2DRHI():nullptr;
            O.Ready=TextureRHI!=nullptr;if(!TextureRHI)continue;const auto& Desc=TextureRHI->GetDesc();O.Width=Desc.Extent.X;O.Height=Desc.Extent.Y;O.Mips=Desc.NumMips;O.Format=int32(Desc.Format);
        }
    });
    // Descriptor reads only; no GPU image readback, streaming overrides or mip
    // forcing. Called at captures, never while collecting performance samples.
    FlushRenderingCommands();
    for(int32 I=0;I<Actual->Num();++I)
    {
        const auto& O=(*Actual)[I];const auto Row=Rows[I]->AsObject();Row->SetBoolField(TEXT("rhi_texture_present"),O.Ready);
        Row->SetNumberField(TEXT("rhi_resource_width"),O.Width);Row->SetNumberField(TEXT("rhi_resource_height"),O.Height);Row->SetNumberField(TEXT("rhi_resource_mips"),O.Mips);
        Row->SetNumberField(TEXT("rhi_pixel_format"),O.Format);Row->SetStringField(TEXT("rhi_pixel_format_name"),GPixelFormats[O.Format].Name);
    }
    for(int32 I=Rows.Num()-1;I>=0;--I)if((*Actual)[I].Texture==Supplemental){J->SetObjectField(TEXT("supplemental_fx_density_atlas_resource"),Rows[I]->AsObject());Rows.RemoveAt(I);}
    J->SetBoolField(TEXT("supplemental_fx_density_atlas_loaded"),Supplemental!=nullptr);
    J->SetArrayField(TEXT("used_texture2d_resources"),Rows);J->SetNumberField(TEXT("texture2d_count"),Rows.Num());
    J->SetStringField(TEXT("scope"),TEXT("Report-only actual assigned ground material at this camera. Imported size is UTexture2D metadata, platform dimensions are current asset dimensions, residency/request/limits are current streaming state; RHI allocation descriptors are read once on the render thread. No native-8K requirement or forced mip residency. RHI allocation can exceed current resident mips. Game-thread whole-map flags are retained diagnostics, distinct from the earlier no-fallback raster shader proof."));
    return J;
}
}

void StartFrontlineV47Review(AWarfareFrontlineGameMode* Battle)
{
    if(!Allowed47(Battle))return;
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> B;
        FString Dir;int32 Stage=0,W=3840,H=2160;
        bool Passed=true,Sampling=false,Stress=false,Orbit=false,CameraQueued=false,TerrainOnly=false,FinalProfiles=false;
        double Start=FPlatformTime::Seconds(),Next=Start+2,Last=Start,StageStart=Start,NextBlast=0,ExecutionStart=0;
        FVector Focus=FVector(0,0,650),QueuedFocus=FVector::ZeroVector;float Width=6000,QueuedWidth=6000,FixtureAge=0;
        FSample47 Sample;TArray<double> InitialContacts;
        TArray<FString> Shots;TArray<TSharedPtr<FJsonValue>> Checks,Views;
        TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    };
    auto R=MakeShared<FRun>();R->B=Battle;
    R->TerrainOnly=FParse::Param(FCommandLine::Get(),TEXT("WFV47TerrainOnly"));
    R->FinalProfiles=FParse::Param(FCommandLine::Get(),TEXT("WFV47FinalProfiles"));
    FParse::Value(FCommandLine::Get(),TEXT("ResX="),R->W);FParse::Value(FCommandLine::Get(),TEXT("ResY="),R->H);
    R->Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V47/Review"));IFileManager::Get().MakeDirectory(*R->Dir,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->B.Get();const double Now=FPlatformTime::Seconds();if(!B)return false;
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        if(!PC||!PC->GetHUD())
        {
            if(Now-R->Start<300)return true;
            R->Report->SetBoolField(TEXT("success"),false);R->Report->SetStringField(TEXT("error"),TEXT("No player controller/HUD within the 300-second startup budget"));
            FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("review.json")));FPlatformMisc::RequestExit(false);return false;
        }
        auto Camera=[&](float X,float Y,float Width,float Z,bool Cut=true)
        {
            if(FScreenshotRequest::IsScreenshotRequested()){R->QueuedFocus=FVector(X,Y,Z);R->QueuedWidth=Width;R->CameraQueued=true;return;}
            R->Focus=FVector(X,Y,Z);R->Width=Width;FrontlineReviewUIV41::Camera(PC,X,Y,Width,Z,Cut);
        };
        if(R->CameraQueued&&!FScreenshotRequest::IsScreenshotRequested())
        {const FVector Focus=R->QueuedFocus;R->CameraQueued=false;Camera(Focus.X,Focus.Y,R->QueuedWidth,Focus.Z);}
        auto Shot=[&](const FString& Name)
        {
            const FString Path=R->Dir/(Name+TEXT(".png"));R->Shots.Add(Path);FScreenshotRequest::RequestScreenshot(Path,true,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("capture"),Name);J->SetNumberField(TEXT("width_cm"),R->Width);J->SetStringField(TEXT("focus"),R->Focus.ToString());
            J->SetNumberField(TEXT("battle_seconds"),B->Elapsed);J->SetNumberField(TEXT("faction"),int32(B->GetPlayerFaction()));J->SetNumberField(TEXT("active_trenches"),B->Trenches.Num());
            J->SetObjectField(TEXT("actual_scene"),Scene47(B));
            if(R->Stage<=5||R->Stage==31||R->Stage==33)J->SetObjectField(TEXT("actual_ground_texture_residency"),GroundTextureResidency47(B));
            if(!R->TerrainOnly)J->SetObjectField(TEXT("actual_v47_fx"),FrontlineFXV47::Describe(B));
            if(PC->BattleCamera){J->SetStringField(TEXT("actual_camera"),PC->BattleCamera->GetActorTransform().ToString());J->SetNumberField(TEXT("actual_ortho_width"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);}
            R->Views.Add(MakeShared<FJsonValueObject>(J));
        };
        auto Geometry=[&](const TCHAR* Key,int32 Expected)
        {
            const auto J=Terrain47(B);R->Report->SetObjectField(Key,J);
            Check(FString(Key)+TEXT("_active_trench_count_and_centre"),B->Trenches.Num()==Expected&&FrontlineBattlefield::TrenchCount==Expected&&FrontlineBattlefield::CentreTrench==Expected/2);
            const bool NaniteDiagnostic=FParse::Param(FCommandLine::Get(),TEXT("WFV47NaniteGround"));
            J->SetBoolField(TEXT("expected_full_source_raster"),!NaniteDiagnostic);
            Check(FString(Key)+TEXT("_twenty_tiles_expected_raster_path_df_cards_and_exact_v47_depth_keys"),BakedTerrain47(J)&&ProfileMatches47(B,J));
            const auto Raster=ObserveAssignedRasterMaterials47(B);J->SetObjectField(TEXT("actual_assigned_raster_materials"),Raster);
            if(!NaniteDiagnostic)Check(FString(Key)+TEXT("_original_assigned_ground_water_have_required_raster_shaders"),Raster->GetBoolField(TEXT("success")));
            else
            {
                // The retained local-VF proof is not a Nanite compute/raster
                // proof. Record it without treating it as this path's approval.
                Raster->SetBoolField(TEXT("supports_selected_ground_render_path"),false);
                Raster->SetBoolField(TEXT("selected_ground_render_path_proven"),false);
                Raster->SetStringField(TEXT("diagnostic_limitation"),TEXT("Isolated Nanite-ground A/B: this local-vertex-factory observer does not certify the active Nanite ground shader path. Default production full-source checks are unchanged."));
                bool WaterReady=false;for(const auto& V:Raster->GetArrayField(TEXT("materials")))if(V->AsObject()->GetStringField(TEXT("kind"))==TEXT("water"))WaterReady=V->AsObject()->GetBoolField(TEXT("ready"));
                Check(FString(Key)+TEXT("_diagnostic_water_original_raster_shaders_available"),WaterReady);
            }
            bool GroundBound=false,WaterBound=false,CollisionPreserved=false;UMaterialInterface* ActualGround=nullptr;
            for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
            {
                if(!IsValid(*It)||It->IsActorBeingDestroyed()||!It->Surface)continue;
                GroundBound=HasMaterial47(It->Surface->GetMaterial(0),TEXT("/Game/Warfare/Field/V47/Materials/MI_GroundMud_V47.MI_GroundMud_V47"));
                bool TilesBound=It->BakedTerrainV43.Num()==20,TilesNonColliding=TilesBound;
                for(const auto& C:It->BakedTerrainV43)
                {
                    TilesBound&=C&&HasMaterial47(C->GetMaterial(0),TEXT("/Game/Warfare/Field/V47/Materials/MI_GroundMud_V47.MI_GroundMud_V47"));
                    TilesNonColliding&=C&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
                }
                GroundBound&=TilesBound;
                if(!It->BakedTerrainV43.IsEmpty()&&It->BakedTerrainV43[0])ActualGround=It->BakedTerrainV43[0]->GetMaterial(0);
                WaterBound=It->BasinWaterV44&&HasMaterial47(It->BasinWaterV44->GetMaterial(0),TEXT("/Game/Warfare/Field/V47/Materials/M_BasinWater_V47.M_BasinWater_V47"));
                CollisionPreserved=It->Surface->GetStaticMesh()&&It->Surface->GetCollisionEnabled()!=ECollisionEnabled::NoCollision&&TilesNonColliding
                    &&It->BasinWaterV44&&It->BasinWaterV44->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
            }
            Check(FString(Key)+TEXT("_new_ground_and_water_assigned_to_actual_components"),GroundBound&&WaterBound);
            TSet<FString> ActualScans;TArray<TSharedPtr<FJsonValue>> ScanAssets;
            if(ActualGround)
            {
                TArray<UTexture*> Used;ActualGround->GetUsedTextures(Used,{},B->GetWorld()->Scene?B->GetWorld()->Scene->GetShaderPlatform():GMaxRHIShaderPlatform);
                for(UTexture* Texture:Used)if(Cast<UTexture2D>(Texture))ActualScans.Add(Texture->GetPathName());
            }
            for(const FString& Path:ActualScans)ScanAssets.Add(MakeShared<FJsonValueString>(Path));
            J->SetArrayField(TEXT("actual_high_ground_scan_assets"),ScanAssets);
            bool CurrentScanSet=ActualScans.Num()==4;
            for(const TCHAR* Path:{TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_Albedo_V23.T_Mud_Albedo_V23"),
                TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_Normal_V23.T_Mud_Normal_V23"),
                TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_Roughness_V23.T_Mud_Roughness_V23"),
                TEXT("/Game/Warfare/Field/V47/Textures/T_Mud_HeightAO_V47.T_Mud_HeightAO_V47")})CurrentScanSet&=ActualScans.Contains(Path);
            Check(FString(Key)+TEXT("_actual_four_scan_set_uses_packed_height_and_original_ao"),CurrentScanSet);
            Check(FString(Key)+TEXT("_authoritative_contact_surface_retained_no_duplicate_visual_collision"),CollisionPreserved);
            bool Finite=true;TArray<TSharedPtr<FJsonValue>> ContactJSON;
            for(double Height:ContactSamples47()){Finite&=FMath::IsFinite(Height);ContactJSON.Add(MakeShared<FJsonValueNumber>(Height));}
            J->SetArrayField(TEXT("protected_frontage_height_samples_cm"),ContactJSON);Check(FString(Key)+TEXT("_protected_frontage_heights_finite"),Finite);
            const TSharedPtr<FJsonObject>* Basins=nullptr;bool SharedLevels=false;double Shores=0,Maximum=0,ContactCount=0,ContactChange=0;
            const bool BasinMetadata=J->TryGetObjectField(TEXT("basin_field_v47"),Basins)&&Basins&&(*Basins)->TryGetBoolField(TEXT("same_resolved_levels_for_surface_and_water"),SharedLevels)&&SharedLevels
                &&(*Basins)->TryGetNumberField(TEXT("shore_vertices"),Shores)&&Shores>0&&(*Basins)->TryGetNumberField(TEXT("maximum_shore_mask"),Maximum)&&Maximum>0;
            Check(FString(Key)+TEXT("_physical_basin_shore_mask_and_shared_water_levels"),BasinMetadata);
            // Only existing bank/rail supports require the old height exactly.
            // Open frontage now has a small shared physical clay layer, sampled
            // by movement, collision and rendering rather than a flat strip.
            const int32 ExpectedFixedContacts=Expected*18*5+33*3;
            Check(FString(Key)+TEXT("_new_landforms_preserve_exact_bank_and_rail_contacts"),Basins&&(*Basins)->TryGetNumberField(TEXT("protected_contact_samples"),ContactCount)&&ContactCount==ExpectedFixedContacts
                &&(*Basins)->TryGetNumberField(TEXT("protected_maximum_height_change_cm"),ContactChange)&&FMath::IsFinite(ContactChange)&&ContactChange>=0&&ContactChange<=.001);
            const TSharedPtr<FJsonObject>* Frontage=nullptr;double FrontageSamples=0,ChangedSamples=0,MaximumDelta=0,MaximumGradient=0;
            const bool HasFrontage=Basins&&(*Basins)->TryGetObjectField(TEXT("frontage_microrelief_v47"),Frontage)&&Frontage;
            Check(FString(Key)+TEXT("_actual_open_frontage_relief_is_nonzero_and_bounded_to_three_cm"),HasFrontage
                &&(*Frontage)->TryGetNumberField(TEXT("samples"),FrontageSamples)&&FrontageSamples==33*5
                &&(*Frontage)->TryGetNumberField(TEXT("changed_samples"),ChangedSamples)&&ChangedSamples>0&&ChangedSamples<=FrontageSamples
                &&(*Frontage)->TryGetNumberField(TEXT("maximum_delta_cm"),MaximumDelta)&&FMath::IsFinite(MaximumDelta)&&MaximumDelta>=0&&MaximumDelta<=3.001);
            Check(FString(Key)+TEXT("_frontage_relief_gradient_suitable_for_shared_contact"),HasFrontage
                &&(*Frontage)->TryGetNumberField(TEXT("maximum_delta_gradient"),MaximumGradient)&&FMath::IsFinite(MaximumGradient)&&MaximumGradient>=0&&MaximumGradient<=.18);
            const TSharedPtr<FJsonObject>* Scan=nullptr;double CalmSamples=0,CalmChange=0,RefinedCells=0;
            const bool HasScan=Basins&&(*Basins)->TryGetObjectField(TEXT("scan_displacement_v47"),Scan)&&Scan;
            Check(FString(Key)+TEXT("_calm_basin_core_retains_resolved_water_bed"),HasScan&&(*Scan)->TryGetNumberField(TEXT("calm_core_samples"),CalmSamples)&&CalmSamples>0
                &&(*Scan)->TryGetNumberField(TEXT("calm_core_maximum_scan_change_cm"),CalmChange)&&CalmChange<=.001);
            Check(FString(Key)+TEXT("_local_second_refinement_within_8000_cell_budget"),HasScan&&(*Scan)->TryGetNumberField(TEXT("second_refinement_cells"),RefinedCells)&&RefinedCells>=0&&RefinedCells<=8000);
            const auto BankContacts=FrontlineFieldV32::RunBankContactChecksV42(B);J->SetObjectField(TEXT("actual_bank_contacts"),BankContacts);
            bool BankGood=false;BankContacts->TryGetBoolField(TEXT("success"),BankGood);Check(FString(Key)+TEXT("_actual_imported_bank_mesh_contacts"),BankGood);
        };
        auto VolumeChecks=[&](const TCHAR* Key)
        {
            const auto J=Scene47(B);R->Report->SetObjectField(Key,J);bool Material=true,Dimensions=true,Atlas=true;int32 Screens=0,Gas=0;
            for(const auto& V:J->GetArrayField(TEXT("actual_volume_proxies")))
            {
                const auto P=V->AsObject();const FString Family=P->GetStringField(TEXT("family"));Screens+=Family==TEXT("smoke");Gas+=Family==TEXT("gas");
                // Finite positive spatial depth is the invariant. An explosion's
                // evolving skirt/column must not be forced into a round ball.
                const double Y=P->GetNumberField(TEXT("depth_width_ratio")),Z=P->GetNumberField(TEXT("height_width_ratio"));
                Dimensions&=FMath::IsFinite(Y)&&FMath::IsFinite(Z)&&Y>0&&Z>0;
                Material&=P->GetStringField(TEXT("base_material")).Contains(TEXT("/Game/Warfare/Field/V47/Materials/M_SmokeVolume_V47"));
                FString AtlasPath;double AtlasWidth=0,AtlasHeight=0;bool Resource=false;
                Atlas&=P->TryGetStringField(TEXT("density_atlas"),AtlasPath)&&AtlasPath.Contains(TEXT("T_FlowDensity64_V47"))
                    &&P->TryGetNumberField(TEXT("atlas_width"),AtlasWidth)&&AtlasWidth==4096&&P->TryGetNumberField(TEXT("atlas_height"),AtlasHeight)&&AtlasHeight==2048
                    &&P->TryGetBoolField(TEXT("atlas_render_resource"),Resource)&&Resource;
            }
            Check(FString(Key)+TEXT("_real_smoke_and_gas_bind_v47_volume_material_and_time_atlas"),Dimensions&&Material&&Atlas&&Screens>0&&Gas>0);
            Check(FString(Key)+TEXT("_finite_no_proxy_ray_shadow_or_distance_field_obstruction"),J->GetNumberField(TEXT("invalid_transforms"))==0&&J->GetNumberField(TEXT("secondary_obstruction_proxies"))==0);
        };
        auto Blast=[&](float Age)
        {
            R->FixtureAge=Age;
            Check(FString::Printf(TEXT("actual_two_profile_fixture_at_%03d_ms"),FMath::RoundToInt(Age*1000)),FrontlineFXV47::SetupTemporalFixture(B,Age));
        };
        auto BlastShot=[&](const TCHAR* Name)
        {
            int32 Count=0,Grenades=0,Shells=0;bool Ready=true,ExactAge=true;
            for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)
            {
                if(!IsValid(*It)||It->IsActorBeingDestroyed())continue;++Count;
                Ready&=It->IsVisualReady();ExactAge&=FMath::Abs(It->GetVisualAgeSeconds()-R->FixtureAge)<.021f;
                Grenades+=It->GetBlastProfileIndex()==2;Shells+=It->GetBlastProfileIndex()==0;
            }
            Check(FString(Name)+TEXT("_actual_two_ready_frozen_profile_ages"),Count==2&&Grenades==1&&Shells==1&&Ready&&ExactAge);Shot(Name);
        };
        auto Finish=[&]()
        {
            R->Sampling=R->Stress=false;PC->SetActorTickEnabled(true);B->SetActorTickEnabled(false);int32 W=0,H=0;PC->GetViewportSize(W,H);
            Check(TEXT("requested_actual_viewport"),W==R->W&&H==R->H);
            const int32 Expected=(R->TerrainOnly?3:10)+(R->FinalProfiles?2:0);bool Written=R->Shots.Num()==Expected;
            for(const auto& Path:R->Shots)Written&=IFileManager::Get().FileSize(*Path)>1024;Check(TEXT("all_requested_changed_area_captures_written"),Written);
            R->Report->SetNumberField(TEXT("expected_capture_count"),Expected);
            // UI layout remains recorded because the real HUD is visible, but
            // this terrain/FX pass does not replay historical typography suites.
            R->Report->SetObjectField(TEXT("actual_UI_layout"),FrontlineReviewUIV41::EndAudit());
            R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetArrayField(TEXT("captures"),R->Views);
            int32 Passed=0;for(const auto& Item:R->Checks)Passed+=Item->AsObject()->GetBoolField(TEXT("passed"));
            R->Report->SetNumberField(TEXT("passed"),Passed);R->Report->SetNumberField(TEXT("total"),R->Checks.Num());
            R->Report->SetObjectField(TEXT("final_render"),Render47(B));R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetNumberField(TEXT("startup_seconds"),R->ExecutionStart>0?R->ExecutionStart-R->Start:Now-R->Start);
            R->Report->SetNumberField(TEXT("active_review_seconds"),R->ExecutionStart>0?Now-R->ExecutionStart:0);
            R->Report->SetNumberField(TEXT("startup_budget_seconds"),300);R->Report->SetNumberField(TEXT("active_budget_seconds"),98);
            R->Report->SetStringField(TEXT("rhi"),GDynamicRHI?GDynamicRHI->GetName():TEXT("unavailable"));R->Report->SetStringField(TEXT("adapter"),GRHIAdapterName);
            R->Report->SetNumberField(TEXT("width"),W);R->Report->SetNumberField(TEXT("height"),H);
            R->Report->SetBoolField(TEXT("terrain_only"),R->TerrainOnly);R->Report->SetBoolField(TEXT("final_profiles_requested"),R->FinalProfiles);
            R->Report->SetBoolField(TEXT("isolated_nanite_ground_diagnostic"),FParse::Param(FCommandLine::Get(),TEXT("WFV47NaniteGround")));
            R->Report->SetBoolField(TEXT("visual_quality_requires_human_review"),true);
            R->Report->SetStringField(TEXT("scope"),R->TerrainOnly
                ?TEXT("V47 local terrain-only: real British profile 0, High 4K Quality SR/RR and FG off, normal/zoom/wide camera. Actual V47 terrain/water bindings, source raster partitions, distance fields/cards, physical shores and preserved authoritative contact. Optional final-profiles adds real 19 -> 0 reload checks. No FX performance or general visual-quality claim.")
                :TEXT("V47 local focused visual evidence: normal/zoom/wide British profile 0 terrain; one actual grenade and one field-shell cosmetic fixture frozen at 0.06/0.25/0.8/2.5 seconds; production smoke/gas viewed from two real camera azimuths; one eight-second live FX pan with real soldier/component ticks and held combat director. Optional final-profiles adds 19 -> 0. No old balance/audio/campaign suite, no dynamic combat benchmark, no automatic artistic acceptance or FPS threshold."));
            FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Dir/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);
        };
        // Real first-editor shader compilation has a separate startup budget;
        // it must not consume the short capture/performance sequence deadline.
        if(R->ExecutionStart<=0&&Now-R->Start>300){Check(TEXT("shader_and_world_startup_completed_within_300_seconds"),false);Finish();return false;}
        if(R->ExecutionStart>0&&Now-R->ExecutionStart>98){Check(TEXT("active_visual_review_completed_within_98_seconds"),false);Finish();return false;}
        const float Step=FMath::Clamp(DT,0.f,.05f);
        if(R->Stage>0&&B->Phase==EFrontlinePhase::Fighting&&!B->IsActorTickEnabled())B->Elapsed+=Step;
        if(R->Stress)
        {
            const float Time=float(Now-R->StageStart),Limit=FrontlineTerrainV42::CameraVerticalLimitV43(6000,float(R->W)/R->H);
            Camera(FMath::Sin(Time*.53f)*650,FMath::Sin(Time*.71f)*Limit*.65f,6000,350,false);
            if(Now>=R->NextBlast)
            {
                R->NextBlast=Now+1.7;const bool Grenade=(int32(Time/1.7f)%2)==0;const float X=Grenade?-1250.f:1250.f;
                if(auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(FVector(X,420,AWarfareFrontlineGround::HeightAt(X,420)),FRotator::ZeroRotator))Impact->ConfigureBlast(Grenade,false,FVector(.5,.2,-1));
            }
        }
        if(R->Stage>0&&PC->BattleCamera)FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),R->Focus,R->Width,R->W);
        if(R->Sampling)
        {
            const double Ms=(Now-R->Last)*1000;R->Last=Now;if(Ms>0){R->Sample.Wall.Add(Ms);R->Sample.Seconds+=Ms*.001;}
            auto Add=[](TArray<double>& Values,double Value){if(FMath::IsFinite(Value)&&Value>0)Values.Add(Value);};
            Add(R->Sample.GPU,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(R->Sample.Game,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(R->Sample.Render,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(R->Sample.RHI,FPlatformTime::ToMilliseconds(GRHIThreadTime));if(!FApp::HasFocus())++R->Sample.Unfocused;
        }
        if(Now<R->Next||FScreenshotRequest::IsScreenshotRequested())return true;
        auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
        switch(R->Stage)
        {
        case 0:
            if(ReviewShadersPending47()){R->Next=Now+.5;break;}
            FrontlineFXV47::Prewarm();
            Check(TEXT("isolated_volatile_review_profile"),Allowed47(B));FrontlineReviewUIV41::BeginAudit();FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(B,false);
            if(Render){Render->SetHDREnabled(false,false);Render->SetGraphicsPreset(EFrontlineGraphicsPreset::High,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetFrameRateSlider(1,false);}
            if(auto* Value=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))Value->Set(0,ECVF_SetByCode);
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);
            Check(TEXT("first_british_field"),FFrontlineReviewV47::Field(B,EFrontlineFaction::British,0));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);
            Check(TEXT("actual_first_mission_weather_prepared"),FrontlineLightingV42::PrepareReviewWeather(B));R->InitialContacts=ContactSamples47();
            R->Stage=1;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+5;break;
        case 1:
        {
            const auto J=Terrain47(B);const auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());
            if(ReviewShadersPending47()){R->Next=Now+.5;break;}
            if((!BakedTerrain47(J)||!Weather||!Weather->IsReady())&&Now-R->StageStart<22){R->Next=Now+.5;break;}
            R->ExecutionStart=Now;
            Geometry(TEXT("first_british_terrain"),3);Check(TEXT("actual_weather_ready"),Weather&&Weather->IsReady());
            R->Report->SetObjectField(TEXT("actual_lighting"),FrontlineLightingV42::DescribeLiveReflectionBindingsV44(B));
            const auto Settings=Render47(B);R->Report->SetObjectField(TEXT("initial_render"),Settings);
            Check(TEXT("actual_quality_sr_rr_and_frame_generation_off"),Settings->GetBoolField(TEXT("quality_sr"))&&Settings->GetBoolField(TEXT("rr_enabled_api"))&&Settings->GetBoolField(TEXT("fg_off_requested_and_effective")));
            Shot(TEXT("01-normal-terrain-6000"));R->Stage=2;R->Next=Now+.35;break;
        }
        case 2:Camera(0,0,3400,350);R->Stage=3;R->Next=Now+1;break;
        case 3:Shot(TEXT("02-zoom-terrain-3400"));R->Stage=4;R->Next=Now+.35;break;
        case 4:Camera(0,0,9000,650);R->Stage=5;R->Next=Now+1;break;
        case 5:Shot(TEXT("03-wide-terrain-9000"));R->Stage=6;R->Next=Now+.35;break;
        case 6:
            if(R->TerrainOnly){R->Stage=R->FinalProfiles?30:40;break;}
            FrontlineFXV47::RunFocusedChecks(B,[&](const TCHAR* Name,bool Good){Check(Name,Good);});
            Camera(0,150,6000,350);Blast(.06f);R->Stage=7;R->Next=Now+.4;break;
        case 7:BlastShot(TEXT("04-grenade-field-shell-0060ms"));R->Stage=8;R->Next=Now+.35;break;
        case 8:Blast(.25f);R->Stage=9;R->Next=Now+.4;break;
        case 9:BlastShot(TEXT("05-grenade-field-shell-0250ms"));R->Stage=10;R->Next=Now+.35;break;
        case 10:Blast(.8f);R->Stage=11;R->Next=Now+.4;break;
        case 11:BlastShot(TEXT("06-grenade-field-shell-0800ms"));R->Stage=12;R->Next=Now+.35;break;
        case 12:Blast(2.5f);R->Stage=13;R->Next=Now+.4;break;
        case 13:BlastShot(TEXT("07-grenade-field-shell-2500ms"));R->Stage=14;R->Next=Now+.35;break;
        case 14:
        {
            for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)It->Destroy();
            auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            const FVector Target(1600,350,AWarfareFrontlineGround::HeightAt(1600,350));
            Check(TEXT("production_gas_mission_for_spatial_review"),Gas&&Gas->Configure(B,EFrontlineSupport::Gas,Target,0));
            B->Elapsed+=B->FireSupportDelayRange(EFrontlineSupport::Gas,Target,0).Y+11.f;if(Gas)Gas->Tick(.01f);
            auto* Smoke=B->GetWorld()->SpawnActor<AWarfareFrontlineSmoke>(FVector(-1600,350,AWarfareFrontlineGround::HeightAt(-1600,350)),FRotator::ZeroRotator);
            Check(TEXT("production_smoke_screen_actor_ready"),Smoke&&Smoke->bVisualReady);if(Smoke){Smoke->DeployAt=B->Elapsed-5.f;Smoke->Tick(.01f);}
            Camera(0,150,6000,350);R->Stage=15;R->Next=Now+.6;break;
        }
        case 15:VolumeChecks(TEXT("normal_volume_bindings"));Shot(TEXT("08-smoke-gas-normal-azimuth"));R->Stage=16;R->Next=Now+.35;break;
        case 16:
            if(PC->BattleCamera)
            {
                PC->SetActorTickEnabled(false);const FVector Offset=PC->BattleCamera->GetActorLocation()-R->Focus;
                const FVector Location=R->Focus+FQuat(FVector::UpVector,FMath::DegreesToRadians(22.f)).RotateVector(Offset);
                PC->BattleCamera->SetActorLocationAndRotation(Location,(R->Focus-Location).Rotation());
                if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);R->Orbit=true;
            }
            Check(TEXT("second_volume_view_has_actual_changed_camera_azimuth"),R->Orbit);R->Stage=17;R->Next=Now+.8;break;
        case 17:VolumeChecks(TEXT("orbited_volume_bindings"));Shot(TEXT("09-smoke-gas-orbit-22-degrees"));R->Stage=18;R->Next=Now+.35;break;
        case 18:
            PC->SetActorTickEnabled(true);R->Orbit=false;Camera(0,0,6000,350);
            R->Stress=true;R->StageStart=Now;R->NextBlast=Now;R->Stage=19;R->Next=Now+1.2;break;
        case 19:R->Sample=FSample47();R->Sampling=true;R->Last=FPlatformTime::Seconds();R->Stage=20;R->Next=R->Last+8;break;
        case 20:
        {
            R->Sampling=R->Stress=false;const auto Perf=R->Sample.Describe();
            Perf->SetStringField(TEXT("scope"),TEXT("One eight-second live smoke/gas scene, alternating production grenade/field-shell blasts and smooth horizontal/vertical camera pans. Real soldiers/component ticks; combat director held. Actual delayed whole-frame RHI GPU/CPU timers, no screenshot/readback during sampling. No FPS acceptance threshold, per-pass subtraction or full-battle claim."));
            R->Report->SetObjectField(TEXT("fx_pan_performance"),Perf);VolumeChecks(TEXT("stress_actual_scene"));
            Check(TEXT("eight_second_active_fx_sample_with_actual_gpu_timers"),R->Sample.Seconds>=7.9&&R->Sample.GPU.Num()>0);
            Shot(TEXT("10-live-fx-pan"));R->Stage=R->FinalProfiles?30:40;R->Next=Now+.5;break;
        }
        case 30:
            Check(TEXT("optional_final_german_field_reload"),FFrontlineReviewV47::Field(B,EFrontlineFaction::German,19));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);FrontlineLightingV42::PrepareReviewWeather(B);
            R->Stage=31;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+4;break;
        case 31:
            if((ReviewShadersPending47()||!BakedTerrain47(Terrain47(B)))&&Now-R->StageStart<15){R->Next=Now+.5;break;}
            Geometry(TEXT("final_german_terrain"),5);Shot(TEXT("11-final-five-trenches"));R->Stage=32;R->Next=Now+.35;break;
        case 32:
            Check(TEXT("optional_first_british_field_restored"),FFrontlineReviewV47::Field(B,EFrontlineFaction::British,0));FrontlineReviewUIV41::Clear(PC);Camera(0,0,6000,650);FrontlineLightingV42::PrepareReviewWeather(B);
            R->Stage=33;R->StageStart=FPlatformTime::Seconds();R->Next=R->StageStart+3;break;
        case 33:
        {
            if((ReviewShadersPending47()||!BakedTerrain47(Terrain47(B)))&&Now-R->StageStart<12){R->Next=Now+.5;break;}
            Geometry(TEXT("restored_first_british_terrain"),3);const auto Samples=ContactSamples47();bool Restored=Samples.Num()==R->InitialContacts.Num();
            double MaximumError=0;for(int32 I=0;I<Samples.Num()&&I<R->InitialContacts.Num();++I)MaximumError=FMath::Max(MaximumError,FMath::Abs(Samples[I]-R->InitialContacts[I]));
            R->Report->SetNumberField(TEXT("restored_contact_maximum_error_cm"),MaximumError);Check(TEXT("first_profile_protected_gameplay_contact_restored"),Restored&&MaximumError<.001);
            Shot(TEXT("12-first-three-trenches-restored"));R->Stage=40;R->Next=Now+.5;break;
        }
        default:Finish();return false;
        }
        return true;
    }));
}

