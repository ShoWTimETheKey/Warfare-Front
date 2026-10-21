// The field console groups infantry, armour, fire support and command.
// There is intentionally no strategic mini-map or FRONT LINE status panel.
const bool German=B->GetPlayerFaction()==EFrontlineFaction::German;
const bool FieldInteraction=!UI.bSuppressBackgroundHits;
const FLinearColor ArmyTrim=German?FLinearColor(.40f,.47f,.48f):FLinearColor(.47f,.40f,.27f);
const FLinearColor ArmyInk=German?FLinearColor(.56f,.61f,.59f):FLinearColor(.64f,.59f,.45f);
UI.Skin(0,0,-6,1920,84,13);UI.Hit(0,0,1920,78,TEXT("Blocked"),0);
const auto FrontsV76=FrontlineFrontsV76::Read(B,UIMotionReviewV83::HoldMorale?0.f:FApp::GetDeltaTime());
if(FParse::Param(FCommandLine::Get(),TEXT("WFV66Review"))&&B->IsProgressionVolatile())
{
    static TSet<FGuid> RecordedGaugeEncountersV8213;
    if(!RecordedGaugeEncountersV8213.Contains(B->GetEncounterId()))
    {
        RecordedGaugeEncountersV8213.Add(B->GetEncounterId());
        auto J=MakeShared<FJsonObject>();
        for(int32 T=0;T<2;++T)
        {
            const FString K=FString::FromInt(T);
            J->SetNumberField(TEXT("morale_actual_")+K,B->Morale[T]);
            J->SetNumberField(TEXT("morale_fraction_")+K,FrontsV76.Morale[T]);
            J->SetNumberField(TEXT("morale_display_")+K,FrontsV76.MoraleDisplay[T]);
            J->SetNumberField(TEXT("advance_display_")+K,FrontsV76.AdvanceDisplay[T]);
        }
        FString JSON;auto Writer=TJsonWriterFactory<>::Create(&JSON);FJsonSerializer::Serialize(J,Writer);
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V66/Review");
        IFileManager::Get().MakeDirectory(*Dir,true);
        FFileHelper::SaveStringToFile(JSON,*(Dir/(TEXT("gauge-")+B->GetEncounterId().ToString()+TEXT(".json"))));
    }
}
for(int32 Team=0;Team<2;++Team)
{
    const bool Left=B->GetTeamDirection(Team)>0;const float FlagX=Left?5.f:1835.f,GaugeX=Left?91.f:1081.f;
    UI.Skin(2,FlagX-1,3,82,58,5);DrawCasualtyFlag(UI,B,Team,FlagX+3,7,74,49);
    const int32 GaugeNation=WarfareFRRUCampaignV82::Active(B)?FMath::Clamp(int32(WarfareFRRUCampaignV82::Nation(B,Team)),0,3):int32(B->GetFactionForTeam(Team));
    const FLinearColor AdvanceInk=NationalConsolePaintV8213(GaugeNation);
    const FLinearColor MoraleInk(.54f,.20f,.19f);
    UI.ConsoleGaugeV79(GaugeX,3,748,48);
    UI.Line(GaugeX+25,24,GaugeX+723,24,FLinearColor(.009f,.012f,.01f,.45f));
    for(int32 Row=0;Row<2;++Row)
    {
        if(Row&&FrontsV76.MoraleGhostActive[Team]&&!UI.MotionStableV83)
            UI.ConsoleFillV79(GaugeX+25,25,698,9,FrontsV76.MoraleGhost[Team],Left,FLinearColor(.54f,.20f,.19f,.42f),true,1.4f,GaugeNation);
        const float Width=698*(Row?FrontsV76.MoraleDisplay[Team]:FrontsV76.AdvanceDisplay[Team]);
        if(Width>1)
        {
            const float Y=Row?25.f:14.f,H=9.f;
            UI.ConsoleFillV79(GaugeX+25,Y,698,H,Row?FrontsV76.MoraleDisplay[Team]:FrontsV76.AdvanceDisplay[Team],Left,Row?MoraleInk:AdvanceInk,true,Row?1.1f:2.2f,GaugeNation);
        }
    }
    const TCHAR* Army=WarfareFRRUCampaignV82::Active(B)?B->GetFactionName(Team):B->GetFactionForTeam(Team)==EFrontlineFaction::British?TEXT("BRITISH EXPEDITIONARY FORCE"):TEXT("DEUTSCHES HEER");
    UI.PrimaryLabel(Army,GaugeX,54,748,17,13,Muted,Left?0:2);
    if(FieldInteraction&&PC->SupportTargetMode==0&&UI.Hovered(GaugeX,3,748,48))
    {
        UI.TooltipV83(TooltipKeyV83(1,Team*2+(UI.Hovered(GaugeX,3,748,22)?0:1)),uint64(FMath::RoundToInt(100*(UI.Hovered(GaugeX,3,748,22)?FrontsV76.Advance[Team]:FrontsV76.Morale[Team]))),[&]{
        const bool Advance=UI.Hovered(GaugeX,3,748,22);const float X=Left?108.f:1114.f;
        UI.PaperSheet(X,81,686,218);const FLinearColor Ink(.025f,.029f,.025f);
        UI.Label(WarRoom52::L(Advance?TEXT("GROUND TAKEN"):TEXT("MORALE"),Advance?TEXT("前锋推进"):TEXT("士气")),X+23,99,465,36,25,Ink);
        UI.Label(FString::Printf(TEXT("%.0f%%"),100*(Advance?FrontsV76.Advance[Team]:FrontsV76.Morale[Team])),X+500,99,158,36,25,Ink,2);
        const bool NationalOperation=WarfareFRRUCampaignV82::Active(B);
        const FString Explanation=Advance
            ?(NationalOperation?WarRoom52::L(TEXT("The foremost surviving infantry marks the advance. Complete the operation's objectives to secure the sector."),TEXT("推进条记录最前沿生还步兵的位置。完成本次作战目标，方可夺取该地区。"))
                :WarRoom52::L(TEXT("The foremost surviving infantry marks this army's advance. The rear must still be cleared before a breakthrough."),TEXT("由本方最前沿的生还步兵标记推进。突破仍须肃清敌军后卫。")))
            :(NationalOperation?WarRoom52::L(TEXT("Losses, friendly fire, gas and armour sap morale. Taking ground and mutual support restore it. If your army breaks, the operation is lost."),TEXT("减员、友军误伤、毒气和装甲威胁削弱士气。夺取阵地与协同推进可恢复士气。我军士气耗尽即告失败。"))
                :WarRoom52::L(TEXT("Losses, friendly fire, gas and nearby enemy armour shake confidence. Captured ground and a supported advance restore it. An army at zero morale breaks."),TEXT("减员、友军误伤、毒气和近距离装甲威胁动摇信心。占领与互相支援的推进恢复士气。士气耗尽，部队溃败。")));
        UI.Paragraph(Explanation,X+23,150,640,116,21,Ink,true);
        UI.Hit(X,81,686,218,TEXT("Blocked"),20);

        });
    }
}
auto ArmyBadgeV82=[&](float X,float Y,float W,float H)
{
    if(WarfareFRRUCampaignV82::Active(B))
    {
        if(auto* Badge=NationalTextureV82(WarfareFRRUCampaignV82::Nation(B),TEXT("Emblem")))
        {UI.ImageContain(Badge,X,Y,W,H);return;}
    }
    UI.ConsoleToolV79(11,X,Y,W,H);
};
// Identical heraldry is mirrored around the clock's x=960 centre. Both calls
// share the national renderer and contain bounds, including the FR/RU crests.
ArmyBadgeV82(1024,12,47,47);
UI.PrimaryLabel(FString::FromInt(B->GetEncounterYear()),850,6,63,25,16,Muted,1);UI.PrimaryLabel(ClockText(B->Elapsed),844,34,75,29,20,Paper,1);
MedalMountV8216::Header(UI,PC,B,FieldInteraction&&PC->SupportTargetMode==0);
for(int32 Team=0;Team<2;++Team)
{
 const bool Left=B->GetTeamDirection(Team)>0;const FBox2D Label(FVector2D(Left?91.f:1081.f,54),FVector2D(Left?839.f:1829.f,71));
 if(FieldInteraction&&PC->SupportTargetMode==0&&(UI.Hovered(Left?5.f:1835.f,3,82,58)||UI.Hovered(Label.Min.X,Label.Min.Y,Label.GetSize().X,Label.GetSize().Y)))
 {
  const bool OwnFlag=Team==0&&UI.Hovered(Left?5.f:1835.f,3,82,58);
  const bool FlagChinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
  const FString FlagBody=OwnFlag?FrontlineMedalsV8215::FlagTooltip(WarfareFRRUCampaignV82::Nation(B,0),FlagChinese):FString();
        UI.TooltipV83(TooltipKeyV83(2,Team),GetTypeHash(FlagBody),[&]{
  const bool DE=B->GetFactionForTeam(Team)==EFrontlineFaction::German;const float X=Left?30:1240;
  if(Team==0&&UI.Hovered(Left?5.f:1835.f,3,82,58))
  {
   const auto Nation=WarfareFRRUCampaignV82::Nation(B,0);const auto Role=EFrontlineTextRoleV8210::Text;
   const FString Value=UI.RoleValueV8210(Role,FlagBody);const bool CJK=UI.ContainsCJKV45(Value);
   float BodyH=0;
   {
    TGuardValue<bool> FaceScope(UI.bRoleChineseV8210,CJK);const auto Rows=UI.RoleRowsV8210(Role,Value,602,19);
    float GlyphH=0;for(const auto& Row:Rows)GlyphH=FMath::Max(GlyphH,float(UI.RoleSizeRawV8210(Role,Row,19).Y));
    BodyH=FMath::CeilToFloat(FMath::Max(0,Rows.Num()-1)*19.f*(CJK?1.9f:1.48f)+GlyphH)+6.f;
   }
   const int32 RibbonCount=MedalWallV8215::OrderedRibbonItems(Nation).Num();
   const float RackH=RibbonCount>0?((RibbonCount+2)/3)*18.f:0.f;
   const float BodyLimit=975.f-48.f-(RackH>0?RackH+16.f:0.f);BodyH=FMath::Min(BodyH,BodyLimit);
   const float PopupH=48.f+BodyH+(RackH>0?RackH+16.f:0.f);const FLinearColor Ink(.025f,.029f,.025f);
   UI.PaperSheet(X,81,650,PopupH);UI.RoleParagraph(Role,FlagBody,X+24,105,602,BodyH,19,Ink);
   if(RackH>0)DrawFactionRibbonV8215(UI,B,Nation,X+24,105+BodyH+16,580,RackH);
   return;
  }
  UI.PaperSheet(X,81,650,244);const FLinearColor Ink(.025f,.029f,.025f);
  if(WarfareFRRUCampaignV82::Active(B))
  {
   const auto NationalArmy=WarfareFRRUCampaignV82::Nation(B,Team);
   FString NationalHeadingV82,NationalBriefV82;
   switch(NationalArmy)
   {
    case EFRRUNationV81::French:
     NationalHeadingV82=WarRoom52::L(TEXT("FRENCH ARMY"),TEXT("法国陆军"));
     NationalBriefV82=WarRoom52::L(TEXT("Infantry regiments and artillery hold the French front. Assault detachments follow the barrage through broken ground."),TEXT("步兵团与炮兵守住法军战线。突击分队紧随炮幕，穿过被炮火撕裂的地面。"));break;
    case EFRRUNationV81::Russian:
     NationalHeadingV82=WarRoom52::L(TEXT("RUSSIAN ARMY"),TEXT("俄国陆军"));
     NationalBriefV82=WarRoom52::L(TEXT("Field armies face long fronts and uncertain supply. Machine-gun sections hold the crossings while reserves wait for a passage."),TEXT("战线漫长，补给难继。机枪分队守住渡口，预备队等待通路打开。"));break;
    case EFRRUNationV81::AustroHungarian:
     NationalHeadingV82=WarRoom52::L(TEXT("AUSTRO-HUNGARIAN ARMY"),TEXT("奥匈帝国陆军"));
     NationalBriefV82=WarRoom52::L(TEXT("The Monarchy's field armies guard passes, towns and river crossings. M.95 rifles and Schwarzlose guns anchor the infantry line."),TEXT("帝国诸军守卫山口、城镇与河渡。M.95步枪与施瓦茨洛泽机枪支撑步兵防线。"));break;
    case EFRRUNationV81::German:
     NationalHeadingV82=WarRoom52::L(TEXT("DEUTSCHES HEER"),TEXT("德意志帝国陆军"));
     NationalBriefV82=WarRoom52::L(TEXT("German infantry holds the firing bays. MG 08 sections secure the line; assault patrols work through the traverses."),TEXT("德军步兵据守射击阵位。MG 08机枪扼住战线，突击巡逻队穿行于壕沟折角。"));break;
    default:NationalHeadingV82=WarRoom52::L(TEXT("FIELD ARMY"),TEXT("野战军"));break;
   }
   UI.Label(NationalHeadingV82,X+24,100,602,38,24,Ink);UI.Paragraph(NationalBriefV82,X+24,158,602,124,21,Ink,true);
  }
  else
  {
   UI.Label(WarRoom52::L(DE?TEXT("DEUTSCHES HEER"):TEXT("BRITISH EXPEDITIONARY FORCE"),DE?TEXT("德意志帝国陆军"):TEXT("英国远征军")),X+24,100,602,38,24,Ink);
   UI.Paragraph(WarRoom52::L(DE?TEXT("Years of war have made the Western Front a fortress. Machine-gun sections anchor the defence; assault troops seek the gaps."):TEXT("Britain's field armies hold the line from Flanders to the Somme. Rifle sections advance under the cover of heavy guns and armour."),DE?TEXT("数年的战事将西线铸成堡垒。机枪扼守阵地，突击队寻找缺口。"):TEXT("从佛兰德斯到索姆河，远征军守住战线。步兵在重炮与装甲掩护下推进。")),X+24,158,602,124,21,Ink,true);
  }

        });
    }
}

if(FieldInteraction&&PC->SupportTargetMode==0&&UI.Hovered(845,0,70,74))if(const auto* M=B->GetEncounterCampaignMission())
{
        UI.TooltipV83(TooltipKeyV83(3),0,[&]{
    constexpr float X=624,W=672;UI.PaperSheet(X,80,W,294);
    const FLinearColor Ink(.025f,.029f,.025f);
    UI.FitText(M->Title,X+23,99,W-46,24,Ink);
    UI.FitText(FString(FrontlineDifficultyV30::Label(B->GetCampaignDifficulty())),X+23,134,W-46,17,Ink);
    UI.Paragraph(M->Briefing,X+23,169,W-46,181,16,Ink,true);
    UI.Hit(X,80,W,294,TEXT("Blocked"),20);

        });
    }

if (PC->SupportTargetMode>0)
{
    UI.Metal(725,88,470,53,true);
    const FString Label=FString(B->GetSupportName(SupportForTargetMode(PC->SupportTargetMode)))+TEXT(" · 选择覆盖地段");
    UI.PrimaryLabel(Label,741,95,438,38,18,Amber,1);UI.Shortcut(725,88,470,53,TEXT("鼠标右键取消"));UI.Hit(725,88,470,53,TEXT("Blocked"),0);
}
UI.Gradient(0,DockTop-18,1920,18,FLinearColor(0,0,0,0),FLinearColor(0,0,0,.68f),true);
UI.Metal(0,DockTop,1920,1080-DockTop);
if(WarfareFRRUCampaignV82::Active(B))ArmyBadgeV82(146,DockTop-10,56,42);
else UI.ConsoleToolV79(11,126,DockTop-10,96,42);
UI.Hit(0,DockTop,1920,1080-DockTop,TEXT("Blocked"),0);
// One row geometry and one measured font for all console groups. Portrait
// captions share the same box below the image, including the armour cards.
constexpr float CardW=FrontlineConsoleLayoutV79::CardWidth,CardH=FrontlineConsoleLayoutV79::CardHeight,CardPitch=FrontlineConsoleLayoutV79::CardPitch,CardTop=FrontlineConsoleLayoutV79::CardTop;
constexpr float CardNameY=138.f,CardNameH=27.f,CardNameSize=13.f;
auto GroupHeadingV70=[&](const TCHAR* Name,float X,float W){UI.PrimaryLabel(Name,X,DockTop+6,W,28,15,Paper,1);};
auto CardCaptionV70=[&](const FString& Name,float X,float Y,bool Ready){UI.PrimaryCaption(Name,X+9,Y+CardNameY,CardW-18,CardNameH,CardNameSize,Ready?Paper:Muted);};
GroupHeadingV70(TEXT("INFANTRY"),24,672);
UI.PrimaryLabel(B->ActiveSquads(0)==1?FString(TEXT("1  SECTION")):FString::Printf(TEXT("%d  SECTIONS"),B->ActiveSquads(0)),540,DockTop+6,156,28,15,Muted,2);

// The same opaque emblem and framed recess are used for every unavailable slot.
auto LockedSlotV64=[&](float X,float Y,float W,float H)
{
 UI.Skin(2,X,Y,W,H,6);
 const bool UnitSlot=H>W;const float D=UnitSlot?48.f:32.f;
 const float CentreY=H*.5f;
 DrawEmblemV30(UI,4,X+(W-D)*.5f,Y+CentreY-D*.5f,D,D);
 UI.Hit(X,Y,W,H,TEXT("Blocked"));
};
const auto Roles=FieldRolesV32(B);
for(int32 Slot=0;Slot<5;++Slot)
{
    const float X=FrontlineConsoleLayoutV79::InfantryX+Slot*CardPitch,W=CardW,Y=DockTop+CardTop,H=CardH;
    if(!Roles.IsValidIndex(Slot)){LockedSlotV64(X,Y,W,H);continue;}
    const auto SelectedRole=Roles[Slot];const FString Reason=B->RecruitBlockReason(SelectedRole);
    const float CD=B->RecruitCooldown(SelectedRole);const bool Ready=Reason.IsEmpty(),Hover=FieldInteraction&&UI.Hovered(X,Y,W,H);
    if(!B->IsRoleUnlocked(SelectedRole))
    {
        LockedSlotV64(X,Y,W,H);
        if(Hover){
        UI.TooltipV83(TooltipKeyV83(4,uint32(SelectedRole)),0,[&]{UI.Metal(X,DockTop-62,310,47,true);UI.PlainText(TEXT("随战役推进获得新的配属"),X+14,DockTop-49,16,Muted);
        });
    }
        continue;
    }
    UI.Skin(2,X,Y,W,H,6);
    DrawRolePhotoV32(UI,B,SelectedRole,X+6,Y+6,116,116,CD>.01f);
    UI.CooldownShade(X+6,Y+6,116,116,1-FMath::Clamp(CD/FMath::Max(1.f,B->EffectiveRecruitSeconds(SelectedRole)),0.f,1.f));
    UI.Gradient(X+6,Y+104,116,18,FLinearColor(0,0,0,0),FLinearColor(.004f,.004f,.004f,.46f),true);
    CardCaptionV70(RoleCaptionV32(B,SelectedRole),X,Y,Ready);
    UI.PrimaryRight(FString::FromInt(B->GetSquadSize(SelectedRole)),X+114,Y+99,16,Paper);
    if(State.AutoRoleV63==int32(SelectedRole))DrawRepeatOrderV63(UI,X+20,Y+107);
    UI.ConsoleSelectionV79(X,Y,W,H,false,Ready&&Hover);
    UI.Hit(X,Y,W,H,Ready?FName(*FString::Printf(TEXT("RecruitRole%d"),int32(SelectedRole))):FName(TEXT("Blocked")));
    if(Hover)
    {
        UI.TooltipV83(TooltipKeyV83(5,uint32(SelectedRole)),GetTypeHash(Reason),[&]{
        const float TX=FMath::Min(X,1460.f);
        UI.Metal(TX,DockTop-300,450,286,true);
        UI.PrimaryLabel(B->GetRecruitName(SelectedRole),TX+14,DockTop-287,420,31,19,Paper);
        UI.Paragraph(RoleTipV32(SelectedRole,B),TX+14,DockTop-246,420,164,16,Muted,true);
        if(!Ready)UI.Label(ChineseStatus(Reason),TX+14,DockTop-76,390,25,14,Amber);
        UI.Label(WarRoom52::L(TEXT("Right-click: toggle continuous recruitment"),TEXT("右键：开启或取消连续征募"))+FString::Printf(TEXT(" · %d"),Slot+1),TX+14,DockTop-48,422,20,13,Muted);

        });
    }
}
// Armour is a separate roster; it never displaces any of the five infantry roles.
UI.ConsoleSeamV79(708,DockTop+36,708,DockTop+216,ArmyTrim*.5f);
GroupHeadingV70(TEXT("ARMOUR"),728,264);
if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
{
    if(WarfareFRRUCampaignV82::Active(B))
    {
        const auto Roster=FRRUArmourUIV82::Selected(B);
        for(int32 I=0;I<2;++I)
        {
            const float X=FrontlineConsoleLayoutV79::ArmourX+I*CardPitch,Y=DockTop+CardTop,W=CardW,H=CardH;
            UI.Skin(2,X,Y,W,H,6);
            if(!Roster.IsValidIndex(I)){LockedSlotV64(X,Y,W,H);continue;}
            const auto* D=WarfareFRRUArmourV82::Definition(Roster[I]);if(!D){LockedSlotV64(X,Y,W,H);continue;}
            const FString Reason=WarfareFRRUArmourV82::BlockReason(B,D->Id);const bool Ready=Reason.IsEmpty();const float CD=WarfareFRRUArmourV82::CooldownFraction(B,0,D->Id);
            FRRUArmourUIV82::Portrait(UI,D->Id,X+6,Y+6,116,CD>.001f);UI.CooldownShade(X+6,Y+6,116,116,1-CD);CardCaptionV70(D->Name,X,Y,Ready);
            if(State.AutoArmourStableV82==D->Id)DrawRepeatOrderV63(UI,X+20,Y+107);
            UI.Hit(X,Y,W,H,Ready?FName(*(TEXT("FRRUArmour:recruit:")+D->Id)):FName(TEXT("Blocked")));
            if(FieldInteraction&&UI.Hovered(X,Y,W,H))
            {
        UI.TooltipV83(TooltipKeyV83(6,I),GetTypeHash(Reason),[&]{UI.Metal(X-74,DockTop-315,468,302,true);UI.PrimaryLabel(D->Name,X-59,DockTop-302,437,32,19,Paper);UI.Paragraph(WarfareFRRUTextV82::Resolve(D->Id,TEXT("description"),D->Description,D->Description),X-59,DockTop-261,437,182,16,Muted,true);if(!Ready)UI.Label(Reason,X-59,DockTop-74,437,25,14,Amber);
        });
    }
        }
    }
    else
    {
    const auto Roster=Vehicles->GetEquippedRoster();
    for(int32 I=0;I<2;++I)
    {
        const float X=FrontlineConsoleLayoutV79::ArmourX+I*CardPitch,Y=DockTop+CardTop,W=CardW,H=CardH;
        UI.Skin(2,X,Y,W,H,6);
        if(!Roster.IsValidIndex(I)){LockedSlotV64(X,Y,W,H);continue;}
        const auto& Entry=Roster[I];const FString Reason=Vehicles->RecruitBlockReason(Entry.Type);const bool Ready=Reason.IsEmpty();
        const float CD=Vehicles->CooldownFraction(Entry.Type);
        const bool Available=Vehicles->IsDeploymentEligibleV837(Entry.Type,0);
        UI.Image(Vehicles->GetPortrait(Entry.Type),X+6,Y+6,116,116,FLinearColor(.91f,.92f,.89f,1),0,0,1,1,CD>.001f||!Available,false);
        UI.CooldownShade(X+6,Y+6,116,116,1-CD);
        CardCaptionV70(ArmourCaptionV70(Entry.Type),X,Y,Ready);
        if(State.AutoVehicleV63==int32(Entry.Type))DrawRepeatOrderV63(UI,X+20,Y+107);
        UI.Hit(X,Y,W,H,Ready?FName(*FString::Printf(TEXT("Vehicle%d"),int32(Entry.Type))):FName(TEXT("Blocked")));
        if(FieldInteraction&&UI.Hovered(X,Y,W,H))
        {
        UI.TooltipV83(TooltipKeyV83(7,uint32(Entry.Type)),GetTypeHash(Reason),[&]{
            UI.Metal(X-74,DockTop-315,468,302,true);UI.PrimaryLabel(Entry.Name,X-59,DockTop-302,437,32,19,Paper);
            UI.Paragraph(FrontlinePlayerGuideV43::Armour(Entry.Type),X-59,DockTop-261,437,182,16,Muted,true);
            if(!Ready)UI.Label(Reason,X-59,DockTop-74,437,25,14,Amber);
            UI.Label(WarRoom52::L(TEXT("Right-click: toggle continuous recruitment"),TEXT("右键：开启或取消连续征募")),X-59,DockTop-43,437,19,13,Muted);

        });
    }
    }
    }
}
// Two rows of square support paintings share the portrait-frame treatment.
UI.ConsoleSeamV79(1004,DockTop+36,1004,DockTop+216,ArmyTrim*.5f);
GroupHeadingV70(TEXT("FIRE SUPPORT"),1016,364);

const auto Supports=FieldSupportsV32(B);
for(int32 I=0;I<6;++I)
{
    const float X=1031+(I%3)*124,Y=DockTop+36+(I/3)*94,W=86,H=86;
    if(!Supports.IsValidIndex(I)){LockedSlotV64(X,Y,W,H);continue;}
    const auto Kind=Supports[I];const FString Reason=B->SupportBlockReason(Kind,0);
    const bool Ready=Reason.IsEmpty(),Active=PC->SupportTargetMode==SupportTargetMode(Kind);
    const float CD=B->SupportCooldown(Kind),Total=Kind==EFrontlineSupport::Artillery?B->EffectiveArtillerySeconds():B->SupportCooldownSeconds(Kind);
    if(!B->IsSupportUnlocked(Kind))
    {
        LockedSlotV64(X,Y,W,H);continue;
    }
    UI.Skin(2,X,Y,W,H,5);
    DrawSupportPhotoV32(UI,B,Kind,X+5,Y+5,76,76,CD>.01f);
    UI.CooldownShade(X+5,Y+5,76,76,1-FMath::Clamp(CD/FMath::Max(1.f,Total),0.f,1.f));
    UI.Gradient(X+5,Y+53,76,28,FLinearColor(0,0,0,0),FLinearColor(.004f,.004f,.004f,.92f),true);
    UI.PrimaryCaption(Active?TEXT("CANCEL"):SupportCaptionV32(B,Kind),X+5,Y+62,W-10,17,11,Ready?Paper:Muted);
    UI.ConsoleSelectionV79(X,Y,W,H,Active,Ready&&FieldInteraction&&UI.Hovered(X,Y,W,H));
    UI.Hit(X,Y,W,H,Ready||Active?FName(*FString::Printf(TEXT("SupportKind%d"),int32(Kind))):FName(TEXT("Blocked")));
    if(FieldInteraction&&UI.Hovered(X,Y,W,H))
    {
        const FVector Target=PC->bPreviewValid?PC->ArtilleryPreview:FVector::ZeroVector;
        const FVector2D Arrival=B->FireSupportDelayRange(Kind,Target);
        const uint32 ArrivalRevision=HashCombineFast(GetTypeHash(Reason),HashCombineFast(GetTypeHash(Arrival.X),GetTypeHash(Arrival.Y)));
        UI.TooltipV83(TooltipKeyV83(8,uint32(Kind)),ArrivalRevision,[&]{
        const float PanelH=362.f,Top=DockTop-PanelH-13;
        UI.Metal(X-144,Top,459,PanelH,true);
        UI.PrimaryLabel(B->GetSupportName(Kind),X-129,Top+13,429,28,19,Paper);
        FString Tip=B->GetSupportDescription(Kind);
        UI.Paragraph(Tip,X-129,Top+54,429,222,15,Muted,true);
        UI.Label(FrontlineStatusTextV8217::SupportArrival(Arrival),X-129,Top+281,429,27,14,Amber);
        if(!Reason.IsEmpty())UI.Label(Reason,X-129,Top+309,429,25,14,NoticeWarningV75);
        const FString Key=SupportKeyV32(Kind);
        if(!Key.IsEmpty())UI.PlainText(TEXT("快捷键  ")+Key,X-129,DockTop-39,13,Muted);

        });
    }
}

UI.ConsoleSeamV79(1392,DockTop+36,1392,DockTop+216,ArmyTrim*.5f);

UI.Skin(2,1404,DockTop+36,124,180,6);UI.ConsoleToolV79(10,1420,DockTop+47,92,92);
UI.PrimaryCaption(TEXT("RESEARCH"),1413,DockTop+147,106,24,13,Paper);
const int32 VisibleResearchV828=B->GetDisplayedResearchPointsV828();
UI.PrimaryLabel(FString::Printf(TEXT("%d"),VisibleResearchV828),1413,DockTop+177,106,23,16,Amber,1);
UI.Hit(1404,DockTop+36,124,180,TEXT("Technology"));
UI.Shortcut(1404,DockTop+36,124,180,TEXT("T"));
UI.ConsoleSeamV79(1540,DockTop+36,1540,DockTop+216,ArmyTrim*.5f);
GroupHeadingV70(TEXT("COMMAND"),1552,344);
UI.PrimaryButton(1552,DockTop+36,224,52,Paused?TEXT("RESUME"):TEXT("PAUSE"),TEXT("Space"),TEXT("Pause"));
// A three-position control occupies the former camera-centre switch.
// Simulation time changes; mouse, camera and menu animation retain real time.
for(int32 I=0;I<3;++I)
{
    const float Values[]={.5f,1.f,1.5f};const TCHAR* Labels[]={TEXT("0.5×"),TEXT("1×"),TEXT("1.5×")};
    const float X=1552+I*76.f;const bool Selected=FMath::IsNearlyEqual(B->BattleSpeed,Values[I]);
    UI.Skin(3,X,DockTop+100,72,52,7,Selected?FLinearColor::White:FLinearColor(.70f,.72f,.71f,1));
    UI.PrimaryLabel(Labels[I],X+3,DockTop+110,66,29,20,Selected?Amber:Muted,1);
    UI.ConsoleSelectionV79(X,DockTop+100,72,52,Selected,FieldInteraction&&UI.Hovered(X,DockTop+100,72,52));
    UI.Hit(X,DockTop+100,72,52,FName(*FString::Printf(TEXT("BattleRate%d"),I)));
    if(FieldInteraction&&UI.Hovered(X,DockTop+100,72,52))
    {
        UI.TooltipV83(TooltipKeyV83(9,I),0,[&]{UI.Metal(1447,DockTop-70,443,55,true);UI.Label(WarRoom52::L(TEXT("Battle speed · 0.5× / 1× / 1.5×"),TEXT("战斗速度 · 0.5× / 1× / 1.5×")),1462,DockTop-57,414,29,18,Paper);
        });
    }
}
UI.PrimaryButton(1552,DockTop+164,224,52,TEXT("MENU"),TEXT("ESC"),TEXT("Menu"));
UI.IconButton(5,1792,DockTop+36,104,84,TEXT("Settings"),TEXT("设置 · O"));
UI.IconButton(6,1792,DockTop+132,104,84,TEXT("Help"),TEXT("作战手册 · H"));
if (PC->SupportTargetMode==0 && !State.Dispatches.IsEmpty() && B->Elapsed-State.Dispatches[0].Key<4.f)
{
    const FString& Message=State.Dispatches[0].Value;
    const bool SupportAccepted=Message.Contains(TEXT("已受理"))||Message.Contains(TEXT("命令已传往后方"));
    if (SupportAccepted || Message.Contains(TEXT("占领")) || Message.Contains(TEXT("失守")) || Message.Contains(TEXT("误伤")))
    {
        const FString Dispatch=SupportAccepted?(Message.Contains(TEXT("烟幕"))?TEXT("烟幕任务已受理"):TEXT("火力任务已受理")):Message;
        UI.Metal(699,89,522,38,true);UI.Label(Dispatch,715,94,490,28,16,NoticeColourV75(Message),1);
    }
}

// Rail detachments use an ordinary selected armour portrait above. They never
// create a third recruitment control or a second readiness queue.
