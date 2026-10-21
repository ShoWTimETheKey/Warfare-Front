#include "WarfareFrontCursorV67.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/ICursor.h"

namespace
{
TWeakObjectPtr<UGameViewportClient> InstalledViewport;
int32 InstalledScale=0;
bool Installed=false;
}
namespace FrontlineCursorV67
{
bool Install(APlayerController* Player)
{
    if(!Player||!Player->GetLocalPlayer()||!FSlateApplication::IsInitialized())return false;
    // Offscreen reviews use FNullCursor; there is no OS pointer to replace.
    const auto NativeCursor=FSlateApplication::Get().GetPlatformCursor();
    if(!NativeCursor.IsValid()||!NativeCursor->IsCreateCursorFromRGBABufferSupported())return false;
    auto* Viewport=Player->GetLocalPlayer()->ViewportClient.Get();if(!Viewport)return false;
    const auto Window=Viewport->GetWindow();const float Dpi=Window.IsValid()?Window->GetDPIScaleFactor():1.f;
    // Select against this viewport's actual monitor, not the primary monitor.
    const int32 Scale=FMath::Clamp(FMath::RoundToInt(Dpi*4.f)*25,100,300);
    if(InstalledViewport.Get()==Viewport&&InstalledScale==Scale)return Installed;
    InstalledViewport=Viewport;InstalledScale=Scale;
    const FString Path=FString::Printf(TEXT("UI/Cursors/V81/%d/"),Scale);
    const FVector2D Tip(3.f/40.f,2.f/48.f);
    bool OK=true;
    OK=Viewport->SetHardwareCursor(EMouseCursor::Default,FName(*(Path+TEXT("Idle"))),Tip)&&OK;
    OK=Viewport->SetHardwareCursor(EMouseCursor::Hand,FName(*(Path+TEXT("Hover"))),Tip)&&OK;
    OK=Viewport->SetHardwareCursor(EMouseCursor::Crosshairs,FName(*(Path+TEXT("Target"))),FVector2D(.5,.5))&&OK;
    OK=Viewport->SetHardwareCursor(EMouseCursor::SlashedCircle,FName(*(Path+TEXT("Disabled"))),Tip)&&OK;
    Installed=OK;
    UE_LOG(LogTemp,Display,TEXT("WF_CURSOR_V73 ready=%d dpi=%.2f asset_scale=%d"),OK,Dpi,Scale);
    return OK;
}
void SetState(APlayerController* Player,EFrontlineCursorV67 State)
{
    if(!Player||!Install(Player))return;
    const EMouseCursor::Type Cursor=State==EFrontlineCursorV67::Hover?EMouseCursor::Hand:
        State==EFrontlineCursorV67::Target?EMouseCursor::Crosshairs:
        State==EFrontlineCursorV67::Disabled?EMouseCursor::SlashedCircle:EMouseCursor::Default;
    Player->CurrentMouseCursor=Cursor;
    Player->DefaultMouseCursor=EMouseCursor::Default;
}
}
