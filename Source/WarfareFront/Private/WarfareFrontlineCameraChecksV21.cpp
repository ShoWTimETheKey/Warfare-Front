#include "WarfareFrontline.h"
#include "WarfareFrontlineCameraInput.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AWarfareFrontlineController::RunV21CameraChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV21Review")))return;
    using namespace FrontlineCameraInput;
    FInput In;In.bActive=true;In.bPointerValid=true;In.Viewport=FVector2D(1920,1080);
    In.FieldTop=68;In.FieldBottom=874;In.Mouse=FVector2D(1,400);
    Check(TEXT("left_and_right_edges_pan_in_screen_direction"),Axis(In)<-.99f
        &&[&](){In.Mouse.X=1919;return Axis(In)>.99f;}());
    In.Mouse.X=1920*.98f;const float Half=Axis(In);
    In.Mouse.X=1919;
    Check(TEXT("edge_band_has_progressive_speed"),Half>.45f&&Half<.55f&&Axis(In)>Half);
    In.Mouse.X=960;Check(TEXT("centre_pointer_never_pans"),Axis(In)==0);
    In.Mouse=FVector2D(1,30);const bool NoTop=Axis(In)==0;
    In.Mouse.Y=900;Check(TEXT("morale_bar_and_bottom_console_are_excluded"),NoTop&&Axis(In)==0);
    In.Mouse=FVector2D(-1,400);const bool NoLeftOutside=Axis(In)==0;
    In.Mouse.X=1920;Check(TEXT("pointer_outside_window_does_not_continue_pan"),NoLeftOutside&&Axis(In)==0);
    In.Mouse=FVector2D(1919,400);In.bOverUI=true;
    Check(TEXT("trench_portraits_and_other_hitboxes_suppress_edge_pan"),Axis(In)==0);
    In.bOverUI=false;In.bActive=false;
    Check(TEXT("lost_focus_and_overlay_disable_input"),Axis(In)==0);
    In.bActive=true;In.bLeftKey=true;
    Check(TEXT("keyboard_overrides_opposite_mouse_edge"),Axis(In)==-1);
    In.bRightKey=true;Check(TEXT("opposite_keyboard_keys_cancel_without_edge_fallback"),Axis(In)==0);
    In.bLeftKey=false;In.bRightKey=false;
    In.bPointerValid=false;Check(TEXT("invalid_or_pause_disabled_pointer_is_ignored"),Axis(In)==0);

    auto Travel=[](int32 FPS)
    {
        double X=0;float V=0;
        for(int32 I=0;I<FPS;++I)Step(1,6000,1.f/FPS,true,10800,X,V);
        return X;
    };
    const double X30=Travel(30),X60=Travel(60),X120=Travel(120);
    Check(TEXT("camera_distance_matches_at_30_60_120_fps"),FMath::Abs(X30-X120)<.1&&FMath::Abs(X60-X120)<.1);
    Check(TEXT("edge_full_speed_uses_readable_screen_relative_travel"),X120>3000&&X120<3200);
    double X=0;float V=3300;
    for(int32 I=0;I<24;++I)Step(0,6000,1.f/120,true,10800,X,V);
    Check(TEXT("edge_release_stops_quickly_with_bounded_coast"),X<120&&FMath::Abs(V)<13.f);
    X=0;V=3300;Step(-1,6000,1.f/60,true,10800,X,V);
    Check(TEXT("direction_reversal_does_not_coast_wrong_way"),X<0&&V<0);
    X=10800;V=3300;Step(1,6000,1.f/60,true,10800,X,V);
    Check(TEXT("world_limit_stops_without_velocity_accumulation"),X==10800&&V==0);
    X=500;V=3300;Step(1,6000,1.f/60,false,10800,X,V);
    Check(TEXT("deactivation_stops_immediately_without_background_drift"),X==500&&V==0);
    X=11320;V=200;Step(0,9000,1.f/60,false,10200,X,V);
    Check(TEXT("zoom_reclamps_extent_even_when_pointer_or_pause_blocks_pan"),X==10200&&V==0);
    double WideX=0;float WideV=0;X=0;V=0;
    for(int32 I=0;I<60;++I){Step(1,6000,1.f/60,true,20000,X,V);Step(1,9000,1.f/60,true,20000,WideX,WideV);}
    Check(TEXT("zoom_preserves_screen_relative_pan_speed"),FMath::Abs(X/6000.-WideX/9000.)<.0001);
}
