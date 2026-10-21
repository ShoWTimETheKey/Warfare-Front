void DrawNationalCampaignEndingV82(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    FFRRUCampaignSummaryV82 Record;if(!WarfareFRRUCampaignV82::CurrentRecord(B,Record))return;
    const bool French=Record.PlayerNation==EFRRUNationV81::French;
    auto L=[](const TCHAR* E,const TCHAR* C){return WarRoom52::L(E,C);};
    const auto Roll=[&](int32 Value){return FString::FromInt(FrontlineUIMotionV83::RollNumber(Value,UI.MotionNumberAgeV83,UI.MotionStableV83));};
    UI.Rect(0,0,1920,1080,FLinearColor(.013f,.017f,.018f,1));
    if(auto* T=FRRUArtworkV82(int32(Record.PlayerNation),TEXT("Cover")))UI.ImageCover(T,0,0,1920,1080,FLinearColor(.76f,.77f,.74f,1));
    UI.Gradient(0,0,1420,1080,FLinearColor(.007f,.009f,.010f,.94f),FLinearColor(.007f,.009f,.010f,0));
    UI.Gradient(0,728,1920,352,FLinearColor(.008f,.011f,.012f,0),FLinearColor(.008f,.011f,.012f,.94f),true);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),70);
    UI.BilingualLabel(TEXT("REGIMENTAL RECORD"),TEXT("战役档案"),142,92,1160,35,22,Muted);
    const auto& Missions=B->GetCampaignMissions();if(!Missions.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(Missions.Last().DateLabel),142,140,1060,34,21,Paper);
    UI.BilingualLabel(French?TEXT("BEYOND SOMME-PY"):TEXT("THE ROAD FROM RIGA"),French?TEXT("越过索姆皮"):TEXT("里加归途"),136,232,1150,106,72,Paper,0,EFrontlineTextRoleV8210::Inscription);
    UI.RoleLabel(EFrontlineTextRoleV8210::Headline,French?L(TEXT("French Army"),TEXT("法国陆军")):L(TEXT("Imperial Russian Army"),TEXT("俄罗斯帝国陆军")),143,366,905,44,26,Amber);
    const FString First=French?
        L(TEXT("Verdun, the Aisne and the villages of the Marne remain in the regimental record. In September 1918, the French Fourth Army advances through Champagne. Beyond Somme-Py, the war continues."),TEXT("凡尔登、埃纳河与马恩河畔的村庄，已写入部队战史。1918年9月，法军第四集团军向香槟纵深推进。越过索姆皮，战争仍未结束。")):
        L(TEXT("At Riga, the guns cover a withdrawal. After the fighting in Galicia, the siege of Przemysl and the long struggle along the Eastern Front, the surviving companies take the road east."),TEXT("里加的炮火掩护着撤退。从加利西亚到普热梅希尔，再到东线漫长的阵地战，幸存的小队如今踏上东去的道路。"));
    const FString Second=French?
        L(TEXT("The company returns to its billets. Rifles are counted, ammunition is collected, and the roll is called. There are names that no one answers."),TEXT("部队返回驻地。清点枪支，收集弹药，重新点名。有些名字，再无人应答。")):
        L(TEXT("The army that set out in 1914 is breaking apart. Men carry their rifles through a country changed by revolution, while the fate of the regiments and the empire remains unsettled."),TEXT("1914年出征的军队正在瓦解。士兵背着步枪，穿过被革命改变的国土。各个团与帝国的命运，仍悬而未决。"));
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,First,145,440,832,164,23,Paper);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Second,145,638,832,151,23,Paper);
    constexpr float X=1078,W=690,Y=382;
    UI.Gradient(X-22,Y-20,W+44,392,FLinearColor(.008f,.011f,.012f,.86f),FLinearColor(.008f,.011f,.012f,.36f));
    UI.BilingualLabel(TEXT("CAMPAIGN RETURN"),TEXT("战役战果总录"),X,Y,W,38,24,Paper);
    const auto& Return=Record.CombatRecord;
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Infantry"),TEXT("步兵")),X+340,Y+54,142,28,18,Muted,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Armour"),TEXT("装甲")),X+504,Y+54,186,28,18,Muted,2);
    FFrontlineKillTallyV65 Categories[4];
    for(const auto& K:Return.InfantrySources)Categories[0].Add(K);
    for(const auto& K:Return.ArmourSources)Categories[1].Add(K);
    for(const auto& K:Return.NationalArmourSourcesV82)Categories[1].Add(K.Value);
    for(const auto& K:Return.SupportSources)Categories[2].Add(K);
    Categories[3]=Return.OtherSources;
    const FString Names[]={L(TEXT("Infantry sections"),TEXT("步兵小队")),L(TEXT("Armoured forces"),TEXT("装甲部队")),L(TEXT("Fire support"),TEXT("火力支援")),L(TEXT("Other confirmed kills"),TEXT("其他已确认战果")),L(TEXT("TOTAL"),TEXT("总计"))};
    int32 Detail=INDEX_NONE;
    for(int32 I=0;I<5;++I)
    {
        const float RY=Y+99+I*41;const auto& K=I<4?Categories[I]:Return.Total;
        if(I==4)UI.Line(X,RY-8,X+W,RY-8,FLinearColor(.56f,.55f,.49f,.4f));
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,Names[I],X,RY,324,30,19,I==4?Amber:Paper);
        UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(K.Infantry),X+340,RY,142,30,27,I==4?Amber:Paper,2);
        UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(K.Armour),X+504,RY,186,30,27,I==4?Amber:Paper,2);
        if(I<3&&UI.Hovered(X,RY,W,35))Detail=I;
    }
    UI.Line(X,Y+306,X+W,Y+306,FLinearColor(.56f,.55f,.49f,.30f));
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Own losses"),TEXT("己方损失")),X,Y+321,180,30,18,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Infantry"),TEXT("步兵")),X+204,Y+321,96,30,17,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(Return.InfantryLost),X+307,Y+321,105,30,24,Paper,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Armour"),TEXT("装甲")),X+450,Y+321,96,30,17,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(Return.ArmourLost),X+566,Y+321,124,30,24,Paper,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::Text,FString::FromInt(Return.BattlesRecorded)+L(TEXT(" completed engagements"),TEXT(" 场战斗")),X,Y+361,W,28,17,Muted);
    if(Return.bIncludesEarlierUnrecordedBattles)UI.RoleLabel(EFrontlineTextRoleV8210::Text,L(TEXT("Earlier returns have no unit breakdown."),TEXT("旧战果未记录各部明细。")),X,Y+400,W,25,16,NoticeInfoV75);
    if(Detail>=0)
    {
        TArray<TPair<FString,FFrontlineKillTallyV65>> Rows;
        if(Detail==0)for(int32 I=0;I<UE_ARRAY_COUNT(Return.InfantrySources);++I)if(Return.InfantrySources[I].Total())Rows.Add({B->GetRecruitName(EFrontlineRole(I)),Return.InfantrySources[I]});
        if(Detail==1)for(const auto& D:WarfareFRRUArmourV82::Definitions())if(const auto* K=Return.NationalArmourSourcesV82.Find(D.Id);K&&K->Total())Rows.Add({D.Name,*K});
        if(Detail==2)for(int32 I=0;I<UE_ARRAY_COUNT(Return.SupportSources);++I)if(Return.SupportSources[I].Total())Rows.Add({B->GetSupportName(EFrontlineSupport(I)),Return.SupportSources[I]});
        const float PX=1048,PY=FMath::Max(74.f,Y-94-Rows.Num()*32),H=94+Rows.Num()*32;
        const FLinearColor Ink(.03f,.034f,.03f);UI.PaperSheet(PX,PY,744,H);UI.RoleLabel(EFrontlineTextRoleV8210::Label,Names[Detail],PX+22,PY+16,700,32,22,Ink);
        if(Rows.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Text,L(TEXT("No confirmed kills."),TEXT("尚无确认战果。")),PX+22,PY+57,700,28,19,Ink);
        for(int32 I=0;I<Rows.Num();++I)
        {UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Rows[I].Key,PX+22,PY+58+I*32,438,28,20,Ink);UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::Printf(TEXT("%d / %d"),Rows[I].Value.Infantry,Rows[I].Value.Armour),PX+476,PY+58+I*32,246,28,23,Ink,2);}
    }
    UI.Line(143,826,1777,826,FLinearColor(.56f,.55f,.49f,.40f));
    const FString Labels[]={L(TEXT("OPERATIONS"),TEXT("行动")),L(TEXT("VICTORIES"),TEXT("胜利")),L(TEXT("DEFEATS"),TEXT("失败")),L(TEXT("COMMAND UPGRADES"),TEXT("指挥部升级"))};
    const int32 Values[]={Record.Completed,Record.Victories,Record.Defeats,Record.Orders};
    for(int32 I=0;I<UE_ARRAY_COUNT(Values);++I)
    {const float LX=143+I*301.f;UI.RoleLabel(EFrontlineTextRoleV8210::Label,Labels[I],LX,850,279,27,16,Muted);UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(Values[I]),LX,887,279,42,35,Paper);}
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("DIFFICULTY"),TEXT("难度")),1404,850,372,27,16,Muted,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::FieldValue,FrontlineDifficultyV30::Label(Record.Difficulty),1404,887,372,42,21,Paper,2);
    UI.Button(1355,970,422,58,L(TEXT("DETAILED CAMPAIGN REPORT"),TEXT("全战役详细战报")),TEXT(""),TEXT("CampaignDetailedReportV86"),true,false,80);
    UI.Button(143,970,438,58,L(TEXT("RETURN TO OPERATIONS"),TEXT("返回作战室")),TEXT("Enter"),TEXT("V51EndingOperations"),true,true,80);
    UI.Button(605,970,365,58,L(TEXT("CAMPAIGN RECORDS"),TEXT("战役档案")),TEXT(""),TEXT("V51EndingArchives"),true,false,80);
    UI.Button(994,970,337,58,L(TEXT("MAIN MENU"),TEXT("主菜单")),TEXT("Esc"),TEXT("V51EndingMenu"),true,false,80);
}
void DrawCampaignEndingV51(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    if(FrontlineReportV839::Active(UI.HUD.GetOwningPlayerController(),B)){ReportUIV839::Draw(UI,B);return;}
    if(WarfareFRRUCampaignV82::Active(B)){DrawNationalCampaignEndingV82(UI,B);return;}
    using namespace FrontlineCampaignV51;
    const auto* R=B->GetCurrentCampaign();if(!R)return;
    const bool German=R->Faction==EFrontlineFaction::German;
    const auto Roll=[&](int32 Value){return FString::FromInt(FrontlineUIMotionV83::RollNumber(Value,UI.MotionNumberAgeV83,UI.MotionStableV83));};
    UI.Rect(0,0,1920,1080,FLinearColor(.013f,.017f,.018f,1));
    if(auto* T=Photo(3))UI.ImageCover(T,0,0,1920,1080,FLinearColor(.76f,.77f,.74f,1));
    else DrawMenuArtwork(UI,false);
    UI.Gradient(0,0,1400,1080,FLinearColor(.007f,.009f,.010f,.94f),FLinearColor(.007f,.009f,.010f,0));
    UI.Gradient(0,728,1920,352,FLinearColor(.008f,.011f,.012f,0),FLinearColor(.008f,.011f,.012f,.94f),true);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),70);
    UI.BilingualLabel(Copy(TEXT("Record")),142,92,1160,35,22,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(Copy(TEXT("Date"))),142,140,1060,34,21,Paper);
    UI.BilingualLabel(Copy(TEXT("Title")),136,232,1150,106,72,Paper,0,EFrontlineTextRoleV8210::Inscription);
    UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Copy(German?TEXT("German.Subtitle"):TEXT("British.Subtitle")),143,366,905,44,26,Amber);
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Copy(German?TEXT("German.End1"):TEXT("British.End1")),145,440,832,157,23,Paper);
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Copy(German?TEXT("German.End2"):TEXT("British.End2")),145,624,832,164,23,Paper);
    // The final record remains part of the campaign archive, including defeated
    // and replayed engagements. Source rows reveal their unit-level breakdown.
    const auto& Record=R->CombatRecordV65;
    auto L=[](const TCHAR* E,const TCHAR* C){return FrontlineArmourV63::L(E,C);};
    constexpr float LedgerX=1050,LedgerW=727,LedgerY=339;
    UI.Gradient(LedgerX-22,LedgerY-16,LedgerW+44,457,FLinearColor(.008f,.011f,.012f,.86f),FLinearColor(.008f,.011f,.012f,.36f));
    UI.BilingualLabel(TEXT("CAMPAIGN CASUALTY RETURN"),TEXT("战役战果总录"),LedgerX,LedgerY,LedgerW,36,22,Paper);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Enemy infantry"),TEXT("敌军步兵")),LedgerX+306,LedgerY+54,188,25,16,Muted,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Armour destroyed"),TEXT("摧毁装甲")),LedgerX+510,LedgerY+54,217,25,16,Muted,2);
    FFrontlineKillTallyV65 Categories[4];
    for(const auto& K:Record.InfantrySources)Categories[0].Add(K);
    for(const auto& K:Record.ArmourSources)Categories[1].Add(K);
    for(const auto& K:Record.SupportSources)Categories[2].Add(K);
    Categories[3]=Record.OtherSources;
    const FString Names[]={L(TEXT("Infantry sections"),TEXT("步兵小队")),L(TEXT("Armoured forces"),TEXT("装甲部队")),L(TEXT("Fire support"),TEXT("火力支援")),L(TEXT("Mines and field weapons"),TEXT("地雷与阵地武器")),L(TEXT("TOTAL"),TEXT("总计"))};
    int32 Detail=-1;
    for(int32 I=0;I<5;++I)
    {
        const float Y=LedgerY+99+I*43;const auto& K=I<4?Categories[I]:Record.Total;
        if(I==4)UI.Line(LedgerX,Y-8,LedgerX+LedgerW,Y-8,FLinearColor(.56f,.55f,.49f,.4f));
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,Names[I],LedgerX,Y,305,30,I==4?20:18,I==4?Amber:Paper);
        UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(K.Infantry),LedgerX+306,Y,188,30,26,I==4?Amber:Paper,2);
        UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(K.Armour),LedgerX+510,Y,217,30,26,I==4?Amber:Paper,2);
        if(I<3&&UI.Hovered(LedgerX,Y,LedgerW,35))Detail=I;
    }
    UI.Line(LedgerX,LedgerY+321,LedgerX+LedgerW,LedgerY+321,FLinearColor(.56f,.55f,.49f,.30f));
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Own losses"),TEXT("己方损失")),LedgerX,LedgerY+337,194,30,18,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Infantry"),TEXT("步兵")),LedgerX+220,LedgerY+337,96,30,17,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(Record.InfantryLost),LedgerX+324,LedgerY+337,105,30,24,Paper,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("Armour"),TEXT("装甲")),LedgerX+465,LedgerY+337,96,30,17,Muted);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(Record.ArmourLost),LedgerX+581,LedgerY+337,146,30,24,Paper,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::Text,FString::FromInt(Record.BattlesRecorded)+L(TEXT(" completed engagements · hover a force for details"),TEXT(" 场战斗 · 悬停查看各部战果")),LedgerX,LedgerY+382,LedgerW,25,15,Muted);
    if(Record.bIncludesEarlierUnrecordedBattles)
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,L(TEXT("Earlier engagements have no casualty return."),TEXT("早期战斗的详细战果未有记录。")),LedgerX,LedgerY+416,LedgerW,24,15,Muted);
    if(Detail>=0)
    {
        TArray<TPair<FString,FFrontlineKillTallyV65>> Rows;
        if(Detail==0)for(int32 I=0;I<UE_ARRAY_COUNT(Record.InfantrySources);++I)
            if(Record.InfantrySources[I].Total())Rows.Add({B->GetRecruitName(EFrontlineRole(I)),Record.InfantrySources[I]});
        if(Detail==1)for(int32 I=0;I<UE_ARRAY_COUNT(Record.ArmourSources);++I)
            if(Record.ArmourSources[I].Total())Rows.Add({UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle(I)).Name,Record.ArmourSources[I]});
        if(Detail==2)for(int32 I=0;I<UE_ARRAY_COUNT(Record.SupportSources);++I)
            if(Record.SupportSources[I].Total())Rows.Add({B->GetSupportName(EFrontlineSupport(I)),Record.SupportSources[I]});
        const float H=93+Rows.Num()*36,X=1080,Y=FMath::Max(102.f,LedgerY-H-18);
        UI.PaperSheet(X,Y,697,H);const FLinearColor Ink(.03f,.034f,.03f);
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,Names[Detail],X+22,Y+16,653,31,21,Ink);
        if(Rows.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Text,L(TEXT("No confirmed kills."),TEXT("尚无确认战果。")),X+22,Y+54,650,27,18,Ink);
        for(int32 I=0;I<Rows.Num();++I)
        {
            UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Rows[I].Key,X+22,Y+58+I*36,393,29,20,Ink);
            UI.RoleLabel(EFrontlineTextRoleV8210::Text,Chinese?FString::Printf(TEXT("步兵 %d · 装甲 %d"),Rows[I].Value.Infantry,Rows[I].Value.Armour):FString::Printf(TEXT("%d infantry · %d armour"),Rows[I].Value.Infantry,Rows[I].Value.Armour),X+430,Y+58+I*36,245,29,17,Ink,2);
        }
    }
    UI.Line(143,826,1777,826,FLinearColor(.56f,.55f,.49f,.40f));
    const TCHAR* Labels[]={TEXT("Operations"),TEXT("Victories"),TEXT("Losses"),TEXT("Doctrine")};
    const int32 Values[]={FrontlineCampaignV52::ActualVictories(*R),R->Research.Victories,R->Research.Defeats,FrontlineCampaignV32::BitCount(R->Research.TechnologyMask)};
    for(int32 I=0;I<4;++I)
    {
        const float X=144+I*301.f;
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,I==0?L(TEXT("OPERATIONS"),TEXT("行动")):Copy(Labels[I]),X,850,279,27,15,Muted);
        UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Roll(Values[I]),X,887,279,42,35,Paper);
    }
    UI.RoleLabel(EFrontlineTextRoleV8210::Label,L(TEXT("DIFFICULTY"),TEXT("难度")),1404,850,372,27,16,Muted,2);
    UI.RoleLabel(EFrontlineTextRoleV8210::FieldValue,FrontlineDifficultyV30::Label(R->Difficulty),1404,887,372,42,21,Paper,2);
    UI.Button(1355,970,422,58,L(TEXT("DETAILED CAMPAIGN REPORT"),TEXT("全战役详细战报")),TEXT(""),TEXT("CampaignDetailedReportV86"),true,false,80);
    UI.Button(143,970,438,58,Copy(TEXT("WarRoom")),TEXT("Enter"),TEXT("V51EndingOperations"),true,true,80);
    UI.Button(605,970,365,58,Copy(TEXT("Archives")),TEXT(""),TEXT("V51EndingArchives"),true,false,80);
    UI.Button(994,970,337,58,Copy(TEXT("MainMenu")),TEXT("Esc"),TEXT("V51EndingMenu"),true,false,80);
}
