#include "WarfareFrontline.h"
#include "WarfareFrontlineMotionV34.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineCharacterV29.h"
#include "WarfareFrontlineAudio.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

bool AWarfareFrontlineGameMode::HasCommissionedOfficers(int32 Team) const
{
    if (Team==0) return HasTechnology(GetFactionForTeam(Team)==EFrontlineFaction::British?9:6,0);
    const auto* Mission=GetEncounterCampaignMission(); if (!Mission) Mission=GetSelectedCampaignMission();
    return Team==1 && Mission && Mission->bEnemyCommissionedOfficers;
}

void AWarfareFrontlineSoldier::ConfigureOfficerEquipment()
{
    const bool Observer=Role==EFrontlineRole::Signals&&MemberIndex==0;
    if (!Battle || (!Observer&&(!IsSectionLeader() || !Battle->HasCommissionedOfficers(Team))) || !Soldier || !Soldier->GetSkeletalMeshAsset()) return;
    const bool British=GetFaction()==EFrontlineFaction::British;
    const TCHAR* Name=British?TEXT("SK_British_Officer_V24"):TEXT("SK_German_Officer_V24");
    auto* Mesh=FrontlineCharacterV29::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V24/Meshes/%s.%s"),Name,Name));
    const TCHAR* Names[]={TEXT("A_Officer_LowReady_V24"),TEXT("A_Officer_Aim_V24"),TEXT("A_Officer_Walk_V24"),TEXT("A_Officer_Run_V24"),TEXT("A_Officer_Fire_V24"),
        British?TEXT("A_Webley_Reload_V24"):TEXT("A_P08_Reload_V24"),British?TEXT("A_British_Officer_Command_V24"):TEXT("A_German_Officer_Command_V24")};
    TArray<TObjectPtr<UAnimationAsset>> Loaded; bool Complete=Mesh && Mesh->GetSkeleton()==Soldier->GetSkeletalMeshAsset()->GetSkeleton();
    for (const TCHAR* Motion:Names)
    {
        auto* Asset=FrontlineMotionV34::Load(FString::Printf(TEXT("/Game/Warfare/Characters/V24/Animations/%s.%s"),Motion,Motion));
        Complete &= Asset && Asset->GetSkeleton()==Soldier->GetSkeletalMeshAsset()->GetSkeleton(); Loaded.Add(Asset);
    }
    if (!Complete) { UE_LOG(LogTemp,Error,TEXT("WF_V24_OFFICER_ASSETS_MISSING %s; NCO equipment retained"),Name); return; }
    OfficerMotions=MoveTemp(Loaded);bCommissionedOfficer=true;
    Soldier->EmptyOverrideMaterials();Soldier->SetSkeletalMeshAsset(Mesh);Soldier->SetAnimInstanceClass(UWarfareFrontlineAnimation::StaticClass());Playing=nullptr;
    RoundsRemaining=GetMagazineCapacity();SetMotion(GetIdleMotion());
}

bool AWarfareFrontlineSoldier::BeginOfficerCommand()
{
    if (!IsCommissionedOfficer() || !IsActiveCombatant() || !Battle || Battle->Phase!=EFrontlinePhase::Fighting
        || WeaponPhase!=EFrontlineWeaponPhase::Ready || !OfficerMotions.IsValidIndex(6)) return false;
    const auto* Clip=Cast<UAnimSequenceBase>(OfficerMotions[6]); if (!Clip) return false;
    WeaponPhase=EFrontlineWeaponPhase::OfficerCommand;BurstRemaining=0;MotionSpeed=0;bMoving=false;
    SetMotion(OfficerMotions[6],false);FirePoseClock=Clip->GetPlayLength();ShotClock=FMath::Max(ShotClock,FirePoseClock+.2f);
    return true;
}

void AWarfareFrontlineGameMode::ReportOfficerSpotted(const AWarfareFrontlineSoldier* Observer,AWarfareFrontlineSoldier* Officer) const
{
    if (Phase!=EFrontlinePhase::Fighting || !Observer || !Officer || Observer->Team==Officer->Team
        || !Observer->IsActiveCombatant() || !Officer->IsActiveCombatant() || !Officer->IsCommissionedOfficer() || Officer->bOfficerSeenReported
        || DirectFireMultiplier(Observer->Feet(),Officer->Feet())<.95f || !HasSight(Observer,Officer)) return;
    Officer->bOfficerSeenReported=true;
    if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        Audio->PlayVoice(EFrontlineVoiceEvent::EnemyOfficer,Observer->GetFaction()==EFrontlineFaction::German,Observer->Feet(),Observer->SquadId);
}
