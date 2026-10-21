#include "WarfareFrontline.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMotionV34.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineCharacterV29.h"
#include "WarfareFrontlineCharacterV34.h"
#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlinePacingV24.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
#include "WarfareFrontlineBlockedFireProbeV87.h"
#endif

bool FrontlineUseLegacyCombatV835();

float FrontlineWeapons::GrenadierPreparationRateBoostV835(const AWarfareFrontlineSoldier* Unit,const AWarfareFrontlineSoldier* Target,bool ObservedThisTick)
{
    if(!Unit||!Unit->Battle||FrontlineUseLegacyCombatV835()
        ||Unit->Role!=EFrontlineRole::Mills||Unit->IsStormtrooper()||!Unit->IsActiveCombatant())return 0.f;
    const auto Eligible=[&](const AWarfareFrontlineSoldier* Enemy)
    {
        return IsValid(Enemy)&&Enemy->IsActiveCombatant()&&Enemy->Team!=Unit->Team&&Enemy->Battle==Unit->Battle
            &&Unit->Battle->CoverFor(Enemy)>0.f
            &&FVector::Dist2D(Unit->Feet(),Enemy->Feet())<=FrontlineGrenade::MaxRangeCm;
    };
    // Advancing contact uses the shorter stopping distance. During accepted
    // travel, observe a covered enemy inside the physical hand-bomb range for
    // preparation only; this never writes the firing target or orders a stop.
    if(!Target&&Unit->bMoving)
    {
        float Nearest=MAX_flt;
        for(const auto& Enemy:Unit->Battle->Units)
            if(Eligible(Enemy))
            {
                const float Distance=FVector::Dist2D(Unit->Feet(),Enemy->Feet());
                if(Distance<Nearest&&Unit->Battle->HasSight(Unit,Enemy)){Target=Enemy;Nearest=Distance;}
            }
        ObservedThisTick=Target!=nullptr;
    }
    if(!Eligible(Target)||(!ObservedThisTick&&!Unit->Battle->HasSight(Unit,Target)))return 0.f;
    // A visible trench position lets a bomb team prepare on its final approach.
    // Throwing still requires the original stationary, aim and weapon clocks.
    // Open contacts, Stormtroopers and the separate armour clock are unchanged.
    return FrontlinePacingV24::GrenadeReadinessRate*(1.f-FrontlineGrenade::ReadinessRateMultiplier);
}

namespace
{
float ActionLength(UAnimationAsset* Asset)
{
    const UAnimSequenceBase* Clip=Cast<UAnimSequenceBase>(Asset);
    return Clip?Clip->GetPlayLength():0.f;
}
}

int32 AWarfareFrontlineSoldier::GetMagazineCapacity() const
{
    if(WarfareFRRUCampaignV82::Active(Battle))
    {const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(this);return Equipment?Equipment->Capacity:0;}
    return MemberIndex==0&&(Role==EFrontlineRole::RifleGrenadier||Role==EFrontlineRole::AntiTankRifle)?1:IsCommissionedOfficer()?(GetFaction()==EFrontlineFaction::British?6:8):IsStormtrooper()?32:IsMachineGunner()?(GetFaction()==EFrontlineFaction::British?47:100):(GetFaction()==EFrontlineFaction::British?10:5);
}

void AWarfareFrontlineSoldier::StartReload()
{
    WeaponPhase=EFrontlineWeaponPhase::Reload;
    ++ReloadsStarted;
    BurstRemaining=0;
    // One physical magazine reload blocks firing; no timer refills ammunition invisibly.
    SetMotion(GetReloadMotion(),false);
    const float ReloadScale=FMath::Clamp(Battle->WeaponReloadScale(Team),.7f,1.f);
    if (auto* Node=Soldier->GetSingleNodeInstance()) Node->SetPlayRate(1.f/ReloadScale);
    FirePoseClock=FMath::Max(.01f,ActionLength(GetReloadMotion())*ReloadScale);
    if(FrontlineSpecialistsV41::IsSpecialist(this))
    {
        FirePoseClock=FrontlineSpecialistsV41::ReloadSeconds(this)*ReloadScale;
        if(auto* Node=Soldier->GetSingleNodeInstance())Node->SetPlayRate(ActionLength(GetReloadMotion())/FMath::Max(.1f,FirePoseClock));
    }
    if(WarfareFRRUCampaignV82::Active(Battle))
    {
        const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(this);
        if(!Equipment||Equipment->Capacity<=0||Equipment->ReloadSeconds<=0)
        {FirePoseClock=MAX_flt;ShotClock=MAX_flt;return;}
        // An imported action illustrates the native firearm timing; its clip
        // duration never grants a faster national reload or classic modifier.
        FirePoseClock=Equipment->ReloadSeconds;
        if(auto* Node=Soldier->GetSingleNodeInstance())Node->SetPlayRate(ActionLength(GetReloadMotion())/FMath::Max(.1f,FirePoseClock));
    }
    ReloadDurationSecondsV82=FirePoseClock;
    ShotClock=FMath::Max(ShotClock,FirePoseClock+.18f+ReactionSeconds*.24f);
    if(IsAutomaticWeapon() && ShotsDischarged>0 && bEngaging)
        for(AWarfareFrontlineSoldier* Guard:Battle->Units)
            if(Guard && Guard!=this && Guard->IsAlive() && Guard->SquadId==SquadId
                && !Guard->IsAutomaticWeapon() && FVector::Dist2D(Guard->Feet(),Feet())<900.f)
            {
                if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
                    Audio->PlayVoice(EFrontlineVoiceEvent::ReloadCover,GetFaction()==EFrontlineFaction::German,Feet(),SquadId);
                break;
            }
}

float AWarfareFrontlineSoldier::GetWeaponReloadNormalizedPhase() const
{
    if(!IsActiveCombatant()||WeaponPhase!=EFrontlineWeaponPhase::Reload||!FMath::IsFinite(ReloadDurationSecondsV82)
        ||ReloadDurationSecondsV82<=0||ReloadDurationSecondsV82>=MAX_flt)return 0;
    return FMath::Clamp(1.f-FirePoseClock/ReloadDurationSecondsV82,0.f,1.f);
}

void AWarfareFrontlineSoldier::UpdateWeaponCycle(float DT)
{
    FirePoseClock=FMath::Max(0.f,FirePoseClock-DT);
    if (WeaponPhase==EFrontlineWeaponPhase::OfficerCommand)
    {
        if (FirePoseClock<=0) {WeaponPhase=EFrontlineWeaponPhase::Ready;SetMotion(GetAimMotion());}
        return;
    }
    const bool Prepared=Battle && (Battle->HasFactionTechnology(EFrontlineFaction::British,15,Team)
        || (IsStormtrooper() && Battle->HasFactionTechnology(EFrontlineFaction::German,15,Team)));
    const bool Quiet=Prepared && WeaponPhase==EFrontlineWeaponPhase::Ready && !bMoving
        && Battle->CoverFor(this)>0.f && !Battle->AcquireTarget(this) && Suppression<.3f;
    QuietReloadClock=Quiet?QuietReloadClock+DT:0.f;
    if (QuietReloadClock>=5.f && RoundsRemaining<GetMagazineCapacity()/2)
    { QuietReloadClock=0.f; StartReload(); return; }
    if (WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow)
    {
        if (FirePoseClock>0) return;
        WeaponPhase=EFrontlineWeaponPhase::Ready; AimSettleClock=.25f;
        SetMotion(GetAimMotion()); return;
    }
    if (FirePoseClock>0 || WeaponPhase==EFrontlineWeaponPhase::Ready) return;
    if (WeaponPhase==EFrontlineWeaponPhase::Recoil)
    {
        if (RoundsRemaining<=0) { StartReload(); return; }
        if (!IsAutomaticWeapon() && !IsCommissionedOfficer())
        {
            WeaponPhase=EFrontlineWeaponPhase::BoltCycle;
            // Cover is a tactical state, not an order to squat after every shot.
            // The neutral bolt clip preserves the firing step and hand contacts.
            UAnimationAsset* Cycle=Battle->BoltAnimation.Get();
            if(auto* Supported=FrontlineMachinegunsV30::Action(this,TEXT("Bolt")))Cycle=Supported;
            SetMotion(Cycle,false);
            FirePoseClock=FMath::Max(.01f,ActionLength(Cycle));
            return;
        }
    }
    else if (WeaponPhase==EFrontlineWeaponPhase::Reload)
    {
        RoundsRemaining=GetMagazineCapacity(); ++ReloadsCompleted;
        AimSettleClock=FMath::Max(AimSettleClock,MotionRandom.FRandRange(.19f,.43f));
    }
    else if (WeaponPhase==EFrontlineWeaponPhase::BoltCycle)
    {
        ++BoltCyclesCompleted;
        // Shared clip lengths must not pull a whole squad back into lockstep.
        // Each man independently reacquires his sight picture after working the bolt.
        AimSettleClock=FMath::Max(AimSettleClock,MotionRandom.FRandRange(.12f,.58f));
    }
    WeaponPhase=EFrontlineWeaponPhase::Ready;
    // Do not cut the automatic recoil loop between rounds of the same burst.
    if (!IsAutomaticWeapon() || BurstRemaining==0) SetMotion(GetAimMotion());
}

void AWarfareFrontlineSoldier::StartGrenadeThrow(AWarfareFrontlineSoldier* Target,AFrontlineTank* VehicleTarget)
{
    const bool InfantryTarget=Target && Target->IsActiveCombatant() && Target->Team!=Team;
    const bool ArmourTarget=VehicleTarget && VehicleTarget->IsOperational() && VehicleTarget->HasEnteredField() && VehicleTarget->Team!=Team;
    if (!Battle || !IsActiveCombatant() || (!InfantryTarget && !ArmourTarget) || !Battle->GrenadeAnimation
        || Role!=EFrontlineRole::Mills || bMoving || WeaponPhase!=EFrontlineWeaponPhase::Ready) return;
    const FVector Observed=InfantryTarget?Target->Feet():VehicleTarget->GrenadeAimPointFor(Feet());
    const float Range=FVector::Dist2D(Feet(),Observed);if(Range>FrontlineGrenade::MaxRangeCm)return;
    if(InfantryTarget)GrenadeAimPoint=FrontlineGrenade::EstimateAim(this,Target);
    else
    {
        // Same poor throwing precision and snapshot aim as infantry targets.
        // A tank does not grant homing grenades or bypass the release animation.
        const float Scatter=FrontlineGrenade::AimScatterMultiplier*(FrontlineGrenade::BaseScatterCm
            +FrontlineGrenade::RangeScatterCm*FMath::Square(Range/FrontlineGrenade::TrajectoryReferenceRangeCm)
            +Suppression*FrontlineGrenade::PressureScatterCm);
        const float Angle=MotionRandom.FRandRange(0,2*PI),Radius=FMath::Sqrt(MotionRandom.FRand())*Scatter;
        GrenadeAimPoint=Observed+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius*FrontlineGrenade::CrossRangeScale,0);
        GrenadeAimPoint.Z=AWarfareFrontlineGround::HeightAt(GrenadeAimPoint.X,GrenadeAimPoint.Y);
    }
    GrenadeReleased=false; ++GrenadeThrowsStarted;
    WeaponPhase=EFrontlineWeaponPhase::GrenadeThrow; SetMotion(Battle->GrenadeAnimation,false);
    FirePoseClock=ActionLength(Battle->GrenadeAnimation); ShotClock=FMath::Max(ShotClock,FirePoseClock+.25f);
}

void AWarfareFrontlineSoldier::DischargeWeapon(AWarfareFrontlineSoldier* Target,AFrontlineTank* VehicleTarget)
{
#if WITH_EDITOR
#define WF_BLOCKED_FIRE_RESULT_V87(Reason) FrontlineBlockedFireProbeV87::Discharge(this,Target,VehicleTarget,TEXT(Reason))
#else
#define WF_BLOCKED_FIRE_RESULT_V87(Reason)
#endif
    const bool InfantryTarget=Target && Target->IsActiveCombatant() && Target->Team!=Team;
    const bool ArmourTarget=VehicleTarget && VehicleTarget->IsOperational() && VehicleTarget->HasEnteredField() && VehicleTarget->Team!=Team;
    if (!Battle || (!InfantryTarget && !ArmourTarget) || !IsActiveCombatant() || bMoving || WeaponPhase!=EFrontlineWeaponPhase::Ready
        || Battle->Phase!=EFrontlinePhase::Fighting)
    {
#if WITH_EDITOR
        FrontlineBlockedFireProbeV87::Discharge(this,Target,VehicleTarget,!Battle?TEXT("battle_missing"):!InfantryTarget&&!ArmourTarget?TEXT("target_invalid"):!IsActiveCombatant()?TEXT("unit_inactive"):bMoving?TEXT("moving"):WeaponPhase!=EFrontlineWeaponPhase::Ready?TEXT("weapon_not_ready"):TEXT("battle_not_fighting"));
#endif
        return;
    }
    if(!FrontlineMachinegunsV30::CanFire(this)){WF_BLOCKED_FIRE_RESULT_V87("machinegun_can_fire_refused");return;}
    if(InfantryTarget&&WarfareFRRUCampaignV82::Active(Battle)&&!WarfareFRRUWorldV82::FiringLine(this,Target)){WF_BLOCKED_FIRE_RESULT_V87("national_actual_muzzle_firing_line_refused");return;}
    // Perception may retain a target until its next review. Recheck the existing
    // observation points before spending a round; a newly hidden man is no shot.
    // Supported crews still need the separate current-muzzle test below.
    if(InfantryTarget&&!WarfareFRRUCampaignV82::Active(Battle)&&!Battle->HasSight(this,Target)){WF_BLOCKED_FIRE_RESULT_V87("observation_sight_refused");return;}
    if(ArmourTarget&&!FrontlineSpecialistsV41::IsRifleGrenadier(this))
    {
        // Stable observation can retain contact through reload. The actual
        // discharge still needs a clear muzzle, including every terrain bank.
        FCollisionQueryParams Q(SCENE_QUERY_STAT(V63InfantryArmourShot),true);Q.AddIgnoredActor(VehicleTarget);
        for(const AWarfareFrontlineSoldier* U:Battle->Units)if(U)Q.AddIgnoredActor(U);
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,GetMuzzleLocation(),VehicleTarget->AimPoint(),ECC_Visibility,Q)){WF_BLOCKED_FIRE_RESULT_V87("armour_actual_muzzle_trace_refused");return;}
    }
    // Tactical observation survives stance/feed motion; actual supported fire
    // always clears the current muzzle, on open ground as well as a trench lip.
    if(InfantryTarget)
        if(const auto* Gun=FrontlineMachinegunsV30::Get(this);Gun&&Gun->IsSupported()&&!Gun->HasInfantryFiringLine(Target)){WF_BLOCKED_FIRE_RESULT_V87("supported_actual_muzzle_firing_line_refused");return;}
    const FrontlineWeapons::FProfile& Profile=FrontlineWeapons::ProfileFor(this);
    // A cached target may have left range since the last perception refresh.
    const bool HeavyRifleBomb=ArmourTarget&&!InfantryTarget&&FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(this);
    const FVector ArmourAim=ArmourTarget?(HeavyRifleBomb?VehicleTarget->ObservedArmourAimPointV837(Feet()):VehicleTarget->AimPoint()):FVector::ZeroVector;
    const float RangeDistance=InfantryTarget?FrontlineWeapons::DirectFireRangeDistanceCm(this,Target->Feet()):FVector::Dist2D(Feet(),ArmourAim);
    const float RangeLimit=HeavyRifleBomb?FrontlineSpecialistsV41::AntiTankRifleGrenadeRangeCmV836:Profile.FireRangeCm;
    if (RangeDistance>RangeLimit*Battle->EncounterVisibilityScale()){WF_BLOCKED_FIRE_RESULT_V87("actual_weapon_range_refused");return;}
    if(FrontlineFieldV32::IsFlameOperator(this)){FrontlineFieldV32::FlameDischarge(this,Target);ShotClock=.3f;WF_BLOCKED_FIRE_RESULT_V87("flame_delegate");return;}
    if (RoundsRemaining<=0) { StartReload();WF_BLOCKED_FIRE_RESULT_V87("empty_magazine_started_reload");return; }
    if(FrontlineSpecialistsV41::IsSpecialist(this))
    {
        if(!FrontlineSpecialistsV41::Fire(this,InfantryTarget?Target:nullptr,ArmourTarget?VehicleTarget:nullptr,MotionRandom)){WF_BLOCKED_FIRE_RESULT_V87("specialist_fire_refused");return;}
        ++ShotsDischarged;--RoundsRemaining;ShotTimes.Add(Battle->Elapsed);if(FirstShotAt<0)FirstShotAt=Battle->Elapsed;
        WF_BLOCKED_FIRE_RESULT_V87("actual_round_spent");
        WeaponPhase=EFrontlineWeaponPhase::Recoil;ShotClock=.2f;
        UAnimationAsset* Recoil=FrontlineMachinegunsV30::Action(this,TEXT("Fire"));
        if(!Recoil)Recoil=FrontlineMotionV41::SpecialistAction(this,TEXT("Fire"),false);if(!Recoil)Recoil=Battle->FireAnimation;
        SetMotion(Recoil,false);FirePoseClock=FMath::Max(.12f,ActionLength(Recoil));return;
    }
    if(InfantryTarget)Battle->Fire(this,Target);
    else
    {
        ShowWeaponDischarge();++Battle->ShotsFired;
        if(IsAutomaticWeapon())LastAutomaticFireAt=Battle->Elapsed;
        const float Distance=FVector::Dist2D(Feet(),VehicleTarget->AimPoint());
        const float Spread=35.f+Distance*.03f+Suppression*90.f;
        const FVector Impact=VehicleTarget->AimPoint()+FVector(MotionRandom.FRandRange(-Spread,Spread),MotionRandom.FRandRange(-Spread,Spread),MotionRandom.FRandRange(-65,80));
        if(auto* V=UFrontlineVehicleSubsystem::Get(GetWorld()))V->ApplySmallArmsHit(VehicleTarget,Impact,IsAutomaticWeapon(),Team);
        if(IsAutomaticWeapon() && ShotsDischarged%3==0)Battle->Tracers.Add({GetMuzzleLocation(),Impact,.06f,.06f});
        if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->PlayWeapon(IsAutomaticWeapon(),GetFaction()==EFrontlineFaction::German,GetMuzzleLocation(),SquadId);
    }
    ++ShotsDischarged; --RoundsRemaining;
    WF_BLOCKED_FIRE_RESULT_V87("actual_round_spent");
    ShotTimes.Add(Battle->Elapsed);
    if (FirstShotAt<0) FirstShotAt=Battle->Elapsed;
    WeaponPhase=EFrontlineWeaponPhase::Recoil;
    if (IsCommissionedOfficer())
    {
        ShotClock=Profile.AimedCycleSeconds*CadenceScale+Suppression*.45f+MotionRandom.FRandRange(.04f,.18f);
        SetMotion(OfficerMotions[4],false);FirePoseClock=ActionLength(OfficerMotions[4]);
    }
    else if (IsAutomaticWeapon())
    {
        // Mechanical cadence inside a burst is separate from the pause to observe,
        // correct aim and manage the gun. Longer pauses leave a window to manoeuvre.
        if (BurstRemaining<=0) { BurstRemaining=MotionRandom.RandRange(Profile.BurstMin,Profile.BurstMax); ++BurstsStarted; }
        --BurstRemaining;
        const float RoundInterval=Profile.RoundIntervalSeconds;
        ShotClock=BurstRemaining>0?RoundInterval:MotionRandom.FRandRange(Profile.RecoveryMin,Profile.RecoveryMax)
            *FrontlineBalanceV50::CoveringFireRecoveryScaleV8215(this)+Suppression*.5f;
        FirePoseClock=RoundInterval;
        UAnimationAsset* Recoil=IsStormtrooper()?StormFireMotion.Get():Battle->AutomaticFireAnimation.Get();
        if(auto* Supported=FrontlineMachinegunsV30::Action(this,TEXT("Fire")))Recoil=Supported;
        SetMotion(Recoil,true);
        if (UAnimSingleNodeInstance* Node=Soldier->GetSingleNodeInstance())
            Node->SetPlayRate(ActionLength(Recoil)/RoundInterval);
    }
    else
    {
        // Firing, recoil and bolt manipulation occupy one indivisible weapon cycle.
        // Aim/pressure can lengthen it but never truncate the authored action.
        const float AuthoredCycle=ActionLength(Battle->FireAnimation)+ActionLength(Battle->BoltAnimation.Get())+.18f;
        ShotClock=(WarfareFRRUCampaignV82::Active(Battle)?Profile.AimedCycleSeconds:FMath::Max(Profile.AimedCycleSeconds,AuthoredCycle))*CadenceScale+Suppression*.55f+MotionRandom.FRandRange(.04f,.22f);
        // Deliberate sighting is the marksman's tradeoff: a full bolt/reload action plus
        // a longer observation interval, never a sped-up or truncated rifle animation.
        if (Role==EFrontlineRole::Marksman) ShotClock=FMath::Max(ShotClock,Profile.AimedCycleSeconds*CadenceScale+MotionRandom.FRandRange(.20f,.65f)+Suppression);
        if (IsSectionLeader()) ShotClock+=.45f; // Time spent observing and directing his section.
        UAnimationAsset* Recoil=FrontlineMachinegunsV30::Action(this,TEXT("Fire"));if(!Recoil)Recoil=Battle->FireAnimation;
        SetMotion(Recoil,false);
        FirePoseClock=FMath::Max(.01f,ActionLength(Recoil));
    }
#undef WF_BLOCKED_FIRE_RESULT_V87
}

void AWarfareFrontlineSoldier::ConfigureSpecialistEquipment()
{
    if (!Battle || !Soldier) return;
    const bool Scoped=Role==EFrontlineRole::Marksman && MemberIndex==0 && Battle->HasScopedMarksmen(Team);
    const bool Storm=Role==EFrontlineRole::Mills && Battle->HasStormtroopers(Team);
    if (!Scoped && !Storm) return;
    const TCHAR* Name=Scoped?TEXT("SK_British_ScopedMarksman_V23"):TEXT("SK_German_Stormtrooper_V23");
    auto* Mesh=FrontlineCharacterV29::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V23/Meshes/%s.%s"),Name,Name));
    if (Storm)
    {
        StormFireMotion=FrontlineMotionV34::Load(TEXT("/Game/Warfare/Characters/V23/Animations/A_MP18_Fire_V23.A_MP18_Fire_V23"));
        StormReloadMotion=FrontlineMotionV34::Load(TEXT("/Game/Warfare/Characters/V23/Animations/A_MP18_Reload_V23.A_MP18_Reload_V23"));
    }
    if (!Mesh || !Soldier->GetSkeletalMeshAsset() || Mesh->GetSkeleton()!=Soldier->GetSkeletalMeshAsset()->GetSkeleton() || (Storm && (!StormFireMotion || !StormReloadMotion)))
    { UE_LOG(LogTemp,Error,TEXT("WF_V23_SPECIALIST_ASSET_MISSING %s; baseline equipment retained"),Name); return; }
    bScopedWeapon=Scoped; bStormWeapon=Storm;
    Soldier->EmptyOverrideMaterials(); Soldier->SetSkeletalMeshAsset(Mesh);
    Soldier->SetAnimInstanceClass(UWarfareFrontlineAnimation::StaticClass()); Playing=nullptr;
    RoundsRemaining=GetMagazineCapacity(); SetMotion(GetIdleMotion());
}

bool AWarfareFrontlineSoldier::RecoverMachineGunFrom(AWarfareFrontlineSoldier* FallenGunner)
{
    if (!Battle || !IsAlive() || !FallenGunner || FallenGunner->IsAlive() || !FallenGunner->IsMachineGunner()
        || FallenGunner->Team!=Team || FallenGunner->SquadId!=SquadId || (Role!=EFrontlineRole::Lewis&&Role!=EFrontlineRole::MobileGun) || IsMachineGunner()
        || !Battle->HasFactionTechnology(EFrontlineFaction::British,8,Team)
        || RecoveryGunner.Get()!=FallenGunner || GunRecoveryReadyAt<=0.f || Battle->Elapsed<GunRecoveryReadyAt
        || FVector::Dist2D(Feet(),FallenGunner->Feet())>90.f) return false;
    for (const AWarfareFrontlineSoldier* Other:Battle->Units)
        if (Other!=this && Other->SquadId==SquadId && Other->IsAlive() && Other->IsMachineGunner()) return false;
    USkeletalMesh* Mesh=Battle->V16SoldierMeshes.IsValidIndex(1)?Battle->V16SoldierMeshes[1].Get():nullptr;
    if (!Mesh || !Soldier->GetSkeletalMeshAsset() || Mesh->GetSkeleton()!=Soldier->GetSkeletalMeshAsset()->GetSkeleton()) return false;
    bRecoveredMachineGun=true;
    RoundsRemaining=FMath::Clamp(FallenGunner->RoundsRemaining,0,47); // Take the actual pan, not a free full magazine.
    FallenGunner->Soldier->HideBoneByName(TEXT("weapon_rifle"),EPhysBodyOp::PBO_None);
    FrontlineCharacterV34::ConsumeReleasedWeapon(FallenGunner);
    Soldier->EmptyOverrideMaterials(); Soldier->SetSkeletalMeshAsset(Mesh);
    Soldier->SetAnimInstanceClass(UWarfareFrontlineAnimation::StaticClass()); Playing=nullptr;
    BurstRemaining=0; FocusTarget.Reset(); RecoveryGunner.Reset(); GunRecoveryReadyAt=0;
    MotionSpeed=0; bMoving=false; WeaponPhase=EFrontlineWeaponPhase::Ready;
    AimSettleClock=.8f; ShotClock=FMath::Max(ShotClock,.8f); SetMotion(GetAimMotion());
    FrontlineMaterialV27::Apply(this);
    if (RoundsRemaining<=0) StartReload();
    UE_LOG(LogTemp,Display,TEXT("WF_V23_GUN_TAKEOVER squad=%d assistant=%d donor=%d rounds=%d health=%.3f"),SquadId,MemberIndex,FallenGunner->MemberIndex,RoundsRemaining,Health);
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        Audio->PlayVoice(EFrontlineVoiceEvent::GunTakeover,false,Feet(),SquadId);
    return true;
}

bool AWarfareFrontlineSoldier::UpdateMachineGunRecovery(float DT)
{
    if (!Battle || !IsActiveCombatant() || (Role!=EFrontlineRole::Lewis&&Role!=EFrontlineRole::MobileGun) || IsMachineGunner()
        || !Battle->HasFactionTechnology(EFrontlineFaction::British,8,Team)) return false;
    if (WeaponPhase!=EFrontlineWeaponPhase::Ready) return false;
    AWarfareFrontlineSoldier* Fallen=nullptr;
    for (AWarfareFrontlineSoldier* Other:Battle->Units)
        if (Other->SquadId==SquadId && Other->IsMachineGunner())
        {
            if (Other->IsAlive()) { RecoveryGunner.Reset(); GunRecoveryReadyAt=0; return false; }
            if (!Fallen || Other->bRecoveredMachineGun) Fallen=Other;
        }
    if (!Fallen || FVector::Dist2D(Feet(),Fallen->Feet())>1400.f || Suppression>.75f)
    { RecoveryGunner.Reset(); GunRecoveryReadyAt=0; return false; }
    // One living bearer has the responsibility. Stable member order prevents
    // tick-order races and changes naturally when that bearer is killed.
    for (const AWarfareFrontlineSoldier* Other:Battle->Units)
        if (Other->SquadId==SquadId && Other->IsAlive() && !Other->IsMachineGunner()
            && Other->MemberIndex<MemberIndex && Other->Suppression<=.75f
            && FVector::Dist2D(Other->Feet(),Fallen->Feet())<=1400.f) return false;
    if (RecoveryGunner.Get()!=Fallen) { RecoveryGunner=Fallen; GunRecoveryReadyAt=0; }
    FVector Delta=Fallen->Feet()-Feet(); Delta.Z=0;
    const float Distance=Delta.Size2D();
    bEngaging=false; FocusTarget.Reset();
    bWantedMovement=false;bAtDestinationV43=false;RequestedTravelSpeedV43=0;
    // A prone bearer must lift his own equipment before approaching the fallen
    // gunner. Recovery runs before ordinary locomotion, so it owns this gate too.
    if(auto* Gun=FrontlineMachinegunsV30::Get(this);Gun&&Gun->Update(DT,Distance>75.f))
    {
        bMoving=false;MotionSpeed=0;GunRecoveryReadyAt=0;
        if(auto* Motion=Gun->TransitionMotion())
        {SetMotion(Motion,false);if(auto* Node=Soldier->GetSingleNodeInstance())Node->SetPlayRate(Gun->TransitionRate());}
        return true;
    }
    if (Distance>75.f)
    {
        if (WeaponPhase!=EFrontlineWeaponPhase::Ready) return false;
        const float Speed=145.f*MovementScale*(Battle->CoverFor(this)>0.f?1.f:
            Battle->EncounterMudMoveScale()*FrontlineTerrainTacticsV54::InfantryTravelScale(Feet()));
        const FVector Before=Feet();
        FVector Next=FrontlineMobilityV41::Step(this,Fallen->Feet(),FMath::Min(Distance-65.f,Speed*DT),false,DT);
        Next.Z=FrontlineInfantryV45::SurfaceHeight(Battle,Next)+88.f;
        if(WarfareFRRUCampaignV82::Active(Battle))WarfareFRRUWorldV82::PlaceOnGround(this,Next-FVector(0,0,88));
        else SetActorLocation(Next);
        const float Travel=FVector::Dist2D(Before,Feet());DistanceTravelled+=Travel;
        MotionSpeed=Travel/FMath::Max(DT,SMALL_NUMBER);bMoving=MotionSpeed>8.f;
        if(bMoving)
        {
            FrontlineMotionV41::CancelWaiting(this);
            const FVector Heading=(Feet()-Before).GetSafeNormal2D();
            SetActorRotation(FMath::RInterpTo(GetActorRotation(),Heading.Rotation(),DT,6.f));
            SetMotion(GetWalkMotion());
            if(auto* Node=Soldier->GetSingleNodeInstance())Node->SetPlayRate(FMath::Clamp(MotionSpeed/210.f,.08f,1.45f));
        }
        else SetMotion(GetIdleMotion());
        GunRecoveryReadyAt=0; return true;
    }
    bMoving=false; MotionSpeed=0; SetMotion(GetIdleMotion());
    if (GunRecoveryReadyAt<=0) GunRecoveryReadyAt=Battle->Elapsed+2.4f;
    if (Battle->Elapsed>=GunRecoveryReadyAt) RecoverMachineGunFrom(Fallen);
    return true;
}

