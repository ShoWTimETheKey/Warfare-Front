namespace FieldHoverV8218
{
bool Contains(const FVector2D& Mouse,float X,float Y,float W,float H)
{return Mouse.X>=X&&Mouse.X<=X+W&&Mouse.Y>=Y&&Mouse.Y<=Y+H;}
bool PanelOwnsPointer(const FFrontlineCanvas& UI,const FVector2D& Anchor)
{const auto P=TrenchControlBounds(Anchor);return Contains(UI.Mouse,P.X,P.Y,P.W,P.H);}
bool SitePoint(const FFrontlineCanvas& UI,AWarfareFrontlineController* PC,const FrontlineFieldSitesV59::FView& Site,float BattlefieldBottom,FVector2D& Point)
{
 if(Site.Kind==FrontlineFieldSitesV59::EKind::FieldWorks&&Site.Work<=0&&Site.Integrity>0)return false;
 FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(Site.Position+FVector(0,0,100),Screen))return false;
 Point=UI.Local(Screen);return Point.X>=35&&Point.X<=1885&&Point.Y>=100&&Point.Y<=BattlefieldBottom-45;
}
bool FlagPoint(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,const FrontlineFieldSitesV59::FView& Site,float BattlefieldBottom,FVector2D& Point,UTexture2D*& Flag,float& Height)
{
 FVector2D Screen;if(!Site.ShowOwnershipFlag||Site.Owner<0||Site.Owner>1||!PC->ProjectWorldLocationToScreen(Site.MarkerPosition,Screen))return false;
 Point=UI.Local(Screen)-FVector2D(0,2.5f);const int32 Nation=int32(B->GetFactionForTeam(Site.Owner));if(Nation<0||Nation>3)return false;
 Flag=Artwork().Flags[Nation].Get();if(!FrontlineTextureReady(Flag)||Point.X<=24||Point.X>=1896||Point.Y<=105||Point.Y>=BattlefieldBottom-30)return false;
 Height=22.f*Flag->GetSizeY()/FMath::Max(1,Flag->GetSizeX());return true;
}
bool HasFieldTarget(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,float BattlefieldBottom)
{
 if(!B||!PC||PC->SupportTargetMode!=0)return false;
 for(const auto& Site:FrontlineFieldSitesV59::Views(B))
 {
  FVector2D P;if(!SitePoint(UI,PC,Site,BattlefieldBottom,P))continue;FVector2D FP;UTexture2D* Flag=nullptr;float FH=0;
  if((FlagPoint(UI,B,PC,Site,BattlefieldBottom,FP,Flag,FH)&&UI.Hovered(FP.X-15,FP.Y-FH-4,30,FH+8))||UI.Hovered(P.X-58,P.Y-50,116,100))return true;
 }
 for(const auto& Repair:FrontlineFieldSitesV59::RepairViewsV60(B))
 {
  if(!Repair.Tank.IsValid()||Repair.Tank->Team!=0||(Repair.Work<=0&&!Repair.Working))continue;
  FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(Repair.Position+FVector(0,0,145),Screen))continue;
  const auto P=UI.Local(Screen);if(P.X>=85&&P.X<=1835&&P.Y>=130&&P.Y<=BattlefieldBottom-60&&UI.Hovered(P.X-105,P.Y-100,210,170))return true;
 }
 return false;
}
bool Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,float BattlefieldBottom,bool PanelClaimed)
{
 if(PanelClaimed&&UI.TooltipStateV83)
 {
  const uint32 Site=uint32(UI.TooltipStateV83->Pending>>32);
  // A passive field sheet may fade in empty battlefield space, but cannot
  // linger over the interactive trench panel after that panel owns the mouse.
  if(Site==13||Site==14)UI.TooltipStateV83->Reset();
 }
 TOptional<FrontlineFieldSitesV59::FView> HoveredSite;FVector2D HoveredPoint=FVector2D::ZeroVector;double Best=MAX_dbl;
 for(const auto& Site:FrontlineFieldSitesV59::Views(B))
 {
  FVector2D P;if(!SitePoint(UI,PC,Site,BattlefieldBottom,P))continue;FVector2D FP;UTexture2D* Flag=nullptr;float FH=0;bool FlagHovered=false;
  if(FlagPoint(UI,B,PC,Site,BattlefieldBottom,FP,Flag,FH))
  {
   UI.Image(Flag,FP.X-11,FP.Y-FH-3,22,FH,FLinearColor(.90f,.89f,.85f,1),0,0,1,1,false,true);
   FlagHovered=UI.Hovered(FP.X-15,FP.Y-FH-4,30,FH+8);
  }
  if(PanelClaimed||(!FlagHovered&&!UI.Hovered(P.X-58,P.Y-50,116,100)))continue;
  // A small ownership flag is a precise target; otherwise take the nearest
  // projected facility, not whichever happens to be first in the world list.
  const double Score=FVector2D::DistSquared(UI.Mouse,FlagHovered?FP:P)-(FlagHovered?1000000:0);
  if(Score<Best){Best=Score;HoveredSite=Site;HoveredPoint=P;}
 }
 if(HoveredSite.IsSet())
 {
  const auto& Site=HoveredSite.GetValue();const auto P=HoveredPoint;
  const FString Copy=FrontlineFieldSitesV59::Description(Site),Value=UI.DisplayText(Copy);
  // Use the same face, wrapping and leading as the tooltip paragraph. Explicit
  // effect rows and their blank separator must fit in Chinese as well as English.
  const float Em=UI.DefaultEmV8210(Value,17.f);
  const auto Rows=UI.RoleRowsV8210(EFrontlineTextRoleV8210::DarkText,Value,410.f,Em);
  float RowHeight=0.f;
  for(const auto& Row:Rows)RowHeight=FMath::Max(RowHeight,float(UI.RoleSizeRawV8210(EFrontlineTextRoleV8210::DarkText,Row,Em).Y));
  const float Leading=Em*(UI.ContainsCJKV45(Value)?1.75f:1.48f);
  const float BodyHeight=FMath::Max(142.f,FMath::CeilToFloat(FMath::Max(0,Rows.Num()-1)*Leading+RowHeight)+2.f);
  const float SheetHeight=60.f+BodyHeight+18.f;
  const float X=FMath::Clamp(float(P.X)+25.f,32.f,1430.f),Y=FMath::Clamp(float(P.Y)-SheetHeight-5.f,110.f,FMath::Max(110.f,BattlefieldBottom-SheetHeight-30.f));
  UI.CursorPriorityV67=FMath::Max(UI.CursorPriorityV67,13);
  UI.TooltipV83(TooltipKeyV83(13,HashCombineFast(uint32(Site.Kind),GetTypeHash(Site.Position))),HashCombineFast(HashCombineFast(uint32(FMath::RoundToInt(Site.Work*100)),uint32(Site.Owner+1)),HashCombineFast(uint32(Site.Ammunition),uint32(Site.RewardTaken)*2+uint32(Site.Integrity>0))),[&]{
   UI.PaperSheet(X,Y,450,SheetHeight);const FLinearColor Ink(.03f,.034f,.03f);
   UI.Label(FrontlineFieldSitesV59::Title(Site.Kind),X+20,Y+18,410,30,22,Ink);
   UI.Paragraph(Copy,X+20,Y+60,410,BodyHeight,17,Ink,true);
  },{X,Y});
  return true;
 }
 if(PanelClaimed)return false;
 for(const auto& Repair:FrontlineFieldSitesV59::RepairViewsV60(B))
 {
  if(!Repair.Tank.IsValid()||Repair.Tank->Team!=0||(Repair.Work<=0&&!Repair.Working))continue;
  FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(Repair.Position+FVector(0,0,145),Screen))continue;
  const auto P=UI.Local(Screen);if(P.X<85||P.X>1835||P.Y<130||P.Y>BattlefieldBottom-60||!UI.Hovered(P.X-105,P.Y-100,210,170))continue;
  const float X=FMath::Clamp(float(P.X)-205.f,32.f,1478.f),Y=FMath::Clamp(float(P.Y)-222.f,115.f,BattlefieldBottom-270.f);
  UI.TooltipV83(TooltipKeyV83(14,HashCombineFast(Repair.Tank->GetUniqueID(),Repair.Side)),uint64(FMath::RoundToInt(Repair.Work*100)),[&]{
   UI.PaperSheet(X,Y,410,250);const FLinearColor Ink(.035f,.04f,.035f);
   UI.Label(Repair.Side>=2?WarRoom52::L(TEXT("GUN REPAIR"),TEXT("抢修机枪")):WarRoom52::L(TEXT("TRACK REPAIR"),TEXT("抢修履带")),X+18,Y+14,374,30,21,Ink);
   UI.Paragraph(FrontlineFieldSitesV59::RepairDescriptionV60(Repair),X+18,Y+58,374,161,16,Ink,true);
   UI.Line(X+19,Y+232,X+390,Y+232,Ink*.22f,2);
   UI.Line(X+19,Y+232,X+19+371*FMath::Clamp(Repair.Work,0.f,1.f),Y+232,WarRoom52::Red,3);
  },{X,Y});return true;
 }
 return false;
}
}
