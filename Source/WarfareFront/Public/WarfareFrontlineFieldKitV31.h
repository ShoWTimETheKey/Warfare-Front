#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WarfareFrontlineFieldKitV31.generated.h"
class AWarfareFrontlineSoldier;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class FJsonObject;

/** Stable kit identity and a single physically detached real helmet per casualty. */
UCLASS()
class WARFAREFRONT_API UFrontlineFieldKitV31 : public UActorComponent
{
    GENERATED_BODY()
public:
    UFrontlineFieldKitV31();
    void Initialize(AWarfareFrontlineSoldier* Unit);
    void BindMaterials();
    void TryReleaseHelmet();
    virtual void TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Function) override;
    bool HasBayonet() const;
    bool HasDetachedHelmet() const { return bDetached; }
    int32 GetCoverStyle() const { return CoverStyle; }
    int32 GetHairVariant() const { return HairVariant; }
    static constexpr int32 MaximumDetachedHelmets=18;
private:
    void ApplyIdentity(UMaterialInstanceDynamic* Material,bool bLooseHelmet=false);
    UPROPERTY() TObjectPtr<AWarfareFrontlineSoldier> OwnerUnit;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bayonet;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LooseHelmet;
    uint32 Identity=0;
    float Dust=.5f,Rust=.4f,DetachedAge=0;
    int32 CoverStyle=0,HairVariant=0;
    bool bDetached=false,bReleaseAttempted=false;
};

namespace FrontlineFieldKitV31
{
UMaterialInterface* RefinedMaterial(UMaterialInterface* Source);
void Apply(AWarfareFrontlineSoldier* Unit);
void ReleaseHelmet(AWarfareFrontlineSoldier* Unit);
// Pure bounded policy used by production and focused runtime review.
FVector BlastVelocity(const FVector& Direction,bool bArtillery,float Strength,uint32 Identity);
TSharedRef<FJsonObject> Review(UWorld* World);
}
