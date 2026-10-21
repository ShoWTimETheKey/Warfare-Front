#include "WarfareFrontlineTrenchMotionV8444Checks.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

// These are bounded production-actor trajectories in the prepared UK/DE world.
// They do not replace Tick/Step with an independently implemented movement model.
struct FFrontlineTrenchMotionV8444Review
{
    struct FMan
    {
        AWarfareFrontlineSoldier* Unit=nullptr;
        FVector Previous=FVector::ZeroVector,Goal=FVector::ZeroVector;
        float LateTravel=0.f,MaxVerticalStep=0.f,MaxIntrusion=0.f,LateSupportError=0.f;
        int32 LateMoveFlips=0,LateTrenchFlips=0,StartPacks=0,StartDeployments=0,StartTransitions=0;
        bool WasMoving=false,WasTrench=false;
    };
    static void Quiet(AWarfareFrontlineSoldier* U)
    {
        U->SetActorTickEnabled(false);U->ReturnToIdle();
        U->ShotClock=U->GrenadeClock=10000.f;U->DepartureClock=U->ReactionSeconds=0.f;
        U->Suppression=0.f;U->bFieldEntered=true;
    }
    static void Pose(AWarfareFrontlineSoldier* U,float DT)
    {
        U->Soldier->HandleExistingParallelEvaluationTask(true,true);
        U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
    }
    static FVector Destination(AWarfareFrontlineSoldier* U)
    {
        bool Reform=false;
        return FrontlineMachinegunsV30::Destination(U,U->Battle->SoldierDestination(U),Reform);
    }
    static void Put(AWarfareFrontlineSoldier* U,FVector Feet)
    {
        Feet.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Feet);
        U->SetActorLocation(Feet+FVector(0,0,88.f));
    }
    static bool PhysicalLeg(AWarfareFrontlineSoldier* U,const FVector& From,const FVector& To)
    {
        const int32 Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(From,To)));
        FVector Previous=From;
        for(int32 I=1;I<=Steps;++I)
        {
            FVector Next=FMath::Lerp(From,To,double(I)/Steps);
            if(!FrontlineInfantryV45::AllowsTrenchStep(U,Previous,Next))return false;
            Next.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);Previous=Next;
        }
        return true;
    }
    static const TCHAR* InvalidationName(FrontlineTrenchMotionV8444::EBankInvalidation Reason)
    {
        using E=FrontlineTrenchMotionV8444::EBankInvalidation;
        switch(Reason)
        {
            case E::None:return TEXT("none");case E::GoalChanged:return TEXT("horizontal_goal_changed");
            case E::AssignmentChanged:return TEXT("assignment_changed");case E::BayUnvalidated:return TEXT("bay_unvalidated");
            case E::OutsideLocalScope:return TEXT("outside_local_scope");case E::WaypointReached:return TEXT("waypoint_reached");
            case E::AttemptStalled:return TEXT("actual_steering_attempts_stalled");
        }
        return TEXT("unknown");
    }
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,
        TFunctionRef<void(const FString&,bool)> ExternalCheck)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases,Witnesses;
        bool Good=true;
        const auto Check=[&](const FString& Name,bool Pass)
        {
            auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);
            Checks.Add(MakeShared<FJsonValueObject>(C));Good&=Pass;ExternalCheck(Name,Pass);
        };
        FString UserDir;
        const bool Allowed=B&&B->GetWorld()&&B->IsProgressionVolatile()
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty()
            &&!WarfareFRRUCampaignV82::Active(B);
        Check(TEXT("v8444_trench_motion_disposable_uk_de_only"),Allowed);
        if(!Allowed){J->SetBoolField(TEXT("success"),false);J->SetArrayField(TEXT("checks"),Checks);return J;}
        const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const auto SavedTrenches=B->Trenches;
        const auto SavedTracers=B->Tracers;const auto SavedSmoke=B->SmokeZones;const auto SavedPhase=B->Phase;
        const float SavedElapsed=B->Elapsed;const int32 SavedOutro=B->BattleOutroWinner;
        const int32 SavedAdvances=B->AdvanceCommands;const FString SavedMessage=B->LastMessage;
        TArray<TPair<AWarfareFrontlineSoldier*,bool>> SavedCollision;
        for(AWarfareFrontlineSoldier* U:SavedUnits)if(IsValid(U))
        {SavedCollision.Add({U,U->GetActorEnableCollision()});U->SetActorEnableCollision(false);}
        TArray<AWarfareFrontlineSoldier*> Made;
        ON_SCOPE_EXIT
        {
            for(auto* U:Made)if(IsValid(U))U->Destroy();
            B->Units=SavedUnits;B->Squads=SavedSquads;B->Trenches=SavedTrenches;B->Tracers=SavedTracers;B->SmokeZones=SavedSmoke;
            B->Phase=SavedPhase;B->Elapsed=SavedElapsed;B->BattleOutroWinner=SavedOutro;
            B->AdvanceCommands=SavedAdvances;B->LastMessage=SavedMessage;
            for(const auto& Collision:SavedCollision)if(IsValid(Collision.Key))Collision.Key->SetActorEnableCollision(Collision.Value);
            FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
        };
        const auto Clear=[&]()
        {
            for(auto* U:Made)if(IsValid(U))U->Destroy();Made.Reset();
            B->Units.Empty();B->Squads.Empty();B->Tracers.Empty();B->SmokeZones.Empty();
            FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
            B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
        };
        bool PauseWitness=false;
        int32 CaseIndex=0;
        for(int32 Team:{0,1})for(EFrontlineRole Role:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::MobileGun})
        {
            Clear();B->Elapsed=0.f;
            const int32 Trench=Role==EFrontlineRole::MobileGun?FrontlineBattlefield::CentreTrench:B->GetHomeTrenchIndex(Team);
            if(!B->Trenches.IsValidIndex(Trench)){Check(TEXT("v8444_real_legacy_trench_available"),false);continue;}
            B->Trenches[Trench].Owner=Team;
            FFrontlineSquad Squad;Squad.Id=844400+CaseIndex;Squad.Team=Team;Squad.Role=Role;
            Squad.Trench=Squad.Destination=Trench;Squad.Slot=CaseIndex%3;
            const int32 Count=B->GetSquadSize(Role,Team),Dir=B->GetTeamDirection(Team);
            TArray<FMan> Men;
            for(int32 Member=0;Member<Count;++Member)
            {
                auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
                U->Initialize(B,Squad.Id,Team,Role,Member);Quiet(U);
                B->Units.Add(U);Squad.Members.Add(U);Made.Add(U);Men.Add(FMan{U});
            }
            B->Squads.Add(Squad);
            const FString Prefix=FString::Printf(TEXT("v8444_motion_case%d_"),CaseIndex);
            Check(Prefix+TEXT("full_real_section_spawned"),Men.Num()==Count&&Count>=2);
            if(Men.Num()!=Count||Men.IsEmpty()){++CaseIndex;continue;}
            bool Assets=true;
            for(auto& M:Men)
            {
                auto* U=M.Unit;const FVector Nominal=B->SoldierDestination(U);
                FVector Start=Nominal;Start.X=FrontlineFieldV32::TrenchCentreX(Trench,Start.Y)-Dir*100.f;
                Put(U,Start);M.Previous=U->Feet();M.Goal=Destination(U);
                Assets&=U->GetWalkMotion()!=nullptr&&U->GetAimMotion()!=nullptr;
                if(const auto* Gun=FrontlineMachinegunsV30::Get(U))Assets&=Gun->HasAssets();
            }
            Check(Prefix+TEXT("authored_role_motions_present"),Assets);
            float Time=0.f;int32 Frame=0;bool Finite=true,Separated=true,Planted=true;
            // Mixed genuine small steps plus a bounded hitch, rather than a
            // single SetActorLocation at the asserted final coordinate.
            while(Time<16.f)
            {
                const float Cadence[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};
                const float DT=FMath::Min(Cadence[Frame++%4],16.f-Time);Time+=DT;B->Elapsed+=DT;
                for(auto& M:Men)
                {
                    auto* U=M.Unit;U->Tick(DT);Pose(U,DT);const FVector Feet=U->Feet();
                    Finite&=!Feet.ContainsNaN()&&FMath::IsFinite(U->GetMotionSpeed());
                    M.MaxVerticalStep=FMath::Max(M.MaxVerticalStep,float(FMath::Abs(Feet.Z-M.Previous.Z)));
                    M.MaxIntrusion=FMath::Max(M.MaxIntrusion,FrontlineInfantryV45::BankIntrusion(U,Feet));
                    if(Time>=12.f)
                    {
                        M.LateTravel+=FVector::Dist2D(Feet,M.Previous);
                        M.LateSupportError=FMath::Max(M.LateSupportError,float(FMath::Abs(Feet.Z-FrontlineInfantryV45::SurfaceHeight(B,Feet))));
                        if(U->bMoving!=M.WasMoving)++M.LateMoveFlips;
                        if(const auto* Gun=FrontlineMachinegunsV30::Get(U))if(Gun->IsTrench()!=M.WasTrench)++M.LateTrenchFlips;
                    }
                    if(Time-DT<12.f&&Time>=12.f)
                    {
                        M.StartTransitions=U->MotionTransitions;
                        if(const auto* Gun=FrontlineMachinegunsV30::Get(U))
                        {M.StartPacks=Gun->Packs;M.StartDeployments=Gun->Deployments;}
                    }
                    if(const auto* Gun=FrontlineMachinegunsV30::Get(U))M.WasTrench=Gun->IsTrench();
                    M.WasMoving=U->bMoving;M.Previous=Feet;
                }
                for(int32 A=0;A<Men.Num();++A)for(int32 C=A+1;C<Men.Num();++C)
                    Separated&=FVector::DistSquared2D(Men[A].Unit->Feet(),Men[C].Unit->Feet())>=FMath::Square(55.4f);
            }
            TArray<TSharedPtr<FJsonValue>> MemberRows;
            bool Calm=true,StanceStable=true,Supported=true;
            for(auto& M:Men)
            {
                auto* U=M.Unit;M.Goal=Destination(U);const float Error=FVector::Dist2D(U->Feet(),M.Goal);
                Planted&=Error<=12.f;Calm&=M.LateTravel<=4.f&&M.LateMoveFlips<=2;
                Supported&=M.LateSupportError<=3.1f;
                auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("member"),U->MemberIndex);
                R->SetStringField(TEXT("feet"),U->Feet().ToString());R->SetStringField(TEXT("destination"),M.Goal.ToString());
                R->SetNumberField(TEXT("final_error_cm"),Error);R->SetNumberField(TEXT("late_travel_cm"),M.LateTravel);
                R->SetNumberField(TEXT("late_move_flips"),M.LateMoveFlips);R->SetNumberField(TEXT("max_vertical_step_cm"),M.MaxVerticalStep);
                R->SetNumberField(TEXT("max_bank_intrusion_cm"),M.MaxIntrusion);
                R->SetNumberField(TEXT("late_support_error_cm"),M.LateSupportError);
                R->SetNumberField(TEXT("late_motion_transitions"),U->MotionTransitions-M.StartTransitions);
                if(const auto* Gun=FrontlineMachinegunsV30::Get(U))
                {
                    const int32 LatePacks=Gun->Packs-M.StartPacks,LateDeploys=Gun->Deployments-M.StartDeployments;
                    StanceStable&=LatePacks==0&&LateDeploys==0&&M.LateTrenchFlips==0;
                    R->SetNumberField(TEXT("late_packs"),LatePacks);R->SetNumberField(TEXT("late_deployments"),LateDeploys);
                    R->SetNumberField(TEXT("late_trench_context_flips"),M.LateTrenchFlips);
                }
                MemberRows.Add(MakeShared<FJsonValueObject>(R));
            }
            Check(Prefix+TEXT("actual_16_second_entry_reaches_owned_slots"),Finite&&Planted);
            Check(Prefix+TEXT("last_four_seconds_have_no_position_oscillation"),Calm);
            Check(Prefix+TEXT("last_four_seconds_have_no_repeated_pack_deploy"),StanceStable);
            Check(Prefix+TEXT("real_body_clearance_kept"),Separated);
            Check(Prefix+TEXT("planted_feet_keep_real_surface_support"),Supported);
            auto Case=MakeShared<FJsonObject>();Case->SetNumberField(TEXT("case"),CaseIndex);Case->SetNumberField(TEXT("team"),Team);
            Case->SetStringField(TEXT("faction"),B->GetFactionName(Team));Case->SetNumberField(TEXT("role"),int32(Role));
            Case->SetArrayField(TEXT("members"),MemberRows);Cases.Add(MakeShared<FJsonValueObject>(Case));

            // Seek a real curved-bank route using the production resolver.
            // No fake waypoint or alternate collision surface is injected.
            if(!PauseWitness)
            {
                auto* Probe=Men[0].Unit;const FVector Goal=Destination(Probe);
                TArray<FVector> RestoreFeet;for(const auto& M:Men)RestoreFeet.Add(M.Unit->Feet());
                for(int32 I=1;I<Men.Num();++I)Put(Men[I].Unit,RestoreFeet[I]+FVector(0,3000.f+I*100.f,0));
                bool Found=false;
                for(float Y:{-160.f,-112.f,-80.f,-56.f,56.f,80.f,112.f,160.f})
                {
                    if(Found)break;
                    for(float X:{-100.f,-60.f,-20.f,20.f,60.f,100.f})
                    {
                        FVector Start=Goal+FVector(0,Y,0);
                        Start.X=FrontlineFieldV32::TrenchCentreX(Trench,Start.Y)+Dir*X;
                        Start.Z=FrontlineInfantryV45::SurfaceHeight(B,Start);
                        float Distance=FVector::Dist2D(Start,Goal);
                        if(Distance<30.f||Distance>209.f||FrontlineInfantryV45::BankIntrusion(Probe,Start)>8.f
                            ||PhysicalLeg(Probe,Start,Goal))continue;
                        Put(Probe,Start);FrontlineMobilityV41::Reset(B);
                        FrontlineTrenchMotionV8444::FBankRouteEvidence Before;
                        bool Routed=false;
                        // Follow the real approach to the corner instead of
                        // requiring an arbitrary grid point to coincide with
                        // its sub-centimetre stopping edge.
                        for(int32 Attempt=0;Attempt<400;++Attempt)
                        {
                            const FVector From=Probe->Feet();const FVector Wish=(Goal-From).GetSafeNormal2D();
                            if(FVector::DistSquared2D(From,Goal)<=FMath::Square(5.f))break;
                            const float Budget=PhysicalLeg(Probe,From,From+Wish*8.f)?8.f:.1f;
                            B->Elapsed+=1.f/120.f;
                            Put(Probe,FrontlineMobilityV41::Step(Probe,Goal,Budget,false,1.f/120.f));
                            if(!FrontlineTrenchMotionV8444::ReadBankRoute(Probe,Before)||!Before.Active||Before.Decision==0)continue;
                            // One accepted small step supplies the old timer's
                            // exact retained progress point before a real reload.
                            Start=Probe->Feet();B->Elapsed+=1.f/120.f;
                            Put(Probe,FrontlineMobilityV41::Step(Probe,Goal,.1f,false,1.f/120.f));
                            if(!FrontlineTrenchMotionV8444::ReadBankRoute(Probe,Before)||!Before.Active)continue;
                            Routed=true;break;
                        }
                        if(!Routed)continue;
                        const FVector Next=Probe->Feet();Distance=FVector::Dist2D(Start,Goal);
                        Found=true;
                        const auto* BeforeGun=FrontlineMachinegunsV30::Get(Probe);
                        const bool BeforeTrench=BeforeGun&&BeforeGun->IsTrench();
                        const float PauseStarted=B->Elapsed;const int32 Reloads=Probe->ReloadsStarted;
                        Probe->RoundsRemaining=0;Probe->StartReload();
                        const bool AnimatedReload=Probe->ReloadsStarted==Reloads+1;int32 Ticks=0;
                        // Soldier Tick owns the reload, the ensuing authored
                        // pack, its reaction and the first accepted travel.
                        while(FVector::DistSquared2D(Probe->Feet(),Next)<=FMath::Square(.15f)&&Ticks++<2000)
                        {B->Elapsed+=1.f/120.f;Probe->Tick(1.f/120.f);Pose(Probe,1.f/120.f);}
                        const float Pause=B->Elapsed-PauseStarted;
                        const FVector AfterStep=Probe->Feet();
                        FrontlineTrenchMotionV8444::FBankRouteEvidence After;
                        const bool Kept=FrontlineTrenchMotionV8444::ReadBankRoute(Probe,After)&&After.Active
                            &&After.Decision==Before.Decision
                            &&FVector::DistSquared2D(After.Waypoint,Before.Waypoint)<=FMath::Square(.001f)
                            &&FVector::DistSquared2D(After.Goal,Before.Goal)<=FMath::Square(.001f);
                        const bool LegacyExpired=Pause>.75f
                            &&FVector::DistSquared2D(Next,Before.ProgressFeet)<=FMath::Square(.25f);
                        const auto* CurrentAssignment=B->FindSquad(Probe->SquadId);
                        const auto* AfterGun=FrontlineMachinegunsV30::Get(Probe);
                        const bool Held=CurrentAssignment&&!CurrentAssignment->bAdvancing&&CurrentAssignment->Trench==Trench;
                        const bool SameStance=AfterGun&&AfterGun->IsTrench()==BeforeTrench;
                        const bool LegacyXYZ=Kept
                            &&FVector::DistSquared2D(Before.Goal,After.LastAttemptGoal)<=FMath::Square(.01f)
                            &&FMath::Abs(Before.Goal.Z-After.LastAttemptGoal.Z)>.01f;
                        // A new authoritative horizontal target or arrival at
                        // the waypoint can legally release a route. Verify the
                        // cause against the actual state, never accept an
                        // unexplained changed decision as a successful pause.
                        const bool GoalChanged=After.LastInvalidation==FrontlineTrenchMotionV8444::EBankInvalidation::GoalChanged
                            &&After.InvalidatedDecision==Before.Decision&&After.HeldAtInvalidation&&After.ValidatedAtInvalidation
                            &&FVector::DistSquared2D(After.ReplacementGoal,Before.Goal)>FMath::Square(.01f)
                            &&FrontlineInfantryV45::BankIntrusion(Probe,After.ReplacementGoal)<=8.f
                            &&PhysicalLeg(Probe,Next,AfterStep);
                        const bool Reached=After.LastInvalidation==FrontlineTrenchMotionV8444::EBankInvalidation::WaypointReached
                            &&After.InvalidatedDecision==Before.Decision
                            &&FVector::DistSquared2D(AfterStep,Before.Waypoint)<=FMath::Square(2.1f);
                        const bool ContextRespected=Held&&SameStance&&(Kept||GoalChanged||Reached);
                        Check(Prefix+TEXT("real_reload_respects_checked_route_context"),AnimatedReload&&Pause>.75f&&ContextRespected);
                        if(Kept)
                            Check(Prefix+TEXT("reload_steering_time_excludes_authored_pause"),
                                After.TotalAttemptSeconds-Before.TotalAttemptSeconds<=.25f
                                &&After.StalledSeconds-Before.StalledSeconds<=After.TotalAttemptSeconds-Before.TotalAttemptSeconds+.001f);
                        if(Kept&&LegacyXYZ)
                            Check(Prefix+TEXT("legacy_full_xyz_route_failure_witness"),AnimatedReload&&Held&&SameStance);
                        PauseWitness|=AnimatedReload&&Held&&SameStance&&LegacyXYZ;
                        auto W=MakeShared<FJsonObject>();W->SetNumberField(TEXT("case"),CaseIndex);
                        W->SetNumberField(TEXT("initial_goal_distance_cm"),Distance);W->SetNumberField(TEXT("reload_pause_seconds"),Pause);
                        W->SetStringField(TEXT("start"),Start.ToString());W->SetStringField(TEXT("goal"),Goal.ToString());
                        W->SetStringField(TEXT("retained_progress_feet"),Before.ProgressFeet.ToString());
                        W->SetStringField(TEXT("waypoint_before"),Before.Waypoint.ToString());W->SetStringField(TEXT("waypoint_after"),After.Waypoint.ToString());
                        W->SetNumberField(TEXT("decision_before"),Before.Decision);W->SetNumberField(TEXT("decision_after"),After.Decision);
                        W->SetStringField(TEXT("actual_step_goal_after"),After.LastAttemptGoal.ToString());
                        W->SetNumberField(TEXT("actual_goal_z_difference_cm"),FMath::Abs(Before.Goal.Z-After.LastAttemptGoal.Z));
                        W->SetBoolField(TEXT("legacy_full_xyz_comparison_would_invalidate"),LegacyXYZ);
                        W->SetBoolField(TEXT("legacy_wall_clock_timeout_would_expire"),LegacyExpired);
                        W->SetNumberField(TEXT("stalled_seconds_before"),Before.StalledSeconds);W->SetNumberField(TEXT("stalled_seconds_after"),After.StalledSeconds);
                        W->SetNumberField(TEXT("actual_steering_seconds_after_reload"),After.TotalAttemptSeconds-Before.TotalAttemptSeconds);
                        W->SetStringField(TEXT("last_invalidation"),InvalidationName(After.LastInvalidation));
                        W->SetNumberField(TEXT("invalidated_decision"),After.InvalidatedDecision);
                        W->SetStringField(TEXT("replacement_goal"),After.ReplacementGoal.ToString());
                        W->SetBoolField(TEXT("held_assignment_after"),Held);W->SetBoolField(TEXT("same_trench_stance_after"),SameStance);
                        W->SetNumberField(TEXT("accepted_post_reload_step_cm"),FVector::Dist2D(Next,AfterStep));
                        Witnesses.Add(MakeShared<FJsonValueObject>(W));break;
                    }
                }
                for(int32 I=0;I<Men.Num();++I){Put(Men[I].Unit,RestoreFeet[I]);Quiet(Men[I].Unit);}
                FrontlineMobilityV41::Reset(B);
            }
            // A genuine order change releases local corner ownership and lets
            // the complete section leave its trench through ordinary Tick.
            auto* Assignment=B->FindSquad(Squad.Id);TArray<FVector> DepartureFeet;
            TArray<float> ReadySeconds,BlockedSeconds,FirstTravelAt;
            TArray<int32> DeparturePacks,DepartureDeploys;
            const int32 InitialSlot=Assignment?Assignment->Slot:INDEX_NONE;
            for(const auto& M:Men)DepartureFeet.Add(M.Unit->Feet());
            for(const auto& M:Men)
            {
                ReadySeconds.Add(0.f);BlockedSeconds.Add(0.f);FirstTravelAt.Add(-1.f);
                const auto* Gun=FrontlineMachinegunsV30::Get(M.Unit);
                DeparturePacks.Add(Gun?Gun->Packs:0);DepartureDeploys.Add(Gun?Gun->Deployments:0);
            }
            const bool Ordered=B->AdvanceSquad(Trench,Squad.Id,Team);
            Check(Prefix+TEXT("public_advance_order_accepted"),Ordered);
            for(int32 Tick=0;Tick<600;++Tick)
            {
                B->Elapsed+=1.f/120.f;
                for(int32 I=0;I<Men.Num();++I)
                {
                    auto* U=Men[I].Unit;U->Tick(1.f/120.f);Pose(U,1.f/120.f);
                    const auto* Gun=FrontlineMachinegunsV30::Get(U);
                    const bool Ready=U->WeaponPhase==EFrontlineWeaponPhase::Ready&&U->FirePoseClock<=0.f
                        &&U->DepartureClock<=0.f&&U->bWantedMovement&&(!Gun||Gun->State==EFrontlineGunStanceV30::Carry);
                    if(Ready){ReadySeconds[I]+=1.f/120.f;if(U->GetMotionSpeed()<=8.f)BlockedSeconds[I]+=1.f/120.f;}
                    if(FirstTravelAt[I]<0.f&&FVector::DistSquared2D(DepartureFeet[I],U->Feet())>FMath::Square(1.f))FirstTravelAt[I]=(Tick+1)/120.f;
                }
            }
            bool Departed=Ordered&&Assignment!=nullptr,Released=true;
            TArray<TSharedPtr<FJsonValue>> AdvanceRows;
            for(int32 I=0;I<Men.Num();++I)
            {
                auto* U=Men[I].Unit;const float Progress=(U->Feet().X-DepartureFeet[I].X)*Dir;
                Departed&=Progress>160.f;
                FrontlineTrenchMotionV8444::FBankRouteEvidence Route;
                Released&=!FrontlineTrenchMotionV8444::ReadBankRoute(U,Route)||!Route.Active;
                auto A=MakeShared<FJsonObject>();A->SetNumberField(TEXT("member"),U->MemberIndex);
                A->SetStringField(TEXT("from"),DepartureFeet[I].ToString());A->SetStringField(TEXT("to"),U->Feet().ToString());
                A->SetStringField(TEXT("current_order_destination"),B->SoldierDestination(U).ToString());
                A->SetNumberField(TEXT("forward_progress_cm"),Progress);A->SetNumberField(TEXT("ready_for_locomotion_seconds"),ReadySeconds[I]);
                A->SetNumberField(TEXT("ready_but_motion_blocked_seconds"),BlockedSeconds[I]);A->SetNumberField(TEXT("first_actual_travel_seconds"),FirstTravelAt[I]);
                A->SetNumberField(TEXT("weapon_phase"),int32(U->WeaponPhase));A->SetNumberField(TEXT("remaining_fire_pose_seconds"),U->FirePoseClock);
                A->SetNumberField(TEXT("remaining_departure_seconds"),U->DepartureClock);
                A->SetNumberField(TEXT("physical_forward_side_of_bank_cm"),(U->Feet().X-FrontlineFieldV32::TrenchCentreX(Trench,U->Feet().Y))*Dir);
                if(const auto* Gun=FrontlineMachinegunsV30::Get(U))
                {
                    A->SetNumberField(TEXT("stance"),int32(Gun->State));A->SetNumberField(TEXT("packs_during_advance"),Gun->Packs-DeparturePacks[I]);
                    A->SetNumberField(TEXT("deployments_during_advance"),Gun->Deployments-DepartureDeploys[I]);
                    A->SetNumberField(TEXT("remaining_stance_seconds"),Gun->TransitionMotion()?FMath::Max(0.f,Gun->TransitionDurationV38()-Gun->StanceClock):0.f);
                }
                AdvanceRows.Add(MakeShared<FJsonValueObject>(A));
            }
            Case->SetNumberField(TEXT("slot_before_advance"),InitialSlot);Case->SetNumberField(TEXT("receiving_slot_after_advance"),Assignment?Assignment->Slot:INDEX_NONE);
            Case->SetArrayField(TEXT("advance_members"),AdvanceRows);
            Check(Prefix+TEXT("real_advance_order_leaves_trench"),Departed);
            Check(Prefix+TEXT("advance_order_releases_local_bank_route"),Released);
            ++CaseIndex;
        }
        Check(TEXT("v8444_trench_motion_real_reload_legacy_xyz_failure_exercised"),PauseWitness);
        J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);
        J->SetArrayField(TEXT("cases"),Cases);J->SetArrayField(TEXT("legacy_failure_witnesses"),Witnesses);
        J->SetStringField(TEXT("scope"),TEXT("Six UK/DE held-trench trajectories using real section actors, mixed 120/60/30/10 Hz steps, final four-second position and stance stability, one real curved-bank route across an authored reload with the old full-XYZ target mismatch, and subsequent five-second public advance. The initial local route scope remains 110 cm. Actual steering time is recorded separately from animation waiting; there is no independent field claim that a wall-clock stall alone caused the earlier failure. No natural campaign or visual/gameplay acceptance claim."));
        return J;
    }
};

TSharedRef<FJsonObject> FrontlineTrenchMotionV8444::RunChecks(AWarfareFrontlineGameMode* B,
    TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineTrenchMotionV8444Review::Run(B,Check);}
