#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineFilmV32.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlinePrecipitation.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontlineLightingV42.h"
#include "WarfareFrontlineArtV68.h"
#include "WarfareFRRUContentV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "FRRU/Native/Expansion.h"
#include "HAL/IConsoleManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "WarfareFRRULightingV87.inl"

namespace
{
struct FOperationLightV842
{
    const TCHAR* Key;
    int32 Month;
    const TCHAR* Phase;
    float Sun,Sky,Pitch,Yaw,Angle,CloudShadow,Backdrop;
    FLinearColor SunTint,SkyTint;
};
// These are camera-authored company sectors, not astronomical reconstructions.
// The operation date determines the season; a clock phase is used only where
// the current narrative names it. The single existing movable key supplies
// both sunlight and the winter patrol's cloud-softened moonlight.
// Rain, ground moisture, visibility and fog are deliberately not duplicated.
const FOperationLightV842 OperationLightsV842[]=
{
    {TEXT("arras"),4,TEXT("Day"),1.45f,.88f,-22.f,-16.f,1.25f,.65f,1.f,FLinearColor(.98f,.97f,.95f),FLinearColor(.88f,.93f,1.f)},
    {TEXT("roeux1917"),4,TEXT("Day"),1.24f,.90f,-27.f,-21.f,2.2f,.50f,1.f,FLinearColor(.96f,.965f,.97f),FLinearColor(.89f,.94f,1.f)},
    {TEXT("bullecourt1917"),5,TEXT("Day"),1.35f,.86f,-31.f,-25.f,1.8f,.58f,1.f,FLinearColor(.98f,.97f,.95f),FLinearColor(.89f,.94f,1.f)},
    {TEXT("messines"),6,TEXT("First light"),.77f,.66f,-9.f,-28.f,3.0f,0.f,.73f,FLinearColor(.84f,.90f,1.f),FLinearColor(.82f,.90f,1.f)},
    {TEXT("messines_counter1917"),6,TEXT("Day"),1.17f,.92f,-26.f,-19.f,3.2f,0.f,1.f,FLinearColor(.97f,.965f,.95f),FLinearColor(.91f,.95f,1.f)},
    {TEXT("ypres"),7,TEXT("Day"),1.03f,.93f,-29.f,-12.f,3.6f,0.f,1.f,FLinearColor(.90f,.95f,1.f),FLinearColor(.89f,.95f,1.f)},
    {TEXT("langemarck1917"),8,TEXT("Day"),1.02f,.95f,-32.f,-18.f,4.2f,0.f,1.f,FLinearColor(.915f,.95f,.99f),FLinearColor(.90f,.95f,1.f)},
    {TEXT("menin1917"),9,TEXT("Day"),1.28f,.89f,-24.f,-24.f,2.0f,.52f,1.f,FLinearColor(.985f,.965f,.935f),FLinearColor(.90f,.94f,1.f)},
    {TEXT("cambrai"),11,TEXT("Day"),1.23f,.85f,-17.f,-22.f,2.1f,.50f,.96f,FLinearColor(.965f,.96f,.945f),FLinearColor(.875f,.93f,1.f)},
    {TEXT("bourlon1917"),11,TEXT("Day"),1.07f,.91f,-20.f,-18.f,3.5f,0.f,.98f,FLinearColor(.93f,.95f,.975f),FLinearColor(.885f,.94f,1.f)},
    {TEXT("winter_raid1918"),2,TEXT("Night"),.34f,.44f,-12.f,-26.f,2.8f,0.f,.48f,FLinearColor(.78f,.86f,1.f),FLinearColor(.82f,.89f,1.f)},
    {TEXT("michael"),3,TEXT("Morning mist"),1.09f,.89f,-16.f,-13.f,3.4f,0.f,1.f,FLinearColor(.965f,.955f,.94f),FLinearColor(.89f,.94f,1.f)},
    {TEXT("lys1918"),4,TEXT("Day"),1.04f,.94f,-28.f,-17.f,4.0f,0.f,1.f,FLinearColor(.925f,.955f,.99f),FLinearColor(.90f,.95f,1.f)},
    {TEXT("villers1918"),4,TEXT("Day"),1.25f,.90f,-29.f,-23.f,2.4f,.48f,1.f,FLinearColor(.975f,.967f,.95f),FLinearColor(.895f,.94f,1.f)},
    {TEXT("amiens"),8,TEXT("Morning mist"),1.16f,.90f,-18.f,-8.f,3.f,0.f,1.f,FLinearColor(.97f,.955f,.93f),FLinearColor(.91f,.95f,1.f)},
    {TEXT("scarpe1918"),8,TEXT("Day"),1.37f,.86f,-33.f,-27.f,1.7f,.60f,1.f,FLinearColor(.985f,.97f,.945f),FLinearColor(.89f,.94f,1.f)},
    {TEXT("hindenburg"),9,TEXT("Morning mist"),1.12f,.91f,-21.f,-16.f,3.1f,0.f,1.f,FLinearColor(.97f,.957f,.935f),FLinearColor(.90f,.945f,1.f)},
    {TEXT("selle"),10,TEXT("Day"),1.06f,.92f,-23.f,-20.f,4.1f,0.f,.98f,FLinearColor(.925f,.95f,.985f),FLinearColor(.89f,.94f,1.f)},
    {TEXT("valenciennes1918"),11,TEXT("Day"),1.15f,.88f,-18.f,-24.f,2.9f,.40f,.97f,FLinearColor(.96f,.96f,.95f),FLinearColor(.875f,.93f,1.f)},
    {TEXT("sambre"),11,TEXT("Morning"),1.10f,.87f,-14.f,-19.f,3.3f,0.f,.93f,FLinearColor(.965f,.955f,.94f),FLinearColor(.88f,.935f,1.f)},
};
static_assert(UE_ARRAY_COUNT(OperationLightsV842)==20,"Every available operation has one authored light profile.");
void ApplyOperationLightV842(int32 Operation,FFrontlineWeatherLookV50& L)
{
    if(Operation<0||Operation>=UE_ARRAY_COUNT(OperationLightsV842))return;
    const auto& P=OperationLightsV842[Operation];
    L.bAuthoredLightingV84=true;L.LightProfileKeyV842=P.Key;
    L.LightSeasonMonthV842=P.Month;L.LightPhaseV842=P.Phase;
    L.SunScale=P.Sun;L.SkyScale=P.Sky;L.Pitch=P.Pitch;L.Yaw=P.Yaw;L.SourceAngle=P.Angle;
    L.bCloudShadowV84=P.CloudShadow>0.f;L.CloudShadowScaleV84=P.CloudShadow;
    L.SunTint=P.SunTint;L.SkyTint=P.SkyTint;L.BackdropScale*=P.Backdrop;
}
}

FFrontlineWeatherLookV50 FrontlineWeatherLookV50(int32 Operation,float Wetness,float Visibility)
{
    FFrontlineWeatherLookV50 L;
    L.Mist=FMath::Clamp((1.f-Visibility)/.26f,0.f,1.f);
    // Reserved adapter IDs identify a dated catalog operation, never a UK/DE
    // slot. Rainfall, visibility and wetness remain owned by that operation.
    for(const auto& Mission:wfexp::Missions())
    {
        const int32 Id=(Mission.faction==wfexp::Faction::French?1000:2000)+Mission.order;
        if(Id!=Operation)continue;
        const int Month=(Mission.date/100)%100;
        if(Month<=3||Month==12)
        {
            L.Name=TEXT("Winter low light");L.CloudBase=.72f;L.SunScale=.78f;L.SkyScale=.94f;
            L.Pitch=-16.f;L.Yaw=58.f;L.SourceAngle=5.f;
            L.FogStart=13500.f;L.FogFalloff=.26f;
            L.SunTint=FLinearColor(.86f,.92f,1.f);L.SkyTint=FLinearColor(.87f,.94f,1.f);
            L.FogTint=FLinearColor(.059f,.071f,.085f);
        }
        else if(Mission.rainBase>=.5f)
        {
            L.Name=TEXT("Rain over saturated ground");L.CloudBase=.70f;L.SunScale=.87f;L.SkyScale=1.02f;
            L.Pitch=-27.f;L.Yaw=45.f;L.SourceAngle=5.8f;L.FogStart=13700.f;L.FogFalloff=.27f;
            L.SunTint=FLinearColor(.90f,.95f,1.f);L.SkyTint=FLinearColor(.89f,.95f,1.f);
            L.FogTint=FLinearColor(.061f,.071f,.080f);
        }
        else if(Mission.visibilityScale<.9f&&Mission.rainBase<.15f)
        {
            L.Name=TEXT("Ground mist");L.CloudBase=.59f;L.SunScale=.96f;L.SkyScale=.96f;
            L.Pitch=-21.f;L.Yaw=39.f;L.SourceAngle=4.8f;
            L.FogStart=13150.f;L.FogFalloff=.23f;
            L.SunTint=FLinearColor(.97f,.955f,.93f);L.SkyTint=FLinearColor(.91f,.95f,1.f);
            L.FogTint=FLinearColor(.074f,.081f,.086f);
        }
        else
        {
            L.Name=TEXT("Cold broken light");L.CloudBase=.48f;L.SunScale=1.07f;L.SkyScale=.94f;
            L.Pitch=Mission.terrain==wfexp::Terrain::Ridge?-23.f:-30.f;L.Yaw=48.f;L.SourceAngle=3.f;
            L.FogStart=14900.f;L.FogFalloff=.38f;
            L.SunTint=FLinearColor(.98f,.97f,.95f);L.SkyTint=FLinearColor(.88f,.93f,1.f);
            L.FogTint=FLinearColor(.049f,.059f,.068f);
        }
        if(!FrontlineLightingV42::IsBaseline()&&!FrontlineLightingV42::IsV84Baseline()
            &&!FrontlineLightingV42::IsV842Baseline())WarfareFRRULightingV87::Apply(Mission,L);
        return L;
    }
    switch(Operation)
    {
    case 0:case 2:case 3:case 8:case 15:
        L.Name=TEXT("Cold broken light");L.CloudBase=.40f;L.SunScale=1.10f;L.SkyScale=.92f;
        L.Pitch=Operation==8?-19.f:Operation==3?-23.f:-34.f;L.Yaw=Operation==15?52.f:36.f;L.SourceAngle=2.5f;
        L.FogStart=14900.f;L.FogFalloff=.38f;L.BackdropScale=1.025f;
        L.SunTint=FLinearColor(.98f,.97f,.95f);L.SkyTint=FLinearColor(.88f,.93f,1.f);L.FogTint=FLinearColor(.049f,.059f,.068f);break;
    case 5:case 6:case 12:case 17:
        L.Name=TEXT("Rain over saturated ground");L.CloudBase=.70f;L.SunScale=.87f;L.SkyScale=1.02f;
        L.Pitch=-27.f;L.Yaw=Operation==17?53.f:45.f;L.SourceAngle=5.8f;
        L.FogStart=13700.f;L.FogFalloff=.27f;L.BackdropScale=.97f;
        L.SunTint=FLinearColor(.90f,.95f,1.f);L.SkyTint=FLinearColor(.89f,.95f,1.f);L.FogTint=FLinearColor(.061f,.071f,.080f);break;
    case 4:case 11:case 14:case 16:
        L.Name=TEXT("Ground mist");L.CloudBase=.59f;L.SunScale=.96f;L.SkyScale=.96f;
        L.Pitch=Operation==11?-15.f:-21.f;L.Yaw=Operation==14?55.f:39.f;L.SourceAngle=4.8f;
        L.Mist=FMath::Max(.68f,L.Mist);L.FogStart=13150.f;L.FogFalloff=.23f;L.BackdropScale=.94f;
        L.SunTint=FLinearColor(.97f,.955f,.93f);L.SkyTint=FLinearColor(.91f,.95f,1.f);L.FogTint=FLinearColor(.074f,.081f,.086f);break;
    case 10:
        L.Name=TEXT("Winter low light");L.CloudBase=.72f;L.SunScale=.78f;L.SkyScale=.94f;
        L.Pitch=-12.f;L.Yaw=58.f;L.SourceAngle=5.f;L.Mist=FMath::Max(.60f,L.Mist);
        L.FogStart=13500.f;L.FogFalloff=.26f;L.BackdropScale=.91f;
        L.SunTint=FLinearColor(.86f,.92f,1.f);L.SkyTint=FLinearColor(.87f,.94f,1.f);L.FogTint=FLinearColor(.059f,.071f,.085f);break;
    case 7:
        // Menin's raised road and collapsed banks need a raking key. Preserve
        // the fixed exposure and photographed horizon while easing uniform fill.
        L.Name=TEXT("Menin broken light");L.CloudBase=.58f;L.SunScale=1.07f;L.SkyScale=.94f;
        L.Pitch=-20.f;L.Yaw=55.f;L.SourceAngle=3.f;
        L.FogStart=14200.f;L.FogFalloff=.30f;
        L.SunTint=FLinearColor(.96f,.97f,.96f);L.SkyTint=FLinearColor(.90f,.945f,1.f);
        L.FogTint=FLinearColor(.060f,.069f,.077f);break;
    default:
        L.CloudBase=FMath::Lerp(.53f,.65f,FMath::Clamp(Wetness,0.f,1.f));
        L.Pitch=-26.f;L.Yaw=Operation==13?50.f:42.f;break;
    }
    // These new registered panoramas are unlit photographs, whereas the
    // adjacent clay is physically shaded. Bring their low horizon into the
    // same overcast tonal family without changing world exposure, RT quality
    // or the terrain's normal response. Verify the contact in matched 4K views.
    if(Operation==7)L.BackdropScale*=.94f;
    if(Operation==12)L.BackdropScale*=.90f;
    if(Operation==13)L.BackdropScale*=.90f;
    if(!FrontlineLightingV42::IsBaseline()&&!FrontlineLightingV42::IsV84Baseline())
    {
        // The orthographic camera looks from +Y toward -Y. These modest
        // side-front keys light the visible uniform planes without flattening
        // the banks. Do not infer illuminance ratios from Sky/Sun multipliers:
        // the captured HDRI and directional light have different radiometry.
        // Preserve the first accepted trio for the V84.2 before/after route.
        switch(Operation)
        {
        case 0: // Arras: a defined cold break in the cloud.
            L.bAuthoredLightingV84=true;L.SunScale=1.45f;L.SkyScale=.88f;
            L.Pitch=-22.f;L.Yaw=-16.f;L.SourceAngle=1.25f;
            L.CloudShadowScaleV84=.65f;break;
        case 5: // Pilckem: soft overcast, with a readable directional key.
            L.bAuthoredLightingV84=true;L.SunScale=1.03f;L.SkyScale=.93f;
            L.Pitch=-29.f;L.Yaw=-12.f;L.SourceAngle=3.6f;
            L.bCloudShadowV84=false;break;
        case 14: // Amiens: preserve dry ground fog and its visibility limit.
            L.bAuthoredLightingV84=true;L.SunScale=1.16f;L.SkyScale=.90f;
            L.Pitch=-18.f;L.Yaw=-8.f;L.SourceAngle=3.f;
            L.bCloudShadowV84=false;break;
        default:break;
        }
        if(!FrontlineLightingV42::IsV842Baseline())ApplyOperationLightV842(Operation,L);
    }
    return L;
}

void RunFrontlineWeatherChecksV50(TFunctionRef<void(const FString&,bool)> Check)
{
    TSet<FString> Looks;bool Bounded=true,Authored=true;
    for(int32 F=0;F<2;++F)for(const auto& M:AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction(F)))
    {
        const auto L=FrontlineWeatherLookV50(M.Id,M.Wetness,M.VisibilityScale);Looks.Add(L.Name);
        // Sanity bounds are a corruption guard, never an artistic standard.
        Bounded&=FMath::IsFinite(L.SunScale)&&L.SunScale>.0f&&L.SunScale<=2.5f
            &&FMath::IsFinite(L.SkyScale)&&L.SkyScale>.0f&&L.SkyScale<=1.5f
            &&L.SourceAngle>=1.f&&L.SourceAngle<=6.f&&L.FogStart>=13150&&L.FogStart<=14900&&L.Mist>=0&&L.Mist<=1.f;
        if(M.Id==10)Authored&=FString(L.Name)==TEXT("Winter low light")&&L.Pitch==-12;
        if(M.Id==11||M.Id==14)Authored&=FString(L.Name)==TEXT("Ground mist")&&M.RainBase<.04f;
        if(M.Id==5||M.Id==6)Authored&=FString(L.Name)==TEXT("Rain over saturated ground")&&M.RainBase>.6f;
    }
    Check(TEXT("v50_weather_six_distinct_operation_looks_bounded_lighting_and_existing_volume_budget"),Looks.Num()==6&&Bounded);
    Check(TEXT("v50_weather_distinguishes_dry_fog_wet_rain_and_february_low_light"),Authored);
}

void RunFrontlineWeatherChecksV842(TFunctionRef<void(const FString&,bool)> Check)
{
    if(FrontlineLightingV42::IsBaseline()||FrontlineLightingV42::IsV84Baseline()
        ||FrontlineLightingV42::IsV842Baseline())return;
    TSet<FString> Keys,Recipes;
    bool Seasons=true,SharedSector=true,WetOvercast=true,DryMist=true,Night=true;
    const auto& UK=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    const auto& DE=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::German);
    Check(TEXT("v842_lighting_twenty_available_sector_profiles"),UK.Num()==20&&DE.Num()==20);
    for(const auto& M:UK)
    {
        const auto L=FrontlineWeatherLookV50(M.Id,M.Wetness,M.VisibilityScale);
        Keys.Add(L.LightProfileKeyV842);
        Recipes.Add(FString::Printf(TEXT("%.3f|%.3f|%.1f|%.1f|%.2f"),L.SunScale,L.SkyScale,L.Pitch,L.Yaw,L.SourceAngle));
        const bool Bounded=L.bAuthoredLightingV84&&L.SunScale>=.25f&&L.SunScale<=1.6f
            &&L.SkyScale>=.35f&&L.SkyScale<=1.1f&&L.Yaw<0.f&&L.Yaw>=-30.f
            &&L.Pitch<=-8.f&&L.Pitch>=-36.f&&L.SourceAngle>=1.f&&L.SourceAngle<=4.5f
            &&L.CloudShadowScaleV84>=0.f&&L.CloudShadowScaleV84<=.70f
            &&L.BackdropScale>.35f&&L.BackdropScale<=1.10f;
        Check(FString::Printf(TEXT("v842_lighting_operation_%02d_authored_and_bounded"),M.Id),Bounded);
        Seasons&=L.LightSeasonMonthV842==(M.CalendarDateV41/100)%100
            &&FString(L.LightProfileKeyV842)==M.MapKey;
        if(DE.IsValidIndex(M.Id))
        {
            const auto& Enemy=DE[M.Id];const auto Other=FrontlineWeatherLookV50(Enemy.Id,Enemy.Wetness,Enemy.VisibilityScale);
            Seasons&=Other.LightSeasonMonthV842==(Enemy.CalendarDateV41/100)%100;
            SharedSector&=FString(L.LightProfileKeyV842)==Other.LightProfileKeyV842
                &&L.SunScale==Other.SunScale&&L.SkyScale==Other.SkyScale&&L.Pitch==Other.Pitch&&L.Yaw==Other.Yaw
                &&L.SunTint==Other.SunTint&&L.SkyTint==Other.SkyTint;
        }
        else SharedSector=false;
        if(M.RainBase>.4f)WetOvercast&=!L.bCloudShadowV84&&L.SourceAngle>=3.6f;
        if(M.Id==11||M.Id==14)DryMist&=!L.bCloudShadowV84&&M.RainBase<.04f&&L.Mist>.68f;
        if(M.Id==10)Night&=FString(L.LightPhaseV842)==TEXT("Night")&&L.SunScale<=.40f
            &&L.SkyScale<=.5f&&L.BackdropScale<.45f&&!L.bCloudShadowV84;
    }
    Check(TEXT("v842_lighting_each_operation_has_a_distinct_key_and_recipe"),Keys.Num()==20&&Recipes.Num()==20);
    Check(TEXT("v842_lighting_seasons_match_actual_operation_calendar"),Seasons);
    Check(TEXT("v842_lighting_sector_look_retained_when_player_faction_changes"),SharedSector);
    Check(TEXT("v842_lighting_wet_overcast_has_soft_key_without_patterned_cloud_occlusion"),WetOvercast);
    Check(TEXT("v842_lighting_michael_and_amiens_keep_dry_fog"),DryMist);
    Check(TEXT("v842_lighting_winter_patrol_uses_low_moon_fill_and_matched_horizon"),Night);
    const auto Arras=FrontlineWeatherLookV50(0,.64f,.91f),Rain=FrontlineWeatherLookV50(5,.97f,.8f),Fog=FrontlineWeatherLookV50(14,.48f,.78f);
    Check(TEXT("v842_lighting_retains_three_visually_accepted_v84_keys"),Arras.SunScale==1.45f&&Arras.SkyScale==.88f&&Arras.Pitch==-22.f&&Arras.Yaw==-16.f&&Arras.SourceAngle==1.25f
        &&Rain.SunScale==1.03f&&Rain.SkyScale==.93f&&Rain.Pitch==-29.f&&Rain.Yaw==-12.f&&Rain.SourceAngle==3.6f
        &&Fog.SunScale==1.16f&&Fog.SkyScale==.90f&&Fog.Pitch==-18.f&&Fog.Yaw==-8.f&&Fog.SourceAngle==3.f);
}

AWarfareFrontlineWeather::AWarfareFrontlineWeather()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bTickEvenWhenPaused=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Weather"));
}
AWarfareFrontlineWeather* AWarfareFrontlineWeather::Get(UWorld* World)
{
    if(World)for(TActorIterator<AWarfareFrontlineWeather> It(World);It;++It)return *It;
    return nullptr;
}
void AWarfareFrontlineWeather::BeginPlay()
{
    Super::BeginPlay();
    // Weather belongs to the operation; old personal weather preferences are ignored.
    Mode=EFrontlineWeatherMode::Campaign;
}
void AWarfareFrontlineWeather::BindScene()
{
    if(bBound)return;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        if(auto* C=It->FindComponentByClass<UDirectionalLightComponent>())Sun=C;
        if(auto* C=It->FindComponentByClass<USkyLightComponent>())Sky=C;
        if(auto* C=It->FindComponentByClass<UExponentialHeightFogComponent>())Fog=C;
        if(auto* G=Cast<AWarfareFrontlineGround>(*It))
        {
            // Terrain owns its current material generation; weather only drives
            // that displayed instance instead of silently downgrading it to V23.
            if(G->Surface)
            {
                if(FrontlineLightingV42::IsBaseline())
                {
                    // The matched reference must reproduce V41's old V23 ->
                    // V30 migration, including maps saved with earlier materials.
                    if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V23/Materials/MI_GroundMud_V23.MI_GroundMud_V23")))
                        GroundMID=G->Surface->CreateDynamicMaterialInstance(0,Mat);
                }
                else
                {
                    GroundMID=Cast<UMaterialInstanceDynamic>(G->Surface->GetMaterial(0));
                    if(!GroundMID)GroundMID=G->Surface->CreateDynamicMaterialInstance(0);
                }
            }
        }
        if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            if(auto* M=It->FindComponentByClass<UStaticMeshComponent>())
            {
                // This camera-facing photograph supplies composition, not
                // physical sky radiance. Its opaque plane otherwise intercepts
                // low-angle water reflections before they reach the sky.
                M->SetVisibleInRayTracing(false);
                M->SetAffectDistanceFieldLighting(false);
                M->SetAffectDynamicIndirectLighting(false);
                M->SetAffectIndirectLightingWhileHidden(false);
                M->SetCastShadow(false);M->SetCastHiddenShadow(false);
                M->SetEmissiveLightSource(false);
                // UE 5.8 exposes these capture/reflection flags without setters.
                M->bVisibleInReflectionCaptures=false;
                M->bVisibleInRealTimeSkyCaptures=false;
                M->bVisibleInReflections=false;
                M->MarkRenderStateDirty();
                // Isolated art review: check whether a regional landform still
                // reads without the photographed matte supplying its silhouette.
                // Both flags are required so ordinary play is unaffected.
                if(FParse::Param(FCommandLine::Get(),TEXT("WFV55NoDistantMatte"))
                    &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
                    M->SetVisibility(false);
                auto* Current=M->GetMaterial(0);
                const UMaterial* Base=Current?Current->GetBaseMaterial():nullptr;
                if(Base&&(Base->GetName()==TEXT("M_DistantFront_V50")||Base->GetName()==TEXT("M_DistantFront_V56")||Base->GetName()==TEXT("M_DistantFront_V64")))
                {
                    BackdropMID=Cast<UMaterialInstanceDynamic>(Current);
                    if(!BackdropMID)BackdropMID=M->CreateDynamicMaterialInstance(0);
                }
                else if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Weather/V19/Materials/M_DistantWeather_V19.M_DistantWeather_V19")))
                    BackdropMID=M->CreateDynamicMaterialInstance(0,Mat);
            }
    }
    if(Sun)
    {
        Sun->SetMobility(EComponentMobility::Movable);
        // Match the authored brighter bank of cloud behind the left of the
        // battlefield. Camera-facing frontal light flattened every mud ridge;
        // this broad rear-quarter key separates wet lips, helmets and loose clay.
        Sun->SetWorldRotation(FRotator(-28.f,42.f,0.f));
        Sun->SetLightSourceAngle(4.f);
        Sun->SetIndirectLightingIntensity(1.f);
        Sun->SetVolumetricScatteringIntensity(.35f);
        // VSM already resolves the cloth and fractured timber geometry. A
        // second 35 cm screen-space ray exceeded the complete sack height and
        // added unstable dark halos on each bag. Keep the existing non-VSM
        // compatibility contact pass only where it is actually needed.
        Sun->ContactShadowLength=GetWorld()->GetFeatureLevel()>=ERHIFeatureLevel::SM6?0.f:35.f;
        Sun->ContactShadowLengthInWS=true;
        if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Weather/V19/Materials/M_CloudShadow_V19.M_CloudShadow_V19")))
        {CloudMID=UMaterialInstanceDynamic::Create(Mat,this);Sun->SetLightFunctionMaterial(CloudMID);}
    }
    if(Sky)
    {
        Sky->SetMobility(EComponentMobility::Movable);Sky->SetLightColor(FLinearColor(.86f,.91f,1.f));
        // Capture the already-authored linear overcast HDRI at sufficient
        // angular resolution for shallow-pool reflections. One queued capture
        // at scene binding, never a recurring gameplay recapture.
        const int32 SkyResolution=GetWorld()->GetFeatureLevel()>=ERHIFeatureLevel::SM6?512:128;
        if(Sky->CubemapResolution!=SkyResolution){Sky->CubemapResolution=SkyResolution;Sky->SetCaptureIsDirty();}
    }
    if(Fog)
    {
        Fog->SetVolumetricFog(true);Fog->SetFogHeightFalloff(.30f);
        Fog->SetVolumetricFogStartDistance(13800);Fog->SetVolumetricFogNearFadeInDistance(1600);
        Fog->SetVolumetricFogDistance(22000);Fog->SetStartDistance(14200);
        Fog->SetVolumetricFogScatteringDistribution(.15f);Fog->SetVolumetricFogAlbedo(FColor(126,136,141));
        Fog->SetVolumetricFogEmissive(FLinearColor::Black);
    }
    Grade=GetWorld()->SpawnActor<APostProcessVolume>();
    Grade->bUnbound=true;Grade->Priority=100.f;Grade->BlendWeight=1.f;
    auto& S=Grade->Settings;
    S.bOverride_DynamicGlobalIlluminationMethod=true;S.DynamicGlobalIlluminationMethod=EDynamicGlobalIlluminationMethod::Lumen;
    S.bOverride_ReflectionMethod=true;S.ReflectionMethod=EReflectionMethod::Lumen;
    S.bOverride_AutoExposureMinBrightness=S.bOverride_AutoExposureMaxBrightness=S.bOverride_AutoExposureBias=true;
    S.AutoExposureMinBrightness=S.AutoExposureMaxBrightness=0.f;S.AutoExposureBias=.65f;
    S.bOverride_LumenSceneDetail=true;S.LumenSceneDetail=1.5f;
    S.bOverride_LumenFinalGatherQuality=true;S.LumenFinalGatherQuality=1.0f;
    S.bOverride_LumenSkylightLeaking=true;S.LumenSkylightLeaking=0.f;
    S.bOverride_LumenDiffuseColorBoost=true;S.LumenDiffuseColorBoost=1.f;
    S.bOverride_LumenReflectionQuality=true;S.LumenReflectionQuality=1.2f;
    S.bOverride_LumenMaxRoughnessToTraceReflections=true;S.LumenMaxRoughnessToTraceReflections=.42f;
    FrontlineLightingV42::ApplyGrade(S);
    FrontlineFilmV32::Apply(S);
    S.bOverride_BloomIntensity=true;S.BloomIntensity=.18f;
    Precipitation=GetWorld()->SpawnActor<AWarfareFrontlinePrecipitation>();
    bBound=true;EvaluateTarget();Rain=TargetRain;Cloud=TargetCloud;
    ApplyWeather();
    const auto* MaterialMode=IConsoleManager::Get().FindConsoleVariable(TEXT("wf.Materials.V27"));
    FrontlineMaterialV27::ApplyWorld(GetWorld(),!MaterialMode || MaterialMode->GetInt()!=0);
    FrontlineLightingV42::ObserveReviewWorld(GetWorld());
    UE_LOG(LogTemp,Display,TEXT("V19_WEATHER_BOUND ready=%d sun=%d sky=%d fog=%d wetGround=%d backdrop=%d"),IsReady(),!!Sun,!!Sky,!!Fog,!!GroundMID,!!BackdropMID);
}
void AWarfareFrontlineWeather::RebindGroundMaterial()
{
    for(TActorIterator<AWarfareFrontlineGround> It(GetWorld());It;++It)
        if(It->Surface)
        {
            GroundMID=Cast<UMaterialInstanceDynamic>(It->Surface->GetMaterial(0));
            if(!GroundMID)GroundMID=It->Surface->CreateDynamicMaterialInstance(0);
            if(GroundMID)GroundMID->SetScalarParameterValue(TEXT("WeatherWetness"),Wetness);
            It->SyncTerrainMaterialV43();
            break;
        }
    // Terrain can select a regional panorama after weather initially binds.
    // Follow its displayed MID; never reinstate the old generic sky material.
    for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
        if(auto* M=It->FindComponentByClass<UStaticMeshComponent>())
        {
            BackdropMID=Cast<UMaterialInstanceDynamic>(M->GetMaterial(0));
            if(!BackdropMID)BackdropMID=M->CreateDynamicMaterialInstance(0);
            break;
        }
    ApplyWeather();
}
bool AWarfareFrontlineWeather::UsesDisplayedGroundMaterial() const
{
    if(!GroundMID)return false;
    for(TActorIterator<AWarfareFrontlineGround> It(GetWorld());It;++It)
        if(It->Surface)return It->Surface->GetMaterial(0)==GroundMID.Get();
    return false;
}
void AWarfareFrontlineWeather::SetMode(EFrontlineWeatherMode Value,bool bSave)
{
    // Retained solely for isolated visual/performance fixtures. No player-facing control or save.
    const FString Command=FCommandLine::Get();
    const bool Review=Command.Contains(TEXT("Review"))||Command.Contains(TEXT("Benchmark"));
    Mode=Review?EFrontlineWeatherMode(FMath::Clamp(int32(Value),0,3)):EFrontlineWeatherMode::Campaign;
    EvaluateTarget();
}
void AWarfareFrontlineWeather::EvaluateTarget()
{
    const auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const auto* Mission=B?B->GetEncounterCampaignMission():nullptr;
    if(!Mission && B)Mission=B->GetSelectedCampaignMission();
    OperationLook=FrontlineWeatherLookV50(Mission?Mission->Id:0,Mission?Mission->Wetness:.6f,Mission?Mission->VisibilityScale:1.f);
    if(Mode==EFrontlineWeatherMode::Overcast){TargetRain=0;TargetCloud=.57f;return;}
    if(Mode==EFrontlineWeatherMode::Drizzle){TargetRain=.30f;TargetCloud=.73f;return;}
    if(Mode==EFrontlineWeatherMode::Rain){TargetRain=.85f;TargetCloud=.96f;return;}
    const float Phase=(Mission?Mission->Id:0)*1.7f+(B?B->Elapsed:Clock)/95.f;
    const float Base=Mission?Mission->RainBase:.18f,Variation=Mission?Mission->RainVariation:.08f;
    TargetRain=FMath::Clamp(Base+Variation*(.72f*FMath::Sin(Phase)+.28f*FMath::Sin(Phase*.39f)),0.f,.96f);
    TargetCloud=FrontlineLightingV42::IsBaseline()?FMath::Clamp(.64f+TargetRain*.32f,.64f,.98f)
        :FMath::Clamp(OperationLook.CloudBase+TargetRain*.30f,.38f,.98f);
    if(Mission && LastMissionSeed!=Mission->RandomSeed)
    {
        LastMissionSeed=Mission->RandomSeed;Rain=TargetRain;Cloud=TargetCloud;
        Wetness=FMath::Clamp(Mission->Wetness,0.f,1.f);
        UE_LOG(LogTemp,Display,TEXT("WF_V50_WEATHER operation=%d look=%s cloud=%.3f rain=%.3f wetness=%.3f sun_scale=%.3f sky_scale=%.3f pitch=%.1f fog_start=%.0f"),
            Mission->Id,OperationLook.Name,Cloud,Rain,Wetness,OperationLook.SunScale,OperationLook.SkyScale,OperationLook.Pitch,OperationLook.FogStart);
    }
}
void AWarfareFrontlineWeather::Tick(float DT)
{
    Super::Tick(DT);if(!GetWorld()->HasBegunPlay())return;
    if(!bBound)BindScene();
    if(Grade)FrontlineFilmV32::Apply(Grade->Settings);
    const bool Paused=GetWorld()->IsPaused();
    const float RealDT=FMath::Clamp(float(FApp::GetDeltaTime()),0.f,.1f);
    if(!Paused)Clock+=RealDT;
    EvaluateTarget();
    Rain=FMath::FInterpTo(Rain,TargetRain,RealDT,.45f);
    Cloud=FMath::FInterpTo(Cloud,TargetCloud,RealDT,.30f);
    const auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    const auto* Mission=B?B->GetEncounterCampaignMission():nullptr;
    if(!Mission && B)Mission=B->GetSelectedCampaignMission();
    const float WetTarget=Mode==EFrontlineWeatherMode::Campaign?
        FMath::Clamp((Mission?Mission->Wetness:.6f)+(Rain-(Mission?Mission->RainBase:.18f))*.16f,0.f,1.f):.58f+Rain*.38f;
    Wetness=FMath::FInterpTo(Wetness,WetTarget,RealDT,WetTarget>Wetness?.035f:.008f);
    UpdateClock+=RealDT;if(UpdateClock<.1f)return;UpdateClock=0;
    ApplyWeather();
}
void AWarfareFrontlineWeather::ApplyWeather()
{
    const float Storm=FMath::Clamp((Cloud-.57f)/.39f,0.f,1.f);
    const float Gust=.90f+.10f*FMath::Sin(Clock*.15f);
    const bool Baseline=FrontlineLightingV42::IsBaseline()||Mode!=EFrontlineWeatherMode::Campaign;
    if(Baseline)FrontlineLightingV42::ApplyLights(Sun,Sky,Storm);
    else
    {
        // Different hours and weather share the accepted exposure, output gamut
        // and quality settings. No sky recapture, RT-mode change or extra lights.
        float Pitch=OperationLook.Pitch,Yaw=OperationLook.Yaw;
        float SunIntensity=FMath::Lerp(2.55f,1.90f,Storm)*OperationLook.SunScale*(FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline"))?1.f:1.12f);
        float SkyIntensity=FMath::Lerp(.30f,.28f,Storm)*OperationLook.SkyScale*(FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline"))?1.f:1.18f);
        FrontlineArtV68::AdjustLight(Pitch,Yaw,SunIntensity,SkyIntensity);
        if(Sun)
        {
            Sun->SetIntensity(SunIntensity);
            Sun->SetLightColor(OperationLook.SunTint);
            Sun->SetWorldRotation(FRotator(Pitch,Yaw,0));
            Sun->SetLightSourceAngle(OperationLook.SourceAngle);
            // Rain and ground mist already soften the key. A second patterned
            // attenuation makes their form response less coherent. Broken
            // cloud retains its existing broad light function at reduced gain.
            UMaterialInterface* LightFunction=OperationLook.bCloudShadowV84?CloudMID.Get():nullptr;
            if(Sun->LightFunctionMaterial!=LightFunction)Sun->SetLightFunctionMaterial(LightFunction);
        }
        if(Sky){Sky->SetIntensity(SkyIntensity);Sky->SetLightColor(OperationLook.SkyTint);}
    }
    if(Fog)
    {
        const auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
        const auto* Mission=B?B->GetEncounterCampaignMission():nullptr;
        if(!Mission && B)Mission=B->GetSelectedCampaignMission();
        const float Mist=Baseline?(Mission?FMath::Clamp((1.f-Mission->VisibilityScale)/.22f,0.f,1.f):0.f):OperationLook.Mist;
        Fog->SetFogDensity(FMath::Lerp(Baseline?.009f:.0065f,.018f,Storm)+Mist*(Baseline?.008f:.010f));
        Fog->SetFogMaxOpacity(FMath::Lerp(Baseline?.30f:.26f,.43f,Storm)+Mist*.08f);
        Fog->SetFogInscatteringColor(Baseline?FLinearColor(.060f,.069f,.077f):OperationLook.FogTint);
        Fog->SetFogHeightFalloff(Baseline?.30f:OperationLook.FogFalloff);
        const float Start=Baseline?14200.f:OperationLook.FogStart;
        Fog->SetStartDistance(Start);Fog->SetVolumetricFogStartDistance(Start-400.f);
        Fog->SetVolumetricFogExtinctionScale(FMath::Lerp(.70f,1.10f,Storm));
    }
    if(GroundMID)GroundMID->SetScalarParameterValue(TEXT("WeatherWetness"),Wetness);
    if(const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());WarfareFRRUCampaignV82::Active(Battle))
    {
        WarfareFRRUContentV82::ApplyWeatherWetness(Battle,Wetness);
        WarfareFRRUContentV82::ApplyWeatherDistant(Battle,FrontlineLightingV42::BackdropGain(Storm)*(Baseline?1.f:OperationLook.BackdropScale),Cloud,Clock);
    }
    if(BackdropMID)
    {
        BackdropMID->SetScalarParameterValue(TEXT("WeatherSkyGain"),FrontlineLightingV42::BackdropGain(Storm)*(Baseline?1.f:OperationLook.BackdropScale));
        BackdropMID->SetScalarParameterValue(TEXT("WeatherCloudCover"),Cloud);
        BackdropMID->SetScalarParameterValue(TEXT("WeatherTime"),Clock);
    }
    if(CloudMID){CloudMID->SetScalarParameterValue(TEXT("WeatherCloudCover"),Cloud*(Baseline?1.f:OperationLook.CloudShadowScaleV84));CloudMID->SetScalarParameterValue(TEXT("WeatherTime"),Clock);}
    if(Precipitation)Precipitation->SetWeather(Rain,FVector((85+Rain*200)*Gust,-35-Rain*45,0));
    if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->SetRainAmount(Rain);
}
float AWarfareFrontlineWeather::GetSunIntensity() const{return Sun?Sun->Intensity:0;}
float AWarfareFrontlineWeather::GetSkyIntensity() const{return Sky?Sky->Intensity:0;}
bool AWarfareFrontlineWeather::IsReady() const{return Sun&&Sky&&Fog&&Grade&&GroundMID&&BackdropMID&&CloudMID&&Precipitation&&Precipitation->IsVisualReady();}
void AWarfareFrontlineWeather::SnapToWeatherForReview()
{
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84Review")))
    {
        FString UserDir;const auto* B=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
        if(!B || !B->IsProgressionVolatile() || !FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")) ||
            !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty())return;
        if(!bBound)BindScene();
        Mode=EFrontlineWeatherMode::Campaign;Clock=UpdateClock=0;LastMissionSeed=INDEX_NONE;
        EvaluateTarget();Rain=TargetRain;Cloud=TargetCloud;ApplyWeather();return;
    }
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV19WeatherReview")))return;
    EvaluateTarget();Rain=TargetRain;Cloud=TargetCloud;Wetness=.58f+Rain*.38f;ApplyWeather();
}
