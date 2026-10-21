#include "WarfareFrontlineFieldKitV31.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineProgression.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Dom/JsonObject.h"

namespace
{
const FLinearColor HairColors[]={FLinearColor(.013f,.010f,.007f),FLinearColor(.043f,.025f,.013f),
    FLinearColor(.095f,.065f,.028f),FLinearColor(.064f,.021f,.009f),FLinearColor(.075f,.066f,.052f)};
uint32 UnitIdentity(const AWarfareFrontlineSoldier* U)
{return 0x31a19717u^(uint32(U->SquadId+1)*0x9e3779b9u)^(uint32(U->MemberIndex+1)*0x85ebca6bu)^(uint32(U->Team+1)*0xc2b2ae35u);}
UStaticMesh* KitMesh(const TCHAR* Kind,bool bGerman)
{
    const FString N=FString(TEXT("SM_"))+Kind+(bGerman?TEXT("_German_V31"):TEXT("_British_V31"));
    return LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Warfare/Characters/V31/Meshes/")+N+TEXT(".")+N));
}
}

UFrontlineFieldKitV31::UFrontlineFieldKitV31()
{PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.10f;PrimaryComponentTick.TickGroup=TG_PostPhysics;}
void UFrontlineFieldKitV31::Initialize(AWarfareFrontlineSoldier* Unit)
{
    OwnerUnit=Unit;if(!Unit || !Unit->Soldier)return;
    Identity=UnitIdentity(Unit);FRandomStream R{int32(Identity)};Dust=R.FRandRange(.25f,.86f);Rust=R.FRandRange(.18f,.79f);HairVariant=R.RandRange(0,UE_ARRAY_COUNT(HairColors)-1);
    const bool German=Unit->GetFaction()==EFrontlineFaction::German;
    const auto* Battle=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();
    const auto* Mission=Battle?Battle->GetEncounterCampaignMission():nullptr;
    // Arras/Messines/Ypres/Cambrai remain field grey. Standard geometric
    // German camouflage is restricted to the late-1918 Amiens chapter.
    const bool Late1918=Mission && Mission->Year>=1918 && Mission->Id>=5;
    CoverStyle=Unit->IsCommissionedOfficer()?0:(!German && Identity%4u==0u?1:(German && Late1918 && Identity%3u==0u?2:0));
    if(Unit->IsMachineGunner())Dust=FMath::Min(1.f,Dust+.13f);
    BindMaterials();
}
void UFrontlineFieldKitV31::ApplyIdentity(UMaterialInstanceDynamic* M,bool Loose)
{
    if(!M)return;
    M->SetScalarParameterValue(TEXT("DeploymentSeed"),float(Identity%997u));
    M->SetScalarParameterValue(TEXT("FieldDust"),Dust);M->SetScalarParameterValue(TEXT("FieldRust"),Rust);
    M->SetScalarParameterValue(TEXT("FieldCoverStyle"),float(CoverStyle));
    M->SetScalarParameterValue(TEXT("HelmetMesh"),Loose?1.f:0.f);M->SetScalarParameterValue(TEXT("HelmetDropped"),bDetached && !Loose?1.f:0.f);
    M->SetVectorParameterValue(TEXT("HairColor"),HairColors[HairVariant]);
    const float Dye=.87f+float((Identity>>8)%101u)*.002f;
    // Dye variation remains neutral field-grey for German uniforms.
    M->SetVectorParameterValue(TEXT("PersonalWeatheringTint"),FLinearColor(Dye,Dye*.99f,Dye*.975f));
}
void UFrontlineFieldKitV31::BindMaterials()
{
    if(!OwnerUnit || !OwnerUnit->Soldier)return;
    for(int32 I=0;I<OwnerUnit->Soldier->GetNumMaterials();++I)ApplyIdentity(Cast<UMaterialInstanceDynamic>(OwnerUnit->Soldier->GetMaterial(I)));
    // A rifle-only bayonet; never leave a blade attached after the bearer
    // recovers a machine gun or a specialist switches to an automatic weapon.
    const bool Eligible=OwnerUnit->IsAlive() && OwnerUnit->Role!=EFrontlineRole::Flamethrower && OwnerUnit->Role!=EFrontlineRole::RifleGrenadier && OwnerUnit->Role!=EFrontlineRole::AntiTankRifle && !OwnerUnit->IsAutomaticWeapon() && !OwnerUnit->IsScopedMarksman() && !OwnerUnit->IsCommissionedOfficer()
        && OwnerUnit->Soldier->DoesSocketExist(TEXT("weapon_rifle")) && Identity%3u!=0u;
    if(Bayonet){Bayonet->SetVisibility(Eligible);return;}
    if(!Eligible)return;
    auto* Mesh=KitMesh(TEXT("Bayonet"),OwnerUnit->GetFaction()==EFrontlineFaction::German);if(!Mesh)return;
    Bayonet=NewObject<UStaticMeshComponent>(OwnerUnit,TEXT("FixedFieldBayonet_V31"));OwnerUnit->AddInstanceComponent(Bayonet);
    Bayonet->SetupAttachment(OwnerUnit->Soldier,TEXT("weapon_rifle"));Bayonet->SetStaticMesh(Mesh);Bayonet->SetRelativeTransform(FTransform::Identity);
    Bayonet->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bayonet->SetCastShadow(true);Bayonet->SetReceivesDecals(false);Bayonet->RegisterComponent();
    for(int32 I=0;I<Bayonet->GetNumMaterials();++I)ApplyIdentity(Bayonet->CreateDynamicMaterialInstance(I));
}
bool UFrontlineFieldKitV31::HasBayonet() const {return Bayonet && Bayonet->IsVisible();}
void UFrontlineFieldKitV31::TryReleaseHelmet()
{
    if(bReleaseAttempted || !OwnerUnit || OwnerUnit->IsAlive() || !OwnerUnit->Soldier)return;
    const auto& Death=OwnerUnit->GetDeathState();if(Death.PublishedPoseSamples<1)return;
    bReleaseAttempted=true;
    if(OwnerUnit->IsCommissionedOfficer()||(OwnerUnit->Role==EFrontlineRole::Flamethrower&&OwnerUnit->GetFaction()==EFrontlineFaction::British))return;
    const bool Blast=IsFrontlineExplosiveDamage(Death.Kind);
    if((Blast && Death.ForceScale<.32f) || (!Blast && Identity%5u!=0u))return;
    int32 Count=0;
    for(TActorIterator<AWarfareFrontlineSoldier> It(GetWorld());It;++It)
        if(const auto* Kit=It->FindComponentByClass<UFrontlineFieldKitV31>())Count+=Kit->HasDetachedHelmet()?1:0;
    if(Count>=MaximumDetachedHelmets)return;
    auto* Mesh=KitMesh(TEXT("Helmet"),OwnerUnit->GetFaction()==EFrontlineFaction::German);if(!Mesh || !Mesh->GetBodySetup())return;
    // The exact old helmet must have the new section mask; a missing import
    // keeps the intact helmet rather than duplicating it in flight.
    bool MaskReady=false;
    for(int32 I=0;I<OwnerUnit->Soldier->GetNumMaterials();++I)
        if(auto* M=OwnerUnit->Soldier->GetMaterial(I))
        {float Value=0;if(M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("HelmetDropped")),Value))MaskReady=true;}
    if(!MaskReady)return;
    LooseHelmet=NewObject<UStaticMeshComponent>(OwnerUnit,TEXT("ReleasedFieldHelmet_V31"));OwnerUnit->AddInstanceComponent(LooseHelmet);
    LooseHelmet->SetStaticMesh(Mesh);LooseHelmet->SetWorldTransform(OwnerUnit->Soldier->GetSocketTransform(TEXT("Head")));
    LooseHelmet->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);LooseHelmet->SetCollisionObjectType(ECC_PhysicsBody);
    LooseHelmet->SetCollisionResponseToAllChannels(ECR_Ignore);LooseHelmet->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    LooseHelmet->SetCastShadow(true);LooseHelmet->SetReceivesDecals(false);LooseHelmet->RegisterComponent();
    LooseHelmet->SetMassOverrideInKg(NAME_None,1.15f,true);LooseHelmet->SetLinearDamping(.24f);LooseHelmet->SetAngularDamping(.55f);LooseHelmet->SetSimulatePhysics(true);
    if(!LooseHelmet->IsSimulatingPhysics()){LooseHelmet->DestroyComponent();LooseHelmet=nullptr;return;}
    bDetached=true;DetachedAge=0;
    for(int32 I=0;I<LooseHelmet->GetNumMaterials();++I)ApplyIdentity(LooseHelmet->CreateDynamicMaterialInstance(I),true);
    for(int32 I=0;I<OwnerUnit->Soldier->GetNumMaterials();++I)ApplyIdentity(Cast<UMaterialInstanceDynamic>(OwnerUnit->Soldier->GetMaterial(I)));
    const FVector Dir=Death.IncomingDirection.GetSafeNormal();FRandomStream R(int32(Identity^0x31b10adu));
    const FVector Velocity=Death.InheritedVelocity+Dir*(Blast?220.f+340.f*Death.ForceScale:48.f)+FVector(R.FRandRange(-42,42),R.FRandRange(-60,60),Blast?210.f+180.f*Death.ForceScale:50.f);
    LooseHelmet->SetPhysicsLinearVelocity(Velocity);LooseHelmet->SetPhysicsAngularVelocityInRadians(FVector(R.FRandRange(-9,9),R.FRandRange(-7,7),R.FRandRange(-5,5)));
    SetComponentTickEnabled(true);
    UE_LOG(LogTemp,Display,TEXT("WF_V31_HELMET_RELEASE squad=%d member=%d blast=%d speed=%.2f true_source_geometry=1 active=%d budget=%d"),OwnerUnit->SquadId,OwnerUnit->MemberIndex,int32(Blast),Velocity.Size(),Count+1,MaximumDetachedHelmets);
}
void UFrontlineFieldKitV31::TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(DT,Type,Function);
    if(!OwnerUnit){SetComponentTickEnabled(false);return;}
    if(OwnerUnit->IsAlive())return; // Stable living identity has no per-frame shader work.
    TryReleaseHelmet();
    if(bDetached && LooseHelmet)
    {
        DetachedAge+=DT;
        if(DetachedAge>8.f){LooseHelmet->PutAllRigidBodiesToSleep();SetComponentTickEnabled(false);}
    }
    else if(bReleaseAttempted)SetComponentTickEnabled(false);
}
UMaterialInterface* FrontlineFieldKitV31::RefinedMaterial(UMaterialInterface* Source)
{
    if(!Source || Source->GetName().StartsWith(TEXT("V31_")))return Source;
    const FString N=TEXT("V31_")+Source->GetName();
    if(auto* M=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Warfare/Characters/V31/Materials/")+N+TEXT(".")+N)))return M;
    return Source;
}
void FrontlineFieldKitV31::Apply(AWarfareFrontlineSoldier* Unit)
{
    if(!Unit)return;
    if(auto* Kit=Unit->FindComponentByClass<UFrontlineFieldKitV31>()){Kit->BindMaterials();return;}
    auto* Kit=NewObject<UFrontlineFieldKitV31>(Unit,TEXT("PersonalFieldKit_V31"));Unit->AddInstanceComponent(Kit);Kit->RegisterComponent();Kit->Initialize(Unit);
}
void FrontlineFieldKitV31::ReleaseHelmet(AWarfareFrontlineSoldier* Unit)
{if(Unit)if(auto* Kit=Unit->FindComponentByClass<UFrontlineFieldKitV31>())Kit->TryReleaseHelmet();}
FVector FrontlineFieldKitV31::BlastVelocity(const FVector& Direction,bool Artillery,float Strength,uint32 Identity)
{
    Strength=FMath::Clamp(Strength,0.f,1.f);const float Near=FMath::Square(Strength);
    const float Variation=.90f+float(Identity%31u)*.006f;
    // Broad common velocity preserves articulated constraints. Edge-of-blast
    // casualties fall locally; only close heavy HE can lift a body 1–2 metres.
    const FVector Flat=Direction.GetSafeNormal2D();
    return Flat*((Artillery?510.f:225.f)*Near*Variation)+FVector(0,0,(Artillery?580.f:165.f)*Near*Variation);
}
TSharedRef<FJsonObject> FrontlineFieldKitV31::Review(UWorld* World)
{
    auto R=MakeShared<FJsonObject>();int32 Kits=0,Blades=0,Covers=0,Loose=0;TSet<int32> Hair;
    if(World)for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)if(auto* K=It->FindComponentByClass<UFrontlineFieldKitV31>())
    {++Kits;Blades+=K->HasBayonet()?1:0;Covers+=K->GetCoverStyle()>0?1:0;Loose+=K->HasDetachedHelmet()?1:0;Hair.Add(K->GetHairVariant());}
    const FVector Core=BlastVelocity(FVector::ForwardVector,true,1,7),Edge=BlastVelocity(FVector::ForwardVector,true,.2f,7),Grenade=BlastVelocity(FVector::ForwardVector,false,1,7);
    const bool Policy=Core.Z>500 && Core.Size()<900 && Edge.Size()<Core.Size()*.05f && Grenade.Z<200 && Loose<=UFrontlineFieldKitV31::MaximumDetachedHelmets;
    R->SetBoolField(TEXT("success"),Policy);R->SetNumberField(TEXT("kits"),Kits);R->SetNumberField(TEXT("bayonets"),Blades);R->SetNumberField(TEXT("covered_or_painted_helmets"),Covers);R->SetNumberField(TEXT("hair_variants"),Hair.Num());R->SetNumberField(TEXT("detached_helmets"),Loose);
    R->SetNumberField(TEXT("heavy_core_launch_cm_s"),Core.Size());R->SetNumberField(TEXT("grenade_core_vertical_cm_s"),Grenade.Z);R->SetStringField(TEXT("scope"),TEXT("Live kit inventory and production blast policy; visual fit requires screenshots."));return R;
}
