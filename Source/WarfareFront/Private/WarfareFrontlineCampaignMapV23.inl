// Original ink cartography with geographic town positions. Front lines and arrows
// indicate a sector and intent, not a battalion-level historical survey.
void DrawCampaignCartography(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,float X,float Y,float W,float H)
{
    TGuardValue<bool> MapTypographyScopeV8210(UI.bLegacyTypographyV8210,true);
    if(DrawPaintedMapV30(UI,B,X,Y,W,H))return;
    const auto* M=B->GetSelectedCampaignMission();if(!M)return;
    const FLinearColor Ink(.08f,.10f,.083f,.75f),Faint(.13f,.16f,.12f,.14f),River(.14f,.25f,.28f,.43f);
    const FLinearColor British(.13f,.22f,.22f,.83f),German(.33f,.12f,.08f,.80f);
    auto P=[&](double Lon,double Lat){return FVector2D(X+(Lon-1.55)/2.65*W,Y+(51.32-Lat)/1.67*H);};
    auto Stroke=[&](std::initializer_list<FVector2D> Points,FLinearColor Color,float Thickness)
    {
        bool First=true;FVector2D A;
        for(const FVector2D& G:Points){const FVector2D Z=P(G.X,G.Y);if(!First)UI.Line(A.X,A.Y,Z.X,Z.Y,Color,Thickness);A=Z;First=false;}
    };
    for(int32 I=0;I<10;++I)UI.Line(X+I*W/9,Y,X+I*W/9,Y+H,Faint,.7f);
    for(int32 I=0;I<6;++I)UI.Line(X,Y+I*H/5,X+W,Y+I*H/5,Faint,.7f);
    Stroke({{1.57,50.69},{1.61,50.81},{1.72,50.90},{1.87,50.95},{2.18,51.01},{2.38,51.05},{2.58,51.13},{2.76,51.19},{2.92,51.23},{3.20,51.30}},River,2.5f);
    Stroke({{2.20,49.91},{2.30,49.90},{2.49,49.93},{2.70,49.89},{2.86,49.93},{2.94,49.86},{3.08,49.79},{3.28,49.85}},River,2.f);
    Stroke({{3.22,50.14},{3.25,50.28},{3.45,50.44},{3.41,50.61},{3.58,50.75},{3.73,51.04}},River,1.5f);
    // A restrained hachured ridge behind the Ypres salient, rather than decorative contour noise.
    for(int32 I=0;I<17;++I)
    {
        const double Lat=50.71+I*.013,Lon=2.96+.065*FMath::Sin(I*.19);
        const FVector2D A=P(Lon,Lat),Z=P(Lon+.04,Lat-.012);UI.Line(A.X,A.Y,Z.X,Z.Y,Faint*2.f,1);
    }
    const bool Spring=FString(M->MapKey)==TEXT("michael"),Amiens=FString(M->MapKey)==TEXT("amiens");
    const double Bulge=Spring?-.18:Amiens?-.26:0;
    Stroke({{2.77,51.08},{2.88,50.99},{2.93,50.92},{2.92,50.86},{2.83,50.79},{2.89,50.70},{2.82,50.60},{2.87,50.43},{2.89,50.30},{3.06,50.22},{3.15,50.12},{3.20+Bulge,50.01},{3.12+Bulge,49.88},{3.20+Bulge,49.72}},German,2.5f);
    struct Town{const TCHAR* Name;double Lon,Lat;float DX,DY;};
    const Town Towns[]={{TEXT("CALAIS"),1.859,50.951,9,0},{TEXT("YPRES"),2.886,50.850,-77,-17},{TEXT("MESSINES"),2.895,50.765,-104,3},
        {TEXT("LILLE"),3.058,50.630,9,-13},{TEXT("ARRAS"),2.777,50.292,-75,-16},{TEXT("CAMBRAI"),3.234,50.176,11,-15},
        {TEXT("AMIENS"),2.296,49.895,-72,7},{TEXT("ST QUENTIN"),3.287,49.848,11,8},{TEXT("PÉRONNE"),2.936,49.933,7,-17}};
    for(const auto& T:Towns){const FVector2D A=P(T.Lon,T.Lat);UI.Rect(A.X-2,A.Y-2,4,4,Ink);UI.Text(T.Name,A.X+T.DX,A.Y+T.DY,13,Ink);}
    auto Tag=[&](const TCHAR* Name,double Lon,double Lat,float Size,FLinearColor Color){const FVector2D A=P(Lon,Lat);UI.Text(Name,A.X,A.Y,Size,Color);};
    Tag(TEXT("FRANCE"),1.83,50.30,31,Faint*2.7f);Tag(TEXT("BELGIQUE"),3.45,51.11,22,Faint*2.7f);
    Tag(TEXT("SOMME"),2.45,49.77,13,River);Tag(TEXT("NORTH SEA"),1.60,51.17,14,River);
    const FVector2D C=P(M->MapPosition.X,M->MapPosition.Y);
    const FLinearColor Side=B->GetPlayerFaction()==EFrontlineFaction::German?German:British;
    for(int32 I=0;I<48;++I)
    {
        const float A=I*2*PI/48,Z=(I+1)*2*PI/48;
        UI.Line(C.X+FMath::Cos(A)*27,C.Y+FMath::Sin(A)*27,C.X+FMath::Cos(Z)*27,C.Y+FMath::Sin(Z)*27,Side,2);
    }
    UI.Rect(C.X-4,C.Y-4,8,8,Side);
    const float Direction=B->GetPlayerFaction()==EFrontlineFaction::German?-1:1;
    const FVector2D From(C.X-Direction*94,C.Y+27),End(C.X-Direction*34,C.Y+7);
    UI.Line(From.X,From.Y,End.X,End.Y,Side,4);
    UI.Line(End.X,End.Y,End.X-Direction*15,End.Y-4,Side,3);UI.Line(End.X,End.Y,End.X-Direction*10,End.Y+13,Side,3);
    UI.Text(TEXT("N"),X+W-28,Y+12,16,Ink);UI.Line(X+W-23,Y+61,X+W-23,Y+35,Ink,1.5f);
    UI.Line(X+W-23,Y+35,X+W-28,Y+44,Ink);UI.Line(X+W-23,Y+35,X+W-18,Y+44,Ink);
    UI.Line(X+20,Y+H-21,X+115,Y+H-21,Ink,2);UI.Line(X+20,Y+H-26,X+20,Y+H-17,Ink);UI.Line(X+115,Y+H-26,X+115,Y+H-17,Ink);
    UI.Text(TEXT("约 20 km"),X+29,Y+H-48,12,Ink);UI.Right(TEXT("战区态势示意"),X+W-12,Y+H-20,13,Ink);
}

void DrawCampaignDossierV23(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    const auto* Run=B->GetCurrentCampaign();const auto* M=B->GetSelectedCampaignMission();if(!Run||!M)return;
    const FLinearColor Ink(.028f,.032f,.029f),Faded(.08f,.09f,.077f),Red(.31f,.12f,.08f,.83f);
    UI.PaperSheet(120,103,1150,830);UI.PaperSheet(1300,145,510,788);
    UI.Text(TEXT("WESTERN FRONT"),163,130,36,Ink);UI.Right(M->DateLabel,1226,141,21,Faded);
    UI.Text(B->GetFactionName(),166,193,20,Faded);
    UI.Right(FrontlineDifficultyV30::Label(B->GetCampaignDifficulty()),1226,193,18,Ink);
    DrawCampaignCartography(UI,B,168,235,1060,427);
    DrawCommandersV30(UI,B,168,681);
    const auto& Missions=B->GetCampaignMissions();
    for(int32 I=0;I<Missions.Num();++I)
    {
        const auto& Item=Missions[I];const float X=167+(I%10)*106,Y=806+(I/10)*57;
        const bool Selected=Item.Id==Run->SelectedMission,Open=B->CampaignMissionBlockReason(Item.Id).IsEmpty();
        if(Selected)UI.Rect(X-6,Y-4,100,46,FLinearColor(.25f,.19f,.12f,.13f));
        UI.Text(FString::Printf(TEXT("%02d"),I+1),X,Y,24,Open?Ink:Faded*.55f);
        if(UI.Hovered(X-7,Y-4,101,47))
        {
            const float TX=FMath::Min(X-15,903.f),TY=Y-220;UI.PaperSheet(TX,TY,415,204);
            UI.FitText(Item.Title,TX+17,TY+15,381,19,Ink);UI.Text(Item.DateLabel,TX+17,TY+46,13,Faded);
            UI.Paragraph(Item.AlliedContext,TX+17,TY+75,381,110,15,Ink,true);
        }
        UI.Line(X,Y+39,X+90,Y+39,Selected?Red:Faded*.27f,Selected?2.f:1.f);
        UI.Hit(X-7,Y-4,101,47,FName(*FString::Printf(TEXT("CampaignMission%d"),I)),50);
        if(I<Run->CompletedMissions)UI.Text(TEXT("✓"),X+65,Y+3,19,Ink);
    }
    if(UI.Hovered(168,235,1060,427))
    {UI.Metal(168,541,1060,121,true);UI.Paragraph(M->TerrainNotes,187,558,1020,88,17,Paper);}
    UI.Label(M->Title,1335,173,438,48,29,Ink);UI.Label(M->Location,1338,230,432,32,19,Faded);
    UI.Line(1337,275,1773,275,Faded*.35f);
    const float BodySize=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?16.f:18.f;
    UI.Paragraph(M->HistoricalContext,1338,294,432,145,BodySize,Ink);
    UI.Label(TEXT("连队命令"),1338,453,432,32,20,Red);
    UI.Paragraph(M->Briefing,1338,502,432,168,BodySize,Ink);
    UI.Paragraph(M->ObjectiveText,1338,697,432,78,BodySize-1,Faded);
    UI.Button(1336,795,438,64,TEXT("开始行动"),TEXT("ENTER"),TEXT("Begin"),true,true,50);
    UI.Button(1336,871,438,47,TEXT("整备部队"),TEXT("T"),TEXT("Technology"),true,false,50);
    const auto Loadout=FrontlineCampaignV32::GetLoadout(B);if(Loadout.bSelectRoles||Loadout.bSelectSupports)UI.Button(1090,959,720,53,TEXT("查看出战序列"),TEXT(""),TEXT("V32Deployment"),true,false,50);
    UI.Button(120,959,240,53,TEXT("战役档案"),TEXT(""),TEXT("CampaignArchives"),true,false,50);
    UI.Button(380,959,240,53,TEXT("主菜单"),TEXT("Esc"),TEXT("MenuHome"),true,false,50);
    if(FrontlineCampaignV51::CampaignCompleted(B))UI.Button(640,959,420,53,FrontlineCampaignV51::Copy(TEXT("OpenRecord")),TEXT(""),TEXT("V51CampaignRecord"),true,false,50);
}
