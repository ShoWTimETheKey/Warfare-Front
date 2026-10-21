// Accepted Vaux composition. No native/collision changes, new
// tree stems, grass cards, colour tweaks or source-asset writes.
#include "WarfareFRRUVauxCompositionV87.h"
namespace WarfareFRRUVauxCompositionV87
{
bool Enabled(const FString& Scene)
{
    if(Scene!=TEXT("fr.vaux.scene.families.v1"))return false;
#if WITH_EDITOR
    if(WarfareFRRUWorldRenderBindingV87::CandidateEnabled())
        return FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxComposition"));
#endif
    return true;
}
FVector CameraOffset(const AWarfareFrontlineGameMode* B)
{return B&&WarfareFRRUCampaignV82::Active(B)&&Enabled(WarfareFRRUWorldV82::SceneId(B))?FVector(0,0,-375.):FVector::ZeroVector;}
int32 ComponentCount(const FString& Scene){return Enabled(Scene)?3:0;}
namespace
{
const TCHAR* SourceMeshPaths[]={
    TEXT("/Game/Warfare/Field/V42/Meshes/SM_RootedApron_A_V42.SM_RootedApron_A_V42"),
    TEXT("/Game/Warfare/Field/V42/Meshes/SM_RootedApron_B_V42.SM_RootedApron_B_V42"),
    TEXT("/Game/Warfare/Field/V42/Meshes/SM_CollapseScatter_V42.SM_CollapseScatter_V42")};
const TCHAR* ReleaseMeshPaths[]={
    TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxComposition/ReleaseV1/SM_RootA.SM_RootA"),
    TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxComposition/ReleaseV1/SM_RootB.SM_RootB"),
    TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxComposition/ReleaseV1/SM_Collapse.SM_Collapse")};
const TCHAR* MeshPath(const FString& Scene,int32 Index)
{return WarfareFRRUWorldRenderBindingV87::ProductionEnabled(Scene)?ReleaseMeshPaths[Index]:SourceMeshPaths[Index];}
const FBox ExpectedBounds[]={
    FBox(FVector(-115.971985,-148.412430,-2),FVector(114.503075,130.646164,23.856436)),
    FBox(FVector(-105.086159,-143.673447,-2),FVector(119.459862,142.923355,25.944965)),
    FBox(FVector(-103.062737,-117.134834,-7.182710),FVector(105.046936,118.017708,7.768645))};
struct FStamp {TArray<FTransform> Placements;};
TMap<TWeakObjectPtr<const UStaticMeshComponent>,FStamp> Stamps;
bool MaterialsMatch(const UStaticMeshComponent* C,const UStaticMesh* Mesh)
{
    if(!C||!Mesh||C->GetNumMaterials()!=Mesh->GetStaticMaterials().Num())return false;
    for(int32 I=0;I<C->GetNumMaterials();++I)
    {
        const auto* Material=C->GetMaterial(I);const FName Slot=Mesh->GetStaticMaterials()[I].MaterialSlotName;
        const TCHAR* Path=Slot==TEXT("V42_Mud")?TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42.MI_EarthBank_V42"):
            Slot==TEXT("V42_Timber")?TEXT("/Game/Warfare/Field/V42/Materials/M_Timber_V42.M_Timber_V42"):
            Slot==TEXT("V42_Rust")?TEXT("/Game/Warfare/Field/V42/Materials/M_Rust_V42.M_Rust_V42"):nullptr;
        if(!Path||!Material||Material!=Mesh->GetMaterial(I)||Material->GetPathName()!=Path)return false;
    }
    return true;
}
bool Clear(const wfexp::scene::SceneDefinition& N,const FBox& Bounds,int32 OwnTree)
{
    const FBox2D B(FVector2D(Bounds.Min.X,Bounds.Min.Y),FVector2D(Bounds.Max.X,Bounds.Max.Y));
    if(B.Min.X<N.minX+100||B.Max.X>N.maxX-100||B.Min.Y<N.minY+100||B.Max.Y>N.maxY-100)return false;
    const FBox2D Route(B.Min-FVector2D(500),B.Max+FVector2D(500));
    for(const auto& R:N.routes)if(R.traversable)for(size_t J=1;J<R.points.size();++J)
        if(WarfareFRRUWorldDressingV827::SegmentTouches({R.points[J-1].x,R.points[J-1].y},{R.points[J].x,R.points[J].y},Route))return false;
    if(const auto* M=wfexp::tactical::FindMission("fr.vaux"))for(const auto& Node:M->nodes)
    {const FVector2D P(Node.position.x,Node.position.y),Closest(FMath::Clamp(P.X,B.Min.X,B.Max.X),FMath::Clamp(P.Y,B.Min.Y,B.Max.Y));
        if(FVector2D::DistSquared(P,Closest)<FMath::Square(Node.radius+350.))return false;}
    for(size_t I=0;I<N.solids.size();++I)
    {
        if(int32(I)==OwnTree&&std::string_view(N.solids[I].kind)=="tree_trunk")continue;
        const auto& S=N.solids[I];if(B.Intersect(FBox2D({S.minimum.x-100,S.minimum.y-100},{S.maximum.x+100,S.maximum.y+100})))return false;
    }
    for(const auto& D:N.decks)if(WarfareFRRUWorldDressingV827::SegmentTouches({D.from.x,D.from.y},{D.to.x,D.to.y},
        FBox2D(B.Min-FVector2D(D.halfWidth+500),B.Max+FVector2D(D.halfWidth+500))))return false;
    for(const auto& P:N.waterPatches)if(!P.polygon.empty()&&WarfareFRRUWorldDressingV827::WaterTouches(P,B))return false;
    return true;
}
bool Seat(const wfexp::scene::SceneDefinition& N,UStaticMesh* Mesh,const TArray<FVector>& Feet,
    int32 Tree,const FVector2D& XY,double Yaw,FTransform& Out,double& WorstEmbed)
{
    double H,X0,X1,Y0,Y1;
    if(!wfexp::scene::HeightAt(N,XY.X,XY.Y,H)||!wfexp::scene::HeightAt(N,XY.X-45,XY.Y,X0)
        ||!wfexp::scene::HeightAt(N,XY.X+45,XY.Y,X1)||!wfexp::scene::HeightAt(N,XY.X,XY.Y-45,Y0)
        ||!wfexp::scene::HeightAt(N,XY.X,XY.Y+45,Y1))return false;
    const FVector Normal(-(X1-X0)/90.,-(Y1-Y0)/90.,1.);if(Normal.SizeSquared()>1.16)return false;
    const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal.GetSafeNormal())*FQuat(FVector::UpVector,FMath::DegreesToRadians(Yaw));
    double Low=DBL_MAX,High=-DBL_MAX;
    for(const FVector& Local:Feet)
    {const FVector P=Q.RotateVector(Local);double Floor;if(!wfexp::scene::HeightAt(N,XY.X+P.X,XY.Y+P.Y,Floor))return false;
        const double Target=Floor-P.Z;Low=FMath::Min(Low,Target);High=FMath::Max(High,Target);}
    // Every actual lower-surface bearing remains inside native soil. Reject a
    // rough footprint instead of floating/flattening it or moving the tree.
    const double Z=Low-1.5;WorstEmbed=High-Z;if(WorstEmbed>18.)return false;
    Out=FTransform(Q,FVector(XY.X,XY.Y,Z));return Clear(N,Mesh->GetBoundingBox().TransformBy(Out),Tree);
}
}
bool ValidateComponent(const FString& Scene,const UStaticMeshComponent* Component)
{
    const auto* C=Cast<UHierarchicalInstancedStaticMeshComponent>(Component);const auto* Stamp=Stamps.Find(Component);
    if(!Enabled(Scene)||!C||!Stamp||!C->IsRegistered()||!C->ComponentHasTag(TEXT("FRRU_V87VauxRoots"))
        ||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||!C->GetComponentTransform().Equals(FTransform::Identity,.0001)
        ||C->GetInstanceCount()!=Stamp->Placements.Num()||!MaterialsMatch(C,C->GetStaticMesh()))return false;
    bool Known=false;for(int32 I=0;I<UE_ARRAY_COUNT(ReleaseMeshPaths);++I)Known|=C->GetStaticMesh()->GetPathName()==MeshPath(Scene,I);if(!Known)return false;
    if(const auto* Body=C->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
    for(int32 I=0;I<Stamp->Placements.Num();++I)
    {FTransform T;if(!C->GetInstanceTransform(I,T,true)||!T.Equals(Stamp->Placements[I],.0001))return false;}
    return true;
}
bool Stage(AActor* Owner,USceneComponent* Root,const FString& Scene,FString& Error,bool(*Ready)(UMaterialInterface*,FString&))
{
    if(!Enabled(Scene))return true;const auto* N=wfexp::scene::ForMission("fr.vaux");
    if(!Owner||!Root||!N||Scene!=UTF8_TO_TCHAR(N->id)){Error=TEXT("Vaux composition needs its actual native scene");return false;}
    for(auto I=Stamps.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
    UStaticMesh* Meshes[3]={};UHierarchicalInstancedStaticMeshComponent* Components[3]={};TArray<FVector> Feet[3];
    for(int32 I=0;I<3;++I)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,MeshPath(Scene,I));Meshes[I]=Mesh;
        if(!Mesh||Mesh->GetPathName()!=MeshPath(Scene,I)||!Mesh->GetRenderData()||!Mesh->GetBoundingBox().Equals(ExpectedBounds[I],.05)
            ||(WarfareFRRUWorldRenderBindingV87::ProductionEnabled(Scene)&&!Mesh->bAllowCPUAccess))
        {Error=TEXT("Accepted Vaux root/debris mesh bounds changed");return false;}
        const auto& Vertices=Mesh->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
        if(!Vertices.GetVertexData()||Vertices.GetNumVertices()==0){Error=TEXT("Actual Vaux root bearing vertices unavailable");return false;}
        FBox FootBounds(ForceInit);const FBox Local=Mesh->GetBoundingBox();
        for(uint32 V=0;V<Vertices.GetNumVertices();++V)
        {const FVector P(Vertices.VertexPosition(V));if(P.Z<=Local.Min.Z+5.){Feet[I].Add(P);FootBounds+=P;}}
        const FVector Span=FootBounds.GetSize(),Full=Local.GetSize();
        if(Feet[I].Num()<3||Span.X<Full.X*.9||Span.Y<Full.Y*.9)
        {Error=TEXT("Actual Vaux root lower surface does not cover its complete base");return false;}
        auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("VauxRootsV87_%d"),I)),RF_Transient);
        Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);
        C->SetAbsolute(true,true,true);C->SetWorldTransform(FTransform::Identity);C->SetStaticMesh(Mesh);
        C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(true);C->SetCastShadow(true);C->SetCullDistances(0,30000);
        C->ComponentTags.Add(TEXT("FRRU_V87VauxRoots"));C->RegisterComponent();Components[I]=C;
        if(!MaterialsMatch(C,Mesh)){Error=TEXT("Accepted Vaux root/debris material pairing differs");return false;}
        for(int32 S=0;S<C->GetNumMaterials();++S)if(!Ready||!Ready(C->GetMaterial(S),Error))return false;
        Stamps.Add(C,FStamp());
    }
    int32 Trees=0,Roots=0,Debris=0,Rejected=0;double MaximumEmbed=0;
    const auto Add=[&](int32 Model,int32 Tree,const FVector2D& XY,double Yaw)
    {
        FTransform T;double Embed=0;
        if(!Seat(*N,Meshes[Model],Feet[Model],Tree,XY,Yaw,T,Embed)){++Rejected;return;}
        Components[Model]->AddInstance(T);Stamps.FindChecked(Components[Model]).Placements.Add(T);
        MaximumEmbed=FMath::Max(MaximumEmbed,Embed);if(Model<2)++Roots;else ++Debris;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_VAUX_ROOT scene=%s ownerSolid=fr.vaux.solid.%d model=%d actualBearingVertices=%d maxEmbedCm=%.4f maxGapCm=-1.5 x=%.3f y=%.3f"),
            *Scene,Tree,Model,Feet[Model].Num(),Embed,XY.X,XY.Y);
    };
    for(size_t I=0;I<N->solids.size();++I)if(std::string_view(N->solids[I].kind)=="tree_trunk")
    {
        const auto& Tree=N->solids[I];const FVector2D Centre((Tree.minimum.x+Tree.maximum.x)*.5,(Tree.minimum.y+Tree.maximum.y)*.5);
        const double Yaw=FMath::Fmod(double(I)*67.,360.);Add(int32(I)%2,int32(I),Centre,Yaw);
        if(I%2==0)
        {const double Angle=FMath::DegreesToRadians(Yaw);Add(2,int32(I),Centre+FVector2D(FMath::Cos(Angle)*120.,FMath::Sin(Angle)*120.),Yaw+31.);}
        ++Trees;
    }
    if(Trees!=16){Error=TEXT("Vaux composition requires the original 16 physical tree identities");return false;}
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_VAUX_COMPOSITION nativeTrees=%d rooted=%d debris=%d rejected=%d actualCameraOffsetZ=-375 maxEmbedCm=%.4f geometryCollisionUnchanged=1"),Trees,Roots,Debris,Rejected,MaximumEmbed);
    for(const auto* C:Components)if(!ValidateComponent(Scene,C)){Error=TEXT("Vaux root/debris staging differs");return false;}
    return true;
}
}
