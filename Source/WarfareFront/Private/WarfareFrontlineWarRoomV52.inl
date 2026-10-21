// Geographic map and dossier use one projection, viewport and input transform.
// The generated relief is illustrative; mission/front positions are authored data.
namespace WarRoom52
{
constexpr float X=48,Y=140,W=1220,H=1220.f*1050.f/1498.f;
const FLinearColor Ink(.028f,.036f,.036f),Faded(.115f,.135f,.13f),Blue(.055f,.16f,.20f),Red(.27f,.065f,.046f),Brass(.37f,.24f,.09f);
FString L(const TCHAR* EN,const TCHAR* ZH){return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?EN:ZH;}
struct FState
{
 TWeakObjectPtr<APlayerController> Owner;FGuid Campaign;
 int32 Preview=INDEX_NONE,LastSelected=INDEX_NONE,Stage=0,Tab=0;
 float Zoom=1.f,FlightFromZoom=1.f,FlightToZoom=1.f;
 FVector2D Centre=FVector2D(.5,.5),FlightFromCentre=FVector2D(.5,.5),FlightToCentre=FVector2D(.5,.5),LastPointer=FVector2D::ZeroVector;
 double FlightStart=0.,FlightDuration=0.,SheetStarted=0.,RouteStarted=0.;int32 FlightPhase=0;
 bool Dragging=false,OperationSheet=false,WasActive=false;FString Notice;
 int32 AtlasSector=-1,AtlasMode=0;
 float Detail=0,FromDetail=0,ToDetail=0,Roll=0,Tilt=0,DetailZoom=1;
 FVector2D DetailPan=FVector2D::ZeroVector;
};
int32 AtlasSectorFor(int32 Mission);
void AtlasChooseSector(FState& S,const AWarfareFrontlineGameMode* B,int32 Sector);
void AtlasTick(FState& S,double Now);
FVector2D AtlasPoint(const FState& S,FVector2D Geo);
FVector2D AtlasInverse(const FState& S,FVector2D Pixel);
void AtlasDraw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FState& S);
void AtlasBackdrop(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FState& S);
FState& State(APlayerController* PC)
{static FState S;if(S.Owner.Get()!=PC){S=FState();S.Owner=PC;}return S;}
struct FRegionTile{float U0,V0,U1,V1;};
constexpr FRegionTile RegionBounds[5]={{.205f,.055f,.655f,.505f},{.260f,.030f,.610f,.380f},{.275f,.165f,.635f,.525f},{.180f,.075f,.680f,.575f},{.235f,.135f,.675f,.575f}};
// Control-point registered to the same western-front Mercator atlas. Unlike
// the broad campaign sheets, this is contemporary 1:80,000 survey ink: at
// the final scale a village, rail junction and cut can be read as geography.
constexpr FRegionTile CambraiOperationBounds={.4344f,.3294724445f,.4688f,.3640051437f};
constexpr float OperationZoom=72.f;
constexpr FRegionTile ArrasOperationBounds={.4120f,.2984579690f,.4448f,.3312933785f};
constexpr float ArrasOperationZoom=60.f;
UTexture2D* Map(int32 StageIndex=INDEX_NONE);UTexture2D* Region(int32 StageIndex);UTexture2D* Operation(int32 StageIndex);
bool HasOperation(int32 Mission){return Mission==0||Mission==1||(Mission>=8&&Mission<=10);}
const FRegionTile& OperationBounds(int32 StageIndex){return StageIndex==0?ArrasOperationBounds:CambraiOperationBounds;}
float MissionOperationZoom(int32 StageIndex){return StageIndex==0?ArrasOperationZoom:OperationZoom;}
double Merc(double Lat){return FMath::Loge(FMath::Tan(PI/4+FMath::DegreesToRadians(Lat)/2));}
FVector2D UV(FVector2D LonLat){return FVector2D((LonLat.X+2.5)/12.5,(Merc(52)-Merc(LonLat.Y))/(Merc(52)-Merc(46.4)));}
int32 Stage(int32 Id){return Id<5?0:Id<8?1:Id<11?2:Id<14?3:4;}
float MaximumZoom(const FState& S){return 24.f;}
void Clamp(FState& S){S.Zoom=FMath::Clamp(S.Zoom,1.f,MaximumZoom(S));const double R=.5/S.Zoom;S.Centre.X=FMath::Clamp(S.Centre.X,R,1-R);S.Centre.Y=FMath::Clamp(S.Centre.Y,R,1-R);}
// The detailed front is part of the same UDIM document. It has eight times
// the native texel density of the surrounding theatre. Do not zoom a coarse
// ocean/interior page past its useful scale or switch to another image.
bool DetailedPageV64(FVector2D C)
{
 const int32 Column=FMath::FloorToInt(C.X*8),Row=FMath::FloorToInt(C.Y*8);
 return ((Column==2||Column==3)&&Row>=1&&Row<=3)||(Column==4&&(Row==2||Row==3));
}
void ClampManual(FState& S)
{
 Clamp(S);
 if(S.Zoom>8.f)
 {
  const double R=.6/S.Zoom;
  S.Centre.X=FMath::Clamp(S.Centre.X,.25+R,.625-R);
  S.Centre.Y=FMath::Clamp(S.Centre.Y,.125+R,.5-R);
  if(S.Centre.Y<.25+R)S.Centre.X=FMath::Min(S.Centre.X,.5-R);
 }
}
FVector2D StageCentre(const AWarfareFrontlineGameMode* B,int32 ChosenStage)
{
 FVector2D Sum=FVector2D::ZeroVector;int32 Count=0;
 for(const auto& Node:FrontlineCampaignV52::Nodes(B))if(Stage(Node.Id)==ChosenStage){Sum+=UV(Node.LonLat);++Count;}
 return Count?Sum/Count:FVector2D(.5,.5);
}
void Flight(FState& S,float Zoom,FVector2D Centre,float Seconds,int32 Phase=0)
{
 S.FlightFromZoom=S.Zoom;S.FlightFromCentre=S.Centre;S.FlightToZoom=FMath::Clamp(Zoom,1.f,MaximumZoom(S));
 S.FlightToCentre=Centre;S.FlightStart=FPlatformTime::Seconds();S.FlightDuration=FMath::Max(.08f,Seconds);S.FlightPhase=Phase;
}
void StopFlight(FState& S){S.FlightDuration=0.;S.FlightPhase=0;}
void BeginRoute(FState& S,const AWarfareFrontlineGameMode* B)
{
 S.Zoom=1.f;S.Centre=FVector2D(.5,.5);S.OperationSheet=false;StopFlight(S);
 S.AtlasMode=0;S.AtlasSector=-1;S.Detail=S.FromDetail=S.ToDetail=0;S.Roll=S.Tilt=0;
 S.RouteStarted=FPlatformTime::Seconds();
}
void FocusOperation(FState& S,const AWarfareFrontlineGameMode* B,FVector2D Destination,bool ChangedStage,float DirectSeconds)
{
 S.OperationSheet=false;S.AtlasMode=2;S.AtlasSector=AtlasSectorFor(S.Preview);
 S.FromDetail=S.Detail;S.ToDetail=1;S.DetailZoom=1;S.DetailPan=FVector2D::ZeroVector;

 Flight(S,20.f,Destination,ChangedStage||S.Zoom<1.8f?3.2f:2.5f,12);
}
FVector2D Project(const FState& S,FVector2D LonLat)
{const FVector2D P=UV(LonLat);return FVector2D(X+W*(.5+(P.X-S.Centre.X)*S.Zoom),Y+H*(.5+(P.Y-S.Centre.Y)*S.Zoom));}
bool Inside(FVector2D P,float Margin=0){return P.X>=X+Margin&&P.X<=X+W-Margin&&P.Y>=Y+Margin&&P.Y<=Y+H-100-Margin;}
void Synchronize(FState& S,const AWarfareFrontlineGameMode* B)
{
 const auto* R=B->GetCurrentCampaign();if(!R)return;
 if(S.Campaign!=R->Id||S.LastSelected!=R->SelectedMission)
 {
  const bool NewCampaign=S.Campaign!=R->Id;const int32 PreviousStage=S.Stage;
  S.Campaign=R->Id;S.Preview=R->SelectedMission;S.Stage=Stage(S.Preview);S.Tab=0;S.Notice.Empty();
  if(NewCampaign)BeginRoute(S,B);
  if(B->GetCampaignMissions().IsValidIndex(S.Preview))
   FocusOperation(S,B,UV(FrontlineCampaignV52::GetNode(B,S.Preview).LonLat),PreviousStage!=S.Stage,.86f);
 }
 S.LastSelected=R->SelectedMission;
 if(!B->GetCampaignMissions().IsValidIndex(S.Preview))S.Preview=R->SelectedMission;
 Clamp(S);
}
bool Active(AWarfareFrontlineController* PC)
{
 const auto* B=FrontlineFor(PC);if(!B)return false;const auto& S=ScreenState(PC);
 return B->Phase==EFrontlinePhase::Briefing&&B->GetCurrentCampaign()&&S.Dossier==1917&&!S.Settings&&!S.Technology&&!S.Help&&!S.AchievementsV76&&!S.Preparing&&!S.Deployment&&!S.HistoricalDispatch&&!S.UnitIntroduction&&!S.CampaignEndingV51;
}
void ZoomAt(FState& S,float Factor,FVector2D Anchor)
{
 StopFlight(S);S.OperationSheet=false;
 const float Next=FMath::Clamp(S.Zoom*Factor,1.f,DetailedPageV64(S.Centre)?24.f:3.f);
 S.Centre+=Anchor*(1./S.Zoom-1./Next);S.Zoom=Next;ClampManual(S);
}
void Zoom(AWarfareFrontlineController* PC,float Factor)
{
 if(!Active(PC))return;auto& S=State(PC);if(S.FlightDuration>0.)return;
 
 float MX=0,MY=0;int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);if(!PC->GetMousePosition(MX,MY))return;
 const float Scale=FMath::Min(VX/1920.f,VY/1080.f);if(Scale<=0)return;
 const FVector2D P((MX-(VX-1920*Scale)*.5)/Scale,(MY-(VY-1080*Scale)*.5)/Scale);
 const FVector2D Before=AtlasInverse(S,P);
 S.Zoom=FMath::Clamp(S.Zoom*Factor,1.f,DetailedPageV64(Before)?24.f:3.f);
 // Recompute the true camera before inverse picking the new scale.
 FrontlineMapSceneV61::Update(PC,true,S.Centre,S.Zoom,1.f,S.AtlasSector,S.Preview,S.Detail);
 S.Centre+=Before-AtlasInverse(S,P);ClampManual(S);
}
void Tick(AWarfareFrontlineController* PC,bool Focused)
{
 auto& S=State(PC);
 if(!Active(PC)){S.Dragging=false;S.WasActive=false;return;}
 
 if(!S.WasActive){S.WasActive=true;if(auto* B=FrontlineFor(PC))Synchronize(S,B);}
 AtlasTick(S,FPlatformTime::Seconds());
 if(!Focused){S.Dragging=false;return;}
 float MX=0,MY=0;int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);if(!PC->GetMousePosition(MX,MY))return;
 const float Scale=FMath::Min(VX/1920.f,VY/1080.f);if(Scale<=0)return;
 const FVector2D P((MX-(VX-1920*Scale)*.5)/Scale,(MY-(VY-1080*Scale)*.5)/Scale);
 if(S.FlightDuration>0.){S.Dragging=false;return;}
 if(PC->IsInputKeyDown(EKeys::RightMouseButton))
 {
  if(S.Dragging){const auto D=P-S.LastPointer;
   S.Centre+=AtlasInverse(S,S.LastPointer)-AtlasInverse(S,P);ClampManual(S);}
  else S.Dragging=P.Y>132&&P.Y<996&&(S.AtlasMode!=2||P.X<1270);
  S.LastPointer=P;
 }else S.Dragging=false;
}
UTexture2D* Map(int32 StageIndex)
{
 static TStrongObjectPtr<UTexture2D> Theatre;
 if(!Theatre.IsValid())Theatre.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Campaign/V54/T_WesternFront_V54.T_WesternFront_V54")));
 return Theatre.Get();
}
UTexture2D* Region(int32 StageIndex)
{
 static TStrongObjectPtr<UTexture2D> T;static int32 Cached=INDEX_NONE;static bool Tried=false;
 const int32 I=FMath::Clamp(StageIndex,0,4);
 if(Cached!=I){T.Reset();Cached=I;Tried=false;}
 if(!Tried)
 {
  const FString Name=FString::Printf(TEXT("T_Region_%d"),I);
  if(I==0||I==2)
  {
   // Two accepted sector sheets retain the V54 projection/UV footprint but
   // carry native-resolution geographic ink through the operation zoom.
   const FString Path=FString::Printf(TEXT("/Game/Warfare/Campaign/V56/%s.%s"),*Name,*Name);
   T.Reset(LoadObject<UTexture2D>(nullptr,*Path));
  }
  if(!T.IsValid())
  {
   const FString Path=FString::Printf(TEXT("/Game/Warfare/Campaign/V54/%s.%s"),*Name,*Name);
   T.Reset(LoadObject<UTexture2D>(nullptr,*Path));
  }
  Tried=true;
 }
 return T.Get();
}
UTexture2D* Operation(int32 StageIndex)
{
 static TStrongObjectPtr<UTexture2D> Cambrai,Arras;
 if(StageIndex==2)
 {
  if(!Cambrai.IsValid())Cambrai.Reset(LoadObject<UTexture2D>(nullptr,
   TEXT("/Game/Warfare/Campaign/V57/T_Cambrai_Operation_V57.T_Cambrai_Operation_V57")));
  return Cambrai.Get();
 }
 if(StageIndex==0)
 {
  if(!Arras.IsValid())Arras.Reset(LoadObject<UTexture2D>(nullptr,
   TEXT("/Game/Warfare/Campaign/V57/T_Arras_Operation_V57.T_Arras_Operation_V57")));
  return Arras.Get();
 }
 return nullptr;
}
bool RegionVisible(const FState& S,int32 StageIndex)
{
 const auto& T=RegionBounds[FMath::Clamp(StageIndex,0,4)];const float R=.5f/S.Zoom;
 return S.Centre.X+R>T.U0&&S.Centre.X-R<T.U1&&S.Centre.Y+R>T.V0&&S.Centre.Y-R<T.V1;
}
void DrawRegion(const FFrontlineCanvas& UI,const FState& S)
{
 if(S.Zoom<1.40f||!RegionVisible(S,S.Stage))return;
 auto* Texture=Region(S.Stage);if(!Texture)return;
 const auto& Tile=RegionBounds[S.Stage];const float Radius=.5f/S.Zoom;
 const float Left=S.Centre.X-Radius,Top=S.Centre.Y-Radius,Right=S.Centre.X+Radius,Bottom=S.Centre.Y+Radius;
 const float A=FMath::Max(Left,Tile.U0),B=FMath::Max(Top,Tile.V0),C=FMath::Min(Right,Tile.U1),D=FMath::Min(Bottom,Tile.V1);
 if(C<=A||D<=B)return;
 // Geographic registration is invariant through the flight. The source's
 // feathered alpha blends detail into the matching theatre plate without a
 // second paper rectangle or an exposed atlas seam.
 const float Alpha=FMath::SmoothStep(1.40f,2.16f,S.Zoom);
 if(Alpha<=.001f)return;
 const float DX=X+W*(A-Left)/(Right-Left),DY=Y+H*(B-Top)/(Bottom-Top);
 const float DW=W*(C-A)/(Right-Left),DH=H*(D-B)/(Bottom-Top);
 UI.Image(Texture,DX,DY,DW,DH,FLinearColor(.9f,.94f,.93f,Alpha),
  (A-Tile.U0)/(Tile.U1-Tile.U0),(B-Tile.V0)/(Tile.V1-Tile.V0),
  (C-A)/(Tile.U1-Tile.U0),(D-B)/(Tile.V1-Tile.V0));
}
void DrawOperation(const FFrontlineCanvas& UI,const FState& S)
{
 if(!HasOperation(S.Preview)||S.Zoom<6.f)return;
 auto* Texture=Operation(S.Stage);if(!Texture)return;
 const auto& Tile=OperationBounds(S.Stage);const float Radius=.5f/S.Zoom;
 const float Left=S.Centre.X-Radius,Top=S.Centre.Y-Radius,Right=S.Centre.X+Radius,Bottom=S.Centre.Y+Radius;
 const float A=FMath::Max(Left,Tile.U0),B=FMath::Max(Top,Tile.V0),C=FMath::Min(Right,Tile.U1),D=FMath::Min(Bottom,Tile.V1);
 if(C<=A||D<=B)return;
 const float Alpha=FMath::SmoothStep(6.f,15.f,S.Zoom);
 if(Alpha<=.001f)return;
 UI.Image(Texture,X+W*(A-Left)/(Right-Left),Y+H*(B-Top)/(Bottom-Top),
  W*(C-A)/(Right-Left),H*(D-B)/(Bottom-Top),FLinearColor(.9f,.94f,.93f,Alpha),
  (A-Tile.U0)/(Tile.U1-Tile.U0),(B-Tile.V0)/(Tile.V1-Tile.V0),
  (C-A)/(Tile.U1-Tile.U0),(D-B)/(Tile.V1-Tile.V0));
}
void Circle(const FFrontlineCanvas& UI,FVector2D P,float R,FLinearColor C,float Weight=1)
{for(int32 I=0;I<40;++I){const float A=I*2*PI/40,Z=(I+1)*2*PI/40;UI.Line(P.X+R*FMath::Cos(A),P.Y+R*FMath::Sin(A),P.X+R*FMath::Cos(Z),P.Y+R*FMath::Sin(Z),C,Weight);}}
// The same physical sheet is docked on the map and carried into preparation.
// Layout blends from the compact map card to the full loading-page sheet.
void OperationSheet(const FFrontlineCanvas& UI,UTexture2D* Texture,const FString& Location,
 float PX,float PY,float PW,float PH,float Alpha,float Layout)
{
 if(!Texture||PW<=0||PH<=0||Alpha<=0)return;
 const float Pad=FMath::Lerp(11.f,22.f,Layout),Header=FMath::Lerp(33.f,62.f,Layout);
 const float Footer=FMath::Lerp(10.f,100.f,Layout);
 UI.Rect(PX+6,PY+8,PW,PH,FLinearColor(.008f,.012f,.011f,.34f*Alpha));
 UI.Rect(PX,PY,PW,PH,FLinearColor(.12f,.115f,.092f,.95f*Alpha));
 UI.Rect(PX+4,PY+4,PW-8,PH-8,FLinearColor(.75f,.72f,.61f,.98f*Alpha));
 UI.Line(PX+Pad,PY+Header-8,PX+PW-Pad,PY+Header-8,FLinearColor(.31f,.18f,.11f,.40f*Alpha),1.f);
 if(PW>250.f)UI.Label(L(TEXT("OPERATION SHEET"),TEXT("作战图纸")),PX+Pad,PY+8,PW-2*Pad,Header-13,
  FMath::Lerp(15.f,19.f,Layout),FLinearColor(Ink.R,Ink.G,Ink.B,Alpha));
 const float ImageH=FMath::Max(1.f,PH-Header-Footer);
 UI.ImageContain(Texture,PX+Pad,PY+Header,PW-2*Pad,ImageH,FLinearColor(.94f,.93f,.87f,Alpha));
 if(Layout>.28f)
 {
  const float FooterAlpha=Alpha*FMath::SmoothStep(.15f,.85f,Layout);
  UI.Line(PX+Pad,PY+PH-Footer+8,PX+PW-Pad,PY+PH-Footer+8,FLinearColor(.12f,.13f,.11f,.38f*FooterAlpha),1.f);
  UI.Label(Location,PX+Pad,PY+PH-Footer+26,PW-2*Pad,FMath::Max(1.f,Footer-30),FMath::Lerp(15.f,21.f,Layout),
   FLinearColor(Ink.R,Ink.G,Ink.B,FooterAlpha));
 }
}
UTexture2D* OperationV61(int32 Mission)
{
 static TStrongObjectPtr<UTexture2D> Sheet;static int32 Index=INDEX_NONE;
 if(Index!=Mission){Index=Mission;const FString N=FString::Printf(TEXT("T_Operation_%02d"),FMath::Clamp(Mission,0,19));
 Sheet.Reset(LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Warfare/Campaign/V61/")+N+TEXT(".")+N)));}
 return Sheet.Get();
}
bool LoadingGround(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* Mission,
 const FState* FromWarRoom=nullptr,float Transition=1.f)
{
 if(!B||!Mission)return false;
 if(auto* Snapshot=FrontlineMapSceneV61::LoadingView(Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController())))
 {
  const float ViewAspect=UI.Surface->ClipX/FMath::Max(1.f,UI.Surface->ClipY);
  const float ImageAspect=Snapshot->GetSurfaceWidth()/FMath::Max(1.f,Snapshot->GetSurfaceHeight());
  // Preserve the photographed paper's proportions when the window is resized
  // during preparation. At the captured aspect this remains the exact view.
  const float USize=FMath::Min(1.f,ViewAspect/ImageAspect),VSize=FMath::Min(1.f,ImageAspect/ViewAspect);
  UI.HUD.DrawTexture(Snapshot,0,0,UI.Surface->ClipX,UI.Surface->ClipY,(1-USize)*.5f,(1-VSize)*.5f,USize,VSize,FLinearColor::White,BLEND_Opaque);
  return true;
 }
 auto* Cover=MenuCoverV63(B);const bool CoverDrawn=FrontlineTextureReady(Cover);
 if(CoverDrawn)UI.Image(Cover,0,0,1920,1080,FLinearColor(.32f,.34f,.34f,1));
 if(auto* T=CampaignMapTextureV41(B,Mission);FrontlineTextureReady(T))
 {
  UI.ImageContain(T,68,225,1163,620,FLinearColor(.85f,.86f,.80f,1));return true;
 }
 return CoverDrawn;
}

void Segment(const FFrontlineCanvas& UI,FVector2D A,FVector2D B,FLinearColor C,float Weight=1)
{
 // Liang-Barsky clips ink before it can reach dossier text or outer controls.
 const FVector2D D=B-A;double T0=0,T1=1;
 auto Edge=[&](double P,double Q){if(FMath::Abs(P)<1e-8)return Q>=0;const double R=Q/P;if(P<0){if(R>T1)return false;T0=FMath::Max(T0,R);}else{if(R<T0)return false;T1=FMath::Min(T1,R);}return true;};
 if(Edge(-D.X,A.X-X-6)&&Edge(D.X,X+W-6-A.X)&&Edge(-D.Y,A.Y-Y-6)&&Edge(D.Y,Y+H-112-A.Y))
 {const auto P=A+D*T0,Q=A+D*T1;UI.Line(P.X,P.Y,Q.X,Q.Y,C,Weight);}
}
void Portrait(const FFrontlineCanvas& UI,const FFrontlineCampaignMission& M,int32 F,float PX,float PY)
{
 const int32 I=M.CommanderPortraitV41[F];UI.Rect(PX-3,PY-3,75,81,FLinearColor(.14f,.13f,.105f,.7f));
 if(F==1&&I==10)UI.ImageContain(FrontlineCampaignV41::QuastPortrait(),PX,PY,69,75);
 else{const auto A=FrontlinePresentationV37::Commander(F==1,I);auto* T=A.Atlas==2?Artwork().CommandersV37.Get():Artwork().CommandersV30[A.Atlas].Get();
 if(T)UI.Image(T,PX,PY,69,75,FLinearColor(.96f,.96f,.92f,1),(A.Slot%A.Columns+.015f)/A.Columns,(A.Slot/A.Columns+.015f)/A.Rows,.97f/A.Columns,.97f/A.Rows,false,true);}
 const auto Bio=CommanderBiographyV41(M,F);const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Bio.Name,PX+82,PY+6,162,33,Chinese?15.f:20.f,Ink);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Bio.Role,PX+82,PY+42,162,52,15,Faded);
}
void CommanderTip(const FFrontlineCanvas& UI,const FFrontlineCampaignMission& M)
{
 for(int32 F=0;F<2;++F)if(UI.Hovered(1333+F*263,319,245,75))
 {const auto Bio=CommanderBiographyV41(M,F);UI.PaperSheet(1322,418,543,348);const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Bio.Name,1344,438,494,42,Chinese?22.f:30.f,Ink);UI.RoleLabel(EFrontlineTextRoleV8210::FieldValue,Bio.Role,1344,486,494,30,15,Faded);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Bio.Text,1344,532,494,210,17,Ink);}
}
FString TerrainHint(const FFrontlineCampaignMission& M)
{
 if(M.Id==7)return L(TEXT("The raised road is quick but exposed; the shelled wooded gullies hide a slower approach."),
  TEXT("高起的道路行进快却暴露；遭炮击的林间冲沟可掩护较慢的接近。"));
 if(M.Id==12)return L(TEXT("The Lys wash cuts the advance. Its two dry fords spare men and tracks the flooded bed."),
  TEXT("利斯河漫水带横断前进线；两处干燥渡点可让步兵与履带避开深泥。"));
 if(M.Id==13)return L(TEXT("The angled village road carries armour; ruined shoulders and cellars shelter slower infantry."),
  TEXT("村口斜路便于装甲通过；破墙与地窖让步兵走得更慢，却有掩护。"));
 switch(M.TerrainStyleV32)
 {
  case EFrontlineMissionTerrainV32::Ridge:return L(TEXT("The crest commands the low ground; gullies cut the approach."),TEXT("高地压住低处，冲沟割断接近线。"));
  case EFrontlineMissionTerrainV32::Marsh:return L(TEXT("Flooded drains slow every crossing, especially the heavy tracks."),TEXT("漫水的排水沟阻滞渡越，沉重的履带尤其难行。"));
  case EFrontlineMissionTerrainV32::Canal:return L(TEXT("The cut delays a crossing; its low floor shelters defenders."),TEXT("运河切口阻滞渡越，低处可暂避直射火力。"));
  case EFrontlineMissionTerrainV32::Village:return L(TEXT("Collapsed cellars shelter men; fallen masonry checks armour."),TEXT("坍塌地窖可隐蔽守军，碎石阻滞装甲。"));
  case EFrontlineMissionTerrainV32::RailCutting:return L(TEXT("The rail shoulder checks vehicles; the train stays on its track."),TEXT("铁路路肩阻滞车辆，列车只能沿轨机动。"));
  default:break;
 }
 if(M.Id==10)return L(TEXT("Frozen furrows and snow drifts break a quick crossing."),TEXT("冻土田沟与积雪打断快速穿越。"));
 if(M.Id==8||M.Id==14)return L(TEXT("Deep vehicle ruts slow armour; firm field edges matter."),TEXT("深陷履痕拖慢装甲，坚实田界成为通路。"));
 return L(TEXT("Broken field drains slow crossing and shelter men who hold."),TEXT("破损田沟拖慢渡越，也能护住伏守的士兵。"));
}
bool Command(FName Name,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
 if(!Active(PC))return false;auto& S=State(PC);Synchronize(S,B);const FString N=Name.ToString();
 if(Name==TEXT("V60Current")){const auto* R=B->GetCurrentCampaign();if(R){S.Preview=R->SelectedMission;S.Stage=Stage(S.Preview);FocusOperation(S,B,UV(FrontlineCampaignV52::GetNode(B,S.Preview).LonLat),true,2.f);}return true;}
 if((Name==TEXT("V52Begin")||Name==TEXT("V52Deployment"))&&(S.AtlasSector>=5||S.FlightDuration>0.))return true;
 if(N.StartsWith(TEXT("V60Sector"))){AtlasChooseSector(S,B,FCString::Atoi(*N.Mid(9)));return true;}
 if(N.StartsWith(TEXT("V52Node"))){const int32 Id=FCString::Atoi(*N.Mid(7));if(B->GetCampaignMissions().IsValidIndex(Id)){const int32 PreviousStage=S.Stage;S.Preview=Id;S.Stage=Stage(Id);S.Notice.Empty();FocusOperation(S,B,UV(FrontlineCampaignV52::GetNode(B,Id).LonLat),PreviousStage!=S.Stage,.74f);}return true;}
 if(N.StartsWith(TEXT("V52Stage"))){AtlasChooseSector(S,B,FMath::Clamp(FCString::Atoi(*N.Mid(8)),0,4));return true;}
 if(N.StartsWith(TEXT("V52Tab"))){S.Tab=FMath::Clamp(FCString::Atoi(*N.Mid(6)),0,3);return true;}
 if(Name==TEXT("V52ZoomIn")){Zoom(PC,1.25f);return true;}if(Name==TEXT("V52ZoomOut")){Zoom(PC,.8f);return true;}
 if(Name==TEXT("V52Overview")){S.OperationSheet=false;S.AtlasMode=0;S.AtlasSector=-1;S.FromDetail=S.Detail;S.ToDetail=0;Flight(S,1.f,FVector2D(.5,.5),1.7f,10);return true;}
 if(Name==TEXT("V52Focus")){const auto Node=FrontlineCampaignV52::GetNode(B,S.Preview);FocusOperation(S,B,UV(Node.LonLat),false,.86f);return true;}
 if(Name==TEXT("V52Begin"))
 {if(FrontlineCampaignV52::Select(B,S.Preview)){ScreenState(PC).AcceptedLoadoutMission=INDEX_NONE;PC->BeginBattle();}else S.Notice=B->CampaignMissionBlockReason(S.Preview);return true;}
 if(Name==TEXT("V52Deployment"))
 {if(FrontlineCampaignV52::Select(B,S.Preview)){FrontlineMapSceneV61::CaptureLoadingView(PC);ScreenState(PC).AcceptedLoadoutMission=INDEX_NONE;ScreenState(PC).Deployment=true;}else S.Notice=B->CampaignMissionBlockReason(S.Preview);return true;}
 if(Name==TEXT("V52Dispatch"))
 {if(FrontlineCampaignV52::Select(B,S.Preview)&&FrontlineCampaignV52::Dispatch(B).bPending){FrontlineMapSceneV61::CaptureLoadingView(PC);ScreenState(PC).HistoricalDispatch=true;}return true;}
 return Name==TEXT("V52MapSurface");
}
void MapPanel(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FState& S)
{
 UI.Rect(X+6,Y+9,W,H,FLinearColor(0,0,0,.7f));UI.Rect(X-5,Y-5,W+10,H+10,FLinearColor(.24f,.225f,.184f));
 if(auto* Theatre=Map(S.Stage))UI.Image(Theatre,X,Y,W,H,FLinearColor(.9f,.94f,.93f,1),S.Centre.X-.5/S.Zoom,S.Centre.Y-.5/S.Zoom,1/S.Zoom,1/S.Zoom);
 else UI.Rect(X,Y,W,H,FLinearColor(.45f,.45f,.39f));
 DrawRegion(UI,S);
 DrawOperation(UI,S);
 UI.Hit(X,Y,W,H-104,TEXT("V52MapSurface"),45);
 // Fine geographic graticule; subdued ink is part of the paper, not a UI grid.
 for(int32 Lon=-2;Lon<=10;++Lon){const auto A=Project(S,FVector2D(Lon,46.4)),Z=Project(S,FVector2D(Lon,52));Segment(UI,A,Z,FLinearColor(.09f,.13f,.13f,.10f),.6f);}
 for(int32 Lat=47;Lat<=52;++Lat){const auto A=Project(S,FVector2D(-2.5,Lat)),Z=Project(S,FVector2D(10,Lat));Segment(UI,A,Z,FLinearColor(.09f,.13f,.13f,.10f),.6f);}
 struct FPlace{const TCHAR* Name;double Lon,Lat;int32 Size;};
 const FPlace Places[]={{TEXT("FRANCE"),2.1,48.4,30},{TEXT("BELGIQUE"),4.65,50.78,23},{TEXT("DEUTSCHES REICH"),7.85,50.8,22},{TEXT("SCHWEIZ"),7.5,47.35,21},{TEXT("ENGLAND"),-.95,51.1,23},{TEXT("NEDERLAND"),5.2,51.65,20},{TEXT("PARIS"),2.35,48.857,20},{TEXT("VERDUN"),5.38,49.16,17},{TEXT("REIMS"),4.03,49.258,17},{TEXT("BRUXELLES"),4.35,50.85,16},{TEXT("STRASBOURG"),7.75,48.58,17}};
 for(const auto& T:Places){if(S.Zoom<1.30f&&T.Size<20)continue;const auto P=Project(S,FVector2D(T.Lon,T.Lat));if(Inside(P,35)&&P.X+170<X+W)UI.Text(T.Name,P.X,P.Y,T.Size,FLinearColor(.06f,.09f,.09f,.63f));}
 const auto Front=FrontlineCampaignV52::Frontline(B,S.Preview);
 const float Trace=S.RouteStarted>0.
  ?FMath::SmoothStep(.15f,1.65f,float(FPlatformTime::Seconds()-S.RouteStarted))*FMath::Max(1,Front.Points.Num()-1)
  :float(Front.Points.Num());
 for(int32 I=1;I<Front.Points.Num();++I)
 {const float Part=FMath::Clamp(Trace-float(I-1),0.f,1.f);if(Part<=0.f)continue;
 const auto A=Project(S,Front.Points[I-1]),Z=FMath::Lerp(A,Project(S,Front.Points[I]),Part);Segment(UI,A+FVector2D(-3,0),Z+FVector2D(-3,0),Blue,.9f);Segment(UI,A,Z,Red,2.1f);
 const float Length=FVector2D::Distance(A,Z);for(float V=8;V<Length;V+=16){const auto P=FMath::Lerp(A,Z,V/Length);Segment(UI,P,P+FVector2D(4,2),Red,1.f);}}
 // Four restrained survey corners identify the geographic area the camera is
 // approaching. They shrink out with the flight; no separate map slides over
 // the theatre or shifts the selected front's coordinates.
 if(S.FlightDuration>0.&&S.FlightPhase!=0)
 {
  const auto Focus=FrontlineCampaignV52::GetNode(B,S.Preview);
  const float Alpha=FMath::SmoothStep(1.03f,1.42f,S.Zoom)*(1.f-FMath::SmoothStep(2.0f,2.9f,S.Zoom));
  if(Focus.Mission&&Alpha>.005f)
  {
   const auto P=UV(Focus.LonLat);constexpr double R=0.5/3.6;
   const auto TL=FVector2D(X+W*(.5+(P.X-R-S.Centre.X)*S.Zoom),Y+H*(.5+(P.Y-R-S.Centre.Y)*S.Zoom));
   const auto BR=FVector2D(X+W*(.5+(P.X+R-S.Centre.X)*S.Zoom),Y+H*(.5+(P.Y+R-S.Centre.Y)*S.Zoom));
   const FLinearColor C(Brass.R,Brass.G,Brass.B,.58f*Alpha);constexpr float Corner=23.f;
   for(int32 Side=0;Side<2;++Side)for(int32 End=0;End<2;++End)
   {
    const float PX=Side?BR.X:TL.X,PY=End?BR.Y:TL.Y;
    Segment(UI,FVector2D(PX,PY),FVector2D(PX+(Side?-Corner:Corner),PY),C,1.3f);
    Segment(UI,FVector2D(PX,PY),FVector2D(PX,PY+(End?-Corner:Corner)),C,1.3f);
   }
  }
 }
 const auto Nodes=FrontlineCampaignV52::Nodes(B);TArray<FVector2D> LabelPositions;
 const float PinAlpha=FMath::SmoothStep(1.20f,1.68f,S.Zoom);
 const float StageAlpha=1.f-FMath::SmoothStep(1.20f,1.68f,S.Zoom);
 struct FPin{const FrontlineCampaignV52::FNode* Node;FVector2D Anchor,Label;FLinearColor Color;bool Selected;};TArray<FPin> Pins;
 // Nearby operations separate into pinned labels; leaders retain exact map positions.
 for(const auto& N:Nodes)
 {
  if(Stage(N.Id)!=S.Stage||PinAlpha<=.01f)continue;const auto Anchor=Project(S,N.LonLat);if(!Inside(Anchor,20))continue;
  const bool Selected=N.Id==S.Preview,Open=N.State==FrontlineCampaignV52::ENodeState::Available||N.State==FrontlineCampaignV52::ENodeState::Selected;
  const FLinearColor C=Selected?Red:N.State==FrontlineCampaignV52::ENodeState::Completed?Blue:Open?Ink:Faded;
  FVector2D Label=Anchor+FVector2D(N.Id%2?34:-214,-24);
  Label.X=FMath::Clamp(Label.X,double(X+20),double(X+W-226));Label.Y=FMath::Clamp(Label.Y,double(Y+108),double(Y+H-240));
  for(int32 Attempt=0;Attempt<12;++Attempt){bool Clash=false;for(const auto& Previous:LabelPositions)if(FMath::Abs(Previous.X-Label.X)<224&&FMath::Abs(Previous.Y-Label.Y)<59){Clash=true;break;}if(!Clash)break;Label.Y+=61;if(Label.Y>Y+H-240){Label.Y=Y+113;Label.X=FMath::Clamp(Label.X+235,double(X+20),double(X+W-225));}}
  LabelPositions.Add(Label);Pins.Add({&N,Anchor,Label,C,Selected});
 }
 // Draw every leader first: later operations must never cross earlier lettering.
 for(const auto& P:Pins)
 {
  if(P.Selected&&S.FlightDuration<=0.)
  {
   const float Pulse=.5f+.5f*FMath::Sin(float(FPlatformTime::Seconds())*2.2f);
   Circle(UI,P.Anchor,11.f+4.f*Pulse,FLinearColor(Red.R,Red.G,Red.B,.23f*PinAlpha*(1.f-Pulse)),1.2f);
  }
  Circle(UI,P.Anchor,P.Selected?8:5,P.Color*PinAlpha,P.Selected?2.5f:1.4f);
  Segment(UI,P.Anchor,FVector2D(P.Label.X+102,P.Label.Y+24),P.Color*(.75f*PinAlpha),1.1f);
 }
 for(const auto& P:Pins)
 {
  const auto& N=*P.Node;const auto Label=P.Label,Anchor=P.Anchor;const auto C=P.Color;const bool Selected=P.Selected;
  UI.Rect(Label.X+2,Label.Y+3,209,48,FLinearColor(.03f,.03f,.022f,.19f*PinAlpha));UI.Rect(Label.X,Label.Y,209,48,(Selected?FLinearColor(.69f,.63f,.49f,.97f):FLinearColor(.66f,.65f,.55f,.9f))*PinAlpha);
  UI.Line(Label.X,Label.Y+47,Label.X+209,Label.Y+47,C*PinAlpha,Selected?2.5f:1.f);
  UI.Label(FString::Printf(TEXT("%02d"),N.Id+1),Label.X+10,Label.Y+5,30,36,23,C*PinAlpha);
  FString Place=FrontlineLocalizationV35::Text(N.Mission->Location),Country,Local;
  if(Place.Split(TEXT(" · "),&Country,&Local))Place=Local;
  UI.Label(Place,Label.X+46,Label.Y+3,154,23,17,C*PinAlpha);
  UI.Label(N.StateText,Label.X+46,Label.Y+28,154,15,11,C*PinAlpha);
  if(PinAlpha>.5f){UI.Hit(Label.X-3,Label.Y-3,215,54,FName(*FString::Printf(TEXT("V52Node%d"),N.Id)),65);
   UI.Hit(Anchor.X-14,Anchor.Y-14,28,28,FName(*FString::Printf(TEXT("V52Node%d"),N.Id)),64);}
 }
 if(StageAlpha>.01f)
 {
  // At theatre scale, show campaigns rather than twenty overlapping town labels.
  const FVector2D Offsets[5]={{-214,-105},{-231,-183},{53,-38},{-226,73},{65,103}};
  const TCHAR* Short[5]={TEXT("ARRAS / MESSINES"),TEXT("YPRES"),TEXT("CAMBRAI"),TEXT("SPRING 1918"),TEXT("HUNDRED DAYS")};
  for(int32 I=0;I<5;++I)
  {
   const auto P=FVector2D(X+W*(.5+(StageCentre(B,I).X-S.Centre.X)*S.Zoom),Y+H*(.5+(StageCentre(B,I).Y-S.Centre.Y)*S.Zoom));
   if(!Inside(P,22))continue;
   FVector2D Q=P+Offsets[I];
   // Keep theatre labels on the map when a northern campaign pin is near its
   // edge. The leader still starts at the unchanged geographic pin.
   Q.X=FMath::Clamp(Q.X,double(X+18),double(X+W-203));
   Q.Y=FMath::Clamp(Q.Y,double(Y+91),double(Y+H-153));
   Segment(UI,P,Q+FVector2D(88,16),(I==S.Stage?Red:Ink*.6f)*StageAlpha,1);
   Circle(UI,P,I==S.Stage?8:5,(I==S.Stage?Red:Ink)*StageAlpha,1.6f);
   UI.Rect(Q.X,Q.Y,185,32,FLinearColor(.73f,.72f,.62f,.86f*StageAlpha));
   UI.Label(FString::Printf(TEXT("%02d  %s"),I+1,Short[I]),Q.X+7,Q.Y+3,176,24,14,(I==S.Stage?Red:Ink)*StageAlpha);
   if(StageAlpha>.5f)UI.Hit(Q.X,Q.Y,185,32,FName(*FString::Printf(TEXT("V52Stage%d"),I)),63);
  }
 }
 if(S.OperationSheet&&S.Zoom>2.55f)
 {
  const auto Node=FrontlineCampaignV52::GetNode(B,S.Preview);
  if(Node.Mission)if(auto* Texture=CampaignMapTextureV41(B,Node.Mission))
  {
   const auto Anchor=Project(S,Node.LonLat);
   const float T=FMath::SmoothStep(0.f,.82f,float(FPlatformTime::Seconds()-S.SheetStarted));
   // Dock above the mission-label band. Keep the selected geographic pin and
   // its orders readable while the same paper sheet carries into loading.
   const float CX=FMath::Lerp(float(Anchor.X-65),80.f,T),CY=FMath::Lerp(float(Anchor.Y-32),255.f,T);
   const float CW=FMath::Lerp(130.f,555.f,T),CH=FMath::Lerp(65.f,273.f,T);
   Segment(UI,Anchor,FVector2D(CX+CW*.5f,CY+CH),FLinearColor(Brass.R,Brass.G,Brass.B,.30f*T*(1.f-T)),1.f);
   OperationSheet(UI,Texture,Node.Mission->Location,CX,CY,CW,CH,T,0.f);
   UI.Hit(CX,CY,CW,CH,TEXT("V52Tab1"),69);
  }
 }
 UI.Rect(X+17,Y+15,528,57,FLinearColor(.68f,.67f,.58f,.87f));
 const TCHAR* SectorTitles[]={TEXT("ARRAS / MESSINES SECTOR"),TEXT("YPRES SECTOR"),
  TEXT("CAMBRAI SECTOR"),TEXT("SPRING 1918 SECTOR"),TEXT("HUNDRED DAYS SECTOR")};
 // Keep a legible caption throughout the approach. The short overlap avoids
 // the empty paper label visible halfway through the former disjoint fades.
 const float SectorTitleAlpha=FMath::SmoothStep(1.70f,1.90f,S.Zoom);
 UI.Text(SectorTitles[FMath::Clamp(S.Stage,0,4)],X+30,Y+25,29,
  FLinearColor(Ink.R,Ink.G,Ink.B,Ink.A*SectorTitleAlpha));
 UI.Label(Front.Label,X+20,Y+H-177,W-40,28,18,Ink);
 if(UI.Hovered(X,Y,W,H-104))UI.Label(L(TEXT("Wheel to zoom  /  Right-drag to move"),TEXT("滚轮缩放  /  右键拖动地图")),X+20,Y+H-141,W-40,24,15,Ink);
 UI.Rect(X,Y+H-103,W,103,FLinearColor(.095f,.113f,.11f,.94f));
 const TCHAR* StageEN[]={TEXT("ARRAS & MESSINES"),TEXT("THIRD YPRES"),TEXT("CAMBRAI"),TEXT("THE SPRING OFFENSIVES"),TEXT("THE HUNDRED DAYS")};
 const TCHAR* StageZH[]={TEXT("阿拉斯与梅西讷"),TEXT("第三次伊普尔"),TEXT("康布雷"),TEXT("春季攻势"),TEXT("百日攻势")};
 for(int32 I=0;I<5;++I){const float CX=X+14+I*241;
  UI.Label(I<2?TEXT("1917"):I==2?TEXT("1917–1918"):TEXT("1918"),CX+8,Y+H-84,222,19,14,Muted);
  UI.Label(L(StageEN[I],StageZH[I]),CX+8,Y+H-56,222,29,18,I==S.Stage?FLinearColor(.83f,.75f,.53f):Paper);
  UI.Line(CX+7,Y+H-13,CX+221,Y+H-13,I==S.Stage?FLinearColor(.69f,.48f,.21f):Border,I==S.Stage?3:1);
  UI.Hit(CX,Y+H-93,233,85,FName(*FString::Printf(TEXT("V52Stage%d"),I)),60);}
}
void Dossier(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FState& S)
{
 const auto N=FrontlineCampaignV52::GetNode(B,S.Preview);if(!N.Mission)return;const auto& M=*N.Mission;
 const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.PaperSheet(1301,140,571,854);
 UI.RoleLabel(EFrontlineTextRoleV8210::Plate,N.StateText,1332,159,306,26,Chinese?14.f:19.f,Red);
 UI.BilingualTitle(M.Title,1332,197,508,55,Chinese?26.f:37.f,Ink);UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(M.DateLabel),1334,267,503,26,17,Faded);UI.Line(1332,303,1840,303,Faded*.38f);
 Portrait(UI,M,0,1333,319);Portrait(UI,M,1,1596,319);
 const TCHAR* TabsEN[]={TEXT("ORDERS"),TEXT("GROUND"),TEXT("ENEMY"),TEXT("ATTACHMENTS")};const TCHAR* TabsZH[]={TEXT("命令"),TEXT("地形"),TEXT("敌情"),TEXT("配属")};
 for(int32 I=0;I<4;++I){const float TX=1332+I*127;UI.RoleLabel(EFrontlineTextRoleV8210::Plate,L(TabsEN[I],TabsZH[I]),TX,424,124,31,Chinese?16.f:22.f,I==S.Tab?Red:Faded,1);UI.Line(TX+4,465,TX+120,465,I==S.Tab?Red:Faded*.3f,I==S.Tab?2.5f:1);UI.Hit(TX,417,124,51,FName(*FString::Printf(TEXT("V52Tab%d"),I)),60);}
 if(S.Tab==0)
 {
  UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,M.HistoricalContext,1334,491,503,140,19,Ink);UI.BilingualLabel(TEXT("ORDERS"),TEXT("本部命令"),1334,633,503,28,18,Red);
  UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,N.Assignment,1334,678,503,98,17,Ink);
  UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,M.ObjectiveText,1334,785,503,66,17,Red);
 }
 else if(S.Tab==1)
 {
  FrontlineLoadingPageV82::DrawOperationMapV8447(UI,B,&M,1334,488,503,200,FLinearColor::White);
  UI.RoleLabel(EFrontlineTextRoleV8210::Text,TerrainHint(M),1334,695,503,27,15,Red);
  UI.RoleParagraph(EFrontlineTextRoleV8210::Text,M.TerrainNotes,1334,729,503,111,17,Ink);
 }
 else if(S.Tab==2)
 {
  UI.BilingualLabel(TEXT("OPPOSING FORCES"),TEXT("敌方部署"),1334,492,503,32,19,Red);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,N.Opposition,1334,551,503,225,17,Ink);
  UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,L(TEXT("This is headquarters' estimate. Reinforcements may alter the battle."),TEXT("这是司令部掌握的敌情，后续增援仍可能改变战场。")),1334,782,503,70,19,Faded);
 }
 else
 {
  UI.BilingualLabel(TEXT("ORDERS OF BATTLE"),TEXT("即将配属"),1334,492,503,32,19,Red);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,N.NewAttachments,1334,549,503,175,17,Ink);
  UI.RoleLabel(EFrontlineTextRoleV8210::FieldValue,N.BranchTitle,1334,750,503,30,17,Red);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,N.State==FrontlineCampaignV52::ENodeState::Bypassed?N.StateText:L(TEXT("Review your deployment before departure."),TEXT("出发前在出战序列中选择兵种与支援。")),1334,795,503,60,16,Faded);
 }
 const auto E=FrontlineCampaignV52::Dispatch(B,S.Preview);
 if(E.bPending&&FrontlineCampaignV52::CanStart(B,S.Preview))
 {UI.Button(1332,855,508,40,L(TEXT("DISPATCH AWAITING REPLY"),TEXT("电报待复")),TEXT(""),TEXT("V52Dispatch"),true,false,70);}
 else UI.RoleLabel(EFrontlineTextRoleV8210::Text,S.Notice.IsEmpty()?N.BranchTitle:S.Notice,1334,858,503,31,16,S.Notice.IsEmpty()?Faded:NoticeWarningV75);
 const bool Can=FrontlineCampaignV52::CanStart(B,S.Preview);
 UI.Button(1332,911,508,57,Can?L(TEXT("ISSUE ORDERS"),TEXT("下达出战命令")):N.StateText,TEXT("Enter"),TEXT("V52Begin"),Can,true,65);
 CommanderTip(UI,M);
}
#include "WarfareFrontlineAtlasV60.inl"
}
void DrawWarRoomV52(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 auto* PC=Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController());if(!PC)return;
 auto& S=WarRoom52::State(PC);WarRoom52::Synchronize(S,B);WarRoom52::AtlasDraw(UI,B,S);
}
void DrawHistoricalDispatchV52(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 const auto E=FrontlineCampaignV52::Dispatch(B);const auto* M=B->GetSelectedCampaignMission();
 DrawArchivePhotoV32(UI,FrontlineCampaignV51::PhotoForOperation(M?M->Id:0)>=0?FrontlineCampaignV51::PhotoForOperation(M->Id)+4:2,0,0,1920,1080,FLinearColor(.29f,.31f,.30f));
 UI.Rect(0,0,1920,1080,FLinearColor(0,0,0,.42f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);UI.PaperSheet(310,126,1300,824);
 UI.Label(TEXT("DISPATCHES FROM THE FRONT"),353,164,810,45,32,WarRoom52::Ink);UI.Label(E.Date,1165,169,397,33,19,WarRoom52::Faded,2);
 UI.Line(353,229,1563,229,WarRoom52::Faded*.4f);UI.Label(E.Title,356,258,1199,53,34,WarRoom52::Ink);
 UI.Paragraph(E.Body,357,336,1200,169,24,WarRoom52::Ink,true);
 for(int32 I=0;I<E.Choices.Num()&&I<2;++I)
 {const float CX=354+I*620;UI.Line(CX,551,CX+588,551,WarRoom52::Faded*.5f);UI.Label(E.Choices[I].Title,CX+3,580,575,44,27,WarRoom52::Red);
 UI.Paragraph(E.Choices[I].Effect,CX+3,652,575,120,22,WarRoom52::Ink,true);
 UI.Button(CX,816,588,65,WarRoom52::L(TEXT("SEND THE ORDER"),TEXT("签发此令")),TEXT(""),FName(*FString::Printf(TEXT("V52DispatchChoice%d"),I)),E.bPending,true,90);}
 UI.Hit(1500,132,83,51,TEXT("V52DispatchBack"),92);UI.Label(TEXT("×"),1500,132,83,51,28,WarRoom52::Ink,1);
}
