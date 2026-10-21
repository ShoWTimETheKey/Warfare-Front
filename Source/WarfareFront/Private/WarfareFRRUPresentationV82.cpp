#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "FRRU/Native/Expansion.h"
#include "FRRU/Native/UE/ExpansionArchiveUE.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCompressionTypes.h"
#include "HAL/IConsoleManager.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AttributesRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Misc/MemStack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "StaticMeshResources.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif
#include "RHI.h"
#include "UObject/StrongObjectPtr.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Algo/AllOf.h"

// Presentation only: preserve the soldier's private gameplay animation state.
struct FFRRUPresentationAccessV82
{
    static void SetMotion(AWarfareFrontlineSoldier* Soldier,UAnimationAsset* Asset,bool Loop=false)
    {Soldier->SetMotion(Asset,Loop);}
    static void RestartIdle(AWarfareFrontlineSoldier* Soldier)
    {Soldier->Playing=nullptr;Soldier->SetMotion(Soldier->GetIdleMotion());}
};

namespace WarfareFRRUPresentationV82
{
namespace
{
const TCHAR* SkeletonURI=TEXT("/Game/FRRUExpansionR2/FormalV82/Dependencies/SKEL_Human67_V82.SKEL_Human67_V82");
const TCHAR* R3Animations=TEXT("/Game/FRRUExpansionR3/FormalV82/Animations/");
const TCHAR* Legacy67=TEXT("/Game/FRRUIntegration/FormalV82/Motions/Legacy67/");
const TCHAR* National67=TEXT("/Game/FRRUIntegration/FormalV82/Motions/National67/");
const TCHAR* German67=TEXT("/Game/FRRUIntegration/FormalV82/Motions/German67/");
const TCHAR* FrenchVB67=TEXT("/Game/FRRUIntegration/FormalV82/Motions/FrenchVB67/");
const TCHAR* FrenchVBWeapons67=TEXT("/Game/FRRUIntegration/FormalV82/Weapons/FrenchVB67/");
const TCHAR* OriginalBritishFace=TEXT("/Game/Warfare/Characters/V16/Materials/M_V16_Face_British_Rifle.M_V16_Face_British_Rifle");
const TCHAR* OriginalGermanFace=TEXT("/Game/Warfare/Characters/V16/Materials/M_V16_Face_German_Rifle.M_V16_Face_German_Rifle");
TMap<FString,TStrongObjectPtr<UObject>> Assets;
bool Reject(FString& Error,const FString& Reason){Error=Reason;return false;}
template<class T> T* Asset(const FString& URI)
{
    if(const auto* Found=Assets.Find(URI))return Cast<T>(Found->Get());
    T* Loaded=LoadObject<T>(nullptr,*URI);if(Loaded)Assets.Add(URI,TStrongObjectPtr<UObject>(Loaded));return Loaded;
}
FString URI(const TCHAR* Folder,const FString& Name){return FString(Folder)+Name+TEXT(".")+Name;}
FString Token(FString Id){Id.ReplaceInline(TEXT("."),TEXT("_"));Id.ReplaceInline(TEXT("-"),TEXT("_"));return Id;}
bool ShaderReady(UMaterialInterface* Material)
{
    const FMaterialResource* R=Material?Material->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
    const FMaterialShaderMap* Map=R?R->GetGameThreadShaderMap():nullptr;
#if WITH_EDITOR
    if(Material&&(!R||!R->IsGameThreadShaderMapComplete()||!Map||!Map->IsValidForRendering()))
    {
        // An instance without its own static permutation delegates to its
        // parent resource. Caching only the leaf cannot compile that base.
        TArray<UMaterialInterface*> Chain;
        for(UMaterialInterface* Current=Material;Current;)
        {
            if(Chain.Contains(Current))return false;
            Chain.Add(Current);const auto* Instance=Cast<UMaterialInstance>(Current);
            Current=Instance?Instance->Parent.Get():nullptr;
        }
        if(auto* Base=Material->GetMaterial())Chain.AddUnique(Base);
        for(int32 I=Chain.Num()-1;I>=0;--I)Chain[I]->CacheShaders(EMaterialShaderPrecompileMode::Synchronous);
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        R=Material->GetMaterialResource(GMaxRHIShaderPlatform);
        Map=R?R->GetGameThreadShaderMap():nullptr;
    }
#endif
    return R&&R->IsGameThreadShaderMapComplete()&&Map&&Map->IsValidForRendering();
}
bool StaticReady(UStaticMesh* Mesh,FString& Error)
{
    const auto* Render=Mesh?Mesh->GetRenderData():nullptr;
    if(!Render||Render->LODResources.IsEmpty()||Render->LODResources[0].GetNumVertices()==0
        ||Render->LODResources[0].GetNumTriangles()==0)return Reject(Error,TEXT("missing actual formal static render geometry"));
    for(int32 LOD=0;LOD<Render->LODResources.Num();++LOD)
        for(const auto& Section:Render->LODResources[LOD].Sections)
        {
            if(Section.NumTriangles==0)continue;
            const int32 Index=Section.MaterialIndex;
            if(!Mesh->GetStaticMaterials().IsValidIndex(Index))
                return Reject(Error,FString::Printf(TEXT("static visible section has invalid material index: %s LOD%d slot%d"),*Mesh->GetPathName(),LOD,Index));
            const FStaticMaterial& Slot=Mesh->GetStaticMaterials()[Index];
            if(!ShaderReady(Slot.MaterialInterface))
                return Reject(Error,FString::Printf(TEXT("static visible shader missing/incomplete: %s LOD%d slot%d[%s] material=%s"),
                    *Mesh->GetPathName(),LOD,Index,*Slot.MaterialSlotName.ToString(),
                    Slot.MaterialInterface?*Slot.MaterialInterface->GetPathName():TEXT("NULL")));
        }
    return true;
}
bool HumanReady(USkeletalMesh* Mesh,USkeleton* Skeleton,bool Physics,FString& Error)
{
    const auto* Render=Mesh?Mesh->GetResourceForRendering():nullptr;
    if(!Mesh||!Skeleton||Mesh->GetSkeleton()!=Skeleton||!Render||Render->LODRenderData.IsEmpty()
        ||Render->LODRenderData[0].GetNumVertices()==0||Render->LODRenderData[0].GetTotalFaces()==0)
        return Reject(Error,TEXT("missing actual formal human render mesh/Skeleton binding"));
    const auto& A=Mesh->GetRefSkeleton();const auto& B=Skeleton->GetReferenceSkeleton();
    if(A.GetNum()!=67||B.GetNum()!=67)return Reject(Error,TEXT("human presentation requires actual complete67 reference bones"));
    for(int32 I=0;I<67;++I)
        if(A.GetBoneName(I)!=B.GetBoneName(I)||A.GetParentIndex(I)!=B.GetParentIndex(I)
            ||!A.GetRefBonePose()[I].Equals(B.GetRefBonePose()[I],.001f))
            return Reject(Error,TEXT("actual full67 hierarchy/reference transforms differ"));
    // Imported zero-face metadata slots are retained for the frozen source
    // contract. Only actual positive-face render sections consume a shader.
    for(int32 LOD=0;LOD<Render->LODRenderData.Num();++LOD)
        for(const auto& Section:Render->LODRenderData[LOD].RenderSections)
        {
            if(Section.NumTriangles==0)continue;
            const int32 Index=Section.MaterialIndex;
            if(!Mesh->GetMaterials().IsValidIndex(Index))
                return Reject(Error,FString::Printf(TEXT("human visible section has invalid material index: %s LOD%d slot%d"),*Mesh->GetPathName(),LOD,Index));
            const FSkeletalMaterial& Slot=Mesh->GetMaterials()[Index];
            auto* Material=ResolveMaterial(Slot.MaterialInterface);
            if(!Material||!Material->GetUsageByFlag(MATUSAGE_SkeletalMesh))
                return Reject(Error,FString::Printf(TEXT("human visible skeletal usage missing: %s LOD%d slot%d[%s] material=%s"),
                    *Mesh->GetPathName(),LOD,Index,*Slot.MaterialSlotName.ToString(),Material?*Material->GetPathName():TEXT("NULL")));
            const FString Original=Slot.MaterialInterface->GetPathName();
            if((Original==OriginalBritishFace||Original==OriginalGermanFace)&&!Material->GetUsageByFlag(MATUSAGE_MorphTargets))
                return Reject(Error,TEXT("declared national visible face lacks actual MorphTargets usage: ")+Material->GetPathName());
            if(!ShaderReady(Material))
                return Reject(Error,FString::Printf(TEXT("human visible shader missing/incomplete: %s LOD%d slot%d[%s] material=%s"),
                    *Mesh->GetPathName(),LOD,Index,*Slot.MaterialSlotName.ToString(),
                    *Material->GetPathName()));
        }
    if(Physics)
    {
        const auto* P=Mesh->GetPhysicsAsset();
        const FString GermanBase=TEXT("/Game/FRRUIntegration/FormalV82/Characters/German67/");
        const FString GermanName=Mesh->GetName();
        const bool DeclaredGerman=(GermanName==TEXT("SK_DE_EarlyCap_N67_V82")||GermanName==TEXT("SK_DE_Late_N67_V82"))
            &&Mesh->GetPathName()==GermanBase+GermanName+TEXT(".")+GermanName
            &&P&&P->GetPathName()==GermanBase+GermanName+TEXT("_PhysicsAsset.")+GermanName+TEXT("_PhysicsAsset");
        if(!P||(!P->GetPathName().StartsWith(TEXT("/Game/FRRUExpansionR3/FormalV82/"))&&!DeclaredGerman)||P->SkeletalBodySetups.IsEmpty())
            return Reject(Error,TEXT("missing actual bound formal67 PhysicsAsset"));
        for(const auto& Entry:P->SkeletalBodySetups)
        {const auto* Body=Entry.Get();if(!Body||A.FindBoneIndex(Body->BoneName)==INDEX_NONE||Body->AggGeom.GetElementCount()==0)
            return Reject(Error,TEXT("PhysicsAsset contains foreign/missing bone or empty actual body"));}
        for(const auto& Entry:P->ConstraintSetup)
        {const auto* C=Entry.Get();if(!C||A.FindBoneIndex(C->DefaultInstance.ConstraintBone1)==INDEX_NONE
            ||A.FindBoneIndex(C->DefaultInstance.ConstraintBone2)==INDEX_NONE)return Reject(Error,TEXT("PhysicsAsset constraint references foreign/missing bone"));}
    }
    return true;
}
bool MechanicalReady(USkeletalMesh* Mesh,const FString& Id,FString& Error)
{
    const auto* Render=Mesh?Mesh->GetResourceForRendering():nullptr;
    if(!Mesh||!Mesh->GetSkeleton()||!Render||Render->LODRenderData.IsEmpty()
        ||Render->LODRenderData[0].GetNumVertices()==0||Render->LODRenderData[0].GetTotalFaces()==0)
        return Reject(Error,TEXT("exact mechanical render mesh/Skeleton missing: ")+Id);
    for(int32 LODIndex=0;LODIndex<Render->LODRenderData.Num();++LODIndex)
        for(const auto& Section:Render->LODRenderData[LODIndex].RenderSections)if(Section.NumTriangles>0)
    {
        const int32 Index=Section.MaterialIndex;
        if(!Mesh->GetMaterials().IsValidIndex(Index))
            return Reject(Error,FString::Printf(TEXT("mechanical visible material index invalid: weapon=%s mesh=%s LOD%d slot%d triangles=%u"),
                *Id,*Mesh->GetPathName(),LODIndex,Index,Section.NumTriangles));
        const auto& Slot=Mesh->GetMaterials()[Index];auto* Original=Slot.MaterialInterface.Get();auto* Material=ResolveMaterial(Original);
        const bool Usage=Material&&Material->GetUsageByFlag(MATUSAGE_SkeletalMesh);
        const bool Ready=Usage&&ShaderReady(Material);
        if(!Ready)
        {
            const auto* Base=Material?Material->GetMaterial():nullptr;
            const auto* Resource=Material?Material->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
            const auto* Map=Resource?Resource->GetGameThreadShaderMap():nullptr;
            return Reject(Error,FString::Printf(TEXT("mechanical actual visible skeletal shader unavailable: weapon=%s mesh=%s Skeleton=%s LOD%d slot%d[%s] triangles=%u sourceMaterial=%s material=%s class=%s base=%s skeletalUsage=%d baseSkeletalUsage=%d resource=%d map=%d mapComplete=%d mapValid=%d platform=%d"),
                *Id,*Mesh->GetPathName(),*Mesh->GetSkeleton()->GetPathName(),LODIndex,Index,*Slot.MaterialSlotName.ToString(),Section.NumTriangles,
                Original?*Original->GetPathName():TEXT("NULL"),Material?*Material->GetPathName():TEXT("NULL"),Material?*Material->GetClass()->GetName():TEXT("NULL"),Base?*Base->GetPathName():TEXT("NULL"),
                Usage,Base&&Base->GetUsageByFlag(MATUSAGE_SkeletalMesh),Resource!=nullptr,Map!=nullptr,
                Resource&&Resource->IsGameThreadShaderMapComplete(),Map&&Map->IsValidForRendering(),int32(GMaxRHIShaderPlatform)));
        }
    }
    return true;
}
bool CompressedReady(UAnimSequence* Sequence)
{
#if WITH_EDITOR
    if(Sequence&&!Sequence->IsCompressedDataValid())
    {
        Sequence->CacheDerivedDataForCurrentPlatform();
        Sequence->WaitOnExistingCompression(true);
    }
#endif
    return Sequence&&Sequence->IsCompressedDataValid();
}
UAnimSequence* Clip(const FString& Path,USkeleton* Skeleton,FString& Error)
{
    auto* Sequence=Asset<UAnimSequence>(Path);
    if(!Sequence||Sequence->GetSkeleton()!=Skeleton||Sequence->GetPlayLength()<=0||Sequence->IsValidAdditive()
        ||!CompressedReady(Sequence))
    {Reject(Error,TEXT("missing/non67/invalid actual compressed sequence: ")+Path);return nullptr;}
    return Sequence;
}
bool ComponentPose(UAnimSequence* ClipAsset,USkeletalMesh* Mesh,float Time,TArray<FTransform>& Out)
{
    if(!ClipAsset||!Mesh||ClipAsset->GetSkeleton()!=Mesh->GetSkeleton()||!ClipAsset->IsCompressedDataValid())return false;
    FMemMark Mark(FMemStack::Get());const auto& Ref=Mesh->GetRefSkeleton();TArray<FBoneIndexType> Indices;
    for(int32 I=0;I<Ref.GetNum();++I)Indices.Add(FBoneIndexType(I));
    FBoneContainer Bones(Indices,UE::Anim::FCurveFilterSettings(),*Mesh);
    FCompactPose Pose;Pose.SetBoneContainer(&Bones);Pose.ResetToRefPose();FBlendedCurve Curve;Curve.InitFrom(Bones);
    UE::Anim::FStackAttributeContainer Attributes;FAnimationPoseData Data(Pose,Curve,Attributes);
    FAnimExtractContext Context(double(Time),false);
#if WITH_EDITOR
    Context.bEnforceCompressedDataSampling=true;
#endif
    {
        const auto Compressed=ClipAsset->GetCompressedData(Context);
        if(!Compressed.Get().IsValid(ClipAsset)||ClipAsset->GetSkeletonVirtualBoneGuid()!=ClipAsset->GetSkeleton()->GetVirtualBoneGuid()
            ||Bones.GetDisableRetargeting()||Bones.ShouldUseRawData()||Bones.ShouldUseSourceData())return false;
#if WITH_EDITOR
        const auto* ForceRaw=IConsoleManager::Get().FindConsoleVariable(TEXT("a.ForceEvalRawData"));
        if(ForceRaw&&ForceRaw->GetInt()==1)return false;
#endif
    }
    ClipAsset->GetAnimationPose(Data,Context);Out.SetNum(Ref.GetNum());
    for(int32 I=0;I<Ref.GetNum();++I)
    {const FTransform Local=Pose[Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(I))];if(Local.ContainsNaN())return false;
        const int32 Parent=Ref.GetParentIndex(I);Out[I]=Parent==INDEX_NONE?Local:Local*Out[Parent];}
    return true;
}
bool ReferenceComponents(USkeletalMesh* Mesh,TArray<FTransform>& Out)
{
    if(!Mesh)return false;const auto& Ref=Mesh->GetRefSkeleton();Out.SetNum(Ref.GetNum());
    for(int32 I=0;I<Ref.GetNum();++I)
    {
        const auto& Local=Ref.GetRefBonePose()[I];const int32 Parent=Ref.GetParentIndex(I);
        if(Local.ContainsNaN()||!Local.GetScale3D().Equals(FVector::OneVector,.0001f)||Parent<INDEX_NONE||Parent>=I)return false;
        // UE applies the local transform first, then its complete parent.
        Out[I]=Parent==INDEX_NONE?Local:Local*Out[Parent];
    }
    return !Out.IsEmpty();
}
struct FBinding
{
    FString Key,WeaponId,VisualWeaponId,Kind,BodyURI,KitURI,WorkingKitURI;
    USkeleton* Skeleton=nullptr;USkeletalMesh* Body=nullptr;USkeletalMesh* Kit=nullptr;USkeletalMesh* WorkingKit=nullptr;
    UStaticMesh* Weapon=nullptr;UStaticMesh* WireTool=nullptr;UStaticMesh* Projectile=nullptr;USkeletalMesh* Pair=nullptr;UAnimSequence* PairLoad=nullptr;
    USkeletalMesh* Mechanism=nullptr;UAnimSequence* Feed=nullptr;TMap<FString,UAnimSequence*> MechanismMotions;
    USkeletalMesh* BoltMesh=nullptr;FName BoltBone;FTransform HumanBoltRest=FTransform::Identity,MechanicalBoltRest=FTransform::Identity;
    FVector MuzzleLocal=FVector::ZeroVector;TMap<FString,UAnimSequence*> Motions;
    int32 ReloadRepeats=1;
};
struct FUnitState
{
    FBinding Binding;TWeakObjectPtr<UStaticMeshComponent> Weapon,LoadedGrenade;TWeakObjectPtr<USkeletalMeshComponent> Kit,Pair,WorkingKit,Mechanism;
    bool WasReload=false,ReleasedWeaponV87=false,ReleasedEmplacedV87=false;float LastReloadPhase=0;int32 ReloadCycles=0,CancelledReloads=0,CompletedAtReloadStart=0;
    bool WasWorking=false;float WorkPhase=0;TWeakObjectPtr<UStaticMeshComponent> WireTools[2];
    bool WasTrain=false;FRotator BeforeTrainMeshRotation=FRotator::ZeroRotator;
    TWeakObjectPtr<UPoseableMeshComponent> BoltRig;FDelegateHandle BoltFinalized;
    int32 BoltPoseFrames=0,BoltRejectedFrames=0;bool BoltVisible=false;
    float MaximumBoltPoseErrorCm=0,MaximumBoltTravelCm=0;
    FTransform LastHumanBoltDelta=FTransform::Identity,LastMechanicalBolt=FTransform::Identity;
};
TMap<FString,FBinding> Bindings;
TMap<TWeakObjectPtr<const AWarfareFrontlineSoldier>,FUnitState> Units;
bool PrepareOrdinaryBolt(FBinding& Binding,FString& Error)
{
    const FString Id=Binding.VisualWeaponId;const bool G98=Id==TEXT("de.gewehr98");
    if(!G98&&Id!=TEXT("fr.lebel")&&Id!=TEXT("ru.mosin")&&Id!=TEXT("at-hu.m95"))return true;
    Binding.BoltMesh=G98?Binding.Pair:Asset<USkeletalMesh>(URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Weapons/Mechanical/"),TEXT("SK_R2_")+Token(Id)));
    Binding.BoltBone=G98?TEXT("bolt"):TEXT("weapon_bolt");
    auto* Mesh=Binding.BoltMesh;const auto* Render=Mesh?Mesh->GetResourceForRendering():nullptr;
    if(!Mesh||!Mesh->GetSkeleton()||!Render||Render->LODRenderData.IsEmpty()||Render->LODRenderData[0].GetNumVertices()==0)
        return Reject(Error,TEXT("exact ordinary bolt mechanism render mesh missing: ")+Id);
    const auto& Ref=Mesh->GetRefSkeleton();const auto& SkeletonRef=Mesh->GetSkeleton()->GetReferenceSkeleton();
    const TArray<FName> Names=G98?TArray<FName>{TEXT("weapon_root"),TEXT("bolt"),TEXT("charger"),TEXT("round_0"),TEXT("round_1"),TEXT("round_2"),TEXT("round_3"),TEXT("round_4")}
        :TArray<FName>{TEXT("root"),TEXT("weapon_body"),TEXT("weapon_bolt"),TEXT("feed"),TEXT("top_cover")};
    if(Ref.GetNum()!=Names.Num()||SkeletonRef.GetNum()!=Names.Num())return Reject(Error,TEXT("ordinary bolt exact bone count differs: ")+Id);
    for(int32 I=0;I<Names.Num();++I)
    {
        const int32 Parent=I==0?INDEX_NONE:G98?0:I==1?0:1;
        if(Ref.GetBoneName(I)!=Names[I]||Ref.GetParentIndex(I)!=Parent||SkeletonRef.GetBoneName(I)!=Names[I]
            ||SkeletonRef.GetParentIndex(I)!=Parent||!Ref.GetRefBonePose()[I].Equals(SkeletonRef.GetRefBonePose()[I],.001f))
            return Reject(Error,TEXT("ordinary bolt exact names/parents/reference binding differs: ")+Id);
    }
    if(!MechanicalReady(Mesh,Id,Error))return false;
    TArray<FTransform> Human,Mechanical;if(!ReferenceComponents(Binding.Body,Human)||!ReferenceComponents(Mesh,Mechanical))
        return Reject(Error,TEXT("ordinary bolt reference composition invalid: ")+Id);
    const auto& HRef=Binding.Body->GetRefSkeleton();const int32 W=HRef.FindBoneIndex(TEXT("weapon_rifle")),H=HRef.FindBoneIndex(TEXT("weapon_bolt"));
    const int32 M=Ref.FindBoneIndex(Binding.BoltBone);
    if(!Human.IsValidIndex(W)||!Human.IsValidIndex(H)||!Mechanical.IsValidIndex(M)||HRef.GetParentIndex(H)!=W)
        return Reject(Error,TEXT("ordinary bolt human/mechanical pivot hierarchy invalid: ")+Id);
    Binding.HumanBoltRest=Human[H].GetRelativeTransform(Human[W]);Binding.MechanicalBoltRest=Mechanical[M];return true;
}
void UpdateOrdinaryBolt(AWarfareFrontlineSoldier* Soldier)
{
    auto* State=Soldier?Units.Find(Soldier):nullptr;if(!State||State->ReleasedWeaponV87||!State->BoltRig.IsValid()||!State->Weapon.IsValid())return;
    auto* Rig=State->BoltRig.Get();const bool Active=Soldier->IsAlive()&&Soldier->WeaponPhase==EFrontlineWeaponPhase::BoltCycle
        &&!WarfareFRRUArmourV82::Embarked(Soldier)&&Soldier->Soldier&&Soldier->Soldier->GetSkeletalMeshAsset()==State->Binding.Body;
    if(!Active)
    {
        Rig->SetVisibility(false);State->BoltVisible=false;
        State->Weapon->SetVisibility(!State->WasReload);return;
    }
    // Called after the BODY's bone finalization, never from a prior actor tick.
    const FTransform W=Soldier->Soldier->GetBoneTransform(TEXT("weapon_rifle"),RTS_Component);
    const FTransform H=Soldier->Soldier->GetBoneTransform(TEXT("weapon_bolt"),RTS_Component).GetRelativeTransform(W);
    // Matrix form: Delta = H_live * inverse(H_ref); M_live = Delta * M_ref.
    // FTransform multiplication uses the opposite written matrix order.
    const FTransform Delta=State->Binding.HumanBoltRest.Inverse()*H;
    const FTransform Wanted=State->Binding.MechanicalBoltRest*Delta;
    const auto RejectPose=[&]()
    {
        ++State->BoltRejectedFrames;Rig->SetVisibility(false);State->BoltVisible=false;State->Weapon->SetVisibility(false);
        if(Soldier->Battle)Soldier->Battle->AssetFailure=TEXT("Actual national bolt pose failed: ")+State->Binding.VisualWeaponId;
    };
    if(H.ContainsNaN()||Delta.ContainsNaN()||Wanted.ContainsNaN()||!Delta.GetScale3D().Equals(FVector::OneVector,.001f)
        ||Delta.GetTranslation().Size()>20.f||Delta.GetRotation().AngularDistance(FQuat::Identity)>FMath::DegreesToRadians(140.f))
    {RejectPose();return;}
    Rig->SetWorldTransform(Soldier->Soldier->GetSocketTransform(TEXT("weapon_rifle"),RTS_World));
    Rig->SetBoneTransformByName(State->Binding.BoltBone,Wanted,EBoneSpaces::ComponentSpace);Rig->RefreshBoneTransforms();
    const FTransform Actual=Rig->GetBoneTransformByName(State->Binding.BoltBone,EBoneSpaces::ComponentSpace);
    float Error=0;for(const FVector Point:{FVector::ZeroVector,FVector(10,0,0),FVector(0,10,0),FVector(0,0,10)})
        Error=FMath::Max(Error,float(FVector::Dist(Actual.TransformPosition(Point),Wanted.TransformPosition(Point))));
    if(Actual.ContainsNaN()||!FMath::IsFinite(Error)||Error>.02f){RejectPose();return;}
    ++State->BoltPoseFrames;State->MaximumBoltPoseErrorCm=FMath::Max(State->MaximumBoltPoseErrorCm,Error);
    State->MaximumBoltTravelCm=FMath::Max(State->MaximumBoltTravelCm,float(FVector::Dist(Actual.GetTranslation(),State->Binding.MechanicalBoltRest.GetTranslation())));
    State->LastHumanBoltDelta=Delta;State->LastMechanicalBolt=Actual;State->BoltVisible=true;
    Rig->SetVisibility(true);State->Weapon->SetVisibility(false);
}
FString VisualWeapon(FString Id)
{
    if(Id==TEXT("fr.marksman")||Id==TEXT("fr.command"))return TEXT("fr.lebel");
    if(Id==TEXT("ru.marksman")||Id==TEXT("ru.command"))return TEXT("ru.mosin");
    if(Id==TEXT("at-hu.marksman")||Id==TEXT("at-hu.command"))return TEXT("at-hu.m95");
    if(Id==TEXT("de.marksman")||Id==TEXT("de.command"))return TEXT("de.gewehr98");
    return Id;
}
FVector AuthoredMuzzle(const FString& Id)
{
    // Receiver copy of Refined weapons-r2-manifest: weapon_rifle local cm,
    // forward -Y/up Z. Never vehicle +X or an old Lee-Enfield gun socket.
    if(Id==TEXT("fr.lebel")||Id==TEXT("ru.mosin"))return FVector(0,-94,3.8);
    if(Id==TEXT("fr.berthier"))return FVector(0,-93,3.8);
    if(Id==TEXT("at-hu.m95"))return FVector(0,-90,3.8);
    if(Id==TEXT("fr.hotchkiss"))return FVector(0,-94,5);
    if(Id==TEXT("fr.chauchat"))return FVector(0,-85,4);
    if(Id==TEXT("ru.maxim")||Id==TEXT("de.mg08"))return FVector(0,-78,5);
    if(Id==TEXT("at-hu.schwarzlose"))return FVector(0,-67,5);
    if(Id==TEXT("fr.m1892")||Id==TEXT("ru.nagant"))return FVector(0,-18,4);
    if(Id==TEXT("fr.vb"))return FVector(0,-99,3.8);
    if(Id==TEXT("fr.schilt"))return FVector(0,-70,2);
    if(Id==TEXT("de.gewehr98"))return FVector(0,-92,4.4);
    if(Id==TEXT("de.mg0815"))return FVector(0,-108.6,5.2);
    return FVector::ZeroVector;
}
const wfexp::Weapon* NativeWeapon(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side,int32 Member)
{
    return B&&WarfareFRRUCampaignV82::Active(B)?wfexp::WeaponFor(static_cast<wfexp::Faction>(uint8(WarfareFRRUCampaignV82::Nation(B,Side))),
        int32(Role),Member,WarfareFRRUCampaignV82::CalendarDate(B)):nullptr;
}
bool BuildBinding(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side,int32 Member,FBinding& Out,FString& Error)
{
    if(!B||!WarfareFRRUEquipmentV82::RoleAvailable(B,Role,Side))return Reject(Error,TEXT("national role/date unavailable"));
    const auto* W=NativeWeapon(B,Role,Side,Member);if(!W)return Reject(Error,TEXT("actual member weapon/date unavailable"));
    const auto Nation=WarfareFRRUCampaignV82::Nation(B,Side);const int32 Date=WarfareFRRUCampaignV82::CalendarDate(B);
    const bool Lead=Member==0;const FString Kind=wfexpue::Wide(W->kind),WeaponId=wfexpue::Wide(W->id);
    const FString Key=FString::Printf(TEXT("%d|%d|%d|%d|%s"),int32(Nation),Date,int32(Role),Lead,*WeaponId);
    if(const auto* Found=Bindings.Find(Key)){Out=*Found;return true;}
    Out={};Out.Key=Key;Out.WeaponId=WeaponId;Out.VisualWeaponId=VisualWeapon(WeaponId);Out.Kind=Kind;
    const bool German=Nation==EFRRUNationV81::German;
    if(!German&&Nation!=EFRRUNationV81::French&&Nation!=EFRRUNationV81::Russian&&Nation!=EFRRUNationV81::AustroHungarian)
        return Reject(Error,TEXT("no formal national67 presentation contract"));
    const FString Prefix=German?TEXT("de"):Nation==EFRRUNationV81::French?TEXT("fr"):Nation==EFRRUNationV81::Russian?TEXT("ru"):TEXT("at_hu");
    static const TCHAR* RoleTokens[]={TEXT("rifle"),TEXT("machinegun"),TEXT("assault"),TEXT("marksman"),TEXT("command"),TEXT("flame"),TEXT("engineer"),TEXT("grenadier"),TEXT("invalid"),TEXT("mobilegun"),TEXT("signals")};
    if(int32(Role)<0||int32(Role)>=UE_ARRAY_COUNT(RoleTokens))return Reject(Error,TEXT("unknown native presentation role"));
    const FString RoleToken=Lead?RoleTokens[int32(Role)]:TEXT("rifle");
    if(Nation==EFRRUNationV81::French&&Date<19160101)return Reject(Error,TEXT("French early dated role equipment composition pending"));
    // Russian cap remains default on every current Eastern mission. The Adrian
    // candidate is never selected by a broad calendar >=1916 entitlement.
    if(German)
        Out.BodyURI=URI(TEXT("/Game/FRRUIntegration/FormalV82/Characters/German67/"),Date<19160101?TEXT("SK_DE_EarlyCap_N67_V82"):TEXT("SK_DE_Late_N67_V82"));
    else if(Nation==EFRRUNationV81::French)
        Out.BodyURI=URI(TEXT("/Game/FRRUIntegration/FormalV82/Characters/FrenchHeadgearV827/"),TEXT("SK_fr_")+RoleToken+TEXT("_R3_Combined"));
    else if(Nation==EFRRUNationV81::AustroHungarian&&Date<19160101)
    {
        Out.BodyURI=URI(TEXT("/Game/FRRUIntegration/FormalV82/Characters/EasternHeadgearV87/"),TEXT("SK_AH_1914_R3"));
        Out.KitURI=URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Characters/RoleKits/"),TEXT("SK_")+Prefix+TEXT("_")+RoleToken+TEXT("_R2_Kit"));
    }
    else Out.BodyURI=URI(TEXT("/Game/FRRUIntegration/FormalV82/Characters/EasternHeadgearV87/"),TEXT("SK_")+Prefix+TEXT("_")+RoleToken+TEXT("_R3_Combined"));
    Out.Skeleton=Asset<USkeleton>(SkeletonURI);Out.Body=Asset<USkeletalMesh>(Out.BodyURI);
    if(!HumanReady(Out.Body,Out.Skeleton,true,Error))return false;
    if(!Out.KitURI.IsEmpty()){Out.Kit=Asset<USkeletalMesh>(Out.KitURI);if(!HumanReady(Out.Kit,Out.Skeleton,false,Error))return false;}
    if(!German&&Lead&&(Role==EFrontlineRole::Pioneer||Role==EFrontlineRole::Signals))
    {
        Out.WorkingKitURI=URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Characters/WorkingKits/"),TEXT("SK_R2_Work_")+Prefix
            +(Role==EFrontlineRole::Pioneer?TEXT("_engineer"):TEXT("_signals")));
        Out.WorkingKit=Asset<USkeletalMesh>(Out.WorkingKitURI);if(!HumanReady(Out.WorkingKit,Out.Skeleton,false,Error))return false;
    }
    const FString T=Token(Out.VisualWeaponId);
    const bool FrenchVB=Out.VisualWeaponId==TEXT("fr.vb");
    Out.Weapon=Asset<UStaticMesh>(FrenchVB?URI(FrenchVBWeapons67,TEXT("SM_FR_VB_Cup_N67_V82")):Out.VisualWeaponId==TEXT("de.gewehr98")||Out.VisualWeaponId==TEXT("de.mg0815")
        ?URI(TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/"),Out.VisualWeaponId==TEXT("de.gewehr98")?TEXT("SM_DE_G98_N67_V82"):TEXT("SM_DE_MG0815_Carry_N67_V82"))
        :URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Weapons/Static/"),TEXT("SM_R2_")+T));
    if(!StaticReady(Out.Weapon,Error))return false;
    Out.MuzzleLocal=AuthoredMuzzle(Out.VisualWeaponId);if(Out.MuzzleLocal.IsNearlyZero())return Reject(Error,TEXT("national muzzle coordinate contract missing"));
    const auto Add=[&](const TCHAR* Semantic,const FString& Name,bool Legacy=false)
    {
        auto* Sequence=Clip(URI(Legacy?Legacy67:R3Animations,Name+(Legacy?TEXT("_Human67_V82"):TEXT(""))),Out.Skeleton,Error);
        if(Sequence)Out.Motions.Add(Semantic,Sequence);return Sequence!=nullptr;
    };
    const auto AddNational=[&](const FString& Semantic,const FString& Name)
    {auto* Sequence=Clip(URI(National67,Name),Out.Skeleton,Error);if(Sequence)Out.Motions.Add(Semantic,Sequence);return Sequence!=nullptr;};
    const auto AddGerman=[&](const FString& Semantic,const FString& Name)
    {auto* Sequence=Clip(URI(German67,Name),Out.Skeleton,Error);if(Sequence)Out.Motions.Add(Semantic,Sequence);return Sequence!=nullptr;};
    const bool Pistol=Kind==TEXT("pistol"),Automatic=Kind==TEXT("automatic"),Flame=Kind==TEXT("flame");
    if(Flame)
    {
        for(const auto& M:TMap<FString,FString>{{TEXT("Idle"),TEXT("LowReady")},{TEXT("Aim"),TEXT("Aim")},{TEXT("Fire"),TEXT("Aim")},
            {TEXT("Walk"),TEXT("Walk")},{TEXT("Run"),TEXT("Run")},{TEXT("Reload"),TEXT("LowReady")}})
            if(!AddNational(M.Key,TEXT("A_fr_schilt_")+M.Value+TEXT("_N67_V82")))return false;
    }
    else if(Automatic)
    {
        for(const auto& M:TMap<FString,FString>{{TEXT("Idle"),TEXT("Idle")},{TEXT("Walk"),TEXT("Walk")},{TEXT("Run"),TEXT("Run")}})
            if(!(German?AddGerman(M.Key,TEXT("A_")+T+TEXT("_Carry_")+M.Value+TEXT("_N67_V82"))
                :AddNational(M.Key,TEXT("A_")+T+TEXT("_Carry_")+M.Value+TEXT("_N67_V82"))))return false;
        for(const TCHAR* Context:{TEXT("Ground"),TEXT("Trench")})for(const TCHAR* Action:{TEXT("Aim"),TEXT("Fire"),TEXT("Reload"),TEXT("Deploy"),TEXT("Pack")})
        {
            if((T==TEXT("fr_chauchat")||T==TEXT("de_mg0815"))&&FCString::Strcmp(Context,TEXT("Trench"))==0)continue;
            const FString GermanName=T==TEXT("de_mg08")&&FCString::Strcmp(Action,TEXT("Reload"))==0
                ?FString(TEXT("A_DE_MG08_"))+Context+TEXT("_FullReload_N67_V82")
                :TEXT("A_")+T+TEXT("_")+Context+TEXT("_")+Action+TEXT("_N67_V82");
            if(!(German?AddGerman(FString(Context)+Action,GermanName)
                :AddNational(FString(Context)+Action,TEXT("A_")+T+TEXT("_")+Context+TEXT("_")+Action+TEXT("_N67_V82"))))return false;
        }
        Out.Motions.Add(TEXT("Aim"),Out.Motions[TEXT("GroundAim")]);Out.Motions.Add(TEXT("Fire"),Out.Motions[TEXT("GroundFire")]);
        Out.Motions.Add(TEXT("Reload"),Out.Motions[TEXT("GroundReload")]);
        if(T==TEXT("de_mg0815"))
        {
            Out.Mechanism=Asset<USkeletalMesh>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/"),TEXT("SK_DE_MG0815_Pair_N67_V82")));
            if(!Out.Mechanism||!Out.Mechanism->GetSkeleton())return Reject(Error,TEXT("exact MG0815 four-bone mechanism missing"));
            const auto& Ref=Out.Mechanism->GetRefSkeleton();const TCHAR* Names[]={TEXT("weapon_root"),TEXT("drum"),TEXT("bipod_l"),TEXT("bipod_r")};
            if(Ref.GetNum()!=4)return Reject(Error,TEXT("MG0815 mechanism must contain its exact four declared bones"));
            for(int32 I=0;I<4;++I)if(Ref.GetBoneName(I)!=FName(Names[I])||Ref.GetParentIndex(I)!=(I?0:INDEX_NONE))
                return Reject(Error,TEXT("MG0815 mechanism names/parents differ"));
            for(const TCHAR* Action:{TEXT("Carry"),TEXT("Supported"),TEXT("Deploy"),TEXT("Pack")})
            {
                auto* A=Asset<UAnimSequence>(URI(German67,FString(TEXT("A_DE_MG0815_"))+Action+TEXT("Mechanism_N67_V82")));
                if(!A||A->GetSkeleton()!=Out.Mechanism->GetSkeleton()||!CompressedReady(A))
                    return Reject(Error,FString(TEXT("exact compressed MG0815 mechanism unavailable: "))+Action);
                for(const float Phase:{0.f,.25f,.5f,.75f,1.f})
                {TArray<FTransform> Pose;if(!ComponentPose(A,Out.Mechanism,Phase*A->GetPlayLength(),Pose)||Pose.Num()!=4)
                    return Reject(Error,FString(TEXT("actual compressed MG0815 four-bone extraction failed: "))+Action);}
                Out.MechanismMotions.Add(Action,A);
            }
            Out.Feed=Out.MechanismMotions[TEXT("Supported")];
        }
        else
        {
            Out.Mechanism=Asset<USkeletalMesh>(URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Weapons/Mechanical/"),TEXT("SK_R2_")+T));
            Out.Feed=Asset<UAnimSequence>(URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Weapons/Mechanical/"),TEXT("A_R2_")+T+TEXT("_Feed")));
        }
        if(!Out.Mechanism||!Out.Feed||Out.Feed->GetSkeleton()!=Out.Mechanism->GetSkeleton()||!CompressedReady(Out.Feed))
            return Reject(Error,TEXT("exact national firing mechanism mesh/sequence/Skeleton unavailable"));
        if(!MechanicalReady(Out.Mechanism,Out.VisualWeaponId+TEXT(" firing"),Error))return false;
    }
    else
    {
    if(!Add(TEXT("Idle"),Pistol?TEXT("A_Officer_LowReady_V34"):Automatic?TEXT("A_LowReady_MG_V41"):TEXT("A_LowReady_Rifle_V41"),true)
        ||!Add(TEXT("Aim"),Pistol?TEXT("A_Officer_Aim_V34"):German?TEXT("A_Aim_V34"):TEXT("A_Aim_R3"),Pistol||German)
        ||!Add(TEXT("Walk"),Pistol?TEXT("A_Officer_Walk_V34"):Automatic?TEXT("A_MG_Walk_R3"):German?TEXT("A_Walk_V34"):TEXT("A_Walk_R3"),Pistol||German)
        ||!Add(TEXT("Run"),Pistol?TEXT("A_Officer_Run_V34"):Automatic?TEXT("A_MG_Run_V34"):German?TEXT("A_Run_V34"):TEXT("A_Run_R3"),Pistol||Automatic||German)
        ||!Add(TEXT("Fire"),Pistol?TEXT("A_Officer_Fire_V34"):Automatic?TEXT("A_MG_Fire_V34"):TEXT("A_Fire_V34"),true))return false;
    if((Kind==TEXT("grenade")&&!FrenchVB)||Pistol)
        return Reject(Error,TEXT("exact national specialist/revolver/grenade prop and complete motion/reload lifecycle is not available"));
    if(FrenchVB)
    {
        Out.Projectile=Asset<UStaticMesh>(URI(FrenchVBWeapons67,TEXT("SM_FR_VB_Projectile_N67_V82")));
        if(!StaticReady(Out.Projectile,Error))return false;
        for(const TCHAR* Action:{TEXT("Aim"),TEXT("Fire"),TEXT("Reload")})
        {
            auto* Sequence=Clip(URI(FrenchVB67,FString(TEXT("A_FR_VB_"))+Action+TEXT("_N67_V82")),Out.Skeleton,Error);
            if(!Sequence)return false;Out.Motions.Add(Action,Sequence);
        }
    }
    else if(German)
    {if(!AddGerman(TEXT("Reload"),TEXT("A_DE_G98_OneCharger_Reload_N67_V82")))return false;}
    else if(!Add(TEXT("Reload"),TEXT("A_")+T+TEXT("_Load_R3")))return false;
    if(FrenchVB)Out.Motions.Add(TEXT("Bolt"),Out.Motions[TEXT("Aim")]);
    else if(T==TEXT("at_hu_m95"))
    {if(!AddNational(TEXT("Bolt"),TEXT("A_at_hu_m95_StraightPull_N67_V82")))return false;}
    else if(!Add(TEXT("Bolt"),German?TEXT("A_BoltCycle_V34"):TEXT("A_BoltCycle_R3"),German))return false;
    }
    if(!Add(TEXT("DeathBack"),TEXT("A_DeathBack_V34"),true)||!Add(TEXT("DeathFront"),TEXT("A_DeathFront_V34"),true)
        ||!Add(TEXT("DeathSide"),TEXT("A_DeathSide_V34"),true)||!Add(TEXT("Grenade"),TEXT("A_Grenade_Overarm_R3")))return false;
    if(Lead&&Role==EFrontlineRole::Pioneer)
    {
        if(!Add(TEXT("EngineerClear"),German?TEXT("A_WireCut_V40"):TEXT("A_Engineer_Clear_R3"),German))return false;
        if(German)
        {
            auto* Tool=Asset<UStaticMesh>(TEXT("/Game/Warfare/Characters/V40/Tools/SM_WireCutterHalf_V40.SM_WireCutterHalf_V40"));
            if(!StaticReady(Tool,Error))return false;
            Out.WireTool=Tool;
        }
    }
    // Native work-phase display is connected separately to its authoritative
    // worker/query. Never play the French telephone gesture empty-handed on DE.
    if(!German&&Lead&&Role==EFrontlineRole::Signals&&!Add(TEXT("SignalsRelay"),TEXT("A_Signals_Relay_R3")))return false;
    if(!Lead&&(Role==EFrontlineRole::Lewis||Role==EFrontlineRole::MobileGun))
        for(const TCHAR* Action:{TEXT("Aim"),TEXT("Fire"),TEXT("Bolt"),TEXT("Reload"),TEXT("Deploy"),TEXT("Pack")})
            if(!AddNational(FString(TEXT("Bearer"))+Action,TEXT("A_Bearer_Ground_")+FString(Action)+TEXT("_N67_V82")))return false;
    // Schilt uses the host's finite pressure/fuel discharge cycle. It never
    // enters magazine reload or swaps to an unrelated rifle ammunition prop.
    if(!Flame)
    {
    if(FrenchVB)
    {
        Out.Pair=Asset<USkeletalMesh>(URI(FrenchVBWeapons67,TEXT("SK_FR_VB_Pair_N67_V82")));
        Out.PairLoad=Asset<UAnimSequence>(URI(FrenchVB67,TEXT("A_FR_VB_LoadPair_N67_V82")));
    }
    else if(Out.VisualWeaponId==TEXT("de.gewehr98"))
    {
        Out.Pair=Asset<USkeletalMesh>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/"),TEXT("SK_DE_G98_LoadPair_N67_V82")));
        Out.PairLoad=Asset<UAnimSequence>(URI(German67,TEXT("A_DE_G98_LoadPair_N67_V82")));
    }
    else if(Out.VisualWeaponId==TEXT("de.mg08"))
    {
        Out.Pair=Asset<USkeletalMesh>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Weapons/German67/"),TEXT("SK_DE_MG08_FullLoad_N67_V82")));
        Out.PairLoad=Asset<UAnimSequence>(URI(German67,TEXT("A_DE_MG08_FullLoad_N67_V82")));
    }
    else if(Out.VisualWeaponId==TEXT("de.mg0815"))
    {
        Out.Pair=Out.Mechanism;Out.PairLoad=Asset<UAnimSequence>(URI(German67,TEXT("A_DE_MG0815_LoadPair_N67_V82")));
    }
    else
    {
        Out.Pair=Asset<USkeletalMesh>(URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Weapons/ReloadPairs/"),TEXT("SK_R2_Pair_")+T));
        Out.PairLoad=Asset<UAnimSequence>(URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Weapons/ReloadPairs/"),TEXT("A_R2_Pair_")+T+TEXT("_Load")));
    }
    if(!Out.Pair||!Out.PairLoad||!Out.Pair->GetSkeleton()||Out.PairLoad->GetSkeleton()!=Out.Pair->GetSkeleton()
        ||!CompressedReady(Out.PairLoad))return Reject(Error,TEXT("actual exact mechanical reload pair/Skeleton missing"));
    if(!MechanicalReady(Out.Pair,Out.VisualWeaponId+TEXT(" reload"),Error))return false;
    if(FrenchVB)
    {
        const auto& Ref=Out.Pair->GetRefSkeleton();const TCHAR* Names[]={TEXT("root"),TEXT("weapon_body"),TEXT("weapon_bolt"),TEXT("feed"),TEXT("top_cover"),TEXT("grenade")};
        const int32 Parents[]={INDEX_NONE,0,1,1,1,1};
        if(Ref.GetNum()!=6)return Reject(Error,TEXT("French VB requires its exact six-bone mechanical pair"));
        for(int32 I=0;I<6;++I)if(Ref.GetBoneName(I)!=FName(Names[I])||Ref.GetParentIndex(I)!=Parents[I])
            return Reject(Error,TEXT("French VB pair names/parents differ"));
        const auto* Render=Out.Pair->GetResourceForRendering();
        if(!Render||Render->LODRenderData.IsEmpty()||Render->LODRenderData[0].GetNumVertices()==0)
            return Reject(Error,TEXT("French VB pair actual render geometry missing"));

        for(const float Phase:{0.f,.25f,.5f,.75f,1.f})
        {TArray<FTransform> Pose;if(!ComponentPose(Out.PairLoad,Out.Pair,Phase*Out.PairLoad->GetPlayLength(),Pose)||Pose.Num()!=6)
            return Reject(Error,TEXT("French VB actual compressed six-bone pair extraction failed"));}
    }
    Out.ReloadRepeats=Out.VisualWeaponId==TEXT("fr.lebel")?W->capacity:1;
    }
    if(!PrepareOrdinaryBolt(Out,Error))return false;
    for(const auto& Motion:Out.Motions)for(const float Phase:{0.f,.25f,.5f,.75f,1.f})
    {TArray<FTransform> Pose;if(!ComponentPose(Motion.Value,Out.Body,Phase*Motion.Value->GetPlayLength(),Pose)||Pose.Num()!=67)
        return Reject(Error,TEXT("actual full67 compressed pose extraction failed: ")+Motion.Key);}
    Bindings.Add(Key,Out);return true;
}
const FBinding* BindingFor(const AWarfareFrontlineSoldier* S)
{const auto* State=S?Units.Find(S):nullptr;return State?&State->Binding:nullptr;}
}
UMaterialInterface* ResolveMaterial(UMaterialInterface* Material)
{
    if(!Material)return nullptr;const FString Path=Material->GetPathName();
    // The visible R3 torso/upper-arm coat is this Human UV_Game material.
    // Actual normal/restart actors passed the V87 material/mip checks. Keep
    // Legacy UV0 trousers and hats independent; retain the original for review.
    bool UseMainCoatV2=true;
#if WITH_EDITOR
    UseMainCoatV2=!(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87OriginalMainCoat"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview")));
#endif
    if(UseMainCoatV2)
    {
        const TCHAR* Families[]={TEXT("horizon_wool"),TEXT("khaki_wool"),TEXT("field_wool")};
        const TCHAR* Nations[]={TEXT("FR"),TEXT("RU"),TEXT("AH")};
        for(int32 I=0;I<3;++I)if(Path==URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Materials/Human/"),FString(TEXT("M_R2_Human_"))+Families[I]+TEXT("_UV6_T3")))
            if(auto* Candidate=Asset<UMaterialInterface>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Materials/MainCoatV87/CandidateV2/Materials/"),FString(TEXT("M_MainCoat_"))+Nations[I]+TEXT("_V2_V87"))))return Candidate;
    }
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87MainCoatCandidate"))
        &&Path==URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Materials/Human/"),TEXT("M_R2_Human_horizon_wool_UV6_T3")))
        if(auto* Candidate=Asset<UMaterialInterface>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Materials/MainCoatV87/"),TEXT("M_FRRU_MainCoat_FR_Horizon_V87"))))return Candidate;
#endif
    if(Path==OriginalBritishFace||Path==OriginalGermanFace)
        return Asset<UMaterialInterface>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Materials/Morph/"),
            Path==OriginalBritishFace?TEXT("M_FRRU_Face_British_V82"):TEXT("M_FRRU_Face_German_V82")));
    // Receiver copies preserve each dated nation's paired cloth maps and UV0.
    // Only the accepted host field-wear/fatal-blood layer is added downstream.
    for(const TCHAR* Name:{TEXT("M_AH_1914_1_V67"),TEXT("M_AH_1916_1_V67"),
        TEXT("M_FR_1914_1_V67"),TEXT("M_FR_1916_1_V67"),TEXT("M_RU_1914_1_V67"),TEXT("M_RU_1916_Adrian_Candidate_1_V67")})
        if(Path==URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Materials/UV3Legacy/"),Name))
        {
#if WITH_EDITOR
            // Three material-only candidates retain the actual national dye,
            // UV and blood graph. Promote only after the root's paired frames.
            if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87WoolCandidate"))
                &&(FCString::Strcmp(Name,TEXT("M_FR_1916_1_V67"))==0||FCString::Strcmp(Name,TEXT("M_RU_1914_1_V67"))==0||FCString::Strcmp(Name,TEXT("M_AH_1914_1_V67"))==0))
                if(auto* Candidate=Asset<UMaterialInterface>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Materials/WoolV87/"),
                    FString(TEXT("M_FRRU_Wool_"))+FString(Name).Mid(2)+TEXT("_V87"))))return Candidate;
#endif
            return Asset<UMaterialInterface>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Materials/FieldWear67/"),
                FString(TEXT("M_FRRU_FieldWear_"))+FString(Name).Mid(2)+TEXT("_V82")));
        }
    for(const TCHAR* Family:{TEXT("brass"),TEXT("canvas"),TEXT("khaki_paint"),TEXT("leather"),TEXT("recess"),TEXT("steel"),TEXT("walnut")})
    {
        const FString Name=FString(TEXT("M_R2_Surface_"))+Family+TEXT("_UV0_T1");
        if(Path==URI(TEXT("/Game/FRRUExpansionR2/FormalV82/Materials/Surface/"),Name))
            return Asset<UMaterialInterface>(URI(TEXT("/Game/FRRUIntegration/FormalV82/Materials/Mechanical67/"),
                FString(TEXT("M_FRRU_Mechanical_"))+Family+TEXT("_V82")));
    }
    return Material;
}

bool PrepareRoleAssets(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side,FString& Error)
{
    const int32 Count=WarfareFRRUEquipmentV82::RoleSize(B,Role,Side);if(Count<=0)return Reject(Error,TEXT("national role unavailable"));
    for(int32 I=0;I<Count;++I){FBinding Binding;if(!BuildBinding(B,Role,Side,I,Binding,Error))return false;}
    Error.Reset();return true;
}
bool RoleAssetsReady(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side,FString& Error)
{return PrepareRoleAssets(B,Role,Side,Error);}
bool PreparedMeshes(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,int32 Side,TArray<USkeletalMesh*>& Skeletal,TArray<UStaticMesh*>& Static,FString& Error)
{
    const int32 Count=WarfareFRRUEquipmentV82::RoleSize(B,Role,Side);
    if(!B||!WarfareFRRUCampaignV82::Active(B)||Count<=0)return Reject(Error,TEXT("national prepared role unavailable"));
    TArray<USkeletalMesh*> FoundSkeletal;TArray<UStaticMesh*> FoundStatic;
    for(int32 Member=0;Member<Count;++Member)
    {
        const auto* W=NativeWeapon(B,Role,Side,Member);if(!W)return Reject(Error,TEXT("prepared member weapon/date unavailable"));
        const FString Key=FString::Printf(TEXT("%d|%d|%d|%d|%s"),int32(WarfareFRRUCampaignV82::Nation(B,Side)),
            WarfareFRRUCampaignV82::CalendarDate(B),int32(Role),Member==0,*wfexpue::Wide(W->id));
        const FBinding* Binding=Bindings.Find(Key);
        if(!Binding||!Binding->Body||!Binding->Weapon)return Reject(Error,TEXT("exact national role has not been prepared"));
        for(USkeletalMesh* Mesh:{Binding->Body,Binding->Kit,Binding->WorkingKit,Binding->Mechanism,Binding->Pair,Binding->BoltMesh})
            if(Mesh)FoundSkeletal.AddUnique(Mesh);
        FoundStatic.AddUnique(Binding->Weapon);
        if(Binding->WireTool)FoundStatic.AddUnique(Binding->WireTool);
        if(Binding->Projectile)FoundStatic.AddUnique(Binding->Projectile);
    }
    // A partially prepared squad never leaks a partial list to the warmer.
    for(USkeletalMesh* Mesh:FoundSkeletal)Skeletal.AddUnique(Mesh);
    for(UStaticMesh* Mesh:FoundStatic)Static.AddUnique(Mesh);
    Error.Reset();return true;
}
bool InitializeUnit(AWarfareFrontlineSoldier* S,FString& Error)
{
    if(!S||!S->Soldier)return Reject(Error,TEXT("missing actual soldier component"));
    FBinding B;if(!BuildBinding(S->Battle,S->Role,S->Team,S->IsMachineGunner()?0:S->MemberIndex,B,Error))return false;
    if(auto* Previous=Units.Find(S);Previous&&Previous->BoltFinalized.IsValid())
        S->Soldier->UnregisterOnBoneTransformsFinalizedDelegate(Previous->BoltFinalized);
    for(auto It=Units.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    S->Soldier->SetSkeletalMeshAsset(B.Body);S->Soldier->SetPhysicsAsset(B.Body->GetPhysicsAsset(),true);
    // SetSkeletalMeshAsset retains component overrides. National slots must
    // resolve from this accepted source mesh, including after a reused actor.
    S->Soldier->EmptyOverrideMaterials();
    const auto& BodyMaterials=B.Body->GetMaterials();
    for(int32 I=0;I<BodyMaterials.Num();++I)
    {
        auto* Original=BodyMaterials[I].MaterialInterface.Get();auto* Parent=ResolveMaterial(Original);float Fatal=0;
        S->Soldier->SetMaterial(I,Parent);
        if(!Parent||!Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FatalBlood")),Fatal))continue;
        if(auto* MID=S->Soldier->CreateDynamicMaterialInstance(I,Parent))
        {
            MID->SetScalarParameterValue(TEXT("DeploymentSeed"),float((S->SquadId*17+S->MemberIndex*11)%97));
            MID->SetScalarParameterValue(TEXT("MudAmount"),.48f+.24f*S->GetMotionPhase());
            MID->SetScalarParameterValue(TEXT("FatalBlood"),S->IsAlive()?0.f:1.f);
            MID->SetVectorParameterValue(TEXT("BloodSide"),FLinearColor(1,0,0,0));
            const float Dye=.955f+S->GetMotionPhase()*.085f;
            MID->SetVectorParameterValue(TEXT("PersonalWeatheringTint"),FLinearColor(Dye,Dye*(.992f+S->GetMotionPhase()*.016f),Dye,1));
        }
    }
    TArray<FTransform> Pose;if(!ComponentPose(B.Motions[TEXT("Aim")],B.Body,0,Pose))return Reject(Error,TEXT("actual compressed aim extraction failed"));
    const int32 WeaponBone=B.Body->GetRefSkeleton().FindBoneIndex(TEXT("weapon_rifle"));
    if(WeaponBone==INDEX_NONE)return Reject(Error,TEXT("missing actual national weapon bone"));
    const FVector Forward=Pose[WeaponBone].TransformVectorNoScale(FVector(0,-1,0)).GetSafeNormal2D();
    if(Forward.IsNearlyZero())return Reject(Error,TEXT("actual aim weapon axis has no horizontal direction"));
    S->Soldier->SetRelativeRotation(FRotator(0,-Forward.Rotation().Yaw,0));
    FUnitState State;State.Binding=B;
    auto* Weapon=NewObject<UStaticMeshComponent>(S,TEXT("NationalWeaponV82"));S->AddInstanceComponent(Weapon);
    Weapon->SetupAttachment(S->Soldier,TEXT("weapon_rifle"));Weapon->SetStaticMesh(B.Weapon);Weapon->SetRelativeTransform(FTransform::Identity);
    Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);Weapon->SetCanEverAffectNavigation(false);Weapon->RegisterComponent();State.Weapon=Weapon;
    if(B.Projectile)
    {
        auto* Loaded=NewObject<UStaticMeshComponent>(S,TEXT("NationalLoadedVBV82"));S->AddInstanceComponent(Loaded);
        Loaded->SetupAttachment(S->Soldier,TEXT("weapon_rifle"));Loaded->SetStaticMesh(B.Projectile);
        Loaded->SetRelativeLocation(FVector(0,-97.25,3.8));Loaded->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Loaded->SetCanEverAffectNavigation(false);Loaded->SetVisibility(S->IsAlive()&&S->RoundsRemaining>0);
        Loaded->RegisterComponent();State.LoadedGrenade=Loaded;
    }
    if(B.Kit)
    {
        auto* Kit=NewObject<USkeletalMeshComponent>(S,TEXT("NationalDatedKitV82"));S->AddInstanceComponent(Kit);Kit->SetupAttachment(S->Soldier);
        Kit->SetSkeletalMeshAsset(B.Kit);Kit->SetLeaderPoseComponent(S->Soldier);Kit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for(int32 I=0;I<Kit->GetNumMaterials();++I)if(auto* Material=ResolveMaterial(Kit->GetMaterial(I)))Kit->SetMaterial(I,Material);
        Kit->SetCanEverAffectNavigation(false);Kit->RegisterComponent();State.Kit=Kit;
    }
    if(B.WorkingKit)
    {
        auto* Kit=NewObject<USkeletalMeshComponent>(S,TEXT("NationalWorkingKitV82"));S->AddInstanceComponent(Kit);Kit->SetupAttachment(S->Soldier);
        Kit->SetSkeletalMeshAsset(B.WorkingKit);Kit->SetLeaderPoseComponent(S->Soldier);Kit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for(int32 I=0;I<Kit->GetNumMaterials();++I)if(auto* Material=ResolveMaterial(Kit->GetMaterial(I)))Kit->SetMaterial(I,Material);
        Kit->SetCanEverAffectNavigation(false);Kit->SetVisibility(false);Kit->RegisterComponent();State.WorkingKit=Kit;
    }
    if(B.Pair)
    {
    auto* Pair=NewObject<USkeletalMeshComponent>(S,TEXT("NationalReloadPairV82"));S->AddInstanceComponent(Pair);Pair->SetupAttachment(S->Soldier,TEXT("weapon_rifle"));
    Pair->SetSkeletalMeshAsset(B.Pair);
    for(int32 I=0;I<B.Pair->GetMaterials().Num();++I)Pair->SetMaterial(I,ResolveMaterial(B.Pair->GetMaterials()[I].MaterialInterface.Get()));Pair->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Pair->SetCollisionEnabled(ECollisionEnabled::NoCollision);Pair->SetCanEverAffectNavigation(false);Pair->SetVisibility(false);Pair->RegisterComponent();State.Pair=Pair;
    // Registration creates the SingleNode instance; a transient animation
    // assignment made before registration has no instance to receive it.
    Pair->PlayAnimation(B.PairLoad,false);
    if(auto* Node=Pair->GetSingleNodeInstance()){Node->SetPlaying(false);Node->SetPosition(0,false);}
    }
    if(B.Mechanism)
    {
        auto* Rig=NewObject<USkeletalMeshComponent>(S,TEXT("NationalFireMechanismV82"));S->AddInstanceComponent(Rig);Rig->SetupAttachment(S->Soldier,TEXT("weapon_rifle"));
        Rig->SetSkeletalMeshAsset(B.Mechanism);
        for(int32 I=0;I<B.Mechanism->GetMaterials().Num();++I)Rig->SetMaterial(I,ResolveMaterial(B.Mechanism->GetMaterials()[I].MaterialInterface.Get()));Rig->SetAnimationMode(EAnimationMode::AnimationSingleNode);Rig->PlayAnimation(B.Feed,false);
        Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rig->SetCanEverAffectNavigation(false);Rig->SetVisibility(false);Rig->RegisterComponent();State.Mechanism=Rig;
    }
    if(B.BoltMesh)
    {
        auto* Rig=NewObject<UPoseableMeshComponent>(S,TEXT("NationalOrdinaryBoltV82"));S->AddInstanceComponent(Rig);
        Rig->SetupAttachment(S->Soldier,TEXT("weapon_rifle"));Rig->SetSkinnedAssetAndUpdate(B.BoltMesh);
        for(int32 I=0;I<B.BoltMesh->GetMaterials().Num();++I)Rig->SetMaterial(I,ResolveMaterial(B.BoltMesh->GetMaterials()[I].MaterialInterface.Get()));
        Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);Rig->SetCanEverAffectNavigation(false);
        Rig->SetVisibility(false);Rig->SetComponentTickEnabled(false);Rig->RegisterComponent();
        // The G98 retained pair's removable charger/rounds belong only to its
        // existing Reload driver, not to the ordinary post-shot bolt stroke.
        if(B.VisualWeaponId==TEXT("de.gewehr98"))for(const TCHAR* Name:{TEXT("charger"),TEXT("round_0"),TEXT("round_1"),TEXT("round_2"),TEXT("round_3"),TEXT("round_4")})
            Rig->SetBoneScaleByName(Name,FVector(.001f),EBoneSpaces::ComponentSpace);
        Rig->MarkRefreshTransformDirty();Rig->RefreshBoneTransforms();State.BoltRig=Rig;
    }
    Units.Add(S,MoveTemp(State));
    if(B.BoltMesh)
    {
        const TWeakObjectPtr<AWarfareFrontlineSoldier> Weak=S;
        Units.FindChecked(S).BoltFinalized=S->Soldier->RegisterOnBoneTransformsFinalizedDelegate(
            FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateLambda([Weak](){if(auto* Unit=Weak.Get())UpdateOrdinaryBolt(Unit);}));
    }
    Error.Reset();return true;
}
bool TrainPassengerAssetsReady(const AWarfareFrontlineGameMode* B,int32 Side,FString& Error)
{
    if(!B||WarfareFRRUCampaignV82::Nation(B,Side)!=EFRRUNationV81::AustroHungarian)
        return Reject(Error,TEXT("train passenger requires the actual Austro-Hungarian rifle roster"));
    FBinding Binding;if(!BuildBinding(B,EFrontlineRole::Rifle,Side,0,Binding,Error))return false;
    for(const TCHAR* Action:{TEXT("Seated"),TEXT("Stand"),TEXT("Stairs"),TEXT("CarryWalk")})
    {
        auto* Sequence=Clip(URI(TEXT("/Game/FRRUIntegration/FormalV82/Motions/Train67Correction1/"),FString(TEXT("A_AH_Train_"))+Action+TEXT("67_V82")),Binding.Skeleton,Error);
        if(!Sequence)return false;
        for(const float Phase:{0.f,.25f,.5f,.75f,1.f})
        {TArray<FTransform> Pose;if(!ComponentPose(Sequence,Binding.Body,Phase*Sequence->GetPlayLength(),Pose)||Pose.Num()!=67)
            return Reject(Error,FString(TEXT("actual train full67 compressed extraction failed: "))+Action);}
    }
    Error.Reset();return true;
}
bool SeekTrainPassenger(AWarfareFrontlineSoldier* S,UAnimSequence* Sequence,float Phase,FString& Error)
{
    auto* State=S?Units.Find(S):nullptr;FFRRUTrainPassengerStateV82 Passenger;
    if(!State||!S->Soldier||!S->IsAlive()||!Sequence||!FMath::IsFinite(Phase)||Phase<0||Phase>1
        ||!WarfareFRRUArmourV82::PassengerState(S,Passenger)||!Passenger.Embarked
        ||WarfareFRRUCampaignV82::Nation(S)!=EFRRUNationV81::AustroHungarian)
        return Reject(Error,TEXT("actual live embarked national passenger/phase missing"));
    const FString Name=Sequence->GetName();bool Declared=false;
    for(const TCHAR* Action:{TEXT("Seated"),TEXT("Stand"),TEXT("Stairs"),TEXT("CarryWalk")})
    {const FString Expected=FString(TEXT("A_AH_Train_"))+Action+TEXT("67_V82");
        Declared|=Name==Expected&&Sequence->GetPathName()==URI(TEXT("/Game/FRRUIntegration/FormalV82/Motions/Train67Correction1/"),Expected);}
    TArray<FTransform> Pose;
    if(!Declared||Sequence->GetSkeleton()!=State->Binding.Skeleton||!CompressedReady(Sequence)
        ||!ComponentPose(Sequence,State->Binding.Body,Phase*Sequence->GetPlayLength(),Pose)||Pose.Num()!=67)
        return Reject(Error,TEXT("undeclared/non67/invalid actual compressed train pose"));
    const int32 W=State->Binding.Body->GetRefSkeleton().FindBoneIndex(TEXT("weapon_rifle"));
    const FVector Forward=Pose[W].TransformVectorNoScale(FVector(0,-1,0)).GetSafeNormal2D();
    if(Forward.IsNearlyZero()||Passenger.Facing.IsNearlyZero())return Reject(Error,TEXT("actual train body/host heading invalid"));
    if(!State->WasTrain){State->BeforeTrainMeshRotation=S->Soldier->GetRelativeRotation();State->WasTrain=true;}
    S->SetActorRotation(Passenger.Facing.Rotation());S->Soldier->SetRelativeRotation(FRotator(0,-Forward.Rotation().Yaw,0));
    FFRRUPresentationAccessV82::SetMotion(S,Sequence,false);
    auto* Node=S->Soldier->GetSingleNodeInstance();if(!Node)return Reject(Error,TEXT("actual train animation instance missing"));
    Node->SetPlaying(false);Node->SetPosition(Phase*Sequence->GetPlayLength(),false);
    if(State->Pair.IsValid())State->Pair->SetVisibility(false);if(State->Mechanism.IsValid())State->Mechanism->SetVisibility(false);
    if(State->WorkingKit.IsValid())State->WorkingKit->SetVisibility(false);State->Weapon->SetVisibility(true);
    for(auto& Tool:State->WireTools)if(Tool.IsValid())Tool->SetVisibility(false);
    Error.Reset();return true;
}
void EndTrainPassengerPose(AWarfareFrontlineSoldier* S)
{
    auto* State=S?Units.Find(S):nullptr;if(!State||!State->WasTrain||!S->Soldier)return;
    S->Soldier->SetRelativeRotation(State->BeforeTrainMeshRotation);State->WasTrain=false;
    // The custom animation instance was retained aboard. Restart presentation
    // only; the carrier/world owns entry, weapon state and actor identity.
    if(S->IsAlive()){FFRRUPresentationAccessV82::RestartIdle(S);}
}
bool TickWork(AWarfareFrontlineSoldier* S)
{
    auto* State=S?Units.Find(S):nullptr;if(!State||State->ReleasedWeaponV87||!S->Soldier)return false;
    FFRRUWorkStateV82 Work;UAnimationAsset* Action=nullptr;
    const bool Active=S->IsAlive()&&S->WeaponPhase==EFrontlineWeaponPhase::Ready
        &&WarfareFRRUWorldV82::WorkState(S,Work)&&Work.Active&&!Work.Paused;
    if(Active)Action=Motion(S,Work.Semantic==EFRRUWorkSemanticV82::ClearPassage?TEXT("EngineerClear"):TEXT("SignalsRelay"));
    auto* Sequence=Cast<UAnimSequence>(Action);
    if(!Sequence)
    {
        if(State->WorkingKit.IsValid())State->WorkingKit->SetVisibility(false);
        for(auto& Tool:State->WireTools)if(Tool.IsValid())Tool->SetVisibility(false);
        if(State->WasWorking&&S->IsAlive())
        {FFRRUPresentationAccessV82::RestartIdle(S);}
        State->WasWorking=false;State->WorkPhase=0;return false;
    }
    // Untimed relay occupation is a visual loop on the existing host elapsed
    // value. It never creates a completion or a second progress counter.
    const bool ArmourRepair=Work.NodeId.StartsWith(TEXT("vehicle.repair."));
    const float Phase=ArmourRepair?float(FMath::Fmod(FMath::Max(0.,Work.HostElapsed-Work.StartedAt),double(Sequence->GetPlayLength()))/Sequence->GetPlayLength())
        :Work.Timed?FMath::Clamp(Work.NormalizedPhase,0.f,1.f)
        :float(FMath::Fmod(Work.HostElapsed,double(Sequence->GetPlayLength()))/Sequence->GetPlayLength());
    if(!Work.Facing.IsNearlyZero())S->SetActorRotation(Work.Facing.Rotation());
    FFRRUPresentationAccessV82::SetMotion(S,Sequence,false);
    if(auto* Node=S->Soldier->GetSingleNodeInstance())
    {Node->SetPlaying(false);Node->SetPosition(Phase*Sequence->GetPlayLength(),false);}
    if(State->WorkingKit.IsValid())State->WorkingKit->SetVisibility(true);
    if(ArmourRepair)for(auto& Tool:State->WireTools)if(Tool.IsValid())Tool->SetVisibility(false);
    if(State->Binding.WireTool&&Work.Semantic==EFRRUWorkSemanticV82::ClearPassage&&!ArmourRepair)
    {
        const auto Ease=[](float X){X=FMath::Clamp(X,0.f,1.f);return X*X*(3.f-2.f*X);};
        const float Visible=Ease(Phase/.24f)*(1.f-Ease((Phase-.80f)/.2f));
        const float Close=Ease((Phase-.38f)/.19f)*(1.f-Ease((Phase-.70f)/.13f));
        const FVector F=S->GetActorForwardVector(),R=S->GetActorRightVector();
        for(int32 I=0;I<2;++I)
        {
            auto* C=State->WireTools[I].Get();
            if(!C)
            {
                C=NewObject<UStaticMeshComponent>(S);S->AddInstanceComponent(C);C->SetupAttachment(S->GetRootComponent());
                C->SetStaticMesh(State->Binding.WireTool);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                C->SetCanEverAffectNavigation(false);C->RegisterComponent();State->WireTools[I]=C;
            }
            const float HandleSign=C->GetStaticMesh()->GetBounds().Origin.Y>=0?1.f:-1.f;
            const FQuat Rotation=FRotationMatrix::MakeFromY(-F*HandleSign).ToQuat()
                *FQuat(FVector::UpVector,FMath::DegreesToRadians((I?1.f:-1.f)*FMath::Lerp(21.f,10.f,Close)));
            C->SetWorldLocationAndRotation(S->Feet()+F*67.f+FVector(0,0,58)+R*(I?.55f:-.55f),Rotation);C->SetVisibility(Visible>.5f);
        }
    }
    State->WasWorking=true;State->WorkPhase=Phase;return true;
}
UAnimationAsset* Motion(const AWarfareFrontlineSoldier* S,const TCHAR* Semantic)
{const auto* B=BindingFor(S);const auto* ClipAsset=B?B->Motions.Find(Semantic):nullptr;return ClipAsset?*ClipAsset:nullptr;}
UAnimationAsset* SupportedMotion(const AWarfareFrontlineSoldier* S,const TCHAR* Semantic,bool Trench)
{
    const auto* B=BindingFor(S);if(!B)return nullptr;
    const FString Context=B->Kind==TEXT("automatic")?(Trench&&B->VisualWeaponId!=TEXT("fr.chauchat")&&B->VisualWeaponId!=TEXT("de.mg0815")?TEXT("Trench"):TEXT("Ground")):TEXT("Bearer");
    const auto* A=B->Motions.Find(Context+Semantic);return A?*A:nullptr;
}
static bool GunSupportPointsV82(const FString& Id,TArray<FVector,TInlineAllocator<4>>& Points)
{
    Points.Reset();
    if(Id==TEXT("ru.maxim"))Points={FVector(-24,-4,-38.9),FVector(24,-4,-38.9)};
    else if(Id==TEXT("fr.hotchkiss")||Id==TEXT("at-hu.schwarzlose"))
        Points={FVector(-22,-38,-38.9),FVector(22,-38,-38.9),FVector(0,18,-38.9)};
    else if(Id==TEXT("fr.chauchat"))Points={FVector(-11,-61,-26),FVector(11,-61,-26)};
    else if(Id==TEXT("de.mg08"))
        Points={FVector(-19,-43,-30.4),FVector(19,-43,-30.4),FVector(-19,27,-30.4),FVector(19,27,-30.4)};
    else if(Id==TEXT("de.mg0815"))
        Points={FVector(-11.418825,-36.036987,-32.322731),FVector(11.418835,-36.036983,-32.322739)};
    return !Points.IsEmpty();
}

bool GroundGunPoseTargetV82(const AWarfareFrontlineSoldier* S,UAnimationAsset* CurrentAsset,FGroundGunPoseTargetV82& Out)
{
    // The measured two-wheel Maxim and three-foot families only. The two
    // real wheels constrain roll; their free pitch keeps the authored bearing.
    const auto* State=S?Units.Find(S):nullptr;const auto* Gun=FrontlineMachinegunsV30::Get(S);
    if(!IsInGameThread()||!S||!S->IsAlive()||!S->bFieldEntered||S->bMoving||!S->Soldier||!S->GetWorld()
        ||!WarfareFRRUCampaignV82::Active(S->Battle)||WarfareFRRUArmourV82::Embarked(S)
        ||!State||!State->Weapon.IsValid()||State->Binding.Kind!=TEXT("automatic")
        ||!Gun||!Gun->IsSupported()||Gun->IsTrench()
        ||(State->Binding.VisualWeaponId!=TEXT("fr.hotchkiss")&&State->Binding.VisualWeaponId!=TEXT("at-hu.schwarzlose")
            &&State->Binding.VisualWeaponId!=TEXT("ru.maxim")))return false;
    bool ActualGroundClip=false;
    for(const TCHAR* Semantic:{TEXT("Aim"),TEXT("Fire"),TEXT("Reload")})
        ActualGroundClip|=CurrentAsset&&CurrentAsset==SupportedMotion(S,Semantic,false);
    if(!ActualGroundClip||!State->Weapon->GetRelativeTransform().Equals(FTransform::Identity,.0001f))return false;
    TArray<FVector,TInlineAllocator<4>> Points;
    const int32 ExpectedContacts=State->Binding.VisualWeaponId==TEXT("ru.maxim")?2:3;
    if(!GunSupportPointsV82(State->Binding.VisualWeaponId,Points)||Points.Num()!=ExpectedContacts)return false;
    const FTransform Original=State->Weapon->GetComponentTransform(),BodyWorld=S->Soldier->GetComponentTransform();
    if(Original.ContainsNaN()||BodyWorld.ContainsNaN()
        ||!Original.GetScale3D().Equals(FVector::OneVector,.0001f)||!BodyWorld.GetScale3D().Equals(FVector::OneVector,.0001f))return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(NationalGunSupportsV82),true);
    if(S->Battle)for(const AWarfareFrontlineSoldier* U:S->Battle->Units)if(U)Q.AddIgnoredActor(U);
    FTransform Candidate=Original;
    const int32 LastIteration=ExpectedContacts==2?3:4;
    for(int32 Iteration=0;Iteration<=LastIteration;++Iteration)
    {
        const double Shift=FVector::Dist(Candidate.GetTranslation(),Original.GetTranslation());
        const double Tilt=FMath::RadiansToDegrees(Candidate.GetRotation().AngularDistance(Original.GetRotation()));
        if(Candidate.ContainsNaN()||!FMath::IsFinite(Shift)||!FMath::IsFinite(Tilt)||Shift>12.||Tilt>15.)return false;
        FVector Feet[3],Contacts[3];float NativeZ[3]={};bool AllAccepted=true;
        for(int32 I=0;I<ExpectedContacts;++I)
        {
            Feet[I]=Candidate.TransformPosition(Points[I]);FHitResult Hit;
            if(!WarfareFRRUWorldV82::TryHeight(S->Battle,Feet[I].X,Feet[I].Y,NativeZ[I])
                ||!S->GetWorld()->LineTraceSingleByChannel(Hit,Feet[I]+FVector(0,0,150),Feet[I]-FVector(0,0,180),ECC_Visibility,Q)
                ||FMath::Abs(Hit.ImpactPoint.Z-NativeZ[I])>=.25f)return false;
            Contacts[I]=FVector(Feet[I].X,Feet[I].Y,Hit.ImpactPoint.Z);
            const double Gap=Feet[I].Z-Hit.ImpactPoint.Z;
            AllAccepted&=Gap>=-1.5f&&Gap<=2.5f;
        }
        if(AllAccepted)
        {
            FGroundGunPoseTargetV82 Snapshot;Snapshot.Frame=GFrameCounter;Snapshot.ContactCount=ExpectedContacts;Snapshot.BodyWorld=BodyWorld;
            Snapshot.WeaponComponent=Candidate.GetRelativeTransform(BodyWorld);
            for(int32 I=0;I<ExpectedContacts;++I){Snapshot.LocalFeet[I]=Points[I];Snapshot.HitZ[I]=Contacts[I].Z;Snapshot.NativeZ[I]=NativeZ[I];}
            // Publish a value-only snapshot once all original surface/contact
            // predicates pass. Worker evaluation never reads S/World/Units.
            Out=Snapshot;return true;
        }
        if(Iteration==LastIteration)return false;
        FQuat Correction;
        if(ExpectedContacts==2)
        {
            FVector OldSpan=Feet[1]-Feet[0],NewSpan=Contacts[1]-Contacts[0];
            if(!OldSpan.Normalize()||!NewSpan.Normalize()||FMath::Abs(NewSpan.Z)>FMath::Sin(FMath::DegreesToRadians(15.)))return false;
            // Minimum rigid rotation adds no twist about the measured axle.
            // Unequal projected lengths are never scaled to manufacture contact.
            Correction=FQuat::FindBetweenNormals(OldSpan,NewSpan);
        }
        else
        {
            FVector OldNormal=FVector::CrossProduct(Feet[1]-Feet[0],Feet[2]-Feet[0]);
            FVector NewNormal=FVector::CrossProduct(Contacts[1]-Contacts[0],Contacts[2]-Contacts[0]);
            if(OldNormal.Z<0)OldNormal=-OldNormal;if(NewNormal.Z<0)NewNormal=-NewNormal;
            if(!OldNormal.Normalize()||!NewNormal.Normalize()||NewNormal.Z<FMath::Cos(FMath::DegreesToRadians(15.)))return false;
            Correction=FQuat::FindBetweenNormals(OldNormal,NewNormal);
        }
        if(Correction.ContainsNaN())return false;
        Candidate.SetRotation((Correction*Candidate.GetRotation()).GetNormalized());
        FVector LocalCentre=FVector::ZeroVector,GroundCentre=FVector::ZeroVector;
        for(int32 I=0;I<ExpectedContacts;++I){LocalCentre+=Points[I];GroundCentre+=Contacts[I];}
        LocalCentre/=ExpectedContacts;GroundCentre/=ExpectedContacts;
        Candidate.SetTranslation(GroundCentre-Candidate.GetRotation().RotateVector(Candidate.GetScale3D()*LocalCentre));
        // Tilt changes XY: the next iteration repeats the *actual* native
        // floor and visibility trace at every real newly transformed support.
    }
    return false;
}

static bool EvaluateGunSupportContactsV82(const AWarfareFrontlineSoldier* S,int32& ActualContacts,FJsonObject* Readback)
{
    ActualContacts=0;const auto* State=S?Units.Find(S):nullptr;
    if(!State||!State->Weapon.IsValid()||!S->GetWorld()||State->Binding.Kind!=TEXT("automatic"))return false;
    TArray<FVector,TInlineAllocator<4>> Points;
    if(!GunSupportPointsV82(State->Binding.VisualWeaponId,Points))return false;
    const auto VectorRecord=[](const FVector& V)
    {TArray<TSharedPtr<FJsonValue>> A;for(double C:{V.X,V.Y,V.Z})A.Add(MakeShared<FJsonValueNumber>(C));return A;};
    const auto TransformRecord=[&](const FTransform& T)
    {
        auto A=MakeShared<FJsonObject>();A->SetArrayField(TEXT("translationCm"),VectorRecord(T.GetTranslation()));
        A->SetArrayField(TEXT("scale"),VectorRecord(T.GetScale3D()));const FQuat Rotation=T.GetRotation();TArray<TSharedPtr<FJsonValue>> Q;
        for(double C:{Rotation.X,Rotation.Y,Rotation.Z,Rotation.W})Q.Add(MakeShared<FJsonValueNumber>(C));A->SetArrayField(TEXT("quaternionXYZW"),Q);
        A->SetBoolField(TEXT("finite"),!T.ContainsNaN());return A;
    };
    const FTransform WeaponWorld=State->Weapon->GetComponentTransform();
    if(Readback)
    {
        Readback->SetBoolField(TEXT("readOnly"),true);Readback->SetBoolField(TEXT("bindingValid"),true);
        Readback->SetNumberField(TEXT("frame"),double(GFrameCounter));Readback->SetNumberField(TEXT("squad"),S->SquadId);Readback->SetNumberField(TEXT("member"),S->MemberIndex);
        Readback->SetNumberField(TEXT("hostElapsed"),S->Battle?S->Battle->Elapsed:0);Readback->SetStringField(TEXT("weaponId"),State->Binding.VisualWeaponId);
        Readback->SetArrayField(TEXT("actualFeetCm"),VectorRecord(S->Feet()));Readback->SetNumberField(TEXT("groundContactOffsetCm"),S->GroundContactOffsetV82);
        Readback->SetObjectField(TEXT("actorWorldTransform"),TransformRecord(S->GetActorTransform()));
        Readback->SetObjectField(TEXT("weaponComponentWorldTransform"),TransformRecord(WeaponWorld));
        Readback->SetObjectField(TEXT("weaponComponentRelativeTransform"),TransformRecord(State->Weapon->GetRelativeTransform()));
        Readback->SetStringField(TEXT("weaponStaticMesh"),State->Weapon->GetStaticMesh()?State->Weapon->GetStaticMesh()->GetPathName():FString());
        Readback->SetArrayField(TEXT("weaponMinusYWorldDirection"),VectorRecord(WeaponWorld.TransformVector(FVector(0,-1,0)).GetSafeNormal()));
        Readback->SetArrayField(TEXT("weaponUpWorldDirection"),VectorRecord(WeaponWorld.TransformVector(FVector(0,0,1)).GetSafeNormal()));
        Readback->SetNumberField(TEXT("weaponMinusYPitchDegrees"),FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(WeaponWorld.TransformVector(FVector(0,-1,0)).GetSafeNormal().Z,-1.,1.))));
        if(S->Soldier)
        {
            Readback->SetObjectField(TEXT("bodyMeshWorldTransform"),TransformRecord(S->Soldier->GetComponentTransform()));
            Readback->SetObjectField(TEXT("bodyMeshRelativeTransform"),TransformRecord(S->Soldier->GetRelativeTransform()));
            for(const TCHAR* Bone:{TEXT("weapon_rifle"),TEXT("pelvis"),TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r")})
                if(S->Soldier->GetBoneIndex(FName(Bone))!=INDEX_NONE)
                {
                    const FTransform World=S->Soldier->GetBoneTransform(FName(Bone),RTS_World);
                    Readback->SetObjectField(FString(Bone)+TEXT("_world"),TransformRecord(World));
                    Readback->SetObjectField(FString(Bone)+TEXT("_bodyComponent"),TransformRecord(S->Soldier->GetBoneTransform(FName(Bone),RTS_Component)));
                    Readback->SetObjectField(FString(Bone)+TEXT("_weaponRelative"),TransformRecord(World.GetRelativeTransform(WeaponWorld)));
                }
            auto* AnimationNode=S->Soldier->GetSingleNodeInstance();const auto* Clip=AnimationNode?AnimationNode->GetCurrentAsset():nullptr;
            Readback->SetStringField(TEXT("actualAnimation"),Clip?Clip->GetPathName():FString());
            Readback->SetNumberField(TEXT("actualAnimationTimeSeconds"),AnimationNode?AnimationNode->GetCurrentTime():-1.f);
            Readback->SetNumberField(TEXT("actualAnimationLengthSeconds"),AnimationNode?AnimationNode->GetLength():0.f);
        }
    }
    FCollisionQueryParams Q(SCENE_QUERY_STAT(NationalGunSupportsV82),true);
    if(S->Battle)for(const AWarfareFrontlineSoldier* U:S->Battle->Units)if(U)Q.AddIgnoredActor(U);
    TArray<TSharedPtr<FJsonValue>> FootRecords;
    for(const FVector& Local:Points)
    {
        const FVector Foot=WeaponWorld.TransformPosition(Local);FHitResult Hit;float NativeZ=0;
        const bool HasNative=WarfareFRRUWorldV82::TryHeight(S->Battle,Foot.X,Foot.Y,NativeZ);
        // Preserve the original short circuit: no native floor means no trace.
        const bool HitSurface=HasNative&&S->GetWorld()->LineTraceSingleByChannel(Hit,Foot+FVector(0,0,150),Foot-FVector(0,0,180),ECC_Visibility,Q);
        const bool SameNativeSurface=HitSurface&&FMath::Abs(Hit.ImpactPoint.Z-NativeZ)<.25f;
        const bool FootWithinTolerance=SameNativeSurface&&Foot.Z-Hit.ImpactPoint.Z>=-1.5f&&Foot.Z-Hit.ImpactPoint.Z<=2.5f;
        if(FootWithinTolerance)++ActualContacts;
        if(Readback)
        {
            auto A=MakeShared<FJsonObject>();A->SetArrayField(TEXT("localCm"),VectorRecord(Local));A->SetArrayField(TEXT("worldCm"),VectorRecord(Foot));
            A->SetBoolField(TEXT("nativeHeightAvailable"),HasNative);A->SetNumberField(TEXT("nativeZCm"),NativeZ);
            A->SetBoolField(TEXT("traceAttempted"),HasNative);A->SetBoolField(TEXT("traceHit"),HitSurface);
            A->SetArrayField(TEXT("impactCm"),VectorRecord(Hit.ImpactPoint));A->SetArrayField(TEXT("impactNormal"),VectorRecord(Hit.ImpactNormal));
            A->SetStringField(TEXT("hitActor"),Hit.GetActor()?Hit.GetActor()->GetPathName():FString());
            A->SetStringField(TEXT("hitComponent"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():FString());
            A->SetBoolField(TEXT("initialPenetration"),Hit.bStartPenetrating);A->SetNumberField(TEXT("footMinusNativeCm"),Foot.Z-NativeZ);
            A->SetNumberField(TEXT("footMinusHitCm"),Foot.Z-Hit.ImpactPoint.Z);A->SetNumberField(TEXT("hitMinusNativeCm"),Hit.ImpactPoint.Z-NativeZ);
            A->SetBoolField(TEXT("sameNativeSurface"),SameNativeSurface);A->SetBoolField(TEXT("accepted"),FootWithinTolerance);
            FootRecords.Add(MakeShared<FJsonValueObject>(A));
        }
    }
    if(Readback)
    {
        Readback->SetArrayField(TEXT("feet"),FootRecords);Readback->SetNumberField(TEXT("actualContacts"),ActualContacts);
        Readback->SetNumberField(TEXT("requiredContacts"),Points.Num());Readback->SetBoolField(TEXT("allContactsAccepted"),ActualContacts==Points.Num());
        Readback->SetNumberField(TEXT("minimumFootMinusHitCm"),-1.5);Readback->SetNumberField(TEXT("maximumFootMinusHitCm"),2.5);
        Readback->SetNumberField(TEXT("maximumHitNativeDifferenceCm"),.25);
    }
    return ActualContacts==Points.Num();
}
bool GunSupportContacts(const AWarfareFrontlineSoldier* S,int32& ActualContacts)
{return EvaluateGunSupportContactsV82(S,ActualContacts,nullptr);}
#if WITH_EDITOR
TSharedRef<FJsonObject> GunSupportReadback(const AWarfareFrontlineSoldier* S)
{
    auto Out=MakeShared<FJsonObject>();Out->SetBoolField(TEXT("readOnly"),true);Out->SetBoolField(TEXT("bindingValid"),false);
    Out->SetBoolField(TEXT("gameThread"),IsInGameThread());if(!IsInGameThread())return Out;
    int32 Contacts=0;EvaluateGunSupportContactsV82(S,Contacts,&Out.Get());return Out;
}
#endif
UAnimationAsset* RemapMotion(const AWarfareFrontlineSoldier* S,UAnimationAsset* Requested)
{
    const auto* B=BindingFor(S);if(!B||!Requested)return nullptr;
    if(Requested->GetSkeleton()==B->Skeleton)return Requested;
    const FString Name=Requested->GetName();FString Error;
    if(Name==TEXT("A_Aim_V34"))return Motion(S,TEXT("Aim"));
    if(Name==TEXT("A_Walk_V34"))return Motion(S,TEXT("Walk"));
    if(Name==TEXT("A_Run_V34"))return Motion(S,TEXT("Run"));
    if(Name.Contains(TEXT("Reload")))return S->GetReloadMotion();
    if(Name.Contains(TEXT("BoltCycle")))
    {if(auto* A=FrontlineMachinegunsV30::Action(S,TEXT("Bolt")))return A;return Motion(S,TEXT("Bolt"));}
    if(Name.StartsWith(TEXT("A_Fire"))||Name.StartsWith(TEXT("A_MG_Fire"))||Name.StartsWith(TEXT("A_Officer_Fire")))
    {if(auto* A=FrontlineMachinegunsV30::Action(S,TEXT("Fire")))return A;return Motion(S,TEXT("Fire"));}
    if(B->VisualWeaponId==TEXT("fr.vb")&&Name==TEXT("A_No23_Fire_V41"))return Motion(S,TEXT("Fire"));
    return Clip(URI(Legacy67,Name+TEXT("_Human67_V82")),B->Skeleton,Error);
}
bool UsesPistol(const AWarfareFrontlineSoldier* S)
{const auto* W=S?NativeWeapon(S->Battle,S->Role,S->Team,S->IsMachineGunner()?0:S->MemberIndex):nullptr;return W&&FString(wfexpue::Wide(W->kind))==TEXT("pistol");}
bool HasBoundWeapon(const AWarfareFrontlineSoldier* S){const auto* State=S?Units.Find(S):nullptr;return State&&!State->ReleasedWeaponV87&&State->Weapon.IsValid();}
UStaticMeshComponent* ReleaseHeldWeaponV87(AWarfareFrontlineSoldier* S,bool& LeaveEmplaced)
{
    LeaveEmplaced=false;auto* State=S?Units.Find(S):nullptr;
    if(!S||!S->Soldier||!State||!State->Weapon.IsValid())return nullptr;
    LeaveEmplaced=State->ReleasedEmplacedV87;
    auto* Weapon=State->Weapon.Get();if(State->ReleasedWeaponV87)return Weapon;
    const FString& Id=State->Binding.VisualWeaponId;
    LeaveEmplaced=Id==TEXT("fr.hotchkiss")||Id==TEXT("ru.maxim")||Id==TEXT("at-hu.schwarzlose")||Id==TEXT("de.mg08");
    // A planted crew-served gun retains its last real support frame when that
    // frame is still available. Otherwise the common release path settles its
    // unchanged geometry onto the actual surface beneath it.
    if(LeaveEmplaced)if(const auto* Gun=FrontlineMachinegunsV30::Get(S))
    {FTransform Anchor;if(Gun->ReadSupportedWeaponAnchorV8447(Anchor))Weapon->SetWorldTransform(Anchor);}
    Weapon->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Weapon->SetVisibility(true);Weapon->ComponentTags.AddUnique(TEXT("WF_V34_RELEASED_WEAPON"));
    if(State->LoadedGrenade.IsValid())State->LoadedGrenade->AttachToComponent(Weapon,FAttachmentTransformRules::KeepWorldTransform);
    for(auto* Component:{State->Pair.Get(),State->Mechanism.Get()})if(Component){Component->SetVisibility(false);Component->SetComponentTickEnabled(false);}
    if(State->BoltRig.IsValid())State->BoltRig->SetVisibility(false);
    if(State->BoltFinalized.IsValid()){S->Soldier->UnregisterOnBoneTransformsFinalizedDelegate(State->BoltFinalized);State->BoltFinalized.Reset();}
    State->BoltVisible=false;State->ReleasedWeaponV87=true;State->ReleasedEmplacedV87=LeaveEmplaced;
    return Weapon;
}
UStaticMesh* RifleGrenadeMesh(const AWarfareFrontlineSoldier* S)
{const auto* B=BindingFor(S);return B&&B->VisualWeaponId==TEXT("fr.vb")?B->Projectile:nullptr;}
FVector Muzzle(const AWarfareFrontlineSoldier* S)
{const auto* State=S?Units.Find(S):nullptr;return State&&State->Weapon.IsValid()?State->Weapon->GetComponentTransform().TransformPosition(State->Binding.MuzzleLocal):S?S->Feet():FVector::ZeroVector;}
FVector MuzzleForward(const AWarfareFrontlineSoldier* S)
{const auto* State=S?Units.Find(S):nullptr;return State&&State->Weapon.IsValid()?State->Weapon->GetComponentTransform().TransformVectorNoScale(FVector(0,-1,0)).GetSafeNormal():FVector::ZeroVector;}
void TickUnit(AWarfareFrontlineSoldier* S,float DT)
{
    auto* State=S?Units.Find(S):nullptr;if(!State||State->ReleasedWeaponV87||!State->Weapon.IsValid())return;
    const bool Reload=S->IsAlive()&&S->WeaponPhase==EFrontlineWeaponPhase::Reload;
    if(Reload)
    {
        if(!State->WasReload){State->CompletedAtReloadStart=S->ReloadsCompleted;++State->ReloadCycles;}
        const float Whole=S->GetWeaponReloadNormalizedPhase();State->LastReloadPhase=Whole;
        const float Local=Whole>=1.f?1.f:FMath::Frac(Whole*State->Binding.ReloadRepeats);
        // The host owns ammo and the reload clock. This seeks presentation only;
        // pause, cancellation, death and arbitrary DT never dispatch ammunition.
        if(auto* Node=S->Soldier->GetSingleNodeInstance()){if(auto* A=Cast<UAnimSequence>(S->GetReloadMotion()))Node->SetPosition(Local*A->GetPlayLength(),false);}
        if(State->Pair.IsValid())
        {
            auto* Pair=State->Pair.Get();auto* Node=Pair->GetSingleNodeInstance();
            if(!Node){Pair->InitAnim(true);Node=Pair->GetSingleNodeInstance();}
            if(Node&&Node->GetCurrentAsset()!=State->Binding.PairLoad){Pair->PlayAnimation(State->Binding.PairLoad,false);Node=Pair->GetSingleNodeInstance();}
            if(Node){Node->SetPlaying(false);Node->SetPosition(Local*State->Binding.PairLoad->GetPlayLength(),false);}
        }
    }
    else if(State->WasReload){if(S->ReloadsCompleted==State->CompletedAtReloadStart)++State->CancelledReloads;State->LastReloadPhase=0;}
    const bool MG15=State->Binding.VisualWeaponId==TEXT("de.mg0815");
    const bool Firing=S->IsAlive()&&!Reload&&State->Mechanism.IsValid()&&S->WeaponPhase==EFrontlineWeaponPhase::Recoil;
    const bool DisplayMechanism=Firing||(MG15&&S->IsAlive()&&!Reload);
    if(State->Pair.IsValid())State->Pair->SetVisibility(Reload);
    if(State->Mechanism.IsValid())
    {
        State->Mechanism->SetVisibility(DisplayMechanism);
        if(MG15&&DisplayMechanism)
        {
            FString Action=TEXT("Carry");float Phase=0;
            if(const auto* Stance=FrontlineMachinegunsV30::Get(S))
            {
                if(Stance->State==EFrontlineGunStanceV30::Supported)Action=TEXT("Supported");
                else if(Stance->State==EFrontlineGunStanceV30::Deploy||Stance->State==EFrontlineGunStanceV30::Pack)
                {Action=Stance->State==EFrontlineGunStanceV30::Deploy?TEXT("Deploy"):TEXT("Pack");
                    Phase=Stance->TransitionDurationV38()>0?FMath::Clamp(Stance->StanceClock/Stance->TransitionDurationV38(),0.f,1.f):0.f;}
            }
            auto* Sequence=State->Binding.MechanismMotions.FindRef(Action);
            if(Sequence)
            {
                auto* Node=State->Mechanism->GetSingleNodeInstance();
                if(!Node||Node->GetCurrentAsset()!=Sequence)State->Mechanism->PlayAnimation(Sequence,false);
                if((Node=State->Mechanism->GetSingleNodeInstance()))
                {Node->SetPlaying(false);Node->SetPosition(Phase*Sequence->GetPlayLength(),false);}
            }
        }
        else if(Firing)if(auto* Body=S->Soldier->GetSingleNodeInstance())if(auto* Node=State->Mechanism->GetSingleNodeInstance())
        {const float Phase=Body->GetLength()>0?FMath::Clamp(Body->GetCurrentTime()/Body->GetLength(),0.f,1.f):0.f;
            Node->SetPlaying(false);Node->SetPosition(Phase*State->Binding.Feed->GetPlayLength(),false);}
    }
    State->Weapon->SetVisibility(!Reload&&!DisplayMechanism&&!State->BoltVisible);State->WasReload=Reload;
    if(State->LoadedGrenade.IsValid())State->LoadedGrenade->SetVisibility(S->IsAlive()&&!Reload&&S->RoundsRemaining>0);
    if(State->WorkingKit.IsValid())
    {auto* Node=S->Soldier->GetSingleNodeInstance();const auto* A=Node?Node->GetCurrentAsset():nullptr;
        State->WorkingKit->SetVisibility(S->IsAlive()&&A&&(A==Motion(S,TEXT("EngineerClear"))||A==Motion(S,TEXT("SignalsRelay"))));}
}
TSharedRef<FJsonObject> ProbeUnit(const AWarfareFrontlineSoldier* S,bool IncludeCompressedPoses)
{
    auto R=MakeShared<FJsonObject>();const auto* State=S?Units.Find(S):nullptr;
    R->SetBoolField(TEXT("actualBinding"),State&&S&&S->Soldier&&S->Soldier->GetSkeletalMeshAsset()==State->Binding.Body);
    R->SetBoolField(TEXT("artAccepted"),false);R->SetBoolField(TEXT("weaponContactsAccepted"),false);R->SetBoolField(TEXT("skinAccepted"),false);
    if(State)
    {
#if WITH_EDITOR
        // Read the actual component overrides and render sections, rather than
        // assuming that a similarly named cloth source is visible on the coat.
        if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ParityReview"))&&IsInGameThread()&&S->Soldier)
        {
            const auto* Mesh=S->Soldier->GetSkeletalMeshAsset();const auto* Render=Mesh?Mesh->GetResourceForRendering():nullptr;
            if(Render&&!Render->LODRenderData.IsEmpty())
            {
                const auto& LOD=Render->LODRenderData[0];TMap<int32,int32> Triangles;int32 Total=0;
                for(const auto& Section:LOD.RenderSections)if(!Section.bDisabled)
                {Triangles.FindOrAdd(Section.MaterialIndex)+=Section.NumTriangles;Total+=Section.NumTriangles;}
                R->SetNumberField(TEXT("surfaceReadbackRenderLOD"),0);R->SetNumberField(TEXT("surfaceReadbackUVChannels"),LOD.GetNumTexCoords());
                R->SetNumberField(TEXT("surfaceReadbackTotalTriangles"),Total);TArray<TSharedPtr<FJsonValue>> Slots;
                bool HasCoatV2=false,CoatLifecycleV2=true,CoatFreshV2=true,CoatMipsV2=true;
                for(int32 I=0;I<Mesh->GetMaterials().Num();++I)
                {
                    const auto& Source=Mesh->GetMaterials()[I];auto* Actual=S->Soldier->GetMaterial(I);auto* Resolved=ResolveMaterial(Source.MaterialInterface);
                    auto Slot=MakeShared<FJsonObject>();Slot->SetNumberField(TEXT("index"),I);
                    Slot->SetStringField(TEXT("importedSlot"),Source.ImportedMaterialSlotName.ToString());
                    Slot->SetStringField(TEXT("sourceMaterial"),GetPathNameSafe(Source.MaterialInterface));
                    Slot->SetStringField(TEXT("resolvedSourceMaterial"),GetPathNameSafe(Resolved));Slot->SetStringField(TEXT("actualComponentMaterial"),GetPathNameSafe(Actual));
                    const int32 Count=Triangles.FindRef(I);Slot->SetNumberField(TEXT("renderTriangles"),Count);
                    Slot->SetNumberField(TEXT("renderTriangleFraction"),Total?double(Count)/Total:0.);
                    Slot->SetBoolField(TEXT("actualBaseMatchesResolved"),Actual&&Resolved&&Actual->GetMaterial()==Resolved->GetMaterial());
                    const bool CoatV2=Resolved&&Resolved->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Materials/MainCoatV87/CandidateV2/Materials/"));
                    if(CoatV2)
                    {
                        HasCoatV2=true;const auto* Dynamic=Cast<UMaterialInstanceDynamic>(Actual);float Fatal=0,Mud=0,Burn=0,Seed=0;
                        const bool Bound=Dynamic&&Dynamic->Parent.Get()==Resolved
                            &&Actual->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FatalBlood")),Fatal)
                            &&Actual->GetScalarParameterValue(FMaterialParameterInfo(TEXT("MudAmount")),Mud)
                            &&Actual->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BurnAmountV84411")),Burn)
                            &&Actual->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DeploymentSeed")),Seed);
                        CoatLifecycleV2&=Bound;
                        CoatFreshV2&=Bound&&S->IsAlive()&&FMath::IsNearlyZero(Fatal)&&FMath::IsNearlyZero(Burn)
                            &&FMath::IsNearlyEqual(Mud,.48f+.24f*S->GetMotionPhase(),.0001f)
                            &&FMath::IsNearlyEqual(Seed,float((S->SquadId*17+S->MemberIndex*11)%97));
                    }
                    TArray<TSharedPtr<FJsonValue>> Parents;TSet<UMaterialInterface*> Seen;auto* Parent=Actual;
                    while(Parent&&!Seen.Contains(Parent)&&Seen.Num()<12)
                    {Seen.Add(Parent);Parents.Add(MakeShared<FJsonValueString>(Parent->GetPathName()));const auto* Instance=Cast<UMaterialInstance>(Parent);Parent=Instance?Instance->Parent.Get():nullptr;}
                    Slot->SetArrayField(TEXT("actualParentChain"),Parents);TArray<TSharedPtr<FJsonValue>> Textures;
                    auto Parameters=MakeShared<FJsonObject>();
                    for(const TCHAR* Name:{TEXT("DeploymentSeed"),TEXT("FatalBlood"),TEXT("MudAmount"),TEXT("BurnAmountV84411")})
                    {float Value=0;auto P=MakeShared<FJsonObject>();const bool Present=Actual&&Actual->GetScalarParameterValue(FMaterialParameterInfo(Name),Value);P->SetBoolField(TEXT("present"),Present);if(Present)P->SetNumberField(TEXT("value"),Value);Parameters->SetObjectField(Name,P);}
                    Slot->SetObjectField(TEXT("actualLifecycleParameters"),Parameters);
                    if(Actual&&Count>0)
                    {
                        TArray<UTexture*> Used;
                        // UE5.8's former five-argument overload is an empty
                        // deprecated final function. Query the live SM6 graph.
                        Actual->GetUsedTextures(Used,TOptional<EMaterialQualityLevel::Type>(EMaterialQualityLevel::High),TOptional<EShaderPlatform>(GMaxRHIShaderPlatform));
                        int32 CoatScanCount=0;
                        for(UTexture* Texture:Used)if(auto* T=Cast<UTexture2D>(Texture))
                        {
                            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("object"),T->GetPathName());
                            Item->SetNumberField(TEXT("platformWidth"),T->GetSizeX());Item->SetNumberField(TEXT("platformHeight"),T->GetSizeY());
                            Item->SetNumberField(TEXT("residentMips"),T->GetStreamableResourceState().NumResidentLODs);
                            Item->SetNumberField(TEXT("platformMips"),T->GetNumMips());Textures.Add(MakeShared<FJsonValueObject>(Item));
#if WITH_EDITORONLY_DATA
                            Item->SetNumberField(TEXT("sourceWidth"),T->Source.GetSizeX());Item->SetNumberField(TEXT("sourceHeight"),T->Source.GetSizeY());
                            if(CoatV2&&T->GetPathName().StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Materials/MainCoatV87/CandidateV2/Textures/")))
                            {++CoatScanCount;CoatMipsV2&=T->Source.GetSizeX()==4096&&T->Source.GetSizeY()==4181&&T->GetSizeX()==4096&&T->GetSizeY()==4096&&T->GetNumMips()==13&&T->GetStreamableResourceState().NumResidentLODs>1;}
#endif
                        }
                        if(CoatV2)CoatMipsV2&=CoatScanCount==3;
                    }
                    Slot->SetArrayField(TEXT("actualUsedTextures"),Textures);Slots.Add(MakeShared<FJsonValueObject>(Slot));
                }
                R->SetArrayField(TEXT("actualSurfaceSlotsV87"),Slots);
                R->SetBoolField(TEXT("nationalCoatV2Present"),HasCoatV2);R->SetBoolField(TEXT("nationalCoatV2ActualMIDLifecycleBound"),HasCoatV2&&CoatLifecycleV2);
                R->SetBoolField(TEXT("nationalCoatV2NewActorDefaults"),HasCoatV2&&CoatFreshV2);R->SetBoolField(TEXT("nationalCoatV2ActualMipChain"),HasCoatV2&&CoatMipsV2);
            }
        }
#endif
        R->SetBoolField(TEXT("actualWeaponReleasedV87"),State->ReleasedWeaponV87);
        R->SetBoolField(TEXT("releasedWeaponLeftEmplacedV87"),State->ReleasedEmplacedV87);
        if(auto* Weapon=State->Weapon.Get())
        {
            R->SetStringField(TEXT("actualIssuedWeaponMeshV87"),GetPathNameSafe(Weapon->GetStaticMesh()));
            R->SetBoolField(TEXT("releasedWeaponDetachedV87"),State->ReleasedWeaponV87&&Weapon->GetAttachParent()==nullptr);
            R->SetBoolField(TEXT("releasedWeaponSimulatingV87"),Weapon->IsSimulatingPhysics());
        }
        R->SetStringField(TEXT("body"),State->Binding.BodyURI);R->SetStringField(TEXT("datedKit"),State->Binding.KitURI);
        R->SetStringField(TEXT("workingKit"),State->Binding.WorkingKitURI);
        if(auto* Node=Cast<UWarfareFrontlineAnimation>(S->Soldier->GetAnimInstance()))
        {
            const auto D=Node->GetCharacterDiagnostics();R->SetNumberField(TEXT("nationalCarryGripFrames"),D.NationalCarryGripFramesV82);
            R->SetNumberField(TEXT("nationalCarryGripRejectedFrames"),D.NationalCarryGripRejectedFramesV82);
            R->SetNumberField(TEXT("nationalGroundGunFrames"),D.NationalGroundGunFramesV82);
            R->SetNumberField(TEXT("nationalGroundGunRejectedFrames"),D.NationalGroundGunRejectedFramesV82);
            R->SetNumberField(TEXT("maximumNationalGroundGunShiftCm"),D.MaximumNationalGroundGunShiftCmV82);
            R->SetNumberField(TEXT("maximumNationalGroundGunTiltDegrees"),D.MaximumNationalGroundGunTiltDegreesV82);
            R->SetNumberField(TEXT("maximumNationalGroundFootErrorCm"),D.MaximumNationalGroundFootErrorCmV82);
            R->SetNumberField(TEXT("maximumNationalGroundGripErrorCm"),D.MaximumNationalGroundGripErrorCmV82);
            R->SetNumberField(TEXT("maximumNationalCarryGunShiftCm"),D.MaximumNationalCarryGunShiftCmV82);
            R->SetNumberField(TEXT("maximumNationalBothGripResidualCm"),D.MaximumNationalBothGripResidualCmV82);
        }
        R->SetBoolField(TEXT("actualNativeWorkPoseActive"),State->WasWorking);R->SetNumberField(TEXT("actualNativeWorkPosePhase"),State->WorkPhase);
        FFRRUWorkStateV82 Work;const bool NativeWork=WarfareFRRUWorldV82::WorkState(S,Work);
        R->SetBoolField(TEXT("actualNativeWorker"),NativeWork);
        if(NativeWork){R->SetStringField(TEXT("nativeWorkNode"),Work.NodeId);R->SetNumberField(TEXT("nativeWorkProgressSeconds"),Work.ProgressSeconds);
            R->SetNumberField(TEXT("nativeWorkRequiredSeconds"),Work.RequiredSeconds);R->SetBoolField(TEXT("nativeWorkTimed"),Work.Timed);}
        R->SetStringField(TEXT("weaponId"),State->Binding.WeaponId);R->SetStringField(TEXT("weaponKind"),State->Binding.Kind);
        R->SetStringField(TEXT("skeleton"),State->Binding.Skeleton->GetPathName());R->SetNumberField(TEXT("actualReferenceBones"),State->Binding.Body->GetRefSkeleton().GetNum());
        R->SetStringField(TEXT("actualMuzzleWorldCm"),Muzzle(S).ToString());R->SetNumberField(TEXT("reloadRepeatedSourceCycles"),State->Binding.ReloadRepeats);
        R->SetNumberField(TEXT("visualReloadsStarted"),State->ReloadCycles);R->SetNumberField(TEXT("visualReloadsCancelled"),State->CancelledReloads);
        R->SetBoolField(TEXT("reloadPairVisible"),State->Pair.IsValid()&&State->Pair->IsVisible());
        R->SetNumberField(TEXT("authoritativeReloadPhase"),S->GetWeaponReloadNormalizedPhase());
        R->SetNumberField(TEXT("lastSoughtReloadPhase"),State->LastReloadPhase);
        int32 Contacts=0;R->SetBoolField(TEXT("actualGunSupportReady"),GunSupportContacts(S,Contacts));R->SetNumberField(TEXT("actualGunSupportContacts"),Contacts);
#if WITH_EDITOR
        if(State->Binding.Kind==TEXT("automatic"))R->SetObjectField(TEXT("actualGunSupportReadback"),GunSupportReadback(S));
#endif
        R->SetStringField(TEXT("mechanismMesh"),State->Binding.Mechanism?State->Binding.Mechanism->GetPathName():TEXT(""));
        R->SetStringField(TEXT("ordinaryBoltMesh"),State->Binding.BoltMesh?State->Binding.BoltMesh->GetPathName():TEXT(""));
        R->SetBoolField(TEXT("ordinaryBoltVisible"),State->BoltVisible);R->SetNumberField(TEXT("ordinaryBoltSamePoseFrames"),State->BoltPoseFrames);
        R->SetNumberField(TEXT("ordinaryBoltRejectedFrames"),State->BoltRejectedFrames);R->SetNumberField(TEXT("ordinaryBoltMaximumPoseErrorCm"),State->MaximumBoltPoseErrorCm);
        R->SetNumberField(TEXT("ordinaryBoltMaximumTravelCm"),State->MaximumBoltTravelCm);
        R->SetStringField(TEXT("ordinaryBoltHumanDelta"),State->LastHumanBoltDelta.ToString());R->SetStringField(TEXT("ordinaryBoltActualMechanicalPose"),State->LastMechanicalBolt.ToString());
        R->SetBoolField(TEXT("ordinaryBoltSurfaceContactsAccepted"),false);
        R->SetStringField(TEXT("rifleGrenadeMesh"),State->Binding.Projectile?State->Binding.Projectile->GetPathName():TEXT(""));
        R->SetBoolField(TEXT("loadedRifleGrenadeVisible"),State->LoadedGrenade.IsValid()&&State->LoadedGrenade->IsVisible());
        R->SetBoolField(TEXT("mechanismVisible"),State->Mechanism.IsValid()&&State->Mechanism->IsVisible());
        R->SetStringField(TEXT("actualMuzzleForward"),MuzzleForward(S).ToString());
        if(!IncludeCompressedPoses)return R;
        const auto& Ref=State->Binding.Body->GetRefSkeleton();
        const auto PoseRecord=[&](const TArray<FTransform>& Pose)
        {
            auto P=MakeShared<FJsonObject>();const int32 W=Ref.FindBoneIndex(TEXT("weapon_rifle"));
            P->SetBoolField(TEXT("all67Finite"),Pose.Num()==67&&Algo::AllOf(Pose,[](const FTransform& T){return !T.ContainsNaN();}));
            for(const TCHAR* Name:{TEXT("root"),TEXT("pelvis"),TEXT("Head"),TEXT("hand_l"),TEXT("hand_r"),
                TEXT("index_04_leaf_l"),TEXT("index_04_leaf_r"),TEXT("foot_l"),TEXT("foot_r"),TEXT("weapon_rifle"),TEXT("weapon_bolt")})
            {const int32 I=Ref.FindBoneIndex(Name);if(Pose.IsValidIndex(I))
                {P->SetStringField(FString(Name)+TEXT("_componentCm"),Pose[I].GetTranslation().ToString());
                    if(Pose.IsValidIndex(W))P->SetStringField(FString(Name)+TEXT("_weaponLocalCm"),Pose[W].InverseTransformPosition(Pose[I].GetTranslation()).ToString());}}
            return P;
        };
        TArray<TSharedPtr<FJsonValue>> ClipRecords;
        for(const auto& Entry:State->Binding.Motions)
        {
            auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("semantic"),Entry.Key);C->SetStringField(TEXT("asset"),Entry.Value->GetPathName());
            C->SetBoolField(TEXT("compressedDataValid"),Entry.Value->IsCompressedDataValid());
            C->SetNumberField(TEXT("playLengthSeconds"),Entry.Value->GetPlayLength());TArray<TSharedPtr<FJsonValue>> Samples;
            // Includes source reach/feed/regrip guards 33/36/63/66 and both ends.
            for(const float Phase:{0.f,32.f/95.f,35.f/95.f,62.f/95.f,65.f/95.f,1.f})
            {TArray<FTransform> Pose;const bool Extracted=ComponentPose(Entry.Value,State->Binding.Body,Phase*Entry.Value->GetPlayLength(),Pose);
                auto Sample=PoseRecord(Pose);Sample->SetNumberField(TEXT("normalizedTime"),Phase);Sample->SetBoolField(TEXT("actualCompressedExtraction"),Extracted);
                Samples.Add(MakeShared<FJsonValueObject>(Sample));}
            C->SetArrayField(TEXT("samples"),Samples);ClipRecords.Add(MakeShared<FJsonValueObject>(C));
        }
        R->SetArrayField(TEXT("actualCompressedPoses"),ClipRecords);
        TArray<FTransform> Displayed;for(int32 I=0;I<Ref.GetNum();++I)Displayed.Add(S->Soldier->GetBoneTransform(Ref.GetBoneName(I),RTS_Component));
        R->SetObjectField(TEXT("actualDisplayedComponentPose"),PoseRecord(Displayed));
        R->SetBoolField(TEXT("inertialBlendAccepted"),false);R->SetBoolField(TEXT("ragdollAccepted"),false);
        R->SetBoolField(TEXT("completeReloadContactAccepted"),false);
    }
    return R;
}
}
