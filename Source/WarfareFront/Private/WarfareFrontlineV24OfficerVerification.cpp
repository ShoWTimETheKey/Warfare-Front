#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::BeginV24OfficerVerification()
{
    if (!FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVerify")) || !bProgressionVolatile) return;
    SetActorTickEnabled(false);
    const FString RunId=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V24/Officers"),RunId);
    IFileManager::Get().MakeDirectory(*Dir,true);
    const auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;int32 Passed=0;
    auto Check=[&](const FString& Name,bool Result)
    {
        const auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(Row));Passed+=Result?1:0;
        UE_LOG(LogTemp,Display,TEXT("WF_V24_OFFICER %s %s"),*Name,Result?TEXT("PASS"):TEXT("FAIL"));
    };
    // Explicit fixture: isolated file, synthetic research budget, local clock and
    // high clear-air positions for deterministic sight tests. No user archive.
    ProgressionFile=FPaths::Combine(Dir,TEXT("isolated-profile.json"));bProgressionVolatile=false;bProgressionReadOnly=false;
    CampaignRuns.Empty();ActiveCampaignId.Invalidate();RewardedEncounterIds.Empty();ArmyProgress[0]={};ArmyProgress[1]={};ResetEncounter();
    for (int32 Nation=0;Nation<2;++Nation)
    {
        RestartBattle();const auto Faction=EFrontlineFaction(Nation);const bool British=Nation==0;
        const FString Prefix=British?TEXT("british_"):TEXT("german_");const int32 Id=British?9:6;
        StartNewCampaign(Faction);ActiveResearchMutable().ResearchPoints=100;
        const auto& Node=GetTechnologyNodes()[Id];
        Check(Prefix+TEXT("sixteen_stable_ids_and_commission_prerequisites"),GetTechnologyNodes().Num()==16 && Node.Id==Id && Node.Cost==3
            && Node.Prerequisite==(British?5:1) && Node.SecondaryPrerequisite==(British?1:2) && !PurchaseTechnology(Id));
        PurchaseTechnology(0);PurchaseTechnology(1);if(British){PurchaseTechnology(4);PurchaseTechnology(5);}else PurchaseTechnology(2);
        const int32 Points=GetResearchPoints();
        Check(Prefix+TEXT("real_purchase_cost_and_role_replacement"),PurchaseTechnology(Id) && GetResearchPoints()==Points-3 && HasCommissionedOfficers()
            && FString(GetRecruitName(EFrontlineRole::Officer)).Contains(British?TEXT("Webley"):TEXT("P08")) && !PurchaseTechnology(Id));
        const FGuid Campaign=ActiveCampaignId;const uint32 Mask=GetActiveResearch().TechnologyMask;SaveProgression();LoadProgression(true);
        Check(Prefix+TEXT("existing_schema4_bits_roundtrip_without_new_mask_or_fee"),ActiveCampaignId==Campaign && GetActiveResearch().TechnologyMask==Mask && GetResearchPoints()==Points-3 && HasCommissionedOfficers());
        StartNewCampaign(Faction);
        Check(Prefix+TEXT("new_campaign_starts_without_officers_and_old_campaign_retained"),!HasCommissionedOfficers() && GetResearchPoints()==2 && ContinueCampaign(Campaign) && HasCommissionedOfficers());
        BeginBattle();const int32 Home=GetHomeTrenchIndex();SpawnSquad(0,EFrontlineRole::Officer,Home);SpawnSquad(1,EFrontlineRole::Rifle,GetHomeTrenchIndex(1));
        AWarfareFrontlineSoldier* Leader=nullptr;AWarfareFrontlineSoldier* Escort=nullptr;AWarfareFrontlineSoldier* Enemy=nullptr;int32 OfficerCount=0;
        for (AWarfareFrontlineSoldier* U:Units)
        {U->SetActorTickEnabled(false);if(U->IsCommissionedOfficer())++OfficerCount;if(U->Team==0){if(U->MemberIndex==0)Leader=U;else Escort=U;}else if(U->MemberIndex==0)Enemy=U;}
        const bool Ready=Leader && Escort && Enemy && Leader->IsCommissionedOfficer();
        Check(Prefix+TEXT("only_leader_has_real_officer_mesh_escorts_keep_rifles"),Ready && OfficerCount==1 && !Escort->IsCommissionedOfficer()
            && Leader->Soldier->GetSkeletalMeshAsset()->GetName()==(British?TEXT("SK_British_Officer_V24"):TEXT("SK_German_Officer_V24"))
            && Leader->Soldier->GetSkeletalMeshAsset()->GetSkeleton()==Escort->Soldier->GetSkeletalMeshAsset()->GetSkeleton()
            && Leader->GetMagazineCapacity()==(British?6:8) && Escort->GetMagazineCapacity()==(British?10:5));
        if (!Ready) continue;
        const auto& Profile=FrontlineWeapons::ProfileFor(Leader);
        Check(Prefix+TEXT("national_pistol_profile_and_complete_reload_asset"),FString(Profile.WeaponId)==(British?TEXT("WebleyMkVI"):TEXT("P08"))
            && !Leader->IsAutomaticWeapon() && Profile.FireRangeCm==float(British?1900:2100)
            && Leader->GetReloadMotion()->GetName()==(British?TEXT("A_Webley_Reload_V24"):TEXT("A_P08_Reload_V24"))
            && Leader->GetWalkMotion()!=Escort->GetWalkMotion());
        for (AWarfareFrontlineSoldier* U:Units)U->SetActorLocation(FVector(U->Team?8000:0,float(U->MemberIndex*80),1288));
        Leader->bMoving=false;Leader->Suppression=0;
        const FVector FarPoint=Leader->Feet()+FVector(6000*EncounterVisibilityScale(),0,0);
        Check(Prefix+TEXT("actual_observer_range_differs_by_nation"),HasForwardObserver(FarPoint,0)==British);
        if(British)
        {
            Leader->bMoving=true;const bool Moving=HasForwardObserver(FarPoint,0);Leader->bMoving=false;Leader->bFieldEntered=false;
            const bool OffMap=HasForwardObserver(FarPoint,0);Leader->bFieldEntered=true;Leader->Suppression=.7f;const bool Suppressed=HasForwardObserver(FarPoint,0);Leader->Suppression=0;
            Check(Prefix+TEXT("moving_offmap_suppressed_officers_do_not_observe"),!Moving && !OffMap && !Suppressed);
        }
        const FVector Station=SoldierDestination(Leader);Leader->SetActorLocation(FVector(Station.X,Station.Y,1288));
        // 0.44f is exactly twice the float representation of 0.22f. This
        // subtraction has an exact expected float, unlike 0.6f-0.22f versus
        // a separately rounded 0.38f literal. Keep a strict effect check.
        Elapsed=20;Escort->SetActorLocation(Leader->GetActorLocation()+FVector(British?1000:650,0,0));Escort->Suppression=.44f;
        const float Health=Escort->Health;const int32 Ammo=Escort->RoundsRemaining;const int32 Orders=LeaderOrders[0];
        UpdateSectionOrders();
        UE_LOG(LogTemp,Display,TEXT("WF_V24_OFFICER_ORDER_STATE faction=%s before=%.9g after=%.9g health_before=%.9g health_after=%.9g ammo_before=%d ammo_after=%d orders_delta=%d phase=%d ready_at=%.9g elapsed=%.9g"),
            *Prefix,double(.44f),double(Escort->Suppression),double(Health),double(Escort->Health),Ammo,Escort->RoundsRemaining,
            LeaderOrders[0]-Orders,int32(Leader->WeaponPhase),double(Leader->CommandReadyAt),double(Elapsed));
        Check(Prefix+TEXT("real_order_removes_pressure_without_healing_or_ammunition"),LeaderOrders[0]==Orders+1
            && Escort->Suppression==.22f && Escort->Health==Health && Escort->RoundsRemaining==Ammo
            && Leader->WeaponPhase==EFrontlineWeaponPhase::OfficerCommand && FMath::IsNearlyEqual(Leader->CommandReadyAt,Elapsed+8));
        const int32 CommandShots=Leader->ShotsDischarged;Leader->Tick(.05f);
        Check(Prefix+TEXT("command_gesture_blocks_weapon_until_full_action_returns"),Leader->WeaponPhase==EFrontlineWeaponPhase::OfficerCommand && Leader->ShotsDischarged==CommandShots);
        for(int32 Frame=0;Frame<110;++Frame){Elapsed+=.02f;Leader->Tick(.02f);}
        Check(Prefix+TEXT("command_action_returns_to_ready"),Leader->WeaponPhase==EFrontlineWeaponPhase::Ready);
        auto* Section=FindSquad(Leader->SquadId);Section->Trench=INDEX_NONE;Section->bAdvancing=true;Leader->bMoving=true;
        Leader->CommandReadyAt=0;Escort->LastSectionOrderAt=-100;Escort->Suppression=.6f;const int32 BeforeAdvance=LeaderOrders[0];UpdateSectionOrders();
        Check(Prefix+TEXT("only_german_officer_orders_during_open_advance"),LeaderOrders[0]==BeforeAdvance+(British?0:1));
        Leader->ReturnToIdle();Section->Trench=Home;Section->bAdvancing=false;Leader->CommandReadyAt=0;Escort->LastSectionOrderAt=-100;Escort->Suppression=.6f;Escort->bFieldEntered=false;
        const float WaitingPressure=Escort->Suppression;UpdateSectionOrders();Escort->bFieldEntered=true;
        Check(Prefix+TEXT("offmap_reinforcement_receives_no_command"),Escort->Suppression==WaitingPressure);
        // Empty the real pistol and exercise its production reload/recoil path.
        Leader->ReturnToIdle();Leader->RoundsRemaining=0;Leader->Suppression=0;Leader->CommandReadyAt=Elapsed+100;
        for (AWarfareFrontlineSoldier* U:Units)if(U->Team==1)U->SetActorLocation(Leader->GetActorLocation()+FVector(GetTeamDirection(0)*(800+U->MemberIndex*70),0,0));
        for(int32 Frame=0;Frame<450;++Frame){Elapsed+=.02f;Leader->Tick(.02f);}
        Check(Prefix+TEXT("actual_pistol_tick_reloads_and_fires_without_rifle_bolt"),Leader->ReloadsStarted>0 && Leader->ReloadsCompleted>0 && Leader->ShotsDischarged>0 && Leader->BoltCyclesCompleted==0);
        Leader->ReturnToIdle();Enemy->SetActorLocation(Leader->GetActorLocation()+FVector(GetTeamDirection(0)*4000,0,0));const int32 ShotsBefore=ShotsFired;Fire(Leader,Enemy);
        Check(Prefix+TEXT("actual_fire_rejects_rifle_distance_for_pistol"),ShotsFired==ShotsBefore);
        Leader->bOfficerSeenReported=false;Enemy->SetActorLocation(Leader->GetActorLocation()+FVector(GetTeamDirection(0)*1000,0,0));
        if(!Enemy->IsAlive())
        {
            // Select an untouched living escort; never revive the target killed by pistol fire.
            Enemy=nullptr;for(AWarfareFrontlineSoldier* U:Units)if(U->Team==1 && U->IsActiveCombatant()){Enemy=U;break;}
        }
        for(AWarfareFrontlineSoldier* U:Units)if(U->Team==0 && U!=Leader)U->SetActorLocation(Leader->GetActorLocation()+FVector(-GetTeamDirection(0)*6000,0,0));
        if(Enemy){Enemy->SetActorLocation(Leader->GetActorLocation()+FVector(GetTeamDirection(0)*1000,0,0));AcquireTarget(Enemy);}
        Check(Prefix+TEXT("actual_enemy_acquisition_reports_visible_officer"),Enemy && Leader->bOfficerSeenReported);
        Escort->SetActorLocation(Leader->GetActorLocation()+FVector(600,0,0));
        Leader->ReceiveHit(500,0,1,FVector(1,0,0),EFrontlineDamageKind::Bullet);Escort->Suppression=.6f;Escort->LastSectionOrderAt=-100;Elapsed+=20;const int32 BeforeDeath=LeaderOrders[0];UpdateSectionOrders();
        Check(Prefix+TEXT("dead_officer_never_issues_new_command"),!Leader->IsAlive() && LeaderOrders[0]==BeforeDeath && FMath::IsNearlyEqual(Escort->Suppression,.6f));
    }
    // Both actual battle setups above have loaded the real audio bank. Run the
    // existing isolated-clock scheduler checks once, and fail explicitly if a
    // missing subsystem or entry guard would otherwise silently skip them.
    auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
    Check(TEXT("audio_world_subsystem_and_actual_voice_bank_ready"),Audio && Audio->GetLoadedVoiceClipCount()>0);
    int32 VoiceChecks=0;bool SawSharpBoundary=false,SawNoLiveMutation=false;
    if(Audio)Audio->RunV20VoiceChecks([&](const FString& Name,bool Result)
    {
        ++VoiceChecks;
        SawSharpBoundary|=Name==TEXT("optional_sharp_delivery_requires_blast_low_probability_and_thirty_six_second_gap");
        SawNoLiveMutation|=Name==TEXT("isolated_scheduler_review_does_not_play_audio_or_mutate_live_cadence");
        Check(Name,Result);
    });
    Check(TEXT("all_eighteen_voice_checks_including_scream_boundary_actually_executed"),VoiceChecks==18 && SawSharpBoundary && SawNoLiveMutation);
    bProgressionVolatile=true;RunV24PacingChecks(Check);
    Report->SetStringField(TEXT("run_id"),RunId);Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());Report->SetBoolField(TEXT("explicit_isolated_fixture"),true);Report->SetBoolField(TEXT("player_profile_touched"),false);
    Report->SetStringField(TEXT("scope"),TEXT("Real stable research purchase/save paths, officer/escort meshes, national pistol reload/fire, observation, command cadence and live/dead/off-map guards. Synthetic points, relocation and local clock. No visual acceptance claim."));
    Report->SetArrayField(TEXT("checks"),Checks);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Checks.Num());Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());
    FString Output;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output));
    FFileHelper::SaveStringToFile(Output,*FPaths::Combine(Dir,TEXT("officer-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(Output,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V24/officer-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FPlatformMisc::RequestExit(false);
}
