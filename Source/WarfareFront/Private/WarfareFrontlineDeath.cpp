#include "WarfareFrontlineDeath.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineFieldKitV31.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFrontlineMotionV40.h"
#include "WarfareFrontlineMotionV41.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFrontlineAudio.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AttributesRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Misc/MemStack.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool DeathSleepReviewRequested()
{
    static const bool Requested=[]
    {
        FString UserDir;
        return FParse::Param(FCommandLine::Get(),TEXT("WFDeathSleepReview"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)
            &&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
    }();
    return Requested;
}

bool OldDeathAudits(const AWarfareFrontlineGameMode* Battle)
{
    static const bool Requested=DeathSleepReviewRequested()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFDeathOldAudits"));
    return Requested&&Battle&&Battle->IsProgressionVolatile();
}

bool SleepingDeathAuditReady(const AWarfareFrontlineSoldier* Unit)
{
    if(!Unit||!Unit->Soldier||WarfareFRRUCampaignV82::Active(Unit->Battle))return false;
    const auto& D=Unit->GetDeathState();
    if(!D.bPhysicsStarted||D.Age<=1.f||D.PublishedPoseSamples<1||D.PhysicalPoseSamples<1
        ||!D.bSeverAttempted||D.bNonFinite||Unit->Soldier->Bodies.IsEmpty())return false;
    for(const FBodyInstance* Body:Unit->Soldier->Bodies)
    {
        if(!Body||!Body->IsValidBodyInstance()||!Body->IsInstanceSimulatingPhysics()||Body->IsInstanceAwake())return false;
        // The actor's two-second damping transition is functional work. A
        // naturally sleeping but not yet settled body must receive it first.
        if(!D.bSettled&&(Body->LinearDamping!=.9f||Body->AngularDamping!=6.f))return false;
    }
    return true;
}

struct FDeathSleepProbe
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;
    int32 ActorAudits=0,ActorSkips=0,PublishedAudits=0,PublishedSkips=0;
    int32 Stage=0,FramesAtSleep=0,ActorAtSleep=0,PublishedAtSleep=0,ActorAtWake=0,PublishedAtWake=0;
    float SleepAt=0,AgeAtSleep=0,WakeAt=0,MaximumSleepDriftCm=0,MaximumSleepAngleDegrees=0,MaximumSleepVisualErrorCm=0,FirstPoseError=-1;
    bool WakeObserved=false,Passed=false;
    TArray<FTransform> SleepingBodies;
};
TArray<FDeathSleepProbe>* DeathSleepProbes=nullptr; // Only the isolated native fixture owns this pointer.
void CountDeathAudit(AWarfareFrontlineSoldier* Unit,bool Published,bool Skipped)
{
    if(!DeathSleepProbes)return;
    for(auto& P:*DeathSleepProbes)if(P.Unit.Get()==Unit)
    {
        if(Published){if(Skipped)++P.PublishedSkips;else ++P.PublishedAudits;}
        else {if(Skipped)++P.ActorSkips;else ++P.ActorAudits;}
        break;
    }
}

void DeathReviewMotion(AWarfareFrontlineSoldier* Unit,UAnimationAsset* Motion,bool Loop,float Rate)
{
    if(!Unit||!Unit->Soldier||!Motion)return;
    // Use the existing public animation instance API. Do not replace its
    // approved pose proxy or expose the soldier's private gameplay state.
    if(auto* Node=Cast<UWarfareFrontlineAnimation>(Unit->Soldier->GetAnimInstance()))
        if(Node->GetAnimationAsset()!=Motion)Node->TransitionTo(Motion,Loop,Rate,0.f,.10f);
}

// Explicit native fixture. Actor/component/Chaos ticks remain live; it never
// forces sleep or changes body shapes, collision, damping or settlement.
void StartDeathSleepReview(UWorld* World)
{
    auto* B=World?World->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;
    if(!DeathSleepReviewRequested()||!B||!B->IsProgressionVolatile()||DeathSleepProbes)return;
    struct FRun
    {
        TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
        TArray<FDeathSleepProbe> Probes;
        float Started=0;
        double WallStarted=FPlatformTime::Seconds();
        bool Baseline=false;
    };
    auto R=MakeShared<FRun>();R->Battle=B;R->Baseline=OldDeathAudits(B);
    B->AbandonBattleToOperations();
    if(!B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Rookie))return;
    B->BeginBattle();B->SetPaused(false);B->SetActorTickEnabled(false);
    if(B->Phase!=EFrontlinePhase::Fighting||!B->AssetFailure.IsEmpty())return;
    if(auto* PC=World->GetFirstPlayerController()){PC->SetActorTickEnabled(false);PC->DisableInput(PC);}
    R->Started=World->GetTimeSeconds();
    for(int32 I=0;I<2;++I)
    {
        auto* U=World->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return;
        // MG identity is eligible for the existing helmet release and heavy
        // loss appearance; rifle uses the ordinary articulated blast path.
        U->Initialize(B,I?841317:841318,0,I?EFrontlineRole::Lewis:EFrontlineRole::Rifle,0);
        FVector P(B->Trenches[B->GetHomeTrenchIndex(0)].X+700.f,I?-600.f:600.f,0);
        P.Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
        U->SetActorLocation(P+FVector(0,0,88));U->SetActorRotation(FRotator::ZeroRotator);
        U->bFieldEntered=true;U->SetActorTickEnabled(false);U->ReturnToIdle();U->bEngaging=true;
        B->Units.Add(U);FDeathSleepProbe PRow;PRow.Unit=U;R->Probes.Add(MoveTemp(PRow));
    }
    DeathSleepProbes=&R->Probes;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float)
    {
        auto* Battle=R->Battle.Get();auto* W=Battle?Battle->GetWorld():nullptr;
        if(!W){DeathSleepProbes=nullptr;return false;}
        const float Now=W->GetTimeSeconds(),DT=W->GetDeltaSeconds();
        bool Finished=true;
        for(int32 I=0;I<R->Probes.Num();++I)
        {
            auto& P=R->Probes[I];auto* U=P.Unit.Get();if(!U){P.Stage=4;continue;}
            const auto& D=U->GetDeathState();
            if(P.Stage==0)
            {
                bool Ready=Now-R->Started>3.5f;
                if(I==1)
                {
                    auto* Gun=FrontlineMachinegunsV30::Get(U);
                    if(!Gun){P.Stage=4;continue;}
                    Gun->Update(DT,false);
                    if(auto* Motion=Gun->TransitionMotion())DeathReviewMotion(U,Motion,false,Gun->TransitionRate());
                    if(Gun->IsSupported())DeathReviewMotion(U,Gun->Action(TEXT("Aim")),true,1.f);
                    Ready&=Gun->IsSupported()&&Gun->IsProne();
                }
                if(Ready)
                {
                    U->ReceiveHit(1.f,0.f,1,FVector(1,0,0),I?EFrontlineDamageKind::HeavyBullet:EFrontlineDamageKind::Artillery,.7f);
                    U->SetActorTickEnabled(true);P.Stage=1;
                }
            }
            else if(P.Stage==1&&SleepingDeathAuditReady(U))
            {
                P.Stage=2;P.SleepAt=Now;P.AgeAtSleep=D.Age;P.FramesAtSleep=D.Frames;
                P.ActorAtSleep=P.ActorAudits;P.PublishedAtSleep=P.PublishedAudits;P.FirstPoseError=D.FirstPublishedPoseErrorCm;
                P.SleepingBodies.Reset();for(const auto* Body:U->Soldier->Bodies)P.SleepingBodies.Add(Body->GetUnrealWorldTransform());
            }
            else if(P.Stage==2)
            {
                if(!SleepingDeathAuditReady(U)){P.Stage=1;continue;}
                for(int32 BodyIndex=0;BodyIndex<U->Soldier->Bodies.Num();++BodyIndex)
                {
                    const FTransform At=U->Soldier->Bodies[BodyIndex]->GetUnrealWorldTransform();
                    P.MaximumSleepDriftCm=FMath::Max(P.MaximumSleepDriftCm,float(FVector::Dist(P.SleepingBodies[BodyIndex].GetLocation(),At.GetLocation())));
                    P.MaximumSleepAngleDegrees=FMath::Max(P.MaximumSleepAngleDegrees,float(FMath::RadiansToDegrees(P.SleepingBodies[BodyIndex].GetRotation().AngularDistance(At.GetRotation()))));
                }
                // Independent fixture readback must see the live published
                // skeleton even while the production measurement is skipped.
                for(const auto& Binding:D.VisualBindings)if(const auto* Body=U->Soldier->GetBodyInstance(Binding.PhysicsBody))
                    P.MaximumSleepVisualErrorCm=FMath::Max(P.MaximumSleepVisualErrorCm,float(FVector::Dist(
                        Body->GetUnrealWorldTransform().TransformPosition(Binding.BodyLocalPoint),U->Soldier->GetBoneLocation(Binding.Bone))));
                if(Now-P.SleepAt>.35f)
                {
                    const auto* Kit=U->FindComponentByClass<UFrontlineFieldKitV31>();
                    const bool Audits=R->Baseline?(P.PublishedAudits>P.PublishedAtSleep&&P.PublishedSkips==0&&P.ActorSkips==0)
                        :(P.PublishedSkips>=2&&(I==0||P.ActorSkips>=2));
                    P.Passed=Audits&&D.PublishedPoseSamples>0&&P.FirstPoseError>=0&&P.FirstPoseError<1.f
                        &&D.FirstPublishedTickGroup>=int32(TG_EndPhysics)&&D.bSeverAttempted
                        &&Kit&&Kit->HasDetachedHelmet()&&!D.bNonFinite&&D.Age>P.AgeAtSleep&&D.Frames>P.FramesAtSleep
                        &&P.MaximumSleepDriftCm<.1f&&P.MaximumSleepAngleDegrees<.1f&&P.MaximumSleepVisualErrorCm<1.f
                        &&!U->Soldier->IsVisible()&&(I==0||D.bProneAtHit);
                    P.ActorAtWake=P.ActorAudits;P.PublishedAtWake=P.PublishedAudits;P.WakeAt=Now;
                    U->Soldier->AddImpulse(FVector(0,180,45),NAME_None,true);
                    P.WakeObserved=U->Soldier->IsAnyRigidBodyAwake();P.Stage=3;
                }
            }
            else if(P.Stage==3&&Now-P.WakeAt>.15f)
            {
                P.Passed&=P.WakeObserved&&P.ActorAudits>P.ActorAtWake&&P.PublishedAudits>P.PublishedAtWake
                    &&D.FirstPublishedPoseErrorCm==P.FirstPoseError&&D.MaximumPublishedPoseErrorCm<1.f
                    &&D.bSeverAttempted&&!D.bNonFinite&&U->Soldier->IsSimulatingPhysics();
                P.Stage=4;
            }
            Finished&=P.Stage==4;
        }
        const bool TimedOut=Now-R->Started>20.f||FPlatformTime::Seconds()-R->WallStarted>60.;
        if(!Finished&&!TimedOut)return true;
        auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;bool Passed=Finished&&!TimedOut&&R->Probes.Num()==2;
        for(int32 I=0;I<R->Probes.Num();++I)
        {
            const auto& P=R->Probes[I];auto J=MakeShared<FJsonObject>();Passed&=P.Passed;
            J->SetStringField(TEXT("case"),I?TEXT("naturally_deployed_prone_mg_heavy_hit"):TEXT("standing_rifle_artillery_hit"));
            J->SetBoolField(TEXT("passed"),P.Passed);J->SetNumberField(TEXT("stage"),P.Stage);
            J->SetNumberField(TEXT("actor_audits"),P.ActorAudits);J->SetNumberField(TEXT("actor_skips"),P.ActorSkips);
            J->SetNumberField(TEXT("published_audits"),P.PublishedAudits);J->SetNumberField(TEXT("published_skips"),P.PublishedSkips);
            J->SetNumberField(TEXT("first_published_pose_error_cm"),P.FirstPoseError);J->SetNumberField(TEXT("maximum_sleep_body_drift_cm"),P.MaximumSleepDriftCm);
            J->SetNumberField(TEXT("maximum_sleep_body_rotation_degrees"),P.MaximumSleepAngleDegrees);J->SetNumberField(TEXT("maximum_sleep_published_visual_error_cm"),P.MaximumSleepVisualErrorCm);
            J->SetBoolField(TEXT("impulse_woke_bodies"),P.WakeObserved);
            J->SetNumberField(TEXT("actor_audits_after_wake"),P.ActorAudits-P.ActorAtWake);J->SetNumberField(TEXT("published_audits_after_wake"),P.PublishedAudits-P.PublishedAtWake);
            if(const auto* U=P.Unit.Get())
            {
                const auto& D=U->GetDeathState();J->SetBoolField(TEXT("prone_at_hit"),D.bProneAtHit);
                J->SetBoolField(TEXT("settled"),D.bSettled);J->SetBoolField(TEXT("cosmetic_attempted"),D.bSeverAttempted);
                J->SetNumberField(TEXT("death_age_seconds"),D.Age);J->SetNumberField(TEXT("maximum_published_pose_error_cm"),D.MaximumPublishedPoseErrorCm);
            }
            Rows.Add(MakeShared<FJsonValueObject>(J));
        }
        Report->SetBoolField(TEXT("success"),Passed);Report->SetBoolField(TEXT("timed_out"),TimedOut);Report->SetBoolField(TEXT("old_audit_baseline"),R->Baseline);
        Report->SetStringField(TEXT("scope"),TEXT("Isolated labelled UK native fixture: real standing/prone meshes, normal fatal hits and live Chaos; natural sleep, bounded pose stability, first published pose/cosmetics and physical impulse wake. No forced sleep, settlement, physics changes or performance claim."));
        Report->SetArrayField(TEXT("cases"),Rows);FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V8213/DeathSleepReview");IFileManager::Get().MakeDirectory(*Directory,true);
        FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("review.json")));DeathSleepProbes=nullptr;
        FPlatformMisc::RequestExit(false);return false;
    }));
}
FAutoConsoleCommandWithWorldAndArgs DeathSleepReviewCommand(TEXT("wf.Death.SleepReview"),
    TEXT("Isolated WFDeathSleepReview + WFRenderNoSave + absolute UserDir native sleep/wake fixture."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&,UWorld* World){StartDeathSleepReview(World);}));

struct FDeathBodySpec
{
    const TCHAR* Bone;
    const TCHAR* End;
    const TCHAR* Parent;
    float RadiusCm,MassKg,Swing1,Swing2,Twist;
};

// Simple major-body shapes exclude fingers, weapon and small helper bones. Dimensions
// are in centimetres; inverse evaluated bone scale is applied below. FBX reference
// transforms and the playing animation can carry different root-unit scales.
constexpr FDeathBodySpec DeathBodies[]=
{
    {TEXT("pelvis"),TEXT("spine_01"),nullptr,10.f,14.f,0,0,0},
    {TEXT("spine_01"),TEXT("spine_03"),TEXT("pelvis"),9.f,9.f,22,20,18},
    {TEXT("spine_03"),TEXT("neck_01"),TEXT("spine_01"),11.f,22.f,25,22,20},
    {TEXT("Head"),nullptr,TEXT("spine_03"),8.5f,5.f,30,25,35},
    {TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("spine_03"),4.2f,2.5f,75,60,40},
    {TEXT("lowerarm_l"),TEXT("hand_l"),TEXT("upperarm_l"),3.5f,1.6f,75,10,12},
    {TEXT("hand_l"),TEXT("middle_01_l"),TEXT("lowerarm_l"),3.f,.5f,18,12,18},
    {TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("spine_03"),4.2f,2.5f,75,60,40},
    {TEXT("lowerarm_r"),TEXT("hand_r"),TEXT("upperarm_r"),3.5f,1.6f,75,10,12},
    {TEXT("hand_r"),TEXT("middle_01_r"),TEXT("lowerarm_r"),3.f,.5f,18,12,18},
    {TEXT("thigh_l"),TEXT("calf_l"),TEXT("pelvis"),6.f,7.5f,55,35,25},
    {TEXT("calf_l"),TEXT("foot_l"),TEXT("thigh_l"),4.5f,3.5f,75,8,8},
    {TEXT("foot_l"),TEXT("ball_l"),TEXT("calf_l"),3.8f,1.f,22,12,12},
    {TEXT("thigh_r"),TEXT("calf_r"),TEXT("pelvis"),6.f,7.5f,55,35,25},
    {TEXT("calf_r"),TEXT("foot_r"),TEXT("thigh_r"),4.5f,3.5f,75,8,8},
    {TEXT("foot_r"),TEXT("ball_r"),TEXT("calf_r"),3.8f,1.f,22,12,12}
};

bool IsMergedExtremity(const FDeathBodySpec& Spec)
{
    return FCString::Strncmp(Spec.Bone,TEXT("hand_"),5)==0 || FCString::Strncmp(Spec.Bone,TEXT("foot_"),5)==0;
}

float SphereTerrainClearance(const FVector& Center,float Radius)
{
    float Clearance=float(Center.Z-Radius-AWarfareFrontlineGround::HeightAt(Center.X,Center.Y));
    // Sample the lower hemisphere as well as the vertical support point. At a
    // trench lip the contact can be on the uphill side of a capsule, not beneath it.
    constexpr float Ring=.70710678f;
    for (int32 DirectionIndex=0;DirectionIndex<8;++DirectionIndex)
    {
        const float Angle=DirectionIndex*PI*.25f;
        const FVector P=Center+FVector(FMath::Cos(Angle)*Radius*Ring,FMath::Sin(Angle)*Radius*Ring,-Radius*Ring);
        Clearance=FMath::Min(Clearance,float(P.Z-AWarfareFrontlineGround::HeightAt(P.X,P.Y)));
    }
    return Clearance;
}

float BodyTerrainClearance(const FBodyInstance* Body)
{
    float Clearance=MAX_flt;
    FTransform ShapeWorld=Body->GetUnrealWorldTransform(); ShapeWorld.SetScale3D(Body->Scale3D);
    if (const UBodySetup* Setup=Body->GetBodySetup())
    {
        for (const FKSphylElem& Shape:Setup->AggGeom.SphylElems)
        {
            const FVector Axis=Shape.Rotation.Quaternion().GetAxisZ()*(Shape.Length*.5f);
            const float Radius=Shape.Radius*Body->Scale3D.GetAbsMax();
            for (int32 SampleIndex=-1;SampleIndex<=1;++SampleIndex)
                Clearance=FMath::Min(Clearance,SphereTerrainClearance(ShapeWorld.TransformPosition(Shape.Center+Axis*SampleIndex),Radius));
        }
        for (const FKSphereElem& Shape:Setup->AggGeom.SphereElems)
            Clearance=FMath::Min(Clearance,SphereTerrainClearance(ShapeWorld.TransformPosition(Shape.Center),Shape.Radius*Body->Scale3D.GetAbsMax()));
    }
    return Clearance;
}

float PosedTerrainClearance(const TMap<FName,FVector>& Points,const FVector& Pivot,const FQuat& Rotation)
{
    const auto Point=[&](const TCHAR* Name) { return Pivot+Rotation.RotateVector(Points.FindChecked(FName(Name))-Pivot); };
    float Clearance=MAX_flt;
    for (const FDeathBodySpec& Spec:DeathBodies)
    {
        const FVector Start=Point(Spec.Bone);
        if (Spec.End)
        {
            const FVector End=Point(Spec.End),Span=End-Start,Center=(Start+End)*.5;
            const FVector HalfAxis=Span.GetSafeNormal()*FMath::Max(.1f,float(Span.Size())*.5f-Spec.RadiusCm);
            for (int32 SampleIndex=-1;SampleIndex<=1;++SampleIndex)
                Clearance=FMath::Min(Clearance,SphereTerrainClearance(Center+HalfAxis*SampleIndex,Spec.RadiusCm));
        }
        else Clearance=FMath::Min(Clearance,SphereTerrainClearance(Start+(Start-Point(TEXT("neck_01"))).GetSafeNormal()*7.f,Spec.RadiusCm));
    }
    return Clearance;
}

bool PrepareDeathTerrainFit(USkeletalMeshComponent* Mesh,UAnimationAsset* Animation,FFrontlineDeathState& State)
{
    UAnimSequence* Sequence=Cast<UAnimSequence>(Animation);
    USkeletalMesh* MeshAsset=Mesh->GetSkeletalMeshAsset();
    if (!Sequence || !MeshAsset || Sequence->IsValidAdditive()) return false;
    // Sample through the same full-pose path as SingleNode: per-track decompression
    // alone misses mesh retargeting/root-lock rules and can reintroduce unit errors.
    FMemMark Mark(FMemStack::Get());
    const FReferenceSkeleton& Ref=MeshAsset->GetRefSkeleton();
    TArray<FBoneIndexType> Indices;
    for (int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex) Indices.Add(FBoneIndexType(BoneIndex));
    FBoneContainer Bones(Indices,UE::Anim::FCurveFilterSettings(),*MeshAsset);
    FCompactPose Pose; Pose.SetBoneContainer(&Bones); Pose.ResetToRefPose();
    FBlendedCurve Curve; Curve.InitFrom(Bones);
    UE::Anim::FStackAttributeContainer Attributes;
    FAnimationPoseData Data(Pose,Curve,Attributes);
    Sequence->GetAnimationPose(Data,FAnimExtractContext(double(Sequence->GetPlayLength()),false));
    TArray<FTransform> ComponentPose; ComponentPose.SetNum(Ref.GetNum());
    TMap<FName,FVector> Points;
    State.AuthoredBaseTransform=Mesh->GetComponentTransform();
    for (int32 BoneIndex=0;BoneIndex<Ref.GetNum();++BoneIndex)
    {
        const FCompactPoseBoneIndex CompactIndex=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneIndex));
        const FTransform Local=Pose[CompactIndex]; const int32 Parent=Ref.GetParentIndex(BoneIndex);
        ComponentPose[BoneIndex]=Parent==INDEX_NONE?Local:Local*ComponentPose[Parent];
        Points.Add(Ref.GetBoneName(BoneIndex),(ComponentPose[BoneIndex]*State.AuthoredBaseTransform).GetLocation());
    }
    for (const FDeathBodySpec& Spec:DeathBodies)
        if (!Points.Contains(FName(Spec.Bone)) || (Spec.End && !Points.Contains(FName(Spec.End)))) return false;
    if (!Points.Contains(TEXT("neck_01"))) return false;
    State.TerrainPivot=(Mesh->GetBoneLocation(TEXT("foot_l"))+Mesh->GetBoneLocation(TEXT("foot_r")))*.5;
    State.TerrainPivot.Z=AWarfareFrontlineGround::HeightAt(State.TerrainPivot.X,State.TerrainPivot.Y);
    State.PredictedUnfittedClearanceCm=PosedTerrainClearance(Points,State.TerrainPivot,FQuat::Identity);
    const FVector Feet=(Points.FindChecked(TEXT("foot_l"))+Points.FindChecked(TEXT("foot_r")))*.5;
    const FVector Head=Points.FindChecked(TEXT("Head"));
    const FVector Along=(Head-Feet).GetSafeNormal2D(),Across=FVector::CrossProduct(FVector::UpVector,Along);
    const FVector Center=(Feet+Head)*.5;
    const auto Height=[](const FVector& P) { return AWarfareFrontlineGround::HeightAt(P.X,P.Y); };
    const float AlongSlope=(Height(Head)-Height(Feet))/FMath::Max(40.f,float(FVector::Dist2D(Head,Feet)));
    const float AcrossSlope=(Height(Center+Across*35.f)-Height(Center-Across*35.f))/70.f;
    const FVector Normal=(FVector::UpVector-Along*AlongSlope-Across*AcrossSlope).GetSafeNormal();
    FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Normal);
    const float Angle=float(Rotation.GetAngle());
    const float MaximumAngle=FMath::DegreesToRadians(50.f);
    if (Angle>MaximumAngle) Rotation=FQuat::Slerp(FQuat::Identity,Rotation,MaximumAngle/Angle);
    State.TerrainFitRotation=Rotation;
    State.TerrainFitAngleDegrees=float(FMath::RadiansToDegrees(Rotation.GetAngle()));
    State.TerrainFitLiftCm=FMath::Clamp(1.5f-PosedTerrainClearance(Points,State.TerrainPivot,Rotation),0.f,30.f);
    const auto FittedPoint=[&](FName Name) { return State.TerrainPivot+Rotation.RotateVector(Points.FindChecked(Name)-State.TerrainPivot)+FVector(0,0,State.TerrainFitLiftCm); };
    State.PredictedFittedHead=FittedPoint(TEXT("Head"));
    State.PredictedFittedPelvis=FittedPoint(TEXT("pelvis"));
    return true;
}

void ApplyDeathTerrainFit(USkeletalMeshComponent* Mesh,const FFrontlineDeathState& State,float ClipTime)
{
    const float NormalizedTime=ClipTime/FMath::Max(.1f,State.AuthoredClipSeconds);
    const float T=FMath::Clamp((NormalizedTime-.18f)/.66f,0.f,1.f);
    const float Weight=T*T*(3.f-2.f*T);
    const FQuat Rotation=FQuat::Slerp(FQuat::Identity,State.TerrainFitRotation,Weight);
    FTransform Fitted=State.AuthoredBaseTransform;
    Fitted.SetRotation(Rotation*Fitted.GetRotation());
    Fitted.SetLocation(State.TerrainPivot+Rotation.RotateVector(State.AuthoredBaseTransform.GetLocation()-State.TerrainPivot)
        +FVector(0,0,State.TerrainFitLiftCm*Weight));
    Mesh->SetWorldTransform(Fitted,false,nullptr,ETeleportType::TeleportPhysics);
}

void AddLimbShape(USkeletalMeshComponent* Mesh,USkeletalBodySetup* Body,const FTransform& BodyWorld,const FDeathBodySpec& Spec)
{
    const FVector Start=BodyWorld.InverseTransformPosition(Mesh->GetBoneLocation(Spec.Bone));
    const FVector End=BodyWorld.InverseTransformPosition(Mesh->GetBoneLocation(Spec.End));
    const FVector Span=End-Start;
    const float Scale=FMath::Max(.0001f,float(BodyWorld.GetScale3D().GetAbsMax()));
    const float Radius=Spec.RadiusCm/Scale;
    FKSphylElem Capsule(Radius,FMath::Max(.2f/Scale,float(Span.Size())-2.f*Radius));
    Capsule.Center=(Start+End)*.5;
    Capsule.Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Span.GetSafeNormal()).Rotator();
    Body->AggGeom.SphylElems.Add(Capsule);
}

void AuditSourceAsset(USkeletalMeshComponent* Mesh,UPhysicsAsset* Original)
{
    static bool bWritten=false;
    if (bWritten || !Original) return;
    bWritten=true;
    auto Root=MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("source_asset"),Original->GetPathName());
    Root->SetStringField(TEXT("scope"),TEXT("Read-only audit of the actual source_asset path at the first lethal in-game pose. The V11 import creates its own automatic PhysicsAsset; this is not evidence of the approved V07 binary. AABB overlaps are conservative candidates, not proof of narrow-phase penetration."));
    Root->SetStringField(TEXT("mesh_component_scale"),Mesh->GetComponentScale().ToString());
    Root->SetNumberField(TEXT("body_count"),Original->SkeletalBodySetups.Num());
    Root->SetNumberField(TEXT("constraint_count"),Original->ConstraintSetup.Num());
    TArray<TSharedPtr<FJsonValue>> Bodies,Pairs;
    TArray<FBox> Boxes;
    for (const USkeletalBodySetup* Setup:Original->SkeletalBodySetups)
    {
        auto Row=MakeShared<FJsonObject>();
        const int32 BoneIndex=Mesh->GetBoneIndex(Setup->BoneName);
        const FTransform Bone=BoneIndex>=0?Mesh->GetBoneTransform(BoneIndex):FTransform::Identity;
        const FBox Bounds=Setup->AggGeom.CalcAABB(Bone);
        Boxes.Add(Bounds);
        Row->SetStringField(TEXT("bone"),Setup->BoneName.ToString());
        Row->SetStringField(TEXT("world_bone_scale"),Bone.GetScale3D().ToString());
        Row->SetStringField(TEXT("posed_aabb_size_cm"),Bounds.GetSize().ToString());
        Row->SetNumberField(TEXT("shapes"),Setup->AggGeom.GetElementCount());
        Row->SetNumberField(TEXT("scaled_volume_cm3"),Setup->AggGeom.GetScaledVolume(Bone.GetScale3D()));
        Row->SetNumberField(TEXT("linear_damping"),Setup->DefaultInstance.LinearDamping);
        Row->SetNumberField(TEXT("angular_damping"),Setup->DefaultInstance.AngularDamping);
        Bodies.Add(MakeShared<FJsonValueObject>(Row));
    }
    for (int32 A=0;A<Boxes.Num();++A) for (int32 B=A+1;B<Boxes.Num();++B)
        if (Boxes[A].Intersect(Boxes[B]) && !Original->CollisionDisableTable.Contains(FRigidBodyIndexPair(A,B)))
            Pairs.Add(MakeShared<FJsonValueString>(Original->SkeletalBodySetups[A]->BoneName.ToString()+TEXT(" / ")+Original->SkeletalBodySetups[B]->BoneName.ToString()));
    Root->SetArrayField(TEXT("bodies"),Bodies);
    Root->SetArrayField(TEXT("collidable_posed_aabb_overlap_candidates"),Pairs);
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V11");
    IFileManager::Get().MakeDirectory(*Directory,true);
    FString Json;
    FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("death-physics-asset-audit.json")));
    UE_LOG(LogTemp,Display,TEXT("WF_DEATH_SOURCE_AUDIT source=%s bodies=%d constraints=%d collidable_aabb_candidates=%d component_scale=%s"),
        *Original->GetPathName(),Bodies.Num(),Original->ConstraintSetup.Num(),Pairs.Num(),*Mesh->GetComponentScale().ToString());
}

UPhysicsAsset* MakeDeathAsset(USkeletalMeshComponent* Mesh,UObject* Outer,bool bPreserveAuthoredPose)
{
    UPhysicsAsset* Original=Mesh->GetPhysicsAsset();
    USkeletalMesh* SkeletalMesh=Mesh->GetSkeletalMeshAsset();
    if (!Original || !SkeletalMesh) return nullptr;
    AuditSourceAsset(Mesh,Original);
    const FReferenceSkeleton& Skeleton=SkeletalMesh->GetRefSkeleton();
    UPhysicsAsset* Asset=DuplicateObject<UPhysicsAsset>(Original,Outer,TEXT("PA_Soldier_Death_V11"));
    Asset->SetFlags(RF_Transient);
    Asset->SkeletalBodySetups.Reset(); Asset->ConstraintSetup.Reset();
    Asset->CollisionDisableTable.Reset(); Asset->BodySetupIndexMap.Reset();
    UPhysicalMaterial* Contact=NewObject<UPhysicalMaterial>(Asset,NAME_None,RF_Transient);
    Contact->Friction=.8f; Contact->StaticFriction=.9f; Contact->Restitution=0.f;
    Contact->bOverrideRestitutionCombineMode=true;
    Contact->RestitutionCombineMode=EFrictionCombineMode::Min;
    for (const FDeathBodySpec& Spec:DeathBodies)
    {
        if (IsMergedExtremity(Spec)) continue;
        const int32 BoneIndex=Skeleton.FindBoneIndex(Spec.Bone);
        if (BoneIndex<0 || (Spec.End && Skeleton.FindBoneIndex(Spec.End)<0)) return nullptr;
        // Use exactly the transform that InstantiatePhysicsAsset will consume.
        // This asset's reference skeleton has scale 1 but the approved animation
        // evaluates bones at scale 100. Reference-scale radii produced 100x bodies.
        const FTransform Bone=Mesh->GetBoneTransform(BoneIndex);
        const float Scale=FMath::Max(.0001f,float(Bone.GetScale3D().GetAbsMax()));
        USkeletalBodySetup* Body=NewObject<USkeletalBodySetup>(Asset,NAME_None,RF_Transient);
        Body->BoneName=Spec.Bone; Body->PhysicsType=PhysType_Simulated; Body->bConsiderForBounds=true;
        Body->PhysMaterial=Contact;
        Body->CollisionTraceFlag=CTF_UseSimpleAsComplex;
        if (Spec.End)
        {
            AddLimbShape(Mesh,Body,Bone,Spec);
        }
        else
        {
            FKSphereElem Head(Spec.RadiusCm/Scale);
            const FVector SkullDirection=(Bone.GetLocation()-Mesh->GetBoneLocation(TEXT("neck_01"))).GetSafeNormal();
            Head.Center=Bone.InverseTransformVector(SkullDirection*7.f);
            Body->AggGeom.SphereElems.Add(Head);
        }
        // Hands and boots retain their own contact shape, welded to the forearm or
        // lower leg. The small independent bodies were the measured first source
        // of high-frequency angular spikes on ground contact. At the game camera
        // scale independent wrist/ankle dynamics add instability without useful detail.
        float Mass=Spec.MassKg;
        for (const FDeathBodySpec& Extremity:DeathBodies)
            if (IsMergedExtremity(Extremity) && FCString::Strcmp(Extremity.Parent,Spec.Bone)==0)
            {
                if (Skeleton.FindBoneIndex(Extremity.Bone)<0 || Skeleton.FindBoneIndex(Extremity.End)<0) return nullptr;
                AddLimbShape(Mesh,Body,Bone,Extremity); Mass+=Extremity.MassKg;
            }
        FBodyInstance& Instance=Body->DefaultInstance;
        Instance.SetMassOverride(Mass,true);
        Instance.LinearDamping=.25f; Instance.AngularDamping=3.f;
        Instance.SetOverrideIterationCounts(true);
        Instance.SetPositionSolverIterationCount(12); Instance.SetVelocitySolverIterationCount(4);
        Instance.SetMaxDepenetrationVelocity(100.f);
        Instance.SetMaxAngularVelocityInRadians(7.f,false);
        Asset->SkeletalBodySetups.Add(Body);
    }
    if (bPreserveAuthoredPose)
    {
        // Ordinary gun deaths have an approved complete silhouette. Preserve it
        // with one real compound rigid body, rather than unlocking shoulders and
        // elbows after the animation has already finished its fall. All 16 contact
        // shapes remain, expressed in true skeleton-root space; explosions retain
        // the joints. Binding to root also avoids non-root scaled-parent rebasing.
        USkeletalBodySetup* Compound=DuplicateObject<USkeletalBodySetup>(Asset->SkeletalBodySetups[0],Asset);
        Compound->BoneName=Skeleton.GetBoneName(0);
        Compound->AggGeom.SphylElems.Reset(); Compound->AggGeom.SphereElems.Reset();
        const FTransform RootWorld=Mesh->GetBoneTransform(0);
        float TotalMass=0;
        for (int32 BodyIndex=0;BodyIndex<Asset->SkeletalBodySetups.Num();++BodyIndex)
        {
            USkeletalBodySetup* Part=Asset->SkeletalBodySetups[BodyIndex];
            TotalMass+=Part->DefaultInstance.GetMassOverride();
            const FTransform PartWorld=Mesh->GetBoneTransform(Mesh->GetBoneIndex(Part->BoneName));
            const float ScaleRatio=float(PartWorld.GetScale3D().GetAbsMax()/RootWorld.GetScale3D().GetAbsMax());
            for (const FKSphylElem& Shape:Part->AggGeom.SphylElems)
            {
                FKSphylElem Merged=Shape;
                Merged.Center=RootWorld.InverseTransformPosition(PartWorld.TransformPosition(Shape.Center));
                Merged.Rotation=(RootWorld.GetRotation().Inverse()*PartWorld.GetRotation()*Shape.Rotation.Quaternion()).Rotator();
                Merged.Radius*=ScaleRatio; Merged.Length*=ScaleRatio;
                Compound->AggGeom.SphylElems.Add(Merged);
            }
            for (const FKSphereElem& Shape:Part->AggGeom.SphereElems)
            {
                FKSphereElem Merged=Shape;
                Merged.Center=RootWorld.InverseTransformPosition(PartWorld.TransformPosition(Shape.Center));
                Merged.Radius*=ScaleRatio; Compound->AggGeom.SphereElems.Add(Merged);
            }
        }
        Compound->DefaultInstance.SetMassOverride(TotalMass,true);
        Asset->SkeletalBodySetups.Reset(); Asset->SkeletalBodySetups.Add(Compound);
    }
    Asset->UpdateBodySetupIndexMap();
    // Tight carried-arm poses naturally overlap torso/weapon space. At this camera scale
    // self-collision brings no useful contact detail and creates an opposing-force loop.
    for (int32 A=0;A<Asset->SkeletalBodySetups.Num();++A)
        for (int32 B=A+1;B<Asset->SkeletalBodySetups.Num();++B) Asset->DisableCollision(A,B);
    for (const FDeathBodySpec& Spec:DeathBodies) if (!bPreserveAuthoredPose && Spec.Parent && !IsMergedExtremity(Spec))
    {
        const FTransform Child=Mesh->GetBoneTransform(Mesh->GetBoneIndex(Spec.Bone));
        const FTransform Parent=Mesh->GetBoneTransform(Mesh->GetBoneIndex(Spec.Parent));
        UPhysicsConstraintTemplate* Template=NewObject<UPhysicsConstraintTemplate>(Asset,NAME_None,RF_Transient);
        FConstraintInstance& Joint=Template->DefaultInstance;
        Joint.JointName=Spec.Bone; Joint.ConstraintBone1=Spec.Bone; Joint.ConstraintBone2=Spec.Parent;
        // Start the constraints at the visible death pose. Enabling physics must not
        // violently pull a raised rifle pose back to an autogenerated reference pose.
        // FBX bone axes are not guaranteed to place the limb along X. Align the
        // constraint twist axis to the measured limb so the knee/elbow is not
        // forced to use a narrow twist limit as its main bending axis.
        const FVector LimbAxis=Spec.End
            ?(Mesh->GetBoneLocation(Spec.End)-Child.GetLocation()).GetSafeNormal()
            :Child.GetUnitAxis(EAxis::Y);
        FVector BendAxis=FVector::VectorPlaneProject(Mesh->GetOwner()->GetActorForwardVector(),LimbAxis).GetSafeNormal();
        if (BendAxis.IsNearlyZero()) BendAxis=FVector::VectorPlaneProject(Mesh->GetOwner()->GetActorRightVector(),LimbAxis).GetSafeNormal();
        const bool bHinge=FCString::Strncmp(Spec.Bone,TEXT("lowerarm_"),9)==0 || FCString::Strncmp(Spec.Bone,TEXT("calf_"),5)==0;
        if (bHinge)
        {
            // Chaos Swing1 rotates about frame Z, so its broad 75-degree range
            // must bend in frame XY. Actor forward is not the bend plane of a
            // raised/cross-body arm. Use this limb's actual parent-child-end plane.
            const FVector UpperAxis=(Child.GetLocation()-Parent.GetLocation()).GetSafeNormal();
            const FVector AnatomicalY=FVector::VectorPlaneProject(UpperAxis,LimbAxis);
            // A nearly straight limb has no well-conditioned plane. Retain the
            // existing actor-axis fallback within roughly .57 degrees of collinearity.
            const bool bUseLimbPlane=AnatomicalY.SizeSquared()>1.e-4;
            const FVector PreviousY=BendAxis;
            if (bUseLimbPlane) BendAxis=AnatomicalY.GetSafeNormal();
            const double PlaneChange=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FMath::Abs(FVector::DotProduct(PreviousY,BendAxis)),0.,1.)));
            const double LimbBend=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(UpperAxis,LimbAxis),-1.,1.)));
            const AWarfareFrontlineSoldier* Unit=Cast<AWarfareFrontlineSoldier>(Mesh->GetOwner());
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_HINGE_FRAME frame=%llu squad=%d member=%d joint=%s limb_plane=%d old_plane_error_deg=%.3f actual_bend_deg=%.3f world_x=%s world_y=%s swing1=%.1f swing2=%.1f twist=%.1f"),
                static_cast<unsigned long long>(GFrameCounter),Unit?Unit->SquadId:INDEX_NONE,Unit?Unit->MemberIndex:INDEX_NONE,Spec.Bone,bUseLimbPlane,
                PlaneChange,LimbBend,*LimbAxis.ToString(),*BendAxis.ToString(),Spec.Swing1,Spec.Swing2,Spec.Twist);
        }
        const FQuat FrameRotation=FRotationMatrix::MakeFromXY(LimbAxis,BendAxis).ToQuat();
        Joint.SetRefFrame(EConstraintFrame::Frame1,FTransform(Child.GetRotation().Inverse()*FrameRotation,FVector::ZeroVector));
        Joint.SetRefFrame(EConstraintFrame::Frame2,FTransform(Parent.GetRotation().Inverse()*FrameRotation,Parent.InverseTransformPosition(Child.GetLocation())));
        Joint.SetLinearLimits(LCM_Locked,LCM_Locked,LCM_Locked,0.f);
        Joint.SetAngularSwing1Limit(ACM_Limited,Spec.Swing1);
        Joint.SetAngularSwing2Limit(ACM_Limited,Spec.Swing2);
        Joint.SetAngularTwistLimit(ACM_Limited,Spec.Twist);
        if (bHinge)
        {
            // Default UE soft limits permit measurable elbow/knee overtravel and
            // store spring energy. Keep the anatomical angles, use passive hard
            // stops, and retain contact distances without treating them as a fix.
            Joint.SetSoftSwingLimitParams(false,0.f,0.f,0.f,Joint.GetSoftSwingLimitContactDistance());
            Joint.SetSoftTwistLimitParams(false,0.f,0.f,0.f,Joint.GetSoftTwistLimitContactDistance());
        }
        if (FCString::Strcmp(Spec.Bone,TEXT("lowerarm_r"))==0)
        {
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_ELBOW_COMPLIANCE joint=%s mode=passive_hard_hinge soft_swing=%d soft_twist=%d swing1_limit=%.1f swing2_limit=%.1f twist_limit=%.1f"),
                Spec.Bone,int32(Joint.GetIsSoftSwingLimit()),int32(Joint.GetIsSoftTwistLimit()),Spec.Swing1,Spec.Swing2,Spec.Twist);
        }
        Joint.SetDisableCollision(true); Joint.EnableMassConditioning();
        // Passive ragdoll limits only. Active acceleration-mode motors added an
        // unnecessary competing correction during ground contact; body damping
        // handles energy dissipation without a joint target.
        Joint.SetAngularDriveMode(EAngularDriveMode::SLERP);
        Joint.SetAngularVelocityDriveSLERP(false);
        Joint.SetAngularDriveParams(0.f,0.f,0.f);
        Joint.SetProjectionParams(false,0,0,5,30); Joint.SetShockPropagationParams(false,0);
        Joint.SetLinearBreakable(false,0); Joint.SetAngularBreakable(false,0);
#if WITH_EDITOR
        // Editor game worlds reapply the template's separate DefaultProfile after
        // copying DefaultInstance. Keep it in sync or every tuned limit/drive above
        // is silently replaced by a fresh ball-socket profile in editor validation.
        Template->SetDefaultProfile(Joint);
#endif
        Asset->ConstraintSetup.Add(Template);
    }
    Asset->UpdateBoundsBodiesArray();
    return Asset;
}

FVector PelvisPosition(USkeletalMeshComponent* Mesh)
{
    if (const FBodyInstance* Body=Mesh->GetBodyInstance(TEXT("pelvis"))) return Body->GetUnrealWorldTransform().GetLocation();
    return Mesh->GetBoneLocation(TEXT("pelvis"));
}

FString DeathGroundReadback(USkeletalMeshComponent* Mesh,const AWarfareFrontlineGameMode* Battle,const FFrontlineDeathState& State)
{
    const auto Number=[](bool Valid,float Value)
    { return Valid && FMath::IsFinite(Value) && Value!=MAX_flt?FString::Printf(TEXT("%.4f"),Value):FString(TEXT("unavailable")); };
    FString Out=FString::Printf(TEXT("prone_at_hit=%d entry_clip=%s authored_clip=%s initial_native=%d initial_pelvis_shape_clearance_cm=%s initial_chest_shape_clearance_cm=%s"),
        State.bProneAtHit,*State.ActualEntryClip,*GetNameSafe(State.AuthoredAnimation.Get()),State.bInitialNativeGroundRead,
        *Number(State.bInitialNativeGroundRead,State.InitialPelvisTerrainClearanceCm),*Number(State.bInitialNativeGroundRead,State.InitialChestTerrainClearanceCm));
    for(const FName Bone:{FName(TEXT("pelvis")),FName(TEXT("spine_03"))})
    {
        const FBodyInstance* Body=Mesh?Mesh->GetBodyInstance(Bone):nullptr;
        if(!Body||!Body->IsValidBodyInstance()){Out+=FString::Printf(TEXT(" %s_body=unavailable"),*Bone.ToString());continue;}
        const FTransform Chaos=Body->GetUnrealWorldTransform();const FVector Position=Chaos.GetLocation();
        const auto Bounds=Body->GetBodyBounds();float Floor=0;
        const bool NativeFloor=WarfareFRRUWorldV82::TryHeight(Battle,Position.X,Position.Y,Floor);
        // AABB lower support samples are labelled separately: their corners
        // can lie outside the primitive. Shape clearance uses actual capsules.
        float AabbClearance=MAX_flt;bool AabbNative=true;
        const FVector Centre=Bounds.GetCenter();
        for(const FVector P:{FVector(Centre.X,Centre.Y,Bounds.Min.Z),FVector(Bounds.Min.X,Bounds.Min.Y,Bounds.Min.Z),
            FVector(Bounds.Min.X,Bounds.Max.Y,Bounds.Min.Z),FVector(Bounds.Max.X,Bounds.Min.Y,Bounds.Min.Z),FVector(Bounds.Max.X,Bounds.Max.Y,Bounds.Min.Z)})
        {
            float SampleFloor=0;
            if(WarfareFRRUWorldV82::TryHeight(Battle,P.X,P.Y,SampleFloor))AabbClearance=FMath::Min(AabbClearance,float(P.Z-SampleFloor));
            else AabbNative=false;
        }
        Out+=FString::Printf(TEXT(" %s{chaos_position=%s chaos_rotation=%s local_z_axis=%s aabb_size_cm=%s native_floor=%s origin_clearance_cm=%s shape_clearance_cm=%s aabb_lower_sample_clearance_cm=%s visual_bone_error_cm=%.4f simulating=%d awake=%d}"),
            *Bone.ToString(),*Position.ToString(),*Chaos.GetRotation().Rotator().ToString(),*Chaos.GetUnitAxis(EAxis::Z).ToString(),*Bounds.GetSize().ToString(),
            *Number(NativeFloor,Floor),*Number(NativeFloor,float(Position.Z-Floor)),*Number(NativeFloor,BodyTerrainClearance(Body)),*Number(AabbNative,AabbClearance),
            FVector::Dist(Position,Mesh->GetBoneLocation(Bone)),Body->IsInstanceSimulatingPhysics(),Body->IsInstanceAwake());
    }
    const FBodyInstance* Pelvis=Mesh?Mesh->GetBodyInstance(TEXT("pelvis")):nullptr;
    const FBodyInstance* Chest=Mesh?Mesh->GetBodyInstance(TEXT("spine_03")):nullptr;
    if(Pelvis&&Chest&&Pelvis->IsValidBodyInstance()&&Chest->IsValidBodyInstance())
        Out+=FString::Printf(TEXT(" chaos_pelvis_to_chest_axis=%s"),*(Chest->GetUnrealWorldTransform().GetLocation()-Pelvis->GetUnrealWorldTransform().GetLocation()).GetSafeNormal().ToString());
    return Out;
}

void AuditActualBodies(USkeletalMeshComponent* Mesh)
{
    static bool Written[2]={false,false};
    const bool Articulated=!Mesh->Constraints.IsEmpty();
    if (Written[Articulated?1:0]) return;
    Written[Articulated?1:0]=true;
    auto Root=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Bodies,Joints;
    Root->SetStringField(TEXT("scope"),TEXT("Actual initialized V11 death bodies. Bounds and mass/inertia are read from FBodyInstance, not inferred from asset presence. Reported inertia excludes Chaos automatic inertia conditioning."));
    float TotalMass=0;
    for (const FBodyInstance* Body:Mesh->Bodies) if (Body && Body->IsValidBodyInstance())
    {
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("bone"),Mesh->GetBoneName(Body->InstanceBoneIndex).ToString());
        Row->SetStringField(TEXT("scale"),Body->Scale3D.ToString());
        Row->SetStringField(TEXT("actual_aabb_size_cm"),Body->GetBodyBounds().GetSize().ToString());
        Row->SetNumberField(TEXT("mass_kg"),Body->GetBodyMass()); TotalMass+=Body->GetBodyMass();
        Row->SetStringField(TEXT("inertia_kg_cm2"),Body->GetBodyInertiaTensor().ToString());
        Row->SetStringField(TEXT("configured_inertia_tensor_scale"),Body->InertiaTensorScale.ToString());
        Row->SetNumberField(TEXT("contact_shapes"),Body->GetBodySetup()?Body->GetBodySetup()->AggGeom.GetElementCount():0);
        Row->SetNumberField(TEXT("position_iterations"),Body->GetPositionSolverIterationCount());
        Row->SetNumberField(TEXT("velocity_iterations"),Body->GetVelocitySolverIterationCount());
        Row->SetNumberField(TEXT("angular_limit_rad_s"),Body->GetMaxAngularVelocityInRadians());
        Row->SetBoolField(TEXT("continuous_collision_detection"),Body->bUseCCD);
        Row->SetNumberField(TEXT("linear_damping"),Body->LinearDamping);
        Row->SetNumberField(TEXT("angular_damping"),Body->AngularDamping);
        Row->SetBoolField(TEXT("inertia_conditioning"),Body->IsInertiaConditioningEnabled());
        // Exact primitive support samples against the authored terrain height. This
        // excludes props and is labelled separately from a scene collision query.
        const float Clearance=BodyTerrainClearance(Body);
        Row->SetNumberField(TEXT("initial_terrain_support_clearance_cm"),Clearance==MAX_flt?0:Clearance);
        Bodies.Add(MakeShared<FJsonValueObject>(Row));
    }
    for (const FConstraintInstance* Joint:Mesh->Constraints) if (Joint)
    {
        auto Row=MakeShared<FJsonObject>(); const auto& Profile=Joint->ProfileInstance;
        Row->SetStringField(TEXT("joint"),Joint->JointName.ToString());
        Row->SetBoolField(TEXT("valid_initialized_constraint"),Joint->IsValidConstraintInstance());
        Row->SetNumberField(TEXT("swing1_motion"),int32(Joint->GetAngularSwing1Motion()));
        Row->SetNumberField(TEXT("swing2_motion"),int32(Joint->GetAngularSwing2Motion()));
        Row->SetNumberField(TEXT("twist_motion"),int32(Joint->GetAngularTwistMotion()));
        Row->SetNumberField(TEXT("swing1_degrees"),Joint->GetAngularSwing1Limit());
        Row->SetNumberField(TEXT("swing2_degrees"),Joint->GetAngularSwing2Limit());
        Row->SetNumberField(TEXT("twist_degrees"),Joint->GetAngularTwistLimit());
        Row->SetBoolField(TEXT("slerp_velocity_drive"),Profile.AngularDrive.SlerpDrive.bEnableVelocityDrive);
        Row->SetBoolField(TEXT("slerp_position_drive"),Profile.AngularDrive.SlerpDrive.bEnablePositionDrive);
        Row->SetBoolField(TEXT("drive_acceleration_mode"),Profile.AngularDrive.bAccelerationMode);
        Row->SetNumberField(TEXT("angular_drive_mode"),int32(Profile.AngularDrive.AngularDriveMode));
        Row->SetNumberField(TEXT("angular_drive_damping"),Profile.AngularDrive.SlerpDrive.Damping);
        Row->SetBoolField(TEXT("soft_swing_limit"),Joint->GetIsSoftSwingLimit());
        Row->SetBoolField(TEXT("soft_twist_limit"),Joint->GetIsSoftTwistLimit());
        Row->SetNumberField(TEXT("soft_swing_stiffness"),Joint->GetSoftSwingLimitStiffness());
        Row->SetNumberField(TEXT("soft_swing_damping"),Joint->GetSoftSwingLimitDamping());
        Row->SetNumberField(TEXT("soft_twist_stiffness"),Joint->GetSoftTwistLimitStiffness());
        Row->SetNumberField(TEXT("soft_twist_damping"),Joint->GetSoftTwistLimitDamping());
        Row->SetBoolField(TEXT("projection"),Profile.bEnableProjection);
        Row->SetBoolField(TEXT("mass_conditioning"),Profile.bEnableMassConditioning);
        Joints.Add(MakeShared<FJsonValueObject>(Row));
    }
    Root->SetNumberField(TEXT("total_mass_kg"),TotalMass); Root->SetArrayField(TEXT("bodies"),Bodies);
    Root->SetArrayField(TEXT("initialized_constraints"),Joints);
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V11");
    IFileManager::Get().MakeDirectory(*Directory,true);
    FString Json; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(Directory/(Articulated?TEXT("actual-death-bodies-explosion.json"):TEXT("actual-death-bodies.json"))));
}
}

void AWarfareFrontlineSoldier::ReceiveHit(float Impact,float Pressure,int32 AttackingTeam,const FVector& ForceDirection,EFrontlineDamageKind Kind,float ForceScale,const FFrontlineKillSourceV65& SourceV65)
{
    const bool CarrierFatal=Kind==EFrontlineDamageKind::Artillery&&FMath::IsFinite(Impact)&&Impact>0
        &&WarfareFRRUArmourV82::DestroyedCarrierPassenger(this);
    const bool CommittedSupport=Battle && Battle->CanResolveCommittedSupportV8217()
        &&SourceV65.Support>=0&&SourceV65.Support<int32(EFrontlineSupport::Count)
        &&SourceV65.Team==AttackingTeam;
    if ((!IsActiveCombatant() || !Battle || (Battle->Phase!=EFrontlinePhase::Fighting&&!CommittedSupport))&&!CarrierFatal) return;
    const bool bLandedHit=FMath::IsFinite(Impact) && Impact>0.f;
    // Keep the alive/dead marker binary. The flame operator can survive one
    // landed firearm round; misses and pressure never spend that protection.
    // Explosions, fire and gas retain their ordinary one-hit casualty path.
    const bool bProtectedBullet=bLandedHit && Kind==EFrontlineDamageKind::Bullet
        && FrontlineFieldV32::IsFlameOperator(this) && !bFlameBulletProtectionSpentV48;
    if(bProtectedBullet)bFlameBulletProtectionSpentV48=true;
    const bool bFatalHit=bLandedHit && !bProtectedBullet;
    const float Applied=bFatalHit?100.f:0.f;
    if (bFatalHit)
    {
        // Acquisition admits an actual crossing before this soldier's own Tick.
        // Freeze that living position before death bypasses the entry update;
        // later ragdoll motion must never create or erase a participant.
        if(!bFieldEntered&&!WarfareFRRUCampaignV82::Active(Battle)
            &&FrontlinePacingV24::HasCrossedEntry(Feet().X,Battle->GetTeamDirection(Team)))
            bFieldEntered=true;
        Health=0.f;
    }
    const float ValidPressure=FMath::IsFinite(Pressure)?FMath::Max(0.f,Pressure):0.f;
    Suppression=FMath::Clamp(Suppression+ValidPressure,0.f,1.f);
    Battle->PeakSuppression=FMath::Max(Battle->PeakSuppression,Suppression);
    if (AttackingTeam==0 || AttackingTeam==1)
    {
        Battle->DamageDealt[AttackingTeam]+=Applied;
        if (AttackingTeam==Team) Battle->FriendlyFireDamage+=Applied;
    }
    if (bFatalHit)
    {
        auto Credit=SourceV65;Credit.Team=AttackingTeam;FatalSourceV65=Credit;
#if WITH_EDITOR
        if(WarfareFRRUCampaignV82::Active(Battle))
            UE_LOG(LogTemp,Display,TEXT("WF_FRRU_FATAL host=%.3f victim_side=%d squad=%d member=%d role=%d kind=%d source_side=%d source_squad=%d source_role=%d support=%d mission=%d vehicle=%d feet={%s}"),
                Battle->Elapsed,Team,SquadId,MemberIndex,int32(Role),int32(Kind),Credit.Team,Credit.SquadId,Credit.Role,Credit.Support,Credit.FireMissionId,Credit.VehicleSerial,*GetActorLocation().ToString());
#endif
        FrontlineCombatRecordV65::InfantryLost(Battle,Team,Credit,this);
        FrontlineFieldV32::OnFatalHitV33(this,Kind,AttackingTeam,ForceDirection);
        FrontlineMotionV40::Abort(this);
        FrontlineMotionV41::CancelWaiting(this);
        if(Kind!=EFrontlineDamageKind::Gas && Kind!=EFrontlineDamageKind::Fire)if(auto* FX=AWarfareFrontlineWoundFX::Get(GetWorld()))
        {
            if(Kind==EFrontlineDamageKind::HeavyBullet)FX->FatalHeavyBulletV75(this,ForceDirection);
            else FX->FatalHit(this,ForceDirection,IsFrontlineExplosiveDamage(Kind));
        }
        if(auto* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        {
            if(Kind==EFrontlineDamageKind::Gas)Audio->PlayGasCasualty(GetFaction()==EFrontlineFaction::German,GetActorLocation(),SquadId*16+MemberIndex,SquadId);
            else if(Kind==EFrontlineDamageKind::Fire)Audio->PlayBurnCasualty(GetFaction()==EFrontlineFaction::German,GetActorLocation(),SquadId*16+MemberIndex,SquadId);
            else
            Audio->PlayCasualty(GetFaction()==EFrontlineFaction::German,GetActorLocation(),SquadId*16+MemberIndex,
                true,IsFrontlineExplosiveDamage(Kind)||Kind==EFrontlineDamageKind::Fire||Kind==EFrontlineDamageKind::HeavyBullet,Applied,SquadId);
        }
    }
    if (IsAlive()) return;
    const FVector InheritedVelocity=GetActorForwardVector()*FMath::Min(MotionSpeed,280.f);
    bMoving=bEngaging=bWantedMovement=false; MotionSpeed=0; FocusTarget.Reset();
    Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DeathState=FFrontlineDeathState(); DeathState.bStarted=true; DeathState.Kind=Kind;
    FVector Direction=ForceDirection.ContainsNaN()?FVector::ZeroVector:ForceDirection.GetSafeNormal();
    if (Direction.IsNearlyZero()) Direction=-GetActorForwardVector();
    DeathState.IncomingDirection=Direction;
    DeathState.InitialPelvis=Soldier->GetBoneLocation(TEXT("pelvis"));
    DeathState.InheritedVelocity=InheritedVelocity;
    DeathState.ForceScale=FMath::Clamp(ForceScale,0.f,1.f);
    // The casualty leaves gameplay immediately; a complete authored loss of support
    // precedes ordinary gun-death physics. Explosions apply force on the next
    // complete PrePhysics tick, preserving this actual pose during the handoff.
    Battle->RegisterCasualty(this,AttackingTeam);
    UAnimationAsset* SelectedFall=Battle->DeathAnimation;
    if (!IsFrontlineExplosiveDamage(Kind) && !Battle->DeathAnimations.IsEmpty())
    {
        // Stable per-man variation avoids a whole squad repeating the same take.
        // Moving casualties can pitch forward; lateral impacts favour the side
        // take. The force response remains separate from authored choreography.
        const uint32 Variation=uint32(SquadId)*73856093u ^ uint32(MemberIndex)*19349663u ^ uint32(Team)*83492791u;
        int32 FallIndex=int32(Variation%uint32(Battle->DeathAnimations.Num()));
        if (Battle->DeathAnimations.Num()>=3)
        {
            const float Lateral=FMath::Abs(float(FVector::DotProduct(Direction,GetActorRightVector())));
            if (Lateral>.45f) FallIndex=2;
            else if (InheritedVelocity.SizeSquared()>FMath::Square(20.f) && (Variation&1u)) FallIndex=1;
        }
        if (Battle->DeathAnimations[FallIndex])
        {
            SelectedFall=Battle->DeathAnimations[FallIndex];
            DeathState.AuthoredVariantIndex=FallIndex;
        }
    }
    if(Kind==EFrontlineDamageKind::Fire)if(auto* Burn=FrontlineFieldV32::BurningFallV33(this))SelectedFall=Burn;
    const auto* SupportedGun=FrontlineMachinegunsV30::Get(this);
    const bool WasProne=SupportedGun && SupportedGun->IsProne();
    DeathState.bProneAtHit=WasProne;
    if(UAnimSingleNodeInstance* EntryNode=Soldier->GetSingleNodeInstance())
        DeathState.ActualEntryClip=EntryNode->GetCurrentAsset()?EntryNode->GetCurrentAsset()->GetPathName():TEXT("None");
    // Injury belongs to this committed hit, before its fall or ragdoll starts.
    // A leader-pose follower changes visible triangles only; the authoritative
    // articulated body remains intact for the established safe physics handoff.
    DeathState.bSeverAttempted=true;
    const uint32 SeverChoice=uint32(SquadId)*92821u^uint32(MemberIndex+1)*68917u^uint32(Team+1)*283923u;
    const bool SevereLoss=IsFrontlineExplosiveDamage(Kind)&&DeathState.ForceScale>.64f&&(SeverChoice%100u)<64u;
    const bool HeavyLoss=Kind==EFrontlineDamageKind::HeavyBullet&&((SeverChoice*1664525u+1013904223u)%100u)<82u;
    if(SevereLoss||HeavyLoss)if(auto* FX=AWarfareFrontlineWoundFX::Get(GetWorld()))FX->ShowBlastLoss(this);
    if(auto* NewFall=FrontlineMotionV41::FatalMotion(this,Kind,Direction,WasProne,DeathState.AuthoredVariantIndex))SelectedFall=NewFall;
    if(Kind==EFrontlineDamageKind::Gas)if(auto* Gas=FrontlineMotionV40::GasDeathMotion(this,WasProne))SelectedFall=Gas;
    // Resolve the displayed skeleton once. Playback, compressed terrain fitting
    // and the later ragdoll handoff must all use that same actual sequence.
    if(WarfareFRRUCampaignV82::Active(Battle)&&SelectedFall)
    {
        SelectedFall=WarfareFRRUPresentationV82::RemapMotion(this,SelectedFall);
        if(!SelectedFall)
        {
            DeathState.bMissingPhysics=true;
            UE_LOG(LogTemp,Error,TEXT("WF_DEATH_ANIMATION_FAILURE squad=%d member=%d: required national death sequence is missing."),SquadId,MemberIndex);
            return;
        }
    }
    // A prone casualty loses elbow support in the actual displayed pose; a
    // standing fall would first lift him out of the mud by almost a metre.
    if (!IsFrontlineExplosiveDamage(Kind) && SelectedFall && (!WasProne||Kind==EFrontlineDamageKind::Gas))
    {
        DeathState.AuthoredAnimation=SelectedFall;
        Soldier->bPauseAnims=false;
        SetMotion(SelectedFall,false);
        UAnimSingleNodeInstance* Node=Soldier->GetSingleNodeInstance();
        if (Node && Node->GetAnimationAsset()==SelectedFall && Node->GetLength()>.1f)
        {
            // A 30% faster bullet reaction preserves every authored pose and
            // its final ragdoll handoff; clip-time terrain fitting stays exact.
            Node->SetPlayRate(Kind==EFrontlineDamageKind::Bullet||Kind==EFrontlineDamageKind::HeavyBullet?1.30f:
                Kind==EFrontlineDamageKind::Gas?1.10f+MotionPhase*.18f:1.f);
            DeathState.bAuthoredFall=true;
            DeathState.AuthoredClipSeconds=Node->GetLength();
            if (!PrepareDeathTerrainFit(Soldier,SelectedFall,DeathState))
            {
                DeathState.bMissingPhysics=true;
                UE_LOG(LogTemp,Error,TEXT("WF_DEATH_TERRAIN_POSE_FAILURE squad=%d member=%d: complete death pose could not be sampled."),SquadId,MemberIndex);
                return;
            }
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_AUTHORED_BEGIN squad=%d member=%d clip=%s length=%.3f initial_pelvis=%s"),
                SquadId,MemberIndex,*GetNameSafe(SelectedFall),DeathState.AuthoredClipSeconds,*DeathState.InitialPelvis.ToString());
            return;
        }
        DeathState.bMissingPhysics=true;
        UE_LOG(LogTemp,Error,TEXT("WF_DEATH_ANIMATION_FAILURE squad=%d member=%d: actual death animation instance did not accept the required clip."),SquadId,MemberIndex);
        return;
    }
    // Damage can arrive after this frame's mesh evaluation. Starting bodies here
    // would register a late EndPhysics blend against already-published buffers.
    // Freeze the real displayed pose now; do not skip or delay its first audit.
    Soldier->bPauseAnims=true;
    Soldier->HandleExistingParallelEvaluationTask(true,true);
    if (auto* Animation=Cast<UWarfareFrontlineAnimation>(Soldier->GetAnimInstance()))
        Animation->FreezeForRagdoll();
    DeathState.PendingPhysicsFrame=GFrameCounter;
    UE_LOG(LogTemp,Display,TEXT("WF_DEATH_HANDOFF_QUEUED squad=%d member=%d frame=%llu group=%d"),
        SquadId,MemberIndex,static_cast<unsigned long long>(GFrameCounter),int32(GetWorld()->TickGroup.GetValue()));
}

void AWarfareFrontlineSoldier::OnFuelRuptureV41(float RemainingFuel)
{
    if(!Soldier||IsAlive()||!DeathState.bStarted||DeathState.bFuelRupture||!FMath::IsFinite(RemainingFuel))return;
    DeathState.bFuelRupture=true;DeathState.FuelRuptureAt=DeathState.Age;
    const float Strength=FMath::Clamp(RemainingFuel/12.f,0.f,1.f);
    const float Side=(uint32(SquadId)*17u+uint32(MemberIndex)*7u)&1u?1.f:-1.f;
    DeathState.FuelRuptureDeltaV=GetActorForwardVector().GetSafeNormal2D()*FMath::Lerp(220.f,330.f,Strength)
        +GetActorRightVector().GetSafeNormal2D()*(55.f*Side)+FVector::UpVector*FMath::Lerp(70.f,110.f,Strength);
    if(DeathState.bPhysicsStarted){ApplyFuelRuptureImpulseV41();return;}
    // Publish the actual distress pose. Never jump to the clip's final kneel,
    // create physics during a late mesh evaluation, or count this as a new hit.
    Soldier->HandleExistingParallelEvaluationTask(true,true);
    if(auto* N=Soldier->GetSingleNodeInstance()){DeathState.FuelRupturePoseSeconds=N->GetCurrentTime();N->SetPlaying(false);}
    Soldier->bPauseAnims=true;
    if(auto* A=Cast<UWarfareFrontlineAnimation>(Soldier->GetAnimInstance()))A->FreezeForRagdoll();
    DeathState.bFuelInterruptedAuthoredFall=DeathState.bAuthoredFall;
    DeathState.bAuthoredFall=false;DeathState.PendingPhysicsFrame=GFrameCounter;
    UE_LOG(LogTemp,Display,TEXT("WF_V41_FUEL_CARRIER_HANDOFF squad=%d pose=%.3f frame=%llu"),SquadId,DeathState.FuelRupturePoseSeconds,static_cast<unsigned long long>(GFrameCounter));
}

void AWarfareFrontlineSoldier::ApplyFuelRuptureImpulseV41()
{
    if(!Soldier||!DeathState.bPhysicsStarted||!DeathState.bFuelRupture||DeathState.bFuelRuptureApplied)return;
    DeathState.bFuelRuptureApplied=true;DeathState.bSettled=false;DeathState.QuietSeconds=0;DeathState.SettledAt=-1;
    const float Limit=IsFrontlineExplosiveDamage(DeathState.Kind)?950.f:700.f;
    for(FBodyInstance* Body:Soldier->Bodies)if(Body&&Body->IsValidBodyInstance())
    {
        const FVector Old=Body->GetUnrealWorldVelocity();
        const FVector Safe=Old.ContainsNaN()?FVector::ZeroVector:Old;
        Body->SetLinearVelocity((Safe+DeathState.FuelRuptureDeltaV).GetClampedToMaxSize(Limit),false,false);
    }
    // A local reservoir rupture folds the upper body forward. Most energy is
    // lateral, with only 2.5–6 cm of ballistic lift from the added vertical speed.
    if(FBodyInstance* Chest=Soldier->GetBodyInstance(TEXT("spine_03")))
    {
        const FVector Old=Chest->GetUnrealWorldAngularVelocityInRadians();
        const FVector Safe=Old.ContainsNaN()?FVector::ZeroVector:Old;
        Chest->SetAngularVelocityInRadians((Safe-GetActorRightVector()*2.6f).GetClampedToMaxSize(7.f),false,false);
    }
    Soldier->WakeAllRigidBodies();
    UE_LOG(LogTemp,Display,TEXT("WF_V41_FUEL_CARRIER_IMPULSE squad=%d delta_cm_s=%s once=1"),SquadId,*DeathState.FuelRuptureDeltaV.ToString());
}

void AWarfareFrontlineSoldier::BeginDeathPhysics()
{
    if (DeathState.bPhysicsStarted || DeathState.bMissingPhysics || !Soldier) return;
    Soldier->bPauseAnims=true;
    // Publish any in-flight parallel animation before reading bone transforms.
    // SetPhysicsAsset refreshes them internally too; both shape construction and
    // body initialization must use the same pose, especially on immediate blasts.
    // RefreshBoneTransforms returns immediately after completing a previously
    // queued evaluation. Complete that task explicitly, then evaluate/publish the
    // stopped pose on this thread before constructing shapes and joint frames.
    Soldier->HandleExistingParallelEvaluationTask(true,true);
    Soldier->RefreshBoneTransforms();
    if (auto* Animation=Cast<UWarfareFrontlineAnimation>(Soldier->GetAnimInstance()))
    {
        Animation->FreezeForRagdoll();
        Soldier->RefreshBoneTransforms();
    }
    const TArray<FTransform> BeforeAssetPose=Soldier->GetComponentSpaceTransforms();
    if (DeathState.bAuthoredFall)
        DeathState.MaximumHandoffPosePredictionErrorCm=FMath::Max(float(FVector::Dist(Soldier->GetBoneLocation(TEXT("Head")),DeathState.PredictedFittedHead)),
            float(FVector::Dist(Soldier->GetBoneLocation(TEXT("pelvis")),DeathState.PredictedFittedPelvis)));
    const EFrontlineDamageKind Kind=DeathState.Kind;
    const FVector Direction=DeathState.IncomingDirection;
    // The authored fall already dissipates walking momentum through foot/ground
    // contact. Reapplying the standing velocity to its grounded end pose creates a
    // second collision impulse. Explosions start from their actual moving pose.
    const FVector InheritedVelocity=DeathState.bAuthoredFall?FVector::ZeroVector:DeathState.InheritedVelocity;
    DeathPhysicsAsset=MakeDeathAsset(Soldier,this,DeathState.bAuthoredFall);
    if (DeathPhysicsAsset)
    {
        // SkeletalMeshComponent copies these component iteration values over the
        // asset defaults when creating each body. Set both levels explicitly.
        Soldier->BodyInstance.SetOverrideIterationCounts(true);
        Soldier->BodyInstance.SetPositionSolverIterationCount(16);
        Soldier->BodyInstance.SetVelocitySolverIterationCount(8);
        Soldier->SetPhysicsAsset(DeathPhysicsAsset,true);
        // SetPhysicsAsset rebuilds bodies after a forced full-bone evaluation,
        // then invalidates RequiredBones again. Finish that recalculation while
        // collision is still disabled and the bodies are kinematic. Otherwise a
        // first-frame animation refresh can replace leaf-bone locals only after
        // the new dynamic bodies have already advanced.
        Soldier->SetAllBodiesSimulatePhysics(false);
        Soldier->RefreshBoneTransforms();
        if (FPhysScene* Scene=GetWorld()->GetPhysicsScene()) Scene->ClearPreSimKinematicUpdate(Soldier);
        Soldier->UpdateKinematicBonesToAnim(Soldier->GetComponentSpaceTransforms(),ETeleportType::TeleportPhysics,true,EAllowKinematicDeferral::DisallowDeferral);
        const TArray<FTransform>& InitializedPose=Soldier->GetComponentSpaceTransforms();
        for (int32 BoneIndex=0;BoneIndex<FMath::Min(BeforeAssetPose.Num(),InitializedPose.Num());++BoneIndex)
            DeathState.InitializationBoneShiftCm=FMath::Max(DeathState.InitializationBoneShiftCm,
                float(FVector::Dist(BeforeAssetPose[BoneIndex].GetLocation(),InitializedPose[BoneIndex].GetLocation())));
        for (const FBodyInstance* Body:Soldier->Bodies) if (Body && Body->IsValidBodyInstance())
            DeathState.InitializedBodyToBoneErrorCm=FMath::Max(DeathState.InitializedBodyToBoneErrorCm,
                float(FVector::Dist(Body->GetUnrealWorldTransform().GetLocation(),Soldier->GetBoneTransform(Body->InstanceBoneIndex).GetLocation())));
        UE_LOG(LogTemp,Display,TEXT("WF_DEATH_INITIALIZATION_POSE squad=%d member=%d kind=%d bone_recalculation_shift_cm=%.5f initialized_body_error_cm=%.5f required_bones=%d"),
            SquadId,MemberIndex,int32(Kind),DeathState.InitializationBoneShiftCm,DeathState.InitializedBodyToBoneErrorCm,Soldier->RequiredBones.Num());
        Soldier->SetCollisionObjectType(ECC_PhysicsBody);
        Soldier->SetCollisionResponseToAllChannels(ECR_Ignore);
        Soldier->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
        Soldier->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
        Soldier->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        Soldier->SetEnableGravity(true);
        Soldier->SetSimulatePhysics(true);
        Soldier->SetAllBodiesSimulatePhysics(true);
        Soldier->SetAllBodiesPhysicsBlendWeight(1.f);
        for (FBodyInstance* Body:Soldier->Bodies) if (Body && Body->IsValidBodyInstance())
        {
            Body->SetOverrideIterationCounts(true);
            Body->SetPositionSolverIterationCount(16); Body->SetVelocitySolverIterationCount(8);
            Body->SetMaxAngularVelocityInRadians(!IsFrontlineExplosiveDamage(Kind)?7.f:9.f,false);
            Body->SetMaxDepenetrationVelocity(100.f);
            Body->SetInertiaConditioningEnabled(true);
        }
        AuditActualBodies(Soldier);
        if(const FBodyInstance* InitialPelvisBody=Soldier->GetBodyInstance(TEXT("pelvis")))
            if(const FBodyInstance* InitialChestBody=Soldier->GetBodyInstance(TEXT("spine_03")))
            {
                float PelvisFloor=0,ChestFloor=0;
                const FVector PelvisAt=InitialPelvisBody->GetUnrealWorldTransform().GetLocation(),ChestAt=InitialChestBody->GetUnrealWorldTransform().GetLocation();
                DeathState.bInitialNativeGroundRead=WarfareFRRUWorldV82::TryHeight(Battle,PelvisAt.X,PelvisAt.Y,PelvisFloor)
                    && WarfareFRRUWorldV82::TryHeight(Battle,ChestAt.X,ChestAt.Y,ChestFloor);
                if(DeathState.bInitialNativeGroundRead)
                {
                    DeathState.InitialPelvisTerrainClearanceCm=BodyTerrainClearance(InitialPelvisBody);
                    DeathState.InitialChestTerrainClearanceCm=BodyTerrainClearance(InitialChestBody);
                }
            }
        DeathState.HandoffTerrainClearanceCm=MAX_flt;
        for (const FBodyInstance* Body:Soldier->Bodies) if (Body && Body->IsValidBodyInstance())
        {
            const float Clearance=BodyTerrainClearance(Body);
            if (Clearance<DeathState.HandoffTerrainClearanceCm)
            {
                DeathState.HandoffTerrainClearanceCm=Clearance;
                DeathState.HandoffLowestBody=Soldier->GetBoneName(Body->InstanceBoneIndex);
            }
        }
        UE_LOG(LogTemp,Display,TEXT("WF_DEATH_HANDOFF_GROUND squad=%d member=%d kind=%d predicted_unfitted_cm=%.3f actual_min_cm=%.3f lowest=%s fitted_degrees=%.3f fitted_lift_cm=%.3f pose_prediction_error_cm=%.4f"),
            SquadId,MemberIndex,int32(Kind),DeathState.PredictedUnfittedClearanceCm,DeathState.HandoffTerrainClearanceCm,*DeathState.HandoffLowestBody.ToString(),
            DeathState.TerrainFitAngleDegrees,DeathState.TerrainFitLiftCm,DeathState.MaximumHandoffPosePredictionErrorCm);
        Soldier->SetAllPhysicsLinearVelocity(InheritedVelocity);
        Soldier->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        DeathState.BodyCount=Soldier->Bodies.Num();
        DeathState.ConstraintCount=Soldier->Constraints.Num();
        DeathState.bPhysicsStarted=true; DeathState.PhysicsStartedAt=DeathState.Age;
        // Preserve actual pre-physics bone points relative to their physical owner.
        // Later samples compare the rendered skeleton with the now-moving body,
        // so legitimate falling is not mistaken for a handover teleport.
        const FReferenceSkeleton& Ref=Soldier->GetSkeletalMeshAsset()->GetRefSkeleton();
        for (const FName BoneName:{FName(TEXT("pelvis")),FName(TEXT("Head")),FName(TEXT("foot_l")),FName(TEXT("foot_r"))})
        {
            int32 BoneIndex=Ref.FindBoneIndex(BoneName);
            while (BoneIndex!=INDEX_NONE && DeathPhysicsAsset->FindBodyIndex(Ref.GetBoneName(BoneIndex))==INDEX_NONE)
                BoneIndex=Ref.GetParentIndex(BoneIndex);
            if (BoneIndex==INDEX_NONE) continue;
            FFrontlineDeathVisualBinding Binding; Binding.Bone=BoneName; Binding.PhysicsBody=Ref.GetBoneName(BoneIndex);
            if (const FBodyInstance* Body=Soldier->GetBodyInstance(Binding.PhysicsBody))
            {
                Binding.BodyLocalPoint=Body->GetUnrealWorldTransform().InverseTransformPosition(Soldier->GetBoneLocation(BoneName));
                DeathState.VisualBindings.Add(Binding);
            }
        }
        if (!IsFrontlineExplosiveDamage(Kind) && !DeathState.bAuthoredFall)
        {
            // A bullet transfers modest momentum to the torso; it cannot launch a man.
            // The visible fall is loss of support under gravity, retaining walking inertia.
            if (FBodyInstance* Chest=Soldier->GetBodyInstance(TEXT("spine_03")))
                Soldier->AddImpulse(Direction*Chest->GetBodyMass()*(Kind==EFrontlineDamageKind::HeavyBullet?58.f:18.f),TEXT("spine_03"),false);
        }
        else if (IsFrontlineExplosiveDamage(Kind))
        {
            const float Strength=DeathState.ForceScale;
            const bool Artillery=Kind==EFrontlineDamageKind::Artillery;
            const uint32 Identity=uint32(SquadId)*17u+uint32(MemberIndex)*7u+uint32(Team)*11u;
            FVector Blast=FrontlineFieldKitV31::BlastVelocity(Direction,Artillery,Strength,Identity);
            // Radial source direction stays authoritative. Body orientation,
            // unequal footing and blast elevation vary the launch, rather
            // than sending a whole squad through one identical upward arc.
            const float Near=Strength*Strength;
            const FVector Side=FVector::CrossProduct(Direction.GetSafeNormal2D(),FVector::UpVector);
            Blast+=Side*(float(int32((Identity*1664525u+1013904223u)%81u)-40)*2.f*Near);
            Blast.Z*=.76f+float((Identity*1103515245u+12345u)%41u)*.009f;
            if(Direction.Z<-.25f)Blast.Z*=FMath::Lerp(1.f,.62f,FMath::Clamp(-Direction.Z,0.f,1.f));
            Blast=Blast.GetClampedToMaxSize(950.f);
            Soldier->SetAllPhysicsLinearVelocity(Blast,true);
        }
        ApplyFuelRuptureImpulseV41();
        Soldier->WakeAllRigidBodies();
        // FinalizeBoneTransform broadcasts after the component-space buffers are
        // flipped. During normal gameplay physics blends finalize in EndPhysics.
        // A manual RefreshBoneTransforms can also broadcast in PrePhysics, so
        // record and exclude those callbacks from this explicitly published probe.
        DeathState.BoneFinalizedHandle=Soldier->RegisterOnBoneTransformsFinalizedDelegate(
            FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateWeakLambda(this,[this]()
            {
                if (!Soldier || !DeathState.bPhysicsStarted || !GetWorld()) return;
                const int32 PublishedGroup=int32(GetWorld()->TickGroup.GetValue());
                if (PublishedGroup<int32(TG_EndPhysics)) { ++DeathState.NonPhysicsFinalizations; return; }
                if (DeathState.LastPublishedFrame==GFrameCounter) return;
                DeathState.LastPublishedFrame=GFrameCounter;
                // Retain the first published pose and all actual physics
                // blending. Only redundant readback of sleeping bodies stops.
                const bool SkipSleepingAudit=!OldDeathAudits(Battle)&&SleepingDeathAuditReady(this);
                CountDeathAudit(this,true,SkipSleepingAudit);
                if(SkipSleepingAudit)return;
                float Error=0; FName WorstBone; FVector ExpectedWorst=FVector::ZeroVector,VisibleWorst=FVector::ZeroVector;
                for (const FFrontlineDeathVisualBinding& Binding:DeathState.VisualBindings)
                    if (const FBodyInstance* Body=Soldier->GetBodyInstance(Binding.PhysicsBody))
                    {
                        const FVector Expected=Body->GetUnrealWorldTransform().TransformPosition(Binding.BodyLocalPoint);
                        const FVector Visible=Soldier->GetBoneLocation(Binding.Bone);
                        const float Difference=float(FVector::Dist(Expected,Visible));
                        if (Difference>Error) { Error=Difference; WorstBone=Binding.Bone; ExpectedWorst=Expected; VisibleWorst=Visible; }
                    }
                if (DeathState.VisualBindings.IsEmpty()) return;
                const bool First=DeathState.PublishedPoseSamples++==0;
                if (First)
                {
                    DeathState.FirstPublishedPoseErrorCm=Error;
                    DeathState.FirstPublishedTickGroup=PublishedGroup;
                    for (const FFrontlineDeathVisualBinding& Binding:DeathState.VisualBindings)
                        if (const FBodyInstance* Body=Soldier->GetBodyInstance(Binding.PhysicsBody))
                        {
                            const int32 LeafIndex=Soldier->GetBoneIndex(Binding.Bone);
                            const FTransform VisualBody=Soldier->GetBoneTransform(Soldier->GetBoneIndex(Binding.PhysicsBody));
                            const FTransform PhysicalBody=Body->GetUnrealWorldTransform();
                            const FVector VisualLocal=VisualBody.InverseTransformPosition(Soldier->GetBoneLocation(Binding.Bone));
                            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_FIRST_LEAF_SPACE squad=%d member=%d bone=%s body=%s lod=%d required=%d fill=%d required_count=%d parent_position_error_cm=%.5f parent_angle_error_deg=%.5f leaf_local_error_cm=%.5f initial_local=%s published_local=%s"),
                                SquadId,MemberIndex,*Binding.Bone.ToString(),*Binding.PhysicsBody.ToString(),Soldier->GetPredictedLODLevel(),
                                int32(Soldier->RequiredBones.Contains(FBoneIndexType(LeafIndex))),int32(Soldier->FillComponentSpaceTransformsRequiredBones.Contains(FBoneIndexType(LeafIndex))),Soldier->RequiredBones.Num(),
                                FVector::Dist(VisualBody.GetLocation(),PhysicalBody.GetLocation()),FMath::RadiansToDegrees(VisualBody.GetRotation().AngularDistance(PhysicalBody.GetRotation())),
                                FVector::Dist(VisualLocal,Binding.BodyLocalPoint),*Binding.BodyLocalPoint.ToString(),*VisualLocal.ToString());
                        }
                }
                if (First || Error>DeathState.MaximumPublishedPoseErrorCm+.25f)
                    UE_LOG(LogTemp,Display,TEXT("WF_DEATH_PUBLISHED_POSE squad=%d member=%d kind=%d age=%.3f frame=%llu group=%d first=%d error_cm=%.5f bone=%s expected=%s visible=%s prior_actor_tick_max_cm=%.5f"),
                        SquadId,MemberIndex,int32(DeathState.Kind),DeathState.Age,GFrameCounter,PublishedGroup,int32(First),Error,
                        *WorstBone.ToString(),*ExpectedWorst.ToString(),*VisibleWorst.ToString(),DeathState.MaximumPhysicalPoseErrorCm);
                DeathState.LastPublishedPoseErrorCm=Error;
                DeathState.MaximumPublishedPoseErrorCm=FMath::Max(DeathState.MaximumPublishedPoseErrorCm,Error);
            }));
        UE_LOG(LogTemp,Display,TEXT("WF_DEATH_BEGIN squad=%d member=%d kind=%d bodies=%d constraints=%d incoming=%s inherited_cm_s=%.2f"),
            SquadId,MemberIndex,int32(Kind),DeathState.BodyCount,DeathState.ConstraintCount,*Direction.ToString(),InheritedVelocity.Size());
    }
    else
    {
        DeathState.bMissingPhysics=true;
        UE_LOG(LogTemp,Error,TEXT("WF_DEATH_MISSING_PHYSICS squad=%d member=%d: casualty remains dead; expected V11 soldier skeleton/physics source unavailable."),SquadId,MemberIndex);
    }
}

void AWarfareFrontlineSoldier::TickDeath(float DT)
{
    if (!DeathState.bStarted || DeathState.bMissingPhysics || DT<=0 || !Soldier) return;
    DeathState.Age+=DT; ++DeathState.Frames;
    if (DeathState.bSettled)
    {
        const FVector Pelvis=PelvisPosition(Soldier);
        if(Pelvis.ContainsNaN()) { DeathState.bNonFinite=true; return; }
        DeathState.MaximumSettledDriftCm=FMath::Max(DeathState.MaximumSettledDriftCm,float(FVector::Dist(Pelvis,DeathState.SettledPelvis)));
        // Sleeping bodies retain their solved pose. Resume the full contact
        // inspection only when a later physical interaction wakes this corpse.
        if(!Soldier->IsAnyRigidBodyAwake()) return;
        DeathState.bSettled=false;DeathState.QuietSeconds=0.f;
    }
    if (!DeathState.bPhysicsStarted)
    {
        if (DeathState.PendingPhysicsFrame!=MAX_uint64 && GFrameCounter>DeathState.PendingPhysicsFrame)
        {
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_HANDOFF_PREPHYSICS squad=%d member=%d queued_frame=%llu frame=%llu group=%d"),
                SquadId,MemberIndex,static_cast<unsigned long long>(DeathState.PendingPhysicsFrame),
                static_cast<unsigned long long>(GFrameCounter),int32(GetWorld()->TickGroup.GetValue()));
            BeginDeathPhysics();
            DeathState.PendingPhysicsFrame=MAX_uint64;
            return;
        }
        const UAnimSingleNodeInstance* Node=Soldier->GetSingleNodeInstance();
        if (DeathState.bAuthoredFall && Node) ApplyDeathTerrainFit(Soldier,DeathState,Node->GetCurrentTime());
        if (DeathState.bAuthoredFall && Node && Node->GetCurrentTime()>=DeathState.AuthoredClipSeconds-.001f)
            BeginDeathPhysics();
        return;
    }
    float MaxSpeed=0,MaxAngular=0; FName PeakAngularBone;
    const bool SkipSleepingAudit=!OldDeathAudits(Battle)&&SleepingDeathAuditReady(this);
    CountDeathAudit(this,false,SkipSleepingAudit);
    if(!SkipSleepingAudit)
    {
    float VisualError=0;
    for (const FFrontlineDeathVisualBinding& Binding:DeathState.VisualBindings)
        if (const FBodyInstance* Body=Soldier->GetBodyInstance(Binding.PhysicsBody))
        {
            const FVector Expected=Body->GetUnrealWorldTransform().TransformPosition(Binding.BodyLocalPoint);
            VisualError=FMath::Max(VisualError,float(FVector::Dist(Expected,Soldier->GetBoneLocation(Binding.Bone))));
        }
    if (!DeathState.VisualBindings.IsEmpty())
    {
        ++DeathState.PhysicalPoseSamples;
        if (DeathState.FirstPhysicalPoseErrorCm<0)
        {
            DeathState.FirstPhysicalPoseErrorCm=VisualError;
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_FIRST_PHYSICAL_POSE squad=%d member=%d kind=%d age=%.3f bindings=%d error_cm=%.4f"),
                SquadId,MemberIndex,int32(DeathState.Kind),DeathState.Age,DeathState.VisualBindings.Num(),VisualError);
        }
        DeathState.MaximumPhysicalPoseErrorCm=FMath::Max(DeathState.MaximumPhysicalPoseErrorCm,VisualError);
    }
    const float LinearLimit=!IsFrontlineExplosiveDamage(DeathState.Kind)?700.f:950.f;
    const float AngularLimit=!IsFrontlineExplosiveDamage(DeathState.Kind)?7.f:9.f;
    for (FBodyInstance* Body:Soldier->Bodies) if (Body && Body->IsValidBodyInstance())
    {
        const FVector Velocity=Body->GetUnrealWorldVelocity(),Angular=Body->GetUnrealWorldAngularVelocityInRadians();
        if (Velocity.ContainsNaN() || Angular.ContainsNaN() || Body->GetUnrealWorldTransform().ContainsNaN())
        {
            DeathState.bNonFinite=true;
            Body->SetLinearVelocity(FVector::ZeroVector,false,false);
            Body->SetAngularVelocityInRadians(FVector::ZeroVector,false,false);
            continue;
        }
        MaxSpeed=FMath::Max(MaxSpeed,float(Velocity.Size()));
        if (Angular.Size()>MaxAngular)
        {
            MaxAngular=float(Angular.Size()); PeakAngularBone=Soldier->GetBoneName(Body->InstanceBoneIndex);
        }
        if (SquadId<=2 && Angular.Size()>DeathState.MaximumAngularSpeedRadS+5.f)
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_ENERGY_SOURCE squad=%d member=%d age=%.3f bone=%s angular=%.3f limit=%.3f mass=%.3f inertia=%s iterations=%d/%d"),
                SquadId,MemberIndex,DeathState.Age,*Soldier->GetBoneName(Body->InstanceBoneIndex).ToString(),Angular.Size(),
                Body->GetMaxAngularVelocityInRadians(),Body->GetBodyMass(),*Body->GetBodyInertiaTensor().ToString(),
                Body->GetPositionSolverIterationCount(),Body->GetVelocitySolverIterationCount());
        if (Velocity.SizeSquared()>FMath::Square(LinearLimit))
        {
            ++DeathState.VelocityLimitEvents;
            Body->SetLinearVelocity(Velocity.GetClampedToMaxSize(LinearLimit),false,false);
        }
        if (Angular.SizeSquared()>FMath::Square(AngularLimit))
        {
            ++DeathState.VelocityLimitEvents;
            Body->SetAngularVelocityInRadians(Angular.GetClampedToMaxSize(AngularLimit),false,false);
        }
        // Increased damping only after the initial fall; do not turn gravity into syrup.
        const float LinearDamping=DeathState.Age>2.f?.9f:.25f;
        const float AngularDamping=DeathState.Age>2.f?6.f:3.f;
        if (Body->LinearDamping!=LinearDamping)
        {
            Body->LinearDamping=LinearDamping; Body->AngularDamping=AngularDamping;
            Body->UpdateDampingProperties();
        }
    }
    DeathState.CurrentBodySpeedCmS=MaxSpeed; DeathState.CurrentAngularSpeedRadS=MaxAngular;
    DeathState.MaximumBodySpeedCmS=FMath::Max(DeathState.MaximumBodySpeedCmS,MaxSpeed);
    if (MaxAngular>14.f && MaxAngular>DeathState.MaximumAngularSpeedRadS+.5f)
    {
        const FBodyInstance* PeakBody=Soldier->GetBodyInstance(PeakAngularBone);
        UE_LOG(LogTemp,Display,TEXT("WF_DEATH_PEAK_CONTACT squad=%d member=%d kind=%d age=%.3f bone=%s raw_angular=%.3f raw_speed=%.3f terrain_support_clearance_cm=%.3f"),
            SquadId,MemberIndex,int32(DeathState.Kind),DeathState.Age,*PeakAngularBone.ToString(),MaxAngular,MaxSpeed,PeakBody?BodyTerrainClearance(PeakBody):MAX_flt);
        for (const FConstraintInstance* Joint:Soldier->Constraints) if (Joint && (Joint->ConstraintBone1==PeakAngularBone || Joint->ConstraintBone2==PeakAngularBone))
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_PEAK_JOINT squad=%d member=%d age=%.3f peak_body=%s joint=%s swing1_deg=%.3f limit1_deg=%.3f swing2_deg=%.3f limit2_deg=%.3f twist_deg=%.3f twist_limit_deg=%.3f"),
                SquadId,MemberIndex,DeathState.Age,*PeakAngularBone.ToString(),*Joint->JointName.ToString(),FMath::RadiansToDegrees(Joint->GetCurrentSwing1()),Joint->GetAngularSwing1Limit(),
                FMath::RadiansToDegrees(Joint->GetCurrentSwing2()),Joint->GetAngularSwing2Limit(),FMath::RadiansToDegrees(Joint->GetCurrentTwist()),Joint->GetAngularTwistLimit());
    }
    if (MaxAngular>DeathState.MaximumAngularSpeedRadS) DeathState.PeakAngularBody=PeakAngularBone;
    DeathState.MaximumAngularSpeedRadS=FMath::Max(DeathState.MaximumAngularSpeedRadS,MaxAngular);
    for (const FConstraintInstance* Joint:Soldier->Constraints) if (Joint)
    {
        if (Joint->JointName==TEXT("lowerarm_r"))
        {
            const float Swing1Excess=FMath::Abs(FMath::RadiansToDegrees(Joint->GetCurrentSwing1()))-Joint->GetAngularSwing1Limit();
            const float Swing2Excess=FMath::Abs(FMath::RadiansToDegrees(Joint->GetCurrentSwing2()))-Joint->GetAngularSwing2Limit();
            const float TwistExcess=FMath::Abs(FMath::RadiansToDegrees(Joint->GetCurrentTwist()))-Joint->GetAngularTwistLimit();
            DeathState.MaximumRightElbowLimitExcessDegrees=FMath::Max(DeathState.MaximumRightElbowLimitExcessDegrees,FMath::Max3(Swing1Excess,Swing2Excess,TwistExcess));
        }
        const FBodyInstance* A=Soldier->GetBodyInstance(Joint->ConstraintBone1);
        const FBodyInstance* B=Soldier->GetBodyInstance(Joint->ConstraintBone2);
        if (A && B)
        {
            const FVector PA=A->GetUnrealWorldTransform().TransformPosition(Joint->GetRefFrame(EConstraintFrame::Frame1).GetLocation()*Joint->GetLastKnownScale());
            const FVector PB=B->GetUnrealWorldTransform().TransformPosition(Joint->GetRefFrame(EConstraintFrame::Frame2).GetLocation()*Joint->GetLastKnownScale());
            const float Separation=FVector::Dist(PA,PB);
            if (FMath::IsFinite(Separation)) DeathState.MaximumJointSeparationCm=FMath::Max(DeathState.MaximumJointSeparationCm,Separation);
            else DeathState.bNonFinite=true;
        }
    }
    }
    // Chaos sleep guarantees zero solved velocity. Keep the original pelvis,
    // cosmetics and settlement path below, including upright-sleeper rejection.
    DeathState.CurrentBodySpeedCmS=MaxSpeed;DeathState.CurrentAngularSpeedRadS=MaxAngular;
    const FVector Pelvis=PelvisPosition(Soldier);
    if (Pelvis.ContainsNaN()) { DeathState.bNonFinite=true; return; }
    DeathState.MaximumPelvisTravelCm=FMath::Max(DeathState.MaximumPelvisTravelCm,float(FVector::Dist2D(Pelvis,DeathState.InitialPelvis)));
    DeathState.MaximumPelvisRiseCm=FMath::Max(DeathState.MaximumPelvisRiseCm,float(Pelvis.Z-DeathState.InitialPelvis.Z));
    // Loss-of-limb appearance is a separately recorded, bounded post-solve
    // layer. The full hidden skeleton and all physical probes remain intact.
    if(DeathState.PublishedPoseSamples>0)
    {
        FrontlineFieldKitV31::ReleaseHelmet(this);
    }
    if (DeathState.bSettled)
    {
        DeathState.MaximumSettledDriftCm=FMath::Max(DeathState.MaximumSettledDriftCm,float(FVector::Dist(Pelvis,DeathState.SettledPelvis)));
        return;
    }
    const bool PhysicallyQuiet=DeathState.Age>.8f && MaxSpeed<5.f && MaxAngular<.22f;
    bool ProneOnGround=false;
    // The national ground-fire clips do not use the legacy gun component's
    // prone flag. Judge their solved torso and both actual terrain contacts;
    // an upright or suspended body cannot satisfy these physical conditions.
    if(PhysicallyQuiet&&WarfareFRRUCampaignV82::Active(Battle))
    {
        const FBodyInstance* PelvisBody=Soldier->GetBodyInstance(TEXT("pelvis"));
        const FBodyInstance* ChestBody=Soldier->GetBodyInstance(TEXT("spine_03"));
        if(PelvisBody&&ChestBody&&PelvisBody->IsValidBodyInstance()&&ChestBody->IsValidBodyInstance()
            &&PelvisBody->IsInstanceSimulatingPhysics()&&ChestBody->IsInstanceSimulatingPhysics())
        {
            const FVector P=PelvisBody->GetUnrealWorldTransform().GetLocation(),C=ChestBody->GetUnrealWorldTransform().GetLocation();
            const FVector Torso=C-P;float PF=0,CF=0;
            if(!Torso.ContainsNaN()&&Torso.SizeSquared()>15.f*15.f&&FMath::Abs(Torso.GetSafeNormal().Z)<.45f
                &&WarfareFRRUWorldV82::TryHeight(Battle,P.X,P.Y,PF)&&WarfareFRRUWorldV82::TryHeight(Battle,C.X,C.Y,CF))
            {
                const float PC=BodyTerrainClearance(PelvisBody),CC=BodyTerrainClearance(ChestBody);
                // A solved prone torso can rest on its chest while the pelvis
                // sits a few centimetres above uneven ground. Requiring both
                // bodies to touch kept already sleeping MG casualties active
                // forever. Keep the real non-penetration, lying-axis and
                // whole-body quiet gates; do not lower or reanimate the body.
                ProneOnGround=FMath::IsFinite(PC)&&FMath::IsFinite(CC)
                    &&PC>=-.5f&&CC>=-.5f&&FMath::Min(PC,CC)<=2.5f&&FMath::Max(PC,CC)<=8.f;
            }
        }
    }
    const bool HasFallen=Pelvis.Z<DeathState.InitialPelvis.Z-25.f||ProneOnGround;
    if (!SkipSleepingAudit && DeathState.Age>6.f && DeathState.Age-DeathState.LastDiagnosticAt>=2.f)
    {
        DeathState.LastDiagnosticAt=DeathState.Age;
        int32 Awake=0; for (const FBodyInstance* Body:Soldier->Bodies) if (Body && Body->IsInstanceAwake()) ++Awake;
        UE_LOG(LogTemp,Display,TEXT("WF_DEATH_UNSETTLED squad=%d member=%d age=%.3f speed=%.3f angular=%.3f pelvis_drop_cm=%.3f fallen=%d awake=%d quiet=%.3f ground={%s}"),
            SquadId,MemberIndex,DeathState.Age,MaxSpeed,MaxAngular,DeathState.InitialPelvis.Z-Pelvis.Z,HasFallen,Awake,DeathState.QuietSeconds,*DeathGroundReadback(Soldier,Battle,DeathState));
    }
    const bool Quiet=HasFallen && PhysicallyQuiet;
    DeathState.QuietSeconds=Quiet?DeathState.QuietSeconds+DT:0.f;
    if (DeathState.QuietSeconds>=.5f)
    {
        // Sleep the solved physical pose, retaining it. Disabling simulation would snap
        // the mesh back to its animation and hide the defect instead of settling it.
        Soldier->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
        Soldier->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        Soldier->PutAllRigidBodiesToSleep();
        DeathState.bSettled=true; DeathState.SettledAt=DeathState.Age; DeathState.SettledPelvis=Pelvis;
        UE_LOG(LogTemp,Display,TEXT("WF_DEATH_SETTLED squad=%d member=%d kind=%d seconds=%.3f peak_speed=%.2f peak_angular=%.3f separation_cm=%.3f travel_cm=%.2f rise_cm=%.2f limit_events=%d peak_angular_bone=%s ground={%s}"),
            SquadId,MemberIndex,int32(DeathState.Kind),DeathState.SettledAt,DeathState.MaximumBodySpeedCmS,DeathState.MaximumAngularSpeedRadS,
            DeathState.MaximumJointSeparationCm,DeathState.MaximumPelvisTravelCm,DeathState.MaximumPelvisRiseCm,DeathState.VelocityLimitEvents,*DeathState.PeakAngularBody.ToString(),*DeathGroundReadback(Soldier,Battle,DeathState));
        if (IsFrontlineExplosiveDamage(DeathState.Kind))
            UE_LOG(LogTemp,Display,TEXT("WF_DEATH_ELBOW_LIMIT_OBSERVATION squad=%d member=%d maximum_sampled_axis_excess_degrees=%.4f"),SquadId,MemberIndex,DeathState.MaximumRightElbowLimitExcessDegrees);
    }
}

// Called by controller BeginPlay: Shipping need not execute console commands.
// Ordinary launches register no ticker; only the isolated explicit fixture waits.
void StartFrontlineDeathSleepReviewV8213(UWorld* World)
{
    if(!World||!DeathSleepReviewRequested())return;
    static TWeakObjectPtr<UWorld> QueuedWorld;
    if(QueuedWorld.Get()==World)return;
    QueuedWorld=World;
    const TWeakObjectPtr<UWorld> WeakWorld(World);
    const double QueuedAt=FPlatformTime::Seconds();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld,QueuedAt](float)
    {
        auto* W=WeakWorld.Get();if(!W)return false;
        auto* B=W->GetAuthGameMode<AWarfareFrontlineGameMode>();
        if(B&&B->HasActorBegunPlay()&&B->LoadedSoldier&&B->IsProgressionVolatile()&&B->AssetFailure.IsEmpty())
        {StartDeathSleepReview(W);return false;}
        if(FPlatformTime::Seconds()-QueuedAt<20.)return true;
        auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),false);
        Report->SetStringField(TEXT("failure"),B&&!B->AssetFailure.IsEmpty()?B->AssetFailure:TEXT("Isolated death fixture startup did not become ready."));
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Gameplay/V8213/DeathSleepReview");
        IFileManager::Get().MakeDirectory(*Directory,true);FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("review.json")));
        FPlatformMisc::RequestExit(false);return false;
    }),2.f);
}

