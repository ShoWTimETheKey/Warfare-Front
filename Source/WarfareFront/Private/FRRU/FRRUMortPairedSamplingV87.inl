// One Editor NoSave Mort capture-only material comparison. No public enable,
// additional images, terrain, UV scale, masks, colour or BRDF parameter edits.
#pragma once
namespace FRRUMortPairedSamplingV87
{
inline const WarfareFRRUWorldSurfaceV826::FEntry* Find(const FString& Scene)
{
#if WITH_EDITOR
    FString Case;
    if(Scene==TEXT("fr.mort_homme.scene.redesign.v1")
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87MortPairedSampling"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly"))
        &&FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalCase="),Case)&&Case==TEXT("mort-rifle"))
    {
        // Reuse the existing explicit streaming-scale contract: all nine old
        // T_WR maps still use native UV0*200/550. Custom stochastic coordinates
        // must not leave the texture streamer guessing their density.
        static const WarfareFRRUWorldSurfaceV826::FEntry Entry={
            TEXT("fr.mort_homme.scene.redesign.v1"),TEXT("fr_mort_homme"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/MortSurfaceSampling/CandidateV2/M_MortPairedSampling_V87.M_MortPairedSampling_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/MortSurfaceSampling/CandidateV2/M_MortPairedSampling_V87.M_MortPairedSampling_V87"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldRedesign/Materials/M_WR_PairedEarth_C1_V82.M_WR_PairedEarth_C1_V82"),
            TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldRedesign/Textures/T_WR_"),550.f,1000.f};
        return &Entry;
    }
#endif
    return nullptr;
}
}
