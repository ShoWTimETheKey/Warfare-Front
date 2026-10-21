#include "WarfareFrontlineEquipmentV41.h"
#include "WarfareFrontlineSoldierSurfaceV67.h"
#include "WarfareFrontlineArtV68.h"
#include "WarfareFrontline.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
namespace { const FName RoundTag(TEXT("WF_V41_MOUNTED_NO23")); }
void FrontlineEquipmentV41::Initialize(AWarfareFrontlineSoldier* U)
{
 if(!U||!U->Soldier||U->MemberIndex!=0)return;
 const bool Grenadier=U->Role==EFrontlineRole::RifleGrenadier,AT=U->Role==EFrontlineRole::AntiTankRifle;
 if(!Grenadier&&!AT)return;
 const FString Name=Grenadier?TEXT("SK_British_RifleGrenadier_V41"):TEXT("SK_German_AntiTankRifle_V41");
 auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Warfare/Characters/V41/Meshes/")+Name+TEXT(".")+Name));
 Mesh=FrontlineArtV68::ResolveMesh(FrontlineSoldierSurfaceV67::ResolveMesh(Mesh));
 auto* Base=U->Soldier->GetSkeletalMeshAsset();auto* Physics=U->Soldier->GetPhysicsAsset();
 if(!Mesh||!Base||Mesh->GetSkeleton()!=Base->GetSkeleton()||!Physics){UE_LOG(LogTemp,Error,TEXT("WF_V41_SPECIALIST_ASSET_CONTRACT %s"),*Name);return;}
 U->Soldier->EmptyOverrideMaterials();U->Soldier->SetSkeletalMeshAsset(Mesh);U->Soldier->SetPhysicsAsset(Physics,true);
 if(Grenadier)
 {
  auto* Round=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Characters/V41/Weapons/SM_No23_RifleGrenade_V41.SM_No23_RifleGrenade_V41"));
  if(Round){auto* C=NewObject<UStaticMeshComponent>(U);U->AddInstanceComponent(C);C->ComponentTags.Add(RoundTag);C->SetupAttachment(U->Soldier,TEXT("weapon_rifle"));C->SetStaticMesh(Round);C->SetRelativeLocation(FVector(0,90,4.4));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);C->RegisterComponent();}
 }
 UE_LOG(LogTemp,Display,TEXT("WF_V41_SPECIALIST_EQUIPMENT role=%d mesh=%s"),int32(U->Role),*Name);
}
void FrontlineEquipmentV41::Tick(AWarfareFrontlineSoldier* U)
{
 if(!U||U->Role!=EFrontlineRole::RifleGrenadier||U->MemberIndex!=0)return;
 TInlineComponentArray<UStaticMeshComponent*> Parts(U);
 for(auto* C:Parts)if(C->ComponentHasTag(RoundTag))C->SetVisibility(U->IsActiveCombatant()&&U->RoundsRemaining>0&&U->Battle&&U->Battle->BattleOutroWinner<0);
}
void FrontlineEquipmentV41::RunChecks(AWarfareFrontlineSoldier* U,TFunctionRef<void(const FString&,bool)> Check)
{
 if(!U||U->MemberIndex!=0)return;auto* Mesh=U->Soldier->GetSkeletalMeshAsset();
 const bool AT=U->Role==EFrontlineRole::AntiTankRifle,RG=U->Role==EFrontlineRole::RifleGrenadier;if(!AT&&!RG)return;
 Check(TEXT("specialist_actual_equipment_variant"),Mesh&&FrontlineSoldierSurfaceV67::SourceMeshName(Mesh->GetName()).Contains(AT?TEXT("AntiTankRifle_V41"):TEXT("RifleGrenadier_V41")));
 Check(TEXT("specialist_actual_mesh_muzzle_socket"),U->Soldier->DoesSocketExist(TEXT("WF_Muzzle"))&&U->Soldier->GetSocketBoneName(TEXT("WF_Muzzle"))==TEXT("weapon_rifle"));
 Check(TEXT("specialist_retains_fitted_face_morphs_and_physics"),Mesh&&!Mesh->GetMorphTargets().IsEmpty()&&U->Soldier->GetPhysicsAsset());
 if(AT)Check(TEXT("antitank_actual_mesh_bipod_socket"),U->Soldier->DoesSocketExist(TEXT("WF_Bipod")));
 if(RG){bool Found=false;TInlineComponentArray<UStaticMeshComponent*> Parts(U);for(auto* C:Parts)Found|=C->ComponentHasTag(RoundTag)&&C->GetStaticMesh();Check(TEXT("rifle_grenade_visible_ammunition_mesh"),Found);}
}
