#include "WarfareFrontlineFrameCap.h"

void FFrontlineFrameCapController::Select(int32 NewMode,int32 MaximumRatio,bool Draining,double Now)
{
    const bool WasGenerating=Mode>0 || bDraining;
    bDraining=NewMode==0 && Draining;
    if(NewMode!=Mode)
    {
        Mode=NewMode;ModeChangedAt=Now;CandidateSamples=0;LastFrame=0;
        if(Mode>0)
        {
            const int32 Seed=Mode<6?Mode+1:FMath::Clamp(MaximumRatio,2,6);
            Ratio=WasGenerating?FMath::Max(Ratio,Seed):Seed;
        }
    }
    // Keep the old ratio until every queued FG present has retired.
    if(Mode==0 && !bDraining){Ratio=1;Candidate=1;CandidateSamples=0;}
}
void FFrontlineFrameCapController::Observe(int32 PresentedPerRendered,uint64 RenderFrame,double Now)
{
    if(Mode==0 || RenderFrame==LastFrame || PresentedPerRendered<1 || PresentedPerRendered>6)return;
    LastFrame=RenderFrame;
    if(PresentedPerRendered>Ratio)
    {Ratio=PresentedPerRendered;CandidateSamples=0;Candidate=Ratio;CandidateSince=Now;return;}
    if(PresentedPerRendered==Ratio){CandidateSamples=0;Candidate=Ratio;return;}
    if(Candidate!=PresentedPerRendered){Candidate=PresentedPerRendered;CandidateSamples=1;CandidateSince=Now;}
    else ++CandidateSamples;
    // Slower, stable downward changes prevent one-frame SDK fallback reports
    // from making render FPS and FG multiplier chase one another.
    if(Now-ModeChangedAt>=2. && Now-CandidateSince>=1.5 && CandidateSamples>=30)
    {Ratio=Candidate;CandidateSamples=0;}
}
float FFrontlineFrameCapController::RenderLimit(float TotalFPS) const
{return FMath::Max(1.f,TotalFPS/FMath::Max(1,Ratio));}
