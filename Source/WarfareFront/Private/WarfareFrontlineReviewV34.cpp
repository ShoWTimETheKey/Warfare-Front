#include "WarfareFrontlineReviewV34.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineAppearanceV29.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/MorphTarget.h"
#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSafeCounter.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RenderTimer.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
TSharedRef<FJsonObject> InspectFaceMorphLODsV29(const USkeletalMesh* Mesh)
{
    auto Report=MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("mesh"),GetPathNameSafe(Mesh));
    const TCHAR* Channels[]={TEXT("WFV29_BlinkLeft"),TEXT("WFV29_BlinkRight"),TEXT("WFV29_BrowTension"),
        TEXT("WFV29_JawSet"),TEXT("WFV29_CheekLean"),TEXT("WFV29_JawBroad")};
    bool Complete=Mesh && Mesh->GetLODNum()>=4;
    TArray<TSharedPtr<FJsonValue>> ChannelReports;
    for(const TCHAR* Channel:Channels)
    {
        UMorphTarget* Found=nullptr;int32 Matches=0;
        if(Mesh)for(const auto& Morph:Mesh->GetMorphTargets())
            if(Morph && (Morph->GetName()==Channel || Morph->GetName().EndsWith(FString(TEXT("_"))+Channel)))
            {Found=Morph.Get();++Matches;}
        auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("channel"),Channel);
        C->SetStringField(TEXT("resolved_morph"),GetNameSafe(Found));C->SetNumberField(TEXT("matching_channels"),Matches);
        TArray<TSharedPtr<FJsonValue>> LODReports;
        for(int32 LOD=0;LOD<(Mesh?Mesh->GetLODNum():0);++LOD)
        {
            // UE 5.8 can keep cooked morph deltas only in compressed GPU data.
            // Raw Vertices.Num() / editor-only GetMorphTargetDeltas would then
            // falsely report an empty morph in the packaged game.
            const int32 Deltas=Found?Found->GetNumDeltasForLOD(LOD):0;
            const bool HasData=Found && Found->HasDataForLOD(LOD);
            auto L=MakeShared<FJsonObject>();L->SetNumberField(TEXT("lod"),LOD);
            L->SetNumberField(TEXT("vertex_delta_count"),Deltas);L->SetBoolField(TEXT("has_data"),HasData);
            LODReports.Add(MakeShared<FJsonValueObject>(L));Complete&=Matches==1 && HasData && Deltas>0;
        }
        C->SetArrayField(TEXT("lods"),LODReports);ChannelReports.Add(MakeShared<FJsonValueObject>(C));
    }
    Report->SetArrayField(TEXT("channels"),ChannelReports);Report->SetBoolField(TEXT("all_six_channels_present_at_all_lods"),Complete);
    Report->SetStringField(TEXT("count_source"),TEXT("UMorphTarget::GetNumDeltasForLOD and HasDataForLOD; supports cooked GPU-only morph storage"));
    return Report;
}
}

// A labelled actual-renderer fixture, followed by a short unmodified recruitment
// flow. Numeric success means evidence is complete, never aesthetic acceptance.
void StartFrontlineV34Review(AWarfareFrontlineController* Controller)
{
    FString UserDir;
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV34Review")) ||
        !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Interrupts=0,Samples=0;bool Ready=true,Canary=false,Focused=false,FocusedLOD3Ready=false;
        double Started=FPlatformTime::Seconds(),Next=Started+1,StageAt=0,Last=0,InitialFixtureBuildSeconds=0;
        FString Folder,Failure,RequestedFile,CapturedFile;TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Men;
        TArray<TSharedPtr<FJsonValue>> Captures;TArray<double> WallMs,GPUMs;
        FDelegateHandle Screenshot;FThreadSafeCounter Pending,Failed;
        float MaxGrip=0,MaxCorrection=0,MaxArmLengthError=0,MaxPersonalGripDrift=0;int32 ConstraintFrames=0,InvalidBoneFrames=0,UnsafeHierarchyFrames=0;
        bool NaturalRecruits=false,BurningSampleReady=false;int32 NaturalLiving=0;
        TMap<int32,FVector> DropOrigins;TSet<int32> DropsMoved;
        int32 MatchingPhysicalDrops=0,HiddenHeldGuns=0;
    };
    auto R=MakeShared<FRun,ESPMode::ThreadSafe>();TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    R->Canary=FParse::Param(FCommandLine::Get(),TEXT("WFV34Canary"));
    R->Focused=FParse::Param(FCommandLine::Get(),TEXT("WFV34Focused"));
    if(R->Focused&&(R->Canary||FParse::Param(FCommandLine::Get(),TEXT("WFV34Baseline"))))return;
    R->Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V34/CharacterReview")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*R->Folder,true);FModuleManager::Get().LoadModuleChecked(TEXT("ImageWrapper"));
    R->Screenshot=UGameViewportClient::OnScreenshotCaptured().AddLambda([R](int32 W,int32 H,const TArray<FColor>& Pixels)
    {
        const FString Path=FScreenshotRequest::GetFilename();if(!Path.StartsWith(R->Folder))return;
        R->CapturedFile=Path;
        TArray<FColor> Copy=Pixels;R->Pending.Increment();
        Async(EAsyncExecution::ThreadPool,[R,Path,W,H,Copy=MoveTemp(Copy)]()
        {TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(W,H,Copy,PNG);
         if(PNG.IsEmpty() || !FFileHelper::SaveArrayToFile(PNG,*Path))R->Failed.Increment();R->Pending.Decrement();});
    });
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,R](float DT)
    {
        auto* PC=Weak.Get();auto* World=PC?PC->GetWorld():nullptr;
        auto* B=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
        if(!PC || !B){UGameViewportClient::OnScreenshotCaptured().Remove(R->Screenshot);FPlatformMisc::RequestExit(false);return false;}
        const double Now=FPlatformTime::Seconds();
        if(R->Stage>=8 && R->Stage<=9)
            for(int32 I:{2,8})if(R->Men.IsValidIndex(I))if(auto* U=R->Men[I].Get())
            {
                // Pose evaluation uses world time. A PNG/readback hitch must
                // not advance the weapon clock by a different wall-time delta
                // and clear GrenadeThrow before its actual release notify.
                const float WorldDT=World->GetDeltaSeconds();
                U->TickWeaponEffects(WorldDT);U->UpdateWeaponCycle(WorldDT);
            }
        // Counter collection consumes poses published by the real animation proxy.
        for(const auto& Ref:R->Men)if(auto* U=Ref.Get())if(auto* N=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
        {R->MaxGrip=FMath::Max(R->MaxGrip,N->GetMaximumGripResidualCm());R->MaxCorrection=FMath::Max(R->MaxCorrection,N->GetMaximumGripCorrectionCm());}
        if(R->Stage==11 && Now-R->StageAt<1.6 && Now>=R->Next)
        {
            for(const auto& Ref:R->Men)if(auto* U=Ref.Get())
            {U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->bMoving=false;U->SetMotion((R->Interrupts%2)?U->GetIdleMotion():U->GetAimMotion());}
            ++R->Interrupts;R->Next=Now+.05;return true;
        }
        if(R->Stage==16 && Now-R->StageAt>1.5 && Now-R->StageAt<7.5)
        {
            if(R->Last>0){R->WallMs.Add((Now-R->Last)*1000.);const double G=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(G>0)R->GPUMs.Add(G);}
            R->Last=Now;
        }
        if(Now<R->Next)return true;
        auto InvalidateStagedViewHistory=[&]()
        {
            // These fixture-only cuts teleport the camera or replace the visible
            // roster. DLSS/RR must not reproject the previous staged composition.
            // Ordinary clip transitions and continued movement retain history.
            if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
        };
        auto Camera=[&](float Width,float X=1800,float Z=115)
        {
            const FVector Focus(X,0,Z),From=Focus+FVector(0,14000,4000);
            PC->BattleCamera->SetActorLocation(From);PC->BattleCamera->SetActorRotation((Focus-From).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);
            // Controller tick is isolated in this fixture. Preserve its existing
            // matte pan/zoom contract instead of leaving the backdrop at home.
            const double Centre=.6*X,MatteWidth=FMath::Max(12500.,Width+2*FMath::Abs(X-Centre)+400.);
            const double Height=4200.*MatteWidth/12500.;
            for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("WF_DistantMatte_V12")))
            {It->SetActorLocation(FVector(Centre,-5200,180.-.4*Height));It->SetActorScale3D(FVector(MatteWidth/100.,1,Height/100.));}
            InvalidateStagedViewHistory();
        };
        auto Show=[&](TArray<int32> Indices,float Spacing=235.f)
        {
            for(const auto& Ref:R->Men)if(auto* U=Ref.Get())U->SetActorHiddenInGame(true);
            for(int32 I=0;I<Indices.Num();++I)if(R->Men.IsValidIndex(Indices[I]))if(auto* U=R->Men[Indices[I]].Get())
            {const float X=1800+(I-(Indices.Num()-1)*.5f)*Spacing;U->SetActorHiddenInGame(false);U->SetActorLocation(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)+88));U->SetActorRotation(FRotator(0,U->Team?150:30,0));}
            InvalidateStagedViewHistory();
        };
        auto GripClose=[&](int32 Index,float Yaw)
        {
            // The roster view is useful at play scale but its hands occupy
            // only tens of pixels. Keep the real shoulder, stock and both
            // hands intact in these three diagnostic LOD0 close views.
            Show({Index});
            if(auto* U=R->Men[Index].Get())
            {
                U->SetActorTickEnabled(false);U->bMoving=false;U->MotionSpeed=0;
                U->WeaponPhase=EFrontlineWeaponPhase::Ready;U->SetMotion(U->GetAimMotion());
                U->Soldier->SetForcedLOD(1);U->SetActorRotation(FRotator(0,Yaw,0));
                TInlineComponentArray<USkeletalMeshComponent*> Components(U);
                for(auto* C:Components)if(C!=U->Soldier && GetNameSafe(C->GetSkeletalMeshAsset()).Contains(TEXT("Respirator")))C->SetVisibility(false);
            }
            Camera(240,1800,AWarfareFrontlineGround::HeightAt(1800,0)+140);
        };
        auto Snap=[&](const TCHAR* Label)
        {
            const FString File=R->Folder/(FString(Label)+TEXT(".png"));
            if(R->CapturedFile==File)return true;
            if(R->RequestedFile==File)return false;
            R->RequestedFile=File;FScreenshotRequest::RequestScreenshot(File,false,false);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),File);J->SetNumberField(TEXT("camera_width_cm"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);J->SetNumberField(TEXT("wall_seconds"),Now-R->Started);
            TArray<TSharedPtr<FJsonValue>> Visible;
            for(const auto& Ref:R->Men)if(const auto* U=Ref.Get())if(!U->IsHidden())
            {
                auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("mesh"),GetNameSafe(U->Soldier->GetSkeletalMeshAsset()));V->SetNumberField(TEXT("individual_phase"),U->GetMotionPhase());V->SetNumberField(TEXT("weapon_phase"),int32(U->WeaponPhase));
                V->SetBoolField(TEXT("alive"),U->IsAlive());V->SetNumberField(TEXT("predicted_lod"),U->Soldier->GetPredictedLODLevel());
                V->SetNumberField(TEXT("team"),U->Team);V->SetNumberField(TEXT("role"),int32(U->Role));
                if(const auto* Mesh=U->Soldier->GetSkeletalMeshAsset())
                {
                    TArray<TSharedPtr<FJsonValue>> LODs;
                    if(const auto* Data=Mesh->GetResourceForRendering())for(int32 L=0;L<Data->LODRenderData.Num();++L)
                    {
                        const auto& D=Data->LODRenderData[L];auto LJ=MakeShared<FJsonObject>();
                        LJ->SetNumberField(TEXT("lod"),L);LJ->SetNumberField(TEXT("vertices"),D.GetNumVertices());
                        LJ->SetNumberField(TEXT("sections"),D.RenderSections.Num());int64 Triangles=0;
                        for(const auto& Section:D.RenderSections)Triangles+=Section.NumTriangles;
                        LJ->SetNumberField(TEXT("triangles"),double(Triangles));LODs.Add(MakeShared<FJsonValueObject>(LJ));
                    }
                    V->SetArrayField(TEXT("cooked_lods"),LODs);
                }
                TArray<TSharedPtr<FJsonValue>> Followers;
                TInlineComponentArray<USkeletalMeshComponent*> Components(U);
                for(const auto* C:Components)if(C!=U->Soldier && C->IsVisible())
                {auto F=MakeShared<FJsonObject>();F->SetStringField(TEXT("mesh"),GetPathNameSafe(C->GetSkeletalMeshAsset()));F->SetNumberField(TEXT("lod"),C->GetPredictedLODLevel());F->SetStringField(TEXT("head_world"),C->GetSocketLocation(TEXT("Head")).ToString());Followers.Add(MakeShared<FJsonValueObject>(F));}
                V->SetArrayField(TEXT("visible_followers"),Followers);
                TArray<TSharedPtr<FJsonValue>> Drops;TInlineComponentArray<UStaticMeshComponent*> StaticParts(const_cast<AWarfareFrontlineSoldier*>(U));
                const int32 FixtureIndex=R->Men.IndexOfByPredicate([U](const auto& Ref){return Ref.Get()==U;});
                for(const auto* C:StaticParts)if(C->ComponentHasTag(TEXT("WF_V34_RELEASED_WEAPON")))
                {
                    auto D=MakeShared<FJsonObject>();D->SetStringField(TEXT("mesh"),GetPathNameSafe(C->GetStaticMesh()));
                    D->SetStringField(TEXT("world_location"),C->GetComponentLocation().ToString());D->SetBoolField(TEXT("visible"),C->IsVisible());
                    D->SetBoolField(TEXT("simulating_physics"),C->IsSimulatingPhysics());D->SetNumberField(TEXT("collision_enabled"),int32(C->GetCollisionEnabled()));
                    D->SetNumberField(TEXT("bottom_above_terrain_cm"),C->Bounds.Origin.Z-C->Bounds.BoxExtent.Z-AWarfareFrontlineGround::HeightAt(C->Bounds.Origin.X,C->Bounds.Origin.Y));
                    if(const FVector* Origin=R->DropOrigins.Find(FixtureIndex))
                    {const double Travel=FVector::Dist(*Origin,C->GetComponentLocation());D->SetNumberField(TEXT("travel_from_release_cm"),Travel);if(Travel>3.)R->DropsMoved.Add(FixtureIndex);}
                    Drops.Add(MakeShared<FJsonValueObject>(D));
                }
                V->SetArrayField(TEXT("released_weapons"),Drops);V->SetBoolField(TEXT("held_weapon_bone_hidden"),U->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle")));
                if(auto* N=U->Soldier->GetSingleNodeInstance()){V->SetStringField(TEXT("clip"),GetNameSafe(N->GetCurrentAsset()));V->SetNumberField(TEXT("clip_time"),N->GetCurrentTime());}
                V->SetBoolField(TEXT("per_bone_motion_vectors_enabled"),U->Soldier->bPerBoneMotionBlur);
                V->SetNumberField(TEXT("bone_revision"),U->Soldier->GetBoneTransformRevisionNumber());
                V->SetNumberField(TEXT("previous_bone_revision"),U->Soldier->GetPreviousBoneTransformRevisionNumber());
                const auto& CurrentPose=U->Soldier->GetComponentSpaceTransforms();
                const auto& PreviousPose=U->Soldier->GetPreviousComponentTransformsArray();
                double MaxLimbDelta=0;int32 HistoryBones=0;
                for(const TCHAR* Bone:{TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r"),TEXT("weapon_rifle")})
                {
                    const int32 Index=U->Soldier->GetBoneIndex(Bone);
                    if(CurrentPose.IsValidIndex(Index) && PreviousPose.IsValidIndex(Index))
                    {++HistoryBones;MaxLimbDelta=FMath::Max(MaxLimbDelta,FVector::Distance(CurrentPose[Index].GetLocation(),PreviousPose[Index].GetLocation()));}
                }
                V->SetNumberField(TEXT("previous_pose_sampled_bones"),HistoryBones);
                V->SetNumberField(TEXT("previous_pose_max_limb_delta_cm"),MaxLimbDelta);
                V->SetStringField(TEXT("motion_vector_audit_scope"),TEXT("CPU skin pose history and velocity policy; does not read back the GPU velocity texture"));
                for(const TCHAR* Bone:{TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r"),TEXT("Head")})
                    V->SetStringField(Bone,U->Soldier->GetSocketLocation(Bone).ToString());
                Visible.Add(MakeShared<FJsonValueObject>(V));
            }
            J->SetArrayField(TEXT("visible_units"),Visible);R->Captures.Add(MakeShared<FJsonValueObject>(J));
            return false;
        };
        auto Finish=[&]()
        {
            int32 Written=0,Refined=0,Expressive=0,ObservedBlinks=0,GrenadesReleased=0,DeadSoldiers=0,PhysicsHandoffs=0;TSet<int32> Phases,FaceSeeds;
            bool CompleteMorphLODs=true,CanaryPaths=true;int32 CanaryTeamMask=0;TMap<FString,bool> InspectedMorphMeshes;TArray<TSharedPtr<FJsonValue>> MorphLODReports;
            for(const auto& J:R->Captures)Written+=IFileManager::Get().FileExists(*J->AsObject()->GetStringField(TEXT("file")))?1:0;
            TArray<TSharedPtr<FJsonValue>> Units;
            for(const auto& Ref:R->Men)if(auto* U=Ref.Get())
            {
                Refined+=U->Soldier->GetSkeletalMeshAsset()->GetName().EndsWith(TEXT("_V34"))?1:0;Phases.Add(FMath::RoundToInt(U->GetMotionPhase()*100000));
                auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("mesh"),GetNameSafe(U->Soldier->GetSkeletalMeshAsset()));J->SetNumberField(TEXT("shots"),U->ShotsDischarged);J->SetNumberField(TEXT("completed_reloads"),U->ReloadsCompleted);J->SetNumberField(TEXT("grip_frames"),0);
                GrenadesReleased+=U->GrenadesReleased;DeadSoldiers+=U->IsAlive()?0:1;PhysicsHandoffs+=U->DeathState.bPhysicsStarted?1:0;
                J->SetNumberField(TEXT("grenades_released"),U->GrenadesReleased);J->SetBoolField(TEXT("alive"),U->IsAlive());
                J->SetBoolField(TEXT("death_started"),U->DeathState.bStarted);J->SetBoolField(TEXT("physics_handoff_started"),U->DeathState.bPhysicsStarted);
                const auto* Mesh=U->Soldier->GetSkeletalMeshAsset();const FString MeshPath=GetPathNameSafe(Mesh);
                J->SetStringField(TEXT("mesh_path"),MeshPath);
                if(R->Canary)
                {
                    const FString Faction=U->Team==0?TEXT("British"):TEXT("German");
                    const FString Name=TEXT("SK_")+Faction+TEXT("_Rifle_V34");
                    const FString Expected=TEXT("/Game/Warfare/Characters/V34/Meshes/")+Name+TEXT(".")+Name;
                    J->SetStringField(TEXT("expected_mesh_path"),Expected);
                    CanaryPaths&=MeshPath==Expected && U->Role==EFrontlineRole::Rifle && (U->Team==0 || U->Team==1);
                    if(U->Team==0 || U->Team==1)CanaryTeamMask|=1<<U->Team;
                }
                if(!InspectedMorphMeshes.Contains(MeshPath))
                {
                    auto MorphReport=InspectFaceMorphLODsV29(Mesh);
                    const bool Good=MorphReport->GetBoolField(TEXT("all_six_channels_present_at_all_lods"));
                    InspectedMorphMeshes.Add(MeshPath,Good);CompleteMorphLODs&=Good;
                    MorphLODReports.Add(MakeShared<FJsonValueObject>(MorphReport));
                }
                J->SetBoolField(TEXT("face_morph_lods_complete"),InspectedMorphMeshes.FindChecked(MeshPath));
                if(auto* F=U->FindComponentByClass<UFrontlineAppearanceV29>())
                {Expressive+=F->HasAuthoredExpressions()?1:0;FaceSeeds.Add(F->GetIdentitySeed());ObservedBlinks+=F->GetCompletedBlinkCount();J->SetBoolField(TEXT("authored_face_channels"),F->HasAuthoredExpressions());J->SetNumberField(TEXT("face_seed"),F->GetIdentitySeed());J->SetNumberField(TEXT("blink_events"),F->GetCompletedBlinkCount());}
                if(auto* N=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()))
                {
                    J->SetNumberField(TEXT("grip_frames"),N->GetGripConstraintFrames());R->ConstraintFrames+=N->GetGripConstraintFrames();const auto D=N->GetCharacterDiagnostics();
                    J->SetNumberField(TEXT("evaluated_frames"),D.EvaluatedFrames);J->SetNumberField(TEXT("personal_layer_frames"),D.PersonalLayerFrames);J->SetNumberField(TEXT("pole_fallback_frames"),D.PoleFallbackFrames);J->SetNumberField(TEXT("snapshot_remaps"),D.SnapshotRemaps);J->SetNumberField(TEXT("interrupted_transitions"),D.InterruptedTransitions);
                    R->MaxArmLengthError=FMath::Max(R->MaxArmLengthError,D.MaximumArmLengthErrorCm);R->MaxPersonalGripDrift=FMath::Max(R->MaxPersonalGripDrift,D.MaximumPersonalGripDriftCm);R->InvalidBoneFrames+=D.InvalidBoneFrames;R->UnsafeHierarchyFrames+=D.UnsafeTorsoHierarchyFrames;
                }
                Units.Add(MakeShared<FJsonValueObject>(J));
            }
            const auto Mean=[](const TArray<double>& A){double Sum=0;for(double V:A)Sum+=V;return A.Num()?Sum/A.Num():0.;};
            auto J=MakeShared<FJsonObject>();int32 W=0,H=0;PC->GetViewportSize(W,H);
            const int32 ExpectedCaptures=R->Canary?3:R->Focused?5:26;
            const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV34Baseline"));
            J->SetBoolField(TEXT("success"),R->Ready&&R->Failure.IsEmpty()&&Written==ExpectedCaptures&&R->Captures.Num()==ExpectedCaptures&&R->Failed.GetValue()==0&&R->MaxGrip<1.f&&R->Interrupts>=12&&R->InvalidBoneFrames==0&&R->UnsafeHierarchyFrames==0&&R->MaxArmLengthError<.01f&&R->MaxPersonalGripDrift<.001f&&Phases.Num()==R->Men.Num()&&GrenadesReleased==2&&DeadSoldiers==2&&PhysicsHandoffs==2&&(Baseline||(Refined==R->Men.Num()&&Expressive==R->Men.Num()&&FaceSeeds.Num()==R->Men.Num()&&ObservedBlinks>0&&CompleteMorphLODs)));
            J->SetNumberField(TEXT("expected_capture_count"),ExpectedCaptures);J->SetNumberField(TEXT("requested_capture_count"),R->Captures.Num());J->SetNumberField(TEXT("written_capture_count"),Written);
            J->SetNumberField(TEXT("initial_fixture_build_seconds_excluded_from_sampling_budget"),R->InitialFixtureBuildSeconds);
            J->SetNumberField(TEXT("sampling_budget_seconds"),100);
            if(R->Canary)
                J->SetBoolField(TEXT("success"),R->Ready&&R->Failure.IsEmpty()&&Written==ExpectedCaptures&&R->Captures.Num()==ExpectedCaptures&&R->Failed.GetValue()==0&&R->Men.Num()==2&&CanaryPaths&&CanaryTeamMask==3&&Refined==2&&CompleteMorphLODs&&R->InvalidBoneFrames==0&&R->UnsafeHierarchyFrames==0&&R->MaxArmLengthError<.01f&&R->MaxPersonalGripDrift<.001f&&R->MaxGrip<1.f);
            if(R->Focused)
                J->SetBoolField(TEXT("success"),R->Ready&&R->Failure.IsEmpty()&&Written==ExpectedCaptures&&R->Captures.Num()==ExpectedCaptures&&R->Failed.GetValue()==0&&R->FocusedLOD3Ready&&Refined==R->Men.Num()&&CompleteMorphLODs&&R->InvalidBoneFrames==0&&R->UnsafeHierarchyFrames==0&&R->MaxArmLengthError<.01f&&R->MaxPersonalGripDrift<.001f&&R->MaxGrip<1.f&&R->BurningSampleReady);
            J->SetBoolField(TEXT("focused"),R->Focused);J->SetBoolField(TEXT("focused_exact_four_meshes_at_lod3"),R->Focused&&R->FocusedLOD3Ready);
            J->SetBoolField(TEXT("canary"),R->Canary);J->SetBoolField(TEXT("canary_exact_rifle_paths"),R->Canary&&CanaryPaths&&CanaryTeamMask==3);
            J->SetBoolField(TEXT("baseline"),Baseline);J->SetStringField(TEXT("failure"),R->Failure);J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);J->SetNumberField(TEXT("refined_meshes_instantiated"),Refined);
            J->SetNumberField(TEXT("distinct_personal_phases"),Phases.Num());J->SetNumberField(TEXT("interrupted_transitions"),R->Interrupts);J->SetNumberField(TEXT("grip_constraint_frames"),R->ConstraintFrames);J->SetNumberField(TEXT("max_grip_residual_cm"),R->MaxGrip);J->SetNumberField(TEXT("max_grip_correction_cm"),R->MaxCorrection);
            J->SetNumberField(TEXT("max_arm_length_error_cm"),R->MaxArmLengthError);J->SetNumberField(TEXT("max_personal_grip_drift_cm"),R->MaxPersonalGripDrift);J->SetNumberField(TEXT("invalid_bone_frames"),R->InvalidBoneFrames);J->SetNumberField(TEXT("unsafe_torso_hierarchy_frames"),R->UnsafeHierarchyFrames);
            J->SetNumberField(TEXT("expressive_characters"),Expressive);J->SetNumberField(TEXT("distinct_face_seeds"),FaceSeeds.Num());J->SetNumberField(TEXT("observed_blinks"),ObservedBlinks);
            J->SetNumberField(TEXT("grenades_released"),GrenadesReleased);J->SetNumberField(TEXT("dead_soldiers"),DeadSoldiers);J->SetNumberField(TEXT("physics_handoffs"),PhysicsHandoffs);
            J->SetNumberField(TEXT("matching_physical_weapon_drops_at_release"),R->MatchingPhysicalDrops);J->SetNumberField(TEXT("hidden_held_guns_after_release"),R->HiddenHeldGuns);J->SetNumberField(TEXT("physical_weapon_drops_moved"),R->DropsMoved.Num());
            if(!R->Canary&&!R->Focused&&!Baseline)J->SetBoolField(TEXT("success"),J->GetBoolField(TEXT("success"))&&R->MatchingPhysicalDrops==2&&R->HiddenHeldGuns==2&&R->DropsMoved.Num()==2);
            J->SetStringField(TEXT("weapon_release_evidence_scope"),TEXT("The two bullet death samples require matched V34 rifle static meshes, physics enabled at release, the original weapon bone hidden and more than 3 cm of later independent travel. Existing death-entry and grounded-casualty PNGs show the guns and full bodies. This is not all-18-role or saturation-cap physics acceptance."));
            J->SetNumberField(TEXT("morph_meshes_checked"),InspectedMorphMeshes.Num());J->SetBoolField(TEXT("all_face_morph_lods_complete"),CompleteMorphLODs);
            J->SetArrayField(TEXT("morph_lod_audit"),MorphLODReports);
            J->SetNumberField(TEXT("render_sample_frames"),R->WallMs.Num());J->SetNumberField(TEXT("render_loop_fps"),Mean(R->WallMs)>0?1000./Mean(R->WallMs):0);J->SetNumberField(TEXT("gpu_ms_mean"),Mean(R->GPUMs));J->SetArrayField(TEXT("units"),Units);J->SetArrayField(TEXT("captures"),R->Captures);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(World))J->SetObjectField(TEXT("renderer"),Render->GetDiagnostics());
            auto LightingHistory=MakeShared<FJsonObject>();
            for(const TCHAR* Name:{TEXT("r.Lumen.ScreenProbeGather.Temporal"),TEXT("r.Lumen.ScreenProbeGather.Temporal.DistanceThreshold"),TEXT("r.Lumen.ScreenProbeGather.Temporal.MaxFramesAccumulated")})
                if(const auto* V=IConsoleManager::Get().FindConsoleVariable(Name))LightingHistory->SetNumberField(Name,V->GetFloat());
            J->SetObjectField(TEXT("lighting_history"),LightingHistory);
            J->SetBoolField(TEXT("aesthetic_acceptance_asserted"),false);
            J->SetBoolField(TEXT("burning_frame31_sample_ready"),R->BurningSampleReady);
            J->SetStringField(TEXT("burning_sample_scope"),TEXT("Two complete live fixture bodies held at 1.0 seconds / source frame 31 in the actual left/right BurningFall clips, with held weapons hidden as on fire death. Static pose evidence only: no extra fatalities, ignition simulation or claim that a still image validates the temporal landing impulse."));
            J->SetBoolField(TEXT("natural_recruitment_succeeded"),R->NaturalRecruits);
            J->SetNumberField(TEXT("natural_living_soldiers"),R->NaturalLiving);
            J->SetStringField(TEXT("scope"),TEXT("Labelled 21-soldier actual-renderer fixture on L_Frontline including two flame operators: source-selected reload, grenade, supported MG, interrupted transitions, physical deaths, followers, LOD 0/1/2 and normal-scale LOD 3; three single-character shoulder/stock/hand close views at LOD0. Six-second travelling-pose render-cost sample is not a dense battle benchmark. Last image uses real BeginBattle/Recruit. Evidence completion is not proof of zero penetration or AAA art; review PNGs independently against concepts and anatomy."));
            if(R->Canary)J->SetStringField(TEXT("scope"),TEXT("Two-rifle first-article review only: three LOD0 shoulder/stock/hand close views, complete exact-path V34 British and German rifle meshes, on the actual renderer. No other role is instantiated. This is not all-role, full-animation, reinforcement, death, LOD-appearance or performance acceptance. Counters do not assert aesthetic quality; inspect the three PNGs."));
            if(R->Focused)J->SetStringField(TEXT("scope"),TEXT("Five-image repair check reusing the unchanged fixture: three rifle LOD0 shoulder/hand/neck views, a normal-scale exact British/German rifle/officer LOD3 view, and left/right BurningFall frame 31. The retained 21 instantiated characters are not all visually accepted. No grenade, real death/drop, recruitment, transition or performance test is run in this mode. Original Full26 evidence remains separate; inspect these five PNGs for the identified repairs."));
            J->SetBoolField(TEXT("all_roles_acceptance_asserted"),false);
            J->SetBoolField(TEXT("success"),J->GetBoolField(TEXT("success"))&&(R->Canary||R->Focused||(R->NaturalRecruits&&R->NaturalLiving>0&&R->BurningSampleReady)));
            FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Folder/TEXT("character-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UGameViewportClient::OnScreenshotCaptured().Remove(R->Screenshot);UE_LOG(LogTemp,Display,TEXT("WF_V34_REVIEW_COMPLETE success=%d images=%d grip=%.5f %s"),J->GetBoolField(TEXT("success")),Written,R->MaxGrip,*R->Folder);FPlatformMisc::RequestExit(false);
        };
        if(!B->bProgressionVolatile || !B->AssetFailure.IsEmpty() || Now-R->Started>100)
        {R->Ready=false;R->Failure=!B->AssetFailure.IsEmpty()?B->AssetFailure:TEXT("Isolation or bounded fixture timeout");Finish();return false;}
        if(!PC->BattleCamera)return true;
        switch(R->Stage++)
        {
        case 0:
        {
            PC->DisableInput(PC);PC->SetActorTickEnabled(false);if(auto* H=PC->GetHUD())H->bShowHUD=false;
            B->SetActorTickEnabled(false);B->StartNewCampaign(EFrontlineFaction::British);
            auto Add=[&](int32 Team,EFrontlineRole Role,int32 Member)
            {auto* U=World->SpawnActor<AWarfareFrontlineSoldier>(FVector(0,0,100),FRotator::ZeroRotator);if(!U){R->Ready=false;return;}
             U->Initialize(B,500+R->Men.Num(),Team,Role,Member);U->bFieldEntered=true;B->Units.Add(U);R->Men.Add(U);};
            if(R->Canary)
            {
                Add(0,EFrontlineRole::Rifle,0);Add(1,EFrontlineRole::Rifle,0);R->Ready&=R->Men.Num()==2;
                B->Phase=EFrontlinePhase::Briefing;
                if(R->Ready)GripClose(0,30);
                R->Stage=24;R->InitialFixtureBuildSeconds=FPlatformTime::Seconds()-R->Started;
                R->Started=FPlatformTime::Seconds();R->Next=R->Started+3.;break;
            }
            for(int32 Team=0;Team<2;++Team){Add(Team,EFrontlineRole::Rifle,0);Add(Team,EFrontlineRole::Lewis,0);Add(Team,EFrontlineRole::Mills,0);Add(Team,EFrontlineRole::Marksman,0);Add(Team,EFrontlineRole::Officer,0);Add(Team,EFrontlineRole::Lewis,1);}
            // Research/mission setup is explicitly synthetic and remains volatile.
            B->ActiveResearchMutable().TechnologyMask|=(1u<<9)|(1u<<12);
            for(auto& C:B->CampaignRuns)if(C.Id==B->ActiveCampaignId){C.CompletedMissions=5;C.SelectedMission=5;}
            B->BindCampaignEncounter();Add(0,EFrontlineRole::Marksman,0);Add(1,EFrontlineRole::Mills,0);Add(0,EFrontlineRole::Officer,0);Add(1,EFrontlineRole::Officer,0);
            Add(0,EFrontlineRole::Rifle,1);Add(0,EFrontlineRole::Rifle,2);Add(0,EFrontlineRole::Rifle,3);
            Add(0,EFrontlineRole::Flamethrower,0);Add(1,EFrontlineRole::Flamethrower,0);
            R->Ready&=R->Men.Num()==21;B->Phase=EFrontlinePhase::Briefing;
            TArray<int32> All;for(int32 I=0;I<R->Men.Num();++I)All.Add(I);Show(All,220);Camera(6000,1800,650);
            if(R->Focused)
            {
                Show({0,6,14,15},250);Camera(6000,1800,500);
                for(int32 I:{0,6,14,15})if(auto* U=R->Men[I].Get())
                {U->SetActorTickEnabled(false);U->bMoving=false;U->MotionSpeed=0;U->SetMotion(U->GetAimMotion());U->Soldier->SetForcedLOD(4);}
                R->Stage=20;
            }
            // A first load can synchronously build skeletal LOD/DDC data.
            // Only this fixture construction is outside the unchanged 100 s
            // sampling watchdog; actual capture/readback stalls remain bounded.
            R->InitialFixtureBuildSeconds=FPlatformTime::Seconds()-R->Started;
            R->Started=FPlatformTime::Seconds();R->Next=R->Started+3.;break;
        }
        case 1:if(!Snap(TEXT("01-normal-scale-roster"))){--R->Stage;R->Next=Now+.01;return true;}Show({0,1,2,3,4,5});Camera(1700);R->Next=Now+1.;break;
        case 2:if(!Snap(TEXT("02-british-equipment"))){--R->Stage;R->Next=Now+.01;return true;}Show({6,7,8,9,10,11});R->Next=Now+1.;break;
        case 3:if(!Snap(TEXT("03-german-equipment"))){--R->Stage;R->Next=Now+.01;return true;}Show({12,13,14,15,19,20});Camera(1750);R->Next=Now+1.;break;
        case 4:if(!Snap(TEXT("04-specialists-and-officers"))){--R->Stage;R->Next=Now+.01;return true;}Show({0,6,14},190);Camera(700,1800,145);for(int32 I:{0,6,14})R->Men[I]->SetActorRotation(FRotator(0,75,0));R->Next=Now+1.;break;
        case 5:if(!Snap(TEXT("05-face-and-helmet"))){--R->Stage;R->Next=Now+.01;return true;}Show({0,1,6,7},260);Camera(1300);for(int32 I:{0,1,6,7})R->Men[I]->SetMotion(R->Men[I]->GetAimMotion());R->Next=Now+.6;break;
        case 6:if(!Snap(TEXT("06-rifle-and-machinegun-aim"))){--R->Stage;R->Next=Now+.01;return true;}for(int32 I:{0,1,6,7}){auto* U=R->Men[I].Get();U->RoundsRemaining=0;U->StartReload();}R->Next=Now+1.2;break;
        case 7:if(!Snap(TEXT("07-feed-and-reload"))){--R->Stage;R->Next=Now+.01;return true;}Show({2,8},320);Camera(900);B->Phase=EFrontlinePhase::Fighting;for(const auto& Ref:R->Men)if(Ref.IsValid())Ref->SetActorTickEnabled(false);for(int32 I:{2,8}){auto* U=R->Men[I].Get();U->ReturnToIdle();U->StartGrenadeThrow(R->Men[I==2?8:2].Get());R->Ready&=U->WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow;}R->Next=Now+.7;break;
        case 8:if(!Snap(TEXT("08-overarm-preparation"))){--R->Stage;R->Next=Now+.01;return true;}R->Next=Now+.6;break;
        case 9:if(!Snap(TEXT("09-overarm-release"))){--R->Stage;R->Next=Now+.01;return true;}B->Phase=EFrontlinePhase::Briefing;for(const auto& Ref:R->Men)if(Ref.IsValid())Ref->SetActorTickEnabled(true);Show({0,16,17,18},230);Camera(1200);R->Next=Now+.1;break;
        case 10:R->StageAt=Now;R->Next=Now;break;
        case 11:if(!Snap(TEXT("10-interrupted-transitions"))){--R->Stage;R->Next=Now+.01;return true;}for(int32 I:{0,16,17,18}){R->Men[I]->bMoving=true;R->Men[I]->MotionSpeed=390;R->Men[I]->SetMotion(R->Men[I]->GetRunMotion());}R->Next=Now+.8;break;
        case 12:if(!Snap(TEXT("11-independent-strides"))){--R->Stage;R->Next=Now+.01;return true;}
            Show({16,17},270);Camera(850,1800,AWarfareFrontlineGround::HeightAt(1800,0)+85);B->Phase=EFrontlinePhase::Fighting;
            for(int32 I:{16,17})
            {
                auto* U=R->Men[I].Get();U->ReceiveHit(1,.8f,1,FVector(-1,0,.12),EFrontlineDamageKind::Bullet,1);
                R->HiddenHeldGuns+=U->Soldier->IsBoneHiddenByName(TEXT("weapon_rifle"))?1:0;
                TInlineComponentArray<UStaticMeshComponent*> Parts(U);
                for(auto* C:Parts)if(C->ComponentHasTag(TEXT("WF_V34_RELEASED_WEAPON")))
                {
                    R->DropOrigins.Add(I,C->GetComponentLocation());
                    const bool Exact=GetPathNameSafe(C->GetStaticMesh())==TEXT("/Game/Warfare/Characters/V34/Weapons/SM_Dropped_British_Rifle_V34.SM_Dropped_British_Rifle_V34");
                    R->MatchingPhysicalDrops+=Exact&&C->IsSimulatingPhysics()&&C->IsVisible()?1:0;
                }
            }
            B->Phase=EFrontlinePhase::Briefing;R->Next=Now+.32;break;
        case 13:if(!Snap(TEXT("12-death-entry"))){--R->Stage;R->Next=Now+.01;return true;}R->Next=Now+1.2;break;
        case 14:if(!Snap(TEXT("13-grounded-casualties"))){--R->Stage;R->Next=Now+.01;return true;}for(int32 I=0;I<R->Men.Num();++I)if(auto* U=R->Men[I].Get())if(U->IsAlive()){U->ReturnToIdle();U->bMoving=true;U->MotionSpeed=210;U->SetMotion(U->GetWalkMotion());U->SetActorHiddenInGame(false);float X=1800+(I-9)*180.f;U->SetActorLocation(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)+88));}Camera(6000,1800,650);R->Next=Now+.2;break;
        case 15:R->StageAt=Now;R->Next=Now+8.;break;
        case 16:if(!Snap(TEXT("14-normal-scale-moving-company"))){--R->Stage;R->Next=Now+.01;return true;}
            Show({0,6,14,15},250);Camera(1250);for(int32 I:{0,6,14,15}){auto* U=R->Men[I].Get();U->bMoving=false;U->MotionSpeed=0;U->SetMotion(U->GetAimMotion());U->Soldier->SetForcedLOD(1);}R->Next=Now+1.;break;
        case 17:if(!Snap(TEXT("15-aim-seams-lod0"))){--R->Stage;R->Next=Now+.01;return true;}
            for(int32 I:{0,6,14,15})R->Men[I]->Soldier->SetForcedLOD(2);InvalidateStagedViewHistory();R->Next=Now+.7;break;
        case 18:if(!Snap(TEXT("16-aim-seams-lod1"))){--R->Stage;R->Next=Now+.01;return true;}
            for(int32 I:{0,6,14,15})R->Men[I]->Soldier->SetForcedLOD(3);InvalidateStagedViewHistory();R->Next=Now+.7;break;
        case 19:if(!Snap(TEXT("17-aim-seams-lod2"))){--R->Stage;R->Next=Now+.01;return true;}
            for(int32 I:{0,6,14,15})R->Men[I]->Soldier->SetForcedLOD(4);Camera(6000,1800,500);R->Next=Now+.8;break;
        case 20:if(!Snap(TEXT("17b-lod3-normal-game-scale"))){--R->Stage;R->Next=Now+.01;return true;}
            if(R->Focused)
            {
                R->FocusedLOD3Ready=true;
                for(int32 I:{0,6,14,15})if(auto* U=R->Men[I].Get())
                {
                    const FString Name=FString::Printf(TEXT("SK_%s_%s_V34"),U->Team?TEXT("German"):TEXT("British"),I>=14?TEXT("Officer"):TEXT("Rifle"));
                    const FString Path=TEXT("/Game/Warfare/Characters/V34/Meshes/")+Name+TEXT(".")+Name;
                    R->FocusedLOD3Ready&=GetPathNameSafe(U->Soldier->GetSkeletalMeshAsset())==Path&&U->Soldier->GetPredictedLODLevel()==3;
                }
                GripClose(0,30);R->Stage=24;R->Next=Now+1.2;break;
            }
            for(int32 I:{0,6,14,15})R->Men[I]->Soldier->SetForcedLOD(0);
            Show({1,5,7,11},360);Camera(1850,1800,60);
            for(int32 I:{1,5,7,11})if(auto* U=R->Men[I].Get())if(auto* MG=FrontlineMachinegunsV30::Get(U))
            {MG->State=EFrontlineGunStanceV30::Supported;U->bMoving=false;U->MotionSpeed=0;U->SetMotion(MG->Action(TEXT("Aim")));}
            R->Next=Now+1.;break;
        case 21:if(!Snap(TEXT("18-supported-ground-gun-and-bearer"))){--R->Stage;R->Next=Now+.01;return true;}
            Show({0,6,19,20},310);Camera(1550,1800,125);
            for(int32 I:{0,6,19,20})if(auto* U=R->Men[I].Get())
            {U->SetActorTickEnabled(false);U->SetMotion(U->GetAimMotion());TInlineComponentArray<USkeletalMeshComponent*> Components(U);
             for(auto* C:Components)if(C!=U->Soldier && GetNameSafe(C->GetSkeletalMeshAsset()).Contains(TEXT("Respirator")))C->SetVisibility(true);}
            R->Next=Now+1.;break;
        case 22:if(!Snap(TEXT("19-mask-helmet-grip-contact"))){--R->Stage;R->Next=Now+.01;return true;}
            for(int32 I:{0,6,19,20})R->Men[I]->SetActorRotation(FRotator(0,250,0));InvalidateStagedViewHistory();R->Next=Now+.8;break;
        case 23:if(!Snap(TEXT("20-rear-shoulders-kit-contact"))){--R->Stage;R->Next=Now+.01;return true;}
            GripClose(0,30);R->Next=Now+1.2;break;
        case 24:if(!Snap(TEXT("21-uk-trigger-hand-close-lod0"))){--R->Stage;R->Next=Now+.01;return true;}
            GripClose(0,210);R->Next=Now+1.2;break;
        case 25:if(!Snap(TEXT("22-uk-opposite-grip-close-lod0"))){--R->Stage;R->Next=Now+.01;return true;}
            GripClose(R->Canary?1:6,30);R->Next=Now+1.2;break;
        case 26:if(!Snap(TEXT("23-de-trigger-hand-close-lod0"))){--R->Stage;R->Next=Now+.01;return true;}
            if(R->Canary){R->Stage=29;R->Next=Now+.2;break;}
            // One remaining source-scan concern: expose the actual frame-31
            // burning posture without changing the two real fatal-hit samples.
            Show({0,6},320);Camera(900,1800,AWarfareFrontlineGround::HeightAt(1800,0)+90);
            R->BurningSampleReady=true;
            for(int32 I:{0,6})if(auto* U=R->Men[I].Get())
            {
                const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV34Baseline"));
                const FString Version=Baseline?TEXT("V33"):TEXT("V34");
                const FString Name=FString::Printf(TEXT("A_BurningFall_%s_%s"),I==0?TEXT("Left"):TEXT("Right"),*Version);
                const FString Path=FString::Printf(TEXT("/Game/Warfare/Characters/%s/Animations/%s.%s"),*Version,*Name,*Name);
                auto* Clip=LoadObject<UAnimSequenceBase>(nullptr,*Path);
                auto* Node=Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance());
                R->BurningSampleReady&=Clip&&Node&&GetPathNameSafe(Clip)==Path;
                U->SetActorTickEnabled(false);U->bMoving=false;U->MotionSpeed=0;
                U->SetActorRotation(FRotator(0,I==0?30:110,0));U->Soldier->SetForcedLOD(1);
                U->Soldier->HideBoneByName(TEXT("weapon_rifle"),EPhysBodyOp::PBO_None);
                // This pose-only sample has no ReleaseWeapon call. Hide the
                // independent bayonet attached to that same hidden gun bone.
                TInlineComponentArray<UStaticMeshComponent*> Parts(U);
                for(auto* Part:Parts)if(Part->GetAttachParent()==U->Soldier && Part->GetAttachSocketName()==FName(TEXT("weapon_rifle")))
                    Part->SetVisibility(false,true);
                if(Clip&&Node){Node->TransitionTo(Clip,false,0.f,1.f,0.f);Node->SetPlaying(false);}
            }
            R->Next=Now+1.2;break;
        case 27:if(!Snap(TEXT("25-burning-left-right-frame31-lod0"))){--R->Stage;R->Next=Now+.01;return true;}
            for(int32 I:{0,6})if(auto* U=R->Men[I].Get())if(auto* Node=U->Soldier->GetSingleNodeInstance())
                R->BurningSampleReady&=FMath::IsNearlyEqual(Node->GetCurrentTime(),1.f,.001f);
            if(R->Focused){R->Stage=29;R->Next=Now+.2;break;}
            // Hide the retained fixture before starting normal recruitment. It
            // stays alive for the final counters but is absent from simulation.
            for(const auto& Ref:R->Men)if(auto* U=Ref.Get()){U->SetActorHiddenInGame(true);U->SetActorTickEnabled(false);}
            B->Units.Reset();B->Squads.Reset();B->Phase=EFrontlinePhase::Briefing;B->BeginBattle();
            R->NaturalRecruits=B->Recruit(EFrontlineRole::Rifle,0)&&B->Recruit(EFrontlineRole::Rifle,1);
            B->SetActorTickEnabled(true);Camera(8000,B->GetHomeCameraFocusX(),550);R->Next=Now+5.;break;
        case 28:R->NaturalLiving=B->LivingSoldiers(0)+B->LivingSoldiers(1);
            if(!Snap(TEXT("24-real-reinforcement-game-scale"))){--R->Stage;R->Next=Now+.01;return true;}R->Next=Now+.2;break;
        default:if(R->Pending.GetValue()){R->Next=Now+.1;break;}Finish();return false;
        }
        return true;
    }));
}
