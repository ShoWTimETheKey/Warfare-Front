#include "WarfareFrontlineCombatV63.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineSupportV28.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlineBalanceV50.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

struct FFrontlineCombatV63Review
{
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        const bool Isolated=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&(FParse::Param(FCommandLine::Get(),TEXT("WFV63Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV63Combat")))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
        Check(TEXT("v63_combat_requires_disposable_no_save_profile"),Isolated);if(!Isolated)return;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        Check(TEXT("v63_vehicle_service_available"),Fleet!=nullptr);if(!Fleet)return;
        const auto OldUnits=B->Units;const auto OldVehicles=Fleet->Vehicles;
        const auto OldSquads=B->Squads;const auto OldStrikes=B->Strikes;const auto OldTracers=B->Tracers;const auto OldSmoke=B->SmokeZones;
        const auto OldPhase=B->Phase;const auto OldMessage=B->LastMessage;
        const auto OldEncounter=Fleet->LastEncounter;const auto OldRandom=B->Random;
        const float OldTime=B->Elapsed,OldPressure=B->PeakSuppression,OldFriendly=B->FriendlyFireDamage;
        const int32 OldOutro=B->BattleOutroWinner,OldShots=B->ShotsFired,OldGrenades=B->GrenadesThrown,OldSerial=Fleet->NextSerial;
        const float OldMorale[2]={B->Morale[0],B->Morale[1]},OldDamage[2]={B->DamageDealt[0],B->DamageDealt[1]};
        const int32 OldCasualties[2]={B->Casualties[0],B->Casualties[1]},OldDeployments[2]={Fleet->DeploymentCountV37[0],Fleet->DeploymentCountV37[1]};
        struct FActorState {AActor* Actor;bool Hidden,Collision;};TArray<FActorState> ActorStates;
        auto Hide=[&](AActor* A){if(IsValid(A)){ActorStates.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
        for(AWarfareFrontlineSoldier* U:OldUnits)Hide(U);for(AFrontlineTank* T:OldVehicles)Hide(T);
        TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
        B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->Tracers.Empty();B->SmokeZones.Empty();Fleet->Vehicles.Empty();
        B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;
        Fleet->LastEncounter=B->GetEncounterId();Fleet->NextSerial=63000;
        ON_SCOPE_EXIT
        {
            for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
            for(const auto& S:ActorStates)if(IsValid(S.Actor)){S.Actor->SetActorHiddenInGame(S.Hidden);S.Actor->SetActorEnableCollision(S.Collision);}
            B->Units=OldUnits;B->Squads=OldSquads;B->Strikes=OldStrikes;B->Tracers=OldTracers;B->SmokeZones=OldSmoke;
            B->Phase=OldPhase;B->LastMessage=OldMessage;B->Elapsed=OldTime;B->BattleOutroWinner=OldOutro;B->Random=OldRandom;
            B->ShotsFired=OldShots;B->GrenadesThrown=OldGrenades;B->PeakSuppression=OldPressure;B->FriendlyFireDamage=OldFriendly;
            Fleet->Vehicles=OldVehicles;Fleet->LastEncounter=OldEncounter;Fleet->NextSerial=OldSerial;
            for(int32 T=0;T<2;++T){B->Morale[T]=OldMorale[T];B->DamageDealt[T]=OldDamage[T];B->Casualties[T]=OldCasualties[T];Fleet->DeploymentCountV37[T]=OldDeployments[T];}
        };
        auto Make=[&](EFrontlineRole Role,int32 Team,int32 Id,const FVector& Feet,int32 Member=0)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;
            U->SetActorLocation(Feet+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
            U->SetMotion(U->GetAimMotion());U->Soldier->TickAnimation(.3f,false);U->Soldier->RefreshBoneTransforms();
            U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->RoundsRemaining=U->GetMagazineCapacity();B->Units.Add(U);return U;
        };
        const int32 Dir=B->GetTeamDirection(0);
        auto* Grenadier=Make(EFrontlineRole::RifleGrenadier,0,630001,FVector(-Dir*1300,0,10000));
        auto* Enemy=Make(EFrontlineRole::Rifle,1,630002,FVector(Dir*1300,0,10000));
        auto* Tank=Fleet->Spawn(EFrontlineVehicle::A7V,1,true);
        Check(TEXT("v63_production_grenadier_and_armour_assets_spawn"),Grenadier&&Enemy&&Tank);if(!Grenadier||!Enemy||!Tank)return;
        Tank->bEntered=true;Tank->HeadingYawV37=Dir>0?180:0;Tank->SetActorLocationAndRotation(FVector(Dir*1000,0,10000),FRotator(0,Tank->HeadingYawV37,0));
        const int32 Before=B->Strikes.Num();Grenadier->DischargeWeapon(nullptr,Tank);
        Check(TEXT("v63_rifle_grenadier_fires_real_delayed_armour_round_and_spends_ammunition"),B->Strikes.Num()==Before+1&&Grenadier->ShotsDischarged==1
            &&Grenadier->RoundsRemaining==0&&B->Strikes.Last().bRifleLaunched&&B->Strikes.Last().bGrenade&&B->Strikes.Last().ImpactAt>B->Elapsed&&B->Strikes.Last().Projectile.IsValid());
        for(auto& S:B->Strikes)if(S.Projectile.IsValid())S.Projectile->Destroy();B->Strikes.Empty();

        // This uses the production module path, not a formula-only assertion.
        Tank->TrackIntegrity[0]=Tank->TrackIntegrity[1]=180;const float Hull=Tank->HullIntegrity;
        const FVector Close=Tank->GetActorTransform().TransformPosition(FVector(0,-190,35));
        Fleet->ApplyGrenadeBlast(Close,FrontlineGrenade::BlastRadiusCm,0);
        Check(TEXT("v63_close_scattered_bomb_contact_damages_running_gear_without_one_shotting_armour"),Tank->TrackIntegrity[0]<180
            &&Tank->TrackIntegrity[0]>=138.f&&Tank->HullIntegrity>=Hull-12.f&&Tank->IsOperational());
        const float Track=Tank->TrackIntegrity[0],HullAfter=Tank->HullIntegrity;
        Fleet->ApplyGrenadeBlast(Tank->GetActorTransform().TransformPosition(FVector(0,-900,35)),FrontlineGrenade::BlastRadiusCm,0);
        Check(TEXT("v63_fragmentation_does_not_damage_distant_tanks"),Tank->TrackIntegrity[0]==Track&&Tank->HullIntegrity==HullAfter);

        // A local rear contact is eligible everywhere, not only at a victory line.
        Enemy->SetActorLocation(Grenadier->GetActorLocation()-FVector(Dir*600,0,0));
        Check(TEXT("v63_passing_close_infantry_is_reacquired_before_marching_on"),B->AcquireTarget(Grenadier,true)==Enemy);
        Enemy->SetActorLocation(Grenadier->GetActorLocation()-FVector(Dir*1800,0,0));
        Check(TEXT("v63_local_rear_reaction_does_not_grant_mapwide_reverse_chasing"),B->AcquireTarget(Grenadier,true)==nullptr);

        // A real friendly infantry screen cannot mask an opposing hull.
        Enemy->SetActorLocation(FVector(Dir*3000,1200,10000));
        auto* Screen=Make(EFrontlineRole::Rifle,0,630003,FVector(0,0,10000));
        Check(TEXT("v63_armour_acquired_through_friendly_rifle_screen"),Screen&&Fleet->AcquireInfantryTarget(Grenadier)==Tank);
        Grenadier->SetActorLocation(FVector(-Dir*5000,2500,10088));if(Screen)Screen->SetActorLocation(FVector(-Dir*5000,3000,10088));

        // Two legal gun arcs must work independently, even if the nearest man
        // stands behind a forward mount. Immobilising the fixture isolates aim.
        Tank->TrackIntegrity[0]=0;Tank->SetActorLocationAndRotation(FVector(0,0,10000),FRotator(0,0,0));Tank->HeadingYawV37=0;
        for(auto& C:Tank->CannonClock)C=1000;
        auto* Rear=Make(EFrontlineRole::Rifle,0,630004,FVector(-850,-150,10000));
        auto* Front=Make(EFrontlineRole::Rifle,0,630005,FVector(1300,500,10000));
        if(Rear&&Front)
        {
            B->Units={Rear,Front};for(float& C:Tank->GunClock)C=0;
            for(int32 I=0;I<16;++I){Rear->Health=Front->Health=100;Tank->Simulate(.025f);}
            Check(TEXT("v63_tank_side_mounts_engage_front_and_rear_contacts_independently"),Rear->Suppression>0&&Front->Suppression>0&&Tank->MachinegunShots>=2);
        }
        else Check(TEXT("v63_tank_side_mounts_engage_front_and_rear_contacts_independently"),false);
        Fleet->Vehicles.Empty();Tank->SetActorEnableCollision(false);B->Units.Empty();

        auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const FVector Aim(0,0,AWarfareFrontlineGround::HeightAt(0,0));
        const bool AirReady=Air&&Air->Configure(B,EFrontlineSupport::AirStrike,Aim,0);
        Check(TEXT("v63_real_air_mission_configures"),AirReady);
        if(AirReady)
        {
            FVector Low(MAX_flt),High(-MAX_flt);bool Forward=true,Deferred=true;
            for(int32 Pass=0;Pass<2;++Pass)for(int32 Frame=0;Frame<=160;++Frame)
            {
                const float Time=Air->Arrival+Pass*12.f+1.15f+Frame*.01f;B->Elapsed=Air->RequestedAt+Time;
                const int32 Old=Air->RoundsFired;Air->Strafe(Time);
                if(Air->RoundsFired==Old)continue;
                const auto& Round=Air->PendingStrafe.Last();Low=Low.ComponentMin(Round.Point);High=High.ComponentMax(Round.Point);
                const auto Flight=FrontlineSupportArtV40::AttackFlight(Aim,Dir*(Pass? -1:1),Time-Air->Arrival-Pass*12.f,Pass);
                Forward&=FVector::DotProduct((Round.Point-Round.Origin).GetSafeNormal(),Flight.Rotation.Vector())>.998f;
                Deferred&=Round.ImpactAt>B->Elapsed&&Round.ImpactAt-B->Elapsed<.2f;
            }
            Check(TEXT("v63_two_visible_strafing_passes_cover_broad_committed_lanes"),Air->RoundsFired>=36&&Air->RoundsFired<=44
                &&High.X-Low.X>2000.f&&High.Y-Low.Y>700.f&&Forward&&Deferred);
            if(!Air->PendingStrafe.IsEmpty())
            {
                auto Round=Air->PendingStrafe[Air->PendingStrafe.Num()/2];Air->PendingStrafe={Round};
                auto* Victim=Make(EFrontlineRole::Rifle,1,630006,Round.Point);
                if(Victim)
                {
                    for(int32 Seed=1;Seed<100;++Seed){FRandomStream R(Seed);if(R.FRand()<.05f){Air->PendingStrafe[0].Seed=Seed;break;}}
                    B->Elapsed=Round.ImpactAt-.01f;Air->ResolveStrafeRounds();const bool Held=Victim->Health>0&&Air->PendingStrafe.Num()==1;
                    B->Elapsed=Round.ImpactAt+.01f;Air->ResolveStrafeRounds();
                    Check(TEXT("v63_strafing_has_real_lethal_effect_only_at_committed_arrival"),Held&&!Victim->IsActiveCombatant()&&Air->PendingStrafe.IsEmpty());
                }
            }
        }
        Check(TEXT("v63_specialist_shared_queue_costs_stay_above_rifle_but_not_twice_as_long"),B->RoleRecruitSeconds(EFrontlineRole::RifleGrenadier)>B->RoleRecruitSeconds(EFrontlineRole::Rifle)
            &&B->RoleRecruitSeconds(EFrontlineRole::RifleGrenadier)<1.4f*B->RoleRecruitSeconds(EFrontlineRole::Rifle)
            &&B->RoleRecruitSeconds(EFrontlineRole::Flamethrower)<1.4f*B->RoleRecruitSeconds(EFrontlineRole::Rifle));
        Check(TEXT("v63_command_frontage_covers_dispersed_sections_without_extra_accuracy_stacking"),FrontlineBalanceV50::OrderRange(false,false)>=1900
            &&FrontlineBalanceV50::OrderRange(true,false)>FrontlineBalanceV50::OrderRange(true,true));
    }
};
void FrontlineCombatV63::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineCombatV63Review::Run(B,Check);}
