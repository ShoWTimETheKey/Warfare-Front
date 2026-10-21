// The released 1917 campaign introduces both new soldiers and fire support.
// This list reads real unlock APIs; confirming the final sheet begins preparation.
struct FUnitIntroductionCard {FString Heading,Title,Context,Tactic;UTexture2D* Portrait=nullptr;int32 Illustration=INDEX_NONE;EFrontlineRole Attachment=EFrontlineRole::Count;EFrontlineSupport Support=EFrontlineSupport::Count;EFrontlineRole Role=EFrontlineRole::Count;FString NationalArmour;};
FString NationalIntroductionTacticV827(EFrontlineRole Role)
{
    using WarRoom52::L;
    switch(Role)
    {
    case EFrontlineRole::Rifle:return L(TEXT("Hold the firing line. Cross open ground under cover, then occupy the next position."),TEXT("守住射击线。在掩护下穿越空地，夺取下一处阵地。"));
    case EFrontlineRole::Lewis:return L(TEXT("The gun must be set up before it fires. Keep the crew covered during each move and deployment."),TEXT("机枪须架设后才能开火。转移与部署时，枪组需要掩护。"));
    case EFrontlineRole::Mills:return L(TEXT("Close under covering fire. Grenades are most useful against troops gathered in a trench or shelter."),TEXT("借掩护火力接近敌军。手榴弹适合攻击聚集在壕沟与掩体中的步兵。"));
    case EFrontlineRole::Marksman:return L(TEXT("Use the longer reach to engage exposed crews. Keep this small section behind the main firing line."),TEXT("利用射程打击暴露的敌军枪组。小队人数少，宜留在主射击线后方。"));
    case EFrontlineRole::Officer:return L(TEXT("Keep the officer with the fighting line, under cover. His section is too small to lead an unsupported charge."),TEXT("军官应随射击线行动，利用掩体。指挥小队人数少，不宜独自冲击敌壕。"));
    case EFrontlineRole::Flamethrower:return L(TEXT("Smoke and covering fire help the operator close on a shelter. The short jet cannot replace long-range fire."),TEXT("以烟幕和火力掩护喷火手接近掩体。喷流射程短，无法替代远程火力。"));
    case EFrontlineRole::Pioneer:return L(TEXT("Send the sappers to the marked obstruction. Keep enemy fire away while they cut the passage."),TEXT("令工兵抵达标明的障碍，掩护他们清除铁丝、打通道路。"));
    case EFrontlineRole::RifleGrenadier:return L(TEXT("Rifle grenades reach beyond a hand throw. Fire from cover against troops gathered around a position."),TEXT("枪榴弹能越过手榴弹的投掷距离。利用掩体，攻击阵地附近聚集的敌军。"));
    case EFrontlineRole::MobileGun:return L(TEXT("Advance in short bounds with the infantry. A light gun gives up some sustained fire for a quicker move and deployment."),TEXT("随步兵逐段推进。轻机枪转移、部署较快，持续火力逊于固定枪组。"));
    case EFrontlineRole::Signals:return L(TEXT("Keep the liaison section in cover near the fighting troops. It needs protection when the line moves."),TEXT("通信小队应在前沿附近利用掩体。战线转移时，为他们留出掩护。"));
    default:return FString();
    }
}
TArray<FUnitIntroductionCard> UnitIntroductionCards(AWarfareFrontlineGameMode* B)
{
    TArray<FUnitIntroductionCard> Cards;if(!B||B->Phase!=EFrontlinePhase::Briefing)return Cards;
    auto AddRoleCard=[&](EFrontlineRole Role,const FString& Heading,const FString& Title,const FString& Context,const FString& Tactic)
    {
        const auto Art=ResolveRoleArtworkV71(B,Role,EFrontlinePortraitContextV72::Narrative);
        Cards.Add({Heading,Title,Context,Tactic,Art.Texture.Get(),INDEX_NONE,EFrontlineRole::Count,EFrontlineSupport::Count,Role});
        if(Role==EFrontlineRole::MobileGun||Role==EFrontlineRole::Signals)Cards.Last().Attachment=Role;
    };
    const auto* Mission=B->GetSelectedCampaignMission();if(!Mission)return Cards;
    if(WarfareFRRUCampaignV82::Active(B))
    {
        // A replay already owns its complete dated arsenal; it grants no new
        // campaign attachment and must not repeat the ordinary introduction.
        if(WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))return Cards;
        // Native unlock orders are 0..9; the host's 1000/2000 mission IDs
        // are not progression indices. Availability also checks the current
        // mission's actual mask, calendar date and national equipment.
        const auto& Missions=WarfareFRRUCampaignV82::Missions(B);
        const int32 Order=Missions.IndexOfByPredicate([&](const FFrontlineCampaignMission& M){return M.Id==Mission->Id;});
        if(Order==INDEX_NONE)return Cards;
        for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
        {
            const auto Role=EFrontlineRole(I);
            if(WarfareFRRUEquipmentV82::FirstMissionForRole(B,Role,0)!=Order
                ||!WarfareFRRUEquipmentV82::RoleAvailable(B,Role,0))continue;
            AddRoleCard(Role,WarRoom52::L(TEXT("FIELD ATTACHMENT"),TEXT("新增部队配属")),
                B->GetRecruitName(Role),WarfareFRRUEquipmentV82::RoleDescription(B,Role,0),NationalIntroductionTacticV827(Role));
        }
        for(const auto Kind:FrontlineSupportCards)
        {
            if(WarfareFRRUEquipmentV82::FirstMissionForSupport(B,Kind,0)!=Order
                ||!WarfareFRRUEquipmentV82::SupportAvailable(B,Kind,0))continue;
            const FString Tactic=Kind==EFrontlineSupport::Smoke
                ?WarRoom52::L(TEXT("Wait for the smoke to spread before crossing. It obstructs both armies' observation and fire."),TEXT("待烟幕展开后再穿越空地。烟幕会同时遮蔽双方的观察与射击。"))
                :Kind==EFrontlineSupport::Rally
                ?WarRoom52::L(TEXT("Use the rally order when the line is under pressure. Keep troops in cover while they recover."),TEXT("前沿承受重压时下达集结命令。部队恢复期间，仍须利用掩体。"))
                :WarRoom52::L(TEXT("Fire support arrives after the order is relayed. Keep friendly troops clear of the target area."),TEXT("命令传达后，炮火才会抵达。让友军离开目标地域。"));
            Cards.Add({WarRoom52::L(TEXT("FIRE SUPPORT ATTACHMENT"),TEXT("新增火力配属")),
                B->GetSupportName(Kind),WarfareFRRUEquipmentV82::SupportDescription(B,Kind,0),Tactic,nullptr,INDEX_NONE});
            Cards.Last().Support=Kind;
        }
        for(const auto& Definition:WarfareFRRUArmourV82::Definitions())
        {
            const FString& Id=Definition.Id;
            const auto* D=WarfareFRRUArmourV82::Definition(Id);
            const int32 Date=WarfareFRRUCampaignV82::CalendarDate(B);
            // FirstMissionForArmour checks this nation's real assignment and
            // dates. A railway need not already be instantiated in the room
            // for its allocation notice; actual dispatch still checks track.
            if(!D||WarfareFRRUArmourV82::FirstMissionForArmour(B,Id)!=Order
                ||Date<D->FirstDate||Date>D->LastDate||!WarfareFRRUArmourV82::Accepted(B,Id))continue;
            FUnitIntroductionCard Card;
            Card.Heading=WarRoom52::L(TEXT("ARMOURED ATTACHMENT"),TEXT("装甲小队配属"));
            Card.Title=D->Name;
            Card.Context=WarfareFRRUTextV82::Resolve(Id,TEXT("description"),D->Description,D->Description);
            Card.Tactic=D->Wheeled
                ?WarRoom52::L(TEXT("Keep to firm routes and use the two turrets to cover the infantry. Withdraw from field guns before the wheels are disabled."),TEXT("沿坚实道路机动，以双炮塔掩护步兵。遇到野炮应及时撤离，避免车轮受损后被困在火力下。"))
                :WarRoom52::L(TEXT("Advance with infantry covering the flanks. Keep a clear route behind the tank; damaged tracks leave it exposed to enemy guns."),TEXT("与步兵一同推进，让步兵掩护侧翼。车后应留有退路，履带受损的战车容易成为敌炮的目标。"));
            Card.NationalArmour=Id;Cards.Add(MoveTemp(Card));
        }
        return Cards;
    }
    const auto Intro=B->GetMissionIntroduction();
    AddRoleCard(Intro.Role,Intro.Heading,Intro.Title,Intro.Context,Intro.Tactic);
    const int32 First=FrontlineUnlocksV53::IntroductionFirstOperation(B);
    const auto Arrives=[&](int32 Operation){return Operation>=First&&Operation<=Mission->Id;};
    // A chapter may attach two roles at once (German Wex and pioneers). The
    // single legacy introduction role must not silently discard the other.
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
    {
        const auto Role=EFrontlineRole(I);
        if(FrontlineGameplayV52::SelectableRole(Role)&&Arrives(B->RoleUnlockMission(Role))&&!(Mission->DisabledRoleMask&(1u<<I))&&Role!=Intro.Role&&B->IsRoleUnlocked(Role))
            AddRoleCard(Role,TEXT("本连配属命令"),B->GetRecruitName(Role),FrontlinePlayerGuideV43::Unit(B,Role),Mission->AlliedContext);
    }
    for(int32 I=0;I<UE_ARRAY_COUNT(FrontlineSupportCards);++I)
    {
        const auto Kind=FrontlineSupportCards[I];if(!Arrives(B->SupportUnlockMission(Kind))||!B->IsSupportUnlocked(Kind))continue;
        Cards.Add({TEXT("新增火力配属"),B->GetSupportName(Kind),B->GetSupportDescription(Kind),
            Kind==EFrontlineSupport::Smoke?TEXT("烟幕遮住双方射界。待烟幕展开后，部队可穿越空地或撤离暴露的枪位。"):
            TEXT("炮火接令后才会抵达。目标地域内的友军同样面临危险。"),nullptr,INDEX_NONE});
        Cards.Last().Support=Kind;
    }
    if(auto* Vehicles=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& Entry:Vehicles->GetRoster())
        if(Arrives(Vehicles->UnlockMission(Entry.Type))&&Vehicles->IsUnlocked(Entry.Type))
            Cards.Add({Entry.Type==EFrontlineVehicle::RailConvoy?WarRoom52::L(TEXT("RAIL DETACHMENT"),TEXT("铁路分遣队配属")):TEXT("装甲分队配属"),Entry.Name,FrontlinePlayerGuideV43::Armour(Entry.Type),
                Entry.Type==EFrontlineVehicle::RailConvoy?WarRoom52::L(TEXT("Select the train in the armour allocation before departure. Call it from its ordinary armour card; the second slot remains available for a tank."),TEXT("出发前，在装甲配属中选入列车。战斗中从对应装甲栏呼叫；另一个栏位仍可配属战车。")):
                TEXT("步兵掩护战车侧翼。履带受损会拖慢推进；失效武器需要车组抢修。"),Vehicles->GetPortrait(Entry.Type)});
    return Cards;
}
bool DismissNationalAssignmentV87(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    if(!PC||!B)return false;auto& S=ScreenState(PC);
    if(!S.NationalAssignmentNoticeV87)return false;
    if(WarfareFRRUCampaignV82::Active(B))
    {S.AcceptedNationalIntroCampaignV827=WarfareFRRUCampaignV82::CampaignId(B);S.AcceptedNationalIntroMissionV827=WarfareFRRUCampaignV82::MissionId(B);}
    S.NationalAssignmentNoticeV87=false;S.UnitIntroduction=false;S.UnitIntroductionPage=0;return true;
}
void ReleaseNationalAssignmentAfterReportV87(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,int32 CompletedHostMission)
{
    if(!PC||!B)return;auto& S=ScreenState(PC);S.NationalAssignmentNoticeV87=false;
    if(CompletedHostMission==INDEX_NONE||B->Phase!=EFrontlinePhase::Briefing
        ||!WarfareFRRUCampaignV82::Active(B)||WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))return;
    const auto& Missions=WarfareFRRUCampaignV82::Missions(B);const auto* Next=B->GetSelectedCampaignMission();
    const int32 WonOrder=Missions.IndexOfByPredicate([&](const auto& M){return M.Id==CompletedHostMission;});
    const int32 NextOrder=Next?Missions.IndexOfByPredicate([&](const auto& M){return M.Id==Next->Id;}):INDEX_NONE;
    if(WonOrder==INDEX_NONE||NextOrder!=WonOrder+1)return;
    const FGuid Campaign=WarfareFRRUCampaignV82::CampaignId(B);const FString Mission=WarfareFRRUCampaignV82::MissionId(B);
    if(S.AcceptedNationalIntroCampaignV827==Campaign&&S.AcceptedNationalIntroMissionV827==Mission)return;
    if(UnitIntroductionCards(B).IsEmpty())
    {S.AcceptedNationalIntroCampaignV827=Campaign;S.AcceptedNationalIntroMissionV827=Mission;return;}
    S.NationalAssignmentNoticeV87=true;S.UnitIntroduction=true;S.UnitIntroductionPage=0;S.Deployment=false;S.Preparing=false;
}
void ConfirmUnitIntroductionV27(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    auto& S=ScreenState(PC);if(S.Preparing||!S.UnitIntroduction||B->Phase!=EFrontlinePhase::Briefing||!B->GetSelectedCampaignMission())return;
    const auto Cards=UnitIntroductionCards(B);
    if(S.UnitIntroductionPage+1<Cards.Num()){++S.UnitIntroductionPage;return;}
    if(WarfareFRRUCampaignV82::Active(B))
    {S.AcceptedNationalIntroCampaignV827=WarfareFRRUCampaignV82::CampaignId(B);S.AcceptedNationalIntroMissionV827=WarfareFRRUCampaignV82::MissionId(B);}
    // The earned-allocation sheet belongs to the return to the war room.
    // Its last acknowledgement must not silently start another deployment.
    if(DismissNationalAssignmentV87(PC,B))return;
    S.UnitIntroduction=false;BeginOperationPreparation(PC,B);
}
void DrawUnitIntroductionV27(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FFrontlineScreenState& S)
{
    const auto Cards=UnitIntroductionCards(B);if(Cards.IsEmpty())return;
    S.UnitIntroductionPage=FMath::Clamp(S.UnitIntroductionPage,0,Cards.Num()-1);const auto& Intro=Cards[S.UnitIntroductionPage];
    const bool National=WarfareFRRUCampaignV82::Active(B);
    const int32 Nation=int32(B->GetPlayerFaction());auto& A=Artwork();
    UTexture2D* Backdrop=A.BackgroundsV71[Nation].Texture?A.BackgroundsV71[Nation].Texture.Get():Nation==1&&A.GermanBackgroundV30?A.GermanBackgroundV30.Get():A.Keyart[0].Get();
    if(National)Backdrop=NationalTextureV82(WarfareFRRUCampaignV82::Nation(B),TEXT("Cover"));
    UI.ImageCover(Backdrop,0,0,1920,1080,FLinearColor(.18f,.21f,.23f,1));UI.Hit(0,0,1920,1080,TEXT("Blocked"),80);
    UI.PageSurfaceV73(96,64,1728,968,true);const FLinearColor Ink(.032f,.036f,.033f),Faded(.14f,.15f,.13f);
    if(S.NationalAssignmentNoticeV87)UI.BilingualLabel(TEXT("NEXT ASSIGNMENT"),TEXT("下一道配属命令"),144,100,1040,48,21,Faded);
    else UI.BilingualLabel(Intro.Heading,144,100,1040,48,21,Faded);
    const FString Army=National?(WarfareFRRUCampaignV82::Nation(B)==EFRRUNationV81::French
        ?WarRoom52::L(TEXT("FRENCH ARMY"),TEXT("法国陆军")):WarRoom52::L(TEXT("IMPERIAL RUSSIAN ARMY"),TEXT("俄罗斯帝国陆军"))):FString(B->GetFactionName());
    UI.RoleLabel(EFrontlineTextRoleV8210::Plate,Army,1312,100,464,48,20,Faded,2);
    UI.Line(144,184,1776,184,Faded*.45f);
    if(!Intro.NationalArmour.IsEmpty())FRRUArmourUIV82::Portrait(UI,Intro.NationalArmour,144,286,440,false);
    else if(Intro.Support!=EFrontlineSupport::Count)DrawSupportPhotoV32(UI,B,Intro.Support,144,286,440,440);
    else if(Intro.Role!=EFrontlineRole::Count)DrawRolePhotoV32(UI,B,Intro.Role,144,240,440,560,false,EFrontlinePortraitContextV72::Narrative);
    else if(Intro.Illustration>=0)V32Atlas(UI,Artwork().IllustrationsV32,Intro.Illustration,6,4,144,240,440,560);
    else UI.ImageContain(Intro.Portrait,144,240,440,560,FLinearColor(.94f,.92f,.86f,1));
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    const float ProseW=Chinese?FMath::Min(1128.f,UI.RoleWidth(EFrontlineTextRoleV8210::Text,TEXT("国"),21)*30.f):1128.f;
    UI.BilingualTitle(Intro.Title,648,232,1128,66,Chinese?34.f:49.f,Ink);
    UI.Description(Intro.Context,648,334,ProseW,260,21,Ink);
    UI.Line(648,626,1776,626,Faded*.45f);
    if(National)UI.BilingualLabel(TEXT("IN THE FIELD"),TEXT("临战须知"),648,650,1128,46,21,Faded);
    else UI.BilingualLabel(TEXT("军士的叮嘱"),648,650,1128,46,21,Faded);
    UI.RoleParagraph(EFrontlineTextRoleV8210::Objective,Intro.Tactic,648,722,ProseW,144,21,Ink);
    UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,FString::Printf(TEXT("%d / %d"),S.UnitIntroductionPage+1,Cards.Num()),144,838,440,36,25,Faded,1);
    UI.Line(144,900,1776,900,Faded*.4f);
    UI.PageButtonV73(144,936,380,64,S.UnitIntroductionPage>0?WarRoom52::L(TEXT("PREVIOUS"),TEXT("上一页")):WarRoom52::L(TEXT("RETURN TO ORDERS"),TEXT("返回命令图")),TEXT("Esc"),TEXT("UnitIntroductionBack"),true,false,90);
    UI.PageButtonV73(1236,936,540,64,S.UnitIntroductionPage+1<Cards.Num()?WarRoom52::L(TEXT("CONTINUE"),TEXT("继续阅读")):S.NationalAssignmentNoticeV87?WarRoom52::L(TEXT("RETURN TO OPERATIONS"),TEXT("返回作战室")):WarRoom52::L(TEXT("READY TO MOVE"),TEXT("明白，准备出发")),TEXT("Enter"),TEXT("UnitIntroductionBegin"),true,true,90);
}
