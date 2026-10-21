// Editor-only, exact solid27 cosmetic pilot. Include globally after the
// existing Vaux release loader; never include inside another namespace.
#pragma once
#if WITH_EDITOR
namespace FRRUVauxHQ27VisualV87
{
constexpr const TCHAR* Flag=TEXT("WFFRRUV87VauxHQ27");
constexpr const TCHAR* Scene=TEXT("fr.vaux.scene.families.v1");
constexpr const TCHAR* ActivationRelative=TEXT("ArtSource/FRRU/V87/VauxHQ27Visual/CandidateV1/activation.json");
// Deliberately unarmed: freeze only after a successful actual five-package
// copy receipt. A failed/NullRHI-only draft never manufactures asset hashes.
constexpr const TCHAR* ActivationSHA=TEXT("1769321a4f959ea398d1c681934fad3a8fbd588a7dc55e6aa07bb6112bdd8332");
constexpr const TCHAR* OldPath=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/SM_WF_fr_vaux_ruins_000_V87_dressing.SM_WF_fr_vaux_ruins_000_V87_dressing");
constexpr const TCHAR* RemainderPath=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRedoubtRemainder/CandidateR2/Scenes/SM_WF_fr_vaux_redoubt27_remainder_clean_R2_V87_dressing.SM_WF_fr_vaux_redoubt27_remainder_clean_R2_V87_dressing");
constexpr const TCHAR* HQPath=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/EarthRedoubtHQ/CandidateV1/Meshes/SM_EarthRedoubt27_HQ_V87.SM_EarthRedoubt27_HQ_V87");
constexpr const TCHAR* MaterialPath=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/EarthRedoubtHQ/CandidateV1/Materials/M_EarthRedoubt27_HQ_V87.M_EarthRedoubt27_HQ_V87");
constexpr const TCHAR* RemainderTag=TEXT("FRRU_V87HQ27Remainder");
constexpr const TCHAR* HQTag=TEXT("FRRU_V87HQ27Visual");
struct FStamp
{
    TWeakObjectPtr<const AActor> Owner,HQ;TWeakObjectPtr<const UStaticMeshComponent> Part,Visual;
    TWeakObjectPtr<const UStaticMesh> NativeGround,NativeSolids,Remainder;
    TWeakObjectPtr<const UMaterialInstanceDynamic> MID;FTransform OwnerPose,Pose;
};
inline TMap<TWeakObjectPtr<const AActor>,FStamp> Stamps;
inline bool Fail(FString& Error,const TCHAR* Why){Error=FString(TEXT("Vaux HQ27 visual: "))+Why;return false;}
inline FString Normal(FString P){P=FPaths::ConvertRelativePathToFull(P);FPaths::NormalizeDirectoryName(P);return P;}
inline bool Requested()
{
    TArray<FString> Tokens,Switches;FCommandLine::Parse(FCommandLine::Get(),Tokens,Switches);
    for(FString T:Tokens){if(T.StartsWith(TEXT("-"))||T.StartsWith(TEXT("/")))T.RightChopInline(1);FString N,V;if(!T.Split(TEXT("="),&N,&V))N=T;if(N.Equals(Flag,ESearchCase::IgnoreCase))return true;}return false;
}
inline bool Scope(AWarfareFrontlineGameMode* B,const FString& Id,FString& User,FString& Error)
{
    if(!B||!B->GetWorld()||B->GetWorld()->WorldType!=EWorldType::Game||!FApp::IsGame()||!IsInGameThread()
        ||!B->IsProgressionVolatile()||!WarfareFRRUCampaignV82::Active(B)||WarfareFRRUCampaignV82::Nation(B)!=EFRRUNationV81::French
        ||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.vaux")||Id!=Scene)return Fail(Error,TEXT("requires actual isolated French Vaux game world"));
    const TCHAR* Command=FCommandLine::Get();FString Case;
    for(const TCHAR* Required:{Flag,TEXT("WFRenderNoSave"),TEXT("WFFRRUV82Review"),TEXT("WFFRRUV82WorldReview"),TEXT("WFFRRUV82NaturalReview"),TEXT("WFFRRUV82VisualCaptureOnly")})
        if(!FParse::Param(Command,Required))return Fail(Error,TEXT("requires explicit HQ27 visual-only NoSave flags"));
    if(!FParse::Value(Command,TEXT("FRRUNaturalCase="),Case)||Case!=TEXT("vaux-rifle")||!FParse::Value(Command,TEXT("UserDir="),User)
        ||User.IsEmpty()||FPaths::IsRelative(User)||User.Contains(TEXT("..")))return Fail(Error,TEXT("requires vaux-rifle absolute GUID UserDir"));
    TArray<FString> Tokens,Switches;FCommandLine::Parse(Command,Tokens,Switches);TSet<FString> Seen;
    for(FString T:Tokens)
    {
        if(T.StartsWith(TEXT("-"))||T.StartsWith(TEXT("/")))T.RightChopInline(1);FString Name,Value;const bool Has=T.Split(TEXT("="),&Name,&Value);if(!Has)Name=T;
        if((Name.StartsWith(TEXT("WFFRRUV87Vaux"),ESearchCase::IgnoreCase)&&!Name.Equals(Flag,ESearchCase::IgnoreCase))
            ||Name.StartsWith(TEXT("WFFRRUV87Tripo"),ESearchCase::IgnoreCase)||Name.StartsWith(TEXT("FRRUFTNav"),ESearchCase::IgnoreCase))return Fail(Error,TEXT("other art/old ruins/navigation candidate is incompatible"));
        const bool ValueOption=Name.Equals(TEXT("UserDir"),ESearchCase::IgnoreCase)||Name.Equals(TEXT("FRRUNaturalCase"),ESearchCase::IgnoreCase);
        const bool BoolOption=Name.Equals(Flag,ESearchCase::IgnoreCase)||Name.Equals(TEXT("WFRenderNoSave"),ESearchCase::IgnoreCase)
            ||Name.StartsWith(TEXT("WFFRRUV82"),ESearchCase::IgnoreCase);
        if(ValueOption||BoolOption){const FString K=Name.ToLower();if(Seen.Contains(K)||(BoolOption&&Has))return Fail(Error,TEXT("duplicate or malformed scope option"));Seen.Add(K);}
    }
    const FString Root=Normal(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"))),Prefix=Root+TEXT("/Saved/FRRU/V82NaturalReview/vaux-rifle/");
    User=Normal(User);TArray<FString> Parts;User.Mid(Prefix.Len()).ParseIntoArray(Parts,TEXT("/"));FGuid Guid;
    if(!Normal(FPaths::ProjectDir()).Equals(Root,ESearchCase::IgnoreCase)||!User.StartsWith(Prefix,ESearchCase::IgnoreCase)||Parts.Num()!=2||Parts[1]!=TEXT("User")
        ||!FGuid::ParseExact(Parts[0],EGuidFormats::Digits,Guid)||!Guid.IsValid()||Normal(FPaths::ProjectUserDir())!=User||Normal(FPaths::ProjectSavedDir())!=User+TEXT("/Saved"))return Fail(Error,TEXT("project/GUID/User/Saved disagreement"));
#if PLATFORM_WINDOWS
    for(FString Part=User;Part.Len()>=Root.Len();Part=FPaths::GetPath(Part))
    {const DWORD A=GetFileAttributesW(*Part);if(A!=INVALID_FILE_ATTRIBUTES&&(A&FILE_ATTRIBUTE_REPARSE_POINT))return Fail(Error,TEXT("reparse path refused"));if(A==INVALID_FILE_ATTRIBUTES&&GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)return Fail(Error,TEXT("cannot prove plain path"));if(Part.Equals(Root,ESearchCase::IgnoreCase))break;}
#endif
    return IFileManager::Get().DirectoryExists(*User)||Fail(Error,TEXT("existing disposable User directory required"));
}
inline bool Cosmetic(const UStaticMeshComponent* C)
{
    if(!C||!C->IsRegistered()||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||C->GetGenerateOverlapEvents()||C->CanEverAffectNavigation()||C->IsPhysicsStateCreated())return false;
    const auto* Body=C->GetBodyInstance();return !Body||!Body->IsValidBodyInstance();
}
inline bool IsRemainder(const UStaticMeshComponent* C){return C&&C->ComponentHasTag(RemainderTag);}
inline bool ValidateRemainder(const FString& Id,const UStaticMeshComponent* C)
{
    const auto* S=C?Stamps.Find(C->GetOwner()):nullptr;const auto* HQ=S?S->Visual.Get():nullptr;
    return Id==Scene&&Requested()&&S&&S->Owner.IsValid()&&S->HQ.IsValid()&&S->Part.Get()==C&&S->Remainder.IsValid()&&S->MID.IsValid()
        &&Cosmetic(C)&&C->GetFName()==FName(TEXT("WorldRenderV826_1_000"))&&C->GetStaticMesh()==S->Remainder.Get()&&C->GetStaticMesh()->GetPathName()==RemainderPath
        &&C->GetRelativeTransform().Equals(FTransform::Identity,1.e-8)&&C->IsVisible()&&!C->bHiddenInGame&&C->CastShadow
        &&C->GetOwner()->GetActorTransform().Equals(S->OwnerPose,1.e-8)&&Cosmetic(HQ)&&HQ->GetOwner()==S->HQ.Get()
        &&HQ->GetStaticMesh()&&HQ->GetStaticMesh()->GetPathName()==HQPath&&HQ->GetMaterial(0)==S->MID.Get()
        &&HQ->GetNumMaterials()==1&&HQ->IsVisible()&&!HQ->bHiddenInGame&&HQ->CastShadow&&HQ->GetComponentTransform().Equals(S->Pose,.0001)
        &&HQ->IsUsingAbsoluteLocation()&&HQ->IsUsingAbsoluteRotation()&&HQ->IsUsingAbsoluteScale();
}
inline void Forget(const AActor* Owner){Stamps.Remove(Owner);}
inline bool Stage(AWarfareFrontlineGameMode* B,AActor* Owner,const FString& Id,AActor*& Out,TSharedPtr<FJsonObject>& Evidence,FString& Error,bool(*Ready)(UMaterialInterface*,FString&))
{
    FString User;if(!Scope(B,Id,User,Error))return false;
    if(!Owner||Out||!Ready||!Owner->GetActorTransform().Equals(FTransform(FQuat::Identity,FVector::ZeroVector,FVector(1,-1,1)),1.e-8))return Fail(Error,TEXT("exact original reflected scene owner required"));
    if(FCString::Strlen(ActivationSHA)!=64)return Fail(Error,TEXT("actual successful formal-copy activation hash has not been frozen"));
    TArray<uint8> Bytes;FString Hash,Text;TSharedPtr<FJsonObject> Plan;
    if(!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectDir()+ActivationRelative))||!FRRUReadOnlySHA256V87(Bytes,Hash)||Hash!=ActivationSHA
        ||!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()+ActivationRelative))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Plan)
        ||!Plan||Plan->GetStringField(TEXT("schema"))!=TEXT("v87.vaux.hq27.visual.activation.1")||!Plan->GetBoolField(TEXT("actualFormalCopySucceeded")))return Fail(Error,TEXT("exact successful copy/activation proof missing"));
    const auto* Native=wfexp::scene::ForMission("fr.vaux");
    if(!Native||Native->revision!=1||Native->solids.size()!=30||Native->routes.size()!=14)return Fail(Error,TEXT("native Vaux scene contract changed"));
    const auto& S=Native->solids[27];
    if(S.minimum.x!=7150.||S.minimum.y!=-4830.||S.minimum.z!=30.||S.maximum.x!=8050.||S.maximum.y!=-4170.||S.maximum.z!=450.)return Fail(Error,TEXT("original solid27 AABB changed"));
    const auto Pins=Plan->GetObjectField(TEXT("packagePins"));if(!Pins||Pins->Values.Num()!=9)return Fail(Error,TEXT("exact HQ5/R2/old-render/native2 package pins required"));
    for(const auto& Pin:Pins->Values)
    {const FString PinPath(*Pin.Key);if(!PinPath.StartsWith(TEXT("Content/"))||PinPath.Contains(TEXT(".."))||!PinPath.EndsWith(TEXT(".uasset"))||!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectDir()+PinPath))||!FRRUReadOnlySHA256V87(Bytes,Hash)||Hash!=Pin.Value->AsString())return Fail(Error,TEXT("actual pinned package changed"));}
    UStaticMeshComponent *Old=nullptr,*Ground=nullptr,*Solids=nullptr;TArray<UStaticMeshComponent*> Components;Owner->GetComponents(Components);int32 OldCount=0,GroundCount=0,SolidsCount=0;
    for(auto* C:Components)if(C)
    {if(C->GetFName()==FName(TEXT("WorldRenderV826_1_000"))){Old=C;++OldCount;}if(C->ComponentHasTag(TEXT("ground"))&&C->ComponentHasTag(TEXT("FRRU_V82Collision"))){Ground=C;++GroundCount;}if(C->ComponentHasTag(TEXT("solids"))&&C->ComponentHasTag(TEXT("FRRU_V82Collision"))){Solids=C;++SolidsCount;}}
    if(OldCount!=1||GroundCount!=1||SolidsCount!=1||!Cosmetic(Old)||!Old->GetStaticMesh()||Old->GetStaticMesh()->GetPathName()!=OldPath
        ||!Old->GetRelativeTransform().Equals(FTransform::Identity,1.e-8)||!Ground->IsRegistered()||!Solids->IsRegistered()
        ||Ground->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics||Solids->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics
        ||!Ground->IsPhysicsStateCreated()||!Solids->IsPhysicsStateCreated())return Fail(Error,TEXT("actual original carrier/unique native collision components differ"));
    const auto* Underlay=Ground->GetMaterial(0);if(!Underlay||!Underlay->GetMaterial()||Underlay->GetMaterial()->GetPathName()!=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxSurfaceParity/CandidateV1/Materials/M_VauxSurfaceParity_V87.M_VauxSurfaceParity_V87"))return Fail(Error,TEXT("original actual SurfaceParity underlay required"));
    auto* Remainder=LoadObject<UStaticMesh>(nullptr,RemainderPath);auto* Mesh=LoadObject<UStaticMesh>(nullptr,HQPath);auto* Material=LoadObject<UMaterialInterface>(nullptr,MaterialPath);
    if(!Remainder||!Mesh||!Material||Remainder->GetNumTriangles(0)!=316724||Remainder->GetStaticMaterials().Num()!=7
        ||!Mesh->HasValidNaniteData()||Mesh->GetNumNaniteTriangles()!=1885492||Mesh->GetNumTriangles(0)!=282822||Mesh->GetStaticMaterials().Num()!=1||Mesh->GetStaticMaterials()[0].MaterialInterface.Get()!=Material
        ||!Remainder->GetBodySetup()||Remainder->GetBodySetup()->AggGeom.GetElementCount()!=0||!Mesh->GetBodySetup()||Mesh->GetBodySetup()->AggGeom.GetElementCount()!=0||!Ready(Material,Error))return Fail(Error,TEXT("actual precise R2/HQ mesh/material/render contract differs"));
    const auto* Description=Mesh->GetMeshDescription(0);if(!Description||Description->Triangles().Num()!=1885492)return Fail(Error,TEXT("actual full HQ source geometry unavailable"));
    const FTransform Pose(FQuat::Identity,FVector(7600,-4500,30),FVector::OneVector);FBox FullBounds(ForceInit);
    const auto Positions=FStaticMeshConstAttributes(*Description).GetVertexPositions();for(FVertexID V:Description->Vertices().GetElementIDs())FullBounds+=Pose.TransformPosition(FVector(Positions[V]));
    if(!FullBounds.Min.Equals(FVector(7150,-4830,30),.075)||!FullBounds.Max.Equals(FVector(8050,-4170,450),.075))return Fail(Error,TEXT("HQ actual full-source world AABB differs"));
    TArray<UTexture*> Used;Material->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(GetCachedScalabilityCVars().MaterialQualityLevel),TOptional<EShaderPlatform>(GMaxRHIShaderPlatform));TSet<FString> ActualTextures;
    for(auto* T:Used)if(auto* T2=Cast<UTexture2D>(T)){if(!T2->GetResource()||T2->GetPixelFormat()==PF_Unknown||T2->GetSizeX()!=4096||T2->GetSizeY()!=4096)return Fail(Error,TEXT("actual rendered 4K PBR resources not ready"));ActualTextures.Add(T2->GetPathName());}
    if(ActualTextures.Num()!=3)return Fail(Error,TEXT("actual compiled HQ material must use exactly three original PBR copies"));
    for(const TCHAR* Tail:{TEXT("Color"),TEXT("Normal"),TEXT("Masks")})
    {const FString Name=FString(TEXT("T_EarthRedoubt27_"))+Tail+TEXT("_V87"),Path=FString(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/EarthRedoubtHQ/CandidateV1/Textures/"))+Name+TEXT(".")+Name;if(!ActualTextures.Contains(Path))return Fail(Error,TEXT("compiled HQ texture identity differs"));}
    FLinearColor Tint;float Wetness=0;if(!Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SoilTint")),Tint)||!Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Wetness)||!FMath::IsFinite(Wetness))return Fail(Error,TEXT("original HQ weather parameters absent"));
    const auto& Contacts=Plan->GetArrayField(TEXT("actualIntakeContactsUECm"));if(Contacts.Num()!=703)return Fail(Error,TEXT("exact 703 actual source bottom contacts required"));
    TArray<TSharedPtr<FJsonValue>> ContactRows;double Low=MAX_dbl,High=-MAX_dbl,RayError=0;
    for(const auto& Contact:Contacts)
    {const auto& A=Contact->AsArray();if(A.Num()!=3)return Fail(Error,TEXT("malformed actual contact"));const FVector Local(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()),World=Pose.TransformPosition(Local);double Floor=0;
        if(World.ContainsNaN()||!wfexp::scene::HeightAt(*Native,World.X,World.Y,Floor))return Fail(Error,TEXT("HQ contact has no native floor"));
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV87HQ27Contact),true);if(!Ground->LineTraceComponent(Hit,FVector(World.X,World.Y,Floor+1000),FVector(World.X,World.Y,Floor-1000),Q)||Hit.bStartPenetrating||FMath::Abs(Hit.ImpactPoint.Z-Floor)>.5)return Fail(Error,TEXT("actual unique ground ray differs from native floor"));
        const double Gap=World.Z-Floor;Low=FMath::Min(Low,Gap);High=FMath::Max(High,Gap);RayError=FMath::Max(RayError,FMath::Abs(Hit.ImpactPoint.Z-Floor));auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("actualWorld"),World.ToString());Row->SetNumberField(TEXT("nativeFloor"),Floor);Row->SetNumberField(TEXT("actualGroundRayZ"),Hit.ImpactPoint.Z);Row->SetNumberField(TEXT("visibleBaseGapCm"),Gap);ContactRows.Add(MakeShared<FJsonValueObject>(Row));}
    if(!FMath::IsNearlyEqual(Low,-1.5413413060506187,.01)||!FMath::IsNearlyEqual(High,5.474674922943109,.01))return Fail(Error,TEXT("fixed-pose source/native contact witness envelope changed"));
    FHitResult SolidHit;FCollisionQueryParams SolidQuery(SCENE_QUERY_STAT(FRRUV87HQ27OriginalUCX),false);if(!Solids->LineTraceComponent(SolidHit,FVector(7600,-4500,700),FVector(7600,-4500,-1000),SolidQuery)||FMath::Abs(SolidHit.ImpactPoint.Z-450.)>.5)return Fail(Error,TEXT("original registered solid27 roof collision changed"));
    int32 HQActors=0,NativeSceneActors=0;for(TActorIterator<AActor> It(B->GetWorld());It;++It){if(It->ActorHasTag(HQTag))++HQActors;if(It->ActorHasTag(TEXT("FRRU_V82Scene")))++NativeSceneActors;}if(HQActors||NativeSceneActors!=1)return Fail(Error,TEXT("exact single native scene and no prior HQ27 cosmetic actor required"));
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Out=B->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Params);if(!Out)return Fail(Error,TEXT("cosmetic actor allocation failed"));
    auto* C=NewObject<UStaticMeshComponent>(Out,TEXT("VauxHQ27Visual"),RF_Transient);Out->AddInstanceComponent(C);Out->SetRootComponent(C);Out->Tags.Add(HQTag);C->ComponentTags.Add(HQTag);C->ComponentTags.Add(TEXT("FRRU_V87TripoRuin"));C->SetAbsolute(true,true,true);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
    C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->RegisterComponent();Out->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics);
    auto* MID=C->CreateDynamicMaterialInstance(0,Material);if(!MID||!Cosmetic(C)||!C->GetComponentTransform().Equals(Pose,.0001)){Out->Destroy();Out=nullptr;return Fail(Error,TEXT("actual independent positive-scale cosmetic registration failed"));}
    MID->SetVectorParameterValue(TEXT("SoilTint"),Tint);MID->SetScalarParameterValue(TEXT("WeatherWetness"),Wetness);
    TArray<UMaterialInterface*> OldMaterials;for(int32 I=0;I<Old->GetNumMaterials();++I)OldMaterials.Add(Old->GetMaterial(I));Old->SetStaticMesh(Remainder);Old->ComponentTags.Add(RemainderTag);
    bool SameMaterials=OldMaterials.Num()==Old->GetNumMaterials();for(int32 I=0;I<OldMaterials.Num()&&SameMaterials;++I)SameMaterials&=Old->GetMaterial(I)==OldMaterials[I];
    Stamps.Add(Owner,{Owner,Out,Old,C,Ground->GetStaticMesh(),Solids->GetStaticMesh(),Remainder,MID,Owner->GetActorTransform(),Pose});
    if(!SameMaterials||!ValidateRemainder(Id,Old)){Stamps.Remove(Owner);Out->Destroy();Out=nullptr;return Fail(Error,TEXT("immutable candidate registration/material validation failed"));}
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("candidate"),TEXT("Editor-only exact solid27 cosmetic pilot"));Evidence->SetStringField(TEXT("remainder"),RemainderPath);Evidence->SetStringField(TEXT("HQ"),HQPath);Evidence->SetStringField(TEXT("actualWorldPose"),Pose.ToHumanReadableString());Evidence->SetStringField(TEXT("actualSourceBoundsMin"),FullBounds.Min.ToString());Evidence->SetStringField(TEXT("actualSourceBoundsMax"),FullBounds.Max.ToString());
    Evidence->SetStringField(TEXT("frontBasis"),TEXT("gltf front +Z -> Blender -Y -> UE +Y -> world +Y; independent positive identity rotation/scale; do not inherit joined-scene Y reflection"));Evidence->SetStringField(TEXT("nativeBBoxMin"),TEXT("7150,-4830,30"));Evidence->SetStringField(TEXT("nativeBBoxMax"),TEXT("8050,-4170,450"));Evidence->SetNumberField(TEXT("minimumVisibleBaseNativeGapCm"),Low);Evidence->SetNumberField(TEXT("maximumVisibleBaseNativeGapCm"),High);Evidence->SetNumberField(TEXT("maximumActualGroundRayNativeErrorCm"),RayError);Evidence->SetArrayField(TEXT("actual703BottomContacts"),ContactRows);Evidence->SetNumberField(TEXT("actualOriginalSolid27RoofRayZ"),SolidHit.ImpactPoint.Z);
    HQActors=0;for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(HQTag))++HQActors;if(HQActors!=1)return Fail(Error,TEXT("actual cosmetic actor census differs"));
    Evidence->SetNumberField(TEXT("actualNativeSceneActorCount"),NativeSceneActors);Evidence->SetNumberField(TEXT("actualHQ27CosmeticActorCount"),HQActors);Evidence->SetNumberField(TEXT("originalOwnerStaticComponentCount"),Components.Num());Evidence->SetBoolField(TEXT("originalNativePhysicsChanged"),false);Evidence->SetBoolField(TEXT("otherTrees28and29Changed"),false);Evidence->SetBoolField(TEXT("wholeSceneArtAccepted"),false);Evidence->SetBoolField(TEXT("foundation28Fixed"),false);Evidence->SetBoolField(TEXT("artAccepted"),false);Evidence->SetBoolField(TEXT("shippingAccepted"),false);Evidence->SetBoolField(TEXT("NTBRoundtripBitExact"),false);Evidence->SetNumberField(TEXT("actualNaniteTriangles"),Mesh->GetNumNaniteTriangles());Evidence->SetNumberField(TEXT("actualFallbackTriangles"),Mesh->GetNumTriangles(0));return true;
}
}
#endif
