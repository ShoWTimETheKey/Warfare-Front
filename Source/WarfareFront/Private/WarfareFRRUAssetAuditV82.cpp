#include "WarfareFRRUAssetAuditV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "FRRU/WarfareFRRUGermanHostAcceptanceV82.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimCompressionTypes.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "StaticMeshResources.h"
#include "Serialization/MemoryWriter.h"
#include "PhysicsEngine/BodySetup.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/SecureHash.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "FRRU/FRRUWorldRenderBasisV826.inl"
#include "FRRU/FRRUVauxSourceBasisV87.inl"
#include "FRRU/FRRUDressingSourceBasisV87.inl"
#include "FRRU/FRRUVauxSpatialPartsV87.inl"
#endif

#if WITH_EDITOR
namespace
{
FAutoConsoleCommand PersistenceBoundaryCommandV82(TEXT("WF.FRRU.CheckPersistenceBoundaryV82"),
    TEXT("Read the isolated Editor persistence predicate without opening records."),FConsoleCommandDelegate::CreateLambda([]
{
    FString Receipt,Expected;
    if(!FParse::Value(FCommandLine::Get(),TEXT("BoundaryReceipt="),Receipt)
        ||!FParse::Value(FCommandLine::Get(),TEXT("BoundaryExpected="),Expected)||FPaths::IsRelative(Receipt))return;
    Receipt=FPaths::ConvertRelativePathToFull(Receipt);FPaths::NormalizeFilename(Receipt);
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        ||!Receipt.StartsWith(Project+TEXT("/Saved/FRRU/PersistenceBoundary/"))||!Receipt.EndsWith(TEXT(".json"))
        ||FPaths::FileExists(Receipt)||(Expected!=TEXT("accepted")&&Expected!=TEXT("rejected")))return;
    TSharedPtr<FJsonObject> Result;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UWarfareFRRUAssetAuditV82::SnapshotPersistenceReviewBoundary()),Result))return;
    const bool Pass=Result->GetBoolField(TEXT("accepted"))==(Expected==TEXT("accepted"))
        &&Result->GetBoolField(TEXT("outputUnchanged"))==(Expected==TEXT("rejected"))&&!Result->GetBoolField(TEXT("mutationAttempted"));
    Result->SetStringField(TEXT("revision"),TEXT("v82.persistence.boundary.1"));Result->SetStringField(TEXT("expected"),Expected);
    Result->SetBoolField(TEXT("passed"),Pass);FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);
    const bool Written=FFileHelper::SaveStringToFile(Text,*Receipt,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FPlatformMisc::RequestExitWithStatus(false,Pass&&Written?0:4);
}));
}
#endif

FString UWarfareFRRUAssetAuditV82::RepairGermanIngressSourceBasisV82(UStaticMesh* Source,UStaticMesh* Candidate)
{
#if WITH_EDITOR
    const auto Result=FRRUGermanStaticContactV82::RepairSourceInvalidBasisCandidateV82(Source,Candidate);
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result.ToSharedRef(),Writer);return Text;
#else
    return TEXT("{\"success\":false,\"mutationAttempted\":false,\"reason\":\"Editor authored-source audit is unavailable.\"}");
#endif
}

FString UWarfareFRRUAssetAuditV82::RepairWorldRenderBasisV826(UStaticMesh* Mesh)
{
#if WITH_EDITOR
    const auto Result=FRRUWorldRenderBasisV826::Repair(Mesh);
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result.ToSharedRef(),Writer);return Text;
#else
    return TEXT("{\"success\":false,\"mutationAttempted\":false,\"reason\":\"Editor world-render preflight is unavailable.\"}");
#endif
}

FString UWarfareFRRUAssetAuditV82::SnapshotStaticCollisionV87(UStaticMesh* Mesh)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
#if WITH_EDITOR
    const FString Asset=Mesh?Mesh->GetPathName():FString();
    const bool Exact=Asset==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Scenes/SM_WF_fr_vaux_solids.SM_WF_fr_vaux_solids")
        ||Asset==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxFoundation/CandidateV1/Physics/SM_WF_fr_vaux_foundation_V87_solids.SM_WF_fr_vaux_foundation_V87_solids");
    const UBodySetup* Body=Exact?Mesh->GetBodySetup():nullptr;
    if(IsInGameThread()&&Body&&Body->AggGeom.ConvexElems.Num()<=128)
    {
        const auto VectorJSON=[](const FVector& P)
        {return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)});};
        Result->SetStringField(TEXT("asset"),Asset);Result->SetField(TEXT("buildScale3D"),VectorJSON(Body->BuildScale3D));
        Result->SetNumberField(TEXT("collisionTraceFlag"),int32(Body->CollisionTraceFlag));
        Result->SetNumberField(TEXT("spheres"),Body->AggGeom.SphereElems.Num());
        Result->SetNumberField(TEXT("boxes"),Body->AggGeom.BoxElems.Num());
        Result->SetNumberField(TEXT("capsules"),Body->AggGeom.SphylElems.Num());
        Result->SetNumberField(TEXT("taperedCapsules"),Body->AggGeom.TaperedCapsuleElems.Num());
        Result->SetNumberField(TEXT("totalShapes"),Body->AggGeom.GetElementCount());
        TArray<TSharedPtr<FJsonValue>> Hulls;int32 Total=0;bool Finite=!Body->BuildScale3D.ContainsNaN();
        for(int32 I=0;I<Body->AggGeom.ConvexElems.Num();++I)
        {
            const auto& Elem=Body->AggGeom.ConvexElems[I];const FTransform Pose=Elem.GetTransform();
            Total+=Elem.VertexData.Num();if(Total>16384||Elem.VertexData.IsEmpty()||Pose.ContainsNaN()){Finite=false;break;}
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("ordinal"),I);Row->SetStringField(TEXT("name"),Elem.GetName().ToString());
            TArray<TSharedPtr<FJsonValue>> Vertices;
            for(const FVector& Vertex:Elem.VertexData)
            {const FVector P=Pose.TransformPosition(Vertex);Finite&=!P.ContainsNaN();Vertices.Add(VectorJSON(P));}
            Row->SetArrayField(TEXT("verticesAssetLocalCm"),Vertices);Hulls.Add(MakeShared<FJsonValueObject>(Row));
        }
        Result->SetArrayField(TEXT("convexes"),Hulls);Result->SetNumberField(TEXT("vertices"),Total);
        Result->SetStringField(TEXT("coordinateSystem"),TEXT("UE asset local centimetres; convex transform applied, BodySetup build scale reported separately; no actor transform or Y reflection"));
        Result->SetBoolField(TEXT("success"),Finite&&Hulls.Num()==Body->AggGeom.ConvexElems.Num());
        Result->SetBoolField(TEXT("mutationAttempted"),false);
    }
    else Result->SetStringField(TEXT("reason"),TEXT("Exact old/new Vaux physics asset, game thread and bounded collision required."));
#else
    Result->SetStringField(TEXT("reason"),TEXT("Editor collision readback unavailable."));
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

FString UWarfareFRRUAssetAuditV82::RepairVauxSourceBasisV87(UStaticMesh* Source,UStaticMesh* Candidate)
{
#if WITH_EDITOR
    const auto Result=FRRUVauxSourceBasisV87::Repair(Source,Candidate);
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result.ToSharedRef(),Writer);return Text;
#else
    return TEXT("{\"success\":false,\"mutationAttempted\":false,\"reason\":\"Editor Vaux source-copy audit unavailable.\"}");
#endif
}

FString UWarfareFRRUAssetAuditV82::PartitionVauxSourceV87(UStaticMesh* Source,UStaticMesh* Candidate,int32 Part)
{
#if WITH_EDITOR
    const auto Result=FRRUVauxSpatialPartsV87::Partition(Source,Candidate,Part);
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result.ToSharedRef(),Writer);return Text;
#else
    return TEXT("{\"success\":false,\"mutationAttempted\":false,\"reason\":\"Editor Vaux spatial partition unavailable.\"}");
#endif
}

FString UWarfareFRRUAssetAuditV82::RepairDressingSourceBasisV87(UStaticMesh* Source,UStaticMesh* Candidate)
{
#if WITH_EDITOR
    const auto Result=FRRUDressingSourceBasisV87::Repair(Source,Candidate);
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result.ToSharedRef(),Writer);return Text;
#else
    return TEXT("{\"success\":false,\"mutationAttempted\":false,\"reason\":\"Editor registered dressing source-copy unavailable.\"}");
#endif
}

bool UWarfareFRRUAssetAuditV82::PrepareAnimationCompression(UAnimSequence* Sequence)
{
    if(!Sequence||!Sequence->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Motions/")))return false;
#if WITH_EDITOR
    Sequence->CacheDerivedDataForCurrentPlatform();
    Sequence->WaitOnExistingCompression(true);
#endif
    auto* Skeleton=Sequence->GetSkeleton();
    if(!Skeleton||!Sequence->IsCompressedDataValid()||Skeleton->GetReferenceSkeleton().GetNum()!=67)return false;
    TArray<FBoneIndexType> Indices;
    for(int32 I=0;I<67;++I)Indices.Add(FBoneIndexType(I));
    FBoneContainer Bones(Indices,UE::Anim::FCurveFilterSettings(),*Skeleton);
    FAnimExtractContext Context(0.0,false);
#if WITH_EDITOR
    Context.bEnforceCompressedDataSampling=true;
#endif
    // Mirror every UE 5.8 raw-fallback input using public, scoped APIs. The
    // engine's ShouldUseRawDataForPoseExtraction helper itself is protected.
#if WITH_EDITOR
    const auto* ForceRaw=IConsoleManager::Get().FindConsoleVariable(TEXT("a.ForceEvalRawData"));
    if(ForceRaw&&ForceRaw->GetInt()==1)return false;
#endif
    return Sequence->GetCompressedData(Context).Get().IsValid(Sequence)
        &&Sequence->GetSkeletonVirtualBoneGuid()==Skeleton->GetVirtualBoneGuid()
        &&!Bones.GetDisableRetargeting()&&!Bones.ShouldUseRawData()&&!Bones.ShouldUseSourceData();
}

bool UWarfareFRRUAssetAuditV82::PrepareMechanicalAnimationCompression(UAnimSequence* Sequence,USkeletalMesh* Mesh)
{
    if(!Sequence||!Mesh)return false;
    const bool G98=Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_G98_LoadPair_N67_V82.A_DE_G98_LoadPair_N67_V82")
        &&Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/SK_DE_G98_LoadPair_N67_V82.SK_DE_G98_LoadPair_N67_V82");
    const bool MG08=Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_MG08_FullLoad_N67_V82.A_DE_MG08_FullLoad_N67_V82")
        &&Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/SK_DE_MG08_FullLoad_N67_V82.SK_DE_MG08_FullLoad_N67_V82");
    const bool MG0815=Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/SK_DE_MG0815_Pair_N67_V82.SK_DE_MG0815_Pair_N67_V82")
        &&(Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_MG0815_LoadPair_N67_V82.A_DE_MG0815_LoadPair_N67_V82")
            ||Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_MG0815_CarryMechanism_N67_V82.A_DE_MG0815_CarryMechanism_N67_V82")
            ||Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_MG0815_SupportedMechanism_N67_V82.A_DE_MG0815_SupportedMechanism_N67_V82")
            ||Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_MG0815_DeployMechanism_N67_V82.A_DE_MG0815_DeployMechanism_N67_V82")
            ||Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/A_DE_MG0815_PackMechanism_N67_V82.A_DE_MG0815_PackMechanism_N67_V82"));
    const bool VB=Sequence->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Motions/FrenchVB67/A_FR_VB_LoadPair_N67_V82.A_FR_VB_LoadPair_N67_V82")
        &&Mesh->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Weapons/FrenchVB67/SK_FR_VB_Pair_N67_V82.SK_FR_VB_Pair_N67_V82");
    if(!G98&&!MG08&&!MG0815&&!VB)return false;
    USkeleton* Skeleton=Sequence->GetSkeleton();
    if(!Skeleton||Mesh->GetSkeleton()!=Skeleton)return false;
    const FReferenceSkeleton& MeshRest=Mesh->GetRefSkeleton();
    const FReferenceSkeleton& SkeletonRest=Skeleton->GetReferenceSkeleton();
    // Only these receiver-authored pairs are eligible. The original MG 08
    // root/body/bolt/feed hierarchy is retained; its previously unweighted
    // top_cover leaf carries the new crank, never the receiver or sights.
    const TArray<FName> Names=G98?TArray<FName>{TEXT("weapon_root"),TEXT("bolt"),TEXT("charger"),
        TEXT("round_0"),TEXT("round_1"),TEXT("round_2"),TEXT("round_3"),TEXT("round_4")}
        :MG08?TArray<FName>{TEXT("root"),TEXT("weapon_body"),TEXT("weapon_bolt"),TEXT("feed"),TEXT("top_cover")}
        :VB?TArray<FName>{TEXT("root"),TEXT("weapon_body"),TEXT("weapon_bolt"),TEXT("feed"),TEXT("top_cover"),TEXT("grenade")}
        :TArray<FName>{TEXT("weapon_root"),TEXT("drum"),TEXT("bipod_l"),TEXT("bipod_r")};
    const TArray<int32> Parents=G98?TArray<int32>{INDEX_NONE,0,0,0,0,0,0,0}
        :MG08?TArray<int32>{INDEX_NONE,0,1,1,1}:VB?TArray<int32>{INDEX_NONE,0,1,1,1,1}:TArray<int32>{INDEX_NONE,0,0,0};
    if(MeshRest.GetNum()!=Names.Num()||SkeletonRest.GetNum()!=Names.Num())return false;
    TArray<FBoneIndexType> Indices;
    for(int32 I=0;I<Names.Num();++I)
    {
        const int32 Parent=Parents[I];
        if(MeshRest.GetBoneName(I)!=Names[I]||SkeletonRest.GetBoneName(I)!=Names[I]
            ||MeshRest.GetParentIndex(I)!=Parent||SkeletonRest.GetParentIndex(I)!=Parent)return false;
        Indices.Add(FBoneIndexType(I));
    }
#if WITH_EDITOR
    Sequence->CacheDerivedDataForCurrentPlatform();
    Sequence->WaitOnExistingCompression(true);
    const auto* ForceRaw=IConsoleManager::Get().FindConsoleVariable(TEXT("a.ForceEvalRawData"));
    if(ForceRaw&&ForceRaw->GetInt()==1)return false;
#endif
    if(!Sequence->IsCompressedDataValid())return false;
    FBoneContainer Bones(Indices,UE::Anim::FCurveFilterSettings(),*Skeleton);
    FAnimExtractContext Context(0.0,false);
#if WITH_EDITOR
    Context.bEnforceCompressedDataSampling=true;
#endif
    return Sequence->GetCompressedData(Context).Get().IsValid(Sequence)
        &&Sequence->GetSkeletonVirtualBoneGuid()==Skeleton->GetVirtualBoneGuid()
        &&!Bones.GetDisableRetargeting()&&!Bones.ShouldUseRawData()&&!Bones.ShouldUseSourceData();
}

FString UWarfareFRRUAssetAuditV82::SnapshotPersistenceReviewBoundary()
{
    auto Result=MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("accepted"),false);
    Result->SetBoolField(TEXT("mutationAttempted"),false);
#if WITH_EDITOR
    FString User=TEXT("unchanged-review-sentinel");
    const bool Accepted=WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(User);
    Result->SetBoolField(TEXT("accepted"),Accepted);
    Result->SetBoolField(TEXT("outputUnchanged"),User==TEXT("unchanged-review-sentinel"));
    if(Accepted)Result->SetStringField(TEXT("user"),User);
    const TCHAR* CommandLine=FCommandLine::Get();FString Supplied;
    Result->SetBoolField(TEXT("gameFlag"),FParse::Param(CommandLine,TEXT("game")));
    Result->SetBoolField(TEXT("unattendedFlag"),FParse::Param(CommandLine,TEXT("unattended")));
    Result->SetBoolField(TEXT("manualFlag"),FParse::Param(CommandLine,TEXT("WFV13ManualReview")));
    FParse::Value(CommandLine,TEXT("UserDir="),Supplied);Result->SetStringField(TEXT("suppliedUser"),Supplied);
    auto Normal=[](const FString& Path){FString Value=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeDirectoryName(Value);return Value;};
    Result->SetStringField(TEXT("normalizedProject"),Normal(FPaths::ProjectDir()));
    Result->SetStringField(TEXT("normalizedUser"),Normal(FPaths::ProjectUserDir()));
    Result->SetStringField(TEXT("normalizedSaved"),Normal(FPaths::ProjectSavedDir()));
    TArray<FString> Tokens,Switches;FCommandLine::Parse(CommandLine,Tokens,Switches);int32 Count=0;
    TArray<TSharedPtr<FJsonValue>> Unexpected;
    for(FString Argument:Tokens)
    {
        if(Argument.StartsWith(TEXT("-"))||Argument.StartsWith(TEXT("/")))Argument.RightChopInline(1);
        FString Name,Value;if(!Argument.Split(TEXT("="),&Name,&Value))Name=Argument;
        if(Name.Equals(TEXT("UserDir"),ESearchCase::IgnoreCase))++Count;
        if(Name.StartsWith(TEXT("WF"),ESearchCase::IgnoreCase)&&!Name.Equals(TEXT("WFV13ManualReview"),ESearchCase::IgnoreCase))Unexpected.Add(MakeShared<FJsonValueString>(Name));
    }
    Result->SetNumberField(TEXT("userDirArguments"),Count);Result->SetArrayField(TEXT("unexpectedWFArguments"),Unexpected);
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

FString UWarfareFRRUAssetAuditV82::SnapshotSurfaceTextureV87(UTexture2D* Texture)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);Result->SetBoolField(TEXT("readOnly"),true);
#if WITH_EDITOR
    if(Texture&&IsInGameThread())
    {
        Result->SetStringField(TEXT("object"),Texture->GetPathName());Result->SetBoolField(TEXT("compiling"),Texture->IsCompiling());
        Result->SetNumberField(TEXT("sourceWidth"),Texture->Source.GetSizeX());Result->SetNumberField(TEXT("sourceHeight"),Texture->Source.GetSizeY());
        Result->SetNumberField(TEXT("platformWidth"),Texture->GetSizeX());Result->SetNumberField(TEXT("platformHeight"),Texture->GetSizeY());
        Result->SetNumberField(TEXT("platformMips"),Texture->GetNumMips());Result->SetBoolField(TEXT("sRGB"),Texture->SRGB);
        Result->SetNumberField(TEXT("compression"),int32(Texture->CompressionSettings));
        Result->SetBoolField(TEXT("flipGreen"),Texture->bFlipGreenChannel);Result->SetBoolField(TEXT("normalizeNormals"),Texture->bNormalizeNormals);
        TArray<TSharedPtr<FJsonValue>> Mips;const auto* Platform=Texture->GetPlatformData();
        if(Platform)for(int32 I=0;I<Platform->Mips.Num();++I)
        {auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("level"),I);M->SetNumberField(TEXT("width"),Platform->Mips[I].SizeX);M->SetNumberField(TEXT("height"),Platform->Mips[I].SizeY);Mips.Add(MakeShared<FJsonValueObject>(M));}
        Result->SetArrayField(TEXT("mips"),Mips);Result->SetBoolField(TEXT("success"),Platform&&!Texture->IsCompiling()&&!Mips.IsEmpty());
    }
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

FString UWarfareFRRUAssetAuditV82::SnapshotHeightfieldTexture(UTexture2D* Texture)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    Result->SetBoolField(TEXT("artAccepted"),false);
#if WITH_EDITOR
    if(Texture&&(Texture->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Water/"))
        ||Texture->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldTerrainV826/CorrectiveV1/Water/"))
        ||Texture->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldTerrainV826/BatchV1/Water/"))))
    {
        TArray<UTexture*> Pending;Pending.Add(Texture);FTextureCompilingManager::Get().FinishCompilation(Pending);
        FTexturePlatformData* Platform=Texture->GetPlatformData();TArray64<uint8> Source;
        Result->SetStringField(TEXT("object"),Texture->GetPathName());
        Result->SetNumberField(TEXT("sourceFormat"),int32(Texture->Source.GetFormat()));
        Result->SetBoolField(TEXT("sRGB"),Texture->SRGB);
        Result->SetNumberField(TEXT("sourceWidth"),Texture->Source.GetSizeX());
        Result->SetNumberField(TEXT("sourceHeight"),Texture->Source.GetSizeY());
        if(Platform)
        {
            Result->SetStringField(TEXT("platformFormat"),GPixelFormats[Platform->PixelFormat].Name);
            Result->SetNumberField(TEXT("platformWidth"),Platform->SizeX);
            Result->SetNumberField(TEXT("platformHeight"),Platform->SizeY);
            Result->SetNumberField(TEXT("platformMips"),Platform->Mips.Num());
        }
        if(Platform&&Platform->Mips.Num()==1&&Platform->PixelFormat==PF_G16
            &&Texture->Source.GetFormat()==TSF_G16&&!Texture->SRGB&&Texture->Source.GetMipData(Source,0)
            &&Source.Num()==int64(Platform->SizeX)*Platform->SizeY*2)
        {
            const auto& Bulk=Platform->Mips[0].BulkData;
            if(Bulk.GetBulkDataSize()==Source.Num())
            {
                const void* Pixels=Bulk.LockReadOnly();
                if(Pixels)
                {
                    FSHAHash SourceHash,PlatformHash;FSHA1::HashBuffer(Source.GetData(),Source.Num(),SourceHash.Hash);
                    FSHA1::HashBuffer(Pixels,Source.Num(),PlatformHash.Hash);
                    Result->SetStringField(TEXT("sourceG16SHA1"),SourceHash.ToString());
                    Result->SetStringField(TEXT("platformG16SHA1"),PlatformHash.ToString());
                    Result->SetBoolField(TEXT("sourcePlatformIdentical"),SourceHash==PlatformHash);
                    Result->SetBoolField(TEXT("success"),SourceHash==PlatformHash);
                }
                Bulk.Unlock();
            }
        }
    }
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

FString UWarfareFRRUAssetAuditV82::SnapshotStaticMesh(UStaticMesh* Mesh)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    Result->SetBoolField(TEXT("artAccepted"),false);
#if WITH_EDITOR
    const FStaticMeshRenderData* Render=Mesh?Mesh->GetRenderData():nullptr;
    if(Render&&!Render->LODResources.IsEmpty())
    {
        FSHA1 Hash;TArray<TSharedPtr<FJsonValue>> LODs,Slots,BuildSettings,ZeroBasisSamples;int32 ZeroTangentX=0,ZeroTangentY=0;
        const auto Bytes=[&Hash](const auto& Value){Hash.Update(reinterpret_cast<const uint8*>(&Value),sizeof(Value));};
        for(int32 L=0;L<Render->LODResources.Num();++L)
        {
            const auto& LOD=Render->LODResources[L];const auto& VB=LOD.VertexBuffers;
            const uint32 Count=VB.PositionVertexBuffer.GetNumVertices();
            if(Count&&(!VB.PositionVertexBuffer.GetVertexData()||!VB.StaticMeshVertexBuffer.GetTangentData()
                ||!VB.StaticMeshVertexBuffer.GetTexCoordData()))
                return TEXT("{\"success\":false,\"reason\":\"actual static CPU buffers unavailable\"}");
            Bytes(Count);Bytes(VB.StaticMeshVertexBuffer.GetNumTexCoords());
            auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("lod"),L);
            Item->SetNumberField(TEXT("vertices"),Count);Item->SetNumberField(TEXT("uvChannels"),VB.StaticMeshVertexBuffer.GetNumTexCoords());
            Item->SetNumberField(TEXT("vertexColourCount"),VB.ColorVertexBuffer.GetNumVertices());
            Item->SetBoolField(TEXT("fullPrecisionUV"),VB.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
            TArray<TSharedPtr<FJsonValue>> UVHashes,Samples;
            for(uint32 UV=0;UV<VB.StaticMeshVertexBuffer.GetNumTexCoords();++UV)
            {
                FSHA1 UVHash;
                for(uint32 V=0;V<Count;++V){const FVector2f Value=VB.StaticMeshVertexBuffer.GetVertexUV(V,UV);UVHash.Update(reinterpret_cast<const uint8*>(&Value),sizeof(Value));}
                UVHash.Final();uint8 UVDigest[FSHA1::DigestSize];UVHash.GetHash(UVDigest);
                auto Channel=MakeShared<FJsonObject>();Channel->SetNumberField(TEXT("channel"),UV);
                Channel->SetStringField(TEXT("sha1"),BytesToHex(UVDigest,FSHA1::DigestSize));UVHashes.Add(MakeShared<FJsonValueObject>(Channel));
            }
            int32 NonFinite=0,ZeroNormals=0,Degenerate=0,InvalidIndices=0;
            for(uint32 V=0;V<Count;++V)
            {
                const FVector3f Position=VB.PositionVertexBuffer.VertexPosition(V);
                const FVector4f X=VB.StaticMeshVertexBuffer.VertexTangentX(V);
                const FVector3f Y=VB.StaticMeshVertexBuffer.VertexTangentY(V);
                const FVector4f Z=VB.StaticMeshVertexBuffer.VertexTangentZ(V);
                if(Position.ContainsNaN()||X.ContainsNaN()||Y.ContainsNaN()||Z.ContainsNaN())++NonFinite;
                if(FMath::Square(Z.X)+FMath::Square(Z.Y)+FMath::Square(Z.Z)<1.e-12f)++ZeroNormals;
                const bool InvalidX=FMath::Square(X.X)+FMath::Square(X.Y)+FMath::Square(X.Z)<1.e-12f;
                const bool InvalidY=Y.SizeSquared()<1.e-12f;
                if(InvalidX)++ZeroTangentX;if(InvalidY)++ZeroTangentY;
                if((InvalidX||InvalidY)&&ZeroBasisSamples.Num()<8)
                {
                    auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("lod"),L);Sample->SetNumberField(TEXT("vertex"),V);
                    Sample->SetStringField(TEXT("position"),Position.ToString());Sample->SetStringField(TEXT("normal"),Z.ToString());
                    Sample->SetBoolField(TEXT("zeroX"),InvalidX);Sample->SetBoolField(TEXT("zeroY"),InvalidY);
                    ZeroBasisSamples.Add(MakeShared<FJsonValueObject>(Sample));
                }
                Bytes(Position);Bytes(X);Bytes(Y);Bytes(Z);
                for(uint32 UV=0;UV<VB.StaticMeshVertexBuffer.GetNumTexCoords();++UV)
                {const FVector2f Coordinate=VB.StaticMeshVertexBuffer.GetVertexUV(V,UV);Bytes(Coordinate);if(Coordinate.ContainsNaN())++NonFinite;}
                if(VB.ColorVertexBuffer.GetNumVertices())
                {if(V>=VB.ColorVertexBuffer.GetNumVertices()||!VB.ColorVertexBuffer.GetVertexData())return TEXT("{\"success\":false,\"reason\":\"actual color buffer inconsistent\"}");Bytes(VB.ColorVertexBuffer.VertexColor(V));}
                if(V==0||V==Count-1||(Count>7&&V%(Count/7)==0))
                {
                    const auto Vector=[](const auto& Value,int32 Dimensions){TArray<TSharedPtr<FJsonValue>> Values;Values.Add(MakeShared<FJsonValueNumber>(Value.X));Values.Add(MakeShared<FJsonValueNumber>(Value.Y));if(Dimensions==3)Values.Add(MakeShared<FJsonValueNumber>(Value.Z));return Values;};
                    auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("vertex"),V);
                    Sample->SetArrayField(TEXT("position"),Vector(Position,3));Sample->SetArrayField(TEXT("normal"),Vector(Z,3));
                    if(VB.ColorVertexBuffer.GetNumVertices()==Count)
                    {
                        const FColor Colour=VB.ColorVertexBuffer.VertexColor(V);
                        TArray<TSharedPtr<FJsonValue>> RGBA;
                        for(const uint8 Channel:{Colour.R,Colour.G,Colour.B,Colour.A})RGBA.Add(MakeShared<FJsonValueNumber>(Channel));
                        Sample->SetArrayField(TEXT("vertexColourRGBA"),RGBA);
                    }
                    Sample->SetArrayField(TEXT("tangentX"),Vector(X,3));Sample->SetArrayField(TEXT("tangentY"),Vector(Y,3));
                    TArray<TSharedPtr<FJsonValue>> UVValues;
                    for(uint32 UV=0;UV<VB.StaticMeshVertexBuffer.GetNumTexCoords();++UV)
                    {const FVector2f Value=VB.StaticMeshVertexBuffer.GetVertexUV(V,UV);TArray<TSharedPtr<FJsonValue>> Pair;Pair.Add(MakeShared<FJsonValueNumber>(Value.X));Pair.Add(MakeShared<FJsonValueNumber>(Value.Y));UVValues.Add(MakeShared<FJsonValueArray>(Pair));}
                    Sample->SetArrayField(TEXT("uvs"),UVValues);Samples.Add(MakeShared<FJsonValueObject>(Sample));
                }
            }
            const FIndexArrayView Indices=LOD.IndexBuffer.GetArrayView();Bytes(Indices.Num());
            for(int32 I=0;I<Indices.Num();++I){const uint32 Index=Indices[I];Bytes(Index);}
            for(int32 I=0;I+2<Indices.Num();I+=3)
            {
                const uint32 A=Indices[I],B=Indices[I+1],C=Indices[I+2];
                if(A>=Count||B>=Count||C>=Count){++InvalidIndices;continue;}
                const FVector3f Cross=FVector3f::CrossProduct(VB.PositionVertexBuffer.VertexPosition(B)-VB.PositionVertexBuffer.VertexPosition(A),VB.PositionVertexBuffer.VertexPosition(C)-VB.PositionVertexBuffer.VertexPosition(A));
                if(Cross.SizeSquared()<1.e-12f)++Degenerate;
            }
            TArray<TSharedPtr<FJsonValue>> Sections;
            for(int32 I=0;I<LOD.Sections.Num();++I)
            {
                const auto& S=LOD.Sections[I];Bytes(S.MaterialIndex);Bytes(S.FirstIndex);Bytes(S.NumTriangles);
                Bytes(S.MinVertexIndex);Bytes(S.MaxVertexIndex);Bytes(S.bEnableCollision);Bytes(S.bCastShadow);
                auto Section=MakeShared<FJsonObject>();Section->SetNumberField(TEXT("section"),I);
                Section->SetNumberField(TEXT("materialIndex"),S.MaterialIndex);Section->SetNumberField(TEXT("triangles"),S.NumTriangles);
                Sections.Add(MakeShared<FJsonValueObject>(Section));
            }
            Item->SetArrayField(TEXT("sections"),Sections);Item->SetNumberField(TEXT("nonFiniteVertices"),NonFinite);
            Item->SetArrayField(TEXT("uvChannelHashes"),UVHashes);Item->SetArrayField(TEXT("vertexSamples"),Samples);
            Item->SetNumberField(TEXT("zeroNormalVertices"),ZeroNormals);Item->SetNumberField(TEXT("degenerateTriangles"),Degenerate);
            Item->SetNumberField(TEXT("invalidTriangleIndices"),InvalidIndices);LODs.Add(MakeShared<FJsonValueObject>(Item));
        }
        Hash.Final();uint8 Digest[FSHA1::DigestSize];Hash.GetHash(Digest);
        Result->SetStringField(TEXT("geometryUVSectionSHA1"),BytesToHex(Digest,FSHA1::DigestSize));
        if(UBodySetup* Body=Mesh->GetBodySetup())
        {
            TArray<uint8> Data;FMemoryWriter Archive(Data,true);FKAggregateGeom::StaticStruct()->SerializeBin(Archive,&Body->AggGeom);
            Archive<<Body->BodySetupGuid;uint8 TraceFlag=uint8(Body->CollisionTraceFlag);Archive.Serialize(&TraceFlag,sizeof(TraceFlag));
            uint8 CollisionDigest[FSHA1::DigestSize];FSHA1::HashBuffer(Data.GetData(),Data.Num(),CollisionDigest);
            Result->SetStringField(TEXT("collisionSHA1"),BytesToHex(CollisionDigest,FSHA1::DigestSize));
            Result->SetNumberField(TEXT("collisionShapes"),Body->AggGeom.GetElementCount());
            Result->SetNumberField(TEXT("collisionTraceFlag"),TraceFlag);
            // These serialized UPROPERTY() flags are not exposed to Python's
            // editor-property binding. Report them without changing the legacy
            // collision digest or any asset, so metadata-only copies pin both.
            Result->SetBoolField(TEXT("generateNonMirroredCollision"),Body->bGenerateNonMirroredCollision);
            Result->SetBoolField(TEXT("generateMirroredCollision"),Body->bGenerateMirroredCollision);
        }
        else Result->SetStringField(TEXT("collisionSHA1"),TEXT("no-body"));
        for(int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)
        {
            const auto& S=Mesh->GetStaticMaterials()[I];auto Slot=MakeShared<FJsonObject>();Slot->SetNumberField(TEXT("index"),I);
            Slot->SetStringField(TEXT("importedMaterialSlotName"),S.ImportedMaterialSlotName.ToString());
            Slot->SetStringField(TEXT("materialSlotName"),S.MaterialSlotName.ToString());Slots.Add(MakeShared<FJsonValueObject>(Slot));
        }
        for(int32 I=0;I<Mesh->GetNumSourceModels();++I)
        {
            const auto& B=Mesh->GetSourceModel(I).BuildSettings;auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("lod"),I);
            Item->SetBoolField(TEXT("generateLightmapUVs"),B.bGenerateLightmapUVs);Item->SetNumberField(TEXT("sourceLightmapIndex"),B.SrcLightmapIndex);
            Item->SetNumberField(TEXT("destinationLightmapIndex"),B.DstLightmapIndex);Item->SetBoolField(TEXT("recomputeNormals"),B.bRecomputeNormals);
            Item->SetBoolField(TEXT("recomputeTangents"),B.bRecomputeTangents);Item->SetBoolField(TEXT("removeDegenerates"),B.bRemoveDegenerates);
            Item->SetBoolField(TEXT("fullPrecisionUV"),B.bUseFullPrecisionUVs);
            BuildSettings.Add(MakeShared<FJsonValueObject>(Item));
        }
        Result->SetArrayField(TEXT("sourceBuildSettings"),BuildSettings);
        Result->SetArrayField(TEXT("rawMaterialSlots"),Slots);Result->SetArrayField(TEXT("lods"),LODs);
        // Aggregate diagnostics leave existing LOD receipts and byte hashes
        // unchanged, and inspect every saved tangent rather than eight samples.
        Result->SetNumberField(TEXT("zeroTangentXVertices"),ZeroTangentX);Result->SetNumberField(TEXT("zeroTangentYVertices"),ZeroTangentY);
        Result->SetArrayField(TEXT("zeroBasisSamples"),ZeroBasisSamples);
        Result->SetObjectField(TEXT("runtimeStaticMeshSnapshot"),FRRUGermanStaticContactV82::RuntimeStaticMeshSnapshot(Mesh));
        Result->SetBoolField(TEXT("success"),true);
    }
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

FString UWarfareFRRUAssetAuditV82::SnapshotStaticMeshCorrespondence(UStaticMesh* Mesh,const TArray<FVector>& AssetLocalPoints,double MaximumErrorCm)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    Result->SetBoolField(TEXT("mutationAttempted"),false);Result->SetBoolField(TEXT("artAccepted"),false);
#if WITH_EDITOR
    const auto* Render=Mesh?Mesh->GetRenderData():nullptr;
    const bool InputValid=!AssetLocalPoints.IsEmpty()&&AssetLocalPoints.Num()<=64
        &&FMath::IsFinite(MaximumErrorCm)&&MaximumErrorCm>0.&&MaximumErrorCm<=1.;
    if(InputValid&&Render&&!Render->LODResources.IsEmpty())
    {
        const auto& Buffers=Render->LODResources[0].VertexBuffers;
        const uint32 Count=Buffers.PositionVertexBuffer.GetNumVertices();
        const uint32 UVCount=Buffers.StaticMeshVertexBuffer.GetNumTexCoords();
        const uint32 Colours=Buffers.ColorVertexBuffer.GetNumVertices();
        const bool BuffersValid=Count>0&&Buffers.PositionVertexBuffer.GetVertexData()
            &&UVCount>0&&Buffers.StaticMeshVertexBuffer.GetTexCoordData()
            &&Buffers.StaticMeshVertexBuffer.GetTangentData()
            &&(Colours==0||(Colours==Count&&Buffers.ColorVertexBuffer.GetVertexData()));
        if(BuffersValid)
        {
            TArray<TSharedPtr<FJsonValue>> Samples;bool Passed=true;double MaximumObserved=0.;
            for(const FVector& Point:AssetLocalPoints)
            {
                if(Point.ContainsNaN()){Passed=false;break;}
                uint32 Nearest=0;double DistanceSquared=TNumericLimits<double>::Max();
                for(uint32 V=0;V<Count;++V)
                {
                    const FVector Position(Buffers.PositionVertexBuffer.VertexPosition(V));
                    const double Candidate=FVector::DistSquared(Point,Position);
                    if(Candidate<DistanceSquared){DistanceSquared=Candidate;Nearest=V;}
                }
                const double Error=FMath::Sqrt(DistanceSquared);MaximumObserved=FMath::Max(MaximumObserved,Error);
                Passed&=FMath::IsFinite(Error)&&Error<=MaximumErrorCm;
                auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("vertex"),Nearest);
                Sample->SetNumberField(TEXT("errorCm"),Error);
                const auto Array=[](const auto& V){TArray<TSharedPtr<FJsonValue>> A;A.Add(MakeShared<FJsonValueNumber>(V.X));A.Add(MakeShared<FJsonValueNumber>(V.Y));A.Add(MakeShared<FJsonValueNumber>(V.Z));return A;};
                Sample->SetArrayField(TEXT("expectedPosition"),Array(Point));
                Sample->SetArrayField(TEXT("position"),Array(Buffers.PositionVertexBuffer.VertexPosition(Nearest)));
                Sample->SetArrayField(TEXT("normal"),Array(Buffers.StaticMeshVertexBuffer.VertexTangentZ(Nearest)));
                TArray<TSharedPtr<FJsonValue>> UVs;
                for(uint32 Channel=0;Channel<UVCount;++Channel)
                {
                    const FVector2f UV=Buffers.StaticMeshVertexBuffer.GetVertexUV(Nearest,Channel);
                    TArray<TSharedPtr<FJsonValue>> Pair;Pair.Add(MakeShared<FJsonValueNumber>(UV.X));Pair.Add(MakeShared<FJsonValueNumber>(UV.Y));
                    UVs.Add(MakeShared<FJsonValueArray>(Pair));Passed&=!UV.ContainsNaN();
                }
                Sample->SetArrayField(TEXT("uvs"),UVs);
                if(Colours==Count)
                {
                    const FColor Colour=Buffers.ColorVertexBuffer.VertexColor(Nearest);TArray<TSharedPtr<FJsonValue>> RGBA;
                    for(const uint8 Channel:{Colour.R,Colour.G,Colour.B,Colour.A})RGBA.Add(MakeShared<FJsonValueNumber>(Channel));
                    Sample->SetArrayField(TEXT("vertexColourRGBA"),RGBA);
                }
                Samples.Add(MakeShared<FJsonValueObject>(Sample));
            }
            Result->SetStringField(TEXT("mesh"),Mesh->GetPathName());Result->SetNumberField(TEXT("vertices"),Count);
            Result->SetNumberField(TEXT("uvChannels"),UVCount);Result->SetNumberField(TEXT("vertexColourCount"),Colours);
            Result->SetBoolField(TEXT("fullPrecisionUV"),Buffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
            Result->SetNumberField(TEXT("maximumErrorCm"),MaximumObserved);Result->SetArrayField(TEXT("samples"),Samples);
            Result->SetBoolField(TEXT("success"),Passed&&Samples.Num()==AssetLocalPoints.Num());
        }
    }
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

bool UWarfareFRRUAssetAuditV82::AppendVerifiedUnusedStaticMaterialSlot(UStaticMesh* Mesh,FName Name,int64 SourceTriangles)
{
#if WITH_EDITOR
    if(!Mesh||Name.IsNone()||SourceTriangles!=0||!Mesh->GetPathName().StartsWith(TEXT("/Game/FRRUExpansionR3/FormalV82/Scenes/")))return false;
    for(const auto& S:Mesh->GetStaticMaterials())if(S.MaterialSlotName==Name||S.ImportedMaterialSlotName==Name)return false;
    TSharedPtr<FJsonObject> Before,After;
    const auto Read=[](const FString& Text,TSharedPtr<FJsonObject>& Out){return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out)&&Out.IsValid()&&Out->GetBoolField(TEXT("success"));};
    if(!Read(SnapshotStaticMesh(Mesh),Before))return false;
    Mesh->Modify();Mesh->GetStaticMaterials().Emplace(nullptr,Name,Name);Mesh->MarkPackageDirty();
    return Read(SnapshotStaticMesh(Mesh),After)
        &&Before->GetStringField(TEXT("geometryUVSectionSHA1"))==After->GetStringField(TEXT("geometryUVSectionSHA1"))
        &&Before->GetStringField(TEXT("collisionSHA1"))==After->GetStringField(TEXT("collisionSHA1"));
#else
    return false;
#endif
}

FString UWarfareFRRUAssetAuditV82::SnapshotPhysicsAsset(UPhysicsAsset* Asset,USkeletalMesh* Mesh)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    Result->SetBoolField(TEXT("ragdollAccepted"),false);
#if WITH_EDITOR
    if(Asset&&Mesh)
    {
        const FReferenceSkeleton& Rest=Mesh->GetRefSkeleton();
        TArray<TSharedPtr<FJsonValue>> Bodies,Constraints,ForeignBodies,ForeignConstraints;
        for(const USkeletalBodySetup* Body:Asset->SkeletalBodySetups)
        {
            auto Item=MakeShared<FJsonObject>();Item->SetBoolField(TEXT("valid"),Body!=nullptr);
            if(Body)
            {
                Item->SetStringField(TEXT("boneName"),Body->BoneName.ToString());
                if(Rest.FindBoneIndex(Body->BoneName)==INDEX_NONE)
                    ForeignBodies.Add(MakeShared<FJsonValueString>(Body->BoneName.ToString()));
                Item->SetNumberField(TEXT("shapeCount"),Body->AggGeom.GetElementCount());
                Item->SetNumberField(TEXT("convexCount"),Body->AggGeom.ConvexElems.Num());
                Item->SetNumberField(TEXT("sphereCount"),Body->AggGeom.SphereElems.Num());
                Item->SetNumberField(TEXT("boxCount"),Body->AggGeom.BoxElems.Num());
                Item->SetNumberField(TEXT("capsuleCount"),Body->AggGeom.SphylElems.Num());
            }
            Bodies.Add(MakeShared<FJsonValueObject>(Item));
        }
        for(const UPhysicsConstraintTemplate* Constraint:Asset->ConstraintSetup)
        {
            auto Item=MakeShared<FJsonObject>();Item->SetBoolField(TEXT("valid"),Constraint!=nullptr);
            if(Constraint)
            {
                Item->SetStringField(TEXT("bone1"),Constraint->DefaultInstance.ConstraintBone1.ToString());
                Item->SetStringField(TEXT("bone2"),Constraint->DefaultInstance.ConstraintBone2.ToString());
                for(const FName Bone:{Constraint->DefaultInstance.ConstraintBone1,Constraint->DefaultInstance.ConstraintBone2})
                    if(Rest.FindBoneIndex(Bone)==INDEX_NONE)
                        ForeignConstraints.Add(MakeShared<FJsonValueString>(Bone.ToString()));
            }
            Constraints.Add(MakeShared<FJsonValueObject>(Item));
        }
        Result->SetStringField(TEXT("objectPath"),Asset->GetPathName());
        Result->SetStringField(TEXT("referenceMesh"),Mesh->GetPathName());
        Result->SetNumberField(TEXT("bodyCount"),Bodies.Num());
        Result->SetNumberField(TEXT("constraintCount"),Constraints.Num());
        Result->SetArrayField(TEXT("foreignBodyBones"),ForeignBodies);
        Result->SetArrayField(TEXT("foreignConstraintBones"),ForeignConstraints);
        Result->SetArrayField(TEXT("bodies"),Bodies);Result->SetArrayField(TEXT("constraints"),Constraints);
        Result->SetBoolField(TEXT("success"),true);
    }
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

FString UWarfareFRRUAssetAuditV82::SnapshotSkeletalMesh(USkeletalMesh* Mesh)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    Result->SetBoolField(TEXT("skinAccepted"),false);
#if WITH_EDITOR
    const FSkeletalMeshModel* Model=Mesh?Mesh->GetImportedModel():nullptr;
    if(Model&&!Model->LODModels.IsEmpty())
    {
        FSHA1 Hash,GeometryHash,NamedSkinHash;TArray<TSharedPtr<FJsonValue>> LODs;
        const auto Bytes=[&Hash](const auto& Value){Hash.Update(reinterpret_cast<const uint8*>(&Value),sizeof(Value));};
        const auto Attributes=[&](const auto& Value){Bytes(Value);GeometryHash.Update(reinterpret_cast<const uint8*>(&Value),sizeof(Value));};
        for(int32 L=0;L<Model->LODModels.Num();++L)
        {
            const FSkeletalMeshLODModel& LOD=Model->LODModels[L];
            if(LOD.NumTexCoords>MAX_TEXCOORDS)return TEXT("{\"success\":false,\"reason\":\"actual UV count exceeds imported model storage\"}");
            Attributes(LOD.NumVertices);Attributes(LOD.NumTexCoords);
            if(!LOD.IndexBuffer.IsEmpty())
            {
                const uint8* Data=reinterpret_cast<const uint8*>(LOD.IndexBuffer.GetData());
                const uint32 Size=LOD.IndexBuffer.Num()*sizeof(uint32);Hash.Update(Data,Size);GeometryHash.Update(Data,Size);
            }
            auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("lod"),L);
            Item->SetNumberField(TEXT("vertices"),LOD.NumVertices);Item->SetNumberField(TEXT("uvChannels"),LOD.NumTexCoords);
            TArray<TSharedPtr<FJsonValue>> Sections;
            int32 NonFiniteVertices=0,ZeroNormals=0,UnweightedVertices=0,InvalidWeightedBones=0;
            for(int32 I=0;I<LOD.Sections.Num();++I)
            {
                const FSkelMeshSection& S=LOD.Sections[I];
                Attributes(S.MaterialIndex);Attributes(S.BaseIndex);Attributes(S.NumTriangles);Attributes(S.BaseVertexIndex);Attributes(S.NumVertices);
                if(!S.BoneMap.IsEmpty())Hash.Update(reinterpret_cast<const uint8*>(S.BoneMap.GetData()),S.BoneMap.Num()*sizeof(FBoneIndexType));
                for(const FSoftSkinVertex& V:S.SoftVertices)
                {
                    if(V.Position.ContainsNaN()||V.TangentX.ContainsNaN()||V.TangentY.ContainsNaN()||V.TangentZ.ContainsNaN())++NonFiniteVertices;
                    if(FMath::Square(V.TangentZ.X)+FMath::Square(V.TangentZ.Y)+FMath::Square(V.TangentZ.Z)<1.e-12f)++ZeroNormals;
                    uint32 TotalWeight=0;
                    for(uint32 Influence=0;Influence<UE_ARRAY_COUNT(V.InfluenceWeights);++Influence)
                    {
                        TotalWeight+=V.InfluenceWeights[Influence];
                        if(V.InfluenceWeights[Influence]&&!S.BoneMap.IsValidIndex(V.InfluenceBones[Influence]))++InvalidWeightedBones;
                    }
                    if(TotalWeight==0)++UnweightedVertices;
                    Attributes(V.Position);Attributes(V.TangentX);Attributes(V.TangentY);Attributes(V.TangentZ);Attributes(V.Color);
                    for(uint32 UV=0;UV<LOD.NumTexCoords;++UV)Attributes(V.UVs[UV]);
                    Hash.Update(reinterpret_cast<const uint8*>(V.InfluenceBones),sizeof(V.InfluenceBones));
                    Hash.Update(reinterpret_cast<const uint8*>(V.InfluenceWeights),sizeof(V.InfluenceWeights));
                    TArray<TPair<FName,uint16>,TInlineAllocator<12>> NamedWeights;
                    for(uint32 Influence=0;Influence<UE_ARRAY_COUNT(V.InfluenceWeights);++Influence)
                    {
                        if(!V.InfluenceWeights[Influence])continue;
                        if(!S.BoneMap.IsValidIndex(V.InfluenceBones[Influence]))
                            return TEXT("{\"success\":false,\"reason\":\"weighted section bone index invalid\"}");
                        const int32 Bone=S.BoneMap[V.InfluenceBones[Influence]];
                        if(Bone<0||Bone>=Mesh->GetRefSkeleton().GetNum())
                            return TEXT("{\"success\":false,\"reason\":\"weighted reference bone index invalid\"}");
                        NamedWeights.Emplace(Mesh->GetRefSkeleton().GetBoneName(Bone),V.InfluenceWeights[Influence]);
                    }
                    NamedWeights.Sort([](const auto& A,const auto& B){return A.Key==B.Key?A.Value<B.Value:A.Key.LexicalLess(B.Key);});
                    const int32 Influences=NamedWeights.Num();NamedSkinHash.Update(reinterpret_cast<const uint8*>(&Influences),sizeof(Influences));
                    for(const auto& Weight:NamedWeights)
                    {
                        const FTCHARToUTF8 Name(*Weight.Key.ToString());const int32 Size=Name.Length();
                        NamedSkinHash.Update(reinterpret_cast<const uint8*>(&Size),sizeof(Size));
                        NamedSkinHash.Update(reinterpret_cast<const uint8*>(Name.Get()),Size);
                        NamedSkinHash.Update(reinterpret_cast<const uint8*>(&Weight.Value),sizeof(Weight.Value));
                    }
                }
                auto Section=MakeShared<FJsonObject>();Section->SetNumberField(TEXT("section"),I);
                Section->SetNumberField(TEXT("materialIndex"),S.MaterialIndex);Section->SetNumberField(TEXT("triangles"),S.NumTriangles);
                Section->SetNumberField(TEXT("vertices"),S.NumVertices);Section->SetNumberField(TEXT("baseIndex"),S.BaseIndex);
                if(Mesh->GetMaterials().IsValidIndex(S.MaterialIndex))
                    Section->SetStringField(TEXT("importedMaterialSlotName"),Mesh->GetMaterials()[S.MaterialIndex].ImportedMaterialSlotName.ToString());
                Sections.Add(MakeShared<FJsonValueObject>(Section));
            }
            TArray<FSoftSkinVertex> Vertices;LOD.GetVertices(Vertices);
            int32 DegenerateTriangles=0,InvalidIndices=0;
            for(int32 I=0;I+2<LOD.IndexBuffer.Num();I+=3)
            {
                const uint32 A=LOD.IndexBuffer[I],B=LOD.IndexBuffer[I+1],C=LOD.IndexBuffer[I+2];
                if(A>=uint32(Vertices.Num())||B>=uint32(Vertices.Num())||C>=uint32(Vertices.Num())){++InvalidIndices;continue;}
                const FVector3f Cross=FVector3f::CrossProduct(Vertices[B].Position-Vertices[A].Position,Vertices[C].Position-Vertices[A].Position);
                if(Cross.SizeSquared()<1.e-12f)++DegenerateTriangles;
            }
            Item->SetNumberField(TEXT("nonFiniteVertices"),NonFiniteVertices);
            Item->SetNumberField(TEXT("zeroNormalVertices"),ZeroNormals);
            Item->SetNumberField(TEXT("unweightedVertices"),UnweightedVertices);
            Item->SetNumberField(TEXT("invalidWeightedBones"),InvalidWeightedBones);
            Item->SetNumberField(TEXT("degenerateTriangles"),DegenerateTriangles);
            Item->SetNumberField(TEXT("invalidTriangleIndices"),InvalidIndices);
            Item->SetArrayField(TEXT("sections"),Sections);LODs.Add(MakeShared<FJsonValueObject>(Item));
        }
        TArray<TSharedPtr<FJsonValue>> Rest;
        const FReferenceSkeleton& Skeleton=Mesh->GetRefSkeleton();
        for(int32 I=0;I<Skeleton.GetNum();++I)
        {
            const FTransform& Pose=Skeleton.GetRefBonePose()[I];
            const FVector Translation=Pose.GetTranslation(),Scale=Pose.GetScale3D();const FQuat Rotation=Pose.GetRotation();
            Bytes(Translation);Bytes(Scale);Bytes(Rotation);
            auto Bone=MakeShared<FJsonObject>();Bone->SetStringField(TEXT("name"),Skeleton.GetBoneName(I).ToString());
            Bone->SetNumberField(TEXT("parentIndex"),Skeleton.GetParentIndex(I));
            Bone->SetStringField(TEXT("referencePose"),Pose.ToString());Rest.Add(MakeShared<FJsonValueObject>(Bone));
        }
        Result->SetArrayField(TEXT("referenceSkeleton"),Rest);
        Hash.Final();uint8 Digest[FSHA1::DigestSize];Hash.GetHash(Digest);
        Result->SetStringField(TEXT("geometrySkinUVSectionSHA1"),BytesToHex(Digest,FSHA1::DigestSize));
        GeometryHash.Final();GeometryHash.GetHash(Digest);
        Result->SetStringField(TEXT("geometryAttributesSHA1"),BytesToHex(Digest,FSHA1::DigestSize));
        NamedSkinHash.Final();NamedSkinHash.GetHash(Digest);
        Result->SetStringField(TEXT("namedSkinWeightsSHA1"),BytesToHex(Digest,FSHA1::DigestSize));
        Result->SetArrayField(TEXT("lods"),LODs);Result->SetBoolField(TEXT("success"),true);
    }
#endif
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Result,Writer);return Text;
}

bool UWarfareFRRUAssetAuditV82::AppendVerifiedUnusedMaterialSlot(USkeletalMesh* Mesh,FName Name,int64 SourceTriangles)
{
#if WITH_EDITOR
    if(!Mesh||Name.IsNone()||SourceTriangles!=0)return false;
    const FString Path=Mesh->GetPathName();
    const bool DeclaredGerman=Path==TEXT("/Game/FRRUIntegration/FormalV82/Characters/German67/SK_DE_EarlyCap_N67_V82.SK_DE_EarlyCap_N67_V82")
        ||Path==TEXT("/Game/FRRUIntegration/FormalV82/Characters/German67/SK_DE_Late_N67_V82.SK_DE_Late_N67_V82");
    // Only these losslessly derived Adrian bodies may append proven unused
    // slots; the surrounding Integration namespace remains ineligible.
    static const TCHAR* const FrenchHeadgearNames[]={
        TEXT("SK_FR_1916_R3_NoWeapon"),TEXT("SK_fr_rifle_R3_Combined"),TEXT("SK_fr_machinegun_R3_Combined"),
        TEXT("SK_fr_assault_R3_Combined"),TEXT("SK_fr_marksman_R3_Combined"),TEXT("SK_fr_command_R3_Combined"),
        TEXT("SK_fr_flame_R3_Combined"),TEXT("SK_fr_engineer_R3_Combined"),TEXT("SK_fr_grenadier_R3_Combined"),
        TEXT("SK_fr_mobilegun_R3_Combined"),TEXT("SK_fr_signals_R3_Combined")};
    bool DeclaredFrenchHeadgear=false;
    for(const TCHAR* CandidateName:FrenchHeadgearNames)
        if(Path==FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Characters/FrenchHeadgearV827/%s.%s"),CandidateName,CandidateName))
        {DeclaredFrenchHeadgear=true;break;}
    // Only the17 lossless cap derivatives may restore their three proven
    // connected-but-unused source slots. This grants no surrounding namespace.
    static const TCHAR* const EasternHeadgearNames[]={
        TEXT("SK_RU_1914_R3"),TEXT("SK_AH_1914_R3"),TEXT("SK_AH_1916_R3"),
        TEXT("SK_ru_rifle_R3_Combined"),TEXT("SK_ru_machinegun_R3_Combined"),
        TEXT("SK_ru_assault_R3_Combined"),TEXT("SK_ru_marksman_R3_Combined"),TEXT("SK_ru_command_R3_Combined"),
        TEXT("SK_ru_engineer_R3_Combined"),TEXT("SK_ru_signals_R3_Combined"),
        TEXT("SK_at_hu_rifle_R3_Combined"),TEXT("SK_at_hu_machinegun_R3_Combined"),
        TEXT("SK_at_hu_assault_R3_Combined"),TEXT("SK_at_hu_marksman_R3_Combined"),TEXT("SK_at_hu_command_R3_Combined"),
        TEXT("SK_at_hu_engineer_R3_Combined"),TEXT("SK_at_hu_signals_R3_Combined")};
    bool DeclaredEasternHeadgear=false;
    for(const TCHAR* CandidateName:EasternHeadgearNames)
        if(Path==FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Characters/EasternHeadgearV87/%s.%s"),CandidateName,CandidateName))
        {DeclaredEasternHeadgear=true;break;}
    if(DeclaredEasternHeadgear)
    {
        const bool DeclaredUnusedName=Name==FName(TEXT("M_V07_Gear"))||Name==FName(TEXT("M_V16_GermanCanvas"))||Name==FName(TEXT("M_V16_MapLeather"));
        const FSkeletalMeshModel* Model=Mesh->GetImportedModel();
        if(!DeclaredUnusedName||!Model||Model->LODModels.IsEmpty()||Mesh->GetRefSkeleton().GetNum()!=67||Mesh->GetRefSkeleton().FindBoneIndex(FName(TEXT("Head")))==INDEX_NONE
            ||!Mesh->GetSkeleton()||Mesh->GetSkeleton()->GetPathName()!=TEXT("/Game/FRRUExpansionR2/FormalV82/Dependencies/SKEL_Human67_V82.SKEL_Human67_V82"))return false;
    }
    if(!Path.StartsWith(TEXT("/Game/FRRUExpansionR2/FormalV82/Characters/"))
        &&!Path.StartsWith(TEXT("/Game/FRRUExpansionR3/FormalV82/Characters/"))&&!DeclaredGerman&&!DeclaredFrenchHeadgear&&!DeclaredEasternHeadgear)return false;
    for(const auto& Slot:Mesh->GetMaterials())
        if(Slot.MaterialSlotName==Name||Slot.ImportedMaterialSlotName==Name)return false;
    const FString Before=SnapshotSkeletalMesh(Mesh);
    TArray<FSkeletalMaterial> Materials=Mesh->GetMaterials();
    Materials.Emplace(nullptr,true,false,Name,Name);
    Mesh->Modify();Mesh->SetMaterials(Materials);Mesh->MarkPackageDirty();
    return SnapshotSkeletalMesh(Mesh)==Before;
#else
    return false;
#endif
}
