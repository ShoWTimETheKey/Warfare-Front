#include "WarfareFrontlineFlameVolumeV836.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UMaterialInterface* FrontlineFlameVolumeV836::Material()
{
    static TStrongObjectPtr<UMaterialInterface> Held;
    if(!Held.IsValid())
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline")))Held.Reset(LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Warfare/Field/V8310/Materials/M_FlameVolume_V8310.M_FlameVolume_V8310")));
        if(!Held.IsValid())Held.Reset(LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Warfare/Field/V836/Materials/M_FlameVolume_V836.M_FlameVolume_V836")));
    }
    return Held.Get();
}
UMaterialInterface* FrontlineFlameVolumeV836::WreckMaterialV8311()
{
    static TStrongObjectPtr<UMaterialInterface> Held;
    if(!Held.IsValid()&&!FParse::Param(FCommandLine::Get(),TEXT("WFV8311FireBaseline"))
        &&!FParse::Param(FCommandLine::Get(),TEXT("WFV8310FlameBaseline")))
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFV8311InitialFlame")))
            Held.Reset(LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Warfare/Field/V8311/Materials/M_WreckCombustion_Turbulent_V8311.M_WreckCombustion_Turbulent_V8311")));
        if(!Held.IsValid())
        Held.Reset(LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Warfare/Field/V8311/Materials/M_WreckCombustion_V8311.M_WreckCombustion_V8311")));
    }
    return Held.IsValid()?Held.Get():Material();
}
UStaticMeshComponent* FrontlineFlameVolumeV836::Create(AActor* Owner,FName Name,bool Wreck)
{
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Base=Wreck?WreckMaterialV8311():Material();if(!Owner||!Cube||!Base)return nullptr;
    auto* C=NewObject<UStaticMeshComponent>(Owner,Name);Owner->AddInstanceComponent(C);
    C->SetupAttachment(Owner->GetRootComponent());C->SetStaticMesh(Cube);C->SetMaterial(0,Base);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
    C->SetCastShadow(false);C->SetReceivesDecals(false);C->SetVisibleInRayTracing(false);
    C->SetAffectDistanceFieldLighting(false);C->SetAffectDynamicIndirectLighting(false);
    C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;
    C->SetMobility(EComponentMobility::Movable);C->SetVisibility(false);
    C->ComponentTags.Add(TEXT("WF_V836_TRUE_FLAME_VOLUME"));C->RegisterComponent();return C;
}
void FrontlineFlameVolumeV836::Update(UStaticMeshComponent* C,UMaterialInstanceDynamic* M,
    const FVector& Centre,const FVector& Size,const FVector& Forward,float Strength,float Seed,float Clock,
    float Age,bool Contact,bool WorldClock)
{
    if(!C||!M)return;
    if(Centre.ContainsNaN()||Size.ContainsNaN()||Forward.ContainsNaN()||Size.GetMin()<2.f||Size.GetMax()>2000.f
        ||!FMath::IsFinite(Strength)||!FMath::IsFinite(Seed)||!FMath::IsFinite(Clock)||!FMath::IsFinite(Age))
    {C->SetVisibility(false);return;}
    C->SetWorldLocation(Centre);C->SetWorldRotation(FRotator::ZeroRotator);C->SetWorldScale3D(Size/100.f);
    C->SetVisibility(Strength>.01f);
    M->SetVectorParameterValue(TEXT("VolumeCenter"),FLinearColor(Centre));
    M->SetVectorParameterValue(TEXT("VolumeRadii"),FLinearColor(Size*.5));
    M->SetVectorParameterValue(TEXT("ViewForward"),FLinearColor(Forward.GetSafeNormal(.0001f,FVector(0,-1,-.28).GetSafeNormal())));
    M->SetScalarParameterValue(TEXT("Density"),FMath::Clamp(Strength,0.f,2.5f));
    M->SetScalarParameterValue(TEXT("Seed"),Seed);M->SetScalarParameterValue(TEXT("ClockOverride"),Clock);
    M->SetScalarParameterValue(TEXT("UseOverride"),WorldClock?0.f:1.f);
    M->SetScalarParameterValue(TEXT("Age"),Age);M->SetScalarParameterValue(TEXT("Profile"),Contact?1.f:0.f);
    const FName Parent=M->GetBaseMaterial()->GetFName();
    M->SetScalarParameterValue(TEXT("QualitySteps"),Parent==TEXT("M_WreckCombustion_Turbulent_V8311")?64.f:
        Parent==TEXT("M_WreckCombustion_V8311")?28.f:Parent==TEXT("M_FlameVolume_V8310")?18.f:12.f);
    float ReviewClock=-1.f;
    if((FParse::Param(FCommandLine::Get(),TEXT("WFV8310Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review")))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("WFV8310FlameCompareClock="),ReviewClock)&&FMath::IsFinite(ReviewClock)&&ReviewClock>=0.f&&ReviewClock<=3600.f)
    {M->SetScalarParameterValue(TEXT("ClockOverride"),ReviewClock);M->SetScalarParameterValue(TEXT("UseOverride"),1.f);}
}
