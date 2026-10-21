#include "WarfareFRRUMalmaisonSurfaceV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineTerrainMaterialV47.h"
#include "WarfareFRRUWorldRenderV826.generated.inl"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureStreamingTypes.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Rendering/ColorVertexBuffer.h"
#include "RenderResource.h"
#include "RHI.h"
#include "StaticMeshComponentLODInfo.h"
#include "StaticMeshResources.h"
#include "UnrealEngine.h"

namespace
{
constexpr float NativeUVCm=200.f;
constexpr float SoilProjection=.52f*(NativeUVCm/145.f);
constexpr float DepositProjection=.725f*(NativeUVCm/145.f);
const FName AdapterTag(TEXT("FRRU_V82MalmaisonSurfaceV76"));
const TCHAR* ScenePath=TEXT("fr.malmaison.scene.families.v1");
bool Reject(FString& Error,const TCHAR* Detail)
{
    Error=FString(TEXT("Malmaison surface candidate: "))+Detail;
    return false;
}
bool IsSourceMesh(const UStaticMesh* Mesh)
{
    if(!Mesh)return false;
    return WarfareFRRUWorldRenderV826::Registered(Mesh->GetPathName(),WarfareFRRUWorldRenderV826::Find(ScenePath),0)
        ||Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/SurfaceData/Scenes/SM_WF_fr_malmaison_ground.SM_WF_fr_malmaison_ground")
        ||Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/SurfaceData/Scenes/SM_WF_fr_malmaison_outsideBackdrop.SM_WF_fr_malmaison_outsideBackdrop");
}
FColor Adapt(const FColor& Source)
{
    // WorldFamilies stores linear mineral / moisture / cover. The retained
    // V76 shader decodes sRGB moisture / shoreline / Menin classification.
    // Malmaison has no water surface; neutral B and its own seed disable
    // Menin's operation-specific road/gully branch.
    return FLinearColor(float(Source.G)/255.f,0.f,.5f,1.f).ToFColor(true);
}
FString ColourSHA1(const FColorVertexBuffer& Colours)
{
    if(!Colours.GetVertexData())return FString();
    uint8 Hash[FSHA1::DigestSize];
    FSHA1::HashBuffer(Colours.GetVertexData(),uint64(Colours.GetNumVertices())*Colours.GetStride(),Hash);
    return BytesToHex(Hash,FSHA1::DigestSize);
}
bool SamplingEntries(UMaterialInterface* Material,TArray<FMaterialTextureInfo>& Entries,FString& Error)
{
    TArray<UTexture*> Used;Material->GetUsedTextures(Used,GetCachedScalabilityCVars().MaterialQualityLevel,GMaxRHIShaderPlatform);
    TSet<FName> Names;int32 Mud=0,HeightAO=0,Clay=0;
    for(UTexture* Texture:Used)
    {
        if(!Cast<UTexture2D>(Texture)||Names.Contains(Texture->GetFName()))
            return Reject(Error,TEXT("unexpected or duplicate retained scan"));
        const FString Path=Texture->GetPathName();float Scale=0.f;
        if(Path==TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_Albedo_V23.T_Mud_Albedo_V23")
            ||Path==TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_Normal_V23.T_Mud_Normal_V23")
            ||Path==TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_Roughness_V23.T_Mud_Roughness_V23"))
        {Scale=SoilProjection;++Mud;}
        else if(Path==TEXT("/Game/Warfare/Field/V47/Textures/T_Mud_HeightAO_V47.T_Mud_HeightAO_V47"))
        {Scale=SoilProjection;++HeightAO;}
        else if(Path==TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_Albedo_V42.T_Clay_Albedo_V42")
            ||Path==TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_Normal_V42.T_Clay_Normal_V42")
            ||Path==TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_Roughness_V42.T_Clay_Roughness_V42")
            ||Path==TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_Height_V42.T_Clay_Height_V42"))
        {Scale=DepositProjection;++Clay;}
        else return Reject(Error,TEXT("retained texture has no measured UV sampling contract"));
        FMaterialTextureInfo Entry;Entry.TextureName=Texture->GetFName();Entry.UVChannelIndex=0;Entry.SamplingScale=Scale;
#if WITH_EDITORONLY_DATA
        Entry.TextureReference=FSoftObjectPath(Texture);
#endif
        Entries.Add(Entry);Names.Add(Texture->GetFName());
    }
    return (Mud==3&&HeightAO==1&&Clay==4&&Entries.Num()==8)
        ||Reject(Error,TEXT("retained paired soil scan set changed"));
}
}

namespace WarfareFRRUMalmaisonSurfaceV82
{
bool ReviewEnabled(const FString& SceneId)
{
    // Fixed-camera review 63bcf986 accepted this scene's paired V76 surface.
    // The adapter remains scene-specific; it does not approve other terrain.
    constexpr bool AcceptedMalmaisonSurface=true;
    if(SceneId!=ScenePath)return false;
    if(AcceptedMalmaisonSurface)return true;
#if WITH_EDITOR
    static const bool Allowed=[]()
    {
        FString User,Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFMalmaisonSurfaceReviewV82"))
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User))return false;
        FPaths::NormalizeDirectoryName(User);
        return Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
            &&User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase)&&!User.Contains(TEXT(".."));
    }();
    return Allowed;
#else
    return false;
#endif
}
const TCHAR* InstancePath()
{return TEXT("/Game/Warfare/Field/V76/Materials/MI_GroundMud_V76.MI_GroundMud_V76");}
const TCHAR* BasePath()
{return TEXT("/Game/Warfare/Field/V76/Materials/M_GroundMud_V76.M_GroundMud_V76");}

UFrontlineTerrainMaterialV47* Apply(const FString& SceneId,UStaticMeshComponent* Component,UMaterialInterface* Source,FString& Error)
{
    UStaticMesh* Mesh=Component?Component->GetStaticMesh().Get():nullptr;
    if(!ReviewEnabled(SceneId)||!Component||Component->IsRegistered()||!Component->HasAnyFlags(RF_Transient)
        ||!IsSourceMesh(Mesh)||!Source||Source->GetPathName()!=InstancePath()
        ||!Source->GetMaterial()||Source->GetMaterial()->GetPathName()!=BasePath())
    {Reject(Error,TEXT("wrong review, component, physical source or material parent"));return nullptr;}
    if(Mesh->HasValidNaniteData()||!Mesh->bAllowCPUAccess||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=1
        ||Mesh->GetStaticMaterials().Num()!=1)
    {Reject(Error,TEXT("exact non-Nanite single-LOD source required"));return nullptr;}
    const auto& LOD=Mesh->GetRenderData()->LODResources[0];const auto& Vertex=LOD.VertexBuffers;
    const auto& Colours=Vertex.ColorVertexBuffer;const uint32 Count=Vertex.PositionVertexBuffer.GetNumVertices();
    if(!Count||Colours.GetNumVertices()!=Count||!Colours.GetVertexData()||Colours.GetStride()!=sizeof(FColor)
        ||Vertex.StaticMeshVertexBuffer.GetNumVertices()!=Count||Vertex.StaticMeshVertexBuffer.GetNumTexCoords()<1)
    {Reject(Error,TEXT("complete immutable CPU semantic/UV buffer required"));return nullptr;}
    TArray<FMaterialTextureInfo> Entries;if(!SamplingEntries(Source,Entries,Error))return nullptr;
    TArray<FColor> Adapted;Adapted.Reserve(Count);const FString SourceSHA1=ColourSHA1(Colours);
    for(uint32 I=0;I<Count;++I)
    {
        const FColor& Value=Colours.VertexColor(I);
        const FVector3f Position=Vertex.PositionVertexBuffer.VertexPosition(I);
        const auto UV=Vertex.StaticMeshVertexBuffer.GetVertexUV(I,0);
        if(Value.A!=255||Value.R>141||Value.G<25||Value.G>235||Value.B>184
            ||!FMath::IsFinite(Position.X)||!FMath::IsFinite(Position.Y)
            ||!FMath::IsFinite(Position.Z)||!FMath::IsFinite(UV.X)||!FMath::IsFinite(UV.Y)
            ||FMath::Abs(UV.X-Position.X/NativeUVCm)>.001f
            ||FMath::Abs(UV.Y-(1.f+Position.Y/NativeUVCm))>.001f)
        {Reject(Error,TEXT("linear semantic bytes or native200cm UV convention changed"));return nullptr;}
        Adapted.Add(Adapt(Value));
    }
    Component->SetLODDataCount(1,1);
    if(Component->LODData.Num()!=1||Component->LODData[0].OverrideVertexColors)
    {Reject(Error,TEXT("new component already has a colour override"));return nullptr;}
    auto* MID=UFrontlineTerrainMaterialV47::CreateForGround(Source,Component);
    if(!MID){Reject(Error,TEXT("typed soil sampling instance missing"));return nullptr;}
    MID->SetScalarParameterValue(TEXT("SoilProjectionScaleV47"),SoilProjection);
    MID->SetScalarParameterValue(TEXT("DepositProjectionScaleV51"),DepositProjection);
    MID->SetScalarParameterValue(TEXT("FieldSeed"),1005.f);
    MID->SetWorldSampling(Entries);
    auto& Info=Component->LODData[0];Info.OverrideVertexColors=new FColorVertexBuffer;
    Info.OverrideVertexColors->InitFromColorArray(Adapted);
    if(Info.OverrideVertexColors->GetNumVertices()!=Count
        ||FMemory::Memcmp(Info.OverrideVertexColors->GetVertexData(),Adapted.GetData(),uint64(Count)*sizeof(FColor))!=0
        ||ColourSHA1(Colours)!=SourceSHA1)
    {
        delete Info.OverrideVertexColors;Info.OverrideVertexColors=nullptr;
        Reject(Error,TEXT("component colour copy changed its source or payload"));return nullptr;
    }
    BeginInitResource(Info.OverrideVertexColors);
    Component->ComponentTags.Add(AdapterTag);Component->SetMaterial(0,MID);
    UE_LOG(LogTemp,Display,TEXT("WF_V82_MALMAISON_SURFACE part=%s vertices=%u original_semantic_sha1=%s override_sha1=%s physical_source_unchanged=1"),
        *Component->GetName(),Count,*SourceSHA1,*ColourSHA1(*Info.OverrideVertexColors));
    return MID;
}

bool AppliedTo(const UStaticMeshComponent* Component)
{
    const UStaticMesh* Mesh=Component?Component->GetStaticMesh().Get():nullptr;
    const auto* Material=Component?Cast<UFrontlineTerrainMaterialV47>(Component->GetMaterial(0)):nullptr;
    if(!Component||!Component->ComponentHasTag(AdapterTag)||!IsSourceMesh(Mesh)||Mesh->HasValidNaniteData()
        ||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=1||Component->LODData.Num()!=1
        ||!Material||!Material->GetMaterial()||Material->GetMaterial()->GetPathName()!=BasePath())return false;
    const auto* Override=Component->LODData[0].OverrideVertexColors;const auto& Source=Mesh->GetRenderData()->LODResources[0].VertexBuffers.ColorVertexBuffer;
    float Soil=0.f,Deposit=0.f,Seed=0.f;
    return Override&&Source.GetNumVertices()>0&&Source.GetVertexData()
        &&Override->GetNumVertices()==Source.GetNumVertices()&&Override->GetVertexData()
        &&Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SoilProjectionScaleV47")),Soil)&&Soil==SoilProjection
        &&Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DepositProjectionScaleV51")),Deposit)&&Deposit==DepositProjection
        &&Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FieldSeed")),Seed)&&Seed==1005.f;
}

TSharedPtr<FJsonObject> Describe(const UStaticMeshComponent* Component)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("applied"),AppliedTo(Component));
    Result->SetBoolField(TEXT("artAccepted"),false);
    if(!AppliedTo(Component))return Result;
    const UStaticMesh* Mesh=Component->GetStaticMesh().Get();const auto& Source=Mesh->GetRenderData()->LODResources[0].VertexBuffers.ColorVertexBuffer;
    const auto& Adapted=*Component->LODData[0].OverrideVertexColors;
    Result->SetBoolField(TEXT("actualMeshNaniteEnabled"),Mesh->HasValidNaniteData());
    Result->SetNumberField(TEXT("sourceVertices"),Source.GetNumVertices());Result->SetNumberField(TEXT("overrideVertices"),Adapted.GetNumVertices());
    Result->SetStringField(TEXT("sourceSemanticSHA1"),ColourSHA1(Source));Result->SetStringField(TEXT("componentOverrideSHA1"),ColourSHA1(Adapted));
    Result->SetNumberField(TEXT("nativeUVCm"),NativeUVCm);Result->SetNumberField(TEXT("soilProjection"),SoilProjection);
    Result->SetStringField(TEXT("importedUV0Convention"),TEXT("U=localX/200; V=1+localY/200; imported Y reflection and FBX V flip"));
    Result->SetNumberField(TEXT("depositProjection"),DepositProjection);Result->SetBoolField(TEXT("overrideOwnedByTransientComponent"),Component->HasAnyFlags(RF_Transient));
    TArray<TSharedPtr<FJsonValue>> Samples;const uint32 Count=Source.GetNumVertices();
    for(uint32 I:{0u,Count/4,Count/2,Count*3/4,Count-1})
    {
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("vertex"),I);
        Row->SetStringField(TEXT("sourceLinearRGBABytes"),Source.VertexColor(I).ToString());
        Row->SetStringField(TEXT("adaptedSRGBABytes"),Adapted.VertexColor(I).ToString());
        Row->SetBoolField(TEXT("matchesExactAdapter"),Adapted.VertexColor(I)==Adapt(Source.VertexColor(I)));
        Samples.Add(MakeShared<FJsonValueObject>(Row));
    }
    Result->SetArrayField(TEXT("semanticSamples"),Samples);return Result;
}
}
