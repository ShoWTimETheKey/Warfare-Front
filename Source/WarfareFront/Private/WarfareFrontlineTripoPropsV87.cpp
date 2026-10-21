#include "WarfareFrontlineTripoPropsV87.h"
#include "WarfareFrontline.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#if WITH_EDITOR
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineGroundContactV8449.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineTerrainProfilesV51.h"
#include "WarfareFrontlineTerrainMaterialV47.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFRRUCampaignV82.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "ContentStreaming.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureStreamingTypes.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "MeshUVChannelInfo.h"
#include "TextureCompiler.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
constexpr const TCHAR* MeshPath=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_h31_85eb6262_review01/import05/Meshes/SM_Tripo_worn_sandbag_h31_85eb6262_review01.SM_Tripo_worn_sandbag_h31_85eb6262_review01");
constexpr const TCHAR* MaterialPath=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_h31_85eb6262_review01/import05/Materials/M_Tripo_worn_sandbag_h31_85eb6262_review01_0.M_Tripo_worn_sandbag_h31_85eb6262_review01_0");
constexpr const TCHAR* MatchedMeshPath=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_h31_85eb6262_review04/import01/Meshes/SM_Tripo_worn_sandbag_h31_85eb6262_review04.SM_Tripo_worn_sandbag_h31_85eb6262_review04");
constexpr const TCHAR* MatchedPackage=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_h31_85eb6262_review04/import01");
constexpr const TCHAR* SmartMeshPath=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_smartmesh_p1_9728_smart03/import01/Meshes/SM_Tripo_worn_sandbag_smartmesh_p1_9728_smart03.SM_Tripo_worn_sandbag_smartmesh_p1_9728_smart03");
constexpr const TCHAR* SmartMaterialPath=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_smartmesh_p1_9728_smart03/import01/Materials/M_Tripo_worn_sandbag_smartmesh_p1_9728_smart03_0.M_Tripo_worn_sandbag_smartmesh_p1_9728_smart03_0");
constexpr const TCHAR* SmartPackage=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_smartmesh_p1_9728_smart03/import01");
struct FTriangle { FVector A,B,C; };
struct FGroup
{
    TWeakObjectPtr<UInstancedStaticMeshComponent> Component;
    TStrongObjectPtr<UStaticMesh> Original;
    TArray<TStrongObjectPtr<UMaterialInterface>> Materials;
    TArray<FTransform> Transforms;
    ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;
};
struct FFootSample
{
    FVector World=FVector::ZeroVector;
    double GroundZ=0,LowerTopZ=0,SupportZ=0,Gap=0;
    bool HasLowerTop=false;
};
struct FPlaced
{
    int32 Group=0,Instance=0,Bearing=INDEX_NONE;
    FTransform Original,Candidate,FixedCandidate;
    double MinGap=0,MaxGap=0,ContactError=0;
    double FixedMinGap=0,FixedMaxGap=0,PitchDelta=0,RollDelta=0,ChangedAngle=0;
    int32 LowerSurfaceHits=0;
    int32 FitPosesTested=0;
    bool FitAttempted=false,FitApplied=false;
    TArray<FFootSample> Feet,FixedFeet;
};
struct FState
{
    TArray<FGroup> Groups;
    TArray<FPlaced> Bags;
    TArray<FTriangle> Triangles;
    TArray<FVector> Feet;
    TStrongObjectPtr<UStaticMesh> Candidate;
    TStrongObjectPtr<UMaterialInterface> Material;
    TArray<TStrongObjectPtr<UMaterialInterface>> CandidateMaterials;
    FVector Focus=FVector::ZeroVector;
    bool Switched=false;
    bool MatchedNormals=false;
    bool Smart03=false;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TSharedPtr<FState>> States;

TSharedRef<FJsonObject> Point(const FVector& V)
{auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("x"),V.X);J->SetNumberField(TEXT("y"),V.Y);J->SetNumberField(TEXT("z"),V.Z);return J;}
TSharedRef<FJsonObject> Quaternion(const FQuat& Q)
{auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("x"),Q.X);J->SetNumberField(TEXT("y"),Q.Y);J->SetNumberField(TEXT("z"),Q.Z);J->SetNumberField(TEXT("w"),Q.W);return J;}
TArray<TSharedPtr<FJsonValue>> FootRows(const TArray<FFootSample>& Feet)
{
    TArray<TSharedPtr<FJsonValue>> Rows;
    for(int32 I=0;I<Feet.Num();++I)
    {
        const auto& F=Feet[I];auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("foot"),I);
        R->SetObjectField(TEXT("world_position"),Point(F.World));R->SetNumberField(TEXT("indexed_ground_and_bank_height_cm"),F.GroundZ);
        R->SetBoolField(TEXT("actual_lower_triangle_hit"),F.HasLowerTop);
        if(F.HasLowerTop)R->SetNumberField(TEXT("actual_lower_top_height_cm"),F.LowerTopZ);
        R->SetNumberField(TEXT("chosen_support_height_cm"),F.SupportZ);R->SetNumberField(TEXT("gap_cm"),F.Gap);
        Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    return Rows;
}
TSharedRef<FJsonObject> Result(bool Success,const FString& Scope)
{auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Success);J->SetStringField(TEXT("scope"),Scope);J->SetBoolField(TEXT("production_promoted"),false);return J;}
TArray<UTexture*> CandidateTextures(const FState& S)
{
    TArray<UTexture*> Result;
    for(const auto& M:S.CandidateMaterials)if(M.IsValid())
    {
        TArray<UTexture*> Used;M->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(EMaterialQualityLevel::High));
        for(auto* Texture:Used)Result.AddUnique(Texture);
    }
    if(S.CandidateMaterials.IsEmpty()&&S.Material.IsValid())S.Material->GetUsedTextures(Result,TOptional<EMaterialQualityLevel::Type>(EMaterialQualityLevel::High));
    return Result;
}
FString CandidateTexturePrefix(const FState& S)
{return S.Smart03?FString(SmartPackage)+TEXT("/Textures/"):S.MatchedNormals?FString(MatchedPackage)+TEXT("/Textures/"):TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_worn_sandbag_h31_85eb6262_review01/import05/Textures/");}
int32 ExpectedTextureEdge(const FState& S,const UTexture* Texture)
{
    if(!S.Smart03)return 4096;
    if(!Texture||!Texture->GetPathName().StartsWith(CandidateTexturePrefix(S)))return 0;
    const FString Name=Texture->GetName();
    if(Name==TEXT("T_Tripo_worn_sandbag_smartmesh_p1_9728_smart03_0_Color"))return 4096;
    if(Name==TEXT("T_Tripo_worn_sandbag_smartmesh_p1_9728_smart03_1_Normal")||
       Name==TEXT("T_Tripo_worn_sandbag_smartmesh_p1_9728_smart03_2_Masks"))return 2048;
    return 0;
}

TSharedRef<FJsonObject> MaterialSampling(UMaterialInterface* M,const FMeshUVChannelInfo* UV)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("material"),GetPathNameSafe(M));
    if(!M)return J;
    J->SetBoolField(TEXT("has_serialized_texture_streaming_data"),M->HasTextureStreamingData());
    TArray<TSharedPtr<FJsonValue>> Entries,Textures;
    for(const auto& E:M->GetTextureStreamingData())
    {
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("texture_name"),E.TextureName.ToString());
        R->SetNumberField(TEXT("uv_channel"),E.UVChannelIndex);R->SetNumberField(TEXT("sampling_scale"),E.SamplingScale);
        R->SetBoolField(TEXT("valid_sampling_entry"),E.IsValid(false));Entries.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("serialized_sampling_entries"),Entries);
    TArray<UTexture*> Used;M->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(EMaterialQualityLevel::High));
    for(auto* T:Used)if(T)
    {
        FTextureSamplingInfo Sampling;const bool Valid=M->CalculateTexturesSamplingInfo(T,Sampling);
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("texture"),T->GetPathName());
        R->SetBoolField(TEXT("actual_sampling_info_valid"),Valid);
        R->SetNumberField(TEXT("world_cm_per_texture_uv"),Valid&&UV?Sampling.GetTextureDensity(*UV):0);
        TArray<TSharedPtr<FJsonValue>> Scales;for(int32 I=0;I<MAX_TEXCOORDS;++I)
            Scales.Add(MakeShared<FJsonValueNumber>(Sampling.ChannelMinSamplingScale[I]));
        R->SetArrayField(TEXT("channel_min_sampling_scale"),Scales);Textures.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("actual_used_texture_sampling"),Textures);return J;
}

/** Actual transformed triangle ray, expressed in world z so no analytic top proxy is used. */
bool Surface(const TArray<FTriangle>& Triangles,const FTransform& Transform,double X,double Y,bool Highest,double& Z)
{
    const FVector O=Transform.InverseTransformPosition(FVector(X,Y,0));
    const FVector D=Transform.InverseTransformVector(FVector::UpVector);
    bool Found=false;Z=Highest?-MAX_dbl:MAX_dbl;
    for(const auto& F:Triangles)
    {
        const FVector E1=F.B-F.A,E2=F.C-F.A,P=FVector::CrossProduct(D,E2);
        const double Det=FVector::DotProduct(E1,P);if(FMath::Abs(Det)<1.e-12)continue;
        const FVector T=O-F.A;const double U=FVector::DotProduct(T,P)/Det;if(U<-.000001||U>1.000001)continue;
        const FVector Q=FVector::CrossProduct(T,E1);const double V=FVector::DotProduct(D,Q)/Det;
        if(V<-.000001||U+V>1.000001)continue;
        const double Hit=FVector::DotProduct(E2,Q)/Det;if(!FMath::IsFinite(Hit))continue;
        Z=Highest?FMath::Max(Z,Hit):FMath::Min(Z,Hit);Found=true;
    }
    return Found;
}
bool ReadCandidateGeometry(FState& S)
{
    const FMeshDescription* Mesh=S.Candidate->GetMeshDescription(0);if(!Mesh)return false;
    const FStaticMeshConstAttributes Attributes(*Mesh);const auto Positions=Attributes.GetVertexPositions();
    S.Triangles.Reset();FBox Bounds(ForceInit);
    for(const FTriangleID Id:Mesh->Triangles().GetElementIDs())
    {
        const auto Corners=Mesh->GetTriangleVertexInstances(Id);if(Corners.Num()!=3)return false;
        FTriangle F;FVector* Values[]={&F.A,&F.B,&F.C};
        for(int32 I=0;I<3;++I){const auto P=Positions[Mesh->GetVertexInstanceVertex(Corners[I])];*Values[I]=FVector(P);if(Values[I]->ContainsNaN())return false;Bounds+=*Values[I];}
        S.Triangles.Add(F);
    }
    if(!Bounds.IsValid||S.Triangles.IsEmpty())return false;
    S.Feet.Reset();for(double XF:{.25,.5,.75})for(double YF:{.25,.5,.75})
    {
        const double X=FMath::Lerp(Bounds.Min.X,Bounds.Max.X,XF),Y=FMath::Lerp(Bounds.Min.Y,Bounds.Max.Y,YF);double Z=0;
        if(!Surface(S.Triangles,FTransform::Identity,X,Y,false,Z))return false;
        S.Feet.Add(FVector(X,Y,Z));
    }
    return S.Feet.Num()==9;
}
bool GroundHeight(AWarfareFrontlineGameMode* B,AWarfareFrontlineGround* Ground,double X,double Y,double& Z)
{
    FVector P,N;if(!FrontlineGroundContactV8449::Sample(Ground,float(X),float(Y),P,N))return false;
    Z=P.Z;float Bank=0;if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,P,Bank))Z=FMath::Max(Z,double(Bank));return true;
}
TSharedRef<FJsonObject> Diagnostics(const FState& S,AWarfareFrontlineGameMode* B)
{
    auto J=Result(true,TEXT("Actual private scene visuals only; indexed ground, collision, routes and gameplay authority retained."));
    J->SetNumberField(TEXT("groups"),S.Groups.Num());J->SetNumberField(TEXT("instances"),S.Bags.Num());
    J->SetBoolField(TEXT("candidate_installed"),S.Switched);J->SetBoolField(TEXT("g_is_editor"),GIsEditor);J->SetObjectField(TEXT("fixed_camera_focus"),Point(S.Focus));
    J->SetStringField(TEXT("private_candidate_descriptor"),S.Smart03?TEXT("smart03/import01 vendor Smart Mesh paired PBR"):S.MatchedNormals?TEXT("review04/import01 matched normal per LOD"):TEXT("review01/import05 original PBR candidate"));
    J->SetNumberField(TEXT("expected_candidate_material_slots"),S.MatchedNormals?4:1);
    auto Streaming=MakeShared<FJsonObject>();auto& Manager=IStreamingManager::Get();
    Streaming->SetBoolField(TEXT("texture_streaming_enabled"),Manager.IsTextureStreamingEnabled());
    auto& Assets=Manager.GetRenderAssetStreamingManager();
    Streaming->SetNumberField(TEXT("pool_bytes"),double(Assets.GetPoolSize()));
    Streaming->SetNumberField(TEXT("required_pool_bytes"),double(Assets.GetRequiredPoolSize()));
    Streaming->SetNumberField(TEXT("over_budget_bytes"),double(Assets.GetMemoryOverBudget()));
    TArray<TSharedPtr<FJsonValue>> Views;
    for(int32 I=0;I<Manager.GetNumViews();++I)
    {
        const auto& V=Manager.GetViewInformation(I);auto R=MakeShared<FJsonObject>();
        R->SetObjectField(TEXT("origin"),Point(V.ViewOrigin));R->SetNumberField(TEXT("screen_size_consumed_by_streamer"),V.ScreenSize);
        R->SetNumberField(TEXT("legacy_fov_screen_size"),V.FOVScreenSize);R->SetNumberField(TEXT("boost"),V.BoostFactor);
        R->SetNumberField(TEXT("duration_seconds"),V.Duration);R->SetBoolField(TEXT("override_location"),V.bOverrideLocation);
        R->SetBoolField(TEXT("matches_current_world"),V.World.Get()==B->GetWorld());Views.Add(MakeShared<FJsonValueObject>(R));
    }
    Streaming->SetArrayField(TEXT("actual_final_views"),Views);
    for(const TCHAR* Name:{TEXT("r.Streaming.MaxEffectiveScreenSize"),TEXT("r.Streaming.MipBias"),TEXT("r.Streaming.UseMaterialData"),
        TEXT("r.Streaming.Boost"),TEXT("s.StreamableAssets.UseSimpleStreamableAssetManager")})
        if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))Streaming->SetStringField(Name,C->GetString());
    if(auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController()))if(PC->BattleCamera)
    {
        const auto* Camera=PC->BattleCamera->GetCameraComponent();int32 W=0,H=0;PC->GetViewportSize(W,H);
        const double Distance=FVector::Distance(PC->BattleCamera->GetActorLocation(),S.Focus);
        auto R=MakeShared<FJsonObject>();R->SetObjectField(TEXT("position"),Point(PC->BattleCamera->GetActorLocation()));
        R->SetNumberField(TEXT("actual_ortho_width_cm"),Camera?Camera->OrthoWidth:0);
        R->SetNumberField(TEXT("distance_to_original_focus_cm"),Distance);R->SetNumberField(TEXT("viewport_width_pixels"),W);
        R->SetBoolField(TEXT("controller_tick_enabled"),PC->IsActorTickEnabled());
        R->SetNumberField(TEXT("equivalent_screen_size_at_original_focus"),Camera&&Camera->OrthoWidth>1?W*2.*Distance/Camera->OrthoWidth:0);
        Streaming->SetObjectField(TEXT("actual_private_camera"),R);
    }
    J->SetObjectField(TEXT("actual_streaming_manager"),Streaming);
    TArray<TSharedPtr<FJsonValue>> Contacts,Groups;
    bool Authority=true,Weather=true;int32 Upper=0,FitAttempts=0,FitApplied=0,FitPoses=0;double MaxChangedAngle=0;
    for(const auto& Bag:S.Bags)
    {
        auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("group"),Bag.Group);R->SetNumberField(TEXT("instance"),Bag.Instance);
        R->SetNumberField(TEXT("bearing_index"),Bag.Bearing);Upper+=Bag.Bearing!=INDEX_NONE;
        R->SetNumberField(TEXT("min_gap_cm"),Bag.MinGap);R->SetNumberField(TEXT("max_gap_cm"),Bag.MaxGap);
        R->SetNumberField(TEXT("embed_contact_error_cm"),Bag.ContactError);R->SetNumberField(TEXT("lower_surface_hits"),Bag.LowerSurfaceHits);
        R->SetObjectField(TEXT("original_position"),Point(Bag.Original.GetLocation()));R->SetObjectField(TEXT("candidate_position"),Point(Bag.Candidate.GetLocation()));
        R->SetObjectField(TEXT("original_quaternion"),Quaternion(Bag.Original.GetRotation()));
        R->SetObjectField(TEXT("fixed_candidate_quaternion"),Quaternion(Bag.FixedCandidate.GetRotation()));
        R->SetObjectField(TEXT("candidate_quaternion"),Quaternion(Bag.Candidate.GetRotation()));
        if(S.Bags.IsValidIndex(Bag.Bearing))
        {
            R->SetObjectField(TEXT("bearing_original_quaternion"),Quaternion(S.Bags[Bag.Bearing].Original.GetRotation()));
            R->SetObjectField(TEXT("bearing_candidate_quaternion"),Quaternion(S.Bags[Bag.Bearing].Candidate.GetRotation()));
        }
        R->SetNumberField(TEXT("fixed_min_gap_cm"),Bag.FixedMinGap);R->SetNumberField(TEXT("fixed_max_gap_cm"),Bag.FixedMaxGap);
        R->SetBoolField(TEXT("pose_fit_attempted"),Bag.FitAttempted);R->SetBoolField(TEXT("pose_fit_applied"),Bag.FitApplied);
        R->SetNumberField(TEXT("pitch_delta_deg"),Bag.PitchDelta);R->SetNumberField(TEXT("roll_delta_deg"),Bag.RollDelta);
        R->SetNumberField(TEXT("changed_combined_angle_deg"),Bag.ChangedAngle);R->SetNumberField(TEXT("fit_poses_tested"),Bag.FitPosesTested);
        R->SetArrayField(TEXT("fixed_pose_actual_feet"),FootRows(Bag.FixedFeet));R->SetArrayField(TEXT("chosen_pose_actual_feet"),FootRows(Bag.Feet));
        FitAttempts+=Bag.FitAttempted;FitApplied+=Bag.FitApplied;FitPoses+=Bag.FitPosesTested;
        MaxChangedAngle=FMath::Max(MaxChangedAngle,Bag.ChangedAngle);
        Contacts.Add(MakeShared<FJsonValueObject>(R));
    }
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);const float Wet=Mission?Mission->Wetness:.58f,Snow=Mission&&Mission->Id==10?.44f:0;
    const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,Mission?Mission->Id:0);
    for(const auto& G:S.Groups)if(auto* C=G.Component.Get())
    {
        Authority&=C->GetInstanceCount()==G.Transforms.Num()&&C->GetCollisionEnabled()==G.Collision;
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("mesh"),GetPathNameSafe(C->GetStaticMesh()));R->SetNumberField(TEXT("instances"),C->GetInstanceCount());
        const auto* UV=C->GetStaticMesh()?C->GetStaticMesh()->GetUVChannelData(0):nullptr;
        R->SetBoolField(TEXT("mesh_uv_data_initialized"),UV&&UV->IsInitialized());
        R->SetBoolField(TEXT("mesh_uv_density_manually_overridden"),UV&&UV->bOverrideDensities);
        TArray<TSharedPtr<FJsonValue>> Densities;for(int32 I=0;I<MAX_TEXCOORDS;++I)Densities.Add(MakeShared<FJsonValueNumber>(UV?UV->LocalUVDensities[I]:0));
        R->SetArrayField(TEXT("mesh_world_cm_per_uv_channel"),Densities);
        R->SetNumberField(TEXT("streaming_distance_multiplier"),C->StreamingDistanceMultiplier);R->SetNumberField(TEXT("component_mobility"),int32(C->Mobility));
        R->SetObjectField(TEXT("actual_material_sampling"),MaterialSampling(C->GetMaterial(0),UV));
        FStreamingTextureLevelContext Context(EMaterialQualityLevel::High,B->GetWorld()->GetFeatureLevel(),false);
        TArray<FStreamingRenderAssetPrimitiveInfo> Infos;C->GetStreamingRenderAssetInfo(Context,Infos);
        TArray<TSharedPtr<FJsonValue>> Records;for(const auto& Info:Infos)
        {
            auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("asset"),GetPathNameSafe(Info.RenderAsset));
            E->SetNumberField(TEXT("texel_factor"),Info.TexelFactor);E->SetStringField(TEXT("bounds"),Info.Bounds.ToString());
            E->SetBoolField(TEXT("can_stream_by_distance"),Info.CanBeStreamedByDistance(true));Records.Add(MakeShared<FJsonValueObject>(E));
        }
        R->SetArrayField(TEXT("actual_component_streaming_records"),Records);
        if(S.Switched)
        {
            const int32 Expected=S.MatchedNormals?4:1;bool GroupWeather=C->GetNumMaterials()==Expected;
            TArray<TSharedPtr<FJsonValue>> Slots;
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
            {
                float ActualWet=0,ActualSnow=0;FLinearColor Soil=FLinearColor::Black;auto* Mat=C->GetMaterial(Slot);
                const bool Params=Mat&&Mat->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),ActualWet)
                    &&Mat->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SnowCoverage")),ActualSnow)
                    &&Mat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Soil);
                const bool Matches=Params&&FMath::IsNearlyEqual(ActualWet,Wet)&&FMath::IsNearlyEqual(ActualSnow,Snow)&&Soil.Equals(Palette.Soil,.0001f);
                GroupWeather&=Matches;auto E=MakeShared<FJsonObject>();E->SetNumberField(TEXT("slot"),Slot);
                E->SetStringField(TEXT("material"),GetPathNameSafe(Mat));E->SetNumberField(TEXT("weather_wetness"),ActualWet);
                E->SetNumberField(TEXT("snow_coverage"),ActualSnow);E->SetObjectField(TEXT("soil_tint_rgb"),Point(FVector(Soil.R,Soil.G,Soil.B)));
                E->SetBoolField(TEXT("actual_mid_parameters_match_mission"),Matches);
                E->SetObjectField(TEXT("actual_material_sampling"),MaterialSampling(Mat,C->GetStaticMesh()->GetUVChannelData(Slot)));
                Slots.Add(MakeShared<FJsonValueObject>(E));
                if(Slot==0){R->SetStringField(TEXT("actual_material"),GetPathNameSafe(Mat));R->SetNumberField(TEXT("weather_wetness"),ActualWet);R->SetNumberField(TEXT("snow_coverage"),ActualSnow);}
            }
            Weather&=GroupWeather;R->SetArrayField(TEXT("actual_material_slots"),Slots);R->SetBoolField(TEXT("actual_mid_parameters_match_mission"),GroupWeather);
        }
        Groups.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("groups_inventory"),Groups);J->SetArrayField(TEXT("actual_candidate_contacts"),Contacts);J->SetNumberField(TEXT("upper_bags"),Upper);
    J->SetBoolField(TEXT("collision_and_instance_authority_unchanged"),Authority);J->SetBoolField(TEXT("weather_parameters_match"),Weather);
    auto PoseFit=MakeShared<FJsonObject>();PoseFit->SetBoolField(TEXT("explicit_private_fit_flag"),FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoContactFit")));
    PoseFit->SetBoolField(TEXT("pose_fit_enabled_for_descriptor"),!S.Smart03&&FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoContactFit")));
    PoseFit->SetNumberField(TEXT("bags_attempted"),FitAttempts);PoseFit->SetNumberField(TEXT("bags_fit_applied"),FitApplied);
    PoseFit->SetNumberField(TEXT("actual_poses_tested"),FitPoses);PoseFit->SetNumberField(TEXT("max_changed_combined_angle_deg"),MaxChangedAngle);
    PoseFit->SetNumberField(TEXT("max_per_axis_delta_deg"),16);PoseFit->SetNumberField(TEXT("pitch_roll_grid_step_deg"),1);
    PoseFit->SetStringField(TEXT("selection"),TEXT("Smallest quaternion angular change among the bounded 1-degree pitch/roll grid, original yaw/XY/scale; unchanged 2.5/4.5cm embed and 6cm gap."));
    J->SetObjectField(TEXT("private_contact_pose_fit"),PoseFit);
    if(S.Candidate.IsValid())
    {
        J->SetNumberField(TEXT("actual_source_description_triangles"),S.Triangles.Num());
        TArray<TSharedPtr<FJsonValue>> Lods;
        if(const auto* Data=S.Candidate->GetRenderData())for(int32 I=0;I<Data->LODResources.Num();++I)
        {
            const auto& L=Data->LODResources[I];const auto& VB=L.VertexBuffers.StaticMeshVertexBuffer;
            auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("lod"),I);R->SetNumberField(TEXT("render_triangles"),L.GetNumTriangles());R->SetNumberField(TEXT("render_vertices"),VB.GetNumVertices());
            const bool CPU=VB.GetTangentData()!=nullptr&&VB.GetTexCoordData()!=nullptr;
            R->SetBoolField(TEXT("tangent_cpu_data_available"),CPU);R->SetBoolField(TEXT("cpu_access_forced"),false);
            if(CPU)
            {
                int32 BadX=0,BadY=0;for(uint32 V=0;V<VB.GetNumVertices();++V)
                {const auto X=VB.VertexTangentX(V);BadX+=FVector3f(X.X,X.Y,X.Z).IsNearlyZero(1.e-4f);BadY+=VB.VertexTangentY(V).IsNearlyZero(1.e-4f);}
                R->SetNumberField(TEXT("nearly_zero_tangent_vertices"),BadX);R->SetNumberField(TEXT("nearly_zero_binormal_vertices"),BadY);
            }
            Lods.Add(MakeShared<FJsonValueObject>(R));
        }
        J->SetArrayField(TEXT("actual_lod_tangent_diagnostics"),Lods);
    }
    if(S.Switched&&S.Material.IsValid())
    {
        J->SetObjectField(TEXT("candidate_asset_material_sampling"),MaterialSampling(S.Material.Get(),S.Candidate.IsValid()?S.Candidate->GetUVChannelData(0):nullptr));
        const TArray<UTexture*> Used=CandidateTextures(S);
        TArray<TSharedPtr<FJsonValue>> Textures;
        for(auto* Texture:Used)if(auto* T=Cast<UTexture2D>(Texture))
        {
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("asset"),T->GetPathName());R->SetBoolField(TEXT("async_compiling_or_default_texture"),T->IsCompiling());R->SetNumberField(TEXT("platform_width"),T->GetSizeX());R->SetNumberField(TEXT("platform_height"),T->GetSizeY());
            R->SetNumberField(TEXT("expected_source_width"),ExpectedTextureEdge(S,T));R->SetNumberField(TEXT("expected_source_height"),ExpectedTextureEdge(S,T));
#if WITH_EDITORONLY_DATA
            R->SetNumberField(TEXT("source_width"),T->Source.GetSizeX());R->SetNumberField(TEXT("source_height"),T->Source.GetSizeY());
#endif
            const int32 Total=T->GetNumMips(),Resident=T->GetNumResidentMips();R->SetNumberField(TEXT("platform_mips"),Total);R->SetNumberField(TEXT("resident_mips"),Resident);
            const auto& State=T->GetStreamableResourceState();
            R->SetBoolField(TEXT("render_resource_supports_streaming"),T->RenderResourceSupportsStreaming());
            R->SetBoolField(TEXT("registered_streamable"),T->IsStreamable());R->SetNumberField(TEXT("streaming_index"),T->GetStreamingIndex());
            R->SetNumberField(TEXT("non_streaming_mip_floor"),State.NumNonStreamingLODs);
            R->SetNumberField(TEXT("requested_mips"),State.NumRequestedLODs);
#if !UE_BUILD_SHIPPING
            R->SetNumberField(TEXT("streamer_last_wanted_mips"),T->GetCachedNumWantedLODs());
#endif
            const int32 Shift=FMath::Clamp(Total-Resident,0,30);R->SetNumberField(TEXT("highest_resident_width_estimate"),FMath::Max(1,T->GetSizeX()>>Shift));R->SetNumberField(TEXT("highest_resident_height_estimate"),FMath::Max(1,T->GetSizeY()>>Shift));
            R->SetStringField(TEXT("scope"),TEXT("Streaming-state estimate; no renderer RHI size readback claim."));Textures.Add(MakeShared<FJsonValueObject>(R));
        }
        J->SetArrayField(TEXT("actual_texture_streaming_state"),Textures);
    }
    J->SetBoolField(TEXT("success"),Authority&&Weather);return J;
}
}

namespace FrontlineTripoPropsV87
{
bool Allowed(const AWarfareFrontlineGameMode* B)
{
    FString User;return B&&B->GetWorld()&&B->HasActorBegunPlay()&&B->Phase==EFrontlinePhase::Fighting&&B->AssetFailure.IsEmpty()
        &&B->IsProgressionVolatile()&&(B->GetPlayerFaction()==EFrontlineFaction::British||B->GetPlayerFaction()==EFrontlineFaction::German)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))&&FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoProps"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(User)
        &&!FrontlineUltimateArchiveV8215::Active(B)&&!WarfareFRRUCampaignV82::Active(B);
}
TSharedRef<FJsonObject> Prepare(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Gate=Allowed(B);Check(TEXT("v87_tripo_props_private_disposable_gate"),Gate);if(!Gate)return Result(false,TEXT("Private scene review gate refused; no candidate asset loaded."));
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoReview04")))
    {Check(TEXT("v87_tripo_review04_bake_chain_not_ready_no_asset_binding"),false);return Result(false,TEXT("Review04 bake stopped at negative-Z diagnosis; no empty candidate assets loaded or bound."));}
    Restore(B);auto S=MakeShared<FState>();S->MatchedNormals=FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoReview04"));
    S->Smart03=FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoSmart03"));
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        TArray<UInstancedStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)
        {
            if(!C->ComponentHasTag(TEXT("WF_Environment_V42"))||!C->GetStaticMesh())continue;
            const FString Path=C->GetStaticMesh()->GetPathName();int32 Variant=INDEX_NONE;
            for(int32 I=0;I<6;++I)if(Path.Contains(FString::Printf(TEXT("/V84412/Props/Meshes/SM_Sandbag_%c_V84412."),TCHAR('A'+I))))Variant=I;
            if(Variant==INDEX_NONE)continue;
            FGroup G;G.Component=C;G.Original.Reset(C->GetStaticMesh());G.Collision=C->GetCollisionEnabled();
            for(int32 M=0;M<C->GetNumMaterials();++M)G.Materials.Emplace(C->GetMaterial(M));
            for(int32 I=0;I<C->GetInstanceCount();++I){FTransform T;if(!C->GetInstanceTransform(I,T,true)||T.ContainsNaN())return Result(false,TEXT("Actual baseline instance transform unavailable."));G.Transforms.Add(T);}
            S->Groups.Add(MoveTemp(G));
        }
    }
    const bool Groups=S->Groups.Num()==6;Check(TEXT("v87_tripo_props_actual_six_existing_sandbag_groups"),Groups);if(!Groups)return Result(false,TEXT("Expected actual six V84412 sandbag groups, no gallery/stand-in accepted."));
    double Score=MAX_dbl;for(int32 G=0;G<S->Groups.Num();++G)for(int32 I=0;I<S->Groups[G].Transforms.Num();++I)
    {
        FPlaced P;P.Group=G;P.Instance=I;P.Original=P.Candidate=P.FixedCandidate=S->Groups[G].Transforms[I];S->Bags.Add(P);
        const FVector V=P.Original.GetLocation();const double Test=FMath::Abs(V.X)+.05*FMath::Abs(V.Y);if(Test<Score){Score=Test;S->Focus=V+FVector(0,0,30);}
    }
    bool NoCollision=true;for(const auto& G:S->Groups)NoCollision&=G.Collision==ECollisionEnabled::NoCollision;
    Check(TEXT("v87_tripo_props_existing_visual_groups_have_no_gameplay_collision"),NoCollision);
    const bool Count=!S->Bags.IsEmpty()&&S->Bags.Num()<=150;Check(TEXT("v87_tripo_props_actual_field_barriers_bounded"),Count);if(!Count||!NoCollision)return Result(false,TEXT("Actual field bag count or existing visual-only collision boundary unavailable."));
    for(int32 I=0;I<S->Bags.Num();++I)
    {
        auto& Bag=S->Bags[I];double Best=MAX_dbl;
        for(int32 J=0;J<S->Bags.Num();++J)if(I!=J)
        {
            const FVector A=Bag.Original.GetLocation(),Lower=S->Bags[J].Original.GetLocation();const double XY=FVector::DistSquared2D(A,Lower);
            // Original PlaceBag's upper centre is its bearing centre plus only
            // the five documented 0/+/-5cm attempts. No nearest distant wall is inferred.
            if(Lower.Z<A.Z-5.&&XY<=25.1&&XY<Best){Best=XY;Bag.Bearing=J;}
        }
    }
    States.Add(B,S);auto J=Diagnostics(*S,B);J->SetStringField(TEXT("baseline"),TEXT("Existing live field wall transforms/materials; no candidate loaded."));return J;
}
TSharedRef<FJsonObject> SwitchCandidate(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!Allowed(B))return Result(false,TEXT("Private review gate refused."));auto* Slot=States.Find(B);if(!Slot||!Slot->IsValid())return Result(false,TEXT("Prepare must snapshot the actual field first."));
    auto& S=**Slot;if(S.Switched)return Diagnostics(S,B);
    S.Candidate.Reset(LoadObject<UStaticMesh>(nullptr,S.Smart03?SmartMeshPath:S.MatchedNormals?MatchedMeshPath:MeshPath));S.CandidateMaterials.Reset();
    const int32 MaterialCount=S.MatchedNormals?4:1;bool Assets=S.Candidate.IsValid();
    for(int32 MaterialIndex=0;MaterialIndex<MaterialCount;++MaterialIndex)
    {
        const FString Path=S.Smart03?FString(SmartMaterialPath):S.MatchedNormals?FString(MatchedPackage)+FString::Printf(TEXT("/Materials/M_Tripo_worn_sandbag_h31_85eb6262_review04_%d.M_Tripo_worn_sandbag_h31_85eb6262_review04_%d"),MaterialIndex,MaterialIndex):FString(MaterialPath);
        auto* M=LoadObject<UMaterialInterface>(nullptr,*Path);Assets&=M!=nullptr;S.CandidateMaterials.Emplace(M);
    }
    S.Material.Reset(S.CandidateMaterials[0].Get());Assets&=S.Candidate.IsValid()&&S.Candidate->GetStaticMaterials().Num()==MaterialCount;
    if(S.Smart03&&Assets)Assets&=S.Candidate->GetStaticMaterials()[0].MaterialSlotName==FName(TEXT("Tripo_0_tripo_node_f24443dc_a04b_46da_8b4a_327e9222c208_material"))&&S.Candidate->GetMaterial(0)==S.Material.Get();
    Check(S.Smart03?TEXT("v87_tripo_props_exact_actual_smart03_import01_mesh_material_slot_loaded"):S.MatchedNormals?TEXT("v87_tripo_props_exact_review04_four_lod_material_assets_loaded"):TEXT("v87_tripo_props_exact_actual_import05_assets_loaded"),Assets);
    if(!Assets)return Result(false,TEXT("Exact actual imported candidate/declared material slots missing."));
    if(S.Smart03)
    {
        const auto Textures=CandidateTextures(S);bool Sources=Textures.Num()==3;
        for(auto* Texture:Textures)
        {
            const auto* T=Cast<UTexture2D>(Texture);const int32 Edge=ExpectedTextureEdge(S,T);
            Sources&=T&&Edge>0;
#if WITH_EDITORONLY_DATA
            if(T)Sources&=T->Source.GetSizeX()==Edge&&T->Source.GetSizeY()==Edge;
#endif
        }
        Check(TEXT("v87_tripo_smart03_actual_source_color4k_normal2k_masks2k"),Sources);
        if(!Sources)return Result(false,TEXT("Exact Smart Mesh received texture paths/source dimensions differ; no visual binding."));
    }
    const bool Geometry=ReadCandidateGeometry(S);Check(TEXT("v87_tripo_props_actual_ue_lod0_triangle_bottom_and_top_basis"),Geometry);if(!Geometry)return Result(false,TEXT("Actual Editor mesh-description triangles/9 bottom rays unavailable; no fabricated contact proxy."));
    AWarfareFrontlineGround* Ground=nullptr;for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(It->Surface){Ground=*It;break;}
    const bool Contact=Ground&&FrontlineGroundContactV8449::Prepare(Ground);Check(TEXT("v87_tripo_props_existing_indexed_ground_receiver"),Contact);if(!Contact)return Result(false,TEXT("Actual indexed terrain contact unavailable."));
    TArray<int32> Order;for(int32 I=0;I<S.Bags.Num();++I)Order.Add(I);Order.Sort([&](int32 A,int32 C){return S.Bags[A].Original.GetLocation().Z<S.Bags[C].Original.GetLocation().Z;});
    const bool ContactFit=!S.Smart03&&FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoContactFit"));
    TSet<int32> Seated;bool Fits=true;int32 Failures=0;
    for(int32 Index:Order)
    {
        auto& Bag=S.Bags[Index];const bool Upper=Bag.Bearing!=INDEX_NONE;const double Embed=Upper?4.5:2.5;
        auto FitPose=[&](const FQuat& Q,FTransform& T,TArray<FFootSample>& Feet,double& MinGap,double& MaxGap,double& ContactError,int32& LowerHits)
        {
            T=Bag.Original;T.SetRotation(Q);FVector Location=T.GetLocation();Location.Z=0;T.SetLocation(Location);
            double Base=-MAX_dbl;Feet.Reset();LowerHits=0;MinGap=MAX_dbl;MaxGap=-MAX_dbl;ContactError=MAX_dbl;
            for(const auto& Foot:S.Feet)
            {
                FFootSample F;F.World=T.TransformPosition(Foot);
                if(!GroundHeight(B,Ground,F.World.X,F.World.Y,F.GroundZ))return false;
                F.SupportZ=F.GroundZ;
                if(Upper&&Seated.Contains(Bag.Bearing))
                {
                    F.HasLowerTop=Surface(S.Triangles,S.Bags[Bag.Bearing].Candidate,F.World.X,F.World.Y,true,F.LowerTopZ);
                    if(F.HasLowerTop){F.SupportZ=FMath::Max(F.SupportZ,F.LowerTopZ);++LowerHits;}
                }
                else if(Upper)return false;
                Feet.Add(F);Base=FMath::Max(Base,F.SupportZ-F.World.Z);
            }
            if(Feet.Num()!=9)return false;
            Location.Z=Base-Embed;T.SetLocation(Location);
            for(int32 I=0;I<Feet.Num();++I)
            {
                auto& F=Feet[I];F.World=T.TransformPosition(S.Feet[I]);F.Gap=F.World.Z-F.SupportZ;
                MinGap=FMath::Min(MinGap,F.Gap);MaxGap=FMath::Max(MaxGap,F.Gap);
                ContactError=FMath::Min(ContactError,FMath::Abs(F.Gap+Embed));
            }
            return MinGap>=-(Embed+.08)&&MaxGap<=6.&&ContactError<=.08;
        };
        bool Good=FitPose(Bag.Original.GetRotation(),Bag.Candidate,Bag.Feet,Bag.MinGap,Bag.MaxGap,Bag.ContactError,Bag.LowerSurfaceHits);
        Bag.FixedCandidate=Bag.Candidate;Bag.FixedFeet=Bag.Feet;Bag.FixedMinGap=Bag.MinGap;Bag.FixedMaxGap=Bag.MaxGap;
        // A pose search is never a substitute for missing ground or bearing data.
        // The original pose must fail ONLY the high gap, with all nine real supports.
        if(!Good&&ContactFit&&Bag.MaxGap>6.&&Bag.Feet.Num()==9&&(!Upper||Bag.LowerSurfaceHits==9)
            &&Bag.MinGap>=-(Embed+.08)&&Bag.ContactError<=.08)
        {
            Bag.FitAttempted=true;
            struct FPose {FQuat Q;double Pitch=0,Roll=0,Angle=0;};TArray<FPose> Poses;
            const FRotator Original=Bag.Original.GetRotation().Rotator();
            for(int32 Pitch=-16;Pitch<=16;++Pitch)for(int32 Roll=-16;Roll<=16;++Roll)
            {
                if(Pitch==0&&Roll==0)continue;
                FPose P;P.Pitch=Pitch;P.Roll=Roll;P.Q=FRotator(Original.Pitch+Pitch,Original.Yaw,Original.Roll+Roll).Quaternion();
                P.Angle=FMath::RadiansToDegrees(Bag.Original.GetRotation().AngularDistance(P.Q));Poses.Add(P);
            }
            Poses.Sort([](const FPose& A,const FPose& C)
            {if(A.Angle!=C.Angle)return A.Angle<C.Angle;if(A.Pitch!=C.Pitch)return A.Pitch<C.Pitch;return A.Roll<C.Roll;});
            for(const auto& P:Poses)
            {
                ++Bag.FitPosesTested;FTransform Candidate;TArray<FFootSample> Feet;double Low=0,High=0,Error=0;int32 Hits=0;
                const bool Fit=FitPose(P.Q,Candidate,Feet,Low,High,Error,Hits);
                if(!Fit||(Upper&&Hits!=9))continue;
                Bag.Candidate=Candidate;Bag.Feet=MoveTemp(Feet);Bag.MinGap=Low;Bag.MaxGap=High;Bag.ContactError=Error;Bag.LowerSurfaceHits=Hits;
                Bag.PitchDelta=P.Pitch;Bag.RollDelta=P.Roll;Bag.ChangedAngle=P.Angle;Bag.FitApplied=true;Good=true;break;
            }
        }
        Fits&=Good;Failures+=!Good;if(Good)Seated.Add(Index);
    }
    Check(TEXT("v87_tripo_props_real_candidate_contacts_retain_existing_6cm_support_contract"),Fits);
    if(!Fits){auto J=Diagnostics(S,B);J->SetBoolField(TEXT("success"),false);J->SetNumberField(TEXT("contact_failures"),Failures);J->SetStringField(TEXT("error"),TEXT("Narrow candidate cannot satisfy actual original XY/rotation/scale supports; no visual change applied."));return J;}
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,Mission?Mission->Id:0);
    for(auto& G:S.Groups)if(auto* C=G.Component.Get())
    {
        C->SetStaticMesh(S.Candidate.Get());C->EmptyOverrideMaterials();
        for(int32 MaterialIndex=0;MaterialIndex<S.CandidateMaterials.Num();++MaterialIndex)
        {
            auto* MID=UMaterialInstanceDynamic::Create(S.CandidateMaterials[MaterialIndex].Get(),B);
            if(!MID){Restore(B);return Result(false,TEXT("Candidate MID creation failed; baseline restored."));}
            MID->SetScalarParameterValue(TEXT("WeatherWetness"),Mission?Mission->Wetness:.58f);
            MID->SetScalarParameterValue(TEXT("SnowCoverage"),Mission&&Mission->Id==10?.44f:0.f);
            MID->SetVectorParameterValue(TEXT("SoilTint"),Palette.Soil);C->SetMaterial(MaterialIndex,MID);
        }
    }
    auto RotationAllowed=[&](const FPlaced& Bag,const FQuat& Actual)
    {
        if(!Bag.FitApplied)return Bag.Original.GetRotation().Equals(Actual,.00001);
        const auto Before=Bag.Original.GetRotation().Rotator(),After=Actual.Rotator();
        const double Pitch=FMath::FindDeltaAngleDegrees(Before.Pitch,After.Pitch),Roll=FMath::FindDeltaAngleDegrees(Before.Roll,After.Roll);
        const double Yaw=FMath::FindDeltaAngleDegrees(Before.Yaw,After.Yaw);
        const double Angle=FMath::RadiansToDegrees(Bag.Original.GetRotation().AngularDistance(Actual));
        return ContactFit&&Bag.FitAttempted&&FMath::Abs(Pitch)<=16.0001&&FMath::Abs(Roll)<=16.0001&&FMath::Abs(Yaw)<=.0001
            &&FMath::IsNearlyEqual(Pitch,Bag.PitchDelta,.0001)&&FMath::IsNearlyEqual(Roll,Bag.RollDelta,.0001)
            &&FMath::IsFinite(Angle)&&Angle<=32.0001&&FMath::IsNearlyEqual(Angle,Bag.ChangedAngle,.0001);
    };
    bool Transforms=true;for(const auto& Bag:S.Bags)if(auto* C=S.Groups[Bag.Group].Component.Get())
    {
        Transforms&=C->UpdateInstanceTransform(Bag.Instance,Bag.Candidate,true,false,true);
        Transforms&=RotationAllowed(Bag,Bag.Candidate.GetRotation())&&Bag.Original.GetScale3D().Equals(Bag.Candidate.GetScale3D(),1.e-8)
            &&FVector::DistSquared2D(Bag.Original.GetLocation(),Bag.Candidate.GetLocation())<1.e-12;
        FTransform Actual;Transforms&=C->GetInstanceTransform(Bag.Instance,Actual,true);
        Transforms&=!Actual.ContainsNaN()&&FVector::DistSquared2D(Actual.GetLocation(),Bag.Original.GetLocation())<.0001
            &&RotationAllowed(Bag,Actual.GetRotation())&&Actual.GetRotation().Equals(Bag.Candidate.GetRotation(),.00001)
            &&Actual.GetScale3D().Equals(Bag.Original.GetScale3D(),.00001);
    }
    for(auto& G:S.Groups)if(auto* C=G.Component.Get())C->MarkRenderStateDirty();S.Switched=true;
    Check(ContactFit?TEXT("v87_tripo_props_world_xy_yaw_scale_counts_and_bounded_pose_fit_retained"):
        TEXT("v87_tripo_props_world_xy_rotation_scale_and_instance_counts_retained"),Transforms);
    auto J=Diagnostics(S,B);J->SetBoolField(TEXT("success"),Transforms&&J->GetBoolField(TEXT("success")));if(!J->GetBoolField(TEXT("success")))Restore(B);return J;
}
bool Focus(const AWarfareFrontlineGameMode* B,FVector& Out)
{const auto* S=States.Find(const_cast<AWarfareFrontlineGameMode*>(B));if(!S||!S->IsValid())return false;Out=(*S)->Focus;return !Out.ContainsNaN();}
TSharedRef<FJsonObject> DescribeCurrent(const AWarfareFrontlineGameMode* B)
{
    const auto* S=States.Find(const_cast<AWarfareFrontlineGameMode*>(B));
    if(!S||!S->IsValid())return Result(false,TEXT("No prepared private scene state; no candidate asset loaded."));
    return Diagnostics(**S,const_cast<AWarfareFrontlineGameMode*>(B));
}
TSharedRef<FJsonObject> EnsureTextureReady(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Gate=Allowed(B)&&IsInGameThread()&&FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoTextureWait"));
    Check(TEXT("v87_tripo_texture_wait_explicit_private_actual_game_gate"),Gate);
    if(!Gate)return Result(false,TEXT("Private candidate-only texture wait gate refused."));
    auto* Slot=States.Find(B);if(!Slot||!Slot->IsValid()||!(**Slot).Switched||!(**Slot).Material.IsValid())
        return Result(false,TEXT("Actual installed candidate required; no asset loaded by texture-ready query."));
    auto& S=**Slot;auto J=Result(false,TEXT("One explicit candidate-only async texture completion; no asset save, global toggle or scene transform mutation."));
    J->SetObjectField(TEXT("before"),Diagnostics(S,B));
    const TArray<UTexture*> Used=CandidateTextures(S);
    TArray<UTexture*> Pending;bool Exact=!Used.IsEmpty();
    for(auto* Texture:Used)
    {
        const auto* T=Cast<UTexture2D>(Texture);
        Exact&=T&&T->GetPathName().StartsWith(CandidateTexturePrefix(S));
        if(T)Pending.AddUnique(Texture);
    }
    Exact&=Pending.Num()==(S.MatchedNormals?6:3);
    Check(S.MatchedNormals?TEXT("v87_tripo_texture_wait_only_six_matched_lod_used_textures"):TEXT("v87_tripo_texture_wait_only_three_actual_candidate_used_textures"),Exact);
    if(!Exact){J->SetStringField(TEXT("error"),TEXT("Used texture closure differs from this exact actual import; nothing waited."));return J;}
    const double Started=FPlatformTime::Seconds();
    // Public UE5.8 API, normal review game-thread tick (not a PostCompilation callback).
    // FinishCompilation itself waits and routes PostCompilation only for this list.
    FTextureCompilingManager::Get().FinishCompilation(Pending);
    J->SetNumberField(TEXT("targeted_finish_seconds"),FPlatformTime::Seconds()-Started);
    bool Ready=true;for(auto* Texture:Pending)
    {
        const auto* T=CastChecked<UTexture2D>(Texture);const int32 Edge=ExpectedTextureEdge(S,T);
        Ready&=Edge>0&&!T->IsCompiling()&&T->GetSizeX()==Edge&&T->GetSizeY()==Edge;
    }
    J->SetObjectField(TEXT("after"),Diagnostics(S,B));J->SetNumberField(TEXT("waited_textures"),Pending.Num());
    J->SetBoolField(TEXT("global_async_disabled"),false);J->SetBoolField(TEXT("source_asset_saved"),false);
    J->SetBoolField(TEXT("transforms_changed"),false);J->SetBoolField(S.Smart03?TEXT("platform_matches_actual_descriptor_and_not_compiling"):TEXT("platform_4k_and_not_compiling"),Ready);
    Check(S.Smart03?TEXT("v87_tripo_texture_wait_actual_smart03_color4k_normal2k_masks2k_no_default_stub"):S.MatchedNormals?TEXT("v87_tripo_texture_wait_actual_six_platform_4k_no_default_stub"):TEXT("v87_tripo_texture_wait_actual_three_platform_4k_no_default_stub"),Ready);J->SetBoolField(TEXT("success"),Ready);
    return J;
}
TSharedRef<FJsonObject> RepairCandidateSampling(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Gate=Allowed(B)&&IsInGameThread()&&FParse::Param(FCommandLine::Get(),TEXT("WFV87TripoSamplingFix"));
    Check(TEXT("v87_tripo_sampling_fix_explicit_private_gate"),Gate);
    if(!Gate)return Result(false,TEXT("Private candidate sampling repair gate refused."));
    auto* Slot=States.Find(B);if(!Slot||!Slot->IsValid()||!(**Slot).Switched||!(**Slot).Material.IsValid())
        return Result(false,TEXT("Actual installed candidate required; no asset loaded by sampling repair."));
    auto& S=**Slot;auto J=Result(false,TEXT("Candidate-only transient UV0 atlas sampling metadata; normal budgeted distance streaming retained."));
    if(S.MatchedNormals)return Result(false,TEXT("Legacy three-texture sampling repair is not the review04 descriptor; inspect actual per-slot metadata instead."));
    J->SetObjectField(TEXT("before"),Diagnostics(S,B));J->SetBoolField(TEXT("fix_applied"),false);J->SetBoolField(TEXT("no_fix_needed"),false);
    TArray<UTexture*> Used;S.Material->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(EMaterialQualityLevel::High));
    TArray<UTexture2D*> ExactTextures;bool Exact=true;
    for(auto* Texture:Used)
    {
        auto* T=Cast<UTexture2D>(Texture);
        Exact&=T&&T->GetPathName().StartsWith(CandidateTexturePrefix(S));
        if(T)ExactTextures.AddUnique(T);
    }
    Exact&=ExactTextures.Num()==3;Check(TEXT("v87_tripo_sampling_fix_exact_three_candidate_atlas_textures"),Exact);
    if(!Exact){J->SetStringField(TEXT("error"),TEXT("Actual used texture closure differs from exact import05; no change applied."));return J;}
    bool PositiveUV=S.Groups.Num()==6,Missing=false,SameAssets=true;
    TArray<UMaterialInterface*> Assigned;
    for(const auto& G:S.Groups)
    {
        auto* C=G.Component.Get();const auto* UV=C&&C->GetStaticMesh()?C->GetStaticMesh()->GetUVChannelData(0):nullptr;
        PositiveUV&=UV&&UV->IsInitialized()&&FMath::IsFinite(UV->LocalUVDensities[0])&&UV->LocalUVDensities[0]>UE_SMALL_NUMBER;
        auto* M=C?C->GetMaterial(0):nullptr;Assigned.Add(M);
        SameAssets&=C&&C->GetStaticMesh()==S.Candidate.Get()&&M&&M->GetBaseMaterial()==S.Material->GetBaseMaterial();
        TArray<UTexture*> Actual;if(M)M->GetUsedTextures(Actual,TOptional<EMaterialQualityLevel::Type>(EMaterialQualityLevel::High));
        SameAssets&=Actual.Num()==3;for(auto* T:ExactTextures)SameAssets&=Actual.Contains(T);
        for(auto* T:ExactTextures)
        {
            FTextureSamplingInfo Sampling;const bool Valid=M&&M->CalculateTexturesSamplingInfo(T,Sampling);
            const float Density=Valid&&UV?Sampling.GetTextureDensity(*UV):0;
            Missing|=!Valid||!FMath::IsFinite(Density)||Density<=UE_SMALL_NUMBER;
        }
    }
    Check(TEXT("v87_tripo_sampling_fix_actual_positive_uv0_density"),PositiveUV);
    Check(TEXT("v87_tripo_sampling_fix_same_six_mesh_and_shader_texture_bindings"),SameAssets);
    if(!PositiveUV||!SameAssets){J->SetStringField(TEXT("error"),TEXT("Actual UV0 density or six installed bindings unavailable; no invented density or change applied."));return J;}
    J->SetBoolField(TEXT("actual_sampling_missing_or_invalid"),Missing);
    if(!Missing)
    {
        J->SetBoolField(TEXT("no_fix_needed"),true);J->SetObjectField(TEXT("after"),Diagnostics(S,B));J->SetBoolField(TEXT("success"),true);return J;
    }
    TArray<FMaterialTextureInfo> Entries;
    for(auto* Texture:ExactTextures)
    {
        FMaterialTextureInfo E;E.TextureName=Texture->GetFName();E.UVChannelIndex=0;E.SamplingScale=1.f;
#if WITH_EDITORONLY_DATA
        E.TextureReference=FSoftObjectPath(Texture);
#endif
        Entries.Add(E);
    }
    TArray<TStrongObjectPtr<UFrontlineTerrainMaterialV47>> Replacements;
    bool Prepared=true;
    for(auto* M:Assigned)
    {
        auto* MID=UFrontlineTerrainMaterialV47::CreateForGround(M,B);if(!MID){Prepared=false;break;}
        MID->SetWorldSampling(Entries);Replacements.Emplace(MID);
        float BeforeWet=0,BeforeSnow=0,AfterWet=0,AfterSnow=0;FLinearColor BeforeSoil,AfterSoil;
        Prepared&=M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),BeforeWet)
            &&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SnowCoverage")),BeforeSnow)
            &&M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),BeforeSoil)
            &&MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),AfterWet)
            &&MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SnowCoverage")),AfterSnow)
            &&MID->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),AfterSoil)
            &&FMath::IsNearlyEqual(BeforeWet,AfterWet)&&FMath::IsNearlyEqual(BeforeSnow,AfterSnow)&&BeforeSoil.Equals(AfterSoil,.0001f)
            &&MID->GetBaseMaterial()==M->GetBaseMaterial();
    }
    Prepared&=Replacements.Num()==6;Check(TEXT("v87_tripo_sampling_fix_weather_and_base_shader_preserved"),Prepared);
    if(!Prepared){J->SetStringField(TEXT("error"),TEXT("Transient metadata instances failed preserved weather/shader preflight; original bindings retained."));return J;}
    for(int32 I=0;I<S.Groups.Num();++I)if(auto* C=S.Groups[I].Component.Get())
    {
        C->SetMaterial(0,Replacements[I].Get());C->MarkRenderStateDirty();
    }
    bool ValidRecords=true;
    for(const auto& G:S.Groups)if(auto* C=G.Component.Get())
    {
        FStreamingTextureLevelContext Context(EMaterialQualityLevel::High,B->GetWorld()->GetFeatureLevel(),false);
        TArray<FStreamingRenderAssetPrimitiveInfo> Infos;C->GetStreamingRenderAssetInfo(Context,Infos);
        for(auto* Texture:ExactTextures)
        {
            bool Found=false;for(const auto& Info:Infos)if(Info.RenderAsset==Texture)
                Found|=FMath::IsFinite(Info.TexelFactor)&&Info.TexelFactor>UE_SMALL_NUMBER&&Info.CanBeStreamedByDistance(true);
            ValidRecords&=Found;
        }
    }
    Check(TEXT("v87_tripo_sampling_fix_actual_three_positive_streaming_records_per_group"),ValidRecords);
    if(!ValidRecords)
    {
        for(int32 I=0;I<S.Groups.Num();++I)if(auto* C=S.Groups[I].Component.Get())
        {C->SetMaterial(0,Assigned[I]);C->MarkRenderStateDirty();}
        J->SetBoolField(TEXT("repair_rolled_back"),true);
        J->SetStringField(TEXT("error"),TEXT("Actual distance-streaming records did not cover all three textures; candidate's preceding material bindings restored."));
    }
    J->SetBoolField(TEXT("fix_applied"),ValidRecords);J->SetBoolField(TEXT("no_fix_needed"),false);
    J->SetBoolField(TEXT("shader_graph_changed"),false);J->SetBoolField(TEXT("source_asset_saved"),false);
    J->SetBoolField(TEXT("geometry_or_transforms_changed"),false);J->SetBoolField(TEXT("forced_residency"),false);
    J->SetNumberField(TEXT("atlas_uv_channel"),0);J->SetNumberField(TEXT("atlas_sampling_scale"),1);
    J->SetObjectField(TEXT("after"),Diagnostics(S,B));J->SetBoolField(TEXT("success"),ValidRecords);
    return J;
}
void Restore(AWarfareFrontlineGameMode* B)
{
    auto* Slot=States.Find(B);if(!Slot||!Slot->IsValid())return;auto& S=**Slot;
    for(auto& G:S.Groups)if(auto* C=G.Component.Get())
    {
        C->SetStaticMesh(G.Original.Get());C->EmptyOverrideMaterials();for(int32 I=0;I<G.Materials.Num();++I)C->SetMaterial(I,G.Materials[I].Get());
        if(C->GetInstanceCount()==G.Transforms.Num())for(int32 I=0;I<G.Transforms.Num();++I)C->UpdateInstanceTransform(I,G.Transforms[I],true,false,true);
        C->MarkRenderStateDirty();
    }
    S.Switched=false;
}
}
#else
namespace FrontlineTripoPropsV87
{
bool Allowed(const AWarfareFrontlineGameMode*){return false;}
TSharedRef<FJsonObject> Prepare(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);J->SetStringField(TEXT("scope"),TEXT("Private Editor-only review; Shipping never loads candidate."));return J;}
TSharedRef<FJsonObject> SwitchCandidate(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);return J;}
bool Focus(const AWarfareFrontlineGameMode*,FVector&){return false;}
TSharedRef<FJsonObject> DescribeCurrent(const AWarfareFrontlineGameMode*){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);return J;}
TSharedRef<FJsonObject> EnsureTextureReady(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);return J;}
TSharedRef<FJsonObject> RepairCandidateSampling(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)>){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);return J;}
void Restore(AWarfareFrontlineGameMode*){}
}
#endif
