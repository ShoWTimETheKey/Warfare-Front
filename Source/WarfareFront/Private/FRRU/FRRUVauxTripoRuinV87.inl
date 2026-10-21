// One Editor-only comparison. The original collision, LOS and ten other
// ruined buildings remain owned by the unchanged native Vaux scene.
#if WITH_EDITOR
static bool VauxTripoRequestedV87(const FString& Id)
{
    return Id==TEXT("fr.vaux.scene.families.v1")&&WarfareFRRUWorldRenderBindingV87::CandidateEnabled()
        &&WarfareFRRUWorldRenderBindingV87::RuinsEnabled()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87TripoRuin"));
}
#include "FRRUVauxTripoClusterV87.inl"
static bool StageVauxTripoRuinV87(AWarfareFrontlineGameMode* B,AActor* SceneOwner,AActor*& Out,
    TSharedPtr<FJsonObject>& Evidence,FString& Error)
{
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87TripoCluster")))return StageVauxTripoClusterV87(B,SceneOwner,Out,Evidence,Error);
    if(!B||!B->IsProgressionVolatile()||Out||!SceneOwner)return Reject(Error,TEXT("Vaux Tripo requires its disposable cosmetic slot"));
    const auto* Native=wfexp::scene::ForMission("fr.vaux");
    if(!Native||Native->revision!=1||Native->solids.size()!=30)return Reject(Error,TEXT("Vaux native solids differ"));
    const auto& Solid=Native->solids[16];
    if(Solid.minimum.x!=720.||Solid.minimum.y!=550.||Solid.maximum.x!=1380.||Solid.maximum.y!=1050.
        ||!FMath::IsNearlyEqual(Solid.maximum.z,418.,.0001))return Reject(Error,TEXT("Vaux solid16 envelope differs"));
    const FString Prefix=TEXT("/Game/Warfare/AssetIntake/Tripo/Tripo_v87_vaux_farmhouse_h31_b81c0c32_ruin02/import01/");
    const FString Name=TEXT("SM_Tripo_v87_vaux_farmhouse_h31_b81c0c32_ruin02"),MaterialName=TEXT("M_Tripo_v87_vaux_farmhouse_h31_b81c0c32_ruin02_0");
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Prefix+TEXT("Meshes/")+Name+TEXT(".")+Name));
    if(!Mesh||!Mesh->GetRenderData()||Mesh->GetNumNaniteTriangles()!=1990241||Mesh->GetNumTriangles(0)!=298536
        ||!Mesh->GetNaniteSettings().bEnabled||!FMath::IsNearlyEqual(Mesh->GetNaniteSettings().KeepPercentTriangles,1.f)
        ||Mesh->GetNaniteSettings().TrimRelativeError!=0||Mesh->GetStaticMaterials().Num()!=1)
        return Reject(Error,TEXT("Exact full-source Tripo Nanite/fallback candidate unavailable"));
    const auto* Body=Mesh->GetBodySetup();const auto& Slot=Mesh->GetStaticMaterials()[0];auto* Material=Slot.MaterialInterface.Get();
    if(!Body||Body->AggGeom.GetElementCount()!=0
        ||Slot.MaterialSlotName!=TEXT("Tripo_0_tripo_material_90420bb7_de5b_4119_8f9f_7b799f0c74d2")
        ||!Material||Material->GetPathName()!=Prefix+TEXT("Materials/")+MaterialName+TEXT(".")+MaterialName||!ShaderReady(Material,Error))
        return Reject(Error,TEXT("Exact Tripo NoCollision/material contract differs"));
    const FBox Local=Mesh->GetBoundingBox();
    if(!Local.Min.Equals(FVector(-330,-241.53082275390625,0),.075)||!Local.Max.Equals(FVector(330,241.53082275390625,295.91845703125),.075))
        return Reject(Error,TEXT("Actual Tripo centimetre/UE bounds differ"));
    TArray<UTexture*> Used;Material->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(GetCachedScalabilityCVars().MaterialQualityLevel),TOptional<EShaderPlatform>(GMaxRHIShaderPlatform));
    TSet<FString> ActualTextures;for(auto* Texture:Used)if(auto* T=Cast<UTexture2D>(Texture))
    {
        if(T->GetSizeX()!=4096||T->GetSizeY()!=4096||!T->GetPathName().StartsWith(Prefix+TEXT("Textures/")))return Reject(Error,TEXT("Tripo paired 4K texture changed"));
        ActualTextures.Add(T->GetPathName());
    }
    TSet<FString> ExpectedTextures;
    for(const TCHAR* Tail:{TEXT("0_Color"),TEXT("1_Normal"),TEXT("2_Masks")})
    {const FString TextureName=TEXT("T_Tripo_v87_vaux_farmhouse_h31_b81c0c32_ruin02_")+FString(Tail);ExpectedTextures.Add(Prefix+TEXT("Textures/")+TextureName+TEXT(".")+TextureName);}
    if(ActualTextures.Num()!=3) return Reject(Error,TEXT("Tripo must retain its three received PBR sources"));
    for(const auto& Texture:ExpectedTextures)if(!ActualTextures.Contains(Texture))return Reject(Error,TEXT("Tripo actual PBR source identity differs"));
    FString Json;TSharedPtr<FJsonObject> Plan;
    const FString PlanPath=FPaths::ProjectDir()+TEXT("ArtSource/FRRU/V87/VauxTripo/placement-v87.json");
    if(!FFileHelper::LoadFileToString(Json,*PlanPath)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Plan)||!Plan.IsValid()
        ||Plan->GetStringField(TEXT("schema"))!=TEXT("v87.vaux.single.tripo.rigid.fit.1")||Plan->GetStringField(TEXT("meshName"))!=Name
        ||Plan->GetStringField(TEXT("sourceFBXSHA256"))!=TEXT("80ea0ee6531e65b8a3b80861cedb76c9d7eaad86f700c33eec20611d7bc9ef93")
        ||Plan->GetStringField(TEXT("nativeSHA256"))!=TEXT("310d9cc21b8c167207dbe655c11829f2ca6c174b5069c59c16b514f16ac7f512"))
        return Reject(Error,TEXT("Exact offline full-source Vaux fit missing"));
    const auto VectorField=[](const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FVector& Value)
    {
        const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!J||!J->TryGetArrayField(Key,A)||A->Num()!=3)return false;
        double X=0,Y=0,Z=0;if(!(*A)[0]->TryGetNumber(X)||!(*A)[1]->TryGetNumber(Y)||!(*A)[2]->TryGetNumber(Z))return false;
        Value=FVector(X,Y,Z);return !Value.ContainsNaN();
    };
    FVector Translation;const TArray<TSharedPtr<FJsonValue>>* Q=nullptr;double Scale=0;
    if(!VectorField(Plan,TEXT("translationCm"),Translation)||!Plan->TryGetArrayField(TEXT("rotationQuaternionXYZW"),Q)||Q->Num()!=4
        ||!Plan->TryGetNumberField(TEXT("uniformScale"),Scale)||!FMath::IsFinite(Scale)||Scale<.5||Scale>1.)return Reject(Error,TEXT("Invalid rigid Vaux fit transform"));
    const FQuat Rotation((*Q)[0]->AsNumber(),(*Q)[1]->AsNumber(),(*Q)[2]->AsNumber(),(*Q)[3]->AsNumber());
    if(Rotation.ContainsNaN()||!Rotation.IsNormalized()||!FMath::IsNearlyEqual(Translation.X,1050.,.001)||!FMath::IsNearlyEqual(Translation.Y,800.,.001))
        return Reject(Error,TEXT("Tripo fit must be positive-scale native-centred UE coordinates"));
    const FTransform Placement(Rotation,Translation,FVector(Scale));const FMeshDescription* Description=Mesh->GetMeshDescription(0);
    if(!Description||Description->Triangles().Num()!=1990241)return Reject(Error,TEXT("Actual full source description unavailable for envelope check"));
    const auto Positions=FStaticMeshConstAttributes(*Description).GetVertexPositions();FBox ActualBounds(ForceInit);
    for(const FVertexID Vertex:Description->Vertices().GetElementIDs())ActualBounds+=Placement.TransformPosition(FVector(Positions[Vertex]));
    if(!ActualBounds.IsValid||ActualBounds.Min.X<720.-.075||ActualBounds.Min.Y<550.-.075||ActualBounds.Max.X>1380.+.075
        ||ActualBounds.Max.Y>1050.+.075||ActualBounds.Max.Z>418.+.075||ActualBounds.GetSize().Z>310.+.075)
        return Reject(Error,TEXT("Actual full-source rigid fit exceeds native XY/top/height envelope"));
    const TArray<TSharedPtr<FJsonValue>>* Witnesses=nullptr;
    if(!Plan->TryGetArrayField(TEXT("bottomWitnesses"),Witnesses)||Witnesses->Num()!=614)return Reject(Error,TEXT("Complete actual bottom witnesses required"));
    TArray<TSharedPtr<FJsonValue>> ActualWitnesses;double MinimumGap=MAX_dbl,MaximumGap=-MAX_dbl;
    for(const auto& Value:*Witnesses)
    {
        const auto Row=Value->AsObject();FVector Point;
        if(!VectorField(Row,TEXT("localUECm"),Point))return Reject(Error,TEXT("Invalid bottom source witness"));
        const FVector World=Placement.TransformPosition(Point);float Floor=0;
        if(!WarfareFRRUWorldV82::TryHeight(B,float(World.X),float(World.Y),Floor))return Reject(Error,TEXT("Tripo witness has no native floor"));
        const double Gap=World.Z-Floor;MinimumGap=FMath::Min(MinimumGap,Gap);MaximumGap=FMath::Max(MaximumGap,Gap);
        auto Actual=MakeShared<FJsonObject>();Actual->SetStringField(TEXT("localUE"),Point.ToString());Actual->SetStringField(TEXT("actualWorld"),World.ToString());
        Actual->SetNumberField(TEXT("actualNativeFloorCm"),Floor);Actual->SetNumberField(TEXT("actualFloorGapCm"),Gap);ActualWitnesses.Add(MakeShared<FJsonValueObject>(Actual));
    }
    if(MaximumGap>.075||MinimumGap< -6.075)return Reject(Error,TEXT("Real Vaux base floats or over-embeds; inspect actual witnesses"));
    UStaticMeshComponent* Old=nullptr;TArray<UStaticMeshComponent*> Existing;SceneOwner->GetComponents(Existing);
    const FString Replaced=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/SM_WF_fr_vaux_ruins_001_V87_dressing.SM_WF_fr_vaux_ruins_001_V87_dressing");
    for(auto* Component:Existing)if(Component&&Component->GetName()==TEXT("WorldRenderV826_1_001")&&Component->GetStaticMesh()&&Component->GetStaticMesh()->GetPathName()==Replaced)Old=Component;
    if(!Old||!Old->IsRegistered()||Old->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||!Old->GetRelativeTransform().Equals(FTransform::Identity,1.e-8))
        return Reject(Error,TEXT("Exact old solid16 cosmetic part missing"));
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Out=B->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Params);
    if(!Out)return Reject(Error,TEXT("Tripo cosmetic actor spawn failed"));
    auto* Component=NewObject<UStaticMeshComponent>(Out,TEXT("VauxTripoRuin02"),RF_Transient);Out->AddInstanceComponent(Component);Out->SetRootComponent(Component);
    Out->Tags.Add(TEXT("FRRU_V87TripoRuin"));Component->ComponentTags.Add(TEXT("FRRU_V87TripoRuin"));Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Mesh);Component->SetCollisionProfileName(TEXT("NoCollision"));Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false);Component->SetCanEverAffectNavigation(false);Component->SetCastShadow(true);Component->RegisterComponent();
    Out->SetActorTransform(Placement,false,nullptr,ETeleportType::TeleportPhysics);
    const auto* ActualBody=Component->GetBodyInstance();
    if(!Component->IsRegistered()||Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||Component->GetGenerateOverlapEvents()
        ||Component->IsPhysicsStateCreated()||(ActualBody&&ActualBody->IsValidBodyInstance())||!Out->GetActorTransform().Equals(Placement,.0001))
    {Out->Destroy();Out=nullptr;return Reject(Error,TEXT("Tripo actual cosmetic registration/transform failed"));}
    auto* MID=Component->CreateDynamicMaterialInstance(0,Material);if(!MID){Out->Destroy();Out=nullptr;return Reject(Error,TEXT("Tripo per-scene material instance failed"));}
    Old->SetVisibility(false);Old->SetHiddenInGame(true);Old->SetCastShadow(false);
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("mesh"),Mesh->GetPathName());Evidence->SetStringField(TEXT("actualWorldTransform"),Out->GetActorTransform().ToHumanReadableString());
    Evidence->SetStringField(TEXT("actualSourceVertexWorldMinimum"),ActualBounds.Min.ToString());Evidence->SetStringField(TEXT("actualSourceVertexWorldMaximum"),ActualBounds.Max.ToString());
    Evidence->SetNumberField(TEXT("uniformScale"),Scale);Evidence->SetNumberField(TEXT("actualNaniteTriangles"),Mesh->GetNumNaniteTriangles());Evidence->SetNumberField(TEXT("actualFallbackTriangles"),Mesh->GetNumTriangles(0));
    Evidence->SetNumberField(TEXT("actualSourceVertices"),Description->Vertices().Num());Evidence->SetNumberField(TEXT("actualSourceTriangles"),Description->Triangles().Num());
    Evidence->SetNumberField(TEXT("assetSimpleCollisionShapes"),Body->AggGeom.GetElementCount());Evidence->SetStringField(TEXT("assetDefaultCollisionProfile"),Body->DefaultInstance.GetCollisionProfileName().ToString());
    Evidence->SetNumberField(TEXT("assetDefaultCollisionEnabled"),int32(Body->DefaultInstance.GetCollisionEnabled(false)));
    Evidence->SetBoolField(TEXT("actualComponentNoCollisionNoOverlapNoBody"),true);
    Evidence->SetStringField(TEXT("sourceFBXSHA256"),Plan->GetStringField(TEXT("sourceFBXSHA256")));Evidence->SetStringField(TEXT("nativeDataSHA256"),Plan->GetStringField(TEXT("nativeSHA256")));
    Evidence->SetNumberField(TEXT("minimumBottomFloorGapCm"),MinimumGap);Evidence->SetNumberField(TEXT("maximumBottomFloorGapCm"),MaximumGap);Evidence->SetArrayField(TEXT("completeBottomWitnesses"),ActualWitnesses);
    Evidence->SetNumberField(TEXT("renderBelowOriginalColliderBottomCm"),FMath::Max(0.,108.-ActualBounds.Min.Z));Evidence->SetBoolField(TEXT("oldRenderHiddenButRegistered"),!Old->IsVisible()&&Old->IsRegistered());
    Evidence->SetStringField(TEXT("actualHiddenOldBoundsOrigin"),Old->Bounds.Origin.ToString());Evidence->SetStringField(TEXT("actualHiddenOldBoundsExtent"),Old->Bounds.BoxExtent.ToString());
    Evidence->SetBoolField(TEXT("nativePhysicsOrLOSChanged"),false);Evidence->SetBoolField(TEXT("visualLOSPointwiseEquivalent"),false);Evidence->SetBoolField(TEXT("artAccepted"),false);Evidence->SetBoolField(TEXT("runtimeAccepted"),false);
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_TRIPO_VAUX_COSMETIC scale=%.6f transform=%s fullTriangles=%d fallback=%d floorGap=[%.3f,%.3f]"),Scale,*Placement.ToHumanReadableString(),Mesh->GetNumNaniteTriangles(),Mesh->GetNumTriangles(0),MinimumGap,MaximumGap);
    return true;
}
#endif
