// Uses the production paper, buttons, text roles and numeral animation unchanged.
namespace ReportUIV839
{
const FLinearColor Ink(.028f,.032f,.029f),Faded(.07f,.075f,.064f),Rule(.20f,.20f,.14f,.22f);
constexpr float RowTop=391.f,RowHeight=49.f;
constexpr int32 PageRows=8;
FString Number(const FFrontlineCanvas& UI,int32 Value)
{return FString::FromInt(FrontlineUIMotionV83::RollNumber(Value,UI.MotionNumberAgeV83,UI.MotionStableV83));}
void Value(const FFrontlineCanvas& UI,const FString& Text,float X,float Y,float W=120)
{bool Numeric=true;for(TCHAR C:Text)if(!FChar::IsDigit(C)&&C!=TEXT('.')&&C!=TEXT('/')&&!FChar::IsWhitespace(C)){Numeric=false;break;}UI.RoleLabel(Numeric?EFrontlineTextRoleV8210::Numeral:EFrontlineTextRoleV8210::Label,Text,X,Y,W,34,Numeric?25:18,Ink,1);}
void Label(const FFrontlineCanvas& UI,const FString& Text,float X,float Y,float W=490,float Size=19)
{UI.RoleLabel(EFrontlineTextRoleV8210::Label,Text,X,Y,W,36,Size,Ink);}
FString KnownNumber(const FFrontlineCanvas& UI,int32 Count,bool Recorded)
{return Recorded?Number(UI,Count):UI.DisplayText(TEXT("未记录"));}
FString L86(const TCHAR* English,const TCHAR* Chinese){return FrontlineArmourV63::L(English,Chinese);}
FString ReportRoleNameV87(AWarfareFrontlineGameMode* B,int32 Role,int32 Team,bool Campaign)
{
 if(!B||Role<0||Role>=11)return {};
 if(!Campaign||Team!=1||!WarfareFRRUCampaignV82::Active(B))return B->GetRecruitName(EFrontlineRole(Role),Team);
 // Russia faced different armies across its route. Aggregate role totals do
 // not claim that every earlier section carried the last enemy's weapon.
 static const TCHAR* EN[]={TEXT("Rifle sections"),TEXT("Machine-gun sections"),TEXT("Grenadiers"),TEXT("Marksmen"),TEXT("Command sections"),TEXT("Flamethrower sections"),TEXT("Engineer sections"),TEXT("Rifle-grenade sections"),TEXT("Anti-tank rifle sections"),TEXT("Mobile gun sections"),TEXT("Signals sections")};
 static const TCHAR* ZH[]={TEXT("步枪小队"),TEXT("机枪小队"),TEXT("掷弹兵"),TEXT("精确射手"),TEXT("指挥小队"),TEXT("喷火兵"),TEXT("工兵小队"),TEXT("枪榴弹小队"),TEXT("反坦克步枪组"),TEXT("机动枪组"),TEXT("通信组")};
 return L86(EN[Role],ZH[Role]);
}
struct FRow
{
    FString Name;
    FString Values[4];
    bool Heading=false;
};
void Rows(const FFrontlineCanvas& UI,FrontlineReportV839::FView& V,const TArray<FRow>& Rows,const TCHAR* C0,const TCHAR* C1,const TCHAR* C2,const TCHAR* C3,int32 TotalRows=INDEX_NONE)
{
    constexpr float X[]={840,1029,1218,1407};
    const TCHAR* Captions[]={C0,C1,C2,C3};
    for(int32 I=0;I<4;++I)UI.RoleLabel(EFrontlineTextRoleV8210::Label,Captions[I],X[I],348,170,34,17,Faded,1);
    UI.Line(285,382,1635,382,Rule);
    const int32 Count=TotalRows==INDEX_NONE?Rows.Num():TotalRows;
    V.Offset=FMath::Clamp(V.Offset,0,FMath::Max(0,Count-PageRows));
    if(Count==0)UI.RoleParagraph(EFrontlineTextRoleV8210::Text,V.CampaignV86?L86(TEXT("No entries in the campaign return."),TEXT("战役总录暂无此项记录。")):FString(TEXT("本次行动尚无此项记录")),305,421,1230,74,23,Faded);
    const int32 End=FMath::Min(Count,V.Offset+PageRows);
    for(int32 I=V.Offset;I<End;++I)
    {
        const auto& Row=Rows[TotalRows==INDEX_NONE?I:I-V.Offset];const float Y=RowTop+(I-V.Offset)*RowHeight;
        Label(UI,Row.Name,302,Y,505,Row.Heading?20:18);
        for(int32 C=0;C<4;++C)Value(UI,Row.Values[C],X[C],Y,170);
        UI.Line(285,Y+40,1635,Y+40,Rule);
    }
    if(Count>0)
    {
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,FString::Printf(TEXT("第 %d–%d 条，共 %d 条"),V.Offset+1,End,Count),305,799,680,33,17,Faded);
        if(Count>PageRows){UI.PageButtonV73(1172,792,212,49,TEXT("上页"),TEXT(""),TEXT("ReportUpV839"),V.Offset>0,false,60);
        UI.PageButtonV73(1402,792,212,49,TEXT("下页"),TEXT(""),TEXT("ReportDownV839"),End<Count,false,60);}
    }
}
void Summary(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineReportDataV839& Data,int32 Team)
{
    const auto& R=Data.Armies[Team];const auto& D=R.DetailsV839;
    const bool MissingSide=Data.CampaignV86&&Data.bEarlierAttemptsUnrecordedV86&&R.BattlesRecorded==0;
    const TCHAR* Names[]={TEXT("敌军阵亡"),TEXT("装甲击毁"),TEXT("步兵阵亡"),TEXT("装甲损失"),TEXT("友军误伤阵亡"),TEXT("友军误伤装甲"),TEXT("已出动兵员"),TEXT("其他击杀")};
    int32 Deployed=0;for(int32 N:D.DeployedByRole)Deployed+=N;
    const int32 Counts[]={R.Total.Infantry,R.Total.Armour,R.InfantryLost,R.ArmourLost,D.FriendlyFire.Infantry,D.FriendlyFire.Armour,Deployed,R.OtherSources.Total()};
    for(int32 I=0;I<8;++I)
    {
        const float Y=390+I*46;Label(UI,Names[I],305,Y,501,21);
        Value(UI,KnownNumber(UI,Counts[I],!MissingSide&&(I<4||I==7||D.bRecorded)),801,Y,222);UI.Line(285,Y+38,1049,Y+38,Rule);
    }
    int32 BestRole=INDEX_NONE,BestKills=0;
    for(int32 I=0;I<11;++I)if(R.InfantrySources[I].Total()>BestKills){BestRole=I;BestKills=R.InfantrySources[I].Total();}
    Label(UI,TEXT("累计击杀最多的兵种"),1101,391,508,18);
    UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,BestRole==INDEX_NONE?FString(MissingSide?TEXT("未记录"):TEXT("没有已记录的击杀")):ReportRoleNameV87(B,BestRole,Team,Data.CampaignV86),1101,433,508,83,24,Ink);
    if(BestRole!=INDEX_NONE)Value(UI,Number(UI,BestKills),1110,501,202);
    const FFrontlineSquadReportV839* Best=nullptr;
    if(Data.CampaignV86)
    {const auto& Sections=Data.SortedSquadsV86[Team];if(!Sections.IsEmpty()&&Sections[0].Kills.Total()>0)Best=&Sections[0];}
    else for(const auto& E:D.Squads)if(E.Value.Kills.Total()>0&&(!Best||E.Value.Kills.Total()>Best->Kills.Total()||(E.Value.Kills.Total()==Best->Kills.Total()&&E.Value.Id<Best->Id)))Best=&E.Value;
    Label(UI,TEXT("累计击杀最多的小队"),1101,565,508,18);
    FString Section=UI.DisplayText(D.bRecorded?TEXT("没有已记录的击杀"):TEXT("未记录"));
    if(Best&&Best->OriginalRole>=0&&Best->OriginalRole<11)
    {
        Section=UI.DisplayText(FString::Printf(TEXT("%s · 小队 %d"),*ReportRoleNameV87(B,Best->OriginalRole,Team,Data.CampaignV86),Best->Id));
    }
    if(Best&&Data.CampaignV86&&Best->OriginalRole>=0&&Best->OriginalRole<11)
    {
        const FString Identity=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?FString::Printf(TEXT("第 %d 场 · 小队 %d"),Best->BattleNumberV86,Best->Id):FString::Printf(TEXT("Engagement %d · Section %d"),Best->BattleNumberV86,Best->Id);
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,Identity,1101,607,508,33,20,Ink);
        UI.RoleLabel(EFrontlineTextRoleV8210::Objective,ReportRoleNameV87(B,Best->OriginalRole,Team,Data.CampaignV86),1101,644,508,33,22,Ink);
        Value(UI,Number(UI,Best->Kills.Total()),1110,681,202);
    }
    else
    {
        UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,Section,1101,607,508,81,24,Ink);
        if(Best)Value(UI,Number(UI,Best->Kills.Total()),1110,677,202);
    }
    if(Data.CampaignV86)
    {
        const int32 Victories=Team==0?Data.VictoriesV86:Data.DefeatsV86,Defeats=Team==0?Data.DefeatsV86:Data.VictoriesV86;
        UI.RoleLabel(EFrontlineTextRoleV8210::Label,(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?FString::Printf(TEXT("%d 场战斗 · %d 场胜利 · %d 场失败"),Data.BattlesV86,Victories,Defeats):FString::Printf(TEXT("%d engagements · %d victories · %d defeats"),Data.BattlesV86,Victories,Defeats)),1101,721,508,34,17,Faded);
    }
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,TEXT("击杀按致命事件的来源记录；友军误伤另列。击毁装甲只计车体毁灭，履带损伤不计击毁。"),305,771,1260,76,18,Faded);
}
void Infantry(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineCombatRecordV65& R,FrontlineReportV839::FView& V)
{
    if(V.CampaignV86&&V.Team==1&&!R.DetailsV839.bRecorded&&R.DetailsV839.bIncludesEarlierUnrecordedBattles)
    {UI.RoleParagraph(EFrontlineTextRoleV8210::Text,L86(TEXT("Enemy deployment and losses were not recorded for these earlier engagements."),TEXT("早期战斗未记录敌军出动与损失。")),305,409,1280,107,24,Faded);return;}
    TArray<FRow> Rows;
    for(int32 I=0;I<11;++I)
    {
        const auto& K=R.InfantrySources[I];const auto& D=R.DetailsV839;
        if(K.Total()==0&&D.DeployedByRole[I]==0&&D.LostByRole[I]==0)continue;
        FRow Row;Row.Name=ReportRoleNameV87(B,I,V.Team,V.CampaignV86);Row.Values[0]=KnownNumber(UI,D.DeployedByRole[I],D.bRecorded);Row.Values[1]=KnownNumber(UI,D.LostByRole[I],D.bRecorded);Row.Values[2]=Number(UI,K.Infantry);Row.Values[3]=Number(UI,K.Armour);Rows.Add(Row);
    }
    ReportUIV839::Rows(UI,V,Rows,TEXT("出动"),TEXT("损失"),TEXT("步兵击杀"),TEXT("装甲击毁"));
}
void Sections(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineReportDataV839& Data,FrontlineReportV839::FView& V)
{
    const auto& R=Data.Armies[V.Team];
    if(!R.DetailsV839.bRecorded){UI.RoleParagraph(EFrontlineTextRoleV8210::Text,TEXT("较早的行动未记录这些项目；现存击杀和损失仍予保留。"),305,409,1280,107,24,Faded);return;}
    TArray<FFrontlineSquadReportV839> BattleSections;
    if(!Data.CampaignV86)
    {R.DetailsV839.Squads.GenerateValueArray(BattleSections);BattleSections.Sort([](const auto& A,const auto& B){return A.Kills.Total()!=B.Kills.Total()?A.Kills.Total()>B.Kills.Total():A.Id<B.Id;});}
    const auto& Sections=Data.CampaignV86?Data.SortedSquadsV86[V.Team]:BattleSections;
    if(Data.CampaignV86)V.Offset=FMath::Clamp(V.Offset,0,FMath::Max(0,Sections.Num()-PageRows));
    const int32 Start=Data.CampaignV86?V.Offset:0,End=Data.CampaignV86?FMath::Min(Sections.Num(),Start+PageRows):Sections.Num();
    TArray<FRow> Rows;
    for(int32 I=Start;I<End;++I)
    {
        const auto& S=Sections[I];
        if(S.OriginalRole<0||S.OriginalRole>=11)continue;
        FRow Row;Row.Name=UI.DisplayText(FString::Printf(TEXT("%s · 小队 %d"),*ReportRoleNameV87(B,S.OriginalRole,V.Team,V.CampaignV86),S.Id));
        if(V.CampaignV86)Row.Name=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?FString::Printf(TEXT("第 %d 场 · %s"),S.BattleNumberV86,*Row.Name):FString::Printf(TEXT("%d · %s"),S.BattleNumberV86,*Row.Name);
        Row.Values[0]=S.Deployed>0?Number(UI,S.Deployed):UI.DisplayText(TEXT("未记录"));Row.Values[1]=Number(UI,S.Lost);Row.Values[2]=Number(UI,S.Kills.Infantry);Row.Values[3]=Number(UI,S.Kills.Armour);Rows.Add(Row);
    }
    ReportUIV839::Rows(UI,V,Rows,TEXT("出动"),TEXT("损失"),TEXT("步兵击杀"),TEXT("装甲击毁"),Data.CampaignV86?Sections.Num():INDEX_NONE);
}
void Armour(const FFrontlineCanvas& UI,const FFrontlineCombatRecordV65& R,FrontlineReportV839::FView& V)
{
    if(V.CampaignV86&&V.Team==1&&!R.DetailsV839.bRecorded&&R.DetailsV839.bIncludesEarlierUnrecordedBattles)
    {UI.RoleParagraph(EFrontlineTextRoleV8210::Text,L86(TEXT("Enemy armour records were not preserved for these earlier engagements."),TEXT("早期战斗未留存敌军装甲记录。")),305,409,1280,107,24,Faded);return;}
    const auto& D=R.DetailsV839;TArray<FRow> Rows;
    auto Add=[&](const FString& Name,const FFrontlineKillTallyV65& K,int32 Lost,int32 Destroyed)
    {
        if(K.Total()==0&&Lost==0&&Destroyed==0)return;
        FRow Row;Row.Name=Name;Row.Values[0]=Number(UI,K.Infantry);Row.Values[1]=Number(UI,K.Armour);Row.Values[2]=KnownNumber(UI,Lost,D.bRecorded);Row.Values[3]=KnownNumber(UI,Destroyed,D.bRecorded);Rows.Add(Row);
    };
    for(int32 I=0;I<13;++I)Add(UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle(I)).Name,R.ArmourSources[I],D.ArmourLostByModel[I],D.ArmourDestroyedByModel[I]);
    TSet<FString> Ids;for(const auto& E:R.NationalArmourSourcesV82)Ids.Add(E.Key);for(const auto& E:D.NationalArmourLost)Ids.Add(E.Key);for(const auto& E:D.NationalArmourDestroyed)Ids.Add(E.Key);
    TArray<FString> Sorted=Ids.Array();Sorted.Sort();
    for(const auto& Id:Sorted)if(const auto* Model=WarfareFRRUArmourV82::Definition(Id))Add(Model->Name,R.NationalArmourSourcesV82.FindRef(Id),D.NationalArmourLost.FindRef(Id),D.NationalArmourDestroyed.FindRef(Id));
    if(V.CampaignV86)ReportUIV839::Rows(UI,V,Rows,*L86(TEXT("Infantry kills"),TEXT("步兵击杀")),*L86(TEXT("Armour kills"),TEXT("装甲击毁")),*L86(TEXT("Lost"),TEXT("我方损失")),*L86(TEXT("Destroyed"),TEXT("敌方击毁")));
    else ReportUIV839::Rows(UI,V,Rows,TEXT("该型步兵击杀"),TEXT("该型装甲击毁"),TEXT("该型损失"),TEXT("敌军该型击毁"));
}
void Support(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineCombatRecordV65& R,FrontlineReportV839::FView& V)
{
    const auto& D=R.DetailsV839;TArray<int32> Kinds;
    for(int32 I=0;I<10;++I){const auto& S=D.Support[I];if(S.Delivered||S.Rounds||R.SupportSources[I].Total()||D.FriendlySupport[I].Total()||S.EnemyHullDamage||S.EnemyTrackDamage||S.FriendlyHullDamage||S.FriendlyTrackDamage)Kinds.Add(I);}
    constexpr int32 PerPage=3;V.Offset=FMath::Clamp(V.Offset,0,FMath::Max(0,Kinds.Num()-PerPage));
    if(Kinds.IsEmpty())UI.RoleParagraph(EFrontlineTextRoleV8210::Text,D.bRecorded?TEXT("本次行动尚无此项记录"):TEXT("较早的行动未记录这些项目；现存击杀和损失仍予保留。"),305,409,1280,107,24,Faded);
    const int32 End=FMath::Min(Kinds.Num(),V.Offset+PerPage);
    const TCHAR* Captions[]={TEXT("支援命令"),TEXT("实发弹药／释放"),TEXT("步兵击杀"),TEXT("装甲击毁数"),TEXT("友军步兵损失"),TEXT("友军装甲损失")};
    constexpr float X[]={440,646,852,1058,1264,1470};
    for(int32 I=V.Offset;I<End;++I)
    {
        const int32 Kind=Kinds[I];const auto& S=D.Support[Kind];const auto& K=R.SupportSources[Kind];const auto& F=D.FriendlySupport[Kind];const float Y=389+(I-V.Offset)*140;
        Label(UI,B->GetSupportName(EFrontlineSupport(Kind),V.Team),305,Y,1210,22);
        const bool Lethal=Kind!=int32(EFrontlineSupport::Smoke)&&Kind!=int32(EFrontlineSupport::Rally);
        const FString Values[]={KnownNumber(UI,S.Delivered,S.bDeliveryRecorded),KnownNumber(UI,S.Rounds,S.bRoundsRecorded),Lethal?Number(UI,K.Infantry):FString(TEXT("—")),Lethal?Number(UI,K.Armour):FString(TEXT("—")),Lethal?KnownNumber(UI,F.Infantry,D.bRecorded):FString(TEXT("—")),Lethal?KnownNumber(UI,F.Armour,D.bRecorded):FString(TEXT("—"))};
        for(int32 C=0;C<6;++C){if(C>=2&&!Lethal)continue;if(C==1&&Kind==int32(EFrontlineSupport::Rally))continue;UI.RoleLabel(EFrontlineTextRoleV8210::Label,Captions[C],X[C]-35,Y+38,185,25,15,Faded,1);Value(UI,Values[C],X[C]-35,Y+65,185);}
        if(S.bDamageRecorded&&(S.EnemyHullDamage||S.EnemyTrackDamage||S.FriendlyHullDamage||S.FriendlyTrackDamage))
        {
            Label(UI,TEXT("敌车体／履带损伤"),305,Y+101,300,15);Value(UI,FString::Printf(TEXT("%.1f / %.1f"),S.EnemyHullDamage,S.EnemyTrackDamage),619,Y+99,297);
            Label(UI,TEXT("友车体／履带损伤"),955,Y+101,300,15);Value(UI,FString::Printf(TEXT("%.1f / %.1f"),S.FriendlyHullDamage,S.FriendlyTrackDamage),1251,Y+99,363);
        }
        UI.Line(285,Y+137,1635,Y+137,Rule);
    }
    UI.RoleParagraph(EFrontlineTextRoleV8210::Text,TEXT("车体与履带栏列出命中造成的耐久损失。友军损伤另列。"),305,817,1278,35,17,Faded);
    if(Kinds.Num()>PerPage)
    {
        UI.PageButtonV73(1172,857,212,49,TEXT("上页"),TEXT(""),TEXT("ReportUpV839"),V.Offset>0,false,60);
        UI.PageButtonV73(1402,857,212,49,TEXT("下页"),TEXT(""),TEXT("ReportDownV839"),End<Kinds.Num(),false,60);
    }
}
int32 ReturnOrdinalV86(int32 KnownBattles,int32 RetainedReturns,int32 Index)
{return FMath::Max(0,KnownBattles-RetainedReturns)+Index+1;}
void EngagementsV86(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineReportDataV839& Data,FrontlineReportV839::FView& V)
{
    const auto* Campaign=B->GetCurrentCampaign();
    const auto* Record=WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::CampaignCombatRecordV87(B):Campaign?&Campaign->CombatRecordV65:nullptr;
    if(!Record)return;
    TArray<FRow> Rows;const auto& Missions=B->GetCampaignMissions();
    for(int32 I=0;I<Record->BattlesV86.Num();++I)
    {
        const auto& Return=Record->BattlesV86[I];if(!Return.Armies[V.Team].IsValid())continue;
        const bool Live=Return.Encounter==B->GetEncounterId()&&(B->Phase==EFrontlinePhase::Victory||B->Phase==EFrontlinePhase::Defeat);
        const auto& Side=Live?B->CombatRecordV65[V.Team]:*Return.Armies[V.Team];FRow Row;
        const FString Title=Missions.IsValidIndex(Return.Mission)?UI.DisplayText(Missions[Return.Mission].Title):FString::FromInt(Return.Mission+1);
        Row.Name=FString::Printf(TEXT("%d · %s"),ReturnOrdinalV86(Data.BattlesV86,Record->BattlesV86.Num(),I),*Title);
        Row.Values[0]=Return.Winner==V.Team?L86(TEXT("Victory"),TEXT("胜利")):L86(TEXT("Defeat"),TEXT("失败"));
        Row.Values[1]=Number(UI,Side.Total.Infantry);Row.Values[2]=Number(UI,Side.Total.Armour);Row.Values[3]=FString::Printf(TEXT("%d / %d"),Side.InfantryLost,Side.ArmourLost);Rows.Add(Row);
    }
    ReportUIV839::Rows(UI,V,Rows,*L86(TEXT("Outcome"),TEXT("胜负")),TEXT("步兵击杀"),TEXT("装甲击毁"),*L86(TEXT("Infantry / armour lost"),TEXT("步兵／装甲损失")));
}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    auto* PC=UI.HUD.GetOwningPlayerController();auto& V=FrontlineReportV839::View(PC);
    FFrontlineReportDataV839 BattleData;if(!V.CampaignV86)BattleData=FrontlineReportV839::GetReportData(B);
    const auto& Data=V.CampaignV86?FrontlineReportV839::GetCampaignReportV86(B):BattleData;const auto& R=Data.Armies[V.Team];
    UI.Rect(0,0,1920,1080,FLinearColor(.004f,.009f,.013f,.84f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);UI.PaperSheet(215,100,1490,900);
    UI.RoleLabel(EFrontlineTextRoleV8210::Headline,V.CampaignV86?L86(TEXT("CAMPAIGN AFTER-ACTION REPORT"),TEXT("全战役详细战报")):FString(TEXT("详细战报")),282,144,1094,57,36,Ink);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,Data.CampaignV86&&Data.bEarlierAttemptsUnrecordedV86?FString(TEXT("—")):ClockText(Data.Seconds),1417,151,219,43,29,Faded,2);
    const auto* Mission=B->GetEncounterCampaignMission();
    if(V.CampaignV86)
    {
        const auto Nation=WarfareFRRUCampaignV82::Nation(B,0);
        const FString Army=Nation==EFRRUNationV81::French?L86(TEXT("French Army · 1916–1918"),TEXT("法国陆军 · 1916—1918")):
            Nation==EFRRUNationV81::Russian?L86(TEXT("Imperial Russian Army · 1914–1917"),TEXT("俄罗斯帝国陆军 · 1914—1917")):
            Nation==EFRRUNationV81::German?L86(TEXT("Deutsches Heer · 1914–1918"),TEXT("德意志陆军 · 1914—1918")):
            L86(TEXT("British Expeditionary Force · 1914–1918"),TEXT("英国远征军 · 1914—1918"));
        UI.RoleLabel(EFrontlineTextRoleV8210::Objective,Army,285,213,927,40,22,Faded);
    }
    else if(Mission)UI.RoleLabel(EFrontlineTextRoleV8210::Objective,Mission->Title,285,213,927,40,22,Faded);
    UI.PageButtonV73(1238,205,183,49,TEXT("我军"),TEXT(""),TEXT("ReportOurArmyV839"),true,V.Team==0,60);
    UI.PageButtonV73(1440,205,183,49,TEXT("敌军"),TEXT(""),TEXT("ReportEnemyArmyV839"),true,V.Team==1,60);
    const TCHAR* Tabs[]={TEXT("战斗总览"),TEXT("步兵兵种"),TEXT("小队记录"),TEXT("装甲记录"),TEXT("支援记录")};
    const int32 TabCount=V.CampaignV86?6:5;const float TabStep=1350.f/TabCount;
    for(int32 I=0;I<TabCount;++I)UI.PageButtonV73(285+I*TabStep,279,TabStep-17,51,I==5?L86(TEXT("Engagements"),TEXT("逐场记录")):FString(Tabs[I]),TEXT(""),FName(*FString::Printf(TEXT("ReportTabV839_%d"),I)),true,V.Tab==I,60);
    if(V.Tab==0)Summary(UI,B,Data,V.Team);else if(V.Tab==1)Infantry(UI,B,R,V);else if(V.Tab==2)Sections(UI,B,Data,V);else if(V.Tab==3)Armour(UI,R,V);else if(V.Tab==5&&V.CampaignV86)EngagementsV86(UI,B,Data,V);else Support(UI,B,R,V);
    if(!V.CampaignV86&&V.Tab!=0&&V.Tab!=4)UI.RoleParagraph(EFrontlineTextRoleV8210::Text,TEXT("记录包括战斗结束后落下的已部署炮火。"),305,856,1267,61,17,Faded);
    if(V.CampaignV86&&V.Tab!=4)UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Data.bEarlierAttemptsUnrecordedV86?(V.Team==1?L86(TEXT("Enemy figures cover only retained battle returns. Earlier deployment and losses were not recorded."),TEXT("敌军数据仅列已留存战报；早期出动与损失未记录。")):L86(TEXT("Earlier engagements retain their recorded totals; their individual returns were not preserved."),TEXT("早期战斗保留已有累计战果，逐场记录尚未留存。"))):L86(TEXT("The return includes victories, defeats and retried engagements. Each engagement is counted once."),TEXT("总录包含胜利、失败与重试战斗，每场战斗只计一次。")),305,856,1267,48,18,Faded);
    UI.PageButtonV73(285,921,519,56,V.CampaignV86?L86(TEXT("Return to the campaign ending"),TEXT("返回战役结语")):FString(TEXT("返回战报")),TEXT("Esc"),TEXT("ReportBackV839"),true,true,60);
    if(Data.CampaignV86&&Data.bEarlierAttemptsUnrecordedV86)UI.RoleLabel(EFrontlineTextRoleV8210::Label,R.BattlesRecorded>0?L86(TEXT("Retained records only"),TEXT("仅现存记录")):L86(TEXT("Unrecorded"),TEXT("未记录")),1219,927,398,34,18,Faded,2);
    else if(!R.DetailsV839.bRecorded||R.DetailsV839.bIncludesEarlierUnrecordedBattles)UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("未记录"),1219,927,398,34,18,Faded,2);
}
}
