#include "WarfareFrontlineDisplayMonitor.h"
#include "Misc/SecureHash.h"
#include "Templates/RefCounting.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <dxgi1_6.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

FIntPoint FrontlineDisplay::InitialResolution(FIntPoint Desktop,const TArray<FIntPoint>& Supported)
{
    if(Desktop.X<=0 || Desktop.Y<=0)return FIntPoint(1920,1080);
    constexpr int64 PixelBudget=int64(3840)*2160;
    if(int64(Desktop.X)*Desktop.Y<=PixelBudget)return Desktop;
    const double Aspect=double(Desktop.X)/Desktop.Y;
    FIntPoint Best=FIntPoint::ZeroValue;double BestAspectError=TNumericLimits<double>::Max();int64 BestPixels=0;
    for(const FIntPoint Mode:Supported)
    {
        const int64 Pixels=int64(Mode.X)*Mode.Y;
        if(Mode.X<640 || Mode.Y<480 || Mode.X>Desktop.X || Mode.Y>Desktop.Y || Pixels>PixelBudget)continue;
        const double AspectError=FMath::Abs(double(Mode.X)/Mode.Y/Aspect-1.);
        if(AspectError<BestAspectError-.005 || (FMath::Abs(AspectError-BestAspectError)<=.005 && Pixels>BestPixels))
        {Best=Mode;BestAspectError=AspectError;BestPixels=Pixels;}
    }
    // A failed mode query must not invent a fullscreen mode. The known current
    // desktop is always valid; the user can select a smaller window afterwards.
    return BestPixels>0?Best:Desktop;
}

int32 FrontlineDisplay::SelectByWindow(const FIntRect& Window,const TArray<FFrontlineActiveDisplay>& Displays)
{
    int32 Selected=INDEX_NONE;int64 BestArea=-1;double BestDistance=TNumericLimits<double>::Max();
    for(int32 I=0;I<Displays.Num();++I)
    {
        const auto& B=Displays[I].Bounds;
        const int64 Area=int64(FMath::Max(0,FMath::Min(Window.Max.X,B.Max.X)-FMath::Max(Window.Min.X,B.Min.X)))
            *FMath::Max(0,FMath::Min(Window.Max.Y,B.Max.Y)-FMath::Max(Window.Min.Y,B.Min.Y));
        const FVector2D Centre((Window.Min.X+Window.Max.X)*.5,(Window.Min.Y+Window.Max.Y)*.5);
        const FVector2D Closest(FMath::Clamp(Centre.X,double(B.Min.X),double(B.Max.X)),FMath::Clamp(Centre.Y,double(B.Min.Y),double(B.Max.Y)));
        const double Distance=FVector2D::DistSquared(Centre,Closest);
        if(Area>BestArea || (Area==BestArea && Distance<BestDistance)){Selected=I;BestArea=Area;BestDistance=Distance;}
    }
    return Selected;
}
FIntPoint FrontlineDisplay::FrameRateRange(float Hz)
{
    // Integer FPS follows the nominal current refresh (e.g. 59.94 -> 60), never
    // the panel's EDID maximum. Unknown displays use an explicitly labelled bound.
    const int32 Maximum=FMath::Clamp(FMath::IsFinite(Hz)&&Hz>1?FMath::RoundToInt(Hz):120,10,1000);
    return FIntPoint(FMath::Min(30,Maximum),Maximum);
}
int32 FrontlineDisplay::SliderFrameRate(float Position,float Hz)
{
    const FIntPoint R=FrameRateRange(Hz);
    return FMath::RoundToInt(FMath::Lerp(float(R.X),float(R.Y),FMath::Clamp(Position,0.f,1.f)));
}
FString FrontlineDisplay::HDRSection(const FString& Id)
{return TEXT("Warfare.HDR.")+FMD5::HashAnsiString(*Id);}
FFrontlineHDRValues FrontlineDisplay::DefaultHDR(const FFrontlineActiveDisplay& Display)
{
    FFrontlineHDRValues V;
    V.PeakNits=FMath::Clamp(FMath::IsFinite(Display.PeakNits)&&Display.PeakNits>100?FMath::RoundToInt(Display.PeakNits):1000,200,4000);
    V.BlackNits=FMath::Clamp(FMath::IsFinite(Display.BlackNits)?Display.BlackNits:.0001f,.0001f,.1f);
    // Peak/full-frame panel capability is not diffuse white. Windows exposes
    // the user's reference white separately, in 80-nit / 1000 units.
    const float White=Display.bSDRWhiteKnown && FMath::IsFinite(Display.SDRWhiteNits)?Display.SDRWhiteNits:203.f;
    V.PaperWhiteNits=V.UILuminanceNits=FMath::Clamp(White,80.f,FMath::Min(1000.f,V.PeakNits*.8f));
    return V;
}
FFrontlineActiveDisplay FrontlineDisplay::PreserveKnownState(FFrontlineActiveDisplay Current,const FFrontlineActiveDisplay& Previous)
{
    if(!Current.bValid)
    {
        // An entirely failed physical query is transient. A partial query that
        // identifies a different display must not become the previous monitor.
        if((!Current.Id.IsEmpty() && Current.Id!=Previous.Id) ||
           (!Current.Name.IsEmpty() && Current.Name!=Previous.Name))return Current;
        return Previous;
    }
    const bool SamePersistent=Current.bPersistentIdentity && Previous.bPersistentIdentity && Current.Id==Previous.Id;
    if(!Current.bRefreshKnown && Previous.bValid && SamePersistent)
    {Current.RefreshHz=Previous.RefreshHz;Current.RefreshSource=TEXT("Last valid current-mode query; refresh temporarily unavailable");}
    if(SamePersistent)
    {
        if(!Current.bHDRStateKnown)
        {Current.bHDRSupported=Previous.bHDRSupported;Current.bWindowsHDREnabled=Previous.bWindowsHDREnabled;Current.bHDRStateKnown=Previous.bHDRStateKnown;}
        if(!Current.bLuminanceKnown)
        {Current.PeakNits=Previous.PeakNits;Current.BlackNits=Previous.BlackNits;Current.FullFrameNits=Previous.FullFrameNits;Current.bLuminanceKnown=Previous.bLuminanceKnown;}
        if(!Current.bSDRWhiteKnown && Previous.bSDRWhiteKnown)
        {Current.SDRWhiteNits=Previous.SDRWhiteNits;Current.bSDRWhiteKnown=true;}
    }
    return Current;
}
FFrontlineActiveDisplay FrontlineDisplay::Query(void* NativeWindow)
{
    FFrontlineActiveDisplay D;
#if PLATFORM_WINDOWS
    if(!NativeWindow)return D;
    const HMONITOR Monitor=MonitorFromWindow(static_cast<HWND>(NativeWindow),MONITOR_DEFAULTTONEAREST);
    MONITORINFOEXW Info{};Info.cbSize=sizeof(Info);
    if(!GetMonitorInfoW(Monitor,reinterpret_cast<MONITORINFO*>(&Info)))return D;
    D.bValid=true;D.Name=Info.szDevice;
    FString SessionIdentity=FString::Printf(TEXT("session:%s:%p"),Info.szDevice,static_cast<void*>(Monitor));
    DISPLAY_DEVICEW Device{};Device.cb=sizeof(Device);
    // Interface identity survives Windows renumbering DISPLAY1 / DISPLAY2.
    if(EnumDisplayDevicesW(Info.szDevice,0,&Device,EDD_GET_DEVICE_INTERFACE_NAME) && Device.DeviceID[0])
    {D.Id=Device.DeviceID;D.bPersistentIdentity=true;}
    D.Bounds=FIntRect(Info.rcMonitor.left,Info.rcMonitor.top,Info.rcMonitor.right,Info.rcMonitor.bottom);
    // This enumerates the active desktop setting, not the supported-mode list.
    DEVMODEW Mode{};Mode.dmSize=sizeof(Mode);
    if(EnumDisplaySettingsExW(Info.szDevice,ENUM_CURRENT_SETTINGS,&Mode,0)&&Mode.dmDisplayFrequency>1)
    {D.RefreshHz=float(Mode.dmDisplayFrequency);D.bRefreshKnown=true;D.RefreshSource=TEXT("EnumDisplaySettingsEx ENUM_CURRENT_SETTINGS");}
    for(int32 Attempt=0;Attempt<3;++Attempt)
    {
        UINT32 PathCount=0,ModeCount=0;
        if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&PathCount,&ModeCount)!=ERROR_SUCCESS)break;
        TArray<DISPLAYCONFIG_PATH_INFO> Paths;Paths.SetNumZeroed(PathCount);
        TArray<DISPLAYCONFIG_MODE_INFO> Modes;Modes.SetNumZeroed(ModeCount);
        const LONG Result=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&PathCount,Paths.GetData(),&ModeCount,Modes.GetData(),nullptr);
        if(Result==ERROR_INSUFFICIENT_BUFFER)continue;
        if(Result!=ERROR_SUCCESS)break;
        for(UINT32 I=0;I<PathCount;++I)
        {
            const auto& P=Paths[I];DISPLAYCONFIG_SOURCE_DEVICE_NAME Source{};
            Source.header={DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,sizeof(Source),P.sourceInfo.adapterId,P.sourceInfo.id};
            if(DisplayConfigGetDeviceInfo(&Source.header)!=ERROR_SUCCESS || FCString::Stricmp(Source.viewGdiDeviceName,Info.szDevice)!=0)continue;
            SessionIdentity=FString::Printf(TEXT("session:%s:%08x:%08x:%u:%p"),Info.szDevice,
                uint32(P.targetInfo.adapterId.HighPart),uint32(P.targetInfo.adapterId.LowPart),uint32(P.targetInfo.id),static_cast<void*>(Monitor));
            DISPLAYCONFIG_TARGET_DEVICE_NAME Target{};
            Target.header={DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,sizeof(Target),P.targetInfo.adapterId,P.targetInfo.id};
            if(DisplayConfigGetDeviceInfo(&Target.header)==ERROR_SUCCESS)
            {
                if(Target.monitorDevicePath[0]){D.Id=Target.monitorDevicePath;D.bPersistentIdentity=true;}
                if(Target.monitorFriendlyDeviceName[0])D.Name=Target.monitorFriendlyDeviceName;
            }
            if(P.targetInfo.refreshRate.Denominator && P.targetInfo.refreshRate.Numerator>1)
            {D.RefreshHz=float(P.targetInfo.refreshRate.Numerator)/P.targetInfo.refreshRate.Denominator;D.bRefreshKnown=true;D.RefreshSource=TEXT("QueryDisplayConfig QDC_ONLY_ACTIVE_PATHS");}
            DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO Color{};
            Color.header={DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO,sizeof(Color),P.targetInfo.adapterId,P.targetInfo.id};
            if(DisplayConfigGetDeviceInfo(&Color.header)==ERROR_SUCCESS)
            {D.bHDRStateKnown=true;D.bHDRSupported=Color.advancedColorSupported;D.bWindowsHDREnabled=Color.advancedColorEnabled;}
            DISPLAYCONFIG_SDR_WHITE_LEVEL White{};
            White.header={DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,sizeof(White),P.targetInfo.adapterId,P.targetInfo.id};
            if(DisplayConfigGetDeviceInfo(&White.header)==ERROR_SUCCESS && White.SDRWhiteLevel>=1000 && White.SDRWhiteLevel<=12500)
            {D.SDRWhiteNits=float(White.SDRWhiteLevel)*80.f/1000.f;D.bSDRWhiteKnown=true;}
            break;
        }
        break;
    }
    // Refresh DXGI metadata rather than reusing RHI's startup display cache.
    TRefCountPtr<IDXGIFactory1> Factory;
    if(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(Factory.GetInitReference()))))
        for(UINT A=0;;++A)
        {
            TRefCountPtr<IDXGIAdapter1> Adapter;if(Factory->EnumAdapters1(A,Adapter.GetInitReference())!=S_OK)break;
            bool Found=false;
            for(UINT O=0;;++O)
            {
                TRefCountPtr<IDXGIOutput> Output;if(Adapter->EnumOutputs(O,Output.GetInitReference())!=S_OK)break;
                DXGI_OUTPUT_DESC Base{};if(FAILED(Output->GetDesc(&Base))||Base.Monitor!=Monitor)continue;
                TRefCountPtr<IDXGIOutput6> Output6;
                if(SUCCEEDED(Output->QueryInterface(IID_PPV_ARGS(Output6.GetInitReference()))))
                {
                    DXGI_OUTPUT_DESC1 Desc{};
                    if(SUCCEEDED(Output6->GetDesc1(&Desc)))
                    {
                        D.PeakNits=FMath::IsFinite(Desc.MaxLuminance)?Desc.MaxLuminance:0;
                        D.BlackNits=FMath::IsFinite(Desc.MinLuminance)?Desc.MinLuminance:0;
                        D.FullFrameNits=FMath::IsFinite(Desc.MaxFullFrameLuminance)?Desc.MaxFullFrameLuminance:0;
                        D.bHDRStateKnown=true;D.bLuminanceKnown=D.PeakNits>=100 && D.PeakNits<=10000;
                        D.bWindowsHDREnabled=Desc.ColorSpace==DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                        D.bHDRSupported|=D.bWindowsHDREnabled;
                    }
                }
                Found=true;break;
            }
            if(Found)break;
        }
#endif
    // Missing stable identity does not mean missing physical HDR support. Keep
    // current-mode/HDR observations usable, but never save or reuse a profile
    // under a transient GDI/adapter/target session key.
#if PLATFORM_WINDOWS
    if(D.bValid && D.Id.IsEmpty()){D.Id=SessionIdentity;D.bPersistentIdentity=false;}
#endif
    if(!D.Id.IsEmpty())D.Id.ToLowerInline();
    return D;
}
