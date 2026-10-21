// Bounded Vaux-only native Description partition. No FBX or geometry synthesis.
#pragma once
#if WITH_EDITOR
#include "MeshElementRemappings.h"
namespace FRRUVauxSpatialPartsV87
{
using namespace FRRUWorldRenderBasisV826;
struct FGroup {FIntVector Cell;TArray<int32> Faces;};
TArray<FGroup> Groups(const FMeshDescription& D,int32& Small,int32& Long)
{
    FStaticMeshConstAttributes A(D);const auto P=A.GetVertexPositions();TArray<int32> Parent;Parent.Init(INDEX_NONE,D.Vertices().GetArraySize());FBox3d Scene(ForceInit);
    for(const auto ID:D.Vertices().GetElementIDs()){Parent[ID.GetValue()]=ID.GetValue();Scene+=FVector3d(P[ID]);}
    const auto Find=[&Parent](int32 V){while(Parent[V]!=V){Parent[V]=Parent[Parent[V]];V=Parent[V];}return V;};
    for(const auto ID:D.Triangles().GetElementIDs())
    {const auto VI=D.GetTriangleVertexInstances(ID);const int32 R=Find(D.GetVertexInstanceVertex(VI[0]).GetValue());for(int32 K=1;K<3;++K)Parent[Find(D.GetVertexInstanceVertex(VI[K]).GetValue())]=R;}
    struct FComponent {FBox3d Bounds=FBox3d(ForceInit);TArray<int32> Faces;};TMap<int32,FComponent> Components;
    for(const auto ID:D.Triangles().GetElementIDs())
    {const auto VI=D.GetTriangleVertexInstances(ID);auto& C=Components.FindOrAdd(Find(D.GetVertexInstanceVertex(VI[0]).GetValue()));C.Faces.Add(ID.GetValue());for(const auto V:VI)C.Bounds+=FVector3d(P[D.GetVertexInstanceVertex(V)]);}
    TMap<FIntVector,TArray<int32>> Cells;Small=Long=0;
    const auto Cell=[&](const FVector3d& V,int32 Kind){return FIntVector(FMath::FloorToInt((V.X-Scene.Min.X)/1600.),FMath::FloorToInt((V.Y-Scene.Min.Y)/1600.),Kind);};
    for(const auto& Pair:Components)
    {
        const auto& C=Pair.Value;const FVector3d Size=C.Bounds.GetSize();
        if(FMath::Max(Size.X,Size.Y)<=1600.){++Small;Cells.FindOrAdd(Cell(C.Bounds.GetCenter(),0)).Append(C.Faces);}
        else
        {
            ++Long;TSet<int32> Polygons;for(int32 Value:C.Faces)Polygons.Add(D.GetTrianglePolygon(FTriangleID(Value)).GetValue());
            // A source polygon can contain several immutable triangles. Keep
            // that polygon together, including its perimeter/internal edges;
            // deleting triangles alone leaves the polygon-group index alive.
            for(int32 Value:Polygons)
            {
                const FPolygonID Polygon(Value);const auto VIs=D.GetPolygonVertexInstances(Polygon);FVector3d Centre=FVector3d::ZeroVector;
                for(const auto VI:VIs)Centre+=FVector3d(P[D.GetVertexInstanceVertex(VI)]);
                auto& Bucket=Cells.FindOrAdd(Cell(Centre/VIs.Num(),1));for(const auto Face:D.GetPolygonTriangles(Polygon))Bucket.Add(Face.GetValue());
            }
        }
    }
    TArray<FIntVector> Keys;Cells.GetKeys(Keys);Keys.Sort([](const FIntVector& L,const FIntVector& R){return L.Z!=R.Z?L.Z<R.Z:L.X!=R.X?L.X<R.X:L.Y<R.Y;});TArray<FGroup> Out;
    for(const auto& Key:Keys){auto& G=Out.AddDefaulted_GetRef();G.Cell=Key;G.Faces=MoveTemp(Cells.FindChecked(Key));G.Faces.Sort();}return Out;
}
FString SelectedPin(const FMeshDescription& D,const TArray<int32>* Faces)
{
    FStaticMeshConstAttributes A(D);const auto P=A.GetVertexPositions();const auto N=A.GetVertexInstanceNormals(),T=A.GetVertexInstanceTangents();const auto S=A.GetVertexInstanceBinormalSigns();
    const auto UV=A.GetVertexInstanceUVs();const auto C=A.GetVertexInstanceColors();const auto Names=A.GetPolygonGroupMaterialSlotNames();const auto Hard=A.GetEdgeHardnesses();
    TArray<int32> All;if(!Faces){for(const auto ID:D.Triangles().GetElementIDs())All.Add(ID.GetValue());Faces=&All;}
    TArray<TArray<uint8>> Rows;Rows.Reserve(Faces->Num());
    for(const int32 Value:*Faces)
    {
        const FTriangleID ID(Value);const auto VI=D.GetTriangleVertexInstances(ID);const auto Edges=D.GetTriangleEdges(ID);TArray<uint8> Corners[3];
        for(int32 K=0;K<3;++K)
        {auto& B=Corners[K];Append(B,P[D.GetVertexInstanceVertex(VI[K])]);Append(B,N[VI[K]]);Append(B,T[VI[K]]);Append(B,S[VI[K]]);Append(B,C[VI[K]]);for(int32 U=0;U<UV.GetNumChannels();++U)Append(B,UV.Get(VI[K],U));Append(B,bool(Hard[Edges[K]]));}
        int32 Start=0;for(int32 R=1;R<3;++R)for(int32 K=0;K<3;++K){const int32 Q=Compare(Corners[(R+K)%3],Corners[(Start+K)%3]);if(Q<0){Start=R;break;}if(Q>0)break;}
        auto& B=Rows.AddDefaulted_GetRef();const FString Name=Names[D.GetTrianglePolygonGroup(ID)].ToString();FTCHARToUTF8 UTF(*Name);Append(B,UTF.Length());B.Append(reinterpret_cast<const uint8*>(UTF.Get()),UTF.Length());for(int32 K=0;K<3;++K)B.Append(Corners[(Start+K)%3]);
    }
    Rows.Sort([](const TArray<uint8>& L,const TArray<uint8>& R){return Compare(L,R)<0;});FPin H;H.Bytes(UV.GetNumChannels());H.Bytes(Rows.Num());for(const auto& Row:Rows)H.Record(Row);return H.Finish();
}
TSharedPtr<FJsonObject> Partition(UStaticMesh* Source,UStaticMesh* Candidate,int32 Part)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schemaVersion"),TEXT("v87.vaux.native.spatial.parts.1"));J->SetBoolField(TEXT("success"),false);J->SetBoolField(TEXT("mutationAttempted"),false);
    J->SetBoolField(TEXT("artAccepted"),false);J->SetBoolField(TEXT("runtimeAccepted"),false);J->SetBoolField(TEXT("sceneActivationPerformed"),false);
    const auto Reject=[&](const FString& Why){J->SetStringField(TEXT("reason"),Why);return J;};
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!IsInGameThread()||!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase))return Reject(TEXT("wrong isolated receiver or thread"));
    if(!Source||Source->GetPathName()!=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRender/BasisRepairV2/SM_VauxDressing_SourceBasisOnly_V87.SM_VauxDressing_SourceBasisOnly_V87")
        ||Source->GetOutermost()->IsDirty()||Source->GetNumSourceModels()!=1||!Source->bAllowCPUAccess||Source->GetNaniteSettings().bEnabled||!NoCollision(Source))return Reject(TEXT("exact clean saved Vaux V2 basis candidate required"));
    const auto* D=Source->GetMeshDescription(0);if(!D||D->Triangles().Num()!=318760||D->VertexInstances().Num()>1000000||D->Vertices().Num()>1000000)return Reject(TEXT("exact bounded Vaux source Description required"));
    const FMeshBuildSettings Settings=Source->GetSourceModel(0).BuildSettings;
    if(Settings.bRecomputeNormals||Settings.bRecomputeTangents||Settings.bRemoveDegenerates||Settings.bGenerateLightmapUVs||Settings.BuildScale3D!=FVector(1,1,1)||Settings.MaxLumenMeshCards!=12)return Reject(TEXT("original settings and twelve-card budget required"));
    const auto Before=RenderProof(Source);J->SetObjectField(TEXT("sourceRender"),Before);
    if(!Before->GetBoolField(TEXT("success"))||Before->GetNumberField(TEXT("triangles"))!=318760.||Before->GetNumberField(TEXT("zeroNormalVertices"))!=0.||Before->GetNumberField(TEXT("zeroTangentXVertices"))!=0.||Before->GetNumberField(TEXT("zeroTangentYVertices"))!=0.)return Reject(TEXT("source actual render is not the healthy V2 candidate"));
    int32 Small=0,Long=0;const auto Parts=Groups(*D,Small,Long);if(Parts.Num()<2||Parts.Num()>128)return Reject(TEXT("unexpected spatial partition count"));
    TSet<int32> Covered;TArray<TSharedPtr<FJsonValue>> Plan;for(int32 Index=0;Index<Parts.Num();++Index)
    {
        const auto& G=Parts[Index];auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("part"),Index);Row->SetNumberField(TEXT("triangles"),G.Faces.Num());Row->SetStringField(TEXT("cell"),G.Cell.ToString());
        FPin IDs;TArray<TSharedPtr<FJsonValue>> Values;for(int32 ID:G.Faces){if(Covered.Contains(ID))return Reject(TEXT("duplicate source face"));Covered.Add(ID);IDs.Bytes(ID);if(!Candidate&&Part<0)Values.Add(MakeShared<FJsonValueNumber>(ID));}
        Row->SetStringField(TEXT("sourceFaceIDsSHA1"),IDs.Finish());if(!Values.IsEmpty())Row->SetArrayField(TEXT("sourceFaceIDs"),Values);Plan.Add(MakeShared<FJsonValueObject>(Row));
    }
    if(Covered.Num()!=D->Triangles().Num())return Reject(TEXT("partition misses a source triangle"));
    J->SetNumberField(TEXT("parts"),Parts.Num());J->SetNumberField(TEXT("smallConnectedComponentsKeptWhole"),Small);J->SetNumberField(TEXT("longConnectedComponentsSplitSpatially"),Long);J->SetNumberField(TEXT("cellSizeCm"),1600);J->SetNumberField(TEXT("maxLumenCardsPerPart"),12);J->SetBoolField(TEXT("allSourceFacesExactlyOnce"),true);J->SetArrayField(TEXT("plan"),Plan);
    if(!Candidate&&Part<0){J->SetBoolField(TEXT("readOnlyPlan"),true);J->SetBoolField(TEXT("success"),true);return J;}
    if(!Candidate||!Parts.IsValidIndex(Part))return Reject(TEXT("isolated candidate and valid part number required"));
    const FString Name=FString::Printf(TEXT("SM_VauxDressing_P%03d_V87"),Part);
    if(Candidate==Source||Candidate->GetPathName()!=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRender/SpatialPartsV2/")+Name+TEXT(".")+Name)return Reject(TEXT("new exact spatial part V2 namespace required"));
    const TMap<int32,FBadCorner> None;const FString SourceNonBasis=DescriptionPin(*D),SourceBasis=BasisPin(*D,None,false),Materials=MaterialsPin(Source),Collision=CollisionPin(Source);
    const auto* Copy=Candidate->GetMeshDescription(0);
    if(!Copy||DescriptionPin(*Copy)!=SourceNonBasis||BasisPin(*Copy,None,false)!=SourceBasis||MaterialsPin(Candidate)!=Materials||CollisionPin(Candidate)!=Collision||Candidate->GetSourceModel(0).BuildSettings!=Settings)return Reject(TEXT("candidate is not an exact source duplicate"));
    const auto& Keep=Parts[Part].Faces;TSet<int32> KeepSet;for(int32 ID:Keep)KeepSet.Add(ID);TArray<FPolygonID> Remove;
    for(const auto Polygon:D->Polygons().GetElementIDs())
    {
        int32 Kept=0;const auto Faces=D->GetPolygonTriangles(Polygon);for(const auto Face:Faces)Kept+=KeepSet.Contains(Face.GetValue())?1:0;
        if(Kept!=0&&Kept!=Faces.Num())return Reject(TEXT("spatial group would split an original polygon"));
        if(Kept==0)Remove.Add(Polygon);
    }
    const FString Expected=SelectedPin(*D,&Keep);FMeshDescription Cut=*D;Cut.DeletePolygons(Remove);FElementIDRemappings Remapping;Cut.Compact(Remapping);
    if(Cut.Triangles().Num()!=Keep.Num()||SelectedPin(Cut,nullptr)!=Expected)return Reject(TEXT("polygon-safe DeletePolygons/Compact changed selected oriented source attributes"));
    J->SetBoolField(TEXT("wholeSourcePolygonsPreserved"),true);J->SetStringField(TEXT("deletionAPI"),TEXT("MeshDescription.DeletePolygons then Compact; no retriangulation"));
    // Compact changes polygon-group/section ordinals, not their imported slot
    // names. A duplicate's old ordinal overrides would put hessian on timber.
    // Keep the six original slots and rebuild only this new part's overrides
    // from the compact group's exact imported name before the real build.
    J->SetBoolField(TEXT("mutationAttempted"),true);Candidate->GetSectionInfoMap().Clear();Candidate->GetOriginalSectionInfoMap().Clear();
    FStaticMeshConstAttributes CutAttributes(Cut);const auto SlotNames=CutAttributes.GetPolygonGroupMaterialSlotNames();TArray<int32> ExpectedSectionMaterials;TArray<TSharedPtr<FJsonValue>> SlotMap;
    for(const auto Group:Cut.PolygonGroups().GetElementIDs())
    {
        if(Cut.GetPolygonGroupPolygons(Group).IsEmpty())continue;const FName SlotName=SlotNames[Group];int32 MaterialIndex=INDEX_NONE;
        for(int32 Index=0;Index<Source->GetStaticMaterials().Num();++Index)
        {
            const auto& Slot=Source->GetStaticMaterials()[Index];if(Slot.ImportedMaterialSlotName!=SlotName&&Slot.MaterialSlotName!=SlotName)continue;
            if(MaterialIndex!=INDEX_NONE)return Reject(TEXT("ambiguous exact original material slot name"));MaterialIndex=Index;
        }
        if(MaterialIndex==INDEX_NONE)return Reject(TEXT("compact polygon group has no exact original material slot"));
        int32 SourceSection=INDEX_NONE;const auto& Sections=Source->GetRenderData()->LODResources[0].Sections;
        for(int32 Index=0;Index<Sections.Num();++Index)if(Sections[Index].MaterialIndex==MaterialIndex){SourceSection=Index;break;}
        if(SourceSection==INDEX_NONE)return Reject(TEXT("original rendered material section missing"));
        FMeshSectionInfo Info=Source->GetSectionInfoMap().Get(0,SourceSection);Info.MaterialIndex=MaterialIndex;
        Info.bEnableCollision=Sections[SourceSection].bEnableCollision;Info.bCastShadow=Sections[SourceSection].bCastShadow;
        const int32 Section=ExpectedSectionMaterials.Num();Candidate->GetSectionInfoMap().Set(0,Section,Info);Candidate->GetOriginalSectionInfoMap().Set(0,Section,Info);ExpectedSectionMaterials.Add(MaterialIndex);
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("section"),Section);Row->SetNumberField(TEXT("compactPolygonGroup"),Group.GetValue());Row->SetStringField(TEXT("importedSlot"),SlotName.ToString());Row->SetNumberField(TEXT("materialIndex"),MaterialIndex);SlotMap.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetArrayField(TEXT("exactCompactSectionMaterialMap"),SlotMap);
    J->SetNumberField(TEXT("part"),Part);J->SetNumberField(TEXT("expectedTriangles"),Keep.Num());J->SetStringField(TEXT("expectedSourceAttributeSurfaceSHA1"),Expected);J->SetBoolField(TEXT("sourceAttributesExactBeforeBuild"),true);J->SetBoolField(TEXT("mutationAttempted"),true);
    Candidate->CreateMeshDescription(0,MoveTemp(Cut));Candidate->CommitMeshDescription(0);TArray<FText> Errors;Candidate->Build(true,&Errors);
    TArray<TSharedPtr<FJsonValue>> Messages;for(const auto& Error:Errors)Messages.Add(MakeShared<FJsonValueString>(Error.ToString()));J->SetArrayField(TEXT("buildErrors"),Messages);
    const auto* Actual=Candidate->GetMeshDescription(0);const auto Render=RenderProof(Candidate);J->SetObjectField(TEXT("actualPartRender"),Render);
    const auto* RenderData=Candidate->GetRenderData();bool SectionMappingExact=RenderData&&RenderData->LODResources.Num()==1&&RenderData->LODResources[0].Sections.Num()==ExpectedSectionMaterials.Num();
    if(SectionMappingExact)for(int32 Index=0;Index<ExpectedSectionMaterials.Num();++Index)SectionMappingExact&=RenderData->LODResources[0].Sections[Index].MaterialIndex==ExpectedSectionMaterials[Index];
    const bool AttributesExact=Actual&&SelectedPin(*Actual,nullptr)==Expected,SettingsExact=Candidate->GetSourceModel(0).BuildSettings==Settings,MaterialsExact=MaterialsPin(Candidate)==Materials,CollisionExact=CollisionPin(Candidate)==Collision&&NoCollision(Candidate);
    const bool SourceExact=!Source->GetOutermost()->IsDirty()&&DescriptionPin(*Source->GetMeshDescription(0))==SourceNonBasis&&BasisPin(*Source->GetMeshDescription(0),None,false)==SourceBasis;
    const bool Healthy=Render->GetBoolField(TEXT("success"))&&Render->GetNumberField(TEXT("triangles"))==Keep.Num()&&Render->GetNumberField(TEXT("zeroNormalVertices"))==0.&&Render->GetNumberField(TEXT("zeroTangentXVertices"))==0.&&Render->GetNumberField(TEXT("zeroTangentYVertices"))==0.&&Render->GetNumberField(TEXT("nonFiniteBasisVertices"))==0.;
    J->SetBoolField(TEXT("sourceAttributesExactAfterBuild"),AttributesExact);J->SetBoolField(TEXT("buildSettingsExact"),SettingsExact);J->SetBoolField(TEXT("materialSlotsExact"),MaterialsExact);J->SetBoolField(TEXT("actualSectionMaterialMapExact"),SectionMappingExact);J->SetBoolField(TEXT("collisionExact"),CollisionExact);J->SetBoolField(TEXT("sourceUnchanged"),SourceExact);J->SetBoolField(TEXT("actualPartBasisAndFaceCountHealthy"),Healthy);J->SetBoolField(TEXT("actualUnionAndNTBDifferenceReviewPending"),true);
    if(!Errors.IsEmpty()||!AttributesExact||!SettingsExact||!MaterialsExact||!SectionMappingExact||!CollisionExact||!SourceExact||!Healthy)return Reject(TEXT("native part source/face/basis/material/collision preservation failed; do not save"));
    J->SetBoolField(TEXT("success"),true);return J;
}
}
#endif
