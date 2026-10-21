#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ScopeExit.h"
#include "UnrealClient.h"

namespace FrontlineDebugV37
{
namespace
{
struct FCommands
{
    bool NoCooldown=false,Research=false,HonoursGranted=false;
    FGuid Campaign;
    bool Bound=false;
};
struct FConsole
{
    bool Open=false;
    FString Line,Draft;
    int32 Cursor=0,HistoryIndex=0;
    TArray<FString> Output,History;
};
TMap<TWeakObjectPtr<const AWarfareFrontlineGameMode>,FCommands> Commands;
TMap<TWeakObjectPtr<const AWarfareFrontlineController>,FConsole> Consoles;
FGuid CampaignKey(const AWarfareFrontlineGameMode* B)
{
    if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::CampaignId(B);
    const auto* Run=B?B->GetCurrentCampaign():nullptr;return Run?Run->Id:FGuid();
}
FCommands& State(const AWarfareFrontlineGameMode* B)
{
    auto& S=Commands.FindOrAdd(B);const FGuid Current=CampaignKey(B);
    if(!S.Bound||S.Campaign!=Current)
    {
        const bool Changed=S.Bound;S=FCommands();S.Bound=true;S.Campaign=Current;
        if(Changed&&B)const_cast<AWarfareFrontlineGameMode*>(B)->SetBattleSpeed(1.f);
    }
    return S;
}
FConsole& Terminal(const AWarfareFrontlineController* C){return Consoles.FindOrAdd(C);}
AWarfareFrontlineGameMode* Battle(AWarfareFrontlineController* C)
{return C&&C->GetWorld()?Cast<AWarfareFrontlineGameMode>(C->GetWorld()->GetAuthGameMode()):nullptr;}
const TCHAR* BoolText(bool On){return On?TEXT("ON"):TEXT("OFF");}
void Print(FConsole& C,const FString& Message)
{
    TArray<FString> Lines;Message.ParseIntoArrayLines(Lines,false);
    C.Output.Append(Lines);while(C.Output.Num()>64)C.Output.RemoveAt(0);
}
void Toggle(AWarfareFrontlineController* PC,bool Open)
{
    if(!PC)return;
    auto& C=Terminal(PC);C.Open=Open;
    PC->FlushPressedKeys();PC->CancelTargeting();
    if(Open&&C.Output.IsEmpty())
        Print(C,TEXT("Type help for commands. Command switches last only for this running game.\nPurchased upgrades remain in the selected campaign."));
}
bool ParseSwitch(const FString& Word,bool& Value)
{
    if(Word==TEXT("on")){Value=true;return true;}
    if(Word==TEXT("off")){Value=false;return true;}
    return false;
}
float ParseSpeed(const FString& Word)
{
    if(Word==TEXT("0.5")||Word==TEXT("0.5x"))return .5f;
    if(Word==TEXT("1")||Word==TEXT("1x"))return 1.f;
    if(Word==TEXT("1.5")||Word==TEXT("1.5x")||Word==TEXT("2")||Word==TEXT("2x"))return 1.5f;
    if(Word==TEXT("4")||Word==TEXT("4x"))return 4.f;
    return 0;
}
void ClearReadiness(AWarfareFrontlineGameMode* B)
{
    for(int32 R=0;R<int32(EFrontlineRole::Count);++R)B->RecruitReadyAt[0][R]=B->Elapsed;
    B->ArtilleryReadyAt[0]=B->SmokeReadyAt[0]=B->MortarReadyAt[0]=B->RallyReadyAt[0]=B->Elapsed;
    for(float& Ready:B->ExtendedSupportReadyAt[0])Ready=B->Elapsed;
    FrontlineSupportV32::ResetReadinessForDebug(B,0);
    if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->ResetReadinessForDebug(0);
}
bool ProcessKey(AWarfareFrontlineController* PC,const FKeyEvent& E)
{
    if(!PC)return false;
    const FKey Key=E.GetKey();
    if(Key==EKeys::Tilde && !E.IsControlDown() && !E.IsAltDown())
    {if(!E.IsRepeat())Toggle(PC,!Terminal(PC).Open);return true;}
    auto& C=Terminal(PC);if(!C.Open)return false;
    // OS/window shortcuts remain available. Ordinary game input is consumed.
    if(E.IsAltDown()||E.IsCommandDown())return false;
    if(Key==EKeys::Escape){Toggle(PC,false);return true;}
    if(E.IsControlDown())
    {if(Key==EKeys::L)C.Output.Empty();return true;}
    if(Key==EKeys::Enter)
    {
        const FString Command=C.Line.TrimStartAndEnd();
        if(!Command.IsEmpty())
        {
            Print(C,TEXT("> ")+Command);
            if(C.History.IsEmpty()||C.History.Last()!=Command)C.History.Add(Command);
            while(C.History.Num()>32)C.History.RemoveAt(0);
            FString Response;
            if(Command.Equals(TEXT("clear"),ESearchCase::IgnoreCase))C.Output.Empty();
            else{Execute(Battle(PC),Command,Response);Print(C,Response);}
        }
        C.Line.Empty();C.Draft.Empty();C.Cursor=0;C.HistoryIndex=C.History.Num();return true;
    }
    if(Key==EKeys::Up||Key==EKeys::Down)
    {
        if(C.HistoryIndex==C.History.Num())C.Draft=C.Line;
        C.HistoryIndex=FMath::Clamp(C.HistoryIndex+(Key==EKeys::Up?-1:1),0,C.History.Num());
        C.Line=C.History.IsValidIndex(C.HistoryIndex)?C.History[C.HistoryIndex]:C.Draft;
        C.Cursor=C.Line.Len();return true;
    }
    if(Key==EKeys::Left){C.Cursor=FMath::Max(0,C.Cursor-1);return true;}
    if(Key==EKeys::Right){C.Cursor=FMath::Min(C.Line.Len(),C.Cursor+1);return true;}
    if(Key==EKeys::Home){C.Cursor=0;return true;}
    if(Key==EKeys::End){C.Cursor=C.Line.Len();return true;}
    if(Key==EKeys::BackSpace)
    {if(C.Cursor>0){C.Line.RemoveAt(--C.Cursor);};return true;}
    if(Key==EKeys::Delete)
    {if(C.Cursor<C.Line.Len())C.Line.RemoveAt(C.Cursor);return true;}
    if(Key==EKeys::Tab)
    {
        const TCHAR* Names[]={TEXT("help"),TEXT("status"),TEXT("nocd"),TEXT("research"),TEXT("speed"),TEXT("morale"),TEXT("honours"),TEXT("achievements"),TEXT("medals"),TEXT("reset"),TEXT("clear")};
        TArray<FString> Matches;for(auto Name:Names)if(FString(Name).StartsWith(C.Line.TrimStartAndEnd(),ESearchCase::IgnoreCase))Matches.Add(Name);
        if(Matches.Num()==1){C.Line=Matches[0]+TEXT(" ");C.Cursor=C.Line.Len();}
        else Print(C,FString::Join(Matches,TEXT("   ")));
        return true;
    }
    uint32 Character=E.GetCharacter();
    // Some game viewports deliver a zero character code for punctuation/numpad.
    if(Character<32||Character>126)
    {
        const FString Name=Key.ToString();
        if(Name.Len()==1)Character=Name[0];
        else if(Key==EKeys::SpaceBar)Character=' ';
        else if(Key==EKeys::Period||Key==EKeys::Decimal)Character='.';
        else if(Key==EKeys::Hyphen||Key==EKeys::Subtract)Character='-';
        else
        {
            const FKey Digits[]={EKeys::NumPadZero,EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,EKeys::NumPadEight,EKeys::NumPadNine};
            for(int32 I=0;I<10;++I)if(Key==Digits[I])Character='0'+I;
        }
    }
    if(Character>=32&&Character<=126&&Character!='`'&&Character!='~'&&C.Line.Len()<128)
    {C.Line.InsertAt(C.Cursor++,FChar::ToLower(TCHAR(Character)));}
    return true;
}
class FConsoleInput final:public IInputProcessor
{
public:
    TWeakObjectPtr<AWarfareFrontlineController> Controller;
    explicit FConsoleInput(AWarfareFrontlineController* C):Controller(C){}
    virtual void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override{}
    bool Focused() const
    {
        auto* C=Controller.Get();auto* G=C&&C->GetWorld()?C->GetWorld()->GetGameViewport():nullptr;
        return G&&G->Viewport&&G->Viewport->HasFocus()&&G->Viewport->IsForegroundWindow()&&FApp::HasFocus();
    }
    virtual bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& E) override
    {return Focused()&&ProcessKey(Controller.Get(),E);}
    virtual bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent& E) override
    {return Focused()&&IsOpen(Controller.Get())&&!E.IsAltDown()&&!E.IsCommandDown();}
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent&) override
    {return Focused()&&IsOpen(Controller.Get());}
    virtual bool HandleMouseButtonUpEvent(FSlateApplication&,const FPointerEvent&) override
    {return Focused()&&IsOpen(Controller.Get());}
    virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication&,const FPointerEvent&) override
    {return Focused()&&IsOpen(Controller.Get());}
    virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication&,const FPointerEvent&,const FPointerEvent*) override
    {return Focused()&&IsOpen(Controller.Get());}
    virtual const TCHAR* GetDebugName() const override{return TEXT("WarfareFrontFieldConsole");}
};
TSharedPtr<FConsoleInput> Input;
}

void Initialize(AWarfareFrontlineController* C)
{
    if(!C||!C->IsLocalController()||!FSlateApplication::IsInitialized())return;
    if(Input&&Input->Controller==C)return;
    if(Input)FSlateApplication::Get().UnregisterInputPreProcessor(Input);
    Input=MakeShared<FConsoleInput>(C);FSlateApplication::Get().RegisterInputPreProcessor(Input,0);
}
void Shutdown(AWarfareFrontlineController* C)
{
    if(Input&&Input->Controller==C)
    {if(FSlateApplication::IsInitialized())FSlateApplication::Get().UnregisterInputPreProcessor(Input);Input.Reset();}
    if(auto* B=Battle(C)){B->SetBattleSpeed(1.f);Commands.Remove(B);}
    Consoles.Remove(C);
}
bool IsOpen(const AWarfareFrontlineController* C)
{const auto* T=Consoles.Find(C);return T&&T->Open;}
void SetReviewOpen(AWarfareFrontlineController* C,bool Open){Toggle(C,Open);}
bool SubmitReviewCommand(AWarfareFrontlineController* C,const FString& Command)
{
    auto* B=Battle(C);if(!B||!B->IsProgressionVolatile()||Command.Len()>128)return false;
    Toggle(C,true);auto& T=Terminal(C);T.Line=Command;T.Cursor=Command.Len();
    return ProcessKey(C,FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));
}
bool HasCheatState(const AWarfareFrontlineGameMode* B)
{if(!B)return false;const auto& S=State(B);return S.NoCooldown||S.Research||S.HonoursGranted||B->BattleSpeed>1.5f;}
bool UnlimitedResearch(const AWarfareFrontlineGameMode* B)
{return B&&State(B).Research;}
void Tick(AWarfareFrontlineGameMode* B)
{
    if(!B)return;
    for(auto It=Commands.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    const auto& S=State(B);
    if(S.NoCooldown)ClearReadiness(B);
}
bool Execute(AWarfareFrontlineGameMode* B,const FString& Command,FString& Response)
{
    if(!B){Response=TEXT("No active game world.");return false;}
    if(Command.Len()>128){Response=TEXT("Command too long.");return false;}
    TArray<FString> Words;Command.ToLower().ParseIntoArrayWS(Words);
    if(Words.IsEmpty()){Response=TEXT("Type help for available commands.");return false;}
    auto& S=State(B);const FString& Name=Words[0];
    if(Name==TEXT("help")&&Words.Num()==1)
    {
        Response=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese
        ?TEXT("nocd on|off       征募与支援无冷却。\nresearch on|off   研发不消耗研究点。\nspeed 0.5|1|1.5|4  调整战斗速度。\nmorale 1..100     设置己方士气。\nhonours all      作弊授予全部成就与真实勋章。\nachievements all / medals all   仅授予成就 / 勋章。\nhonours reset    清空全部成就、勋章与累计战功进度。\nstatus / reset / clear   状态 / 恢复开关 / 清屏。\n使用作弊会取消本战役的正常战功资格。删除战役档案会撤销所有作弊授予。↑↓ 历史；Tab 补全。")
        :TEXT("nocd on|off       Recruitment and support without cooldowns.\nresearch on|off   Research without spending points.\nspeed 0.5|1|1.5|4  Set battle speed.\nmorale 1..100     Set player morale.\nhonours all      Cheat-grant all achievements and real decorations.\nachievements all / medals all   Grant either group.\nhonours reset    Clear all achievements, decorations and honour progress.\nstatus / reset / clear   Status / restore switches / clear output.\nCheats disable natural honours for this campaign. Deleting an archive revokes all cheat grants. Up/Down: history; Tab: complete.");return true;
    }
    if(Name==TEXT("status")&&Words.Num()==1)
    {Response=FString::Printf(TEXT("No cooldown %s | Unlimited research %s | Battle speed %.1fx"),BoolText(S.NoCooldown),BoolText(S.Research),B->BattleSpeed);return true;}
    if(Name==TEXT("reset")&&Words.Num()==1)
    {S=FCommands();S.Bound=true;S.Campaign=CampaignKey(B);B->SetBattleSpeed(1.f);Response=TEXT("Normal command switches and speed restored. Purchased upgrades remain.");return true;}
    if(Name==TEXT("honours")&&Words.Num()==2&&Words[1]==TEXT("reset"))
    {const bool Reset=FrontlineAchievementsV76::ResetAllHonoursV83(B,Response);if(Reset)S.HonoursGranted=false;return Reset;}
    if((Name==TEXT("honours")||Name==TEXT("achievements")||Name==TEXT("medals"))&&Words.Num()==2&&Words[1]==TEXT("all"))
    {S.HonoursGranted=true;return FrontlineAchievementsV76::GrantAllForCheatV8218(B,Name!=TEXT("medals"),Name!=TEXT("achievements"),Response);}
    if((Name==TEXT("nocd")||Name==TEXT("research"))&&Words.Num()==2)
    {
        bool Value=false;if(!ParseSwitch(Words[1],Value)){Response=TEXT("Expected on or off.");return false;}
        if(Value)FrontlineAchievementsV76::MarkCheat(B);
        if(Name==TEXT("nocd")){S.NoCooldown=Value;if(Value)ClearReadiness(B);}
        else S.Research=Value;
        Response=Name+TEXT(" ")+BoolText(Value);return true;
    }
    if(Name==TEXT("speed")&&Words.Num()==2)
    {
        const float Speed=ParseSpeed(Words[1]);
        if(Speed<=0){Response=TEXT("Choose speed 0.5, 1, 1.5 or 4.");return false;}
        if(B->Phase!=EFrontlinePhase::Fighting){Response=TEXT("Change speed during an active battle.");return false;}
        if(Speed>1.5f)FrontlineAchievementsV76::MarkCheat(B);
        B->SetBattleSpeed(Speed);Response=FString::Printf(TEXT("Battle speed %.1fx. Pause and menu controls remain available."),Speed);return true;
    }
    if(Name==TEXT("morale")&&Words.Num()==2)
    {
        bool Digits=!Words[1].IsEmpty();for(TCHAR C:Words[1])Digits&=C>='0'&&C<='9';
        const int32 Value=Digits&&Words[1].Len()<=3?FCString::Atoi(*Words[1]):-1;
        if(Value<1||Value>100||B->Phase!=EFrontlinePhase::Fighting)
        {Response=TEXT("Use morale 1..100 during an active battle.");return false;}
        FrontlineAchievementsV76::MarkCheat(B);B->Morale[0]=Value;Response=FString::Printf(TEXT("Player morale %d."),Value);return true;
    }
    Response=TEXT("Unknown command or arguments. Type help.");return false;
}
void Draw(AHUD* HUD,UCanvas* Canvas)
{
    auto* PC=HUD?Cast<AWarfareFrontlineController>(HUD->GetOwningPlayerController()):nullptr;
    if(!Canvas||Canvas->SizeX<1||Canvas->SizeY<1||!PC||!IsOpen(PC))return;
    auto& C=Terminal(PC);
    static TWeakObjectPtr<UFont> Font;
    if(!Font.IsValid())Font=LoadObject<UFont>(nullptr,TEXT("/Game/Warfare/UI/V09/F_UI.F_UI"));
    UFont* Face=Font.IsValid()?Font.Get():GEngine->GetMediumFont();
    const float S=FMath::Min(Canvas->SizeX/1920.f,Canvas->SizeY/1080.f);
    const float W=FMath::Min(Canvas->SizeX/S-64.f,1520.f),X=32.f,Y=86.f,H=428.f;
    HUD->DrawRect(FLinearColor(.012f,.016f,.018f,.975f),X*S,Y*S,W*S,H*S);
    HUD->DrawRect(FLinearColor(.34f,.30f,.23f,.95f),X*S,Y*S,W*S,2*S);
    HUD->DrawRect(FLinearColor(.34f,.30f,.23f,.95f),X*S,(Y+H)*S,W*S,2*S);
    float FW=0,FH=1;HUD->GetTextSize(TEXT("Hg"),FW,FH,Face,1.f);
    auto Width=[&](const FString& Text,float Size){float TW=0,TH=0;HUD->GetTextSize(Text,TW,TH,Face,Size/FMath::Max(FH,1.f));return TW;};
    auto Text=[&](const FString& Value,float PX,float PY,float Size,FLinearColor Colour)
    {HUD->DrawText(Value,Colour,PX*S,PY*S,Face,Size/FMath::Max(FH,1.f)*S,false);};
    Text(TEXT("WARFARE FRONT  /  FIELD CONSOLE"),X+20,Y+16,25,FLinearColor(.78f,.75f,.66f));
    FString Status;Execute(Battle(PC),TEXT("status"),Status);
    Text(Status,X+20,Y+52,19,FLinearColor(.55f,.62f,.59f));
    TArray<FString> Wrapped;
    for(const auto& Line:C.Output)
    {
        TArray<FString> Words;Line.ParseIntoArray(Words,TEXT(" "),true);FString Row;
        for(const auto& Word:Words)
        {
            const FString Next=Row.IsEmpty()?Word:Row+TEXT(" ")+Word;
            if(Width(Next,21)>W-42&&!Row.IsEmpty()){Wrapped.Add(Row);Row.Empty();}
            if(!Row.IsEmpty())Row+=TEXT(" ");
            for(const TCHAR Character:Word)
            {
                if(!Row.IsEmpty()&&Width(Row+FString::Chr(Character),21)>W-42)
                {Wrapped.Add(Row);Row.Empty();}
                Row.AppendChar(Character);
            }
        }
        Wrapped.Add(Row);
    }
    const int32 First=FMath::Max(0,Wrapped.Num()-9);
    for(int32 I=First;I<Wrapped.Num();++I)Text(Wrapped[I],X+20,Y+91+(I-First)*28,21,FLinearColor(.72f,.73f,.70f));
    HUD->DrawRect(FLinearColor(.06f,.075f,.075f,1), (X+15)*S,(Y+366)*S,(W-30)*S,43*S);
    FString Entry=C.Line;Entry.InsertAt(FMath::Clamp(C.Cursor,0,Entry.Len()),TEXT('|'));
    while(Width(Entry,23)>W-70&&Entry.Len()>1)
    {if(Entry.Find(TEXT("|"))>Entry.Len()/2)Entry.RightChopInline(1);else Entry.LeftChopInline(1);}
    Text(TEXT("> ")+Entry,X+24,Y+372,23,FLinearColor(.85f,.83f,.70f));
}

void RunSpeedChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"));
    Check(TEXT("v836_speed_review_requires_exact_disposable_profile"),Safe);if(!Safe)return;
    const float Before=B->BattleSpeed;ON_SCOPE_EXIT{B->SetBattleSpeed(Before);};
    bool Rates=true;for(float Rate:{.5f,1.f,1.5f,4.f})
    {B->SetBattleSpeed(Rate);Rates&=B->BattleSpeed==Rate&&UGameplayStatics::GetGlobalTimeDilation(B)==Rate;}
    Check(TEXT("v836_half_normal_one_and_half_existing_debug_four_use_actual_world_clock"),Rates);
    B->SetBattleSpeed(2.f);
    Check(TEXT("v836_old_two_speed_api_migrates_to_one_and_half_world_clock"),B->BattleSpeed==1.5f&&UGameplayStatics::GetGlobalTimeDilation(B)==1.5f);
    Check(TEXT("v836_old_console_speed_two_alias_never_restores_double_speed"),ParseSpeed(TEXT("2"))==1.5f&&ParseSpeed(TEXT("2x"))==1.5f
        &&ParseSpeed(TEXT("1.5"))==1.5f&&ParseSpeed(TEXT("1.5x"))==1.5f);
    B->SetBattleSpeed(3.f);
    Check(TEXT("v836_unsupported_speed_restores_normal_clock"),B->BattleSpeed==1.f&&UGameplayStatics::GetGlobalTimeDilation(B)==1.f);
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B||!B->IsProgressionVolatile()){Check(TEXT("console_review_requires_volatile_profile"),false);return;}
    const FCommands Before=State(B);const float PriorSpeed=B->BattleSpeed;const auto PriorPhase=B->Phase;
    const float PriorMorale=B->Morale[0],PriorEnemyReady=B->RecruitReadyAt[1][0];float BeforeReady[int32(EFrontlineRole::Count)];
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)BeforeReady[I]=B->RecruitReadyAt[0][I];
    ON_SCOPE_EXIT{State(B)=Before;B->Phase=PriorPhase;B->Morale[0]=PriorMorale;B->SetBattleSpeed(PriorSpeed);B->RecruitReadyAt[1][0]=PriorEnemyReady;for(int32 I=0;I<int32(EFrontlineRole::Count);++I)B->RecruitReadyAt[0][I]=BeforeReady[I];};
    FString R;B->Phase=EFrontlinePhase::Fighting;Execute(B,TEXT("reset"),R);
    bool Rates=true;for(auto Word:{TEXT("0.5"),TEXT("1"),TEXT("1.5"),TEXT("4")})
    {Rates&=Execute(B,FString(TEXT("speed "))+Word,R)&&B->BattleSpeed==ParseSpeed(Word);}
    Check(TEXT("console_all_requested_simulation_rates_use_world_dilation"),Rates);
    Check(TEXT("console_rejects_bad_or_unbounded_rates"),!Execute(B,TEXT("speed nan"),R)&&!Execute(B,TEXT("speed 999"),R)&&!Execute(B,TEXT("speed 2 unexpected"),R)&&B->BattleSpeed==4.f);
    const int32 Stored=B->GetActiveResearch().ResearchPoints;
    Execute(B,TEXT("research on"),R);
    Check(TEXT("console_unlimited_research_does_not_inflate_saved_balance"),B->GetResearchPoints()==99999&&B->GetActiveResearch().ResearchPoints==Stored);
    Execute(B,TEXT("research off"),R);
    Check(TEXT("console_research_off_restores_real_balance"),B->GetResearchPoints()==Stored);
    B->RecruitReadyAt[0][0]=B->Elapsed+90;B->RecruitReadyAt[1][0]=B->Elapsed+120;
    const float EnemyReady=B->RecruitReadyAt[1][0];Execute(B,TEXT("nocd on"),R);Tick(B);
    Check(TEXT("console_no_cooldown_affects_player_only"),B->RecruitCooldown(EFrontlineRole::Rifle,0)==0&&B->RecruitReadyAt[1][0]==EnemyReady);
    Execute(B,TEXT("reset"),R);
    Check(TEXT("console_reset_restores_normal_flags_and_rate"),!State(B).NoCooldown&&!State(B).Research&&B->BattleSpeed==1.f);
    Check(TEXT("console_rejects_unknown_commands_without_exec_fallback"),!Execute(B,TEXT("open https://example.org"),R)&&!Execute(B,FString::ChrN(129,'x'),R));
    if(auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController()))
    {
        auto Saved=Terminal(PC);ON_SCOPE_EXIT{Terminal(PC)=Saved;};
        Terminal(PC)=FConsole();
        const FModifierKeysState Modifiers;
        ProcessKey(PC,FKeyEvent(EKeys::Tilde,Modifiers,0,false,0,0));
        const bool Open=IsOpen(PC);
        ProcessKey(PC,FKeyEvent(EKeys::N,Modifiers,0,false,'n',0));
        ProcessKey(PC,FKeyEvent(EKeys::Tab,Modifiers,0,false,0,0));
        const bool Complete=Terminal(PC).Line==TEXT("nocd ");
        ProcessKey(PC,FKeyEvent(EKeys::Escape,Modifiers,0,false,0,0));
        Check(TEXT("console_toggle_text_completion_close_use_actual_input_handler"),Open&&Complete&&!IsOpen(PC));
    }
}
}

void AWarfareFrontlineController::EndPlay(const EEndPlayReason::Type Reason)
{
    FrontlineDebugV37::Shutdown(this);Super::EndPlay(Reason);
}
