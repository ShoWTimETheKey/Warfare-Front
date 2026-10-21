// National replays borrow a separate volatile runtime. The original repository
// object, its path and selected campaign remain intact until the room is left.
namespace
{
struct FArchiveReplayV87
{
    TSharedPtr<FRuntime> Restore;
    FGuid Replay;
    EFRRUNationV81 Nation=EFRRUNationV81::Invalid;
    EFrontlineDifficulty Level=EFrontlineDifficulty::Rookie;
    int32 MissionOrder=INDEX_NONE;
    bool HostVolatile=false,HostReadOnly=false,Qualified=false,Assisted=false;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FArchiveReplayV87> ArchiveReplaysV87;
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TSharedPtr<FRuntime>> LegacyArchiveRestoresV87;
TMap<uint8,TSharedPtr<FRuntime>> ArchiveMissionPreviewsV87;
TWeakObjectPtr<AWarfareFrontlineGameMode> ArchiveKeyV87(const AWarfareFrontlineGameMode* B)
{return TWeakObjectPtr<AWarfareFrontlineGameMode>(const_cast<AWarfareFrontlineGameMode*>(B));}
void PruneArchiveOwnersV87()
{
    for(auto It=ArchiveReplaysV87.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    for(auto It=LegacyArchiveRestoresV87.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
}

bool AvailableArchiveReplayV87(const AWarfareFrontlineGameMode* B,EFRRUNationV81 N)
{
    if(!B||!CampaignsAvailable(N))return false;
    // Use the same account's earned route flag as the British/German room.
    // Temporary replay progress cannot unlock a second campaign's room.
    const uint8 Bit=N==EFRRUNationV81::French?4u:8u;
    return (FrontlineAchievementsV76::CompletedCampaignRouteMaskV82()&Bit)!=0;
}
const TArray<FFrontlineCampaignMission>& ArchiveMissionsV87(const AWarfareFrontlineGameMode* B,EFRRUNationV81 N)
{
    static const TArray<FFrontlineCampaignMission> Empty;
    if(!B||(N!=EFRRUNationV81::French&&N!=EFRRUNationV81::Russian))return Empty;
    auto& Cache=ArchiveMissionPreviewsV87.FindOrAdd(uint8(N));if(!Cache.IsValid())Cache=MakeShared<FRuntime>();
    // Dedicated owners keep every preview's localized TCHAR pointer valid;
    // switching tabs never changes the active campaign or its mission cache.
    CacheMissions(*Cache,NativeNation(N));return Cache->MissionRows;
}
bool ArchiveReplayActiveV87(const AWarfareFrontlineGameMode* B)
{
    if(!B)return false;const auto Key=ArchiveKeyV87(B);const auto* Session=ArchiveReplaysV87.Find(Key);
    const auto* Runtime=Sessions.Find(Key);
    return Session&&Runtime&&Runtime->IsValid()&&(*Runtime)->Loaded&&(*Runtime)->Selected&&(*Runtime)->Volatile
        &&(*Runtime)->Path.IsEmpty()&&(*Runtime)->Current==Session->Replay
        &&(*Runtime)->Repository.Entries.Num()==1&&(*Runtime)->Repository.Entries[0].Id==Session->Replay;
}
bool ArchiveReplayEligibleV87(const AWarfareFrontlineGameMode* B)
{
    if(!ArchiveReplayActiveV87(B))return false;const auto& Session=ArchiveReplaysV87.FindChecked(ArchiveKeyV87(B));
    const auto* Entry=Current(RuntimeFor(B));return Session.Qualified&&!Session.Assisted&&Entry&&Entry->HonourEligible;
}
bool ArchiveFullArmoryV87(const AWarfareFrontlineGameMode* B,int32 Side)
{return (Side==0||Side==1)&&ArchiveReplayActiveV87(B);}
EFRRUNationV81 ArchiveReplayNationV87(const AWarfareFrontlineGameMode* B)
{const auto* Session=B?ArchiveReplaysV87.Find(ArchiveKeyV87(B)):nullptr;return Session?Session->Nation:EFRRUNationV81::Invalid;}
int32 ArchiveReplayMissionV87(const AWarfareFrontlineGameMode* B)
{const auto* Session=B?ArchiveReplaysV87.Find(ArchiveKeyV87(B)):nullptr;return Session?Session->MissionOrder:INDEX_NONE;}
EFrontlineDifficulty ArchiveReplayDifficultyV87(const AWarfareFrontlineGameMode* B)
{const auto* Session=B?ArchiveReplaysV87.Find(ArchiveKeyV87(B)):nullptr;return Session?Session->Level:EFrontlineDifficulty::Rookie;}
bool BeginArchiveReplayV87(AWarfareFrontlineGameMode* B,EFRRUNationV81 N,int32 Order,EFrontlineDifficulty Level)
{
    PruneArchiveOwnersV87();
    if(!B||B->Phase!=EFrontlinePhase::Briefing||uint8(Level)>2||!AvailableArchiveReplayV87(B,N)
        ||FrontlineUltimateArchiveV8215::Active(B)||LegacyArchiveRestoresV87.Contains(ArchiveKeyV87(B)))return false;
    auto& Previous=RuntimeFor(B);if(Previous.ReadOnly||Previous.PendingOutcome.IsSet()||Previous.BoundEncounter.IsValid())return false;
    const wfexp::Mission* Operation=nullptr;for(const auto& M:wfexp::Missions())if(M.faction==NativeNation(N)&&M.order==Order){Operation=&M;break;}
    if(!Operation)return false;
    FEntry Replay;Replay.Id=FGuid::NewGuid();Replay.Level=Level;Replay.CreatedUtc=Replay.UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    if(!wfexp::tactical::NewTacticalCampaign(NativeNation(N),Replay.Campaign))return false;
    // Synthetic in-memory progression enables selection only. No victories or
    // campaign completion are credited to the player or the restored record.
    for(const auto& M:wfexp::Missions())if(M.faction==NativeNation(N))
    {
        const auto Token=wfexpue::Narrow(FGuid::NewGuid().ToString(EGuidFormats::Digits));
        if(!wfexp::SelectMission(Replay.Campaign.progression,M.id)||!wfexp::BeginEncounter(Replay.Campaign.progression,M.id,Token)
            ||!wfexp::RecordVictory(Replay.Campaign.progression,M.id,Token))return false;
    }
    if(!wfexp::SelectMission(Replay.Campaign.progression,Operation->id))return false;
    int64 OptionBudget=0;
    for(const auto& Option:wfexp::tactical::Options())if(Option.faction==NativeNation(N)&&Option.implemented)OptionBudget+=Option.cost;
    if(OptionBudget<0||OptionBudget>99999)return false;
    Replay.Campaign.progression.researchPoints=FMath::Max(Replay.Campaign.progression.researchPoints,int(OptionBudget));
    for(const auto& Option:wfexp::tactical::Options())if(Option.faction==NativeNation(N)&&Option.implemented)
        if(!wfexp::tactical::PurchaseOption(Replay.Campaign,Option.id))return false;
    if(!wfexp::tactical::ValidateCampaign(Replay.Campaign))return false;
    auto Runtime=MakeShared<FRuntime>();Runtime->Loaded=true;Runtime->Volatile=true;Runtime->Selected=true;
    Runtime->Current=Runtime->Repository.Recent=Replay.Id;Runtime->Repository.Entries.Add(MoveTemp(Replay));
    RefreshRows(*Runtime);
    FArchiveReplayV87 Session;Session.Restore=Sessions.FindChecked(ArchiveKeyV87(B));Session.Replay=Runtime->Current;
    Session.Nation=N;Session.Level=Level;Session.MissionOrder=Order;Session.Qualified=true;
    FFrontlineNationalArchiveAccessV87::ReadFlags(B,Session.HostVolatile,Session.HostReadOnly);
    ArchiveReplaysV87.Add(ArchiveKeyV87(B),MoveTemp(Session));Sessions.Add(ArchiveKeyV87(B),Runtime);
    FFrontlineNationalArchiveAccessV87::SetFlags(B,true,false);FFrontlineNationalArchiveAccessV87::Reset(B);
    return ArchiveReplayActiveV87(B)&&B->Phase==EFrontlinePhase::Briefing;
}
void LeaveArchiveReplayV87(AWarfareFrontlineGameMode* B)
{
    if(!B)return;const auto* Found=ArchiveReplaysV87.Find(ArchiveKeyV87(B));if(!Found)return;
    const auto Restore=*Found;
    // Retire only the temporary encounter before restoring the original
    // runtime. No late shell, callback or replay GUID may reach its repository.
    FFrontlineNationalArchiveAccessV87::Reset(B);
    Sessions.Add(ArchiveKeyV87(B),Restore.Restore);ArchiveReplaysV87.Remove(ArchiveKeyV87(B));
    FFrontlineNationalArchiveAccessV87::SetFlags(B,Restore.HostVolatile,Restore.HostReadOnly);
    FFrontlineNationalArchiveAccessV87::Reset(B);
}
void MarkArchiveReplayAssistedV87(AWarfareFrontlineGameMode* B)
{
    if(!ArchiveReplayActiveV87(B))return;auto& Session=ArchiveReplaysV87.FindChecked(ArchiveKeyV87(B));Session.Assisted=true;
    if(auto* Entry=Current(RuntimeFor(B)))Entry->HonourEligible=false;
}
bool SetArchiveOptionEnabledV87(AWarfareFrontlineGameMode* B,const FString& Id,bool Enabled)
{
    if(!ArchiveReplayActiveV87(B)||B->Phase!=EFrontlinePhase::Briefing)return false;
    auto& Runtime=RuntimeFor(B);if(Runtime.PendingOutcome.IsSet()||Runtime.BoundEncounter.IsValid())return false;
    auto Candidate=Runtime.Repository;auto& Campaign=Candidate.Entries[0].Campaign;const auto Key=wfexpue::Narrow(Id);
    const auto* Option=wfexp::tactical::FindOption(Key);if(!Option||Option->faction!=Campaign.faction||!Option->implemented)return false;
    auto& Owned=Campaign.purchasedOptions;
    if(Enabled)
    {
        std::set<std::string> Needed{Key};
        for(std::size_t Pass=0;Pass<wfexp::tactical::Options().size();++Pass)
            for(const auto& Item:wfexp::tactical::Options())if(Needed.count(Item.id))for(const auto& Parent:Item.prerequisites)Needed.insert(Parent);
        for(std::size_t Pass=0;Pass<wfexp::tactical::Options().size();++Pass)
            for(const auto& Item:wfexp::tactical::Options())if(Needed.count(Item.id)&&std::find(Owned.begin(),Owned.end(),Item.id)==Owned.end()
                &&Item.faction==Campaign.faction&&Item.implemented&&std::all_of(Item.prerequisites.begin(),Item.prerequisites.end(),[&](const auto& Parent){return std::find(Owned.begin(),Owned.end(),Parent)!=Owned.end();}))Owned.push_back(Item.id);
    }
    else
    {
        Owned.erase(std::remove(Owned.begin(),Owned.end(),Key),Owned.end());
        for(std::size_t Pass=0;Pass<wfexp::tactical::Options().size();++Pass)
        {
            const auto Before=Owned;
            Owned.erase(std::remove_if(Owned.begin(),Owned.end(),[&](const auto& ItemId)
            {const auto* Item=wfexp::tactical::FindOption(ItemId);return !Item||std::any_of(Item->prerequisites.begin(),Item->prerequisites.end(),[&](const auto& Parent){return std::find(Before.begin(),Before.end(),Parent)==Before.end();});}),Owned.end());
            if(Owned.size()==Before.size())break;
        }
    }
    return wfexp::tactical::ValidateCampaign(Campaign)&&Commit(Runtime,MoveTemp(Candidate));
}
bool SuspendForLegacyArchiveV87(AWarfareFrontlineGameMode* B)
{
    PruneArchiveOwnersV87();
    if(!B||B->Phase!=EFrontlinePhase::Briefing||ArchiveReplayActiveV87(B))return false;
    if(!Active(B))return true;auto& Previous=RuntimeFor(B);
    if(Previous.PendingOutcome.IsSet()||Previous.BoundEncounter.IsValid()||LegacyArchiveRestoresV87.Contains(ArchiveKeyV87(B)))return false;
    LegacyArchiveRestoresV87.Add(ArchiveKeyV87(B),Sessions.FindChecked(ArchiveKeyV87(B)));
    auto Empty=MakeShared<FRuntime>();Empty->Loaded=true;Empty->Volatile=true;Sessions.Add(ArchiveKeyV87(B),Empty);return true;
}
void RestoreAfterLegacyArchiveV87(AWarfareFrontlineGameMode* B)
{
    if(!B)return;const auto* Found=LegacyArchiveRestoresV87.Find(ArchiveKeyV87(B));if(!Found)return;
    const auto Restore=*Found;LegacyArchiveRestoresV87.Remove(ArchiveKeyV87(B));Sessions.Add(ArchiveKeyV87(B),Restore);
    FFrontlineNationalArchiveAccessV87::Reset(B);
}
#if WITH_EDITOR
bool PrepareParityCampaignReportV87(AWarfareFrontlineGameMode* B,EFRRUNationV81 N,const FFrontlineCombatRecordV65& Record)
{
    FString User;
    if(!FRRUParityReviewSafetyV87::UserDir(User)||!B||!B->IsProgressionVolatile()||B->Phase!=EFrontlinePhase::Briefing
        ||ArchiveReplayActiveV87(B)||(N!=EFRRUNationV81::French&&N!=EFRRUNationV81::Russian))return false;
    auto& Runtime=RuntimeFor(B);
    const FString Expected=FPaths::Combine(User,TEXT("FRRUReview/campaigns-tactical-v2.json"));
    if(!Runtime.Path.Equals(Expected,ESearchCase::IgnoreCase)||Runtime.PendingOutcome.IsSet()||Runtime.BoundEncounter.IsValid())return false;
    if(!Start(B,N,EFrontlineDifficulty::Veteran))return false;
    auto Candidate=Runtime.Repository;auto* Entry=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==Runtime.Current;});if(!Entry)return false;
    wfexp::tactical::Campaign Synthetic;if(!wfexp::tactical::NewTacticalCampaign(NativeNation(N),Synthetic))return false;
    for(const auto& Mission:wfexp::Missions())if(Mission.faction==NativeNation(N))
    {
        const auto Token=wfexpue::Narrow(FGuid::NewGuid().ToString(EGuidFormats::Digits));
        if(!wfexp::SelectMission(Synthetic.progression,Mission.id)||!wfexp::BeginEncounter(Synthetic.progression,Mission.id,Token)
            ||!wfexp::RecordVictory(Synthetic.progression,Mission.id,Token))return false;
    }
    Entry->Campaign=std::move(Synthetic);Entry->CombatRecord=Record;Entry->Kills=Record.Total.Infantry;Entry->ArmourKills=Record.Total.Armour;
    Entry->HonourEligible=false;Entry->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    // This schema-checked write is confined to the explicit disposable path.
    // It supplies report/ending layout data, never a campaign completion proof.
    return Commit(Runtime,MoveTemp(Candidate));
}
void RunArchiveReplayChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString Disposable;
    const bool Allowed=B&&B->IsProgressionVolatile()&&B->Phase==EFrontlinePhase::Briefing
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ParityReview"))&&ReviewPath(Disposable)
        &&CampaignsAvailable(EFRRUNationV81::French)&&CampaignsAvailable(EFRRUNationV81::Russian);
    Check(TEXT("v87_national_archive_checks_require_explicit_disposable_review"),Allowed);if(!Allowed)return;
    Check(TEXT("v87_national_campaign_gate_rejects_enemy_and_unknown_player_nations"),!CampaignsAvailable(EFRRUNationV81::AustroHungarian)
        &&!CampaignsAvailable(EFRRUNationV81::Invalid)&&!CampaignsAvailable(static_cast<EFRRUNationV81>(254)));
    const auto OriginalRuntime=Sessions.FindChecked(ArchiveKeyV87(B));
    const bool ExactPath=OriginalRuntime->Path.Equals(Disposable,ESearchCase::IgnoreCase);
    Check(TEXT("v87_national_archive_original_path_is_exact_disposable_repository"),ExactPath);if(!ExactPath)return;
    FString Files[2]={OriginalRuntime->Path,OriginalRuntime->Path+TEXT(".bak")};
    bool Existed[2]={};TArray<uint8> BeforeBytes[2];
    for(int32 Index=0;Index<2;++Index)
    {
        Existed[Index]=IFileManager::Get().FileExists(*Files[Index]);
        const int64 Length=Existed[Index]?IFileManager::Get().FileSize(*Files[Index]):0;
        const bool Readable=!Existed[Index]||(Length>=0&&Length<=MaximumRepositoryBytes&&FFileHelper::LoadFileToArray(BeforeBytes[Index],*Files[Index]));
        Check(FString::Printf(TEXT("v87_national_archive_original_file_snapshot_%d"),Index),Readable);if(!Readable)return;
    }
    FString Before;Check(TEXT("v87_national_archive_original_repository_encoded"),Encode(OriginalRuntime->Repository,Before));
    const FGuid OriginalCurrent=OriginalRuntime->Current;const bool OriginalSelected=OriginalRuntime->Selected;
    bool HostVolatile=false,HostReadOnly=false;FFrontlineNationalArchiveAccessV87::ReadFlags(B,HostVolatile,HostReadOnly);
    for(const auto N:{EFRRUNationV81::French,EFRRUNationV81::Russian})
    {
        const FString Prefix=N==EFRRUNationV81::French?TEXT("v87_french_archive."):TEXT("v87_russian_archive.");
        const auto& Preview=ArchiveMissionsV87(B,N);
        Check(Prefix+TEXT("preview_ten_distinct_chronological_operations"),Preview.Num()==10&&Preview[0].CalendarDateV41<Preview.Last().CalendarDateV41);
        Check(Prefix+TEXT("preview_leaves_current_runtime_unchanged"),Sessions.FindChecked(ArchiveKeyV87(B))==OriginalRuntime
            &&OriginalRuntime->Current==OriginalCurrent&&OriginalRuntime->Selected==OriginalSelected);
        const bool Began=BeginArchiveReplayV87(B,N,0,EFrontlineDifficulty::Veteran);Check(Prefix+TEXT("public_begin_qualified_temporary_replay"),Began);
        if(!Began)continue;
        auto& Temporary=RuntimeFor(B);const auto* Entry=Current(Temporary);
        Check(Prefix+TEXT("distinct_guid_empty_path_volatile_single_record"),Entry&&Entry->Id!=OriginalCurrent&&Temporary.Volatile&&Temporary.Path.IsEmpty()
            &&Temporary.Repository.Entries.Num()==1&&Sessions.FindChecked(ArchiveKeyV87(B))!=OriginalRuntime&&B->IsProgressionVolatile());
        int32 Expected=0,Owned=0;for(const auto& Option:wfexp::tactical::Options())if(Option.faction==NativeNation(N)&&Option.implemented)
        {++Expected;if(OwnsOption(B,wfexpue::Wide(Option.id)))++Owned;}
        Check(Prefix+TEXT("all_implemented_national_options_owned"),Expected==15&&Expected<=24&&Owned==Expected);
        TSharedPtr<FJsonObject> EncodedTactical;wfexp::tactical::Campaign DecodedTactical;FString CodecError;
        const bool CodecRoundTrip=Entry&&wfexpue::WriteTacticalCampaign(Entry->Campaign,EncodedTactical,CodecError)
            &&EncodedTactical.IsValid()&&wfexpue::ReadTacticalCampaign(*EncodedTactical,DecodedTactical,CodecError);
        Check(Prefix+TEXT("fifteen_stable_ids_roundtrip_unchanged_tactical_codec"),CodecRoundTrip
            &&DecodedTactical.purchasedOptions==Entry->Campaign.purchasedOptions&&DecodedTactical.profile==Entry->Campaign.profile);
        Check(Prefix+TEXT("tactical_profile_never_stacks_classic_percentages"),Entry&&Entry->Campaign.profile==wfexp::tactical::Profile::TacticalV2
            &&Entry->Campaign.progression.technologyMask==0&&Entry->Campaign.progression.boughtTechnologyIds.empty());
        Check(Prefix+TEXT("both_sides_full_armory_qualified_replay_no_campaign_ending"),ArchiveFullArmoryV87(B,0)&&ArchiveFullArmoryV87(B,1)
            &&!ArchiveFullArmoryV87(B,2)&&ArchiveReplayEligibleV87(B)&&!CanOpenEnding(B));
        const auto* First=wfexp::FindMission(wfexpue::Narrow(MissionId(B)));
        Check(Prefix+TEXT("early_roster_extra_date_legal_command_section_selectable"),First&&wfexp::RoleFor(NativeNation(N),4,First->date)
            &&!(First->roleMask&(1u<<4))&&SetSelection(B,First->roleMask|(1u<<4),First->supportMask));
        const FString Basic=wfexpue::Wide(std::string(wfexp::FactionId(NativeNation(N)))+".tech.0");
        Check(Prefix+TEXT("disable_option_and_dependent_options_keeps_native_contract"),SetArchiveOptionEnabledV87(B,Basic,false)
            &&!OwnsOption(B,Basic)&&wfexp::tactical::ValidateCampaign(Current(RuntimeFor(B))->Campaign).ok);
        bool Restored=true;
        for(const auto& Option:wfexp::tactical::Options())if(Option.faction==NativeNation(N)&&Option.implemented)
            Restored&=SetArchiveOptionEnabledV87(B,wfexpue::Wide(Option.id),true);
        Check(Prefix+TEXT("reenable_options_restores_prerequisites_without_cost"),Restored&&Current(RuntimeFor(B))->Campaign.purchasedOptions.size()==std::size_t(Expected));
        const FString ArmourPrefix=N==EFRRUNationV81::French?TEXT("fr.armour."):TEXT("ru.armour.");
        const int32 ArmourPointsBefore=ResearchPoints(B);
        const bool CrewDisabled=SetArchiveOptionEnabledV87(B,ArmourPrefix+TEXT("crew"),false);
        Check(Prefix+TEXT("archive_disabling_crew_cascades_only_passive_armour_dependents"),CrewDisabled
            &&!OwnsOption(B,ArmourPrefix+TEXT("crew"))&&!OwnsOption(B,ArmourPrefix+TEXT("repair"))&&!OwnsOption(B,ArmourPrefix+TEXT("prepare"))
            &&Current(RuntimeFor(B))->Campaign.purchasedOptions.size()==12);
        const bool RepairRestored=SetArchiveOptionEnabledV87(B,ArmourPrefix+TEXT("repair"),true);
        Check(Prefix+TEXT("archive_repair_toggle_restores_crew_without_granting_preparation"),RepairRestored
            &&OwnsOption(B,ArmourPrefix+TEXT("crew"))&&OwnsOption(B,ArmourPrefix+TEXT("repair"))&&!OwnsOption(B,ArmourPrefix+TEXT("prepare")));
        Check(Prefix+TEXT("archive_restoring_all_passives_changes_no_research_points"),SetArchiveOptionEnabledV87(B,ArmourPrefix+TEXT("prepare"),true)
            &&ResearchPoints(B)==ArmourPointsBefore&&Current(RuntimeFor(B))->Campaign.purchasedOptions.size()==15);
        const int32 AllocationPoints=ResearchPoints(B);
        Check(Prefix+TEXT("early_archive_pioneer_attachment_uses_real_free_transaction"),CanRequisitionV87(B,0)
            &&BuyRequisitionV87(B,0)&&RequisitionV87(B)==0&&ResearchPoints(B)==AllocationPoints);
        Check(Prefix+TEXT("early_archive_attachment_cannot_invent_a_date_illegal_vehicle"),!CanRequisitionV87(B,2));
        int32 SmokeAllocationPoints=AllocationPoints;
        if(First&&!wfexp::SupportFor(NativeNation(N),int32(EFrontlineSupport::Smoke),First->date))
        {
            const auto Early=FrontlineCampaignV32::GetLoadout(B);
            Check(Prefix+TEXT("early_archive_date_illegal_smoke_is_rejected_without_replacing_pioneers"),
                !SetSelection(B,Early.SelectedRoles,Early.SelectedSupports|(1u<<uint8(EFrontlineSupport::Smoke)))
                &&!CanRequisitionV87(B,1)&&!BuyRequisitionV87(B,1)&&RequisitionV87(B)==0&&ResearchPoints(B)==AllocationPoints);
            Check(Prefix+TEXT("early_unpaid_pioneer_cancellation_mints_no_points"),BuyRequisitionV87(B,0)
                &&RequisitionV87(B)==INDEX_NONE&&ResearchPoints(B)==AllocationPoints);
            const wfexp::Mission* SmokeOperation=nullptr;
            for(const auto& Mission:wfexp::Missions())if(Mission.faction==NativeNation(N)
                &&wfexp::SupportFor(Mission.faction,int32(EFrontlineSupport::Smoke),Mission.date)
                &&(!SmokeOperation||Mission.order<SmokeOperation->order))SmokeOperation=&Mission;
            LeaveArchiveReplayV87(B);
            const bool Later=SmokeOperation&&BeginArchiveReplayV87(B,N,SmokeOperation->order,EFrontlineDifficulty::Veteran);
            Check(Prefix+TEXT("public_reopen_uses_first_date_legal_smoke_operation"),Later
                &&MissionId(B)==wfexpue::Wide(SmokeOperation->id)&&CalendarDate(B)==SmokeOperation->date
                &&WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::Smoke,0));
            if(!Later){LeaveArchiveReplayV87(B);return;}
            SmokeAllocationPoints=ResearchPoints(B);
            Check(Prefix+TEXT("later_smoke_fixture_has_real_free_pioneer_to_replace"),BuyRequisitionV87(B,0)
                &&RequisitionV87(B)==0&&ResearchPoints(B)==SmokeAllocationPoints);
        }
        const auto SmokeLoadout=FrontlineCampaignV32::GetLoadout(B);
        const uint32 WithSmoke=SupportSelection(B)|(1u<<uint8(EFrontlineSupport::Smoke));
        const bool SmokeAssigned=SetSelection(B,SmokeLoadout.SelectedRoles,WithSmoke);
        Check(Prefix+TEXT("archive_replaces_attachment_without_cost_or_refund"),SmokeAssigned&&CanRequisitionV87(B,1)
            &&BuyRequisitionV87(B,1)&&RequisitionV87(B)==1&&ResearchPoints(B)==SmokeAllocationPoints);
        Check(Prefix+TEXT("archive_second_click_cancels_unpaid_attachment_without_minting_points"),BuyRequisitionV87(B,1)
            &&RequisitionV87(B)==INDEX_NONE&&ResearchPoints(B)==SmokeAllocationPoints);
        MarkArchiveReplayAssistedV87(B);Check(Prefix+TEXT("cheats_revoke_only_temporary_replay_eligibility"),!ArchiveReplayEligibleV87(B));
        LeaveArchiveReplayV87(B);
        FString After;bool AfterVolatile=false,AfterReadOnly=false;FFrontlineNationalArchiveAccessV87::ReadFlags(B,AfterVolatile,AfterReadOnly);
        Check(Prefix+TEXT("leave_restores_exact_original_repository_and_host_flags"),!ArchiveReplayActiveV87(B)
            &&Sessions.FindChecked(ArchiveKeyV87(B))==OriginalRuntime&&OriginalRuntime->Current==OriginalCurrent
            &&OriginalRuntime->Selected==OriginalSelected&&Encode(OriginalRuntime->Repository,After)&&Before==After
            &&AfterVolatile==HostVolatile&&AfterReadOnly==HostReadOnly&&B->Phase==EFrontlinePhase::Briefing);
        for(int32 Index=0;Index<2;++Index)
        {
            const bool ExistsNow=IFileManager::Get().FileExists(*Files[Index]);TArray<uint8> AfterBytes;
            const int64 Length=ExistsNow?IFileManager::Get().FileSize(*Files[Index]):0;
            const bool Same=ExistsNow==Existed[Index]&&(!ExistsNow||(Length>=0&&Length<=MaximumRepositoryBytes
                &&FFileHelper::LoadFileToArray(AfterBytes,*Files[Index])&&BeforeBytes[Index]==AfterBytes));
            Check(Prefix+FString::Printf(TEXT("original_repository_file_unchanged_%d"),Index),Same);
        }
    }
    RunPaidRequisitionChecksV87(B,Check);
}
#endif
