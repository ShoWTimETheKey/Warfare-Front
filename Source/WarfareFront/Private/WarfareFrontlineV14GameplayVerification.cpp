#include "WarfareFrontline.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FSpecialistReview
{
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Checks;
    int32 Passed=0;
    bool SmokeOrdered=false,SmokeExpired=false,MortarOrdered=false;
    float SmokeAt=0;
    float FriendlyDamageBeforeMortar=0;
    double Started=FPlatformTime::Seconds();
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Cohort;
    void Check(const TCHAR* Name,bool Pass,const TCHAR* Context=TEXT("API fixture"))
    {
        const auto Entry=MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"),Name); Entry->SetBoolField(TEXT("passed"),Pass); Entry->SetStringField(TEXT("context"),Context);
        Checks.Add(MakeShared<FJsonValueObject>(Entry)); Passed+=Pass?1:0;
        UE_LOG(LogTemp,Display,TEXT("V14 gameplay check %s: %s [%s]"),Name,Pass?TEXT("PASS"):TEXT("FAIL"),Context);
    }
    void Write(AWarfareFrontlineGameMode* Battle)
    {
        Report->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());
        Report->SetNumberField(TEXT("elapsed_battle_seconds"),Battle?Battle->Elapsed:0);
        Report->SetNumberField(TEXT("elapsed_wall_seconds"),FPlatformTime::Seconds()-Started);
        Report->SetNumberField(TEXT("passed"),Passed); Report->SetNumberField(TEXT("total"),Checks.Num());
        Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num()); Report->SetArrayField(TEXT("checks"),Checks);
        const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V14"));
        IFileManager::Get().MakeDirectory(*Directory,true);
        FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*FPaths::Combine(Directory,TEXT("gameplay-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        FPlatformMisc::RequestExit(false);
    }
};
}

void AWarfareFrontlineGameMode::BeginV14GameplayVerification()
{
    // A separate scenario, not the campaign or a replacement for V11/V12 natural
    // combat tests. API setup uses real recruitment; the later firing cohort is
    // explicitly stationed in opposing trenches. No health is boosted or restored,
    // no firing/reload clock is advanced, and support waits its production timer.
    const auto Q=MakeShared<FSpecialistReview>();
    Q->Report->SetStringField(TEXT("run_id"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Q->Report->SetStringField(TEXT("started_utc"),FDateTime::UtcNow().ToIso8601());
    Q->Report->SetBoolField(TEXT("isolated_stationed_cohort_fixture"),true);
    Q->Report->SetBoolField(TEXT("synthetic_command_pressure_fixture"),true);
    Q->Report->SetBoolField(TEXT("synthetic_leader_fatality_fixture"),true);
    Q->Report->SetBoolField(TEXT("stationary_friendly_mortar_target_fixture"),true);
    Q->Report->SetBoolField(TEXT("health_boost_or_restoration"),false);
    Q->Report->SetBoolField(TEXT("accelerated_weapon_or_support_timers"),false);
    Q->Report->SetBoolField(TEXT("player_profile_touched"),!bProgressionVolatile);
    Q->Check(TEXT("verification_profile_is_volatile"),bProgressionVolatile);
    Q->Check(TEXT("all_required_existing_character_assets_loaded"),AssetFailure.IsEmpty() && RoleSoldierMeshes.Num()==6 && V16SoldierMeshes.Num()==12);
    if (!AssetFailure.IsEmpty()) { Q->Write(this); return; }
    Q->Check(TEXT("original_role_ids_preserved_and_new_bounds_valid"),int32(EFrontlineRole::Rifle)==0 && int32(EFrontlineRole::Lewis)==1
        && int32(EFrontlineRole::Mills)==2 && RoleCount==5 && RecruitCooldown(EFrontlineRole::Count)==MAX_flt);
    Q->Check(TEXT("new_supports_reject_briefing"),!RequestSmoke(FVector::ZeroVector) && !RequestMortar(FVector::ZeroVector));
    Q->Check(TEXT("specialists_have_distinct_squad_size_range_and_preparation"),GetSquadSize(EFrontlineRole::Marksman)==2 && GetSquadSize(EFrontlineRole::Officer)==3
        && RoleFireRange(EFrontlineRole::Marksman)>RoleFireRange(EFrontlineRole::Lewis)
        && RoleRecruitSeconds(EFrontlineRole::Marksman)>RoleRecruitSeconds(EFrontlineRole::Rifle));

    for (const EFrontlineFaction Faction:{EFrontlineFaction::British,EFrontlineFaction::German})
        for (const EFrontlineRole Specialist:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Marksman,EFrontlineRole::Officer})
        {
            ResetEncounter(); SelectPlayerFaction(Faction); BeginBattle();
            const int32 LivingBefore=LivingSoldiers(0),NextId=NextSquadId;
            const bool Accepted=Recruit(Specialist);
            bool Shared=Accepted,Models=Accepted,Magazine=Accepted; int32 Specialists=0;
            for (int32 Index=0;Index<RoleCount;++Index)
                Shared&=FMath::IsNearlyEqual(RecruitCooldown(EFrontlineRole(Index)),EffectiveRecruitSeconds(EFrontlineRole(Index)),.001f);
            for (const AWarfareFrontlineSoldier* Unit:Units) if (Unit->Team==0 && Unit->Role==Specialist && Unit->SquadId>=NextId)
            {
                ++Specialists;
                const int32 Visual=Unit->GetPresentationRoleIndex();
                Models&=V16SoldierMeshes.IsValidIndex(Visual) && Unit->Soldier->GetSkeletalMeshAsset()==V16SoldierMeshes[Visual];
                const int32 Expected=Unit->IsAutomaticWeapon()?(Faction==EFrontlineFaction::British?47:100):(Faction==EFrontlineFaction::British?10:5);
                Magazine&=Unit->GetMagazineCapacity()==Expected;
            }
            const FString Prefix=FString::Printf(TEXT("faction_%d_role_%d"),int32(Faction),int32(Specialist));
            Q->Check(*(Prefix+TEXT("_recruits_actual_squad")),Accepted && Specialists==GetSquadSize(Specialist) && LivingSoldiers(0)-LivingBefore==Specialists);
            Q->Check(*(Prefix+TEXT("_resets_all_five_shared_counters")),Shared && !Recruit(EFrontlineRole::Rifle));
            Q->Check(*(Prefix+TEXT("_uses_valid_national_equipment")),Models && Magazine);
        }

    ResetEncounter(); SelectPlayerFaction(EFrontlineFaction::German); ResetEncounter();
    // Three sections per trench, using the exact production spawn/formation path.
    SpawnSquad(0,EFrontlineRole::Marksman,0); SpawnSquad(0,EFrontlineRole::Officer,0);
    SpawnSquad(1,EFrontlineRole::Lewis,1); SpawnSquad(1,EFrontlineRole::Marksman,1);
    Trenches[1].Owner=1;
    for (FFrontlineSquad& Section:Squads) if (Section.Team==1 && Section.Trench==2)
    { Section.Trench=Section.Destination=1; Section.Slot=2; }
    for (AWarfareFrontlineSoldier* Unit:Units)
    { Unit->SetActorLocation(SoldierDestination(Unit)+FVector(0,0,88)); Q->Cohort.Add(Unit); }
    BeginBattle(); EnemyThinkAt=MAX_flt; // Explicit stationary firing fixture; no AI reinforcements.
    Q->Check(TEXT("support_rejects_initial_preparation"),!RequestSmoke(FVector::ZeroVector) && !RequestMortar(FVector::ZeroVector)
        && FMath::IsNearlyEqual(SmokeCooldown(),12.f) && FMath::IsNearlyEqual(MortarCooldown(),45.f));
    Q->Check(TEXT("invalid_roles_and_teams_reject_without_array_access"),!Recruit(EFrontlineRole::Count) && !Recruit(EFrontlineRole(255))
        && !Recruit(EFrontlineRole::Marksman,-1) && !RequestMortar(FVector::ZeroVector,2) && MortarCooldown(-1)==MAX_flt);

    AWarfareFrontlineSoldier* Leader=nullptr; AWarfareFrontlineSoldier* Recipient=nullptr; AWarfareFrontlineSoldier* Marksman=nullptr;
    for (AWarfareFrontlineSoldier* Unit:Units) if (Unit->Team==0)
    {
        if (Unit->IsSectionLeader()) Leader=Unit;
        if (Unit->Role==EFrontlineRole::Marksman) { Marksman=Unit; Recipient=Unit; }
    }
    const auto* Priority=Marksman?AcquireTarget(Marksman):nullptr;
    Q->Check(TEXT("marksman_selects_visible_machinegun_over_riflemen"),Priority && Priority->IsAutomaticWeapon(),TEXT("stationed target-selection fixture"));
    if (Leader && Recipient)
    {
        // Move only the fixture leader closer inside his occupied trench; this does
        // not provide a health or range advantage to the subsequent weapon checks.
        const FVector Original=Leader->GetActorLocation(); Leader->SetActorLocation(Recipient->GetActorLocation()+FVector(0,100,0));
        Leader->CommandReadyAt=0;
        Recipient->ReceiveHit(0,.50f,1);
        const float HealthBefore=Recipient->Health,PressureBefore=Recipient->Suppression;
        UpdateSectionOrders(); const int32 Orders=LeaderOrders[0];
        Q->Check(TEXT("living_nearby_nco_gives_bounded_pressure_relief_without_healing"),Orders==1 && FMath::IsNearlyEqual(PressureBefore-Recipient->Suppression,.12f,.001f)
            && Recipient->Health==HealthBefore && Morale[0]==100.f,TEXT("synthetic pressure fixture"));
        const float After=Recipient->Suppression; UpdateSectionOrders();
        Q->Check(TEXT("nco_order_cannot_repeat_or_stack_immediately"),LeaderOrders[0]==Orders && Recipient->Suppression==After,TEXT("synthetic pressure fixture"));
        Leader->ReceiveHit(200.f,0.f,1,Leader->Feet()-Recipient->Feet(),EFrontlineDamageKind::Bullet);
        Leader->CommandReadyAt=0; Recipient->LastSectionOrderAt=-12.f;
        UpdateSectionOrders();
        Q->Check(TEXT("fallen_nco_cannot_continue_orders_or_heal_section"),!Leader->IsAlive() && LeaderOrders[0]==Orders
            && Recipient->Suppression==After && Recipient->Health==HealthBefore,TEXT("explicit leader fatality and command-eligibility fixture"));
        if (Leader->IsAlive()) Leader->SetActorLocation(Original);
    }
    else Q->Check(TEXT("nco_fixture_spawned_required_units"),false);
    Q->Check(TEXT("instant_rally_support_remains_unavailable"),!RequestRally(FVector(-3300,0,0)) && RallyCommands==0);
    const TWeakObjectPtr<AWarfareFrontlineGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis,Q](float)
    {
        AWarfareFrontlineGameMode* B=WeakThis.Get(); if (!B) { Q->Check(TEXT("world_remains_available"),false); Q->Write(nullptr); return false; }
        if (!Q->SmokeOrdered && B->Elapsed>=12.05f)
        {
            Q->SmokeOrdered=true; Q->SmokeAt=B->Elapsed;
            const FVector Screen(-1650,0,0); const float Before=B->SmokeCooldown();
            B->SetPaused(true); const bool PausedRejected=!B->RequestSmoke(Screen); B->SetPaused(false);
            Q->Check(TEXT("smoke_rejects_pause_without_spending_preparation"),PausedRejected && B->SmokeCooldown()==Before);
            const bool InvalidRejected=!B->RequestSmoke(FVector(7000,0,0));
            Q->Check(TEXT("smoke_invalid_point_keeps_preparation"),InvalidRejected && B->SmokeCooldown()==Before);
            Q->Check(TEXT("smoke_commits_visual_and_only_its_own_preparation"),B->RequestSmoke(Screen) && B->SmokeZones.Num()==1
                && FMath::IsNearlyEqual(B->SmokeCooldown(),30.f,.01f) && B->MortarCooldown()>30.f);
            const FVector A(-3300,0,0),C(0,0,0);
            Q->Check(TEXT("smoke_attenuates_both_directions_but_not_clear_lines"),FMath::IsNearlyEqual(B->DirectFireMultiplier(A,C),.4f)
                && FMath::IsNearlyEqual(B->DirectFireMultiplier(C,A),.4f) && B->DirectFireMultiplier(FVector(-3300,1500,0),FVector(0,1500,0))==1.f);
            Q->Check(TEXT("smoke_immediate_repeat_rejected"),!B->RequestSmoke(Screen) && B->SmokeCommands==1);
        }
        if (Q->SmokeOrdered && !Q->SmokeExpired && B->Elapsed>=Q->SmokeAt+12.15f)
        {
            Q->SmokeExpired=true;
            Q->Check(TEXT("smoke_expires_after_real_lifetime"),B->SmokeZones.IsEmpty() && B->DirectFireMultiplier(FVector(-3300,0,0),FVector::ZeroVector)==1.f,TEXT("real-time support observation"));
            Q->Check(TEXT("natural_shots_record_smoke_interference"),B->SmokeAffectedShots[0]>0 && B->SmokeAffectedShots[1]>0,TEXT("natural firing in stationed cohort"));
        }
        if (!Q->MortarOrdered && B->Elapsed>=45.05f)
        {
            Q->MortarOrdered=true;
            const float OtherPreparation=B->ArtilleryCooldown();
            Q->Check(TEXT("mortar_invalid_point_does_not_consume_preparation"),!B->RequestMortar(FVector(0,900,0)) && B->MortarCooldown()==0);
            Q->Check(TEXT("mortar_orders_three_fixed_staggered_rounds"),B->RequestMortar(FVector(0,0,0)) && B->Strikes.Num()==3
                && B->Strikes[0].bMortar && B->Strikes[0].Radius==210.f && B->Strikes[1].ImpactAt>B->Strikes[0].ImpactAt
                && B->Strikes[2].ImpactAt>B->Strikes[1].ImpactAt && !B->RequestMortar(FVector::ZeroVector));
            Q->Check(TEXT("mortar_preserves_artillery_preparation_and_zero_wallets"),B->ArtilleryCooldown()==OtherPreparation
                && FMath::IsNearlyEqual(B->MortarCooldown(),45.f,.01f) && B->CommandPoints[0]==0 && B->Supplies[0]==0);
            Q->FriendlyDamageBeforeMortar=B->FriendlyFireDamage;
            // A separate, visibly identified friendly casualty fixture is placed at
            // the first impact. It has normal 100 HP and receives the real timed
            // blast; neither the observed firing cohort nor the damage is boosted.
            if (B->Strikes.Num()>0)
            {
                FActorSpawnParameters Params; Params.Name=FName(TEXT("V14_StationaryFriendlyMortarFixture"));
                Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (auto* Friendly=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(B->Strikes[0].Target+FVector(0,0,88),FRotator::ZeroRotator,Params))
                {
                    Friendly->Initialize(B,INDEX_NONE,0,EFrontlineRole::Rifle,0);
                    Friendly->SetActorTickEnabled(false); B->Units.Add(Friendly);
                }
            }
        }
        if (B->Elapsed<57.f && FPlatformTime::Seconds()-Q->Started<95.f) return true;
        Q->Check(TEXT("real_time_support_scenario_completed"),Q->SmokeExpired && Q->MortarOrdered && B->MortarRoundsResolved==3,TEXT("real-time support observation"));
        Q->Check(TEXT("mortar_applies_actual_friendly_fire"),Q->MortarOrdered && B->FriendlyFireDamage>Q->FriendlyDamageBeforeMortar,TEXT("normal-HP stationary friendly target fixture"));
        Q->Report->SetNumberField(TEXT("mortar_friendly_damage"),B->FriendlyFireDamage-Q->FriendlyDamageBeforeMortar);
        TArray<TSharedPtr<FJsonValue>> Actors;
        int32 MarksmanShots[2]={0,0},MarksmanReloads=0,OfficerShots=0; bool ActionsValid=true; float ShortestMarksmanInterval=MAX_flt;
        for (const auto& WeakUnit:Q->Cohort) if (const AWarfareFrontlineSoldier* Unit=WeakUnit.Get())
        {
            const auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("actor"),Unit->GetName());
            Row->SetNumberField(TEXT("role"),int32(Unit->Role)); Row->SetNumberField(TEXT("faction"),int32(Unit->GetFaction()));
            Row->SetNumberField(TEXT("shots"),Unit->ShotsDischarged); Row->SetNumberField(TEXT("reloads_completed"),Unit->ReloadsCompleted);
            Row->SetNumberField(TEXT("bolt_cycles"),Unit->BoltCyclesCompleted); Row->SetNumberField(TEXT("health"),Unit->Health);
            Row->SetNumberField(TEXT("magazine_capacity"),Unit->GetMagazineCapacity()); Row->SetNumberField(TEXT("remaining_rounds"),Unit->RoundsRemaining);
            Row->SetStringField(TEXT("mesh"),Unit->Soldier->GetSkeletalMeshAsset()?Unit->Soldier->GetSkeletalMeshAsset()->GetName():TEXT("missing"));
            Actors.Add(MakeShared<FJsonValueObject>(Row));
            ActionsValid&=Unit->ShotsDuringReload==0 && Unit->ShotsWhileMoving==0 && Unit->RoundsRemaining>=0
                && Unit->RoundsRemaining<=Unit->GetMagazineCapacity() && Unit->ReloadsCompleted<=Unit->ReloadsStarted;
            if (Unit->Role==EFrontlineRole::Marksman)
            {
                MarksmanShots[int32(Unit->GetFaction())]+=Unit->ShotsDischarged; MarksmanReloads+=Unit->ReloadsCompleted;
                for (int32 I=1;I<Unit->ShotTimes.Num();++I) ShortestMarksmanInterval=FMath::Min(ShortestMarksmanInterval,Unit->ShotTimes[I]-Unit->ShotTimes[I-1]);
            }
            if (Unit->Role==EFrontlineRole::Officer) OfficerShots+=Unit->ShotsDischarged;
        }
        Q->Report->SetArrayField(TEXT("natural_cohort"),Actors);
        Q->Report->SetNumberField(TEXT("marksman_shortest_shot_interval"),ShortestMarksmanInterval==MAX_flt?-1:ShortestMarksmanInterval);
        Q->Report->SetNumberField(TEXT("mortar_rounds_resolved"),B->MortarRoundsResolved);
        Q->Report->SetNumberField(TEXT("leader_orders"),B->LeaderOrders[0]);
        Q->Check(TEXT("both_factions_marksmen_fire_actual_bolt_action_rifles"),MarksmanShots[0]>=4 && MarksmanShots[1]>=4,TEXT("natural firing in stationed cohort"));
        Q->Check(TEXT("marksman_preserves_deliberate_cadence"),ShortestMarksmanInterval!=MAX_flt && ShortestMarksmanInterval>=4.35f,TEXT("natural firing in stationed cohort"));
        Q->Check(TEXT("marksman_completes_an_actual_magazine_reload"),MarksmanReloads>0,TEXT("natural firing in stationed cohort"));
        Q->Check(TEXT("nco_section_also_fires_rifles"),OfficerShots>0,TEXT("natural firing in stationed cohort"));
        Q->Check(TEXT("new_units_obey_movement_reload_and_magazine_locks"),ActionsValid,TEXT("natural firing in stationed cohort"));
        Q->Check(TEXT("new_supports_never_create_negative_or_spent_wallets"),B->CommandPoints[0]==0 && B->CommandPoints[1]==0 && B->CommandSpent[0]==0 && B->CommandSpent[1]==0);
        Q->Write(B); return false;
    }),.10f);
}
