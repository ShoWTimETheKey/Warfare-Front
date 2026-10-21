// One bounded Editor derivative of the saved Vaux source, never an FBX roundtrip.
// Reuses pure basis/proof helpers; neither old repair whitelist is widened.
#pragma once
#if WITH_EDITOR
namespace FRRUVauxSourceBasisV87
{
TSharedPtr<FJsonObject> Repair(UStaticMesh* Source,UStaticMesh* Candidate)
{
    using namespace FRRUWorldRenderBasisV826;
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schemaVersion"),TEXT("v87.vaux.source.invalid.basis.copy.1"));
    J->SetBoolField(TEXT("success"),false);J->SetBoolField(TEXT("mutationAttempted"),false);J->SetBoolField(TEXT("repairEligible"),false);
    J->SetBoolField(TEXT("artAccepted"),false);J->SetBoolField(TEXT("runtimeAccepted"),false);J->SetBoolField(TEXT("partitionInputAccepted"),false);
    const auto Reject=[&](const FString& Why){J->SetStringField(TEXT("reason"),Why);return J;};
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!IsInGameThread()||!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase))return Reject(TEXT("wrong isolated receiver or thread"));
    if(!Source||Source->GetPathName()!=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Scenes/SM_WF_fr_vaux_dressing.SM_WF_fr_vaux_dressing")
        ||Source->GetOutermost()->IsDirty()||Source->GetNumSourceModels()!=1||!Source->bAllowCPUAccess||Source->GetNaniteSettings().bEnabled||!NoCollision(Source))return Reject(TEXT("exact saved NoCollision Vaux source required"));
    const FString Expected=TEXT("e8be3c8e9946245d7fa8ec66dbc84ae9e216b808e1bd81e5020fb68a56561e03");
    if(!Expected.Equals(Source->GetOutermost()->GetMetaData().GetValue(Source,TEXT("WF_Source_SHA256")),ESearchCase::IgnoreCase))return Reject(TEXT("Vaux original source SHA metadata differs"));
    const FMeshBuildSettings Settings=Source->GetSourceModel(0).BuildSettings;
    if(Settings.bRecomputeNormals||Settings.bRecomputeTangents||Settings.bRemoveDegenerates||Settings.bGenerateLightmapUVs||Settings.BuildScale3D!=FVector(1,1,1))return Reject(TEXT("original authored-basis settings changed"));
    const FMeshDescription* D=Source->GetMeshDescription(0);const auto* R=Source->GetRenderData();
    if(!D||!R||R->LODResources.Num()!=1||D->Vertices().Num()==0||D->VertexInstances().Num()==0||D->Triangles().Num()==0
        ||D->Vertices().Num()>1000000||D->VertexInstances().Num()>1000000||D->Triangles().Num()>320452)return Reject(TEXT("original Vaux source description outside bounded budget"));
    TSet<FString> RenderBadKeys;const auto Before=RenderProof(Source,&RenderBadKeys);J->SetObjectField(TEXT("beforeRender"),Before);
    if(!Before->GetBoolField(TEXT("success"))||Before->GetNumberField(TEXT("triangles"))!=318760.)return Reject(TEXT("saved Vaux actual 318760-face render authority changed"));
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
    J->SetNumberField(TEXT("authoredTriangles"),D->Triangles().Num());J->SetNumberField(TEXT("renderTriangles"),318760);
    J->SetNumberField(TEXT("invalidAuthoredCorners"),Bad.Num());J->SetNumberField(TEXT("renderInvalidAttributeKeys"),RenderBadKeys.Num());J->SetNumberField(TEXT("mappedInvalidAttributeKeys"),Covered.Num());
    if(Bad.IsEmpty()||Bad.Num()>4096||Covered.Num()!=RenderBadKeys.Num())return Reject(TEXT("actual invalid basis does not map completely to bounded authored invalid components"));
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
    if(Candidate==Source||Candidate->GetPathName()!=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRender/BasisRepairV2/SM_VauxDressing_SourceBasisOnly_V87.SM_VauxDressing_SourceBasisOnly_V87")
        ||Candidate->GetNumSourceModels()!=1||!Candidate->GetMeshDescription(0))return Reject(TEXT("new isolated Vaux derivative namespace required"));
    const auto DuplicateBefore=RenderProof(Candidate);J->SetObjectField(TEXT("candidateBeforeRender"),DuplicateBefore);
    if(!DuplicateBefore->GetBoolField(TEXT("success"))||DuplicateBefore->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))!=Before->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))
        ||DescriptionPin(*Candidate->GetMeshDescription(0))!=NonBasis||BasisPin(*Candidate->GetMeshDescription(0),Bad,false)!=AllBasis
        ||Candidate->GetSourceModel(0).BuildSettings!=Settings||MaterialsPin(Candidate)!=Materials||CollisionPin(Candidate)!=Collision)return Reject(TEXT("candidate is not an exact original description/surface/material/collision copy"));
    J->SetBoolField(TEXT("mutationAttempted"),true);Candidate->CreateMeshDescription(0,MoveTemp(Repaired));Candidate->CommitMeshDescription(0);TArray<FText> Errors;Candidate->Build(true,&Errors);
    TArray<TSharedPtr<FJsonValue>> Messages;for(const auto& Error:Errors)Messages.Add(MakeShared<FJsonValueString>(Error.ToString()));J->SetArrayField(TEXT("buildErrors"),Messages);
    const auto After=RenderProof(Candidate);J->SetObjectField(TEXT("afterRender"),After);const auto* Actual=Candidate->GetMeshDescription(0);
    const bool NonBasisExact=Actual&&DescriptionPin(*Actual)==NonBasis,HealthyExact=Actual&&HealthyComponentPin(*Actual,Bad)==Healthy;
    const bool SurfaceExact=After->GetBoolField(TEXT("success"))&&After->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"))==Before->GetStringField(TEXT("orientedNonBasisSurfaceSHA1"));
    const bool BasisHealthy=After->GetBoolField(TEXT("success"))&&After->GetNumberField(TEXT("zeroNormalVertices"))==0.&&After->GetNumberField(TEXT("zeroTangentXVertices"))==0.&&After->GetNumberField(TEXT("zeroTangentYVertices"))==0.&&After->GetNumberField(TEXT("nonFiniteBasisVertices"))==0.;
    const bool SourceUnchanged=!Source->GetOutermost()->IsDirty()&&DescriptionPin(*Source->GetMeshDescription(0))==NonBasis&&BasisPin(*Source->GetMeshDescription(0),Bad,false)==AllBasis;
    const bool SettingsExact=Candidate->GetSourceModel(0).BuildSettings==Settings,MaterialsExact=MaterialsPin(Candidate)==Materials,CollisionExact=CollisionPin(Candidate)==Collision&&NoCollision(Candidate);
    J->SetBoolField(TEXT("authoredNonBasisExact"),NonBasisExact);J->SetBoolField(TEXT("authoredHealthyComponentsExact"),HealthyExact);J->SetBoolField(TEXT("actualOrientedNonBasisSurfaceExact"),SurfaceExact);
    J->SetBoolField(TEXT("actualCPUBasisHealthy"),BasisHealthy);J->SetBoolField(TEXT("originalSourceUnchanged"),SourceUnchanged);J->SetBoolField(TEXT("buildSettingsExact"),SettingsExact);J->SetBoolField(TEXT("materialBindingsExact"),MaterialsExact);J->SetBoolField(TEXT("collisionRecipeExact"),CollisionExact);
    J->SetBoolField(TEXT("actualHealthyBasisExportComparisonPending"),true);
    if(!Errors.IsEmpty()||!NonBasisExact||!HealthyExact||!SurfaceExact||!BasisHealthy||!SourceUnchanged||!SettingsExact||!MaterialsExact||!CollisionExact)return Reject(TEXT("isolated source-copy build failed a strict preservation or basis gate; do not save"));
    J->SetBoolField(TEXT("success"),true);return J;
}
}
#endif
