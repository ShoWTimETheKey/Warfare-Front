// Bounded vehicle ground support shared by Editor and cooked runtime. Release
// acceptance remains the host's gate; diagnostics never grant movement.
namespace FTGroundPilotV82
{
using namespace FRRUVehicleGroundDraftV82;
#if WITH_EDITOR
#define FT_CONTACT_DIAG(...) do { __VA_ARGS__; } while(false)
#else
#define FT_CONTACT_DIAG(...) do { } while(false)
#endif
TSharedPtr<FJsonObject> ContactDiagnostic()
{
#if WITH_EDITOR
    return MakeShared<FJsonObject>();
#else
    return nullptr;
#endif
}
constexpr int32 MaxSkinVertices=500000,MaxSkinTriangles=600000,MaxCellPairs=1200000;
constexpr int32 MaxContactVertices=6144,MaxContactTriangles=8192,MaxContactCellPairs=65536;
constexpr int32 MaxBodyFaceVisits=131072,MaxBodyVertices=65536;
constexpr int32 MaxContactRoofCellVisits=262144,MaxContactRoofCells=8192;
constexpr int32 MaxBodyPatchVisits=8192,MaxBodyPatchNodes=32768,BodyPatchLeafFaces=64;
constexpr int32 MaxContactPasses=4;
// Explicit gameplay contact budget: at most 1mm at actual finite faces.
// The separate 0.1mm skin diagnostic remains strict. These are this pilot's
// chosen limits, not a claim about UE defaults. Actual rays/Hull are intact.
constexpr double ProductionContactToleranceCm=.1,DiagnosticContactResolutionCm=.01;
struct FBoundVertex {FVector BoneLocal;int32 Bone=INDEX_NONE;};
struct FBoundMarker {int32 Vertex=INDEX_NONE,Side=0,Element=0;};
enum class EContactKind:uint8 {SoleFace,BodyEnvelope,WheelEnvelope,BodyFace};
struct FContactWitness
{
    FVector NativeActorLocalCm=FVector::ZeroVector;
    int32 Bone=INDEX_NONE,Strip=INDEX_NONE;EContactKind Kind=EContactKind::BodyEnvelope;double GapCm=0;bool Valid=false;
};
struct FSurfaceScan {bool Complete=false,WithinTolerance=false;double MinimumGapCm=0;};
struct FBodyPatchNode
{
    FBox Bounds=FBox(ForceInit);int32 First=0,Count=0,Left=INDEX_NONE,Right=INDEX_NONE;
};
struct FBodyPatchTree {TArray<int32> Faces;TArray<FBodyPatchNode> Nodes;};
struct FMeshCache
{
    TWeakObjectPtr<USkeletalMesh> Mesh;FString ModelId;TArray<FBoundVertex> Vertices;
    TArray<FIntVector> Triangles;TArray<FBoundMarker> Markers;
    // Built once from the already verified rigid binding. Original triangles
    // remain immutable, including every cross-bone face.
    TArray<FBox> BoneLocalBounds;
    TArray<TArray<int32>> SingleBoneTriangles;
    TArray<int32> CrossBoneTriangles;
    // A conservative spatial index of the same original single-bone faces.
    // It never substitutes geometry or supplies contact witnesses.
    TArray<FBodyPatchTree> BodyPatchTrees;
    // Fixed support surface and rigid-body broad phase. A body box that
    // touches ground is resolved against that bone's original finite faces.
    TArray<FBoundVertex> ContactVertices;
    TArray<FIntVector> ContactTriangles;
    TArray<EContactKind> ContactKind;
    TArray<double> WheelMaximumRadiusCm;
    TArray<int32> GearSide;
    TArray<double> GearCentreY;
    TArray<FVector> RestTrackPath;
    bool Wheeled=false;
    double MinimumSupportZ=0;
    TSharedPtr<FJsonObject> ContactContract;
};
struct FMeshFailure {FString ModelId,Reason;TSharedPtr<FJsonObject> Diagnostic;};
struct FState
{
    FGuid Encounter;FString SceneId;uint32 Revision=0;int32 Serial=0;
    TWeakObjectPtr<UStaticMeshComponent> Ground,VerifiedApron;
    TSharedPtr<FJsonObject> VerifiedApronEvidence;
    FWarmSupport Warm,ProposedWarm;bool HasWarm=false,HasProposedWarm=false,ProposedHasWarm=false;
    float LastSupportSearchRejectAt=-1;
#if WITH_EDITOR
    TSharedPtr<FJsonObject> Last,FirstRejected;
    TSharedPtr<FJsonObject> LastSkinAudit,FirstContactSkinAudit;double LastSkinAuditHostAt=-DBL_MAX;
#endif
};
TMap<TWeakObjectPtr<USkeletalMesh>,FMeshCache> MeshCaches;
TMap<TWeakObjectPtr<USkeletalMesh>,FMeshFailure> MeshFailures;
TMap<TWeakObjectPtr<AFrontlineTank>,FState> States;
FVector FlipRawY(const FVector& P){return {P.X,-P.Y,P.Z};}
bool ShoeRestAxes(const FTransform& Frame,const TArray<FVector>& Path,FVector& Normal,FVector& Tangent)
{
    // The rigid shoe angle resides in its real bound geometry. The path is
    // reconstructed once from THIS imported rig, not another vehicle's wheels.
    if(Path.Num()!=145)return false;
    FVector Centre=FlipRawY(Frame.GetTranslation());Centre.Y=0;double Best=DBL_MAX;FVector Along=FVector::ZeroVector;
    for(int32 I=0;I<Path.Num();++I)
    {
        const FVector Edge=Path[(I+1)%Path.Num()]-Path[I];if(Edge.SizeSquared()<1.e-8)continue;
        const double T=FMath::Clamp(FVector::DotProduct(Centre-Path[I],Edge)/Edge.SizeSquared(),0.,1.);
        const double Distance=(Centre-(Path[I]+T*Edge)).SizeSquared();
        // Reference translations are float. At the closed-path seam preserve
        // the first forward segment when the squared distances agree within
        // 1e-5cm position resolution; do not select the preceding arc by noise.
        if(Distance+1.e-10<Best){Best=Distance;Along=Edge.GetSafeNormal();}
    }
    if(!FMath::IsFinite(Best)||Best>.05*.05||Along.IsNearlyZero())return false;
    Normal=Frame.InverseTransformVectorNoScale(FlipRawY(FVector(-Along.Z,0,Along.X))).GetSafeNormal();
    Tangent=Frame.InverseTransformVectorNoScale(FlipRawY(Along)).GetSafeNormal();return true;
}
bool Enabled(const AFrontlineTank* V)
{return V&&Definition(V)&&!Definition(V)->Rail&&(FormalFiveVehicleAcceptance||Review());}

bool Cache(const FFRRUArmourDefinitionV82& Definition,USkeletalMesh* Mesh,USkeletalMeshComponent* Driver,FMeshCache*& Out,FString& Reason,TSharedPtr<FJsonObject> Diag)
{
    auto* Render=Mesh?Mesh->GetResourceForRendering():nullptr;
    auto* LOD=Render&&Render->LODRenderData.IsValidIndex(0)?&Render->LODRenderData[0]:nullptr;
    auto* Weights=Driver?Driver->GetSkinWeightBuffer(0):nullptr;
    FString ModelId=FString::Printf(TEXT("render_lod0:%p:%p:%u:%d"),static_cast<const void*>(LOD),static_cast<const void*>(Weights),LOD?LOD->GetNumVertices():0,LOD&&LOD->IsDataReady()?1:0);
#if WITH_EDITOR
    // Reimport may reuse render resource addresses. Its real source version
    // additionally invalidates both successful and failed editor caches.
    if(const auto* Imported=Mesh?Mesh->GetImportedModel():nullptr)ModelId+=TEXT(":")+Imported->GetIdString();
#endif
    if(auto* Existing=MeshCaches.Find(Mesh))
    {
        if(Existing->ModelId==ModelId){Out=Existing;FT_CONTACT_DIAG(Diag->SetObjectField(TEXT("meshContactContract"),Existing->ContactContract));return true;}
        MeshCaches.Remove(Mesh);
    }
    if(auto* Failed=MeshFailures.Find(Mesh))
    {
        if(Failed->ModelId==ModelId)
        {Reason=Failed->Reason;FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCollisionReported"),false));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("meshContactFailureReused"),true));FT_CONTACT_DIAG(Diag->SetObjectField(TEXT("meshContactContract"),Failed->Diagnostic));return false;}
        MeshFailures.Remove(Mesh);
    }
    [[maybe_unused]] const double CacheBegin=FPlatformTime::Seconds();auto Contract=ContactDiagnostic();
    FT_CONTACT_DIAG(Contract->SetStringField(TEXT("modelId"),ModelId));
    auto Reject=[&](const TCHAR* Why)
    {
        Reason=Why;FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("meshCacheBuildMs"),(FPlatformTime::Seconds()-CacheBegin)*1000.));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCollisionReported"),false));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("meshContactFailureReused"),false));
        FT_CONTACT_DIAG(Diag->SetObjectField(TEXT("meshContactContract"),Contract));
        UE_LOG(LogTemp,Error,TEXT("WF_FRRU_FT_GROUND_CACHE_REJECT mesh=%s reason=%s"),*GetPathNameSafe(Mesh),Why);
        if(Mesh)MeshFailures.Add(Mesh,{ModelId,Reason,Contract});return false;
    };
    if(!Mesh||!Driver||Driver->GetSkeletalMeshAsset()!=Mesh||!LOD||!LOD->IsDataReady()
        ||Driver->LeaderPoseComponent.IsValid())return Reject(TEXT("actual_render_lod_or_driver_missing"));
    const auto& Positions=LOD->StaticVertexBuffers.PositionVertexBuffer;
    if(!Mesh->GetLODInfo(0)||!Mesh->GetLODInfo(0)->bAllowCPUAccess
        ||!Positions.GetAllowCPUAccess()||!Positions.GetVertexData()
        ||!Weights||Weights!=LOD->GetSkinWeightVertexBuffer()||!Weights->GetNeedsCPUAccess()
        ||Weights->GetNumVertices()!=Positions.GetNumVertices()
        ||!Weights->GetDataVertexBuffer()->GetWeightData()
        ||Weights->GetDataVertexBuffer()->GetNumBoneWeights()<Positions.GetNumVertices()
        ||(!Weights->GetVariableBonesPerVertex()
            &&uint64(Weights->GetDataVertexBuffer()->GetNumBoneWeights())!=uint64(Positions.GetNumVertices())*Weights->GetMaxBoneInfluences()))
        return Reject(TEXT("actual_cooked_cpu_positions_or_skin_weights_missing"));
    if(Weights->GetVariableBonesPerVertex()&&(!Weights->GetLookupVertexBuffer()->GetNeedsCPUAccess()
        ||!Weights->GetLookupVertexBuffer()->GetLookupData()
        ||Weights->GetLookupVertexBuffer()->GetNumVertices()!=Positions.GetNumVertices()
        ||uint64(Weights->GetLookupVertexBuffer()->GetVertexDataSize())!=uint64(Positions.GetNumVertices())*sizeof(uint32)))
        return Reject(TEXT("actual_cooked_cpu_skin_lookup_missing"));
    // The two engine Is*DataValid methods are not DLL exports. Require the
    // saved CPU retention policy, real pointers and matching storage counts,
    // then the actual CPU index bytes below before reading any skin data.
    // Whole-LOD ReleaseCPUResources empties these indices, so stale cached
    // position/weight pointers alone cannot make a released LOD pass.
    if(!LOD->MultiSizeIndexContainer.IsIndexBufferValid())return Reject(TEXT("actual_cooked_index_buffer_missing"));
    const auto* Indices=LOD->MultiSizeIndexContainer.GetIndexBuffer();const uint32 IndexSize=LOD->MultiSizeIndexContainer.GetDataTypeSize();
    if(!Indices->GetNeedsCPUAccess()||(IndexSize!=2&&IndexSize!=4)||Indices->Num()<=0||Indices->Num()%3
        ||uint64(Indices->GetResourceDataSize())<uint64(Indices->Num())*IndexSize)
        return Reject(TEXT("actual_cooked_cpu_indices_missing"));
    const auto& Ref=Mesh->GetRefSkeleton();const int32 NumVertices=int32(Positions.GetNumVertices());
    // These are the sealed refined rigs, not guessed wheel/contact positions.
    const bool Wheeled=Definition.Id==TEXT("ru.austin_series1");
    const int32 ExpectedBones=Wheeled?26:Definition.Id==TEXT("fr.saint_chamond")?215:197;
    const int32 ExpectedShoes=Wheeled?0:Definition.Id==TEXT("fr.schneider")?158:Definition.Id==TEXT("fr.saint_chamond")?174:154;
    if(Definition.Rail||(!Wheeled&&Definition.Id!=TEXT("fr.ft")&&Definition.Id!=TEXT("fr.schneider")&&Definition.Id!=TEXT("fr.saint_chamond"))
        ||Ref.GetNum()!=ExpectedBones||Ref.GetBoneName(0)!=TEXT("root")||NumVertices<=0
        ||NumVertices>MaxSkinVertices||Indices->Num()/3>MaxSkinTriangles
        ||Mesh->GetRefBasesInvMatrix().Num()!=Ref.GetNum()||Weights->GetMaxBoneInfluences()==0||Weights->GetMaxBoneInfluences()>MAX_TOTAL_INFLUENCES)
    return Reject(TEXT("actual_vehicle_mesh_contract_or_size"));
    FMeshCache C;C.Mesh=Mesh;C.ModelId=ModelId;C.Vertices.SetNum(NumVertices);C.Wheeled=Wheeled;
    TArray<FTransform> ReferenceFrames;ReferenceFrames.SetNum(Ref.GetNum());
    for(int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex)ReferenceFrames[BoneIndex]=ReferenceFrame(Ref,BoneIndex);
    C.GearSide.Init(0,Ref.GetNum());C.GearCentreY.Init(0.,Ref.GetNum());
    if(!Wheeled)
    {
        const int32 Rear=Ref.FindBoneIndex(TEXT("rear_drive_L")),Front=Ref.FindBoneIndex(TEXT("front_idler_L")),Shoe=Ref.FindBoneIndex(TEXT("shoe_L0"));
        if(Rear==INDEX_NONE||Front==INDEX_NONE||Shoe==INDEX_NONE)return Reject(TEXT("actual_track_reference_bones"));
        const FVector A=FlipRawY(ReferenceFrames[Rear].GetTranslation()),B=FlipRawY(ReferenceFrames[Front].GetTranslation());
        const FVector Top=FlipRawY(ReferenceFrames[Shoe].GetTranslation());
        const double RA=Top.Z-A.Z,RB=RA+B.Z-A.Z;
        if(!FMath::IsFinite(RA)||!FMath::IsFinite(RB)||RA<=0||RB<=0||B.X<=A.X
            ||FMath::Abs(Top.X-A.X)>.05||FMath::Abs(Top.Y-A.Y)>.05||FMath::Abs(A.Y-B.Y)>.05)
            return Reject(TEXT("actual_closed_track_reference_path"));
        // r2 refine_vehicles.py's closed 33/40/32/40 path. Every dimension
        // comes from the imported reference rig; each shoe is then checked
        // against that path and its actual render outer face below.
        for(int32 I=0;I<=32;++I){const double T=double(I)/32;C.RestTrackPath.Add({FMath::Lerp(A.X,B.X,T),0,A.Z+RA+2*(RB-RA)*T});}
        for(int32 I=1;I<=40;++I){const double Angle=PI/2-PI*I/40;C.RestTrackPath.Add({B.X+RB*FMath::Cos(Angle),0,B.Z+RB*FMath::Sin(Angle)});}
        for(int32 I=1;I<=32;++I){const double T=double(I)/32;C.RestTrackPath.Add({FMath::Lerp(B.X,A.X,T),0,A.Z-RA});}
        for(int32 I=1;I<=40;++I){const double Angle=-PI/2-PI*I/40;C.RestTrackPath.Add({A.X+RA*FMath::Cos(Angle),0,A.Z+RA*FMath::Sin(Angle)});}
        FT_CONTACT_DIAG(Contract->SetStringField(TEXT("actualTrackRearCentreCm"),A.ToString()));
        FT_CONTACT_DIAG(Contract->SetStringField(TEXT("actualTrackFrontCentreCm"),B.ToString()));
        FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("actualTrackRearRadiusCm"),RA));FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("actualTrackFrontRadiusCm"),RB));
    }
    C.SingleBoneTriangles.SetNum(Ref.GetNum());
    for(int32 I=0;I<Ref.GetNum();++I)C.BoneLocalBounds.Add(FBox(ForceInit));
    TArray<bool> Written;Written.Init(false,NumVertices);
    TArray<TArray<int32>> ShoeVertices,WheelVertices;ShoeVertices.SetNum(Ref.GetNum());WheelVertices.SetNum(Ref.GetNum());
    TArray<bool> WheelBones;WheelBones.Init(false,Ref.GetNum());C.WheelMaximumRadiusCm.Init(0.,Ref.GetNum());
    for(int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex)
    {
        const FString Label=Ref.GetBoneName(BoneIndex).ToString();
        WheelBones[BoneIndex]=Label.StartsWith(TEXT("road_"))||Label.StartsWith(TEXT("return_"))
            ||Label.StartsWith(TEXT("front_idler_"))||Label.StartsWith(TEXT("rear_drive_"))
            ||(Wheeled&&(Label.StartsWith(TEXT("rear_wheel_"))||Label.StartsWith(TEXT("front_wheel_"))));
        if(Label.StartsWith(TEXT("shoe_"))||(Wheeled&&WheelBones[BoneIndex]))
        {
            const FVector Centre=FlipRawY(ReferenceFrames[BoneIndex].GetTranslation());
            if(FMath::Abs(Centre.Y)<1.)return Reject(TEXT("actual_gear_side_reference"));
            C.GearSide[BoneIndex]=Centre.Y<0?-1:1;C.GearCentreY[BoneIndex]=Centre.Y;
        }
    }
    for(const auto& Section:LOD->RenderSections)for(uint32 I=0;I<Section.NumVertices;++I)
    {
        const int32 Index=int32(Section.BaseVertexIndex)+I;
        if(!C.Vertices.IsValidIndex(Index)||Written[Index])return Reject(TEXT("actual_vertex_index"));
        int32 BoneIndex=INDEX_NONE,Positive=0;uint32 Weight=0;
        for(uint32 J=0;J<Weights->GetMaxBoneInfluences();++J)
        {
            const uint32 BoneMapIndex=Weights->GetBoneIndex(Index,J);const uint16 InfluenceWeight=Weights->GetBoneWeight(Index,J);
            // Official GetTypedSkinnedVertexPosition also uses Section.BoneMap,
            // including sections populated with the 5.8 unified bone map.
            if(!Section.BoneMap.IsValidIndex(BoneMapIndex))return Reject(TEXT("actual_skin_bone"));
            if(InfluenceWeight){BoneIndex=Section.BoneMap[BoneMapIndex];Weight+=InfluenceWeight;++Positive;}
        }
        // Refined mechanical geometry is rigid-bound; an unexpected blended
        // asset is refused rather than evaluated with an invented rigid rule.
        if(Positive!=1||Weight!=65535||BoneIndex<0||BoneIndex>=Ref.GetNum())
        return Reject(TEXT("actual_rigid_skin_required"));
        // This is the exact rigid reduction of the engine's weighted skinning:
        // RefBasesInvMatrix[mesh bone] * component-space compressed bone pose.
        // The real cooked weight must be unity; blended geometry is refused.
        const FVector3f RawLocal(Mesh->GetRefBasesInvMatrix()[BoneIndex].TransformPosition(Positions.VertexPosition(Index)));
        const FVector Local(RawLocal);
        if(Local.ContainsNaN())return Reject(TEXT("actual_render_position_not_finite"));
        C.Vertices[Index]={Local,BoneIndex};
        C.BoneLocalBounds[BoneIndex]+=C.Vertices[Index].BoneLocal;
        Written[Index]=true;
        if(Ref.GetBoneName(BoneIndex).ToString().StartsWith(TEXT("shoe_")))ShoeVertices[BoneIndex].Add(Index);
        if(WheelBones[BoneIndex])WheelVertices[BoneIndex].Add(Index);
    }
    for(bool Present:Written)if(!Present)return Reject(TEXT("actual_vertex_unwritten"));
    // The cached overload of GetSkinnedVertexPosition in UE 5.8 evaluates the
    // component's current pose. ComputeSkinnedPositions actually consumes the
    // supplied matrices, so validate our rigid reference reduction once here.
    TArray<FMatrix44f> RefToLocal;RefToLocal.SetNum(Ref.GetNum());
    for(int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex)
        RefToLocal[BoneIndex]=Mesh->GetRefBasesInvMatrix()[BoneIndex]*FMatrix44f(ReferenceFrames[BoneIndex].ToMatrixWithScale());
    TArray<FVector3f> ReferencePositions;USkinnedMeshComponent::ComputeSkinnedPositions(Driver,ReferencePositions,RefToLocal,*LOD,*Weights);
    if(ReferencePositions.Num()!=NumVertices)return Reject(TEXT("actual_render_reference_skin_missing"));
    for(int32 Index=0;Index<NumVertices;++Index)
    {
        const auto& Vertex=C.Vertices[Index];const FVector Actual(ReferencePositions[Index]);
        if(Actual.ContainsNaN()||!ReferenceFrames[Vertex.Bone].TransformPosition(Vertex.BoneLocal).Equals(Actual,1.e-3))
            return Reject(TEXT("actual_render_rigid_skin_semantics"));
    }
    for(int32 I=0;I+2<Indices->Num();I+=3)
    {
        const FIntVector T(int32(Indices->Get(I)),int32(Indices->Get(I+1)),int32(Indices->Get(I+2)));
        if(!C.Vertices.IsValidIndex(T.X)||!C.Vertices.IsValidIndex(T.Y)||!C.Vertices.IsValidIndex(T.Z))
        return Reject(TEXT("actual_triangle_index"));
        const int32 TriangleIndex=C.Triangles.Add(T);
        const int32 B0=C.Vertices[T.X].Bone,B1=C.Vertices[T.Y].Bone,B2=C.Vertices[T.Z].Bone;
        if(B0==B1&&B1==B2)C.SingleBoneTriangles[B0].Add(TriangleIndex);
        else C.CrossBoneTriangles.Add(TriangleIndex);
    }
    FT_CONTACT_DIAG(Contract->SetStringField(TEXT("geometrySource"),TEXT("actual_cpu_accessible_cooked_lod0_render_positions_section_maps_and_skin_weights")));
    // Actual exterior sole vertices; the path normal selects the band only.
    // Actual compressed vertices, full faces and physical rays govern fit.
    for(int32 BoneIndex=0;BoneIndex<ShoeVertices.Num();++BoneIndex)if(!ShoeVertices[BoneIndex].IsEmpty())
    {
        const FString Label=Ref.GetBoneName(BoneIndex).ToString();
        const int32 Side=Label.StartsWith(TEXT("shoe_L"))?-1:Label.StartsWith(TEXT("shoe_R"))?1:0;
        auto RejectBone=[&](const TCHAR* Why){FT_CONTACT_DIAG(Contract->SetStringField(TEXT("failedBone"),Label));return Reject(Why);};
        if(!Side)return RejectBone(TEXT("actual_shoe_side"));
        const FTransform Frame=ReferenceFrame(Ref,BoneIndex);FVector Outward,Along;
        if(!ShoeRestAxes(Frame,C.RestTrackPath,Outward,Along))return RejectBone(TEXT("actual_shoe_rest_path"));
        const FVector Across=Frame.InverseTransformVectorNoScale(FVector(0,-1,0)).GetSafeNormal();
        if(FMath::Abs(FVector::DotProduct(Outward,Along))>1.e-6||FMath::Abs(FVector::DotProduct(Outward,Across))>1.e-6
            ||FMath::Abs(FVector::DotProduct(Along,Across))>1.e-6)return RejectBone(TEXT("actual_shoe_contact_axes"));
        double Outer=-DBL_MAX;for(int32 VIndex:ShoeVertices[BoneIndex])Outer=FMath::Max(Outer,FVector::DotProduct(C.Vertices[VIndex].BoneLocal,Outward));
        if(FMath::Abs(Outer-4.2)>.05){FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("outerCm"),Outer));return RejectBone(TEXT("actual_outward_grouser_band"));}
        FVector2D BandMin(DBL_MAX,DBL_MAX),BandMax(-DBL_MAX,-DBL_MAX);int32 BandCount=0;
        for(int32 VIndex:ShoeVertices[BoneIndex])
        {
            const FVector P=C.Vertices[VIndex].BoneLocal;if(FVector::DotProduct(P,Outward)<Outer-1.e-3)continue;
            const FVector2D XY(FVector::DotProduct(P,Along),FVector::DotProduct(P,Across));++BandCount;
            BandMin.X=FMath::Min(BandMin.X,XY.X);BandMin.Y=FMath::Min(BandMin.Y,XY.Y);
            BandMax.X=FMath::Max(BandMax.X,XY.X);BandMax.Y=FMath::Max(BandMax.Y,XY.Y);
        }
        auto RejectSole=[&](const TCHAR* Why)
        {
            FT_CONTACT_DIAG(Contract->SetStringField(TEXT("failedBone"),Label));FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("bandVertices"),BandCount));
            FT_CONTACT_DIAG(Contract->SetStringField(TEXT("bandSpanCm"),(BandMax-BandMin).ToString()));FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("outerCm"),Outer));
            FT_CONTACT_DIAG(Contract->SetStringField(TEXT("quadEnvelopeMinCm"),BandMin.ToString()));FT_CONTACT_DIAG(Contract->SetStringField(TEXT("quadEnvelopeMaxCm"),BandMax.ToString()));
            return Reject(Why);
        };
        if(BandCount<3||BandMax.X-BandMin.X<1.e-6||BandMax.Y-BandMin.Y<1.e-6)return RejectSole(TEXT("actual_sole_band_degenerate"));
        // The actual bevelled outer face supplies BOTH the support vertices and
        // finite contact faces. An inscribed four-marker quad misses its ends;
        // an enclosing rectangle would instead add nonexistent support corners.
        TArray<int32> Band,Contour;
        for(int32 VIndex:ShoeVertices[BoneIndex])
        {
            const FVector P=C.Vertices[VIndex].BoneLocal;
            if(FVector::DotProduct(P,Outward)>=Outer-1.e-3)Band.Add(VIndex);
        }
        auto XY=[&](int32 Index){const FVector& P=C.Vertices[Index].BoneLocal;return FVector2D(FVector::DotProduct(P,Along),FVector::DotProduct(P,Across));};
        auto Turn=[&](int32 A,int32 B,int32 D){const FVector2D AB=XY(B)-XY(A),AD=XY(D)-XY(A);return AB.X*AD.Y-AB.Y*AD.X;};
        Band.Sort([&](int32 A,int32 B){const FVector2D P=XY(A),Q=XY(B);return P.X<Q.X||(P.X==Q.X&&P.Y<Q.Y);});
        for(int32 Index:Band)
        {while(Contour.Num()>=2&&Turn(Contour[Contour.Num()-2],Contour.Last(),Index)<=0.)Contour.Pop(EAllowShrinking::No);Contour.Add(Index);}
        const int32 Lower=Contour.Num();
        for(int32 Index=Band.Num()-2;Index>=0;--Index)
        {while(Contour.Num()>Lower&&Turn(Contour[Contour.Num()-2],Contour.Last(),Band[Index])<=0.)Contour.Pop(EAllowShrinking::No);Contour.Add(Band[Index]);}
        if(!Contour.IsEmpty())Contour.Pop(EAllowShrinking::No);
        if(Contour.Num()<4||Contour.Num()>32)return RejectSole(TEXT("actual_sole_contour_contract"));
        for(int32 Index:Band)for(int32 EdgeIndex=0;EdgeIndex<Contour.Num();++EdgeIndex)
            if(Turn(Contour[EdgeIndex],Contour[(EdgeIndex+1)%Contour.Num()],Index)< -1.e-8)return RejectSole(TEXT("actual_sole_band_not_covered"));
        const int32 SoleFirst=C.ContactVertices.Num();
        for(int32 Index:Contour){const int32 ContactIndex=C.ContactVertices.Add(C.Vertices[Index]);C.Markers.Add({ContactIndex,Side,BoneIndex});}
        for(int32 Index=1;Index+1<Contour.Num();++Index)
        {C.ContactTriangles.Add({SoleFirst,SoleFirst+Index,SoleFirst+Index+1});C.ContactKind.Add(EContactKind::SoleFace);}
    }
    int32 ActualShoes=0;for(const auto& Shoe:ShoeVertices)ActualShoes+=!Shoe.IsEmpty();
    if(ActualShoes!=ExpectedShoes||C.Markers.Num()<ExpectedShoes*4||C.Markers.Num()>MaxMarkers)return Reject(TEXT("actual_shoe_markers_contract"));
    // A rotating wheel box creates corners outside the actual round wheel.
    // Cache its complete actual render XZ convex silhouette, closed across
    // its real axle extents. The axial extrusion remains conservative; no
    // original wheel vertex is omitted and no wheel radius is invented.
    int32 WheelContours=0,MaximumWheelContour=0;
    for(int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex)if(WheelBones[BoneIndex])
    {
        auto RejectWheel=[&](const TCHAR* Why)
        {FT_CONTACT_DIAG(Contract->SetStringField(TEXT("failedBone"),Ref.GetBoneName(BoneIndex).ToString()));return Reject(Why);};
        const FBox& Bound=C.BoneLocalBounds[BoneIndex];auto Sorted=WheelVertices[BoneIndex];
        if(!Bound.IsValid||Sorted.Num()<3)return RejectWheel(TEXT("actual_wheel_vertices"));
        auto XZ=[&](int32 Index){const FVector& P=C.Vertices[Index].BoneLocal;return FVector2D(P.X,P.Z);};
        auto Turn=[&](int32 A,int32 B,int32 D)
        {const FVector2D AB=XZ(B)-XZ(A),AD=XZ(D)-XZ(A);return AB.X*AD.Y-AB.Y*AD.X;};
        Sorted.Sort([&](int32 A,int32 B){const FVector2D P=XZ(A),Q=XZ(B);return P.X<Q.X||(P.X==Q.X&&P.Y<Q.Y);});
        TArray<int32> Contour;
        for(int32 Index:Sorted)
        {while(Contour.Num()>=2&&Turn(Contour[Contour.Num()-2],Contour.Last(),Index)<=0.)Contour.Pop(EAllowShrinking::No);Contour.Add(Index);}
        const int32 Lower=Contour.Num();
        for(int32 Index=Sorted.Num()-2;Index>=0;--Index)
        {while(Contour.Num()>Lower&&Turn(Contour[Contour.Num()-2],Contour.Last(),Sorted[Index])<=0.)Contour.Pop(EAllowShrinking::No);Contour.Add(Sorted[Index]);}
        if(!Contour.IsEmpty())Contour.Pop(EAllowShrinking::No);
        if(Contour.Num()<3||Contour.Num()>128)return RejectWheel(TEXT("actual_wheel_contour_contract"));
        for(int32 Index:Sorted)
        {
            const FVector2D P=XZ(Index);C.WheelMaximumRadiusCm[BoneIndex]=FMath::Max(C.WheelMaximumRadiusCm[BoneIndex],P.Size());
            for(int32 EdgeIndex=0;EdgeIndex<Contour.Num();++EdgeIndex)
                if(Turn(Contour[EdgeIndex],Contour[(EdgeIndex+1)%Contour.Num()],Index)< -1.e-8)
                    return RejectWheel(TEXT("actual_wheel_contour_not_covered"));
        }
        const int32 First=C.ContactVertices.Num(),Count=Contour.Num();
        for(int32 Index:Contour)for(double Y:{Bound.Min.Y,Bound.Max.Y})
        {
            const FVector2D P=XZ(Index);const int32 ContactIndex=C.ContactVertices.Add({FVector(P.X,Y,P.Y),BoneIndex});
            // Austin has four real tyres; its finite axial contour is both
            // the conservative contact surface and the source of support.
            if(Wheeled)C.Markers.Add({ContactIndex,C.GearSide[BoneIndex],BoneIndex});
        }
        auto AddWheelFace=[&](int32 A,int32 B,int32 D)
        {C.ContactTriangles.Add({First+A,First+B,First+D});C.ContactKind.Add(EContactKind::WheelEnvelope);};
        for(int32 Index=0;Index<Count;++Index)
        {
            const int32 Next=(Index+1)%Count;AddWheelFace(2*Index,2*Next,2*Next+1);AddWheelFace(2*Index,2*Next+1,2*Index+1);
        }
        for(int32 Index=1;Index+1<Count;++Index)
        {AddWheelFace(0,2*(Index+1),2*Index);AddWheelFace(1,2*Index+1,2*(Index+1)+1);}
        ++WheelContours;MaximumWheelContour=FMath::Max(MaximumWheelContour,Count);
    }
    if(WheelContours!=(Wheeled?4:30)||C.Markers.Num()<3||C.Markers.Num()>MaxMarkers)return Reject(TEXT("actual_wheel_contours_contract"));
    C.MinimumSupportZ=DBL_MAX;
    for(const auto& Marker:C.Markers)
    {const auto& Vertex=C.ContactVertices[Marker.Vertex];C.MinimumSupportZ=FMath::Min(C.MinimumSupportZ,ReferenceFrames[Vertex.Bone].TransformPosition(Vertex.BoneLocal).Z);}
    // The other actual rigid bones retain conservative broad-phase boxes.
    // Empty box corners are not a physical surface: SkinGround resolves any
    // penetrating body box against the same posed original rigid faces.
    const int32 BoxFaces[12][3]={{0,1,3},{0,3,2},{4,6,7},{4,7,5},
        {0,4,5},{0,5,1},{2,3,7},{2,7,6},{0,2,6},{0,6,4},{1,5,7},{1,7,3}};
    C.BodyPatchTrees.SetNum(Ref.GetNum());int32 BodyPatchNodes=0;
    for(int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex)
    {
        const FBox& Bound=C.BoneLocalBounds[BoneIndex];
        if(!Bound.IsValid||!ShoeVertices[BoneIndex].IsEmpty()||WheelBones[BoneIndex])continue;
        auto& Tree=C.BodyPatchTrees[BoneIndex];Tree.Faces=C.SingleBoneTriangles[BoneIndex];
        TMap<int32,FVector> Centres;Centres.Reserve(Tree.Faces.Num());
        for(int32 FaceIndex:Tree.Faces)
        {
            const auto& T=C.Triangles[FaceIndex];
            Centres.Add(FaceIndex,(C.Vertices[T.X].BoneLocal+C.Vertices[T.Y].BoneLocal+C.Vertices[T.Z].BoneLocal)/3.);
        }
        TFunction<int32(int32,int32)> BuildPatch=[&](int32 FirstFace,int32 Count)->int32
        {
            if(++BodyPatchNodes>MaxBodyPatchNodes)return INDEX_NONE;
            const int32 NodeIndex=Tree.Nodes.AddDefaulted();FBox PatchBound(ForceInit),CentreBound(ForceInit);
            for(int32 J=FirstFace;J<FirstFace+Count;++J)
            {
                const int32 FaceIndex=Tree.Faces[J];const auto& T=C.Triangles[FaceIndex];
                PatchBound+=C.Vertices[T.X].BoneLocal;PatchBound+=C.Vertices[T.Y].BoneLocal;PatchBound+=C.Vertices[T.Z].BoneLocal;
                CentreBound+=Centres.FindChecked(FaceIndex);
            }
            Tree.Nodes[NodeIndex].Bounds=PatchBound;Tree.Nodes[NodeIndex].First=FirstFace;Tree.Nodes[NodeIndex].Count=Count;
            if(Count>BodyPatchLeafFaces)
            {
                const FVector Extent=CentreBound.GetSize();const int32 Axis=Extent.X>=Extent.Y&&Extent.X>=Extent.Z?0:Extent.Y>=Extent.Z?1:2;
                Algo::Sort(MakeArrayView(Tree.Faces.GetData()+FirstFace,Count),[&](int32 A,int32 B)
                {const double CA=Centres.FindChecked(A)[Axis],CB=Centres.FindChecked(B)[Axis];return CA==CB?A<B:CA<CB;});
                const int32 Left=BuildPatch(FirstFace,Count/2),Right=BuildPatch(FirstFace+Count/2,Count-Count/2);
                if(Left==INDEX_NONE||Right==INDEX_NONE)return INDEX_NONE;
                Tree.Nodes[NodeIndex].Left=Left;Tree.Nodes[NodeIndex].Right=Right;
            }
            return NodeIndex;
        };
        if(!Tree.Faces.IsEmpty()&&BuildPatch(0,Tree.Faces.Num())!=0)return Reject(TEXT("actual_body_patch_node_budget"));
        const int32 First=C.ContactVertices.Num();
        for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)for(int32 Z=0;Z<2;++Z)
            C.ContactVertices.Add({FVector(X?Bound.Max.X:Bound.Min.X,Y?Bound.Max.Y:Bound.Min.Y,Z?Bound.Max.Z:Bound.Min.Z),BoneIndex});
        for(const auto& Face:BoxFaces){C.ContactTriangles.Add({First+Face[0],First+Face[1],First+Face[2]});C.ContactKind.Add(EContactKind::BodyEnvelope);}
    }
    if(C.ContactVertices.Num()>MaxContactVertices||C.ContactTriangles.Num()>MaxContactTriangles
        ||C.ContactKind.Num()!=C.ContactTriangles.Num())
    return Reject(TEXT("bounded_contact_surface_contract"));
    FT_CONTACT_DIAG(Contract->SetStringField(TEXT("soleSurface"),TEXT("actual_outer_band_convex_faces")));FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("soleSupportVertices"),C.Markers.Num()));
    FT_CONTACT_DIAG(Contract->SetStringField(TEXT("wheelSurface"),TEXT("actual_xz_convex_contour_axial_envelope")));
    FT_CONTACT_DIAG(Contract->SetStringField(TEXT("vehicle"),Definition.Id));FT_CONTACT_DIAG(Contract->SetBoolField(TEXT("wheeledSupport"),Wheeled));
    FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("wheelContours"),WheelContours));FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("maximumWheelContourVertices"),MaximumWheelContour));
    FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("boundedContactVertices"),C.ContactVertices.Num()));FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("boundedContactTriangles"),C.ContactTriangles.Num()));
    FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("actualBodyPatchNodes"),BodyPatchNodes));
    FT_CONTACT_DIAG(Contract->SetNumberField(TEXT("meshCacheBuildMs"),(FPlatformTime::Seconds()-CacheBegin)*1000.));
    C.ContactContract=Contract;FT_CONTACT_DIAG(Diag->SetObjectField(TEXT("meshContactContract"),Contract));
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_FT_GROUND_CACHE source=cooked_lod0 mesh=%s markers=%d contactVertices=%d contactTriangles=%d cacheMs=%.3f"),*Mesh->GetPathName(),C.Markers.Num(),C.ContactVertices.Num(),C.ContactTriangles.Num(),(FPlatformTime::Seconds()-CacheBegin)*1000.);
    Out=&MeshCaches.Add(Mesh,MoveTemp(C));return true;
}

bool CompressedPose(USkeletalMesh* Mesh,UAnimSequence* Drive,double Phase,TArray<FTransform>& Out,FString& Reason)
{
    auto* Skeleton=Drive?Drive->GetSkeleton():nullptr;
    if(!Mesh||!Skeleton||Skeleton!=Mesh->GetSkeleton()||!Drive->IsCompressedDataValid())
    {Reason=TEXT("actual_compressed_drive");return false;}
    const auto& Ref=Mesh->GetRefSkeleton();TArray<FBoneIndexType> Indices;
    for(int32 I=0;I<Ref.GetNum();++I)Indices.Add(FBoneIndexType(I));
    FBoneContainer Bones(Indices,UE::Anim::FCurveFilterSettings(),*Mesh);
    FAnimExtractContext Context(Phase,false);
#if WITH_EDITOR
    Context.bEnforceCompressedDataSampling=true;
#endif
    const auto* ForceRaw=IConsoleManager::Get().FindConsoleVariable(TEXT("a.ForceEvalRawData"));
    const auto Compressed=Drive->GetCompressedData(Context);const auto& ActualData=Compressed.Get();
    // IsValid() alone is an unconditional true in Shipping. Require the real
    // runtime codec, track map and resident byte stream consumed by extraction.
    if((ForceRaw&&ForceRaw->GetInt()==1)||!ActualData.IsValid(Drive)
        ||!ActualData.CompressedDataStructure||!ActualData.BoneCompressionCodec
        ||ActualData.CompressedByteStream.Num()<=0||ActualData.CompressedTrackToSkeletonMapTable.IsEmpty()
        ||Drive->GetSkeletonVirtualBoneGuid()!=Skeleton->GetVirtualBoneGuid()
        ||Bones.GetDisableRetargeting()||Bones.ShouldUseRawData()||Bones.ShouldUseSourceData())
    {Reason=TEXT("compressed_path_fallback_refused");return false;}
    FMemMark Mark(FMemStack::Get());FCompactPose Pose;Pose.SetBoneContainer(&Bones);Pose.ResetToRefPose();
    FBlendedCurve Curve;Curve.InitFrom(Bones);UE::Anim::FStackAttributeContainer Attributes;
    FAnimationPoseData Data(Pose,Curve,Attributes);Drive->GetAnimationPose(Data,Context);
    Out.SetNum(Ref.GetNum());
    for(int32 I=0;I<Ref.GetNum();++I)
    {
        const FCompactPoseBoneIndex Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(I));
        FTransform Local=I==0?Ref.GetRefBonePose()[0]:Pose[Compact];
        const int32 Parent=Ref.GetParentIndex(I);Out[I]=Parent==INDEX_NONE?Local:Local*Out[Parent];
        if(Out[I].ContainsNaN()){Reason=TEXT("compressed_pose_not_finite");return false;}
    }
    return true;
}

bool Context(AFrontlineTank* V,FState& S,const wfexp::scene::SceneDefinition*& G,FString& Reason)
{
    const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;G=Scene(B);
    FString SceneId;uint32 Revision=0;int32 Solids=0;FVector2D Min,Max;
    if(!R||!B||R->Encounter!=B->GetEncounterId()||!G||!Enabled(V)
        ||!WarfareFRRUWorldV82::SceneInfo(B,SceneId,Revision,Solids,Min,Max)
        ||SceneId!=wfexpue::Wide(G->id)||Revision!=G->revision
        ||!WarfareFRRUContentV82::SceneStaged(B,SceneId,Revision))
    {Reason=TEXT("exact_current_scene_context");return false;}
    if(!R->Model.IsValid()||!R->Driver.IsValid()||!V->GetActorScale3D().Equals(FVector::OneVector,1.e-6)
        ||!R->Model->GetRelativeScale3D().Equals(FVector(1,-1,1),1.e-6)
        ||!R->Model->GetRelativeLocation().IsNearlyZero(1.e-6)||!R->Model->GetRelativeRotation().IsNearlyZero(1.e-6)
        ||!R->Driver->GetRelativeScale3D().Equals(FVector::OneVector,1.e-6)
        ||!R->Driver->GetRelativeLocation().IsNearlyZero(1.e-6)||!R->Driver->GetRelativeRotation().IsNearlyZero(1.e-6))
    {Reason=TEXT("actual_actor_driver_model_basis");return false;}
    if(S.Encounter!=R->Encounter||S.SceneId!=SceneId||S.Revision!=Revision||S.Serial!=V->Serial)
    {S=FState{};S.Encounter=R->Encounter;S.SceneId=SceneId;S.Revision=Revision;S.Serial=V->Serial;}
    if(!S.Ground.IsValid())
    {
        // Content has already verified the selected native scene/revision.
        // Read its actual staged ground identity once, rather than reproducing
        // the legacy/receiver/world-family asset naming rules in this pilot.
        const auto Rendering=WarfareFRRUContentV82::DescribeSceneRendering(B);
        FString RenderingScene,Exact;const TArray<TSharedPtr<FJsonValue>>* Parts=nullptr;
        if(!Rendering.IsValid()||!Rendering->TryGetStringField(TEXT("sceneId"),RenderingScene)
            ||RenderingScene!=SceneId||!Rendering->TryGetArrayField(TEXT("components"),Parts)||!Parts)
        {Reason=TEXT("actual_staged_ground_identity_missing");return false;}
        for(const auto& Part:*Parts)if(Part.IsValid())
        {
            const auto Item=Part->AsObject();FString Name,MeshPath;
            if(!Item.IsValid()||!Item->TryGetStringField(TEXT("component"),Name)||Name!=TEXT("ground"))continue;
            if(!Item->TryGetStringField(TEXT("mesh"),MeshPath)||MeshPath.IsEmpty())
            {Reason=TEXT("actual_staged_ground_identity_missing");return false;}
            if(!Exact.IsEmpty()){Reason=TEXT("ambiguous_actual_staged_ground_identity");return false;}Exact=MeshPath;
        }
        if(Exact.IsEmpty()){Reason=TEXT("actual_staged_ground_identity_missing");return false;}
        UStaticMeshComponent* Found=nullptr;
        for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(TEXT("FRRU_V82Scene")))
        {
            TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
            for(auto* C:Components)if(C&&C->IsRegistered()&&C->ComponentHasTag(TEXT("ground"))
                &&C->ComponentHasTag(TEXT("FRRU_V82Collision"))&&C->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics
                &&C->IsPhysicsStateCreated()&&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==Exact)
            {if(Found){Reason=TEXT("ambiguous_actual_ground");return false;}Found=C;}
        }
        if(!Found){Reason=TEXT("actual_ground_component_missing");return false;}S.Ground=Found;
    }
    return true;
}

// Clip an actual posed skin triangle against the XY projection of each native
// finite 00->11 triangle. Both heights are affine within that intersection;
// the exact minimum difference is therefore at an intersection vertex.
double CrossXY(const FVector& A,const FVector& B){return A.X*B.Y-A.Y*B.X;}
void Clip(const TArray<FVector>& Input,const FVector& A,const FVector& B,TArray<FVector>& Output)
{
    Output.Reset();if(Input.IsEmpty())return;const FVector Edge=B-A;
    FVector Previous=Input.Last();double Prev=CrossXY(Edge,Previous-A);
    for(const FVector& P:Input)
    {
        const double Value=CrossXY(Edge,P-A);const bool In=Value>=-1.e-8,PrevIn=Prev>=-1.e-8;
        if(In!=PrevIn)Output.Add(FMath::Lerp(Previous,P,Prev/(Prev-Value)));
        if(In)Output.Add(P);Previous=P;Prev=Value;
    }
}
bool SkinGround(const FMeshCache& C,const TArray<FTransform>& Pose,const FTransform& Actor,
    const wfexp::scene::SceneDefinition& G,TSharedPtr<FJsonObject> Diag,FString& Reason,bool FullSkin,int32 AuditBone=INDEX_NONE,TArray<FContactWitness>* Witnesses=nullptr,FSurfaceScan* Result=nullptr)
{
    const auto& Vertices=FullSkin?C.Vertices:C.ContactVertices;
    const auto& Triangles=FullSkin?C.Triangles:C.ContactTriangles;
    [[maybe_unused]] const double SkinBegin=FPlatformTime::Seconds();
    int32 Transformed=0,Pairs=0,Relevant=0,Skipped=0,SkippedBones=0,CrossProcessed=0,RoofVertices=0;
    int32 BodyBones=0,BodyFaces=0,BodyTransformed=0,BodyPairs=0,BodyPatchVisits=0,BodyPatchSkippedFaces=0;
    int32 LocalRoofVisits=0,LocalRoofReads=0,LocalRoofSkipped=0;
    TMap<int32,double> LocalRoofCells;
    double Minimum=DBL_MAX,RecordedGap=DBL_MAX;
    const double Resolution=FullSkin?DiagnosticContactResolutionCm:ProductionContactToleranceCm;
    bool ScanComplete=false,ContactViolation=false;FString NonSoleReason;
    FContactWitness DeepestByStrip[4];if(Witnesses)Witnesses->Reset();
    auto Flush=[&]()
    {
        if(Result)
        {
            Result->Complete=ScanComplete;Result->MinimumGapCm=Minimum==DBL_MAX?0:Minimum;
            Result->WithinTolerance=ScanComplete&&Result->MinimumGapCm>=-ProductionContactToleranceCm;
        }
        if(!FullSkin)
        {
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("boundedContactVertices"),Vertices.Num()));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("boundedContactTriangles"),Triangles.Num()));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("boundedContactCellPairs"),Pairs));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("minimumBoundedContactGapCm"),Minimum==DBL_MAX?0:Minimum));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("boundedContactMs"),(FPlatformTime::Seconds()-SkinBegin)*1000.));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactResolutionCm"),ProductionContactToleranceCm));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("diagnosticContactResolutionCm"),DiagnosticContactResolutionCm));
            FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("boundedContactScanComplete"),ScanComplete));
            FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("boundedContactWithinProductionTolerance"),ScanComplete&&!ContactViolation));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyBonesRefined"),BodyBones));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyFaceVisits"),BodyFaces));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyVerticesTransformed"),BodyTransformed));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyCellPairs"),BodyPairs));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyFaceVisitBudget"),MaxBodyFaceVisits));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyVertexBudget"),MaxBodyVertices));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyPatchVisits"),BodyPatchVisits));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyPatchSkippedFaces"),BodyPatchSkippedFaces));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualBodyPatchVisitBudget"),MaxBodyPatchVisits));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("localRoofCellVisits"),LocalRoofVisits));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("localRoofCachedCells"),LocalRoofCells.Num()));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("localRoofHeightReads"),LocalRoofReads));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("localRoofSkippedFaces"),LocalRoofSkipped));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("localRoofCellVisitBudget"),MaxContactRoofCellVisits));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("localRoofCellCacheBudget"),MaxContactRoofCells));
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactPenetrationCm"),Minimum==DBL_MAX?0:FMath::Max(0.,-Minimum)));return;
        }
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinContactResolutionCm"),DiagnosticContactResolutionCm));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("productionContactToleranceCm"),ProductionContactToleranceCm));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("skinScanComplete"),ScanComplete));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("skinWithinProductionTolerance"),ScanComplete&&(Minimum==DBL_MAX||Minimum>=-ProductionContactToleranceCm)));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinVertices"),C.Vertices.Num()));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinTriangles"),C.Triangles.Num()));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinVerticesTransformed"),Transformed));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinBoundSkippedTriangles"),Skipped));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinBoundSkippedBones"),SkippedBones));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinCrossBoneFacesProcessed"),CrossProcessed));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinCrossBoneFacesTotal"),C.CrossBoneTriangles.Num()));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinRoofVerticesRead"),RoofVertices));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("groundRelevantTriangles"),Relevant));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinCellPairs"),Pairs));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("minimumSkinGapCm"),Minimum==DBL_MAX?0:Minimum));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinGroundMs"),(FPlatformTime::Seconds()-SkinBegin)*1000.));
    };
    auto Reject=[&](const TCHAR* Why){Reason=Why;Flush();return false;};
    const bool BoneAudit=FullSkin&&AuditBone!=INDEX_NONE;
    if(BoneAudit)
    {
        if(!C.SingleBoneTriangles.IsValidIndex(AuditBone))return Reject(TEXT("actual_bone_audit_index"));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("skinBoneAuditOnly"),true));FT_CONTACT_DIAG(Diag->SetStringField(TEXT("skinAuditBone"),C.Mesh->GetRefSkeleton().GetBoneName(AuditBone).ToString()));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("skinAuditBoneTriangles"),C.SingleBoneTriangles[AuditBone].Num()));
        for(int32 TriangleIndex:C.CrossBoneTriangles)
        {
            const auto& T=C.Triangles[TriangleIndex];
            if(C.Vertices[T.X].Bone==AuditBone||C.Vertices[T.Y].Bone==AuditBone||C.Vertices[T.Z].Bone==AuditBone)
                return Reject(TEXT("actual_bone_audit_cross_binding"));
        }
    }
    // Deck caps/aprons are baked into the same authoritative native grid.
    // Actual support rays must hit this registered ground, and FullHull still
    // blocks every independent deck/solid component. A distant deck is not a
    // reason to reject an otherwise supported pose elsewhere in the scene.
    if(C.BoneLocalBounds.Num()!=Pose.Num()||C.SingleBoneTriangles.Num()!=Pose.Num()||C.BodyPatchTrees.Num()!=Pose.Num())return Reject(TEXT("actual_rigid_bound_cache"));
    const double DX=(G.maxX-G.minX)/(G.columns-1),DY=(G.maxY-G.minY)/(G.rows-1);
    TArray<FBox> WorldBones;WorldBones.SetNum(Pose.Num());FBox ConservativeSkinBox(ForceInit);
    for(int32 BoneIndex=0;BoneIndex<Pose.Num();++BoneIndex)
    {
        if(BoneAudit&&BoneIndex!=AuditBone)continue;
        FBox Bound(ForceInit);const FBox& Local=C.BoneLocalBounds[BoneIndex];
        if(Local.IsValid)for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)for(int32 Z=0;Z<2;++Z)
        {
            const FVector Corner(X?Local.Max.X:Local.Min.X,Y?Local.Max.Y:Local.Min.Y,Z?Local.Max.Z:Local.Min.Z);
            Bound+=Actor.TransformPosition(FlipRawY(Pose[BoneIndex].TransformPosition(Corner)));
        }
        // Outward padding concerns the conservative envelope's arithmetic,
        // not penetration/contact tolerance. It cannot make a face pass.
        if(Bound.IsValid){Bound=Bound.ExpandBy(1.e-4);ConservativeSkinBox+=Bound;}
        WorldBones[BoneIndex]=Bound;
    }
    auto InGrid=[&](const FBox& Box)
    {return Box.IsValid&&Box.Min.X>=G.minX&&Box.Max.X<=G.maxX&&Box.Min.Y>=G.minY&&Box.Max.Y<=G.maxY;};
    auto Roof=[&](const FBox& Box)
    {
        const int32 X0=FMath::Clamp(FMath::FloorToInt((Box.Min.X-G.minX)/DX),0,G.columns-2),X1=FMath::Clamp(FMath::FloorToInt((Box.Max.X-G.minX)/DX),0,G.columns-2);
        const int32 Y0=FMath::Clamp(FMath::FloorToInt((Box.Min.Y-G.minY)/DY),0,G.rows-2),Y1=FMath::Clamp(FMath::FloorToInt((Box.Max.Y-G.minY)/DY),0,G.rows-2);
        double High=-DBL_MAX;for(int32 Y=Y0;Y<=Y1+1;++Y)for(int32 X=X0;X<=X1+1;++X)
        {High=FMath::Max(High,double(G.heightsCm[Y*G.columns+X]));++RoofVertices;}
        return High;
    };
    TArray<FVector> World;World.SetNumUninitialized(Vertices.Num());TArray<uint8> VertexReady;VertexReady.Init(0,Vertices.Num());
    auto Vertex=[&](int32 I)->const FVector&
    {
        if(!VertexReady[I])
        {const auto& V=Vertices[I];World[I]=Actor.TransformPosition(FlipRawY(Pose[V.Bone].TransformPosition(V.BoneLocal)));VertexReady[I]=1;++Transformed;}
        return World[I];
    };
    // Allocated only when a body box intersects native ground. These are the
    // original cached rigid vertices, posed by THIS compressed candidate.
    TArray<FVector> BodyWorld;TArray<uint8> BodyReady;
    auto BodyVertex=[&](int32 I)->const FVector&
    {
        if(!BodyReady[I])
        {const auto& V=C.Vertices[I];BodyWorld[I]=Actor.TransformPosition(FlipRawY(Pose[V.Bone].TransformPosition(V.BoneLocal)));BodyReady[I]=1;++BodyTransformed;}
        return BodyWorld[I];
    };
    TArray<double> BodyMinimum;if(!FullSkin)BodyMinimum.Init(DBL_MAX,Pose.Num());
    // A transformed bone AABB is an overestimate. If it crosses a scene edge,
    // use the original exact-vertex box/loop instead of rejecting valid empty
    // corners or clipping an actual outside vertex into the native domain.
    const bool UseBounds=FullSkin&&InGrid(ConservativeSkinBox);FBox SkinBox=ConservativeSkinBox;
    if(!UseBounds){SkinBox=FBox(ForceInit);for(int32 I=0;I<Vertices.Num();++I)if(!BoneAudit||Vertices[I].Bone==AuditBone)SkinBox+=Vertex(I);}
    if(!InGrid(SkinBox))return Reject(TEXT("ft_actual_skin_outside_native_grid"));
    const double GlobalRoof=Roof(SkinBox);
    auto Point=[&](int32 X,int32 Y){return FVector(G.minX+X*DX,G.minY+Y*DY,G.heightsCm[Y*G.columns+X]);};
    auto LocalRoof=[&](int32 X0,int32 X1,int32 Y0,int32 Y1,double& High)
    {
        High=-DBL_MAX;
        for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)
        {
            if(++LocalRoofVisits>MaxContactRoofCellVisits){Reason=TEXT("local_roof_cell_visit_budget");return false;}
            const int32 Key=Y*(G.columns-1)+X;const double* Cached=LocalRoofCells.Find(Key);
            if(!Cached)
            {
                if(LocalRoofCells.Num()>=MaxContactRoofCells){Reason=TEXT("local_roof_cell_cache_budget");return false;}
                double CellHigh=-DBL_MAX;
                for(int32 YY=Y;YY<=Y+1;++YY)for(int32 XX=X;XX<=X+1;++XX)
                {
                    const double Z=G.heightsCm[YY*G.columns+XX];++LocalRoofReads;
                    if(!FMath::IsFinite(Z)){Reason=TEXT("local_roof_height_not_finite");return false;}
                    CellHigh=FMath::Max(CellHigh,Z);
                }
                Cached=&LocalRoofCells.Add(Key,CellHigh);
            }
            High=FMath::Max(High,*Cached);
        }
        return true;
    };
    TArray<FVector> Poly,Buffer;Poly.Reserve(8);Buffer.Reserve(8);
    auto Face=[&](int32 TriangleIndex,double GroundRoof,bool ActualBody=false,bool BodyProbe=false)
    {
        const auto& FaceVertices=ActualBody?C.Vertices:Vertices;
        const auto& T=ActualBody?C.Triangles[TriangleIndex]:Triangles[TriangleIndex];
        if(ActualBody&&++BodyFaces>MaxBodyFaceVisits){Reason=TEXT("actual_body_face_visit_budget");return false;}
        const FVector V[3]={ActualBody?BodyVertex(T.X):Vertex(T.X),ActualBody?BodyVertex(T.Y):Vertex(T.Y),ActualBody?BodyVertex(T.Z):Vertex(T.Z)};
        if(ActualBody&&BodyTransformed>MaxBodyVertices){Reason=TEXT("actual_body_vertex_budget");return false;}
        // Every possibly contacting face still receives the finite-face test.
        // Conservative bounds never replace an actual contact test.
        if(FMath::Min3(V[0].Z,V[1].Z,V[2].Z)>GroundRoof+1.e-4){++Skipped;return true;}++Relevant;
        const int32 X0=FMath::Clamp(FMath::FloorToInt((FMath::Min3(V[0].X,V[1].X,V[2].X)-G.minX)/DX),0,G.columns-2),X1=FMath::Clamp(FMath::FloorToInt((FMath::Max3(V[0].X,V[1].X,V[2].X)-G.minX)/DX),0,G.columns-2);
        const int32 Y0=FMath::Clamp(FMath::FloorToInt((FMath::Min3(V[0].Y,V[1].Y,V[2].Y)-G.minY)/DY),0,G.rows-2),Y1=FMath::Clamp(FMath::FloorToInt((FMath::Max3(V[0].Y,V[1].Y,V[2].Y)-G.minY)/DY),0,G.rows-2);
        if(!FullSkin)
        {
            // Native height is affine on each finite grid triangle, so the
            // greatest of all four covered cell vertices is an upper bound.
            // A posed face lies in its vertex convex hull. This local proof
            // discards no possible contact, including vertical rigid faces.
            double FaceRoof=0;if(!LocalRoof(X0,X1,Y0,Y1,FaceRoof))return false;
            if(FMath::Min3(V[0].Z,V[1].Z,V[2].Z)>FaceRoof+1.e-4)
            {++Skipped;++LocalRoofSkipped;return true;}
        }
        for(int32 Y=Y0;Y<=Y1;++Y)for(int32 X=X0;X<=X1;++X)for(int32 Half=0;Half<2;++Half)
        {
            if(ActualBody)++BodyPairs;
            if(++Pairs>(FullSkin?MaxCellPairs:MaxContactCellPairs))
            {Reason=FullSkin?TEXT("actual_skin_cell_budget"):TEXT("bounded_contact_cell_budget");return false;}
            const FVector Ground[3]={Point(X,Y),Half?Point(X+1,Y+1):Point(X+1,Y),Half?Point(X,Y+1):Point(X+1,Y+1)};
            Poly.Reset();Poly.Append(V,3);for(int32 E=0;E<3&&!Poly.IsEmpty();++E){Clip(Poly,Ground[E],Ground[(E+1)%3],Buffer);Swap(Poly,Buffer);}
            const FVector N=FVector::CrossProduct(Ground[1]-Ground[0],Ground[2]-Ground[0]);
            for(const FVector& P:Poly)
            {
                const double Floor=Ground[0].Z-(N.X*(P.X-Ground[0].X)+N.Y*(P.Y-Ground[0].Y))/N.Z;
                const double Gap=P.Z-Floor;
                if(!FMath::IsFinite(Gap)){Reason=TEXT("actual_contact_gap_not_finite");return false;}
                // The conservative box is only a broad phase. Its empty
                // corners never enter the real contact minimum or witnesses.
                if(BodyProbe){const int32 Bone=FaceVertices[T.X].Bone;BodyMinimum[Bone]=FMath::Min(BodyMinimum[Bone],Gap);continue;}
                Minimum=FMath::Min(Minimum,Gap);
                if(Gap< -Resolution)
                {
                    ContactViolation=true;
                    // Complete separation scan: report the deepest point, not
                    // whichever triangle happened to be visited first.
                    if(Gap<RecordedGap)
                    {
                        RecordedGap=Gap;
                        FT_CONTACT_DIAG(Diag->SetStringField(FullSkin?TEXT("firstSkinPoint"):ActualBody?TEXT("firstActualBodyPoint"):TEXT("firstContactPoint"),P.ToString()));
                        FT_CONTACT_DIAG(Diag->SetNumberField(FullSkin?TEXT("skinPenetrationCm"):TEXT("contactPenetrationCm"),-Gap));
                        FT_CONTACT_DIAG(Diag->SetNumberField(FullSkin?TEXT("firstSkinTriangle"):ActualBody?TEXT("firstActualBodyTriangle"):TEXT("firstContactTriangle"),TriangleIndex));
                        FT_CONTACT_DIAG(Diag->SetStringField(FullSkin?TEXT("firstSkinBones"):ActualBody?TEXT("firstActualBodyBones"):TEXT("firstContactBones"),FString::Printf(TEXT("%s,%s,%s"),
                            *C.Mesh->GetRefSkeleton().GetBoneName(FaceVertices[T.X].Bone).ToString(),
                            *C.Mesh->GetRefSkeleton().GetBoneName(FaceVertices[T.Y].Bone).ToString(),
                            *C.Mesh->GetRefSkeleton().GetBoneName(FaceVertices[T.Z].Bone).ToString())));
                        if(!FullSkin&&FaceVertices[T.X].Bone==FaceVertices[T.Y].Bone&&FaceVertices[T.X].Bone==FaceVertices[T.Z].Bone)
                        {
                            const int32 Bone=FaceVertices[T.X].Bone;
                            const FVector BonePoint=Pose[Bone].InverseTransformPosition(FlipRawY(Actor.InverseTransformPosition(P)));
                            FT_CONTACT_DIAG(Diag->SetStringField(ActualBody?TEXT("firstActualBodyBoneLocalPoint"):TEXT("firstContactBoneLocalPoint"),BonePoint.ToString()));
                            if(!ActualBody&&C.ContactKind[TriangleIndex]==EContactKind::WheelEnvelope)
                            {FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactWitnessWheelRadiusCm"),FVector2D(BonePoint.X,BonePoint.Z).Size()));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("actualMaximumWheelRadiusCm"),C.WheelMaximumRadiusCm[Bone]));}
                        }
                    }
                    if(!FullSkin)
                    {
                        const auto Kind=ActualBody?EContactKind::BodyFace:C.ContactKind[TriangleIndex];
                        const bool SupportFace=Kind==EContactKind::SoleFace||(C.Wheeled&&Kind==EContactKind::WheelEnvelope);
                        if(!SupportFace)
                        {
                            if(Kind==EContactKind::BodyFace||NonSoleReason.IsEmpty())NonSoleReason=Kind==EContactKind::BodyFace?
                                TEXT("actual_body_face_penetration"):Kind==EContactKind::BodyEnvelope?
                                TEXT("body_contact_envelope_penetration"):TEXT("wheel_contact_envelope_penetration");
                        }
                        else if(Witnesses&&Vertices[T.X].Bone==Vertices[T.Y].Bone&&Vertices[T.X].Bone==Vertices[T.Z].Bone)
                        {
                            const int32 Bone=Vertices[T.X].Bone;const int32 Side=C.GearSide[Bone];
                            const FVector Local=Actor.InverseTransformPosition(P);
                            const int32 Strip=(Side<0?0:2)+(Local.Y-C.GearCentreY[Bone]<0?0:1);
                            if(Side&&(!DeepestByStrip[Strip].Valid||Gap<DeepestByStrip[Strip].GapCm))
                            {
                                auto& Contact=DeepestByStrip[Strip];Contact.NativeActorLocalCm=Local;Contact.Bone=Bone;
                                Contact.Strip=Strip;Contact.Kind=Kind;Contact.GapCm=Gap;Contact.Valid=true;
                            }
                        }
                    }
                }
            }
        }
        return true;
    };
    if(UseBounds)
    {
        for(int32 BoneIndex=0;BoneIndex<C.SingleBoneTriangles.Num();++BoneIndex)
        {
            if(BoneAudit&&BoneIndex!=AuditBone)continue;
            const auto& Faces=C.SingleBoneTriangles[BoneIndex];if(Faces.IsEmpty())continue;
            const FBox& Bound=WorldBones[BoneIndex];if(!InGrid(Bound))return Reject(TEXT("actual_bone_bound_grid"));
            const double BoneRoof=Roof(Bound);
            // Every rigid-bound vertex is in this affine-transformed box. A
            // face's convex hull is too. Native height on every covered grid
            // triangle is<=the maximum of its grid vertices. Thus MinZ>roof
            // proves the ENTIRE face lies above the entire ground envelope.
            if(Bound.Min.Z>BoneRoof+1.e-4){Skipped+=Faces.Num();++SkippedBones;continue;}
            for(int32 TriangleIndex:Faces)if(!Face(TriangleIndex,BoneRoof)){Flush();return false;}
        }
        // A cross-bone face may span a ridge between its endpoint bones; it
        // is never culled by independent per-bone decisions. All three exact
        // posed vertices and the common complete ground roof are considered.
        for(int32 TriangleIndex:C.CrossBoneTriangles)
        {if(BoneAudit)continue;++CrossProcessed;if(!Face(TriangleIndex,GlobalRoof)){Flush();return false;}}
    }
    else for(int32 I=0;I<Triangles.Num();++I)
    {
        if(BoneAudit&&Vertices[Triangles[I].X].Bone!=AuditBone)continue;
        const bool BodyProbe=!FullSkin&&C.ContactKind[I]==EContactKind::BodyEnvelope;
        if(BodyProbe)
        {
            const auto& T=Triangles[I];const int32 Bone=Vertices[T.X].Bone;
            if(!BodyMinimum.IsValidIndex(Bone)||Vertices[T.Y].Bone!=Bone||Vertices[T.Z].Bone!=Bone)
                return Reject(TEXT("body_envelope_rigid_binding"));
        }
        if(!Face(I,GlobalRoof,false,BodyProbe)){Flush();return false;}
    }
    if(!FullSkin)for(int32 Bone=0;Bone<BodyMinimum.Num();++Bone)
    {
        const double BoxGap=BodyMinimum[Bone];if(BoxGap==DBL_MAX)continue;
        if(BoxGap>=-ProductionContactToleranceCm)
        {Minimum=FMath::Min(Minimum,BoxGap);continue;}
        // Resolve every intersecting body bone, not only the deepest box.
        // A passing envelope is already a conservative proof; a failing one
        // requires all original rigid faces, including vertical/front faces.
        const auto& Faces=C.SingleBoneTriangles[Bone];if(Faces.IsEmpty())return Reject(TEXT("actual_body_faces_missing"));
        for(int32 I:C.CrossBoneTriangles)
        {
            const auto& T=C.Triangles[I];
            if(C.Vertices[T.X].Bone==Bone||C.Vertices[T.Y].Bone==Bone||C.Vertices[T.Z].Bone==Bone)
                return Reject(TEXT("actual_body_cross_binding"));
        }
        if(BodyWorld.IsEmpty()){BodyWorld.SetNumUninitialized(C.Vertices.Num());BodyReady.Init(0,C.Vertices.Num());}
        const FBox& Bound=WorldBones[Bone];if(!InGrid(Bound))return Reject(TEXT("actual_body_bound_grid"));
        const double BoneRoof=Roof(Bound);++BodyBones;
        const auto& Tree=C.BodyPatchTrees[Bone];
        if(Tree.Faces.Num()!=Faces.Num()||Tree.Nodes.IsEmpty())return Reject(TEXT("actual_body_patch_cache"));
        TArray<int32,TInlineAllocator<64>> Pending;Pending.Add(0);
        while(!Pending.IsEmpty())
        {
            if(++BodyPatchVisits>MaxBodyPatchVisits)return Reject(TEXT("actual_body_patch_visit_budget"));
            const int32 NodeIndex=Pending.Pop(EAllowShrinking::No);
            if(!Tree.Nodes.IsValidIndex(NodeIndex))return Reject(TEXT("actual_body_patch_index"));
            const auto& Node=Tree.Nodes[NodeIndex];FBox PatchWorld(ForceInit);
            for(int32 X=0;X<2;++X)for(int32 Y=0;Y<2;++Y)for(int32 Z=0;Z<2;++Z)
            {
                const FVector Corner(X?Node.Bounds.Max.X:Node.Bounds.Min.X,Y?Node.Bounds.Max.Y:Node.Bounds.Min.Y,Z?Node.Bounds.Max.Z:Node.Bounds.Min.Z);
                PatchWorld+=Actor.TransformPosition(FlipRawY(Pose[Bone].TransformPosition(Corner)));
            }
            PatchWorld=PatchWorld.ExpandBy(1.e-4);
            if(!InGrid(PatchWorld))return Reject(TEXT("actual_body_patch_grid"));
            double PatchRoof=BoneRoof;
            if(PatchWorld.Min.Z<=PatchRoof+1.e-4)
            {
                const int32 X0=FMath::Clamp(FMath::FloorToInt((PatchWorld.Min.X-G.minX)/DX),0,G.columns-2),X1=FMath::Clamp(FMath::FloorToInt((PatchWorld.Max.X-G.minX)/DX),0,G.columns-2);
                const int32 Y0=FMath::Clamp(FMath::FloorToInt((PatchWorld.Min.Y-G.minY)/DY),0,G.rows-2),Y1=FMath::Clamp(FMath::FloorToInt((PatchWorld.Max.Y-G.minY)/DY),0,G.rows-2);
                if(!LocalRoof(X0,X1,Y0,Y1,PatchRoof)){Flush();return false;}
            }
            // This box encloses every original face in this rigid patch under
            // the same compressed pose. The native cell roof is conservative;
            // only a complete separation proof can skip original vertices.
            if(PatchWorld.Min.Z>PatchRoof+1.e-4)
            {Skipped+=Node.Count;BodyPatchSkippedFaces+=Node.Count;continue;}
            if(Node.Left!=INDEX_NONE||Node.Right!=INDEX_NONE)
            {
                if(Node.Left==INDEX_NONE||Node.Right==INDEX_NONE)return Reject(TEXT("actual_body_patch_children"));
                Pending.Add(Node.Right);Pending.Add(Node.Left);continue;
            }
            if(Node.First<0||Node.Count<1||Node.First>Tree.Faces.Num()-Node.Count)return Reject(TEXT("actual_body_patch_span"));
            for(int32 J=Node.First;J<Node.First+Node.Count;++J)
                if(!Face(Tree.Faces[J],PatchRoof,true,false)){Flush();return false;}
        }
    }
    ScanComplete=true;
    if(Witnesses)for(const auto& Contact:DeepestByStrip)if(Contact.Valid)Witnesses->Add(Contact);
    if(ContactViolation)return Reject(FullSkin?TEXT("actual_gear_or_body_face_penetration"):
        !NonSoleReason.IsEmpty()?*NonSoleReason:TEXT("sole_contact_face_penetration"));
    Flush();return true;
}

bool FullHull(AFrontlineTank* V,UStaticMeshComponent* Ground,const FTransform& From,const FTransform& To,TSharedPtr<FJsonObject> Diag,FString& Reason)
{
    const auto* D=Definition(V);const FVector Centre=(D->Minimum+D->Maximum)*.5;
    // This replaces the old elevated 80cm half-height occupancy proxy.
    // Full registered model bounds remain intact, plus the old lateral skin.
    const FVector Half=(D->Maximum-D->Minimum)*.5+FVector(12,12,0);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV82FTFullHull),false);Q.AddIgnoredActor(V);Q.AddIgnoredComponent(Ground);
    // The two disconnected offstage strips have one broad-phase AABB spanning
    // the battlefield. Admit only the existing finite-face/native-validated
    // component for this exact scene/revision; every independent solid stays.
    if(auto* State=States.Find(V))
    {
        auto* Apron=State->VerifiedApron.Get();
        if(!Apron||!Apron->IsRegistered()||Apron->GetWorld()!=V->GetWorld())
        {
            Apron=FRRUGermanStaticContactV82::ValidatedSceneApronV87(Runtime(V)->Battle.Get(),Ground,State->VerifiedApronEvidence);
            State->VerifiedApron=Apron;
        }
        if(Apron)Q.AddIgnoredComponent(Apron);
        FT_CONTACT_DIAG(if(State->VerifiedApronEvidence)Diag->SetObjectField(TEXT("verifiedSceneApronTerrainV87"),State->VerifiedApronEvidence));
        FT_CONTACT_DIAG(if(Apron)Diag->SetStringField(TEXT("ignoredVerifiedApronComponent"),Apron->GetPathName()));
    }
    // The current infantry capsule ignores WorldStatic (Visibility alone
    // blocks). This complete-volume query proves WorldStatic scene/vehicle
    // clearance only; it does not claim Pawn collision or invent crushing.
    FT_CONTACT_DIAG(Diag->SetStringField(TEXT("hullQueryChannel"),TEXT("WorldStatic")));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("pawnCollisionValidated"),false));
    auto Record=[&](const UPrimitiveComponent* C){if(C)FT_CONTACT_DIAG(Diag->SetStringField(TEXT("firstHullComponent"),C->GetPathName()));};
    for(const FTransform* Endpoint:{&From,&To})
    {
        TArray<FOverlapResult> Overlaps;V->GetWorld()->OverlapMultiByChannel(Overlaps,Endpoint->TransformPosition(Centre),Endpoint->GetRotation(),ECC_WorldStatic,FCollisionShape::MakeBox(Half),Q);
        for(const auto& Hit:Overlaps)if(Hit.bBlockingHit){Record(Hit.GetComponent());Reason=TEXT("complete_hull_endpoint_overlap");return false;}
    }
    FVector A,B,Extent;FQuat Rotation;HullSweepEnvelope(From,To,Centre,Half,A,B,Rotation,Extent);FHitResult Hit;
    if(V->GetWorld()->SweepSingleByChannel(Hit,A,B,Rotation,ECC_WorldStatic,FCollisionShape::MakeBox(Extent),Q))
    {Record(Hit.GetComponent());FT_CONTACT_DIAG(Diag->SetStringField(TEXT("firstHullImpact"),Hit.ImpactPoint.ToString()));FT_CONTACT_DIAG(Diag->SetStringField(TEXT("firstHullNormal"),Hit.ImpactNormal.ToString()));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("firstHullStartPenetrating"),Hit.bStartPenetrating));Reason=TEXT("complete_hull_swept_block");return false;}
    FT_CONTACT_DIAG(Diag->SetStringField(TEXT("completeHullHalfExtent"),Half.ToString()));FT_CONTACT_DIAG(Diag->SetStringField(TEXT("ignoredNativeGround"),Ground->GetPathName()));return true;
}

const wfexp::scene::Deck* RegisteredDeckAt(const wfexp::scene::SceneDefinition& G,const FVector& P)
{
    for(const auto& Deck:G.decks)
    {
        const FVector2D A(Deck.from.x,Deck.from.y),B(Deck.to.x,Deck.to.y),XY(P.X,P.Y),Edge=B-A;
        const double LengthSquared=Edge.SizeSquared();
        if(!FMath::IsFinite(LengthSquared)||LengthSquared<=1.e-9||!FMath::IsFinite(Deck.halfWidth)||Deck.halfWidth<=0)continue;
        const double T=FMath::Clamp(FVector2D::DotProduct(XY-A,Edge)/LengthSquared,0.,1.);
        if((XY-(A+T*Edge)).SizeSquared()<=Deck.halfWidth*Deck.halfWidth)return &Deck;
    }
    return nullptr;
}

// Return handled=true only for the exact selected model inside the complete
// native footprint. Rejection spends no phase, distance, actor pose or clock.
bool PlanStep(AFrontlineTank* V,const FTransform& Previous,const FVector& Desired,float Heading,float Phase,FTransform& Accepted,bool SelectAuditFrame)
{
    FState& S=States.FindOrAdd(V);[[maybe_unused]] const double Begin=FPlatformTime::Seconds();auto Diag=ContactDiagnostic();
    FT_CONTACT_DIAG(Diag->SetStringField(TEXT("revision"),TEXT("v82.vehicle.ground.pilot.2")));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("runtimeAccepted"),false));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("shippingAccepted"),false));
    FT_CONTACT_DIAG(Diag->SetStringField(TEXT("desired"),Desired.ToString()));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("hostElapsed"),Runtime(V)->Battle->Elapsed));
    FString Reason;const wfexp::scene::SceneDefinition* G=nullptr;
    auto Fail=[&](const FString& Why)
    {
        if(Why==TEXT("support_search_budget"))S.LastSupportSearchRejectAt=Runtime(V)->Battle->Elapsed;
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("passed"),false));FT_CONTACT_DIAG(Diag->SetStringField(TEXT("reason"),Why));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("totalMs"),(FPlatformTime::Seconds()-Begin)*1000.));
        FT_CONTACT_DIAG(S.Last=Diag;if(!S.FirstRejected)S.FirstRejected=Diag);return false;
    };
    if(!Context(V,S,G,Reason))return Fail(Reason);
    FT_CONTACT_DIAG(Diag->SetStringField(TEXT("groundBindingMode"),TEXT("exact_current_staged_scene_identity_and_collision_tags")));
    FT_CONTACT_DIAG(Diag->SetStringField(TEXT("groundComponent"),S.Ground->GetPathName()));FT_CONTACT_DIAG(Diag->SetStringField(TEXT("groundMesh"),S.Ground->GetStaticMesh()->GetPathName()));
    auto* R=Runtime(V);auto* Mesh=R->Driver.IsValid()?R->Driver->GetSkeletalMeshAsset():nullptr;
    auto* Drive=LoadObject<UAnimSequence>(nullptr,*URI(*Definition(V),TEXT(""),TEXT("Drive144cm")));
    if(!Mesh||Mesh->GetPathName()!=URI(*Definition(V),TEXT("SK"))
        ||!Drive||Drive->GetPathName()!=URI(*Definition(V),TEXT(""),TEXT("Drive144cm")))return Fail(TEXT("actual_vehicle_mesh_drive_namespace"));
    FMeshCache* C=nullptr;if(!Cache(*Definition(V),Mesh,R->Driver.Get(),C,Reason,Diag))return Fail(Reason);
    TArray<FTransform> Pose;if(!CompressedPose(Mesh,Drive,Phase,Pose,Reason))return Fail(Reason);
    FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("prospectivePhase"),Phase));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("actualCompressedPoseRead"),true));
    TArray<FMarker> Markers;Markers.Reserve(C->Markers.Num());
    for(int32 I=0;I<C->Markers.Num();++I)
    {
        const auto& M=C->Markers[I];const auto& Vertex=C->ContactVertices[M.Vertex];
        const FVector P=FlipRawY(Pose[Vertex.Bone].TransformPosition(Vertex.BoneLocal));
        const int32 Strip=(M.Side<0?0:2)+(P.Y-C->GearCentreY[Vertex.Bone]<0?0:1);
        Markers.Add({P,I,Strip,M.Element,P.Z<C->MinimumSupportZ+12,M.Side});
    }
    const auto* D=Definition(V);TArray<FNinePoint> Nine;
    const FTransform Flat(FRotator(0,Heading,0),Desired);
    for(double X:{D->Minimum.X,(D->Minimum.X+D->Maximum.X)*.5,D->Maximum.X})for(double Y:{D->Minimum.Y,(D->Minimum.Y+D->Maximum.Y)*.5,D->Maximum.Y})
    {
        const FVector P=Flat.TransformPosition({X,Y,0});double Z=0;
        const auto Foot=wfexp::scene::FootingAt(*G,P.X,P.Y);
        if(!Foot.valid||!Foot.walkable||!wfexp::scene::HeightAt(*G,P.X,P.Y,Z))return Fail(TEXT("native_floor_or_footing"));
        Nine.Add({{P.X,P.Y},Z});
    }
    FPlane Plane;if(!FitNine(Nine,{Desired.X,Desired.Y},Plane))return Fail(TEXT("nine_fit"));
    FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("rawNineReliefCm"),Plane.RawReliefCm));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("nineResidualRangeCm"),Plane.ResidualRangeCm));
    FCandidate Fit;FStats Failure;double ContactSolveMs=0;int32 ContactNewton=0,ContactQueries=0,ContactPivots=0;
    auto Floor=[&](const FVector2D& XY,double& Z){return wfexp::scene::HeightAt(*G,XY.X,XY.Y,Z);};
    auto SolverStats=[&](const FStats& Stats)
    {
        ContactSolveMs+=Stats.SolveMs;ContactNewton+=Stats.NewtonCandidates;ContactQueries+=Stats.FloorQueries;ContactPivots+=Stats.ActivePivots;
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactSolveTotalMs"),ContactSolveMs));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactSolveTotalCandidates"),ContactNewton));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactSolveTotalFloorQueries"),ContactQueries));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactSolveActivePivots"),ContactPivots));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("solveMs"),Stats.SolveMs));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("floorQueries"),Stats.FloorQueries));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("candidateTriples"),Stats.Triples));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("newtonCandidates"),Stats.NewtonCandidates));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("solverIterations"),Stats.Iterations));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("fullMarkerValidations"),Stats.FullValidations));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("convergedCandidates"),Stats.ConvergedCandidates));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("markerRejectedCandidates"),Stats.MarkerRejectedCandidates));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("supportActivePivots"),Stats.ActivePivots));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("worstRejectedMarker"),Stats.WorstRejectedMarker));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("supportActiveBatchAttempts"),Stats.ActiveBatchAttempts));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("supportActiveBatchTriples"),Stats.ActiveBatchTriples));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("supportActiveBatchNewtonCandidates"),Stats.ActiveBatchNewtonCandidates));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("supportActiveBatchMarkerRejections"),Stats.ActiveBatchMarkerRejections));
        if(Stats.WorstRejectedMarker!=INDEX_NONE)
        {
            FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("bestRejectedMarkerGapCm"),Stats.BestRejectedGapCm));
            FT_CONTACT_DIAG(Diag->SetStringField(TEXT("bestRejectedSupportMarkers"),Stats.BestRejectedSupport.ToString()));
            FT_CONTACT_DIAG(Diag->SetStringField(TEXT("bestRejectedCandidateLocation"),FVector(Desired.X,Desired.Y,Stats.BestRejectedParams.Z).ToString()));
            FT_CONTACT_DIAG(Diag->SetStringField(TEXT("bestRejectedCandidateRotation"),Orientation(Stats.BestRejectedParams.X,Stats.BestRejectedParams.Y,Heading).Rotator().ToString()));
            FT_CONTACT_DIAG(Diag->SetStringField(TEXT("worstRejectedMarkerActorLocal"),Stats.WorstRejectedLocal.ToString()));
            FT_CONTACT_DIAG(Diag->SetStringField(TEXT("worstRejectedMarkerWorld"),Stats.WorstRejectedWorld.ToString()));
            FT_CONTACT_DIAG(Diag->SetStringField(TEXT("worstRejectedMarkerBone"),Stats.WorstRejectedElement>=0&&Stats.WorstRejectedElement<Mesh->GetRefSkeleton().GetNum()?Mesh->GetRefSkeleton().GetBoneName(Stats.WorstRejectedElement).ToString():FString()));
        }
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("supportSearchBudgetExhausted"),Stats.BudgetExhausted));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("minimumMarkerGapCm"),Stats.MinimumMarkerGapCm));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("maximumMarkerGapCm"),Stats.MaximumMarkerGapCm));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("markerContactResolutionCm"),ProductionContactToleranceCm));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("diagnosticContactResolutionCm"),DiagnosticContactResolutionCm));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("kinematicReferenceRejectedCandidates"),Stats.ReferenceRejectedCandidates));
    };
    // Explicit 1mm production face/marker budget; selected actual support
    // contacts still solve to 1e-6cm and must pass the original 1e-3cm rays.
    // No artificial root clearance or phase advance is introduced.
    FWarmSupport ContactWarm;const FWarmSupport* SolveWarm=S.HasWarm?&S.Warm:nullptr;bool ContactPassed=false;
    double RefitFloor=0;if(!Floor({Desired.X,Desired.Y},RefitFloor))return Fail(TEXT("native_center_floor"));
#if WITH_EDITOR
    TArray<TSharedPtr<FJsonValue>> ContactWitnesses;
#endif
    FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactRefits"),0));
    for(int32 ContactPass=0;ContactPass<MaxContactPasses;++ContactPass)
    {
        if(!Find(Markers,Nine,{Desired.X,Desired.Y},Heading,D->Wheeled?50:125,ProductionContactToleranceCm,SolveWarm,Floor,Fit,Failure))
        {SolverStats(Failure);FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCollisionReported"),false));return Fail(Failure.FirstBlock);}
        SolverStats(Fit.Stats);
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("solveMs"),Fit.Stats.SolveMs));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("floorQueries"),Fit.Stats.FloorQueries));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("candidateTriples"),Fit.Stats.Triples));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("newtonCandidates"),Fit.Stats.NewtonCandidates));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("slope"),FVector2D(Fit.Params.X,Fit.Params.Y).Size()));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("minimumMarkerGapCm"),Fit.Stats.MinimumMarkerGapCm));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("maximumMarkerGapCm"),Fit.Stats.MaximumMarkerGapCm));
        FT_CONTACT_DIAG(Diag->SetStringField(TEXT("acceptedCandidateRotation"),Fit.Pose.Rotator().ToString()));
        FT_CONTACT_DIAG(Diag->SetStringField(TEXT("acceptedCandidateLocation"),Fit.Pose.GetLocation().ToString()));
        FT_CONTACT_DIAG(Diag->SetStringField(TEXT("supportModel"),TEXT("kinematic_actor_root_reference")));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCenterOfMassValidated"),false));
        FT_CONTACT_DIAG(Diag->SetStringField(TEXT("kinematicReferenceWeights"),Fit.KinematicReferenceWeights.ToString()));
#if WITH_EDITOR
        TArray<TSharedPtr<FJsonValue>> Supports;
#endif
        FCollisionQueryParams Rays(SCENE_QUERY_STAT(FRRUV82FTSupport),true);
        for(int32 I=0;I<3;++I)
        {
            const FVector P=Fit.WorldSupport[I];const auto Foot=wfexp::scene::FootingAt(*G,P.X,P.Y);double Z=0;FHitResult Hit;
            if(!Foot.valid||!Foot.walkable||!Floor({P.X,P.Y},Z)
                ||!S.Ground->LineTraceComponent(Hit,P+FVector(0,0,1000),P-FVector(0,0,1000),Rays)
                ||FMath::Abs(Hit.ImpactPoint.Z-Z)>.5||FMath::Abs(P.Z-Z)>1.e-3)return Fail(TEXT("actual_support_ray_or_contact"));
#if WITH_EDITOR
            auto Item=MakeShared<FJsonObject>();
#endif
            FT_CONTACT_DIAG(Item->SetNumberField(TEXT("marker"),Fit.Support[I]));FT_CONTACT_DIAG(Item->SetNumberField(TEXT("side"),Markers[Fit.Support[I]].GearSide));
            if(const auto* Deck=RegisteredDeckAt(*G,P))
            {
                // Only a deck cap actually baked into native + registered ground
                // can support this pilot. A separate shelf never gets ignored by
                // the complete hull query, and deck metadata alone grants nothing.
                if(!FMath::IsFinite(Deck->topZ)||FMath::Abs(Z-Deck->topZ)>.5)return Fail(TEXT("registered_deck_not_baked_at_support"));
                FT_CONTACT_DIAG(Item->SetStringField(TEXT("registeredDeckEdge"),Deck->edgeId?wfexpue::Wide(Deck->edgeId):FString()));
                FT_CONTACT_DIAG(Item->SetStringField(TEXT("supportComponent"),S.Ground->GetPathName()));
            }
            FT_CONTACT_DIAG(Item->SetStringField(TEXT("world"),P.ToString()));FT_CONTACT_DIAG(Item->SetNumberField(TEXT("nativeCm"),Z));FT_CONTACT_DIAG(Item->SetNumberField(TEXT("actualHitCm"),Hit.ImpactPoint.Z));FT_CONTACT_DIAG(Supports.Add(MakeShared<FJsonValueObject>(Item)));
        }
        FT_CONTACT_DIAG(Diag->SetArrayField(TEXT("supports"),Supports));
        FT_CONTACT_DIAG(Diag->SetStringField(TEXT("deckSupportMode"),TEXT("actual_baked_registered_ground_only")));
        TArray<FContactWitness> Witnesses;FSurfaceScan ContactScan;
        if(!SkinGround(*C,Pose,Fit.Pose,*G,Diag,Reason,false,INDEX_NONE,&Witnesses,&ContactScan))
        {
            [[maybe_unused]] const bool Envelope=Reason==TEXT("body_contact_envelope_penetration")||Reason==TEXT("wheel_contact_envelope_penetration");
            const bool Sole=Reason==TEXT("sole_contact_face_penetration");
            FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCollisionReported"),false));FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("conservativeContactRejected"),Envelope));
            FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("soleContactConstraintRejected"),Sole));
            // One first-rejection diagnostic uses this exact compressed candidate
            // and only the implicated rigid bone's original faces. It never grants
            // movement and never invokes the full render skin on every substep.
#if WITH_EDITOR
            if((Envelope||Sole)&&!S.FirstContactSkinAudit&&Diag->HasField(TEXT("firstContactTriangle")))
            {
                const int32 TriangleIndex=int32(Diag->GetNumberField(TEXT("firstContactTriangle")));const auto& T=C->ContactTriangles[TriangleIndex];
                const int32 Bone=C->ContactVertices[T.X].Bone;
                if(Bone==C->ContactVertices[T.Y].Bone&&Bone==C->ContactVertices[T.Z].Bone)
                {
                    auto Audit=MakeShared<FJsonObject>();FString AuditReason;
                    const bool Passed=SkinGround(*C,Pose,Fit.Pose,*G,Audit,AuditReason,true,Bone);
                    FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("passed"),Passed));FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("diagnosticOnly"),true));FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("actualCompressedPoseRead"),true));
                    FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("preContactRefitCandidate"),true));FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("candidateWasAccepted"),false));
                    FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("runtimeAccepted"),false));FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("shippingAccepted"),false));
                    FT_CONTACT_DIAG(Audit->SetStringField(TEXT("reason"),Passed?TEXT("same_frame_rigid_bone_skin_passed"):AuditReason));FT_CONTACT_DIAG(Audit->SetStringField(TEXT("boundedContactReason"),Reason));
                    FT_CONTACT_DIAG(Audit->SetStringField(TEXT("boneLocation"),Pose[Bone].GetTranslation().ToString()));FT_CONTACT_DIAG(Audit->SetStringField(TEXT("boneRotation"),Pose[Bone].Rotator().ToString()));
                    FT_CONTACT_DIAG(Audit->SetStringField(TEXT("boneScale"),Pose[Bone].GetScale3D().ToString()));FT_CONTACT_DIAG(Audit->SetStringField(TEXT("candidateLocation"),Fit.Pose.GetLocation().ToString()));
                    FT_CONTACT_DIAG(Audit->SetStringField(TEXT("candidateRotation"),Fit.Pose.Rotator().ToString()));FT_CONTACT_DIAG(Audit->SetNumberField(TEXT("hostElapsed"),R->Battle->Elapsed));
                    FT_CONTACT_DIAG(Audit->SetNumberField(TEXT("prospectivePhase"),Phase));S.FirstContactSkinAudit=Audit;
                }
            }
#endif
            if(Sole&&!Witnesses.IsEmpty())
            {
                // Batch at most one deepest actual finite-face/terrain point
                // per support strip from the completed scan. All stay local to
                // this exact compressed phase; the next pass checks every face.
                if(ContactPass+1>=MaxContactPasses)return Fail(TEXT("contact_active_set_budget"));
                int32 Added=0;
                for(const auto& Witness:Witnesses)
                {
                    if(!Witness.Valid||(Witness.Kind!=EContactKind::SoleFace&&!(C->Wheeled&&Witness.Kind==EContactKind::WheelEnvelope)))continue;
                    bool Duplicate=false;for(const auto& M:Markers)if(M.NativeActorLocalCm.Equals(Witness.NativeActorLocalCm,1.e-7)){Duplicate=true;break;}
                    if(Duplicate)continue;
                    const FString Label=Mesh->GetRefSkeleton().GetBoneName(Witness.Bone).ToString();
                    const int32 Side=C->GearSide[Witness.Bone];
                    if(!Side||Witness.Strip<0||Witness.Strip>=4||Markers.Num()>=MaxMarkers)return Fail(TEXT("contact_witness_contract"));
                    const FVector P=Witness.NativeActorLocalCm;const int32 Index=Markers.Num();
                    Markers.Add({P,Index,Witness.Strip,Witness.Bone,P.Z<C->MinimumSupportZ+12,Side});++Added;
                    FT_CONTACT_DIAG(auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("bone"),Label);Item->SetStringField(TEXT("actorLocalPoint"),P.ToString());Item->SetNumberField(TEXT("gapCm"),Witness.GapCm);Item->SetNumberField(TEXT("marker"),Index);Item->SetNumberField(TEXT("strip"),Witness.Strip);Item->SetNumberField(TEXT("contactPass"),ContactPass);ContactWitnesses.Add(MakeShared<FJsonValueObject>(Item)));
                }
                if(!Added)return Fail(TEXT("contact_active_set_no_progress"));
                FT_CONTACT_DIAG(Diag->SetArrayField(TEXT("activeContactWitnesses"),ContactWitnesses));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactRefits"),ContactPass+1));
                FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("contactWitnessBatchSize"),Added));
                for(int32 J=0;J<3;++J)ContactWarm.Indices[J]=Fit.Support[J];ContactWarm.A=Fit.Params.X;ContactWarm.B=Fit.Params.Y;
                ContactWarm.RootAboveCenterFloorCm=Fit.Params.Z-RefitFloor;SolveWarm=&ContactWarm;continue;
            }
            return Fail(Reason);
        }
        if(!ContactScan.Complete||!ContactScan.WithinTolerance||ContactScan.MinimumGapCm< -ProductionContactToleranceCm)
            return Fail(TEXT("final_contact_scan_not_within_production_tolerance"));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("finalContactPass"),ContactPass));FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("finalContactPhase"),Phase));
        FT_CONTACT_DIAG(Diag->SetStringField(TEXT("finalContactCandidateLocation"),Fit.Pose.GetLocation().ToString()));
        ContactPassed=true;break;
    }
    if(!ContactPassed)return Fail(TEXT("contact_active_set_budget"));
    FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("boundedContactPassed"),true));
    if(!FullHull(V,S.Ground.Get(),Previous,Fit.Pose,Diag,Reason))
    {FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCollisionReported"),true));return Fail(Reason);}
    FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("physicalCollisionReported"),false));
    // Full render-skin work is diagnostic, once on the first selected final
    // substep. An explicit existing review invocation can sample every two
    // actual host seconds; it never becomes an additional movement clock/gate.
#if WITH_EDITOR
    const double HostElapsed=R->Battle->Elapsed;
    FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("finalSkinDiagnosticSampled"),false));
    if(SelectAuditFrame&&(!S.LastSkinAudit||(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUFTFullSkinAudit"))&&HostElapsed-S.LastSkinAuditHostAt>=2.)))
    {
        auto Audit=MakeShared<FJsonObject>();FString AuditReason;
        const bool Passed=SkinGround(*C,Pose,Fit.Pose,*G,Audit,AuditReason,true);
        FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("passed"),Passed));FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("diagnosticOnly"),true));
        FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("finalCandidatePose"),true));
        FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("runtimeAccepted"),false));FT_CONTACT_DIAG(Audit->SetBoolField(TEXT("shippingAccepted"),false));
        FT_CONTACT_DIAG(Audit->SetStringField(TEXT("reason"),Passed?TEXT("selected_skin_audit_passed"):AuditReason));
        FT_CONTACT_DIAG(Audit->SetStringField(TEXT("candidateLocation"),Fit.Pose.GetLocation().ToString()));FT_CONTACT_DIAG(Audit->SetNumberField(TEXT("hostElapsed"),HostElapsed));
        FT_CONTACT_DIAG(Audit->SetNumberField(TEXT("prospectivePhase"),Phase));S.LastSkinAudit=Audit;S.LastSkinAuditHostAt=HostElapsed;
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("finalSkinDiagnosticSampled"),true));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("finalSkinScanComplete"),Audit->GetBoolField(TEXT("skinScanComplete"))));
        FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("finalSkinWithinProductionTolerance"),Audit->GetBoolField(TEXT("skinWithinProductionTolerance"))));
        FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("finalSkinMinimumGapCm"),Audit->GetNumberField(TEXT("minimumSkinGapCm"))));
    }
#endif
    // Scene/encounter is rechecked immediately before publishing the result.
    if(!Context(V,S,G,Reason))return Fail(Reason);
    double CentreFloor=0;if(!Floor({Desired.X,Desired.Y},CentreFloor))return Fail(TEXT("native_center_floor"));
    for(int32 I=0;I<3;++I)S.Warm.Indices[I]=Fit.Support[I];S.Warm.A=Fit.Params.X;S.Warm.B=Fit.Params.Y;
    S.Warm.RootAboveCenterFloorCm=Fit.Params.Z-CentreFloor;
    S.HasWarm=Fit.Support[0]<C->Markers.Num()&&Fit.Support[1]<C->Markers.Num()&&Fit.Support[2]<C->Markers.Num();
    FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("warmSupportsReusable"),S.HasWarm));
    FT_CONTACT_DIAG(Diag->SetBoolField(TEXT("passed"),true));FT_CONTACT_DIAG(Diag->SetStringField(TEXT("reason"),TEXT("candidate_pose_committed_by_tick_only")));
    S.LastSupportSearchRejectAt=-1;
    FT_CONTACT_DIAG(Diag->SetNumberField(TEXT("totalMs"),(FPlatformTime::Seconds()-Begin)*1000.));FT_CONTACT_DIAG(S.Last=Diag);Accepted=Fit.Pose;return true;
}

void RecordRejected(AFrontlineTank* V,const FVector& Goal,const TCHAR* Why);
bool Plan(AFrontlineTank* V,const FVector& Desired,float Heading,float ProposedTravel,FTransform& Accepted)
{
    // Existing DT<=.1 means<=20cm tracked and<=30cm for Austin. Split the physical
    // proposal into<=7cm checked poses without changing elapsed or speed.
    // No intermediate actor/driver mutation occurs; all substeps must pass.
    [[maybe_unused]] const double Begin=FPlatformTime::Seconds();const FVector Start=V->GetActorLocation();
    const auto* R=Runtime(V);const int32 Count=FMath::Max(1,FMath::CeilToInt(ProposedTravel/7.f));
    if(!R||!Definition(V)||Count>(Definition(V)->Wheeled?5:4)||ProposedTravel<0||!FMath::IsFinite(ProposedTravel))return false;
    auto& Binding=States.FindOrAdd(V);const wfexp::scene::SceneDefinition* Grid=nullptr;FString ContextReason;
    if(!Context(V,Binding,Grid,ContextReason)){RecordRejected(V,Desired,*ContextReason);return false;}
    const FWarmSupport PreviousWarm=Binding.Warm;const bool HadWarm=Binding.HasWarm;Binding.HasProposedWarm=false;
    FTransform Previous=V->GetActorTransform();
    for(int32 I=1;I<=Count;++I)
    {
        const float Fraction=float(I)/Count;const FVector PointOnLeg=FMath::Lerp(Start,Desired,Fraction);
        const float Phase=FMath::Fmod(R->Phase+ProposedTravel*Fraction/60.f+2.4f,2.4f);
        FTransform Pose;
        if(!PlanStep(V,Previous,PointOnLeg,Heading,Phase,Pose,I==Count))
        {Binding.Warm=PreviousWarm;Binding.HasWarm=HadWarm;FT_CONTACT_DIAG(if(auto* S=States.Find(V);S&&S->Last){S->Last->SetNumberField(TEXT("failedSubstep"),I);S->Last->SetNumberField(TEXT("substeps"),Count);S->Last->SetNumberField(TEXT("totalMovementPlanMs"),(FPlatformTime::Seconds()-Begin)*1000.);});return false;}
        Previous=Pose;
    }
    FT_CONTACT_DIAG(if(auto* S=States.Find(V);S&&S->Last){S->Last->SetNumberField(TEXT("substeps"),Count);S->Last->SetNumberField(TEXT("totalMovementPlanMs"),(FPlatformTime::Seconds()-Begin)*1000.);});
    Binding.ProposedWarm=Binding.Warm;Binding.ProposedHasWarm=Binding.HasWarm;Binding.HasProposedWarm=true;
    Binding.Warm=PreviousWarm;Binding.HasWarm=HadWarm;Accepted=Previous;return true;
}

void CommitWarm(AFrontlineTank* V)
{
    if(auto* S=States.Find(V);S&&S->HasProposedWarm)
    {S->Warm=S->ProposedWarm;S->HasWarm=S->ProposedHasWarm;S->HasProposedWarm=false;}
}

bool InCompleteGrid(const AFrontlineTank* V,const FVector& P,float Heading)
{
    const auto* R=Runtime(V);const auto* G=R?Scene(R->Battle.Get()):nullptr;const auto* D=Definition(V);if(!G||!D)return false;
    const FTransform Flat(FRotator(0,Heading,0),P);
    for(double X:{D->Minimum.X,D->Maximum.X})for(double Y:{D->Minimum.Y,D->Maximum.Y})
    {const FVector Q=Flat.TransformPosition({X,Y,0});if(Q.X<G->minX+130||Q.X>G->maxX-130||Q.Y<G->minY+130||Q.Y>G->maxY-130)return false;}return true;
}
bool TryContinuousEscape(AFrontlineTank* V,float DT,FTransform& Accepted,float& ActualHeading,float& ActualTravel,float& AvoidHeading)
{
    // Called only after the ordinary forward step and both in-place turns
    // failed, inside the caller's existing no-weapon-halt/0.5s retry gate.
    auto* R=Runtime(V);const auto* D=Definition(V);const auto* S=States.Find(V);
    if(!R||!D||D->Id!=TEXT("fr.ft")||!S||!R->Battle.IsValid()||V->IsImmobilized()
        ||!FMath::IsFinite(DT)||DT<=0||DT>.1f||R->Wait<.3f
        ||FMath::Abs(S->LastSupportSearchRejectAt-R->Battle->Elapsed)>.001f)return false;
    const FVector Start=V->GetActorLocation();const float Budget=FMath::Min(7.f,D->Speed*DT);
    if(!FMath::IsFinite(Budget)||Budget<=.001f)return false;
    const float RouteYaw=R->Battle->GetTeamDirection(V->Team)>0?0.f:180.f;
    const float First=V->Serial%2?-1.f:1.f;int32 Attempt=0;
    for(float Sign:{First,-First})for(float Fraction:{1.f,.5f})
    {
        ++Attempt;const float Want=RouteYaw+Sign*35.f;
        const float Heading=FMath::FixedTurn(R->Heading,Want,DT*32.f);
        if(FMath::Abs(FMath::FindDeltaAngleDegrees(R->Heading,Heading))<=KINDA_SMALL_NUMBER)continue;
        FVector Goal=Start+FRotator(0,Heading,0).Vector()*(Budget*Fraction);float Floor=0;
        if(!WarfareFRRUWorldV82::TryHeight(R->Battle.Get(),Goal.X,Goal.Y,Floor))continue;Goal.Z=Floor;
        if(!InCompleteGrid(V,Start,Heading)||!InCompleteGrid(V,Goal,Heading)
            ||!WarfareFRRUWorldV82::CanTraverse(R->Battle.Get(),Start,Goal))continue;
        FTransform Pose;const float ProposedTravel=float(FVector::Dist2D(Start,Goal));
        // Plan preserves its normal substep, pose-continuity, complete shoe/
        // face, support-ray and full-hull checks; failed trials restore Warm.
        if(!Plan(V,Goal,Heading,ProposedTravel,Pose))continue;
        const float Travel=float(FVector::Dist2D(Start,Pose.GetLocation()));const float Yaw=Pose.Rotator().Yaw;
        if(Travel<=.001f||Travel>Budget+.001f
            ||FMath::Abs(FMath::FindDeltaAngleDegrees(R->Heading,Yaw))>DT*32.f+.001f)continue;
        Accepted=Pose;ActualHeading=Yaw;ActualTravel=Travel;AvoidHeading=Want;
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_FT_CONTINUOUS_ESCAPE_SELECTED serial=%d host=%.3f candidate=%d from=(%s) to=(%s) travel=%.6f budget=%.6f yaw=%.6f full_plan_passed=true actor_committed=false"),
            V->Serial,R->Battle->Elapsed,Attempt,*Start.ToString(),*Pose.GetLocation().ToString(),Travel,Budget,Yaw);
        return true;
    }
    return false;
}
#if WITH_EDITOR
TSharedRef<FJsonObject> Diagnosis(const AFrontlineTank* V)
{
    auto Out=MakeShared<FJsonObject>();FT_CONTACT_DIAG(Out->SetBoolField(TEXT("ftPilotEnabled"),Enabled(V)));
    FT_CONTACT_DIAG(Out->SetBoolField(TEXT("groundPilotEnabled"),Enabled(V)));FT_CONTACT_DIAG(if(const auto* D=Definition(V))Out->SetStringField(TEXT("vehicle"),D->Id));
    if(const auto* S=States.Find(const_cast<AFrontlineTank*>(V)))
    {if(S->Last)FT_CONTACT_DIAG(Out->SetObjectField(TEXT("lastAttempt"),S->Last));if(S->FirstRejected)FT_CONTACT_DIAG(Out->SetObjectField(TEXT("firstRejected"),S->FirstRejected));
        if(S->LastSkinAudit)FT_CONTACT_DIAG(Out->SetObjectField(TEXT("lastSelectedSkinAudit"),S->LastSkinAudit));
        if(S->FirstContactSkinAudit)FT_CONTACT_DIAG(Out->SetObjectField(TEXT("firstContactSkinAudit"),S->FirstContactSkinAudit));}
    return Out;
}
void RecordRejected(AFrontlineTank* V,const FVector& Goal,const TCHAR* Why)
{
    if(!Enabled(V))return;auto& S=States.FindOrAdd(V);auto D=MakeShared<FJsonObject>();
    FT_CONTACT_DIAG(D->SetStringField(TEXT("revision"),TEXT("v82.vehicle.ground.pilot.2")));FT_CONTACT_DIAG(D->SetBoolField(TEXT("passed"),false));
    FT_CONTACT_DIAG(D->SetStringField(TEXT("reason"),Why));FT_CONTACT_DIAG(D->SetStringField(TEXT("desired"),Goal.ToString()));
    FT_CONTACT_DIAG(D->SetNumberField(TEXT("hostElapsed"),Runtime(V)->Battle->Elapsed));S.Last=D;if(!S.FirstRejected)S.FirstRejected=D;
}
#else
void RecordRejected(AFrontlineTank*,const FVector&,const TCHAR*) {}
#endif
#undef FT_CONTACT_DIAG
} // FTGroundPilotV82
