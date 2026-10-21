void DrawHistoricalDispatchV32(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{ DrawHistoricalDispatchV52(UI,B); }
void DrawDeploymentV32(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    const bool National=WarfareFRRUCampaignV82::Active(B);
    if(National&&ScreenState(UI.HUD.GetOwningPlayerController()).DeploymentTabV63==1){FRRUArmourUIV82::DrawDeployment(UI,B);return;}
    if(!National&&ScreenState(UI.HUD.GetOwningPlayerController()).DeploymentTabV63==1){DrawArmourDeploymentV63(UI,B);return;}
    const auto L=FrontlineCampaignV32::GetLoadout(B);
    UI.PageSurfaceV73();UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    UI.BilingualTitle(TEXT("ORDERS OF BATTLE"),TEXT("出战序列"),72,64,1210,65,Chinese?32.f:49.f,Paper);
    const auto* M=B->GetSelectedCampaignMission();if(M)UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(M->DateLabel),1340,74,468,54,19,Muted,2);
    UI.PageTabV73(72,PageCanvasV73::TabY,556,WarRoom52::L(TEXT("INFANTRY / SUPPORT"),TEXT("步兵与火力")),TEXT("V63DeploymentMain"),true,true,90);
    UI.PageTabV73(648,PageCanvasV73::TabY,556,WarRoom52::L(TEXT("ARMOUR"),TEXT("装甲配属")),TEXT("V63DeploymentArmour"),false,true,90);
    UI.BilingualLabel(TEXT("配属步兵"),112,270,1070,38,21,Paper);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::Printf(TEXT("%d / %d"),FrontlineCampaignV32::BitCount(L.SelectedRoles),L.RoleCapacity),1450,270,358,38,29,Amber,2);
    FString Tip,TipTitle;
    int32 RoleCount=0;
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(National?(L.EligibleRoles&(1u<<I))!=0:FrontlineGameplayV52::SelectableRole(EFrontlineRole(I))&&FrontlineCampaignV41::RoleAllowed(B->GetPlayerFaction(),EFrontlineRole(I)))++RoleCount;
    const float RoleStep=FMath::Min(204.f,1696.f/FMath::Max(1,RoleCount)),RoleW=RoleStep-16,RolePhoto=RoleW-12;
    int32 RoleSlot=0;
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
    {
        if(National?!(L.EligibleRoles&(1u<<I)):!FrontlineGameplayV52::SelectableRole(EFrontlineRole(I))||!FrontlineCampaignV41::RoleAllowed(B->GetPlayerFaction(),EFrontlineRole(I)))continue;
        const float X=112+RoleSlot++*RoleStep,Y=326,W=RoleW,H=RolePhoto+50;
        const auto DisplayRole=National?EFrontlineRole(I):FrontlineGameplayV52::RecruitVariant(B,EFrontlineRole(I));
        const bool Eligible=(L.EligibleRoles&(1u<<I))!=0,Selected=(L.SelectedRoles&(1u<<I))!=0;
        UI.Skin(2,X,Y,W,H,6);
        if(Eligible)
        {
            DrawRolePhotoV32(UI,B,DisplayRole,X+6,Y+6,RolePhoto,RolePhoto,!Selected);
            UI.RoleLabel(EFrontlineTextRoleV8210::Plate,RoleCaptionV32(B,DisplayRole),X+4,Y+RolePhoto+14,W-8,30,22,Selected?Paper:Muted,1);
            UI.ConsoleSelectionV79(X,Y,W,H,Selected,Eligible&&UI.Hovered(X,Y,W,H));
        }
        else DeploymentCardV83::Lock(UI,X,Y,W,H,60);
        UI.Hit(X,Y,W,H,Eligible?FName(*FString::Printf(TEXT("V32Role%d"),I)):FName(TEXT("Blocked")),90);
        if(UI.Hovered(X,Y,W,H))
        {
            TipTitle=Eligible?FrontlineLocalizationV35::English(B->GetRecruitName(DisplayRole)):FString();
            Tip=Eligible?FrontlineLocalizationV35::Text(RoleTipV32(DisplayRole,B)):B->RoleUnlockReason(EFrontlineRole(I));
            if(Eligible&&!Selected&&FrontlineCampaignV32::BitCount(L.SelectedRoles)>=L.RoleCapacity)Tip+=TEXT("\n配属已满，先取消一支分队的配属。");
        }
    }
    if(FrontlineGameplayV52::MobileAvailable(B))
    {
        const bool Mobile=FrontlineGameplayV52::RecruitVariant(B,EFrontlineRole::Lewis)==EFrontlineRole::MobileGun;
        UI.PageButtonV73(112+RoleStep,554,90,38,WarRoom52::L(TEXT("FIXED"),TEXT("固定")),TEXT(""),TEXT("V52GunFixed"),true,!Mobile,90);
        UI.PageButtonV73(210+RoleStep,554,90,38,WarRoom52::L(TEXT("MOBILE"),TEXT("伴随")),TEXT(""),TEXT("V52GunMobile"),true,Mobile,90);
        if(UI.Hovered(112+RoleStep,554,188,38))Tip=WarRoom52::L(TEXT("Fixed crews deliver stronger fire at longer range. Escort crews move and deploy faster. New recruits use the selected configuration."),TEXT("固定枪组射程更远，火力更强。伴随枪组移动、架设更快。后续征募采用所选编制。"));
    }
    if(National)UI.BilingualLabel(TEXT("SUPPORT ATTACHMENTS"),TEXT("支援配属"),112,606,1070,38,21,Paper);
    else UI.BilingualLabel(TEXT("炮兵与航空配属"),112,606,1070,38,21,Paper);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::Printf(TEXT("%d / %d"),FrontlineCampaignV32::BitCount(L.SelectedSupports),L.SupportCapacity),1450,606,358,38,29,Amber,2);
    TArray<EFrontlineSupport,TInlineAllocator<10>> SupportCards;SupportCards.Append(FrontlineSupportCards,UE_ARRAY_COUNT(FrontlineSupportCards));
    if(National)SupportCards.Insert(EFrontlineSupport::Rally,2);
    const int32 SupportCount=SupportCards.Num();
    const float SupportStep=FMath::Min(190.f,1696.f/SupportCount),SupportW=SupportStep-16,SupportPhoto=SupportW-12;
    for(int32 I=0;I<SupportCount;++I)
    {
        const auto Kind=SupportCards[I];const int32 Id=int32(Kind);
        const float X=112+I*SupportStep,Y=660,W=SupportW,H=SupportPhoto+45;
        const bool Eligible=(L.EligibleSupports&(1u<<Id))!=0,Selected=(L.SelectedSupports&(1u<<Id))!=0;
        UI.Skin(2,X,Y,W,H,6);
        if(Eligible)
        {
            DrawSupportPhotoV32(UI,B,Kind,X+6,Y+6,SupportPhoto,SupportPhoto,!Selected);
            {TGuardValue<bool> EnglishName(UI.bEnglishPrimaryV45,true);
            UI.RoleLabel(EFrontlineTextRoleV8210::Plate,SupportCaptionV32(B,Kind),X+4,Y+SupportPhoto+10,W-8,29,22,Selected?Paper:Muted,1);}
            UI.ConsoleSelectionV79(X,Y,W,H,Selected,Eligible&&UI.Hovered(X,Y,W,H));
        }
        else DeploymentCardV83::Lock(UI,X,Y,W,H,48);
        UI.Hit(X,Y,W,H,Eligible?FName(*FString::Printf(TEXT("V32Support%d"),Id)):FName(TEXT("Blocked")),90);
        if(UI.Hovered(X,Y,W,H))
        {
            TipTitle=Eligible?FrontlineLocalizationV35::English(B->GetSupportName(Kind)):FString();
            Tip=Eligible?FrontlineLocalizationV35::Text(B->GetSupportDescription(Kind)):B->SupportUnlockReason(Kind);
            if(Eligible&&!Selected&&FrontlineCampaignV32::BitCount(L.SelectedSupports)>=L.SupportCapacity)Tip+=TEXT("\n配属已满，先取消一项支援的配属。");
        }
    }
    const FString RosterBlock=WarfareFRRUCampaignV82::DeploymentBlockReason(B);
    UI.RoleLabel(EFrontlineTextRoleV8210::DarkText,RosterBlock.IsEmpty()?WarRoom52::L(TEXT("Assign troops and fire support for the operation. At least one infantry formation is required."),TEXT("编定本次行动的步兵与火力支援。须配属至少一种步兵。")):RosterBlock,112,878,1696,32,17,RosterBlock.IsEmpty()?Muted:Amber);
    UI.PageRuleV74(80,PageCanvasV73::FooterY,1744);
    UI.PageButtonV73(72,958,340,58,TEXT("返回命令图"),TEXT("Esc"),TEXT("V32DeploymentBack"),true,false,90);
    UI.PageButtonV73(1368,958,440,58,TEXT("签发出战序列"),TEXT("Enter"),TEXT("V32DeploymentConfirm"),RosterBlock.IsEmpty(),true,90);
    if(!Tip.IsEmpty())
    {
        // The tooltip is a readable sheet, with one border and no nested casing.
        const float X=FMath::Clamp(float(UI.Mouse.X)-260.f,112.f,1288.f),Y=UI.Mouse.Y<600?584.f:324.f;
        UI.PageSurfaceV73(X,Y,520,300,true);const FLinearColor Ink(.03f,.035f,.03f);
        if(!TipTitle.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Headline,TipTitle,X+24,Y+18,472,34,28,Ink);
        UI.Description(Tip,X+24,Y+(TipTitle.IsEmpty()?24:70),472,TipTitle.IsEmpty()?252:210,17,Ink);
    }
}
