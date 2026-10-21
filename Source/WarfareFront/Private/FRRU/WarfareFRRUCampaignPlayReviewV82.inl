// Included after the existing natural scheduler in the presentation translation
// unit. This boxed review uses normal public campaign transactions and real
// objective orders. No synthetic archive, outcome, host clock or actor placement.
#if WITH_EDITOR
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineAchievementsV76.h"
namespace FRRUCampaignPlayReviewV82
{
using FBase=FRRUNaturalReviewV82::FScheduler;
const FRRUNaturalReviewV82::FScenario Cases[]={
    {TEXT("fresh-fr"),TEXT("fr.mort_homme"),FRRUNaturalReviewV82::EProbe::Rifle},
    {TEXT("fresh-ru"),TEXT("ru.galicia"),FRRUNaturalReviewV82::EProbe::Rifle}};
const FRRUNaturalReviewV82::FScenario OrdinaryCasesV87[]={
    {TEXT("fresh-fr-ordinary"),TEXT("fr.mort_homme"),FRRUNaturalReviewV82::EProbe::Ordinary,true},
    {TEXT("fresh-ru-ordinary"),TEXT("ru.galicia"),FRRUNaturalReviewV82::EProbe::Ordinary,true}};
TSharedPtr<FJsonObject> AccountSnapshot()
{
    auto Snapshot=MakeShared<FJsonObject>();Snapshot->SetBoolField(TEXT("readThroughPublicAchievementApi"),true);
    Snapshot->SetNumberField(TEXT("catalogCount"),FrontlineAchievementsV76::Count);
    Snapshot->SetNumberField(TEXT("earnedCount"),FrontlineAchievementsV76::EarnedCount());
    Snapshot->SetNumberField(TEXT("completedRouteMask"),FrontlineAchievementsV76::CompletedCampaignRouteMaskV82());
    TArray<TSharedPtr<FJsonValue>> Records,EarnedIds;
    for(int32 Index=0;Index<FrontlineAchievementsV76::Count;++Index)
    {
        const auto& Definition=FrontlineAchievementsV76::Definition(Index);const bool Earned=FrontlineAchievementsV76::Earned(Index);
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("index"),Index);Row->SetStringField(TEXT("id"),Definition.Id);
        Row->SetNumberField(TEXT("nation"),int32(Definition.Nation));Row->SetBoolField(TEXT("earned"),Earned);
        Row->SetStringField(TEXT("statusEN"),FrontlineAchievementsV76::Status(Index,false));Row->SetStringField(TEXT("statusZH"),FrontlineAchievementsV76::Status(Index,true));
        Records.Add(MakeShared<FJsonValueObject>(Row));if(Earned)EarnedIds.Add(MakeShared<FJsonValueString>(Definition.Id));
    }
    Snapshot->SetArrayField(TEXT("records"),Records);Snapshot->SetArrayField(TEXT("earnedIds"),EarnedIds);return Snapshot;
}
TSharedPtr<FJsonObject> CampaignRecord(const AWarfareFrontlineGameMode* Battle)
{
    auto Record=MakeShared<FJsonObject>();FFRRUCampaignSummaryV82 Summary;
    const bool Found=WarfareFRRUCampaignV82::CurrentRecord(Battle,Summary);Record->SetBoolField(TEXT("available"),Found);
    if(!Found)return Record;
    Record->SetStringField(TEXT("guid"),Summary.Id.ToString(EGuidFormats::Digits));Record->SetNumberField(TEXT("nation"),int32(Summary.PlayerNation));
    Record->SetNumberField(TEXT("difficulty"),int32(Summary.Difficulty));Record->SetStringField(TEXT("mission"),Summary.MissionId);
    Record->SetNumberField(TEXT("victories"),Summary.Victories);Record->SetNumberField(TEXT("defeats"),Summary.Defeats);
    Record->SetNumberField(TEXT("completed"),Summary.Completed);Record->SetNumberField(TEXT("researchPoints"),Summary.ResearchPoints);
    Record->SetNumberField(TEXT("infantryKills"),Summary.InfantryKills);Record->SetNumberField(TEXT("armourKills"),Summary.ArmourKills);Record->SetNumberField(TEXT("orders"),Summary.Orders);
    Record->SetNumberField(TEXT("selectedRoles"),WarfareFRRUCampaignV82::RoleSelection(Battle));Record->SetNumberField(TEXT("selectedSupport"),WarfareFRRUCampaignV82::SupportSelection(Battle));
    TArray<TSharedPtr<FJsonValue>> Armour;for(const FString& Id:WarfareFRRUCampaignV82::ArmourSelection(Battle))Armour.Add(MakeShared<FJsonValueString>(Id));
    Record->SetArrayField(TEXT("selectedArmour"),Armour);Record->SetBoolField(TEXT("honourEligible"),WarfareFRRUCampaignV82::HonourEligible(Battle));
    TArray<TSharedPtr<FJsonValue>> Options;for(const auto& Row:WarfareFRRUCampaignV82::Options(Battle))if(Row.Owned)Options.Add(MakeShared<FJsonValueString>(Row.Id));
    Record->SetArrayField(TEXT("tacticalOptions"),Options);Record->SetStringField(TEXT("technologyMode"),TEXT("tactical_v2"));
    auto FullCombat=MakeShared<FJsonObject>();FrontlineCombatRecordV65::WriteArchive(*FullCombat,Summary.CombatRecord);
    Record->SetObjectField(TEXT("fullCombatRecord"),FullCombat);
    // The ended encounter keeps its own date until its binding is retired.
    // Persistence compares the newly selected mission's date, separately.
    const auto* SelectedMission=wfexp::FindMission(wfexpue::Narrow(Summary.MissionId));
    Record->SetNumberField(TEXT("calendarDate"),SelectedMission?SelectedMission->date:0);
    Record->SetNumberField(TEXT("encounterCalendarDate"),WarfareFRRUCampaignV82::CalendarDate(Battle));
    return Record;
}
struct FScheduler: FBase
{
    EFRRUNationV81 WantedNation=EFRRUNationV81::Invalid;EFrontlineDifficulty WantedDifficulty=EFrontlineDifficulty::Rookie;
    FString RequestedDifficultyLabel=TEXT("Rookie");bool ContinueOnly=false,StartedNormally=false,VictoryObserved=false,ColdAccountMatches=false;
    bool VictoryChecked=false,ContinueReadbackChecked=false,HonoursOpened=false,HonoursCaptured=false;
    bool OutcomeCommitWitnessedV87=false;
    bool RecoverEarnedContinueV87=false;FString RecoverySourceRunV87;
    bool PlayEarnedNextV87=false;FString EarnedSourceRunV87;
    TSharedPtr<FJsonObject> AccountAtColdContinueV87;
    bool FastPlayRequestedV87=false,FastPlayHudClickAppliedV87=false;
    float FastPlayActualSpeedV87=1.f,FastPlayClickedAtHostV87=-1.f;
    TSharedPtr<FJsonObject> OutcomeRecordBeforeReturnV87,NativeObjectiveBeforeReturnV87;
    double HonoursOpenedAt=0;FString HonoursCaptureName;int32 HonoursPage=INDEX_NONE,HonoursChapter=INDEX_NONE,HonoursFocus=INDEX_NONE;
    TSharedPtr<FJsonObject> HonoursAccount;
    FGuid ActualCampaign;TSharedPtr<FJsonObject> Expected;FString LastStage;TArray<FString> StagesSeen;
    TSharedPtr<FJsonObject> AccountBefore,AccountAfter,ExpectedAccount;
    TArray<FString> CompletedStagesWitnessed,PassagesCleared;FString EffectiveStage,EffectiveTarget;
    int32 EffectiveLost=0,EffectiveAlive=0,EngineerSurvivors=0;TArray<TSharedPtr<FJsonValue>> EffectiveSquads;
    int32 CoverRifle=INDEX_NONE,FireObserver=INDEX_NONE,LeadSectionsOrdered=0;bool PioneerMovedAfterClear=false,CoverObserved=false;
    int32 RifleRelayOrderSquad=INDEX_NONE,RifleRelayArrivalSquad=INDEX_NONE;TSharedPtr<FJsonObject> LastRifleReadiness;
    int32 RearPioneer=INDEX_NONE,RearPioneerArrivedAlive=0;bool RearPioneerRecruited=false,RearPioneerArrived=false,RearPioneerHeld=false;
    int32 FrenchGun=INDEX_NONE,FrenchRetiringGun=INDEX_NONE,FrenchFlank=INDEX_NONE;
    bool FrenchObserverDispatched=false,FrenchFlankOrdered=false;
    double NextFirePlanAttemptAt=0,NextObservationAt=0,NextDirectSupportAt=0,NextSmokeAttemptAt=0;
    TArray<TSharedPtr<FJsonValue>> ActualSupportReceipts;TSet<FString> SeenSupportReceipts;
    bool Stationary(int32 Id)const
    {
        const auto* ActiveBattle=Battle();const auto* Group=ActiveBattle?ActiveBattle->FindSquad(Id):nullptr;
        if(!Group||Group->LivingCount()<=0||Group->bAdvancing||Group->bHoldPosition)return false;
        for(const auto& Weak:Group->Members)if(const auto* Soldier=Weak.Get();Soldier&&Soldier->IsAlive())
            if(!Soldier->bFieldEntered||Soldier->bMoving||Soldier->GetMotionSpeed()>.5f)return false;
        return true;
    }
    // Normal squad witness only; explicit base Member(Id,index) probes stay unchanged.
    AWarfareFrontlineSoldier* LivingMember(int32 Id)const
    {
        const auto* ActiveBattle=Battle();const auto* Group=ActiveBattle?ActiveBattle->FindSquad(Id):nullptr;
        AWarfareFrontlineSoldier* Result=nullptr;if(!Group)return nullptr;
        for(const auto& Weak:Group->Members)if(auto* Unit=Weak.Get();Unit&&Unit->IsAlive()&&Unit->bFieldEntered&&Unit->IsActiveCombatant()
            &&(!Result||Unit->MemberIndex<Result->MemberIndex))Result=Unit;
        return Result;
    }
    int32 EnteredActiveLiving(int32 Id)const
    {
        const auto* ActiveBattle=Battle();const auto* Group=ActiveBattle?ActiveBattle->FindSquad(Id):nullptr;int32 Living=0;
        if(Group)for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();Unit&&Unit->Team==0&&Unit->bFieldEntered&&Unit->IsActiveCombatant())++Living;
        return Living;
    }
    void ObserveRifleReadiness()
    {
        const auto* ActiveBattle=Battle();if(!ActiveBattle||WantedNation!=EFRRUNationV81::Russian||ContinueOnly)return;
        const auto* Group=ActiveBattle->FindSquad(CoverRifle);const bool AtRelay=Stationary(CoverRifle)&&At(CoverRifle,TEXT("relay"));
        LastRifleReadiness=MakeShared<FJsonObject>();LastRifleReadiness->SetBoolField(TEXT("readOnly"),true);
        LastRifleReadiness->SetNumberField(TEXT("hostElapsed"),ActiveBattle->Elapsed);LastRifleReadiness->SetNumberField(TEXT("squad"),CoverRifle);
        LastRifleReadiness->SetNumberField(TEXT("enteredActiveAlive"),EnteredActiveLiving(CoverRifle));
        LastRifleReadiness->SetBoolField(TEXT("acceptedRelayOrderForCurrentSquad"),RifleRelayOrderSquad==CoverRifle&&CoverRifle!=INDEX_NONE);
        LastRifleReadiness->SetBoolField(TEXT("stationaryWholeFormationAtRelay"),AtRelay);
        LastRifleReadiness->SetBoolField(TEXT("physicalRelayArrivalWitnessedForCurrentSquad"),RifleRelayArrivalSquad==CoverRifle&&CoverRifle!=INDEX_NONE);
        if(Group&&!AtRelay)
        {
            LastRifleReadiness->SetBoolField(TEXT("advancing"),Group->bAdvancing);LastRifleReadiness->SetBoolField(TEXT("holding"),Group->bHoldPosition);
            TArray<TSharedPtr<FJsonValue>> Members;const AWarfareFrontlineSoldier* Blocking=nullptr;
            for(const auto& Weak:Group->Members)if(const auto* Unit=Weak.Get();IsValid(Unit)&&Unit->IsAlive())
            {
                auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("member"),Unit->MemberIndex);Row->SetBoolField(TEXT("entered"),Unit->bFieldEntered);
                Row->SetBoolField(TEXT("active"),Unit->IsActiveCombatant());Row->SetBoolField(TEXT("moving"),Unit->bMoving);
                Row->SetNumberField(TEXT("speed"),Unit->GetMotionSpeed());Row->SetStringField(TEXT("feet"),Unit->Feet().ToString());Members.Add(MakeShared<FJsonValueObject>(Row));
                if(!Blocking&&(!Unit->bFieldEntered||Unit->bMoving||Unit->GetMotionSpeed()>.5f))Blocking=Unit;
            }
            LastRifleReadiness->SetArrayField(TEXT("livingMembers"),Members);if(!Blocking)Blocking=LivingMember(CoverRifle);
            if(Blocking){LastRifleReadiness->SetNumberField(TEXT("movementWitnessMember"),Blocking->MemberIndex);LastRifleReadiness->SetObjectField(TEXT("movement"),WarfareFRRUWorldV82::MovementDiagnosis(Blocking));}
        }
        if(!Samples.IsEmpty())Samples.Last()->AsObject()->SetObjectField(TEXT("normalRifleReadiness"),LastRifleReadiness);
    }
    bool CaptureRealNationalHonours()
    {
        if(HonoursCaptured)return true;
        auto* ActiveController=Controller.Get();auto* HUD=ActiveController?Cast<AWarfareFrontlineHUD>(ActiveController->GetHUD()):nullptr;
        if(!ActiveController||!HUD)
        {Check(TEXT("normal_real_national_honours_has_actual_controller_and_hud"),false);HonoursCaptured=true;return true;}
        const double Now=FPlatformTime::Seconds();
        if(!HonoursOpened)
        {
            HonoursAccount=AccountSnapshot();ActiveController->ToggleAchievementsV76();HonoursOpened=true;HonoursOpenedAt=Now;
            const auto& Screen=ScreenState(ActiveController);HonoursPage=HUD->GetPresentationPage();
            HonoursChapter=INDEX_NONE;HonoursFocus=Screen.AchievementFocusV76;
            HonoursCaptureName=ContinueOnly?TEXT("07b-cold-continue-national-honours"):TEXT("07b-real-win-national-honours");
            const bool Opened=Screen.AchievementsV76&&HonoursPage==19
                &&HonoursFocus>=0&&HonoursFocus<FrontlineAchievementsV76::Count
                &&HonoursListV82::Visible(Screen,HonoursFocus);
            Event(TEXT("normal_public_open_real_national_honours"),Opened);
            auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("actualPage"),HonoursPage);Row->SetNumberField(TEXT("actualChapter"),HonoursChapter);
            Row->SetNumberField(TEXT("actualFocus"),HonoursFocus);Row->SetNumberField(TEXT("publicEarnedCount"),HonoursAccount->GetIntegerField(TEXT("earnedCount")));
            Check(TEXT("normal_real_national_honours_uses_unified_public_list"),Opened);
            if(!Opened){HonoursCaptured=true;return true;}
            return false;
        }
        if(Now-HonoursOpenedAt>15)
        {
            Check(TEXT("normal_real_national_honours_capture_completes_before_exit"),false,TEXT("The existing asynchronous screenshot did not complete within 15 wall seconds."));
            if(ScreenState(ActiveController).AchievementsV76)ActiveController->ToggleAchievementsV76();HonoursCaptured=true;return true;
        }
        if(Now-HonoursOpenedAt<1)return false;
        if(!PendingCaptureName.IsEmpty()&&PendingCaptureName!=HonoursCaptureName){Capture(PendingCaptureName);return false;}
        if(!Capture(HonoursCaptureName))return false;
        const auto Current=AccountSnapshot();bool Unchanged=true;
        for(const TCHAR* Key:{TEXT("catalogCount"),TEXT("earnedCount"),TEXT("completedRouteMask")})Unchanged&=Current->GetIntegerField(Key)==HonoursAccount->GetIntegerField(Key);
        const auto& Before=HonoursAccount->GetArrayField(TEXT("records"));const auto& After=Current->GetArrayField(TEXT("records"));Unchanged&=Before.Num()==After.Num();
        for(int32 Index=0;Index<FMath::Min(Before.Num(),After.Num());++Index)
        {
            const auto Prior=Before[Index]->AsObject(),Fresh=After[Index]->AsObject();
            Unchanged&=Prior->GetBoolField(TEXT("earned"))==Fresh->GetBoolField(TEXT("earned"));
            for(const TCHAR* Key:{TEXT("id"),TEXT("statusEN"),TEXT("statusZH")})Unchanged&=Prior->GetStringField(Key)==Fresh->GetStringField(Key);
        }
        Check(TEXT("normal_real_honours_capture_preserves_public_earned_account"),Unchanged);
        auto CaptureRow=Captures.Last()->AsObject();CaptureRow->SetNumberField(TEXT("actualAchievementChapter"),HonoursChapter);
        CaptureRow->SetNumberField(TEXT("actualAchievementFocus"),HonoursFocus);CaptureRow->SetObjectField(TEXT("actualPublicAccount"),Current);
        Check(TEXT("normal_real_honours_capture_stays_on_actual_page"),HUD->GetPresentationPage()==19);
        ActiveController->ToggleAchievementsV76();Event(TEXT("normal_public_close_real_national_honours"),!ScreenState(ActiveController).AchievementsV76);
        Check(TEXT("normal_real_honours_capture_closes_through_public_toggle"),!ScreenState(ActiveController).AchievementsV76);
        HonoursCaptured=true;return true;
    }
    void FireEvent(const FString& Name,bool Accepted,const FString& Target)
    {
        Event(Name,Accepted);const auto Actual=WarfareFRRUWorldV82::View(Battle());
        const auto Row=Commands.Last()->AsObject();Row->SetStringField(TEXT("stage"),Actual.StageId);
        Row->SetStringField(TEXT("target"),Target);Row->SetNumberField(TEXT("observerSquad"),FireObserver);
        Row->SetNumberField(TEXT("actualFirePhase"),Actual.FirePhase);Row->SetNumberField(TEXT("actualResolvedSalvos"),Actual.ResolvedSalvos);
        Row->SetNumberField(TEXT("actualArtilleryCooldownAfterOrder"),Battle()->SupportCooldown(EFrontlineSupport::Artillery,0));
        if(!Accepted)Row->SetStringField(TEXT("rejection"),Actual.Notice);
    }
    bool FrenchRoute(const FString& Name,int32 Section,const FString& Target)
    {
        auto* ActiveBattle=Battle();const auto* Unit=LivingMember(Section);
        const auto Before=WarfareFRRUWorldV82::MovementDiagnosis(Unit);
        const bool Accepted=WarfareFRRUWorldV82::MoveSquad(ActiveBattle,Section,Target);
        Event(Name,Accepted);const auto Row=Commands.Last()->AsObject();
        Row->SetNumberField(TEXT("squad"),Section);Row->SetStringField(TEXT("target"),Target);
        const auto After=WarfareFRRUWorldV82::View(ActiveBattle);Row->SetStringField(TEXT("stage"),After.StageId);
        FString From;Before->TryGetStringField(TEXT("currentNode"),From);Row->SetStringField(TEXT("fromNode"),From);
        Row->SetObjectField(TEXT("movementAfterOrder"),WarfareFRRUWorldV82::MovementDiagnosis(Unit));
        if(!Accepted){Row->SetStringField(TEXT("rejection"),TEXT("Public movement order rejected."));Row->SetStringField(TEXT("noticeAfterOrder"),After.Notice);}
        return Accepted;
    }
    const FFRRUNodeV82* NodeIn(const FFRRUWorldViewV82& View,const FString& Id)const
    {return View.Nodes.FindByPredicate([&](const FFRRUNodeV82& Node){return Node.Id==Id;});}
    AWarfareFrontlineSoldier* RussianPoint(const FFRRUWorldViewV82& View)const
    {
        const int32 Id=View.StageId.EndsWith(TEXT("stage.1"))?Second
            :View.StageId.EndsWith(TEXT("stage.2"))?Lead:CoverRifle;
        if(auto* Unit=LivingMember(Id))return Unit;
        return LivingMember(Lead);
    }
    bool FriendlyFireSafe(const FFRRUWorldViewV82& View,const FString& Id)const
    {
        const auto* Node=NodeIn(View,Id);const auto* ActiveBattle=Battle();
        return Node&&ActiveBattle&&ActiveBattle->IsFireSupportAreaSafe(Node->Position,EFrontlineSupport::Artillery,0);
    }
    FString VisibleEnemyFirePost(const FFRRUWorldViewV82& View)const
    {
        const auto* ActiveBattle=Battle();const auto* ObserverUnit=LivingMember(FireObserver);if(!ActiveBattle||!ObserverUnit)return {};
        FString BestPost;float BestScore=0;
        for(const auto& Node:View.Nodes)
        {
            if(!FriendlyFireSafe(View,Node.Id)||!WarfareFRRUWorldV82::HasSight(ActiveBattle,ObserverUnit->Feet()+FVector(0,0,135),Node.Position+FVector(0,0,135)))continue;
            float Score=0;
            for(AWarfareFrontlineSoldier* Enemy:ActiveBattle->Units)if(Enemy&&Enemy->Team==1&&Enemy->IsActiveCombatant())
            {
                const float Distance=float(FVector::Dist2D(Node.Position,Enemy->Feet()));
                if(Distance<1350&&WarfareFRRUWorldV82::FiringLine(ObserverUnit,Enemy))Score+=1-Distance/1500;
            }
            if(Score>BestScore){BestScore=Score;BestPost=Node.Id;}
        }
        return BestPost;
    }
    void TickFrenchFireOrders()
    {
        auto* ActiveBattle=Battle();if(!ActiveBattle||WantedNation!=EFRRUNationV81::French||ActiveBattle->Phase!=EFrontlinePhase::Fighting)return;
        // These are existing host journal receipts from real resolved rounds,
        // not elapsed-time estimates or source-only salvo counts.
        const auto Diagnostic=WarfareFRRUWorldV82::SupportDiagnosis(ActiveBattle);
        if(Diagnostic->GetBoolField(TEXT("contextValid")))for(const auto& Value:Diagnostic->GetArrayField(TEXT("activeActualPhaseReceipts")))
        {
            const auto Row=Value->AsObject();if(Row->GetIntegerField(TEXT("side"))!=0)continue;
            const FString Key=Row->GetStringField(TEXT("planId"))+TEXT(".")+FString::FromInt(Row->GetIntegerField(TEXT("resolvedSalvos")));
            if(!SeenSupportReceipts.Contains(Key)){SeenSupportReceipts.Add(Key);ActualSupportReceipts.Add(Value);}
        }
        auto View=WarfareFRRUWorldV82::View(ActiveBattle);
        const auto* Guard=ActiveBattle->FindSquad(FireObserver);
        if(!Guard||Guard->Role!=EFrontlineRole::Marksman||!Stationary(FireObserver))return;
        if(View.FirePhase>0&&View.FirePhase<4&&!FriendlyFireSafe(View,View.FireTarget))
        {
            const bool Cancelled=WarfareFRRUWorldV82::Command(ActiveBattle,EFRRUTacticalCommandV82::CancelFire);
            FireEvent(TEXT("normal_cancel_uncommitted_old_objective_draft.")+View.FireTarget,Cancelled,View.FireTarget);
            if(!Cancelled)return;View=WarfareFRRUWorldV82::View(ActiveBattle);
        }
        if((View.FirePhase==0||(View.FirePhase==4&&View.ResolvedSalvos==4))&&ActiveBattle->Elapsed>=NextFirePlanAttemptAt
            &&WarfareFRRUEquipmentV82::SupportAvailable(ActiveBattle,EFrontlineSupport::Artillery,0)
            &&ActiveBattle->SupportBlockReason(EFrontlineSupport::Artillery,0).IsEmpty())
        {
            NextFirePlanAttemptAt=ActiveBattle->Elapsed+4;
            const FString FirePost=VisibleEnemyFirePost(View);if(FirePost.IsEmpty())return;
            FireEvent(TEXT("normal_public_fire_plan.")+FirePost,WarfareFRRUWorldV82::Command(ActiveBattle,EFRRUTacticalCommandV82::PlanFire,{},FirePost),FirePost);
            View=WarfareFRRUWorldV82::View(ActiveBattle);
        }
        // Observation is driven by real host seconds even at the supported
        // 1.5x speed; a wall-second command cadence would break its fresh proof.
        if(View.FirePhase>0&&View.FirePhase<4&&ActiveBattle->Elapsed>=NextObservationAt)
        {
            NextObservationAt=ActiveBattle->Elapsed+.3;
            FireEvent(TEXT("normal_real_marksman_observation.")+View.FireTarget,WarfareFRRUWorldV82::Command(ActiveBattle,EFRRUTacticalCommandV82::Observe,{FireObserver}),View.FireTarget);
            View=WarfareFRRUWorldV82::View(ActiveBattle);
        }
        if(View.FirePhase==3&&FriendlyFireSafe(View,View.FireTarget)&&ActiveBattle->Elapsed>=NextFirePlanAttemptAt)
        {
            NextFirePlanAttemptAt=ActiveBattle->Elapsed+1;
            FireEvent(TEXT("normal_public_commit_observed_fire.")+View.FireTarget,WarfareFRRUWorldV82::Command(ActiveBattle,EFRRUTacticalCommandV82::CommitFire),View.FireTarget);
        }
    }
    void TickVisibleDirectSupport()
    {
        auto* ActiveBattle=Battle();if(!ActiveBattle||ActiveBattle->Phase!=EFrontlinePhase::Fighting||ActiveBattle->Elapsed<NextDirectSupportAt)return;
        NextDirectSupportAt=ActiveBattle->Elapsed+.75;
        const auto SupportView=WarfareFRRUWorldV82::View(ActiveBattle);
        TSet<FString> FrenchMortarFootholds;
        if(WantedNation==EFRRUNationV81::French)
        {
            for(const auto& Node:SupportView.Nodes)if(Node.Owner==0)FrenchMortarFootholds.Add(Node.Id);
            TSet<int32> ReadSections;
            for(const auto& Friendly:ActiveBattle->Units)if(Friendly&&Friendly->Team==0&&Friendly->IsActiveCombatant()
                &&!ReadSections.Contains(Friendly->SquadId))
            {
                ReadSections.Add(Friendly->SquadId);FString OrderedPost;
                WarfareFRRUWorldV82::MovementDiagnosis(Friendly)->TryGetStringField(TEXT("targetNode"),OrderedPost);
                if(NodeIn(SupportView,OrderedPost))FrenchMortarFootholds.Add(OrderedPost);
            }
        }
        for(auto Kind:{EFrontlineSupport::Artillery,EFrontlineSupport::Mortar,EFrontlineSupport::AirRecon})
        {
        // French field guns retain their observed public fire-plan workflow.
        if(Kind==EFrontlineSupport::Artillery&&WantedNation!=EFRRUNationV81::Russian)continue;
        if(!WarfareFRRUEquipmentV82::SupportAvailable(ActiveBattle,Kind,0)||!ActiveBattle->SupportBlockReason(Kind,0).IsEmpty())continue;
        AWarfareFrontlineSoldier* Target=nullptr;float Best=0;
        for(AWarfareFrontlineSoldier* Enemy:ActiveBattle->Units)if(Enemy&&Enemy->Team==1&&Enemy->IsActiveCombatant())
        {
            if(!ActiveBattle->IsFireSupportAreaSafe(Enemy->Feet(),Kind,0))continue;
            if((Kind==EFrontlineSupport::Artillery||Kind==EFrontlineSupport::AirRecon)&&WantedNation==EFRRUNationV81::Russian)
            {
                // A fixed four-salvo area must also spare the actual ordered
                // destination, not just friendly positions when it is called.
                const auto Extent=ActiveBattle->FireSupportScatterExtent(Kind,0);
                TSet<int32> SeenSections;bool CrossesRouteEnd=false;
                for(AWarfareFrontlineSoldier* Friendly:ActiveBattle->Units)if(Friendly&&Friendly->Team==0&&Friendly->IsActiveCombatant()
                    &&!SeenSections.Contains(Friendly->SquadId))
                {
                    SeenSections.Add(Friendly->SquadId);FString Post;
                    WarfareFRRUWorldV82::MovementDiagnosis(Friendly)->TryGetStringField(TEXT("targetNode"),Post);
                    if(const auto* Endpoint=NodeIn(SupportView,Post))
                    {
                        const FVector Gap=Enemy->Feet()-Endpoint->Position;
                        const float Radius=Kind==EFrontlineSupport::AirRecon?920.f:FrontlineFireSupportV26::FieldGunBlastRadius;
                        const float Margin=Radius+Endpoint->Radius+160.f;
                        CrossesRouteEnd|=FMath::Abs(Gap.X)<=Extent.X+Margin&&FMath::Abs(Gap.Y)<=Extent.Y+Margin;
                    }
                }
                if(CrossesRouteEnd)continue;
            }
            if(Kind==EFrontlineSupport::Mortar&&WantedNation==EFRRUNationV81::French)
            {
                // Four salvos remain fixed while infantry moves. Protect the real
                // held/ordered node footprint as well as request-time live feet.
                const FVector2D Extent=ActiveBattle->FireSupportScatterExtent(Kind,0);bool CrossesFoothold=false;
                for(const FString& Post:FrenchMortarFootholds)if(const auto* Foothold=NodeIn(SupportView,Post))
                {
                    const FVector Gap=Enemy->Feet()-Foothold->Position;
                    const float Margin=FrontlineFireSupportV26::MortarBlastRadius+Foothold->Radius+160.f;
                    CrossesFoothold|=FMath::Abs(Gap.X)<=Extent.X+Margin&&FMath::Abs(Gap.Y)<=Extent.Y+Margin;
                }
                if(CrossesFoothold)continue;
            }
            bool Seen=false;float Nearest=MAX_flt;
            for(AWarfareFrontlineSoldier* Friendly:ActiveBattle->Units)if(Friendly&&Friendly->Team==0&&Friendly->IsActiveCombatant())
            {
                if(WarfareFRRUWorldV82::FiringLine(Friendly,Enemy))Seen=true;
                Nearest=FMath::Min(Nearest,float(FVector::Dist2D(Friendly->Feet(),Enemy->Feet())));
            }
            if(!Seen)continue;
            float Score=0;for(AWarfareFrontlineSoldier* Other:ActiveBattle->Units)if(Other&&Other->Team==1&&Other->IsActiveCombatant()
                &&FVector::DistSquared2D(Enemy->Feet(),Other->Feet())<FMath::Square(650.f))Score+=1;
            Score/=1+FMath::Square(Nearest/2500.f);
            if(WantedNation==EFRRUNationV81::Russian)
            {
                if(const auto* Point=RussianPoint(SupportView))
                    Score*=1+4.f/(1+FMath::Square(float(FVector::Dist2D(Point->Feet(),Enemy->Feet()))/2000.f));
                if(Enemy->IsMachineGunner())Score*=2.f;
                else if(Enemy->Role==EFrontlineRole::Marksman)Score*=1.5f;
            }
            if(Score>Best){Best=Score;Target=Enemy;}
        }
        if(Target)
            {
                const float CooldownBefore=ActiveBattle->SupportCooldown(Kind,0);
                const bool Accepted=ActiveBattle->RequestSupport(Kind,Target->Feet(),0);
                Event(TEXT("normal_visible_enemy_public_direct_support"),Accepted);
                const auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("hostSupportOrdinal"),int32(Kind));
                Row->SetNumberField(TEXT("actualTargetSquad"),Target->SquadId);Row->SetStringField(TEXT("targetFeet"),Target->Feet().ToString());
                Row->SetNumberField(TEXT("actualSupportCooldownBeforeOrder"),CooldownBefore);
                if(Accepted)break;
            }
        }
    }
    void TickRussianRally()
    {
        auto* ActiveBattle=Battle();if(!ActiveBattle||WantedNation!=EFRRUNationV81::Russian
            ||ActiveBattle->Phase!=EFrontlinePhase::Fighting
            ||!WarfareFRRUEquipmentV82::SupportAvailable(ActiveBattle,EFrontlineSupport::Rally,0)
            ||!ActiveBattle->SupportBlockReason(EFrontlineSupport::Rally,0).IsEmpty())return;
        auto* Point=RussianPoint(WarfareFRRUWorldV82::View(ActiveBattle));
        if(!Point||Point->RallyUntil>ActiveBattle->Elapsed||(!Point->bEngaging&&Point->Suppression<.12f))return;
        const float Before=ActiveBattle->SupportCooldown(EFrontlineSupport::Rally,0);
        const bool Accepted=ActiveBattle->RequestSupport(EFrontlineSupport::Rally,Point->Feet(),0);
        Event(TEXT("normal_public_rally_actual_contact_section"),Accepted);
        const auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("actualSquad"),Point->SquadId);
        Row->SetNumberField(TEXT("actualSupportCooldownBeforeOrder"),Before);
        Row->SetStringField(TEXT("targetFeet"),Point->Feet().ToString());
    }
    void TickPointSmoke()
    {
        auto* ActiveBattle=Battle();const auto* Point=LivingMember(Lead);if(!ActiveBattle||!Point||!Point->IsActiveCombatant()
            ||WantedNation!=EFRRUNationV81::French||ActiveBattle->Elapsed<NextSmokeAttemptAt)return;
        const auto View=WarfareFRRUWorldV82::View(ActiveBattle);
        // A real pending fire plan needs a clear observer ray; do not renew its smoke screen.
        if(View.FirePhase>0&&View.FirePhase<4)return;
        const auto* Goal=NodeIn(View,View.TargetNode);
        if(!Goal||!WarfareFRRUEquipmentV82::SupportAvailable(ActiveBattle,EFrontlineSupport::Smoke,0)
            ||!ActiveBattle->SupportBlockReason(EFrontlineSupport::Smoke,0).IsEmpty())return;
        const float Distance=float(FVector::Dist2D(Point->Feet(),Goal->Position));
        // The public primary approach screen covers the contact area and ridge.
        // Request before the unchanged 5.5s arrival; normal readiness owns renewals.
        const bool RidgeScreen=View.StageId.EndsWith(TEXT("stage.1"))&&View.TargetNode==TEXT("ridge")&&!View.FallbackAwaitingArrival;
        const auto* Approach=RidgeScreen?NodeIn(View,TEXT("assembly")):nullptr;
        if(Approach?Distance>6000.f:(Distance<500.f||Distance>4200.f))return;
        NextSmokeAttemptAt=ActiveBattle->Elapsed+3;
        const FVector Screen=Approach?Goal->Position+(Approach->Position-Goal->Position).GetSafeNormal2D()*800.f
            :Point->Feet()+(Goal->Position-Point->Feet()).GetSafeNormal2D()*FMath::Min(1450.f,Distance*.65f);
        const float CooldownBefore=ActiveBattle->SupportCooldown(EFrontlineSupport::Smoke,0);
        const bool Accepted=ActiveBattle->RequestSupport(EFrontlineSupport::Smoke,Screen,0);
        Event(TEXT("normal_public_smoke_screen_ahead_of_point"),Accepted);
        Commands.Last()->AsObject()->SetStringField(TEXT("targetFeet"),Screen.ToString());
        Commands.Last()->AsObject()->SetNumberField(TEXT("actualSupportCooldownBeforeOrder"),CooldownBefore);
    }
    void ObserveAchievementScope(const FFRRUWorldViewV82& View)
    {
        const auto* ActiveBattle=Battle();if(!ActiveBattle||(ContinueOnly&&!PlayEarnedNextV87))return;
        EffectiveLost=ActiveBattle->CombatRecordV65[0].InfantryLost;EffectiveAlive=0;
        const auto Present=[](const AWarfareFrontlineSoldier* Unit){return IsValid(Unit)&&Unit->Team==0&&Unit->IsAlive()&&(Unit->bFieldEntered||Unit->IsActiveCombatant());};
        for(const AWarfareFrontlineSoldier* Unit:ActiveBattle->Units)EffectiveAlive+=Present(Unit);
        EffectiveStage=View.StageId;EffectiveTarget=View.TargetNode;
        for(const auto& Node:View.Nodes)if(WarfareFRRUWorldV82::PassageCleared(ActiveBattle,Node.Id))PassagesCleared.AddUnique(Node.Id);
        if(View.Finished&&View.Success&&WarfareFRRUWorldV82::OutcomePersisted(ActiveBattle)&&!View.StageId.IsEmpty())CompletedStagesWitnessed.AddUnique(View.StageId);
        EffectiveSquads.Empty();EngineerSurvivors=0;
        for(const auto& Group:ActiveBattle->Squads)if(Group.Team==0)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("squad"),Group.Id);Row->SetNumberField(TEXT("role"),int32(Group.Role));
            int32 PresentAlive=0;for(const auto& Weak:Group.Members)PresentAlive+=Present(Weak.Get());
            Row->SetNumberField(TEXT("alive"),PresentAlive);Row->SetNumberField(TEXT("aliveIncludingPendingReinforcements"),Group.LivingCount());
            Row->SetNumberField(TEXT("squadAggregateInfantryKills"),Group.KillsV65.Infantry);
            FString Weapon;for(const auto& Weak:Group.Members)if(const auto* Unit=Weak.Get();Unit&&Unit->MemberIndex==0)
                if(const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Unit))Weapon=Equipment->WeaponId;
            Row->SetStringField(TEXT("leadWeapon"),Weapon);EffectiveSquads.Add(MakeShared<FJsonValueObject>(Row));
            if(Group.Role==EFrontlineRole::Pioneer)EngineerSurvivors=FMath::Max(EngineerSurvivors,PresentAlive);
        }
    }
    void FillDetail(FJsonObject& Report)
    {
        const auto* ActiveBattle=Battle();Report.SetBoolField(TEXT("normalPublicStart"),StartedNormally);
        Report.SetBoolField(TEXT("normalFastPlayRequestedV87"),FastPlayRequestedV87);
        Report.SetBoolField(TEXT("normalFastPlayHudClickAppliedV87"),FastPlayHudClickAppliedV87);
        Report.SetNumberField(TEXT("normalFastPlayActualSpeedV87"),FastPlayActualSpeedV87);
        Report.SetNumberField(TEXT("normalFastPlayClickedAtHostV87"),FastPlayClickedAtHostV87);
        Report.SetBoolField(TEXT("normalGameplayVictory"),VictoryObserved);Report.SetBoolField(TEXT("actualSubmissionPersisted"),OutcomeCommitWitnessedV87);
        Report.SetBoolField(TEXT("actualOutcomeCommitWitnessedBeforeReturnV87"),OutcomeCommitWitnessedV87);
        if(OutcomeRecordBeforeReturnV87.IsValid())Report.SetObjectField(TEXT("actualSavedRecordBeforeReportReturnV87"),OutcomeRecordBeforeReturnV87);
        if(NativeObjectiveBeforeReturnV87.IsValid())Report.SetObjectField(TEXT("actualNativeObjectiveBeforeReportReturnV87"),NativeObjectiveBeforeReturnV87);
        bool ReturnedNormallyV87=false;for(const auto& Window:RequiredWindows)
            if(Window->AsObject()->GetStringField(TEXT("window"))==TEXT("ordinary_live_first_win_return_completed"))ReturnedNormallyV87=true;
        Report.SetBoolField(TEXT("normalLiveReportReturnCompletedV87"),ReturnedNormallyV87);
        Report.SetBoolField(TEXT("earnedNextPublicContinuePlayV87"),PlayEarnedNextV87);
        Report.SetStringField(TEXT("earnedNextSourceRunV87"),EarnedSourceRunV87);
        if(AccountAtColdContinueV87.IsValid())Report.SetObjectField(TEXT("accountAtColdContinueV87"),AccountAtColdContinueV87);
        if(PlayEarnedNextV87)
        {Report.SetStringField(TEXT("reviewCompletionProvenanceV87"),TEXT("actual_earned_public_continue_next_operation"));
            Report.SetBoolField(TEXT("normalFreshStartInThisProcessV87"),false);
            Report.SetBoolField(TEXT("earnedNextActualVictoryV87"),VictoryObserved&&OutcomeCommitWitnessedV87);}
        Report.SetBoolField(TEXT("recoverEarnedContinueV87"),RecoverEarnedContinueV87);
        Report.SetStringField(TEXT("recoverySourceRunV87"),RecoverySourceRunV87);
        Report.SetBoolField(TEXT("recoveryProvesPriorLiveReportReturnV87"),false);
        Report.SetBoolField(TEXT("freshProcessContinue"),ContinueOnly);Report.SetStringField(TEXT("actualCampaignGuid"),ActualCampaign.ToString(EGuidFormats::Digits));
        Report.SetNumberField(TEXT("requestedNation"),int32(WantedNation));Report.SetNumberField(TEXT("productionBattleSpeed"),ActiveBattle?ActiveBattle->BattleSpeed:1);
        Report.SetNumberField(TEXT("requestedDifficulty"),int32(WantedDifficulty));Report.SetStringField(TEXT("requestedDifficultyLabel"),RequestedDifficultyLabel);
        Report.SetObjectField(TEXT("actualSavedRecord"),CampaignRecord(ActiveBattle));
        if(!AccountAfter.IsValid())AccountAfter=AccountSnapshot();
        Report.SetObjectField(TEXT("accountBefore"),AccountBefore.IsValid()?AccountBefore:AccountAfter);Report.SetObjectField(TEXT("accountAfter"),AccountAfter);
        TArray<TSharedPtr<FJsonValue>> Awarded;TSet<FString> Earlier;
        if(AccountBefore.IsValid())for(const auto& Id:AccountBefore->GetArrayField(TEXT("earnedIds")))Earlier.Add(Id->AsString());
        for(const auto& Id:AccountAfter->GetArrayField(TEXT("earnedIds")))if(!Earlier.Contains(Id->AsString()))Awarded.Add(MakeShared<FJsonValueString>(Id->AsString()));
        Report.SetArrayField(TEXT("newlyAwardedAchievementIds"),Awarded);Report.SetBoolField(TEXT("accountColdContinueCompared"),ContinueOnly&&ExpectedAccount.IsValid());
        Report.SetBoolField(TEXT("accountColdContinueMatches"),ColdAccountMatches);
        auto Honours=MakeShared<FJsonObject>();Honours->SetBoolField(TEXT("openedThroughPublicToggle"),HonoursOpened);
        Honours->SetBoolField(TEXT("actualPageCaptured"),CompletedCaptures.Contains(HonoursCaptureName));Honours->SetStringField(TEXT("captureName"),HonoursCaptureName);
        Honours->SetNumberField(TEXT("actualPage"),HonoursPage);Honours->SetNumberField(TEXT("actualChapter"),HonoursChapter);Honours->SetNumberField(TEXT("actualFocus"),HonoursFocus);
        if(HonoursAccount.IsValid())Honours->SetObjectField(TEXT("actualPublicAccountAtOpen"),HonoursAccount);Report.SetObjectField(TEXT("realNationalHonoursPage"),Honours);
        auto Reserve=MakeShared<FJsonObject>();Reserve->SetNumberField(TEXT("squad"),RearPioneer);Reserve->SetBoolField(TEXT("publicRecruitAccepted"),RearPioneerRecruited);
        Reserve->SetBoolField(TEXT("actualHomeArrivalObserved"),RearPioneerArrived);Reserve->SetBoolField(TEXT("publicHoldAccepted"),RearPioneerHeld);
        Reserve->SetNumberField(TEXT("enteredActiveAliveAtHomeArrival"),RearPioneerArrivedAlive);Reserve->SetNumberField(TEXT("enteredActiveAliveAtReport"),EnteredActiveLiving(RearPioneer));
        Reserve->SetNumberField(TEXT("acceptedRifleRelayOrderSquad"),RifleRelayOrderSquad);Reserve->SetNumberField(TEXT("physicalRifleRelayArrivalSquad"),RifleRelayArrivalSquad);
        Report.SetObjectField(TEXT("normalRearEngineerReserve"),Reserve);
        if(LastRifleReadiness.IsValid())Report.SetObjectField(TEXT("lastNormalRifleReadiness"),LastRifleReadiness);
        if(ExpectedAccount.IsValid())Report.SetObjectField(TEXT("accountExpectedFromFirstProcess"),ExpectedAccount);
        auto Scope=MakeShared<FJsonObject>();Scope->SetBoolField(TEXT("realFirstMissionBattleObserved"),!ContinueOnly&&StartedNormally);
        Scope->SetStringField(TEXT("mission"),Scenario->Mission);Scope->SetStringField(TEXT("lastNativeStage"),EffectiveStage);Scope->SetStringField(TEXT("lastNativeTarget"),EffectiveTarget);
        Scope->SetNumberField(TEXT("effectiveInfantryLost"),EffectiveLost);Scope->SetNumberField(TEXT("effectiveInfantryAlive"),EffectiveAlive);
        Scope->SetNumberField(TEXT("largestEngineerSectionAlive"),EngineerSurvivors);Scope->SetArrayField(TEXT("actualSquads"),EffectiveSquads);
        TArray<TSharedPtr<FJsonValue>> Completed,Cleared;for(const auto& Id:CompletedStagesWitnessed)Completed.Add(MakeShared<FJsonValueString>(Id));
        for(const auto& Id:PassagesCleared)Cleared.Add(MakeShared<FJsonValueString>(Id));
        Scope->SetArrayField(TEXT("completedNativeStagesWitnessed"),Completed);Scope->SetArrayField(TEXT("actualClearedPassageNodes"),Cleared);
        Scope->SetStringField(TEXT("stageEvidenceMethod"),TEXT("Read-only public native stage transitions and saved final objective; passages use PassageCleared. Survivors are living soldiers with actual field entry or active field presence; pending off-field reinforcements are excluded and entered embarked survivors remain counted. Squad kills are aggregate attribution, not specialist weapon counters."));
        Report.SetObjectField(TEXT("achievementBattleScope"),Scope);
        TArray<TSharedPtr<FJsonValue>> Stages;for(const FString& Id:StagesSeen)Stages.Add(MakeShared<FJsonValueString>(Id));Report.SetArrayField(TEXT("actualStagesSeen"),Stages);
        Report.SetArrayField(TEXT("actualPlayerSupportPhaseReceipts"),ActualSupportReceipts);
        Report.SetStringField(TEXT("normalCasePolicyV87"),WantedNation==EFRRUNationV81::French
            ?TEXT("mort_rifle_hotchkiss_3to1_contact_rally_v87"):TEXT("galicia_one_pioneer_maxim_rifle_1to3_contact_rally_v87"));
        Report.SetStringField(TEXT("normalSupportStrategy"),NormalCampaignCompletionV87
            ?TEXT("Normal public Start/default dated loadout; original case recruitment and contact/advance Rally, without cumulative review recruitment/Artillery/Rally caps. Only real public readiness and actual home ingress congestion defer calls; total live sections and capacity sums are diagnostic only; automatic tasks, guards, native deadlines and the original wall deadline remain authoritative. Actual recruit/support counters and public block reasons are reported. Earned report return and separate-process Continue remain independent requirements.")
            :TEXT("Normal public Start/default dated loadout, then the shared case recruitment and contact/advance Rally policy with a 24-section review budget. Existing automatic movement, actual hostile-contact artillery and normal cooldowns remain authoritative. After the real native win: public report return, actual next-assignment acknowledgement and war-room return before a distinct fresh-process Continue."));
        Report.SetStringField(TEXT("scope"),PlayEarnedNextV87?TEXT("Real earned Mort archive copied to a fresh GUID User; public cold Continue, actual selected Vaux play, one real outcome and live return to Douaumont. No synthetic progression, new normal Start, historical-save, whole-campaign, art or Shipping acceptance."):
            TEXT("Fresh normal public FR/RU campaign; real objective/Outcome commit and separate-process Continue. Boxed account/config paths only. No historical-save, complete campaign, art or Shipping acceptance."));
        if(PlayEarnedNextV87)
        {Report.SetStringField(TEXT("normalCasePolicyV87"),TEXT("earned_vaux_default_dated_loadout_ordinary_known_position"));
            Report.SetStringField(TEXT("normalSupportStrategy"),TEXT("Existing ordinary Vaux full-operation scheduler from verified public Continue; inherited dated loadout/difficulty, automatic tasks and real home ingress. Public readiness/CD and original Native/wall deadlines, no cumulative review caps, no VauxTactics or imposed alternative story; no actor/clock/outcome grants."));}
    }
    bool CheckReadback()
    {
        auto* ActiveBattle=Battle();if(!ActiveBattle||!Expected||!ExpectedAccount)return false;
        FGuid Id;const bool ValidId=FGuid::ParseExact(Expected->GetStringField(TEXT("guid")),EGuidFormats::Digits,Id)&&Id.IsValid();
        Check(TEXT("first_actual_outcome_has_valid_guid"),ValidId);if(!ValidId)return false;
        Check(TEXT("fresh_process_reads_exact_one_normal_repository_entry"),WarfareFRRUCampaignV82::Archives(ActiveBattle).Num()==1);
        const bool Loaded=WarfareFRRUCampaignV82::Continue(ActiveBattle,Id);Event(TEXT("normal_public_continue"),Loaded);
        Check(TEXT("fresh_public_continue_succeeds"),Loaded,WarfareFRRUCampaignV82::Status(ActiveBattle));if(!Loaded)return false;
        ActualCampaign=Id;auto Actual=CampaignRecord(ActiveBattle);
        Check(TEXT("fresh_continue_retains_requested_difficulty"),WarfareFRRUCampaignV82::Difficulty(ActiveBattle)==WantedDifficulty);
        for(const TCHAR* Key:{TEXT("nation"),TEXT("difficulty"),TEXT("victories"),TEXT("defeats"),TEXT("completed"),TEXT("researchPoints"),TEXT("infantryKills"),TEXT("armourKills"),TEXT("orders"),TEXT("selectedRoles"),TEXT("selectedSupport"),TEXT("calendarDate")})
            Check(FString(TEXT("fresh_readback."))+Key,Actual->GetNumberField(Key)==Expected->GetNumberField(Key));
        for(const TCHAR* Key:{TEXT("guid"),TEXT("mission"),TEXT("technologyMode")})Check(FString(TEXT("fresh_readback."))+Key,Actual->GetStringField(Key)==Expected->GetStringField(Key));
        Check(TEXT("fresh_readback.honourEligibility"),Actual->GetBoolField(TEXT("honourEligible"))==Expected->GetBoolField(TEXT("honourEligible")));
        const auto& PriorArmour=Expected->GetArrayField(TEXT("selectedArmour"));const auto& FreshArmour=Actual->GetArrayField(TEXT("selectedArmour"));
        bool SameArmour=PriorArmour.Num()==FreshArmour.Num();for(int32 Index=0;SameArmour&&Index<PriorArmour.Num();++Index)SameArmour=PriorArmour[Index]->AsString()==FreshArmour[Index]->AsString();
        Check(TEXT("fresh_readback.selectedArmour"),SameArmour);
        const auto& PriorOptions=Expected->GetArrayField(TEXT("tacticalOptions"));const auto& FreshOptions=Actual->GetArrayField(TEXT("tacticalOptions"));
        bool SameOptions=PriorOptions.Num()==FreshOptions.Num();for(int32 Index=0;SameOptions&&Index<PriorOptions.Num();++Index)SameOptions=PriorOptions[Index]->AsString()==FreshOptions[Index]->AsString();
        Check(TEXT("fresh_readback.tacticalOptions"),SameOptions);Check(TEXT("fresh_continue_has_no_resurrected_battle_or_clock"),ActiveBattle->Phase==EFrontlinePhase::Briefing&&ActiveBattle->Elapsed==0&&ActiveBattle->Units.IsEmpty()&&ActiveBattle->Strikes.IsEmpty());
        FFrontlineCombatRecordV65 PriorCombat,FreshCombat;
        const bool CombatRead=FrontlineCombatRecordV65::ReadArchive(*Expected->GetObjectField(TEXT("fullCombatRecord")),PriorCombat,false)
            &&FrontlineCombatRecordV65::ReadArchive(*Actual->GetObjectField(TEXT("fullCombatRecord")),FreshCombat,false);
        Check(TEXT("fresh_readback.full_combat_record_decodes"),CombatRead);
        const auto EqualTally=[](const FFrontlineKillTallyV65& A,const FFrontlineKillTallyV65& Z){return A.Infantry==Z.Infantry&&A.Armour==Z.Armour;};
        bool SameCombat=CombatRead&&EqualTally(PriorCombat.Total,FreshCombat.Total)&&EqualTally(PriorCombat.OtherSources,FreshCombat.OtherSources)
            &&PriorCombat.InfantryLost==FreshCombat.InfantryLost&&PriorCombat.ArmourLost==FreshCombat.ArmourLost
            &&PriorCombat.BattlesRecorded==FreshCombat.BattlesRecorded&&PriorCombat.bIncludesEarlierUnrecordedBattles==FreshCombat.bIncludesEarlierUnrecordedBattles;
        for(int32 Index=0;Index<11;++Index)SameCombat&=EqualTally(PriorCombat.InfantrySources[Index],FreshCombat.InfantrySources[Index]);
        for(int32 Index=0;Index<10;++Index)SameCombat&=EqualTally(PriorCombat.ArmourSources[Index],FreshCombat.ArmourSources[Index])&&EqualTally(PriorCombat.SupportSources[Index],FreshCombat.SupportSources[Index]);
        SameCombat&=PriorCombat.NationalArmourSourcesV82.Num()==FreshCombat.NationalArmourSourcesV82.Num();
        for(const auto& Source:PriorCombat.NationalArmourSourcesV82)
        {const auto* Fresh=FreshCombat.NationalArmourSourcesV82.Find(Source.Key);SameCombat&=Fresh&&EqualTally(Source.Value,*Fresh);}
        Check(TEXT("fresh_readback.full_combat_sources_and_losses_unchanged"),SameCombat);
        Check(TEXT("fresh_readback.exact_one_battle_and_summary_totals"),CombatRead&&FreshCombat.BattlesRecorded==1
            &&FreshCombat.Total.Infantry==Actual->GetIntegerField(TEXT("infantryKills"))&&FreshCombat.Total.Armour==Actual->GetIntegerField(TEXT("armourKills")));
        AccountAfter=AccountSnapshot();ColdAccountMatches=true;
        for(const TCHAR* Key:{TEXT("catalogCount"),TEXT("earnedCount"),TEXT("completedRouteMask")})ColdAccountMatches&=AccountAfter->GetIntegerField(Key)==ExpectedAccount->GetIntegerField(Key);
        const auto& PriorHonours=ExpectedAccount->GetArrayField(TEXT("records"));const auto& FreshHonours=AccountAfter->GetArrayField(TEXT("records"));
        ColdAccountMatches&=PriorHonours.Num()==FreshHonours.Num()&&FreshHonours.Num()==FrontlineAchievementsV76::Count;
        for(int32 Index=0;Index<FMath::Min(PriorHonours.Num(),FreshHonours.Num());++Index)
        {
            const auto Prior=PriorHonours[Index]->AsObject(),Fresh=FreshHonours[Index]->AsObject();
            ColdAccountMatches&=Prior.IsValid()&&Fresh.IsValid();if(!Prior.IsValid()||!Fresh.IsValid())continue;
            ColdAccountMatches&=Prior->GetIntegerField(TEXT("index"))==Fresh->GetIntegerField(TEXT("index"))&&Prior->GetBoolField(TEXT("earned"))==Fresh->GetBoolField(TEXT("earned"));
            for(const TCHAR* Key:{TEXT("id"),TEXT("statusEN"),TEXT("statusZH")})ColdAccountMatches&=Prior->GetStringField(Key)==Fresh->GetStringField(Key);
        }
        Check(TEXT("fresh_readback.account_earned_ids_dates_status_and_route_mask"),ColdAccountMatches);
        if(PlayEarnedNextV87)AccountAtColdContinueV87=AccountAfter;
        return FirstFailure.IsEmpty();
    }
    bool TickNormalOrdinaryV87(float Delta)
    {
        auto* PersistenceBattleV87=Battle();if(!PersistenceBattleV87)return FBase::Tick(Delta);
        const double PersistenceNowV87=FPlatformTime::Seconds();
        if(Step>=EStep::Cover&&Step<=EStep::Reframed)return FBase::Tick(Delta);
        if(OrdinaryReturnStepV87)return TickOrdinaryReturnV87(PersistenceNowV87);
        const auto PersistenceViewV87=WarfareFRRUWorldV82::View(PersistenceBattleV87);
        if(FastPlayRequestedV87&&!FastPlayHudClickAppliedV87&&Step>=EStep::Recruit
            &&PersistenceBattleV87->Phase==EFrontlinePhase::Fighting&&!PersistenceViewV87.StoryPending
            &&!UGameplayStatics::IsGamePaused(PersistenceBattleV87))
        {
            // Exercise the same 1.5x HUD control available to the player.
            // The game retains all host clock, movement and outcome authority.
            auto* PC=Controller.Get();auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
            if(!HUD)return Fail(TEXT("normal_fast_play_actual_hud"),TEXT("The loaded battle HUD is unavailable."));
            const float Before=PersistenceBattleV87->BattleSpeed;
            HUD->NotifyHitBoxClick(TEXT("BattleRate2"));
            FastPlayActualSpeedV87=PersistenceBattleV87->BattleSpeed;
            FastPlayClickedAtHostV87=PersistenceBattleV87->Elapsed;
            FastPlayHudClickAppliedV87=FMath::IsNearlyEqual(FastPlayActualSpeedV87,1.5f);
            Event(TEXT("normal_public_hud_fast_play_1_5x"),FastPlayHudClickAppliedV87);
            auto Evidence=Commands.Last()->AsObject();Evidence->SetStringField(TEXT("actualHitBox"),TEXT("BattleRate2"));
            Evidence->SetNumberField(TEXT("battleSpeedBefore"),Before);Evidence->SetNumberField(TEXT("battleSpeedAfter"),FastPlayActualSpeedV87);
            Check(TEXT("normal_fast_play_public_hud_applies_one_and_half_speed"),FastPlayHudClickAppliedV87);
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
        }
        ObserveAchievementScope(PersistenceViewV87);
        if(!PersistenceViewV87.StageId.IsEmpty()&&PersistenceViewV87.StageId!=LastStage)
        {
            if(!LastStage.IsEmpty())CompletedStagesWitnessed.AddUnique(LastStage);
            LastStage=PersistenceViewV87.StageId;StagesSeen.AddUnique(LastStage);Event(TEXT("actual_stage.")+LastStage,true);
        }
        if(PersistenceBattleV87->Phase!=EFrontlinePhase::Victory)return FBase::Tick(Delta);
        if(!WarfareFRRUWorldV82::OutcomePersisted(PersistenceBattleV87)||PersistenceBattleV87->IsBattleOutroActive()
            ||!FrontlineAchievementsV76::FinalizeBattleKillsV8217(PersistenceBattleV87))return true;
        if(!VictoryChecked)
        {
            VictoryChecked=true;VictoryObserved=PersistenceViewV87.Finished&&PersistenceViewV87.Success;
            OutcomeCommitWitnessedV87=VictoryObserved&&WarfareFRRUWorldV82::OutcomePersisted(PersistenceBattleV87);
            OutcomeRecordBeforeReturnV87=CampaignRecord(PersistenceBattleV87);
            NativeObjectiveBeforeReturnV87=WarfareFRRUWorldV82::ObjectiveDiagnosis(PersistenceBattleV87);
            Check(TEXT("normal_real_objective_result_was_persisted"),OutcomeCommitWitnessedV87);
            FFRRUCampaignSummaryV82 PersistenceWonRecordV87;
            const bool PersistenceRecordFoundV87=WarfareFRRUCampaignV82::CurrentRecord(PersistenceBattleV87,PersistenceWonRecordV87);
            const FString PersistenceNextMissionV87=PlayEarnedNextV87?TEXT("fr.douaumont"):
                WantedNation==EFRRUNationV81::French?TEXT("fr.vaux"):TEXT("ru.przemysl");
            const int32 ExpectedWinsV87=PlayEarnedNextV87?Expected->GetIntegerField(TEXT("victories"))+1:1;
            const int32 ExpectedCompletedV87=PlayEarnedNextV87?Expected->GetIntegerField(TEXT("completed"))+1:1;
            const int32 ExpectedResearchV87=PlayEarnedNextV87?Expected->GetIntegerField(TEXT("researchPoints"))+2:4;
            Check(TEXT("normal_committed_guid_nation_difficulty"),PersistenceRecordFoundV87&&PersistenceWonRecordV87.Id==ActualCampaign
                &&PersistenceWonRecordV87.PlayerNation==WantedNation&&PersistenceWonRecordV87.Difficulty==WantedDifficulty);
            Check(PlayEarnedNextV87?TEXT("earned_next_committed_real_victory_and_reward"):TEXT("normal_committed_real_first_victory_and_reward"),PersistenceRecordFoundV87&&PersistenceWonRecordV87.Victories==ExpectedWinsV87
                &&PersistenceWonRecordV87.Defeats==0&&PersistenceWonRecordV87.Completed==ExpectedCompletedV87&&PersistenceWonRecordV87.ResearchPoints==ExpectedResearchV87
                &&PersistenceWonRecordV87.MissionId==PersistenceNextMissionV87);
            Check(TEXT("normal_committed_full_record_exact_once_and_summary_totals"),PersistenceRecordFoundV87
                &&PersistenceWonRecordV87.CombatRecord.BattlesRecorded==ExpectedCompletedV87&&PersistenceWonRecordV87.CombatRecord.Total.Infantry==PersistenceWonRecordV87.InfantryKills
                &&PersistenceWonRecordV87.CombatRecord.Total.Armour==PersistenceWonRecordV87.ArmourKills);
            const auto* ActualMissionV87=wfexp::tactical::FindMission(wfexpue::Narrow(FString(Scenario->Mission)));
            Check(PlayEarnedNextV87?TEXT("earned_next_all_actual_objective_stages_observed"):TEXT("normal_all_three_objective_stages_observed"),
                PlayEarnedNextV87?ActualMissionV87&&StagesSeen.Num()==int32(ActualMissionV87->stages.size()):StagesSeen.Num()==3);
            Check(TEXT("normal_public_ordinary_advance_and_hold_windows"),OrdinaryAdvanceSeenV827&&OrdinaryHoldPreservedV827&&OrdinaryFirstCompletedV827);
            Check(TEXT("normal_campaign_retains_earned_honour_eligibility"),WarfareFRRUCampaignV82::HonourEligible(PersistenceBattleV87));
            AccountAfter=AccountSnapshot();bool MatchingNationalAwardsV87=true;
            for(int32 Index=0;Index<FrontlineAchievementsV76::Count;++Index)if(FrontlineAchievementsV76::Earned(Index))
            {const auto AwardNationV87=FrontlineAchievementsV76::Definition(Index).Nation;MatchingNationalAwardsV87&=AwardNationV87==EFRRUNationV81::Invalid||AwardNationV87==WantedNation;}
            Check(TEXT("normal_real_first_win_awards_only_matching_national_honours"),MatchingNationalAwardsV87);
        }
        if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
        if(!CaptureRealNationalHonours())return true;
        OrdinaryReturnStepV87=1;return TickOrdinaryReturnV87(PersistenceNowV87);
    }
    bool Tick(float Delta)
    {
        auto* ActiveBattle=Battle();auto* ActiveController=Controller.Get();if(!ActiveBattle||!ActiveController)return FBase::Tick(Delta);
        if(Step==EStep::FinalCapture)return FBase::Tick(Delta);
        const double Now=FPlatformTime::Seconds();WallFrameMs=double(Delta)*1000;
        if(Now-Started>DeadlineSeconds||(WallDeadlineUtc.GetTicks()>0&&FDateTime::UtcNow()>=WallDeadlineUtc))
        {Sample(true);return Fail(TEXT("bounded_normal_campaign_deadline"),TEXT("No forced victory; retain real orders, stage, soldiers, repository and terminal screenshot."));}
        if((!ContinueOnly||PlayEarnedNextV87&&ContinueReadbackChecked)&&Scenario&&Scenario->Probe==FRRUNaturalReviewV82::EProbe::Ordinary)return TickNormalOrdinaryV87(Delta);
        if(ContinueOnly&&ContinueReadbackChecked&&!PlayEarnedNextV87)
        {
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            if(!CaptureRealNationalHonours())return true;Finish();return !Finished;
        }
        if(Step==EStep::Setup)
        {
            AccountBefore=AccountSnapshot();
            if(ContinueOnly)
            {
                const bool LoadedV87=CheckReadback();ContinueReadbackChecked=true;
                if(PlayEarnedNextV87&&!LoadedV87&&FirstFailure.IsEmpty())return Fail(TEXT("earned_next_public_continue_readback"),TEXT("The existing cold Continue check did not complete."));
                if(!LoadedV87||!FirstFailure.IsEmpty()){Finish();return !Finished;}
                if(PlayEarnedNextV87)
                {
                    Check(TEXT("earned_next_selected_real_vaux_from_same_campaign"),ActualCampaign.IsValid()
                        &&WarfareFRRUCampaignV82::CampaignId(ActiveBattle)==ActualCampaign
                        &&WarfareFRRUCampaignV82::MissionId(ActiveBattle)==TEXT("fr.vaux")
                        &&Expected->GetStringField(TEXT("mission"))==TEXT("fr.vaux")&&Expected->GetIntegerField(TEXT("completed"))==1);
                    const FRRUNaturalReviewV82::FScenario* ActualCaseV87=nullptr;
                    for(const auto& CandidateV87:FRRUNaturalReviewV82::Scenarios)
                        if(CandidateV87.WholeOperation&&FString(CandidateV87.Mission)==TEXT("fr.vaux")&&CandidateV87.Probe==FRRUNaturalReviewV82::EProbe::Ordinary){ActualCaseV87=&CandidateV87;break;}
                    Check(TEXT("earned_next_existing_ordinary_driver_exists"),ActualCaseV87!=nullptr);
                    if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
                    Scenario=ActualCaseV87;OrdinaryOpeningResearchV87=WarfareFRRUCampaignV82::ResearchPoints(ActiveBattle);
                    NormalCampaignCompletionV87=false;auto& EarnedScreenV87=ScreenState(ActiveController);
                    EarnedScreenV87.Dossier=0;EarnedScreenV87.Menu=false;SetStep(EStep::Cover);
                }
                return !Finished;
            }
            Check(TEXT("normal_account_before_real_first_mission_is_empty"),AccountBefore->GetIntegerField(TEXT("earnedCount"))==0&&AccountBefore->GetIntegerField(TEXT("completedRouteMask"))==0);
            Check(TEXT("fresh_normal_repository_empty"),WarfareFRRUCampaignV82::Archives(ActiveBattle).IsEmpty());if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            StartedNormally=WarfareFRRUCampaignV82::Start(ActiveBattle,WantedNation,WantedDifficulty);Event(TEXT("normal_public_start"),StartedNormally);
            Check(TEXT("normal_public_start_creates_first_dated_mission"),StartedNormally&&WarfareFRRUCampaignV82::MissionId(ActiveBattle)==Scenario->Mission,WarfareFRRUCampaignV82::Status(ActiveBattle));
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}ActualCampaign=WarfareFRRUCampaignV82::CampaignId(ActiveBattle);
            Check(TEXT("normal_guid_independent_of_legacy_archive"),ActualCampaign.IsValid()&&ActiveBattle->GetCurrentCampaign()==nullptr);
            const auto DefaultLoadout=FrontlineCampaignV32::GetLoadout(ActiveBattle);
            const uint32 Required=WarfareFRRUCampaignV82::RequiredRoleMask(ActiveBattle);
            Check(TEXT("normal_default_roster_contains_every_required_stage_role"),(DefaultLoadout.SelectedRoles&Required)==Required);
            Check(TEXT("normal_default_roster_preserves_five_slots_and_date_eligibility"),FrontlineCampaignV32::BitCount(DefaultLoadout.SelectedRoles)<=DefaultLoadout.RoleCapacity&&(DefaultLoadout.SelectedRoles&~DefaultLoadout.EligibleRoles)==0);
            Check(TEXT("normal_start_does_not_manually_grant_a_pioneer_or_rewrite_selection"),WarfareFRRUCampaignV82::RoleSelection(ActiveBattle)==0);
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            OrdinaryOpeningResearchV87=WarfareFRRUCampaignV82::ResearchPoints(ActiveBattle);
            Scenario=&OrdinaryCasesV87[WantedNation==EFRRUNationV81::French?0:1];
            // Only this already validated public fresh Start enables completion
            // scheduling. Cold/recovered Continue and standalone natural cases
            // keep the base scheduler's default false state.
            NormalCampaignCompletionV87=StartedNormally&&!ContinueOnly&&!RecoverEarnedContinueV87&&ActualCampaign.IsValid()
                &&WarfareFRRUCampaignV82::CampaignId(ActiveBattle)==ActualCampaign;
            auto& Screen=ScreenState(ActiveController);Screen.Dossier=0;Screen.Menu=false;SetStep(EStep::Cover);return true;
        }
        if(Step>=EStep::Cover&&Step<=EStep::Reframed)return FBase::Tick(Delta);
        if(!ScreenState(ActiveController).PreparationError.IsEmpty())return Fail(TEXT("actual_normal_loading"),ScreenState(ActiveController).PreparationError);
        if(!ActiveBattle->AssetFailure.IsEmpty())return Fail(TEXT("actual_normal_asset_binding"),ActiveBattle->AssetFailure);
        const auto PendingStory=WarfareFRRUWorldV82::View(ActiveBattle);
        if(PendingStory.StoryPending&&PendingStory.ChoiceIds.Contains(TEXT("steady")))
        {
            const auto Result=WarfareFRRUWorldV82::ReviewStoryChoiceDwell(ActiveBattle,PendingStory.StoryId,TEXT("steady"));
            if(!Result.IsSet())return true;
            Check(TEXT("normal_story_dwell_freezes_host.")+PendingStory.StoryId,Result.GetValue());
            Event(TEXT("normal_real_story_choice"),Result.GetValue());
            if(!Result.GetValue())return Fail(TEXT("normal_story_pause_and_choice"),TEXT("Real story dwell or normal choice failed; inspect STORY_DWELL_REVIEW."));
        }
        if(Now>=NextSample){Sample();NextSample=Now+.5;}
        if(Step==EStep::Recruit)
        {
            if(ActiveBattle->Phase!=EFrontlinePhase::Fighting)return Fail(TEXT("normal_battle_started"),ActiveBattle->EndReason);
            const EFrontlineRole PointRole=WantedNation==EFRRUNationV81::French?EFrontlineRole::Rifle:EFrontlineRole::Lewis;
            if(ActiveBattle->RecruitBlockReason(PointRole,0).IsEmpty())
            {Lead=Recruit(PointRole,0);if(Lead==INDEX_NONE)return Fail(TEXT("normal_real_point_section_recruit"),ActiveBattle->LastMessage);++LeadSectionsOrdered;EntryOrderedAt=ActiveBattle->Elapsed;SetStep(EStep::Entry);}
            return true;
        }
        if(Step==EStep::Entry)
        {
            if(!At(Lead,TEXT("home")))
            {if(ActiveBattle->Elapsed-EntryOrderedAt>=45){Sample(true);return Fail(TEXT("normal_recruits_physically_reach_home"),TEXT("45 host seconds; no radius, capsule or arrival criteria relaxed."));}return true;}
            Check(TEXT("all_real_living_recruits_stationary_at_home"),true);const auto Probe=WarfareFRRUPresentationV82::ProbeUnit(Member(Lead));
            Check(TEXT("normal_real_body_weapon_binding"),Probe->GetBoolField(TEXT("actualBinding")));
            ActiveBattle->SetBattleSpeed(1.5f);Check(TEXT("production_supported_one_and_half_speed"),FMath::IsNearlyEqual(ActiveBattle->BattleSpeed,1.5f));
            Event(TEXT("normal_hud_supported_speed_1_5x"),FMath::IsNearlyEqual(ActiveBattle->BattleSpeed,1.5f));SetStep(EStep::Combat);return true;
        }
        if(Step!=EStep::Combat)return Fail(TEXT("normal_scheduler_known_state"),TEXT("Unexpected scheduler state."));
        const auto View=WarfareFRRUWorldV82::View(ActiveBattle);
        ObserveAchievementScope(View);
        ObserveRifleReadiness();
        if(ActiveBattle->Phase==EFrontlinePhase::Victory)
        {
            if(!WarfareFRRUWorldV82::OutcomePersisted(ActiveBattle))return true;
            if(!VictoryChecked)
            {
                VictoryChecked=true;
                VictoryObserved=View.Finished&&View.Success;Check(TEXT("normal_real_objective_result_was_persisted"),VictoryObserved);
                FFRRUCampaignSummaryV82 Record;const bool Found=WarfareFRRUCampaignV82::CurrentRecord(ActiveBattle,Record);
                const FString Next=WantedNation==EFRRUNationV81::French?TEXT("fr.vaux"):TEXT("ru.przemysl");
                Check(TEXT("normal_committed_guid_nation_difficulty"),Found&&Record.Id==ActualCampaign&&Record.PlayerNation==WantedNation&&Record.Difficulty==WantedDifficulty);
                Check(TEXT("normal_committed_real_first_victory_and_reward"),Found&&Record.Victories==1&&Record.Defeats==0&&Record.Completed==1&&Record.ResearchPoints==4&&Record.MissionId==Next);
                Check(TEXT("normal_committed_full_record_exact_once_and_summary_totals"),Found&&Record.CombatRecord.BattlesRecorded==1
                    &&Record.CombatRecord.Total.Infantry==Record.InfantryKills&&Record.CombatRecord.Total.Armour==Record.ArmourKills);
                Check(TEXT("normal_all_three_objective_stages_observed"),StagesSeen.Num()==3);
                AccountAfter=AccountSnapshot();bool CorrectNationalAwards=true;
                for(int32 Index=0;Index<FrontlineAchievementsV76::Count;++Index)if(FrontlineAchievementsV76::Earned(Index))
                {const auto Nation=FrontlineAchievementsV76::Definition(Index).Nation;CorrectNationalAwards&=Nation==EFRRUNationV81::Invalid||Nation==WantedNation;}
                Check(TEXT("normal_real_first_win_awards_only_matching_national_honours"),CorrectNationalAwards);
                if(RearPioneerRecruited)Check(TEXT("normal_rear_engineer_reserve_has_real_home_arrival_and_public_hold"),RearPioneerArrived&&RearPioneerHeld);
            }
            if(!FirstFailure.IsEmpty()){Finish();return !Finished;}
            if(!CaptureRealNationalHonours())return true;
            Finish();return !Finished;
        }
        if(ActiveBattle->Phase!=EFrontlinePhase::Fighting)return Fail(TEXT("normal_gameplay_not_defeated"),ActiveBattle->EndReason);
        TickFrenchFireOrders();
        TickVisibleDirectSupport();
        TickRussianRally();
        TickPointSmoke();
        if(Now<NextOrders)return true;NextOrders=Now+.5;
        if(View.StageId!=LastStage){if(!LastStage.IsEmpty())CompletedStagesWitnessed.AddUnique(LastStage);LastStage=View.StageId;StagesSeen.AddUnique(LastStage);Event(TEXT("actual_stage.")+LastStage,true);}
        if(View.StoryPending&&View.ChoiceIds.Contains(TEXT("steady")))Event(TEXT("normal_real_story_choice"),WarfareFRRUWorldV82::ChooseStory(ActiveBattle,View.StoryId,TEXT("steady")));
        const auto Dead=[&](int32 Id){const auto* Group=ActiveBattle->FindSquad(Id);return Id==INDEX_NONE||!Group||Group->LivingCount()==0;};
        const bool FirstStage=View.StageId.EndsWith(TEXT("stage.1"));
        if(WantedNation==EFRRUNationV81::French)
        {
            // Every recruitment restarts the shared queue. The ridge needs its
            // gun and independent rifle flank before a later fire observer;
            // wait for the gun's real cooldown instead of spending it early.
            if(!View.StageId.EndsWith(TEXT("stage.3")))
            {
                const auto* ExistingGunSection=ActiveBattle->FindSquad(FrenchGun);bool HasAliveGunner=false;
                if(ExistingGunSection)for(const auto& Weak:ExistingGunSection->Members)
                    if(const auto* ExistingGunner=Weak.Get();ExistingGunner&&ExistingGunner->IsAlive()&&ExistingGunner->IsMachineGunner())HasAliveGunner=true;
                // Surviving escorts are genuine actors. Withdraw them by a real
                // order before the new gun claims their occupied firing bay.
                if(!Dead(FrenchGun)&&!HasAliveGunner&&FrenchRetiringGun!=FrenchGun&&Stationary(FrenchGun))
                    if(FrenchRoute(TEXT("normal_surviving_gun_escorts_return_to_assembly"),FrenchGun,TEXT("assembly")))FrenchRetiringGun=FrenchGun;
                if((Dead(FrenchGun)||FrenchRetiringGun==FrenchGun)
                    &&ActiveBattle->RecruitBlockReason(EFrontlineRole::Lewis,0).IsEmpty())FrenchGun=Recruit(EFrontlineRole::Lewis,0);
                Second=FrenchGun;
                // The wire spur is behind the actual 46 m covering range. The
                // registered forward bay covers the objective and enemy approach.
                const FString GunPost=FirstStage?TEXT("front_left"):TEXT("objective");
                if(Stationary(FrenchGun)&&!At(FrenchGun,GunPost))
                {
                    // Two ordinary node orders expose the actual next node: the
                    // redesign's direct assembly flank avoids ridge's one bay.
                    const FString NextPost=FirstStage&&At(FrenchGun,TEXT("home"))?TEXT("assembly"):GunPost;
                    FrenchRoute(TEXT("normal_hotchkiss_real_registered_flank_route"),FrenchGun,NextPost);
                    if(FirstStage&&NextPost==TEXT("front_left"))Commands.Last()->AsObject()->SetStringField(TEXT("intendedRegisteredEdge"),TEXT("assembly__front_left.flank"));
                }
            }
            // Dispatch an independent flank at the next real rifle cooldown,
            // before the rear guard takes that cycle. A point stopped by contact
            // cannot complete its whole formation or supply a recovery arrival.
            if(FirstStage&&FrenchGun!=INDEX_NONE&&!FrenchFlankOrdered
                &&ActiveBattle->RecruitBlockReason(EFrontlineRole::Rifle,0).IsEmpty())
            {
                FrenchFlank=Recruit(EFrontlineRole::Rifle,0);
                FrenchFlankOrdered=FrenchFlank!=INDEX_NONE;
                Event(TEXT("normal_first_stage_independent_flank_section"),FrenchFlankOrdered);
            }
            // The successful flank recruitment proves the first gun and rifle
            // have both used normal public recruitment before this observer.
            if((!FirstStage||FrenchFlankOrdered)&&Dead(FireObserver)
                &&ActiveBattle->RecruitBlockReason(EFrontlineRole::Marksman,0).IsEmpty())
                FireObserver=Recruit(EFrontlineRole::Marksman,0);
            Observer=FireObserver;
            const FString ObservationPost=FirstStage?TEXT("relay"):TEXT("ridge");
            if((FirstStage||At(FrenchGun,TEXT("front_left"))||At(FrenchGun,TEXT("objective")))&&Stationary(FireObserver)&&!At(FireObserver,ObservationPost))
            {
                const bool Accepted=FrenchRoute(TEXT("normal_marksman_first_real_observation_post"),FireObserver,ObservationPost);
                FrenchObserverDispatched|=Accepted;
            }
            if(FirstStage&&At(FireObserver,TEXT("relay")))FrenchObserverDispatched=true;
            // There is one persistent rear guard, not two rifle sections waiting
            // in assembly. Replacements still use the original public cooldown.
            if((!FirstStage||At(FireObserver,TEXT("relay")))&&Dead(CoverRifle)
                &&ActiveBattle->RecruitBlockReason(EFrontlineRole::Rifle,0).IsEmpty())CoverRifle=Recruit(EFrontlineRole::Rifle,0);
            if(Stationary(CoverRifle)&&At(CoverRifle,TEXT("home")))
                FrenchRoute(TEXT("normal_single_rear_rifle_real_route"),CoverRifle,TEXT("assembly"));
        }
        if(WantedNation==EFRRUNationV81::Russian)
        {
            if(FirstStage&&Dead(Second)&&ActiveBattle->RecruitBlockReason(EFrontlineRole::Pioneer,0).IsEmpty())Second=Recruit(EFrontlineRole::Pioneer,0);
            if(Dead(CoverRifle)&&ActiveBattle->RecruitBlockReason(EFrontlineRole::Rifle,0).IsEmpty())CoverRifle=Recruit(EFrontlineRole::Rifle,0);
            if(Stationary(CoverRifle)&&At(CoverRifle,TEXT("home")))
            {
                // The public planner owns both authored edges. The review does
                // not impose an extra whole-section stop at the intermediate.
                const bool Accepted=WarfareFRRUWorldV82::MoveSquad(ActiveBattle,CoverRifle,TEXT("relay"));
                Event(TEXT("normal_rifle_public_home_to_relay_via_authored_assembly"),Accepted);
                auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("squad"),CoverRifle);Row->SetStringField(TEXT("target"),TEXT("relay"));
                Row->SetNumberField(TEXT("actualEnteredActiveAlive"),EnteredActiveLiving(CoverRifle));Row->SetBoolField(TEXT("actualWholeFormationStationaryAtHome"),true);
                if(Accepted)RifleRelayOrderSquad=CoverRifle;
            }
            if(RifleRelayOrderSquad==CoverRifle&&CoverRifle!=INDEX_NONE&&RifleRelayArrivalSquad!=CoverRifle
                &&EnteredActiveLiving(CoverRifle)>=4&&Stationary(CoverRifle)&&At(CoverRifle,TEXT("relay")))
            {
                RifleRelayArrivalSquad=CoverRifle;Event(TEXT("normal_rifle_actual_stationary_relay_arrival"),true);
                auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("squad"),CoverRifle);Row->SetStringField(TEXT("actualArrivedNode"),TEXT("relay"));
                Row->SetNumberField(TEXT("actualEnteredActiveAlive"),EnteredActiveLiving(CoverRifle));Row->SetBoolField(TEXT("actualWholeFormationStationaryAtRelay"),true);
            }
            // One intact rear reserve uses the ordinary queue after the actual
            // bridge clearance. The original clearance team and its onward
            // route remain intact; no reserve is counted before field entry.
            const auto* ExistingPoint=ActiveBattle->FindSquad(Lead);
            if(!FirstStage&&WarfareFRRUWorldV82::PassageCleared(ActiveBattle,TEXT("bridge"))&&!RearPioneerRecruited
                &&EnteredActiveLiving(Second)<3&&ExistingPoint&&ExistingPoint->LivingCount()>0
                &&CoverRifle!=INDEX_NONE&&RifleRelayOrderSquad==CoverRifle&&RifleRelayArrivalSquad==CoverRifle
                &&EnteredActiveLiving(CoverRifle)>=4&&Stationary(CoverRifle)&&At(CoverRifle,TEXT("relay"))
                &&ActiveBattle->RecruitBlockReason(EFrontlineRole::Pioneer,0).IsEmpty())
            {
                const float CooldownBefore=ActiveBattle->RecruitCooldown(EFrontlineRole::Pioneer,0);
                RearPioneer=Recruit(EFrontlineRole::Pioneer,0);RearPioneerRecruited=RearPioneer!=INDEX_NONE;
                Event(TEXT("normal_one_rear_engineer_reserve_after_real_bridge_clearance"),RearPioneerRecruited);
                auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("squad"),RearPioneer);Row->SetNumberField(TEXT("originalEngineerSquad"),Second);
                Row->SetNumberField(TEXT("originalEngineerEnteredActiveAlive"),EnteredActiveLiving(Second));Row->SetStringField(TEXT("stage"),View.StageId);
                Row->SetBoolField(TEXT("actualBridgeCleared"),WarfareFRRUWorldV82::PassageCleared(ActiveBattle,TEXT("bridge")));
                Row->SetNumberField(TEXT("actualReadiedRifleSquad"),CoverRifle);Row->SetNumberField(TEXT("actualReadiedRifleEnteredActiveAlive"),EnteredActiveLiving(CoverRifle));
                Row->SetBoolField(TEXT("acceptedRifleRelayOrderAndStationaryArrivalWitnessed"),true);
                Row->SetNumberField(TEXT("normalPioneerCooldownBeforeRecruit"),CooldownBefore);Row->SetNumberField(TEXT("normalPioneerCooldownAfterRecruit"),ActiveBattle->RecruitCooldown(EFrontlineRole::Pioneer,0));
            }
            if(RearPioneerRecruited&&!RearPioneerHeld&&Stationary(RearPioneer)&&At(RearPioneer,TEXT("home")))
            {
                RearPioneerArrived=true;RearPioneerArrivedAlive=EnteredActiveLiving(RearPioneer);
                RearPioneerHeld=WarfareFRRUWorldV82::HoldSquad(ActiveBattle,RearPioneer,true);
                Event(TEXT("normal_rear_engineer_actual_home_arrival_then_public_hold"),RearPioneerHeld);
                auto Row=Commands.Last()->AsObject();Row->SetNumberField(TEXT("squad"),RearPioneer);Row->SetStringField(TEXT("actualArrivedNode"),TEXT("home"));
                Row->SetNumberField(TEXT("actualEnteredActiveAlive"),RearPioneerArrivedAlive);Row->SetBoolField(TEXT("actualWholeFormationStationaryAtHome"),true);
            }
        }
        const auto* LeadGroup=ActiveBattle->FindSquad(Lead);
        if(WantedNation==EFRRUNationV81::French&&FirstStage)
        {
            if(Stationary(FrenchFlank))
            {
                // A section already holding the real recovery post stays there.
                // The production recovery gate evaluates its actual presence;
                // the pilot must not march it away merely to manufacture a new
                // movement receipt. Final traversal still needs a new arrival.
                if(!View.Fallback&&At(FrenchFlank,TEXT("home")))
                    FrenchRoute(TEXT("normal_flank_reinforcement_assembly"),FrenchFlank,TEXT("assembly"));
                else if(!View.Fallback&&At(FrenchFlank,TEXT("assembly")))
                    FrenchRoute(TEXT("normal_flank_reinforcement_forest"),FrenchFlank,TEXT("forest"));
                else if(!View.Fallback&&At(FrenchFlank,TEXT("forest")))
                    FrenchRoute(TEXT("normal_flank_reinforcement_front_right"),FrenchFlank,TEXT("front_right"));
            }
        }
        const EFrontlineRole PointRole=WantedNation==EFRRUNationV81::French?EFrontlineRole::Rifle:EFrontlineRole::Lewis;
        const bool FrenchPointNeedsRelief=WantedNation==EFRRUNationV81::French&&!FirstStage&&!View.StageId.EndsWith(TEXT("stage.3"))
            &&LeadGroup&&LeadGroup->LivingCount()<=FMath::Max(1,WarfareFRRUEquipmentV82::RoleSize(ActiveBattle,PointRole,0)-2);
        if((!LeadGroup||LeadGroup->LivingCount()==0||FrenchPointNeedsRelief)&&LeadSectionsOrdered<3&&ActiveBattle->RecruitBlockReason(PointRole,0).IsEmpty())
        {
            if(WantedNation!=EFRRUNationV81::French)
            {Lead=Recruit(PointRole,0);++LeadSectionsOrdered;Event(TEXT("normal_replace_lost_point_section_from_real_recruitment"),Lead!=INDEX_NONE);}
            else
            {
                const int32 RelievedSection=Lead,LivingBefore=LeadGroup?LeadGroup->LivingCount():0;
                const int32 Replacement=Recruit(PointRole,0);
                Event(FrenchPointNeedsRelief?TEXT("normal_reinforce_point_after_actual_casualties"):TEXT("normal_replace_lost_point_section_from_real_recruitment"),Replacement!=INDEX_NONE);
                Commands.Last()->AsObject()->SetNumberField(TEXT("relievedSquad"),RelievedSection);
                Commands.Last()->AsObject()->SetNumberField(TEXT("relievedLiving"),LivingBefore);
                if(Replacement!=INDEX_NONE){Lead=Replacement;++LeadSectionsOrdered;}
            }
        }

        bool RealCoverReady=false;
        if(WantedNation==EFRRUNationV81::Russian)
        {
            // The wire covers the clearance team. Once clearance is complete,
            // the gun itself makes the new Traverse arrival at the single
            // bridge bay, then covers the rifle's onward attack. Sending a
            // rifle to that bay first would leave no place to deploy its gun.
            if(FirstStage&&Stationary(Lead)&&At(Lead,TEXT("home")))
                Event(TEXT("normal_maxim_real_same_flank_cover_route"),WarfareFRRUWorldV82::MoveSquad(ActiveBattle,Lead,TEXT("wire")));
            if(View.StageId.EndsWith(TEXT("stage.3"))&&Stationary(Lead)&&!At(Lead,TEXT("bridge")))
                Event(TEXT("normal_maxim_real_bridge_cover_route"),WarfareFRRUWorldV82::MoveSquad(ActiveBattle,Lead,TEXT("bridge")));
            if(FirstStage&&Stationary(Second)&&At(Second,TEXT("home")))
                Event(TEXT("normal_pioneer_real_rear_assembly"),WarfareFRRUWorldV82::MoveSquad(ActiveBattle,Second,TEXT("assembly")));
            const auto* GunSection=ActiveBattle->FindSquad(Lead);
            const FString CoverPost=FirstStage?TEXT("wire"):TEXT("bridge");
            if(GunSection&&Stationary(Lead)&&At(Lead,CoverPost))
                for(const auto& Weak:GunSection->Members)if(const auto* Gun=Weak.Get();Gun&&Gun->IsAlive()&&Gun->IsMachineGunner())
                    RealCoverReady|=WarfareFRRUPresentationV82::ProbeUnit(Gun,false)->GetBoolField(TEXT("actualGunSupportReady"));
            if(RealCoverReady&&!CoverObserved)
            {CoverObserved=true;Event(TEXT("normal_actual_maxim_support_ready_at_wire"),true);}
        }
        int32 Worker=WantedNation==EFRRUNationV81::Russian?(FirstStage?Second:View.StageId.EndsWith(TEXT("stage.2"))?Lead:CoverRifle)
            :(FirstStage&&View.FallbackAwaitingArrival?FrenchFlank
                :View.StageId.EndsWith(TEXT("stage.3"))?CoverRifle:Lead);
        // Clearance's actual arrival is never reused as the next Traverse
        // receipt. Vacate the bridge by the ordinary outward physical route.
        if(WantedNation==EFRRUNationV81::Russian&&View.StageId.EndsWith(TEXT("stage.2"))&&Second!=INDEX_NONE&&Stationary(Second)&&At(Second,View.TargetNode))
        {PioneerMovedAfterClear=WarfareFRRUWorldV82::MoveSquad(ActiveBattle,Second,TEXT("objective"));Event(TEXT("normal_clearer_vacates_bridge_by_real_route"),PioneerMovedAfterClear);}
        // Clearance recovery still requires its frozen actual pioneer and
        // exact rail target. This candidate makes no claim
        // that a wire gun provides cover on the opposite rail approach.
        const bool WorkerMayAdvance=WantedNation!=EFRRUNationV81::Russian
            ||(FirstStage?(RealCoverReady&&View.TargetNode==TEXT("bridge"))
                :View.StageId.EndsWith(TEXT("stage.2"))||RealCoverReady);
        if(WorkerMayAdvance&&Worker!=INDEX_NONE&&Stationary(Worker)&&!At(Worker,View.TargetNode))
        {
            Event(TEXT("normal_actual_objective_order"),WarfareFRRUWorldV82::Command(ActiveBattle,View.FallbackAwaitingArrival?EFRRUTacticalCommandV82::PolicyOrder:EFRRUTacticalCommandV82::AdvanceToObjective,{Worker},View.TargetNode));
            const auto Order=Commands.Last()->AsObject();Order->SetStringField(TEXT("target"),View.TargetNode);
            Order->SetNumberField(TEXT("workerSquad"),Worker);
            Order->SetBoolField(TEXT("fallback"),View.Fallback);Order->SetBoolField(TEXT("fallbackAwaitingArrival"),View.FallbackAwaitingArrival);
            Order->SetStringField(TEXT("tacticalCommand"),View.FallbackAwaitingArrival?TEXT("PolicyOrder"):TEXT("AdvanceToObjective"));
            if(!Order->GetBoolField(TEXT("accepted")))Order->SetStringField(TEXT("rejection"),WarfareFRRUWorldV82::View(ActiveBattle).Notice);
        }
        return true;
    }
};
FAutoConsoleCommand Command(TEXT("WF.FRRU.ReviewPersistenceV82"),TEXT("Exact-boxed fresh normal first-mission victory and separate-process Continue."),FConsoleCommandDelegate::CreateLambda([]()
{
    FString User,Mode,Nation,Difficulty=TEXT("Rookie");FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceDifficulty="),Difficulty);
    const bool ValidDifficulty=Difficulty.Equals(TEXT("Rookie"),ESearchCase::IgnoreCase)||Difficulty.Equals(TEXT("Veteran"),ESearchCase::IgnoreCase)||Difficulty.Equals(TEXT("Nightmare"),ESearchCase::IgnoreCase);
    if(!WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(User)
        ||!FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceMode="),Mode)||!FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceNation="),Nation)
        ||(Mode!=TEXT("start")&&Mode!=TEXT("continue"))||(Nation!=TEXT("fr")&&Nation!=TEXT("ru"))||!ValidDifficulty)
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_PLAY_PERSISTENCE_REFUSED exact disposable ManualReview/user/mode/nation required"));return;}
    const FString Repository=FPaths::Combine(User,TEXT("Saved/FRRU/Profiles/frru-tactical-v2.json"));
    if(Mode==TEXT("start")&&(IFileManager::Get().FileExists(*Repository)||IFileManager::Get().FileExists(*(Repository+TEXT(".bak")))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_PLAY_PERSISTENCE_REFUSED normal repository is not fresh"));return;}
    static bool Requested=false;if(Requested)return;
    auto Probe=MakeShared<FScheduler>();Probe->User=User;Probe->Scenario=&Cases[Nation==TEXT("fr")?0:1];Probe->WantedNation=Nation==TEXT("fr")?EFRRUNationV81::French:EFRRUNationV81::Russian;
    Probe->WantedDifficulty=Difficulty.Equals(TEXT("Veteran"),ESearchCase::IgnoreCase)?EFrontlineDifficulty::Veteran
        :Difficulty.Equals(TEXT("Nightmare"),ESearchCase::IgnoreCase)?EFrontlineDifficulty::Survivor:EFrontlineDifficulty::Rookie;
    Probe->RequestedDifficultyLabel=Probe->WantedDifficulty==EFrontlineDifficulty::Veteran?TEXT("Veteran"):Probe->WantedDifficulty==EFrontlineDifficulty::Survivor?TEXT("Nightmare"):TEXT("Rookie");
    Probe->RequestedDifficultyV87=Probe->WantedDifficulty;Probe->RequestedDifficultyLabelV87=Probe->RequestedDifficultyLabel;
    Probe->ContinueOnly=Mode==TEXT("continue");Probe->FixtureSynthetic=false;Probe->ReportRevision=TEXT("v82.frru.play-persistence.2");
    FString RecoverySource;const bool RecoverEarned=FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceRecoverEarnedContinue="),RecoverySource);
    FGuid RecoveryId;
    if(RecoverEarned&&(!Probe->ContinueOnly||Nation!=TEXT("fr")||!FGuid::ParseExact(RecoverySource,EGuidFormats::Digits,RecoveryId)||!RecoveryId.IsValid()))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_EARNED_CONTINUE_RECOVERY_REFUSED French continue and exact prior run ID required"));return;}
    Probe->RecoverEarnedContinueV87=RecoverEarned;Probe->RecoverySourceRunV87=RecoverySource;
    const bool PlayEarnedNextV87=FParse::Param(FCommandLine::Get(),TEXT("FRRUPersistencePlayEarnedNext"));
    FString EarnedSourceV87;FGuid EarnedSourceIdV87;
    if(PlayEarnedNextV87&&(!Probe->ContinueOnly||Nation!=TEXT("fr")||RecoverEarned
        ||!FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceSourceRun="),EarnedSourceV87)
        ||!FGuid::ParseExact(EarnedSourceV87,EGuidFormats::Digits,EarnedSourceIdV87)||!EarnedSourceIdV87.IsValid()
        ||FParse::Param(FCommandLine::Get(),TEXT("FRRUVauxTactics"))||FParse::Param(FCommandLine::Get(),TEXT("FRRUHoldDepartureProbeV87"))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_PLAY_EARNED_NEXT_REFUSED exact French earned-source Continue only"));return;}
    Probe->PlayEarnedNextV87=PlayEarnedNextV87;Probe->EarnedSourceRunV87=EarnedSourceV87;
    Probe->FastPlayRequestedV87=!Probe->ContinueOnly&&FParse::Param(FCommandLine::Get(),TEXT("FRRUPersistenceFastPlay"));
    Probe->ReportName=Probe->ContinueOnly?TEXT("persistence-continue.json"):TEXT("persistence-start.json");
    double Deadline=600;FString DeadlineUtc;
    if(!FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalDeadline="),Deadline)||!FMath::IsFinite(Deadline)||Deadline<60||Deadline>820
        ||!FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalDeadlineUtc="),DeadlineUtc)||!FDateTime::ParseIso8601(*DeadlineUtc,Probe->WallDeadlineUtc))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_PLAY_PERSISTENCE_REFUSED bounded wall deadline required"));return;}
    Probe->DeadlineSeconds=Deadline;
    if(Probe->ContinueOnly)
    {
        FString Prior;TSharedPtr<FJsonObject> First;const bool Read=FFileHelper::LoadFileToString(Prior,*FPaths::Combine(User,TEXT("FRRUReview/persistence-start.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Prior),First)&&First.IsValid();
        double PriorDifficulty=0;const TSharedPtr<FJsonObject>* PriorAccount=nullptr;
        const bool MatchingDifficulty=Read&&First->TryGetNumberField(TEXT("requestedDifficulty"),PriorDifficulty)&&PriorDifficulty==int32(Probe->WantedDifficulty);
        const bool HasAccount=Read&&First->TryGetObjectField(TEXT("accountAfter"),PriorAccount)&&PriorAccount&&PriorAccount->IsValid();
        bool ExactReturnFailure=false;
        if(RecoverEarned&&Read)
        {
            FString SourcePrior;
            const FString SourceReceipt=FPaths::Combine(FPaths::ProjectDir(),TEXT("Saved/FRRU/Persistence"),RecoverySource,TEXT("User/FRRUReview/persistence-start.json"));
            const TArray<TSharedPtr<FJsonValue>>* PriorChecks=nullptr;int32 Failures=0;
            ExactReturnFailure=FFileHelper::LoadFileToString(SourcePrior,*SourceReceipt)&&SourcePrior==Prior&&!First->GetBoolField(TEXT("allPassed"))
                &&First->GetStringField(TEXT("firstFailure")).StartsWith(TEXT("ordinary_public_report_return_to_operations:"))
                &&!First->GetBoolField(TEXT("normalLiveReportReturnCompletedV87"))
                &&!First->GetBoolField(TEXT("simulationClockArtificiallyAdvanced"))&&!First->GetBoolField(TEXT("actorsTeleported"))
                &&First->TryGetArrayField(TEXT("checks"),PriorChecks)&&PriorChecks;
            if(PriorChecks)for(const auto& Value:*PriorChecks)
            {
                const auto Row=Value->AsObject();if(!Row.IsValid()){ExactReturnFailure=false;continue;}
                if(!Row->GetBoolField(TEXT("passed"))){++Failures;ExactReturnFailure&=Row->GetStringField(TEXT("name"))==TEXT("ordinary_public_report_return_to_operations");}
            }
            ExactReturnFailure&=Failures==1;
        }
        const bool ReturnProof=Read&&(RecoverEarned?ExactReturnFailure:
            First->GetBoolField(TEXT("allPassed"))&&First->GetBoolField(TEXT("normalLiveReportReturnCompletedV87")));
        if(!Read||!MatchingDifficulty||!HasAccount||First->GetStringField(TEXT("revision"))!=Probe->ReportRevision||!ReturnProof||First->GetBoolField(TEXT("progressionFixtureSynthetic"))
            ||!First->GetBoolField(TEXT("normalPublicStart"))||!First->GetBoolField(TEXT("normalGameplayVictory"))||!First->GetBoolField(TEXT("actualSubmissionPersisted"))
            ||!First->GetBoolField(TEXT("actualOutcomeCommitWitnessedBeforeReturnV87"))
            ||First->GetNumberField(TEXT("requestedNation"))!=int32(Probe->WantedNation))
        {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_PLAY_PERSISTENCE_REFUSED no passed actual normal first outcome receipt"));return;}
        if(PlayEarnedNextV87)
        {
            const FString SourceRootV87=FPaths::Combine(FPaths::ProjectDir(),TEXT("Saved/FRRU/Persistence"),EarnedSourceV87);
            FString ActualPriorV87,ActualSummaryV87,ActualContinueV87;TSharedPtr<FJsonObject> SourceSummaryV87,SourceContinueV87;
            const bool SourceValidV87=FFileHelper::LoadFileToString(ActualPriorV87,*FPaths::Combine(SourceRootV87,TEXT("User/FRRUReview/persistence-start.json")))&&ActualPriorV87==Prior
                &&FFileHelper::LoadFileToString(ActualSummaryV87,*FPaths::Combine(SourceRootV87,TEXT("summary.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ActualSummaryV87),SourceSummaryV87)&&SourceSummaryV87.IsValid()
                &&FFileHelper::LoadFileToString(ActualContinueV87,*FPaths::Combine(SourceRootV87,TEXT("User/FRRUReview/persistence-continue.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ActualContinueV87),SourceContinueV87)&&SourceContinueV87.IsValid()
                &&SourceSummaryV87->GetBoolField(TEXT("passed"))&&SourceSummaryV87->GetBoolField(TEXT("player_data_unchanged"))
                &&SourceContinueV87->GetBoolField(TEXT("allPassed"))&&SourceContinueV87->GetBoolField(TEXT("freshProcessContinue"))
                &&SourceContinueV87->GetBoolField(TEXT("accountColdContinueMatches"))
                &&SourceContinueV87->GetStringField(TEXT("actualCampaignGuid"))==First->GetStringField(TEXT("actualCampaignGuid"));
            if(!SourceValidV87){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_PLAY_EARNED_NEXT_REFUSED source lacks passed real start/live return/cold Continue"));return;}
        }
        Probe->Expected=First->GetObjectField(TEXT("actualSavedRecord"));
        Probe->ExpectedAccount=*PriorAccount;
    }
    Requested=true;const TWeakPtr<FScheduler> WeakProbe=Probe;
    Probe->ReportDetail=[WeakProbe](FJsonObject& Report){if(auto Actual=WeakProbe.Pin())Actual->FillDetail(Report);};
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float Delta)
    {
        if(!Probe->Controller.IsValid())
        {
            if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World();World&&World->WorldType==EWorldType::Game)
                if(auto* Controller=Cast<AWarfareFrontlineController>(World->GetFirstPlayerController());Controller&&Controller->HasActorBegunPlay())
                    if(auto* Battle=FrontlineFor(Controller);Battle&&Battle->HasActorBegunPlay()&&Battle->Phase==EFrontlinePhase::Briefing)Probe->Controller=Controller;
            if(!Probe->Controller.IsValid()){if(FPlatformTime::Seconds()-Probe->Started>45)return Probe->Fail(TEXT("actual_normal_beginplay_context"),TEXT("No actual begun-play controller within 45 wall seconds."));return true;}
        }
        return Probe->Tick(Delta);
    }));
}));
}
#endif
