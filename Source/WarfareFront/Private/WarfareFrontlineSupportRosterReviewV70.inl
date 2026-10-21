// Synchronous, volatile campaign fixture. No actors, world ticks, file writes or
// weapon effects are started; the normal binding/selection APIs are exercised.
void FFrontlineCampaignV32Access::CheckSupportsV70(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Report)
{
    using namespace FrontlineCampaignV32;
    FString UserDir;
    const bool Isolated=B&&B->bProgressionVolatile&&FParse::Param(FCommandLine::Get(),TEXT("WFV70Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    Report(TEXT("v70_support_checks_require_volatile_isolated_profile"),Isolated);if(!Isolated)return;
    const auto OldRuns=B->CampaignRuns;const auto OldActive=B->ActiveCampaignId;const auto OldFaction=B->PlayerFaction;
    const auto OldPhase=B->Phase;const auto OldEncounter=B->EncounterId;const auto OldBoundEncounter=B->BoundCampaignEncounterId;
    const auto OldBoundRun=B->BoundCampaignId;const int32 OldBoundMission=B->BoundCampaignMission,OldBoundStage=B->BoundAttachmentStageV53;
    const bool OldAdvanced=B->bCampaignMissionAdvanced,OldReadOnly=B->bProgressionReadOnly;
    const FString OldMessage=B->LastMessage,OldStatus=B->ProgressionSaveStatus;
    ON_SCOPE_EXIT{
        B->CampaignRuns=OldRuns;B->ActiveCampaignId=OldActive;B->PlayerFaction=OldFaction;B->Phase=OldPhase;
        B->EncounterId=OldEncounter;B->BoundCampaignEncounterId=OldBoundEncounter;B->BoundCampaignId=OldBoundRun;
        B->BoundCampaignMission=OldBoundMission;B->BoundAttachmentStageV53=OldBoundStage;
        B->bCampaignMissionAdvanced=OldAdvanced;B->bProgressionReadOnly=OldReadOnly;
        B->LastMessage=OldMessage;B->ProgressionSaveStatus=OldStatus;
    };
    auto Install=[&](EFrontlineFaction F,int32 Completed,int32 Selected,uint32 SavedMask=0u)
    {
        FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.CompletedMissions=Completed;R.SelectedMission=Selected;
        R.SupportSelectionMask=SavedMask;FrontlineCampaignV52::Initialize(R);
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->Phase=EFrontlinePhase::Briefing;
        B->EncounterId=FGuid::NewGuid();B->BoundCampaignEncounterId.Invalidate();B->BoundCampaignId.Invalidate();
        B->BoundCampaignMission=B->BoundAttachmentStageV53=INDEX_NONE;B->bProgressionReadOnly=false;
    };
    auto Mask=[](const TArray<EFrontlineSupport>& Supports)
    {uint32 Result=0;for(const auto Kind:Supports)Result|=1u<<uint8(Kind);return Result;};
    auto Coherent=[&]()
    {
        const auto L=GetLoadout(B);const auto Supports=GetEquippedSupports(B);const uint32 Visible=Mask(Supports);
        bool Valid=Supports.Num()<=L.SupportCapacity&&Supports.Num()==BitCount(Visible)
            &&Visible==(L.SelectedSupports&L.EligibleSupports)&&!(Visible&(1u<<uint8(EFrontlineSupport::Rally)));
        for(const auto Kind:Supports)Valid&=B->IsSupportUnlocked(Kind)&&SupportSelected(B,Kind)&&SupportBlockReason(B,Kind).IsEmpty();
        return Valid;
    };
    constexpr int32 Arrivals[]={0,1,MAX_int32,5,7,8,11,15,3,16};
    const uint32 Shrapnel=1u<<uint8(EFrontlineSupport::AirRecon),Smoke=1u<<uint8(EFrontlineSupport::Smoke);
    for(int32 Nation=0;Nation<2;++Nation)
    {
        const auto F=EFrontlineFaction(Nation);const FString Prefix=FString::Printf(TEXT("v70_support_%s_"),Nation?TEXT("german"):TEXT("british"));
        bool Operations=true,Bound=true,EarlyShrapnel=true;
        for(int32 Id=0;Id<20;++Id)
        {
            Install(F,Id,Id);uint32 Expected=0;const auto* Mission=B->GetSelectedCampaignMission();
            for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)
                if(Id>=Arrivals[I]&&!(Mission->DisabledSupportMask&(1u<<I)))Expected|=1u<<I;
            const auto Prepared=GetLoadout(B);const auto Equipped=GetEquippedSupports(B);
            Operations&=Prepared.EligibleSupports==Expected&&Coherent();
            if(Id>=3&&Id<=9)EarlyShrapnel&=!Prepared.bSelectSupports&&(Mask(Equipped)&Shrapnel)!=0;
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            Bound&=B->GetEncounterCampaignMission()&&B->GetEncounterCampaignMission()->Id==Id
                &&GetEquippedSupports(B)==Equipped&&Coherent();
            if(!Coherent())UE_LOG(LogTemp,Error,TEXT("WF_V70_SUPPORT_CONTEXT faction=%d operation=%d eligible=%u equipped=%u"),Nation,Id,Expected,Mask(GetEquippedSupports(B)));
        }
        Report(Prefix+TEXT("all_20_operations_use_selected_eligible_unique_six_slot_roster"),Operations);
        Report(Prefix+TEXT("preparation_and_bound_battle_use_identical_supports"),Bound);
        Report(Prefix+TEXT("shrapnel_visible_from_operation_3_before_roster_overflow"),EarlyShrapnel);

        bool Branches=true,NoPremature=true;
        for(int32 Fork:{2,6,12,17})for(int32 Pick=0;Pick<2;++Pick)
        {
            const int32 Id=Fork+Pick;Install(F,Fork,Id);B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;
            const auto Before=GetEquippedSupports(B);
            Branches&=FrontlineCampaignV52::RecordVictory(B->CampaignRuns[0],Id);
            B->Phase=EFrontlinePhase::Victory;
            NoPremature&=GetEquippedSupports(B)==Before&&Coherent();
            B->Phase=EFrontlinePhase::Briefing;
            Branches&=B->GetSelectedCampaignMission()->Id==Fork+2&&Coherent();
            if(Fork==2)Branches&=(Mask(GetEquippedSupports(B))&Shrapnel)!=0;
            B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Defeat;
            Branches&=Coherent()&&B->GetEncounterCampaignMission()->Id==Fork+2;
        }
        Report(Prefix+TEXT("both_paths_of_all_four_forks_reveal_bypassed_arrivals"),Branches);
        Report(Prefix+TEXT("victory_retains_completed_encounter_roster"),NoPremature);

        const uint32 Saved=Smoke|Shrapnel|(1u<<uint8(EFrontlineSupport::Artillery))|(1u<<uint8(EFrontlineSupport::Mortar))
            |(1u<<uint8(EFrontlineSupport::AntiTank))|(1u<<uint8(EFrontlineSupport::AirBombing));
        Install(F,20,16,Saved);const auto Normal=GetEquippedSupports(B);
        bool SavedChoices=Mask(Normal)==Saved&&Coherent();
        B->CampaignRuns[0].SelectedMission=10;
        SavedChoices&=Mask(GetEquippedSupports(B))==Smoke&&!B->IsSupportUnlocked(EFrontlineSupport::AirRecon)
            &&!SupportBlockReason(B,EFrontlineSupport::AirRecon).IsEmpty()&&B->CampaignRuns[0].SupportSelectionMask==Saved;
        B->CampaignRuns[0].SelectedMission=18;
        SavedChoices&=!(Mask(GetEquippedSupports(B))&((1u<<uint8(EFrontlineSupport::Gas))|(1u<<uint8(EFrontlineSupport::AirBombing))))
            &&B->CampaignRuns[0].SupportSelectionMask==Saved&&Coherent();
        B->CampaignRuns[0].SelectedMission=16;
        SavedChoices&=GetEquippedSupports(B)==Normal&&B->CampaignRuns[0].SupportSelectionMask==Saved;
        Report(Prefix+TEXT("archive_selection_survives_temporary_and_civilian_restrictions"),SavedChoices);
        const bool Refused=!ToggleSupport(B,EFrontlineSupport::Gas)&&B->CampaignRuns[0].SupportSelectionMask==Saved;
        const bool Exchanged=ToggleSupport(B,EFrontlineSupport::Artillery)&&ToggleSupport(B,EFrontlineSupport::Gas);
        const uint32 ExchangedMask=Saved^(1u<<uint8(EFrontlineSupport::Artillery))^(1u<<uint8(EFrontlineSupport::Gas));
        Report(Prefix+TEXT("full_roster_requires_explicit_exchange_and_preserves_shrapnel"),Refused&&Exchanged
            &&B->CampaignRuns[0].SupportSelectionMask==ExchangedMask&&Mask(GetEquippedSupports(B))==ExchangedMask&&Coherent());
        B->Phase=EFrontlinePhase::Fighting;
        Report(Prefix+TEXT("live_battle_cannot_change_saved_support_selection"),!ToggleSupport(B,EFrontlineSupport::Gas)
            &&B->CampaignRuns[0].SupportSelectionMask==ExchangedMask);

        Install(F,0,0);const auto Starting=GetEquippedSupports(B);
        auto& R=B->CampaignRuns[0];R.DispatchOrderMaskV52=R.DispatchResolvedMaskV52=1;R.DispatchChoiceMaskV52=0;
        Report(Prefix+TEXT("temporary_infantry_attachment_does_not_grant_fire_support"),FrontlineCampaignV52::IsLoanRole(B,EFrontlineRole::Lewis)
            &&GetEquippedSupports(B)==Starting&&!B->IsSupportUnlocked(EFrontlineSupport::AirRecon)&&R.SupportSelectionMask==0);
    }
    Report(TEXT("v70_support_null_context_has_no_invented_cards"),GetEquippedSupports(nullptr).IsEmpty());
}
