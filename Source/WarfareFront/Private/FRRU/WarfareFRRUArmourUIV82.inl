namespace FRRUArmourUIV82
{
UTexture2D* Portraits(){return Artwork().FRRUArmourAtlasV825.Get();}
void Portrait(const FFrontlineCanvas& UI,const FString& Id,float X,float Y,float Size,bool Dimmed)
{
    int32 Index=0;for(const auto& D:WarfareFRRUArmourV82::Definitions())
    {if(D.Id==Id){if(auto* T=Portraits()){const float IU=1.f/T->GetSizeX(),IV=1.f/T->GetSizeY();UI.Image(T,X,Y,Size,Size,FLinearColor(.87f,.88f,.85f,1),float(Index%3)/3+IU,float(Index/3)/2+IV,1.f/3-2*IU,.5f-2*IV,Dimmed,true);}return;}++Index;}
}
TArray<FString> Selected(AWarfareFrontlineGameMode* B)
{auto R=WarfareFRRUCampaignV82::ArmourSelection(B);R.RemoveAll([&](const FString& Id){return !WarfareFRRUArmourV82::Eligible(B,Id,0);});return R;}
void DrawDeployment(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    UI.PageSurfaceV73();UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);
    UI.Label(WarRoom52::L(TEXT("ORDERS OF BATTLE"),TEXT("出战序列")),72,64,1210,65,40,Paper);
    if(const auto* M=B->GetSelectedCampaignMission())UI.Label(M->DateLabel,1340,74,468,54,21,Muted,2);
    UI.PageTabV73(72,PageCanvasV73::TabY,556,WarRoom52::L(TEXT("INFANTRY / SUPPORT"),TEXT("步兵与火力")),TEXT("V63DeploymentMain"),false,true,90);
    UI.PageTabV73(648,PageCanvasV73::TabY,556,WarRoom52::L(TEXT("ARMOUR"),TEXT("装甲配属")),TEXT("V63DeploymentArmour"),true,true,90);
    const auto Roster=WarfareFRRUArmourV82::Roster(B);const auto Slots=Selected(B);
    FString Title,Tip;
    if(Roster.IsEmpty())UI.Paragraph(WarRoom52::L(TEXT("No armour is assigned to this operation."),TEXT("本次行动未配属装甲部队。")),112,340,1696,80,26,Muted,true);
    for(int32 I=0;I<Roster.Num();++I)
    {
        const auto* D=WarfareFRRUArmourV82::Definition(Roster[I]);if(!D)continue;
        const float X=112+I*408,Y=326,W=350;const bool IsSelected=Slots.Contains(D->Id),Allowed=WarfareFRRUArmourV82::Accepted(B,D->Id);
        UI.Skin(2,X,Y,W,W+52,7);Portrait(UI,D->Id,X+6,Y+6,W-12,!IsSelected);
        UI.PrimaryCaption(D->Name,X+8,Y+W+6,W-16,36,23,Allowed?Paper:Muted);UI.ConsoleSelectionV79(X,Y,W,W+52,IsSelected,Allowed&&UI.Hovered(X,Y,W,W+52));
        UI.Hit(X,Y,W,W+52,Allowed?FName(*(TEXT("FRRUArmour:select:")+D->Id)):FName(TEXT("Blocked")),90);
        if(UI.Hovered(X,Y,W,W+52)){Title=D->Name;Tip=WarfareFRRUTextV82::Resolve(D->Id,TEXT("description"),D->Description,D->Description);if(!Allowed)Tip+=TEXT("\n")+WarfareFRRUArmourV82::BlockReason(B,D->Id);}
    }
    const FString RosterBlock=WarfareFRRUCampaignV82::DeploymentBlockReason(B);
    UI.Label(RosterBlock.IsEmpty()?WarRoom52::L(TEXT("Select up to two vehicles. Each uses the armour readiness queue."),TEXT("最多携带两种战车，共用装甲整备队列。")):RosterBlock,112,270,1696,42,23,RosterBlock.IsEmpty()?Muted:Amber);
    DrawSpecialAllocationV87(UI,B,Tip,Title);
    UI.PageRuleV74(80,PageCanvasV73::FooterY,1744);
    UI.PageButtonV73(72,958,340,58,WarRoom52::L(TEXT("RETURN TO ORDERS"),TEXT("返回作战命令")),TEXT("Esc"),TEXT("V32DeploymentBack"),true,false,90);
    UI.PageButtonV73(1368,958,440,58,WarRoom52::L(TEXT("ISSUE ORDERS"),TEXT("签发出战序列")),TEXT("Enter"),TEXT("V32DeploymentConfirm"),RosterBlock.IsEmpty(),true,90);
    if(!Tip.IsEmpty()){UI.PageSurfaceV73(1276,326,530,392,true);UI.PrimaryLabel(Title,1300,350,482,42,27,FLinearColor(.03f,.035f,.03f));UI.Paragraph(Tip,1300,412,482,278,21,FLinearColor(.03f,.035f,.03f),true);}
}
bool Hit(const FName& Box,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    const FString Key=Box.ToString();if(!Key.StartsWith(TEXT("FRRUArmour:")))return false;auto& S=ScreenState(PC);
    if(!WarfareFRRUCampaignV82::Active(B)||S.Settings||S.AchievementsV76||S.Menu||S.Help||S.Technology||S.Preparing)return true;
    if(Key.StartsWith(TEXT("FRRUArmour:select:")))
    {
        if(!S.Deployment||B->Phase!=EFrontlinePhase::Briefing)return true;const FString Id=Key.Mid(17);if(!WarfareFRRUArmourV82::Accepted(B,Id))return true;
        auto Slots=Selected(B);if(Slots.Contains(Id))Slots.Remove(Id);else if(Slots.Num()<2)Slots.Add(Id);WarfareFRRUCampaignV82::SetArmourSelection(B,Slots);
    }
    else if(Key.StartsWith(TEXT("FRRUArmour:recruit:"))&&B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE)
    {const FString Id=Key.Mid(18);if(Selected(B).Contains(Id))WarfareFRRUArmourV82::Recruit(B,Id);}
    return true;
}
}
