// Included inside the presentation namespace. Display changes use the engine's
// real window settings. One confirmation applies and persists the choice immediately.
void LoadDisplayChoices(FFrontlineScreenState& S)
{
    UGameUserSettings* G=GEngine?GEngine->GetGameUserSettings():nullptr;
    if (!G) return;
    G->ResetToCurrentSettings(); // Includes a one-off resolution/fullscreen launcher override.
    S.DisplayResolution=G->GetScreenResolution(); S.DisplayMode=G->GetFullscreenMode();
    TArray<FIntPoint> Supported; UKismetSystemLibrary::GetSupportedFullscreenResolutions(Supported);
    S.ResolutionChoices.Empty();
    for (const FIntPoint R:Supported)
        if (R.X>=1280 && R.Y>=720) S.ResolutionChoices.AddUnique(R);
    if (S.ResolutionChoices.IsEmpty())
        for (const FIntPoint R:{FIntPoint(1280,720),FIntPoint(1920,1080),FIntPoint(2560,1440),G->GetDesktopResolution()})
            if (R.X>0 && R.Y>0) S.ResolutionChoices.AddUnique(R);
    S.ResolutionChoices.AddUnique(S.DisplayResolution);
    S.ResolutionChoices.Sort([](const FIntPoint& A,const FIntPoint& B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});
}

// Kept for old review source compatibility. Closing the page never rolls back.
void FinishVideoChange(FFrontlineScreenState& S,bool)
{ S.VideoPending=false;S.VideoDeadline=0; }

void ApplyVideoChange(FFrontlineScreenState& S)
{
    if (UGameUserSettings* G=GEngine?GEngine->GetGameUserSettings():nullptr)
    {
        auto* Render=S.Owner.IsValid()?UFrontlineRenderSettingsSubsystem::Get(S.Owner->GetWorld()):nullptr;
        if(Render)
        {
            TWeakObjectPtr<AWarfareFrontlineController> Weak(Cast<AWarfareFrontlineController>(S.Owner.Get()));
            if(Render->DeferOutputChangeV844(TEXT("WindowMode"),[Weak]{if(auto* PC=Weak.Get())ApplyVideoChange(ScreenState(PC));}))return;
        }
        if (Render) Render->SuspendFrameGeneration(1.f);
        if (S.DisplayMode==EWindowMode::WindowedFullscreen)
            S.DisplayResolution=Render && Render->GetActiveDisplay().bValid?Render->GetActiveDisplay().Bounds.Size():G->GetDesktopResolution();
        G->SetScreenResolution(S.DisplayResolution);G->SetFullscreenMode(S.DisplayMode);
        G->ApplyResolutionSettings(false);
        G->ConfirmVideoMode();
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))G->SaveSettings();
        if (Render) Render->Reapply();
        S.DisplayResolution=G->GetScreenResolution();S.DisplayMode=G->GetFullscreenMode();
        S.VideoPending=false;S.VideoDeadline=0;
        UE_LOG(LogTemp,Display,TEXT("WF_V18_DISPLAY_APPLIED resolution=%dx%d mode=%d"),S.DisplayResolution.X,S.DisplayResolution.Y,int32(S.DisplayMode));
    }
}

constexpr float FrameRateSliderX=662.f,FrameRateSliderWidth=450.f;
bool UpdateFrameRateFromPointer(FFrontlineScreenState& S,UFrontlineRenderSettingsSubsystem* R)
{
    auto* PC=S.Owner.Get();float MouseX=0,MouseY=0;int32 W=0,H=0;
    if(!PC || !R || !PC->GetMousePosition(MouseX,MouseY))return false;
    PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1920.f,H/1080.f);if(Scale<=0)return false;
    const float X=(MouseX-(W-1920.f*Scale)*.5f)/Scale;
    return R->SetFrameRateSlider((X-FrameRateSliderX)/FrameRateSliderWidth,false);
}
void TickFrameRateInput(FFrontlineScreenState& S,double Now)
{
    auto* PC=S.Owner.Get();auto* R=PC?UFrontlineRenderSettingsSubsystem::Get(PC->GetWorld()):nullptr;
    if(!PC || !R)return;
    const bool Active=S.Settings && S.SettingsTab==0 && FApp::HasFocus();
    if(S.FrameRateDragging)
    {
        if(Active && PC->IsInputKeyDown(EKeys::LeftMouseButton))UpdateFrameRateFromPointer(S,R);
        else {R->CommitFrameRateLimit();S.FrameRateDragging=false;}
    }
    if(!Active){if(S.FrameRateKeyHeld)R->CommitFrameRateLimit();S.FrameRateFocused=S.FrameRateKeyHeld=false;return;}
    if(!S.FrameRateFocused || S.FrameRateDragging)return;
    const bool Left=PC->IsInputKeyDown(EKeys::Left),Right=PC->IsInputKeyDown(EKeys::Right);
    const bool Home=PC->IsInputKeyDown(EKeys::Home),End=PC->IsInputKeyDown(EKeys::End);
    const bool Held=Left || Right || Home || End;
    if(Held && (!S.FrameRateKeyHeld || Now>=S.FrameRateNextRepeat))
    {
        const auto Range=R->GetFrameRateRange();
        const float Next=Home?float(Range.X):End?float(Range.Y):FMath::Clamp(R->GetFrameRateLimit()+(Right?1.f:-1.f),float(Range.X),float(Range.Y));
        R->SetFrameRateLimit(Next,false);S.FrameRateNextRepeat=Now+(S.FrameRateKeyHeld?.065:.3);
    }
    if(!Held && S.FrameRateKeyHeld)R->CommitFrameRateLimit();
    S.FrameRateKeyHeld=Held;
}

void TickDisplay(FFrontlineScreenState& S)
{
    const double Now=FPlatformTime::Seconds();
    TickFrameRateInput(S,Now);
    if (!S.Performance) { S.PerfLast=0; S.PerfFrames=0; S.PerfFrameSum=0; S.PerfGpuSum=0; S.PerfGpuFrames=0; return; }
    if (S.PerfLast>0)
    {
        const double Ms=(Now-S.PerfLast)*1000.;
        if (Ms>0 && Ms<5000)
        {
            S.PerfFrameSum+=Ms; ++S.PerfFrames; S.FrameHistory.Add(float(Ms));
            if (S.FrameHistory.Num()>180) S.FrameHistory.RemoveAt(0,S.FrameHistory.Num()-180,EAllowShrinking::No);
            const double GpuMs=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
            if (GpuMs>0) { S.PerfGpuSum+=GpuMs; ++S.PerfGpuFrames; }
        }
    }
    S.PerfLast=Now;
    if (Now-S.PerfUpdate>=.4 && S.PerfFrames>0)
    {
        S.PerfFrameMs=float(S.PerfFrameSum/S.PerfFrames);
        S.PerfGpuMs=S.PerfGpuFrames?float(S.PerfGpuSum/S.PerfGpuFrames):0;
        S.PerfMemoryMB=float(FPlatformMemory::GetStats().UsedPhysical/(1024.*1024.));
        if(const auto* R=S.Owner.IsValid()?UFrontlineRenderSettingsSubsystem::Get(S.Owner->GetWorld()):nullptr)
        {const auto Timing=R->GetFrameTiming();S.PerfPresentFPS=Timing.ReportedPresentFPS;S.PerfPresentValid=Timing.bPresentEstimateValid;}
        S.PerfFrames=S.PerfGpuFrames=0; S.PerfFrameSum=S.PerfGpuSum=0; S.PerfUpdate=Now;
    }
}

const TCHAR* WindowModeLabel(EWindowMode::Type M)
{ return M==EWindowMode::Fullscreen?TEXT("全屏"):M==EWindowMode::WindowedFullscreen?TEXT("无边框"):TEXT("窗口"); }

const TCHAR* ImageQualityLabel(EFrontlineRenderQuality Quality)
{
    switch (Quality)
    {
        case EFrontlineRenderQuality::DLAA:return TEXT("DLAA");
        case EFrontlineRenderQuality::Quality:return TEXT("质量");
        case EFrontlineRenderQuality::Balanced:return TEXT("平衡");
        case EFrontlineRenderQuality::Performance:return TEXT("性能");
        default:return TEXT("原生");
    }
}
const TCHAR* GeneratedFramesLabel(EFrontlineFrameGeneration Mode)
{
    switch (Mode)
    {
        case EFrontlineFrameGeneration::X2:return TEXT("2×");
        case EFrontlineFrameGeneration::X3:return TEXT("3×");
        case EFrontlineFrameGeneration::X4:return TEXT("4×");
        case EFrontlineFrameGeneration::X5:return TEXT("5×");
        case EFrontlineFrameGeneration::X6:return TEXT("6×");
        case EFrontlineFrameGeneration::Dynamic:return TEXT("动态");
        default:return TEXT("关闭");
    }
}

bool HandleImageTechnology(UWorld* World,FName Command)
{
    auto* Render=UFrontlineRenderSettingsSubsystem::Get(World);
    if (!Render) return false;
    for(int32 I=0;I<3;++I)
        if(Command==FName(*FString::Printf(TEXT("GraphicsPreset%d"),I)))
        {Render->SetGraphicsPreset(EFrontlineGraphicsPreset(I));return true;}
    for (int32 I=0;I<5;++I)
        if (Command==FName(*FString::Printf(TEXT("ImageQuality%d"),I)))
        {Render->SetQuality(EFrontlineRenderQuality(I));return true;}
    for (int32 I=0;I<7;++I)
        if (Command==FName(*FString::Printf(TEXT("ImageGeneration%d"),I)))
        {Render->SetFrameGeneration(EFrontlineFrameGeneration(I));return true;}
    for (int32 I=0;I<3;++I)
        if (Command==FName(*FString::Printf(TEXT("ImageReflex%d"),I)))
        {Render->SetReflex(EFrontlineReflexMode(I));return true;}
    if (Command==TEXT("ImageReconstruction0") || Command==TEXT("ImageReconstruction1"))
    {Render->SetRayReconstruction(Command==TEXT("ImageReconstruction1"));return true;}
    return false;
}

bool HandleHDRSettings(UWorld* World,FName Command)
{
    auto* R=UFrontlineRenderSettingsSubsystem::Get(World);if(!R)return false;
    const auto H=R->GetHDRStatus();
    if(Command==TEXT("HDR0") || Command==TEXT("HDR1")){R->SetHDREnabled(Command==TEXT("HDR1"));return true;}
    if(Command==TEXT("HDRReset")){R->ResetHDRCalibration();return true;}
    const FString N=Command.ToString();
    const float Sign=N.EndsWith(TEXT("Up"))?1.f:-1.f;
    if(N.StartsWith(TEXT("HDRPeak"))){R->SetHDRPeakNits(H.PeakNits+int32(Sign*50));return true;}
    if(N.StartsWith(TEXT("HDRWhite"))){R->SetHDRPaperWhiteNits(H.PaperWhiteNits+Sign*10);return true;}
    if(N.StartsWith(TEXT("HDRUI"))){R->SetHDRUILuminanceNits(H.UILuminanceNits+Sign*10);return true;}
    if(N.StartsWith(TEXT("HDRBlack"))){R->SetHDRBlackNits(H.BlackNits*(Sign>0?2.f:.5f));return true;}
    return false;
}
bool HandleFrameRate(FFrontlineScreenState& S,UWorld* World,FName Command)
{
    if(Command!=TEXT("FrameRatePrev") && Command!=TEXT("FrameRateNext") && Command!=TEXT("FrameRateSlider"))return false;
    auto* R=UFrontlineRenderSettingsSubsystem::Get(World);if(!R)return true;
    S.FrameRateFocused=true;
    if(Command==TEXT("FrameRateSlider")){S.FrameRateDragging=true;UpdateFrameRateFromPointer(S,R);return true;}
    const auto Range=R->GetFrameRateRange();
    R->SetFrameRateLimit(FMath::Clamp(R->GetFrameRateLimit()+(Command==TEXT("FrameRateNext")?1.f:-1.f),float(Range.X),float(Range.Y)));return true;
}

FString HDRCalibrationTip(int32 Index,const FFrontlineHDRStatus& H)
{
    switch(Index)
    {
    case 0:return FString::Printf(TEXT("当前 %.0f nit，是游戏最亮高光的显示目标。1 nit = 1 cd/m²。\n\n保持平时的屏幕亮度和局部调光设置，以显示器或系统 HDR 校准的峰值起步。左侧亮块的十字比背景暗 10%%；逐步调到十字刚好消失。若始终可见，屏幕可能在映射亮部，请沿用已校准的峰值，不必一直推高。\n\n200–4000 nit；每次调整 50 nit。"),float(H.PeakNits));
    case 1:return FString::Printf(TEXT("当前 %.0f nit，决定普通白色和漫反射物体的亮度基准，不是火光的最高亮度。%s\n\n默认跟随当前屏幕的 Windows 白亮度，无法读取时使用 203 nit。峰值亮度不能代替纸白。手动调整后会为这块屏幕保留，恢复推荐值可再次跟随系统。\n\n白色应自然舒适，块内十字应能分辨，不要以让它消失为目标。80–1000 nit，最高为屏幕峰值的 80%%；每次调整 10 nit。"),H.PaperWhiteNits,H.bPaperWhiteAutomatic?TEXT("目前自动跟随系统。"):TEXT("目前使用手动校准。"));
    case 2:return FString::Printf(TEXT("当前 %.0f nit，单独控制文字、照片和操作台的亮度；不改变战场曝光或高光峰值。%s\n\n默认跟随当前屏幕的 Windows 白亮度，读取失败时使用 203 nit。先把画面纸白调舒服，再看本页文字与战场操作台。降低刺眼的字和亮边，提高到文字清楚即可。\n\n每块屏幕独立保留手动值。80–1000 nit，最高为屏幕峰值的 80%%；每次调整 10 nit。"),H.UILuminanceNits,H.bUIWhiteAutomatic?TEXT("目前自动跟随系统。"):TEXT("目前使用手动校准。"));
    default:return FString::Printf(TEXT("右侧参照块为 %.4f nit，十字为 %.4f nit。它只用来观察屏幕的近黑层次，不改变战场曝光或黑位。\n\n从接近纯黑开始，看十字是否仍能辨认。若暗部被室内反光盖住，先调整环境；不要用这个参照去抬亮整幅游戏。\n\n0.0001–0.1 nit；每次减半或加倍。"),H.BlackNits,FMath::Max(.005f,H.BlackNits*2));
    }
}

void DrawSettingsHelpV73(const FFrontlineCanvas& UI,const FString& Title,const FString& Copy)
{
    UI.Label(Title,1296,282,512,44,26,Paper);
    UI.Paragraph(Copy,1296,346,512,430,19,Muted,true);
}

// Settings need a persistent selection cue, distinct from pointer hover. The
// key is the existing national enamel object: relief and paint value convey
// its state while keeping the caption field free of extra symbols.
FLinearColor SettingsFaceV831(bool Enabled,bool Selected,bool Hover)
{return !Enabled?FLinearColor(.40f,.43f,.44f,1):Selected?FLinearColor(1.28f,1.21f,1.05f,1)
    :Hover?FLinearColor(.96f,.99f,.98f,1):FLinearColor(.61f,.67f,.68f,1);}
namespace SettingsGeometryV831
{
    uint64 Frame=MAX_uint64;TArray<TSharedPtr<FJsonValue>> Rows;
    void Record(FName Name,float X,float Y,float W,float H,bool Enabled,bool Selected,bool Hover,bool Tab,const FString& Caption)
    {
        static const bool Review=FParse::Param(FCommandLine::Get(),TEXT("WFV831Review"));if(!Review)return;
        if(Frame!=GFrameCounter){Frame=GFrameCounter;Rows.Empty();}
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("command"),Name.ToString());
        R->SetNumberField(TEXT("x"),X);R->SetNumberField(TEXT("y"),Y);R->SetNumberField(TEXT("w"),W);R->SetNumberField(TEXT("h"),H);
        R->SetBoolField(TEXT("enabled"),Enabled);R->SetBoolField(TEXT("selected"),Selected);R->SetBoolField(TEXT("hover"),Hover);R->SetBoolField(TEXT("tab"),Tab);
        R->SetStringField(TEXT("caption"),Caption);
        Rows.Add(MakeShared<FJsonValueObject>(R));
    }
}
void SettingsKeyV831(const FFrontlineCanvas& UI,float X,float Y,float W,float H,const FString& Value,const FString& Key,FName Name,
    bool Enabled=true,bool Selected=false,int32 Priority=50,bool Tab=false,bool LiteralCaption=false)
{
    const bool Hover=Enabled&&(!UI.bSuppressBackgroundHits||Priority>=40)&&UI.Hovered(X,Y,W,H);
    const bool Pressed=Hover&&UI.HUD.GetOwningPlayerController()&&UI.HUD.GetOwningPlayerController()->IsInputKeyDown(EKeys::LeftMouseButton);
    const float PX=X+(Pressed?1.f:0.f),PY=Y+(Tab&&!Selected?4.f:0.f)+(Pressed?2.f:0.f);
    const float PW=W-(Pressed?1.f:0.f),PH=H-(Tab&&!Selected?4.f:0.f)-(Pressed?2.f:0.f);
    if(!Pressed)UI.PageShadowV74(PX,PY,PW,PH,Selected?1.f:3.f);
    const FLinearColor Face=SettingsFaceV831(Enabled,Selected,Hover);
    if(!UI.ConsoleSliceV80(Tab?3:2,PX,PY,PW,PH,FMath::Min(12.f,PH*.21f),UI.ConsoleTintV79()*Face))
        UI.Skin(3,PX,PY,PW,PH,9,Face);
    if(Selected)
    {
        const float Lip=8.f;
        UI.Gradient(PX+Lip,PY+Lip,PW-Lip*2,7,FLinearColor(0,0,0,.50f),FLinearColor(0,0,0,0),true);
        UI.Line(PX+Lip,PY+PH-Lip,PX+PW-Lip,PY+PH-Lip,FLinearColor(.68f,.59f,.39f,.84f),1.5f);
    }
    const auto Ink=!Enabled?Muted*.75f:Selected?FLinearColor(.95f,.91f,.80f,1):Hover?Paper:FLinearColor(.65f,.69f,.67f,1);
    FString PaintedCaption;
    {
        // Language names are written in their own language. Only this caption
        // bypasses lookup; other labels keep the normal localization path.
        TGuardValue<bool> Literal(UI.bResolvedTextV8210,UI.bResolvedTextV8210||LiteralCaption);
        // Headline keeps the same control face without the plate's uppercase
        // transformation, preserving the exact endonym "English".
        const auto CaptionRole=LiteralCaption?EFrontlineTextRoleV8210::Headline:
            UI.bLegacyTypographyV8210?EFrontlineTextRoleV8210::Auto:EFrontlineTextRoleV8210::Plate;
        PaintedCaption=UI.RoleValueV8210(CaptionRole,Value);
        UI.Label(Value,X+16,PY,W-32,PH,Tab?22.f:H>=56?21.f:18.f,Ink,1,false,
            CaptionRole);
    }
    SettingsGeometryV831::Record(Name,PX,PY,PW,PH,Enabled,Selected,Hover,Tab,PaintedCaption);
    UI.Shortcut(X,Y,W,H,Key,Priority);UI.Hit(X,Y,W,H,Enabled?Name:FName(TEXT("Blocked")),Priority);
}
void SettingsTabV831(const FFrontlineCanvas& UI,float X,float Y,float W,const FString& Value,FName Name,bool Selected,bool Enabled=true,int32 Priority=50)
{SettingsKeyV831(UI,X,Y,W,PageCanvasV73::TabH,Value,TEXT(""),Name,Enabled,Selected,Priority,true);}

void DrawHDRSettings(const FFrontlineCanvas& UI)
{
    auto* R=UFrontlineRenderSettingsSubsystem::Get(UI.HUD.GetWorld());
    const auto H=R?R->GetHDRStatus():FFrontlineHDRStatus();
    if(R)R->SetHDRCalibrationVisible(true);
    UI.Label(PageCanvasV73::L(TEXT("Display output"),TEXT("显示输出")),112,284,350,44,25,Paper);
    UI.Label(H.OutputLabel,484,286,704,40,21,H.bOutputActive?Allied:Muted,2);
    const FString Monitor=H.bSupported&&!H.DisplayName.IsEmpty()?H.DisplayName:H.Reason;
    UI.Paragraph(Monitor,112,332,650,44,18,Muted);
    SettingsKeyV831(UI,794,332,186,48,PageCanvasV73::L(TEXT("OFF"),TEXT("关闭")),TEXT(""),TEXT("HDR0"),R!=nullptr,!H.bEnabled);
    SettingsKeyV831(UI,996,332,192,48,PageCanvasV73::L(TEXT("ENABLE HDR"),TEXT("开启 HDR")),TEXT(""),TEXT("HDR1"),H.bSupported,H.bEnabled);
    const TCHAR* Titles[]={TEXT("峰值亮度"),TEXT("画面纸白"),TEXT("界面亮度"),TEXT("暗部参照")};
    const TCHAR* Keys[]={TEXT("HDRPeak"),TEXT("HDRWhite"),TEXT("HDRUI"),TEXT("HDRBlack")};
    const float Values[]={float(H.PeakNits),H.PaperWhiteNits,H.UILuminanceNits,H.BlackNits};
    int32 HoveredCalibration=INDEX_NONE;
    for(int32 I=0;I<4;++I)
    {
        const float Y=398+I*76.f;
        UI.PageRuleV74(112,Y-10,1076);
        UI.Label(Titles[I],112,Y,452,54,24,Paper);
        SettingsKeyV831(UI,660,Y,64,54,TEXT("−"),TEXT(""),FName(*(FString(Keys[I])+TEXT("Down"))),H.bOutputActive);
        UI.Label(I==3?FString::Printf(TEXT("%.4f nit"),Values[I]):FString::Printf(TEXT("%.0f nit"),Values[I]),746,Y,366,54,24,Paper,1);
        SettingsKeyV831(UI,1124,Y,64,54,TEXT("+"),TEXT(""),FName(*(FString(Keys[I])+TEXT("Up"))),H.bOutputActive);
        if(UI.Hovered(112,Y,1076,64))HoveredCalibration=I;
    }
    UI.Label(H.bOutputActive?PageCanvasV73::L(TEXT("True HDR output · observe the screen directly"),TEXT("真实 HDR 输出 · 请直接观察屏幕")):PageCanvasV73::L(TEXT("Enable HDR to calibrate the display"),TEXT("开启 HDR 后可校准屏幕")),112,702,1076,30,18,Muted);
    for(int32 I=0;I<3;++I)
    {
        UI.Rect(112+I*364,744,340,100,FLinearColor(.02f,.02f,.02f,1));
        UI.Label(I==0?TEXT("亮部"):I==1?TEXT("纸白"):TEXT("暗部"),112+I*364,856,340,32,18,Muted,1);
    }
    DrawSettingsHelpV73(UI,HoveredCalibration==INDEX_NONE?PageCanvasV73::L(TEXT("HDR calibration"),TEXT("HDR 校准")):FrontlineLocalizationV35::Text(Titles[HoveredCalibration]),
        HoveredCalibration==INDEX_NONE?PageCanvasV73::L(TEXT("Calibrate for the display you use to play.\n\nKeep your usual screen brightness and local dimming. Begin with the peak brightness from your monitor or Windows HDR calibration.\n\nHover each brightness row for its measurement and calibration instructions. Manual values are retained for this display.\n\nSet paper white to a comfortable ordinary white, then adjust UI luminance for clear, comfortable text."),TEXT("按实际游玩的屏幕校准。\n\n保持平时的屏幕亮度和局部调光设置，以显示器或 Windows HDR 校准的峰值起步。\n\n悬停每一行可阅读数值含义及校准方法。手动值按屏幕分别保留。\n\n先把画面纸白调到自然舒适，再调整界面亮度，让文字清楚而不刺眼。")):HDRCalibrationTip(HoveredCalibration,H));
    SettingsKeyV831(UI,754,958,432,58,TEXT("恢复屏幕推荐值"),TEXT(""),TEXT("HDRReset"),H.bSupported);
}

void DrawPerformance(const FFrontlineCanvas& UI,const FFrontlineScreenState& S)
{
    if (!S.Performance) return;
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(UI.HUD.GetWorld()->GetAuthGameMode());
    if(S.Preparing||S.Settings||S.Technology||S.Menu||S.Help||!Battle||Battle->Phase!=EFrontlinePhase::Fighting)
    {
        UI.Metal(1420,12,476,42,true);
        UI.Text(FString::Printf(TEXT("RENDER %.0f FPS  /  %.2f MS"),S.PerfFrameMs>0?1000.f/S.PerfFrameMs:0,S.PerfFrameMs),1434,26,14,Paper);
        UI.Right(S.PerfPresentValid?FString::Printf(TEXT("PRESENT %.0f"),S.PerfPresentFPS):TEXT("PRESENT —"),1882,26,14,Muted);return;
    }
    const auto* Mission=Battle->GetEncounterCampaignMission();
    const float X=23,Y=Mission&&Mission->Objective==EFrontlineCampaignObjective::HoldCenter?138.f:88.f,W=360,H=220;
    UI.Metal(X,Y,W,H,true);
    UI.Stamped(TEXT("PERFORMANCE"),X+12,Y+10,15,Muted);
    UI.Shortcut(X,Y,W,29,TEXT("F10"));
    UI.Text(S.PerfFrameMs>0?FString::Printf(TEXT("%.0f 渲染 FPS"),1000.f/S.PerfFrameMs):TEXT("— 渲染 FPS"),X+12,Y+33,23,Paper);
    UI.Right(FString::Printf(TEXT("%.2f ms"),S.PerfFrameMs),X+W-12,Y+39,16,Amber);
    UI.Text(S.PerfPresentValid?FString::Printf(TEXT("%.0f 呈现 FPS"),S.PerfPresentFPS):TEXT("— 呈现 FPS"),X+12,Y+65,20,Paper);
    UI.Right(TEXT("含生成帧"),X+W-12,Y+70,13,Muted);
    UI.Text(FString::Printf(TEXT("GPU  %.2f ms    RAM  %.0f MB"),S.PerfGpuMs,S.PerfMemoryMB),X+12,Y+99,13,Muted);
    const auto* Render=UFrontlineRenderSettingsSubsystem::Get(UI.HUD.GetWorld());
    const auto Quality=Render?Render->GetQuality():EFrontlineRenderQuality::Native;
    const FString QualityText=Quality==EFrontlineRenderQuality::Native?TEXT("原生"):
        Quality==EFrontlineRenderQuality::DLAA?TEXT("DLAA"):FString::Printf(TEXT("DLSS · %s"),ImageQualityLabel(Quality));
    UI.Text(QualityText,X+12,Y+123,14,Muted);
    UI.Right(FString::Printf(TEXT("%d × %d"),int32(UI.Surface->ClipX),int32(UI.Surface->ClipY)),X+W-12,Y+123,13,Muted);
    const auto Requested=Render?Render->GetFrameGeneration():EFrontlineFrameGeneration::Off;
    const auto Effective=Render?Render->GetEffectiveFrameGeneration():EFrontlineFrameGeneration::Off;
    const FString Generation=Requested!=EFrontlineFrameGeneration::Off && Effective==EFrontlineFrameGeneration::Off
        ?FString::Printf(TEXT("帧生成 %s · 暂停"),GeneratedFramesLabel(Requested))
        :FString::Printf(TEXT("帧生成 %s"),GeneratedFramesLabel(Effective));
    UI.Text(Generation,X+12,Y+148,13,Muted);
    const float GX=X+161,GY=Y+183,GW=183,GH=28;
    UI.Line(GX,GY-GH*.5f,GX+GW,GY-GH*.5f,Border);
    for (int32 I=1;I<S.FrameHistory.Num();++I)
        UI.Line(GX+(I-1)*GW/179.f,GY-FMath::Min(S.FrameHistory[I-1]/33.333f,1.f)*GH,
            GX+I*GW/179.f,GY-FMath::Min(S.FrameHistory[I]/33.333f,1.f)*GH,Allied,1);
    UI.Text(TEXT("帧耗时"),X+12,Y+175,12,Muted);
    UI.Text(TEXT("呈现帧率为 NVIDIA 插件估算 · GPU 延迟采样"),X+12,Y+200,11,Muted);
}

void DrawImageTechnology(const FFrontlineCanvas& UI)
{
    const auto* Render=UFrontlineRenderSettingsSubsystem::Get(UI.HUD.GetWorld());
    const FFrontlineRenderCapabilities Caps=Render?Render->GetCapabilities():FFrontlineRenderCapabilities();
    FString Hint,Title=PageCanvasV73::L(TEXT("Graphics"),TEXT("画面技术"));
    auto Explain=[&](float X,float Y,float W,float H,bool Enabled,const FString& Reason,const FString& Usage,const FString& Heading)
    {
        if(UI.Hovered(X,Y,W,H)){Title=Heading;Hint=Enabled?Usage:Reason.IsEmpty()?PageCanvasV73::L(TEXT("Unavailable on this device or display mode."),TEXT("当前设备或显示模式不支持此选项。")):Reason;}
    };
    const bool English=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English;
    const TCHAR* PresetsEN[]={TEXT("LOW"),TEXT("MEDIUM"),TEXT("HIGH")};
    const TCHAR* PresetsZH[]={TEXT("低"),TEXT("中"),TEXT("高")};
    const TCHAR* TipsEN[]={TEXT("For integrated graphics: simple lighting, lighter shadows and smoke. Start at 1280 × 720. Combat rules and visibility remain identical."),TEXT("Balanced lighting, screen-space reflections and reduced effects. Start at 1920 × 1080 on an RTX 3060-class GPU."),TEXT("Full terrain detail, Lumen illumination, ray-traced reflections on supported hardware and the complete effects budget.")};
    const TCHAR* TipsZH[]={TEXT("适合核显：简化光照、阴影和烟雾。建议从 1280 × 720 开始；战斗规则和视野完全相同。"),TEXT("兼顾光照、屏幕空间反射和特效。RTX 3060 级显卡建议从 1920 × 1080 开始。"),TEXT("完整地形细节、Lumen 光照、受支持硬件上的光追反射和完整特效。")};
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,PageCanvasV73::L(TEXT("Scene quality"),TEXT("场景画质")),112,288,360,46,25,Paper);
    for(int32 I=0;I<3;++I)
    {
        const float X=660+I*180.f;
        SettingsKeyV831(UI,X,284,168,54,English?PresetsEN[I]:PresetsZH[I],TEXT(""),FName(*FString::Printf(TEXT("GraphicsPreset%d"),I)),Render!=nullptr,Render&&int32(Render->GetGraphicsPreset())==I);
        Explain(X,284,168,54,Render!=nullptr,TEXT(""),English?TipsEN[I]:TipsZH[I],PageCanvasV73::L(TEXT("Scene quality"),TEXT("场景画质")));
    }
    UI.PageRuleV74(112,364,1076);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("DLSS 与抗锯齿"),112,384,1076,38,25,Paper);
    const TCHAR* QualityTips[]={TEXT("按输出分辨率渲染"),TEXT("保持原生分辨率，使用 DLAA 抗锯齿"),TEXT("DLSS 质量模式，优先保留细节"),TEXT("DLSS 平衡模式，兼顾细节与速度"),TEXT("DLSS 性能模式，进一步降低渲染负担")};
    for(int32 I=0;I<5;++I)
    {
        const auto Mode=EFrontlineRenderQuality(I);const float X=112+I*218.f,Y=438;
        const bool Enabled=Render&&Caps.Qualities.Contains(Mode);
        SettingsKeyV831(UI,X,Y,204,56,ImageQualityLabel(Mode),TEXT(""),FName(*FString::Printf(TEXT("ImageQuality%d"),I)),Enabled,Render&&Render->GetQuality()==Mode);
        Explain(X,Y,204,56,Enabled,Caps.SuperResolutionReason,QualityTips[I],TEXT("DLSS 与抗锯齿"));
    }
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("帧生成"),112,524,1076,38,25,Paper);
    for(int32 I=0;I<7;++I)
    {
        const auto Mode=EFrontlineFrameGeneration(I);const float X=112+I*156.f,Y=578;
        const bool Enabled=Render&&Caps.FrameGenerationModes.Contains(Mode);
        SettingsKeyV831(UI,X,Y,140,56,GeneratedFramesLabel(Mode),TEXT(""),FName(*FString::Printf(TEXT("ImageGeneration%d"),I)),Enabled,Render&&Render->GetFrameGeneration()==Mode);
        Explain(X,Y,140,56,Enabled,Caps.FrameGenerationReason,I==0?TEXT("仅显示实际渲染帧"):TEXT("在渲染帧之间生成补充画面；不增加游戏模拟帧率，暂停与菜单中暂缓"),TEXT("帧生成"));
    }
    UI.PageRuleV74(112,666,1076);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("NVIDIA Reflex"),112,690,460,54,25,Paper);
    const TCHAR* ReflexLabels[]={TEXT("关闭"),TEXT("开启"),TEXT("开启并增强")};
    for(int32 I=0;I<3;++I)
    {
        const auto Mode=EFrontlineReflexMode(I);const float X=660+I*180.f,Y=690;
        const bool Required=Render&&Render->GetFrameGeneration()!=EFrontlineFrameGeneration::Off;
        const bool Enabled=Render&&Caps.ReflexModes.Contains(Mode)&&!(I==0&&Required);
        SettingsKeyV831(UI,X,Y,168,54,ReflexLabels[I],TEXT(""),FName(*FString::Printf(TEXT("ImageReflex%d"),I)),Enabled,Render&&Render->GetReflex()==Mode);
        Explain(X,Y,168,54,Enabled,I==0&&Required?FString(TEXT("帧生成需要开启 Reflex")):Caps.ReflexReason,I==2?TEXT("降低延迟并保持较高显卡工作频率，可能增加功耗"):TEXT("减少渲染队列延迟"),TEXT("NVIDIA Reflex"));
    }
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("光线重建"),112,802,460,54,25,Paper);
    for(int32 I=0;I<2;++I)
    {
        const float X=832+I*184.f,Y=802;
        const bool Native=Render&&Render->GetQuality()==EFrontlineRenderQuality::Native;
        const bool High=Render&&Render->GetGraphicsPreset()==EFrontlineGraphicsPreset::High;
        const bool Enabled=Render&&(I==0||(Caps.bRayReconstruction&&!Native&&High));
        SettingsKeyV831(UI,X,Y,172,54,I?TEXT("开启"):TEXT("关闭"),TEXT(""),FName(*FString::Printf(TEXT("ImageReconstruction%d"),I)),Enabled,Render&&Render->GetRayReconstruction()==bool(I));
        const FString Reason=!High?PageCanvasV73::L(TEXT("Ray Reconstruction is used with High scene quality."),TEXT("光线重建在高画质下使用。")):I&&Native&&Caps.bRayReconstruction?FString(TEXT("请先选择 DLSS 或 DLAA")):Caps.RayReconstructionReason;
        Explain(X,Y,172,54,Enabled,Reason,TEXT("使用 DLSS 光线重建改善光照与反射细节"),TEXT("光线重建"));
    }
    DrawSettingsHelpV73(UI,Title,Hint.IsEmpty()?PageCanvasV73::L(TEXT("Choose a scene quality for your hardware, then configure anti-aliasing and frame generation.\n\nChanges are visible immediately. Hover an unavailable option to read its requirements.\n\nFrame generation adds presentation frames. Reflex improves responsiveness; it remains required while frame generation is enabled."),TEXT("先按硬件选择场景画质，再设置抗锯齿与帧生成。\n\n调整后可立即看到变化。暂不可用的选项会在悬停时说明所需条件。\n\n帧生成增加呈现画面。Reflex 让操作响应更快；开启帧生成时需要保持 Reflex 开启。")):Hint);
}

void DrawDisplaySettings(const FFrontlineCanvas& UI,const FFrontlineScreenState& S)
{
    if(S.SettingsTab!=3)if(auto* R=UFrontlineRenderSettingsSubsystem::Get(UI.HUD.GetWorld()))R->SetHDRCalibrationVisible(false);
    UI.PageSurfaceV73(0,0,1920,1080,false,false);
    UI.PhysicalPanelV74(80,252,1152,676);
    UI.PhysicalPanelV74(1272,252,552,676,true);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    UI.Label(PageCanvasV73::L(TEXT("FIELD SETTINGS"),TEXT("游戏设置")),72,64,1210,65,44,Paper);
    UI.Label(PageCanvasV73::L(TEXT("Display, sound and image preferences"),TEXT("显示、声音与画面偏好")),76,124,1160,30,19,Muted);
    SettingsKeyV831(UI,1448,80,172,46,TEXT("English"),TEXT(""),TEXT("LanguageEnglish"),true,FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English,50,false,true);
    SettingsKeyV831(UI,1636,80,172,46,TEXT("简体中文"),TEXT(""),TEXT("LanguageChinese"),true,FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese,50,false,true);
    const TCHAR* LabelsEN[]={TEXT("DISPLAY"),TEXT("AUDIO"),TEXT("GRAPHICS"),TEXT("HDR")};
    const TCHAR* LabelsZH[]={TEXT("显示"),TEXT("声音"),TEXT("画面技术"),TEXT("HDR")};
    const TCHAR* Tabs[]={TEXT("SettingsVideo"),TEXT("SettingsAudio"),TEXT("SettingsImage"),TEXT("SettingsHDR")};
    for(int32 I=0;I<4;++I)SettingsTabV831(UI,72+I*284.f,PageCanvasV73::TabY,272,PageCanvasV73::L(LabelsEN[I],LabelsZH[I]),FName(Tabs[I]),S.SettingsTab==I);
    SettingsKeyV831(UI,72,958,280,58,TEXT("返回"),TEXT("Esc"),TEXT("DisplayBack"));
    const auto& Update=FrontlineUpdatesV36::Get();
    UI.Label(FString::Printf(TEXT("当前版本 %s"),FrontlineUpdatesV36::CurrentTag()),1296,835,512,24,17,Muted);
    SettingsKeyV831(UI,1296,874,244,42,TEXT("检查更新"),TEXT(""),TEXT("UpdateCheck"),Update.Status!=FrontlineUpdatesV36::EStatus::Checking);
    SettingsKeyV831(UI,1556,874,252,42,TEXT("打开发行页面"),TEXT(""),TEXT("UpdateOpen"));
    FString UpdateText;
    switch(Update.Status)
    {
        case FrontlineUpdatesV36::EStatus::Checking:UpdateText=TEXT("正在查询 GitHub 最新发行版…");break;
        case FrontlineUpdatesV36::EStatus::Current:UpdateText=TEXT("当前版本已是最新。");break;
        case FrontlineUpdatesV36::EStatus::Available:UpdateText=FString::Printf(TEXT("新版本 %s 已发布。"),*Update.LatestTag);break;
        case FrontlineUpdatesV36::EStatus::Offline:UpdateText=TEXT("暂时无法连接，请稍后重试。");break;
        case FrontlineUpdatesV36::EStatus::RateLimited:UpdateText=TEXT("GitHub 暂时限制查询，请稍后重试。");break;
        default:UpdateText=TEXT("在 GitHub 查看新版与发行说明。");break;
    }
    // The idle action is already named on the key; result messages belong on
    // the reading card, clear of its seam and the footer's woodwork.
    if(Update.Status!=FrontlineUpdatesV36::EStatus::Idle)UI.Label(UpdateText,1296,798,512,28,16,Muted);
    if(S.SettingsTab==2){DrawImageTechnology(UI);return;}
    if(S.SettingsTab==3){DrawHDRSettings(UI);return;}
    if(S.SettingsTab==1)
    {
        auto* Audio=UI.HUD.GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        const TCHAR* BusLabels[]={TEXT("总音量"),TEXT("枪炮与战斗"),TEXT("战场呼喊"),TEXT("风声与远景"),TEXT("配乐")};
        for(int32 Bus=0;Bus<5;++Bus)
        {
            const float Y=288+Bus*98.f,Volume=Audio?Audio->GetVolume(EFrontlineAudioBus(Bus)):0;
            UI.Label(BusLabels[Bus],112,Y,440,56,25,Paper);
            SettingsKeyV831(UI,602,Y,56,56,TEXT("−"),TEXT(""),FName(*FString::Printf(TEXT("AudioDown%d"),Bus)),Audio!=nullptr);
            UI.PageSliderV74(682,Y+24,370,Volume,false,Audio&&Audio->IsMuted());
            for(int32 Step=0;Step<=10;++Step)
            {
                UI.Line(682+Step*37,Y+38,682+Step*37,Y+43,Muted);
                UI.Hit(664+Step*37,Y+3,36,50,FName(*FString::Printf(TEXT("AudioLevel%d_%d"),Bus,Step)),50);
            }
            SettingsKeyV831(UI,1082,Y,56,56,TEXT("+"),TEXT(""),FName(*FString::Printf(TEXT("AudioUp%d"),Bus)),Audio!=nullptr);
            UI.Label(FString::Printf(TEXT("%d%%"),FMath::RoundToInt(Volume*100)),1146,Y,60,56,21,Muted,2);
            UI.PageRuleV74(112,Y+77,1076);
        }
        UI.Label(TEXT("静音"),112,802,440,56,25,Paper);
        SettingsKeyV831(UI,832,802,356,56,Audio&&Audio->IsMuted()?TEXT("开"):TEXT("关"),TEXT(""),TEXT("AudioMute"),Audio!=nullptr,Audio&&Audio->IsMuted());
        DrawSettingsHelpV73(UI,PageCanvasV73::L(TEXT("Sound mix"),TEXT("声音设置")),PageCanvasV73::L(TEXT("Set separate levels for combat, voices, ambience and music.\n\nClick a mark on the scale or use the − / + buttons.\n\nMaster volume controls all sound. Mute preserves your levels for when sound is enabled again."),TEXT("分别调整战斗声、呼喊、环境声与配乐。\n\n点击刻度，或使用 − / + 调整音量。\n\n总音量控制所有声音。静音保留各组设置，重新开启后恢复原来的音量。")));
        return;
    }
    UI.Label(TEXT("分辨率"),112,288,452,56,25,Paper);
    const bool CanChange=S.DisplayMode!=EWindowMode::WindowedFullscreen;
    SettingsKeyV831(UI,660,288,64,56,TEXT("‹"),TEXT(""),TEXT("ResolutionPrev"),CanChange);
    UI.Label(FString::Printf(TEXT("%d × %d"),S.DisplayResolution.X,S.DisplayResolution.Y),744,288,360,56,25,Paper,1);
    SettingsKeyV831(UI,1124,288,64,56,TEXT("›"),TEXT(""),TEXT("ResolutionNext"),CanChange);
    UI.PageRuleV74(112,372,1076);
    UI.Label(TEXT("显示模式"),112,400,452,56,25,Paper);
    const EWindowMode::Type Modes[]={EWindowMode::Windowed,EWindowMode::WindowedFullscreen,EWindowMode::Fullscreen};
    for(int32 I=0;I<3;++I)SettingsKeyV831(UI,660+I*180.f,400,168,56,WindowModeLabel(Modes[I]),TEXT(""),FName(*FString::Printf(TEXT("WindowMode%d"),I)),true,S.DisplayMode==Modes[I]);
    UI.PageRuleV74(112,484,1076);
    UI.Label(TEXT("帧率上限"),112,516,452,56,25,Paper);
    const auto* Render=UFrontlineRenderSettingsSubsystem::Get(UI.HUD.GetWorld());
    const float Limit=Render?Render->GetFrameRateLimit():0;
    const auto Range=Render?Render->GetFrameRateRange():FIntPoint(30,120);
    const float Position=Range.Y>Range.X?FMath::Clamp((Limit-Range.X)/(Range.Y-Range.X),0.f,1.f):1.f;
    UI.Label(FString::Printf(TEXT("%.0f FPS"),Limit),988,516,200,40,22,Paper,2);
    SettingsKeyV831(UI,582,558,56,48,TEXT("−"),TEXT(""),TEXT("FrameRatePrev"),Render&&Limit>Range.X);
    UI.PageSliderV74(FrameRateSliderX,578,FrameRateSliderWidth,Position,S.FrameRateFocused);
    if(Render)UI.Hit(FrameRateSliderX-8,556,FrameRateSliderWidth+16,52,TEXT("FrameRateSlider"),51);
    SettingsKeyV831(UI,1132,558,56,48,TEXT("+"),TEXT(""),TEXT("FrameRateNext"),Render&&Limit<Range.Y);
    const auto Display=Render?Render->GetActiveDisplay():FFrontlineActiveDisplay();
    UI.Label(Display.bRefreshKnown?FString::Printf(TEXT("当前屏幕 %.2f Hz · %d–%d FPS"),Display.RefreshHz,Range.X,Range.Y):TEXT("屏幕刷新率暂不可读 · 暂用安全上限"),662,619,526,30,18,Muted);
    UI.Shortcut(582,516,606,133,TEXT("← / →  ·  Home / End"),50);
    UI.PageRuleV74(112,674,1076);
    UI.Label(TEXT("性能监视器"),112,696,452,56,25,Paper);
    SettingsKeyV831(UI,832,696,356,56,S.Performance?TEXT("开"):TEXT("关"),TEXT("F10"),TEXT("Performance"),true,S.Performance);
    UI.Label(TEXT("胶片颗粒"),112,802,452,56,25,Paper);
    SettingsKeyV831(UI,832,802,356,56,FrontlineFilmV32::IsEnabled()?TEXT("开"):TEXT("关"),TEXT(""),TEXT("FilmGrain"),true,FrontlineFilmV32::IsEnabled());
    FString Help=PageCanvasV73::L(TEXT("Choose a resolution and display mode, then confirm once to apply and save them.\n\nBorderless mode uses the current display's desktop resolution.\n\nThe frame limit follows the active display range. Drag the slider, or focus it and use arrow keys, Home and End.\n\nThe performance monitor separates rendered and presented frames."),TEXT("选择分辨率与显示模式，确认一次即可应用并保存。\n\n无边框模式使用当前屏幕的桌面分辨率。\n\n帧率范围跟随当前屏幕。拖动滑杆，或选中滑杆后使用方向键、Home 和 End。\n\n性能监视器区分渲染帧率与呈现帧率。"));
    if(UI.Hovered(112,794,1076,72))Help=TEXT("细微的明暗颗粒保留战地胶片质感，只作用于战场，不覆盖界面文字。");
    DrawSettingsHelpV73(UI,UI.Hovered(112,794,1076,72)?FString(TEXT("胶片颗粒")):PageCanvasV73::L(TEXT("Display preferences"),TEXT("显示偏好")),Help);
    SettingsKeyV831(UI,754,958,432,58,PageCanvasV73::L(TEXT("APPLY DISPLAY"),TEXT("应用显示设置")),TEXT(""),TEXT("VideoApply"),true,true);
}
