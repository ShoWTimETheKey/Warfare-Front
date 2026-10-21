#include "WarfareFrontlineArmourWreckV8220.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFrontlineGround.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
const FName TransitionTag(TEXT("CooledWreckV8221"));
const TCHAR* SurfaceKeys[]={TEXT("British"),TEXT("German"),TEXT("Steel"),TEXT("WoodEnd"),TEXT("Wood"),TEXT("White"),TEXT("Black"),TEXT("Rubber"),TEXT("Rust"),TEXT("Stencil")};
const TCHAR* BodyKeys[]={TEXT("MarkIVMale"),TEXT("Whippet"),TEXT("A7V"),TEXT("BeuteMarkIV"),TEXT("MarkIVFemale"),TEXT("BeuteMarkIVFemale"),TEXT("MarkVMale"),TEXT("A7VGretchen"),TEXT("MarkVStar"),TEXT("Austin1918"),TEXT("RailConvoy")};
TMap<FString,TStrongObjectPtr<UMaterialInterface>>& Surfaces()
{static TMap<FString,TStrongObjectPtr<UMaterialInterface>> Cache;return Cache;}
TMap<FString,TStrongObjectPtr<UStaticMesh>>& WreckBodies()
{static TMap<FString,TStrongObjectPtr<UStaticMesh>> Cache;return Cache;}
FString BodyPath(const FString& Key)
{
    if(Key==TEXT("RailConvoy"))return TEXT("/Game/Warfare/Vehicles/V837/SM_RailConvoy_Wreck_V837.SM_RailConvoy_Wreck_V837");
    if(auto* Current=FrontlineVehicleArtV836::ResolveMesh(TEXT("SM_")+Key+TEXT("_Wreck")))return Current->GetPathName();
    const FString Name=TEXT("SM_")+Key+TEXT("_Wreck_V836");return TEXT("/Game/Warfare/Vehicles/V836/WreckMeshes/")+Name+TEXT(".")+Name;
}
FString SurfaceKey(const UMaterialInterface* Surface)
{
    if(!Surface)return FString();
    const FString Name=Surface->GetMaterial()->GetName();
    if(Name==TEXT("M_Vehicle_RailPaint"))return TEXT("German");
    if(Name==TEXT("M_Vehicle_RailSteel"))return TEXT("Steel");
    for(const TCHAR* Key:SurfaceKeys)
        if(Name==FString(TEXT("M_Vehicle_"))+Key||Name==FString(TEXT("M_Vehicle_"))+Key+TEXT("_V8218")
            ||(Name.StartsWith(TEXT("M_"))&&(Name.EndsWith(FString(TEXT("_"))+Key+TEXT("_V838"))||Name.EndsWith(FString(TEXT("_"))+Key+TEXT("_Wreck_V838"))
                ||Name.EndsWith(FString(TEXT("_"))+Key+TEXT("_V8310"))||Name.EndsWith(FString(TEXT("_"))+Key+TEXT("_Wreck_V8310"))))
            ||Name==FString(TEXT("M_Wreck_"))+Key+TEXT("_V8221")
            ||Name==FString(TEXT("M_Wreck_"))+Key+TEXT("_V8220")
            ||Name==FString(TEXT("M_Wreck_"))+Key+TEXT("_V836"))return Key;
    return FString();
}
FLinearColor VectorParameter(const FVector& Vector)
{return FLinearColor(Vector.X,Vector.Y,Vector.Z,0);}
}

bool FFrontlineArmourWreckV8220Access::Warm()
{
    bool Ready=true;
    for(const TCHAR* Key:SurfaceKeys)
    {
        auto* Cached=Surfaces().Find(Key);if(Cached&&Cached->IsValid())continue;
        const FString Name=FString(TEXT("M_Wreck_"))+Key+TEXT("_V836");
        auto* Material=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V836/Materials/")+Name+TEXT(".")+Name));
        if(Material)Surfaces().Add(Key,TStrongObjectPtr<UMaterialInterface>(Material));else Ready=false;
    }
    for(const TCHAR* Key:BodyKeys)
    {
        auto* Cached=WreckBodies().Find(Key);if(Cached&&Cached->IsValid())continue;
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*BodyPath(Key));
        if(Mesh)WreckBodies().Add(Key,TStrongObjectPtr<UStaticMesh>(Mesh));else Ready=false;
    }
    return Ready;
}

void FFrontlineArmourWreckV8220Access::Apply(AFrontlineTank* Tank)
{
    if(!IsValid(Tank)||Tank->IsOperational()||Tank->Tags.Contains(TransitionTag))return;
    // Frozen national receivers own their material/contact contract. The
    // ordinary British/German and rail actor use the accepted shared surfaces.
    if(WarfareFRRUArmourV82::National(Tank)||Tank->bNationalGermanGroundV82)return;
    Tank->Tags.Add(TransitionTag);
    const FTransform Pose=Tank->GetActorTransform();
    const float Length=Tank->Body&&Tank->Body->GetStaticMesh()?Tank->Body->GetStaticMesh()->GetBoundingBox().GetExtent().X:390.f;
    const float Height=Tank->bRailConvoyV32?412.f:Tank->Type==EFrontlineVehicle::A7V||Tank->Type==EFrontlineVehicle::A7VGretchen?252.f:223.f;
    if(Tank->Body&&Tank->Body->GetStaticMesh())
    {
        const FString Key=UFrontlineVehicleSubsystem::Definition(Tank->Type).AssetKey;
        auto* Cached=WreckBodies().Find(Key);
        if(!Cached||!Cached->IsValid()){Warm();Cached=WreckBodies().Find(Key);}
        auto* Wreck=Cached?Cached->Get():nullptr;
        // Preserve service paint parameters and the accepted slot contract.
        // A damaged derivative is used only after the vehicle is non-operational.
        if(Wreck&&Wreck->GetStaticMaterials().Num()==Tank->Body->GetStaticMesh()->GetStaticMaterials().Num())
        {
            TArray<UMaterialInterface*> Previous;
            for(int32 Slot=0;Slot<Tank->Body->GetNumMaterials();++Slot)Previous.Add(Tank->Body->GetMaterial(Slot));
            Tank->Body->SetStaticMesh(Wreck);
            for(int32 Slot=0;Slot<Previous.Num();++Slot)Tank->Body->SetMaterial(Slot,Previous[Slot]);
            Tank->Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Tank->Body->SetCanEverAffectNavigation(false);
        }
        else UE_LOG(LogTemp,Error,TEXT("WF_V8221_WRECK_BODY_CONTRACT_MISSING %s"),*Key);
    }
    TArray<UStaticMeshComponent*> Parts;
    if(Tank->Body)Parts.Add(Tank->Body);
    if(Tank->TrackLinks)Parts.Add(Tank->TrackLinks);
    for(const auto* Group:{&Tank->Guns,&Tank->Cannons,&Tank->RoadWheelsV8218,&Tank->RoadTurretsV8218,&Tank->RailWheelsV33,&Tank->RailRodsV33,&Tank->RailBogieWheelsV837,&Tank->RailDoorsV837,&Tank->RailCupolasV837,&Tank->MarkingsV37})
        for(const auto& Component:*Group)if(Component)Parts.AddUnique(Component.Get());
    int32 Changed=0;
    for(UStaticMeshComponent* Component:Parts)for(int32 Slot=0;Slot<Component->GetNumMaterials();++Slot)
    {
        UMaterialInterface* Previous=Component->GetMaterial(Slot);
        const FString Key=SurfaceKey(Previous);if(Key.IsEmpty())continue;
        const FString Name=TEXT("M_Wreck_")+Key+TEXT("_V836");
        auto* Cached=Surfaces().Find(Key);
        FString Model;
        if(Component==Tank->Body&&Component->GetStaticMesh()&&Component->GetStaticMesh()->GetName().EndsWith(TEXT("_Wreck_V838")))Model=UFrontlineVehicleSubsystem::Definition(Tank->Type).AssetKey;
        else if(Component->GetStaticMesh()&&Component->GetStaticMesh()->GetName()==TEXT("SM_AustinWheel_V838"))Model=TEXT("AustinWheel");
        else if(Component->GetStaticMesh()&&Component->GetStaticMesh()->GetName()==TEXT("SM_AustinTurret_V838"))Model=TEXT("AustinTurret");
        auto* Parent=FrontlineVehicleArtV836::ResolveWreckMaterialV838(Model,Key);
        if(!Parent)Parent=Cached?Cached->Get():nullptr;
        if(!Parent){Warm();Cached=Surfaces().Find(Key);Parent=Cached?Cached->Get():nullptr;}
        if(!Parent){UE_LOG(LogTemp,Error,TEXT("WF_V8220_WRECK_MATERIAL_MISSING %s"),*Name);continue;}
        auto* Material=UMaterialInstanceDynamic::Create(Parent,Tank);if(!Material)continue;
        if(auto* Dynamic=Cast<UMaterialInstanceDynamic>(Previous))
        {
            // Preserve individual service dust, rust, camouflage and deployment
            // seed without walking or rebuilding the source shader graph.
            Material->CopyInterpParameters(Dynamic);
            for(const FName Parameter:{FName(TEXT("FieldDust")),FName(TEXT("FieldRust")),FName(TEXT("CamouflageStrength")),FName(TEXT("RecognitionPaint")),FName(TEXT("RecognitionType"))})
            {float Value=0;if(Dynamic->GetScalarParameterValue(FMaterialParameterInfo(Parameter),Value))Material->SetScalarParameterValue(Parameter,Value);}
            const int32 Index=Tank->BodyMaterials.IndexOfByKey(Dynamic);
            if(Index!=INDEX_NONE)Tank->BodyMaterials.RemoveAtSwap(Index);
        }
        // The accepted partial-damage blackening must not flatten the texture
        // before the richer char/oxide/ash layer reads it. This derivative is
        // no longer fed by the live-vehicle VehicleDamage update list.
        Material->SetScalarParameterValue(TEXT("VehicleDamage"),0.f);
        Material->SetScalarParameterValue(TEXT("WreckStateV8220"),1.f);
        FrontlineVehicleArtV836::HoldMaterialDetailV837(Material);
        Material->SetScalarParameterValue(TEXT("WreckHalfLengthV8220"),Length);
        Material->SetScalarParameterValue(TEXT("WreckRoofV8220"),Height);
        Material->SetScalarParameterValue(TEXT("WreckPatternSeedV8220"),float((Tank->Serial*37+Tank->Team*1917)%997));
        Material->SetVectorParameterValue(TEXT("WreckOriginV8220"),VectorParameter(Pose.GetLocation()));
        Material->SetVectorParameterValue(TEXT("WreckForwardV8220"),VectorParameter(Pose.GetUnitAxis(EAxis::X)));
        Material->SetVectorParameterValue(TEXT("WreckRightV8220"),VectorParameter(Pose.GetUnitAxis(EAxis::Y)));
        Material->SetVectorParameterValue(TEXT("WreckUpV8220"),VectorParameter(Pose.GetUnitAxis(EAxis::Z)));
        Component->SetMaterial(Slot,Material);++Changed;
    }
    // Weapons sag independently; fixed sponsons stay recognizable. No new
    // turret, fresh recognition decals or collision shape is introduced.
    FRandomStream Random(822019+Tank->Serial*71+Tank->Team*1917);
    for(auto& Gun:Tank->Guns)if(Gun)
    {
        FRotator Angle=Gun->GetRelativeRotation();
        Angle.Pitch=-Random.FRandRange(31.f,55.f);Angle.Yaw+=Random.FRandRange(-15.f,15.f);Angle.Roll=Random.FRandRange(-17.f,17.f);
        Gun->SetRelativeRotation(Angle);
    }
    for(auto& Cannon:Tank->Cannons)if(Cannon)
    {
        FRotator Angle=Cannon->GetRelativeRotation();Angle.Pitch=-Random.FRandRange(28.f,42.f);Angle.Roll=Random.FRandRange(-13.f,13.f);
        Cannon->SetRelativeRotation(Angle);
        // A short, independently collapsed tube reads as a disabled weapon at
        // the tactical distance; the fixed armoured sponson is left intact.
        if(Random.FRand()<.5f){FVector Scale=Cannon->GetRelativeScale3D();Scale.X*=.80f;Cannon->SetRelativeScale3D(Scale);}
    }
    for(auto& Wheel:Tank->RoadWheelsV8218)if(Wheel)
    {
        FRotator Angle=Wheel->GetRelativeRotation();Angle.Yaw+=Random.FRandRange(-9.f,9.f);Angle.Roll+=Random.FRandRange(-11.f,11.f);
        Wheel->SetRelativeRotation(Angle);
    }
    // The long convoy retains its couplings/chassis, while separate wheels,
    // doors and gun shields settle independently after the roof collapse.
    for(auto& Wheel:Tank->RailBogieWheelsV837)if(Wheel)
    {
        FRotator Angle=Wheel->GetRelativeRotation();Angle.Pitch+=Random.FRandRange(-11.f,11.f);Angle.Roll+=Random.FRandRange(-6.f,6.f);
        Wheel->SetRelativeRotation(Angle);
    }
    for(auto& Door:Tank->RailDoorsV837)if(Door)
    {
        FRotator Angle=Door->GetRelativeRotation();Angle.Yaw+=Random.FRandRange(9.f,27.f);Angle.Roll+=Random.FRandRange(-8.f,8.f);
        Door->SetRelativeRotation(Angle);
    }
    for(auto& Cupola:Tank->RailCupolasV837)if(Cupola)
    {
        FRotator Angle=Cupola->GetRelativeRotation();Angle.Pitch-=Random.FRandRange(7.f,12.f);Angle.Roll+=Random.FRandRange(-9.f,9.f);
        Cupola->SetRelativeRotation(Angle);
    }
    // The fire ends, but bent loose steel keeps a different physical silhouette.
    // Real lit geometry rests on the hull/earth; no glowing card or collision blocker.
    auto* TornSteel=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V8221/Meshes/SM_ShellSplinter_V8221.SM_ShellSplinter_V8221"));
    auto* SteelCache=Surfaces().Find(TEXT("Steel"));
    if(TornSteel&&SteelCache&&SteelCache->IsValid())for(int32 I=0;I<8;++I)
    {
        auto* Piece=NewObject<UStaticMeshComponent>(Tank,*FString::Printf(TEXT("LooseArmourV8221_%d"),I));
        Piece->SetupAttachment(Tank->GetRootComponent());Piece->SetStaticMesh(TornSteel);
        Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);Piece->SetCanEverAffectNavigation(false);
        Piece->SetCastShadow(true);Piece->SetVisibleInRayTracing(true);Piece->SetReceivesDecals(true);
        auto* Burned=UMaterialInstanceDynamic::Create(SteelCache->Get(),Tank);
        Burned->SetScalarParameterValue(TEXT("VehicleDamage"),0);Burned->SetScalarParameterValue(TEXT("WreckStateV8220"),1);
        Burned->SetScalarParameterValue(TEXT("WreckHalfLengthV8220"),Length);Burned->SetScalarParameterValue(TEXT("WreckRoofV8220"),Height);
        Burned->SetScalarParameterValue(TEXT("WreckPatternSeedV8220"),float(Tank->Serial%997));
        Burned->SetVectorParameterValue(TEXT("WreckOriginV8220"),VectorParameter(Pose.GetLocation()));
        Burned->SetVectorParameterValue(TEXT("WreckForwardV8220"),VectorParameter(Pose.GetUnitAxis(EAxis::X)));
        Burned->SetVectorParameterValue(TEXT("WreckRightV8220"),VectorParameter(Pose.GetUnitAxis(EAxis::Y)));
        Burned->SetVectorParameterValue(TEXT("WreckUpV8220"),VectorParameter(Pose.GetUnitAxis(EAxis::Z)));
        Piece->SetMaterial(0,Burned);Piece->RegisterComponent();Tank->AddInstanceComponent(Piece);
        FVector At;
        if(I<3)At=Pose.TransformPosition(FVector(-Length*.28f+Random.FRandRange(-65,65),Random.FRandRange(-65,65),Height+Random.FRandRange(3,15)));
        else {At=Pose.TransformPosition(FVector(Random.FRandRange(-Length*.7f,Length*.8f),Random.FRandRange(205,380)*(I%2?1:-1),0));At.Z=AWarfareFrontlineGround::HeightAt(At.X,At.Y)+7;}
        const float Size=I<3?Random.FRandRange(3,5):Random.FRandRange(1.5f,3.6f);
        Piece->SetWorldTransform(FTransform(FRotator(Random.FRandRange(69,85),Random.FRandRange(-180,180),Random.FRandRange(-8,8)),At,FVector(Size*2.f,Size*.42f,Size)));
    }
    int32 ThrownLinks=0;
    // Repose an existing section of the near-side chain onto the actual earth.
    // The remaining side retains the chassis silhouette; no extra instances,
    // disconnected fake wheels, blockers or per-frame deformation are added.
    if(!Tank->bRailConvoyV32&&!Tank->IsWheeledV8218()&&Tank->TrackLinks)
    {
        const int32 Count=Tank->TrackLinks->GetInstanceCount()/2;
        const float Spacing=Count>0?Tank->PathLength/Count:0.f;
        if(Count>28&&Tank->TrackTransformsV33.Num()==Count*2&&Spacing>1.f)
        {
            const auto* Camera=UGameplayStatics::GetPlayerCameraManager(Tank->GetWorld(),0);
            const FVector View=Camera?Camera->GetCameraLocation():Pose.TransformPosition(FVector(0,-1200,400));
            const bool Positive=Pose.InverseTransformPosition(View).Y>=0;
            const int32 Offset=(Positive?1:0)*Count;const float Sign=Positive?1.f:-1.f;
            int32 Start=0;double Best=TNumericLimits<double>::Max();
            const FVector Desired(-Length*.65f,Sign*150.f,38.f);
            for(int32 Index=0;Index<Count;++Index)
            {
                const double Distance=FVector::DistSquared(Tank->TrackTransformsV33[Offset+Index].GetLocation(),Desired);
                if(Distance<Best){Best=Distance;Start=Index;}
            }
            const FVector Anchor=Tank->TrackTransformsV33[Offset+Start].GetLocation();
            const int32 Section=FMath::Min(18,Count/6);
            const FVector Heading=(Pose.GetUnitAxis(EAxis::X)*-.66f+Pose.GetUnitAxis(EAxis::Y)*Sign*.75f).GetSafeNormal2D();
            const FVector WorldAnchor=Pose.TransformPosition(Anchor);
            const FTransform ComponentPose=Tank->TrackLinks->GetComponentTransform();
            for(int32 Index=0;Index<Section;++Index)
            {
                const float T=float(Index)/FMath::Max(1,Section-1);
                FVector At=WorldAnchor+Heading*(Spacing*Index);
                const float Ground=AWarfareFrontlineGround::HeightAt(At.X,At.Y)+10.f;
                At.Z=FMath::Lerp(WorldAnchor.Z,Ground,FMath::SmoothStep(0.f,.28f,T));
                FVector Next=At+Heading*Spacing;Next.Z=AWarfareFrontlineGround::HeightAt(Next.X,Next.Y)+10.f;
                const FRotator Direction=(Next-At).Rotation();
                const int32 Instance=Offset+(Start+Index)%Count;
                const FVector Scale=Tank->TrackTransformsV33[Instance].GetScale3D();
                const FTransform World(Direction,At,Scale);
                Tank->TrackTransformsV33[Instance]=World.GetRelativeTransform(ComponentPose);++ThrownLinks;
            }
            Tank->TrackLinks->BatchUpdateInstancesTransforms(0,Tank->TrackTransformsV33,false,true,true);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V8220_WRECK_TRANSITION serial=%d materials=%d thrown_links=%d"),Tank->Serial,Changed,ThrownLinks);
}

TSharedRef<FJsonObject> FFrontlineArmourWreckV8220Access::Describe(const AFrontlineTank* Tank)
{
    auto Result=MakeShared<FJsonObject>();if(!Tank)return Result;
    Result->SetBoolField(TEXT("transition"),Tank->Tags.Contains(TransitionTag));
    Result->SetBoolField(TEXT("operational"),Tank->IsOperational());
    Result->SetBoolField(TEXT("actor_collision"),Tank->GetActorEnableCollision());
    const FString Key=UFrontlineVehicleSubsystem::Definition(Tank->Type).AssetKey;
    const UStaticMesh* BodyMesh=Tank->Body?Tank->Body->GetStaticMesh():nullptr;
    Result->SetStringField(TEXT("body_mesh"),BodyMesh?BodyMesh->GetPathName():TEXT(""));
    Result->SetStringField(TEXT("expected_wreck_body_mesh"),BodyPath(Key));
    Result->SetBoolField(TEXT("body_geometry_replaced"),BodyMesh&&BodyMesh->GetPathName()==BodyPath(Key));
    int32 Surfaces=0,Expected=0,BodyExpected=0,BodyCovered=0;bool Parameters=true,DamageZero=true,RemovedFromLive=true,CollisionDisabled=true;
    TArray<UStaticMeshComponent*> Parts;
    if(Tank->Body)Parts.Add(Tank->Body);
    if(Tank->TrackLinks)Parts.Add(Tank->TrackLinks);
    for(const auto* Group:{&Tank->Guns,&Tank->Cannons,&Tank->RoadWheelsV8218,&Tank->RoadTurretsV8218,&Tank->RailWheelsV33,&Tank->RailRodsV33,&Tank->RailBogieWheelsV837,&Tank->RailDoorsV837,&Tank->RailCupolasV837,&Tank->MarkingsV37})
        for(const auto& Component:*Group)if(Component)Parts.AddUnique(Component.Get());
    // Count only physical vehicle surfaces, excluding muzzle cards, legacy
    // smoke cards and the shared fuel service. Report missing hull slots too.
    for(const auto* Component:Parts)
    {
        CollisionDisabled&=Component->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
        for(int32 Slot=0;Slot<Component->GetNumMaterials();++Slot)
        {
            const auto* Surface=Component->GetMaterial(Slot);const bool IsBody=Component==Tank->Body;
            if(IsBody)++BodyExpected;
            if(SurfaceKey(Surface).IsEmpty())continue;
            ++Expected;
            if(const auto* Material=Cast<UMaterialInstanceDynamic>(Surface))
                if(Material->GetMaterial()->GetName().StartsWith(TEXT("M_Wreck_")))
                {
                    ++Surfaces;if(IsBody)++BodyCovered;float State=0,Damage=1;
                    Parameters&=Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WreckStateV8220")),State)&&State==1.f;
                    DamageZero&=Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VehicleDamage")),Damage)&&FMath::IsNearlyZero(Damage);
                    RemovedFromLive&=!Tank->BodyMaterials.ContainsByPredicate([&](const auto& Live){return Live.Get()==Material;});
                }
        }
    }
    int32 LooseSteel=0;bool LooseSafe=true;
    TInlineComponentArray<UStaticMeshComponent*> AllParts(Tank);
    for(auto* C:AllParts)if(C->GetName().StartsWith(TEXT("LooseArmourV8221_"))){++LooseSteel;LooseSafe&=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&C->CastShadow;}
    Result->SetNumberField(TEXT("loose_physical_steel_pieces"),LooseSteel);
    Result->SetBoolField(TEXT("loose_steel_nonblocking_and_shadowed"),LooseSafe&&LooseSteel==8);
    Result->SetNumberField(TEXT("bound_wreck_surfaces"),Surfaces);
    Result->SetNumberField(TEXT("expected_wreck_surfaces"),Expected);
    Result->SetBoolField(TEXT("surface_coverage_complete"),Expected>0&&Expected==Surfaces&&BodyExpected>0&&BodyExpected==BodyCovered);
    Result->SetNumberField(TEXT("body_surface_slots"),BodyExpected);
    Result->SetNumberField(TEXT("covered_body_surface_slots"),BodyCovered);
    Result->SetBoolField(TEXT("body_surface_complete"),BodyExpected>0&&BodyExpected==BodyCovered);
    Result->SetBoolField(TEXT("persistent_wreck_parameters"),Parameters&&Surfaces>0);
    Result->SetBoolField(TEXT("legacy_damage_zero"),DamageZero&&Surfaces>0);
    Result->SetBoolField(TEXT("removed_from_live_material_updates"),RemovedFromLive&&Surfaces>0);
    Result->SetBoolField(TEXT("physical_components_collision_disabled"),CollisionDisabled);
    int32 Moved=0,GroundContacts=0,InstanceReadbacks=0;float MaxGroundedClearance=0;
    bool ChainCacheMatchesInstances=true;
    if(Tank->TrackLinks&&!Tank->IsWheeledV8218()&&!Tank->bRailConvoyV32&&Tank->PathLength>0)
    {
        const int32 Count=Tank->TrackLinks->GetInstanceCount()/2;
        const bool Heavy=Tank->Type!=EFrontlineVehicle::Whippet&&Tank->Type!=EFrontlineVehicle::A7V&&Tank->Type!=EFrontlineVehicle::A7VGretchen;
        const float Y=Heavy?147.f:(Tank->Type==EFrontlineVehicle::A7V||Tank->Type==EFrontlineVehicle::A7VGretchen)?123.f:109.f;
        for(int32 Side=0;Side<2;++Side)for(int32 Index=0;Index<Count;++Index)
        {
            FTransform Actual;if(!Tank->TrackLinks->GetInstanceTransform(Side*Count+Index,Actual,false)){ChainCacheMatchesInstances=false;continue;}
            ++InstanceReadbacks;
            ChainCacheMatchesInstances&=Tank->TrackTransformsV33.IsValidIndex(Side*Count+Index)&&Actual.Equals(Tank->TrackTransformsV33[Side*Count+Index],.01f);
            float Distance=FMath::Fmod(float(Index)*Tank->PathLength/Count+Tank->TrackOffset,Tank->PathLength);int32 Segment=0;
            while(Segment<Tank->TrackSegments.Num()-1&&Distance>Tank->TrackSegments[Segment]){Distance-=Tank->TrackSegments[Segment];++Segment;}
            if(!Tank->TrackSegments.IsValidIndex(Segment)||Tank->TrackPath.IsEmpty()||Tank->TrackSegments[Segment]<=0)continue;
            const FVector2D Delta=Tank->TrackPath[(Segment+1)%Tank->TrackPath.Num()]-Tank->TrackPath[Segment];
            const FVector2D Original=Tank->TrackPath[Segment]+Delta*(Distance/Tank->TrackSegments[Segment]);
            if(Actual.GetLocation().Equals(FVector(Original.X,(Side?1.f:-1.f)*Y,Original.Y),.1f))continue;
            ++Moved;
            const FVector At=Tank->TrackLinks->GetComponentTransform().TransformPosition(Actual.GetLocation());
            const float Clearance=At.Z-AWarfareFrontlineGround::HeightAt(At.X,At.Y);
            if(Clearance>=6.f&&Clearance<=11.f){++GroundContacts;MaxGroundedClearance=FMath::Max(MaxGroundedClearance,Clearance);}
        }
    }
    Result->SetNumberField(TEXT("track_instance_count"),Tank->TrackLinks?Tank->TrackLinks->GetInstanceCount():0);
    Result->SetNumberField(TEXT("actual_track_instance_readbacks"),InstanceReadbacks);
    Result->SetNumberField(TEXT("actual_thrown_link_count"),Moved);
    Result->SetNumberField(TEXT("grounded_thrown_link_count"),GroundContacts);
    Result->SetNumberField(TEXT("grounded_thrown_link_max_clearance_cm"),MaxGroundedClearance);
    Result->SetBoolField(TEXT("track_cache_matches_actual_instances"),ChainCacheMatchesInstances);
    Result->SetBoolField(TEXT("thrown_chain_has_ground_contact"),Moved>=6&&GroundContacts>=Moved/2);
    return Result;
}
