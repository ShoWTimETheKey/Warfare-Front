#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

TSharedRef<FJsonObject> RunFrontlineTrenchSectorSelectorChecksV8444(TFunctionRef<void(const FString&,bool)> Check);

struct FFrontlineFireControlV831Review
{
static TSharedRef<FJsonObject> RunV844(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Result=MakeShared<FJsonObject>();bool Good=true;TArray<TSharedPtr<FJsonValue>> Rows;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    FString User;const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV844Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review")))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User)
        &&!WarfareFRRUCampaignV82::Active(B);
    Assert(TEXT("v844_trench_fire_disposable_ukde_world"),Safe);if(!Safe){Result->SetBoolField(TEXT("success"),false);return Result;}
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    Assert(TEXT("v844_trench_fire_actual_vehicle_subsystem"),Fleet!=nullptr);if(!Fleet){Result->SetBoolField(TEXT("success"),false);return Result;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;const auto Vehicles=Fleet->Vehicles;
    const auto Tracers=B->Tracers;const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Phase=B->Phase;const auto Random=B->Random;
    const float Clock=B->Elapsed,Enemy=B->EnemyThinkAt,Extended=B->ExtendedSupportThinkAt,Peak=B->PeakSuppression;
    const int32 Shots=B->ShotsFired,Winner=B->BattleOutroWinner,NextId=B->NextSquadId,Casualties[2]={B->Casualties[0],B->Casualties[1]};
    const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
    TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
    struct FHidden{TWeakObjectPtr<AActor> A;bool Hidden,Collision;};TArray<FHidden> Hidden;TArray<AActor*> Owned;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AWarfareFrontlineSoldier* U:Units)Hide(U);for(AFrontlineTank* V:Vehicles)Hide(V);
    const auto Clear=[&](){for(AActor* A:Owned)if(IsValid(A))A->Destroy();Owned.Reset();B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);};
    ON_SCOPE_EXIT
    {
        Clear();for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
        B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;Fleet->Vehicles=Vehicles;B->Tracers=Tracers;B->Strikes=Strikes;B->SmokeZones=Smoke;
        B->Random=Random;B->Phase=Phase;B->Elapsed=Clock;B->EnemyThinkAt=Enemy;B->ExtendedSupportThinkAt=Extended;B->PeakSuppression=Peak;
        B->ShotsFired=Shots;B->BattleOutroWinner=Winner;B->NextSquadId=NextId;B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;
        for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];}
        for(const auto& H:Hidden)if(auto* A=H.A.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        Result->SetBoolField(TEXT("success"),Good);Result->SetArrayField(TEXT("cases"),Rows);
        Result->SetStringField(TEXT("scope"),TEXT("Actual current UK/DE trench bank geometry, stationed rifle and supported gun, newly obstructed diagonal sector, real peer observation, production Tick placement and discharge, broad opaque wall and bypass rejection. V8444 additionally uses home/centre full rosters, a nearest-opaque/farther-reachable witness, actual repeated rifle/MG cycles and distinct Y/pose/range/cached-XY-identity checks. Separate numerical candidate-admission boundaries share the production MG scoring helper. The unsupported original-footpoint over-traverse production extension was withdrawn and is reported as not exercised. Contact obstacles are explicit collision fixtures; this is not a whole-map natural-battle acceptance."));
    };
    B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();B->Strikes.Reset();B->SmokeZones.Reset();
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->EnemyThinkAt=B->ExtendedSupportThinkAt=MAX_flt;B->Elapsed=1000;
    const auto Ground=[&](FVector P){P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);return P;};
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT){if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();if(auto* G=FrontlineMachinegunsV30::Get(U))G->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const auto Make=[&](int32 Team,int32 Id,EFrontlineRole Role,int32 Member,const FVector& Feet)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;U->Initialize(B,Id,Team,Role,Member);Owned.Add(U);B->Units.Add(U);U->SetActorTickEnabled(false);
        U->ReturnToIdle();U->SetActorLocation(Ground(Feet)+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
        U->bFieldEntered=true;U->ShotClock=U->GrenadeClock=10000;U->AimSettleClock=U->ReactionSeconds=U->DepartureClock=0;U->SetMotion(U->GetAimMotion());Pose(U,.1f);return U;
    };
    for(int32 Team=0;Team<2;++Team)for(int32 Heavy=0;Heavy<2;++Heavy)
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Id=++B->NextSquadId;
        const FString Prefix=FString::Printf(TEXT("v844_team%d_%s"),Team,Heavy?TEXT("mg"):TEXT("rifle"));
        if(!B->Trenches.IsValidIndex(Home)){Assert(Prefix+TEXT("_actual_home_trench"),false);continue;}B->Trenches[Home].Owner=Team;
        FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=Heavy?EFrontlineRole::Lewis:EFrontlineRole::Rifle;S.Trench=S.Destination=S.LastPassedTrench=Home;S.Slot=1;S.bHoldPosition=true;B->Squads.Add(S);
        TArray<AWarfareFrontlineSoldier*> Men;for(int32 M=0;M<(Heavy?3:6);++M)if(auto* U=Make(Team,Id,S.Role,M,FVector(B->Trenches[Home].X,(M-2.f)*130.f,0))){B->FindSquad(Id)->Members.Add(U);Men.Add(U);}
        Assert(Prefix+TEXT("_complete_actual_section"),Men.Num()==(Heavy?3:6));if(Men.Num()!=(Heavy?3:6))continue;
        for(auto* U:Men){bool Reform=false;const FVector P=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);U->SetActorLocation(Ground(P)+FVector(0,0,88));Pose(U,.1f);}
        // No contact has existed. Normal deploy/aim must bootstrap the genuine
        // measured socket, without pretending the gun already fired.
        for(int32 F=0;F<100;++F){B->Elapsed+=.05f;for(auto* U:Men){U->Tick(.05f);Pose(U,.05f);}}
        auto* Subject=Men[0];auto* Observer=Men.Last();auto* Gun=FrontlineMachinegunsV30::Get(Subject);
        const FVector Start=Subject->Feet();const float StartYaw=Subject->GetActorRotation().Yaw;const int32 DeployBefore=Gun?Gun->Deployments:0,PackBefore=Gun?Gun->Packs:0;
        Assert(Prefix+TEXT("_first_arrival_before_contact_is_settled"),!Subject->bMoving&&(!Heavy||(Gun&&Gun->IsTrench()&&Gun->IsSupported()&&Gun->TrenchObservationOrigin().Equals(Subject->GetMuzzleLocation(),1.f))));
        auto* Target=Make(1-Team,++B->NextSquadId,EFrontlineRole::Rifle,0,Start+FVector(Dir*900.f,350.f,0));if(!Target){Assert(Prefix+TEXT("_target_spawned"),false);continue;}
        bool Baseline=false;
        for(float Along:{900.f,1200.f,1500.f})for(float Across:{350.f,-350.f,500.f,-500.f})
        {if(Baseline)break;Target->SetActorLocation(Ground(Start+FVector(Dir*Along,Across,0))+FVector(0,0,88));Pose(Target,.1f);Baseline=B->HasSight(Subject,Target)&&B->HasSight(Observer,Target)&&(!Gun||Gun->HasInfantryFiringLine(Target));}
        Assert(Prefix+TEXT("_actual_diagonal_baseline_visible_to_both_members"),Baseline);if(!Baseline)continue;
        Observer->Tick(.05f);Pose(Observer,.05f);
        Assert(Prefix+TEXT("_peer_contact_is_acquired_by_actual_tick"),Observer->FocusTarget.Get()==Target);
        auto* Wall=B->GetWorld()->SpawnActor<AActor>();if(!Wall){Assert(Prefix+TEXT("_wall_spawned"),false);continue;}Owned.Add(Wall);
        auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(10,16,500));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->RegisterComponent();
        const FVector Source=FrontlineMachinegunsV30::SightOrigin(Subject),End=FrontlineMachinegunsV30::SightTarget(Target);
        Wall->SetActorLocation(FMath::Lerp(Source,End,.18f));
        FCollisionQueryParams Q(SCENE_QUERY_STAT(V844GenericBayProbe),true);for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Q.AddIgnoredActor(U);
        FVector Forward=Source+FVector(Dir*650.f,0,0);
        const bool GenericClear=!B->GetWorld()->LineTraceTestByChannel(Source,Forward,ECC_Visibility,Q);
        const bool Blocked=!B->HasSight(Subject,Target)&&B->HasSight(Observer,Target)&&GenericClear;
        Assert(Prefix+TEXT("_new_diagonal_wall_misses_old_generic_frontage_but_blocks_subject"),Blocked);
        const int32 Before=Subject->ShotsDischarged;Subject->ShotClock=0;Subject->DischargeWeapon(Target);
        Assert(Prefix+TEXT("_shared_observation_never_bypasses_real_wall"),Subject->ShotsDischarged==Before);
        float MaxStep=0;int32 Frames=0;
        for(;Frames<240&&Subject->ShotsDischarged==Before;++Frames)
        {
            B->Elapsed+=.05f;for(auto* U:Men){const FVector Old=U->Feet();U->Tick(.05f);Pose(U,.05f);MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Old,U->Feet())));}
        }
        const FVector Finish=Subject->Feet();
        const float Turn=FMath::Abs(FMath::FindDeltaAngleDegrees(StartYaw,Subject->GetActorRotation().Yaw));
        Assert(Prefix+TEXT("_blocked_sector_refits_or_traverses_and_really_discharges"),Blocked&&Subject->ShotsDischarged>Before&&(FVector::Dist2D(Start,Finish)>12.f||Turn>3.f));
        Assert(Prefix+TEXT("_local_relocation_uses_bounded_physical_steps"),MaxStep<30.f&&!Finish.ContainsNaN()&&FrontlineInfantryV45::BankIntrusion(Subject,Finish)<=8.1f);
        Assert(Prefix+TEXT("_local_refit_does_not_loop_pack_deploy"),!Gun||(Gun->Deployments-DeployBefore<=1&&Gun->Packs-PackBefore<=1));
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetStringField(TEXT("role"),Heavy?TEXT("machinegun"):TEXT("rifle"));
        Row->SetStringField(TEXT("old_feet"),Start.ToString());Row->SetStringField(TEXT("new_feet"),Finish.ToString());Row->SetNumberField(TEXT("seconds_to_discharge"),Frames*.05f);
        Row->SetNumberField(TEXT("actual_traverse_degrees"),Turn);
        Row->SetNumberField(TEXT("starting_yaw"),StartYaw);Row->SetNumberField(TEXT("final_yaw"),Subject->GetActorRotation().Yaw);
        if(Gun)Row->SetObjectField(TEXT("actual_gun_sector"),Gun->DescribeTrenchFiringSectorV844());
        Row->SetBoolField(TEXT("old_generic_ray_clear"),GenericClear);Row->SetBoolField(TEXT("real_contact_blocked"),Blocked);Row->SetNumberField(TEXT("shots"),Subject->ShotsDischarged-Before);Row->SetNumberField(TEXT("max_actual_tick_step_cm"),MaxStep);
        Row->SetNumberField(TEXT("extra_deployments"),Gun?Gun->Deployments-DeployBefore:0);Row->SetNumberField(TEXT("extra_packs"),Gun?Gun->Packs-PackBefore:0);Rows.Add(MakeShared<FJsonValueObject>(Row));
        // With a full opaque barrier no reachable local candidate is legal.
        Box->SetBoxExtent(FVector(15,5000,5000));Wall->SetActorLocation(Start+FVector(Dir*420.f,0,145.f));
        const FVector DestinationBefore=B->SoldierDestination(Subject);Subject->ReturnToIdle();Subject->ShotClock=0;const int32 OpaqueBefore=Subject->ShotsDischarged;
        for(int32 F=0;F<80;++F){B->Elapsed+=.05f;Subject->Tick(.05f);Pose(Subject,.05f);}
        Assert(Prefix+TEXT("_opaque_wall_does_not_create_fire_or_a_new_blind_destination"),Subject->ShotsDischarged==OpaqueBefore&&B->SoldierDestination(Subject).Equals(DestinationBefore,.1f));
        const auto Assignment=*B->FindSquad(Id);B->FindSquad(Id)->bAdvancing=true;B->FindSquad(Id)->Trench=INDEX_NONE;
        Assert(Prefix+TEXT("_passing_or_locked_trench_is_not_a_garrison_contact_source"),FrontlineInfantryV45::ObservedTrenchContactV844(Subject)==nullptr);*B->FindSquad(Id)=Assignment;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review")))
    {
#include "WarfareFrontlineTrenchFireReviewV8444.inl"
    }
    return Result;
}
static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto Result=MakeShared<FJsonObject>();bool Good=true;TArray<TSharedPtr<FJsonValue>> Rows,SightRows;
    int32 SightPairs=0,ShortBlockedClearAtGoal=0,GoalBlockedClearAtShort=0;
    const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
    FString User;const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV831Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User)
        &&!WarfareFRRUCampaignV82::Active(B);
    Assert(TEXT("v831_fire_control_disposable_ukde_world"),Safe);
    if(!Safe){Result->SetBoolField(TEXT("success"),false);return Result;}
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
    Assert(TEXT("v831_fire_control_real_vehicle_subsystem"),Fleet!=nullptr);
    if(!Fleet){Result->SetBoolField(TEXT("success"),false);return Result;}
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;const auto Vehicles=Fleet->Vehicles;
    const auto Tracers=B->Tracers;const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;
    const auto Phase=B->Phase;const auto Random=B->Random;
    const float Clock=B->Elapsed,Enemy=B->EnemyThinkAt,Extended=B->ExtendedSupportThinkAt,Peak=B->PeakSuppression;
    const int32 Shots=B->ShotsFired,Winner=B->BattleOutroWinner,NextId=B->NextSquadId;
    const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]};
    const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
    const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
    TSet<AActor*> Existing;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Existing.Add(*I);
    struct FHidden{TWeakObjectPtr<AActor> A;bool Hidden,Collision;};TArray<FHidden> Hidden;
    const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AWarfareFrontlineSoldier* U:Units)Hide(U);for(AFrontlineTank* V:Vehicles)Hide(V);
    TArray<AActor*> Owned;
    const auto Clear=[&]()
    {
        for(AActor* A:Owned)if(IsValid(A))A->Destroy();Owned.Reset();B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();
        FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
    };
    ON_SCOPE_EXIT
    {
        Clear();for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Existing.Contains(*I))I->Destroy();
        B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;Fleet->Vehicles=Vehicles;
        B->Tracers=Tracers;B->Strikes=Strikes;B->SmokeZones=Smoke;B->Random=Random;B->Phase=Phase;B->Elapsed=Clock;
        B->EnemyThinkAt=Enemy;B->ExtendedSupportThinkAt=Extended;B->PeakSuppression=Peak;
        B->ShotsFired=Shots;B->BattleOutroWinner=Winner;B->NextSquadId=NextId;
        B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;
        for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];}
        for(const auto& H:Hidden)if(auto* A=H.A.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
        Result->SetBoolField(TEXT("success"),Good);Result->SetArrayField(TEXT("cases"),Rows);
        Result->SetArrayField(TEXT("sight_sweep_changed_pairs"),SightRows);Result->SetNumberField(TEXT("sight_sweep_pairs"),SightPairs);
        Result->SetNumberField(TEXT("sight_sweep_old_short_blocked_corrected_clear"),ShortBlockedClearAtGoal);Result->SetNumberField(TEXT("sight_sweep_goal_blocked_short_clear"),GoalBlockedClearAtShort);
        Result->SetStringField(TEXT("scope"),TEXT("Current UK/DE mission geometry, all three held rifle bays in both directions, home and centre trenches, actual Tick from 20 cm short of the accepted slot, range and physical-wall rejection, old short-stop versus exact-slot actual visibility sweep across downrange/lateral terrain, ordinary rear-field and both complete supported crews. No national campaign, numerical rebalance or whole-campaign acceptance."));
    };
    B->Units.Reset();B->Squads.Reset();Fleet->Vehicles.Reset();B->Strikes.Reset();B->SmokeZones.Reset();
    B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->EnemyThinkAt=B->ExtendedSupportThinkAt=MAX_flt;B->Elapsed=1000;
    const auto Ground=[&](const FVector& P)->FVector{return FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P));};
    const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
    {if(U&&U->Soldier){U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();if(auto* C=FrontlineMachinegunsV30::Get(U))C->TickComponent(DT,LEVELTICK_All,nullptr);}};
    const auto Make=[&](int32 Team,int32 Id,EFrontlineRole Role,int32 Member,const FVector& Feet)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,Team,Role,Member);Owned.Add(U);B->Units.Add(U);U->SetActorTickEnabled(false);
        U->ReturnToIdle();U->SetActorLocation(Ground(Feet)+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
        U->bFieldEntered=true;U->ShotClock=U->GrenadeClock=10000;U->AimSettleClock=U->ReactionSeconds=U->DepartureClock=0;
        U->SetMotion(U->GetAimMotion());Pose(U,.1f);return U;
    };
    const auto EnemyAt=[&](int32 Team,const FVector& Feet){return Make(1-Team,++B->NextSquadId,EFrontlineRole::Rifle,0,Feet);};
    for(int32 Team=0;Team<2;++Team)for(int32 Depth=0;Depth<2;++Depth)for(int32 Slot=0;Slot<3;++Slot)
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team),Home=Depth?FrontlineBattlefield::CentreTrench:B->GetHomeTrenchIndex(Team),Id=++B->NextSquadId;
        if(!B->Trenches.IsValidIndex(Home)){Assert(TEXT("v831_fire_control_actual_home_trench"),false);continue;}
        B->Trenches[Home].Owner=Team;
        FFrontlineSquad S;S.Id=Id;S.Team=Team;S.Role=EFrontlineRole::Rifle;S.Trench=S.Destination=Home;S.Slot=Slot;S.bAdvancing=false;S.bReinforcement=false;
        B->Squads.Add(S);
        for(int32 M=0;M<6;++M)if(auto* U=Make(Team,Id,EFrontlineRole::Rifle,M,FVector(B->Trenches[Home].X,(Slot-1)*630.f+(M-2.5f)*95.f,0)))B->FindSquad(Id)->Members.Add(U);
        auto Members=B->FindSquad(Id)->Members;Assert(FString::Printf(TEXT("v831_team%d_trench%d_bay%d_full_rifle_roster"),Team,Home,Slot),Members.Num()==6);
        for(const auto& W:Members)if(auto* U=W.Get())
        {const FVector Goal=B->SoldierDestination(U);U->SetActorLocation(Ground(Goal-FVector(Dir*20.f,0,0))+FVector(0,0,88));Pose(U,0);}
        double BaySeparationSq=MAX_flt;
        for(int32 I=0;I<Members.Num();++I)for(int32 J=I+1;J<Members.Num();++J)
            if(auto* A=Members[I].Get())if(auto* C=Members[J].Get())
                BaySeparationSq=FMath::Min(BaySeparationSq,FVector::DistSquared2D(B->SoldierDestination(A),B->SoldierDestination(C)));
        const float BaySeparation=FMath::Sqrt(BaySeparationSq);
        Assert(FString::Printf(TEXT("v831_team%d_trench%d_bay%d_displaced_and_nominal_firing_steps_keep_existing_body_reservation"),Team,Home,Slot),BaySeparationSq>=FMath::Square(56.f));
        for(const auto& W:Members)if(auto* U=W.Get())
        {
            const FVector Goal=B->SoldierDestination(U),Start=U->Feet();
            auto* Target=EnemyAt(Team,Goal+FVector(Dir*650.f,0,0));if(!Target){Assert(TEXT("v831_actual_rifle_target_spawned"),false);continue;}
            const bool Before=B->HasSight(U,Target);const int32 Old=U->ShotsDischarged;U->ShotClock=0;
            float FirstShotGap=-1.f;int32 ObservedFrames=0;
            // A valid shot during a stationary bank-detour frame is not proof
            // of finished placement. Observe the real recoil/return until both
            // actions finish, retaining the same six-second cap and assertions.
            for(int32 F=0;F<240&&(U->ShotsDischarged==Old||FVector::Dist2D(U->Feet(),B->SoldierDestination(U))>3.01f||U->bMoving);++F)
            {
                B->Elapsed+=.025f;U->Tick(.025f);Pose(U,.025f);++ObservedFrames;
                if(U->ShotsDischarged>Old){if(FirstShotGap<0)FirstShotGap=FVector::Dist2D(U->Feet(),B->SoldierDestination(U));U->ShotClock=10000;}
            }
            const float Gap=FVector::Dist2D(U->Feet(),B->SoldierDestination(U));
            const FString Prefix=FString::Printf(TEXT("v831_team%d_trench%d_bay%d_rifle%d"),Team,Home,Slot,U->MemberIndex);
            Assert(Prefix+TEXT("_moves_to_actual_firing_step_from_legacy_short_stop"),FVector::Dist2D(Start,U->Feet())>10.f&&Gap<=10.1f&&!U->bMoving);
            Assert(Prefix+TEXT("_fires_through_real_visible_frontage"),U->ShotsDischarged>Old);
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("trench"),Home);Row->SetNumberField(TEXT("slot"),Slot);Row->SetNumberField(TEXT("member"),U->MemberIndex);
            Row->SetBoolField(TEXT("legacy_short_position_sees_enemy"),Before);Row->SetNumberField(TEXT("actual_translation_cm"),FVector::Dist2D(Start,U->Feet()));
            Row->SetNumberField(TEXT("first_shot_gap_cm"),FirstShotGap);Row->SetNumberField(TEXT("observation_seconds"),ObservedFrames*.025f);Row->SetNumberField(TEXT("bay_min_reserved_spacing_cm"),BaySeparation);
            Row->SetNumberField(TEXT("final_gap_cm"),Gap);Row->SetNumberField(TEXT("shots"),U->ShotsDischarged-Old);Row->SetStringField(TEXT("goal"),Goal.ToString());Row->SetStringField(TEXT("feet"),U->Feet().ToString());Rows.Add(MakeShared<FJsonValueObject>(Row));
            // Sweep the actual collision scene, keeping both source positions
            // inside this weapon's unchanged range. A torso/head ray must clear
            // real terrain; the sweep never creates a shot or a fake sight result.
            // Reuse one posed target to avoid thousands of spawning/asset loads.
            if(!Target->IsAlive()){B->Units.Remove(Target);Target->Destroy();Target=EnemyAt(Team,Goal+FVector(Dir*650.f,0,0));}
            if(Target)
            {
                const FVector SavedFeet=U->Feet();const int32 Count=U->ShotsDischarged;const float HP=Target->Health;
                const float Range=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
                for(float Along:{350.f,650.f,1000.f,1350.f,1700.f})for(float Across:{-900.f,-600.f,-300.f,0.f,300.f,600.f,900.f})
                {
                    const FVector To=Ground(Goal+FVector(Dir*Along,Across,0));
                    if(FMath::Abs(To.Y)>FrontlineBattlefield::HalfFrontage-35.f||FVector::Dist2D(Goal,To)>Range)continue;
                    Target->SetActorLocation(To+FVector(0,0,88));Pose(Target,0);U->SetActorLocation(Goal+FVector(0,0,88));
                    const bool AtGoal=B->HasSight(U,Target);
                    for(float Short:{20.f,40.f,58.f})
                    {
                        const FVector OldFeet=Ground(Goal-FVector(Dir*Short,0,0));if(FVector::Dist2D(OldFeet,To)>Range)continue;
                        U->SetActorLocation(OldFeet+FVector(0,0,88));const bool AtShort=B->HasSight(U,Target);++SightPairs;
                        if(!AtShort&&AtGoal)++ShortBlockedClearAtGoal;if(AtShort&&!AtGoal)++GoalBlockedClearAtShort;
                        if(AtShort!=AtGoal&&SightRows.Num()<160)
                        {
                            auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("team"),Team);R->SetNumberField(TEXT("trench"),Home);R->SetNumberField(TEXT("slot"),Slot);R->SetNumberField(TEXT("member"),U->MemberIndex);
                            R->SetNumberField(TEXT("short_cm"),Short);R->SetStringField(TEXT("old_feet"),OldFeet.ToString());R->SetStringField(TEXT("corrected_feet"),Goal.ToString());R->SetStringField(TEXT("target_feet"),To.ToString());
                            R->SetBoolField(TEXT("old_short_sees_target"),AtShort);R->SetBoolField(TEXT("corrected_sees_target"),AtGoal);
                            R->SetNumberField(TEXT("old_bank_intrusion_cm"),FrontlineInfantryV45::BankIntrusion(U,OldFeet));R->SetNumberField(TEXT("goal_bank_intrusion_cm"),FrontlineInfantryV45::BankIntrusion(U,Goal));
                            R->SetBoolField(TEXT("exact_slot_was_physically_admitted"),FrontlineInfantryV45::HasValidatedFiringStepV831(U,Goal));SightRows.Add(MakeShared<FJsonValueObject>(R));
                        }
                    }
                }
                U->SetActorLocation(SavedFeet+FVector(0,0,88));Pose(U,0);
                Assert(Prefix+TEXT("_sight_sweep_cannot_spend_ammunition_or_injure_target"),U->ShotsDischarged==Count&&Target->Health==HP);
                B->Units.Remove(Target);Target->Destroy();
            }
            U->ShotClock=10000;
        }
        if(Depth==0&&Slot==0&&!Members.IsEmpty())if(auto* Occupant=Members[0].Get())
        {
            const FVector Claimed=B->SoldierDestination(Occupant);
            const auto CheckClaim=[&](const TCHAR* State,bool Released)
            {
                FFrontlineSquad Probe=S;Probe.Id=++B->NextSquadId;Probe.Members.Reset();B->Squads.Add(Probe);
                auto* P=Make(Team,Probe.Id,EFrontlineRole::Rifle,0,Claimed);
                bool Pass=false;
                if(P)
                {
                    B->FindSquad(Probe.Id)->Members.Add(P);
                    const FVector Chosen=FrontlineInfantryV45::TrenchSlot(P,Home,Claimed,true);
                    Pass=Released?Chosen.Equals(Claimed,.1f):FVector::DistSquared2D(Chosen,Claimed)>=FMath::Square(56.f);
                    B->Units.Remove(P);P->Destroy();
                }
                B->Squads.RemoveAll([&](const FFrontlineSquad& Q){return Q.Id==Probe.Id;});
                Assert(FString::Printf(TEXT("v831_team%d_%s_firing_reservation"),Team,State),Pass);
            };
            CheckClaim(TEXT("living_held_member_keeps_existing_spacing"),false);
            const float OriginalHealth=Occupant->Health;Occupant->Health=0;
            Assert(FString::Printf(TEXT("v831_team%d_reservation_corpse_remains_a_valid_actor"),Team),IsValid(Occupant)&&!Occupant->IsAlive());
            CheckClaim(TEXT("dead_actor_releases_cached_bay_without_destruction"),true);Occupant->Health=OriginalHealth;
            if(auto* Assignment=B->FindSquad(Id))
            {
                const FFrontlineSquad Saved=*Assignment;Assignment->Trench=INDEX_NONE;Assignment->bAdvancing=true;
                CheckClaim(TEXT("departed_squad_releases_cached_bay"),true);
                if(auto* Restore=B->FindSquad(Id))*Restore=Saved;
            }
        }
        if(!Members.IsEmpty())if(auto* U=Members[0].Get())
        {
            U->ReturnToIdle();U->bMoving=false;Pose(U,.05f);
            auto* Target=EnemyAt(Team,U->Feet()+FVector(Dir*650.f,0,0));
            if(Target)
            {
                auto* Wall=B->GetWorld()->SpawnActor<AActor>();Owned.Add(Wall);auto* Box=NewObject<UBoxComponent>(Wall);
                Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(30,900,1800));
                Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->RegisterComponent();
                Wall->SetActorLocation((U->Feet()+Target->Feet())*.5f+FVector(0,0,120));
                const int32 Old=U->ShotsDischarged,Rounds=U->RoundsRemaining,Combat=B->ShotsFired;const float HP=Target->Health;
                U->ShotClock=0;U->DischargeWeapon(Target);B->Fire(U,Target);
                Assert(FString::Printf(TEXT("v831_team%d_trench%d_bay%d_opaque_wall_blocks_observation_and_both_discharge_paths"),Team,Home,Slot),
                    !B->HasSight(U,Target)&&!B->AcquireTarget(U)&&U->ShotsDischarged==Old&&B->ShotsFired==Combat&&U->RoundsRemaining==Rounds&&Target->Health==HP);
                Wall->Destroy();B->Units.Remove(Target);Target->Destroy();
            }
            const float Range=FrontlineWeapons::ProfileFor(U).FireRangeCm*B->EncounterVisibilityScale();
            auto* Far=EnemyAt(Team,U->Feet()+FVector(Dir*(Range+60.f),0,0));const int32 Old=U->ShotsDischarged;
            if(Far){U->DischargeWeapon(Far);Assert(FString::Printf(TEXT("v831_team%d_trench%d_bay%d_same_profile_rejects_out_of_range"),Team,Home,Slot),!B->AcquireTarget(U)&&U->ShotsDischarged==Old);B->Units.Remove(Far);Far->Destroy();}
        }
    }
    for(int32 Team=0;Team<2;++Team)
    {
        Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Id=++B->NextSquadId;
        const FVector Rear(B->Trenches[Home].X-Dir*900.f,0,0);
        auto* U=Make(Team,Id,EFrontlineRole::Rifle,0,Rear);auto* Target=EnemyAt(Team,Rear+FVector(Dir*450.f,0,0));
        if(U&&Target)
        {
            U->ReturnToIdle();U->bMoving=false;U->ShotClock=0;Pose(U,.05f);const int32 Before=U->ShotsDischarged;U->DischargeWeapon(Target);
            Assert(FString::Printf(TEXT("v831_team%d_rear_field_remains_a_valid_combat_area"),Team),U->ShotsDischarged>Before);
        }
        Clear();B->Trenches[Home].Owner=Team;
        FFrontlineSquad Crew;Crew.Id=++B->NextSquadId;Crew.Team=Team;Crew.Role=EFrontlineRole::Lewis;Crew.Trench=Crew.Destination=Home;Crew.Slot=1;Crew.bAdvancing=false;Crew.bReinforcement=false;B->Squads.Add(Crew);
        for(int32 M=0;M<3;++M)if(auto* Man=Make(Team,Crew.Id,EFrontlineRole::Lewis,M,FVector(B->Trenches[Home].X,(M-1)*110.f,0)))B->FindSquad(Crew.Id)->Members.Add(Man);
        auto CrewMen=B->FindSquad(Crew.Id)->Members;
        for(const auto& W:CrewMen)if(auto* Man=W.Get())
        {bool Reform=false;const FVector Goal=FrontlineMachinegunsV30::Destination(Man,B->SoldierDestination(Man),Reform);Man->SetActorLocation(Ground(Goal-FVector(Dir*20.f,0,0))+FVector(0,0,88));Man->ShotClock=0;Pose(Man,.05f);}
        for(int32 I=0;I<6;++I)EnemyAt(Team,FVector(B->Trenches[Home].X+Dir*(1000.f+I*80.f),-250.f+I*100.f,0));
        for(int32 F=0;F<400;++F)
        {
            B->Elapsed+=.025f;bool Done=true;
            for(const auto& W:CrewMen)if(auto* Man=W.Get())
            {if(Man->ShotsDischarged>0)Man->ShotClock=10000;else Done=false;Man->Tick(.025f);Pose(Man,.025f);}
            if(Done)break;
        }
        for(const auto& W:CrewMen)if(auto* Man=W.Get())
        {
            Assert(FString::Printf(TEXT("v831_team%d_supported_member%d_fires_from_actual_approach"),Team,Man->MemberIndex),Man->ShotsDischarged>0&&!Man->bMoving);
            Assert(FString::Printf(TEXT("v831_team%d_supported_member%d_faces_contact_without_stationary_running"),Team,Man->MemberIndex),
                Man->MotionSpeed<=8.f&&FMath::Abs(FMath::FindDeltaAngleDegrees(Man->GetActorRotation().Yaw,Dir>0?0.f:180.f))<75.f);
        }
    }
    return Result;
}
};

TSharedRef<FJsonObject> FrontlineInfantryV45::RunFireControlChecksV831(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    return FFrontlineFireControlV831Review::Run(B,Check);
}

TSharedRef<FJsonObject> FrontlineInfantryV45::RunTrenchFireChecksV844(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    return FFrontlineFireControlV831Review::RunV844(B,Check);
}
