// The next order's title may appear on the filed report, but its attachments
// belong to the next operation's preparation and introduction, not this victory.
enum class EAttachmentKindV38:uint8 { Infantry,Support,Armour };
bool V38LockCoverageReview=false;
void DrawLockCoverageReviewV38(const FFrontlineCanvas& UI)
{
    UI.Rect(0,0,1920,1080,FLinearColor(.018f,.023f,.027f,1));
    const FLinearColor Backgrounds[]={FLinearColor(.012f,.017f,.022f,1),FLinearColor(.80f,.78f,.69f,1),FLinearColor(.17f,.26f,.30f,1)};
    for(int32 I=0;I<3;++I)
    {
        const float X=90+I*610;UI.Rect(X,170,540,620,Backgrounds[I]);
        for(int32 K=0;K<15;++K)UI.Line(X+K*36,170,X+K*36,790,FLinearColor(.36f,.24f,.14f,.55f),2);
        DrawEmblemV30(UI,4,X+108,225,324,324);
        UI.Skin(2,X+150,619,136,144,6);DrawEmblemV30(UI,4,X+189,658,58,58);
        UI.Skin(2,X+306,619,116,86,5);DrawEmblemV30(UI,4,X+348,646,32,32);
    }
}
struct FAttachmentV38
{
    EAttachmentKindV38 Kind;int32 Id;FString Name,Caption,Description;
};
const FFrontlineCampaignMission* NextAwardedOperationV38(const AWarfareFrontlineGameMode* B)
{
    if(!B||B->Phase!=EFrontlinePhase::Victory||!B->WasCampaignMissionAdvanced())return nullptr;
    const auto* Previous=B->GetEncounterCampaignMission();const auto* Next=B->GetSelectedCampaignMission();const auto* Run=B->GetCurrentCampaign();
    return Previous&&Next&&Run&&Next->Id>Previous->Id&&Run->CompletedMissions==Next->Id?Next:nullptr;
}
TArray<FAttachmentV38> AwardedAttachmentsV38(AWarfareFrontlineGameMode* B)
{
    // Retained for archived V38 review callers. No production report draws an
    // attachment award; the normal entry sheets introduce the actual mission.
    return {};
}
void DrawAwardedAttachmentsV38(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const TArray<FAttachmentV38>& Entries)
{
    const FLinearColor Ink(.028f,.032f,.029f),Faded(.07f,.075f,.064f);
    UI.Label(TEXT("新的配属"),1034,469,470,31,20,Ink);
    const float Cell=FMath::Min(94.f,470.f/FMath::Max(1,Entries.Num()));
    const float Start=1034+(470-Cell*Entries.Num())*.5f;
    int32 Hover=INDEX_NONE;
    for(int32 I=0;I<Entries.Num();++I)
    {
        const auto& Entry=Entries[I];const float X=Start+I*Cell,Photo=FMath::Min(78.f,Cell-8);
        UI.Skin(2,X+(Cell-Photo)*.5f-3,509,Photo+6,Photo+6,4);
        if(Entry.Kind==EAttachmentKindV38::Infantry)DrawRolePhotoV32(UI,B,EFrontlineRole(Entry.Id),X+(Cell-Photo)*.5f,512,Photo,Photo);
        else if(Entry.Kind==EAttachmentKindV38::Support)DrawSupportPhotoV32(UI,B,EFrontlineSupport(Entry.Id),X+(Cell-Photo)*.5f,512,Photo,Photo);
        else if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))UI.Image(V->GetPortrait(EFrontlineVehicle(Entry.Id)),X+(Cell-Photo)*.5f,512,Photo,Photo,FLinearColor::White,0,0,1,1,false,true);
        UI.Label(Entry.Caption,X+2,603,Cell-4,34,12,Ink,1);
        if(UI.Hovered(X,509,Cell,136))Hover=I;
    }
    if(Hover!=INDEX_NONE)
    {
        const auto& Entry=Entries[Hover];constexpr float X=978,Y=178,W=556,H=286;
        UI.Metal(X,Y,W,H,true);UI.Label(Entry.Name,X+20,Y+15,W-40,35,21,Paper);
        UI.Paragraph(Entry.Description,X+20,Y+67,W-40,166,16,Paper,true);
        UI.Label(TEXT("已配属下一行动，可在作战室查看。"),X+20,Y+241,W-40,27,14,Muted);
    }
}
