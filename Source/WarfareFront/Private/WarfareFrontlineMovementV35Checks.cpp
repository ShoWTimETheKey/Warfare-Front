#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

// Explicit, volatile review fixtures only. Production code never rewinds a
// reserve column or teleports a vehicle to an artificial terrain sample.
struct FFrontlineMovementV35Review
{
    static bool NeutralBolt(AWarfareFrontlineSoldier* U)
    {
        U->WeaponPhase=EFrontlineWeaponPhase::Recoil;U->FirePoseClock=0;U->RoundsRemaining=4;
        U->UpdateWeaponCycle(.01f);
        return U->WeaponPhase==EFrontlineWeaponPhase::BoltCycle && U->Playing==U->Battle->BoltAnimation;
    }
    static void Quiet(AWarfareFrontlineSoldier* U)
    {U->ReturnToIdle();U->ShotClock=U->GrenadeClock=10000;U->FirePoseClock=0;}
    static bool VisualTank(AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,int32 Team,const FVector& Position)
    {
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        // This explicit fixture bypasses Recruit's campaign unlock check, but
        // must still bind the same encounter epoch before calling private Spawn.
        // Otherwise the next fleet tick clears all four freshly staged tanks.
        if(Fleet && Fleet->LastEncounter!=B->GetEncounterId())
        {Fleet->ClearEncounter();Fleet->LastEncounter=B->GetEncounterId();}
        auto* Tank=Fleet?Fleet->Spawn(Type,Team,false):nullptr;if(!Tank)return false;
        Tank->bEntered=true;Tank->FitGroundV35(Position,0);
        // The real fleet owns these fixtures, so ResetEncounter cleans them up.
        Tank->CannonClock[0]=Tank->CannonClock[1]=10000;Tank->NextVoiceCheck=10000;
        for(float& Clock:Tank->GunClock)Clock=10000;
        for(int32& Remaining:Tank->BurstRemaining)Remaining=0;
        return true;
    }
    static void VisualInfantry(AWarfareFrontlineSoldier* U)
    {
        Quiet(U);U->SetActorTickEnabled(false);
        if(auto* Stance=FrontlineMachinegunsV30::Get(U))
            for(int32 Tick=0;Tick<70 && !Stance->IsSupported();++Tick)Stance->Update(.1f,false);
        U->SetMotion(U->GetAimMotion());
    }
    static bool TankCrossing(AWarfareFrontlineGameMode* B,EFrontlineVehicle Type,int32 Team,float& MinimumClearance)
    {
        const int32 Dir=B->GetTeamDirection(Team);
        const float Y=Team?455.f:-455.f;
        auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>(FVector(-Dir*600.f,Y,0),FRotator(0,Dir>0?0:180,0));
        if(!Tank || !Tank->Configure(B,Type,Team,3500+int32(Type))){if(Tank)Tank->Destroy();return false;}
        ON_SCOPE_EXIT {Tank->Destroy();};
        Tank->bEntered=true;Tank->FitGroundV35(Tank->GetActorLocation(),0);
        const bool Heavy=Type==EFrontlineVehicle::MarkIVMale || Type==EFrontlineVehicle::BeuteMarkIV;
        const float Rear=Heavy?290.f:Type==EFrontlineVehicle::A7V?275.f:255.f;
        const float Front=Heavy?330.f:Rear,HalfWidth=(Heavy?147.f:Type==EFrontlineVehicle::A7V?123.f:109.f)+22.f;
        MinimumClearance=MAX_flt;float Furthest=-600.f;
        for(int32 Step=0;Step<120;++Step)
        {
            Tank->Simulate(.1f);Furthest=FMath::Max(Furthest,float(Tank->GetActorLocation().X*Dir));
            // Independent, denser sample than the 27 runtime support points.
            for(int32 Row=0;Row<5;++Row)for(int32 I=0;I<25;++I)
            {
                const FVector P=Tank->GetActorTransform().TransformPosition(FVector(FMath::Lerp(-Rear,Front,I/24.f),FMath::Lerp(-HalfWidth,HalfWidth,Row*.25f),Heavy?12.f:14.f));
                MinimumClearance=FMath::Min(MinimumClearance,float(P.Z)-AWarfareFrontlineGround::HeightAt(P.X,P.Y));
            }
        }
        return Furthest>240.f && MinimumClearance>=-6.f && FMath::Abs(Tank->GetActorRotation().Pitch)<=12.01f;
    }
};

void AWarfareFrontlineGameMode::RunV35MovementChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Allowed=FParse::Param(FCommandLine::Get(),TEXT("WFV35Review")) && bProgressionVolatile;
    Check(TEXT("movement_fixture_is_explicit_and_volatile"),Allowed);if(!Allowed)return;
    auto* PC=Cast<AWarfareFrontlineController>(GetWorld()->GetFirstPlayerController());
    auto* Camera=PC?PC->BattleCamera.Get():nullptr;
    const FTransform CameraBefore=Camera?Camera->GetActorTransform():FTransform::Identity;
    const float WidthBefore=Camera?Camera->GetCameraComponent()->OrthoWidth:6000.f;
    const EFrontlineFaction FactionBefore=PlayerFaction;
    ON_SCOPE_EXIT {if(Camera){Camera->SetActorTransform(CameraBefore);Camera->GetCameraComponent()->OrthoWidth=WidthBefore;}};
    for(EFrontlineFaction Faction:{EFrontlineFaction::British,EFrontlineFaction::German})
    {
        TArray<FVector> Reference;
        const FString Army=Faction==EFrontlineFaction::British?TEXT("british_"):TEXT("german_");
        bool Same=true,Outside=true,Progress=true;
        for(int32 View=0;View<3;++View)
        {
            ResetEncounter();SelectPlayerFaction(Faction);BeginBattle();
            if(Camera){FVector P=Camera->GetActorLocation();P.X=(View-1)*(FrontlineBattlefield::SupportHalfLength-1800.f);Camera->SetActorLocation(P);Camera->GetCameraComponent()->OrthoWidth=9000.f;}
            if(!SpawnSquad(0,EFrontlineRole::Rifle)){Same=Outside=Progress=false;continue;}
            const int32 Dir=GetTeamDirection(0);
            for(int32 I=0;I<Squads.Last().Members.Num();++I)
            {
                auto* U=Squads.Last().Members[I].Get();if(!U){Same=Outside=Progress=false;continue;}
                U->SetActorTickEnabled(false);const FVector Start=U->Feet();
                if(View==0)Reference.Add(Start);else Same&=Reference.IsValidIndex(I) && Reference[I].Equals(Start,.01f);
                Outside&=Start.X*Dir<-FrontlineBattlefield::SupportHalfLength-2700.f-300.f && !U->IsActiveCombatant();
                for(int32 Tick=0;Tick<40;++Tick)U->Tick(.05f);
                Progress&=(U->Feet().X-Start.X)*Dir>180.f && U->DistanceTravelled>180.f;
            }
        }
        Check(Army+TEXT("actual_squad_world_spawn_is_identical_at_three_camera_positions"),Same);
        Check(Army+TEXT("all_members_spawn_beyond_every_legal_viewport"),Outside);
        Check(Army+TEXT("actual_soldier_tick_advances_reserves_without_camera_visibility"),Progress);

        ResetEncounter();SelectPlayerFaction(Faction);BeginBattle();
        const bool Spawned=SpawnSquad(0,EFrontlineRole::Rifle,GetHomeTrenchIndex(0));
        auto* Unit=Spawned?Squads.Last().Members[0].Get():nullptr;
        bool Facing=false,Bolt=false;
        if(Unit)
        {
            Unit->SetActorTickEnabled(false);Unit->SetActorLocation(SoldierDestination(Unit)+FVector(0,0,88));
            FFrontlineMovementV35Review::Quiet(Unit);
            Unit->SetActorRotation(FRotator(0,GetTeamDirection(0)>0?180:0,0));
            for(int32 Tick=0;Tick<40;++Tick)Unit->Tick(.05f);
            Facing=FVector::DotProduct(Unit->GetActorForwardVector(),FVector(GetTeamDirection(0),0,0))>.995f;
            Bolt=CoverFor(Unit)>0.f && FFrontlineMovementV35Review::NeutralBolt(Unit);
        }
        Check(Army+TEXT("settled_infantry_restore_enemy_facing_after_slot_adjustment"),Facing);
        Check(Army+TEXT("trench_rifle_works_bolt_without_repetitive_squat_clip"),Bolt);
        bool CrewStable=false;
        if(SpawnSquad(0,EFrontlineRole::Lewis,GetHomeTrenchIndex(0)))
        {
            auto* Gunner=Squads.Last().Members[0].Get();auto* Bearer=Squads.Last().Members[1].Get();
            if(Gunner && Bearer)if(auto* Stance=FrontlineMachinegunsV30::Get(Gunner))
            {
                Stance->State=EFrontlineGunStanceV30::Supported;
                const FVector Stand(-2000,0,AWarfareFrontlineGround::HeightAt(-2000,0));
                Gunner->SetActorLocation(Stand+FVector(0,0,88));Gunner->SetActorRotation(FRotator::ZeroRotator);Gunner->bMoving=false;
                FVector P=Stand-FVector(24,0,0)+FVector(0,90+Bearer->GetMotionPhase()*12+20,0);
                P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);Bearer->SetActorLocation(P+FVector(0,0,88));Bearer->bMoving=false;
                bool Reform=false;const FVector Destination=FrontlineMachinegunsV30::Destination(Bearer,SoldierDestination(Bearer),Reform);
                CrewStable=Destination.Equals(Bearer->Feet(),.01f) && !Reform;
            }
        }
        Check(Army+TEXT("minor_gun_traverse_does_not_restart_bearer_pack_deploy"),CrewStable);
    }
    ResetEncounter();SelectPlayerFaction(EFrontlineFaction::British);BeginBattle();
    for(EFrontlineVehicle Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::Whippet,EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIV})
    {
        float Clearance=0;const int32 Team=Type==EFrontlineVehicle::A7V || Type==EFrontlineVehicle::BeuteMarkIV?1:0;
        const bool Passed=FFrontlineMovementV35Review::TankCrossing(this,Type,Team,Clearance);
        UE_LOG(LogTemp,Display,TEXT("WF_V35_TERRAIN vehicle=%d minimum_dense_underside_clearance_cm=%.3f"),int32(Type),Clearance);
        Check(FString::Printf(TEXT("vehicle_%d_actual_simulation_bridges_trench_without_underside_penetration"),int32(Type)),Passed);
    }
    int32 MissionCount=0;bool BreakthroughOnly=true;
    for(EFrontlineFaction Faction:{EFrontlineFaction::British,EFrontlineFaction::German})
        for(const auto& Mission:GetCampaignMissionsForFaction(Faction))
        {++MissionCount;BreakthroughOnly&=Mission.Objective==EFrontlineCampaignObjective::Breakthrough && Mission.HoldCenterSeconds==0.f;}
    Check(TEXT("all_twenty_campaign_missions_use_breakthrough_without_timed_hold"),MissionCount==20 && BreakthroughOnly);
    ResetEncounter();BeginBattle();Trenches[FrontlineBattlefield::CentreTrench].Owner=0;
    CampaignHoldElapsed=15000.f;Elapsed=15000.f;const float BeforeMorale=Morale[1];
    UpdateCampaignObjective(6000.f);
    Check(TEXT("stale_legacy_hold_elapsed_cannot_win_a_battle"),Phase==EFrontlinePhase::Fighting && BattleOutroWinner==INDEX_NONE
        && CampaignHoldElapsed==0.f && GetCampaignObjectiveProgress()==0.f && EndReason.IsEmpty() && Morale[1]==BeforeMorale && !bEncounterRewarded);
    ResetEncounter();SelectPlayerFaction(FactionBefore);
}

bool AWarfareFrontlineGameMode::SetupV35MovementVisualFixture()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV35Review")) || !bProgressionVolatile)return false;
    ResetEncounter();SelectPlayerFaction(EFrontlineFaction::British);BeginBattle();
    if(Phase!=EFrontlinePhase::Fighting)return false;
    EnemyRecruitAt=EnemyThinkAt=EnemyFireAssessAt=EnemyAssaultAt=MAX_flt;EnemyFireExecuteAt=0;bEnemyFirePending=false;
    struct FGroup {int32 Team;EFrontlineRole Role;float Y;};
    bool Ready=true;
    for(const FGroup& Group:{FGroup{0,EFrontlineRole::Rifle,-300.f},FGroup{1,EFrontlineRole::Rifle,300.f},FGroup{0,EFrontlineRole::Lewis,-900.f},FGroup{1,EFrontlineRole::Lewis,900.f}})
    {
        if(!SpawnSquad(Group.Team,Group.Role,FrontlineBattlefield::CentreTrench)){Ready=false;continue;}
        auto& Squad=Squads.Last();Squad.Slot=1;
        for(const auto& Ref:Squad.Members)if(auto* Unit=Ref.Get())
        {
            Unit->FormationOffset.Y+=Group.Y;Unit->FormationOffset.X+=Group.Team?70.f:-70.f;
            Unit->SetActorLocation(SoldierDestination(Unit)+FVector(0,0,88));
            Unit->SetActorRotation(FRotator(0,GetTeamDirection(Group.Team)>0?0:180,0));
        }
    }
    // Two distinct terrain-contact rows leave the firing step readable between
    // them. All vehicles retain their real materials, tracks and fitted hulls.
    Ready&=FFrontlineMovementV35Review::VisualTank(this,EFrontlineVehicle::MarkIVMale,0,FVector(-360,-1350,0));
    Ready&=FFrontlineMovementV35Review::VisualTank(this,EFrontlineVehicle::A7V,1,FVector(520,-1350,0));
    Ready&=FFrontlineMovementV35Review::VisualTank(this,EFrontlineVehicle::Whippet,0,FVector(-420,1350,0));
    Ready&=FFrontlineMovementV35Review::VisualTank(this,EFrontlineVehicle::BeuteMarkIV,1,FVector(460,1350,0));
    for(const auto& Unit:Units)if(Unit)FFrontlineMovementV35Review::VisualInfantry(Unit.Get());
    LastMessage.Empty();
    UE_LOG(LogTemp,Display,TEXT("WF_V35_CONTACT_FIXTURE ready=%d tanks=4 infantry=%d synthetic=1"),Ready,Units.Num());
    return Ready;
}
