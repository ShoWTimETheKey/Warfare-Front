// Included only in the existing friend access struct, and only in Editor.
// Controlled positions/quiet target weapons and two explicit timer fixtures
// expose rare boundaries. Combat runs through the real subsystem on host time.
struct FGameplayReviewV87
{
    FString User,Failure;double Started=FPlatformTime::Seconds();int32 Stage=0;
    TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    TWeakObjectPtr<AFrontlineTank> Gun,Target;
    TArray<TSharedPtr<FJsonValue>> Checks,Samples;
    float StageAt=0,InitialAngle=0,TravelAt=0,SmokeAt=0,WreckAt=0,FirstDispatchReadyAt=0;int32 ShotsAt=0;
    bool SawAcquiring=false,SawStop=false,FirstHostTickProven=false;FVector PairAnchor=FVector::ZeroVector;
    FGuid OpeningEncounter;float OpeningLong=0,OpeningShort=0,BaselineLong=0,BaselineShort=0,TrainOpening=0;
    int32 OpeningQueries=0;bool SawShortOpeningReady=false;
    const bool AllocationConsumersOnly=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87AllocationConsumersOnly"));
    TSharedPtr<FrontlineAchievementsV76::FNationalParityFixtureV87> AllocationAccount;
    FGuid AllocationSourceCampaign,AllocationEncounter;FString AllocationSourceMission;
    int32 AllocationSourcePoints=0,AllocationCase=0,AllocationStage=0,AllocationPioneers=0,AllocationPioneerId=INDEX_NONE,AllocationSmokeZones=0;
    float AllocationReady[AWarfareFrontlineGameMode::RoleCount]={},AllocationSmokeBaseline=0,AllocationSecondSmokeReady=0,AllocationAt=0;
    TArray<FString> AllocationModels;TMap<FString,float> AllocationModelReady;TSet<int32> AllocationInitialPioneerIds;

    bool Check(const TCHAR* Name,bool Good,const FString& Detail=FString())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Good);
        Row->SetStringField(TEXT("detail"),Detail);Checks.Add(MakeShared<FJsonValueObject>(Row));
        if(!Good&&Failure.IsEmpty())Failure=Name;return Good;
    }
    bool Finish()
    {
        if(AllocationConsumersOnly)
        {
            if(auto* B=Battle.Get();B&&WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))WarfareFRRUCampaignV82::LeaveArchiveReplayV87(B);
            if(auto* B=Battle.Get();B&&AllocationSourceCampaign.IsValid())
                Check(TEXT("allocation_consumers_restore_original_volatile_campaign"),B->Phase==EFrontlinePhase::Briefing
                    &&WarfareFRRUCampaignV82::CampaignId(B)==AllocationSourceCampaign&&WarfareFRRUCampaignV82::MissionId(B)==AllocationSourceMission
                    &&WarfareFRRUCampaignV82::ResearchPoints(B)==AllocationSourcePoints);
            AllocationAccount.Reset();
        }
        auto Out=MakeShared<FJsonObject>();Out->SetStringField(TEXT("revision"),TEXT("v87.frru.armour.gameplay.1"));
        Out->SetBoolField(TEXT("success"),Failure.IsEmpty());Out->SetStringField(TEXT("failure"),Failure);
        Out->SetBoolField(TEXT("realRecruitAndSubsystemSimulation"),true);Out->SetBoolField(TEXT("hostClockRewritten"),false);
        Out->SetBoolField(TEXT("controlledPlacementQuietWeaponsAndTargetTrackDamage"),!AllocationConsumersOnly);
        Out->SetBoolField(TEXT("controlledEnemyActorPoseRebase"),!AllocationConsumersOnly);Out->SetBoolField(TEXT("naturalTravelToFixturePairNotTested"),true);
        Out->SetBoolField(TEXT("controlledExpiredCooldownAndWreckAgeFixtures"),!AllocationConsumersOnly);
        Out->SetBoolField(TEXT("allocationConsumersOnly"),AllocationConsumersOnly);Out->SetNumberField(TEXT("allocationConsumerCasesCompleted"),AllocationCase);
        if(AllocationConsumersOnly)Out->SetStringField(TEXT("scope"),TEXT("Six French/Russian late-operation Archive attachment consumer fixtures. Synthetic isolated account qualification; public Archive Begin/Leave, selection, attachment purchase, real PrepareScene/BeginBattle, public story choices and ordinary host ticks. Actual pioneer spawn without infantry queue charge, one smoke reserve after natural initial preparation, and opening armour release followed by actual public dispatch. No debug cooldown grants, host clock rewrite, natural campaign, battle balance, art or Shipping acceptance."));
        Out->SetBoolField(TEXT("naturalCampaignAccepted"),false);Out->SetBoolField(TEXT("artAccepted"),false);Out->SetBoolField(TEXT("shippingAccepted"),false);
        Out->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-Started);Out->SetArrayField(TEXT("checks"),Checks);Out->SetArrayField(TEXT("samples"),Samples);
        const FString Path=FPaths::Combine(User,TEXT("FRRUReview/armour-gameplay-v87.json"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
        FString Json;const bool Written=FJsonSerializer::Serialize(Out,TJsonWriterFactory<>::Create(&Json))&&FFileHelper::SaveStringToFile(Json,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ARMOUR_GAMEPLAY_FINISHED success=%d failure=%s receipt=%s"),Failure.IsEmpty(),*Failure,*Path);
        FPlatformMisc::RequestExitWithStatus(false,Written&&Failure.IsEmpty()?0:3);return false;
    }
    bool AllocationCheck(const TCHAR* Name,bool Good,const FString& Detail=FString())
    {return Check(*FString::Printf(TEXT("allocation_%s_kind_%d.%s"),AllocationCase<3?TEXT("fr"):TEXT("ru"),AllocationCase%3,Name),Good,Detail);}
    int32 PlayerPioneers(AWarfareFrontlineGameMode* B)const
    {int32 Count=0;for(const auto& S:B->Squads)if(S.Team==0&&S.Role==EFrontlineRole::Pioneer)++Count;return Count;}
    int32 PlayerSmokeZones(AWarfareFrontlineGameMode* B)const
    {int32 Count=0;for(const auto& Z:B->SmokeZones)if(Z.Team==0)++Count;return Count;}
    bool AllocationQueueUnchanged(AWarfareFrontlineGameMode* B)const
    {for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)if(B->RecruitReadyAt[0][I]!=AllocationReady[I])return false;return true;}
    void AllocationSample(AWarfareFrontlineGameMode* B,const TCHAR* Event)
    {
        using namespace WarfareFRRUArmourV82;
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("event"),Event);
        Row->SetStringField(TEXT("nation"),AllocationCase<3?TEXT("fr"):TEXT("ru"));Row->SetNumberField(TEXT("kind"),AllocationCase%3);
        Row->SetStringField(TEXT("encounter"),AllocationEncounter.ToString());Row->SetNumberField(TEXT("hostElapsed"),B->Elapsed);
        Row->SetNumberField(TEXT("playerPioneerSections"),PlayerPioneers(B));Row->SetNumberField(TEXT("playerSmokeZones"),PlayerSmokeZones(B));Row->SetNumberField(TEXT("smokeReadyAt"),B->SmokeReadyAt[0]);
        TArray<TSharedPtr<FJsonValue>> Queue;for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)Queue.Add(MakeShared<FJsonValueNumber>(B->RecruitReadyAt[0][I]));Row->SetArrayField(TEXT("playerInfantryReadyAt"),Queue);
        auto Models=MakeShared<FJsonObject>();if(const auto* Reserve=Fleets.Find(UFrontlineVehicleSubsystem::Get(B->GetWorld())))
            for(const auto& Id:AllocationModels)if(const float* At=Reserve->ModelReadyAt[0].Find(Id))Models->SetNumberField(Id,*At);
        Row->SetObjectField(TEXT("selectedModelReadyAt"),Models);Samples.Add(MakeShared<FJsonValueObject>(Row));
    }
    bool CompleteAllocationCase(AWarfareFrontlineGameMode* B)
    {
        AllocationSample(B,TEXT("archive_allocation_consumer_complete"));
        WarfareFRRUCampaignV82::LeaveArchiveReplayV87(B);++AllocationCase;AllocationStage=0;return true;
    }
    bool TickAllocationConsumers(AWarfareFrontlineGameMode* B)
    {
        using namespace WarfareFRRUArmourV82;
        FString SafeUser;
        if(!FRRUParityReviewSafetyV87::UserDir(SafeUser)||!FPaths::IsSamePath(SafeUser,User)||!B->IsProgressionVolatile())
        {Check(TEXT("allocation_consumers_exact_disposable_parity_profile"),false);return Finish();}
        if(!AllocationAccount.IsValid())
        {
            const FString Marker=User/TEXT("allocation-consumers-started.json"),AccountPath=User/TEXT("AccountReview/achievements-v2.json"),CampaignPath=User/TEXT("FRRUReview/campaigns-tactical-v2.json");
            const bool Fresh=!IFileManager::Get().FileExists(*Marker)&&!IFileManager::Get().FileExists(*AccountPath)&&!IFileManager::Get().FileExists(*(AccountPath+TEXT(".bak")))
                &&!IFileManager::Get().FileExists(*CampaignPath)&&!IFileManager::Get().FileExists(*(CampaignPath+TEXT(".bak")));
            if(!Check(TEXT("allocation_consumers_fresh_private_fixture"),Fresh&&FFileHelper::SaveStringToFile(TEXT("{\"synthetic_archive_fixture\":true,\"natural_campaign\":false}"),*Marker)))return Finish();
            auto CheckFn=[&](const FString& Name,bool Passed){Check(*Name,Passed);};
            AllocationAccount=FrontlineAchievementsV76::BeginNationalParityFixtureV87(B,CheckFn);
            if(!Check(TEXT("allocation_consumers_synthetic_account_qualification_acquired"),AllocationAccount.IsValid())||!Failure.IsEmpty())return Finish();
            if(!Check(TEXT("allocation_consumers_original_volatile_source_started"),WarfareFRRUCampaignV82::Start(B,EFRRUNationV81::French,EFrontlineDifficulty::Veteran)))return Finish();
            AllocationSourceCampaign=WarfareFRRUCampaignV82::CampaignId(B);AllocationSourceMission=WarfareFRRUCampaignV82::MissionId(B);AllocationSourcePoints=WarfareFRRUCampaignV82::ResearchPoints(B);
        }
        if(AllocationCase==6){Check(TEXT("allocation_consumers_all_six_real_cases_completed"),true);return Finish();}
        const int32 Kind=AllocationCase%3;const auto Nation=AllocationCase<3?EFRRUNationV81::French:EFRRUNationV81::Russian;
        if(AllocationStage==0)
        {
            if(!AllocationCheck(TEXT("public_late_archive_begin"),WarfareFRRUCampaignV82::BeginArchiveReplayV87(B,Nation,9,EFrontlineDifficulty::Veteran)
                &&WarfareFRRUCampaignV82::ArchiveFullArmoryV87(B,0)))return Finish();
            const uint32 Roles=(1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Pioneer))|WarfareFRRUCampaignV82::RequiredRoleMask(B);
            const uint32 Supports=(1u<<int32(EFrontlineSupport::Artillery))|(1u<<int32(EFrontlineSupport::Smoke));
            AllocationModels.Empty();if(Kind==2){AllocationModels.Add(Nation==EFRRUNationV81::French?TEXT("fr.ft"):TEXT("ru.austin_series1"));if(Nation==EFRRUNationV81::French)AllocationModels.Add(TEXT("fr.schneider"));}
            if(!AllocationCheck(TEXT("public_date_legal_loadout"),WarfareFRRUCampaignV82::SetSelection(B,Roles,Supports)
                &&WarfareFRRUCampaignV82::SetArmourSelection(B,AllocationModels)))return Finish();
            const int32 Points=WarfareFRRUCampaignV82::ResearchPoints(B);
            if(!AllocationCheck(TEXT("public_free_attachment_transaction"),WarfareFRRUCampaignV82::BuyRequisitionV87(B,Kind)
                &&WarfareFRRUCampaignV82::RequisitionV87(B)==Kind&&WarfareFRRUCampaignV82::ResearchPoints(B)==Points))return Finish();
            FString Error;if(!AllocationCheck(TEXT("real_scene_prepared"),WarfareFRRUWorldV82::PrepareScene(B,Error),Error))return Finish();
            B->SetPaused(false);B->BeginBattle();if(!AllocationCheck(TEXT("real_world_begun"),B->Phase==EFrontlinePhase::Fighting&&WarfareFRRUWorldV82::Ready(B),B->LastMessage))return Finish();
            B->SetBattleSpeed(1.f);AllocationEncounter=B->GetEncounterId();AllocationAt=B->Elapsed;
            AllocationPioneers=PlayerPioneers(B);AllocationSmokeZones=PlayerSmokeZones(B);AllocationPioneerId=INDEX_NONE;AllocationInitialPioneerIds.Empty();
            for(const auto& S:B->Squads)if(S.Team==0&&S.Role==EFrontlineRole::Pioneer)AllocationInitialPioneerIds.Add(S.Id);
            for(int32 I=0;I<AWarfareFrontlineGameMode::RoleCount;++I)AllocationReady[I]=B->RecruitReadyAt[0][I];
            int32 Frozen=INDEX_NONE;float Baseline=0;
            if(!AllocationCheck(TEXT("encounter_freezes_attachment_and_real_initial_smoke_clock"),WarfareFRRUCampaignV82::FrozenRequisitionV87(B,Frozen,Baseline)
                &&Frozen==Kind&&Baseline==B->SmokeReadyAt[0]&&Baseline>B->Elapsed))return Finish();
            AllocationSmokeBaseline=Baseline;
            if(!AllocationCheck(TEXT("fighting_rejects_purchase_or_replacement"),!WarfareFRRUCampaignV82::CanRequisitionV87(B,Kind)
                &&!WarfareFRRUCampaignV82::BuyRequisitionV87(B,(Kind+1)%3)))return Finish();
            if(Kind==2)
            {
                for(const auto& Id:AllocationModels)CooldownFraction(B,0,Id);
                const auto* Reserve=Fleets.Find(UFrontlineVehicleSubsystem::Get(B->GetWorld()));bool Initial=Reserve&&!Reserve->OpeningReleasedV87[0];
                for(const auto& Id:AllocationModels){const float* Ready=Reserve?Reserve->ModelReadyAt[0].Find(Id):nullptr;Initial&=Ready&&*Ready>B->Elapsed;}
                if(!AllocationCheck(TEXT("selected_native_models_have_real_initial_preparation"),Initial))return Finish();
            }
            AllocationSample(B,TEXT("archive_allocation_before_first_host_tick"));AllocationStage=1;return true;
        }
        if(!AllocationCheckActive(B))return Finish();
        const auto Story=WarfareFRRUWorldV82::View(B);
        if(Story.StoryPending)
        {
            if(!AllocationCheck(TEXT("pending_story_uses_real_public_choice"),!Story.ChoiceIds.IsEmpty()
                &&WarfareFRRUWorldV82::ChooseStory(B,Story.StoryId,Story.ChoiceIds[0])))return Finish();
            return true;
        }
        if(Kind==0)
        {
            if(!AllocationQueueUnchanged(B)){AllocationCheck(TEXT("pioneers_do_not_charge_ordinary_infantry_queue"),false);return Finish();}
            if(AllocationStage==1)
            {
                if(PlayerPioneers(B)==AllocationPioneers&&B->Elapsed-AllocationAt<10.f)return true;
                for(const auto& S:B->Squads)if(S.Team==0&&S.Role==EFrontlineRole::Pioneer&&!AllocationInitialPioneerIds.Contains(S.Id)&&S.LivingCount()>0)AllocationPioneerId=S.Id;
                const auto* Extra=B->FindSquad(AllocationPioneerId);
                if(!AllocationCheck(TEXT("actual_extra_living_pioneer_section_delivered_once"),B->Elapsed>.15f&&PlayerPioneers(B)==AllocationPioneers+1
                    &&Extra&&Extra->LivingCount()>0&&AllocationQueueUnchanged(B)))return Finish();
                AllocationSample(B,TEXT("archive_allocation_actual_pioneer_spawn"));AllocationAt=B->Elapsed;AllocationStage=2;return true;
            }
            if(B->Elapsed-AllocationAt<.5f)return true;
            if(!AllocationCheck(TEXT("later_host_ticks_do_not_duplicate_pioneers_or_spend_infantry_queue"),PlayerPioneers(B)==AllocationPioneers+1
                &&B->FindSquad(AllocationPioneerId)&&AllocationQueueUnchanged(B)))return Finish();
            return CompleteAllocationCase(B);
        }
        if(Kind==1)
        {
            if(AllocationStage<=2)
            {
                if(B->SmokeReadyAt[0]!=AllocationSmokeBaseline){AllocationCheck(TEXT("initial_host_ticks_do_not_consume_smoke_reserve"),false);return Finish();}
                if(AllocationStage==1&&B->Elapsed>.2f){AllocationCheck(TEXT("initial_host_ticks_keep_real_smoke_preparation"),true);AllocationStage=2;}
                if(B->Elapsed<AllocationSmokeBaseline)return true;
                const int32 Commands=B->SmokeCommands;
                if(!AllocationCheck(TEXT("first_public_smoke_waits_real_initial_clock_and_creates_zone"),B->RequestSmoke(FVector::ZeroVector,0)
                    &&B->SmokeCommands==Commands+1&&PlayerSmokeZones(B)==AllocationSmokeZones+1&&B->SmokeReadyAt[0]>B->Elapsed,B->LastMessage))return Finish();
                AllocationSample(B,TEXT("archive_allocation_actual_first_smoke"));AllocationAt=B->Elapsed;AllocationStage=3;return true;
            }
            if(AllocationStage==3)
            {
                if(B->Elapsed<=AllocationAt)return true;
                if(!AllocationCheck(TEXT("ordinary_tick_releases_exactly_one_smoke_reserve"),B->SmokeReadyAt[0]<=B->Elapsed&&B->SmokeCooldown(0)==0))return Finish();
                AllocationSample(B,TEXT("archive_allocation_actual_smoke_reserve_release"));
                const int32 Commands=B->SmokeCommands;
                if(!AllocationCheck(TEXT("second_public_smoke_creates_real_zone_and_starts_normal_cooldown"),B->RequestSmoke(FVector::ZeroVector,0)
                    &&B->SmokeCommands==Commands+1&&PlayerSmokeZones(B)==AllocationSmokeZones+2&&B->SmokeReadyAt[0]>B->Elapsed,B->LastMessage))return Finish();
                AllocationSample(B,TEXT("archive_allocation_actual_second_smoke"));AllocationSecondSmokeReady=B->SmokeReadyAt[0];AllocationAt=B->Elapsed;AllocationStage=4;return true;
            }
            if(B->Elapsed-AllocationAt<.5f)return true;
            const int32 Commands=B->SmokeCommands;
            if(!AllocationCheck(TEXT("later_ticks_and_third_call_cannot_repeat_smoke_reserve"),B->SmokeReadyAt[0]==AllocationSecondSmokeReady
                &&B->SmokeCooldown(0)>0&&!B->RequestSmoke(FVector::ZeroVector,0)&&B->SmokeCommands==Commands&&PlayerSmokeZones(B)==AllocationSmokeZones+2))return Finish();
            return CompleteAllocationCase(B);
        }
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());const auto* Reserve=Fleet?Fleets.Find(Fleet):nullptr;
        if(!Reserve){AllocationCheck(TEXT("actual_native_fleet_exists"),false);return Finish();}
        if(AllocationStage==1)
        {
            if(B->Elapsed<=.2f)return true;
            bool Ready=Reserve->OpeningReleasedV87[0]&&Reserve->ReadyAt[0]<=B->Elapsed;
            for(const auto& Id:AllocationModels){const float* At=Reserve->ModelReadyAt[0].Find(Id);Ready&=At&&*At<=B->Elapsed&&BlockReason(B,Id,0).IsEmpty();}
            if(!AllocationCheck(TEXT("ordinary_tick_releases_selected_native_opening_readiness"),Ready))return Finish();
            AllocationSample(B,TEXT("archive_allocation_actual_opening_armour_release"));
            const int32 Before=Fleet->GetVehicles().Num();
            if(!AllocationCheck(TEXT("public_native_recruit_creates_actual_vehicle"),WarfareFRRUArmourV82::Recruit(B,AllocationModels[0],0)
                &&Fleet->GetVehicles().Num()==Before+1,BlockReason(B,AllocationModels[0],0)))return Finish();
            for(AFrontlineTank* V:Fleet->GetVehicles())if(V&&V->Team==0&&Id(V)==AllocationModels[0])Gun=V;
            Reserve=Fleets.Find(Fleet);AllocationModelReady.Empty();bool Cooldown=Gun.IsValid()&&Gun->IsOperational()&&Gun->AreAssetsReady()&&Fleet->GetVehicles().Contains(Gun.Get())&&Reserve;
            for(const auto& Id:AllocationModels){const float* At=Reserve?Reserve->ModelReadyAt[0].Find(Id):nullptr;Cooldown&=At&&*At>B->Elapsed;if(At)AllocationModelReady.Add(Id,*At);}
            if(!AllocationCheck(TEXT("actual_dispatch_starts_each_selected_model_new_cooldown"),Cooldown))return Finish();
            AllocationSample(B,TEXT("archive_allocation_actual_native_dispatch"));
            if(!AllocationCheck(TEXT("repeated_opening_release_rejected_after_actual_dispatch"),!ReleaseOpeningReadinessV87(B,0)))return Finish();
            AllocationAt=B->Elapsed;AllocationStage=2;return true;
        }
        if(B->Elapsed-AllocationAt<.5f)return true;
        bool Same=Gun.IsValid()&&Gun->IsOperational()&&Fleet->LastEncounter==AllocationEncounter;
        for(const auto& Pair:AllocationModelReady){const float* At=Reserve->ModelReadyAt[0].Find(Pair.Key);Same&=At&&*At==Pair.Value&&*At>B->Elapsed;}
        const int32 Before=Fleet->GetVehicles().Num();
        if(!AllocationCheck(TEXT("later_host_ticks_and_repeat_dispatch_keep_real_model_cooldowns"),Same
            &&!WarfareFRRUArmourV82::Recruit(B,AllocationModels[0],0)&&Fleet->GetVehicles().Num()==Before))return Finish();
        return CompleteAllocationCase(B);
    }
    bool AllocationCheckActive(AWarfareFrontlineGameMode* B)
    {
        int32 Kind=INDEX_NONE;float Baseline=0;
        const bool Good=B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE&&B->GetEncounterId()==AllocationEncounter
            &&WarfareFRRUCampaignV82::FrozenRequisitionV87(B,Kind,Baseline)&&Kind==AllocationCase%3&&Baseline==AllocationSmokeBaseline;
        return Good||AllocationCheck(TEXT("same_real_active_encounter_keeps_frozen_attachment"),false,B->LastMessage);
    }
    bool BeginMission(AWarfareFrontlineGameMode* B,const FString& Mission,const TArray<FString>& Models,bool BuyPreparation=false)
    {
        FString Error;B->BattleOutroWinner=INDEX_NONE;
        if(!WarfareFRRUCampaignInternalV82::PrepareReviewMission(B,Mission,Error))return Check(TEXT("disposable_prior_progression_and_legal_mission"),false,Error);
        const auto* M=wfexp::FindMission(wfexpue::Narrow(Mission));if(!M)return Check(TEXT("catalogue_mission_exists"),false);
        uint32 Roles=((1u<<int32(EFrontlineRole::Rifle))|(1u<<int32(EFrontlineRole::Pioneer)))&M->roleMask;
        if(!WarfareFRRUCampaignV82::SetSelection(B,Roles,1u)||!WarfareFRRUCampaignV82::SetArmourSelection(B,Models))return Check(TEXT("formal_loadout_selection"),false);
        if(BuyPreparation)
            if(!Check(TEXT("public_native_crew_and_prepare_purchase"),WarfareFRRUCampaignV82::PurchaseOption(B,TEXT("fr.armour.crew"))
                &&WarfareFRRUCampaignV82::PurchaseOption(B,TEXT("fr.armour.prepare"))
                &&WarfareFRRUCampaignV82::OwnsOption(B,TEXT("fr.armour.prepare"))))return false;
        if(!WarfareFRRUWorldV82::PrepareScene(B,Error))return Check(TEXT("formal_scene_preparation"),false,Error);
        B->BeginBattle();if(!Check(TEXT("formal_begin_uses_real_world"),B->Phase==EFrontlinePhase::Fighting&&WarfareFRRUWorldV82::Ready(B),B->LastMessage))return false;
        B->SetPaused(false);B->SetBattleSpeed(1.f);
        StageAt=B->Elapsed;return true;
    }
    bool CaptureOpening(AWarfareFrontlineGameMode* B,bool Researched)
    {
        using namespace WarfareFRRUArmourV82;
        const float LongFraction=CooldownFraction(B,0,TEXT("fr.schneider")),ShortFraction=CooldownFraction(B,0,TEXT("fr.ft"));
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());const auto* Reserve=Fleets.Find(Fleet);
        const float* Long=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.schneider")):nullptr;
        const float* Short=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.ft")):nullptr;
        if(!Check(Researched?TEXT("researched_models_have_real_opening_preparation"):TEXT("unresearched_models_have_real_opening_preparation"),
            Long&&Short&&*Short>B->Elapsed&&*Long>*Short&&LongFraction>0&&ShortFraction>0
            &&WarfareFRRUCampaignV82::OwnsOption(B,TEXT("fr.armour.prepare"))==Researched&&DispatchSpecV87(B,0).Prepare==Researched
            &&!BlockReason(B,TEXT("fr.schneider"),0).IsEmpty()&&!BlockReason(B,TEXT("fr.ft"),0).IsEmpty()))return false;
        OpeningEncounter=B->GetEncounterId();OpeningLong=*Long;OpeningShort=*Short;OpeningQueries=0;SawShortOpeningReady=false;
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("event"),Researched?TEXT("researched_opening_readiness"):TEXT("unresearched_opening_readiness"));
        Row->SetNumberField(TEXT("hostElapsed"),B->Elapsed);Row->SetStringField(TEXT("encounter"),OpeningEncounter.ToString());
        Row->SetNumberField(TEXT("schneiderReadyAt"),OpeningLong);Row->SetNumberField(TEXT("ftReadyAt"),OpeningShort);
        Row->SetBoolField(TEXT("prepareOwned"),WarfareFRRUCampaignV82::OwnsOption(B,TEXT("fr.armour.prepare")));
        Row->SetBoolField(TEXT("openingReleaseUsed"),false);Samples.Add(MakeShared<FJsonValueObject>(Row));return true;
    }
    void QuietTarget(AFrontlineTank* V)
    {
        if(!V)return;
        // Test only the selected national gun's acquisition. Disable its
        // target's running gear through real module damage; hull health,
        // visibility, bounds and incoming damage remain the service values.
        if(!V->IsImmobilized())V->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,1000000);
        for(auto& Clock:V->CannonClock)Clock=MAX_flt;
        for(auto& Clock:V->GunClock)Clock=MAX_flt;
    }
    void SampleActualWreck(AFrontlineTank* V,const TCHAR* Event)
    {
        auto Sample=WarfareFRRUArmourV82::GroundPilotDiagnosis(V);Sample->SetStringField(TEXT("event"),Event);
        TArray<TSharedPtr<FJsonValue>> Hinges;
        if(const auto* R=WarfareFRRUArmourV82::Runtime(V))if(auto* Pose=R->Model.Get())for(const auto& Bone:R->HatchBones)
        {
            const FTransform Parent=Bone.ParentBone.IsNone()?FTransform::Identity:Pose->GetBoneTransformByName(Bone.ParentBone,EBoneSpaces::ComponentSpace);
            const FTransform Actual=Pose->GetBoneTransformByName(Bone.Bone,EBoneSpaces::ComponentSpace).GetRelativeTransform(Parent);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("bone"),Bone.Bone.ToString());Row->SetBoolField(TEXT("finite"),!Actual.ContainsNaN());
            Row->SetNumberField(TEXT("actualLocalRotationFromRestDegrees"),FMath::RadiansToDegrees(Actual.GetRotation().AngularDistance(Bone.RestLocal.GetRotation())));
            Row->SetNumberField(TEXT("actualLocalTranslationFromRestCm"),FVector::Dist(Actual.GetTranslation(),Bone.RestLocal.GetTranslation()));Hinges.Add(MakeShared<FJsonValueObject>(Row));
        }
        Sample->SetArrayField(TEXT("actualDestroyedHatchPoseV87"),Hinges);Samples.Add(MakeShared<FJsonValueObject>(Sample));
    }
    bool PlacePair(AWarfareFrontlineGameMode* B,int32 AcrossSign)
    {
        using namespace WarfareFRRUArmourV82;auto* V=Gun.Get();auto* Enemy=Target.Get();auto* RuntimeState=Runtime(V);
        if(!V||!Enemy||!RuntimeState)return false;
        struct FCandidate {FVector A,Z;double Roughness=0;};TArray<FCandidate> Candidates;
        const int32 Dir=B->GetTeamDirection(0);const float Yaw=Dir>0?0.f:180.f;
        int32 OutsideHeightGrid=0;
        for(float X=-8000;X<=8000;X+=1000)for(float Y=-6000;Y<=6000;Y+=1000)
        {
            FCandidate Candidate;Candidate.A=FVector(X,Y,0);Candidate.Z=Candidate.A+FVector(Dir*2600,AcrossSign*1800,0);
            bool Valid=true;
            for(FVector* Point:{&Candidate.A,&Candidate.Z})
            {
                float Height=0;if(!WarfareFRRUWorldV82::TryHeight(B,Point->X,Point->Y,Height)){Valid=false;break;}Point->Z=Height;
            }
            // The real contact solver, complete-hull collision and committed
            // muzzle sight below decide validity. A nine-height flatness cap
            // and a guessed 160cm eye line were fixture-only false exclusions.
            if(Valid){Candidate.Roughness=FMath::Abs(Candidate.A.Z-Candidate.Z.Z);Candidates.Add(Candidate);}
            else ++OutsideHeightGrid;
        }
        Candidates.Sort([](const FCandidate& A,const FCandidate& Z){return A.Roughness<Z.Roughness;});
        TArray<TSharedPtr<FJsonValue>> Attempts;TMap<FString,int32> Rejections;bool SearchRecorded=false;
        const auto RecordSearch=[&](bool Accepted)
        {
            if(SearchRecorded)return;SearchRecorded=true;auto Search=MakeShared<FJsonObject>();
            Search->SetStringField(TEXT("event"),AcrossSign<0?TEXT("left_physical_pair_search"):TEXT("right_physical_pair_search"));
            Search->SetNumberField(TEXT("heightEligibleCandidates"),Candidates.Num());Search->SetNumberField(TEXT("outsideNativeHeightGrid"),OutsideHeightGrid);
            Search->SetNumberField(TEXT("attemptedCandidates"),Attempts.Num());Search->SetBoolField(TEXT("accepted"),Accepted);
            Search->SetBoolField(TEXT("productionContactAndLOSUnchanged"),true);Search->SetBoolField(TEXT("artificialFlatnessFilterApplied"),false);
            auto Counts=MakeShared<FJsonObject>();for(const auto& Pair:Rejections)Counts->SetNumberField(Pair.Key,Pair.Value);
            Search->SetObjectField(TEXT("rejectionsByStage"),Counts);Search->SetArrayField(TEXT("actualCandidateAttempts"),Attempts);Samples.Add(MakeShared<FJsonValueObject>(Search));
        };
        for(int32 Index=0;Index<Candidates.Num();++Index)
        {
            const auto& Candidate=Candidates[Index];FTransform Pose;
            auto Attempt=MakeShared<FJsonObject>();Attempt->SetNumberField(TEXT("candidate"),Index);Attempt->SetStringField(TEXT("gunPosition"),Candidate.A.ToString());Attempt->SetStringField(TEXT("enemyPosition"),Candidate.Z.ToString());
            Attempts.Add(MakeShared<FJsonValueObject>(Attempt));
            const auto Reject=[&](const TCHAR* Reason){Attempt->SetStringField(TEXT("rejectedStage"),Reason);++Rejections.FindOrAdd(Reason);};
            if(!FTGroundPilotV82::InCompleteGrid(V,Candidate.A,Yaw)){Reject(TEXT("national_complete_grid"));continue;}
            if(!FTGroundPilotV82::Plan(V,Candidate.A,Yaw,0,Pose))
            {Reject(TEXT("national_actual_contact_or_hull"));Attempt->SetObjectField(TEXT("actualNationalGroundRejection"),GroundPilotDiagnosis(V));continue;}
            Attempt->SetBoolField(TEXT("actualNationalGroundPlanPassed"),true);
            const FTransform PriorEnemy=Enemy->GetActorTransform(),PriorNational=V->GetActorTransform();
            const float PriorEnemyHeading=Enemy->HeadingYawV37,PriorNationalHeading=RuntimeState->Heading;
            bool EnemyCommitted=false;
            const auto RestorePlacement=[&]()
            {
                V->SetActorTransform(PriorNational,false,nullptr,ETeleportType::TeleportPhysics);RuntimeState->Heading=PriorNationalHeading;
                const bool Restored=Enemy->SetActorTransform(PriorEnemy,false,nullptr,ETeleportType::TeleportPhysics);Enemy->HeadingYawV37=PriorEnemyHeading;
                // A successful contact commit has a warm cache and real shoe
                // poses. Revalidate those at the rebased old actor location;
                // never simulate travel back across the entire battlefield.
                const bool ContactRestored=!EnemyCommitted||Enemy->CommitNationalGroundV82(PriorEnemy.GetLocation(),PriorEnemyHeading,0,true);
                Attempt->SetBoolField(TEXT("controlledRejectedPlacementRestored"),Restored&&ContactRestored);
                return Restored&&ContactRestored;
            };
            FTransform RebasedEnemy=PriorEnemy;RebasedEnemy.SetLocation(Candidate.Z);RebasedEnemy.SetRotation(FRotator(0,Yaw+180.f,0).Quaternion());
            Attempt->SetBoolField(TEXT("controlledNoTickEnemyPoseRebase"),true);Attempt->SetStringField(TEXT("enemyBeforeFixtureRebase"),PriorEnemy.GetLocation().ToString());
            if(!Enemy->SetActorTransform(RebasedEnemy,false,nullptr,ETeleportType::TeleportPhysics))
            {Reject(TEXT("controlled_enemy_actor_rebase"));if(!RestorePlacement())return false;continue;}
            Enemy->HeadingYawV37=Yaw+180.f;
            if(!Enemy->CommitNationalGroundV82(Candidate.Z,Yaw+180.f,0,true))
            {Reject(TEXT("german_actual_contact_or_hull"));if(auto German=Enemy->DescribeNationalGroundContactV82())Attempt->SetObjectField(TEXT("actualGermanGroundRejection"),German);if(!RestorePlacement())return false;continue;}
            EnemyCommitted=true;
            Attempt->SetBoolField(TEXT("actualGermanGroundCommitPassed"),true);
            if(!V->SetActorTransform(Pose,false,nullptr,ETeleportType::None)){Reject(TEXT("national_actor_transform"));if(!RestorePlacement())return false;continue;}
            FTGroundPilotV82::CommitWarm(V);RuntimeState->Heading=Yaw;RuntimeState->AvoidUntil=0;
            if(!Clear(V,Enemy,Muzzle(V,0,true),Enemy->AimPoint())){Reject(TEXT("actual_committed_muzzle_line_of_sight"));if(!RestorePlacement())return false;continue;}
            V->bEntered=Enemy->bEntered=true;QuietTarget(Enemy);PairAnchor=Candidate.A;
            InitialAngle=FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,(Enemy->GetActorLocation()-V->GetActorLocation()).Rotation().Yaw));
            ShotsAt=V->CannonShots;StageAt=B->Elapsed;SawAcquiring=SawStop=false;
            Attempt->SetBoolField(TEXT("accepted"),true);RecordSearch(true);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("event"),AcrossSign<0?TEXT("left_placement"):TEXT("right_placement"));
            Row->SetStringField(TEXT("actualGun"),GetPathNameSafe(V));Row->SetStringField(TEXT("actualEnemy"),GetPathNameSafe(Enemy));
            Row->SetStringField(TEXT("gunPosition"),V->GetActorLocation().ToString());Row->SetStringField(TEXT("enemyPosition"),Enemy->GetActorLocation().ToString());
            Row->SetNumberField(TEXT("initialBearingErrorDegrees"),InitialAngle);Row->SetNumberField(TEXT("hostElapsed"),B->Elapsed);Samples.Add(MakeShared<FJsonValueObject>(Row));return true;
        }
        RecordSearch(false);
        auto Miss=GroundPilotDiagnosis(V);Miss->SetStringField(TEXT("event"),TEXT("no_clear_physical_pair"));Miss->SetNumberField(TEXT("candidateCount"),Candidates.Num());
        Samples.Add(MakeShared<FJsonValueObject>(Miss));
        if(auto German=Enemy->DescribeNationalGroundContactV82())Samples.Add(MakeShared<FJsonValueObject>(German));
        return false;
    }
    bool Tick()
    {
        using namespace WarfareFRRUArmourV82;
        if(FPlatformTime::Seconds()-Started>480){Check(TEXT("bounded_actual_runtime_review"),false);return Finish();}
        auto* B=Battle.Get();
        if(!B)
        {
            if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World();World&&World->WorldType==EWorldType::Game)
                if(auto* Candidate=World->GetAuthGameMode<AWarfareFrontlineGameMode>();Candidate&&Candidate->HasActorBegunPlay()&&Candidate->IsProgressionVolatile()&&Candidate->Phase==EFrontlinePhase::Briefing){Battle=Candidate;B=Candidate;break;}
            if(!B)return true;
        }
        if(AllocationConsumersOnly)return TickAllocationConsumers(B);
        const auto View=WarfareFRRUWorldV82::View(B);
        if(View.StoryPending&&!View.ChoiceIds.IsEmpty())WarfareFRRUWorldV82::ChooseStory(B,View.StoryId,View.ChoiceIds[0]);
        if(Stage==0)
        {
            if(!BeginMission(B,TEXT("fr.matz"),{TEXT("fr.schneider"),TEXT("fr.ft")}))return Finish();
            if(!CaptureOpening(B,false))return Finish();Stage=10;return true;
        }
        if(Stage==10||Stage==11)
        {
            if(B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)
            {Check(TEXT("opening_readiness_remains_in_real_active_battle"),false,B->LastMessage);return Finish();}
            auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            const float LongFraction=CooldownFraction(B,0,TEXT("fr.schneider")),ShortFraction=CooldownFraction(B,0,TEXT("fr.ft"));
            const auto* Reserve=Fleet?Fleets.Find(Fleet):nullptr;
            const float* Long=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.schneider")):nullptr;
            const float* Short=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.ft")):nullptr;
            if(!Reserve||!Long||!Short||B->GetEncounterId()!=OpeningEncounter||*Long!=OpeningLong||*Short!=OpeningShort)
            {Check(TEXT("repeated_public_readiness_queries_keep_original_model_timestamps"),false);return Finish();}
            // Isolate the selected gun's later contact test; elapsed advances
            // through ordinary world ticks. No readiness/clock is granted.
            Fleet->NextEnemyAt=MAX_flt;Fleets.FindChecked(Fleet).EnemyAt=MAX_flt;++OpeningQueries;
            if(!SawShortOpeningReady&&B->Elapsed>=OpeningShort&&B->Elapsed<OpeningLong)
            {
                if(!Check(Stage==10?TEXT("unresearched_ft_becomes_ready_before_schneider"):TEXT("researched_ft_becomes_ready_before_schneider"),
                    ShortFraction==0&&LongFraction>0&&BlockReason(B,TEXT("fr.ft"),0).IsEmpty()&&!BlockReason(B,TEXT("fr.schneider"),0).IsEmpty()))return Finish();
                SawShortOpeningReady=true;
            }
            if(B->Elapsed<OpeningLong)return true;
            if(!Check(Stage==10?TEXT("unresearched_opening_waits_on_real_host_clock"):TEXT("researched_opening_waits_on_real_host_clock"),
                SawShortOpeningReady&&OpeningQueries>2&&LongFraction==0&&BlockReason(B,TEXT("fr.schneider"),0).IsEmpty()))return Finish();
            if(Stage==10)
            {
                BaselineLong=OpeningLong;BaselineShort=OpeningShort;
                if(!BeginMission(B,TEXT("fr.matz"),{TEXT("fr.schneider"),TEXT("fr.ft")},true)||!CaptureOpening(B,true))return Finish();
                const auto* Option=wfexp::tactical::FindOption("fr.armour.prepare");
                if(!Check(TEXT("public_prepare_purchase_scales_actual_initial_model_timestamps_once"),Option&&DispatchSpecV87(B,0).Prepare
                    &&FMath::IsNearlyEqual(OpeningLong,BaselineLong*float(Option->primaryEffect),.001f)
                    &&FMath::IsNearlyEqual(OpeningShort,BaselineShort*float(Option->primaryEffect),.001f)))return Finish();
                Stage=11;return true;
            }
            Stage=12;
        }
        if(Stage==12)
        {
            auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            if(!Check(TEXT("actual_vehicle_subsystem_exists"),Fleet!=nullptr))return Finish();
            if(!Check(TEXT("formal_player_schneider_recruit"),Recruit(B,TEXT("fr.schneider"),0)))return Finish();
            for(AFrontlineTank* V:Fleet->GetVehicles())if(V&&Id(V)==TEXT("fr.schneider"))Gun=V;
            if(!Check(TEXT("first_dispatch_after_real_opening_wait_uses_current_host_encounter"),Fleet->LastEncounter==B->GetEncounterId(),
                FString::Printf(TEXT("hostEpoch=%s; battleEpoch=%s; hostElapsed=%.3f; vehicles=%d"),*Fleet->LastEncounter.ToString(),*B->GetEncounterId().ToString(),B->Elapsed,Fleet->GetVehicles().Num())))return Finish();
            // Real opening ticks already established the encounter. This run
            // does not claim to repeat the earlier same-frame epoch fixture.
            Fleet->NextEnemyAt=MAX_flt;Fleets.FindChecked(Fleet).EnemyAt=MAX_flt;
            const auto* Reserve=Fleets.Find(Fleet);const float* Short=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.ft")):nullptr;
            const float* Long=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.schneider")):nullptr;
            const auto* Dispatched=Runtime(Gun.Get());
            if(!Check(TEXT("dispatch_freezes_distinct_model_ready_times"),Short&&Long&&*Short<*Long&&Dispatched
                &&FMath::IsNearlyEqual(*Short-B->Elapsed,Definition(TEXT("fr.ft"))->Cooldown*Dispatched->Spec.PreparationScale,.01f)))return Finish();
            const float FirstSchneiderReadyAt=Long?*Long:-1.f;
            FirstDispatchReadyAt=FirstSchneiderReadyAt;
            const bool ReleasedAfterDispatch=ReleaseOpeningReadinessV87(B,0);const auto* AfterRelease=Fleets.Find(Fleet);
            const float* ProtectedReady=AfterRelease?AfterRelease->ModelReadyAt[0].Find(TEXT("fr.schneider")):nullptr;
            if(!Check(TEXT("opening_release_cannot_reset_actual_post_dispatch_model_cooldown"),!ReleasedAfterDispatch&&ProtectedReady&&*ProtectedReady==FirstSchneiderReadyAt))return Finish();
            const int32 Before=Fleet->GetVehicles().Num();Check(TEXT("other_model_cannot_bypass_shared_dispatch"),!Recruit(B,TEXT("fr.ft"),0)&&Fleet->GetVehicles().Num()==Before);
            // The host's German reserve has its own initial preparation,
            // independent of the national player's model timers above. This
            // bounded acquisition fixture expires that reserve explicitly;
            // it must not rewrite the battle clock or production readiness.
            const FString InitialGermanBlock=Fleet->RecruitBlockReason(EFrontlineVehicle::A7V,1);
            Fleet->ResetReadinessForDebug(1);
            const FString GermanBlockBefore=Fleet->RecruitBlockReason(EFrontlineVehicle::A7V,1);
            const bool GermanRecruited=Fleet->Recruit(EFrontlineVehicle::A7V,1);
            const FString GermanBlockAfter=Fleet->RecruitBlockReason(EFrontlineVehicle::A7V,1);
            if(!Check(TEXT("formal_enemy_german_a7v_recruit"),GermanRecruited,FString::Printf(
                TEXT("controlledEnemyInitialCooldownExpired=1; initialBlock=%s; preRecruitBlock=%s; postRecruitBlock=%s; lastMessage=%s; hostElapsed=%.3f"),
                *InitialGermanBlock,*GermanBlockBefore,*GermanBlockAfter,*B->LastMessage,B->Elapsed)))return Finish();
            const auto* KeptReserve=Fleets.Find(Fleet);const float* KeptReady=KeptReserve?KeptReserve->ModelReadyAt[0].Find(TEXT("fr.schneider")):nullptr;
            if(!Check(TEXT("host_dispatch_preserves_existing_national_actor_and_model_cooldown"),Gun.IsValid()&&!Gun->IsActorBeingDestroyed()
                &&Fleet->GetVehicles().Contains(Gun.Get())&&Runtime(Gun.Get())&&KeptReady&&*KeptReady==FirstSchneiderReadyAt,
                FString::Printf(TEXT("nationalActorValid=%d; nationalActorRegistered=%d; beforeReadyAt=%.3f; afterReadyAt=%.3f; hostElapsed=%.3f"),
                    Gun.IsValid(),Gun.IsValid()&&Fleet->GetVehicles().Contains(Gun.Get()),FirstSchneiderReadyAt,KeptReady?*KeptReady:-1.f,B->Elapsed)))return Finish();
            for(AFrontlineTank* V:Fleet->GetVehicles())if(V&&V->Team==1&&V->Type==EFrontlineVehicle::A7V)Target=V;
            if(!Check(TEXT("real_national_and_german_ground_assets"),Gun.IsValid()&&Target.IsValid()&&Target->UsesNationalGroundContactV82()&&Gun->AreAssetsReady()&&Target->AreAssetsReady()))return Finish();
            if(!Check(TEXT("physical_left_pair_available"),PlacePair(B,-1)))return Finish();Stage=1;return true;
        }
        if(Stage==1||Stage==2)
        {
            auto* V=Gun.Get();auto* Enemy=Target.Get();auto* R=Runtime(V);if(!V||!Enemy||!R||!V->IsOperational()||!Enemy->IsOperational()){Check(TEXT("selected_live_armour_remains_available"),false);return Finish();}
            if(!FirstHostTickProven&&R->EngagementHostAt>StageAt)
            {
                auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());const auto* Reserve=Fleets.Find(Fleet);
                const float* Ready=Reserve?Reserve->ModelReadyAt[0].Find(TEXT("fr.schneider")):nullptr;
                if(!Check(TEXT("real_host_simulation_tick_keeps_first_national_dispatch_and_readiness"),Fleet&&Fleet->LastEncounter==B->GetEncounterId()
                    &&Fleet->GetVehicles().Contains(V)&&!V->IsActorBeingDestroyed()&&Ready&&*Ready==FirstDispatchReadyAt,
                    FString::Printf(TEXT("actualSimulationHostAt=%.3f; placementHostAt=%.3f; beforeReadyAt=%.3f; afterReadyAt=%.3f"),
                        R->EngagementHostAt,StageAt,FirstDispatchReadyAt,Ready?*Ready:-1.f)))return Finish();
                FirstHostTickProven=true;
            }
            QuietTarget(Enemy);SawAcquiring|=R->HullAcquiringArmour;SawStop|=R->HaltForWeapon&&R->StopContact.Target.Get()==Enemy;
            const float Error=FMath::Abs(FMath::FindDeltaAngleDegrees(R->Heading,(Enemy->GetActorLocation()-V->GetActorLocation()).Rotation().Yaw));
            if(V->CannonShots>ShotsAt)
            {
                Check(Stage==1?TEXT("left_fixed_gun_acquires_and_fires"):TEXT("right_fixed_gun_acquires_and_fires"),SawAcquiring&&SawStop&&Error<InitialAngle-10.f&&B->Elapsed>StageAt);
                Samples.Add(MakeShared<FJsonValueObject>(GroundPilotDiagnosis(V)));
                Check(TEXT("model_cooldown_fractions_diverge_on_same_host_clock"),CooldownFraction(B,0,TEXT("fr.ft"))<CooldownFraction(B,0,TEXT("fr.schneider")));
                if(Stage==1){if(!Check(TEXT("physical_right_pair_available"),PlacePair(B,1)))return Finish();Stage=2;return true;}
                // Remove just this target through the real destruction route.
                // Release must follow its actual operational state, not a flag.
                Enemy->ReceiveModuleHit(EFrontlineTankModule::Hull,1000000,0,FFrontlineKillSourceV65::Vehicle(V));
                Check(TEXT("target_destroyed_by_actual_damage_route"),!Enemy->IsOperational());TravelAt=V->DistanceTravelled;StageAt=B->Elapsed;Stage=3;return true;
            }
            if(B->Elapsed-StageAt>18){Check(Stage==1?TEXT("left_fixed_gun_acquires_and_fires"):TEXT("right_fixed_gun_acquires_and_fires"),false);Samples.Add(MakeShared<FJsonValueObject>(GroundPilotDiagnosis(V)));return Finish();}
            return true;
        }
        if(Stage==3)
        {
            auto* V=Gun.Get();auto* R=Runtime(V);if(!V||!R){Check(TEXT("gun_survives_release"),false);return Finish();}
            if(B->Elapsed-StageAt<1.f)return true;
            Check(TEXT("destroyed_target_releases_hull_acquisition"),!R->HullAcquiringArmour);
            Check(TEXT("released_vehicle_can_resume_supported_travel"),V->DistanceTravelled>TravelAt+1.f);
            V->ReceiveModuleHit(EFrontlineTankModule::Hull,1000000,0,FFrontlineKillSourceV65::Vehicle(Target.Get()));
            Check(TEXT("national_wreck_created_by_actual_damage"),!V->IsOperational());
            V->WreckAge=150.1f;WreckAt=V->WreckAge;SmokeAt=V->SmokeAge;StageAt=B->Elapsed;Stage=4;return true;
        }
        if(Stage==4)
        {
            if(B->Elapsed-StageAt<.3f)return true;auto* V=Gun.Get();
            if(!Check(TEXT("aged_national_wreck_persists_and_updates_effects"),IsValid(V)&&!V->IsActorBeingDestroyed()&&V->WreckAge>WreckAt&&V->SmokeAge>SmokeAt))return Finish();
            SampleActualWreck(V,TEXT("actual_aged_national_wreck_appearance"));
            B->BattleOutroWinner=0;SmokeAt=V->SmokeAge;StageAt=B->Elapsed;Stage=5;return true;
        }
        if(Stage==5)
        {
            if(B->Elapsed-StageAt<.3f)return true;auto* V=Gun.Get();Check(TEXT("outro_keeps_existing_national_effects_advancing"),V&&V->SmokeAge>SmokeAt);
            SampleActualWreck(V,TEXT("actual_outro_national_wreck_appearance"));
            if(!BeginMission(B,TEXT("ru.czernowitz"),{}))return Finish();
            auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!Check(TEXT("actual_rail_subsystem_exists"),Fleet!=nullptr))return Finish();
            const float Fraction=CooldownFraction(B,1,TEXT("at-hu.armoured_train"));const auto* Reserve=Fleets.Find(Fleet);
            const float* Initial=Reserve?Reserve->ModelReadyAt[1].Find(TEXT("at-hu.armoured_train")):nullptr;
            if(!Check(TEXT("dated_enemy_train_has_real_initial_preparation"),Initial&&*Initial>B->Elapsed&&Fraction>0))return Finish();
            TrainOpening=*Initial;OpeningEncounter=B->GetEncounterId();Stage=20;return true;
        }
        if(Stage==20)
        {
            if(B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)
            {Check(TEXT("train_opening_readiness_remains_in_real_active_battle"),false,B->LastMessage);return Finish();}
            auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());const float Fraction=CooldownFraction(B,1,TEXT("at-hu.armoured_train"));
            const auto* InitialReserve=Fleet?Fleets.Find(Fleet):nullptr;const float* Initial=InitialReserve?InitialReserve->ModelReadyAt[1].Find(TEXT("at-hu.armoured_train")):nullptr;
            if(!Initial||*Initial!=TrainOpening||B->GetEncounterId()!=OpeningEncounter){Check(TEXT("enemy_train_initial_readiness_does_not_restart"),false);return Finish();}
            Fleet->NextEnemyAt=MAX_flt;Fleets.FindChecked(Fleet).EnemyAt=MAX_flt;
            if(B->Elapsed<TrainOpening)return true;
            if(!Check(TEXT("enemy_train_waits_real_initial_clock_before_recruit"),Fraction==0&&BlockReason(B,TEXT("at-hu.armoured_train"),1).IsEmpty()))return Finish();
            const int32 Before=Fleet->GetVehicles().Num();
            if(!Check(TEXT("formal_austrohungarian_train_recruit"),Recruit(B,TEXT("at-hu.armoured_train"),1)&&Fleet->GetVehicles().Num()==Before+1))return Finish();
            auto& Reserve=Fleets.FindChecked(Fleet);Reserve.ReadyAt[1]=B->Elapsed;Reserve.ModelReadyAt[1].FindChecked(TEXT("at-hu.armoured_train"))=B->Elapsed;
            const int32 One=Fleet->GetVehicles().Num();const FString Reason=BlockReason(B,TEXT("at-hu.armoured_train"),1);
            Check(TEXT("living_train_blocks_second_dispatch_after_controlled_cd_expiry"),!Reason.IsEmpty()&&!Recruit(B,TEXT("at-hu.armoured_train"),1)&&Fleet->GetVehicles().Num()==One);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("event"),TEXT("unique_train"));Row->SetStringField(TEXT("actualBlockReason"),Reason);Row->SetNumberField(TEXT("hostElapsed"),B->Elapsed);Samples.Add(MakeShared<FJsonValueObject>(Row));return Finish();
        }
        return true;
    }
};
