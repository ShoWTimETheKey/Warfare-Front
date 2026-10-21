#pragma once
#include "WarfareFrontlineUIMotionV83.h"

// One real-time sequence per committed award; the battle clock cannot slow it.
namespace FrontlineAwardMotionV8217
{
struct FSample
{float OffsetX=0,StampAlpha=1,StampScale=1,StampOffsetY=0,StampBrightness=1;};
inline FSample Sample(double Age,bool Stable=false)
{
    if(Stable)return {};
    FSample S;S.OffsetX=668.f*(1.f-FrontlineUIMotionV83::CubicOut(Age/.32)+FrontlineUIMotionV83::CubicIn((Age-6.26)/.24));
    S.StampAlpha=FrontlineUIMotionV83::Unit((Age-.38)/.04);
    if(Age<.51)
    {
        const float Drop=FrontlineUIMotionV83::CubicIn((Age-.38)/.13);
        S.StampScale=FMath::Lerp(1.48f,.98f,Drop);S.StampOffsetY=-25.f*(1.f-Drop);
    }
    else S.StampScale=FMath::Lerp(.98f,1.f,FrontlineUIMotionV83::CubicOut((Age-.51)/.12));
    // The single metallic glint follows contact; it never cycles while held.
    if(Age>.57&&Age<.88)S.StampBrightness=1.f+.32f*FMath::Sin(PI*FrontlineUIMotionV83::Unit((Age-.57)/.31));
    return S;
}
inline void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto Before=Sample(.37),Drop=Sample(.45),Contact=Sample(.51),Shine=Sample(.725),Settled=Sample(.89);
    Check(TEXT("v8217_award_badge_waits_until_banner_has_arrived"),Sample(.32).OffsetX==0&&Before.StampAlpha==0);
    Check(TEXT("v8217_award_actual_stamp_drops_and_contracts_before_contact"),Drop.StampAlpha==1&&Drop.StampOffsetY<0&&Drop.StampScale>1);
    Check(TEXT("v8217_award_stamp_contacts_before_single_shine"),Contact.StampOffsetY==0&&Contact.StampBrightness==1&&Contact.StampScale<1);
    Check(TEXT("v8217_award_has_one_post_contact_glint_then_no_idle_pulse"),Shine.StampBrightness>1.30f&&Settled.StampBrightness==1&&Sample(3).StampBrightness==1);
    Check(TEXT("v8217_award_terminal_badge_scale_and_exit_unchanged"),Settled.StampScale==1&&Sample(6.5).OffsetX>=660&&Sample(1,true).StampBrightness==1);
}
}
