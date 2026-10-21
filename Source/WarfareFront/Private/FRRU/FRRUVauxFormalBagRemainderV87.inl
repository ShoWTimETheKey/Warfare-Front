// One Editor-only native source-copy derivative. No runtime scene/HISM binding.
#pragma once
#if WITH_EDITOR
#include "WarfareFRRUAssetAuditV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Misc/FileHelper.h"
#include "MeshDescription.h"
#include "MeshElementRemappings.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "StaticMeshCompiler.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/Package.h"
#include "UObject/PackageReload.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/MetaData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
#endif

// Windows UE 5.8 inherits GenericPlatformMisc's fatal SHA-256 stub. Use the
// real system CNG implementation instead; every provider/hash failure rejects
// the pinned review normally. This is shared only by the two Editor readers.
inline bool FRRUReadOnlySHA256V87(const TArray<uint8>& Data,FString& Out)
{
    Out.Reset();
#if PLATFORM_WINDOWS
    BCRYPT_ALG_HANDLE Algorithm=nullptr;BCRYPT_HASH_HANDLE Hash=nullptr;
    if(::BCryptOpenAlgorithmProvider(&Algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0||!Algorithm)return false;
    ULONG ObjectBytes=0,HashBytes=0,Returned=0;uint8 Digest[32]={};TArray<uint8> Object;
    bool Good=::BCryptGetProperty(Algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&ObjectBytes),sizeof(ObjectBytes),&Returned,0)>=0
        &&Returned==sizeof(ObjectBytes)&&ObjectBytes>0&&ObjectBytes<=1024*1024;
    if(Good)Good=::BCryptGetProperty(Algorithm,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&HashBytes),sizeof(HashBytes),&Returned,0)>=0
        &&Returned==sizeof(HashBytes)&&HashBytes==sizeof(Digest);
    if(Good)
    {
        Object.SetNumZeroed(int32(ObjectBytes));
        Good=::BCryptCreateHash(Algorithm,&Hash,Object.GetData(),ObjectBytes,nullptr,0,0)>=0&&Hash;
    }
    if(Good)Good=::BCryptHashData(Hash,const_cast<PUCHAR>(Data.GetData()),ULONG(Data.Num()),0)>=0;
    if(Good)Good=::BCryptFinishHash(Hash,Digest,sizeof(Digest),0)>=0;
    if(Hash&&::BCryptDestroyHash(Hash)<0)Good=false;
    if(::BCryptCloseAlgorithmProvider(Algorithm,0)<0)Good=false;
    if(Good)Out=BytesToHex(Digest,UE_ARRAY_COUNT(Digest)).ToLower();
    return Good&&Out.Len()==64;
#else
    return false;
#endif
}

namespace FRRUVauxFormalBagRemainderV87
{
constexpr const TCHAR* PlanRelative=TEXT("ArtSource/FRRU/V87/FrenchFormalBags/CandidateV1/fr_vaux/pilot02/plan.json");
constexpr const TCHAR* PlanSHA=TEXT("0d8708c6573cd18ca177ed49e48eaef5103a507f222c006453ac7ba5cf9793f3");
constexpr const TCHAR* SourceObject=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRender/SpatialPartsV2/SM_VauxDressing_P002_V87.SM_VauxDressing_P002_V87");
constexpr const TCHAR* SourceSHA=TEXT("4fdd11e1d87b8b84f268a851cefff68128301a1eafd2b2526414ae9ab63306f1");
constexpr const TCHAR* TargetPackage=TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/FrenchFormalBagsV1/fr_vaux/NativePilot01/SM_VauxFormalBagRemainder_P002_Native_V87");
constexpr const TCHAR* TargetName=TEXT("SM_VauxFormalBagRemainder_P002_Native_V87");
constexpr int32 SourceFaces=31020,RemovedFaces=15476,KeptFaces=15544,InputKeys=7921;

template<class T> void Add(TArray<uint8>& B,const T& V){B.Append(reinterpret_cast<const uint8*>(&V),sizeof(V));}
void Text(TArray<uint8>& B,const FString& V){FTCHARToUTF8 U(*V);Add(B,U.Length());B.Append(reinterpret_cast<const uint8*>(U.Get()),U.Length());}
int32 Compare(const TArray<uint8>& A,const TArray<uint8>& B)
{const int32 N=FMath::Min(A.Num(),B.Num()),C=N?FMemory::Memcmp(A.GetData(),B.GetData(),N):0;return C?C:(A.Num()<B.Num()?-1:(A.Num()>B.Num()?1:0));}
TArray<uint8> Face(const FString& Material,TArray<uint8> (&Corners)[3],bool Collision=false,bool Shadow=true)
{
    int32 Start=0;for(int32 R=1;R<3;++R)for(int32 K=0;K<3;++K)
    {const int32 C=Compare(Corners[(R+K)%3],Corners[(Start+K)%3]);if(C<0){Start=R;break;}if(C>0)break;}
    TArray<uint8> Out;Text(Out,Material);Add(Out,Collision);Add(Out,Shadow);for(int32 K=0;K<3;++K)Out.Append(Corners[(Start+K)%3]);return Out;
}
struct FProof
{
    int32 UVs=0,Total=0,Removed=0,Partial=0;bool Colour=false;
    TArray<int32> RemovedIDs;TArray<TArray<uint8>> Rows;
    void Sort(){Rows.Sort([](const auto& L,const auto& R){return Compare(L,R)<0;});RemovedIDs.Sort();}
    bool Equals(const FProof& Other)const{return UVs==Other.UVs&&Colour==Other.Colour&&Rows==Other.Rows;}
    FString Hash()const
    {
        FSHA1 H;H.Update(reinterpret_cast<const uint8*>(&UVs),sizeof(UVs));H.Update(reinterpret_cast<const uint8*>(&Colour),sizeof(Colour));
        const int32 Count=Rows.Num();H.Update(reinterpret_cast<const uint8*>(&Count),sizeof(Count));
        for(const auto& Row:Rows){const int32 N=Row.Num();H.Update(reinterpret_cast<const uint8*>(&N),sizeof(N));H.Update(Row.GetData(),N);}
        H.Final();uint8 D[FSHA1::DigestSize];H.GetHash(D);return BytesToHex(D,FSHA1::DigestSize);
    }
};
FString PointKey(FVector3f P)
{
    P.Y=-P.Y; // Exact UE mesh-local -> actual FBX/native basis specified by pilot02.
    if(P.X==0.f)P.X=0.f;if(P.Y==0.f)P.Y=0.f;if(P.Z==0.f)P.Z=0.f;
    return BytesToHex(reinterpret_cast<const uint8*>(&P),12);
}
bool FileSHA(const FString& Path,FString& Out)
{
    const int64 Size=IFileManager::Get().FileSize(*Path);if(Size<=0||Size>192*1024*1024)return false;
    const FDateTime Stamp=IFileManager::Get().GetTimeStamp(*Path);TArray<uint8> Data;
    if(!FFileHelper::LoadFileToArray(Data,*Path)||Data.Num()!=Size||IFileManager::Get().FileSize(*Path)!=Size
        ||IFileManager::Get().GetTimeStamp(*Path)!=Stamp)return false;
    return FRRUReadOnlySHA256V87(Data,Out);
}
bool Parse(const FString& TextValue,TSharedPtr<FJsonObject>& Out)
{return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TextValue),Out)&&Out.IsValid();}
bool PinnedFiles(const TMap<FString,FString>& Pins,TArray<TSharedPtr<FJsonValue>>& Failures)
{
    bool Good=true;for(const auto& Pair:Pins){FString Actual;if(!FileSHA(Pair.Key,Actual)||!Actual.Equals(Pair.Value,ESearchCase::IgnoreCase))
        {Good=false;Failures.Add(MakeShared<FJsonValueString>(Pair.Key));}}return Good;
}
bool NoCollision(const UStaticMesh* Mesh)
{
    const auto* B=Mesh?Mesh->GetBodySetup():nullptr;return B&&B->AggGeom.GetElementCount()==0
        &&B->DefaultInstance.GetCollisionEnabled(false)==ECollisionEnabled::NoCollision
        &&B->DefaultInstance.GetCollisionProfileName()==FName(TEXT("NoCollision"));
}
FString Materials(UStaticMesh* Mesh)
{
    TArray<uint8> Bytes;Add(Bytes,Mesh->GetStaticMaterials().Num());for(const auto& Slot:Mesh->GetStaticMaterials())
    {Text(Bytes,Slot.MaterialSlotName.ToString());Text(Bytes,Slot.ImportedMaterialSlotName.ToString());
     Text(Bytes,Slot.MaterialInterface?Slot.MaterialInterface->GetPathName():FString());Text(Bytes,Slot.OverlayMaterialInterface?Slot.OverlayMaterialInterface->GetPathName():FString());}
    return BytesToHex(Bytes.GetData(),Bytes.Num());
}
FString Collision(UStaticMesh* Mesh)
{
    const auto* B=Mesh->GetBodySetup();if(!B)return {};TArray<uint8> Bytes;
    Add(Bytes,uint8(B->CollisionTraceFlag));Add(Bytes,bool(B->bDoubleSidedGeometry));Add(Bytes,bool(B->bGenerateNonMirroredCollision));Add(Bytes,bool(B->bGenerateMirroredCollision));
    Text(Bytes,B->PhysMaterial?B->PhysMaterial->GetPathName():FString());Add(Bytes,Mesh->LODForCollision);Add(Bytes,B->BuildScale3D);
    Text(Bytes,B->DefaultInstance.GetCollisionProfileName().ToString());Add(Bytes,uint8(B->DefaultInstance.GetCollisionEnabled(false)));Add(Bytes,uint8(B->DefaultInstance.GetObjectType()));
    for(int32 C=0;C<int32(ECC_OverlapAll_Deprecated);++C)Add(Bytes,uint8(B->DefaultInstance.GetResponseToChannel(ECollisionChannel(C))));
    Add(Bytes,B->AggGeom.GetElementCount());return BytesToHex(Bytes.GetData(),Bytes.Num());
}
bool Description(const FMeshDescription& D,const TSet<int32>* Keep,FProof& Out,FString& Why)
{
    FStaticMeshConstAttributes A(D);const auto P=A.GetVertexPositions();const auto N=A.GetVertexInstanceNormals(),T=A.GetVertexInstanceTangents();
    const auto S=A.GetVertexInstanceBinormalSigns();const auto C=A.GetVertexInstanceColors();const auto UV=A.GetVertexInstanceUVs();
    const auto Names=A.GetPolygonGroupMaterialSlotNames();const auto Hard=A.GetEdgeHardnesses();Out.UVs=UV.GetNumChannels();Out.Colour=true;
    if(Out.UVs<1||Out.UVs>8){Why=TEXT("authored UV channel contract");return false;}
    for(const auto ID:D.Triangles().GetElementIDs())
    {
        ++Out.Total;if(Keep&&!Keep->Contains(ID.GetValue()))continue;
        const auto VI=D.GetTriangleVertexInstances(ID);const auto Edges=D.GetTriangleEdges(ID);TArray<uint8> Corners[3];
        for(int32 K=0;K<3;++K)
        {
            const FVector3f Position=P[D.GetVertexInstanceVertex(VI[K])],Normal=N[VI[K]],Tangent=T[VI[K]];
            if(Position.ContainsNaN()||Normal.ContainsNaN()||Tangent.ContainsNaN()||Normal.SizeSquared()<1.e-12f||Tangent.SizeSquared()<1.e-12f
                ||(S[VI[K]]!=1.f&&S[VI[K]]!=-1.f)||C[VI[K]].ContainsNaN()){Why=TEXT("invalid authored position/basis/color");return false;}
            auto& B=Corners[K];Add(B,Position);Add(B,Normal);Add(B,Tangent);Add(B,S[VI[K]]);Add(B,C[VI[K]]);
            for(int32 U=0;U<Out.UVs;++U){const FVector2f V=UV.Get(VI[K],U);if(V.ContainsNaN()){Why=TEXT("invalid authored UV");return false;}Add(B,V);}
            Add(B,bool(Hard[Edges[K]]));
        }
        Out.Rows.Add(Face(Names[D.GetTrianglePolygonGroup(ID)].ToString(),Corners));
    }
    Out.Sort();return true;
}
bool Render(UStaticMesh* Mesh,const TSet<FString>* Remove,FProof& Out,FString& Why)
{
    const auto* Data=Mesh?Mesh->GetRenderData():nullptr;if(!Data||Data->LODResources.Num()!=1){Why=TEXT("one actual render LOD required");return false;}
    const auto& L=Data->LODResources[0];const auto& V=L.VertexBuffers;const auto Indices=L.IndexBuffer.GetArrayView();
    const uint32 Count=V.PositionVertexBuffer.GetNumVertices();Out.UVs=V.StaticMeshVertexBuffer.GetNumTexCoords();Out.Colour=V.ColorVertexBuffer.GetNumVertices()!=0;
    if(!Count||Out.UVs<1||Out.UVs>8||!Indices.Num()||Indices.Num()%3||Count>100000||Indices.Num()>150000
        ||!V.PositionVertexBuffer.GetVertexData()||!V.StaticMeshVertexBuffer.GetTangentData()||!V.StaticMeshVertexBuffer.GetTexCoordData()
        ||(Out.Colour&&V.ColorVertexBuffer.GetNumVertices()!=Count)){Why=TEXT("actual CPU stream/count contract");return false;}
    TBitArray<> Covered(false,Indices.Num());
    for(const auto& Section:L.Sections)
    {
        if(!Mesh->GetStaticMaterials().IsValidIndex(Section.MaterialIndex)||!Mesh->GetMaterial(Section.MaterialIndex)
            ||Section.FirstIndex%3||int64(Section.FirstIndex)+int64(Section.NumTriangles)*3>Indices.Num())
        {Why=TEXT("invalid actual section/material");return false;}
        for(uint32 F=Section.FirstIndex;F<Section.FirstIndex+Section.NumTriangles*3;F+=3)
        {
            int32 Selected=0;TArray<uint8> Corners[3];
            for(int32 K=0;K<3;++K)
            {
                if(Covered[int32(F)+K]){Why=TEXT("overlapping actual sections");return false;}Covered[int32(F)+K]=true;
                const uint32 Index=Indices[int32(F)+K];if(Index>=Count){Why=TEXT("invalid actual vertex index");return false;}
                const FVector3f P=V.PositionVertexBuffer.VertexPosition(Index),Y=V.StaticMeshVertexBuffer.VertexTangentY(Index);
                const FVector4f NX=V.StaticMeshVertexBuffer.VertexTangentX(Index),NZ=V.StaticMeshVertexBuffer.VertexTangentZ(Index);
                const FVector3f X(NX.X,NX.Y,NX.Z),Z(NZ.X,NZ.Y,NZ.Z);
                if(P.ContainsNaN()||X.ContainsNaN()||Y.ContainsNaN()||Z.ContainsNaN()||X.SizeSquared()<1.e-12f||Y.SizeSquared()<1.e-12f||Z.SizeSquared()<1.e-12f)
                {Why=TEXT("invalid actual NTB/position");return false;}
                if(Remove&&Remove->Contains(PointKey(P)))++Selected;
                auto& B=Corners[K];Add(B,P);Add(B,X);Add(B,Y);Add(B,Z);
                for(int32 U=0;U<Out.UVs;++U){const FVector2f UV=V.StaticMeshVertexBuffer.GetVertexUV(Index,U);if(UV.ContainsNaN()){Why=TEXT("invalid actual UV");return false;}Add(B,UV);}
                if(Out.Colour)Add(B,V.ColorVertexBuffer.VertexColor(Index));
            }
            ++Out.Total;if(Selected&&Selected!=3){++Out.Partial;Why=TEXT("actual triangle crosses exact selected source keys");return false;}
            if(Selected==3){++Out.Removed;Out.RemovedIDs.Add(int32(F/3));continue;}
            Out.Rows.Add(Face(Mesh->GetMaterial(Section.MaterialIndex)->GetPathName(),Corners,Section.bEnableCollision,Section.bCastShadow));
        }
    }
    for(int32 K=0;K<Covered.Num();++K)if(!Covered[K]){Why=TEXT("unassigned actual render indices");return false;}
    Out.Sort();return true;
}
void Run()
{
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    const FString Expected=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"));FString User;
    if(!IsInGameThread()||!GIsEditor||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxFormalBagBuild"))||!Project.Equals(Expected,ESearchCase::IgnoreCase)
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||User.IsEmpty()||FPaths::IsRelative(User)||User.Contains(TEXT("..")))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_BAG_REMAINDER_REFUSED: exact Editor/NoSave/project/UserDir boundary"));return;}
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
    const FString ReviewRoot=Project+TEXT("/Saved/FRRU/V87FormalBags/");
    if(!User.StartsWith(ReviewRoot,ESearchCase::IgnoreCase)||!User.EndsWith(TEXT("/User")))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_BAG_REMAINDER_REFUSED: disposable Saved/FRRU/V87FormalBags UserDir required"));return;}
    const FString RunID=User.Mid(ReviewRoot.Len(),User.Len()-ReviewRoot.Len()-5);
    if(RunID.IsEmpty()||RunID.Len()>96||RunID.Contains(TEXT("/"))||RunID.Contains(TEXT("\\")))return;
    for(TCHAR C:RunID)if(!FChar::IsAlnum(C)&&C!=TEXT('-')&&C!=TEXT('_'))return;
    const FString Receipt=User+TEXT("/FRRUReview/vaux-formal-bag-remainder.json");
    if(IFileManager::Get().FileExists(*Receipt)){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_BAG_REMAINDER_REFUSED: existing receipt retained"));return;}
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("schema"),TEXT("v87.vaux.formal.bag.native.remainder.1"));J->SetBoolField(TEXT("success"),false);
    J->SetBoolField(TEXT("UEExecuted"),true);J->SetBoolField(TEXT("FBXImported"),false);J->SetBoolField(TEXT("runtimeStageChanged"),false);J->SetBoolField(TEXT("bagsInstanced"),false);
    J->SetBoolField(TEXT("artAccepted"),false);J->SetBoolField(TEXT("runtimeAccepted"),false);J->SetBoolField(TEXT("shippingAccepted"),false);
    J->SetStringField(TEXT("sourceObject"),SourceObject);J->SetStringField(TEXT("targetPackage"),TargetPackage);J->SetStringField(TEXT("planSHA256"),PlanSHA);
    TMap<FString,FString> Pins;bool Saved=false;
    const auto Finish=[&](const FString& Why,bool Passed=false)
    {
        TArray<TSharedPtr<FJsonValue>> Changed;const bool Unchanged=PinnedFiles(Pins,Changed);
        J->SetBoolField(TEXT("protectedFilesUnchanged"),Unchanged);J->SetArrayField(TEXT("changedProtectedFiles"),Changed);
        J->SetNumberField(TEXT("protectedFileCount"),Pins.Num());J->SetBoolField(TEXT("newDerivedPackageSaved"),Saved);
        J->SetBoolField(TEXT("success"),Passed&&Unchanged);J->SetStringField(TEXT("reason"),Why);J->SetStringField(TEXT("recordedUTC"),FDateTime::UtcNow().ToIso8601());
        FString JSON;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&JSON));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Receipt),true);
        const bool Written=FFileHelper::SaveStringToFile(JSON,*Receipt,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_NoReplaceExisting);
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_BAG_REMAINDER success=%d reportWritten=%d saved=%d reason=%s receipt=%s"),Passed&&Unchanged,Written,Saved,*Why,*Receipt);
    };
    const FString PlanFile=Project+TEXT("/")+PlanRelative;FString ActualPlan;
    if(!FileSHA(PlanFile,ActualPlan)||ActualPlan!=PlanSHA)return Finish(TEXT("exact frozen pilot02 plan SHA mismatch"));
    FString JSON;TSharedPtr<FJsonObject> Plan;if(!FFileHelper::LoadFileToString(JSON,*PlanFile)||!Parse(JSON,Plan)
        ||!Plan->GetBoolField(TEXT("success"))||Plan->GetStringField(TEXT("missionId"))!=TEXT("fr.vaux")
        ||Plan->GetStringField(TEXT("site"))!=TEXT("assembly.low_bay.0"))return Finish(TEXT("exact Vaux pilot02 selection required"));
    Pins.Add(PlanFile,PlanSHA);
    const auto Frozen=Plan->GetObjectField(TEXT("protectedBeforeSHA256"));for(const auto& Pair:Frozen->Values)
    {
        const FString Relative(*Pair.Key);
        if(FPaths::IsRelative(Relative)&&!Relative.Contains(TEXT(".."))&&!Relative.Contains(TEXT(":")))
            Pins.Add(Project+TEXT("/")+Relative,Pair.Value->AsString());
        else return Finish(TEXT("protected path escaped fixed project"));
    }
    const auto& Parts=Plan->GetArrayField(TEXT("derivedDressingParts"));if(Parts.Num()!=1)return Finish(TEXT("one fixed source part only"));
    const auto Part=Parts[0]->AsObject();if(!Part.IsValid()||Part->GetStringField(TEXT("sourceObject"))!=SourceObject
        ||Part->GetStringField(TEXT("sourcePackageSHA256"))!=SourceSHA||Part->GetIntegerField(TEXT("removedTriangles"))!=RemovedFaces
        ||Part->GetIntegerField(TEXT("keptTriangles"))!=KeptFaces)return Finish(TEXT("fixed source/count plan mismatch"));
    const FString SourceFile=FPackageName::LongPackageNameToFilename(FPackageName::ObjectPathToPackageName(FString(SourceObject)),FPackageName::GetAssetPackageExtension());Pins.Add(SourceFile,SourceSHA);
    TArray<TSharedPtr<FJsonValue>> BadPins;if(!PinnedFiles(Pins,BadPins))return Finish(TEXT("protected native/collision/source/formal SHA preflight failed"));
    const auto Contract=Part->GetObjectField(TEXT("nativeDeletionContract"));const auto& Keys=Contract->GetArrayField(TEXT("sourcePointKeysLEHex"));
    if(Keys.Num()!=InputKeys||!Contract->GetBoolField(TEXT("rejectPartialPolygonSelection"))||Contract->GetBoolField(TEXT("distanceMatchingAllowed")))return Finish(TEXT("strict point-key contract mismatch"));
    TSet<FString> Selection;for(const auto& Value:Keys)
    {
        const FString Key=Value->AsString().ToUpper();if(Key.Len()!=24)return Finish(TEXT("position key must contain exactly 12 float32 bytes"));
        for(TCHAR C:Key)if(!FChar::IsHexDigit(C))return Finish(TEXT("invalid exact position key"));Selection.Add(Key);
    }
    if(Selection.Num()!=InputKeys)return Finish(TEXT("duplicate exact position keys"));
    TArray<int32> ExpectedRemovedIDs;for(const auto& Value:Part->GetArrayField(TEXT("removedActualFaceIds")))ExpectedRemovedIDs.Add(int32(Value->AsNumber()));
    if(ExpectedRemovedIDs.Num()!=RemovedFaces)return Finish(TEXT("actual source face-ID witness count mismatch"));ExpectedRemovedIDs.Sort();
    const FString TargetFile=FPackageName::LongPackageNameToFilename(TargetPackage,FPackageName::GetAssetPackageExtension());
    if(FindPackage(nullptr,TargetPackage)||FPackageName::DoesPackageExist(TargetPackage)||IFileManager::Get().DirectoryExists(*FPaths::GetPath(TargetFile)))return Finish(TEXT("fixed new NativePilot01 namespace already exists; refuse overwrite"));
    auto* Source=LoadObject<UStaticMesh>(nullptr,SourceObject);if(!Source||Source->GetPathName()!=SourceObject||Source->GetOutermost()->IsDirty()
        ||Source->GetNumSourceModels()!=1||!Source->bAllowCPUAccess||Source->GetNaniteSettings().bEnabled||!NoCollision(Source))return Finish(TEXT("exact clean corrected NoCollision source unavailable"));
    TArray<UStaticMesh*> Wait{Source};FStaticMeshCompilingManager::Get().FinishCompilation(Wait);
    const auto* D=Source->GetMeshDescription(0);if(!D||D->Triangles().Num()!=SourceFaces||Source->GetStaticMaterials().Num()!=6)return Finish(TEXT("actual source Description/slot count differs"));
    TSharedPtr<FJsonObject> Snapshot;if(!Parse(UWarfareFRRUAssetAuditV82::SnapshotStaticMesh(Source),Snapshot)||!Snapshot->GetBoolField(TEXT("success"))
        ||Snapshot->GetStringField(TEXT("geometryUVSectionSHA1"))!=TEXT("BEE6A42F3397D1C9627AB64C0811322F946A847E")
        ||Snapshot->GetStringField(TEXT("collisionSHA1"))!=TEXT("7EBD28E14C5BE1ECC63C71539CEA20D62568033A"))return Finish(TEXT("actual saved/reloaded source snapshot differs from P002 proof"));
    J->SetObjectField(TEXT("sourceSnapshot"),Snapshot);
    const auto* SourceRender=Source->GetRenderData();
    if(!SourceRender||SourceRender->LODResources.Num()!=1)return Finish(TEXT("fixed source one-LOD stream count differs"));
    const auto& SourceLOD=SourceRender->LODResources[0];
    if(SourceLOD.VertexBuffers.PositionVertexBuffer.GetNumVertices()!=30131||SourceLOD.IndexBuffer.GetNumIndices()!=SourceFaces*3
        ||SourceLOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()!=1||SourceLOD.VertexBuffers.ColorVertexBuffer.GetNumVertices()!=30131)
        return Finish(TEXT("fixed source 30131 vertices / 31020 triangles / one UV / 30131 colors differs"));
    const auto& Bindings=Part->GetArrayField(TEXT("materialBindings"));if(Bindings.Num()!=Source->GetStaticMaterials().Num())return Finish(TEXT("source material witness count"));
    for(int32 I=0;I<Bindings.Num();++I)
    {
        const auto* Material=Source->GetMaterial(I);if(!Material||Material->GetPathName()!=Bindings[I]->AsObject()->GetStringField(TEXT("asset"))||Material->GetOutermost()->IsDirty())return Finish(TEXT("source exact paired material path/dirty boundary"));
        const FString Path=FPackageName::LongPackageNameToFilename(Material->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());FString SHA;if(!FileSHA(Path,SHA))return Finish(TEXT("source material package SHA unavailable"));Pins.Add(Path,SHA);
    }
    const FMeshBuildSettings Settings=Source->GetSourceModel(0).BuildSettings;
    if(Settings.bRecomputeNormals||Settings.bRecomputeTangents||Settings.bRemoveDegenerates||Settings.bGenerateLightmapUVs
        ||Settings.BuildScale3D!=FVector(1,1,1)||!Settings.bUseFullPrecisionUVs)return Finish(TEXT("original source build recipe differs"));
    const FString MaterialPin=Materials(Source),CollisionPin=Collision(Source);FString Why;FProof SourceD,SourceR,ExpectedR;
    if(!Description(*D,nullptr,SourceD,Why)||!Render(Source,nullptr,SourceR,Why)||!Render(Source,&Selection,ExpectedR,Why))return Finish(Why);
    if(SourceR.Total!=SourceFaces||ExpectedR.Removed!=RemovedFaces||ExpectedR.Rows.Num()!=KeptFaces||ExpectedR.RemovedIDs!=ExpectedRemovedIDs)return Finish(TEXT("actual renderer does not pair exactly with pilot02 keys/face IDs"));
    FStaticMeshConstAttributes A(*D);const auto Positions=A.GetVertexPositions();TSet<int32> RemoveSet,KeepSet;int32 Partial=0,SelectedCorners=0;
    for(const auto ID:D->Triangles().GetElementIDs())
    {
        int32 Count=0;for(const auto VI:D->GetTriangleVertexInstances(ID))Count+=Selection.Contains(PointKey(Positions[D->GetVertexInstanceVertex(VI)]))?1:0;
        SelectedCorners+=Count;if(Count==3)RemoveSet.Add(ID.GetValue());else if(Count==0)KeepSet.Add(ID.GetValue());else ++Partial;
    }
    J->SetNumberField(TEXT("exactSelectedSourceCorners"),SelectedCorners);J->SetNumberField(TEXT("partialSourceTriangles"),Partial);
    J->SetNumberField(TEXT("removedTriangles"),RemoveSet.Num());J->SetNumberField(TEXT("keptTriangles"),KeepSet.Num());J->SetNumberField(TEXT("inputPositionKeys"),Selection.Num());
    if(Partial||RemoveSet.Num()!=RemovedFaces||KeepSet.Num()!=KeptFaces)return Finish(TEXT("actual authored corner keys/counts fail exact pairing; no approximate deletion"));
    TArray<FPolygonID> Remove;for(const auto Polygon:D->Polygons().GetElementIDs())
    {
        int32 Count=0;const auto Faces=D->GetPolygonTriangles(Polygon);for(const auto FaceID:Faces)Count+=RemoveSet.Contains(FaceID.GetValue())?1:0;
        if(Count&&Count!=Faces.Num())return Finish(TEXT("selection splits an original native polygon"));if(Count)Remove.Add(Polygon);
    }
    FProof ExpectedD;if(!Description(*D,&KeepSet,ExpectedD,Why))return Finish(Why);
    FMeshDescription Cut=*D;Cut.DeletePolygons(Remove);FElementIDRemappings Remapping;Cut.Compact(Remapping);FProof CutProof;
    if(Cut.Triangles().Num()!=KeptFaces||!Description(Cut,nullptr,CutProof,Why)||!CutProof.Equals(ExpectedD))return Finish(TEXT("in-memory DeletePolygons/Compact changed a surviving exact corner"));
    J->SetStringField(TEXT("expectedDescriptionSHA1"),ExpectedD.Hash());J->SetStringField(TEXT("expectedActualRenderSHA1"),ExpectedR.Hash());J->SetNumberField(TEXT("removedWholePolygons"),Remove.Num());
    BadPins.Empty();if(!PinnedFiles(Pins,BadPins)||Source->GetOutermost()->IsDirty())return Finish(TEXT("protected source changed before new duplication"));
    UPackage* NewPackage=CreatePackage(TargetPackage);if(!NewPackage)return Finish(TEXT("new package creation failed"));
    auto* Candidate=DuplicateObject<UStaticMesh>(Source,NewPackage,FName(TargetName));if(!Candidate||Candidate==Source)return Finish(TEXT("exact new source duplicate failed"));
    Candidate->SetFlags(RF_Public|RF_Standalone);Candidate->ClearFlags(RF_Transient);Wait={Candidate};FStaticMeshCompilingManager::Get().FinishCompilation(Wait);
    FProof DuplicateD;const auto* Copy=Candidate->GetMeshDescription(0);
    if(!Copy||!Description(*Copy,nullptr,DuplicateD,Why)||!DuplicateD.Equals(SourceD)||Materials(Candidate)!=MaterialPin||Collision(Candidate)!=CollisionPin
        ||Candidate->GetSourceModel(0).BuildSettings!=Settings)return Finish(TEXT("candidate was not an exact source duplicate"));
    Candidate->GetSectionInfoMap().Clear();Candidate->GetOriginalSectionInfoMap().Clear();
    FStaticMeshConstAttributes CutAttributes(Cut);const auto SlotNames=CutAttributes.GetPolygonGroupMaterialSlotNames();TArray<int32> SectionMaterials;
    for(const auto Group:Cut.PolygonGroups().GetElementIDs())
    {
        if(Cut.GetPolygonGroupPolygonIDs(Group).IsEmpty())continue;int32 MaterialIndex=INDEX_NONE;
        for(int32 I=0;I<Source->GetStaticMaterials().Num();++I)
        {
            const auto& Slot=Source->GetStaticMaterials()[I];if(Slot.ImportedMaterialSlotName!=SlotNames[Group]&&Slot.MaterialSlotName!=SlotNames[Group])continue;
            if(MaterialIndex!=INDEX_NONE)return Finish(TEXT("ambiguous exact original material slot"));MaterialIndex=I;
        }
        int32 OldSection=INDEX_NONE;const auto& Sections=Source->GetRenderData()->LODResources[0].Sections;
        for(int32 I=0;I<Sections.Num();++I)if(Sections[I].MaterialIndex==MaterialIndex){OldSection=I;break;}
        if(MaterialIndex==INDEX_NONE||OldSection==INDEX_NONE)return Finish(TEXT("compact group has no exact original rendered material"));
        FMeshSectionInfo Info=Source->GetSectionInfoMap().Get(0,OldSection);Info.MaterialIndex=MaterialIndex;
        Info.bEnableCollision=Sections[OldSection].bEnableCollision;Info.bCastShadow=Sections[OldSection].bCastShadow;
        const int32 Index=SectionMaterials.Num();Candidate->GetSectionInfoMap().Set(0,Index,Info);Candidate->GetOriginalSectionInfoMap().Set(0,Index,Info);SectionMaterials.Add(MaterialIndex);
    }
    Candidate->CreateMeshDescription(0,MoveTemp(Cut));Candidate->CommitMeshDescription(0);TArray<FText> Errors;Candidate->Build(true,&Errors);
    Wait={Candidate};FStaticMeshCompilingManager::Get().FinishCompilation(Wait);TArray<TSharedPtr<FJsonValue>> BuildErrors;for(const auto& Error:Errors)BuildErrors.Add(MakeShared<FJsonValueString>(Error.ToString()));J->SetArrayField(TEXT("buildErrors"),BuildErrors);
    const auto Validate=[&](UStaticMesh* Mesh)
    {
        FProof ActualD,ActualR;const auto* DescriptionData=Mesh?Mesh->GetMeshDescription(0):nullptr;
        if(!DescriptionData||Mesh->GetNumSourceModels()!=1||!Mesh->bAllowCPUAccess||Mesh->GetNaniteSettings().bEnabled
            ||!Description(*DescriptionData,nullptr,ActualD,Why)||!ActualD.Equals(ExpectedD)
            ||!Render(Mesh,nullptr,ActualR,Why)||ActualR.Total!=KeptFaces||!ActualR.Equals(ExpectedR)
            ||Materials(Mesh)!=MaterialPin||Collision(Mesh)!=CollisionPin||!NoCollision(Mesh)
            ||Mesh->GetSourceModel(0).BuildSettings!=Settings)return false;
        const auto& Sections=Mesh->GetRenderData()->LODResources[0].Sections;if(Sections.Num()!=SectionMaterials.Num())return false;
        for(int32 I=0;I<Sections.Num();++I)if(Sections[I].MaterialIndex!=SectionMaterials[I])return false;
        J->SetStringField(TEXT("actualDescriptionSHA1"),ActualD.Hash());J->SetStringField(TEXT("actualRenderSHA1"),ActualR.Hash());return true;
    };
    FProof AfterSourceD,AfterSourceR;
    if(!Errors.IsEmpty()||!Validate(Candidate)||!Description(*Source->GetMeshDescription(0),nullptr,AfterSourceD,Why)||!AfterSourceD.Equals(SourceD)
        ||!Render(Source,nullptr,AfterSourceR,Why)||!AfterSourceR.Equals(SourceR)||Source->GetOutermost()->IsDirty())return Finish(TEXT("build changed a surviving NTB/UV/color/material/source component; do not save: ")+Why);
    BadPins.Empty();if(!PinnedFiles(Pins,BadPins))return Finish(TEXT("protected files changed before SavePackage"));
    J->SetBoolField(TEXT("wholePolygonsDeletedWithoutRetriangulation"),true);J->SetBoolField(TEXT("allSurvivingDescriptionAndRenderCornersExact"),true);
    NewPackage->GetMetaData().SetValue(Candidate,TEXT("WF_FRRU_PrivateCandidate"),TEXT("V87 Vaux bag remainder only; no scene or bag HISM acceptance"));
    NewPackage->GetMetaData().SetValue(Candidate,TEXT("WF_SourcePlan_SHA256"),PlanSHA);NewPackage->MarkPackageDirty();
    if(IFileManager::Get().FileExists(*TargetFile))return Finish(TEXT("new package file appeared before save; refuse overwrite"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(TargetFile),true);FSavePackageArgs Save;Save.TopLevelFlags=RF_Public|RF_Standalone;Save.SaveFlags=SAVE_NoError;Save.bSlowTask=false;
    if(!UPackage::SavePackage(NewPackage,Candidate,*TargetFile,Save))return Finish(TEXT("new derived SavePackage failed"));Saved=true;
    FAssetRegistryModule::AssetCreated(Candidate);FString SavedSHA;if(!FileSHA(TargetFile,SavedSHA))return Finish(TEXT("new derived file hash unavailable after save"));J->SetStringField(TEXT("savedPackageSHA256"),SavedSHA);
    UPackage* Reloaded=ReloadPackage(NewPackage,LOAD_None);if(!Reloaded)return Finish(TEXT("saved private package reload failed; retained for inspection"));
    Candidate=FindObject<UStaticMesh>(Reloaded,TargetName);if(!Candidate)return Finish(TEXT("saved derived mesh not present after reload"));Wait={Candidate};FStaticMeshCompilingManager::Get().FinishCompilation(Wait);
    FString ReloadSHA;if(!Validate(Candidate)||!FileSHA(TargetFile,ReloadSHA)||ReloadSHA!=SavedSHA)return Finish(TEXT("saved/reloaded derived payload differs; retained for inspection: ")+Why);
    J->SetBoolField(TEXT("actualDiskReloadVerified"),true);J->SetBoolField(TEXT("sourcePackagesSaved"),false);J->SetStringField(TEXT("targetObject"),Candidate->GetPathName());
    Finish(TEXT("Exact corrected-source remainder saved and reloaded; runtime replacement and three bags remain unstaged"),true);
}
static FAutoConsoleCommand Command(TEXT("WF.FRRU.BuildVauxFormalBagRemainderV87"),
    TEXT("Private NoSave Editor-only exact Vaux P002 source-copy bag remainder; never stages gameplay."),FConsoleCommandDelegate::CreateStatic(&Run));
}
#endif
