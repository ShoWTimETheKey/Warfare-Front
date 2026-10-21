// The loading page owns the compiled bilingual narrative; its definition follows
// this sheet in the same translation unit.
namespace FrontlineLoadingPageV82
{FString Narrative(AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M);}
namespace TextReviewV829
{static FString Loading(const TCHAR* Field,const FString& Fallback);}
namespace MedalWallV8215
{void RibbonBarV8218(const FFrontlineCanvas& UI,EFRRUNationV81 Nation,float X,float Y,float W,float H,bool Centred,bool HoverDetails,bool WarRoomLinkV8444);}

// Geographic theatre navigation and illustrated operation areas are separate views.
namespace FRRUWarRoomV82
{
constexpr double MapX=48,MapY=156,MapW=1218,MapH=762;
const FLinearColor MapInk(.042f,.045f,.041f),FieldRed(.28f,.045f,.035f);
struct FView
{
    FGuid Campaign;FString Preview,SelectedMission;
    FVector2D Centre{.5,.5},From{.5,.5},To{.5,.5};
    float Zoom=1,FromZoom=1,ToZoom=1;
    double Started=0,Duration=0;
    bool Dragging=false,OperationMap=false;FVector2D Pointer;
};
struct FNavigationView
{
    FGuid Campaign;FString Preview;FVector2D Centre{.5,.5};float Zoom=1;
    bool OperationMap=false;
};
TMap<TWeakObjectPtr<AWarfareFrontlineController>,FView> Views;
FView& View(AWarfareFrontlineController* PC)
{for(auto I=Views.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();return Views.FindOrAdd(PC);}
bool Active(AWarfareFrontlineController* PC)
{
    const auto* B=FrontlineFor(PC);if(!B||!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Briefing)return false;
    const auto& S=ScreenState(PC);return S.Dossier==1917&&!S.Settings&&!S.AchievementsV76&&!S.Help&&!S.Technology&&!S.Preparing&&!S.Deployment&&!S.UnitIntroduction&&!S.HistoricalDispatch&&!S.CampaignEndingV51&&!S.Menu;
}
const FRRUArtV82::FMap* Map(const AWarfareFrontlineGameMode* B)
{const TCHAR* Id=WarfareFRRUCampaignV82::Nation(B)==EFRRUNationV81::French?TEXT("fr"):TEXT("ru");for(const auto& M:FRRUArtV82::Maps)if(FString(M.Nation)==Id)return &M;return nullptr;}
UTexture2D* Texture(const TCHAR* Path)
{static TMap<FString,TStrongObjectPtr<UTexture2D>> Loaded;if(!Path||!*Path)return nullptr;if(const auto* T=Loaded.Find(Path))return T->Get();return Loaded.Add(Path,TStrongObjectPtr<UTexture2D>(LoadObject<UTexture2D>(nullptr,Path))).Get();}
double Mercator(double Latitude){return FMath::Loge(FMath::Tan(PI*.25+FMath::DegreesToRadians(Latitude)*.5));}
FVector2D UV(const FRRUArtV82::FMap& M,const FVector2D& Geo)
{return {(Geo.X-M.West)/(M.East-M.West),(Mercator(M.North)-Mercator(Geo.Y))/(Mercator(M.North)-Mercator(M.South))};}
FVector2D Project(const FView& V,const FVector2D& PointUV)
{return {MapX+MapW*(.5+(PointUV.X-V.Centre.X)*V.Zoom),MapY+MapH*(.5+(PointUV.Y-V.Centre.Y)*V.Zoom)};}
FVector2D Unproject(const FView& V,const FVector2D& Pixel)
{return {V.Centre.X+((Pixel.X-MapX)/MapW-.5)/V.Zoom,V.Centre.Y+((Pixel.Y-MapY)/MapH-.5)/V.Zoom};}
bool Inside(const FVector2D& P,double Margin=0)
{return P.X>=MapX+Margin&&P.X<=MapX+MapW-Margin&&P.Y>=MapY+Margin&&P.Y<=MapY+MapH-Margin;}
void Clamp(FView& V)
{V.Zoom=FMath::Clamp(V.Zoom,1.f,1.75f);const double Half=.5/V.Zoom;V.Centre.X=FMath::Clamp(V.Centre.X,Half,1-Half);V.Centre.Y=FMath::Clamp(V.Centre.Y,Half,1-Half);}
const FFrontlineCampaignMission* Preview(const AWarfareFrontlineGameMode* B,const FView& V)
{for(const auto& M:B->GetCampaignMissions())if(FString(M.MapKey)==V.Preview)return &M;return B->GetSelectedCampaignMission();}
void Focus(AWarfareFrontlineController* PC,const FString& StableId);
void Synchronise(AWarfareFrontlineController* PC)
{
    auto* B=FrontlineFor(PC);if(!B||!WarfareFRRUCampaignV82::Active(B))return;auto& V=View(PC);const FGuid Id=WarfareFRRUCampaignV82::CampaignId(B);
    const FString Selected=WarfareFRRUCampaignV82::MissionId(B);
    // A victory advances the operation inside the same campaign. Follow that
    // actual selection once; ordinary map browsing must not reset each frame.
    if(V.Campaign!=Id||V.SelectedMission!=Selected)
    {
        if(V.Campaign!=Id)V=FView();
        // Match the existing British/German room: a new campaign starts wide;
        // later progression flies from the current view to its real operation.
        // Publish identity first, so Focus's own synchronisation is a no-op.
        V.Campaign=Id;V.SelectedMission=Selected;V.Preview=Selected;V.OperationMap=false;
        Focus(PC,Selected);
    }
    if(!Preview(B,V))V.Preview=WarfareFRRUCampaignV82::MissionId(B);
}
FNavigationView CaptureNavigation(AWarfareFrontlineController* PC)
{
    Synchronise(PC);const auto& V=View(PC);FNavigationView Saved;
    Saved.Campaign=V.Campaign;Saved.Preview=V.Preview;Saved.OperationMap=V.OperationMap;
    // Record the selected destination, never an animation clock or a held drag.
    Saved.Centre=V.Duration>0?V.To:V.Centre;Saved.Zoom=V.Duration>0?V.ToZoom:V.Zoom;
    return Saved;
}
bool SameNavigation(const FNavigationView& A,const FNavigationView& B)
{
    return A.Campaign==B.Campaign&&A.Preview==B.Preview&&A.OperationMap==B.OperationMap
        &&A.Centre.Equals(B.Centre,1.e-6)&&FMath::IsNearlyEqual(A.Zoom,B.Zoom,1.e-5f);
}
bool RestoreNavigation(AWarfareFrontlineController* PC,const FNavigationView& Saved,bool Animate)
{
    auto* B=FrontlineFor(PC);
    if(!B||!WarfareFRRUCampaignV82::Active(B)||Saved.Campaign!=WarfareFRRUCampaignV82::CampaignId(B)
        ||!FMath::IsFinite(Saved.Centre.X)||!FMath::IsFinite(Saved.Centre.Y)||!FMath::IsFinite(Saved.Zoom)
        ||!B->GetCampaignMissions().ContainsByPredicate([&](const FFrontlineCampaignMission& M){return FString(M.MapKey)==Saved.Preview;}))return false;
    Synchronise(PC);auto& V=View(PC);const FVector2D From=V.Centre;const float FromZoom=V.Zoom;
    V.Preview=Saved.Preview;V.OperationMap=Saved.OperationMap;V.Centre=Saved.Centre;V.Zoom=Saved.Zoom;Clamp(V);
    V.From=V.To=V.Centre;V.FromZoom=V.ToZoom=V.Zoom;V.Duration=0;V.Started=0;
    V.Dragging=false;V.Pointer=FVector2D::ZeroVector;
    if(Animate&&!V.OperationMap&&(!From.Equals(V.Centre,1.e-6)||!FMath::IsNearlyEqual(FromZoom,V.Zoom,1.e-5f)))
    {
        V.From=From;V.FromZoom=FromZoom;V.Centre=From;V.Zoom=FromZoom;Clamp(V);
        V.Started=FPlatformTime::Seconds();V.Duration=1.6;
    }
    return true;
}
void Focus(AWarfareFrontlineController* PC,const FString& StableId)
{
    auto* B=FrontlineFor(PC);const auto* MapSurface=B?Map(B):nullptr;if(!B||!MapSurface)return;Synchronise(PC);auto& V=View(PC);
    for(const auto& M:B->GetCampaignMissions())if(FString(M.MapKey)==StableId)
    {
        V.Preview=StableId;V.From=V.Centre;V.FromZoom=V.Zoom;FView End=V;End.Centre=UV(*MapSurface,M.MapPosition);End.Zoom=1.75f;Clamp(End);
        V.To=End.Centre;V.ToZoom=End.Zoom;V.Started=FPlatformTime::Seconds();V.Duration=V.OperationMap?0:1.65;V.Dragging=false;
        if(V.OperationMap){V.Centre=V.To;V.Zoom=V.ToZoom;}return;
    }
}
void Zoom(AWarfareFrontlineController* PC,float Ratio)
{if(!Active(PC))return;auto& V=View(PC);if(V.OperationMap)return;V.Duration=0;V.Zoom*=Ratio;Clamp(V);}
void Tick(AWarfareFrontlineController* PC,bool Focused)
{
    if(!Active(PC)){if(PC)View(PC).Dragging=false;return;}Synchronise(PC);auto& V=View(PC);
    if(V.Duration>0)
    {
        const float T=FMath::Clamp(float((FPlatformTime::Seconds()-V.Started)/V.Duration),0.f,1.f);
        const float Ease=T*T*T*(T*(T*6-15)+10);V.Centre=FMath::Lerp(V.From,V.To,double(Ease));V.Zoom=FMath::Exp(FMath::Lerp(FMath::Loge(V.FromZoom),FMath::Loge(V.ToZoom),Ease));Clamp(V);
        if(T>=1)V.Duration=0;
    }
    if(V.OperationMap){V.Dragging=false;return;}
    float MX=0,MY=0;int32 W=0,H=0;PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1920.f,H/1080.f);
    if(!Focused||Scale<=0||!PC->GetMousePosition(MX,MY)){V.Dragging=false;return;}
    const FVector2D Pointer((MX-(W-1920*Scale)*.5)/Scale,(MY-(H-1080*Scale)*.5)/Scale);
    const bool Down=PC->IsInputKeyDown(EKeys::RightMouseButton)&&Inside(Pointer);
    if(Down&&V.Dragging&&V.Duration<=0){const FVector2D Delta=Pointer-V.Pointer;V.Centre-=FVector2D(Delta.X/MapW,Delta.Y/MapH)/V.Zoom;Clamp(V);}
    V.Pointer=Pointer;V.Dragging=Down;
}
const FRRUArtV82::FHeadquarters* Headquarters(const FString& StableId,int32 Side)
{for(const auto& H:FRRUArtV82::Headquarters)if(FString(H.Mission)==StableId&&H.Side==Side)return &H;return nullptr;}
const FRRUArtV82::FCommander* Commander(const TCHAR* StableId)
{for(const auto& C:FRRUArtV82::Commanders)if(FString(C.Id)==StableId)return &C;return nullptr;}
FString Local(const TCHAR* EN,const TCHAR* ZH){return WarRoom52::L(EN,ZH);}
UTexture2D* OperationTexture(const FFrontlineCampaignMission* M)
{
    if(!M)return nullptr;FString Token=M->MapKey;Token.ReplaceInline(TEXT("."),TEXT("_"));
    const FString ObjectName=TEXT("T_Map_")+Token+TEXT("_V82");
    const FString ObjectPath=TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/")+ObjectName+TEXT(".")+ObjectName;
    return Texture(*ObjectPath);
}
FString Excerpt(const FString& NarrativeText)
{
    const int32 Break=NarrativeText.Find(TEXT("\n\n"));const FString Paragraph=Break==INDEX_NONE?NarrativeText:NarrativeText.Left(Break);
    int32 Sentences=0;
    for(int32 I=0;I<Paragraph.Len();++I)
    {
        const TCHAR Ch=Paragraph[I];const bool End=Ch==TCHAR(0x3002)||Ch==TEXT('!')||Ch==TEXT('?')||(Ch==TEXT('.')&&(I+1==Paragraph.Len()||FChar::IsWhitespace(Paragraph[I+1])));
        if(End&&++Sentences==2)return Paragraph.Left(I+1);
    }
    return Paragraph;
}
FString NarrativeExcerptV87(const FString& NarrativeText)
{
    TArray<FString> Blocks;NarrativeText.ParseIntoArray(Blocks,TEXT("\n\n"),false);
    // The published diary schema has a dateline, two narrative paragraphs and
    // a historian's line. Its dateline is metadata, not the situation report.
    return Excerpt(Blocks.Num()==4?Blocks[1]:NarrativeText);
}
void DrawOperation(const FFrontlineCanvas& UI,const FFrontlineCampaignMission* M)
{
    TGuardValue<bool> MapTypographyScopeV8210(UI.bLegacyTypographyV8210,true);
    UI.PaperSheet(MapX-9,MapY-9,MapW+18,MapH+18);if(!M)return;
    UI.Label(Local(TEXT("OPERATION AREA"),TEXT("作战地域")),MapX+28,MapY+25,MapW-56,32,20,MapInk);
    auto* OperationArt=OperationTexture(M);
    if(FrontlineTextureReady(OperationArt))UI.ImageContain(OperationArt,MapX+18,MapY+90,MapW-36,MapH-180);
    else UI.Paragraph(Local(TEXT("The operation map is unavailable. Use the theatre view to select this operation."),TEXT("作战地域图暂不可用，请切换战区图选择此次行动。")),MapX+100,MapY+290,MapW-200,130,24,MapInk);
    UI.Label(FString(M->DateLabel)+TEXT(" · ")+M->Location,MapX+28,MapY+MapH-60,MapW-56,35,22,MapInk);
}
void DrawSurface(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FView& V)
{
    TGuardValue<bool> MapTypographyScopeV8210(UI.bLegacyTypographyV8210,true);
    const auto* MapSurface=Map(B);if(!MapSurface)return;
    UI.PaperSheet(MapX-9,MapY-9,MapW+18,MapH+18);
    UI.Image(Texture(MapSurface->Texture),MapX,MapY,MapW,MapH,FLinearColor(.92f,.91f,.85f),V.Centre.X-.5/V.Zoom,V.Centre.Y-.5/V.Zoom,1/V.Zoom,1/V.Zoom);
    const auto* Selected=Preview(B,V);if(!Selected)return;
    const auto SelectedPin=Project(V,UV(*MapSurface,Selected->MapPosition));
    const bool HasSelectedLabel=Inside(SelectedPin,22)&&SelectedPin.X<MapX+MapW-245;
    const FBox2D SelectedLabel(FVector2D(SelectedPin.X+10,SelectedPin.Y-28),FVector2D(SelectedPin.X+248,SelectedPin.Y+22));
    // Discrete pencil marks retain pixel size as the same paper is magnified.
    for(const auto& M:B->GetCampaignMissions())
    {
        const auto Pin=Project(V,UV(*MapSurface,M.MapPosition));if(!Inside(Pin,22))continue;
        const bool Chosen=FString(M.MapKey)==V.Preview;const FLinearColor Pigment=Chosen?FieldRed:MapInk*.8f;
        // Nearby operations retain their real coordinates. The selected mark
        // takes precedence; dated buttons keep the neighbouring actions accessible.
        if(!Chosen&&(FVector2D::Distance(Pin,SelectedPin)<24.||(HasSelectedLabel&&SelectedLabel.IsInside(Pin))))continue;
        WarRoom52::Circle(UI,Pin,Chosen?11.f:6.f,Pigment,Chosen?2.f:1.f);UI.Line(Pin.X-6,Pin.Y+4,Pin.X+9,Pin.Y-6,Pigment,Chosen?2.f:1.f);
        if(Chosen&&Pin.X<MapX+MapW-245)UI.Label(M.Location,Pin.X+20,Pin.Y-20,218,34,22,MapInk);
    }
}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    auto* PC=Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController());if(!PC||!Active(PC))return;Synchronise(PC);auto& V=View(PC);const auto* M=Preview(B,V);
    UI.PageSurfaceV73(0,0,1920,1080,true);UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    UI.RoleLabel(EFrontlineTextRoleV8210::Inscription,Local(TEXT("OPERATIONS"),TEXT("作战室")),96,56,570,52,38,MapInk);
    MedalWallV8215::RibbonBarV8218(UI,WarfareFRRUCampaignV82::Nation(B),96,113,276,30,false,true,true);
    UI.PageButtonV73(674,82,278,48,Local(TEXT("THEATRE"),TEXT("战区")),TEXT(""),TEXT("FRRUMap:theatre"),true,!V.OperationMap,50);
    UI.PageButtonV73(968,82,298,48,Local(TEXT("OPERATION AREA"),TEXT("作战地域")),TEXT(""),TEXT("FRRUMap:operation"),true,V.OperationMap,50);
    if(V.OperationMap)DrawOperation(UI,M);else DrawSurface(UI,B,V);
    UI.Hit(MapX,MapY,MapW,MapH,TEXT("FRRUMap:surface"),45);
    if(!M)return;FString HoverTitle,HoverBody;UI.PaperSheet(1292,145,580,786);
    UI.Label(M->DateLabel,1320,170,526,35,23,MapInk);UI.Paragraph(M->Title,1320,218,526,84,32,MapInk);
    if(UI.Hovered(1320,170,526,132)&&M->HistoricalContext&&*M->HistoricalContext){HoverTitle=M->Title;HoverBody=M->HistoricalContext;}
    const FString NarrativeText=TextReviewV829::Loading(TEXT("narrative"),FrontlineLoadingPageV82::Narrative(B,M));
    UI.Paragraph(NarrativeExcerptV87(NarrativeText),1320,316,526,143,22,MapInk);
    if(UI.Hovered(1320,316,526,143)){HoverTitle=M->Title;HoverBody=NarrativeText;}
    for(int Side=0;Side<2;++Side)
    {
        const auto* H=Headquarters(M->MapKey,Side);const auto* C=H?Commander(H->Commander):nullptr;const float CY=Side?604.f:474.f;
        if(!C||!H)continue;UI.Label(Local(H->FormationEN,H->FormationZH),1428,CY+4,410,34,20,MapInk);UI.Label(Local(C->NameEN,C->NameZH),1428,CY+42,410,48,21,MapInk);
        if(auto* T=Texture(C->Portrait))UI.Image(T,1320,CY,90,104,FLinearColor::White,float(C->Column)/C->Columns,float(C->Row)/C->Rows,1.f/C->Columns,1.f/C->Rows);
        else
        {
            EFRRUNationV81 PreviewNation=WarfareFRRUCampaignV82::Nation(B);
            if(Side==1)if(const auto* Native=wfexp::tactical::FindMission(wfexpue::Narrow(M->MapKey)))
                PreviewNation=static_cast<EFRRUNationV81>(static_cast<uint8>(Native->enemy));
            UI.ImageContain(NationalTextureV82(PreviewNation,TEXT("Flag")),1320,CY+18,90,60);
        }
        if(UI.Hovered(1320,CY,516,104)){HoverTitle=Local(C->NameEN,C->NameZH);HoverBody=Local(C->BiographyEN,C->BiographyZH);}
    }
    UI.Label(Local(TEXT("TERRAIN"),TEXT("地貌")),1320,753,526,26,18,MapInk);
    UI.Paragraph(Excerpt(M->TerrainNotes),1320,783,526,45,20,MapInk);
    if(UI.Hovered(1320,753,526,77)){HoverTitle=Local(TEXT("TERRAIN"),TEXT("地貌"));HoverBody=M->TerrainNotes;}
    UI.Label(Local(TEXT("OBJECTIVE"),TEXT("任务")),1320,839,526,26,18,MapInk);
    UI.Paragraph(Excerpt(M->ObjectiveText),1320,870,526,40,20,MapInk);
    if(UI.Hovered(1320,839,526,71)){HoverTitle=Local(TEXT("OBJECTIVE"),TEXT("任务"));HoverBody=M->ObjectiveText;}
    const auto& Missions=B->GetCampaignMissions();for(int I=0;I<Missions.Num();++I)
    {
        const float BX=54+I*120;const auto& Item=Missions[I];const FString Date=FString(Item.DateLabel).Left(10);
        UI.PageButtonV73(BX,928,110,44,Date,TEXT(""),FName(*(TEXT("FRRUMap:preview:")+FString(Item.MapKey))),true,FString(Item.MapKey)==V.Preview,50);
        if(UI.Hovered(BX,928,110,44)){HoverTitle=Item.Title;HoverBody=FString(Item.DateLabel)+TEXT(" · ")+Item.Location;}
    }
    UI.PageButtonV73(48,994,250,54,Local(TEXT("RETURN"),TEXT("返回")),TEXT("Esc"),TEXT("MenuHome"));
    if(FrontlineCampaignV51::CanOpenEnding(B))UI.PageButtonV73(326,994,350,54,Local(TEXT("CAMPAIGN RECORD"),TEXT("战役档案")),TEXT(""),TEXT("V51CampaignRecord"));
    UI.PageButtonV73(930,994,320,54,Local(TEXT("ULTIMATE ARCHIVE"),TEXT("终极档案室")),TEXT(""),TEXT("V8215ArchiveOpen"),FrontlineUltimateArchiveV8215::NationAvailableV87(B,WarfareFRRUCampaignV82::Nation(B,0)));
    UI.PageButtonV73(1300,994,245,54,Local(TEXT("RESEARCH"),TEXT("研究")),TEXT("T"),TEXT("Technology"));
    const FString Block=WarfareFRRUCampaignV82::MissionBlockReason(B,M->MapKey);
    const FString RosterBlock=WarfareFRRUCampaignV82::DeploymentBlockReason(B);
    const bool AdjustRoster=V.Preview==WarfareFRRUCampaignV82::MissionId(B)&&!RosterBlock.IsEmpty();
    UI.PageButtonV73(1562,994,308,54,Local(TEXT("DEPLOY"),TEXT("部署部队")),TEXT("Enter"),TEXT("FRRUMap:deploy"),Block.IsEmpty()&&(WarfareFRRUCampaignV82::WorldReady(B)||AdjustRoster));
    if(UI.Hovered(1562,994,308,54)&&(!Block.IsEmpty()||!RosterBlock.IsEmpty())){HoverTitle=M->Title;HoverBody=Block.IsEmpty()?RosterBlock:Block;}
    if(!HoverBody.IsEmpty())
    {
        const float TX=FMath::Clamp(float(UI.Mouse.X)-250,60.f,1350.f),TY=FMath::Clamp(float(UI.Mouse.Y)-250,160.f,730.f);
        UI.PaperSheet(TX,TY,510,284);UI.Label(HoverTitle,TX+22,TY+16,466,48,23,MapInk);UI.Paragraph(HoverBody,TX+22,TY+76,466,188,19,MapInk,true);
    }
}
bool Hit(FName Name,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    const FString Key=Name.ToString();if(!Key.StartsWith(TEXT("FRRUMap:")))return false;if(!Active(PC))return true;Synchronise(PC);auto& V=View(PC);
    if(Key.StartsWith(TEXT("FRRUMap:preview:"))){Focus(PC,Key.Mid(16));return true;}
    if(Key==TEXT("FRRUMap:operation")){V.OperationMap=true;V.Duration=0;V.Dragging=false;return true;}
    if(Key==TEXT("FRRUMap:theatre")){V.OperationMap=false;V.Dragging=false;return true;}
    if(Key==TEXT("FRRUMap:wide")){V.OperationMap=false;V.Dragging=false;V.From=V.Centre;V.FromZoom=V.Zoom;V.To={.5,.5};V.ToZoom=1;V.Duration=1.4;V.Started=FPlatformTime::Seconds();return true;}
    if(Key==TEXT("FRRUMap:deploy"))
    {
        if(!WarfareFRRUCampaignV82::CampaignsAvailable(WarfareFRRUCampaignV82::Nation(B))){B->LastMessage=WarfareFRRUCampaignV82::WorldBlockReason(B);return true;}
        const bool AdjustRoster=V.Preview==WarfareFRRUCampaignV82::MissionId(B)&&!WarfareFRRUCampaignV82::DeploymentBlockReason(B).IsEmpty();
        if(V.Duration>0||(!WarfareFRRUCampaignV82::WorldReady(B)&&!AdjustRoster))return true;
        if(V.Preview!=WarfareFRRUCampaignV82::MissionId(B)&&!WarfareFRRUCampaignV82::SelectMission(B,V.Preview))return true;
        ScreenState(PC).Deployment=true;return true;
    }
    if(Key==TEXT("FRRUMap:surface"))
    {
        if(V.OperationMap)return true;
        float MX=0,MY=0;int W=0,H=0;PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1920.f,H/1080.f);if(Scale<=0||!PC->GetMousePosition(MX,MY))return true;
        const FVector2D Pointer((MX-(W-1920*Scale)*.5)/Scale,(MY-(H-1080*Scale)*.5)/Scale);const auto* MapSurface=Map(B);if(!MapSurface||!Inside(Pointer))return true;
        const FVector2D GeoUV=Unproject(V,Pointer);const FFrontlineCampaignMission* Found=nullptr;double Distance=1.e30;
        for(const auto& M:B->GetCampaignMissions())
        {const double D=FVector2D::DistSquared(UV(*MapSurface,M.MapPosition),GeoUV);if(D<Distance-1.e-7||(FMath::Abs(D-Distance)<=1.e-7&&B->CampaignMissionBlockReason(M.Id).IsEmpty())){Found=&M;Distance=D;}}
        if(Found)Focus(PC,Found->MapKey);return true;
    }
    return true;
}
bool Loading(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    if(!WarfareFRRUCampaignV82::Active(B))return false;auto* PC=Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController());if(!PC)return false;
    Synchronise(PC);auto V=View(PC);V.Preview=WarfareFRRUCampaignV82::MissionId(B);DrawSurface(UI,B,V);return true;
}
}
