// V20: three resident portraits are the order buttons. There is no expand/select
// page; the adjacent entry lever makes reinforcements pass, not leave the garrison.
UTexture2D* SectionPortrait(EFrontlineRole Role,bool German,AWarfareFrontlineGameMode* Battle=nullptr)
{
    if(Battle && Role==EFrontlineRole::Marksman && Battle->HasScopedMarksmen(0))return Artwork().UpgradedPortraits[0];
    if(Battle && Role==EFrontlineRole::Mills && Battle->HasStormtroopers(0))return Artwork().UpgradedPortraits[1];
    if(Battle && Role==EFrontlineRole::Officer && Battle->HasCommissionedOfficers(0))return Artwork().OfficerPortraits[German?1:0];
    const int32 R=int32(Role);
    return R<3?Artwork().BasicPortraits[(German?3:0)+R].Get():
        R<5?Artwork().SpecialistPortraits[(German?2:0)+R-3].Get():nullptr;
}

struct FTrenchControlBounds {float X,Y,W=269.f,H=91.f;};
FTrenchControlBounds TrenchControlBounds(const FVector2D& Anchor)
{return {FMath::Clamp(float(Anchor.X)-134.5f,16.f,1635.f),FMath::Max(91.f,float(Anchor.Y)+16.f-91.f)};}
bool TrenchPointerNear(const FVector2D& Mouse,const FVector2D& Trench)
{return FMath::Abs(Mouse.X-Trench.X)<=115.f&&FMath::Abs(Mouse.Y-Trench.Y)<=100.f;}
bool TrenchPointerStays(const FVector2D& Mouse,const FVector2D& Trench,const FVector2D& Anchor)
{
    const auto P=TrenchControlBounds(Anchor);
    const bool OverPanel=Mouse.X>=P.X-14&&Mouse.X<=P.X+P.W+14&&Mouse.Y>=P.Y-14&&Mouse.Y<=P.Y+P.H+14;
    const bool Bridge=FMath::Abs(Mouse.X-Trench.X)<=P.W*.5f+18&&Mouse.Y>=FMath::Min(P.Y, float(Trench.Y)-100)-14&&Mouse.Y<=FMath::Max(P.Y+P.H,float(Trench.Y)+100)+14;
    return TrenchPointerNear(Mouse,Trench)||OverPanel||Bridge;
}

void DrawTrenchCommand(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,
    AWarfareFrontlineController* PC,int32 Index,const FVector2D& Anchor)
{
    const auto& T=B->Trenches[Index];
    if (T.Owner==1) return;
    const bool Owned=T.Owner==0;
    const bool Selected=PC->SelectedTrench==Index,Paused=UGameplayStatics::IsGamePaused(B);
    const TArray<int32> Slots=B->GetTrenchSquadSlots(Index);
    const bool German=B->GetPlayerFaction()==EFrontlineFaction::German;
    const auto Bounds=TrenchControlBounds(Anchor);const float X=Bounds.X,Y=Bounds.Y,W=Bounds.W,H=Bounds.H;
    UI.Metal(X,Y,W,H,true);
    UI.PrimaryText(FString::Printf(TEXT("%02d"),Index+1),X+9,Y+5,14,Selected?Paper:Muted);
    UI.PrimaryRight(!Owned?TEXT("尚未占领"):Selected?TEXT("出击"):TEXT(""),X+W-9,Y+5,12,Muted);
    if(Selected)UI.Shortcut(X,Y,W,23,TEXT("Q"));
    UI.Hit(X,Y,W,23,FName(*FString::Printf(TEXT("TrenchFocus%d"),Index)),5);
    if (Selected) UI.Line(X+7,Y+H-2,X+W-7,Y+H-2,Allied,1);
    FString TooltipTitle,TooltipDetail;
    for (int32 Slot=0;Slot<FrontlineBattlefield::TrenchCapacity;++Slot)
    {
        const auto* Squad=Slots.IsValidIndex(Slot)?B->FindSquad(Slots[Slot]):nullptr;
        const float SX=X+7+Slot*63,SY=Y+24;const bool Hover=UI.Hovered(SX,SY,57,59);
        UI.Skin(2,SX,SY,57,59,4);
        if (!Squad)
        {
            UI.Line(SX+21,SY+28,SX+36,SY+28,Dim,1);
            UI.Hit(SX,SY,57,59,TEXT("Blocked"),12);
            if (Hover) {TooltipTitle=TEXT("空余席位");TooltipDetail=T.bBypass?TEXT("入壕关闭 · 经过的小队继续向前"):TEXT("后续部队可在这里驻防");}
            continue;
        }
        const FString Reason=B->SquadAdvanceBlockReason(Index,Squad->Id);
        DrawRolePhotoV32(UI,B,Squad->Role,SX+2,SY+2,53,53,!Reason.IsEmpty());
        UI.Gradient(SX+2,SY+34,53,21,FLinearColor(0,0,0,0),FLinearColor(.001f,.003f,.002f,.91f),true);
        UI.PrimaryRight(FString::FromInt(Squad->LivingCount()),SX+49,SY+36,16,Paper);
        const float Strength=SquadStrengthV39(*Squad).Fraction;
        UI.Rect(SX+3,SY+56,51*Strength,2,Allied);
        if (Hover)
        {
            const float Tail=German?40.f:10.f,Tip=German?10.f:40.f,Shoulder=German?19.f:31.f;
            UI.Line(SX+Tail,SY+26,SX+Tip,SY+26,Reason.IsEmpty()?Amber:Muted,2);
            UI.Line(SX+Shoulder,SY+17,SX+Tip,SY+26,Reason.IsEmpty()?Amber:Muted,2);
            UI.Line(SX+Shoulder,SY+35,SX+Tip,SY+26,Reason.IsEmpty()?Amber:Muted,2);
            TooltipTitle=B->GetRecruitName(Squad->Role);
            TooltipDetail=Reason.IsEmpty()?TEXT("点击出壕 · 满员友壕自动通过"):ChineseStatus(Reason);
        }
        UI.Hit(SX,SY,57,59,Reason.IsEmpty()?FName(*FString::Printf(TEXT("SectionAdvance%d_%d"),Index,Squad->Id)):FName(TEXT("Blocked")),12);
    }
    // This control changes admission only. It never orders current residents out.
    const float LX=X+200,LY=Y+24;
    UI.Line(X+193,Y+26,X+193,Y+81,Border);
    UI.Skin(1,LX,LY,61,59,5);
    UI.PrimaryLabel(TEXT("ENTRY"),LX+4,LY+5,53,19,13,Muted,1);
    UI.PrimaryLabel(T.bBypass?TEXT("OFF"):TEXT("ON"),LX+4,LY+27,53,23,19,T.bBypass?Amber:Paper,1);
    UI.Line(LX+13,LY+53,LX+48,LY+53,T.bBypass?Amber:Allied,2);
    UI.Hit(LX,LY,61,59,(!Owned||Paused)?FName(TEXT("Blocked")):FName(*FString::Printf(TEXT("TrenchEntry%d"),Index)),12);
    if (UI.Hovered(LX,LY,61,59))
    {
        TooltipTitle=T.bBypass?TEXT("开启入壕"):TEXT("关闭入壕");
        TooltipDetail=!Owned?TEXT("占领后可管理驻军。"):Paused?TEXT("暂停中，请先继续。"):(T.bBypass?TEXT("允许后续小队进入空席"):TEXT("后续部队通过 · 驻军不动"));
    }
    if (!TooltipTitle.IsEmpty())
    {
        UI.Metal(X,Y-60,W,55,true);UI.FitText(TooltipTitle,X+10,Y-53,W-20,16,Paper);
        UI.FitText(TooltipDetail,X+10,Y-29,W-20,14,Muted);
    }
}
