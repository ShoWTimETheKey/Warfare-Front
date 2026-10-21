#include "WarfareFrontlineEmbersV27.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFlameVolumeV836.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"

void FrontlineEmbersV27::ReseatGround(UWorld* World)
{
    if(!World)return;
    // This actor persists across operation terrain rebuilds. Its two sites
    // cannot share one Z offset: each remnant has independent mud support.
    for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("V27BoundedBurningTimber")))
    {
        TInlineComponentArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)if(C&&C->GetStaticMesh())
        {
            if(C->ComponentHasTag(TEXT("WF_V836_TRUE_FLAME_VOLUME")))
            {
                FVector P=C->GetComponentLocation();P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+37.f;
                C->SetMobility(EComponentMobility::Movable);C->SetWorldLocation(P);
                if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))M->SetVectorParameterValue(TEXT("VolumeCenter"),FLinearColor(P));
                continue;
            }
            const FString Name=C->GetStaticMesh()->GetName();
            if(Name!=TEXT("SM_FallenTimber_V12")&&Name!=TEXT("SM_BrokenRevetment_B")&&Name!=TEXT("SM_BrokenRevetment_C"))continue;
            const FBox Box=C->GetStaticMesh()->GetBoundingBox().TransformBy(C->GetComponentTransform());
            if(!Box.IsValid||Box.GetSize().Z>45.f||FMath::Max(Box.GetSize().X,Box.GetSize().Y)>160.f)continue;
            const FVector Mid=Box.GetCenter(),Extent=Box.GetExtent();float Support= -MAX_flt;
            for(const FVector2D& Offset:{FVector2D(0,0),FVector2D(Extent.X*.25f,0),FVector2D(-Extent.X*.25f,0),FVector2D(0,Extent.Y*.25f),FVector2D(0,-Extent.Y*.25f)})
                Support=FMath::Max(Support,AWarfareFrontlineGround::HeightAt(Mid.X+Offset.X,Mid.Y+Offset.Y));
            FVector P=C->GetComponentLocation();P.Z+=Support-1.f-float(Box.Min.Z);
            C->SetMobility(EComponentMobility::Movable);C->SetWorldLocation(P);
        }
        TInlineComponentArray<UPointLightComponent*> Lights;It->GetComponents(Lights);
        for(auto* Light:Lights)if(Light)
        {
            FVector P=Light->GetComponentLocation();P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y)+26.f;
            Light->SetMobility(EComponentMobility::Movable);Light->SetWorldLocation(P);
        }
    }
}

void FrontlineEmbersV27::Ensure(UWorld* World,bool bVisible)
{
    if(!World)return;
    const FName Tag(TEXT("V27BoundedBurningTimber"));
    for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(Tag))
    {
        It->SetActorHiddenInGame(!bVisible);
        TInlineComponentArray<UPointLightComponent*> Lights;It->GetComponents(Lights);
        for(auto* Light:Lights)Light->SetVisibility(bVisible);
        ReseatGround(World);
        return;
    }
    if(!bVisible)return;
    auto* Flame=FrontlineFlameVolumeV836::Material();
    auto* Coal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V27/Materials/M_CharredTimber_V27.M_CharredTimber_V27"));
    UStaticMesh* Debris[]={
        LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_FallenTimber_V12.SM_FallenTimber_V12")),
        LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_BrokenRevetment_B.SM_BrokenRevetment_B")),
        LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_BrokenRevetment_C.SM_BrokenRevetment_C"))};
    if(!Flame || !Coal || !Debris[0] || !Debris[1] || !Debris[2]){UE_LOG(LogTemp,Warning,TEXT("WF_V27_EMBER_MATERIALS_MISSING"));return;}
    auto* Actor=World->SpawnActor<AActor>();if(!Actor)return;Actor->Tags.Add(Tag);
    auto* Root=NewObject<USceneComponent>(Actor);Actor->SetRootComponent(Root);Root->RegisterComponent();
    // Two residual fires by existing basin rims, clear of the +/-370cm firing
    // corridor. Zero collision, six timber meshes, two true-volume fire cores.
    const FVector2D Sites[]={{-1000,-670},{2050,-1230}};
    for(int32 Site=0;Site<2;++Site)
    {
        const auto XY=Sites[Site];const float Ground=AWarfareFrontlineGround::HeightAt(XY.X,XY.Y);
        for(int32 Piece=0;Piece<3;++Piece)
        {
            auto* Wood=NewObject<UStaticMeshComponent>(Actor);Wood->SetupAttachment(Root);Wood->SetMobility(EComponentMobility::Movable);Wood->SetStaticMesh(Debris[Piece]);
            for(int32 Slot=0;Slot<Wood->GetNumMaterials();++Slot)Wood->SetMaterial(Slot,Coal);
            Wood->SetCollisionEnabled(ECollisionEnabled::NoCollision);Wood->SetReceivesDecals(false);
            // One shattered remnant and two unequal splinters lie in the mud,
            // not three matching beams assembled into a campfire.
            const FVector Size=Debris[Piece]->GetBoundingBox().GetSize();
            const float Lengths[]={108.f,57.f,34.f};
            const float XYScale=Lengths[Piece]/FMath::Max(1.f,FMath::Max(Size.X,Size.Y));
            const FVector Scale(XYScale,XYScale,(Piece==0?15.f:7.f)/FMath::Max(1.f,Size.Z));
            const FRotator Rotation(Piece*3-2,23+Piece*49+Site*37,Piece*4-3);
            const FBox Fitted=Debris[Piece]->GetBoundingBox().TransformBy(FTransform(Rotation,FVector::ZeroVector,Scale));
            const float PX=XY.X+(Piece==0?-7.f:Piece==1?27.f:-32.f),PY=XY.Y+(Piece==0?0.f:Piece==1?21.f:-13.f);
            const float Z=AWarfareFrontlineGround::HeightAt(PX,PY);
            Wood->SetRelativeLocation(FVector(PX-Fitted.GetCenter().X,PY-Fitted.GetCenter().Y,Z-Fitted.Min.Z-1.f));
            Wood->SetRelativeRotation(Rotation);Wood->SetRelativeScale3D(Scale);
            Wood->SetCastShadow(true);Wood->SetVisibleInRayTracing(true);Wood->RegisterComponent();
        }
        if(auto* Core=FrontlineFlameVolumeV836::Create(Actor,*FString::Printf(TEXT("TimberFlameVolumeV836_%d"),Site)))
        {
            auto* M=UMaterialInstanceDynamic::Create(Flame,Actor);Core->SetMaterial(0,M);
            FVector Camera;FRotator View;
            if(auto* PC=World->GetFirstPlayerController())PC->GetPlayerViewPoint(Camera,View);
            else View=FRotator(-30,90,0);
            FrontlineFlameVolumeV836::Update(Core,M,FVector(XY.X,XY.Y,Ground+37),FVector(92,76,98),
                View.Vector(),.95f,Site*3.17f,0.f,0.f,false,true);
        }
        auto* Light=NewObject<UPointLightComponent>(Actor);Light->SetupAttachment(Root);
        Light->SetRelativeLocation(FVector(XY.X,XY.Y,Ground+26));Light->SetLightColor(FLinearColor(1,.43f,.14f));
        Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(90.f);Light->SetAttenuationRadius(230.f);
        Light->SetSourceRadius(24.f);Light->SetCastShadows(true);Light->RegisterComponent();
    }
    ReseatGround(World);
    UE_LOG(LogTemp,Display,TEXT("WF_V836_EMBERS sites=2 timber_meshes=6 true_flame_volumes=2 pause_aware_clock=1 point_lights=2 collision=none"));
}
