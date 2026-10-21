#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineRenderSettings.h"
#include "DLSSLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "ContentStreaming.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeLock.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "RenderingThread.h"
#include "Shader.h"
#include "VertexFactory.h"
#include "UObject/StrongObjectPtr.h"
#include "PrimitiveSceneProxy.h"
#include "StaticMeshResources.h"
#include "MeshBatch.h"
#include "RayTracingInstance.h"

namespace
{
float CVar(const TCHAR* Name,float Missing=-1.f)
{
    const auto* V=IConsoleManager::Get().FindConsoleVariable(Name);
    return V?V->GetFloat():Missing;
}
bool IsIsolatedReview(const AWarfareFrontlineGameMode* B)
{
    FString UserDir;
    return B && B->IsProgressionVolatile() && (FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))) &&
        FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")) &&
        FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) && !UserDir.IsEmpty();
}
struct FViewObservation
{
    FCriticalSection Mutex;
    uint64 Frames=0,PathFrames=0;
    uint32 Samples=0,TargetSamples=0,CompletedViews=0,Resets=0;
    uint32 PausedPathViewFamilies=0,PathCameraCuts=0,PathForceResets=0,PathProjectionChanges=0;
    bool bPathTracing=false,bOrthographic=false,bStaticReference=false,bHasPathProjection=false;
    FMatrix LastPathProjection=FMatrix::Identity;
    int32 GI=0,Reflections=0;
    float ExposureMin=0,ExposureMax=0,ExposureBias=0,SkylightLeaking=0,DiffuseBoost=0;
};
class FLightingViewObserver final:public FWorldSceneViewExtension
{
public:
    FLightingViewObserver(const FAutoRegister& AutoRegister,UWorld* InWorld,TSharedRef<FViewObservation,ESPMode::ThreadSafe> Data)
        :FWorldSceneViewExtension(AutoRegister,InWorld),ReviewWorld(InWorld),Observation(Data) {}
    virtual void SetupViewFamily(FSceneViewFamily& Family) override
    {
        UWorld* ReviewSceneWorld=ReviewWorld.Get();if(!ReviewSceneWorld || !ReviewSceneWorld->IsPaused())return;
        FScopeLock Lock(&Observation->Mutex);
        if(Observation->bStaticReference && IsIsolatedReview(Cast<AWarfareFrontlineGameMode>(ReviewSceneWorld->GetAuthGameMode())))
        {
            // Keep simulation and all components frozen, but allow native view
            // history to accept the last detail -> wide camera cut. UE otherwise
            // leaves PrevFrameViewInfo read-only while paused and invalidates
            // every PT sample against the old projection (SceneVisibility.cpp).
            // Camera, geometry and material invalidation remain fully enabled.
            Family.bWorldIsPaused=false;
        }
    }
    virtual void SetupView(FSceneViewFamily&,FSceneView&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void PostRenderView_RenderThread(FRDGBuilder&,FSceneView& View) override
    {
        if(View.bIsSceneCapture || !View.Family)return;
        FScopeLock Lock(&Observation->Mutex);
        auto& O=*Observation;
        ++O.Frames;O.bPathTracing=View.Family->EngineShowFlags.PathTracing;
        O.bOrthographic=!View.IsPerspectiveProjection();
        const auto& S=View.FinalPostProcessSettings;
        O.GI=int32(S.DynamicGlobalIlluminationMethod);O.Reflections=int32(S.ReflectionMethod);
        O.ExposureMin=S.AutoExposureMinBrightness;O.ExposureMax=S.AutoExposureMaxBrightness;
        O.ExposureBias=S.AutoExposureBias;O.SkylightLeaking=S.LumenSkylightLeaking;O.DiffuseBoost=S.LumenDiffuseColorBoost;
#if RHI_RAYTRACING
        if(O.bPathTracing && View.State)
        {
            O.PausedPathViewFamilies+=View.Family->bWorldIsPaused;
            O.PathCameraCuts+=View.bCameraCut;
            O.PathForceResets+=View.bForcePathTracerReset || View.bForceCameraVisibilityReset;
            const FMatrix Projection=View.ViewMatrices.GetViewToClipNoAA();
            O.PathProjectionChanges+=O.bHasPathProjection && Projection!=O.LastPathProjection;
            O.LastPathProjection=Projection;O.bHasPathProjection=true;
            const uint32 Sample=View.State->GetPathTracingSampleIndex();
            if(Sample<O.Samples)++O.Resets;
            O.Samples=Sample;O.TargetSamples=View.State->GetPathTracingSampleCount();++O.PathFrames;
            O.CompletedViews=O.TargetSamples>=64 && O.Samples>=O.TargetSamples?O.CompletedViews+1:0;
        }
#endif
    }
private:
    TWeakObjectPtr<UWorld> ReviewWorld;
    TSharedRef<FViewObservation,ESPMode::ThreadSafe> Observation;
};
struct FReviewObservation
{
    TWeakObjectPtr<UWorld> World;
    TSharedPtr<FViewObservation,ESPMode::ThreadSafe> Data;
    TSharedPtr<ISceneViewExtension,ESPMode::ThreadSafe> Extension;
    bool bReferenceStarted=false;
    bool bReferenceRenderStatesRecreated=false;
    int32 ReferenceRecreatedComponents=0;
    double ReferenceStartedAt=0;
    TSharedPtr<FJsonObject> PathMaterialShaders;
};
FReviewObservation& ReviewObservation(){static FReviewObservation V;return V;}
TSharedRef<FJsonObject> ObservePathMaterialShadersV42(UWorld* World)
{
    struct FPrimitiveRow {FPrimitiveSceneProxy* Proxy=nullptr;TSharedPtr<FJsonObject> Data;FString BatchScope;};
    auto Primitives=MakeShared<TArray<FPrimitiveRow>,ESPMode::ThreadSafe>();
    TArray<TStrongObjectPtr<UStaticMeshComponent>> PrimitiveKeepAlive;
    struct FMaterialRow
    {
        FString Path,Base;
        const FMaterialRenderProxy* Proxy=nullptr;
        bool Original=false,Valid=false;
        int32 Shaders=0,Pipelines=0;
        TArray<FString> PathHitShaders;
    };
    TMap<UMaterial*,UMaterialInterface*> Representatives;
    bool SelectedGround=false,SelectedV42=false,SelectedV40=false;
    for(TActorIterator<AActor> It(World);It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Components(*It);
        for(auto* C:Components)
        {
            const auto* ISM=Cast<UInstancedStaticMeshComponent>(C);
            // Include hidden and main-pass-disabled components: these were
            // deliberately absent from the live visible-material inventory.
            auto Item=MakeShared<FJsonObject>();UStaticMesh* Mesh=C->GetStaticMesh();
            Item->SetStringField(TEXT("actor"),It->GetPathName());Item->SetStringField(TEXT("component"),C->GetPathName());
            Item->SetStringField(TEXT("mesh"),GetPathNameSafe(Mesh));Item->SetStringField(TEXT("bounds"),C->Bounds.ToString());
            Item->SetStringField(TEXT("location"),C->GetComponentLocation().ToString());Item->SetStringField(TEXT("scale"),C->GetComponentScale().ToString());
            Item->SetBoolField(TEXT("actor_hidden"),It->IsHidden());Item->SetBoolField(TEXT("component_hidden_in_game"),C->bHiddenInGame);
            Item->SetBoolField(TEXT("component_visible"),C->IsVisible());Item->SetBoolField(TEXT("registered"),C->IsRegistered());
            Item->SetBoolField(TEXT("visible_in_ray_tracing"),C->bVisibleInRayTracing);Item->SetBoolField(TEXT("main_pass"),C->bRenderInMainPass);
            Item->SetBoolField(TEXT("depth_pass"),C->bRenderInDepthPass);Item->SetBoolField(TEXT("casts_hidden_shadow"),C->bCastHiddenShadow);
            Item->SetBoolField(TEXT("indirect_lighting_while_hidden"),C->bAffectIndirectLightingWhileHidden);
            Item->SetNumberField(TEXT("instances"),ISM?ISM->GetInstanceCount():1);
            TArray<TSharedPtr<FJsonValue>> Defaults,Assigned,Sections,Tags;
            for(const FName Tag:It->Tags)Tags.Add(MakeShared<FJsonValueString>(Tag.ToString()));Item->SetArrayField(TEXT("actor_tags"),Tags);
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)Assigned.Add(MakeShared<FJsonValueString>(GetPathNameSafe(C->GetMaterial(Slot))));
            if(Mesh)
            {
                for(const auto& Slot:Mesh->GetStaticMaterials())Defaults.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Slot.MaterialInterface.Get())));
                if(const auto* Data=Mesh->GetRenderData())for(int32 Lod=0;Lod<Data->LODResources.Num();++Lod)
                    for(const auto& Section:Data->LODResources[Lod].Sections)
                    {
                        auto S=MakeShared<FJsonObject>();S->SetNumberField(TEXT("lod"),Lod);S->SetNumberField(TEXT("material_index"),Section.MaterialIndex);
                        S->SetNumberField(TEXT("triangles"),Section.NumTriangles);Sections.Add(MakeShared<FJsonValueObject>(S));
                    }
            }
            Item->SetArrayField(TEXT("mesh_default_materials"),Defaults);Item->SetArrayField(TEXT("assigned_materials"),Assigned);Item->SetArrayField(TEXT("mesh_sections"),Sections);
            FString BatchScope;
            const bool Visible=C->IsRegistered()&&C->IsVisible()&&!It->IsHidden()&&Mesh&&(!ISM||ISM->GetInstanceCount()>0);
            if(Visible && It->IsA<AWarfareFrontlineGround>() && C->GetFName()==TEXT("Terrain") && !SelectedGround)
                {SelectedGround=true;BatchScope=TEXT("current_ground");}
            else if(Visible && ISM && Mesh->GetPathName().Contains(TEXT("/Field/V42/")) && !SelectedV42)
                {SelectedV42=true;BatchScope=TEXT("one_v42_environment_ism");}
            else if(Visible && ISM && Mesh->GetPathName().Contains(TEXT("/Field/V40/")) && !SelectedV40)
                {SelectedV40=true;BatchScope=TEXT("one_v40_environment_ism");}
            if(!BatchScope.IsEmpty())
            {
                Item->SetStringField(TEXT("selected_rt_batch_scope"),BatchScope);
                Item->SetBoolField(TEXT("component_pso_precaching"),C->IsPSOPrecaching());
                Item->SetBoolField(TEXT("component_pso_fallback_requested"),C->GetPSOPrecacheComponentData().UsePSOPrecacheFallbackMaterial());
            }
            PrimitiveKeepAlive.Emplace(C);Primitives->Add({C->GetSceneProxy(),Item,BatchScope});
            if(!C->IsRegistered()||!C->IsVisible()||!C->GetStaticMesh()||
                It->IsHidden()||(!It->IsA<AWarfareFrontlineGround>()&&!ISM)||(ISM&&ISM->GetInstanceCount()==0))continue;
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
                if(auto* M=C->GetMaterial(Slot))if(auto* Base=M->GetMaterial())Representatives.FindOrAdd(Base,M);
        }
    }
    TArray<TStrongObjectPtr<UMaterialInterface>> KeepAlive;
    auto Rows=MakeShared<TArray<FMaterialRow>,ESPMode::ThreadSafe>();
    for(const auto& Pair:Representatives)
    {
        KeepAlive.Emplace(Pair.Value);FMaterialRow Row;
        Row.Path=Pair.Value->GetPathName();Row.Base=Pair.Key->GetPathName();Row.Proxy=Pair.Value->GetRenderProxy();Rows->Add(MoveTemp(Row));
    }
    const auto FeatureLevel=World->GetFeatureLevel();
    ENQUEUE_RENDER_COMMAND(FrontlineV42ObserveAuthoredPathHitShaders)([Rows,Primitives,FeatureLevel](FRHICommandListImmediate&)
    {
        for(auto& P:*Primitives)
        {
            P.Data->SetBoolField(TEXT("render_proxy_present"),P.Proxy!=nullptr);if(!P.Proxy)continue;
            P.Data->SetBoolField(TEXT("proxy_drawn_in_game"),P.Proxy->IsDrawnInGame());
            P.Data->SetBoolField(TEXT("proxy_main_pass"),P.Proxy->ShouldRenderInMainPass());
            P.Data->SetBoolField(TEXT("proxy_retains_when_hidden"),P.Proxy->CastsHiddenShadow()||P.Proxy->AffectsIndirectLightingWhileHidden());
#if RHI_RAYTRACING
            P.Data->SetBoolField(TEXT("proxy_visible_in_ray_tracing"),P.Proxy->IsVisibleInRayTracing());
            P.Data->SetBoolField(TEXT("proxy_has_ray_tracing_representation"),P.Proxy->HasRayTracingRepresentation());
            if(!P.BatchScope.IsEmpty())
            {
                const ERayTracingPrimitiveFlags Flags=P.Proxy->GetRayTracingPrimitiveFlags();
                P.Data->SetNumberField(TEXT("actual_rt_primitive_flags"),uint8(Flags));
                const bool CanPoll=EnumHasAnyFlags(Flags,ERayTracingPrimitiveFlags::CacheInstances|ERayTracingPrimitiveFlags::ComputeLOD)&&
                    !EnumHasAnyFlags(Flags,ERayTracingPrimitiveFlags::Dynamic|ERayTracingPrimitiveFlags::Exclude|ERayTracingPrimitiveFlags::UnsupportedProxyType|ERayTracingPrimitiveFlags::Skip);
                P.Data->SetBoolField(TEXT("actual_static_rt_batches_polled"),CanPoll);
                TArray<TSharedPtr<FJsonValue>> Batches;
                if(CanPoll)
                {
                    FStaticRayTracingInstances Instances;P.Proxy->GetStaticRayTracingInstances(Instances);
                    for(int32 LODIndex=0;LODIndex<Instances.LODs.Num();++LODIndex)
                        for(const auto& Instance:Instances.LODs[LODIndex].Instances)
                            for(const FMeshBatch& Batch:Instance.GetMaterials())
                            {
                                auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("lod"),LODIndex);
                                Entry->SetNumberField(TEXT("segment"),Batch.SegmentIndex);Entry->SetBoolField(TEXT("use_for_material"),Batch.bUseForMaterial);
                                Entry->SetBoolField(TEXT("geometry_present"),Instance.Geometry!=nullptr);
                                const FVertexFactoryType* VF=Batch.VertexFactory?Batch.VertexFactory->GetType():nullptr;
                                Entry->SetStringField(TEXT("vertex_factory"),VF?VF->GetName():TEXT("none"));
                                Entry->SetStringField(TEXT("bound_material_proxy"),Batch.MaterialRenderProxy?Batch.MaterialRenderProxy->GetMaterialName():TEXT("none"));
                                TArray<TSharedPtr<FJsonValue>> Chain;int32 Depth=0;
                                for(const FMaterialRenderProxy* Proxy=Batch.MaterialRenderProxy;Proxy&&Depth<4;Proxy=Proxy->GetFallback(FeatureLevel),++Depth)
                                {
                                    auto Link=MakeShared<FJsonObject>();Link->SetNumberField(TEXT("fallback_depth"),Depth);
                                    Link->SetStringField(TEXT("material_proxy"),Proxy->GetMaterialName());
                                    const FMaterial* Material=Proxy->GetMaterialNoFallback(FeatureLevel);
                                    Link->SetStringField(TEXT("render_material"),Material?Material->GetFriendlyName():TEXT("none"));
                                    const auto* Map=Material?Material->GetRenderingThreadShaderMap():nullptr;
                                    Link->SetBoolField(TEXT("render_map_valid"),Map&&Map->IsValidForRendering());
                                    TArray<TSharedPtr<FJsonValue>> Hits;
                                    if(Map)
                                    {
                                        TMap<FShaderId,TShaderRef<FShader>> ShaderList;Map->GetShaderList(ShaderList);
                                        for(const auto& ShaderEntry:ShaderList)
                                        {
                                            const FShaderId& Id=ShaderEntry.Key;const FString Name=Id.Type?Id.Type->GetName():TEXT("");
                                            if(Id.VFType==VF&&Name.Contains(TEXT("PathTracingMaterial"))&&Name.Contains(TEXT("CHS")))
                                                Hits.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("%s | permutation=%d"),*Name,Id.PermutationId)));
                                        }
                                    }
                                    Link->SetArrayField(TEXT("path_hit_shaders_for_actual_vertex_factory"),Hits);Chain.Add(MakeShared<FJsonValueObject>(Link));
                                }
                                Entry->SetArrayField(TEXT("original_and_fallback_chain"),Chain);Batches.Add(MakeShared<FJsonValueObject>(Entry));
                            }
                }
                P.Data->SetArrayField(TEXT("actual_static_rt_mesh_batches"),Batches);
            }
#endif
        }
        for(auto& Row:*Rows)
        {
            const FMaterial* Material=Row.Proxy?Row.Proxy->GetMaterialNoFallback(FeatureLevel):nullptr;
            Row.Original=Material!=nullptr;const auto* Map=Material?Material->GetRenderingThreadShaderMap():nullptr;
            Row.Valid=Map&&Map->IsValidForRendering();if(!Map)continue;
            Map->CountNumShaders(Row.Shaders,Row.Pipelines);
            TMap<FShaderId,TShaderRef<FShader>> Shaders;Map->GetShaderList(Shaders);
            for(const auto& Entry:Shaders)
            {
                const FShaderId& Id=Entry.Key;
                const FString Type=Id.Type?Id.Type->GetName():TEXT("");
                if(Type.Contains(TEXT("PathTracingMaterial"))&&Type.Contains(TEXT("CHS")))
                    Row.PathHitShaders.AddUnique(FString::Printf(TEXT("%s | %s | permutation=%d"),*Type,
                        Id.VFType?Id.VFType->GetName():TEXT("no vertex factory"),Id.PermutationId));
            }
            Row.PathHitShaders.Sort();
        }
    });
    // One isolated reference-only observation after live GPU sampling. Retain
    // UObjects until completion; never drain the render queue during gameplay.
    FlushRenderingCommands();
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Results;int32 WithPathShaders=0;
    for(const auto& Row:*Rows)
    {
        auto M=MakeShared<FJsonObject>();M->SetStringField(TEXT("assigned_representative"),Row.Path);M->SetStringField(TEXT("base_material"),Row.Base);
        M->SetBoolField(TEXT("original_render_material"),Row.Original);M->SetBoolField(TEXT("render_map_valid"),Row.Valid);
        M->SetNumberField(TEXT("shader_count"),Row.Shaders);M->SetNumberField(TEXT("pipeline_count"),Row.Pipelines);
        TArray<TSharedPtr<FJsonValue>> Hits;for(const auto& Hit:Row.PathHitShaders)Hits.Add(MakeShared<FJsonValueString>(Hit));
        M->SetArrayField(TEXT("path_hit_shaders_and_vertex_factories"),Hits);WithPathShaders+=!Hits.IsEmpty();Results.Add(MakeShared<FJsonValueObject>(M));
    }
    J->SetArrayField(TEXT("materials"),Results);J->SetNumberField(TEXT("unique_assigned_base_materials"),Rows->Num());
    J->SetNumberField(TEXT("materials_with_path_hit_permutations"),WithPathShaders);
    TArray<TSharedPtr<FJsonValue>> CandidateRows;for(const auto& P:*Primitives)CandidateRows.Add(MakeShared<FJsonValueObject>(P.Data));
    J->SetArrayField(TEXT("all_static_component_rt_candidates"),CandidateRows);
    J->SetNumberField(TEXT("path_substrate_compile_simplified"),CVar(TEXT("r.PathTracing.Substrate.CompileSimplifiedMaterials")));
    J->SetNumberField(TEXT("path_substrate_use_simplified"),CVar(TEXT("r.PathTracing.Substrate.UseSimplifiedMaterials")));
    J->SetNumberField(TEXT("substrate_enabled"),CVar(TEXT("r.Substrate")));
    J->SetStringField(TEXT("selected_rt_batch_scope"),TEXT("One current ground, one visible V42 ISM and one visible V40 ISM, polled only for supported static RT proxies. Actual batch material proxy, vertex factory and original/fallback CHS content; this records generated batches, not final shader-table selection."));
    J->SetStringField(TEXT("rt_candidate_scope"),TEXT("All static-mesh components, including hidden and main-pass-disabled candidates; original component and render-proxy flags, world bounds, default/assigned materials and section indices. This is eligibility inventory, not a claim that every component entered the TLAS."));
    J->SetStringField(TEXT("scope"),TEXT("One original-proxy observation before static path tracing; one actually assigned representative per base material on visible ground/ISM components. Shader names include vertex factory and permutation; presence does not by itself validate rendered surfaces."));
    return J;
}
TSharedRef<FJsonObject> ObservedView()
{
    auto J=MakeShared<FJsonObject>();auto& Review=ReviewObservation();
    if(!Review.Data)
    {
        J->SetBoolField(TEXT("observed"),false);J->SetBoolField(TEXT("orthographic"),false);
        J->SetBoolField(TEXT("native_path_tracing"),false);J->SetBoolField(TEXT("path_converged"),false);
        for(const auto* N:{TEXT("gi_method"),TEXT("reflection_method"),TEXT("exposure_min_ev100"),TEXT("exposure_max_ev100"),TEXT("exposure_bias"),TEXT("skylight_leaking"),TEXT("diffuse_color_boost")})J->SetNumberField(N,-1);
        return J;
    }
    FScopeLock Lock(&Review.Data->Mutex);const auto& O=*Review.Data;
    J->SetBoolField(TEXT("observed"),O.Frames>0);J->SetNumberField(TEXT("rendered_views"),double(O.Frames));
    J->SetBoolField(TEXT("native_path_tracing"),O.bPathTracing);J->SetBoolField(TEXT("orthographic"),O.bOrthographic);
    J->SetNumberField(TEXT("gi_method"),O.GI);J->SetNumberField(TEXT("reflection_method"),O.Reflections);
    J->SetNumberField(TEXT("exposure_min_ev100"),O.ExposureMin);J->SetNumberField(TEXT("exposure_max_ev100"),O.ExposureMax);
    J->SetNumberField(TEXT("exposure_bias"),O.ExposureBias);J->SetNumberField(TEXT("skylight_leaking"),O.SkylightLeaking);
    J->SetNumberField(TEXT("diffuse_color_boost"),O.DiffuseBoost);
    J->SetNumberField(TEXT("path_sample_index"),O.Samples);J->SetNumberField(TEXT("path_sample_target"),O.TargetSamples);
    J->SetNumberField(TEXT("path_views"),double(O.PathFrames));J->SetNumberField(TEXT("path_resets"),O.Resets);
    J->SetStringField(TEXT("path_resets_scope"),TEXT("Observed sample-index decreases only; a reset and resample between observations can retain index 1."));
    J->SetNumberField(TEXT("path_paused_view_families"),O.PausedPathViewFamilies);
    J->SetNumberField(TEXT("path_camera_cut_views"),O.PathCameraCuts);J->SetNumberField(TEXT("path_forced_reset_views"),O.PathForceResets);
    J->SetNumberField(TEXT("path_projection_changes"),O.PathProjectionChanges);
    if(O.bHasPathProjection && O.bOrthographic && FMath::Abs(O.LastPathProjection.M[0][0])>SMALL_NUMBER)
        J->SetNumberField(TEXT("path_observed_ortho_width_cm"),2./FMath::Abs(O.LastPathProjection.M[0][0]));
    // A second observed completed view avoids capturing the preceding frame
    // before the final sample and its denoiser output have reached the viewport.
    J->SetBoolField(TEXT("path_converged"),O.bPathTracing && O.CompletedViews>=2);
    return J;
}
APostProcessVolume* WeatherGrade(UWorld* World)
{
    APostProcessVolume* Result=nullptr;
    if(World)for(TActorIterator<APostProcessVolume> It(World);It;++It)
        if(It->bUnbound && It->bEnabled && It->BlendWeight>0 && (!Result || It->Priority>Result->Priority))Result=*It;
    return Result;
}
}

bool FrontlineLightingV42::IsBaseline()
{
    return FParse::Param(FCommandLine::Get(),TEXT("WFV42Review")) && FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"));
}
bool FrontlineLightingV42::IsV84Baseline()
{
    FString UserDir;
    return FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV84LightingBaseline"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
bool FrontlineLightingV42::IsV842Baseline()
{
    FString UserDir;
    return FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV842LightingBaseline"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
void FrontlineLightingV42::ApplyGrade(FPostProcessSettings& S)
{
    const bool Old=IsBaseline();
    // Work on the captured irradiance and material separation, not HDR output
    // or black-point compensation. Fixed scene exposure remains unchanged.
    S.bOverride_ColorSaturation=true;S.ColorSaturation=Old?FVector4(.76f,.77f,.79f,1):FVector4(.84f,.84f,.84f,1);
    S.bOverride_ColorContrast=true;S.ColorContrast=Old?FVector4(1.045f,1.045f,1.04f,1):FVector4(1.025f,1.025f,1.025f,1);
    S.bOverride_ColorGamma=true;S.ColorGamma=Old?FVector4(1.025f,1.025f,1.02f,1):FVector4(1.015f,1.015f,1.015f,1);
    S.bOverride_ColorGain=true;S.ColorGain=Old?FVector4(.97f,.985f,1,1):FVector4(1.f,1.f,1.f,1);
    if(!Old)
    {
        // Scene-linear zones preserve sun/fire colour and neutral specular
        // highlights. Authored UI paintings have a separate display transform.
        S.bOverride_ColorSaturationShadows=true;S.ColorSaturationShadows=FVector4(.84f,.84f,.84f,1);
        S.bOverride_ColorGainShadows=true;S.ColorGainShadows=FVector4(.976f,1.006f,1.018f,1);
        S.bOverride_ColorGainMidtones=true;S.ColorGainMidtones=FVector4(.991f,1.003f,1.008f,1);
        S.bOverride_ColorSaturationHighlights=true;S.ColorSaturationHighlights=FVector4(1.04f,1.04f,1.04f,1);
        S.bOverride_ColorGainHighlights=true;S.ColorGainHighlights=FVector4(1.01f,1.002f,.990f,1);
    }
    S.bOverride_VignetteIntensity=true;S.VignetteIntensity=Old?.25f:.19f;
}
void FrontlineLightingV42::ApplyLights(UDirectionalLightComponent* Sun,USkyLightComponent* Sky,float Storm)
{
    Storm=FMath::Clamp(Storm,0.f,1.f);const bool Old=IsBaseline();
    if(Sun)
    {
        Sun->SetIntensity(Old?FMath::Lerp(2.35f,1.55f,Storm):FMath::Lerp(2.55f,1.90f,Storm));
        Sun->SetLightColor(Old?FLinearColor(.90f,.94f,1):FLinearColor(.95f,.97f,1));
    }
    if(Sky)
    {
        // Preserve a readable key/fill ratio on the clay geometry. Excessive
        // uniform sky fill previously made shaded hollows as bright as ridges.
        Sky->SetIntensity(Old?FMath::Lerp(.245f,.205f,Storm):FMath::Lerp(.30f,.28f,Storm));
        Sky->SetLightColor(Old?FLinearColor(.86f,.91f,1):FLinearColor(.93f,.96f,1));
    }
}
float FrontlineLightingV42::BackdropGain(float Storm)
{
    return IsBaseline()?FMath::Lerp(.67f,.47f,Storm):FMath::Lerp(1.02f,.86f,Storm);
}
void FrontlineLightingV42::ObserveReviewWorld(UWorld* World)
{
    if(!World || !(FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))))return;
    auto& R=ReviewObservation();if(R.World==World && R.Extension)return;
    R.Extension.Reset();R.Data=MakeShared<FViewObservation,ESPMode::ThreadSafe>();R.World=World;
    R.bReferenceStarted=false;R.ReferenceStartedAt=0;R.bReferenceRenderStatesRecreated=false;R.ReferenceRecreatedComponents=0;
    R.Extension=FSceneViewExtensions::NewExtension<FLightingViewObserver>(World,R.Data.ToSharedRef());
}
bool FrontlineLightingV42::PrepareReviewWeather(AWarfareFrontlineGameMode* B)
{
    if(!IsIsolatedReview(B))return false;
    auto* Weather=AWarfareFrontlineWeather::Get(B->GetWorld());if(!Weather)return false;
    Weather->SnapToWeatherForReview();ObserveReviewWorld(B->GetWorld());return true;
}
TSharedRef<FJsonObject> FrontlineLightingV42::DescribeLiveReflectionBindingsV44(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();const bool Allowed=IsIsolatedReview(B);
    J->SetBoolField(TEXT("isolated_review"),Allowed);if(!Allowed)return J;
    J->SetStringField(TEXT("scope"),TEXT("Three existing visible mesh components, generated static RT batches and original/fallback shader maps. No final SBT selection claim; no pause, renderer-mode switch, recompilation or CVar change."));
    auto Vars=MakeShared<FJsonObject>();
    for(const TCHAR* N:{TEXT("r.RayTracing.UseTextureLod"),TEXT("r.AllowStaticLighting"),TEXT("r.MaterialQualityLevel"),TEXT("r.RayTracing.NonBlockingPipelineCreation"),TEXT("r.Lumen.Reflections.ScreenTraces"),TEXT("r.Lumen.Reflections.SampleSceneColorAtHit"),TEXT("r.RayTracing.Nanite.Mode")})Vars->SetNumberField(N,CVar(N));
    J->SetObjectField(TEXT("selection_cvars"),Vars);
    UStaticMeshComponent* Selected[3]={};double Best[3]={DBL_MAX,DBL_MAX,DBL_MAX};
    const FVector SamplePoint(0,1000,0); // The V43 fixed wet-field view, after its timed sample.
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(It->IsHidden())continue;TInlineComponentArray<UStaticMeshComponent*> Components(*It);
        for(auto* C:Components)
        {
            UStaticMesh* Mesh=C->GetStaticMesh();
            if(!Mesh||!C->IsRegistered()||!C->IsVisible()||C->bHiddenInGame||!C->bRenderInMainPass)continue;
            const FString Path=Mesh->GetPathName(),Name=Mesh->GetName();
            // Follow the live authored mesh family across operation/version bakes.
            // A version-specific path silently omitted V45 ground and water.
            const bool FieldMesh=Path.Contains(TEXT("/Warfare/Field/"));
            const int32 Kind=FieldMesh&&Name.StartsWith(TEXT("SM_Field_"))?0:Name.StartsWith(TEXT("SM_TrenchEarth_"))?1:FieldMesh&&Name.StartsWith(TEXT("SM_Basins_"))?2:INDEX_NONE;
            if(Kind==INDEX_NONE)continue;
            const double Distance=C->Bounds.GetBox().ComputeSquaredDistanceToPoint(SamplePoint);
            if(Distance<Best[Kind]){Best[Kind]=Distance;Selected[Kind]=C;}
        }
    }
    struct FReadback {FPrimitiveSceneProxy* Proxy=nullptr;TSharedPtr<FJsonObject> Row;const FMaterialRenderProxy* AssignedMaterial=nullptr;};
    auto Rows=MakeShared<TArray<FReadback>,ESPMode::ThreadSafe>();TArray<TStrongObjectPtr<UStaticMeshComponent>> KeepAlive;
    const TCHAR* Names[]={TEXT("visible_ground_tile"),TEXT("visible_earth_bank"),TEXT("basin_water")};
    for(int32 I=0;I<3;++I)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("scope"),Names[I]);auto* C=Selected[I];Row->SetBoolField(TEXT("found"),C!=nullptr);
        if(C)
        {
            KeepAlive.Emplace(C);Row->SetStringField(TEXT("component"),C->GetPathName());Row->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());
            Row->SetStringField(TEXT("bounds"),C->Bounds.ToString());Row->SetBoolField(TEXT("component_rt_visible"),C->bVisibleInRayTracing);
            Row->SetStringField(TEXT("assigned_material_slot_zero"),GetPathNameSafe(C->GetMaterial(0)));
            Row->SetStringField(TEXT("mesh_default_material_slot_zero"),GetPathNameSafe(C->GetStaticMesh()->GetMaterial(0)));
            Row->SetBoolField(TEXT("component_pso_precaching"),C->IsPSOPrecaching());
            Row->SetBoolField(TEXT("component_pso_fallback_requested"),C->GetPSOPrecacheComponentData().UsePSOPrecacheFallbackMaterial());
        }
        Rows->Add({C?C->GetSceneProxy():nullptr,Row,C&&C->GetMaterial(0)?C->GetMaterial(0)->GetRenderProxy():nullptr});
    }
    const auto Feature=B->GetWorld()->GetFeatureLevel();const bool TextureLOD=CVar(TEXT("r.RayTracing.UseTextureLod"))!=0;
    const bool NoStaticLighting=CVar(TEXT("r.AllowStaticLighting"))==0;
    ENQUEUE_RENDER_COMMAND(FrontlineLiveReflectionBindingsV44)([Rows,Feature,TextureLOD,NoStaticLighting](FRHICommandListImmediate&)
    {
        for(auto& Entry:*Rows)
        {
            auto& Row=Entry.Row;auto* Proxy=Entry.Proxy;Row->SetBoolField(TEXT("render_proxy_present"),Proxy!=nullptr);if(!Proxy)continue;
            Row->SetBoolField(TEXT("raster_proxy_is_nanite"),Proxy->IsNaniteMesh());
            TArray<TSharedPtr<FJsonValue>> RasterChain;int32 RasterDepth=0;
            for(const auto* P=Entry.AssignedMaterial;P&&RasterDepth<4;P=P->GetFallback(Feature),++RasterDepth)
            {
                auto Link=MakeShared<FJsonObject>();Link->SetNumberField(TEXT("fallback_depth"),RasterDepth);Link->SetStringField(TEXT("material_proxy"),P->GetMaterialName());
                const auto* M=P->GetMaterialNoFallback(Feature);const auto* Map=M?M->GetRenderingThreadShaderMap():nullptr;
                Link->SetStringField(TEXT("render_material"),M?M->GetFriendlyName():TEXT("missing"));Link->SetBoolField(TEXT("render_map_valid"),Map&&Map->IsValidForRendering());
                const auto* NaniteVF=FindVertexFactoryType(TEXT("FNaniteVertexFactory"));
                Link->SetBoolField(TEXT("material_nanite_usage"),M&&M->IsUsedWithNanite());
                Link->SetBoolField(TEXT("supports_nanite_rendering"),M&&NaniteVF&&SupportsNaniteRendering(NaniteVF,*M,Feature));
                TArray<TSharedPtr<FJsonValue>> RasterShaders;
                if(Map)
                {
                    TMap<FShaderId,TShaderRef<FShader>> Shaders;Map->GetShaderList(Shaders);
                    for(const auto& Pair:Shaders)
                    {
                        const auto& Id=Pair.Key;const FString Type=Id.Type?Id.Type->GetName():TEXT(""),VF=Id.VFType?Id.VFType->GetName():TEXT("none");
                        if(!Type.Contains(TEXT("CHS"))&&(VF.Contains(TEXT("Nanite"))||Type.Contains(TEXT("BasePass"))||Type.Contains(TEXT("Rasterize"))))
                            RasterShaders.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("%s | %s | permutation=%d"),*Type,*VF,Id.PermutationId)));
                    }
                }
                Link->SetArrayField(TEXT("available_raster_types_vfs_permutations"),RasterShaders);RasterChain.Add(MakeShared<FJsonValueObject>(Link));
            }
            Row->SetArrayField(TEXT("assigned_raster_material_eligibility_chain"),RasterChain);
#if RHI_RAYTRACING
            Row->SetBoolField(TEXT("proxy_rt_visible"),Proxy->IsVisibleInRayTracing());
            const auto Flags=Proxy->GetRayTracingPrimitiveFlags();Row->SetNumberField(TEXT("rt_primitive_flags"),uint8(Flags));
            const bool CanPoll=EnumHasAnyFlags(Flags,ERayTracingPrimitiveFlags::CacheInstances|ERayTracingPrimitiveFlags::ComputeLOD)&&
                !EnumHasAnyFlags(Flags,ERayTracingPrimitiveFlags::Dynamic|ERayTracingPrimitiveFlags::Exclude|ERayTracingPrimitiveFlags::UnsupportedProxyType|ERayTracingPrimitiveFlags::Skip);
            Row->SetBoolField(TEXT("static_batches_polled"),CanPoll);if(!CanPoll)continue;
            FStaticRayTracingInstances Instances;Proxy->GetStaticRayTracingInstances(Instances);TArray<TSharedPtr<FJsonValue>> Batches;
            for(int32 LOD=0;LOD<Instances.LODs.Num();++LOD)for(const auto& Instance:Instances.LODs[LOD].Instances)
                for(const FMeshBatch& Batch:Instance.GetMaterials())
                {
                    auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("lod"),LOD);Item->SetNumberField(TEXT("segment"),Batch.SegmentIndex);
                    Item->SetBoolField(TEXT("geometry_present"),Instance.Geometry!=nullptr);Item->SetBoolField(TEXT("use_for_material"),Batch.bUseForMaterial);
                    const auto* VF=Batch.VertexFactory?Batch.VertexFactory->GetType():nullptr;
                    Item->SetStringField(TEXT("vertex_factory"),VF?VF->GetName():TEXT("missing"));
                    Item->SetStringField(TEXT("bound_material_proxy"),Batch.MaterialRenderProxy?Batch.MaterialRenderProxy->GetMaterialName():TEXT("missing"));
                    TArray<TSharedPtr<FJsonValue>> Chain;int32 Depth=0;
                    for(const FMaterialRenderProxy* P=Batch.MaterialRenderProxy;P&&Depth<4;P=P->GetFallback(Feature),++Depth)
                    {
                        auto Link=MakeShared<FJsonObject>();Link->SetNumberField(TEXT("fallback_depth"),Depth);Link->SetStringField(TEXT("material_proxy"),P->GetMaterialName());
                        const auto* Material=P->GetMaterialNoFallback(Feature);const auto* Map=Material?Material->GetRenderingThreadShaderMap():nullptr;
                        Link->SetStringField(TEXT("render_material"),Material?Material->GetFriendlyName():TEXT("missing"));Link->SetBoolField(TEXT("render_map_valid"),Map&&Map->IsValidForRendering());
                        const bool Known=Material&&VF&&NoStaticLighting&&Material->GetBlendMode()==BLEND_Opaque&&!VF->SupportsRayTracingProceduralPrimitive();
                        const FString Required=TextureLOD?TEXT("TMaterialCHSLodFNoLightMapPolicy"):TEXT("TMaterialCHSFNoLightMapPolicy");
                        Link->SetBoolField(TEXT("required_type_contract_known"),Known);Link->SetStringField(TEXT("required_live_hit_shader"),Known?Required:TEXT("see CHS list; non-opaque/procedural/static-lighting contract"));
                        bool Found=false;TArray<TSharedPtr<FJsonValue>> Hits,MatchingVF;
                        if(Map)
                        {
                            TMap<FShaderId,TShaderRef<FShader>> Shaders;Map->GetShaderList(Shaders);
                            for(const auto& Pair:Shaders)
                            {
                                const auto& Id=Pair.Key;const FString Type=Id.Type?Id.Type->GetName():TEXT("");if(!Type.Contains(TEXT("CHS")))continue;
                                const FString Label=FString::Printf(TEXT("%s | %s | permutation=%d"),*Type,Id.VFType?Id.VFType->GetName():TEXT("none"),Id.PermutationId);
                                Hits.Add(MakeShared<FJsonValueString>(Label));
                                if(Id.VFType==VF){MatchingVF.Add(MakeShared<FJsonValueString>(Label));Found|=Type==Required&&Id.PermutationId==0;}
                            }
                        }
                        // Read the existing map only: TryGetShaders/HasShaders can
                        // enqueue editor ODSC jobs and would alter this diagnosis.
                        Link->SetBoolField(TEXT("required_live_hit_shader_present"),Known&&Found);
                        Link->SetArrayField(TEXT("all_chs_types_vfs_permutations"),Hits);Link->SetArrayField(TEXT("chs_for_actual_vertex_factory"),MatchingVF);Chain.Add(MakeShared<FJsonValueObject>(Link));
                    }
                    Item->SetArrayField(TEXT("original_and_fallback_chain"),Chain);Batches.Add(MakeShared<FJsonValueObject>(Item));
                }
            Row->SetArrayField(TEXT("actual_static_rt_batches"),Batches);
#endif
        }
    });
    FlushRenderingCommands();TArray<TSharedPtr<FJsonValue>> Result;
    for(const auto& R:*Rows)Result.Add(MakeShared<FJsonValueObject>(R.Row));J->SetArrayField(TEXT("components"),Result);
    return J;
}

TSharedRef<FJsonObject> FrontlineLightingV42::Describe(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();UWorld* World=B?B->GetWorld():nullptr;ObserveReviewWorld(World);
    J->SetStringField(TEXT("profile"),IsBaseline()?TEXT("V41 baseline"):TEXT("V42 overcast material readability"));
    J->SetStringField(TEXT("live_architecture"),TEXT("Deferred Lumen; hardware RT hit lighting for reflections when supported; GI and secondary bounce surface cache; virtual shadow maps."));
    J->SetBoolField(TEXT("baseline"),IsBaseline());J->SetObjectField(TEXT("actual_view"),ObservedView());
    const auto* Weather=AWarfareFrontlineWeather::Get(World);
    J->SetBoolField(TEXT("weather_ready"),Weather && Weather->IsReady());
    J->SetBoolField(TEXT("weather_drives_displayed_ground"),false);
    J->SetNumberField(TEXT("cloud_cover"),0);J->SetNumberField(TEXT("sky_intensity"),0);
    if(Weather)
    {
        J->SetNumberField(TEXT("cloud_cover"),Weather->GetCloudCover());J->SetNumberField(TEXT("wetness"),Weather->GetWetness());
        J->SetNumberField(TEXT("rain"),Weather->GetRainAmount());J->SetNumberField(TEXT("sun_intensity"),Weather->GetSunIntensity());
        J->SetNumberField(TEXT("sky_intensity"),Weather->GetSkyIntensity());
        J->SetBoolField(TEXT("weather_drives_displayed_ground"),Weather->UsesDisplayedGroundMaterial());
    }
    int32 KeyLights=0,SkyLights=0;TArray<TSharedPtr<FJsonValue>> MatteRows;
    bool MattePrimaryOnly=true;
    if(World)for(TActorIterator<AActor> It(World);It;++It)
    {
        if(const auto* Sun=It->FindComponentByClass<UDirectionalLightComponent>())
        {++KeyLights;J->SetStringField(TEXT("sun_color"),Sun->GetLightColor().ToString());J->SetNumberField(TEXT("source_angle_degrees"),Sun->LightSourceAngle);}
        if(const auto* Sky=It->FindComponentByClass<USkyLightComponent>())
        {++SkyLights;J->SetStringField(TEXT("sky_color"),Sky->GetLightColor().ToString());J->SetStringField(TEXT("sky_cubemap"),GetPathNameSafe(Sky->Cubemap));J->SetBoolField(TEXT("sky_realtime_capture"),Sky->bRealTimeCapture);}
        if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            if(const auto* M=It->FindComponentByClass<UStaticMeshComponent>())
            {
                auto Row=MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("component"),M->GetPathName());
                Row->SetStringField(TEXT("mesh"),GetPathNameSafe(M->GetStaticMesh()));
                Row->SetStringField(TEXT("material"),GetPathNameSafe(M->GetMaterial(0)));
                const bool Primary=M->IsRegistered()&&M->IsVisible()&&!M->bHiddenInGame&&!It->IsHidden()&&M->bRenderInMainPass;
                Row->SetBoolField(TEXT("primary_camera_visible"),Primary);
                Row->SetBoolField(TEXT("visible_in_ray_tracing"),M->bVisibleInRayTracing);
                Row->SetBoolField(TEXT("visible_in_reflections"),M->bVisibleInReflections);
                Row->SetBoolField(TEXT("visible_in_reflection_captures"),M->bVisibleInReflectionCaptures);
                Row->SetBoolField(TEXT("visible_in_realtime_sky_captures"),M->bVisibleInRealTimeSkyCaptures);
                Row->SetBoolField(TEXT("affects_distance_field_lighting"),M->bAffectDistanceFieldLighting);
                Row->SetBoolField(TEXT("affects_dynamic_indirect_lighting"),M->bAffectDynamicIndirectLighting);
                Row->SetBoolField(TEXT("affects_indirect_lighting_while_hidden"),M->bAffectIndirectLightingWhileHidden);
                Row->SetBoolField(TEXT("casts_shadow"),M->CastShadow);
                Row->SetBoolField(TEXT("casts_hidden_shadow"),M->bCastHiddenShadow);
                Row->SetBoolField(TEXT("emissive_light_source"),M->bEmissiveLightSource);
                MattePrimaryOnly&=Primary&&!M->bVisibleInRayTracing&&!M->bVisibleInReflections&&
                    !M->bVisibleInReflectionCaptures&&!M->bVisibleInRealTimeSkyCaptures&&
                    !M->bAffectDistanceFieldLighting&&!M->bAffectDynamicIndirectLighting&&
                    !M->bAffectIndirectLightingWhileHidden&&!M->CastShadow&&!M->bCastHiddenShadow&&!M->bEmissiveLightSource;
                MatteRows.Add(MakeShared<FJsonValueObject>(Row));
            }
        if(const auto* Ground=Cast<AWarfareFrontlineGround>(*It))if(Ground->Surface)
            if(auto* M=Ground->Surface->GetMaterial(0))
            {J->SetStringField(TEXT("ground_material"),M->GetPathName());J->SetStringField(TEXT("ground_base_material"),GetPathNameSafe(M->GetBaseMaterial()));}
    }
    J->SetNumberField(TEXT("directional_light_count"),KeyLights);J->SetNumberField(TEXT("sky_light_count"),SkyLights);
    J->SetArrayField(TEXT("backdrop_components_v44"),MatteRows);
    J->SetBoolField(TEXT("backdrop_primary_only_flags_v44"),MatteRows.Num()>0&&MattePrimaryOnly);
    // Scene-color screen traces intentionally bypass primitive reflection flags;
    // these flags verify secondary-scene exclusion, not final reflection pixels.
    J->SetNumberField(TEXT("reflection_screen_traces"),CVar(TEXT("r.Lumen.Reflections.ScreenTraces")));
    if(const auto* Grade=WeatherGrade(World))
    {
        const auto& S=Grade->Settings;
        J->SetNumberField(TEXT("postprocess_priority"),Grade->Priority);
        J->SetNumberField(TEXT("exposure_bias"),S.AutoExposureBias);J->SetNumberField(TEXT("exposure_min_ev100"),S.AutoExposureMinBrightness);
        J->SetNumberField(TEXT("exposure_max_ev100"),S.AutoExposureMaxBrightness);J->SetNumberField(TEXT("vignette"),S.VignetteIntensity);
        J->SetStringField(TEXT("saturation"),S.ColorSaturation.ToString());J->SetStringField(TEXT("contrast"),S.ColorContrast.ToString());
        J->SetNumberField(TEXT("reflection_quality"),S.LumenReflectionQuality);J->SetNumberField(TEXT("reflection_roughness_limit"),S.LumenMaxRoughnessToTraceReflections);
        J->SetNumberField(TEXT("lumen_final_gather_quality"),S.LumenFinalGatherQuality);
    }
    auto Vars=MakeShared<FJsonObject>();
    for(const auto* N:{TEXT("r.RayTracing"),TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Lumen.HardwareRayTracing.LightingMode"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.RayTracing.Shadows"),TEXT("r.Lumen.Reflections.DownsampleFactor"),TEXT("r.Lumen.Reflections.MaxBounces"),TEXT("r.RayTracing.ResidentGeometryMemoryPoolSizeInMB")})
        Vars->SetNumberField(N,CVar(N));
    J->SetObjectField(TEXT("render_cvars"),Vars);
    if(const auto* Render=UFrontlineRenderSettingsSubsystem::Get(World))J->SetObjectField(TEXT("renderer_and_hdr"),Render->GetDiagnostics());
    return J;
}
TSharedRef<FJsonObject> FrontlineLightingV42::DescribeV84(AWarfareFrontlineGameMode* B)
{
    auto J=Describe(B);UWorld* World=B?B->GetWorld():nullptr;
    J->SetBoolField(TEXT("v84_isolated_review"),IsIsolatedReview(B));
    J->SetBoolField(TEXT("v84_lighting_baseline"),IsV84Baseline());
    J->SetBoolField(TEXT("v842_lighting_baseline"),IsV842Baseline());
    const auto* Weather=AWarfareFrontlineWeather::Get(World);
    if(Weather)
    {
        const auto L=Weather->GetOperationLookV50();
        J->SetBoolField(TEXT("v84_authored_scene"),L.bAuthoredLightingV84);
        J->SetNumberField(TEXT("v84_authored_sun_scale"),L.SunScale);
        J->SetNumberField(TEXT("v84_authored_sky_scale"),L.SkyScale);
        J->SetNumberField(TEXT("v84_authored_pitch"),L.Pitch);
        J->SetNumberField(TEXT("v84_authored_yaw"),L.Yaw);
        J->SetNumberField(TEXT("v84_authored_source_angle"),L.SourceAngle);
        J->SetBoolField(TEXT("v84_cloud_shadow_expected"),L.bCloudShadowV84);
        J->SetNumberField(TEXT("v84_cloud_shadow_gain"),L.CloudShadowScaleV84);
        J->SetStringField(TEXT("v842_profile_key"),L.LightProfileKeyV842);
        J->SetStringField(TEXT("v842_day_phase"),L.LightPhaseV842);
        J->SetNumberField(TEXT("v842_season_month"),L.LightSeasonMonthV842);
        J->SetNumberField(TEXT("v842_backdrop_scale"),L.BackdropScale);
    }
    TArray<TSharedPtr<FJsonValue>> Casters;
    if(World)for(TActorIterator<AActor> It(World);It;++It)
    {
        if(const auto* Sun=It->FindComponentByClass<UDirectionalLightComponent>())
        {
            const auto Rotation=Sun->GetComponentRotation();
            J->SetNumberField(TEXT("v84_actual_sun_pitch"),Rotation.Pitch);
            J->SetNumberField(TEXT("v84_actual_sun_yaw"),Rotation.Yaw);
            J->SetBoolField(TEXT("v84_sun_casts_shadows"),Sun->CastShadows);
            J->SetNumberField(TEXT("v84_contact_shadow_length"),Sun->ContactShadowLength);
            J->SetBoolField(TEXT("v84_contact_shadow_world_space"),Sun->ContactShadowLengthInWS);
            J->SetStringField(TEXT("v84_cloud_light_function"),GetPathNameSafe(Sun->LightFunctionMaterial));
            J->SetBoolField(TEXT("v84_cloud_light_function_enabled"),Sun->LightFunctionMaterial!=nullptr);
        }
        if(const auto* Sky=It->FindComponentByClass<USkyLightComponent>())
        {
            J->SetNumberField(TEXT("v84_sky_source_type"),int32(Sky->SourceType));
            J->SetNumberField(TEXT("v84_sky_cubemap_angle"),Sky->SourceCubemapAngle);
            J->SetNumberField(TEXT("v84_sky_cubemap_resolution"),Sky->CubemapResolution);
        }
        // Read the real caster flags; no blanket restoration of intentionally
        // shadow-free sheets, decals, particles or the distant photograph.
        TInlineComponentArray<UPrimitiveComponent*> Components;It->GetComponents(Components);
        for(const auto* C:Components)if(C&&C->IsRegistered()&&C->IsVisible()
            &&(It->IsA<AWarfareFrontlineSoldier>()||It->IsA<AFrontlineTank>()
                ||C->ComponentHasTag(TEXT("WF_Environment_V40"))))
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("component"),C->GetPathName());
            Row->SetBoolField(TEXT("casts_shadow"),C->CastShadow);
            Row->SetBoolField(TEXT("casts_dynamic_shadow"),C->bCastDynamicShadow);
            Row->SetBoolField(TEXT("casts_contact_shadow"),C->bCastContactShadow);
            Row->SetBoolField(TEXT("visible_in_rt"),C->bVisibleInRayTracing);
            Casters.Add(MakeShared<FJsonValueObject>(Row));
        }
    }
    J->SetArrayField(TEXT("v84_visible_unit_and_environment_casters"),Casters);
    J->SetStringField(TEXT("v84_contact_policy"),TEXT("VSM remains authoritative on SM6; no duplicate screen-space contact pass."));
    return J;
}
void FrontlineLightingV42::RunV84Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto J=DescribeV84(B);const auto* Weather=AWarfareFrontlineWeather::Get(B?B->GetWorld():nullptr);
    Check(TEXT("v84_lighting_review_isolated"),IsIsolatedReview(B));
    Check(TEXT("v84_lighting_weather_ready"),Weather&&Weather->IsReady());
    Check(TEXT("v84_lighting_single_main_and_fill"),J->GetNumberField(TEXT("directional_light_count"))==1&&J->GetNumberField(TEXT("sky_light_count"))==1);
    Check(TEXT("v84_lighting_fixed_exposure"),FMath::IsNearlyZero(J->GetNumberField(TEXT("exposure_min_ev100")))
        &&FMath::IsNearlyZero(J->GetNumberField(TEXT("exposure_max_ev100")))&&FMath::IsNearlyEqual(J->GetNumberField(TEXT("exposure_bias")),.65,.0001));
    if(Weather)
    {
        const auto L=Weather->GetOperationLookV50();
        Check(TEXT("v84_lighting_runtime_matches_scene_direction"),FMath::IsNearlyEqual(J->GetNumberField(TEXT("v84_actual_sun_pitch")),double(L.Pitch),.001)
            &&FMath::IsNearlyEqual(J->GetNumberField(TEXT("v84_actual_sun_yaw")),double(L.Yaw),.001)
            &&FMath::IsNearlyEqual(J->GetNumberField(TEXT("source_angle_degrees")),double(L.SourceAngle),.001));
        Check(TEXT("v84_lighting_scene_cloud_function_matches"),J->GetBoolField(TEXT("v84_cloud_light_function_enabled"))==L.bCloudShadowV84);
        Check(TEXT("v84_lighting_retains_vsm_casting"),J->GetBoolField(TEXT("v84_sun_casts_shadows"))&&CVar(TEXT("r.Shadow.Virtual.Enable"))==1);
    }
    Check(TEXT("v84_lighting_sky_single_capture"),!J->GetBoolField(TEXT("sky_realtime_capture"))
        &&!J->GetStringField(TEXT("sky_cubemap")).IsEmpty());
    Check(TEXT("v84_lighting_retains_ground_binding"),J->GetBoolField(TEXT("weather_drives_displayed_ground")));
}
void FrontlineLightingV42::RunV842Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    RunV84Checks(B,Check);RunFrontlineWeatherChecksV842(Check);
    if(IsBaseline()||IsV84Baseline()||IsV842Baseline())return;
    const auto* W=AWarfareFrontlineWeather::Get(B?B->GetWorld():nullptr);
    if(!W)return;
    const auto L=W->GetOperationLookV50();
    const auto* M=B?B->GetEncounterCampaignMission():nullptr;
    if(!M&&B)M=B->GetSelectedCampaignMission();
    // Do not cast the closed national-adapter IDs back into a British sector.
    if(!M||M->Id<0||M->Id>=20)return;
    const float Storm=FMath::Clamp((W->GetCloudCover()-.57f)/.39f,0.f,1.f);
    const bool LegacyGain=FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline"));
    const float ExpectedSun=FMath::Lerp(2.55f,1.90f,Storm)*L.SunScale*(LegacyGain?1.f:1.12f);
    const float ExpectedSky=FMath::Lerp(.30f,.28f,Storm)*L.SkyScale*(LegacyGain?1.f:1.18f);
    Check(TEXT("v842_lighting_actual_operation_has_its_authored_profile"),L.bAuthoredLightingV84
        &&FString(L.LightProfileKeyV842)==M->MapKey&&L.LightSeasonMonthV842==(M->CalendarDateV41/100)%100);
    Check(TEXT("v842_lighting_actual_key_and_fill_match_weather_energy"),FMath::IsNearlyEqual(W->GetSunIntensity(),ExpectedSun,.001f)
        &&FMath::IsNearlyEqual(W->GetSkyIntensity(),ExpectedSky,.001f));
    bool Contact=true,Tint=false;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
        if(const auto* Sun=It->FindComponentByClass<UDirectionalLightComponent>())
        {
            Contact&=B->GetWorld()->GetFeatureLevel()<ERHIFeatureLevel::SM6||Sun->ContactShadowLength==0.f;
            // Component colour is stored in an 8-bit sRGB field. Compare its
            // actual decoded value at the quantization tolerance, not a string.
            Tint=Sun->GetLightColor().Equals(L.SunTint,.015f);
        }
    Check(TEXT("v842_lighting_actual_sm6_has_no_duplicate_contact_shadow"),Contact);
    Check(TEXT("v842_lighting_actual_key_has_authored_season_tint"),Tint);
}
void FrontlineLightingV42::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto J=Describe(B);Check(TEXT("lighting_isolated_review"),IsIsolatedReview(B));
    Check(TEXT("lighting_weather_and_precipitation_ready"),J->GetBoolField(TEXT("weather_ready")));
    Check(TEXT("lighting_preserves_displayed_ground_material"),J->GetBoolField(TEXT("weather_drives_displayed_ground")));
    Check(TEXT("lighting_single_captured_sky_and_directional_light"),J->GetNumberField(TEXT("directional_light_count"))==1 && J->GetNumberField(TEXT("sky_light_count"))==1);
    Check(TEXT("lighting_backdrop_visible_without_secondary_scene_obstruction"),J->GetBoolField(TEXT("backdrop_primary_only_flags_v44")));
    const auto V=J->GetObjectField(TEXT("actual_view"));
    Check(TEXT("lighting_actual_orthographic_view_observed"),V->GetBoolField(TEXT("observed")) && V->GetBoolField(TEXT("orthographic")));
    Check(TEXT("lighting_live_view_uses_lumen_not_progressive_path_tracing"),!V->GetBoolField(TEXT("native_path_tracing")) && V->GetNumberField(TEXT("gi_method"))==int32(EDynamicGlobalIlluminationMethod::Lumen) && V->GetNumberField(TEXT("reflection_method"))==int32(EReflectionMethod::Lumen));
    Check(TEXT("lighting_fixed_exposure_preserved"),FMath::IsNearlyZero(V->GetNumberField(TEXT("exposure_min_ev100"))) && FMath::IsNearlyZero(V->GetNumberField(TEXT("exposure_max_ev100"))) && FMath::IsNearlyEqual(V->GetNumberField(TEXT("exposure_bias")),.65,.0001));
    Check(TEXT("lighting_no_skylight_leaking_or_diffuse_boost"),FMath::IsNearlyZero(V->GetNumberField(TEXT("skylight_leaking"))) && FMath::IsNearlyEqual(V->GetNumberField(TEXT("diffuse_color_boost")),1.,.0001));
    const float Storm=FMath::Clamp(float((J->GetNumberField(TEXT("cloud_cover"))-.57)/.39),0.f,1.f);
    const float Sky=IsBaseline()?FMath::Lerp(.245f,.205f,Storm):FMath::Lerp(.30f,.28f,Storm);
    Check(TEXT("lighting_mission_cloud_drives_bounded_sky_fill"),FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("sky_intensity"))),Sky,.004f));
    Check(TEXT("lighting_existing_reflection_and_shadow_cost_preserved"),CVar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"))==2 && CVar(TEXT("r.Lumen.Reflections.DownsampleFactor"))==2 && CVar(TEXT("r.Lumen.Reflections.MaxBounces"))==2 && CVar(TEXT("r.Shadow.Virtual.Enable"))==1);
}
bool FrontlineLightingV42::BeginStaticReference(AWarfareFrontlineGameMode* B)
{
    if(!IsIsolatedReview(B) || !UDLSSLibrary::IsRayTracingAvailable() || CVar(TEXT("r.PathTracing"))!=1)return false;
    UWorld* World=B->GetWorld();ObserveReviewWorld(World);auto& R=ReviewObservation();
    if(R.bReferenceStarted)return true;
    auto* Render=UFrontlineRenderSettingsSubsystem::Get(World);if(!Render)return false;
    R.PathMaterialShaders=ObservePathMaterialShadersV42(World);
    Render->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);
    Render->SetRayReconstruction(false,false);Render->SetQuality(EFrontlineRenderQuality::Native,false);
    auto Set=[](const TCHAR* Name,int32 Value){if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))V->SetWithCurrentPriority(Value);};
    // This isolated final diagnostic exits its process after capture. A PSO
    // fallback can shade newly encountered hit groups opaque black while the
    // path tracer accumulates; an offline reference must wait for the real PSO.
    // Ordinary gameplay retains the engine's nonblocking pipeline policy.
    Set(TEXT("r.RayTracing.NonBlockingPipelineCreation"),0);
    Set(TEXT("r.PathTracing.SamplesPerPixel"),64);Set(TEXT("r.PathTracing.MaxBounces"),4);
    Set(TEXT("r.PathTracing.Denoiser"),1);Set(TEXT("r.PathTracing.ProgressDisplay"),0);
    if(auto* PC=Cast<AWarfareFrontlineController>(World->GetFirstPlayerController()))
    {
        // Apply the requested wide camera before pausing its normal POV update.
        if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);
        if(PC->BattleCamera)
        {
            FVector Origin;FRotator Rotation;PC->GetPlayerViewPoint(Origin,Rotation);
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            const float OrthoWidth=PC->BattleCamera->GetCameraComponent()->OrthoWidth;
            if(Width>0 && OrthoWidth>1)
            {
                const float EquivalentScreenSize=float(Width)*2.f*14560.f/OrthoWidth;
                IStreamingManager::Get().AddViewInformation(Origin,EquivalentScreenSize,EquivalentScreenSize,1.f,false,60.f,nullptr,World);
            }
        }
    }
    for(TActorIterator<AActor> It(World);It;++It)
    {
        It->SetActorTickEnabled(false);TInlineComponentArray<UActorComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)C->SetComponentTickEnabled(false);
    }
    UGameplayStatics::SetGamePaused(World,true);
    {
        FScopeLock Lock(&R.Data->Mutex);R.Data->Samples=R.Data->TargetSamples=R.Data->CompletedViews=R.Data->Resets=0;R.Data->PathFrames=0;
        R.Data->PausedPathViewFamilies=R.Data->PathCameraCuts=R.Data->PathForceResets=R.Data->PathProjectionChanges=0;
        R.Data->bHasPathProjection=false;R.Data->bStaticReference=true;
    }
    UKismetRenderingLibrary::EnablePathTracing(true);UKismetRenderingLibrary::RefreshPathTracingOutput();
    R.bReferenceStarted=GEngine && GEngine->GameViewport && GEngine->GameViewport->EngineShowFlags.PathTracing;
    R.ReferenceStartedAt=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("WF_V42_STATIC_REFERENCE started=%d target_samples=64 max_bounces=4 convergence=actual_view_state timeout_seconds=45"),R.bReferenceStarted);
    return R.bReferenceStarted;
}
TSharedRef<FJsonObject> FrontlineLightingV42::DescribeReference(AWarfareFrontlineGameMode* B)
{
    auto J=ObservedView();auto& R=ReviewObservation();
    const bool Started=R.bReferenceStarted && B && R.World==B->GetWorld();
    if(Started && IsIsolatedReview(B) && !R.bReferenceRenderStatesRecreated && J->GetNumberField(TEXT("path_views"))>=2)
    {
        // A cooked native PT reference still showed default-grid/black static
        // surfaces although its actual RT mesh batches contained the correct
        // authored CHS for FLocalVertexFactory. Recreate only these visible
        // static render states once, after the renderer has entered PT mode.
        // This is an isolated mode-transition repair; ordinary gameplay and its
        // persistent RT caches are unaffected. Its result needs visual review.
        UWorld* World=B->GetWorld();
        for(TActorIterator<AActor> It(World);It;++It)
        {
            if(It->IsHidden())continue;
            TInlineComponentArray<UStaticMeshComponent*> Components(*It);
            for(UStaticMeshComponent* C:Components)
            {
                auto* ISM=Cast<UInstancedStaticMeshComponent>(C);
                if((!It->IsA<AWarfareFrontlineGround>()&&!ISM)||!C->IsRegistered()||!C->IsVisible()||C->bHiddenInGame||
                    !C->bRenderInMainPass||!C->bVisibleInRayTracing||!C->GetStaticMesh()||!C->GetSceneProxy()||(ISM&&ISM->GetInstanceCount()==0))continue;
                C->MarkRenderStateDirty();++R.ReferenceRecreatedComponents;
            }
        }
        R.bReferenceRenderStatesRecreated=true;
        UKismetRenderingLibrary::RefreshPathTracingOutput();
        if(R.Data)
        {
            FScopeLock Lock(&R.Data->Mutex);R.Data->CompletedViews=0;
        }
        J->SetBoolField(TEXT("path_converged"),false);
    }
    const double Elapsed=Started?FPlatformTime::Seconds()-R.ReferenceStartedAt:0;
    bool Converged=false;J->TryGetBoolField(TEXT("path_converged"),Converged);
    J->SetBoolField(TEXT("started"),Started);J->SetBoolField(TEXT("converged"),Started && Converged);
    J->SetBoolField(TEXT("simulation_paused"),B && B->GetWorld()->IsPaused());
    J->SetBoolField(TEXT("blocking_material_pipeline_creation"),Started && CVar(TEXT("r.RayTracing.NonBlockingPipelineCreation"))==0);
    J->SetBoolField(TEXT("static_mode_render_states_recreated"),R.bReferenceRenderStatesRecreated);
    J->SetNumberField(TEXT("static_mode_recreated_components"),R.ReferenceRecreatedComponents);
    if(R.PathMaterialShaders)J->SetObjectField(TEXT("authored_path_material_shaders"),R.PathMaterialShaders.ToSharedRef());
    J->SetNumberField(TEXT("elapsed_seconds"),Elapsed);J->SetBoolField(TEXT("timed_out"),Started && !Converged && Elapsed>=45);
    J->SetStringField(TEXT("scope"),TEXT("Frozen 64-sample, four-bounce native UE path-traced diagnostic. Sample convergence requires separate visual validation of all surfaces. Not a realtime gameplay renderer or a motion/VFX benchmark."));
    return J;
}
