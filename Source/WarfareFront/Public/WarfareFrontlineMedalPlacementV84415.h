#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineMedalRepeatLimitsV85.h"

// Coordinates belong to the complete accepted source canvas. Both the normal
// museum image and the cropped, swinging hanger project that same canvas.
namespace FrontlineMedalPlacementV84415
{
struct FBarLayout
{
    FBox2D Textile{ForceInit};
    TArray<FBox2D,TInlineAllocator<5>> Bars;
    int32 RecordedCount=0;
    bool Valid=false;
};
inline FBox2D NativeTextile(int32 Index)
{
    // Interior cloth only: exclude frayed selvedges and the metal suspension.
    // Audited against the original PNGs, not the overall transparent bounds.
    switch(Index)
    {
    case 0:return {{335.f/1147,38.f/1371},{814.f/1147,622.f/1371}};
    case 1:return {{266.f/1024,36.f/1536},{754.f/1024,550.f/1536}};
    case 2:return {{235.f/1024,48.f/1536},{788.f/1024,587.f/1536}};
    case 12:return {{289.f/1024,51.f/1536},{735.f/1024,624.f/1536}};
    case 13:return {{273.f/1024,80.f/1536},{750.f/1024,657.f/1536}};
    case 37:return {{284.f/1024,84.f/1536},{742.f/1024,609.f/1536}};
    case 38:return {{308.f/1024,48.f/1536},{712.f/1024,536.f/1536}};
    case 39:return {{290.f/1024,73.f/1536},{735.f/1024,551.f/1536}};
    default:return FBox2D(ForceInit);
    }
}
inline FBarLayout Layout(int32 Index,int32 Count,float X,float Y,float W,float H)
{
    FBarLayout Result;const auto Native=NativeTextile(Index);
    if(!Native.bIsValid||!FMath::IsFinite(X)||!FMath::IsFinite(Y)
        ||!FMath::IsFinite(W)||!FMath::IsFinite(H)||W<=0||H<=0)return Result;
    Result.Textile={{X+W*Native.Min.X,Y+H*Native.Min.Y},{X+W*Native.Max.X,Y+H*Native.Max.Y}};
    // This is a presentation projection, never a rewrite of the saved count.
    Result.RecordedCount=FrontlineMedalsV8215::PresentedRepeatsV85(Index,Count);Result.Valid=true;
    const int32 Visible=Result.RecordedCount;if(Visible==0)return Result;
    const auto Size=Result.Textile.GetSize();const float Gap=Size.Y*.035f;
    const float BarHeight=FMath::Min(float(Size.X)*(Index==12?313.f/1809.f:Index==2?237.f/2047.f:
        (Index==1||Index==37)?316.f/1812.f:(Index==38||Index==39)?249.f/2077.f:235.f/2066.f),
        (float(Size.Y)-(Visible-1)*Gap)/(Visible+1.f));
    const float Used=Visible*BarHeight+(Visible-1)*Gap,Top=Result.Textile.GetCenter().Y-Used*.5f;
    for(int32 K=0;K<Visible;++K)
    {
        const float BarY=Top+K*(BarHeight+Gap);
        Result.Bars.Add({{Result.Textile.Min.X,BarY},{Result.Textile.Max.X,BarY+BarHeight}});
    }
    return Result;
}
inline float RibbonDeviceSizeV85(float W,float H,int32 Count)
{
    if(!FMath::IsFinite(W)||!FMath::IsFinite(H)||W<=0||H<=0)return 0;
    const int32 Visible=FMath::Clamp(Count,1,FrontlineMedalsV8215::MaximumRepeatsV85);
    return FMath::Min(H*.84f,W/(Visible+1.f)*.82f);
}
inline FBox2D RibbonDeviceBoundsV85(float X,float Y,float W,float H,int32 Count,int32 Position)
{
    const float Size=RibbonDeviceSizeV85(W,H,Count);
    if(Size<=0||Count<1||Position<0||Position>=Count)return FBox2D(ForceInit);
    const float CX=X+W*(Position+1.f)/(Count+1.f),CY=Y+H*.5f;
    return {{CX-Size*.5f,CY-Size*.5f},{CX+Size*.5f,CY+Size*.5f}};
}
}
