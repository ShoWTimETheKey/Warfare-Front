#include "WarfareFrontlineMedalsV8215.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineArmourKillTallyV8220.h"
#include "WarfareFRRUCampaignV82.h"
namespace FrontlineMedalsV8215
{
namespace
{
#include "WarfareFrontlineMedalCatalogueV8216.generated.inl"
bool RepeatBars(int32 I){return RepeatLimitV85(I)>0;}
// These bars recognise a new qualifying battle, not a rising career counter.
bool BattleDeedBarsV84414(int32 I){return I==37||I==39;}
uint16 AllowedAttachments(int32 I)
{
 if(RepeatBars(I))return uint16(EAttachment::Bar);
 if(I>=37)return 0;
 if(I==5||I==17)return uint16(EAttachment::OakLeaves);
 if(I==6)return uint16(EAttachment::Palm)|uint16(EAttachment::Star);
 if(I==14)return uint16(EAttachment::Clasp)|uint16(EAttachment::Rosette);
 if(I==3||I==11||I==15||I==16||I==21||I==22||I==23||I==24||I==25||I==28||I==29||I==30||I==36)return 0;
 if(I==8)return uint16(EAttachment::Rosette);
 if(I==9||I==10)return uint16(EAttachment::Bow);
 if(I==19||I==20)return uint16(EAttachment::Swords)|uint16(EAttachment::Crown);
 if(I==26||I==27)return uint16(EAttachment::Clasp);
 if(I==33)return uint16(EAttachment::Swords)|uint16(EAttachment::Bow);
 return uint16(EAttachment::Swords);
}
int32 StatValue(const FDefinition& D,const FStats& S)
{return D.Metric==0?S.Infantry:D.Metric==1?S.Armour:D.Metric==2?S.Victories:D.Metric==3?S.CleanVictories:D.Metric==6?0:D.Metric==5?(S.NaturalRoute&2?1:0):(S.NaturalRoute&1?1:0);}
int32 TargetForStage(int32 I,int32 Stage)
{
 static const int32 Bavarian[]={12,24,40,65,100,150};
 static const int32 Legion[]={0,5,10,15,20},GeorgeMedal[]={0,600,1200,1800},GeorgeCross[]={0,4,8,12},GeorgeOrder[]={0,6,12};
 static const int32 Anne[]={0,800,1600,2400},Stanislaus[]={0,6,12},Vladimir[]={0,1200,2400,3600};
 if(I==8)return Legion[FMath::Clamp(Stage,0,4)];if(I==9)return GeorgeMedal[FMath::Clamp(Stage,0,3)];
 if(I==10)return GeorgeCross[FMath::Clamp(Stage,0,3)];if(I==11)return GeorgeOrder[FMath::Clamp(Stage,0,2)];
 if(I==31)return Anne[FMath::Clamp(Stage,0,3)];if(I==32)return Stanislaus[FMath::Clamp(Stage,0,2)];if(I==33)return Vladimir[FMath::Clamp(Stage,0,3)];
 if(BattleDeedBarsV84414(I)||Definition(I).Metric==6)return 0;
 if(RepeatBars(I)&&I!=2&&Stage>=3){const auto& D=Definition(I);return D.Threshold[2]+(Stage-2)*(D.Threshold[2]-D.Threshold[1]);}
 if(I==20)return Stage<UE_ARRAY_COUNT(Bavarian)?Bavarian[FMath::Max(0,Stage)]:0;
 return Definition(I).Threshold[FMath::Clamp(Stage,0,2)];
}
}
bool Available(EFRRUNationV81 N){return N==EFRRUNationV81::British||N==EFRRUNationV81::German
 ||WarfareFRRUCampaignV82::CampaignsAvailable(N);}
const FDefinition& Definition(int32 I){return Catalog[FMath::Clamp(I,0,Count-1)];}
FStats Stats(EFrontlineFaction F){return FrontlineAchievementsV76::MedalStatsV8215(F);}
FStats Stats(EFRRUNationV81 N){return Available(N)?FrontlineAchievementsV76::NationalMedalStatsV87(N):FStats{};}
int32 Value(int32 I){return StatValue(Definition(I),Stats(Definition(I).Nation));}
FAward PresentedAwardV85(int32 I,const FAward& Recorded)
{auto A=Recorded;A.Repeats=uint8(PresentedRepeatsV85(I,Recorded.Repeats));return A;}
FAward Award(int32 I){return PresentedAwardV85(I,I!=4&&(Available(Definition(I).Nation)||FrontlineAchievementsV76::CheatGrantedMedalV8218(Definition(I).Id))?FrontlineAchievementsV76::MedalAwardV8216(Definition(I).Id):FAward{});}
int32 Tier(int32 I){return Award(I).Grade;}
int32 NextTargetForAwardV85(int32 I,const FAward& A)
{
 const int32 Step=RepeatBars(I)?int32(A.Repeats)+(A.Grade?1:0):int32(A.Grade);
 const int32 Max=RepeatBars(I)?RepeatLimitV85(I)+1:I==11?3:Variants(I).Num();if(Step>=Max)return 0;
 return TargetForStage(I,Step);
}
int32 NextTarget(int32 I){return NextTargetForAwardV85(I,Award(I));}
TArray<int32> NationIndices(EFRRUNationV81 N)
{
 switch(N)
 {case EFRRUNationV81::British:return {0,1,2,12,13,14,15,16,17,37,38,39,40};
 case EFRRUNationV81::German:return {3,5,18,19,20,21,22,23,24,41,42,43,44};
 case EFRRUNationV81::French:return {6,7,8,25,26,27,28,29,30,45,46,47,48};
 case EFRRUNationV81::Russian:return {9,10,11,31,32,33,34,35,36,49,50,51,52};default:return {};}
}
TArray<FVariant> Variants(int32 I)
{
 switch(I){
#include "WarfareFrontlineMedalVariantsV8216.generated.inl"
 default:return {};}
}
FString VariantArtworkPath(int32 I,int32 Grade)
{
 const auto V=Variants(I);if(V.IsEmpty())return {};
 const FString Stem=V[FMath::Clamp(Grade-1,0,V.Num()-1)].ArtworkStem;
 return FString(I==18?TEXT("/Game/Warfare/UI/V85/Medals/"):I>=37?TEXT("/Game/Warfare/UI/V84413/Honours/"):TEXT("/Game/Warfare/UI/V8216/"))+Stem+TEXT(".")+Stem;
}
FString ArtworkPath(int32 I){return VariantArtworkPath(I,FMath::Max(1,Tier(I)));}
FString RibbonArtworkPath(int32 I){return VariantArtworkPath(I,I==3?1:FMath::Max(1,Tier(I)));}
FString ArtworkPath(){return TEXT("/Game/Warfare/UI/V8215/T_MedalAtlasV8215.T_MedalAtlasV8215");}
int32 IndexForId(const FString& Id){for(int32 I=0;I<Count;++I)if(Id==Definition(I).Id)return I;return INDEX_NONE;}
bool ValidAward(int32 I,const FAward& A)
{
 // Storage compatibility: larger valid legacy records remain readable and
 // saveable. Only new grants and presentation use each sourced game boundary.
 if(I<0||I>=Count||I==4||A.Grade<1||A.Grade>Variants(I).Num()||A.Repeats>(I==2?1:RepeatBars(I)?255:0))return false;
 if((A.Attachments&~AllowedAttachments(I))!=0)return false;
 if((I==2||I==5)&&!A.Campaign.IsValid())return false;
 if(A.CampaignDeeds.Num()>3)return false;TSet<FGuid> Deeds;for(const auto& Id:A.CampaignDeeds){if(!Id.IsValid()||Deeds.Contains(Id))return false;Deeds.Add(Id);}
 if((I==2||I==5||I==21)&&(A.CampaignDeeds.IsEmpty()||!Deeds.Contains(A.Campaign)))return false;
 if((I==2||I==5)&&A.CampaignDeeds.Num()>2)return false;
 if(RepeatBars(I)&&((A.Repeats>0)!=(bool(A.Attachments&uint16(EAttachment::Bar)))))return false;
 FDateTime Date;return A.Date.Len()<=40&&FDateTime::ParseIso8601(*A.Date,Date);
}
FString AwardLabel(int32 I,bool ZH)
{
 const auto A=Award(I);const bool Cheat=FrontlineAchievementsV76::CheatGrantedMedalV8218(Definition(I).Id);if(!Available(Definition(I).Nation)&&!Cheat)return ZH?TEXT("暂未开放"):TEXT("NOT YET AVAILABLE");
 if(!A.Grade)return ZH?TEXT("尚未授予"):TEXT("NOT AWARDED");
 const auto V=Variants(I);FString Label=V.IsValidIndex(A.Grade-1)?FString(ZH?V[A.Grade-1].NameZH:V[A.Grade-1].NameEN):FString();
 if(A.Repeats>0)Label+=ZH?FString::Printf(TEXT(" · %d道复授横杠"),A.Repeats):FString::Printf(TEXT(" · %d AWARD BAR%s"),A.Repeats,A.Repeats>1?TEXT("S"):TEXT(""));
 if(A.Attachments&uint16(EAttachment::OakLeaves))Label+=ZH?TEXT(" · 橡叶"):TEXT(" · OAK LEAVES");if(Cheat)Label+=ZH?TEXT(" · 作弊授予"):TEXT(" · CHEAT");return Label;
}
FAward CompleteCheatAwardV8218(int32 I,const FGuid& Campaign,const FString& Date)
{
 FAward A;if(I<0||I>=Count||I==4||!Campaign.IsValid())return A;const auto Forms=Variants(I);if(Forms.IsEmpty())return A;
 A.Grade=uint8(Forms.Num());A.Date=Date;A.Campaign=Campaign;
 if(I==2||I==5||I==21)A.CampaignDeeds.Add(Campaign);
 // Grant one genuine repeat attachment, never a made-up service level or an
 // arbitrary combination of every decoration the category could ever carry.
 if(RepeatBars(I)){A.Repeats=1;A.Attachments=uint16(EAttachment::Bar);}
 else if(I==5||I==17)A.Attachments=uint16(EAttachment::OakLeaves);
 else if(I==6)A.Attachments=uint16(EAttachment::Palm);
 else if(I==14)A.Attachments=uint16(EAttachment::Clasp)|uint16(EAttachment::Rosette);
 return ValidAward(I,A)?A:FAward{};
}
FString Progress(int32 I,bool ZH)
{
 if(!Available(Definition(I).Nation))return AwardLabel(I,ZH);
 if(I==14)return ZH?TEXT("服役资格：1914年8月5日—11月22日"):TEXT("SERVICE: 5 AUGUST–22 NOVEMBER 1914");
 if(I==15)return ZH?TEXT("服役资格：1914—1915年"):TEXT("SERVICE: 1914–1915");
 const auto A=Award(I);const int32 Target=NextTarget(I);
 if(I==3&&A.Grade==1)return AwardLabel(I,ZH)+(ZH?TEXT(" · 下一授级：一役击毁4辆装甲"):TEXT(" · NEXT: 4 ARMOUR KILLS IN ONE VICTORY"));
 if(I==3&&A.Grade==2)return AwardLabel(I,ZH)+(ZH?TEXT(" · 下一授级：全程未败通关去而复返"):TEXT(" · NEXT: UNDEFEATED SURVIVOR CAMPAIGN"));
 if(I==21&&A.Grade<3)return AwardLabel(I,ZH)+(ZH?TEXT(" · 新战役全程未败通关去而复返"):TEXT(" · NEW UNDEFEATED SURVIVOR CAMPAIGN"));
 if(I==20&&Target>0)return AwardLabel(I,ZH)+FString::Printf(TEXT(" · %d / %d"),Value(I),Target)+(ZH?TEXT(" · 一役击毁2辆装甲，无误伤"):TEXT(" · 2 ARMOUR KILLS IN ONE VICTORY, NO FRIENDLY FIRE"));
 if(I==37)return AwardLabel(I,ZH)+(ZH?TEXT(" · 一役以支援击毁2辆装甲"):TEXT(" · FIRE SUPPORT: 2 ARMOUR KILLS IN ONE VICTORY"));
 if(I==38||I==39)return AwardLabel(I,ZH)+(Target>0?FString::Printf(TEXT(" · %d / %d"),Value(I),Target):FString())+(ZH?TEXT(" · 1918年6月起"):TEXT(" · FROM JUNE 1918"));
 if((I==2||I==5)&&A.Grade)return AwardLabel(I,ZH);
 return AwardLabel(I,ZH)+(Target>0?FString::Printf(TEXT(" · %d / %d"),Value(I),Target):FString());
}
FString FlagTooltip(EFrontlineFaction F,bool ZH){return FlagTooltip(EFRRUNationV81(uint8(F)),ZH);}
FString FlagTooltip(EFRRUNationV81 F,bool ZH)
{
 static const TCHAR* EN[]={TEXT("BRITAIN"),TEXT("GERMANY"),TEXT("FRANCE"),TEXT("RUSSIA")};static const TCHAR* CN[]={TEXT("英国"),TEXT("德国"),TEXT("法国"),TEXT("俄国")};const int32 N=FMath::Clamp(int32(F),0,3);const auto S=Stats(F);
 FString Text=ZH?FString::Printf(TEXT("%s · 勋章记录\n步兵击杀 %d · 装甲击毁 %d"),CN[N],S.Infantry,S.Armour):FString::Printf(TEXT("%s · DECORATIONS\nInfantry kills %d · Armour destroyed %d"),EN[N],S.Infantry,S.Armour);
 if(N<4)Text+=ZH?FString::Printf(TEXT("\n英勇行为：步兵击毁装甲 · %d 笔"),FrontlineArmourKillTallyV8220::Count(F))
     :FString::Printf(TEXT("\nActs of valour: infantry armour kills · %d"),FrontlineArmourKillTallyV8220::Count(F));
 for(int32 I:NationIndices(F))if(Tier(I)>0)Text+=TEXT("\n")+FString(ZH?Definition(I).NameZH:Definition(I).NameEN)+TEXT(" · ")+AwardLabel(I,ZH);return Text;
}
TArray<int32> Ribbons(EFrontlineFaction F){return Ribbons(EFRRUNationV81(uint8(F)));}
int32 RibbonGradeForHighestClassV8443(int32 I,int32 HighestGrade)
{
 if(I<0||I>=Count||HighestGrade<=0||!Definition(I).Ribbon)return 0;
 const int32 G=FMath::Min(HighestGrade,Variants(I).Num());if(G<=0)return 0;
 // The rack is a record of earned distinctions. Promotion to a neck or pin
 // class retains the earlier breast ribbon, as with the Iron Cross II.
 if(I==18||I==21||I==22||I==11||I==32||I==33||I==42||I==43)return 1;
 if(I==20||I==8)return FMath::Min(G,2);
 if(I==31)return G>=2?2:0; // Anna IV is a weapon badge; III first earns a breast ribbon.
 if(I==34||I==35||I==36)return 0; // These orders never have a breast-ribbon class here.
 return G;
}
TArray<int32> Ribbons(EFRRUNationV81 F)
{
 // Wearer's right to left. Keep earned breast ribbons throughout the award chain.
 static const int32 Order[]={2,12,0,37,1,38,13,39,40,14,15,16,17,3,21,22,20,18,19,24,41,42,43,23,8,7,6,46,47,45,27,25,26,28,29,48,36,11,34,35,33,31,32,10,9,50,52};
 TArray<int32> R;for(int32 I:Order)if(Definition(I).Nation==F&&RibbonGradeForHighestClassV8443(I,Tier(I))>0)R.Add(I);
 return R;
}
bool PreservedForceV842(int32 Entered,int32 Returned)
{return Entered>=24&&Returned>=24&&Returned<=Entered&&int64(Returned)*4>=int64(Entered)*3;}
void ApplyAwards(TMap<FString,FAward>& Awards,const FStats& S,const FProof& P)
{
 if(!P.Won||P.Nation<0||P.Nation>3||!Available(EFRRUNationV81(P.Nation))||P.Date<19140801||P.Date>19181111)return;
 if(P.Nation>=2&&!P.Campaign.IsValid())return;
 const FString Date=FDateTime::UtcNow().ToIso8601();
 for(int32 I:NationIndices(EFRRUNationV81(P.Nation)))
 {
  const auto& D=Definition(I);auto A=Awards.FindRef(D.Id);const bool Existing=A.Grade>0;bool Qualified=false;int32 Desired=1;
  const int32 V=StatValue(D,S);const bool Hard=P.Difficulty>=1;
  switch(I)
  {
  case 0:case 1:Qualified=V>=D.Threshold[0]&&P.FriendlyFire==0;break;
  case 2:case 5:Qualified=P.Completed&&P.Undefeated&&Hard&&!P.Replay&&P.Campaign.IsValid()&&!A.CampaignDeeds.Contains(P.Campaign);break;
  case 3:
   Qualified=S.Infantry>=500;
   if(Existing&&P.Armour>=4)Desired=2;
   if(Existing&&A.Grade>=2&&P.Completed&&P.Undefeated&&P.Difficulty==2&&!P.Replay)Desired=3;break;
  // National decorations use the same durable encounter receipts as Britain
  // and Germany. A career counter never supplies a missing battlefield deed.
  case 6:Qualified=Hard&&P.FriendlyFire==0&&P.Infantry>=36;break;
  case 7:Qualified=Hard&&S.Infantry>=600&&P.Infantry>=36&&P.FriendlyFire==0;break;
  case 8:Qualified=Hard&&P.FriendlyFire==0&&P.Infantry>=48;Desired=FMath::Min(5,1+S.Victories/5);break;
  case 9:Qualified=Hard&&P.FriendlyFire==0&&P.Infantry>=36;Desired=FMath::Min(4,1+S.Infantry/600);break;
  case 10:Qualified=Hard&&P.Obstacles>=2&&P.Infantry>=24;Desired=FMath::Min(4,1+S.Victories/4);break;
  case 11:Qualified=Hard&&P.Infantry>=60&&P.FriendlyFire==0;Desired=FMath::Min(3,1+S.Victories/6);break;
  case 12:Qualified=S.CleanVictories>=12&&Hard&&P.ArmourLosses==0;break;
  case 13:Qualified=S.Infantry>=1000&&P.FriendlyFire==0;break;
  case 14:Qualified=P.Date>=19140805&&P.Date<=19141122&&!Awards.Contains(TEXT("uk.star_1914_15"));break;
  case 15:Qualified=P.Date>=19140805&&P.Date<=19151231&&!Awards.Contains(TEXT("uk.star_1914"));break;
  case 16:case 17:Qualified=P.Completed&&!P.Replay;break;
  case 18:Qualified=Hard&&P.FriendlyFire==0&&P.Infantry>=60;Desired=P.Infantry>=120?3:P.Infantry>=90?2:1;break;
  case 19:Qualified=P.FriendlyFire==0&&P.Infantry>=24&&P.Obstacles>=3;Desired=P.Obstacles>=6?3:P.Obstacles>=4?2:1;break;
  case 20:Qualified=Hard&&P.FriendlyFire==0&&P.Armour>=2&&S.Armour>=12;Desired=S.Armour>=150?6:S.Armour>=100?5:S.Armour>=65?4:S.Armour>=40?3:S.Armour>=24?2:1;break;
  case 21:Qualified=P.Completed&&P.Undefeated&&P.Difficulty==2&&!P.Replay&&P.Campaign.IsValid()&&!A.CampaignDeeds.Contains(P.Campaign);Desired=FMath::Min(3,int32(A.Grade)+1);break;
  case 22:
  {
   // A challenging preservation deed allows casualties at every class. The
   // denominator is the actual entered roster, not purchased reserves.
   static constexpr int32 Percent[]={75,80,85,90};const int32 Stage=FMath::Min(3,int32(A.Grade));
   Qualified=Hard&&P.Infantry>=40&&PreservedForceV842(P.EnteredInfantryV842,P.ReturnedInfantryV842)
    &&int64(P.ReturnedInfantryV842)*100>=int64(P.EnteredInfantryV842)*Percent[Stage];
   Desired=Stage+1;break;
  }
  case 23:Qualified=Hard&&P.Armour>=2&&P.MinimumMorale<.25f;break;
  case 24:Qualified=S.Infantry>=1200&&PreservedForceV842(P.EnteredInfantryV842,P.ReturnedInfantryV842);Desired=S.Infantry>=5000?2:1;break;
  case 25:case 26:Qualified=P.Completed&&!P.Replay;break;
  // No colonial/eastern-expedition service, certified wounds or escape from
  // captivity is inferred from a Western Front casualty counter.
  case 27:case 28:case 29:case 30:case 47:break;
  case 31:Qualified=Hard&&P.Infantry>=36&&P.FriendlyFire==0;Desired=FMath::Min(4,1+S.Infantry/800);break;
  case 32:Qualified=Hard&&P.Obstacles>=3&&P.Infantry>=24;Desired=FMath::Min(3,1+S.Victories/6);break;
  case 33:Qualified=Hard&&P.Infantry>=48&&P.FriendlyFire==0;Desired=FMath::Min(4,1+S.Infantry/1200);break;
  case 34:Qualified=P.Completed&&Hard&&!P.Replay;break;
  case 35:Qualified=P.Completed&&P.Difficulty==2&&!P.Replay;break;
  case 36:Qualified=P.Completed&&P.Undefeated&&P.Difficulty==2&&!P.Replay;break;
  case 37:Qualified=Hard&&P.SupportArmourV84412>=2;break;
  case 38:Qualified=Hard&&P.Date>=19180603&&S.Infantry>=1000&&P.AircraftInfantryV84412>=24;break;
  case 39:Qualified=Hard&&P.Date>=19180603&&P.AircraftArmourV84412>=1;break;
  case 40:Qualified=Hard&&S.Victories>=16&&P.Obstacles>=4;break;
  case 41:Qualified=Hard&&P.FriendlyFire==0&&P.Infantry>=48&&P.MinimumMorale<.30f;break;
  case 42:Qualified=Hard&&P.Obstacles>=3&&P.Infantry>=40;Desired=P.Obstacles>=5&&P.Infantry>=60?2:1;break;
  case 43:Qualified=Hard&&P.FriendlyFire==0&&P.SupportArmourV84412>=1;Desired=P.SupportArmourV84412>=2?2:1;break;
  case 44:Qualified=P.Date>=19170514&&P.Difficulty==2&&P.FriendlyFire==0&&P.Infantry>=80;break;
  case 45:Qualified=P.Completed&&Hard&&!P.Replay;break;
  case 46:Qualified=P.Completed&&P.Undefeated&&P.Difficulty==2&&!P.Replay;break;
  case 48:Qualified=Hard&&S.Victories>=12&&P.Obstacles>=4;Desired=S.Victories>=40?3:S.Victories>=24?2:1;break;
  case 49:Qualified=S.Armour>=40&&P.Completed&&P.Undefeated&&P.Difficulty==2&&!P.Replay;break;
  case 50:Qualified=Hard&&S.Victories>=16&&P.Obstacles>=4;Desired=S.Victories>=32?2:1;break;
  case 51:Qualified=Hard&&S.Victories>=20&&PreservedForceV842(P.EnteredInfantryV842,P.ReturnedInfantryV842);break;
  case 52:Qualified=Hard&&S.Victories>=24;break;
  }
  if(!Qualified)continue;
  // Legacy scores cannot manufacture a high historical class. Each advancement
  // requires a new receipt-qualified deed; a new award starts at the real base.
  const uint8 BeforeGrade=A.Grade,BeforeRepeats=A.Repeats;const uint16 BeforeAttachments=A.Attachments;
  if(!Existing){A.Grade=1;A.Date=Date;A.Campaign=P.Campaign;}
  else if(!RepeatBars(I)&&I!=5&&I!=17)A.Grade=FMath::Max(int32(A.Grade),FMath::Min(Desired,int32(A.Grade)+1));
  if(Existing&&RepeatBars(I))
  {const int32 Max=RepeatLimitV85(I);const bool NewDeed=I==2||BattleDeedBarsV84414(I)||V>=TargetForStage(I,int32(A.Repeats)+1);if(A.Repeats<Max&&NewDeed){++A.Repeats;A.Attachments|=uint16(EAttachment::Bar);}}
  if(I==5&&Existing)A.Attachments|=uint16(EAttachment::OakLeaves);
  if((I==2||I==5||I==21)&&(BeforeGrade!=A.Grade||BeforeRepeats!=A.Repeats||BeforeAttachments!=A.Attachments)){A.Campaign=P.Campaign;A.CampaignDeeds.AddUnique(P.Campaign);}
  if(I==17&&P.Undefeated&&Hard)A.Attachments|=uint16(EAttachment::OakLeaves);
  if(I==14)A.Attachments=uint16(EAttachment::Clasp)|uint16(EAttachment::Rosette);
  if(I==18||I==19||I==20)A.Attachments|=uint16(EAttachment::Swords);
  if(I==6)A.Attachments|=uint16(EAttachment::Palm);
  if(I==8&&A.Grade>=2)A.Attachments|=uint16(EAttachment::Rosette);
  if(I==9||I==10){A.Attachments&=~uint16(EAttachment::Bow);if(A.Grade==2||A.Grade==4)A.Attachments|=uint16(EAttachment::Bow);}
  if(I==31||I==32||I==33||I==34||I==35)A.Attachments|=uint16(EAttachment::Swords);
  Awards.Add(D.Id,MoveTemp(A));
 }
}
void RunCatalogueChecks(TFunctionRef<void(const FString&,bool)> Check)
{
 TSet<FString> Ids;bool Unique=true,VariantsValid=true;for(int32 N=0;N<4;++N){const auto List=NationIndices(EFRRUNationV81(N));Check(FString::Printf(TEXT("v84412_nation_%d_has_thirteen_real_categories"),N),List.Num()==13);for(int32 I:List){Unique&=!Ids.Contains(Definition(I).Id);Ids.Add(Definition(I).Id);VariantsValid&=!Variants(I).IsEmpty();}}
 Check(TEXT("v84412_52_stable_ids_no_iron_class_double_count"),Unique&&Ids.Num()==52&&!NationIndices(EFRRUNationV81::German).Contains(4));Check(TEXT("v8216_real_variants_present"),VariantsValid);
 FStats S;S.Infantry=8000;S.Armour=150;S.CleanVictories=40;S.NaturalRoute=3;FProof P;P.Date=19170409;P.Nation=0;P.Won=true;TMap<FString,FAward> A;
 ApplyAwards(A,S,P);Check(TEXT("v8216_legacy_high_counters_start_at_base_not_fake_grade"),A.FindRef(TEXT("uk.distinguished_conduct")).Grade==1&&A.FindRef(TEXT("uk.distinguished_conduct")).Repeats==0);
 Check(TEXT("v8216_1917_cannot_award_1914_service_stars"),!A.Contains(TEXT("uk.star_1914"))&&!A.Contains(TEXT("uk.star_1914_15")));
 P.Nation=2;const auto Before=A;ApplyAwards(A,S,P);Check(TEXT("v87_french_awards_follow_release_gate"),Available(EFRRUNationV81::French)?A.Contains(TEXT("fr.medaille_militaire"))==false:A.Num()==Before.Num());
 P.Nation=0;P.Date=19140901;TMap<FString,FAward> Early;ApplyAwards(Early,S,P);Check(TEXT("v8216_two_campaign_stars_mutually_exclusive"),Early.Contains(TEXT("uk.star_1914"))&&!Early.Contains(TEXT("uk.star_1914_15")));
 P.Date=19140804;TMap<FString,FAward> PreStar;ApplyAwards(PreStar,S,P);Check(TEXT("v8216_service_stars_exclude_before_5_august_1914"),!PreStar.Contains(TEXT("uk.star_1914"))&&!PreStar.Contains(TEXT("uk.star_1914_15")));
 P.Date=19141123;TMap<FString,FAward> LaterStar;ApplyAwards(LaterStar,S,P);Check(TEXT("v8216_1914_15_star_starts_after_1914_star_window"),LaterStar.Contains(TEXT("uk.star_1914_15"))&&!LaterStar.Contains(TEXT("uk.star_1914")));
 FAward Bad;Bad.Grade=3;Bad.Date=FDateTime::UtcNow().ToIso8601();Check(TEXT("v8216_single_class_cannot_decode_virtual_level_three"),!ValidAward(0,Bad));
 // The evaluator receives only new durable encounter receipts. Distinct natural
 // campaign IDs are also required for the few campaign-completion decorations.
 S.Infantry=40000;S.Armour=2000;S.CleanVictories=100;P.Date=19171101;P.Difficulty=2;P.Nation=0;P.Completed=false;
 TMap<FString,FAward> Repeats;for(int32 Deed=0;Deed<5;++Deed){P.Campaign=FGuid::NewGuid();ApplyAwards(Repeats,S,P);}
 // V85 replaced the former open-ended fixture with a sourced Great War game
 // boundary: Corey's four awards are one medal and three bars (AWM C331609).
 // That documented record is not asserted to be a statutory legal ceiling.
 const auto MM=Repeats.FindRef(TEXT("uk.military_medal"));const int32 MilitaryMedalLimit=RepeatLimitV85(13);
 Check(TEXT("v87_repeat_bars_follow_current_sourced_v85_catalogue_limit"),MilitaryMedalLimit==3&&MM.Grade==1
     &&MM.Repeats==FMath::Min(4,MilitaryMedalLimit)&&MM.Attachments==uint16(EAttachment::Bar)&&ValidAward(13,MM));
 P.Campaign=FGuid::NewGuid();ApplyAwards(Repeats,S,P);const auto AfterLimit=Repeats.FindRef(TEXT("uk.military_medal"));
 Check(TEXT("v87_further_qualifying_deed_cannot_exceed_catalogue_repeat_limit"),AfterLimit.Grade==MM.Grade&&AfterLimit.Repeats==MM.Repeats&&AfterLimit.Attachments==MM.Attachments);
 TMap<FString,FAward> VC;P.Completed=true;P.Undefeated=true;P.Campaign=FGuid::NewGuid();const FGuid First=P.Campaign;
 ApplyAwards(VC,S,P);ApplyAwards(VC,S,P);Check(TEXT("v8216_same_campaign_cannot_generate_vc_repeat"),VC.FindRef(TEXT("uk.victoria_cross")).Repeats==0);
 P.Campaign=FGuid::NewGuid();ApplyAwards(VC,S,P);P.Campaign=First;ApplyAwards(VC,S,P);P.Campaign=FGuid::NewGuid();ApplyAwards(VC,S,P);
 const auto Victory=VC.FindRef(TEXT("uk.victoria_cross"));Check(TEXT("v8216_vc_one_real_bar_distinct_campaign_receipts"),Victory.Repeats==1&&Victory.CampaignDeeds.Num()==2&&ValidAward(2,Victory));
 P.Nation=1;P.Obstacles=6;P.Armour=4;P.Infantry=120;P.EnteredInfantryV842=100;P.ReturnedInfantryV842=90;TMap<FString,FAward> German;bool Incremental=true;
 for(int32 Deed=0;Deed<6;++Deed){P.Campaign=FGuid::NewGuid();ApplyAwards(German,S,P);const auto M=German.FindRef(TEXT("de.bavarian_merit_order"));Incremental&=M.Grade==Deed+1&&ValidAward(20,M);}
 Check(TEXT("v8216_all_six_bavarian_order_classes_reachable_in_sequence"),Incremental);
 Check(TEXT("v8216_all_real_german_order_classes_reachable"),German.FindRef(TEXT("de.hohenzollern")).Grade==3&&German.FindRef(TEXT("de.max_joseph")).Grade==3&&German.FindRef(TEXT("de.st_henry")).Grade==4);
 const auto PLM=German.FindRef(TEXT("de.pour_le_merite"));Check(TEXT("v8216_plm_oak_leaves_have_two_distinct_campaign_deeds"),PLM.Grade==1&&PLM.Attachments==uint16(EAttachment::OakLeaves)&&PLM.CampaignDeeds.Num()==2&&ValidAward(5,PLM));
 auto Duplicate=Victory;const FGuid DuplicatedDeed=Duplicate.CampaignDeeds[0];Duplicate.CampaignDeeds.Add(DuplicatedDeed);Check(TEXT("v8216_duplicate_campaign_deed_rejected_on_decode"),!ValidAward(2,Duplicate));
 auto Fictional=MM;Fictional.Attachments|=uint16(EAttachment::Palm);Check(TEXT("v8216_attachments_cannot_cross_historical_awards"),!ValidAward(13,Fictional));
 P.Replay=true;TMap<FString,FAward> Replay;ApplyAwards(Replay,S,P);Check(TEXT("v8216_archive_replay_never_forges_natural_campaign_decoration"),!Replay.Contains(TEXT("de.max_joseph"))&&!Replay.Contains(TEXT("de.pour_le_merite")));
 P.Replay=false;P.Nation=3;ApplyAwards(Replay,S,P);bool Russian=false;for(const auto& Entry:Replay)Russian|=Entry.Key.StartsWith(TEXT("ru."));Check(TEXT("v87_russian_awards_follow_release_gate"),Russian==Available(EFRRUNationV81::Russian));
}
#include "WarfareFrontlineHonoursChecksV84413.inl"
#include "WarfareFrontlineGermanAwardChecksV85.inl"
#include "WarfareFrontlineRepeatCapChecksV85.inl"
#include "WarfareFrontlineAwardChecksV84414.inl"
void RunNationalAwardChecksV87(TFunctionRef<void(const FString&,bool)> Check)
{
 for(int32 N:{2,3})
 {
  FStats S;S.Infantry=10000;S.Armour=100;S.Victories=40;S.NaturalRoute=3;
  FProof P;P.Nation=N;P.Date=19171024;P.Difficulty=2;P.Won=true;P.Completed=true;P.Undefeated=true;
  P.Infantry=100;P.Obstacles=5;P.EnteredInfantryV842=40;P.ReturnedInfantryV842=32;P.Campaign=FGuid::NewGuid();
  TMap<FString,FAward> Awards;for(int32 Step=0;Step<5;++Step)ApplyAwards(Awards,S,P);
  const bool Open=Available(EFRRUNationV81(N));bool Valid=true,OwnNation=true;
  for(const auto& Entry:Awards){const int32 I=IndexForId(Entry.Key);Valid&=ValidAward(I,Entry.Value);OwnNation&=Definition(I).Nation==EFRRUNationV81(N);}
  Check(FString::Printf(TEXT("v87_nation_%d_durable_deeds_only_own_valid_decorations"),N),Valid&&OwnNation&&(Open?!Awards.IsEmpty():Awards.IsEmpty()));
  if(Open&&N==2)
  {bool Unsupported=false;for(int32 I:{27,28,29,30,47})Unsupported|=Awards.Contains(Definition(I).Id);Check(TEXT("v87_french_western_service_cannot_forge_other_theatres_wounds_or_captivity"),!Unsupported);Check(TEXT("v87_french_eight_qualifying_categories_reachable"),Awards.Num()==8&&Awards.FindRef(Definition(8).Id).Grade==5);}
  if(Open&&N==3)Check(TEXT("v87_russian_thirteen_categories_and_real_class_limits"),Awards.Num()==13&&Awards.FindRef(Definition(11).Id).Grade==3&&Awards.FindRef(Definition(9).Id).Grade==4);
  P.Won=false;TMap<FString,FAward> Defeat;ApplyAwards(Defeat,S,P);Check(FString::Printf(TEXT("v87_nation_%d_defeat_cannot_award_victory_honours"),N),Defeat.IsEmpty());
  P.Won=true;P.Replay=true;TMap<FString,FAward> Replay;ApplyAwards(Replay,S,P);
  bool Natural=false;for(int32 I:N==2?TArray<int32>{25,26,45,46}:TArray<int32>{34,35,36,49})Natural|=Replay.Contains(Definition(I).Id);
  Check(FString::Printf(TEXT("v87_nation_%d_replay_cannot_forge_campaign_completion"),N),!Natural);
 }
}
void RunExtensionChecksV84412(TFunctionRef<void(const FString&,bool)> Check)
{
 RunNationalAwardChecksV87(Check);
 bool Ids=true,Forms=true,Cheats=true;TSet<FString> Seen;int32 NewForms=0;
 for(int32 N=0;N<4;++N){const auto List=NationIndices(EFRRUNationV81(N));Ids&=List.Num()==13;for(int32 I:List){Ids&=!Seen.Contains(Definition(I).Id);Seen.Add(Definition(I).Id);}}
 for(int32 I=37;I<Count;++I){const auto V=Variants(I);NewForms+=V.Num();Forms&=!V.IsEmpty()&&VariantArtworkPath(I,1).StartsWith(TEXT("/Game/Warfare/UI/V84413/Honours/"));Cheats&=ValidAward(I,CompleteCheatAwardV8218(I,FGuid::NewGuid(),FDateTime::UtcNow().ToIso8601()));}
 Check(TEXT("v84412_16_unique_new_categories_and_21_real_forms"),Ids&&Seen.Num()==52&&Forms&&NewForms==21);
 Check(TEXT("v84412_complete_cheat_awards_cover_real_new_classes_and_bars"),Cheats);
 Check(TEXT("v84412_pin_and_weapon_awards_never_fabricate_ribbons"),!Definition(44).Ribbon&&!Definition(49).Ribbon&&!Definition(51).Ribbon&&RibbonGradeForHighestClassV8443(42,2)==1&&RibbonGradeForHighestClassV8443(43,2)==1);
 FStats S;S.Infantry=2500;S.Armour=80;S.Victories=24;FProof P;P.Nation=0;P.Date=19180531;P.Difficulty=1;P.Won=true;P.Campaign=FGuid::NewGuid();P.SupportArmourV84412=2;P.AircraftArmourV84412=1;P.AircraftInfantryV84412=24;P.Obstacles=4;
 TMap<FString,FAward> Early;ApplyAwards(Early,S,P);Check(TEXT("v84412_aviation_medals_exclude_dates_before_institution"),!Early.Contains(Definition(38).Id)&&!Early.Contains(Definition(39).Id));
 P.Date=19180603;TMap<FString,FAward> A;ApplyAwards(A,S,P);Check(TEXT("v84412_aviation_actual_proof_qualifies_at_institution"),A.FindRef(Definition(38).Id).Grade==1&&A.FindRef(Definition(39).Id).Grade==1&&A.FindRef(Definition(37).Id).Grade==1&&A.FindRef(Definition(40).Id).Grade==1);
 P.AircraftInfantryV84412=23;P.AircraftArmourV84412=0;P.SupportArmourV84412=1;TMap<FString,FAward> Below;ApplyAwards(Below,S,P);Check(TEXT("v84412_cumulative_stats_cannot_substitute_actual_air_and_support_deed"),!Below.Contains(Definition(37).Id)&&!Below.Contains(Definition(38).Id)&&!Below.Contains(Definition(39).Id));
 P.Nation=1;P.Date=19180901;P.Difficulty=2;P.Infantry=80;P.Armour=4;P.Obstacles=5;P.SupportArmourV84412=2;P.MinimumMorale=.29f;TMap<FString,FAward> German;ApplyAwards(German,S,P);ApplyAwards(German,S,P);Check(TEXT("v84412_real_two_class_german_crosses_advance_without_virtual_levels"),German.FindRef(Definition(42).Id).Grade==2&&German.FindRef(Definition(43).Id).Grade==2&&German.FindRef(Definition(41).Id).Grade==1&&German.FindRef(Definition(44).Id).Grade==1);
 for(int32 N:{2,3}){P.Nation=N;TMap<FString,FAward> Paused;ApplyAwards(Paused,S,P);Check(FString::Printf(TEXT("v87_nation_%d_awards_follow_campaign_gate"),N),Available(EFRRUNationV81(N))?!Paused.IsEmpty():Paused.IsEmpty());}
}
}
