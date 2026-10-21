#include "WarfareFrontlineV11Verification.h"
#include "WarfareFrontline.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "Dom/JsonObject.h"

namespace
{
struct FObservedUnit
{
    int32 Squad=INDEX_NONE,Member=0,Team=0,RoleIndex=0,Magazine=0;
    bool Automatic=false,Alive=true,Seen=false,AmmoValid=true,CounterValid=true,ShotTimesValid=true;
    bool ReloadBlockValid=true,ReloadClipObserved=false,BoltClipObserved=false,RecoilClipObserved=false;
    int32 Shots=0,Rounds=0,ReloadsStarted=0,ReloadsCompleted=0,BoltCycles=0,Bursts=0,ShotsDuringReload=0,ShotsWhileMoving=0;
    int32 LastShots=0,LastReloadsCompleted=0,LastPhase=0;
    double LastWorldTime=0;
    float LastClipTime=0,ReloadClipAdvance=0,BoltClipAdvance=0,MinimumShotGap=MAX_flt;
    float ObservedBodySpeed=0,ObservedAngularSpeed=0,PostSettleSeconds=0,PostSettlePositionDrift=0,PostSettleRotationDrift=0;
    float MaximumBodyDimensionCm=0,MaximumBodyInertiaKgCm2=0;
    float MaximumContactShapeDimensionCm=0,ActualTotalMassKg=0;
    float DeathClipAdvance=0,LastDeathClipTime=0;
    bool DeathClipObserved=false;
    int32 ActualValidBodies=0,ActualContactShapes=0;
    bool BodyStateFinite=true,PhysicalInertiaBounded=true;
    FString LastClip,LastExpectedActionClip;
    FString ExpectedMeshName,ActualMeshName;
    int32 VisualRoleIndex=INDEX_NONE,LivingRoleSamples=0,GrenadeStarts=0,GrenadeReleases=0;
    int32 LastGrenadeStarts=0,LastGrenadeReleases=0;
    bool RoleMeshValid=true,MagazineRoleValid=true,MuzzleValid=true,GrenadeCountersValid=true;
    bool GrenadeClipObserved=false,AutomaticRoleClipObserved=false;
    float MinimumMuzzleDistanceCm=MAX_flt,MaximumMuzzleDistanceCm=0,GrenadeClipAdvance=0;
    TArray<float> ShotTimes;
    FFrontlineDeathState Death;
    TMap<FName,FTransform> SettledBodies;
    TArray<TSharedPtr<FJsonValue>> Events;
};

void RecordEvent(FObservedUnit& U,const TCHAR* Name,double Time,const FString& Detail=FString())
{
    auto Event=MakeShared<FJsonObject>();
    Event->SetStringField(TEXT("event"),Name); Event->SetNumberField(TEXT("world_seconds"),Time);
    Event->SetStringField(TEXT("detail"),Detail); U.Events.Add(MakeShared<FJsonValueObject>(Event));
}

struct FCheckWriter
{
    TArray<TSharedPtr<FJsonValue>> Checks;
    int32 Failed=0,Incomplete=0,Passed=0;
    void Add(const TCHAR* Name,bool Covered,bool Good,const FString& Detail=FString())
    {
        auto C=MakeShared<FJsonObject>(); C->SetStringField(TEXT("name"),Name);
        C->SetStringField(TEXT("status"),!Covered?TEXT("incomplete"):Good?TEXT("passed"):TEXT("failed"));
        C->SetBoolField(TEXT("covered"),Covered); C->SetBoolField(TEXT("passed"),Covered && Good);
        C->SetStringField(TEXT("detail"),Detail); Checks.Add(MakeShared<FJsonValueObject>(C));
        if (!Covered) ++Incomplete; else if (Good) ++Passed; else ++Failed;
    }
};
}

struct FFrontlineV11Verification::FImpl
{
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FObservedUnit> Units;
    TMap<int32,TSharedPtr<FJsonObject>> LewisComposition;
    int32 Samples=0;
    bool CompositionValid=true,AssetReferencesValid=true;
};

FFrontlineV11Verification::FFrontlineV11Verification() : Impl(MakeShared<FImpl>()) {}

void FFrontlineV11Verification::Sample(const AWarfareFrontlineGameMode* Battle)
{
    if (!Battle || !Battle->GetWorld()) return;
    ++Impl->Samples;
    Impl->AssetReferencesValid&=Battle->BoltAnimation && Battle->TrenchCycleAnimation && Battle->ReloadAnimation
        && Battle->FireAnimation && Battle->AutomaticFireAnimation && Battle->LewisReloadAnimation && Battle->GermanMGReloadAnimation;
    const double WorldTime=Battle->GetWorld()->GetTimeSeconds();
    for (const FFrontlineSquad& Squad:Battle->Squads) if (Squad.Role==EFrontlineRole::Lewis)
    {
        // Include already dead members. Tactical casualties must not change the original
        // squad-composition check from one gunner/two riflemen into a false failure.
        int32 Automatic=0,Rifle=0,Members=0;
        for (const auto& Member:Squad.Members) if (Member.IsValid())
        {
            ++Members;
            if (Member->IsAutomaticWeapon()) ++Automatic; else ++Rifle;
        }
        if (Members==3)
        {
            auto C=MakeShared<FJsonObject>(); C->SetNumberField(TEXT("squad"),Squad.Id);
            C->SetNumberField(TEXT("team"),Squad.Team); C->SetNumberField(TEXT("automatic_gunners"),Automatic);
            C->SetNumberField(TEXT("riflemen"),Rifle);
            const bool Good=Automatic==1 && Rifle==2;
            C->SetBoolField(TEXT("passed"),Good); Impl->CompositionValid&=Good;
            Impl->LewisComposition.Add(Squad.Id,C);
        }
    }
    for (AWarfareFrontlineSoldier* Actor:Battle->Units) if (IsValid(Actor))
    {
        FObservedUnit& U=Impl->Units.FindOrAdd(Actor);
        U.Squad=Actor->SquadId; U.Member=Actor->MemberIndex; U.Team=Actor->Team; U.RoleIndex=int32(Actor->Role);
        U.Automatic=Actor->IsAutomaticWeapon(); U.Alive=Actor->IsAlive(); U.Magazine=Actor->GetMagazineCapacity();
        U.Shots=Actor->ShotsDischarged; U.Rounds=Actor->RoundsRemaining;
        U.ReloadsStarted=Actor->ReloadsStarted; U.ReloadsCompleted=Actor->ReloadsCompleted;
        U.BoltCycles=Actor->BoltCyclesCompleted; U.Bursts=Actor->BurstsStarted;
        U.ShotsDuringReload=Actor->ShotsDuringReload; U.ShotsWhileMoving=Actor->ShotsWhileMoving;
        U.GrenadeStarts=Actor->GrenadeThrowsStarted; U.GrenadeReleases=Actor->GrenadesReleased;
        U.GrenadeCountersValid&=U.GrenadeStarts>=0 && U.GrenadeReleases>=0 && U.GrenadeReleases<=U.GrenadeStarts
            && (U.RoleIndex==int32(EFrontlineRole::Mills) || (U.GrenadeStarts==0 && U.GrenadeReleases==0));
        U.AmmoValid&=U.Rounds>=0 && U.Rounds<=U.Magazine && U.Magazine>0
            && U.Magazine+U.ReloadsCompleted*U.Magazine-U.Shots==U.Rounds;
        U.CounterValid&=U.ReloadsCompleted<=U.ReloadsStarted && U.ReloadsStarted-U.ReloadsCompleted<=1;
        U.ShotTimes=Actor->ShotTimes;
        U.ShotTimesValid&=U.ShotTimes.Num()==U.Shots;
        for (int32 Index=1;Index<U.ShotTimes.Num();++Index)
        {
            const float Gap=U.ShotTimes[Index]-U.ShotTimes[Index-1];
            U.ShotTimesValid&=FMath::IsFinite(Gap) && Gap>0;
            U.MinimumShotGap=FMath::Min(U.MinimumShotGap,Gap);
        }
        const int32 Phase=int32(Actor->WeaponPhase);
        if (U.Seen)
        {
            U.CounterValid&=U.Shots>=U.LastShots && U.ReloadsCompleted>=U.LastReloadsCompleted;
            U.GrenadeCountersValid&=U.GrenadeStarts>=U.LastGrenadeStarts && U.GrenadeReleases>=U.LastGrenadeReleases;
            if (U.GrenadeStarts>U.LastGrenadeStarts) RecordEvent(U,TEXT("grenade_throw_started"),WorldTime,FString::FromInt(U.GrenadeStarts));
            if (U.GrenadeReleases>U.LastGrenadeReleases) RecordEvent(U,TEXT("grenade_released"),WorldTime,FString::FromInt(U.GrenadeReleases));
            // Only a sampled interval wholly inside the same reload is tested. A tick
            // completing reload can legally fire later; crossing that boundary is excluded.
            if (U.LastPhase==int32(EFrontlineWeaponPhase::Reload) && Phase==U.LastPhase
                && U.ReloadsCompleted==U.LastReloadsCompleted) U.ReloadBlockValid&=U.Shots==U.LastShots;
            if (Phase!=U.LastPhase) RecordEvent(U,TEXT("weapon_phase"),WorldTime,FString::FromInt(Phase));
            if (U.ReloadsCompleted>U.LastReloadsCompleted) RecordEvent(U,TEXT("reload_completed"),WorldTime,FString::FromInt(U.Rounds));
        }
        if (U.Alive && Actor->Soldier)
        {
            // Inspect the mesh and socket actually used by each living actor. The
            // two companions in a machinegun squad carry their faction's rifles.
            static const TCHAR* RoleMeshes[]={TEXT("SK_British_Rifle_V12"),TEXT("SK_British_Lewis_V12"),TEXT("SK_British_Mills_V12"),
                TEXT("SK_German_Gewehr98_V12"),TEXT("SK_German_MG0815_V12"),TEXT("SK_German_Assault_V12")};
            const bool British=Actor->GetFaction()==EFrontlineFaction::British;
            U.VisualRoleIndex=Actor->GetVisualRoleIndex();
            const bool ValidIndex=U.VisualRoleIndex>=0 && U.VisualRoleIndex<int32(UE_ARRAY_COUNT(RoleMeshes));
            USkeletalMesh* ActualMesh=Actor->Soldier->GetSkeletalMeshAsset();
            U.ExpectedMeshName=ValidIndex?RoleMeshes[U.VisualRoleIndex]:TEXT("invalid_role_or_team");
            U.ActualMeshName=GetNameSafe(ActualMesh); ++U.LivingRoleSamples;
            U.RoleMeshValid&=ValidIndex && ActualMesh && U.ActualMeshName==U.ExpectedMeshName
                && Battle->RoleSoldierMeshes.IsValidIndex(U.VisualRoleIndex) && ActualMesh==Battle->RoleSoldierMeshes[U.VisualRoleIndex];
            // U.Magazine is sampled from GetMagazineCapacity above; keep an
            // independent national-capacity expectation rather than comparing
            // that getter with itself after the player switches to Germany.
            U.MagazineRoleValid&=U.Magazine==(U.Automatic?(British?47:100):(British?10:5));
            const bool HasMuzzle=Actor->Soldier->DoesSocketExist(TEXT("WF_Muzzle"))
                && Actor->Soldier->GetBoneIndex(TEXT("weapon_rifle"))!=INDEX_NONE;
            U.MuzzleValid&=HasMuzzle;
            if (HasMuzzle)
            {
                const float Distance=float(FVector::Dist(Actor->GetMuzzleLocation(),Actor->Soldier->GetBoneLocation(TEXT("weapon_rifle"))));
                U.MuzzleValid&=FMath::IsFinite(Distance) && Distance>=70.f && Distance<=115.f;
                U.MinimumMuzzleDistanceCm=FMath::Min(U.MinimumMuzzleDistanceCm,Distance);
                U.MaximumMuzzleDistanceCm=FMath::Max(U.MaximumMuzzleDistanceCm,Distance);
            }
            UAnimSingleNodeInstance* Node=Actor->Soldier->GetSingleNodeInstance();
            UAnimationAsset* Clip=Node?Node->GetAnimationAsset():nullptr;
            const FString ClipName=GetNameSafe(Clip);
            const float ClipTime=Node?Node->GetCurrentTime():0;
            const float Advance=U.Seen && U.LastClip==ClipName && U.LastPhase==Phase
                ?FMath::Max(0.f,ClipTime-U.LastClipTime):0;
            if (Phase==int32(EFrontlineWeaponPhase::Reload) && Clip==Actor->GetReloadMotion())
            { U.ReloadClipObserved=true; U.ReloadClipAdvance+=Advance; }
            if (Phase==int32(EFrontlineWeaponPhase::BoltCycle) && (Clip==Battle->BoltAnimation || Clip==Battle->TrenchCycleAnimation))
            { U.BoltClipObserved=true; U.BoltClipAdvance+=Advance; }
            UAnimationAsset* ExpectedRecoil=U.Automatic?Battle->AutomaticFireAnimation.Get():Battle->FireAnimation.Get();
            if (Phase==int32(EFrontlineWeaponPhase::Recoil) && Clip==ExpectedRecoil) U.RecoilClipObserved=true;
            if (Phase==int32(EFrontlineWeaponPhase::GrenadeThrow) && Clip==Battle->GrenadeAnimation)
            { U.GrenadeClipObserved=true; U.GrenadeClipAdvance+=Advance; }
            if (U.Automatic && Clip && (Clip==Battle->MGIdleAnimation || Clip==Battle->MGAimAnimation
                || Clip==Battle->MGWalkAnimation || Clip==Battle->MGRunAnimation || Clip==Battle->AutomaticFireAnimation
                || Clip==(British?Battle->LewisReloadAnimation.Get():Battle->GermanMGReloadAnimation.Get())))
                U.AutomaticRoleClipObserved=true;
            U.LastClip=ClipName; U.LastClipTime=ClipTime;
        }
        const bool PreviouslySettled=U.Death.bSettled;
        U.Death=Actor->GetDeathState();
        if (U.Death.bStarted && !U.Death.bPhysicsStarted && Actor->Soldier)
        {
            const UAnimSingleNodeInstance* Node=Actor->Soldier->GetSingleNodeInstance();
            if (Node && Node->GetAnimationAsset()==U.Death.AuthoredAnimation.Get())
            {
                if (U.DeathClipObserved) U.DeathClipAdvance+=FMath::Max(0.f,Node->GetCurrentTime()-U.LastDeathClipTime);
                U.LastDeathClipTime=Node->GetCurrentTime(); U.DeathClipObserved=true;
            }
        }
        if (U.Death.bPhysicsStarted && Actor->Soldier)
        {
            U.ActualValidBodies=0; U.ActualContactShapes=0; U.ActualTotalMassKg=0;
            for (const FBodyInstance* Body:Actor->Soldier->Bodies) if (Body && Body->IsValidBodyInstance())
            {
                ++U.ActualValidBodies;
                const FTransform Transform=Body->GetUnrealWorldTransform();
                const FVector Velocity=Body->GetUnrealWorldVelocity(),Angular=Body->GetUnrealWorldAngularVelocityInRadians();
                const FVector BodySize=Body->GetBodyBounds().GetSize(),Inertia=Body->GetBodyInertiaTensor();
                const bool Finite=!Transform.ContainsNaN() && !Velocity.ContainsNaN() && !Angular.ContainsNaN()
                    && !BodySize.ContainsNaN() && !Inertia.ContainsNaN();
                U.BodyStateFinite&=Finite;
                if (!Finite) continue;
                U.ObservedBodySpeed=FMath::Max(U.ObservedBodySpeed,float(Velocity.Size()));
                U.ObservedAngularSpeed=FMath::Max(U.ObservedAngularSpeed,float(Angular.Size()));
                U.MaximumBodyDimensionCm=FMath::Max(U.MaximumBodyDimensionCm,float(BodySize.GetAbsMax()));
                U.MaximumBodyInertiaKgCm2=FMath::Max(U.MaximumBodyInertiaKgCm2,float(Inertia.GetAbsMax()));
                const float Mass=Body->GetBodyMass(); U.ActualTotalMassKg+=Mass;
                // The single whole-corpse compound legitimately has much larger
                // inertia than a limb. Bound inertia by measured mass and spatial
                // extent; keep primitive dimensions independently human-sized.
                const float InertiaBound=Mass*float(BodySize.SizeSquared())*.25f;
                U.PhysicalInertiaBounded&=Inertia.GetMin()>0 && Inertia.GetAbsMax()<=InertiaBound*1.02f+1.f;
                if (const UBodySetup* Setup=Body->GetBodySetup())
                {
                    U.ActualContactShapes+=Setup->AggGeom.GetElementCount();
                    const float Scale=float(Body->Scale3D.GetAbsMax());
                    for (const FKSphylElem& Shape:Setup->AggGeom.SphylElems)
                        U.MaximumContactShapeDimensionCm=FMath::Max(U.MaximumContactShapeDimensionCm,(Shape.Length+2.f*Shape.Radius)*Scale);
                    for (const FKSphereElem& Shape:Setup->AggGeom.SphereElems)
                        U.MaximumContactShapeDimensionCm=FMath::Max(U.MaximumContactShapeDimensionCm,2.f*Shape.Radius*Scale);
                }
                if (U.Death.bSettled)
                {
                    const FName BoneName=Actor->Soldier->GetBoneName(Body->InstanceBoneIndex);
                    if (const FTransform* Base=U.SettledBodies.Find(BoneName))
                    {
                        U.PostSettlePositionDrift=FMath::Max(U.PostSettlePositionDrift,float(FVector::Dist(Base->GetLocation(),Transform.GetLocation())));
                        U.PostSettleRotationDrift=FMath::Max(U.PostSettleRotationDrift,float(Base->GetRotation().AngularDistance(Transform.GetRotation())));
                    }
                    else U.SettledBodies.Add(BoneName,Transform);
                }
            }
            if (U.Death.bSettled && PreviouslySettled) U.PostSettleSeconds+=FMath::Max(0.,WorldTime-U.LastWorldTime);
        }
        U.LastShots=U.Shots; U.LastReloadsCompleted=U.ReloadsCompleted; U.LastPhase=Phase;
        U.LastGrenadeStarts=U.GrenadeStarts; U.LastGrenadeReleases=U.GrenadeReleases;
        U.LastWorldTime=WorldTime; U.Seen=true;
    }
}

bool FFrontlineV11Verification::AreObservedDeathsSettled() const
{
    int32 Deaths=0;
    for (const auto& Entry:Impl->Units) if (Entry.Value.Death.bStarted)
    {
        ++Deaths;
        if (!Entry.Value.Death.bSettled || Entry.Value.PostSettleSeconds<2.f) return false;
    }
    return Deaths>0;
}

TSharedRef<FJsonObject> FFrontlineV11Verification::BuildReport() const
{
    auto Report=MakeShared<FJsonObject>(); FCheckWriter Checks;
    bool Ammo=true,Counters=true,ShotTimes=true,ReloadBlock=true,RifleCadence=true,Physics=true,Energy=true,Stable=true;
    bool RifleReloadVisual=true,AutomaticReloadVisual=true;
    bool HumanColliderScale=true;
    int32 RiflesFired=0,AutomaticFired=0,CompanionsFired=0,AutomaticFastGaps=0,AutomaticRecoveryGaps=0;
    int32 AutomaticRecoilClips=0;
    int32 RifleReloads=0,AutomaticReloads=0,Deaths=0,BulletDeaths=0,ExplosionDeaths=0,Settled=0,CompleteDeaths=0;
    int32 ReloadClips=0,BoltClips=0,TotalShots=0,TotalBoltCycles=0,TotalReloads=0;
    float MaxJoint=0,MaxRise=0,MaxTravel=0,MaxBody=0,MaxAngular=0,MaxSettle=0,MaxDrift=0;
    int32 LimitEvents=0; bool AuthoredGunDeaths=true,AuthoredTerrainHandoffs=true,VisualPhysicsCoherent=true,PublishedPhysicsCoherent=true;
    TSet<FString> ObservedDeathClips;
    TSet<int32> LivingVisualRoles,AutomaticTeams;
    int32 LivingRoleSamples=0,GrenadeStarts=0,GrenadeReleases=0,GrenadeClips=0;
    bool ActualRolesValid=true,GrenadeCountersValid=true,GrenadeClipsValid=true,AutomaticRoleClipsValid=true;
    TArray<TSharedPtr<FJsonValue>> Units,Compositions;
    for (const auto& Entry:Impl->LewisComposition) Compositions.Add(MakeShared<FJsonValueObject>(Entry.Value));
    for (const auto& Entry:Impl->Units)
    {
        const FObservedUnit& U=Entry.Value;
        auto Row=MakeShared<FJsonObject>();
        Row->SetNumberField(TEXT("squad"),U.Squad); Row->SetNumberField(TEXT("member"),U.Member); Row->SetNumberField(TEXT("team"),U.Team);
        Row->SetNumberField(TEXT("role_index"),U.RoleIndex); Row->SetBoolField(TEXT("automatic"),U.Automatic); Row->SetBoolField(TEXT("alive"),U.Alive);
        Row->SetNumberField(TEXT("visual_role_index"),U.VisualRoleIndex);
        Row->SetStringField(TEXT("expected_role_mesh"),U.ExpectedMeshName); Row->SetStringField(TEXT("actual_living_role_mesh"),U.ActualMeshName);
        Row->SetNumberField(TEXT("living_role_samples"),U.LivingRoleSamples);
        Row->SetBoolField(TEXT("actual_role_mesh_matches"),U.RoleMeshValid);
        Row->SetBoolField(TEXT("faction_weapon_magazine_matches"),U.MagazineRoleValid);
        Row->SetBoolField(TEXT("actual_muzzle_at_weapon_front"),U.MuzzleValid);
        Row->SetNumberField(TEXT("minimum_muzzle_from_weapon_bone_cm"),U.MinimumMuzzleDistanceCm==MAX_flt?-1:U.MinimumMuzzleDistanceCm);
        Row->SetNumberField(TEXT("maximum_muzzle_from_weapon_bone_cm"),U.MaximumMuzzleDistanceCm);
        Row->SetBoolField(TEXT("dedicated_automatic_role_clip_observed"),U.AutomaticRoleClipObserved);
        Row->SetNumberField(TEXT("grenade_throws_started"),U.GrenadeStarts); Row->SetNumberField(TEXT("grenades_released"),U.GrenadeReleases);
        Row->SetBoolField(TEXT("grenade_counters_valid"),U.GrenadeCountersValid);
        Row->SetBoolField(TEXT("actual_throw_clip_observed"),U.GrenadeClipObserved);
        Row->SetNumberField(TEXT("throw_clip_advanced_seconds"),U.GrenadeClipAdvance);
        LivingRoleSamples+=U.LivingRoleSamples;
        if (U.LivingRoleSamples>0) LivingVisualRoles.Add(U.VisualRoleIndex);
        ActualRolesValid&=U.RoleMeshValid && U.MagazineRoleValid && U.MuzzleValid;
        GrenadeStarts+=U.GrenadeStarts; GrenadeReleases+=U.GrenadeReleases;
        GrenadeCountersValid&=U.GrenadeCountersValid;
        GrenadeClips+=U.GrenadeClipObserved && U.GrenadeClipAdvance>.05f?1:0;
        if (U.GrenadeReleases>0) GrenadeClipsValid&=U.GrenadeClipObserved && U.GrenadeClipAdvance>.05f;
        if (U.Automatic && U.Shots>0) { AutomaticTeams.Add(U.Team); AutomaticRoleClipsValid&=U.AutomaticRoleClipObserved; }
        Row->SetNumberField(TEXT("magazine_capacity"),U.Magazine); Row->SetNumberField(TEXT("rounds_remaining"),U.Rounds);
        Row->SetNumberField(TEXT("shots"),U.Shots); Row->SetNumberField(TEXT("bolt_cycles_completed"),U.BoltCycles);
        Row->SetNumberField(TEXT("reloads_started"),U.ReloadsStarted); Row->SetNumberField(TEXT("reloads_completed"),U.ReloadsCompleted);
        Row->SetNumberField(TEXT("bursts_started"),U.Bursts); Row->SetBoolField(TEXT("ammunition_conserved"),U.AmmoValid);
        Row->SetBoolField(TEXT("sampled_reload_blocks_shots"),U.ReloadBlockValid);
        Row->SetNumberField(TEXT("reload_clip_advanced_seconds"),U.ReloadClipAdvance);
        Row->SetNumberField(TEXT("bolt_clip_advanced_seconds"),U.BoltClipAdvance);
        Row->SetBoolField(TEXT("expected_recoil_clip_observed"),U.RecoilClipObserved);
        Row->SetNumberField(TEXT("minimum_shot_interval_seconds"),U.MinimumShotGap==MAX_flt?-1:U.MinimumShotGap);
        TArray<TSharedPtr<FJsonValue>> Times;
        for (float Time:U.ShotTimes) Times.Add(MakeShared<FJsonValueNumber>(Time));
        Row->SetArrayField(TEXT("shot_times"),Times); Row->SetArrayField(TEXT("sampled_events"),U.Events);
        Ammo&=U.AmmoValid; Counters&=U.CounterValid; ShotTimes&=U.ShotTimesValid;
        ReloadBlock&=U.ReloadBlockValid && U.ShotsDuringReload==0 && U.ShotsWhileMoving==0;
        TotalShots+=U.Shots; TotalBoltCycles+=U.BoltCycles; TotalReloads+=U.ReloadsCompleted;
        ReloadClips+=U.ReloadClipObserved && U.ReloadClipAdvance>.05f?1:0;
        BoltClips+=U.BoltClipObserved && U.BoltClipAdvance>.05f?1:0;
        if (U.Automatic)
        {
            AutomaticFired+=U.Shots>0?1:0; AutomaticReloads+=U.ReloadsCompleted;
            AutomaticRecoilClips+=U.RecoilClipObserved?1:0;
            if (U.ReloadsCompleted>0) AutomaticReloadVisual&=U.ReloadClipObserved && U.ReloadClipAdvance>.05f;
            for (int32 I=1;I<U.ShotTimes.Num();++I)
            {
                const float Gap=U.ShotTimes[I]-U.ShotTimes[I-1];
                AutomaticFastGaps+=Gap>.025f && Gap<.25f?1:0;
                AutomaticRecoveryGaps+=Gap>.65f?1:0;
            }
        }
        else
        {
            RiflesFired+=U.Shots>0?1:0; RifleReloads+=U.ReloadsCompleted;
            if (U.ReloadsCompleted>0) RifleReloadVisual&=U.ReloadClipObserved && U.ReloadClipAdvance>.05f;
            CompanionsFired+=U.RoleIndex==int32(EFrontlineRole::Lewis) && U.Shots>1?1:0;
            if (U.Shots>1) RifleCadence&=U.MinimumShotGap>.8f;
        }
        if (U.Death.bStarted)
        {
            const FFrontlineDeathState& D=U.Death;
            ++Deaths; const bool Bullet=D.Kind==EFrontlineDamageKind::Bullet;
            BulletDeaths+=Bullet?1:0; ExplosionDeaths+=Bullet?0:1;
            if (Bullet) AuthoredGunDeaths&=D.bAuthoredFall && U.DeathClipObserved && U.DeathClipAdvance>.1f
                && D.bPhysicsStarted && D.PhysicsStartedAt>=D.AuthoredClipSeconds-.02f;
            if (Bullet) AuthoredTerrainHandoffs&=D.bPhysicsStarted && D.HandoffTerrainClearanceCm>=-2.f
                && D.MaximumHandoffPosePredictionErrorCm<1.f;
            VisualPhysicsCoherent&=D.PhysicalPoseSamples>0 && D.VisualBindings.Num()==4
                && D.FirstPhysicalPoseErrorCm>=0 && D.MaximumPhysicalPoseErrorCm<1.f;
            PublishedPhysicsCoherent&=D.PublishedPoseSamples>0 && D.VisualBindings.Num()==4
                && D.FirstPublishedPoseErrorCm>=0 && D.MaximumPublishedPoseErrorCm<1.f;
            if (Bullet && U.DeathClipObserved) ObservedDeathClips.Add(GetNameSafe(D.AuthoredAnimation.Get()));
            Settled+=D.bSettled?1:0;
            const bool Complete=D.bSettled && U.PostSettleSeconds>=2.f;
            CompleteDeaths+=Complete?1:0;
            Physics&=!D.bMissingPhysics && !D.bNonFinite && U.BodyStateFinite
                && U.ActualValidBodies==(Bullet?1:12) && D.ConstraintCount==(Bullet?0:11) && U.ActualContactShapes==16;
            HumanColliderScale&=U.MaximumContactShapeDimensionCm>0 && U.MaximumContactShapeDimensionCm<80.f
                && U.MaximumBodyDimensionCm>0 && U.MaximumBodyDimensionCm<(Bullet?240.f:80.f)
                && U.ActualTotalMassKg>75.f && U.ActualTotalMassKg<90.f && U.PhysicalInertiaBounded
                && (Bullet || U.MaximumBodyInertiaKgCm2<20000.f);
            Energy&=D.MaximumJointSeparationCm<5.f && D.MaximumPelvisTravelCm<(Bullet?350.f:600.f)
                && (!Bullet || D.MaximumPelvisRiseCm<25.f) && D.MaximumBodySpeedCmS<1200.f && D.MaximumAngularSpeedRadS<14.f;
            if (Complete) Stable&=D.SettledAt<=8.f && U.PostSettlePositionDrift<2.f && U.PostSettleRotationDrift<.12f;
            else if (D.Age>=10.f) Stable=false;
            MaxJoint=FMath::Max(MaxJoint,D.MaximumJointSeparationCm); MaxRise=FMath::Max(MaxRise,D.MaximumPelvisRiseCm);
            MaxTravel=FMath::Max(MaxTravel,D.MaximumPelvisTravelCm); MaxBody=FMath::Max(MaxBody,D.MaximumBodySpeedCmS);
            MaxAngular=FMath::Max(MaxAngular,D.MaximumAngularSpeedRadS); MaxSettle=FMath::Max(MaxSettle,D.SettledAt);
            MaxDrift=FMath::Max(MaxDrift,U.PostSettlePositionDrift); LimitEvents+=D.VelocityLimitEvents;
            auto Death=MakeShared<FJsonObject>();
            Death->SetStringField(TEXT("kind"),Bullet?TEXT("bullet"):D.Kind==EFrontlineDamageKind::Grenade?TEXT("grenade"):TEXT("artillery"));
            Death->SetNumberField(TEXT("age_seconds"),D.Age); Death->SetBoolField(TEXT("settled"),D.bSettled);
            Death->SetBoolField(TEXT("authored_fall"),D.bAuthoredFall);
            Death->SetBoolField(TEXT("physics_started"),D.bPhysicsStarted);
            Death->SetNumberField(TEXT("physics_started_at_seconds"),D.PhysicsStartedAt);
            Death->SetNumberField(TEXT("authored_clip_length_seconds"),D.AuthoredClipSeconds);
            Death->SetStringField(TEXT("authored_clip"),GetNameSafe(D.AuthoredAnimation.Get()));
            Death->SetNumberField(TEXT("authored_variant_index"),D.AuthoredVariantIndex);
            Death->SetNumberField(TEXT("independently_observed_death_clip_advance_seconds"),U.DeathClipAdvance);
            Death->SetNumberField(TEXT("predicted_unfitted_terrain_clearance_cm"),D.PredictedUnfittedClearanceCm);
            Death->SetNumberField(TEXT("actual_handoff_terrain_clearance_cm"),D.HandoffTerrainClearanceCm);
            Death->SetStringField(TEXT("lowest_handoff_body"),D.HandoffLowestBody.ToString());
            Death->SetNumberField(TEXT("terrain_fit_angle_degrees"),D.TerrainFitAngleDegrees);
            Death->SetNumberField(TEXT("terrain_fit_lift_cm"),D.TerrainFitLiftCm);
            Death->SetNumberField(TEXT("handoff_pose_prediction_error_cm"),D.MaximumHandoffPosePredictionErrorCm);
            Death->SetNumberField(TEXT("initialization_bone_recalculation_shift_cm"),D.InitializationBoneShiftCm);
            Death->SetNumberField(TEXT("initialized_body_to_bone_position_error_cm"),D.InitializedBodyToBoneErrorCm);
            Death->SetNumberField(TEXT("maximum_sampled_right_elbow_axis_limit_excess_degrees"),D.MaximumRightElbowLimitExcessDegrees);
            Death->SetNumberField(TEXT("first_actual_physics_visual_pose_error_cm"),D.FirstPhysicalPoseErrorCm);
            Death->SetNumberField(TEXT("maximum_actual_physics_visual_pose_error_cm"),D.MaximumPhysicalPoseErrorCm);
            Death->SetNumberField(TEXT("actual_physics_visual_pose_samples"),D.PhysicalPoseSamples);
            Death->SetStringField(TEXT("original_visual_probe_phase"),TEXT("Actor TickDeath; retained raw observation, may precede mesh physics-blend publication"));
            Death->SetNumberField(TEXT("first_published_physics_pose_error_cm"),D.FirstPublishedPoseErrorCm);
            Death->SetNumberField(TEXT("maximum_published_physics_pose_error_cm"),D.MaximumPublishedPoseErrorCm);
            Death->SetNumberField(TEXT("last_published_physics_pose_error_cm"),D.LastPublishedPoseErrorCm);
            Death->SetNumberField(TEXT("published_physics_pose_samples"),D.PublishedPoseSamples);
            Death->SetNumberField(TEXT("first_published_tick_group"),D.FirstPublishedTickGroup);
            Death->SetNumberField(TEXT("finalizations_before_end_physics"),D.NonPhysicsFinalizations);
            Death->SetNumberField(TEXT("settled_at_seconds"),D.SettledAt); Death->SetNumberField(TEXT("stable_observation_seconds"),U.PostSettleSeconds);
            Death->SetNumberField(TEXT("actual_valid_bodies"),U.ActualValidBodies);
            Death->SetNumberField(TEXT("actual_contact_shapes"),U.ActualContactShapes);
            Death->SetNumberField(TEXT("actual_total_mass_kg"),U.ActualTotalMassKg);
            Death->SetStringField(TEXT("physical_response"),Bullet?TEXT("authored_fall_then_single_compound_rigid_body"):TEXT("twelve_body_articulated_explosion_response"));
            Death->SetNumberField(TEXT("maximum_contact_shape_dimension_cm"),U.MaximumContactShapeDimensionCm);
            Death->SetBoolField(TEXT("inertia_bounded_by_actual_mass_and_spatial_extent"),U.PhysicalInertiaBounded);
            Death->SetNumberField(TEXT("maximum_actual_body_dimension_cm"),U.MaximumBodyDimensionCm);
            Death->SetNumberField(TEXT("maximum_actual_body_inertia_kg_cm2"),U.MaximumBodyInertiaKgCm2);
            Death->SetNumberField(TEXT("peak_body_speed_before_cap_cm_s"),D.MaximumBodySpeedCmS);
            Death->SetNumberField(TEXT("peak_angular_speed_before_cap_rad_s"),D.MaximumAngularSpeedRadS);
            Death->SetStringField(TEXT("peak_angular_body"),D.PeakAngularBody.ToString());
            Death->SetNumberField(TEXT("independently_sampled_peak_body_speed_cm_s"),U.ObservedBodySpeed);
            Death->SetNumberField(TEXT("independently_sampled_peak_angular_speed_rad_s"),U.ObservedAngularSpeed);
            Death->SetNumberField(TEXT("maximum_joint_gap_cm"),D.MaximumJointSeparationCm);
            Death->SetNumberField(TEXT("maximum_pelvis_travel_cm"),D.MaximumPelvisTravelCm);
            Death->SetNumberField(TEXT("maximum_pelvis_rise_cm"),D.MaximumPelvisRiseCm);
            Death->SetNumberField(TEXT("maximum_all_body_post_settle_drift_cm"),U.PostSettlePositionDrift);
            Death->SetNumberField(TEXT("maximum_all_body_post_settle_rotation_rad"),U.PostSettleRotationDrift);
            Death->SetNumberField(TEXT("velocity_cap_events"),D.VelocityLimitEvents);
            Death->SetBoolField(TEXT("stability_window_complete"),Complete);
            Row->SetObjectField(TEXT("death"),Death);
        }
        Units.Add(MakeShared<FJsonValueObject>(Row));
    }
    Checks.Add(TEXT("weapon_animation_assets_present"),Impl->Samples>0,Impl->AssetReferencesValid);
    Checks.Add(TEXT("living_actors_use_correct_role_mesh_magazine_and_distal_muzzle"),LivingRoleSamples>0,ActualRolesValid,
        TEXT("Samples actual living SkeletalMeshComponent assets, including rifle companions; magazines 10/5/47/100; WF_Muzzle must be 70–115 cm from the animated weapon_rifle bone. This distance check cannot prove the muzzle lies on visible barrel geometry; screenshots remain necessary."));
    Checks.Add(TEXT("all_six_role_meshes_observed_on_living_actors"),LivingVisualRoles.Num()==6,ActualRolesValid);
    Checks.Add(TEXT("both_factions_automatic_gunners_play_dedicated_role_clips"),AutomaticTeams.Num()==2,AutomaticRoleClipsValid);
    Checks.Add(TEXT("grenade_release_counts_do_not_exceed_started_assault_throws"),GrenadeStarts>0,GrenadeCountersValid);
    Checks.Add(TEXT("released_grenades_have_an_actual_advancing_throw_animation"),GrenadeReleases>0,GrenadeClipsValid,
        TEXT("Passive actor sampling observes counters and the playing throw clip. Serialized 0.8-second Notify inspection is a separate import report; this check does not claim exact sub-frame release timing. Interrupted throws may legitimately produce no grenade."));
    Checks.Add(TEXT("ammunition_conserved_and_counters_monotonic"),TotalShots>0,Ammo && Counters && ShotTimes);
    Checks.Add(TEXT("lewis_squads_have_one_automatic_and_two_riflemen"),!Compositions.IsEmpty(),Impl->CompositionValid);
    Checks.Add(TEXT("lewis_companions_actually_use_rifle_cadence"),CompanionsFired>0,RifleCadence);
    Checks.Add(TEXT("actual_rifle_bolt_animation_advances"),RiflesFired>0,BoltClips>0 && TotalBoltCycles>0 && RifleCadence);
    Checks.Add(TEXT("automatic_fire_has_fast_rounds_and_recovery"),AutomaticFired>0,AutomaticFastGaps>=3 && AutomaticRecoveryGaps>0 && AutomaticRecoilClips>0,
        TEXT("Requires the dedicated AutomaticFireAnimation on an actual automatic gunner, in addition to its shot timestamps."));
    Checks.Add(TEXT("rifle_reload_completes_with_visible_clip"),RifleReloads>0,RifleReloadVisual && ReloadBlock);
    Checks.Add(TEXT("automatic_magazine_reload_completes"),AutomaticReloads>0,AutomaticReloadVisual && ReloadBlock);
    Checks.Add(TEXT("no_shots_during_sampled_reload_or_movement"),TotalShots>0,ReloadBlock);
    Checks.Add(TEXT("fatal_gun_hit_observed"),BulletDeaths>0,true);
    Checks.Add(TEXT("gun_deaths_play_complete_authored_fall_before_real_physics"),BulletDeaths>0,AuthoredGunDeaths,
        TEXT("Observed advancing death clip on actual casualties, followed by initialized Chaos after the full clip duration. Settlement time is measured from the lethal hit, including animation."));
    Checks.Add(TEXT("gun_death_handoff_avoids_deep_terrain_penetration"),BulletDeaths>0,AuthoredTerrainHandoffs,
        TEXT("Actual initialized primitive lower-support samples must not begin over 2 cm inside the shared terrain; predicted final head/pelvis must match the played pose within 1 cm. Prop contacts still need visual review."));
    Checks.Add(TEXT("actual_physics_preserves_visible_bone_to_body_relationships"),Deaths>0,VisualPhysicsCoherent,
        TEXT("Original actor-tick comparison is retained with the same 1 cm threshold. It may compare a newly advanced body with the prior published skeleton; use the additional finalized-blend probe to distinguish timing from actual visual displacement."));
    Checks.Add(TEXT("published_physics_pose_preserves_body_relationships"),Deaths>0,PublishedPhysicsCoherent,
        TEXT("OnBoneTransformsFinalized callback after EndPhysics begins, after mesh pose-buffer publication: pelvis/head/both feet must remain within 1 cm of captured physical-body relationships. PrePhysics refresh callbacks are counted separately."));
    Checks.Add(TEXT("fatal_explosion_observed"),ExplosionDeaths>0,true);
    Checks.Add(TEXT("actual_corpse_bodies_are_finite_and_connected"),Deaths>0,Physics && MaxJoint<5.f,
        TEXT("Gun deaths preserve the authored silhouette with one compound rigid body / zero joints; explosions use twelve bodies / eleven joints. Both retain sixteen contact shapes and 83.2 kg total mass."));
    Checks.Add(TEXT("death_collider_scale_matches_human_body"),Deaths>0,HumanColliderScale,
        TEXT("Every contact shape below 80 cm; whole gun-death compound below 240 cm, explosion body below 80 cm; actual total mass 75–90 kg. Inertia must be positive and bounded by actual mass and spatial extent; explosion bodies additionally stay below 20,000 kg cm^2."));
    Checks.Add(TEXT("death_motion_has_bounded_energy_and_travel"),Deaths>0,Energy);
    const bool StabilityDecidable=Deaths>0 && (CompleteDeaths==Deaths || !Stable);
    Checks.Add(TEXT("corpses_settle_and_remain_stable_for_two_seconds"),StabilityDecidable,Stable && CompleteDeaths==Deaths,
        FString::Printf(TEXT("%d of %d casualties have a completed post-settle window; do not reset the match before collecting this evidence."),CompleteDeaths,Deaths));
    Report->SetStringField(TEXT("scope"),TEXT("Passive sampling of actual game actors, animation instances, shot timestamps and Chaos bodies. Never changes health, targets, ammunition, clocks, poses or the world. Use a fresh observer for each natural match or labelled fixture."));
    Report->SetStringField(TEXT("limitations"),TEXT("Sampling cannot prove hand-to-weapon contact, collision-free silhouettes or visual quality. Missing reload/explosion/settling coverage is incomplete, not passed. Geometry and bone-scale audit is a separate first-death JSON."));
    Report->SetNumberField(TEXT("samples"),Impl->Samples); Report->SetNumberField(TEXT("actors_observed"),Units.Num());
    Report->SetNumberField(TEXT("living_visual_role_count"),LivingVisualRoles.Num());
    Report->SetNumberField(TEXT("automatic_factions_observed_firing"),AutomaticTeams.Num());
    Report->SetNumberField(TEXT("grenade_throws_started"),GrenadeStarts); Report->SetNumberField(TEXT("grenades_released"),GrenadeReleases);
    Report->SetNumberField(TEXT("actors_with_advancing_throw_clip"),GrenadeClips);
    Report->SetNumberField(TEXT("total_shots"),TotalShots); Report->SetNumberField(TEXT("total_bolt_cycles"),TotalBoltCycles);
    Report->SetNumberField(TEXT("total_reloads"),TotalReloads); Report->SetNumberField(TEXT("automatic_fast_intervals"),AutomaticFastGaps);
    Report->SetNumberField(TEXT("actors_with_advancing_reload_clip"),ReloadClips);
    Report->SetNumberField(TEXT("automatic_gunners_with_expected_recoil_clip"),AutomaticRecoilClips);
    Report->SetNumberField(TEXT("automatic_recovery_intervals"),AutomaticRecoveryGaps);
    Report->SetNumberField(TEXT("deaths"),Deaths); Report->SetNumberField(TEXT("settled_deaths"),Settled);
    TArray<TSharedPtr<FJsonValue>> DeathClipNames;
    for (const FString& Name:ObservedDeathClips) DeathClipNames.Add(MakeShared<FJsonValueString>(Name));
    Report->SetArrayField(TEXT("observed_authored_death_clips"),DeathClipNames);
    Report->SetNumberField(TEXT("complete_stability_windows"),CompleteDeaths);
    Report->SetNumberField(TEXT("maximum_joint_separation_cm"),MaxJoint); Report->SetNumberField(TEXT("maximum_pelvis_rise_cm"),MaxRise);
    Report->SetNumberField(TEXT("maximum_pelvis_travel_cm"),MaxTravel); Report->SetNumberField(TEXT("maximum_raw_body_speed_cm_s"),MaxBody);
    Report->SetNumberField(TEXT("maximum_raw_angular_speed_rad_s"),MaxAngular); Report->SetNumberField(TEXT("maximum_settle_seconds"),MaxSettle);
    Report->SetNumberField(TEXT("maximum_all_body_settled_drift_cm"),MaxDrift); Report->SetNumberField(TEXT("velocity_cap_events"),LimitEvents);
    Report->SetArrayField(TEXT("lewis_composition"),Compositions); Report->SetArrayField(TEXT("actors"),Units); Report->SetArrayField(TEXT("checks"),Checks.Checks);
    Report->SetNumberField(TEXT("passed_checks"),Checks.Passed); Report->SetNumberField(TEXT("failed_checks"),Checks.Failed);
    Report->SetNumberField(TEXT("incomplete_checks"),Checks.Incomplete);
    Report->SetBoolField(TEXT("observed_invariants_passed"),Checks.Failed==0 && Impl->Samples>0);
    Report->SetBoolField(TEXT("full_coverage_complete"),Checks.Incomplete==0);
    Report->SetBoolField(TEXT("passed"),Checks.Failed==0 && Checks.Incomplete==0);
    return Report;
}
