#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineTerrainProfilesV51.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlinePacingV24.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ContentStreaming.h"
#include "WarfareFrontlineTerrainMaterialV47.h"
#include "WarfareFrontlineArtV68.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineGroundContactV8449.h"
#include "WarfareFRRUCampaignV82.h"
#include "Engine/Texture2D.h"
#include "SceneInterface.h"

namespace
{
struct FGroundUnitsBindingV839 {const TCHAR* SourceName;const TCHAR* CandidateName;const TCHAR* Source;const TCHAR* Candidate;};
const FGroundUnitsBindingV839 GroundUnitsBindingsV839[]={
    {TEXT("M_GroundChalk_V51"),TEXT("M_GroundChalk51_V839"),TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundChalk_V51.MI_GroundChalk_V51"),TEXT("/Game/Warfare/Field/V839/Materials/MI_GroundChalk51_V839.MI_GroundChalk51_V839")},
    {TEXT("M_GroundRubble_V51"),TEXT("M_GroundRubble51_V839"),TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundRubble_V51.MI_GroundRubble_V51"),TEXT("/Game/Warfare/Field/V839/Materials/MI_GroundRubble51_V839.MI_GroundRubble51_V839")},
    {TEXT("M_GroundWinter_V51"),TEXT("M_GroundWinter51_V839"),TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundWinter_V51.MI_GroundWinter_V51"),TEXT("/Game/Warfare/Field/V839/Materials/MI_GroundWinter51_V839.MI_GroundWinter51_V839")},
    {TEXT("M_GroundChalk_V58"),TEXT("M_GroundChalk58_V839"),TEXT("/Game/Warfare/Field/V58/Materials/MI_GroundChalk_V58.MI_GroundChalk_V58"),TEXT("/Game/Warfare/Field/V839/Materials/MI_GroundChalk58_V839.MI_GroundChalk58_V839")},
    {TEXT("M_GroundRubble_V58"),TEXT("M_GroundRubble58_V839"),TEXT("/Game/Warfare/Field/V58/Materials/MI_GroundRubble_V58.MI_GroundRubble_V58"),TEXT("/Game/Warfare/Field/V839/Materials/MI_GroundRubble58_V839.MI_GroundRubble58_V839")}};
struct FSurfaceBindingV8449 {const TCHAR* SourceBase;const TCHAR* Candidate;};
const FSurfaceBindingV8449 SurfaceBindingsV8449[]={
    {TEXT("M_GroundMud_V76"),TEXT("/Game/Warfare/Field/V8449/Surface/Materials/MI_GroundMud76_V8449.MI_GroundMud76_V8449")},
    {TEXT("M_GroundChalk51_V839"),TEXT("/Game/Warfare/Field/V8449/Surface/Materials/MI_GroundChalk51_V8449.MI_GroundChalk51_V8449")},
    {TEXT("M_GroundRubble51_V839"),TEXT("/Game/Warfare/Field/V8449/Surface/Materials/MI_GroundRubble51_V8449.MI_GroundRubble51_V8449")},
    {TEXT("M_GroundWinter51_V839"),TEXT("/Game/Warfare/Field/V8449/Surface/Materials/MI_GroundWinter51_V8449.MI_GroundWinter51_V8449")},
    {TEXT("M_GroundChalk58_V839"),TEXT("/Game/Warfare/Field/V8449/Surface/Materials/MI_GroundChalk58_V8449.MI_GroundChalk58_V8449")},
    {TEXT("M_GroundRubble58_V839"),TEXT("/Game/Warfare/Field/V8449/Surface/Materials/MI_GroundRubble58_V8449.MI_GroundRubble58_V8449")}};
bool UseV56Backdrop(UWorld* World)
{
    // Registered landscapes have a deeper physical rear apron and
    // authored contact height. Other missions retain their V55 panorama.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV55BackdropLegacy")))return false;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV56MatteBlend"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return true;
    if(!World)return false;
    const auto* B=Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode());
    const auto* Mission=B?FrontlineCampaignV32::CurrentOperation(B):nullptr;
    return Mission&&(Mission->Id==3||Mission->Id==4||Mission->Id==5
        ||Mission->Id==7||Mission->Id==12||Mission->Id==13||Mission->Id==16||Mission->Id==17);
}
}

float FrontlineTerrainV42::CameraVerticalLimitV43(float Width,float Aspect)
{
    // The scene UI letterboxes on tall displays; its bottom remains at 852 in
    // the 1080-point design. Reserve the visible foreground plus a 2 m margin.
    const float DockOffset=312.f*FMath::Min(FMath::Max(.3f,Aspect)/1920.f,1.f/1080.f);
    const float NearReach=(DockOffset*Width/FMath::Max(.3f,Aspect)-650.f*.961524f+150.f*.961524f)/.274721f;
    const float SafeFront=8087.5f-NearReach-200.f;
    return FMath::Max(0.f,FMath::Min(SafeFront,FMath::Clamp(2000.f-Width*.08f,350.f,1600.f)));
}

void FrontlineTerrainV42::PlaceBackdropV43(AActor* Matte,const FVector& Focus,float Width,float Aspect)
{
    if(!Matte)return;
    const double Centre=.6*Focus.X;
    const double Span=FMath::Max(12500.,Width+2*FMath::Abs(Focus.X-Centre)+400.);
    // Preserve the authored church/treeline horizon while extending only the
    // canvas coverage when a tall display would expose the top edge.
    const double SkyCoverage=(Width/FMath::Max(.3f,Aspect)*.5+150.-1428.5-Focus.Y*.274721+760.)/.576914;
    const double Height=FMath::Max(4200.*Span/12500.,SkyCoverage);
    if(UseV56Backdrop(Matte->GetWorld()))
    {
        // The new regional paintings have their horizon at different image
        // heights. A single V12 card transform crops the St Quentin skyline.
        // Keep these calibrated, per-operation values separate from the
        // accepted V55 placement, and expose bounded review-only overrides.
        float HeightScale=1.f,ContactImageV=.60f,ContactZ=-140.f;
        bool bRegisteredRegion=false;
        if(const auto* B=Cast<AWarfareFrontlineGameMode>(Matte->GetWorld()->GetAuthGameMode()))
            if(const auto* Mission=FrontlineCampaignV32::CurrentOperation(B))
                switch(Mission->Id)
                {
                case 3:case 4: // Messines ridge: the original skyline is already visible.
                    bRegisteredRegion=true;HeightScale=1.f;ContactImageV=.60f;break;
                case 5: // Ypres marsh: preserve the painted water and village horizon.
                    bRegisteredRegion=true;HeightScale=1.f;ContactImageV=.60f;break;
                case 7: // Menin Road's far wood line sits above a low broken physical crest.
                    bRegisteredRegion=true;HeightScale=.45f;ContactImageV=.70f;ContactZ=0.f;break;
                case 12: // The Lys water course reaches this fixed landscape at the same bearing.
                    bRegisteredRegion=true;HeightScale=.45f;ContactImageV=.70f;ContactZ=0.f;break;
                case 13: // Villers: ruined town meets a continuous, physically lit rubble apron.
                    bRegisteredRegion=true;HeightScale=.45f;ContactImageV=.71f;ContactZ=0.f;break;
                case 17: // Selle wash: meet the receding rail bed with the raised physical rear apron.
                    bRegisteredRegion=true;HeightScale=.45f;ContactImageV=.70f;break;
                case 16: // Riqueval: include clouds and ruins above the muddy bank.
                    bRegisteredRegion=true;HeightScale=.33f;ContactImageV=.77f;ContactZ=0.f;break;
                default:break;
                }
        FParse::Value(FCommandLine::Get(),TEXT("WFV56MatteHeightScale="),HeightScale);
        FParse::Value(FCommandLine::Get(),TEXT("WFV56MatteContactV="),ContactImageV);
        FParse::Value(FCommandLine::Get(),TEXT("WFV56MatteContactZ="),ContactZ);
        // Unlike the V55 camera-following card, the regional art keeps one
        // world-space X registration through every pan and zoom. The battle
        // camera is clamped to CombatHalfLength - Width/2, so this span covers
        // either extreme with 300 cm of extra canvas on each side.
        const double RegisteredCentre=bRegisteredRegion?0.:Centre;
        const double RegisteredSpan=bRegisteredRegion
            ?FMath::Max(12500.,2.*(FrontlinePacingV24::CombatHalfLength()+300.)):Span;
        // Horizontal coverage must not rescale the authored vertical horizon.
        const double RegisteredHeight=bRegisteredRegion?FMath::Max(4200.,SkyCoverage):Height;
        const double CalibratedHeight=RegisteredHeight*FMath::Clamp(HeightScale,.20f,1.50f);
        const double CalibratedV=FMath::Clamp(ContactImageV,.08f,.92f);
        Matte->SetActorLocation(FVector(RegisteredCentre,-5200.,ContactZ-(1.-CalibratedV)*CalibratedHeight));
        Matte->SetActorScale3D(FVector(RegisteredSpan/100.,1.,CalibratedHeight/100.));
        return;
    }
    // The physical apron reaches the photograph's plane. Meet its distant
    // treeline, not the large flat photographic foreground that formed a dark
    // horizontal band above the real shell holes. The higher coverage bound
    // keeps the top covered when zooming or using a tall aspect ratio.
    Matte->SetActorLocation(FVector(Centre,-5200,-140.-.4*Height));
    Matte->SetActorScale3D(FVector(Span/100.,1,Height/100.));
}

void FrontlineTerrainV42::UpdateStreaming(UWorld* World,const FVector& Camera,const FVector& Focus,float OrthoWidth,int32 ViewportWidth)
{
    if(!World||OrthoWidth<=1||ViewportWidth<=0||FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))return;
    // UE5.8's ComputeViewInfoExtras consumes ScreenSize, not FOVScreenSize.
    // Convert the ortho pixel/cm projection to an equivalent perspective
    // screen size at the actual focus distance; sending it only in the legacy
    // FOV field leaves the streamer's demand unchanged at the tiny mip floor.
    // Ordinary spatial culling, mip budgets and hardware limits remain active.
    const float Distance=FVector::Distance(Camera,Focus);
    const float EquivalentScreenSize=float(ViewportWidth)*2.f*Distance/OrthoWidth;
    IStreamingManager::Get().AddViewInformation(Camera,EquivalentScreenSize,EquivalentScreenSize,1.f,false,0,nullptr,World);

    struct FGroundDemand
    {
        TWeakObjectPtr<AWarfareFrontlineGround> Ground;
        TWeakObjectPtr<UFrontlineTerrainMaterialV47> Material;
        double NextSearch=0;
        float ScanRepeatCm=145.f/.52f;
    };
    static TMap<TWeakObjectPtr<UWorld>,FGroundDemand> Demand;
    for(auto It=Demand.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& State=Demand.FindOrAdd(World);
    const double Now=FPlatformTime::Seconds();
    auto* Ground=State.Ground.Get();
    if((!Ground||Ground->IsActorBeingDestroyed())&&Now>=State.NextSearch)
    {
        State.Ground.Reset();State.Material.Reset();State.NextSearch=Now+.5;
        for(TActorIterator<AWarfareFrontlineGround> It(World);It;++It)
            if(IsValid(*It)&&!It->IsActorBeingDestroyed()&&It->Surface){State.Ground=*It;break;}
        Ground=State.Ground.Get();
    }
    if(!Ground||Ground->IsActorBeingDestroyed()||!Ground->Surface)return;
    auto* Assigned=Ground->Surface->GetMaterial(0);
    if(!Assigned||!Assigned->GetBaseMaterial())return;
    // The ground's sampling contract is a material parameter, not an asset
    // name prefix. Otherwise a renamed/authored material silently falls back
    // to undersized mips despite being correctly assigned to all visible tiles.
    float Projection=.52f;
    if(!Assigned->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SoilProjectionScaleV47")),Projection))return;
    Projection=FMath::Clamp(Projection,.05f,8.f);
    if(Assigned!=State.Material.Get())
    {
        auto* Dynamic=UFrontlineTerrainMaterialV47::CreateForGround(Assigned,Ground);
        if(!Dynamic)return;
        TArray<UTexture*> Textures;Assigned->GetUsedTextures(Textures,{},World->Scene?World->Scene->GetShaderPlatform():GMaxRHIShaderPlatform);
        float DepositProjection=Projection;
        Assigned->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DepositProjectionScaleV51")),DepositProjection);
        DepositProjection=FMath::Clamp(DepositProjection,.05f,8.f);
        TArray<FMaterialTextureInfo> Entries;TSet<FName> Names;
        for(UTexture* Texture:Textures)
        {
            if(!Cast<UTexture2D>(Texture)||Names.Contains(Texture->GetFName()))continue;
            const FString Path=Texture->GetPathName();
            const bool bMeninMacro=Path.StartsWith(TEXT("/Game/Warfare/Field/V60/Textures/T_MeninMacroMask_V60"));
            if(!bMeninMacro&&!Path.StartsWith(TEXT("/Game/Warfare/Field/V23/"))&&!Path.StartsWith(TEXT("/Game/Warfare/Field/V42/"))&&!Path.StartsWith(TEXT("/Game/Warfare/Field/V47/Textures/T_Mud_HeightAO_V47"))&&!Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/")))continue;
            FMaterialTextureInfo Entry;Entry.TextureName=Texture->GetFName();Entry.UVChannelIndex=0;
            // The candidate mask covers the complete 300 m field once, while
            // the photographed soil scans repeat every 2.79 m.
            const bool bDeposit=Path.StartsWith(TEXT("/Game/Warfare/Field/V51/Textures/"))
                ||Path.StartsWith(TEXT("/Game/Warfare/Field/V42/Textures/T_Clay_"));
            Entry.SamplingScale=bMeninMacro?145.f/30000.f:bDeposit?DepositProjection:Projection;
#if WITH_EDITORONLY_DATA
            Entry.TextureReference=FSoftObjectPath(Texture);
#endif
            Entries.Add(Entry);Names.Add(Entry.TextureName);
        }
        if(Entries.IsEmpty())return;
        Dynamic->SetWorldSampling(Entries);
        Ground->Surface->SetMaterial(0,Dynamic);Ground->SyncTerrainMaterialV43();
        // Root earth shares the final streaming-aware ground instance, so
        // subsequent weather changes reach both surfaces. Existing roots are
        // rebound here without rebuilding geometry or changing ground contact.
        if(auto* Battle=Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()))
            FrontlineEnvironmentV42::FinishContactPropsV8449(Battle);
        // The simple streaming manager snapshots a proxy's cached material
        // sampling entries. Re-register the twenty tiles once after repair.
        for(auto& Tile:Ground->BakedTerrainV43)if(Tile)Tile->MarkRenderStateDirty();
        for(TActorIterator<AWarfareFrontlineWeather> It(World);It;++It)It->RebindGroundMaterial();
        State.Material=Dynamic;
        float CmPerUV=145.f;
        if(!Ground->BakedTerrainV43.IsEmpty()&&Ground->BakedTerrainV43[0])
            if(UStaticMesh* Mesh=Ground->BakedTerrainV43[0]->GetStaticMesh())
                if(!Mesh->GetStaticMaterials().IsEmpty())CmPerUV=Mesh->GetStaticMaterials()[0].UVChannelData.LocalUVDensities[0];
        State.ScanRepeatCm=FMath::Clamp(CmPerUV/Projection,25.f,2000.f);
        UE_LOG(LogTemp,Display,TEXT("WF_V47_GROUND_STREAMING sampling_entries=%d scan_repeat_cm=%.3f; transient material and tile sampling caches rebuilt"),Entries.Num(),State.ScanRepeatCm);
    }
    // AddViewInformation above and the repaired world-space sampling metadata
    // provide the demand. UE 5.8's BoostTextures implementation asks this
    // primitive for its textures with EMaterialQualityLevel::Num, which trips
    // a material-quality ensure on the first battlefield tick. Do not add an
    // avoidable hitch merely to request extra mips beyond the actual view.
}

void FrontlineTerrainV42::Apply(AWarfareFrontlineGameMode* B)
{
    if(!B || FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))return;
    // Accepted V839 legacy-bank and exact ground-family corrections. National
    // campaigns retain their own terrain; an explicit opt-out keeps rollback.
    const bool EnvironmentReviewV839=B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    const bool EnvironmentEnabledV839=!WarfareFRRUCampaignV82::Active(B)
        &&!FParse::Param(FCommandLine::Get(),TEXT("WFV839EnvironmentalBaseline"));
    const bool BankCandidateV839=EnvironmentEnabledV839;
    const bool GroundCandidateV839=EnvironmentEnabledV839;
    float BankModeV839=0.f;
    if(BankCandidateV839&&EnvironmentReviewV839)FParse::Value(FCommandLine::Get(),TEXT("WFV839BankMode="),BankModeV839);
    BankModeV839=FMath::Clamp(BankModeV839,0.f,3.f);
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"));
    if(!Mat)Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V44/Materials/MI_EarthBank_V44.MI_EarthBank_V44"));
    if(!Mat)Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42.MI_EarthBank_V42"));
    // V68 changes only the bank's moisture/BRDF response; the same meshes,
    // collision, footing and scan textures remain bound.
    Mat=FrontlineArtV68::ResolveBankMaterial(Mat);
    if(BankCandidateV839)
        if(auto* Candidate=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V839/Materials/MI_EarthBank_V839.MI_EarthBank_V839")))Mat=Candidate;
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);
    const float Wet=Mission?Mission->Wetness:.58f;
    const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,Mission?Mission->Id:0);
    const float Chalk=Palette.Chalk;
    UMaterialInterface* RegionalGround=LoadObject<UMaterialInterface>(nullptr,Palette.GroundMaterial);
    // Menin uses the material matching its entire visible tile generation.
    // A missing V59 set falls back with its V58/V57 mesh and height function.
    int32 MeninVisibleVersion=0;
    if(Mission&&Mission->Id==7)
        for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        {
            int32 V59Tiles=0,V58Tiles=0;
            for(const auto& Tile:It->BakedTerrainV43)
                if(Tile&&Tile->GetStaticMesh())
                {
                    const FString Path=Tile->GetStaticMesh()->GetPathName();
                    // V63 changes water/shore coverage over the same V59
                    // physical soil. Its tiles retain the V59 material family.
                    V59Tiles+=Path.Contains(TEXT("/V59/Terrain/"))
                        ||Path.Contains(TEXT("/V63/Terrain/"));
                    V58Tiles+=Path.Contains(TEXT("/V58/Terrain/"));
                }
            if(It->BakedTerrainV43.Num()==20)
                MeninVisibleVersion=V59Tiles==20?59:V58Tiles==20?58:57;
            break;
        }
    if(Mission)
    {
        const TCHAR* V58Ground=Mission->Id==7&&MeninVisibleVersion==59
            ?TEXT("/Game/Warfare/Field/V59/Materials/MI_GroundMud_V59.MI_GroundMud_V59")
            :Mission->Id==7&&MeninVisibleVersion==58
            ?TEXT("/Game/Warfare/Field/V58/Materials/MI_GroundChalk_V58.MI_GroundChalk_V58")
            :Mission->Id==12
                ?TEXT("/Game/Warfare/Field/V58/Materials/MI_GroundMud_V58.MI_GroundMud_V58")
                :Mission->Id==13
                    ?TEXT("/Game/Warfare/Field/V58/Materials/MI_GroundRubble_V58.MI_GroundRubble_V58")
                    :nullptr;
        if(V58Ground)
            if(auto* Refined=LoadObject<UMaterialInterface>(nullptr,V58Ground))RegionalGround=Refined;
        // An isolated, opt-in material review shares V59's physical terrain,
        // water and collision. A missing candidate keeps the complete V59 set.
        if(Mission->Id==7&&MeninVisibleVersion==59)
        {
            if(FParse::Param(FCommandLine::Get(),TEXT("WFMeninSurface")))
            {
                if(auto* Surface=LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Warfare/Field/Production/Menin/Materials/MI_MeninSurface.MI_MeninSurface")))
                    RegionalGround=Surface;
            }
            else if(FParse::Param(FCommandLine::Get(),TEXT("WFV61MeninPBR")))
            {
                if(auto* Paired=LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Warfare/Field/V61/Materials/MI_GroundMud_Menin_V61.MI_GroundMud_Menin_V61")))
                    RegionalGround=Paired;
            }
            else if(FParse::Param(FCommandLine::Get(),TEXT("WFV60MeninMacro")))
            {
                if(auto* Macro=LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Warfare/Field/V60/Materials/MI_GroundMud_Menin_V60.MI_GroundMud_Menin_V60")))
                    RegionalGround=Macro;
            }
        }
    }
    // One paired compact-clay material serves existing mud profiles. This
    // changes neither the accepted height field nor contact/collision meshes.
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV65Baseline"))&&RegionalGround
        &&RegionalGround->GetName().Contains(TEXT("GroundMud")))
        if(auto* Clay=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V65/Materials/MI_GroundMud_V65.MI_GroundMud_V65")))RegionalGround=Clay;
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV67Baseline"))&&RegionalGround
        &&RegionalGround->GetName().Contains(TEXT("GroundMud")))
        if(auto* Soil=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V67/Materials/MI_GroundMud_V67.MI_GroundMud_V67")))RegionalGround=Soil;
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV76FieldBaseline"))&&RegionalGround
        &&RegionalGround->GetName().Contains(TEXT("GroundMud")))
        if(auto* Soil=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V76/Materials/MI_GroundMud_V76.MI_GroundMud_V76")))RegionalGround=Soil;
    // V68 terrain-aligned facies on the same scans, tiles, collision and water.
    RegionalGround=FrontlineArtV68::ResolveGroundMaterial(RegionalGround);
    if(GroundCandidateV839&&RegionalGround)
    {
        const FString Source=GetNameSafe(RegionalGround->GetBaseMaterial());
        for(const auto& Candidate:GroundUnitsBindingsV839)
            if(Source==Candidate.SourceName)
            {if(auto* Replacement=LoadObject<UMaterialInterface>(nullptr,Candidate.Candidate))RegionalGround=Replacement;break;}
    }
    // Material-only UK/DE candidate. Source parents remain installed and one
    // explicit flag restores the complete preceding field presentation.
    const bool SurfaceEnabledV8449=!WarfareFRRUCampaignV82::Active(B)
        &&!FrontlineEnvironmentV42::FieldBaselineV8449(B);
    if(SurfaceEnabledV8449&&RegionalGround)
    {
        const FString SourceBase=GetNameSafe(RegionalGround->GetBaseMaterial());
        for(const auto& Binding:SurfaceBindingsV8449)
            if(SourceBase==Binding.SourceBase)
            {
                if(auto* Candidate=LoadObject<UMaterialInterface>(nullptr,Binding.Candidate))RegionalGround=Candidate;
                else UE_LOG(LogTemp,Warning,TEXT("WF_V8449_SURFACE_MISSING source=%s candidate=%s"),*SourceBase,Binding.Candidate);
                break;
            }
    }
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
        for(auto* C:Meshes)
        {
            const bool Bank=C->GetStaticMesh()&&C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_TrenchEarth_"));
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
            {
                auto* Existing=C->GetMaterial(Slot);
                const auto* Base=Existing?Existing->GetBaseMaterial():nullptr;
                const bool NewEarth=Base&&(Base->GetName().Contains(TEXT("EarthBank_V42"))||Base->GetName().Contains(TEXT("EarthBank_V44"))||Base->GetName().Contains(TEXT("EarthBank_V68"))||Base->GetName().Contains(TEXT("EarthBank_V76"))||Base->GetName().Contains(TEXT("EarthBank_V839")));
                if((Bank||NewEarth)&&Mat)
                    if(auto* MID=C->CreateDynamicMaterialInstance(Slot,Mat))
                    {MID->SetScalarParameterValue(TEXT("WeatherWetness"),Wet);MID->SetScalarParameterValue(TEXT("ChalkExposure"),Chalk);MID->SetVectorParameterValue(TEXT("SoilTint"),Palette.Soil);
                        if(BankCandidateV839)MID->SetScalarParameterValue(TEXT("BankReviewModeV839"),BankModeV839);}
            }
        }
    }
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        if(auto* Existing=It->Surface->GetMaterial(0))
        {
            if(RegionalGround&&Existing->GetBaseMaterial()!=RegionalGround->GetBaseMaterial())
            {It->Surface->SetMaterial(0,RegionalGround);Existing=RegionalGround;}
            auto* MID=Cast<UMaterialInstanceDynamic>(Existing);
            if(!MID)MID=It->Surface->CreateDynamicMaterialInstance(0,Existing);
            if(MID){MID->SetScalarParameterValue(TEXT("ChalkExposure"),Chalk);MID->SetScalarParameterValue(TEXT("WeatherWetness"),Wet);
                MID->SetVectorParameterValue(TEXT("SoilTint"),Palette.Soil);MID->SetVectorParameterValue(TEXT("HorizonTint"),Palette.Horizon);
                MID->SetScalarParameterValue(TEXT("GrassCoverage"),Palette.Grass);MID->SetScalarParameterValue(TEXT("BallastCoverage"),Palette.Ballast);
                MID->SetScalarParameterValue(TEXT("FieldSeed"),Mission?Mission->Id:0);
                FrontlineArtV68::BindFieldFacies(*It,MID); }
            It->SyncTerrainMaterialV43();
            if(It->Surface->IsRegistered())IStreamingManager::Get().NotifyPrimitiveUpdated(It->Surface);
        }
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
    {
        if(auto* C=It->FindComponentByClass<UStaticMeshComponent>())
        {
            auto* Existing=C->GetMaterial(0);auto* MID=Cast<UMaterialInstanceDynamic>(Existing);
            const bool Prototype=UseV56Backdrop(B->GetWorld());
            const bool V64=!FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline"));
            const TCHAR* ExpectedName=V64?TEXT("M_DistantFront_V64"):Prototype?TEXT("M_DistantFront_V56"):TEXT("M_DistantFront_V50");
            const TCHAR* MaterialPath=V64?TEXT("/Game/Warfare/Field/V64/Materials/M_DistantFront_V64.M_DistantFront_V64"):Prototype
                ?TEXT("/Game/Warfare/Field/V56/Materials/M_DistantFront_V56.M_DistantFront_V56")
                :TEXT("/Game/Warfare/Field/V50/Materials/M_DistantFront_V50.M_DistantFront_V50");
            if(!Existing||!Existing->GetBaseMaterial()||Existing->GetBaseMaterial()->GetName()!=ExpectedName)
                if(auto* BackdropMaterial=LoadObject<UMaterialInterface>(nullptr,MaterialPath))MID=C->CreateDynamicMaterialInstance(0,BackdropMaterial);
            const TCHAR* BackdropKind=Mission&&Mission->Id==10?TEXT("Winter"):Mission&&Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Canal?TEXT("Canal"):Palette.Backdrop;
            const int32 BackdropVersion=(FCString::Strcmp(BackdropKind,TEXT("Winter"))==0||FCString::Strcmp(BackdropKind,TEXT("Canal"))==0)?51:50;
            const FString Path=FString::Printf(TEXT("/Game/Warfare/Field/V%d/Textures/T_Distant%s.T_Distant%s"),BackdropVersion,BackdropKind,BackdropKind);
            UTexture2D* Texture=nullptr;
            if(Mission&&(Mission->Id==3||Mission->Id==4||Mission->Id==5||Mission->Id==8||Mission->Id==14||Mission->Id==16))
            {
                const TCHAR* Region=(Mission->Id==3||Mission->Id==4)?TEXT("Messines"):
                    Mission->Id==5?TEXT("Marsh"):Mission->Id==8?TEXT("Cambrai"):Mission->Id==14?TEXT("Amiens"):TEXT("Canal");
                const int32 RegionalVersion=(Mission->Id==3||Mission->Id==4||Mission->Id==5||Mission->Id==16)?55:54;
                const FString RegionalPath=FString::Printf(TEXT("/Game/Warfare/Field/V%d/Textures/T_Distant%s_V%d.T_Distant%s_V%d"),RegionalVersion,Region,RegionalVersion,Region,RegionalVersion);
                Texture=LoadObject<UTexture2D>(nullptr,*RegionalPath);
            }
            if(Mission&&Mission->Id==17)
                Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Field/V56/Textures/T_DistantSelle_V56.T_DistantSelle_V56"));
            if(Mission&&Mission->Id==7)
            {
                // Match the painted road to the installed V59 oblique terrain.
                // A missing new matte or an older tile set keeps the V57 art.
                if(MeninVisibleVersion==59)
                    Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Field/V59/Textures/T_DistantMenin_V59.T_DistantMenin_V59"));
                if(!Texture)
                    Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Field/V57/Textures/T_DistantMenin_V57.T_DistantMenin_V57"));
            }
            if(Mission&&Mission->Id==12)
                Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Field/V57/Textures/T_DistantLys_V57.T_DistantLys_V57"));
            if(!Texture)Texture=LoadObject<UTexture2D>(nullptr,*Path);
            if(MID&&Texture)MID->SetTextureParameterValue(TEXT("DistantLandscape"),Texture);
            if(Prototype&&MID)
            {
                // The op16 photograph paints a canal in a different world-space
                // course than the playable 3D crossing. Retain its distant
                // skyline, but fade its incompatible painted near-ground.
                // All other operations keep the unmodified V56 image.
                float FadeStart=Mission&&Mission->Id==13?.42f:Mission&&(Mission->Id==7||Mission->Id==12)?.43f:.34f;
                float FadeEnd=Mission&&Mission->Id==13?.72f:Mission&&(Mission->Id==7||Mission->Id==12)?.70f:.46f;
                float FadeStrength=Mission&&Mission->Id==16?.5f:
                    Mission&&Mission->Id==13?.72f:
                    Mission&&(Mission->Id==7||Mission->Id==12)?.55f:0.f;
                FParse::Value(FCommandLine::Get(),TEXT("WFV56PhotoFadeStart="),FadeStart);
                FParse::Value(FCommandLine::Get(),TEXT("WFV56PhotoFadeEnd="),FadeEnd);
                FParse::Value(FCommandLine::Get(),TEXT("WFV56PhotoFadeStrength="),FadeStrength);
                FadeStart=FMath::Clamp(FadeStart,.02f,.92f);
                FadeEnd=FMath::Clamp(FadeEnd,FadeStart+.01f,.99f);
                FadeStrength=FMath::Clamp(FadeStrength,0.f,1.f);
                MID->SetScalarParameterValue(TEXT("MattePhotoFadeStartV56"),FadeStart);
                MID->SetScalarParameterValue(TEXT("MattePhotoFadeEndV56"),FadeEnd);
                MID->SetScalarParameterValue(TEXT("MattePhotoFadeStrengthV56"),FadeStrength);
            }
        }
    }
    for(TActorIterator<AWarfareFrontlineWeather> It(B->GetWorld());It;++It)It->RebindGroundMaterial();
    // Prepare the exact physical source-triangle receiver while the operation
    // loads, including maps with no tree roots. Impacts only query this cache.
    if(!WarfareFRRUCampaignV82::Active(B)&&!FrontlineEnvironmentV42::FieldBaselineV8449(B))
        for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
            FrontlineGroundContactV8449::Prepare(*It);
    FrontlineEnvironmentV42::FinishContactPropsV8449(B);
}

bool FrontlineTerrainV42::SetGroundUnitsReviewV839(AWarfareFrontlineGameMode* B,bool Enabled)
{
    if(!B||!B->GetWorld()||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV839Review")))return false;
    int32 Changed=0;bool Complete=true;
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
    {
        if(!It->Surface)continue;
        auto* Current=It->Surface->GetMaterial(0);
        const FString Name=Current?GetNameSafe(Current->GetBaseMaterial()):FString();
        const FGroundUnitsBindingV839* Binding=nullptr;
        for(const auto& Candidate:GroundUnitsBindingsV839)
            if(Name==Candidate.SourceName||Name==Candidate.CandidateName){Binding=&Candidate;break;}
        if(!Binding){Complete=false;continue;}
        auto* Parent=LoadObject<UMaterialInterface>(nullptr,Enabled?Binding->Candidate:Binding->Source);
        if(!Parent){Complete=false;continue;}
        auto* Next=UMaterialInstanceDynamic::Create(Parent,*It);
        if(!Next){Complete=false;continue;}
        if(auto* Parameters=Cast<UMaterialInstance>(Current))Next->CopyParameterOverrides(Parameters);
        It->Surface->SetMaterial(0,Next);It->SyncTerrainMaterialV43();
        if(It->Surface->IsRegistered())IStreamingManager::Get().NotifyPrimitiveUpdated(It->Surface);
        ++Changed;
        UE_LOG(LogTemp,Display,TEXT("WF_V839_GROUND_UNIT_REVIEW enabled=%d actual_parent=%s material_only=1"),Enabled,*GetPathNameSafe(Next->GetBaseMaterial()));
    }
    if(Changed)for(TActorIterator<AWarfareFrontlineWeather> It(B->GetWorld());It;++It)It->RebindGroundMaterial();
    return Changed>0&&Complete;
}
