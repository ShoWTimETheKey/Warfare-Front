#include "WarfareFrontlineGroundContactV8449.h"
#include "WarfareFrontlineGround.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformTime.h"
#include "PhysicsEngine/BodySetup.h"

namespace FrontlineGroundContactV8449
{
namespace
{
constexpr float CellSize=128.f;
struct FContactCache
{
    TWeakObjectPtr<AWarfareFrontlineGround> Ground;
    TWeakObjectPtr<UStaticMeshComponent> Component;
    TWeakObjectPtr<UStaticMesh> Mesh;
    FTransform Transform;
    FVector2f Origin=FVector2f::ZeroVector;
    int32 Columns=0,Rows=0,Triangles=0;
    const FStaticMeshLODResources* SourceLOD=nullptr;
    const void* SourcePositions=nullptr;
    const void* SourceIndices=nullptr;
    FIndexArrayView Indices;
    uint32 VertexCount=0;
    int32 IndexDataSize=0;
    bool bIdentityTransform=false;
    // Compressed rows: each bin references only real indexed triangles whose
    // XY bounds overlap it. No scan, allocation or cache build in Sample.
    TArray<int32> Offsets;
    TArray<uint32> References;
    double BuildMilliseconds=0;
    uint64 Samples=0,Misses=0,PatchQueries=0,PatchTriangles=0;
    bool Matches(AWarfareFrontlineGround* G) const
    {
        if(!G||Ground.Get()!=G||!G->Surface||Component.Get()!=G->Surface.Get()||!Mesh.IsValid()
            ||Mesh.Get()!=G->Surface->GetStaticMesh()||!Transform.Equals(G->Surface->GetComponentTransform(),.0001))return false;
        const FStaticMeshRenderData* Render=Mesh->GetRenderData();
        if(!Render||Render->LODResources.IsEmpty()||&Render->LODResources[0]!=SourceLOD)return false;
        const auto& VB=SourceLOD->VertexBuffers.PositionVertexBuffer;const auto& IB=SourceLOD->IndexBuffer;
        return VB.GetAllowCPUAccess()&&VB.GetVertexData()==SourcePositions&&VB.GetNumVertices()==VertexCount
            &&IB.GetAllowCPUAccess()&&IB.GetIndexDataSize()==IndexDataSize
            &&IB.GetNumIndices()==Indices.Num()
            &&(IB.Is32Bit()?static_cast<const void*>(IB.AccessStream32()):static_cast<const void*>(IB.AccessStream16()))==SourceIndices;
    }
    FVector3f Position(uint32 I) const
    {
        const FVector3f P=SourceLOD->VertexBuffers.PositionVertexBuffer.VertexPosition(I);
        return bIdentityTransform?P:FVector3f(Transform.TransformPosition(FVector(P)));
    }
};
TArray<TUniquePtr<FContactCache>> Caches;
FContactCache* Find(AWarfareFrontlineGround* G)
{
    for(auto& C:Caches)if(C->Matches(G))return C.Get();
    return nullptr;
}
float DecodeSRGB(float V)
{
    // The terrain shader interpolates the packed gamma RGB before decoding.
    // Decode once after barycentric interpolation; alpha is physical linear.
    return V<=.04045f?V/12.92f:FMath::Pow((V+.055f)/1.055f,2.4f);
}
}
bool Prepare(AWarfareFrontlineGround* G)
{
    if(!IsInGameThread()||!G||!G->Surface)return false;
    if(Find(G))return true;
    Caches.RemoveAll([&](const auto& C){return !C->Ground.IsValid()||!C->Mesh.IsValid()||C->Ground==G;});
    UStaticMesh* Mesh=G->Surface->GetStaticMesh();const FStaticMeshRenderData* Render=Mesh?Mesh->GetRenderData():nullptr;
    if(!Render||Render->LODResources.IsEmpty())return false;
    const auto& LOD=Render->LODResources[0];const auto& VB=LOD.VertexBuffers.PositionVertexBuffer;
    const auto& IB=LOD.IndexBuffer;
    // Inspect existing CPU data only. Never enable retention or fetch a GPU
    // fallback, both of which would disguise a missing production receiver.
    if(!VB.GetAllowCPUAccess()||!VB.GetVertexData()||!IB.GetAllowCPUAccess()||IB.GetIndexDataSize()==0
        ||VB.GetNumVertices()<3||VB.GetNumVertices()>12000000||IB.GetNumIndices()<3
        ||IB.GetNumIndices()%3!=0||IB.GetNumIndices()>24000000)return false;
    const auto View=IB.GetArrayView();if(View.Num()!=IB.GetNumIndices())return false;
    const double Start=FPlatformTime::Seconds();auto C=MakeUnique<FContactCache>();
    C->Ground=G;C->Component=G->Surface;C->Mesh=Mesh;C->Transform=G->Surface->GetComponentTransform();
    if(C->Transform.ContainsNaN())return false;
    C->SourceLOD=&LOD;C->SourcePositions=VB.GetVertexData();C->Indices=View;C->VertexCount=VB.GetNumVertices();
    C->IndexDataSize=IB.GetIndexDataSize();
    C->SourceIndices=IB.Is32Bit()?static_cast<const void*>(IB.AccessStream32()):static_cast<const void*>(IB.AccessStream16());
    C->bIdentityTransform=C->Transform.Equals(FTransform::Identity,0.);
    if(!C->SourceIndices)return false;
    FVector2f Low(FLT_MAX,FLT_MAX),High(-FLT_MAX,-FLT_MAX);
    for(uint32 I=0;I<VB.GetNumVertices();++I)
    {
        const FVector3f P=C->Position(I);
        if(P.ContainsNaN())return false;
        Low.X=FMath::Min(Low.X,float(P.X));Low.Y=FMath::Min(Low.Y,float(P.Y));
        High.X=FMath::Max(High.X,float(P.X));High.Y=FMath::Max(High.Y,float(P.Y));
    }
    C->Origin=Low;C->Columns=FMath::FloorToInt((High.X-Low.X)/CellSize)+1;
    C->Rows=FMath::FloorToInt((High.Y-Low.Y)/CellSize)+1;
    const int64 Bins=int64(C->Columns)*C->Rows;if(Bins<=0||Bins>500000)return false;
    for(int32 I=0;I<View.Num();++I)if(View[I]>=C->VertexCount)return false;
    C->Triangles=View.Num()/3;TArray<int32> Counts;Counts.Init(0,int32(Bins));
    auto Bounds=[&](int32 T,int32& X0,int32& X1,int32& Y0,int32& Y1)
    {
        const FVector3f A=C->Position(C->Indices[T*3]),B=C->Position(C->Indices[T*3+1]),D=C->Position(C->Indices[T*3+2]);
        const double Area=double(B.X-A.X)*(D.Y-A.Y)-double(B.Y-A.Y)*(D.X-A.X);
        if(FMath::Abs(Area)<.00001)return false;
        X0=FMath::Clamp(FMath::FloorToInt((FMath::Min3(A.X,B.X,D.X)-Low.X)/CellSize),0,C->Columns-1);
        X1=FMath::Clamp(FMath::FloorToInt((FMath::Max3(A.X,B.X,D.X)-Low.X)/CellSize),0,C->Columns-1);
        Y0=FMath::Clamp(FMath::FloorToInt((FMath::Min3(A.Y,B.Y,D.Y)-Low.Y)/CellSize),0,C->Rows-1);
        Y1=FMath::Clamp(FMath::FloorToInt((FMath::Max3(A.Y,B.Y,D.Y)-Low.Y)/CellSize),0,C->Rows-1);
        return true;
    };
    int64 References=0;
    for(int32 T=0;T<C->Triangles;++T)
    {
        int32 X0,X1,Y0,Y1;if(!Bounds(T,X0,X1,Y0,Y1))continue;
        References+=int64(X1-X0+1)*(Y1-Y0+1);if(References>24000000)return false;
        for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)++Counts[Y*C->Columns+X];
    }
    C->Offsets.SetNumUninitialized(int32(Bins)+1);C->Offsets[0]=0;
    for(int32 I=0;I<Bins;++I)C->Offsets[I+1]=C->Offsets[I]+Counts[I];
    C->References.SetNumUninitialized(int32(References));Counts.Init(0,int32(Bins));
    for(int32 T=0;T<C->Triangles;++T)
    {
        int32 X0,X1,Y0,Y1;if(!Bounds(T,X0,X1,Y0,Y1))continue;
        for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)
        {const int32 Bin=Y*C->Columns+X;C->References[C->Offsets[Bin]+Counts[Bin]++]=T;}
    }
    C->BuildMilliseconds=(FPlatformTime::Seconds()-Start)*1000.;
    UE_LOG(LogTemp,Display,TEXT("WF_V8449_GROUND_CONTACT_READY mesh=%s triangles=%d bins=%lld references=%d build_ms=%.3f CPU_receiver=1 collision_changed=0"),
        *GetPathNameSafe(Mesh),C->Triangles,Bins,C->References.Num(),C->BuildMilliseconds);
    Caches.Add(MoveTemp(C));return true;
}
bool Sample(AWarfareFrontlineGround* G,float X,float Y,FVector& Point,FVector& Normal,FLinearColor* Color)
{
    if(!IsInGameThread()||!FMath::IsFinite(X)||!FMath::IsFinite(Y))return false;
    auto* C=Find(G);if(!C)return false;++C->Samples;
    const int32 BX=FMath::FloorToInt((X-C->Origin.X)/CellSize),BY=FMath::FloorToInt((Y-C->Origin.Y)/CellSize);
    if(BX<0||BY<0||BX>=C->Columns||BY>=C->Rows){++C->Misses;return false;}
    const int32 Bin=BY*C->Columns+BX;bool Found=false;double BestZ=-DBL_MAX;
    uint32 Best[3]={0,0,0};double Weights[3]={0,0,0};
    for(int32 R=C->Offsets[Bin];R<C->Offsets[Bin+1];++R)
    {
        const uint32 T=C->References[R],I=C->Indices[T*3],J=C->Indices[T*3+1],K=C->Indices[T*3+2];
        const FVector3f A=C->Position(I),B=C->Position(J),D=C->Position(K);
        const double Det=double(B.X-A.X)*(D.Y-A.Y)-double(B.Y-A.Y)*(D.X-A.X);
        const double U=(double(X-A.X)*(D.Y-A.Y)-double(Y-A.Y)*(D.X-A.X))/Det;
        const double V=(double(B.X-A.X)*(Y-A.Y)-double(B.Y-A.Y)*(X-A.X))/Det,W=1.-U-V;
        if(U<-.00001||V<-.00001||W<-.00001)continue;
        const double Z=W*A.Z+U*B.Z+V*D.Z;if(Z<=BestZ)continue;
        FVector N=FVector::CrossProduct(FVector(B-A),FVector(D-A)).GetSafeNormal();
        if(N.ContainsNaN()||FMath::Abs(N.Z)<.00001)continue;if(N.Z<0)N=-N;
        Normal=N;BestZ=Z;Best[0]=I;Best[1]=J;Best[2]=K;Weights[0]=W;Weights[1]=U;Weights[2]=V;Found=true;
    }
    if(!Found){++C->Misses;return false;}Point=FVector(X,Y,BestZ);
    if(Color)
    {
        const auto* Render=C->Mesh->GetRenderData();const auto& CB=Render->LODResources[0].VertexBuffers.ColorVertexBuffer;
        if(!CB.GetAllowCPUAccess()||CB.GetNumVertices()!=C->VertexCount||!CB.GetVertexData()){++C->Misses;return false;}
        FVector4f Encoded(0,0,0,0);
        for(int32 I=0;I<3;++I){const FColor V=CB.VertexColor(Best[I]);Encoded+=FVector4f(V.R/255.f,V.G/255.f,V.B/255.f,V.A/255.f)*float(Weights[I]);}
        *Color=FLinearColor(DecodeSRGB(Encoded.X),DecodeSRGB(Encoded.Y),DecodeSRGB(Encoded.Z),Encoded.W);
    }
    return true;
}
bool TrianglesInBox(AWarfareFrontlineGround* G,const FVector2D& Minimum,const FVector2D& Maximum,
    TArray<FFrontlineGroundTriangleV8449>& Out)
{
    Out.Reset();if(!IsInGameThread()||Minimum.ContainsNaN()||Maximum.ContainsNaN()
        ||Maximum.X<=Minimum.X||Maximum.Y<=Minimum.Y
        ||Maximum.X-Minimum.X>1024.||Maximum.Y-Minimum.Y>1024.)return false;
    auto* C=Find(G);if(!C)return false;++C->PatchQueries;
    const int32 X0=FMath::Max(0,FMath::FloorToInt((Minimum.X-C->Origin.X)/CellSize));
    const int32 X1=FMath::Min(C->Columns-1,FMath::FloorToInt((Maximum.X-C->Origin.X)/CellSize));
    const int32 Y0=FMath::Max(0,FMath::FloorToInt((Minimum.Y-C->Origin.Y)/CellSize));
    const int32 Y1=FMath::Min(C->Rows-1,FMath::FloorToInt((Maximum.Y-C->Origin.Y)/CellSize));
    if(X1<X0||Y1<Y0)return false;
    constexpr int32 Limit=8192;TSet<uint32> Seen;Seen.Reserve(Limit);Out.Reserve(1024);
    for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)
    {
        const int32 Bin=Y*C->Columns+X;
        for(int32 R=C->Offsets[Bin];R<C->Offsets[Bin+1];++R)
        {
            const uint32 T=C->References[R];if(Seen.Contains(T))continue;Seen.Add(T);
            const FVector A(C->Position(C->Indices[T*3])),B(C->Position(C->Indices[T*3+1])),D(C->Position(C->Indices[T*3+2]));
            if(FMath::Max3(A.X,B.X,D.X)<Minimum.X||FMath::Min3(A.X,B.X,D.X)>Maximum.X
                ||FMath::Max3(A.Y,B.Y,D.Y)<Minimum.Y||FMath::Min3(A.Y,B.Y,D.Y)>Maximum.Y)continue;
            if(Out.Num()>=Limit){Out.Reset();return false;}
            auto& Triangle=Out.AddDefaulted_GetRef();Triangle.SourceTriangle=int32(T);
            Triangle.Points[0]=A;Triangle.Points[1]=B;Triangle.Points[2]=D;
        }
    }
    C->PatchTriangles+=Out.Num();return !Out.IsEmpty();
}
TSharedRef<FJsonObject> Describe(AWarfareFrontlineGround* G)
{
    auto J=MakeShared<FJsonObject>();auto* C=Find(G);J->SetBoolField(TEXT("actual_indexed_receiver_ready"),C!=nullptr);
    J->SetBoolField(TEXT("analytic_or_physics_fallback"),false);
    if(G&&G->Surface&&G->Surface->GetStaticMesh())if(const auto* Body=G->Surface->GetStaticMesh()->GetBodySetup())
    {
        J->SetNumberField(TEXT("legacy_chaos_triangle_shapes"),Body->TriMeshGeometries.Num());
        J->SetBoolField(TEXT("legacy_physics_mesh_creation_finished"),Body->bCreatedPhysicsMeshes);
    }
    if(C)
    {
        J->SetStringField(TEXT("actual_mesh"),GetPathNameSafe(C->Mesh.Get()));J->SetNumberField(TEXT("actual_triangles"),C->Triangles);
        J->SetNumberField(TEXT("spatial_bins"),C->Columns*C->Rows);J->SetNumberField(TEXT("triangle_references"),C->References.Num());
        J->SetBoolField(TEXT("source_data_identity_verified"),true);
        J->SetNumberField(TEXT("source_vertices"),C->VertexCount);
        J->SetNumberField(TEXT("cache_bytes"),double(C->Offsets.GetAllocatedSize()+C->References.GetAllocatedSize()));
        J->SetNumberField(TEXT("copied_source_vertex_or_index_bytes"),0);
        J->SetNumberField(TEXT("preparation_build_ms"),C->BuildMilliseconds);J->SetNumberField(TEXT("queries"),double(C->Samples));
        J->SetNumberField(TEXT("missing_queries"),double(C->Misses));
        J->SetNumberField(TEXT("local_patch_queries"),double(C->PatchQueries));
        J->SetNumberField(TEXT("local_patch_triangles"),double(C->PatchTriangles));
    }
    return J;
}
}
