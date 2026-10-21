#include "WarfareFrontlineSoldierSurfaceV62.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineMaterialV27.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "RenderingThread.h"
#include "SceneInterface.h"
#include "PipelineStateCache.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif

namespace
{
TAutoConsoleVariable<int32> GSoldierSurfaceV62(TEXT("wf.Characters.SurfaceV62"),0,
    TEXT("Opt-in V62 isolated material candidate for new soldiers. Apply command rebinds living soldiers in place."));
bool CandidateEnabled()
{return GSoldierSurfaceV62.GetValueOnGameThread()!=0||FParse::Param(FCommandLine::Get(),TEXT("WFSoldierSurfaceV62"));}
EShaderPlatform ScenePlatform(UWorld* World)
{return World->Scene?World->Scene->GetShaderPlatform():GetFeatureLevelShaderPlatform(World->GetFeatureLevel());}
struct FSurfaceRenderObservation
{
    const FMaterialRenderProxy* Proxy=nullptr;
    bool OriginalPresent=false,Valid=false,Complete=false;int32 ShaderCount=0,PipelineCount=0;FString Name;
};
TArray<FSurfaceRenderObservation> ObserveRenderMaterials(const TArray<UMaterialInterface*>& Materials,ERHIFeatureLevel::Type FeatureLevel)
{
    TArray<TStrongObjectPtr<UMaterialInterface>> KeepAlive;
    auto Batch=MakeShared<TArray<FSurfaceRenderObservation>,ESPMode::ThreadSafe>();
    for(auto* Material:Materials){KeepAlive.Emplace(Material);FSurfaceRenderObservation O;O.Proxy=Material->GetRenderProxy();Batch->Add(O);}
    ENQUEUE_RENDER_COMMAND(FrontlineV62ObserveSoldierShaders)([Batch,FeatureLevel](FRHICommandListImmediate&)
    {
        for(auto& O:*Batch)
        {
            // A default fallback may draw, but does not prove the assigned MID is ready.
            const FMaterial* M=O.Proxy?O.Proxy->GetMaterialNoFallback(FeatureLevel):nullptr;
            O.OriginalPresent=M!=nullptr;if(!M)continue;
            O.Name=M->GetFriendlyName();O.Complete=M->IsRenderingThreadShaderMapComplete();
            const auto* Map=M->GetRenderingThreadShaderMap();O.Valid=Map&&Map->IsValidForRendering();
            if(Map)Map->CountNumShaders(O.ShaderCount,O.PipelineCount);
        }
    });
    // Opt-in diagnostic only; never part of ordinary soldier updates.
    FlushRenderingCommands();return MoveTemp(*Batch);
}
bool AuditWorld(UWorld* World,const TCHAR* Label)
{
    if(!World)return false;
    auto Root=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Units;
    TSet<FString> Seen;TArray<UMaterialInterface*> Assigned;
    for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)
        if(It->IsAlive()&&It->Soldier&&It->Soldier->IsVisible())
            for(int32 I=0;I<It->Soldier->GetNumMaterials();++I)if(auto* M=It->Soldier->GetMaterial(I))Assigned.AddUnique(M);
    const auto Render=ObserveRenderMaterials(Assigned,World->GetFeatureLevel());
    const auto Platform=ScenePlatform(World);bool Ready=!Assigned.IsEmpty();int32 CandidateSlots=0;
    Root->SetStringField(TEXT("capture_label"),Label);Root->SetNumberField(TEXT("scene_shader_platform"),int32(Platform));
    Root->SetBoolField(TEXT("candidate_requested"),CandidateEnabled());
    const bool ShapeRequested=FParse::Param(FCommandLine::Get(),TEXT("WFSoldierShapeV62"));
    Root->SetBoolField(TEXT("shape_candidate_requested"),ShapeRequested);
    for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)
    {
        auto* Unit=*It;auto* C=Unit->Soldier.Get();if(!C||!Unit->IsAlive()||!C->IsVisible())continue;
        auto* Mesh=C->GetSkeletalMeshAsset();if(!Mesh)continue;
        // One representative per actual mesh, keeping the probe bounded.
        if(Seen.Contains(Mesh->GetPathName()))continue;Seen.Add(Mesh->GetPathName());
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("mesh"),Mesh->GetPathName());
        const bool ShapeExpected=ShapeRequested&&(Mesh->GetName().Contains(TEXT("_Rifle_"))||Mesh->GetName().Contains(TEXT("_Machinegun_"))||Mesh->GetName().Contains(TEXT("_AmmoBearer_")));
        const bool ShapeBound=Mesh->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V62/Meshes/"));
        R->SetBoolField(TEXT("shape_candidate_expected"),ShapeExpected);R->SetBoolField(TEXT("shape_candidate_bound"),ShapeBound);
        Ready&=!ShapeExpected||ShapeBound;
        if(ShapeBound)
        {
            const FString OriginalName=Mesh->GetName().LeftChop(4)+TEXT("_V34");
            auto* Original=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Warfare/Characters/V34/Meshes/")+OriginalName+TEXT(".")+OriginalName));
            bool RefEqual=Original!=nullptr,BoundsEqual=Original!=nullptr;
            float MaximumRestTranslation=0.f;
            if(Original)
            {
                const auto& A=Mesh->GetRefSkeleton();const auto& B=Original->GetRefSkeleton();
                RefEqual=A.GetNum()==B.GetNum();
                for(int32 I=0;I<FMath::Min(A.GetNum(),B.GetNum());++I)
                {
                    const auto& X=A.GetRefBonePose()[I];const auto& Y=B.GetRefBonePose()[I];
                    MaximumRestTranslation=FMath::Max(MaximumRestTranslation,float(FVector::Distance(X.GetTranslation(),Y.GetTranslation())));
                    RefEqual&=A.GetBoneName(I)==B.GetBoneName(I)&&A.GetParentIndex(I)==B.GetParentIndex(I)&&X.Equals(Y,.02f);
                }
                const auto X=Mesh->GetImportedBounds(),Y=Original->GetImportedBounds();
                BoundsEqual=X.Origin.Equals(Y.Origin,1.25)&&X.BoxExtent.Equals(Y.BoxExtent,1.25);
                R->SetStringField(TEXT("imported_bounds"),X.BoxExtent.ToString());
                R->SetStringField(TEXT("source_imported_bounds"),Y.BoxExtent.ToString());
            }
            R->SetBoolField(TEXT("source_reference_pose_match"),RefEqual);
            R->SetBoolField(TEXT("source_bounds_match_within_shape_allowance"),BoundsEqual);
            R->SetNumberField(TEXT("maximum_rest_translation_error_cm"),MaximumRestTranslation);
            Ready&=RefEqual&&BoundsEqual;
        }
        R->SetStringField(TEXT("actor"),Unit->GetPathName());R->SetStringField(TEXT("component"),C->GetPathName());
        R->SetBoolField(TEXT("component_registered"),C->IsRegistered());
        const int32 LOD=C->GetPredictedLODLevel();R->SetNumberField(TEXT("predicted_lod"),LOD);
        TArray<TSharedPtr<FJsonValue>> Materials;
        for(int32 I=0;I<C->GetNumMaterials();++I)
        {
            auto S=MakeShared<FJsonObject>();auto* Mat=C->GetMaterial(I);
            S->SetNumberField(TEXT("slot"),I);S->SetStringField(TEXT("material"),GetPathNameSafe(Mat));
            S->SetStringField(TEXT("base_material"),Mat?GetPathNameSafe(Mat->GetBaseMaterial()):TEXT("None"));
            if(auto* MI=Cast<UMaterialInstance>(Mat))S->SetStringField(TEXT("immediate_parent"),GetPathNameSafe(MI->Parent));
            const auto* Base=Mat?Mat->GetBaseMaterial():nullptr;
            const FString BaseName=GetNameSafe(Base);
            const bool Candidate=Base&&Base->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V62/"));
            const bool CandidateExpected=CandidateEnabled()&&(BaseName.StartsWith(TEXT("M_V34_Cloth_"))||BaseName.StartsWith(TEXT("M_V34_TunicCloth_"))||BaseName.StartsWith(TEXT("M_V34_Helmet_"))||BaseName==TEXT("M_V34_BootLeather"));
            S->SetBoolField(TEXT("candidate_expected"),CandidateExpected);Ready&=!CandidateExpected||Candidate;
            CandidateSlots+=Candidate?1:0;S->SetBoolField(TEXT("candidate_bound"),Candidate);
            const int32 RenderIndex=Assigned.Find(Mat);const auto* O=Render.IsValidIndex(RenderIndex)?&Render[RenderIndex]:nullptr;
            const bool RenderReady=O&&O->OriginalPresent&&O->Valid&&O->ShaderCount>0;Ready&=RenderReady;
            S->SetBoolField(TEXT("render_original_present"),O&&O->OriginalPresent);
            S->SetBoolField(TEXT("render_shader_valid"),O&&O->Valid);S->SetBoolField(TEXT("render_shader_complete"),O&&O->Complete);
            S->SetStringField(TEXT("actual_render_material"),O?O->Name:TEXT("None"));S->SetNumberField(TEXT("render_shader_count"),O?O->ShaderCount:0);
            const auto* Resource=Mat?Mat->GetMaterialResource(Platform):nullptr;
            S->SetBoolField(TEXT("game_thread_shader_valid"),Resource&&Resource->HasValidGameThreadShaderMap());
            S->SetStringField(TEXT("asset_material"),Mesh->GetMaterials().IsValidIndex(I)?GetPathNameSafe(Mesh->GetMaterials()[I].MaterialInterface.Get()):TEXT("None"));
            TArray<TSharedPtr<FJsonValue>> Textures;
            if(Mat)
            {
                TArray<UTexture*> Used;Mat->GetUsedTextures(Used,{},Platform);
                for(auto* T:Used)if(auto* T2=Cast<UTexture2D>(T))
                {
                    auto X=MakeShared<FJsonObject>();X->SetStringField(TEXT("asset"),T2->GetPathName());
                    X->SetNumberField(TEXT("resident_mips"),T2->GetNumResidentMips());X->SetNumberField(TEXT("total_mips"),T2->GetNumMips());
                    X->SetNumberField(TEXT("size_x"),T2->GetSizeX());X->SetNumberField(TEXT("size_y"),T2->GetSizeY());
                    X->SetBoolField(TEXT("fully_streamed_in"),T2->IsFullyStreamedIn());X->SetBoolField(TEXT("pending_streaming"),T2->HasPendingInitOrStreaming());
                    X->SetBoolField(TEXT("render_resource_present"),T2->GetResource()!=nullptr);
                    Ready&=T2->GetResource()!=nullptr&&T2->IsFullyStreamedIn()&&!T2->HasPendingInitOrStreaming();
                    Textures.Add(MakeShared<FJsonValueObject>(X));
                }
            }
            S->SetArrayField(TEXT("textures"),Textures);Materials.Add(MakeShared<FJsonValueObject>(S));
        }
        R->SetArrayField(TEXT("materials"),Materials);
        auto* Data=Mesh->GetResourceForRendering();
        if(Data&&Data->LODRenderData.IsValidIndex(LOD))
        {
            const auto& D=Data->LODRenderData[LOD];const auto& V=D.StaticVertexBuffers.StaticMeshVertexBuffer;
            const bool CPU=V.GetAllowCPUAccess()&&V.GetTexCoordData()&&V.GetTangentData();
            R->SetBoolField(TEXT("uv_cpu_data_available"),CPU);R->SetNumberField(TEXT("uv_channels"),V.GetNumTexCoords());
            TArray<TSharedPtr<FJsonValue>> Sections;
            if(CPU)for(const auto& Section:D.RenderSections)
            {
                auto S=MakeShared<FJsonObject>();S->SetNumberField(TEXT("render_material_index"),Section.MaterialIndex);
                S->SetNumberField(TEXT("vertices"),Section.NumVertices);TArray<TSharedPtr<FJsonValue>> Channels;
                for(uint32 Ch=0;Ch<V.GetNumTexCoords();++Ch)
                {
                    FVector2f Lo(FLT_MAX,FLT_MAX),Hi(-FLT_MAX,-FLT_MAX);int32 Samples=0,Zero=0;
                    const uint32 Step=FMath::Max(1u,Section.NumVertices/512u);
                    for(uint32 J=0;J<Section.NumVertices;J+=Step)
                    {
                        const uint32 Index=Section.BaseVertexIndex+J;if(Index>=V.GetNumVertices())break;
                        const FVector2f UV=V.GetVertexUV(Index,Ch);++Samples;Zero+=UV.IsNearlyZero(1.e-6f)?1:0;
                        Lo.X=FMath::Min(Lo.X,UV.X);Lo.Y=FMath::Min(Lo.Y,UV.Y);Hi.X=FMath::Max(Hi.X,UV.X);Hi.Y=FMath::Max(Hi.Y,UV.Y);
                    }
                    auto U=MakeShared<FJsonObject>();U->SetNumberField(TEXT("channel"),Ch);U->SetNumberField(TEXT("sampled_vertices"),Samples);
                    U->SetNumberField(TEXT("zero_uv_vertices"),Zero);
                    if(Samples){U->SetStringField(TEXT("min"),Lo.ToString());U->SetStringField(TEXT("max"),Hi.ToString());}
                    Channels.Add(MakeShared<FJsonValueObject>(U));
                }
                S->SetArrayField(TEXT("uv_samples"),Channels);Sections.Add(MakeShared<FJsonValueObject>(S));
            }
            R->SetArrayField(TEXT("sections"),Sections);
        }
        Units.Add(MakeShared<FJsonValueObject>(R));
    }
    if(CandidateEnabled())Ready&=CandidateSlots>0;
    Root->SetArrayField(TEXT("units"),Units);Root->SetBoolField(TEXT("read_only"),true);
    Root->SetNumberField(TEXT("candidate_slots"),CandidateSlots);Root->SetBoolField(TEXT("assigned_render_materials_and_mips_ready"),Ready);
    Root->SetStringField(TEXT("limits"),TEXT("Sampled render UV bounds do not prove all triangles or normal bases. No appearance acceptance is inferred."));
    FString Json;const auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Root,Writer);
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V62/SoldierSurfaces");IFileManager::Get().MakeDirectory(*Directory,true);
    FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("runtime-audit.json")));
    if(Label&&*Label)FFileHelper::SaveStringToFile(Json,*(Directory/(FString(TEXT("runtime-audit-"))+Label+TEXT(".json"))));
    UE_LOG(LogTemp,Display,TEXT("WF_V62_SOLDIER_RUNTIME_AUDIT label=%s meshes=%d candidate_slots=%d render_ready=%d path=%s"),Label,Units.Num(),CandidateSlots,Ready,*Directory);
    return Ready;
}
}

UMaterialInterface* FrontlineSoldierSurfaceV62::Resolve(UMaterialInterface* Source)
{
    const bool Enabled=CandidateEnabled();
    if(!Enabled||!Source||!Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/")))return Source;
    const FString Name=Source->GetName();
    if(!Name.StartsWith(TEXT("M_V34_Cloth_"))&&!Name.StartsWith(TEXT("M_V34_TunicCloth_"))&&
       !Name.StartsWith(TEXT("M_V34_Helmet_"))&&Name!=TEXT("M_V34_BootLeather"))return Source;
    const FString Path=TEXT("/Game/Warfare/Characters/V62/Materials/")+Name+TEXT(".")+Name;
    if(auto* Candidate=LoadObject<UMaterialInterface>(nullptr,*Path))return Candidate;
    static TSet<FString> Missing;
    if(!Missing.Contains(Path)){Missing.Add(Path);UE_LOG(LogTemp,Warning,TEXT("WF_V62_SOLDIER_CANDIDATE_MISSING %s"),*Path);}
    return Source;
}

USkeletalMesh* FrontlineSoldierSurfaceV62::ResolveMesh(USkeletalMesh* Source)
{
    if(!Source||!FParse::Param(FCommandLine::Get(),TEXT("WFSoldierShapeV62")))return Source;
    const FString Original=Source->GetName();bool Known=false;
    for(const TCHAR* Faction:{TEXT("British"),TEXT("German")})for(const TCHAR* Role:{TEXT("Rifle"),TEXT("Machinegun"),TEXT("AmmoBearer")})
        Known|=Original==FString::Printf(TEXT("SK_%s_%s_V34"),Faction,Role);
    if(!Known)return Source;
    const FString Name=Original.LeftChop(4)+TEXT("_V62");
    const FString CasualtyName=Name.Replace(TEXT("SK_"),TEXT("SK_Casualty_"));
    auto* Living=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Warfare/Characters/V62/Meshes/")+Name+TEXT(".")+Name));
    auto* Casualty=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Warfare/Characters/V62/Casualties/")+CasualtyName+TEXT(".")+CasualtyName));
    // Incomplete imports cannot silently mix changed living bodies and old
    // wounded torsos. Both representations must retain the shared skeleton.
    if(Living&&Casualty&&Living->GetSkeleton()==Source->GetSkeleton()&&Casualty->GetSkeleton()==Source->GetSkeleton())return Living;
    static TSet<FString> Reported;
    if(!Reported.Contains(Original))
    {Reported.Add(Original);UE_LOG(LogTemp,Warning,TEXT("WF_V62_SHAPE_FALLBACK incomplete_pair_or_skeleton=%s"),*Original);}
    return Source;
}

void FrontlineSoldierSurfaceV62::ScheduleAudit(AWarfareFrontlineSoldier* Unit)
{
    if(!Unit||!Unit->GetWorld()||!FParse::Param(FCommandLine::Get(),TEXT("WFSoldierSurfaceAuditV62")))return;
    static TWeakObjectPtr<UWorld> PendingWorld;static FTimerHandle Handle;static int32 Reports=0;
    UWorld* World=Unit->GetWorld();
    if(PendingWorld.Get()!=World)
    {
        if(auto* Previous=PendingWorld.Get())Previous->GetTimerManager().ClearTimer(Handle);
        PendingWorld=World;Handle.Invalidate();Reports=0;
    }
    // New fixture actors must not postpone the first observation indefinitely.
    if(Reports>=3||World->GetTimerManager().IsTimerActive(Handle))return;
    World->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(World,[World]()
    {
        AuditWorld(World,TEXT("scheduled"));
        if(++Reports>=3)World->GetTimerManager().ClearTimer(Handle);
    }),2.f,true);
}

bool FrontlineSoldierSurfaceV62::PrepareReview(UWorld* World)
{
    if(!World)return false;
    bool Ready=!GUsingNullRHI;int32 Slots=0;TSet<UMaterialInterface*> Materials;TSet<UTexture2D*> Textures;
#if WITH_EDITOR
    Ready&=FAssetCompilingManager::Get().GetNumRemainingAssets()==0&&(!GShaderCompilingManager||!GShaderCompilingManager->IsCompiling());
#endif
    Ready&=PipelineStateCache::GetNumActivePipelinePrecompileTasks()==0;
    const auto Platform=ScenePlatform(World);
    for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)if(It->IsAlive()&&It->Soldier&&It->Soldier->IsVisible())
        for(int32 I=0;I<It->Soldier->GetNumMaterials();++I)
        {
            auto* M=It->Soldier->GetMaterial(I);++Slots;if(!M){Ready=false;continue;}Materials.Add(M);
        }
    for(auto* M:Materials)
    {
        TArray<UTexture*> Used;M->GetUsedTextures(Used,{},Platform);
        for(auto* T:Used)if(auto* T2=Cast<UTexture2D>(T))Textures.Add(T2);
    }
    // UE 5.8 can expose no game-thread shader map for an MID while its
    // render proxy already owns valid shaders. Gate the actual draw resource.
    for(const auto& O:ObserveRenderMaterials(Materials.Array(),World->GetFeatureLevel()))
        Ready&=O.OriginalPresent&&O.Valid&&O.ShaderCount>0;
    for(auto* T:Textures)
    {
        T->SetForceMipLevelsToBeResident(10.f);
        Ready&=T->GetResource()!=nullptr&&T->IsFullyStreamedIn()&&!T->HasPendingInitOrStreaming();
    }
    return Ready&&Slots>0;
}

bool FrontlineSoldierSurfaceV62::WriteAudit(UWorld* World,const TCHAR* Label)
{return AuditWorld(World,Label);}

static FAutoConsoleCommandWithWorld GAuditSoldierSurfacesV62(TEXT("wf.Characters.SurfaceAuditV62"),
    TEXT("Read current soldier component materials, texture residency and bounded render UV samples; does not change assets or saves."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){AuditWorld(World,TEXT("console"));}));

static FAutoConsoleCommandWithWorldAndArgs GApplySoldierSurfacesV62(TEXT("wf.Characters.SurfaceV62.Apply"),
    TEXT("0/1 rebinds living soldier materials in the existing scene; no world/lighting/asset/save mutation. Launch flag forces candidate on."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        if(!World)return;
        GSoldierSurfaceV62.AsVariable()->Set(Args.Num()==0||FCString::Atoi(*Args[0])!=0?1:0,ECVF_SetByConsole);
        for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)if(It->IsAlive())FrontlineMaterialV27::Apply(*It,true);
    }));
