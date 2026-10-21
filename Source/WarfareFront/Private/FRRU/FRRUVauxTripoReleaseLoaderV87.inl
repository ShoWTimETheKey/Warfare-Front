// Include after WorldRenderBinding. Constants are generated from actual
// eight-site runtime plus V3 cold-load receipts. Campaign entry is gated
// separately. Old private VauxParts comparisons never add these eight copies.
// Eight replacements only; old parts 2/8/9 (solids 17/23/24) remain unchanged.
#pragma once
#include "FRRUVauxTripoReleaseV87.generated.inl"
#include "FRRUVauxHQ27ReleaseV87.generated.inl"
namespace FRRUVauxTripoReleaseLoaderV87
{
namespace Data=FRRUVauxTripoReleaseV87;
inline constexpr const TCHAR* SceneId=TEXT("fr.vaux.scene.families.v1");
inline constexpr const TCHAR* Tag=TEXT("FRRU_V87VauxTripoRelease");
inline constexpr int32 RetainedParts[]={2,8,9};
static_assert(UE_ARRAY_COUNT(Data::Models)==3&&UE_ARRAY_COUNT(Data::Sites)==8,"Only the actual three-source/eight-site release is supported");
struct FModel {UStaticMesh* Mesh=nullptr;UMaterialInterface* Material=nullptr;FLinearColor SoilTint;float Wetness=0;};
struct FStamp
{
    TWeakObjectPtr<const AActor> Owner;TWeakObjectPtr<const USceneComponent> Parent;
    TWeakObjectPtr<const UStaticMeshComponent> Old;TWeakObjectPtr<const UStaticMesh> Mesh;
    TWeakObjectPtr<const UMaterialInstanceDynamic> MID;TWeakObjectPtr<const UMaterialInterface> Material;
    FTransform Pose,OwnerPose,ParentPose;int32 Site=INDEX_NONE;
};
struct FRetained {TWeakObjectPtr<const UStaticMeshComponent> Parts[UE_ARRAY_COUNT(RetainedParts)];};
inline TMap<TWeakObjectPtr<const UStaticMeshComponent>,FStamp> Stamps;
inline TMap<TWeakObjectPtr<const AActor>,FRetained> Retained;
inline bool Fail(FString& Error,const FString& Why){Error=TEXT("Vaux Tripo release: ")+Why;return false;}
// The public source snapshot omits the original Tripo-derived data package.
inline bool Enabled(const FString& Scene){return Data::SourceDataAvailable&&WarfareFRRUWorldRenderBindingV87::ProductionEnabled(Scene);}
inline int32 ComponentCount(const FString& Scene){return Enabled(Scene)?UE_ARRAY_COUNT(Data::Sites):0;}
inline FString OldPath(int32 Part)
{
    const FString Name=FString::Printf(TEXT("SM_WF_fr_vaux_ruins_%03d_V87_dressing"),Part);
    return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/")+Name+TEXT(".")+Name;
}
inline bool Cosmetic(const UStaticMeshComponent* C)
{
    if(!C||!C->IsRegistered()||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
        ||C->GetGenerateOverlapEvents()||C->CanEverAffectNavigation()||C->IsPhysicsStateCreated())return false;
    const auto* Body=C->GetBodyInstance();return !Body||!Body->IsValidBodyInstance();
}
inline bool OldValid(const UStaticMeshComponent* C,int32 Part,bool Hidden)
{
    return Cosmetic(C)&&C->GetFName()==FName(*FString::Printf(TEXT("WorldRenderV826_1_%03d"),Part))
        &&C->ComponentHasTag(TEXT("FRRU_V826WorldPart"))&&!C->ComponentHasTag(TEXT("FRRU_V82Collision"))
        &&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==OldPath(Part)
        &&C->GetRelativeTransform().Equals(FTransform::Identity,1.e-8)&&C->IsVisible()!=Hidden
        &&bool(C->bHiddenInGame)==Hidden&&(!Hidden||!C->CastShadow);
}
inline bool ValidateComponent(const FString& Scene,const UStaticMeshComponent* C)
{
    const auto* S=C?Stamps.Find(C):nullptr;
    if(!Enabled(Scene)||!S||!S->Owner.IsValid()||!S->Parent.IsValid()||!S->Mesh.IsValid()||!S->MID.IsValid()
        ||!S->Material.IsValid()||S->Site<0||S->Site>=UE_ARRAY_COUNT(Data::Sites)||!Cosmetic(C)||!C->IsVisible()||C->bHiddenInGame
        ||C->GetOwner()!=S->Owner.Get()||C->GetAttachParent()!=S->Parent.Get()||!C->ComponentHasTag(Tag)
        ||C->GetFName()!=FName(*FString::Printf(TEXT("VauxTripoReleaseSolid%d"),Data::Sites[S->Site].Solid))
        ||C->GetStaticMesh()!=S->Mesh.Get()||C->GetNumMaterials()!=1||C->GetMaterial(0)!=S->MID.Get()
        ||S->MID->Parent.Get()!=S->Material.Get()||C->CanEverAffectNavigation()
        ||!C->IsUsingAbsoluteLocation()||!C->IsUsingAbsoluteRotation()||!C->IsUsingAbsoluteScale()
        ||!C->GetComponentTransform().Equals(S->Pose,.0001)||!C->GetOwner()->GetActorTransform().Equals(S->OwnerPose,1.e-8)
        ||!S->Parent->GetComponentTransform().Equals(S->ParentPose,1.e-8)
        ||!OldValid(S->Old.Get(),Data::Sites[S->Site].OldPart,true)||S->Old->GetOwner()!=S->Owner.Get())return false;
    // Only eight immutable stamps and three retained parts are checked here.
    // No meshes, native floor witnesses, texture lists or files are scanned.
    if(S->Site==0)
    {
        const auto* R=Retained.Find(S->Owner);if(!R)return false;
        for(int32 I=0;I<UE_ARRAY_COUNT(RetainedParts);++I)
            if(!OldValid(R->Parts[I].Get(),RetainedParts[I],false)||R->Parts[I]->GetOwner()!=S->Owner.Get())return false;
    }
    return true;
}
inline bool LoadModel(int32 Index,FModel& Out,FString& Error,bool(*Ready)(UMaterialInterface*,FString&))
{
    const auto& D=Data::Models[Index];Out.Mesh=LoadObject<UStaticMesh>(nullptr,D.Mesh);Out.Material=LoadObject<UMaterialInterface>(nullptr,D.Material);
    const auto* Mesh=Out.Mesh;const auto* Body=Mesh?Mesh->GetBodySetup():nullptr;
    // GetNaniteSettings and MeshDescription are editor-only. These accessors
    // read the actual cooked render/Nanite resources in the installed UE 5.8.
    if(!Mesh||!Out.Material||Mesh->GetPathName()!=D.Mesh||Out.Material->GetPathName()!=D.Material
        ||!Mesh->GetRenderData()||!Mesh->HasValidRenderData()||!Mesh->HasValidNaniteData()
        ||Mesh->GetNumNaniteTriangles()!=D.NaniteTriangles||Mesh->GetNumTriangles(0)!=D.FallbackTriangles
        ||Mesh->GetNumVertices(0)<=0||Mesh->GetNumTexCoords(0)<=0||!Body||Body->AggGeom.GetElementCount()!=0
        ||Mesh->GetStaticMaterials().Num()!=1||Mesh->GetStaticMaterials()[0].MaterialSlotName.ToString()!=D.Slot
        ||Mesh->GetStaticMaterials()[0].MaterialInterface.Get()!=Out.Material||!Ready||!Ready(Out.Material,Error)
        ||!Mesh->GetBoundingBox().Min.Equals(D.LocalBounds.Min,.075)||!Mesh->GetBoundingBox().Max.Equals(D.LocalBounds.Max,.075))
        return Fail(Error,TEXT("actual cooked source mesh/material/render bounds differ"));
    if(!Out.Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Out.SoilTint)
        ||!Out.Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Out.Wetness))
        return Fail(Error,TEXT("actual imported PBR baseline parameters unavailable"));
    TArray<UTexture*> Used;Out.Material->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(GetCachedScalabilityCVars().MaterialQualityLevel),TOptional<EShaderPlatform>(GMaxRHIShaderPlatform));
    TMap<FString,UTexture2D*> Textures;
    for(UTexture* T:Used)if(auto* Texture=Cast<UTexture2D>(T))Textures.Add(Texture->GetPathName(),Texture);
    if(Textures.Num()!=D.TextureCount)return Fail(Error,TEXT("actual independent PBR resource count differs"));
    for(int32 I=0;I<D.TextureCount;++I)
    {
        const auto& P=D.Textures[I];auto** Found=Textures.Find(P.Asset);auto* T=Found?*Found:nullptr;
        // SRGB and CompressionSettings survive cooking. FlipGreenChannel is
        // editor-only; its imported interpretation is already source-pinned.
        // Compression is identity here, not a quality or resident-mip claim.
        if(!T||!T->GetResource()||T->GetPixelFormat()==PF_Unknown||T->GetSizeX()!=P.Width||T->GetSizeY()!=P.Height
            ||bool(T->SRGB)!=P.SRGB||uint8(T->CompressionSettings)!=P.Compression)
            return Fail(Error,TEXT("actual cooked paired texture path/dimensions/resource/settings differ"));
    }
    return true;
}
inline bool Stage(AWarfareFrontlineGameMode* B,AActor* Owner,USceneComponent* Root,const FString& Scene,FString& Error,
    bool(*Ready)(UMaterialInterface*,FString&),TSharedPtr<FJsonObject>* Evidence=nullptr)
{
    if(!Enabled(Scene))return true;
    for(auto I=Stamps.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
    for(auto I=Retained.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
    const auto* Native=wfexp::scene::ForMission("fr.vaux");
    // A combined dressing mesh cannot be hidden per building. Promotion must
    // select the formal split binding consistently before calling this helper.
    const auto* Binding=WarfareFRRUWorldRenderBindingV87::Find(Scene);
    if(!B||!Owner||!Root||Root->GetOwner()!=Owner||!WarfareFRRUCampaignV82::Active(B)
        ||WarfareFRRUCampaignV82::Nation(B,0)!=EFRRUNationV81::French||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.vaux")
        ||!Native||!Native->id||Scene!=UTF8_TO_TCHAR(Native->id)||Native->revision!=2||Native->solids.size()!=30||Native->routes.size()!=14
        ||Binding!=&WarfareFRRUWorldRenderBindingV87::VauxRuins||Binding->Solids!=12||Binding->Dressing!=18)
        return Fail(Error,TEXT("exact native Vaux authority/formal split render binding required"));
    const auto& Receiver28=Native->solids[28];
    if(!Receiver28.blocksMovement||!Receiver28.blocksSight
        ||!FVector(Receiver28.minimum.x,Receiver28.minimum.y,Receiver28.minimum.z).Equals(FRRUVauxHQ27ReleaseV87::Native28Bounds.Min,.0001)
        ||!FVector(Receiver28.maximum.x,Receiver28.maximum.y,Receiver28.maximum.z).Equals(FRRUVauxHQ27ReleaseV87::Native28Bounds.Max,.0001))
        return Fail(Error,TEXT("exact coherent Native2 receiver28 differs; eight original site contracts remain unchanged"));
    TArray<UStaticMeshComponent*> Existing;Owner->GetComponents(Existing);TMap<int32,UStaticMeshComponent*> Old;
    for(auto* C:Existing)
    {
        if(C&&C->ComponentHasTag(Tag))return Fail(Error,TEXT("duplicate staging refused"));
        for(const auto& S:Data::Sites)if(OldValid(C,S.OldPart,false))Old.Add(S.OldPart,C);
        for(int32 Part:RetainedParts)if(OldValid(C,Part,false))Old.Add(Part,C);
    }
    if(Old.Num()!=UE_ARRAY_COUNT(Data::Sites)+UE_ARRAY_COUNT(RetainedParts))return Fail(Error,TEXT("eight replaceable and three retained independent old parts must exist"));
    FModel Models[UE_ARRAY_COUNT(Data::Models)];
    for(int32 I=0;I<UE_ARRAY_COUNT(Models);++I)if(!LoadModel(I,Models[I],Error,Ready))return false;
    TArray<TSharedPtr<FJsonValue>> Members;
    for(const auto& S:Data::Sites)
    {
        if(S.Solid<0||S.Solid>=int32(Native->solids.size())||S.Model<0||S.Model>=UE_ARRAY_COUNT(Models)
            ||S.Pose.ContainsNaN()||!S.Pose.GetRotation().IsNormalized()||S.Pose.GetScale3D().GetMin()<=0.)return Fail(Error,TEXT("generated rigid placement invalid"));
        const auto& N=Native->solids[S.Solid];const FVector Min(N.minimum.x,N.minimum.y,N.minimum.z),Max(N.maximum.x,N.maximum.y,N.maximum.z);
        const FBox& Bounds=S.PrevalidatedSourceWorldBounds;
        if(!N.blocksMovement||!N.blocksSight||!Min.Equals(S.NativeBounds.Min,.0001)||!Max.Equals(S.NativeBounds.Max,.0001)
            ||!Bounds.IsValid||Bounds.Min.X<Min.X-.075||Bounds.Min.Y<Min.Y-.075||Bounds.Max.X>Max.X+.075
            ||Bounds.Max.Y>Max.Y+.075||Bounds.Max.Z>Max.Z+.075||Bounds.GetSize().Z>Max.Z-Min.Z+.075)
            return Fail(Error,TEXT("native authority no longer matches the fully prevalidated source envelope"));
        const auto& M=Data::Models[S.Model];double MinimumGap=MAX_dbl,MaximumGap=-MAX_dbl;
        if(!M.Contacts||M.ContactCount<=0)return Fail(Error,TEXT("generated actual source contacts absent"));
        for(int32 I=0;I<M.ContactCount;++I)
        {
            const FVector P=S.Pose.TransformPosition(M.Contacts[I]);float Floor=0;
            if(!WarfareFRRUWorldV82::TryHeight(B,float(P.X),float(P.Y),Floor))return Fail(Error,TEXT("real native contact unavailable"));
            const double Gap=P.Z-Floor;MinimumGap=FMath::Min(MinimumGap,Gap);MaximumGap=FMath::Max(MaximumGap,Gap);
            if(!FMath::IsFinite(Gap)||Gap< -6.075||Gap>.075)return Fail(Error,TEXT("real source base contact exceeds the accepted fit"));
        }
        if(!FMath::IsNearlyEqual(MinimumGap,S.ExpectedMinimumGap,.075)||!FMath::IsNearlyEqual(MaximumGap,S.ExpectedMaximumGap,.075))
            return Fail(Error,TEXT("real source contact range differs from accepted placement"));
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("nativeSolidIndex"),S.Solid);Row->SetStringField(TEXT("mesh"),M.Mesh);
        Row->SetStringField(TEXT("fitSHA256"),S.FitSHA256);Row->SetNumberField(TEXT("onceAtStageSourceContactCount"),M.ContactCount);
        Row->SetNumberField(TEXT("minimumBottomFloorGapCm"),MinimumGap);Row->SetNumberField(TEXT("maximumBottomFloorGapCm"),MaximumGap);
        Members.Add(MakeShared<FJsonValueObject>(Row));
    }
    TArray<UStaticMeshComponent*,TInlineAllocator<8>> Created;bool Hidden=false;bool OldShadows[UE_ARRAY_COUNT(Data::Sites)]={};
    const auto Cleanup=[&](const FString& Why)
    {
        for(auto* C:Created)if(C){Stamps.Remove(C);C->DestroyComponent();}
        Retained.Remove(Owner);
        if(Hidden)for(int32 I=0;I<UE_ARRAY_COUNT(Data::Sites);++I)
        {auto* C=Old.FindChecked(Data::Sites[I].OldPart);C->SetVisibility(true);C->SetHiddenInGame(false);C->SetCastShadow(OldShadows[I]);}
        return Fail(Error,Why);
    };
    for(int32 I=0;I<UE_ARRAY_COUNT(Data::Sites);++I)
    {
        const auto& S=Data::Sites[I];const auto& M=Models[S.Model];
        auto* C=NewObject<UStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("VauxTripoReleaseSolid%d"),S.Solid)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetAbsolute(true,true,true);
        C->SetStaticMesh(M.Mesh);C->SetWorldTransform(S.Pose);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->ComponentTags.Add(Tag);
        C->ComponentTags.Add(TEXT("FRRU_V87TripoRuin"));C->RegisterComponent();Created.Add(C);
        auto* MID=C->CreateDynamicMaterialInstance(0,M.Material);if(!MID)return Cleanup(TEXT("independent paired PBR instance failed"));
        MID->SetVectorParameterValue(TEXT("SoilTint"),M.SoilTint);MID->SetScalarParameterValue(TEXT("WeatherWetness"),M.Wetness);
        FLinearColor Tint;
        if(!Cosmetic(C)||!C->IsVisible()||!C->GetComponentTransform().Equals(S.Pose,.0001)
            ||!MID->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Tint)||!Tint.Equals(M.SoilTint,.000001f))
            return Cleanup(TEXT("actual source component pose/NoCollision/PBR baseline differs"));
        Stamps.Add(C,{Owner,Root,Old.FindChecked(S.OldPart),M.Mesh,MID,M.Material,S.Pose,Owner->GetActorTransform(),Root->GetComponentTransform(),I});
    }
    // All source resources, 3,958 contacts and eight new components passed.
    // Retain the original physical/native collection; hide only these visuals.
    for(int32 I=0;I<UE_ARRAY_COUNT(Data::Sites);++I)
    {auto* C=Old.FindChecked(Data::Sites[I].OldPart);OldShadows[I]=C->CastShadow;C->SetVisibility(false);C->SetHiddenInGame(true);C->SetCastShadow(false);}Hidden=true;
    FRetained R;for(int32 I=0;I<UE_ARRAY_COUNT(RetainedParts);++I)R.Parts[I]=Old.FindChecked(RetainedParts[I]);Retained.Add(Owner,R);
    for(const auto* C:Created)if(!ValidateComponent(Scene,C))return Cleanup(TEXT("actual immutable owner/component collection differs"));
    if(Evidence)
    {
        *Evidence=MakeShared<FJsonObject>();(*Evidence)->SetStringField(TEXT("schema"),TEXT("v87.vaux.release.eight-cosmetic.1"));
        (*Evidence)->SetNumberField(TEXT("actualModelComponents"),Created.Num());(*Evidence)->SetArrayField(TEXT("members"),Members);
        (*Evidence)->SetBoolField(TEXT("nativePhysicsOrLOSChanged"),false);(*Evidence)->SetNumberField(TEXT("originalUnqualifiedBuildingsRetained"),3);
        (*Evidence)->SetBoolField(TEXT("allElevenBuildingsUpgraded"),false);(*Evidence)->SetBoolField(TEXT("shippingAccepted"),false);
    }
    return true;
}
}
