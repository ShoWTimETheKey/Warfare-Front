void DrawAchievementsV76(const FFrontlineCanvas& UI)
{
    if(MedalWallV8215::Selected(UI.HUD.GetOwningPlayerController()))
    {MedalWallV8215::Draw(UI,FrontlineFor(UI.HUD.GetOwningPlayerController()));return;}
    auto& State=ScreenState(UI.HUD.GetOwningPlayerController());
    UI.PageSurfaceV73(0,0,1920,1080,false,false);UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    UI.BilingualLabel(TEXT("HONOURS"),TEXT("战功"),92,51,1250,67,50,Paper,0,EFrontlineTextRoleV8210::Inscription);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::Printf(TEXT("%d / %d"),FrontlineAchievementsV76::EarnedCount(),FrontlineAchievementsV76::Count),1490,62,334,44,29,Muted,2);
    UI.ConsoleSeamV79(92,137,1828,137,Border,2);
    MedalWallV8215::Tabs(UI);
    const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    HonoursListV82::Advance(State);
    const float LX=HonoursListV82::X,Top=HonoursListV82::Top,LW=HonoursListV82::Width,LH=HonoursListV82::Height;
    UI.Skin(2,LX-8,Top-8,LW+16,LH+16,9,FLinearColor(.80f,.81f,.76f));
    // Render and register only whole rows. No scissor/flush assumptions: the
    // HDR DebugCanvas is submitted later by Slate, unlike the SDR HUD canvas.
    for(int32 I=0;I<FrontlineAchievementsV76::Count;++I)
    {
        if(!HonoursListV82::Visible(State,I))continue;
        const auto& D=FrontlineAchievementsV76::Definition(I);auto& T=Artwork().AchievementBadgesV76[I];
        if(!T)T=FrontlineUIAssetsV8213::Texture(FrontlineAchievementsV76::BadgeObjectPathV82(I));
        const float Y=Top+I*HonoursListV82::Stride-State.AchievementScrollV82,H=HonoursListV82::RowHeight;
        const float HitTop=FMath::Max(Y,Top),HitBottom=FMath::Min(Y+H,Top+LH);
        const bool Earned=FrontlineAchievementsV76::Earned(I),Selected=State.AchievementFocusV76==I,Hover=UI.Hovered(LX,HitTop,LW,HitBottom-HitTop);
        // One continuous material under every row. Focus is a brass edge and
        // brighter lettering; moving the pointer never replaces its grain.
        UI.Image(T,LX+18,Y+9,80,80,FLinearColor::White,0,0,1,1,!Earned,true);
        UI.RoleLabel(EFrontlineTextRoleV8210::Headline,ZH?D.NameZH:D.NameEN,LX+122,Y+8,LW-148,48,ZH?29:36,Selected||Hover||Earned?Paper:Muted);
        const bool CrossArmy=I>=FrontlineAchievementsV76::CrossFactionStartV83&&I<FrontlineAchievementsV76::TributeStartV836;
        const bool Special=FrontlineAchievementsV76::SpecialHonourV844(I);
        const TCHAR* KindEN=Special?TEXT("SPECIAL HONOUR"):CrossArmy?TEXT("ACROSS THE FRONTS"):TEXT("FIELD SERVICE");
        const TCHAR* KindZH=Special?TEXT("特殊战功"):CrossArmy?TEXT("跨阵营战功"):TEXT("前线战功");
        UI.BilingualLabel(KindEN,KindZH,LX+122,Y+58,LW-340,34,18,Special?Amber*.85f:Dim);
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,WarRoom52::L(Earned?TEXT("EARNED"):TEXT("UNCLAIMED"),Earned?TEXT("已达成"):TEXT("尚未达成")),LX+LW-220,Y+58,194,34,20,Earned?Amber:Dim,2);
        UI.ConsoleSeamV79(LX+116,Y+H,LX+LW-20,Y+H,Border,1);
        if(Selected||Hover)UI.Line(LX+5,Y+18,LX+5,Y+H-18,Selected?Amber:Muted,Selected?2.f:1.f);
        UI.Hit(LX,HitTop,LW,HitBottom-HitTop,FName(*FString::Printf(TEXT("AchievementV76_%d"),I)),51);
    }
    // A recessed brass slider follows the continuous list, without new pages.
    const float TrackX=LX+LW+16,ThumbH=LH*LH/(FrontlineAchievementsV76::Count*HonoursListV82::Stride);
    const float ThumbY=Top+(LH-ThumbH)*State.AchievementScrollV82/FMath::Max(1.f,HonoursListV82::Maximum());
    UI.Line(TrackX,Top,TrackX,Top+LH,FLinearColor(.025f,.030f,.027f),4);
    UI.Line(TrackX,ThumbY,TrackX,ThumbY+ThumbH,FLinearColor(.38f,.34f,.24f),4);
    if(UI.Hovered(LX,Top,LW+24,LH))UI.RoleLabel(EFrontlineTextRoleV8210::Text,WarRoom52::L(TEXT("MOUSE WHEEL · SCROLL"),TEXT("鼠标滚轮 · 浏览")),LX,960,LW,34,19,Dim);
    const int32 Focus=FMath::Clamp(State.AchievementFocusV76,0,FrontlineAchievementsV76::Count-1);const auto& D=FrontlineAchievementsV76::Definition(Focus);
    auto& FocusBadge=Artwork().AchievementBadgesV76[Focus];
    if(!FocusBadge)FocusBadge=FrontlineUIAssetsV8213::Texture(FrontlineAchievementsV76::BadgeObjectPathV82(Focus));
    constexpr float PX=1120,PW=708,TX=PX+32,TW=PW-64;
    const FLinearColor Ink(.06f,.065f,.058f),Quiet(.10f,.11f,.095f);UI.PaperSheet(PX,Top,PW,LH);
    UI.Image(FocusBadge,PX+(PW-128)*.5f,Top+24,128,128,FLinearColor::White,0,0,1,1,!FrontlineAchievementsV76::Earned(Focus),true);
    UI.BilingualTitle(D.NameEN,D.NameZH,TX,Top+166,TW,56,ZH?29:36,Ink,1);
    if(FrontlineAchievementsV76::SpecialHonourV844(Focus))
        UI.BilingualLabel(TEXT("SPECIAL HONOUR"),TEXT("特殊战功"),TX,Top+224,TW,20,15,FLinearColor(.32f,.20f,.10f),1);
    const FString Eligibility=Focus==FrontlineAchievementsV76::TributeStartV836+1?WarRoom52::L(TEXT("On “Returned to the Front” difficulty, "),TEXT("在“去而复返”难度下，")):
        Focus==11?WarRoom52::L(TEXT("On any difficulty, "),TEXT("在任意难度下，")):
        WarRoom52::L(TEXT("On “Memories of the Abyss” or “Returned to the Front” difficulty, "),TEXT("在“深渊回忆”难度或“去而复返”难度下，"));
    FString Requirement=UI.DisplayText(ZH?D.RuleZH:D.RuleEN);
    const bool HasDifficulty=ZH?Requirement.Contains(TEXT("难度")):
        Requirement.Contains(TEXT("Memories of the Abyss"))||Requirement.Contains(TEXT("Returned to the Front"));
    if(!HasDifficulty&&!ZH&&!Requirement.IsEmpty())Requirement[0]=FChar::ToLower(Requirement[0]);
    const FString CompleteRequirement=HasDifficulty?Requirement:Eligibility.TrimEnd()+TEXT("\n")+Requirement;
    // Historical context remains separate from the achievement requirement.
    UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,ZH?D.HistoryZH:D.HistoryEN,TX,Top+250,TW,160,23,Quiet);
    UI.Line(TX,Top+430,TX+TW,Top+430,FLinearColor(.23f,.21f,.17f,.35f));
    {TGuardValue<bool> Resolved(UI.bResolvedTextV8210,true);
     UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,CompleteRequirement,TX,Top+454,TW,171,23,Ink);}
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,FrontlineAchievementsV76::Status(Focus,ZH),TX,Top+650,TW,64,ZH?18:20,Quiet);
    UI.PageButtonV73(PX,958,PW,58,WarRoom52::L(TEXT("RETURN"),TEXT("返回")),TEXT("Esc"),TEXT("AchievementsBackV76"));
}

bool BeginNextAwardBannerV8217(FFrontlineScreenState& S,double Now)
{
    if(S.AchievementToastV81==INDEX_NONE&&S.MedalToastV8217==INDEX_NONE)
    {
        S.AchievementToastV81=FrontlineAchievementsV76::TakeNotificationV81();
        if(S.AchievementToastV81<0||S.AchievementToastV81>=FrontlineAchievementsV76::Count)
        {
            S.AchievementToastV81=INDEX_NONE;const auto Medal=FrontlineAchievementsV76::TakeMedalNotificationV8217();
            if(Medal.Index<0||Medal.Index>=FrontlineMedalsV8215::Count)return false;
            S.MedalToastV8217=Medal.Index;S.MedalToastGradeV8217=Medal.Grade;
            S.MedalToastRepeatsV8217=Medal.Repeats;S.MedalToastAttachmentsV8217=Medal.Attachments;
        }
        S.AchievementToastAtV81=Now;
    }
    return true;
}
void DrawAchievementBannerV81(const FFrontlineCanvas& UI,FFrontlineScreenState& S)
{
    const double Now=FPlatformTime::Seconds();if(!BeginNextAwardBannerV8217(S,Now))return;
    const float Age=float(UIMotionReviewV83::BannerAge>=0?UIMotionReviewV83::BannerAge:Now-S.AchievementToastAtV81);
    if(Age>=6.5f){S.AchievementToastV81=INDEX_NONE;S.MedalToastV8217=INDEX_NONE;return;}
    const auto Motion=FrontlineAwardMotionV8217::Sample(Age,UI.MotionStableV83);
    const float X=1260.f+Motion.OffsetX,Y=92.f,W=588.f,H=124.f;
    const bool Medal=S.MedalToastV8217!=INDEX_NONE,Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    UTexture2D* Badge=nullptr;FString Name;
    if(Medal)
    {
        Badge=FrontlineUIAssetsV8213::Texture(FrontlineMedalsV8215::VariantArtworkPath(S.MedalToastV8217,S.MedalToastGradeV8217));
        const auto& D=FrontlineMedalsV8215::Definition(S.MedalToastV8217);const FString Family=Chinese?D.NameZH:D.NameEN;Name=Family;
        const auto Variants=FrontlineMedalsV8215::Variants(S.MedalToastV8217);
        if(Variants.IsValidIndex(int32(S.MedalToastGradeV8217)-1))
        {
            const auto& V=Variants[S.MedalToastGradeV8217-1];const FString Class=Chinese?V.NameZH:V.NameEN;
            if(!Class.IsEmpty())Name=Class.Contains(Family,ESearchCase::IgnoreCase)?Class:Family+TEXT(" · ")+Class;
        }
    }
    else
    {
        const auto& D=FrontlineAchievementsV76::Definition(S.AchievementToastV81);Name=Chinese?D.NameZH:D.NameEN;
        auto& T=Artwork().AchievementBadgesV76[S.AchievementToastV81];
        if(!T)T=FrontlineUIAssetsV8213::Texture(FrontlineAchievementsV76::BadgeObjectPathV82(S.AchievementToastV81));Badge=T;
    }
    UI.Skin(0,X,Y,W,H,16);
    const float StampSize=88.f*Motion.StampScale,StampInset=(StampSize-88.f)*.5f;
    if(Motion.StampAlpha>0)UI.ImageContain(Badge,X+22-StampInset,Y+18-StampInset+Motion.StampOffsetY,StampSize,StampSize,
        FLinearColor(Motion.StampBrightness,Motion.StampBrightness,Motion.StampBrightness,Motion.StampAlpha));
    const bool Special=!Medal&&FrontlineAchievementsV76::SpecialHonourV844(S.AchievementToastV81);
    UI.BilingualLabel(Medal?TEXT("DECORATION AWARDED"):Special?TEXT("SPECIAL HONOUR AWARDED"):TEXT("HONOUR AWARDED"),
        Medal?TEXT("勋章已授予"):Special?TEXT("特殊战功已记入档案"):TEXT("战功已记入档案"),X+130,Y+19,W-156,26,15,Special?Amber:Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Name,X+130,Y+52,W-156,48,Chinese?26:34,Paper);
    // No hitbox: the banner never pauses the outro or consumes a player order.
}
