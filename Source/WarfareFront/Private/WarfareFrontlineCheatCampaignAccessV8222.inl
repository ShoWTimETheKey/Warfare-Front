// This access is used only after the exact disposable-account review guard.
// Archive APIs retire fixture actors, so the visual harness re-seeds afterwards.
struct FFrontlineCheatCampaignReviewV8222
{
    AWarfareFrontlineGameMode* B;
    TArray<FFrontlineCampaignProgress> Runs;
    FGuid Active,Bound,BoundEncounter,Encounter;
    EFrontlineFaction Faction;
    EFrontlinePhase Phase;
    int32 Mission,Attachment;
    explicit FFrontlineCheatCampaignReviewV8222(AWarfareFrontlineGameMode* Battle)
        :B(Battle),Runs(B->CampaignRuns),Active(B->ActiveCampaignId),Bound(B->BoundCampaignId),
         BoundEncounter(B->BoundCampaignEncounterId),Encounter(B->EncounterId),Faction(B->PlayerFaction),
         Phase(B->Phase),Mission(B->BoundCampaignMission),Attachment(B->BoundAttachmentStageV53){}
    ~FFrontlineCheatCampaignReviewV8222()
    {
        B->Phase=EFrontlinePhase::Briefing;B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;
        B->ResetEncounter();B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;
        B->EncounterId=Encounter;B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->Phase=Phase;
    }
    void Empty()
    {B->Phase=EFrontlinePhase::Briefing;B->CampaignRuns.Empty();B->ActiveCampaignId.Invalidate();B->BoundCampaignId.Invalidate();}
    void ReadyForWin()
    {
        auto* Run=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& R){return R.Id==B->ActiveCampaignId;});
        if(!Run)return;Run->SelectedMission=19;Run->Difficulty=EFrontlineDifficulty::Veteran;
        B->BoundCampaignId=Run->Id;B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=19;
        B->Phase=EFrontlinePhase::Fighting;B->Elapsed=10;B->BattleOutroWinner=INDEX_NONE;
        B->EndReason=TEXT("我军已突破敌军端线。");B->CombatRecordV65[0]={};B->CombatRecordV65[1]={};
    }
};
