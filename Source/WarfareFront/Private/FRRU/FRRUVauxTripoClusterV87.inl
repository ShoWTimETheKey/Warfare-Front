// Eight Editor-only cosmetic replacements. Exact received assets and independent
// fits are pinned; native collision, LOS, terrain, routes and bays stay intact.
#if WITH_EDITOR
namespace VauxTripoClusterV87
{
struct FModel {const TCHAR* Stem;const TCHAR* ReceiptSHA;int32 TextureCount;};
static const FModel Models[]={
    {TEXT("Tripo_v87_vaux_farmhouse_h31_b81c0c32_ruin02"),TEXT("2e0b11ad8193571b3f479dcbb371b643c419d2aff9887efcf23f2fecd0512622"),3},
    {TEXT("Tripo_v87_vaux_workshop_h31_7c116104_object01"),TEXT("f5e4aaebbbbf4904e88805aef384c92edc0288981987516decaa6fe70370b0f5"),4},
    {TEXT("Tripo_v87_vaux_stable_h31_017d5f71_object01"),TEXT("12b151b23d25f8b756041617eb89ffe7ef6f1e4502726c9ab582a193a5b5432d"),4}};
struct FSite {int32 Solid,Model,OldPart;const TCHAR* Fit;const TCHAR* FitSHA;};
static const FSite Sites[]={
    {16,0,1,TEXT("fit-solid16-farmhouse-heightpin-v2.json"),TEXT("374628a1c0a074e3ee63d96dc9c282bdbbdd206b09801c19e2b8f2f91de4b67e")},
    {20,1,5,TEXT("fit-workshop20-object01-v87-heightpin-v2.json"),TEXT("4fe3a56902d1f524946716c1a101a63c823774eab107c6a7a83bed0bef4ea6b9")},
    {21,1,6,TEXT("fit-workshop21-object01-v87-heightpin-v2.json"),TEXT("8bfd17a19eb1617fd1785284df5381abda73287791bdbae8fbcbfbc0ca781fcb")},
    {22,0,7,TEXT("fit-solid22-farmhouse-180-heightpin-v2.json"),TEXT("401132a36a9045938e532932df2a5245d4f3c647a33bba95fea58db8e4c9c85a")},
    {18,2,3,TEXT("fit-stable18-object01-v87-heightpin-v2.json"),TEXT("620203750c94dca5fe58c917914af0c2b6bd3ded48efdce15a4b6df335493358")},
    {19,0,4,TEXT("fit-solid19-farmhouse-0-all11-v1.json"),TEXT("b8afd8c35afc1e2eaf61f4805a5daeda512a1fd993ea4d6b9e952dbbf31c6bcc")},
    {25,2,10,TEXT("fit-solid25-stable-180-all11-alt-v1.json"),TEXT("9e94f6841cf5ea8fc8e70facf5939064cbaef47701aff9572beb4c1e608c6cb7")},
    {26,0,11,TEXT("fit-solid26-farmhouse-180-all11-v1.json"),TEXT("cb14cb35f69d0018b6da2a45653b216a34052fbd4fd1b7532392e88da280e6a4")}};
bool VectorValue(const TSharedPtr<FJsonValue>& Value,FVector& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;double X=0,Y=0,Z=0;
    if(!Value||!Value->TryGetArray(A)||A->Num()!=3||!(*A)[0]->TryGetNumber(X)||!(*A)[1]->TryGetNumber(Y)||!(*A)[2]->TryGetNumber(Z))return false;
    Out=FVector(X,Y,Z);return !Out.ContainsNaN();
}
bool VectorField(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FVector& Out)
{const auto* V=J?J->Values.Find(Key):nullptr;return V&&VectorValue(*V,Out);}
bool LoadPinned(const FString& Relative,const FString& Expected,TArray<uint8>& Data,FString& Error)
{
    if(Relative.IsEmpty()||!FPaths::IsRelative(Relative)||Relative.Contains(TEXT(".."))||Relative.Contains(TEXT(":"))||Expected.Len()!=64)
        return Reject(Error,TEXT("Tripo cluster needs an exact project-relative source pin"));
    const FString Path=FPaths::ProjectDir()+Relative;const int64 Bytes=IFileManager::Get().FileSize(*Path);
    if(Bytes<=0||Bytes>192*1024*1024)return Reject(Error,TEXT("Tripo cluster pinned source absent/oversized: ")+Relative);
    const auto Stamp=IFileManager::Get().GetTimeStamp(*Path);
    if(!FFileHelper::LoadFileToArray(Data,*Path)||Data.Num()!=Bytes||IFileManager::Get().FileSize(*Path)!=Bytes
        ||IFileManager::Get().GetTimeStamp(*Path)!=Stamp)return Reject(Error,TEXT("Tripo cluster pinned source changed while reading: ")+Relative);
    FString SHA;
    if(!FRRUReadOnlySHA256V87(Data,SHA)||!SHA.Equals(Expected,ESearchCase::IgnoreCase))
        return Reject(Error,TEXT("Tripo cluster exact source SHA differs: ")+Relative);
    return true;
}
bool ReadPinned(const FString& Relative,const FString& SHA,TSharedPtr<FJsonObject>& Out,FString& Error)
{
    TArray<uint8> Bytes;if(!LoadPinned(Relative,SHA,Bytes,Error))return false;FString Text;
    FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());
    return (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out)&&Out.IsValid())
        ||Reject(Error,TEXT("Tripo cluster pinned JSON invalid: ")+Relative);
}
struct FActualModel
{
    UStaticMesh* Mesh=nullptr;UMaterialInterface* Material=nullptr;TSharedPtr<FJsonObject> Receipt,MeshRow;
    FString ReceiptPath;FLinearColor SoilTint;float Wetness=0;TArray<TSharedPtr<FJsonValue>> Textures;
};
bool Model(int32 Index,FActualModel& Out,FString& Error)
{
    const auto& Spec=Models[Index];const FString Stem=Spec.Stem,Prefix=TEXT("/Game/Warfare/AssetIntake/Tripo/")+Stem+TEXT("/import01/");
    Out.ReceiptPath=TEXT("Saved/AssetPipeline/Tripo/SceneImports/")+Stem+TEXT("/import01/scene-import.json");
    if(!ReadPinned(Out.ReceiptPath,Spec.ReceiptSHA,Out.Receipt,Error))return false;
    const auto& J=Out.Receipt;
    if(!J->GetBoolField(TEXT("success"))||!J->GetBoolField(TEXT("protected_unchanged"))||!J->GetBoolField(TEXT("source_pbr_bindings_preserved"))
        ||J->GetBoolField(TEXT("production_binding_changed"))||J->GetBoolField(TEXT("collision_changed"))||J->GetArrayField(TEXT("meshes")).Num()!=1
        ||J->GetArrayField(TEXT("materials")).Num()!=1||J->GetArrayField(TEXT("textures")).Num()!=Spec.TextureCount)
        return Reject(Error,TEXT("Tripo cluster actual import receipt contract differs"));
    Out.MeshRow=J->GetArrayField(TEXT("meshes"))[0]->AsObject();const auto M=J->GetArrayField(TEXT("materials"))[0]->AsObject();
    const FString MeshName=TEXT("SM_")+Stem,MaterialName=TEXT("M_")+Stem+TEXT("_0");
    const FString MeshPath=Prefix+TEXT("Meshes/")+MeshName+TEXT(".")+MeshName,MaterialPath=Prefix+TEXT("Materials/")+MaterialName+TEXT(".")+MaterialName;
    Out.Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath);Out.Material=LoadObject<UMaterialInterface>(nullptr,*MaterialPath);
    const auto& R=Out.MeshRow;auto* Mesh=Out.Mesh;const auto* Body=Mesh?Mesh->GetBodySetup():nullptr;
    if(R->GetStringField(TEXT("asset"))!=MeshPath||M->GetStringField(TEXT("asset"))!=MaterialPath||!Mesh||!Mesh->GetRenderData()||!Body
        ||Body->AggGeom.GetElementCount()!=0||Mesh->GetStaticMaterials().Num()!=1||Mesh->GetStaticMaterials()[0].MaterialInterface.Get()!=Out.Material
        ||Mesh->GetStaticMaterials()[0].MaterialSlotName.ToString()!=M->GetStringField(TEXT("source_slot"))||!ShaderReady(Out.Material,Error)
        ||!Mesh->GetNaniteSettings().bEnabled||!FMath::IsNearlyEqual(Mesh->GetNaniteSettings().KeepPercentTriangles,1.f)
        ||Mesh->GetNaniteSettings().TrimRelativeError!=0||Mesh->GetNumNaniteTriangles()!=R->GetIntegerField(TEXT("actual_nanite_triangles"))
        ||Mesh->GetNumTriangles(0)!=R->GetIntegerField(TEXT("actual_engine_fallback_triangles")))
        return Reject(Error,TEXT("Tripo cluster exact actual mesh/material/Nanite contract differs: ")+Stem);
    const auto& Bounds=R->GetArrayField(TEXT("actual_bounds_cm"));FVector Minimum=FVector::ZeroVector,Maximum=FVector::ZeroVector;
    if(Bounds.Num()!=2||!VectorValue(Bounds[0],Minimum)||!VectorValue(Bounds[1],Maximum)
        ||!Mesh->GetBoundingBox().Min.Equals(Minimum,.075)||!Mesh->GetBoundingBox().Max.Equals(Maximum,.075))
        return Reject(Error,TEXT("Tripo cluster actual centimetre mesh bounds differ"));
    if(!Out.Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Out.SoilTint)
        ||!Out.Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Out.Wetness))
        return Reject(Error,TEXT("Tripo cluster actual received-PBR dynamic parameters missing"));
    TArray<UTexture*> Used;Out.Material->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(GetCachedScalabilityCVars().MaterialQualityLevel),TOptional<EShaderPlatform>(GMaxRHIShaderPlatform));
    TMap<FString,UTexture2D*> Actual;for(UTexture* T:Used)if(auto* Texture=Cast<UTexture2D>(T))Actual.Add(Texture->GetPathName(),Texture);
    if(Actual.Num()!=Spec.TextureCount)return Reject(Error,TEXT("Tripo cluster actual PBR texture count differs"));
    for(const auto& Value:J->GetArrayField(TEXT("textures")))
    {
        const auto Row=Value->AsObject();const FString Path=Row->GetStringField(TEXT("asset"));auto** Found=Actual.Find(Path);
        if(!Found||!Path.StartsWith(Prefix+TEXT("Textures/")))return Reject(Error,TEXT("Tripo cluster paired texture identity differs"));
        auto* Texture=*Found;bool Flip=false;
        if(Texture->GetSizeX()!=Row->GetIntegerField(TEXT("received_width"))||Texture->GetSizeY()!=Row->GetIntegerField(TEXT("received_height"))
            ||Texture->GetSizeX()!=4096||Texture->GetSizeY()!=4096||bool(Texture->SRGB)!=Row->GetBoolField(TEXT("srgb"))
            ||(Row->TryGetBoolField(TEXT("flip_green_channel"),Flip)&&(bool(Texture->bFlipGreenChannel)!=Flip||Texture->CompressionSettings!=TC_Normalmap)))
            return Reject(Error,TEXT("Tripo cluster actual 4K/linear/received-normal interpretation differs"));
        Out.Textures.Add(Value);
    }
    return true;
}
struct FActualSite
{
    FTransform Placement;UStaticMeshComponent* Old=nullptr;TSharedPtr<FJsonObject> Evidence;
};
bool Site(AWarfareFrontlineGameMode* B,AActor* Owner,const wfexp::scene::SceneDefinition& Native,
    const FSite& Spec,const FActualModel& Model,FActualSite& Out,FString& Error)
{
    TSharedPtr<FJsonObject> Plan;const FString FitPath=TEXT("ArtSource/FRRU/V87/VauxTripo/CompositionV1/")+FString(Spec.Fit);
    if(!ReadPinned(FitPath,Spec.FitSHA,Plan,Error))return false;
    const auto Pins=Plan->GetObjectField(TEXT("nativePins"));const auto NativeSolid=Plan->GetObjectField(TEXT("nativeSolid"));
    const auto& Solid=Native.solids[Spec.Solid];FVector SolidMin=FVector::ZeroVector,SolidMax=FVector::ZeroVector;
    TArray<uint8> SourceBytes;
    if(Plan->GetStringField(TEXT("schema"))!=TEXT("v87.vaux.independent.tripo.rigid.fit.1")||!Plan->GetBoolField(TEXT("fitAccepted"))
        ||!Plan->GetBoolField(TEXT("fullSourceVertexEnvelopeChecked"))||!Plan->GetBoolField(TEXT("nativeEnvelopePass"))||!Plan->GetBoolField(TEXT("bottomWitnessPass"))
        ||NativeSolid->GetStringField(TEXT("id"))!=FString::Printf(TEXT("fr.vaux.solid.%d"),Spec.Solid)
        ||!VectorField(NativeSolid,TEXT("minimumCm"),SolidMin)||!VectorField(NativeSolid,TEXT("maximumCm"),SolidMax)
        ||!SolidMin.Equals(FVector(Solid.minimum.x,Solid.minimum.y,Solid.minimum.z),.0001)
        ||!SolidMax.Equals(FVector(Solid.maximum.x,Solid.maximum.y,Solid.maximum.z),.0001)
        ||Plan->GetStringField(TEXT("sourceManifest"))!=Model.Receipt->GetStringField(TEXT("source_manifest"))
        ||Plan->GetStringField(TEXT("sourceManifestSHA256"))!=Model.Receipt->GetStringField(TEXT("source_manifest_sha256")))
        return Reject(Error,TEXT("Tripo cluster exact native/independent fit identity differs"));
    for(const auto& Pair:{TPair<FString,FString>(Pins->GetStringField(TEXT("nativeJSON")),Pins->GetStringField(TEXT("nativeSHA256"))),
                         TPair<FString,FString>(Pins->GetStringField(TEXT("nativeHeightSource")),Pins->GetStringField(TEXT("nativeHeightSourceSHA256"))),
                         TPair<FString,FString>(Plan->GetStringField(TEXT("sourceManifest")),Plan->GetStringField(TEXT("sourceManifestSHA256"))),
                         TPair<FString,FString>(Plan->GetStringField(TEXT("sourceFBX")),Plan->GetStringField(TEXT("sourceFBXSHA256")))})
        if(!LoadPinned(Pair.Key,Pair.Value,SourceBytes,Error))return false;
    TSharedPtr<FJsonObject> Profile;
    if(!ReadPinned(Plan->GetStringField(TEXT("sourceProfile")),Plan->GetStringField(TEXT("sourceProfileSHA256")),Profile,Error))return false;
    if(!Profile->GetBoolField(TEXT("success"))||!Profile->GetBoolField(TEXT("originals_unchanged"))||Profile->GetBoolField(TEXT("decimation_run"))
        ||Profile->GetBoolField(TEXT("source_UV_rewritten"))||Profile->GetBoolField(TEXT("physics_changed")))
        return Reject(Error,TEXT("Tripo cluster real source preparation profile differs"));
    const auto LOD=Model.MeshRow->GetArrayField(TEXT("geometry_lods"))[0]->AsObject();
    const auto* Description=Model.Mesh->GetMeshDescription(0);
    if(!Description||Description->Triangles().Num()!=Plan->GetIntegerField(TEXT("sourceTriangles"))
        ||Description->Triangles().Num()!=LOD->GetIntegerField(TEXT("source_triangles"))
        ||LOD->GetStringField(TEXT("source_sha256"))!=Plan->GetStringField(TEXT("sourceFBXSHA256"))
        ||Model.Mesh->GetName()!=Plan->GetStringField(TEXT("sourceMeshName")))return Reject(Error,TEXT("Tripo cluster complete actual source mesh differs"));
    FVector Translation=FVector::ZeroVector;const auto& Q=Plan->GetArrayField(TEXT("rotationQuaternionXYZW"));const double Scale=Plan->GetNumberField(TEXT("uniformScale"));
    if(Q.Num()!=4||!VectorField(Plan,TEXT("translationCm"),Translation)||!FMath::IsFinite(Scale)||Scale<.5||Scale>1.)
        return Reject(Error,TEXT("Tripo cluster independent rigid placement invalid"));
    const FQuat Rotation(Q[0]->AsNumber(),Q[1]->AsNumber(),Q[2]->AsNumber(),Q[3]->AsNumber());
    if(Rotation.ContainsNaN()||!Rotation.IsNormalized())return Reject(Error,TEXT("Tripo cluster fit rotation invalid"));
    Out.Placement=FTransform(Rotation,Translation,FVector(Scale));FBox ActualBounds(ForceInit);
    const auto Positions=FStaticMeshConstAttributes(*Description).GetVertexPositions();
    for(const FVertexID Vertex:Description->Vertices().GetElementIDs())ActualBounds+=Out.Placement.TransformPosition(FVector(Positions[Vertex]));
    const auto& PlannedBounds=Plan->GetArrayField(TEXT("worldBoundsCm"));FVector PlannedMin=FVector::ZeroVector,PlannedMax=FVector::ZeroVector;
    if(!ActualBounds.IsValid||ActualBounds.Min.X<SolidMin.X-.075||ActualBounds.Min.Y<SolidMin.Y-.075
        ||ActualBounds.Max.X>SolidMax.X+.075||ActualBounds.Max.Y>SolidMax.Y+.075||ActualBounds.Max.Z>SolidMax.Z+.075
        ||ActualBounds.GetSize().Z>SolidMax.Z-SolidMin.Z+.075||PlannedBounds.Num()!=2
        ||!VectorValue(PlannedBounds[0],PlannedMin)||!VectorValue(PlannedBounds[1],PlannedMax)
        ||!ActualBounds.Min.Equals(PlannedMin,.075)||!ActualBounds.Max.Equals(PlannedMax,.075))
        return Reject(Error,TEXT("Tripo cluster all actual source vertices exceed/differ from frozen native envelope"));
    const auto& Witnesses=Plan->GetArrayField(TEXT("bottomWitnesses"));const auto& Contacts=Model.MeshRow->GetArrayField(TEXT("contacts_ue_cm"));
    if(Witnesses.Num()!=Plan->GetIntegerField(TEXT("completeBottomWitnessCount"))||Witnesses.Num()!=Contacts.Num()||Witnesses.IsEmpty())
        return Reject(Error,TEXT("Tripo cluster complete actual import floor witnesses missing"));
    double MinimumGap=MAX_dbl,MaximumGap=-MAX_dbl;TArray<TSharedPtr<FJsonValue>> ActualWitnesses;
    for(int32 I=0;I<Witnesses.Num();++I)
    {
        const auto W=Witnesses[I]->AsObject();FVector Local=FVector::ZeroVector,ActualSource=FVector::ZeroVector,PlannedWorld=FVector::ZeroVector;
        if(!VectorField(W,TEXT("localUECm"),Local)||!VectorValue(Contacts[I],ActualSource)||!Local.Equals(ActualSource,.001)
            ||!VectorField(W,TEXT("worldCm"),PlannedWorld))return Reject(Error,TEXT("Tripo cluster witness differs from actual received/imported source"));
        const FVector World=Out.Placement.TransformPosition(Local);float Floor=0;
        if(!World.Equals(PlannedWorld,.075)||!WarfareFRRUWorldV82::TryHeight(B,float(World.X),float(World.Y),Floor))
            return Reject(Error,TEXT("Tripo cluster actual floor witness has no native contact"));
        const double Gap=World.Z-Floor;MinimumGap=FMath::Min(MinimumGap,Gap);MaximumGap=FMath::Max(MaximumGap,Gap);
        if(!FMath::IsNearlyEqual(double(Floor),W->GetNumberField(TEXT("nativeFloorCm")),.075)
            ||!FMath::IsNearlyEqual(Gap,W->GetNumberField(TEXT("floorGapCm")),.075))return Reject(Error,TEXT("Tripo cluster actual native contact differs from fit"));
        auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("actualWorld"),World.ToString());A->SetNumberField(TEXT("actualNativeFloorCm"),Floor);
        A->SetNumberField(TEXT("actualFloorGapCm"),Gap);ActualWitnesses.Add(MakeShared<FJsonValueObject>(A));
    }
    if(MaximumGap>.075||MinimumGap< -6.075)return Reject(Error,TEXT("Tripo cluster real base floats or exceeds existing 6cm embedding limit"));
    TArray<UStaticMeshComponent*> Existing;Owner->GetComponents(Existing);
    const FString OldName=FString::Printf(TEXT("WorldRenderV826_1_%03d"),Spec.OldPart);
    const FString OldMeshName=FString::Printf(TEXT("SM_WF_fr_vaux_ruins_%03d_V87_dressing"),Spec.OldPart);
    const FString OldPath=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/")+OldMeshName+TEXT(".")+OldMeshName;
    for(auto* C:Existing)if(C&&C->GetName()==OldName&&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==OldPath)Out.Old=C;
    if(!Out.Old||!Out.Old->IsRegistered()||!Out.Old->IsVisible()||Out.Old->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
        ||!Out.Old->GetRelativeTransform().Equals(FTransform::Identity,1.e-8))return Reject(Error,TEXT("Tripo cluster exact old cosmetic part missing"));
    Out.Evidence=MakeShared<FJsonObject>();auto& E=Out.Evidence;E->SetNumberField(TEXT("nativeSolidIndex"),Spec.Solid);
    E->SetStringField(TEXT("mesh"),Model.Mesh->GetPathName());E->SetStringField(TEXT("actualWorldTransform"),Out.Placement.ToHumanReadableString());
    E->SetStringField(TEXT("fit"),FitPath);E->SetStringField(TEXT("fitSHA256"),Spec.FitSHA);E->SetStringField(TEXT("importReceipt"),Model.ReceiptPath);
    E->SetStringField(TEXT("sourceProfile"),Plan->GetStringField(TEXT("sourceProfile")));E->SetStringField(TEXT("sourceProfileSHA256"),Plan->GetStringField(TEXT("sourceProfileSHA256")));
    E->SetStringField(TEXT("sourceFBXSHA256"),Plan->GetStringField(TEXT("sourceFBXSHA256")));E->SetStringField(TEXT("actualSourceVertexWorldMinimum"),ActualBounds.Min.ToString());
    E->SetStringField(TEXT("actualSourceVertexWorldMaximum"),ActualBounds.Max.ToString());E->SetNumberField(TEXT("actualSourceVertices"),Description->Vertices().Num());
    E->SetNumberField(TEXT("actualSourceTriangles"),Description->Triangles().Num());E->SetNumberField(TEXT("actualNaniteTriangles"),Model.Mesh->GetNumNaniteTriangles());
    E->SetNumberField(TEXT("actualFallbackTriangles"),Model.Mesh->GetNumTriangles(0));E->SetArrayField(TEXT("receivedPairedTextures"),Model.Textures);
    E->SetStringField(TEXT("replacedCosmeticPart"),OldPath);E->SetNumberField(TEXT("minimumBottomFloorGapCm"),MinimumGap);
    E->SetNumberField(TEXT("maximumBottomFloorGapCm"),MaximumGap);E->SetArrayField(TEXT("completeBottomWitnesses"),ActualWitnesses);
    E->SetBoolField(TEXT("allActualSourceVerticesInNativeEnvelope"),true);E->SetBoolField(TEXT("sourceFoundationSemanticsConfirmed"),false);
    return true;
}
}
static bool StageVauxTripoClusterV87(AWarfareFrontlineGameMode* B,AActor* Owner,AActor*& Out,TSharedPtr<FJsonObject>& Evidence,FString& Error)
{
    using namespace VauxTripoClusterV87;
    if(!B||!B->IsProgressionVolatile()||Out||!Owner||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.vaux")
        ||!VauxTripoRequestedV87(TEXT("fr.vaux.scene.families.v1"))||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87TripoCluster")))
        return Reject(Error,TEXT("Tripo cluster requires its exact private Vaux NoSave slot"));
    const auto* Native=wfexp::scene::ForMission("fr.vaux");
    if(!Native||Native->revision!=1||Native->solids.size()!=30)return Reject(Error,TEXT("Tripo cluster native Vaux authority differs"));
    FActualModel ActualModels[3];for(int32 I=0;I<3;++I)if(!Model(I,ActualModels[I],Error))return false;
    FActualSite ActualSites[UE_ARRAY_COUNT(Sites)];for(int32 I=0;I<UE_ARRAY_COUNT(Sites);++I)if(!Site(B,Owner,*Native,Sites[I],ActualModels[Sites[I].Model],ActualSites[I],Error))return false;
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Out=B->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Params);
    if(!Out)return Reject(Error,TEXT("Tripo cluster group spawn failed"));
    auto* Root=NewObject<USceneComponent>(Out,TEXT("VauxTripoClusterRoot"),RF_Transient);Out->AddInstanceComponent(Root);Out->SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();Out->SetActorTransform(FTransform::Identity);
    Out->Tags.Add(TEXT("FRRU_V87TripoRuin"));Out->Tags.Add(TEXT("FRRU_V87TripoCluster"));TArray<TSharedPtr<FJsonValue>> Members;
    const auto Fail=[&](const FString& Reason){Out->Destroy();Out=nullptr;return Reject(Error,Reason);};
    for(int32 I=0;I<UE_ARRAY_COUNT(Sites);++I)
    {
        const auto& Site=ActualSites[I];const auto& Model=ActualModels[Sites[I].Model];
        auto* C=NewObject<UStaticMeshComponent>(Out,FName(*FString::Printf(TEXT("VauxTripoClusterSolid%d"),Sites[I].Solid)),RF_Transient);
        Out->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Model.Mesh);
        C->SetRelativeTransform(Site.Placement);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);
        C->ComponentTags.Add(TEXT("FRRU_V87TripoRuin"));C->ComponentTags.Add(TEXT("FRRU_V87TripoCluster"));C->RegisterComponent();
        auto* MID=C->CreateDynamicMaterialInstance(0,Model.Material);
        if(!MID)return Fail(TEXT("Tripo cluster per-source paired PBR instance failed"));
        // Copy the actual imported PBR baseline, just as the original single
        // actor did. Existing scene-weather updates find the retained tag.
        MID->SetVectorParameterValue(TEXT("SoilTint"),Model.SoilTint);MID->SetScalarParameterValue(TEXT("WeatherWetness"),Model.Wetness);
        FLinearColor Tint;const auto* Body=C->GetBodyInstance();
        if(!C->IsRegistered()||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||C->GetGenerateOverlapEvents()||C->IsPhysicsStateCreated()
            ||(Body&&Body->IsValidBodyInstance())||!C->GetComponentTransform().Equals(Site.Placement,.0001)
            ||!MID->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Tint)||!Tint.Equals(Model.SoilTint,.000001f))
            return Fail(TEXT("Tripo cluster actual NoCollision/transform/PBR baseline differs"));
        Site.Evidence->SetBoolField(TEXT("actualComponentNoCollisionNoOverlapNoBody"),true);
        Site.Evidence->SetStringField(TEXT("actualMaterialParent"),Model.Material->GetPathName());Site.Evidence->SetStringField(TEXT("actualSoilTint"),Tint.ToString());
        Members.Add(MakeShared<FJsonValueObject>(Site.Evidence));
    }
    // Hide only after all eight real components passed; failure leaves every
    // original visual intact. The physical native carrier is never touched.
    for(const auto& Site:ActualSites){Site.Old->SetVisibility(false);Site.Old->SetHiddenInGame(true);Site.Old->SetCastShadow(false);
        Site.Evidence->SetBoolField(TEXT("oldRenderHiddenButRegistered"),!Site.Old->IsVisible()&&Site.Old->IsRegistered());}
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("schema"),TEXT("v87.vaux.actual.tripo.eight-point.cosmetic.1"));
    Evidence->SetBoolField(TEXT("clusterComparison"),true);Evidence->SetBoolField(TEXT("singleModelComparison"),false);Evidence->SetNumberField(TEXT("actualModelComponents"),Members.Num());
    Evidence->SetArrayField(TEXT("members"),Members);Evidence->SetBoolField(TEXT("nativePhysicsOrLOSChanged"),false);
    Evidence->SetBoolField(TEXT("visualLOSPointwiseEquivalent"),false);Evidence->SetBoolField(TEXT("artAccepted"),false);Evidence->SetBoolField(TEXT("runtimeAccepted"),false);
    Evidence->SetBoolField(TEXT("shippingAccepted"),false);UE_LOG(LogTemp,Display,TEXT("WF_FRRU_TRIPO_VAUX_CLUSTER actual_components=%d native_solids=%d physics_changed=0"),Members.Num(),int32(Native->solids.size()));
    return true;
}
#endif
