#include "WarfareFrontlineEffectsSafetyV844.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFRRUCampaignV82.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"
#include "VertexFactory.h"
#include "HAL/CriticalSection.h"
#include "MaterialShared.h"
#include "SceneInterface.h"
#include "RHI.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/ScopeLock.h"

namespace
{
struct FRenderEvidence
{
    FCriticalSection Mutex;bool Done=false,NoFallback=false,Valid=false,GPU=false,Complete=false;
    int32 Vertex=0,Pixel=0;FString Resolved;TArray<FString> ShaderSamples;
};
TMap<const FMaterialResource*,TSharedPtr<FRenderEvidence,ESPMode::ThreadSafe>> RenderEvidence;
TSharedPtr<FRenderEvidence,ESPMode::ThreadSafe> ShaderEvidence(UMaterialInterface* M,const FMaterialResource* Resource,ERHIFeatureLevel::Type Feature)
{
    if(!M||!Resource)return nullptr;
    if(const auto* Prior=RenderEvidence.Find(Resource))return *Prior;
    auto State=MakeShared<FRenderEvidence,ESPMode::ThreadSafe>();RenderEvidence.Add(Resource,State);const auto* Proxy=M->GetRenderProxy();
    ENQUEUE_RENDER_COMMAND(WarfareEffectsShaderV844)([State,Proxy,Feature](FRHICommandListImmediate&)
    {
        bool DirectOK=false,MapOK=false,GPU=false,Complete=false;int32 VS=0,PS=0;FString Name;TArray<FString> Samples;
        if(Proxy)if(const FMaterial* Direct=Proxy->GetMaterialNoFallback(Feature))
        {
            const FMaterial& Resolved=Proxy->GetIncompleteMaterialWithFallback(Feature);
            DirectOK=&Resolved==Direct&&!Direct->IsDefaultMaterial();Name=Resolved.GetFriendlyName();Complete=Direct->IsRenderingThreadShaderMapComplete();
            if(const auto* Map=Direct->GetRenderingThreadShaderMap())
            {
                MapOK=Map->IsValidForRendering();
                // Query each known mesh map itself. Cooked shader IDs need not
                // preserve a registered VF pointer suitable for string filtering.
                for(const TCHAR* Factory:{TEXT("FLocalVertexFactory"),TEXT("FInstancedStaticMeshVertexFactory")})
                {
                    const auto* Mesh=Map->GetMeshShaderMap(FHashedName(Factory));if(!Mesh){Samples.Add(FString(Factory)+TEXT(": missing mesh map"));continue;}
                    TMap<FHashedName,TShaderRef<FShader>> Shaders;Mesh->GetShaderList(*Map,Shaders);
                    for(const auto& Entry:Shaders)
                    {
                        const FString Type=Entry.Value.GetType()->GetName();const auto Frequency=Entry.Value->GetFrequency();
                        if(Samples.Num()<24)Samples.Add(FString(Factory)+TEXT(": ")+Type+FString::Printf(TEXT(" frequency=%d"),int32(Frequency)));
                        // Base-pass registrations include F128BitRTBasePassPS
                        // and the mesh-shader alternative on current RHIs.
                        const bool BasePass=Type.Contains(TEXT("BasePass"));
                        VS+=BasePass&&((Frequency==SF_Vertex&&Entry.Value.GetVertexShader()!=nullptr)
                            ||(Frequency==SF_Mesh&&Entry.Value.GetMeshShader()!=nullptr));
                        PS+=BasePass&&Frequency==SF_Pixel&&Entry.Value.GetPixelShader()!=nullptr;
                    }
                }
                GPU=VS>0&&PS>0;
            }
        }
        FScopeLock Lock(&State->Mutex);State->NoFallback=DirectOK;State->Valid=MapOK;State->GPU=GPU;State->Complete=Complete;State->Vertex=VS;State->Pixel=PS;State->Resolved=Name;State->ShaderSamples=Samples;State->Done=true;
    });
    return State;
}
struct FProxy
{
    UMaterialInterface* Material=nullptr;
    const FMaterialResource* Resource=nullptr;
    bool Visible=false,Cube=false,Finite=false,Size=false,Surface=false,Blend=false,Default=false,Shader=false,Complete=false,Kernel=true;
    bool RenderDone=false,RenderValid=false,RenderNoFallback=false,RenderGPU=false;int32 RenderVS=0,RenderPS=0;FString RenderName;TArray<FString> ShaderSamples;
    int32 Platform=-1;FVector Dimensions=FVector::ZeroVector;
    bool StructuralSafe()const{return Finite&&Size&&Surface&&Blend&&!Default&&Kernel;}
    // A queried map which is valid and resolves this non-default proxy is
    // authoritative readiness evidence. UE5.8 can expose no enumerated mesh
    // maps to this inspector while the renderer uses its cached permutations;
    // an unsupported enumeration is diagnostic, not proof of a missing pass.
    bool Safe()const{return StructuralSafe()&&RenderDone&&RenderValid&&RenderNoFallback;}
};
FProxy Observe(UStaticMeshComponent* C,bool Diagnostic=false)
{
    FProxy P;if(!C)return P;P.Visible=C->IsVisible();
    auto* W=C->GetWorld();const EShaderPlatform Platform=W&&W->Scene?W->Scene->GetShaderPlatform():GMaxRHIShaderPlatform;P.Platform=int32(Platform);
    P.Cube=C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==TEXT("/Engine/BasicShapes/Cube.Cube");
    P.Material=C->GetMaterial(0);P.Resource=P.Material?P.Material->GetMaterialResource(Platform):nullptr;
    P.Surface=P.Resource&&P.Resource->GetMaterialDomain()==MD_Surface;
    P.Blend=P.Material&&(!P.Cube||IsTranslucentBlendMode(*P.Material));
    P.Default=!P.Material||P.Material->GetBaseMaterial()==UMaterial::GetDefaultMaterial(MD_Surface);
    P.Shader=P.Resource&&P.Resource->HasValidGameThreadShaderMap()&&P.Resource->GetGameThreadShaderMap()
        &&P.Resource->GetGameThreadShaderMap()->IsValidForRendering();
    P.Complete=P.Resource&&P.Resource->IsGameThreadShaderMapComplete();
    if(auto State=ShaderEvidence(P.Material,P.Resource,W?W->GetFeatureLevel():GMaxRHIFeatureLevel))
    {FScopeLock Lock(&State->Mutex);P.RenderDone=State->Done;P.RenderValid=State->Valid;P.RenderNoFallback=State->NoFallback;P.RenderGPU=State->GPU;P.RenderVS=State->Vertex;P.RenderPS=State->Pixel;if(Diagnostic){P.RenderName=State->Resolved;P.ShaderSamples=State->ShaderSamples;}}
    const auto Bounds=C->CalcBounds(C->GetComponentTransform());
    P.Dimensions=Bounds.BoxExtent*2.f;
    P.Finite=!C->GetComponentTransform().ContainsNaN()&&!P.Dimensions.ContainsNaN()
        &&FMath::IsFinite(Bounds.SphereRadius)&&Bounds.SphereRadius>=0.f;
    P.Size=P.Finite&&P.Dimensions.GetMax()<64000.f&&(!P.Cube||(P.Dimensions.GetMin()>1.f&&P.Dimensions.GetMax()<18000.f));
    if(P.Cube)
    {
        auto* M=Cast<UMaterialInstanceDynamic>(P.Material);FLinearColor Centre,Radii;float Density=0,Age=0;
        const bool Pins=M&&M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("VolumeCenter")),Centre)
            &&M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("VolumeRadii")),Radii)
            &&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Density)
            &&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Age")),Age);
        const FVector Physical=C->GetComponentScale().GetAbs()*50.f;
        P.Kernel=Pins&&!FVector(Centre).ContainsNaN()&&!FVector(Radii).ContainsNaN()
            &&FMath::IsFinite(Density)&&Density>=0.f&&Density<=4.f&&FMath::IsFinite(Age)&&Age>=0.f&&Age<=120.f
            &&FVector(Centre).Equals(C->GetComponentLocation(),.1f)&&FVector(Radii).Equals(Physical,.1f);
    }
    return P;
}
TMap<TWeakObjectPtr<AActor>,TSet<FName>> Warned;
TMap<TWeakObjectPtr<AActor>,TMap<FName,FString>> RejectionReasons;
TSet<TWeakObjectPtr<UStaticMeshComponent>> ShaderHidden;
bool Allowed(AWarfareFrontlineGameMode* B)
{
    return B&&!WarfareFRRUCampaignV82::Active(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV844Review"))
        &&FrontlineEffectsReviewSafetyV8216::Allowed(B);
}
struct FFixture
{
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;int32 Stage=-1;
    TArray<TWeakObjectPtr<AWarfareFrontlineImpact>> Made;
    struct FHidden{TWeakObjectPtr<AWarfareFrontlineImpact> A;bool Hidden;};TArray<FHidden> Hidden;
};
FFixture Fixture;
}

void FrontlineEffectsSafetyV844::Guard(AActor* A)
{
    if(!IsValid(A)||A->IsActorBeingDestroyed()||A->IsHidden())return;
    TInlineComponentArray<UStaticMeshComponent*,32> Cs;A->GetComponents(Cs);
    for(auto* C:Cs)if(C&&C->IsRegistered()&&(C->IsVisible()||ShaderHidden.Contains(C)))
    {
        if(const auto* ISM=Cast<UInstancedStaticMeshComponent>(C);ISM&&ISM->GetInstanceCount()==0)continue;
        const auto P=Observe(C);
        if(P.Safe())
        {
            if(ShaderHidden.Contains(C)&&Cast<UInstancedStaticMeshComponent>(C))C->SetVisibility(true);
            ShaderHidden.Remove(C);continue;
        }
        C->SetVisibility(false);
        if(P.StructuralSafe()&&!P.RenderDone){ShaderHidden.Add(C);continue;}
        ShaderHidden.Remove(C);
        RejectionReasons.FindOrAdd(A).Add(C->GetFName(),FString::Printf(TEXT("material=%s platform=%d finite=%d bounded=%d domain=%d blend=%d default=%d kernel=%d rt_done=%d rt_map=%d rt_no_fallback=%d rt_gpu=%d"),
            *GetPathNameSafe(P.Material),P.Platform,P.Finite,P.Size,P.Surface,P.Blend,P.Default,P.Kernel,P.RenderDone,P.RenderValid,P.RenderNoFallback,P.RenderGPU));
        // Only a genuinely rejected live proxy is logged. Editor compilation
        // can defer its availability; later normal visual ticks may retry it.
        auto& Names=Warned.FindOrAdd(A);
        if(!Names.Contains(C->GetFName()))
        {
            Names.Add(C->GetFName());
            UE_LOG(LogTemp,Warning,TEXT("WF_V844_FX_PROXY_REJECTED actor=%s component=%s material=%s platform=%d cube=%d finite=%d size=%d surface=%d blend=%d default=%d gt_shader=%d gt_complete=%d kernel=%d rt_done=%d rt_map=%d rt_no_fallback=%d rt_gpu=%d bounds_cm=%s"),
                *A->GetName(),*C->GetName(),*GetPathNameSafe(P.Material),P.Platform,P.Cube,P.Finite,P.Size,P.Surface,P.Blend,P.Default,P.Shader,P.Complete,P.Kernel,P.RenderDone,P.RenderValid,P.RenderNoFallback,P.RenderGPU,*P.Dimensions.ToString());
        }
    }
    if(Warned.Num()>128)for(auto I=Warned.CreateIterator();I;++I)if(!I.Key().IsValid()){RejectionReasons.Remove(I.Key());I.RemoveCurrent();}
}

TSharedRef<FJsonObject> FrontlineEffectsSafetyV844::Describe(AActor* A)
{
    auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Safe=IsValid(A);int32 Visible=0;
    if(IsValid(A))
    {
        TInlineComponentArray<UStaticMeshComponent*,32> Cs;A->GetComponents(Cs);
        for(auto* C:Cs)if(C&&C->IsRegistered()&&(C->IsVisible()||ShaderHidden.Contains(C)||(Warned.Contains(A)&&Warned.FindChecked(A).Contains(C->GetFName()))))
        {
            if(const auto* ISM=Cast<UInstancedStaticMeshComponent>(C);ISM&&ISM->GetInstanceCount()==0)continue;
            const auto P=Observe(C,true);Visible+=C->IsVisible();if(C->IsVisible())Safe&=P.Safe();auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("visible"),C->IsVisible());R->SetBoolField(TEXT("shader_evidence_pending"),ShaderHidden.Contains(C));
            if(const auto* Reasons=RejectionReasons.Find(A))if(const auto* Reason=Reasons->Find(C->GetFName()))R->SetStringField(TEXT("last_rejection_reason"),*Reason);
            R->SetStringField(TEXT("component"),C->GetName());R->SetStringField(TEXT("mesh"),GetPathNameSafe(C->GetStaticMesh()));
            R->SetStringField(TEXT("material"),GetPathNameSafe(P.Material));R->SetStringField(TEXT("base_material"),P.Material?GetPathNameSafe(P.Material->GetBaseMaterial()):FString());
            R->SetNumberField(TEXT("shader_platform"),P.Platform);R->SetBoolField(TEXT("cube"),P.Cube);R->SetBoolField(TEXT("finite"),P.Finite);R->SetBoolField(TEXT("bounded"),P.Size);
            R->SetBoolField(TEXT("surface_domain"),P.Surface);R->SetBoolField(TEXT("expected_blend"),P.Blend);R->SetBoolField(TEXT("bound_default_material"),P.Default);
            R->SetBoolField(TEXT("game_thread_shader_map_valid"),P.Shader);R->SetBoolField(TEXT("game_thread_shader_map_complete"),P.Complete);R->SetBoolField(TEXT("physical_kernel_binding"),P.Kernel);
            R->SetStringField(TEXT("bounds_cm"),P.Dimensions.ToString());R->SetBoolField(TEXT("render_thread_evidence_done"),P.RenderDone);R->SetBoolField(TEXT("render_thread_map_valid"),P.RenderValid);
            R->SetBoolField(TEXT("rendered_without_fallback"),P.RenderNoFallback);R->SetBoolField(TEXT("required_base_pass_gpu_shaders_ready"),P.RenderGPU);R->SetStringField(TEXT("render_resolved_material"),P.RenderName);
            R->SetNumberField(TEXT("basepass_vertex_shaders"),P.RenderVS);R->SetNumberField(TEXT("basepass_pixel_shaders"),P.RenderPS);
            R->SetStringField(TEXT("basepass_enumeration_state"),P.RenderGPU?TEXT("gpu_stages_observed"):TEXT("unknown_no_known_mesh_map_stages_observed"));
            TArray<TSharedPtr<FJsonValue>> Samples;for(const auto& N:P.ShaderSamples)Samples.Add(MakeShared<FJsonValueString>(N));R->SetArrayField(TEXT("mesh_shader_samples"),Samples);
            Rows.Add(MakeShared<FJsonValueObject>(R));
        }
    }
    J->SetBoolField(TEXT("visible_proxies_safe"),Safe);J->SetNumberField(TEXT("visible_proxies"),Visible);J->SetArrayField(TEXT("proxies"),Rows);
    J->SetNumberField(TEXT("rejected_component_count"),IsValid(A)&&Warned.Contains(A)?Warned.FindChecked(A).Num():0);return J;
}

struct FFrontlineEffectsSafetyV844Review
{
static void Bind(AWarfareFrontlineImpact* A,int32 Profile)
{
    if(Profile==5)A->ConfigureMineV8214(600.f);else if(Profile==4)A->ConfigureAirburstV64(1200.f,FVector(.8,.2,-1.f));
    else if(Profile==3)A->ConfigureHeavyBulletV8215(FVector(.8,.2,-1.f),true);else A->ConfigureBlast(Profile==2,Profile==1,FVector(.8,.2,-1.f));
    A->SetCasualtyDustV8216({A->GetActorLocation()+FVector(80,40,0)});A->bFirstTick=false;
}
static TSharedRef<FJsonObject> Inspect(AWarfareFrontlineImpact* A)
{
    auto J=FrontlineEffectsSafetyV844::Describe(A);bool History=true,Finite=true;float MaxStep=0,MaxScale=0,MaxDiameter=0;
    if(A&&A->Fragments)
    {
        const FVector MeshSize=A->Fragments->GetStaticMesh()?A->Fragments->GetStaticMesh()->GetBounds().BoxExtent*2.f:FVector::ZeroVector;
        History=A->Soil.Num()==A->FragmentTransformsV835.Num()&&A->Soil.Num()==A->PreviousFragmentTransformsV835.Num()&&A->Soil.Num()==A->Fragments->GetInstanceCount();
        for(int32 I=0;I<A->FragmentTransformsV835.Num();++I)
        {
            const auto& T=A->FragmentTransformsV835[I];Finite&=!T.ContainsNaN();MaxScale=FMath::Max(MaxScale,float(T.GetScale3D().GetAbs().GetMax()));
            MaxDiameter=FMath::Max(MaxDiameter,float((MeshSize*T.GetScale3D().GetAbs()).GetMax()));
            FTransform SubmittedCurrent,SubmittedPrevious;
            const bool Submitted=A->Fragments->GetInstanceTransform(I,SubmittedCurrent,false)&&A->Fragments->GetInstancePrevTransform(I,SubmittedPrevious,false);
            // A zero-scale retired shard's matrix has no recoverable rotation;
            // compare its submitted translation/scale, and rotation only while
            // that authored geometry still has nonzero dimensions.
            History&=Submitted&&SubmittedCurrent.GetLocation().Equals(T.GetLocation(),.001f)
                &&SubmittedCurrent.GetScale3D().Equals(T.GetScale3D(),.001f)
                &&(T.GetScale3D().GetAbs().GetMin()<.00001f||SubmittedCurrent.GetRotation().Equals(T.GetRotation(),.001f));
            Finite&=Submitted&&!SubmittedCurrent.ContainsNaN()&&!SubmittedPrevious.ContainsNaN();
            if(Submitted)MaxStep=FMath::Max(MaxStep,float(FVector::Dist(SubmittedCurrent.GetLocation(),SubmittedPrevious.GetLocation())));
        }
        J->SetNumberField(TEXT("profile"),A->BlastKind);J->SetNumberField(TEXT("age_s"),A->Age);J->SetNumberField(TEXT("instance_count"),A->Fragments->GetInstanceCount());
    }
    J->SetBoolField(TEXT("fragment_array_instance_counts_agree"),History);J->SetBoolField(TEXT("fragment_transforms_finite"),Finite);
    J->SetNumberField(TEXT("maximum_fragment_scale"),MaxScale);J->SetNumberField(TEXT("maximum_fragment_diameter_cm"),MaxDiameter);J->SetNumberField(TEXT("maximum_current_previous_displacement_cm"),MaxStep);
    return J;
}
static void Advance(AWarfareFrontlineImpact* A,float DT){A->bFirstTick=false;A->Tick(DT);}
};

TSharedRef<FJsonObject> FrontlineEffectsSafetyV844::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto J=MakeShared<FJsonObject>();bool Good=true;TArray<TSharedPtr<FJsonValue>> Rows;
    const auto Assert=[&](const FString& N,bool Pass){Good&=Pass;Check(N,Pass);};
    Assert(TEXT("v844_effects_safety_disposable_ukde_world"),Allowed(B));if(!Allowed(B)){J->SetBoolField(TEXT("success"),false);return J;}
    TArray<AWarfareFrontlineImpact*> Made;ON_SCOPE_EXIT{for(auto* A:Made)if(IsValid(A))A->Destroy();};
    // Two real profiles bind every retained and short-layer parent once. The
    // isolated checker alone waits for its one render-thread evidence batch;
    // production Guard is asynchronous and never flushes the renderer.
    TArray<AWarfareFrontlineImpact*> Warm;
    for(int32 Kind:{0,3})if(auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(FVector(0,0,0),FRotator::ZeroRotator))
    {A->SetActorTickEnabled(false);FFrontlineEffectsSafetyV844Review::Bind(A,Kind);FFrontlineEffectsSafetyV844Review::Advance(A,.08f);Warm.Add(A);}
    FlushRenderingCommands();for(auto* A:Warm)A->Destroy();
    for(int32 Profile:{0,1,2,4,5,3})
    {
        FVector At(Profile*260.f-700.f,300.f,0);At.Z=AWarfareFrontlineGround::HeightAt(At.X,At.Y);
        auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);Assert(TEXT("v844_effects_actual_actor_spawn"),A!=nullptr);if(!A)continue;Made.Add(A);A->SetActorTickEnabled(false);
        for(int32 Cycle=0;Cycle<3;++Cycle)
        {
            const int32 Kind=Cycle==1?3:Profile;FFrontlineEffectsSafetyV844Review::Bind(A,Kind);FFrontlineEffectsSafetyV844Review::Advance(A,.08f);
            auto R=FFrontlineEffectsSafetyV844Review::Inspect(A);R->SetNumberField(TEXT("cycle"),Cycle);Rows.Add(MakeShared<FJsonValueObject>(R));
            const FString Prefix=FString::Printf(TEXT("v844_fx_profile%d_cycle%d"),Profile,Cycle);
            Assert(Prefix+TEXT("_actual_visible_material_shader_and_physical_bounds"),R->GetBoolField(TEXT("visible_proxies_safe"))&&R->GetNumberField(TEXT("visible_proxies"))>0);
            Assert(Prefix+TEXT("_finite_matched_real_instances"),R->GetBoolField(TEXT("fragment_array_instance_counts_agree"))&&R->GetBoolField(TEXT("fragment_transforms_finite")));
            const float Diameter=R->GetNumberField(TEXT("maximum_fragment_diameter_cm"));Assert(Prefix+TEXT("_ordinary_clods_remain_authored_centimetres"),Kind==3||Kind==4||Diameter<=16.1f);
            if(Profile==0&&Cycle==0)
            {
                TInlineComponentArray<UStaticMeshComponent*,32> Cs;A->GetComponents(Cs);UStaticMeshComponent* Cube=nullptr;
                for(auto* C:Cs)if(C&&C->IsVisible()&&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==TEXT("/Engine/BasicShapes/Cube.Cube")){Cube=C;break;}
                Assert(TEXT("v844_fx_fail_closed_probe_has_real_visible_cube"),Cube!=nullptr);
                if(Cube)
                {
                    auto* Original=Cube->GetMaterial(0);Cube->SetMaterial(0,UMaterial::GetDefaultMaterial(MD_Surface));Cube->SetVisibility(true);Guard(A);
                    Assert(TEXT("v844_fx_explicit_opaque_default_cannot_remain_visible"),!Cube->IsVisible());
                    Cube->SetMaterial(0,Original);Cube->SetVisibility(true);Guard(A);
                    Assert(TEXT("v844_fx_valid_original_material_can_render_again"),Cube->IsVisible());
                    auto Fault=MakeShared<FJsonObject>();Fault->SetStringField(TEXT("intentional_fault_probe"),TEXT("bound_default_surface_material"));
                    Fault->SetBoolField(TEXT("rejected"),Warned.Contains(A));Rows.Add(MakeShared<FJsonValueObject>(Fault));
                }
            }
            FFrontlineEffectsSafetyV844Review::Advance(A,12.f);auto End=FFrontlineEffectsSafetyV844Review::Inspect(A);
            Assert(Prefix+TEXT("_large_delta_cleans_fragment_instances_before_reconfiguration"),End->GetBoolField(TEXT("fragment_array_instance_counts_agree"))&&End->GetNumberField(TEXT("instance_count"))==0);
        }
    }
    J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("cases"),Rows);
    J->SetStringField(TEXT("scope"),TEXT("Actual platform resources; asynchronous valid render maps and non-default resolved proxies; production reconfiguration and real submitted ISM transforms. Whole-map GT completion and unavailable mesh-pass enumeration remain diagnostic. Actual GPU appearance requires framed captures."));return J;
}

void FrontlineEffectsSafetyV844::ClearFixture(AWarfareFrontlineGameMode* B)
{
    if(Fixture.Battle.Get()!=B)return;for(const auto& W:Fixture.Made)if(auto* A=W.Get())A->Destroy();
    for(const auto& H:Fixture.Hidden)if(auto* A=H.A.Get())A->SetActorHiddenInGame(H.Hidden);Fixture=FFixture();
}
bool FrontlineEffectsSafetyV844::SetupFixture(AWarfareFrontlineGameMode* B,int32 Stage)
{
    if(!Allowed(B)||Stage<0||Stage>5)return false;
    if(Stage==0)
    {
        if(Fixture.Battle.IsValid()&&Fixture.Battle.Get()!=B)return false;ClearFixture(B);Fixture.Battle=B;
        for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I){Fixture.Hidden.Add({*I,I->IsHidden()});I->SetActorHiddenInGame(true);}
        int32 I=0;for(int32 Kind:{0,1,2,4,5,3})
        {
            FVector At(-650.f+(I%3)*650.f,-200.f+(I/3)*580.f,0);At.Z=AWarfareFrontlineGround::HeightAt(At.X,At.Y);
            auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);if(!A){ClearFixture(B);return false;}
            A->SetActorTickEnabled(false);Fixture.Made.Add(A);FFrontlineEffectsSafetyV844Review::Bind(A,Kind);FFrontlineEffectsSafetyV844Review::Advance(A,.12f);++I;
        }
    }
    else
    {
        if(Fixture.Battle.Get()!=B||(Stage!=1&&Fixture.Stage!=Stage-1)||Fixture.Made.Num()!=6)return false;int32 I=0;
        for(const auto& W:Fixture.Made)
        {
            auto* A=W.Get();if(!A)return false;
            if(Stage==1||Stage==4){FFrontlineEffectsSafetyV844Review::Bind(A,(I%3)==0?5:(I%3)==1?4:0);FFrontlineEffectsSafetyV844Review::Advance(A,.12f);}
            else FFrontlineEffectsSafetyV844Review::Advance(A,Stage==2?.9f:Stage==3?12.f:.13f);++I;
        }
    }
    Fixture.Stage=Stage;return true;
}
TSharedRef<FJsonObject> FrontlineEffectsSafetyV844::DescribeFixture(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("isolated"),Allowed(B)&&Fixture.Battle.Get()==B);J->SetNumberField(TEXT("stage"),Fixture.Stage);
    TArray<TSharedPtr<FJsonValue>> Rows;for(const auto& W:Fixture.Made)if(auto* A=W.Get())Rows.Add(MakeShared<FJsonValueObject>(FFrontlineEffectsSafetyV844Review::Inspect(A)));
    J->SetArrayField(TEXT("actors"),Rows);return J;
}

bool FrontlineEffectsSafetyV844::SetFixtureLiveV844(AWarfareFrontlineGameMode* B,bool Live)
{
    if(!Allowed(B)||Fixture.Battle.Get()!=B)return false;
    for(const auto& W:Fixture.Made)if(auto* A=W.Get())A->SetActorTickEnabled(Live);else return false;
    return true;
}
