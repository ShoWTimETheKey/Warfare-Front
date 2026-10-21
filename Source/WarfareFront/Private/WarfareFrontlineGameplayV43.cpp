#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

bool FrontlineGameplayV43::HasCrossedEnemyEnd(const AWarfareFrontlineGameMode* B,const FFrontlineSquad& S)
{
    if(!B||B->Phase!=EFrontlinePhase::Fighting||S.Team<0||S.Team>1||!S.bAdvancing
        ||S.Trench!=INDEX_NONE||S.Destination!=B->GetGoalTrenchIndex(S.Team))return false;
    int32 Living=0;
    for(const auto& W:S.Members)if(const auto* U=W.Get();U&&U->IsAlive())
    {
        if(U->Team!=S.Team||!U->IsActiveCombatant())return false;
        const FVector Feet=U->Feet();if(Feet.ContainsNaN()
            ||Feet.X*B->GetTeamDirection(S.Team)<=FrontlineGameplayV49::BreakthroughLine()
            ||FMath::Abs(Feet.Y)>FrontlineBattlefield::HalfFrontage+200.f)return false;
        ++Living;
    }
    return Living>0&&!FrontlineGameplayV49::EnemyEndDefended(B,S.Team);
}

// These fixtures exercise ordinary recruitment, actor Tick, gun Update and
// objective dispatch. They never run in a player's profile or change a save.
struct FFrontlineGameplayV43Review
{
    static void Operation(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Index,EFrontlineDifficulty D=EFrontlineDifficulty::Rookie)
    {
        B->AbandonBattleToOperations();FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.Difficulty=D;
        R.SelectedMission=R.CompletedMissions=Index;R.Research.TechnologyMask=0;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->ResetEncounter();
        B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;B->SetActorTickEnabled(false);
    }
    static void Quiet(AWarfareFrontlineSoldier* U)
    {U->SetActorTickEnabled(false);U->ReturnToIdle();U->ShotClock=U->GrenadeClock=100000.f;U->FirePoseClock=0;U->Suppression=0;}
    static void Place(AWarfareFrontlineSoldier* U,const FVector& P)
    {U->SetActorLocation(FVector(P.X,P.Y,AWarfareFrontlineGround::HeightAt(P.X,P.Y)+88.f));}
    static AWarfareFrontlineSoldier* Rifle(AWarfareFrontlineGameMode* B,int32 Team=0)
    {
        if(!B->SpawnSquad(Team,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(Team)))return nullptr;
        for(const auto& W:B->Squads.Last().Members)if(auto* U=W.Get())Quiet(U);
        auto* U=B->Squads.Last().Members[0].Get();if(U)Place(U,B->SoldierDestination(U));return U;
    }
    static void TickUnit(AWarfareFrontlineGameMode* B,AWarfareFrontlineSoldier* U,float DT)
    {B->Elapsed+=DT;U->Tick(DT);}

    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        FString UserDir;
        if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
        {Check(TEXT("v43_gameplay_requires_explicit_volatile_isolated_profile"),false);return;}
        const auto SavedRuns=B->CampaignRuns;const auto SavedId=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
        const bool SavedTick=B->IsActorTickEnabled();
        ON_SCOPE_EXIT
        {
            B->AbandonBattleToOperations();B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedId;B->PlayerFaction=SavedFaction;
            B->ResetEncounter();B->SetActorTickEnabled(SavedTick);FrontlineMobilityV41::Reset(B);
        };

        bool Depth=true;int32 MissionCount=0;
        for(const auto F:{EFrontlineFaction::British,EFrontlineFaction::German})
            for(const auto& M:B->GetCampaignMissionsForFaction(F))
            {
                ++MissionCount;const float Before=M.DepthScaleV41*.95f*.90f;
                const float Scale=FrontlineGameplayV33::OperationDepthScale(M.Id);
                FrontlineBattlefield::ConfigureOperationDepth(Scale);
                Depth&=FMath::IsNearlyEqual(Scale/Before,.85f,.00001f)
                    &&FMath::IsNearlyEqual(FrontlineBattlefield::GoalHalfLength,10856.f*Before*.85f,.02f)
                    &&FMath::IsNearlyEqual(FrontlineBattlefield::TrenchSpacing,3680.f*Before*.85f,.02f)
                    &&FrontlineBattlefield::GoalHalfLength>FrontlineBattlefield::SpawnHalfLength;
            }
        Check(TEXT("all_40_operations_are_15_percent_shorter_without_invalid_scale_fallback"),Depth&&MissionCount==40);

        bool LastTrench=false,EndLine=true,Reserves=true,Dead=false;int32 Wins=0;
        for(const auto F:{EFrontlineFaction::British,EFrontlineFaction::German})for(int32 Team=0;Team<2;++Team)
        {
            Operation(B,F,0);auto* U=Rifle(B,Team);if(!U){EndLine=Reserves=false;continue;}
            auto& S=B->Squads.Last();const int32 Dir=B->GetTeamDirection(Team);
            for(auto& Trench:B->Trenches)Trench.Owner=Team;
            const int32 Last=B->GetHomeTrenchIndex(1-Team);S.Trench=Last;S.Destination=Last;S.bAdvancing=false;
            for(const auto& W:S.Members)if(auto* Man=W.Get())Place(Man,FVector(B->Trenches[Last].X,Man->Feet().Y,0));
            B->UpdateCampaignObjective(.05f);LastTrench=B->Phase==EFrontlinePhase::Fighting&&B->BattleOutroWinner==INDEX_NONE;
            Check(FString::Printf(TEXT("army_%d_team_%d_last_trench_capture_does_not_finish"),int32(F),Team),LastTrench);
            S.bAdvancing=true;S.bReinforcement=false;S.Trench=INDEX_NONE;S.Destination=B->GetGoalTrenchIndex(Team);
            for(const auto& W:S.Members)if(auto* Man=W.Get())Place(Man,FVector(Dir*(FrontlineBattlefield::GoalHalfLength-2.f),Man->Feet().Y,0));
            EndLine&=!FrontlineCampaignV41::HasObjectiveVictory(B,Team);
            for(const auto& W:S.Members)if(auto* Man=W.Get())Place(Man,FVector(Dir*(FrontlineBattlefield::GoalHalfLength+4.f),Man->Feet().Y,0));
            for(const auto& W:S.Members)if(auto* Man=W.Get())Man->bFieldEntered=false;
            Reserves&=!FrontlineCampaignV41::HasObjectiveVictory(B,Team);
            for(const auto& W:S.Members)if(auto* Man=W.Get())Man->bFieldEntered=true;
            EndLine&=FrontlineCampaignV41::HasObjectiveVictory(B,Team);
            // A dead section has no centre; it must never inherit a crossing.
            for(const auto& W:S.Members)if(auto* Man=W.Get())Man->Health=0;
            Dead=!FrontlineCampaignV41::HasObjectiveVictory(B,Team);
            for(const auto& W:S.Members)if(auto* Man=W.Get())Man->Health=100;
            B->UpdateCampaignObjective(.05f);
            Wins+=B->BattleOutroWinner==Team&&B->Phase==(Team==0?EFrontlinePhase::Victory:EFrontlinePhase::Defeat);
        }
        Check(TEXT("both_factions_both_teams_require_real_end_crossing_and_dispatch_outcome"),EndLine&&Wins==4);
        Check(TEXT("inactive_reserves_and_dead_sections_cannot_claim_end_line"),Reserves&&Dead);
        Operation(B,EFrontlineFaction::British,0);B->Morale[1]=0;B->EnemyThinkAt=100000;B->Tick(.01f);
        Check(TEXT("morale_collapse_remains_an_alternative_victory"),B->BattleOutroWinner==0&&B->Phase==EFrontlinePhase::Victory);

        bool Availability=true,Order=true;float Previous=MAX_flt;
        for(int32 D=0;D<3;++D)
        {
            Operation(B,EFrontlineFaction::British,0,EFrontlineDifficulty(D));
            const auto* M=B->GetEncounterCampaignMission();const auto& Doctrine=FrontlineDifficultyV30::Doctrine(EFrontlineDifficulty(D));
            const bool P=B->Recruit(EFrontlineRole::Rifle,0),E=B->Recruit(EFrontlineRole::Rifle,1);
            const float Draft=B->GetFactionForTeam(1)==EFrontlineFaction::German?.88f:1.f;
            const float OldEnemy=B->RoleRecruitSeconds(EFrontlineRole::Rifle)*Draft*(M?M->EnemyRecruitScale:1.f)*Doctrine.RecruitScale;
            const float Player=B->RecruitReadyAt[0][0]-B->Elapsed,Enemy=B->RecruitReadyAt[1][0]-B->Elapsed;
            Availability&=P&&E&&FMath::IsNearlyEqual(Player,B->RoleRecruitSeconds(EFrontlineRole::Rifle),.001f)
                &&FMath::IsNearlyEqual(OldEnemy/Enemy,FrontlineGameplayV43::AvailabilityRate(EFrontlineDifficulty(D)),.00001f);
            Order&=Enemy<Previous;Previous=Enemy;
        }
        Check(TEXT("v59_difficulty_uses_explicit_enemy_supply_rates_player_unchanged"),Availability);
        Check(TEXT("rookie_veteran_survivor_recruitment_order_is_preserved"),Order);
        Operation(B,EFrontlineFaction::British,17);B->Elapsed=1000;
        bool Supports=true;for(int32 T=0;T<2;++T)
        {
            B->ArtilleryReadyAt[T]=B->MortarReadyAt[T]=0;
            const bool Cannon=B->RequestArtillery(FVector(0,0,0),T),Mortar=B->RequestMortar(FVector(0,0,0),T);
            const float Rate=T?.75f:1.f;
            Supports&=Cannon&&Mortar&&FMath::IsNearlyEqual(B->SupportCooldownSeconds(EFrontlineSupport::Artillery)/(B->ArtilleryReadyAt[T]-B->Elapsed),Rate,.00001f)
                &&FMath::IsNearlyEqual(45.f/(B->MortarReadyAt[T]-B->Elapsed),Rate,.00001f);
        }
        Check(TEXT("actual_artillery_and_mortar_requests_apply_availability_once"),Supports);
        B->Strikes.Empty();auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());bool Armour=Fleet!=nullptr;
        if(Fleet)for(int32 T=0;T<2;++T)
        {
            Fleet->ResetReadinessForDebug(T);const auto Type=T?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale;
            const bool Recruited=Fleet->Recruit(Type,T);const float Old=Fleet->Definition(Type).Cooldown*FrontlinePacingV24::TempoIntervalScale;
            Armour&=Recruited&&FMath::IsNearlyEqual(Old/Fleet->ArmourCycleSecondsV40[T],T?.75f:1.f,.00001f);
        }
        Check(TEXT("actual_shared_armour_preparation_uses_same_single_availability_factor"),Armour);

        Operation(B,EFrontlineFaction::British,0);auto* U=Rifle(B);bool Arrived=false,Occupied=false;
        if(U)
        {
            U->MotionSpeed=U->RequestedTravelSpeedV43=390;U->SetMotion(U->GetRunMotion());const FVector Start=U->Feet();
            TickUnit(B,U,.016f);Arrived=FVector::Dist2D(Start,U->Feet())<.01f&&!U->bMoving&&U->MotionSpeed==0
                &&U->Playing!=U->GetRunMotion()&&U->Playing!=U->GetWalkMotion();
            auto* Neighbour=B->Squads.Last().Members[1].Get();const FVector Goal=B->SoldierDestination(U);
            if(Neighbour)
            {
                Place(Neighbour,Goal);Place(U,Goal-FVector(70,0,0));U->bAtDestinationV43=false;FrontlineMobilityV41::Reset(B);
                float LateTravel=0;bool Finite=true;for(int32 I=0;I<240;++I)
                {const FVector Before=U->Feet();TickUnit(B,U,.05f);if(I>=200)LateTravel+=FVector::Dist2D(Before,U->Feet());Finite&=!U->Feet().ContainsNaN();}
                Occupied=Finite&&LateTravel<2.f&&!U->bMoving&&FVector::Dist2D(U->Feet(),Neighbour->Feet())>=55.f;
            }
        }
        Check(TEXT("actual_arrival_tick_clears_residual_run_even_with_previous_full_speed"),Arrived);
        Check(TEXT("occupied_slot_stops_without_orbiting_or_body_overlap"),Occupied);

        Operation(B,EFrontlineFaction::British,17);U=Rifle(B);Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());bool VehicleAim=false;
        if(U&&Fleet)
        {
            auto* Tank=Fleet->Spawn(EFrontlineVehicle::A7V,1,false);
            if(Tank)
            {
                Tank->bEntered=true;Tank->FitGroundV35(U->Feet()+FVector(800,0,0),0);
                U->FocusTarget.Reset();TickUnit(B,U,.05f);const int32 Changes=U->MotionTransitions;
                for(int32 I=0;I<30;++I)TickUnit(B,U,.05f);
                VehicleAim=Fleet->AcquireInfantryTarget(U)==Tank&&U->Playing==U->GetAimMotion()&&U->MotionTransitions-Changes<=1;
            }
        }
        Check(TEXT("vehicle_only_contact_does_not_restart_idle_aim_blend_every_tick"),VehicleAim);

        // A rifle's stationary aim check cannot catch a supported weapon locking
        // a different deployment sector. Exercise actual MG ticks against a lone
        // diagonal tank for long enough to expose repeated deploy/pack cycles.
        bool VehicleSupport=true;int32 VehicleSections=0;
        for(const auto F:{EFrontlineFaction::British,EFrontlineFaction::German})
        {
            Operation(B,F,17);Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
            if(!Fleet||!B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(0)))
            {VehicleSupport=false;continue;}
            auto& S=B->Squads.Last();S.Trench=S.Destination=INDEX_NONE;S.bAdvancing=false;
            for(const auto& W:S.Members)if(auto* Man=W.Get())Quiet(Man);
            auto* G=S.Members[0].Get();auto* Gun=FrontlineMachinegunsV30::Get(G);
            auto* Tank=Fleet->Spawn(F==EFrontlineFaction::British?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,1,false);
            if(!G||!Gun||!Tank){VehicleSupport=false;continue;}
            const FVector Station(-1.5f*FrontlineBattlefield::TrenchSpacing,0,0);
            G->FormationOffset+=Station-B->SoldierDestination(G);G->FormationOffset.Z=0;Place(G,Station);
            G->SetActorRotation(FRotator::ZeroRotator);
            Tank->bEntered=true;Tank->FitGroundV35(G->Feet()+FVector(850,680,0),0);
            FrontlineMobilityV41::Reset(B);
            for(int32 I=0;I<160;++I)TickUnit(B,G,.05f);
            const int32 Packs=Gun->Packs,Deploys=Gun->Deployments;
            bool Held=Gun->IsSupported()&&Fleet->AcquireInfantryTarget(G)==Tank;
            for(int32 I=0;I<80;++I)
            {TickUnit(B,G,.05f);Held&=Gun->IsSupported()&&!G->bMoving&&G->MotionSpeed==0;}
            VehicleSupport&=Held&&Gun->CanFire()&&Gun->Packs==Packs&&Gun->Deployments==Deploys&&Deploys==1;
            ++VehicleSections;
        }
        Check(TEXT("both_armies_machineguns_keep_supported_sector_on_vehicle_only_contact"),VehicleSupport&&VehicleSections==2);

        // Wire work owns an early-return movement branch. Its last short step
        // and an occupied cutting position must use accepted displacement, just
        // like ordinary travel and machinegun takeover do.
        Operation(B,EFrontlineFaction::British,17);bool WireStride=false,WireBlocked=false;
        if(B->SpawnSquad(0,EFrontlineRole::Pioneer,B->GetHomeTrenchIndex(0)))
        {
            auto& S=B->Squads.Last();for(const auto& W:S.Members)if(auto* Man=W.Get())Quiet(Man);
            auto* Cutter=S.Members[0].Get();auto* Blocker=S.Members.Num()>1?S.Members[1].Get():nullptr;
            if(Cutter&&Blocker)
            {
                const FVector Station(-1.5f*FrontlineBattlefield::TrenchSpacing,0,0);
                Place(Cutter,Station);Place(Blocker,Station+FVector(0,500,0));FrontlineMobilityV41::Reset(B);
                const FVector Wire=Cutter->Feet()+FVector(69,0,0),Before=Cutter->Feet();
                bool Requested=FrontlineMotionV40::RequestWireCut(Cutter,Wire);
                B->Elapsed+=.05f;FrontlineMotionV40::TickAction(Cutter,.05f);
                const float Travel=FVector::Dist2D(Before,Cutter->Feet());
                WireStride=Requested&&Travel>1.9f&&Travel<2.1f&&FMath::IsNearlyEqual(Cutter->MotionSpeed,Travel/.05f,.01f);
                FrontlineMotionV40::Abort(Cutter);Cutter->ReturnToIdle();
                Place(Cutter,Station);Place(Blocker,Station+FVector(78,0,0));FrontlineMobilityV41::Reset(B);
                Requested=FrontlineMotionV40::RequestWireCut(Cutter,Cutter->Feet()+FVector(145,0,0));
                bool Clear=true,Measured=true;float LateTravel=0;
                for(int32 I=0;I<80;++I)
                {
                    const FVector From=Cutter->Feet();B->Elapsed+=.05f;FrontlineMotionV40::TickAction(Cutter,.05f);
                    const float Accepted=FVector::Dist2D(From,Cutter->Feet());
                    Clear&=FVector::Dist2D(Cutter->Feet(),Blocker->Feet())>=55.4f;
                    Measured&=FMath::IsNearlyEqual(Cutter->MotionSpeed,Accepted/.05f,.02f);
                    if(I>=60)LateTravel+=Accepted;
                }
                WireBlocked=Requested&&Clear&&Measured&&LateTravel<.1f&&!Cutter->bMoving
                    &&Cutter->Playing!=Cutter->GetWalkMotion()&&Cutter->Playing!=Cutter->GetRunMotion();
                FrontlineMotionV40::Abort(Cutter);Cutter->ReturnToIdle();
            }
        }
        Check(TEXT("wire_approach_short_final_step_reports_actual_stride_speed"),WireStride);
        Check(TEXT("occupied_wire_work_position_preserves_bodies_and_stops_walk_loop"),WireBlocked);

        Operation(B,EFrontlineFaction::British,5);B->CampaignRuns[0].Research.TechnologyMask|=1u<<8;
        bool RecoveryPack=false,RecoveryClearance=false,RecoveryCompleted=false;
        if(B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(0)))
        {
            auto& Section=B->Squads.Last();
            if(Section.Members.Num()>=3)
            {
                auto* Fallen=Section.Members[0].Get();auto* Bearer=Section.Members[1].Get();auto* Blocker=Section.Members[2].Get();
                if(Fallen&&Bearer&&Blocker)if(auto* Gun=FrontlineMachinegunsV30::Get(Bearer))
                {
                    for(const auto& W:Section.Members)if(auto* Man=W.Get())Quiet(Man);
                    const float X=-1.5f*FrontlineBattlefield::TrenchSpacing;
                    Place(Fallen,FVector(X,0,0));Place(Bearer,FVector(X-330,0,0));Place(Blocker,FVector(X-200,0,0));
                    Bearer->SetActorRotation(FRotator::ZeroRotator);
                    for(int32 I=0;I<60&&!Gun->IsSupported();++I)Gun->Update(.1f,false);
                    if(Gun->IsSupported())
                    {
                        Bearer->SetMotion(Gun->Action(TEXT("Aim")));Fallen->Health=0;FrontlineMobilityV41::Reset(B);
                        const int32 Packs=Gun->Packs;const FVector Before=Bearer->Feet();TickUnit(B,Bearer,.05f);
                        RecoveryPack=Gun->State==EFrontlineGunStanceV30::Pack&&Gun->Packs==Packs+1
                            &&Bearer->Playing==Gun->TransitionMotion()&&!Bearer->bMoving
                            &&FVector::Dist2D(Before,Bearer->Feet())<.001f;
                        bool Clear=true,Stride=true,MovingOnlyCarried=true;float Travelled=0,SideStep=0;
                        for(int32 I=0;I<240&&!Bearer->bRecoveredMachineGun;++I)
                        {
                            const FVector From=Bearer->Feet();TickUnit(B,Bearer,.05f);
                            const float Travel=FVector::Dist2D(From,Bearer->Feet());Travelled+=Travel;
                            Clear&=FVector::Dist2D(Bearer->Feet(),Blocker->Feet())>=55.4f;
                            Stride&=FMath::IsNearlyEqual(Bearer->MotionSpeed,Travel/.05f,.02f);
                            MovingOnlyCarried&=Travel<.001f||Gun->State==EFrontlineGunStanceV30::Carry;
                            SideStep=FMath::Max(SideStep,FMath::Abs(float(Bearer->Feet().Y)));
                        }
                        RecoveryClearance=Clear&&Stride&&MovingOnlyCarried&&Travelled>250.f&&SideStep>30.f;
                        RecoveryCompleted=Bearer->bRecoveredMachineGun&&Bearer->IsMachineGunner()&&Gun->Packs==Packs+1;
                    }
                }
            }
        }
        Check(TEXT("supported_bearer_packs_before_machinegun_takeover_approach"),RecoveryPack);
        Check(TEXT("takeover_approach_uses_body_clearance_and_actual_motion_speed"),RecoveryClearance);
        Check(TEXT("takeover_reaches_fallen_gun_without_repeated_posture_changes"),RecoveryCompleted);

        Operation(B,EFrontlineFaction::British,5);bool Stable=false,Departure=false,Sector=false;
        if(B->SpawnSquad(0,EFrontlineRole::Lewis,B->GetHomeTrenchIndex(0)))
        {
            auto* G=B->Squads.Last().Members[0].Get();for(const auto& W:B->Squads.Last().Members)if(auto* Man=W.Get())Quiet(Man);
            if(G)if(auto* Gun=FrontlineMachinegunsV30::Get(G))
            {
                const float X=-1.5f*FrontlineBattlefield::TrenchSpacing;Place(G,FVector(X,0,0));G->SetActorRotation(FRotator::ZeroRotator);
                for(int32 I=0;I<60&&!Gun->IsSupported();++I)Gun->Update(.1f,false);
                const int32 Packs=Gun->Packs,Deploys=Gun->Deployments;bool Held=true;
                for(int32 I=0;I<20;++I){Held&=Gun->Update(.05f,true);Gun->Update(.05f,false);}
                Stable=Gun->IsSupported()&&Gun->Packs==Packs&&Gun->Deployments==Deploys&&Held;
                for(int32 I=0;I<6;++I)Gun->Update(.05f,true);
                Departure=Gun->State==EFrontlineGunStanceV30::Pack&&Gun->Packs==Packs+1;
                // Two candidate enemies straddle the firing sector. The retained
                // contact, not a separate best-score query, owns the next setup.
                for(int32 I=0;I<80&&Gun->State!=EFrontlineGunStanceV30::Carry;++I)Gun->Update(.05f,true);
                AWarfareFrontlineSoldier* Target[2]={};
                for(int32 I=0;I<2;++I)
                {
                    Target[I]=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!Target[I])continue;
                    Target[I]->Initialize(B,943900+I,1,EFrontlineRole::Rifle,0);Quiet(Target[I]);B->Units.Add(Target[I]);
                    Place(Target[I],FVector(X+850,I?610.f:-450.f,0));
                }
                if(Target[1])
                {
                    G->FocusTarget=Target[1];G->SetActorRotation((Target[1]->Feet()-G->Feet()).Rotation());
                    for(int32 I=0;I<60&&!Gun->IsSupported();++I)Gun->Update(.1f,false);
                    const int32 After=Gun->Packs;for(int32 I=0;I<30;++I)Gun->Update(.05f,false);
                    Sector=Gun->IsSupported()&&G->FocusTarget.Get()==Target[1]&&Gun->Packs==After;
                }
            }
        }
        Check(TEXT("brief_contact_loss_does_not_repeat_pack_deploy_or_slide_supported_gun"),Stable);
        Check(TEXT("sustained_departure_still_starts_a_real_pack_action"),Departure);
        Check(TEXT("supported_weapon_retains_same_aim_contact_during_deployment"),Sector);
    }
};

void FrontlineGameplayV43::RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineGameplayV43Review::Run(B,Check);}
