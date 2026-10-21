#pragma once
#include "CoreMinimal.h"

// Presentation-only scalar timelines. They never read a battle clock, write a
// save, load an asset or participate in a casualty/victory decision.
namespace FrontlineUIMotionV83
{
inline float Unit(double Value)
{return float(FMath::Clamp(FMath::IsFinite(Value)?Value:0.,0.,1.));}
inline float CubicOut(double Value)
{const float T=1.f-Unit(Value);return 1.f-T*T*T;}
inline float CubicIn(double Value)
{const float T=Unit(Value);return T*T*T;}

struct FTooltipSample
{float Alpha=0.f,OffsetY=6.f;};
inline FTooltipSample TooltipSample(double HoverAge,double FadeAge=-1.,float FadeStartAlpha=1.f,bool Stable=false)
{
    if(Stable)return {FadeAge<0?1.f:0.f,0.f};
    const float T=Unit((HoverAge-.25)/.12);
    // Alpha is linear so the specified 310-ms half-opacity sample is exact;
    // the six-pixel travel decelerates independently using cubic-out.
    return {FadeAge>=0?Unit(FadeStartAlpha)*(1.f-Unit(FadeAge/.08)):T,6.f*(1.f-CubicOut(T))};
}

struct FBannerSample
{float OffsetX=0.f,StampScale=1.f,StampBrightness=1.f;};
inline FBannerSample BannerSample(double Age,bool Stable=false)
{
    if(Stable)return {};
    constexpr float Outside=668.f; // Existing x=1260; right canvas edge=1920.
    const float Entry=CubicOut(Age/.32),Exit=CubicIn((Age-6.26)/.24);
    const float Stamp=CubicOut((Age-.18)/.50);
    return {Outside*(1.f-Entry+Exit),1.f+.35f*(1.f-Stamp),1.f+.45f*(1.f-Stamp)};
}
inline int32 RollNumber(int32 Final,double VisibleAge,bool Stable=false)
{
    if(Stable||VisibleAge>=.60)return Final;
    if(VisibleAge<=0.||Final<=0)return 0;
    // Keep the intermediate in double/int64: float flooring rounds MAX_int32
    // up to 2147483648 and can wrap an otherwise monotone counter.
    return int32(FMath::Clamp(FMath::FloorToInt64(double(Final)*double(CubicOut(VisibleAge/.60))),int64(0),int64(Final)));
}

struct FMoraleMotion
{
    float Display=1.f,Ghost=1.f,PreviousTarget=1.f,GhostFrom=1.f,GhostTarget=1.f;
    double GhostAge=.88;
    bool Initial=true,HasGhost=false;
    void Step(float RawTarget,float RealSeconds)
    {
        const float Target=Unit(RawTarget),DT=FMath::Clamp(FMath::IsFinite(RealSeconds)?RealSeconds:0.f,0.f,.25f);
        if(Initial){Display=Ghost=PreviousTarget=GhostFrom=GhostTarget=Target;Initial=false;HasGhost=false;return;}
        if(Target<PreviousTarget)
        {
            GhostFrom=FMath::Max(Display,HasGhost?Ghost:Display);
            GhostTarget=Target;GhostAge=0.;HasGhost=GhostFrom>Target;
        }
        else if(Target>PreviousTarget)HasGhost=false;
        PreviousTarget=Target;
        // exp(-15 * .2) leaves 4.9787% of a step: the readout completes more
        // than 95% in 200 ms, at any fixed real-time sample rate.
        Display=FMath::Lerp(Display,Target,1.f-FMath::Exp(-DT*15.f));
        if(HasGhost)
        {
            GhostAge+=DT;
            Ghost=FMath::Lerp(GhostFrom,GhostTarget,CubicIn((GhostAge-.28)/.60));
            // RealSeconds is binary32; e.g. 88 * .01f can be a few ns short.
            if(GhostAge>=.88-1.e-7)HasGhost=false;
        }
        if(!HasGhost)Ghost=Display;
    }
};

inline void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    Check(TEXT("v83_tooltip_hidden_before_250ms"),TooltipSample(.24).Alpha==0.f);
    Check(TEXT("v83_tooltip_half_opacity_at_310ms"),FMath::IsNearlyEqual(TooltipSample(.31).Alpha,.5f,.00001f));
    Check(TEXT("v83_tooltip_terminal_at_370ms"),FMath::IsNearlyEqual(TooltipSample(.37).Alpha,1.f)&&TooltipSample(.37).OffsetY==0.f);
    Check(TEXT("v83_tooltip_gone_after_80ms_fade"),TooltipSample(.37,.08).Alpha==0.f);
    Check(TEXT("v83_tooltip_fade_preserves_partial_opacity"),FMath::IsNearlyEqual(TooltipSample(.31,.04,.5f).Alpha,.25f));
    const auto B0=BannerSample(0),B32=BannerSample(.32),B18=BannerSample(.18),B68=BannerSample(.68),B65=BannerSample(6.5);
    Check(TEXT("v83_banner_starts_wholly_off_right_edge"),1260.f+B0.OffsetX>=1920.f);
    Check(TEXT("v83_banner_entry_completes_at_320ms"),B32.OffsetX==0.f);
    Check(TEXT("v83_banner_stamp_begins_at_135_percent"),FMath::IsNearlyEqual(B18.StampScale,1.35f));
    Check(TEXT("v83_banner_stamp_settles_at_680ms"),B68.StampScale==1.f&&B68.StampBrightness==1.f);
    Check(TEXT("v83_banner_leaves_wholly_off_right_edge_at_6500ms"),1260.f+B65.OffsetX>=1920.f);
    Check(TEXT("v83_banner_terminal_review_is_static"),BannerSample(.3,true).OffsetX==0.f&&BannerSample(.3,true).StampScale==1.f);
    bool Monotone=true,Endpoints=true;
    for(const int32 Final:{0,1,3,27,134,10000,MAX_int32})
    {
        int32 Last=0;Endpoints&=RollNumber(Final,0)==0&&RollNumber(Final,.6)==Final&&RollNumber(Final,.9)==Final;
        for(int32 I=0;I<=600;++I){const int32 Value=RollNumber(Final,double(I)/1000.);Monotone&=Value>=Last&&Value<=Final;Last=Value;}
    }
    Check(TEXT("v83_report_numbers_exact_endpoints"),Endpoints);
    Check(TEXT("v83_report_numbers_monotone_integer_roll"),Monotone);
    Check(TEXT("v83_report_roll_uses_cubic_out"),RollNumber(800,.3)==700);
    bool First=true,Fast=true,Hold=true,Retract=true,Repeated=true,Recover=true,RealTime=true;
    for(const float DT:{1.f/30.f,1.f/60.f,1.f/120.f,.01f})
    {
        FMoraleMotion M;M.Step(.9f,DT);First&=!M.HasGhost&&M.Display==.9f;
        M.Step(.4f,0);const int32 Steps=FMath::CeilToInt(.2f/DT);
        for(int32 I=0;I<Steps;++I)M.Step(.4f,DT);
        Fast&=(M.Display-.4f)/(.9f-.4f)<=.05001f;
        FMoraleMotion H;H.Step(1.f,0);H.Step(.4f,0);
        const int32 HoldSteps=FMath::FloorToInt(.28f/DT);
        for(int32 I=0;I<HoldSteps;++I)H.Step(.4f,DT);
        Hold&=H.Ghost==1.f;
        const int32 RetractSteps=FMath::CeilToInt(.88f/DT)+1;
        for(int32 I=HoldSteps;I<RetractSteps;++I)H.Step(.4f,DT);
        Retract&=!H.HasGhost&&FMath::IsNearlyEqual(H.Ghost,H.Display);
        FMoraleMotion R;R.Step(1.f,0);R.Step(.7f,0);R.Step(.7f,.25f);R.Step(.7f,.25f);
        const float LongEdge=R.Ghost;R.Step(.4f,0);
        Repeated&=R.Ghost>=LongEdge&&R.GhostAge==0.&&R.HasGhost;
        R.Step(.6f,0);Recover&=!R.HasGhost;
        FMoraleMotion Paused,HalfSpeed,DoubleSpeed;
        Paused.Step(1.f,0);HalfSpeed.Step(1.f,0);DoubleSpeed.Step(1.f,0);
        for(int32 I=0;I<50;++I){Paused.Step(.4f,DT);HalfSpeed.Step(.4f,DT);DoubleSpeed.Step(.4f,DT);}
        RealTime&=Paused.Display==HalfSpeed.Display&&Paused.Display==DoubleSpeed.Display&&Paused.Ghost==HalfSpeed.Ghost&&Paused.Ghost==DoubleSpeed.Ghost;
    }
    Check(TEXT("v83_morale_first_sample_has_no_ghost"),First);
    Check(TEXT("v83_morale_at_least_95_percent_in_200ms_fixed_steps"),Fast);
    Check(TEXT("v83_morale_ghost_holds_280ms_fixed_steps"),Hold);
    Check(TEXT("v83_morale_ghost_retracts_by_880ms_fixed_steps"),Retract);
    FMoraleMotion Exact;Exact.Step(1.f,0);Exact.Step(.4f,0);for(int32 I=0;I<88;++I)Exact.Step(.4f,.01f);
    Check(TEXT("v83_morale_exact_880ms_binary32_step_closes_ghost"),!Exact.HasGhost&&Exact.Ghost==Exact.Display);
    Check(TEXT("v83_morale_repeat_loss_retains_long_edge_and_restarts_hold"),Repeated);
    Check(TEXT("v83_morale_recovery_clears_ghost"),Recover);
    Check(TEXT("v83_motion_has_no_simulation_speed_input"),RealTime);
}
}
