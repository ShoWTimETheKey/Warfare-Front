#include "WarfareFrontlineTripoCharacterV87.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFRRUCampaignV82.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
TSharedRef<FJsonObject> Result(const TCHAR* Scope)
{auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("scope"),Scope);J->SetBoolField(TEXT("executed"),false);return J;}
}

#if WITH_EDITOR
// Integrator adds only WITH_EDITOR friend struct FFrontlineTripoCharacterAccessV87
// to AWarfareFrontlineSoldier. No public gameplay/cheat API is introduced.
struct FFrontlineTripoCharacterAccessV87
{
    static void ClearMotion(AWarfareFrontlineSoldier* U){U->Playing=nullptr;U->LastAnimationTime=0;}
    static UAnimationAsset* Playing(const AWarfareFrontlineSoldier* U){return U->Playing;}
    static float Time(const AWarfareFrontlineSoldier* U){return U->LastAnimationTime;}
    static void RestoreMotionIdentity(AWarfareFrontlineSoldier* U,UAnimationAsset* A,float T)
    {U->Playing=A;U->LastAnimationTime=T;}
    static void Set(AWarfareFrontlineSoldier* U,UAnimationAsset* A,bool Loop=true){U->SetMotion(A,Loop);}
    static void Reload(AWarfareFrontlineSoldier* U){U->StartReload();}
    static float ReloadClock(const AWarfareFrontlineSoldier* U){return U->FirePoseClock;}
    static void Prepare(AWarfareFrontlineSoldier* U,AWarfareFrontlineSoldier* Enemy)
    {U->FocusTarget=Enemy;U->bEngaging=true;U->ShotClock=U->GrenadeClock=10000.f;U->AimSettleClock=U->ContactReactionClock=0;}
};

namespace
{
const FName DisposableTag(TEXT("WF_V87_TRIPO_DISPOSABLE"));
const FName DroppedTag(TEXT("WF_V87_TRIPO_DROPPED_WEAPON"));
struct FBinding
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;
    FString Id,Receipt;
    TArray<TStrongObjectPtr<UObject>> Keep;
    USkeletalMesh* Body=nullptr;USkeletalMesh* Casualty=nullptr;USkeleton* Skeleton=nullptr;
    UPhysicsAsset* Physics=nullptr;UStaticMesh* Dropped=nullptr;UStaticMesh* Limb=nullptr;FName CutBone;
    TMap<FName,UAnimationAsset*> Clips;
    TMap<UAnimationAsset*,UAnimationAsset*> Forward,Reverse;
    USkeletalMesh* OldBody=nullptr;UPhysicsAsset* OldPhysics=nullptr;UClass* OldClass=nullptr;
    UAnimationAsset* OldPlaying=nullptr;UAnimInstance* OldInstance=nullptr;
    TArray<UMaterialInterface*> OldMaterials;
    FTransform OldRelative;float OldTime=0,OldRate=1,OldIdentityTime=0;
    FSingleAnimationPlayData OldAnimationData;
    int32 OldTransitions=0,Rebuilds=0,RejectedMotions=0;
    bool OldLoop=true,OldPlayingFlag=true;
    TWeakObjectPtr<UStaticMeshComponent> Released;
    struct FAttachment{TWeakObjectPtr<USceneComponent> Child;FTransform Relative;};
    TArray<FAttachment> WeaponChildren;
    void Retain(UObject* Object){if(Object)Keep.Emplace(Object);}
};
TUniquePtr<FBinding>& Registry(){static TUniquePtr<FBinding> R;return R;}
FBinding* Find(const AWarfareFrontlineSoldier* U)
{
    auto* R=Registry().Get();return R&&R->Unit.Get()==U&&U&&U->Soldier
        &&FrontlineTripoCharacterV87::Allowed(U->Battle)&&U->Soldier->GetSkeletalMeshAsset()==R->Body?R:nullptr;
}
bool CandidatePath(const FString& P,const FString& Root)
{return !P.IsEmpty()&&P.StartsWith(Root,ESearchCase::CaseSensitive)&&!P.Contains(TEXT(".."));}
template<class T>T* Load(FBinding& R,const FString& Path)
{auto* A=LoadObject<T>(nullptr,*Path);R.Retain(A);return A;}
bool SameReference(const FReferenceSkeleton& A,const FReferenceSkeleton& B)
{
    if(A.GetNum()!=B.GetNum())return false;
    for(int32 I=0;I<A.GetNum();++I)if(A.GetBoneName(I)!=B.GetBoneName(I)||A.GetParentIndex(I)!=B.GetParentIndex(I)
        ||!A.GetRefBonePose()[I].Equals(B.GetRefBonePose()[I],.0001f))return false;
    return true;
}
int32 SlotIndex(const USkeletalMesh* Mesh,FName Name)
{for(int32 I=0;I<Mesh->GetMaterials().Num();++I)if(Mesh->GetMaterials()[I].MaterialSlotName==Name)return I;return INDEX_NONE;}
bool Canonical(const FReferenceSkeleton& Ref,const FReferenceSkeleton& Legacy,const FrontlineTripoCharacterV87::FDescriptor& D)
{
    if(Ref.GetNum()!=D.ExpectedBoneNames.Num()||Ref.GetNum()!=D.ExpectedParentIndices.Num()
        ||Ref.GetNum()==0||Ref.GetBoneName(0)!=TEXT("root")||Ref.GetParentIndex(0)!=INDEX_NONE)return false;
    for(int32 I=0;I<Ref.GetNum();++I)if(Ref.GetBoneName(I)!=D.ExpectedBoneNames[I]||Ref.GetParentIndex(I)!=D.ExpectedParentIndices[I]
        ||Ref.GetRefBonePose()[I].ContainsNaN())return false;
    // Names/parents are a runtime interface. Independent accepted rest/bone lengths
    // are intentionally not compared to the old Skeleton's reference transforms.
    for(int32 I=0;I<Legacy.GetNum();++I)
    {
        const int32 N=Ref.FindBoneIndex(Legacy.GetBoneName(I));if(N==INDEX_NONE)return false;
        const int32 LP=Legacy.GetParentIndex(I),NP=Ref.GetParentIndex(N);
        if(LP==INDEX_NONE){if(NP!=INDEX_NONE)return false;}
        else if(NP==INDEX_NONE||Ref.GetBoneName(NP)!=Legacy.GetBoneName(LP))return false;
    }
    return true;
}
void Rebuild(AWarfareFrontlineSoldier* U,USkeletalMesh* Body,UPhysicsAsset* Physics,UClass* Class,const FTransform& Relative,
    const FSingleAnimationPlayData* AnimationData=nullptr)
{
    auto* M=U->Soldier.Get();M->HandleExistingParallelEvaluationTask(true,true);
    // Destroy the old proxy: displayed/transition mesh-index snapshots, ragdoll
    // locals and elbow caches may not survive a different reference skeleton.
    M->SetAnimInstanceClass(nullptr);M->AnimationData=AnimationData?*AnimationData:FSingleAnimationPlayData();
    M->SetSkeletalMeshAsset(Body);M->SetPhysicsAsset(Physics,true);
    M->SetRelativeTransform(Relative);M->EmptyOverrideMaterials();
    FFrontlineTripoCharacterAccessV87::ClearMotion(U);M->SetAnimInstanceClass(Class);M->InitAnim(true);
}
void Pose(AWarfareFrontlineSoldier* U,float DT)
{U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();}
}
#endif

bool FrontlineTripoCharacterV87::Allowed(const AWarfareFrontlineGameMode* B)
{
#if WITH_EDITOR
    FString Dir;return B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV87CharacterOnly"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
#else
    return false;
#endif
}
bool FrontlineTripoCharacterV87::IsBound(const AWarfareFrontlineSoldier* U)
{
#if WITH_EDITOR
    return Find(U)!=nullptr;
#else
    return false;
#endif
}
UAnimationAsset* FrontlineTripoCharacterV87::Motion(const AWarfareFrontlineSoldier* U,FName Semantic)
{
#if WITH_EDITOR
    if(auto* R=Find(U))if(auto* A=R->Clips.Find(Semantic))return *A;
#endif
    return nullptr;
}
bool FrontlineTripoCharacterV87::IsSemantic(const AWarfareFrontlineSoldier* U,const UAnimationAsset* Asset,FName Semantic)
{return Asset&&Asset==Motion(U,Semantic);}
UAnimationAsset* FrontlineTripoCharacterV87::RemapMotion(const AWarfareFrontlineSoldier* U,UAnimationAsset* Legacy)
{
#if WITH_EDITOR
    if(auto* R=Find(U))
    {
        if(R->Reverse.Contains(Legacy))return Legacy;
        if(auto* A=R->Forward.Find(Legacy))return *A;
        ++R->RejectedMotions;return nullptr;
    }
#endif
    return Legacy;
}
UAnimationAsset* FrontlineTripoCharacterV87::LegacyEquivalent(const AWarfareFrontlineSoldier* U,UAnimationAsset* Asset)
{
#if WITH_EDITOR
    if(auto* R=Find(U))if(auto* A=R->Reverse.Find(Asset))return *A;
#endif
    return Asset;
}
USkeletalMesh* FrontlineTripoCharacterV87::CasualtyBody(const AWarfareFrontlineSoldier* U)
{
#if WITH_EDITOR
    if(auto* R=Find(U))return R->Casualty;
#endif
    return nullptr;
}
bool FrontlineTripoCharacterV87::ResolveCasualty(const AWarfareFrontlineSoldier* U,USkeletalMesh*& Body,UStaticMesh*& Limb,FName& CutBone)
{
    Body=nullptr;Limb=nullptr;CutBone=NAME_None;
#if WITH_EDITOR
    if(auto* R=Find(U)){Body=R->Casualty;Limb=R->Limb;CutBone=R->CutBone;return true;}
#endif
    return false;
}

TSharedRef<FJsonObject> FrontlineTripoCharacterV87::Bind(AWarfareFrontlineSoldier* U,const FDescriptor& D,
    TFunctionRef<void(const FString&,bool)> External)
{
    auto J=Result(TEXT("One accepted independent UK NCO body/skeleton/motion binding in Editor/NoSave only. No default roster or asset writes."));
    bool Good=true;TArray<TSharedPtr<FJsonValue>> Checks;
    const auto Check=[&](const FString& N,bool P){External(N,P);Good&=P;auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),N);C->SetBoolField(TEXT("passed"),P);Checks.Add(MakeShared<FJsonValueObject>(C));};
    const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);return J;};
#if WITH_EDITOR
    Check(TEXT("v87_character_disposable_single_uk_nco"),U&&Allowed(U->Battle)&&U->ActorHasTag(DisposableTag)&&U->IsAlive()
        &&U->GetFaction()==EFrontlineFaction::British&&U->IsSectionLeader()&&!U->IsCommissionedOfficer()
        &&U->Soldier&&U->Soldier->GetSkeletalMeshAsset()&&U->Battle->LoadedSoldier
        &&Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()));
    if(!Good)return Finish();
    Check(TEXT("v87_character_acceptance_declared_before_any_load"),D.bSkinAccepted&&D.bDerivedRigAccepted&&D.bWeaponFrameAccepted
        &&!D.Id.IsEmpty()&&!D.AcceptanceReceipt.IsEmpty()&&!D.MeshRelativeTransform.ContainsNaN()
        &&D.MeshRelativeTransform.GetScale3D().GetMin()>0
        &&FMath::IsNearlyEqual(D.MeshRelativeTransform.GetScale3D().X,D.MeshRelativeTransform.GetScale3D().Y,.0001)
        &&FMath::IsNearlyEqual(D.MeshRelativeTransform.GetScale3D().Y,D.MeshRelativeTransform.GetScale3D().Z,.0001));
    Check(TEXT("v87_character_new_candidate_namespace_only"),D.CandidateRoot.EndsWith(TEXT("/"))
        &&D.CandidateRoot.StartsWith(TEXT("/Game/Warfare/AssetIntake/Tripo/"))&&!D.CandidateRoot.Contains(TEXT("..")));
    if(!Good)return Finish();
    if(Registry()&&!Registry()->Unit.IsValid())Registry().Reset();
    Check(TEXT("v87_character_registry_empty"),!Registry());if(!Good)return Finish();
    TArray<FString> Paths={D.Body,D.Skeleton,D.BasePhysics,D.CasualtyBody,D.DetachedLimb,D.DroppedWeapon};for(const auto& P:D.Motions)Paths.Add(P.Value);
    bool Owned=true;for(const auto& P:Paths)Owned&=CandidatePath(P,D.CandidateRoot);
    Check(TEXT("v87_character_all_assets_explicit_in_new_namespace"),Owned);if(!Good)return Finish();
    auto R=MakeUnique<FBinding>();R->Unit=U;R->Id=D.Id;R->Receipt=D.AcceptanceReceipt;
    R->Body=Load<USkeletalMesh>(*R,D.Body);R->Skeleton=Load<USkeleton>(*R,D.Skeleton);R->Physics=Load<UPhysicsAsset>(*R,D.BasePhysics);
    R->Casualty=Load<USkeletalMesh>(*R,D.CasualtyBody);R->Dropped=Load<UStaticMesh>(*R,D.DroppedWeapon);
    R->Limb=Load<UStaticMesh>(*R,D.DetachedLimb);R->CutBone=D.CasualtyCutBone;
    Check(TEXT("v87_character_required_body_skeleton_physics_casualty_limb_drop_loaded"),R->Body&&R->Skeleton&&R->Physics&&R->Casualty&&R->Limb&&R->Dropped);
    if(!Good)return Finish();
    const auto& Ref=R->Body->GetRefSkeleton();
    bool Render=true;
    for(const auto* Mesh:{R->Body,R->Casualty})
    {
        const auto* Data=Mesh->GetResourceForRendering();Render&=Data&&!Data->LODRenderData.IsEmpty()&&Data->LODRenderData[0].GetNumVertices()>0;
        for(const auto& Material:Mesh->GetMaterials())Render&=Material.MaterialInterface!=nullptr;
    }
    Check(TEXT("v87_character_actual_render_data_and_bound_materials"),Render);
    Check(TEXT("v87_character_independent_accepted_canonical_reference"),R->Skeleton!=U->Battle->LoadedSoldier->GetSkeleton()
        &&R->Body->GetSkeleton()==R->Skeleton&&R->Casualty->GetSkeleton()==R->Skeleton
        &&SameReference(Ref,R->Skeleton->GetReferenceSkeleton())&&SameReference(Ref,R->Casualty->GetRefSkeleton())
        &&Canonical(Ref,U->Battle->LoadedSoldier->GetRefSkeleton(),D));
    bool Physics=R->Body->GetPhysicsAsset()==R->Physics&&!R->Physics->SkeletalBodySetups.IsEmpty();
    for(const auto& Body:R->Physics->SkeletalBodySetups)Physics&=Body&&Ref.FindBoneIndex(Body->BoneName)!=INDEX_NONE;
    Check(TEXT("v87_character_basis_physics_actual_body_bones"),Physics);
    const auto* Muzzle=R->Body->FindSocket(TEXT("WF_Muzzle"));
    Check(TEXT("v87_character_actual_weapon_muzzle_socket"),Muzzle&&Muzzle->BoneName==TEXT("weapon_rifle")
        &&!Muzzle->RelativeLocation.ContainsNaN()&&!Muzzle->RelativeRotation.ContainsNaN());
    const FVector LimbSize=R->Limb->GetBounds().BoxExtent*2;
    Check(TEXT("v87_character_actual_casualty_cut_and_fragment"),!R->CutBone.IsNone()&&Ref.FindBoneIndex(R->CutBone)!=INDEX_NONE
        &&LimbSize.GetMax()>=18&&LimbSize.GetMax()<=120&&LimbSize.GetMin()>=2);
    bool Drop=R->Dropped->GetBodySetup()&&R->Dropped->GetBodySetup()->AggGeom.GetElementCount()>0;
    for(const auto& Mat:R->Dropped->GetStaticMaterials())Drop&=SlotIndex(R->Body,Mat.MaterialSlotName)!=INDEX_NONE;
    Check(TEXT("v87_character_drop_collision_and_weapon_material_slots"),Drop);
    if(!Good)return Finish();
    TMap<FName,UAnimationAsset*> Legacy;
    Legacy.Add(TEXT("Idle"),U->GetIdleMotion());Legacy.Add(TEXT("Aim"),U->GetAimMotion());Legacy.Add(TEXT("Reload"),U->GetReloadMotion());
    Legacy.Add(TEXT("Walk"),U->GetWalkMotion());Legacy.Add(TEXT("Run"),U->GetRunMotion());
    Legacy.Add(TEXT("Fire"),U->Battle->FireAnimation);Legacy.Add(TEXT("Bolt"),U->Battle->BoltAnimation);
    Legacy.Add(TEXT("TrenchCycle"),U->Battle->TrenchCycleAnimation);Legacy.Add(TEXT("Grenade"),U->Battle->GrenadeAnimation);
    for(int32 I=0;I<U->Battle->DeathAnimations.Num();++I)Legacy.Add(I==0?TEXT("DeathBack"):I==1?TEXT("DeathFront"):TEXT("DeathSide"),U->Battle->DeathAnimations[I]);
    for(const TCHAR* Required:{TEXT("Idle"),TEXT("Aim"),TEXT("Reload")})Check(FString(TEXT("v87_character_required_motion_"))+Required,D.Motions.Contains(Required));
    for(const auto& Pair:D.Motions)
    {
        auto* Old=Legacy.Find(Pair.Key);auto* Seq=Load<UAnimSequence>(*R,Pair.Value);const auto* OldSeq=Old?Cast<UAnimSequenceBase>(*Old):nullptr;
        FString Family=OldSeq?OldSeq->GetName():FString();const int32 Version=Family.Find(TEXT("_V"),ESearchCase::CaseSensitive,ESearchDir::FromEnd);
        if(Version!=INDEX_NONE)Family=Family.Left(Version);
        const bool Valid=Seq&&OldSeq&&Seq->GetSkeleton()==R->Skeleton&&Seq->IsCompressedDataValid()
            &&!Seq->IsValidAdditive()&&!Seq->HasRootMotion()&&Seq->GetPlayLength()>0
            &&FMath::Abs(Seq->GetPlayLength()-OldSeq->GetPlayLength())<=.0001f&&Seq->GetName().StartsWith(Family+TEXT("_"))
            &&(!R->Forward.Contains(*Old)||R->Forward.FindChecked(*Old)==Seq)
            &&(!R->Reverse.Contains(Seq)||R->Reverse.FindChecked(Seq)==*Old);
        Check(TEXT("v87_character_actual_clip_and_legacy_duration_")+Pair.Key.ToString(),Valid);
        if(Valid){R->Clips.Add(Pair.Key,Seq);R->Forward.Add(*Old,Seq);R->Reverse.Add(Seq,*Old);}
    }
    if(!Good)return Finish();
    R->OldBody=U->Soldier->GetSkeletalMeshAsset();R->OldPhysics=U->Soldier->GetPhysicsAsset();R->OldClass=U->Soldier->GetAnimClass();
    R->OldRelative=U->Soldier->GetRelativeTransform();R->OldPlaying=FFrontlineTripoCharacterAccessV87::Playing(U);
    R->OldAnimationData=U->Soldier->AnimationData;R->Retain(R->OldAnimationData.AnimToPlay);
    R->OldInstance=U->Soldier->GetAnimInstance();R->Retain(R->OldInstance);
    R->OldIdentityTime=FFrontlineTripoCharacterAccessV87::Time(U);R->OldTransitions=U->MotionTransitions;
    R->Retain(R->OldBody);R->Retain(R->OldPhysics);R->Retain(R->OldClass);R->Retain(R->OldPlaying);
    for(int32 I=0;I<U->Soldier->GetNumMaterials();++I){auto* A=U->Soldier->GetMaterial(I);R->OldMaterials.Add(A);R->Retain(A);}
    for(const auto& Attached:U->Soldier->GetAttachChildren())if(auto* Child=Attached.Get();Child&&Child->GetAttachSocketName()==TEXT("weapon_rifle"))
        R->WeaponChildren.Add(FBinding::FAttachment{Child,Child->GetRelativeTransform()});
    if(auto* Node=U->Soldier->GetSingleNodeInstance()){R->OldTime=Node->GetCurrentTime();R->OldRate=Node->GetPlayRate();R->OldLoop=Node->IsLooping();R->OldPlayingFlag=Node->IsPlaying();}
    Registry()=MoveTemp(R);auto* Bound=Registry().Get();
    Rebuild(U,Bound->Body,Bound->Physics,UWarfareFrontlineAnimation::StaticClass(),D.MeshRelativeTransform);++Bound->Rebuilds;
    auto* New=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance());
    Check(TEXT("v87_character_new_proxy_bone_snapshots_empty"),New&&New!=Bound->OldInstance&&New->GetCurrentAsset()==nullptr
        &&New->GetCharacterDiagnostics().SnapshotRemaps==0);
    if(!Good){Restore(U,External);return Finish();}
    FFrontlineTripoCharacterAccessV87::Set(U,Bound->Clips.FindChecked(TEXT("Idle")));Pose(U,0);
    Check(TEXT("v87_character_actual_bound_idle_and_muzzle"),U->Soldier->GetSingleNodeInstance()->GetCurrentAsset()==Bound->Clips.FindChecked(TEXT("Idle"))
        &&U->Soldier->DoesSocketExist(TEXT("WF_Muzzle"))&&!U->GetMuzzleLocation().ContainsNaN());
    if(!Good){Restore(U,External);return Finish();}
    J->SetBoolField(TEXT("executed"),true);J->SetObjectField(TEXT("binding"),Describe(U));
#else
    Check(TEXT("v87_character_editor_only"),false);
#endif
    return Finish();
}

TSharedRef<FJsonObject> FrontlineTripoCharacterV87::Restore(AWarfareFrontlineSoldier* U,TFunctionRef<void(const FString&,bool)> Check)
{
    auto J=Result(TEXT("Restore the captured existing actor binding; no asset/profile writes."));bool Good=true;
#if WITH_EDITOR
    auto* R=Registry().Get();if(!R||R->Unit.Get()!=U||!U||!U->Soldier){J->SetBoolField(TEXT("success"),false);return J;}
    for(const auto& A:R->WeaponChildren)if(auto* Child=A.Child.Get())
    {Child->AttachToComponent(U->Soldier,FAttachmentTransformRules::KeepRelativeTransform,TEXT("weapon_rifle"));Child->SetRelativeTransform(A.Relative);}
    if(auto* Drop=R->Released.Get())Drop->DestroyComponent();
    Rebuild(U,R->OldBody,R->OldPhysics,R->OldClass,R->OldRelative,&R->OldAnimationData);
    for(int32 I=0;I<R->OldMaterials.Num();++I)U->Soldier->SetMaterial(I,R->OldMaterials[I]);
    if(auto* Node=U->Soldier->GetSingleNodeInstance())if(R->OldPlaying)
    {Node->SetAnimationAsset(R->OldPlaying,R->OldLoop,R->OldRate);Node->SetPosition(R->OldTime,false);Node->SetPlaying(R->OldPlayingFlag);}
    FFrontlineTripoCharacterAccessV87::RestoreMotionIdentity(U,R->OldPlaying,R->OldIdentityTime);U->MotionTransitions=R->OldTransitions;Pose(U,0);
    Good=U->Soldier->GetSkeletalMeshAsset()==R->OldBody&&U->Soldier->GetPhysicsAsset()==R->OldPhysics
        &&U->Soldier->GetRelativeTransform().Equals(R->OldRelative,.0001f)&&U->Soldier->GetAnimClass()==R->OldClass;
    for(int32 I=0;I<R->OldMaterials.Num();++I)Good&=U->Soldier->GetMaterial(I)==R->OldMaterials[I];
    Registry().Reset();Check(TEXT("v87_character_original_binding_restored"),Good);J->SetBoolField(TEXT("executed"),true);
#endif
    J->SetBoolField(TEXT("success"),Good);return J;
}

TSharedRef<FJsonObject> FrontlineTripoCharacterV87::Describe(const AWarfareFrontlineSoldier* U)
{
    auto J=Result(TEXT("Read actual current candidate mesh/skeleton/clip; no load or mutation."));
#if WITH_EDITOR
    if(auto* R=Find(U))
    {
        J->SetBoolField(TEXT("executed"),true);J->SetStringField(TEXT("id"),R->Id);J->SetStringField(TEXT("acceptance_receipt"),R->Receipt);
        J->SetStringField(TEXT("body"),R->Body->GetPathName());J->SetStringField(TEXT("skeleton"),R->Skeleton->GetPathName());
        J->SetStringField(TEXT("physics"),R->Physics->GetPathName());J->SetStringField(TEXT("casualty"),R->Casualty->GetPathName());
        J->SetStringField(TEXT("detached_limb"),R->Limb->GetPathName());J->SetStringField(TEXT("casualty_cut_bone"),R->CutBone.ToString());
        J->SetNumberField(TEXT("bones"),R->Body->GetRefSkeleton().GetNum());J->SetNumberField(TEXT("animation_proxy_rebuilds"),R->Rebuilds);
        J->SetNumberField(TEXT("unsupported_motion_requests_rejected"),R->RejectedMotions);
        if(auto* Node=U->Soldier->GetSingleNodeInstance())J->SetStringField(TEXT("actual_clip"),GetPathNameSafe(Node->GetCurrentAsset()));
        if(auto* Node=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
        {J->SetNumberField(TEXT("actual_grip_constraint_frames"),Node->GetGripConstraintFrames());J->SetNumberField(TEXT("actual_maximum_grip_residual_cm"),Node->GetMaximumGripResidualCm());}
        J->SetStringField(TEXT("actual_muzzle"),U->GetMuzzleLocation().ToString());
    }
#endif
    return J;
}

bool FrontlineTripoCharacterV87::TryReleaseWeapon(AWarfareFrontlineSoldier* U,bool Explosive,const FVector& ForceDirection,UStaticMeshComponent*& Out)
{
    Out=nullptr;
#if WITH_EDITOR
    if(auto* R=Find(U))
    {
        if(auto* Existing=R->Released.Get()){Out=Existing;return true;}
        if(!U->GetWorld()||!U->Soldier->DoesSocketExist(TEXT("weapon_rifle")))return true;
        auto* C=NewObject<UStaticMeshComponent>(U);U->AddInstanceComponent(C);C->ComponentTags.Add(DroppedTag);
        C->SetStaticMesh(R->Dropped);C->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);C->SetCollisionObjectType(ECC_PhysicsBody);
        C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
        C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(false);C->RegisterComponent();C->SetWorldTransform(U->Soldier->GetSocketTransform(TEXT("weapon_rifle")));
        for(int32 I=0;I<R->Dropped->GetStaticMaterials().Num();++I)C->SetMaterial(I,U->Soldier->GetMaterial(U->Soldier->GetMaterialIndex(R->Dropped->GetStaticMaterials()[I].MaterialSlotName)));
        for(const auto& A:R->WeaponChildren)if(auto* Child=A.Child.Get())Child->AttachToComponent(C,FAttachmentTransformRules::KeepWorldTransform);
        // The single NCO retains the existing rifle release numbers.
        auto* Contact=NewObject<UPhysicalMaterial>(C);Contact->Friction=.62f;Contact->Restitution=.04f;C->SetPhysMaterialOverride(Contact);
        C->SetMassOverrideInKg(NAME_None,4.2f,true);C->SetLinearDamping(.5f);C->SetAngularDamping(.9f);C->SetSimulatePhysics(true);
        const FVector Kick=Explosive?ForceDirection.GetSafeNormal()*230.f+FVector(0,0,80):FVector(0,0,-20);
        C->SetPhysicsLinearVelocity(U->GetActorForwardVector()*FMath::Min(U->GetMotionSpeed(),280.f)+Kick);
        const float Phase=U->GetMotionPhase();C->SetPhysicsAngularVelocityInDegrees(FVector(25+Phase*45,(Phase-.5f)*100,20-Phase*40)*(Explosive?2.f:1.f));
        U->Soldier->HideBoneByName(TEXT("weapon_rifle"),EPhysBodyOp::PBO_None);R->Released=C;Out=C;return true;
    }
#endif
    return false;
}

TSharedRef<FJsonObject> FrontlineTripoCharacterV87::RunAimReloadChecks(AWarfareFrontlineSoldier* U,AWarfareFrontlineSoldier* Enemy,
    TFunctionRef<void(const FString&,bool)> External)
{
    auto J=Result(TEXT("Actual single UK NCO StartReload + Actor Tick + evaluated candidate poses. No discharge, natural campaign, death/limb or frame-time acceptance."));
    bool Good=true;TArray<TSharedPtr<FJsonValue>> Checks,Timeline;
    const auto Check=[&](const FString& N,bool P){External(N,P);Good&=P;auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),N);C->SetBoolField(TEXT("passed"),P);Checks.Add(MakeShared<FJsonValueObject>(C));};
    const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("timeline"),Timeline);return J;};
#if WITH_EDITOR
    Check(TEXT("v87_character_bound_real_disposable_pair"),IsBound(U)&&Enemy&&Enemy->ActorHasTag(DisposableTag)&&Enemy->Battle==U->Battle
        &&Enemy->Team!=U->Team&&Enemy->IsAlive()&&U->Battle->Phase==EFrontlinePhase::Fighting&&U->Battle->HasSight(U,Enemy)
        &&(Enemy->Feet().X-U->Feet().X)*U->Battle->GetTeamDirection(U->Team)>100
        &&U->WeaponPhase==EFrontlineWeaponPhase::Ready&&U->RoundsRemaining==U->GetMagazineCapacity());
    if(!Good)return Finish();
    Check(TEXT("v87_character_getters_and_setmotion_hooks_required"),U->GetIdleMotion()==Motion(U,TEXT("Idle"))
        &&U->GetAimMotion()==Motion(U,TEXT("Aim"))&&U->GetReloadMotion()==Motion(U,TEXT("Reload")));
    if(!Good)return Finish();
    const int32 Started=U->ReloadsStarted,Completed=U->ReloadsCompleted,Shots=U->ShotsDischarged,BlockedShots=U->ShotsDuringReload;
    const FVector ActorBefore=U->GetActorLocation();const bool TickWasEnabled=U->IsActorTickEnabled();U->SetActorTickEnabled(false);
    ON_SCOPE_EXIT{U->SetActorTickEnabled(TickWasEnabled);};
    FFrontlineTripoCharacterAccessV87::Prepare(U,Enemy);FFrontlineTripoCharacterAccessV87::Set(U,U->GetAimMotion());Pose(U,0);
    for(int32 I=0;I<16;++I){U->Tick(1.f/120.f);Pose(U,1.f/120.f);}
    Check(TEXT("v87_character_actual_aim_before_reload"),U->Soldier->GetSingleNodeInstance()->GetCurrentAsset()==Motion(U,TEXT("Aim")));
    if(!Good)return Finish();
    U->RoundsRemaining=0;FFrontlineTripoCharacterAccessV87::Reload(U);Pose(U,0);
    Check(TEXT("v87_character_real_ammo0_reload_start"),U->WeaponPhase==EFrontlineWeaponPhase::Reload&&U->ReloadsStarted==Started+1
        &&U->Soldier->GetSingleNodeInstance()->GetCurrentAsset()==Motion(U,TEXT("Reload"))&&FFrontlineTripoCharacterAccessV87::ReloadClock(U)>0);
    if(!Good)return Finish();
    const float DT=1.f/120.f,Duration=FFrontlineTripoCharacterAccessV87::ReloadClock(U);
    const int32 Limit=FMath::Clamp(FMath::CeilToInt((Duration+1.f)/DT),1,1200);bool Finite=true,ReloadSeen=false;int32 Ticks=0;
    for(;Ticks<Limit;++Ticks)
    {
        U->Tick(DT);Pose(U,DT);ReloadSeen|=U->WeaponPhase==EFrontlineWeaponPhase::Reload;Finite&=!U->GetMuzzleLocation().ContainsNaN();
        if(Ticks%12==0||U->WeaponPhase!=EFrontlineWeaponPhase::Reload)
        {auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("seconds"),(Ticks+1)*DT);P->SetNumberField(TEXT("phase"),int32(U->WeaponPhase));P->SetNumberField(TEXT("ammo"),U->RoundsRemaining);
            P->SetStringField(TEXT("actual_clip"),GetPathNameSafe(U->Soldier->GetSingleNodeInstance()->GetCurrentAsset()));P->SetStringField(TEXT("muzzle"),U->GetMuzzleLocation().ToString());Timeline.Add(MakeShared<FJsonValueObject>(P));}
        if(U->WeaponPhase==EFrontlineWeaponPhase::Ready)break;
    }
    Check(TEXT("v87_character_real_reload_finishes_with_full_mag"),ReloadSeen&&U->WeaponPhase==EFrontlineWeaponPhase::Ready
        &&U->ReloadsCompleted==Completed+1&&U->RoundsRemaining==U->GetMagazineCapacity());
    Check(TEXT("v87_character_actual_candidate_aim_after_reload"),U->Soldier->GetSingleNodeInstance()->GetCurrentAsset()==Motion(U,TEXT("Aim")));
    Check(TEXT("v87_character_no_shot_or_actor_translation_during_reload"),U->ShotsDischarged==Shots&&U->ShotsDuringReload==BlockedShots&&U->GetActorLocation().Equals(ActorBefore,.01f));
    const auto Diagnostics=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance())->GetCharacterDiagnostics();
    Check(TEXT("v87_character_actual_finite_pose_and_required_hierarchy"),Finite&&Diagnostics.InvalidBoneFrames==0&&Diagnostics.UnsafeTorsoHierarchyFrames==0);
    J->SetBoolField(TEXT("executed"),true);J->SetNumberField(TEXT("actual_ticks"),Ticks+1);J->SetNumberField(TEXT("initial_real_reload_clock"),Duration);
    J->SetObjectField(TEXT("binding"),Describe(U));
#else
    Check(TEXT("v87_character_editor_only"),false);
#endif
    return Finish();
}
