namespace UltimateArchiveUIV8215
{
struct FView{bool Open=false,InitializedV85=false;EFrontlineFaction Faction=EFrontlineFaction::British;EFRRUNationV81 NationV87=EFRRUNationV81::British;int32 Mission=0,Difficulty=1;TOptional<NavigationV61::FPage> EntryV851;};
TMap<TWeakObjectPtr<APlayerController>,FView> Views;
FView& View(APlayerController* P){return Views.FindOrAdd(P);}
bool Active(APlayerController* P){return View(P).Open;}
EFRRUNationV81 SelectedNationV87(const FView& S)
{return S.NationV87==EFRRUNationV81::French||S.NationV87==EFRRUNationV81::Russian?S.NationV87:S.Faction==EFrontlineFaction::German?EFRRUNationV81::German:EFRRUNationV81::British;}
void SetNationV87(FView& S,EFRRUNationV81 N)
{S.NationV87=N;if(N==EFRRUNationV81::British)S.Faction=EFrontlineFaction::British;else if(N==EFRRUNationV81::German)S.Faction=EFrontlineFaction::German;}
void Open(APlayerController* P,AWarfareFrontlineGameMode* B)
{
 auto& S=View(P);if(S.Open)return;
 if(auto* PC=Cast<AWarfareFrontlineController>(P);PC&&!FrontlineUltimateArchiveV8215::Active(B))S.EntryV851=NavigationV61::Capture(PC);
 if(!S.InitializedV85){SetNationV87(S,B?WarfareFRRUCampaignV82::Nation(B,0):EFRRUNationV81::British);S.Mission=0;S.Difficulty=1;S.InitializedV85=true;}S.Open=true;
}
void ReturnToSelectionV85(APlayerController* P,EFrontlineFaction Faction,int32 Mission,EFrontlineDifficulty Difficulty)
{auto& S=View(P);SetNationV87(S,Faction==EFrontlineFaction::German?EFRRUNationV81::German:EFRRUNationV81::British);S.Mission=Mission;S.Difficulty=int32(Difficulty);S.InitializedV85=true;S.Open=true;}
void ReturnToNationalSelectionV87(APlayerController* P,EFRRUNationV81 Nation,int32 Mission,EFrontlineDifficulty Difficulty)
{auto& S=View(P);SetNationV87(S,Nation);S.Mission=Mission;S.Difficulty=int32(Difficulty);S.InitializedV85=true;S.Open=true;}
void Close(APlayerController* P){View(P).Open=false;}
void BackV851(APlayerController* P)
{
 auto& S=View(P);S.Open=false;auto* PC=Cast<AWarfareFrontlineController>(P);if(!PC)return;
 // The replay restores its campaign snapshot for persistence only. Closing
 // its selector must restore the page that opened it, not that hidden campaign.
 const auto Entry=S.EntryV851;S.EntryV851.Reset();
 if(!Entry.IsSet()||!NavigationV61::Restore(PC,Entry.GetValue()))
 {auto& Screen=ScreenState(PC);Screen.Menu=Screen.Help=Screen.Settings=Screen.Technology=Screen.Deployment=Screen.AchievementsV76=false;Screen.Dossier=0;}
 auto& History=NavigationV61::History(PC);History.Back.Empty();History.Forward.Empty();
}
// 0 unhandled, 1 handled, 2 replay prepared, 3 close requested.
int32 Hit(APlayerController* P,AWarfareFrontlineGameMode* B,FName Name)
{
 const FString N=Name.ToString();auto& S=View(P);
 if(N==TEXT("V8215ArchiveClose")){BackV851(P);return 3;}
 if(N==TEXT("V8215ArchiveBegin")){if(FrontlineUltimateArchiveV8215::BeginNationV87(B,SelectedNationV87(S),S.Mission,EFrontlineDifficulty(S.Difficulty))){S.Open=false;return 2;}return 1;}
 if(N.StartsWith(TEXT("V8215ArchiveFaction"))){const FString D=N.RightChop(FString(TEXT("V8215ArchiveFaction")).Len());if(D.Len()==1&&D.IsNumeric()){const int32 I=FCString::Atoi(*D);if(I>=0&&I<4){SetNationV87(S,EFRRUNationV81(I));S.Mission=0;}}return 1;}
 if(N.StartsWith(TEXT("V8215ArchiveMission"))){const FString D=N.RightChop(FString(TEXT("V8215ArchiveMission")).Len());if(!D.IsEmpty()&&D.IsNumeric()){const int32 I=FCString::Atoi(*D);if(B&&FrontlineUltimateArchiveV8215::NationMissionsV87(B,SelectedNationV87(S)).IsValidIndex(I))S.Mission=I;}return 1;}
 if(N.StartsWith(TEXT("V8215ArchiveDifficulty"))){const FString D=N.RightChop(FString(TEXT("V8215ArchiveDifficulty")).Len());if(D.Len()==1&&D.IsNumeric()){const int32 I=FCString::Atoi(*D);if(I<3)S.Difficulty=I;}return 1;}
 return 0;
}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 auto& S=View(UI.HUD.GetOwningPlayerController());const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.PageSurfaceV73(0,0,1920,1080,false,false);UI.Hit(0,0,1920,1080,TEXT("Blocked"),70);
 UI.BilingualLabel(TEXT("ULTIMATE ARCHIVE"),TEXT("终极档案室"),92,51,1360,67,50,Paper,0,EFrontlineTextRoleV8210::Inscription);UI.ConsoleSeamV79(92,137,1828,137,Border,2);
 static const TCHAR* NationEN[]={TEXT("BRITAIN"),TEXT("GERMANY"),TEXT("FRANCE"),TEXT("RUSSIA")};
 static const TCHAR* NationZH[]={TEXT("英国"),TEXT("德国"),TEXT("法国"),TEXT("俄国")};
 const auto Nation=SelectedNationV87(S);
 for(int32 I=0;I<4;++I)UI.PageTabV73(92+I*436,157,428,WarRoom52::L(NationEN[I],NationZH[I]),FName(*FString::Printf(TEXT("V8215ArchiveFaction%d"),I)),int32(Nation)==I,true,80);
 const bool Available=FrontlineUltimateArchiveV8215::NationAvailableV87(B,Nation);const auto& Missions=FrontlineUltimateArchiveV8215::NationMissionsV87(B,Nation);S.Mission=FMath::Clamp(S.Mission,0,FMath::Max(0,Missions.Num()-1));
 for(int32 I=0;I<Missions.Num();++I)
 {
  const bool SingleColumn=Missions.Num()<=10;
  const float X=92+(SingleColumn?0:(I/10)*450),Y=244+(I%10)*64,W=SingleColumn?878:428;const auto& M=Missions[I];
  UI.PageButtonV73(X,Y,W,53,FrontlineLocalizationV35::Text(M.Title),TEXT(""),FName(*FString::Printf(TEXT("V8215ArchiveMission%d"),I)),Available,S.Mission==I,80);
 }
 const float PX=1048,PW=780;UI.PaperSheet(PX,244,PW,678);const FLinearColor Ink(.045f,.049f,.043f),Quiet(.13f,.14f,.12f);
 if(Missions.IsValidIndex(S.Mission))
 {
  const auto& M=Missions[S.Mission];UI.BilingualTitle(M.Title,PX+32,272,PW-64,64,ZH?32:42,Ink);
  UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(M.DateLabel),PX+32,348,PW-64,38,19,Quiet);
  auto* Map=Nation==EFRRUNationV81::French||Nation==EFRRUNationV81::Russian?FRRUWarRoomV82::OperationTexture(&M):WarRoom52::Map(WarRoom52::Stage(S.Mission));if(Map)UI.ImageContain(Map,PX+32,406,PW-64,262);
  UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,FrontlineLocalizationV35::Text(M.Briefing),PX+32,706,PW-64,118,21,Ink);
 }
 UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Available?WarRoom52::L(TEXT("Revisit any operation with the full company arsenal. Battle honours count towards this army's record."),TEXT("重返旧战场，重新编列部队。战功记入本阵营记录。")):WarRoom52::L(TEXT("Complete this army's campaign to open its archive."),TEXT("完成该阵营战役后开放。")),PX+32,845,PW-64,63,18,Available?Quiet:NoticeWarningV75);
 for(int32 I=0;I<3;++I)UI.PageButtonV73(92+I*303,914,282,52,FrontlineLocalizationV35::Text(FrontlineDifficultyV30::Label(EFrontlineDifficulty(I))),TEXT(""),FName(*FString::Printf(TEXT("V8215ArchiveDifficulty%d"),I)),true,S.Difficulty==I,80);
 UI.PageButtonV73(92,986,412,55,WarRoom52::L(TEXT("RETURN"),TEXT("返回")),TEXT("Esc"),TEXT("V8215ArchiveClose"),true,false,80);
 UI.PageButtonV73(1260,986,568,55,WarRoom52::L(TEXT("PREPARE THE COMPANY"),TEXT("整备部队")),TEXT("Enter"),TEXT("V8215ArchiveBegin"),Available,true,80);
}
}
namespace NewUnlockUIV8215
{
struct FNoticePage{FGuid Encounter,Campaign;int32 Mission=INDEX_NONE,Count=0,Index=0;uint64 Revision=0;};
TMap<TWeakObjectPtr<APlayerController>,FNoticePage> Pages;
uint64 NextNoticeRevisionV835=0;
void SyncNoticeV835(FNoticePage& S,const FGuid& Encounter,const FGuid& Campaign,int32 Mission,int32 Count)
{
 Count=FMath::Max(0,Count);
 if(S.Revision==0||S.Encounter!=Encounter||S.Campaign!=Campaign||S.Mission!=Mission||S.Count!=Count)
 {S.Encounter=Encounter;S.Campaign=Campaign;S.Mission=Mission;S.Count=Count;S.Index=0;S.Revision=++NextNoticeRevisionV835;}
 S.Index=Count>0?FMath::Clamp(S.Index,0,Count-1):0;
}
FNoticePage& PageFor(APlayerController* P,AWarfareFrontlineGameMode* B)
{
 auto& S=Pages.FindOrAdd(P);const auto* Run=B?B->GetCurrentCampaign():nullptr;
 SyncNoticeV835(S,B?B->GetEncounterId():FGuid(),Run?Run->Id:FGuid(),Run?Run->SelectedMission:INDEX_NONE,FrontlineNewUnlockV8215::Cards(B).Num());
 return S;
}
bool PreviousVisibleV835(const FNoticePage& S){return S.Count>1&&S.Index>0;}
FName ButtonNameV835(const FNoticePage& S,bool Previous)
{return FName(*FString::Printf(TEXT("V835Unlock_%llu_%d_%s"),static_cast<unsigned long long>(S.Revision),S.Index,Previous?TEXT("Previous"):TEXT("Continue")));}
FString PageCounterV835(const FNoticePage& S)
{return S.Count>0?FString::Printf(TEXT("%d / %d"),S.Index+1,S.Count):FString();}
// 0 ignored, 1 changed page, 2 dismissed. Each hit belongs to the notice and
// page that painted it, so a removed previous button cannot remain actionable.
int32 NavigateV835(FNoticePage& S,FName Name)
{
 if(S.Count<=0)return 0;
 S.Index=FMath::Clamp(S.Index,0,S.Count-1);
 if(Name==ButtonNameV835(S,true)){if(!PreviousVisibleV835(S))return 0;--S.Index;return 1;}
 if(Name==ButtonNameV835(S,false)){if(S.Index+1<S.Count){++S.Index;return 1;}return 2;}
 return 0;
}
bool Hit(APlayerController* P,AWarfareFrontlineGameMode* B,FName N)
{
 if(!FrontlineNewUnlockV8215::Pending(B))return false;auto& Page=PageFor(P,B);
 const int32 Result=NavigateV835(Page,N);if(Result==2){FrontlineNewUnlockV8215::Dismiss(B);Page.Index=0;}
 return Result!=0;
}
bool Confirm(APlayerController* P,AWarfareFrontlineGameMode* B)
{if(!FrontlineNewUnlockV8215::Pending(B))return false;return Hit(P,B,ButtonNameV835(PageFor(P,B),false));}
void RunNavigationChecksV835(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 const bool Safe=B&&B->GetWorld()->IsGameWorld()&&FParse::Param(FCommandLine::Get(),TEXT("WFV835Review"));
 Check(TEXT("v835_unlock_navigation_review_uses_isolated_page_state"),Safe);if(!Safe)return;
 const uint64 OldSequence=NextNoticeRevisionV835;ON_SCOPE_EXIT{NextNoticeRevisionV835=OldSequence;};
 const FGuid Encounter=FGuid::NewGuid(),Campaign=FGuid::NewGuid();FNoticePage S;
 SyncNoticeV835(S,Encounter,Campaign,1,1);
 Check(TEXT("v835_unlock_single_card_has_no_previous_button_and_exact_one_of_one"),!PreviousVisibleV835(S)&&PageCounterV835(S)==TEXT("1 / 1"));
 Check(TEXT("v835_unlock_single_card_rejects_previous_hit_without_state_change"),NavigateV835(S,ButtonNameV835(S,true))==0&&S.Index==0);
 Check(TEXT("v835_unlock_single_card_continue_returns_to_operations"),NavigateV835(S,ButtonNameV835(S,false))==2&&S.Index==0);
 SyncNoticeV835(S,Encounter,Campaign,2,3);const FName FirstContinue=ButtonNameV835(S,false);
 Check(TEXT("v835_unlock_multi_card_first_page_has_exact_total_and_no_previous"),PageCounterV835(S)==TEXT("1 / 3")&&!PreviousVisibleV835(S));
 Check(TEXT("v835_unlock_continue_moves_exactly_one_page"),NavigateV835(S,FirstContinue)==1&&S.Index==1&&PageCounterV835(S)==TEXT("2 / 3")&&PreviousVisibleV835(S));
 Check(TEXT("v835_unlock_rejects_previous_page_stale_continue_rect"),NavigateV835(S,FirstContinue)==0&&S.Index==1);
 const FName SecondPrevious=ButtonNameV835(S,true);
 Check(TEXT("v835_unlock_previous_returns_exactly_one_page"),NavigateV835(S,SecondPrevious)==1&&S.Index==0);
 Check(TEXT("v835_unlock_removed_previous_rect_is_not_actionable"),NavigateV835(S,SecondPrevious)==0&&S.Index==0);
 NavigateV835(S,ButtonNameV835(S,false));NavigateV835(S,ButtonNameV835(S,false));
 Check(TEXT("v835_unlock_last_page_keeps_true_total_and_dismisses_only_at_end"),S.Index==2&&PageCounterV835(S)==TEXT("3 / 3")&&NavigateV835(S,ButtonNameV835(S,false))==2);
 const FName OldNoticeContinue=ButtonNameV835(S,false);SyncNoticeV835(S,Encounter,FGuid::NewGuid(),2,1);
 Check(TEXT("v835_unlock_campaign_switch_resets_page_and_rejects_old_notice_rect"),S.Index==0&&S.Count==1&&!PreviousVisibleV835(S)&&NavigateV835(S,OldNoticeContinue)==0);
 const uint64 Revision=S.Revision;S.Index=8;SyncNoticeV835(S,S.Encounter,S.Campaign,S.Mission,1);
 Check(TEXT("v835_unlock_same_notice_clamps_invalid_page_without_restarting_identity"),S.Index==0&&S.Revision==Revision&&PageCounterV835(S)==TEXT("1 / 1"));
 Check(TEXT("v835_unlock_old_fixed_previous_and_continue_names_are_rejected"),NavigateV835(S,TEXT("V8215UnlockPrevious"))==0&&NavigateV835(S,TEXT("V8215UnlockContinue"))==0&&S.Index==0);
 SyncNoticeV835(S,S.Encounter,S.Campaign,S.Mission,0);
 Check(TEXT("v835_unlock_empty_notice_has_no_counter_and_no_action"),PageCounterV835(S).IsEmpty()&&!PreviousVisibleV835(S)&&NavigateV835(S,ButtonNameV835(S,false))==0);
}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
 const auto& Cards=FrontlineNewUnlockV8215::Cards(B);if(Cards.IsEmpty())return;auto& Page=PageFor(UI.HUD.GetOwningPlayerController(),B);const auto& C=Cards[Page.Index];
 UI.PageSurfaceV73();UI.Hit(0,0,1920,1080,TEXT("Blocked"),85);const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.BilingualLabel(TEXT("NEXT ASSIGNMENT"),TEXT("下一道配属命令"),144,100,1350,48,28,Muted);UI.ConsoleSeamV79(144,184,1776,184,Border,1);
 if(C.Kind==0)DrawRolePhotoV32(UI,B,EFrontlineRole(C.Index),144,264,440,496,false,EFrontlinePortraitContextV72::Narrative);
 else if(C.Kind==1)DrawSupportPhotoV32(UI,B,EFrontlineSupport(C.Index),144,286,440,440);
 else if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))UI.ImageContain(V->GetPortrait(EFrontlineVehicle(C.Index)),144,286,440,440);
 UI.BilingualTitle(C.Title,648,264,1128,72,ZH?35:48,Paper);UI.Description(C.Description,648,378,1128,328,22,Paper);
 UI.RoleLabel(EFrontlineTextRoleV8210::Numeral,PageCounterV835(Page),144,835,440,38,24,Muted,1);
 UI.RoleParagraph(EFrontlineTextRoleV8210::Text,WarRoom52::L(TEXT("Available for the next assignment. Review deployment before departure."),TEXT("下一次行动可安排此项配属。出发前，在出战序列中选择。")),648,765,1128,91,20,Muted);
 if(PreviousVisibleV835(Page))UI.PageButtonV73(144,936,380,64,WarRoom52::L(TEXT("PREVIOUS"),TEXT("上一页")),TEXT(""),ButtonNameV835(Page,true),true,false,90);
 UI.PageButtonV73(1236,936,540,64,Page.Index+1<Cards.Num()?WarRoom52::L(TEXT("CONTINUE"),TEXT("继续")):WarRoom52::L(TEXT("RETURN TO OPERATIONS"),TEXT("返回作战室")),TEXT("Enter"),ButtonNameV835(Page,false),true,true,90);
}
}
