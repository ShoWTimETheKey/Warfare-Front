// Paired corrected German Feuerwalze paintings. Keep V32/V75 baselines intact.
// Include in the artwork constructor after V8215 bindings, before registration.
auto GermanBarrageV851=[&](const TCHAR* Name)->FFrontlineArtRegionV71
{
    const FString Path=FString::Printf(TEXT("/Game/Warfare/UI/V851/%s.%s"),Name,Name);
    return {LoadObject<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn)};
};
const auto GermanBarrageColourV851=GermanBarrageV851(TEXT("T_GermanBarrage_Colour_V851"));
const auto GermanBarrageArchiveV851=GermanBarrageV851(TEXT("T_GermanBarrage_Archive_V851"));
if(GermanBarrageColourV851.Texture&&GermanBarrageArchiveV851.Texture)
{
    TechnologyPaintingsV71[1][19]=GermanBarrageColourV851;
    ArchivePaintingsV74[1][19]=GermanBarrageArchiveV851;
    SupportPaintingsV71[1][int32(EFrontlineSupport::CreepingBarrage)]=GermanBarrageColourV851;
}
