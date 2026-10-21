#include "WarfareFrontlineV25HDRReview.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Async/Async.h"
#include "CanvasItem.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Canvas.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSafeCounter.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Rendering/SlateRenderer.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/SlateViewportProvider.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

namespace FrontlineHDRReviewV25
{
namespace
{
FThreadSafeCounter TileDraws;
constexpr int32 PhaseCount=7;
const TCHAR* PhaseNames[PhaseCount]={TEXT("01-sdr"),TEXT("02-hdr-ui203-paper203"),TEXT("03-hdr-ui100-paper203"),
    TEXT("04-hdr-ui100-paper100"),TEXT("05-hdr-ui100-paper203-return"),TEXT("06-sdr-return"),TEXT("07-hdr-windows-white-auto")};
const TCHAR* TileNames[5]={TEXT("white"),TEXT("gray50"),TEXT("red"),TEXT("green"),TEXT("blue")};
const FLinearColor TileColors[5]={FLinearColor::White,FLinearColor(.5f,.5f,.5f,1),FLinearColor::Red,FLinearColor::Green,FLinearColor::Blue};

bool Enabled(){return FParse::Param(FCommandLine::Get(),TEXT("WFV25HDRReview"));}
double DecodePQ(double Code)
{
    const double P=FMath::Pow(FMath::Clamp(Code,0.,1.),32./2523.);
    return 10000.*FMath::Pow(FMath::Max(P-3424./4096.,0.)/FMath::Max(2413./128.-2392./128.*P,1.e-12),16384./2610.);
}
double DecodeSRGB(double V){return V<=.04045?V/12.92:FMath::Pow((V+.055)/1.055,2.4);}
FVector3d Nits(const FLinearColor& V,int32 Format)
{
    if(Format==3 || Format==4)return FVector3d(DecodePQ(V.R),DecodePQ(V.G),DecodePQ(V.B));
    if(Format==5 || Format==6)return FVector3d(V.R,V.G,V.B)*80.;
    // SDR comparisons use a documented 100-nit reference, not a measurement
    // of Windows SDR brightness or the monitor's physical output.
    return FVector3d(DecodeSRGB(V.R),DecodeSRGB(V.G),DecodeSRGB(V.B))*100.;
}
double Luminance(const FVector3d& V,int32 Format)
{return (Format==3 || Format==4)?V.X*.2627+V.Y*.6780+V.Z*.0593:V.X*.2126+V.Y*.7152+V.Z*.0722;}
FVector3d Rec709(const FVector3d& V,int32 Format)
{
    if(Format!=3 && Format!=4)return V;
    return FVector3d(1.660491*V.X-.587641*V.Y-.072850*V.Z,-.124550*V.X+1.132900*V.Y-.008349*V.Z,-.018151*V.X-.100579*V.Y+1.118730*V.Z);
}
TArray<TSharedPtr<FJsonValue>> VectorJSON(const FVector3d& V)
{return {MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};}
FIntRect PixelRect(float X,float Y,float W,float H,FIntPoint Size)
{
    const float S=FMath::Min(Size.X/1920.f,Size.Y/1080.f),OX=(Size.X-1920*S)*.5f,OY=(Size.Y-1080*S)*.5f;
    return FIntRect(FMath::Clamp(FMath::RoundToInt(OX+X*S),0,Size.X-1),FMath::Clamp(FMath::RoundToInt(OY+Y*S),0,Size.Y-1),
        FMath::Clamp(FMath::RoundToInt(OX+(X+W)*S),1,Size.X),FMath::Clamp(FMath::RoundToInt(OY+(Y+H)*S),1,Size.Y));
}
TSharedRef<FJsonObject> Region(const TArray<FLinearColor>& Pixels,FIntPoint Size,FIntRect Rect,int32 Format)
{
    FVector3d Code(0),Light(0);double Min=DBL_MAX,Max=-DBL_MAX;int32 Count=0;
    for(int32 Y=Rect.Min.Y;Y<Rect.Max.Y;++Y)for(int32 X=Rect.Min.X;X<Rect.Max.X;++X)
    {
        const auto& P=Pixels[Y*Size.X+X];const auto N=Nits(P,Format);Code+=FVector3d(P.R,P.G,P.B);Light+=N;
        const double L=Luminance(N,Format);Min=FMath::Min(Min,L);Max=FMath::Max(Max,L);++Count;
    }
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("pixels"),Count);
    J->SetArrayField(TEXT("rect_xyxy"),{MakeShared<FJsonValueNumber>(Rect.Min.X),MakeShared<FJsonValueNumber>(Rect.Min.Y),MakeShared<FJsonValueNumber>(Rect.Max.X),MakeShared<FJsonValueNumber>(Rect.Max.Y)});
    J->SetArrayField(TEXT("mean_raw_rgb"),VectorJSON(Code/FMath::Max(Count,1)));J->SetArrayField(TEXT("mean_rgb_nits"),VectorJSON(Light/FMath::Max(Count,1)));
    J->SetArrayField(TEXT("mean_rec709_rgb_nits"),VectorJSON(Rec709(Light/FMath::Max(Count,1),Format)));
    J->SetNumberField(TEXT("mean_luminance_nits"),Luminance(Light/FMath::Max(Count,1),Format));
    J->SetNumberField(TEXT("minimum_luminance_nits"),Min);J->SetNumberField(TEXT("maximum_luminance_nits"),Max);return J;
}
struct FRun
{
    FCriticalSection Mutex;FDelegateHandle Backbuffer;
    const SWindow* Window=nullptr;
    FString Folder,Expected,Failure;int32 Stage=0,Phase=0,RequestPhase=-1,RequestFormat=0,RequestTileDraws=0;
    uint64 Frames=0,PhaseFrame=0;double Started=FPlatformTime::Seconds(),Next=Started+3,PhaseAt=0;
    bool Requested=false,InFlight=false,Passed=true,HasOriginal=false,Finished=false;
    float OriginalTimeDilation=1,OriginalCap=120;EFrontlineFrameGeneration OriginalFG=EFrontlineFrameGeneration::Off;
    FFrontlineHDRStatus Original;
    TSharedPtr<FJsonObject> Ready;TArray<TSharedPtr<FJsonValue>> Captures,Checks;
};
void Check(const TSharedRef<FRun,ESPMode::ThreadSafe>& Run,const TCHAR* Name,bool Pass,const FString& Detail=FString())
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);J->SetStringField(TEXT("detail"),Detail);
    Run->Checks.Add(MakeShared<FJsonValueObject>(J));Run->Passed&=Pass;
    UE_LOG(LogTemp,Display,TEXT("WF_V25_HDR %s %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
}
double Tile(const TSharedPtr<FJsonObject>& J,const TCHAR* Name)
{return J->GetObjectField(TEXT("tiles"))->GetObjectField(Name)->GetNumberField(TEXT("mean_luminance_nits"));}
double Scene(const TSharedPtr<FJsonObject>& J,int32 I)
{return J->GetArrayField(TEXT("scene_regions"))[I]->AsObject()->GetNumberField(TEXT("mean_luminance_nits"));}
FVector3d TileRGB(const TSharedPtr<FJsonObject>& J,const TCHAR* Name)
{
    const auto& V=J->GetObjectField(TEXT("tiles"))->GetObjectField(Name)->GetArrayField(TEXT("mean_rec709_rgb_nits"));
    return FVector3d(V[0]->AsNumber(),V[1]->AsNumber(),V[2]->AsNumber());
}
double MaxComponent(const FVector3d& V){return FMath::Max3(V.X,V.Y,V.Z);}
double MaxAbsComponent(const FVector3d& V){return FMath::Max3(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));}
bool NearRelative(double A,double B,double Tolerance,double Floor=.05)
{return FMath::Abs(A-B)<=FMath::Max(FMath::Abs(B)*Tolerance,Floor);}
}

void DrawTiles(UCanvas* Canvas)
{
    if(!Enabled() || !Canvas || !Canvas->Canvas || !Canvas->DefaultTexture || !Canvas->DefaultTexture->GetResource())return;
    const FIntPoint Size(FMath::RoundToInt(Canvas->ClipX),FMath::RoundToInt(Canvas->ClipY));
    for(int32 I=0;I<5;++I)
    {
        const auto Rect=PixelRect(600+144*I,430,120,100,Size);
        FCanvasTileItem Item(FVector2D(Rect.Min.X,Rect.Min.Y),Canvas->DefaultTexture->GetResource(),FVector2D(Rect.Width(),Rect.Height()),TileColors[I]);
        Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);
    }
    TileDraws.Increment();
}

void Begin(UWorld* World)
{
    if(!Enabled() || !World)return;
    FString UserDir;
    if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty() ||
        !FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
    {UE_LOG(LogTemp,Error,TEXT("WF_V25_HDR requires unique -UserDir and -WFRenderNoSave"));FPlatformMisc::RequestExit(false);return;}
    const auto Run=MakeShared<FRun,ESPMode::ThreadSafe>();
    Run->Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V25/HDRReview")/FGuid::NewGuid().ToString());
    FParse::Value(FCommandLine::Get(),TEXT("WFV25HDRExpected="),Run->Expected);
    IFileManager::Get().MakeDirectory(*Run->Folder,true);
    TWeakObjectPtr<UWorld> Weak(World);
    if(!FSlateApplication::IsInitialized()){FPlatformMisc::RequestExit(false);return;}
    Run->Backbuffer=FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddLambda(
        [Run](SWindow& Window,ISlateViewportProvider& Provider)
        {
            int32 Phase,Format,Draws;uint64 Frame;
            {
                FScopeLock Lock(&Run->Mutex);if(Run->Finished || Run->Window!=&Window)return;
                ++Run->Frames;if(!Run->Requested || Run->InFlight)return;
                Phase=Run->RequestPhase;Format=Run->RequestFormat;Draws=Run->RequestTileDraws;Frame=Run->Frames;
            }
            FRHITexture* Buffer=Provider.GetBackBufferResource();if(!Buffer)return;
            const auto PixelFormat=Buffer->GetDesc().Format;const auto Size=Buffer->GetDesc().Extent;
            const bool PQ=Format==3 || Format==4,ScRGB=Format==5 || Format==6;
            if((PQ && PixelFormat!=PF_A2B10G10R10) || (ScRGB && PixelFormat!=PF_FloatRGBA))return;
            if(!PQ && !ScRGB && PixelFormat!=PF_B8G8R8A8 && PixelFormat!=PF_R8G8B8A8 && PixelFormat!=PF_A2B10G10R10)return;
            if(Size.X<=0 || Size.Y<=0 || int64(Size.X)*Size.Y>33554432)return;
            {FScopeLock Lock(&Run->Mutex);Run->Requested=false;Run->InFlight=true;}
            auto& Cmd=FRHICommandListExecutor::GetImmediateCommandList();
            FReadSurfaceDataFlags Flags(RCM_MinMax);Flags.SetLinearToGamma(false);
            TArray<FLinearColor> Pixels;
            if(ScRGB)
            {
                TArray<FFloat16Color> Half;Cmd.ReadSurfaceFloatData(Buffer,FIntRect(0,0,Size.X,Size.Y),Half,Flags);
                Pixels.Reserve(Half.Num());for(const auto& P:Half)Pixels.Emplace(P.R.GetFloat(),P.G.GetFloat(),P.B.GetFloat(),P.A.GetFloat());
            }
            else if(PixelFormat==PF_A2B10G10R10)
            {
                // UE5.8 RHISurfaceDataConversion.h: R/G/B divided by 1023,
                // alpha divided by 3. This is encoded UNorm, NOT PQ-decoded.
                Cmd.ReadSurfaceData(Buffer,FIntRect(0,0,Size.X,Size.Y),Pixels,Flags);
            }
            else
            {
                // Only SDR8 uses FColor, preserving actual sRGB code values.
                // The FLinearColor overload unconditionally decodes SDR8.
                TArray<FColor> Byte;Cmd.ReadSurfaceData(Buffer,FIntRect(0,0,Size.X,Size.Y),Byte,Flags);
                Pixels.Reserve(Byte.Num());for(const auto& P:Byte)Pixels.Emplace(P.R/255.f,P.G/255.f,P.B/255.f,P.A/255.f);
            }
            Async(EAsyncExecution::ThreadPool,[Run,Phase,Format,Draws,Frame,PixelFormat,Size,Pixels=MoveTemp(Pixels)]() mutable
            {
                auto J=MakeShared<FJsonObject>();const FString Path=Run->Folder/(FString(PhaseNames[Phase])+TEXT(".rgba32f"));
                J->SetStringField(TEXT("phase"),PhaseNames[Phase]);J->SetStringField(TEXT("raw_file"),Path);J->SetNumberField(TEXT("width"),Size.X);J->SetNumberField(TEXT("height"),Size.Y);
                J->SetNumberField(TEXT("display_output_format"),Format);J->SetNumberField(TEXT("rhi_pixel_format"),int32(PixelFormat));
                J->SetStringField(TEXT("rhi_pixel_format_name"),GPixelFormats[PixelFormat].Name);
                J->SetStringField(TEXT("raw_layout"),TEXT("little-endian float32 RGBA; top row first; no image gamma conversion"));
                J->SetStringField(TEXT("encoding"),Format==3 || Format==4?TEXT("PQ-Rec2020-UNorm"):(Format==5 || Format==6?TEXT("scRGB-linear-Rec709-80nits"):TEXT("sRGB-UNorm-SDR100nit-reference")));
                J->SetStringField(TEXT("readback"),Format==5 || Format==6?TEXT("ReadSurfaceFloatData PF_FloatRGBA; RCM_MinMax; gamma=false"):
                    (PixelFormat==PF_A2B10G10R10?TEXT("ReadSurfaceData FLinearColor; raw 10bit UNorm; RCM_MinMax; gamma=false"):TEXT("ReadSurfaceData FColor SDR8 only; gamma=false; normalized to float")));
                J->SetNumberField(TEXT("backbuffer_frame"),double(Frame));J->SetNumberField(TEXT("hud_tile_draw_calls"),Draws);
                bool Saved=false,Finite=Pixels.Num()==Size.X*Size.Y;double Minimum=DBL_MAX,Maximum=-DBL_MAX;
                for(const auto& P:Pixels)for(float V:{P.R,P.G,P.B,P.A}){Finite&=FMath::IsFinite(V);Minimum=FMath::Min(Minimum,double(V));Maximum=FMath::Max(Maximum,double(V));}
                if(Finite)
                {
                    static_assert(sizeof(FLinearColor)==sizeof(float)*4,"Raw capture assumes packed four floats");
                    TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*Path));
                    if(File){File->Serialize(Pixels.GetData(),int64(Pixels.Num())*sizeof(FLinearColor));Saved=File->Close() && !File->IsError();}
                    auto Tiles=MakeShared<FJsonObject>();for(int32 I=0;I<5;++I)Tiles->SetObjectField(TileNames[I],Region(Pixels,Size,PixelRect(628+144*I,452,64,56,Size),Format));
                    J->SetObjectField(TEXT("tiles"),Tiles);TArray<TSharedPtr<FJsonValue>> Regions;
                    for(float X:{240.f,840.f,1440.f})Regions.Add(MakeShared<FJsonValueObject>(Region(Pixels,Size,PixelRect(X,650,140,65,Size),Format)));
                    J->SetArrayField(TEXT("scene_regions"),Regions);
                    // Real material-rendered artwork, in addition to the simple
                    // tiles. Mixed alpha/background means these are diagnostic
                    // crops, not opaque-white luminance assertions.
                    auto Art=MakeShared<FJsonObject>();
                    Art->SetObjectField(TEXT("left_flag"),Region(Pixels,Size,PixelRect(5,6,80,53,Size),Format));
                    Art->SetObjectField(TEXT("right_flag"),Region(Pixels,Size,PixelRect(1835,6,80,53,Size),Format));
                    Art->SetObjectField(TEXT("infantry_etched_label"),Region(Pixels,Size,PixelRect(25,877,150,35,Size),Format));
                    J->SetObjectField(TEXT("actual_hud_art_regions"),Art);
                }
                J->SetBoolField(TEXT("raw_saved"),Saved);J->SetBoolField(TEXT("all_samples_finite"),Finite);
                J->SetNumberField(TEXT("minimum_raw_component"),Finite?Minimum:0);J->SetNumberField(TEXT("maximum_raw_component"),Finite?Maximum:0);
                {FScopeLock Lock(&Run->Mutex);Run->Ready=J;Run->InFlight=false;}
            });
        });
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* W=Weak.Get();if(!W)return false;
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;
        auto* PC=Cast<AWarfareFrontlineController>(W->GetFirstPlayerController());
        auto* B=Cast<AWarfareFrontlineGameMode>(W->GetAuthGameMode());
        auto* R=UFrontlineRenderSettingsSubsystem::Get(W);auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        if(Now-Run->Started>75 && Run->Stage<90){Check(Run,TEXT("completed_without_timeout"),false);Run->Failure=TEXT("No complete stable compatible backbuffer within 75 seconds");Run->Stage=90;}
        if(!PC || !B || !R || !HUD){if(Run->Stage<90)return true;FPlatformMisc::RequestExit(false);return false;}
        switch(Run->Stage)
        {
        case 0:
            Check(Run,TEXT("campaign_save_isolated"),B->GetProgressionSaveStatus()==TEXT("独立验证档案"));
            if(!Run->Passed){Run->Stage=90;break;}
            Run->Original=R->GetHDRStatus();Run->OriginalFG=R->GetFrameGeneration();Run->OriginalCap=R->GetFrameRateLimit();Run->OriginalTimeDilation=W->GetWorldSettings()->TimeDilation;Run->HasOriginal=true;
            R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false);R->SetFrameRateLimit(60,false);R->SetHDRCalibrationVisible(false);
            HUD->NotifyHitBoxClick(FName(TEXT("NewCampaign")));Run->Stage=1;Run->Next=Now+.4;break;
        case 1:HUD->NotifyHitBoxClick(FName(TEXT("FactionBritish")));Run->Stage=2;Run->Next=Now+.4;break;
        case 2:
            HUD->NotifyHitBoxClick(FName(TEXT("Begin")));
            // V27 adds the real pre-operation attachment sheet to this flow.
            HUD->NotifyHitBoxClick(FName(TEXT("UnitIntroductionBegin")));
            Run->Stage=3;Run->Next=Now+1;break;
        case 3:
        {
            const int32 Page=HUD->GetPresentationPage();
            const bool Paused=UGameplayStatics::IsGamePaused(W);
            // V33 builds the real encounter before removing its preparation
            // artwork: Fighting can therefore coexist with page 9 and pause.
            // Let the production controller finish its resident-texture and
            // temporal-history warm-up before freezing actors for HDR captures.
            if(B->AssetFailure.IsEmpty() && (B->Phase==EFrontlinePhase::Briefing || Page==9 || Paused) && Now-Run->Started<40)
            {
                if(Page==15)HUD->NotifyHitBoxClick(FName(TEXT("UnitIntroductionBegin")));
                Run->Next=Now+.15;break;
            }
            Check(Run,TEXT("actual_battle_and_normal_hud"),B->Phase==EFrontlinePhase::Fighting && B->AssetFailure.IsEmpty() && Page==3 && !Paused,
                FString::Printf(TEXT("phase=%d page=%d paused=%d wall=%.2f"),int32(B->Phase),Page,Paused,Now-Run->Started));
            Check(Run,TEXT("hdr_capable_current_display"),R->GetHDRStatus().bSupported,R->GetHDRStatus().Reason);
            if(!Run->Passed){Run->Stage=90;break;}
            PC->DisableInput(PC);PC->bShowMouseCursor=false;
            // Freeze actor motion and near-freeze material game time, without
            // setting Pause (which would replace the battle HUD with a modal).
            for(TActorIterator<AActor> It(W);It;++It)It->SetActorTickEnabled(false);
            W->GetWorldSettings()->SetTimeDilation(.0001f);
            const auto* V=W->GetGameViewport();const auto Window=V?V->GetWindow():nullptr;
            {FScopeLock Lock(&Run->Mutex);Run->Window=Window.Get();}
            Run->Stage=4;break;
        }
        case 4:
        {
            switch(Run->Phase)
            {
            case 0:R->SetHDREnabled(false,false);R->SetHDRPaperWhiteNits(203,false);R->SetHDRUILuminanceNits(203,false);break;
            case 1:Check(Run,TEXT("hdr_enable_accepted"),R->SetHDREnabled(true,false));break;
            case 2:R->SetHDRUILuminanceNits(100,false);break;
            case 3:R->SetHDRPaperWhiteNits(100,false);break;
            case 4:R->SetHDRPaperWhiteNits(203,false);break;
            case 5:Check(Run,TEXT("sdr_return_accepted"),R->SetHDREnabled(false,false));break;
            case 6:R->ResetHDRCalibration(false);Check(Run,TEXT("automatic_windows_hdr_enable"),R->SetHDREnabled(true,false));break;
            }
            {FScopeLock Lock(&Run->Mutex);Run->PhaseFrame=Run->Frames;}
            Run->PhaseAt=Now;Run->Stage=5;Run->Next=Now+2.5;break;
        }
        case 5:
        {
            const auto Status=R->GetHDRStatus();const bool WantHDR=Run->Phase>0 && Run->Phase!=5;
            if(Status.bOutputActive!=WantHDR){Run->Next=Now+.1;break;}
            FScopeLock Lock(&Run->Mutex);if(Run->Frames-Run->PhaseFrame<20){Run->Next=Now+.1;break;}
            Run->RequestPhase=Run->Phase;Run->RequestFormat=Status.OutputFormat;Run->RequestTileDraws=TileDraws.GetValue();Run->Requested=true;Run->Stage=6;break;
        }
        case 6:
        {
            TSharedPtr<FJsonObject> Capture;{FScopeLock Lock(&Run->Mutex);Capture=Run->Ready;Run->Ready.Reset();}
            if(!Capture)break;
            const auto Status=R->GetHDRStatus();Capture->SetNumberField(TEXT("ui_setting_nits"),Status.UILuminanceNits);Capture->SetNumberField(TEXT("paper_setting_nits"),Status.PaperWhiteNits);
            Capture->SetNumberField(TEXT("settled_wall_seconds"),Now-Run->PhaseAt);Capture->SetObjectField(TEXT("render_diagnostics"),R->GetDiagnostics());
            Check(Run,*FString::Printf(TEXT("phase_%d_actual_backbuffer_saved"),Run->Phase),Capture->GetBoolField(TEXT("raw_saved")) && Capture->GetBoolField(TEXT("all_samples_finite")) && Capture->GetNumberField(TEXT("hud_tile_draw_calls"))>0);
            Run->Captures.Add(MakeShared<FJsonValueObject>(Capture));++Run->Phase;Run->Stage=Run->Phase<PhaseCount?4:90;break;
        }
        default:
        {
            {FScopeLock Lock(&Run->Mutex);if(Run->InFlight)return true;Run->Finished=true;Run->Requested=false;}
            FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().Remove(Run->Backbuffer);
            bool Complete=Run->Captures.Num()==PhaseCount;
            for(const auto& C:Run->Captures)Complete&=C->AsObject()->GetBoolField(TEXT("raw_saved"));
            Check(Run,TEXT("seven_actual_output_captures"),Complete);
            if(Complete)
            {
                auto A=Run->Captures[1]->AsObject(),U=Run->Captures[2]->AsObject(),P=Run->Captures[3]->AsObject(),Back=Run->Captures[4]->AsObject();
                const int32 Format=int32(A->GetNumberField(TEXT("display_output_format")));
                Check(Run,TEXT("requested_hdr_transport_observed"),Run->Expected.IsEmpty() || (Run->Expected==TEXT("PQ")?(Format==3 || Format==4):(Run->Expected==TEXT("scRGB") && (Format==5 || Format==6))),Run->Expected);
                Check(Run,TEXT("ui_white_tracks_203_and_100_nits"),NearRelative(Tile(A,TEXT("white")),203,.10) && NearRelative(Tile(U,TEXT("white")),100,.10),
                    FString::Printf(TEXT("203 setting=%.4f nits; 100 setting=%.4f nits"),Tile(A,TEXT("white")),Tile(U,TEXT("white"))));
                bool Ratios=true,UIUnchanged=true,SceneUnchanged=true,SceneReturn=true;double MaxSceneRelative=0,PaperDelta=0;
                for(const TCHAR* Name:TileNames)
                {
                    Ratios&=Tile(U,Name)>.05 && NearRelative(Tile(A,Name)/FMath::Max(Tile(U,Name),1.e-8),2.03,.10,.02);
                    UIUnchanged&=NearRelative(Tile(U,Name),Tile(P,Name),.04) && NearRelative(Tile(U,Name),Tile(Back,Name),.04);
                }
                for(int32 I=0;I<3;++I)
                {
                    SceneUnchanged&=NearRelative(Scene(A,I),Scene(U,I),.05,.15);SceneReturn&=NearRelative(Scene(U,I),Scene(Back,I),.05,.15);
                    MaxSceneRelative=FMath::Max(MaxSceneRelative,FMath::Abs(Scene(A,I)-Scene(U,I))/FMath::Max(Scene(A,I),.5));
                    PaperDelta=FMath::Max(PaperDelta,FMath::Abs(Scene(U,I)-Scene(P,I))/FMath::Max(Scene(U,I),.5));
                }
                Check(Run,TEXT("all_five_ui_colors_follow_same_luminance_ratio"),Ratios);
                bool Primaries=true,Neutral=true,SDRChromaticity=true;double LargestPrimaryError=0,LargestSDRError=0;
                for(const auto& C:Run->Captures)
                {
                    const auto Shot=C->AsObject();const double White=FMath::Max(MaxComponent(TileRGB(Shot,TEXT("white"))),1.e-8);
                    for(int32 I=0;I<5;++I)
                    {
                        const auto RGB=TileRGB(Shot,TileNames[I]);const auto Relative=RGB/White;
                        const auto SDR=TileRGB(Run->Captures[0]->AsObject(),TileNames[I])/FMath::Max(MaxComponent(TileRGB(Run->Captures[0]->AsObject(),TEXT("white"))),1.e-8);
                        const double Difference=MaxAbsComponent(Relative-SDR);LargestSDRError=FMath::Max(LargestSDRError,Difference);SDRChromaticity&=Difference<=.025;
                        if(I>=2)
                        {
                            const FVector3d Target(I==2?1.:0.,I==3?1.:0.,I==4?1.:0.);
                            const double Error=MaxAbsComponent(RGB/FMath::Max(MaxComponent(RGB),1.e-8)-Target);
                            LargestPrimaryError=FMath::Max(LargestPrimaryError,Error);Primaries&=Error<=.02;
                        }
                        else
                        {
                            Neutral&=MaxAbsComponent(RGB/FMath::Max(MaxComponent(RGB),1.e-8)-FVector3d(1.))<=.02;
                        }
                    }
                }
                Check(Run,TEXT("decoded_rec709_primary_chromaticities"),Primaries,FString::Printf(TEXT("maximum normalized component error=%.6f; limit0.02"),LargestPrimaryError));
                Check(Run,TEXT("decoded_white_and_gray_are_neutral"),Neutral);
                Check(Run,TEXT("hdr_ui_colors_match_sdr_after_white_normalization"),SDRChromaticity,FString::Printf(TEXT("maximum normalized component difference=%.6f; limit0.025"),LargestSDRError));
                Check(Run,TEXT("ui_only_change_preserves_scene_regions"),SceneUnchanged,FString::Printf(TEXT("maximum scene relative change=%.6f; 5%% or 0.15nit tolerance"),MaxSceneRelative));
                Check(Run,TEXT("paper_change_preserves_ui_luminance"),UIUnchanged);
                Check(Run,TEXT("paper_change_affects_scene"),PaperDelta>.08,FString::Printf(TEXT("maximum scene relative change=%.6f"),PaperDelta));
                Check(Run,TEXT("paper_return_restores_scene"),SceneReturn);
                bool SDRReturn=true;for(const TCHAR* Name:TileNames)SDRReturn&=NearRelative(Tile(Run->Captures[0]->AsObject(),Name),Tile(Run->Captures[5]->AsObject(),Name),.025,.1);
                for(int32 I=0;I<3;++I)SDRReturn&=NearRelative(Scene(Run->Captures[0]->AsObject(),I),Scene(Run->Captures[5]->AsObject(),I),.05,.15);
                Check(Run,TEXT("sdr_return_restores_ui_and_scene"),SDRReturn);
                const auto Automatic=Run->Captures[6]->AsObject();const auto Status=R->GetHDRStatus();
                Check(Run,TEXT("windows_default_white_reaches_actual_hdr_backbuffer"),Status.bPaperWhiteAutomatic && Status.bUIWhiteAutomatic &&
                    NearRelative(Tile(Automatic,TEXT("white")),Status.UILuminanceNits,.10),
                    FString::Printf(TEXT("system white=%.2f nit known=%d; applied=%.2f; actual UI=%.2f nit"),Status.DisplaySDRWhiteNits,Status.bSystemWhiteKnown,Status.UILuminanceNits,Tile(Automatic,TEXT("white"))));
            }
            auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),Run->Passed);Report->SetArrayField(TEXT("checks"),Run->Checks);Report->SetArrayField(TEXT("captures"),Run->Captures);
            int32 Failed=0;for(const auto& C:Run->Checks)Failed+=C->AsObject()->GetBoolField(TEXT("passed"))?0:1;
            Report->SetNumberField(TEXT("failed_count"),Failed);Report->SetStringField(TEXT("failure"),Run->Failure);Report->SetStringField(TEXT("expected_hdr_transport"),Run->Expected);
            Report->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);Report->SetStringField(TEXT("scope"),TEXT("Seven actual final Slate backbuffer captures including automatic Windows reference white, production HUD route with five test-only tiles. Real battle camera; actor ticks frozen and game time dilation 0.0001 to isolate lighting controls. No calibration-patch bypass, no FG, no screenshot tone conversion, no user-save writes. Raw HDR10 UNorm decoded as ST2084 Rec2020; scRGB floats retained incl negative/>1 and 80nit scaling. SDR uses assumed100nit reference only. Numeric output verifies game pixels, not monitor peak luminance, OS compositor/physical panel calibration, animation, or HDR aesthetics; inspect SDR previews separately."));
            FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(Run->Folder/TEXT("hdr-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            if(Run->HasOriginal)
            {
                R->SetHDRPaperWhiteNits(Run->Original.PaperWhiteNits,false);R->SetHDRUILuminanceNits(Run->Original.UILuminanceNits,false);
                R->SetHDREnabled(Run->Original.bEnabled,false);R->SetFrameGeneration(Run->OriginalFG,false);R->SetFrameRateLimit(Run->OriginalCap,false);
                W->GetWorldSettings()->SetTimeDilation(Run->OriginalTimeDilation);
            }
            UE_LOG(LogTemp,Display,TEXT("WF_V25_HDR_COMPLETE %s failed=%d %s"),Run->Passed?TEXT("PASS"):TEXT("FAIL"),Failed,*Run->Folder);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
}

