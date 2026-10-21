#include "WarfareFrontlineWaterReviewV83.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFRRUCampaignV82.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"

namespace FrontlineWaterReviewV83
{
namespace
{
struct FState
{
    TWeakObjectPtr<AWarfareFrontlineGround> Ground;
    TWeakObjectPtr<UStaticMeshComponent> Component;
    TStrongObjectPtr<UStaticMesh> OriginalMesh,AcceptedMesh,CandidateMesh;
    TStrongObjectPtr<UMaterialInterface> OriginalMaterial,AcceptedMaterial,CandidateMaterial;
    TStrongObjectPtr<UMaterialInstanceDynamic> Snapshot;
    TWeakObjectPtr<UFrontlineWaterResponseV51> Response;
    bool Visible=false,ResponseTick=false;int32 Reflection=0,WireCount=0;uint32 ReflectionFlags=0;
    int32 LegacyTriangles=0,RegionalTriangles=0,LegacyEdgeVertices=0;float NearestWire=MAX_flt;
    FVector Focus=FVector::ZeroVector;EContribution Mode=EContribution::Restore;
};
TUniquePtr<FState> State;
const FName ReflectionReviewTag(TEXT("WFV83WaterContributionOnly"));
bool Allowed(AWarfareFrontlineGameMode* B)
{
    return FrontlineEffectsReviewSafetyV8216::Allowed(B)&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV83WaterReview"))&&!GUsingNullRHI;
}
FString CandidatePath(UStaticMesh* Source)
{
    const FString Path=Source->GetOutermost()->GetName(),Prefix=TEXT("/Game/Warfare/Field/V");
    if(!Path.StartsWith(Prefix))return FString();
    const int32 End=Path.Find(TEXT("/"),ESearchCase::CaseSensitive,ESearchDir::FromStart,Prefix.Len());if(End==INDEX_NONE)return FString();
    const FString Version=Path.Mid(Prefix.Len(),End-Prefix.Len());if(!Version.IsNumeric())return FString();
    return FString::Printf(TEXT("/Game/Warfare/Field/V83/Water/%s_S%s.%s_S%s"),*Source->GetName(),*Version,*Source->GetName(),*Version);
}
void Restore()
{
    if(!State)return;
    if(auto* C=State->Component.Get())
    {C->SetStaticMesh(State->OriginalMesh.Get());C->SetMaterial(0,State->OriginalMaterial.Get());C->SetVisibility(State->Visible);}
    if(auto* R=State->Response.Get())R->SetComponentTickEnabled(State->ResponseTick);
    // UE 5.8 forbids replacing a Constructor-priority value through
    // SetWithCurrentPriority. A tagged Temp entry keeps the original history;
    // Unset restores its value and all original flags, including Constructor.
    if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.SingleLayer.Reflection")))V->Unset(ECVF_SetByTemp,ReflectionReviewTag);
    State.Reset();
}
bool Begin(AWarfareFrontlineGameMode* B)
{
    AWarfareFrontlineGround* G=nullptr;
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(IsValid(It->BasinWaterV44)&&It->BasinWaterV44->IsRegistered()&&!It->IsHidden()){G=*It;break;}
    auto* C=G?G->BasinWaterV44.Get():nullptr;if(!C||!C->GetStaticMesh()||!C->GetMaterial(0))return false;
    auto New=MakeUnique<FState>();New->Ground=G;New->Component=C;New->OriginalMesh.Reset(C->GetStaticMesh());New->OriginalMaterial.Reset(C->GetMaterial(0));New->Visible=C->IsVisible();
    UStaticMesh* Source=C->GetStaticMesh();const FString Name=Source->GetName();
    if(Source->GetOutermost()->GetName().StartsWith(TEXT("/Game/Warfare/Field/V83/Water/")))
    {
        const int32 At=Name.Find(TEXT("_S"),ESearchCase::CaseSensitive,ESearchDir::FromEnd);if(At==INDEX_NONE||!Name.Mid(At+2).IsNumeric())return false;
        const FString Original=Name.Left(At),Version=Name.Mid(At+2);
        Source=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V%s/Water/%s.%s"),*Version,*Original,*Original));
    }
    if(!Source)return false;New->AcceptedMesh.Reset(Source);
    const FString Candidate=CandidatePath(Source);New->CandidateMesh.Reset(Candidate.IsEmpty()?nullptr:LoadObject<UStaticMesh>(nullptr,*Candidate));
    New->AcceptedMaterial.Reset(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V75/Materials/MI_BasinWater_V75.MI_BasinWater_V75")));
    New->CandidateMaterial.Reset(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V83/Materials/MI_BasinWater_V83.MI_BasinWater_V83")));
    auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.SingleLayer.Reflection"));if(!V||!New->AcceptedMaterial.Get()||!New->CandidateMesh.Get()||!New->CandidateMaterial.Get())return false;
    New->Reflection=V->GetInt();New->ReflectionFlags=V->GetFlags();New->Response=G->FindComponentByClass<UFrontlineWaterResponseV51>();
    New->ResponseTick=New->Response.IsValid()&&New->Response->IsComponentTickEnabled();
    // Capture a constant production parameter snapshot. Do not Rebind: it
    // would clear the real ripple/blood state. Restore the same original MID.
    New->Snapshot.Reset(UMaterialInstanceDynamic::Create(New->OriginalMaterial.Get(),G));New->Snapshot->CopyMaterialUniformParameters(New->OriginalMaterial.Get());
    const auto* Render=New->CandidateMesh->GetRenderData();if(!Render||Render->LODResources.IsEmpty()||!New->CandidateMesh->bAllowCPUAccess)return false;
    const auto& L=Render->LODResources[0];const auto& P=L.VertexBuffers.PositionVertexBuffer;const auto& Colours=L.VertexBuffers.ColorVertexBuffer;
    const auto Indices=L.IndexBuffer.GetArrayView();if(!Indices.Num()||Colours.GetNumVertices()!=P.GetNumVertices())return false;
    TArray<FVector> Wires;
    for(TObjectIterator<UStaticMeshComponent> It;It;++It)if(It->GetWorld()==B->GetWorld()&&It->IsRegistered()&&It->IsVisible()&&It->GetStaticMesh()
        &&It->GetStaticMesh()->GetName()==TEXT("SM_FieldWire_Coil"))Wires.Add(It->GetComponentLocation());
    New->WireCount=Wires.Num();float Best=-MAX_flt;
    for(uint32 I=0;I<P.GetNumVertices();++I){const auto Col=Colours.VertexColor(I);New->LegacyEdgeVertices+=Col.B<128&&Col.A<32;}
    for(int32 I=0;I+2<Indices.Num();I+=3)
    {
        const auto A=Colours.VertexColor(Indices[I]),D=Colours.VertexColor(Indices[I+1]),E=Colours.VertexColor(Indices[I+2]);
        if(A.B>=128||D.B>=128||E.B>=128){++New->RegionalTriangles;continue;}++New->LegacyTriangles;
        const float Depth=(FLinearColor(A).R+FLinearColor(D).R+FLinearColor(E).R)*4.f;if(Depth<2.5f)continue;
        const FVector Local(FVector3f((P.VertexPosition(Indices[I])+P.VertexPosition(Indices[I+1])+P.VertexPosition(Indices[I+2]))/3.f));
        const FVector Point=C->GetComponentTransform().TransformPosition(Local);if(FMath::Abs(Point.Y)>1800.f)continue;
        float Distance=MAX_flt;for(const auto& Wire:Wires)Distance=FMath::Min(Distance,float(FVector::DistSquaredXY(Point,Wire)));
        const float Score=Depth*3.f-(Wires.IsEmpty()?float(Point.SizeSquared2D())*.00001f:FMath::Sqrt(Distance)*.03f);
        if(Score>Best){Best=Score;New->Focus=Point;New->NearestWire=Wires.IsEmpty()?MAX_flt:FMath::Sqrt(Distance);}
    }
    if(Best==-MAX_flt||!New->LegacyTriangles||!New->LegacyEdgeVertices)return false;
    if(New->Response.IsValid())New->Response->SetComponentTickEnabled(false);State=MoveTemp(New);return true;
}
const TCHAR* Name(EContribution Mode)
{
    switch(Mode){case EContribution::Baseline:return TEXT("baseline");case EContribution::Hidden:return TEXT("hidden");case EContribution::NoReflection:return TEXT("no-reflection");case EContribution::Candidate:return TEXT("candidate");default:return TEXT("restore");}
}
}
bool SetContribution(AWarfareFrontlineGameMode* B,EContribution Mode,FVector& Focus)
{
    if(!Allowed(B))return false;
    if(Mode==EContribution::Restore){if(State)Focus=State->Focus;Restore();return true;}
    if(State&&(!State->Ground.IsValid()||!State->Component.IsValid()||State->Ground->GetWorld()!=B->GetWorld()
        ||State->Ground->BasinWaterV44.Get()!=State->Component.Get()))Restore();
    if(!State&&!Begin(B))return false;
    auto* C=State->Ground->BasinWaterV44.Get();if(!C)return false;
    const bool Candidate=Mode==EContribution::Candidate;auto* Material=Candidate?State->CandidateMaterial.Get():State->AcceptedMaterial.Get();
    auto* MID=UMaterialInstanceDynamic::Create(Material,State->Ground.Get());MID->CopyMaterialUniformParameters(State->Snapshot.Get());
    C->SetStaticMesh(Candidate?State->CandidateMesh.Get():State->AcceptedMesh.Get());C->SetMaterial(0,MID);C->SetVisibility(Mode!=EContribution::Hidden&&State->Visible);
    if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.SingleLayer.Reflection")))
    {
        if(Mode==EContribution::NoReflection)V->Set(0,ECVF_SetByTemp,ReflectionReviewTag);
        else V->Unset(ECVF_SetByTemp,ReflectionReviewTag);
    }
    State->Mode=Mode;Focus=State->Focus;
    FString Dir;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir);
    return FFileHelper::SaveStringToFile(Describe(B),*(Dir/FString::Printf(TEXT("water-v83-%s.json"),Name(Mode))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FString Describe(AWarfareFrontlineGameMode* B)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("strict_actual_rhi_review"),Allowed(B));
    if(Allowed(B)&&State&&State->Ground.IsValid())if(auto* C=State->Ground->BasinWaterV44.Get())
    {
        J->SetStringField(TEXT("mode"),Name(State->Mode));J->SetStringField(TEXT("component"),C->GetPathName());J->SetStringField(TEXT("actual_mesh"),GetPathNameSafe(C->GetStaticMesh()));
        J->SetStringField(TEXT("actual_material"),GetPathNameSafe(C->GetMaterial(0)));J->SetStringField(TEXT("base_material"),C->GetMaterial(0)?GetPathNameSafe(C->GetMaterial(0)->GetMaterial()):TEXT("none"));
        J->SetStringField(TEXT("accepted_source_mesh"),GetPathNameSafe(State->AcceptedMesh.Get()));J->SetStringField(TEXT("focus_world_cm"),State->Focus.ToString());
        J->SetBoolField(TEXT("visible"),C->IsVisible());J->SetBoolField(TEXT("no_collision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        J->SetBoolField(TEXT("receives_decals"),C->bReceivesDecals);J->SetNumberField(TEXT("legacy_triangles"),State->LegacyTriangles);J->SetNumberField(TEXT("regional_triangles"),State->RegionalTriangles);
        J->SetNumberField(TEXT("legacy_edge_vertices"),State->LegacyEdgeVertices);J->SetNumberField(TEXT("actual_wire_components"),State->WireCount);J->SetNumberField(TEXT("nearest_wire_cm"),State->NearestWire);
        if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.SingleLayer.Reflection")))
        {
            J->SetNumberField(TEXT("reflection"),V->GetInt());J->SetNumberField(TEXT("reflection_original_value"),State->Reflection);
            J->SetNumberField(TEXT("reflection_original_flags"),State->ReflectionFlags);J->SetNumberField(TEXT("reflection_actual_flags"),V->GetFlags());
            J->SetNumberField(TEXT("reflection_original_priority_bits"),State->ReflectionFlags&ECVF_SetByMask);J->SetNumberField(TEXT("reflection_actual_priority_bits"),V->GetFlags()&ECVF_SetByMask);
            J->SetBoolField(TEXT("reflection_priority_unchanged"),V->GetFlags()==State->ReflectionFlags);J->SetStringField(TEXT("reflection_restore_method"),TEXT("tagged SetByTemp then Unset; no SetFlags override"));
        }
        auto* Material=C->GetMaterial(0);auto* Resource=Material?Material->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
        J->SetBoolField(TEXT("actual_shader_map_complete"),Resource&&Resource->IsGameThreadShaderMapComplete());J->SetBoolField(TEXT("actual_shader_map_valid"),Resource&&Resource->HasValidGameThreadShaderMap());
        J->SetBoolField(TEXT("single_layer_water"),Material&&Material->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));J->SetNumberField(TEXT("blend_mode"),Material?int32(Material->GetBlendMode()):-1);
    }
    FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));return Text;
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    Check(TEXT("v83_water_strict_disposable_uk_de_rhi_scene"),Allowed(B));if(!Allowed(B))return;
    FVector Focus;Check(TEXT("v83_water_actual_paired_candidate_and_b0_near_wire"),SetContribution(B,EContribution::Candidate,Focus));if(!State)return;
    ON_SCOPE_EXIT{SetContribution(B,EContribution::Restore,Focus);};
    const TStrongObjectPtr<UStaticMesh> OriginalMesh(State->OriginalMesh.Get());
    const TStrongObjectPtr<UMaterialInterface> OriginalMaterial(State->OriginalMaterial.Get());
    Check(TEXT("v83_water_actual_production_default_pair_without_candidate_flag"),
        !FParse::Param(FCommandLine::Get(),TEXT("WFV83WaterCandidate"))
        &&OriginalMesh.IsValid()&&OriginalMaterial.IsValid()
        &&OriginalMesh->GetOutermost()->GetName().StartsWith(TEXT("/Game/Warfare/Field/V83/Water/"))
        &&OriginalMaterial->GetMaterial()->GetOutermost()->GetName()==TEXT("/Game/Warfare/Field/V83/Materials/M_BasinWater_V83"));
    const bool OriginalVisible=State->Visible,OriginalTick=State->ResponseTick;
    const auto Response=State->Response;const int32 OriginalReflection=State->Reflection;const uint32 OriginalReflectionFlags=State->ReflectionFlags;
    auto* C=State->Ground->BasinWaterV44.Get();auto* M=C?C->GetMaterial(0):nullptr;auto* R=M?M->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
    Check(TEXT("v83_water_actual_candidate_masked_slw_shader_ready"),M&&M->GetBlendMode()==BLEND_Masked&&M->GetShadingModels().HasShadingModel(MSM_SingleLayerWater)&&R&&R->IsGameThreadShaderMapComplete()&&R->HasValidGameThreadShaderMap());
    Check(TEXT("v83_water_actual_candidate_contact_geometry_and_no_decal_collision"),C&&C->GetStaticMesh()==State->CandidateMesh.Get()&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->bReceivesDecals&&State->LegacyEdgeVertices>0&&State->WireCount>0);
    Check(TEXT("v83_water_actual_baseline_restores_paired_source"),SetContribution(B,EContribution::Baseline,Focus)&&C->GetStaticMesh()==State->AcceptedMesh.Get());
    Check(TEXT("v83_water_actual_hidden_contribution"),SetContribution(B,EContribution::Hidden,Focus)&&!C->IsVisible());
    auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.SingleLayer.Reflection"));Check(TEXT("v83_water_actual_no_reflection_contribution"),SetContribution(B,EContribution::NoReflection,Focus)&&V&&V->GetInt()==0);
    Check(TEXT("v83_water_actual_candidate_restores_original_reflection"),SetContribution(B,EContribution::Candidate,Focus)&&V&&V->GetInt()==State->Reflection&&V->GetFlags()==State->ReflectionFlags);
    SetContribution(B,EContribution::Restore,Focus);
    Check(TEXT("v83_water_actual_component_mid_tick_reflection_restored"),C&&C->GetStaticMesh()==OriginalMesh.Get()&&C->GetMaterial(0)==OriginalMaterial.Get()
        &&C->IsVisible()==OriginalVisible&&(!Response.IsValid()||Response->IsComponentTickEnabled()==OriginalTick)
        &&V&&V->GetInt()==OriginalReflection&&V->GetFlags()==OriginalReflectionFlags);
}
}
