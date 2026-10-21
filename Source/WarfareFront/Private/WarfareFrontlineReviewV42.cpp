#include "WarfareFrontlineReviewV42.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineReviewV41.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineTerrainV42.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ComponentInterfaces.h"
#include "ContentStreaming.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureStreamingTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "MaterialShared.h"
#include "MeshUVChannelInfo.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialRenderProxy.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "RenderingThread.h"
#include "SceneInterface.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
bool Allowed(AWarfareFrontlineGameMode* B)
{
    FString Dir;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
struct FSceneV42 {EFrontlineFaction Faction;int32 Mission;const TCHAR* Key;};
const FSceneV42 Scenes[]={{EFrontlineFaction::British,5,TEXT("british-pilckem-wet")},
    {EFrontlineFaction::German,0,TEXT("german-arras-chalk")},{EFrontlineFaction::German,18,TEXT("german-valenciennes-late")}};
TSharedRef<FJsonObject> Distribution(TArray<double> V)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),V.Num());J->SetBoolField(TEXT("available"),!V.IsEmpty());if(V.IsEmpty())return J;
    double Sum=0;for(double X:V)Sum+=X;V.Sort();J->SetNumberField(TEXT("mean_ms"),Sum/V.Num());
    J->SetNumberField(TEXT("p95_ms"),V[FMath::Clamp(FMath::CeilToInt(V.Num()*.95)-1,0,V.Num()-1)]);J->SetNumberField(TEXT("max_ms"),V.Last());return J;
}
struct FSamplesV42
{
    TArray<double> Wall,GPU,Game,Render,RHI;int32 Unfocused=0;double Seconds=0;
    TSharedRef<FJsonObject> Describe() const
    {
        auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("wall"),Distribution(Wall));J->SetObjectField(TEXT("gpu"),Distribution(GPU));
        J->SetObjectField(TEXT("game_thread"),Distribution(Game));J->SetObjectField(TEXT("render_thread"),Distribution(Render));J->SetObjectField(TEXT("rhi_thread"),Distribution(RHI));
        J->SetNumberField(TEXT("sample_seconds"),Seconds);J->SetNumberField(TEXT("render_fps"),Seconds>0?Wall.Num()/Seconds:0);J->SetNumberField(TEXT("unfocused_frames"),Unfocused);
        J->SetStringField(TEXT("timing_source"),TEXT("One core-ticker observation per frame; wall interval plus RHIGetGPUFrameCycles and actual game/render/RHI thread timers. Delayed GPU values are sampled as exposed by the RHI, not per-pass profiler timings."));return J;
    }
};
struct FMaterialRenderObservationV42
{
    const FMaterialRenderProxy* Proxy=nullptr;
    bool OriginalPresent=false,Complete=false,Valid=false,CompilationFinalized=false;
    int32 Quality=INDEX_NONE,ShaderCount=0,PipelineCount=0;FString MaterialName;
};
TArray<FMaterialRenderObservationV42> ObserveMaterialsOnRenderThread(const TArray<UMaterialInterface*>& Materials,ERHIFeatureLevel::Type FeatureLevel)
{
    TArray<TStrongObjectPtr<UMaterialInterface>> KeepAlive;
    auto Batch=MakeShared<TArray<FMaterialRenderObservationV42>,ESPMode::ThreadSafe>();
    for(auto* Material:Materials){KeepAlive.Emplace(Material);FMaterialRenderObservationV42 O;O.Proxy=Material->GetRenderProxy();Batch->Add(O);}
    ENQUEUE_RENDER_COMMAND(FrontlineV42ObserveAssignedMaterialShaders)([Batch,FeatureLevel](FRHICommandListImmediate&)
    {
        for(auto& O:*Batch)
        {
            // No fallback query: a default surface must never count as the
            // requested material being ready, even when it renders successfully.
            const FMaterial* Material=O.Proxy?O.Proxy->GetMaterialNoFallback(FeatureLevel):nullptr;
            O.OriginalPresent=Material!=nullptr;if(!Material)continue;
            O.Complete=Material->IsRenderingThreadShaderMapComplete();O.Quality=int32(Material->GetQualityLevel());O.MaterialName=Material->GetFriendlyName();
            const auto* Map=Material->GetRenderingThreadShaderMap();O.Valid=Map&&Map->IsValidForRendering();O.CompilationFinalized=Map&&Map->IsCompilationFinalized();
            if(Map)Map->CountNumShaders(O.ShaderCount,O.PipelineCount);
        }
    });
    // Isolated review inventory only, outside every timed interval. One batch
    // drains the render command queue; never called from ordinary gameplay.
    // Strong object references remain on this thread until the batch completes.
    FlushRenderingCommands();return MoveTemp(*Batch);
}
TSharedRef<FJsonObject> RenderContract(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();auto* R=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld());
    J->SetBoolField(TEXT("fg_off_requested_and_effective"),R&&R->GetFrameGeneration()==EFrontlineFrameGeneration::Off&&R->GetEffectiveFrameGeneration()==EFrontlineFrameGeneration::Off);
    J->SetBoolField(TEXT("quality_sr"),R&&R->GetQuality()==EFrontlineRenderQuality::Quality);J->SetBoolField(TEXT("rr_requested"),R&&R->GetRayReconstruction());
    if(R){J->SetObjectField(TEXT("runtime_diagnostics"),R->GetDiagnostics());J->SetNumberField(TEXT("production_frame_cap"),R->GetFrameRateLimit());}
    auto C=MakeShared<FJsonObject>();const TCHAR* Names[]={TEXT("r.ScreenPercentage"),TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("t.IdleWhenNotForeground"),
        TEXT("r.RayTracing"),TEXT("r.PathTracing"),TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Lumen.HardwareRayTracing.LightingMode"),
        TEXT("r.Lumen.Reflections.HardwareRayTracing"),TEXT("r.Lumen.Reflections.DownsampleFactor"),TEXT("r.Lumen.Reflections.MaxRoughnessToTrace"),
        TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"),
        TEXT("r.Nanite"),TEXT("r.RayTracing.Geometry.InstancedStaticMeshes"),TEXT("r.RayTracing.Culling"),TEXT("r.Streaming.PoolSize"),
        TEXT("r.RayTracing.Geometry.DynamicGeometryPoolSize"),TEXT("r.VolumetricFog.GridPixelSize"),TEXT("r.VolumetricFog.GridSizeZ")};
    for(const TCHAR* Name:Names)if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))C->SetStringField(Name,V->GetString());
    J->SetObjectField(TEXT("render_cvars"),C);return J;
}
TSharedRef<FJsonObject> FieldInventory(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Components,Materials,Textures,Units;
    TSet<UMaterialInterface*> UsedMaterials;TSet<UTexture*> UsedTextures;bool Ready=true;int32 Instances=0,VisibleComponents=0;
    const EShaderPlatform Platform=B->GetWorld()->Scene?B->GetWorld()->Scene->GetShaderPlatform():GetFeatureLevelShaderPlatform(B->GetWorld()->GetFeatureLevel());
    J->SetNumberField(TEXT("actual_scene_shader_platform"),int32(Platform));
    J->SetNumberField(TEXT("feature_level_default_shader_platform"),int32(GetFeatureLevelShaderPlatform(B->GetWorld()->GetFeatureLevel())));
    auto Streaming=MakeShared<FJsonObject>();auto& Manager=IStreamingManager::Get();
    Streaming->SetBoolField(TEXT("streaming_enabled"),Manager.IsStreamingEnabled());
    Streaming->SetBoolField(TEXT("texture_streaming_enabled"),Manager.IsTextureStreamingEnabled());
    auto& Assets=Manager.GetRenderAssetStreamingManager();
    Streaming->SetNumberField(TEXT("pool_bytes"),double(Assets.GetPoolSize()));
    Streaming->SetNumberField(TEXT("required_pool_bytes"),double(Assets.GetRequiredPoolSize()));
    Streaming->SetNumberField(TEXT("over_budget_bytes"),double(Assets.GetMemoryOverBudget()));
    TArray<TSharedPtr<FJsonValue>> Views;
    for(int32 Index=0;Index<Manager.GetNumViews();++Index)
    {
        const auto& V=Manager.GetViewInformation(Index);auto View=MakeShared<FJsonObject>();
        View->SetStringField(TEXT("origin"),V.ViewOrigin.ToString());View->SetNumberField(TEXT("screen_size_consumed_by_streamer"),V.ScreenSize);
        View->SetNumberField(TEXT("fov_screen_size"),V.FOVScreenSize);View->SetNumberField(TEXT("boost"),V.BoostFactor);
        View->SetNumberField(TEXT("remaining_seconds"),V.Duration);View->SetBoolField(TEXT("override"),V.bOverrideLocation);
        View->SetBoolField(TEXT("matches_current_world"),V.World.Get()==B->GetWorld());View->SetBoolField(TEXT("world_unspecified"),!V.World.IsValid());
        Views.Add(MakeShared<FJsonValueObject>(View));
    }
    Streaming->SetArrayField(TEXT("actual_final_views"),Views);
    auto StreamCvars=MakeShared<FJsonObject>();
    for(const TCHAR* Name:{TEXT("r.TextureStreaming"),TEXT("r.Streaming.PoolSize"),TEXT("r.Streaming.UseFixedPoolSize"),
        TEXT("r.Streaming.MipBias"),TEXT("r.Streaming.UsePerTextureBias"),TEXT("r.Streaming.Boost"),TEXT("r.Streaming.MinBoost"),
        TEXT("r.Streaming.MaxEffectiveScreenSize"),TEXT("r.Streaming.HiddenPrimitiveScale"),TEXT("r.Streaming.UseNewMetrics"),
        TEXT("r.Streaming.UseAllMips"),TEXT("r.Streaming.FullyLoadUsedTextures"),TEXT("r.Streaming.DropMips"),TEXT("r.Streaming.ForceFullyLoad"),
        TEXT("r.Streaming.FramesForFullUpdate"),TEXT("r.Streaming.UseMaterialData"),TEXT("s.StreamableAssets.UseSimpleStreamableAssetManager"),
        TEXT("s.StreamableAssets.SimpleStreamableAssetManager.ConsiderVisibility")})
        if(auto* Cvar=IConsoleManager::Get().FindConsoleVariable(Name))StreamCvars->SetStringField(Name,Cvar->GetString());
    Streaming->SetObjectField(TEXT("cvars"),StreamCvars);J->SetObjectField(TEXT("actual_streaming_manager"),Streaming);
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
        for(auto* C:Meshes)
        {
            if(!C->IsRegistered()||!C->IsVisible()||It->IsHidden()||!C->GetStaticMesh())continue;
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("component"),C->GetPathName());Row->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());
            const auto* ISM=Cast<UInstancedStaticMeshComponent>(C);const int32 Count=ISM?ISM->GetInstanceCount():1;Instances+=Count;++VisibleComponents;
            Row->SetNumberField(TEXT("instances"),Count);Row->SetNumberField(TEXT("collision_mode"),int32(C->GetCollisionEnabled()));Row->SetStringField(TEXT("world_bounds"),C->Bounds.ToString());
            if(It->IsA<AWarfareFrontlineGround>())
            {
                const auto* UV=C->GetStaticMesh()->GetUVChannelData(0);
                Row->SetBoolField(TEXT("runtime_uv_density_initialized"),UV&&UV->IsInitialized());
                Row->SetNumberField(TEXT("runtime_world_cm_per_uv0"),UV?UV->LocalUVDensities[0]:0);
                Row->SetNumberField(TEXT("streaming_distance_multiplier"),C->StreamingDistanceMultiplier);
                Row->SetNumberField(TEXT("component_mobility"),int32(C->Mobility));
                Row->SetBoolField(TEXT("runtime_ground_surface"),C==CastChecked<AWarfareFrontlineGround>(*It)->Surface);
                auto StreamRows=[](const TArray<FStreamingRenderAssetPrimitiveInfo>& Infos)
                {
                    TArray<TSharedPtr<FJsonValue>> Rows;
                    for(const auto& Info:Infos)
                    {
                        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("asset"),GetPathNameSafe(Info.RenderAsset));
                        R->SetNumberField(TEXT("texel_factor"),Info.TexelFactor);R->SetStringField(TEXT("bounds"),Info.Bounds.ToString());
                        R->SetBoolField(TEXT("valid_distance_streaming"),Info.CanBeStreamedByDistance(true));Rows.Add(MakeShared<FJsonValueObject>(R));
                    }
                    return Rows;
                };
                FStreamingTextureLevelContext ActualContext(EMaterialQualityLevel::High,B->GetWorld()->GetFeatureLevel(),false);
                TArray<FStreamingRenderAssetPrimitiveInfo> ActualInfo;C->GetStreamingRenderAssetInfo(ActualContext,ActualInfo);
                Row->SetArrayField(TEXT("actual_streaming_records"),StreamRows(ActualInfo));
            }
            TArray<TSharedPtr<FJsonValue>> Slots;for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)if(auto* M=C->GetMaterial(Slot)){UsedMaterials.Add(M);Slots.Add(MakeShared<FJsonValueString>(M->GetPathName()));}
            Row->SetArrayField(TEXT("assigned_materials"),Slots);Components.Add(MakeShared<FJsonValueObject>(Row));Ready&=!C->GetStaticMesh()->IsCompiling();
        }
    }
    const TSet<UMaterialInterface*> AssignedMaterials=UsedMaterials;
    for(const TCHAR* Path:{TEXT("/Game/Warfare/Weather/V19/M_RainLine_V19.M_RainLine_V19"),TEXT("/Game/Warfare/Weather/V19/M_RainContact_V19.M_RainContact_V19")})
        if(auto* M=FindObject<UMaterialInterface>(nullptr,Path))UsedMaterials.Add(M);
    const TArray<UMaterialInterface*> MaterialList=UsedMaterials.Array();
    const auto Observed=ObserveMaterialsOnRenderThread(MaterialList,B->GetWorld()->GetFeatureLevel());
    for(int32 MaterialIndex=0;MaterialIndex<MaterialList.Num();++MaterialIndex)
    {
        auto* M=MaterialList[MaterialIndex];const auto& Actual=Observed[MaterialIndex];
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("path"),M->GetPathName());Row->SetStringField(TEXT("base_material"),GetPathNameSafe(M->GetMaterial()));
        Row->SetBoolField(TEXT("assigned_to_visible_static_component"),AssignedMaterials.Contains(M));
        if(const auto* MI=Cast<UMaterialInstance>(M))Row->SetStringField(TEXT("immediate_parent"),GetPathNameSafe(MI->Parent));
        const auto* Resource=M->GetMaterialResource(Platform);const bool Complete=Resource&&Resource->IsGameThreadShaderMapComplete();
        Row->SetBoolField(TEXT("material_resource_present"),Resource!=nullptr);Row->SetBoolField(TEXT("shader_map_valid"),Resource&&Resource->HasValidGameThreadShaderMap());
        Row->SetNumberField(TEXT("game_thread_resource_quality"),Resource?int32(Resource->GetQualityLevel()):INDEX_NONE);
        Row->SetBoolField(TEXT("render_original_material_present"),Actual.OriginalPresent);Row->SetBoolField(TEXT("render_shader_map_complete"),Actual.Complete);
        Row->SetBoolField(TEXT("render_shader_map_valid"),Actual.Valid);Row->SetBoolField(TEXT("render_compilation_finalized"),Actual.CompilationFinalized);
        Row->SetNumberField(TEXT("render_resource_quality"),Actual.Quality);Row->SetStringField(TEXT("render_material"),Actual.MaterialName);
        Row->SetNumberField(TEXT("render_shader_count"),Actual.ShaderCount);Row->SetNumberField(TEXT("render_pipeline_count"),Actual.PipelineCount);
        // UE's actual base-pass and ray-tracing processors use the original
        // proxy's frozen shader content and request the shaders for their pass.
        // Whole-map compilation/finalization is not their admission criterion.
        Ready&=Actual.OriginalPresent&&Actual.Valid&&Actual.ShaderCount>0;
        Row->SetBoolField(TEXT("shader_map_complete"),Complete);TArray<UTexture*> TexturesForMaterial;M->GetUsedTextures(TexturesForMaterial,{},Platform);
        TArray<TSharedPtr<FJsonValue>> Paths;for(auto* T:TexturesForMaterial)if(IsValid(T)){UsedTextures.Add(T);Paths.Add(MakeShared<FJsonValueString>(T->GetPathName()));}
        Row->SetArrayField(TEXT("used_textures_current_shader_platform"),Paths);Materials.Add(MakeShared<FJsonValueObject>(Row));
    }
    for(auto* T:UsedTextures)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("path"),T->GetPathName());Row->SetBoolField(TEXT("render_resource_present"),T->GetResource()!=nullptr);Ready&=T->GetResource()!=nullptr;
        if(auto* T2=Cast<UTexture2D>(T))
        {
            Row->SetNumberField(TEXT("width"),T2->GetSizeX());Row->SetNumberField(TEXT("height"),T2->GetSizeY());Row->SetNumberField(TEXT("total_mips"),T2->GetNumMips());
            Row->SetNumberField(TEXT("resident_mips"),T2->GetNumResidentMips());Row->SetBoolField(TEXT("pending_streaming"),T2->HasPendingInitOrStreaming());
            const int32 FirstResident=FMath::Clamp(T2->GetNumMips()-T2->GetNumResidentMips(),0,30);
            Row->SetNumberField(TEXT("first_resident_mip"),FirstResident);
            Row->SetNumberField(TEXT("largest_resident_width"),T2->GetNumResidentMips()>0?FMath::Max(1,T2->GetSizeX()>>FirstResident):0);
            Row->SetNumberField(TEXT("largest_resident_height"),T2->GetNumResidentMips()>0?FMath::Max(1,T2->GetSizeY()>>FirstResident):0);
            Row->SetBoolField(TEXT("fully_streamed"),T2->IsFullyStreamedIn());Ready&=T2->GetNumResidentMips()>0&&!T2->HasPendingInitOrStreaming();
            if(T2->GetPathName().Contains(TEXT("/V23/Textures/T_Mud_"))||T2->GetPathName().Contains(TEXT("/V42/Textures/")))
            {
                auto State=MakeShared<FJsonObject>();const auto& S=T2->GetStreamableResourceState();
                State->SetBoolField(TEXT("valid"),S.IsValid());State->SetBoolField(TEXT("supports_streaming"),S.bSupportsStreaming);
                State->SetBoolField(TEXT("virtual_streaming"),S.bSupportsVirtualStreaming);State->SetBoolField(TEXT("streamer_linked"),T2->IsStreamable());
                State->SetNumberField(TEXT("streaming_index"),T2->GetStreamingIndex());State->SetNumberField(TEXT("cached_lod_bias"),T2->GetCachedLODBias());
                State->SetNumberField(TEXT("max_resource_lods"),S.MaxNumLODs);State->SetNumberField(TEXT("nonoptional_lods"),S.NumNonOptionalLODs);
                State->SetNumberField(TEXT("nonstreaming_lods"),S.NumNonStreamingLODs);State->SetNumberField(TEXT("resident_lods"),S.NumResidentLODs);
                State->SetNumberField(TEXT("requested_lods"),S.NumRequestedLODs);State->SetNumberField(TEXT("asset_lod_bias"),S.AssetLODBias);
                State->SetNumberField(TEXT("lod_bias_modifier"),S.LODBiasModifier);
                State->SetNumberField(TEXT("resident_first_asset_mip"),S.LODCountToAssetFirstLODIdx(S.NumResidentLODs));
                State->SetNumberField(TEXT("requested_first_asset_mip"),S.LODCountToAssetFirstLODIdx(S.NumRequestedLODs));
#if !UE_BUILD_SHIPPING
                State->SetNumberField(TEXT("manager_cached_wanted_lods"),T2->GetCachedNumWantedLODs());
#else
                State->SetStringField(TEXT("manager_wanted_lods_scope"),TEXT("Not exposed by the Shipping public API; requested resource LODs remain recorded."));
#endif
                TArray<const IPrimitiveComponent*> References;Assets.GetAssetComponents(T2,References);
                TArray<TSharedPtr<FJsonValue>> RefRows;
                for(const auto* C:References)if(C)
                {
                    auto Ref=MakeShared<FJsonObject>();Ref->SetStringField(TEXT("component"),C->GetFullName());
                    Ref->SetBoolField(TEXT("registered"),C->IsRegistered());Ref->SetBoolField(TEXT("render_state"),C->IsRenderStateCreated());
                    Ref->SetNumberField(TEXT("last_rendered_world_seconds"),C->GetLastRenderTimeOnScreen());
                    RefRows.Add(MakeShared<FJsonValueObject>(Ref));
                }
                State->SetArrayField(TEXT("actual_manager_component_references"),RefRows);State->SetNumberField(TEXT("manager_component_count"),References.Num());
                const auto* Simple=IConsoleManager::Get().FindConsoleVariable(TEXT("s.StreamableAssets.UseSimpleStreamableAssetManager"));
                State->SetBoolField(TEXT("manager_component_query_supported"),!Simple||Simple->GetInt()==0);
                Row->SetObjectField(TEXT("streaming_demand"),State);
            }
        }
        Textures.Add(MakeShared<FJsonValueObject>(Row));
    }
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U))
    {
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("squad"),U->SquadId);Row->SetNumberField(TEXT("member"),U->MemberIndex);Row->SetNumberField(TEXT("role"),int32(U->Role));
        Row->SetNumberField(TEXT("team"),U->Team);Row->SetBoolField(TEXT("alive"),U->IsAlive());Row->SetStringField(TEXT("feet"),U->Feet().ToString());
        Row->SetNumberField(TEXT("animation_seconds_advanced"),U->AnimationTimeAdvanced);Row->SetNumberField(TEXT("shots"),U->ShotsDischarged);Units.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetBoolField(TEXT("ready"),Ready&&VisibleComponents>0&&Units.Num()>0);J->SetNumberField(TEXT("visible_static_components"),VisibleComponents);J->SetNumberField(TEXT("static_instances"),Instances);
    J->SetNumberField(TEXT("unit_count"),Units.Num());J->SetArrayField(TEXT("units"),Units);J->SetArrayField(TEXT("static_components"),Components);J->SetArrayField(TEXT("materials"),Materials);J->SetArrayField(TEXT("textures"),Textures);
    J->SetStringField(TEXT("readiness_scope"),TEXT("Assigned materials plus two precipitation sources: original render-proxy frozen map is present, render-valid and contains shaders. Whole-map completeness/finalization is recorded separately; this does not assert every permutation compiled. No substituted default proxy accepted. Batched render observation runs outside timing. Graph output wiring is verified by the importer receipt."));return J;
}

TSharedRef<FJsonObject> SoilResolutionContract(AWarfareFrontlineGameMode* B,float OrthoWidth,int32 OutputWidth)
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Adequate=true;int32 Count=0;
    const EShaderPlatform Platform=B->GetWorld()->Scene->GetShaderPlatform();
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
    {
        auto* C=It->Surface.Get();UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;const auto* UV=Mesh?Mesh->GetUVChannelData(0):nullptr;
        auto* Material=C?C->GetMaterial(0):nullptr;if(!Material||!UV||!UV->IsInitialized())continue;
        float Projection=.52f;
        Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SoilProjectionScaleV47")),Projection);
        Projection=FMath::Clamp(Projection,.05f,8.f);
        // The world-projected scan repeats at UV density / projection scale.
        // Using mesh density alone under-reports the mip needed when zoomed in.
        const float TileCm=UV->LocalUVDensities[0]*C->GetComponentTransform().GetMaximumAxisScale()/Projection;
        const float TilePixels=TileCm*OutputWidth/FMath::Max(OrthoWidth,1.f);
        const int32 RequiredSide=FMath::RoundUpToPowerOfTwo(FMath::Max(1,FMath::CeilToInt(TilePixels)));
        J->SetNumberField(TEXT("world_cm_per_uv"),TileCm);J->SetNumberField(TEXT("soil_projection_scale"),Projection);
        J->SetNumberField(TEXT("projected_tile_pixels_upper_bound"),TilePixels);
        J->SetNumberField(TEXT("required_largest_resident_side"),RequiredSide);J->SetNumberField(TEXT("output_width"),OutputWidth);J->SetNumberField(TEXT("ortho_width_cm"),OrthoWidth);
        TArray<UTexture*> Used;Material->GetUsedTextures(Used,{},Platform);
        for(auto* Texture:Used)if(auto* T=Cast<UTexture2D>(Texture))
        {
            if(!T->GetPathName().Contains(TEXT("/V23/Textures/T_Mud_"))&&!T->GetPathName().Contains(TEXT("/V42/Textures/")))continue;
            const auto& S=T->GetStreamableResourceState();const int32 First=FMath::Clamp(S.LODCountToAssetFirstLODIdx(S.NumResidentLODs),0,30);
            const int32 Width=S.NumResidentLODs?FMath::Max(1,T->GetSizeX()>>First):0,Height=S.NumResidentLODs?FMath::Max(1,T->GetSizeY()>>First):0;
            const bool Good=Width>=FMath::Min(RequiredSide,T->GetSizeX())&&Height>=FMath::Min(RequiredSide,T->GetSizeY());
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("path"),T->GetPathName());R->SetNumberField(TEXT("largest_resident_width"),Width);
            R->SetNumberField(TEXT("largest_resident_height"),Height);R->SetBoolField(TEXT("satisfies_projected_demand"),Good);
            Rows.Add(MakeShared<FJsonValueObject>(R));Adequate&=Good;++Count;
        }
    }
    J->SetArrayField(TEXT("textures"),Rows);J->SetBoolField(TEXT("adequate"),Adequate&&Count==8);
    J->SetStringField(TEXT("scope"),TEXT("Required side is the next power-of-two mip covering one world-space UV tile in the actual output-width orthographic projection. All eight soil maps must satisfy this. It is a minimum sampling-density check, not a claim that the full source resolution should remain resident."));
    return J;
}
}

struct FFrontlineEnvironmentReviewV42
{
    static bool Prepare(AWarfareFrontlineGameMode* B,int32 Scene)
    {
        if(!Allowed(B)||Scene<0||Scene>=UE_ARRAY_COUNT(Scenes))return false;const auto& S=Scenes[Scene];
        B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid(42,1917,uint32(S.Mission),uint32(S.Faction)+1);
        R.Faction=S.Faction;R.Difficulty=EFrontlineDifficulty::Veteran;R.CompletedMissions=R.SelectedMission=S.Mission;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=S.Faction;B->ResetEncounter();
        B->BeginBattle();B->SetBattleSpeed(1);B->Elapsed=20;B->SetActorTickEnabled(false);B->NextSquadId=1;B->Random.Initialize(421917+Scene);B->EnemyDecisionRandom.Initialize(42+Scene);
        bool Good=B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty();
        // Real production spawning, real trench destinations and skeletal ticks.
        // Combat director is held, so the visual/performance A/B has constant
        // composition instead of different casualties between shader versions.
        for(int32 Line=0;Line<2;++Line)
        {
            // Even opposing home trenches can be inside a late MG's real
            // suppression range. Friendly reserve squads avoid combat through
            // actual team rules, without overriding weapon ranges or mortality.
            constexpr int32 Team=0;
            const int32 Trench=Line==0?FrontlineBattlefield::CentreTrench:B->GetHomeTrenchIndex(0);B->Trenches[Trench].Owner=Team;
            for(int32 Slot=0;Slot<3;++Slot)
            {
                const auto Role=Slot==1&&B->IsRoleUnlocked(EFrontlineRole::Lewis,Team)?EFrontlineRole::Lewis:EFrontlineRole::Rifle;
                Good&=B->SpawnSquad(Team,Role,Trench);
            }
        }
        for(auto& Squad:B->Squads)Squad.bHoldPosition=true;
        return Good&&B->Squads.Num()==6;
    }
};

void StartFrontlineV42Review(AWarfareFrontlineGameMode* Battle)
{
    if(!Allowed(Battle))return;
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;FString Directory;int32 Stage=0,Scene=0,SceneCount=3,StreamingViews=0;bool Passed=true,Baseline=false,Sampling=false,Quick=false;
        double Start=FPlatformTime::Seconds(),StageStart=Start,Next=Start+3,Last=Start,ReadinessAt=0,SampleStart=0;FSamplesV42 Samples;
        TArray<TSharedPtr<FJsonValue>> Checks,SceneReports;TArray<FString> Screenshots;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();TSharedPtr<FJsonObject> Current;
    };
    auto R=MakeShared<FRun>();R->Battle=Battle;R->Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"));
    R->Quick=FParse::Param(FCommandLine::Get(),TEXT("WFV42QuickReview"));R->SceneCount=R->Quick?1:UE_ARRAY_COUNT(Scenes);
    R->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V42/Review"));IFileManager::Get().MakeDirectory(*R->Directory,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float DT)
    {
        auto* B=R->Battle.Get();const double Now=FPlatformTime::Seconds();if(!B)return false;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());if(!PC||!PC->GetHUD())return Now-R->Start<25;
        if(R->Stage<6||R->Stage==8)
        {
            // The deterministic review camera disables controller ticking.
            // Supply exactly the streaming view normally emitted by its camera
            // update; otherwise a fixed-camera review would suppress the real
            // production ortho demand and falsely leave terrain at tiny mips.
            int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);
            const auto* Camera=PC->BattleCamera?PC->BattleCamera->GetCameraComponent():nullptr;
            if(Camera)FrontlineTerrainV42::UpdateStreaming(B->GetWorld(),PC->BattleCamera->GetActorLocation(),
                FVector(0,R->Stage==5?100.f:0.f,R->Stage==5?100.f:650.f),Camera->OrthoWidth,VX);
            if(Camera&&VX>0&&!R->Baseline)++R->StreamingViews;
        }
        if((R->Stage>=1&&R->Stage<=4)||R->Stage==8)B->Elapsed+=FMath::Clamp(DT,0.f,.1f);
        auto Check=[&](const FString& Name,bool Good){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Good);R->Checks.Add(MakeShared<FJsonValueObject>(J));R->Passed&=Good;};
        auto Shot=[&](const FString& Name){const FString Path=R->Directory/(Name+TEXT(".png"));R->Screenshots.Add(Path);FScreenshotRequest::RequestScreenshot(Path,true,false);};
        auto Finish=[&]()
        {
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);Check(TEXT("actual_3840x2160_viewport"),Width==3840&&Height==2160);
            bool Files=true;for(const auto& P:R->Screenshots)Files&=IFileManager::Get().FileSize(*P)>1024;Check(TEXT("requested_captures_written"),Files&&R->Screenshots.Num()>=R->SceneCount*2);
            R->Report->SetBoolField(TEXT("quick_preview"),R->Quick);R->Report->SetNumberField(TEXT("requested_scene_count"),R->SceneCount);
            R->Report->SetStringField(TEXT("variant"),R->Baseline?TEXT("accepted-v41-baseline"):TEXT("v42-environment"));R->Report->SetNumberField(TEXT("width"),Width);R->Report->SetNumberField(TEXT("height"),Height);
            R->Report->SetArrayField(TEXT("scenes"),R->SceneReports);R->Report->SetArrayField(TEXT("checks"),R->Checks);R->Report->SetBoolField(TEXT("success"),R->Passed);R->Report->SetNumberField(TEXT("elapsed_wall_seconds"),Now-R->Start);
            R->Report->SetStringField(TEXT("scope"),(R->Quick?FString(TEXT("Quick preview: first wet British field only. ")):FString(TEXT("Three seeded real campaign fields. ")))+TEXT("Normal 6000 cm wide camera and six production-spawned player-side squads: three centre, three rear. Soldier skeletal/actor ticks and weather remain active, combat director is held to preserve composition. Eight-second post-warmup environment performance, 4K Quality SR + RR, FG off, production display cap. This is not a full-combat benchmark; screenshots do not establish universal artistic quality."));
            FString Text;FJsonSerializer::Serialize(R->Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Directory/TEXT("review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FPlatformMisc::RequestExit(false);
        };
        if(Now-R->Start>190){Check(TEXT("bounded_review_timeout"),false);Finish();return false;}
        if(R->Sampling)
        {
            const double Ms=(Now-R->Last)*1000.;R->Last=Now;if(Ms>0){R->Samples.Wall.Add(Ms);R->Samples.Seconds+=Ms*.001;}
            auto Add=[](TArray<double>& V,double X){if(FMath::IsFinite(X)&&X>0)V.Add(X);};
            Add(R->Samples.GPU,FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Add(R->Samples.Game,FPlatformTime::ToMilliseconds(GGameThreadTime));
            Add(R->Samples.Render,FPlatformTime::ToMilliseconds(GRenderThreadTime));Add(R->Samples.RHI,FPlatformTime::ToMilliseconds(GRHIThreadTime));
            if(!FApp::HasFocus())++R->Samples.Unfocused;
        }
        if(Now<R->Next)return true;
        switch(R->Stage)
        {
        case 0:
        {
            FrontlineLocalizationV35::SetLanguage(FrontlineLocalizationV35::ELanguage::English);Check(TEXT("isolated_volatile_profile"),Allowed(B));
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()))
            {Render->SetHDREnabled(false,false);Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);Render->SetQuality(EFrontlineRenderQuality::Quality,false);Render->SetRayReconstruction(true,false);Render->SetFrameRateSlider(1,false);}
            if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))V->Set(0,ECVF_SetByCode);
            R->Stage=1;R->Next=Now;break;
        }
        case 1:
        {
            FrontlineReviewUIV41::Clear(PC);UGameplayStatics::SetGamePaused(PC,false);
            Check(FString(Scenes[R->Scene].Key)+TEXT("_production_setup"),FFrontlineEnvironmentReviewV42::Prepare(B,R->Scene));
            FrontlineReviewUIV41::Clear(PC);FrontlineReviewUIV41::Camera(PC,0,0,6000,650);
            Check(FString(Scenes[R->Scene].Key)+TEXT("_repeatable_mission_weather"),FrontlineLightingV42::PrepareReviewWeather(B));
            R->Current=MakeShared<FJsonObject>();R->Current->SetStringField(TEXT("key"),Scenes[R->Scene].Key);R->Current->SetNumberField(TEXT("mission"),Scenes[R->Scene].Mission);R->Current->SetNumberField(TEXT("faction"),int32(Scenes[R->Scene].Faction));
            R->StreamingViews=0;
            const auto* M=B->GetEncounterCampaignMission();if(M){R->Current->SetStringField(TEXT("mission_title"),M->Title);R->Current->SetNumberField(TEXT("mission_seed"),M->RandomSeed);R->Current->SetNumberField(TEXT("authored_wetness"),M->Wetness);}
            R->Current->SetStringField(TEXT("wide_camera"),TEXT("X0 Y0 width6000 focusZ650; accepted production camera angle"));R->Current->SetObjectField(TEXT("initial_field"),FieldInventory(B));
            R->Stage=2;R->StageStart=FPlatformTime::Seconds();R->ReadinessAt=R->StageStart;R->Next=R->StageStart+1;break;
        }
        case 2:
        {
            const double Age=Now-R->StageStart;if(Age<8){R->Next=Now+1;break;}
            const auto Inventory=FieldInventory(B);const auto* W=AWarfareFrontlineWeather::Get(B->GetWorld());const bool Ready=Inventory->GetBoolField(TEXT("ready"))&&W&&W->IsReady();
            if(!Ready&&Age<22){R->Next=FPlatformTime::Seconds()+1;break;}
            Check(FString(Scenes[R->Scene].Key)+TEXT("_warm_materials_and_weather"),Ready);R->Current->SetNumberField(TEXT("warmup_seconds"),Age);
            R->Current->SetObjectField(TEXT("warm_field"),Inventory);R->Current->SetObjectField(TEXT("lighting"),FrontlineLightingV42::Describe(B));
            FrontlineLightingV42::RunChecks(B,[&](const FString& Name,bool Good){Check(FString(Scenes[R->Scene].Key)+TEXT("_")+Name,Good);});
            if(!R->Baseline){const auto Env=FrontlineEnvironmentV42::RunChecks(B);R->Current->SetObjectField(TEXT("environment_installation"),Env);bool Good=false;Env->TryGetBoolField(TEXT("success"),Good);Check(FString(Scenes[R->Scene].Key)+TEXT("_v42_dressing_installed"),Good);}
            const auto Contract=RenderContract(B);R->Current->SetObjectField(TEXT("render_contract"),Contract);Check(FString(Scenes[R->Scene].Key)+TEXT("_quality_rr_fg_off"),Contract->GetBoolField(TEXT("fg_off_requested_and_effective"))&&Contract->GetBoolField(TEXT("quality_sr"))&&Contract->GetBoolField(TEXT("rr_requested")));
            // Loading may take different wall times in the two variants. Start
            // the measured weather phase from the same authored state, then
            // give lighting a fixed settle interval before either sample.
            B->Elapsed=20;Check(FString(Scenes[R->Scene].Key)+TEXT("_matched_sample_weather_reset"),FrontlineLightingV42::PrepareReviewWeather(B));
            R->Stage=8;R->Next=FPlatformTime::Seconds()+2;break;
        }
        case 3:
        {
            R->Sampling=false;R->Current->SetObjectField(TEXT("performance"),R->Samples.Describe());const auto EndField=FieldInventory(B);R->Current->SetObjectField(TEXT("sample_end_field"),EndField);
            R->Current->SetNumberField(TEXT("supplemental_streaming_view_submissions"),R->StreamingViews);
            if(!R->Baseline)
            {
                int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
                const auto Resolution=SoilResolutionContract(B,6000,Width);R->Current->SetObjectField(TEXT("wide_soil_resolution"),Resolution);
                Check(FString(Scenes[R->Scene].Key)+TEXT("_all_eight_soil_maps_meet_wide_projected_texel_demand"),Resolution->GetBoolField(TEXT("adequate")));
                bool PublishedSoil=false;
                for(const auto& V:EndField->GetArrayField(TEXT("static_components")))
                {
                    const auto C=V->AsObject();bool RuntimeSoil=false;
                    if(!C->TryGetBoolField(TEXT("runtime_ground_surface"),RuntimeSoil)||!RuntimeSoil)continue;
                    int32 ValidSoil=0;
                    for(const auto& Asset:C->GetArrayField(TEXT("actual_streaming_records")))
                    {
                        const auto A=Asset->AsObject();const FString Path=A->GetStringField(TEXT("asset"));
                        if((Path.Contains(TEXT("/V23/Textures/T_Mud_"))||Path.Contains(TEXT("/V42/Textures/")))
                            &&A->GetBoolField(TEXT("valid_distance_streaming"))&&A->GetNumberField(TEXT("texel_factor"))>=140&&A->GetNumberField(TEXT("texel_factor"))<300)++ValidSoil;
                    }
                    PublishedSoil|=ValidSoil==8;
                }
                Check(FString(Scenes[R->Scene].Key)+TEXT("_all_eight_soil_maps_publish_real_distance_streaming_records"),PublishedSoil);
            }
            Check(FString(Scenes[R->Scene].Key)+TEXT("_eight_seconds_actual_gpu_and_gt_samples"),R->Samples.Seconds>=7.9&&R->Samples.Wall.Num()>=60&&R->Samples.GPU.Num()>=60&&R->Samples.Game.Num()>=60);
            int32 Living=0,Shots=0;for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){Living+=U->IsAlive()?1:0;Shots+=U->ShotsDischarged;}
            Check(FString(Scenes[R->Scene].Key)+TEXT("_stable_composition_without_combat"),Living==int32(R->Current->GetObjectField(TEXT("initial_field"))->GetNumberField(TEXT("unit_count")))&&Shots==0);
            Shot(FString::Printf(TEXT("%02d-%s-wide"),R->Scene*2+1,Scenes[R->Scene].Key));R->SceneReports.Add(MakeShared<FJsonValueObject>(R->Current));R->Stage=4;R->Next=FPlatformTime::Seconds()+.8;break;
        }
        case 4:FrontlineReviewUIV41::Camera(PC,0,100,2600,100);R->Stage=5;R->Next=Now+1.5;break;
        case 5:
        {
            if(!R->Baseline)
            {
                int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
                const auto Resolution=SoilResolutionContract(B,2600,Width);R->Current->SetObjectField(TEXT("detail_soil_resolution"),Resolution);
                Check(FString(Scenes[R->Scene].Key)+TEXT("_all_eight_soil_maps_meet_detail_projected_texel_demand"),Resolution->GetBoolField(TEXT("adequate")));
            }
            Shot(FString::Printf(TEXT("%02d-%s-trench-detail"),R->Scene*2+2,Scenes[R->Scene].Key));
            if(++R->Scene<R->SceneCount){R->Stage=1;R->Next=Now+.8;}
            else {R->Stage=6;R->Next=Now+1;}break;
        }
        case 6:
            if(!R->Baseline&&FParse::Param(FCommandLine::Get(),TEXT("WFV42PathReference")))
            {
                FrontlineReviewUIV41::Camera(PC,0,0,6000,650);const bool Started=FrontlineLightingV42::BeginStaticReference(B);R->Report->SetBoolField(TEXT("static_path_reference_started"),Started);
                R->Stage=Started?7:9;R->Next=FPlatformTime::Seconds()+1;
            }
            else {R->Stage=9;R->Next=Now+.2;}break;
        case 7:
        {
            const auto Reference=FrontlineLightingV42::DescribeReference(B);R->Report->SetObjectField(TEXT("static_path_reference"),Reference);
            bool Converged=false,Timeout=false;Reference->TryGetBoolField(TEXT("converged"),Converged);Reference->TryGetBoolField(TEXT("timed_out"),Timeout);
            if(Converged){Shot(TEXT("07-static-native-path-tracer-reference"));R->Stage=9;R->Next=Now+1;}
            else if(Timeout){R->Stage=9;R->Next=Now+.1;}else R->Next=Now+.1;break;
        }
        case 8:
            R->Current->SetObjectField(TEXT("sample_start_lighting"),FrontlineLightingV42::Describe(B));
            R->Current->SetNumberField(TEXT("sample_start_game_seconds"),B->Elapsed);R->Current->SetNumberField(TEXT("fixed_weather_settle_seconds"),2);
            R->Samples=FSamplesV42();R->Sampling=true;R->SampleStart=R->Last=FPlatformTime::Seconds();R->Next=R->SampleStart+8;R->Stage=3;break;
        default:Finish();return false;
        }
        return true;
    }));
}
