// One reviewed Vaux solid27 visual replacement. No save or gameplay policy.
#pragma once
#include "FRRUVauxHQ27ReleaseV87.generated.inl"
namespace FRRUVauxHQ27ReleaseLoaderV87
{
namespace Data=FRRUVauxHQ27ReleaseV87;
constexpr const TCHAR* Tag=TEXT("FRRU_V87HQ27Release");
struct FStamp
{
    TWeakObjectPtr<const AWarfareFrontlineGameMode> Battle;FGuid Encounter;
    TWeakObjectPtr<const AActor> Owner,HQ;
    TWeakObjectPtr<const UStaticMeshComponent> Part,Visual;
    TWeakObjectPtr<const UStaticMesh> Remainder,Mesh;
    TWeakObjectPtr<const UMaterialInterface> Material;
    TWeakObjectPtr<const UMaterialInstanceDynamic> MID;
    FTransform OwnerPose;
    TWeakObjectPtr<const UStaticMeshComponent> Upper28,Base28;
    TWeakObjectPtr<const UStaticMesh> BaseMesh28;
    TWeakObjectPtr<const UMaterialInterface> Earth28,Timber28;
    TWeakObjectPtr<const UMaterialInstanceDynamic> UpperMID28,EarthMID28,TimberMID28;
};
inline TMap<TWeakObjectPtr<const AActor>,FStamp> Stamps;
// The public source snapshot omits the original Tripo-derived data package.
inline bool Enabled(const FString& Scene){return Data::SourceDataAvailable&&WarfareFRRUWorldRenderBindingV87::HQ27ProductionEnabled(Scene);}
inline bool Fail(FString& Error,const TCHAR* Why){Error=FString(TEXT("Vaux HQ27 release: "))+Why;return false;}
inline bool Cosmetic(const UStaticMeshComponent* C){return FRRUVauxTripoReleaseLoaderV87::Cosmetic(C);}
inline void Forget(const AActor* Owner){Stamps.Remove(Owner);}
inline bool Validate28(const FStamp& S)
{
    const auto* U=S.Upper28.Get();const auto* F=S.Base28.Get();
    return S.BaseMesh28.IsValid()&&S.Earth28.IsValid()&&S.Timber28.IsValid()&&Cosmetic(U)&&Cosmetic(F)&&U->GetOwner()==S.HQ.Get()&&F->GetOwner()==S.HQ.Get()
        &&U->ComponentHasTag(TEXT("FRRU_V87TripoRuin"))&&F->ComponentHasTag(TEXT("FRRU_V87TripoRuin"))
        &&U->GetStaticMesh()==S.Mesh.Get()&&F->GetStaticMesh()==S.BaseMesh28.Get()&&F->GetStaticMesh()->GetPathName()==Data::Base28Path
        &&U->GetNumMaterials()==1&&U->GetMaterial(0)==S.UpperMID28.Get()&&S.UpperMID28.IsValid()&&S.UpperMID28->Parent.Get()==S.Material.Get()
        &&F->GetNumMaterials()==2&&S.EarthMID28.IsValid()&&S.TimberMID28.IsValid()&&F->GetMaterial(0)==S.EarthMID28.Get()&&F->GetMaterial(1)==S.TimberMID28.Get()
        &&S.EarthMID28->Parent.Get()==S.Earth28.Get()&&S.TimberMID28->Parent.Get()==S.Timber28.Get()
        &&U->IsVisible()&&!U->bHiddenInGame&&U->CastShadow&&F->IsVisible()&&!F->bHiddenInGame&&F->CastShadow
        &&U->GetComponentTransform().Equals(Data::Pose28,.0001)&&F->GetComponentTransform().Equals(Data::Pose28,.0001)
        &&U->IsUsingAbsoluteLocation()&&U->IsUsingAbsoluteRotation()&&U->IsUsingAbsoluteScale()
        &&F->IsUsingAbsoluteLocation()&&F->IsUsingAbsoluteRotation()&&F->IsUsingAbsoluteScale();
}
inline bool Validate(const AActor* Owner,const FString& Scene)
{
    const auto* S=Stamps.Find(Owner);const auto* C=S?S->Visual.Get():nullptr;const auto* P=S?S->Part.Get():nullptr;
    return Enabled(Scene)&&S&&S->Battle.IsValid()&&S->Owner.Get()==Owner&&S->HQ.IsValid()&&S->Remainder.IsValid()&&S->Mesh.IsValid()
        &&S->Material.IsValid()&&S->MID.IsValid()&&S->Encounter==S->Battle->GetEncounterId()
        &&Cosmetic(P)&&P->GetOwner()==Owner&&P->GetFName()==FName(TEXT("WorldRenderV826_1_000"))
        &&P->GetStaticMesh()==S->Remainder.Get()&&P->GetStaticMesh()->GetPathName()==Data::RemainderPath&&P->IsVisible()&&!P->bHiddenInGame&&P->CastShadow
        &&P->GetRelativeTransform().Equals(FTransform::Identity,1.e-8)&&Owner->GetActorTransform().Equals(S->OwnerPose,1.e-8)
        &&Cosmetic(C)&&C->GetOwner()==S->HQ.Get()&&C->GetStaticMesh()==S->Mesh.Get()&&C->GetStaticMesh()->GetPathName()==Data::MeshPath
        &&C->GetNumMaterials()==1&&C->GetMaterial(0)==S->MID.Get()&&S->MID->Parent.Get()==S->Material.Get()
        &&C->IsVisible()&&!C->bHiddenInGame&&C->CastShadow&&C->GetComponentTransform().Equals(Data::Pose,.0001)
        &&C->IsUsingAbsoluteLocation()&&C->IsUsingAbsoluteRotation()&&C->IsUsingAbsoluteScale()&&Validate28(*S);
}
inline bool Stage(AWarfareFrontlineGameMode* B,AActor* Owner,const FString& Scene,AActor*& Out,FString& Error,
    bool(*Ready)(UMaterialInterface*,FString&),TSharedPtr<FJsonObject>* Evidence=nullptr)
{
    if(!Enabled(Scene))return true;
    for(auto It=Stamps.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    const auto* Native=wfexp::scene::ForMission("fr.vaux");
    if(!B||!Owner||Out||!Ready||!WarfareFRRUCampaignV82::Active(B)||WarfareFRRUCampaignV82::Nation(B)!=EFRRUNationV81::French
        ||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.vaux")||!Native||!Native->id||Scene!=UTF8_TO_TCHAR(Native->id)
        ||Native->revision!=2||Native->solids.size()!=30||Native->routes.size()!=14
        ||!Owner->GetActorTransform().Equals(FTransform(FQuat::Identity,FVector::ZeroVector,FVector(1,-1,1)),1.e-8))return Fail(Error,TEXT("exact original native Vaux owner/mission/revision required"));
    const auto& Solid=Native->solids[27];
    if(!Solid.blocksMovement||!Solid.blocksSight||!FVector(Solid.minimum.x,Solid.minimum.y,Solid.minimum.z).Equals(Data::NativeBounds.Min,.0001)
        ||!FVector(Solid.maximum.x,Solid.maximum.y,Solid.maximum.z).Equals(Data::NativeBounds.Max,.0001))return Fail(Error,TEXT("original solid27 authority differs"));
    UStaticMeshComponent *Part=nullptr,*Ground=nullptr,*Solids=nullptr;int32 Parts=0,Grounds=0,SolidBodies=0;TArray<UStaticMeshComponent*> Components;Owner->GetComponents(Components);
    for(auto* C:Components)if(C)
    {if(C->GetFName()==FName(TEXT("WorldRenderV826_1_000"))){Part=C;++Parts;}
        if(C->ComponentHasTag(TEXT("ground"))&&C->ComponentHasTag(TEXT("FRRU_V82Collision"))){Ground=C;++Grounds;}
        if(C->ComponentHasTag(TEXT("solids"))&&C->ComponentHasTag(TEXT("FRRU_V82Collision"))){Solids=C;++SolidBodies;}}
    if(Parts!=1||Grounds!=1||SolidBodies!=1||!Cosmetic(Part)||!Part->GetStaticMesh()||Part->GetStaticMesh()->GetPathName()!=Data::RemainderPath
        ||Part->GetStaticMesh()->GetNumTriangles(0)!=255765||Part->GetStaticMesh()->GetStaticMaterials().Num()!=7
        ||!Part->GetRelativeTransform().Equals(FTransform::Identity,1.e-8)||!Ground->IsRegistered()||!Solids->IsRegistered()
        ||!Ground->GetStaticMesh()||Ground->GetStaticMesh()->GetPathName()!=Data::GroundPath||!Solids->GetStaticMesh()||Solids->GetStaticMesh()->GetPathName()!=Data::SolidsPath
        ||Ground->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics||Solids->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics
        ||!Ground->IsPhysicsStateCreated()||!Solids->IsPhysicsStateCreated())return Fail(Error,TEXT("actual R2/unique unchanged native collision collection differs"));
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,Data::MeshPath);auto* Material=LoadObject<UMaterialInterface>(nullptr,Data::MaterialPath);
    const auto* Body=Mesh?Mesh->GetBodySetup():nullptr;
    if(!Mesh||!Material||!Mesh->HasValidRenderData()||!Mesh->HasValidNaniteData()||Mesh->GetNumNaniteTriangles()!=1885492
        ||Mesh->GetNumTriangles(0)!=282822||Mesh->GetNumVertices(0)<=0||!Body||Body->AggGeom.GetElementCount()!=0||Mesh->GetStaticMaterials().Num()!=1
        ||Mesh->GetStaticMaterials()[0].MaterialSlotName.ToString()!=Data::Slot||Mesh->GetStaticMaterials()[0].MaterialInterface.Get()!=Material
        ||!Mesh->GetBoundingBox().Min.Equals(Data::LocalBounds.Min,.075)||!Mesh->GetBoundingBox().Max.Equals(Data::LocalBounds.Max,.075)||!Ready(Material,Error))return Fail(Error,TEXT("actual cooked HQ render/material/bounds differ"));
    FLinearColor Tint;float Wetness=0;if(!Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Tint)
        ||!Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Wetness)||!FMath::IsFinite(Wetness))return Fail(Error,TEXT("paired authored weather parameters missing"));
    TArray<UTexture*> Used;Material->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(GetCachedScalabilityCVars().MaterialQualityLevel),TOptional<EShaderPlatform>(GMaxRHIShaderPlatform));TMap<FString,UTexture2D*> Textures;
    for(auto* T:Used)if(auto* T2=Cast<UTexture2D>(T))Textures.Add(T2->GetPathName(),T2);
    if(Textures.Num()!=3)return Fail(Error,TEXT("exact three formal PBR resources required"));
    for(const auto& Expected:Data::Textures)
    {auto** Found=Textures.Find(Expected.Asset);auto* T=Found?*Found:nullptr;if(!T||!T->GetResource()||T->GetPixelFormat()==PF_Unknown||T->GetSizeX()!=4096||T->GetSizeY()!=4096
        ||bool(T->SRGB)!=Expected.SRGB||uint8(T->CompressionSettings)!=Expected.Compression)return Fail(Error,TEXT("actual formal texture identity/settings/resources differ"));}
    // Full contacts are a bounded stage check, never a readiness-frame scan.
    // Preserve the reviewed fixed pose and its measured small base gap; do not
    // move the model or change any native floor/capsule tolerance to hide it.
    double Low=MAX_dbl,High=-MAX_dbl;
    for(const FVector& Contact:Data::Contacts)
    {const FVector P=Data::Pose.TransformPosition(Contact);double Floor=0;if(!wfexp::scene::HeightAt(*Native,P.X,P.Y,Floor))return Fail(Error,TEXT("native source contact unavailable"));const double Gap=P.Z-Floor;
        if(!FMath::IsFinite(Gap))return Fail(Error,TEXT("nonfinite actual source contact"));Low=FMath::Min(Low,Gap);High=FMath::Max(High,Gap);}
    if(!FMath::IsNearlyEqual(Low,Data::MinimumGap,.075)||!FMath::IsNearlyEqual(High,Data::MaximumGap,.075))return Fail(Error,TEXT("reviewed source/native contact envelope changed"));
    double MaximumRayError=0;
    for(double X:{7150.,7600.,8050.})for(double Y:{-4830.,-4500.,-4170.})
    {double Floor=0;FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV87HQ27ReleaseGround),true);
        if(!wfexp::scene::HeightAt(*Native,X,Y,Floor)||!Ground->LineTraceComponent(Hit,FVector(X,Y,Floor+1000),FVector(X,Y,Floor-1000),Q)
            ||Hit.bStartPenetrating||FMath::Abs(Hit.ImpactPoint.Z-Floor)>.5)return Fail(Error,TEXT("actual unchanged ground ray differs from native floor"));MaximumRayError=FMath::Max(MaximumRayError,FMath::Abs(Hit.ImpactPoint.Z-Floor));}
    FHitResult Roof;FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV87HQ27ReleaseRoof),false);
    if(!Solids->LineTraceComponent(Roof,FVector(7600,-4500,700),FVector(7600,-4500,-1000),Q)||FMath::Abs(Roof.ImpactPoint.Z-450.)>.5)return Fail(Error,TEXT("actual original solid27 roof collision differs"));
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Out=B->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Params);if(!Out)return Fail(Error,TEXT("cosmetic owner allocation failed"));
    auto* C=NewObject<UStaticMeshComponent>(Out,TEXT("VauxHQ27Release"),RF_Transient);Out->AddInstanceComponent(C);Out->SetRootComponent(C);Out->Tags.Add(Tag);
    C->SetMobility(EComponentMobility::Movable);C->SetAbsolute(true,true,true);C->SetStaticMesh(Mesh);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->ComponentTags.Add(Tag);C->ComponentTags.Add(TEXT("FRRU_V87TripoRuin"));C->RegisterComponent();Out->SetActorTransform(Data::Pose,false,nullptr,ETeleportType::TeleportPhysics);
    auto* MID=C->CreateDynamicMaterialInstance(0,Material);if(!MID){Out->Destroy();Out=nullptr;return Fail(Error,TEXT("independent paired material instance failed"));}
    MID->SetVectorParameterValue(TEXT("SoilTint"),Tint);MID->SetScalarParameterValue(TEXT("WeatherWetness"),Wetness);
    // Receiver28: exact existing upper, independent closed B3 bearing base.
    // Checks run once at staging; cached readiness only reads immutable stamps.
    const auto Fail28=[&](const TCHAR* Why){Out->Destroy();Out=nullptr;return Fail(Error,Why);};
    const auto& Solid28=Native->solids[28];
    if(!Solid28.blocksMovement||!Solid28.blocksSight||!FVector(Solid28.minimum.x,Solid28.minimum.y,Solid28.minimum.z).Equals(Data::Native28Bounds.Min,.0001)
        ||!FVector(Solid28.maximum.x,Solid28.maximum.y,Solid28.maximum.z).Equals(Data::Native28Bounds.Max,.0001))return Fail28(TEXT("exact paired native28 bounds differ"));
    auto* BaseMesh28=LoadObject<UStaticMesh>(nullptr,Data::Base28Path);const auto* BaseBody28=BaseMesh28?BaseMesh28->GetBodySetup():nullptr;
    if(!BaseMesh28||!BaseMesh28->HasValidRenderData()||BaseMesh28->GetNumTriangles(0)!=252||!BaseBody28||BaseBody28->AggGeom.GetElementCount()!=0
        ||BaseMesh28->GetStaticMaterials().Num()!=2||!BaseMesh28->GetBoundingBox().Min.Equals(Data::Base28LocalBounds.Min,.001)
        ||!BaseMesh28->GetBoundingBox().Max.Equals(Data::Base28LocalBounds.Max,.001))return Fail28(TEXT("actual B3 base geometry/bounds/collision differ"));
    UMaterialInterface* BaseMaterials28[2]={};
    for(int32 I=0;I<2;++I)
    {BaseMaterials28[I]=LoadObject<UMaterialInterface>(nullptr,Data::Base28Materials[I]);
        if(!BaseMaterials28[I]||BaseMesh28->GetStaticMaterials()[I].MaterialSlotName.ToString()!=Data::Base28Slots[I]
            ||BaseMesh28->GetStaticMaterials()[I].MaterialInterface.Get()!=BaseMaterials28[I]||!Ready(BaseMaterials28[I],Error))return Fail28(TEXT("exact paired B3 materials/resources differ"));}
    double Bearing28=MAX_dbl;
    for(const FVector& Contact:Data::Contacts)
    {for(int32 I=0;I<UE_ARRAY_COUNT(Data::Base28Footprint);++I)
        {const FVector A=Data::Base28Footprint[I],Z=Data::Base28Footprint[(I+1)%UE_ARRAY_COUNT(Data::Base28Footprint)];
            if((Z.X-A.X)*(Contact.Y-A.Y)-(Z.Y-A.Y)*(Contact.X-A.X)<-.075)return Fail28(TEXT("actual upper witness lies outside closed base footprint"));}
        Bearing28=FMath::Min(Bearing28,Data::Base28Top-Contact.Z);}
    if(!FMath::IsFinite(Bearing28)||Bearing28<0.)return Fail28(TEXT("upper and closed bearing base do not meet"));
    int32 GroundSamples28=0,GroundInsideBase28=0;
    for(const FVector& Local:Data::Base28Footprint)
    {const FVector P=Data::Pose28.TransformPosition(Local);double Floor=0;FHitResult G;FCollisionQueryParams GroundQ(SCENE_QUERY_STAT(FRRUV87Foundation28Ground),true);
        if(!wfexp::scene::HeightAt(*Native,P.X,P.Y,Floor)||Floor<P.Z-.5
            ||!Ground->LineTraceComponent(G,FVector(P.X,P.Y,Floor+1000),FVector(P.X,P.Y,Floor-1000),GroundQ)||G.bStartPenetrating
            ||FMath::Abs(G.ImpactPoint.Z-Floor)>.5)return Fail28(TEXT("actual base/native ground contact differs"));
        ++GroundSamples28;if(Floor<=Data::Pose28.GetLocation().Z+Data::Base28Top)++GroundInsideBase28;}
    if(GroundInsideBase28==0)return Fail28(TEXT("closed base has no sampled intersection with real ground"));
    FHitResult Roof28;FCollisionQueryParams RoofQ28(SCENE_QUERY_STAT(FRRUV87Foundation28Roof),false);const FVector Centre28=Data::Native28Bounds.GetCenter();
    if(!Solids->LineTraceComponent(Roof28,FVector(Centre28.X,Centre28.Y,Data::Native28Bounds.Max.Z+1000),FVector(Centre28.X,Centre28.Y,Data::Native28Bounds.Min.Z-1000),RoofQ28)
        ||Roof28.bStartPenetrating||FMath::Abs(Roof28.ImpactPoint.Z-Data::Native28Bounds.Max.Z)>.5)return Fail28(TEXT("actual paired solid28 roof differs"));
    auto* Upper28=NewObject<UStaticMeshComponent>(Out,TEXT("VauxHQ28Upper"),RF_Transient);auto* Base28=NewObject<UStaticMeshComponent>(Out,TEXT("VauxHQ28ClosedBase"),RF_Transient);
    for(auto* V:{Upper28,Base28})
    {Out->AddInstanceComponent(V);V->SetupAttachment(Out->GetRootComponent());V->SetMobility(EComponentMobility::Movable);V->SetAbsolute(true,true,true);
        V->SetStaticMesh(V==Upper28?Mesh:BaseMesh28);V->SetCollisionProfileName(TEXT("NoCollision"));V->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        V->SetGenerateOverlapEvents(false);V->SetCanEverAffectNavigation(false);V->SetCastShadow(true);V->ComponentTags.Add(Tag);V->ComponentTags.Add(TEXT("FRRU_V87TripoRuin"));V->RegisterComponent();V->SetWorldTransform(Data::Pose28);}
    auto* UpperMID28=Upper28->CreateDynamicMaterialInstance(0,Material);UMaterialInstanceDynamic* BaseMIDs28[2]={};
    if(!UpperMID28)return Fail28(TEXT("upper28 paired MID failed"));UpperMID28->SetVectorParameterValue(TEXT("SoilTint"),Tint);UpperMID28->SetScalarParameterValue(TEXT("WeatherWetness"),Wetness);
    for(int32 I=0;I<2;++I)
    {FLinearColor BaseTint;float BaseWet=0;
        BaseMIDs28[I]=Base28->CreateDynamicMaterialInstance(I,BaseMaterials28[I]);if(!BaseMIDs28[I])return Fail28(TEXT("base paired MID failed"));
        // Original Earth/Timber modules are static paired PBR, not weather-
        // parameter assets. Preserve their graph; copy supported parameters
        // only, exactly as the shared scene weather binder does for modules.
        if(BaseMaterials28[I]->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),BaseTint))BaseMIDs28[I]->SetVectorParameterValue(TEXT("SoilTint"),BaseTint);
        if(BaseMaterials28[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),BaseWet))
        {if(!FMath::IsFinite(BaseWet))return Fail28(TEXT("nonfinite supported base weather value"));BaseMIDs28[I]->SetScalarParameterValue(TEXT("WeatherWetness"),BaseWet);}}
    Stamps.Add(Owner,{B,B->GetEncounterId(),Owner,Out,Part,C,Part->GetStaticMesh(),Mesh,Material,MID,Owner->GetActorTransform()});
    auto& Stamp28=Stamps.FindChecked(Owner);Stamp28.Upper28=Upper28;Stamp28.Base28=Base28;Stamp28.BaseMesh28=BaseMesh28;
    Stamp28.Earth28=BaseMaterials28[0];Stamp28.Timber28=BaseMaterials28[1];Stamp28.UpperMID28=UpperMID28;Stamp28.EarthMID28=BaseMIDs28[0];Stamp28.TimberMID28=BaseMIDs28[1];
    if(!Validate(Owner,Scene)){Stamps.Remove(Owner);Out->Destroy();Out=nullptr;return Fail(Error,TEXT("actual immutable HQ27 component stamp differs"));}
#if WITH_EDITOR
    if(Evidence)
    {*Evidence=MakeShared<FJsonObject>();auto& J=*Evidence;J->SetStringField(TEXT("schema"),TEXT("v87.vaux.hq27.single-visual-release.1"));J->SetStringField(TEXT("mesh"),Data::MeshPath);J->SetStringField(TEXT("remainder"),Data::RemainderPath);J->SetStringField(TEXT("actualWorldPose"),C->GetComponentTransform().ToHumanReadableString());
        J->SetNumberField(TEXT("onceAtStageNativeContactCount"),UE_ARRAY_COUNT(Data::Contacts));J->SetNumberField(TEXT("onceAtStageGroundRayCount"),9);J->SetNumberField(TEXT("minimumVisibleBaseNativeGapCm"),Low);J->SetNumberField(TEXT("maximumVisibleBaseNativeGapCm"),High);J->SetNumberField(TEXT("maximumActualGroundRayNativeErrorCm"),MaximumRayError);J->SetNumberField(TEXT("originalSolid27RoofZ"),Roof.ImpactPoint.Z);
        J->SetBoolField(TEXT("onlySolid27VisualPromoted"),true);J->SetBoolField(TEXT("nativePhysicsChanged"),true);J->SetBoolField(TEXT("trees28and29Changed"),true);J->SetBoolField(TEXT("onlyNative28PhysicsChanged"),true);J->SetBoolField(TEXT("original27TreesAnd29Changed"),false);J->SetBoolField(TEXT("foundation28Fixed"),false);J->SetBoolField(TEXT("foundation28CoherentCandidateStaged"),true);J->SetNumberField(TEXT("foundation28RoofZ"),Roof28.ImpactPoint.Z);J->SetNumberField(TEXT("foundation28GroundRayCount"),GroundSamples28);J->SetNumberField(TEXT("foundation28MinimumBearingOverlapCm"),Bearing28);J->SetStringField(TEXT("foundation28Acceptance"),TEXT("pending actual combined contact, camera and route review"));J->SetBoolField(TEXT("wholeSceneArtAccepted"),false);J->SetBoolField(TEXT("shippingAccepted"),false);}
#else
    (void)Evidence;(void)MaximumRayError;
#endif
    return true;
}
}
