// Included in Presentation.cpp's existing anonymous namespace.
// Shared player text is authored in interface.json; this file owns drawing only.
#include "WarfareFrontlineManualContentV841.generated.inl"

bool DrawManualFieldcraftV841(const FFrontlineCanvas& UI)
{
    const auto& State=ScreenState(UI.HUD.GetOwningPlayerController());
    if(!State.ManualBattleV841)return false;
    using R=EFrontlineTextRoleV8210;
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    const int32 Topic=FMath::Clamp(State.ManualBattleTopicV841,0,int32(UE_ARRAY_COUNT(ManualPagesV841))-1);
    const auto& Page=ManualPagesV841[Topic];
    const TCHAR* const* Sections=Chinese?Page.SectionsZh:Page.SectionsEn;

    UI.PhysicalPanelV74(80,252,1152,676);
    UI.PhysicalPanelV74(1272,252,552,676,true);
    for(int32 I=0;I<int32(UE_ARRAY_COUNT(ManualPagesV841));++I)
    {
        const auto& Item=ManualPagesV841[I];
        const FName Command(*FString::Printf(TEXT("ManualTopicV841%d"),I));
        UI.PageButtonV73(112+I*272.f,278,260,48,Chinese?Item.TitleZh:Item.TitleEn,TEXT(""),Command,true,I==Topic,50);
    }
    UI.BilingualTitle(Page.TitleEn,Page.TitleZh,112,345,1076,61,32,Paper);
    UI.PageRuleV74(112,410,1076);

    // Measure the same shaped rows, font height and language-dependent leading
    // used by RoleParagraph. A long topic cannot push its last rule into the frame.
    constexpr float BodyX=112.f,BodyY=429.f,BodyW=1076.f,BodyH=474.f,Gap=16.f;
    auto ParagraphHeight=[&](const FString& Text,float Em)
    {
        const bool CJK=FFrontlineCanvas::ContainsCJKV45(Text);
        TGuardValue<bool> FaceScope(UI.bRoleChineseV8210,CJK);
        float Width=BodyW;
        if(CJK)
        {
            const float Cell=FMath::Max(.001f,float(UI.RoleSizeRawV8210(R::Text,TEXT("国国"),Em).X-UI.RoleSizeRawV8210(R::Text,TEXT("国"),Em).X));
            Width=FMath::Max(Cell,FMath::FloorToFloat(BodyW/Cell)*Cell);
        }
        const auto Rows=UI.RoleRowsV8210(R::Text,Text,Width,Em);
        const float Leading=Em*(CJK?(Rows.Num()<3?1.6f:1.85f):1.48f);
        float Height=0.f;
        for(const auto& Row:Rows)Height=FMath::Max(Height,float(UI.RoleSizeRawV8210(R::Text,Row,Em).Y));
        return Rows.IsEmpty()?0.f:Height+FMath::Max(0,Rows.Num()-1)*Leading;
    };
    float Em=22.f;
    for(;;)
    {
        float Needed=Gap*FMath::Max(0,Page.Count-1);
        for(int32 I=0;I<Page.Count;++I)Needed+=ParagraphHeight(Sections[I],Em);
        if(Needed<=BodyH+.1f||Em<=20.f)break;
        Em-=.5f;
    }
    float Y=BodyY;
    for(int32 I=0;I<Page.Count;++I)
    {
        const float Height=ParagraphHeight(Sections[I],Em);
        UI.RoleParagraph(R::Text,Sections[I],BodyX,Y,BodyW,Height+.5f,Em,Paper);
        Y+=Height+Gap;
    }

    UI.BilingualTitle(TEXT("Field notes"),TEXT("战地要点"),1296,282,504,62,27,Paper);
    UI.PageRuleV74(1296,359,504);
    UI.RoleParagraph(R::Text,Chinese?Page.SummaryZh:Page.SummaryEn,1296,394,504,434,21,Muted);
    return true;
}
