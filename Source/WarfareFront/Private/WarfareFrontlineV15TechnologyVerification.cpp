#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::BeginV15TechnologyVerification()
{
    // Isolated causal fixtures: balances, readiness, positions, suppression and
    // one reload phase are explicitly arranged. Production purchase/save/fire/
    // support/order paths run unchanged. No player profile or natural QA is used.
    const FString Run=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V15"));
    IFileManager::Get().MakeDirectory(*Directory,true);
    const auto Report=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Checks;
    int32 Passed=0;
    auto Check=[&](const FString& Name,bool Result)
    {
        const auto Item=MakeShared<FJsonObject>(); Item->SetStringField(TEXT("name"),Name); Item->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(Item)); Passed+=Result?1:0;
        UE_LOG(LogTemp,Display,TEXT("V15 technology %s: %s"),*Name,Result?TEXT("PASS"):TEXT("FAIL"));
    };
    ProgressionFile=FPaths::Combine(Directory,FString::Printf(TEXT("technology-fixture-%s.json"),*Run));
    bProgressionVolatile=false; bProgressionReadOnly=false; RewardedEncounterIds.Empty();
    PlayerFaction=EFrontlineFaction::British; ArmyProgress[0]={}; ArmyProgress[1]={}; ResetEncounter();
    constexpr uint32 CompleteMask=(1u<<12)-1,LegacyMask=(1u<<6)-1;
    const int32 PurchaseOrder[]={0,2,4,3,1,7,5,10,6,8,9,11};
    for (int32 National=0;National<2;++National)
    {
        const FString Prefix=National==0?TEXT("british_"):TEXT("german_");
        Check(Prefix+TEXT("select_faction"),SelectPlayerFaction(EFrontlineFaction(National)));
        ArmyProgress[National].ResearchPoints=100; // Labelled research-balance fixture.
        const auto& Nodes=GetTechnologyNodes(); bool Graph=Nodes.Num()==12; TSet<FString> Art; TSet<int32> Cells;
        for (int32 Id=0;Id<Nodes.Num();++Id)
        {
            const auto& Node=Nodes[Id]; Graph&=Node.Id==Id && Node.Branch>=0 && Node.Branch<4 && Node.Tier>=0 && Node.Tier<3;
            Graph&=Node.Cost>0 && FCString::Strlen(Node.Title)>0 && FCString::Strlen(Node.Effect)>0;
            Art.Add(Node.ArtKey); Cells.Add(Node.Branch*3+Node.Tier);
            for (int32 Required:{Node.Prerequisite,Node.SecondaryPrerequisite})
                Graph&=Required==INDEX_NONE || (Nodes.IsValidIndex(Required) && Nodes[Required].Tier<Node.Tier);
        }
        Check(Prefix+TEXT("twelve_named_unique_art_nodes_in_acyclic_four_by_three_graph"),Graph && Art.Num()==12 && Cells.Num()==12);
        Check(Prefix+TEXT("locked_cross_branch_purchase_preserves_points"),!PurchaseTechnology(11) && GetResearchPoints()==100 && !HasTechnology(11));
        Check(Prefix+TEXT("artillery_root_available_without_machinegun_training"),Nodes[3].Prerequisite==INDEX_NONE && TechnologyBlockReason(3).IsEmpty());
        ArmyProgress[National].TechnologyMask=(1u<<3)|(1u<<10);
        Check(Prefix+TEXT("purchase_requires_secondary_assault_branch"),!PurchaseTechnology(11) && GetResearchPoints()==100 && !HasTechnology(11));
        ArmyProgress[National].TechnologyMask=(1u<<4);
        Check(Prefix+TEXT("purchase_requires_primary_observation_branch"),!PurchaseTechnology(11) && GetResearchPoints()==100 && !HasTechnology(11));
        ArmyProgress[National].TechnologyMask=0;
        Check(Prefix+TEXT("invalid_node_ids_do_not_mutate_progress"),!PurchaseTechnology(-1) && !PurchaseTechnology(12) && GetResearchPoints()==100);
        int32 Expected=100;
        for (int32 Id:PurchaseOrder)
        {
            Expected-=Nodes[Id].Cost;
            Check(Prefix+FString::Printf(TEXT("purchase_%02d_debits_exactly_once"),Id),PurchaseTechnology(Id)
                && HasTechnology(Id) && GetResearchPoints()==Expected && !PurchaseTechnology(Id) && GetResearchPoints()==Expected);
        }
        Check(Prefix+TEXT("full_tree_and_enemy_isolation"),ArmyProgress[National].TechnologyMask==CompleteMask
            && !HasTechnology(0,1) && !HasTechnology(11,1) && GetResearchPoints()==74);
    }
    Check(TEXT("both_national_full_trees_survive_disk_readback"),SaveProgression());
    LoadProgression(true);
    Check(TEXT("schema2_reads_all_twenty_four_nodes_without_cross_army_leak"),!bProgressionReadOnly
        && PlayerFaction==EFrontlineFaction::German && ArmyProgress[0].TechnologyMask==CompleteMask && ArmyProgress[1].TechnologyMask==CompleteMask
        && ArmyProgress[0].ResearchPoints==74 && ArmyProgress[1].ResearchPoints==74);

    // Construct a real V13-format document in the isolated UUID file.
    FString Stored; TSharedPtr<FJsonObject> Legacy;
    FFileHelper::LoadFileToString(Stored,*ProgressionFile);
    const bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Stored),Legacy) && Legacy.IsValid();
    Check(TEXT("isolated_profile_document_readable"),Parsed);
    if (Parsed)
    {
        Legacy->SetNumberField(TEXT("schema"),1);
        const FString Receipt=FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Legacy->SetArrayField(TEXT("rewarded_encounters"),{MakeShared<FJsonValueString>(Receipt)});
        for (int32 Army=0;Army<2;++Army)
        {
            const auto National=Legacy->GetObjectField(Army==0?TEXT("British"):TEXT("German"));
            National->SetNumberField(TEXT("technology_mask"),LegacyMask); National->SetNumberField(TEXT("research_points"),Army==0?7:11);
            National->SetNumberField(TEXT("victories"),Army+2); National->SetNumberField(TEXT("defeats"),Army+1);
        }
        FString LegacyText; FJsonSerializer::Serialize(Legacy,TJsonWriterFactory<>::Create(&LegacyText));
        FFileHelper::SaveStringToFile(LegacyText,*ProgressionFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        LoadProgression(true);
        Check(TEXT("schema1_migrates_all_six_paid_bits_balances_results_and_receipt"),!bProgressionReadOnly
            && ArmyProgress[0].TechnologyMask==LegacyMask && ArmyProgress[1].TechnologyMask==LegacyMask
            && ArmyProgress[0].ResearchPoints==7 && ArmyProgress[1].ResearchPoints==11
            && ArmyProgress[0].Victories==2 && ArmyProgress[1].Defeats==2 && RewardedEncounterIds.Contains(Receipt));
        Check(TEXT("legacy_bonuses_retained_without_new_nodes_granted"),RifleAccuracyScale(0)==1.1f && WeaponReloadScale(0)==.85f
            && MachinegunPressureScale(0)==1.18f && GrenadeHitChanceScale(0)==1.12f && !HasTechnology(6) && !HasTechnology(11));
        Check(TEXT("legacy_profile_can_commit_current_schema_without_spending_points"),SaveProgression());
        FFileHelper::LoadFileToString(Stored,*ProgressionFile);
        TSharedPtr<FJsonObject> Current; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Stored),Current);
        Check(TEXT("migrated_disk_schema_is_three"),Current.IsValid() && Current->GetIntegerField(TEXT("schema"))==3);
        if (Current.IsValid())
        {
            // Removing either required branch must reject a schema2 record.
            const int32 InvalidMasks[]={ (1<<0)|(1<<1)|(1<<6), (1<<2)|(1<<6), (1<<12) };
            for (int32 I=0;I<UE_ARRAY_COUNT(InvalidMasks);++I)
            {
                Current->GetObjectField(TEXT("German"))->SetNumberField(TEXT("technology_mask"),InvalidMasks[I]);
                FString Invalid; FJsonSerializer::Serialize(Current,TJsonWriterFactory<>::Create(&Invalid));
                FFileHelper::SaveStringToFile(Invalid,*ProgressionFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
                LoadProgression(true); FString After; FFileHelper::LoadFileToString(After,*ProgressionFile);
                Check(FString::Printf(TEXT("invalid_schema2_graph_%d_is_readonly_and_original_bytes_preserved"),I),bProgressionReadOnly && Invalid==After && !PurchaseTechnology(0));
            }
            FFileHelper::SaveStringToFile(Stored,*ProgressionFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            LoadProgression(true);
        }
    }

    bProgressionVolatile=true; bProgressionReadOnly=false;
    auto Fixture=[&](EFrontlineFaction Faction)
    {
        PlayerFaction=Faction; ArmyProgress[int32(Faction)].TechnologyMask=CompleteMask;
        ResetEncounter(); BeginBattle(); Elapsed=20.f;
    };
    auto UnitOf=[&](int32 Team,EFrontlineRole InRole,int32 Member=0)->AWarfareFrontlineSoldier*
    {
        for (AWarfareFrontlineSoldier* Unit:Units) if (Unit->Team==Team && Unit->Role==InRole && Unit->MemberIndex==Member && Unit->IsAlive()) return Unit;
        return nullptr;
    };
    auto Toggle=[&](int32 Id,bool On) { auto& Mask=ArmyProgress[int32(PlayerFaction)].TechnologyMask; if (On) Mask|=1u<<Id; else Mask&=~(1u<<Id); };
    auto Place=[](AWarfareFrontlineSoldier* Unit,FVector Feet)
    { Feet.Z=AWarfareFrontlineGround::HeightAt(Feet.X,Feet.Y); Unit->SetActorLocation(Feet+FVector(0,0,88)); };
    auto PressureShot=[&](AWarfareFrontlineSoldier* Shooter,AWarfareFrontlineSoldier* Target)
    { Target->Suppression=0; Fire(Shooter,Target); return Target->Suppression; };

    Fixture(EFrontlineFaction::British);
    Check(TEXT("british_fixture_recruits_actual_machinegun_and_officer_squads"),SpawnSquad(0,EFrontlineRole::Lewis,0) && SpawnSquad(0,EFrontlineRole::Officer,0));
    auto* Gun=UnitOf(0,EFrontlineRole::Lewis); auto* Runner=UnitOf(0,EFrontlineRole::Rifle); auto* Enemy=UnitOf(1,EFrontlineRole::Rifle);
    auto* Companion=UnitOf(0,EFrontlineRole::Lewis,1); auto* Leader=UnitOf(0,EFrontlineRole::Officer);
    if (Gun && Runner && Enemy && Companion && Leader)
    {
        Place(Runner,Gun->Feet()+FVector(800,0,0)); Runner->bMoving=true;
        Check(TEXT("british_covering_fire_requires_actual_recent_mg_fire"),IncomingBulletPressureScale(Runner)==1.f);
        Fire(Gun,Enemy);
        Toggle(6,false); const float BasePressure=PressureShot(Enemy,Runner); Toggle(6,true);
        const float CoveredPressure=PressureShot(Enemy,Runner);
        Check(TEXT("british_06_real_incoming_fire_pressure_reduced_35_percent"),BasePressure>0 && FMath::IsNearlyEqual(CoveredPressure,BasePressure*.65f,.0001f));
        Elapsed+=2.1f;
        Check(TEXT("british_covering_fire_expires_without_further_fire"),IncomingBulletPressureScale(Runner)==1.f);
        Runner->bMoving=false;
        auto* Adjacent=UnitOf(1,EFrontlineRole::Rifle,1);
        if (Adjacent)
        {
            Place(Adjacent,Enemy->Feet()+FVector(0,300,0)); Adjacent->Suppression=0;
            Toggle(7,false); Fire(Gun,Enemy); const float Narrow=Adjacent->Suppression;
            Toggle(7,true); Fire(Gun,Enemy);
            Check(TEXT("british_07_actual_mg_fire_reaches_additional_adjacent_enemy"),Narrow==0 && Adjacent->Suppression>0);
        }
        Gun->WeaponPhase=EFrontlineWeaponPhase::Reload; // Explicit reload-state fixture, not an animation completion claim.
        Toggle(8,false); const float CompanionBase=PressureShot(Companion,Enemy); Toggle(8,true);
        const float CompanionBoost=PressureShot(Companion,Enemy);
        Check(TEXT("british_08_companion_fire_pressure_changes_during_mg_reload"),FMath::IsNearlyEqual(CompanionBoost,CompanionBase*1.8f,.0001f));
        Gun->WeaponPhase=EFrontlineWeaponPhase::Ready;
        Check(TEXT("british_companion_bonus_ends_when_mg_reload_ends"),OutgoingPressureScale(Companion,Enemy)==1.f);
        Place(Companion,Leader->Feet()+FVector(0,100,0)); Companion->Suppression=.5f; Companion->LastSectionOrderAt=-12;
        const float HealthBefore=Companion->Health; Leader->CommandReadyAt=Elapsed; UpdateSectionOrders();
        Check(TEXT("british_09_actual_section_order_relief_and_eight_second_interval"),FMath::IsNearlyEqual(Companion->Suppression,.28f,.0001f)
            && FMath::IsNearlyEqual(Leader->CommandReadyAt-Elapsed,8.f,.0001f) && Companion->Health==HealthBefore);
        const int32 Orders=LeaderOrders[0]; Leader->ReceiveHit(1000,0,1); Companion->Suppression=.5f; Elapsed+=9;
        UpdateSectionOrders();
        Check(TEXT("dead_british_nco_cannot_issue_upgraded_orders_or_heal"),LeaderOrders[0]==Orders && Companion->Suppression==.5f && Companion->Health==HealthBefore);
    }
    else Check(TEXT("british_causal_fixture_units_available"),false);

    Fixture(EFrontlineFaction::British); SpawnSquad(0,EFrontlineRole::Marksman,0);
    auto* Observer=UnitOf(0,EFrontlineRole::Marksman);
    auto* PartnerObserver=UnitOf(0,EFrontlineRole::Marksman,1);
    if (Observer && PartnerObserver)
    {
        const FVector Point=Observer->Feet(); ArtilleryReadyAt[0]=Elapsed; MortarReadyAt[0]=Elapsed;
        Check(TEXT("british_10_actual_observed_artillery_arrives_at_2_4_seconds"),RequestArtillery(Point) && FMath::IsNearlyEqual(Strikes.Last().ImpactAt-Elapsed,2.4f,.0001f));
        Strikes.Empty();
        Check(TEXT("british_10_actual_observed_mortar_first_round_arrives_at_1_8_seconds"),RequestMortar(Point) && FMath::IsNearlyEqual(Strikes[0].ImpactAt-Elapsed,1.8f,.0001f));
        // The marksman squad has two real riflemen. Removing one observer must
        // not suppress the other man's valid contribution. For each negative
        // eligibility check, make both candidates ineligible explicitly.
        Observer->bMoving=true;
        Check(TEXT("stationary_partner_still_observes_when_first_marksman_moves"),HasForwardObserver(Point,0));
        PartnerObserver->bMoving=true;
        Check(TEXT("moving_observer_is_ineligible"),!HasForwardObserver(Point,0));
        Observer->bMoving=false; PartnerObserver->bMoving=false;
        Observer->Suppression=.6f;
        Check(TEXT("unpressured_partner_still_observes_when_first_marksman_is_suppressed"),HasForwardObserver(Point,0));
        PartnerObserver->Suppression=.6f;
        Check(TEXT("suppressed_observer_is_ineligible"),!HasForwardObserver(Point,0));
        Observer->Suppression=0; PartnerObserver->Suppression=0;
        Check(TEXT("british_smoke_order_creates_real_visual_and_zone"),RequestSmoke(Point));
        Check(TEXT("smoke_blocks_observer_and_remains_bidirectional"),!HasForwardObserver(Point,0)
            && DirectFireMultiplier(Point,Point+FVector(1500,0,0))==.4f && DirectFireMultiplier(Point+FVector(1500,0,0),Point)==.4f);
        ArtilleryReadyAt[0]=Elapsed; Strikes.Empty();
        Check(TEXT("british_11_smoke_artillery_schedules_two_separated_real_strikes"),RequestArtillery(Point) && Strikes.Num()==2
            && FMath::IsNearlyEqual(Strikes[1].ImpactAt-Strikes[0].ImpactAt,1.f,.0001f));
        const float BeforeArtilleryFriendlyDamage=FriendlyFireDamage;
        int32 ArtilleryResolved=0;
        for (FFrontlineStrike& Strike:Strikes)
        {
            // Explicit impact-time fixture: use the actual queued objects and
            // production ResolveStrike, not a second implementation of damage.
            Elapsed=Strike.ImpactAt; ResolveStrike(Strike); ArtilleryResolved+=Strike.bResolved?1:0;
        }
        Check(TEXT("british_11_both_queued_artillery_rounds_actually_resolve_with_friendly_damage"),ArtilleryResolved==2 && FriendlyFireDamage>BeforeArtilleryFriendlyDamage);
        MortarReadyAt[0]=Elapsed; Strikes.Empty();
        Check(TEXT("british_11_smoke_mortar_schedules_four_real_rounds"),RequestMortar(Point) && Strikes.Num()==4);
        const int32 BeforeResolvedMortars=MortarRoundsResolved;
        int32 ResolvedMortars=0; float ExtraRoundDamage=0;
        for (int32 I=0;I<Strikes.Num();++I)
        {
            AWarfareFrontlineSoldier* Probe=I==3?UnitOf(0,EFrontlineRole::Rifle):nullptr;
            // A still-living ordinary rifleman enters the fourth impact area only
            // after the first three rounds; health is never replenished.
            if (Probe) Place(Probe,Strikes[I].Target);
            const float Before=Probe?Probe->Health:0.f;
            Elapsed=Strikes[I].ImpactAt; ResolveStrike(Strikes[I]);
            ResolvedMortars+=Strikes[I].bResolved?1:0;
            if (Probe) ExtraRoundDamage=Before-Probe->Health;
        }
        Check(TEXT("british_11_four_real_mortar_resolutions_include_damaging_extra_round"),ResolvedMortars==4
            && MortarRoundsResolved-BeforeResolvedMortars==4 && ExtraRoundDamage>0);
        if (!SmokeZones.IsEmpty()) SmokeZones[0].Team=1;
        MortarReadyAt[0]=Elapsed; Strikes.Empty();
        Check(TEXT("enemy_smoke_cannot_grant_british_extra_round"),RequestMortar(Point) && Strikes.Num()==3);
    }
    else Check(TEXT("two_observer_fixture_available"),false);

    Fixture(EFrontlineFaction::German); SpawnSquad(0,EFrontlineRole::Lewis,0);
    auto* Rifle=UnitOf(0,EFrontlineRole::Rifle); Enemy=UnitOf(1,EFrontlineRole::Rifle); Gun=UnitOf(0,EFrontlineRole::Lewis);
    if (Rifle && Enemy && Gun)
    {
        Enemy->bMoving=true; Toggle(6,false); const float Base=PressureShot(Rifle,Enemy); Toggle(6,true);
        Check(TEXT("german_06_real_entrenched_rifle_fire_suppresses_moving_enemy_more"),FMath::IsNearlyEqual(PressureShot(Rifle,Enemy),Base*1.4f,.0001f));
        Enemy->bMoving=false; Check(TEXT("german_blocking_fire_requires_advancing_target"),OutgoingPressureScale(Rifle,Enemy)==1.f);
        int32 HitSeed=1; for (;HitSeed<1000;++HitSeed) { FRandomStream Trial(HitSeed); if (Trial.FRand()<.05f) break; }
        Toggle(7,false); Random.Initialize(HitSeed); const float Before=Gun->Health; const float BasePressure=PressureShot(Enemy,Gun); const float BaseDamage=Before-Gun->Health;
        Toggle(7,true); Random.Initialize(HitSeed); const float BeforeBoost=Gun->Health; const float BoostPressure=PressureShot(Enemy,Gun);
        Check(TEXT("german_07_real_mg_incoming_damage_and_pressure_reduced_25_percent"),BaseDamage>0
            && FMath::IsNearlyEqual(BeforeBoost-Gun->Health,BaseDamage*.75f,.001f) && FMath::IsNearlyEqual(BoostPressure,BasePressure*.75f,.0001f));
        const int32 RifleId=Rifle->SquadId,GunId=Gun->SquadId;
        Check(TEXT("german_08_advance_leaves_mg_and_moves_infantry"),AdvanceTrench(0)
            && FindSquad(RifleId)->bAdvancing && FindSquad(GunId)->Trench==0 && !FindSquad(GunId)->bAdvancing);
        Check(TEXT("german_rearguard_can_advance_on_next_order"),AdvanceTrench(0) && FindSquad(GunId)->bAdvancing);
    }
    else Check(TEXT("german_causal_fixture_units_available"),false);

    Fixture(EFrontlineFaction::German); SpawnSquad(0,EFrontlineRole::Lewis,0);
    Gun=UnitOf(0,EFrontlineRole::Lewis); Companion=UnitOf(0,EFrontlineRole::Lewis,1);
    if (Gun && Companion)
    {
        const int32 OrphanedGunSquad=Gun->SquadId;
        Gun->ReceiveHit(1000,0,1); // Actual fatal hit; escorts retain their current health.
        Check(TEXT("german_08_dead_gunner_does_not_hold_rifle_companions_in_trench"),!Gun->IsAlive() && Companion->IsAlive()
            && AdvanceTrench(0) && FindSquad(OrphanedGunSquad)->bAdvancing && FindSquad(OrphanedGunSquad)->Trench==INDEX_NONE
            && !LastMessage.Contains(TEXT("留守")));
    }
    else Check(TEXT("german_dead_gunner_fixture_available"),false);
    Fixture(EFrontlineFaction::German);
    Check(TEXT("german_08_no_machinegun_does_not_claim_a_rearguard"),AdvanceTrench(0) && !LastMessage.Contains(TEXT("留守")));

    Fixture(EFrontlineFaction::German); SpawnSquad(0,EFrontlineRole::Mills,0);
    auto* Assault=UnitOf(0,EFrontlineRole::Mills); Enemy=UnitOf(1,EFrontlineRole::Rifle);
    if (Assault && Enemy)
    {
        RequestSmoke(Assault->Feet()); Assault->bMoving=true; Toggle(9,false); const float Base=PressureShot(Enemy,Assault); Toggle(9,true);
        Check(TEXT("german_09_actual_incoming_fire_halved_in_friendly_smoke"),FMath::IsNearlyEqual(PressureShot(Enemy,Assault),Base*.5f,.0001f));
        if (!SmokeZones.IsEmpty()) SmokeZones[0].Team=1;
        Check(TEXT("german_infiltration_requires_friendly_smoke"),IncomingBulletPressureScale(Assault)==1.f && AssaultContactScale(Assault)==1.f);
        // Open-ground acquisition fixture: 600 cm lies between the ordinary
        // 750 cm stop range and the researched 450 cm range. Keep the actual
        // terrain trace and all target selection filters active.
        Place(Assault,FVector(-1800,100,0)); Place(Enemy,FVector(-1200,100,0));
        if (!SmokeZones.IsEmpty()) { SmokeZones[0].Team=0; SmokeZones[0].Centre=Assault->Feet(); }
        Toggle(9,false); AWarfareFrontlineSoldier* OrdinaryContact=AcquireTarget(Assault,true);
        Toggle(9,true); AWarfareFrontlineSoldier* InfiltratingContact=AcquireTarget(Assault,true);
        Check(TEXT("german_09_actual_acquisition_allows_advancing_past_600cm_contact"),HasSight(Assault,Enemy)
            && OrdinaryContact==Enemy && InfiltratingContact==nullptr);
        Place(Enemy,Assault->Feet()+FVector(440,0,0));
        Check(TEXT("german_09_actual_acquisition_still_stops_at_close_enemy"),HasSight(Assault,Enemy) && AcquireTarget(Assault,true)==Enemy);
        Place(Enemy,Assault->Feet()+FVector(600,0,0));
        if (!SmokeZones.IsEmpty()) SmokeZones[0].Team=1;
        Check(TEXT("enemy_smoke_does_not_extend_german_advance_contact_gate"),AcquireTarget(Assault,true)==Enemy);
    }
    else Check(TEXT("german_assault_fixture_available"),false);

    Fixture(EFrontlineFaction::German); MortarReadyAt[0]=Elapsed; Toggle(10,false); const FVector Centre(0,0,0);
    const bool BaselineOrdered=RequestMortar(Centre); const auto Baseline=Strikes; Strikes.Empty(); MortarReadyAt[0]=Elapsed; MortarCommands=0; Toggle(10,true);
    bool Tightened=BaselineOrdered && RequestMortar(Centre) && Strikes.Num()==3 && Baseline.Num()==3;
    if (Tightened) for (int32 I=0;I<3;++I) Tightened&=FMath::IsNearlyEqual(float(FVector::Dist2D(Strikes[I].Target,Centre)),float(FVector::Dist2D(Baseline[I].Target,Centre))*.6f,.001f)
        && Strikes[I].Radius==Baseline[I].Radius && Strikes[I].ImpactAt==Baseline[I].ImpactAt;
    Check(TEXT("german_10_actual_mortar_scatter_shrinks_without_radius_or_timing_change"),Tightened);

    Fixture(EFrontlineFaction::German);
    // Place three ordinary 100-HP actors in owned trenches; radius=1 isolates each
    // normal ResolveStrike damage calculation without boosting health or reviving.
    auto* BaseTarget=UnitOf(1,EFrontlineRole::Rifle,0); auto* BoostTarget=UnitOf(1,EFrontlineRole::Rifle,1); auto* Friendly=UnitOf(0,EFrontlineRole::Rifle);
    if (BaseTarget && BoostTarget && Friendly)
    {
        for (FFrontlineSquad& S:Squads) if (S.Team==1) { S.Trench=2; S.bAdvancing=false; }
        auto GrenadeDamage=[&](AWarfareFrontlineSoldier* Target)
        {
            FFrontlineStrike Strike; Strike.Team=0; Strike.bGrenade=true; Strike.Target=Target->Feet(); Strike.Radius=1.f;
            Target->Suppression=.6f; const float Before=Target->Health; ResolveStrike(Strike); return Before-Target->Health;
        };
        Toggle(11,false); const float Base=GrenadeDamage(BaseTarget); Toggle(11,true);
        const float Boost=GrenadeDamage(BoostTarget),FriendlyDamage=GrenadeDamage(Friendly);
        Check(TEXT("german_11_actual_grenade_damage_vs_suppressed_trench_enemy_increases_35_percent"),Base>0 && FMath::IsNearlyEqual(Boost,Base*1.35f,.001f));
        Check(TEXT("german_assault_bonus_does_not_remove_or_discount_friendly_fire"),FMath::IsNearlyEqual(FriendlyDamage,Base,.001f) && FriendlyFireDamage>0);
    }
    else Check(TEXT("german_strike_fixture_available"),false);

    Report->SetStringField(TEXT("run_id"),Run); Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
    Report->SetStringField(TEXT("scope"),TEXT("V15 isolated technology graph, migration, and causal gameplay fixtures; not natural-play or animation-quality evidence."));
    Report->SetBoolField(TEXT("player_profile_touched"),false);
    Report->SetBoolField(TEXT("synthetic_research_readiness_positions_suppression_reload_phase"),true);
    Report->SetBoolField(TEXT("health_boosted_or_dead_actors_revived"),false);
    Report->SetStringField(TEXT("isolated_profile"),FPaths::ConvertRelativePathToFull(ProgressionFile));
    Report->SetArrayField(TEXT("checks"),Checks); Report->SetNumberField(TEXT("passed"),Passed); Report->SetNumberField(TEXT("total"),Checks.Num());
    Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());
    FString Output; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output));
    FFileHelper::SaveStringToFile(Output,*FPaths::Combine(Directory,TEXT("technology-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FPlatformMisc::RequestExit(false);
}
