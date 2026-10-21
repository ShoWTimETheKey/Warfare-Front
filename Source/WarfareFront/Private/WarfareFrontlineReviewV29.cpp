#include "WarfareFrontlineReviewV29.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineAppearanceV29.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineRenderSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/MorphTarget.h"
#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
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
    bool Complete=Mesh!=nullptr;
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
        for(int32 LOD=0;LOD<3;++LOD)
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
    Report->SetArrayField(TEXT("channels"),ChannelReports);Report->SetBoolField(TEXT("all_six_channels_present_at_lod_0_1_2"),Complete);
    Report->SetStringField(TEXT("count_source"),TEXT("UMorphTarget::GetNumDeltasForLOD and HasDataForLOD; supports cooked GPU-only morph storage"));
    return Report;
}
}

// A short, labelled asset/transition fixture on the real map. It does not write saves.
void StartFrontlineV29Review(AWarfareFrontlineController* Controller)
{
    FString UserDir;
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV29Review")) ||
        !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || UserDir.IsEmpty())return;
    struct FRun
    {
        int32 Stage=0,Interrupts=0,Samples=0;bool Ready=true;
        double Started=FPlatformTime::Seconds(),Next=Started+1,StageAt=0,Last=0;
        FString Folder,Failure,RequestedFile,CapturedFile;TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Men;
        TArray<TSharedPtr<FJsonValue>> Captures;TArray<double> WallMs,GPUMs;
        FDelegateHandle Screenshot;FThreadSafeCounter Pending,Failed;
        float MaxGrip=0,MaxCorrection=0,MaxArmLengthError=0,MaxPersonalGripDrift=0;int32 ConstraintFrames=0,InvalidBoneFrames=0,UnsafeHierarchyFrames=0;
    };
    auto R=MakeShared<FRun,ESPMode::ThreadSafe>();TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    R->Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V29/CharacterReview")/FGuid::NewGuid().ToString());
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
            bool CompleteMorphLODs=true;TMap<FString,bool> InspectedMorphMeshes;TArray<TSharedPtr<FJsonValue>> MorphLODReports;
            for(const auto& J:R->Captures)Written+=IFileManager::Get().FileExists(*J->AsObject()->GetStringField(TEXT("file")))?1:0;
            TArray<TSharedPtr<FJsonValue>> Units;
            for(const auto& Ref:R->Men)if(auto* U=Ref.Get())
            {
                Refined+=U->Soldier->GetSkeletalMeshAsset()->GetName().EndsWith(TEXT("_V29"))?1:0;Phases.Add(FMath::RoundToInt(U->GetMotionPhase()*100000));
                auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("mesh"),GetNameSafe(U->Soldier->GetSkeletalMeshAsset()));J->SetNumberField(TEXT("shots"),U->ShotsDischarged);J->SetNumberField(TEXT("completed_reloads"),U->ReloadsCompleted);J->SetNumberField(TEXT("grip_frames"),0);
                GrenadesReleased+=U->GrenadesReleased;DeadSoldiers+=U->IsAlive()?0:1;PhysicsHandoffs+=U->DeathState.bPhysicsStarted?1:0;
                J->SetNumberField(TEXT("grenades_released"),U->GrenadesReleased);J->SetBoolField(TEXT("alive"),U->IsAlive());
                J->SetBoolField(TEXT("death_started"),U->DeathState.bStarted);J->SetBoolField(TEXT("physics_handoff_started"),U->DeathState.bPhysicsStarted);
                const auto* Mesh=U->Soldier->GetSkeletalMeshAsset();const FString MeshPath=GetPathNameSafe(Mesh);
                if(!InspectedMorphMeshes.Contains(MeshPath))
                {
                    auto MorphReport=InspectFaceMorphLODsV29(Mesh);
                    const bool Good=MorphReport->GetBoolField(TEXT("all_six_channels_present_at_lod_0_1_2"));
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
            const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFV29Baseline"));
            J->SetBoolField(TEXT("success"),R->Ready&&R->Failure.IsEmpty()&&Written>=10&&R->Failed.GetValue()==0&&R->MaxGrip<1.f&&R->Interrupts>=12&&R->InvalidBoneFrames==0&&R->UnsafeHierarchyFrames==0&&R->MaxArmLengthError<.01f&&R->MaxPersonalGripDrift<.001f&&Phases.Num()==R->Men.Num()&&GrenadesReleased==2&&DeadSoldiers==2&&PhysicsHandoffs==2&&(Baseline||(Refined==R->Men.Num()&&Expressive==R->Men.Num()&&FaceSeeds.Num()==R->Men.Num()&&ObservedBlinks>0&&CompleteMorphLODs)));
            J->SetBoolField(TEXT("baseline"),Baseline);J->SetStringField(TEXT("failure"),R->Failure);J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);J->SetNumberField(TEXT("refined_meshes_instantiated"),Refined);
            J->SetNumberField(TEXT("distinct_personal_phases"),Phases.Num());J->SetNumberField(TEXT("interrupted_transitions"),R->Interrupts);J->SetNumberField(TEXT("grip_constraint_frames"),R->ConstraintFrames);J->SetNumberField(TEXT("max_grip_residual_cm"),R->MaxGrip);J->SetNumberField(TEXT("max_grip_correction_cm"),R->MaxCorrection);
            J->SetNumberField(TEXT("max_arm_length_error_cm"),R->MaxArmLengthError);J->SetNumberField(TEXT("max_personal_grip_drift_cm"),R->MaxPersonalGripDrift);J->SetNumberField(TEXT("invalid_bone_frames"),R->InvalidBoneFrames);J->SetNumberField(TEXT("unsafe_torso_hierarchy_frames"),R->UnsafeHierarchyFrames);
            J->SetNumberField(TEXT("expressive_characters"),Expressive);J->SetNumberField(TEXT("distinct_face_seeds"),FaceSeeds.Num());J->SetNumberField(TEXT("observed_blinks"),ObservedBlinks);
            J->SetNumberField(TEXT("grenades_released"),GrenadesReleased);J->SetNumberField(TEXT("dead_soldiers"),DeadSoldiers);J->SetNumberField(TEXT("physics_handoffs"),PhysicsHandoffs);
            J->SetNumberField(TEXT("morph_meshes_checked"),InspectedMorphMeshes.Num());J->SetBoolField(TEXT("all_face_morph_lods_complete"),CompleteMorphLODs);
            J->SetArrayField(TEXT("morph_lod_audit"),MorphLODReports);
            J->SetNumberField(TEXT("render_sample_frames"),R->WallMs.Num());J->SetNumberField(TEXT("render_loop_fps"),Mean(R->WallMs)>0?1000./Mean(R->WallMs):0);J->SetNumberField(TEXT("gpu_ms_mean"),Mean(R->GPUMs));J->SetArrayField(TEXT("units"),Units);J->SetArrayField(TEXT("captures"),R->Captures);
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(World))J->SetObjectField(TEXT("renderer"),Render->GetDiagnostics());
            auto LightingHistory=MakeShared<FJsonObject>();
            for(const TCHAR* Name:{TEXT("r.Lumen.ScreenProbeGather.Temporal"),TEXT("r.Lumen.ScreenProbeGather.Temporal.DistanceThreshold"),TEXT("r.Lumen.ScreenProbeGather.Temporal.MaxFramesAccumulated")})
                if(const auto* V=IConsoleManager::Get().FindConsoleVariable(Name))LightingHistory->SetNumberField(Name,V->GetFloat());
            J->SetObjectField(TEXT("lighting_history"),LightingHistory);
            J->SetStringField(TEXT("scope"),TEXT("Labelled 19-soldier fixture on L_Frontline. Real meshes, materials, animation proxy, weapon reload/throw entry and Chaos death. Fixed staged transforms and selected clips; interrupted idle/aim transitions are intentional stress. Six-second normal-camera render-cost sample with travelling poses, no screenshots inside sample. Not a natural battle benchmark or proof of zero penetration at every frame. Inspect PNGs independently."));
            FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(R->Folder/TEXT("character-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UGameViewportClient::OnScreenshotCaptured().Remove(R->Screenshot);UE_LOG(LogTemp,Display,TEXT("WF_V29_REVIEW_COMPLETE success=%d images=%d grip=%.5f %s"),J->GetBoolField(TEXT("success")),Written,R->MaxGrip,*R->Folder);FPlatformMisc::RequestExit(false);
        };
        if(!B->bProgressionVolatile || !B->AssetFailure.IsEmpty() || Now-R->Started>90)
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
            for(int32 Team=0;Team<2;++Team){Add(Team,EFrontlineRole::Rifle,0);Add(Team,EFrontlineRole::Lewis,0);Add(Team,EFrontlineRole::Mills,0);Add(Team,EFrontlineRole::Marksman,0);Add(Team,EFrontlineRole::Officer,0);Add(Team,EFrontlineRole::Lewis,1);}
            // Research/mission setup is explicitly synthetic and remains volatile.
            B->ActiveResearchMutable().TechnologyMask|=(1u<<9)|(1u<<12);
            for(auto& C:B->CampaignRuns)if(C.Id==B->ActiveCampaignId){C.CompletedMissions=5;C.SelectedMission=5;}
            B->BindCampaignEncounter();Add(0,EFrontlineRole::Marksman,0);Add(1,EFrontlineRole::Mills,0);Add(0,EFrontlineRole::Officer,0);Add(1,EFrontlineRole::Officer,0);
            Add(0,EFrontlineRole::Rifle,1);Add(0,EFrontlineRole::Rifle,2);Add(0,EFrontlineRole::Rifle,3);
            R->Ready&=R->Men.Num()==19;B->Phase=EFrontlinePhase::Briefing;
            TArray<int32> All;for(int32 I=0;I<R->Men.Num();++I)All.Add(I);Show(All,220);Camera(6000,1800,650);R->Next=Now+3.;break;
        }
        case 1:if(!Snap(TEXT("01-normal-scale-roster"))){--R->Stage;R->Next=Now+.01;return true;}Show({0,1,2,3,4,5});Camera(1700);R->Next=Now+1.;break;
        case 2:if(!Snap(TEXT("02-british-equipment"))){--R->Stage;R->Next=Now+.01;return true;}Show({6,7,8,9,10,11});R->Next=Now+1.;break;
        case 3:if(!Snap(TEXT("03-german-equipment"))){--R->Stage;R->Next=Now+.01;return true;}Show({12,13,14,15});Camera(1200);R->Next=Now+1.;break;
        case 4:if(!Snap(TEXT("04-specialists-and-officers"))){--R->Stage;R->Next=Now+.01;return true;}Show({0,6,14},190);Camera(700,1800,145);for(int32 I:{0,6,14})R->Men[I]->SetActorRotation(FRotator(0,75,0));R->Next=Now+1.;break;
        case 5:if(!Snap(TEXT("05-face-and-helmet"))){--R->Stage;R->Next=Now+.01;return true;}Show({0,1,6,7},260);Camera(1300);for(int32 I:{0,1,6,7})R->Men[I]->SetMotion(R->Men[I]->GetAimMotion());R->Next=Now+.6;break;
        case 6:if(!Snap(TEXT("06-rifle-and-machinegun-aim"))){--R->Stage;R->Next=Now+.01;return true;}for(int32 I:{0,1,6,7}){auto* U=R->Men[I].Get();U->RoundsRemaining=0;U->StartReload();}R->Next=Now+1.2;break;
        case 7:if(!Snap(TEXT("07-feed-and-reload"))){--R->Stage;R->Next=Now+.01;return true;}Show({2,8},320);Camera(900);B->Phase=EFrontlinePhase::Fighting;for(const auto& Ref:R->Men)if(Ref.IsValid())Ref->SetActorTickEnabled(false);for(int32 I:{2,8}){auto* U=R->Men[I].Get();U->ReturnToIdle();U->StartGrenadeThrow(R->Men[I==2?8:2].Get());R->Ready&=U->WeaponPhase==EFrontlineWeaponPhase::GrenadeThrow;}R->Next=Now+.7;break;
        case 8:if(!Snap(TEXT("08-overarm-preparation"))){--R->Stage;R->Next=Now+.01;return true;}R->Next=Now+.6;break;
        case 9:if(!Snap(TEXT("09-overarm-release"))){--R->Stage;R->Next=Now+.01;return true;}B->Phase=EFrontlinePhase::Briefing;for(const auto& Ref:R->Men)if(Ref.IsValid())Ref->SetActorTickEnabled(true);Show({0,16,17,18},230);Camera(1200);R->Next=Now+.1;break;
        case 10:R->StageAt=Now;R->Next=Now;break;
        case 11:if(!Snap(TEXT("10-interrupted-transitions"))){--R->Stage;R->Next=Now+.01;return true;}for(int32 I:{0,16,17,18}){R->Men[I]->bMoving=true;R->Men[I]->MotionSpeed=390;R->Men[I]->SetMotion(R->Men[I]->GetRunMotion());}R->Next=Now+.8;break;
        case 12:if(!Snap(TEXT("11-independent-strides"))){--R->Stage;R->Next=Now+.01;return true;}B->Phase=EFrontlinePhase::Fighting;for(int32 I:{16,17}){auto* U=R->Men[I].Get();U->ReceiveHit(1,.8f,1,FVector(-1,0,.12),EFrontlineDamageKind::Bullet,1);}B->Phase=EFrontlinePhase::Briefing;R->Next=Now+.32;break;
        case 13:if(!Snap(TEXT("12-death-entry"))){--R->Stage;R->Next=Now+.01;return true;}R->Next=Now+1.2;break;
        case 14:if(!Snap(TEXT("13-grounded-casualties"))){--R->Stage;R->Next=Now+.01;return true;}for(int32 I=0;I<R->Men.Num();++I)if(auto* U=R->Men[I].Get())if(U->IsAlive()){U->ReturnToIdle();U->bMoving=true;U->MotionSpeed=210;U->SetMotion(U->GetWalkMotion());U->SetActorHiddenInGame(false);float X=1800+(I-9)*180.f;U->SetActorLocation(FVector(X,0,AWarfareFrontlineGround::HeightAt(X,0)+88));}Camera(6000,1800,650);R->Next=Now+.2;break;
        case 15:R->StageAt=Now;R->Next=Now+8.;break;
        case 16:if(!Snap(TEXT("14-normal-scale-moving-company"))){--R->Stage;R->Next=Now+.01;return true;}R->Next=Now+1.;break;
        default:if(R->Pending.GetValue()){R->Next=Now+.1;break;}Finish();return false;
        }
        return true;
    }));
}
