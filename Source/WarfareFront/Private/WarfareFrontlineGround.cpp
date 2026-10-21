#include "WarfareFrontlineGround.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineTerrainProfilesV51.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineBattlefield.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MeshDescription.h"
#include "MeshUVChannelInfo.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#include "MeshCardBuild.h"
#include "Rendering/NaniteResources.h"
#include "Serialization/JsonSerializer.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineScanHeightV47.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "DistanceFieldAtlas.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "UObject/SavePackage.h"
#endif

namespace
{
struct FFieldCrater { float X,Y,RX,RY,Depth; bool Wet; };
// Composed as a few dominant scars and flatter subsidiary hollows, not a grid of bowls.
const FFieldCrater FieldCraters[]={
    {-1700,180,460,290,88,true},{1550,-170,265,195,53,false},
    {-4550,600,330,245,91,true},{4700,-650,460,220,72,false},
    {-1250,1500,570,310,60,true},{2200,2200,340,230,65,false},
    {-5200,-1800,390,200,73,false},{3100,-2400,220,170,44,false},
    {4400,3700,360,230,80,false},{-800,-980,470,235,67,true},
    {1850,-1500,315,200,53,true},{-2250,1080,270,160,39,false},
    {3650,1450,440,260,69,true},{-5800,1500,350,180,43,false},
    {600,-2500,430,205,49,true},
    {-10800,300,385,270,73,true},{-6550,-240,505,285,79,true},
    {-5400,680,260,195,46,false},{6200,230,425,295,83,true},
    {9950,-340,330,210,60,false},{11200,950,510,300,54,true},
    {-9250,1730,590,285,67,false},{-7350,-2280,440,230,58,false},
    {7450,1690,510,320,76,true},{9550,-1960,470,245,63,false},
    {-11700,-1450,345,205,47,false},{12400,-2700,580,320,70,false}};
// Shallow linked hollows in the apron. Each adjacent pair shares one water
// elevation; their overlap produces a drainage shape instead of more bowls.
const FFieldCrater DrainageHollowsV47[]={
    {-1330,1940,570,330,34,true},{-1730,2200,630,255,27,true},
    {1280,1540,515,315,34,true},{1690,1880,595,280,28,true},
    {-1520,-1890,620,310,37,true},{-1120,-2180,530,305,30,true},
    {5140,2270,680,300,34,true},{5510,2510,570,255,25,true}};
float Ease(float T) { T=FMath::Clamp(T,0.f,1.f); return T*T*(3-2*T); }
// Chosen from the complete visible tile set before building collision. It is
// atomic because foot/vehicle height queries may run outside the game thread.
TAtomic<bool> GMeninV58Height{true};
TAtomic<bool> GMeninV59Height{false};
// V63 water and its stained ground vertices are an explicit review variant.
// Both variants use the unchanged V59 physical height/collision function.
TAtomic<bool> GMeninV63Water{false};
// Lys must use the same version for visible tiles, hidden collision, water
// clipping and all foot/vehicle queries. An incomplete V58 bake selects V57.
TAtomic<bool> GLysV58Height{true};
// St Quentin is seen along the ruined bridge, not as a transverse ditch.
// One section drives the collision surface, traversal mask and narrow water
// reach; changing the camera cannot rearrange any of these physical forms.
struct FStQuentinSectionV55
{
    float Axis=0,Length=0,Distance=0,Gap=0,Bed=0,NearBank=0,FarBank=0,Towpath=0;
};
FStQuentinSectionV55 StQuentinSectionV55(float X,float Y,float Home)
{
    FStQuentinSectionV55 S;
    // At the near firing line the cut enters left of frame. Its far reach
    // points into the photographed bridge, instead of running parallel to
    // the flat panorama seam. Keep the engineered bearing mostly straight.
    // Set the crossing between, rather than directly under, the staggered
    // trench works. At the old bearing one trench's protection erased the
    // northern cut and made every tank choose that single free passage.
    S.Axis=600.f+.24f*Home-.70f*(Y+1450.f)+90.f*FMath::Sin(Y/1450.f+1.1f);
    S.Distance=X-S.Axis;
    // Carry the cut beneath the distant skyline instead of tapering it to a
    // dry end in the visible 3D apron once the unregistered photo water fades.
    S.Length=(1-Ease((Y-1050.f)/1050.f))*(1-Ease((-Y-6500.f)/950.f));
    const float GapNear=FMath::Exp(-FMath::Square((Y-850.f)/390.f));
    // Both breaches must be inside the playable +/-1300 cm frontage. The old
    // second opening at -2600 cm was only scenery and offered no route choice.
    const float GapFar=FMath::Exp(-FMath::Square((Y+850.f)/470.f));
    S.Gap=FMath::Max(GapNear,GapFar);
    const float Across=FMath::Abs(S.Distance);
    const float BedHalfWidth=570.f+35.f*FMath::Sin(Y/970.f+.8f);
    S.Bed=S.Length*(1-Ease((Across-BedHalfWidth)/720.f));
    // Damaged opposing banks still retain a continuous cut silhouette.
    // Their unequal crests and a raised remnant towpath give the section a
    // clear near/far edge at the normal, wide gameplay camera.
    const float Battered=.82f+.18f*FMath::Sin(Y/570.f+.3f)*FMath::Sin(Y/213.f+1.2f);
    const float BankGap=1-.82f*S.Gap;
    S.NearBank=S.Length*Battered*BankGap*FMath::Exp(-FMath::Square((S.Distance+1470.f)/315.f));
    S.FarBank=S.Length*Battered*BankGap*FMath::Exp(-FMath::Square((S.Distance-1510.f)/340.f));
    S.Towpath=S.Length*(1-.76f*S.Gap)*Ease((S.Distance-1170.f)/185.f)
        *(1-Ease((S.Distance-1930.f)/230.f));
    return S;
}
// Messines' mined reverse slope is not the same uninterrupted chalk hogback
// used for the later Menin Road operation. These broad overlapping craters
// are excavated from the collision surface, including the route-height query;
// the trench and water-bed protection below still controls their edges.
struct FMessinesMineV55 {float Cut=0,Rim=0;};
FMessinesMineV55 MessinesMineV55(float X,float Y,int32 Op,float Home)
{
    FMessinesMineV55 Result;
    if(Op!=3&&Op!=4)return Result;
    auto Pit=[&](float CX,float CY,float RX,float RY,float Angle,float Seed)
    {
        const float DX=X-CX,DY=Y-CY;
        const float U=(DX*FMath::Cos(Angle)+DY*FMath::Sin(Angle))/RX;
        const float V=(-DX*FMath::Sin(Angle)+DY*FMath::Cos(Angle))/RY;
        const float D=FMath::Sqrt(U*U+V*V);
        const float Bearing=FMath::Atan2(V,U);
        const float Ragged=.77f+.23f*FMath::Sin(Bearing*3.f+Seed)*FMath::Cos(Bearing*5.f-.7f);
        Result.Cut=FMath::Max(Result.Cut,1-Ease((D-.10f)/.86f));
        Result.Rim=FMath::Max(Result.Rim,Ragged*FMath::Exp(-FMath::Square((D-.97f)/.19f)));
    };
    const float Counter=Op==4?175.f:0.f;
    Pit(-.10f*Home+Counter,435.f,1110.f,610.f,.13f,1.4f);
    Pit(.39f*Home-Counter,-1190.f,970.f,650.f,-.23f,2.9f);
    return Result;
}
// A third Ypres drainage reach passes between the two older watercourses.
// Its analytic bed and shoreline are shared by geometry, water and terrain
// masks, so there is no painted wet stripe over a physically flat crossing.
struct FYpresBraidV55 {float Axis=0,Span=0,Bed=0,Levee=0,ShoreHalfWidth=0;};
FYpresBraidV55 YpresBraidV55(float X,float Y,float Home)
{
    FYpresBraidV55 B;
    B.Axis=-1260.f+.11f*X+145.f*FMath::Sin(X/1190.f+.56f);
    const float D=FMath::Abs(Y-B.Axis);
    B.Span=FMath::Exp(-FMath::Square((X+210.f)/(Home*1.30f+450.f)))
        *(1-Ease((FMath::Abs(X)-Home-650.f)/1450.f));
    B.Bed=B.Span*(1-Ease((D-345.f)/645.f));
    B.Levee=B.Span*FMath::Exp(-FMath::Square((D-980.f)/345.f));
    B.ShoreHalfWidth=FMath::Max(140.f,260.f+64.f*FMath::Sin(X/1140.f+.4f)
        *FMath::Sin(X/475.f+.9f)+39.f*FMath::PerlinNoise2D(FVector2D(X/310.f+4.8f,Y/420.f)));
    return B;
}
// The Lys operation crosses a drainage line instead of reusing Ypres'
// lengthwise flooded beds. Two raised, battered fords are real geometry;
// the same section gates standing water and local movement cost.
struct FLysWashV57 {float Axis=0,Distance=0,Span=0,Ford=0,Bed=0,Bank=0,WaterHalfWidth=0;};
FLysWashV57 LysWashV57(float X,float Y,float Home)
{
    FLysWashV57 W;
    // The active river lies between the western and centre trenches; the
    // rear reach bends towards the painted river at the matte edge.
    W.Axis=-.45f*Home+.19f*Y+145.f*FMath::Sin(Y/830.f+.35f);
    W.Distance=X-W.Axis;
    W.Span=(1-Ease((Y-1750.f)/900.f))*(1-Ease((-Y-7000.f)/650.f))
        *(1-Ease((FMath::Abs(X)-Home-1700.f)/1750.f));
    W.Ford=FMath::Max(FMath::Exp(-FMath::Square((Y+720.f)/290.f)),
        FMath::Exp(-FMath::Square((Y-690.f)/315.f)));
    const float Across=FMath::Abs(W.Distance);
    W.Bed=W.Span*(1-Ease((Across-620.f)/670.f));
    W.Bank=W.Span*(1-.78f*W.Ford)
        *FMath::Exp(-FMath::Square((Across-1280.f)/345.f))
        *(.84f+.16f*FMath::Sin(Y/470.f+.2f)*FMath::Sin(Y/181.f+1.3f));
    W.WaterHalfWidth=390.f+55.f*FMath::Sin(Y/680.f+.5f)
        +35.f*FMath::PerlinNoise2D(FVector2D(X/310.f,Y/370.f+3.9f));
    return W;
}
float LysSiltBarsV58(const FLysWashV57& W,float Y)
{
    // Staggered low silt lenses, not continuous parallel embankments. Every
    // deposit is actual ground and can be cut by a later impact crater.
    if(W.Bed<=.001f||FMath::Abs(W.Distance)>720.f)return 0.f;
    auto Lens=[&](float Along,float Across,float Length,float Width)
    {
        return FMath::Exp(-FMath::Square((Y-Along)/Length)
            -FMath::Square((W.Distance-Across)/Width));
    };
    return FMath::Max3(FMath::Max(Lens(-5250.f,155.f,370.f,145.f),Lens(-3730.f,-105.f,420.f,125.f)),
        FMath::Max(Lens(-2210.f,185.f,315.f,165.f),Lens(-760.f,-190.f,330.f,135.f)),
        FMath::Max(Lens(180.f,165.f,290.f,150.f),Lens(1150.f,-70.f,350.f,135.f)));
}
float LysRunnelsV58(const FLysWashV57& W,float Y)
{
    // One sinuous main thread persists. Two secondary cuts separate only
    // through irregular reaches, joining it again rather than forming three
    // ruler-straight, parallel drainage ditches from horizon to camera.
    if(W.Bed<=.001f||FMath::Abs(W.Distance)>720.f)return 0.f;
    const float Centre=155.f*FMath::Sin(Y/700.f+.35f)+85.f*FMath::Sin(Y/280.f+1.1f);
    const float MainWidth=145.f+40.f*FMath::Sin(Y/390.f+.6f)+13.f*FMath::Sin(Y/165.f+1.2f);
    const float Main=FMath::Exp(-FMath::Square((W.Distance-Centre)/MainWidth));
    const float WestPulse=Ease((FMath::Sin(Y/540.f+.55f*FMath::Sin(Y/1200.f+.8f)+.35f)-.08f)/.72f);
    const float EastPulse=Ease((FMath::Sin(Y/450.f+.29f*FMath::Sin(Y/1020.f)-1.4f)-.08f)/.72f);
    const float West=Centre-(175.f+45.f*FMath::Sin(Y/900.f+.4f))*WestPulse;
    const float East=Centre+(175.f+35.f*FMath::Sin(Y/830.f+1.6f))*EastPulse;
    const float Left=WestPulse*FMath::Exp(-FMath::Square((W.Distance-West)/(103.f+16.f*FMath::Sin(Y/320.f))));
    const float Right=EastPulse*FMath::Exp(-FMath::Square((W.Distance-East)/(100.f+18.f*FMath::Sin(Y/370.f+.8f))));
    return FMath::Max3(Main,Left,Right);
}
// The road is a compacted, raised ribbon with two physical drainage gullies.
// Their gradual shoulders survive the wide gameplay camera and remain in the
// same positions as the travel masks and baked vertex-material semantic.
struct FMeninRoadSection {float Crest=0,Road=0,Gully=0;};
FMeninRoadSection MeninRoadV57(float X,float Y,float Home)
{
    FMeninRoadSection R;
    const float Span=(1-Ease((FMath::Abs(X)-Home-1250.f)/1850.f))
        *(1-Ease((FMath::Abs(Y)-1600.f)/1300.f));
    const float CrestAxis=X-.19f*Y-.17f*Home-155.f*FMath::Sin(Y/1230.f+.7f);
    R.Crest=Span*FMath::Exp(-FMath::Square(CrestAxis/(CrestAxis<0?1900.f:1510.f)));
    const float RoadAxis=-150.f+.115f*X+125.f*FMath::Sin(X/1450.f+.42f);
    R.Road=Span*FMath::Exp(-FMath::Square((Y-RoadAxis)/490.f));
    const float CutA=FMath::Exp(-FMath::Square((X+.36f*Home-.16f*Y)/690.f));
    const float CutB=FMath::Exp(-FMath::Square((X-.34f*Home+.11f*Y)/760.f));
    R.Gully=Span*FMath::Max(CutA,CutB)*(1-.87f*R.Road);
    return R;
}
FMeninRoadSection MeninRoadV58(float X,float Y,float Home)
{
    FMeninRoadSection R;
    const float Span=(1-Ease((FMath::Abs(X)-Home-1250.f)/1850.f))
        *(1-Ease((FMath::Abs(Y)-1600.f)/1300.f));
    const float CrestAxis=X-.19f*Y-.17f*Home-155.f*FMath::Sin(Y/1230.f+.7f);
    R.Crest=Span*FMath::Exp(-FMath::Square(CrestAxis/(CrestAxis<0?1900.f:1510.f)));
    const float RoadAxis=-150.f+.115f*X+125.f*FMath::Sin(X/1450.f+.42f);
    const float Across=Y-RoadAxis;
    // A flat, traversable 5.3 m core grades into beaten shoulders. The old
    // Gaussian crest was as lumpy as the shell field and read as no route.
    R.Road=Span*(1-Ease((FMath::Abs(Across)-265.f)/405.f));
    const float SouthAxis=925.f+90.f*FMath::Sin(X/1580.f+.8f);
    const float NorthAxis=-865.f+100.f*FMath::Sin(X/1760.f+1.4f);
    const float South=FMath::Exp(-FMath::Square((Across-SouthAxis)/340.f));
    const float North=FMath::Exp(-FMath::Square((Across-NorthAxis)/370.f));
    const float Break=.78f+.22f*FMath::PerlinNoise2D(FVector2D(X/1350.f+2.4f,Y/1800.f-1.7f));
    // The former two transverse washouts remain as weak tributaries; they no
    // longer substitute for the missing dark gullies along the road itself.
    const float CutA=FMath::Exp(-FMath::Square((X+.36f*Home-.16f*Y)/690.f));
    const float CutB=FMath::Exp(-FMath::Square((X-.34f*Home+.11f*Y)/760.f));
    R.Gully=Span*FMath::Max(FMath::Max(South,North)*Break,.32f*FMath::Max(CutA,CutB))
        *(1-.92f*R.Road);
    return R;
}
float MeninRoadAxisV59(float X)
{
    // A real oblique route crosses the camera's rear-left to near-right field.
    // The accepted V58 shallow bearing remains available with its tile set.
    return -300.f+1.20f*X+145.f*FMath::Sin(X/1950.f+.42f);
}
float MeninGullyOffsetV59(float X,bool South)
{
    return South?925.f+90.f*FMath::Sin(X/1580.f+.8f)
        :-865.f+100.f*FMath::Sin(X/1760.f+1.4f);
}
FMeninRoadSection MeninRoadV59(float X,float Y,float Home)
{
    FMeninRoadSection R;
    const float Span=(1-Ease((FMath::Abs(X)-Home-2000.f)/2500.f))
        *(1-Ease((FMath::Abs(Y)-3500.f)/1600.f));
    const float CrestAxis=X-.19f*Y-.17f*Home-155.f*FMath::Sin(Y/1230.f+.7f);
    R.Crest=Span*FMath::Exp(-FMath::Square(CrestAxis/(CrestAxis<0?1900.f:1510.f)));
    const float Across=Y-MeninRoadAxisV59(X);
    R.Road=Span*(1-Ease((FMath::Abs(Across)-270.f)/420.f));
    const float SouthAxis=MeninGullyOffsetV59(X,true);
    const float NorthAxis=MeninGullyOffsetV59(X,false);
    const float South=FMath::Exp(-FMath::Square((Across-SouthAxis)/340.f));
    const float North=FMath::Exp(-FMath::Square((Across-NorthAxis)/370.f));
    const float Break=.78f+.22f*FMath::PerlinNoise2D(FVector2D(X/1350.f+2.4f,Y/1800.f-1.7f));
    const float CutA=FMath::Exp(-FMath::Square((X+.36f*Home-.16f*Y)/690.f));
    const float CutB=FMath::Exp(-FMath::Square((X-.34f*Home+.11f*Y)/760.f));
    R.Gully=Span*FMath::Max(FMath::Max(South,North)*Break,.32f*FMath::Max(CutA,CutB))
        *(1-.92f*R.Road);
    return R;
}
float MeninSurfaceSemanticV58(float X,float Y,float Home)
{
    // Vertex B is otherwise unused by the displayed V51 soil. Encode one
    // feathered road-to-gully axis from the same section that shapes height,
    // cover and traversal: .5 ordinary clay, ~1 raised road, ~0 wet gully.
    // It cannot wander away from the real route like a detached decal.
    const auto Section=MeninRoadV58(X,Y,Home);
    const float Edge=.96f+.04f*FMath::PerlinNoise2D(FVector2D(X/310.f+4.1f,Y/270.f-2.7f));
    return FMath::Clamp(.5f+.46f*Section.Road*Edge-.43f*Section.Gully,.04f,.96f);
}
float MeninRegionalProtectionV59(float X,float Y);
float MeninSurfaceSemanticV59(float X,float Y,float Home)
{
    const auto Section=MeninRoadV59(X,Y,Home);
    const float Edge=.96f+.04f*FMath::PerlinNoise2D(FVector2D(X/310.f+4.1f,Y/270.f-2.7f));
    // The route colour follows the same bank and basin clearance as its real
    // displaced surface. It must fade where an authored trench or pool wins.
    const float Protection=MeninRegionalProtectionV59(X,Y);
    return FMath::Clamp(.5f+Protection*(.46f*Section.Road*Edge-.43f*Section.Gully),.04f,.96f);
}
float CraterDistance(const FFieldCrater& C,float X,float Y,int32 Index)
{
    const float A=Index*.79f,DX=X-C.X,DY=Y-C.Y;
    const float U=(DX*FMath::Cos(A)+DY*FMath::Sin(A))/C.RX;
    const float V=(-DX*FMath::Sin(A)+DY*FMath::Cos(A))/C.RY;
    const float Angle=FMath::Atan2(V,U);
    return FMath::Sqrt(U*U+V*V)/(1+.085f*FMath::Sin(Angle*3+Index)+.045f*FMath::Cos(Angle*7));
}
// Disconnected shallow water pockets follow the two real V59 gullies. Their
// small cuts belong to HeightAt, and their water is later solved against that
// same surface; this is not a reflective overlay on an undisturbed slope.
constexpr int32 MeninDrainSourceBaseV59=1202;
struct FMeninDrainSeedV59 {float X;bool South;float Drift,RX,RY,Depth;};
const FMeninDrainSeedV59 MeninDrainSeedsV59[]={
    {-2110.f,true,30.f,670.f,195.f,32.f},
    {-1130.f,true,-35.f,610.f,185.f,29.f},
    {-180.f,true,45.f,700.f,205.f,34.f},
    {870.f,true,-22.f,570.f,180.f,30.f},
    {1900.f,true,38.f,660.f,190.f,31.f},
    {-1640.f,false,-25.f,630.f,180.f,30.f},
    {-600.f,false,32.f,690.f,200.f,34.f},
    {380.f,false,-40.f,450.f,175.f,48.f},
    {1350.f,false,20.f,490.f,195.f,52.f},
    {2280.f,false,-28.f,450.f,180.f,38.f}};
const TArray<FFieldCrater>& MeninDrainShapesV59()
{
    static const TArray<FFieldCrater> Shapes=[]
    {
        TArray<FFieldCrater> Built;Built.Reserve(UE_ARRAY_COUNT(MeninDrainSeedsV59));
        for(const auto& Seed:MeninDrainSeedsV59)
            Built.Add({Seed.X,MeninRoadAxisV59(Seed.X)
                +MeninGullyOffsetV59(Seed.X,Seed.South)+Seed.Drift,
                Seed.RX,Seed.RY,Seed.Depth,true});
        return Built;
    }();
    return Shapes;
}
// A field of overlapping shallow impacts composes the middle-scale land.
// The hash jitters centres, axes, depth and retained wet pockets; a 3x3 local
// lookup replaces an O(site-count) scan in every soldier/mesh height query.
constexpr int32 ImpactMinXV47=-29,ImpactMaxXV47=28,ImpactMinYV47=-9,ImpactMaxYV47=10;
constexpr int32 ImpactColumnsV47=ImpactMaxXV47-ImpactMinXV47+1;
constexpr float ImpactPitchV47=520.f;
// V42 banks are re-stationed inside +/-1290 cm, boards inside +/-1405,
// and the furthest dugout is inside +/-1724 after its rotated mesh bounds.
// Keep a conservative 76 cm support margin, then release only empty apron.
constexpr float BankSupportHalfExtentV47=1800.f;
constexpr float BankReleaseWidthV47=400.f;
constexpr float AggregateScanStrengthV47=.75f;
float MeninRegionalProtectionV59(float X,float Y)
{
    float NearestExcavation=FLT_MAX;
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
        NearestExcavation=FMath::Min(NearestExcavation,
            FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y)));
    float WaterClearance=1.f;
    for(int32 I=0;I<UE_ARRAY_COUNT(FieldCraters);++I)
    {
        const auto& C=FieldCraters[I];
        if(!C.Wet||FMath::Abs(X-C.X)>3.f*FMath::Max(C.RX,C.RY)
            ||FMath::Abs(Y-C.Y)>3.f*FMath::Max(C.RX,C.RY))continue;
        WaterClearance=FMath::Min(WaterClearance,Ease((CraterDistance(C,X,Y,I)-.24f)/1.18f));
    }
    const float Bank=FMath::Max(Ease((NearestExcavation-305.f)/510.f),
        Ease((FMath::Abs(Y)-BankSupportHalfExtentV47)/700.f));
    // The drainage clearance is applied only where the original impact apron
    // evaluates those hollows. Menin is not a railway operation.
    const float Apron=FMath::Max(Ease((NearestExcavation-305.f)/145.f),
        Ease((FMath::Abs(Y)-BankSupportHalfExtentV47)/BankReleaseWidthV47));
    float DrainageClearance=1.f;
    if(Apron>0.f)
        for(int32 I=0;I<UE_ARRAY_COUNT(DrainageHollowsV47);++I)
        {
            const auto& C=DrainageHollowsV47[I];
            const float Reach=FMath::Max(C.RX,C.RY)*1.4f;
            if(FMath::Abs(X-C.X)>Reach||FMath::Abs(Y-C.Y)>Reach)continue;
            DrainageClearance=FMath::Min(DrainageClearance,
                Ease((CraterDistance(C,X,Y,UE_ARRAY_COUNT(FieldCraters)+I)-.24f)/1.18f));
        }
    return Bank*WaterClearance*DrainageClearance;
}
const TArray<FFieldCrater>& ImpactSitesV47()
{
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    // Stable per-operation scars: the physical terrain, water and feet query
    // the same field. No runtime random stream or camera-dependent layout.
    const bool MeninV59=Op==7&&GMeninV59Height.Load();
    const int32 Key=Op*2+int32(MeninV59);
    static TMap<int32,TArray<FFieldCrater>> Profiles;
    static int32 LastKey=INDEX_NONE;static const TArray<FFieldCrater>* Last=nullptr;
    if(Last&&LastKey==Key)return *Last;
    if(auto* Found=Profiles.Find(Key)){LastKey=Key;Last=Found;return *Last;}
    TArray<FFieldCrater> Sites;
    {
        for(int32 Y=ImpactMinYV47;Y<=ImpactMaxYV47;++Y)for(int32 X=ImpactMinXV47;X<=ImpactMaxXV47;++X)
        {
            auto Random=[&](uint32 Salt){return float(FrontlineScanHeightV47::Hash(X,Y,Salt+uint32(Op+1)*7919u)>>8)*(1.f/16777216.f);};
            const float SoilDepth=Style==EFrontlineMissionTerrainV32::Marsh?1.12f:Style==EFrontlineMissionTerrainV32::Ridge?.82f:1.f;
            const bool Wet=Random(227)<(Style==EFrontlineMissionTerrainV32::Marsh?.76f:
                Style==EFrontlineMissionTerrainV32::Ridge?.36f:.56f);
            // Cambrai's brief surprise barrage did not create the same long-
            // attrition carpet as Ypres. Retain all wet basins and every
            // authored dominant scar; some older dry shell marks have settled
            // nearly flush with the firm approach. Array indexing stays fixed.
            const bool SettledDry=Op==8&&!Wet&&Random(349)<.48f;
            if(MeninV59)
            {
                // Smooth, hashed district density leaves whole quiet patches
                // among battered clusters. A zero-depth entry retains the
                // fixed cell index used by height and water lookup.
                const float PX=(X+11.25f)/5.0f,PY=(Y-3.8f)/4.5f;
                const int32 GX=FMath::FloorToInt(PX),GY=FMath::FloorToInt(PY);
                const auto Patch=[&](int32 IX,int32 IY)
                {
                    return float(FrontlineScanHeightV47::Hash(IX,IY,0x59c712efu)>>8)*(1.f/16777216.f);
                };
                const float U=Ease(PX-GX),V=Ease(PY-GY);
                const float Density=FMath::Lerp(
                    FMath::Lerp(Patch(GX,GY),Patch(GX+1,GY),U),
                    FMath::Lerp(Patch(GX,GY+1),Patch(GX+1,GY+1),U),V);
                const bool Present=Random(401)<FMath::Clamp((Density-.32f)*2.55f,0.f,.90f);
                Sites.Add({(X+.5f+(Random(47)-.5f)*.90f)*ImpactPitchV47,
                    (Y+.5f+(Random(83)-.5f)*.90f)*ImpactPitchV47,
                    125.f+195.f*Random(117),100.f+165.f*Random(151),
                    Present?16.f+48.f*FMath::Pow(Random(193),1.30f):0.f,
                    Present&&Random(227)<.38f});
            }
            else Sites.Add({(X+.5f+(Random(47)-.5f)*.64f)*ImpactPitchV47,
                (Y+.5f+(Random(83)-.5f)*.64f)*ImpactPitchV47,
                250.f+90.f*Random(117),170.f+80.f*Random(151),
                (24.f+22.f*Random(193))*SoilDepth*(SettledDry?.16f:1.f),Wet});
        }
    }
    LastKey=Key;Last=&Profiles.Add(Key,MoveTemp(Sites));return *Last;
}
struct FImpactReliefV47 {float Cut=0,Rim=0,Disturbed=0,Settled=0,Calm=1,WetLip=0;};
FImpactReliefV47 ImpactReliefV47(float X,float Y)
{
    FImpactReliefV47 R;
    if(Y<-5200.f||Y>6200.f||FMath::Abs(X)>15600.f)return R;
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    const bool MeninV59=Op==7&&GMeninV59Height.Load();
    const int32 CX=FMath::FloorToInt(X/ImpactPitchV47),CY=FMath::FloorToInt(Y/ImpactPitchV47);
    const auto& Sites=ImpactSitesV47();
    for(int32 IY=FMath::Max(ImpactMinYV47,CY-1);IY<=FMath::Min(ImpactMaxYV47,CY+1);++IY)
        for(int32 IX=FMath::Max(ImpactMinXV47,CX-1);IX<=FMath::Min(ImpactMaxXV47,CX+1);++IX)
        {
            const int32 I=(IY-ImpactMinYV47)*ImpactColumnsV47+IX-ImpactMinXV47;
            const auto& C=Sites[I];
            if(C.Depth<8.f)continue; // settled Cambrai marks do not cut a bowl
            const int32 Id=UE_ARRAY_COUNT(FieldCraters)+UE_ARRAY_COUNT(DrainageHollowsV47)+I;
            const float D=CraterDistance(C,X,Y,Id);if(D>=1.40f)continue;
            const float Angle=FMath::Atan2(Y-C.Y,X-C.X),Phase=Id*.713f;
            const float Arc=MeninV59
                ?.08f+.92f*Ease((.18f+.42f*FMath::Sin(Angle*3.f+Phase)
                    +.26f*FMath::Cos(Angle*7.f-Phase)+.16f*FMath::Sin(Angle*11.f+Phase*.7f))/.62f)
                :.28f+.72f*Ease((.52f+.32f*FMath::Sin(Angle*2.f+Phase)+.21f*FMath::Cos(Angle*5.f-Phase)-.18f)/.63f);
            R.Cut=FMath::Max(R.Cut,C.Depth*(1-Ease((D-.16f)/.79f)));
            const float Rim=C.Depth*(MeninV59?.38f:.57f)*FMath::Exp(-FMath::Square((D-.99f)/.18f))*Arc*(1-Ease((D-1.20f)/.20f));
            R.Rim=FMath::Max(R.Rim,Rim);
            R.Disturbed=FMath::Max(R.Disturbed,1-Ease((D-1.02f)/.36f));
            R.Settled=FMath::Max(R.Settled,1-Ease((D-.24f)/.45f));
            if(C.Wet)
            {
                R.Calm=FMath::Min(R.Calm,Ease((D-.42f)/.16f));
                R.WetLip=FMath::Max(R.WetLip,Ease((D-.45f)/.20f)*(1-Ease((D-.92f)/.25f)));
            }
        }
    // A later impact breaches an earlier rim; intersecting scars do not stack
    // into closed doughnuts or add their depths into a mine-sized pit.
    R.Rim*=1-Ease(R.Cut/25.f);
    return R;
}
// All dimensions are centimetres. These are continuous, operation-stable
// landforms, rather than another colour/noise layer on the same battlefield.
// The protection factor is shared with the terrain surface; callers may use
// the returned masks for gameplay only where the corresponding shape exists.
FFrontlineTerrainFeaturesV54 RegionalLandformV54(float X,float Y,int32 Op,EFrontlineMissionTerrainV32 Style,float Protection)
{
    FFrontlineTerrainFeaturesV54 F;
    F.Operation=Op;F.Style=Style;
    if(Protection<=0.f||!FMath::IsFinite(X)||!FMath::IsFinite(Y))return F;
    const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
    // Stable district-scale shifts prevent repeat visits to the same terrain
    // family from sharing the same ridge/cut/building footprint.
    const float OffsetA=float((Op*7)%5-2)*132.f;
    const float OffsetB=float((Op*11)%7-3)*73.f;
    float Shape=0;
    switch(Style)
    {
    case EFrontlineMissionTerrainV32::Farmland:
    {
        // Farmland is six different battlefields, not the former flat shell
        // between craters. Each operation receives one broad, traversable
        // piece of geography that follows its campaign account: breached
        // field drains at Bullecourt and Scarpe, a sunken supply lane during
        // Michael, firm but deeply tracked Cambrai/Amiens approaches, and a
        // frozen furrow under the winter drift. The kilometre-scale envelope
        // carries them into the apron without an abrupt terrain seam.
        const float Envelope=(1-Ease((FMath::Abs(X)-Home-1800.f)/2100.f))
            *(1-Ease((FMath::Abs(Y)-1850.f)/1450.f));
        if(Envelope<=0.f)break;
        float Ditch=0,Boundary=0,Churn=0;
        if(Op==8||Op==14)
        {
            // Vehicle approaches are a broad beaten low strip with irregular
            // paired wheel/track depressions. The two campaigns use different
            // bearings, frontage and scar spacing; they are not re-colours.
            const float Centre=(Op==8?-390.f:340.f)
                +(Op==8?.13f:-.19f)*X
                +160.f*FMath::Sin(X/(Op==8?1820.f:1250.f)+Op*.47f);
            const float Across=Y-Centre;
            const float Route=FMath::Exp(-FMath::Square(Across/(Op==8?890.f:1080.f)));
            const float Pair=FMath::Exp(-FMath::Square((FMath::Abs(Across)-(Op==8?185.f:270.f))/145.f));
            const float Scour=.80f+.20f*FMath::Sin(X/990.f+Op*.58f)*FMath::Sin(Y/710.f+.8f);
            const float Beaten=Envelope*Route*Scour;
            const float PairedRuts=Envelope*Pair*Scour;
            Churn=FMath::Max(.70f*Beaten,PairedRuts);
            // A damaged field edge bounds the crossing, fading away at gaps.
            Boundary=Envelope*FMath::Exp(-FMath::Square((FMath::Abs(Across)-1210.f)/350.f))
                *(.64f+.36f*FMath::Sin(X/1160.f+Op*.31f)*FMath::Sin(X/470.f));
            Shape=-(Op==8?72.f:79.f)*Beaten-(Op==8?37.f:41.f)*PairedRuts
                +(Op==8?53.f:55.f)*Boundary;
        }
        else
        {
            const bool Transverse=Op==11;
            const float Centre=Op==2?-420.f:Op==10?85.f:Op==15?460.f:-120.f;
            const float Axis=Transverse?
                X+.14f*Home-.21f*Y-190.f*FMath::Sin(Y/1190.f+Op*.33f):
                Y-Centre-(Op==15?-.23f:.14f)*X-155.f*FMath::Sin(X/1510.f+Op*.29f);
            const float Width=Op==10?570.f:Op==11?500.f:Op==15?540.f:470.f;
            const float Side=Op==10?790.f:Op==11?750.f:Op==15?840.f:720.f;
            Ditch=Envelope*FMath::Exp(-FMath::Square(Axis/Width));
            const float Break=.72f+.28f*FMath::Sin((Transverse?Y:X)/830.f+Op*.19f)
                *FMath::Sin((Transverse?Y:X)/390.f+1.4f);
            Boundary=Envelope*Break*FMath::Exp(-FMath::Square((FMath::Abs(Axis)-Side)/260.f));
            Shape=-(Op==10?62.f:Op==11?112.f:Op==15?105.f:92.f)*Ditch
                +(Op==10?35.f:Op==11?53.f:Op==15?56.f:48.f)*Boundary;
        }
        F.FieldDitch=Protection*Ditch;
        F.FieldBoundary=Protection*Boundary;
        F.TankChurn=Protection*Churn;
        break;
    }
    case EFrontlineMissionTerrainV32::Ridge:
    {
        if(FMath::Abs(X)>Home+5600.f)break;
        if(Op==7)
        {
            const bool NewRoad=GMeninV58Height.Load();
            const auto Road=GMeninV59Height.Load()?MeninRoadV59(X,Y,Home):
                NewRoad?MeninRoadV58(X,Y,Home):MeninRoadV57(X,Y,Home);
            F.RidgeCrest=Protection*FMath::Max(Road.Crest,.56f*Road.Road);
            F.WoodlandGully=Protection*Road.Gully;
            Shape=138.f*Road.Crest+(NewRoad?72.f:42.f)*Road.Road
                -(NewRoad?118.f:102.f)*Road.Gully;
            if(GMeninV59Height.Load())
            {
                const float Across=FMath::Abs(Y-MeninRoadAxisV59(X));
                const float GullyBelt=Ease((Across-700.f)/110.f)
                    *Ease((1320.f-Across)/150.f);
                float PocketCut=0.f;
                if(GullyBelt>0.f)
                {
                    const auto& Pockets=MeninDrainShapesV59();
                    for(int32 I=0;I<Pockets.Num();++I)
                    {
                        const auto& C=Pockets[I];
                        if(FMath::Abs(X-C.X)>C.RX*1.3f||FMath::Abs(Y-C.Y)>C.RX*1.3f)continue;
                        const float D=CraterDistance(C,X,Y,MeninDrainSourceBaseV59+4*I);
                        PocketCut=FMath::Max(PocketCut,C.Depth*(1.f-Ease((D-.16f)/.74f)));
                    }
                }
                Shape-=PocketCut*GullyBelt;
            }
            break;
        }
        // A slanted chalk hogback with two breached gullies. Its back carries
        // into the distant apron, rather than ending at the gameplay strip.
        const float Axis=X-(.15f+.02f*(Op%5))*Y-.16f*Home-OffsetA
            -190.f*FMath::Sin(Y/1450.f+Op*.27f);
        // One long reverse slope and a sharper exposed face are legible from
        // the field camera; an equal-sided 1 m Gaussian disappeared beneath
        // the shared crater carpet in the first 4K comparison.
        const float Crest=FMath::Exp(-FMath::Square(Axis/(Axis<0?1900.f:1370.f)));
        const float GullyA=FMath::Exp(-FMath::Square((Y-360.f-.13f*X)/240.f))
            *FMath::Exp(-FMath::Square((X+.27f*Home)/(Home*.78f)));
        const float GullyB=FMath::Exp(-FMath::Square((Y+1450.f+.10f*X)/320.f))
            *FMath::Exp(-FMath::Square((X-.14f*Home)/(Home*.85f)));
        const auto Mine=MessinesMineV55(X,Y,Op,Home);
        F.RidgeCrest=Protection*Crest*(1-.45f*FMath::Max(GullyA,GullyB))*(1-.72f*Mine.Cut);
        // Subtract the mined bowl from the crest before adding a low,
        // discontinuous ejecta lip. No superposed cone or impassable wall.
        Shape=213.f*Crest-69.f*GullyA-55.f*GullyB-127.f*Mine.Cut
            +23.f*Mine.Rim*(1-.72f*Crest);
        break;
    }
    case EFrontlineMissionTerrainV32::Marsh:
    {
        if(FMath::Abs(X)>Home*2.6f+3000.f)break;
        if(Op==12)
        {
            const auto Wash=LysWashV57(X,Y,Home);
            if(!GLysV58Height.Load())
            {
                F.MarshChannel=Protection*Wash.Bed*(1.f-.72f*Wash.Ford);
                Shape=-158.f*Wash.Bed*(1.f-.79f*Wash.Ford)+46.f*Wash.Bank;
                break;
            }
            const float Silt=LysSiltBarsV58(Wash,Y);
            const float Runnels=LysRunnelsV58(Wash,Y);
            // The exposed wash floor remains saturated clay even where its
            // narrow runnels do not carry standing water. The two raised
            // fords are genuinely firmer; otherwise the shallow visual pass
            // would remove the useful crossing decision from squad routing.
            F.MarshChannel=Protection*Wash.Bed*(.82f+.18f*Runnels)
                *(1-.94f*Wash.Ford)*(1-.15f*Silt);
            Shape=-43.f*Wash.Bed*(1-.72f*Wash.Ford)
                -38.f*Wash.Bed*Runnels*(1-.78f*Wash.Ford)
                +25.f*Wash.Bank+8.f*Wash.Bed*(1-.85f*Wash.Ford)*Silt;
            break;
        }
        // Related drainage beds reach the photographed flooded plain; Ypres
        // carries an additional broad braid through the central crossing.
        // Their wet clay, low broken levees and real shallow water use
        // one physical height function; neither is a flat overlay card.
        const float Centre=-90.f+OffsetB+205.f*FMath::Sin(X/1590.f+Op*.29f)+.045f*X;
        const float D=Y-Centre;
        const float Across=FMath::Exp(-FMath::Square(X/(Home*1.15f+1500.f)));
        const float Channel=Across*(1-Ease((FMath::Abs(D)-530.f)/720.f));
        const float Levee=Across*FMath::Exp(-FMath::Square((FMath::Abs(D)-1410.f)/410.f));
        const float RearAxis=Y+2600.f+.10f*X+145.f*FMath::Sin(X/1750.f+.44f);
        const float RearWash=(1-Ease((FMath::Abs(RearAxis)-420.f)/650.f))
            *FMath::Exp(-FMath::Square(X/(Home*1.85f)));
        const float RearLevee=FMath::Exp(-FMath::Square((FMath::Abs(RearAxis)-1220.f)/390.f))
            *FMath::Exp(-FMath::Square(X/(Home*1.85f)));
        const auto Braid=Op==5?YpresBraidV55(X,Y,Home):FYpresBraidV55{};
        F.MarshChannel=Protection*FMath::Max3(Channel,.78f*RearWash,.88f*Braid.Bed);
        // The three Ypres beds interleave instead of summing to an implausible
        // deep excavation where drainage reaches overlap. Existing operations
        // retain their old lowland section and traversal masks.
        Shape=Op==5?-FMath::Max3(135.f*Channel,99.f*RearWash,126.f*Braid.Bed)
                +(1-.85f*Braid.Bed)*(34.f*Levee+28.f*RearLevee)+23.f*Braid.Levee:
                -135.f*Channel-99.f*RearWash+34.f*Levee+28.f*RearLevee;
        break;
    }
    case EFrontlineMissionTerrainV32::Canal:
    {
        if(Op==16)
        {
            if(Y>2300.f||Y< -7000.f||FMath::Abs(X)>Home+3200.f)break;
            const auto S=StQuentinSectionV55(X,Y,Home);
            // Two shell-breached, graded crossings line up with the staggered
            // defended works. Even there the floor stays below the banks, but
            // infantry and armour need not climb an unbroken raised wall.
            F.CanalCut=Protection*S.Bed*(1-.38f*S.Gap);
            Shape=-202.f*S.Bed*(1-.57f*S.Gap)
                +81.f*S.NearBank+68.f*S.FarBank+24.f*S.Towpath;
            break;
        }
        if(Y>3900.f||Y< -5700.f||FMath::Abs(X-.47f*Home)>6500.f)break;
        // The camera looks along -Y: a straight north-south trench disappeared
        // into the shell field. The defended cut runs diagonally to a damaged
        // rear canal reach, visibly joining the photographed water at the rear
        // edge while leaving a broad, broken, walkable towpath at its banks.
        const float Axis=.47f*Home+.55f*OffsetA+.36f*(Y+1900.f)
            +105.f*FMath::Sin(Y/910.f+Op*.21f);
        const float D=X-Axis;
        const float Length=(1-Ease((Y-2450.f)/1250.f))*(1-Ease((-Y-4700.f)/1000.f));
        const float Cut=Length*(1-Ease((FMath::Abs(D)-480.f)/650.f));
        const float Tow=Length*FMath::Exp(-FMath::Square((FMath::Abs(D)-1260.f)/330.f));
        const float RearAxis=Y+3670.f+110.f*FMath::Sin(X/1450.f+Op*.13f);
        const float RearSpan=1-Ease((FMath::Abs(X-1650.f)-2580.f)/1350.f);
        const float RearCut=RearSpan*(1-Ease((FMath::Abs(RearAxis)-300.f)/620.f));
        const float RearTow=RearSpan*FMath::Exp(-FMath::Square((FMath::Abs(RearAxis)-1120.f)/330.f));
        F.CanalCut=Protection*FMath::Max(Cut,RearCut);
        // The final canal operation shares the legacy frontage depression at
        // x=3500,y=-360. Ease its cut by three centimetres there rather than
        // relaxing the strict 220 cm route-height envelope for every mission.
        const float CutDepth=Op==19?175.f:178.f;
        Shape=-FMath::Max(CutDepth*Cut,146.f*RearCut)+FMath::Max(61.f*Tow,53.f*RearTow);
        break;
    }
    case EFrontlineMissionTerrainV32::RailCutting:
    {
        if((Op==17?(Y>1700.f||Y< -3200.f):FMath::Abs(Y+1130.f)>1800.f)
            ||FMath::Abs(X)>Home+4900.f)break;
        // The actual sleepers remain graded at Y=-1130. Cut shoulders are
        // outside that support and read as a defended railway section.
        const float Axis=-1130.f+47.f*FMath::Sin(X/1370.f+Op*.17f);
        const float D=FMath::Abs(Y-Axis);
        const float Along=1-Ease((FMath::Abs(X)-Home-2400.f)/2400.f);
        const float Shoulder=Along*FMath::Exp(-FMath::Square((D-430.f-OffsetB*.22f)/220.f));
        const float Outside=Along*FMath::Exp(-FMath::Square((D-790.f)/330.f));
        F.RailShoulder=Protection*Shoulder;
        Shape=84.f*Shoulder-32.f*Outside;
        if(Op==17)
        {
            // The Selle rail cutting crosses a shallow muddy runoff wash, not
            // the same dry, level strip used beneath every other railway.
            // One firm service causeway breaks the wash at +410 Y; the far
            // running rail is separately protected by RegionalRail. Thus a
            // squad can wade through or curve toward a visible dry crossing.
            const float WashAxis=-.18f*Home+.22f*Y
                +145.f*FMath::Sin(Y/770.f+1.7f);
            const float Across=X-WashAxis;
            const float Reach=(1-Ease((Y-1130.f)/440.f))
                *(1-Ease((-Y-1600.f)/410.f));
            const float Causeway=FMath::Exp(-FMath::Square((Y-410.f)/210.f));
            const float Bed=Reach*FMath::Exp(-FMath::Square(Across/620.f));
            const float BrokenLip=Reach*FMath::Exp(-FMath::Square((FMath::Abs(Across)-700.f)/230.f))
                *(.72f+.28f*FMath::Sin(Y/330.f+Across/490.f));
            const float Wash=Bed*(1-.94f*Causeway);
            F.RailWash=Protection*Wash;
            Shape+=-86.f*Wash+34.f*BrokenLip*(1-.72f*Causeway)
                +19.f*Bed*Causeway;
        }
        break;
    }
    case EFrontlineMissionTerrainV32::Village:
    {
        if(Op==13&&Y< -3900.f&&Y> -6900.f)
        {
            // The Villers painting begins behind a physically modelled ruined
            // outskirts, rather than at the former ruler-straight mesh edge.
            // Broad, broken foundations die away before the far matte; none
            // of this rear detail changes the soldiers' playable frontage.
            const float Reach=Ease((-Y-3900.f)/490.f)*(1-Ease((-Y-6250.f)/600.f));
            const float LobeA=FMath::Exp(-FMath::Square((X+4550.f)/1350.f)-FMath::Square((Y+4750.f)/680.f));
            const float LobeB=FMath::Exp(-FMath::Square((X+300.f)/1850.f)-FMath::Square((Y+5260.f)/910.f));
            const float LobeC=FMath::Exp(-FMath::Square((X-4870.f)/1430.f)-FMath::Square((Y+4590.f)/740.f));
            const float Broken=FMath::Clamp(.64f+.22f*FMath::PerlinNoise2D(FVector2D(X/410.f+13.f,Y/380.f)),.28f,.94f);
            const float Rubble=Reach*FMath::Max3(LobeA,LobeB,LobeC)*Broken;
            F.Ruins=Protection*Rubble;
            Shape=81.f*Rubble;
            break;
        }
        if(FMath::Abs(X)>Home+2600.f||Y>2100.f||Y< -3900.f)break;
        // Ruined rectangular cellars and discontinuous masonry berms. The
        // signed box distance makes them read as buildings, not shell craters.
        float Berm=0,Cellar=0;
        auto Building=[&](float CX,float CY,float HalfX,float HalfY,float Angle)
        {
            const float DX=X-CX,DY=Y-CY;
            if(FMath::Abs(DX)>HalfX*1.7f||FMath::Abs(DY)>HalfY*1.7f)return;
            const float U=(DX*FMath::Cos(Angle)+DY*FMath::Sin(Angle))/HalfX;
            const float V=(-DX*FMath::Sin(Angle)+DY*FMath::Cos(Angle))/HalfY;
            const float Edge=FMath::Max(FMath::Abs(U),FMath::Abs(V));
            const float Break=.72f+.28f*Ease((FMath::PerlinNoise2D(FVector2D(X/170.f+CX*.01f,Y/140.f+CY*.01f))+.2f)/.55f);
            Berm=FMath::Max(Berm,Break*FMath::Exp(-FMath::Square((Edge-.91f)/.20f)));
            Cellar=FMath::Max(Cellar,1-Ease((Edge-.41f)/.43f));
        };
        if(Op==13)
        {
            // Villers-Bretonneux: the Amiens road and a damaged village lane
            // form a broad, firm crossroads. Rubble and sunken cellars sit on
            // the flanks, so armour has a usable exposed road while infantry
            // can make a slower protected advance through the ruins.
            const float Envelope=(1-Ease((FMath::Abs(X)-Home-1250.f)/1700.f))
                *(1-Ease((FMath::Abs(Y)-1800.f)/1300.f));
            const auto RoadY=[](float AtX){return 90.f+.085f*AtX+125.f*FMath::Sin(AtX/1590.f+.4f);};
            const float Across=Y-RoadY(X);
            const float Main=FMath::Exp(-FMath::Square(Across/570.f));
            const float LaneAxis=X-.16f*Home-.21f*Y-90.f*FMath::Sin(Y/1180.f+.8f);
            const float Lane=FMath::Exp(-FMath::Square(LaneAxis/430.f))
                *(1-Ease((FMath::Abs(Y)-1370.f)/770.f));
            const float Road=Envelope*FMath::Max(Main,.82f*Lane);
            const float Broken=.66f+.34f*FMath::Sin(X/790.f+.7f)*FMath::Sin(X/330.f+Y/410.f);
            const float Shoulder=Envelope*Broken
                *FMath::Exp(-FMath::Square((FMath::Abs(Across)-930.f)/305.f))*(1-.82f*Lane);
            Building(-.50f*Home,RoadY(-.50f*Home)+1280.f,810.f,460.f,.11f);
            Building(-.03f*Home,RoadY(-.03f*Home)-1330.f,700.f,430.f,-.17f);
            Building(.43f*Home,RoadY(.43f*Home)+1240.f,590.f,510.f,.23f);
            // A road breach through an old foundation must remain traversable;
            // the same exclusion applies to physical relief and gameplay masks.
            const float OffRoad=1-.91f*Road;
            Berm*=Envelope*OffRoad;Cellar*=Envelope*OffRoad;
            F.VillageRoad=Protection*Road;
            F.Ruins=Protection*FMath::Max(Berm,.84f*Shoulder);
            F.RuinCellar=Protection*Cellar;
            Shape=-48.f*Road+31.f*Shoulder+69.f*Berm-44.f*Cellar;
            break;
        }
        Building(-.48f*Home+OffsetA,-270.f+OffsetB,590.f,360.f,.19f+Op*.013f);
        Building(.46f*Home-OffsetB,390.f+OffsetA*.45f,460.f,420.f,-.24f);
        Building(-.18f*Home+OffsetA*.5f,-2290.f+OffsetB,690.f,360.f,.10f);
        F.Ruins=Protection*Berm;
        F.RuinCellar=Protection*Cellar;
        Shape=70.f*Berm-38.f*Cellar;
        break;
    }
    default:break;
    }
    if(Op==10)
    {
        // Wind-laid diagonal drift tongues sit above that operation's geology.
        // Their irregular spacing and taper avoid a repeated wave pattern.
        float Drift=0;
        auto Tongue=[&](float CX,float CY,float RX,float RY)
        {
            const float U=(X-CX+.31f*(Y-CY))/RX;
            const float V=(Y-CY-.31f*(X-CX))/RY;
            Drift=FMath::Max(Drift,FMath::Exp(-FMath::Square(U)-FMath::Square(V)));
        };
        if(FMath::Abs(X)<Home+3200.f&&Y>-2600.f&&Y<1600.f)
        {
            Tongue(-.43f*Home,-400.f,740.f,260.f);
            Tongue(.12f*Home,370.f,1010.f,320.f);
            Tongue(.57f*Home,-1150.f,820.f,230.f);
        }
        F.SnowDrift=Protection*Drift;
        Shape+=43.f*Drift;
    }
    F.ReliefCm=Protection*Shape;
    return F;
}
float StructuralCoreV54(const FFrontlineTerrainFeaturesV54& F)
{
    // Only the authored large forms earn a quieter crater microstructure.
    // This creates a readable continuous silhouette, not an empty clean road.
    const float Primary=FMath::Max(FMath::Max3(F.RidgeCrest,F.WoodlandGully,F.MarshChannel),
        FMath::Max3(F.CanalCut,F.VillageRoad,FMath::Max3(F.TankChurn,F.FieldDitch,F.RailWash)));
    const float Secondary=FMath::Max(FMath::Max(F.RailShoulder,F.Ruins),F.SnowDrift);
    return Ease((FMath::Max(Primary,Secondary*.74f)-.09f)/.66f);
}
float UnpooledHeightV42(float X,float Y,bool V47=true,bool Scanned=true);
float LevelPoolV42(float X,float Y,float Height,float* OutWater=nullptr);
constexpr int32 LevelPoolIndicesV42[]={4,9,10,12,14,20,23};
bool LevelPoolCandidateV42(float X,float Y)
{
    static const bool Enabled=!FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"));
    if(!Enabled || FMath::Abs(Y)<=720.f)return false;
    for(int32 Index:LevelPoolIndicesV42)
    {
        const FFieldCrater& C=FieldCraters[Index];
        const float Reach=FMath::Max(C.RX,C.RY)*1.10f;
        if(FMath::Abs(X-C.X)<Reach && FMath::Abs(Y-C.Y)<Reach)return true;
    }
    return false;
}
float CraterWetness(float X,float Y,float* OutLevelWater=nullptr)
{
    float Wetness=0;
    const float Foreground=Ease((Y-520.f)/800.f);
    int32 Index=0;
    for (const FFieldCrater& C:FieldCraters)
    {
        const float Reach=3.f*FMath::Max(C.RX,C.RY);
        if(C.Wet && FMath::Abs(X-C.X)<Reach && FMath::Abs(Y-C.Y)<Reach)
        {
            // The near apron reads as broken damp basins, not parallel silver
            // strokes. Keep the firing corridor's original water mask exact.
            const float ShoreAngle=FMath::Atan2(Y-C.Y,X-C.X);
            const float BrokenShore=.5f+.5f*FMath::Sin(ShoreAngle*3.f+Index*.83f);
            const float ShoreBreak=Ease((FMath::Abs(Y)-720.f)/350.f)*.035f
                *FMath::PerlinNoise2D(FVector2D(X/85.f+7.3f,Y/115.f));
            const float D=CraterDistance(C,X,Y,Index)+Foreground*.11f*BrokenShore+ShoreBreak;
            const float Pool=(1-Ease((D-.27f)/.23f))*(1-Foreground*.34f);
            Wetness=FMath::Max(Wetness,Pool);
        }
        ++Index;
    }
    // The same bounded height operation supplies the water coverage. Never
    // paint a reflective patch onto an unflattened slope or a protected bank.
    if(OutLevelWater)*OutLevelWater=0;
    if(LevelPoolCandidateV42(X,Y))
    {
        float LevelWater=0;
        LevelPoolV42(X,Y,UnpooledHeightV42(X,Y),&LevelWater);
        if(OutLevelWater)*OutLevelWater=LevelWater;
        Wetness=FMath::Max(Wetness,LevelWater);
    }
    return Wetness;
}
}
AWarfareFrontlineGround::AWarfareFrontlineGround()
{
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Terrain"));
    SetRootComponent(Surface);
    Surface->SetCollisionProfileName(TEXT("BlockAll"));
    // Trench movement uses the authored corridor and HeightAt, not navmesh.
    Surface->SetCanEverAffectNavigation(false);
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))Surface->SetMobility(EComponentMobility::Movable);
    Sandbags=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Sandbags"));
    Sandbags->SetupAttachment(Surface);
    Sandbags->SetCollisionProfileName(TEXT("NoCollision"));
    Sandbags->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sandbags->SetCanEverAffectNavigation(false);
}

namespace
{
float UnpooledHeightV42(float X, float Y,bool V47,bool Scanned)
{
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    const bool IsRail=Style==EFrontlineMissionTerrainV32::RailCutting;
    // Mud remains the continuous substrate under local chalk/brick deposits.
    // Winter's compact snow has its own fine normal; keep its mesoform only.
    const bool MudScan=Op!=10;
    static const bool RefinedRelief=!FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"));
    const bool MaterialFirstV59=Op==7&&RefinedRelief&&V47&&GMeninV59Height.Load();
    const auto RawRegion=RefinedRelief&&V47?RegionalLandformV54(X,Y,Op,Style,1.f):FFrontlineTerrainFeaturesV54{};
    FMeninRoadSection MeninSection;
    if(Op==7&&RefinedRelief&&V47&&GMeninV58Height.Load())
    {
        const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
        MeninSection=MaterialFirstV59?MeninRoadV59(X,Y,Home):MeninRoadV58(X,Y,Home);
    }
    const float StructuralQuiet=RefinedRelief&&V47?StructuralCoreV54(RawRegion):0.f;
    // Broad undulations read as land before small surface texture. Fine relief is
    // bounded so feet can follow the same function without staircase motion.
    float H = 25 + 26 * FMath::Sin(X / 1550.f) * FMath::Cos(Y / 1250.f)
        + 6 * FMath::Sin(X/180.f+Y/280.f)*FMath::Cos(Y/215.f);
    const float OutsideFooting=Ease((FMath::Abs(Y)-720.f)/350.f);
    float ReliefClearance=1.f,RegionalWaterClearance=1.f;
    float WetLipV47=0,CalmCoreV47=1;
    float NearestExcavation=FLT_MAX;
    float BrokenBank=0.f;
    int32 CraterIndex=0;
    for (const FFieldCrater& C : FieldCraters)
    {
        // Compact evaluation keeps the longer field affordable for per-soldier
        // foot queries; beyond this bound the rim tail is below float precision.
        const float Reach=3.f*FMath::Max(C.RX,C.RY);
        if (FMath::Abs(X-C.X)>Reach || FMath::Abs(Y-C.Y)>Reach) { ++CraterIndex; continue; }
        const float D=CraterDistance(C,X,Y,CraterIndex);
        if(V47&&Scanned&&C.Wet)
        {
            WetLipV47=FMath::Max(WetLipV47,Ease((D-.45f)/.20f)*(1-Ease((D-.92f)/.25f)));
            CalmCoreV47=FMath::Min(CalmCoreV47,Ease((D-.42f)/.16f));
        }
        if(C.Wet)
        {
            ReliefClearance=FMath::Min(ReliefClearance,Ease((D-.82f)/.48f));
            // A landform needs a much broader shore transition than clods.
            RegionalWaterClearance=FMath::Min(RegionalWaterClearance,Ease((D-.24f)/1.18f));
        }
        H-=C.Depth*(1-Ease((D-.18f)/.94f));
        // Low, broken ejecta rims: subsidiary hollows do not all become identical cones.
        const float Angle=FMath::Atan2(Y-C.Y,X-C.X);
        H+=C.Depth*.16f*FMath::Exp(-FMath::Square((D-1.12f)/.26f))
            *(.62f+.38f*FMath::Sin(Angle*2+CraterIndex));
        if(OutsideFooting>0)BrokenBank=FMath::Max(BrokenBank,(1-Ease(FMath::Abs(D-1.01f)/.31f))*.78f);
        ++CraterIndex;
    }
    const float WaterClearance=ReliefClearance;
    const float OriginalLengthMask=1-Ease((FMath::Abs(Y)-(Y<0?1850.f:3100.f))/750.f);
    for (int32 TrenchIndex=0;TrenchIndex<FrontlineBattlefield::TrenchCount;++TrenchIndex)
    {
        const float TrenchX=FrontlineFieldV32::TrenchCentreX(TrenchIndex,Y);
        // The playable firing step stays level; earthworks meander beyond it.
        const float Bend=0;
        const float D=FMath::Abs(X-TrenchX-Bend);
        NearestExcavation=FMath::Min(NearestExcavation,D);
        ReliefClearance=FMath::Min(ReliefClearance,Ease((D-560.f)/210.f));
        if (D>650.f) continue;
        // Different surviving communication-trench ends: long western rear
        // cut, a collapsed central approach, and a shorter eastern rear spur.
        // Only fill the old excavation beyond existing duckboards/revetment;
        // do not shift saved parapets or deepen ground beneath their feet.
        constexpr float RearEnds[]={1850.f,1720.f,1630.f,1510.f,1850.f};
        constexpr float NearEnds[]={3100.f,2800.f,2380.f,2550.f,2970.f};
        constexpr float NearPinch[]={0.f,17.f,34.f,17.f,6.f};
        const float LengthMask=1-Ease((FMath::Abs(Y)-(Y<0?RearEnds[FrontlineBattlefield::AuthoringFromActive(TrenchIndex)]:NearEnds[FrontlineBattlefield::AuthoringFromActive(TrenchIndex)]))/750.f);
        const float FarApron=Ease((Y-1700.f)/750.f);
        const float FloorHalfWidth=100.f-NearPinch[FrontlineBattlefield::AuthoringFromActive(TrenchIndex)]*FarApron;
        const bool SecondLine=FrontlineBattlefield::TrenchCount>3&&(TrenchIndex==1 || TrenchIndex==FrontlineBattlefield::TrenchCount-2);
        const float CutDepth=(FrontlineBattlefield::TrenchCount%2==1&&TrenchIndex==FrontlineBattlefield::CentreTrench)?90.f:(SecondLine?115.f:103.f);
        const float RimHeight=(FrontlineBattlefield::TrenchCount%2==1&&TrenchIndex==FrontlineBattlefield::CentreTrench)?8.f:(SecondLine?22.f:14.f);
        H-=CutDepth*(1-Ease((D-FloorHalfWidth)/110.f))*LengthMask;
        H+=RimHeight*FMath::Exp(-FMath::Square((D-255.f)/88.f))*OriginalLengthMask;
        // Break the outer excavated shoulder, beyond the saved sandbag/bank
        // supports. These deeper fractures stay outside the crossing strip.
        if(OutsideFooting>0)BrokenBank=FMath::Max(BrokenBank,(1-Ease(FMath::Abs(D-415.f)/115.f))*LengthMask);
    }
    if(BrokenBank>0 && OutsideFooting>0)
    {
        const float Fracture=11.f*FMath::PerlinNoise2D(FVector2D(X/91.f+17.3f,Y/119.f))
            +4.f*FMath::PerlinNoise2D(FVector2D(X/43.f,Y/61.f+9.7f));
        H+=FMath::Clamp(Fracture,-9.f,9.f)*BrokenBank*OutsideFooting;
    }
    // Soft distant ridges prevent the playable strip from reading as an empty tabletop.
    const float Background=Ease((-Y-1900.f)/3100.f);
    // The Menin matte already carries the distant road and wood line. Let its
    // real skyline show through instead of raising a second full-width bank.
    H+=Background*(Op==7&&RefinedRelief&&V47?.48f:1.f)
        *(80+42*FMath::Sin(X/1900.f)+28*FMath::Cos(X/780.f));
    // The physical rear edge meets the matte along a broken low earth ridge,
    // not a horizontal plane cut. This uses the same surface/light response.
    const float RearRidge=Ease((-Y-4020.f)/940.f);
    const float RidgeBreak=FMath::PerlinNoise2D(FVector2D(X/440.f+Op*2.17f,Y/1150.f))
        +.45f*FMath::PerlinNoise2D(FVector2D(X/190.f+13.8f,Y/630.f));
    // This distant V42 earth wall was identical in every 4K screenshot. A
    // floodplain must meet the marsh panorama low, and the cut/firm fields
    // need open sight lines; the chalk operation alone keeps a raised skyline.
    const float MeninOpenings=FMath::Max(
        FMath::Exp(-FMath::Square((X+2650.f)/970.f)),
        FMath::Exp(-FMath::Square((X-2450.f)/1180.f)));
    const float DistrictRear=Op==7?.25f*(1.f-.78f*MeninOpenings):
        Style==EFrontlineMissionTerrainV32::Marsh?.16f:
        Style==EFrontlineMissionTerrainV32::Canal?.27f:
        Style==EFrontlineMissionTerrainV32::Farmland?(Op==8?.34f:.55f):
        Style==EFrontlineMissionTerrainV32::RailCutting?.56f:
        Style==EFrontlineMissionTerrainV32::Village?.65f:1.f;
    H+=RearRidge*FMath::Clamp(88.f+112.f*RidgeBreak,18.f,185.f)
        *(RefinedRelief&&V47?DistrictRear:1.f);
    // Two unequal spoil ridges break the empty middle distance. Their envelope
    // leaves the occupied firing steps and the playable crossing unobstructed.
    const float DistantOnly=Ease((-Y-1000.f)/750.f);
    const float DistantSpoil=DistantOnly*(300*FMath::Exp(-FMath::Square((X+2100)/1080.f)-FMath::Square((Y+2620)/405.f))
        +200*FMath::Exp(-FMath::Square((X-1260)/900.f)-FMath::Square((Y+3150)/510.f))
        -48*FMath::Exp(-FMath::Square((X+650)/740.f)-FMath::Square((Y+2060)/450.f))
        +220*FMath::Exp(-FMath::Square((X+8350)/1430.f)-FMath::Square((Y+2810)/510.f))
        +265*FMath::Exp(-FMath::Square((X-8460)/1280.f)-FMath::Square((Y+2730)/470.f)));
    // Shell-torn clay needs readable middle-scale relief at the wide camera,
    // not just a bumpy normal map. Preserve the complete standing strip,
    // physical parapet supports and calm pool beds using the same clearance.
    const float OriginalBankClearance=Ease((NearestExcavation-305.f)/145.f);
    const float BankClearance=RefinedRelief&&V47?FMath::Max(OriginalBankClearance,
        Ease((FMath::Abs(Y)-BankSupportHalfExtentV47)/BankReleaseWidthV47)):OriginalBankClearance;
    const float RailClearance=IsRail?Ease((FMath::Abs(Y+1130.f)-200.f)/120.f):1.f;
    // A soldier corridor is not a flat parade ground. Keep only the actual
    // revetment/duckboard support clear; shallow impacts cross the whole field.
    const float Apron=RefinedRelief&&V47?BankClearance*RailClearance:0.f;
    const FImpactReliefV47 Impacts=Apron>0?ImpactReliefV47(X,Y):FImpactReliefV47{};
    if(Apron>0)
    {
        // The previous uniform shell field hid every district-scale form at
        // the normal side camera. Keep all impact sites, but make their local
        // height subordinate to the occupied ridge, channel, cut or route.
        // Wet cores stay untouched so basin depths and shore contacts survive.
        const float Readable=StructuralQuiet*RegionalWaterClearance*Impacts.Calm;
        const float DistrictCalm=Style==EFrontlineMissionTerrainV32::Marsh?.96f:
            Style==EFrontlineMissionTerrainV32::Canal?(Op==16?.985f:.95f):
            Style==EFrontlineMissionTerrainV32::Farmland?.83f:.63f;
        // Years of traffic leave a flatter roadbed without erasing every
        // shell strike. Its damaged shoulders and the gullies keep the full
        // impact profile; the old identical clods concealed the route.
        H+=Apron*(Impacts.Rim-Impacts.Cut)*(1.f-DistrictCalm*Readable)
            *(1.f-.45f*MeninSection.Road);
        CalmCoreV47=FMath::Min(CalmCoreV47,Impacts.Calm);
        WetLipV47=FMath::Max(WetLipV47,Impacts.WetLip);
    }
    // Keep the bank-support heights exact while lowering the intervening
    // artificial hills. The concept's skyline is a low battered field.
    const float BehindAuthoredBanks=Ease((-Y-2250.f)/450.f);
    const float DistantApron=RefinedRelief&&V47?OutsideFooting*RailClearance*FMath::Max(BankClearance,BehindAuthoredBanks):0.f;
    // The two old fixed spoil mounds were taller than most regional landforms
    // and repeated their silhouette across every operation. They become low
    // residual debris; the mission-specific landform now carries the horizon.
    const float RegionalSpoil=Op==7?.10f:
        Style==EFrontlineMissionTerrainV32::Marsh?.12f:
        Style==EFrontlineMissionTerrainV32::Canal?.20f:
        Style==EFrontlineMissionTerrainV32::Ridge?.27f:
        Style==EFrontlineMissionTerrainV32::Farmland?.22f:
        Style==EFrontlineMissionTerrainV32::RailCutting?.31f:.35f;
    // The revised distant spoil must fade to the retained V46 profile across
    // the railway's graded support, just as the additive apron term does.
    // Otherwise the negative old spoil hollow leaks ~0.8 cm into the rail
    // shoulder at Y=-1330 despite an exactly protected trench footing.
    const float SpoilRelease=FMath::Max(BankClearance*RailClearance,BehindAuthoredBanks);
    H+=DistantSpoil*FMath::Lerp(1.f,.38f,DistantApron)
        *(RefinedRelief&&V47?FMath::Lerp(1.f,RegionalSpoil,SpoilRelease):1.f);
    float Drainage=0,DrainageRim=0,RegionalDrainageClearance=1.f;
    if(Apron>0)
    {
        for(int32 I=0;I<UE_ARRAY_COUNT(DrainageHollowsV47);++I)
        {
            const auto& C=DrainageHollowsV47[I];
            const float Reach=FMath::Max(C.RX,C.RY)*1.4f;
            if(FMath::Abs(X-C.X)>Reach||FMath::Abs(Y-C.Y)>Reach)continue;
            const float D=CraterDistance(C,X,Y,UE_ARRAY_COUNT(FieldCraters)+I);
            RegionalDrainageClearance=FMath::Min(RegionalDrainageClearance,Ease((D-.24f)/1.18f));
            if(Scanned)
            {
                WetLipV47=FMath::Max(WetLipV47,Ease((D-.45f)/.20f)*(1-Ease((D-.92f)/.25f)));
                CalmCoreV47=FMath::Min(CalmCoreV47,Ease((D-.42f)/.16f));
            }
            const float Hollow=1-Ease((D-.20f)/.89f);
            // Use the deepest overlapping depression, not the sum. A linked
            // pool must not create a new deep hole where its two lobes meet.
            Drainage=FMath::Max(Drainage,C.Depth*Hollow);
            const float Broken=.35f+.65f*Ease((FMath::PerlinNoise2D(FVector2D(X/175.f+I,Y/123.f))+.22f)/.65f);
            DrainageRim=FMath::Max(DrainageRim,6.f*FMath::Exp(-FMath::Square((D-1.04f)/.18f))*Broken);
        }
        H+=Apron*(DrainageRim-Drainage);
    }
    if(RefinedRelief&&OutsideFooting>0&&ReliefClearance>0)
    {
        // V59 keeps the broken rear earth silhouette but moves its small
        // chips and fine clay grain to the material's existing normal/AO.
        const float Stratum=FMath::PerlinNoise2D(FVector2D(X/(MaterialFirstV59?740.f:370.f)+43.7f,
            Y/(MaterialFirstV59?510.f:185.f)));
        const float Fragments=MaterialFirstV59?0.f:FMath::PerlinNoise2D(FVector2D(X/117.f,Y/93.f+8.9f));
        const float Ejecta=Ease((FMath::PerlinNoise2D(FVector2D(X/880.f+13.7f,Y/630.f))+.02f)/.40f);
        const float BrokenCrest=FMath::Max(0.f,Stratum-.18f);
        const float Clay=FMath::Clamp((MaterialFirstV59?22.f:36.f)*BrokenCrest+8.f*Fragments,-7.f,17.f);
        const float PriorClay=FMath::Clamp(46.f*FMath::Max(0.f,FMath::Abs(Stratum)*2.f-.16f)+19.f*Fragments,-12.f,42.f);
        H+=OutsideFooting*ReliefClearance*FMath::Lerp(PriorClay,Clay*(.15f+.85f*Ejecta)*(1-Ease(Drainage/19.f)),V47?RailClearance:0.f);
    }
    // V45: the old complete flat crossing strip removed the geometry that
    // catches contact shadows at the game camera. Short, broken sod relief is
    // sampled by BOTH the visible mesh and footing; it never enters the trench
    // floor, revetments, graded railway or water beds. Broad ridges above are
    // restrained so these clods do not become another row of smooth hills.
    const float ClodMask=BankClearance*RailClearance*WaterClearance;
    if(ClodMask>0&&!MaterialFirstV59)
    {
        const float Lumps=FMath::PerlinNoise2D(FVector2D(X/82.f+21.7f,Y/97.f+13.1f));
        const float Chips=FMath::PerlinNoise2D(FVector2D(X/39.f+3.4f,Y/53.f));
        const float Rut=FMath::PerlinNoise2D(FVector2D(X/340.f+71.f,Y/48.f+9.f));
        const float Relief=22.f*FMath::Pow(FMath::Clamp(FMath::Abs(Lumps)*2.1f,0.f,1.f),1.45f)
            +7.f*Chips-5.f*FMath::Max(0.f,Rut)-4.f;
        const float Ejecta=Ease((FMath::PerlinNoise2D(FVector2D(X/810.f+19.4f,Y/570.f+6.f))-.015f)/.39f);
        const float Clods=26.f*FMath::Pow(FMath::Max(0.f,Lumps-.08f),1.30f)+3.f*Chips;
        const float Loose=FMath::Lerp(1.5f*Chips-2.f*FMath::Max(0.f,Rut),Clods,Ejecta);
        const float Quiet=(1-Ease(Drainage/18.f))*CalmCoreV47;
        float Varied=FMath::Clamp(Loose,-3.f,13.f)*Quiet;
        if(RefinedRelief&&V47&&Scanned&&MudScan&&Quiet>0)
        {
            // The photographed height shares the material's integer lattice,
            // mirror/offset and height-aware blend. Loose apron patches carry
            // up to +/-12 cm of photographed aggregate relief. Fixed bank/rail
            // supports remain exact and calm basin beds receive none.
            // Loose photographed aggregates cover the ground BETWEEN impact
            // scars as well as their broken lips. Compact centres stay smooth;
            // the old positive-Perlin stripe gate made isolated worm heaps.
            const float Exposure=(.64f+.36f*Impacts.Disturbed)*(1-.86f*Impacts.Settled)*Quiet;
            const float ContactScale=.55f+.45f*OutsideFooting;
            Varied=FMath::Lerp(Varied,Varied*.25f+ContactScale*AggregateScanStrengthV47*FrontlineScanHeightV47::Displacement(X,Y),Exposure);
        }
        // Retain the original small frontage relief and add its separately
        // bounded scanned layer below. The apron carries the torn aggregates.
        const float Readable=StructuralQuiet*RegionalWaterClearance*RegionalDrainageClearance*CalmCoreV47;
        const float DistrictCalm=Style==EFrontlineMissionTerrainV32::Marsh?.95f:
            Style==EFrontlineMissionTerrainV32::Canal?(Op==16?.975f:.94f):
            Style==EFrontlineMissionTerrainV32::Farmland?.79f:.57f;
        // The scanned 2.8 m aggregate was overpowering the road and making
        // the whole Menin field repeat. Retain shell and bank silhouettes;
        // attenuate this one fine relief band by 30% across operation 7 and
        // further compact only its raised road core.
        const float MeninFineScale=Op==7&&RefinedRelief&&V47&&GMeninV58Height.Load()?.70f:1.f;
        H+=ClodMask*(1.f-DistrictCalm*Readable)*MeninFineScale
            *(1.f-.13f*MeninSection.Crest-.75f*MeninSection.Road)
            *FMath::Lerp(FMath::Clamp(Relief,-6.f,24.f)*FMath::Lerp(.42f,1.f,OutsideFooting),Varied,RefinedRelief&&V47?1.f:0.f);
    }
    if(Apron>0&&Scanned&&MudScan&&WetLipV47>0&&CalmCoreV47>0)
    {
        // Small real mud tongues perturb the bed/water intersection, not the
        // level water surface. Any overlapping authored calm core masks this
        // band too; no cached water query can recurse back into HeightAt.
        const float Lip=FMath::Clamp(FrontlineScanHeightV47::Displacement(X,Y)*
            (6.f/FrontlineScanHeightV47::ReliefRangeCm),-3.f,3.f);
        H+=Apron*WetLipV47*CalmCoreV47*Lip;
    }
    if(RefinedRelief&&V47)
    {
        // The photographed-mud and small-puddle masks change over tens of cm.
        // Multiplying a metre-high ridge by them creates a false near-vertical
        // edge. Instead fade regional geology over several metres; local impact
        // basins are re-solved/baked atop it using the same HeightAt surface.
        const float RegionalBank=FMath::Max(Ease((NearestExcavation-305.f)/510.f),
            Ease((FMath::Abs(Y)-BankSupportHalfExtentV47)/700.f));
        const float RegionalRail=IsRail?Ease((FMath::Abs(Y+1130.f)-220.f)/350.f):1.f;
        const float RegionalProtection=MaterialFirstV59?MeninRegionalProtectionV59(X,Y):
            RegionalBank*RegionalRail*RegionalWaterClearance*RegionalDrainageClearance;
        H+=RawRegion.ReliefCm*RegionalProtection;
    }
    return H+FrontlineFieldV32::TerrainLift(X,Y,H);
}

float LevelPoolV42(float X,float Y,float Height,float* OutWater)
{
    if(OutWater)*OutWater=0;
    static const bool Enabled=!FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"));
    if(!Enabled || FMath::Abs(Y)<=720.f)return Height;

    // Seven already-authored wet craters, five in the central wide view.
    // Larger depth produces a larger pool; no tiled water plane or new crater.
    for(int32 Index:LevelPoolIndicesV42)
    {
        const FFieldCrater& C=FieldCraters[Index];
        const float Reach=FMath::Max(C.RX,C.RY)*1.10f;
        if(FMath::Abs(X-C.X)>Reach || FMath::Abs(Y-C.Y)>Reach)continue;
        const float D=CraterDistance(C,X,Y,Index)
            +.055f*FMath::PerlinNoise2D(FVector2D(X/91.f+Index,Y/127.f+3.7f));
        if(D>=.88f)continue;
        float Clearance=Ease((FMath::Abs(Y)-720.f)/160.f);
        for(int32 Trench=0;Trench<FrontlineBattlefield::TrenchCount;++Trench)
            Clearance=FMath::Min(Clearance,Ease((FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(Trench,Y))-560.f)/210.f));
        // The railway shares a graded support bed in RailCutting operations.
        // Preserve that same corridor in every operation; querying current
        // mission state or keeping stale cached water levels is unnecessary.
        Clearance=FMath::Min(Clearance,Ease((FMath::Abs(Y+1130.f)-220.f)/100.f));
        if(Clearance<=0)continue;
        const float Influence=Clearance*(1-Ease((D-.72f)/.16f));
        // Re-evaluate the unpooled centre from the current operation. This is
        // non-recursive and includes TerrainLift before making the water level.
        // Only points inside these small footprints pay for a second query.
        const float Level=UnpooledHeightV42(C.X,C.Y)+C.Depth*.40f;
        if(Height>=Level)continue;
        const float Depth=Level-Height;
        const float Coverage=Ease((Influence-.97f)/.03f)*Ease(Depth/2.f);
        if(OutWater)*OutWater=FMath::Max(*OutWater,Coverage);
        // Raise the submerged floor to the level surface. Soft perimeter and
        // clearance fades remain damp earth, never a floating hard-edged card.
        Height=FMath::Lerp(Height,Level,Influence);
    }
    return Height;
}
}

float AWarfareFrontlineGround::HeightAt(float X,float Y)
{
    float Physical=0;if(WarfareFRRUWorldV82::TryGlobalHeight(X,Y,Physical))return Physical;
    const float Height=UnpooledHeightV42(X,Y);
    // V47 standing water has its own level mesh. The older vertex mirror
    // raised crater floors to flat shelves; those shelves are no longer water.
    static const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"));
    return Baseline?LevelPoolV42(X,Y,Height):Height;
}

FFrontlineTerrainFeaturesV54 AWarfareFrontlineGround::QueryTerrainFeaturesV54(float X,float Y)
{
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    if(!FMath::IsFinite(X)||!FMath::IsFinite(Y)||FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))
        return RegionalLandformV54(X,Y,Op,Style,0.f);
    float Nearest=FLT_MAX,RegionalWater=1.f,RegionalDrainage=1.f;
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
        Nearest=FMath::Min(Nearest,FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y)));
    for(int32 I=0;I<UE_ARRAY_COUNT(FieldCraters);++I)if(FieldCraters[I].Wet)
    {
        const auto& C=FieldCraters[I];
        if(FMath::Abs(X-C.X)>C.RX*1.5f||FMath::Abs(Y-C.Y)>C.RY*1.5f)continue;
        RegionalWater=FMath::Min(RegionalWater,Ease((CraterDistance(C,X,Y,I)-.24f)/1.18f));
    }
    for(int32 I=0;I<UE_ARRAY_COUNT(DrainageHollowsV47);++I)
    {
        const auto& C=DrainageHollowsV47[I];
        if(FMath::Abs(X-C.X)>C.RX*1.4f||FMath::Abs(Y-C.Y)>C.RY*1.4f)continue;
        const float D=CraterDistance(C,X,Y,UE_ARRAY_COUNT(FieldCraters)+I);
        RegionalDrainage=FMath::Min(RegionalDrainage,Ease((D-.24f)/1.18f));
    }
    const float Bank=FMath::Max(Ease((Nearest-305.f)/510.f),Ease((FMath::Abs(Y)-BankSupportHalfExtentV47)/700.f));
    const float Rail=Style==EFrontlineMissionTerrainV32::RailCutting?Ease((FMath::Abs(Y+1130.f)-220.f)/350.f):1.f;
    return RegionalLandformV54(X,Y,Op,Style,Bank*Rail*RegionalWater*RegionalDrainage);
}

namespace
{
constexpr float BasinGridV47=18.f,BasinShoreWidthV47=85.f;
struct FBasinV47
{
    FFieldCrater Shape;
    int32 SourceIndex=0,Group=0;
    float Level=0,InitialLevel=0,GroupTarget=0;
    bool bHasWater=false,bWetGroup=false,bMeninDrain=false;
};
struct FBasinFieldV47
{
    TArray<FBasinV47> Basins;
    TMap<FIntPoint,TArray<int32>> ShoreCells;
    bool bRail=false;
    int32 ShoreVertices=0,ExposedShoreVertices=0,SurfaceVertices=0;
    int32 ProtectedSamples=0;
    float MaximumShore=0,ProtectedMaximumChange=0;
    int32 ScanSamples=0,ScanChangedSamples=0,RefinedCells=0,BaseCells=0;
    int32 RefinedHeightOnly=0,RefinedShoreOnly=0,RefinedHeightAndShore=0;
    float ScanMaximumChange=0,ScanSquareSum=0,MaximumCentreError=0;
    int32 FineScanCells=0,FineScanCandidates=0,CalmCoreSamples=0;
    float MaximumQuarterError=0,CalmCoreMaximumChange=0;
    float ScanRefinementHalfExtent=0;
    int32 FrontageSamples=0,FrontageChangedSamples=0;
    float FrontageMaximumDelta=0,FrontageMaximumGradient=0;
    int32 FrontageOpenSamples=0;
    float FrontageIncludingBanksGradient=0,FrontageWorstBankDistance=0;
};
float BasinProtectionV47(const FBasinV47& B,const FVector2f& P,bool Rail)
{
    float V=(.94f-CraterDistance(B.Shape,P.X,P.Y,B.SourceIndex))*B.Shape.RY;
    if(B.bMeninDrain)
    {
        // The road crown is dry and passable; only the paired excavated
        // gutters can hold these pools. Trench revetments are checked below.
        const float Across=FMath::Abs(P.Y-MeninRoadAxisV59(P.X));
        V=FMath::Min(V,FMath::Min(Across-710.f,1300.f-Across));
    }
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
        V=FMath::Min(V,FMath::Max(FMath::Abs(P.X-FrontlineFieldV32::TrenchCentreX(T,P.Y))-370.f,
            FMath::Abs(P.Y)-BankSupportHalfExtentV47));
    if(Rail)V=FMath::Min(V,FMath::Abs(P.Y+1130.f)-300.f);
    return V;
}
// HeightAt samples enter only while solving a profile. Cache those levels by
// operation, geometry version and depth for the ground and basin mesh to share.
FBasinFieldV47& BasinFieldV47()
{
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    // Basin levels sample HeightAt while this cache is first constructed.
    // A V57 fallback cannot reuse levels solved against V58 terrain (or the
    // corresponding Menin variant); give each geometry version its own key.
    const int32 GeometryVersion=Op==7
        ?(GMeninV59Height.Load()?2:GMeninV58Height.Load()?1:0)
        :(Op==12&&GLysV58Height.Load()?1:0);
    const FIntVector Key(Op,int32(Style)*4+GeometryVersion,
        FMath::RoundToInt(FrontlineBattlefield::OperationDepthScale*1000000.f));
    static TMap<FIntVector,FBasinFieldV47> Profiles;
    if(auto* Found=Profiles.Find(Key))return *Found;
    FBasinFieldV47 F;
    const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    F.bRail=Missions.IsValidIndex(Op)&&Missions[Op].bArmoredTrain;
    constexpr int32 AuthoredCount=UE_ARRAY_COUNT(FieldCraters)+UE_ARRAY_COUNT(DrainageHollowsV47);
    const auto& Impacts=ImpactSitesV47();
    const float WetExtent=FMath::Min(14500.f,FrontlineBattlefield::GoalHalfLength+2100.f);
    for(int32 I=0;I<AuthoredCount+Impacts.Num();++I)
    {
        const auto& C=I<UE_ARRAY_COUNT(FieldCraters)?FieldCraters[I]:
            I<AuthoredCount?DrainageHollowsV47[I-UE_ARRAY_COUNT(FieldCraters)]:Impacts[I-AuthoredCount];
        if(!C.Wet||C.Y>4800.f)continue;
        if(I>=AuthoredCount&&(FMath::Abs(C.X)>WetExtent||C.Y<-4350.f))continue;
        FBasinV47 B;B.Shape=C;B.SourceIndex=I;
        float Bed=MAX_flt;
        for(int32 Y=-2;Y<=2;++Y)for(int32 X=-2;X<=2;++X)
            Bed=FMath::Min(Bed,AWarfareFrontlineGround::HeightAt(C.X+X*C.RX*.12f,C.Y+Y*C.RY*.12f));
        const bool Shallow=I>=UE_ARRAY_COUNT(FieldCraters);
        B.Level=Bed+C.Depth*(I>=AuthoredCount?.66f:Shallow?.49f:(Style==EFrontlineMissionTerrainV32::Marsh?.56f:.38f))+2.f;
        B.InitialLevel=B.Level;
        F.Basins.Add(B);
    }
    if(Op==7&&GMeninV59Height.Load())
    {
        const auto& Pockets=MeninDrainShapesV59();
        for(int32 I=0;I<Pockets.Num();++I)
        {
            const auto& C=Pockets[I];
            FBasinV47 B;B.Shape=C;B.SourceIndex=MeninDrainSourceBaseV59+4*I;
            B.bMeninDrain=true;
            float Bed=MAX_flt;
            for(int32 Y=-2;Y<=2;++Y)for(int32 X=-2;X<=2;++X)
                Bed=FMath::Min(Bed,AWarfareFrontlineGround::HeightAt(
                    C.X+X*C.RX*.12f,C.Y+Y*C.RY*.12f));
            B.Level=Bed+C.Depth*.54f+2.f;
            B.InitialLevel=B.Level;
            F.Basins.Add(B);
        }
    }
    // Form the physical footprint unions BEFORE solving their exposed spill
    // boundaries. A member's elliptical edge inside another hollow is not a
    // retaining wall or an outlet and must not lower the common water level.
    TArray<int32> Groups;for(int32 I=0;I<F.Basins.Num();++I)Groups.Add(I);
    auto Root=[&](int32 I){while(Groups[I]!=I)I=Groups[I];return I;};
    TMap<FIntPoint,int32> FootprintSamples;
    constexpr float JoinGrid=BasinGridV47*.5f;
    for(int32 I=0;I<F.Basins.Num();++I)
    {
        const auto& B=F.Basins[I];const auto& C=B.Shape;
        // Covers the full .94 footprint even at the maximum angular bulge.
        const float R=FMath::Max(C.RX,C.RY)*1.13f*.94f+1.f;
        const int32 X0=FMath::FloorToInt((C.X-R)/JoinGrid),X1=FMath::CeilToInt((C.X+R)/JoinGrid);
        const int32 Y0=FMath::FloorToInt((C.Y-R)/JoinGrid),Y1=FMath::CeilToInt((C.Y+R)/JoinGrid);
        for(int32 IY=Y0;IY<=Y1;++IY)for(int32 IX=X0;IX<=X1;++IX)
        {
            if(BasinProtectionV47(B,{IX*JoinGrid,IY*JoinGrid},F.bRail)<0)continue;
            const FIntPoint SampleKey(IX,IY);
            if(const auto* Other=FootprintSamples.Find(SampleKey)){const int32 A=Root(I),K=Root(*Other);if(A!=K)Groups[A]=K;}
            else FootprintSamples.Add(SampleKey,I);
        }
    }
    FootprintSamples.Empty();
    for(int32 I=0;I<F.Basins.Num();++I)F.Basins[I].Group=Root(I);
    TSet<int32> Solved;
    for(const auto& Seed:F.Basins)
    {
        if(Solved.Contains(Seed.Group))continue;Solved.Add(Seed.Group);
        float Level=-MAX_flt;
        TArray<int32,TInlineAllocator<12>> Members;
        for(int32 I=0;I<F.Basins.Num();++I)if(F.Basins[I].Group==Seed.Group){Members.Add(I);Level=FMath::Max(Level,F.Basins[I].InitialLevel);}
        const float GroupTarget=Level;
        auto Protection=[&](const FVector2f& P)
        {
            float V=-MAX_flt;
            for(int32 I:Members)V=FMath::Max(V,BasinProtectionV47(F.Basins[I],P,F.bRail));
            return V;
        };
        auto Boundary=[&](FVector2f P,FVector2f Q)
        {
            const float A=Protection(P),K=Protection(Q);if((A>=0)==(K>=0))return;
            for(int32 N=0;N<10;++N){const auto Mid=(P+Q)*.5f;if((Protection(Mid)>=0)==(A>=0))P=Mid;else Q=Mid;}
            const auto Edge=(P+Q)*.5f;Level=FMath::Min(Level,AWarfareFrontlineGround::HeightAt(Edge.X,Edge.Y)-2.f);
        };
        TSet<FIntPoint> Cells;
        for(const auto& B:F.Basins)if(B.Group==Seed.Group)
        {
            const auto& C=B.Shape;const float R=FMath::Max(C.RX,C.RY)*1.12f;
            const int32 X0=FMath::FloorToInt((C.X-R)/BasinGridV47),X1=FMath::CeilToInt((C.X+R)/BasinGridV47);
            const int32 Y0=FMath::FloorToInt((C.Y-R)/BasinGridV47),Y1=FMath::CeilToInt((C.Y+R)/BasinGridV47);
            for(int32 IY=Y0;IY<Y1;++IY)for(int32 IX=X0;IX<X1;++IX)
            {
                const FIntPoint CellKey(IX,IY);if(Cells.Contains(CellKey))continue;Cells.Add(CellKey);
                const float X=IX*BasinGridV47,Y=IY*BasinGridV47;
                Boundary({X,Y},{X+BasinGridV47,Y});Boundary({X,Y},{X,Y+BasinGridV47});
                Boundary({X,Y+BasinGridV47},{X+BasinGridV47,Y});
            }
        }
        for(auto& B:F.Basins)if(B.Group==Seed.Group){B.Level=Level;B.GroupTarget=GroupTarget;}
    }
    for(auto& B:F.Basins)
    {
        // Record actual submerged support before enabling this whole union.
        const float R=FMath::Max(B.Shape.RX,B.Shape.RY)*1.12f;
        const int32 X0=FMath::FloorToInt((B.Shape.X-R)/BasinGridV47),X1=FMath::CeilToInt((B.Shape.X+R)/BasinGridV47);
        const int32 Y0=FMath::FloorToInt((B.Shape.Y-R)/BasinGridV47),Y1=FMath::CeilToInt((B.Shape.Y+R)/BasinGridV47);
        for(int32 IY=Y0;IY<=Y1&&!B.bHasWater;++IY)for(int32 IX=X0;IX<=X1&&!B.bHasWater;++IX)
        {
            const FVector2f P(IX*BasinGridV47,IY*BasinGridV47);
            B.bHasWater=BasinProtectionV47(B,P,F.bRail)>=0&&AWarfareFrontlineGround::HeightAt(P.X,P.Y)<B.Level;
        }
    }
    for(auto& B:F.Basins)for(const auto& Other:F.Basins)if(Other.Group==B.Group&&Other.bHasWater){B.bWetGroup=true;break;}
    // The denser impact field must not turn each terrain color sample into a
    // full-map basin scan. Conservative shore bounds populate a compact grid.
    for(int32 I=0;I<F.Basins.Num();++I)
    {
        const auto& B=F.Basins[I];if(!B.bWetGroup)continue;
        const float R=FMath::Max(B.Shape.RX,B.Shape.RY)*1.13f*(.94f+BasinShoreWidthV47/B.Shape.RY)+1.f;
        for(int32 Y=FMath::FloorToInt((B.Shape.Y-R)/800.f);Y<=FMath::FloorToInt((B.Shape.Y+R)/800.f);++Y)
            for(int32 X=FMath::FloorToInt((B.Shape.X-R)/800.f);X<=FMath::FloorToInt((B.Shape.X+R)/800.f);++X)
                F.ShoreCells.FindOrAdd(FIntPoint(X,Y)).Add(I);
    }
    return Profiles.Add(Key,MoveTemp(F));
}
float BasinShoreV47(const FBasinFieldV47& Field,float X,float Y,float Height,float Slope,bool* Exposed=nullptr)
{
    float Shore=0;if(Exposed)*Exposed=false;
    const auto* Candidates=Field.ShoreCells.Find(FIntPoint(FMath::FloorToInt(X/800.f),FMath::FloorToInt(Y/800.f)));
    if(!Candidates)return 0;
    for(int32 I:*Candidates)
    {
        const auto& B=Field.Basins[I];
        if(!B.bWetGroup)continue;
        // Protection's radial margin is expressed in RY units. The bound must
        // include that scaled fade and the full .085+.045 angular expansion;
        // adding 85 world cm alone clips long-axis shores to a rectangle.
        const float R=FMath::Max(B.Shape.RX,B.Shape.RY)*1.13f*(.94f+BasinShoreWidthV47/B.Shape.RY)+1.f;
        if(FMath::Abs(X-B.Shape.X)>R||FMath::Abs(Y-B.Shape.Y)>R)continue;
        const float Protection=BasinProtectionV47(B,{X,Y},Field.bRail);
        // Convert height above the physical level into an approximate shoreline
        // distance, so a steep bank and a shallow beach get the same 85 cm band.
        const float Distance=FMath::Max(FMath::Max(0.f,Height-B.Level)/FMath::Max(.16f,Slope),-Protection);
        const float Wet=1-Ease(Distance/BasinShoreWidthV47);
        Shore=FMath::Max(Shore,Wet);
        if(Exposed&&Wet>.05f&&Height>B.Level+1.2f)*Exposed=true;
    }
    return Shore;
}
TSharedRef<FJsonObject> BasinFieldReportV47()
{
    const auto& F=BasinFieldV47();auto J=MakeShared<FJsonObject>();
    J->SetNumberField(TEXT("resolved_basins"),F.Basins.Num());J->SetNumberField(TEXT("auxiliary_hollows"),UE_ARRAY_COUNT(DrainageHollowsV47));
    int32 MeninPockets=0,MeninWetPockets=0;
    for(const auto& B:F.Basins)if(B.bMeninDrain){++MeninPockets;MeninWetPockets+=B.bHasWater?1:0;}
    J->SetNumberField(TEXT("menin_gully_pockets"),MeninPockets);
    J->SetNumberField(TEXT("menin_gully_wet_pockets"),MeninWetPockets);
    J->SetNumberField(TEXT("composed_impact_sites"),ImpactSitesV47().Num());
    J->SetNumberField(TEXT("impact_site_pitch_cm"),ImpactPitchV47);
    J->SetNumberField(TEXT("shore_lookup_cells"),F.ShoreCells.Num());
    J->SetNumberField(TEXT("surface_vertices"),F.SurfaceVertices);J->SetNumberField(TEXT("shore_vertices"),F.ShoreVertices);
    J->SetNumberField(TEXT("exposed_shore_vertices"),F.ExposedShoreVertices);J->SetNumberField(TEXT("maximum_shore_mask"),F.MaximumShore);
    J->SetNumberField(TEXT("shore_band_cm"),BasinShoreWidthV47);J->SetBoolField(TEXT("same_resolved_levels_for_surface_and_water"),true);
    J->SetNumberField(TEXT("protected_contact_samples"),F.ProtectedSamples);J->SetNumberField(TEXT("protected_maximum_height_change_cm"),F.ProtectedMaximumChange);
    J->SetStringField(TEXT("protected_comparison"),TEXT("Actual production HeightAt against retained V46 terms at curved bank +/-285cm over 18 rows within +/-1740cm Y, and railway +/-200cm. Rows outside actual V42 bank/board/dugout bounds were replaced explicitly. Open crossing frontage is checked separately for bounded additional scanned relief."));
    J->SetNumberField(TEXT("bank_support_half_extent_cm"),BankSupportHalfExtentV47);
    J->SetNumberField(TEXT("bank_release_width_cm"),BankReleaseWidthV47);
    J->SetNumberField(TEXT("bank_contact_sample_half_extent_cm"),1740);
    auto Frontage=MakeShared<FJsonObject>();
    Frontage->SetNumberField(TEXT("samples"),F.FrontageSamples);Frontage->SetNumberField(TEXT("changed_samples"),F.FrontageChangedSamples);
    Frontage->SetNumberField(TEXT("maximum_delta_cm"),F.FrontageMaximumDelta);
    Frontage->SetNumberField(TEXT("maximum_contact_gradient"),F.FrontageMaximumGradient);
    Frontage->SetNumberField(TEXT("open_ground_contact_samples"),F.FrontageOpenSamples);
    Frontage->SetNumberField(TEXT("maximum_gradient_including_retained_banks"),F.FrontageIncludingBanksGradient);
    Frontage->SetNumberField(TEXT("worst_inclusive_sample_distance_to_bank_cm"),F.FrontageWorstBankDistance);
    Frontage->SetNumberField(TEXT("maximum_permitted_absolute_delta_cm"),120);
    Frontage->SetNumberField(TEXT("scan_contact_strength"),.55);
    Frontage->SetStringField(TEXT("comparison"),TEXT("165 actual HeightAt samples over |Y|<=720 against retained pre-impact terms. Gradient limit applies to open ground >=500cm from a trench centre, matching the runtime contact check. Existing steep trench walls are checked separately for unchanged footing, not required to become walkable slopes. Inclusive maximum and its bank clearance remain reported."));
    J->SetObjectField(TEXT("frontage_relief_v51"),Frontage);
    J->SetStringField(TEXT("reference_rubric"),TEXT("V42 normal wide: low middle-distance silhouette, patchy torn clods, quiet compacted clay and connected low puddles with damp sediment margins. Numeric checks do not establish visual parity."));
    auto Scan=MakeShared<FJsonObject>();
    Scan->SetStringField(TEXT("source_sha256"),UTF8_TO_TCHAR(FrontlineScanHeightV47::SourceSHA256));
    Scan->SetNumberField(TEXT("source_bit_depth"),16);Scan->SetNumberField(TEXT("lut_size"),FrontlineScanHeightV47::Size);
    Scan->SetNumberField(TEXT("artist_peak_to_valley_cm"),FrontlineScanHeightV47::ReliefRangeCm*AggregateScanStrengthV47);
    Scan->SetNumberField(TEXT("aggregate_scan_strength"),AggregateScanStrengthV47);
    Scan->SetNumberField(TEXT("world_cm_per_scan_repeat"),145.f/.52f);
    Scan->SetNumberField(TEXT("actual_production_samples"),F.ScanSamples);Scan->SetNumberField(TEXT("changed_samples"),F.ScanChangedSamples);
    Scan->SetNumberField(TEXT("maximum_change_cm"),F.ScanMaximumChange);
    Scan->SetNumberField(TEXT("rms_change_cm"),F.ScanSamples?FMath::Sqrt(F.ScanSquareSum/F.ScanSamples):0);
    Scan->SetNumberField(TEXT("base_cells"),F.BaseCells);Scan->SetNumberField(TEXT("refined_cells"),F.RefinedCells);
    Scan->SetNumberField(TEXT("refined_height_only_cells"),F.RefinedHeightOnly);
    Scan->SetNumberField(TEXT("refined_shore_only_cells"),F.RefinedShoreOnly);
    Scan->SetNumberField(TEXT("refined_height_and_shore_cells"),F.RefinedHeightAndShore);
    Scan->SetNumberField(TEXT("maximum_base_centre_interpolation_error_cm"),F.MaximumCentreError);
    Scan->SetNumberField(TEXT("second_refinement_cells"),F.FineScanCells);Scan->SetNumberField(TEXT("second_refinement_candidates"),F.FineScanCandidates);
    Scan->SetNumberField(TEXT("second_refinement_cell_budget"),8000);
    Scan->SetNumberField(TEXT("second_refinement_half_extent_cm"),F.ScanRefinementHalfExtent);
    Scan->SetNumberField(TEXT("maximum_candidate_one_level_quarter_error_cm"),F.MaximumQuarterError);
    Scan->SetNumberField(TEXT("calm_core_samples"),F.CalmCoreSamples);Scan->SetNumberField(TEXT("calm_core_maximum_scan_change_cm"),F.CalmCoreMaximumChange);
    Scan->SetNumberField(TEXT("wet_margin_maximum_displacement_cm"),3);
    int32 ProfileOp=0;EFrontlineMissionTerrainV32 ProfileStyle;
    FrontlineFieldV32::TerrainProfileV43(ProfileOp,ProfileStyle);
    const bool MaterialFirst=ProfileOp==7&&GMeninV59Height.Load();
    Scan->SetBoolField(TEXT("material_first_v59"),MaterialFirst);
    Scan->SetStringField(TEXT("comparison"),MaterialFirst
        ?TEXT("V59 moves dry sub-metre aggregate off HeightAt into the existing material scans. Only a bounded wet-lip displacement remains; first refinement still protects actual landform and water-shore interpolation. Counts separate height and shore triggers.")
        :TEXT("Production HeightAt versus identical V47 height with scan/clipped-margin relief disabled, on a deterministic 33 by 31 apron grid. Only up to 8000 cells use 12.5 by 8.3125 cm scan geometry; this remains a filtered scan, not the full source resolution."));
    J->SetObjectField(TEXT("scan_displacement_v47"),Scan);
    TArray<TSharedPtr<FJsonValue>> Rows;
    int32 WetBasins=0;
    for(const auto& B:F.Basins){auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("source"),B.SourceIndex);R->SetNumberField(TEXT("level_cm"),B.Level);R->SetNumberField(TEXT("authored_initial_level_cm"),B.InitialLevel);R->SetNumberField(TEXT("group_target_level_cm"),B.GroupTarget);R->SetNumberField(TEXT("hydraulic_group"),B.Group);R->SetNumberField(TEXT("protection_lowering_cm"),B.GroupTarget-B.Level);R->SetBoolField(TEXT("has_submerged_grid_vertex"),B.bHasWater);WetBasins+=B.bHasWater;Rows.Add(MakeShared<FJsonValueObject>(R));}
    J->SetNumberField(TEXT("actual_wet_lobes"),WetBasins);
    J->SetArrayField(TEXT("levels"),Rows);return J;
}

// A thin, broken flooded bed is authored from the SAME displaced height as
// boots and vehicle tracks. These two campaign landmarks were numerically
// present but unreadable in the normal side camera without a reflective plane.
// A metre-scale shore clip and the V47 pool footprint keep it off existing
// water, revetments and dry high ground; there is no collision-only fake lake.
struct FRegionalWaterV54 {float Margin=-MAX_flt,Level=0,Depth=0,SignedShoreCm=-MAX_flt;};
FRegionalWaterV54 MeninGullyWaterV63(float X,float Y,const FBasinFieldV47& Field,float GroundSlopeForShore)
{
    FRegionalWaterV54 R;
    // V59 already excavates both gutters in HeightAt. The water is clipped
    // against that actual bed; it is not a translucent sheet over dry ground.
    if(FMath::Abs(X)>3850.f||FMath::Abs(Y)>3850.f)return R;
    const float Across=Y-MeninRoadAxisV59(X);
    const bool South=Across>=0.f;
    const float Axis=MeninRoadAxisV59(X)+MeninGullyOffsetV59(X,South);
    const float Lateral=FMath::Abs(Y-Axis);
    if(Lateral>400.f)return R;
    const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
    const auto Section=MeninRoadV59(X,Y,Home);
    if(Section.Road>.08f||Section.Gully<.23f)return R;
    const float Side=South?1.f:-1.f;
    const float Width=118.f+27.f*FMath::Sin(X/505.f+Side*.8f)
        +18.f*FMath::PerlinNoise2D(FVector2D(X/220.f+Side*3.f,Y/285.f));
    const float Silt=.5f+.5f*FMath::Sin(X/615.f+Side*1.9f+.42f*FMath::Sin(X/211.f));
    float Shore=FMath::Min(Width-Lateral,
        FMath::Min((Silt-.31f)*210.f,(Section.Gully-.24f)*360.f));
    float NearestTrench=FLT_MAX;
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
        NearestTrench=FMath::Min(NearestTrench,FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y)));
    Shore=FMath::Min(Shore,FMath::Max(NearestTrench-395.f,
        FMath::Abs(Y)-BankSupportHalfExtentV47));
    const bool DrySide=GroundSlopeForShore>=0.f;
    if(Shore<(DrySide?-240.f:0.f))return R;
    // Existing levelled craters remain separate hydraulically; a new gutter
    // must not overlap a basin with a different water elevation.
    if(const auto* Candidates=Field.ShoreCells.Find(FIntPoint(FMath::FloorToInt(X/800.f),FMath::FloorToInt(Y/800.f))))
        for(int32 I:*Candidates)if(Field.Basins[I].bWetGroup&&
            BasinProtectionV47(Field.Basins[I],{X,Y},Field.bRail)>-55.f)return R;
    // A low-pass centreline samples the same V59 physical height used by
    // terrain and collision. Its broad slope represents runoff through the
    // gutter; short silt bars above expose stretches of muddy bed.
    auto Bed=[&](float AtX)
    {
        return AWarfareFrontlineGround::HeightAt(AtX,
            MeninRoadAxisV59(AtX)+MeninGullyOffsetV59(AtX,South));
    };
    R.Level=.25f*Bed(X-205.f)+.5f*Bed(X)+.25f*Bed(X+205.f)+13.f;
    R.Depth=R.Level-AWarfareFrontlineGround::HeightAt(X,Y);
    const float DepthMargin=R.Depth-.8f;
    R.Margin=FMath::Min(Shore,DepthMargin);
    if(DrySide)R.SignedShoreCm=FMath::Min(Shore,
        DepthMargin/FMath::Max(.08f,GroundSlopeForShore));
    return R;
}
FRegionalWaterV54 LegacyRegionalWaterV54(float X,float Y,const FBasinFieldV47& Field,float GroundSlopeForShore=-1.f)
{
    FRegionalWaterV54 R;
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    if(Op==7&&GMeninV59Height.Load()&&GMeninV63Water.Load())
        return MeninGullyWaterV63(X,Y,Field,GroundSlopeForShore);
    if(Style!=EFrontlineMissionTerrainV32::Marsh&&Style!=EFrontlineMissionTerrainV32::Canal)return R;
    // Ground vertices need the dry-side contour for the Lys too. The terrain
    // already requests it for op12; excluding op12 here left an opaque water
    // polygon against unstained, dry-looking mud. Water queries (no supplied
    // slope) still retain their exact wet footprint.
    const bool bLysV58=Op==12&&GLysV58Height.Load();
    const bool bDrySideShoreV56=GroundSlopeForShore>=0.f&&(Op==5||bLysV58||Op==16);
    const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
    if(Style==EFrontlineMissionTerrainV32::Marsh)
    {if(FMath::Abs(X)>Home+3300.f||Y>(Op==12?2650.f:1500.f)||Y<(Op==12?-7010.f:-3800.f))return R;}
    else if(Op==16?(X< -3500.f||X>6500.f||Y>2400.f||Y< -7000.f):
        (X< -2600.f||X>6200.f||Y>3000.f||Y< -5350.f))return R;
    float Shore=-MAX_flt;
    if(Style==EFrontlineMissionTerrainV32::Marsh)
    {
        if(Op==12)
        {
            const auto Wash=LysWashV57(X,Y,Home);
            if(bLysV58)
            {
                // Reject distant dry ground before the finer bank/island
                // noise. The broad gate only bounds the physical runnels.
                if(FMath::Abs(Wash.Distance)>Wash.WaterHalfWidth*1.45f+350.f)return R;
                const float ErodedBank=24.f*FMath::PerlinNoise2D(FVector2D(X/215.f+7.8f,Y/240.f))
                    +8.f*FMath::PerlinNoise2D(FVector2D(X/77.f,Y/93.f+3.1f));
                Shore=FMath::Min(Wash.WaterHalfWidth*1.45f+ErodedBank-FMath::Abs(Wash.Distance),
                    FMath::Min((Wash.Span-.43f)*390.f,(.35f-Wash.Ford)*240.f));
            }
            else
            {
                // Preserve the accepted V57 water footprint with V57 tiles.
                Shore=FMath::Min(Wash.WaterHalfWidth-FMath::Abs(Wash.Distance),
                    FMath::Min((Wash.Span-.43f)*390.f,(.35f-Wash.Ford)*240.f));
            }
        }
        else
        {
        const float OffsetB=float((Op*11)%7-3)*73.f;
        const float Centre=-90.f+OffsetB+205.f*FMath::Sin(X/1590.f+Op*.29f)+.045f*X;
        const float FrontAxis=Centre-170.f+115.f*FMath::Sin(X/960.f+Op*.17f);
        // Flooded open water occupies the deeper meandering threads only.
        // The surrounding marsh trough remains saturated, exposed mud with
        // its existing height and movement mask rather than a featureless lake.
        const float Width=(Op==5?237.f:151.f)
            +53.f*FMath::Sin(X/1120.f+1.5f)*FMath::Sin(X/510.f+.6f)
            +61.f*FMath::PerlinNoise2D(FVector2D(X/310.f+12.7f,Y/490.f));
        const float Front=FMath::Min(Width-FMath::Abs(Y-FrontAxis),
            (1-Ease((FMath::Abs(X)-Home-1300.f)/1500.f)) * 330.f-60.f);
        const float RearAxis=-2600.f-.10f*X-145.f*FMath::Sin(X/1750.f+.44f);
        const float RearWidth=(Op==5?206.f:168.f)
            +61.f*FMath::Sin(X/1330.f+2.2f)*FMath::Sin(X/560.f)
            +66.f*FMath::PerlinNoise2D(FVector2D(X/370.f,Y/410.f+6.1f));
        const float Rear=FMath::Min(RearWidth-FMath::Abs(Y-RearAxis),
            (1-Ease((FMath::Abs(X)-Home-2150.f)/1700.f))*330.f-60.f);
        Shore=FMath::Max(Front,Rear);
        if(Op==5)
        {
            const auto Braid=YpresBraidV55(X,Y,Home);
            const float Middle=FMath::Min(Braid.ShoreHalfWidth-FMath::Abs(Y-Braid.Axis),
                (Braid.Span-.44f)*510.f);
            Shore=FMath::Max(Shore,Middle);
        }
        }
    }
    else
    {
        if(Op==16)
        {
            const auto S=StQuentinSectionV55(X,Y,Home);
            // A narrow, continuous water thread follows the actual excavated
            // floor. The two breached crossings and existing levelled impact
            // basins interrupt it physically, rather than random grey slabs.
            const float Width=222.f+32.f*FMath::Sin(Y/760.f+.7f)
                +19.f*FMath::PerlinNoise2D(FVector2D(X/210.f,Y/330.f));
            Shore=FMath::Min(Width-FMath::Abs(S.Distance),
                FMath::Min((S.Length-.54f)*620.f,(.71f-S.Gap)*430.f));
        }
        else
        {
        const float OffsetA=float((Op*7)%5-2)*132.f;
        const float Axis=.47f*Home+.55f*OffsetA+.36f*(Y+1900.f)
            +105.f*FMath::Sin(Y/910.f+Op*.21f);
        const float Length=(1-Ease((Y-2450.f)/1250.f))*(1-Ease((-Y-4700.f)/1000.f));
        const float Width=132.f+29.f*FMath::Sin(Y/810.f+Op*.27f)
            +34.f*FMath::PerlinNoise2D(FVector2D(X/170.f+4.2f,Y/350.f));
        // The excavated trench and towpath still reach the foreground, but
        // open water ends at an irregular silt bar in the middle distance.
        // This leaves a traversable wet-clay floor where troops cross instead
        // of projecting a huge triangular water decal into the bottom HUD.
        const float FrontWater=1-Ease((Y+2050.f+210.f*FMath::Sin(X/930.f+1.1f)
            +85.f*FMath::Sin(X/290.f+.6f))/1060.f);
        // Silt dams expose the floor of the main cut in short reaches. They
        // interrupt the old solid diagonal grey ribbon but leave the entire
        // excavated depression and its wet-mud traversal mask intact.
        const float MainSilt=.5f+.5f*FMath::Sin(Y/390.f+.64f*FMath::Sin(Y/980.f+Op*.31f));
        const float RearAxis=-3670.f-110.f*FMath::Sin(X/1450.f+Op*.13f);
        const float CrossingClearance=FMath::Abs(Y-RearAxis);
        const float Main=FMath::Min(Width-FMath::Abs(X-Axis),
            FMath::Min(FMath::Min((Length-.43f)*560.f,(FrontWater-.24f)*430.f),
                FMath::Min((MainSilt-.25f)*250.f,(CrossingClearance-210.f)*.8f)));
        const float Span=1-Ease((FMath::Abs(X-1650.f)-2580.f)/1350.f);
        // The photographed canal is continuous in the far background; its
        // damaged near-bank cut reads better as separate shallow pools across
        // exposed silt bars than as one opaque straight strip on the mesh.
        const float RearSilt=.5f+.5f*FMath::Sin(X/425.f+.58f*FMath::Sin(X/890.f+Op*.23f));
        const float Rear=FMath::Min(99.f+22.f*FMath::Sin(X/970.f)
            +26.f*FMath::PerlinNoise2D(FVector2D(X/330.f,Y/180.f+10.1f))-FMath::Abs(Y-RearAxis),
            FMath::Min((Span-.45f)*560.f,(RearSilt-.41f)*280.f));
        Shore=FMath::Max(Main,Rear);
        }
    }
    if(Shore<=0.f&&!bDrySideShoreV56)return R;
    if(bDrySideShoreV56&&Shore< -270.f)return R;
    const auto Features=AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y);
    const float Physical=Style==EFrontlineMissionTerrainV32::Marsh?Features.MarshChannel:Features.CanalCut;
    if(Physical<=.43f&&!bDrySideShoreV56)return R;
    const float PhysicalMargin=(Physical-.43f)*520.f;
    if(bDrySideShoreV56&&PhysicalMargin< -270.f)return R;
    // The older impact pools have separately levelled water. Cut this ribbon
    // around their protected footprint, avoiding intersecting surface sheets.
    if(const auto* Candidates=Field.ShoreCells.Find(FIntPoint(FMath::FloorToInt(X/800.f),FMath::FloorToInt(Y/800.f))))
        for(int32 I:*Candidates)if(Field.Basins[I].bWetGroup&&
            BasinProtectionV47(Field.Basins[I],{X,Y},Field.bRail)>0.f)return R;
    const float Broad=25.f+26.f*FMath::Sin(X/1550.f)*FMath::Cos(Y/1250.f)
        +Ease((-Y-1900.f)/3100.f)*(80.f+42.f*FMath::Sin(X/1900.f)+28.f*FMath::Cos(X/780.f));
    R.Level=Broad+FrontlineFieldV32::TerrainLift(X,Y,Broad)
        -(Op==12?60.f:Style==EFrontlineMissionTerrainV32::Marsh?72.f:105.f);
    R.Depth=R.Level-AWarfareFrontlineGround::HeightAt(X,Y);
    // The visible water plane is 1.2 cm above the nominal level. Its Lys
    // contact must use that same surface: the former 2 cm nominal-depth
    // cutoff left a ~3 cm vertical lip at a hard opaque polygon edge. Keep
    // the accepted older operations' exposed-silt cutoff unchanged.
    const float SiltShelf=bLysV58?
        .38f+.24f*FMath::PerlinNoise2D(FVector2D(X/94.f,Y/112.f+5.4f)):
        2.1f+1.5f*FMath::Sin(X/123.f)*FMath::Sin(Y/151.f)
            +1.2f*FMath::PerlinNoise2D(FVector2D(X/97.f,Y/119.f));
    const float DepthMargin=R.Depth+(bLysV58?1.2f:0.f)-SiltShelf;
    if(bLysV58)R.Depth=FMath::Max(0.f,R.Depth+1.2f);
    R.Margin=FMath::Min(FMath::Min(Shore,PhysicalMargin),DepthMargin);
    if(bDrySideShoreV56)
    {
        // All three gates retain the same zero contour as the physical mesh.
        // Depth is vertical centimetres, so convert it to an approximate
        // horizontal shore reach using the already-computed ground gradient.
        // This makes the dry-side stain a bounded bank, not a second water
        // sheet or a wide tint wherever the canal floor is merely shallow.
        R.SignedShoreCm=FMath::Min(FMath::Min(Shore,PhysicalMargin),
            DepthMargin/FMath::Max(.06f,GroundSlopeForShore));
    }
    return R;
}
#include "WarfareFrontlineRegionalWaterV72.inl"
}

bool AWarfareFrontlineGround::QueryWaterSurfaceV51(float X,float Y,float& Level,float& Depth,float* BasinIdentity)
{
    Level=Depth=0;
    if(BasinIdentity)*BasinIdentity=-1.f;
    if(!FMath::IsFinite(X)||!FMath::IsFinite(Y))return false;
    if(WarfareFRRUWorldV82::PhysicalSceneActive())
        return WarfareFRRUWorldV82::TryGlobalWater(X,Y,Level,Depth,BasinIdentity);
    const auto& Field=BasinFieldV47();
    const auto* Candidates=Field.ShoreCells.Find(FIntPoint(FMath::FloorToInt(X/800.f),FMath::FloorToInt(Y/800.f)));
    if(Candidates)
    {
        const float Bed=HeightAt(X,Y);
        for(int32 I:*Candidates)
        {
            const auto& B=Field.Basins[I];
            if(B.bWetGroup&&BasinProtectionV47(B,{X,Y},Field.bRail)>=0&&B.Level>Bed+.1f)
            {
                Level=B.Level+1.2f;Depth=B.Level-Bed;
                // Static-mesh vertex colours are stored as UNorm8. Return the same
                // quantized group carried by the displayed basin mesh's green channel.
                if(BasinIdentity)*BasinIdentity=FLinearColor(0.f,B.Group/float(Field.Basins.Num()),0.f,1.f).ToFColor(true).G/255.f;
                return true;
            }
        }
    }
    const auto Regional=RegionalWaterV54(X,Y,Field);
    if(Regional.Margin>=0.f)
    {
        Level=Regional.Level+1.2f;Depth=Regional.Depth;
        if(BasinIdentity)
        {
            const auto* Data=ActiveWaterPlanesV72.Get();const int32 Group=RegionalGroupV72(Data,X,Y);
            *BasinIdentity=!FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline"))&&Data&&Data->GroupColours.IsValidIndex(Group)
                ?Data->GroupColours[Group]/255.f:FLinearColor(0.f,.97f,0.f,1.f).ToFColor(true).G/255.f;
        }
        return true;
    }
    return false;
}

void AWarfareFrontlineGround::RunChecksV51(TFunctionRef<void(const FString&,bool)> Check)
{
    int32 Op;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    int32 Changed=0,Wet=0;float Peak=0,Gradient=0;bool Finite=true;
    for(float X=-FrontlineBattlefield::GoalHalfLength;X<=FrontlineBattlefield::GoalHalfLength;X+=240.f)
        for(float Y=-600;Y<=600;Y+=150.f)
        {
            float Bank=MAX_flt;
            for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)Bank=FMath::Min(Bank,FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y)));
            if(Bank<500)continue;
            const float H=HeightAt(X,Y),Delta=H-UnpooledHeightV42(X,Y,false);
            Changed+=FMath::Abs(Delta)>8.f;Peak=FMath::Max(Peak,FMath::Abs(Delta));Finite&=FMath::IsFinite(H);
            const float DX=(HeightAt(X+12,Y)-HeightAt(X-12,Y))/24.f;
            const float DY=(HeightAt(X,Y+12)-HeightAt(X,Y-12))/24.f;
            Gradient=FMath::Max(Gradient,FMath::Sqrt(DX*DX+DY*DY));
            float Level,Depth;Wet+=QueryWaterSurfaceV51(X,Y,Level,Depth);
        }
    Check(TEXT("V51 middle crossing contains real crater relief, not a flat protected band"),Changed>15&&Peak>20.f);
    Check(TEXT("V51 terrain contact queries remain finite and traversable"),Finite&&Gradient<1.8f);
    Check(TEXT("V51 actual water reaches the central battlefield"),Wet>0);
    float Protected=0;
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)for(float Y=-1600;Y<=1600;Y+=200)
        for(float DX:{-280.f,0.f,280.f})
        {const float X=FrontlineFieldV32::TrenchCentreX(T,Y)+DX;Protected=FMath::Max(Protected,FMath::Abs(HeightAt(X,Y)-UnpooledHeightV42(X,Y,false)));}
    Check(TEXT("V51 existing trench floor and revetment supports are preserved"),Protected<.002f);
    const auto& Current=ImpactSitesV47();const FVector First(Current[300].X,Current[300].Y,Current[300].Depth);
    int32 Other=(Op+1)%20;auto OtherStyle=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British)[Other].TerrainStyleV32;
    FrontlineFieldV32::TerrainProfileV43(Other,OtherStyle,true);
    const auto& Next=ImpactSitesV47();const FVector Second(Next[300].X,Next[300].Y,Next[300].Depth);
    FrontlineFieldV32::TerrainProfileV43(Op,Style,true);
    const auto& Again=ImpactSitesV47();
    Check(TEXT("V51 operation scars vary but regenerate deterministically"),!First.Equals(Second,1.f)&&First.Equals(FVector(Again[300].X,Again[300].Y,Again[300].Depth),.001f));
}

void AWarfareFrontlineGround::RunChecksV54(TFunctionRef<void(const FString&,bool)> Check)
{
    int32 SavedOp=0;EFrontlineMissionTerrainV32 SavedStyle;FrontlineFieldV32::TerrainProfileV43(SavedOp,SavedStyle);
    ON_SCOPE_EXIT{FrontlineFieldV32::TerrainProfileV43(SavedOp,SavedStyle,true);};
    const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
    for(const auto Style:{EFrontlineMissionTerrainV32::Ridge,EFrontlineMissionTerrainV32::Marsh,
        EFrontlineMissionTerrainV32::Canal,EFrontlineMissionTerrainV32::RailCutting,EFrontlineMissionTerrainV32::Village})
    {
        int32 TestOp=SavedOp;auto TestStyle=Style;FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        float Peak=0,Positive=0,Negative=0,MaximumSlope=0;int32 Samples=0;
        for(float Y=-1400.f;Y<=900.f;Y+=180.f)for(float X=-Home;X<=Home;X+=180.f)
        {
            const auto F=QueryTerrainFeaturesV54(X,Y);
            const float Mask=Style==EFrontlineMissionTerrainV32::Ridge?F.RidgeCrest:
                Style==EFrontlineMissionTerrainV32::Marsh?F.MarshChannel:
                Style==EFrontlineMissionTerrainV32::Canal?F.CanalCut:
                Style==EFrontlineMissionTerrainV32::RailCutting?F.RailShoulder:FMath::Max(F.Ruins,F.RuinCellar);
            Peak=FMath::Max(Peak,Mask);Positive=FMath::Max(Positive,F.ReliefCm);
            Negative=FMath::Min(Negative,F.ReliefCm);++Samples;
            if(int32(X+Home)%720==0&&int32(Y+1400.f)%720==0)
            {
                const float DX=(HeightAt(X+24.f,Y)-HeightAt(X-24.f,Y))/48.f;
                const float DY=(HeightAt(X,Y+24.f)-HeightAt(X,Y-24.f))/48.f;
                MaximumSlope=FMath::Max(MaximumSlope,FMath::Sqrt(DX*DX+DY*DY));
            }
        }
        const bool Displaced=Style==EFrontlineMissionTerrainV32::Ridge?Positive>45.f:
            Style==EFrontlineMissionTerrainV32::Marsh?Negative< -22.f:
            Style==EFrontlineMissionTerrainV32::Canal?Negative< -45.f:
            Style==EFrontlineMissionTerrainV32::RailCutting?Positive>30.f:Positive>22.f&&Negative< -6.f;
        Check(FString::Printf(TEXT("V54 profile %d has visible displaced relief and a spatial gameplay mask"),int32(Style)),
            Samples>100&&Peak>.28f&&Displaced&&MaximumSlope<1.8f);
        UE_LOG(LogTemp,Display,TEXT("WF_V54_MORPH style=%d peak_mask=%.3f positive_cm=%.1f negative_cm=%.1f max_sampled_slope=%.3f"),
            int32(Style),Peak,Positive,Negative,MaximumSlope);
    }
    for(const int32 MineOp:{3,4})
    {
        int32 TestOp=MineOp;auto TestStyle=EFrontlineMissionTerrainV32::Ridge;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const float Shift=MineOp==4?175.f:0.f;
        const float CX=-.10f*Home+Shift,CY=435.f;
        const auto Mine=MessinesMineV55(CX,CY,MineOp,Home);
        const auto Core=RegionalLandformV54(CX,CY,MineOp,TestStyle,1.f);
        // Menin Road now has its own cut/road section. Compare the mined
        // saddle with the retained, unmined chalk profile instead.
        const auto Unmined=RegionalLandformV54(CX,CY,0,TestStyle,1.f);
        float MaximumSlope=0,PeakRelief=0;
        for(float Y=-1250.f;Y<=1000.f;Y+=180.f)for(float X=-.60f*Home;X<=.68f*Home;X+=180.f)
        {
            const auto F=RegionalLandformV54(X,Y,MineOp,TestStyle,1.f);
            PeakRelief=FMath::Max(PeakRelief,F.ReliefCm);
            const float DX=(HeightAt(X+24.f,Y)-HeightAt(X-24.f,Y))/48.f;
            const float DY=(HeightAt(X,Y+24.f)-HeightAt(X,Y-24.f))/48.f;
            MaximumSlope=FMath::Max(MaximumSlope,FMath::Sqrt(DX*DX+DY*DY));
        }
        Check(FString::Printf(TEXT("V55 Messines operation %d has a mined saddle rather than the Menin hogback"),MineOp),
            Mine.Cut>.98f&&Core.ReliefCm+40.f<Unmined.ReliefCm
            &&PeakRelief<220.f&&MaximumSlope<1.8f);
    }
    {
        int32 TestOp=7;auto TestStyle=EFrontlineMissionTerrainV32::Ridge;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const bool PreviousMeninVersion=GMeninV58Height.Load();
        const bool PreviousMaterialFirst=GMeninV59Height.Load();
        GMeninV58Height.Store(true);
        GMeninV59Height.Store(false);
        const float X=-.36f*Home;
        const float RoadY=-150.f+.115f*X+125.f*FMath::Sin(X/1450.f+.42f);
        const auto Road=MeninRoadV58(X,RoadY,Home);
        const auto Lower=MeninRoadV58(X,RoadY+970.f,Home);
        const auto High=RegionalLandformV54(X,RoadY,7,TestStyle,1.f);
        const auto Low=RegionalLandformV54(X,RoadY+970.f,7,TestStyle,1.f);
        Check(TEXT("v58_menin_road_has_a_raised_exposed_route_beside_real_woodland_gullies"),
            Road.Road>.85f&&Lower.Gully>.40f&&Road.Gully<Lower.Gully*.35f
            &&High.ReliefCm>Low.ReliefCm+55.f&&Low.WoodlandGully>.4f);
        bool BothBanks=true;
        for(int32 Step=-4;Step<=4;++Step)
        {
            const float Along=Step*.13f*Home;
            const float Axis=-150.f+.115f*Along+125.f*FMath::Sin(Along/1450.f+.42f);
            const auto Crown=RegionalLandformV54(Along,Axis,7,TestStyle,1.f);
            const auto South=RegionalLandformV54(Along,Axis+970.f,7,TestStyle,1.f);
            const auto North=RegionalLandformV54(Along,Axis-970.f,7,TestStyle,1.f);
            BothBanks&=Crown.ReliefCm>South.ReliefCm+70.f
                &&Crown.ReliefCm>North.ReliefCm+70.f
                &&South.WoodlandGully>.40f&&North.WoodlandGully>.40f;
        }
        Check(TEXT("v58_menin_continuous_physical_road_and_two_drainage_gullies"),BothBanks);
        const float RoadSemantic=MeninSurfaceSemanticV58(X,RoadY,Home);
        const float GullySemantic=MeninSurfaceSemanticV58(X,RoadY+970.f,Home);
        Check(TEXT("v58_menin_surface_vertex_semantic_tracks_the_real_road_and_gully"),
            RoadSemantic>.84f&&GullySemantic<.37f&&RoadSemantic-GullySemantic>.48f);
        const FFieldCrater V58Impact=ImpactSitesV47()[300];
        GMeninV59Height.Store(true);
        const auto& V59Sites=ImpactSitesV47();
        int32 Occupied=0,QuietBlocks=0,DenseBlocks=0;
        float SmallRadius=FLT_MAX,LargeRadius=0.f,Shallow=FLT_MAX,Deep=0.f;
        bool LocalLookupBounded=true;
        for(const auto& C:V59Sites)
        {
            if(C.Depth<8.f){LocalLookupBounded&=!C.Wet;continue;}
            ++Occupied;
            SmallRadius=FMath::Min(SmallRadius,C.RX);
            LargeRadius=FMath::Max(LargeRadius,C.RX);
            Shallow=FMath::Min(Shallow,C.Depth);
            Deep=FMath::Max(Deep,C.Depth);
            LocalLookupBounded&=FMath::Max(C.RX,C.RY)*1.13f*1.40f<ImpactPitchV47;
        }
        for(int32 Y=ImpactMinYV47;Y<=ImpactMaxYV47-3;++Y)
            for(int32 CellX=ImpactMinXV47;CellX<=ImpactMaxXV47-3;++CellX)
            {
                int32 Count=0;
                for(int32 DY=0;DY<4;++DY)for(int32 DX=0;DX<4;++DX)
                    Count+=V59Sites[(Y+DY-ImpactMinYV47)*ImpactColumnsV47+CellX+DX-ImpactMinXV47].Depth>=8.f;
                QuietBlocks+=Count==0;DenseBlocks+=Count>=11;
            }
        Check(TEXT("v59_menin_impacts_form_deterministic_dense_and_empty_patches_with_local_lookup_bounds"),
            V59Sites.Num()==ImpactColumnsV47*(ImpactMaxYV47-ImpactMinYV47+1)
            &&Occupied>200&&Occupied<800&&QuietBlocks>0&&DenseBlocks>0
            &&LargeRadius-SmallRadius>150.f&&Deep-Shallow>40.f&&LocalLookupBounded);
        const float ObliqueX=-.60f*Home;
        const float ObliqueAxis=MeninRoadAxisV59(ObliqueX);
        const float OldAxis=-150.f+.115f*ObliqueX+125.f*FMath::Sin(ObliqueX/1450.f+.42f);
        bool PhysicalDiagonal=FMath::Abs(ObliqueAxis-OldAxis)>300.f;
        for(int32 Step=-4;Step<=4;++Step)
        {
            const float Along=Step*.13f*Home,Axis=MeninRoadAxisV59(Along);
            const auto Crown=RegionalLandformV54(Along,Axis,7,TestStyle,1.f);
            const auto South=RegionalLandformV54(Along,Axis+970.f,7,TestStyle,1.f);
            const auto North=RegionalLandformV54(Along,Axis-970.f,7,TestStyle,1.f);
            PhysicalDiagonal&=Crown.ReliefCm>South.ReliefCm+65.f
                &&Crown.ReliefCm>North.ReliefCm+65.f
                &&South.WoodlandGully>.40f&&North.WoodlandGully>.40f;
        }
        Check(TEXT("v59_menin_oblique_road_has_two_physical_gullies"),PhysicalDiagonal);
        int32 DrainPockets=0,WetDrainPockets=0;bool DryRoad=true,InGully=true;
        for(const auto& B:BasinFieldV47().Basins)if(B.bMeninDrain)
        {
            ++DrainPockets;WetDrainPockets+=B.bHasWater?1:0;
            const auto Section=MeninRoadV59(B.Shape.X,B.Shape.Y,Home);
            InGully&=Section.Gully>.30f&&Section.Road<.05f;
            DryRoad&=BasinProtectionV47(B,
                {B.Shape.X,MeninRoadAxisV59(B.Shape.X)},false)<0.f;
        }
        Check(TEXT("v59_menin_shallow_gully_water_uses_real_beds_and_keeps_road_dry"),
            DrainPockets==UE_ARRAY_COUNT(MeninDrainSeedsV59)
            &&WetDrainPockets>=2&&InGully&&DryRoad);
        const float CentreAxis=MeninRoadAxisV59(0.f);
        Check(TEXT("v59_menin_scan_geometry_retains_only_bounded_wet_lip"),
            FMath::Abs(UnpooledHeightV42(0.f,CentreAxis,true,true)
                -UnpooledHeightV42(0.f,CentreAxis,true,false))<=3.001f);
        int32 ExposedRoad=0,DarkGully=0,PairedSections=0;
        for(int32 Step=-5;Step<=5;++Step)
        {
            const float SemanticX=Step*.16f*Home;
            const float Axis=MeninRoadAxisV59(SemanticX);
            const float V59RoadSample=MeninSurfaceSemanticV59(SemanticX,Axis,Home);
            const float V59GullySample=MeninSurfaceSemanticV59(SemanticX,Axis-970.f,Home);
            const bool bRoad=V59RoadSample>.80f,bGully=V59GullySample<.39f;
            ExposedRoad+=bRoad;DarkGully+=bGully;PairedSections+=bRoad&&bGully;
        }
        const float TrenchX=FrontlineBattlefield::TrenchX(1);
        const float TrenchY=MeninRoadAxisV59(TrenchX);
        const float SuppressedRoad=MeninSurfaceSemanticV59(TrenchX,TrenchY,Home);
        UE_LOG(LogTemp,Display,TEXT("WF_V59_SEMANTIC exposed_road=%d dark_gully=%d paired=%d trench=%.3f"),
            ExposedRoad,DarkGully,PairedSections,SuppressedRoad);
        Check(TEXT("v59_menin_material_semantic_tracks_oblique_physical_height"),
            ExposedRoad>=3&&DarkGully>=3&&PairedSections>=2&&FMath::Abs(SuppressedRoad-.5f)<.05f);
        GMeninV59Height.Store(false);
        const auto& RestoredV58=ImpactSitesV47()[300];
        Check(TEXT("v59_menin_impact_cache_does_not_replace_v58_fallback"),
            V58Impact.X==RestoredV58.X&&V58Impact.Y==RestoredV58.Y
            &&V58Impact.RX==RestoredV58.RX&&V58Impact.RY==RestoredV58.RY
            &&V58Impact.Depth==RestoredV58.Depth&&V58Impact.Wet==RestoredV58.Wet);
        GMeninV58Height.Store(false);
        const auto LegacyRoad=MeninRoadV57(X,RoadY,Home);
        const auto LegacyShape=RegionalLandformV54(X,RoadY,7,TestStyle,1.f);
        Check(TEXT("v57_menin_fallback_uses_the_original_height_function"),
            FMath::IsNearlyEqual(LegacyShape.ReliefCm,
                138.f*LegacyRoad.Crest+42.f*LegacyRoad.Road-102.f*LegacyRoad.Gully,.001f));
        GMeninV58Height.Store(PreviousMeninVersion);
        GMeninV59Height.Store(PreviousMaterialFirst);
    }
    {
        int32 TestOp=12;auto TestStyle=EFrontlineMissionTerrainV32::Marsh;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const bool PreviousLysVersion=GLysV58Height.Load();
        ON_SCOPE_EXIT{GLysV58Height.Store(PreviousLysVersion);};
        GLysV58Height.Store(true);
        const auto Deep=LysWashV57(0.f,0.f,Home);
        const auto Core=LysWashV57(Deep.Axis,0.f,Home);
        const auto Ford=LysWashV57(0.f,-720.f,Home);
        const auto Dry=LysWashV57(Ford.Axis,-720.f,Home);
        const auto WetLand=RegionalLandformV54(Core.Axis,0.f,12,TestStyle,1.f);
        const auto DryLand=RegionalLandformV54(Dry.Axis,-720.f,12,TestStyle,1.f);
        const auto Rear=LysWashV57(0.f,-5000.f,Home);
        const auto RearCentre=LysWashV57(Rear.Axis,-5000.f,Home);
        float RearLow=MAX_flt,RearHigh=-MAX_flt;
        for(float Delta=-350.f;Delta<=350.f;Delta+=50.f)
        {
            const float Relief=RegionalLandformV54(Rear.Axis+Delta,-5000.f,12,TestStyle,1.f).ReliefCm;
            RearLow=FMath::Min(RearLow,Relief);RearHigh=FMath::Max(RearHigh,Relief);
        }
        Check(TEXT("v58_lys_valley_crosses_the_advance_with_two_graded_dry_fords"),
            Core.Bed>.96f&&Core.Ford<.10f&&Dry.Ford>.98f
            &&WetLand.MarshChannel>DryLand.MarshChannel+.55f
            &&DryLand.ReliefCm>WetLand.ReliefCm+45.f);
        Check(TEXT("v58_lys_physical_river_reaches_the_distant_matting_edge"),
            RearCentre.Bed>.95f&&RearLow< -78.f&&RearHigh-RearLow>32.f);
        const auto& Basins=BasinFieldV47();
        int32 VisibleWater=0,DryBank=0,DepthMismatch=0;
        for(float Y=-4800.f;Y<=1000.f;Y+=300.f)
        {
            const auto Wash=LysWashV57(0.f,Y,Home);
            for(float Delta=-500.f;Delta<=500.f;Delta+=50.f)
            {
                const float X=Wash.Axis+Delta;
                const auto Water=RegionalWaterV54(X,Y,Basins);
                if(Water.Margin>=0.f)
                {
                    ++VisibleWater;
                    DepthMismatch+=FMath::Abs(Water.Depth-(Water.Level+1.2f-HeightAt(X,Y)))>.015f;
                }
                // The supplied slope requests the dry-side bank contour;
                // ordinary water queries above must never use that extension.
                const auto Shore=RegionalWaterV54(X,Y,Basins,.3f);
                DryBank+=Shore.Margin<0.f&&Shore.SignedShoreCm> -200.f&&Shore.SignedShoreCm< -5.f;
            }
        }
        Check(TEXT("v58_lys_braided_water_meets_the_ground_with_an_exposed_wet_mud_bank"),
            VisibleWater>25&&DryBank>6&&DepthMismatch==0);
        GLysV58Height.Store(false);
        const auto LegacyLand=RegionalLandformV54(Core.Axis,0.f,12,TestStyle,1.f);
        const float LegacyShape=-158.f*Core.Bed*(1.f-.79f*Core.Ford)+46.f*Core.Bank;
        const float LegacyMask=Core.Bed*(1.f-.72f*Core.Ford);
        Check(TEXT("v57_lys_fallback_restores_original_height_and_traversal_mask"),
            FMath::IsNearlyEqual(LegacyLand.ReliefCm,LegacyShape,.001f)
            &&FMath::IsNearlyEqual(LegacyLand.MarshChannel,LegacyMask,.001f));
        const FBasinFieldV47 NoImpactBasins;
        int32 LegacyWaterSamples=0,LegacyDepthErrors=0;
        for(float Y=-4500.f;Y<=900.f;Y+=900.f)
        {
            const auto Wash=LysWashV57(0.f,Y,Home);
            for(float Delta=-500.f;Delta<=500.f;Delta+=100.f)
            {
                const float X=Wash.Axis+Delta;
                const auto Water=RegionalWaterV54(X,Y,NoImpactBasins,.3f);
                if(Water.Margin<0.f)continue;
                ++LegacyWaterSamples;
                LegacyDepthErrors+=FMath::Abs(Water.Depth-(Water.Level-HeightAt(X,Y)))>.015f
                    ||Water.SignedShoreCm> -MAX_flt*.5f;
            }
        }
        Check(TEXT("v57_lys_fallback_restores_original_water_depth_and_footprint"),
            LegacyWaterSamples>0&&LegacyDepthErrors==0);
    }
    {
        int32 TestOp=13;auto TestStyle=EFrontlineMissionTerrainV32::Village;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const auto RoadY=[](float AtX){return 90.f+.085f*AtX+125.f*FMath::Sin(AtX/1590.f+.4f);};
        const float X=.40f*Home,Y=RoadY(X);
        const auto Road=RegionalLandformV54(X,Y,13,TestStyle,1.f);
        const auto Shoulder=RegionalLandformV54(X,Y-930.f,13,TestStyle,1.f);
        const float CellarX=-.50f*Home,CellarY=RoadY(CellarX)+1280.f;
        const auto Cellar=RegionalLandformV54(CellarX,CellarY,13,TestStyle,1.f);
        Check(TEXT("v57_villers_crossroads_is_a_firm_exposed_road_between_ruins_and_cellars"),
            Road.VillageRoad>.92f&&Road.Ruins<.20f
            &&Shoulder.Ruins>.40f&&Shoulder.ReliefCm>Road.ReliefCm+40.f
            &&Cellar.RuinCellar>.95f&&Cellar.VillageRoad<.15f);
    }
    {
        int32 TestOp=5;auto TestStyle=EFrontlineMissionTerrainV32::Marsh;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const auto Centre=YpresBraidV55(0.f,0.f,Home);
        const auto Core=YpresBraidV55(0.f,Centre.Axis,Home);
        const auto Middle=RegionalLandformV54(0.f,Centre.Axis,5,TestStyle,1.f);
        const auto EarlierSection=RegionalLandformV54(0.f,Centre.Axis,6,TestStyle,1.f);
        int32 WetSamples=0;float MaximumSlope=0;
        const auto& Basins=BasinFieldV47();
        for(float X=-1800.f;X<=1800.f;X+=180.f)
        {
            const auto Braid=YpresBraidV55(X,0.f,Home);
            WetSamples+=RegionalWaterV54(X,Braid.Axis,Basins).Margin>=0.f;
            for(float Y=Braid.Axis-650.f;Y<=Braid.Axis+650.f;Y+=260.f)
            {
                const float DX=(HeightAt(X+24.f,Y)-HeightAt(X-24.f,Y))/48.f;
                const float DY=(HeightAt(X,Y+24.f)-HeightAt(X,Y-24.f))/48.f;
                MaximumSlope=FMath::Max(MaximumSlope,FMath::Sqrt(DX*DX+DY*DY));
            }
        }
        Check(TEXT("V55 Ypres has a flooded middle braid with a matching physical trough"),
            Core.Bed>.95f&&Middle.MarshChannel>.8f
            &&Middle.ReliefCm+65.f<EarlierSection.ReliefCm&&WetSamples>=4&&MaximumSlope<1.8f);
    }
    {
        int32 TestOp=16;auto TestStyle=EFrontlineMissionTerrainV32::Canal;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const float CanalHome=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
        float LowestBank=FLT_MAX,HighestBed=-FLT_MAX,LowestMask=FLT_MAX;
        for(const float Y:{-4200.f,-1800.f,-1450.f,0.f})
        {
            const auto Section=StQuentinSectionV55(0.f,Y,CanalHome);
            const auto Bed=RegionalLandformV54(Section.Axis,Y,16,TestStyle,1.f);
            const auto Near=RegionalLandformV54(Section.Axis-1470.f,Y,16,TestStyle,1.f);
            const auto Far=RegionalLandformV54(Section.Axis+1510.f,Y,16,TestStyle,1.f);
            HighestBed=FMath::Max(HighestBed,Bed.ReliefCm);
            LowestBank=FMath::Min(LowestBank,FMath::Min(Near.ReliefCm,Far.ReliefCm));
            LowestMask=FMath::Min(LowestMask,Bed.CanalCut);
        }
        constexpr float NorthY=850.f,SouthY=-850.f;
        const auto GapA=StQuentinSectionV55(0.f,NorthY,CanalHome);
        const auto GapB=StQuentinSectionV55(0.f,SouthY,CanalHome);
        const auto Closed=StQuentinSectionV55(0.f,0.f,CanalHome);
        const float ClosedBed=RegionalLandformV54(Closed.Axis,0.f,16,TestStyle,1.f).ReliefCm;
        const auto GradedBed=[&](float Y)
        {
            const auto Section=StQuentinSectionV55(0.f,Y,CanalHome);
            return RegionalLandformV54(Section.Axis,Y,16,TestStyle,1.f).ReliefCm>ClosedBed+55.f;
        };
        Check(TEXT("v55_st_quentin_has_continuous_excavated_bed_opposing_banks_and_two_playable_graded_breaches"),
            HighestBed< -150.f&&LowestBank>35.f&&LowestMask>.9f
            &&FMath::Abs(NorthY)<FMath::Min(980.f,FrontlineBattlefield::HalfFrontage)
            &&FMath::Abs(SouthY)<FMath::Min(980.f,FrontlineBattlefield::HalfFrontage)
            &&GapA.Gap>.95f&&GapB.Gap>.95f&&Closed.Gap<.2f
            &&GradedBed(NorthY)&&GradedBed(SouthY));
    }
    {
        // Inspect the actual Selle operation layout. Testing with a different
        // mission's trench count can accidentally protect the wash beneath a
        // foreign trench and make its intended route appear absent.
        const float OldDepth=FrontlineBattlefield::OperationDepthScale;
        const int32 OldOperation=FrontlineBattlefield::OperationIndex;
        ON_SCOPE_EXIT{FrontlineBattlefield::ConfigureOperationDepth(OldDepth,OldOperation);};
        FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(17),17);
        int32 TestOp=17;auto TestStyle=EFrontlineMissionTerrainV32::RailCutting;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        const float SelleHome=FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
        const auto WashAxis=[&](float Y)
        {return -.18f*SelleHome+.22f*Y+145.f*FMath::Sin(Y/770.f+1.7f);};
        const auto Wet=QueryTerrainFeaturesV54(WashAxis(0.f),0.f);
        const auto Dry=QueryTerrainFeaturesV54(WashAxis(410.f),410.f);
        const auto RailBed=QueryTerrainFeaturesV54(WashAxis(-1130.f),-1130.f);
        float MaximumSlope=0.f;
        for(float Y=-800.f;Y<=950.f;Y+=250.f)
            for(float X=WashAxis(Y)-900.f;X<=WashAxis(Y)+900.f;X+=180.f)
            {
                const float DX=(HeightAt(X+24.f,Y)-HeightAt(X-24.f,Y))/48.f;
                const float DY=(HeightAt(X,Y+24.f)-HeightAt(X,Y-24.f))/48.f;
                MaximumSlope=FMath::Max(MaximumSlope,FMath::Sqrt(DX*DX+DY*DY));
            }
        Check(TEXT("v56_selle_has_physical_muddy_wash_and_raised_dry_crossing_outside_running_rail"),
            Wet.RailWash>.65f&&Wet.ReliefCm< -35.f
            &&Dry.RailWash<.15f&&Dry.ReliefCm>Wet.ReliefCm+48.f
            &&RailBed.RailWash<.005f&&MaximumSlope<1.8f);
    }
    float BullecourtCentre=0,ScarpeCentre=0,CambraiCentre=0,AmiensCentre=0;
    for(const int32 FarmOp:{2,8,10,11,14,15})
    {
        int32 TestOp=FarmOp;auto TestStyle=EFrontlineMissionTerrainV32::Farmland;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        float Ditch=0,Boundary=0,Churn=0,Negative=0,Positive=0,MaximumSlope=0;
        double SumY=0,SumMask=0;
        for(float Y=-1450.f;Y<=1450.f;Y+=160.f)for(float X=-Home;X<=Home;X+=160.f)
        {
            const auto F=QueryTerrainFeaturesV54(X,Y);
            Ditch=FMath::Max(Ditch,F.FieldDitch);
            Boundary=FMath::Max(Boundary,F.FieldBoundary);
            Churn=FMath::Max(Churn,F.TankChurn);
            Negative=FMath::Min(Negative,F.ReliefCm);
            Positive=FMath::Max(Positive,F.ReliefCm);
            const float Low=FMath::Max(F.FieldDitch,F.TankChurn);
            SumMask+=Low;SumY+=double(Y)*Low;
            if(int32(X+Home)%640==0&&int32(Y+1450.f)%640==0)
            {
                const float DX=(HeightAt(X+24.f,Y)-HeightAt(X-24.f,Y))/48.f;
                const float DY=(HeightAt(X,Y+24.f)-HeightAt(X,Y-24.f))/48.f;
                MaximumSlope=FMath::Max(MaximumSlope,FMath::Sqrt(DX*DX+DY*DY));
            }
        }
        const float Centre=SumMask>1?float(SumY/SumMask):0.f;
        if(FarmOp==2)BullecourtCentre=Centre;
        if(FarmOp==15)ScarpeCentre=Centre;
        if(FarmOp==8)CambraiCentre=Centre;
        if(FarmOp==14)AmiensCentre=Centre;
        const bool Vehicle=FarmOp==8||FarmOp==14;
        Check(FString::Printf(TEXT("V54 farmland operation %d has displaced, traversable distinct landform"),FarmOp),
            (Vehicle?Churn>.22f&&Ditch<.01f:Ditch>.22f&&Churn<.01f)
            &&Boundary>.10f&&Negative< -12.f&&Positive>7.f&&MaximumSlope<1.8f);
        UE_LOG(LogTemp,Display,TEXT("WF_V54_FARM operation=%d ditch=%.3f boundary=%.3f churn=%.3f low_cm=%.1f high_cm=%.1f centre_y=%.1f slope=%.3f"),
            FarmOp,Ditch,Boundary,Churn,Negative,Positive,Centre,MaximumSlope);
    }
    Check(TEXT("V54 repeated farmland missions shift the physical large-form approach"),
        FMath::Abs(BullecourtCentre-ScarpeCentre)>160.f&&FMath::Abs(CambraiCentre-AmiensCentre)>130.f);
    for(const int32 LandmarkOp:{0,5,8,16})
    {
        int32 TestOp=LandmarkOp;
        auto TestStyle=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British)[LandmarkOp].TerrainStyleV32;
        FrontlineFieldV32::TerrainProfileV43(TestOp,TestStyle,true);
        int32 Samples=0,Visible=0,Core=0;float Peak=0;
        for(float Y=-1400.f;Y<=1100.f;Y+=170.f)for(float X=-Home;X<=Home;X+=170.f)
        {
            const auto F=QueryTerrainFeaturesV54(X,Y);
            const float Quiet=StructuralCoreV54(F);
            ++Samples;Core+=Quiet>.35f;
            Visible+=Quiet>.35f&&FMath::Abs(F.ReliefCm)>45.f;
            Peak=FMath::Max(Peak,FMath::Abs(F.ReliefCm));
        }
        const float Coverage=Samples>0?float(Visible)/Samples:0.f;
        UE_LOG(LogTemp,Display,TEXT("WF_V54_READABLE operation=%d strong_footprint=%.3f quiet_footprint=%.3f peak_cm=%.1f"),
            LandmarkOp,Coverage,Samples>0?float(Core)/Samples:0.f,Peak);
        Check(FString::Printf(TEXT("V54 operation %d has a broad visible main landform at gameplay scale"),LandmarkOp),
            Coverage>.055f&&Peak>75.f);
    }
    int32 WinterOp=10;auto WinterStyle=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British)[10].TerrainStyleV32;
    FrontlineFieldV32::TerrainProfileV43(WinterOp,WinterStyle,true);
    float Snow=0;
    for(float Y=-1200.f;Y<=850.f;Y+=180.f)for(float X=-Home;X<=Home;X+=180.f)
        Snow=FMath::Max(Snow,QueryTerrainFeaturesV54(X,Y).SnowDrift);
    Check(TEXT("V54 winter operation has physical drift tongues"),Snow>.30f);
    int32 ProtectedOp=SavedOp;auto ProtectedStyle=SavedStyle;FrontlineFieldV32::TerrainProfileV43(ProtectedOp,ProtectedStyle,true);
    float TrenchRelief=0;
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)for(float Y=-600.f;Y<=600.f;Y+=200.f)
        for(const float DX:{-280.f,0.f,280.f})
            TrenchRelief=FMath::Max(TrenchRelief,FMath::Abs(QueryTerrainFeaturesV54(FrontlineFieldV32::TrenchCentreX(T,Y)+DX,Y).ReliefCm));
    Check(TEXT("V54 existing firing steps and revetments keep exact footing"),TrenchRelief<.002f);
    const auto Wet=QueryTerrainFeaturesV54(FieldCraters[0].X,FieldCraters[0].Y);
    Check(TEXT("V54 authored wet crater bed excludes regional landform displacement"),FMath::Abs(Wet.ReliefCm)<.002f);
    const auto At=QueryTerrainFeaturesV54(.32f*Home,800.f),Again=QueryTerrainFeaturesV54(.32f*Home,800.f);
    Check(TEXT("V54 feature query remains deterministic"),FMath::IsNearlyEqual(At.ReliefCm,Again.ReliefCm,.0001f));
}

FTransform AWarfareFrontlineGround::TrackSupportAtV35(const FVector& Position,const FRotator& Previous,float Yaw,float Rear,float Front,float HalfWidth,float SoleHeight,float Seconds)
{
    const FQuat Flat=FRotator(0,Yaw,0).Quaternion();
    float ForwardHeight=0,RearHeight=0,LeftHeight=0,RightHeight=0;
    for(int32 I=0;I<3;++I)
    {
        const float Y=(I-1)*HalfWidth;
        const FVector F=Position+Flat.RotateVector(FVector(Front,Y,0));
        const FVector R=Position+Flat.RotateVector(FVector(-Rear,Y,0));
        ForwardHeight+=HeightAt(F.X,F.Y);RearHeight+=HeightAt(R.X,R.Y);
        const float X=FMath::Lerp(-Rear,Front,I*.5f);
        const FVector L=Position+Flat.RotateVector(FVector(X,-HalfWidth,0));
        const FVector T=Position+Flat.RotateVector(FVector(X,HalfWidth,0));
        LeftHeight+=HeightAt(L.X,L.Y);RightHeight+=HeightAt(T.X,T.Y);
    }
    // UE positive pitch raises local +X. The previous negated pitch drove the
    // leading end into a rising bank. Roll uses the inverse local-Y convention.
    const float Pitch=FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2((ForwardHeight-RearHeight)/3.f,Rear+Front)),-12.f,12.f);
    const float Roll=FMath::Clamp(-FMath::RadiansToDegrees(FMath::Atan2((RightHeight-LeftHeight)/3.f,HalfWidth*2.f)),-8.f,8.f);
    const FRotator Target(Pitch,Yaw,Roll);
    FRotator Supported=Seconds>0?FMath::RInterpTo(Previous,Target,Seconds,5.f):Target;
    // Route steering has already bounded yaw with FixedTurn. Damping it a
    // second time leaves the rendered hull behind its collision/weapon frame.
    // Bank pitch/roll retain damping; solve clearance using this exact yaw.
    Supported.Yaw=Yaw;
    const FQuat Rotation=Supported.Quaternion();
    float Z=-MAX_flt;
    // A rigid tracked hull bridges a trench on its supporting banks. Fit the
    // complete underside envelope, not the average height of two center points.
    // Clearance is solved AFTER smoothing orientation, so damping cannot bury it.
    for(int32 Row=0;Row<3;++Row)for(int32 I=0;I<9;++I)
    {
        const FVector Local(FMath::Lerp(-Rear,Front,I/8.f),(Row-1)*HalfWidth,SoleHeight);
        const FVector Offset=Rotation.RotateVector(Local);
        const float H=HeightAt(Position.X+Offset.X,Position.Y+Offset.Y);
        Z=FMath::Max(Z,H-Offset.Z+2.f);
    }
    return FTransform(Rotation,FVector(Position.X,Position.Y,Z),FVector::OneVector);
}

void AWarfareFrontlineGround::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildTerrain();
}

void AWarfareFrontlineGround::BeginPlay()
{
    Super::BeginPlay();
    // The generated mesh is transient; reconstruct when a saved level starts.
    RebuildTerrain();
}

static FMeshDescription BuildTerrainDescriptionV43(float& Density)
{
    auto& BasinField=BasinFieldV47();
    int32 ShoreOperation=0;EFrontlineMissionTerrainV32 ShoreStyle;
    FrontlineFieldV32::TerrainProfileV43(ShoreOperation,ShoreStyle);
    const float MeninHome=ShoreOperation==7
        ?FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)):0.f;
    const bool bRegionalShoreV56=((ShoreOperation==5||ShoreOperation==12)&&ShoreStyle==EFrontlineMissionTerrainV32::Marsh)
        ||(ShoreOperation==16&&ShoreStyle==EFrontlineMissionTerrainV32::Canal)
        ||(ShoreOperation==7&&GMeninV63Water.Load());
    BasinField.SurfaceVertices=BasinField.ShoreVertices=BasinField.ExposedShoreVertices=0;BasinField.MaximumShore=0;
    BasinField.ProtectedSamples=0;BasinField.ProtectedMaximumChange=0;
    BasinField.FrontageSamples=BasinField.FrontageChangedSamples=0;
    BasinField.FrontageMaximumDelta=BasinField.FrontageMaximumGradient=0;
    BasinField.FrontageOpenSamples=0;BasinField.FrontageIncludingBanksGradient=BasinField.FrontageWorstBankDistance=0;
    BasinField.ScanSamples=BasinField.ScanChangedSamples=0;BasinField.ScanMaximumChange=BasinField.ScanSquareSum=0;
    BasinField.RefinedHeightOnly=BasinField.RefinedShoreOnly=BasinField.RefinedHeightAndShore=0;
    BasinField.MaximumCentreError=0;
    BasinField.FineScanCells=BasinField.FineScanCandidates=BasinField.CalmCoreSamples=0;
    BasinField.MaximumQuarterError=BasinField.CalmCoreMaximumChange=0;
    auto CheckContact=[&](float X,float Y)
    {
        const float Before=UnpooledHeightV42(X,Y,false),After=AWarfareFrontlineGround::HeightAt(X,Y);
        ++BasinField.ProtectedSamples;
        const float Delta=FMath::IsFinite(Before)&&FMath::IsFinite(After)?FMath::Abs(Before-After):MAX_flt;
        if(Delta>BasinField.ProtectedMaximumChange+.05f)
            UE_LOG(LogTemp,Warning,TEXT("WF_V54_PROTECTED_CONTACT x=%.1f y=%.1f before=%.3f after=%.3f delta=%.3f"),X,Y,Before,After,Delta);
        BasinField.ProtectedMaximumChange=FMath::Max(BasinField.ProtectedMaximumChange,Delta);
    };
    auto FrontageDelta=[](float X,float Y){return AWarfareFrontlineGround::HeightAt(X,Y)-UnpooledHeightV42(X,Y,false);};
    for(float X=-8000;X<=8000;X+=500)for(float Y=-720;Y<=720;Y+=360)
    {
        const float Delta=FrontageDelta(X,Y);
        // A difference between two heightfields is not a physical slope. Test
        // the new surface over a boot-sized contact. Existing trench walls
        // are not walking terrain; their exact supports are checked below.
        const float DX=(AWarfareFrontlineGround::HeightAt(X+12.f,Y)-AWarfareFrontlineGround::HeightAt(X-12.f,Y))/24.f;
        const float DY=(AWarfareFrontlineGround::HeightAt(X,Y+12.f)-AWarfareFrontlineGround::HeightAt(X,Y-12.f))/24.f;
        ++BasinField.FrontageSamples;BasinField.FrontageChangedSamples+=FMath::Abs(Delta)>.001f;
        if(FMath::Abs(Delta)>219.5f)
            UE_LOG(LogTemp,Warning,TEXT("WF_V54_FRONTAGE_DELTA x=%.1f y=%.1f delta=%.3f"),X,Y,Delta);
        BasinField.FrontageMaximumDelta=FMath::Max(BasinField.FrontageMaximumDelta,FMath::IsFinite(Delta)?FMath::Abs(Delta):MAX_flt);
        const float Gradient=FMath::IsFinite(DX)&&FMath::IsFinite(DY)?FMath::Sqrt(DX*DX+DY*DY):MAX_flt;
        float Bank=MAX_flt;for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
            Bank=FMath::Min(Bank,FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y)));
        if(Gradient>BasinField.FrontageIncludingBanksGradient){BasinField.FrontageIncludingBanksGradient=Gradient;BasinField.FrontageWorstBankDistance=Bank;}
        if(Bank>=500.f){++BasinField.FrontageOpenSamples;BasinField.FrontageMaximumGradient=FMath::Max(BasinField.FrontageMaximumGradient,Gradient);}
    }
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)for(int32 Row=0;Row<18;++Row)
    {
        const float Y=FMath::Lerp(-1740.f,1740.f,Row/17.f);
        for(float DX:{-285.f,-140.f,0.f,140.f,285.f})CheckContact(FrontlineFieldV32::TrenchCentreX(T,Y)+DX,Y);
    }
    if(BasinField.bRail)for(float X=-8000;X<=8000;X+=500)for(float DY:{-200.f,0.f,200.f})CheckContact(X,-1130.f+DY);
    for(int32 IX=0;IX<33;++IX)for(int32 IY=0;IY<31;++IY)
    {
        const float X=-6000.f+IX*375.f,Y=-3300.f+IY*235.f;
        const float Delta=AWarfareFrontlineGround::HeightAt(X,Y)-UnpooledHeightV42(X,Y,true,false);
        ++BasinField.ScanSamples;BasinField.ScanChangedSamples+=FMath::Abs(Delta)>.01f;
        BasinField.ScanMaximumChange=FMath::Max(BasinField.ScanMaximumChange,FMath::Abs(Delta));
        BasinField.ScanSquareSum+=Delta*Delta;
    }
    auto CheckCalm=[&](const FFieldCrater& C)
    {
        if(!C.Wet)return;
        for(const FVector2f D:{FVector2f::ZeroVector,FVector2f(C.RX*.12f,0),FVector2f(-C.RX*.12f,0),FVector2f(0,C.RY*.12f),FVector2f(0,-C.RY*.12f)})
        {
            const float X=C.X+D.X,Y=C.Y+D.Y;
            const float Delta=AWarfareFrontlineGround::HeightAt(X,Y)-UnpooledHeightV42(X,Y,true,false);
            ++BasinField.CalmCoreSamples;
            BasinField.CalmCoreMaximumChange=FMath::Max(BasinField.CalmCoreMaximumChange,FMath::Abs(Delta));
        }
    };
    for(const auto& C:FieldCraters)CheckCalm(C);for(const auto& C:DrainageHollowsV47)CheckCalm(C);
    for(const auto& C:ImpactSitesV47())if(FMath::Abs(C.Y)>1100.f)CheckCalm(C);
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    auto Positions = Attributes.GetVertexPositions();
    auto Normals = Attributes.GetVertexInstanceNormals();
    auto Tangents = Attributes.GetVertexInstanceTangents();
    auto Signs = Attributes.GetVertexInstanceBinormalSigns();
    auto UVs = Attributes.GetVertexInstanceUVs();
    auto Colors = Attributes.GetVertexInstanceColors();
    UVs.SetNumChannels(1);
    const FPolygonGroupID Group = Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Ground");
    // A V56 physical middle distance continues behind the photographic matte
    // for the four newly painted operations. It is the same HeightAt surface
    // and material as the battlefield, so a soft matte edge can reveal actual
    // lit ground instead of empty sky. Other operations keep their accepted
    // raster budget and all playable row coordinates remain unchanged.
    const int32 RearRowsV56=(ShoreOperation==3||ShoreOperation==4||ShoreOperation==5||ShoreOperation==7||ShoreOperation==12||ShoreOperation==13
        ||ShoreOperation==16||ShoreOperation==17)?104:48;
    constexpr int32 Width=int32(FrontlineBattlefield::GroundHalfLength*2.f/50.f)+1;
    const int32 Depth=351+RearRowsV56;
    // Keep clods at soil scale; five-metre tiles read as blurred rock at the
    // normal side camera. Vertex colour supplies the separate broad wet patches.
    const float WorldUnitsPerUV = FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"))?180.f:145.f;
    double WeightedUVDensity=0, UVDensityWeight=0;
    int32 LevelPoolCoreVertices=0;
    float LevelPoolMaximumCoreSlope=0;
    TArray<FVertexID> Vertices;
    TArray<FVector3f> VertexNormals,VertexTangents;
    TArray<FVector4f> VertexColors;
    TArray<float> BasinOnlyShore;
    Vertices.Reserve(Width*Depth);VertexNormals.Reserve(Width*Depth);
    VertexTangents.Reserve(Width*Depth);VertexColors.Reserve(Width*Depth);
    BasinOnlyShore.Reserve(Width*Depth);
    auto AddVertex=[&](float PX,float PY)
        {
            const int32 Index=Vertices.Num();
            const FVertexID V=Description.CreateVertex();
            Positions[V]=FVector3f(PX,PY,AWarfareFrontlineGround::HeightAt(PX,PY));
            Vertices.Add(V);
            // Compute once per shared vertex, not again for six triangle corners.
            const float DX=(AWarfareFrontlineGround::HeightAt(PX+1,PY)-AWarfareFrontlineGround::HeightAt(PX-1,PY))*.5f;
            const float DY=(AWarfareFrontlineGround::HeightAt(PX,PY+1)-AWarfareFrontlineGround::HeightAt(PX,PY-1))*.5f;
            VertexNormals.Add(FVector3f(-DX,-DY,1).GetSafeNormal());
            VertexTangents.Add(FVector3f(1,0,DX).GetSafeNormal());
            const float Broad=FMath::PerlinNoise2D(FVector2D(PX*.00052f+19.17f,PY*.00067f));
            const float Fine=FMath::PerlinNoise2D(FVector2D(PX*.0027f,PY*.0021f+7.2f));
            // Separate broken wet strata across the middle slopes from the
            // playable mud. This uses the existing physical wet material,
            // rather than adding detached dark planes or a new shader layer.
            const float MiddleWetBand=Ease((-PY-1700.f)/450.f)
                *FMath::Exp(-FMath::Square((PY+2720.f+145.f*FMath::Sin(PX/620.f))/570.f))
                *(.65f+.35f*FMath::PerlinNoise2D(FVector2D(PX*.0022f+4.7f,PY*.0017f)));
            float LipMoisture=0;
            for(int32 TrenchIndex=0;TrenchIndex<FrontlineBattlefield::TrenchCount;++TrenchIndex)
            {
                const float TX=FrontlineFieldV32::TrenchCentreX(TrenchIndex,PY);
                const float Bend=0;
                const float LipDistance=FMath::Abs(PX-TX-Bend);
                // Dark irregular moisture follows the actual trench shoulders;
                // it does not change the firing-step height or prop supports.
                LipMoisture=FMath::Max(LipMoisture,FMath::Exp(-FMath::Square((LipDistance-290.f)/120.f))
                    *(.20f+.075f*FMath::Sin(PY/510.f+TX/870.f)));
            }
            bool ExposedShore=false;
            const float GroundSlope=FMath::Sqrt(DX*DX+DY*DY);
            float Shore=BasinShoreV47(BasinField,PX,PY,Positions[V].Z,GroundSlope,&ExposedShore);
            BasinOnlyShore.Add(Shore);
            if(bRegionalShoreV56&&Shore<=0.f)
            {
                // The same analytic boundary that clips the regional water
                // now stains its exposed bank. Keep the old crater-pool shore
                // untouched, with no detached plane or fabricated water.
                const auto Regional=RegionalWaterV54(PX,PY,BasinField,GroundSlope);
                if(Regional.SignedShoreCm> -240.f)
                {
                    const float DryReach=Ease((Regional.SignedShoreCm+240.f)/240.f);
                    const float InnerFade=1.f-Ease((Regional.SignedShoreCm-45.f)/155.f);
                    Shore=FMath::Max(Shore,DryReach*InnerFade*(.82f+.12f*Fine));
                }
            }
            const float Lowland=Ease((35.f-Positions[V].Z)/85.f);
            const float Wet=FMath::Clamp(.24f+Broad*.58f+Fine*.09f+Lowland*.16f+MiddleWetBand*.14f+LipMoisture*.35f+Shore*.34f,.04f,.94f);
            const float Secondary=ShoreOperation==7
                ?(GMeninV59Height.Load()?MeninSurfaceSemanticV59(PX,PY,MeninHome)
                    :MeninSurfaceSemanticV58(PX,PY,MeninHome))
                :FMath::Clamp(.45f+FMath::PerlinNoise2D(FVector2D(PX*.0014f+37.1f,PY*.0011f))*.85f,0.f,1.f);
            float LevelWater=0;
            const float Water=CraterWetness(PX,PY,&LevelWater);
            if(LevelWater>.9999f)
            {
                ++LevelPoolCoreVertices;
                LevelPoolMaximumCoreSlope=FMath::Max(LevelPoolMaximumCoreSlope,FMath::Max(FMath::Abs(DX),FMath::Abs(DY)));
            }
            // G is the actual resolved basin's mud margin. Standing water is
            // rendered only by the separate clipped mesh (LegacyWaterMask=0).
            VertexColors.Add(FVector4f(Wet,Shore,Secondary,Water));
            ++BasinField.SurfaceVertices;BasinField.ShoreVertices+=Shore>.05f;
            BasinField.ExposedShoreVertices+=ExposedShore;BasinField.MaximumShore=FMath::Max(BasinField.MaximumShore,Shore);
            return Index;
        };
    // The rear reaches -7008 cm for the photographed operations versus -5146
    // elsewhere. The near edge remains 8087.5 cm in every profile.
    for (int32 Y=0; Y<Depth; ++Y)
        for (int32 X=0; X<Width; ++X)AddVertex((X-Width/2)*50.f,-3550.f+(Y-RearRowsV56)*33.25f);
    int32 TriangleCount=0,RefinedCells=0;
    auto AddTriangle = [&](int32 A, int32 B, int32 C)
    {
        const FVector3f Edge1=Positions[Vertices[B]]-Positions[Vertices[A]];
        const FVector3f Edge2=Positions[Vertices[C]]-Positions[Vertices[A]];
        const double SurfaceArea=FVector3f::CrossProduct(Edge1,Edge2).Size();
        const double UVArea=FMath::Abs(double(Edge1.X)*Edge2.Y-double(Edge1.Y)*Edge2.X)
            /FMath::Square(double(WorldUnitsPerUV));
        if (SurfaceArea>UE_SMALL_NUMBER && UVArea>UE_SMALL_NUMBER)
        {
            // Match UE's world-units-per-UV definition, including this triangle's slope.
            // All generated triangles contribute; there are no unrelated atlas islands.
            const double Weight=FMath::Sqrt(SurfaceArea);
            WeightedUVDensity+=FMath::Sqrt(SurfaceArea/UVArea)*Weight;
            UVDensityWeight+=Weight;
        }
        TArray<FVertexInstanceID> Corners;
        for (int32 Index : { A,B,C })
        {
            const FVertexInstanceID VI=Description.CreateVertexInstance(Vertices[Index]);
            const FVector3f P=Positions[Vertices[Index]];
            Normals[VI]=VertexNormals[Index];
            Tangents[VI]=VertexTangents[Index]; Signs[VI]=1;
            UVs.Set(VI,0,FVector2f(P.X/WorldUnitsPerUV,P.Y/WorldUnitsPerUV));
            Colors[VI]=VertexColors[Index]; Corners.Add(VI);
        }
        Description.CreateTriangle(Group,Corners);
        ++TriangleCount;
    };
    // Spend geometry where linear interpolation visibly loses a crater shore
    // or excavated contour. Flat distant ground retains its old 50 x 33.25 cm
    // grid. Shared edge splits also stitch adjacent coarse cells (no T-junctions).
    TArray<uint8> Refine;Refine.Init(0,(Width-1)*(Depth-1));
    // Detail follows each operation's real rear boundaries, including the long
    // final battlefields. The fixed budget is assigned by geometric error.
    BasinField.ScanRefinementHalfExtent=FMath::Min(FrontlineBattlefield::GroundHalfLength-50.f,FrontlineBattlefield::GoalHalfLength+1700.f);
    struct FScanRefineCandidate {int32 Cell;float Error;};
    TArray<FScanRefineCandidate> ScanCandidates;
    for (int32 Y=0; Y<Depth-1; ++Y)
        for (int32 X=0; X<Width-1; ++X)
        {
            const int32 I=Y*Width+X;
            const FVector3f A=Positions[Vertices[I]],C=Positions[Vertices[I+Width+1]];
            const float PX=(A.X+C.X)*.5f,PY=(A.Y+C.Y)*.5f;
            const float LinearHeight=(Positions[Vertices[I+Width]].Z+Positions[Vertices[I+1]].Z)*.5f;
            const float RealHeight=AWarfareFrontlineGround::HeightAt(PX,PY);
            const float Error=FMath::Abs(RealHeight-LinearHeight);
            BasinField.MaximumCentreError=FMath::Max(BasinField.MaximumCentreError,Error);
            const auto Normal=(VertexNormals[I+Width]+VertexNormals[I+1]).GetSafeNormal();
            const float Slope=FMath::Sqrt(Normal.X*Normal.X+Normal.Y*Normal.Y)/FMath::Max(.08f,Normal.Z);
            // Regional water changes the material's wet margin, not the old
            // crater-basin contour that drives adaptive geometry refinement.
            const float ShoreError=FMath::Abs(BasinShoreV47(BasinField,PX,PY,RealHeight,Slope)
                -(BasinOnlyShore[I+Width]+BasinOnlyShore[I+1])*.5f);
            const bool HeightNeeded=Error>.90f,ShoreNeeded=ShoreError>.035f;
            if(HeightNeeded||ShoreNeeded)
            {
                Refine[Y*(Width-1)+X]=1;++RefinedCells;
                BasinField.RefinedHeightOnly+=HeightNeeded&&!ShoreNeeded;
                BasinField.RefinedShoreOnly+=ShoreNeeded&&!HeightNeeded;
                BasinField.RefinedHeightAndShore+=HeightNeeded&&ShoreNeeded;
            }
            if(Refine[Y*(Width-1)+X]&&FMath::Abs(PX)<BasinField.ScanRefinementHalfExtent&&PY>-3400.f&&PY<4100.f)
            {
                const float ScanChange=FMath::Abs(RealHeight-UnpooledHeightV42(PX,PY,true,false));
                if(ScanChange>1.25f)
                {
                    float MissingError=0;
                    for(const int32 Corner:{I,I+1,I+Width,I+Width+1})
                    {
                        const auto& V=Positions[Vertices[Corner]];
                        const float Actual=AWarfareFrontlineGround::HeightAt((V.X+PX)*.5f,(V.Y+PY)*.5f);
                        MissingError=FMath::Max(MissingError,FMath::Abs(Actual-(V.Z+RealHeight)*.5f));
                    }
                    BasinField.MaximumQuarterError=FMath::Max(BasinField.MaximumQuarterError,MissingError);
                    if(MissingError>.80f)ScanCandidates.Add({Y*(Width-1)+X,MissingError});
                }
            }
        }
    ScanCandidates.Sort([](const FScanRefineCandidate& A,const FScanRefineCandidate& B){return A.Error==B.Error?A.Cell<B.Cell:A.Error>B.Error;});
    BasinField.FineScanCandidates=ScanCandidates.Num();BasinField.FineScanCells=FMath::Min(8000,ScanCandidates.Num());
    for(int32 I=0;I<BasinField.FineScanCells;++I)Refine[ScanCandidates[I].Cell]=2;
    auto Level=[&](int32 X,int32 Y){return X>=0&&X<Width-1&&Y>=0&&Y<Depth-1?int32(Refine[Y*(Width-1)+X]):0;};
    TMap<uint64,int32> GridVertices;
    auto GridVertex=[&](int32 GX,int32 GY)
    {
        if(GX%4==0&&GY%4==0)return GY/4*Width+GX/4;
        const uint64 Key=(uint64(GX)<<32)|uint32(GY);
        if(const int32* Found=GridVertices.Find(Key))return *Found;
        const int32 N=AddVertex((GX-(Width/2)*4)*12.5f,-3550.f+(GY-RearRowsV56*4)*8.3125f);
        GridVertices.Add(Key,N);return N;
    };
    auto Fan=[&](int32 GX,int32 GY,int32 Span,int32 Left,int32 Bottom,int32 Right,int32 Top)
    {
        TArray<int32,TInlineAllocator<16>> Rim;
        for(int32 I=0;I<Left;++I)Rim.Add(GridVertex(GX,GY+Span*I/Left));
        for(int32 I=0;I<Bottom;++I)Rim.Add(GridVertex(GX+Span*I/Bottom,GY+Span));
        for(int32 I=0;I<Right;++I)Rim.Add(GridVertex(GX+Span,GY+Span-Span*I/Right));
        for(int32 I=0;I<Top;++I)Rim.Add(GridVertex(GX+Span-Span*I/Top,GY));
        const int32 Centre=GridVertex(GX+Span/2,GY+Span/2);
        for(int32 Edge=0;Edge<Rim.Num();++Edge)AddTriangle(Centre,Rim[Edge],Rim[(Edge+1)%Rim.Num()]);
    };
    for(int32 Y=0;Y<Depth-1;++Y)for(int32 X=0;X<Width-1;++X)
    {
        const int32 I=Y*Width+X,Fine=Level(X,Y);
        if(Fine==2)
        {for(int32 SY=0;SY<2;++SY)for(int32 SX=0;SX<2;++SX)Fan(X*4+SX*2,Y*4+SY*2,2,2,2,2,2);continue;}
        const int32 Left=1<<FMath::Max(Fine,Level(X-1,Y)),Bottom=1<<FMath::Max(Fine,Level(X,Y+1));
        const int32 Right=1<<FMath::Max(Fine,Level(X+1,Y)),Top=1<<FMath::Max(Fine,Level(X,Y-1));
        if(Left==1&&Bottom==1&&Right==1&&Top==1){AddTriangle(I,I+Width,I+1);AddTriangle(I+1,I+Width,I+Width+1);continue;}
        Fan(X*4,Y*4,4,Left,Bottom,Right,Top);
    }
    BasinField.RefinedCells=RefinedCells;BasinField.BaseCells=(Width-1)*(Depth-1);
    UE_LOG(LogTemp,Display,TEXT("WF_V47_GROUND_MESH vertices=%d triangles=%d refined_cells=%d base_triangles=%d refinement_trigger_cm=0.90 clods_exclude_trench_radius_cm=305"),Vertices.Num(),TriangleCount,RefinedCells,BasinField.BaseCells*2);
    UE_LOG(LogTemp,Display,TEXT("WF_V42_LEVEL_POOLS candidates=7 actual_core_vertices=%d maximum_core_slope=%f protected_frontage_cm=720 protected_bank_cm=560"),LevelPoolCoreVertices,LevelPoolMaximumCoreSlope);
    Density=UVDensityWeight>UE_SMALL_NUMBER ? float(WeightedUVDensity/UVDensityWeight) : WorldUnitsPerUV;
    return Description;
}

namespace
{
FString TerrainPackage43(int32 Op,EFrontlineMissionTerrainV32 Style,float Scale,int32 Tile,int32 VersionOverride=-1);
FString WaterPackage44(int32 Op,EFrontlineMissionTerrainV32 Style,float Scale,int32 VersionOverride);
bool HasVersionedWater44(int32 Op,EFrontlineMissionTerrainV32 Style,float Scale,int32 Version)
{
    const FString Path=WaterPackage44(Op,Style,Scale,Version);
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+FPaths::GetBaseFilename(Path)));
    const auto* Render=Mesh?Mesh->GetRenderData():nullptr;
    return Render&&!Render->LODResources.IsEmpty()&&Render->LODResources[0].GetNumTriangles()>0;
}
bool LoadTerrainTiles43(int32 Op,EFrontlineMissionTerrainV32 Style,float Scale,TArray<UStaticMesh*>& Assets,int32* VersionUsed=nullptr)
{
    // Select an entire coherent visual/collision/material generation. Menin
    // V59 may fall back to V58, then V57; no mixed per-tile heightfields.
    // A changed Menin or Lys heightfield requires its paired water mesh.
    TArray<int32,TInlineAllocator<4>> Versions;
    if(Op==7&&FParse::Param(FCommandLine::Get(),TEXT("WFV63MeninWater")))Versions.Add(63);
    Versions.Add(Op==7?59:Op==12?58:Op==13?57:54);
    if(Op==7)Versions.Add(58);
    if(Op==7||Op==12)Versions.Add(57);
    for(const int32 Version:Versions)
    {
        if(Op==7&&Version==59&&!LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Warfare/Field/V59/Materials/MI_GroundMud_V59.MI_GroundMud_V59")))continue;
        Assets.Reset();
        for(int32 Tile=0;Tile<20;++Tile)
        {
            const FString Path=TerrainPackage43(Op,Style,Scale,Tile,Version);
            auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+FPaths::GetBaseFilename(Path)));
            const auto* Render=Mesh?Mesh->GetRenderData():nullptr;
            if(!Render||Render->LODResources.IsEmpty()||Render->LODResources[0].GetNumTriangles()==0)break;
            Assets.Add(Mesh);
        }
        if(Assets.Num()==20&&((Op==12)||(Op==7&&(Version==59||Version==63)))
            &&!HasVersionedWater44(Op,Style,Scale,Version))continue;
        if(Assets.Num()==20)
        {
            if(VersionUsed)*VersionUsed=Version;
            return true;
        }
    }
    Assets.Reset();return false;
}
struct FCollisionBuildV50
{
    bool BakesComplete=false,Reused=false;
    int32 BakedVersion=0;
    double DescriptionSeconds=0,RebuildSeconds=0;
    FIntVector Profile=FIntVector(-1,-1,-1);
    TWeakObjectPtr<UStaticMesh> InstalledMesh;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGround>,FCollisionBuildV50> CollisionBuildsV50;

bool CompleteTerrainV50(TArray<TStrongObjectPtr<UStaticMesh>>& KeepAlive,int32* SelectedVersion)
{
    if(SelectedVersion)*SelectedVersion=0;
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV43RuntimeTerrain"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))return false;
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    TArray<UStaticMesh*> Assets;
    int32 Version=0;
    if(!LoadTerrainTiles43(Op,Style,FrontlineBattlefield::OperationDepthScale,Assets,&Version))return false;
    if(SelectedVersion)*SelectedVersion=Version;
    for(auto* Mesh:Assets)KeepAlive.Emplace(Mesh);
    return KeepAlive.Num()==20;
}


}

void AWarfareFrontlineGround::RebuildTerrain()
{
    // A national encounter owns its imported physical ground. Do not construct
    // another legacy visual/collision surface after that campaign is selected.
    // Construction before a national selection, and all UK/DE builds, retain
    // the original path; PrepareScene separately hides/restores their owners.
    if(GetWorld())if(const auto* Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()))
        if(WarfareFRRUCampaignV82::Active(Battle))
        {UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_LEGACY_TERRAIN_SKIPPED active_national_campaign=1"));return;}
    const double RebuildStart=FPlatformTime::Seconds();
    int32 ProfileOp=0;EFrontlineMissionTerrainV32 ProfileStyle;FrontlineFieldV32::TerrainProfileV43(ProfileOp,ProfileStyle);
    // Selection was last-success-wins. Try that same precedence in reverse,
    // loading historical fallbacks only when the accepted material is absent.
    auto* SelectedMaterial=LoadObject<UMaterialInterface>(nullptr,TerrainPaletteV51(ProfileStyle,ProfileOp).GroundMaterial);
    const TCHAR* FallbackMaterials[]={
        TEXT("/Game/Warfare/Field/V50/Materials/MI_GroundMud_V50.MI_GroundMud_V50"),
        TEXT("/Game/Warfare/Field/V47/Materials/MI_GroundMud_V47.MI_GroundMud_V47"),
        TEXT("/Game/Warfare/Field/V46/Materials/MI_GroundMud_V46.MI_GroundMud_V46"),
        TEXT("/Game/Warfare/Field/V44/Materials/MI_GroundMud_V44.MI_GroundMud_V44"),
        TEXT("/Game/Warfare/Field/V43/Materials/MI_GroundMud_V43.MI_GroundMud_V43")};
    for(const TCHAR* Path:FallbackMaterials)
    {
        if(SelectedMaterial)break;
        SelectedMaterial=LoadObject<UMaterialInterface>(nullptr,Path);
    }
    if(!SelectedMaterial&&!FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))
        SelectedMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V42/Materials/MI_GroundMud_V42.MI_GroundMud_V42"));
    if(SelectedMaterial)GroundMaterial=SelectedMaterial;
    for(auto It=CollisionBuildsV50.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Audit=CollisionBuildsV50.FindOrAdd(this);const auto Previous=Audit;Audit=FCollisionBuildV50();
    Audit.Profile=FIntVector(ProfileOp,int32(ProfileStyle),FMath::RoundToInt(FrontlineBattlefield::OperationDepthScale*1000000.f));
    TArray<TStrongObjectPtr<UStaticMesh>> KeepBakesAlive;
    Audit.BakesComplete=CompleteTerrainV50(KeepBakesAlive,&Audit.BakedVersion);
    // A complete V57 visual fallback must rebuild V57 collision and answer
    // foot/vehicle height queries from the exact same historical function.
    // With no complete tiles, use the source matching an available water mesh.
    if(ProfileOp==7)
    {
        GMeninV58Height.Store(Audit.BakedVersion==58||Audit.BakedVersion==59||Audit.BakedVersion==63);
        GMeninV59Height.Store(Audit.BakedVersion==59||Audit.BakedVersion==63);
        GMeninV63Water.Store(Audit.BakedVersion==63);
    }
    if(ProfileOp==12)
        GLysV58Height.Store(Audit.BakedVersion==58 ||
            (Audit.BakedVersion==0&&HasVersionedWater44(ProfileOp,ProfileStyle,
                FrontlineBattlefield::OperationDepthScale,58)));
    float Density=145.f;FMeshDescription Description;
    const bool Reuse=Audit.BakesComplete&&Previous.BakesComplete&&Previous.Profile==Audit.Profile
        &&Previous.BakedVersion==Audit.BakedVersion&&Previous.InstalledMesh.IsValid();
    if(Reuse){Audit=Previous;Audit.Reused=true;}
    else
    {
        const double DescriptionStart=FPlatformTime::Seconds();
        Description=BuildTerrainDescriptionV43(Density);
        Audit.DescriptionSeconds=FPlatformTime::Seconds()-DescriptionStart;
    }
    auto Install=[&](const FMeshDescription& Source,bool CollisionOnly)
    {
    UStaticMesh* Mesh=NewObject<UStaticMesh>(this,NAME_None,RF_Transient);
    // A hidden collision surface must not allocate another full-size ray
    // tracing BLAS alongside the twenty already-rendered physical tiles.
    Mesh->bSupportRayTracing=!CollisionOnly;
    // Params.bAllowCpuAccess retains the LOD index buffer, but packaged runtime
    // GetPhysicsTriMeshData also gates access on this separate UStaticMesh flag.
    Mesh->bAllowCPUAccess=true;
    FStaticMaterial GroundSlot(GroundMaterial,TEXT("Ground"));
    // UpdateUVChannelData is editor-only. Initialize before BuildFromMeshDescriptions
    // can expose this transient material slot to streaming in a packaged game.
    GroundSlot.UVChannelData=FMeshUVChannelInfo(0.f);
    GroundSlot.UVChannelData.bOverrideDensities=true;
    GroundSlot.UVChannelData.LocalUVDensities[0]=Density;
    Mesh->GetStaticMaterials().Add(GroundSlot);
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild=true; Params.bBuildSimpleCollision=false;
    Params.bAllowCpuAccess=true; Params.bMarkPackageDirty=false;
    TArray<const FMeshDescription*> Descriptions; Descriptions.Add(&Source);
    if (Mesh->BuildFromMeshDescriptions(Descriptions,Params))
    {
        if (Mesh->GetBodySetup())
        {
            Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
            Mesh->GetBodySetup()->CreatePhysicsMeshes();
        }
        Surface->SetStaticMesh(Mesh);
        if (GroundMaterial) Surface->SetMaterial(0,GroundMaterial);
        Audit.InstalledMesh=Mesh;
        return true;
    }
    return false;
    };
    bool Installed=Reuse;
    if(Reuse){Surface->SetStaticMesh(Audit.InstalledMesh.Get());if(GroundMaterial)Surface->SetMaterial(0,GroundMaterial);}
    else Installed=Install(Description,Audit.BakesComplete);
    if(Installed)
    {
        const bool Bound=BindBakedTerrainV43();
        if(!Bound&&Audit.BakesComplete)
        {
            // Unexpected binding failure still restores the full visible source
            // and its ray-tracing support; never expose a collision-only mesh.
            Audit.BakesComplete=false;
            Audit.Reused=false;
            Description=BuildTerrainDescriptionV43(Density);
            if(!Install(Description,false))return;
        }
        BindWaterV44();
        Audit.RebuildSeconds=FPlatformTime::Seconds()-RebuildStart;
        UE_LOG(LogTemp,Display,TEXT("WF_V50_COLLISION_INSTALL exact_source_geometry=1 reused_same_profile=%d all_twenty_bakes=%d no_duplicate_rt=%d rebuild_seconds=%.3f"),Audit.Reused,Audit.BakesComplete,Bound&&Audit.BakesComplete,Audit.RebuildSeconds);
        UE_LOG(LogTemp,Display,TEXT("WF_V43_TERRAIN %s"),*DescribeTerrainV43());
        // Reconstruction can follow Weather::BindScene. Reacquire the displayed
        // MID here, not merely when a level initially creates its light rig.
        if(GetWorld()&&GetWorld()->HasBegunPlay())
            FrontlineTerrainV42::Apply(Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()));
    }
}

namespace
{
constexpr int32 TerrainTilesX43=10,TerrainTilesY43=2,TerrainTileCount43=TerrainTilesX43*TerrainTilesY43;
FString TerrainPackage43(int32 Op,EFrontlineMissionTerrainV32 Style,float Scale,int32 Tile,int32 VersionOverride)
{
    const int32 Version=VersionOverride>0?VersionOverride:Op==7?
        (FParse::Param(FCommandLine::Get(),TEXT("WFV63MeninTerrainBake"))?63:59):
        Op==12?58:Op==13?57:54;
    return FString::Printf(TEXT("/Game/Warfare/Field/V%d/Terrain/SM_Field_%02d_T%d_D%d_R3_%02d"),
        Version,Op,int32(Style),FMath::RoundToInt(Scale*1000000.f),Tile);
}
TSharedRef<FJsonObject> MeshRepresentation43(const UStaticMesh* Mesh)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("mesh"),GetPathNameSafe(Mesh));
    const auto* R=Mesh?Mesh->GetRenderData():nullptr;const auto* L=R&&R->LODResources.Num()?&R->LODResources[0]:nullptr;
    J->SetNumberField(TEXT("triangles"),L?L->GetNumTriangles():0);
    J->SetNumberField(TEXT("lumen_cards"),L&&L->CardRepresentationData?L->CardRepresentationData->MeshCardsBuildData.CardBuildData.Num():0);
    J->SetBoolField(TEXT("distance_field"),L&&L->DistanceFieldData!=nullptr);
    J->SetNumberField(TEXT("nanite_clusters"),R&&R->NaniteResourcesPtr?R->NaniteResourcesPtr->NumClusters:0);
#if WITH_EDITOR
    if(Mesh){const auto& N=Mesh->GetNaniteSettings();J->SetBoolField(TEXT("explicit_tangents"),N.bExplicitTangents);
        J->SetNumberField(TEXT("position_precision"),N.PositionPrecision);J->SetNumberField(TEXT("normal_precision"),N.NormalPrecision);J->SetNumberField(TEXT("tangent_precision"),N.TangentPrecision);}
#endif
    return J;
}
FString TerrainJSON43(const TSharedRef<FJsonObject>& J)
{FString S;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&S));return S;}
}

void AWarfareFrontlineGround::SyncTerrainMaterialV43()
{
    if(!Surface)return;
    for(const auto& C:BakedTerrainV43)if(C)C->SetMaterial(0,Surface->GetMaterial(0));
}

bool AWarfareFrontlineGround::BindBakedTerrainV43()
{
    for(const auto& C:BakedTerrainV43)if(C)C->DestroyComponent();BakedTerrainV43.Reset();
    Surface->SetVisibility(true,false);Surface->SetVisibleInRayTracing(true);Surface->SetCastShadow(true);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV43RuntimeTerrain")))return false;
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    const float Scale=FrontlineBattlefield::OperationDepthScale;
    TArray<UStaticMesh*> Assets;
    if(!LoadTerrainTiles43(Op,Style,Scale,Assets))return false;
    FString ReviewUserDir;
    const bool NaniteScanReview=FParse::Param(FCommandLine::Get(),TEXT("WFV47NaniteGround"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),ReviewUserDir)&&!ReviewUserDir.IsEmpty();
    for(auto* Mesh:Assets)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(Surface);
        C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetStaticMesh(Mesh);C->SetMaterial(0,Surface->GetMaterial(0));
        // V46 same-camera isolation: the straight soil shading seam survives
        // Lumen diffuse-off but disappears with Nanite off. Render only these
        // twenty soil partitions through their unreduced source fallback;
        // retain exact geometry, tangent basis, RT/DF/cards and other Nanite art.
        // Diagnostic only: all isolation flags are mandatory, nothing is
        // persisted and the normal game retains the accepted full-source path.
        C->SetForceDisableNanite(!NaniteScanReview);
        C->RegisterComponent();BakedTerrainV43.Add(C);
    }
    // Keep the original authoritative collision mesh; do not render it a second
    // time, and do not hide child banks/duckboards when hiding the root surface.
    Surface->SetVisibility(false,false);Surface->SetVisibleInRayTracing(false);Surface->SetCastShadow(false);
    return true;
}

FString AWarfareFrontlineGround::DescribeTerrainV43() const
{
    auto J=MakeShared<FJsonObject>();J->SetObjectField(TEXT("runtime_collision_mesh"),MeshRepresentation43(Surface?Surface->GetStaticMesh():nullptr));
    if(const auto* A=CollisionBuildsV50.Find(const_cast<AWarfareFrontlineGround*>(this)))
    {
        auto C=MakeShared<FJsonObject>();C->SetBoolField(TEXT("all_twenty_current_bakes_verified"),A->BakesComplete);
        C->SetNumberField(TEXT("visible_tile_version"),A->BakedVersion);
        C->SetBoolField(TEXT("source_collision_geometry_unchanged"),true);
        C->SetBoolField(TEXT("same_profile_collision_reused"),A->Reused);
        C->SetNumberField(TEXT("collision_description_seconds"),A->DescriptionSeconds);C->SetNumberField(TEXT("total_rebuild_seconds"),A->RebuildSeconds);
        C->SetBoolField(TEXT("collision_mesh_ray_tracing_enabled"),Surface&&Surface->GetStaticMesh()&&Surface->GetStaticMesh()->bSupportRayTracing);
        C->SetStringField(TEXT("scope"),TEXT("Original complete collision source geometry. Reuse only the same ground instance and operation/style/depth profile; disable duplicate ray-tracing geometry only when all twenty visible baked tiles are present."));
        J->SetObjectField(TEXT("runtime_collision_v50"),C);
    }
    TArray<TSharedPtr<FJsonValue>> Rows;int32 Cards=0,Clusters=0;
    for(const auto& C:BakedTerrainV43)if(C)
    {
        auto M=MeshRepresentation43(C->GetStaticMesh());
        M->SetBoolField(TEXT("full_source_raster_v46"),C->IsForceDisableNanite());
        auto* Material=C->GetMaterial(0);
        const auto* Resource=Material?Material->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
        const auto* ShaderMap=Resource?Resource->GetGameThreadShaderMap():nullptr;
        M->SetStringField(TEXT("material"),Material?Material->GetPathName():TEXT("missing"));
        M->SetStringField(TEXT("material_base"),Material?GetPathNameSafe(Material->GetBaseMaterial()):TEXT("missing"));
        M->SetBoolField(TEXT("material_resource_present"),Resource!=nullptr);
        M->SetBoolField(TEXT("material_shader_map_present"),ShaderMap!=nullptr);
        M->SetBoolField(TEXT("material_shader_map_valid"),ShaderMap&&ShaderMap->IsValidForRendering());
        M->SetBoolField(TEXT("material_game_thread_complete"),Resource&&Resource->IsGameThreadShaderMapComplete());
        M->SetBoolField(TEXT("material_shader_valid"),Resource&&Resource->IsGameThreadShaderMapComplete()&&ShaderMap&&ShaderMap->IsValidForRendering());
        Cards+=M->GetIntegerField(TEXT("lumen_cards"));Clusters+=M->GetIntegerField(TEXT("nanite_clusters"));Rows.Add(MakeShared<FJsonValueObject>(M));
    }
    J->SetArrayField(TEXT("visible_tiles"),Rows);J->SetNumberField(TEXT("lumen_cards"),Cards);J->SetNumberField(TEXT("nanite_clusters"),Clusters);
    J->SetBoolField(TEXT("baked_complete"),Rows.Num()==TerrainTileCount43&&Cards>0&&Clusters>0);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))&&!BakedTerrainV43.IsEmpty()&&BakedTerrainV43[0])
        if(auto* Assigned=BakedTerrainV43[0]->GetMaterial(0))
        {
            TArray<UTexture*> Used;Assigned->GetUsedTextures(Used,{},GMaxRHIShaderPlatform);
            TArray<TSharedPtr<FJsonValue>> TextureRows;
            for(auto* Texture:Used)if(auto* T=Cast<UTexture2D>(Texture))
            {
                auto Item=MakeShared<FJsonObject>();
                Item->SetStringField(TEXT("asset"),T->GetPathName());
                Item->SetNumberField(TEXT("width"),T->GetSizeX());
                Item->SetNumberField(TEXT("height"),T->GetSizeY());
                Item->SetNumberField(TEXT("total_mips"),T->GetNumMips());
                Item->SetNumberField(TEXT("resident_mips"),T->GetNumResidentMips());
                Item->SetNumberField(TEXT("largest_resident_width"),T->GetNumResidentMips()>0
                    ?FMath::Max(1,T->GetSizeX()>>FMath::Clamp(T->GetNumMips()-T->GetNumResidentMips(),0,30)):0);
                Item->SetBoolField(TEXT("pending_streaming"),T->HasPendingInitOrStreaming());
                TextureRows.Add(MakeShared<FJsonValueObject>(Item));
            }
            J->SetArrayField(TEXT("assigned_ground_texture_residency"),TextureRows);
        }
    J->SetObjectField(TEXT("basin_field_v47"),BasinFieldReportV47());
    if(BasinWaterV44&&BasinWaterV44->GetStaticMesh())
    {
        auto Water=MeshRepresentation43(BasinWaterV44->GetStaticMesh());
        Water->SetBoolField(TEXT("collision_disabled"),BasinWaterV44->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        auto* Mat=BasinWaterV44->GetMaterial(0);const auto* R=Mat?Mat->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
        const auto* Map=R?R->GetGameThreadShaderMap():nullptr;
        Water->SetStringField(TEXT("assigned_material"),GetPathNameSafe(Mat));
        Water->SetStringField(TEXT("material_base"),Mat?GetPathNameSafe(Mat->GetBaseMaterial()):TEXT("missing"));
        Water->SetBoolField(TEXT("material_resource_present"),R!=nullptr);
        Water->SetBoolField(TEXT("material_shader_map_present"),Map!=nullptr);
        Water->SetBoolField(TEXT("material_shader_map_valid"),Map&&Map->IsValidForRendering());
        Water->SetBoolField(TEXT("material_game_thread_complete"),R&&R->IsGameThreadShaderMapComplete());
        Water->SetBoolField(TEXT("material_shader_valid"),R&&R->IsGameThreadShaderMapComplete()&&Map&&Map->IsValidForRendering());
        J->SetObjectField(TEXT("basin_water_v44"),Water);
    }
    return TerrainJSON43(J);
}

#include "WarfareFrontlineWaterV44.inl"

FString AWarfareFrontlineGround::DescribeWaterV72()
{
    auto J=MakeShared<FJsonObject>();const auto* Data=ActiveWaterPlanesV72.Get();
    int32 RequestedOp=0;EFrontlineMissionTerrainV32 RequestedStyle;FrontlineFieldV32::TerrainProfileV43(RequestedOp,RequestedStyle);
    J->SetNumberField(TEXT("requested_operation"),RequestedOp);
    J->SetNumberField(TEXT("requested_style"),int32(RequestedStyle));
    J->SetNumberField(TEXT("requested_scale"),FrontlineBattlefield::OperationDepthScale);
    J->SetNumberField(TEXT("requested_depth_key"),FMath::RoundToInt(FrontlineBattlefield::OperationDepthScale*1000000.f));
    J->SetNumberField(TEXT("requested_geometry_version"),WaterGeometryV72(RequestedOp));
    J->SetBoolField(TEXT("active"),MatchingWaterPlanesV72(Data));
    J->SetBoolField(TEXT("baseline_requested"),FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline")));
    if(!MatchingWaterPlanesV72(Data))return TerrainJSON43(J);
    J->SetStringField(TEXT("water_mesh"),GetPathNameSafe(Data->GetOuter()));
    J->SetNumberField(TEXT("operation"),Data->Operation);J->SetNumberField(TEXT("depth_key"),Data->DepthKey);
    J->SetNumberField(TEXT("geometry_version"),Data->GeometryVersion);
    J->SetNumberField(TEXT("regional_groups"),Data->Levels.Num());
    J->SetNumberField(TEXT("legacy_wet_cells"),Data->WetCells);J->SetNumberField(TEXT("retained_wet_cells"),Data->RetainedCells);
    J->SetNumberField(TEXT("cell_size_cm"),Data->CellSize);
    TArray<TSharedPtr<FJsonValue>> Groups;
    for(int32 I=0;I<Data->Levels.Num();++I)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("group"),I);Row->SetNumberField(TEXT("level_cm"),Data->Levels[I]);
        Row->SetNumberField(TEXT("legacy_mean_cm"),Data->LegacyMeanLevels[I]);Row->SetNumberField(TEXT("spill_limit_cm"),Data->SpillLevels[I]);
        Row->SetNumberField(TEXT("encoded_group"),Data->GroupColours[I]);Groups.Add(MakeShared<FJsonValueObject>(Row));
        Row->SetStringField(TEXT("surface_kind"),FlowingRegionalGroupV72(Data,I)?TEXT("flowing_reach"):TEXT("standing_pool"));
    }
    J->SetArrayField(TEXT("groups"),Groups);
    const auto* Mesh=Cast<UStaticMesh>(Data->GetOuter());const auto* Render=Mesh?Mesh->GetRenderData():nullptr;
    const auto* Lod=Render&&!Render->LODResources.IsEmpty()?&Render->LODResources[0]:nullptr;
    float Variation=0,LevelError=0,FlowNormalError=0;int32 RegionalVertices=0,RegionalTriangles=0,StandingTriangles=0,FlowVertices=0;bool Finite=true;
    if(Lod&&Lod->VertexBuffers.ColorVertexBuffer.GetNumVertices()==Lod->VertexBuffers.PositionVertexBuffer.GetNumVertices())
    {
        const auto& Positions=Lod->VertexBuffers.PositionVertexBuffer;const auto& Colours=Lod->VertexBuffers.ColorVertexBuffer;
        for(uint32 I=0;I<Positions.GetNumVertices();++I)
        {
            const auto P=Positions.VertexPosition(I);const auto C=Colours.VertexColor(I);
            if(C.B<128)continue;++RegionalVertices;Finite&=FMath::IsFinite(P.X)&&FMath::IsFinite(P.Y)&&FMath::IsFinite(P.Z);
            const int32 Group=Data->GroupColours.Find(int32(C.G));
            if(!Data->Levels.IsValidIndex(Group)){Finite=false;continue;}
            const bool Flowing=FlowingRegionalGroupV72(Data,Group);
            const float Expected=Flowing?RegionalReferenceLevelV72(P.X,P.Y):Data->Levels[Group];
            LevelError=FMath::Max(LevelError,FMath::Abs(P.Z-(Expected+1.2f)));
            if(Flowing)
            {
                ++FlowVertices;const FVector3f Normal(Lod->VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(I));
                FlowNormalError=FMath::Max(FlowNormalError,(Normal-RegionalSurfaceNormalV72(P.X,P.Y)).Size());
                Finite&=!Normal.ContainsNaN()&&Normal.SizeSquared()>.99f;
            }
        }
        const auto Indices=Lod->IndexBuffer.GetArrayView();
        for(int32 I=0;I+2<Indices.Num();I+=3)
        {
            const uint32 A=Indices[I],B=Indices[I+1],C=Indices[I+2];
            const bool Regional=Colours.VertexColor(A).B>=128&&Colours.VertexColor(B).B>=128&&Colours.VertexColor(C).B>=128;
            if(Regional)++RegionalTriangles;
            if(Regional&&FlowingRegionalGroupV72(Data,Data->GroupColours.Find(int32(Colours.VertexColor(A).G))))continue;
            ++StandingTriangles;const float ZA=Positions.VertexPosition(A).Z,ZB=Positions.VertexPosition(B).Z,ZC=Positions.VertexPosition(C).Z;
            Variation=FMath::Max(Variation,FMath::Max3(FMath::Abs(ZA-ZB),FMath::Abs(ZB-ZC),FMath::Abs(ZC-ZA)));
        }
    }
    J->SetNumberField(TEXT("actual_regional_vertices"),RegionalVertices);J->SetNumberField(TEXT("actual_regional_triangles"),RegionalTriangles);
    J->SetNumberField(TEXT("actual_standing_triangles"),StandingTriangles);J->SetNumberField(TEXT("actual_flowing_vertices"),FlowVertices);
    J->SetNumberField(TEXT("maximum_actual_flow_normal_error"),FlowNormalError);J->SetBoolField(TEXT("lys_composition"),Data->LysComposition==1);
    J->SetNumberField(TEXT("maximum_actual_triangle_z_variation_cm"),Variation);
    J->SetNumberField(TEXT("maximum_actual_vertex_level_error_cm"),LevelError);J->SetBoolField(TEXT("actual_vertices_finite"),Finite);
    return TerrainJSON43(J);
}

void AWarfareFrontlineGround::RunChecksV72(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto* Data=ActiveWaterPlanesV72.Get();Check(TEXT("v72_horizontal_profile_matches_actual_operation_geometry_and_depth"),MatchingWaterPlanesV72(Data));
    if(!MatchingWaterPlanesV72(Data))return;
    TSharedPtr<FJsonObject> Stats;const FString Text=DescribeWaterV72();
    const bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Stats)&&Stats.IsValid();
    Check(TEXT("v72_actual_standing_water_triangles_are_horizontal"),Parsed&&Stats->GetNumberField(TEXT("actual_standing_triangles"))>0
        &&Stats->GetNumberField(TEXT("maximum_actual_triangle_z_variation_cm"))<.01f);
    Check(TEXT("v72_baked_water_vertices_use_the_embedded_query_levels"),Parsed&&Stats->GetBoolField(TEXT("actual_vertices_finite"))
        &&Stats->GetNumberField(TEXT("actual_regional_vertices"))>0&&Stats->GetNumberField(TEXT("maximum_actual_vertex_level_error_cm"))<.01f);
    bool Agreement=true,Spill=true,Colours=true;int32 Samples=0;
    const auto& Field=BasinFieldV47();
    for(int32 G=0;G<Data->Levels.Num();++G)
    {
        Spill&=FMath::IsFinite(Data->Levels[G])&&(FlowingRegionalGroupV72(Data,G)||
            (Data->Levels[G]<=Data->LegacyMeanLevels[G]+.001f&&Data->Levels[G]<=Data->SpillLevels[G]+.001f));
        Colours&=Data->GroupColours[G]>0&&Data->GroupColours[G]<255;
    }
    const int32 Stride=FMath::Max(1,Data->CellGroups.Num()/1200);
    for(int32 I=0;I<Data->CellGroups.Num();I+=Stride)
    {
        const int32 G=Data->CellGroups[I];if(!Data->Levels.IsValidIndex(G))continue;
        const float X=Data->Origin.X+(I%Data->Columns+.5f)*Data->CellSize,Y=Data->Origin.Y+(I/Data->Columns+.5f)*Data->CellSize;
        const auto R=RegionalWaterV54(X,Y,Field);if(R.Margin<0)continue;
        float Level=0,Depth=0,Colour=-1;const bool Wet=QueryWaterSurfaceV51(X,Y,Level,Depth,&Colour);
        ++Samples;Agreement&=Wet&&FMath::IsFinite(Depth)&&Depth>.0f
            &&FMath::Abs(Level-(R.Level+1.2f))<.01f
            &&FMath::Abs(Depth-(Level-HeightAt(X,Y)))<.01f&&FMath::Abs(Colour-Data->GroupColours[G]/255.f)<.0001f;
    }
    Check(TEXT("v72_shared_water_query_depth_level_and_group_match_physical_surface"),Samples>0&&Agreement);
    Check(TEXT("v72_connected_water_does_not_exceed_legacy_mean_or_bank_spill_level"),Spill);
    Check(TEXT("v72_regional_blood_and_contact_groups_have_valid_disjoint_palette"),Colours);
    if(Data->Operation==12)Check(TEXT("v72_lys_flowing_reaches_keep_actual_gradient_and_matching_baked_normals"),
        Parsed&&Stats->GetNumberField(TEXT("actual_flowing_vertices"))>0&&Stats->GetNumberField(TEXT("maximum_actual_flow_normal_error"))<.003f);
}

FString AWarfareFrontlineGround::BakeTerrainProfileV43(int32 OperationIndex)
{
    auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),false);
#if WITH_EDITOR
    const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    const auto& German=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::German);
    if(!Missions.IsValidIndex(OperationIndex)||!German.IsValidIndex(OperationIndex))return TerrainJSON43(Report);
    const auto& Mission=Missions[OperationIndex];
    // Both armies see the same battlefield. Refuse a stale/baked foreign profile.
    if(Mission.TerrainStyleV32!=German[OperationIndex].TerrainStyleV32||Mission.DepthScaleV41!=German[OperationIndex].DepthScaleV41)return TerrainJSON43(Report);
    int32 OldOp;EFrontlineMissionTerrainV32 OldStyle;FrontlineFieldV32::TerrainProfileV43(OldOp,OldStyle);
    const float OldScale=FrontlineBattlefield::OperationDepthScale;
    const bool OldMeninVersion=GMeninV58Height.Load();
    const bool OldMeninMaterialFirst=GMeninV59Height.Load();
    const bool OldMeninWater=GMeninV63Water.Load();
    const bool OldLysVersion=GLysV58Height.Load();
    ON_SCOPE_EXIT{FrontlineFieldV32::TerrainProfileV43(OldOp,OldStyle,true);FrontlineBattlefield::ConfigureOperationDepth(OldScale,OldOp);GMeninV58Height.Store(OldMeninVersion);GMeninV59Height.Store(OldMeninMaterialFirst);GMeninV63Water.Store(OldMeninWater);GLysV58Height.Store(OldLysVersion);};
    int32 Op=OperationIndex;auto Style=Mission.TerrainStyleV32;
    FrontlineFieldV32::TerrainProfileV43(Op,Style,true);
    const float Scale=FrontlineGameplayV33::OperationDepthScale(Op);FrontlineBattlefield::ConfigureOperationDepth(Scale,Op);
    if(Op==7)
    {
        GMeninV58Height.Store(true);GMeninV59Height.Store(true);
        GMeninV63Water.Store(FParse::Param(FCommandLine::Get(),TEXT("WFV63MeninTerrainBake")));
    }
    if(Op==12)GLysV58Height.Store(true);
    float Density=145;FMeshDescription Source=BuildTerrainDescriptionV43(Density);FStaticMeshConstAttributes A(Source);
    const auto SP=A.GetVertexPositions();const auto SN=A.GetVertexInstanceNormals();const auto ST=A.GetVertexInstanceTangents();
    const auto SS=A.GetVertexInstanceBinormalSigns();const auto SU=A.GetVertexInstanceUVs();const auto SC=A.GetVertexInstanceColors();
    TArray<FMeshDescription> Tiles;Tiles.SetNum(TerrainTileCount43);TArray<TMap<FVertexID,FVertexID>> Maps;Maps.SetNum(TerrainTileCount43);
    for(auto& Tile:Tiles){FStaticMeshAttributes TA(Tile);TA.Register();TA.GetVertexInstanceUVs().SetNumChannels(1);TA.GetPolygonGroupMaterialSlotNames()[Tile.CreatePolygonGroup()]=TEXT("Ground");}
    for(const auto Triangle:Source.Triangles().GetElementIDs())
    {
        const auto Corners=Source.GetTriangleVertexInstances(Triangle);FVector3f Mid=FVector3f::ZeroVector;
        for(auto VI:Corners)Mid+=SP[Source.GetVertexInstanceVertex(VI)]/3.f;
        const int32 IX=FMath::Clamp(FMath::FloorToInt((Mid.X+15000.f)/3000.f),0,TerrainTilesX43-1);
        // Keep existing partition seams fixed; the added rear apron belongs
        // to the original rear tile row, including its complete raster mesh.
        const int32 IY=FMath::Clamp(FMath::FloorToInt((Mid.Y+3550.f)/5818.75f),0,TerrainTilesY43-1);
        const int32 Index=IY*TerrainTilesX43+IX;auto& Tile=Tiles[Index];FStaticMeshAttributes TA(Tile);
        TArray<FVertexInstanceID,TInlineAllocator<3>> Out;
        for(auto VI:Corners)
        {
            const auto SV=Source.GetVertexInstanceVertex(VI);FVertexID V;
            if(const auto* Found=Maps[Index].Find(SV))V=*Found;
            else{V=Tile.CreateVertex();TA.GetVertexPositions()[V]=SP[SV];Maps[Index].Add(SV,V);}
            const auto N=Tile.CreateVertexInstance(V);TA.GetVertexInstanceNormals()[N]=SN[VI];TA.GetVertexInstanceTangents()[N]=ST[VI];
            TA.GetVertexInstanceBinormalSigns()[N]=SS[VI];TA.GetVertexInstanceUVs().Set(N,0,SU.Get(VI,0));TA.GetVertexInstanceColors()[N]=SC[VI];Out.Add(N);
        }
        Tile.CreateTriangle(FPolygonGroupID(0),Out);
    }
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TerrainPaletteV51(Style,Op).GroundMaterial);
    if(!Mat)return TerrainJSON43(Report);
    TArray<UStaticMesh*> Built;bool Good=true;int64 Triangles=0;TArray<TSharedPtr<FJsonValue>> Rows;
    for(int32 I=0;I<TerrainTileCount43;++I)
    {
        const FString PackageName=TerrainPackage43(Op,Style,Scale,I);auto* Package=CreatePackage(*PackageName);Package->FullyLoad();
        UStaticMesh* Mesh=FindObject<UStaticMesh>(Package,*FPaths::GetBaseFilename(PackageName));
        if(!Mesh)Mesh=NewObject<UStaticMesh>(Package,*FPaths::GetBaseFilename(PackageName),RF_Public|RF_Standalone);
        Mesh->GetStaticMaterials().Reset();
        Mesh->SetNumSourceModels(1);auto& Settings=Mesh->GetSourceModel(0).BuildSettings;
        Settings.bRecomputeNormals=false;Settings.bRecomputeTangents=false;Settings.bGenerateLightmapUVs=false;
        Settings.bUseFullPrecisionUVs=true;Settings.bUseHighPrecisionTangentBasis=true;Settings.MaxLumenMeshCards=24;
        Settings.bGenerateDistanceFieldAsIfTwoSided=true;Settings.DistanceFieldResolutionScale=1.f;
        Mesh->GetNaniteSettings().bEnabled=true;
        // All partitions retain the same authored tangent basis and quantization grid.
        Mesh->GetNaniteSettings().bExplicitTangents=true;Mesh->GetNaniteSettings().PositionPrecision=4;
        Mesh->GetNaniteSettings().NormalPrecision=12;Mesh->GetNaniteSettings().TangentPrecision=10;Mesh->GetNaniteSettings().GenerateFallback=ENaniteGenerateFallback::Enabled;
        Mesh->GetNaniteSettings().FallbackTarget=ENaniteFallbackTarget::RelativeError;
        // UE 5.8's reduced explicit-tangent fallback drops tangent X/Y. Keep
        // the original source fallback (about 3% more triangles on profile 0).
        Mesh->GetNaniteSettings().FallbackPercentTriangles=1.f;Mesh->GetNaniteSettings().FallbackRelativeError=0.f;
        Mesh->GetNaniteSettings().KeepPercentTriangles=1.f;Mesh->GetNaniteSettings().TrimRelativeError=0.f;
        auto& RayProxy=Mesh->GetRayTracingProxySettings();RayProxy.bEnabled=false;
        RayProxy.FallbackTarget=ENaniteFallbackTarget::RelativeError;RayProxy.FallbackPercentTriangles=1.f;RayProxy.FallbackRelativeError=0.f;
        FStaticMaterial Slot(Mat,TEXT("Ground"));Slot.UVChannelData=FMeshUVChannelInfo(Density);Slot.UVChannelData.bOverrideDensities=true;Slot.UVChannelData.LocalUVDensities[0]=Density;
        Mesh->GetStaticMaterials().Add(Slot);UStaticMesh::FBuildMeshDescriptionsParams Params;
        Params.bFastBuild=false;Params.bCommitMeshDescription=true;Params.bBuildSimpleCollision=false;Params.bMarkPackageDirty=true;
        TArray<const FMeshDescription*> Descs;Descs.Add(&Tiles[I]);Good&=Mesh->BuildFromMeshDescriptions(Descs,Params);
        Built.Add(Mesh);Triangles+=Tiles[I].Triangles().Num();FAssetRegistryModule::AssetCreated(Mesh);
    }
    FAssetCompilingManager::Get().FinishAllCompilation();
    if(GDistanceFieldAsyncQueue)GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
    if(GCardRepresentationAsyncQueue)GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete();
    for(auto* Mesh:Built)
    {
        auto Row=MeshRepresentation43(Mesh);Good&=Row->GetIntegerField(TEXT("lumen_cards"))>0&&Row->GetIntegerField(TEXT("nanite_clusters"))>0;
        int32 InvalidBasis=0;const auto& Buffer=Mesh->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer;
        for(uint32 V=0;V<Buffer.GetNumVertices();++V)
        {
            const FVector3f X(Buffer.VertexTangentX(V)),Y(Buffer.VertexTangentY(V)),Z(Buffer.VertexTangentZ(V));
            InvalidBasis+=X.ContainsNaN()||Y.ContainsNaN()||Z.ContainsNaN()||X.SizeSquared()<.8f||Y.SizeSquared()<.8f||Z.SizeSquared()<.8f;
        }
        Row->SetNumberField(TEXT("fallback_invalid_tangent_bases"),InvalidBasis);Row->SetBoolField(TEXT("unreduced_source_fallback"),true);Good&=InvalidBasis==0;
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_None;
        const FString File=FPackageName::LongPackageNameToFilename(Mesh->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
        const bool Saved=UPackage::SavePackage(Mesh->GetOutermost(),Mesh,*File,Args);Good&=Saved;Row->SetBoolField(TEXT("saved"),Saved);Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Good&=Triangles==Source.Triangles().Num()&&BasinFieldV47().ProtectedMaximumChange<=.001f;
    Good&=BasinFieldV47().FrontageSamples==165&&BasinFieldV47().FrontageChangedSamples>15
        // V54's authored regional scar may add up to 1.3 m outside protected
        // works; the old V51 1.2 m cap described scanned mud alone.
        &&BasinFieldV47().FrontageOpenSamples>50&&BasinFieldV47().FrontageMaximumDelta<220.f&&BasinFieldV47().FrontageMaximumGradient<1.8f;
    Report->SetNumberField(TEXT("source_triangles"),Source.Triangles().Num());Report->SetNumberField(TEXT("partition_triangles"),Triangles);
    Report->SetNumberField(TEXT("operation"),Op);Report->SetNumberField(TEXT("depth_scale"),Scale);Report->SetArrayField(TEXT("tiles"),Rows);Report->SetBoolField(TEXT("success"),Good);
    Report->SetObjectField(TEXT("basin_field_v47"),BasinFieldReportV47());
#else
    Report->SetStringField(TEXT("error"),TEXT("Terrain baking requires the editor asset pipeline."));
#endif
    return TerrainJSON43(Report);
}

#if WITH_EDITOR
namespace
{
// These hashes intentionally omit alpha. It is optical edge coverage only;
// physical depth, hydraulic labels and the authored mesh must remain exact.
struct FWaterDigestV75
{
    FSHA1 Hash;
    template<class T> void Value(const T& V){Hash.Update(reinterpret_cast<const uint8*>(&V),sizeof(T));}
    template<class T> void Array(const TArray<T>& Values)
    {
        Value(Values.Num());
        if(!Values.IsEmpty())Hash.Update(reinterpret_cast<const uint8*>(Values.GetData()),Values.Num()*sizeof(T));
    }
    FString Finish(){uint8 Bytes[FSHA1::DigestSize];Hash.Final();Hash.GetHash(Bytes);return BytesToHex(Bytes,UE_ARRAY_COUNT(Bytes));}
};
struct FWaterMeshDigestsV75{FString Geometry,RGB,Basis;};
FWaterMeshDigestsV75 WaterDescriptionDigestsV75(const FMeshDescription& D)
{
    FStaticMeshConstAttributes A(D);const auto P=A.GetVertexPositions();const auto C=A.GetVertexInstanceColors();
    const auto N=A.GetVertexInstanceNormals();const auto T=A.GetVertexInstanceTangents();
    const auto S=A.GetVertexInstanceBinormalSigns();const auto UV=A.GetVertexInstanceUVs();
    FWaterDigestV75 Geometry,RGB,Basis;Geometry.Value(D.Vertices().Num());Geometry.Value(D.Triangles().Num());
    for(const auto V:D.Vertices().GetElementIDs()){Geometry.Value(V.GetValue());Geometry.Value(P[V]);}
    for(const auto VI:D.VertexInstances().GetElementIDs())
    {
        Geometry.Value(VI.GetValue());Geometry.Value(D.GetVertexInstanceVertex(VI).GetValue());
        RGB.Value(VI.GetValue());RGB.Value(C[VI].X);RGB.Value(C[VI].Y);RGB.Value(C[VI].Z);
        Basis.Value(N[VI]);Basis.Value(T[VI]);Basis.Value(S[VI]);
        for(int32 Channel=0;Channel<UV.GetNumChannels();++Channel)Basis.Value(UV.Get(VI,Channel));
    }
    for(const auto Triangle:D.Triangles().GetElementIDs())
    {
        Geometry.Value(Triangle.GetValue());Geometry.Value(D.GetTrianglePolygonGroup(Triangle).GetValue());
        for(const auto VI:D.GetTriangleVertexInstances(Triangle))Geometry.Value(VI.GetValue());
    }
    return {Geometry.Finish(),RGB.Finish(),Basis.Finish()};
}
FString WaterPlaneDigestV75(const UFrontlineWaterPlanesV72* D)
{
    FWaterDigestV75 H;
    H.Value(D->Schema);H.Value(D->Operation);H.Value(D->Style);H.Value(D->DepthKey);H.Value(D->GeometryVersion);
    H.Value(D->Origin);H.Value(D->CellSize);H.Value(D->Columns);H.Value(D->Rows);
    H.Array(D->CellGroups);H.Array(D->Levels);H.Array(D->LegacyMeanLevels);H.Array(D->SpillLevels);
    H.Array(D->GroupColours);H.Array(D->GroupKinds);H.Value(D->LysComposition);H.Value(D->WetCells);
    H.Value(D->RetainedCells);H.Value(D->SpillBoundarySamples);H.Value(D->PhysicalSpillSamples);
    H.Value(D->ClippedSubmergedBoundaries);H.Value(D->bValid);return H.Finish();
}
bool WaterRenderDigestsV75(const UStaticMesh* M,FString& Geometry,FString& RGB)
{
    const auto* R=M->GetRenderData();if(!R||R->LODResources.IsEmpty())return false;
    const auto& L=R->LODResources[0];const auto& P=L.VertexBuffers.PositionVertexBuffer;
    const auto& C=L.VertexBuffers.ColorVertexBuffer;
    if(C.GetNumVertices()!=P.GetNumVertices())return false;
    FWaterDigestV75 GH,CH;GH.Value(P.GetNumVertices());
    for(uint32 I=0;I<P.GetNumVertices();++I)
    {
        GH.Value(P.VertexPosition(I));const FColor Colour=C.VertexColor(I);
        CH.Value(Colour.R);CH.Value(Colour.G);CH.Value(Colour.B);
    }
    const auto Indices=L.IndexBuffer.GetArrayView();GH.Value(Indices.Num());
    for(int32 I=0;I<Indices.Num();++I)GH.Value(Indices[I]);
    Geometry=GH.Finish();RGB=CH.Finish();return true;
}
struct FWaterWeldVertexV75{FVector3f Position;uint32 Group=0;};
struct FWaterEdgeV75{FVector2D A,B;float DepthA=0,DepthB=0;uint32 Group=0;int32 Uses=0;};
struct FWaterTriangleV75{FVector2D P[3];uint32 Group=0;};
struct FWaterSegmentV75{FVector2D A,B;uint32 Group=0;};
uint32 WaterGroupBitsV75(float G){uint32 Bits;FMemory::Memcpy(&Bits,&G,sizeof(Bits));return Bits;}
FIntVector WaterWeldCellV75(const FVector3f& P)
{
    return FIntVector(FMath::FloorToInt(P.X/.2f),FMath::FloorToInt(P.Y/.2f),FMath::FloorToInt(P.Z/.2f));
}
FIntPoint WaterBucketV75(const FVector2D& P,double Size){return FIntPoint(FMath::FloorToInt(P.X/Size),FMath::FloorToInt(P.Y/Size));}
double WaterCrossV75(const FVector2D& A,const FVector2D& B){return A.X*B.Y-A.Y*B.X;}
bool WaterTriangleContainsV75(const FWaterTriangleV75& T,const FVector2D& P)
{
    const double Area=WaterCrossV75(T.P[1]-T.P[0],T.P[2]-T.P[0]);if(FMath::Abs(Area)<1.e-6)return false;
    const double A=WaterCrossV75(T.P[1]-T.P[0],P-T.P[0]);
    const double B=WaterCrossV75(T.P[2]-T.P[1],P-T.P[1]);
    const double C=WaterCrossV75(T.P[0]-T.P[2],P-T.P[2]);
    return Area>0?(A>=-1.e-6&&B>=-1.e-6&&C>=-1.e-6):(A<=1.e-6&&B<=1.e-6&&C<=1.e-6);
}
}
#endif

FString AWarfareFrontlineGround::CloneWaterEdgeCoverageV75(int32 Op)
{
    auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),false);Report->SetNumberField(TEXT("operation"),Op);
#if WITH_EDITOR
    const TCHAR* Asset=nullptr;int32 ExpectedStyle=-1,ExpectedDepth=-1;
    switch(Op)
    {
    case 5:Asset=TEXT("SM_Basins_05_T2_D517809");ExpectedStyle=2;ExpectedDepth=517809;break;
    case 6:Asset=TEXT("SM_Basins_06_T2_D501457");ExpectedStyle=2;ExpectedDepth=501457;break;
    case 12:Asset=TEXT("SM_Basins_12_T2_D512359");ExpectedStyle=2;ExpectedDepth=512359;break;
    case 16:Asset=TEXT("SM_Basins_16_T3_D534161");ExpectedStyle=3;ExpectedDepth=534161;break;
    case 19:Asset=TEXT("SM_Basins_19_T3_D697680");ExpectedStyle=3;ExpectedDepth=697680;break;
    default:Report->SetStringField(TEXT("error"),TEXT("Only the five accepted V72 regional water meshes may be cloned."));return TerrainJSON43(Report);
    }
    const FString SourcePath=FString::Printf(TEXT("/Game/Warfare/Field/V72/Water/%s.%s"),Asset,Asset);
    const FString PackageName=FString::Printf(TEXT("/Game/Warfare/Field/V75/Water/%s"),Asset);
    Report->SetStringField(TEXT("source_asset"),SourcePath);Report->SetStringField(TEXT("clone_asset"),PackageName+TEXT(".")+Asset);
    UStaticMesh* Source=LoadObject<UStaticMesh>(nullptr,*SourcePath);
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V75/Materials/MI_BasinWater_V75.MI_BasinWater_V75"));
    const auto* Data=Source?Cast<UFrontlineWaterPlanesV72>(Source->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass())):nullptr;
    const FMeshDescription* Original=Source?Source->GetMeshDescription(0):nullptr;
    if(!Material||!Original||!Data||!Data->bValid||Data->Schema!=72||Data->Operation!=Op||Data->Style!=ExpectedStyle
        ||Data->DepthKey!=ExpectedDepth||Data->GeometryVersion!=(Op==12?1:0)||Data->Columns<=0||Data->Rows<=0||Data->CellSize<=0
        ||Data->CellGroups.Num()!=Data->Columns*Data->Rows||Data->Levels.IsEmpty()
        ||Data->Levels.Num()!=Data->GroupColours.Num()||Data->Levels.Num()!=Data->LegacyMeanLevels.Num()
        ||Data->Levels.Num()!=Data->SpillLevels.Num()||(!Data->GroupKinds.IsEmpty()&&Data->GroupKinds.Num()!=Data->Levels.Num())
        ||(Op==12&&Data->LysComposition!=1)||Source->GetNumSourceModels()!=1)
    {Report->SetStringField(TEXT("error"),TEXT("Accepted source mesh, complete valid hydrology metadata or V75 material is missing/mismatched."));return TerrainJSON43(Report);}
    FAssetCompilingManager::Get().FinishAllCompilation();
    const FWaterMeshDigestsV75 Before=WaterDescriptionDigestsV75(*Original);const FString PlaneBefore=WaterPlaneDigestV75(Data);
    FString RenderBefore,RenderRGBBefore;
    if(!WaterRenderDigestsV75(Source,RenderBefore,RenderRGBBefore))
    {Report->SetStringField(TEXT("error"),TEXT("Source LOD0 CPU positions, indices or RGB are unavailable."));return TerrainJSON43(Report);}
    FMeshDescription D=*Original;FStaticMeshAttributes A(D);const auto P=A.GetVertexPositions();auto Colours=A.GetVertexInstanceColors();
    TArray<FWaterWeldVertexV75> Welded;TMap<FIntVector,TArray<int32>> WeldCells;TMap<FVertexInstanceID,int32> Canonical;
    TMap<uint64,FWaterEdgeV75> Edges;TArray<FWaterTriangleV75> Triangles;TMap<FIntPoint,TArray<int32>> TriangleCells;
    int32 RegionalTriangles=0,LegacyTriangles=0,WeldDegenerates=0;
    auto Weld=[&](FVertexInstanceID VI)
    {
        if(const int32* Existing=Canonical.Find(VI))return *Existing;
        const FVector3f Position=P[D.GetVertexInstanceVertex(VI)];const uint32 Group=WaterGroupBitsV75(Colours[VI].Y);
        const FIntVector Cell=WaterWeldCellV75(Position);int32 Found=INDEX_NONE;
        for(int32 Z=-1;Z<=1&&Found==INDEX_NONE;++Z)for(int32 Y=-1;Y<=1&&Found==INDEX_NONE;++Y)for(int32 X=-1;X<=1&&Found==INDEX_NONE;++X)
            if(const auto* Bucket=WeldCells.Find(Cell+FIntVector(X,Y,Z)))for(int32 I:*Bucket)
                if(Welded[I].Group==Group&&(Welded[I].Position-Position).SizeSquared()<=.04f){Found=I;break;}
        if(Found==INDEX_NONE){Found=Welded.Add({Position,Group});WeldCells.FindOrAdd(Cell).Add(Found);}
        Canonical.Add(VI,Found);return Found;
    };
    for(const auto Triangle:D.Triangles().GetElementIDs())
    {
        const auto Corners=D.GetTriangleVertexInstances(Triangle);int32 Regional=0;
        for(const auto VI:Corners)Regional+=Colours[VI].Z>.5f;
        if(!Regional){++LegacyTriangles;continue;}
        if(Regional!=3||Colours[Corners[0]].Y!=Colours[Corners[1]].Y||Colours[Corners[1]].Y!=Colours[Corners[2]].Y)
        {Report->SetStringField(TEXT("error"),TEXT("A source triangle mixes water branches or hydraulic groups."));return TerrainJSON43(Report);}
        ++RegionalTriangles;FWaterTriangleV75 T;T.Group=WaterGroupBitsV75(Colours[Corners[0]].Y);int32 W[3];
        for(int32 I=0;I<3;++I)
        {
            const FVector3f V=P[D.GetVertexInstanceVertex(Corners[I])];T.P[I]=FVector2D(V.X,V.Y);W[I]=Weld(Corners[I]);
        }
        const int32 TI=Triangles.Add(T);
        const FVector2D Low(FMath::Min3(T.P[0].X,T.P[1].X,T.P[2].X),FMath::Min3(T.P[0].Y,T.P[1].Y,T.P[2].Y));
        const FVector2D High(FMath::Max3(T.P[0].X,T.P[1].X,T.P[2].X),FMath::Max3(T.P[0].Y,T.P[1].Y,T.P[2].Y));
        const auto Lo=WaterBucketV75(Low,100),Hi=WaterBucketV75(High,100);
        for(int32 Y=Lo.Y;Y<=Hi.Y;++Y)for(int32 X=Lo.X;X<=Hi.X;++X)TriangleCells.FindOrAdd(FIntPoint(X,Y)).Add(TI);
        if(W[0]==W[1]||W[1]==W[2]||W[2]==W[0]){++WeldDegenerates;continue;}
        for(int32 I=0;I<3;++I)
        {
            const int32 J=(I+1)%3;const uint64 Key=(uint64(FMath::Min(W[I],W[J]))<<32)|uint32(FMath::Max(W[I],W[J]));
            auto& E=Edges.FindOrAdd(Key);
            if(!E.Uses){E.A=T.P[I];E.B=T.P[J];E.DepthA=Colours[Corners[I]].X*80.f;E.DepthB=Colours[Corners[J]].X*80.f;E.Group=T.Group;}
            ++E.Uses;
        }
    }
    auto Covered=[&](const FVector2D& V,uint32 Group)
    {
        if(const auto* Bucket=TriangleCells.Find(WaterBucketV75(V,100)))for(int32 I:*Bucket)
            if(Triangles[I].Group==Group&&WaterTriangleContainsV75(Triangles[I],V))return true;
        return false;
    };
    TArray<FWaterSegmentV75> Segments;TMap<FIntPoint,TArray<int32>> SegmentCells;
    int32 Shared=0,NonManifold=0,Open=0,FalseInternal=0,Shallow=0;
    for(const auto& Pair:Edges)
    {
        const auto& E=Pair.Value;if(E.Uses==2){++Shared;continue;}if(E.Uses>2){++NonManifold;continue;}++Open;
        const FVector2D Delta=E.B-E.A;const double Length=Delta.Size();if(Length<.001)continue;
        const FVector2D Mid=(E.A+E.B)*.5,Across(-Delta.Y/Length*.3,Delta.X/Length*.3);
        // Exact triangle coverage on both sides catches a T-junction or a
        // sub-weld clipping seam. Neither is a shoreline worth darkening.
        if(Covered(Mid+Across,E.Group)&&Covered(Mid-Across,E.Group)){++FalseInternal;continue;}
        if(E.DepthA<=2.f&&E.DepthB<=2.f){++Shallow;continue;}
        FWaterSegmentV75 S{E.A,E.B,E.Group};
        if(E.DepthA<=2.f)S.A=E.A+Delta*((2.f-E.DepthA)/(E.DepthB-E.DepthA));
        else if(E.DepthB<=2.f)S.B=E.A+Delta*((2.f-E.DepthA)/(E.DepthB-E.DepthA));
        const int32 SI=Segments.Add(S);
        const auto Lo=WaterBucketV75(FVector2D(FMath::Min(S.A.X,S.B.X)-45,FMath::Min(S.A.Y,S.B.Y)-45),45);
        const auto Hi=WaterBucketV75(FVector2D(FMath::Max(S.A.X,S.B.X)+45,FMath::Max(S.A.Y,S.B.Y)+45),45);
        for(int32 Y=Lo.Y;Y<=Hi.Y;++Y)for(int32 X=Lo.X;X<=Hi.X;++X)SegmentCells.FindOrAdd(FIntPoint(X,Y)).Add(SI);
    }
    int32 Changed=0,RegionalInstances=0,LegacyInstances=0;float MinimumAlpha=1.f;
    for(const auto VI:D.VertexInstances().GetElementIDs())
    {
        auto C=Colours[VI];if(C.Z<=.5f){++LegacyInstances;continue;}++RegionalInstances;
        const FVector3f V=P[D.GetVertexInstanceVertex(VI)];const FVector2D XY(V.X,V.Y);const uint32 Group=WaterGroupBitsV75(C.Y);
        double DistanceSquared=45.*45.;
        if(const auto* Bucket=SegmentCells.Find(WaterBucketV75(XY,45)))for(int32 I:*Bucket)
        {
            const auto& S=Segments[I];if(S.Group!=Group)continue;const FVector2D Delta=S.B-S.A;
            const double T=FMath::Clamp(FVector2D::DotProduct(XY-S.A,Delta)/FMath::Max(Delta.SizeSquared(),1.e-9),0.,1.);
            DistanceSquared=FMath::Min(DistanceSquared,(XY-(S.A+Delta*T)).SizeSquared());
        }
        const float T=float(FMath::Sqrt(DistanceSquared)/45.);const float Alpha=T*T*(3.f-2.f*T);
        Changed+=Alpha!=C.W;MinimumAlpha=FMath::Min(MinimumAlpha,Alpha);C.W=Alpha;Colours[VI]=C;
    }
    const FWaterMeshDigestsV75 Edited=WaterDescriptionDigestsV75(D);
    if(Edited.Geometry!=Before.Geometry||Edited.RGB!=Before.RGB||Edited.Basis!=Before.Basis||Segments.IsEmpty()||!Changed)
    {Report->SetStringField(TEXT("error"),TEXT("Coverage-only copy failed geometry/RGB/basis equality or found no supported edge coverage."));return TerrainJSON43(Report);}
    auto* Package=CreatePackage(*PackageName);Package->FullyLoad();
    UStaticMesh* Mesh=FindObject<UStaticMesh>(Package,Asset);const bool NewAsset=!Mesh;
    if(!Mesh)Mesh=NewObject<UStaticMesh>(Package,Asset,RF_Public|RF_Standalone);
    Mesh->SetNumSourceModels(1);Mesh->GetSourceModel(0).BuildSettings=Source->GetSourceModel(0).BuildSettings;
    Mesh->GetNaniteSettings()=Source->GetNaniteSettings();Mesh->GetStaticMaterials()=Source->GetStaticMaterials();
    if(Mesh->GetStaticMaterials().IsEmpty())Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,TEXT("BasinWater")));
    else for(auto& Slot:Mesh->GetStaticMaterials())Slot.MaterialInterface=Material;
    Mesh->bAllowCPUAccess=true;
    Mesh->RemoveUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass());
    Mesh->AddAssetUserData(DuplicateObject<UFrontlineWaterPlanesV72>(Data,Mesh,TEXT("HorizontalWaterPlanesV72")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=false;Params.bCommitMeshDescription=true;
    Params.bBuildSimpleCollision=false;Params.bMarkPackageDirty=true;
    TArray<const FMeshDescription*> Descriptions;Descriptions.Add(&D);
    bool Good=Mesh->BuildFromMeshDescriptions(Descriptions,Params);FAssetCompilingManager::Get().FinishAllCompilation();
    if(GDistanceFieldAsyncQueue)GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
    if(GCardRepresentationAsyncQueue)GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete();
    const auto* CloneData=Cast<UFrontlineWaterPlanesV72>(Mesh->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass()));
    const auto* CloneDescription=Mesh->GetMeshDescription(0);
    FWaterMeshDigestsV75 After;if(CloneDescription)After=WaterDescriptionDigestsV75(*CloneDescription);else Good=false;
    FString RenderAfter,RenderRGBAfter;Good&=WaterRenderDigestsV75(Mesh,RenderAfter,RenderRGBAfter);
    const FString PlaneAfter=CloneData?WaterPlaneDigestV75(CloneData):TEXT("missing");
    const FWaterMeshDigestsV75 SourceAfter=WaterDescriptionDigestsV75(*Source->GetMeshDescription(0));
    Good&=After.Geometry==Before.Geometry&&After.RGB==Before.RGB&&After.Basis==Before.Basis
        &&RenderBefore==RenderAfter&&RenderRGBBefore==RenderRGBAfter&&PlaneAfter==PlaneBefore&&CloneData&&CloneData->bValid
        &&SourceAfter.Geometry==Before.Geometry&&SourceAfter.RGB==Before.RGB&&SourceAfter.Basis==Before.Basis
        &&WaterPlaneDigestV75(Data)==PlaneBefore&&Mesh->bAllowCPUAccess;
    Report->SetStringField(TEXT("source_geometry_digest"),Before.Geometry);Report->SetStringField(TEXT("clone_geometry_digest"),After.Geometry);
    Report->SetStringField(TEXT("source_rgb_digest"),Before.RGB);Report->SetStringField(TEXT("clone_rgb_digest"),After.RGB);
    Report->SetStringField(TEXT("source_basis_digest"),Before.Basis);Report->SetStringField(TEXT("clone_basis_digest"),After.Basis);
    Report->SetStringField(TEXT("source_render_geometry_digest"),RenderBefore);Report->SetStringField(TEXT("clone_render_geometry_digest"),RenderAfter);
    Report->SetStringField(TEXT("source_render_rgb_digest"),RenderRGBBefore);Report->SetStringField(TEXT("clone_render_rgb_digest"),RenderRGBAfter);
    Report->SetStringField(TEXT("source_userdata_digest"),PlaneBefore);Report->SetStringField(TEXT("clone_userdata_digest"),PlaneAfter);
    Report->SetNumberField(TEXT("source_vertices"),Original->Vertices().Num());Report->SetNumberField(TEXT("source_triangles"),Original->Triangles().Num());
    Report->SetNumberField(TEXT("regional_triangles"),RegionalTriangles);Report->SetNumberField(TEXT("legacy_triangles"),LegacyTriangles);
    Report->SetNumberField(TEXT("regional_instances"),RegionalInstances);Report->SetNumberField(TEXT("legacy_instances_unchanged"),LegacyInstances);
    Report->SetNumberField(TEXT("welded_regional_vertices"),Welded.Num());Report->SetNumberField(TEXT("weld_degenerate_triangles"),WeldDegenerates);
    Report->SetNumberField(TEXT("shared_internal_edges"),Shared);Report->SetNumberField(TEXT("nonmanifold_edges_excluded"),NonManifold);
    Report->SetNumberField(TEXT("open_edge_candidates"),Open);Report->SetNumberField(TEXT("false_internal_edges_excluded"),FalseInternal);
    Report->SetNumberField(TEXT("shallow_edges_untouched"),Shallow);Report->SetNumberField(TEXT("coverage_segments"),Segments.Num());
    Report->SetNumberField(TEXT("changed_alpha_instances"),Changed);Report->SetNumberField(TEXT("minimum_alpha"),MinimumAlpha);
    Report->SetNumberField(TEXT("coverage_world_width_cm"),45);Report->SetNumberField(TEXT("weld_tolerance_cm"),.2);
    Report->SetNumberField(TEXT("userdata_level_count"),Data->Levels.Num());Report->SetNumberField(TEXT("userdata_cell_count"),Data->CellGroups.Num());
    Report->SetBoolField(TEXT("userdata_valid"),CloneData&&CloneData->bValid);Report->SetBoolField(TEXT("cpu_access"),Mesh->bAllowCPUAccess);
    Report->SetBoolField(TEXT("exact_preservation"),Good);bool Saved=false;
    if(Good)
    {
        if(NewAsset)FAssetRegistryModule::AssetCreated(Mesh);
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_None;
        const FString File=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
        Saved=UPackage::SavePackage(Package,Mesh,*File,Args);
    }
    Report->SetBoolField(TEXT("saved"),Saved);Report->SetBoolField(TEXT("success"),Good&&Saved);
    if(!Good)Report->SetStringField(TEXT("error"),TEXT("Clone failed exact source/cooked mesh preservation; candidate was not saved."));
#else
    Report->SetStringField(TEXT("error"),TEXT("Water coverage cloning is an editor-only asset operation."));
#endif
    return TerrainJSON43(Report);
}

#include "WarfareFrontlineWaterShoreV83.inl"
