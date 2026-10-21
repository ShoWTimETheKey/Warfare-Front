#include "WarfareFrontStartupV67.h"
#include "WarfareFrontlineConsoleV79.h"
#include "FRRU/WarfareFRRUIntegration.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineColourV73.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformProcess.h"
#include "HAL/IConsoleManager.h"
#include "GenericPlatform/GenericWindow.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Widgets/SWindow.h"
#include <atomic>

#if PLATFORM_WINDOWS
#include "Windows/WindowsApplication.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <dwmapi.h>
#include <wincodec.h>
#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
constexpr UINT SplashFinishedMessage=WM_APP+67;
constexpr UINT SplashFactionMessageV79=WM_APP+79;
constexpr wchar_t SplashClass[]=L"WarfareFrontStartupV67";
struct FStartupState final : IWindowsMessageHandler
{
    std::atomic<HWND> Splash{nullptr};
    std::atomic<HWND> GameWindow{nullptr};
    std::atomic<bool> Waiting{false},MenuOnRenderThread{false},RevealQueued{false},CreationCancelled{false};
    std::atomic<int32> ErrorMode{0};
    HANDLE Thread=nullptr,Created=nullptr;
    HBITMAP Artwork=nullptr,WordmarkV72=nullptr;
    FString ArtworkPath,WordmarkPathV72;
    std::atomic<int32> RequestedFactionV79{-1},AppliedFactionV79{0};
    std::atomic<bool> VerifiedFactionV79{false};
    FDelegateHandle PresentHandle,ExitHandle;
    FGPUFenceRHIRef MenuFence;
    FTSTicker::FDelegateHandle Watchdog,FocusHandoffV8310;
    bool Attached=false,MenuSignalQueued=false;
    std::atomic<bool> WasEnabled{true};
    std::atomic<uint32> RenderAcks{0};
    double BeganAt=0;
    HWND LaunchForegroundV75=nullptr;
    DWORD LaunchInputV75=0;
    DWORD RevealInputV8310=0;
    bool FocusAllowedV8310=false,FocusCoverClosedV845=false;
    std::atomic<bool> FocusRequestedV8310{false};
    double NativeHandoffDeadlineV845=0; // Read and written only on the native cover thread.
    double FocusDeadlineV8310=0;
    IConsoleVariable* IdleVariable=nullptr;
    int32 SavedIdle=0;
    struct FAtomicDiagnostics
    {
        std::atomic<bool> SplashObserved{false},SplashExcludedFromTaskbar{false},GameObserved{false};
        std::atomic<bool> GameShellStylePreserved{false},InputRestored{false},GameUncloaked{false};
        std::atomic<bool> FocusCompletedV8310{false},FocusCancelledV8310{false};
        std::atomic<bool> SplashInitiallyForegroundV845{false},SplashForegroundBeforeCloseV845{false},ForegroundRequestSucceededV845{false};
        std::atomic<int32> ForegroundRequestsV8310{0};
        std::atomic<int32> MaximumTaskbarEligibleWindows{0};
    } Diagnostics;
    std::atomic<LONG_PTR> GameShellStyle{0};

    static bool TaskbarEligible(HWND H)
    {
        if(!H||!IsWindow(H)||!IsWindowVisible(H)||(GetWindowLongPtrW(H,GWL_STYLE)&WS_CHILD))return false;
        const LONG_PTR Ex=GetWindowLongPtrW(H,GWL_EXSTYLE);
        // DWM cloaking does not remove a window's shell identity.
        return !(Ex&WS_EX_TOOLWINDOW)&&((Ex&WS_EX_APPWINDOW)||!GetWindow(H,GW_OWNER));
    }
    void ObserveWindows()
    {
        HWND Cover=Splash.load(),Game=GameWindow.load();
        if(Cover&&IsWindow(Cover))
        {
            Diagnostics.SplashObserved=true;
            const LONG_PTR Ex=GetWindowLongPtrW(Cover,GWL_EXSTYLE);
            Diagnostics.SplashExcludedFromTaskbar=(Ex&WS_EX_TOOLWINDOW)&&!(Ex&WS_EX_APPWINDOW);
        }
        if(Game&&IsWindow(Game))
        {
            Diagnostics.GameObserved=true;
            Diagnostics.GameShellStylePreserved=(GetWindowLongPtrW(Game,GWL_EXSTYLE)&(WS_EX_APPWINDOW|WS_EX_TOOLWINDOW))==GameShellStyle;
            if(!Waiting.load())
            {
                Diagnostics.InputRestored=(!!IsWindowEnabled(Game))==WasEnabled;
                DWORD Cloaked=0;Diagnostics.GameUncloaked=SUCCEEDED(DwmGetWindowAttribute(Game,DWMWA_CLOAKED,&Cloaked,sizeof(Cloaked)))&&Cloaked==0;
            }
        }
        const int32 Count=int32(TaskbarEligible(Cover))+int32(TaskbarEligible(Game));
        int32 Maximum=Diagnostics.MaximumTaskbarEligibleWindows.load();
        while(Count>Maximum&&!Diagnostics.MaximumTaskbarEligibleWindows.compare_exchange_weak(Maximum,Count)){}
    }

    virtual bool ProcessMessage(HWND H,uint32 Msg,WPARAM W,LPARAM L,int32&) override
    {
        if(!Waiting.load()||GameWindow.load()||!(Msg==WM_SHOWWINDOW&&W
            ||Msg==WM_WINDOWPOSCHANGING&&L&&(reinterpret_cast<WINDOWPOS*>(L)->flags&SWP_SHOWWINDOW)))return false;
        wchar_t Class[64]={};GetClassNameW(H,Class,64);
        if(wcscmp(Class,L"UnrealWindow")||GetWindowLongPtrW(H,GWL_STYLE)&WS_CHILD)return false;
        RECT R{};GetClientRect(H,&R);if(R.right<320||R.bottom<180)return false;
        const BOOL Cloak=1;
        // DWM cloaking leaves the window and its swapchain rendering normally.
        // It does not add WS_EX_LAYERED, alter opacity, or change the HDR route.
        if(FAILED(DwmSetWindowAttribute(H,DWMWA_CLOAK,&Cloak,sizeof(Cloak))))
        {
            UE_LOG(LogTemp,Warning,TEXT("WF_STARTUP_V67 cloak unavailable; retaining normal visible startup"));
            Reveal(TEXT("cloak-unavailable"));return false;
        }
        GameWindow.store(H);GameShellStyle=GetWindowLongPtrW(H,GWL_EXSTYLE)&(WS_EX_APPWINDOW|WS_EX_TOOLWINDOW);
        WasEnabled=!!IsWindowEnabled(H);EnableWindow(H,0);ObserveWindows();
        UE_LOG(LogTemp,Display,TEXT("WF_STARTUP_V67 window gated before first show"));
        return false; // Never consume Windows input or UE window messages.
    }
    void CloseSplash(bool RequestHandoff=false)
    {
        if(HWND H=Splash.load())PostMessageW(H,SplashFinishedMessage,WPARAM(RequestHandoff),0);
    }
    void CompleteFocusHandoffV8310()
    {
        if(!FocusAllowedV8310)return;
        FocusDeadlineV8310=FPlatformTime::Seconds()+1.5;
        FocusHandoffV8310=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float)
        {
            const HWND H=GameWindow.load();
            const double Now=FPlatformTime::Seconds();
            if(!H||!IsWindow(H))return false;
            // The cover's own input queue must finish DestroyWindow first.
            // Activating the game before that lets the cover's destruction
            // restore another foreground window over the completed menu.
            // WM_DESTROY is still inside DestroyWindow. Wait for the native
            // cover thread to finish, so its queue cannot undo activation.
            if(Thread&&WaitForSingleObject(Thread,0)!=WAIT_OBJECT_0)
            {
                if(Now>FocusDeadlineV8310){Diagnostics.FocusCancelledV8310=true;return false;}
                return true;
            }
            if(!FocusCoverClosedV845){FocusCoverClosedV845=true;FocusDeadlineV8310=Now+1.5;}
            if(Now>FocusDeadlineV8310){Diagnostics.FocusCancelledV8310=true;return false;}
            const HWND Foreground=GetForegroundWindow();DWORD Process=0;
            if(Foreground)GetWindowThreadProcessId(Foreground,&Process);
            LASTINPUTINFO Input{sizeof(LASTINPUTINFO)};GetLastInputInfo(&Input);
            const bool OwnForeground=Process==GetCurrentProcessId();
            // Pointer movement and a double-click's trailing input do not mean
            // that another application was selected. The foreground owner is
            // the cancellation signal, not the global last-input timestamp.
            const bool UnchangedLaunch=Foreground==LaunchForegroundV75;
            const bool EmptyForeground=!Foreground&&Input.dwTime==RevealInputV8310;
            if(!OwnForeground&&!UnchangedLaunch&&!EmptyForeground)
            {
                Diagnostics.FocusCancelledV8310=true;
                UE_LOG(LogTemp,Display,TEXT("WF_STARTUP_V8310 focus handoff cancelled after external activation/input"));
                return false;
            }
            if(!FocusRequestedV8310)
            {
                FocusRequestedV8310=true;++Diagnostics.ForegroundRequestsV8310;
                const BOOL Requested=SetForegroundWindow(H);
                Diagnostics.ForegroundRequestSucceededV845=Requested!=0;
                UE_LOG(LogTemp,Display,TEXT("WF_STARTUP_V8310 foreground requested=%d after cover destruction"),int32(Requested));
            }
            // SetForegroundWindow can be asynchronous across input queues.
            // Observe subsequent message-pumped frames; do not steal focus
            // repeatedly, join input queues, synthesize Alt or make us topmost.
            if(GetForegroundWindow()!=H)return true;
            SetWindowPos(H,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
            if(FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocusToGameViewport();
            Diagnostics.FocusCompletedV8310=true;ObserveWindows();
            UE_LOG(LogTemp,Display,TEXT("WF_STARTUP_V8310 foreground handoff completed"));
            return false;
        }),.05f);
    }
    void Reveal(const TCHAR* Reason)
    {
        if(!Waiting.exchange(false))return;
        ObserveWindows();
        if(HWND H=GameWindow.load();H&&IsWindow(H))
        {
            const BOOL Cloak=0;DwmSetWindowAttribute(H,DWMWA_CLOAK,&Cloak,sizeof(Cloak));
            EnableWindow(H,WasEnabled);
            Diagnostics.InputRestored=!!IsWindowEnabled(H)==WasEnabled;
            DWORD Cloaked=0;
            Diagnostics.GameUncloaked=SUCCEEDED(DwmGetWindowAttribute(H,DWMWA_CLOAKED,&Cloaked,sizeof(Cloaked)))&&Cloaked==0;
            DWORD ForegroundProcess=0;GetWindowThreadProcessId(GetForegroundWindow(),&ForegroundProcess);
            const bool OwnForeground=ForegroundProcess==GetCurrentProcessId();
            LASTINPUTINFO Input{sizeof(LASTINPUTINFO)};GetLastInputInfo(&Input);RevealInputV8310=Input.dwTime;
            const bool LaunchUnchanged=GetForegroundWindow()==LaunchForegroundV75;
            FocusAllowedV8310=(OwnForeground||LaunchUnchanged)&&FCString::Strcmp(Reason,TEXT("shutdown"))!=0;
            if(!FocusAllowedV8310&&FCString::Strcmp(Reason,TEXT("shutdown"))!=0)Diagnostics.FocusCancelledV8310=true;
            UE_LOG(LogTemp,Display,TEXT("WF_STARTUP_V845 reveal focus_allowed=%d own_foreground=%d launch_window_unchanged=%d input_changed=%d foreground_pid=%u"),
                int32(FocusAllowedV8310),int32(OwnForeground),int32(LaunchUnchanged),int32(Input.dwTime!=LaunchInputV75),uint32(ForegroundProcess));
        }
        if(IdleVariable)IdleVariable->SetWithCurrentPriority(SavedIdle);
        CloseSplash(FocusAllowedV8310);
        CompleteFocusHandoffV8310();
        UE_LOG(LogTemp,Display,TEXT("WF_STARTUP_V67 reveal reason=%s elapsed=%.3f render_ack=%u"),Reason,FPlatformTime::Seconds()-BeganAt,RenderAcks.load());
    }
};
FStartupState StartupStateV67;

void SynchronizeVerifiedFactionV79()
{
    if(!GEngine||!StartupStateV67.Waiting.load())return;
    for(const auto& Context:GEngine->GetWorldContexts())
    {
        auto* World=Context.World();
        if(!World||World->WorldType!=EWorldType::Game||!World->HasBegunPlay())continue;
        const auto* Battle=World->GetAuthGameMode<AWarfareFrontlineGameMode>();
        if(!Battle||!Battle->HasActorBegunPlay())continue;
        const int32 Faction=0; // The launch artwork is a fixed British plate.
        // Only the native window thread owns or replaces its GDI bitmap.
        if(StartupStateV67.RequestedFactionV79.load()!=Faction)
        {
            StartupStateV67.VerifiedFactionV79.store(false);
            if(HWND Window=StartupStateV67.Splash.load();Window&&PostMessageW(Window,SplashFactionMessageV79,WPARAM(Faction),0))
                StartupStateV67.RequestedFactionV79.store(Faction);
        }
        return;
    }
}

HBITMAP LoadStartupPng(const FString& Path)
{
    IWICImagingFactory* Factory=nullptr;IWICBitmapDecoder* Decoder=nullptr;
    IWICBitmapFrameDecode* Frame=nullptr;IWICFormatConverter* Converter=nullptr;
    HBITMAP Bitmap=nullptr;UINT W=0,H=0;
    HRESULT Result=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&Factory));
    if(SUCCEEDED(Result))Result=Factory->CreateDecoderFromFilename(*Path,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&Decoder);
    if(SUCCEEDED(Result))Result=Decoder->GetFrame(0,&Frame);
    if(SUCCEEDED(Result))Result=Frame->GetSize(&W,&H);
    // Startup PNGs use the same native-thread decoder and bounded allocation.
    // Bounds also reject corrupt or unexpectedly large input.
    if(SUCCEEDED(Result)&&(W==0||H==0||W>8192||H>4096||uint64(W)*H>8388608))Result=E_INVALIDARG;
    if(SUCCEEDED(Result))Result=Factory->CreateFormatConverter(&Converter);
    if(SUCCEEDED(Result))Result=Converter->Initialize(Frame,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(Result))
    {
        BITMAPINFO Info{};Info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);Info.bmiHeader.biWidth=int(W);Info.bmiHeader.biHeight=-int(H);
        Info.bmiHeader.biPlanes=1;Info.bmiHeader.biBitCount=32;Info.bmiHeader.biCompression=BI_RGB;
        void* Pixels=nullptr;Bitmap=CreateDIBSection(nullptr,&Info,DIB_RGB_COLORS,&Pixels,nullptr,0);
        if(!Bitmap||!Pixels)Result=E_OUTOFMEMORY;
        else Result=Converter->CopyPixels(nullptr,W*4,W*H*4,reinterpret_cast<BYTE*>(Pixels));
    }
    if(Converter)Converter->Release();if(Frame)Frame->Release();if(Decoder)Decoder->Release();if(Factory)Factory->Release();
    if(FAILED(Result)){if(Bitmap)DeleteObject(Bitmap);UE_LOG(LogTemp,Warning,TEXT("WF_STARTUP_V67 PNG unavailable hr=%08x"),uint32(Result));return nullptr;}
    return Bitmap;
}

void PaintSplash(HWND H,HDC DC)
{
    RECT R{};GetClientRect(H,&R);const int W=R.right,Height=R.bottom;
    HDC Buffer=CreateCompatibleDC(DC);HBITMAP Surface=CreateCompatibleBitmap(DC,W,Height);auto Old=SelectObject(Buffer,Surface);
    HBRUSH Background=CreateSolidBrush(RGB(25,27,25));FillRect(Buffer,&R,Background);DeleteObject(Background);
    if(StartupStateV67.Artwork)
    {
        HDC Source=CreateCompatibleDC(DC);auto SourceOld=SelectObject(Source,StartupStateV67.Artwork);BITMAP B{};GetObjectW(StartupStateV67.Artwork,sizeof(B),&B);
        SetStretchBltMode(Buffer,HALFTONE);SetBrushOrgEx(Buffer,0,0,nullptr);
        StretchBlt(Buffer,0,0,W,Height,Source,0,0,B.bmWidth,B.bmHeight,SRCCOPY);
        SelectObject(Source,SourceOld);DeleteDC(Source);
    }
    SetBkMode(Buffer,TRANSPARENT);
    const int Left=FMath::RoundToInt(W*.07f);
    if(StartupStateV67.WordmarkV72)
    {
        HDC Source=CreateCompatibleDC(DC);auto SourceOld=SelectObject(Source,StartupStateV67.WordmarkV72);
        BITMAP B{};GetObjectW(StartupStateV67.WordmarkV72,sizeof(B),&B);
        const float Fit=FMath::Min(W*.42f/FMath::Max(B.bmWidth,1L),Height*.30f/FMath::Max(B.bmHeight,1L));
        const BLENDFUNCTION Blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
        AlphaBlend(Buffer,Left,FMath::RoundToInt(Height*.17f),FMath::RoundToInt(B.bmWidth*Fit),FMath::RoundToInt(B.bmHeight*Fit),Source,0,0,B.bmWidth,B.bmHeight,Blend);
        SelectObject(Source,SourceOld);DeleteDC(Source);
    }
    if(StartupStateV67.ErrorMode.load())
    {
        HFONT Body=CreateFontW(-FMath::Max(13,Height/30),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
        auto OldFont=SelectObject(Buffer,Body);SetTextColor(Buffer,RGB(222,211,188));
        RECT Message{Left,int(Height*.72f),W-Left,int(Height*.80f)};
        DrawTextW(Buffer,StartupStateV67.ErrorMode.load()==1?L"Launch artwork is unavailable.":L"The main menu did not finish loading.",-1,&Message,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        RECT Continue{Left,int(Height*.83f),int(W*.32f),int(Height*.91f)};
        RECT Exit{int(W*.36f),int(Height*.83f),int(W*.51f),int(Height*.91f)};
        DrawTextW(Buffer,L"OPEN GAME",-1,&Continue,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        DrawTextW(Buffer,L"EXIT",-1,&Exit,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        SelectObject(Buffer,OldFont);DeleteObject(Body);
    }
    BitBlt(DC,0,0,W,Height,Buffer,0,0,SRCCOPY);
    SelectObject(Buffer,Old);DeleteObject(Surface);DeleteDC(Buffer);
}
void GradeStartupBitmapV73(HBITMAP Bitmap)
{
    DIBSECTION Section{};
    if(!Bitmap||GetObjectW(Bitmap,sizeof(Section),&Section)!=sizeof(Section)||!Section.dsBm.bmBits)return;
    const int Channels=Section.dsBm.bmBitsPixel/8;
    if(Channels!=3&&Channels!=4)return;
    auto* Bytes=static_cast<uint8*>(Section.dsBm.bmBits);
    for(int Y=0;Y<Section.dsBm.bmHeight;++Y)for(int X=0;X<Section.dsBm.bmWidth;++X)
    {
        auto* P=Bytes+Y*Section.dsBm.bmWidthBytes+X*Channels;
        const FColor Encoded(P[2],P[1],P[0],255);
        const FColor Graded=FrontlineColourV73::Painting(FLinearColor(Encoded)).ToFColorSRGB();
        P[0]=Graded.B;P[1]=Graded.G;P[2]=Graded.R;
    }
}
LRESULT CALLBACK SplashProcedure(HWND H,UINT Msg,WPARAM W,LPARAM L)
{
    switch(Msg)
    {
        case WM_PAINT:{PAINTSTRUCT P;HDC DC=BeginPaint(H,&P);PaintSplash(H,DC);EndPaint(H,&P);return 0;}
        case WM_ERASEBKGND:return 1;
        case SplashFactionMessageV79:
        {
            const auto Nation=EFRRUNationV81::British;
            const int32 Faction=int32(Nation);
            if(StartupStateV67.AppliedFactionV79.load()!=Faction)
            {
                const TCHAR* Filename=TEXT("British.bmp");
                switch(Nation)
                {
                    case EFRRUNationV81::German:Filename=TEXT("German.bmp");break;
                    case EFRRUNationV81::French:Filename=TEXT("French.png");break;
                    case EFRRUNationV81::Russian:Filename=TEXT("Russian.png");break;
                    case EFRRUNationV81::British:default:break;
                }
                const FString Path=FPaths::GetPath(StartupStateV67.ArtworkPath)/Filename;
                const bool Png=Nation==EFRRUNationV81::French||Nation==EFRRUNationV81::Russian;
                auto Bitmap=Png?LoadStartupPng(Path):(HBITMAP)LoadImageW(nullptr,*Path,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
                if(!Bitmap){UE_LOG(LogTemp,Warning,TEXT("WF_STARTUP_V79 verified faction artwork unavailable faction=%d"),Faction);return 0;}
                GradeStartupBitmapV73(Bitmap);
                if(StartupStateV67.Artwork)DeleteObject(StartupStateV67.Artwork);
                StartupStateV67.Artwork=Bitmap;StartupStateV67.AppliedFactionV79.store(Faction);
            }
            StartupStateV67.VerifiedFactionV79.store(true);InvalidateRect(H,nullptr,0);
            return 0;
        }
        case WM_NCHITTEST:return StartupStateV67.ErrorMode.load()?HTCLIENT:HTCAPTION;
        case WM_LBUTTONUP:
        {
            if(!StartupStateV67.ErrorMode.load())return 0;
            RECT R{};GetClientRect(H,&R);const float X=float(short(LOWORD(L)))/FMath::Max(1L,R.right),Y=float(short(HIWORD(L)))/FMath::Max(1L,R.bottom);
            if(Y>=.82f&&Y<=.92f&&X>=.06f&&X<=.33f)AsyncTask(ENamedThreads::GameThread,[]{StartupStateV67.Reveal(TEXT("user-recovery"));});
            else if(Y>=.82f&&Y<=.92f&&X>=.35f&&X<=.52f)PostMessageW(H,WM_CLOSE,0,0);
            return 0;
        }
        case SplashFinishedMessage:
        {
            const HWND Game=StartupStateV67.GameWindow.load();
            DWORD ForegroundProcess=0;GetWindowThreadProcessId(GetForegroundWindow(),&ForegroundProcess);
            const bool OwnForeground=ForegroundProcess==GetCurrentProcessId();
            StartupStateV67.Diagnostics.SplashForegroundBeforeCloseV845=GetForegroundWindow()==H;
            // Transfer while our cover still owns the foreground. This keeps
            // the legitimate launch activation alive across the two queues.
            if(W&&Game&&IsWindow(Game)&&OwnForeground&&!StartupStateV67.FocusRequestedV8310.exchange(true))
            {
                ++StartupStateV67.Diagnostics.ForegroundRequestsV8310;
                const BOOL Requested=SetForegroundWindow(Game);
                StartupStateV67.Diagnostics.ForegroundRequestSucceededV845=Requested!=0;
                if(Requested&&GetForegroundWindow()!=Game)
                {
                    StartupStateV67.NativeHandoffDeadlineV845=FPlatformTime::Seconds()+1.;
                    if(SetTimer(H,67,10,nullptr))return 0;
                }
            }
            DestroyWindow(H);return 0;
        }
        case WM_TIMER:
            if(W==67)
            {
                if(GetForegroundWindow()!=StartupStateV67.GameWindow.load()
                    &&FPlatformTime::Seconds()<StartupStateV67.NativeHandoffDeadlineV845)return 0;
                KillTimer(H,67);DestroyWindow(H);return 0;
            }
            return DefWindowProcW(H,Msg,W,L);
        case WM_CLOSE:AsyncTask(ENamedThreads::GameThread,[]{RequestEngineExit(TEXT("Startup window closed"));});DestroyWindow(H);return 0;
        case WM_DESTROY:StartupStateV67.Splash.store(nullptr);PostQuitMessage(0);return 0;
        default:return DefWindowProcW(H,Msg,W,L);
    }
}
DWORD WINAPI SplashThread(void*)
{
    // A private awareness context keeps this pre-Slate window sharp without
    // changing the process context that UE establishes during initialization.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HRESULT Com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    StartupStateV67.Artwork=(HBITMAP)LoadImageW(nullptr,*StartupStateV67.ArtworkPath,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
    GradeStartupBitmapV73(StartupStateV67.Artwork);
    if(SUCCEEDED(Com))StartupStateV67.WordmarkV72=LoadStartupPng(StartupStateV67.WordmarkPathV72);
    if(!StartupStateV67.Artwork||!StartupStateV67.WordmarkV72)StartupStateV67.ErrorMode.store(1);
    WNDCLASSEXW WC{};WC.cbSize=sizeof(WC);WC.lpfnWndProc=SplashProcedure;WC.hInstance=GetModuleHandleW(nullptr);WC.lpszClassName=SplashClass;WC.hCursor=LoadCursor(nullptr,IDC_ARROW);
    RegisterClassExW(&WC);
    POINT Mouse{};GetCursorPos(&Mouse);MONITORINFO Monitor{sizeof(Monitor)};GetMonitorInfoW(MonitorFromPoint(Mouse,MONITOR_DEFAULTTONEAREST),&Monitor);
    const int MW=Monitor.rcWork.right-Monitor.rcWork.left,MH=Monitor.rcWork.bottom-Monitor.rcWork.top;
    const int W=FMath::Clamp(FMath::RoundToInt(MW*.40f),640,1100),H=FMath::RoundToInt(W*941.f/1672.f);
    // A transient cover is not a second application. Only UE's normal window
    // owns a taskbar/Alt-Tab entry. Never reparent that window to this short-
    // lived cover or change its HDR swapchain, opacity or extended styles.
    HWND Window=CreateWindowExW(WS_EX_TOOLWINDOW,SplashClass,L"Warfare Front",WS_POPUP,
        Monitor.rcWork.left+(MW-W)/2,Monitor.rcWork.top+(MH-H)/2,W,H,nullptr,nullptr,WC.hInstance,nullptr);
    StartupStateV67.Splash.store(Window);
    if(Window)
    {
        DWORD ForegroundProcess=0;GetWindowThreadProcessId(GetForegroundWindow(),&ForegroundProcess);
        const bool LaunchSelected=GetForegroundWindow()==StartupStateV67.LaunchForegroundV75||ForegroundProcess==GetCurrentProcessId();
        ShowWindow(Window,LaunchSelected?SW_SHOWNORMAL:SW_SHOWNOACTIVATE);
        if(LaunchSelected&&GetForegroundWindow()!=Window)SetForegroundWindow(Window);
        StartupStateV67.Diagnostics.SplashInitiallyForegroundV845=GetForegroundWindow()==Window;
        UpdateWindow(Window);
    }
    SetEvent(StartupStateV67.Created);
    if(StartupStateV67.CreationCancelled.load()&&Window)PostMessageW(Window,SplashFinishedMessage,0,0);
    if(Window){MSG M;while(GetMessageW(&M,nullptr,0,0)>0){TranslateMessage(&M);DispatchMessageW(&M);}}
    DeleteObject(StartupStateV67.Artwork);StartupStateV67.Artwork=nullptr;
    DeleteObject(StartupStateV67.WordmarkV72);StartupStateV67.WordmarkV72=nullptr;if(SUCCEEDED(Com))CoUninitialize();
    UnregisterClassW(SplashClass,WC.hInstance);return 0;
}
}
#endif

namespace FrontlineStartupV67
{
void Begin()
{
#if PLATFORM_WINDOWS
    if(StartupStateV67.Thread||GIsEditor||IsRunningCommandlet()||IsRunningDedicatedServer()||!FApp::CanEverRender()
        ||FParse::Param(FCommandLine::Get(),TEXT("NoSplash"))||FParse::Param(FCommandLine::Get(),TEXT("RenderOffScreen")))return;
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("Startup/V73"));
    StartupStateV67.AppliedFactionV79.store(0);
    StartupStateV67.ArtworkPath=Folder/TEXT("British.bmp");StartupStateV67.WordmarkPathV72=Folder/TEXT("Wordmark.png");
    StartupStateV67.LaunchForegroundV75=GetForegroundWindow();
    LASTINPUTINFO LaunchInput{sizeof(LASTINPUTINFO)};GetLastInputInfo(&LaunchInput);StartupStateV67.LaunchInputV75=LaunchInput.dwTime;
    StartupStateV67.BeganAt=FPlatformTime::Seconds();StartupStateV67.Created=CreateEventW(nullptr,1,0,nullptr);
    StartupStateV67.Thread=CreateThread(nullptr,0,SplashThread,nullptr,0,nullptr);
    if(StartupStateV67.Thread&&WaitForSingleObject(StartupStateV67.Created,2000)!=WAIT_OBJECT_0)StartupStateV67.CreationCancelled.store(true);
    StartupStateV67.Waiting.store(StartupStateV67.Splash.load()!=nullptr&&!StartupStateV67.CreationCancelled.load());
    StartupStateV67.ObserveWindows();
    if(StartupStateV67.Waiting.load())
    {
        StartupStateV67.IdleVariable=IConsoleManager::Get().FindConsoleVariable(TEXT("t.IdleWhenNotForeground"));
        if(StartupStateV67.IdleVariable){StartupStateV67.SavedIdle=StartupStateV67.IdleVariable->GetInt();StartupStateV67.IdleVariable->SetWithCurrentPriority(0);}
    }
    StartupStateV67.ExitHandle=FCoreDelegates::OnPreExit.AddStatic(&Shutdown);
#endif
}
void AttachSlate()
{
#if PLATFORM_WINDOWS
    if(!StartupStateV67.Waiting.load()||StartupStateV67.Attached||!FSlateApplication::IsInitialized())return;
    StartupStateV67.Attached=true;
    StaticCastSharedPtr<FWindowsApplication>(FSlateApplication::Get().GetPlatformApplication())->AddMessageHandler(StartupStateV67);
    StartupStateV67.PresentHandle=FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddLambda([](SWindow& Window,ISlateViewportProvider&)
    {
        if(!StartupStateV67.Waiting.load()||StartupStateV67.RevealQueued.load()||!StartupStateV67.MenuOnRenderThread.load()||!Window.GetNativeWindow().IsValid()
            ||Window.GetNativeWindow()->GetOSWindowHandle()!=StartupStateV67.GameWindow.load())return;
        ++StartupStateV67.RenderAcks;
        // This callback precedes Present. Wait for the first complete menu's
        // GPU work on a subsequent frame, without blocking either CPU thread.
        if(!StartupStateV67.MenuFence.IsValid())
        {
            StartupStateV67.MenuFence=RHICreateGPUFence(TEXT("WarfareFrontMainMenuV67"));
            FRHICommandListExecutor::GetImmediateCommandList().WriteGPUFence(StartupStateV67.MenuFence);
            return;
        }
        if(!StartupStateV67.MenuFence->Poll()||StartupStateV67.RevealQueued.exchange(true))return;
        StartupStateV67.MenuFence.SafeRelease();
        AsyncTask(ENamedThreads::GameThread,[]{StartupStateV67.Reveal(TEXT("main-menu-presented"));});
    });
    StartupStateV67.Watchdog=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
    {
        if(!StartupStateV67.Waiting.load())return false;
        SynchronizeVerifiedFactionV79();
        StartupStateV67.ObserveWindows();
        if(FPlatformTime::Seconds()-StartupStateV67.BeganAt>90.)
        {
            StartupStateV67.ErrorMode.store(2);
            if(HWND H=StartupStateV67.Splash.load())InvalidateRect(H,nullptr,0);
            UE_LOG(LogTemp,Warning,TEXT("WF_STARTUP_V67 menu timeout: recovery choices shown on native launch window"));
            return false;
        }
        return true;
    }),.5f);
#endif
}
void NotifyMainMenuDrawn(bool bArtworkAndFontsReady)
{
#if PLATFORM_WINDOWS
    SynchronizeVerifiedFactionV79();
    // Let the native thread finish the verified national plate before the
    // render handoff closes it. Bitmap decoding never races the reveal.
    if(!bArtworkAndFontsReady||!StartupStateV67.VerifiedFactionV79.load()
        ||StartupStateV67.AppliedFactionV79.load()!=StartupStateV67.RequestedFactionV79.load()
        ||!StartupStateV67.Waiting.load()||StartupStateV67.MenuSignalQueued)return;
    StartupStateV67.MenuSignalQueued=true;
    ENQUEUE_RENDER_COMMAND(FrontlineMainMenuReadyV67)([](FRHICommandListImmediate&){StartupStateV67.MenuOnRenderThread.store(true);});
#endif
}
bool IsWaitingForMenu()
{
#if PLATFORM_WINDOWS
    return StartupStateV67.Waiting.load();
#else
    return false;
#endif
}
FWindowDiagnostics WindowDiagnostics()
{
#if PLATFORM_WINDOWS
    StartupStateV67.ObserveWindows();const auto& D=StartupStateV67.Diagnostics;
    FWindowDiagnostics Copy;
    Copy.SplashObserved=D.SplashObserved.load();Copy.SplashExcludedFromTaskbar=D.SplashExcludedFromTaskbar.load();
    Copy.GameObserved=D.GameObserved.load();Copy.GameShellStylePreserved=D.GameShellStylePreserved.load();
    Copy.InputRestored=D.InputRestored.load();Copy.GameUncloaked=D.GameUncloaked.load();
    Copy.GameForegroundV75=GetForegroundWindow()==StartupStateV67.GameWindow.load();
    Copy.FocusCompletedV8310=D.FocusCompletedV8310.load();Copy.FocusCancelledV8310=D.FocusCancelledV8310.load();
    Copy.ForegroundRequestsV8310=D.ForegroundRequestsV8310.load();
    Copy.SplashInitiallyForegroundV845=D.SplashInitiallyForegroundV845.load();
    Copy.SplashForegroundBeforeCloseV845=D.SplashForegroundBeforeCloseV845.load();
    Copy.ForegroundRequestSucceededV845=D.ForegroundRequestSucceededV845.load();
    if(HWND Game=StartupStateV67.GameWindow.load();Game&&IsWindow(Game))
    {
        RECT GameRect{};GetWindowRect(Game,&GameRect);MONITORINFO Monitor{sizeof(Monitor)};
        if(GetMonitorInfoW(MonitorFromWindow(Game,MONITOR_DEFAULTTONEAREST),&Monitor))
            Copy.GameCoversMonitorV8310=GameRect.left<=Monitor.rcMonitor.left+2&&GameRect.top<=Monitor.rcMonitor.top+2
                &&GameRect.right>=Monitor.rcMonitor.right-2&&GameRect.bottom>=Monitor.rcMonitor.bottom-2;
        // Read shell z-order, not just taskbar visibility. A visible taskbar
        // may correctly remain behind an active borderless/fullscreen game.
        for(HWND Above=GetWindow(Game,GW_HWNDPREV);Above;Above=GetWindow(Above,GW_HWNDPREV))
        {
            if(!IsWindowVisible(Above))continue;wchar_t Class[64]={};GetClassNameW(Above,Class,64);
            if(wcscmp(Class,L"Shell_TrayWnd")&&wcscmp(Class,L"Shell_SecondaryTrayWnd"))continue;
            RECT Taskbar{},Overlap{};GetWindowRect(Above,&Taskbar);
            if(IntersectRect(&Overlap,&GameRect,&Taskbar))Copy.TaskbarAboveGameV8310=true;
        }
    }
    Copy.SplashFactionVerifiedV79=StartupStateV67.VerifiedFactionV79.load();Copy.SplashFactionV79=StartupStateV67.AppliedFactionV79.load();
    Copy.MaximumTaskbarEligibleWindows=D.MaximumTaskbarEligibleWindows.load();return Copy;
#else
    return {};
#endif
}
void Shutdown()
{
#if PLATFORM_WINDOWS
    StartupStateV67.Reveal(TEXT("shutdown"));StartupStateV67.CloseSplash();
    if(StartupStateV67.Attached&&FSlateApplication::IsInitialized())
    {
        StaticCastSharedPtr<FWindowsApplication>(FSlateApplication::Get().GetPlatformApplication())->RemoveMessageHandler(StartupStateV67);
        if(auto* Renderer=FSlateApplication::Get().GetRenderer())Renderer->OnBackBufferReadyToPresent().Remove(StartupStateV67.PresentHandle);
    }
    StartupStateV67.Attached=false;FTSTicker::GetCoreTicker().RemoveTicker(StartupStateV67.Watchdog);
    FTSTicker::GetCoreTicker().RemoveTicker(StartupStateV67.FocusHandoffV8310);
    if(GIsThreadedRendering)
    {
        ENQUEUE_RENDER_COMMAND(FrontlineStartupReleaseV67)([](FRHICommandListImmediate&){StartupStateV67.MenuFence.SafeRelease();});
        FlushRenderingCommands();
    }
    else StartupStateV67.MenuFence.SafeRelease();
    FCoreDelegates::OnPreExit.Remove(StartupStateV67.ExitHandle);
    if(StartupStateV67.Thread){WaitForSingleObject(StartupStateV67.Thread,1500);CloseHandle(StartupStateV67.Thread);StartupStateV67.Thread=nullptr;}
    if(StartupStateV67.Created){CloseHandle(StartupStateV67.Created);StartupStateV67.Created=nullptr;}
#endif
}
}
