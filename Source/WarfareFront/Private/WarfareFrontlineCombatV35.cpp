#include "WarfareFrontlineCombatV35.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportArrivalV30.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineImpact.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

struct FFrontlineCombatV35Review
{
    static bool SetupVisual(AWarfareFrontlineGameMode* B)
    {
        if(!B||!B->bProgressionVolatile||!FParse::Param(FCommandLine::Get(),TEXT("WFV35Review")))return false;
        const FVector Left(-1500,-350,AWarfareFrontlineGround::HeightAt(-1500,-350));
        const FVector Right(1500,-350,AWarfareFrontlineGround::HeightAt(1500,-350));
        auto* Smoke=B->GetWorld()->SpawnActor<AWarfareFrontlineSmoke>(Left,FRotator::ZeroRotator);
        if(!Smoke||!Smoke->bVisualReady){if(Smoke)Smoke->Destroy();return false;}
        Smoke->DeployAt=B->Elapsed-6;Smoke->Tick(0);
        TSet<AFrontlineSupportArrivalV30*> Existing;
        for(TActorIterator<AFrontlineSupportArrivalV30> It(B->GetWorld());It;++It)Existing.Add(*It);
        auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool Configured=Gas&&Gas->Configure(B,EFrontlineSupport::Gas,Right,1);
        // Sample established density at six seconds. Retire this fixture's
        // future delivery actors; do not damage the separate pose-review men.
        for(TActorIterator<AFrontlineSupportArrivalV30> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
        if(!Configured){if(Gas)Gas->Destroy();Smoke->Destroy();return false;}
        Gas->RequestedAt=B->Elapsed-Gas->Arrival-6;Gas->LastTime=Gas->RequestedAt;
        Gas->NextAction=MAX_flt;Gas->Warned=true;Gas->GasBurstMask=7;Gas->Tick(0);
        const FVector Blast(0,600,AWarfareFrontlineGround::HeightAt(0,600));
        if(auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Blast,FRotator::ZeroRotator))Impact->ConfigureBlast(false,false);
        UE_LOG(LogTemp,Display,TEXT("WF_V35_VOLUME_VISUAL_FIXTURE smoke_x=-1500 gas_x=1500 density_age=6 volume_count=12 exposure_disabled=1 cosmetic_snapshot=1"));
        return true;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        const bool Allowed=B&&B->bProgressionVolatile&&FParse::Param(FCommandLine::Get(),TEXT("WFV35Review"));
        Check(TEXT("v35_combat_isolated_volatile_entry"),Allowed);if(!Allowed)return;
        const auto BeforeZones=B->SmokeZones;const auto BeforeUnits=B->Units;const auto BeforeStrikes=B->Strikes;
        const auto BeforeTracers=B->Tracers;const auto BeforeRandom=B->Random;const auto BeforeResearch=B->ActiveResearchMutable();
        const auto BeforePhase=B->Phase;const float BeforeClock=B->Elapsed;const FString BeforeMessage=B->LastMessage;
        const int32 BeforeShots=B->ShotsFired,BeforeMortars=B->MortarRoundsResolved;
        const int32 BeforeC0=B->Casualties[0],BeforeC1=B->Casualties[1];
        const float BeforeM0=B->Morale[0],BeforeM1=B->Morale[1],BeforeD0=B->DamageDealt[0],BeforeD1=B->DamageDealt[1],BeforeFriendly=B->FriendlyFireDamage,BeforePeak=B->PeakSuppression;
        TArray<AActor*> Owned;
        TSet<AWarfareFrontlineImpact*> PreviousImpacts;
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)PreviousImpacts.Add(*It);
        ON_SCOPE_EXIT
        {
            for(AActor* A:Owned)if(IsValid(A))A->Destroy();
            for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)if(!PreviousImpacts.Contains(*It))It->Destroy();
            B->SmokeZones=BeforeZones;B->Units=BeforeUnits;B->Strikes=BeforeStrikes;B->Tracers=BeforeTracers;B->Random=BeforeRandom;B->ActiveResearchMutable()=BeforeResearch;
            B->Phase=BeforePhase;B->Elapsed=BeforeClock;B->LastMessage=BeforeMessage;B->ShotsFired=BeforeShots;B->MortarRoundsResolved=BeforeMortars;
            B->Casualties[0]=BeforeC0;B->Casualties[1]=BeforeC1;B->Morale[0]=BeforeM0;B->Morale[1]=BeforeM1;B->DamageDealt[0]=BeforeD0;B->DamageDealt[1]=BeforeD1;B->FriendlyFireDamage=BeforeFriendly;B->PeakSuppression=BeforePeak;
        };
        B->Phase=EFrontlinePhase::Fighting;B->Elapsed=100;B->SmokeZones.Empty();B->Units.Empty();
        const FVector A(-1500,0,100),C(1500,0,100);FFrontlineSmokeZone Smoke;Smoke.Centre=FVector::ZeroVector;
        Smoke.Radius=FrontlineFireSupportV26::SmokeRadius;Smoke.StartedAt=105;Smoke.ExpiresAt=125;B->SmokeZones.Add(Smoke);
        const bool BeforeArrival=B->DirectFireMultiplier(A,C)==1;
        B->Elapsed=108;const bool Live=B->DirectFireMultiplier(A,C)==.22f&&B->DirectFireMultiplier(C,A)==.22f;
        FFrontlineSmokeZone Gas=Smoke;Gas.bGas=true;B->SmokeZones.Insert(Gas,0);
        const bool Mixed=B->DirectFireMultiplier(A,C)==.22f;
        Swap(B->SmokeZones[0],B->SmokeZones[1]);const bool Reordered=B->DirectFireMultiplier(A,C)==.22f;
        B->Elapsed=125;const bool Expired=B->DirectFireMultiplier(A,C)==1;
        Check(TEXT("v35_smoke_actual_arrival_expiry_and_bidirectional_concealment"),BeforeArrival&&Live&&Expired);
        Check(TEXT("v35_mixed_gas_smoke_uses_strongest_screen_independent_of_array_order"),Mixed&&Reordered);
        B->Elapsed=108;B->SmokeZones={Smoke};
        Check(TEXT("v35_screen_is_spatial_not_global_and_has_wider_frontage"),B->DirectFireMultiplier(FVector(-1500,1040,0),FVector(1500,1040,0))==.22f
            &&B->DirectFireMultiplier(FVector(-1500,1060,0),FVector(1500,1060,0))==1.f);
        auto* Visual=B->GetWorld()->SpawnActor<AWarfareFrontlineSmoke>(FVector(1000,0,0),FRotator::ZeroRotator);Owned.Add(Visual);
        bool VolumeReady=Visual&&Visual->bVisualReady;int32 Volumes=0;
        if(VolumeReady)
        {
            Visual->DeployAt=100;Visual->Tick(0);
            TArray<UStaticMeshComponent*> Components;Visual->GetComponents(Components);
            for(auto* Proxy:Components)if(Proxy->GetStaticMesh())
            {
                ++Volumes;auto* M=Cast<UMaterialInstanceDynamic>(Proxy->GetMaterial(0));
                const auto R=M?M->K2_GetVectorParameterValue(TEXT("VolumeRadii")):FLinearColor::Black;
                VolumeReady&=Proxy->GetStaticMesh()->GetName()==TEXT("Cube")&&M&&M->GetMaterial()->GetName()==TEXT("M_SmokeVolume_V35")
                    &&Proxy->IsVisible()&&R.R>200&&R.G>150&&R.B>100;
            }
        }
        Check(TEXT("v35_smoke_six_actual_depth_volume_bindings"),VolumeReady&&Volumes==FrontlineSmokeVolumeV35::CloudsPerMission);
        B->SmokeZones.Empty();
        auto* Unit=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(FVector(2100,0,88),FRotator::ZeroRotator);Owned.Add(Unit);
        Check(TEXT("v35_combat_soldier_fixture_spawned"),Unit!=nullptr);if(!Unit)return;
        B->ActiveResearchMutable().TechnologyMask&=~(1u<<16);
        Unit->Initialize(B,93501,0,EFrontlineRole::Rifle,0);Unit->SetActorTickEnabled(false);Unit->bFieldEntered=true;B->Units={Unit};
        FFrontlineSmokeZone Exposure;Exposure.bGas=true;Exposure.Centre=Unit->Feet();Exposure.Radius=950;Exposure.StartedAt=B->Elapsed;Exposure.ExpiresAt=B->Elapsed+5;B->SmokeZones.Add(Exposure);
        const float Unprotected=FrontlineFieldV32::GasHitScale(Unit);
        auto HasVisible=[&](const TCHAR* Part)
        {TArray<UMeshComponent*> Meshes;Unit->GetComponents(Meshes);for(auto* M:Meshes)if(M->IsVisible())
            {if(auto* Static=Cast<UStaticMeshComponent>(M);Static&&Static->GetStaticMesh()&&Static->GetStaticMesh()->GetName().Contains(Part))return true;
             if(auto* Skel=Cast<USkeletalMeshComponent>(M);Skel&&Skel->GetSkeletalMeshAsset()&&Skel->GetSkeletalMeshAsset()->GetName().Contains(Part))return true;}return false;};
        Check(TEXT("v35_unresearched_gas_has_no_visible_mask_carrier_or_hidden_protection"),Unprotected==1.f&&!HasVisible(TEXT("Respirator"))&&!HasVisible(TEXT("MaskTin")));
        B->ActiveResearchMutable().TechnologyMask|=1u<<16;const float Protected=FrontlineFieldV32::GasHitScale(Unit);
        Check(TEXT("v35_research_issues_carrier_and_dons_protective_mask_in_gas"),Protected<.1f&&HasVisible(TEXT("Respirator")));
        B->SmokeZones.Empty();FrontlineFieldV32::GasHitScale(Unit);
        bool FaceMaskVisible=false;TArray<USkeletalMeshComponent*> SkelComponents;Unit->GetComponents(SkelComponents);
        for(auto* M:SkelComponents)if(M!=Unit->Soldier&&M->GetSkeletalMeshAsset()&&M->GetSkeletalMeshAsset()->GetName().Contains(TEXT("Respirator")))FaceMaskVisible|=M->IsVisible();
        Check(TEXT("v35_mask_is_removed_when_clear_air_returns"),!FaceMaskVisible);
        B->ActiveResearchMutable().TechnologyMask&=~(1u<<16);Unit->Suppression=0;
        TSet<AFrontlineSupportArrivalV30*> ExistingArrivals;
        for(TActorIterator<AFrontlineSupportArrivalV30> It(B->GetWorld());It;++It)ExistingArrivals.Add(*It);
        auto* GasMission=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();Owned.Add(GasMission);
        const bool GasConfigured=GasMission&&GasMission->Configure(B,EFrontlineSupport::Gas,Unit->Feet(),1);
        for(TActorIterator<AFrontlineSupportArrivalV30> It(B->GetWorld());It;++It)if(!ExistingArrivals.Contains(*It))Owned.Add(*It);
        bool GasSeed=false;
        if(GasConfigured)
        {
            const bool Covered=B->CoverFor(Unit)>0;const float Low=Covered?FrontlineGasV35::CoveredExposureChance:FrontlineGasV35::OpenExposureChance;
            const float OldUngated=Covered?.08f:.045f;
            for(int32 Seed=1;Seed<10000&&!GasSeed;++Seed){FRandomStream Trial(Seed);const float Roll=Trial.FRand();if(Roll>Low&&Roll<OldUngated){GasMission->Random.Initialize(Seed);GasSeed=true;}}
            GasMission->GasBurstMask=7;GasMission->Warned=true;GasMission->NextAction=0;
            if(GasSeed)GasMission->EmitGas(GasMission->Arrival+6);
        }
        Check(TEXT("v35_actual_unmasked_gas_tick_keeps_exposure_budget_and_pressure"),GasConfigured&&GasSeed&&Unit->Health==100&&Unit->Suppression>=.35f&&FrontlineFieldV32::GasHitScale(Unit)==1.f);
        const float CoveredRisk=1-FMath::Pow(1-FrontlineGasV35::CoveredExposureChance,7.f),OpenRisk=1-FMath::Pow(1-FrontlineGasV35::OpenExposureChance,7.f);
        Check(TEXT("v35_full_unprotected_cloud_is_risky_without_routine_squad_erasure"),CoveredRisk>.28f&&CoveredRisk<.33f&&OpenRisk>.16f&&OpenRisk<.20f);
        B->SmokeZones.Empty();Unit->Suppression=0;
        Unit->Role=EFrontlineRole::Lewis;Unit->MemberIndex=0;const auto Gun=FrontlineWeapons::ProfileFor(Unit);
        Unit->MemberIndex=1;const auto Escort=FrontlineWeapons::ProfileFor(Unit);Unit->MemberIndex=0;
        const auto FactionBefore=B->PlayerFaction;B->PlayerFaction=EFrontlineFaction::British;
        const auto ResearchBeforeGun=B->ActiveResearchMutable();B->ActiveResearchMutable().TechnologyMask|=1u<<7;
        Check(TEXT("v35_british_beaten_zone_research_still_extends_upgraded_base"),B->AdjacentPressureRadius(Unit)>FrontlineWeapons::ProfileFor(Unit).AdjacentRadiusCm
            &&FMath::IsNearlyEqual(B->AdjacentPressureRadius(Unit),FrontlineWeapons::ProfileFor(Unit).AdjacentRadiusCm*1.25f,.001f));
        B->ActiveResearchMutable()=ResearchBeforeGun;B->PlayerFaction=FactionBefore;Unit->Role=EFrontlineRole::Rifle;
        Check(TEXT("v35_machinegun_beaten_zone_strength_keeps_rifle_escort_and_deliberate_bursts"),Gun.DirectPressure>=.55f&&Gun.AdjacentPressure>=.16f&&Gun.AdjacentRadiusCm>=360&&Gun.RecoveryMin>=2.6f
            &&Escort.RoundIntervalSeconds==0&&Escort.DirectPressure<Gun.DirectPressure*.3f);
        const float ImpactX=Unit->Feet().X-530;
        FFrontlineStrike Shell;Shell.Team=1;Shell.Target=FVector(ImpactX,0,0);Shell.Radius=FrontlineFireSupportV26::FieldGunBlastRadius;
        const float P=B->ExplosionHitChance(Shell,Unit);bool Seeded=false;
        for(int32 Seed=1;Seed<10000&&!Seeded;++Seed){FRandomStream Trial(Seed);if(Trial.FRand()<P){B->Random.Initialize(Seed);Seeded=true;}}
        Check(TEXT("v35_shell_exposure_extends_beyond_old_450cm_radius"),P>0&&P<=1&&Seeded);
        B->ResolveStrike(Shell);
        Check(TEXT("v35_actual_shell_hit_still_kills_without_wound_state"),Shell.bResolved&&Unit->Health==0&&Unit->GetDeathState().bStarted);
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>(FVector(-5000,0,0),FRotator::ZeroRotator);Owned.Add(Tank);
        const bool Ready=Tank&&Tank->Configure(B,EFrontlineVehicle::MarkIVMale,0,93502);
        Check(TEXT("v35_tank_actual_hull_track_and_mount_durability"),Ready&&Tank->GetHullIntegrity()>=350&&Tank->GetTrackIntegrity(0)==180&&Tank->GetMachinegunIntegrity(0)==60);
        if(Ready)
        {
            const float Hull=Tank->GetHullIntegrity();Tank->ReceiveImpact(Tank->AimPoint(),30,false);
            const bool BulletSafe=Tank->GetHullIntegrity()==Hull;
            Tank->ReceiveModuleHit(EFrontlineTankModule::LeftTrack,106);Tank->ReceiveModuleHit(EFrontlineTankModule::Machinegun,61,0);
            Check(TEXT("v35_bullets_cannot_erode_hull_but_tracks_and_individual_guns_remain_destructible"),BulletSafe&&Tank->IsImmobilized()&&Tank->GetMachinegunIntegrity(0)==0&&Tank->GetMachinegunIntegrity(1)>0&&Tank->IsOperational());
        }
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->RunV35AudioChecks(Check);
    }
};

void RunV35CombatChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCombatV35Review::Run(B,Check);}

bool SetupV35CombatVisualFixture(AWarfareFrontlineGameMode* B)
{return FFrontlineCombatV35Review::SetupVisual(B);}
