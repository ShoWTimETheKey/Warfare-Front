#include "WarfareFrontline.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::BeginV23MechanicsVerification()
{
    if (!FParse::Param(FCommandLine::Get(),TEXT("WFV23MechanicsVerify")) || !bProgressionVolatile) return;
    SetActorTickEnabled(false);
    const FString RunId=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V23"),RunId);
    IFileManager::Get().MakeDirectory(*Dir,true);
    const auto Report=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Checks; int32 Passed=0;
    auto Check=[&](const TCHAR* Name,bool Result)
    {
        const auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(Row));if(Result)++Passed;
        UE_LOG(LogTemp,Display,TEXT("WF_V23_MECHANICS %s %s"),Name,Result?TEXT("PASS"):TEXT("FAIL"));
    };
    // All synthetic points, relocation, clock advances and lethal hits below are
    // limited to this explicit fixture and its random Saved/Gameplay file.
    ProgressionFile=FPaths::Combine(Dir,TEXT("isolated-profile.json"));bProgressionVolatile=false;bProgressionReadOnly=false;
    CampaignRuns.Empty();ActiveCampaignId.Invalidate();RewardedEncounterIds.Empty();ArmyProgress[0]={};ArmyProgress[1]={};ResetEncounter();
    Check(TEXT("fresh_campaign_has_two_points_zero_training"),StartNewCampaign(EFrontlineFaction::British) && GetResearchPoints()==2 && GetActiveResearch().TechnologyMask==0);
    const FGuid First=ActiveCampaignId;
    Check(TEXT("purchase_uses_actual_prerequisite_and_price"),!PurchaseTechnology(12) && PurchaseTechnology(0) && GetResearchPoints()==1 && !PurchaseTechnology(0));
    Check(TEXT("second_same_faction_campaign_starts_fresh"),StartNewCampaign(EFrontlineFaction::British) && ActiveCampaignId!=First && GetResearchPoints()==2 && !HasTechnology(0));
    const FGuid Second=ActiveCampaignId;
    Check(TEXT("continuing_first_restores_only_its_research"),ContinueCampaign(First) && GetResearchPoints()==1 && HasTechnology(0));
    LoadProgression(true);
    Check(TEXT("schema4_roundtrip_keeps_independent_research"),!bProgressionReadOnly && ActiveCampaignId==First && GetResearchPoints()==1 && ContinueCampaign(Second) && GetResearchPoints()==2 && !HasTechnology(0));

    // Re-create the real schema3 layout, then run the production migration.
    FString Json;TSharedPtr<FJsonObject> Legacy;
    bool LegacyReady=FFileHelper::LoadFileToString(Json,*ProgressionFile) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Legacy);
    if (LegacyReady)
    {
        Legacy->SetNumberField(TEXT("schema"),3);
        Legacy->GetObjectField(TEXT("British"))->SetNumberField(TEXT("research_points"),9);
        Legacy->GetObjectField(TEXT("British"))->SetNumberField(TEXT("technology_mask"),63);
        for(const auto& Value:Legacy->GetArrayField(TEXT("campaigns")))Value->AsObject()->RemoveField(TEXT("research"));
        FJsonSerializer::Serialize(Legacy,TJsonWriterFactory<>::Create(&Json));LegacyReady=FFileHelper::SaveStringToFile(Json,*ProgressionFile);
        LoadProgression(true);
    }
    FString Backup;const bool Preserved=FFileHelper::LoadFileToString(Backup,*(ProgressionFile+TEXT(".pre-v23"))) && Backup==Json;
    Check(TEXT("schema3_original_retained_and_each_old_line_gets_snapshot"),LegacyReady && Preserved && !bProgressionReadOnly
        && ContinueCampaign(First) && GetResearchPoints()==9 && GetActiveResearch().TechnologyMask==63
        && ContinueCampaign(Second) && GetResearchPoints()==9 && GetActiveResearch().TechnologyMask==63);
    Check(TEXT("new_campaign_after_migration_never_inherits_old_mask"),StartNewCampaign(EFrontlineFaction::British) && GetResearchPoints()==2 && GetActiveResearch().TechnologyMask==0);
    Check(TEXT("both_national_trees_have_sixteen_stable_ids"),GetTechnologyNodesForFaction(EFrontlineFaction::British).Num()==16
        && GetTechnologyNodesForFaction(EFrontlineFaction::German).Num()==16 && GetTechnologyNodes()[8].Cost==3 && GetTechnologyNodes()[12].Prerequisite==1);
    auto BuyAll=[&]()
    {
        ActiveResearchMutable().ResearchPoints=100; // labelled research-budget fixture, never a user archive.
        for(int32 Pass=0;Pass<4;++Pass)for(int32 I=0;I<16;++I)if(!HasTechnology(I) && TechnologyBlockReason(I).IsEmpty())PurchaseTechnology(I);
        return GetActiveResearch().TechnologyMask==65535;
    };
    auto Member=[&](int32 Team,EFrontlineRole InRole,int32 Index)->AWarfareFrontlineSoldier*
    {for(AWarfareFrontlineSoldier* U:Units)if(U->Team==Team && U->Role==InRole && U->MemberIndex==Index)return U;return nullptr;};
    Check(TEXT("complete_british_tree_purchased_through_real_graph"),BuyAll());
    BeginBattle();SpawnSquad(0,EFrontlineRole::Marksman,GetHomeTrenchIndex());SpawnSquad(0,EFrontlineRole::Lewis,GetHomeTrenchIndex());SpawnSquad(1,EFrontlineRole::Officer,GetHomeTrenchIndex(1));
    auto* Sniper=Member(0,EFrontlineRole::Marksman,0);auto* Gun=Member(0,EFrontlineRole::Lewis,0);auto* Bearer=Member(0,EFrontlineRole::Lewis,1);auto* Reserve=Member(0,EFrontlineRole::Lewis,2);auto* EnemyLeader=Member(1,EFrontlineRole::Officer,0);
    Check(TEXT("scoped_recruit_has_real_mesh_and_independent_rifle_profile"),Sniper && Sniper->IsScopedMarksman() && !Sniper->IsAutomaticWeapon()
        && Sniper->Soldier->GetSkeletalMeshAsset()->GetName()==TEXT("SK_British_ScopedMarksman_V23")
        && FrontlineWeapons::ProfileFor(Sniper).FireRangeCm==8500 && Sniper->GetMagazineCapacity()==10);
    if(Sniper && EnemyLeader)
    {
        for(AWarfareFrontlineSoldier* U:Units)if(U->Team==1)U->SetActorLocation(Sniper->GetActorLocation()+FVector(1600+U->MemberIndex*100,0,0));
        Check(TEXT("counter_observer_uses_actual_acquisition_priority"),AcquireTarget(Sniper)==EnemyLeader);
    }
    if(Gun && Bearer && Reserve)
    {
        const float HealthBefore=Bearer->Health;Gun->RoundsRemaining=17;
        Check(TEXT("living_gunner_cannot_be_taken_over"),!Bearer->RecoverMachineGunFrom(Gun));
        Gun->ReceiveHit(500,0,1,FVector(1,0,0),EFrontlineDamageKind::Bullet);
        Bearer->SetActorLocation(Gun->GetActorLocation()+FVector(-300,0,0));
        const FVector Before=Bearer->Feet();const bool Walking=Bearer->UpdateMachineGunRecovery(.1f);
        Check(TEXT("assistant_physically_approaches_before_taking_gun"),Walking && FVector::Dist2D(Before,Bearer->Feet())>0 && !Bearer->IsMachineGunner() && !Bearer->RecoverMachineGunFrom(Gun));
        Bearer->SetActorLocation(Gun->GetActorLocation()+FVector(-65,0,0));Bearer->UpdateMachineGunRecovery(.01f);
        Check(TEXT("gun_pickup_has_nonzero_handling_delay"),!Bearer->IsMachineGunner() && Bearer->GunRecoveryReadyAt>Elapsed+2.f);
        Elapsed+=2.5f;Bearer->UpdateMachineGunRecovery(.01f);
        Check(TEXT("one_surviving_assistant_takes_real_pan_without_healing"),Bearer->IsMachineGunner() && Bearer->bRecoveredMachineGun
            && Bearer->RoundsRemaining==17 && Bearer->Health==HealthBefore && !Reserve->IsMachineGunner()
            && Bearer->Soldier->GetSkeletalMeshAsset()->GetName()==TEXT("SK_British_Machinegun_V16"));
        Bearer->ReceiveHit(500,0,1,FVector(1,0,0),EFrontlineDamageKind::Bullet);
        Reserve->SetActorLocation(Bearer->GetActorLocation());Reserve->UpdateMachineGunRecovery(.01f);
        Reserve->ReceiveHit(500,0,1,FVector(1,0,0),EFrontlineDamageKind::Bullet);Elapsed+=3;
        Check(TEXT("dead_assistant_never_completes_recovery_or_resurrects"),!Reserve->UpdateMachineGunRecovery(.1f) && !Reserve->IsAlive() && !Reserve->bRecoveredMachineGun);
    }
    RestartBattle();StartNewCampaign(EFrontlineFaction::German);Check(TEXT("complete_german_tree_purchased_independently"),BuyAll());
    BeginBattle();SpawnSquad(0,EFrontlineRole::Mills,GetHomeTrenchIndex());auto* Early=Member(0,EFrontlineRole::Mills,0);
    Check(TEXT("1917_doctrine_never_issues_mp18"),GetEncounterYear()==1917 && !HasStormtroopers() && Early && !Early->IsStormtrooper() && !Early->IsAutomaticWeapon() && Early->GetMagazineCapacity()==5);
    RestartBattle();
    auto* Current=CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& R){return R.Id==ActiveCampaignId;});
    if(Current)Current->CompletedMissions=4; // Explicit unlocked-1918 fixture, not a fabricated victory.
    Check(TEXT("1918_is_a_real_selectable_bound_campaign_mission"),SelectCampaignMission(4) && GetSelectedCampaignMission() && GetSelectedCampaignMission()->Year==1918);
    BeginBattle();SpawnSquad(0,EFrontlineRole::Mills,GetHomeTrenchIndex());SpawnSquad(1,EFrontlineRole::Rifle,GetHomeTrenchIndex(1));
    auto* Storm=Member(0,EFrontlineRole::Mills,0);auto* Target=Member(1,EFrontlineRole::Rifle,0);
    Check(TEXT("1918_recruit_has_actual_mp18_mesh_drum_and_short_range_profile"),Storm && Storm->IsStormtrooper() && Storm->IsAutomaticWeapon() && !Storm->IsMachineGunner()
        && Storm->GetMagazineCapacity()==32 && Storm->Soldier->GetSkeletalMeshAsset()->GetName()==TEXT("SK_German_Stormtrooper_V23")
        && FrontlineWeapons::ProfileFor(Storm).RoundIntervalSeconds==.15f && FrontlineWeapons::ProfileFor(Storm).FireRangeCm==2800
        && Storm->GetReloadMotion() && Storm->GetReloadMotion()->GetName()==TEXT("A_MP18_Reload_V23"));
    if(Storm && Target)
    {
        for(AWarfareFrontlineSoldier* U:Units)if(U->Team==1)U->SetActorLocation(Storm->GetActorLocation()+FVector(-1700-U->MemberIndex*80,0,0));
        Storm->RoundsRemaining=0; // Exercise actual empty-drum reload, not its duration formula.
        for(int32 Frame=0;Frame<800;++Frame){Elapsed+=.02f;Storm->Tick(.02f);}
        int32 BurstIntervals=0;for(int32 I=1;I<Storm->ShotTimes.Num();++I)if(Storm->ShotTimes[I]-Storm->ShotTimes[I-1]<.25f)++BurstIntervals;
        Check(TEXT("actual_soldier_tick_reloads_drum_and_fires_bursts_without_bolting"),Storm->ReloadsStarted>0 && Storm->ReloadsCompleted>0 && Storm->ShotsDischarged>=4 && BurstIntervals>=2 && Storm->BoltCyclesCompleted==0);
        const int32 ShotsBefore=ShotsFired;Target->SetActorLocation(Storm->GetActorLocation()+FVector(-4000,0,0));Fire(Storm,Target);
        Check(TEXT("actual_fire_rejects_out_of_range_mp18"),ShotsFired==ShotsBefore);
    }
    const bool AddedRearGun=SpawnSquad(0,EFrontlineRole::Lewis,GetHomeTrenchIndex());
    const int32 RearGunId=AddedRearGun?Squads.Last().Id:INDEX_NONE;
    const int32 StormId=Storm?Storm->SquadId:INDEX_NONE;
    const bool AssaultOrder=AddedRearGun && AdvanceTrench(GetHomeTrenchIndex(),0);
    Check(TEXT("german_rearguard_keeps_machinegun_but_never_detains_mp18_assault"),AssaultOrder
        && FindSquad(RearGunId) && !FindSquad(RearGunId)->bAdvancing && FindSquad(StormId) && FindSquad(StormId)->bAdvancing);
    Report->SetStringField(TEXT("run_id"),RunId);Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
    Report->SetBoolField(TEXT("explicit_isolated_fixture"),true);Report->SetBoolField(TEXT("player_profile_touched"),false);
    Report->SetStringField(TEXT("fixture_scope"),TEXT("Actual campaign save/load/migration, purchases, equipment and soldier recovery/fire paths. Synthetic research budget, unlocked 1918 node, unit positions, local simulation clock and lethal test hits. No visual or whole-battle performance claim."));
    Report->SetArrayField(TEXT("checks"),Checks);Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("total"),Checks.Num());Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());
    Report->SetStringField(TEXT("isolated_profile"),FPaths::ConvertRelativePathToFull(ProgressionFile));
    FString Output;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output));
    FFileHelper::SaveStringToFile(Output,*FPaths::Combine(Dir,TEXT("mechanics-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(Output,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V23/mechanics-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FPlatformMisc::RequestExit(false);
}
