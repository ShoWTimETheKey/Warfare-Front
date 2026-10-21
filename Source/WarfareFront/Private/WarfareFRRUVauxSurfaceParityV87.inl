// Accepted Vaux-only paired material. Original source shaders and channel
// semantics are unchanged. Campaign access remains a separate release gate.
#pragma once
namespace WarfareFRRUVauxSurfaceParityV87
{
inline const WarfareFRRUWorldSurfaceV826::FEntry* Find(const FString& Scene)
{
    if(Scene!=TEXT("fr.vaux.scene.families.v1"))return nullptr;
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrownMud"))&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxMacro"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxRubble")))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Brown={
            TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxBrownMud/CandidateV1/Materials/MI_VauxBrownMudScan_V87.MI_VauxBrownMudScan_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxBrownMud/CandidateV1/Materials/M_VauxBrownMudScan_V87.M_VauxBrownMudScan_V87"),
            TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1308.f};
        return &Brown;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust")))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Candidate={
            TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxMacroDesign/CandidateV4/Materials/MI_VauxBrickClastsFineDust_V87.MI_VauxBrickClastsFineDust_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxMacroDesign/CandidateV4/Materials/M_VauxBrickClastsFineDust_V87.M_VauxBrickClastsFineDust_V87"),
            TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1308.f};
        return &Candidate;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxMacro")))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Candidate={
            TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxMacroDesign/CandidateV2/Materials/MI_VauxMacroRegions_V87.MI_VauxMacroRegions_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxMacroDesign/CandidateV2/Materials/M_VauxMacroRegions_V87.M_VauxMacroRegions_V87"),
            TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1308.f};
        return &Candidate;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxRubble")))
    {
        static const WarfareFRRUWorldSurfaceV826::FEntry Candidate={
            TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRubbleTransition/CandidateV1/Materials/MI_VauxRubbleTransition_V87.MI_VauxRubbleTransition_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRubbleTransition/CandidateV1/Materials/M_VauxRubbleTransition_V87.M_VauxRubbleTransition_V87"),
            TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1308.f};
        return &Candidate;
    }
#endif
    static const WarfareFRRUWorldSurfaceV826::FEntry Entry={
        TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),
        TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxSurfaceParity/CandidateV1/Materials/MI_VauxSurfaceParity_V87.MI_VauxSurfaceParity_V87"),
        TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxSurfaceParity/CandidateV1/Materials/M_VauxSurfaceParity_V87.M_VauxSurfaceParity_V87"),
        TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"),
        TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1308.f};
    return &Entry;
}
}
