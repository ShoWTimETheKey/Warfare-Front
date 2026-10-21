// Isolated material candidate only. This file does not enable scene consumers.
// Mother shaders already adapt imported native UV0 from 200cm to V76's 145cm.
// Keep original native vertex colours; never call the old Malmaison override.
#pragma once
namespace WarfareFRRUWorldSurfaceV826
{
inline constexpr bool CandidateOnly=true;
inline constexpr float NativeUVCm=200.f;
inline constexpr float SoilProjection=.52f;
inline constexpr float DepositProjection=.725f;
inline constexpr float MudSamplingScale=(200.f/145.f)*SoilProjection;
inline constexpr float ClaySamplingScale=(200.f/145.f)*DepositProjection;
inline constexpr bool PreserveRawNativeVertexColours=true;
// Zero pseudo colour-only overlays after any host palette writes. Real mineral
// and partial cover are authored R/B; G remains the physical moisture channel.
inline constexpr bool DisableLegacyPaletteOverlays=true;
inline constexpr float GrassCoverage=0.f,ChalkExposure=0.f,BallastCoverage=0.f,SnowCover=0.f;
struct FEntry
{
    const TCHAR* Scene;
    const TCHAR* Token;
    const TCHAR* GroundInstance;
    const TCHAR* GroundBase;
    const TCHAR* ShoulderInstance;
    const TCHAR* CoverTexturePrefix;
    float CoverRepeatCm;
    float FieldSeed;
};
inline const FEntry Entries[]={
    {TEXT("fr.malmaison.scene.families.v1"),TEXT("fr_malmaison"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/AppearanceFaciesV826/CandidateV1/Materials/MI_AF_fr_malmaison_V826.MI_AF_fr_malmaison_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/AppearanceFaciesV826/CandidateV1/Materials/M_AF_fr_malmaison_V826.M_AF_fr_malmaison_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/AppearanceFaciesV826/CandidateV1/Materials/MI_Shoulder_AF_fr_malmaison_V826.MI_Shoulder_AF_fr_malmaison_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_"),315.f,1005.f},
    {TEXT("ru.galicia.scene.redesign.v1"),TEXT("ru_galicia"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/CandidateV1/Materials/MI_WS_ru_galicia_V826.MI_WS_ru_galicia_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/CandidateV1/Materials/M_WS_ru_galicia_V826.M_WS_ru_galicia_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/CandidateV1/Materials/MI_Shoulder_WS_ru_galicia_V826.MI_Shoulder_WS_ru_galicia_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1100.f},
    {TEXT("fr.chemin_dames.scene.families.v1"),TEXT("fr_chemin_dames"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_fr_chemin_dames_V826.MI_WS_fr_chemin_dames_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_fr_chemin_dames_V826.M_WS_fr_chemin_dames_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_fr_chemin_dames_V826.MI_Shoulder_WS_fr_chemin_dames_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1300.f},
    {TEXT("fr.champagne_defence.scene.families.v1"),TEXT("fr_champagne_defence"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_fr_champagne_defence_V826.MI_WS_fr_champagne_defence_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_fr_champagne_defence_V826.M_WS_fr_champagne_defence_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_fr_champagne_defence_V826.MI_Shoulder_WS_fr_champagne_defence_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1301.f},
    {TEXT("fr.sommepy.scene.families.v1"),TEXT("fr_sommepy"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_fr_sommepy_V826.MI_WS_fr_sommepy_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_fr_sommepy_V826.M_WS_fr_sommepy_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_fr_sommepy_V826.MI_Shoulder_WS_fr_sommepy_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1302.f},
    {TEXT("fr.soissons.scene.families.v1"),TEXT("fr_soissons"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_fr_soissons_V826.MI_WS_fr_soissons_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_fr_soissons_V826.M_WS_fr_soissons_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_fr_soissons_V826.MI_Shoulder_WS_fr_soissons_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_"),315.f,1303.f},
    {TEXT("ru.lutsk.scene.families.v1"),TEXT("ru_lutsk"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_ru_lutsk_V826.MI_WS_ru_lutsk_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_ru_lutsk_V826.M_WS_ru_lutsk_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_ru_lutsk_V826.MI_Shoulder_WS_ru_lutsk_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1304.f},
    {TEXT("ru.czernowitz.scene.families.v1"),TEXT("ru_czernowitz"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_ru_czernowitz_V826.MI_WS_ru_czernowitz_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_ru_czernowitz_V826.M_WS_ru_czernowitz_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_ru_czernowitz_V826.MI_Shoulder_WS_ru_czernowitz_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_"),315.f,1305.f},
    {TEXT("ru.naroch.scene.families.v1"),TEXT("ru_naroch"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_ru_naroch_V826.MI_WS_ru_naroch_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_ru_naroch_V826.M_WS_ru_naroch_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_ru_naroch_V826.MI_Shoulder_WS_ru_naroch_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_"),315.f,1306.f},
    {TEXT("ru.warsaw_rearguard.scene.families.v1"),TEXT("ru_warsaw_rearguard"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_ru_warsaw_rearguard_V826.MI_WS_ru_warsaw_rearguard_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_ru_warsaw_rearguard_V826.M_WS_ru_warsaw_rearguard_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_ru_warsaw_rearguard_V826.MI_Shoulder_WS_ru_warsaw_rearguard_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1307.f},
    {TEXT("ru.kovel.scene.families.v1"),TEXT("ru_kovel"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_ru_kovel_V826.MI_WS_ru_kovel_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_ru_kovel_V826.M_WS_ru_kovel_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_ru_kovel_V826.MI_Shoulder_WS_ru_kovel_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_grass_ground_"),251.f,1308.f},
    {TEXT("ru.riga.scene.families.v1"),TEXT("ru_riga"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_WS_ru_riga_V826.MI_WS_ru_riga_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/M_WS_ru_riga_V826.M_WS_ru_riga_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldSurfaceV826/BatchV1/Materials/MI_Shoulder_WS_ru_riga_V826.MI_Shoulder_WS_ru_riga_V826"),
     TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Textures/T_WF_forest_ground_04_"),315.f,1309.f},
};
}
