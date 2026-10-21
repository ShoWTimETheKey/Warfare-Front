#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFrontlineEquipmentV41.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

FVector AWarfareFrontlineSoldier::GetMuzzleLocation() const
{
    if(WarfareFRRUCampaignV82::Active(Battle))return WarfareFRRUPresentationV82::Muzzle(this);
    return Soldier->GetSocketLocation(TEXT("WF_Muzzle"));
}

void AWarfareFrontlineSoldier::InitializeWeaponEffects()
{
    const bool National=WarfareFRRUCampaignV82::Active(Battle);
    if (National?!WarfareFRRUPresentationV82::HasBoundWeapon(this):!Soldier->DoesSocketExist(TEXT("WF_Muzzle")))
    {
        Battle->AssetFailure=TEXT("枪械挂点未能载入，请检查兵种资源安装。");
        return;
    }
    if (Role==EFrontlineRole::Mills && !National)
    {
        const bool British=GetFaction()==EFrontlineFaction::British;
        const TCHAR* Name=British?TEXT("SM_MillsBomb_V12"):TEXT("SM_StickGrenade_V12");
        auto* Asset=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Characters/V12/Projectiles/%s.%s"),Name,Name));
        if (!Asset) { Battle->AssetFailure=TEXT("手榴弹模型未能载入。"); return; }
        HeldGrenade=NewObject<UStaticMeshComponent>(this,TEXT("Held grenade"));
        HeldGrenade->SetupAttachment(Soldier,TEXT("hand_r")); HeldGrenade->SetStaticMesh(Asset);
        const FQuat Grip=British?FQuat(.14006165,.19857150,.02868417,.96960270):FQuat(-.96960264,-.02868416,.19857152,.14006169);
        HeldGrenade->SetRelativeTransform(FTransform(Grip,FVector(-1.965295,-9.979923,6.321775)));
        HeldGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision); HeldGrenade->SetVisibility(false); HeldGrenade->RegisterComponent();
    }
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* FlashBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Characters/V13/Materials/M_MuzzleFlash_V13.M_MuzzleFlash_V13"));
    auto* SmokeBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V12/Materials/M_ImpactVolume_1_V12.M_ImpactVolume_1_V12"));
    auto* DirtBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V12/Materials/M_ImpactVolume_0_V12.M_ImpactVolume_0_V12"));
    if (!Plane || !FlashBase || !SmokeBase || !DirtBase) { Battle->AssetFailure=TEXT("枪口效果资源未能载入。"); return; }
    auto Component=[&](const TCHAR* Name,UMaterialInterface* Base,TObjectPtr<UMaterialInstanceDynamic>& Material)
    {
        auto* Mesh=NewObject<UStaticMeshComponent>(this,FName(Name));
        Mesh->SetupAttachment(GetRootComponent()); Mesh->SetStaticMesh(Plane);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCastShadow(false);
        Mesh->SetVisibility(false); Material=UMaterialInstanceDynamic::Create(Base,this);
        Material->SetScalarParameterValue(TEXT("Density"),0); Mesh->SetMaterial(0,Material);
        Mesh->RegisterComponent(); return Mesh;
    };
    MuzzleFlash=Component(TEXT("Muzzle flash"),FlashBase,MuzzleFlashMaterial);
    MuzzleSmoke=Component(TEXT("Muzzle smoke"),SmokeBase,MuzzleSmokeMaterial);
    BulletDust=Component(TEXT("Bullet earth strike"),DirtBase,BulletDustMaterial);
    if (IsAutomaticWeapon()||FrontlineSpecialistsV41::IsAntiTankGunner(this))
    {
        // Only the actual automatic gunner carries a local muzzle light, not his
        // rifle escorts. One reused, shadowless light keeps burst readability cheap.
        MuzzleLight=NewObject<UPointLightComponent>(this,TEXT("Automatic muzzle light"));
        MuzzleLight->SetupAttachment(GetRootComponent()); MuzzleLight->SetMobility(EComponentMobility::Movable);
        MuzzleLight->SetIntensityUnits(ELightUnits::Lumens); MuzzleLight->SetIntensity(0);
        MuzzleLight->SetLightColor(FLinearColor(1.f,.48f,.16f));
        MuzzleLight->SetAttenuationRadius(FrontlineSpecialistsV41::IsAntiTankGunner(this)?340.f:180.f); MuzzleLight->SetSourceRadius(8.f);
        // Sub-frame flashes must not enter the fog's long temporal history.
        // Direct illumination of the weapon and ground remains unchanged.
        MuzzleLight->SetVolumetricScatteringIntensity(0.f);
        MuzzleLight->SetCastShadows(false); MuzzleLight->SetVisibility(false); MuzzleLight->RegisterComponent();
    }
}

void AWarfareFrontlineSoldier::ShowWeaponDischarge()
{
    if (!MuzzleFlash || !MuzzleSmoke) return;
    MuzzleAge=0; WeaponHeat=FMath::Min(1.f,WeaponHeat+(IsAutomaticWeapon()?.16f:.06f));
    LastMuzzlePosition=GetMuzzleLocation(); TickWeaponEffects(0);
}

void AWarfareFrontlineSoldier::TickWeaponEffects(float DT)
{
    FrontlineEquipmentV41::Tick(this);
    if (HeldGrenade)
    {
        const auto* Node=Soldier->GetSingleNodeInstance();
        // The hand first leaves the rifle and reaches to the equipment. Reveal
        // the held bomb after that pickup, then hide on the real release Notify.
        HeldGrenade->SetVisibility(IsActiveCombatant() && Battle->Phase==EFrontlinePhase::Fighting &&
            WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow && !GrenadeReleased && Node && Node->GetCurrentTime()>=.20f);
    }
    if (!MuzzleFlash || !MuzzleSmoke) return;
    MuzzleAge+=DT;
    WeaponHeat=FMath::Max(0.f,WeaponHeat-DT*.32f);
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    const FRotationMatrix Axes(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0));
    const FRotator Billboard=FRotationMatrix::MakeFromXY(Axes.GetUnitAxis(EAxis::Y),Axes.GetUnitAxis(EAxis::Z)).Rotator();
    BulletDustAge+=DT;
    if (BulletDust)
    {
        BulletDust->SetVisibility(BulletDustAge<.55f);
        if (BulletDustAge<.55f)
        {
            BulletDust->SetWorldRotation(Billboard);
            BulletDust->SetWorldLocation(LastBulletImpact+FVector(BulletDustAge*9,0,BulletDustAge*27));
            const float Size=.22f+BulletDustAge*.75f;
            BulletDust->SetWorldScale3D(FVector(Size,Size*1.5f,1));
            BulletDustMaterial->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.19f,.16f,.12f));
            BulletDustMaterial->SetScalarParameterValue(TEXT("Density"),.65f*FMath::Square(1-BulletDustAge/.55f));
        }
    }
    const bool Automatic=IsAutomaticWeapon();
    const bool Heavy=FrontlineSpecialistsV41::IsAntiTankGunner(this);
    const float FlashDuration=Heavy?.079f:Automatic?.054f:.036f,SmokeDuration=Heavy?1.38f:Automatic?1.85f:.84f;
    const float FlashEnvelope=FMath::Square(FMath::Max(0.f,1-MuzzleAge/FlashDuration));
    if (MuzzleLight)
    {
        MuzzleLight->SetWorldLocation(GetMuzzleLocation()); MuzzleLight->SetVisibility(FlashEnvelope>0);
        MuzzleLight->SetIntensity((Heavy?1650.f:620.f)*FlashEnvelope);
    }
    if (MuzzleAge>SmokeDuration) { MuzzleFlash->SetVisibility(false); MuzzleSmoke->SetVisibility(false); return; }
    MuzzleFlash->SetVisibility(MuzzleAge<FlashDuration);
    const FVector Forward=WarfareFRRUCampaignV82::Active(Battle)?WarfareFRRUPresentationV82::MuzzleForward(this):Soldier->GetSocketRotation(TEXT("WF_Muzzle")).Vector();
    MuzzleFlash->SetWorldLocation(GetMuzzleLocation()+Forward*(Heavy?17.f:Automatic?12.f:6.f)); MuzzleFlash->SetWorldRotation(Billboard);
    const float Shape=1.f+.13f*FMath::Sin(float(ShotsDischarged)*2.399963f);
    MuzzleFlash->SetWorldScale3D(Heavy?FVector(1.08f*Shape,.44f/Shape,1):Automatic?FVector(.56f*Shape,.32f/Shape,1):FVector(.23f*Shape,.13f/Shape,1));
    MuzzleFlashMaterial->SetVectorParameterValue(TEXT("DustColor"),Heavy?FLinearColor(18.f,6.8f,1.4f):Automatic?FLinearColor(25.f,11.f,2.5f):FLinearColor(16.f,7.f,1.8f));
    MuzzleFlashMaterial->SetScalarParameterValue(TEXT("Density"),FlashEnvelope);
    MuzzleSmoke->SetVisibility(true);
    MuzzleSmoke->SetWorldRotation(Billboard);
    MuzzleSmoke->SetWorldLocation(LastMuzzlePosition+Forward*(MuzzleAge*(Heavy?42.f:Automatic?25.f:12.f))+FVector(MuzzleAge*14,0,MuzzleAge*27));
    const float Size=Heavy?.46f+MuzzleAge*.96f:Automatic?.28f+WeaponHeat*.31f+MuzzleAge*.72f:.15f+MuzzleAge*.59f;
    MuzzleSmoke->SetWorldScale3D(FVector(Size,Size*.74f,1));
    MuzzleSmokeMaterial->SetVectorParameterValue(TEXT("DustColor"),Automatic?FLinearColor(.55f,.57f,.59f):FLinearColor(.62f,.64f,.65f));
    const float SmokeDensity=Heavy?.57f:Automatic?.38f+WeaponHeat*.27f:.27f;
    MuzzleSmokeMaterial->SetScalarParameterValue(TEXT("Density"),SmokeDensity*FMath::Square(1-MuzzleAge/SmokeDuration));
}

void AWarfareFrontlineSoldier::ShowBulletImpact(const FVector& Position)
{
    LastBulletImpact=Position; BulletDustAge=0;
}

FVector AWarfareFrontlineSoldier::GetGrenadeReleaseLocation() const
{
    // Resolve against the newly evaluated hand pose, not a child component
    // transform that may still contain the previous tick's attachment update.
    return HeldGrenade?(HeldGrenade->GetRelativeTransform()*Soldier->GetSocketTransform(TEXT("hand_r"),RTS_World)).GetLocation():Soldier->GetBoneLocation(TEXT("hand_r"));
}

void AWarfareFrontlineSoldier::ReleaseGrenade()
{
    if (!IsActiveCombatant() || !Battle || Battle->Phase!=EFrontlinePhase::Fighting || WeaponPhase!=EFrontlineWeaponPhase::GrenadeThrow || GrenadeReleased) return;
    GrenadeReleased=true; ++GrenadesReleased;
    Battle->ThrowGrenadeAt(this,GrenadeAimPoint);
    if (HeldGrenade) HeldGrenade->SetVisibility(false);
}
