// V63 two independent shared queues, with explicit per-encounter repeat orders.
FString DeploymentProseV69(const FString& Name,const FString& Description)
{
 // Guides are catalogued as complete multiline messages. Resolve each authored
 // message before composing a tooltip; splitting the composition into lines
 // loses the whole-message key and leaves its prose in English.
 return FrontlineLocalizationV35::English(Name)+TEXT("\n")+FrontlineLocalizationV35::Text(Description);
}
namespace DeploymentCardV83
{
FBox2D LockedEmblemBounds(float X,float Y,float W,float H,float Preferred=60.f)
{
 const float D=FMath::Max(0.f,FMath::Min(Preferred,FMath::Min(W,H)-12.f));
 return FBox2D(FVector2D(X+(W-D)*.5f,Y+(H-D)*.5f),FVector2D(X+(W+D)*.5f,Y+(H+D)*.5f));
}
void Lock(const FFrontlineCanvas& UI,float X,float Y,float W,float H,float Preferred=60.f)
{
 const auto P=LockedEmblemBounds(X,Y,W,H,Preferred);const auto Size=P.GetSize();
 if(Size.X>0)DrawEmblemV30(UI,4,P.Min.X,P.Min.Y,Size.X,Size.Y);
}
void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Centred=true,Fits=true;
 for(const FVector2D Size:{FVector2D(296,342),FVector2D(182,226),FVector2D(153,192),FVector2D(72,88)})
 {
  const auto P=LockedEmblemBounds(17,31,Size.X,Size.Y);const auto Centre=P.GetCenter();
  Centred&=FMath::IsNearlyEqual(float(Centre.X),float(17+Size.X*.5),.001f)&&FMath::IsNearlyEqual(float(Centre.Y),float(31+Size.Y*.5),.001f);
  Fits&=P.Min.X>=23&&P.Min.Y>=37&&P.Max.X<=17+Size.X-6&&P.Max.Y<=31+Size.Y-6&&FMath::IsNearlyEqual(float(P.GetSize().X),float(P.GetSize().Y),.001f);
 }
 Check(TEXT("v83_deployment_locked_emblems_centred_in_entire_card"),Centred);
 Check(TEXT("v83_deployment_locked_emblems_square_and_inside_frame"),Fits);
}
}
namespace ArmourDeploymentUIV83
{
struct FCard
{
 EFrontlineVehicle Type=EFrontlineVehicle::MarkIVMale;
 FBox2D Bounds,LockBounds;
 bool Eligible=false,Caption=false,Variant=false;
};
struct FDraw{const UCanvas* Canvas=nullptr;uint64 Frame=0;TArray<FCard> Cards;};
static FDraw Last;
bool Safe(const AWarfareFrontlineGameMode* B)
{
 FString UserDir;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV83Review"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
}
void TickAutoOrdersV63(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
 auto& S=ScreenState(PC);
 if(S.AutoEncounterV63!=B->GetEncounterId()||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)
 {S.AutoRoleV63=S.AutoVehicleV63=INDEX_NONE;S.AutoArmourStableV82.Empty();S.AutoEncounterV63=B->GetEncounterId();return;}
 if(UGameplayStatics::IsGamePaused(PC)||S.Menu||S.Settings||S.Help||S.Technology||S.Preparing)return;
 if(S.AutoRoleV63>=0&&S.AutoRoleV63<int32(EFrontlineRole::Count))
 {const auto R=EFrontlineRole(S.AutoRoleV63);if(B->RecruitBlockReason(R).IsEmpty())B->Recruit(R);}
 if(WarfareFRRUCampaignV82::Active(B))
 {
  if(!S.AutoArmourStableV82.IsEmpty())
  {
   const auto Selected=WarfareFRRUCampaignV82::ArmourSelection(B);
   if(!Selected.Contains(S.AutoArmourStableV82)||!WarfareFRRUArmourV82::Eligible(B,S.AutoArmourStableV82,0))S.AutoArmourStableV82.Empty();
   else if(WarfareFRRUArmourV82::BlockReason(B,S.AutoArmourStableV82).IsEmpty())WarfareFRRUArmourV82::Recruit(B,S.AutoArmourStableV82);
  }
  return;
 }
 if(S.AutoVehicleV63>=0&&S.AutoVehicleV63<int32(EFrontlineVehicle::Count))if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
 {const auto T=EFrontlineVehicle(S.AutoVehicleV63);if(V->RecruitBlockReason(T).IsEmpty())V->Recruit(T);}
}
void DrawRepeatOrderV63(const FFrontlineCanvas& UI,float X,float Y)
{
 // Small engraved return-arrow pair; no flashing overlay obscures the portrait.
 const auto C=Amber*.88f;const float R=8.f;
 for(int32 Half=0;Half<2;++Half)for(int32 I=0;I<10;++I)
 {float A=(Half*PI+.28f)+I*.11f,B=A+.11f;UI.Line(X+R*FMath::Cos(A),Y+R*FMath::Sin(A),X+R*FMath::Cos(B),Y+R*FMath::Sin(B),C,1.6f);}
 UI.Line(X-2,Y+8,X+3,Y+9,C,1.6f);UI.Line(X+3,Y+9,X+1,Y+4,C,1.6f);
 UI.Line(X+2,Y-8,X-3,Y-9,C,1.6f);UI.Line(X-3,Y-9,X-1,Y-4,C,1.6f);
}
void DrawArmourResearchV63(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 const auto Nodes=FrontlineArmourV63::Research(B);
 constexpr float CardW=132.f,Column=278.f,Row=144.f;
 const auto Position=[&](const FrontlineArmourV63::FResearch& N){return FVector2D(174+N.Branch*Column,320+N.Tier*Row);};
 const TCHAR* BranchEN[]={TEXT("SERVICE AND STORES"),TEXT("RUNNING GEAR"),TEXT("GUN CREWS"),TEXT("ASSAULT COOPERATION")};
 const TCHAR* BranchZH[]={TEXT("勤务与补给"),TEXT("行走机构"),TEXT("车载火力"),TEXT("突击协同")};
 for(int32 I=0;I<4;++I)UI.RoleLabel(EFrontlineTextRoleV8210::Label,WarRoom52::L(BranchEN[I],BranchZH[I]),112+I*Column,270,256,34,14,Muted,1);
 for(const auto& N:Nodes)if(Nodes.IsValidIndex(N.Prerequisite))
 {
  const auto P=Position(N),Q=Position(Nodes[N.Prerequisite]);const auto C=FrontlineArmourV63::Has(B,N.Prerequisite)?Amber*.45f:Muted*.3f;
  UI.ConsoleSeamV79(Q.X+CardW*.5f,Q.Y+CardW,P.X+CardW*.5f,P.Y,C,1.6f);
 }
 static TStrongObjectPtr<UTexture2D> Colour[2],Archive[2];const int32 Nation=B->GetPlayerFaction()==EFrontlineFaction::German?1:0;
 const FString Base=Nation?TEXT("DE"):TEXT("UK");
 if(!Colour[Nation].IsValid())
 {
  const FString Name=TEXT("T_ArmourTech")+Base+TEXT("_V8218");
  Colour[Nation].Reset(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8218/")+Name+TEXT(".")+Name));
 }
 if(!Archive[Nation].IsValid())
 {
  const FString Name=TEXT("T_ArmourTech")+Base+TEXT("Archive_V8218");
  Archive[Nation].Reset(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8218/")+Name+TEXT(".")+Name));
 }
 int32 Hover=INDEX_NONE;
 for(const auto& N:Nodes)
 {
  const auto P=Position(N);const bool Owned=FrontlineArmourV63::Has(B,N.Id),Can=FrontlineUltimateArchiveV8215::Active(B)||FrontlineArmourV63::ResearchBlock(B,N.Id).IsEmpty(),Over=UI.Hovered(P.X,P.Y,CardW,CardW);
  UI.ConsoleArtFrameV79(P.X,P.Y,CardW,CardW,Over,Owned);
  FFrontlineArtRegionV71 Art,OldPhoto;Art.Texture=Colour[Nation].Get();OldPhoto.Texture=Archive[Nation].Get();
  Art.Slot=OldPhoto.Slot=N.Id;Art.Columns=OldPhoto.Columns=4;Art.Rows=OldPhoto.Rows=4;
  DrawResearchPhotoRegionsV82(UI,Art,OldPhoto,ResearchCardKeyV72(B,N.Id,true),ResearchColourV72(B,N.Id,Owned,true),P.X,P.Y,CardW,CardW);
  UI.Hit(P.X,P.Y,CardW,CardW,Can?FName(*FString::Printf(TEXT("V63ArmourBuy%d"),N.Id)):FName(TEXT("Blocked")),50);
  if(Over)Hover=N.Id;
 }
 if(Nodes.IsValidIndex(Hover))
 {
  const auto& N=Nodes[Hover];const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
  UI.BilingualTitle(N.TitleEN,N.TitleZH,1296,282,512,70,Chinese?24.f:34.f,Paper);
  UI.Description(N.Description,1296,378,512,316,18,Paper,false);
  const bool Owned=FrontlineArmourV63::Has(B,Hover);
  FString Reason=FrontlineUltimateArchiveV8215::Active(B)?(Owned?WarRoom52::L(TEXT("Equipped. Select to remove."),TEXT("已配备。点击移除。")):WarRoom52::L(TEXT("Select to equip."),TEXT("点击配备。"))):(Owned?WarRoom52::L(TEXT("RESEARCHED"),TEXT("已完成研究")):FrontlineArmourV63::ResearchBlock(B,Hover));
  if(!Owned&&!FrontlineUltimateArchiveV8215::Active(B))Reason=WarRoom52::L(TEXT("Research cost: "),TEXT("研究消耗："))+FString::FromInt(N.Cost)+WarRoom52::L(TEXT(" points"),TEXT(" 点"))+(Reason.IsEmpty()?TEXT(""):TEXT("\n")+Reason);
  UI.PageRuleV74(1296,730,512);UI.RoleParagraph(EFrontlineTextRoleV8210::DarkText,Reason,1296,762,512,132,17,Owned?Allied:Muted);
 }
 else UI.BilingualTitle(TEXT("Armour School"),TEXT("装甲学院"),1296,282,512,58,24.f,Paper);
}
namespace ArmourVariantUIReviewV8219
{
struct FButton
{
 EFrontlineVehicle Target=EFrontlineVehicle::MarkIVMale;
 float X=0,Y=0,W=0,H=0;bool Enabled=false,Hovered=false;
};
struct FDraw
{
 const UCanvas* Canvas=nullptr;uint64 Frame=0;TArray<FButton> Buttons;
 EFrontlineVehicle TooltipTarget=EFrontlineVehicle::MarkIVMale;
 bool TooltipDrawn=false;float TooltipX=0,TooltipY=0,TooltipW=0,TooltipH=0;FString Reason;
};
static FDraw Last;
static bool Safe(const AWarfareFrontlineGameMode* B)
{
 FString UserDir;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
// Call immediately after the production deployment draw with the real Canvas.
// Last.Buttons exposes the actual drawn button rectangles to position the next
// review pointer; the check uses that rectangle and the observed tooltip draw.
void WithCanvas(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,EFrontlineVehicle Target,TFunctionRef<void(const FString&,bool)> Check)
{
 const bool Scoped=Safe(B)&&Last.Canvas==UI.Surface&&Last.Frame==GFrameCounter;
 Check(TEXT("v8219_variant_hover_uses_current_disposable_canvas_draw"),Scoped);if(!Scoped)return;
 const auto* Button=Last.Buttons.FindByPredicate([&](const FButton& P){return P.Target==Target;});
 Check(TEXT("v8219_disabled_variant_button_actual_bounds_and_pointer"),Button&&!Button->Enabled&&Button->Hovered
  &&Button->W>0&&Button->H>0&&UI.Hovered(Button->X,Button->Y,Button->W,Button->H));
 auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 Check(TEXT("v8219_disabled_variant_tooltip_draws_authoritative_unlock_reason"),V&&Last.TooltipDrawn&&Last.TooltipTarget==Target
  &&Last.TooltipW>0&&Last.TooltipH>0&&!Last.Reason.IsEmpty()
  &&Last.Reason==FrontlineLocalizationV35::Text(V->RecruitBlockReason(Target)));
}
}
void DrawSpecialAllocationV87(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FString& Tip,FString& TipTitle)
{
 const auto* M=B->GetSelectedCampaignMission();const int32 Order=FrontlineArmourV63::Requisition(B);
 bool LateOperation=M&&M->Id>=8;
 if(WarfareFRRUCampaignV82::Active(B))
 {
  const auto& Missions=WarfareFRRUCampaignV82::Missions(B);
  LateOperation=M&&Missions.IndexOfByPredicate([M](const FFrontlineCampaignMission& Row){return Row.Id==M->Id;})>=8;
 }
 UI.PageRuleV74(112,742,1696);
 UI.BilingualLabel(TEXT("SPECIAL ALLOCATION"),TEXT("特别配属"),112,762,1100,38,21,Paper);
 const bool ArchiveAllocation=FrontlineUltimateArchiveV8215::FullArmory(B,0);
 if(ArchiveAllocation)UI.RoleLabel(EFrontlineTextRoleV8210::Plate,WarRoom52::L(TEXT("FREE ALLOCATION"),TEXT("自由配属")),1308,762,500,38,18,Muted,2);
 else if(LateOperation)UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,WarRoom52::L(TEXT("3 RESEARCH POINTS"),TEXT("3 研究点")),1308,762,500,38,18,Muted,2);
 else UI.RoleLabel(EFrontlineTextRoleV8210::Plate,WarRoom52::L(TEXT("NOT ASSIGNED"),TEXT("未配属")),1308,762,500,38,18,Muted,2);
 for(int32 I=0;I<3;++I)
 {
  const float X=112+I*572;const bool Can=FrontlineArmourV63::CanRequisition(B,I);
  UI.PageButtonV73(X,806,552,58,FrontlineArmourV63::RequisitionTitle(I),TEXT(""),FName(*FString::Printf(TEXT("V63Requisition%d"),I)),Can,Order==I,90);
  if(UI.Hovered(X,806,552,58))
  {TipTitle.Empty();Tip=FrontlineArmourV63::RequisitionDescription(I)+TEXT("\n\n")+(ArchiveAllocation?WarRoom52::L(TEXT("Choose one available attachment. Select another to replace it, or click again to cancel."),TEXT("可任选一项可用配属。选择另一项即可替换，再次点击可取消。")):WarRoom52::L(TEXT("One attachment per operation. Click again to cancel and refund paid research points. Retained on retry."),TEXT("每次行动可配属一项。再次点击可取消，返还已支付的研究点；重试时保留。")));}
 }
 if(Order>=0)UI.RoleLabel(EFrontlineTextRoleV8210::FieldValue,WarRoom52::L(TEXT("Attached: "),TEXT("已配属："))+FrontlineArmourV63::RequisitionTitle(Order),112,872,1696,32,16,Muted);
}
void DrawArmourDeploymentV63(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 const bool RecordVariantHover=ArmourVariantUIReviewV8219::Safe(B);
 const bool RecordLockedCards=ArmourDeploymentUIV83::Safe(B);
 if(RecordVariantHover){ArmourVariantUIReviewV8219::Last=ArmourVariantUIReviewV8219::FDraw();ArmourVariantUIReviewV8219::Last.Canvas=UI.Surface;ArmourVariantUIReviewV8219::Last.Frame=GFrameCounter;}
 if(RecordLockedCards){ArmourDeploymentUIV83::Last=ArmourDeploymentUIV83::FDraw();ArmourDeploymentUIV83::Last.Canvas=UI.Surface;ArmourDeploymentUIV83::Last.Frame=GFrameCounter;}
 UI.PageSurfaceV73();UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);
 const float PageTitleEm=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?32.f:49.f;
 UI.BilingualTitle(TEXT("ORDERS OF BATTLE"),72,64,1210,65,PageTitleEm,Paper);
 const auto* M=B->GetSelectedCampaignMission();if(M)UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(M->DateLabel),1340,74,468,54,19,Muted,2);
 UI.PageTabV73(72,PageCanvasV73::TabY,556,WarRoom52::L(TEXT("INFANTRY / SUPPORT"),TEXT("步兵与火力")),TEXT("V63DeploymentMain"),false,true,90);
 UI.PageTabV73(648,PageCanvasV73::TabY,556,WarRoom52::L(TEXT("ARMOUR"),TEXT("装甲配属")),TEXT("V63DeploymentArmour"),true,true,90);
 UI.RoleLabel(EFrontlineTextRoleV8210::Plate,WarRoom52::L(TEXT("SELECT UP TO TWO PLATFORMS"),TEXT("选择至多两种装甲车型")),112,270,1180,38,21,Paper);
 UI.PageButtonV73(1420,270,388,42,WarRoom52::L(TEXT("ARMOURED RESEARCH"),TEXT("装甲研究")),TEXT(""),TEXT("V63DeploymentResearch"),true,false,90);
 FString Tip,TipTitle;bool VariantTip=false;EFrontlineVehicle TooltipTarget=EFrontlineVehicle::MarkIVMale;auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 if(V)
 {
  const auto Roster=V->GetRoster();const float Step=FMath::Min(318.f,1696.f/FMath::Max(1,Roster.Num())),W=Step-22,StartX=112+(1696.f-Step*Roster.Num())*.5f;int32 I=0;
  for(const auto& D:Roster)
  {
   const float X=StartX+I++*Step,Y=326;const bool Eligible=V->IsUnlocked(D.Type),Selected=Eligible&&FrontlineArmourV63::IsSelected(B,D.Type);
   const bool Forced=Eligible&&FrontlineArmourV63::ForcedAllocationV842(B,D.Type);
   UI.Skin(2,X,Y,W,W+46,Eligible?7:6);
   if(Eligible)
   {
    UI.Image(V->GetPortrait(D.Type),X+6,Y+6,W-12,W-12,FLinearColor::White,0,0,1,1,!Selected,false);
    UI.RoleLabel(EFrontlineTextRoleV8210::Plate,ArmourCaptionV70(D.Type),X+6,Y+W+6,W-12,34,25,Paper,1);
    UI.ConsoleSelectionV79(X,Y,W,W+46,Selected,UI.Hovered(X,Y,W,W+46));
   }
   else DeploymentCardV83::Lock(UI,X,Y,W,W+46);
   UI.Hit(X,Y,W,W+46,Eligible&&!Forced?FName(*FString::Printf(TEXT("V63ArmourSelect%d"),int32(D.Type))):FName(TEXT("Blocked")),90);
   const auto Platform=FrontlineArmourV63::PlatformV8218(D.Type);
   const bool HasVariant=Eligible&&(Platform==EFrontlineVehicle::MarkIVMale||Platform==EFrontlineVehicle::BeuteMarkIV||Platform==EFrontlineVehicle::A7V);
   if(RecordLockedCards)
   {
    ArmourDeploymentUIV83::FCard Card;Card.Type=D.Type;Card.Eligible=Eligible;Card.Caption=Eligible;Card.Variant=HasVariant;
    Card.Bounds=FBox2D(FVector2D(X,Y),FVector2D(X+W,Y+W+46));Card.LockBounds=DeploymentCardV83::LockedEmblemBounds(X,Y,W,W+46);
    ArmourDeploymentUIV83::Last.Cards.Add(Card);
   }
   if(HasVariant)
   {
    const auto Alternate=Platform==EFrontlineVehicle::A7V?EFrontlineVehicle::A7VGretchen:Platform==EFrontlineVehicle::BeuteMarkIV?EFrontlineVehicle::BeuteMarkIVFemale:EFrontlineVehicle::MarkIVFemale;
    const auto Target=D.Type==Platform?Alternate:Platform;const bool CanSwap=V->IsUnlocked(Target);
    const FString Caption=Target==EFrontlineVehicle::A7VGretchen?TEXT("GRETCHEN"):Target==Platform?WarRoom52::L(TEXT("CANNON FIT"),TEXT("火炮配置")):WarRoom52::L(TEXT("MACHINE-GUN FIT"),TEXT("机枪配置"));
    const float ButtonX=X+6,ButtonY=Y+W+52,ButtonW=W-12,ButtonH=40;
    const bool VariantHovered=UI.Hovered(ButtonX,ButtonY,ButtonW,ButtonH);
    UI.PageButtonV73(ButtonX,ButtonY,ButtonW,ButtonH,Caption,TEXT(""),FName(*FString::Printf(TEXT("V8218ArmourVariant%d"),int32(Platform))),CanSwap,false,92);
    if(RecordVariantHover){ArmourVariantUIReviewV8219::FButton Button;Button.Target=Target;Button.X=ButtonX;Button.Y=ButtonY;Button.W=ButtonW;Button.H=ButtonH;Button.Enabled=CanSwap;Button.Hovered=VariantHovered;ArmourVariantUIReviewV8219::Last.Buttons.Add(Button);}
    if(VariantHovered&&!CanSwap)
    {TipTitle=ArmourCaptionV70(Target);Tip=FrontlineLocalizationV35::Text(V->RecruitBlockReason(Target));VariantTip=true;TooltipTarget=Target;}
   }

   if(UI.Hovered(X,Y,W,W+46))
   {TipTitle=Eligible?FrontlineLocalizationV35::English(D.Name):FString();Tip=FrontlineLocalizationV35::Text(Eligible?FrontlinePlayerGuideV43::Armour(D.Type):V->RecruitBlockReason(D.Type));if(Eligible)Tip+=TEXT("\n")+(Forced?WarRoom52::L(TEXT("Required for this rail operation. One armour slot remains available."),TEXT("本次铁路行动固定配属，另可选一种装甲。")):WarRoom52::L(TEXT("Select up to two vehicles, or click a selected card to remove it. Each dispatch restarts preparation for both slots; models become ready at their own pace."),TEXT("至多配属两种装甲，再次点击可取消。每次出动会重新开始整备，各型装甲按自身所需时间就绪。")));}
  }
 }
 DrawSpecialAllocationV87(UI,B,Tip,TipTitle);
 UI.PageButtonV73(72,958,340,58,TEXT("返回命令图"),TEXT("Esc"),TEXT("V32DeploymentBack"),true,false,90);
 UI.PageButtonV73(1368,958,440,58,TEXT("签发出战序列"),TEXT("Enter"),TEXT("V32DeploymentConfirm"),true,true,90);
 if(!Tip.IsEmpty())
 {
  const float X=FMath::Clamp(float(UI.Mouse.X)-280.f,112.f,1248.f),Y=UI.Mouse.Y<700?552.f:342.f;
  UI.PageSurfaceV73(X,Y,560,350,true);const FLinearColor Ink(.03f,.035f,.03f);
  if(!TipTitle.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Headline,TipTitle,X+24,Y+18,512,36,28,Ink);
  UI.Description(Tip,X+24,Y+(TipTitle.IsEmpty()?24:74),512,TipTitle.IsEmpty()?302:252,17,Ink);
  if(RecordVariantHover&&VariantTip)
  {auto& Draw=ArmourVariantUIReviewV8219::Last;Draw.TooltipDrawn=true;Draw.TooltipTarget=TooltipTarget;Draw.Reason=Tip;Draw.TooltipX=X+24;Draw.TooltipY=Y+(TipTitle.IsEmpty()?24:74);Draw.TooltipW=512;Draw.TooltipH=TipTitle.IsEmpty()?302:252;}
 }
}
