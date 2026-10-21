void DrawMenuLink(const FFrontlineCanvas& UI,float X,float Y,float W,const FString& Label,const FString& Key,FName Command,bool Primary=false)
{
    const bool Hover=UI.Hovered(X,Y,W,57);
    if (Hover) UI.Gradient(X-14,Y,W+28,57,FLinearColor(.33f,.31f,.23f,.35f),FLinearColor(0,0,0,0));
    UI.RoleLabel(EFrontlineTextRoleV8210::Plate,Label,X,Y+7,W,42,Primary?29:23,Hover||Primary?Paper:Muted);
    UI.Shortcut(X,Y,W,57,Key,50);UI.Hit(X,Y,W,57,Command,50);
}
UTexture2D* FRRUArtworkV82(int32 Nation,const TCHAR* Kind)
{
    if(Nation!=2&&Nation!=3)return nullptr;
    static TMap<FString,TStrongObjectPtr<UTexture2D>> Images;
    const FString Name=FString::Printf(TEXT("T_%s_%s"),Nation==2?TEXT("FR"):TEXT("RU"),Kind);
    const FString Path=FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/UI/%s.%s"),*Name,*Name);
    if(!FrontlineUIAssetsV8213::BaselineMode())return FrontlineUIAssetsV8213::Texture(Path);
    auto* Image=Images.Find(Name);
    if(!Image)if(auto* Loaded=FrontlineUIAssetsV8213::Texture(Path))Image=&Images.Add(Name,TStrongObjectPtr<UTexture2D>(Loaded));
    if(!Image)return nullptr;
    return Image->Get();
}
void DrawFRRUArtworkV82(const FFrontlineCanvas& UI,int32 Nation,const TCHAR* Kind,float X,float Y,float W,float H)
{
    if(auto* Image=FRRUArtworkV82(Nation,Kind))
    {
        const float Aspect=float(Image->GetSizeX())/FMath::Max(1,Image->GetSizeY());
        const float FitW=FMath::Min(W,H*Aspect),FitH=FitW/Aspect;UI.Image(Image,X+(W-FitW)*.5f,Y+(H-FitH)*.5f,FitW,FitH);
    }
}

void DrawMainMenu(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    UTexture2D* Cover=MenuCoverV63(B);
    if(WarfareFRRUCampaignV82::Active(B))Cover=FRRUArtworkV82(int32(WarfareFRRUCampaignV82::Nation(B)),TEXT("Cover"));
    const bool NationalCover=Cover&&Cover->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/UI/"));
    if(Cover)UI.Image(Cover,0,0,1920,1080);else if(WarfareFRRUCampaignV82::Active(B))UI.Rect(0,0,1920,1080,FLinearColor(.014f,.018f,.018f,1));else DrawMenuArtwork(UI,false);
    if(NationalCover)UI.Gradient(0,0,850,1080,FLinearColor(.006f,.012f,.014f,.73f),FLinearColor(.006f,.012f,.014f,0));
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    // The photograph carries the material and atmosphere; type is live ink,
    // not lettering enlarged with the image or a second steel enclosure.
    const FLinearColor Ivory(.81f,.80f,.73f),Quiet=NationalCover?FLinearColor(.72f,.71f,.65f):FLinearColor(.43f,.46f,.43f);
    DrawBrandTitleV72(UI,122,174,493,271);
    UI.Line(129,500,505,500,FLinearColor(.42f,.40f,.31f,.44f));
    float Y=534;
    auto Main=[&](const FString& Label,FName Name,bool Enabled=true)
    {
        const bool Over=Enabled&&UI.Hovered(125,Y,424,53);
        if(Over){UI.Line(130,Y+45,481,Y+45,FLinearColor(.67f,.62f,.46f,.65f));UI.Rect(109,Y+18,3,18,Amber);}
        const FString Value=UI.DisplayText(Label);const auto Ink=Enabled?(Over?Paper:Ivory):FLinearColor(.25f,.28f,.27f);
        UI.RoleLabel(EFrontlineTextRoleV8210::Plate,Label,130,Y+3,385,43,29,Ink);
        if(Enabled)UI.Hit(125,Y,424,53,Name,51);
        Y+=68;
    };
    Main(TEXT("继续战役"),TEXT("ContinueCampaign"),!B->GetCampaignRuns().IsEmpty()
        ||WarfareFRRUCampaignV82::Archives(B).ContainsByPredicate([](const FFRRUCampaignSummaryV82& Row){return WarfareFRRUCampaignV82::CampaignsAvailable(Row.PlayerNation);}));
    Main(TEXT("新战役"),TEXT("NewCampaign"));Main(TEXT("战役档案"),TEXT("CampaignArchives"));
    UI.Line(129,758,505,758,FLinearColor(.42f,.40f,.31f,.28f));
    auto Minor=[&](float X,float YY,const FString& Label,FName Name,const FString& Key=TEXT(""))
    {const bool Over=UI.Hovered(X,YY,189,48);const auto Value=UI.DisplayText(Label);
     UI.RoleLabel(EFrontlineTextRoleV8210::Plate,Label,X,YY+3,187,37,20,Over?Paper:Quiet);
     UI.Hit(X,YY,189,48,Name,51);UI.Shortcut(X,YY,189,48,Key,51);};
    Minor(130,842,TEXT("设置"),TEXT("Settings"),TEXT("O"));Minor(336,842,TEXT("战地手册"),TEXT("Help"),TEXT("H"));
    Minor(130,782,TEXT("战功册"),TEXT("AchievementsV76"));
    if(FrontlineUltimateArchiveV8215::Available(EFrontlineFaction::British)||FrontlineUltimateArchiveV8215::Available(EFrontlineFaction::German))
        Minor(336,782,WarRoom52::L(TEXT("ULTIMATE ARCHIVE"),TEXT("终极档案室")),TEXT("V8215ArchiveOpen"));
    Minor(130,902,TEXT("制作与鸣谢"),TEXT("Credits"));Minor(336,902,TEXT("退出游戏"),TEXT("QuitGame"),TEXT("Esc"));
    auto& S=ScreenState(UI.HUD.GetOwningPlayerController());
    if(S.ExitConfirmV61)
    {
        UI.Rect(0,0,1920,1080,FLinearColor(.005f,.009f,.010f,.76f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),90);
        UI.Skin(0,606,369,708,342,14,FLinearColor(.72f,.76f,.68f,1));
        UI.Label(WarRoom52::L(TEXT("EXIT GAME?"),TEXT("退出游戏？")),648,405,624,55,34,Paper,1);
        UI.Paragraph(WarRoom52::L(TEXT("Campaign progress is saved."),TEXT("战役进度已保存。")),648,485,624,68,22,Muted);
        UI.Button(648,602,302,62,WarRoom52::L(TEXT("STAY"),TEXT("留下")),TEXT("Esc"),TEXT("ExitCancelV61"),true,false,91);
        UI.Button(970,602,302,62,WarRoom52::L(TEXT("EXIT GAME"),TEXT("退出游戏")),TEXT(""),TEXT("ExitConfirmV61"),true,false,91);
    }
}

void DrawCampaignSelection(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    FactionSelectionV82::Draw(UI,B);
}
void DrawCampaignArchives(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    UI.PageSurfaceV73(0,0,1920,1080,true);UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    const FLinearColor Ink(.028f,.032f,.029f),Faded(.07f,.075f,.064f);
    UI.Label(TEXT("CAMPAIGN RECORDS"),72,64,1390,65,40,Ink);
    const auto& Runs=B->GetCampaignRuns();auto& S=ScreenState(UI.HUD.GetOwningPlayerController());
    UI.PageButtonV73(1328,78,480,56,S.FRRUArchivesV82?WarRoom52::L(TEXT("BRITAIN / GERMANY"),TEXT("英国 / 德国")):WarRoom52::L(TEXT("FRANCE / RUSSIA"),TEXT("法国 / 俄国")),TEXT(""),TEXT("FRRUArchiveTabV82"));
    if(S.FRRUArchivesV82)
    {
        if(!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French)&&!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian))
            UI.Label(WarRoom52::L(TEXT("NOT YET AVAILABLE"),TEXT("暂未开放")),112,264,1696,40,23,Faded,1);
        const auto& Rows=WarfareFRRUCampaignV82::Archives(B);S.ArchiveOffset=FMath::Clamp(S.ArchiveOffset,0,FMath::Max(0,Rows.Num()-5));
        UI.PageButtonV73(112,198,260,54,S.ArchiveSelection.Num()==Rows.Num()&&!Rows.IsEmpty()?TEXT("取消全选"):TEXT("全选"),TEXT(""),TEXT("ArchiveSelectAll"),!Rows.IsEmpty());
        UI.PageButtonV73(392,198,360,54,FString::Printf(TEXT("删除所选（%d）"),S.ArchiveSelection.Num()),TEXT(""),TEXT("ArchiveDeleteSelected"),!S.ArchiveSelection.IsEmpty());
        for(int32 I=0;I<5&&I+S.ArchiveOffset<Rows.Num();++I)
        {
            const auto& Row=Rows[Rows.Num()-1-I-S.ArchiveOffset];const float Y=326+I*106;const FString Id=Row.Id.ToString(EGuidFormats::Digits);
            if(S.ArchiveSelection.Contains(Row.Id))UI.Rect(112,Y-4,1696,96,FLinearColor(.18f,.16f,.10f,.10f));
            UI.Box(126,Y+26,28,28,FLinearColor(0,0,0,0),Faded*.65f);if(S.ArchiveSelection.Contains(Row.Id)){UI.Line(131,Y+40,139,Y+48,Ink,2);UI.Line(139,Y+48,150,Y+30,Ink,2);}
            UI.Hit(112,Y,58,90,FName(*(TEXT("FRRUSelect:")+Id)),51);
            UI.Label(Row.PlayerNation==EFRRUNationV81::French?WarRoom52::L(TEXT("FRENCH CAMPAIGN"),TEXT("法军战役")):WarRoom52::L(TEXT("RUSSIAN CAMPAIGN"),TEXT("俄军战役")),198,Y+2,546,36,27,Ink);
            UI.Label(FrontlineDifficultyV30::Label(Row.Difficulty),198,Y+46,546,32,20,Faded);
            UI.Label(FString::Printf(TEXT("%d / 10 已完成"),Row.Completed),810,Y+4,350,32,21,Faded);
            UI.Label(Row.UpdatedUtc.Left(10),810,Y+46,350,29,18,Faded);
            if(FrontlineAchievementsV76::CampaignAssistedV8222(Row.Id,Row.HonourEligible))
                UI.Label(WarRoom52::L(TEXT("CHEAT-ASSISTED"),TEXT("使用过作弊")),1166,Y+6,290,30,18,FLinearColor(.36f,.075f,.038f),2);
            if(WarfareFRRUCampaignV82::CampaignId(B)==Row.Id)UI.Label(TEXT("当前战役"),1190,Y+46,280,30,20,Faded,2);
            const bool Available=WarfareFRRUCampaignV82::CampaignsAvailable(Row.PlayerNation);
            UI.PageButtonV73(1510,Y+17,160,56,Available?FString(TEXT("继续")):WarRoom52::L(TEXT("UNAVAILABLE"),TEXT("暂未开放")),TEXT(""),FName(*(TEXT("FRRUResume:")+Id)),Available,false,51);
            UI.PageButtonV73(1688,Y+17,120,56,TEXT("删除"),TEXT(""),FName(*(TEXT("FRRUDelete:")+Id)),true,false,51);
            UI.Line(112,Y+94,1808,Y+94,FLinearColor(.12f,.13f,.10f,.30f));
        }
        if(Rows.IsEmpty())UI.Paragraph(WarRoom52::L(TEXT("No campaign records."),TEXT("尚无战役档案。")),112,390,1696,100,28,Ink);
        if(!S.ArchiveNotice.IsEmpty())UI.Label(S.ArchiveNotice,112,880,1696,40,20,FLinearColor(.36f,.075f,.038f));
        UI.PageButtonV73(72,958,280,58,TEXT("返回"),TEXT("Esc"),TEXT("MenuHome"));
        UI.PageButtonV73(1296,958,244,58,TEXT("较新"),TEXT("←"),TEXT("ArchiveNewer"),S.ArchiveOffset>0);
        UI.PageButtonV73(1556,958,252,58,TEXT("较早"),TEXT("→"),TEXT("ArchiveOlder"),S.ArchiveOffset+5<Rows.Num());
        if(!S.ArchivePendingDelete.IsEmpty())
        {
            UI.Rect(0,0,1920,1080,FLinearColor(.003f,.004f,.005f,.78f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);UI.PageSurfaceV73(552,324,816,414,true);
            UI.Label(TEXT("删除战役档案？"),595,366,725,52,31,Ink);
            UI.Paragraph(TEXT("这条战役的进度和研究记录将被删除。"),597,446,725,104,23,Faded);
            UI.PageButtonV73(597,616,342,62,TEXT("保留档案"),TEXT("Esc"),TEXT("ArchiveDeleteCancel"),true,true,90);
            UI.PageButtonV73(965,616,352,62,TEXT("确认删除"),TEXT(""),TEXT("ArchiveDeleteConfirm"),true,false,90);
        }
        return;
    }
    S.ArchiveOffset=FMath::Clamp(S.ArchiveOffset,0,FMath::Max(0,Runs.Num()-5));
    UI.PageButtonV73(112,198,260,54,S.ArchiveSelection.Num()==Runs.Num()&&!Runs.IsEmpty()?TEXT("取消全选"):TEXT("全选"),TEXT(""),TEXT("ArchiveSelectAll"),!Runs.IsEmpty());
    UI.PageButtonV73(392,198,360,54,FString::Printf(TEXT("删除所选（%d）"),S.ArchiveSelection.Num()),TEXT(""),TEXT("ArchiveDeleteSelected"),!S.ArchiveSelection.IsEmpty());
    UI.Label(FString::Printf(TEXT("%d 条战役档案"),Runs.Num()),1296,198,512,54,21,Faded,2);
    UI.Line(112,288,1808,288,Faded*.4f);
    for(int32 Row=0;Row<5&&Row+S.ArchiveOffset<Runs.Num();++Row)
    {
        const int32 I=Runs.Num()-1-Row-S.ArchiveOffset;const auto& R=Runs[I];const float Y=326+Row*106;
        const bool Selected=S.ArchiveSelection.Contains(R.Id);const FString Id=R.Id.ToString(EGuidFormats::Digits);
        if(Selected)UI.Rect(112,Y-4,1696,96,FLinearColor(.18f,.16f,.10f,.10f));
        UI.Box(126,Y+26,28,28,FLinearColor(0,0,0,0),Faded*.65f);
        if(Selected){UI.Line(131,Y+40,139,Y+48,Ink,2);UI.Line(139,Y+48,150,Y+30,Ink,2);}
        UI.Hit(112,Y,58,90,FName(*(TEXT("ArchiveSelect")+Id)),51);
        DrawFactionFlag(UI,R.Faction,198,Y+17,92,56);
        UI.Label(R.Faction==EFrontlineFaction::German?TEXT("德军战役"):TEXT("英军战役"),326,Y+2,428,36,27,Ink);
        UI.Label(FrontlineDifficultyV30::Label(R.Difficulty),326,Y+46,428,32,20,Faded);
        UI.Label(FString::Printf(TEXT("%d / %d 已完成"),FrontlineCampaignV52::ActualVictories(R),FrontlineCampaignV52::RouteOperationCount(R)),810,Y+4,350,32,21,Faded);
        UI.Label(R.UpdatedUtc.Left(10),810,Y+46,350,29,18,Faded);
        if(FrontlineAchievementsV76::CampaignAssistedV8222(R.Id,R.bAchievementEligibleV76))
            UI.Label(WarRoom52::L(TEXT("CHEAT-ASSISTED"),TEXT("使用过作弊")),1166,Y+6,290,30,18,FLinearColor(.36f,.075f,.038f),2);
        if(const auto* Active=B->GetCurrentCampaign();Active&&Active->Id==R.Id)UI.Label(TEXT("当前战役"),1190,Y+46,280,30,20,Faded,2);
        UI.PageButtonV73(1510,Y+17,160,56,TEXT("继续"),TEXT(""),FName(*FString::Printf(TEXT("CampaignResume%d"),I)),true,false,51);
        UI.PageButtonV73(1688,Y+17,120,56,TEXT("删除"),TEXT(""),FName(*(TEXT("ArchiveDeleteOne")+Id)),true,false,51);
        UI.Line(112,Y+94,1808,Y+94,FLinearColor(.12f,.13f,.10f,.30f));
    }
    if(Runs.IsEmpty())UI.Paragraph(WarRoom52::L(TEXT("No campaign records."),TEXT("尚无战役档案。")),112,390,1696,100,28,Ink);
    if(!S.ArchiveNotice.IsEmpty())UI.Label(S.ArchiveNotice,112,880,1696,40,20,FLinearColor(.36f,.075f,.038f));
    UI.Line(64,PageCanvasV73::FooterY,1856,PageCanvasV73::FooterY,Faded*.4f);
    UI.PageButtonV73(72,958,280,58,TEXT("返回"),TEXT("Esc"),TEXT("MenuHome"));
    UI.PageButtonV73(1296,958,244,58,TEXT("较新"),TEXT("←"),TEXT("ArchiveNewer"),S.ArchiveOffset>0);
    UI.PageButtonV73(1556,958,252,58,TEXT("较早"),TEXT("→"),TEXT("ArchiveOlder"),S.ArchiveOffset+5<Runs.Num());
    if(!S.ArchivePendingDelete.IsEmpty())
    {
        UI.Rect(0,0,1920,1080,FLinearColor(.003f,.004f,.005f,.78f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);
        UI.PageSurfaceV73(552,324,816,414,true);
        UI.Label(TEXT("删除战役档案？"),595,366,725,52,31,Ink);
        UI.Paragraph(FString::Printf(TEXT("将删除所选的 %d 条战役及其研究和进度。此操作无法在游戏中撤销。"),S.ArchivePendingDelete.Num()),597,443,721,109,21,Ink);
        UI.PageButtonV73(597,616,342,62,TEXT("保留档案"),TEXT("Esc"),TEXT("ArchiveDeleteCancel"),true,true,90);
        UI.PageButtonV73(965,616,352,62,TEXT("确认删除"),TEXT(""),TEXT("ArchiveDeleteConfirm"),true,false,90);
    }
}
void DrawDossier(const FFrontlineCanvas& UI,bool Is1944,AWarfareFrontlineGameMode* B)
{
    if(WarfareFRRUCampaignV82::Active(B))
    {
        FRRUWarRoomV82::Draw(UI,B);
        return;
    }
    if(!B->GetCurrentCampaign() || !B->GetSelectedCampaignMission())
    {ScreenState(UI.HUD.GetOwningPlayerController()).Dossier=1918;DrawCampaignSelection(UI,B);return;}
    if(!FrontlineMapSceneV61::IsActive(Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController())))DrawMenuArtwork(UI,false);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    DrawWarRoomV52(UI,B);
}
void DrawFieldReport(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    if(FrontlineReportV839::Active(UI.HUD.GetOwningPlayerController(),B)){ReportUIV839::Draw(UI,B);return;}
    const bool Won=B->Phase==EFrontlinePhase::Victory,German=B->GetPlayerFaction()==EFrontlineFaction::German;
    const auto* Mission=B->GetEncounterCampaignMission();const auto* Run=B->GetCurrentCampaign();
    UI.Rect(0,0,1920,1080,FLinearColor(.004f,.009f,.013f,.84f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    UI.PaperSheet(360,134,1200,788);
    const FLinearColor Ink(.028f,.032f,.029f),Faded(.07f,.075f,.064f);
    {TGuardValue<bool> English(UI.bEnglishPrimaryV45,true);UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("FIELD REPORT"),410,178,690,35,24,Faded);}
    UI.RoleLabel(EFrontlineTextRoleV8210::Typed,Mission?FrontlineLocalizationV35::English(Mission->DateLabel):TEXT("1917"),1102,182,406,30,20,Faded,2);
    {TGuardValue<bool> English(UI.bEnglishPrimaryV45,true);UI.BilingualLabel(German?(Won?TEXT("SIEG"):TEXT("NIEDERLAGE")):(Won?TEXT("VICTORY"):TEXT("DEFEAT")),403,230,825,90,86,Ink,0,EFrontlineTextRoleV8210::Inscription);}
    UI.BilingualTitle(Mission?Mission->Title:TEXT("西线行动"),410,321,823,54,29,Ink);
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,ChineseStatus(B->EndReason),410,379,823,54,21,Faded);
    DrawCasualtyFlag(UI,B,0,1301,247,200,122);
    UI.Line(410,446,1508,446,FLinearColor(.20f,.20f,.14f,.40f));
    // The two armies share the same column axes for flags, headings and values.
    constexpr float ArmyX[2]={474.f,929.f},LabelX=701.f;
    for(int32 Team=0;Team<2;++Team)
    {
        DrawCasualtyFlag(UI,B,Team,ArmyX[Team]-30,466,60,36);
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,Team?TEXT("敌军"):TEXT("我军"),ArmyX[Team]-150,511,300,31,20,Faded,1);
    }
    const TCHAR* Labels[]={TEXT("阵亡"),TEXT("存活"),TEXT("士气"),TEXT("装甲击毁")};
    for(int32 I=0;I<4;++I)
    {
        const float Y=552+I*44;UI.ReportStatLabel(Labels[I],LabelX,Y,33,Faded);
        for(int32 Team=0;Team<2;++Team)
        {
            const int32 Value=I==0?B->Casualties[Team]:I==1?B->LivingSoldiers(Team):I==2?FMath::RoundToInt(B->Morale[Team]):B->CombatRecordV65[Team].Total.Armour;
            UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::FromInt(FrontlineUIMotionV83::RollNumber(Value,UI.MotionNumberAgeV83,UI.MotionStableV83)),ArmyX[Team]-150,Y,300,35,33,Ink,1);
        }
        UI.Line(410,Y+35,990,Y+35,FLinearColor(.20f,.20f,.14f,.16f));
    }
    const FString ActionClock=ClockText(B->Elapsed);
    const FString ActionTime=UI.DisplayText(FString::Printf(TEXT("行动时间  %s"),*ActionClock));
    const int32 ClockOffset=ActionTime.Find(ActionClock,ESearchCase::CaseSensitive,ESearchDir::FromEnd);
    const FString ActionTimeLabel=ClockOffset!=INDEX_NONE?ActionTime.Left(ClockOffset):ActionTime;
    UI.RoleText(EFrontlineTextRoleV8210::Label,ActionTimeLabel,411,751,21,Faded);
    if(ClockOffset!=INDEX_NONE)UI.RoleText(EFrontlineTextRoleV8210::Numeral,ActionClock,411+UI.RoleWidth(EFrontlineTextRoleV8210::Label,ActionTimeLabel,21),751+21*(FFrontlineCanvas::ContainsCJKV45(ActionTimeLabel)?1.160f:.810f)-26*.810f,26,Faded);
    // File the action's own map with the return, rather than borrowing a
    // technology illustration as an unrelated victory/defeat trophy.
    if(Mission)
    {
        UI.Skin(2,1030,467,478,199,8,FLinearColor(.89f,.90f,.85f,1));
        FrontlineLoadingPageV82::DrawOperationMapV8447(UI,B,Mission,1038,475,462,183,FLinearColor(.94f,.95f,.92f,1));
    }
    // Keep this return about the completed action. Research remains safely
    // receipted once; its available balance is shown in the operations room.
    const auto& Return=B->CombatRecordV65[0];
    if(Return.Total.Total()>0||Return.InfantryLost>0||Return.ArmourLost>0)
    {
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,WarRoom52::L(TEXT("ACTION SUMMARY"),TEXT("本场战果")),1034,686,465,32,22,Faded);
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?FString::Printf(TEXT("Enemy infantry killed: %d"),Return.Total.Infantry):FString::Printf(TEXT("敌军阵亡：%d"),Return.Total.Infantry)),1034,726,465,23,18,Ink);
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?FString::Printf(TEXT("Enemy armour destroyed: %d"),Return.Total.Armour):FString::Printf(TEXT("摧毁装甲：%d"),Return.Total.Armour)),1034,752,465,23,18,Ink);
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?FString::Printf(TEXT("Own losses: %d infantry, %d armour"),Return.InfantryLost,Return.ArmourLost):FString::Printf(TEXT("己方损失：步兵 %d，装甲 %d"),Return.InfantryLost,Return.ArmourLost)),1034,778,465,23,18,Ink);
    }
    UI.Button(409,815,349,64,FrontlineCampaignV51::IsFinalVictory(B)?FrontlineCampaignV51::Copy(TEXT("FinishReport")):Won?TEXT("继续战役"):TEXT("返回作战室"),TEXT("ENTER"),TEXT("CampaignAfterBattle"),true,true,50);
    UI.Button(783,815,349,64,TEXT("详细战报"),TEXT(""),TEXT("DetailedReportV839"),true,false,50);
    UI.Button(1157,815,349,64,TEXT("重试本次行动"),TEXT("R"),TEXT("ReplayEncounter"),true,false,50);
}

