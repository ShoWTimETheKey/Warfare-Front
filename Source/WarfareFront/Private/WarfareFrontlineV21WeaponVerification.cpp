#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontline.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

TSharedRef<FJsonObject> RunFrontlineV21WeaponChecks(AWarfareFrontlineGameMode* Battle)
{
    const auto Report=MakeShared<FJsonObject>();
    if (!FParse::Param(FCommandLine::Get(),TEXT("WFV21Review")))
    {
        Report->SetArrayField(TEXT("checks"),TArray<TSharedPtr<FJsonValue>>{});
        Report->SetBoolField(TEXT("success"),false);
        Report->SetStringField(TEXT("error"),TEXT("Weapon fixtures require -WFV21Review; no world state modified."));
        return Report;
    }
    TArray<TSharedPtr<FJsonValue>> Checks,Profiles;
    int32 Passed=0;
    auto Check=[&](const FString& Name,bool Result,const FString& Detail=FString())
    {
        const auto Item=MakeShared<FJsonObject>(); Item->SetStringField(TEXT("name"),Name);
        Item->SetBoolField(TEXT("passed"),Result); Item->SetStringField(TEXT("detail"),Detail);
        Checks.Add(MakeShared<FJsonValueObject>(Item)); Passed+=Result?1:0;
        UE_LOG(LogTemp,Display,TEXT("V21 weapon %s: %s %s"),*Name,Result?TEXT("PASS"):TEXT("FAIL"),*Detail);
    };
    AWarfareFrontlineSoldier* Gunners[2]={}; AWarfareFrontlineSoldier* Guards[2]={};
    if (Battle)
    {
        for (AWarfareFrontlineSoldier* Unit:Battle->Units)
            if (Unit && Unit->IsAlive() && Unit->IsAutomaticWeapon())
                Gunners[Unit->GetFaction()==EFrontlineFaction::British?0:1]=Unit;
        for (int32 Faction=0;Faction<2;++Faction)
            if (Gunners[Faction])
                for (AWarfareFrontlineSoldier* Unit:Battle->Units)
                    if (Unit && Unit->IsAlive() && !Unit->IsAutomaticWeapon() && Unit->Role==EFrontlineRole::Lewis
                        && Unit->SquadId==Gunners[Faction]->SquadId && Unit->Team==Gunners[Faction]->Team)
                    { Guards[Faction]=Unit; break; }
    }
    const bool Ready=Battle && Gunners[0] && Gunners[1] && Guards[0] && Guards[1];
    Check(TEXT("weapon_fixture_has_both_factions_and_actual_rifle_escorts"),Ready);
    if (Ready)
    {
        struct FSavedPosition { AWarfareFrontlineSoldier* Unit; FTransform Transform; };
        TArray<FSavedPosition> Saved;
        for (AWarfareFrontlineSoldier* Unit:Battle->Units) if (Unit) Saved.Add({Unit,Unit->GetActorTransform()});
        // Synchronous call: no world tick can interleave. High-altitude fixture isolates
        // range decisions from terrain collision; all unrelated targets are moved away.
        auto Isolate=[&]()
        {
            for (int32 I=0;I<Saved.Num();++I) Saved[I].Unit->SetActorLocation(FVector(50000.f+I*8000.f,0,12000));
        };
        for (int32 Faction=0;Faction<2;++Faction)
        {
            auto* Gunner=Gunners[Faction]; auto* Guard=Guards[Faction]; auto* Target=Gunners[1-Faction];
            const auto& Gun=FrontlineWeapons::ProfileFor(Gunner);
            const auto& Rifle=FrontlineWeapons::ProfileFor(Guard);
            const FString Prefix=Faction==0?TEXT("british_"):TEXT("german_");
            const auto Profile=MakeShared<FJsonObject>();
            Profile->SetStringField(TEXT("weapon"),Gun.WeaponId); Profile->SetNumberField(TEXT("team"),Gunner->Team);
            Profile->SetNumberField(TEXT("magazine_rounds"),Gunner->GetMagazineCapacity());
            Profile->SetNumberField(TEXT("fire_range_cm"),Gun.FireRangeCm);
            Profile->SetNumberField(TEXT("escort_fire_range_cm"),Rifle.FireRangeCm);
            Profile->SetNumberField(TEXT("advance_contact_cm"),FrontlineWeapons::AdvanceContactRangeFor(Gunner));
            Profile->SetNumberField(TEXT("escort_advance_contact_cm"),FrontlineWeapons::AdvanceContactRangeFor(Guard));
            Profile->SetNumberField(TEXT("round_interval_seconds"),Gun.RoundIntervalSeconds);
            Profile->SetNumberField(TEXT("aimed_rifle_cycle_seconds"),Rifle.AimedCycleSeconds);
            Profile->SetNumberField(TEXT("burst_min"),Gun.BurstMin); Profile->SetNumberField(TEXT("burst_max"),Gun.BurstMax);
            Profile->SetNumberField(TEXT("recovery_min_seconds"),Gun.RecoveryMin); Profile->SetNumberField(TEXT("recovery_max_seconds"),Gun.RecoveryMax);
            Profile->SetNumberField(TEXT("direct_pressure"),Gun.DirectPressure);
            Profile->SetNumberField(TEXT("adjacent_pressure"),Gun.AdjacentPressure);
            Profiles.Add(MakeShared<FJsonValueObject>(Profile));
            Check(Prefix+TEXT("actual_member_and_nationality_select_weapon"),Gunner->MemberIndex==0 && Guard->MemberIndex>0
                && FString(Gun.WeaponId)==(Faction==0?TEXT("Lewis"):TEXT("MG0815"))
                && FString(Rifle.WeaponId)==(Faction==0?TEXT("SMLE"):TEXT("Gewehr98")));
            Check(Prefix+TEXT("physical_feed_capacity_retained"),Gunner->GetMagazineCapacity()==(Faction==0?47:100)
                && Guard->GetMagazineCapacity()==(Faction==0?10:5));
            Check(Prefix+TEXT("automatic_range_pressure_and_burst_window"),Gun.FireRangeCm>Rifle.FireRangeCm*1.3f
                && Gun.DirectPressure>Rifle.DirectPressure*3 && Gun.AdjacentPressure>Rifle.AdjacentPressure*3
                && Gun.RoundIntervalSeconds>0 && Gun.RoundIntervalSeconds<Rifle.AimedCycleSeconds*.1f
                && Gun.RecoveryMin>=2.6f && Gun.RecoveryMax>Gun.RecoveryMin && Gun.BurstMin>=8 && Gun.BurstMax>=Gun.BurstMin,
                TEXT("Runtime-selected configuration; not a measured continuous-fire rate."));

            Isolate(); Gunner->SetActorLocation(FVector(0,0,12000)); Guard->SetActorLocation(FVector(0,80,12000));
            const float Direction=Battle->GetTeamDirection(Gunner->Team);
            const float GuardingDistance=(Rifle.FireRangeCm+Gun.AdvanceContactCm)*.5f;
            Target->SetActorLocation(FVector(Direction*GuardingDistance,0,12000));
            Check(Prefix+TEXT("gunner_targets_beyond_escort_rifle_range"),Battle->AcquireTarget(Gunner)==Target && Battle->AcquireTarget(Guard)==nullptr);
            Check(Prefix+TEXT("escort_halts_with_live_gunner"),Battle->AcquireTarget(Guard,true)==Target
                && FMath::IsNearlyEqual(FrontlineWeapons::AdvanceContactRangeFor(Guard),Gun.AdvanceContactCm));
            const float GunnerHealth=Gunner->Health; Gunner->Health=0;
            const bool EscortReverts=FMath::IsNearlyEqual(FrontlineWeapons::AdvanceContactRangeFor(Guard),Rifle.AdvanceContactCm)
                && Battle->AcquireTarget(Guard,true)==nullptr;
            Gunner->Health=GunnerHealth;
            Check(Prefix+TEXT("surviving_escort_reverts_after_gunner_loss"),EscortReverts,TEXT("Temporary health-state fixture, restored without death effects."));
            Target->SetActorLocation(FVector(Direction*(Gun.FireRangeCm-25),0,12000));
            const bool Within=Battle->AcquireTarget(Gunner)==Target;
            Target->SetActorLocation(FVector(Direction*(Gun.FireRangeCm+25),0,12000));
            Check(Prefix+TEXT("actual_acquire_respects_machinegun_range_boundary"),Within && Battle->AcquireTarget(Gunner)==nullptr);
            Target->SetActorLocation(FVector(Direction*(Rifle.FireRangeCm-25),80,12000));
            const bool RifleWithin=Battle->AcquireTarget(Guard)==Target;
            Target->SetActorLocation(FVector(Direction*(Rifle.FireRangeCm+25),80,12000));
            Check(Prefix+TEXT("actual_acquire_respects_escort_rifle_range_boundary"),RifleWithin && Battle->AcquireTarget(Guard)==nullptr);
        }
        for (const FSavedPosition& Item:Saved) Item.Unit->SetActorTransform(Item.Transform);
        bool Restored=true;
        for (const FSavedPosition& Item:Saved) Restored&=Item.Unit->GetActorTransform().Equals(Item.Transform,.01f);
        Check(TEXT("weapon_fixture_restores_all_unit_transforms"),Restored);
    }
    Report->SetArrayField(TEXT("checks"),Checks); Report->SetArrayField(TEXT("profiles"),Profiles);
    Report->SetNumberField(TEXT("passed"),Passed); Report->SetNumberField(TEXT("total"),Checks.Num());
    Report->SetBoolField(TEXT("success"),Passed==Checks.Num());
    Report->SetStringField(TEXT("scope"),TEXT("Existing live-soldier profile, feed and isolated actual AcquireTarget checks. No natural cadence, animation, damage or match-balance claim."));
    return Report;
}
