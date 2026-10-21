#include "WarfareFrontlineMachinegunV36Review.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineInfantryV45.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

struct FFrontlineMGReviewV36
{
static void Pose(AWarfareFrontlineSoldier* U,float DT)
{
    U->Soldier->HandleExistingParallelEvaluationTask(true,true);
    U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
    if(auto* C=FrontlineMachinegunsV30::Get(U))C->TickComponent(DT,LEVELTICK_All,nullptr);
}
static void Firing(AWarfareFrontlineGameMode* B,AWarfareFrontlineSoldier* U,TFunctionRef<void(const FString&,bool)> Check)
{
    auto* C=FrontlineMachinegunsV30::Get(U);
    if(!C||!C->IsTrench()||!C->CanFire())return;
    const FString Suffix=TEXT("_")+FString::FromInt(U->Team);
    const int32 Dir=B->GetTeamDirection(U->Team),Before=U->ShotsDischarged;
    FVector P=U->Feet()+FVector(Dir*900.f,0,0);P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
    auto* Target=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(P+FVector(0,0,88),FRotator(0,Dir>0?180:0,0));
    Check(TEXT("trench_mg_live_infantry_target_created")+Suffix,Target!=nullptr);if(!Target)return;
    AActor* Wall=nullptr;
    ON_SCOPE_EXIT
    {
        if(IsValid(Wall))Wall->Destroy();
        B->Units.Remove(Target);Target->Destroy();U->FocusTarget.Reset();
        U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->FirePoseClock=0;U->BurstRemaining=0;
        U->SetMotion(U->GetAimMotion());Pose(U,.25f);
    };
    Target->Initialize(B,936000+U->Team,1-U->Team,EFrontlineRole::Rifle,0);
    Target->SetActorTickEnabled(false);Target->bFieldEntered=true;Target->bMoving=false;Target->MotionSpeed=0;
    B->Units.Add(Target);Pose(Target,.25f);U->SetMotion(U->GetAimMotion());Pose(U,.25f);
    Check(TEXT("trench_mg_acquires_target_beyond_real_firing_lip")+Suffix,B->AcquireTarget(U)==Target&&C->HasInfantryFiringLine(Target));
    const FVector Observation=FrontlineMachinegunsV30::SightOrigin(U);
    Check(TEXT("trench_mg_observes_from_supported_weapon")+Suffix,Observation.Equals(U->GetMuzzleLocation(),.01f));
    Wall=B->GetWorld()->SpawnActor<AActor>();Check(TEXT("trench_mg_occlusion_fixture_created")+Suffix,Wall!=nullptr);if(!Wall)return;
    auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Wall->AddInstanceComponent(Block);
    Block->SetBoxExtent(FVector(8,180,1500));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
    // Reproduce a sheltered head behind a breastwork whose barrel projects
    // through the firing gap. A clear barrel must not inherit head occlusion.
    const FVector Head=U->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,8);
    Wall->SetActorLocation((Head+U->GetMuzzleLocation())*.5f);
    FCollisionQueryParams HeadQuery(SCENE_QUERY_STAT(V36TrenchMGHead),true);
    for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)HeadQuery.AddIgnoredActor(Other);
    FHitResult HeadHit;
    const bool HeadBlocked=B->GetWorld()->LineTraceSingleByChannel(HeadHit,Head,FrontlineMachinegunsV30::SightTarget(Target),ECC_Visibility,HeadQuery);
    Check(TEXT("trench_mg_sheltered_head_does_not_hide_a_clear_firing_gap")+Suffix,
        HeadBlocked&&B->AcquireTarget(U)==Target&&C->HasInfantryFiringLine(Target));
    // Exercise the real feed animation. Looking down to reload cannot make the
    // section forget a still-visible target or repeatedly repack its weapon.
    const int32 Deployments=C->Deployments,Packs=C->Packs;
    U->RoundsRemaining=FMath::Max(1,U->GetMagazineCapacity()-1);U->StartReload();
    int32 ReloadFrames=0;bool Retained=true,StationaryOrigin=true;
    for(int32 I=0;I<300&&U->WeaponPhase==EFrontlineWeaponPhase::Reload;++I)
    {
        U->UpdateWeaponCycle(.05f);C->Update(.05f,false);Pose(U,.05f);
        if(U->WeaponPhase==EFrontlineWeaponPhase::Reload)
        {
            ++ReloadFrames;Retained&=B->AcquireTarget(U)==Target;
            StationaryOrigin&=FrontlineMachinegunsV30::SightOrigin(U).Equals(Observation,.01f);
        }
    }
    Check(TEXT("trench_mg_reload_preserves_observation_without_redeploy")+Suffix,
        ReloadFrames>10&&Retained&&StationaryOrigin&&U->WeaponPhase==EFrontlineWeaponPhase::Ready
        &&C->Deployments==Deployments&&C->Packs==Packs&&C->CanFire());
    U->SetMotion(U->GetAimMotion());Pose(U,.25f);
    Block->SetBoxExtent(FVector(25,260,1500));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Wall->SetActorLocation((U->GetMuzzleLocation()+FrontlineMachinegunsV30::SightTarget(Target))*.5f);
    const bool Hidden=!B->HasSight(U,Target)&&B->AcquireTarget(U)==nullptr&&!C->HasInfantryFiringLine(Target);
    U->DischargeWeapon(Target);
    Check(TEXT("trench_mg_real_wall_blocks_awareness_and_discharge")+Suffix,Hidden&&U->ShotsDischarged==Before);
    Wall->SetActorEnableCollision(false);Wall->Destroy();Wall=nullptr;
    // The ordinary discharge entry point must still enforce the stance gate.
    C->State=EFrontlineGunStanceV30::Deploy;U->DischargeWeapon(Target);
    Check(TEXT("trench_mg_deployment_still_blocks_discharge")+Suffix,U->ShotsDischarged==Before);
    C->State=EFrontlineGunStanceV30::Supported;
    U->DischargeWeapon(Target);
    Check(TEXT("trench_mg_really_fires_after_clear_line_and_completed_deployment")+Suffix,U->ShotsDischarged==Before+1);
    UE_LOG(LogTemp,Display,TEXT("WF_V36_MG_FIRE team=%d reload_frames=%d observed_during_reload=%d deployments=%d packs=%d actual_shots=%d"),
        U->Team,ReloadFrames,Retained,C->Deployments,C->Packs,U->ShotsDischarged-Before);
}
static void ArrivalV61(AWarfareFrontlineGameMode* B,AWarfareFrontlineSoldier* U,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV36Review")))return;
    auto* C=FrontlineMachinegunsV30::Get(U);auto* Squad=B->FindSquad(U->SquadId);
    if(!C||!Squad||Squad->bAdvancing||!C->IsTrench())return;
    AWarfareFrontlineSoldier* Escort=nullptr;
    for(const auto& W:Squad->Members)if(auto* Man=W.Get();Man&&Man!=U&&Man->IsAlive()&&!Man->IsMachineGunner()){Escort=Man;break;}
    const FString Suffix=TEXT("_")+FString::FromInt(U->Team);
    Check(TEXT("v61_live_arrival_has_gunner_and_rifle_escort")+Suffix,Escort!=nullptr);if(!Escort)return;
    const auto GunTransform=U->GetActorTransform(),EscortTransform=Escort->GetActorTransform();
    const float OldTime=B->Elapsed,GunShot=U->ShotClock,EscortShot=Escort->ShotClock;
    const int32 Dir=B->GetTeamDirection(U->Team),Packs=C->Packs,Deployments=C->Deployments;
    AWarfareFrontlineSoldier* Rear=nullptr;
    ON_SCOPE_EXIT
    {
        if(Rear){B->Units.Remove(Rear);Rear->Destroy();}
        U->SetActorTransform(GunTransform);Escort->SetActorTransform(EscortTransform);B->Elapsed=OldTime;
        for(auto* Man:{U,Escort}){Man->ReturnToIdle();Man->SetMotion(Man->GetAimMotion());Pose(Man,.1f);}
        U->ShotClock=GunShot;Escort->ShotClock=EscortShot;
    };
    const FVector Station=FrontlineMachinegunsV30::TrenchFiringStepV36(U,B->SoldierDestination(U));
    FVector Approach=Station-FVector(Dir*80.f,0,0);Approach.Z=FrontlineInfantryV45::SurfaceHeight(B,Approach);
    C->State=EFrontlineGunStanceV30::Carry;U->ReturnToIdle();U->SetActorLocation(Approach+FVector(0,0,88));
    U->SetActorRotation(FRotator(0,(Dir>0?0.f:180.f)+65.f,0));U->SetMotion(U->GetWalkMotion());Pose(U,.1f);
    U->ShotClock=1000;U->DepartureClock=0;U->bWantedMovement=true;
    bool Travelled=false,Deployed=false,NoIdleAtArrival=true,Planted=true,FireGate=true;int32 DeployFrames=0;
    FVector PlantedAt=FVector::ZeroVector;
    for(int32 I=0;I<240&&!C->IsSupported();++I)
    {
        const FVector Before=U->Feet();B->Elapsed+=.02f;U->Tick(.02f);Pose(U,.02f);
        Travelled|=FVector::Dist2D(Before,U->Feet())>.01f;
        if(Travelled&&FVector::Dist2D(U->Feet(),Station)<14.1f&&!U->bMoving&&C->State==EFrontlineGunStanceV30::Carry)NoIdleAtArrival=false;
        if(C->State==EFrontlineGunStanceV30::Deploy)
        {
            if(!Deployed){Deployed=true;PlantedAt=U->Feet();}
            ++DeployFrames;Planted&=U->Feet().Equals(PlantedAt,.01f);FireGate&=!C->CanFire()&&U->Playing==C->TransitionMotion();
        }
    }
    Check(TEXT("v61_actual_trench_walk_flows_into_deploy_without_standing_reset")+Suffix,
        Travelled&&Deployed&&NoIdleAtArrival&&C->IsSupported()&&C->Deployments==Deployments+1&&C->Packs==Packs);
    Check(TEXT("v61_full_deployment_keeps_actor_stationary_and_fire_blocked")+Suffix,DeployFrames>10&&Planted&&FireGate&&C->CanFire());
    Check(TEXT("v61_completed_arrival_faces_front_without_repacking")+Suffix,
        FVector::DotProduct(U->GetActorForwardVector(),FVector(Dir,0,0))>.98f);

    FVector EscortStation=B->SoldierDestination(Escort);Escort->SetActorLocation(EscortStation+FVector(0,0,88));
    Escort->ReturnToIdle();Escort->SetActorRotation(FRotator(0,Dir>0?180:0,0));
    Escort->FocusTarget=U;Escort->TargetReviewClock=10;Escort->WeaponPhase=EFrontlineWeaponPhase::BoltCycle;
    Escort->FirePoseClock=2;Escort->ShotClock=1000;Escort->SetMotion(Escort->GetAimMotion());
    for(int32 I=0;I<40;++I){B->Elapsed+=.02f;Escort->Tick(.02f);Pose(Escort,.02f);}
    Check(TEXT("v61_escort_rejects_friendly_focus_and_restores_watch_during_bolt_action")+Suffix,
        !Escort->FocusTarget.IsValid()&&Escort->WeaponPhase==EFrontlineWeaponPhase::BoltCycle
        &&FVector::DotProduct(Escort->GetActorForwardVector(),FVector(Dir,0,0))>.98f);
    Rear=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    if(Rear)
    {
        Rear->Initialize(B,961000+U->Team,1-U->Team,EFrontlineRole::Rifle,0);Rear->SetActorTickEnabled(false);Rear->bFieldEntered=true;
        Rear->SetActorLocation(Escort->Feet()-FVector(Dir*90.f,0,-88.f));B->Units.Add(Rear);Pose(Rear,.1f);
        Escort->FocusTarget=Rear;Escort->TargetReviewClock=0;Escort->FirePoseClock=0;Escort->WeaponPhase=EFrontlineWeaponPhase::Ready;
        for(int32 I=0;I<40;++I){B->Elapsed+=.02f;Escort->Tick(.02f);Pose(Escort,.02f);}
    }
    Check(TEXT("v61_escort_still_turns_to_a_real_visible_rear_enemy")+Suffix,
        Rear&&FrontlineMachinegunsV30::ValidContactV61(Escort,Rear)&&Escort->FocusTarget==Rear
        &&FVector::DotProduct(Escort->GetActorForwardVector(),FVector(Dir,0,0))<-.98f);
    UE_LOG(LogTemp,Display,TEXT("WF_V61_MG_ARRIVAL team=%d travelled=%d deployed=%d no_idle=%d frames=%d planted=%d fire_gate=%d"),
        U->Team,Travelled,Deployed,NoIdleAtArrival,DeployFrames,Planted,FireGate);
}
static bool Setup(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check,int32 Team)
{
    // The V68 art review reuses this exact fixture for same-scene comparison.
    if(!B||!B->bProgressionVolatile||(!FParse::Param(FCommandLine::Get(),TEXT("WFV36Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview"))))return false;
    B->ResetEncounter();B->SelectPlayerFaction(EFrontlineFaction::British);B->BeginBattle();
    B->SetActorTickEnabled(false);bool Ready=B->Phase==EFrontlinePhase::Fighting;
    B->Trenches[FrontlineBattlefield::CentreTrench].Owner=Team;
    {
        if(!B->SpawnSquad(Team,EFrontlineRole::Lewis,FrontlineBattlefield::CentreTrench))return false;
        auto& S=B->Squads.Last();S.Slot=1;
        for(const auto& W:S.Members)if(auto* U=W.Get())
        {
            FVector P=B->SoldierDestination(U);P=FrontlineMachinegunsV30::TrenchFiringStepV36(U,P);
            U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0:180,0));
            U->bFieldEntered=true;U->SetActorTickEnabled(false);U->bMoving=false;U->MotionSpeed=0;U->WeaponPhase=EFrontlineWeaponPhase::Ready;
            if(auto* C=FrontlineMachinegunsV30::Get(U))for(int32 I=0;I<70&&!C->IsSupported();++I)C->Update(.1f,false);
            U->SetMotion(U->GetAimMotion());
            if(U->IsMachineGunner())
            {
                Check(TEXT("gunner_firing_step_inside_cover_")+FString::FromInt(Team),B->CoverFor(U)>0.f);
                const FVector D=FrontlineMachinegunsV30::TrenchFiringStepV36(U,B->SoldierDestination(U));
                Check(TEXT("gunner_step_destination_stable_")+FString::FromInt(Team),FVector::Dist2D(D,U->Feet())<.01f);
                const bool Old=S.bAdvancing;S.bAdvancing=true;
                const FVector March(1555,202,33);
                Check(TEXT("gunner_advance_does_not_snap_back_to_step_")+FString::FromInt(Team),FrontlineMachinegunsV30::TrenchFiringStepV36(U,March).Equals(March));S.bAdvancing=Old;
            }
        }
    }
    B->LastMessage.Empty();return Ready;
}
};
bool SetupV36TrenchCrew(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check,int32 Team){return FFrontlineMGReviewV36::Setup(B,Check,Team);}
void InspectV36TrenchCrew(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    int32 Gunners=0;bool AllReady=true,NoTowers=true;
    TArray<AWarfareFrontlineSoldier*> GunChecks;
    for(const auto& W:B->Units)if(auto* U=W.Get();U&&U->IsAlive())
    {
        TInlineComponentArray<UInstancedStaticMeshComponent*> Components(U);
        for(const auto* C:Components)NoTowers&=!C->GetName().Contains(TEXT("MachinegunSandbag"));
        if(!U->IsMachineGunner())continue;++Gunners;GunChecks.Add(U);const auto* C=FrontlineMachinegunsV30::Get(U);AllReady&=C&&C->IsTrench()&&C->CanFire();
        Check(TEXT("gunner_two_real_bipod_contacts_")+FString::FromInt(U->Team),C&&C->HasBipodContactV36());
        Check(TEXT("gunner_firing_bay_keeps_body_clear_of_real_bank_")+FString::FromInt(U->Team),FrontlineInfantryV45::BankIntrusion(U,U->Feet())<=8.1f);
        const FVector Weapon=U->Soldier->GetBoneLocation(TEXT("weapon_rifle"));const FVector Forward=(U->GetMuzzleLocation()-Weapon).GetSafeNormal();
        const FVector Pivot=Weapon+Forward*36.f;
        float Surface=AWarfareFrontlineGround::HeightAt(Pivot.X,Pivot.Y),Earthwork=0;
        if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,Pivot,Earthwork))Surface=FMath::Max(Surface,Earthwork);
        const float Gap=Pivot.Z-Surface;
        UE_LOG(LogTemp,Display,TEXT("WF_V36_MG_CONTACT team=%d gun_height_cm=%.2f lip_gap_cm=%.2f step_offset_cm=%.2f bay_side_shift_cm=%.2f body_bank_intrusion_cm=%.2f"),U->Team,Weapon.Z-U->Feet().Z,Gap,
            (U->Feet().X-FrontlineFieldV32::TrenchCentreX(FrontlineBattlefield::CentreTrench,U->Feet().Y))*B->GetTeamDirection(U->Team),
            U->Feet().Y-B->SoldierDestination(U).Y,FrontlineInfantryV45::BankIntrusion(U,U->Feet()));
        Check(TEXT("gunner_not_buried_in_parapet_")+FString::FromInt(U->Team),Gap>2.f&&Gap<55.f);
    }
    Check(TEXT("actual_crew_supported_on_its_owned_firing_lip"),Gunners==1&&AllReady);
    Check(TEXT("no_personal_sandbag_tower_components"),NoTowers);
    for(auto* U:GunChecks){FFrontlineMGReviewV36::Firing(B,U,Check);FFrontlineMGReviewV36::ArrivalV61(B,U,Check);}
}
