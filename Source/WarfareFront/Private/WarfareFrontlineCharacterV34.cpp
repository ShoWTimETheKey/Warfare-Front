#include "WarfareFrontlineCharacterV34.h"
#include "WarfareFrontlineSoldierSurfaceV62.h"
#include "WarfareFrontlineSoldierSurfaceV67.h"
#include "WarfareFrontlineArtV68.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodySetup.h"
#include "TimerManager.h"

namespace
{
TAutoConsoleVariable<int32> GCharactersV34(TEXT("wf.Characters.V34"),1,TEXT("Use reconstructed infantry and fitted accessories for new deployments."));
}

namespace
{
struct FReleasedWeaponSpec {const TCHAR* Character;const TCHAR* Asset;const TCHAR* Bone;};
const FReleasedWeaponSpec ReleasedWeapons[]={
{TEXT("SK_British_RifleGrenadier_V41"),TEXT("/Game/Warfare/Characters/V41/Weapons/SM_Dropped_British_RifleGrenadier_V41.SM_Dropped_British_RifleGrenadier_V41"),TEXT("weapon_rifle")},
{TEXT("SK_German_AntiTankRifle_V41"),TEXT("/Game/Warfare/Characters/V41/Weapons/SM_Dropped_German_AntiTankRifle_V41.SM_Dropped_German_AntiTankRifle_V41"),TEXT("weapon_rifle")},
#include "WarfareFrontlineDroppedWeaponV34.inl"
};
TArray<TWeakObjectPtr<UStaticMeshComponent>> ActiveReleasedWeapons;
const FName ReleasedWeaponTag(TEXT("WF_V34_RELEASED_WEAPON"));
void SettleReleasedWeapon(UStaticMeshComponent* C)
{
    if(!IsValid(C))return;
    // An over-budget burst may retire a gun while it is still airborne. Place
    // that presentation proxy on the actual static surface, never freeze it
    // beside the dead soldier's chest. Already resting bodies keep their pose.
    const FVector Centre=C->Bounds.Origin;FHitResult Hit;
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(V34ReleasedWeaponSupport),false,C->GetOwner());
    bool Found=C->GetWorld()&&C->GetWorld()->LineTraceSingleByObjectType(Hit,Centre+FVector(0,0,2),Centre-FVector(0,0,10000),Objects,Query);
    const FVector Point=Found?Hit.ImpactPoint:FVector(Centre.X,Centre.Y,AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y));
    const FVector Normal=Found?Hit.ImpactNormal:FVector::UpVector;
    if(Centre.Z-C->Bounds.BoxExtent.Z>Point.Z+3.f)
    {
        FVector Along=FVector::VectorPlaneProject(C->GetComponentTransform().GetUnitAxis(EAxis::Y),Normal).GetSafeNormal();
        if(Along.IsNearlyZero())Along=FVector::VectorPlaneProject(FVector::ForwardVector,Normal).GetSafeNormal();
        const FQuat Rest=FRotationMatrix::MakeFromYZ(Along,Normal).ToQuat();
        const auto Bounds=C->GetStaticMesh()->GetBounds();const FVector Scale=C->GetComponentScale();
        C->SetWorldLocationAndRotation(Point+Normal*(Bounds.BoxExtent.Z*FMath::Abs(Scale.Z)+1.f)-Rest.RotateVector(Bounds.Origin*Scale),Rest,false,nullptr,ETeleportType::TeleportPhysics);
    }
    C->SetSimulatePhysics(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
}

UStaticMeshComponent* FrontlineCharacterV34::ReleaseWeapon(AWarfareFrontlineSoldier* Unit,bool Explosive,const FVector& ForceDirection)
{
    if(!Unit||!Unit->Soldier||!Unit->GetWorld())return nullptr;
    if(WarfareFRRUCampaignV82::Active(Unit->Battle))
    {
        bool LeaveEmplaced=false;auto* C=WarfareFRRUPresentationV82::ReleaseHeldWeaponV87(Unit,LeaveEmplaced);if(!C)return nullptr;
        const FName Finalized(TEXT("WF_FRRU_RELEASE_FINALIZED_V87"));if(C->ComponentHasTag(Finalized))return C;
        C->ComponentTags.Add(Finalized);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);C->SetGenerateOverlapEvents(false);
        const UStaticMesh* Mesh=C->GetStaticMesh();const auto* Collision=Mesh?Mesh->GetBodySetup():nullptr;
        const bool CanSimulate=!LeaveEmplaced&&Collision&&Collision->AggGeom.GetElementCount()>0&&Collision->CollisionTraceFlag!=CTF_UseComplexAsSimple;
        if(!CanSimulate)
        {C->SetSimulatePhysics(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);SettleReleasedWeapon(C);
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_WEAPON_RELEASE mesh=%s mode=%s"),*GetPathNameSafe(Mesh),LeaveEmplaced?TEXT("emplaced"):TEXT("no_simple_collision_settled"));return C;}
        C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);C->SetCollisionObjectType(ECC_PhysicsBody);
        C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
        auto* Physics=NewObject<UPhysicalMaterial>(C);Physics->Friction=.62f;Physics->Restitution=.04f;C->SetPhysMaterialOverride(Physics);
        C->SetMassOverrideInKg(NAME_None,WarfareFRRUPresentationV82::UsesPistol(Unit)?1.f:Unit->IsMachineGunner()?12.f:4.2f,true);C->SetLinearDamping(.5f);C->SetAngularDamping(.9f);C->SetSimulatePhysics(true);
        if(!C->IsSimulatingPhysics()){C->SetCollisionEnabled(ECollisionEnabled::NoCollision);SettleReleasedWeapon(C);return C;}
        const FVector Kick=Explosive?ForceDirection.GetSafeNormal()*230.f+FVector(0,0,80):FVector(0,0,-20);
        C->SetPhysicsLinearVelocity(Unit->GetActorForwardVector()*FMath::Min(Unit->GetMotionSpeed(),280.f)+Kick);
        const float Phase=Unit->GetMotionPhase();C->SetPhysicsAngularVelocityInDegrees(FVector(25+Phase*45,(Phase-.5f)*100,20-Phase*40)*(Explosive?2.f:1.f));
        ActiveReleasedWeapons.RemoveAll([](const auto& W){return !W.IsValid()||!W->IsSimulatingPhysics();});
        while(ActiveReleasedWeapons.Num()>=16){SettleReleasedWeapon(ActiveReleasedWeapons[0].Get());ActiveReleasedWeapons.RemoveAt(0);}
        ActiveReleasedWeapons.Add(C);FTimerHandle Handle;TWeakObjectPtr<UStaticMeshComponent> Weak(C);
        Unit->GetWorld()->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateLambda([Weak](){SettleReleasedWeapon(Weak.Get());}),4.f,false);
        return C;
    }
    const auto* Body=Unit->Soldier->GetSkeletalMeshAsset();if(!Body)return nullptr;
    const FReleasedWeaponSpec* Spec=nullptr;
    FString BodyName=FrontlineSoldierSurfaceV67::SourceMeshName(Body->GetName());if(BodyName.EndsWith(TEXT("_V62")))BodyName=BodyName.LeftChop(4)+TEXT("_V34");
    for(const auto& Row:ReleasedWeapons)if(BodyName==Row.Character){Spec=&Row;break;}
    if(!Spec)return nullptr;
    TInlineComponentArray<UStaticMeshComponent*> Existing(Unit);
    for(auto* C:Existing)if(C->ComponentHasTag(ReleasedWeaponTag))return C;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,Spec->Asset);
    if(!Mesh){UE_LOG(LogTemp,Warning,TEXT("WF_V34_WEAPON_RELEASE_MISSING %s"),Spec->Asset);return nullptr;}
    const FTransform Held=Unit->Soldier->GetSocketTransform(Spec->Bone);
    auto* C=NewObject<UStaticMeshComponent>(Unit);
    Unit->AddInstanceComponent(C);C->ComponentTags.Add(ReleasedWeaponTag);
    C->SetStaticMesh(Mesh);C->SetMobility(EComponentMobility::Movable);
    C->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);C->SetCollisionObjectType(ECC_PhysicsBody);
    C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);C->RegisterComponent();C->SetWorldTransform(Held);
    // Keep the actual owner's steel, wood and mud when changing representation.
    for(int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)
    {
        const FName Slot=Mesh->GetStaticMaterials()[I].MaterialSlotName;
        const int32 Source=Unit->Soldier->GetMaterialIndex(Slot);
        if(Source!=INDEX_NONE)C->SetMaterial(I,Unit->Soldier->GetMaterial(Source));
    }
    // The separate bayonet follows the released gun; it must not remain on a hidden bone.
    for(auto* Child:Existing)if(Child->GetAttachParent()==Unit->Soldier && Child->GetAttachSocketName()==FName(Spec->Bone))
        Child->AttachToComponent(C,FAttachmentTransformRules::KeepWorldTransform);
    auto* Physics=NewObject<UPhysicalMaterial>(C);Physics->Friction=.62f;Physics->Restitution=.04f;
    C->SetPhysMaterialOverride(Physics);
    C->SetMassOverrideInKg(NAME_None,Unit->Role==EFrontlineRole::AntiTankRifle&&Unit->MemberIndex==0?16.f:Unit->IsCommissionedOfficer()?1.f:Unit->IsMachineGunner()?18.f:4.2f,true);
    C->SetLinearDamping(.5f);C->SetAngularDamping(.9f);C->SetSimulatePhysics(true);
    const FVector Kick=Explosive?ForceDirection.GetSafeNormal()*230.f+FVector(0,0,80):FVector(0,0,-20);
    C->SetPhysicsLinearVelocity(Unit->GetActorForwardVector()*FMath::Min(Unit->GetMotionSpeed(),280.f)+Kick);
    const float Phase=Unit->GetMotionPhase();
    C->SetPhysicsAngularVelocityInDegrees(FVector(25+Phase*45,(Phase-.5f)*100,20-Phase*40)*(Explosive?2.f:1.f));
    Unit->Soldier->HideBoneByName(Spec->Bone,EPhysBodyOp::PBO_None);
    ActiveReleasedWeapons.RemoveAll([](const auto& W){return !W.IsValid()||!W->IsSimulatingPhysics();});
    while(ActiveReleasedWeapons.Num()>=16){SettleReleasedWeapon(ActiveReleasedWeapons[0].Get());ActiveReleasedWeapons.RemoveAt(0);}
    ActiveReleasedWeapons.Add(C);
    FTimerHandle Handle;TWeakObjectPtr<UStaticMeshComponent> Weak(C);
    Unit->GetWorld()->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateLambda([Weak](){SettleReleasedWeapon(Weak.Get());}),4.f,false);
    UE_LOG(LogTemp,Display,TEXT("WF_V34_WEAPON_RELEASE character=%s mesh=%s active_physics=%d"),*Body->GetName(),*Mesh->GetName(),ActiveReleasedWeapons.Num());
    return C;
}

void FrontlineCharacterV34::ConsumeReleasedWeapon(AWarfareFrontlineSoldier* Unit)
{
    if(!Unit)return;TInlineComponentArray<UStaticMeshComponent*> Parts(Unit);
    for(auto* C:Parts)if(C->ComponentHasTag(ReleasedWeaponTag))
    {
        TArray<USceneComponent*> Children;C->GetChildrenComponents(true,Children);
        for(auto* Child:Children)if(IsValid(Child))Child->DestroyComponent();
        C->DestroyComponent();
    }
}
USkeletalMesh* FrontlineCharacterV34::TryLoad(const FString& OriginalPath)
{
    if(GCharactersV34.GetValueOnGameThread()==0 || FParse::Param(FCommandLine::Get(),TEXT("WFV34Baseline")) ||
        FParse::Param(FCommandLine::Get(),TEXT("WFV29Baseline")))return nullptr;
    FString Name=OriginalPath;int32 Delimiter=INDEX_NONE;
    if(Name.FindLastChar('.',Delimiter) || Name.FindLastChar('/',Delimiter))Name=Name.Mid(Delimiter+1);
    bool Known=false;
    for(const TCHAR* Suffix:{TEXT("_V16"),TEXT("_V23"),TEXT("_V24"),TEXT("_V29"),TEXT("_V32"),TEXT("_V33"),TEXT("_V34")})
        if(Name.EndsWith(Suffix)){Name.LeftChopInline(4);Name+=TEXT("_V34");Known=true;break;}
    if(!Known || !Name.StartsWith(TEXT("SK_")))return nullptr;
    const FString Folder=Name.StartsWith(TEXT("SK_Casualty_"))?TEXT("Casualties"):TEXT("Meshes");
    const FString Path=TEXT("/Game/Warfare/Characters/V34/")+Folder+TEXT("/")+Name+TEXT(".")+Name;
    if(auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*Path))return FrontlineArtV68::ResolveMesh(FrontlineSoldierSurfaceV67::ResolveMesh(FrontlineSoldierSurfaceV62::ResolveMesh(Mesh)));
    static TSet<FString> Reported;
    if(!Reported.Contains(Path)){Reported.Add(Path);UE_LOG(LogTemp,Warning,TEXT("WF_V34_MESH_FALLBACK %s"),*Path);}
    return nullptr;
}
