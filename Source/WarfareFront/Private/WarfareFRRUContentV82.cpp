#include "WarfareFRRUContentV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "FRRU/FRRUVehicleNavV87.h"
#include "WarfareFRRUAssetAuditV82.h"
#include "FRRU/WarfareFRRUGermanHostAcceptanceV82.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineTerrainProfilesV51.h"
#include "WarfareFrontlineTerrainMaterialV47.h"
#include "WarfareFRRUMalmaisonSurfaceV82.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FRRU/Native/SceneRuntime.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureStreamingTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "UnrealEngine.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MaterialShared.h"
#include "MaterialShaderPrecompileMode.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "FRRU/FRRUVehicleNavCandidateV87.h"
#include "EngineUtils.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "FRRU/FRRUVauxFormalBagRemainderV87.inl"
#endif
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "RHI.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "StaticMeshResources.h"
#include "WarfareFRRUWorldRenderV826.generated.inl"
#include "WarfareFRRUWorldRenderBindingV87.inl"
#include "WarfareFRRUWorldSurfaceV826.generated.inl"
#include "WarfareFRRUVauxSurfaceParityV87.inl"
#include "FRRU/FRRUMortPairedSamplingV87.inl"
#include "WarfareFRRUWorldWoodlineV826.inl"
#include "WarfareFRRUBlastedWoodlineV826.inl"
#include "WarfareFRRUWorldDressingV827.inl"
#include "WarfareFRRUFieldDetailV87.inl"
#include "WarfareFRRUVauxCompositionV87.inl"
#include "Dom/JsonObject.h"
#include "FRRU/FRRUMortFlankFireStepV87.inl"
#include "FRRU/FRRUVauxTripoReleaseLoaderV87.inl"
#include "FRRU/FRRUVauxHQ27VisualCandidateV87.inl"
#include "FRRU/FRRUVauxHQ27ReleaseLoaderV87.inl"
#include "FRRU/FRRUVauxStreetRubbleCandidateV87.inl"

namespace
{
const TCHAR* Missions[]={TEXT("fr.mort_homme"),TEXT("fr.vaux"),TEXT("fr.douaumont"),TEXT("fr.louvemont"),
    TEXT("fr.chemin_dames"),TEXT("fr.malmaison"),TEXT("fr.matz"),TEXT("fr.champagne_defence"),TEXT("fr.soissons"),TEXT("fr.sommepy"),
    TEXT("ru.galicia"),TEXT("ru.przemysl"),TEXT("ru.gorlice"),TEXT("ru.warsaw_rearguard"),TEXT("ru.naroch"),
    TEXT("ru.lutsk"),TEXT("ru.czernowitz"),TEXT("ru.kovel"),TEXT("ru.kerensky"),TEXT("ru.riga")};
const TCHAR* Parts[]={TEXT("ground"),TEXT("solids"),TEXT("dressing"),TEXT("outsideBackdrop")};
const TCHAR* MortSurfacePath=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/MortV3/SemanticMacro/M_MortSemanticMacro_V82.M_MortSemanticMacro_V82");
const TCHAR* WorldSurfacePathV82=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldRedesign/Materials/M_WR_PairedEarth_C1_V82.M_WR_PairedEarth_C1_V82");
bool WorldFamilySceneV82(const FString& Id)
{return Id.EndsWith(TEXT(".scene.families.v1"));}
constexpr int32 MatzLumenPartCountV82=99;
constexpr int32 MatzGroundPartCountV82=8;
constexpr int32 MatzFaciesPartCountV82=3;
bool GermanIngressReviewV82(const FString& Id)
{
    if(Id!=TEXT("fr.matz.scene.families.v1")&&Id!=TEXT("fr.champagne_defence.scene.families.v1")
        &&Id!=TEXT("fr.soissons.scene.families.v1")&&Id!=TEXT("fr.sommepy.scene.families.v1"))return false;
    if(WarfareFRRUGermanHostAcceptanceV82::FormalGermanHostAcceptance)return true;
#if WITH_EDITOR
    FString User;FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User))return false;
    FPaths::NormalizeDirectoryName(User);
    return Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        &&User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase)&&!User.Contains(TEXT(".."));
#else
    return false;
#endif
}
FString GermanIngressBackdropPathV82(const FString& Id)
{
    FString Token=Id;Token.RemoveFromEnd(TEXT(".scene.families.v1"));Token.ReplaceInline(TEXT("."),TEXT("_"));
    const FString Name=TEXT("SM_DE_IngressBackdrop_")+Token+TEXT("_AuthoredBasisV1");
    return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/HostArmour/German/IngressAuthoredBasisV1/%s.%s"),*Name,*Name);
}
bool MatzFaciesReviewV82()
{
#if WITH_EDITOR
    FString User;FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFMatzFaciesReviewV82"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User))return false;
    FPaths::NormalizeDirectoryName(User);
    return Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        &&User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase)&&!User.Contains(TEXT(".."));
#else
    return false;
#endif
}
FString MatzFaciesPartPathV82(int32 Index)
{
    const FString Name=FString::Printf(TEXT("SM_WF_fr_matz_facies_%03d_dressing"),Index);
    return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/MatzFacies/AppearanceV1/Scenes/%s.%s"),*Name,*Name);
}
FString MatzGroundPartPathV82(int32 Index)
{
    const FString Name=FString::Printf(TEXT("SM_WF_fr_matz_ground_lumen_%03d_dressing"),Index);
    return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/LumenGroundBytes/Scenes/%s.%s"),*Name,*Name);
}
bool IsMatzGroundPartV82(const UStaticMesh* Mesh)
{
    if(!Mesh)return false;
    static const TSet<FString> Paths=[](){TSet<FString> Result;for(int32 I=0;I<MatzGroundPartCountV82;++I)Result.Add(MatzGroundPartPathV82(I));return Result;}();
    return Paths.Contains(Mesh->GetPathName());
}
FString MatzLumenPartPathV82(int32 Index)
{
    const FString Name=FString::Printf(TEXT("SM_WF_fr_matz_lumen_%03d_dressing"),Index);
    return WarfareFRRUDressingBasisBindingV87::Resolve(FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/LumenParts/Scenes/%s.%s"),*Name,*Name));
}
bool IsMatzLumenPartV82(const UStaticMesh* Mesh)
{
    if(!Mesh)return false;
    static const TSet<FString> Paths=[](){TSet<FString> Result;for(int32 I=0;I<MatzLumenPartCountV82;++I)Result.Add(MatzLumenPartPathV82(I));return Result;}();
    return Paths.Contains(Mesh->GetPathName());
}
const WarfareFRRUWorldSurfaceV826::FEntry* WorldSurfaceCandidateV826(const FString& Id)
{
    if(const auto* Mort=FRRUMortPairedSamplingV87::Find(Id))return Mort;
    if(const auto* Vaux=WarfareFRRUVauxSurfaceParityV87::Find(Id))return Vaux;
#if WITH_EDITOR
    if(Id==TEXT("ru.naroch.scene.families.v1")
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87NarochSnow"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Naroch={
            TEXT("ru.naroch.scene.families.v1"),TEXT("ru_naroch"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/NarochSnowV87/CandidateV2/Materials/MI_NarochContinuousSnow02_V87.MI_NarochContinuousSnow02_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/NarochSnowV87/CandidateV2/Materials/M_NarochContinuousSnow02_V87.M_NarochContinuousSnow02_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/NarochSnowV87/CandidateV1/Materials/MI_Shoulder_NarochSnow_V87.MI_Shoulder_NarochSnow_V87"),
            TEXT("/Game/Warfare/Field/V8449/Surface/Textures/T_WinterFresh_"),200.f,1306.f};
        return &Naroch;
    }
#endif
    if(Id==TEXT("ru.galicia.scene.redesign.v1")&&WarfareFRRUFieldDetailV87::Enabled(Id)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87GaliciaFacies")))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Galicia={
            TEXT("ru.galicia.scene.redesign.v1"),TEXT("ru_galicia"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/GaliciaFaciesV87/CandidateV1/Materials/MI_AF_ru_galicia_V87.MI_AF_ru_galicia_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/GaliciaFaciesV87/CandidateV1/Materials/M_AF_ru_galicia_V87.M_AF_ru_galicia_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/GaliciaFaciesV87/CandidateV1/Materials/MI_Shoulder_AF_ru_galicia_V87.MI_Shoulder_AF_ru_galicia_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1100.f};
        return &Galicia;
    }
    // One actual-camera candidate reuses the reviewed paired mother and native
    // layer semantics. No scene geometry or old material package is rewritten.
    if(Id==TEXT("fr.mort_homme.scene.redesign.v1")&&WarfareFRRUFieldDetailV87::Enabled(Id))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Mort={
            TEXT("fr.mort_homme.scene.redesign.v1"),TEXT("fr_mort_homme"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/CoverageContract/Materials/MI_MatzCoverageContract_V82.MI_MatzCoverageContract_V82"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/CoverageContract/Materials/M_MatzCoverageContract_V82.M_MatzCoverageContract_V82"),
            TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1000.f};
        return &Mort;
    }
    if(!WarfareFRRUWorldRenderBindingV87::Find(Id))return nullptr;
    for(const auto& Entry:WarfareFRRUWorldSurfaceV826::Entries)if(Id==Entry.Scene)return &Entry;
    return nullptr;
}
bool UseLegacyMalmaisonAdapterV82(const FString& Id)
{return !WorldSurfaceCandidateV826(Id)&&WarfareFRRUMalmaisonSurfaceV82::ReviewEnabled(Id);}
bool PreserveAuthoredSoilV826(const FString& Id)
{
    // The reviewed paired soil and bank share their authored tint. Weather is
    // applied separately; the legacy operation palette must not overwrite it.
    return (Id==TEXT("fr.malmaison.scene.families.v1")&&WorldSurfaceCandidateV826(Id))
        ||WarfareFRRUVauxSurfaceParityV87::Find(Id)!=nullptr
        ||(Id==TEXT("ru.galicia.scene.redesign.v1")&&WarfareFRRUFieldDetailV87::Enabled(Id)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87GaliciaFacies")));
}
const TCHAR* WorldSurfaceForSceneV82(const FString& Id)
{
    if(const auto* Candidate=WorldSurfaceCandidateV826(Id))return Candidate->GroundInstance;
    if(UseLegacyMalmaisonAdapterV82(Id))return WarfareFRRUMalmaisonSurfaceV82::InstancePath();
    if(Id==TEXT("ru.galicia.scene.redesign.v1")&&WarfareFRRUWorldRenderBindingV87::Find(Id))
        return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Materials/M_WF_PairedFields_V82.M_WF_PairedFields_V82");
    if(WorldFamilySceneV82(Id))
    {
        if(Id==TEXT("fr.matz.scene.families.v1"))
        {
            if(MatzFaciesReviewV82())return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/MatzFacies/AppearanceV1/Materials/MI_MatzAppearanceFacies_V82.MI_MatzAppearanceFacies_V82");
            return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/CoverageContract/Materials/MI_MatzCoverageContract_V82.MI_MatzCoverageContract_V82");
        }
        if(Id==TEXT("fr.louvemont.scene.families.v1")||Id==TEXT("ru.naroch.scene.families.v1"))
            return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Materials/M_WF_PairedWinter_V82.M_WF_PairedWinter_V82");
        // The engine texture-bombing experiment did not improve the actual
        // tactical view enough to justify its cost. Keep paired source maps;
        // composition and physical landforms are authored in the same scene.
        if(Id==TEXT("fr.vaux.scene.families.v1")||Id==TEXT("fr.louvemont.scene.families.v1")
            ||Id==TEXT("fr.soissons.scene.families.v1")||Id==TEXT("ru.przemysl.scene.families.v1")
            ||Id==TEXT("ru.gorlice.scene.families.v1")||Id==TEXT("ru.czernowitz.scene.families.v1"))
            return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Materials/M_WF_PairedSettlement_V82.M_WF_PairedSettlement_V82");
        return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Materials/M_WF_PairedFields_V82.M_WF_PairedFields_V82");
    }
    return WorldSurfacePathV82;
}
const TCHAR* WorldSurfaceBaseForSceneV82(const FString& Id)
{
    if(const auto* Candidate=WorldSurfaceCandidateV826(Id))return Candidate->GroundBase;
    if(UseLegacyMalmaisonAdapterV82(Id))return WarfareFRRUMalmaisonSurfaceV82::BasePath();
    if(Id==TEXT("fr.matz.scene.families.v1"))
    {
        if(MatzFaciesReviewV82())return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/MatzFacies/AppearanceV1/Materials/M_MatzAppearanceFacies_V82.M_MatzAppearanceFacies_V82");
        return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/CoverageContract/Materials/M_MatzCoverageContract_V82.M_MatzCoverageContract_V82");
    }
    return WorldSurfaceForSceneV82(Id);
}
bool RedesignedSceneV82(const FString& Id)
{return Id==TEXT("fr.mort_homme.scene.redesign.v1")||Id==TEXT("ru.galicia.scene.redesign.v1")||WorldFamilySceneV82(Id);}
const wfexp::scene::SceneDefinition* NativeSceneV82(const FString& Id)
{for(const auto& Scene:wfexp::scene::Scenes())if(Id==UTF8_TO_TCHAR(Scene.id))return &Scene;return nullptr;}
// The verified FBX pipeline maps authoring +Y to imported -Y. Apply the
// inverse once to the complete receiver scene, including its real collision.
// Native mission coordinates, height interpolation and LOS stay unchanged.
const FTransform AuthoredSceneToNative(FQuat::Identity,FVector::ZeroVector,FVector(1.,-1.,1.));
struct FStagedScene
{
    TWeakObjectPtr<AActor> Owner;
    FString SceneId;
    uint32 Revision=0;
    int32 UCX=0;
    TWeakObjectPtr<AActor> CosmeticProbe;
    TWeakObjectPtr<UStaticMeshComponent> DistantMatte;
    uint64 ValidationFrame=MAX_uint64;
    bool ValidationResult=false;
#if WITH_EDITOR
    uint64 ValidationCalls=0;
    double ValidationMilliseconds=0;
    TSharedPtr<FJsonObject> CosmeticEvidenceV87;
#endif
};
TMap<TWeakObjectPtr<const AWarfareFrontlineGameMode>,FStagedScene> Scenes;


bool Reject(FString& Error,const FString& Reason){Error=Reason;return false;}
bool WetScene(const FString& Id)
{
    if(RedesignedSceneV82(Id))if(const auto* Scene=NativeSceneV82(Id))return !Scene->waterPatches.empty();
    return Id==TEXT("ru.galicia.scene.v2")||Id==TEXT("ru.warsaw_rearguard.scene.v2")
        ||Id==TEXT("ru.kovel.scene.v2")||Id==TEXT("ru.riga.scene.v2");
}
bool ShaderReady(UMaterialInterface* Material,FString& Error)
{
    if(!Material)return Reject(Error,TEXT("required material is missing"));
    const FMaterialResource* Resource=Material->GetMaterialResource(GMaxRHIShaderPlatform);
    const FMaterialShaderMap* Map=Resource?Resource->GetGameThreadShaderMap():nullptr;
#if WITH_EDITOR
    if(!Resource||!Resource->IsGameThreadShaderMapComplete()||!Map||!Map->IsValidForRendering())
    {
        // A retained instance can delegate its rendering resource to an
        // uncached parent. Complete that chain before evaluating the leaf.
        TArray<UMaterialInterface*> Chain;
        for(UMaterialInterface* Current=Material;Current;)
        {
            if(Chain.Contains(Current))return Reject(Error,TEXT("cyclic material parent chain"));
            Chain.Add(Current);const auto* Instance=Cast<UMaterialInstance>(Current);
            Current=Instance?Instance->Parent.Get():nullptr;
        }
        if(auto* Base=Material->GetMaterial())Chain.AddUnique(Base);
        for(int32 I=Chain.Num()-1;I>=0;--I)Chain[I]->CacheShaders(EMaterialShaderPrecompileMode::Synchronous);
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        Resource=Material->GetMaterialResource(GMaxRHIShaderPlatform);
        Map=Resource?Resource->GetGameThreadShaderMap():nullptr;
    }
#endif
    if(!Resource||!Resource->IsGameThreadShaderMapComplete()||!Map||!Map->IsValidForRendering())
    {
        FString Details=TEXT("cooked shader map unavailable");
#if WITH_EDITOR
        Details=Resource?FString::Join(Resource->GetCompileErrors(),TEXT("; ")):TEXT("no resource");
#endif
        return Reject(Error,FString::Printf(TEXT("%s: actual shader map incomplete/invalid (resource=%d complete=%d map=%d valid=%d errors=%s)"),
            *Material->GetPathName(),Resource!=nullptr,Resource&&Resource->IsGameThreadShaderMapComplete(),
            Map!=nullptr,Map&&Map->IsValidForRendering(),*Details));
    }
    return true;
}
UMaterialInstanceDynamic* CreateWorldSurfaceInstanceV826(const FString& Id,UStaticMeshComponent* Component,UMaterialInterface* Material,FString& Error)
{
    const auto* Recipe=WorldSurfaceCandidateV826(Id);if(!Recipe)return nullptr;
    auto* Projected=UFrontlineTerrainMaterialV47::CreateForGround(Material,Component);
    if(!Projected){Reject(Error,TEXT("paired world surface sampling instance missing"));return nullptr;}
    TArray<UTexture*> Used;Material->GetUsedTextures(Used,GetCachedScalabilityCVars().MaterialQualityLevel,GMaxRHIShaderPlatform);
    TArray<FMaterialTextureInfo> Sampling;bool HasCover=false;
    for(UTexture* Texture:Used)if(Cast<UTexture2D>(Texture))
    {
        const FString Path=Texture->GetPathName();float Scale=0.f;
        if(Path.StartsWith(TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_"))
            ||Path.StartsWith(TEXT("/Game/Warfare/Field/V47/Textures/T_Mud_HeightAO_")))Scale=WarfareFRRUWorldSurfaceV826::MudSamplingScale;
        else if(Path.StartsWith(TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_")))Scale=WarfareFRRUWorldSurfaceV826::ClaySamplingScale;
        else if(Path.StartsWith(Recipe->CoverTexturePrefix))
        {Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/Recipe->CoverRepeatCm;HasCover=true;}
        else if(Id==TEXT("fr.malmaison.scene.families.v1")&&Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/AppearanceFaciesV826/CandidateV1/Textures/T_AppearanceFacies_fr_malmaison_V826.T_AppearanceFacies_fr_malmaison_V826"))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/24000.f;
        else if(Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/GaliciaFaciesV87/CandidateV1/Textures/T_AppearanceFacies_ru_galicia_V87.T_AppearanceFacies_ru_galicia_V87"))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/24000.f;
        else if(Id==TEXT("ru.naroch.scene.families.v1")&&Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/NarochSnowV87/CandidateV1/Textures/T_SnowFacies_ru_naroch_V87.T_SnowFacies_ru_naroch_V87"))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/24000.f;
#if WITH_EDITOR
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrownMud"))&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FString(Recipe->GroundBase)==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxBrownMud/CandidateV1/Materials/M_VauxBrownMudScan_V87.M_VauxBrownMudScan_V87")
            &&(Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_brown_mud_02_colour_V82.T_WF_brown_mud_02_colour_V82")
                ||Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_brown_mud_02_normal_V82.T_WF_brown_mud_02_normal_V82")
                ||Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_brown_mud_02_rough_V82.T_WF_brown_mud_02_rough_V82")
                ||Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_brown_mud_02_ao_V82.T_WF_brown_mud_02_ao_V82")))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/129.999601840973f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust"))
            &&Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/T_Rubble_")))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/200.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust"))
            &&Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/T_Chalk_")))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/100.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust"))
            &&Path.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_")))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/315.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust"))
            &&Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxMacroDesign/CandidateV4/Textures/T_VauxMacroRegions_RGBA_V87.T_VauxMacroRegions_RGBA_V87"))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/12000.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxMacro"))
            &&(Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/T_Rubble_"))||Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/T_Chalk_"))))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/200.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxMacro"))
            &&Path.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_")))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/315.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxMacro"))
            &&Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxMacroDesign/CandidateV2/Textures/T_VauxMacroRegions_RGBA_V87.T_VauxMacroRegions_RGBA_V87"))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/12000.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxRubble"))
            &&Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/T_Rubble_")))
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/240.f;
        else if(Id==TEXT("fr.vaux.scene.families.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxRubble"))
            &&Path==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRubbleTransition/CandidateV1/Textures/T_VauxRubbleMacroMask_V87.T_VauxRubbleMacroMask_V87"))
            // World XY spans 24000 x 12000 cm; retain the denser axis for streaming.
            Scale=WarfareFRRUWorldSurfaceV826::NativeUVCm/12000.f;
#endif
        if(Scale<=0||!FMath::IsFinite(Scale))
        {Reject(Error,TEXT("unregistered paired world surface texture scale: ")+Path);return nullptr;}
        FMaterialTextureInfo Entry;Entry.TextureName=Texture->GetFName();Entry.UVChannelIndex=0;Entry.SamplingScale=Scale;
#if WITH_EDITORONLY_DATA
        Entry.TextureReference=FSoftObjectPath(Texture);
#endif
        Sampling.Add(Entry);
    }
    if(!HasCover){Reject(Error,TEXT("paired world surface cover maps missing"));return nullptr;}
    Projected->SetWorldSampling(Sampling);Component->SetMaterial(0,Projected);return Projected;
}
bool MissionName(const FString& Scene,FString& Mission)
{
    if(RedesignedSceneV82(Scene))if(const auto* Native=NativeSceneV82(Scene)){Mission=UTF8_TO_TCHAR(Native->missionId);return true;}
    if(Scene==TEXT("fr.mort_homme.scene.v3")){Mission=TEXT("fr.mort_homme");return true;}
    for(const TCHAR* Id:Missions)
        if(Scene==FString(Id)+TEXT(".scene.v2")){Mission=Id;return true;}
    return false;
}
bool ValidateMesh(UStaticMesh* Mesh,int32 Part,int32 ExpectedUCX,FString& Error,bool ReceiverMort=false,bool ReceiverWorld=false)
{
    if(!Mesh||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.IsEmpty()
        ||Mesh->GetRenderData()->LODResources[0].GetNumTriangles()<=0)
        return Reject(Error,FString(Parts[Part])+TEXT(": missing real render triangles"));
    UBodySetup* Body=Mesh->GetBodySetup();
    if(!Body)return Reject(Error,FString(Parts[Part])+TEXT(": missing BodySetup"));
    const int32 Shapes=Body->AggGeom.GetElementCount();
    if(Part==1)
    {
        if(ExpectedUCX<=0||Shapes!=ExpectedUCX||Body->AggGeom.ConvexElems.Num()!=ExpectedUCX
            ||Body->CollisionTraceFlag!=CTF_UseSimpleAsComplex)
            return Reject(Error,TEXT("solids: actual convex shapes differ from native solid count"));
    }
    else if(Shapes!=0||(Part==0&&Body->CollisionTraceFlag!=CTF_UseComplexAsSimple))
        return Reject(Error,FString(Parts[Part])+TEXT(": unexpected simple/complex collision contract"));
    const bool Solid=Part<2;
    if(Body->DefaultInstance.GetCollisionEnabled(false)!=(Solid?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision)
        ||Body->DefaultInstance.GetCollisionProfileName()!=FName(Solid?TEXT("BlockAll"):TEXT("NoCollision")))
        return Reject(Error,FString(Parts[Part])+TEXT(": imported collision profile differs"));
    if(Mesh->GetStaticMaterials().IsEmpty())return Reject(Error,FString(Parts[Part])+TEXT(": no material slots"));
    for(const FStaticMaterial& Slot:Mesh->GetStaticMaterials())
    {
        UMaterialInterface* Material=Slot.MaterialInterface;
        const bool ReceiverSurface=ReceiverMort&&(Part==0||Part==3)&&Material
            &&Material->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/MortV3/M_MortReceiverSurface_V82.M_MortReceiverSurface_V82");
        const bool WorldMaterial=ReceiverWorld&&Material&&(Material->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldRedesign/Materials/"))
            ||Material->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Materials/")));
        bool PairedTerrainV826=false;
        if(ReceiverWorld&&Material&&(Part==0||Part==2))
            for(const auto& Recipe:WarfareFRRUWorldSurfaceV826::Entries)
            {
                bool RegisteredGround=Mesh->GetPathName()==WarfareFRRUWorldTerrainV826::Ground(Recipe.Scene)
                    ||WarfareFRRUWorldRenderBindingV87::Registered(Mesh->GetPathName(),WarfareFRRUWorldRenderBindingV87::Find(Recipe.Scene),0);
                for(int32 I=0;I<8;++I)RegisteredGround|=Mesh->GetPathName()==WarfareFRRUWorldTerrainV826::RenderGround(Recipe.Scene,I);
                // Appearance-only copies replace the runtime component material.
                // Their source meshes retain the exact verified baked slot.
                const FString Baked=FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/CandidateV1/Materials/MI_WS_%s_V826.MI_WS_%s_V826"),Recipe.Token,Recipe.Token);
                const bool RetainedBaked=(FString(Recipe.Scene)==TEXT("fr.malmaison.scene.families.v1")
                    ||FString(Recipe.Scene)==TEXT("ru.galicia.scene.redesign.v1"))&&Material->GetPathName()==Baked;
                PairedTerrainV826|=RegisteredGround&&(Material->GetPathName()==Recipe.GroundInstance||RetainedBaked);
            }
        // This receiver-only dressing reuses the pinned host PBR materials.
        // Keep exact slot/path pairs; accepting the entire host folder would
        // hide an accidental material substitution on unrelated scene parts.
        bool GroupedDressingMaterial=false;
        // ImportedMaterialSlotName is Editor-only in a cooked FStaticMaterial.
        // MaterialSlotName survives cooking; keep the same exact name+URI gate.
        FName RegisteredSlot=Slot.MaterialSlotName;
#if WITH_EDITOR
        if(!Slot.ImportedMaterialSlotName.IsNone())RegisteredSlot=Slot.ImportedMaterialSlotName;
#endif
        if(ReceiverWorld&&Part==2&&Material&&(WarfareFRRUWorldRenderBindingV87::Registered(Mesh->GetPathName())||IsMatzLumenPartV82(Mesh)||Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/GroupedTrenchCarrierRestore/Scenes/SM_WF_fr_matz_grouped_dressing.SM_WF_fr_matz_grouped_dressing")))
        {
            const TCHAR* Slots[]={TEXT("V40_Timber"),TEXT("V40_Rust"),TEXT("V76_Bark"),TEXT("V76_Fracture"),TEXT("V42_Timber"),TEXT("V42_Rust"),TEXT("V42_Mud"),TEXT("V42_Hessian")};
            const TCHAR* Paths[]={TEXT("/Game/Warfare/Field/V40/Materials/M_Timber_V40.M_Timber_V40"),TEXT("/Game/Warfare/Field/V40/Materials/M_Rust_V40.M_Rust_V40"),TEXT("/Game/Warfare/Field/V76/Materials/M_Bark_V76.M_Bark_V76"),TEXT("/Game/Warfare/Field/V76/Materials/M_Fracture_V76.M_Fracture_V76"),TEXT("/Game/Warfare/Field/V42/Materials/M_Timber_V42.M_Timber_V42"),TEXT("/Game/Warfare/Field/V42/Materials/M_Rust_V42.M_Rust_V42"),TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42.MI_EarthBank_V42"),TEXT("/Game/Warfare/Field/V76/Materials/M_Hessian_V76.M_Hessian_V76")};
            for(int32 I=0;I<UE_ARRAY_COUNT(Slots);++I)
                GroupedDressingMaterial|=RegisteredSlot==FName(Slots[I])&&Material->GetPathName()==Paths[I];
        }
        if(!Material||(!ReceiverSurface&&!WorldMaterial&&!PairedTerrainV826&&!GroupedDressingMaterial&&!Material->GetPathName().StartsWith(TEXT("/Game/FRRUExpansionR3/FormalV82/Materials/"))))
            return Reject(Error,FString(Parts[Part])+TEXT(": foreign/missing paired material"));
        if(!ShaderReady(Material,Error))return false;
        if(RegisteredSlot==TEXT("M_Terrain_R3_TreeCollisionProxy"))
        {
            const UMaterial* Base=Material->GetMaterial();
            if(!Base||Base->GetBlendMode()!=BLEND_Masked||Base->OpacityMaskClipValue<=0)
                return Reject(Error,TEXT("tree proxy: actual masked material contract missing"));
        }
    }
    return true;
}
bool LoadScene(const FString& Id,int32 UCX,TArray<UStaticMesh*>& Meshes,FString& Error)
{
    FString Mission;if(!MissionName(Id,Mission))return Reject(Error,TEXT("unregistered native scene revision"));
    const bool ReceiverMort=Id==TEXT("fr.mort_homme.scene.v3");
    const bool ReceiverWorld=RedesignedSceneV82(Id);
    Mission.ReplaceInline(TEXT("."),TEXT("_"));
    for(int32 I=0;I<4;++I)
    {
        const FString Name=FString::Printf(TEXT("SM_R3_%s_%s"),*Mission,Parts[I]);
        FString Path=FString::Printf(TEXT("/Game/FRRUExpansionR3/FormalV82/Scenes/%s.%s"),*Name,*Name);
        if(ReceiverMort&&I!=1)
        {
            const TCHAR* Suffix=I==0?TEXT("Ground"):I==2?TEXT("Dressing"):TEXT("Outside");
            const FString ReceiverName=FString::Printf(TEXT("SM_MortReceiver%s_V82"),Suffix);
            Path=FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/MortV3/%s.%s"),*ReceiverName,*ReceiverName);
        }
        if(Id==TEXT("ru.galicia.scene.v2")&&I==3)
            Path=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/GaliciaOutside/SM_GaliciaOutsideBoundary_V82.SM_GaliciaOutsideBoundary_V82");
        if(ReceiverWorld)
        {
            const bool Family=WorldFamilySceneV82(Id);
            const FString ReceiverName=Family?FString::Printf(TEXT("SM_WF_%s_%s"),*Mission,Parts[I]):FString::Printf(TEXT("SM_WR_%s_%s"),*Mission,Parts[I]);
            Path=FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/%s/Scenes/%s.%s"),Family?TEXT("WorldFamilies"):TEXT("WorldRedesign"),*ReceiverName,*ReceiverName);
            if(I==0||I==3)
            {
                // The previous Interchange path ignored legacy FBX colour
                // settings. These unchanged physical meshes have actual
                // source-byte-verified semantic buffers for all 20 scenes.
                const FString SurfaceName=FString::Printf(TEXT("SM_WF_%s_%s"),*Mission,Parts[I]);
                Path=FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/SurfaceData/Scenes/%s.%s"),*SurfaceName,*SurfaceName);
            }
            if(I==2&&WarfareFRRUWorldRenderBindingV87::Count(WarfareFRRUWorldRenderBindingV87::Find(Id),2)>0)
                Path=WarfareFRRUWorldRenderBindingV87::Path(WarfareFRRUWorldRenderBindingV87::Find(Id),2,0);
            else if(Id==TEXT("fr.matz.scene.families.v1")&&I==2)
                Path=MatzLumenPartPathV82(0);
        }
        if(I==0)
        {
            const FString Terrain=WarfareFRRUWorldTerrainV826::Ground(Id);
            if(!Terrain.IsEmpty())Path=Terrain;
        }
        // Atomic Vaux Native2/UCX2 pair; every other receiver keeps its original physical mesh.
        if(I==1&&Id==TEXT("fr.vaux.scene.families.v1")&&FRRUVauxHQ27ReleaseV87::SourceDataAvailable)Path=FRRUVauxHQ27ReleaseV87::SolidsPath;
        // Replace only the visible outer triangles intersecting the two exact
        // native ingress strips. The separately staged real apron supplies both
        // its visible surface and physical contact; no overlapping old backdrop.
        if(I==3&&GermanIngressReviewV82(Id))Path=GermanIngressBackdropPathV82(Id);
        if(I==2)Path=WarfareFRRUDressingBasisBindingV87::Resolve(Path);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if(!ValidateMesh(Mesh,I,UCX,Error,ReceiverMort,ReceiverWorld))return false;
        Meshes.Add(Mesh);
    }
    return true;
}
bool LoadReceiverSurface(const FString& Id,UMaterialInterface*& Surface,FString& Error)
{
    Surface=nullptr;if(Id!=TEXT("fr.mort_homme.scene.v3")&&!RedesignedSceneV82(Id))return true;
    Surface=LoadObject<UMaterialInterface>(nullptr,RedesignedSceneV82(Id)?WorldSurfaceForSceneV82(Id):MortSurfacePath);
    return ShaderReady(Surface,Error);
}
bool DistantSceneV82(const FString& Id)
{return RedesignedSceneV82(Id)&&NativeSceneV82(Id);}
FString DistantMaterialPathV82(const FString& Id)
{
    const auto* Scene=NativeSceneV82(Id);if(!Scene)return FString();
    const FString Mission=UTF8_TO_TCHAR(Scene->missionId);const TCHAR* Region=nullptr;
    if(Mission==TEXT("fr.louvemont"))Region=TEXT("VerdunWinter");
    else if(Mission==TEXT("fr.mort_homme")||Mission==TEXT("fr.vaux")||Mission==TEXT("fr.douaumont")
        ||Mission==TEXT("fr.chemin_dames")||Mission==TEXT("fr.malmaison"))Region=TEXT("Verdun");
    else if(Mission.StartsWith(TEXT("fr.")))Region=TEXT("Matz");
    else if(Mission==TEXT("ru.naroch"))Region=TEXT("Naroch");
    else if(Mission==TEXT("ru.warsaw_rearguard")||Mission==TEXT("ru.kovel")||Mission==TEXT("ru.riga"))Region=TEXT("EasternLowlands");
    else if(Mission.StartsWith(TEXT("ru.")))Region=TEXT("Galicia");
    if(!Region)return FString();
    return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Distant/MI_Distant%s_V82.MI_Distant%s_V82"),Region,Region);
}
bool LoadDistantV82(const FString& Id,UMaterialInterface*& Material,FString& Error)
{
    Material=nullptr;if(!DistantSceneV82(Id))return true;
    Material=LoadObject<UMaterialInterface>(nullptr,*DistantMaterialPathV82(Id));
    return ShaderReady(Material,Error);
}
FTransform DistantPlacementV82(const FString& Id)
{
    const auto* Scene=NativeSceneV82(Id);if(!Scene)return FTransform::Identity;
    const double Centre=(Scene->minX+Scene->maxX)*.5,Span=Scene->maxX-Scene->minX+600.;
    // Preserve the authored 3:1 panorama and its fixed geographical registration.
    // The photograph's distant fields meet the real northern edge; its near
    // painted ground remains behind the actual three-dimensional apron.
    const double Height=Span/3.;double ContactZ=0.;
    wfexp::scene::HeightAt(*Scene,Centre,Scene->minY,ContactZ);
    double RegistrationV=Id==TEXT("fr.malmaison.scene.families.v1")?.43:.36;
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV826DistantRegistration")))
    {
        // Receiver-owned visual candidate only. The lower, already painted
        // settlement/field band was hidden behind the physical northern edge.
        // Preserve the photo's aspect, horizontal geography and all real ground.
        // The large foreground buildings remain outside this modest registration.
        if(Id==TEXT("ru.galicia.scene.redesign.v1"))RegistrationV=.42;
    }
#endif
    return FTransform(FQuat::Identity,FVector(Centre,Scene->minY-120.,ContactZ-(1.-RegistrationV)*Height),FVector(Span/100.,1.,Height/100.));
}
bool StageDistantV82(AActor* Owner,USceneComponent* Root,const FString& Id,UStaticMeshComponent*& Out,FString& Error)
{
    Out=nullptr;if(!DistantSceneV82(Id))return true;
    UMaterialInterface* Material=nullptr;if(!LoadDistantV82(Id,Material,Error))return false;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_PanoramaCard.SM_PanoramaCard"));
    if(!Mesh||!Mesh->GetRenderData())return Reject(Error,TEXT("registered production panorama card missing"));
    auto* C=NewObject<UStaticMeshComponent>(Owner,TEXT("FRRU_V82DistantMatte"),RF_Transient);
    Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
    // The physical scene's Y reflection must never reflect the authored photo.
    C->SetAbsolute(true,true,true);C->SetStaticMesh(Mesh);C->SetMaterial(0,Material);
    C->SetWorldTransform(DistantPlacementV82(Id));C->ComponentTags.Add(TEXT("FRRU_V82DistantMatte"));
    C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);
    C->SetVisibleInRayTracing(false);C->SetAffectDistanceFieldLighting(false);C->SetAffectDynamicIndirectLighting(false);
    C->SetAffectIndirectLightingWhileHidden(false);C->SetCastShadow(false);C->SetCastHiddenShadow(false);C->SetEmissiveLightSource(false);
    C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;C->bVisibleInReflections=false;
    C->RegisterComponent();if(!C->CreateDynamicMaterialInstance(0,Material))return Reject(Error,TEXT("national panorama instance unavailable"));
    Out=C;return true;
}
bool FieldVegetationSceneV82(const FString& Id)
{
    // The six-thousand-tuft tactical candidate had negligible visible benefit.
    // Keep its owned source assets, but do not pay its geometry cost in play.
    return false;
}

bool StageFieldVegetationV82(AActor* Owner,USceneComponent* Root,UStaticMesh* Ground,const FString& Id,FString& Error)
{
    if(!FieldVegetationSceneV82(Id))return true;
    const auto* Native=NativeSceneV82(Id);
    if(!Owner||!Root||!Native||!Ground||!Ground->GetRenderData())return Reject(Error,TEXT("field vegetation source missing"));
    const auto& LOD=Ground->GetRenderData()->LODResources[0];
    if(LOD.VertexBuffers.ColorVertexBuffer.GetNumVertices()!=LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices())
        return Reject(Error,TEXT("field vegetation needs the actual semantic ground colours"));
    UHierarchicalInstancedStaticMeshComponent* Tufts[3]={};
    for(int32 I=0;I<3;++I)
    {
        const FString Name=FString::Printf(TEXT("SM_WF_FieldVegetation_grass%c"),TCHAR('A'+I));
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Scenes/%s.%s"),*Name,*Name));
        if(!Mesh||!Mesh->GetRenderData()||Mesh->GetStaticMaterials().Num()!=1)
            return Reject(Error,TEXT("original artist field tuft missing"));
        if(!ShaderReady(Mesh->GetMaterial(0),Error))return false;
        auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner,FName(*Name),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(Mesh);C->ComponentTags.Add(TEXT("FRRU_V82FieldVegetation"));
        C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);
        C->SetCullDistances(0,20000);C->RegisterComponent();Tufts[I]=C;
    }
    // Sample the original ground's semantic cover, never a second random floor
    // or a visual-only heightfield. Clustered dry tufts leave roads, craters and
    // native collision envelopes clear. No per-frame vegetation work or bodies.
    FRandomStream Random(0x1f391862);const auto Indices=LOD.IndexBuffer.GetArrayView();
    const int32 Triangles=Indices.Num()/3;int32 Placed=0;
    TArray<int32> CoverTriangles;
    for(int32 Triangle=0;Triangle<Triangles;++Triangle)
    {
        float Cover=0.f,Damp=0.f;
        for(int32 Corner=0;Corner<3;++Corner)
        {
            const FColor Colour=LOD.VertexBuffers.ColorVertexBuffer.VertexColor(Indices[Triangle*3+Corner]);
            Cover+=Colour.B/765.f;Damp+=Colour.G/765.f;
        }
        // The authored field/woodland coverage is the placement authority;
        // six tiny hand-picked islands left the visible farmland empty.
        if(Cover>=.22f&&Damp<.72f)CoverTriangles.Add(Triangle);
    }
    if(CoverTriangles.IsEmpty())return Reject(Error,TEXT("field vegetation has no actual cover triangles"));
    for(int32 Attempt=0;Attempt<60000&&Placed<6000;++Attempt)
    {
        const int32 Triangle=CoverTriangles[Random.RandRange(0,CoverTriangles.Num()-1)]*3;
        const uint32 A=Indices[Triangle],B=Indices[Triangle+1],C=Indices[Triangle+2];
        float U=Random.FRand(),V=Random.FRand();if(U+V>1.f){U=1.f-U;V=1.f-V;}
        const float W=1.f-U-V;
        const FVector P=FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(A))*W
            +FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(B))*U
            +FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(C))*V;
        const FVector World=AuthoredSceneToNative.TransformPosition(P);
        const FColor CA=LOD.VertexBuffers.ColorVertexBuffer.VertexColor(A),CB=LOD.VertexBuffers.ColorVertexBuffer.VertexColor(B),CC=LOD.VertexBuffers.ColorVertexBuffer.VertexColor(C);
        const float Cover=(CA.B*W+CB.B*U+CC.B*V)/255.f;
        const float Damp=(CA.G*W+CB.G*U+CC.G*V)/255.f;
        if(Cover<.22f||Damp>.72f)continue;
        const float Cluster=.5f+.5f*FMath::PerlinNoise2D(FVector2D(World.X,World.Y)/460.f);
        if(Random.FRand()>Cover*Cluster*2.f)continue;
        double H=0,HX=0,HY=0;
        if(!wfexp::scene::HeightAt(*Native,World.X,World.Y,H)
            ||!wfexp::scene::HeightAt(*Native,World.X+12.,World.Y,HX)
            ||!wfexp::scene::HeightAt(*Native,World.X,World.Y+12.,HY))continue;
        if(FMath::Abs(HX-H)>6.||FMath::Abs(HY-H)>6.)continue;
        bool Obstructed=false;
        for(const auto& Solid:Native->solids)
            if(World.X>Solid.minimum.x-20.&&World.X<Solid.maximum.x+20.
                &&World.Y>Solid.minimum.y-20.&&World.Y<Solid.maximum.y+20.){Obstructed=true;break;}
        if(Obstructed)continue;
        wfexp::scene::WaterSurface Water;if(wfexp::scene::QueryWaterSurface(*Native,World.X,World.Y,Water)&&Water.depthCm>.5)continue;
        const FVector Normal=FVector(-(HX-H)/12.,-(HY-H)/12.,1.).GetSafeNormal();
        const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal)
            *FQuat(FVector::UpVector,Random.FRandRange(0.f,2.f*PI));
        const float Scale=Random.FRandRange(.85f,1.3f);
        const FTransform WorldTransform(Q,FVector(World.X,World.Y,H-.7),FVector(Scale));
        Tufts[Placed%3]->AddInstance(WorldTransform.GetRelativeTransform(AuthoredSceneToNative));++Placed;
    }
    return Placed>=3||Reject(Error,TEXT("no real semantic cover for field vegetation"));
}
bool RootedShoulderSceneV82(const FString& Id)
{return Id==TEXT("fr.matz.scene.families.v1");}
bool StageRootedShouldersV82(AActor* Owner,USceneComponent* Root,const FString& Id,FString& Error)
{
    if(!RootedShoulderSceneV82(Id))return true;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V42/Meshes/SM_RootedApron_A_V42.SM_RootedApron_A_V42"));
    if(!Mesh||!Mesh->GetRenderData()||Mesh->GetStaticMaterials().Num()!=2)return Reject(Error,TEXT("retained rooted-earth source missing"));
    for(int32 Slot=0;Slot<2;++Slot)if(!ShaderReady(Mesh->GetMaterial(Slot),Error))return false;
    // Baked full-bottom contact against this scene's exact 00->11 grid.
    // All eight pieces remain outside routes, bays, water and native solids;
    // their bottom gaps are -5..-1.986 cm. Existing logs and firing bays stay.
    struct FShoulder {double X,Y,Z,Yaw,SlopeX,SlopeY;};
    static const FShoulder Shoulders[]={
        {-3565,1530,83.787,-16,.054286,-.128},
        {-3045,1965,65.316,31,.031286,-.041952},
        {-3795,-5250,163.292,-12,.021333,.138667},
        {-3290,-5160,187.146,17,.021333,.137238},
        {3500,2150,160.091,19,-.078238,-.028429},
        {4130,2450,106.816,45,-.059238,-.040762},
        {-1205,-4925,229.814,-15,-.025571,.110905},
        {-1685,-5230,205.345,24,-.004,.132}};
    auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner,TEXT("FRRU_V82RootedShoulders"),RF_Transient);
    Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(Mesh);C->ComponentTags.Add(TEXT("FRRU_V82RootedShoulders"));
    C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetCullDistances(0,24000);C->RegisterComponent();
    for(const auto& S:Shoulders)
    {
        const FVector Normal=FVector(-S.SlopeX,-S.SlopeY,1.).GetSafeNormal();
        const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal)*FQuat(FVector::UpVector,FMath::DegreesToRadians(S.Yaw));
        // The retained FBX's local Y is reflected. Match the source's fitted
        // geometry once, then express it in the existing reflected scene root.
        const FTransform World(Q,FVector(S.X,S.Y,S.Z),FVector(1.,-1.,.85));
        C->AddInstance(World.GetRelativeTransform(AuthoredSceneToNative));
    }
    return C->GetInstanceCount()==8||Reject(Error,TEXT("rooted-earth group incomplete"));
}
bool ComponentsValid(AActor* Owner,int32 ExpectedUCX,const FString& Id)
{
    if(!IsValid(Owner)||!Owner->GetActorTransform().Equals(AuthoredSceneToNative,1.e-8))return false;
    TArray<UStaticMeshComponent*> Components;Owner->GetComponents(Components);
    const bool Wet=Owner->ActorHasTag(TEXT("FRRU_V82WetScene"));
    const bool Field=FieldVegetationSceneV82(Id);
    const bool Distant=DistantSceneV82(Id);
    const bool Rooted=RootedShoulderSceneV82(Id);
    const auto* Render=WarfareFRRUWorldRenderBindingV87::Find(Id);
    const bool MatzParts=!Render&&Id==TEXT("fr.matz.scene.families.v1");
    const int32 ExtraRender=Render?Render->Ground+Render->Solids+FMath::Max(0,Render->Dressing-1):0;
    TSet<FString> RenderPaths;
    const bool Facies=MatzParts&&MatzFaciesReviewV82();TSet<FString> FaciesPaths;
    if(Components.Num()!=((Wet?5:4)+(Field?3:0)+(Distant?1:0)+(Rooted?1:0)+ExtraRender+WarfareFRRUWorldWoodlineV826::ComponentCount(Id)+WarfareFRRUBlastedWoodlineV826::ComponentCount(Id)+WarfareFRRUWorldDressingV827::ComponentCount(Id)+WarfareFRRUFieldDetailV87::ComponentCount(Id)+WarfareFRRUVauxCompositionV87::ComponentCount(Id)+FRRUMortFlankFireStepV87::ComponentCount(Id)+FRRUVauxTripoReleaseLoaderV87::ComponentCount(Id)+(MatzParts?MatzLumenPartCountV82-1+MatzGroundPartCountV82:0)+(Facies?MatzFaciesPartCountV82:0)))return false;
    int32 SolidCount=0,RiverCount=0,ReceiverSurfaces=0,VegetationCount=0,DistantCount=0,RootedCount=0,DressingPartCount=0;
    TSet<FString> GroundPartPaths;
    for(UStaticMeshComponent* Component:Components)
    {
        if(Component->ComponentHasTag(FRRUVauxTripoReleaseLoaderV87::Tag))
        {if(!FRRUVauxTripoReleaseLoaderV87::ValidateComponent(Id,Component))return false;continue;}
        if(Component->ComponentHasTag(FRRUMortFlankFireStepV87::Tag))
        {if(!FRRUMortFlankFireStepV87::ValidateComponent(Id,Component))return false;continue;}
        if(Component->ComponentHasTag(TEXT("FRRU_V87VauxRoots")))
        {if(!WarfareFRRUVauxCompositionV87::ValidateComponent(Id,Component))return false;continue;}
        if(Component->ComponentHasTag(TEXT("FRRU_V87FieldDetail")))
        {if(!WarfareFRRUFieldDetailV87::Validate(Id,Component))return false;continue;}
        if(Component->ComponentHasTag(TEXT("FRRU_V827OperationDressing")))
        {
            if(!WarfareFRRUWorldDressingV827::ValidateComponent(Id,Component))return false;
            continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V826BlastedWoodline")))
        {
            if(!WarfareFRRUBlastedWoodlineV826::ValidateComponent(Id,Component))return false;
            continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V826Woodline")))
        {
            if(!WarfareFRRUWorldWoodlineV826::ValidateComponent(Id,Component))return false;
            continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V826WorldPart")))
        {
#if WITH_EDITOR
            if(FRRUVauxHQ27VisualV87::IsRemainder(Component))
            {
                if(!FRRUVauxHQ27VisualV87::ValidateRemainder(Id,Component))return false;
                const FString CandidatePath=Component->GetStaticMesh()->GetPathName();
                if(RenderPaths.Contains(CandidatePath))return false;RenderPaths.Add(CandidatePath);continue;
            }
#endif
            const UStaticMesh* Mesh=Component->GetStaticMesh();const auto* Material=Component->GetMaterial(0);
            if(!Render||!Mesh||RenderPaths.Contains(Mesh->GetPathName())
                ||!WarfareFRRUWorldRenderBindingV87::Registered(Mesh->GetPathName())
                ||!Component->IsRegistered()||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
                ||!Component->GetRelativeTransform().Equals(FTransform::Identity,1.e-8))return false;
            bool InScene=false;for(int32 Section=0;Section<3;++Section)
                InScene|=WarfareFRRUWorldRenderBindingV87::Registered(Mesh->GetPathName(),Render,Section);
            if(!InScene)return false;
            if(const auto* Body=Component->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            if(WarfareFRRUWorldRenderBindingV87::Registered(Mesh->GetPathName(),Render,0)
                &&(!Material||Material->GetMaterial()->GetPathName()!=WorldSurfaceBaseForSceneV82(Id)
                    ||(UseLegacyMalmaisonAdapterV82(Id)&&!WarfareFRRUMalmaisonSurfaceV82::AppliedTo(Component))))return false;
            RenderPaths.Add(Mesh->GetPathName());continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82MatzFaciesPart")))
        {
            const UStaticMesh* Mesh=Component->GetStaticMesh();if(!Facies||!Mesh||FaciesPaths.Contains(Mesh->GetPathName()))return false;
            bool Known=false;for(int32 I=0;I<MatzFaciesPartCountV82;++I)Known|=Mesh->GetPathName()==MatzFaciesPartPathV82(I);
            if(!Known||!Component->IsRegistered()||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
                ||!Component->GetRelativeTransform().Equals(FTransform::Identity,1.e-8))return false;
            if(const auto* Body=Component->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            FaciesPaths.Add(Mesh->GetPathName());continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82MatzGroundPart")))
        {
            const UStaticMesh* Mesh=Component->GetStaticMesh();const auto* Material=Component->GetMaterial(0);
            if(!MatzParts||!IsMatzGroundPartV82(Mesh)||GroundPartPaths.Contains(Mesh->GetPathName())
                ||!Component->IsRegistered()||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
                ||!Component->GetRelativeTransform().Equals(FTransform::Identity,1.e-8)
                ||!Material||Material->GetMaterial()->GetPathName()!=WorldSurfaceBaseForSceneV82(Id))return false;
            if(const auto* Body=Component->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            GroundPartPaths.Add(Mesh->GetPathName());continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82MatzLumenPart")))
        {
            if(!MatzParts||!IsMatzLumenPartV82(Component->GetStaticMesh())||!Component->IsRegistered()
                ||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
                ||!Component->GetRelativeTransform().Equals(FTransform::Identity,1.e-8))return false;
            if(const auto* Body=Component->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            ++DressingPartCount;continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82RootedShoulders")))
        {
            const auto* Instances=Cast<UHierarchicalInstancedStaticMeshComponent>(Component);
            if(!Rooted||++RootedCount!=1||!Instances||Instances->GetInstanceCount()!=8
                ||!Instances->IsRegistered()||Instances->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
                ||!Instances->GetStaticMesh()||Instances->GetStaticMesh()->GetPathName()!=TEXT("/Game/Warfare/Field/V42/Meshes/SM_RootedApron_A_V42.SM_RootedApron_A_V42"))return false;
            if(const auto* Body=Instances->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82DistantMatte")))
        {
            const auto* Material=Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
            if(!Distant||++DistantCount!=1||!Component->IsRegistered()||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
                ||Component->CastShadow||Component->bReceivesDecals||!Component->GetComponentTransform().Equals(DistantPlacementV82(Id),.001)
                ||!Component->GetStaticMesh()||Component->GetStaticMesh()->GetPathName()!=TEXT("/Game/Warfare/Field/V12/Meshes/SM_PanoramaCard.SM_PanoramaCard")
                ||!Material||!Material->Parent||Material->Parent->GetPathName()!=DistantMaterialPathV82(Id))return false;
            if(const auto* Body=Component->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82FieldVegetation")))
        {
            const auto* Instances=Cast<UHierarchicalInstancedStaticMeshComponent>(Component);
            if(!Field||!Instances||Instances->GetInstanceCount()<=0||!Instances->IsRegistered()
                ||Instances->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||Instances->bReceivesDecals)return false;
            if(const auto* Body=Instances->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            ++VegetationCount;continue;
        }
        if(Component->ComponentHasTag(TEXT("FRRU_V82NativeRiver")))
        {
            if(!Wet||++RiverCount!=1||!Component->IsRegistered()
                ||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||Component->CastShadow
                ||Component->bReceivesDecals)return false;
            if(const auto* Body=Component->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
            const UMaterialInterface* Material=Component->GetMaterial(0);
            if(!Material||!Material->GetMaterial()->GetShadingModels().HasShadingModel(MSM_SingleLayerWater))return false;
            if(RedesignedSceneV82(Id))
            {
                if(!Component->GetRelativeTransform().Equals(FTransform::Identity,1.e-8))return false;
                continue;
            }
            for(const auto& Corner:{FVector(-50,-50,0),FVector(50,-50,0),FVector(50,50,0),FVector(-50,50,0)})
            {
                const FVector World=Component->GetComponentTransform().TransformPosition(Corner);
                if(!FMath::IsNearlyEqual(World.Z,-22.,.001)||!FMath::IsNearlyEqual(FMath::Abs(World.Y),12000.,.01)
                    ||(!FMath::IsNearlyEqual(World.X,240.,.01)&&!FMath::IsNearlyEqual(World.X,1920.,.01)))return false;
            }
            continue;
        }
        const bool Solid=Component->ComponentHasTag(TEXT("FRRU_V82Collision"));
        if(GermanIngressReviewV82(Id)&&Component->ComponentHasTag(TEXT("outsideBackdrop"))
            &&(!Component->GetStaticMesh()||Component->GetStaticMesh()->GetPathName()!=GermanIngressBackdropPathV82(Id)))return false;
        if((MatzParts&&Component->ComponentHasTag(TEXT("ground")))
            ||(Render&&((Render->Ground>0&&Component->ComponentHasTag(TEXT("ground")))||(Render->Solids>0&&Component->ComponentHasTag(TEXT("solids"))))))
        {if(Component->IsVisible()||Component->CastShadow)return false;}
        if(MatzParts&&Component->ComponentHasTag(TEXT("ground"))
            &&(Component->IsVisible()||Component->CastShadow))return false;
        if((Id==TEXT("fr.mort_homme.scene.v3")||RedesignedSceneV82(Id))&&(Component->ComponentHasTag(TEXT("ground"))||Component->ComponentHasTag(TEXT("outsideBackdrop"))))
        {
            const auto* Material=Component->GetMaterial(0);
            if(!Material||!Material->GetMaterial()||Material->GetMaterial()->GetPathName()!=(RedesignedSceneV82(Id)?WorldSurfaceBaseForSceneV82(Id):MortSurfacePath))return false;
            if(UseLegacyMalmaisonAdapterV82(Id)&&!WarfareFRRUMalmaisonSurfaceV82::AppliedTo(Component))return false;
            ++ReceiverSurfaces;
        }
        if(!Component->IsRegistered()||Component->GetCollisionEnabled()!=(Solid?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision))return false;
        if(Solid)
        {
            ++SolidCount;
            if(!Component->IsPhysicsStateCreated()||!Component->GetBodyInstance()||!Component->GetBodyInstance()->IsValidBodyInstance())return false;
        }
        if(Component->ComponentHasTag(TEXT("solids"))&&Component->GetStaticMesh()->GetBodySetup()->AggGeom.ConvexElems.Num()!=ExpectedUCX)return false;
    }
    return RenderPaths.Num()==(Render?Render->Ground+Render->Solids+Render->Dressing:0)&&SolidCount==2&&RiverCount==(Wet?1:0)&&VegetationCount==(Field?3:0)&&DistantCount==(Distant?1:0)&&RootedCount==(Rooted?1:0)
        &&DressingPartCount==(MatzParts?MatzLumenPartCountV82:0)
        &&GroundPartPaths.Num()==(MatzParts?MatzGroundPartCountV82:0)
        &&FaciesPaths.Num()==(Facies?MatzFaciesPartCountV82:0)
        &&ReceiverSurfaces==((Id==TEXT("fr.mort_homme.scene.v3")||RedesignedSceneV82(Id))?2:0);
}
bool LoadWater(const FString& Id,UStaticMesh*& Plane,UMaterial*& Water,UMaterial*& Suppressor,FString& Error)
{
    Plane=nullptr;Water=nullptr;Suppressor=nullptr;if(!WetScene(Id))return true;
    FString Mission;if(!MissionName(Id,Mission))return Reject(Error,TEXT("water scene identity differs"));Mission.ReplaceInline(TEXT("."),TEXT("_"));
    if(RedesignedSceneV82(Id))
    {
        const bool Family=WorldFamilySceneV82(Id);
        const FString Region=Family?TEXT("WorldFamilies/"):TEXT("WorldRedesign/");
        const FString WaterName=TEXT("M_Water_")+Mission+(Family?TEXT("_WF_V82"):TEXT("_WR_V82"));
        Water=LoadObject<UMaterial>(nullptr,*(TEXT("/Game/FRRUIntegration/FormalV82/Water/")+Region+WaterName+TEXT(".")+WaterName));
        const FString PlaneName=(Family?TEXT("SM_WF_"):TEXT("SM_WR_"))+Mission+TEXT("_water");
        Plane=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/")+Region+TEXT("Scenes/")+PlaneName+TEXT(".")+PlaneName));
        const FString TerrainWater=WarfareFRRUWorldTerrainV826::WaterMesh(Id);
        if(!TerrainWater.IsEmpty())
        {
            Plane=LoadObject<UStaticMesh>(nullptr,*TerrainWater);
            Water=LoadObject<UMaterial>(nullptr,*WarfareFRRUWorldTerrainV826::WaterMaterial(Id));
        }
        if(!Plane||!Water||!Water->GetShadingModels().HasShadingModel(MSM_SingleLayerWater)
            ||!Plane->GetRenderData()||Plane->GetRenderData()->LODResources.IsEmpty())return Reject(Error,TEXT("authored pool surface or SingleLayerWater material missing"));
        const auto* Body=Plane->GetBodySetup();
        if(!Body||Body->AggGeom.GetElementCount()!=0||Body->DefaultInstance.GetCollisionEnabled(false)!=ECollisionEnabled::NoCollision)
            return Reject(Error,TEXT("authored water added a collision body"));
        const auto* Native=NativeSceneV82(Id);const auto& LOD=Plane->GetRenderData()->LODResources[0];
        if(!Native||!LOD.VertexBuffers.ColorVertexBuffer.GetNumVertices())return Reject(Error,TEXT("authored water has no actual pool identity colours"));
        for(uint32 I=0;I<LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();++I)
        {
            const FVector P=AuthoredSceneToNative.TransformPosition(FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(I)));
            const uint8 Group=LOD.VertexBuffers.ColorVertexBuffer.VertexColor(I).R;bool Match=false;
            for(const auto& Patch:Native->waterPatches)if(Patch.groupIndex==Group&&FMath::IsNearlyEqual(P.Z,Patch.levelCm,.05)){Match=true;break;}
            if(!Match)return Reject(Error,TEXT("actual water height or vertex pool identity differs from native authoring"));
        }
        return ShaderReady(Water,Error);
    }
    const FString Name=TEXT("M_Water_")+Mission+TEXT("_V82");
    Water=LoadObject<UMaterial>(nullptr,*(TEXT("/Game/FRRUIntegration/FormalV82/Water/")+Name+TEXT(".")+Name));
    Suppressor=LoadObject<UMaterial>(nullptr,TEXT("/Game/FRRUIntegration/FormalV82/Water/M_SourceRiverSuppressed_V82.M_SourceRiverSuppressed_V82"));
    Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FRRUIntegration/FormalV82/Water/SM_NativeRiverPlane_V82.SM_NativeRiverPlane_V82"));
    if(!Plane||!Water||!Suppressor||!Water->GetShadingModels().HasShadingModel(MSM_SingleLayerWater)
        ||Suppressor->GetBlendMode()!=BLEND_Masked||Suppressor->OpacityMaskClipValue<=0)
        return Reject(Error,TEXT("native river candidate assets/shading contract missing"));
    const FBox Bounds=Plane->GetBoundingBox();
    if(!Bounds.Min.Equals(FVector(-50,-50,0),.001)||!Bounds.Max.Equals(FVector(50,50,0),.001))
        return Reject(Error,TEXT("actual receiver water plane differs from verified 100 cm flat source"));
    return ShaderReady(Water,Error)&&ShaderReady(Suppressor,Error);
}
}

namespace WarfareFRRUContentV82
{
FString WaterHeightTexturePathV82(const FString& SceneId)
{
    const FString TerrainHeight=WarfareFRRUWorldTerrainV826::WaterHeight(SceneId);
    if(!TerrainHeight.IsEmpty())return TerrainHeight;
    FString Mission;if(!WetScene(SceneId)||!MissionName(SceneId,Mission))return FString();
    Mission.ReplaceInline(TEXT("."),TEXT("_"));
    const bool Family=WorldFamilySceneV82(SceneId);
    const FString Name=TEXT("T_Height_")+Mission+(Family?TEXT("_WF_V82"):RedesignedSceneV82(SceneId)?TEXT("_WR_V82"):TEXT("_V82"));
    return FString(TEXT("/Game/FRRUIntegration/FormalV82/Water/"))+(Family?TEXT("WorldFamilies/"):RedesignedSceneV82(SceneId)?TEXT("WorldRedesign/"):TEXT(""))+Name+TEXT(".")+Name;
}
bool SceneAssetsReady(const FString& Id,int32 UCX,FString& Error)
{
    Error.Reset();TArray<UStaticMesh*> Meshes;UStaticMesh* Plane=nullptr;UMaterial* Water=nullptr;UMaterial* Suppressor=nullptr;
    UMaterialInterface* Surface=nullptr;UMaterialInterface* Distant=nullptr;
    return LoadScene(Id,UCX,Meshes,Error)&&LoadReceiverSurface(Id,Surface,Error)&&LoadWater(Id,Plane,Water,Suppressor,Error)&&LoadDistantV82(Id,Distant,Error);
}
#if WITH_EDITOR
bool StageMortPropProbe(AWarfareFrontlineGameMode* Battle,AActor*& Out,FString& Error)
{
    const auto* Native=wfexp::scene::ForMission("fr.mort_homme");
    if(!Native)return Reject(Error,TEXT("Mort native prop placement source missing"));
    UStaticMesh* Meshes[5]={};
    const TCHAR* Names[]={TEXT("SM_FiringBay_A_V76"),TEXT("SM_FiringBay_B_V76"),TEXT("SM_FiringBay_C_V76"),
        TEXT("SM_SplinteredTrunk_A_V76"),TEXT("SM_SplinteredTrunk_B_V76")};
    for(int32 I=0;I<5;++I)
    {
        Meshes[I]=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V76/Meshes/%s.%s"),Names[I],Names[I]));
        if(!Meshes[I]||!Meshes[I]->GetRenderData())return Reject(Error,TEXT("retained Mort prop mesh missing"));
        for(const auto& Slot:Meshes[I]->GetStaticMaterials())if(!ShaderReady(Slot.MaterialInterface,Error))return false;
    }
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;
    Out=Battle->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Params);
    if(!Out)return Reject(Error,TEXT("Mort cosmetic owner spawn failed"));
    Out->Tags.Add(TEXT("FRRU_V82MortPropProbe"));
    auto* Root=NewObject<USceneComponent>(Out,TEXT("MortCosmeticRoot"),RF_Transient);
    Out->AddInstanceComponent(Root);Out->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
    Out->SetActorTransform(AuthoredSceneToNative);
    UHierarchicalInstancedStaticMeshComponent* Instances[5]={};
    for(int32 I=0;I<5;++I)
    {
        auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Out,FName(Names[I]),RF_Transient);
        Out->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Meshes[I]);
        C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();Instances[I]=C;
    }
    const auto Add=[&](int32 I,const FVector& NativePosition,const FQuat& NativeRotation,const FVector& Scale)
    {
        // Receiver prop packages already use the host basis. Convert the
        // complete world transform into the unchanged authored scene root once.
        const FTransform World(NativeRotation,NativePosition,Scale);
        Instances[I]->AddInstance(World.GetRelativeTransform(AuthoredSceneToNative));
    };
    FRandomStream Random(2220393811u);
    for(int32 T=0;T<2;++T)for(int32 Side:{-1,1})for(int32 Bay=0;Bay<12;++Bay)
    {
        if((Bay+T+Side+3)%7==0)continue;
        const float X=(T==0?-6600.f:5040.f)+Side*235.f;
        const float Y=-5250.f+Bay*950.f+Random.FRandRange(-60,60);
        float H=0,Before=0,After=0,HX0=0,HX1=0;
        if(!WarfareFRRUWorldV82::TryHeight(Battle,X,Y,H)||!WarfareFRRUWorldV82::TryHeight(Battle,X,Y-70,Before)
            ||!WarfareFRRUWorldV82::TryHeight(Battle,X,Y+70,After)||!WarfareFRRUWorldV82::TryHeight(Battle,X-28,Y,HX0)
            ||!WarfareFRRUWorldV82::TryHeight(Battle,X+28,Y,HX1))continue;
        const int32 I=(Bay+T+(Side>0?1:0))%3;
        const FVector Normal(-FMath::Clamp((HX1-HX0)/56.f,-.26f,.26f),-FMath::Clamp((After-Before)/140.f,-.26f,.26f),1);
        const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal.GetSafeNormal())
            *FQuat(FVector::UpVector,FMath::DegreesToRadians((Side<0?180.f:0.f)+Random.FRandRange(-7,7)));
        const float Bottom=Q.RotateVector(FVector(0,0,Meshes[I]->GetBoundingBox().Min.Z)).Z;
        Add(I,FVector(X,Y,(H*2+Before+After)*.25f-Bottom-4),Q,FVector::OneVector);
    }
    // The full prop box contains projecting roots and branches. Calibrate the
    // visible continuous stem against the unchanged native cylinder instead.
    // These source-pinned fits cover the continuous 100..200 cm eye-height
    // band for all19 Mort trunks, including either source quad triangulation.
    // They do not assert pixel-identical blocking through the fractured crown.
    struct FStemFit{double XY,PX,PY,Z,MinimumRadius;};
    static const FStemFit StemFits[]={
        {2.771553757,-4.597868017,-.040065588,1.086774279,18.678636779},
        {2.232085469,2.508594465,.018137200,1.147838899,20.096717208},
        {2.265645669,-2.175228397,1.064264416,1.580222741,22.336340562},
        {1.659148376,1.042993248,-1.030055274,1.780892334,22.560417514},
        {1.789805029,-1.817730342,1.143108255,1.728786209,22.790794569},
        {2.456711114,1.337202878,-.899140328,1.572531035,22.025586802},
        {1.821707908,-3.022153137,.760428233,1.340594824,20.681766543},
        {2.064581467,1.053226880,-1.026979801,1.772070752,22.566024302},
        {2.340305053,-2.053239717,1.094702870,1.626669173,22.552292992},
        {1.450534869,1.785224040,-.601555945,1.360674779,21.047131445},
        {2.458182071,-3.756378914,.412010775,1.202358323,19.675193101},
        {1.978980392,2.046472652,-.391149743,1.271013067,20.418039860},
        {2.655334742,-4.316431766,.115507342,1.121605663,19.016718709},
        {2.095217424,2.588319168,.091977972,1.130064560,20.014564962},
        {1.676128082,-2.680059350,.899288901,1.423566904,21.312974709},
        {1.520597491,1.228618101,-.954827032,1.640604894,22.170059859},
        {1.577750220,-3.118143673,.718729573,1.319727645,20.522942973},
        {1.946498785,2.334183691,-.140700146,1.190000185,20.130083510},
        {1.727155198,-4.873159445,-.194645112,1.055624935,18.351456912}};
    int32 TreeCount=0;
    for(const auto& Solid:Native->solids)if(Solid.kind&&std::string_view(Solid.kind)=="tree_trunk")
    {
        const int32 I=3+TreeCount%2;const FBox Bounds=Meshes[I]->GetBoundingBox();const FVector Size=Bounds.GetSize();
        const float Diameter=float(Solid.maximum.x-Solid.minimum.x),Height=float(Solid.maximum.z-Solid.minimum.z);
        if(Size.Z<=0||Diameter<=0||Height<=0||TreeCount>=UE_ARRAY_COUNT(StemFits))
        {Out->Destroy();Out=nullptr;return Reject(Error,TEXT("Mort tree extent invalid"));}
        const auto& Fit=StemFits[TreeCount];
        if(!FMath::IsNearlyEqual(double(Height)/Size.Z,Fit.Z,1.e-5)
            ||!FMath::IsNearlyEqual(Fit.XY*Fit.MinimumRadius,double(Diameter)*.5+.5,.002))
        {Out->Destroy();Out=nullptr;return Reject(Error,TEXT("Mort source-pinned stem fit differs from actual native/mesh extents"));}
        const FVector Scale(Fit.XY,Fit.XY,Height/Size.Z);
        const FQuat Q(FVector::UpVector,FMath::DegreesToRadians(Random.FRandRange(0,360)));
        const FVector Offset=Q.RotateVector(FVector(Fit.PX*Scale.X,Fit.PY*Scale.Y,Bounds.Min.Z*Scale.Z));
        Add(I,FVector((Solid.minimum.x+Solid.maximum.x)*.5,(Solid.minimum.y+Solid.maximum.y)*.5,Solid.minimum.z)-Offset,Q,Scale);
        ++TreeCount;
    }
    if(TreeCount!=19){Out->Destroy();Out=nullptr;return Reject(Error,TEXT("Mort exact19 collision carriers differ"));}
    return true;
}
#endif
#include "FRRU/FRRUVauxTripoRuinV87.inl"
bool StageScene(AWarfareFrontlineGameMode* Battle,const FString& Id,const FTransform& Transform,int32 UCX,FString& Error)
{
    Error.Reset();
    if(!Battle||!Battle->GetWorld()||!WarfareFRRUCampaignV82::Active(Battle))return Reject(Error,TEXT("no active national scene world"));
    if(!Transform.Equals(FTransform::Identity,1.e-8))return Reject(Error,TEXT("native scene placement requires identity; fixed verified exchange conversion is receiver-owned"));
    FString NativeId;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
    if(!WarfareFRRUWorldV82::SceneInfo(Battle,NativeId,Revision,Solids,Minimum,Maximum)||NativeId!=Id||Solids!=UCX)
        return Reject(Error,TEXT("stage scene ID/revision/solid count is not the selected native scene"));
    TArray<UStaticMesh*> Meshes;if(!LoadScene(Id,UCX,Meshes,Error))return false;
    const auto* Render=WarfareFRRUWorldRenderBindingV87::Find(Id);
    TArray<UStaticMesh*> RenderMeshes[3];
    if(Render)for(int32 Section=0;Section<3;++Section)
        for(int32 I=Section==2?1:0;I<WarfareFRRUWorldRenderBindingV87::Count(Render,Section);++I)
        {
            auto* Mesh=LoadObject<UStaticMesh>(nullptr,*WarfareFRRUWorldRenderBindingV87::Path(Render,Section,I));
            if(!ValidateMesh(Mesh,2,0,Error,false,true))return false;
            RenderMeshes[Section].Add(Mesh);
        }
    TArray<UStaticMesh*> MatzParts;
    TArray<UStaticMesh*> MatzGroundParts;
    if(!Render&&Id==TEXT("fr.matz.scene.families.v1"))
        for(int32 I=1;I<MatzLumenPartCountV82;++I)
        {
            auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MatzLumenPartPathV82(I));
            if(!ValidateMesh(Mesh,2,0,Error,false,true))return false;
            MatzParts.Add(Mesh);
        }
    if(!Render&&Id==TEXT("fr.matz.scene.families.v1"))
        for(int32 I=0;I<MatzGroundPartCountV82;++I)
        {
            auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MatzGroundPartPathV82(I));
            if(!ValidateMesh(Mesh,2,0,Error,false,true))return false;
            MatzGroundParts.Add(Mesh);
        }
    UMaterialInterface* ReceiverSurface=nullptr;if(!LoadReceiverSurface(Id,ReceiverSurface,Error))return false;
    UStaticMesh* Plane=nullptr;UMaterial* Water=nullptr;UMaterial* Suppressor=nullptr;
    if(!LoadWater(Id,Plane,Water,Suppressor,Error))return false;
    const FBox Bounds=Meshes[0]->GetBoundingBox();
    if(!FMath::IsNearlyEqual(Bounds.Min.X,Minimum.X,.2)||!FMath::IsNearlyEqual(Bounds.Min.Y,Minimum.Y,.2)
        ||!FMath::IsNearlyEqual(Bounds.Max.X,Maximum.X,.2)||!FMath::IsNearlyEqual(Bounds.Max.Y,Maximum.Y,.2))
        return Reject(Error,TEXT("imported ground bounds do not match the native centimetre basis"));
    const TWeakObjectPtr<const AWarfareFrontlineGameMode> Key(Battle);
    if(const FStagedScene* Existing=Scenes.Find(Key))
        if(Existing->SceneId==Id&&Existing->Revision==Revision&&Existing->UCX==UCX&&ComponentsValid(Existing->Owner.Get(),UCX,Id))
        {
            auto* Response=Existing->Owner->FindComponentByClass<UFrontlineWaterResponseV51>();
            if(!WetScene(Id)||(Response&&Response->IsNativeBindingCurrentV82()))return true;
        }
    FActorSpawnParameters Parameters;Parameters.ObjectFlags|=RF_Transient;
    AActor* Owner=Battle->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),AuthoredSceneToNative,Parameters);
    if(!Owner)return Reject(Error,TEXT("scene candidate actor spawn failed"));
    Owner->Tags.Add(TEXT("FRRU_V82Scene"));
    if(WetScene(Id))Owner->Tags.Add(TEXT("FRRU_V82WetScene"));
    auto* Root=NewObject<USceneComponent>(Owner,TEXT("FRRU_V82Root"),RF_Transient);
    Owner->AddInstanceComponent(Root);Owner->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
    // A plain AActor has no root at SpawnActor time. Apply the verified
    // exchange basis after installing its root so render and physics inherit
    // the same transform, rather than relying on a discarded spawn transform.
    Owner->SetActorTransform(AuthoredSceneToNative,false,nullptr,ETeleportType::TeleportPhysics);
    UStaticMeshComponent* Ground=nullptr;
    UMaterialInterface* SoilProbe=nullptr;
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82TerrainProbe")))
    {
        if(Id!=TEXT("fr.mort_homme.scene.v2")||!FApp::IsUnattended()
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview")))
        {Owner->Destroy();return Reject(Error,TEXT("soil candidate requires the isolated unattended Mort no-save review"));}
        SoilProbe=LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/FRRUIntegration/FormalV82/Probes/MortSoil/M_MortPairedSoil_V82.M_MortPairedSoil_V82"));
        if(!ShaderReady(SoilProbe,Error)){Owner->Destroy();return false;}
        auto* Dressing=LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/FRRUIntegration/FormalV82/Probes/MortSoil/SM_MortDressing_Refined_V82.SM_MortDressing_Refined_V82"));
        if(!Dressing||!Dressing->GetRenderData()){Owner->Destroy();return Reject(Error,TEXT("exact cosmetic derivative missing"));}
        for(const auto& Slot:Dressing->GetStaticMaterials())if(!ShaderReady(Slot.MaterialInterface,Error)){Owner->Destroy();return false;}
        Meshes[2]=Dressing;
    }
#endif
    int32 Suppressed=0;
    for(int32 I=0;I<4;++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(Parts[I]),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(Meshes[I]);C->ComponentTags.Add(FName(Parts[I]));
        if(I==2&&Render&&Render->Dressing>0)C->ComponentTags.Add(TEXT("FRRU_V826WorldPart"));
        else if(I==2&&Id==TEXT("fr.matz.scene.families.v1"))C->ComponentTags.Add(TEXT("FRRU_V82MatzLumenPart"));
        if(ReceiverSurface&&(I==0||I==3))
        {
            C->SetMaterial(0,ReceiverSurface);
            UMaterialInstanceDynamic* SurfaceMID=nullptr;
            if(WorldSurfaceCandidateV826(Id))
            {
                SurfaceMID=CreateWorldSurfaceInstanceV826(Id,C,ReceiverSurface,Error);
                if(!SurfaceMID){Owner->Destroy();return false;}
            }
            else if(UseLegacyMalmaisonAdapterV82(Id))
            {
                const auto* Native=NativeSceneV82(Id);
                if(!Native||!Native->waterPatches.empty())
                {Owner->Destroy();return Reject(Error,TEXT("Malmaison surface adapter requires its unchanged dry native scene"));}
                SurfaceMID=WarfareFRRUMalmaisonSurfaceV82::Apply(Id,C,ReceiverSurface,Error);
                if(!SurfaceMID){Owner->Destroy();return false;}
            }
            else if(Id==TEXT("fr.matz.scene.families.v1")&&MatzFaciesReviewV82())
            {
                auto* Projected=UFrontlineTerrainMaterialV47::CreateForGround(ReceiverSurface,C);
                if(!Projected){Owner->Destroy();return Reject(Error,TEXT("facies sampling instance missing"));}
                TArray<UTexture*> Used;ReceiverSurface->GetUsedTextures(Used,GetCachedScalabilityCVars().MaterialQualityLevel,GMaxRHIShaderPlatform);
                TArray<FMaterialTextureInfo> Entries;
                float SoilProjection=.52f,DepositProjection=.725f;
                ReceiverSurface->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SoilProjectionScaleV47")),SoilProjection);
                ReceiverSurface->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DepositProjectionScaleV51")),DepositProjection);
                bool FaciesFound=false;
                for(UTexture* Texture:Used)if(Cast<UTexture2D>(Texture))
                {
                    const FString Path=Texture->GetPathName();float Scale=0.f;
                    if(Path.StartsWith(TEXT("/Game/Warfare/Field/V23/Textures/T_Mud_"))
                        ||Path.StartsWith(TEXT("/Game/Warfare/Field/V47/Textures/T_Mud_HeightAO_")))Scale=(200.f/145.f)*SoilProjection;
                    else if(Path.StartsWith(TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_")))Scale=(200.f/145.f)*DepositProjection;
                    else if(Path.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_")))Scale=200.f/251.f;
                    else if(Path.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/MatzFacies/AppearanceV1/Textures/T_MatzAppearanceFacies_V82")))
                    {Scale=200.f/24000.f;FaciesFound=true;}
                    if(Scale<=0||!FMath::IsFinite(Scale)){Owner->Destroy();return Reject(Error,TEXT("unregistered facies texture sampling scale"));}
                    FMaterialTextureInfo Entry;Entry.TextureName=Texture->GetFName();Entry.UVChannelIndex=0;Entry.SamplingScale=Scale;
#if WITH_EDITORONLY_DATA
                    Entry.TextureReference=FSoftObjectPath(Texture);
#endif
                    Entries.Add(Entry);
                }
                if(!FaciesFound){Owner->Destroy();return Reject(Error,TEXT("facies texture sampling binding missing"));}
                Projected->SetWorldSampling(Entries);C->SetMaterial(0,Projected);SurfaceMID=Projected;
            }
            else SurfaceMID=C->CreateDynamicMaterialInstance(0,ReceiverSurface);
            if(auto* MID=SurfaceMID)
            {
                const auto* Mission=WarfareFRRUCampaignV82::EncounterMission(Battle);
                if(!Mission)Mission=WarfareFRRUCampaignV82::SelectedMission(Battle);
                const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,0);
                MID->SetScalarParameterValue(TEXT("ChalkExposure"),Palette.Chalk);
                if(!PreserveAuthoredSoilV826(Id))MID->SetVectorParameterValue(TEXT("SoilTint"),Palette.Soil);
                MID->SetVectorParameterValue(TEXT("HorizonTint"),Palette.Horizon);
                MID->SetScalarParameterValue(TEXT("GrassCoverage"),Palette.Grass);
                MID->SetScalarParameterValue(TEXT("BallastCoverage"),Palette.Ballast);
                // National operation IDs remain their own stable IDs. In
                // particular, they must not enable Menin's seed-7 road shader.
                MID->SetScalarParameterValue(TEXT("FieldSeed"),UseLegacyMalmaisonAdapterV82(Id)?1005.f:float(Mission?Mission->Id:1000));
                if(const auto* Recipe=WorldSurfaceCandidateV826(Id))
                {
                    // Native R/B carry paired mineral and woodland/field
                    // cover. Legacy palette colour overlays would fake grass
                    // where the authored B channel is zero.
                    for(const TCHAR* Name:{TEXT("GrassCoverage"),TEXT("ChalkExposure"),TEXT("BallastCoverage"),TEXT("SnowCover")})
                        MID->SetScalarParameterValue(Name,0.f);
                    MID->SetScalarParameterValue(TEXT("FieldSeed"),Recipe->FieldSeed);
                }
            }
        }
        if(I==0&&SoilProbe)C->SetMaterial(0,SoilProbe);
        if(Water&&Suppressor&&I>=2)
        {
            FString Mission;MissionName(Id,Mission);Mission.ReplaceInline(TEXT("."),TEXT("_"));
            const FString Expected=TEXT("M_Terrain_R3_Water_")+Mission;
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
                if(const auto* Original=C->GetMaterial(Slot);Original&&Original->GetMaterial()->GetName()==Expected)
                {C->SetMaterial(Slot,Suppressor);++Suppressed;}
        }
        C->SetCollisionProfileName(I<2?TEXT("BlockAll"):TEXT("NoCollision"));
        C->SetCollisionEnabled(I<2?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
        if(I<2)C->ComponentTags.Add(TEXT("FRRU_V82Collision"));
        if((Render&&((I==0&&Render->Ground>0)||(I==1&&Render->Solids>0)))||(I==0&&!MatzGroundParts.IsEmpty()))
        {
            // The original exact complex mesh remains the only ground body.
            // Identical render triangles are partitioned solely for Lumen.
            C->SetVisibility(false);C->SetHiddenInGame(true);C->SetCastShadow(false);
        }
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();
        if(I==0)Ground=C;
    }
    UMaterialInterface* ShoulderV826=nullptr;
    if(const auto* Recipe=WorldSurfaceCandidateV826(Id))
    {
        ShoulderV826=LoadObject<UMaterialInterface>(nullptr,Recipe->ShoulderInstance);
        if(!ShaderReady(ShoulderV826,Error)){Owner->Destroy();return false;}
    }
    if(Render)for(int32 Section=0;Section<3;++Section)for(int32 I=0;I<RenderMeshes[Section].Num();++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("WorldRenderV826_%d_%03d"),Section,I)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(RenderMeshes[Section][I]);C->ComponentTags.Add(TEXT("FRRU_V826WorldPart"));
        if(Section==0)
        {
            if(UseLegacyMalmaisonAdapterV82(Id))
            {
                if(!WarfareFRRUMalmaisonSurfaceV82::Apply(Id,C,ReceiverSurface,Error)){Owner->Destroy();return false;}
            }
            else C->SetMaterial(0,Ground->GetMaterial(0));
        }
        else if(ShoulderV826)
        {
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
            {
                const auto* Original=C->GetMaterial(Slot);
                if(!Original||Original->GetPathName()!=TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42.MI_EarthBank_V42"))continue;
                auto* MID=C->CreateDynamicMaterialInstance(Slot,ShoulderV826);
                const auto* Mission=WarfareFRRUCampaignV82::EncounterMission(Battle);
                if(!Mission)Mission=WarfareFRRUCampaignV82::SelectedMission(Battle);
                const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,0);
                if(!MID){Owner->Destroy();return Reject(Error,TEXT("paired world shoulder instance missing"));}
                if(!PreserveAuthoredSoilV826(Id))MID->SetVectorParameterValue(TEXT("SoilTint"),Palette.Soil);
                MID->SetScalarParameterValue(TEXT("ChalkExposure"),0.f);
            }
        }
        C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();
    }
    for(int32 I=0;I<MatzParts.Num();++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("MatzLumenPart%03d"),I+1)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(MatzParts[I]);C->ComponentTags.Add(TEXT("FRRU_V82MatzLumenPart"));
        C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();
    }
    UStaticMeshComponent* River=nullptr;
    for(int32 I=0;I<MatzGroundParts.Num();++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("MatzGroundPart%03d"),I)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(MatzGroundParts[I]);C->SetMaterial(0,Ground->GetMaterial(0));
        C->ComponentTags.Add(TEXT("FRRU_V82MatzGroundPart"));C->SetCollisionProfileName(TEXT("NoCollision"));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);
        C->SetCanEverAffectNavigation(false);C->RegisterComponent();
    }
    if(Id==TEXT("fr.matz.scene.families.v1")&&MatzFaciesReviewV82())for(int32 I=0;I<MatzFaciesPartCountV82;++I)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MatzFaciesPartPathV82(I));
        const auto* Body=Mesh?Mesh->GetBodySetup():nullptr;
        if(!Mesh||!Mesh->GetRenderData()||!Body||Body->AggGeom.GetElementCount()!=0)
        {Owner->Destroy();return Reject(Error,TEXT("Matz appearance group has invalid cosmetic geometry"));}
        for(const auto& Slot:Mesh->GetStaticMaterials())if(!ShaderReady(Slot.MaterialInterface,Error)){Owner->Destroy();return false;}
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("MatzFaciesPart%03d"),I)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(Mesh);C->ComponentTags.Add(TEXT("FRRU_V82MatzFaciesPart"));
        C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();
    }
    if(Water)
    {
        if(Suppressed!=(RedesignedSceneV82(Id)?0:2)){Owner->Destroy();return Reject(Error,TEXT("native water source slot contract differs"));}
        River=NewObject<UStaticMeshComponent>(Owner,TEXT("FRRU_V82NativeRiver"),RF_Transient);
        Owner->AddInstanceComponent(River);River->SetupAttachment(Root);River->SetMobility(EComponentMobility::Movable);
        River->SetStaticMesh(Plane);River->SetMaterial(0,Water);River->ComponentTags.Add(TEXT("FRRU_V82NativeRiver"));
        if(!RedesignedSceneV82(Id)){River->SetRelativeLocation(FVector(1080,0,-22));River->SetRelativeScale3D(FVector(16.8,240,1));}
        River->SetCollisionProfileName(TEXT("NoCollision"));River->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        River->SetCastShadow(false);River->SetReceivesDecals(false);River->SetGenerateOverlapEvents(false);
        River->SetCanEverAffectNavigation(false);River->RegisterComponent();
    }
    if(!StageFieldVegetationV82(Owner,Root,Meshes[0],Id,Error)){Owner->Destroy();return false;}
    if(!StageRootedShouldersV82(Owner,Root,Id,Error)){Owner->Destroy();return false;}
    if(!WarfareFRRUWorldWoodlineV826::Stage(Owner,Root,Id,Error)){Owner->Destroy();return false;}
    if(!WarfareFRRUBlastedWoodlineV826::Stage(Owner,Root,Id,Error)){Owner->Destroy();return false;}
    if(!WarfareFRRUWorldDressingV827::Stage(Owner,Root,Id,Error,ShaderReady)){Owner->Destroy();return false;}
    if(!WarfareFRRUFieldDetailV87::Stage(Owner,Root,Id,Error,ShaderReady)){Owner->Destroy();return false;}
    if(!WarfareFRRUVauxCompositionV87::Stage(Owner,Root,Id,Error,ShaderReady)){Owner->Destroy();return false;}
    if(!FRRUMortFlankFireStepV87::Stage(Battle,Owner,Root,Id,Error,ShaderReady)){Owner->Destroy();return false;}
    if(!FRRUVauxTripoReleaseLoaderV87::Stage(Battle,Owner,Root,Id,Error,ShaderReady)){Owner->Destroy();return false;}
    UStaticMeshComponent* Distant=nullptr;
    if(!StageDistantV82(Owner,Root,Id,Distant,Error)){Owner->Destroy();return false;}
    if(!ComponentsValid(Owner,UCX,Id))
    {Owner->Destroy();return Reject(Error,TEXT("actual registered scene collision bodies failed"));}
    // Direct component trace cannot be satisfied by another scene/host floor.
    // Native decks/solids/LOS and the complete interpolated grid are reviewed
    // separately by the host; this confirms the actual new ground is queryable.
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82GroundProbe),true);
    const FVector2D Centre=(Minimum+Maximum)*.5;
    if(!Ground||!Ground->LineTraceComponent(Hit,FVector(Centre.X,Centre.Y,10000),FVector(Centre.X,Centre.Y,-10000),Query))
    {Owner->Destroy();return Reject(Error,TEXT("new ground has no actual complex collision trace"));}
    AActor* CosmeticProbe=nullptr;
    TSharedPtr<FJsonObject> TripoEvidenceV87;
    if(!FRRUVauxHQ27ReleaseLoaderV87::Stage(Battle,Owner,Id,CosmeticProbe,Error,ShaderReady,&TripoEvidenceV87))
    {if(CosmeticProbe)CosmeticProbe->Destroy();Owner->Destroy();return false;}
#if WITH_EDITOR
    if(FRRUVauxStreetRubbleV87::Requested()&&!FRRUVauxStreetRubbleV87::Stage(Battle,Owner,CosmeticProbe,Id,Error,ShaderReady,&TripoEvidenceV87))
    {FRRUVauxHQ27ReleaseLoaderV87::Forget(Owner);if(CosmeticProbe)CosmeticProbe->Destroy();Owner->Destroy();return false;}
    if(SoilProbe&&!StageMortPropProbe(Battle,CosmeticProbe,Error)){Owner->Destroy();return false;}
    if(VauxTripoRequestedV87(Id)&&!StageVauxTripoRuinV87(Battle,Owner,CosmeticProbe,TripoEvidenceV87,Error))
    {if(CosmeticProbe)CosmeticProbe->Destroy();Owner->Destroy();return false;}
    if(FRRUVauxHQ27VisualV87::Requested()&&!FRRUVauxHQ27VisualV87::Stage(Battle,Owner,Id,CosmeticProbe,TripoEvidenceV87,Error,ShaderReady))
    {if(CosmeticProbe)CosmeticProbe->Destroy();Owner->Destroy();return false;}
#endif
    ClearScene(Battle);Scenes.Add(Key,{Owner,Id,Revision,UCX,CosmeticProbe,Distant});
#if WITH_EDITOR
    Scenes.FindChecked(Key).CosmeticEvidenceV87=TripoEvidenceV87;
#endif
    if(River&&!UFrontlineWaterResponseV51::BindNativeV82(Owner,{{River,0,Water}},Id,Revision))
    {ClearScene(Battle);return Reject(Error,TEXT("native river response could not bind its actual displayed surface"));}
#if WITH_EDITOR
    if(FRRUVehicleNavCandidateV87::Requested())
    {
        const auto Navigation=FRRUVehicleNavCandidateV87::Begin(Battle,Owner);
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_FT_NAV_SCENE_BEGIN status=%d reason=%s path=%s"),int32(Navigation.Status),*Navigation.Reason,*Navigation.Path);
    }
    else
#endif
    if(FRRUVehicleNavV87::EligibleScene(Battle))FRRUVehicleNavV87::Begin(Battle,Owner);
    return true;
}
bool SceneStaged(const AWarfareFrontlineGameMode* Battle,const FString& Id,uint32 Revision)
{
    auto* Scene=Scenes.Find(TWeakObjectPtr<const AWarfareFrontlineGameMode>(Battle));
    if(!Scene||Scene->SceneId!=Id||Scene->Revision!=Revision||!Scene->Owner.IsValid()
        ||!Scene->Owner->GetActorTransform().Equals(AuthoredSceneToNative,1.e-8))return false;
    // StageScene/ClearScene own the immutable render/collision collection and
    // replace this record on every transition. Units, weapons and water may
    // query readiness hundreds of times in one frame; share its full audit.
    // Keep the live owner/transform checks above even on a cached result.
    if(Scene->ValidationFrame==GFrameCounter)return Scene->ValidationResult;
#if WITH_EDITOR
    const double Started=FPlatformTime::Seconds();
#endif
    const bool Valid=ComponentsValid(Scene->Owner.Get(),Scene->UCX,Id)
        &&(!FRRUVauxHQ27ReleaseLoaderV87::Enabled(Id)||FRRUVauxHQ27ReleaseLoaderV87::Validate(Scene->Owner.Get(),Id));
    Scene->ValidationFrame=GFrameCounter;Scene->ValidationResult=Valid;
#if WITH_EDITOR
    ++Scene->ValidationCalls;Scene->ValidationMilliseconds+=(FPlatformTime::Seconds()-Started)*1000.;
#endif
    return Valid;
}
bool SceneStagedForWater(const AWarfareFrontlineGameMode* Battle,const FString& Id,uint32 Revision)
{
    auto* Scene=Scenes.Find(TWeakObjectPtr<const AWarfareFrontlineGameMode>(Battle));
    if(!Scene||Scene->SceneId!=Id||Scene->Revision!=Revision||!Scene->Owner.IsValid()
        ||!Scene->Owner->GetActorTransform().Equals(AuthoredSceneToNative,1.e-8))return false;
    // The complete field is owned by StageScene/ClearScene, which replace this
    // record. Explicit readiness checks also refresh it, including failures.
    return Scene->ValidationFrame==GFrameCounter?Scene->ValidationResult:SceneStaged(Battle,Id,Revision);
}
TSharedPtr<FJsonObject> DescribeSceneRendering(const AWarfareFrontlineGameMode* Battle)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("artAccepted"),false);
    Result->SetBoolField(TEXT("renderingStateRead"),false);
    const auto* Scene=Scenes.Find(TWeakObjectPtr<const AWarfareFrontlineGameMode>(Battle));
    if(!Battle||!Battle->GetWorld()||!Scene||!Scene->Owner.IsValid())return Result;
    Result->SetStringField(TEXT("sceneId"),Scene->SceneId);
#if WITH_EDITOR
    Result->SetNumberField(TEXT("sceneValidationCalls"),double(Scene->ValidationCalls));
    Result->SetNumberField(TEXT("sceneValidationMilliseconds"),Scene->ValidationMilliseconds);
    if(Scene->CosmeticEvidenceV87.IsValid())Result->SetObjectField(TEXT("singleTripoRuinComparisonV87"),Scene->CosmeticEvidenceV87);
#endif
    Result->SetNumberField(TEXT("hostElapsed"),Battle->Elapsed);
    TArray<UStaticMeshComponent*> Components;Scene->Owner->GetComponents(Components);
    if(AActor* Props=Scene->CosmeticProbe.Get())
    {TArray<UStaticMeshComponent*> Extra;Props->GetComponents(Extra);Components.Append(Extra);}
    TArray<TSharedPtr<FJsonValue>> RenderedParts;
    for(const auto* Component:Components)
    {
        if(!Component)continue;auto Part=MakeShared<FJsonObject>();
        Part->SetStringField(TEXT("component"),Component->GetName());
        Part->SetStringField(TEXT("mesh"),GetPathNameSafe(Component->GetStaticMesh()));
        Part->SetStringField(TEXT("transform"),Component->GetComponentTransform().ToHumanReadableString());
        Part->SetBoolField(TEXT("visible"),Component->IsVisible());Part->SetBoolField(TEXT("castsShadow"),Component->CastShadow);
        if(UseLegacyMalmaisonAdapterV82(Scene->SceneId)
            &&(Component->ComponentHasTag(TEXT("ground"))||Component->ComponentHasTag(TEXT("outsideBackdrop"))))
            Part->SetObjectField(TEXT("malmaisonV76Surface"),WarfareFRRUMalmaisonSurfaceV82::Describe(Component));
        const auto* MeshUV=Component->GetStaticMesh()?Component->GetStaticMesh()->GetUVChannelData(0):nullptr;
        Part->SetBoolField(TEXT("uvDensityInitialized"),MeshUV&&MeshUV->IsInitialized());
        if(MeshUV)Part->SetNumberField(TEXT("worldCmPerUV0"),MeshUV->LocalUVDensities[0]);
        Part->SetNumberField(TEXT("streamingDistanceMultiplier"),Component->StreamingDistanceMultiplier);
        FStreamingTextureLevelContext StreamContext(EMaterialQualityLevel::High,Battle->GetWorld()->GetFeatureLevel(),false);
        TArray<FStreamingRenderAssetPrimitiveInfo> StreamInfos;Component->GetStreamingRenderAssetInfo(StreamContext,StreamInfos);
        TArray<TSharedPtr<FJsonValue>> StreamRows;
        for(const auto& Info:StreamInfos)
        {auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("asset"),GetPathNameSafe(Info.RenderAsset));Row->SetNumberField(TEXT("texelFactor"),Info.TexelFactor);StreamRows.Add(MakeShared<FJsonValueObject>(Row));}
        Part->SetArrayField(TEXT("streamingRecords"),StreamRows);
        TArray<TSharedPtr<FJsonValue>> Materials;
        for(int32 Slot=0;Slot<Component->GetNumMaterials();++Slot)
        {
            const UMaterialInterface* Material=Component->GetMaterial(Slot);auto Entry=MakeShared<FJsonObject>();
            Entry->SetNumberField(TEXT("slot"),Slot);Entry->SetStringField(TEXT("material"),GetPathNameSafe(Material));
            float ActualWetness=0.f;
            const bool HasWetness=Material&&Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),ActualWetness);
            Entry->SetBoolField(TEXT("weatherWetnessParameter"),HasWetness);
            if(HasWetness)Entry->SetNumberField(TEXT("weatherWetness"),ActualWetness);
            auto Parameters=MakeShared<FJsonObject>();
            for(const TCHAR* Name:{TEXT("FieldSeed"),TEXT("GrassCoverage"),TEXT("ChalkExposure"),TEXT("BallastCoverage"),TEXT("SnowCover")})
            {
                float Value=0.f;
                if(Material&&Material->GetScalarParameterValue(FMaterialParameterInfo(Name),Value))Parameters->SetNumberField(Name,Value);
            }
            Entry->SetObjectField(TEXT("surfaceParameters"),Parameters);
            FLinearColor SoilTint;
            if(Material&&Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),SoilTint))
            {
                auto Tint=MakeShared<FJsonObject>();Tint->SetNumberField(TEXT("r"),SoilTint.R);
                Tint->SetNumberField(TEXT("g"),SoilTint.G);Tint->SetNumberField(TEXT("b"),SoilTint.B);
                Tint->SetNumberField(TEXT("a"),SoilTint.A);Entry->SetObjectField(TEXT("actualSoilTint"),Tint);
            }
            const FMaterialResource* Resource=Material?Material->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
            const FMaterialShaderMap* Map=Resource?Resource->GetGameThreadShaderMap():nullptr;
            Entry->SetBoolField(TEXT("shaderReady"),Resource&&Resource->IsGameThreadShaderMapComplete()&&Map&&Map->IsValidForRendering());
            TArray<UTexture*> Used;TArray<TSharedPtr<FJsonValue>> Textures;
            if(Material)Material->GetUsedTextures(Used,GetCachedScalabilityCVars().MaterialQualityLevel,GMaxRHIShaderPlatform);
            for(const auto* Texture:Used)if(const auto* Texture2D=Cast<UTexture2D>(Texture))
            {
                auto Tex=MakeShared<FJsonObject>();const int32 Total=Texture2D->GetNumMips(),Resident=Texture2D->GetNumResidentMips();
                const int32 First=FMath::Clamp(Total-Resident,0,30);
                Tex->SetStringField(TEXT("texture"),Texture2D->GetPathName());
                if(MeshUV&&Material)Tex->SetNumberField(TEXT("materialWorldCmPerRepeat"),Material->GetTextureDensity(Texture2D->GetFName(),*MeshUV));
                Tex->SetNumberField(TEXT("platformWidth"),Texture2D->GetSizeX());Tex->SetNumberField(TEXT("platformHeight"),Texture2D->GetSizeY());
                Tex->SetNumberField(TEXT("totalMips"),Total);Tex->SetNumberField(TEXT("residentMips"),Resident);
                Tex->SetNumberField(TEXT("residentWidth"),Resident>0?FMath::Max(1,Texture2D->GetSizeX()>>First):0);
                Tex->SetNumberField(TEXT("residentHeight"),Resident>0?FMath::Max(1,Texture2D->GetSizeY()>>First):0);
                Textures.Add(MakeShared<FJsonValueObject>(Tex));
            }
            Entry->SetArrayField(TEXT("textures"),Textures);Materials.Add(MakeShared<FJsonValueObject>(Entry));
        }
        Part->SetArrayField(TEXT("materials"),Materials);RenderedParts.Add(MakeShared<FJsonValueObject>(Part));
    }
    Result->SetArrayField(TEXT("components"),RenderedParts);
#if WITH_EDITOR
    // One bounded capture-time census includes legacy and non-national owners.
    // It reads the displayed components only; no mesh, actor or binding changes.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {
        TArray<TSharedPtr<FJsonValue>> Visible;bool Truncated=false;
        auto Tags=[](const TArray<FName>& Values){TArray<FString> Texts;for(const FName Name:Values)Texts.Add(Name.ToString());return FString::Join(Texts,TEXT(","));};
        for(TActorIterator<AActor> It(Battle->GetWorld());It;++It)
        {
            AActor* Actor=*It;if(!Actor||Actor->IsHidden())continue;
            TArray<UStaticMeshComponent*> Actual;Actor->GetComponents(Actual);
            for(const UStaticMeshComponent* Component:Actual)
            {
                if(!Component||!Component->IsRegistered()||!Component->IsVisible()||Component->bHiddenInGame||!Component->GetStaticMesh())continue;
                if(Visible.Num()>=512){Truncated=true;break;}
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("owner"),Actor->GetPathName());
                Row->SetStringField(TEXT("ownerTags"),Tags(Actor->Tags));Row->SetStringField(TEXT("component"),Component->GetName());
                Row->SetStringField(TEXT("componentTags"),Tags(Component->ComponentTags));Row->SetStringField(TEXT("mesh"),Component->GetStaticMesh()->GetPathName());
                Row->SetStringField(TEXT("worldTransform"),Component->GetComponentTransform().ToHumanReadableString());
                Row->SetStringField(TEXT("boundsOrigin"),Component->Bounds.Origin.ToString());Row->SetStringField(TEXT("boundsExtent"),Component->Bounds.BoxExtent.ToString());
                Row->SetBoolField(TEXT("castsShadow"),Component->CastShadow);
                if(const auto* Instances=Cast<UInstancedStaticMeshComponent>(Component))
                {
                    const int32 Count=Instances->GetInstanceCount();Row->SetNumberField(TEXT("instanceCount"),Count);TArray<TSharedPtr<FJsonValue>> Samples;
                    TArray<int32> Selected;for(int32 I:{0,Count/3,Count*2/3,Count-1})if(I>=0&&I<Count)Selected.AddUnique(I);
                    for(int32 I:Selected){FTransform Transform;if(Instances->GetInstanceTransform(I,Transform,true)){auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("instance"),I);Sample->SetStringField(TEXT("worldTransform"),Transform.ToHumanReadableString());Samples.Add(MakeShared<FJsonValueObject>(Sample));}}
                    Row->SetArrayField(TEXT("instanceSamples"),Samples);
                }
                TArray<TSharedPtr<FJsonValue>> BoundMaterials;for(int32 I=0;I<Component->GetNumMaterials();++I)BoundMaterials.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Component->GetMaterial(I))));
                Row->SetArrayField(TEXT("materials"),BoundMaterials);Visible.Add(MakeShared<FJsonValueObject>(Row));
            }
            if(Truncated)break;
        }
        Result->SetArrayField(TEXT("allVisibleStaticComponents"),Visible);Result->SetBoolField(TEXT("visibleComponentCensusTruncated"),Truncated);
    }
#endif
    Result->SetBoolField(TEXT("renderingStateRead"),true);return Result;
}
int32 ApplyWeatherWetness(const AWarfareFrontlineGameMode* Battle,float Wetness)
{
    if(!FMath::IsFinite(Wetness))return 0;
    const auto* Scene=Scenes.Find(TWeakObjectPtr<const AWarfareFrontlineGameMode>(Battle));
    if(!Scene||!Scene->Owner.IsValid())return 0;
    TArray<UStaticMeshComponent*> Components;Scene->Owner->GetComponents(Components);int32 Count=0;
    if(AActor* Props=Scene->CosmeticProbe.Get())
    {TArray<UStaticMeshComponent*> Extra;Props->GetComponents(Extra);Components.Append(Extra);}
    TSet<UMaterialInstanceDynamic*> Updated;
    for(auto* Component:Components)
    {
        if(!Component||(!Component->ComponentHasTag(TEXT("ground"))&&!Component->ComponentHasTag(TEXT("outsideBackdrop"))
            &&!Component->ComponentHasTag(TEXT("FRRU_V826WorldPart"))&&!Component->ComponentHasTag(TEXT("FRRU_V87FieldDetail"))
            &&!Component->ComponentHasTag(TEXT("FRRU_V87TripoRuin"))&&!Component->ComponentHasTag(TEXT("FRRU_V87MortFlankFireStep"))))continue;
        for(int32 Slot=0;Slot<Component->GetNumMaterials();++Slot)
        {
            auto* Material=Component->GetMaterial(Slot);float Previous=0.f;
            if(!Material||!Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Previous))continue;
            auto* Dynamic=Cast<UMaterialInstanceDynamic>(Material);
            if(!Dynamic)Dynamic=Component->CreateDynamicMaterialInstance(Slot);
            if(Dynamic&&!Updated.Contains(Dynamic))
            {
                const float Desired=FMath::Clamp(Wetness,0.f,1.f);
                if(!FMath::IsNearlyEqual(Previous,Desired,1.e-5f))Dynamic->SetScalarParameterValue(TEXT("WeatherWetness"),Desired);
                Updated.Add(Dynamic);++Count;
            }
        }
    }
    return Count;
}
void ApplyWeatherDistant(const AWarfareFrontlineGameMode* Battle,float SkyGain,float CloudCover,float Clock)
{
    if(!FMath::IsFinite(SkyGain)||!FMath::IsFinite(CloudCover)||!FMath::IsFinite(Clock))return;
    const auto* Scene=Scenes.Find(TWeakObjectPtr<const AWarfareFrontlineGameMode>(Battle));
    auto* Component=Scene?Scene->DistantMatte.Get():nullptr;
    auto* Material=Component?Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0)):nullptr;
    if(!Material)return;
    Material->SetScalarParameterValue(TEXT("WeatherSkyGain"),SkyGain);
    Material->SetScalarParameterValue(TEXT("WeatherCloudCover"),CloudCover);
    Material->SetScalarParameterValue(TEXT("WeatherTime"),Clock);
}
bool ProbeScene(const AWarfareFrontlineGameMode* Battle,const TArray<FVector2D>& Extra,
    const TArray<FFRRUSceneSolidRayV82>& SolidRays,TSharedPtr<FJsonObject>& Evidence,FString& Error)
{
    Error.Reset();Evidence=MakeShared<FJsonObject>();Evidence->SetBoolField(TEXT("success"),false);
    Evidence->SetBoolField(TEXT("artAccepted"),false);Evidence->SetBoolField(TEXT("dynamicWireAccepted"),false);
    Evidence->SetBoolField(TEXT("collisionAccepted"),false);
    FString Id;uint32 Revision=0;int32 Count=0;FVector2D Minimum,Maximum;
    const auto* Scene=Scenes.Find(TWeakObjectPtr<const AWarfareFrontlineGameMode>(Battle));
    if(!Scene||!WarfareFRRUWorldV82::SceneInfo(Battle,Id,Revision,Count,Minimum,Maximum)
        ||!SceneStaged(Battle,Id,Revision))return Reject(Error,TEXT("probe requires the selected actually staged scene"));
    Evidence->SetStringField(TEXT("sceneId"),Id);Evidence->SetNumberField(TEXT("nativeRevision"),Revision);
    Evidence->SetStringField(TEXT("authoredSceneToNative"),TEXT("fixed complete-scene inverse FBX Y reflection; native coordinates unchanged"));
    Evidence->SetStringField(TEXT("traceSource"),TEXT("direct staged component; legacy host floor excluded"));
    TArray<UStaticMeshComponent*> Components;Scene->Owner->GetComponents(Components);
    UStaticMeshComponent* Ground=nullptr;UStaticMeshComponent* Solids=nullptr;
    TArray<TSharedPtr<FJsonValue>> DecorationChecks;
    bool Passed=true;
    for(auto* C:Components)
    {
        if(C->ComponentHasTag(TEXT("ground")))Ground=C;
        else if(C->ComponentHasTag(TEXT("solids")))Solids=C;
        else
        {
            const auto* Body=C->GetBodyInstance();
            const bool NoBody=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision
                &&(!Body||!Body->IsValidBodyInstance());
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("component"),C->GetName());
            Item->SetBoolField(TEXT("actualNoCollision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
            Item->SetBoolField(TEXT("actualPhysicsBodyAbsent"),!Body||!Body->IsValidBodyInstance());
            Item->SetBoolField(TEXT("passed"),NoBody);Passed&=NoBody;
            DecorationChecks.Add(MakeShared<FJsonValueObject>(Item));
            if(C->ComponentHasTag(TEXT("FRRU_V82NativeRiver")))
            {
                auto River=MakeShared<FJsonObject>();River->SetStringField(TEXT("component"),C->GetPathName());
                River->SetStringField(TEXT("material"),GetPathNameSafe(C->GetMaterial(0)));
                River->SetBoolField(TEXT("castsShadow"),C->CastShadow);River->SetBoolField(TEXT("receivesDecals"),C->bReceivesDecals);
                River->SetBoolField(TEXT("collisionEnabled"),C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
                River->SetNumberField(TEXT("levelCm"),C->GetComponentLocation().Z);
                River->SetStringField(TEXT("displayFootprintCm"),TEXT("X[240,1920],Y[-12000,12000]"));
                River->SetStringField(TEXT("gameplayQueryFootprintCm"),TEXT("X[240,1920],Y[-6000,6000], floor below -22.1 cm"));
                River->SetBoolField(TEXT("outsideDepthIsVisualProxy"),true);River->SetBoolField(TEXT("artAccepted"),false);
                if(auto* Response=Scene->Owner->FindComponentByClass<UFrontlineWaterResponseV51>())River->SetObjectField(TEXT("actualResponse"),Response->Describe());
#if WITH_EDITOR
                UTexture* Texture=nullptr;
                const auto* Material=C->GetMaterial(0);
                if(Material)Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("NativeHeightGridV82")),Texture);
                TSharedPtr<FJsonObject> HeightEvidence;
                const FString Snapshot=UWarfareFRRUAssetAuditV82::SnapshotHeightfieldTexture(Cast<UTexture2D>(Texture));
                const auto Reader=TJsonReaderFactory<>::Create(Snapshot);
                const bool HeightRead=FJsonSerializer::Deserialize(Reader,HeightEvidence)&&HeightEvidence.IsValid()
                    &&HeightEvidence->GetBoolField(TEXT("success"));
                River->SetStringField(TEXT("actualHeightTexture"),GetPathNameSafe(Texture));
                if(HeightEvidence.IsValid())River->SetObjectField(TEXT("actualHeightData"),HeightEvidence);
                River->SetBoolField(TEXT("actualHeightDataRead"),HeightRead);Passed&=HeightRead;
#endif
                Evidence->SetObjectField(TEXT("nativeWaterRenderer"),River);
            }
        }
    }
    if(!Ground||!Solids)return Reject(Error,TEXT("probe could not locate actual ground/solid components"));
    Evidence->SetArrayField(TEXT("decorationCollision"),DecorationChecks);
    TArray<FVector2D> Samples;
    // Intentionally non-symmetric coordinates expose X/Y flips and transposes.
    const float XS[]={.041f,.138f,.259f,.337f,.456f,.583f,.691f,.817f,.941f};
    const float YS[]={.083f,.281f,.463f,.719f,.907f};
    for(float Y:YS)for(float X:XS)Samples.Add(Minimum+(Maximum-Minimum)*FVector2D(X,Y));
    for(const auto& Node:WarfareFRRUWorldV82::View(Battle).Nodes)Samples.Add(FVector2D(Node.Position));
    Samples.Append(Extra);
    TArray<TSharedPtr<FJsonValue>> Heights;float MaximumError=0.f;
    FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(FRRUV82HeightAcceptance),true);
    for(const auto& P:Samples)
    {
        float Native=0.f;const bool NativeFound=WarfareFRRUWorldV82::TryHeight(Battle,P.X,P.Y,Native);
        FHitResult Hit;const bool Found=Ground->LineTraceComponent(Hit,FVector(P.X,P.Y,10000),FVector(P.X,P.Y,-10000),GroundQuery);
        const float Difference=Found&&NativeFound?FMath::Abs(Hit.ImpactPoint.Z-Native):MAX_flt;
        const bool Valid=Found&&NativeFound&&Difference<=.5f;Passed&=Valid;
        if(Found&&NativeFound)MaximumError=FMath::Max(MaximumError,Difference);
        auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("x"),P.X);Item->SetNumberField(TEXT("y"),P.Y);
        Item->SetBoolField(TEXT("nativeFound"),NativeFound);Item->SetBoolField(TEXT("actualGroundHit"),Found);
        if(NativeFound)Item->SetNumberField(TEXT("nativeHeightCm"),Native);
        if(Found)Item->SetNumberField(TEXT("actualHeightCm"),Hit.ImpactPoint.Z);
        if(Found&&NativeFound)Item->SetNumberField(TEXT("absoluteErrorCm"),Difference);
        Item->SetBoolField(TEXT("passed"),Valid);Heights.Add(MakeShared<FJsonValueObject>(Item));
    }
    Evidence->SetArrayField(TEXT("heightSamples"),Heights);Evidence->SetNumberField(TEXT("heightToleranceCm"),.5);
    Evidence->SetNumberField(TEXT("maximumHeightErrorCm"),MaximumError);
    TArray<TSharedPtr<FJsonValue>> Rays;
    FCollisionQueryParams SolidQuery(SCENE_QUERY_STAT(FRRUV82SolidAcceptance),false);
    for(const auto& Ray:SolidRays)
    {
        FHitResult Hit;const bool Found=Solids->LineTraceComponent(Hit,Ray.From,Ray.To,SolidQuery);
        const bool Valid=Found==Ray.ExpectedBlocked;Passed&=Valid;
        auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("label"),Ray.Label);
        Item->SetBoolField(TEXT("nativeExpectedBlocked"),Ray.ExpectedBlocked);Item->SetBoolField(TEXT("actualSolidHit"),Found);
        for(const auto& Point: {TPair<FString,FVector>(TEXT("from"),Ray.From),TPair<FString,FVector>(TEXT("to"),Ray.To)})
        {TArray<TSharedPtr<FJsonValue>> XYZ;XYZ.Add(MakeShared<FJsonValueNumber>(Point.Value.X));XYZ.Add(MakeShared<FJsonValueNumber>(Point.Value.Y));XYZ.Add(MakeShared<FJsonValueNumber>(Point.Value.Z));Item->SetArrayField(Point.Key,XYZ);}
        Item->SetBoolField(TEXT("passed"),Valid);Rays.Add(MakeShared<FJsonValueObject>(Item));
    }
    Evidence->SetArrayField(TEXT("solidRays"),Rays);Evidence->SetBoolField(TEXT("solidRayCoverageSupplied"),!SolidRays.IsEmpty());
    // Empty optional rays are diagnostic height evidence, not complete solids acceptance.
    Evidence->SetBoolField(TEXT("solidRayChecksPassed"),Passed&&!SolidRays.IsEmpty());
    Evidence->SetBoolField(TEXT("success"),Passed);
    if(!Passed)Error=TEXT("actual scene trace differs from native height/solid or NoCollision contract; inspect probe evidence");
    return Passed;
}
void ClearScene(AWarfareFrontlineGameMode* Battle)
{
#if WITH_EDITOR
    FRRUVehicleNavCandidateV87::Clear(Battle);
#endif
    FRRUVehicleNavV87::Clear(Battle);
    const TWeakObjectPtr<const AWarfareFrontlineGameMode> Key(Battle);
    if(auto* Scene=Scenes.Find(Key))
    {
        if(AActor* Props=Scene->CosmeticProbe.Get())Props->Destroy();
        if(AActor* Owner=Scene->Owner.Get()){
#if WITH_EDITOR
            FRRUVauxHQ27VisualV87::Forget(Owner);
#endif
            FRRUVauxHQ27ReleaseLoaderV87::Forget(Owner);
            UFrontlineWaterResponseV51::UnbindNativeV82(Owner);Owner->Destroy();}
    }
    Scenes.Remove(Key);
    for(auto It=Scenes.CreateIterator();It;++It)if(!It.Key().IsValid())
    {
        if(AActor* Props=It.Value().CosmeticProbe.Get())Props->Destroy();
        if(AActor* Owner=It.Value().Owner.Get()){
#if WITH_EDITOR
            FRRUVauxHQ27VisualV87::Forget(Owner);
#endif
            FRRUVauxHQ27ReleaseLoaderV87::Forget(Owner);
            UFrontlineWaterResponseV51::UnbindNativeV82(Owner);Owner->Destroy();}
        It.RemoveCurrent();
    }
}
}
