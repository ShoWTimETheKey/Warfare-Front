#include "WarfareFrontlineArmourValourV8221.h"
namespace ArmourKillTallyUIV8220
{
struct FView
{
 FrontlineArmourKillTallyV8220::FNotification Record;
 double Began=-1;
};
TMap<TWeakObjectPtr<APlayerController>,FView> Views;
UTexture2D* Badge(EFRRUNationV81 Nation)
{
 const FString Path=FrontlineArmourValourV8221::BadgePath(Nation);
 // A national sign never falls back to another army's emblem.
 return Path.IsEmpty()?nullptr:FrontlineUIAssetsV8213::Texture(Path);
}
void Ribbon(const FFrontlineCanvas& UI,const FrontlineArmourValourV8221::FDisplay& D,float X,float Y,float W,float H,bool IncludeSpecimens=true)
{
 const int32 N=int32(D.Nation);if(N<0||N>3||W<=0||H<=0)return;
 // Every rack cell uses the same 4:1 opaque textile, with no alpha gutters.
 // Native centre crops preserve the weave without stretching or resampling.
 auto* Cloth=FrontlineUIAssetsV8213::Texture(FrontlineArmourValourV8221::RibbonPath(D.Nation));
 const auto Cut=FrontlineArmourValourV8221::RibbonTextileCutV837(D.Nation);
 const auto Source=FrontlineArmourValourV8221::RibbonSourceSizeV837(D.Nation);
 if(!Cloth||Cloth->GetSizeX()!=Source.X||Cloth->GetSizeY()!=Source.Y)return;
 const float RW=FMath::Min(W,H*4.f),RH=RW*.25f,RY=Y+(H-RH)*.5f,RX=X+(W-RW)*.5f;
 const FLinearColor Tint=D.Available&&D.Distinctions>0?FLinearColor(.91f,.92f,.87f):FLinearColor(.32f,.34f,.31f);
 UI.Image(Cloth,RX,RY,RW,RH,Tint,Cut.Min.X/float(Source.X),Cut.Min.Y/float(Source.Y),Cut.Width()/float(Source.X),Cut.Height()/float(Source.Y),false,true);
 if(D.Distinctions<1)return;
 auto Device=[&](const TCHAR* Asset,float DX,float DY,float DW,float DH,FLinearColor Ink=FLinearColor(.91f,.92f,.86f))
 {UI.ImageContain(FrontlineUIAssetsV8213::Texture(Asset),DX,DY,DW,DH,Ink);};
 using E=FrontlineArmourValourV8221::EDevice;
 if(D.Device==E::SilverRosettes)
 {
  const int32 Repeats=D.Distinctions-1;const float Size=RH*.80f;
  for(int32 K=0;K<Repeats;++K)Device(TEXT("/Game/Warfare/UI/V8216/T_MedalAttachment_SilverRosette_V8216.T_MedalAttachment_SilverRosette_V8216"),RX+RW*(K+1)/(Repeats+1)-Size*.5f,RY+(RH-Size)*.5f,Size,Size);
 }
 else if(D.Device==E::IronCrossPin&&D.Distinctions>=2)
 {
  Device(TEXT("/Game/Warfare/UI/V8216/T_MedalAttachment_OakLeaves_V8216.T_MedalAttachment_OakLeaves_V8216"),RX+RW*.39f,RY+RH*.08f,RW*.22f,RH*.84f);
 }
 else if(D.Device==E::CitationDevices)
 {
  const float Size=RH*.76f;const int32 Count=D.Distinctions;
  for(int32 K=0;K<Count;++K)
  {
   const float CX=RX+RW*(K+1)/(Count+1);
   if(K==3)Device(TEXT("/Game/Warfare/UI/V8216/T_MedalAttachment_Palm_V8216.T_MedalAttachment_Palm_V8216"),CX-Size*.76f,RY+(RH-Size)*.5f,Size*1.52f,Size);
   else Device(TEXT("/Game/Warfare/UI/V8221/T_CitationStarV8221.T_CitationStarV8221"),CX-Size*.5f,RY+(RH-Size)*.5f,Size,Size,
     K==0?FLinearColor(.69f,.45f,.28f):K==1?FLinearColor(.91f,.92f,.87f):FLinearColor(.90f,.67f,.29f));
  }
 }
 else if(D.Device==E::GeorgeClass)
 {
  if(D.Distinctions==2||D.Distinctions==4)Device(TEXT("/Game/Warfare/UI/V8221/T_StGeorgeBowV8221.T_StGeorgeBowV8221"),RX+RW*.27f,RY+RH*.04f,RW*.46f,RH*.92f);
 }
}
bool Banner(const FFrontlineCanvas& UI,APlayerController* P,const FFrontlineScreenState& S)
{
 auto& V=Views.FindOrAdd(P);const double Now=FPlatformTime::Seconds();
 if(V.Began<0)
 {
  if(S.AchievementToastV81!=INDEX_NONE||S.MedalToastV8217!=INDEX_NONE)return false;
  V.Record=FrontlineArmourKillTallyV8220::TakeNotification();
  if(!V.Record.Receipt.IsValid()||V.Record.Total<=0)return false;
  V.Began=Now;
 }
 const float Age=float(Now-V.Began);
 if(Age>=6.5f){V.Began=-1;return false;}
 const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 const auto Motion=FrontlineAwardMotionV8217::Sample(Age,UI.MotionStableV83);
 const float X=1260.f+Motion.OffsetX,Y=92.f,W=588.f,H=148.f;
 UI.Skin(0,X,Y,W,H,16);
 const float Size=88.f*Motion.StampScale,Inset=(Size-88.f)*.5f;
 if(Motion.StampAlpha>0)UI.ImageContain(Badge(V.Record.Nation),X+22-Inset,Y+30-Inset+Motion.StampOffsetY,Size,Size,
  FLinearColor(Motion.StampBrightness,Motion.StampBrightness,Motion.StampBrightness,Motion.StampAlpha));
 const auto D=FrontlineArmourValourV8221::Display(V.Record.Nation,V.Record.Total);
 UI.RoleLabel(EFrontlineTextRoleV8210::Headline,FrontlineArmourValourV8221::BannerTitle(ZH),X+126,Y+16,W-152,36,ZH?24:20,Paper);
 Ribbon(UI,D,X+126,Y+65,119,30);
 UI.RoleLabel(EFrontlineTextRoleV8210::Headline,ZH?FString::Printf(TEXT("%d 笔战绩"),V.Record.Total):FString::Printf(TEXT("%d CONFIRMED"),V.Record.Total),X+288,Y+61,W-314,41,ZH?26:25,Paper);
 UI.RoleLabel(EFrontlineTextRoleV8210::Text,FrontlineArmourValourV8221::RibbonLabel(D,ZH),X+126,Y+109,W-152,24,17,Muted);
 return true; // Decorative; no hitbox, pause, input capture or duplicate grant.
}
}
