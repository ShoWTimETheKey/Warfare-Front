#pragma once
#include "CoreMinimal.h"

struct FFrontlineActiveDisplay
{
    FString Id,Name,RefreshSource;
    FIntRect Bounds;
    float RefreshHz=0,PeakNits=0,BlackNits=0,FullFrameNits=0,SDRWhiteNits=203;
    bool bValid=false,bRefreshKnown=false,bHDRSupported=false,bWindowsHDREnabled=false;
    // A detected monitor may lack an EDID/interface path (adapters/virtual TVs).
    // Such a monitor is usable, but its session identity must never key a save.
    bool bPersistentIdentity=false;
    bool bHDRStateKnown=false,bLuminanceKnown=false,bSDRWhiteKnown=false;
};
struct FFrontlineHDRValues
{
    int32 PeakNits=1000;
    float PaperWhiteNits=203,UILuminanceNits=203,BlackNits=.0001f;
    bool bCalibrated=false;
};
struct FFrontlineFrameTiming
{
    float ReportedPresentFPS=0;
    int32 FramesPresentedPerRendered=0;
    bool bPresentEstimateValid=false;
};
namespace FrontlineDisplay
{
    // Fresh profiles follow the desktop's aspect ratio without requesting a
    // mode larger than the panel. Above 4K pixel count, use a supported mode.
    FIntPoint InitialResolution(FIntPoint Desktop,const TArray<FIntPoint>& Supported);
    FFrontlineActiveDisplay Query(void* NativeWindow);
    int32 SelectByWindow(const FIntRect& Window,const TArray<FFrontlineActiveDisplay>& Displays);
    FIntPoint FrameRateRange(float ActiveHz);
    int32 SliderFrameRate(float Position,float ActiveHz);
    FString HDRSection(const FString& DisplayId);
    FFrontlineHDRValues DefaultHDR(const FFrontlineActiveDisplay& Display);
    FFrontlineActiveDisplay PreserveKnownState(FFrontlineActiveDisplay Current,const FFrontlineActiveDisplay& Previous);
}
