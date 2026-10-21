// Included in WarRoom52. One projected paper surface serves drawing and picking.
// Operation sheets are a semantic change of scale, never a 72x atlas crop.
struct FAtlasSector60
{
 const TCHAR* Name;const TCHAR* Chinese;FVector2D Centre,Caption;
 TArray<FVector2D> Boundary;const TCHAR* Account;const TCHAR* AccountZH;
};
const TArray<FAtlasSector60>& AtlasSectors()
{
 static const TArray<FAtlasSector60> S={
  {TEXT("FLANDERS"),TEXT("佛兰德斯"),{2.91,50.80},{394,225},{{2.15,51.30},{3.65,51.30},{3.55,50.54},{2.25,50.54}},TEXT("Ypres, Messines and the Lys. Low ridges overlook a plain cut by flooded drains."),TEXT("伊普尔、梅西讷与利斯河。低矮山脊俯瞰平原，漫水的排水沟横断接近线。")},
  {TEXT("ARTOIS"),TEXT("阿图瓦"),{2.86,50.29},{315,346},{{2.25,50.54},{3.55,50.54},{3.20,50.24},{3.02,50.08},{2.23,50.08}},TEXT("Arras and the Scarpe. Chalk roads lead from ruined villages to belts of wire and dugouts."),TEXT("阿拉斯与斯卡普河。白垩道路穿过残破村庄，通向铁丝网和深掩蔽部。")},
  {TEXT("THE SOMME"),TEXT("索姆河"),{2.64,49.88},{300,475},{{1.65,50.08},{3.02,50.08},{3.15,49.65},{1.70,49.60}},TEXT("Amiens and the approaches to Saint-Quentin. Open farmland gives tanks room to move, and guns a clear field of fire."),TEXT("亚眠与圣康坦接近地。开阔农田便于坦克展开，也给守军留下宽阔射界。")},
  {TEXT("CAMBRAI"),TEXT("康布雷"),{3.21,50.09},{1060,362},{{3.02,50.08},{3.20,50.24},{3.42,50.22},{3.40,49.72},{3.15,49.65}},TEXT("The Hindenburg positions. Canal cuts, sunken roads and Bourlon Wood divide the armoured advance."),TEXT("兴登堡防线。运河切口、下凹道路与布尔隆林地将装甲推进分割成数条通路。")},
  {TEXT("SAMBRE & SELLE"),TEXT("桑布尔河与塞勒河"),{3.55,50.20},{1180,245},{{3.55,50.54},{4.28,50.56},{4.30,49.78},{3.40,49.72},{3.42,50.22},{3.20,50.24}},TEXT("The last crossings. Railway embankments and river towns bar the road into the German rear."),TEXT("最后的渡越。铁路路堤与河畔城镇挡在通往德军后方的道路上。")},
  {TEXT("CHAMPAGNE"),TEXT("香槟"),{4.10,49.20},{1060,652},{{3.15,49.65},{4.30,49.78},{4.85,49.40},{4.85,48.75},{3.20,48.75}},TEXT("The French armies hold the front east of Reims. This sector is outside the company's present orders."),TEXT("法军守卫兰斯以东的战线。本连当前没有在这一战区的作战命令。")},
  {TEXT("VERDUN"),TEXT("凡尔登"),{5.43,49.19},{1440,548},{{4.85,49.40},{5.80,49.65},{6.12,49.20},{5.85,48.73},{4.85,48.75}},TEXT("Forts, wooded heights and the Meuse. French formations hold the approaches to Verdun; no operation here is assigned to this company."),TEXT("要塞、林地高地与默兹河。法军扼守凡尔登接近地；本连未获派在此作战。")},
  {TEXT("LORRAINE & VOSGES"),TEXT("洛林与孚日"),{6.74,48.52},{1485,793},{{5.80,49.65},{7.65,49.22},{7.90,47.65},{6.20,47.45},{5.85,48.73},{6.12,49.20}},TEXT("The line runs south through forest and mountain passes. Headquarters' map records the front; no orders for this company have arrived from these sectors."),TEXT("战线向南穿入森林与山口。这张总图标示其位置；这些战区尚未下达本连的作战命令。")}
 };
 return S;
}
int32 AtlasSectorFor(int32 Mission)
{
 if(Mission==3||Mission==4||Mission==5||Mission==6||Mission==7||Mission==12)return 0;
 if(Mission==0||Mission==1||Mission==2||Mission==15)return 1;
 if(Mission==11||Mission==13||Mission==14)return 2;
 if(Mission==8||Mission==9||Mission==10||Mission==16)return 3;
 return 4;
}
bool AtlasContains(const TArray<FVector2D>& P,FVector2D Q)
{
 bool Inside=false;for(int32 I=0,J=P.Num()-1;I<P.Num();J=I++)
 if((P[I].Y>Q.Y)!=(P[J].Y>Q.Y)&&Q.X<(P[J].X-P[I].X)*(Q.Y-P[I].Y)/(P[J].Y-P[I].Y)+P[I].X)Inside=!Inside;
 return Inside;
}
FVector2D AtlasWarp(const FState& S,FVector2D D)
{
 const double Depth=1.+S.Tilt*D.Y/1346.;D=FVector2D(D.X/Depth,D.Y*(1.-.30*S.Tilt)/Depth);
 const double C=FMath::Cos(double(S.Roll)),R=FMath::Sin(double(S.Roll));
 return FVector2D(960+D.X*C-D.Y*R,553+D.X*R+D.Y*C);
}
FVector2D AtlasPoint(const FState& S,FVector2D Geo)
{if(auto* PC=Cast<AWarfareFrontlineController>(S.Owner.Get());FrontlineMapSceneV61::IsActive(PC))return FrontlineMapSceneV61::Project(PC,Geo);
 return AtlasWarp(S,FVector2D((Geo.X-S.Centre.X)*(1920.*S.Zoom),(Geo.Y-S.Centre.Y)*(1346.*S.Zoom)));}
FVector2D AtlasInverse(const FState& S,FVector2D Pixel)
{
 if(auto* PC=Cast<AWarfareFrontlineController>(S.Owner.Get());FrontlineMapSceneV61::IsActive(PC))return FrontlineMapSceneV61::Unproject(PC,Pixel);
 const auto D=Pixel-FVector2D(960,553);const double C=FMath::Cos(double(S.Roll)),R=FMath::Sin(double(S.Roll));
 const double PlaneX=D.X*C+D.Y*R,PlaneY=-D.X*R+D.Y*C;
 const double OriginalY=PlaneY/(1.-.30*S.Tilt-PlaneY*S.Tilt/1346.);
 return S.Centre+FVector2D(PlaneX*(1.+S.Tilt*OriginalY/1346.)/(1920.*S.Zoom),OriginalY/(1346.*S.Zoom));
}
int32 AtlasSectorAtPointV61(const FState& S,FVector2D Pixel)
{
 if(S.AtlasMode!=0||S.FlightDuration>0||Pixel.Y<=132||Pixel.Y>=996)return INDEX_NONE;
 for(int32 I=0;I<AtlasSectors().Num();++I)
 {
  TArray<FVector2D> Polygon;
  for(const auto& Geo:AtlasSectors()[I].Boundary)Polygon.Add(AtlasPoint(S,UV(Geo)));
  if(AtlasContains(Polygon,Pixel))return I;
 }
 return INDEX_NONE;
}
void AtlasChooseSector(FState& S,const AWarfareFrontlineGameMode* B,int32 Sector)
{
 if(!AtlasSectors().IsValidIndex(Sector))return;
 S.AtlasSector=Sector;S.AtlasMode=1;S.OperationSheet=false;S.FromDetail=S.Detail;S.ToDetail=0;S.Notice.Empty();
 int32 Best=-1,Preferred=S.Preview;
 for(const auto& N:FrontlineCampaignV52::Nodes(B))if(AtlasSectorFor(N.Id)==Sector)
 {
  const int32 Score=N.State==FrontlineCampaignV52::ENodeState::Selected?4:N.State==FrontlineCampaignV52::ENodeState::Available?3:N.State==FrontlineCampaignV52::ENodeState::Completed?2:1;
  if(Score>Best){Best=Score;Preferred=N.Id;}
 }
 if(Best>=0){S.Preview=Preferred;S.Stage=Stage(Preferred);}
 FrontlineMapSceneV61::PrefetchV75(Cast<AWarfareFrontlineController>(S.Owner.Get()),Sector<5?FrontlineMapSceneV61::SectorFocus(Sector):UV(AtlasSectors()[Sector].Centre),Sector<5?8.f:2.7f);
 Flight(S,Sector<5?8.0f:2.7f,Sector<5?FrontlineMapSceneV61::SectorFocus(Sector):UV(AtlasSectors()[Sector].Centre),2.8f,11);
}
void AtlasTick(FState& S,double Now)
{
 if(S.FlightDuration<=0.)return;
 const float T=FMath::Clamp(float((Now-S.FlightStart)/S.FlightDuration),0.f,1.f);
 const auto Ease=[](float V){V=FMath::Clamp(V,0.f,1.f);return V*V*V*(V*(V*6.f-15.f)+10.f);};
 // Find the geographic destination before the close descent. This keeps the
 // operation sheet in frame even when CURRENT ORDERS starts at the full atlas.
 const bool Approaching=S.FlightToZoom>S.FlightFromZoom;
 const float Pan=Ease(Approaching?T/.68f:T),Push=Ease(Approaching?(T-.13f)/.87f:T);
 S.Zoom=S.FlightFromZoom*FMath::Pow(S.FlightToZoom/S.FlightFromZoom,Push);
 S.Centre=FMath::Lerp(S.FlightFromCentre,S.FlightToCentre,Pan);
 S.Roll=0;S.Tilt=0;
 S.Detail=FMath::Lerp(S.FromDetail,S.ToDetail,FMath::SmoothStep(Approaching?.44f:.12f,Approaching?.90f:.50f,T));
 Clamp(S);
 if(T>=1.f){StopFlight(S);S.Roll=S.Tilt=0;S.Detail=S.ToDetail;S.OperationSheet=S.AtlasMode==2;S.SheetStarted=Now;}
}
void AtlasTexture(const FFrontlineCanvas& UI,UTexture2D* T,TFunctionRef<FVector2D(FVector2D)> Point,
 FVector2D UV0,FVector2D UV1,FLinearColor Colour)
{
 if(!FrontlineTextureReady(T)||Colour.A<=.001f)return;
 constexpr int32 NX=20,NY=14;TArray<FCanvasUVTri> Tris;Tris.Reserve(NX*NY*2);
 for(int32 Row=0;Row<NY;++Row)for(int32 Col=0;Col<NX;++Col)
 {
  const FVector2D A(double(Col)/NX,double(Row)/NY),B(double(Col+1)/NX,double(Row)/NY),C(double(Col+1)/NX,double(Row+1)/NY),D(double(Col)/NX,double(Row+1)/NY);
  for(int32 K=0;K<2;++K){auto& Q=Tris.AddDefaulted_GetRef();const auto P1=K?C:B,P2=K?D:C;
   Q.V0_Pos=UI.Screen(Point(A).X,Point(A).Y);Q.V1_Pos=UI.Screen(Point(P1).X,Point(P1).Y);Q.V2_Pos=UI.Screen(Point(P2).X,Point(P2).Y);
   Q.V0_UV=UV0+(UV1-UV0)*A;Q.V1_UV=UV0+(UV1-UV0)*P1;Q.V2_UV=UV0+(UV1-UV0)*P2;
   Q.V0_Color=Q.V1_Color=Q.V2_Color=Colour;}
 }
 FCanvasTriangleItem Item(Tris,T->GetResource());Item.BlendMode=SE_BLEND_Translucent;UI.Surface->DrawItem(Item);
}
void AtlasBackdrop(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FState& S)
{
    TGuardValue<bool> MapTypographyScopeV8210(UI.bLegacyTypographyV8210,true);
 if(FrontlineMapSceneV61::IsActive(Cast<AWarfareFrontlineController>(S.Owner.Get())))return;
 UI.Image(Map(),0,0,1920,1080,FLinearColor(.83f,.82f,.75f,1));
}

void AtlasDraw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FState& S)
{
    TGuardValue<bool> MapTypographyScopeV8210(UI.bLegacyTypographyV8210,true);
 AtlasBackdrop(UI,B,S);const bool Moving=S.FlightDuration>0.;
 FrontlineMapSceneV61::SetHoverV64(Cast<AWarfareFrontlineController>(S.Owner.Get()),INDEX_NONE,INDEX_NONE,FVector2D::ZeroVector,FVector2D::ZeroVector);
 const float Arrived=Moving?0:FMath::SmoothStep(0.f,.44f,float(FPlatformTime::Seconds()-S.SheetStarted));
 const FLinearColor Light(.87f,.85f,.74f),Dark(.007f,.012f,.010f),Rust(.35f,.08f,.044f);
 UI.Hit(0,132,1920,864,TEXT("V52MapSurface"),44);
 if(S.Detail<.45f)
 {
  const float A=1-FMath::SmoothStep(.08f,.45f,S.Detail);
  const auto Front=FrontlineCampaignV52::Frontline(B,S.Preview);
  if(S.AtlasMode==0&&!Moving)
  {
   for(int32 I=0;I<AtlasSectors().Num();++I)
   {
    const auto& Sec=AtlasSectors()[I];TArray<FVector2D> Polygon;
    for(const auto& G:Sec.Boundary)Polygon.Add(AtlasPoint(S,UV(G)));
    const bool Over=UI.Mouse.Y>132&&UI.Mouse.Y<996&&AtlasContains(Polygon,UI.Mouse);
    const auto C=AtlasPoint(S,UV(Sec.Centre));
    if(Over)
    {
        FVector2D Lo(1,1),Hi(0,0);for(const auto& G:Sec.Boundary){const auto U=UV(G);Lo.X=FMath::Min(Lo.X,U.X);Lo.Y=FMath::Min(Lo.Y,U.Y);Hi.X=FMath::Max(Hi.X,U.X);Hi.Y=FMath::Max(Hi.Y,U.Y);}
        FrontlineMapSceneV61::SetHoverV64(Cast<AWarfareFrontlineController>(S.Owner.Get()),I,INDEX_NONE,(Lo+Hi)*.5,(Hi-Lo)*.58);
    }
    if(Over){UI.Hit(UI.Mouse.X-4,UI.Mouse.Y-4,8,8,FName(*FString::Printf(TEXT("V60Sector%d"),I)),65);
      UI.Gradient(40,809,750,160,FLinearColor(.015f,.022f,.024f,.92f),FLinearColor(.015f,.022f,.024f,0));
      UI.Label(L(Sec.Name,Sec.Chinese),64,829,685,31,24,Light);
      UI.Paragraph(L(Sec.Account,Sec.AccountZH),64,875,685,80,19,Light,true);}
    // The whole geographic area is interactive. No floating leader-line
    // labels, coloured polygons or HUD circles are glued onto the paper.
   }
  }
 }
 if(!Moving&&S.AtlasMode==1&&AtlasSectors().IsValidIndex(S.AtlasSector))
 {
  const auto& Sector=AtlasSectors()[S.AtlasSector];UI.PaperSheet(1301,140,571,854);
  UI.Label(L(Sector.Name,Sector.Chinese),1330,168,510,44,31,Ink);UI.Paragraph(L(Sector.Account,Sector.AccountZH),1333,230,505,95,19,Ink,true);
  int32 Row=0;TArray<FVector2D> Used;
  for(const auto& N:FrontlineCampaignV52::Nodes(B))if(AtlasSectorFor(N.Id)==S.AtlasSector)
  {
   const float CY=347+Row*98;const bool Available=FrontlineCampaignV52::CanStart(B,N.Id);
   const auto Name=FName(*FString::Printf(TEXT("V52Node%d"),N.Id));
   UI.Line(1331,CY-8,1838,CY-8,Faded*.25f);
   UI.Label(N.Mission->DateLabel,1335,CY,496,20,14,Faded);
   UI.Label(N.Mission->Title,1335,CY+26,496,32,23,Available?Red:Ink);
   UI.Label(N.StateText,1335,CY+65,496,18,13,Available?Red:Faded);UI.Hit(1322,CY-4,529,94,Name,68);
   const auto Pin=AtlasPoint(S,UV(N.LonLat));
   if(Pin.X>35&&Pin.X<1270&&Pin.Y>150&&Pin.Y<982)
   {UI.Hit(Pin.X-25,Pin.Y-30,50,55,Name,65);if(UI.Hovered(Pin.X-25,Pin.Y-30,50,55))FrontlineMapSceneV61::SetHoverV64(Cast<AWarfareFrontlineController>(S.Owner.Get()),INDEX_NONE,N.Id,FVector2D::ZeroVector,FVector2D::ZeroVector);}
   ++Row;
  }
  if(Row==0){UI.Line(1333,363,1838,363,Faded*.4f);UI.Label(L(TEXT("NO ORDERS IN THIS SECTOR"),TEXT("本战区暂无作战命令")),1333,389,505,40,23,Red);}
 }
 if(S.AtlasMode==2&&S.OperationSheet&&Arrived>0)
 {
  const auto Node=FrontlineCampaignV52::GetNode(B,S.Preview);
  if(Arrived<.995f){FFrontlineCanvas Dock=UI;const float Offset=(1-Arrived)*595;Dock.OX+=Offset*UI.Scale;Dock.Mouse.X-=Offset;Dock.bSuppressBackgroundHits=true;Dossier(Dock,B,S);UI.NextHitId=Dock.NextHitId;}
  else Dossier(UI,B,S);
  const auto N=FrontlineCampaignV52::GetNode(B,S.Preview);
  if(N.Mission){UI.Label(N.Mission->Location,64,854,1125,60,43,Light);UI.Label(N.Mission->DateLabel,68,928,1020,26,18,Light);}
 }
 // Solid peripheral rails mask paper outside the theatrical aperture.
 UI.Gradient(0,0,1920,155,FLinearColor(.008f,.014f,.016f,.91f),FLinearColor(.008f,.014f,.016f,0),true);
 UI.Skin(0,0,996,1920,84,10,FLinearColor(.71f,.75f,.68f,1));
 UI.Hit(0,0,1920,132,TEXT("Blocked"),74);UI.Hit(0,996,1920,84,TEXT("Blocked"),74);
 UI.Label(B->GetPlayerFaction()==EFrontlineFaction::German?L(TEXT("OBERSTE HEERESLEITUNG"),TEXT("最高陆军统帅部")):L(TEXT("GENERAL HEADQUARTERS"),TEXT("远征军总司令部")),48,24,1230,49,34,Light);
 {TGuardValue<bool> Resolved(UI.bResolvedTextV8210,true);const bool English=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English;UI.Label((English?FString(TEXT("DIFFICULTY  ")):FString(TEXT("难度  ")))+(English?FrontlineLocalizationV35::English(FrontlineDifficultyV30::Label(B->GetCampaignDifficulty())):FrontlineLocalizationV35::Text(FrontlineDifficultyV30::Label(B->GetCampaignDifficulty()))),1321,47,530,35,20,Light,2);}
 if(!Moving&&S.AtlasMode==0)
 {
  UI.Label(FrontlineCampaignV52::Frontline(B,S.Preview).Label,70,955,1060,22,16,Light);
  
 }
 UI.Button(48,1018,180,40,L(TEXT("ARCHIVES"),TEXT("战役档案")),TEXT(""),TEXT("CampaignArchives"),true,false,75);
 UI.Button(242,1018,177,40,L(TEXT("MAIN MENU"),TEXT("主菜单")),TEXT("Esc"),TEXT("MenuHome"),true,false,75);
 UI.Button(440,1018,238,40,L(TEXT("OVERVIEW"),TEXT("总览")),TEXT(""),TEXT("V52Overview"),true,false,75);
 UI.Button(692,1018,247,40,L(TEXT("CURRENT ORDERS"),TEXT("当前行动")),TEXT(""),TEXT("V60Current"),true,false,75);
 const bool ArchiveAvailable=FrontlineUltimateArchiveV8215::Available(B->GetPlayerFaction());
 UI.Button(953,1018,278,40,L(TEXT("ULTIMATE ARCHIVE"),TEXT("终极档案室")),TEXT(""),TEXT("V8215ArchiveOpen"),ArchiveAvailable,false,75);
 UI.Button(1301,1018,278,40,L(TEXT("RESEARCH"),TEXT("研究")),TEXT("T"),TEXT("Technology"),!Moving,false,75);
 UI.Button(1593,1018,279,40,L(TEXT("ORDER OF BATTLE"),TEXT("出战序列")),TEXT(""),TEXT("V52Deployment"),!Moving&&S.AtlasSector<5&&FrontlineCampaignV52::CanStart(B,S.Preview),false,75);
 UI.Line(430,1028,430,1048,FLinearColor(.52f,.51f,.43f,.38f));UI.Line(1265,1028,1265,1048,FLinearColor(.52f,.51f,.43f,.38f));
 // The army's honours belong below its headquarters heading. Draw the rack
 // last so its existing hover details remain above the map and paper rails.
 DrawFactionRibbonV8215(UI,B,B->GetPlayerFaction(),52,79,240,42);
 if(!ArchiveAvailable&&UI.Hovered(953,1018,278,40))
  UI.PageTooltipV8222(TooltipKeyV83(24,uint32(B->GetPlayerFaction())),0,[&]{
   UI.PaperSheet(936,884,450,112);
   UI.RoleParagraph(EFrontlineTextRoleV8210::Text,L(TEXT("Complete this army's campaign to open its archive."),TEXT("完成该阵营战役后开放。")),960,906,402,68,20,Ink);
  });
}
