#pragma once
#include "CoreMinimal.h"

// Pure limiter state, also exercised with synthetic presentation observations.
// Mode matches EFrontlineFrameGeneration: Off, 2x..6x, Dynamic.
struct FFrontlineFrameCapController
{
    int32 Mode=0,Ratio=1,Candidate=1,CandidateSamples=0;
    uint64 LastFrame=0;
    double ModeChangedAt=0,CandidateSince=0;
    bool bDraining=false;
    void Select(int32 NewMode,int32 MaximumRatio,bool Draining,double Now);
    void Observe(int32 PresentedPerRendered,uint64 RenderFrame,double Now);
    float RenderLimit(float TotalFPS) const;
};
