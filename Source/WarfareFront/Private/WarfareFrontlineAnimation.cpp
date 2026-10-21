#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineAppearanceV29.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineChoiceTargetsV78.h"
#include "WarfareFrontlineGameplayV49.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineMotionV34.h"
#include "WarfareFrontlineMotionV38.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineEquipmentV41.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontlineMovementV28.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineArmourV63.h"
#if WITH_EDITOR
#include "WarfareFrontlineTripoCharacterV87.h"
#include "WarfareFrontlineBlockedFireProbeV87.h"
#endif
#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TwoBoneIK.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool FrontlineUseLegacyArmourYieldV84411();
bool FrontlineArmourYieldNeedsExactArrivalV86(const AWarfareFrontlineSoldier* U,const FVector& Goal);
bool FrontlineUseLegacyContactRetentionV87();

namespace FrontlineSupportReuseV834
{
struct FReview{uint64 TickLookups=0,Requests=0,PointerChecks=0,PointerFailures=0;};
FReview& State(){static FReview S;return S;}
bool Enabled(const AWarfareFrontlineGameMode* B)
{
    static const bool Requested=FParse::Param(FCommandLine::Get(),TEXT("WFV834Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    return Requested&&B&&B->IsProgressionVolatile();
}
TSharedPtr<FJsonObject> Review()
{
    const auto& S=State();auto J=MakeShared<FJsonObject>();
    J->SetNumberField(TEXT("tick_local_lookups"),double(S.TickLookups));
    J->SetNumberField(TEXT("original_explicit_requests"),double(S.Requests));
    J->SetNumberField(TEXT("avoided_explicit_lookups"),double(S.Requests-S.TickLookups));
    J->SetNumberField(TEXT("same_frame_pointer_checks"),double(S.PointerChecks));
    J->SetNumberField(TEXT("same_frame_pointer_failures"),double(S.PointerFailures));
    return J;
}
}

namespace
{
FCompactPoseBoneIndex FindCompactBone(const FBoneContainer& Bones,const TCHAR* Name)
{
    const int32 MeshIndex=Bones.GetPoseBoneIndexForBoneName(FName(Name));
    return MeshIndex==INDEX_NONE?FCompactPoseBoneIndex(INDEX_NONE):Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
}

bool ValidCompactBone(const FCompactPose& Pose,FCompactPoseBoneIndex Bone)
{
    return Bone.GetInt()>=0 && Bone.GetInt()<Pose.GetNumBones();
}

FVector ProjectBend(const FVector& Direction,const FVector& Axis)
{
    return Direction-Axis*FVector::DotProduct(Direction,Axis);
}

/** Snapshot transitions keep the current on-screen pose even when an order interrupts a blend.
 * All compact-pose access stays on the animation proxy. The approved finger poses are
 * untouched; a no-stretch arm solve keeps their hand attached to the rifle during blends.
 */
struct FFrontlineAnimationProxy final : FAnimSingleNodeInstanceProxy
{
    explicit FFrontlineAnimationProxy(UAnimInstance* Instance) : FAnimSingleNodeInstanceProxy(Instance) {}
    TArray<FTransform> DisplayedPose,PreviousDisplayedPoseV65,TransitionSource;
    TArray<int32> PreviousMeshBonesV65;
    TMap<int32,FVector> TransitionVelocityV65;
    TSet<int32> MomentumMeshBonesV65;
    uint64 PoseStepV65=0,PublishedStepV65=MAX_uint64;
    float PoseDeltaV65=0,PreviousPoseDeltaV65=0;
    float BurningTimeV65=0;bool bBurnDistressV65=false;
    float GasTimeV8215=0,GasClipLengthV8215=0,GasDistressV8215=0,HeavyRecoilAgeV8215=10;
    bool bGasFatalV8215=false,bGasEscapeV8216=false;
    TArray<int32> DisplayedMeshBoneIndices,TransitionMeshBoneIndices;
    TMap<int32,FTransform> RagdollLocalPose;
    bool RagdollPoseFrozen=false;
    float BlendTime=0,BlendDuration=0;
    int32 GripConstraintFrames=0;
    float MaximumGripResidualCm=0,MaximumGripCorrectionCm=0;
    FName DisplayedAssetName,TransitionFrom;
    FName WalkAssetNameV46,RunAssetNameV46;
    float DisplayedLocomotionWeightV46=0,TransitionLocomotionWeightV46=0;
    bool PrepareDeathEntry=false;
    float DeathEntryTime=1.f,DeathPelvisOffset=0.f;
    float PersonalPhase=0,PersonalTime=0;
    float TrenchReloadShelterV8447=0,TrenchReloadLowerV8447=0;
    bool bMountedTrenchReloadV8447=false;
    bool bMountedWeaponTargetV8447=false;FTransform MountedWeaponTargetCSV8447=FTransform::Identity;
    FFrontlineIdleStateV38 IdleStateV38;
    FFrontlineIdlePoseV38 IdlePoseV38;
    bool bPersonalReadyPose=false,bPersonalAiming=false,bPersonalAutomatic=false,bPersonalLocomotion=false,bPersonalPistol=false,bPersonalTrenchIdle=false;
    bool bNationalAutomatic=false,bNationalGroundTargetV82=false;
    WarfareFRRUPresentationV82::FGroundGunPoseTargetV82 NationalGroundTargetV82;
    FQuat GroundGunTorsoOffsetV82=FQuat::Identity;
    FName GroundGunTorsoAssetV82=NAME_None;
    bool bGroundGunTorsoAcceptedV82=false;
    float GroundGunLastDiagnosticTimeV82=-1000.f;
    int32 PersonalRole=0;
    bool bTerrainFeet=false;
    float TerrainFootDelta[2]={0,0};
    FVector TerrainUp=FVector::UpVector;
    FVector PreviousElbowBend[2]={FVector::ZeroVector,FVector::ZeroVector};
    FFrontlineAnimationV29Diagnostics Diagnostics;

    virtual void PreUpdate(UAnimInstance* Instance,float DeltaSeconds) override
    {
        FAnimSingleNodeInstanceProxy::PreUpdate(Instance,DeltaSeconds);
        // Copy actor state on the game thread. Evaluate only consumes proxy data,
        // so the breathing layer remains safe with parallel animation evaluation.
        const auto* Unit=Cast<AWarfareFrontlineSoldier>(Instance?Instance->GetOwningActor():nullptr);
        WalkAssetNameV46=Unit&&Unit->GetWalkMotion()?Unit->GetWalkMotion()->GetFName():NAME_None;
        RunAssetNameV46=Unit&&Unit->GetRunMotion()?Unit->GetRunMotion()->GetFName():NAME_None;
        if(MomentumMeshBonesV65.IsEmpty()&&Unit&&Unit->Soldier&&Unit->Soldier->GetSkeletalMeshAsset())
        {
            const auto& Ref=Unit->Soldier->GetSkeletalMeshAsset()->GetRefSkeleton();
            for(int32 I=0;I<Ref.GetNum();++I){const FString N=Ref.GetBoneName(I).ToString();
                if(N.StartsWith(TEXT("spine_"))||N.StartsWith(TEXT("clavicle_"))||N.StartsWith(TEXT("upperarm_"))||N.StartsWith(TEXT("lowerarm_"))||N==TEXT("Head"))MomentumMeshBonesV65.Add(I);}
        }
        PersonalTime+=DeltaSeconds;
        IdlePoseV38=FrontlineMotionV38::UpdateIdle(IdleStateV38,Unit,CurrentAsset,DeltaSeconds);
        PersonalPhase=Unit?Unit->GetMotionPhase():0.f;
        bBurnDistressV65=Unit&&!Unit->IsAlive()&&Unit->GetDeathState().Kind==EFrontlineDamageKind::Fire&&!Unit->GetDeathState().bPhysicsStarted;
        BurningTimeV65=Unit&&Unit->Soldier&&Unit->Soldier->GetSingleNodeInstance()?Unit->Soldier->GetSingleNodeInstance()->GetCurrentTime():0.f;
        bGasFatalV8215=Unit&&!Unit->IsAlive()&&Unit->GetDeathState().Kind==EFrontlineDamageKind::Gas&&!Unit->GetDeathState().bPhysicsStarted;
        GasTimeV8215=BurningTimeV65;
        GasClipLengthV8215=bGasFatalV8215?Unit->GetDeathState().AuthoredClipSeconds:0;
        const auto GasExposure=FrontlineMotionV40::GasExposureV8215(Unit);
        GasDistressV8215=Unit&&Unit->IsAlive()&&GasExposure.Concentration>0?GasExposure.Distress:0;
        bGasEscapeV8216=Unit&&Unit->IsAlive()&&GasExposure.Escaping;
        HeavyRecoilAgeV8215=Unit&&Unit->IsAlive()?Unit->HeavyRecoilAgeV8215():10.f;
        bPersonalAiming=Unit && CurrentAsset==Unit->GetAimMotion();
        bPersonalAutomatic=Unit && Unit->IsMachineGunner();
        bNationalAutomatic=bPersonalAutomatic && WarfareFRRUCampaignV82::Active(Unit->Battle);
        bPersonalPistol=Unit && (WarfareFRRUCampaignV82::Active(Unit->Battle)?WarfareFRRUPresentationV82::UsesPistol(Unit):Unit->IsCommissionedOfficer());
        PersonalRole=Unit?Unit->GetPresentationRoleIndex()%6:0;
        bPersonalTrenchIdle=Unit && Unit->Battle && Unit->Battle->CoverFor(Unit)>0.f && !Unit->bEngaging && !Unit->bMoving;
        bPersonalLocomotion=Unit && Unit->IsAlive() && Unit->bMoving
            && Unit->WeaponPhase==EFrontlineWeaponPhase::Ready
            && (CurrentAsset==Unit->GetWalkMotion() || CurrentAsset==Unit->GetRunMotion());
        bPersonalReadyPose=Unit && Unit->IsAlive() && !Unit->bMoving
            && Unit->WeaponPhase==EFrontlineWeaponPhase::Ready
            && (bPersonalAiming || CurrentAsset==Unit->GetIdleMotion());
        const auto* Gun=FrontlineMachinegunsV30::Get(Unit);
        TrenchReloadShelterV8447=Unit&&Unit->Battle&&!WarfareFRRUCampaignV82::Active(Unit->Battle)&&Unit->Battle->CoverFor(Unit)>0.f
            ?FrontlineBalanceV50::TrenchReloadShelterBlendV8447(Unit):0.f;
        // Low supported reloads already bend the legs. Their small extra tuck
        // avoids burying the pelvis while the standing rifle posture lowers.
        TrenchReloadLowerV8447=Gun&&(Gun->IsProne()||FrontlineMachinegunsV30::UsesLowTrenchPoseV64(Unit))?5.f:16.f;
        bMountedTrenchReloadV8447=Unit&&Unit->IsMachineGunner()&&Gun&&Gun->IsSupported()&&Gun->IsTrench()
            &&Unit->WeaponPhase==EFrontlineWeaponPhase::Reload&&CurrentAsset==Unit->GetReloadMotion();
        FTransform MountedWorld;
        bMountedWeaponTargetV8447=Gun&&Gun->ReadSupportedWeaponAnchorV8447(MountedWorld);
        if(bMountedWeaponTargetV8447)MountedWeaponTargetCSV8447=MountedWorld.GetRelativeTransform(Unit->Soldier->GetComponentTransform());
        bNationalGroundTargetV82=false;
        if(bNationalAutomatic)
            bNationalGroundTargetV82=WarfareFRRUPresentationV82::GroundGunPoseTargetV82(Unit,CurrentAsset,NationalGroundTargetV82);
        if(!bNationalGroundTargetV82)
        {bGroundGunTorsoAcceptedV82=false;GroundGunTorsoOffsetV82=FQuat::Identity;GroundGunTorsoAssetV82=NAME_None;}
        bTerrainFeet=Unit && Unit->IsAlive() && CurrentAsset && Unit->Soldier
            && (!Unit->Battle || Unit->Battle->BattleOutroWinner<0)
            && (!Gun || (!Gun->IsProne() && !Gun->TransitionMotion()));
        if(bTerrainFeet)
        {
            // Sample world terrain on the game thread. The worker-side pose
            // solve consumes only these two offsets, never an actor/world query.
            TerrainUp=Unit->Soldier->GetComponentTransform().InverseTransformVectorNoScale(FVector::UpVector);
            for(int32 Side=0;Side<2;++Side)
            {
                const FVector Foot=Unit->Soldier->GetBoneLocation(Side?TEXT("foot_r"):TEXT("foot_l"));
                const float Target=FMath::Clamp(FrontlineInfantryV45::SurfaceHeight(Unit->Battle,Foot)-float(Unit->Feet().Z),-32.f,32.f);
                TerrainFootDelta[Side]=FMath::FInterpConstantTo(TerrainFootDelta[Side],Target,DeltaSeconds,460.f);
            }
        }
        else TerrainFootDelta[0]=TerrainFootDelta[1]=0;
    }

    void FreezeForRagdoll()
    {
        if (DisplayedPose.Num()!=DisplayedMeshBoneIndices.Num() || DisplayedPose.IsEmpty()) return;
        RagdollLocalPose.Reset();
        for (int32 I=0;I<DisplayedPose.Num();++I) RagdollLocalPose.Add(DisplayedMeshBoneIndices[I],DisplayedPose[I]);
        RagdollPoseFrozen=true;
    }

    void BeginTransition(float Seconds)
    {
        if (BlendDuration>0 && BlendTime<BlendDuration) ++Diagnostics.InterruptedTransitions;
        TransitionSource=DisplayedPose;
        TransitionMeshBoneIndices=DisplayedMeshBoneIndices;
        TransitionFrom=DisplayedAssetName;
        TransitionVelocityV65.Reset();
        // Carry the outgoing angular velocity for a short, bounded decay. The
        // old frozen-snapshot blend visibly stopped elbows/torso at every cut.
        // Bone identities, not compact indexes, survive an LOD remap.
        if(PreviousPoseDeltaV65>.001f&&PreviousPoseDeltaV65<.1f&&PreviousDisplayedPoseV65.Num()==DisplayedPose.Num()
            &&PreviousMeshBonesV65==DisplayedMeshBoneIndices)
            for(int32 I=0;I<DisplayedPose.Num();++I)
            {
                if(!MomentumMeshBonesV65.Contains(DisplayedMeshBoneIndices[I]))continue;
                FQuat Delta=DisplayedPose[I].GetRotation()*PreviousDisplayedPoseV65[I].GetRotation().Inverse();Delta.Normalize();
                if(Delta.W<0)Delta=Delta*-1.;FVector Axis;double Angle=0;Delta.ToAxisAndAngle(Axis,Angle);
                if(!Axis.ContainsNaN()&&FMath::IsFinite(Angle))TransitionVelocityV65.Add(DisplayedMeshBoneIndices[I],Axis*(FMath::Min(Angle/PreviousPoseDeltaV65,8.)));
            }
        TransitionLocomotionWeightV46=DisplayedLocomotionWeightV46;
        BlendTime=0; BlendDuration=FMath::Max(0.f,Seconds);
        PrepareDeathEntry=true; DeathEntryTime=0; DeathPelvisOffset=0;
    }

    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimSingleNodeInstanceProxy::Initialize(Instance);
        DisplayedPose.Reset(); TransitionSource.Reset(); DisplayedMeshBoneIndices.Reset(); TransitionMeshBoneIndices.Reset(); BlendTime=BlendDuration=0;
        GripConstraintFrames=0; MaximumGripResidualCm=MaximumGripCorrectionCm=0;
        DisplayedAssetName=TransitionFrom=NAME_None;
        WalkAssetNameV46=RunAssetNameV46=NAME_None;DisplayedLocomotionWeightV46=TransitionLocomotionWeightV46=0;
        RagdollLocalPose.Reset(); RagdollPoseFrozen=false;
        PreviousDisplayedPoseV65.Reset();PreviousMeshBonesV65.Reset();TransitionVelocityV65.Reset();MomentumMeshBonesV65.Reset();PoseStepV65=0;PublishedStepV65=MAX_uint64;PoseDeltaV65=PreviousPoseDeltaV65=0;
        PreviousElbowBend[0]=PreviousElbowBend[1]=FVector::ZeroVector;
        bTerrainFeet=false;TerrainFootDelta[0]=TerrainFootDelta[1]=0;
        bNationalGroundTargetV82=false;NationalGroundTargetV82=WarfareFRRUPresentationV82::FGroundGunPoseTargetV82();
        Diagnostics=FFrontlineAnimationV29Diagnostics();
    }

    void RemapTransitionSource(const FCompactPose& Pose)
    {
        if (TransitionSource.IsEmpty()) return;
        const FBoneContainer& Bones=Pose.GetBoneContainer();
        bool bSame=TransitionSource.Num()==Pose.GetNumBones() && TransitionSource.Num()==TransitionMeshBoneIndices.Num();
        if (bSame)
            for (FCompactPoseBoneIndex Bone:Pose.ForEachBoneIndex())
                if (TransitionMeshBoneIndices[Bone.GetInt()]!=Bones.MakeMeshPoseIndex(Bone).GetInt()) { bSame=false; break; }
        if (bSame) return;
        // LOD / required-bone changes can keep the same count but change compact
        // indices. A pose snapshot is a mesh-bone identity, never an array offset.
        TMap<int32,FTransform> Previous;
        for (int32 I=0;I<FMath::Min(TransitionSource.Num(),TransitionMeshBoneIndices.Num());++I)
            Previous.Add(TransitionMeshBoneIndices[I],TransitionSource[I]);
        TransitionSource.SetNum(Pose.GetNumBones()); TransitionMeshBoneIndices.SetNum(Pose.GetNumBones());
        for (FCompactPoseBoneIndex Bone:Pose.ForEachBoneIndex())
        {
            const int32 MeshIndex=Bones.MakeMeshPoseIndex(Bone).GetInt();
            const FTransform* Old=Previous.Find(MeshIndex);
            TransitionSource[Bone.GetInt()]=Old?*Old:Pose[Bone];
            TransitionMeshBoneIndices[Bone.GetInt()]=MeshIndex;
        }
        ++Diagnostics.SnapshotRemaps;
    }

    FVector StableElbowTarget(const FTransform& Upper,const FTransform& Lower,const FTransform& Hand,
        const FVector& Target,const FVector& EndpointBend,int32 Side)
    {
        const FVector Root=Upper.GetLocation();
        const FVector Axis=(Target-Root).GetSafeNormal();
        FVector Bend=ProjectBend(Lower.GetLocation()-Root,Axis);
        // An almost straight elbow does not define a stable rotation plane.
        // Preserve its previous anatomical plane, then the authored endpoint;
        // only use a deterministic anatomical down/out fallback for a fresh pose.
        if (Bend.SizeSquared()<9.0)
        {
            ++Diagnostics.PoleFallbackFrames;
            Bend=ProjectBend(PreviousElbowBend[Side],Axis);
            if (Bend.SizeSquared()<.01) Bend=ProjectBend(EndpointBend,Axis);
            // Use the actual shoulder side in component space; imported left
            // arms need not share the engine mannequin's axis convention.
            if (Bend.SizeSquared()<.01) Bend=ProjectBend(FVector(Root.X<0?-.35:.35,0,-1),Axis);
            if (Bend.SizeSquared()<.01) Bend=ProjectBend(FVector(0,-1,0),Axis);
        }
        Bend.Normalize(); PreviousElbowBend[Side]=Bend;
        const double Reach=FVector::Dist(Root,Lower.GetLocation())+FVector::Dist(Lower.GetLocation(),Hand.GetLocation());
        return Root+Axis*FVector::Dist(Root,Target)*.45+Bend*FMath::Max(Reach*.7,20.0);
    }

    virtual void UpdateAnimationNode(const FAnimationUpdateContext& Context) override
    {
        ++PoseStepV65;PoseDeltaV65=Context.GetDeltaTime();
        BlendTime=FMath::Min(BlendDuration,BlendTime+Context.GetDeltaTime());
        DeathEntryTime+=Context.GetDeltaTime();
        FAnimSingleNodeInstanceProxy::UpdateAnimationNode(Context);
    }

    void OverheadRifleV45(FCompactPose& Local)
    {
        if(!CurrentAsset||CurrentAsset->GetFName()!=FName(TEXT("A_Victory_Rifle_RifleRaise_V41")))return;
        const auto Smooth=[](float T){T=FMath::Clamp(T,0.f,1.f);return T*T*(3.f-2.f*T);};
        const auto Pulse=[&](float Start,float Up,float Hold,float Down)
        {return Smooth((GetCurrentTime()-Start)/Up)*(1.f-Smooth((GetCurrentTime()-Start-Up-Hold)/Down));};
        const float Weight=FMath::Clamp(Pulse(.18f,.80f,.35f,.70f)+Pulse(2.65f,.65f,.45f,.80f),0.f,1.f);
        if(Weight<.0001f)return;
        const FBoneContainer& Bones=Local.GetBoneContainer();
        const auto Bone=[&](const TCHAR* Name){return FindCompactBone(Bones,Name);};
        const auto Weapon=Bone(TEXT("weapon_rifle")),Head=Bone(TEXT("Head")),Bolt=Bone(TEXT("weapon_bolt"));
        const FCompactPoseBoneIndex UpperIds[]={Bone(TEXT("upperarm_l")),Bone(TEXT("upperarm_r"))};
        const FCompactPoseBoneIndex LowerIds[]={Bone(TEXT("lowerarm_l")),Bone(TEXT("lowerarm_r"))};
        const FCompactPoseBoneIndex HandIds[]={Bone(TEXT("hand_l")),Bone(TEXT("hand_r"))};
        bool Valid=ValidCompactBone(Local,Weapon)&&ValidCompactBone(Local,Head);
        for(int32 S=0;S<2;++S)Valid&=ValidCompactBone(Local,UpperIds[S])&&ValidCompactBone(Local,LowerIds[S])&&ValidCompactBone(Local,HandIds[S]);
        if(!Valid){++Diagnostics.OverheadRejectedFramesV45;return;}
        FCSPose<FCompactPose> Pose;Pose.InitPose(Local);
        const FTransform Original=Pose.GetComponentSpaceTransform(Weapon);
        const FTransform BoltAnchor=ValidCompactBone(Local,Bolt)?Pose.GetComponentSpaceTransform(Bolt).GetRelativeTransform(Original):FTransform::Identity;
        FTransform Upper[2],Lower[2],Hand[2],Anchor[2];float ArmLength[2],UpperLength[2],LowerLength[2],MinimumReach[2];
        for(int32 S=0;S<2;++S)
        {
            Upper[S]=Pose.GetComponentSpaceTransform(UpperIds[S]);Lower[S]=Pose.GetComponentSpaceTransform(LowerIds[S]);Hand[S]=Pose.GetComponentSpaceTransform(HandIds[S]);
            Anchor[S]=Hand[S].GetRelativeTransform(Original);
            UpperLength[S]=FVector::Dist(Upper[S].GetLocation(),Lower[S].GetLocation());
            LowerLength[S]=FVector::Dist(Lower[S].GetLocation(),Hand[S].GetLocation());ArmLength[S]=UpperLength[S]+LowerLength[S];
            // A wrist passing too near its shoulder would fold the elbow beyond
            // the 155-degree limit. Leave two degrees of numerical margin.
            MinimumReach[S]=FMath::Sqrt(FMath::Square(UpperLength[S])+FMath::Square(LowerLength[S])
                +2.f*UpperLength[S]*LowerLength[S]*FMath::Cos(FMath::DegreesToRadians(153.f)));
        }
        // Move the complete rifle and both accepted grips as one rigid object.
        // An oblique cross-body axis retains a clear length in the side camera.
        const float LeftSign=Upper[0].GetLocation().X>Upper[1].GetLocation().X?1.f:-1.f;
        const FVector GripLine=(Hand[0].GetLocation()-Hand[1].GetLocation()).GetSafeNormal();
        const FVector RaisedLine=FVector(.65f*LeftSign,.76f,.10f).GetSafeNormal();
        FTransform Raised=Original;
        Raised.SetRotation((FQuat::FindBetweenNormals(GripLine,RaisedLine)*Original.GetRotation()).GetNormalized());
        // Head's joint is near the skull base, about 15 cm below its crown;
        // leave helmet clearance as well as lifting above that joint.
        const FVector Centre=Pose.GetComponentSpaceTransform(Head).GetLocation()+FVector(0,5,32);
        Raised.SetLocation(Centre-Raised.TransformVector((Anchor[0].GetLocation()+Anchor[1].GetLocation())*.5));
        FTransform W;W.Blend(Original,Raised,Weight);W.NormalizeRotation();
        // Project the common grip pair into both anatomical reach shells.
        // Translation preserves every finger/grip relation; bones never stretch.
        for(int32 Iteration=0;Iteration<16;++Iteration)for(int32 S=0;S<2;++S)
        {
            const FVector Target=(Anchor[S]*W).GetLocation(),Delta=Target-Upper[S].GetLocation();
            const float Reach=ArmLength[S]*.965f,Required=Delta.Size();
            if(Required>Reach)W.AddToTranslation(-Delta.GetSafeNormal()*(Required-Reach+.001f));
            else if(Required<MinimumReach[S])
                W.AddToTranslation((Required>UE_SMALL_NUMBER?Delta/Required:FVector(0,1,0))*(MinimumReach[S]-Required+.001f));
        }
        TArray<FBoneTransform,TInlineAllocator<8>> Changes;
        float GripError=0,ArmError=0,ShoulderMax=0,ElbowMax=0;
        for(int32 S=0;S<2;++S)
        {
            const FTransform Target=Anchor[S]*W;
            const float Side=S==0?LeftSign:-LeftSign;
            const FVector Pole=Upper[S].GetLocation()+FVector(Side*38,10,-8);
            AnimationCore::SolveTwoBoneIK(Upper[S],Lower[S],Hand[S],Pole,Target.GetLocation(),false,1.,1.);
            Hand[S].SetRotation(Target.GetRotation());
            const FVector A=(Lower[S].GetLocation()-Upper[S].GetLocation()).GetSafeNormal(),C=(Hand[S].GetLocation()-Lower[S].GetLocation()).GetSafeNormal();
            const float Shoulder=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(FVector::DotProduct(A,-FVector::UpVector)),-1.f,1.f)));
            const float Elbow=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(FVector::DotProduct(A,C)),-1.f,1.f)));
            GripError=FMath::Max(GripError,float(FVector::Dist(Hand[S].GetLocation(),Target.GetLocation())));
            ArmError=FMath::Max(ArmError,FMath::Max(FMath::Abs(float(FVector::Dist(Upper[S].GetLocation(),Lower[S].GetLocation()))-UpperLength[S]),FMath::Abs(float(FVector::Dist(Lower[S].GetLocation(),Hand[S].GetLocation()))-LowerLength[S])));
            ShoulderMax=FMath::Max(ShoulderMax,Shoulder);ElbowMax=FMath::Max(ElbowMax,Elbow);
            Valid&=!Upper[S].ContainsNaN()&&!Lower[S].ContainsNaN()&&!Hand[S].ContainsNaN()&&Shoulder<=170.f&&Elbow<=155.f;
            Changes.Add(FBoneTransform(UpperIds[S],Upper[S]));Changes.Add(FBoneTransform(LowerIds[S],Lower[S]));Changes.Add(FBoneTransform(HandIds[S],Hand[S]));
        }
        if(!Valid||W.ContainsNaN()||GripError>.15f||ArmError>.05f){++Diagnostics.OverheadRejectedFramesV45;return;}
        Changes.Add(FBoneTransform(Weapon,W));if(ValidCompactBone(Local,Bolt))Changes.Add(FBoneTransform(Bolt,BoltAnchor*W));
        Changes.Sort([](const FBoneTransform& A,const FBoneTransform& B){return A.BoneIndex.GetInt()<B.BoneIndex.GetInt();});
        Pose.LocalBlendCSBoneTransforms(MakeArrayView(Changes),1.f);
        for(int32 S=0;S<2;++S)
            GripError=FMath::Max(GripError,float(FVector::Dist(Pose.GetComponentSpaceTransform(HandIds[S]).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation(),Anchor[S].GetLocation())));
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Local);
        ++Diagnostics.OverheadPoseFramesV45;
        Diagnostics.MaximumOverheadGripErrorCmV45=FMath::Max(Diagnostics.MaximumOverheadGripErrorCmV45,GripError);
        Diagnostics.MaximumArmLengthErrorCm=FMath::Max(Diagnostics.MaximumArmLengthErrorCm,ArmError);
        Diagnostics.MaximumOverheadShoulderDegreesV45=FMath::Max(Diagnostics.MaximumOverheadShoulderDegreesV45,ShoulderMax);
        Diagnostics.MaximumOverheadElbowDegreesV45=FMath::Max(Diagnostics.MaximumOverheadElbowDegreesV45,ElbowMax);
    }

    bool NationalCarryGrip(FCompactPose& Local,const FTransform& LeftGrip,const FTransform& RightGrip)
    {
        // Actual Galicia/Mort exposed unreachable wrists during planted exits.
        // Move the whole carried gun and both grips together; planted
        // support, feed/pack work, other nations and UK/DE remain authored.
        if(!bNationalAutomatic||!CurrentAsset)return false;
        const FString Previous=TransitionFrom.ToString(),Current=CurrentAsset->GetName();bool ProvenFamily=false;
        for(const TCHAR* Family:{TEXT("at_hu_schwarzlose"),TEXT("fr_hotchkiss"),TEXT("ru_maxim")})
            ProvenFamily|=Previous==FString(TEXT("A_"))+Family+TEXT("_Ground_Aim_N67_V82")
                &&(Current==FString(TEXT("A_"))+Family+TEXT("_Carry_Walk_N67_V82")||Current==FString(TEXT("A_"))+Family+TEXT("_Carry_Run_N67_V82"));
        if(!ProvenFamily)return false;
        const FBoneContainer& Bones=Local.GetBoneContainer();
        const auto Bone=[&](const TCHAR* Name){return FindCompactBone(Bones,Name);};
        const auto Weapon=Bone(TEXT("weapon_rifle")),Bolt=Bone(TEXT("weapon_bolt"));
        const FCompactPoseBoneIndex UpperIds[]={Bone(TEXT("upperarm_l")),Bone(TEXT("upperarm_r"))};
        const FCompactPoseBoneIndex LowerIds[]={Bone(TEXT("lowerarm_l")),Bone(TEXT("lowerarm_r"))};
        const FCompactPoseBoneIndex HandIds[]={Bone(TEXT("hand_l")),Bone(TEXT("hand_r"))};
        bool Valid=ValidCompactBone(Local,Weapon);
        for(int32 Side=0;Side<2;++Side)Valid&=ValidCompactBone(Local,UpperIds[Side])&&ValidCompactBone(Local,LowerIds[Side])
            &&ValidCompactBone(Local,HandIds[Side])&&Bones.GetParentBoneIndex(LowerIds[Side])==UpperIds[Side]
            &&Bones.GetParentBoneIndex(HandIds[Side])==LowerIds[Side];
        if(!Valid){++Diagnostics.NationalCarryGripRejectedFramesV82;return false;}
        FCSPose<FCompactPose> Pose;Pose.InitPose(Local);const FTransform Original=Pose.GetComponentSpaceTransform(Weapon);
        const FTransform BoltAnchor=ValidCompactBone(Local,Bolt)?Pose.GetComponentSpaceTransform(Bolt).GetRelativeTransform(Original):FTransform::Identity;
        const FTransform Grips[]={LeftGrip,RightGrip};FTransform Upper[2],Lower[2],Hand[2];float UpperLength[2],LowerLength[2],Maximum[2],Minimum[2];
        for(int32 Side=0;Side<2;++Side)
        {
            Upper[Side]=Pose.GetComponentSpaceTransform(UpperIds[Side]);Lower[Side]=Pose.GetComponentSpaceTransform(LowerIds[Side]);Hand[Side]=Pose.GetComponentSpaceTransform(HandIds[Side]);
            UpperLength[Side]=FVector::Dist(Upper[Side].GetLocation(),Lower[Side].GetLocation());LowerLength[Side]=FVector::Dist(Lower[Side].GetLocation(),Hand[Side].GetLocation());
            Maximum[Side]=.965f*(UpperLength[Side]+LowerLength[Side]);Minimum[Side]=FMath::Sqrt(FMath::Square(UpperLength[Side])+FMath::Square(LowerLength[Side])
                +2.f*UpperLength[Side]*LowerLength[Side]*FMath::Cos(FMath::DegreesToRadians(153.f)));
        }
        FTransform W=Original;
        for(int32 Iteration=0;Iteration<16;++Iteration)for(int32 Side=0;Side<2;++Side)
        {
            const FVector Delta=(Grips[Side]*W).GetLocation()-Upper[Side].GetLocation();const float Required=Delta.Size();
            if(Required>Maximum[Side])W.AddToTranslation(-Delta.GetSafeNormal()*(Required-Maximum[Side]+.001f));
            else if(Required<Minimum[Side])W.AddToTranslation((Required>UE_SMALL_NUMBER?Delta/Required:FVector(0,1,0))*(Minimum[Side]-Required+.001f));
        }
        const float Shift=FVector::Dist(W.GetLocation(),Original.GetLocation());float Residual=0,ArmError=0;
        TArray<FBoneTransform,TInlineAllocator<8>> Changes;
        for(int32 Side=0;Side<2;++Side)
        {
            const FTransform Target=Grips[Side]*W;const FVector Bend=Lower[Side].GetLocation()-Upper[Side].GetLocation();
            const FVector Pole=StableElbowTarget(Upper[Side],Lower[Side],Hand[Side],Target.GetLocation(),Bend,Side);
            AnimationCore::SolveTwoBoneIK(Upper[Side],Lower[Side],Hand[Side],Pole,Target.GetLocation(),false,1.,1.);Hand[Side].SetRotation(Target.GetRotation());
            Residual=FMath::Max(Residual,float(FVector::Dist(Hand[Side].GetLocation(),Target.GetLocation())));
            ArmError=FMath::Max(ArmError,FMath::Max(FMath::Abs(float(FVector::Dist(Upper[Side].GetLocation(),Lower[Side].GetLocation()))-UpperLength[Side]),
                FMath::Abs(float(FVector::Dist(Lower[Side].GetLocation(),Hand[Side].GetLocation()))-LowerLength[Side])));
            Valid&=!Upper[Side].ContainsNaN()&&!Lower[Side].ContainsNaN()&&!Hand[Side].ContainsNaN();
            Changes.Add(FBoneTransform(UpperIds[Side],Upper[Side]));Changes.Add(FBoneTransform(LowerIds[Side],Lower[Side]));Changes.Add(FBoneTransform(HandIds[Side],Hand[Side]));
        }
        if(!Valid||W.ContainsNaN()||Shift>12.f||Residual>.15f||ArmError>.05f){++Diagnostics.NationalCarryGripRejectedFramesV82;return false;}
        Changes.Add(FBoneTransform(Weapon,W));if(ValidCompactBone(Local,Bolt))Changes.Add(FBoneTransform(Bolt,BoltAnchor*W));
        Changes.Sort([](const FBoneTransform& A,const FBoneTransform& B){return A.BoneIndex.GetInt()<B.BoneIndex.GetInt();});Pose.LocalBlendCSBoneTransforms(MakeArrayView(Changes),1.f);
        for(int32 Side=0;Side<2;++Side)Residual=FMath::Max(Residual,float(FVector::Dist(Pose.GetComponentSpaceTransform(HandIds[Side]).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation(),Grips[Side].GetLocation())));
        if(!FMath::IsFinite(Residual)||Residual>.15f){++Diagnostics.NationalCarryGripRejectedFramesV82;return false;}
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Local);++Diagnostics.NationalCarryGripFramesV82;++GripConstraintFrames;
        Diagnostics.MaximumNationalCarryGunShiftCmV82=FMath::Max(Diagnostics.MaximumNationalCarryGunShiftCmV82,Shift);
        Diagnostics.MaximumNationalBothGripResidualCmV82=FMath::Max(Diagnostics.MaximumNationalBothGripResidualCmV82,Residual);
        Diagnostics.MaximumArmLengthErrorCm=FMath::Max(Diagnostics.MaximumArmLengthErrorCm,ArmError);MaximumGripResidualCm=FMath::Max(MaximumGripResidualCm,Residual);return true;
    }

    bool NationalGroundGunPoseV82(FCompactPose& Local)
    {
        if(!bNationalGroundTargetV82||!bNationalAutomatic||!CurrentAsset
            ||(NationalGroundTargetV82.ContactCount!=2&&NationalGroundTargetV82.ContactCount!=3))return false;
        const FBoneContainer& Bones=Local.GetBoneContainer();
        const auto Bone=[&](const TCHAR* Name){return FindCompactBone(Bones,Name);};
        const auto Weapon=Bone(TEXT("weapon_rifle")),Bolt=Bone(TEXT("weapon_bolt")),Spine=Bone(TEXT("spine_01"));
        const FCompactPoseBoneIndex UpperIds[]={Bone(TEXT("upperarm_l")),Bone(TEXT("upperarm_r"))};
        const FCompactPoseBoneIndex LowerIds[]={Bone(TEXT("lowerarm_l")),Bone(TEXT("lowerarm_r"))};
        const FCompactPoseBoneIndex HandIds[]={Bone(TEXT("hand_l")),Bone(TEXT("hand_r"))};
        const FCompactPoseBoneIndex FixedIds[]={Bone(TEXT("pelvis")),Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
        bool Valid=ValidCompactBone(Local,Weapon)&&ValidCompactBone(Local,Bolt)&&ValidCompactBone(Local,Spine)
            &&Bones.BoneIsChildOf(Weapon,Spine);
        for(int32 Side=0;Side<2;++Side)Valid&=ValidCompactBone(Local,UpperIds[Side])&&ValidCompactBone(Local,LowerIds[Side])
            &&ValidCompactBone(Local,HandIds[Side])&&Bones.GetParentBoneIndex(LowerIds[Side])==UpperIds[Side]
            &&Bones.GetParentBoneIndex(HandIds[Side])==LowerIds[Side]&&Bones.BoneIsChildOf(UpperIds[Side],Spine);
        for(const auto Id:FixedIds)Valid&=ValidCompactBone(Local,Id);
        if(!Valid){++Diagnostics.NationalGroundGunRejectedFramesV82;return false;}
        FCSPose<FCompactPose> Base;Base.InitPose(Local);const FTransform Original=Base.GetComponentSpaceTransform(Weapon);
        const FTransform W=NationalGroundTargetV82.WeaponComponent;
        const double Shift=FVector::Dist(W.GetTranslation(),Original.GetTranslation());
        const double Tilt=FMath::RadiansToDegrees(W.GetRotation().AngularDistance(Original.GetRotation()));
        if(W.ContainsNaN()||Shift>12.||Tilt>15.||!FMath::IsFinite(Shift)||!FMath::IsFinite(Tilt))
        {++Diagnostics.NationalGroundGunRejectedFramesV82;return false;}
        const FTransform World=W*NationalGroundTargetV82.BodyWorld;float FootError=0;
        for(int32 I=0;I<NationalGroundTargetV82.ContactCount;++I)
        {
            const FVector Foot=World.TransformPosition(NationalGroundTargetV82.LocalFeet[I]);
            const double Gap=Foot.Z-NationalGroundTargetV82.HitZ[I];
            Valid&=!Foot.ContainsNaN()&&FMath::Abs(NationalGroundTargetV82.HitZ[I]-NationalGroundTargetV82.NativeZ[I])<.25f
                &&Gap>=-1.5f&&Gap<=2.5f;FootError=FMath::Max(FootError,float(FMath::Abs(Gap)));
        }
        if(!Valid){++Diagnostics.NationalGroundGunRejectedFramesV82;return false;}
        const FTransform BoltAnchor=Base.GetComponentSpaceTransform(Bolt).GetRelativeTransform(Original);
        FTransform Anchors[2];float UpperLength[2],LowerLength[2];double Minimum[2],Maximum[2];
        const FVector Pivot=Base.GetComponentSpaceTransform(Spine).GetLocation();
        TArray<FVector,TInlineAllocator<3>> SearchAxes;
        FVector CombinedAxis=FVector::ZeroVector;
        for(int32 Side=0;Side<2;++Side)
        {
            const FVector Upper=Base.GetComponentSpaceTransform(UpperIds[Side]).GetLocation();
            const FVector Lower=Base.GetComponentSpaceTransform(LowerIds[Side]).GetLocation();
            const FTransform Hand=Base.GetComponentSpaceTransform(HandIds[Side]);
            Anchors[Side]=Hand.GetRelativeTransform(Original);
            UpperLength[Side]=FVector::Dist(Upper,Lower);LowerLength[Side]=FVector::Dist(Lower,Hand.GetLocation());
            Maximum[Side]=.965*(UpperLength[Side]+LowerLength[Side]);
            Minimum[Side]=FMath::Sqrt(FMath::Square(UpperLength[Side])+FMath::Square(LowerLength[Side])
                +2.*UpperLength[Side]*LowerLength[Side]*FMath::Cos(FMath::DegreesToRadians(153.)));
            const FVector Target=(Anchors[Side]*W).GetLocation();const double Reach=FVector::Dist(Upper,Target);
            // Move a shoulder about the rooted torso, never the grounded gun.
            // An over-folded arm needs its shoulder farther from the fixed grip.
            const FVector Desired=Reach<Minimum[Side]?Upper-Target:Reach>Maximum[Side]?Target-Upper:FVector::ZeroVector;
            const FVector Axis=FVector::CrossProduct(Upper-Pivot,Desired).GetSafeNormal();
            if(!Axis.IsNearlyZero()){SearchAxes.Add(Axis);CombinedAxis+=Axis;}
        }
        if(SearchAxes.Num()>1&&!CombinedAxis.IsNearlyZero())SearchAxes.Add(CombinedAxis.GetSafeNormal());
        if(GroundGunTorsoAssetV82!=CurrentAsset->GetFName())bGroundGunTorsoAcceptedV82=false;
        TArray<FQuat,TInlineAllocator<17>> Offsets;
        // Reuse a still-valid accepted posture first. It is rechecked against
        // this frame's gun/arms, so breathing does not flip between candidates.
        if(bGroundGunTorsoAcceptedV82)Offsets.Add(GroundGunTorsoOffsetV82);
        Offsets.Add(FQuat::Identity);
        for(const FVector& Axis:SearchAxes)for(const float Degrees:{3.f,6.f,9.f,12.f,15.f})
            Offsets.Add(FQuat(Axis,FMath::DegreesToRadians(Degrees)));
        const TCHAR* Failure=TEXT("no_reachable_torso_pose");int32 FailedSide=INDEX_NONE;
        double FailedReach=0,FailedMinimum=0,FailedMaximum=0;
        float GripError=0,ArmError=0;
        for(const FQuat& Offset:Offsets)
        {
            FCSPose<FCompactPose> Pose;Pose.InitPose(Local);
            if(!Offset.Equals(FQuat::Identity,.00001f))
            {
                FTransform Torso=Pose.GetComponentSpaceTransform(Spine);
                Torso.SetRotation((Offset*Torso.GetRotation()).GetNormalized());
                const FBoneTransform Change(Spine,Torso);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&Change,1),1.f);
            }
            bool CandidateValid=true;GripError=0;ArmError=0;
            TArray<FBoneTransform,TInlineAllocator<8>> Changes;
            for(int32 Side=0;Side<2;++Side)
            {
                FTransform Upper=Pose.GetComponentSpaceTransform(UpperIds[Side]);
                FTransform Lower=Pose.GetComponentSpaceTransform(LowerIds[Side]);
                FTransform Hand=Pose.GetComponentSpaceTransform(HandIds[Side]);
                const FTransform Target=Anchors[Side]*W;const double Required=FVector::Dist(Upper.GetLocation(),Target.GetLocation());
                if(!FMath::IsFinite(Required)||Required<Minimum[Side]||Required>Maximum[Side])
                {
                    Failure=Required<Minimum[Side]?TEXT("arm_overfolded"):TEXT("arm_unreachable");
                    FailedSide=Side;FailedReach=Required;FailedMinimum=Minimum[Side];FailedMaximum=Maximum[Side];CandidateValid=false;break;
                }
                const FVector Bend=Lower.GetLocation()-Upper.GetLocation();
                const FVector Pole=StableElbowTarget(Upper,Lower,Hand,Target.GetLocation(),Bend,Side);
                AnimationCore::SolveTwoBoneIK(Upper,Lower,Hand,Pole,Target.GetLocation(),false,1.,1.);Hand.SetRotation(Target.GetRotation());
                GripError=FMath::Max(GripError,float(FVector::Dist(Hand.GetLocation(),Target.GetLocation())));
                ArmError=FMath::Max(ArmError,FMath::Max(FMath::Abs(float(FVector::Dist(Upper.GetLocation(),Lower.GetLocation()))-UpperLength[Side]),
                    FMath::Abs(float(FVector::Dist(Lower.GetLocation(),Hand.GetLocation()))-LowerLength[Side])));
                CandidateValid&=!Upper.ContainsNaN()&&!Lower.ContainsNaN()&&!Hand.ContainsNaN();
                Changes.Add(FBoneTransform(UpperIds[Side],Upper));Changes.Add(FBoneTransform(LowerIds[Side],Lower));Changes.Add(FBoneTransform(HandIds[Side],Hand));
            }
            if(!CandidateValid)continue;
            if(GripError>.15f||ArmError>.05f){Failure=TEXT("fixed_arm_ik_residual");continue;}
            Changes.Add(FBoneTransform(Weapon,W));Changes.Add(FBoneTransform(Bolt,BoltAnchor*W));
            Changes.Sort([](const FBoneTransform& A,const FBoneTransform& B){return A.BoneIndex.GetInt()<B.BoneIndex.GetInt();});
            Pose.LocalBlendCSBoneTransforms(MakeArrayView(Changes),1.f);
            for(int32 Side=0;Side<2;++Side)
            {
                GripError=FMath::Max(GripError,float(FVector::Dist(Pose.GetComponentSpaceTransform(HandIds[Side]).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation(),Anchors[Side].GetLocation())));
                ArmError=FMath::Max(ArmError,FMath::Max(FMath::Abs(float(FVector::Dist(Pose.GetComponentSpaceTransform(UpperIds[Side]).GetLocation(),Pose.GetComponentSpaceTransform(LowerIds[Side]).GetLocation()))-UpperLength[Side]),
                    FMath::Abs(float(FVector::Dist(Pose.GetComponentSpaceTransform(LowerIds[Side]).GetLocation(),Pose.GetComponentSpaceTransform(HandIds[Side]).GetLocation()))-LowerLength[Side])));
            }
            if(!FMath::IsFinite(GripError)||!FMath::IsFinite(ArmError)||GripError>.15f||ArmError>.05f
                ||!Pose.GetComponentSpaceTransform(Weapon).Equals(W,.0001f)
                ||!Pose.GetComponentSpaceTransform(Bolt).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).Equals(BoltAnchor,.0001f))
            {Failure=TEXT("published_gun_or_grip_changed");continue;}
            for(const auto Id:FixedIds)CandidateValid&=Pose.GetComponentSpaceTransform(Id).Equals(Base.GetComponentSpaceTransform(Id),.0001f);
            if(!CandidateValid){Failure=TEXT("rooted_pelvis_or_boot_changed");continue;}
            // The whole pose publishes only after the original grounded-gun,
            // 153-degree reach, fixed lengths and both grip constraints pass.
            FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Local);
            if(!Offset.Equals(FQuat::Identity,.00001f)&&PersonalTime-GroundGunLastDiagnosticTimeV82>=3.f
                &&(!bGroundGunTorsoAcceptedV82||Offset.AngularDistance(GroundGunTorsoOffsetV82)>FMath::DegreesToRadians(.5f)))
            {
                GroundGunLastDiagnosticTimeV82=PersonalTime;
                UE_LOG(LogTemp,Display,TEXT("WF_NATIONAL_GROUND_GUN_POSTURE clip=%s torso_degrees=%.4f right_reach=%.4f right_minimum=%.4f shift=%.4f tilt=%.4f foot_error=%.4f grip=%.4f arm=%.4f"),
                    *CurrentAsset->GetName(),FMath::RadiansToDegrees(Offset.AngularDistance(FQuat::Identity)),
                    FVector::Dist(Pose.GetComponentSpaceTransform(UpperIds[1]).GetLocation(),(Anchors[1]*W).GetLocation()),Minimum[1],Shift,Tilt,FootError,GripError,ArmError);
            }
            GroundGunTorsoOffsetV82=Offset;GroundGunTorsoAssetV82=CurrentAsset->GetFName();bGroundGunTorsoAcceptedV82=true;
            ++Diagnostics.NationalGroundGunFramesV82;
            Diagnostics.MaximumNationalGroundGunShiftCmV82=FMath::Max(Diagnostics.MaximumNationalGroundGunShiftCmV82,float(Shift));
            Diagnostics.MaximumNationalGroundGunTiltDegreesV82=FMath::Max(Diagnostics.MaximumNationalGroundGunTiltDegreesV82,float(Tilt));
            Diagnostics.MaximumNationalGroundFootErrorCmV82=FMath::Max(Diagnostics.MaximumNationalGroundFootErrorCmV82,FootError);
            Diagnostics.MaximumNationalGroundGripErrorCmV82=FMath::Max(Diagnostics.MaximumNationalGroundGripErrorCmV82,GripError);
            Diagnostics.MaximumArmLengthErrorCm=FMath::Max(Diagnostics.MaximumArmLengthErrorCm,ArmError);return true;
        }
        ++Diagnostics.NationalGroundGunRejectedFramesV82;bGroundGunTorsoAcceptedV82=false;
        if(PersonalTime-GroundGunLastDiagnosticTimeV82>=3.f)
        {
            GroundGunLastDiagnosticTimeV82=PersonalTime;
            UE_LOG(LogTemp,Warning,TEXT("WF_NATIONAL_GROUND_GUN_REJECT clip=%s reason=%s side=%d required=%.4f minimum=%.4f maximum=%.4f torso_candidates=%d shift=%.4f tilt=%.4f grip=%.4f arm=%.4f"),
                *CurrentAsset->GetName(),Failure,FailedSide,FailedReach,FailedMinimum,FailedMaximum,Offsets.Num(),Shift,Tilt,GripError,ArmError);
        }
        return false;
    }

    void MountedTrenchReloadV8447(FCompactPose& Local)
    {
        // A supported feed action keeps its last truly supported Ready/Aim
        // gun target. Bolt and hand gestures retain their authored transforms
        // relative to that gun. Run after the ordinary handoff blend so
        // local parent interpolation cannot move the compensated gun again.
        // These coordinates become real world anchors through the unchanged
        // skeletal component transform; the actor and its feet never translate.
        const FBoneContainer& Bones=Local.GetBoneContainer();
        const auto Bone=[&Bones](const TCHAR* Name){return FindCompactBone(Bones,Name);};
        const auto Hip=Bone(TEXT("pelvis")),Spine=Bone(TEXT("spine_02")),Weapon=Bone(TEXT("weapon_rifle")),Bolt=Bone(TEXT("weapon_bolt"));
        const FCompactPoseBoneIndex Hands[]={Bone(TEXT("hand_l")),Bone(TEXT("hand_r"))};
        const FCompactPoseBoneIndex UpperArms[]={Bone(TEXT("upperarm_l")),Bone(TEXT("upperarm_r"))};
        const FCompactPoseBoneIndex LowerArms[]={Bone(TEXT("lowerarm_l")),Bone(TEXT("lowerarm_r"))};
        const FCompactPoseBoneIndex Thighs[]={Bone(TEXT("thigh_l")),Bone(TEXT("thigh_r"))};
        const FCompactPoseBoneIndex Calves[]={Bone(TEXT("calf_l")),Bone(TEXT("calf_r"))};
        const FCompactPoseBoneIndex Feet[]={Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
        bool Safe=ValidCompactBone(Local,Hip)&&ValidCompactBone(Local,Spine)&&ValidCompactBone(Local,Weapon)&&ValidCompactBone(Local,Bolt);
        for(int32 Side=0;Side<2;++Side)Safe&=ValidCompactBone(Local,Hands[Side])&&ValidCompactBone(Local,UpperArms[Side])
            &&ValidCompactBone(Local,LowerArms[Side])&&ValidCompactBone(Local,Thighs[Side])&&ValidCompactBone(Local,Calves[Side])&&ValidCompactBone(Local,Feet[Side]);
        if(Safe)Safe=Bones.BoneIsChildOf(Weapon,Spine)&&Bones.BoneIsChildOf(Hands[0],Spine)&&Bones.BoneIsChildOf(Hands[1],Spine);
        if(!Safe||!bMountedWeaponTargetV8447){++Diagnostics.TrenchReloadRejectedFramesV8447;return;}
        FCompactPose Original;Original.CopyBonesFrom(Local);
        FCSPose<FCompactPose> Base;Base.InitPose(Original);
        const FTransform AuthoredWeapon=Base.GetComponentSpaceTransform(Weapon),WeaponTarget=MountedWeaponTargetCSV8447;
        const FTransform BoltTarget=Base.GetComponentSpaceTransform(Bolt).GetRelativeTransform(AuthoredWeapon)*WeaponTarget;
        FTransform HandTargets[2],FootTargets[2];FVector KneeHints[2],ElbowHints[2];
        float ArmUpper[2],ArmLower[2],LegUpper[2],LegLower[2];
        for(int32 Side=0;Side<2;++Side)
        {
            HandTargets[Side]=Base.GetComponentSpaceTransform(Hands[Side]).GetRelativeTransform(AuthoredWeapon)*WeaponTarget;
            FootTargets[Side]=Base.GetComponentSpaceTransform(Feet[Side]);
            KneeHints[Side]=Base.GetComponentSpaceTransform(Calves[Side]).GetLocation();ElbowHints[Side]=Base.GetComponentSpaceTransform(LowerArms[Side]).GetLocation();
            ArmUpper[Side]=FVector::Dist(Base.GetComponentSpaceTransform(UpperArms[Side]).GetLocation(),ElbowHints[Side]);
            ArmLower[Side]=FVector::Dist(ElbowHints[Side],Base.GetComponentSpaceTransform(Hands[Side]).GetLocation());
            LegUpper[Side]=FVector::Dist(Base.GetComponentSpaceTransform(Thighs[Side]).GetLocation(),KneeHints[Side]);
            LegLower[Side]=FVector::Dist(KneeHints[Side],FootTargets[Side].GetLocation());
        }
        float Accepted=0.f;bool Anchored=false;
        // Five bounded geometric fits. Reject unreachable arm/leg targets and
        // reduce the duck; never stretch a limb or release the supported gun.
        for(float Fraction:{1.f,.75f,.5f,.25f,0.f})
        {
            Local.CopyBonesFrom(Original);FCSPose<FCompactPose> Pose;Pose.InitPose(Local);
            const float Blend=TrenchReloadShelterV8447*Fraction,Lower=TrenchReloadLowerV8447*Blend;
            FTransform Pelvis=Pose.GetComponentSpaceTransform(Hip);Pelvis.AddToTranslation(FVector(0,-3.f*Blend,-Lower));
            const FBoneTransform HipChange(Hip,Pelvis);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&HipChange,1),1.f);
            FTransform Torso=Pose.GetComponentSpaceTransform(Spine);Torso.SetRotation((FRotator(0,0,-7.f*Blend).Quaternion()*Torso.GetRotation()).GetNormalized());
            const FBoneTransform TorsoChange(Spine,Torso);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&TorsoChange,1),1.f);
            bool Reachable=true;
            for(int32 Side=0;Side<2;++Side)
            {
                const float ArmReach=FVector::Dist(Pose.GetComponentSpaceTransform(UpperArms[Side]).GetLocation(),HandTargets[Side].GetLocation());
                const float LegReach=FVector::Dist(Pose.GetComponentSpaceTransform(Thighs[Side]).GetLocation(),FootTargets[Side].GetLocation());
                Reachable&=ArmReach<ArmUpper[Side]+ArmLower[Side]-.005f&&ArmReach>FMath::Abs(ArmUpper[Side]-ArmLower[Side])+.005f
                    &&LegReach<LegUpper[Side]+LegLower[Side]-.005f&&LegReach>FMath::Abs(LegUpper[Side]-LegLower[Side])+.005f;
            }
            if(!Reachable)continue;
            float MaxFoot=0.f,MaxGrip=0.f,MaxArm=0.f,MaxLeg=0.f;
            for(int32 Side=0;Side<2;++Side)
            {
                FTransform A=Pose.GetComponentSpaceTransform(UpperArms[Side]),C=Pose.GetComponentSpaceTransform(LowerArms[Side]),H=Pose.GetComponentSpaceTransform(Hands[Side]);
                const FVector ArmAxis=(HandTargets[Side].GetLocation()-A.GetLocation()).GetSafeNormal();
                FVector ArmBend=ProjectBend(ElbowHints[Side]-A.GetLocation(),ArmAxis);
                if(ArmBend.SizeSquared()<1.f)ArmBend=ProjectBend(FVector(Side?1.f:-1.f,0,0),ArmAxis);
                const FVector ArmPole=A.GetLocation()+ArmAxis*ArmUpper[Side]*.45f+ArmBend.GetSafeNormal()*(ArmUpper[Side]+ArmLower[Side]);
                AnimationCore::SolveTwoBoneIK(A,C,H,ArmPole,HandTargets[Side].GetLocation(),false,1.,1.);H.SetRotation(HandTargets[Side].GetRotation());
                const FBoneTransform Arm[]={FBoneTransform(UpperArms[Side],A),FBoneTransform(LowerArms[Side],C),FBoneTransform(Hands[Side],H)};
                Pose.LocalBlendCSBoneTransforms(MakeArrayView(Arm),1.f);
                MaxArm=FMath::Max(MaxArm,FMath::Max(FMath::Abs(float(FVector::Dist(A.GetLocation(),C.GetLocation()))-ArmUpper[Side]),FMath::Abs(float(FVector::Dist(C.GetLocation(),H.GetLocation()))-ArmLower[Side])));
                MaxGrip=FMath::Max(MaxGrip,float(FVector::Dist(H.GetLocation(),HandTargets[Side].GetLocation())));
                FTransform T=Pose.GetComponentSpaceTransform(Thighs[Side]),K=Pose.GetComponentSpaceTransform(Calves[Side]),F=Pose.GetComponentSpaceTransform(Feet[Side]);
                const FVector Axis=(FootTargets[Side].GetLocation()-T.GetLocation()).GetSafeNormal();FVector Bend=ProjectBend(KneeHints[Side]-T.GetLocation(),Axis);
                if(Bend.SizeSquared()<1.f)Bend=ProjectBend(FVector(0,1,0),Axis);
                const FVector Pole=T.GetLocation()+Axis*LegUpper[Side]*.45f+Bend.GetSafeNormal()*(LegUpper[Side]+LegLower[Side]);
                AnimationCore::SolveTwoBoneIK(T,K,F,Pole,FootTargets[Side].GetLocation(),false,1.,1.);F.SetRotation(FootTargets[Side].GetRotation());
                const FBoneTransform Leg[]={FBoneTransform(Thighs[Side],T),FBoneTransform(Calves[Side],K),FBoneTransform(Feet[Side],F)};
                Pose.LocalBlendCSBoneTransforms(MakeArrayView(Leg),1.f);
                MaxFoot=FMath::Max(MaxFoot,float(FVector::Dist(F.GetLocation(),FootTargets[Side].GetLocation())));
                MaxLeg=FMath::Max(MaxLeg,FMath::Max(FMath::Abs(float(FVector::Dist(T.GetLocation(),K.GetLocation()))-LegUpper[Side]),FMath::Abs(float(FVector::Dist(K.GetLocation(),F.GetLocation()))-LegLower[Side])));
            }
            const FBoneTransform GunChange(Weapon,WeaponTarget);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&GunChange,1),1.f);
            const FBoneTransform BoltChange(Bolt,BoltTarget);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&BoltChange,1),1.f);
            const float GunError=FVector::Dist(Pose.GetComponentSpaceTransform(Weapon).GetLocation(),WeaponTarget.GetLocation());
            const float BoltError=FVector::Dist(Pose.GetComponentSpaceTransform(Bolt).GetLocation(),BoltTarget.GetLocation());
            if(MaxFoot>.02f||MaxGrip>.02f||MaxArm>.02f||MaxLeg>.02f||GunError>.02f||BoltError>.02f)continue;
            Diagnostics.MaximumTrenchReloadFootDriftCmV8447=FMath::Max(Diagnostics.MaximumTrenchReloadFootDriftCmV8447,MaxFoot);
            Diagnostics.MaximumTrenchReloadGripDriftCmV8447=FMath::Max(Diagnostics.MaximumTrenchReloadGripDriftCmV8447,MaxGrip);
            Diagnostics.MaximumTrenchReloadLegLengthErrorCmV8447=FMath::Max(Diagnostics.MaximumTrenchReloadLegLengthErrorCmV8447,MaxLeg);
            Diagnostics.MaximumMountedReloadArmLengthErrorCmV8447=FMath::Max(Diagnostics.MaximumMountedReloadArmLengthErrorCmV8447,MaxArm);
            Diagnostics.MaximumMountedReloadWeaponAnchorDriftCmV8447=FMath::Max(Diagnostics.MaximumMountedReloadWeaponAnchorDriftCmV8447,GunError);
            Diagnostics.MaximumMountedReloadBoltAnchorDriftCmV8447=FMath::Max(Diagnostics.MaximumMountedReloadBoltAnchorDriftCmV8447,BoltError);
            FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Local);Accepted=Fraction;Anchored=true;break;
        }
        if(!Anchored){Local.CopyBonesFrom(Original);++Diagnostics.TrenchReloadRejectedFramesV8447;return;}
        Diagnostics.LastTrenchReloadLowerCmV8447=TrenchReloadLowerV8447*TrenchReloadShelterV8447*Accepted;
        Diagnostics.LastMountedReloadWeaponAnchorCSV8447=WeaponTarget.GetLocation();
        ++Diagnostics.TrenchReloadFramesV8447;++Diagnostics.MountedReloadFramesV8447;
        if(Accepted<1.f)++Diagnostics.MountedReloadReducedFramesV8447;
    }

    virtual bool Evaluate(FPoseContext& Output) override
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_AnimationEvaluate);
        ++Diagnostics.EvaluatedFrames;
        const bool Result=FAnimSingleNodeInstanceProxy::Evaluate(Output);
        // Keep nonphysical hand/boot locals from the actual handoff pose while
        // Chaos drives the articulated bones. Key by mesh bone, because a
        // physics asset can change the compact-pose RequiredBones ordering.
        if (RagdollPoseFrozen)
        {
            const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
            for (FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
                if (const FTransform* Frozen=RagdollLocalPose.Find(Bones.MakeMeshPoseIndex(I).GetInt())) Output.Pose[I]=*Frozen;
            return Result;
        }
        // InitAnim can evaluate the skeleton's reference pose before Initialize assigns a clip.
        // A freshly spawned unit can then receive an aim order in its first actor tick. That
        // bind pose was never an authored carry pose and must not become a transition source.
        if (!CurrentAsset)
        {
            DisplayedPose.Reset(); DisplayedAssetName=NAME_None;
            return Result;
        }
        const int32 Count=Output.Pose.GetNumBones();
        float LocomotionWeightV46=(CurrentAsset->GetFName()==WalkAssetNameV46||CurrentAsset->GetFName()==RunAssetNameV46)?1.f:0.f;
        if (BlendTime<BlendDuration || DeathEntryTime<.30f) RemapTransitionSource(Output.Pose);
        OverheadRifleV45(Output.Pose);
        // Apply personality to the incoming authored pose before blending complete
        // endpoints. The snapshot already contains the previous personal layer;
        // layering after blending would apply it twice and accumulate on interrupts.
        if (bPersonalReadyPose || bPersonalLocomotion)
        {
            // Rotate one common torso ancestor: both complete arms, the rifle and
            // their approved finger poses move together, while feet remain planted.
            // This is restricted to ready/aim/locomotion clips; no bolt, feed or grenade action
            // is time-warped and none of its contact poses or notifies are replaced.
            const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
            const auto Bone=[&Bones](const TCHAR* Name) { return FindCompactBone(Bones,Name); };
            const FCompactPoseBoneIndex Spine=Bone(TEXT("spine_02")),Head=Bone(TEXT("Head"));
            const FCompactPoseBoneIndex LeftUpper=Bone(TEXT("upperarm_l")),RightUpper=Bone(TEXT("upperarm_r"));
            const FCompactPoseBoneIndex Weapon=Bone(TEXT("weapon_rifle")),GripHand=Bone(bPersonalPistol?TEXT("hand_r"):TEXT("hand_l"));
            const bool bHasTorsoBones=ValidCompactBone(Output.Pose,Spine) && ValidCompactBone(Output.Pose,LeftUpper)
                && ValidCompactBone(Output.Pose,RightUpper) && ValidCompactBone(Output.Pose,Weapon) && ValidCompactBone(Output.Pose,GripHand);
            const bool bSafeHierarchy=bHasTorsoBones && Bones.BoneIsChildOf(LeftUpper,Spine)
                && Bones.BoneIsChildOf(RightUpper,Spine) && Bones.BoneIsChildOf(Weapon,Spine);
            if (!bSafeHierarchy) ++Diagnostics.UnsafeTorsoHierarchyFrames;
            if (bSafeHierarchy)
            {
                const float Phase=PersonalPhase*2.f*PI;
                const float RoleTempo=PersonalRole==1?.82f:PersonalRole==2?1.17f:PersonalRole==3?.88f:PersonalRole==4?.91f:1.f;
                const float Breath=FMath::Sin(PersonalTime*(1.12f+PersonalPhase*.34f)*RoleTempo+Phase);
                const float SlowShift=FMath::Sin(PersonalTime*(.34f+PersonalPhase*.18f)+Phase*1.73f);
                const float Amplitude=bPersonalLocomotion?.18f:bPersonalAiming?(bPersonalAutomatic?.23f:PersonalRole==3?.31f:.55f):PersonalRole==4?.76f:1.25f;
                // Every complete arm and the weapon follow this common ancestor.
                // Readable body intent changes without touching a wrist, a finger,
                // an accepted hip/foot target, or the low MG aim/reload clips.
                const float StancePitch[]={-.25f,1.1f,2.7f,1.15f,-1.8f,1.45f};
                const float RoleLean=StancePitch[FMath::Clamp(PersonalRole,0,5)]
                    *(bPersonalAiming?.30f:bPersonalLocomotion?.72f:1.f);
                FCSPose<FCompactPose> Pose; Pose.InitPose(Output.Pose);
                const FVector GripBefore=Pose.GetComponentSpaceTransform(GripHand).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation();
                FTransform Torso=Pose.GetComponentSpaceTransform(Spine);
                // The imported skeleton faces +Y (the component is yawed -90
                // degrees). Roll therefore supplies the sagittal role lean;
                // retain both pre-existing breathing components unchanged.
                const FQuat Turn=(FRotator(Amplitude*(.55f*Breath+.35f*(PersonalPhase-.5f)),0,RoleLean+Amplitude*.45f*SlowShift)+IdlePoseV38.Torso).Quaternion();
                Torso.SetRotation((Turn*Torso.GetRotation()).GetNormalized());
                const FBoneTransform TorsoChange(Spine,Torso); Pose.LocalBlendCSBoneTransforms(MakeArrayView(&TorsoChange,1),1.f);
                if (((!bPersonalAiming && !bPersonalLocomotion) || IdlePoseV38.IsActive()) && ValidCompactBone(Output.Pose,Head))
                {
                    FTransform Look=Pose.GetComponentSpaceTransform(Head);
                    const float Scan=PersonalRole==3?6.2f:PersonalRole==4?4.6f:PersonalRole==1?1.35f:3.2f;
                    const float ObservePitch=PersonalRole==3?1.4f:PersonalRole==4?-.65f:0.f;
                    Look.SetRotation(((FRotator(0,bPersonalAiming?0.f:SlowShift*Scan,bPersonalAiming?0.f:ObservePitch)+IdlePoseV38.Head).Quaternion()*Look.GetRotation()).GetNormalized());
                    const FBoneTransform HeadChange(Head,Look); Pose.LocalBlendCSBoneTransforms(MakeArrayView(&HeadChange,1),1.f);
                }
                const FVector GripAfter=Pose.GetComponentSpaceTransform(GripHand).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation();
                Diagnostics.MaximumPersonalGripDriftCm=FMath::Max(Diagnostics.MaximumPersonalGripDriftCm,float(FVector::Dist(GripBefore,GripAfter)));
                Diagnostics.LastPersonalLeanDegrees=RoleLean+Amplitude*.45f*SlowShift;
                ++Diagnostics.PersonalLayerFrames;
                FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Output.Pose);
            }
        }
        Diagnostics.LastTrenchReloadShelterV8447=TrenchReloadShelterV8447;
        Diagnostics.LastTrenchReloadLowerCmV8447=0;
        if(TrenchReloadShelterV8447>0.f&&!bMountedTrenchReloadV8447)
        {
            // Keep the authored magazine/feed motion intact: both complete
            // arms and the weapon follow their common torso ancestor. Lower
            // the hips, bend the torso slightly and solve each leg back to
            // its original foot target without stretching or moving the actor.
            const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
            const auto Bone=[&Bones](const TCHAR* Name){return FindCompactBone(Bones,Name);};
            const auto Hip=Bone(TEXT("pelvis")),Spine=Bone(TEXT("spine_02")),Weapon=Bone(TEXT("weapon_rifle"));
            const FCompactPoseBoneIndex Hands[]={Bone(TEXT("hand_l")),Bone(TEXT("hand_r"))};
            const FCompactPoseBoneIndex Thighs[]={Bone(TEXT("thigh_l")),Bone(TEXT("thigh_r"))};
            const FCompactPoseBoneIndex Calves[]={Bone(TEXT("calf_l")),Bone(TEXT("calf_r"))};
            const FCompactPoseBoneIndex Feet[]={Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
            bool Safe=ValidCompactBone(Output.Pose,Hip)&&ValidCompactBone(Output.Pose,Spine)&&ValidCompactBone(Output.Pose,Weapon);
            for(int32 Side=0;Side<2;++Side)
                Safe&=ValidCompactBone(Output.Pose,Hands[Side])&&ValidCompactBone(Output.Pose,Thighs[Side])
                    &&ValidCompactBone(Output.Pose,Calves[Side])&&ValidCompactBone(Output.Pose,Feet[Side]);
            if(Safe)Safe=Bones.BoneIsChildOf(Weapon,Spine)&&Bones.BoneIsChildOf(Hands[0],Spine)&&Bones.BoneIsChildOf(Hands[1],Spine);
            if(!Safe)++Diagnostics.TrenchReloadRejectedFramesV8447;
            else
            {
                FCSPose<FCompactPose> Pose;Pose.InitPose(Output.Pose);
                FTransform FootTargets[]={Pose.GetComponentSpaceTransform(Feet[0]),Pose.GetComponentSpaceTransform(Feet[1])};
                FVector GripBefore[2],KneeBefore[2];float UpperLength[2],LowerLength[2];
                for(int32 Side=0;Side<2;++Side)
                {
                    GripBefore[Side]=Pose.GetComponentSpaceTransform(Hands[Side]).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation();
                    KneeBefore[Side]=Pose.GetComponentSpaceTransform(Calves[Side]).GetLocation();
                    UpperLength[Side]=FVector::Distance(Pose.GetComponentSpaceTransform(Thighs[Side]).GetLocation(),KneeBefore[Side]);
                    LowerLength[Side]=FVector::Distance(KneeBefore[Side],FootTargets[Side].GetLocation());
                }
                const float Lower=TrenchReloadLowerV8447*TrenchReloadShelterV8447;
                FTransform Pelvis=Pose.GetComponentSpaceTransform(Hip);Pelvis.AddToTranslation(FVector(0,-3.f*TrenchReloadShelterV8447,-Lower));
                const FBoneTransform HipChange(Hip,Pelvis);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&HipChange,1),1.f);
                FTransform Torso=Pose.GetComponentSpaceTransform(Spine);
                Torso.SetRotation((FRotator(0,0,-7.f*TrenchReloadShelterV8447).Quaternion()*Torso.GetRotation()).GetNormalized());
                const FBoneTransform TorsoChange(Spine,Torso);Pose.LocalBlendCSBoneTransforms(MakeArrayView(&TorsoChange,1),1.f);
                for(int32 Side=0;Side<2;++Side)
                {
                    FTransform A=Pose.GetComponentSpaceTransform(Thighs[Side]),C=Pose.GetComponentSpaceTransform(Calves[Side]),F=Pose.GetComponentSpaceTransform(Feet[Side]);
                    // Bias the saved knee toward the imported +Y front only if
                    // the authored leg is almost straight, keeping its bend side.
                    const FVector Axis=(FootTargets[Side].GetLocation()-A.GetLocation()).GetSafeNormal();
                    FVector Bend=ProjectBend(KneeBefore[Side]-A.GetLocation(),Axis);
                    if(Bend.SizeSquared()<1.f)Bend=ProjectBend(FVector(0,1,0),Axis);
                    const FVector Pole=A.GetLocation()+Axis*UpperLength[Side]*.45f+Bend.GetSafeNormal()*(UpperLength[Side]+LowerLength[Side]);
                    AnimationCore::SolveTwoBoneIK(A,C,F,Pole,FootTargets[Side].GetLocation(),false,1.,1.);
                    F.SetRotation(FootTargets[Side].GetRotation());
                    const FBoneTransform Leg[]={FBoneTransform(Thighs[Side],A),FBoneTransform(Calves[Side],C),FBoneTransform(Feet[Side],F)};
                    Pose.LocalBlendCSBoneTransforms(MakeArrayView(Leg),1.f);
                    const float FootError=FVector::Distance(F.GetLocation(),FootTargets[Side].GetLocation());
                    const float LegError=FMath::Max(FMath::Abs(float(FVector::Distance(A.GetLocation(),C.GetLocation()))-UpperLength[Side]),FMath::Abs(float(FVector::Distance(C.GetLocation(),F.GetLocation()))-LowerLength[Side]));
                    const FVector GripAfter=Pose.GetComponentSpaceTransform(Hands[Side]).GetRelativeTransform(Pose.GetComponentSpaceTransform(Weapon)).GetLocation();
                    Diagnostics.MaximumTrenchReloadFootDriftCmV8447=FMath::Max(Diagnostics.MaximumTrenchReloadFootDriftCmV8447,FootError);
                    Diagnostics.MaximumTrenchReloadLegLengthErrorCmV8447=FMath::Max(Diagnostics.MaximumTrenchReloadLegLengthErrorCmV8447,LegError);
                    Diagnostics.MaximumTrenchReloadGripDriftCmV8447=FMath::Max(Diagnostics.MaximumTrenchReloadGripDriftCmV8447,float(FVector::Dist(GripBefore[Side],GripAfter)));
                }
                Diagnostics.LastTrenchReloadLowerCmV8447=Lower;++Diagnostics.TrenchReloadFramesV8447;
                FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Output.Pose);
            }
        }
        if(bTerrainFeet && (FMath::Abs(TerrainFootDelta[0])>.05f || FMath::Abs(TerrainFootDelta[1])>.05f))
        {
            // Apply the complete incoming leg pose before transition blending;
            // the outgoing snapshot already contains its own ground correction.
            // This preserves gait toe lift and all accepted upper-body contacts.
            const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
            const auto Bone=[&Bones](const TCHAR* Name){return FindCompactBone(Bones,Name);};
            const FCompactPoseBoneIndex Thighs[]={Bone(TEXT("thigh_l")),Bone(TEXT("thigh_r"))};
            const FCompactPoseBoneIndex Calves[]={Bone(TEXT("calf_l")),Bone(TEXT("calf_r"))};
            const FCompactPoseBoneIndex Feet[]={Bone(TEXT("foot_l")),Bone(TEXT("foot_r"))};
            FCSPose<FCompactPose> Pose;Pose.InitPose(Output.Pose);
            for(int32 Side=0;Side<2;++Side)
            {
                if(!ValidCompactBone(Output.Pose,Thighs[Side]) || !ValidCompactBone(Output.Pose,Calves[Side]) || !ValidCompactBone(Output.Pose,Feet[Side]))continue;
                FTransform Upper=Pose.GetComponentSpaceTransform(Thighs[Side]);
                FTransform Lower=Pose.GetComponentSpaceTransform(Calves[Side]);
                FTransform Foot=Pose.GetComponentSpaceTransform(Feet[Side]);
                const FQuat Ankle=Foot.GetRotation();
                const float UpperLength=FVector::Distance(Upper.GetLocation(),Lower.GetLocation());
                const float LowerLength=FVector::Distance(Lower.GetLocation(),Foot.GetLocation());
                const FVector Target=Foot.GetLocation()+TerrainUp*TerrainFootDelta[Side];
                const FVector Axis=(Target-Upper.GetLocation()).GetSafeNormal();
                FVector Bend=ProjectBend(Lower.GetLocation()-Upper.GetLocation(),Axis);
                if(Bend.SizeSquared()<1.f)Bend=ProjectBend(FVector(0,1,0),Axis);
                const FVector Pole=Upper.GetLocation()+Axis*(UpperLength*.45f)+Bend.GetSafeNormal()*(UpperLength+LowerLength);
                AnimationCore::SolveTwoBoneIK(Upper,Lower,Foot,Pole,Target,false,1.,1.);
                Foot.SetRotation(Ankle);
                const FBoneTransform Legs[]={FBoneTransform(Thighs[Side],Upper),FBoneTransform(Calves[Side],Lower),FBoneTransform(Feet[Side],Foot)};
                Pose.LocalBlendCSBoneTransforms(MakeArrayView(Legs),1.f);
                const float Error=FMath::Max(FMath::Abs(float(FVector::Distance(Upper.GetLocation(),Lower.GetLocation()))-UpperLength),FMath::Abs(float(FVector::Distance(Lower.GetLocation(),Foot.GetLocation()))-LowerLength));
                Diagnostics.MaximumTerrainLegLengthErrorCm=FMath::Max(Diagnostics.MaximumTerrainLegLengthErrorCm,Error);
            }
            ++Diagnostics.TerrainFootFrames;
            FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Output.Pose);
        }
        if(HeavyRecoilAgeV8215<.30f)
        {
            // One complete torso ancestor carries both hands and the heavy
            // rifle together. No weapon-only translation breaks the grip.
            const auto Spine=FindCompactBone(Output.Pose.GetBoneContainer(),TEXT("spine_02"));
            if(ValidCompactBone(Output.Pose,Spine))
            {
                const float Kick=(1-FMath::Exp(-HeavyRecoilAgeV8215*145.f))*FMath::Exp(-HeavyRecoilAgeV8215*16.f);
                auto& T=Output.Pose[Spine];
                T.SetRotation((T.GetRotation()*FQuat(FVector::RightVector,FMath::DegreesToRadians(-8.4f*Kick))).GetNormalized());
                ++Diagnostics.HeavyRecoilFramesV8215;
            }
        }
        if((bGasFatalV8215&&GasClipLengthV8215>.1f)||(GasDistressV8215>0&&(bPersonalReadyPose||bGasEscapeV8216)))
        {
            const float Phase=PersonalPhase*2*PI;
            // The lethal clip runs once. Chest contraction and lost head
            // control fade out before its exact terminal terrain/physics pose.
            // Living coughs keep their footing and never enter a crouch loop.
            const float End=FMath::Max(.4f,GasClipLengthV8215-.30f);
            const float FatalEnvelope=bGasFatalV8215?FMath::SmoothStep(.08f,.23f,GasTimeV8215)*(1-FMath::SmoothStep(End-.36f,End,GasTimeV8215)):0.f;
            const float T=bGasFatalV8215?GasTimeV8215:PersonalTime;
            const float Cough=FMath::Pow(FMath::Max(0.f,FMath::Sin(T*(3.7f+PersonalPhase*.7f)+Phase)),bGasFatalV8215?3.f:4.f);
            const float Spasm=FMath::Sin(T*(13.4f+PersonalPhase*2.8f)+Phase);
            const float W=bGasFatalV8215?FatalEnvelope:GasDistressV8215*Cough*(bGasEscapeV8216?.82f:.30f);
            const auto Add=[&](const TCHAR* Name,FVector Axis,float Degrees)
            {const auto I=FindCompactBone(Output.Pose.GetBoneContainer(),Name);if(ValidCompactBone(Output.Pose,I)){auto& Bone=Output.Pose[I];Bone.SetRotation((Bone.GetRotation()*FQuat(Axis,FMath::DegreesToRadians(Degrees*W))).GetNormalized());}};
            Add(TEXT("spine_02"),FVector::RightVector,(bGasFatalV8215?11.0f:bGasEscapeV8216?8.2f:4.8f)*Cough);
            Add(TEXT("spine_03"),FVector::ForwardVector,(bGasFatalV8215?6.5f:3.2f)*Spasm);
            Add(TEXT("Head"),FVector::RightVector,(bGasFatalV8215?-14.5f:-9.5f)*Cough);
            Add(TEXT("Head"),FVector::UpVector,(bGasFatalV8215?11.5f:6.8f)*Spasm);
            if(bGasFatalV8215)
            {
                Add(TEXT("clavicle_l"),FVector::ForwardVector,8.5f*Cough);
                Add(TEXT("clavicle_r"),FVector::ForwardVector,-7.1f*Spasm);
                Add(TEXT("upperarm_l"),FVector::UpVector,10.5f*Spasm);
                Add(TEXT("upperarm_r"),FVector::UpVector,-9.6f*Cough);
            }
            ++Diagnostics.GasDistressFramesV8215;
        }
        if(bBurnDistressV65&&BurningTimeV65<2.05f&&CurrentAsset->GetName().StartsWith(TEXT("A_BurningFall_")))
        {
            // Finite, asymmetric protective spasms layer on the authored loss
            // of footing. Never move the planted pelvis/legs or the final fall;
            // the exact end pose remains available to the terrain/ragdoll fit.
            const float In=FMath::SmoothStep(.08f,.27f,BurningTimeV65);
            const float Out=1-FMath::SmoothStep(1.40f,2.05f,BurningTimeV65);
            const float W=In*Out,Phase=PersonalPhase*2*PI;
            const float Spasm=FMath::Sin(BurningTimeV65*(12.2f+PersonalPhase*2.3f)+Phase);
            const float Counter=FMath::Sin(BurningTimeV65*8.7f+Phase+1.9f);
            const auto Add=[&](const TCHAR* Name,FVector Axis,float Degrees)
            {const auto I=FindCompactBone(Output.Pose.GetBoneContainer(),Name);if(ValidCompactBone(Output.Pose,I)){auto& T=Output.Pose[I];T.SetRotation((T.GetRotation()*FQuat(Axis,FMath::DegreesToRadians(Degrees*W))).GetNormalized());}};
            Add(TEXT("spine_02"),FVector::ForwardVector,3.8f*Spasm);Add(TEXT("spine_03"),FVector::UpVector,3.1f*Counter);
            Add(TEXT("Head"),FVector::UpVector,5.f*Counter);Add(TEXT("Head"),FVector::ForwardVector,3.5f*Spasm);
            Add(TEXT("clavicle_l"),FVector::ForwardVector,4.f*Counter);Add(TEXT("clavicle_r"),FVector::ForwardVector,-3.5f*Spasm);
            Add(TEXT("upperarm_l"),FVector::UpVector,5.5f*Counter);Add(TEXT("upperarm_r"),FVector::UpVector,-5.5f*Spasm);
            ++Diagnostics.BurningDistressFramesV65;
        }
        // A machinegunner's low firing stance must not rise to a standing death
        // entry. Preserve the current hip height and authored foot contacts while
        // this small correction fades into the falling clip. It is gone long
        // before the complete pose is handed to physics; mesh/root scale stays 1.
        if (DeathEntryTime<.30f && CurrentAsset->GetName().StartsWith(TEXT("A_Death")) && TransitionSource.Num()==Count)
        {
            const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
            const auto Index=[&Bones](const TCHAR* Name) { return FindCompactBone(Bones,Name); };
            const FCompactPoseBoneIndex Pelvis=Index(TEXT("pelvis"));
            const FCompactPoseBoneIndex Feet[]={Index(TEXT("foot_l")),Index(TEXT("foot_r"))};
            const FCompactPoseBoneIndex Thighs[]={Index(TEXT("thigh_l")),Index(TEXT("thigh_r"))};
            const FCompactPoseBoneIndex Calves[]={Index(TEXT("calf_l")),Index(TEXT("calf_r"))};
            const bool bHasDeathBones=ValidCompactBone(Output.Pose,Pelvis) && ValidCompactBone(Output.Pose,Feet[0]) && ValidCompactBone(Output.Pose,Feet[1])
                && ValidCompactBone(Output.Pose,Thighs[0]) && ValidCompactBone(Output.Pose,Thighs[1])
                && ValidCompactBone(Output.Pose,Calves[0]) && ValidCompactBone(Output.Pose,Calves[1]);
            if (!bHasDeathBones) ++Diagnostics.InvalidBoneFrames;
            if (bHasDeathBones)
            {
                FCSPose<FCompactPose> Pose; Pose.InitPose(Output.Pose);
                FCompactPose OldLocal; OldLocal.CopyBonesFrom(Output.Pose);
                for (FCompactPoseBoneIndex I:OldLocal.ForEachBoneIndex()) OldLocal[I]=TransitionSource[I.GetInt()];
                FCSPose<FCompactPose> Old; Old.InitPose(OldLocal);
                if (PrepareDeathEntry)
                {
                    DeathPelvisOffset=FMath::Clamp(float(Old.GetComponentSpaceTransform(Pelvis).GetLocation().Z-Pose.GetComponentSpaceTransform(Pelvis).GetLocation().Z),-24.f,0.f);
                    PrepareDeathEntry=false;
                }
                const float Phase=FMath::Clamp(DeathEntryTime/.30f,0.f,1.f);
                const float Offset=DeathPelvisOffset*(1-Phase*Phase*(3-2*Phase));
                if (Offset<-.05f)
                {
                    FTransform FootTargets[]={Pose.GetComponentSpaceTransform(Feet[0]),Pose.GetComponentSpaceTransform(Feet[1])};
                    FTransform Hip=Pose.GetComponentSpaceTransform(Pelvis); Hip.AddToTranslation(FVector(0,0,Offset));
                    const FBoneTransform HipChange(Pelvis,Hip); Pose.LocalBlendCSBoneTransforms(MakeArrayView(&HipChange,1),1.f);
                    for (int32 Side=0;Side<2;++Side)
                    {
                        const auto Thigh=Thighs[Side],Calf=Calves[Side];
                        FTransform A=Pose.GetComponentSpaceTransform(Thigh),C=Pose.GetComponentSpaceTransform(Calf),F=Pose.GetComponentSpaceTransform(Feet[Side]);
                        const FVector KneeHint=Old.GetComponentSpaceTransform(Calf).GetLocation();
                        AnimationCore::SolveTwoBoneIK(A,C,F,KneeHint,FootTargets[Side].GetLocation(),false,1.,1.);
                        F.SetRotation(FootTargets[Side].GetRotation());
                        const FBoneTransform Leg[]={FBoneTransform(Thigh,A),FBoneTransform(Calf,C),FBoneTransform(Feet[Side],F)};
                        Pose.LocalBlendCSBoneTransforms(MakeArrayView(Leg),1.f);
                    }
                    FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Pose,Output.Pose);
                }
            }
        }
        if (BlendDuration>0 && BlendTime<BlendDuration && TransitionSource.Num()==Count)
        {
            const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
            const auto Bone=[&Bones](const TCHAR* Name) { return FindCompactBone(Bones,Name); };
            const FCompactPoseBoneIndex UpperIndex=Bone(TEXT("upperarm_l")),LowerIndex=Bone(TEXT("lowerarm_l"));
            const FCompactPoseBoneIndex LeftIndex=Bone(TEXT("hand_l"));
            // During bolt work the right wrist leaves the stock. Anchor the support
            // hand to the independently compensated rifle, with a V07 fallback.
            // An officer's pistol already inherits hand_r directly. Solving that
            // hand toward its own descendant would introduce a circular target;
            // both officer arms retain their authored command/reload gestures.
            // Anatomical finger locals are untouched for every role.
            const FCompactPoseBoneIndex WeaponIndex=Bone(TEXT("weapon_rifle"));
            const FCompactPoseBoneIndex RightIndex=WeaponIndex.GetInt()!=INDEX_NONE?WeaponIndex:Bone(TEXT("hand_r"));
            // Burning, surrender and celebrations deliberately release the
            // support hand. Their transition must retain the open-hand gesture.
            const FString ActionName=CurrentAsset->GetName();
            const bool NativeFreeHandAction=CurrentAsset->GetPathName().Contains(TEXT("/FormalV82/"))
                &&(ActionName==TEXT("A_Engineer_Clear_R3")||ActionName==TEXT("A_Signals_Relay_R3")
                    ||ActionName.StartsWith(TEXT("A_WireCut_V40_Human67"))||ActionName.StartsWith(TEXT("A_AH_Train_")));
            const bool bWantsGrip=!bPersonalPistol && !NativeFreeHandAction && !FrontlineMotionV40::IsAction(CurrentAsset) && !ActionName.StartsWith(TEXT("A_Death"))
                && !ActionName.StartsWith(TEXT("A_BurningFall_")) && !ActionName.StartsWith(TEXT("A_Victory_"))
                && !ActionName.StartsWith(TEXT("A_Surrender"));
            const bool HasGripBones=bWantsGrip && ValidCompactBone(Output.Pose,UpperIndex) && ValidCompactBone(Output.Pose,LowerIndex)
                && ValidCompactBone(Output.Pose,LeftIndex) && ValidCompactBone(Output.Pose,RightIndex)
                && Bones.GetParentBoneIndex(LowerIndex)==UpperIndex && Bones.GetParentBoneIndex(LeftIndex)==LowerIndex;
            if (bWantsGrip && !HasGripBones) ++Diagnostics.InvalidBoneFrames;
            FTransform ApprovedGrip=FTransform::Identity,SourceGrip=FTransform::Identity;
            FTransform ApprovedRightGrip=FTransform::Identity,SourceRightGrip=FTransform::Identity;
            const auto RightHandIndex=Bone(TEXT("hand_r"));
            FVector IncomingElbowBend=FVector::ZeroVector,SourceElbowBend=FVector::ZeroVector;
            if (HasGripBones)
            {
                FCSPose<FCompactPose> Incoming;
                Incoming.InitPose(Output.Pose);
                ApprovedGrip=Incoming.GetComponentSpaceTransform(LeftIndex).GetRelativeTransform(Incoming.GetComponentSpaceTransform(RightIndex));
                if(ValidCompactBone(Output.Pose,RightHandIndex))ApprovedRightGrip=Incoming.GetComponentSpaceTransform(RightHandIndex).GetRelativeTransform(Incoming.GetComponentSpaceTransform(RightIndex));
                IncomingElbowBend=Incoming.GetComponentSpaceTransform(LowerIndex).GetLocation()-Incoming.GetComponentSpaceTransform(UpperIndex).GetLocation();
                FCompactPose PreviousLocal;
                PreviousLocal.CopyBonesFrom(Output.Pose);
                for (FCompactPoseBoneIndex Index:PreviousLocal.ForEachBoneIndex()) PreviousLocal[Index]=TransitionSource[Index.GetInt()];
                FCSPose<FCompactPose> Previous;
                Previous.InitPose(PreviousLocal);
                SourceGrip=Previous.GetComponentSpaceTransform(LeftIndex).GetRelativeTransform(Previous.GetComponentSpaceTransform(RightIndex));
                if(ValidCompactBone(Output.Pose,RightHandIndex))SourceRightGrip=Previous.GetComponentSpaceTransform(RightHandIndex).GetRelativeTransform(Previous.GetComponentSpaceTransform(RightIndex));
                SourceElbowBend=Previous.GetComponentSpaceTransform(LowerIndex).GetLocation()-Previous.GetComponentSpaceTransform(UpperIndex).GetLocation();
            }
            const float T=FMath::Clamp(BlendTime/BlendDuration,0.f,1.f);
            const float Weight=T*T*T*(T*(T*6.f-15.f)+10.f);
            LocomotionWeightV46=FMath::Lerp(TransitionLocomotionWeightV46,LocomotionWeightV46,Weight);
            // Use both evaluated endpoints; switching immediately to the incoming grip would
            // turn a queued/retargeted pose difference into a discontinuous hand target.
            const FTransform IncomingGrip=ApprovedGrip;
            ApprovedGrip.Blend(SourceGrip,IncomingGrip,Weight);
            const FTransform IncomingRightGrip=ApprovedRightGrip;ApprovedRightGrip.Blend(SourceRightGrip,IncomingRightGrip,Weight);
            for (FCompactPoseBoneIndex Index:Output.Pose.ForEachBoneIndex())
            {
                const FTransform Incoming=Output.Pose[Index];
                FTransform Outgoing=TransitionSource[Index.GetInt()];
                const int32 Mesh=Output.Pose.GetBoneContainer().MakeMeshPoseIndex(Index).GetInt();
                if(const FVector* Velocity=TransitionVelocityV65.Find(Mesh);Velocity&&!Velocity->IsNearlyZero())
                {
                    const float Time=.055f*(1-FMath::Exp(-BlendTime/.055f));
                    const float Angle=FMath::Min(float(Velocity->Size())*Time,FMath::DegreesToRadians(7.f));
                    Outgoing.SetRotation((FQuat(Velocity->GetSafeNormal(),Angle)*Outgoing.GetRotation()).GetNormalized());
                    Diagnostics.MaximumInertialAngleV65=FMath::Max(Diagnostics.MaximumInertialAngleV65,FMath::RadiansToDegrees(Angle));
                }
                Output.Pose[Index].Blend(Outgoing,Incoming,Weight);
                Output.Pose[Index].NormalizeRotation();
            }
            if (HasGripBones && !NationalCarryGrip(Output.Pose,ApprovedGrip,ApprovedRightGrip) && !bNationalAutomatic)
            {
                FCSPose<FCompactPose> ComponentPose;
                ComponentPose.InitPose(Output.Pose);
                FTransform Upper=ComponentPose.GetComponentSpaceTransform(UpperIndex);
                FTransform Lower=ComponentPose.GetComponentSpaceTransform(LowerIndex);
                FTransform Hand=ComponentPose.GetComponentSpaceTransform(LeftIndex);
                const FTransform Target=ApprovedGrip*ComponentPose.GetComponentSpaceTransform(RightIndex);
                const float UpperLength=FVector::Dist(Upper.GetLocation(),Lower.GetLocation()),LowerLength=FVector::Dist(Lower.GetLocation(),Hand.GetLocation());
                const float Reach=UpperLength+LowerLength;
                const float RequiredReach=FVector::Dist(Upper.GetLocation(),Target.GetLocation());
                MaximumGripCorrectionCm=FMath::Max(MaximumGripCorrectionCm,float(FVector::Dist(Hand.GetLocation(),Target.GetLocation())));
                // A projected, persistent elbow plane avoids the solver choosing
                // a different arbitrary plane near a straight-arm singularity.
                // Keep actual arm lengths; authored finger locals are untouched.
                const FVector EndpointBend=FMath::Lerp(SourceElbowBend,IncomingElbowBend,Weight);
                const FVector Pole=StableElbowTarget(Upper,Lower,Hand,Target.GetLocation(),EndpointBend,0);
                AnimationCore::SolveTwoBoneIK(Upper,Lower,Hand,Pole,Target.GetLocation(),false,1.,1.);
                Hand.SetRotation(Target.GetRotation());
                const FBoneTransform Adjusted[]={FBoneTransform(UpperIndex,Upper),FBoneTransform(LowerIndex,Lower),FBoneTransform(LeftIndex,Hand)};
                ComponentPose.LocalBlendCSBoneTransforms(MakeArrayView(Adjusted),1.f);
                const float Residual=FVector::Dist(ComponentPose.GetComponentSpaceTransform(LeftIndex).GetLocation(),Target.GetLocation());
                const float UpperError=FMath::Abs(float(FVector::Dist(ComponentPose.GetComponentSpaceTransform(UpperIndex).GetLocation(),ComponentPose.GetComponentSpaceTransform(LowerIndex).GetLocation()))-UpperLength);
                const float LowerError=FMath::Abs(float(FVector::Dist(ComponentPose.GetComponentSpaceTransform(LowerIndex).GetLocation(),ComponentPose.GetComponentSpaceTransform(LeftIndex).GetLocation()))-LowerLength);
                Diagnostics.MaximumArmLengthErrorCm=FMath::Max(Diagnostics.MaximumArmLengthErrorCm,FMath::Max(UpperError,LowerError));
                if (Residual>1.f && Residual>MaximumGripResidualCm+5.f)
                    UE_LOG(LogTemp,Warning,TEXT("WF_GRIP_DIAG from=%s asset=%s alpha=%.4f residual=%.4f reach=%.3f required=%.3f grip=%s source_grip=%s incoming_grip=%s upper=%s target=%s scale=%s bones=%d/%d/%d/%d parents=%d/%d"),
                        *TransitionFrom.ToString(),*GetNameSafe(CurrentAsset),Weight,Residual,Reach,RequiredReach,*ApprovedGrip.GetLocation().ToString(),
                        *SourceGrip.GetLocation().ToString(),*IncomingGrip.GetLocation().ToString(),
                        *Upper.GetLocation().ToString(),*Target.GetLocation().ToString(),*Upper.GetScale3D().ToString(),
                        UpperIndex.GetInt(),LowerIndex.GetInt(),LeftIndex.GetInt(),RightIndex.GetInt(),
                        Bones.GetParentBoneIndex(LowerIndex).GetInt(),Bones.GetParentBoneIndex(LeftIndex).GetInt());
                MaximumGripResidualCm=FMath::Max(MaximumGripResidualCm,Residual);
                FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(ComponentPose,Output.Pose);
                ++GripConstraintFrames;
            }
        }
        // After all authored/personal/foot/blend layers, before publishing the
        // displayed pose. A failed fit leaves the authored pose and existing
        // GunSupportContacts/CanFire guard untouched.
        if(bNationalGroundTargetV82)NationalGroundGunPoseV82(Output.Pose);
        if(bMountedTrenchReloadV8447&&TrenchReloadShelterV8447>0.f)MountedTrenchReloadV8447(Output.Pose);
        if(BlendDuration>0&&BlendTime<BlendDuration&&!TransitionVelocityV65.IsEmpty())++Diagnostics.InertialBlendFramesV65;
        if(PublishedStepV65!=PoseStepV65)
        {PreviousDisplayedPoseV65=DisplayedPose;PreviousMeshBonesV65=DisplayedMeshBoneIndices;PreviousPoseDeltaV65=PoseDeltaV65;PublishedStepV65=PoseStepV65;}
        DisplayedPose.SetNumUninitialized(Count);
        DisplayedMeshBoneIndices.SetNumUninitialized(Count);
        for (FCompactPoseBoneIndex Index:Output.Pose.ForEachBoneIndex())
        {
            DisplayedPose[Index.GetInt()]=Output.Pose[Index];
            DisplayedMeshBoneIndices[Index.GetInt()]=Output.Pose.GetBoneContainer().MakeMeshPoseIndex(Index).GetInt();
        }
        DisplayedAssetName=CurrentAsset->GetFName();
        Diagnostics.LocomotionPoseWeightV46=DisplayedLocomotionWeightV46=LocomotionWeightV46;
        return Result;
    }
};

float ClipLength(UAnimationAsset* Asset)
{
    const UAnimSequenceBase* Sequence=Cast<UAnimSequenceBase>(Asset);
    return Sequence?Sequence->GetPlayLength():0.f;
}
bool IsLocomotion(const AWarfareFrontlineGameMode* Battle,UAnimationAsset* Asset)
{
    return Battle && Asset && (Asset==Battle->WalkAnimation || Asset==Battle->RunAnimation || Asset==Battle->MGWalkAnimation || Asset==Battle->MGRunAnimation);
}
}

FAnimInstanceProxy* UWarfareFrontlineAnimation::CreateAnimInstanceProxy()
{
    return new FFrontlineAnimationProxy(this);
}

int32 UWarfareFrontlineAnimation::GetGripConstraintFrames()
{
    return GetProxyOnGameThread<FFrontlineAnimationProxy>().GripConstraintFrames;
}
float UWarfareFrontlineAnimation::GetMaximumGripResidualCm()
{
    return GetProxyOnGameThread<FFrontlineAnimationProxy>().MaximumGripResidualCm;
}
float UWarfareFrontlineAnimation::GetMaximumGripCorrectionCm()
{
    return GetProxyOnGameThread<FFrontlineAnimationProxy>().MaximumGripCorrectionCm;
}
FFrontlineAnimationV29Diagnostics UWarfareFrontlineAnimation::GetCharacterDiagnostics()
{
    return GetProxyOnGameThread<FFrontlineAnimationProxy>().Diagnostics;
}

void UWarfareFrontlineAnimation::TransitionTo(UAnimationAsset* Asset,bool bLoop,float Rate,float StartPosition,float BlendSeconds)
{
    if (!Asset) return;
    // GetProxyOnGameThread completes any parallel evaluation before changing its snapshot.
    GetProxyOnGameThread<FFrontlineAnimationProxy>().BeginTransition(BlendSeconds);
    SetAnimationAsset(Asset,bLoop,Rate);
    SetPosition(StartPosition,false); SetPlaying(true);
}

AWarfareFrontlineSoldier::AWarfareFrontlineSoldier()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
    Capsule=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Selection")); SetRootComponent(Capsule);
    Capsule->InitCapsuleSize(25,88); Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Capsule->SetCollisionResponseToAllChannels(ECR_Ignore); Capsule->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Soldier=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Soldier")); Soldier->SetupAttachment(Capsule);
    Soldier->SetRelativeLocation(FVector(0,0,-88)); Soldier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Soldier->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Soldier->AddTickPrerequisiteActor(this);
}

void AWarfareFrontlineSoldier::Initialize(AWarfareFrontlineGameMode* InBattle,int32 InSquad,int32 InTeam,EFrontlineRole InRole,int32 InMember)
{
    bFlameBulletProtectionSpentV48=false;
    Battle=InBattle; SquadId=InSquad; Team=InTeam; Role=InRole; MemberIndex=InMember;
    // A separate deterministic stream does not alter battle accuracy or artillery randomness.
    MotionRandom.Initialize(int32(uint32(SquadId)*92821u ^ uint32(MemberIndex+1)*68917u ^ uint32(Team+1)*283923u ^ 191710u));
    MotionPhase=MotionRandom.FRand(); MovementScale=MotionRandom.FRandRange(.93f,1.07f);
    CadenceScale=MotionRandom.FRandRange(.91f,1.13f); BreathingRate=MotionRandom.FRandRange(.83f,1.17f);
    ReactionSeconds=MotionRandom.FRandRange(.12f,.78f);
    RoundsRemaining=GetMagazineCapacity();
    FormationOffset=FVector(MotionRandom.FRandRange(-29.f,29.f),MotionRandom.FRandRange(-28.f,28.f),0);
    ShotClock=MotionRandom.FRandRange(.5f,3.5f); GrenadeClock=MotionRandom.FRandRange(2.f,5.f);ArmourGrenadeReadyAtV833=-1.f;
    if(WarfareFRRUCampaignV82::Active(Battle))
    {
        FString Error;
        if(!WarfareFRRUPresentationV82::InitializeUnit(this,Error)){Battle->AssetFailure=Error;return;}
        bCommissionedOfficer=WarfareFRRUPresentationV82::UsesPistol(this);
        bScopedWeapon=Role==EFrontlineRole::Marksman&&MemberIndex==0;bStormWeapon=false;
        Soldier->SetReceivesDecals(false);Soldier->SetAnimInstanceClass(UWarfareFrontlineAnimation::StaticClass());
        // The national model already contains its exact dated role equipment.
        // Old officer/specialist/field initialization can replace that body and
        // issue UK bags/masks. There is no national mask issue policy yet.
        SetMotion(GetIdleMotion());InitializeWeaponEffects();InitializeFrontlineAppearanceV29(this);
        FrontlineMachinegunsV30::Initialize(this);return;
    }
    const int32 MeshIndex=GetPresentationRoleIndex();
    Soldier->SetSkeletalMeshAsset(Battle->V16SoldierMeshes.IsValidIndex(MeshIndex)?Battle->V16SoldierMeshes[MeshIndex].Get():nullptr);
    Soldier->SetReceivesDecals(false); // Ground blood must not paint living men or the complete death proxy.
    // Role instances retain the approved material maps. A small personal fabric
    // variation breaks identical dye/weathering without tinting skin or weapons.
    for (int32 Slot=0;Slot<Soldier->GetNumMaterials();++Slot)
        if (const auto* Material=Soldier->GetMaterial(Slot); Material && (Material->GetName().StartsWith(TEXT("MI_V13_")) || Material->GetName().StartsWith(TEXT("MI_V15_")) || Material->GetName().StartsWith(TEXT("MI_V16_"))))
            if (auto* Personal=Soldier->CreateDynamicMaterialInstance(Slot))
            {
                const float Dye=.955f+MotionPhase*.085f;
                Personal->SetVectorParameterValue(TEXT("PersonalWeatheringTint"),FLinearColor(Dye,Dye*(.992f+MotionPhase*.016f),Dye,1));
            }
    Soldier->SetRelativeRotation(FRotator(0,Battle->MeshYawOffset,0));
    Soldier->SetAnimInstanceClass(UWarfareFrontlineAnimation::StaticClass());
    ConfigureSpecialistEquipment();
    ConfigureOfficerEquipment();
    FrontlineEquipmentV41::Initialize(this);
    FrontlineMaterialV27::Apply(this);FrontlineFieldV32::InitializeUnit(this);
    SetMotion(GetIdleMotion());
    InitializeWeaponEffects();
    InitializeFrontlineAppearanceV29(this);
    FrontlineMachinegunsV30::Initialize(this);
    if(auto* Wound=AWarfareFrontlineWoundFX::Get(GetWorld()))Wound->PrepareRoleV8216(Soldier->GetSkeletalMeshAsset());
}

int32 AWarfareFrontlineSoldier::GetPresentationRoleIndex() const
{
    const int32 Identity=IsMachineGunner()?1:(Role==EFrontlineRole::Lewis||Role==EFrontlineRole::MobileGun)?5:
        Role==EFrontlineRole::Mills?2:Role==EFrontlineRole::Marksman?3:(IsSectionLeader()||(Role==EFrontlineRole::Signals&&MemberIndex==0))?4:0;
    return int32(GetFaction())*6+Identity;
}

void UWarfareFrontlineAnimation::FreezeForRagdoll()
{
    GetProxyOnGameThread<FFrontlineAnimationProxy>().FreezeForRagdoll();
    SetPlaying(false);
}

UAnimationAsset* AWarfareFrontlineSoldier::GetIdleMotion() const {
#if WITH_EDITOR
    if(FrontlineTripoCharacterV87::IsBound(this))return FrontlineTripoCharacterV87::Motion(this,TEXT("Idle"));
#endif
    if(WarfareFRRUCampaignV82::Active(Battle)){if(auto* A=FrontlineMachinegunsV30::Action(this,TEXT("Aim")))return A;return WarfareFRRUPresentationV82::Motion(this,TEXT("Idle"));} if(auto* A=FrontlineMotionV41::CarryIdle(this))return A; if(FrontlineFieldV32::IsFlameOperator(this))if(auto* A=FrontlineMotionV34::Flame(TEXT("LowReady")))return A; if(auto* A=FrontlineMachinegunsV30::Action(this,TEXT("Aim")))return A; return !Battle?nullptr:IsCommissionedOfficer()?OfficerMotions[0].Get():IsMachineGunner()?Battle->MGIdleAnimation.Get():Battle->IdleAnimation.Get(); }
UAnimationAsset* AWarfareFrontlineSoldier::GetAimMotion() const {
#if WITH_EDITOR
    if(FrontlineTripoCharacterV87::IsBound(this))return FrontlineTripoCharacterV87::Motion(this,TEXT("Aim"));
#endif
    if(WarfareFRRUCampaignV82::Active(Battle))
    {
        // A temporary route join is still a carry state, not a deployment.
        if(IsMachineGunner()&&!bEngaging&&WarfareFRRUWorldV82::WaitingToJoinFormation(this))
            if(const auto* Gun=FrontlineMachinegunsV30::Get(this);Gun&&Gun->State==EFrontlineGunStanceV30::Carry)
                return GetIdleMotion();
        if(auto* A=FrontlineMachinegunsV30::Action(this,TEXT("Aim")))return A;
        return WarfareFRRUPresentationV82::Motion(this,TEXT("Aim"));
    } if(Role==EFrontlineRole::RifleGrenadier)if(auto* A=FrontlineMotionV41::SpecialistAction(this,TEXT("Aim"),false))return A; if(FrontlineFieldV32::IsFlameOperator(this))if(auto* A=FrontlineMotionV34::Flame(TEXT("Aim")))return A; if(auto* A=FrontlineMachinegunsV30::Action(this,TEXT("Aim")))return A; return !Battle?nullptr:IsCommissionedOfficer()?OfficerMotions[1].Get():IsMachineGunner()?Battle->MGAimAnimation.Get():Battle->AimAnimation.Get(); }
UAnimationAsset* AWarfareFrontlineSoldier::GetReloadMotion() const {
#if WITH_EDITOR
    if(FrontlineTripoCharacterV87::IsBound(this))return FrontlineTripoCharacterV87::Motion(this,TEXT("Reload"));
#endif
    if(WarfareFRRUCampaignV82::Active(Battle)){if(auto* A=FrontlineMachinegunsV30::Action(this,TEXT("Reload")))return A;return WarfareFRRUPresentationV82::Motion(this,TEXT("Reload"));} if(Role==EFrontlineRole::RifleGrenadier)if(auto* A=FrontlineMotionV41::SpecialistAction(this,TEXT("Reload"),false))return A; if(auto* A=FrontlineMachinegunsV30::Action(this,TEXT("Reload")))return A; return !Battle?nullptr:IsCommissionedOfficer()?OfficerMotions[5].Get():IsStormtrooper()?StormReloadMotion.Get():IsMachineGunner()?(GetFaction()==EFrontlineFaction::British?Battle->LewisReloadAnimation.Get():Battle->GermanMGReloadAnimation.Get()):Battle->ReloadAnimation.Get(); }
UAnimationAsset* AWarfareFrontlineSoldier::GetWalkMotion() const {
#if WITH_EDITOR
    if(FrontlineTripoCharacterV87::IsBound(this))return FrontlineTripoCharacterV87::Motion(this,TEXT("Walk"));
#endif
    if(WarfareFRRUCampaignV82::Active(Battle))return WarfareFRRUPresentationV82::Motion(this,TEXT("Walk")); if(FrontlineFieldV32::IsFlameOperator(this))if(auto* A=FrontlineMotionV34::Flame(TEXT("Walk")))return A; return !Battle?nullptr:IsCommissionedOfficer()?OfficerMotions[2].Get():IsMachineGunner()?Battle->MGWalkAnimation.Get():Battle->WalkAnimation.Get(); }
UAnimationAsset* AWarfareFrontlineSoldier::GetRunMotion() const {
#if WITH_EDITOR
    if(FrontlineTripoCharacterV87::IsBound(this))return FrontlineTripoCharacterV87::Motion(this,TEXT("Run"));
#endif
    if(WarfareFRRUCampaignV82::Active(Battle))return WarfareFRRUPresentationV82::Motion(this,TEXT("Run")); if(FrontlineFieldV32::IsFlameOperator(this))if(auto* A=FrontlineMotionV34::Flame(TEXT("Run")))return A; return !Battle?nullptr:IsCommissionedOfficer()?OfficerMotions[3].Get():IsMachineGunner()?Battle->MGRunAnimation.Get():Battle->RunAnimation.Get(); }

void AWarfareFrontlineSoldier::SetMotion(UAnimationAsset* Animation,bool bLoop,bool bAlreadyResolved)
{
    const bool National=WarfareFRRUCampaignV82::Active(Battle);
    if(!bAlreadyResolved)
    {
        if(National)Animation=WarfareFRRUPresentationV82::RemapMotion(this,Animation);
#if WITH_EDITOR
        Animation=FrontlineTripoCharacterV87::RemapMotion(this,Animation);
#endif
    }
    else if(!Animation||!Soldier||!Soldier->GetSkeletalMeshAsset()
        ||Animation->GetSkeleton()!=Soldier->GetSkeletalMeshAsset()->GetSkeleton())
    {UE_LOG(LogTemp,Error,TEXT("WF_RESOLVED_MOTION_SKELETON_REFUSED squad=%d member=%d clip=%s"),SquadId,MemberIndex,*GetNameSafe(Animation));return;}
    if (!Animation || Playing==Animation) return;
    UWarfareFrontlineAnimation* Node=Cast<UWarfareFrontlineAnimation>(Soldier->GetAnimInstance());
    if (!Node) return;
    float Phase=MotionPhase;
    // Walk/run changes preserve the current normalized stride rather than restarting the footfall.
    const auto Identity=[this](UAnimationAsset* Asset) {
#if WITH_EDITOR
        return FrontlineTripoCharacterV87::LegacyEquivalent(this,Asset);
#else
        return Asset;
#endif
    };
    const auto Locomotion=[this,&Identity](UAnimationAsset* Asset) { Asset=Identity(Asset);return IsLocomotion(Battle,Asset) || (Asset&&(Asset==Identity(GetWalkMotion()) || Asset==Identity(GetRunMotion()))); };
    if (Locomotion(Playing) && Locomotion(Animation) && Node->GetLength()>.01f)
        Phase=FMath::Fmod(Node->GetCurrentTime()/Node->GetLength(),1.f);
    UAnimationAsset* ActionIdentity=Identity(Animation);
    const bool IsFire=ActionIdentity==FrontlineMachinegunsV30::Action(this,TEXT("Fire")) || ActionIdentity==Battle->FireAnimation || ActionIdentity==Battle->AutomaticFireAnimation || ActionIdentity==StormFireMotion
        || (IsCommissionedOfficer() && OfficerMotions.IsValidIndex(4) && ActionIdentity==OfficerMotions[4])
        || (National && ActionIdentity==WarfareFRRUPresentationV82::Motion(this,TEXT("Fire")));
    const float Start=bLoop && !IsFire?Phase*ClipLength(Animation):0.f;
    const FString Action=ActionIdentity->GetName();
    const bool Fatal=Action.StartsWith(TEXT("A_Death"))||Action.StartsWith(TEXT("A_BurningFall"))||Action.StartsWith(TEXT("A_Gas"));
    const bool Stance=Action.Contains(TEXT("_Deploy"))||Action.Contains(TEXT("_Pack"));
    const bool Gesture=Action.StartsWith(TEXT("A_Wait"))||Action.StartsWith(TEXT("A_Victory"))||Action.StartsWith(TEXT("A_Surrender"));
    // Preserve fast recoil; give a larger carry/aim or gesture change enough
    // overlap, and match contiguous deployment endpoints with a short blend.
    const float Blend=!Playing?0.f:Fatal?.08f:IsFire?.035f:Stance?.10f:Gesture?.24f:
        Locomotion(Animation)?(Locomotion(Playing)?.18f:.21f):Locomotion(Playing)?.20f:.18f;
    // Use measured travel on the first frame too: SetAnimationAsset may evaluate
    // immediately, before the ordinary locomotion tick updates its play rate.
    const float Rate=Locomotion(Animation)?FMath::Clamp(MotionSpeed/(ActionIdentity==Identity(GetRunMotion())?390.f:210.f),.08f,1.45f)
        :ActionIdentity==Identity(GetIdleMotion()) || ActionIdentity==Identity(GetAimMotion())?BreathingRate:1.f;
    Node->TransitionTo(Animation,bLoop,Rate,Start,Blend);
    Playing=Animation; LastAnimationTime=Start; ++MotionTransitions;
}

void AWarfareFrontlineSoldier::ReturnToIdle()
{
    if (!Battle || !IsAlive()) return;
    bMoving=bEngaging=bWantedMovement=bHadAdvanceContact=false;
    MotionSpeed=RequestedTravelSpeedV43=TrenchFallSpeedV69=0;bAtDestinationV43=false; FirePoseClock=ContactReactionClock=ContactHoldClock=0; FocusTarget.Reset();
    WeaponPhase=EFrontlineWeaponPhase::Ready; BurstRemaining=0;
    NativeInfantryObstructionSecondsV87=0.f;NativeInfantryRecoveryOriginV87=FVector::ZeroVector;
    SetMotion(GetIdleMotion());
}

bool AWarfareFrontlineSoldier::IsRallied() const
{
    return Battle && IsAlive() && Battle->Phase==EFrontlinePhase::Fighting && RallyUntil>Battle->Elapsed;
}

bool AWarfareFrontlineSoldier::IsActiveCombatant() const
{
    if(WarfareFRRUCampaignV82::Active(Battle)&&WarfareFRRUArmourV82::Embarked(this))return false;
    // Eligibility is spatial, not dependent on which actor ticked first. A
    // visible reserve arriving at the rear can be engaged on that same frame.
    return IsAlive() && (bFieldEntered || (Battle && FrontlinePacingV24::HasCrossedEntry(Feet().X,Battle->GetTeamDirection(Team))));
}

void AWarfareFrontlineSoldier::Tick(float DT)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Frontline_SoldierTick);
    Super::Tick(DT);
    if (!Battle || DT<=0) return;
    if(WarfareFRRUCampaignV82::Active(Battle)&&WarfareFRRUArmourV82::Embarked(this))return;
    if(WarfareFRRUCampaignV82::Active(Battle))WarfareFRRUPresentationV82::TickUnit(this,DT);
    FVector FlameAim;
    const bool BracingFlame=FrontlineFieldV32::FlameBraceDirectionV33(this,FlameAim);
    if(BracingFlame)
    {
        bMoving=bWantedMovement=false;bEngaging=true;MotionSpeed=0;
        SetActorRotation(FlameAim.Rotation());SetMotion(GetAimMotion());
    }
    TickWeaponEffects(DT);FrontlineFieldV32::TickUnit(this,DT);
    if (!IsAlive()) { ArmourGrenadeReadyAtV833=-1.f;FrontlineMotionV40::Abort(this);TickDeath(DT); return; }
    UAnimSingleNodeInstance* Node=Soldier->GetSingleNodeInstance();
    if (Node)
    {
        const float Time=Node->GetCurrentTime();
        // A covering assistant may take a genuine backward step at his fixed
        // station. Count its accepted reverse stride, not almost one full loop.
        float Advance=Node->GetPlayRate()<0.f?LastAnimationTime-Time:Time-LastAnimationTime;
        if (Advance<0 && Node->IsLooping()) Advance+=Node->GetLength();
        AnimationTimeAdvanced+=FMath::Max(0.f,Advance); LastAnimationTime=Time;
    }
    if (Battle->Phase!=EFrontlinePhase::Fighting) {FrontlineMotionV40::Abort(this);return;}
    if(FrontlineMotionV40::TickGasEscapeV8216(this,DT))return;
    if(!bFieldEntered && FrontlinePacingV24::HasCrossedEntry(Feet().X,Battle->GetTeamDirection(Team)))
    {bFieldEntered=true;UE_LOG(LogTemp,Display,TEXT("WF_V24_REINFORCEMENT_ENTERED squad=%d member=%d team=%d x=%.1f"),SquadId,MemberIndex,Team,Feet().X);}
    ShotClock-=DT*(IsRallied()?1.32f:1.f); GrenadeClock-=DT*FrontlinePacingV24::GrenadeReadinessRate*FrontlineGrenade::ReadinessRateMultiplier; AimSettleClock-=DT; TargetReviewClock-=DT;
    if(WarfareFRRUCampaignV82::Active(Battle))
    {
        UpdateWeaponCycle(DT);
        if(WarfareFRRUPresentationV82::TickWork(this))return;
        if(FrontlineMotionV40::TickAction(this,DT))return;
    }
    else
    {
        // Equipment actions own the body and pause an interrupted weapon
        // action. Advancing the reload first both delayed the gas response and
        // let its completion overwrite the hands during mask donning.
        if(FrontlineMotionV40::TickAction(this,DT))return;
        UpdateWeaponCycle(DT);
    }
    if(BracingFlame)return;
    if(WeaponPhase==EFrontlineWeaponPhase::Ready&&FrontlineFieldV32::IsMineWorkingV8215(this))
    {
        // Work eligibility includes infantry, hostile hull contact and friendly
        // hull yielding. Failed eligibility continues normal combat/movement.
        bMoving=bWantedMovement=bEngaging=false;MotionSpeed=RequestedTravelSpeedV43=0;
        FocusTarget.Reset();BurstRemaining=0;SetMotion(GetIdleMotion());return;
    }
    if (WeaponPhase==EFrontlineWeaponPhase::OfficerCommand) return;
    Suppression=FMath::Max(0.f,Suppression-DT*(Battle->CoverFor(this)>.4f?.095f:.065f));
    if (UpdateMachineGunRecovery(DT)) return;
    // Recovery/actions above retain their own component handling. This live
    // pointer is local to the remaining tick; those paths never replace it.
    UFrontlineMachinegunV30* const TickGunV834=FrontlineMachinegunsV30::Get(this);
    const bool ReviewSupportV834=FrontlineSupportReuseV834::Enabled(Battle);
    if(ReviewSupportV834)++FrontlineSupportReuseV834::State().TickLookups;
    const auto GunForTickV834=[&]()
    {
        if(ReviewSupportV834)
        {
            auto& S=FrontlineSupportReuseV834::State();++S.Requests;++S.PointerChecks;
            if(FrontlineMachinegunsV30::Get(this)!=TickGunV834)++S.PointerFailures;
        }
        return TickGunV834;
    };
    if(auto* Gun=GunForTickV834())Gun->ReviewTrenchFiringSectorV75();
    FrontlineInfantryV45::ReviewTrenchFiringSectorV844(this);
    bool bCrewReform=false;
    FVector DestinationV8218=FrontlineMachinegunsV30::Destination(this,Battle->SoldierDestination(this),bCrewReform);
    FVector PioneerExitV8445;
    const bool PioneerEgressV8445=FrontlineArmourV63::PioneerEgressDestinationV8445(this,PioneerExitV8445);
    bCrewReform|=PioneerEgressV8445;
    const bool ArmourYieldV8218=FrontlineMobilityV41::ArmourYieldDestinationV8218(this,DestinationV8218);
    bCrewReform|=ArmourYieldV8218;
    FVector Delta=DestinationV8218-Feet(); Delta.Z=0;
    const float Distance=Delta.Size2D();
    const FFrontlineSquad* Squad=Battle->FindSquad(SquadId);
    const AFrontlineTank* EscortTank=FrontlineArmourV63::EscortFor(this);
    AWarfareFrontlineSoldier* Contact=Squad && Squad->bAdvancing?Battle->AcquireTarget(this,true):nullptr;
    // Retain a visible established contact through the narrow stopping-range
    // boundary. Repeatedly crossing that boundary used to order a heavy gun
    // to pack, walk a few centimetres, then deploy again. No retained contact
    // grants a shot: discharge keeps its ordinary range and muzzle sight gate.
    if(!Contact&&Squad&&Squad->bAdvancing&&bEngaging)
        if(auto* Prior=FocusTarget.Get();Prior&&Prior->Team!=Team&&Prior->IsActiveCombatant())
        {
            // Match advancing acquisition's concealment and smoke infiltration
            // scale. A newly arriving friendly smoke screen may shorten an
            // established contact too; keep the existing 220 cm hysteresis.
            const float Concealment=Role!=EFrontlineRole::Mills&&Battle->DirectFireMultiplier(Feet(),Prior->Feet())<1.f?.65f:1.f;
            const float AssaultScaleV87=FrontlineUseLegacyContactRetentionV87()?1.f:Battle->AssaultContactScale(this);
            if(FVector::Dist2D(Feet(),Prior->Feet())<=FrontlineWeapons::AdvanceContactRangeFor(this)*Battle->EncounterVisibilityScale()*Concealment*AssaultScaleV87+220.f
                &&Battle->HasSight(this,Prior))Contact=Prior;
        }
    // An emplaced gun keeps a visible contact within its service firing range.
    // The march stopping radius and the transient firing Focus are not orders
    // to dismantle it. Discharge still requires the actual animated muzzle ray.
    if(!Contact&&Squad&&Squad->bAdvancing&&IsMachineGunner()
        &&TickGunV834&&TickGunV834->IsSupported()&&!WarfareFRRUCampaignV82::Active(Battle))
    {
        auto* Seen=FocusTarget.Get();
        if(!FrontlineMachinegunsV30::ValidContactV61(this,Seen))Seen=Battle->AcquireTarget(this);
        // Feed/recoil temporarily lower the muzzle; finish that handling. A
        // Ready gun with a genuinely obstructed muzzle must remain free to move.
        if(Seen&&(WeaponPhase!=EFrontlineWeaponPhase::Ready||TickGunV834->HasInfantryFiringLine(Seen)))Contact=Seen;
    }
    // An eye-level observation cannot hold a native crew indefinitely behind
    // an earth bank which its planted muzzle cannot clear. Reuse the existing
    // armour-obstruction recovery: finish handling, then follow the unchanged
    // route for a real 120 cm before evaluating another placement. A packed
    // carry muzzle rising above the bank cannot cancel that committed step.
    const bool NativeCrewAdvanceV87=WarfareFRRUCampaignV82::Active(Battle)&&TickGunV834
        &&(Role==EFrontlineRole::Lewis||Role==EFrontlineRole::MobileGun)&&Squad&&Squad->bAdvancing
        &&!Squad->bHoldPosition&&bFieldEntered&&!bCrewReform&&Distance>10.f
        &&!WarfareFRRUWorldV82::WaitingToJoinFormation(this);
    if(!NativeCrewAdvanceV87)NativeInfantryObstructionSecondsV87=0.f;
    else
    {
        if(NativeInfantryObstructionSecondsV87>=2.f)
        {
            if(FVector::DistSquared2D(Feet(),NativeInfantryRecoveryOriginV87)>=FMath::Square(120.f))
                NativeInfantryObstructionSecondsV87=0.f;
        }
        else if(Contact)
        {
            const bool EvaluatingAimV87=WeaponPhase==EFrontlineWeaponPhase::Ready&&TickGunV834->IsSupported()
                &&(Playing==GetAimMotion()||Playing==TickGunV834->Action(TEXT("Fire")));
            bool WorkingPrimaryV87=false;
            if(!IsMachineGunner())if(const auto* Primary=FrontlineMachinegunsV30::LivingGunner(this))
                if(const auto* Mount=FrontlineMachinegunsV30::Get(Primary);Mount&&Mount->IsSupported())
                    WorkingPrimaryV87=Primary->WeaponPhase!=EFrontlineWeaponPhase::Ready||Mount->CanFire();
            if(WorkingPrimaryV87)NativeInfantryObstructionSecondsV87=0.f;
            else if(EvaluatingAimV87)
            {
                const auto* ActualFocusV87=FocusTarget.Get();
                const bool ExecutableV87=TickGunV834->CanFire()
                    &&(WarfareFRRUWorldV82::FiringLine(this,Contact)
                        ||(FrontlineMachinegunsV30::ValidContactV61(this,ActualFocusV87)
                            &&WarfareFRRUWorldV82::FiringLine(this,ActualFocusV87)));
                if(ExecutableV87)NativeInfantryObstructionSecondsV87=0.f;
                else
                {
                    NativeInfantryObstructionSecondsV87+=DT;
                    if(NativeInfantryObstructionSecondsV87>=2.f)NativeInfantryRecoveryOriginV87=Feet();
                }
            }
        }
        else NativeInfantryObstructionSecondsV87=0.f;
        if(NativeInfantryObstructionSecondsV87>=2.f)Contact=nullptr;
    }
    auto* VehicleSystem=UFrontlineVehicleSubsystem::Get(GetWorld());
    const bool AntiTankGunnerV48=FrontlineSpecialistsV41::IsAntiTankGunner(this);
    AFrontlineTank* VehicleContact=VehicleSystem&&!AntiTankGunnerV48?VehicleSystem->AcquireInfantryTarget(this):nullptr;
    if(Role==EFrontlineRole::Mills&&(!VehicleContact
        ||FVector::Dist2D(Feet(),VehicleContact->GrenadeAimPointFor(Feet()))>FrontlineGrenade::MaxRangeCm))
        ArmourGrenadeReadyAtV833=-1.f;
    // AT awareness must not disappear when deployment/reload lowers the muzzle
    // behind the same lip. Only the immediate muzzle contact may authorize fire.
    AFrontlineTank* VehicleFireContact=VehicleContact;
    if(AntiTankGunnerV48)
        if(auto* Gun=GunForTickV834())
        {VehicleContact=Gun->ObserveArmourV48();VehicleFireContact=Gun->HasArmourFiringLineV48(VehicleContact)?VehicleContact:nullptr;}
    if(VehicleContact&&VehicleSystem&&!WarfareFRRUCampaignV82::Active(Battle))
    {
        const auto* Supported=GunForTickV834();
        const bool EvaluatingAim=WeaponPhase==EFrontlineWeaponPhase::Ready&&(!Supported||Supported->IsSupported());
        const bool ClearMuzzle=VehicleSystem->HasInfantryFiringLineV828(this,VehicleContact);
        VehicleFireContact=ClearMuzzle?VehicleContact:nullptr;
        if(ArmourObstructionSecondsV828>=2.f)
        {
            // Packing raises the muzzle above the lip temporarily. Once a
            // relocation is committed, that changing pose must not cancel it
            // and immediately deploy again at the same blocked firing point.
            // This only releases the armour hold; infantry contact and normal
            // support, movement and collision gates still own actual travel.
            if(FVector::DistSquared2D(Feet(),ArmourRecoveryOriginV828)>=FMath::Square(120.f))
                ArmourObstructionSecondsV828=0.f;
        }
        else if(ClearMuzzle)ArmourObstructionSecondsV828=0.f;
        else if(EvaluatingAim)
        {
            ArmourObstructionSecondsV828+=DT;
            if(ArmourObstructionSecondsV828>=2.f)ArmourRecoveryOriginV828=Feet();
        }
    }
    else ArmourObstructionSecondsV828=0.f;
    const bool BlockingArmour=VehicleContact && Squad && Squad->bAdvancing
        &&ArmourObstructionSecondsV828<2.f
        && ((VehicleContact->GetActorLocation().X-Feet().X)*Battle->GetTeamDirection(Team)>0
            ||FVector::Dist2D(Feet(),VehicleContact->GrenadeAimPointFor(Feet()))<900.f
            ||FrontlineGameplayV49::ShareRearSector(Battle,Team,Feet(),VehicleContact->GetActorLocation()))
        && FVector::Dist2D(Feet(),VehicleContact->AimPoint())<=FrontlineWeapons::AdvanceArmourContactRangeForV8310(this)*Battle->EncounterVisibilityScale();
    // Men already crossing react separately to first contact. Waiting trench occupants
    // never acquire movement from this delay; the existing advance order is still required.
    if (Contact && !bHadAdvanceContact)
        ContactReactionClock=bMoving?FMath::Lerp(.10f,.44f,FMath::Clamp((ReactionSeconds-.12f)/.66f,0.f,1.f)):0.f;
    bHadAdvanceContact=Contact!=nullptr;
    ContactReactionClock=Contact?FMath::Max(0.f,ContactReactionClock-DT):0.f;
    const bool StopForContact=(Contact && ContactReactionClock<=0 && !Battle->ShouldFinishFriendlyTrenchEntryV69(this)) || BlockingArmour;
    ContactHoldClock=StopForContact?.22f:FMath::Max(0.f,ContactHoldClock-DT);
    const bool WasInContact=bEngaging;
    bEngaging=StopForContact || ContactHoldClock>0;
    // A stopped man does not chase centimetre-scale changes in his assigned
    // slot. A real advance or a displaced slot still wakes him immediately.
    const bool HoldingSlot=Squad&&!Squad->bAdvancing&&!bCrewReform;
    const bool SupportedCrew=GunForTickV834()!=nullptr;
    const bool GroundCrewStation=FrontlineMachinegunsV30::HasGroundStationV828(this);
    // Ground reformation deliberately sets bCrewReform, so HoldingSlot alone
    // cannot identify this local adjustment. Otherwise a bearer turns his back
    // on the gun and runs for a few centimetres whenever its station changes.
    const bool LocalCrewReformV835=SupportedCrew&&!IsMachineGunner()&&!AntiTankGunnerV48&&!ArmourYieldV8218
        &&((GroundCrewStation&&Distance<=420.f)||(HoldingSlot&&Distance<180.f))
        &&!WarfareFRRUCampaignV82::Active(Battle);
    // A prepared firing step is a physical point, not a 24 cm arrival circle.
    // That former rifle tolerance left front-rank men behind the parapet even
    // though their cached destination had a clear eye line. Apply the same
    // precise planting already used by heavy guns to the ordinary riflemen and
    // escorts assigned a firing step. National formation ownership is unchanged.
    const bool PreparedInfantryBay=HoldingSlot&&Squad&&Battle->Trenches.IsValidIndex(Squad->Trench)
        &&!Squad->bAdvancing&&!IsSectionLeader()&&!WarfareFRRUCampaignV82::Active(Battle);
    const bool PreciseGunBay=HoldingSlot&&(IsMachineGunner()||AntiTankGunnerV48||PreparedInfantryBay);
    // A hull escape owns its physical arrival, independently of the assistant's
    // ordinary 32/64 cm gun station. Stopping that far before the safe edge can
    // leave his body in the vehicle's next step even while he appears settled.
    const bool PreciseArmourYieldV84411=ArmourYieldV8218&&!FrontlineUseLegacyArmourYieldV84411()
        &&!WarfareFRRUCampaignV82::Active(Battle);
    const bool PreciseMovementGoalV84411=PreciseGunBay||PreciseArmourYieldV84411;
    float RegisteredBendArrivalRadiusV87=10.f,RegisteredBendStopDistanceV87=8.f;
    const bool PreciseRegisteredBendV87=WarfareFRRUCampaignV82::Active(Battle)
        &&WarfareFRRUWorldV82::RegisteredBendArrivalV87(this,DestinationV8218,RegisteredBendArrivalRadiusV87,RegisteredBendStopDistanceV87);
    const float ArrivalRadius=PreciseRegisteredBendV87?RegisteredBendArrivalRadiusV87:PreciseMovementGoalV84411?3.f:GroundCrewStation?32.f:HoldingSlot?(SupportedCrew?14.f:24.f):10.f;
    // A national route may return current feet while waiting for join precedence.
    // Resume precise steering after that wait. The separate formation-arrival
    // tolerance also accepts a safely planted man already firing at the post.
    const bool NationalRoute=Squad&&Squad->bAdvancing&&WarfareFRRUCampaignV82::Active(Battle);
    const float ResumeRadius=PreciseRegisteredBendV87?RegisteredBendArrivalRadiusV87:PreciseMovementGoalV84411?10.f:GroundCrewStation?64.f:HoldingSlot?(SupportedCrew?38.f:58.f):NationalRoute?10.f:18.f;
    const bool GroundStationOwnsArrivalV84411=GroundCrewStation&&SupportedCrew&&!IsMachineGunner()
        &&!AntiTankGunnerV48&&!ArmourYieldV8218&&!PioneerEgressV8445;
    // Station selection already owns this assistant's 32/64 cm arrival
    // hysteresis. A former march goal's arrival latch cannot silently seat him
    // at a newly selected, still unreached support station.
    bAtDestinationV43=GroundStationOwnsArrivalV84411?!bCrewReform
        :Distance<=(bAtDestinationV43?ResumeRadius:ArrivalRadius);
    // A planted shoulder must clear the actual next hull, not merely place
    // its destination within the normal arrival hysteresis. Finish the last
    // centimetres through the same physical Step and support checks.
    const bool ExactArmourYieldV86=PreciseArmourYieldV84411&&FrontlineArmourYieldNeedsExactArrivalV86(this,DestinationV8218);
    if(bAtDestinationV43&&ExactArmourYieldV86)bAtDestinationV43=false;
    const bool WantsMovement=!bAtDestinationV43 && (!bEngaging || bCrewReform);
#if WITH_EDITOR
    FrontlineBlockedFireProbeV87::Observe(this,Contact,StopForContact,WantsMovement,bCrewReform,bAtDestinationV43,DestinationV8218);
    const auto BlockedFireTickGateV87=[&](const TCHAR* Reason,AWarfareFrontlineSoldier* ActualTarget)
    {FrontlineBlockedFireProbeV87::TickGate(this,ActualTarget,Reason,ShotClock,AimSettleClock,FirePoseClock,DepartureClock);};
    BlockedFireTickGateV87(TEXT("before_stance_update"),FocusTarget.Get());
#endif
    const float CurrentSupport=FrontlineInfantryV45::SurfaceHeight(Battle,Feet());
    const bool AboveSupport=Feet().Z>CurrentSupport+3.f;
    const bool BelowSupport=Feet().Z<CurrentSupport-3.f;
    const auto FollowSupport=[&](FVector P,float SupportSeconds)
    {
        const float Surface=FrontlineInfantryV45::SurfaceHeight(Battle,P);
        const float Height=Feet().Z;
        if(Surface<Height-3.f)
        {
            // Leaving the inner lip is a short fall, not an instantaneous
            // metre-scale root translation. Bounded integration also handles
            // a hitch without placing the feet below the actual floor.
            float Z=Height,Remaining=SupportSeconds;
            while(Remaining>0.f)
            {
                const float Step=FMath::Min(Remaining,1.f/120.f);
                TrenchFallSpeedV69=FMath::Min(TrenchFallSpeedV69+980.f*Step,650.f);
                Z=FMath::Max(Surface,Z-TrenchFallSpeedV69*Step);Remaining-=Step;
                if(Z<=Surface){TrenchFallSpeedV69=0;break;}
            }
            P.Z=Z;
        }
        else {P.Z=Surface;TrenchFallSpeedV69=0;}
        return P+FVector(0,0,88);
    };
    const auto PlaceFollowingSupport=[&](const FVector& FootPoint,float SupportSeconds=-1.f)
    {
        const float PreviousFallSpeedV8448=TrenchFallSpeedV69;
        const FVector ActorPoint=FollowSupport(FootPoint,SupportSeconds<0.f?DT:SupportSeconds);
        // The XY resolver cannot authorize a later vertical fall through a
        // passing hull. Publish floor integration and actor translation as
        // one physically checked step; a refused placement spends neither.
        if(!FrontlineMobilityV41::AllowsSupportedPlacementV8448(this,Feet(),ActorPoint-FVector(0,0,88)))
        {TrenchFallSpeedV69=PreviousFallSpeedV8448;return;}
        if(WarfareFRRUCampaignV82::Active(Battle))
            WarfareFRRUWorldV82::PlaceOnGround(this,ActorPoint-FVector(0,0,88));
        else SetActorLocation(ActorPoint);
    };
    if(AboveSupport)AimSettleClock=FMath::Max(AimSettleClock,.18f);
    // A settling footfall is vertical support integration, not a new march
    // order. Feeding it into stance intent repeatedly packed planted crews.
    const bool WasPackingV836=TickGunV834&&TickGunV834->State==EFrontlineGunStanceV30::Pack;
    if(auto* Gun=GunForTickV834();Gun && Gun->Update(DT,WantsMovement))
    {
#if WITH_EDITOR
        BlockedFireTickGateV87(TEXT("stance_update_owns_tick"),FocusTarget.Get());
#endif
        MotionSpeed=RequestedTravelSpeedV43=0;bMoving=false;
        if(AboveSupport||BelowSupport)PlaceFollowingSupport(Feet());
        if(UAnimationAsset* A=Gun->TransitionMotion()){SetMotion(A,false);if(Node)Node->SetPlayRate(Gun->TransitionRate());}
        else if(Playing==GetWalkMotion()||Playing==GetRunMotion())SetMotion(GetAimMotion());
        return;
    }
    // Finishing the authored rise already fulfilled the departure reaction.
    // Continue into measured locomotion on this same tick, with no extra idle.
    const bool FinishedPackingV836=WasPackingV836&&TickGunV834&&TickGunV834->State==EFrontlineGunStanceV30::Carry
        &&!WarfareFRRUCampaignV82::Active(Battle);
    if (WantsMovement && !bWantedMovement) DepartureClock=!bFieldEntered?.28f+MotionPhase*.12f:WasInContact?0.f:ReactionSeconds+(IsAutomaticWeapon()?.20f:0.f);
    if(FinishedPackingV836&&WantsMovement)DepartureClock=0.f;
    bWantedMovement=WantsMovement; DepartureClock=FMath::Max(0.f,DepartureClock-DT);
    const bool Running=GetRunMotion()!=nullptr;
    const float LoadScale=Role==EFrontlineRole::Marksman&&GetFaction()==EFrontlineFaction::German?1.08f:
        Role==EFrontlineRole::MobileGun?1.06f:IsMachineGunner()?.91f:(Role==EFrontlineRole::Mills?.97f:1.f);
    // Each man adjusts pace slowly; actual translation drives stride rate below,
    // avoiding random clip speed changes that make planted feet skate.
    const float Pace=1.f+.035f*FMath::Sin(Battle->Elapsed*(.75f+MotionPhase*.3f)+MotionPhase*2.f*PI);
    float DesiredSpeed=WantsMovement && DepartureClock<=0 && FirePoseClock<=0
        ?FrontlineMovementV28::TravelSpeed(Distance,Suppression,Running,ExactArmourYieldV86?0.f:PreciseMovementGoalV84411?1.f:PreciseRegisteredBendV87?RegisteredBendStopDistanceV87:8.f)*MovementScale*LoadScale*Pace*(IsRallied()?1.2f:1.f):0.f;
    if (DesiredSpeed>0.f && Battle->CoverFor(this)<=0.f)
        DesiredSpeed*=Battle->EncounterMudMoveScale()*FrontlineTerrainTacticsV54::InfantryTravelScale(Feet());
    DesiredSpeed*=FrontlineFieldV32::MovementScale(this);
    if(LocalCrewReformV835)DesiredSpeed=FMath::Min(DesiredSpeed,160.f);
    // The unseen reserve approach has one predictable travel time. Once a man
    // crosses the screen/world boundary, normal load and actual mud apply.
    if(!bFieldEntered && WantsMovement && DepartureClock<=0 && FirePoseClock<=0)
        DesiredSpeed=FrontlinePacingV24::IngressSpeedCm;
    if(EscortTank&&Distance<350.f)
        DesiredSpeed=FMath::Min(DesiredSpeed,UFrontlineVehicleSubsystem::Definition(EscortTank->Type).SpeedCm*1.2f);
    // Continuous braking reaches the destination without a final teleport or skating past it.
    // The same live lease that owns exact arrival must also own braking.
    // Keeping the ordinary 1 cm stop distance here stranded the final guide
    // at 0.66 cm while the vehicle correctly waited for a 0.01 cm acknowledgement.
    // The remaining travel still uses the real bounded Step and support gates.
    DesiredSpeed=FMath::Min(DesiredSpeed,FMath::Sqrt(FMath::Max(0.f,2.f*2100.f*(Distance-(ExactArmourYieldV86?0.f:PreciseMovementGoalV84411?1.f:PreciseRegisteredBendV87?RegisteredBendStopDistanceV87:8.f)))));
    RequestedTravelSpeedV43=WantsMovement?FMath::FInterpConstantTo(RequestedTravelSpeedV43,DesiredSpeed,DT,DesiredSpeed>RequestedTravelSpeedV43?1450.f:2400.f):0.f;
    const FVector Before=Feet();FVector StepFacingV8447=FVector::ZeroVector;
    if (RequestedTravelSpeedV43>(ExactArmourYieldV86?.01f:1.f) && Distance>(ExactArmourYieldV86?.01f:1.f))
    {
        // A gun's assistant may need one local backward step to reach the
        // accepted support position. The squad's advance-only restriction
        // applies to marching, not to explicit reformation at its planted gun.
        // SoldierDestination above already advanced the national guide once.
        // Reading whether this is a native route must not mutate it again.
        const bool NativeRoute=WarfareFRRUCampaignV82::Active(Battle);
        const float TravelBudget=FMath::Min(Distance,RequestedTravelSpeedV43*DT);
        // Only a nearby physical support reslot or explicit hull escape may
        // release forward marching. A distant assistant's reform request is
        // still a march, and cannot take him back across the battlefield.
        const bool LocalReform=LocalCrewReformV835||ArmourYieldV8218||PioneerEgressV8445;
        const bool AdvanceOnly=Squad&&Squad->bAdvancing&&!(bCrewReform&&LocalReform)&&!EscortTank&&!NativeRoute;
        if(TravelBudget>80.f)
        {
            // The resolver's 80 cm bound belongs to each checked leg. A large
            // frame must consume ordinary and native travel alike. Weapon and
            // stance clocks above still advance only once for this actor tick.
            // Each placement refreshes the actual body/contact state.
            const int32 Substeps=FMath::CeilToInt(TravelBudget/80.f);
            float RemainingTravel=TravelBudget,RemainingSupport=DT;
            for(int32 StepIndex=0;StepIndex<Substeps&&RemainingTravel>0.f;++StepIndex)
            {
                const float StepTravel=FMath::Min(RemainingTravel,80.f);
                const float StepSeconds=DT*StepTravel/TravelBudget;
                const FVector StepBefore=Feet();
                const FVector Next=FrontlineMobilityV41::Step(this,Before+Delta,StepTravel,AdvanceOnly,StepSeconds,&StepFacingV8447);
                PlaceFollowingSupport(Next,StepSeconds);
                RemainingTravel-=StepTravel;RemainingSupport=FMath::Max(0.f,RemainingSupport-StepSeconds);
                // A rejected/blocked leg cannot earn another movement attempt.
                if(FVector::DistSquared2D(StepBefore,Feet())<=.000001f)
                {
                    if(RemainingSupport>0.f&&AboveSupport)PlaceFollowingSupport(Feet(),RemainingSupport);
                    break;
                }
            }
        }
        else
        {
            const FVector Next=FrontlineMobilityV41::Step(this,Before+Delta,TravelBudget,AdvanceOnly,DT,&StepFacingV8447);
            PlaceFollowingSupport(Next);
        }
    }
    else if(AboveSupport||BelowSupport)
    {
        PlaceFollowingSupport(Feet());
    }
    // Read accepted actor travel even when no step was possible. Previously
    // arrival skipped this assignment, leaving a residual run loop at rest.
    const float Travel=FVector::Dist2D(Before,Feet());DistanceTravelled+=Travel;
    if(NationalRoute)
    {
        const bool Turning= !StepFacingV8447.IsNearlyZero()&&FVector::DotProduct(StepFacingV8447,GetActorForwardVector())<.5f;
        const float Attempt=WantsMovement&&RequestedTravelSpeedV43>1.f&&Distance>1.f&&!Turning?DT:0.f;
        WarfareFRRUWorldV82::RecordMovementV87(this,Before,Feet(),DestinationV8218,Attempt);
    }
    MotionSpeed=Travel/DT;
    bMoving=MotionSpeed>8;
    if(!bMoving&&!StepFacingV8447.IsNearlyZero()&&FVector::DotProduct(StepFacingV8447,GetActorForwardVector())<.5f)
    {
        // This exact tangent has already passed the physical bank/body gates.
        // Keep the feet planted while turning, then resume its ordinary step.
        FRotator Facing=GetActorRotation();Facing.Yaw=FMath::FixedTurn(Facing.Yaw,StepFacingV8447.Rotation().Yaw,360.f*DT);
        SetActorRotation(Facing);
        // A physically checked UK/DE leg retains its planning pace while the real
        // feet are planted. Resetting that budget alternated a short forward
        // candidate with the required outward body tangent each frame, so
        // their opposing headings cancelled the turn indefinitely. Actual
        // speed remains zero and the next step still passes every body/wall/
        // hull gate and this same facing check before translation.
        if(WarfareFRRUCampaignV82::Active(Battle))RequestedTravelSpeedV43=0;
        AimSettleClock=FMath::Max(AimSettleClock,.2f);
#if WITH_EDITOR
        BlockedFireTickGateV87(TEXT("physical_step_turn_owns_tick"),FocusTarget.Get());
#endif
        SetMotion(GetAimMotion());return;
    }
    if (bMoving)
    {
        FrontlineMotionV41::CancelWaiting(this);
        if (FirstMovementAt<0) FirstMovementAt=Battle->Elapsed;
        if (IsStormtrooper() && !bStormAdvanceCalled && Squad && Squad->bAdvancing && !Squad->bReinforcement
            && Battle->Trenches.IsValidIndex(Squad->LastPassedTrench)
            && (Feet().X-Battle->Trenches[Squad->LastPassedTrench].X)*Battle->GetTeamDirection(Team)>180.f)
        {
            bStormAdvanceCalled=true;
            if (auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
                Audio->PlayVoice(EFrontlineVoiceEvent::StormAdvance,true,Feet(),SquadId);
        }
        bWasSettled=false; AimSettleClock=.20f+ReactionSeconds*.5f;
        // Every stride faces its accepted translation. Covering assistants
        // resume watching the gun/frontage once planted; preserving that watch
        // during a rearward station adjustment caused a forward-loop backslide.
        const FVector TravelDirection=Feet()-Before;
        FRotator TravelFacing=FVector(TravelDirection.X,TravelDirection.Y,0).Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),TravelFacing,DT,6.8f+MotionPhase*2.4f));
        const bool RunClip=!LocalCrewReformV835&&GetRunMotion() && (Playing==GetRunMotion()?MotionSpeed>235:MotionSpeed>280);
        SetMotion(RunClip?GetRunMotion():GetWalkMotion());
        if (Node)
        {
            Node->SetPlayRate(FMath::Clamp(MotionSpeed/(RunClip?390.f:210.f),.08f,1.45f));
        }
        if(GrenadeClock>0.f)GrenadeClock-=DT*FrontlineWeapons::GrenadierPreparationRateBoostV835(this,Contact,
            Contact&&FVector::Dist2D(Feet(),Contact->Feet())>110.f);
#if WITH_EDITOR
        BlockedFireTickGateV87(TEXT("actual_locomotion_owns_tick"),FocusTarget.Get());
#endif
        return;
    }
    MotionSpeed=0;
    // Blocked or finished travel cannot retain an advancing clip behind an
    // action timer. Actual fire/reload/deploy assets remain uninterrupted.
    if(Playing==GetWalkMotion()||Playing==GetRunMotion())SetMotion(GetAimMotion());
    AWarfareFrontlineSoldier* Target=FocusTarget.Get();
    // Review cadence may retain a hostile contact, never an ally, casualty or
    // another encounter's actor. A genuine visible rear threat stays eligible.
    if(Target&&(Target->Team==Team||Target->Battle!=Battle||!Target->IsActiveCombatant()))
    {Target=nullptr;FocusTarget.Reset();TargetReviewClock=0;}
    if (!Target || !Target->IsAlive() || TargetReviewClock<=0)
    {
        AWarfareFrontlineSoldier* Candidate=Battle->AcquireTarget(this);
        // Keep a valid aim point across small score changes instead of whipping between men.
        const float Range=FrontlineWeapons::ProfileFor(this).FireRangeCm*Battle->EncounterVisibilityScale();
        if (!Target || !Target->IsAlive() || FrontlineWeapons::DirectFireRangeDistanceCm(this,Target->Feet())>Range || !Battle->HasSight(this,Target)) Target=Candidate;
        else if(IsScopedMarksman() && Battle->HasFactionTechnology(EFrontlineFaction::British,13,Team)
            && Candidate && Candidate->IsSectionLeader() && !Target->IsSectionLeader()) Target=Candidate;
        else if(Role==EFrontlineRole::Marksman && MemberIndex==0 && Candidate && Candidate->IsMachineGunner() && !Target->IsMachineGunner()
            && !(Target->IsSectionLeader() && IsScopedMarksman() && Battle->HasFactionTechnology(EFrontlineFaction::British,13,Team)))Target=Candidate;
        else if(FrontlineChoiceTargetsV78::ShouldRetarget(Battle,this,Candidate,Target))Target=Candidate;
        else if(FrontlineMachinegunsV30::ShouldRetargetV828(this,Candidate,Target))
        {
            const auto* Support=GunForTickV834();
            if(BurstRemaining<=0||(Support&&Support->IsSupported()
                &&(!Support->HasInfantryFiringLine(Target)||!Support->CoversYaw((Target->Feet()-Feet()).Rotation().Yaw))))
            {Target=Candidate;BurstRemaining=0;}
        }
        if (Target!=FocusTarget.Get()) AimSettleClock=(Role==EFrontlineRole::Marksman&&MemberIndex==0&&GetFaction()==EFrontlineFaction::British?1.15f:.20f)+ReactionSeconds*.5f;
        FocusTarget=Target; TargetReviewClock=.15f+ReactionSeconds*.25f;
    }
    // A rifle-grenade section must not spend its one loaded bomb on a rifle
    // screen while holding outside the obsolete 22 m armour preference gate.
    // Its ordinary acquisition, actual ballistic clearance and range still
    // decide whether this observed vehicle can be engaged.
    if(VehicleFireContact&&(FrontlineSpecialistsV41::IsAntiTankGunner(this)
        ||(FrontlineSpecialistsV41::IsRifleGrenadier(this)
            &&(!WarfareFRRUCampaignV82::Active(Battle)||FVector::Dist2D(Feet(),VehicleContact->AimPoint())<2200.f))))Target=nullptr;
    if (!bWasSettled)
    {
        bWasSettled=true; AimSettleClock=FMath::Max(AimSettleClock,(Role==EFrontlineRole::Marksman&&MemberIndex==0&&GetFaction()==EFrontlineFaction::British?1.15f:.20f)+ReactionSeconds*.5f+(IsAutomaticWeapon()?.25f:0.f));
        ShotClock=FMath::Max(ShotClock,Battle->CoverFor(this)>.4f?MotionRandom.FRandRange(.5f,3.5f):MotionRandom.FRandRange(.22f,.65f));
    }
    AWarfareFrontlineSoldier* GrenadeTarget=Target&&FVector::Dist2D(Feet(),Target->Feet())<=FrontlineGrenade::MaxRangeCm?Target:
        Contact&&FVector::Dist2D(Feet(),Contact->Feet())<=FrontlineGrenade::MaxRangeCm?Contact:nullptr;
    FVector ArmourGrenadePoint=FVector::ZeroVector;
    AFrontlineTank* ArmourGrenadeTarget=Role==EFrontlineRole::Mills&&VehicleSystem&&!bMoving
        &&FirePoseClock<=0&&AimSettleClock<=0&&WeaponPhase==EFrontlineWeaponPhase::Ready
        ?VehicleSystem->AcquireGrenadeTarget(this,FrontlineGrenade::MaxRangeCm,ArmourGrenadePoint):nullptr;
    const bool ArmourGrenadePriority=ArmourGrenadeTarget&&!WarfareFRRUCampaignV82::Active(Battle)
        &&(!GrenadeTarget||FVector::Dist2D(Feet(),GrenadeTarget->Feet())>650.f);
    if(ArmourGrenadeTarget&&WarfareFRRUCampaignV82::Active(Battle)
        &&(!GrenadeTarget||FVector::Dist2D(Feet(),ArmourGrenadePoint)<FVector::Dist2D(Feet(),GrenadeTarget->Feet())))GrenadeTarget=nullptr;
    if(ArmourGrenadePriority)
    {
        // The globally reduced willingness remains for infantry throws. An
        // observed tank already within hand-bomb range instead calls for an
        // urgent preparation, with separate crew reaction times and the normal
        // animated release. It does not make open frontal approaches safe.
        if(ArmourGrenadeReadyAtV833<0.f)ArmourGrenadeReadyAtV833=Battle->Elapsed+.65f+ReactionSeconds;
        GrenadeTarget=nullptr;
    }
    else if(WeaponPhase==EFrontlineWeaponPhase::Ready&&FirePoseClock<=0&&AimSettleClock<=0)
        ArmourGrenadeReadyAtV833=-1.f;
    const float CoveredBombPreparationV835=!ArmourGrenadePriority?FrontlineWeapons::GrenadierPreparationRateBoostV835(this,GrenadeTarget,
        GrenadeTarget&&GrenadeTarget==Contact&&FVector::Dist2D(Feet(),GrenadeTarget->Feet())>110.f):0.f;
    if(GrenadeClock>0.f)GrenadeClock-=DT*CoveredBombPreparationV835;
    // A grenade uses the same personal preparation and sight acquisition as a
    // rifle shot. Bypassing these clocks made arrivals throw almost in unison.
    const bool GrenadePrepared=ArmourGrenadePriority?Battle->Elapsed>=ArmourGrenadeReadyAtV833:GrenadeClock<=0;
    if (Role==EFrontlineRole::Mills && (GrenadeTarget || ArmourGrenadeTarget) && GrenadePrepared && FirePoseClock<=0 && (ShotClock<=0||CoveredBombPreparationV835>0.f) && AimSettleClock<=0
        && WeaponPhase==EFrontlineWeaponPhase::Ready && Battle->GrenadeAnimation
        && FVector::Dist2D(Feet(),GrenadeTarget?GrenadeTarget->Feet():ArmourGrenadePoint)<=FrontlineGrenade::MaxRangeCm)
    {
        const int32 BeforeThrow=GrenadeThrowsStarted;
        StartGrenadeThrow(GrenadeTarget,ArmourGrenadeTarget);
        if(GrenadeThrowsStarted>BeforeThrow)
        {
            GrenadeClock=MotionRandom.FRandRange(8.6f,11.3f);
            ArmourGrenadeReadyAtV833=ArmourGrenadePriority?Battle->Elapsed+MotionRandom.FRandRange(5.8f,7.4f):-1.f;
        }
    }
    UAnimationAsset* Waiting=FrontlineMotionV41::WaitingMotion(this,DT,!Target&&!VehicleContact&&FirePoseClock<=0&&BurstRemaining<=0);
    if (FirePoseClock<=0 && !(IsAutomaticWeapon() && BurstRemaining>0 && Target)) SetMotion(Target||VehicleContact||(Squad&&Squad->bAdvancing&&!Battle->Trenches.IsValidIndex(Squad->Destination))?GetAimMotion():Waiting?Waiting:GetIdleMotion(),Waiting==nullptr);
#if WITH_EDITOR
    BlockedFireTickGateV87(!Target?TEXT("no_actual_infantry_fire_target"):ShotClock>0?TEXT("shot_clock_pending"):AimSettleClock>0?TEXT("aim_settle_pending"):FirePoseClock>0?TEXT("fire_pose_pending"):TEXT("calling_actual_discharge"),Target);
#endif
    if (Target)
    {
        FVector Aim=Target->Feet()-Feet(); Aim.Z=0;
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),Aim.Rotation(),DT,5.6f+MotionPhase*2.8f));
        if (ShotClock<=0 && AimSettleClock<=0 && FirePoseClock<=0)
        {
            DischargeWeapon(Target);
        }
    }
    else if(VehicleContact)
    {
        FVector Heading=VehicleContact->AimPoint()-Feet();Heading.Z=0;
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),Heading.Rotation(),DT,5.6f));
        if(VehicleFireContact==VehicleContact && ShotClock<=0 && AimSettleClock<=0 && FirePoseClock<=0)DischargeWeapon(nullptr,VehicleContact);
    }
    else if(FirePoseClock<=0||SupportedCrew)
    {
        // Slot corrections can end with a lateral or rearward heading. Once
        // settled, watch the enemy frontage rather than retaining that last step.
        const auto* Support=GunForTickV834();
        const FRotator Watch(0,Support&&Support->IsSupported()?Support->FormationYawV50():Battle->GetTeamDirection(Team)>0?0:180,0);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),Watch,DT,5.6f+MotionPhase*2.8f));
    }
}

