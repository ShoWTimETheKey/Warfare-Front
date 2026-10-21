// Exact registered dressing invalid-basis copies; no scene binding or FBX roundtrip.
// The older Vaux/German/WorldRender repair guards are unchanged.
#pragma once
#if WITH_EDITOR
namespace FRRUDressingSourceBasisV87
{
#include "FRRUDressingBasisRegistryV87.generated.inl"
TSharedPtr<FJsonObject> DressingRenderProof(UStaticMesh* Mesh,int32 ExpectedTriangles,TSet<FString>* InvalidKeys=nullptr)
{
    using namespace FRRUWorldRenderBasisV826;
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);const auto Reject=[&](const TCHAR* Why){J->SetStringField(TEXT("reason"),Why);return J;};
    const auto* R=Mesh?Mesh->GetRenderData():nullptr;if(!R||R->LODResources.Num()!=1)return Reject(TEXT("single render LOD unavailable"));
    const auto& L=R->LODResources[0];const auto& V=L.VertexBuffers;const auto I=L.IndexBuffer.GetArrayView();
    const uint32 Count=V.PositionVertexBuffer.GetNumVertices(),UVs=V.StaticMeshVertexBuffer.GetNumTexCoords();const bool Colour=V.ColorVertexBuffer.GetNumVertices()!=0;
    if(!Count||UVs==0||UVs>8||I.Num()==0||I.Num()%3||int64(Count)>int64(ExpectedTriangles)*3||I.Num()!=int64(ExpectedTriangles)*3
        ||!V.PositionVertexBuffer.GetVertexData()||!V.StaticMeshVertexBuffer.GetTangentData()||!V.StaticMeshVertexBuffer.GetTexCoordData()
        ||(Colour&&(V.ColorVertexBuffer.GetNumVertices()!=Count||!V.ColorVertexBuffer.GetVertexData())))return Reject(TEXT("CPU stream contract or bounded budget"));
    const int64 CornerSize=12+int64(UVs)*8+(Colour?4:0);if((int64(Count)+I.Num())*CornerSize>512*1024*1024)return Reject(TEXT("surface proof memory bound"));
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
    struct FRegion {int32 Triangles=0;uint32 Sums[5]={};};TMap<FString,FRegion> Regions;
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
            // Diagnostic, order-independent actual NTB signatures per 4m cell.
            // The full oriented nonbasis SHA1 above remains the strict gate.
            FPin BasisFace;FVector3d Centre=FVector3d::ZeroVector;
            for(int32 K=0;K<3;++K)
            {
                const uint32 VI=VIs[(Start+K)%3];BasisFace.Record(Corners[VI]);
                BasisFace.Bytes(V.StaticMeshVertexBuffer.VertexTangentZ(VI));BasisFace.Bytes(V.StaticMeshVertexBuffer.VertexTangentX(VI));BasisFace.Bytes(V.StaticMeshVertexBuffer.VertexTangentY(VI));
                Centre+=FVector3d(V.PositionVertexBuffer.VertexPosition(VI))/3.;
            }
            const FString Cell=FString::Printf(TEXT("%d,%d,%d"),FMath::FloorToInt(Centre.X/400.),FMath::FloorToInt(Centre.Y/400.),FMath::FloorToInt(Centre.Z/400.));
            const FString Digest=BasisFace.Finish();auto& Region=Regions.FindOrAdd(Cell);++Region.Triangles;
            for(int32 K=0;K<5;++K)Region.Sums[K]+=uint32(FCString::Strtoui64(*Digest.Mid(K*8,8),nullptr,16));

        }
        Faces.Sort([](const TArray<uint8>& A,const TArray<uint8>& B){return Compare(A,B)<0;});for(const auto& Face:Faces)Surface.Record(Face);
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("section"),SIndex);Row->SetNumberField(TEXT("materialIndex"),S.MaterialIndex);Row->SetNumberField(TEXT("triangles"),S.NumTriangles);Row->SetBoolField(TEXT("collision"),S.bEnableCollision);Row->SetBoolField(TEXT("castShadow"),S.bCastShadow);Sections.Add(MakeShared<FJsonValueObject>(Row));
    }
    for(int32 K=0;K<Covered.Num();++K)if(!Covered[K])return Reject(TEXT("render unassigned index"));
    TArray<FString> RegionKeys;Regions.GetKeys(RegionKeys);RegionKeys.Sort();TArray<TSharedPtr<FJsonValue>> RegionRows;
    for(const FString& Cell:RegionKeys)
    {
        const auto& Region=Regions.FindChecked(Cell);auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("cell"),Cell);Row->SetNumberField(TEXT("triangles"),Region.Triangles);
        Row->SetStringField(TEXT("actualNTBMultisetDiagnostic"),FString::Printf(TEXT("%08X%08X%08X%08X%08X"),Region.Sums[0],Region.Sums[1],Region.Sums[2],Region.Sums[3],Region.Sums[4]));RegionRows.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetArrayField(TEXT("actualNTBRegions4m"),RegionRows);
    J->SetStringField(TEXT("orientedNonBasisSurfaceSHA1"),Surface.Finish());J->SetNumberField(TEXT("vertices"),Count);J->SetNumberField(TEXT("triangles"),I.Num()/3);J->SetNumberField(TEXT("uvChannels"),UVs);J->SetBoolField(TEXT("coloursPresent"),Colour);J->SetArrayField(TEXT("sections"),Sections);
    J->SetNumberField(TEXT("zeroNormalVertices"),ZeroN);J->SetNumberField(TEXT("zeroTangentXVertices"),ZeroX);J->SetNumberField(TEXT("zeroTangentYVertices"),ZeroY);J->SetNumberField(TEXT("nonFiniteBasisVertices"),NonFinite);J->SetBoolField(TEXT("success"),true);return J;
}


TSharedPtr<FJsonObject> Repair(UStaticMesh* Source,UStaticMesh* Candidate)
{
    using namespace FRRUWorldRenderBasisV826;
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schemaVersion"),TEXT("v87.dressing.source.invalid.basis.copy.1"));
    J->SetBoolField(TEXT("success"),false);J->SetBoolField(TEXT("mutationAttempted"),false);J->SetBoolField(TEXT("repairEligible"),false);
    J->SetBoolField(TEXT("artAccepted"),false);J->SetBoolField(TEXT("runtimeAccepted"),false);J->SetBoolField(TEXT("partitionInputAccepted"),false);
    const auto Reject=[&](const FString& Why){J->SetStringField(TEXT("reason"),Why);return J;};
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!IsInGameThread()||!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase))return Reject(TEXT("wrong isolated receiver or thread"));
    const FDressingPinV87* Pin=nullptr;
    if(Source)for(const auto& Item:DressingPinsV87)if(Source->GetPathName()==Item.Source){Pin=&Item;break;}
    if(!Pin||Source->GetOutermost()->IsDirty()||Source->GetNumSourceModels()!=1||!Source->bAllowCPUAccess||Source->GetNaniteSettings().bEnabled||!NoCollision(Source))return Reject(TEXT("exact saved NoCollision dressing registry source required"));
    TSharedPtr<FJsonObject> Snapshot;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UWarfareFRRUAssetAuditV82::SnapshotStaticMesh(Source)),Snapshot)
        ||!Snapshot->GetBoolField(TEXT("success"))||Snapshot->GetStringField(TEXT("geometryUVSectionSHA1"))!=Pin->SnapshotSHA1
        ||Snapshot->GetStringField(TEXT("collisionSHA1"))!=Pin->CollisionSHA1)return Reject(TEXT("actual registered render/collision snapshot changed"));
    J->SetStringField(TEXT("source"),Pin->Source);J->SetStringField(TEXT("destination"),Pin->Candidate);J->SetStringField(TEXT("protectedPackageSHA256"),Pin->PackageSHA256);
    const FMeshBuildSettings Settings=Source->GetSourceModel(0).BuildSettings;
    if(Settings.bRecomputeNormals||Settings.bRecomputeTangents||Settings.bRemoveDegenerates||Settings.bGenerateLightmapUVs||Settings.BuildScale3D!=FVector(1,1,1))return Reject(TEXT("original authored-basis settings changed"));
    const FMeshDescription* D=Source->GetMeshDescription(0);const auto* R=Source->GetRenderData();
    if(!D||!R||R->LODResources.Num()!=1||D->Vertices().Num()==0||D->VertexInstances().Num()==0||D->Triangles().Num()==0
        ||D->Vertices().Num()>int64(Pin->Triangles)*3||D->VertexInstances().Num()>int64(Pin->Triangles)*3||D->Triangles().Num()!=Pin->Triangles)return Reject(TEXT("registered source description outside exact face budget"));
    TSet<FString> RenderBadKeys;const auto Before=DressingRenderProof(Source,Pin->Triangles,&RenderBadKeys);J->SetObjectField(TEXT("beforeRender"),Before);
    if(!Before->GetBoolField(TEXT("success"))||Before->GetNumberField(TEXT("triangles"))!=Pin->Triangles)return Reject(TEXT("saved registered actual render authority changed"));
    FStaticMeshConstAttributes A(*D);const auto N=A.GetVertexInstanceNormals(),T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();const auto UV=A.GetVertexInstanceUVs();
    if(!A.GetVertexPositions().IsValid()||!N.IsValid()||!T.IsValid()||!S.IsValid()||!UV.IsValid()||UV.GetNumChannels()!=1||!A.GetVertexInstanceColors().IsValid())return Reject(TEXT("original authored channels missing"));
    const bool Colour=R->LODResources[0].VertexBuffers.ColorVertexBuffer.GetNumVertices()!=0;
    const bool High=R->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis();
    TMap<int32,FBadCorner> Bad;TSet<FString> Covered;
    for(const auto ID:D->VertexInstances().GetElementIDs())
    {
        const FString Key=RecordPin(SourceCorner(*D,ID,Colour));if(!RenderBadKeys.Contains(Key))continue;
        FVector3f BN=N[ID],BT=T[ID];const bool NF=Finite(FVector3d(BN)),TF=Finite(FVector3d(BT));if(NF)BN.Normalize();if(TF)BT.Normalize();
        FBadCorner B;B.ID=ID.GetValue();B.N=!NF||Zero(FVector3d(N[ID]))||Zero(FVector3d(Packed(BN,High)));
        B.T=!TF||Zero(FVector3d(T[ID]))||Zero(FVector3d(Packed(BT,High)))
            ||(!B.N&&TF&&Zero(FVector3d(FVector3f::CrossProduct(Packed(BN,High),Packed(BT,High)))));
        B.S=!FMath::IsFinite(S[ID])||S[ID]==0.f;
        if(B.N||B.T||B.S){Bad.Add(ID.GetValue(),B);Covered.Add(Key);}
    }
    J->SetNumberField(TEXT("authoredTriangles"),D->Triangles().Num());J->SetNumberField(TEXT("renderTriangles"),Pin->Triangles);
    J->SetNumberField(TEXT("invalidAuthoredCorners"),Bad.Num());J->SetNumberField(TEXT("renderInvalidAttributeKeys"),RenderBadKeys.Num());J->SetNumberField(TEXT("mappedInvalidAttributeKeys"),Covered.Num());
    if(Bad.IsEmpty()||Bad.Num()>FMath::Max(4096,(Pin->ZeroN+Pin->ZeroTX+Pin->ZeroTY)*3)||Covered.Num()!=RenderBadKeys.Num())return Reject(TEXT("actual invalid basis does not map completely to bounded authored invalid components"));
    const FString NonBasis=DescriptionPin(*D),Healthy=HealthyComponentPin(*D,Bad),AllBasis=BasisPin(*D,Bad,false);
    const FString Materials=MaterialsPin(Source),Collision=CollisionPin(Source);
    FMeshDescription Repaired=*D;Repaired.BuildVertexInstanceIndexers();FStaticMeshAttributes Edit(Repaired);
    auto EN=Edit.GetVertexInstanceNormals(),ET=Edit.GetVertexInstanceTangents();auto ES=Edit.GetVertexInstanceBinormalSigns();TArray<TSharedPtr<FJsonValue>> Ledger;
    const auto Vec=[](const FVector3f& V){return TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};};
    for(const auto ID:Repaired.VertexInstances().GetElementIDs())if(const FBadCorner* B=Bad.Find(ID.GetValue()))
    {
        FVector3d Normal;FLocalBasis Local;
        if(B->N)
        {
            if(!Unit(FVector3d(N[ID]),Normal))
            {if(!LocalBasis(*D,ID,FVector3d::ZeroVector,false,Local))return Reject(TEXT("invalid source normal has no positive-area local face"));Normal=Local.FaceNormal;}
            EN[ID]=FVector3f(Normal);
        }
        else if(!Unit(FVector3d(N[ID]),Normal))return Reject(TEXT("healthy normal unavailable for local calculation"));
        if(B->T||B->S)
        {
            if(!LocalBasis(*D,ID,Normal,true,Local))return Reject(TEXT("invalid source tangent/sign has no legal local geometric basis"));
            if(B->T)ET[ID]=FVector3f(Local.Tangent);
            if(B->S)
            {
                const FVector3d Tangent(ET[ID]);const FVector3d Bitangent=(Local.RankOneUVFallback||Local.GeometricFallback)?FVector3d::CrossProduct(Normal,Tangent):Local.Bitangent;
                const double Sign=FVector3d::DotProduct(FVector3d::CrossProduct(Normal,Tangent),Bitangent);
                if(!FMath::IsFinite(Sign)||Sign==0.)return Reject(TEXT("local handedness unresolved"));ES[ID]=Sign<0.?-1.f:1.f;
            }
        }
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("vertexInstance"),ID.GetValue());Row->SetArrayField(TEXT("normalBefore"),Vec(N[ID]));Row->SetArrayField(TEXT("normalAfter"),Vec(EN[ID]));
        Row->SetArrayField(TEXT("tangentBefore"),Vec(T[ID]));Row->SetArrayField(TEXT("tangentAfter"),Vec(ET[ID]));Row->SetNumberField(TEXT("signBefore"),S[ID]);Row->SetNumberField(TEXT("signAfter"),ES[ID]);
        Row->SetBoolField(TEXT("normalRepaired"),B->N);Row->SetBoolField(TEXT("tangentRepaired"),B->T);Row->SetBoolField(TEXT("signRepaired"),B->S);
        Row->SetNumberField(TEXT("normalTriangle"),Local.NormalTriangle);Row->SetNumberField(TEXT("tangentTriangle"),Local.TangentTriangle);Row->SetNumberField(TEXT("doubleAreaCm2"),Local.DoubleArea);Row->SetNumberField(TEXT("uvDeterminant"),Local.UVDeterminant);
        Row->SetBoolField(TEXT("rankOneUVFallback"),Local.RankOneUVFallback);Row->SetBoolField(TEXT("geometricFallback"),Local.GeometricFallback);Ledger.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetArrayField(TEXT("changedCornerLedger"),Ledger);J->SetStringField(TEXT("sourceNonBasisSHA1"),NonBasis);J->SetStringField(TEXT("sourceHealthyComponentsSHA1"),Healthy);
    if(DescriptionPin(Repaired)!=NonBasis||HealthyComponentPin(Repaired,Bad)!=Healthy)return Reject(TEXT("in-memory repair changed nonbasis or a healthy component"));
    J->SetBoolField(TEXT("repairEligible"),true);J->SetBoolField(TEXT("preflightSourceUnchanged"),!Source->GetOutermost()->IsDirty()&&DescriptionPin(*Source->GetMeshDescription(0))==NonBasis&&BasisPin(*Source->GetMeshDescription(0),Bad,false)==AllBasis);
    if(!Candidate){J->SetBoolField(TEXT("success"),true);J->SetBoolField(TEXT("readOnlyPreflight"),true);return J;}
    if(Candidate==Source||Candidate->GetPathName()!=Pin->Candidate
        ||Candidate->GetNumSourceModels()!=1||!Candidate->GetMeshDescription(0))return Reject(TEXT("exact new isolated registered dressing derivative required"));
    const auto DuplicateBefore=DressingRenderProof(Candidate,Pin->Triangles);J->SetObjectField(TEXT("candidateBeforeRender"),DuplicateBefore);
    if(!DuplicateBefore->GetBoolField(TEXT("success"))||DuplicateBefore->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))!=Before->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))
        ||DescriptionPin(*Candidate->GetMeshDescription(0))!=NonBasis||BasisPin(*Candidate->GetMeshDescription(0),Bad,false)!=AllBasis
        ||Candidate->GetSourceModel(0).BuildSettings!=Settings||MaterialsPin(Candidate)!=Materials||CollisionPin(Candidate)!=Collision)return Reject(TEXT("candidate is not an exact original description/surface/material/collision copy"));
    J->SetBoolField(TEXT("mutationAttempted"),true);Candidate->CreateMeshDescription(0,MoveTemp(Repaired));Candidate->CommitMeshDescription(0);TArray<FText> Errors;Candidate->Build(true,&Errors);
    TArray<TSharedPtr<FJsonValue>> Messages;for(const auto& Error:Errors)Messages.Add(MakeShared<FJsonValueString>(Error.ToString()));J->SetArrayField(TEXT("buildErrors"),Messages);
    const auto After=DressingRenderProof(Candidate,Pin->Triangles);J->SetObjectField(TEXT("afterRender"),After);const auto* Actual=Candidate->GetMeshDescription(0);
    const bool NonBasisExact=Actual&&DescriptionPin(*Actual)==NonBasis,HealthyExact=Actual&&HealthyComponentPin(*Actual,Bad)==Healthy;
    const bool SurfaceExact=After->GetBoolField(TEXT("success"))&&After->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))==Before->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"));
    const bool BasisHealthy=After->GetBoolField(TEXT("success"))&&After->GetNumberField(TEXT("zeroNormalVertices"))==0.&&After->GetNumberField(TEXT("zeroTangentXVertices"))==0.&&After->GetNumberField(TEXT("zeroTangentYVertices"))==0.&&After->GetNumberField(TEXT("nonFiniteBasisVertices"))==0.;
    const bool SourceUnchanged=!Source->GetOutermost()->IsDirty()&&DescriptionPin(*Source->GetMeshDescription(0))==NonBasis&&BasisPin(*Source->GetMeshDescription(0),Bad,false)==AllBasis;
    const bool SettingsExact=Candidate->GetSourceModel(0).BuildSettings==Settings,MaterialsExact=MaterialsPin(Candidate)==Materials,CollisionExact=CollisionPin(Candidate)==Collision&&NoCollision(Candidate);
    J->SetBoolField(TEXT("authoredNonBasisExact"),NonBasisExact);J->SetBoolField(TEXT("authoredHealthyComponentsExact"),HealthyExact);J->SetBoolField(TEXT("actualOrientedNonBasisSurfaceExact"),SurfaceExact);
    J->SetBoolField(TEXT("actualCPUBasisHealthy"),BasisHealthy);J->SetBoolField(TEXT("originalSourceUnchanged"),SourceUnchanged);J->SetBoolField(TEXT("buildSettingsExact"),SettingsExact);J->SetBoolField(TEXT("materialBindingsExact"),MaterialsExact);J->SetBoolField(TEXT("collisionRecipeExact"),CollisionExact);
    if(!After->GetBoolField(TEXT("success")))return Reject(TEXT("actual candidate render proof failed; do not save"));
    TSet<FString> AllowedRegions;
    const auto Positions=A.GetVertexPositions();
    for(const auto& Pair:Bad)for(const auto Triangle:D->GetVertexInstanceConnectedTriangleIDs(FVertexInstanceID(Pair.Key)))
    {
        FVector3d Centre=FVector3d::ZeroVector;for(const auto VI:D->GetTriangleVertexInstances(Triangle))Centre+=FVector3d(Positions[D->GetVertexInstanceVertex(VI)])/3.;
        const FIntVector Cell(FMath::FloorToInt(Centre.X/400.),FMath::FloorToInt(Centre.Y/400.),FMath::FloorToInt(Centre.Z/400.));
        for(int32 X=-1;X<=1;++X)for(int32 Y=-1;Y<=1;++Y)for(int32 Z=-1;Z<=1;++Z)
            AllowedRegions.Add(FString::Printf(TEXT("%d,%d,%d"),Cell.X+X,Cell.Y+Y,Cell.Z+Z));
    }
    TMap<FString,FString> OriginalRegions;for(const auto& Value:Before->GetArrayField(TEXT("actualNTBRegions4m")))
    {const auto Row=Value->AsObject();OriginalRegions.Add(Row->GetStringField(TEXT("cell")),Row->GetStringField(TEXT("actualNTBMultisetDiagnostic")));}
    TArray<TSharedPtr<FJsonValue>> ChangedRegions;int32 NonlocalRegions=0;
    for(const auto& Value:After->GetArrayField(TEXT("actualNTBRegions4m")))
    {
        const auto Row=Value->AsObject();const FString Cell=Row->GetStringField(TEXT("cell"));const auto* Original=OriginalRegions.Find(Cell);
        if(Original&&*Original==Row->GetStringField(TEXT("actualNTBMultisetDiagnostic")))continue;
        auto Delta=MakeShared<FJsonObject>(*Row);const bool Near=AllowedRegions.Contains(Cell);Delta->SetBoolField(TEXT("withinInvalidSourceCornerOneCellNeighbourhood"),Near);
        NonlocalRegions+=!Near;ChangedRegions.Add(MakeShared<FJsonValueObject>(Delta));
    }
    J->SetArrayField(TEXT("actualNTBChangedRegions4m"),ChangedRegions);J->SetNumberField(TEXT("actualNTBChangedRegions"),ChangedRegions.Num());
    J->SetNumberField(TEXT("actualNTBChangedNonlocalRegions"),NonlocalRegions);J->SetBoolField(TEXT("renderNTBBitwisePreservationRequired"),false);
    // Source healthy attributes are exact; runtime packing/rebuild may change
    // nearby healthy corners. Record real regions rather than hide this drift.
    if(NonlocalRegions!=0)return Reject(TEXT("actual built NTB changed outside the invalid source-corner neighbourhood; do not save"));
    if(!Errors.IsEmpty()||!NonBasisExact||!HealthyExact||!SurfaceExact||!BasisHealthy||!SourceUnchanged||!SettingsExact||!MaterialsExact||!CollisionExact)return Reject(TEXT("isolated source-copy build failed a strict preservation or basis gate; do not save"));
    J->SetBoolField(TEXT("success"),true);return J;
}
}
#endif
