// Physical posts and existing squads own these orders. Screen-space panels
// only submit explicit native commands; they never award arrivals or progress.
struct FFRRUOrderNoticeV82
{
    FGuid Encounter;
    FString LastReason,Text;
    double Until=0;
};
FFRRUOrderNoticeV82& FRRUOrderNoticeV82(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    static TMap<TWeakObjectPtr<AWarfareFrontlineController>,FFRRUOrderNoticeV82> Notices;
    auto& Notice=Notices.FindOrAdd(PC);
    if(Notice.Encounter!=B->GetEncounterId()){Notice=FFRRUOrderNoticeV82();Notice.Encounter=B->GetEncounterId();}
    return Notice;
}
FString FRRUOrderBlockTextV82(const FString& Reason)
{
    if(Reason.Contains(TEXT("not purchased")))return WarRoom52::L(TEXT("Develop this order in the operations room first."),TEXT("请先在作战室完成此项研发。"));
    if(Reason.Contains(TEXT("Both drafts"))||Reason.Contains(TEXT("Fork requires")))return WarRoom52::L(TEXT("Select two observers with sight of the primary and alternate targets."),TEXT("请选择两支观察组，分别观察主目标与备选目标。"));
    if(Reason.Contains(TEXT("counterroute stencil")))return WarRoom52::L(TEXT("Choose an alternate target while the fire plan is still being prepared."),TEXT("请在炮击计划批准前选定备选目标。"));
    if(Reason.Contains(TEXT("Two unreleased"))||Reason.Contains(TEXT("Requested real role")))return WarRoom52::L(TEXT("Assemble a rifle section and an assault section before choosing which leads."),TEXT("请先将步枪组和突击组编入预备队，再决定先后次序。"));
    if(Reason.Contains(TEXT("registered reserve"))||Reason.Contains(TEXT("registered real reserve axis")))return WarRoom52::L(TEXT("Register the selected section as a reserve on this approach first."),TEXT("请先将所选小队编入预备队，并指定这条接近路。"));
    if(Reason.Contains(TEXT("Anchor relocation")))return WarRoom52::L(TEXT("Select a machine-gun section to change its firing position."),TEXT("请选择机枪组，再更换射击阵地。"));
    if(Reason.Contains(TEXT("Followup phase"))||Reason.Contains(TEXT("No unused committed"))||Reason.Contains(TEXT("Actual salvo phase")))return WarRoom52::L(TEXT("Assign the follow-up before firing; advance when the chosen salvo lands."),TEXT("请在开火前指定跟进部队，待选定的齐射落下后投入。"));
    if(Reason.Contains(TEXT("No editable")))return WarRoom52::L(TEXT("Prepare a fire plan before changing its orders."),TEXT("请先拟定炮击计划，再调整部署。"));
    if(Reason.Contains(TEXT("Existing squad required")))return WarRoom52::L(TEXT("Select one living section for this order."),TEXT("请选择一支仍有人员的小队。"));
    if(Reason.Contains(TEXT("slot"))||Reason.Contains(TEXT("capacity"))||Reason.Contains(TEXT("formation bay")))return WarRoom52::L(TEXT("This position is full. Move a section out before sending another."),TEXT("此处已满，请先调走一支小队。"));
    if(Reason.Contains(TEXT("stationary"))||Reason.Contains(TEXT("entered rear squad")))return WarRoom52::L(TEXT("Wait for the section to arrive and halt before giving this order."),TEXT("请等小队到达并停驻后再下令。"));
    if(Reason.Contains(TEXT("Fresh completed observation"))||Reason.Contains(TEXT("observer"))||Reason.Contains(TEXT("visible proof"))||Reason.Contains(TEXT("observed")))return WarRoom52::L(TEXT("Complete a fresh observation of the target before firing."),TEXT("请先完成对当前目标的观察，再下令开火。"));
    if(Reason.Contains(TEXT("reload")))return WarRoom52::L(TEXT("Wait for the assigned crew to finish reloading."),TEXT("请等指定枪组装填完毕。"));
    if(Reason.Contains(TEXT("interval not ready")))return WarRoom52::L(TEXT("The next reserve section is still assembling. Wait before releasing it."),TEXT("下一支预备队仍在集结，请稍候再投入。"));
    if(Reason.Contains(TEXT("No unreleased"))||Reason.Contains(TEXT("No existing reserve")))return WarRoom52::L(TEXT("No reserve is waiting. Select a halted section at the rear and hold it in reserve."),TEXT("目前没有待命预备队，请选择后方停驻小队编入预备队。"));
    if(Reason.Contains(TEXT("frozen existing pioneer"))||Reason.Contains(TEXT("frozen pioneer")))return WarRoom52::L(TEXT("Send the original pioneer section to the recovery point."),TEXT("请派原工兵小队前往补救地点。"));
    if(Reason.Contains(TEXT("participant/role"))||Reason.Contains(TEXT("real pioneer")))return WarRoom52::L(TEXT("Select the section required for this objective."),TEXT("请选择本项任务所需的小队。"));
    if(Reason.Contains(TEXT("hold gate"))||Reason.Contains(TEXT("held objective"))||Reason.Contains(TEXT("hold participants")))return WarRoom52::L(TEXT("Complete the current hold before withdrawing this section."),TEXT("请完成当前守备任务后再撤出此小队。"));
    if(Reason.Contains(TEXT("wire")))return WarRoom52::L(TEXT("The passage is blocked. Send pioneers to clear it."),TEXT("通道尚未清通，请派工兵作业。"));
    if(Reason.Contains(TEXT("route"))||Reason.Contains(TEXT("opposed intermediate")))return WarRoom52::L(TEXT("This route is blocked. Choose another approach or secure the position ahead."),TEXT("此路线受阻，请改走其他接近路，或先夺取前方阵地。"));
    if(Reason.Contains(TEXT("draft"))||Reason.Contains(TEXT("French fixed artillery corridor")))return WarRoom52::L(TEXT("Prepare a fire plan and observe its target before committing it."),TEXT("请先拟定炮击计划并观察目标，再批准开火。"));
    if(Reason.Contains(TEXT("Support unavailable")))return WarRoom52::L(TEXT("This support is not ready. Check the assigned support and its ammunition."),TEXT("此支援尚未就绪，请检查配属和剩余弹药。"));
    return WarRoom52::L(TEXT("The order could not be carried out. Check the selected section and target."),TEXT("此命令暂时无法执行，请检查所选小队与目标。"));
}
void FRRUOrderFeedbackV82(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,bool Accepted)
{
    if(Accepted)return;
    auto& Notice=FRRUOrderNoticeV82(PC,B);Notice.LastReason=WarfareFRRUWorldV82::View(B).Notice;
    // View.Notice has already been translated by the command receiver.
    Notice.Text=Notice.LastReason;Notice.Until=FPlatformTime::Seconds()+4.;
}
FString FRRUPostNameV82(const FString& Id,const AWarfareFrontlineGameMode* B=nullptr)
{
    if(B&&Id==TEXT("bridge")&&WarfareFRRUCampaignV82::MissionId(B)==TEXT("fr.mort_homme"))return TEXT("FLANK FIRING STEP");
    static const TCHAR* Keys[]={TEXT("home"),TEXT("assembly"),TEXT("ridge"),TEXT("forest"),TEXT("front_left"),TEXT("front_right"),TEXT("relay"),TEXT("bridge"),TEXT("wire"),TEXT("rail"),TEXT("objective"),TEXT("exit"),TEXT("enemy_assembly")};
    static const TCHAR* Labels[]={TEXT("REAR LINE"),TEXT("ASSEMBLY"),TEXT("RIDGE"),TEXT("WOOD LINE"),TEXT("LEFT APPROACH"),TEXT("RIGHT APPROACH"),TEXT("LIAISON POST"),TEXT("CROSSING"),TEXT("WIRE GAP"),TEXT("RAIL CUTTING"),TEXT("FORWARD POSITION"),TEXT("WITHDRAWAL ROUTE"),TEXT("ENEMY RESERVE")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Keys);++I)if(Id==Keys[I])return Labels[I];return FString();
}
const wfexp::tactical::StageDefinition* FRRUCurrentStageV827(const FFRRUWorldViewV82& V,int32* Index=nullptr,int32* Count=nullptr)
{
    const auto* Mission=wfexp::tactical::FindMission(wfexpue::Narrow(V.MissionId));
    if(!Mission)return nullptr;
    if(Count)*Count=int32(Mission->stages.size());
    for(int32 I=0;I<int32(Mission->stages.size());++I)if(wfexpue::Wide(Mission->stages[I].id)==V.StageId)
    {if(Index)*Index=I;return &Mission->stages[I];}
    return nullptr;
}
FString FRRUPostProseV827(const FString& Id)
{
    static const TCHAR* Keys[]={TEXT("home"),TEXT("assembly"),TEXT("ridge"),TEXT("forest"),TEXT("front_left"),TEXT("front_right"),TEXT("relay"),TEXT("bridge"),TEXT("wire"),TEXT("rail"),TEXT("objective"),TEXT("exit")};
    static const TCHAR* EN[]={TEXT("the rear line"),TEXT("the assembly point"),TEXT("the ridge"),TEXT("the wood line"),TEXT("the left approach"),TEXT("the right approach"),TEXT("the liaison post"),TEXT("the crossing"),TEXT("the wire gap"),TEXT("the rail cutting"),TEXT("the forward position"),TEXT("the withdrawal route")};
    static const TCHAR* ZH[]={TEXT("后方阵地"),TEXT("集合点"),TEXT("山脊"),TEXT("林缘"),TEXT("左侧接近路"),TEXT("右侧接近路"),TEXT("联络岗"),TEXT("渡口"),TEXT("铁丝网缺口"),TEXT("铁路切口"),TEXT("前沿阵地"),TEXT("撤退通路")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Keys);++I)if(Id==Keys[I])return WarRoom52::L(EN[I],ZH[I]);
    return FRRUPostNameV82(Id);
}
FString FRRUObjectiveTitleV827(const FFRRUWorldViewV82& V)
{
    const auto* Stage=FRRUCurrentStageV827(V);
    if(!Stage)return FRRUPostNameV82(V.TargetNode);
    const TCHAR* Verb=TEXT("SECURE");
    if(V.FallbackAwaitingArrival||Stage->kind==wfexp::tactical::GateKind::WithdrawToNode)Verb=TEXT("FALL BACK");
    else if(Stage->kind==wfexp::tactical::GateKind::HoldNode)Verb=TEXT("HOLD");
    else if(Stage->kind==wfexp::tactical::GateKind::ClearPassage)Verb=TEXT("OPEN");
    return FString(Verb)+TEXT("  ·  ")+FRRUPostNameV82(V.TargetNode);
}
FString FRRUObjectiveInstructionsV827(const FFRRUWorldViewV82& V)
{
    const auto* Stage=FRRUCurrentStageV827(V);if(!Stage)return FString();
    const FString Post=FRRUPostProseV827(V.TargetNode);FString Text;
    if(V.FallbackAwaitingArrival)
        Text=WarRoom52::L(TEXT("Re-form at %s. The original position can no longer be held."),TEXT("退至%s重新集结，原阵地已无法坚守。"));
    else switch(Stage->kind)
    {
    case wfexp::tactical::GateKind::HoldNode:
        Text=WarRoom52::L(TEXT("Keep a surviving section at %s and drive the enemy out. The hold advances while the position is secure."),TEXT("派幸存小队坚守%s，赶走进入阵地的敌军。控制阵地后，坚守进度才会增加。"));break;
    case wfexp::tactical::GateKind::ClearPassage:
        Text=WarRoom52::L(TEXT("Send pioneers to %s. Cover them while they cut a passage through the obstruction."),TEXT("令工兵抵达%s，掩护他们清除障碍、打通道路。"));break;
    case wfexp::tactical::GateKind::WithdrawToNode:
        Text=WarRoom52::L(TEXT("Bring a surviving section from the front to %s. Fresh replacements cannot fulfil this withdrawal."),TEXT("将前沿的幸存小队撤至%s。新征募的补充队不能代替他们完成撤退。"));break;
    default:
        Text=WarRoom52::L(TEXT("Advance a surviving section to %s. The destination must be clear of enemy troops."),TEXT("令幸存小队推进至%s，清除目标阵地上的敌军。"));break;
    }
    Text.ReplaceInline(TEXT("%s"),*Post);
    for(const auto& Required:Stage->mustOwn)
    {
        FString Line=WarRoom52::L(TEXT("Keep %s under friendly control."),TEXT("同时守住%s。"));
        Line.ReplaceInline(TEXT("%s"),*FRRUPostProseV827(wfexpue::Wide(Required)));
        Text.AppendChar(TEXT('\n'));Text+=Line;
    }
    return Text;
}
bool FRRUSelectsTargetV82(const FFrontlineScreenState& S,const FFRRUWorldViewV82& V)
{return S.FRRUOptionsOpenV82||(V.FirePhase>0&&V.FirePhase<4)||V.ReservePending;}
FString FRRUOptionVariantV82(bool French,int32 Index,int32 Variant)
{
    const bool Alternate=Variant==1;
    switch(Index)
    {
    case 0:return WarRoom52::L(Alternate?TEXT("Hold this section here."):TEXT("Release the hold on this section."),Alternate?TEXT("命小队留守此处。"):TEXT("解除此小队的留守命令。"));
    case 1:return WarRoom52::L(Alternate?TEXT("Wait for the selected crews to reload."):TEXT("Proceed without waiting for reload."),Alternate?TEXT("等所选枪组装填完毕。"):TEXT("不等待装填。"));
    case 2:return French?WarRoom52::L(Alternate?TEXT("Last salvo: alternate target."):TEXT("All salvos: primary target."),Alternate?TEXT("末轮齐射转向备选目标。"):TEXT("全部齐射集中于主目标。")):
        WarRoom52::L(Alternate?TEXT("Split the reserves between both approaches."):TEXT("Send both reserves along the primary approach."),Alternate?TEXT("两支预备队分走两条接近路。"):TEXT("两支预备队沿主接近路投入。"));
    case 3:return French?WarRoom52::L(Alternate?TEXT("Choose the alternate observed target."):TEXT("Choose the primary observed target."),Alternate?TEXT("采用备选观察目标。"):TEXT("采用主观察目标。")):
        WarRoom52::L(TEXT("Artillery on the registered primary reserve approach."),TEXT("野炮轰击预备队已指定的主接近路。"));
    case 4:return French?WarRoom52::L(Alternate?TEXT("Follow the second salvo."):TEXT("Follow the final salvo."),Alternate?TEXT("第二轮齐射后跟进。"):TEXT("末轮齐射后跟进。")):
        WarRoom52::L(Alternate?TEXT("Assault section leads."):TEXT("Rifle section leads."),Alternate?TEXT("突击组先行。"):TEXT("步枪组先行。"));
    case 6:return French?WarRoom52::L(TEXT("Assign the selected sections to the current barrage."),TEXT("指定所选小队跟随当前炮击计划。")):
        WarRoom52::L(TEXT("Register the selected sections on the primary reserve approach."),TEXT("将所选小队编入预备队，指定主接近路。"));
    case 8:return French?WarRoom52::L(Alternate?TEXT("Choose the alternate observed target."):TEXT("Choose the primary observed target."),Alternate?TEXT("采用备选观察目标。"):TEXT("采用主观察目标。")):
        WarRoom52::L(TEXT("Report control of the two observed positions."),TEXT("报告两处已观察阵地的控制情况。"));
    case 9:return WarRoom52::L(Alternate?TEXT("Smoke: alternate target."):TEXT("Smoke: primary target."),Alternate?TEXT("烟幕投向备选目标。"):TEXT("烟幕投向主目标。"));
    default:return WarRoom52::L(TEXT("Apply this order to the selected sections and primary target."),TEXT("对所选小队与主目标下达此命令。"));
    }
}
void FRRUSelectionV82(AWarfareFrontlineGameMode* B,FFrontlineScreenState& S)
{
    if(S.FRRUSelectionEncounterV82!=B->GetEncounterId())
    {S.FRRUSelectionEncounterV82=B->GetEncounterId();S.FRRUSelectedSquadsV82.Empty();S.FRRUOptionsOpenV82=false;S.FRRUTargetV82.Empty();S.FRRUAlternateV82.Empty();}
    S.FRRUSelectedSquadsV82.RemoveAll([&](int32 Id){const auto* Q=B->FindSquad(Id);return !Q||Q->Team!=0||Q->LivingCount()<=0;});
}
bool FRRUWorldHitV82(const FName& Box,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    const FString Name=Box.ToString();if(!Name.StartsWith(TEXT("FRRUWorld:")))return false;
    auto& S=ScreenState(PC);if(!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting||S.Preparing||S.Menu||S.Settings||S.Help||S.Technology||S.AchievementsV76)return true;
    FRRUSelectionV82(B,S);const auto V=WarfareFRRUWorldV82::View(B);const FString Action=Name.Mid(10);
    if(V.StoryPending&&!Action.StartsWith(TEXT("story:")))return true;
    const FString Target=S.FRRUTargetV82.IsEmpty()?V.TargetNode:S.FRRUTargetV82;
    if(Action.StartsWith(TEXT("squad:")))
    {const int32 Id=FCString::Atoi(*Action.Mid(6));const auto* Q=B->FindSquad(Id);if(Q&&Q->Team==0&&Q->LivingCount()>0){const bool Add=PC->IsInputKeyDown(EKeys::LeftControl)||PC->IsInputKeyDown(EKeys::RightControl);if(!Add)S.FRRUSelectedSquadsV82.Empty();if(S.FRRUSelectedSquadsV82.Contains(Id))S.FRRUSelectedSquadsV82.Remove(Id);else if(S.FRRUSelectedSquadsV82.Num()<2)S.FRRUSelectedSquadsV82.Add(Id);}}
    else if(Action.StartsWith(TEXT("post:")))
    {const FString Id=Action.Mid(5);if(PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift))S.FRRUAlternateV82=Id;else{S.FRRUTargetV82=Id;if(!FRRUSelectsTargetV82(S,V))for(int32 Squad:S.FRRUSelectedSquadsV82)FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::MoveSquad(B,Squad,Id));}}
    else if(Action.StartsWith(TEXT("story:")))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::ChooseStory(B,V.StoryId,Action.Mid(6)));
    else if(Action==TEXT("objective"))
    {for(int32 Id:S.FRRUSelectedSquadsV82)FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,V.FallbackAwaitingArrival?EFRRUTacticalCommandV82::PolicyOrder:EFRRUTacticalCommandV82::AdvanceToObjective,{Id},V.TargetNode));}
    else if(Action==TEXT("move")){for(int32 Id:S.FRRUSelectedSquadsV82)FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::MoveSquad(B,Id,Target));}
    else if(Action==TEXT("hold")){for(int32 Id:S.FRRUSelectedSquadsV82)if(const auto* Group=B->FindSquad(Id))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::HoldSquad(B,Id,!Group->bHoldPosition));}
    else if(Action==TEXT("reserve"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::RegisterReserve,S.FRRUSelectedSquadsV82,Target));
    else if(Action==TEXT("release"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::ReleaseReserve));
    else if(Action==TEXT("plan"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::PlanFire,{},Target,S.FRRUAlternateV82));
    else if(Action==TEXT("observe"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::Observe,S.FRRUSelectedSquadsV82));
    else if(Action==TEXT("commit"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::CommitFire));
    else if(Action==TEXT("cancel"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::CancelFire));
    else if(Action==TEXT("followup"))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::Followup,S.FRRUSelectedSquadsV82));
    else if(Action==TEXT("options"))S.FRRUOptionsOpenV82=!S.FRRUOptionsOpenV82;
    else if(Action==TEXT("variant"))S.FRRUVariantV82=1-S.FRRUVariantV82;
    else if(Action.StartsWith(TEXT("use:")))FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::Command(B,EFRRUTacticalCommandV82::UseOption,S.FRRUSelectedSquadsV82,Target,Action.Mid(4),FString(),S.FRRUVariantV82,0,S.FRRUAlternateV82));
    return true;
}
bool FRRUWorldClickV82(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    if(!WarfareFRRUCampaignV82::Active(B))return false;if(FRRUStoryModalV82(B))return true;auto& S=ScreenState(PC);FRRUSelectionV82(B,S);
    float MX=0,MY=0;if(!PC->GetMousePosition(MX,MY))return true;
    int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);const float Scale=FMath::Min(VX/1920.f,VY/1080.f);const FVector2D Pointer(MX,MY);
    const bool Add=PC->IsInputKeyDown(EKeys::LeftControl)||PC->IsInputKeyDown(EKeys::RightControl);
    const FFrontlineSquad* Pick=nullptr;double Best=FMath::Square(58.f*Scale);
    for(const auto& Group:B->Squads)if(Group.Team==0&&Group.LivingCount()>0)
    {bool Entered=true;for(const auto& W:Group.Members)if(const auto* U=W.Get();U&&U->IsAlive())Entered&=U->bFieldEntered;if(!Entered)continue;FVector2D P;if(PC->ProjectWorldLocationToScreen(Group.Centre()+FVector(0,0,100),P)){const double D=FVector2D::DistSquared(P,Pointer);if(D<Best){Best=D;Pick=&Group;}}}
    if(Pick)
    {if(!Add)S.FRRUSelectedSquadsV82.Empty();if(S.FRRUSelectedSquadsV82.Contains(Pick->Id))S.FRRUSelectedSquadsV82.Remove(Pick->Id);else if(S.FRRUSelectedSquadsV82.Num()<2)S.FRRUSelectedSquadsV82.Add(Pick->Id);return true;}
    const auto V=WarfareFRRUWorldV82::View(B);const FFRRUNodeV82* Post=nullptr;Best=FMath::Square(78.f*Scale);
    for(const auto& N:V.Nodes)if(N.Id!=TEXT("enemy_assembly"))
    {FVector2D P;if(PC->ProjectWorldLocationToScreen(N.Position+FVector(0,0,70),P)){const double D=FVector2D::DistSquared(P,Pointer);if(D<Best){Best=D;Post=&N;}}}
    if(Post)
    {if(PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift))S.FRRUAlternateV82=Post->Id;else {S.FRRUTargetV82=Post->Id;if(!FRRUSelectsTargetV82(S,V))for(int32 Id:S.FRRUSelectedSquadsV82)FRRUOrderFeedbackV82(PC,B,WarfareFRRUWorldV82::MoveSquad(B,Id,Post->Id));}return true;}
    S.FRRUSelectedSquadsV82.Empty();return true;
}
void DrawFRRUWorldV82(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC)
{
    TGuardValue<bool> MapTypographyScopeV8210(UI.bLegacyTypographyV8210,true);
    if(!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting)return;auto& S=ScreenState(PC);if(S.Menu||S.Settings||S.Help||S.Technology||S.Preparing||S.AchievementsV76)return;
    FRRUSelectionV82(B,S);const auto V=WarfareFRRUWorldV82::View(B);const bool French=WarfareFRRUCampaignV82::Nation(B)==EFRRUNationV81::French;
    const FLinearColor Ink(.033f,.037f,.034f);
    if(V.StoryPending)
    {
        const float StoryX=48,StoryY=177;UI.PaperSheet(StoryX,StoryY,580,548);UI.Hit(StoryX,StoryY,580,548,TEXT("Blocked"),120);
        UI.Paragraph(V.StoryTitle,StoryX+26,StoryY+22,528,78,28,Ink);UI.Paragraph(V.StoryBody,StoryX+26,StoryY+116,528,145,21,Ink,true);
        for(int32 I=0;I<V.ChoiceIds.Num()&&I<2;++I)
        {const float CY=StoryY+286+I*113;UI.PageButtonV73(StoryX+26,CY,528,48,V.ChoiceTitles[I],TEXT(""),FName(*(TEXT("FRRUWorld:story:")+V.ChoiceIds[I])),true,false,130);UI.Paragraph(V.ChoiceDescriptions[I],StoryX+29,CY+53,522,53,17,Ink,true);}
        UI.Hit(1552,DockTop+36,224,52,TEXT("Pause"),130);
        UI.Hit(1552,DockTop+164,224,52,TEXT("Menu"),130);
        return;
    }

    for(const auto& N:V.Nodes)
    {
        if(N.Id==TEXT("enemy_assembly"))continue;FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(N.Position+FVector(0,0,100),Screen))continue;const auto P=UI.Local(Screen);
        if(P.X<45||P.X>1875||P.Y<110||P.Y>DockTop-55)continue;
        const bool Target=N.Id==V.TargetNode,Hover=UI.Hovered(P.X-62,P.Y-45,124,90),Selected=N.Id==S.FRRUTargetV82;
        if(N.Owner>=0)UI.Image(NationalTextureV82(WarfareFRRUCampaignV82::Nation(B,N.Owner),TEXT("Flag")),P.X-15,P.Y-38,30,19);
        if(Target||Hover||Selected)
        {UI.Skin(1,P.X-96,P.Y-13,192,32,5);UI.PrimaryLabel(FRRUPostNameV82(N.Id,B),P.X-89,P.Y-10,178,25,15,Target?Amber:Paper,1);if(Hover)UI.Hit(P.X-96,P.Y-48,192,70,FName(*(TEXT("FRRUWorld:post:")+N.Id)),85);}
    }
    for(const auto& Group:B->Squads)if(Group.Team==0&&Group.LivingCount()>0)
    {FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(Group.Centre()+FVector(0,0,100),Screen))continue;const auto P=UI.Local(Screen);if(P.X>35&&P.X<1885&&P.Y>148&&P.Y<DockTop-75&&UI.Hovered(P.X-34,P.Y-40,68,72))UI.Hit(P.X-34,P.Y-40,68,72,FName(*FString::Printf(TEXT("FRRUWorld:squad:%d"),Group.Id)),100);}
    // The objective's live state is shown once; story prose stays on its order
    // sheet, while progress remains a compact engraved field instrument.
    UI.Skin(1,665,91,590,44,6);UI.PrimaryLabel(FRRUObjectiveTitleV827(V),679,97,562,29,19,Paper,1);
    int32 StageIndex=0,StageCount=1;FRRUCurrentStageV827(V,&StageIndex,&StageCount);
    const float OperationProgress=V.Finished&&V.Success?1.f:(StageIndex+FMath::Clamp(V.Progress,0.f,1.f))/FMath::Max(1,StageCount);
    UI.Rect(679,134,562,4,Dim);UI.Rect(679,134,562*OperationProgress,4,Amber);
    if(UI.Hovered(665,91,590,47))UI.TooltipV83(TooltipKeyV83(41),GetTypeHash(V.StageId+V.TargetNode),[&]{
        UI.PaperSheet(650,150,620,224);UI.PrimaryLabel(WarRoom52::L(TEXT("ORDERS"),TEXT("作战命令")),674,165,572,31,23,Ink,1);
        UI.Paragraph(FRRUObjectiveInstructionsV827(V),674,210,572,114,20,Ink,true);
        if(V.RequiredRole>=0)UI.Label(WarRoom52::L(TEXT("Required section: "),TEXT("所需小队："))+B->GetRecruitName(EFrontlineRole(V.RequiredRole)),674,332,572,25,18,Ink);
    });
    if(!S.FRRUSelectedSquadsV82.IsEmpty())
    {
        const auto* Group=B->FindSquad(S.FRRUSelectedSquadsV82[0]);FVector2D Screen;
        if(Group&&PC->ProjectWorldLocationToScreen(Group->Centre()+FVector(0,0,140),Screen))
        {
            const auto P=UI.Local(Screen);const float X=FMath::Clamp(float(P.X)-245.f,36.f,1394.f),Y=FMath::Clamp(float(P.Y)-160.f,160.f,DockTop-200.f);
            UI.PhysicalPanelV74(X,Y,490,134);UI.Hit(X,Y,490,134,TEXT("Blocked"),86);
            UI.PrimaryLabel(B->GetRecruitName(Group->Role),X+16,Y+12,458,28,19,Paper,1);
            UI.PageButtonV73(X+14,Y+53,147,48,TEXT("HOLD"),TEXT(""),TEXT("FRRUWorld:hold"),true,Group->bHoldPosition,90);
            UI.PageButtonV73(X+171,Y+53,147,48,TEXT("ADVANCE"),TEXT(""),TEXT("FRRUWorld:objective"),!Group->bAdvancing,false,90);
            UI.PageButtonV73(X+328,Y+53,147,48,TEXT("ORDERS"),TEXT(""),TEXT("FRRUWorld:options"),true,S.FRRUOptionsOpenV82,90);
            if(S.FRRUOptionsOpenV82)
            {
                const auto Rows=WarfareFRRUCampaignV82::Options(B);int32 Count=0;for(const auto& Row:Rows)if(Row.Owned&&!NationalArmourResearchV87(Row))++Count;
                const float H=136+Count*43.f,OY=FMath::Clamp(Y-H-10,151.f,DockTop-H-20.f);UI.PhysicalPanelV74(X,OY,490,H);UI.Hit(X,OY,490,H,TEXT("Blocked"),91);
                UI.PageButtonV73(X+16,OY+14,458,38,S.FRRUVariantV82?WarRoom52::L(TEXT("ALTERNATE"),TEXT("备选方案")):WarRoom52::L(TEXT("PRIMARY"),TEXT("主方案")),TEXT(""),TEXT("FRRUWorld:variant"),true,false,95);
                const FString Target=S.FRRUTargetV82.IsEmpty()?V.TargetNode:S.FRRUTargetV82;
                const FString Targets=FRRUPostNameV82(Target,B)+(S.FRRUAlternateV82.IsEmpty()?FString():TEXT("  /  ")+FRRUPostNameV82(S.FRRUAlternateV82,B));
                UI.PrimaryLabel(Targets,X+18,OY+57,454,24,15,Muted,1);
                const float HelpX=FMath::Clamp(X<950?X+510:X-552,36.f,1352.f);
                if(UI.Hovered(X+16,OY+14,458,67))UI.TooltipV83(TooltipKeyV83(42),0,[&]{
                    const float TX=HelpX;
                    UI.PaperSheet(TX,OY,532,222);UI.PrimaryLabel(WarRoom52::L(TEXT("TARGETS AND ORDERS"),TEXT("目标与命令")),TX+20,OY+16,492,32,22,Ink,1);
                    UI.Paragraph(WarRoom52::L(TEXT("With ORDERS open, click a post to select the primary target. Shift-click another post to select the alternate target. Switch the plan here, then choose an available order. Selecting a target leaves the section in place."),TEXT("打开命令面板后，点击阵地选择主目标；按住 Shift 点击另一阵地，选择备选目标。在此切换方案，再选择可执行的命令。选择目标时，小队留在原位。")),TX+20,OY+62,492,141,20,Ink,true);
                },FVector2D(HelpX,OY));
                int32 RowIndex=0;for(const auto& Row:Rows)if(Row.Owned&&!NationalArmourResearchV87(Row))
                {
                    const float RY=OY+91+RowIndex++*43.f;
                    const FString Reason=WarfareFRRUWorldV82::CommandBlockReason(B,EFRRUTacticalCommandV82::UseOption,S.FRRUSelectedSquadsV82,Target,Row.Id,FString(),S.FRRUVariantV82,0,S.FRRUAlternateV82);
                    UI.PageButtonV73(X+16,RY,458,38,Row.Title,TEXT(""),FName(*(TEXT("FRRUWorld:use:")+Row.Id)),Reason.IsEmpty(),false,95);
                    const float TX=HelpX,TY=FMath::Clamp(RY-78.f,150.f,DockTop-374.f);
                    if(UI.Hovered(X+16,RY,458,38))UI.TooltipV83(TooltipKeyV83(40,uint32(Row.Index)),GetTypeHash(Reason+Target+S.FRRUAlternateV82+FString::FromInt(S.FRRUVariantV82)),[&]{
                        UI.PaperSheet(TX,TY,532,356);UI.PrimaryLabel(Row.Title,TX+20,TY+16,492,32,22,Ink,1);
                        UI.Paragraph(Row.Description,TX+20,TY+62,492,116,20,Ink,true);
                        UI.Paragraph(FRRUOptionVariantV82(French,Row.Index,S.FRRUVariantV82),TX+20,TY+185,492,48,18,Ink,true);
                        UI.Paragraph(Reason.IsEmpty()?Row.Tradeoff:FRRUOrderBlockTextV82(Reason),TX+20,TY+242,492,96,18,Reason.IsEmpty()?Ink:FLinearColor(.30f,.14f,.045f),true);
                    },FVector2D(TX,TY));
                }
            }
        }
    }
    // Faction mechanics share the console's physical key surface and sounds.
    const float Y=DockTop-65;UI.PhysicalPanelV74(36,Y,680,53);
    {TGuardValue<bool> EnglishConsole(UI.bEnglishPrimaryV45,true);
    if(French)
    {
        const TCHAR* Action=V.FirePhase==0?TEXT("plan"):V.FirePhase==1?TEXT("observe"):V.FirePhase==2?TEXT("observe"):V.FirePhase==3?TEXT("commit"):TEXT("followup");
        const FString Title=V.FirePhase==0?TEXT("FIRE PLAN"):V.FirePhase<3?TEXT("OBSERVE"):V.FirePhase==3?TEXT("FIRE"):TEXT("FOLLOW THROUGH");
        UI.PageButtonV73(49,Y+8,298,38,Title,TEXT(""),FName(*(FString(TEXT("FRRUWorld:"))+Action)),true,false,84);
        UI.PageButtonV73(360,Y+8,343,38,TEXT("CANCEL DRAFT"),TEXT(""),TEXT("FRRUWorld:cancel"),V.FirePhase>0&&V.FirePhase<4,false,84);
    }
    else
    {UI.PageButtonV73(49,Y+8,298,38,TEXT("HOLD IN RESERVE"),TEXT(""),TEXT("FRRUWorld:reserve"),!S.FRRUSelectedSquadsV82.IsEmpty(),false,84);UI.PageButtonV73(360,Y+8,343,38,TEXT("RELEASE RESERVE"),TEXT(""),TEXT("FRRUWorld:release"),V.ReservePending,false,84);}
    }
    auto& Notice=FRRUOrderNoticeV82(PC,B);
    if(!V.Notice.IsEmpty()&&V.Notice!=Notice.LastReason)
    {Notice.LastReason=V.Notice;Notice.Text=V.Notice;Notice.Until=FPlatformTime::Seconds()+4.;}
    if(!Notice.Text.IsEmpty()&&FPlatformTime::Seconds()<Notice.Until)
    {
        const FLinearColor OrderWarningInk(.30f,.14f,.045f);
        UI.PaperSheet(1296,150,568,88);UI.Paragraph(Notice.Text,1318,167,524,54,20,OrderWarningInk,true);
    }
}
