// Receiver-only adapter for the original German V33 static hull/real track
// instances. It is deliberately an isolated Editor review gate, not release
// approval. Original assets, the five native rigs and their solver stay intact.
#include "FRRU/WarfareFRRUGermanHostAcceptanceV82.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif
namespace FRRUGermanStaticContactV82
{
#include "FRRUGermanHostRuntimeMetadataV82.generated.inl"
#include "FTGroundSupportV82.inl"
using namespace FRRUVehicleGroundDraftV82;
constexpr double ContactToleranceCm=.1;
// The authored links have about 18 cm pitch. A bounded 6 cm vertical stroke
// corresponds to about 19 degrees of link articulation, below the native
// atan(.65) slope bound. Pin diameter does not define suspension travel.
// This stroke never permits face penetration or extra chassis clearance.
constexpr double MaxShoeArticulationCm=6.;
constexpr int32 MaxFacePairs=65536,MaxRoofVisits=262144,MaxRoofCells=8192;
constexpr int32 MaxStaticVertices=250000,MaxStaticFaces=300000,MaxInstances=256;
constexpr int32 MaxContactPasses=4,MaxMoveSubsteps=8;
const TCHAR* ReceiverRoot=TEXT("/Game/FRRUIntegration/FormalV82/HostArmour/German/");
// Only runtime-retained properties enter this canonical collision digest.
// Editor-only generated-shape flags, source models and imported slot names
// would otherwise make an identical cooked mesh appear different.
struct FRuntimeHashV82
{
    FSHA1 Hash;bool Valid=true;
    template<class T> void Bytes(const T& Value){Hash.Update(reinterpret_cast<const uint8*>(&Value),sizeof(Value));}
    void Text(const FString& Value){FTCHARToUTF8 UTF8(*Value);const int32 Length=UTF8.Length();Bytes(Length);Hash.Update(reinterpret_cast<const uint8*>(UTF8.Get()),Length);}
    void Number(double Value){Valid&=FMath::IsFinite(Value);Bytes(Value);}
    void Vector(const FVector& Value){Number(Value.X);Number(Value.Y);Number(Value.Z);}
    void Transform(const FTransform& Value){Valid&=!Value.ContainsNaN();Vector(Value.GetLocation());Vector(Value.GetScale3D());const FQuat Q=Value.GetRotation();Number(Q.X);Number(Q.Y);Number(Q.Z);Number(Q.W);}
    void Shape(const FKShapeElem& Value){Text(Value.GetName().ToString());Bytes(uint8(Value.GetShapeType()));Number(Value.RestOffset);Bytes(uint8(Value.GetContributeToMass()));Bytes(uint8(Value.GetCollisionEnabled()));Transform(Value.GetTransform());}
    FString Finish(){if(!Valid)return {};Hash.Final();uint8 Digest[FSHA1::DigestSize];Hash.GetHash(Digest);return BytesToHex(Digest,FSHA1::DigestSize);}
};
struct FCollisionRecipeHashV82
{
    FRuntimeHashV82& Complete;FRuntimeHashV82& Recipe;
    template<class T> void Bytes(const T& V){Complete.Bytes(V);Recipe.Bytes(V);}
    void Text(const FString& V){Complete.Text(V);Recipe.Text(V);}
    void Number(double V){Complete.Number(V);Recipe.Number(V);}
    void Vector(const FVector& V){Complete.Vector(V);Recipe.Vector(V);}
    void Shape(const FKShapeElem& V){Complete.Shape(V);Recipe.Shape(V);}
};
#if WITH_EDITOR
// Rebuilding only the tangent basis may reorder the renderer's vertex/index
// streams. Compare the same oriented corner attributes, with exact float
// bytes, independently of that storage order. A cyclic rotation preserves a
// triangle's winding; a reversal is never canonicalised away.
template<class T> void CornerBytesV82(TArray<uint8>& Out,const T& Value)
{Out.Append(reinterpret_cast<const uint8*>(&Value),sizeof(Value));}
int32 CompareCornerBytesV82(const TArray<uint8>& A,const TArray<uint8>& B)
{const int32 N=FMath::Min(A.Num(),B.Num());const int32 C=N?FMemory::Memcmp(A.GetData(),B.GetData(),N):0;return C?C:(A.Num()<B.Num()?-1:(A.Num()>B.Num()?1:0));}
void HashCornerRecordV82(FRuntimeHashV82& Hash,const TArray<uint8>& Bytes)
{Hash.Bytes(Bytes.Num());if(!Bytes.IsEmpty())Hash.Hash.Update(Bytes.GetData(),Bytes.Num());}
TSharedPtr<FJsonObject> EditorCanonicalCornerProofV82(const FStaticMeshRenderData* Render)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);
    J->SetStringField(TEXT("schemaVersion"),TEXT("v82.german.static.oriented.corners.1"));
    J->SetStringField(TEXT("encoding"),TEXT("exact-float32-byte-corners;per-section;cyclic-only;vertex-multiset"));
    const auto Reject=[&](const TCHAR* Why){J->SetStringField(TEXT("reason"),Why);return J;};
    FRuntimeHashV82 Triangles,Vertices,Sections,Surface,UniqueAttributes;TArray<TSharedPtr<FJsonValue>> LODs;
    if(!Render)return Reject(TEXT("canonical render data missing"));
    Triangles.Bytes(Render->LODResources.Num());Vertices.Bytes(Render->LODResources.Num());Sections.Bytes(Render->LODResources.Num());
    Surface.Bytes(Render->LODResources.Num());UniqueAttributes.Bytes(Render->LODResources.Num());
    for(int32 L=0;L<Render->LODResources.Num();++L)
    {
        const auto& R=Render->LODResources[L];const auto& VB=R.VertexBuffers;const auto Indices=R.IndexBuffer.GetArrayView();
        const uint32 Count=VB.PositionVertexBuffer.GetNumVertices(),UVs=VB.StaticMeshVertexBuffer.GetNumTexCoords();
        const bool Colour=VB.ColorVertexBuffer.GetNumVertices()!=0;const int64 CornerSize=24+int64(UVs)*8+(Colour?4:0);
        // The temporary exact records are bounded and discarded after each
        // LOD; this proof is never evaluated by the per-tick contact cache.
        if((int64(Count)+int64(Indices.Num()))*CornerSize>64*1024*1024)return Reject(TEXT("canonical corner memory budget"));
        TArray<TArray<uint8>> Corners;Corners.SetNum(Count);
        for(uint32 V=0;V<Count;++V)
        {
            auto& C=Corners[V];C.Reserve(int32(CornerSize));CornerBytesV82(C,VB.PositionVertexBuffer.VertexPosition(V));
            const FVector4f N=VB.StaticMeshVertexBuffer.VertexTangentZ(V);CornerBytesV82(C,FVector3f(N.X,N.Y,N.Z));
            for(uint32 UV=0;UV<UVs;++UV)CornerBytesV82(C,VB.StaticMeshVertexBuffer.GetVertexUV(V,UV));
            if(Colour)CornerBytesV82(C,VB.ColorVertexBuffer.VertexColor(V));
            if(C.Num()!=CornerSize)return Reject(TEXT("canonical corner byte layout"));
        }
        Triangles.Bytes(L);Triangles.Bytes(Count);Triangles.Bytes(UVs);Triangles.Bytes(Colour);Triangles.Bytes(Indices.Num());Triangles.Bytes(R.Sections.Num());
        Surface.Bytes(L);Surface.Bytes(UVs);Surface.Bytes(Colour);Surface.Bytes(Indices.Num());Surface.Bytes(R.Sections.Num());
        Vertices.Bytes(L);Vertices.Bytes(Count);Vertices.Bytes(UVs);Vertices.Bytes(Colour);
        Sections.Bytes(L);Sections.Bytes(R.Sections.Num());TBitArray<> Covered(false,Indices.Num());TArray<TSharedPtr<FJsonValue>> SectionRows;
        for(int32 SIndex=0;SIndex<R.Sections.Num();++SIndex)
        {
            const auto& S=R.Sections[SIndex];const int64 End=int64(S.FirstIndex)+int64(S.NumTriangles)*3;
            if(S.FirstIndex%3||End>Indices.Num())return Reject(TEXT("canonical section/index range"));
            Triangles.Bytes(SIndex);Triangles.Bytes(S.MaterialIndex);Triangles.Bytes(S.NumTriangles);Triangles.Bytes(S.bEnableCollision);Triangles.Bytes(S.bCastShadow);
            Surface.Bytes(SIndex);Surface.Bytes(S.MaterialIndex);Surface.Bytes(S.NumTriangles);Surface.Bytes(S.bEnableCollision);Surface.Bytes(S.bCastShadow);
            Sections.Bytes(SIndex);Sections.Bytes(S.MaterialIndex);Sections.Bytes(S.NumTriangles);Sections.Bytes(S.bEnableCollision);Sections.Bytes(S.bCastShadow);
            TArray<TArray<uint8>> Faces;Faces.Reserve(S.NumTriangles);
            for(int64 I=S.FirstIndex;I<End;I+=3)
            {
                uint32 Index[3];for(int32 K=0;K<3;++K){if(Covered[int32(I)+K])return Reject(TEXT("canonical overlapping sections"));Covered[int32(I)+K]=true;Index[K]=Indices[int32(I)+K];if(Index[K]>=Count)return Reject(TEXT("canonical invalid index"));}
                int32 Start=0;for(int32 Candidate=1;Candidate<3;++Candidate)
                {
                    for(int32 K=0;K<3;++K){const int32 C=CompareCornerBytesV82(Corners[Index[(Candidate+K)%3]],Corners[Index[(Start+K)%3]]);if(C<0){Start=Candidate;break;}if(C>0)break;}
                }
                auto& Face=Faces.AddDefaulted_GetRef();Face.Reserve(int32(CornerSize*3));
                for(int32 K=0;K<3;++K)Face.Append(Corners[Index[(Start+K)%3]]);
            }
            Faces.Sort([](const TArray<uint8>& A,const TArray<uint8>& B){return CompareCornerBytesV82(A,B)<0;});
            for(const auto& Face:Faces){HashCornerRecordV82(Triangles,Face);HashCornerRecordV82(Surface,Face);}
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("section"),SIndex);Row->SetNumberField(TEXT("materialIndex"),S.MaterialIndex);Row->SetNumberField(TEXT("triangles"),S.NumTriangles);
            Row->SetBoolField(TEXT("collision"),S.bEnableCollision);Row->SetBoolField(TEXT("castShadow"),S.bCastShadow);SectionRows.Add(MakeShared<FJsonValueObject>(Row));
        }
        for(int32 I=0;I<Covered.Num();++I)if(!Covered[I])return Reject(TEXT("canonical unassigned index"));
        Corners.Sort([](const TArray<uint8>& A,const TArray<uint8>& B){return CompareCornerBytesV82(A,B)<0;});
        for(const auto& Corner:Corners)HashCornerRecordV82(Vertices,Corner);
        UniqueAttributes.Bytes(L);UniqueAttributes.Bytes(UVs);UniqueAttributes.Bytes(Colour);int32 UniqueCount=0;
        for(int32 I=0;I<Corners.Num();++I)if(I==0||CompareCornerBytesV82(Corners[I],Corners[I-1])!=0)++UniqueCount;
        UniqueAttributes.Bytes(UniqueCount);for(int32 I=0;I<Corners.Num();++I)if(I==0||CompareCornerBytesV82(Corners[I],Corners[I-1])!=0)HashCornerRecordV82(UniqueAttributes,Corners[I]);
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("lod"),L);Row->SetNumberField(TEXT("vertices"),Count);Row->SetNumberField(TEXT("indices"),Indices.Num());Row->SetNumberField(TEXT("uvChannels"),UVs);
        Row->SetNumberField(TEXT("vertexColourCount"),VB.ColorVertexBuffer.GetNumVertices());Row->SetArrayField(TEXT("sections"),SectionRows);LODs.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetStringField(TEXT("orientedTriangleCornerSHA1"),Triangles.Finish());J->SetStringField(TEXT("vertexAttributeMultisetSHA1"),Vertices.Finish());J->SetStringField(TEXT("sectionsSHA1"),Sections.Finish());
    // Additive diagnosis only: these fields distinguish identical duplicate
    // collapse from a change to the real oriented surface. Strict hashes and
    // counts above remain unchanged and continue to gate existing tools.
    J->SetStringField(TEXT("orientedTriangleSurfaceSHA1"),Surface.Finish());J->SetStringField(TEXT("uniqueVertexAttributeSetSHA1"),UniqueAttributes.Finish());
    J->SetArrayField(TEXT("lods"),LODs);J->SetBoolField(TEXT("success"),true);return J;
}
#endif
TSharedPtr<FJsonObject> RuntimeStaticMeshSnapshot(UStaticMesh* Mesh)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);
    J->SetStringField(TEXT("schemaVersion"),TEXT("v82.german.static.runtime.snapshot.1"));
    const auto Reject=[&](const TCHAR* Why){J->SetStringField(TEXT("reason"),Why);return J;};
    const auto* Render=Mesh?Mesh->GetRenderData():nullptr;const auto* Setup=Mesh?Mesh->GetBodySetup():nullptr;
    if(!Render||!Setup||Render->LODResources.IsEmpty()||Render->LODResources.Num()>8)return Reject(TEXT("runtime mesh/body/LOD missing"));
    FRuntimeHashV82 Geometry,NonBasis,Normals;int64 TotalVertices=0,TotalIndices=0;TArray<TSharedPtr<FJsonValue>> LODs;
    for(int32 L=0;L<Render->LODResources.Num();++L)
    {
        const auto& R=Render->LODResources[L];const auto& VB=R.VertexBuffers;const uint32 Count=VB.PositionVertexBuffer.GetNumVertices(),UVs=VB.StaticMeshVertexBuffer.GetNumTexCoords();
        const auto Indices=R.IndexBuffer.GetArrayView();TotalVertices+=Count;TotalIndices+=Indices.Num();
        if(!Count||UVs>8||TotalVertices>2000000||TotalIndices>6000000||!Indices.Num()||Indices.Num()%3
            ||!VB.PositionVertexBuffer.GetVertexData()||!VB.StaticMeshVertexBuffer.GetTangentData()||!VB.StaticMeshVertexBuffer.GetTexCoordData()
            ||(VB.ColorVertexBuffer.GetNumVertices()&&(!VB.ColorVertexBuffer.GetVertexData()||VB.ColorVertexBuffer.GetNumVertices()!=Count)))return Reject(TEXT("runtime CPU streams unavailable or bounded budget exceeded"));
        Geometry.Bytes(Count);Geometry.Bytes(UVs);NonBasis.Bytes(Count);NonBasis.Bytes(UVs);Normals.Bytes(Count);
        for(uint32 V=0;V<Count;++V)
        {
            const FVector3f P=VB.PositionVertexBuffer.VertexPosition(V);const FVector4f X=VB.StaticMeshVertexBuffer.VertexTangentX(V),Z=VB.StaticMeshVertexBuffer.VertexTangentZ(V);
            const FVector3f Y=VB.StaticMeshVertexBuffer.VertexTangentY(V);
            if(P.ContainsNaN()||X.ContainsNaN()||Y.ContainsNaN()||Z.ContainsNaN())return Reject(TEXT("runtime nonfinite vertex/tangent"));
            Geometry.Bytes(P);Geometry.Bytes(X);Geometry.Bytes(Y);Geometry.Bytes(Z);NonBasis.Bytes(P);Normals.Bytes(FVector3f(Z.X,Z.Y,Z.Z));
            for(uint32 UV=0;UV<UVs;++UV){const FVector2f T=VB.StaticMeshVertexBuffer.GetVertexUV(V,UV);if(T.ContainsNaN())return Reject(TEXT("runtime nonfinite UV"));Geometry.Bytes(T);NonBasis.Bytes(T);}
            if(VB.ColorVertexBuffer.GetNumVertices()){Geometry.Bytes(VB.ColorVertexBuffer.VertexColor(V));NonBasis.Bytes(VB.ColorVertexBuffer.VertexColor(V));}
        }
        Geometry.Bytes(Indices.Num());NonBasis.Bytes(Indices.Num());for(int32 I=0;I<Indices.Num();++I){const uint32 Index=Indices[I];if(Index>=Count)return Reject(TEXT("runtime index outside actual stream"));Geometry.Bytes(Index);NonBasis.Bytes(Index);}
        for(const auto& S:R.Sections){Geometry.Bytes(S.MaterialIndex);Geometry.Bytes(S.FirstIndex);Geometry.Bytes(S.NumTriangles);Geometry.Bytes(S.MinVertexIndex);Geometry.Bytes(S.MaxVertexIndex);Geometry.Bytes(S.bEnableCollision);Geometry.Bytes(S.bCastShadow);
            NonBasis.Bytes(S.MaterialIndex);NonBasis.Bytes(S.FirstIndex);NonBasis.Bytes(S.NumTriangles);NonBasis.Bytes(S.MinVertexIndex);NonBasis.Bytes(S.MaxVertexIndex);NonBasis.Bytes(S.bEnableCollision);NonBasis.Bytes(S.bCastShadow);}
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("vertices"),Count);Row->SetNumberField(TEXT("indices"),Indices.Num());Row->SetNumberField(TEXT("uvChannels"),UVs);
        Row->SetNumberField(TEXT("vertexColourCount"),VB.ColorVertexBuffer.GetNumVertices());Row->SetNumberField(TEXT("sections"),R.Sections.Num());Row->SetBoolField(TEXT("fullPrecisionUV"),VB.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());LODs.Add(MakeShared<FJsonValueObject>(Row));
    }
    FRuntimeHashV82 Collision;const auto& A=Setup->AggGeom;
    if(A.GetElementCount()>512||!A.LevelSetElems.IsEmpty()||!A.SkinnedLevelSetElems.IsEmpty()||!A.MLLevelSetElems.IsEmpty()||!A.SkinnedTriangleMeshElems.IsEmpty())return Reject(TEXT("runtime unaccepted collision shape family/budget"));
    Collision.Bytes(Setup->BodySetupGuid.A);Collision.Bytes(Setup->BodySetupGuid.B);Collision.Bytes(Setup->BodySetupGuid.C);Collision.Bytes(Setup->BodySetupGuid.D);
    FRuntimeHashV82 Recipe;FCollisionRecipeHashV82 RecipeFields{Collision,Recipe};
    RecipeFields.Bytes(uint8(Setup->CollisionTraceFlag));RecipeFields.Bytes(uint8(Setup->bDoubleSidedGeometry));RecipeFields.Bytes(uint8(Setup->bGenerateNonMirroredCollision));RecipeFields.Bytes(uint8(Setup->bGenerateMirroredCollision));
    RecipeFields.Text(Setup->PhysMaterial?Setup->PhysMaterial->GetPathName():FString());RecipeFields.Bytes(Mesh->LODForCollision);RecipeFields.Vector(Setup->BuildScale3D);
    RecipeFields.Bytes(uint8(Setup->WalkableSlopeOverride.GetWalkableSlopeBehavior()));RecipeFields.Number(Setup->WalkableSlopeOverride.GetWalkableSlopeAngle());
    RecipeFields.Text(Setup->DefaultInstance.GetCollisionProfileName().ToString());RecipeFields.Bytes(uint8(Setup->DefaultInstance.GetCollisionEnabled(false)));RecipeFields.Bytes(uint8(Setup->DefaultInstance.GetObjectType()));
    for(int32 C=0;C<int32(ECC_OverlapAll_Deprecated);++C)RecipeFields.Bytes(uint8(Setup->DefaultInstance.GetResponseToChannel(ECollisionChannel(C))));
    RecipeFields.Bytes(A.SphereElems.Num());for(const auto& E:A.SphereElems){RecipeFields.Shape(E);RecipeFields.Number(E.Radius);}
    RecipeFields.Bytes(A.BoxElems.Num());for(const auto& E:A.BoxElems){RecipeFields.Shape(E);RecipeFields.Number(E.X);RecipeFields.Number(E.Y);RecipeFields.Number(E.Z);}
    RecipeFields.Bytes(A.SphylElems.Num());for(const auto& E:A.SphylElems){RecipeFields.Shape(E);RecipeFields.Number(E.Radius);RecipeFields.Number(E.Length);}
    RecipeFields.Bytes(A.TaperedCapsuleElems.Num());for(const auto& E:A.TaperedCapsuleElems){RecipeFields.Shape(E);RecipeFields.Number(E.Radius0);RecipeFields.Number(E.Radius1);RecipeFields.Number(E.Length);RecipeFields.Number(E.Width);RecipeFields.Bytes(uint8(E.bOneSidedCollision));}
    int64 ConvexVertices=0;RecipeFields.Bytes(A.ConvexElems.Num());for(const auto& E:A.ConvexElems)
    {
        ConvexVertices+=E.VertexData.Num();if(E.VertexData.IsEmpty()||ConvexVertices>262144)return Reject(TEXT("runtime convex data unavailable/budget"));
        RecipeFields.Shape(E);RecipeFields.Bytes(E.VertexData.Num());for(const auto& P:E.VertexData)RecipeFields.Vector(P);
    }
    TArray<TSharedPtr<FJsonValue>> Materials;for(const auto& S:Mesh->GetStaticMaterials())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("materialSlotName"),S.MaterialSlotName.ToString());Row->SetStringField(TEXT("objectPath"),S.MaterialInterface?S.MaterialInterface->GetPathName():FString());
        Row->SetStringField(TEXT("overlayObjectPath"),S.OverlayMaterialInterface?S.OverlayMaterialInterface->GetPathName():FString());Materials.Add(MakeShared<FJsonValueObject>(Row));
    }
    const FString GeometryPin=Geometry.Finish(),CollisionPin=Collision.Finish();if(GeometryPin.IsEmpty()||CollisionPin.IsEmpty()||Materials.IsEmpty())return Reject(TEXT("runtime nonfinite collision/empty material"));
    J->SetStringField(TEXT("geometryUVSectionSHA1"),GeometryPin);J->SetStringField(TEXT("positionIndexUVColourSectionSHA1"),NonBasis.Finish());J->SetStringField(TEXT("normalXYZSHA1"),Normals.Finish());
    J->SetStringField(TEXT("collisionRuntimeSHA1"),CollisionPin);J->SetArrayField(TEXT("materialBindings"),Materials);J->SetArrayField(TEXT("lods"),LODs);
    J->SetStringField(TEXT("collisionRecipeSHA1"),Recipe.Finish());J->SetStringField(TEXT("bodySetupGuid"),Setup->BodySetupGuid.ToString(EGuidFormats::Digits));
    J->SetBoolField(TEXT("allowCPUAccess"),Mesh->bAllowCPUAccess);
#if WITH_EDITOR
    J->SetObjectField(TEXT("editorCanonicalCornerProof"),EditorCanonicalCornerProofV82(Render));
#endif
    J->SetBoolField(TEXT("success"),true);return J;
}
#if WITH_EDITOR
FString SourceNonBasisPinV82(const FMeshDescription& D)
{
    FStaticMeshConstAttributes A(D);FRuntimeHashV82 H;const auto P=A.GetVertexPositions();const auto N=A.GetVertexInstanceNormals();
    const auto UV=A.GetVertexInstanceUVs();const auto C=A.GetVertexInstanceColors();const auto Hard=A.GetEdgeHardnesses();const auto Slots=A.GetPolygonGroupMaterialSlotNames();
    H.Bytes(D.Vertices().Num());for(const auto ID:D.Vertices().GetElementIDs()){H.Bytes(ID.GetValue());H.Bytes(P[ID]);}
    H.Bytes(D.VertexInstances().Num());H.Bytes(UV.GetNumChannels());for(const auto ID:D.VertexInstances().GetElementIDs())
    {H.Bytes(ID.GetValue());H.Bytes(D.GetVertexInstanceVertex(ID).GetValue());H.Bytes(N[ID]);for(int32 K=0;K<UV.GetNumChannels();++K)H.Bytes(UV.Get(ID,K));H.Bytes(C[ID]);}
    H.Bytes(D.Triangles().Num());for(const auto ID:D.Triangles().GetElementIDs())
    {H.Bytes(ID.GetValue());H.Bytes(D.GetTrianglePolygonGroup(ID).GetValue());for(const auto VI:D.GetTriangleVertexInstances(ID))H.Bytes(VI.GetValue());}
    H.Bytes(D.PolygonGroups().Num());for(const auto ID:D.PolygonGroups().GetElementIDs()){H.Bytes(ID.GetValue());H.Text(Slots[ID].ToString());}
    H.Bytes(D.Edges().Num());for(const auto ID:D.Edges().GetElementIDs()){H.Bytes(ID.GetValue());H.Bytes(Hard[ID]);}
    return H.Finish();
}
FString HealthySourceBasisPinV82(const FMeshDescription& D,const TSet<int32>& Invalid)
{
    FStaticMeshConstAttributes A(D);FRuntimeHashV82 H;const auto T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();
    for(const auto ID:D.VertexInstances().GetElementIDs())if(!Invalid.Contains(ID.GetValue())){H.Bytes(ID.GetValue());H.Bytes(T[ID]);H.Bytes(S[ID]);}return H.Finish();
}
// Cold Editor diagnosis only. Position/winding is matched independently of
// normal packing and vertex welding; none of these measurements accepts a
// candidate or changes the strict import/contact gates.
struct FSurfacePairFaceV82
{
    uint32 Vertices[3]{};uint32 Triangle=0;uint8 PositionKey[36]{};
};
TSharedPtr<FJsonObject> CollisionPairFieldsV82(UStaticMesh* Mesh)
{
    auto J=MakeShared<FJsonObject>();const auto* Setup=Mesh?Mesh->GetBodySetup():nullptr;if(!Setup)return J;
    J->SetNumberField(TEXT("traceFlag"),uint8(Setup->CollisionTraceFlag));J->SetBoolField(TEXT("doubleSided"),Setup->bDoubleSidedGeometry);
    J->SetBoolField(TEXT("nonMirrored"),Setup->bGenerateNonMirroredCollision);J->SetBoolField(TEXT("mirrored"),Setup->bGenerateMirroredCollision);
    J->SetStringField(TEXT("physicalMaterial"),Setup->PhysMaterial?Setup->PhysMaterial->GetPathName():FString());
    J->SetNumberField(TEXT("lodForCollision"),Mesh->LODForCollision);TArray<TSharedPtr<FJsonValue>> Scale;
    Scale.Add(MakeShared<FJsonValueNumber>(Setup->BuildScale3D.X));Scale.Add(MakeShared<FJsonValueNumber>(Setup->BuildScale3D.Y));Scale.Add(MakeShared<FJsonValueNumber>(Setup->BuildScale3D.Z));J->SetArrayField(TEXT("buildScale3D"),Scale);
    J->SetNumberField(TEXT("walkableSlopeBehavior"),uint8(Setup->WalkableSlopeOverride.GetWalkableSlopeBehavior()));J->SetNumberField(TEXT("walkableSlopeAngle"),Setup->WalkableSlopeOverride.GetWalkableSlopeAngle());
    J->SetStringField(TEXT("profile"),Setup->DefaultInstance.GetCollisionProfileName().ToString());J->SetNumberField(TEXT("enabled"),uint8(Setup->DefaultInstance.GetCollisionEnabled(false)));J->SetNumberField(TEXT("objectType"),uint8(Setup->DefaultInstance.GetObjectType()));
    TArray<TSharedPtr<FJsonValue>> Responses;for(int32 Channel=0;Channel<int32(ECC_OverlapAll_Deprecated);++Channel)Responses.Add(MakeShared<FJsonValueNumber>(uint8(Setup->DefaultInstance.GetResponseToChannel(ECollisionChannel(Channel)))));
    J->SetArrayField(TEXT("responses"),Responses);J->SetNumberField(TEXT("sphereCount"),Setup->AggGeom.SphereElems.Num());J->SetNumberField(TEXT("boxCount"),Setup->AggGeom.BoxElems.Num());J->SetNumberField(TEXT("capsuleCount"),Setup->AggGeom.SphylElems.Num());J->SetNumberField(TEXT("convexCount"),Setup->AggGeom.ConvexElems.Num());
    J->SetStringField(TEXT("bodySetupGuid"),Setup->BodySetupGuid.ToString(EGuidFormats::Digits));return J;
}
TSharedPtr<FJsonObject> EditorSurfacePairDiagnosisV82(UStaticMesh* Source,UStaticMesh* Candidate)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schemaVersion"),TEXT("v82.german.ingress.surface.pair.1"));J->SetBoolField(TEXT("success"),false);
    J->SetBoolField(TEXT("mutationAttempted"),false);J->SetBoolField(TEXT("comparisonExecuted"),false);J->SetBoolField(TEXT("candidateAccepted"),false);
    const auto Reject=[&](const TCHAR* Why){J->SetStringField(TEXT("reason"),Why);return J;};
    const auto* SourceRender=Source?Source->GetRenderData():nullptr;const auto* CandidateRender=Candidate?Candidate->GetRenderData():nullptr;
    if(!SourceRender||!CandidateRender||SourceRender->LODResources.Num()!=1||CandidateRender->LODResources.Num()!=1)return Reject(TEXT("surface_pair_exact_one_LOD_required"));
    const auto SourceRuntime=RuntimeStaticMeshSnapshot(Source),CandidateRuntime=RuntimeStaticMeshSnapshot(Candidate);
    if(!SourceRuntime->GetBoolField(TEXT("success"))||!CandidateRuntime->GetBoolField(TEXT("success")))return Reject(TEXT("surface_pair_complete_CPU_snapshot_required"));
    J->SetStringField(TEXT("sourceObjectPath"),Source->GetPathName());J->SetStringField(TEXT("candidateObjectPath"),Candidate->GetPathName());
    J->SetObjectField(TEXT("sourceRuntimeSnapshot"),SourceRuntime);J->SetObjectField(TEXT("candidateRuntimeSnapshot"),CandidateRuntime);
    J->SetObjectField(TEXT("sourceCollisionFields"),CollisionPairFieldsV82(Source));J->SetObjectField(TEXT("candidateCollisionFields"),CollisionPairFieldsV82(Candidate));
    const auto& SR=SourceRender->LODResources[0];const auto& CR=CandidateRender->LODResources[0];const auto& SV=SR.VertexBuffers;const auto& CV=CR.VertexBuffers;
    const auto SI=SR.IndexBuffer.GetArrayView(),CI=CR.IndexBuffer.GetArrayView();const uint32 UVs=SV.StaticMeshVertexBuffer.GetNumTexCoords();
    const bool Colour=SV.ColorVertexBuffer.GetNumVertices()!=0;
    if(SR.Sections.Num()!=CR.Sections.Num()||UVs!=CV.StaticMeshVertexBuffer.GetNumTexCoords()||Colour!=(CV.ColorVertexBuffer.GetNumVertices()!=0)||SI.Num()!=CI.Num())return Reject(TEXT("surface_pair_section_index_UV_colour_layout_differs"));
    if(int64(SI.Num()+CI.Num())/3*int64(sizeof(FSurfacePairFaceV82))>64*1024*1024)return Reject(TEXT("surface_pair_temporary_memory_budget"));
    const auto JsonVector=[](const FVector3f& V){TArray<TSharedPtr<FJsonValue>> A;A.Add(MakeShared<FJsonValueNumber>(V.X));A.Add(MakeShared<FJsonValueNumber>(V.Y));A.Add(MakeShared<FJsonValueNumber>(V.Z));return A;};
    const auto JsonUV=[](const FVector2f& V){TArray<TSharedPtr<FJsonValue>> A;A.Add(MakeShared<FJsonValueNumber>(V.X));A.Add(MakeShared<FJsonValueNumber>(V.Y));return A;};
    const auto Normal=[](const FStaticMeshLODResources& R,uint32 V){const FVector4f N=R.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(V);return FVector3f(N.X,N.Y,N.Z);};
    const auto AttributeCompare=[&](const FStaticMeshLODResources& R,uint32 A,uint32 B)
    {
        const auto& Buffers=R.VertexBuffers;const FVector3f NA=Normal(R,A),NB=Normal(R,B);int32 Cmp=FMemory::Memcmp(&NA,&NB,sizeof(NA));if(Cmp)return Cmp;
        for(uint32 U=0;U<UVs;++U){const FVector2f UA=Buffers.StaticMeshVertexBuffer.GetVertexUV(A,U),UB=Buffers.StaticMeshVertexBuffer.GetVertexUV(B,U);Cmp=FMemory::Memcmp(&UA,&UB,sizeof(UA));if(Cmp)return Cmp;}
        if(Colour){const FColor CA=Buffers.ColorVertexBuffer.VertexColor(A),CB=Buffers.ColorVertexBuffer.VertexColor(B);return FMemory::Memcmp(&CA,&CB,sizeof(CA));}return 0;
    };
    const auto Faces=[&](const FStaticMeshLODResources& R,const FStaticMeshSection& S,TArray<FSurfacePairFaceV82>& Out,TBitArray<>& Covered)
    {
        const auto Indices=R.IndexBuffer.GetArrayView();const uint32 Count=R.VertexBuffers.PositionVertexBuffer.GetNumVertices();const int64 End=int64(S.FirstIndex)+int64(S.NumTriangles)*3;
        if(S.FirstIndex%3||End>Indices.Num())return false;Out.Reserve(S.NumTriangles);
        for(int64 Index=S.FirstIndex;Index<End;Index+=3)
        {
            uint32 Vertices[3];for(int32 K=0;K<3;++K){if(Covered[int32(Index)+K])return false;Covered[int32(Index)+K]=true;Vertices[K]=Indices[int32(Index)+K];if(Vertices[K]>=Count)return false;}
            int32 Start=0;for(int32 Choice=1;Choice<3;++Choice)for(int32 K=0;K<3;++K)
            {
                const FVector3f PA=R.VertexBuffers.PositionVertexBuffer.VertexPosition(Vertices[(Choice+K)%3]),PB=R.VertexBuffers.PositionVertexBuffer.VertexPosition(Vertices[(Start+K)%3]);
                const int32 Cmp=FMemory::Memcmp(&PA,&PB,sizeof(PA));if(Cmp<0){Start=Choice;break;}if(Cmp>0)break;
            }
            auto& Face=Out.AddDefaulted_GetRef();Face.Triangle=uint32(Index/3);
            for(int32 K=0;K<3;++K){Face.Vertices[K]=Vertices[(Start+K)%3];const FVector3f P=R.VertexBuffers.PositionVertexBuffer.VertexPosition(Face.Vertices[K]);FMemory::Memcpy(Face.PositionKey+K*12,&P,12);}
        }
        Out.Sort([&](const FSurfacePairFaceV82& A,const FSurfacePairFaceV82& B)
        {const int32 Cmp=FMemory::Memcmp(A.PositionKey,B.PositionKey,36);if(Cmp)return Cmp<0;for(int32 K=0;K<3;++K){const int32 AC=AttributeCompare(R,A.Vertices[K],B.Vertices[K]);if(AC)return AC<0;}return A.Triangle<B.Triangle;});return true;
    };
    FRuntimeHashV82 SourcePositions,CandidatePositions;SourcePositions.Bytes(SR.Sections.Num());CandidatePositions.Bytes(CR.Sections.Num());
    TBitArray<> SourceCovered(false,SI.Num()),CandidateCovered(false,CI.Num());bool GeometryExact=true,SectionsExact=true,ColoursExact=true;
    int64 MatchedCorners=0,ChangedCorners=0,ChangedNormals=0,ChangedUVs=0,ChangedColours=0,SkippedAmbiguousCorners=0;int32 AmbiguousPositionGroups=0;
    double MaxNormalAngle=0,MaxNormalComponent=0;int32 MaxColourComponent=0;TArray<double> MaxUV;MaxUV.Init(0,UVs);TArray<TSharedPtr<FJsonValue>> Differences;
    // Actual cold readback showed only one-ULP near-zero UV1 residues. Keep
    // this independently measured defect class separate from exact hashes;
    // UV0, ordinary UV1 values, normals and colours are never tolerated.
    bool NearZeroUV1RoundoffOnly=true;int32 RoundoffUVComponents=0;uint64 MaximumChangedUVULPs=0;
    for(int32 Section=0;Section<SR.Sections.Num();++Section)
    {
        const auto& SS=SR.Sections[Section];const auto& CS=CR.Sections[Section];
        const auto HashSection=[&](FRuntimeHashV82& H,const FStaticMeshSection& S){H.Bytes(Section);H.Bytes(S.MaterialIndex);H.Bytes(S.NumTriangles);H.Bytes(S.bEnableCollision);H.Bytes(S.bCastShadow);};
        HashSection(SourcePositions,SS);HashSection(CandidatePositions,CS);
        SectionsExact&=SS.MaterialIndex==CS.MaterialIndex&&SS.NumTriangles==CS.NumTriangles&&SS.bEnableCollision==CS.bEnableCollision&&SS.bCastShadow==CS.bCastShadow;
        TArray<FSurfacePairFaceV82> SF,CF;if(!Faces(SR,SS,SF,SourceCovered)||!Faces(CR,CS,CF,CandidateCovered))return Reject(TEXT("surface_pair_invalid_section_closure"));
        for(const auto& F:SF)SourcePositions.Hash.Update(F.PositionKey,36);for(const auto& F:CF)CandidatePositions.Hash.Update(F.PositionKey,36);
        if(SF.Num()!=CF.Num()){GeometryExact=false;continue;}
        for(int32 F=0;F<SF.Num();++F)
        {
            const bool DuplicateBefore=F>0&&FMemory::Memcmp(SF[F].PositionKey,SF[F-1].PositionKey,36)==0;
            const bool DuplicateAfter=F+1<SF.Num()&&FMemory::Memcmp(SF[F].PositionKey,SF[F+1].PositionKey,36)==0;
            const bool CandidateDuplicate=(F>0&&FMemory::Memcmp(CF[F].PositionKey,CF[F-1].PositionKey,36)==0)||(F+1<CF.Num()&&FMemory::Memcmp(CF[F].PositionKey,CF[F+1].PositionKey,36)==0);
            if(!DuplicateBefore&&DuplicateAfter)++AmbiguousPositionGroups;
            if(FMemory::Memcmp(SF[F].PositionKey,CF[F].PositionKey,36)!=0)
            {
                GeometryExact=false;
                if(Differences.Num()<4)
                {
                    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("differenceKind"),TEXT("position_or_winding_face_mismatch"));Row->SetNumberField(TEXT("section"),Section);
                    Row->SetNumberField(TEXT("sourceTriangle"),SF[F].Triangle);Row->SetNumberField(TEXT("candidateTriangle"),CF[F].Triangle);
                    TArray<TSharedPtr<FJsonValue>> OldP,NewP;for(int32 K=0;K<3;++K){OldP.Add(MakeShared<FJsonValueArray>(JsonVector(SV.PositionVertexBuffer.VertexPosition(SF[F].Vertices[K]))));NewP.Add(MakeShared<FJsonValueArray>(JsonVector(CV.PositionVertexBuffer.VertexPosition(CF[F].Vertices[K]))));}
                    Row->SetArrayField(TEXT("sourceOrientedPositions"),OldP);Row->SetArrayField(TEXT("candidateOrientedPositions"),NewP);Row->SetBoolField(TEXT("attributeCorrespondenceValid"),false);Differences.Add(MakeShared<FJsonValueObject>(Row));
                }
                continue;
            }
            // Identical-position duplicate triangles have no unique authored
            // corner correspondence after welding. Their full oriented
            // geometry remains in the exact digest, but they cannot supply
            // evidence for a normal/UV tolerance decision.
            if(DuplicateBefore||DuplicateAfter||CandidateDuplicate||(FMemory::Memcmp(SF[F].PositionKey,SF[F].PositionKey+12,12)==0&&FMemory::Memcmp(SF[F].PositionKey,SF[F].PositionKey+24,12)==0))
            {SkippedAmbiguousCorners+=3;continue;}
            for(int32 K=0;K<3;++K)
            {
                const uint32 SA=SF[F].Vertices[K],CA=CF[F].Vertices[K];const FVector3f SN=Normal(SR,SA),CN=Normal(CR,CA);const bool NChanged=FMemory::Memcmp(&SN,&CN,sizeof(SN))!=0;
                if(SN.SizeSquared()<1.e-12f||CN.SizeSquared()<1.e-12f)return Reject(TEXT("surface_pair_invalid_real_normal"));
                const double NormalAngle=NChanged?FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector3d::DotProduct(FVector3d(SN).GetSafeNormal(),FVector3d(CN).GetSafeNormal()),-1.,1.))):0.;
                const double NormalError=FMath::Max3(FMath::Abs(double(SN.X)-CN.X),FMath::Abs(double(SN.Y)-CN.Y),FMath::Abs(double(SN.Z)-CN.Z));
                MaxNormalAngle=FMath::Max(MaxNormalAngle,NormalAngle);MaxNormalComponent=FMath::Max(MaxNormalComponent,NormalError);ChangedNormals+=NChanged;
                bool UChanged=false;TArray<TSharedPtr<FJsonValue>> OldUV,NewUV,UVError;
                for(uint32 U=0;U<UVs;++U)
                {
                    const FVector2f SU=SV.StaticMeshVertexBuffer.GetVertexUV(SA,U),CU=CV.StaticMeshVertexBuffer.GetVertexUV(CA,U);const double Error=FMath::Max(FMath::Abs(double(SU.X)-CU.X),FMath::Abs(double(SU.Y)-CU.Y));
                    UChanged|=FMemory::Memcmp(&SU,&CU,sizeof(SU))!=0;MaxUV[U]=FMath::Max(MaxUV[U],Error);
                    for(int32 Axis=0;Axis<2;++Axis)
                    {
                        const float OldValue=Axis?SU.Y:SU.X,NewValue=Axis?CU.Y:CU.X;uint32 OldBits=0,NewBits=0;FMemory::Memcpy(&OldBits,&OldValue,4);FMemory::Memcpy(&NewBits,&NewValue,4);
                        if(OldBits!=NewBits)
                        {
                            const uint64 ULPs=uint64(FMath::Abs(int64(OldBits)-int64(NewBits)));MaximumChangedUVULPs=FMath::Max(MaximumChangedUVULPs,ULPs);++RoundoffUVComponents;
                            NearZeroUV1RoundoffOnly&=U==1&&ULPs==1&&FMath::Abs(double(OldValue))<=1./8589934592.&&FMath::Abs(double(NewValue))<=1./8589934592.&&FMath::Abs(double(OldValue)-NewValue)<=1./144115188075855872.;
                        }
                    }
                    if(Differences.Num()<4){OldUV.Add(MakeShared<FJsonValueArray>(JsonUV(SU)));NewUV.Add(MakeShared<FJsonValueArray>(JsonUV(CU)));UVError.Add(MakeShared<FJsonValueNumber>(Error));}
                }
                ChangedUVs+=UChanged;bool CChanged=false;FColor SC=FColor::White,CC=FColor::White;
                if(Colour){SC=SV.ColorVertexBuffer.VertexColor(SA);CC=CV.ColorVertexBuffer.VertexColor(CA);CChanged=SC!=CC;for(int32 Channel=0;Channel<4;++Channel)MaxColourComponent=FMath::Max(MaxColourComponent,FMath::Abs(int32(reinterpret_cast<const uint8*>(&SC)[Channel])-int32(reinterpret_cast<const uint8*>(&CC)[Channel])));}
                ColoursExact&=!CChanged;ChangedColours+=CChanged;++MatchedCorners;const bool Changed=NChanged||UChanged||CChanged;ChangedCorners+=Changed;
                if(Changed&&Differences.Num()<4)
                {
                    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("differenceKind"),TEXT("nonbasis_corner_attributes"));Row->SetBoolField(TEXT("attributeCorrespondenceValid"),true);Row->SetNumberField(TEXT("section"),Section);Row->SetNumberField(TEXT("sourceTriangle"),SF[F].Triangle);Row->SetNumberField(TEXT("candidateTriangle"),CF[F].Triangle);Row->SetNumberField(TEXT("orientedCorner"),K);
                    Row->SetArrayField(TEXT("position"),JsonVector(SV.PositionVertexBuffer.VertexPosition(SA)));Row->SetArrayField(TEXT("sourceNormal"),JsonVector(SN));Row->SetArrayField(TEXT("candidateNormal"),JsonVector(CN));Row->SetNumberField(TEXT("normalAngleDegrees"),NormalAngle);
                    Row->SetArrayField(TEXT("sourceUV"),OldUV);Row->SetArrayField(TEXT("candidateUV"),NewUV);Row->SetArrayField(TEXT("uvAbsoluteError"),UVError);
                    Row->SetStringField(TEXT("sourceColourRGBA"),SC.ToHex());Row->SetStringField(TEXT("candidateColourRGBA"),CC.ToHex());Differences.Add(MakeShared<FJsonValueObject>(Row));
                }
            }
        }
    }
    for(int32 I=0;I<SourceCovered.Num();++I)if(!SourceCovered[I]||!CandidateCovered[I])return Reject(TEXT("surface_pair_unassigned_index"));
    TArray<TSharedPtr<FJsonValue>> UVMaximum;for(const double Error:MaxUV)UVMaximum.Add(MakeShared<FJsonValueNumber>(Error));
    J->SetStringField(TEXT("sourceOrientedPositionWindingSHA1"),SourcePositions.Finish());J->SetStringField(TEXT("candidateOrientedPositionWindingSHA1"),CandidatePositions.Finish());
    const bool CompleteAttributeMatch=GeometryExact&&SkippedAmbiguousCorners==0&&MatchedCorners==SI.Num();
    J->SetBoolField(TEXT("positionsWindingFaceMultiplicityExact"),GeometryExact);J->SetBoolField(TEXT("sectionsMaterialCollisionShadowExact"),SectionsExact);J->SetBoolField(TEXT("coloursExact"),ColoursExact&&CompleteAttributeMatch);J->SetBoolField(TEXT("comparedColoursExact"),ColoursExact);
    J->SetBoolField(TEXT("collisionRecipeExact"),SourceRuntime->GetStringField(TEXT("collisionRecipeSHA1"))==CandidateRuntime->GetStringField(TEXT("collisionRecipeSHA1")));
    J->SetNumberField(TEXT("matchedCorners"),double(MatchedCorners));J->SetNumberField(TEXT("changedCorners"),double(ChangedCorners));J->SetNumberField(TEXT("changedNormalCorners"),double(ChangedNormals));J->SetNumberField(TEXT("changedUVCorners"),double(ChangedUVs));J->SetNumberField(TEXT("changedColourCorners"),double(ChangedColours));
    J->SetNumberField(TEXT("skippedAmbiguousCorners"),double(SkippedAmbiguousCorners));J->SetBoolField(TEXT("allAttributeCornersUnambiguouslyCompared"),CompleteAttributeMatch);J->SetStringField(TEXT("attributeMetricScope"),TEXT("only unambiguous exact-position oriented corner matches; global proof requires complete match"));
    J->SetBoolField(TEXT("onlyNearZeroUV1OneULPRoundoff"),CompleteAttributeMatch&&NearZeroUV1RoundoffOnly&&ChangedNormals==0&&ChangedColours==0);
    J->SetNumberField(TEXT("changedNearZeroUV1Components"),RoundoffUVComponents);J->SetNumberField(TEXT("maximumChangedUVULPs"),double(MaximumChangedUVULPs));
    J->SetNumberField(TEXT("nearZeroUV1AbsoluteLimit"),1./144115188075855872.);J->SetNumberField(TEXT("nearZeroUV1MagnitudeLimit"),1./8589934592.);
    J->SetNumberField(TEXT("ambiguousDuplicatePositionGroups"),AmbiguousPositionGroups);J->SetNumberField(TEXT("maximumNormalAngleDegrees"),MaxNormalAngle);J->SetNumberField(TEXT("maximumNormalComponentError"),MaxNormalComponent);J->SetNumberField(TEXT("maximumColourChannelError"),MaxColourComponent);
    J->SetArrayField(TEXT("maximumUVAbsoluteErrorPerChannel"),UVMaximum);J->SetArrayField(TEXT("firstActualDifferences"),Differences);J->SetBoolField(TEXT("comparisonExecuted"),true);J->SetBoolField(TEXT("success"),true);return J;
}
TSharedPtr<FJsonObject> RepairSourceInvalidBasisCandidateV82(UStaticMesh* Source,UStaticMesh* Candidate)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schemaVersion"),TEXT("v82.german.ingress.source.basis.candidate.1"));
    J->SetBoolField(TEXT("success"),false);J->SetBoolField(TEXT("mutationAttempted"),false);J->SetBoolField(TEXT("repairEligible"),false);
    const auto Reject=[&](const TCHAR* Why){J->SetStringField(TEXT("reason"),Why);return J;};
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)||!IsInGameThread())return Reject(TEXT("source_basis_wrong_receiver_or_thread"));
    FString Token,Part;for(const TCHAR* K:{TEXT("fr_matz"),TEXT("fr_champagne_defence"),TEXT("fr_soissons"),TEXT("fr_sommepy")})for(const TCHAR* Kind:{TEXT("Ingress_"),TEXT("IngressBackdrop_")})
    {const FString Name=TEXT("SM_DE_")+FString(Kind)+K+TEXT("_V82");const FString Path=FString(ReceiverRoot)+TEXT("Ingress/")+Name+TEXT(".")+Name;if(Source&&Source->GetPathName()==Path){Token=K;Part=Kind;}}
    if(Token.IsEmpty()||!Source||Source->GetOutermost()->IsDirty()||Source->GetNumSourceModels()!=1||!Source->bAllowCPUAccess)return Reject(TEXT("source_basis_exact_saved_source_missing"));
    const FString AuthoredName=TEXT("SM_DE_")+Part+Token+TEXT("_AuthoredBasisV1");
    if(Candidate&&Candidate!=Source&&Candidate->GetPathName()==FString(ReceiverRoot)+TEXT("IngressAuthoredBasisV1/")+AuthoredName+TEXT(".")+AuthoredName)
        return EditorSurfacePairDiagnosisV82(Source,Candidate);
    const auto& Settings=Source->GetSourceModel(0).BuildSettings;
    if(Settings.bRecomputeNormals||Settings.bRecomputeTangents||Settings.bRemoveDegenerates||Settings.BuildScale3D!=FVector(1,1,1))return Reject(TEXT("source_basis_source_build_contract"));
    const FMeshDescription* D=Source->GetMeshDescription(0);const auto* Render=Source->GetRenderData();
    if(!D||!Render||Render->LODResources.Num()!=1||D->Vertices().Num()>300000||D->VertexInstances().Num()>900000||D->Triangles().Num()>300000)return Reject(TEXT("source_basis_authored_description_missing_or_budget"));
    FStaticMeshConstAttributes A(*D);const auto P=A.GetVertexPositions();const auto N=A.GetVertexInstanceNormals();const auto T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();
    const auto UV=A.GetVertexInstanceUVs();const auto C=A.GetVertexInstanceColors();
    if(!P.IsValid()||!N.IsValid()||!UV.IsValid()||!C.IsValid()||UV.GetNumChannels()!=2)return Reject(TEXT("source_basis_authored_geometry_attributes_missing"));
    const auto& R=Render->LODResources[0];const auto& VB=R.VertexBuffers;
    if(!VB.PositionVertexBuffer.GetVertexData()||!VB.StaticMeshVertexBuffer.GetTangentData()||!VB.StaticMeshVertexBuffer.GetTexCoordData())return Reject(TEXT("source_basis_actual_render_cpu_missing"));
    TArray<uint32> BadRender;for(uint32 I=0;I<VB.PositionVertexBuffer.GetNumVertices();++I)
    {const FVector4f X=VB.StaticMeshVertexBuffer.VertexTangentX(I);const FVector3f Y=VB.StaticMeshVertexBuffer.VertexTangentY(I);if(FVector3f(X.X,X.Y,X.Z).SizeSquared()<1.e-12f||Y.SizeSquared()<1.e-12f)BadRender.Add(I);}
    int32 BadNormals=0,BadTangents=0,BadSigns=0;TSet<int32> Invalid;TArray<TSharedPtr<FJsonValue>> Samples;
    for(const auto ID:D->VertexInstances().GetElementIDs())
    {
        const FVector3f Normal=N[ID];const FVector3f Tangent=T.IsValid()?T[ID]:FVector3f::ZeroVector;const float Sign=S.IsValid()?S[ID]:0.f;
        const bool InvalidN=Normal.ContainsNaN()||Normal.SizeSquared()<1.e-12f;
        const bool InvalidT=Tangent.ContainsNaN()||Tangent.SizeSquared()<1.e-12f||FVector3f::CrossProduct(Normal,Tangent).SizeSquared()<1.e-12f;
        const bool InvalidS=!FMath::IsFinite(Sign)||FMath::Abs(Sign)<.5f;
        BadNormals+=InvalidN;BadTangents+=InvalidT;BadSigns+=InvalidS;if(InvalidN||InvalidT||InvalidS)Invalid.Add(ID.GetValue());
        if((InvalidN||InvalidT||InvalidS)&&Samples.Num()<8)
        {auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("vertexInstance"),ID.GetValue());Row->SetStringField(TEXT("position"),P[D->GetVertexInstanceVertex(ID)].ToString());Row->SetStringField(TEXT("normal"),Normal.ToString());Row->SetStringField(TEXT("tangent"),Tangent.ToString());Row->SetNumberField(TEXT("binormalSign"),Sign);Samples.Add(MakeShared<FJsonValueObject>(Row));}
    }
    J->SetStringField(TEXT("sourceObjectPath"),Source->GetPathName());J->SetNumberField(TEXT("sourceVertices"),D->Vertices().Num());J->SetNumberField(TEXT("sourceVertexInstances"),D->VertexInstances().Num());J->SetNumberField(TEXT("sourceTriangles"),D->Triangles().Num());
    J->SetNumberField(TEXT("renderInvalidBasisVertices"),BadRender.Num());J->SetNumberField(TEXT("authoredInvalidNormals"),BadNormals);J->SetNumberField(TEXT("authoredInvalidTangents"),BadTangents);J->SetNumberField(TEXT("authoredInvalidBinormalSigns"),BadSigns);J->SetArrayField(TEXT("authoredInvalidSamples"),Samples);
    J->SetObjectField(TEXT("beforeRuntimeSnapshot"),RuntimeStaticMeshSnapshot(Source));J->SetStringField(TEXT("sourceNonBasisSHA1"),SourceNonBasisPinV82(*D));
    const auto Ineligible=[&](const TCHAR* Why){J->SetBoolField(TEXT("success"),true);J->SetBoolField(TEXT("inspected"),true);J->SetStringField(TEXT("ineligibleReason"),Why);return J;};
    if(!BadRender.Num()){J->SetBoolField(TEXT("healthyRenderCopyOnly"),true);return Ineligible(TEXT("healthy_render_no_source_repair_needed"));}
    if(!T.IsValid()||!S.IsValid()||BadTangents==D->VertexInstances().Num())return Ineligible(TEXT("whole_authored_tangent_channel_missing_no_duplicate_or_build"));
    if(BadNormals||BadRender.Num()>32||Invalid.Num()==0||Invalid.Num()>128)return Ineligible(TEXT("authored_invalid_basis_not_a_bounded_render_defect"));
    for(const int32 Value:Invalid)
    {
        const FVertexInstanceID ID(Value);bool Matches=false;const FVector3f Point=P[D->GetVertexInstanceVertex(ID)];
        for(const uint32 V:BadRender)if(Point==VB.PositionVertexBuffer.VertexPosition(V))
        {bool SameUV=true;for(int32 K=0;K<UV.GetNumChannels();++K)SameUV&=UV.Get(ID,K)==VB.StaticMeshVertexBuffer.GetVertexUV(V,K);const FVector4f Colour=C[ID];if(SameUV&&FLinearColor(Colour).ToFColor(true)==VB.ColorVertexBuffer.VertexColor(V)){Matches=true;break;}}
        if(!Matches)return Ineligible(TEXT("authored_invalid_corner_does_not_exactly_match_render_defect"));
    }
    J->SetBoolField(TEXT("repairEligible"),true);J->SetBoolField(TEXT("inspected"),true);J->SetStringField(TEXT("healthyAuthoredBasisSHA1"),HealthySourceBasisPinV82(*D,Invalid));
    if(!Candidate){J->SetBoolField(TEXT("success"),true);return J;}
    const FString Name=TEXT("SM_DE_")+Part+Token+TEXT("_SourceBasisV1");const FString Target=FString(ReceiverRoot)+TEXT("IngressSourceBasisV1/")+Name+TEXT(".")+Name;
    if(Candidate==Source||Candidate->GetPathName()!=Target||Candidate->GetNumSourceModels()!=1)return Reject(TEXT("source_basis_candidate_namespace"));
    auto Before=RuntimeStaticMeshSnapshot(Candidate);J->SetObjectField(TEXT("candidateBeforeRuntimeSnapshot"),Before);
    const auto SourceRuntime=J->GetObjectField(TEXT("beforeRuntimeSnapshot"));
    for(const TCHAR* K:{TEXT("geometryUVSectionSHA1"),TEXT("collisionRuntimeSHA1"),TEXT("materialBindings"),TEXT("lods")})
    {const auto* B=Before->Values.Find(K);const auto* E=SourceRuntime->Values.Find(K);if(!B||!E||!FJsonValue::CompareEqual(**B,**E))return Reject(TEXT("source_basis_duplicate_not_exact_source"));}
    FMeshDescription Repaired=*D;FStaticMeshAttributes Edit(Repaired);auto ET=Edit.GetVertexInstanceTangents();auto ES=Edit.GetVertexInstanceBinormalSigns();Repaired.BuildVertexInstanceIndexers();
    for(const int32 Value:Invalid)
    {
        const FVertexInstanceID ID(Value);const FVector3d Normal=FVector3d(N[ID]).GetSafeNormal();FVector3d ChosenT=FVector3d::ZeroVector,ChosenB=FVector3d::ZeroVector;double Best=0;
        for(const auto Triangle:Repaired.GetVertexInstanceConnectedTriangleIDs(ID))
        {
            const auto VIs=Repaired.GetTriangleVertexInstances(Triangle);const FVector3d P0(P[Repaired.GetVertexInstanceVertex(VIs[0])]),P1(P[Repaired.GetVertexInstanceVertex(VIs[1])]),P2(P[Repaired.GetVertexInstanceVertex(VIs[2])]);
            const FVector2d U0(UV.Get(VIs[0],0)),U1(UV.Get(VIs[1],0)),U2(UV.Get(VIs[2],0));const FVector2d U1D=U1-U0,U2D=U2-U0;const double Det=U1D.X*U2D.Y-U1D.Y*U2D.X;
            if(!FMath::IsFinite(Det)||FMath::Abs(Det)<=FMath::Max(Best,1.e-12))continue;
            const FVector3d RawT=((P1-P0)*U2D.Y-(P2-P0)*U1D.Y)/Det,RawB=((P2-P0)*U1D.X-(P1-P0)*U2D.X)/Det;
            const FVector3d Projected=(RawT-Normal*FVector3d::DotProduct(RawT,Normal)).GetSafeNormal();if(Projected.IsNearlyZero()||RawB.ContainsNaN())continue;
            ChosenT=Projected;ChosenB=RawB;Best=FMath::Abs(Det);
        }
        if(Best==0)return Reject(TEXT("source_basis_no_real_local_uv_derivative"));
        ET[ID]=FVector3f(ChosenT);ES[ID]=FVector3d::DotProduct(FVector3d::CrossProduct(Normal,ChosenT),ChosenB)<0?-1.f:1.f;
    }
    if(SourceNonBasisPinV82(Repaired)!=J->GetStringField(TEXT("sourceNonBasisSHA1"))||HealthySourceBasisPinV82(Repaired,Invalid)!=J->GetStringField(TEXT("healthyAuthoredBasisSHA1")))return Reject(TEXT("source_basis_authored_nonbasis_or_healthy_basis_changed"));
    J->SetBoolField(TEXT("mutationAttempted"),true);Candidate->CreateMeshDescription(0,MoveTemp(Repaired));Candidate->CommitMeshDescription(0);
    TArray<FText> Errors;Candidate->Build(true,&Errors);TArray<TSharedPtr<FJsonValue>> Messages;for(const auto& Error:Errors)Messages.Add(MakeShared<FJsonValueString>(Error.ToString()));J->SetArrayField(TEXT("buildErrors"),Messages);
    const auto After=RuntimeStaticMeshSnapshot(Candidate);J->SetObjectField(TEXT("candidateAfterRuntimeSnapshot"),After);
    if(!Errors.IsEmpty())return Reject(TEXT("source_basis_candidate_build_failed"));
    for(const TCHAR* K:{TEXT("positionIndexUVColourSectionSHA1"),TEXT("normalXYZSHA1"),TEXT("collisionRuntimeSHA1"),TEXT("materialBindings"),TEXT("lods")})
    {const auto* B=Before->Values.Find(K);const auto* E=After->Values.Find(K);if(!B||!E||!FJsonValue::CompareEqual(**B,**E))return Reject(TEXT("source_basis_strict_render_or_collision_preservation_failed"));}
    const auto* ActualDescription=Candidate->GetMeshDescription(0);
    if(!ActualDescription||SourceNonBasisPinV82(*ActualDescription)!=J->GetStringField(TEXT("sourceNonBasisSHA1"))||HealthySourceBasisPinV82(*ActualDescription,Invalid)!=J->GetStringField(TEXT("healthyAuthoredBasisSHA1")))return Reject(TEXT("source_basis_build_changed_authored_data"));
    J->SetNumberField(TEXT("repairedAuthoredCornerCount"),Invalid.Num());J->SetBoolField(TEXT("success"),true);return J;
}
#endif
bool ProductionCandidate()
{return WarfareFRRUGermanHostAcceptanceV82::FormalGermanHostAcceptance&&RuntimeManifestGeneratedV82;}
bool Review()
{
#if WITH_EDITOR
    FString User,Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82GermanHostArmourReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))return false;
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
    return User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase)&&!User.Contains(TEXT(".."));
#else
    return false;
#endif
}
bool Eligible(const AWarfareFrontlineGameMode* B,int32 Side,EFrontlineVehicle Type)
{
    return (Review()||ProductionCandidate())&&B&&WarfareFRRUCampaignV82::Active(B)&&Side==1
        &&WarfareFRRUCampaignV82::Nation(B,Side)==EFRRUNationV81::German
        &&WarfareFRRUCampaignV82::CalendarDate(B)>=19180321
        &&(Type==EFrontlineVehicle::A7V||Type==EFrontlineVehicle::BeuteMarkIV);
}
FString URI(const FString& Name){return FString(ReceiverRoot)+Name+TEXT(".")+Name;}
FString Sha1File(const FString& Path)
{
    TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return {};
    uint8 Digest[FSHA1::DigestSize];FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Digest);return BytesToHex(Digest,FSHA1::DigestSize);
}
bool Json(const FString& Text,TSharedPtr<FJsonObject>& Out)
{return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out)&&Out.IsValid();}
bool Bool(const TSharedPtr<FJsonObject>& J,const TCHAR* Key)
{bool Value=false;return J.IsValid()&&J->TryGetBoolField(Key,Value)&&Value;}
TSharedPtr<FJsonObject> RuntimeRowV82(const FString& Path)
{
    if(!ProductionCandidate())return nullptr;
    static TSharedPtr<FJsonObject> Manifest;
    if(!Manifest&&(!Json(RuntimeManifestJsonV82,Manifest)||!Bool(Manifest,TEXT("generated"))))return nullptr;
    FString Schema;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(!Manifest->TryGetStringField(TEXT("schemaVersion"),Schema)||Schema!=TEXT("v82.german.host.runtime.1")
        ||!Manifest->TryGetArrayField(TEXT("meshes"),Rows)||!Rows||Rows->Num()!=11)return nullptr;
    TSharedPtr<FJsonObject> Selected;for(const auto& Item:*Rows)if(Item)
    {auto Row=Item->AsObject();FString URIValue;if(Row&&Row->TryGetStringField(TEXT("objectPath"),URIValue)&&URIValue==Path){if(Selected)return nullptr;Selected=Row;}}
    return Selected;
}
bool RuntimeMeshV82(UStaticMesh* Mesh,TSharedPtr<FJsonObject>& Actual,FString& Reason)
{
    const auto Row=Mesh?RuntimeRowV82(Mesh->GetPathName()):nullptr;const TSharedPtr<FJsonObject>* Expected=nullptr;
    if(!Row||!Row->TryGetObjectField(TEXT("runtimeStaticMeshSnapshot"),Expected)||!Expected||!(*Expected))
    {Reason=TEXT("german_packaged_runtime_metadata_missing");return false;}
    Actual=RuntimeStaticMeshSnapshot(Mesh);
    if(!Bool(Actual,TEXT("success"))||!Bool(Actual,TEXT("allowCPUAccess")))
    {Reason=TEXT("german_cooked_actual_cpu_streams_missing");return false;}
    for(const TCHAR* Key:{TEXT("geometryUVSectionSHA1"),TEXT("collisionRuntimeSHA1"),TEXT("materialBindings"),TEXT("lods")})
    {
        const auto* A=Actual->Values.Find(Key);const auto* E=(*Expected)->Values.Find(Key);
        if(!A||!E||!(*A)||!(*E)||!FJsonValue::CompareEqual(**A,**E))
        {Reason=TEXT("german_cooked_runtime_stream_collision_material_mismatch");return false;}
    }
    return true;
}
struct FStaticCache
{
    TWeakObjectPtr<UStaticMesh> Mesh;const FStaticMeshLODResources* Resource=nullptr;
    TArray<FVector> Vertices;TArray<FIntVector> Faces;TArray<FVector> OuterSole;
    FBox Bounds{ForceInit};FString PackagePin,GeometryPin,CollisionPin;FGuid LightingGuid;
};
TMap<TWeakObjectPtr<UStaticMesh>,FStaticCache> MeshCaches;
// Proof is generated by the separate, root-owned UE copy job. This reader
// never duplicates a mesh, changes CPU flags, rebuilds or saves an asset.
bool Cache(UStaticMesh* Mesh,FStaticCache*& Out,FString& Reason)
{
    if(!Mesh||!Mesh->bAllowCPUAccess||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.IsEmpty())
    {Reason=TEXT("german_saved_cpu_derivative_missing");return false;}
    const auto* LOD=&Mesh->GetRenderData()->LODResources[0];
    const FString Name=Mesh->GetName();
    if(Mesh->GetOutermost()->IsDirty()){Reason=TEXT("german_saved_mesh_is_dirty");return false;}
    if(auto* Old=MeshCaches.Find(Mesh))if(Old->Resource==LOD&&Old->LightingGuid==Mesh->GetLightingGuid()){Out=Old;return true;}
    FString DuplicatePin;TSharedPtr<FJsonObject> Actual;
    if(!Review())
    {
        if(!RuntimeMeshV82(Mesh,Actual,Reason))return false;
        DuplicatePin=Actual->GetStringField(TEXT("geometryUVSectionSHA1"));
    }
    else
    {
#if WITH_EDITOR
    FString Text;TSharedPtr<FJsonObject> Proof;
    if(!FFileHelper::LoadFileToString(Text,*FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),TEXT("Saved/FRRU/V82GermanHostArmour/mesh-pins.json")))
        ||!Json(Text,Proof)||!Bool(Proof,TEXT("success"))||!Bool(Proof,TEXT("allThreeMatch")))
    {Reason=TEXT("german_exact_saved_three_mesh_proof_missing");return false;}
    FString Project,Schema;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(!Proof->TryGetStringField(TEXT("project"),Project)
        ||!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        ||!Proof->TryGetStringField(TEXT("schemaVersion"),Schema)||Schema!=TEXT("v82.german.host.armour.receiver.1")
        ||!Proof->TryGetArrayField(TEXT("meshes"),Rows)||!Rows||Rows->Num()!=3)
    {Reason=TEXT("german_exact_saved_proof_namespace");return false;}
    TSharedPtr<FJsonObject> Selected;
    for(const auto& Item:*Rows)if(Item.IsValid())
    {
        const auto J=Item->AsObject();FString Path;
        if(J.IsValid()&&J->TryGetStringField(TEXT("objectPath"),Path)&&Path==Mesh->GetPathName())
        {if(Selected){Reason=TEXT("german_ambiguous_mesh_proof");return false;}Selected=J;}
    }
    FString SourcePin,SourcePath;
    const TSharedPtr<FJsonObject>* Source=nullptr;const TSharedPtr<FJsonObject>* Duplicate=nullptr;
    if(!Selected||!Bool(Selected,TEXT("geometryMaterialsCollisionIdentical"))||!Bool(Selected,TEXT("diskReloadVerified"))
        ||!Selected->TryGetStringField(TEXT("duplicatePackageSHA1"),DuplicatePin)
        ||!Selected->TryGetStringField(TEXT("sourcePackageSHA1"),SourcePin)||!Selected->TryGetStringField(TEXT("sourceObjectPath"),SourcePath)
        ||!Selected->TryGetObjectField(TEXT("sourceSnapshot"),Source)||!Source||!(*Source)
        ||!Selected->TryGetObjectField(TEXT("duplicateSnapshot"),Duplicate)||!Duplicate||!(*Duplicate)
        ||!Json(UWarfareFRRUAssetAuditV82::SnapshotStaticMesh(Mesh),Actual)||!Bool(Actual,TEXT("success")))
    {Reason=TEXT("german_actual_stream_proof_missing");return false;}
    if((Name!=TEXT("SM_A7V")&&Name!=TEXT("SM_BeuteMarkIV")&&Name!=TEXT("SM_TrackShoe"))
        ||Mesh->GetPathName()!=URI(Name)||SourcePath!=TEXT("/Game/Warfare/Vehicles/V33/")+Name+TEXT(".")+Name)
    {Reason=TEXT("german_mesh_exact_source_namespace");return false;}
    const FString FrozenSourcePin=Name==TEXT("SM_A7V")?TEXT("0009A2EF0C9D446FD25EB729609284ADBB951B53"):
        Name==TEXT("SM_BeuteMarkIV")?TEXT("723DB13EBBC44E5B59A051310AEBA23670636921"):TEXT("1BD160E79007CA864D543951AAC815AED4B9F367");
    if(!SourcePin.Equals(FrozenSourcePin,ESearchCase::IgnoreCase)){Reason=TEXT("german_frozen_original_package_pin");return false;}
    const FString SourceFile=FPackageName::LongPackageNameToFilename(FPackageName::ObjectPathToPackageName(SourcePath),FPackageName::GetAssetPackageExtension());
    const FString File=FPackageName::LongPackageNameToFilename(Mesh->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    if(!Sha1File(File).Equals(DuplicatePin,ESearchCase::IgnoreCase)||!Sha1File(SourceFile).Equals(SourcePin,ESearchCase::IgnoreCase))
    {Reason=TEXT("german_source_or_duplicate_package_pin");return false;}
    for(const TCHAR* Key:{TEXT("geometryUVSectionSHA1"),TEXT("collisionSHA1")})
    {
        FString A,S,D;
        if(!Actual->TryGetStringField(Key,A)||!(*Source)->TryGetStringField(Key,S)||!(*Duplicate)->TryGetStringField(Key,D)||A!=S||A!=D)
        {Reason=TEXT("german_actual_geometry_or_collision_changed");return false;}
    }
    for(const TCHAR* Key:{TEXT("generateNonMirroredCollision"),TEXT("generateMirroredCollision")})
    {
        bool A=false,S=false,D=false;
        if(!Actual->TryGetBoolField(Key,A)||!(*Source)->TryGetBoolField(Key,S)||!(*Duplicate)->TryGetBoolField(Key,D)||A!=S||A!=D)
        {Reason=TEXT("german_actual_serialized_collision_flags_changed");return false;}
    }
    const TArray<TSharedPtr<FJsonValue>>* Materials=nullptr;
    if(!Selected->TryGetArrayField(TEXT("materialPaths"),Materials)||!Materials||Materials->Num()!=Mesh->GetStaticMaterials().Num())
    {Reason=TEXT("german_actual_material_slots_changed");return false;}
    for(int32 I=0;I<Materials->Num();++I)
        if(!(*Materials)[I].IsValid()||!Mesh->GetStaticMaterials()[I].MaterialInterface
            ||(*Materials)[I]->AsString()!=Mesh->GetStaticMaterials()[I].MaterialInterface->GetPathName())
        {Reason=TEXT("german_actual_material_binding_changed");return false;}
#else
    Reason=TEXT("german_editor_proof_unavailable_in_runtime");return false;
#endif
    }
    const auto& Positions=LOD->VertexBuffers.PositionVertexBuffer;
    const auto Indices=LOD->IndexBuffer.GetArrayView();
    if(!Positions.GetAllowCPUAccess()||!Positions.GetVertexData()||Positions.GetNumVertices()==0
        ||Positions.GetNumVertices()>MaxStaticVertices||Indices.Num()==0||Indices.Num()%3||Indices.Num()/3>MaxStaticFaces)
    {Reason=TEXT("german_actual_cpu_stream_budget");return false;}
    FStaticCache Built;Built.Mesh=Mesh;Built.Resource=LOD;Built.PackagePin=DuplicatePin;Built.LightingGuid=Mesh->GetLightingGuid();
    Built.GeometryPin=Actual->GetStringField(TEXT("geometryUVSectionSHA1"));
    Built.CollisionPin=Actual->GetStringField(Review()?TEXT("collisionSHA1"):TEXT("collisionRuntimeSHA1"));
    Built.Vertices.Reserve(Positions.GetNumVertices());
    for(uint32 I=0;I<Positions.GetNumVertices();++I)
    {
        const FVector P(Positions.VertexPosition(I));if(P.ContainsNaN()){Reason=TEXT("german_actual_vertex_not_finite");return false;}
        Built.Vertices.Add(P);Built.Bounds+=P;
    }
    for(int32 I=0;I<Indices.Num();I+=3)
    {
        const FIntVector T(Indices[I],Indices[I+1],Indices[I+2]);
        if(!Built.Vertices.IsValidIndex(T.X)||!Built.Vertices.IsValidIndex(T.Y)||!Built.Vertices.IsValidIndex(T.Z))
        {Reason=TEXT("german_actual_index_out_of_bounds");return false;}Built.Faces.Add(T);
    }
    if(Name==TEXT("SM_TrackShoe"))
    {
        // V33's rolled grouser is on the real local +Z outer surface. On
        // the lower run the actual 180-degree instance points it downwards.
        // Extract original flat-top vertices, not synthetic shoe box corners.
        for(const FVector& P:Built.Vertices)if(FMath::Abs(P.Z-Built.Bounds.Max.Z)<=1.e-4)
        {
            bool Seen=false;for(const FVector& Q:Built.OuterSole)if(P.Equals(Q,1.e-5)){Seen=true;break;}
            if(!Seen)Built.OuterSole.Add(P);
        }
        if(Built.OuterSole.Num()<3||Built.OuterSole.Num()>32)
        {Reason=TEXT("german_actual_outer_sole_contract");return false;}
    }
    Out=&MeshCaches.Add(Mesh,MoveTemp(Built));return true;
}
bool AssetsReady(EFrontlineVehicle Type,FString& Reason)
{
    if((!Review()&&!ProductionCandidate())||(Type!=EFrontlineVehicle::A7V&&Type!=EFrontlineVehicle::BeuteMarkIV))
    {Reason=TEXT("german_review_date_roster_unavailable");return false;}
    FStaticCache* C=nullptr;
    for(const FString& Name:{TEXT("SM_")+FString(UFrontlineVehicleSubsystem::Definition(Type).AssetKey),FString(TEXT("SM_TrackShoe"))})
        if(!Cache(LoadObject<UStaticMesh>(nullptr,*URI(Name)),C,Reason))return false;
    return true;
}
struct FAttitudeSeed
{
    double A=0,B=0,RootAboveCenterFloorCm=0;
};
struct FState
{
    FGuid Encounter;FString SceneId;uint32 Revision=0;int32 Serial=0;
    TWeakObjectPtr<UStaticMeshComponent> Ground,Apron;
    FWarmSupport Warm;bool HasWarm=false;
    FAttitudeSeed Attitude;bool HasAttitude=false;int32 AttitudeInitialMarkers=0;
    TWeakObjectPtr<UStaticMeshComponent> AttitudeBodyComponent;
    TWeakObjectPtr<UInstancedStaticMeshComponent> AttitudeTrackComponent;
    TWeakObjectPtr<UStaticMesh> AttitudeBodyMesh,AttitudeShoeMesh;
    TSharedPtr<FJsonObject> Last,FirstRejected;
    FString LastLogReason;double LastLogHost=-DBL_MAX;bool IngressPinsVerified=false;
};
TMap<TWeakObjectPtr<AFrontlineTank>,FState> States;
void ClearIngress(UWorld* World)
{
    if(World)for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("FRRU_V82GermanIngressOwned")))It->Destroy();
    States.Empty();
}
const wfexp::scene::SceneDefinition* Scene(const AWarfareFrontlineGameMode* B)
{
    const FTCHARToUTF8 Mission(*WarfareFRRUCampaignV82::MissionId(B));return wfexp::scene::ForMission(std::string_view(Mission.Get(),Mission.Length()));
}
FString Wide(const char* S){return S?FString(UTF8_TO_TCHAR(S)):FString();}
bool IngressPins(const FString& ApronPath,const FString& BackdropPath,const wfexp::scene::SceneDefinition& G,FString& Reason)
{
    if(!Review())
    {
        for(const FString& Path:{ApronPath,BackdropPath})
        {
            const auto Row=RuntimeRowV82(Path);FString Id;double Revision=0;
            TSharedPtr<FJsonObject> Actual;
            if(!Row||!Row->TryGetStringField(TEXT("sceneId"),Id)||Id!=Wide(G.id)
                ||!Row->TryGetNumberField(TEXT("revision"),Revision)||Revision!=G.revision
                ||!RuntimeMeshV82(LoadObject<UStaticMesh>(nullptr,*Path),Actual,Reason))
            {if(Reason.IsEmpty())Reason=TEXT("german_packaged_ingress_scene_stream_mismatch");return false;}
        }
        return true;
    }
    FString Text;TSharedPtr<FJsonObject> Proof;
    if(!FFileHelper::LoadFileToString(Text,*FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),TEXT("Saved/FRRU/V82GermanHostArmour/ingress-authored-basis-corrective-pins.json")))
        ||!Json(Text,Proof)||!Bool(Proof,TEXT("success"))||!Bool(Proof,TEXT("actualUEExecuted"))
        ||!Bool(Proof,TEXT("diskReloadAllEightVerified"))||!Bool(Proof,TEXT("protectedPackagesUnchanged"))
        ||!Bool(Proof,TEXT("completeBasisValidAllEight"))||!Bool(Proof,TEXT("exactShapeNormalsUV0ColoursSectionsAllEight"))
        ||!Bool(Proof,TEXT("exactDiagnosedUV1ResidualOnly"))||!Bool(Proof,TEXT("completeOriginalCollisionRecipesAllEight"))
        ||!Bool(Proof,TEXT("actualCollisionQueriesAllEight")))
    {Reason=TEXT("german_actual_eight_ingress_package_proof_missing");return false;}
    FString Schema,Manifest,Contract,Diagnosis;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    const TSharedPtr<FJsonObject>* Pins=nullptr;
    if(!Proof->TryGetStringField(TEXT("schemaVersion"),Schema)||Schema!=TEXT("v82.german.ingress.authored.basis.corrective.receiver.1")
        ||!Proof->TryGetStringField(TEXT("manifestSHA256"),Manifest)||Manifest!=TEXT("a271a61df88c0e7ff851b1f026d04ea1fda08ea48161d4168e12c1ab9bcf261c")
        ||!Proof->TryGetStringField(TEXT("contractSHA256"),Contract)||Contract!=TEXT("ca26086fac1daa8f97741397d28e742ee39a6ca8281b8879fecdbc5e13ff60a8")
        ||!Proof->TryGetStringField(TEXT("diagnosisProofSHA256"),Diagnosis)||Diagnosis!=TEXT("feeabc3a18e3d8cd8cf480a94edf1841a20553a59b752ccf9ca22e6c263fca9b")
        ||!Proof->TryGetArrayField(TEXT("meshes"),Rows)||!Rows||Rows->Num()!=8
        ||!Proof->TryGetObjectField(TEXT("packageSHA1Pins"),Pins)||!Pins||!(*Pins))
    {Reason=TEXT("german_actual_ingress_proof_namespace");return false;}
    TSharedPtr<FJsonObject> OriginalProof;const TSharedPtr<FJsonObject>* OriginalPins=nullptr;FString OriginalSchema,OriginalManifest,OriginalReadback;
    if(!FFileHelper::LoadFileToString(Text,*FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),TEXT("Saved/FRRU/V82GermanHostArmour/ingress-pins.json")))
        ||!Json(Text,OriginalProof)||!Bool(OriginalProof,TEXT("success"))||!Bool(OriginalProof,TEXT("actualUEExecuted"))
        ||!Bool(OriginalProof,TEXT("diskReloadAllEightVerified"))||!Bool(OriginalProof,TEXT("protectedPackagesUnchanged"))
        ||!OriginalProof->TryGetStringField(TEXT("schemaVersion"),OriginalSchema)||OriginalSchema!=TEXT("v82.german.ingress.receiver.import.1")
        ||!OriginalProof->TryGetStringField(TEXT("manifestSHA256"),OriginalManifest)||OriginalManifest!=TEXT("58626b0db5665d17f4aa4d2fd9d3f70b8388835833ac2b80d6c072a376722b6b")
        ||!OriginalProof->TryGetStringField(TEXT("independentReadbackSHA256"),OriginalReadback)||OriginalReadback!=TEXT("45c2802b778e4d67b71c352611c077bde79ed930a4265a6696bc723dcf2371d5")
        ||!OriginalProof->TryGetObjectField(TEXT("packageSHA1Pins"),OriginalPins)||!OriginalPins||!(*OriginalPins))
    {Reason=TEXT("german_actual_original_ingress_proof_missing");return false;}
    for(const FString& Path:{ApronPath,BackdropPath})
    {
        TSharedPtr<FJsonObject> Row;FString Pin;
        for(const auto& Item:*Rows)if(Item)
        {
            const auto J=Item->AsObject();FString ObjectPath,Id;
            if(J&&J->TryGetStringField(TEXT("objectPath"),ObjectPath)&&ObjectPath==Path)
            {
                double Revision=0;
                if(Row||!J->TryGetStringField(TEXT("sceneId"),Id)||Id!=Wide(G.id)
                    ||!J->TryGetNumberField(TEXT("revision"),Revision)||Revision!=G.revision)
                {Reason=TEXT("german_actual_ingress_scene_revision");return false;}Row=J;
            }
        }
        FString RowPin;
        if(!Row||!Bool(Row,TEXT("diskReloadVerified"))||!Row->TryGetStringField(TEXT("savedPackageSHA1"),RowPin)
            ||!(*Pins)->TryGetStringField(Path,Pin)||!Pin.Equals(RowPin,ESearchCase::IgnoreCase)
            ||!Sha1File(FPackageName::LongPackageNameToFilename(FPackageName::ObjectPathToPackageName(Path),FPackageName::GetAssetPackageExtension())).Equals(Pin,ESearchCase::IgnoreCase))
        {Reason=TEXT("german_actual_ingress_package_pin");return false;}
        FString SourcePath,SourcePin,OriginalPin;
        const FString ExpectedSource=Path.Replace(TEXT("/IngressAuthoredBasisV1/"),TEXT("/Ingress/")).Replace(TEXT("_AuthoredBasisV1"),TEXT("_V82"));
        const TSharedPtr<FJsonObject>* ExpectedEditor=nullptr;const TSharedPtr<FJsonObject>* ExpectedRuntime=nullptr;
        const TSharedPtr<FJsonObject>* SourceEditor=nullptr;const TSharedPtr<FJsonObject>* SourceRuntime=nullptr;
        if(!Row->TryGetStringField(TEXT("sourceObjectPath"),SourcePath)||SourcePath!=ExpectedSource
            ||!Row->TryGetStringField(TEXT("sourceSavedPackageSHA1"),SourcePin)||!(*OriginalPins)->TryGetStringField(SourcePath,OriginalPin)
            ||!SourcePin.Equals(OriginalPin,ESearchCase::IgnoreCase)
            ||!Sha1File(FPackageName::LongPackageNameToFilename(FPackageName::ObjectPathToPackageName(SourcePath),FPackageName::GetAssetPackageExtension())).Equals(SourcePin,ESearchCase::IgnoreCase)
            ||!Row->TryGetObjectField(TEXT("actualSnapshot"),ExpectedEditor)||!ExpectedEditor||!(*ExpectedEditor)
            ||!(*ExpectedEditor)->TryGetObjectField(TEXT("runtimeStaticMeshSnapshot"),ExpectedRuntime)||!ExpectedRuntime||!(*ExpectedRuntime)
            ||!Row->TryGetObjectField(TEXT("sourceSnapshot"),SourceEditor)||!SourceEditor||!(*SourceEditor)
            ||!(*SourceEditor)->TryGetObjectField(TEXT("runtimeStaticMeshSnapshot"),SourceRuntime)||!SourceRuntime||!(*SourceRuntime)
            ||(*SourceRuntime)->GetStringField(TEXT("collisionRecipeSHA1"))!=(*ExpectedRuntime)->GetStringField(TEXT("collisionRecipeSHA1")))
        {Reason=TEXT("german_actual_corrective_original_pin_or_collision_recipe");return false;}
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);const auto Actual=RuntimeStaticMeshSnapshot(Mesh);
        if(!Mesh||Mesh->GetOutermost()->IsDirty()||!Bool(Actual,TEXT("success"))||!Bool(Actual,TEXT("allowCPUAccess")))
        {Reason=TEXT("german_actual_corrective_cpu_streams_missing");return false;}
        for(const TCHAR* Key:{TEXT("geometryUVSectionSHA1"),TEXT("positionIndexUVColourSectionSHA1"),TEXT("normalXYZSHA1"),
                             TEXT("collisionRuntimeSHA1"),TEXT("collisionRecipeSHA1"),TEXT("bodySetupGuid"),
                             TEXT("materialBindings"),TEXT("lods"),TEXT("editorCanonicalCornerProof")})
        {
            const auto* A=Actual->Values.Find(Key);const auto* E=(*ExpectedRuntime)->Values.Find(Key);
            if(!A||!E||!(*A)||!(*E)||!FJsonValue::CompareEqual(**A,**E))
            {Reason=TEXT("german_actual_corrective_runtime_metadata_mismatch");return false;}
        }
    }
    return true;
}
bool NativeApronMesh(UStaticMesh* Mesh,const FTransform& Basis,const wfexp::scene::SceneDefinition& G,FString& Reason)
{
    const auto* Render=Mesh?Mesh->GetRenderData():nullptr;const auto* Setup=Mesh?Mesh->GetBodySetup():nullptr;
    if(!Render||Render->LODResources.IsEmpty()||!Setup||Setup->CollisionTraceFlag!=CTF_UseComplexAsSimple
        ||Setup->AggGeom.GetElementCount()!=0||Mesh->GetStaticMaterials().IsEmpty())
    {Reason=TEXT("german_ingress_actual_complex_mesh_missing");return false;}
    const auto& LOD=Render->LODResources[0];const auto& VB=LOD.VertexBuffers.PositionVertexBuffer;const auto Indices=LOD.IndexBuffer.GetArrayView();
    if(!VB.GetVertexData()||VB.GetNumVertices()==0||VB.GetNumVertices()>8192||Indices.Num()==0||Indices.Num()%3||Indices.Num()/3>4096)
    {Reason=TEXT("german_ingress_actual_mesh_budget");return false;}
    const double DY=(G.maxY-G.minY)/(G.rows-1);FBox Bounds(ForceInit);
    for(uint32 I=0;I<VB.GetNumVertices();++I)
    {
        const FVector P=Basis.TransformPosition(FVector(VB.VertexPosition(I)));
        if(P.ContainsNaN()||P.Y<G.minY-.01||P.Y>G.maxY+.01
            ||!((P.X>=G.minX-2000-.01&&P.X<=G.minX+.01)||(P.X>=G.maxX-.01&&P.X<=G.maxX+2000+.01)))
        {Reason=TEXT("german_ingress_actual_vertex_domain");return false;}
        double Z=0;
        if(!wfexp::scene::HeightAt(G,FMath::Clamp(P.X,G.minX,G.maxX),FMath::Clamp(P.Y,G.minY,G.maxY),Z)||FMath::Abs(P.Z-Z)>.01)
        {Reason=TEXT("german_ingress_actual_native_height_mismatch");return false;}Bounds+=P;
    }
    for(int32 I=0;I<Indices.Num();I+=3)
    {
        if(Indices[I]>=VB.GetNumVertices()||Indices[I+1]>=VB.GetNumVertices()||Indices[I+2]>=VB.GetNumVertices())
        {Reason=TEXT("german_ingress_actual_index_contract");return false;}
        const FVector A=Basis.TransformPosition(FVector(VB.VertexPosition(Indices[I]))),
            B=Basis.TransformPosition(FVector(VB.VertexPosition(Indices[I+1]))),C=Basis.TransformPosition(FVector(VB.VertexPosition(Indices[I+2])));
        if(FMath::Max3(A.Y,B.Y,C.Y)-FMath::Min3(A.Y,B.Y,C.Y)>DY+.01
            ||!((FMath::Max3(A.X,B.X,C.X)<=G.minX+.01)||(FMath::Min3(A.X,B.X,C.X)>=G.maxX-.01)))
        {Reason=TEXT("german_ingress_actual_finite_cell_contract");return false;}
    }
    if(!Bounds.IsValid||FMath::Abs(Bounds.Min.X-(G.minX-2000))>.01||FMath::Abs(Bounds.Max.X-(G.maxX+2000))>.01
        ||FMath::Abs(Bounds.Min.Y-G.minY)>.01||FMath::Abs(Bounds.Max.Y-G.maxY)>.01)
    {Reason=TEXT("german_ingress_actual_boundary_coverage");return false;}
    return true;
}
UStaticMeshComponent* StageApron(AWarfareFrontlineGameMode* B,UStaticMeshComponent* Ground,const FString& Path,
    const wfexp::scene::SceneDefinition& G,FString& Reason)
{
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
    const FTransform NativeBasis(FQuat::Identity,FVector::ZeroVector,FVector(1,-1,1));
    if(!Ground||!Ground->GetComponentTransform().Equals(NativeBasis,1.e-6))
    {Reason=TEXT("german_ingress_exact_staged_native_basis");return nullptr;}
    if(!NativeApronMesh(Mesh,Ground->GetComponentTransform(),G,Reason))return nullptr;
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Params.Owner=Ground->GetOwner();
    AActor* Owner=B->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),FTransform::Identity,Params);
    if(!Owner){Reason=TEXT("german_ingress_actor_creation_failed");return nullptr;}
    Owner->Tags.Add(TEXT("FRRU_V82Scene"));Owner->Tags.Add(TEXT("FRRU_V82GermanIngressOwned"));
    auto* C=NewObject<UStaticMeshComponent>(Owner,TEXT("FRRU_V82GermanIngressApron"),RF_Transient);
    Owner->AddInstanceComponent(C);Owner->SetRootComponent(C);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
    C->ComponentTags.Add(TEXT("FRRU_V82GermanIngressApronGround"));C->ComponentTags.Add(TEXT("FRRU_V82Collision"));
    C->ComponentTags.Add(FName(*(TEXT("FRRU_V82GermanIngressScene:")+Wide(G.id))));
    C->ComponentTags.Add(FName(*FString::Printf(TEXT("FRRU_V82GermanIngressRevision:%u"),G.revision)));
    C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);C->SetCollisionResponseToAllChannels(ECR_Block);
    C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->SetReceivesDecals(true);
    for(int32 I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,Ground->GetMaterial(FMath::Min(I,Ground->GetNumMaterials()-1)));
    C->RegisterComponent();Owner->SetActorTransform(Ground->GetComponentTransform());
    if(!C->IsRegistered()||!C->IsPhysicsStateCreated()||C->GetMaterial(0)!=Ground->GetMaterial(0))
    {Owner->Destroy();Reason=TEXT("german_ingress_actual_binding_failed");return nullptr;}
    return C;
}
bool Context(AFrontlineTank* V,AWarfareFrontlineGameMode* B,UStaticMeshComponent* Body,UInstancedStaticMeshComponent* Tracks,
    FState& S,const wfexp::scene::SceneDefinition*& G,FString& Reason)
{
    FString Id;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;G=Scene(B);
    if(!Eligible(B,V?V->Team:INDEX_NONE,V?V->Type:EFrontlineVehicle::Count)||!G
        ||G->columns<2||G->rows<2||!G->heightsCm||G->heightCount!=size_t(G->columns)*size_t(G->rows)
        ||!FMath::IsFinite(G->minX)||!FMath::IsFinite(G->minY)||!FMath::IsFinite(G->maxX)||!FMath::IsFinite(G->maxY)
        ||G->maxX<=G->minX||G->maxY<=G->minY
        ||!WarfareFRRUWorldV82::SceneInfo(B,Id,Revision,Solids,Minimum,Maximum)||Id!=Wide(G->id)||Revision!=G->revision
        ||!WarfareFRRUContentV82::SceneStaged(B,Id,Revision))
    {Reason=TEXT("german_exact_scene_context");return false;}
    if(!Body||!Tracks||V->GetRootComponent()!=Body||!V->GetActorScale3D().Equals(FVector::OneVector,1.e-6)
        ||!Tracks->GetRelativeTransform().Equals(FTransform::Identity,1.e-6)||!Body->IsRegistered()||!Tracks->IsRegistered())
    {Reason=TEXT("german_actual_component_basis");return false;}
    if(S.Encounter!=B->GetEncounterId()||S.SceneId!=Id||S.Revision!=Revision||S.Serial!=V->Serial)
    {S=FState{};S.Encounter=B->GetEncounterId();S.SceneId=Id;S.Revision=Revision;S.Serial=V->Serial;}
    if(!S.Ground.IsValid()||!S.Apron.IsValid())
    {
        const auto Rendering=WarfareFRRUContentV82::DescribeSceneRendering(B);
        FString ActualId,GroundPath,BackdropPath;const TArray<TSharedPtr<FJsonValue>>* Parts=nullptr;
        if(!Rendering||!Rendering->TryGetStringField(TEXT("sceneId"),ActualId)||ActualId!=Id
            ||!Rendering->TryGetArrayField(TEXT("components"),Parts)||!Parts)
        {Reason=TEXT("german_actual_ground_identity_missing");return false;}
        for(const auto& Part:*Parts)if(Part)
        {
            const auto J=Part->AsObject();FString Key,Path;
            if(J&&J->TryGetStringField(TEXT("component"),Key)&&Key==TEXT("ground")&&J->TryGetStringField(TEXT("mesh"),Path))
            {if(!GroundPath.IsEmpty()){Reason=TEXT("german_ambiguous_ground_identity");return false;}GroundPath=Path;}
            if(J&&J->TryGetStringField(TEXT("component"),Key)&&Key==TEXT("outsideBackdrop")&&J->TryGetStringField(TEXT("mesh"),Path))
            {if(!BackdropPath.IsEmpty()){Reason=TEXT("german_ambiguous_backdrop_identity");return false;}BackdropPath=Path;}
        }
        if(GroundPath.IsEmpty()){Reason=TEXT("german_actual_ground_identity_missing");return false;}
        FString Token=WarfareFRRUCampaignV82::MissionId(B);Token.ReplaceInline(TEXT("."),TEXT("_"));
        const FString Name=TEXT("SM_DE_Ingress_")+Token+TEXT("_AuthoredBasisV1");
        const FString ApronPath=FString(ReceiverRoot)+TEXT("IngressAuthoredBasisV1/")+Name+TEXT(".")+Name;
        const FString BackdropName=TEXT("SM_DE_IngressBackdrop_")+Token+TEXT("_AuthoredBasisV1");
        const FString ExactBackdropPath=FString(ReceiverRoot)+TEXT("IngressAuthoredBasisV1/")+BackdropName+TEXT(".")+BackdropName;
        if(BackdropPath!=ExactBackdropPath){Reason=TEXT("german_actual_matching_clipped_backdrop_missing");return false;}
        if(!S.IngressPinsVerified)
        {if(!IngressPins(ApronPath,ExactBackdropPath,*G,Reason))return false;S.IngressPinsVerified=true;}
        UStaticMeshComponent* Ground=nullptr;UStaticMeshComponent* Apron=nullptr;
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("FRRU_V82Scene")))
        {
            TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
            for(auto* C:Components)if(C&&C->IsRegistered()&&C->IsPhysicsStateCreated()&&C->GetStaticMesh()
                &&C->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics&&C->ComponentHasTag(TEXT("FRRU_V82Collision")))
            {
                if(C->ComponentHasTag(TEXT("ground"))&&C->GetStaticMesh()->GetPathName()==GroundPath)
                {if(Ground){Reason=TEXT("german_ambiguous_actual_ground");return false;}Ground=C;}
                if(C->ComponentHasTag(TEXT("FRRU_V82GermanIngressApronGround"))&&C->GetStaticMesh()->GetPathName()==ApronPath
                    &&C->ComponentHasTag(FName(*(TEXT("FRRU_V82GermanIngressScene:")+Id)))
                    &&C->ComponentHasTag(FName(*FString::Printf(TEXT("FRRU_V82GermanIngressRevision:%u"),Revision))))
                {if(Apron){Reason=TEXT("german_ambiguous_actual_ingress");return false;}Apron=C;}
            }
        }
        // Content selects the separately clipped backdrop. This visible apron
        // is a separate owned actor, so its lifecycle and component count never
        // alter the staged native scene actor's existing contract.
        if(Ground&&!Apron)Apron=StageApron(B,Ground,ApronPath,*G,Reason);
        // There is no fallback to the old, differently sloped backdrop.
        if(!Ground||!Apron){if(Reason.IsEmpty())Reason=TEXT("german_exact_registered_ingress_ground_missing");return false;}
        if(Apron->GetNumMaterials()<1||Ground->GetNumMaterials()<1||Apron->GetMaterial(0)!=Ground->GetMaterial(0))
        {Reason=TEXT("german_ingress_actual_ground_material_mismatch");return false;}
        S.Ground=Ground;S.Apron=Apron;
    }
    return true;
}
UStaticMeshComponent* ValidatedSceneApronV87(const AWarfareFrontlineGameMode* B,UStaticMeshComponent* Ground,TSharedPtr<FJsonObject>& Evidence)
{
    Evidence=MakeShared<FJsonObject>();Evidence->SetBoolField(TEXT("readOnly"),true);Evidence->SetBoolField(TEXT("accepted"),false);
    const auto Reject=[&](const TCHAR* Why)->UStaticMeshComponent*{Evidence->SetStringField(TEXT("reason"),Why);return nullptr;};
    const auto* G=Scene(B);FString Id;uint32 Revision=0;int32 Solids=0;FVector2D Minimum,Maximum;
    if(!B||!G||!Ground||!WarfareFRRUWorldV82::SceneInfo(B,Id,Revision,Solids,Minimum,Maximum)||Id!=Wide(G->id)||Revision!=G->revision||!WarfareFRRUContentV82::SceneStaged(B,Id,Revision))return Reject(TEXT("current_staged_scene_mismatch"));
    UStaticMeshComponent* Apron=nullptr;
    for(const auto& Pair:States)
    {
        const auto* Vehicle=Pair.Key.Get();const auto& S=Pair.Value;
        if(!Vehicle||Vehicle->GetWorld()!=B->GetWorld()||S.Encounter!=B->GetEncounterId()||S.SceneId!=Id||S.Revision!=Revision||S.Ground.Get()!=Ground||!S.IngressPinsVerified||!S.Apron.IsValid())continue;
        auto* Candidate=S.Apron.Get();if(Apron&&Apron!=Candidate)return Reject(TEXT("ambiguous_verified_scene_aprons"));Apron=Candidate;
    }
    if(!Apron)return Reject(TEXT("no_existing_verified_german_ingress_state"));
    FString Token=WarfareFRRUCampaignV82::MissionId(B);Token.ReplaceInline(TEXT("."),TEXT("_"));
    const FString Name=TEXT("SM_DE_Ingress_")+Token+TEXT("_AuthoredBasisV1"),Expected=FString(ReceiverRoot)+TEXT("IngressAuthoredBasisV1/")+Name+TEXT(".")+Name;
    if(!Apron->IsRegistered()||!Apron->IsPhysicsStateCreated()||!Apron->GetStaticMesh()||Apron->GetStaticMesh()->GetPathName()!=Expected
        ||Apron->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics||!Apron->ComponentHasTag(TEXT("FRRU_V82Collision"))||!Apron->ComponentHasTag(TEXT("FRRU_V82GermanIngressApronGround"))
        ||!Apron->ComponentHasTag(FName(*(TEXT("FRRU_V82GermanIngressScene:")+Id)))||!Apron->ComponentHasTag(FName(*FString::Printf(TEXT("FRRU_V82GermanIngressRevision:%u"),Revision)))
        ||!Apron->GetComponentTransform().Equals(Ground->GetComponentTransform(),1.e-6))return Reject(TEXT("registered_apron_identity_basis_or_collision_mismatch"));
    FString Why;if(!NativeApronMesh(Apron->GetStaticMesh(),Apron->GetComponentTransform(),*G,Why)){Evidence->SetStringField(TEXT("nativeMeshRejection"),Why);return Reject(TEXT("actual_apron_finite_face_validation_failed"));}
    const auto& LOD=Apron->GetStaticMesh()->GetRenderData()->LODResources[0];
    Evidence->SetBoolField(TEXT("accepted"),true);Evidence->SetStringField(TEXT("sceneId"),Id);Evidence->SetNumberField(TEXT("sceneRevision"),Revision);Evidence->SetStringField(TEXT("encounter"),B->GetEncounterId().ToString());
    Evidence->SetStringField(TEXT("groundComponent"),Ground->GetPathName());Evidence->SetStringField(TEXT("apronComponent"),Apron->GetPathName());Evidence->SetStringField(TEXT("apronAsset"),Expected);
    Evidence->SetNumberField(TEXT("actualVertices"),LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices());Evidence->SetNumberField(TEXT("actualTriangles"),LOD.IndexBuffer.GetNumIndices()/3);
    Evidence->SetNumberField(TEXT("centralMinimumX"),G->minX);Evidence->SetNumberField(TEXT("centralMaximumX"),G->maxX);Evidence->SetBoolField(TEXT("actualFiniteFacesOnlyOutsideCentralScene"),true);Evidence->SetBoolField(TEXT("actualNativeHeightsValidated"),true);
    Evidence->SetStringField(TEXT("reason"),TEXT("exact_same_scene_revision_asset_and_native_apron_vertices_faces_verified; not an independent obstacle"));return Apron;
}
bool Floor(const wfexp::scene::SceneDefinition& G,const FVector2D& XY,double& Z)
{
    if(XY.ContainsNaN()||XY.X<G.minX-2000||XY.X>G.maxX+2000||XY.Y<G.minY||XY.Y>G.maxY)return false;
    return wfexp::scene::HeightAt(G,FMath::Clamp(XY.X,G.minX,G.maxX),XY.Y,Z)&&FMath::IsFinite(Z);
}
bool Footing(const wfexp::scene::SceneDefinition& G,const FVector& P)
{
    double Z=0;if(!Floor(G,{P.X,P.Y},Z))return false;
    const auto Foot=wfexp::scene::FootingAt(G,FMath::Clamp(P.X,G.minX,G.maxX),P.Y);return Foot.valid&&Foot.walkable;
}
struct FTrackPose {FTransform Transform;FMatrix StoredMatrix;};
bool TracksAt(const TArray<FVector2D>& Path,const TArray<float>& Segments,float Length,float Phase,int32 Count,bool Heavy,float Y,
    TArray<FTrackPose>& Out,FString& Reason)
{
    if(Path.Num()<3||Segments.Num()!=Path.Num()||!FMath::IsFinite(Length)||Length<=0||!FMath::IsFinite(Phase)
        ||Count<3||Count*2>MaxInstances){Reason=TEXT("german_actual_track_path_budget");return false;}
    Out.Reset(Count*2);
    for(int32 Side=0;Side<2;++Side)for(int32 I=0;I<Count;++I)
    {
        float D=FMath::Fmod(float(I)*Length/Count+Phase,Length);if(D<0)D+=Length;int32 Segment=0;
        while(Segment<Segments.Num()-1&&D>Segments[Segment]){D-=Segments[Segment];++Segment;}
        if(!FMath::IsFinite(Segments[Segment])||Segments[Segment]<=0){Reason=TEXT("german_actual_track_segment");return false;}
        const FVector2D Delta=Path[(Segment+1)%Path.Num()]-Path[Segment],P=Path[Segment]+Delta*(D/Segments[Segment]);
        const FTransform T(FRotator(-FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X)),0,0),FVector(P.X,(Side?1:-1)*Y,P.Y),FVector(1,Heavy?1.15f:1,1));
        // Installed UE 5.8 stores an FMatrix directly from this exact call in
        // PerInstanceSMData. No synthetic rounding or decomposed proxy is used.
        Out.Add({T,T.ToMatrixWithScale()});
    }
    return true;
}
double Cross(const FVector& A,const FVector& B){return A.X*B.Y-A.Y*B.X;}
bool XYSeparatedV826(const FVector (&A)[3],const FVector (&B)[3])
{
    // Conservative separating-axis broadphase only. Degenerate projections
    // still pass through the original finite clipping/contact calculation.
    for(int32 I=0;I<3;++I)if(A[I].ContainsNaN()||B[I].ContainsNaN())return false;
    if(FMath::Abs(Cross(A[1]-A[0],A[2]-A[0]))<=1.e-8
        ||FMath::Abs(Cross(B[1]-B[0],B[2]-B[0]))<=1.e-8)return false;
    for(int32 Shape=0;Shape<2;++Shape)for(int32 I=0;I<3;++I)
    {
        const FVector Origin=Shape?B[I]:A[I];
        const FVector Edge=(Shape?B[(I+1)%3]:A[(I+1)%3])-Origin;
        const double Margin=1.e-4*(1.+FVector2D(Edge.X,Edge.Y).Size());
        double AMin=DBL_MAX,AMax=-DBL_MAX,BMin=DBL_MAX,BMax=-DBL_MAX;
        for(int32 K=0;K<3;++K)
        {
            const double PA=Cross(Edge,A[K]-Origin),PB=Cross(Edge,B[K]-Origin);
            AMin=FMath::Min(AMin,PA);AMax=FMath::Max(AMax,PA);
            BMin=FMath::Min(BMin,PB);BMax=FMath::Max(BMax,PB);
        }
        if(AMax<BMin-Margin||BMax<AMin-Margin)return true;
    }
    return false;
}
void Clip(const TArray<FVector>& Input,const FVector& A,const FVector& B,TArray<FVector>& Output)
{
    Output.Reset();if(Input.IsEmpty())return;const FVector Edge=B-A;FVector Previous=Input.Last();double Prev=Cross(Edge,Previous-A);
    for(const FVector& P:Input)
    {
        const double Value=Cross(Edge,P-A);const bool In=Value>=-1.e-8,PrevIn=Prev>=-1.e-8;
        // Epsilon inclusion can put both endpoints just below zero. Bound the
        // intersection to the real edge rather than extrapolating a ghost face.
        if(In!=PrevIn)Output.Add(FMath::Lerp(Previous,P,FMath::Clamp(Prev/(Prev-Value),0.,1.)));
        if(In)Output.Add(P);Previous=P;Prev=Value;
    }
}
struct FWitness {FVector Local=FVector::ZeroVector;double Gap=0;int32 Strip=INDEX_NONE,Element=INDEX_NONE,Side=0;bool Valid=false;};
struct FScan
{
    int32 Pairs=0,RoofVisits=0,RoofReads=0,FaceVisits=0,Transformed=0,Culled=0,SeparatedPairs=0;
    double Minimum=DBL_MAX;bool Complete=false,BodyViolation=false;
    FWitness Witness[4];TArray<FWitness> ShoeContacts;TMap<FIntPoint,double> RoofCells;
};
bool FiniteFaces(const FStaticCache& Body,const FStaticCache& Shoe,const TArray<FTrackPose>& Tracks,const FTransform& Actor,
    const wfexp::scene::SceneDefinition& G,FScan& S,FString& Reason)
{
    S.ShoeContacts.SetNum(Tracks.Num());
    const double DX=(G.maxX-G.minX)/(G.columns-1),DY=(G.maxY-G.minY)/(G.rows-1);
    // Exact constant-X aprons are appended as ordinary cells; all covered XY
    // faces are checked, including the seam. No point is silently clamped.
    const int32 MinCell=-FMath::CeilToInt(2000./DX),MaxCell=G.columns-2+FMath::CeilToInt(2000./DX);
    auto Point=[&](int32 X,int32 Y)
    {return FVector(G.minX+X*DX,G.minY+Y*DY,G.heightsCm[Y*G.columns+FMath::Clamp(X,0,G.columns-1)]);};
    auto Roof=[&](int32 X0,int32 X1,int32 Y0,int32 Y1,double& High)
    {
        High=-DBL_MAX;
        for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)
        {
            if(++S.RoofVisits>MaxRoofVisits){Reason=TEXT("german_native_roof_visit_budget");return false;}
            const FIntPoint Key(X,Y);const double* Cached=S.RoofCells.Find(Key);
            if(!Cached)
            {
                if(S.RoofCells.Num()>=MaxRoofCells){Reason=TEXT("german_native_roof_cache_budget");return false;}
                double Value=-DBL_MAX;
                for(int32 YY=Y;YY<=Y+1;++YY)for(int32 XX=X;XX<=X+1;++XX)
                {const double Z=Point(XX,YY).Z;++S.RoofReads;if(!FMath::IsFinite(Z)){Reason=TEXT("german_native_roof_not_finite");return false;}Value=FMath::Max(Value,Z);}
                Cached=&S.RoofCells.Add(Key,Value);
            }
            High=FMath::Max(High,*Cached);
        }
        return true;
    };
    TArray<FVector> Poly,Buffer;Poly.Reserve(8);Buffer.Reserve(8);
    auto MeshFaces=[&](const FStaticCache& C,const FMatrix& Local,int32 Element,int32 Side)
    {
        FBox Bound(ForceInit);
        for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)for(int32 Z=0;Z<2;++Z)
            Bound+=Actor.TransformPosition(FVector(Local.TransformPosition(FVector(X?C.Bounds.Max.X:C.Bounds.Min.X,Y?C.Bounds.Max.Y:C.Bounds.Min.Y,Z?C.Bounds.Max.Z:C.Bounds.Min.Z))));
        if(!Bound.IsValid||Bound.Min.ContainsNaN()||Bound.Max.ContainsNaN()
            ||Bound.Min.X<G.minX-2000||Bound.Max.X>G.maxX+2000||Bound.Min.Y<G.minY||Bound.Max.Y>G.maxY)
        {Reason=TEXT("german_actual_mesh_outside_contact_domain");return false;}
        auto CellX=[&](double X){return FMath::Clamp(FMath::FloorToInt((X-G.minX)/DX),MinCell,MaxCell);};
        auto CellY=[&](double Y){return FMath::Clamp(FMath::FloorToInt((Y-G.minY)/DY),0,G.rows-2);};
        double RoofZ=0;if(!Roof(CellX(Bound.Min.X),CellX(Bound.Max.X),CellY(Bound.Min.Y),CellY(Bound.Max.Y),RoofZ))return false;
        if(Bound.Min.Z>RoofZ+1.e-4){S.Culled+=C.Faces.Num();return true;}
        TArray<FVector> World;World.SetNumUninitialized(C.Vertices.Num());TArray<uint8> Ready;Ready.Init(0,C.Vertices.Num());
        auto Vertex=[&](int32 I)->const FVector&
        {
            if(!Ready[I]){World[I]=Actor.TransformPosition(FVector(Local.TransformPosition(C.Vertices[I])));Ready[I]=1;++S.Transformed;}
            return World[I];
        };
        for(const FIntVector& T:C.Faces)
        {
            if(++S.FaceVisits>262144){Reason=TEXT("german_actual_face_visit_budget");return false;}
            const FVector V[3]={Vertex(T.X),Vertex(T.Y),Vertex(T.Z)};
            const int32 X0=CellX(FMath::Min3(V[0].X,V[1].X,V[2].X)),X1=CellX(FMath::Max3(V[0].X,V[1].X,V[2].X));
            const int32 Y0=CellY(FMath::Min3(V[0].Y,V[1].Y,V[2].Y)),Y1=CellY(FMath::Max3(V[0].Y,V[1].Y,V[2].Y));
            double LocalRoof=0;if(!Roof(X0,X1,Y0,Y1,LocalRoof))return false;
            if(FMath::Min3(V[0].Z,V[1].Z,V[2].Z)>LocalRoof+1.e-4){++S.Culled;continue;}
            for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)for(int32 Half=0;Half<2;++Half)
            {
                const FVector FloorTriangle[3]={Point(X,Y),Half?Point(X+1,Y+1):Point(X+1,Y),Half?Point(X,Y+1):Point(X+1,Y+1)};
                const FVector N=FVector::CrossProduct(FloorTriangle[1]-FloorTriangle[0],FloorTriangle[2]-FloorTriangle[0]);
                if(!FMath::IsFinite(N.Z)||N.Z<=0){Reason=TEXT("german_native_triangle_contract");return false;}
                if(XYSeparatedV826(V,FloorTriangle)){++S.SeparatedPairs;continue;}
                if(++S.Pairs>MaxFacePairs){Reason=TEXT("german_actual_finite_face_pair_budget");return false;}
                Poly.Reset();Poly.Append(V,3);for(int32 E=0;E<3&&!Poly.IsEmpty();++E){Clip(Poly,FloorTriangle[E],FloorTriangle[(E+1)%3],Buffer);Swap(Poly,Buffer);}
                for(const FVector& P:Poly)
                {
                    const double FloorZ=FloorTriangle[0].Z-(N.X*(P.X-FloorTriangle[0].X)+N.Y*(P.Y-FloorTriangle[0].Y))/N.Z;
                    const double Gap=P.Z-FloorZ;if(!FMath::IsFinite(Gap)){Reason=TEXT("german_actual_face_gap_not_finite");return false;}
                    S.Minimum=FMath::Min(S.Minimum,Gap);
                    if(Element!=INDEX_NONE)
                    {
                        auto& Contact=S.ShoeContacts[Element];
                        if(!Contact.Valid||Gap<Contact.Gap)Contact={Actor.InverseTransformPosition(P),Gap,INDEX_NONE,Element,Side,true};
                    }
                    if(Gap< -ContactToleranceCm)
                    {
                        if(Element==INDEX_NONE){S.BodyViolation=true;continue;}
                        const FVector PLocal=Actor.InverseTransformPosition(P);const double CentreY=Tracks[Element].Transform.GetLocation().Y;
                        const int32 Strip=(Side<0?0:2)+(PLocal.Y<CentreY?0:1);
                        auto& W=S.Witness[Strip];if(!W.Valid||Gap<W.Gap)W={PLocal,Gap,Strip,Element,Side,true};
                    }
                }
            }
        }
        return true;
    };
    if(!MeshFaces(Body,FMatrix::Identity,INDEX_NONE,0))return false;
    for(int32 I=0;I<Tracks.Num();++I)if(!MeshFaces(Shoe,Tracks[I].StoredMatrix,I,I<Tracks.Num()/2?-1:1))return false;
    S.Complete=true;if(S.BodyViolation){Reason=TEXT("german_actual_body_face_penetration");return false;}
    if(S.Minimum< -ContactToleranceCm){Reason=TEXT("german_actual_shoe_face_penetration");return false;}return true;
}
// Keep the chassis rigid. Original shoes actually near the ground, including
// ascending nose links, may move vertically within the bounded link stroke. Their
// actual transformed faces are scanned again; distant return links stay fixed.
// The resulting real contacts must surround the root on both running gears.
bool ArticulateSoles(const FStaticCache& Body,const FStaticCache& Shoe,TArray<FTrackPose>& Tracks,
    const FTransform& Actor,const wfexp::scene::SceneDefinition& G,
    FScan& Scan,FCandidate& Fit,FString& Reason,TSharedPtr<FJsonObject> Diag)
{
    Diag->SetBoolField(TEXT("articulationAttempted"),true);
    auto Reject=[&](const TCHAR* Why,int32 Element=INDEX_NONE,double Gap=0.)
    {
        Reason=Why;Diag->SetBoolField(TEXT("articulationAttemptPassed"),false);
        Diag->SetStringField(TEXT("articulationAttemptReason"),Why);
        if(Element!=INDEX_NONE&&!Diag->HasField(TEXT("firstArticulationRejectedElement")))
        {
            Diag->SetNumberField(TEXT("firstArticulationRejectedElement"),Element);
            Diag->SetNumberField(TEXT("firstArticulationRejectedGapCm"),Gap);
            Diag->SetNumberField(TEXT("firstArticulationRejectedOriginalZCm"),Tracks[Element].Transform.GetLocation().Z);
        }
        return false;
    };
    if(!Scan.Complete||Scan.BodyViolation||Scan.ShoeContacts.Num()!=Tracks.Num())return Reject(TEXT("german_articulation_incomplete_or_body_contact"));
    TArray<uint8> Contacting;Contacting.Init(0,Tracks.Num());
    int32 Adjusted=0;double Maximum=0;
    for(int32 I=0;I<Tracks.Num();++I)
    {
        const auto& C=Scan.ShoeContacts[I];if(!C.Valid)continue;
        // Contact eligibility is measured on the real current face. A fixed
        // local-Z band wrongly excludes ascending end links on an upslope.
        // Distant shoes, including the upper return run, remain suspended.
        if(C.Gap>MaxShoeArticulationCm)continue;
        if(C.Gap< -MaxShoeArticulationCm)return Reject(TEXT("german_shoe_articulation_limit"),I,C.Gap);
        Contacting[I]=1;const double Shift=-C.Gap;
        if(!FMath::IsFinite(Shift))return Reject(TEXT("german_shoe_articulation_nonfinite"),I,C.Gap);
        if(FMath::Abs(Shift)>1.e-6)
        {
            Tracks[I].Transform.AddToTranslation(Actor.GetRotation().UnrotateVector(FVector(0,0,Shift)));
            Tracks[I].StoredMatrix=Tracks[I].Transform.ToMatrixWithScale();++Adjusted;Maximum=FMath::Max(Maximum,FMath::Abs(Shift));
        }
    }
    FScan Final;
    if(!FiniteFaces(Body,Shoe,Tracks,Actor,G,Final,Reason)||!Final.Complete||Final.BodyViolation)
    {const FString FailureReason=Reason;return Reject(FailureReason.IsEmpty()?TEXT("german_articulation_final_faces_incomplete"):*FailureReason);}
    struct FContact {FVector World;int32 Element,Side;};TArray<FContact> Points;
    for(int32 I=0;I<Final.ShoeContacts.Num();++I)
    {
        const auto& C=Final.ShoeContacts[I];
        if(Contacting[I]&&C.Valid&&FMath::Abs(C.Gap)<=1.e-3)Points.Add({Actor.TransformPosition(C.Local),I,C.Side});
    }
    Points.Sort([](const FContact& A,const FContact& B){return A.World.X<B.World.X||(A.World.X==B.World.X&&A.World.Y<B.World.Y);});
    TArray<FContact> Polygon;
    for(const auto& P:Points)
    {while(Polygon.Num()>1&&Cross(Polygon.Last().World-Polygon[Polygon.Num()-2].World,P.World-Polygon.Last().World)<=1.e-8)Polygon.Pop();Polygon.Add(P);}
    const int32 LowerCount=Polygon.Num();
    for(int32 I=Points.Num()-2;I>=0;--I)
    {const auto& P=Points[I];while(Polygon.Num()>LowerCount&&Cross(Polygon.Last().World-Polygon[Polygon.Num()-2].World,P.World-Polygon.Last().World)<=1.e-8)Polygon.Pop();Polygon.Add(P);}
    if(Polygon.Num()>1)Polygon.Pop();bool Supported=false;
    for(int32 I=1;I+1<Polygon.Num();++I)
    {
        const FContact C[3]={Polygon[0],Polygon[I],Polygon[I+1]};
        if(C[0].Side==C[1].Side&&C[1].Side==C[2].Side)continue;
        FVector Weights;const FVector Root=Actor.GetLocation();
        if(!Solve3({C[0].World.X-Root.X,C[1].World.X-Root.X,C[2].World.X-Root.X},
            {C[0].World.Y-Root.Y,C[1].World.Y-Root.Y,C[2].World.Y-Root.Y},{1,1,1},{0,0,1},Weights)
            ||Weights.GetMin()< -1.e-6)continue;
        for(int32 J=0;J<3;++J)Fit.WorldSupport[J]=C[J].World;
        Fit.KinematicReferenceWeights=Weights;Supported=true;break;
    }
    if(!Supported)return Reject(TEXT("german_articulated_support_domain"));
    Diag->SetBoolField(TEXT("articulationAttemptPassed"),true);Diag->SetStringField(TEXT("articulationAttemptReason"),TEXT("actual_contacting_shoes_full_faces_support_domain_passed"));
    Diag->SetNumberField(TEXT("articulatedShoes"),Adjusted);Diag->SetNumberField(TEXT("maximumShoeArticulationCm"),Maximum);
    Diag->SetNumberField(TEXT("shoeArticulationLimitCm"),MaxShoeArticulationCm);Diag->SetNumberField(TEXT("actualContactingShoes"),Points.Num());
    Scan=MoveTemp(Final);return true;
}
FBox LocalHull(const FStaticCache& Body,const FStaticCache& Shoe,const TArray<FTrackPose>& Tracks)
{
    FBox Hull=Body.Bounds;
    for(const auto& Track:Tracks)for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)for(int32 Z=0;Z<2;++Z)
        Hull+=FVector(Track.StoredMatrix.TransformPosition(FVector(X?Shoe.Bounds.Max.X:Shoe.Bounds.Min.X,Y?Shoe.Bounds.Max.Y:Shoe.Bounds.Min.Y,Z?Shoe.Bounds.Max.Z:Shoe.Bounds.Min.Z)));
    return Hull;
}
bool FullHull(AFrontlineTank* V,const FState& S,const FBox& Hull,const FTransform& From,const FTransform& To,FString& Reason,TSharedPtr<FJsonObject> Diag)
{
    if(!Hull.IsValid){Reason=TEXT("german_actual_full_hull_bounds");return false;}
    const FVector Centre=Hull.GetCenter(),Half=Hull.GetExtent()+FVector(12,12,0);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV82GermanFullHull),false);Q.AddIgnoredActor(V);Q.AddIgnoredComponent(S.Ground.Get());Q.AddIgnoredComponent(S.Apron.Get());
    for(const FTransform* T:{&From,&To})
    {
        TArray<FOverlapResult> Hits;V->GetWorld()->OverlapMultiByChannel(Hits,T->TransformPosition(Centre),T->GetRotation(),ECC_WorldStatic,FCollisionShape::MakeBox(Half),Q);
        for(const auto& Hit:Hits)if(Hit.bBlockingHit)
        {Diag->SetStringField(TEXT("firstHullComponent"),GetPathNameSafe(Hit.GetComponent()));Reason=TEXT("german_complete_hull_endpoint_overlap");return false;}
    }
    FVector A,Z,Extent;FQuat Rotation;HullSweepEnvelope(From,To,Centre,Half,A,Z,Rotation,Extent);FHitResult Hit;
    if(V->GetWorld()->SweepSingleByChannel(Hit,A,Z,Rotation,ECC_WorldStatic,FCollisionShape::MakeBox(Extent),Q))
    {Diag->SetStringField(TEXT("firstHullComponent"),GetPathNameSafe(Hit.GetComponent()));Reason=TEXT("german_complete_hull_swept_block");return false;}
    Diag->SetStringField(TEXT("completeHullHalfExtent"),Half.ToString());Diag->SetBoolField(TEXT("completeHullPassed"),true);return true;
}
struct FPlan
{
    FTransform Actor;TArray<FTransform> Tracks;FWarmSupport Warm;bool HasWarm=false;TSharedPtr<FJsonObject> Evidence;
    FAttitudeSeed Attitude;bool HasAttitude=false;
    FGuid Encounter;FString SceneId;uint32 Revision=0;int32 Serial=0,InitialMarkers=0;
    TWeakObjectPtr<UStaticMeshComponent> BodyComponent;TWeakObjectPtr<UInstancedStaticMeshComponent> TrackComponent;
    TWeakObjectPtr<UStaticMesh> BodyMesh,ShoeMesh;
};
bool Plan(AFrontlineTank* V,AWarfareFrontlineGameMode* B,UStaticMeshComponent* Body,UInstancedStaticMeshComponent* Component,
    const TArray<FVector2D>& Path,const TArray<float>& Segments,float Length,float Phase,const FTransform& Previous,
    const FVector& Desired,float Heading,bool Heavy,float TrackY,const FBox& RigidEnvelope,FPlan& Out,const FPlan* Tentative=nullptr)
{
    FState& State=States.FindOrAdd(V);auto Diag=MakeShared<FJsonObject>();FString Reason;
    const double Begin=FPlatformTime::Seconds();
    Diag->SetStringField(TEXT("adapter"),TEXT("v82.german.original.static.finite-face.1"));Diag->SetBoolField(TEXT("shippingAccepted"),false);
    Diag->SetNumberField(TEXT("hostElapsed"),B?B->Elapsed:0);Diag->SetStringField(TEXT("desired"),Desired.ToString());Diag->SetNumberField(TEXT("trackPhaseCm"),Phase);
    auto Fail=[&](const FString& Why)
    {
        Diag->SetBoolField(TEXT("passed"),false);Diag->SetStringField(TEXT("reason"),Why);Diag->SetNumberField(TEXT("totalMs"),(FPlatformTime::Seconds()-Begin)*1000.);State.Last=Diag;if(!State.FirstRejected)State.FirstRejected=Diag;
        if(Why!=State.LastLogReason||(B&&B->Elapsed-State.LastLogHost>10))
        {UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_GERMAN_GROUND_REJECT serial=%d host=%.3f reason=%s desired=%s"),V?V->Serial:0,B?B->Elapsed:0,*Why,*Desired.ToString());State.LastLogReason=Why;State.LastLogHost=B?B->Elapsed:0;}
        return false;
    };
    const wfexp::scene::SceneDefinition* G=nullptr;
    if(!Context(V,B,Body,Component,State,G,Reason))return Fail(Reason);
    if(Desired.ContainsNaN()||!FMath::IsFinite(Heading)||!WarfareFRRUWorldV82::CanTraverse(B,Previous.GetLocation(),Desired))return Fail(TEXT("german_real_native_leg"));
    FStaticCache* BodyCache=nullptr;FStaticCache* ShoeCache=nullptr;
    if(!Cache(Body->GetStaticMesh(),BodyCache,Reason)||!Cache(Component->GetStaticMesh(),ShoeCache,Reason))return Fail(Reason);
    // A first cache insertion can move TMap values. Resolve both after the
    // insertions; no further mesh-cache mutation occurs during this proposal.
    BodyCache=MeshCaches.Find(Body->GetStaticMesh());ShoeCache=MeshCaches.Find(Component->GetStaticMesh());
    if(!BodyCache||!ShoeCache)return Fail(TEXT("german_actual_static_cache_binding"));
    Diag->SetNumberField(TEXT("cachedOriginalBodyVertices"),BodyCache->Vertices.Num());Diag->SetNumberField(TEXT("cachedOriginalBodyFaces"),BodyCache->Faces.Num());
    Diag->SetNumberField(TEXT("cachedOriginalShoeVertices"),ShoeCache->Vertices.Num());Diag->SetNumberField(TEXT("cachedOriginalShoeFaces"),ShoeCache->Faces.Num());
    TArray<FTrackPose> Tracks;
    if(!TracksAt(Path,Segments,Length,Phase,Component->GetInstanceCount()/2,Heavy,TrackY,Tracks,Reason))return Fail(Reason);
    if(Tracks.Num()!=Component->GetInstanceCount())return Fail(TEXT("german_actual_instance_count"));
    Diag->SetNumberField(TEXT("realTrackInstances"),Tracks.Num());
    FBox Hull=LocalHull(*BodyCache,*ShoeCache,Tracks);Hull+=RigidEnvelope;
    // Bound the real shoe throughout every closed path segment, including
    // the prior phase and the segment joins. This is only the whole-volume
    // collision sweep envelope; finite ground faces remain the exact current
    // instances. No envelope corner participates in ground acceptance.
    const double RX=FMath::Max(FMath::Abs(ShoeCache->Bounds.Min.X),FMath::Abs(ShoeCache->Bounds.Max.X));
    const double RZ=FMath::Max(FMath::Abs(ShoeCache->Bounds.Min.Z),FMath::Abs(ShoeCache->Bounds.Max.Z));
    const double Radius=FMath::Sqrt(RX*RX+RZ*RZ);
    const double RY=FMath::Max(FMath::Abs(ShoeCache->Bounds.Min.Y),FMath::Abs(ShoeCache->Bounds.Max.Y))*(Heavy?1.15:1.);
    for(const auto& P:Path)for(int32 Side:{-1,1})
    {
        const FVector Centre(P.X,Side*TrackY,P.Y);const FVector Extent(Radius,RY,Radius);Hull+=Centre-Extent;Hull+=Centre+Extent;
    }
    TArray<FMarker> Markers;double Lowest=DBL_MAX;
    for(int32 I=0;I<Tracks.Num();++I)for(const auto& Vertex:ShoeCache->OuterSole)
    {
        const FVector P(Tracks[I].StoredMatrix.TransformPosition(Vertex));const int32 Side=I<Tracks.Num()/2?-1:1;
        Markers.Add({P,Markers.Num(),(Side<0?0:2)+(P.Y<Tracks[I].Transform.GetLocation().Y?0:1),I,false,Side});Lowest=FMath::Min(Lowest,P.Z);
    }
    if(Markers.Num()>MaxMarkers)return Fail(TEXT("german_real_sole_marker_budget"));
    FBox SupportFootprint(ForceInit);
    for(auto& M:Markers){M.LowerSeedBand=M.NativeActorLocalCm.Z<Lowest+12;if(M.LowerSeedBand)SupportFootprint+=M.NativeActorLocalCm;}
    if(!SupportFootprint.IsValid)return Fail(TEXT("german_lower_run_support_footprint"));
    // Weapons and the upper return belt belong to collision, not suspension
    // support. Keep their complete hull sweep, including bounded articulation.
    Hull=Hull.ExpandBy(MaxShoeArticulationCm);
    const int32 InitialMarkers=Markers.Num();TArray<FNinePoint> Nine;const FTransform Flat(FRotator(0,Heading,0),Desired);
    for(double X:{SupportFootprint.Min.X,SupportFootprint.GetCenter().X,SupportFootprint.Max.X})for(double Y:{SupportFootprint.Min.Y,SupportFootprint.GetCenter().Y,SupportFootprint.Max.Y})
    {
        const FVector P=Flat.TransformPosition({X,Y,0});double Z=0;if(!Footing(*G,P)||!Floor(*G,{P.X,P.Y},Z))return Fail(TEXT("german_native_nine_footing"));Nine.Add({{P.X,P.Y},Z});
    }
    // Only an accepted earlier substep in this same proposal may supply a
    // tentative seed. Witness markers belong to one contact refit and cannot
    // survive into the next phase's original marker binding.
    const FWarmSupport* Warm=State.HasWarm?&State.Warm:nullptr;
    const FAttitudeSeed* Attitude=State.HasAttitude?&State.Attitude:nullptr;
    if(Attitude&&(State.AttitudeBodyComponent.Get()!=Body||State.AttitudeTrackComponent.Get()!=Component
        ||State.AttitudeBodyMesh.Get()!=Body->GetStaticMesh()||State.AttitudeShoeMesh.Get()!=Component->GetStaticMesh()
        ||State.AttitudeInitialMarkers!=InitialMarkers||!FMath::IsFinite(Attitude->A)||!FMath::IsFinite(Attitude->B)
        ||!FMath::IsFinite(Attitude->RootAboveCenterFloorCm)))return Fail(TEXT("german_accepted_attitude_binding"));
    if(Tentative&&(Tentative->HasWarm||Tentative->HasAttitude))
    {
        if(Tentative->Encounter!=State.Encounter||Tentative->SceneId!=State.SceneId||Tentative->Revision!=State.Revision
            ||Tentative->Serial!=State.Serial||Tentative->BodyComponent.Get()!=Body||Tentative->TrackComponent.Get()!=Component
            ||Tentative->BodyMesh.Get()!=Body->GetStaticMesh()||Tentative->ShoeMesh.Get()!=Component->GetStaticMesh()
            ||Tentative->InitialMarkers!=InitialMarkers)return Fail(TEXT("german_tentative_warm_binding"));
        // A refit witness belongs to one phase. It may invalidate the marker
        // tripod seed without discarding the accepted chassis attitude.
        Warm=nullptr;Attitude=nullptr;
        if(Tentative->HasWarm)
        {
            const auto& W=Tentative->Warm;
            if(!FMath::IsFinite(W.A)||!FMath::IsFinite(W.B)||!FMath::IsFinite(W.RootAboveCenterFloorCm))return Fail(TEXT("german_tentative_warm_binding"));
            for(int32 I=0;I<3;++I)if(W.Indices[I]<0||W.Indices[I]>=InitialMarkers
                ||W.Indices[I]==W.Indices[(I+1)%3])return Fail(TEXT("german_tentative_warm_marker_binding"));
            Warm=&W;
        }
        if(Tentative->HasAttitude)
        {
            const auto& A=Tentative->Attitude;
            if(!FMath::IsFinite(A.A)||!FMath::IsFinite(A.B)||!FMath::IsFinite(A.RootAboveCenterFloorCm))return Fail(TEXT("german_tentative_attitude_binding"));
            Attitude=&A;
        }
    }
    FWarmSupport RefitWarm;
    FCandidate Fit;FStats Failure;bool ContactPassed=false;double CentreFloor=0;if(!Floor(*G,{Desired.X,Desired.Y},CentreFloor))return Fail(TEXT("german_native_center_floor"));
    auto Height=[&](const FVector2D& XY,double& Z){return Floor(*G,XY,Z);};
    auto SolverStats=[&](const FStats& Stats)
    {
        Diag->SetNumberField(TEXT("supportFloorQueries"),Stats.FloorQueries);Diag->SetNumberField(TEXT("supportNewtonCandidates"),Stats.NewtonCandidates);
        Diag->SetNumberField(TEXT("supportFullValidations"),Stats.FullValidations);Diag->SetNumberField(TEXT("supportSolveMs"),Stats.SolveMs);
        Diag->SetNumberField(TEXT("supportCandidateTriples"),Stats.Triples);Diag->SetNumberField(TEXT("supportIterations"),Stats.Iterations);
        Diag->SetNumberField(TEXT("supportColdNewtonCandidates"),Stats.ColdNewtonCandidates);Diag->SetBoolField(TEXT("supportColdSlotReserved"),Stats.ColdSlotReserved);
        Diag->SetNumberField(TEXT("supportConvergedCandidates"),Stats.ConvergedCandidates);Diag->SetNumberField(TEXT("supportMarkerRejectedCandidates"),Stats.MarkerRejectedCandidates);
        Diag->SetNumberField(TEXT("supportReferenceRejectedCandidates"),Stats.ReferenceRejectedCandidates);Diag->SetBoolField(TEXT("supportBudgetExhausted"),Stats.BudgetExhausted);
        Diag->SetNumberField(TEXT("supportWorstRejectedMarker"),Stats.WorstRejectedMarker);Diag->SetNumberField(TEXT("supportWorstRejectedElement"),Stats.WorstRejectedElement);
        Diag->SetNumberField(TEXT("supportBestRejectedGapCm"),Stats.BestRejectedGapCm);Diag->SetStringField(TEXT("supportBestRejectedSupport"),Stats.BestRejectedSupport.ToString());
    };
    // Nearby proposals start from the previous accepted suspension attitude.
    // This is a seed, never a contact exemption: all current finite faces,
    // articulated soles, support rays and complete hull sweeps still run.
    bool WarmArticulated=false;FScan WarmScan;
    Diag->SetBoolField(TEXT("acceptedAttitudeSeedAvailable"),Attitude!=nullptr);
    Diag->SetBoolField(TEXT("originalMarkerWarmSeedAvailable"),Warm!=nullptr);
    if(Attitude&&FVector2D(Attitude->A,Attitude->B).Size()<=MaxSlope)
    {
        FCandidate Candidate;Candidate.Params={Attitude->A,Attitude->B,CentreFloor+Attitude->RootAboveCenterFloorCm};
        Candidate.Pose=FTransform(Orientation(Attitude->A,Attitude->B,Heading),FVector(Desired.X,Desired.Y,Candidate.Params.Z));
        // Only a separately valid original-marker tripod may seed the next
        // rigid solve. Current face contacts supply all real supports below.
        for(int32 I=0;I<3;++I)Candidate.Support[I]=Warm?Warm->Indices[I]:INDEX_NONE;
        TArray<FTrackPose> CandidateTracks=Tracks;FString SeedReason;FScan SeedScan;
        FiniteFaces(*BodyCache,*ShoeCache,CandidateTracks,Candidate.Pose,*G,SeedScan,SeedReason);
        if(SeedScan.Complete&&!SeedScan.BodyViolation
            &&ArticulateSoles(*BodyCache,*ShoeCache,CandidateTracks,Candidate.Pose,*G,SeedScan,Candidate,SeedReason,Diag)
            &&FullHull(V,State,Hull,Previous,Candidate.Pose,SeedReason,Diag))
        {Fit=MoveTemp(Candidate);Tracks=MoveTemp(CandidateTracks);WarmScan=MoveTemp(SeedScan);WarmArticulated=true;}
        Diag->SetBoolField(TEXT("previousAcceptedArticulationPassed"),WarmArticulated);
        if(!WarmArticulated)Diag->SetStringField(TEXT("previousAcceptedArticulationRejection"),SeedReason);
    }
    for(int32 Pass=0;Pass<MaxContactPasses;++Pass)
    {
        bool Rigid=WarmArticulated||Find(Markers,Nine,{Desired.X,Desired.Y},Heading,125,ContactToleranceCm,Warm,Height,Fit,Failure,true);
        if(!Rigid)
        {
            SolverStats(Failure);Diag->SetBoolField(TEXT("physicalCollisionReported"),false);
            // A converged, almost supported rigid pose is only a suspension
            // seed. Never turn a search failure into permission to move.
            if(Failure.MarkerRejectedCandidates<=0||Failure.BestRejectedParams.ContainsNaN()
                ||FVector2D(Failure.BestRejectedParams.X,Failure.BestRejectedParams.Y).Size()>MaxSlope
                ||Failure.BestRejectedGapCm< -MaxShoeArticulationCm)return Fail(Failure.FirstBlock);
            Fit.Params=Failure.BestRejectedParams;Fit.Pose=FTransform(Orientation(Fit.Params.X,Fit.Params.Y,Heading),FVector(Desired.X,Desired.Y,Fit.Params.Z));
            Fit.Support[0]=Failure.BestRejectedSupport.X;Fit.Support[1]=Failure.BestRejectedSupport.Y;Fit.Support[2]=Failure.BestRejectedSupport.Z;Fit.Stats=Failure;
        }
        else SolverStats(Fit.Stats);
        FScan Scan;bool Contact=false;
        if(WarmArticulated){Scan=MoveTemp(WarmScan);Contact=true;}
        else Contact=FiniteFaces(*BodyCache,*ShoeCache,Tracks,Fit.Pose,*G,Scan,Reason);
        Diag->SetBoolField(TEXT("rigidTripodSearchPassed"),Rigid&&!WarmArticulated);
        if((!Rigid||(!Contact&&Reason==TEXT("german_actual_shoe_face_penetration")))&&Scan.Complete&&!Scan.BodyViolation)
        {
            const FString RigidReason=Reason;TArray<FTrackPose> ArticulatedTracks=Tracks;
            Contact=ArticulateSoles(*BodyCache,*ShoeCache,ArticulatedTracks,Fit.Pose,*G,Scan,Fit,Reason,Diag);
            if(Contact)Tracks=MoveTemp(ArticulatedTracks);
            else if(!Rigid)return Fail(Reason.IsEmpty()?TEXT("german_articulated_contact_rejected"):Reason);
            else Reason=RigidReason; // Keep the existing rigid witness refit.
        }
        else if(!Rigid)return Fail(Reason.IsEmpty()?Failure.FirstBlock:Reason);
        Diag->SetNumberField(TEXT("finiteFacePairs"),Scan.Pairs);Diag->SetNumberField(TEXT("finiteFacePairBudget"),MaxFacePairs);
        Diag->SetNumberField(TEXT("finiteFaceSeparatedPairs"),Scan.SeparatedPairs);
        Diag->SetNumberField(TEXT("roofCellVisits"),Scan.RoofVisits);Diag->SetNumberField(TEXT("roofCells"),Scan.RoofCells.Num());
        Diag->SetNumberField(TEXT("actualFacesVisited"),Scan.FaceVisits);Diag->SetNumberField(TEXT("conservativelyCulledFaces"),Scan.Culled);
        Diag->SetBoolField(TEXT("finiteFaceScanComplete"),Scan.Complete);Diag->SetNumberField(TEXT("minimumFiniteFaceGapCm"),Scan.Minimum==DBL_MAX?0:Scan.Minimum);
        Diag->SetNumberField(TEXT("contactToleranceCm"),ContactToleranceCm);Diag->SetStringField(TEXT("candidate"),Fit.Pose.GetLocation().ToString());
        Diag->SetStringField(TEXT("candidateRotation"),Fit.Pose.Rotator().ToString());Diag->SetNumberField(TEXT("contactRefits"),Pass);
        if(!Contact)
        {
            if(Reason!=TEXT("german_actual_shoe_face_penetration")||!Scan.Complete||Pass+1>=MaxContactPasses)return Fail(Reason);
            int32 Added=0;
            for(const auto& W:Scan.Witness)if(W.Valid)
            {
                bool Duplicate=false;for(const auto& M:Markers)if(M.NativeActorLocalCm.Equals(W.Local,1.e-7)){Duplicate=true;break;}
                if(Duplicate)continue;if(Markers.Num()>=MaxMarkers)return Fail(TEXT("german_actual_witness_marker_budget"));
                Markers.Add({W.Local,Markers.Num(),W.Strip,W.Element,W.Local.Z<Lowest+12,W.Side});++Added;
            }
            if(!Added)return Fail(TEXT("german_actual_witness_no_progress"));
            for(int32 I=0;I<3;++I)RefitWarm.Indices[I]=Fit.Support[I];RefitWarm.A=Fit.Params.X;RefitWarm.B=Fit.Params.Y;RefitWarm.RootAboveCenterFloorCm=Fit.Params.Z-CentreFloor;Warm=&RefitWarm;continue;
        }
        if(!Scan.Complete||Scan.Minimum< -ContactToleranceCm)return Fail(TEXT("german_final_finite_face_incomplete"));
        FCollisionQueryParams Rays(SCENE_QUERY_STAT(FRRUV82GermanSole),true);
        TArray<TSharedPtr<FJsonValue>> Supports;
        for(int32 I=0;I<3;++I)
        {
            const FVector P=Fit.WorldSupport[I];double Z=0;FHitResult Hit;
            UStaticMeshComponent* Ground=(P.X<G->minX||P.X>G->maxX)?State.Apron.Get():State.Ground.Get();
            if(!Ground||!Footing(*G,P)||!Floor(*G,{P.X,P.Y},Z)||!Ground->LineTraceComponent(Hit,P+FVector(0,0,1000),P-FVector(0,0,1000),Rays)
                ||Hit.Component.Get()!=Ground||FMath::Abs(Hit.ImpactPoint.Z-Z)>.1||FMath::Abs(P.Z-Z)>1.e-3)return Fail(TEXT("german_real_support_ray_or_contact"));
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("component"),Ground->GetPathName());J->SetStringField(TEXT("world"),P.ToString());J->SetNumberField(TEXT("nativeHeightCm"),Z);J->SetNumberField(TEXT("actualHitHeightCm"),Hit.ImpactPoint.Z);Supports.Add(MakeShared<FJsonValueObject>(J));
        }
        Diag->SetArrayField(TEXT("supports"),Supports);ContactPassed=true;break;
    }
    if(!ContactPassed)return Fail(TEXT("german_contact_pass_budget"));
    if(!FullHull(V,State,Hull,Previous,Fit.Pose,Reason,Diag))return Fail(Reason);
    Out.Actor=Fit.Pose;Out.Tracks.Reset(Tracks.Num());for(const auto& T:Tracks)Out.Tracks.Add(T.Transform);
    // These original marker indices are mathematical seeds only. Even after
    // articulation, the next proposal re-solves them and checks current faces.
    Out.HasWarm=true;for(int32 I=0;I<3;++I){Out.Warm.Indices[I]=Fit.Support[I];Out.HasWarm&=Fit.Support[I]>=0&&Fit.Support[I]<InitialMarkers;}
    Out.Warm.A=Fit.Params.X;Out.Warm.B=Fit.Params.Y;Out.Warm.RootAboveCenterFloorCm=Fit.Params.Z-CentreFloor;
    Out.Attitude={Fit.Params.X,Fit.Params.Y,Fit.Params.Z-CentreFloor};Out.HasAttitude=true;
    Out.Encounter=State.Encounter;Out.SceneId=State.SceneId;Out.Revision=State.Revision;Out.Serial=State.Serial;Out.InitialMarkers=InitialMarkers;
    Out.BodyComponent=Body;Out.TrackComponent=Component;Out.BodyMesh=Body->GetStaticMesh();Out.ShoeMesh=Component->GetStaticMesh();
    Diag->SetBoolField(TEXT("passed"),true);Diag->SetStringField(TEXT("reason"),TEXT("exact_current_static_faces_support_and_full_hull_passed"));
    Diag->SetBoolField(TEXT("acceptedAttitudeSeedRetained"),Out.HasAttitude);
    Diag->SetBoolField(TEXT("originalMarkerWarmSeedRetained"),Out.HasWarm);
    Diag->SetStringField(TEXT("sceneId"),State.SceneId);Diag->SetNumberField(TEXT("sceneRevision"),State.Revision);
    Diag->SetNumberField(TEXT("totalMs"),(FPlatformTime::Seconds()-Begin)*1000.);Out.Evidence=Diag;return true;
}
void Committed(AFrontlineTank* V,const FPlan& Plan)
{
    auto& S=States.FindOrAdd(V);S.Warm=Plan.Warm;S.HasWarm=Plan.HasWarm;S.Last=Plan.Evidence;
    S.Attitude=Plan.Attitude;S.HasAttitude=Plan.HasAttitude;S.AttitudeInitialMarkers=Plan.InitialMarkers;
    S.AttitudeBodyComponent=Plan.BodyComponent;S.AttitudeTrackComponent=Plan.TrackComponent;
    S.AttitudeBodyMesh=Plan.BodyMesh;S.AttitudeShoeMesh=Plan.ShoeMesh;
}
TSharedPtr<FJsonObject> Diagnosis(const AFrontlineTank* V)
{const auto* S=States.Find(TWeakObjectPtr<AFrontlineTank>(const_cast<AFrontlineTank*>(V)));return S?S->Last:nullptr;}
} // FRRUGermanStaticContactV82
