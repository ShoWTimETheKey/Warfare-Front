// Cold, receiver-only Editor preflight. This is not visual/runtime acceptance.
// The old German ingress repair and its thresholds are deliberately separate.
#pragma once
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PackedNormal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Materials/MaterialInterface.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "../WarfareFRRUWorldRenderV826.generated.inl"

namespace FRRUWorldRenderBasisV826
{
struct FPin
{
    FSHA1 Hash;
    template<class T> void Bytes(const T& V){Hash.Update(reinterpret_cast<const uint8*>(&V),sizeof(V));}
    void Text(const FString& V){FTCHARToUTF8 U(*V);Bytes(U.Length());Hash.Update(reinterpret_cast<const uint8*>(U.Get()),U.Length());}
    void Record(const TArray<uint8>& V){Bytes(V.Num());if(!V.IsEmpty())Hash.Update(V.GetData(),V.Num());}
    FString Finish(){Hash.Final();uint8 D[FSHA1::DigestSize];Hash.GetHash(D);return BytesToHex(D,FSHA1::DigestSize);}
};
template<class T> void Append(TArray<uint8>& B,const T& V){B.Append(reinterpret_cast<const uint8*>(&V),sizeof(V));}
int32 Compare(const TArray<uint8>& A,const TArray<uint8>& B)
{const int32 N=FMath::Min(A.Num(),B.Num()),C=N?FMemory::Memcmp(A.GetData(),B.GetData(),N):0;return C?C:(A.Num()<B.Num()?-1:(A.Num()>B.Num()?1:0));}
bool Finite(const FVector3d& V){return FMath::IsFinite(V.X)&&FMath::IsFinite(V.Y)&&FMath::IsFinite(V.Z);}
bool Zero(const FVector3d& V){return V.X==0.&&V.Y==0.&&V.Z==0.;}
// No positional tolerance: a positive double-precision surface is never
// discarded. Scaling before division also supports microscopic valid faces.
bool Unit(const FVector3d& V,FVector3d& Out)
{
    if(!Finite(V))return false;const double M=FMath::Max3(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));
    if(M==0.)return false;const FVector3d Q=V/M;const double L=FMath::Sqrt(FVector3d::DotProduct(Q,Q));
    if(!FMath::IsFinite(L)||L==0.)return false;Out=Q/L;return Finite(Out)&&!Zero(Out);
}
FVector3f Packed(const FVector3f& V,bool High)
{return High?FPackedRGBA16N(V).ToFVector3f():FPackedNormal(V).ToFVector3f();}

FString DescriptionPin(const FMeshDescription& D)
{
    FStaticMeshConstAttributes A(D);FPin H;const auto P=A.GetVertexPositions();const auto C=A.GetVertexInstanceColors();
    const auto UV=A.GetVertexInstanceUVs();const auto Hard=A.GetEdgeHardnesses();const auto Slots=A.GetPolygonGroupMaterialSlotNames();
    H.Bytes(D.Vertices().Num());for(const auto ID:D.Vertices().GetElementIDs()){H.Bytes(ID.GetValue());H.Bytes(P[ID]);}
    H.Bytes(D.VertexInstances().Num());H.Bytes(UV.GetNumChannels());for(const auto ID:D.VertexInstances().GetElementIDs())
    {H.Bytes(ID.GetValue());H.Bytes(D.GetVertexInstanceVertex(ID).GetValue());for(int32 K=0;K<UV.GetNumChannels();++K)H.Bytes(UV.Get(ID,K));H.Bytes(C[ID]);}
    H.Bytes(D.Triangles().Num());for(const auto ID:D.Triangles().GetElementIDs())
    {H.Bytes(ID.GetValue());H.Bytes(D.GetTrianglePolygon(ID).GetValue());H.Bytes(D.GetTrianglePolygonGroup(ID).GetValue());for(const auto VI:D.GetTriangleVertexInstances(ID))H.Bytes(VI.GetValue());}
    H.Bytes(D.Polygons().Num());for(const auto ID:D.Polygons().GetElementIDs())
    {H.Bytes(ID.GetValue());H.Bytes(D.GetPolygonPolygonGroup(ID).GetValue());const auto VIs=D.GetPolygonVertexInstances(ID);H.Bytes(VIs.Num());for(const auto VI:VIs)H.Bytes(VI.GetValue());const auto Tris=D.GetPolygonTriangles(ID);H.Bytes(Tris.Num());for(const auto T:Tris)H.Bytes(T.GetValue());}
    H.Bytes(D.PolygonGroups().Num());for(const auto ID:D.PolygonGroups().GetElementIDs()){H.Bytes(ID.GetValue());H.Text(Slots[ID].ToString());}
    H.Bytes(D.Edges().Num());for(const auto ID:D.Edges().GetElementIDs()){H.Bytes(ID.GetValue());for(const auto V:D.GetEdgeVertices(ID))H.Bytes(V.GetValue());H.Bytes(bool(Hard[ID]));}
    return H.Finish();
}
struct FBadCorner {int32 ID=INDEX_NONE;bool N=false,T=false,S=false;};
FString BasisPin(const FMeshDescription& D,const TMap<int32,FBadCorner>& Bad,bool HealthyOnly)
{
    FStaticMeshConstAttributes A(D);const auto N=A.GetVertexInstanceNormals(),T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();FPin H;
    for(const auto ID:D.VertexInstances().GetElementIDs())
    {const FBadCorner* B=Bad.Find(ID.GetValue());if(HealthyOnly&&B)continue;H.Bytes(ID.GetValue());H.Bytes(N[ID]);H.Bytes(T[ID]);H.Bytes(S[ID]);}
    return H.Finish();
}
// Stronger than the healthy-corner assertion: preserve healthy components
// even inside a corner whose other component requires repair.
FString HealthyComponentPin(const FMeshDescription& D,const TMap<int32,FBadCorner>& Bad)
{
    FStaticMeshConstAttributes A(D);const auto N=A.GetVertexInstanceNormals(),T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();FPin H;
    for(const auto ID:D.VertexInstances().GetElementIDs())
    {const FBadCorner* B=Bad.Find(ID.GetValue());H.Bytes(ID.GetValue());if(!B||!B->N){H.Bytes(uint8(0));H.Bytes(N[ID]);}if(!B||!B->T){H.Bytes(uint8(1));H.Bytes(T[ID]);}if(!B||!B->S){H.Bytes(uint8(2));H.Bytes(S[ID]);}}
    return H.Finish();
}
FString MaterialsPin(UStaticMesh* Mesh)
{
    FPin H;H.Bytes(Mesh->GetStaticMaterials().Num());for(const auto& S:Mesh->GetStaticMaterials())
    {H.Text(S.MaterialSlotName.ToString());H.Text(S.ImportedMaterialSlotName.ToString());H.Text(S.MaterialInterface?S.MaterialInterface->GetPathName():FString());H.Text(S.OverlayMaterialInterface?S.OverlayMaterialInterface->GetPathName():FString());}
    return H.Finish();
}
FString CollisionPin(UStaticMesh* Mesh)
{
    const auto* B=Mesh->GetBodySetup();if(!B)return {};FPin H;
    H.Bytes(uint8(B->CollisionTraceFlag));H.Bytes(bool(B->bDoubleSidedGeometry));H.Bytes(bool(B->bGenerateNonMirroredCollision));H.Bytes(bool(B->bGenerateMirroredCollision));
    H.Text(B->PhysMaterial?B->PhysMaterial->GetPathName():FString());H.Bytes(Mesh->LODForCollision);H.Bytes(B->BuildScale3D);
    H.Bytes(uint8(B->WalkableSlopeOverride.GetWalkableSlopeBehavior()));H.Bytes(B->WalkableSlopeOverride.GetWalkableSlopeAngle());
    H.Text(B->DefaultInstance.GetCollisionProfileName().ToString());H.Bytes(uint8(B->DefaultInstance.GetCollisionEnabled(false)));H.Bytes(uint8(B->DefaultInstance.GetObjectType()));
    for(int32 C=0;C<int32(ECC_OverlapAll_Deprecated);++C)H.Bytes(uint8(B->DefaultInstance.GetResponseToChannel(ECollisionChannel(C))));
    H.Bytes(B->AggGeom.GetElementCount());return H.Finish();
}
bool NoCollision(UStaticMesh* Mesh)
{
    const auto* B=Mesh?Mesh->GetBodySetup():nullptr;return B&&B->AggGeom.GetElementCount()==0
        &&B->DefaultInstance.GetCollisionEnabled(false)==ECollisionEnabled::NoCollision
        &&B->DefaultInstance.GetCollisionProfileName()==FName(TEXT("NoCollision"));
}
TArray<uint8> SourceCorner(const FMeshDescription& D,FVertexInstanceID ID,bool Colour)
{
    FStaticMeshConstAttributes A(D);const auto P=A.GetVertexPositions();const auto UV=A.GetVertexInstanceUVs();TArray<uint8> B;
    Append(B,P[D.GetVertexInstanceVertex(ID)]);for(int32 K=0;K<UV.GetNumChannels();++K)Append(B,UV.Get(ID,K));
    if(Colour)Append(B,FLinearColor(A.GetVertexInstanceColors()[ID]).ToFColor(true));return B;
}
TArray<uint8> RenderCorner(const FStaticMeshVertexBuffers& V,uint32 ID,bool Colour)
{
    TArray<uint8> B;Append(B,V.PositionVertexBuffer.VertexPosition(ID));for(uint32 K=0;K<V.StaticMeshVertexBuffer.GetNumTexCoords();++K)Append(B,V.StaticMeshVertexBuffer.GetVertexUV(ID,K));
    if(Colour)Append(B,V.ColorVertexBuffer.VertexColor(ID));return B;
}
FString RecordPin(const TArray<uint8>& B){FPin H;H.Record(B);return H.Finish();}

// Failure-only, bounded correspondence diagnosis. It does not participate
// in acceptance and never substitutes approximate positions/UVs/colours.
TSharedPtr<FJsonObject> CornerMismatch(const FMeshDescription& D,FVertexInstanceID ID,UStaticMesh* Mesh,bool Colour)
{
    auto J=MakeShared<FJsonObject>();FStaticMeshConstAttributes A(D);const auto UV=A.GetVertexInstanceUVs();const auto P=A.GetVertexPositions();
    const FVector3f Point=P[D.GetVertexInstanceVertex(ID)],N=A.GetVertexInstanceNormals()[ID],T=A.GetVertexInstanceTangents()[ID];
    FVector3f BuildN=N,BuildT=T;BuildN.Normalize();BuildT.Normalize();const FLinearColor C(A.GetVertexInstanceColors()[ID]);
    J->SetNumberField(TEXT("vertexInstance"),ID.GetValue());J->SetStringField(TEXT("sourcePosition"),Point.ToString());J->SetStringField(TEXT("sourceNormal"),N.ToString());J->SetStringField(TEXT("sourceTangent"),T.ToString());
    J->SetNumberField(TEXT("sourceNormalLengthSquared"),N.SizeSquared());J->SetNumberField(TEXT("sourceTangentLengthSquared"),T.SizeSquared());J->SetNumberField(TEXT("sourceSign"),A.GetVertexInstanceBinormalSigns()[ID]);
    J->SetStringField(TEXT("engineNormalizedNormal"),BuildN.ToString());J->SetStringField(TEXT("engineNormalizedTangent"),BuildT.ToString());J->SetStringField(TEXT("sourceColourLinear"),C.ToString());
    J->SetStringField(TEXT("sourceColourEngineSRGBRGBA"),C.ToFColor(true).ToHex());J->SetStringField(TEXT("sourceColourLinearRGBA"),C.ToFColor(false).ToHex());
    TArray<TSharedPtr<FJsonValue>> SourceUV;for(int32 U=0;U<UV.GetNumChannels();++U){const FVector2f V=UV.Get(ID,U);TArray<TSharedPtr<FJsonValue>> Q{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y)};SourceUV.Add(MakeShared<FJsonValueArray>(Q));}J->SetArrayField(TEXT("sourceUV"),SourceUV);
    const auto& VB=Mesh->GetRenderData()->LODResources[0].VertexBuffers;const uint32 Count=VB.PositionVertexBuffer.GetNumVertices();int32 SamePosition=0,ExactKeys=0;TArray<TSharedPtr<FJsonValue>> Matches;
    const auto Source=SourceCorner(D,ID,Colour);
    for(uint32 V=0;V<Count;++V)if(VB.PositionVertexBuffer.VertexPosition(V)==Point)
    {
        ++SamePosition;const bool KeyExact=Compare(Source,RenderCorner(VB,V,Colour))==0;ExactKeys+=KeyExact;
        if(Matches.Num()>=8)continue;auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("renderVertex"),V);Row->SetBoolField(TEXT("exactPositionUVColourBytes"),KeyExact);
        const FVector4f RN=VB.StaticMeshVertexBuffer.VertexTangentZ(V),RT=VB.StaticMeshVertexBuffer.VertexTangentX(V);const FVector3f RY=VB.StaticMeshVertexBuffer.VertexTangentY(V);
        Row->SetStringField(TEXT("normal"),RN.ToString());Row->SetStringField(TEXT("tangentX"),RT.ToString());Row->SetStringField(TEXT("tangentY"),RY.ToString());
        Row->SetNumberField(TEXT("normalLengthSquared"),FVector3f(RN.X,RN.Y,RN.Z).SizeSquared());Row->SetNumberField(TEXT("tangentXLengthSquared"),FVector3f(RT.X,RT.Y,RT.Z).SizeSquared());Row->SetNumberField(TEXT("tangentYLengthSquared"),RY.SizeSquared());
        if(Colour)Row->SetStringField(TEXT("colourRGBA"),VB.ColorVertexBuffer.VertexColor(V).ToHex());TArray<TSharedPtr<FJsonValue>> ActualUV;
        for(uint32 U=0;U<VB.StaticMeshVertexBuffer.GetNumTexCoords();++U){const FVector2f Q=VB.StaticMeshVertexBuffer.GetVertexUV(V,U);TArray<TSharedPtr<FJsonValue>> Numbers{MakeShared<FJsonValueNumber>(Q.X),MakeShared<FJsonValueNumber>(Q.Y)};ActualUV.Add(MakeShared<FJsonValueArray>(Numbers));}Row->SetArrayField(TEXT("uv"),ActualUV);Matches.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetNumberField(TEXT("sameExactPositionVertices"),SamePosition);J->SetNumberField(TEXT("exactPositionUVColourVertices"),ExactKeys);J->SetArrayField(TEXT("actualSamePositionSamples"),Matches);return J;
}

// Vertex welding/storage order may change after a basis repair. The exact
// oriented surface, winding, face multiplicity, UVs, colours and per-material
// section flags must not. Cyclic rotation is allowed; reversal is not.
TSharedPtr<FJsonObject> RenderProof(UStaticMesh* Mesh,TSet<FString>* InvalidKeys=nullptr)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);const auto Reject=[&](const TCHAR* Why){J->SetStringField(TEXT("reason"),Why);return J;};
    const auto* R=Mesh?Mesh->GetRenderData():nullptr;if(!R||R->LODResources.Num()!=1)return Reject(TEXT("single render LOD unavailable"));
    const auto& L=R->LODResources[0];const auto& V=L.VertexBuffers;const auto I=L.IndexBuffer.GetArrayView();
    const uint32 Count=V.PositionVertexBuffer.GetNumVertices(),UVs=V.StaticMeshVertexBuffer.GetNumTexCoords();const bool Colour=V.ColorVertexBuffer.GetNumVertices()!=0;
    if(!Count||UVs==0||UVs>8||I.Num()==0||I.Num()%3||Count>2000000||I.Num()>6000000
        ||!V.PositionVertexBuffer.GetVertexData()||!V.StaticMeshVertexBuffer.GetTangentData()||!V.StaticMeshVertexBuffer.GetTexCoordData()
        ||(Colour&&(V.ColorVertexBuffer.GetNumVertices()!=Count||!V.ColorVertexBuffer.GetVertexData())))return Reject(TEXT("CPU stream contract or bounded budget"));
    const int64 CornerSize=12+int64(UVs)*8+(Colour?4:0);if((int64(Count)+I.Num())*CornerSize>192*1024*1024)return Reject(TEXT("surface proof memory bound"));
    TArray<TArray<uint8>> Corners;Corners.SetNum(Count);int32 ZeroN=0,ZeroX=0,ZeroY=0,NonFinite=0;
    for(uint32 K=0;K<Count;++K)
    {
        const auto P=V.PositionVertexBuffer.VertexPosition(K);if(P.ContainsNaN())return Reject(TEXT("nonfinite render position"));
        for(uint32 U=0;U<UVs;++U)if(V.StaticMeshVertexBuffer.GetVertexUV(K,U).ContainsNaN())return Reject(TEXT("nonfinite render UV"));
        Corners[K]=RenderCorner(V,K,Colour);const FVector4f Z=V.StaticMeshVertexBuffer.VertexTangentZ(K),X=V.StaticMeshVertexBuffer.VertexTangentX(K);const FVector3f Y=V.StaticMeshVertexBuffer.VertexTangentY(K);
        const bool BadN=Z.ContainsNaN()||FVector3f(Z.X,Z.Y,Z.Z).SizeSquared()<1.e-12f;
        const bool BadX=X.ContainsNaN()||FVector3f(X.X,X.Y,X.Z).SizeSquared()<1.e-12f,BadY=Y.ContainsNaN()||Y.SizeSquared()<1.e-12f;
        ZeroN+=BadN;ZeroX+=BadX;ZeroY+=BadY;NonFinite+=Z.ContainsNaN()||X.ContainsNaN()||Y.ContainsNaN();
        if(InvalidKeys&&(BadN||BadX||BadY))InvalidKeys->Add(RecordPin(Corners[K]));
    }
    FPin Surface;Surface.Bytes(UVs);Surface.Bytes(Colour);Surface.Bytes(I.Num());Surface.Bytes(L.Sections.Num());TBitArray<> Covered(false,I.Num());TArray<TSharedPtr<FJsonValue>> Sections;
    for(int32 SIndex=0;SIndex<L.Sections.Num();++SIndex)
    {
        const auto& S=L.Sections[SIndex];const int64 End=int64(S.FirstIndex)+int64(S.NumTriangles)*3;
        if(S.FirstIndex%3||End>I.Num())return Reject(TEXT("render section range invalid"));
        Surface.Bytes(SIndex);Surface.Bytes(S.MaterialIndex);Surface.Bytes(S.NumTriangles);Surface.Bytes(bool(S.bEnableCollision));Surface.Bytes(bool(S.bCastShadow));
        TArray<TArray<uint8>> Faces;Faces.Reserve(S.NumTriangles);
        for(int64 F=S.FirstIndex;F<End;F+=3)
        {
            uint32 VIs[3];for(int32 K=0;K<3;++K){if(Covered[int32(F)+K])return Reject(TEXT("overlapping render sections"));Covered[int32(F)+K]=true;VIs[K]=I[int32(F)+K];if(VIs[K]>=Count)return Reject(TEXT("render index out of bounds"));}
            int32 Start=0;for(int32 Candidate=1;Candidate<3;++Candidate)for(int32 K=0;K<3;++K)
            {const int32 C=Compare(Corners[VIs[(Candidate+K)%3]],Corners[VIs[(Start+K)%3]]);if(C<0){Start=Candidate;break;}if(C>0)break;}
            auto& Face=Faces.AddDefaulted_GetRef();Face.Reserve(int32(CornerSize*3));for(int32 K=0;K<3;++K)Face.Append(Corners[VIs[(Start+K)%3]]);
        }
        Faces.Sort([](const TArray<uint8>& A,const TArray<uint8>& B){return Compare(A,B)<0;});for(const auto& Face:Faces)Surface.Record(Face);
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("section"),SIndex);Row->SetNumberField(TEXT("materialIndex"),S.MaterialIndex);Row->SetNumberField(TEXT("triangles"),S.NumTriangles);Row->SetBoolField(TEXT("collision"),S.bEnableCollision);Row->SetBoolField(TEXT("castShadow"),S.bCastShadow);Sections.Add(MakeShared<FJsonValueObject>(Row));
    }
    for(int32 K=0;K<Covered.Num();++K)if(!Covered[K])return Reject(TEXT("render unassigned index"));
    J->SetStringField(TEXT("orientedNonBasisSurfaceSHA1"),Surface.Finish());J->SetNumberField(TEXT("vertices"),Count);J->SetNumberField(TEXT("triangles"),I.Num()/3);J->SetNumberField(TEXT("uvChannels"),UVs);J->SetBoolField(TEXT("coloursPresent"),Colour);J->SetArrayField(TEXT("sections"),Sections);
    J->SetNumberField(TEXT("zeroNormalVertices"),ZeroN);J->SetNumberField(TEXT("zeroTangentXVertices"),ZeroX);J->SetNumberField(TEXT("zeroTangentYVertices"),ZeroY);J->SetNumberField(TEXT("nonFiniteBasisVertices"),NonFinite);J->SetBoolField(TEXT("success"),true);return J;
}

bool PinnedPart(UStaticMesh* Mesh,FString& SourceSHA,FString& Token,FString& Why)
{
    // One fresh, rejected V87 art remainder. Reuse this invalid-corner-only
    // repair; the production registry and its existing guards stay intact.
    if(Mesh&&Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/SM_WF_fr_vaux_ruins_000_V87_dressing.SM_WF_fr_vaux_ruins_000_V87_dressing"))
    {
        TArray<uint8> File;FSHAHash Hash;
        const FString Path=FPaths::ProjectDir()+TEXT("Content/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/SM_WF_fr_vaux_ruins_000_V87_dressing.uasset");
        if(Mesh->GetOutermost()->IsDirty()||!FFileHelper::LoadFileToArray(File,*Path))
        {Why=TEXT("owned ruin remainder must match its clean saved package");return false;}
        FSHA1::HashBuffer(File.GetData(),File.Num(),Hash.Hash);
        SourceSHA=TEXT("f6f58f84d8c38db885e68fdf4f21b4ade26e4997daa26ac799090fb6fd95d3c0");
        if(!Hash.ToString().Equals(TEXT("DC538B579548F912B4E7D22CF81EB4326F4DE0DF"),ESearchCase::IgnoreCase)
            ||!SourceSHA.Equals(Mesh->GetOutermost()->GetMetaData().GetValue(Mesh,TEXT("WF_Source_SHA256")),ESearchCase::IgnoreCase))
        {Why=TEXT("owned ruin remainder package or FBX provenance differs");return false;}
        Token=TEXT("fr_vaux_ruins_000_v87");return true;
    }
    if(!Mesh||!Mesh->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldRenderV826/CorrectiveV1/Scenes/"))
        ||!WarfareFRRUWorldRenderV826::Registered(Mesh->GetPathName())){Why=TEXT("mesh outside exact current CorrectiveV1 registry");return false;}
    int32 Section=INDEX_NONE,Index=INDEX_NONE;for(const auto& E:WarfareFRRUWorldRenderV826::Entries)for(int32 S=0;S<3;++S)for(int32 K=0;K<WarfareFRRUWorldRenderV826::Count(&E,S);++K)
        if(Mesh->GetPathName()==WarfareFRRUWorldRenderV826::Path(&E,S,K)){Token=E.Token;Section=S;Index=K;}
    const TCHAR* Sections[]={TEXT("ground"),TEXT("solids"),TEXT("dressing")};FString Text;
    const FString File=FPaths::ProjectDir()+TEXT("ArtSource/FRRU/V82.6/WorldRender/CorrectiveV1/")+Token+TEXT("/manifest.json");
    TSharedPtr<FJsonObject> Manifest;if(Section==INDEX_NONE||!FFileHelper::LoadFileToString(Text,*File)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Manifest)||!Manifest.IsValid()){Why=TEXT("pinned source manifest unavailable");return false;}
    const TSharedPtr<FJsonObject>* SectionObject=nullptr;const TSharedPtr<FJsonObject>* AllSections=nullptr;const TArray<TSharedPtr<FJsonValue>>* Parts=nullptr;
    if(!Manifest->TryGetObjectField(TEXT("sections"),AllSections)||!(*AllSections)->TryGetObjectField(Sections[Section],SectionObject)||!(*SectionObject)->TryGetArrayField(TEXT("parts"),Parts)||!Parts->IsValidIndex(Index)){Why=TEXT("registered part absent from pinned manifest");return false;}
    const auto Part=(*Parts)[Index]->AsObject();if(!Part.IsValid()||!Part->TryGetStringField(TEXT("sha256"),SourceSHA)||SourceSHA.Len()!=64){Why=TEXT("pinned part SHA256 unavailable");return false;}
    const FString Actual=Mesh->GetOutermost()->GetMetaData().GetValue(Mesh,TEXT("WF_Source_SHA256"));
    if(!SourceSHA.Equals(Actual,ESearchCase::IgnoreCase)){Why=TEXT("actual imported source metadata differs from pinned part");return false;}return true;
}

struct FLocalBasis
{
    FVector3d FaceNormal=FVector3d::ZeroVector,Tangent=FVector3d::ZeroVector,Bitangent=FVector3d::ZeroVector;
    int32 NormalTriangle=INDEX_NONE,TangentTriangle=INDEX_NONE;double DoubleArea=0.,UVDeterminant=0.;bool RankOneUVFallback=false,GeometricFallback=false;
};
bool LocalBasis(const FMeshDescription& D,FVertexInstanceID ID,const FVector3d& Normal,bool NeedDerivative,FLocalBasis& Out)
{
    FStaticMeshConstAttributes A(D);const auto P=A.GetVertexPositions();const auto UV=A.GetVertexInstanceUVs();
    double BestDet=0.,BestRankOne=0.,BestEdge=0.;FVector3d RankOneT=FVector3d::ZeroVector,EdgeT=FVector3d::ZeroVector;
    int32 RankOneTriangle=INDEX_NONE,EdgeTriangle=INDEX_NONE;double RankOneArea=0.,EdgeArea=0.;
    for(const auto Triangle:D.GetVertexInstanceConnectedTriangleIDs(ID))
    {
        const auto V=D.GetTriangleVertexInstances(Triangle);const FVector3d P0(P[D.GetVertexInstanceVertex(V[0])]),P1(P[D.GetVertexInstanceVertex(V[1])]),P2(P[D.GetVertexInstanceVertex(V[2])]);
        const FVector3d E1=P1-P0,E2=P2-P0,Cross=FVector3d::CrossProduct(E1,E2);FVector3d FN;
        if(!Unit(Cross,FN))continue;const double Area=Cross.Size();if(!FMath::IsFinite(Area)||Area==0.)continue;
        if(Area>Out.DoubleArea){Out.FaceNormal=FN;Out.DoubleArea=Area;Out.NormalTriangle=Triangle.GetValue();}
        if(!NeedDerivative)continue;
        const FVector2d U0(UV.Get(V[0],0)),U1(UV.Get(V[1],0)),U2(UV.Get(V[2],0));const FVector2d D1=U1-U0,D2=U2-U0;
        if(D1.ContainsNaN()||D2.ContainsNaN())return false;
        const double Det=D1.X*D2.Y-D1.Y*D2.X;if(!FMath::IsFinite(Det))return false;
        if(Det!=0.)
        {
            const FVector3d RawT=(E1*D2.Y-E2*D1.Y)/Det,RawB=(E2*D1.X-E1*D2.X)/Det;FVector3d Projected;
            if(Finite(RawB)&&Unit(RawT-Normal*FVector3d::DotProduct(RawT,Normal),Projected)&&FMath::Abs(Det)>BestDet)
            {Out.Tangent=Projected;Out.Bitangent=RawB;Out.TangentTriangle=Triangle.GetValue();Out.UVDeterminant=Det;BestDet=FMath::Abs(Det);}
        }
        if(Det==0.)
        {
            // Rank-one UVs still carry a genuine scalar direction. Recover
            // its surface gradient from the positive-area geometry; no UV
            // or positional tolerance and no UV edit is involved.
            const double CrossSquared=FVector3d::DotProduct(Cross,Cross);
            const FVector3d G1=FVector3d::CrossProduct(E2,Cross),G2=FVector3d::CrossProduct(Cross,E1);
            const FVector3d GradU=(G1*D1.X+G2*D2.X)/CrossSquared,GradV=(G1*D1.Y+G2*D2.Y)/CrossSquared;
            FVector3d Projected;bool Valid=Unit(GradU-Normal*FVector3d::DotProduct(GradU,Normal),Projected);
            if(!Valid)Valid=Unit(FVector3d::CrossProduct(GradV,Normal),Projected);
            const double Variation=FMath::Max(FMath::Max(FMath::Abs(D1.X),FMath::Abs(D2.X)),FMath::Max(FMath::Abs(D1.Y),FMath::Abs(D2.Y)));
            if(Valid&&Variation>BestRankOne){BestRankOne=Variation;RankOneT=Projected;RankOneTriangle=Triangle.GetValue();RankOneArea=Area;}
        }
        // Only called for an exact actual CPU defect and an invalid source
        // component. If UVs supply no usable local direction, retain every
        // positive-area surface and construct an orthogonal geometric basis.
        const FVector3d Edges[]={E1,E2,P2-P1};for(const auto& Edge:Edges)
        {FVector3d Projected;const double LengthSquared=Edge.SizeSquared();if(LengthSquared>BestEdge&&Unit(Edge-Normal*FVector3d::DotProduct(Edge,Normal),Projected))
            {BestEdge=LengthSquared;EdgeT=Projected;EdgeTriangle=Triangle.GetValue();EdgeArea=Area;}}
    }
    if(Out.NormalTriangle==INDEX_NONE)return false;if(!NeedDerivative)return true;
    if(BestDet>0.)return true;
    if(RankOneTriangle!=INDEX_NONE){Out.Tangent=RankOneT;Out.TangentTriangle=RankOneTriangle;Out.DoubleArea=RankOneArea;Out.RankOneUVFallback=true;}
    else if(EdgeTriangle!=INDEX_NONE){Out.Tangent=EdgeT;Out.TangentTriangle=EdgeTriangle;Out.DoubleArea=EdgeArea;Out.GeometricFallback=true;}
    else return false;
    Out.Bitangent=FVector3d::CrossProduct(Normal,Out.Tangent);return Finite(Out.Bitangent)&&!Zero(Out.Bitangent);
}

TSharedPtr<FJsonObject> Repair(UStaticMesh* Mesh)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schemaVersion"),TEXT("v826.worldrender.invalid.corner.basis.1"));
    J->SetBoolField(TEXT("success"),false);J->SetBoolField(TEXT("mutationAttempted"),false);J->SetBoolField(TEXT("repairEligible"),false);J->SetBoolField(TEXT("artAccepted"),false);
    const auto Reject=[&](const FString& Why){J->SetStringField(TEXT("reason"),Why);return J;};
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!IsInGameThread()||!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase))return Reject(TEXT("wrong isolated receiver or thread"));
    FString SHA,Token,Why;if(!PinnedPart(Mesh,SHA,Token,Why))return Reject(Why);
    J->SetStringField(TEXT("mesh"),Mesh->GetPathName());J->SetStringField(TEXT("sourceSHA256"),SHA);J->SetStringField(TEXT("sceneToken"),Token);
    if(Mesh->GetNumSourceModels()!=1||!Mesh->bAllowCPUAccess||Mesh->GetNaniteSettings().bEnabled||!NoCollision(Mesh))return Reject(TEXT("requires NoCollision CPU single-LOD non-Nanite render candidate"));
    const FMeshBuildSettings Settings=Mesh->GetSourceModel(0).BuildSettings;
    if(Settings.bRecomputeNormals||Settings.bRecomputeTangents||Settings.bRemoveDegenerates||Settings.bGenerateLightmapUVs||Settings.BuildScale3D!=FVector(1,1,1))return Reject(TEXT("authored-basis build contract differs"));
    const FMeshDescription* D=Mesh->GetMeshDescription(0);if(!D||D->Vertices().Num()==0||D->VertexInstances().Num()==0||D->Triangles().Num()==0
        ||D->Vertices().Num()>2000000||D->VertexInstances().Num()>6000000||D->Triangles().Num()>2000000)return Reject(TEXT("authored MeshDescription missing or bounded budget exceeded"));
    FStaticMeshConstAttributes A(*D);const auto P=A.GetVertexPositions();const auto N=A.GetVertexInstanceNormals(),T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();const auto UV=A.GetVertexInstanceUVs();const auto C=A.GetVertexInstanceColors();
    if(!P.IsValid()||!N.IsValid()||!T.IsValid()||!S.IsValid()||!UV.IsValid()||!C.IsValid()||!A.GetEdgeHardnesses().IsValid()||!A.GetPolygonGroupMaterialSlotNames().IsValid())return Reject(TEXT("authored attributes missing; no global basis construction permitted"));
    const auto* R=Mesh->GetRenderData();if(!R||R->LODResources.Num()!=1||UV.GetNumChannels()!=int32(R->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()))return Reject(TEXT("authored/runtime UV channel contract differs"));
    TSet<FString> RenderBadKeys;const auto Before=RenderProof(Mesh,&RenderBadKeys);J->SetObjectField(TEXT("beforeRender"),Before);if(!Before->GetBoolField(TEXT("success")))return Reject(TEXT("before CPU render proof failed"));
    const FString NonBasis=DescriptionPin(*D),Material=MaterialsPin(Mesh),Collision=CollisionPin(Mesh);J->SetStringField(TEXT("authoredNonBasisSHA1Before"),NonBasis);J->SetStringField(TEXT("materialBindingsSHA1Before"),Material);J->SetStringField(TEXT("collisionRecipeSHA1Before"),Collision);
    if(RenderBadKeys.IsEmpty())
    {
        const TMap<int32,FBadCorner> None;const FString Healthy=BasisPin(*D,None,true),Components=HealthyComponentPin(*D,None);
        J->SetObjectField(TEXT("afterRender"),Before);J->SetStringField(TEXT("authoredNonBasisSHA1After"),NonBasis);J->SetStringField(TEXT("materialBindingsSHA1After"),Material);J->SetStringField(TEXT("collisionRecipeSHA1After"),Collision);
        J->SetStringField(TEXT("healthyCornerBasisSHA1Before"),Healthy);J->SetStringField(TEXT("healthyCornerBasisSHA1After"),Healthy);J->SetStringField(TEXT("healthyComponentsSHA1Before"),Components);J->SetStringField(TEXT("healthyComponentsSHA1After"),Components);
        for(const TCHAR* K:{TEXT("authoredNonBasisExact"),TEXT("healthyCornerBasisExact"),TEXT("healthyComponentsExact"),TEXT("actualOrientedNonBasisSurfaceExact"),TEXT("actualCPUBasisValid"),TEXT("collisionRecipeExact"),TEXT("materialBindingsExact"),TEXT("buildSettingsExact")})J->SetBoolField(K,true);
        J->SetStringField(TEXT("reason"),TEXT("already healthy actual CPU basis; no mutation"));J->SetBoolField(TEXT("success"),true);J->SetNumberField(TEXT("repairedCorners"),0);return J;
    }
    const bool Colour=R->LODResources[0].VertexBuffers.ColorVertexBuffer.GetNumVertices()!=0,High=R->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis();
    TMap<int32,FBadCorner> Bad;int32 BadN=0,BadT=0,BadS=0;
    for(const auto ID:D->VertexInstances().GetElementIDs())
    {
        // Actual CPU defects are authoritative. UE can supply a healthy
        // built tangent for a missing authored tangent; preserve that source
        // corner's bytes instead of trying to repair an already healthy copy.
        if(!RenderBadKeys.Contains(RecordPin(SourceCorner(*D,ID,Colour))))continue;
        const FVector3f NV=N[ID],TV=T[ID];const bool NF=Finite(FVector3d(NV)),TF=Finite(FVector3d(TV));
        // UE ScaleStaticMeshVertex normalizes in float32 before packing even
        // at identity scale. Testing raw short vectors directly would call
        // healthy built corners defective and violate the narrow repair scope.
        FVector3f BuildN=NV,BuildT=TV;if(NF)BuildN.Normalize();if(TF)BuildT.Normalize();
        const bool PackedNZero=NF&&Zero(FVector3d(Packed(BuildN,High))),PackedTZero=TF&&Zero(FVector3d(Packed(BuildT,High)));
        FBadCorner B;B.ID=ID.GetValue();B.N=!NF||Zero(FVector3d(NV))||PackedNZero;
        B.T=!TF||Zero(FVector3d(TV))||PackedTZero||(!B.N&&TF&&Zero(FVector3d(FVector3f::CrossProduct(Packed(BuildN,High),Packed(BuildT,High)))));
        B.S=!FMath::IsFinite(S[ID])||S[ID]==0.f;
        if(!B.N&&!B.T&&!B.S)continue;
        Bad.Add(ID.GetValue(),B);BadN+=B.N;BadT+=B.T;BadS+=B.S;
    }
    J->SetNumberField(TEXT("invalidAuthoredNormals"),BadN);J->SetNumberField(TEXT("invalidAuthoredTangents"),BadT);J->SetNumberField(TEXT("invalidAuthoredSigns"),BadS);J->SetNumberField(TEXT("invalidAuthoredCorners"),Bad.Num());
    if(Bad.IsEmpty()||Bad.Num()==D->VertexInstances().Num())return Reject(TEXT("actual defect is not a bounded authored-corner repair"));
    const FString Healthy=BasisPin(*D,Bad,true),Components=HealthyComponentPin(*D,Bad),OriginalBasis=BasisPin(*D,Bad,false);
    J->SetStringField(TEXT("healthyCornerBasisSHA1Before"),Healthy);J->SetStringField(TEXT("healthyComponentsSHA1Before"),Components);J->SetBoolField(TEXT("repairEligible"),true);
    FMeshDescription Original=*D,Repaired=*D;Repaired.BuildVertexInstanceIndexers();FStaticMeshAttributes Edit(Repaired);auto EN=Edit.GetVertexInstanceNormals(),ET=Edit.GetVertexInstanceTangents();auto ES=Edit.GetVertexInstanceBinormalSigns();
    int32 NormalizedExisting=0,FaceNormals=0,DerivativeTangents=0,RankOneTangents=0,GeometricTangents=0,SignRepairs=0,RankOneFallbacks=0,GeometricFallbacks=0;TArray<TSharedPtr<FJsonValue>> Samples;
    for(const auto ID:Repaired.VertexInstances().GetElementIDs())if(const FBadCorner* B=Bad.Find(ID.GetValue()))
    {
        FVector3d Normal;FLocalBasis Local;
        if(B->N)
        {
            if(Unit(FVector3d(N[ID]),Normal))++NormalizedExisting;
            else{if(!LocalBasis(Repaired,ID,FVector3d::ZeroVector,false,Local)){J->SetObjectField(TEXT("firstLocalBasisFailure"),CornerMismatch(*D,ID,Mesh,Colour));return Reject(TEXT("invalid normal has no legal positive-area local triangle"));}Normal=Local.FaceNormal;++FaceNormals;}
            EN[ID]=FVector3f(Normal);
        }
        else if(!Unit(FVector3d(N[ID]),Normal))return Reject(TEXT("healthy normal could not be normalized for local calculation"));
        if(B->T||B->S)
        {
            if(!LocalBasis(Repaired,ID,Normal,true,Local)){J->SetObjectField(TEXT("firstLocalBasisFailure"),CornerMismatch(*D,ID,Mesh,Colour));return Reject(TEXT("invalid tangent/sign has no positive-area local basis"));}
            if(B->T){ET[ID]=FVector3f(Local.Tangent);if(Local.RankOneUVFallback)++RankOneTangents;else if(Local.GeometricFallback)++GeometricTangents;else ++DerivativeTangents;}
            if(B->S)
            {const FVector3d Tangent(ET[ID]);const FVector3d Bitangent=(Local.RankOneUVFallback||Local.GeometricFallback)?FVector3d::CrossProduct(Normal,Tangent):Local.Bitangent;
                const double Hand=FVector3d::DotProduct(FVector3d::CrossProduct(Normal,Tangent),Bitangent);if(!FMath::IsFinite(Hand)||Hand==0.)return Reject(TEXT("local UV handedness indeterminate"));ES[ID]=Hand<0.?-1.f:1.f;++SignRepairs;}
            RankOneFallbacks+=Local.RankOneUVFallback;GeometricFallbacks+=Local.GeometricFallback;
        }
        if(Samples.Num()<24)
        {auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("vertexInstance"),ID.GetValue());Row->SetBoolField(TEXT("normalRepaired"),B->N);Row->SetBoolField(TEXT("tangentRepaired"),B->T);Row->SetBoolField(TEXT("signRepaired"),B->S);Row->SetNumberField(TEXT("normalTriangle"),Local.NormalTriangle);Row->SetNumberField(TEXT("tangentTriangle"),Local.TangentTriangle);Row->SetNumberField(TEXT("uvDeterminant"),Local.UVDeterminant);Row->SetBoolField(TEXT("rankOneUVFallback"),Local.RankOneUVFallback);Row->SetBoolField(TEXT("geometricFallback"),Local.GeometricFallback);Row->SetNumberField(TEXT("doubleAreaCm2"),Local.DoubleArea);Samples.Add(MakeShared<FJsonValueObject>(Row));}
    }
    J->SetNumberField(TEXT("normalizedExistingNormals"),NormalizedExisting);J->SetNumberField(TEXT("positiveAreaFaceNormals"),FaceNormals);J->SetNumberField(TEXT("uvDerivativeTangents"),DerivativeTangents);J->SetNumberField(TEXT("rankOneUVTangents"),RankOneTangents);J->SetNumberField(TEXT("geometricTangents"),GeometricTangents);J->SetNumberField(TEXT("repairedSigns"),SignRepairs);J->SetNumberField(TEXT("rankOneUVFallback"),RankOneFallbacks);J->SetNumberField(TEXT("geometricFallback"),GeometricFallbacks);J->SetArrayField(TEXT("repairSamples"),Samples);
    if(DescriptionPin(Repaired)!=NonBasis||BasisPin(Repaired,Bad,true)!=Healthy||HealthyComponentPin(Repaired,Bad)!=Components)return Reject(TEXT("local repair changed nonbasis or healthy basis bytes"));
    const bool WasDirty=Mesh->GetOutermost()->IsDirty();J->SetBoolField(TEXT("mutationAttempted"),true);
    Mesh->CreateMeshDescription(0,MoveTemp(Repaired));Mesh->CommitMeshDescription(0);TArray<FText> Errors;Mesh->Build(true,&Errors);
    TArray<TSharedPtr<FJsonValue>> Messages;for(const auto& Error:Errors)Messages.Add(MakeShared<FJsonValueString>(Error.ToString()));J->SetArrayField(TEXT("buildErrors"),Messages);
    const auto After=RenderProof(Mesh);J->SetObjectField(TEXT("afterRender"),After);const auto* Actual=Mesh->GetMeshDescription(0);
    const bool AuthoredExact=Actual&&DescriptionPin(*Actual)==NonBasis,HealthyExact=Actual&&BasisPin(*Actual,Bad,true)==Healthy,ComponentsExact=Actual&&HealthyComponentPin(*Actual,Bad)==Components;
    const bool SurfaceExact=After->GetBoolField(TEXT("success"))&&Before->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))==After->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"));
    const bool BasisValid=After->GetBoolField(TEXT("success"))&&After->GetNumberField(TEXT("zeroNormalVertices"))==0.&&After->GetNumberField(TEXT("zeroTangentXVertices"))==0.&&After->GetNumberField(TEXT("zeroTangentYVertices"))==0.&&After->GetNumberField(TEXT("nonFiniteBasisVertices"))==0.;
    const bool CollisionExact=CollisionPin(Mesh)==Collision&&NoCollision(Mesh),MaterialsExact=MaterialsPin(Mesh)==Material,SettingsExact=Mesh->GetSourceModel(0).BuildSettings==Settings;
    J->SetBoolField(TEXT("authoredNonBasisExact"),AuthoredExact);J->SetBoolField(TEXT("healthyCornerBasisExact"),HealthyExact);J->SetBoolField(TEXT("healthyComponentsExact"),ComponentsExact);J->SetBoolField(TEXT("actualOrientedNonBasisSurfaceExact"),SurfaceExact);J->SetBoolField(TEXT("actualCPUBasisValid"),BasisValid);J->SetBoolField(TEXT("collisionRecipeExact"),CollisionExact);J->SetBoolField(TEXT("materialBindingsExact"),MaterialsExact);J->SetBoolField(TEXT("buildSettingsExact"),SettingsExact);
    if(Actual){J->SetStringField(TEXT("authoredNonBasisSHA1After"),DescriptionPin(*Actual));J->SetStringField(TEXT("healthyCornerBasisSHA1After"),BasisPin(*Actual,Bad,true));J->SetStringField(TEXT("healthyComponentsSHA1After"),HealthyComponentPin(*Actual,Bad));}
    J->SetStringField(TEXT("materialBindingsSHA1After"),MaterialsPin(Mesh));J->SetStringField(TEXT("collisionRecipeSHA1After"),CollisionPin(Mesh));
    if(!Errors.IsEmpty()||!AuthoredExact||!HealthyExact||!ComponentsExact||!SurfaceExact||!BasisValid||!CollisionExact||!MaterialsExact||!SettingsExact)
    {
        J->SetBoolField(TEXT("rollbackAttempted"),true);Mesh->CreateMeshDescription(0,MoveTemp(Original));Mesh->CommitMeshDescription(0);TArray<FText> RollbackErrors;Mesh->Build(true,&RollbackErrors);
        TArray<TSharedPtr<FJsonValue>> RM;for(const auto& E:RollbackErrors)RM.Add(MakeShared<FJsonValueString>(E.ToString()));J->SetArrayField(TEXT("rollbackBuildErrors"),RM);
        const auto* Restored=Mesh->GetMeshDescription(0);const auto RestoredRender=RenderProof(Mesh);J->SetObjectField(TEXT("rollbackRender"),RestoredRender);
        const bool RestoredExact=RollbackErrors.IsEmpty()&&Restored&&DescriptionPin(*Restored)==NonBasis&&BasisPin(*Restored,Bad,false)==OriginalBasis
            &&RestoredRender->GetBoolField(TEXT("success"))&&RestoredRender->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))==Before->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))&&MaterialsPin(Mesh)==Material&&CollisionPin(Mesh)==Collision;
        J->SetBoolField(TEXT("rollbackSucceeded"),RestoredExact);if(RestoredExact)Mesh->GetOutermost()->SetDirtyFlag(WasDirty);
        return Reject(TEXT("build or strict preservation/actual CPU basis verification failed; candidate not approved"));
    }
    J->SetNumberField(TEXT("repairedCorners"),Bad.Num());J->SetBoolField(TEXT("success"),true);J->SetStringField(TEXT("reason"),TEXT("invalid authored corner basis repaired; exact surface and healthy basis retained; not visual acceptance"));return J;
}
}
#endif
