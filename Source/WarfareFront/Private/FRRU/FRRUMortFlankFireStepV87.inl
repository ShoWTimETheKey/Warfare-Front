// One low cosmetic Mort flank position. Reuse the accepted formal02 source,
// public nine-foot/top descriptor and original PBR; never build a substitute.
// Include with the other Content helper inls, before its anonymous namespace.
#pragma once
#include "WarfareFrontlineSandbagDescriptorV87.h"
namespace FRRUMortFlankFireStepV87
{
inline constexpr const TCHAR* SceneId=TEXT("fr.mort_homme.scene.redesign.v1");
inline constexpr const TCHAR* MeshPath=TEXT("/Game/Warfare/Field/V87/Props/Meshes/SM_Sandbag_A_V87.SM_Sandbag_A_V87");
inline constexpr const TCHAR* MaterialPath=TEXT("/Game/Warfare/Field/V87/Props/Materials/M_TripoHessian_V87.M_TripoHessian_V87");
inline constexpr const TCHAR* Tag=TEXT("FRRU_V87MortFlankFireStep");
struct FPlacement {double X,Y,Yaw;};
inline constexpr FPlacement Placements[]={{1900.,-620.,87.},{1896.,-510.,92.},{1904.,-400.,89.}};
struct FSeat {FTransform Pose;TSharedPtr<FJsonObject> Evidence;};
struct FStamp
{
    TWeakObjectPtr<const AActor> Owner;TWeakObjectPtr<const USceneComponent> Parent;
    TWeakObjectPtr<const UStaticMesh> Mesh;FTransform Pose,OwnerTransform,ParentTransform;int32 Index=INDEX_NONE;
};
inline TMap<TWeakObjectPtr<const UStaticMeshComponent>,FStamp> Stamps;
inline int32 ComponentCount(const FString& Scene){return Scene==SceneId?UE_ARRAY_COUNT(Placements):0;}
inline bool Fail(FString& Error,const FString& Why){Error=TEXT("Mort flank fire step: ")+Why;return false;}
inline bool ValidateComponent(const FString& Scene,const UStaticMeshComponent* C)
{
    const auto* Stamp=C?Stamps.Find(C):nullptr;const auto* Owner=C?C->GetOwner():nullptr;
    if(Scene!=SceneId||!C||!Stamp||!Stamp->Owner.IsValid()||!Stamp->Parent.IsValid()||!Stamp->Mesh.IsValid()
        ||Stamp->Index<0||Stamp->Index>=UE_ARRAY_COUNT(Placements)||Owner!=Stamp->Owner.Get()||!C->ComponentHasTag(Tag)
        ||C->GetFName()!=FName(*FString::Printf(TEXT("MortFlankBagV87_%d"),Stamp->Index))||C->GetAttachParent()!=Stamp->Parent.Get()
        ||!Owner->GetActorTransform().Equals(Stamp->OwnerTransform,1.e-8)
        ||!Stamp->Parent->GetComponentTransform().Equals(Stamp->ParentTransform,1.e-8)
        ||!C->IsUsingAbsoluteLocation()||!C->IsUsingAbsoluteRotation()||!C->IsUsingAbsoluteScale()
        ||!C->GetComponentTransform().Equals(Stamp->Pose,.0001)||!C->GetComponentScale().Equals(FVector::OneVector,.000001)
        ||!C->IsRegistered()||!C->IsVisible()||C->GetStaticMesh()!=Stamp->Mesh.Get()||C->GetStaticMesh()->GetPathName()!=MeshPath
        ||C->GetNumMaterials()!=1||!FrontlineSandbagDescriptorV87::IsMaterial(C->GetMaterial(0))
        ||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||C->GetGenerateOverlapEvents()||C->CanEverAffectNavigation()
        ||C->IsPhysicsStateCreated())return false;
    const auto* Body=C->GetBodyInstance();return !Body||!Body->IsValidBodyInstance();
}
inline bool Seat(AWarfareFrontlineGameMode* B,const FPlacement& P,const FVector* Feet,FSeat& Out,FString& Error)
{
    float H=0,X0=0,X1=0,Y0=0,Y1=0;
    if(!WarfareFRRUWorldV82::TryHeight(B,float(P.X),float(P.Y),H)
        ||!WarfareFRRUWorldV82::TryHeight(B,float(P.X-45),float(P.Y),X0)
        ||!WarfareFRRUWorldV82::TryHeight(B,float(P.X+45),float(P.Y),X1)
        ||!WarfareFRRUWorldV82::TryHeight(B,float(P.X),float(P.Y-45),Y0)
        ||!WarfareFRRUWorldV82::TryHeight(B,float(P.X),float(P.Y+45),Y1))return Fail(Error,TEXT("actual native floor missing"));
    const FVector Normal=FVector(-(X1-X0)/90.,-(Y1-Y0)/90.,1.).GetSafeNormal();
    const double Slope=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Normal.Z,-1.,1.)));
    if(!FMath::IsFinite(Slope)||Slope>2.5)return Fail(Error,TEXT("authored gentle bearing slope differs"));
    const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Normal)*FQuat(FVector::UpVector,FMath::DegreesToRadians(P.Yaw));
    double Z=MAX_dbl;FVector Rotated[9];float Floors[9];
    for(int32 I=0;I<9;++I)
    {
        Rotated[I]=Rotation.RotateVector(Feet[I]);
        if(!WarfareFRRUWorldV82::TryHeight(B,float(P.X+Rotated[I].X),float(P.Y+Rotated[I].Y),Floors[I]))
            return Fail(Error,TEXT("one of the nine actual descriptor feet has no floor"));
        Z=FMath::Min(Z,double(Floors[I])-Rotated[I].Z);
    }
    Z-=.05;Out.Pose=FTransform(Rotation,FVector(P.X,P.Y,Z),FVector::OneVector);
    double MinimumGap=MAX_dbl,MaximumGap=-MAX_dbl,MinimumTop=MAX_dbl,MaximumTop=-MAX_dbl;
    TArray<TSharedPtr<FJsonValue>> Witnesses;
    for(int32 I=0;I<9;++I)
    {
        const FVector World=Out.Pose.TransformPosition(Feet[I]);const double Gap=World.Z-Floors[I];float Top=0;
        if(!FrontlineSandbagDescriptorV87::TopHeight(0,Out.Pose,float(World.X),float(World.Y),Top)
            ||!FMath::IsFinite(Top)||Top<Floors[I]+5.f||Top>Floors[I]+28.f)
            return Fail(Error,TEXT("actual accepted LOD0 top is missing or no longer one low layer"));
        MinimumGap=FMath::Min(MinimumGap,Gap);MaximumGap=FMath::Max(MaximumGap,Gap);
        MinimumTop=FMath::Min(MinimumTop,double(Top-Floors[I]));MaximumTop=FMath::Max(MaximumTop,double(Top-Floors[I]));
        auto W=MakeShared<FJsonObject>();W->SetNumberField(TEXT("foot"),I);W->SetStringField(TEXT("actualWorld"),World.ToString());
        W->SetNumberField(TEXT("nativeFloorCm"),Floors[I]);W->SetNumberField(TEXT("gapCm"),Gap);W->SetNumberField(TEXT("actualDescriptorTopCm"),Top);
        Witnesses.Add(MakeShared<FJsonValueObject>(W));
    }
    // Seat only inside the real shallow cloth underside. No soil flattening,
    // stretched bag, skirt, platform or additional support surface is allowed.
    if(MinimumGap<-.75||MaximumGap>.001)return Fail(Error,TEXT("actual nine-foot fit floats or embeds more than 0.75cm"));
    float CentreTop=0;if(!FrontlineSandbagDescriptorV87::TopHeight(0,Out.Pose,float(P.X),float(P.Y),CentreTop))
        return Fail(Error,TEXT("actual centre top query failed"));
    Out.Evidence=MakeShared<FJsonObject>();Out.Evidence->SetStringField(TEXT("actualWorldTransform"),Out.Pose.ToHumanReadableString());
    Out.Evidence->SetNumberField(TEXT("slopeDegrees"),Slope);Out.Evidence->SetNumberField(TEXT("nativeCentreFloorCm"),H);
    Out.Evidence->SetNumberField(TEXT("minimumFootGapCm"),MinimumGap);Out.Evidence->SetNumberField(TEXT("maximumFootGapCm"),MaximumGap);
    Out.Evidence->SetNumberField(TEXT("minimumTopAboveFloorCm"),MinimumTop);Out.Evidence->SetNumberField(TEXT("maximumTopAboveFloorCm"),MaximumTop);
    Out.Evidence->SetNumberField(TEXT("actualCentreTopCm"),CentreTop);Out.Evidence->SetArrayField(TEXT("actualNineFootWitnesses"),Witnesses);
    return true;
}
inline bool Stage(AWarfareFrontlineGameMode* B,AActor* Owner,USceneComponent* Root,const FString& Scene,FString& Error,
    bool(*Ready)(UMaterialInterface*,FString&),TSharedPtr<FJsonObject>* Evidence=nullptr)
{
    if(Scene!=SceneId)return true;
    for(auto I=Stamps.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
    const auto* Native=wfexp::scene::ForMission("fr.mort_homme");const auto* Mission=wfexp::tactical::FindMission("fr.mort_homme");
    if(!B||!Owner||!Root||Root->GetOwner()!=Owner||!WarfareFRRUCampaignV82::Active(B)
        ||WarfareFRRUCampaignV82::Nation(B,0)!=EFRRUNationV81::French||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.mort_homme")
        ||!Native||!Native->id||Scene!=UTF8_TO_TCHAR(Native->id)||Native->revision!=4||!Mission||Mission->sceneId!="fr.mort_homme.scene.redesign.v1"
        ||Mission->sceneRevision!=4)return Fail(Error,TEXT("current Mort revision/national authority differs"));
    const wfexp::tactical::NodeDefinition* Bridge=nullptr;for(const auto& Node:Mission->nodes)if(Node.id=="bridge")Bridge=&Node;
    double NativeBridgeZ=0;
    if(!Bridge||!FMath::IsNearlyEqual(Bridge->position.x,1800.,.001)||!FMath::IsNearlyEqual(Bridge->position.y,-400.,.001)
        ||!FMath::IsNearlyEqual(Bridge->position.z,147.6,.001)||!wfexp::scene::HeightAt(*Native,1800.,-400.,NativeBridgeZ)
        ||!FMath::IsNearlyEqual(NativeBridgeZ,147.6,.001))return Fail(Error,TEXT("bridge is not the exact moved native flank station"));
    TArray<UStaticMeshComponent*> Existing;Owner->GetComponents(Existing);
    for(const auto* C:Existing)if(C&&C->ComponentHasTag(Tag))return Fail(Error,TEXT("duplicate cosmetic staging refused"));
    TArray<UStaticMesh*> Meshes;UMaterialInterface* Material=nullptr;
    if(!FrontlineSandbagDescriptorV87::TryLoad(Meshes,Material)||Meshes.Num()!=1||!Meshes[0]||Meshes[0]->GetPathName()!=MeshPath
        ||!Material||Material->GetPathName()!=MaterialPath||!FrontlineSandbagDescriptorV87::IsMaterial(Material)
        ||!Ready||!Ready(Material,Error))return Fail(Error,TEXT("actual formal02 public descriptor/PBR source unavailable"));
    const FVector* Feet=FrontlineSandbagDescriptorV87::Feet(0);if(!Feet)return Fail(Error,TEXT("actual accepted nine-foot descriptor missing"));
    FSeat Seats[3];for(int32 I=0;I<3;++I)if(!Seat(B,Placements[I],Feet,Seats[I],Error))return false;
    TArray<UStaticMeshComponent*> Created;const auto Cleanup=[&](const FString& Why)
    {for(auto* C:Created)if(C){Stamps.Remove(C);C->DestroyComponent();}return Fail(Error,Why);};
    TArray<TSharedPtr<FJsonValue>> Bags;
    for(int32 I=0;I<3;++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("MortFlankBagV87_%d"),I)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetAbsolute(true,true,true);
        C->SetStaticMesh(Meshes[0]);C->SetWorldTransform(Seats[I].Pose);C->SetCollisionProfileName(TEXT("NoCollision"));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
        C->SetCastShadow(true);C->SetReceivesDecals(true);C->ComponentTags.Add(Tag);C->RegisterComponent();Created.Add(C);
        auto* MID=C->CreateDynamicMaterialInstance(0,Material);const auto* Body=C->GetBodyInstance();
        if(!MID||!C->IsRegistered()||!C->IsVisible()||!C->GetComponentTransform().Equals(Seats[I].Pose,.0001)
            ||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||C->GetGenerateOverlapEvents()||C->IsPhysicsStateCreated()
            ||(Body&&Body->IsValidBodyInstance()))return Cleanup(TEXT("actual cosmetic transform/material/NoCollision registration differs"));
        Stamps.Add(C,{Owner,Root,Meshes[0],Seats[I].Pose,Owner->GetActorTransform(),Root->GetComponentTransform(),I});
        if(!ValidateComponent(Scene,C))return Cleanup(TEXT("actual stamped component/owner transform contract failed"));
        Seats[I].Evidence->SetBoolField(TEXT("actualNoCollisionNoOverlapNoPhysicsBody"),true);Bags.Add(MakeShared<FJsonValueObject>(Seats[I].Evidence));
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_MORT_FLANK_BAG index=%d mesh=%s x=%.3f y=%.3f z=%.6f slope=%.6f gap=[%.6f,%.6f] top=%.6f native_cover_bonus=0"),
            I,MeshPath,Placements[I].X,Placements[I].Y,Seats[I].Pose.GetLocation().Z,Seats[I].Evidence->GetNumberField(TEXT("slopeDegrees")),
            Seats[I].Evidence->GetNumberField(TEXT("minimumFootGapCm")),Seats[I].Evidence->GetNumberField(TEXT("maximumFootGapCm")),
            Seats[I].Evidence->GetNumberField(TEXT("actualCentreTopCm")));
    }
    if(Evidence)
    {
        *Evidence=MakeShared<FJsonObject>();(*Evidence)->SetStringField(TEXT("schema"),TEXT("v87.mort.actual.low.flank.fire.step.1"));
        (*Evidence)->SetStringField(TEXT("scene"),Scene);(*Evidence)->SetStringField(TEXT("node"),TEXT("bridge"));
        (*Evidence)->SetStringField(TEXT("sourceManifest"),TEXT("ArtSource/Tripo/Prepared/worn-sandbag-smartmesh-p1-9728/Scene/formal02/formal-manifest.json"));
        (*Evidence)->SetStringField(TEXT("actualMesh"),Meshes[0]->GetPathName());(*Evidence)->SetStringField(TEXT("actualMaterial"),Material->GetPathName());
        (*Evidence)->SetArrayField(TEXT("bags"),Bags);(*Evidence)->SetBoolField(TEXT("nativePhysicsOrLOSChanged"),false);
        (*Evidence)->SetBoolField(TEXT("coverBonusAdded"),false);(*Evidence)->SetBoolField(TEXT("platformOrTerrainChange"),false);
        (*Evidence)->SetBoolField(TEXT("artAccepted"),false);(*Evidence)->SetBoolField(TEXT("runtimeAccepted"),false);
    }
    return true;
}
}
