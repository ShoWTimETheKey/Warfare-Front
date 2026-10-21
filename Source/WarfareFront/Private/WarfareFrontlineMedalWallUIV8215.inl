#include "WarfareFrontlineMedalRibbonV8446.h"
#include "WarfareFrontlineMedalPlacementV84415.h"
namespace MedalWallV8215
{
// UI-only entries keep the original service record independent of the real
// decoration catalogue and its historical award/save rules.
constexpr int32 DeedBaseV8222=100;
bool IsDeed(int32 I){return I>=DeedBaseV8222&&I<DeedBaseV8222+4;}
EFRRUNationV81 DeedNation(int32 I){return IsDeed(I)?EFRRUNationV81(I-DeedBaseV8222):EFRRUNationV81::Invalid;}
FrontlineArmourValourV8221::FDisplay Deed(int32 I)
{const auto N=DeedNation(I);return FrontlineArmourValourV8221::Display(N,FrontlineArmourKillTallyV8220::Count(N));}
TArray<int32> Items(EFRRUNationV81 N)
{auto R=FrontlineMedalsV8215::NationIndices(N);if(int32(N)>=0&&int32(N)<4)R.Add(DeedBaseV8222+int32(N));return R;}
int32 EntryTier(int32 I){return IsDeed(I)?Deed(I).Distinctions:FrontlineMedalsV8215::Tier(I);}
int32 EntryVariants(int32 I){return IsDeed(I)?(DeedNation(I)==EFRRUNationV81::German?2:4):FrontlineMedalsV8215::InspectionStatesV84413(I).Num();}
FrontlineMedalsV8215::FInspectionStateV84413 InspectionStateV84413(int32 I,int32 Position)
{
 if(IsDeed(I)){FrontlineMedalsV8215::FInspectionStateV84413 R;R.Form.Grade=uint8(FMath::Clamp(Position,1,EntryVariants(I)));R.Earned=Deed(I).Available&&R.Form.Grade<=EntryTier(I);return R;}
 const auto States=FrontlineMedalsV8215::InspectionStatesV84413(I);
 return States.IsEmpty()?FrontlineMedalsV8215::FInspectionStateV84413{}:States[FMath::Clamp(Position-1,0,States.Num()-1)];
}
int32 InspectionPositionV84413(int32 I,int32 Grade=0)
{
 if(IsDeed(I))return FMath::Clamp(Grade>0?Grade:FMath::Max(1,EntryTier(I)),1,EntryVariants(I));
 const auto A=FrontlineMedalsV8215::Award(I);const int32 G=Grade>0?Grade:FMath::Max(1,int32(A.Grade));
 const auto States=FrontlineMedalsV8215::InspectionStatesV84413(I);int32 First=INDEX_NONE;
 for(int32 K=0;K<States.Num();++K)if(States[K].Form.Grade==G)
 {if(First==INDEX_NONE)First=K+1;if(G==A.Grade&&States[K].Form.Repeats==A.Repeats&&States[K].Form.Attachments==A.Attachments)return K+1;}
 return First==INDEX_NONE?1:First;
}
FString Name(int32 I,bool ZH){return IsDeed(I)?FrontlineArmourValourV8221::BannerTitle(ZH):FString(ZH?FrontlineMedalsV8215::Definition(I).NameZH:FrontlineMedalsV8215::Definition(I).NameEN);}
void NameLabel(const FFrontlineCanvas& UI,int32 I,bool ZH,float X,float Y,float W,float H,float Em,const FLinearColor& Ink,int32 Align=1)
{
 const FString Full=Name(I,ZH);using Role=EFrontlineTextRoleV8210;
 if(!IsDeed(I)||UI.RoleWidth(Role::Headline,Full,Em)<=W)
 {UI.RoleLabel(Role::Headline,Full,X,Y,W,H,Em,Ink,Align);return;}
 // A deliberate title break preserves its wording; the long service title
 // never relies on the single-line renderer's minimum-size floor.
 const FString First=ZH?TEXT("英勇行为："):TEXT("ACT OF VALOUR:");
 const FString Second=ZH?TEXT("步兵击毁装甲"):TEXT("INFANTRY ARMOUR KILL");
 const float LineH=FMath::Min(H*.5f,Em*1.42f),Top=Y+(H-LineH*2.f)*.5f;
 UI.RoleLabel(Role::Headline,First,X,Top,W,LineH,Em,Ink,Align);
 UI.RoleLabel(Role::Headline,Second,X,Top+LineH,W,LineH,Em,Ink,Align);
}
FString History(int32 I,bool ZH)
{
 if(!IsDeed(I))return ZH?FrontlineMedalsV8215::Definition(I).HistoryZH:FrontlineMedalsV8215::Definition(I).HistoryEN;
 return ZH?TEXT("步兵击毁敌军装甲的战绩，记入本军荣誉簿。"):
  TEXT("Infantry victories over enemy armour are entered in the army's roll of honour.");
}
FString Rule(int32 I,bool ZH)
{
 if(!IsDeed(I))return ZH?FrontlineMedalsV8215::Definition(I).RuleZH:FrontlineMedalsV8215::Definition(I).RuleEN;
 const auto D=Deed(I);if(!D.Available)return FrontlineArmourValourV8221::Description(D,ZH);
 FString Text=ZH?FString::Printf(TEXT("每次步兵击毁一辆敌方装甲，记一笔战绩。\n累计击毁：%d 辆"),D.Total):
  FString::Printf(TEXT("Each enemy armoured vehicle destroyed by infantry records one deed.\nConfirmed armour kills: %d"),D.Total);
 if(D.NextAt>0)Text+=ZH?FString::Printf(TEXT("\n下次嘉奖：%d 辆"),D.NextAt):
  (D.NextAt==1?FString::Printf(TEXT("\nNext distinction: %d vehicle"),D.NextAt):FString::Printf(TEXT("\nNext distinction: %d vehicles"),D.NextAt));
 return Text;
}
FString AwardLabel(int32 I,bool ZH)
{return IsDeed(I)?FrontlineArmourValourV8221::RibbonLabel(Deed(I),ZH):FrontlineMedalsV8215::AwardLabel(I,ZH);}
FString VariantLabel(int32 I,int32 G,bool ZH)
{
 if(!IsDeed(I)){const auto V=FrontlineMedalsV8215::Variants(I);return V.IsValidIndex(G-1)?FString(ZH?V[G-1].NameZH:V[G-1].NameEN):FString();}
 const int32 Stage=FMath::Clamp(G,1,EntryVariants(I))-1;
 static constexpr int32 Gates[]={1,5,15,40};const auto N=DeedNation(I);
 if(N==EFRRUNationV81::French){static const TCHAR* EN[]={TEXT("BRONZE STAR"),TEXT("SILVER STAR"),TEXT("SILVER-GILT STAR"),TEXT("BRONZE PALM")};static const TCHAR* CN[]={TEXT("铜星嘉奖"),TEXT("银星嘉奖"),TEXT("镀金银星嘉奖"),TEXT("铜棕榈枝嘉奖")};return ZH?CN[Stage]:EN[Stage];}
 if(N==EFRRUNationV81::Russian){static const TCHAR* EN[]={TEXT("SILVER"),TEXT("SILVER AND BOW"),TEXT("GOLD"),TEXT("GOLD AND BOW")};static const TCHAR* CN[]={TEXT("银质嘉奖"),TEXT("银质与勋带结"),TEXT("金质嘉奖"),TEXT("金质与勋带结")};return ZH?CN[Stage]:EN[Stage];}
 return FrontlineArmourValourV8221::RibbonLabel(FrontlineArmourValourV8221::Display(N,Gates[Stage]),ZH);
}
FString Progress(int32 I,bool ZH)
{if(!IsDeed(I))return FrontlineMedalsV8215::Progress(I,ZH);const auto D=Deed(I);return D.Available?(ZH?FString::Printf(TEXT("步兵击毁装甲 · %d 笔战绩"),D.Total):FString::Printf(TEXT("INFANTRY ARMOUR KILLS · %d CONFIRMED"),D.Total)):AwardLabel(I,ZH);}
TArray<int32> OrderedRibbonItems(EFRRUNationV81 N)
{
 auto R=FrontlineMedalsV8215::Ribbons(N);const auto D=FrontlineArmourValourV8221::Display(N,FrontlineArmourKillTallyV8220::Count(N));
 // A gameplay service marker follows all actual national decorations.
 if(D.Available&&D.Distinctions>0)R.Add(DeedBaseV8222+int32(N));return R;
}
struct FScrollV842
{
 float Offset=0,Target=0,Velocity=0;double Last=0;bool Ready=false;
 void Reset(float Value=0){Offset=Target=Value;Velocity=0;Last=0;Ready=true;}
 void Advance(double Now,float Maximum,bool Stable=false)
 {
  Maximum=FMath::Max(0.f,Maximum);Target=FMath::Clamp(Target,0.f,Maximum);
  const float DT=Last>0?FMath::Clamp(float(Now-Last),0.f,.05f):0.f;Last=Now;
  if(Stable){Offset=Target;Velocity=0;return;}
  // Exact critically damped spring: frame-rate independent, fast initial
  // response, no oscillation or rubber-band overscroll at either boundary.
  constexpr float Omega=19.f;const float D=Offset-Target,B=Velocity+Omega*D,E=FMath::Exp(-Omega*DT);
  Offset=Target+(D+B*DT)*E;Velocity=(Velocity-Omega*B*DT)*E;
  if(Offset<0||Offset>Maximum){Offset=FMath::Clamp(Offset,0.f,Maximum);Velocity=0;}
  if(FMath::Abs(Offset-Target)<.025f&&FMath::Abs(Velocity)<.5f){Offset=Target;Velocity=0;}
 }
};
struct FView{bool Wall=false,Inspect=false;int32 Nation=0,Focus=0,Row=0,ExhibitRowV8311=0,PreviewGrade=1,PreviewStateV84413=0;FScrollV842 Catalogue,Gallery;int32 ScrollNation=INDEX_NONE;};
TMap<TWeakObjectPtr<APlayerController>,FView> Views;
FView& View(APlayerController* P){return Views.FindOrAdd(P);}
void SelectInspectionPositionV84413(FView& S,int32 Position)
{S.PreviewStateV84413=FMath::Clamp(Position,1,FMath::Max(1,EntryVariants(S.Focus)));S.PreviewGrade=InspectionStateV84413(S.Focus,S.PreviewStateV84413).Form.Grade;}
bool SelectInspectionStateV84413(APlayerController* P,int32 I,int32 Grade,int32 Repeats=0,uint16 Devices=0)
{
 if(!P||IsDeed(I)||I<0||I>=FrontlineMedalsV8215::Count||I==4)return false;
 if(FrontlineMedalsV8215::RepeatLimitV85(I)>0)Repeats=FrontlineMedalsV8215::PresentedRepeatsV85(I,Repeats);
 const auto States=FrontlineMedalsV8215::InspectionStatesV84413(I);
 for(int32 K=0;K<States.Num();++K)if(States[K].Form.Grade==Grade&&States[K].Form.Repeats==Repeats&&States[K].Form.Attachments==Devices)
 {auto& S=View(P);S.Wall=true;S.Nation=int32(FrontlineMedalsV8215::Definition(I).Nation);S.Focus=I;SelectInspectionPositionV84413(S,K+1);return true;}
 return false;
}
bool Selected(APlayerController* P){return View(P).Wall;}
TArray<FIntPoint> Exhibits(EFRRUNationV81 Nation);
namespace ExhibitLayoutV8311
{
constexpr int32 Columns=6;
// Preserve the 3:1 frame. Its inner aperture is y307..589: the rack starts
// 18px below the top, then the fixed specimens leave 20px / 18px around them.
constexpr float X=138.5f,Top=390,SlotW=154,SlotH=190,TrackX=1083;
constexpr float RackTop=325,RackH=54,ArtW=120,ArtH=172;
constexpr float ArtX=(SlotW-ArtW)*.5f,ArtY=(SlotH-ArtH)*.5f;
int32 Maximum(int32 Count){return FMath::Max(0,(Count+Columns-1)/Columns-1);}
FBox2D Cell(int32 Index,int32 Row,int32 Count=6)
{
 const int32 OnRow=FMath::Clamp(Count-Row*Columns,1,Columns);
 const float CX=X+(Columns-OnRow)*SlotW*.5f+(Index%Columns)*SlotW,CY=Top+(Index/Columns-Row)*SlotH;
 return FBox2D(FVector2D(CX,CY),FVector2D(CX+SlotW,CY+SlotH));
}
}
void RunExhibitChecksV8311(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Bounded=ExhibitLayoutV8311::RackTop>=307.f+18.f,Fixed=true,ExactlyOnce=true,Centred=true;
 for(int32 Count=1;Count<=100;++Count)
 {
  TSet<int32> Seen;
  for(int32 Row=0;Row<=ExhibitLayoutV8311::Maximum(Count);++Row)
  {
   int32 Visible=0;
   for(int32 I=0;I<Count;++I)if(I/ExhibitLayoutV8311::Columns==Row)
   {
    const auto C=ExhibitLayoutV8311::Cell(I,Row,Count);++Visible;Seen.Add(I);
    const float ArtTop=C.Min.Y+ExhibitLayoutV8311::ArtY,ArtBottom=ArtTop+ExhibitLayoutV8311::ArtH;
    Bounded&=C.Min.X>=126&&C.Max.X<=1075&&C.Min.Y>=390&&C.Max.Y<=580
     &&ArtTop>=ExhibitLayoutV8311::RackTop+ExhibitLayoutV8311::RackH+20.f&&ArtBottom<=589.f-18.f;
    Fixed&=FMath::IsNearlyEqual(float(C.GetSize().X),154.f)&&FMath::IsNearlyEqual(float(C.GetSize().Y),190.f)
     &&FMath::IsNearlyEqual(ExhibitLayoutV8311::ArtW,120.f)&&FMath::IsNearlyEqual(ExhibitLayoutV8311::ArtH,172.f);
   }
   Bounded&=Visible>0&&Visible<=6;
   const auto First=ExhibitLayoutV8311::Cell(Row*6,Row,Count),Last=ExhibitLayoutV8311::Cell(Row*6+Visible-1,Row,Count);
   Centred&=FMath::IsNearlyEqual(float((First.Min.X+Last.Max.X)*.5),600.5f,.001f);
  }
  ExactlyOnce&=Seen.Num()==Count;
 }
 Check(TEXT("v8311_medal_exhibits_at_most_six_whole_items_inside_aperture"),Bounded);
 Check(TEXT("v8311_medal_exhibit_sizes_do_not_shrink_as_collection_grows"),Fixed);
 Check(TEXT("v8311_medal_exhibit_scrolling_reaches_each_real_item_once"),ExactlyOnce);
 Check(TEXT("v8311_medal_exhibit_short_rows_stay_centred"),Centred);
}
bool Wheel(APlayerController* P,int32 Direction)
{
 auto& S=View(P);if(!S.Wall)return false;
 if(S.Inspect)return true;
 int32 W=0,H=0;P->GetViewportSize(W,H);float MX=0,MY=0;
 const float Scale=FMath::Min(W/1920.f,H/1080.f);
 if(Scale<=0||(!P->GetMousePosition(MX,MY)&&!V28ReviewPointer.IsSet()))return true;
 const FVector2D Mouse=V28ReviewPointer.IsSet()?V28ReviewPointer.GetValue():FVector2D((MX-(W-1920*Scale)*.5f)/Scale,(MY-(H-1080*Scale)*.5f)/Scale);
 if(Mouse.X>=92&&Mouse.X<=1109&&Mouse.Y>=278&&Mouse.Y<=617)
 {if(!S.Gallery.Ready)S.Gallery.Reset(S.ExhibitRowV8311*ExhibitLayoutV8311::SlotH);S.Gallery.Target=FMath::Clamp(S.Gallery.Target+Direction*80.f,0.f,ExhibitLayoutV8311::Maximum(Exhibits(EFRRUNationV81(S.Nation)).Num())*ExhibitLayoutV8311::SlotH);return true;}
 if(Mouse.X<92||Mouse.X>1109||Mouse.Y<652||Mouse.Y>964)return true;
 const auto A=Items(EFRRUNationV81(S.Nation));
 if(!S.Catalogue.Ready)S.Catalogue.Reset(S.Row*162.f);
 S.Catalogue.Target=FMath::Clamp(S.Catalogue.Target+Direction*68.f,0.f,FMath::Max(0,(A.Num()+2)/3-2)*162.f);return true;
}
bool Back(APlayerController* P){auto& S=View(P);if(!S.Inspect)return false;S.Inspect=false;return true;}
void PreloadVariants(const TArray<int32>& Indices,bool EarnedOnly);
TArray<FIntPoint> Exhibits(EFRRUNationV81 Nation)
{
 TArray<FIntPoint> Result;for(int32 I:FrontlineMedalsV8215::NationIndices(Nation))for(int32 G=1;G<=FMath::Min(FrontlineMedalsV8215::Tier(I),FrontlineMedalsV8215::Variants(I).Num());++G)Result.Add(FIntPoint(I,G));
 const int32 Id=DeedBaseV8222+int32(Nation);const auto D=Deed(Id);if(D.Available&&D.Distinctions>0)Result.Add(FIntPoint(Id,D.Distinctions));return Result;
}
FName WarRoomRibbonNameV8444(EFRRUNationV81 Nation,int32 I)
{return FName(*FString::Printf(TEXT("V8444WarRoomMedalWall%d_%d"),int32(Nation),I));}
bool ParseWarRoomRibbonV8444(FName Name,int32& Nation,int32& I)
{
 Nation=I=INDEX_NONE;const FString Value=Name.ToString(),Prefix(TEXT("V8444WarRoomMedalWall"));
 if(!Value.StartsWith(Prefix))return false;
 FString NationText,ItemText;if(!Value.RightChop(Prefix.Len()).Split(TEXT("_"),&NationText,&ItemText)
  ||NationText.Len()!=1||!NationText.IsNumeric()||!ItemText.IsNumeric())return false;
 Nation=FCString::Atoi(*NationText);I=FCString::Atoi(*ItemText);return Nation>=0&&Nation<4;
}
bool OpenWarRoomRibbonV8444(APlayerController* P,AWarfareFrontlineGameMode* B,FName Name)
{
 int32 Nation=INDEX_NONE,I=INDEX_NONE;auto* PC=Cast<AWarfareFrontlineController>(P);
 if(!PC||!B||B->Phase!=EFrontlinePhase::Briefing||(!WarRoom52::Active(PC)&&!FRRUWarRoomV82::Active(PC))
  ||!ParseWarRoomRibbonV8444(Name,Nation,I)||Nation!=int32(WarfareFRRUCampaignV82::Nation(B,0))
  ||!OrderedRibbonItems(EFRRUNationV81(Nation)).Contains(I))return false;
 // Navigation changes only the museum view. Campaign selection, map browsing
 // and the account's actual highest decoration classes remain authoritative.
 auto& S=View(P);S.Wall=true;S.Inspect=false;S.Nation=Nation;S.Focus=I;
 SelectInspectionPositionV84413(S,InspectionPositionV84413(I));S.Row=S.ExhibitRowV8311=0;
 S.Catalogue.Reset();S.Gallery.Reset();S.ScrollNation=Nation;return true;
}
bool Hit(APlayerController* P,FName Name)
{
 const FString N=Name.ToString();auto& S=View(P);
 if(N==TEXT("V8215WarRecord")){S.Wall=false;S.Inspect=false;return true;}
 if(N==TEXT("V8215MedalWall")){S.Wall=true;S.Inspect=false;return true;}
 if(N.StartsWith(TEXT("V8215MedalNation"))){const FString D=N.RightChop(16);if(D.Len()==1&&D.IsNumeric()){const int32 I=FCString::Atoi(*D);if(I<4){S.Nation=I;S.Row=S.ExhibitRowV8311=0;S.Catalogue.Reset();S.Gallery.Reset();S.ScrollNation=I;const auto A=Items(EFRRUNationV81(I));if(!A.IsEmpty()){S.Focus=A[0];SelectInspectionPositionV84413(S,InspectionPositionV84413(S.Focus));}}}S.Inspect=false;return true;}
 if(N.StartsWith(TEXT("V8216MedalSelect"))){const FString D=N.RightChop(16);const int32 I=D.IsNumeric()?FCString::Atoi(*D):INDEX_NONE;const auto A=Items(EFRRUNationV81(S.Nation));if(A.Contains(I)){S.Focus=I;SelectInspectionPositionV84413(S,InspectionPositionV84413(I));}return true;}
 if(N.StartsWith(TEXT("V8217MedalExhibit")))
 {
  const FString Tail=N.RightChop(FString(TEXT("V8217MedalExhibit")).Len());FString Id,Grade;Tail.Split(TEXT("_"),&Id,&Grade);
  const int32 I=Id.IsNumeric()?FCString::Atoi(*Id):INDEX_NONE,G=Grade.IsNumeric()?FCString::Atoi(*Grade):0;
  if(Exhibits(EFRRUNationV81(S.Nation)).Contains(FIntPoint(I,G))){S.Focus=I;SelectInspectionPositionV84413(S,InspectionPositionV84413(I,G));PreloadVariants({I},true);S.Inspect=true;}return true;
 }
 if(N==TEXT("V8216MedalInspect")){PreloadVariants({S.Focus},false);S.Inspect=true;return true;}
 if(N==TEXT("V8216MedalInspectBack")){S.Inspect=false;return true;}
 if(N==TEXT("V8216MedalVariantPrev")||N==TEXT("V8216MedalVariantNext")){const int32 C=EntryVariants(S.Focus);if(C>0){if(S.PreviewStateV84413<1||S.PreviewStateV84413>C||InspectionStateV84413(S.Focus,S.PreviewStateV84413).Form.Grade!=S.PreviewGrade)SelectInspectionPositionV84413(S,InspectionPositionV84413(S.Focus,S.PreviewGrade));SelectInspectionPositionV84413(S,S.PreviewStateV84413+(N==TEXT("V8216MedalVariantNext")?1:-1));}return true;}
 return false;
}
void Tabs(const FFrontlineCanvas& UI)
{
 const bool Wall=Selected(UI.HUD.GetOwningPlayerController());
 UI.PageTabV73(92,148,424,WarRoom52::L(TEXT("WAR RECORD"),TEXT("战功册")),TEXT("V8215WarRecord"),!Wall,true,51);
 UI.PageTabV73(538,148,424,WarRoom52::L(TEXT("MEDAL WALL"),TEXT("勋章墙")),TEXT("V8215MedalWall"),Wall,true,51);
}
UTexture2D* Art(int32 I,int32 Grade=0)
{
 if(IsDeed(I))return ArmourKillTallyUIV8220::Badge(DeedNation(I));
 const auto Award=FrontlineMedalsV8215::Award(I);const FString Path=FrontlineMedalsV8215::VariantArtworkPath(I,Grade>0?Grade:FMath::Max(1,int32(Award.Grade)));
 return FrontlineUIAssetsV8213::Texture(Path);
}
void PreloadVariants(const TArray<int32>& Indices,bool EarnedOnly)
{
 TArray<FString> Paths;
 for(int32 I:Indices){if(IsDeed(I)){Paths.Add(FrontlineArmourValourV8221::BadgePath(DeedNation(I)));continue;}const auto V=FrontlineMedalsV8215::Variants(I);const int32 Last=EarnedOnly?FMath::Min(V.Num(),FrontlineMedalsV8215::Tier(I)):V.Num();for(int32 G=1;G<=Last;++G)Paths.Add(FrontlineMedalsV8215::VariantArtworkPath(I,G));}
 for(int32 I:Indices)if(I>=37&&I<FrontlineMedalsV8215::Count&&FrontlineMedalsV8215::Definition(I).Ribbon){const FString Stem=FString::Printf(TEXT("T_Ribbon_%d_V84414"),I);Paths.Add(TEXT("/Game/Warfare/UI/V84414/Honours/")+Stem+TEXT(".")+Stem);}
 for(const TCHAR* Stem:{TEXT("T_MedalAttachment_SilverBar_V8216"),TEXT("T_MedalAttachment_GoldBar_V8216"),TEXT("T_MedalAttachment_BronzeBar_V8216"),TEXT("T_MedalAttachment_OakLeaves_V8216"),TEXT("T_MedalAttachment_Palm_V8216"),TEXT("T_MedalAttachment_BronzeDespatchOakLeaf_V8216"),TEXT("T_MedalAttachment_SilverRosette_V8216"),TEXT("T_MedalAttachment_GoldRosette_V8216")})Paths.Add(FString(TEXT("/Game/Warfare/UI/V8216/"))+Stem+TEXT(".")+Stem);
 for(const TCHAR* Stem:{TEXT("T_MedalAttachment_SilverCrownBar_V85"),TEXT("T_MedalAttachment_SilverEagleBar_V85")})Paths.Add(FString(TEXT("/Game/Warfare/UI/V85/Medals/"))+Stem+TEXT(".")+Stem);
 Paths.Add(TEXT("/Game/Warfare/UI/V8221/T_StGeorgeBowV8221.T_StGeorgeBowV8221"));
 Paths.Add(TEXT("/Game/Warfare/UI/V8221/T_CitationStarV8221.T_CitationStarV8221"));
 Paths.Add(TEXT("/Game/Warfare/UI/V8217/T_MedalDisplayWall_V8217.T_MedalDisplayWall_V8217"));
 for(const TCHAR* Code:{TEXT("uk"),TEXT("de"),TEXT("fr"),TEXT("ru")}){const FString Stem=FString(TEXT("T_RibbonAtlas_"))+Code+TEXT("_V8217");Paths.Add(TEXT("/Game/Warfare/UI/V8217/")+Stem+TEXT(".")+Stem);}
 FrontlineUIAssetsV8213::PreloadTextures(Paths);
}
void Attachments(const FFrontlineCanvas& UI,int32 I,float X,float Y,float W,float H,float Angle=0,float AX=0,float AY=0,const FrontlineMedalsV8215::FAward* Inspection=nullptr)
{
 const auto A=FrontlineMedalsV8215::PresentedAwardV85(I,Inspection?*Inspection:FrontlineMedalsV8215::Award(I));if(!A.Grade)return;
 auto Draw=[&](const TCHAR* Stem,float DX,float DY,float DW,float DH)
 {
  const FString Kind(Stem);
  const FString Path=FString(Kind.EndsWith(TEXT("_V85"))?TEXT("/Game/Warfare/UI/V85/Medals/"):TEXT("/Game/Warfare/UI/V8216/"))+Stem+TEXT(".")+Stem;
  auto* T=FrontlineUIAssetsV8213::Texture(Path);if(!T)return;
  float U=0,V=0,UW=1,VH=1;
  if(Kind.Contains(TEXT("SilverCrownBar"))){U=36.f/1882;V=224.f/836;UW=1812.f/1882;VH=316.f/836;}
  else if(Kind.Contains(TEXT("SilverEagleBar"))){U=48.f/2172;V=223.f/724;UW=2077.f/2172;VH=249.f/724;}
  else if(Kind.Contains(TEXT("SilverBar"))){U=53.f/2172;V=232.f/724;UW=2066.f/2172;VH=235.f/724;}
  else if(Kind.Contains(TEXT("GoldBar"))){U=37.f/1881;V=227.f/836;UW=1809.f/1881;VH=313.f/836;}
  else if(Kind.Contains(TEXT("BronzeBar"))){U=63.f/2172;V=232.f/724;UW=2047.f/2172;VH=237.f/724;}
  else if(Kind.Contains(TEXT("BronzeDespatchOakLeaf"))){U=50.f/1774;V=119.f/887;UW=1681.f/1774;VH=656.f/887;}
  else if(Kind.Contains(TEXT("OakLeaves"))){U=82.f/1230;V=91.f/1278;UW=1095.f/1230;VH=1098.f/1278;}
  else if(Kind.Contains(TEXT("Palm"))){U=46.f/1536;V=153.f/1024;UW=1467.f/1536;VH=701.f/1024;}
  if(Angle!=0)UI.MedalOverlayV8216(T,DX,DY,DW,DH,Angle,AX,AY,U,V,UW,VH);else UI.Image(T,DX,DY,DW,DH,FLinearColor(.90f,.91f,.86f),U,V,UW,VH,false,true);
 };
 using E=FrontlineMedalsV8215::EAttachment;
 if(A.Attachments&uint16(E::Bar))
 {
  const TCHAR* Bar=I==2?TEXT("T_MedalAttachment_BronzeBar_V8216"):I==12?TEXT("T_MedalAttachment_GoldBar_V8216"):
   (I==1||I==37)?TEXT("T_MedalAttachment_SilverCrownBar_V85"):(I==38||I==39)?TEXT("T_MedalAttachment_SilverEagleBar_V85"):TEXT("T_MedalAttachment_SilverBar_V8216");
  const auto Placement=FrontlineMedalPlacementV84415::Layout(I,A.Repeats,X,Y,W,H);
  for(const auto& Box:Placement.Bars)Draw(Bar,Box.Min.X,Box.Min.Y,Box.GetSize().X,Box.GetSize().Y);
 }
 if(A.Attachments&uint16(E::OakLeaves))Draw(I==17?TEXT("T_MedalAttachment_BronzeDespatchOakLeaf_V8216"):TEXT("T_MedalAttachment_OakLeaves_V8216"),X+W*.38f,Y+H*(I==5?.40f:.24f),W*.24f,H*.16f);
 if(A.Attachments&uint16(E::Palm))Draw(TEXT("T_MedalAttachment_Palm_V8216"),X+W*.31f,Y+H*.22f,W*.38f,H*.12f);
 if(A.Attachments&uint16(E::Star))
  UI.ImageContain(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8221/T_CitationStarV8221.T_CitationStarV8221")),X+W*.42f,Y+H*(A.Attachments&uint16(E::Palm)?.36f:.24f),W*.16f,H*.12f,FLinearColor(.91f,.92f,.87f));
}
void RunPresentationChecksV84415(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Cloth=true,Spacing=true,Exact=true,Limits=true,Canvas=true;
 for(int32 I:{0,1,2,12,13,37,38,39})for(int32 Count=0;Count<=255;++Count)
  for(const FVector2D Size:{FVector2D(92,138),FVector2D(94,112),FVector2D(120,172),FVector2D(362,244),FVector2D(618,664)})
 {
  const auto Layout=FrontlineMedalPlacementV84415::Layout(I,Count,27,39,Size.X,Size.Y);
  const int32 Actual=FrontlineMedalsV8215::PresentedRepeatsV85(I,Count);
  Exact&=Layout.RecordedCount==Actual&&Layout.Bars.Num()==Actual;
  Cloth&=Layout.Valid&&Layout.Textile.Min.X>=27&&Layout.Textile.Min.Y>=39
   &&Layout.Textile.Max.X<=27+Size.X+.001f&&Layout.Textile.Max.Y<=39+Size.Y+.001f;
  Limits&=Layout.Bars.Num()<=FrontlineMedalsV8215::MaximumRepeatsV85;
  for(int32 K=0;K<Layout.Bars.Num();++K)
  {
   const auto& B=Layout.Bars[K];Cloth&=B.Min.X>=Layout.Textile.Min.X-.001f&&B.Min.Y>=Layout.Textile.Min.Y-.001f
    &&B.Max.X<=Layout.Textile.Max.X+.001f&&B.Max.Y<=Layout.Textile.Max.Y+.001f;
   Spacing&=B.GetSize().X>0&&B.GetSize().Y>0&&FMath::IsNearlyEqual(float(B.GetCenter().X),float(Layout.Textile.GetCenter().X),.001f)
    &&(K==0||B.Min.Y>Layout.Bars[K-1].Max.Y);
   Canvas&=FMath::IsFinite(B.Min.X)&&FMath::IsFinite(B.Min.Y)&&FMath::IsFinite(B.Max.X)&&FMath::IsFinite(B.Max.Y);
  }
  if(!Layout.Bars.IsEmpty())Spacing&=FMath::IsNearlyEqual(float((Layout.Bars[0].Min.Y+Layout.Bars.Last().Max.Y)*.5),float(Layout.Textile.GetCenter().Y),.001f);
 }
 Check(TEXT("v84415_every_bar_stays_inside_its_own_source_cloth_at_all_ui_scales"),Cloth);
 Check(TEXT("v85_supported_bars_are_centred_ordered_and_do_not_overlap"),Spacing);
 Check(TEXT("v85_legacy_repeat_count_projects_to_its_own_medal_display_range"),Exact&&Limits&&Canvas);
 bool DeviceCentres=true;
 for(const FVector2D Size:{FVector2D(66,22),FVector2D(112,30),FVector2D(224,58)})
  for(int32 N=1;N<=FrontlineMedalsV8215::MaximumRepeatsV85;++N)
   for(int32 K=0;K<N;++K)
   {
    const auto B=FrontlineMedalPlacementV84415::RibbonDeviceBoundsV85(27,39,Size.X,Size.Y,N,K);
    DeviceCentres&=B.bIsValid&&FMath::IsNearlyEqual(float(B.GetCenter().Y),39+float(Size.Y)*.5f,.001f)
     &&B.Min.X>=27&&B.Max.X<=27+Size.X&&B.Min.Y>=39&&B.Max.Y<=39+Size.Y;
    if(K>0){const auto Prior=FrontlineMedalPlacementV84415::RibbonDeviceBoundsV85(27,39,Size.X,Size.Y,N,K-1);DeviceCentres&=B.Min.X>Prior.Max.X;}
   }
 Check(TEXT("v85_all_ribbon_devices_are_vertically_centred_and_separated_at_wall_preview_and_hud_sizes"),DeviceCentres);
 Check(TEXT("v84415_unknown_or_invalid_bar_geometry_cannot_draw_on_medal_body"),
  !FrontlineMedalPlacementV84415::Layout(3,4,0,0,100,100).Valid
  &&!FrontlineMedalPlacementV84415::Layout(37,4,0,0,0,100).Valid
  &&!FrontlineMedalPlacementV84415::Layout(37,4,0,0,100,-1).Valid);
 const auto DSC=FrontlineMedalPlacementV84415::Layout(37,4,0,0,1024,1536);
 Check(TEXT("v85_dsc_legacy_fourth_repeat_is_bounded_inside_real_cloth"),DSC.Bars.Num()==FrontlineMedalsV8215::PresentedRepeatsV85(37,4)
  &&DSC.Bars.Last().Max.Y<=609.001f);
 const auto DSC5=FrontlineMedalPlacementV84415::Layout(37,5,0,0,1024,1536);
 Check(TEXT("v85_dsc_legacy_fifth_repeat_stays_on_its_cloth"),DSC5.Bars.Num()==FrontlineMedalsV8215::PresentedRepeatsV85(37,5)&&DSC5.Bars.Last().Max.Y<=609.001f);
 bool RibbonSpacing=true;
 for(const FVector2D Size:{FVector2D(42,14),FVector2D(93,24),FVector2D(220,60),FVector2D(486,106)})for(int32 N=1;N<=FrontlineMedalsV8215::MaximumRepeatsV85;++N)
 {
  const float Device=FrontlineMedalPlacementV84415::RibbonDeviceSizeV85(Size.X,Size.Y,N),Pitch=Size.X/(N+1.f);
  RibbonSpacing&=Device>0&&Device<Pitch&&Device<=Size.Y&&Pitch-Device*.5f>=0&&Pitch*N+Device*.5f<=Size.X;
 }
 Check(TEXT("v85_all_supported_ribbon_repeat_devices_fit_without_overlap"),RibbonSpacing);
 bool Permanent=true,Locked=true,NoInvented=true;
 for(int32 I=0;I<FrontlineMedalsV8215::Count;++I)
 {
  if(I==4)continue;const auto Empty=FrontlineMedalsV8215::InspectionStatesV84413(I,{});
  FrontlineMedalsV8215::FAward Full;Full.Grade=FrontlineMedalsV8215::Variants(I).Num();Full.Repeats=255;
  const auto Earned=FrontlineMedalsV8215::InspectionStatesV84413(I,Full);
  Permanent&=Empty.Num()==Earned.Num()&&!Empty.IsEmpty();
  for(int32 K=0;K<Empty.Num();++K)
  {
   Locked&=!Empty[K].Earned;
   Permanent&=Empty[K].Form.Grade==Earned[K].Form.Grade&&Empty[K].Form.Repeats==Earned[K].Form.Repeats;
   NoInvented&=Empty[K].Form.Grade>=1&&Empty[K].Form.Grade<=FrontlineMedalsV8215::Variants(I).Num();
  }
 }
 Check(TEXT("v84415_preview_sequence_is_complete_before_any_medal_or_repeat_is_earned"),Permanent&&NoInvented);
 Check(TEXT("v84415_future_insignia_previews_are_never_marked_awarded"),Locked);
}
struct FDeedGeometryV83
{
 FFrontlineImagePlacement Badge{};
 float RibbonX=0,RibbonY=0,RibbonW=0,RibbonH=0,RibbonOverhang=0;
 bool HasRibbon=false,Valid=false;
};
FDeedGeometryV83 DeedGeometryV83(EFRRUNationV81 Nation,int32 Grade,float X,float Y,float W,float H,bool Earned)
{
 FDeedGeometryV83 P;const int32 N=int32(Nation);
 if(N<0||N>3||!FMath::IsFinite(X)||!FMath::IsFinite(Y)||!FMath::IsFinite(W)||!FMath::IsFinite(H)||W<=0||H<=0)return P;
 // Visible alpha bounds of the accepted four sources, with two texels of
 // breathing room. Remove empty canvas padding, not any part of the emblem.
 static const FIntRect Cuts[]={FIntRect(27,18,1267,1276),FIntRect(26,18,1242,1251),FIntRect(128,18,1194,1305),FIntRect(40,18,1241,1264)};
 static const FIntPoint SourceSizes[]={FIntPoint(1294,1294),FIntPoint(1269,1269),FIntPoint(1323,1323),FIntPoint(1282,1282)};
 const auto Cut=Cuts[N];const float Aspect=float(Cut.Width())/Cut.Height();
 // A wall specimen is the chest decoration alone. Cloth and repeat devices
 // belong once in the shared ribbon rack, not below every individual badge.
 // A chest badge has no hanging cloth: compare its metal silhouette with the
 // lower metal body of neighbouring ribboned decorations, not their full height.
 const float BadgeH=FMath::Max(0.f,FMath::Min(H*.46f,W/Aspect));
 const float BadgeW=BadgeH*Aspect;
 P.Badge={X+(W-BadgeW)*.5f,Y+(H-BadgeH)*.5f,BadgeW,BadgeH,
  Cut.Min.X/float(SourceSizes[N].X),Cut.Min.Y/float(SourceSizes[N].Y),Cut.Width()/float(SourceSizes[N].X),Cut.Height()/float(SourceSizes[N].Y)};
 P.Valid=BadgeW>0&&BadgeH>0;return P;
}
void RunDeedGeometryChecksV83(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Centred=true,Fits=true,UV=true;
 for(int32 N=0;N<4;++N)for(int32 G=1;G<=(N==1?2:4);++G)for(bool Earned:{false,true})
  for(const FVector2D Size:{FVector2D(362*.93f,304*.93f),FVector2D(618,750),FVector2D(120,172),FVector2D(94,112),FVector2D(240,100)})
 {
  const auto P=DeedGeometryV83(EFRRUNationV81(N),G,13,29,Size.X,Size.Y,Earned);
  const float LastY=P.Badge.Y+P.Badge.H;
  Centred&=P.Valid&&FMath::IsNearlyEqual((P.Badge.Y+LastY)*.5f,float(29+Size.Y*.5),.002f)
   &&FMath::IsNearlyEqual(P.Badge.X+P.Badge.W*.5f,float(13+Size.X*.5),.002f);
  Fits&=P.Valid&&P.Badge.X>=12.999f&&P.Badge.Y>=28.999f&&P.Badge.X+P.Badge.W<=13+Size.X+.001f
   &&LastY<=29+Size.Y+.001f&&!P.HasRibbon&&P.RibbonW==0&&P.RibbonH==0;
  UV&=P.Badge.U>=0&&P.Badge.V>=0&&P.Badge.UW>0&&P.Badge.VH>0&&P.Badge.U+P.Badge.UW<=1&&P.Badge.V+P.Badge.VH<=1;
 }
 Check(TEXT("v837_deed_chest_badge_centred_for_four_armies_and_all_grades"),Centred);
 Check(TEXT("v837_deed_wall_specimen_is_badge_only_inside_viewport"),Fits);
 Check(TEXT("v83_deed_transparent_padding_crop_within_source"),UV);
 Check(TEXT("v83_deed_invalid_geometry_is_empty"),!DeedGeometryV83(EFRRUNationV81::Invalid,1,0,0,100,100,true).Valid
  &&!DeedGeometryV83(EFRRUNationV81::British,1,0,0,0,100,true).Valid);
}
void Decoration(const FFrontlineCanvas& UI,int32 I,int32 G,float X,float Y,float W,float H,bool Earned,const FrontlineMedalsV8215::FAward* Inspection=nullptr)
{
 auto* T=Art(I,G);if(!FrontlineTextureReady(T))return;
 const auto DeedLayout=IsDeed(I)?DeedGeometryV83(DeedNation(I),G,X,Y,W,H,Earned):FDeedGeometryV83();
 if(IsDeed(I)&&!DeedLayout.Valid)return;
 const auto P=IsDeed(I)?DeedLayout.Badge:PlaceFrontlineImage(T,X,Y,W,H,0,0,1,1,true);
 auto Draw=[&]
 {
  if(IsDeed(I))UI.Image(T,P.X,P.Y,P.W,P.H,FLinearColor(.94f,.95f,.91f),P.U,P.V,P.UW,P.VH,false,true);
  else UI.ImageContain(T,X,Y,W,H,FLinearColor(.94f,.95f,.91f));
  if(!IsDeed(I)&&(Inspection||(Earned&&G==FrontlineMedalsV8215::Tier(I))))Attachments(UI,I,P.X,P.Y,P.W,P.H,0,0,0,Inspection);
 };
 if(Earned){Draw();return;}
 else
 {
  auto& A=Artwork();if(!A.MedalLockedMaterialV8217)return;
  TArray<FTooltipOpV83> Ops;auto* Previous=UI.RecordingV83;
  {TGuardValue<bool> Audit(V36TypographyAuditActive,false);UI.RecordingV83=&Ops;Draw();UI.RecordingV83=Previous;}
  for(const auto& O:Ops)if(O.Kind==ETooltipOpV83::Texture)
  {
   auto* Part=Cast<UTexture2D>(O.Texture);if(!FrontlineTextureReady(Part))continue;
   auto& M=A.GrayscalePhotos.FindOrAdd(Part);if(!M||M->Parent!=A.MedalLockedMaterialV8217){M=UMaterialInstanceDynamic::Create(A.MedalLockedMaterialV8217,GetTransientPackage());M->SetTextureParameterValue(TEXT("Photo"),Part);}
   UI.MaterialTile(M,O.X,O.Y,O.W,O.H,Part->GetResource(),O.U,O.V,O.UW,O.VH);
  }
 }
}
void Icon(const FFrontlineCanvas& UI,int32 I,float X,float Y,float W,float H,bool Earned)
{Decoration(UI,I,FMath::Max(1,EntryTier(I)),X,Y,W,H,Earned);}
// Crop each queued museum primitive in design coordinates. Canvas scissor is
// flush-wide, so changing it around a card would also clip deferred HDR UI.
// Text stays as complete shaped rows and fades at the aperture, never leaving
// half glyphs or hitboxes above the sticky ribbon rack.
template<typename Builder> void MuseumViewportV842(const FFrontlineCanvas& UI,const FBox2D& Clip,Builder&& Build)
{
 TArray<FTooltipOpV83> Ops;auto* Previous=UI.RecordingV83;
 {TGuardValue<bool> Audit(V36TypographyAuditActive,false);UI.RecordingV83=&Ops;Build();UI.RecordingV83=Previous;}
 auto Crop=[&](float& X,float& Y,float& W,float& H,float& U,float& V,float& UW,float& VH)
 {
  if(W<=0||H<=0)return false;const float X0=X,Y0=Y,W0=W,H0=H;
  const float L=FMath::Max(X,float(Clip.Min.X)),T=FMath::Max(Y,float(Clip.Min.Y));
  const float R=FMath::Min(X+W,float(Clip.Max.X)),B=FMath::Min(Y+H,float(Clip.Max.Y));
  if(R<=L||B<=T)return false;U+=(L-X0)/W0*UW;V+=(T-Y0)/H0*VH;UW*=float(R-L)/W0;VH*=float(B-T)/H0;X=L;Y=T;W=R-L;H=B-T;return true;
 };
 for(const auto& O:Ops)
 {
  float X=O.X,Y=O.Y,W=O.W,H=O.H,U=O.U,V=O.V,UW=O.UW,VH=O.VH;
  if(O.Kind==ETooltipOpV83::Text)
  {
   const auto S=UI.ShapedV8210(O.Text.ToString(),O.Font,O.W);if(!S.IsValid())continue;
   const float TW=S->GetMeasuredWidth()/UI.RasterUnitsV825(),TH=S->GetMaxTextHeight()/UI.RasterUnitsV825();
   if(X<Clip.Min.X||X+TW>Clip.Max.X||Y<Clip.Min.Y||Y+TH>Clip.Max.Y)continue;
   auto Ink=O.Ink;Ink.A*=FMath::Clamp(FMath::Min(Y-float(Clip.Min.Y),float(Clip.Max.Y)-Y-TH)/12.f,0.f,1.f);
   UI.DrawShapedV8210(UI.Screen(X,Y),O.Text.ToString(),O.Font,Ink,O.W);continue;
  }
  if(O.Kind==ETooltipOpV83::Line)
  {
   const bool Vertical=FMath::IsNearlyEqual(X,W);
   if(Vertical){if(X<Clip.Min.X||X>Clip.Max.X)continue;Y=FMath::Clamp(Y,float(Clip.Min.Y),float(Clip.Max.Y));H=FMath::Clamp(H,float(Clip.Min.Y),float(Clip.Max.Y));}
   else {if(Y<Clip.Min.Y||Y>Clip.Max.Y)continue;X=FMath::Clamp(X,float(Clip.Min.X),float(Clip.Max.X));W=FMath::Clamp(W,float(Clip.Min.X),float(Clip.Max.X));}
   UI.Line(X,Y,W,H,O.Ink,O.Width);continue;
  }
  if(!Crop(X,Y,W,H,U,V,UW,VH))continue;
  switch(O.Kind)
  {
   case ETooltipOpV83::Texture:UI.TextureV83(O.Texture,UI.OX+X*UI.Scale,UI.OY+Y*UI.Scale,W*UI.Scale,H*UI.Scale,U,V,UW,VH,O.Ink);break;
   case ETooltipOpV83::Material:UI.MaterialTile(O.Material,X,Y,W,H,O.Coverage,U,V,UW,VH,O.Ink);break;
   case ETooltipOpV83::Rect:UI.Rect(X,Y,W,H,O.Ink);break;
   case ETooltipOpV83::Gradient:
   {const float A=O.Vertical?(Y-O.Y)/O.H:(X-O.X)/O.W,B=O.Vertical?(Y+H-O.Y)/O.H:(X+W-O.X)/O.W;UI.Gradient(X,Y,W,H,FMath::Lerp(O.Ink,O.End,A),FMath::Lerp(O.Ink,O.End,B),O.Vertical);break;}
   case ETooltipOpV83::Hit:UI.Hit(X,Y,W,H,O.HitName,O.Priority);break;
   default:break;
  }
 }
}
void RunScrollChecksV842(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Smooth=true,Bounded=true,Stable=true;float Results[3]={};int32 K=0;
 for(float Step:{1.f/30,1.f/60,1.f/144})
 {
  FScrollV842 S;S.Reset();S.Last=1;S.Target=136;
  for(int32 I=1;I<=FMath::RoundToInt(.5f/Step);++I){const float Prev=S.Offset;S.Advance(1+I*Step,324);Smooth&=S.Offset>=Prev-.001f&&S.Offset<=136.001f;}
  Results[K++]=S.Offset;S.Target=999;for(int32 I=1;I<=200;++I)S.Advance(2+I*Step,324);Bounded&=S.Offset>=0&&S.Offset<=324&&S.Target==324;
  S.Target=-999;S.Advance(10,324,true);Stable&=S.Offset==0&&S.Velocity==0;
 }
 Check(TEXT("v842_museum_damped_motion_is_monotonic_and_bounded"),Smooth&&Bounded);
 Check(TEXT("v842_museum_damping_matches_30_60_144hz"),FMath::Abs(Results[0]-Results[1])<.05f&&FMath::Abs(Results[1]-Results[2])<.05f);
 Check(TEXT("v842_museum_stable_review_and_empty_bounds_do_not_drift"),Stable);
}
// Native alpha readback selects the interior weave, excluding rounded alpha
// edges and fray. All consumers cover their adjoining 4:1 cells with this cut;
// containing differently proportioned source cuts reintroduces little gutters.
FIntRect RibbonTextileRectV8218(int32 Nation,int32 Slot)
{
 static const FIntRect Cuts[4][9]={
  {{50,67,693,206},{41,65,684,206},{31,65,675,206},{51,48,692,187},{42,47,683,185},{32,46,675,186},{51,32,693,170},{41,30,684,170},{31,30,675,170}},
  {{51,49,674,204},{51,49,674,204},{51,49,673,204},{51,34,674,189},{51,34,674,189},{51,34,673,189},{51,19,674,173},{51,19,674,173},{51,19,673,173}},
  {{46,51,701,211},{34,51,689,211},{25,51,678,211},{48,38,698,197},{35,36,689,197},{26,38,677,198},{47,21,699,181},{35,21,689,181},{25,21,678,182}},
  {{30,47,696,209},{30,48,695,209},{30,48,695,209},{31,34,695,196},{31,34,694,195},{29,34,695,196},{31,23,694,184},{30,22,694,184},{29,21,694,182}}
 };
 return Cuts[FMath::Clamp(Nation,0,3)][FMath::Clamp(Slot,0,8)];
}
TArray<FBox2D> RibbonBarLayoutV8218(int32 Count,float X,float Y,float W,float H,bool Centred)
{
 return FrontlineRibbonRackV8222::Layout(Count,X,Y,W,H,Centred);
}
FBox2D RibbonRackBoundsV8444(const TArray<FBox2D>& Cells)
{
 FBox2D Bounds(ForceInit);for(const auto& Cell:Cells){Bounds+=Cell.Min;Bounds+=Cell.Max;}return Bounds;
}
void Ribbon(const FFrontlineCanvas& UI,int32 I,float X,float Y,float W,float H)
{
 if(IsDeed(I)){ArmourKillTallyUIV8220::Ribbon(UI,Deed(I),X,Y,W,H,false);return;}
 if(I>=37&&I<FrontlineMedalsV8215::Count)
 {
  if(!FrontlineMedalsV8215::Definition(I).Ribbon)return;
  const FString Stem=FString::Printf(TEXT("T_Ribbon_%d_V84414"),I),Path=TEXT("/Game/Warfare/UI/V84414/Honours/")+Stem+TEXT(".")+Stem;
  UI.Image(FrontlineUIAssetsV8213::Texture(Path),X,Y,W,H,FLinearColor(.94f,.95f,.92f),0,0,1,1,false,false);return;
 }
 // Swords mark this catalogue's combatant-war form of Hohenzollern. Its
 // statutory cloth is the Iron Cross ribbon, shared across these classes;
 // the white textile with three black stripes belongs to the peace form.
 const auto N=FrontlineMedalsV8215::Definition(I).Nation;const auto Items=FrontlineMedalsV8215::NationIndices(N);const int32 Slot=Items.Find(I==18?3:I);if(Slot==INDEX_NONE)return;
 static const TCHAR* Codes[]={TEXT("uk"),TEXT("de"),TEXT("fr"),TEXT("ru")};const int32 Nation=int32(N);if(Nation<0||Nation>3)return;
 const FString Stem=FString(TEXT("T_RibbonAtlas_"))+Codes[Nation]+TEXT("_V8217"),Path=TEXT("/Game/Warfare/UI/V8217/")+Stem+TEXT(".")+Stem;
 const auto Cut=RibbonTextileRectV8218(Nation,Slot);const float U=((Slot%3)*724.f+Cut.Min.X)/2172.f,V=((Slot/3)*241.f+Cut.Min.Y)/724.f;
 UI.Image(FrontlineUIAssetsV8213::Texture(Path),X,Y,W,H,FLinearColor(.94f,.95f,.92f),U,V,Cut.Width()/2172.f,Cut.Height()/724.f,false,false);
}
void RibbonDevicesV8446(const FFrontlineCanvas& UI,int32 I,const FrontlineMedalRibbonV8446::FPresentation& P,float X,float Y,float W,float H)
{
 using E=FrontlineMedalRibbonV8446::EDevice;using M=FrontlineMedalRibbonV8446::EMetal;
 auto Draw=[&](const TCHAR* Stem,float DX,float DY,float DW,float DH)
 {
  const FString Path=FString(TEXT("/Game/Warfare/UI/V8216/"))+Stem+TEXT(".")+Stem;
  const FString Kind(Stem);auto* Texture=FrontlineUIAssetsV8213::Texture(Path);
  if(Kind.Contains(TEXT("Rosette")))
  {
   // Centre the visible metal, not the asymmetric transparent source canvas.
   const FIntRect Cut=Kind.Contains(TEXT("Gold"))?FIntRect(0,21,1236,1222):FIntRect(0,21,1196,1189);
   const float Aspect=float(Cut.Width())/Cut.Height(),RW=FMath::Min(DW,DH*Aspect),RH=RW/Aspect;
   UI.Image(Texture,DX+(DW-RW)*.5f,DY+(DH-RH)*.5f,RW,RH,FLinearColor(.88f,.89f,.84f),
    Cut.Min.X/1254.f,Cut.Min.Y/1254.f,Cut.Width()/1254.f,Cut.Height()/1254.f,false,true);
  }
  else UI.ImageContain(Texture,DX,DY,DW,DH,FLinearColor(.88f,.89f,.84f));
 };
 auto Bar=[&](bool Gold,float DX,float DW,bool PlainBacking=false)
 {
  const TCHAR* Stem=Gold?TEXT("T_MedalAttachment_GoldBar_V8216"):TEXT("T_MedalAttachment_SilverBar_V8216");
  const FString Path=FString(TEXT("/Game/Warfare/UI/V8216/"))+Stem+TEXT(".")+Stem;
  const float SourceW=Gold?1881.f:2172.f,SourceH=Gold?836.f:724.f;
  // Native pixel review identified these plain top rims. The British award
  // crown/laurel engraving is outside both cuts and cannot enter this backing.
  const FIntRect Cut=PlainBacking?(Gold?FIntRect(110,272,710,309):FIntRect(105,231,755,268)):
   (Gold?FIntRect(37,227,1846,540):FIntRect(53,232,2119,467));
  UI.Image(FrontlineUIAssetsV8213::Texture(Path),DX,Y+H*.37f,DW,H*.26f,FLinearColor(.90f,.91f,.86f),
   Cut.Min.X/SourceW,Cut.Min.Y/SourceH,Cut.Width()/SourceW,Cut.Height()/SourceH,false,true);
 };
 // The narrow silver/gold demi-barrette sits behind the red cloth rosette.
 for(const auto& D:P.Devices)if(D.Kind==E::RosetteBacking)
 {
  const float BW=H*2.20f,BX=X+(W-BW)*.5f;
  if(D.Metal==M::SilverAndGold){Bar(false,BX,BW*.5f,true);Bar(true,BX+BW*.5f,BW*.5f,true);}
  else Bar(D.Metal==M::Gold,BX,BW,true);
 }
 int32 Count=0;for(const auto& D:P.Devices)if(D.Kind!=E::RosetteBacking)Count+=FMath::Min(FrontlineMedalsV8215::MaximumRepeatsV85,D.Count);
 int32 Position=0;
 for(const auto& D:P.Devices)if(D.Kind!=E::RosetteBacking)for(int32 K=0;K<FMath::Min(FrontlineMedalsV8215::MaximumRepeatsV85,D.Count);++K)
 {
  const auto Bounds=FrontlineMedalPlacementV84415::RibbonDeviceBoundsV85(X,Y,W,H,Count,Position++);
  const float CX=Bounds.GetCenter().X,Size=Bounds.GetSize().X,DY=Bounds.Min.Y;
  if(D.Kind==E::Bow&&(I==9||I==10))
   UI.ImageContain(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8221/T_StGeorgeBowV8221.T_StGeorgeBowV8221")),CX-W*.23f,Y+H*.04f,W*.46f,H*.92f,FLinearColor(.91f,.92f,.86f));
  else if(D.Kind==E::Rosette&&D.Metal==M::RedTextile)
  {
   // The existing Officer source contains the actual red cloth rosette.
   // Its measured source cut avoids recolouring a metallic repeat rosette.
   UI.Image(Art(8,2),CX-Size*.5f,DY,Size,Size,FLinearColor(.94f,.95f,.92f),336.f/1024,88.f/1536,360.f/1024,360.f/1536,false,true);
  }
  else if(D.Kind==E::Rosette)Draw(D.Metal==M::Gold?TEXT("T_MedalAttachment_GoldRosette_V8216"):TEXT("T_MedalAttachment_SilverRosette_V8216"),CX-Size*.5f,DY,Size,Size);
  else if(D.Kind==E::MiniatureCross)UI.Image(Art(2,1),CX-Size*.5f,DY,Size,Size,FLinearColor(.90f,.88f,.81f),198.f/1024,843.f/1536,624.f/1024,624.f/1536,false,true);
  else if(D.Kind==E::OakLeaves)Draw(TEXT("T_MedalAttachment_BronzeDespatchOakLeaf_V8216"),CX-W*.18f,Y+H*.04f,W*.36f,H*.92f);
  else if(D.Kind==E::Palm)Draw(TEXT("T_MedalAttachment_Palm_V8216"),CX-W*(Count>1?.19f:.27f),Y+H*.07f,W*(Count>1?.38f:.54f),H*.86f);
  else if(D.Kind==E::Star)UI.ImageContain(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8221/T_CitationStarV8221.T_CitationStarV8221")),CX-Size*.5f,DY,Size,Size,FLinearColor(.91f,.92f,.87f));
  else if(D.Kind==E::Clasp)Bar(false,CX-W*.27f,W*.54f);
 }
}
void RibbonAttachments(const FFrontlineCanvas& UI,int32 I,float X,float Y,float W,float H,int32 Grade=0,FrontlineMedalRibbonV8446::EContext Context=FrontlineMedalRibbonV8446::EContext::EarnedRack)
{
 if(IsDeed(I))return; // Its national devices are drawn with the service textile.
 const auto A=FrontlineMedalsV8215::Award(I);
 const auto P=FrontlineMedalRibbonV8446::Presentation(I,Grade>0?Grade:int32(A.Grade),Context,A);
 if(P.HasRibbon())RibbonDevicesV8446(UI,I,P,X,Y,W,H);
}
void RibbonBarV8218(const FFrontlineCanvas& UI,EFRRUNationV81 N,float X,float Y,float W,float H,bool Centred,bool HoverDetails,bool WarRoomLinkV8444=false)
{
 const auto A=OrderedRibbonItems(N);const auto Cells=RibbonBarLayoutV8218(A.Num(),X,Y,W,H,Centred);
 for(int32 K=0;K<A.Num();++K)
 {
  const auto& Cell=Cells[K];const int32 I=A[K];const auto Size=Cell.GetSize();
  Ribbon(UI,I,Cell.Min.X,Cell.Min.Y,Size.X,Size.Y);RibbonAttachments(UI,I,Cell.Min.X,Cell.Min.Y,Size.X,Size.Y);
  // Only the visible textile is actionable. The unused header aperture and
  // spaces beside a short row continue to belong to the underlying map.
  if(WarRoomLinkV8444)UI.Hit(Cell.Min.X,Cell.Min.Y,Size.X,Size.Y,WarRoomRibbonNameV8444(N,I),75);
  if(!HoverDetails||!UI.Hovered(Cell.Min.X,Cell.Min.Y,Size.X,Size.Y))continue;
  const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
  // Allocate the same shaped rows that the existing paragraph renderer uses.
  // In particular, Chinese annotation has a 19px minimum and 1.8em leading.
  const FString HistoryCopy=History(I,ZH),RuleCopy=Rule(I,ZH);
  auto ParagraphHeightV831=[&](EFrontlineTextRoleV8210 Role,const FString& Source,float Em)
  {
   const FString Value=UI.RoleValueV8210(Role,Source);if(Value.IsEmpty())return 0.f;
   const bool CJK=UI.ContainsCJKV45(Value);TGuardValue<bool> FaceScope(UI.bRoleChineseV8210,CJK);
   if(CJK&&Role==EFrontlineTextRoleV8210::Annotation)Em=FMath::Max(Em,19.f);
   const auto Rows=UI.RoleRowsV8210(Role,Value,396.f,Em);
   const float Leading=Em*(CJK?(Role==EFrontlineTextRoleV8210::Annotation?1.8f:1.9f):(Role==EFrontlineTextRoleV8210::Objective?1.7f:1.48f));
   float GlyphH=0;for(const auto& Row:Rows)GlyphH=FMath::Max(GlyphH,float(UI.RoleSizeRawV8210(Role,Row,Em).Y));
   return FMath::CeilToFloat(FMath::Max(0,Rows.Num()-1)*Leading+GlyphH)+2.f;
  };
  const float HistoryTop=IsDeed(I)?123.f:103.f;
  const float HistoryH=FMath::Max(IsDeed(I)?79.f:86.f,ParagraphHeightV831(EFrontlineTextRoleV8210::Annotation,HistoryCopy,18.f));
  const float RuleTop=FMath::Max(IsDeed(I)?216.f:203.f,HistoryTop+HistoryH+14.f);
  const float RuleH=FMath::Max(IsDeed(I)?122.f:81.f,ParagraphHeightV831(EFrontlineTextRoleV8210::Objective,RuleCopy,17.f));
  const float PopupH=FMath::Max(IsDeed(I)?358.f:304.f,RuleTop+RuleH+20.f);
  const float TX=FMath::Clamp(float(Cell.Min.X)-30.f,24.f,1920.f-436.f-24.f);
  const float Below=Y+H+18.f,Above=Y-PopupH-18.f;
  const float Preferred=Below+PopupH<=1056.f?Below:Above>=24.f?Above:Below;
  const float TY=FMath::Clamp(Preferred,24.f,FMath::Max(24.f,1056.f-PopupH));
  UI.CursorPriorityV67=FMath::Max(UI.CursorPriorityV67,75);
  const auto Award=IsDeed(I)?FrontlineMedalsV8215::FAward{}:FrontlineMedalsV8215::Award(I);
  const uint32 Revision=IsDeed(I)?uint32(Deed(I).Total):HashCombineFast(Award.Grade,HashCombineFast(Award.Repeats,Award.Attachments));
  UI.PageTooltipV8222(TooltipKeyV83(23,uint32(I)),Revision,[&]{
   UI.PaperSheet(TX,TY,436,PopupH);const FLinearColor Ink(.025f,.029f,.025f);
   NameLabel(UI,I,ZH,TX+20,TY+16,396,IsDeed(I)?56:40,IsDeed(I)?21:23,Ink,1);
   UI.RoleLabel(EFrontlineTextRoleV8210::Text,AwardLabel(I,ZH),TX+20,TY+(IsDeed(I)?80:61),396,27,17,Ink,1);
   UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,HistoryCopy,TX+20,TY+HistoryTop,396,HistoryH,18,Ink);
   UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,RuleCopy,TX+20,TY+RuleTop,396,RuleH,17,Ink);
  },{TX,TY});
 }
}
struct FSelectedRibbonV8444{int32 Grade=0;bool Earned=false;};
FSelectedRibbonV8444 SelectedRibbonForGradesV8444(int32 I,int32 PreviewGrade,int32 HighestGrade)
{
 FSelectedRibbonV8444 R;
 if(IsDeed(I)&&PreviewGrade>0)R.Grade=FMath::Clamp(PreviewGrade,1,DeedNation(I)==EFRRUNationV81::German?2:4);
 else if(I>=0&&I<FrontlineMedalsV8215::Count)
  R.Grade=FrontlineMedalRibbonV8446::Presentation(I,PreviewGrade,FrontlineMedalRibbonV8446::EContext::SelectedClass).Grade;
 R.Earned=R.Grade>0&&PreviewGrade<=HighestGrade;return R;
}
namespace SelectedRibbonLayoutV8444
{
constexpr float DetailY=612,DetailW=172,DetailH=43,DetailMedalH=244;
constexpr float InspectY=841,InspectW=224,InspectH=56,InspectMedalH=664;
FBox2D Detail(float X,float W){const float Left=X+(W-DetailW)*.5f;return FBox2D(FVector2D(Left,DetailY),FVector2D(Left+DetailW,DetailY+DetailH));}
FBox2D Inspect(){return FBox2D(FVector2D(555,InspectY),FVector2D(555+InspectW,InspectY+InspectH));}
}
void SelectedRibbonV8444(const FFrontlineCanvas& UI,int32 I,int32 PreviewGrade,const FBox2D& Cell,const FrontlineMedalsV8215::FInspectionStateV84413* Inspection=nullptr)
{
 auto R=SelectedRibbonForGradesV8444(I,PreviewGrade,EntryTier(I));
 if(Inspection&&!IsDeed(I)){R.Grade=FrontlineMedalRibbonV8446::Presentation(I,Inspection->Form.Grade,FrontlineMedalRibbonV8446::EContext::SelectedClass,Inspection->Form).Grade;R.Earned=Inspection->Earned;}
 if(R.Grade<=0)
 {
  const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
  UI.RoleLabel(EFrontlineTextRoleV8210::Text,ZH?TEXT("本级无勋略"):TEXT("NO RIBBON BAR"),Cell.Min.X,Cell.Min.Y,Cell.GetSize().X,Cell.GetSize().Y,17,Muted,1);return;
 }
 if(IsDeed(I))R.Earned&=Deed(I).Available;
 const auto Size=Cell.GetSize();
 auto Draw=[&]
 {
  if(IsDeed(I))
  {
   static constexpr int32 Gates[]={1,5,15,40};auto D=FrontlineArmourValourV8221::Display(DeedNation(I),Gates[R.Grade-1]);
   D.Available=Deed(I).Available;ArmourKillTallyUIV8220::Ribbon(UI,D,Cell.Min.X,Cell.Min.Y,Size.X,Size.Y,false);
  }
  else
  {
   Ribbon(UI,I,Cell.Min.X,Cell.Min.Y,Size.X,Size.Y);
   if(Inspection)RibbonDevicesV8446(UI,I,FrontlineMedalRibbonV8446::Presentation(I,Inspection->Form.Grade,FrontlineMedalRibbonV8446::EContext::SelectedClass,Inspection->Form),Cell.Min.X,Cell.Min.Y,Size.X,Size.Y);
   else RibbonAttachments(UI,I,Cell.Min.X,Cell.Min.Y,Size.X,Size.Y,R.Grade,FrontlineMedalRibbonV8446::EContext::SelectedClass);
  }
 };
 if(R.Earned){Draw();return;}
 // Reuse the medal catalogue's accepted neutral locked material on every
 // textile/device tile, keeping the crop and alpha geometry unchanged.
 auto& A=Artwork();if(!A.MedalLockedMaterialV8217)return;
 TArray<FTooltipOpV83> Ops;auto* Previous=UI.RecordingV83;
 {TGuardValue<bool> Audit(V36TypographyAuditActive,false);UI.RecordingV83=&Ops;Draw();UI.RecordingV83=Previous;}
 for(const auto& O:Ops)if(O.Kind==ETooltipOpV83::Texture)
 {
  auto* T=Cast<UTexture2D>(O.Texture);if(!FrontlineTextureReady(T))continue;
  auto& M=A.GrayscalePhotos.FindOrAdd(T);
  if(!M||M->Parent!=A.MedalLockedMaterialV8217){M=UMaterialInstanceDynamic::Create(A.MedalLockedMaterialV8217,GetTransientPackage());M->SetTextureParameterValue(TEXT("Photo"),T);}
  UI.MaterialTile(M,O.X,O.Y,O.W,O.H,T->GetResource(),O.U,O.V,O.UW,O.VH);
 }
}
void RunRibbonUIChecksV8444(TFunctionRef<void(const FString&,bool)> Check)
{
 bool CellsExact=true,OutsideFree=true,Projection=true,NoFiction=true;
 for(int32 Count=1;Count<=36;++Count)
 {
  const auto Cells=RibbonBarLayoutV8218(Count,52,79,240,42,false);const auto Bounds=RibbonRackBoundsV8444(Cells);
  CellsExact&=Cells.Num()==Count&&Bounds.bIsValid;
  for(const auto& C:Cells)CellsExact&=Bounds.IsInside(C.GetCenter())&&C.Min.X>=52&&C.Max.X<=292&&C.Min.Y>=79&&C.Max.Y<=121;
  auto Hits=[&](FVector2D P){for(const auto& C:Cells)if(C.IsInside(P))return true;return false;};
  OutsideFree&=!Hits(FVector2D(51,100))&&!Hits(FVector2D(293,100))&&!Hits(FVector2D(172,122));
  if(Count>3&&Count%3)OutsideFree&=!Hits(FVector2D(Cells[Count-3*((Count+2)/3-1)].Min.X+.01,Cells[0].GetCenter().Y));
 }
 for(int32 I=0;I<FrontlineMedalsV8215::Count;++I)for(int32 G=1;G<=FrontlineMedalsV8215::Variants(I).Num();++G)
 {
  const auto Locked=SelectedRibbonForGradesV8444(I,G,0),Earned=SelectedRibbonForGradesV8444(I,G,G);
  const auto P=FrontlineMedalRibbonV8446::Presentation(I,G,FrontlineMedalRibbonV8446::EContext::SelectedClass);
  Projection&=Locked.Grade==P.Grade&&Locked.Grade==Earned.Grade&&!Locked.Earned
   &&(Earned.Grade==0||Earned.Earned);
  const bool NoBar=!FrontlineMedalsV8215::Definition(I).Ribbon||(I==3&&G==2)||(I==31&&G==1);
  if(NoBar)NoFiction&=Locked.Grade==0&&P.Devices.IsEmpty();
  else
  {
   NoFiction&=Locked.Grade==G;
   // A class may prescribe the George bow, Legion insignia or VC miniature.
   // Every other category stays plain without a supplied actual award record.
   if(I!=8&&I!=9&&I!=10&&I!=2)NoFiction&=P.Devices.IsEmpty();
  }
 }
 Check(TEXT("v8444_war_room_links_match_actual_textile_cells"),CellsExact);
 Check(TEXT("v8444_unused_rack_aperture_and_short_row_do_not_block_map"),OutsideFree);
 Check(TEXT("v8446_detail_inspect_share_selected_class_and_locked_earned_state"),Projection);
 Check(TEXT("v8446_selected_class_respects_catalogue_ribbon_flag_and_has_no_unconfigured_grade_devices"),NoFiction);
 Check(TEXT("v8446_selected_class_keeps_known_cloth_except_confirmed_no_bar_forms"),SelectedRibbonForGradesV8444(3,2,2).Grade==0
  &&SelectedRibbonForGradesV8444(31,1,1).Grade==0&&SelectedRibbonForGradesV8444(3,3,3).Grade==3
  &&SelectedRibbonForGradesV8444(18,3,3).Grade==3&&!SelectedRibbonForGradesV8444(18,3,1).Earned
  &&SelectedRibbonForGradesV8444(20,6,6).Grade==6&&SelectedRibbonForGradesV8444(31,3,3).Grade==3);
 Check(TEXT("v8446_legion_selected_class_preserves_all_five_ribbon_grade_markers"),SelectedRibbonForGradesV8444(8,3,0).Grade==3&&SelectedRibbonForGradesV8444(8,5,0).Grade==5);
 const auto Detail=SelectedRibbonLayoutV8444::Detail(1154,674),Inspect=SelectedRibbonLayoutV8444::Inspect();
 Check(TEXT("v8444_single_ribbon_geometry_is_four_to_one_and_clears_existing_controls"),FMath::IsNearlyEqual(float(Detail.GetSize().X/Detail.GetSize().Y),4.f)
  &&FMath::IsNearlyEqual(float(Inspect.GetSize().X/Inspect.GetSize().Y),4.f)&&Detail.Min.Y>=366+SelectedRibbonLayoutV8444::DetailMedalH+2
  &&Detail.Max.Y<=667-12&&Inspect.Min.Y>=151+SelectedRibbonLayoutV8444::InspectMedalH+26&&Inspect.Max.Y<=929-32);
 bool Names=true;for(int32 N=0;N<4;++N){int32 ParsedN=INDEX_NONE,I=INDEX_NONE;Names&=ParseWarRoomRibbonV8444(WarRoomRibbonNameV8444(EFRRUNationV81(N),100+N),ParsedN,I)&&ParsedN==N&&I==100+N;}
 int32 N=0,I=0;Names&=!ParseWarRoomRibbonV8444(TEXT("V8444WarRoomMedalWall4_18"),N,I)
  &&!ParseWarRoomRibbonV8444(TEXT("V8444WarRoomMedalWall1_18_invalid"),N,I)&&!ParseWarRoomRibbonV8444(TEXT("V52MapSurface"),N,I);
 Check(TEXT("v8444_war_room_link_names_roundtrip_and_reject_malformed_routes"),Names);
}
void Inspect(const FFrontlineCanvas& UI,FView& S,bool ZH)
{
 const int32 VariantCount=EntryVariants(S.Focus);
 if(S.PreviewStateV84413<1||S.PreviewStateV84413>VariantCount||InspectionStateV84413(S.Focus,S.PreviewStateV84413).Form.Grade!=S.PreviewGrade)
  SelectInspectionPositionV84413(S,InspectionPositionV84413(S.Focus,S.PreviewGrade));
 const auto State=InspectionStateV84413(S.Focus,S.PreviewStateV84413);
 UI.Rect(0,0,1920,1080,FLinearColor(.008f,.012f,.012f,.80f));UI.Hit(0,0,1920,1080,TEXT("V8216MedalInspectBack"),100);
 UI.PageSurfaceV73(320,94,1280,890,false,false,false);UI.Hit(320,94,1280,890,TEXT("Blocked"),110);
 Decoration(UI,S.Focus,State.Form.Grade,358,151,618,SelectedRibbonLayoutV8444::InspectMedalH,State.Earned,IsDeed(S.Focus)?nullptr:&State.Form);
 SelectedRibbonV8444(UI,S.Focus,State.Form.Grade,SelectedRibbonLayoutV8444::Inspect(),&State);
 NameLabel(UI,S.Focus,ZH,1008,150,550,110,ZH?32:35,Paper,0);
 UI.ConsoleSeamV79(1008,280,1556,280,Border,1);
 UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,History(S.Focus,ZH),1008,304,550,180,23,Muted);
 UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,Rule(S.Focus,ZH),1008,515,550,176,21,Paper);
 if(VariantCount>0)UI.RoleLabel(EFrontlineTextRoleV8210::Inscription,IsDeed(S.Focus)?VariantLabel(S.Focus,State.Form.Grade,ZH):FrontlineMedalsV8215::InspectionLabelV84413(S.Focus,State.Form,ZH),1008,713,550,67,ZH?23:25,Amber,1);
 const bool NationOpen=IsDeed(S.Focus)?Deed(S.Focus).Available:FrontlineMedalsV8215::Available(FrontlineMedalsV8215::Definition(S.Focus).Nation);
 const FString FormStatus=State.Earned?AwardLabel(S.Focus,ZH):!NationOpen?
  (ZH?TEXT("暂未开放"):TEXT("NOT YET AVAILABLE")):(ZH?TEXT("此式尚未获授"):TEXT("THIS FORM HAS NOT BEEN AWARDED"));
 UI.RoleLabel(EFrontlineTextRoleV8210::Text,FormStatus,1008,794,550,64,21,Muted,1);
 if(S.PreviewStateV84413>1)UI.PageButtonV73(1010,864,258,44,WarRoom52::L(TEXT("PREVIOUS"),TEXT("上一式")),TEXT(""),TEXT("V8216MedalVariantPrev"),true,false,120);
 if(S.PreviewStateV84413<VariantCount)UI.PageButtonV73(1294,864,262,44,WarRoom52::L(TEXT("NEXT"),TEXT("下一式")),TEXT(""),TEXT("V8216MedalVariantNext"),true,false,120);
 UI.PageButtonV73(1302,929,254,40,WarRoom52::L(TEXT("RETURN"),TEXT("返回")),TEXT("Esc"),TEXT("V8216MedalInspectBack"),true,false,120);
}
void RunInspectionUIChecksV84413(APlayerController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=PC&&(FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))||FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused")))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
 Check(TEXT("v84413_museum_navigation_checks_require_disposable_nosave_profile"),Safe);if(!Safe)return;
 const auto Before=View(PC);ON_SCOPE_EXIT{View(PC)=Before;};
 bool Routes=true,NoWrites=true;
 for(int32 I=0;I<FrontlineMedalsV8215::Count;++I)
 {
   if(I==4)continue;const auto Prior=FrontlineMedalsV8215::Award(I);const auto RawPrior=FrontlineAchievementsV76::MedalAwardV8216(FrontlineMedalsV8215::Definition(I).Id);const auto States=FrontlineMedalsV8215::InspectionStatesV84413(I,Prior);
  auto& S=View(PC);S.Wall=true;S.Nation=int32(FrontlineMedalsV8215::Definition(I).Nation);
  Hit(PC,FName(*FString::Printf(TEXT("V8216MedalSelect%d"),I)));Routes&=S.Focus==I&&S.PreviewStateV84413==InspectionPositionV84413(I);
  SelectInspectionPositionV84413(S,1);Hit(PC,TEXT("V8216MedalVariantPrev"));Routes&=S.PreviewStateV84413==1;
  for(int32 K=0;K<States.Num();++K)
  {const auto Current=InspectionStateV84413(I,S.PreviewStateV84413);Routes&=S.PreviewGrade==Current.Form.Grade&&S.PreviewStateV84413==K+1;Hit(PC,TEXT("V8216MedalVariantNext"));}
  Routes&=S.PreviewStateV84413==States.Num();
  for(int32 K=States.Num()-1;K>=0;--K)
  {const auto Current=InspectionStateV84413(I,S.PreviewStateV84413);Routes&=S.PreviewGrade==Current.Form.Grade&&S.PreviewStateV84413==K+1;Hit(PC,TEXT("V8216MedalVariantPrev"));}
  Routes&=S.PreviewStateV84413==1;
   const auto After=FrontlineAchievementsV76::MedalAwardV8216(FrontlineMedalsV8215::Definition(I).Id);NoWrites&=RawPrior.Grade==After.Grade&&RawPrior.Repeats==After.Repeats&&RawPrior.Attachments==After.Attachments&&RawPrior.Date==After.Date&&RawPrior.Campaign==After.Campaign&&RawPrior.CampaignDeeds==After.CampaignDeeds;
 }
  bool RepeatBoundary=true;
  for(int32 I:{0,1,2,12,13,37,38,39})for(int32 R:{0,4,5,6,255})
  {
   const int32 Presented=FrontlineMedalsV8215::PresentedRepeatsV85(I,R);
   RepeatBoundary&=SelectInspectionStateV84413(PC,I,1,R,R?uint16(FrontlineMedalsV8215::EAttachment::Bar):0)
    &&View(PC).PreviewStateV84413==Presented+1&&InspectionStateV84413(I,View(PC).PreviewStateV84413).Form.Repeats==Presented;
  }
  Check(TEXT("v85_museum_repeat_boundaries_and_legacy_requests_select_complete_capped_preview"),RepeatBoundary);
 for(int32 N=0;N<4;++N)
 {
  const int32 I=DeedBaseV8222+N,C=EntryVariants(I);const auto Prior=Deed(I);auto& S=View(PC);S.Nation=N;S.Focus=I;
  SelectInspectionPositionV84413(S,1);Hit(PC,TEXT("V8216MedalVariantPrev"));Routes&=S.PreviewStateV84413==1;
  for(int32 K=1;K<=C;++K){Routes&=S.PreviewStateV84413==K&&S.PreviewGrade==K;Hit(PC,TEXT("V8216MedalVariantNext"));}
  Routes&=S.PreviewStateV84413==C;
  for(int32 K=C;K>=1;--K){Routes&=S.PreviewStateV84413==K&&S.PreviewGrade==K;Hit(PC,TEXT("V8216MedalVariantPrev"));}
  Routes&=S.PreviewStateV84413==1;const auto After=Deed(I);NoWrites&=Prior.Total==After.Total&&Prior.Distinctions==After.Distinctions;
 }
 for(int32 N=0;N<4;++N){Hit(PC,FName(*FString::Printf(TEXT("V8215MedalNation%d"),N)));const auto& S=View(PC);Routes&=S.Nation==N&&S.PreviewStateV84413==InspectionPositionV84413(S.Focus)&&S.PreviewGrade==InspectionStateV84413(S.Focus,S.PreviewStateV84413).Form.Grade;}
 Check(TEXT("v85_museum_all_real_forms_reachable_in_both_directions_without_endpoint_wrap"),Routes);
 Check(TEXT("v84413_navigation_never_changes_awards_progress_or_campaign_receipts"),NoWrites);
}
void RunInspectionUIChecksV84415(APlayerController* PC,TFunctionRef<void(const FString&,bool)> Check)
{RunInspectionUIChecksV84413(PC,Check);FrontlineMedalsV8215::RunInspectionChecksV84413(Check);RunPresentationChecksV84415(Check);FrontlineMedalRibbonV8446::RunChecks(Check);}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 auto& S=View(UI.HUD.GetOwningPlayerController());const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.PageSurfaceV73(0,0,1920,1080,false,false,false);UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
 UI.BilingualLabel(TEXT("HONOURS"),TEXT("战功"),92,51,1250,67,50,Paper,0,EFrontlineTextRoleV8210::Inscription);UI.ConsoleSeamV79(92,137,1828,137,Border,2);Tabs(UI);
 static const TCHAR* EN[]={TEXT("BRITAIN"),TEXT("GERMANY"),TEXT("FRANCE"),TEXT("RUSSIA")};static const TCHAR* CN[]={TEXT("英国"),TEXT("德国"),TEXT("法国"),TEXT("俄国")};
 for(int32 N=0;N<4;++N)UI.PageTabV73(92+N*440,209,416,ZH?CN[N]:EN[N],FName(*FString::Printf(TEXT("V8215MedalNation%d"),N)),S.Nation==N,true,51);
 const auto A=Items(EFRRUNationV81(S.Nation));if(A.IsEmpty())return;if(!A.Contains(S.Focus)){S.Focus=A[0];SelectInspectionPositionV84413(S,InspectionPositionV84413(S.Focus));}
 if(S.PreviewStateV84413<1||S.PreviewStateV84413>EntryVariants(S.Focus)||InspectionStateV84413(S.Focus,S.PreviewStateV84413).Form.Grade!=S.PreviewGrade)
  SelectInspectionPositionV84413(S,InspectionPositionV84413(S.Focus,S.PreviewGrade));
 PreloadVariants(A,false);
 if(S.ScrollNation!=S.Nation){S.Catalogue.Reset(S.Row*162.f);S.Gallery.Reset(S.ExhibitRowV8311*ExhibitLayoutV8311::SlotH);S.ScrollNation=S.Nation;}
 if(!S.Catalogue.Ready)S.Catalogue.Reset(S.Row*162.f);if(!S.Gallery.Ready)S.Gallery.Reset(S.ExhibitRowV8311*ExhibitLayoutV8311::SlotH);
 S.Catalogue.Advance(FPlatformTime::Seconds(),FMath::Max(0,(A.Num()+2)/3-2)*162.f,UI.MotionStableV83);
 // One museum surface contains every actual class already awarded, including
 // earlier classes. Pin and sash sets are exhibits, never fictional hangers.
 UI.ImageContain(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8217/T_MedalDisplayWall_V8217.T_MedalDisplayWall_V8217")),92,278,1017,339,FLinearColor(.94f,.95f,.92f));
 const auto Earned=Exhibits(EFRRUNationV81(S.Nation));
 RibbonBarV8218(UI,EFRRUNationV81(S.Nation),126,ExhibitLayoutV8311::RackTop,949,ExhibitLayoutV8311::RackH,true,!S.Inspect);
 S.Gallery.Advance(FPlatformTime::Seconds(),ExhibitLayoutV8311::Maximum(Earned.Num())*ExhibitLayoutV8311::SlotH,UI.MotionStableV83);
 MuseumViewportV842(UI,FBox2D(FVector2D(126,390),FVector2D(1075,580)),[&]{
 for(int32 K=0;K<Earned.Num();++K)
 {
  const int32 Row=K/ExhibitLayoutV8311::Columns;auto Cell=ExhibitLayoutV8311::Cell(K,Row,Earned.Num());
  const float Y=ExhibitLayoutV8311::Top+Row*ExhibitLayoutV8311::SlotH-S.Gallery.Offset;
  if(Y>=580||Y+ExhibitLayoutV8311::SlotH<=390)continue;Cell.Min.Y=Y;Cell.Max.Y=Y+ExhibitLayoutV8311::SlotH;
  const int32 I=Earned[K].X,G=Earned[K].Y;
  Decoration(UI,I,G,Cell.Min.X+ExhibitLayoutV8311::ArtX,Cell.Min.Y+ExhibitLayoutV8311::ArtY,ExhibitLayoutV8311::ArtW,ExhibitLayoutV8311::ArtH,true);
  UI.Hit(Cell.Min.X,Cell.Min.Y,ExhibitLayoutV8311::SlotW,ExhibitLayoutV8311::SlotH,FName(*FString::Printf(TEXT("V8217MedalExhibit%d_%d"),I,G)),56);
 }
 });
 if(Earned.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Annotation,WarRoom52::L(TEXT("No decorations awarded."),TEXT("尚未获授勋章。")),136,405,929,74,25,Muted,1);
 const int32 ExhibitPages=ExhibitLayoutV8311::Maximum(Earned.Num())+1;
 if(ExhibitPages>1)
 {
  const float TrackTop=ExhibitLayoutV8311::Top+4,TrackH=ExhibitLayoutV8311::SlotH-8;
  const float ThumbH=TrackH/ExhibitPages,ThumbY=TrackTop+(TrackH-ThumbH)*S.Gallery.Offset/FMath::Max(1.f,(ExhibitPages-1)*ExhibitLayoutV8311::SlotH);
  UI.Line(ExhibitLayoutV8311::TrackX,TrackTop,ExhibitLayoutV8311::TrackX,TrackTop+TrackH,FLinearColor(.025f,.030f,.027f),4);
  UI.Line(ExhibitLayoutV8311::TrackX,ThumbY,ExhibitLayoutV8311::TrackX,ThumbY+ThumbH,FLinearColor(.38f,.34f,.24f),4);
 }
 MuseumViewportV842(UI,FBox2D(FVector2D(92,652),FVector2D(1109,964)),[&]{
 for(int32 P=0;P<A.Num();++P)
 {
  const int32 I=A[P];const float X=92+(P%3)*345,Y=652+(P/3)*162-S.Catalogue.Offset,W=327,H=150;
  if(Y>=964||Y+H<=652)continue;const bool Held=EntryTier(I)>0,Focus=S.Focus==I;
  UI.Skin(2,X,Y,W,H,7,Focus?FLinearColor(.83f,.81f,.72f):FLinearColor(.62f,.66f,.63f));Icon(UI,I,X+15,Y+13,94,112,Held);
  NameLabel(UI,I,ZH,X+123,Y+25,W-138,94,ZH?20:18,Held?Paper:Muted,1);
  UI.Hit(X,Y,W,H,FName(*FString::Printf(TEXT("V8216MedalSelect%d"),I)),55);
 }
 });
 const float PX=1154,PW=674;UI.Skin(2,PX,278,PW,700,8,FLinearColor(.64f,.67f,.63f));
 NameLabel(UI,S.Focus,ZH,PX+28,296,PW-56,58,ZH?29:31,Paper,1);
 const auto DetailState=InspectionStateV84413(S.Focus,S.PreviewStateV84413);const int32 DetailGrade=DetailState.Form.Grade;
 const float PreviewScaleV84=.93f,PreviewW=362*PreviewScaleV84,PreviewSlotH=SelectedRibbonLayoutV8444::DetailMedalH,PreviewH=PreviewSlotH*PreviewScaleV84;
 Decoration(UI,S.Focus,DetailGrade,PX+(PW-PreviewW)*.5f,366+(PreviewSlotH-PreviewH)*.5f,PreviewW,PreviewH,DetailState.Earned,IsDeed(S.Focus)?nullptr:&DetailState.Form);
 SelectedRibbonV8444(UI,S.Focus,DetailGrade,SelectedRibbonLayoutV8444::Detail(PX,PW),&DetailState);
 UI.PageButtonV73(PX+188,667,298,41,WarRoom52::L(TEXT("INSPECT"),TEXT("检视")),TEXT(""),TEXT("V8216MedalInspect"),true,false,60);
 UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,History(S.Focus,ZH),PX+28,720,PW-56,90,19,Muted);
 UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,Rule(S.Focus,ZH),PX+28,818,PW-56,94,18,Paper);
 const FString AwardCopy=AwardLabel(S.Focus,ZH),AwardPrefix=AwardCopy+TEXT(" · ");FString ProgressCopy=Progress(S.Focus,ZH);
 FString SelectedCopy=IsDeed(S.Focus)?AwardCopy:FrontlineMedalsV8215::InspectionLabelV84413(S.Focus,DetailState.Form,ZH);
 if(!IsDeed(S.Focus))
 {
  if(!DetailState.Earned)SelectedCopy+=FrontlineMedalsV8215::Available(FrontlineMedalsV8215::Definition(S.Focus).Nation)?
   (ZH?TEXT(" · 尚未授予"):TEXT(" · NOT AWARDED")):(ZH?TEXT(" · 暂未开放"):TEXT(" · NOT YET AVAILABLE"));
  else if(FrontlineAchievementsV76::CheatGrantedMedalV8218(FrontlineMedalsV8215::Definition(S.Focus).Id))SelectedCopy+=ZH?TEXT(" · 作弊授予"):TEXT(" · CHEAT");
 }
 // The detail panel owns status and progress; drop only the repeated award
 // prefix, retaining service dates, counts and the next distinction condition.
 if(ProgressCopy==AwardCopy)ProgressCopy.Reset();
 else if(ProgressCopy.StartsWith(AwardPrefix))ProgressCopy.RightChopInline(AwardPrefix.Len());
 UI.RoleLabel(EFrontlineTextRoleV8210::Text,ProgressCopy,PX+28,920,PW-56,24,19,EntryTier(S.Focus)>0?Amber:Muted,1);
 UI.RoleLabel(EFrontlineTextRoleV8210::Text,SelectedCopy,PX+28,948,PW-56,24,19,DetailState.Earned?Amber:Muted,1);
 UI.ConsoleSeamV79(92,983,1828,983,Border*.38f);UI.PageButtonV73(1320,997,508,44,WarRoom52::L(TEXT("RETURN"),TEXT("返回")),TEXT("Esc"),TEXT("AchievementsBackV76"));
 if(S.Inspect)Inspect(UI,S,ZH);
}
}
void DrawFactionRibbonV8215(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,EFRRUNationV81 N,float X,float Y,float W,float H)
{
 const bool Room=B&&B->Phase==EFrontlinePhase::Briefing&&WarRoom52::Active(Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController()));
 MedalWallV8215::RibbonBarV8218(UI,N,X,Y,W,H,false,B&&B->Phase==EFrontlinePhase::Briefing,Room);
}
void DrawFactionRibbonV8215(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,EFrontlineFaction N,float X,float Y,float W,float H)
{DrawFactionRibbonV8215(UI,B,EFRRUNationV81(uint8(N)),X,Y,W,H);}
