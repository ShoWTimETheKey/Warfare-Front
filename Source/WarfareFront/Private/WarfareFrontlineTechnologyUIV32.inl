bool DrawNationalTacticalArtworkV82(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFRRUTacticalOptionV82& Option,float X,float Y,float Size)
{
    // Stable IDs select only the corrected orders. Each lower-row cell is a
    // separately composed original period-style photographic illustration.
    int32 Column=INDEX_NONE;const bool French=Option.Id.StartsWith(TEXT("fr.tech.")),Russian=Option.Id.StartsWith(TEXT("ru.tech."));
    if(!French&&!Russian)return false;
    const TCHAR* RepairIds[]={TEXT("fr.tech.1"),TEXT("fr.tech.4"),TEXT("fr.tech.7"),TEXT("ru.tech.4")};
    for(int32 I=0;I<4;++I)if(Option.Id==RepairIds[I])
    {
        static TStrongObjectPtr<UTexture2D> RepairedOrderArt;
        if(!RepairedOrderArt.IsValid())RepairedOrderArt.Reset(FrontlineUIAssetsV8213::Texture(TEXT("/Game/FRRUIntegration/FormalV82/UI/T_TacticalOrderRepair_V82.T_TacticalOrderRepair_V82")));
        DrawNationalResearchPhotoV82(UI,B,Option,RepairedOrderArt.Get(),I,I+4,4,2,X,Y,Size,Size);return true;
    }
    for(int32 I=0;I<3;++I){const int32 Index=I==0?2:I==1?5:9;if(Option.Id==FString::Printf(TEXT("%s.tech.%d"),French?TEXT("fr"):TEXT("ru"),Index)){Column=I;break;}}
    if(Column==INDEX_NONE)return false;
    const FString Name=French?TEXT("T_Tactical_FR_V82"):TEXT("T_Tactical_RU_V82");
    const FString Path=TEXT("/Game/FRRUIntegration/FormalV82/UI/")+Name+TEXT(".")+Name;
    static TMap<FString,TStrongObjectPtr<UTexture2D>> CorrectedOrders;auto& Texture=CorrectedOrders.FindOrAdd(Path);
    if(!Texture.IsValid())Texture.Reset(FrontlineUIAssetsV8213::Texture(Path));
    DrawNationalResearchPhotoV82(UI,B,Option,Texture.Get(),Column,Column+3,3,2,X,Y,Size,Size);return true;
}

bool NationalArmourResearchV87(const FFRRUTacticalOptionV82& Row)
{return Row.Id.StartsWith(TEXT("fr.armour."))||Row.Id.StartsWith(TEXT("ru.armour."));}
UTexture2D* NationalArmourResearchAtlasV87()
{
    auto& Bank=Artwork();
    // Texture() initiates/reuses the nonblocking cache request. The artwork
    // bank may have been constructed before that request completed; refresh
    // its empty slot instead of retaining the initial nullptr permanently.
    if(!Bank.FRRUArmourResearchAtlasV87)Bank.FRRUArmourResearchAtlasV87=FrontlineUIAssetsV8213::Texture(TEXT("/Game/FRRUIntegration/FormalV82/UI/T_ArmourResearch_FRRU_V87.T_ArmourResearch_FRRU_V87"));
    return Bank.FRRUArmourResearchAtlasV87.Get();
}
bool DrawNationalArmourResearchArtworkV87(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFRRUTacticalOptionV82& Row,float X,float Y,float Size)
{
    if(!NationalArmourResearchV87(Row))return false;
    const int32 Column=Row.Id.EndsWith(TEXT(".crew"))?0:Row.Id.EndsWith(TEXT(".repair"))?1:Row.Id.EndsWith(TEXT(".prepare"))?2:INDEX_NONE;
    if(Column==INDEX_NONE)return false;
    // Loaded with the existing artwork bank before the menu is presented, so
    // changing research pages never starts a synchronous texture import/load.
    auto* Art=NationalArmourResearchAtlasV87();if(!FrontlineTextureReady(Art))return false;
    const int32 Base=Row.Id.StartsWith(TEXT("fr."))?0:6;
    DrawNationalResearchPhotoV82(UI,B,Row,Art,Base+Column,Base+Column+3,3,4,X,Y,Size,Size);return true;
}
FString FRRUResearchBlockTextV82(const FString& Reason)
{
    if(Reason.Contains(TEXT("Armour research is not available at the unlocked campaign date")))return WarRoom52::L(TEXT("Armour has not yet been issued to this campaign."),TEXT("本战役尚未配发装甲。"));
    if(Reason.Contains(TEXT("Insufficient research points")))return WarRoom52::L(TEXT("Not enough research points."),TEXT("研究点不足。"));
    if(Reason.Contains(TEXT("Prerequisite not purchased")))return WarRoom52::L(TEXT("Complete the required training first."),TEXT("请先完成前置训练。"));
    if(Reason.Contains(TEXT("active encounter")))return WarRoom52::L(TEXT("Return to operations before authorising training."),TEXT("请返回作战室后批准训练。"));
    return WarRoom52::L(TEXT("This training cannot be authorised yet."),TEXT("这项训练暂时无法批准。"));
}

void DrawTechnology(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    const float PageTitleEm=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?32.f:49.f;
    if(WarfareFRRUCampaignV82::Active(B))
    {
        const bool Replay=WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B);
        UI.PageSurfaceV73(0,0,1920,1080,false,false);UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);UI.PhysicalPanelV74(80,252,1152,676);UI.PhysicalPanelV74(1272,252,552,676,true);
        UI.Image(NationalTextureV82(WarfareFRRUCampaignV82::Nation(B),TEXT("Flag")),76,80,76,48);
        UI.BilingualTitle(TEXT("COMMAND UPGRADES"),180,64,1080,65,PageTitleEm,Paper);
        if(Replay)UI.RoleLabel(EFrontlineTextRoleV8210::Label,WarRoom52::L(TEXT("ULTIMATE ARCHIVE"),TEXT("终极档案室")),1328,74,480,54,22,Muted,2);
        else UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::FromInt(B->GetResearchPoints())+WarRoom52::L(TEXT("  RESEARCH POINTS"),TEXT("  研究点")),1328,74,480,54,24,Amber,2);
        const auto Options=WarfareFRRUCampaignV82::Options(B);
        auto& State=ScreenState(UI.HUD.GetOwningPlayerController());SynchroniseNationalResearchV82(B,State);
        State.FRRUResearchPageV87=FMath::Clamp(State.FRRUResearchPageV87,0,1);const bool Armour=State.FRRUResearchPageV87==1;
        UI.PageTabV73(72,PageCanvasV73::TabY,548,WarRoom52::L(TEXT("FIELD ORDERS"),TEXT("前线战术")),TEXT("FRRUResearchOrdersV87"),!Armour);
        UI.PageTabV73(638,PageCanvasV73::TabY,548,WarRoom52::L(TEXT("ARMOUR SCHOOL"),TEXT("装甲学院")),TEXT("FRRUResearchArmourV87"),Armour);
        TArray<int32> Visible;for(int32 I=0;I<Options.Num();++I)if(NationalArmourResearchV87(Options[I])==Armour)Visible.Add(I);
        if(!Visible.Contains(State.TechnologyFocus))State.TechnologyFocus=Visible.IsEmpty()?INDEX_NONE:Visible[0];
        for(int32 Card=0;Card<Visible.Num();++Card)
        {
            const int32 I=Visible[Card];const auto& Row=Options[I];
            const float Size=Armour?276.f:148.f,X=Armour?130+Card*374.f:136+(Card%4)*270.f,Y=Armour?344.f:306+(Card/4)*196.f;
            UI.PhysicalPanelV74(X,Y,Size,Size);
            // Archived cells are original period-style photographic illustrations.
            const int32 Cell=(Row.Index/3)*6+Row.Index%3;
            if(Armour)DrawNationalArmourResearchArtworkV87(UI,B,Row,X+6,Y+6,Size-12);
            else if(!DrawNationalTacticalArtworkV82(UI,B,Row,X+6,Y+6,136))
                DrawNationalResearchPhotoV82(UI,B,Row,NationalAtlasV82(WarfareFRRUCampaignV82::Nation(B),TEXT("RESEARCH")),Cell,Cell+3,6,4,X+6,Y+6,136,136);
            UI.ConsoleSelectionV79(X,Y,Size,Size,Row.Owned,UI.Hovered(X,Y,Size,Size));
            UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Row.Title,X-(Armour?12.f:34.f),Y+Size+6,Armour?300.f:216.f,Armour?64.f:36.f,Armour?24.f:19.f,Row.Owned?Paper:Muted,1);
            UI.Hit(X,Y,Size,Size,FName(*((Replay?TEXT("FRRUOption:"):TEXT("FRRUOptionFocus:"))+Row.Id)),50);
            if(UI.Hovered(X,Y,Size,Size))State.TechnologyFocus=I;
        }
        if(Options.IsValidIndex(State.TechnologyFocus))
        {
            const auto& Row=Options[State.TechnologyFocus];const FLinearColor Ink=Paper;
            const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
            UI.BilingualTitle(WarfareFRRUTextV82::English(Row.Id,TEXT("title"),Row.Title),Row.Title,1300,286,496,90,Chinese?28.f:37.f,Ink);UI.Description(Row.Description,1300,394,496,170,20,Ink);
            UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Row.Tradeoff,1300,592,496,156,18,Muted);
            if(Replay)UI.RoleLabel(EFrontlineTextRoleV8210::Label,Row.Owned?WarRoom52::L(TEXT("ENABLED"),TEXT("已启用")):WarRoom52::L(TEXT("DISABLED"),TEXT("已停用")),1300,770,496,36,20,Ink);
            else UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::FromInt(Row.Cost)+WarRoom52::L(TEXT("  RESEARCH POINTS"),TEXT("  研究点")),1300,770,496,36,20,Ink);
            if(Replay)UI.PageButtonV73(1300,838,496,56,Row.Owned?WarRoom52::L(TEXT("DISABLE"),TEXT("停用训练")):WarRoom52::L(TEXT("ENABLE"),TEXT("启用训练")),TEXT(""),FName(*(TEXT("FRRUOption:")+Row.Id)),B->Phase==EFrontlinePhase::Briefing,true,60);
            else if(!Row.Owned)
            {
                UI.PageButtonV73(1300,838,496,56,WarRoom52::L(TEXT("AUTHORISE"),TEXT("批准研究")),TEXT(""),FName(*(TEXT("FRRUOption:")+Row.Id)),Row.BlockReason.IsEmpty(),true,60);
                if(!Row.BlockReason.IsEmpty())
                {
                    const FString Reason=FRRUResearchBlockTextV82(Row.BlockReason);
                    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Reason,1300,899,496,25,15,NoticeWarningV75);
                    if(UI.Hovered(1300,838,496,56))
                    {UI.PaperSheet(1280,724,536,106);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Reason,1302,744,492,64,17,FLinearColor(.035f,.038f,.033f));}
                }
            }
        }
        UI.PageButtonV73(1430,958,378,58,WarRoom52::L(TEXT("RETURN TO OPERATIONS"),TEXT("返回作战室")),TEXT("Esc"),TEXT("TechnologyBack"));return;
    }
    UI.PageSurfaceV73(0,0,1920,1080,false,false);
    UI.PhysicalPanelV74(80,252,1152,676);
    UI.PhysicalPanelV74(1272,252,552,676,true);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    DrawFactionFlag(UI,B->GetPlayerFaction(),76,80,76,48);
    UI.BilingualTitle(TEXT("COMMAND UPGRADES"),180,64,1080,65,PageTitleEm,Paper);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::Printf(TEXT("%d  研究点"),B->GetResearchPoints()),1328,74,480,54,24,Amber,2);
    auto& State=ScreenState(UI.HUD.GetOwningPlayerController());const auto& Nodes=B->GetTechnologyNodes();
    State.TechnologyPage=FMath::Clamp(State.TechnologyPage,0,2);
    UI.PageTabV73(72,PageCanvasV73::TabY,362,TEXT("步兵与炮兵"),TEXT("V32TechPrevious"),State.TechnologyPage==0);
    UI.PageTabV73(450,PageCanvasV73::TabY,362,TEXT("前沿配属"),TEXT("V32TechNext"),State.TechnologyPage==1,Nodes.Num()>16);
    UI.PageTabV73(828,PageCanvasV73::TabY,362,WarRoom52::L(TEXT("ARMOUR SCHOOL"),TEXT("装甲学院")),TEXT("V63TechArmour"),State.TechnologyPage==2);
    UI.PageButtonV73(72,958,340,58,TEXT("返回作战室"),TEXT("Esc"),TEXT("TechnologyBack"));
    if(State.TechnologyPage==2){DrawArmourResearchV63(UI,B);return;}
    constexpr float Column=278;
    const bool Late=State.TechnologyPage==1;
    const float StartY=320,Row=Late?200.f:144.f,CardH=Late?168.f:132.f,CardW=CardH,StartX=240-CardW*.5f;
    auto Visible=[&](int32 Id){return Nodes.IsValidIndex(Id)&&((Id>=16)==Late);};
    auto Position=[&](int32 Id){return FVector2D(StartX+Nodes[Id].Branch*Column,StartY+(Nodes[Id].Tier-(Late?4:0))*Row);};
    for(int32 Branch=0;Branch<4;++Branch)
    {
        const auto* N=Nodes.FindByPredicate([&](const FFrontlineTechnologyNode& A){return A.Branch==Branch&&A.Tier==(Late?4:0);});
        if(N)
        {
            if(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese)
                UI.RoleLabel(EFrontlineTextRoleV8210::Label,N->BranchTitle,112+Branch*Column,270,256,34,13,Muted,1);
            else UI.BilingualLabel(N->BranchTitle,112+Branch*Column,270,256,34,12,Muted,1);
        }
    }
    for(const auto& N:Nodes)if(Visible(N.Id))
    {
        const auto P=Position(N.Id);
        for(int32 Link=0;Link<2;++Link)
        {
            const int32 Pre=Link?N.SecondaryPrerequisite:N.Prerequisite;if(!Visible(Pre))continue;
            const auto A=Position(Pre);const auto C=B->HasTechnology(Pre)?FLinearColor(.46f,.49f,.37f,.88f):FLinearColor(.17f,.20f,.18f,.74f);
            if(Nodes[Pre].Branch==N.Branch)UI.ConsoleSeamV79(A.X+CardW*.5f,A.Y+CardH,P.X+CardW*.5f,P.Y,C,2);
            else
            {
                const bool Right=N.Branch>Nodes[Pre].Branch;const float Edge=A.X+(Right?CardW:0),Ch=Edge+(Right?21:-21),End=P.X+CardW*.5f;
                const float CY=P.Y-(Link?9:18);
                UI.ConsoleSeamV79(Edge,A.Y+CardH*.5f,Ch,A.Y+CardH*.5f,C);UI.ConsoleSeamV79(Ch,A.Y+CardH*.5f,Ch,CY,C);
                UI.ConsoleSeamV79(Ch,CY,End,CY,C);UI.ConsoleSeamV79(End,CY,End,P.Y,C);
            }
        }
    }
    int32 Hovered=INDEX_NONE;
    for(const auto& N:Nodes)if(Visible(N.Id))
    {
        const auto P=Position(N.Id);const bool Owned=B->HasTechnology(N.Id),Available=FrontlineUltimateArchiveV8215::Active(B)||B->TechnologyBlockReason(N.Id).IsEmpty(),Hover=UI.Hovered(P.X,P.Y,CardW,CardH);
        if(Hover){Hovered=N.Id;State.TechnologyFocus=N.Id;}
        // The archive material supplies its paper edge. A thin warm support
        // replaces the recessed black skin without adding another photo matte.
        UI.ConsoleArtFrameV79(P.X,P.Y,CardW,CardH,Hover,Owned);
        const auto Art=ResolveTechnologyArtworkV71(B,N.Id);
        DrawResearchPhotoV72(UI,B,Art,N.Id,Owned,false,P.X,P.Y,CardW,CardH);
        UI.Hit(P.X,P.Y,CardW,CardH,FName(*FString::Printf(TEXT("%s%d"),Available?TEXT("TechBuy"):TEXT("TechFocus"),N.Id)),50);
    }
    if(Nodes.IsValidIndex(Hovered))
    {
        const auto& N=Nodes[Hovered];const bool Owned=B->HasTechnology(N.Id);
        const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
        UI.BilingualTitle(N.Title,1296,282,512,70,Chinese?24.f:34.f,Paper);
        UI.Description(FrontlinePlayerGuideV43::Technology(B,N),1296,378,512,316,18,Paper,false);
        FString Reason=FrontlineUltimateArchiveV8215::Active(B)?(Owned?WarRoom52::L(TEXT("Equipped. Select to remove."),TEXT("已配备。点击移除。")):WarRoom52::L(TEXT("Select to equip."),TEXT("点击配备。"))):(Owned?WarRoom52::L(TEXT("RESEARCHED"),TEXT("已完成研究")):B->TechnologyBlockReason(N.Id));
        const FString Cost=(Owned||FrontlineUltimateArchiveV8215::Active(B))?TEXT(""):WarRoom52::L(TEXT("Research cost: "),TEXT("研究消耗："))+FString::FromInt(N.Cost)+WarRoom52::L(N.Cost==1?TEXT(" point"):TEXT(" points"),TEXT(" 点"));
        for(int32 Pre:{N.Prerequisite,N.SecondaryPrerequisite})if(Nodes.IsValidIndex(Pre)&&!Visible(Pre))
        {
            const FString Title=FrontlineLocalizationV35::Text(Nodes[Pre].Title);
            if(!Reason.Contains(Title)&&!Reason.Contains(Nodes[Pre].Title))
                Reason+=FString(TEXT("\n"))+PageCanvasV73::L(TEXT("Prerequisite research: "),TEXT("前置研究："))+Title;
        }
        UI.PageRuleV74(1296,730,512);
        if(!Owned)UI.RoleLabel(EFrontlineTextRoleV8210::Text,Cost,1296,762,512,30,17,Muted);
        if(!Reason.IsEmpty())UI.RoleParagraph(EFrontlineTextRoleV8210::DarkText,Reason,1296,Owned?762:804,512,Owned?132:90,17,Owned?Allied:NoticeWarningV75);
    }
    else
    {
        const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
        UI.BilingualTitle(TEXT("Research"),TEXT("研究"),1296,282,512,58,Chinese?24.f:34.f,Paper);
    }
}
