// Value-only drawing snapshots: no actor/temporary view references survive a
// hover, and replay never translates, formats or measures text.
enum class ETooltipOpV83 : uint8 {Rect,Line,Gradient,Triangle,Text,Texture,Material,Hit};
struct FTooltipOpV83
{
    ETooltipOpV83 Kind=ETooltipOpV83::Rect;
    float X=0,Y=0,W=0,H=0,U=0,V=0,UW=1,VH=1,Width=1;
    FVector2D Third=FVector2D::ZeroVector;
    FLinearColor Ink=FLinearColor::White,End=FLinearColor::White;
    bool Vertical=false,ShapedV8210=false;
    UTexture* Texture=nullptr;
    UMaterialInterface* Material=nullptr;
    const FTexture* Coverage=nullptr;
    FText Text;
    FSlateFontInfo Font;
    FName HitName;
    int32 Priority=0;
};
struct FTooltipBufferV83
{
    TArray<FTooltipOpV83> Ops;
    uint64 Revision=0;
    bool Ready=false;
    FVector2D CapturedAnchor=FVector2D::ZeroVector,LastAnchor=FVector2D::ZeroVector;
    float FadeAlpha=0,FadeOffset=0;
    float FollowHeight=0;
    double FadeAt=-1;
};
struct FTooltipStateV83
{
    FTooltipBufferV83 Active,Leaving;
    uint64 Pending=0,Candidate=0;
    double HoverAt=0;
    float OutputScale=-1;
    float CanvasDPI=-1;
    bool HDRComposite=false;
    TWeakObjectPtr<UFont> LatinFace,ChineseFace,ControlFace;
    int32 Language=-1;
    FGuid Encounter;
    void Reset()
    {Active.Ops.Reset();Leaving.Ops.Reset();Active.Ready=Leaving.Ready=false;Pending=Candidate=0;}
};
// The upper byte identifies the call site; identity/revision are scalar data,
// independent of projected position and displayed language.
inline uint64 TooltipKeyV83(uint32 Site,uint32 Identity=0)
{return (uint64(Site)<<32)|(uint64(Identity)+1);}
