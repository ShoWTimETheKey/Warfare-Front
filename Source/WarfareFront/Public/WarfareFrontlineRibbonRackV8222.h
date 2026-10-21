#pragma once
#include "CoreMinimal.h"

/** Compact display of an already precedence-sorted ribbon list. Highest
 * distinctions occupy the upper row; any short row is uppermost and centred.
 * Three columns adapt the wearing principles to the game's museum aperture,
 * rather than claiming one identical 1914 regulation for every army. */
namespace FrontlineRibbonRackV8222
{
inline constexpr float TextileAspectV837=4.f;
inline TArray<FBox2D> Layout(int32 Count,float X,float Y,float W,float H,bool Centred)
{
    TArray<FBox2D> Cells;
    if(Count<1||Count>36||!FMath::IsFinite(X)||!FMath::IsFinite(Y)
        ||!FMath::IsFinite(W)||!FMath::IsFinite(H)||W<=0||H<=0)return Cells;
    const int32 Columns=FMath::Min(3,Count),Rows=(Count+Columns-1)/Columns;
    const int32 TopCount=Count-Columns*(Rows-1);
    // Adjacent mounted textiles share their boundary in both directions. A
    // subpixel row gap becomes a clearly visible dark rule at native 4K.
    const float BarHeight=FMath::Min(28.f,FMath::Min(W/(Columns*TextileAspectV837),H/Rows));
    if(BarHeight<=0)return Cells;
    const float BarWidth=BarHeight*TextileAspectV837,BlockW=BarWidth*Columns;
    const float StartX=X+(Centred?(W-BlockW)*.5f:0.f);
    const float StartY=Y+(H-BarHeight*Rows)*.5f;
    Cells.Reserve(Count);
    for(int32 Row=0;Row<Rows;++Row)
    {
        const int32 OnRow=Row==0?TopCount:Columns;
        const float RowX=StartX+(BlockW-BarWidth*OnRow)*.5f,RowY=StartY+Row*BarHeight;
        for(int32 Col=0;Col<OnRow;++Col)
            Cells.Add(FBox2D(FVector2D(RowX+Col*BarWidth,RowY),FVector2D(RowX+(Col+1)*BarWidth,RowY+BarHeight)));
    }
    return Cells;
}
inline void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    bool Fits=true,NoOverlap=true,Touching=true,ShortTop=true,Multiline=true;
    for(int32 Count=1;Count<=36;++Count)for(bool Centred:{false,true})
    {
        const auto C=Layout(Count,126,516,949,60,Centred);Fits&=C.Num()==Count;
        for(int32 I=0;I<C.Num();++I)
        {
            const auto& A=C[I];Fits&=A.GetSize().X>0&&A.GetSize().Y>0&&A.Min.X>=125.999
                &&A.Min.Y>=515.999&&A.Max.X<=1075.001&&A.Max.Y<=576.001;
            for(int32 J=I+1;J<C.Num();++J)
            {const auto& B=C[J];NoOverlap&=A.Max.X<=B.Min.X+.001||B.Max.X<=A.Min.X+.001||A.Max.Y<=B.Min.Y+.001||B.Max.Y<=A.Min.Y+.001;}
            if(I&&FMath::IsNearlyEqual(float(A.Min.Y),float(C[I-1].Min.Y),.001f))
                Touching&=FMath::IsNearlyEqual(float(C[I-1].Max.X),float(A.Min.X),.001f);
        }
        if(Count>3)
        {
            const int32 Top=Count-3*((Count+2)/3-1);
            ShortTop&=C.IsValidIndex(Top)&&C[Top].Min.Y>C[0].Min.Y
                &&FMath::IsNearlyEqual(float((C[0].Min.X+C[Top-1].Max.X)*.5),float((C[Top].Min.X+C[Top+2].Max.X)*.5),.001f);
            Multiline&=C.Last().Min.Y>C[0].Min.Y;
        }
    }
    Check(TEXT("v8222_ribbon_rack_all_cells_inside_aperture"),Fits);
    Check(TEXT("v8222_ribbon_rack_multiple_rows_after_three"),Multiline);
    Check(TEXT("v8222_ribbon_rack_short_top_row_centred"),ShortTop);
    Check(TEXT("v8222_ribbon_rack_textiles_touch_within_row"),Touching);
    Check(TEXT("v8222_ribbon_rack_rows_never_overlap"),NoOverlap);
    bool AllViewsTight=true;
    // Actual museum, operations-header and battlefield-tooltip apertures.
    // All use this function; scales vary, topology and textile aspect do not.
    for(const FVector2D Size:{FVector2D(949,64),FVector2D(240,42),FVector2D(580,54)})
        for(int32 Count=1;Count<=36;++Count)
    {
        const auto C=Layout(Count,0,0,Size.X,Size.Y,false);
        AllViewsTight&=C.Num()==Count;
        for(int32 I=0;I<C.Num();++I)
        {
            const auto S=C[I].GetSize();AllViewsTight&=FMath::IsNearlyEqual(float(S.X/S.Y),TextileAspectV837,.001f);
            if(I&&FMath::IsNearlyEqual(float(C[I].Min.Y),float(C[I-1].Min.Y),.001f))
                AllViewsTight&=FMath::IsNearlyEqual(float(C[I].Min.X),float(C[I-1].Max.X),.001f);
            if(I&&C[I].Min.Y>C[I-1].Min.Y)
                AllViewsTight&=FMath::IsNearlyEqual(float(C[I].Min.Y),float(C[I-1].Max.Y),.001f);
        }
    }
    Check(TEXT("v837_ribbon_rack_wall_headquarters_battle_share_aspect_and_tight_seams"),AllViewsTight);
    Check(TEXT("v8310_ribbon_rack_rows_share_the_same_boundary"),AllViewsTight);
    Check(TEXT("v8222_ribbon_rack_invalid_input_empty"),Layout(0,0,0,100,40,true).IsEmpty()
        &&Layout(37,0,0,100,40,true).IsEmpty()&&Layout(5,0,0,0,40,true).IsEmpty());
}
}
