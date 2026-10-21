#include "WarfareFrontline.h"
#include "WarfareFrontlineAnimation.h"
#include "Async/Async.h"
#include "Engine/GameViewportClient.h"
#include "HAL/ThreadSafeCounter.h"
#include "ImageUtils.h"
#include "Modules/ModuleManager.h"
#include "Animation/AnimationAsset.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

/** Optional close camera on the real encounter; no replacement map or synthetic pose actors. */
void AWarfareFrontlineGameMode::BeginMotionReview()
{
    struct FReview
    {
        double Started=FPlatformTime::Seconds(),NextFrame=0,FinishAt=0;
        bool Requested=false,Begun=false,Released=false,Recruited=false;
        bool LookOnly=FParse::Param(FCommandLine::Get(),TEXT("WFFrontlineLookReview"));
        bool DefaultCamera=LookOnly || FParse::Param(FCommandLine::Get(),TEXT("WFReviewBattle"));
        bool FollowAutomatic=FParse::Param(FCommandLine::Get(),TEXT("WFReviewMachinegun"));
        bool FollowGrenadier=FParse::Param(FCommandLine::Get(),TEXT("WFReviewGrenadier"));
        bool FollowCasualties=FParse::Param(FCommandLine::Get(),TEXT("WFReviewCasualties"));
        bool BlastFixture=FParse::Param(FCommandLine::Get(),TEXT("WFReviewExplosions"));
        bool SmokeFixture=FParse::Param(FCommandLine::Get(),TEXT("WFReviewSmoke")),SmokeIssued=false;
        int32 FixtureShells=0;
        float NextFixtureShellAt=2.f;
        float NextAdvanceOrderAt=8.f;
        int32 NextRecruitRole=1;
        TWeakObjectPtr<AWarfareFrontlineSoldier> Casualty;
        int32 FollowSquad=INDEX_NONE,Frame=0;
        FString Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V12/MotionReview")/FGuid::NewGuid().ToString());
        TArray<TSharedPtr<FJsonValue>> Frames;
        FDelegateHandle ScreenshotHandle;
        FThreadSafeCounter PendingWrites,FailedWrites;
    };
    const auto Review=MakeShared<FReview,ESPMode::ThreadSafe>();
    const TWeakObjectPtr<AWarfareFrontlineGameMode> WeakBattle(this);
    IFileManager::Get().MakeDirectory(*Review->Directory,true);
    // Preserve real pixels while compressing off the game thread. Synchronous
    // PNG and trace compression otherwise introduce 300 ms simulation frames.
    FModuleManager::Get().LoadModuleChecked(TEXT("ImageWrapper"));
    Review->ScreenshotHandle=UGameViewportClient::OnScreenshotCaptured().AddLambda([Review](int32 W,int32 H,const TArray<FColor>& Pixels)
    {
        const FString Path=FScreenshotRequest::GetFilename();
        if (!Path.StartsWith(Review->Directory)) return;
        TArray<FColor> Copy=Pixels; Review->PendingWrites.Increment();
        Async(EAsyncExecution::ThreadPool,[Review,Path,W,H,Copy=MoveTemp(Copy)]()
        {
            TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(W,H,Copy,PNG);
            if (PNG.IsEmpty() || !FFileHelper::SaveArrayToFile(PNG,*Path)) Review->FailedWrites.Increment();
            Review->PendingWrites.Decrement();
        });
    });
    UE_LOG(LogTemp,Display,TEXT("WF_MOTION_REVIEW_DIRECTORY %s"),*Review->Directory);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakBattle,Review](float)
    {
        AWarfareFrontlineGameMode* B=WeakBattle.Get();
        if (!B) { UGameViewportClient::OnScreenshotCaptured().Remove(Review->ScreenshotHandle); return false; }
        const double Now=FPlatformTime::Seconds(),Wall=Now-Review->Started;
        AWarfareFrontlineController* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        if (!PC || !PC->BattleCamera || Wall<1.) return true;
        if (!Review->Requested)
        {
            PC->DisableInput(PC); // Recorded scripted encounter; manual play uses the normal launch.
            if (!B->AssetFailure.IsEmpty()) { UE_LOG(LogTemp,Error,TEXT("Motion review assets missing: %s"),*B->AssetFailure); FPlatformMisc::RequestExit(false); return false; }
            PC->BeginBattle(); PC->BeginBattle(); B->SetBattleSpeed(1.f);
            Review->Requested=true;
        }
        if (!Review->Begun)
        {
            if (B->Phase!=EFrontlinePhase::Fighting)
            {
                if (Wall>22.) {UE_LOG(LogTemp,Error,TEXT("WF_MOTION_PREPARATION_FAILED"));UGameViewportClient::OnScreenshotCaptured().Remove(Review->ScreenshotHandle);FPlatformMisc::RequestExit(false);return false;}
                return true;
            }
            if (!Review->DefaultCamera) PC->SetActorTickEnabled(false); // Only this explicit review controls its close camera.
            for (const FFrontlineSquad& S:B->Squads) if (S.Team==0) { Review->FollowSquad=S.Id; break; }
            Review->Begun=true; Review->NextFrame=Now; Review->FinishAt=Now+(Review->LookOnly?3.:Review->BlastFixture?22.:Review->SmokeFixture?24.:55.);
            const auto Role=Review->FollowAutomatic?EFrontlineRole::Lewis:Review->FollowGrenadier?EFrontlineRole::Mills:EFrontlineRole::Rifle;
            Review->Recruited=B->Recruit(Role);
            if (Review->FollowAutomatic || Review->FollowGrenadier) for (const FFrontlineSquad& S:B->Squads)
                if (S.Team==0 && S.Role==Role) { Review->FollowSquad=S.Id; break; }
        }
        if (!Review->LookOnly && !Review->BlastFixture && !Review->Released && B->Elapsed>=1.2f) Review->Released=B->AdvanceTrench(0);
        if (!Review->LookOnly && !Review->BlastFixture && B->Elapsed>=Review->NextAdvanceOrderAt)
        {
            // Later reinforcements first enter the home trench. Give them the
            // same legal advance command after arrival, rather than filming
            // an idle reserve that never received a player's second order.
            for (int32 I=0;I<3;++I) B->AdvanceTrench(I);
            Review->NextAdvanceOrderAt=B->Elapsed+8.f;
        }
        if (!Review->LookOnly && Review->DefaultCamera && B->Phase==EFrontlinePhase::Fighting)
        {
            const auto Role=EFrontlineRole(Review->NextRecruitRole%AWarfareFrontlineGameMode::RoleCount);
            if (B->RecruitBlockReason(Role).IsEmpty() && B->Recruit(Role)) ++Review->NextRecruitRole;
            if (B->ArtilleryCooldown()<=0)
                for (AWarfareFrontlineSoldier* U:B->Units) if (U && U->Team==1 && U->IsAlive()) { B->RequestArtillery(U->Feet()); break; }
        }
        if (Review->BlastFixture && Review->FixtureShells<2 && B->Elapsed>=Review->NextFixtureShellAt)
        {
            AWarfareFrontlineSoldier* Target=nullptr;
            for (AWarfareFrontlineSoldier* Unit:B->Units) if (Unit->Team==0 && Unit->IsAlive()
                && (!Target || Unit->Health<Target->Health)) Target=Unit;
            if (Target)
            {
                // Explicitly labelled review fixture: only artillery readiness is
                // accelerated. The real delayed blast deals all damage normally.
                B->ArtilleryReadyAt[0]=B->Elapsed;
                if (B->RequestArtillery(Target->Feet()))
                { ++Review->FixtureShells; Review->NextFixtureShellAt=B->Elapsed+4.5f; }
            }
        }
        if (Review->SmokeFixture && !Review->SmokeIssued && B->Elapsed>=3.f)
            Review->SmokeIssued=B->RequestSmoke(FVector(-1150,0,0));
        const FFrontlineSquad* Follow=B->FindSquad(Review->FollowSquad);
        if (Review->FollowCasualties || Review->BlastFixture)
        {
            // Observe naturally occurring casualties from their first update. Keep
            // each complete fall on screen for four seconds before selecting another.
            if (!Review->Casualty.IsValid() || Review->Casualty->GetDeathState().Age>4.f)
                for (AWarfareFrontlineSoldier* Unit:B->Units)
                    if (IsValid(Unit) && Unit->GetDeathState().bStarted && Unit->GetDeathState().Age<.15f)
                    { Review->Casualty=Unit; Review->FollowSquad=Unit->SquadId; break; }
            Follow=B->FindSquad(Review->FollowSquad);
        }
        FVector Focus=Follow && Follow->LivingCount()>0?Follow->Centre():FVector(-1500,0,0);
        if (Review->Casualty.IsValid()) Focus=Review->Casualty->GetActorLocation();
        Focus.Z+=Review->Casualty.IsValid()?40:190;
        const FVector Location=Focus+FVector(0,14000,4000);
        if (!Review->DefaultCamera)
        {
            PC->BattleCamera->SetActorLocation(Location); PC->BattleCamera->SetActorRotation((Focus-Location).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=1900;
            PC->SetViewTarget(PC->BattleCamera);
        }
        if (Now>=Review->NextFrame && Now<Review->FinishAt && Review->PendingWrites.GetValue()<3)
        {
            const FString Path=Review->Directory/FString::Printf(TEXT("motion-%04d.png"),Review->Frame++);
            FScreenshotRequest::RequestScreenshot(Path,true,false);
            const auto Frame=MakeShared<FJsonObject>(); Frame->SetStringField(TEXT("path"),Path);
            Frame->SetNumberField(TEXT("wall_seconds"),Wall); Frame->SetNumberField(TEXT("simulation_seconds"),B->Elapsed);
            if (Review->Casualty.IsValid())
            {
                Frame->SetNumberField(TEXT("followed_casualty_squad"),Review->Casualty->SquadId);
                Frame->SetNumberField(TEXT("followed_casualty_member"),Review->Casualty->MemberIndex);
            }
            TArray<TSharedPtr<FJsonValue>> Soldiers;
            if (Follow) for (const TWeakObjectPtr<AWarfareFrontlineSoldier>& Member:Follow->Members) if (Member.IsValid())
            {
                const auto Soldier=MakeShared<FJsonObject>(); Soldier->SetNumberField(TEXT("member"),Member->MemberIndex);
                Soldier->SetBoolField(TEXT("alive"),Member->IsAlive()); Soldier->SetNumberField(TEXT("speed_cm_s"),Member->GetMotionSpeed());
                Soldier->SetNumberField(TEXT("rounds_remaining"),Member->RoundsRemaining);
                Soldier->SetNumberField(TEXT("weapon_phase"),int32(Member->WeaponPhase));
                Soldier->SetNumberField(TEXT("completed_reloads"),Member->ReloadsCompleted);
                Soldier->SetNumberField(TEXT("grenade_throws_started"),Member->GrenadeThrowsStarted);
                Soldier->SetNumberField(TEXT("grenades_released"),Member->GrenadesReleased);
                Soldier->SetNumberField(TEXT("death_age_seconds"),Member->GetDeathState().Age);
                Soldier->SetBoolField(TEXT("death_settled"),Member->GetDeathState().bSettled);
                Soldier->SetNumberField(TEXT("shots"),Member->ShotsDischarged); Soldier->SetNumberField(TEXT("transitions"),Member->MotionTransitions);
                if (UWarfareFrontlineAnimation* Anim=Cast<UWarfareFrontlineAnimation>(Member->Soldier->GetAnimInstance()))
                {
                    Soldier->SetStringField(TEXT("clip"),GetNameSafe(Anim->GetAnimationAsset()));
                    Soldier->SetNumberField(TEXT("clip_seconds"),Anim->GetCurrentTime());
                    Soldier->SetNumberField(TEXT("grip_constraint_frames"),Anim->GetGripConstraintFrames());
                    Soldier->SetNumberField(TEXT("max_grip_residual_cm"),Anim->GetMaximumGripResidualCm());
                }
                Soldiers.Add(MakeShared<FJsonValueObject>(Soldier));
            }
            Frame->SetArrayField(TEXT("soldiers"),Soldiers); Review->Frames.Add(MakeShared<FJsonValueObject>(Frame));
            Review->NextFrame=Now+(Review->LookOnly?.6:1./12.); // Keep actual timestamps; not a frame-rate benchmark.
        }
        if (Now>=Review->FinishAt+1. && Review->PendingWrites.GetValue()==0)
        {
            const auto Report=MakeShared<FJsonObject>(); Report->SetStringField(TEXT("directory"),Review->Directory);
            Report->SetStringField(TEXT("scope"),Review->BlastFixture
                ?TEXT("Labelled L_Frontline artillery fixture. Only artillery readiness accelerated for two genuine delayed shells. No soldier health or position edits. Camera follows resulting casualties. Timestamped images, no performance or natural-match claim.")
                :Review->DefaultCamera?TEXT("Actual L_Frontline at the normal gameplay camera with genuine orders. Timestamped screen captures; not a performance measurement.")
                :TEXT("Real L_Frontline encounter, genuine recruit/advance, camera-only close-up. Timestamped frame sequence, not a performance benchmark or manufactured pose scene."));
            Report->SetBoolField(TEXT("labelled_artillery_fixture"),Review->BlastFixture);
            Report->SetBoolField(TEXT("requested_smoke_review"),Review->SmokeFixture);
            Report->SetBoolField(TEXT("smoke_order_accepted"),Review->SmokeIssued);
            Report->SetNumberField(TEXT("fixture_shells_ordered"),Review->FixtureShells);
            Report->SetBoolField(TEXT("recruit_accepted"),Review->Recruited); Report->SetBoolField(TEXT("advance_accepted"),Review->Released);
            Report->SetBoolField(TEXT("default_camera_look_only"),Review->LookOnly);
            Report->SetBoolField(TEXT("follows_automatic_squad"),Review->FollowAutomatic);
            Report->SetBoolField(TEXT("follows_natural_casualties"),Review->FollowCasualties);
            Report->SetBoolField(TEXT("default_gameplay_camera"),Review->DefaultCamera);
            Report->SetNumberField(TEXT("failed_image_writes"),Review->FailedWrites.GetValue());
            Report->SetStringField(TEXT("capture_pipeline"),TEXT("Actual viewport screenshot delegate; unchanged RGBA pixels, bounded asynchronous lossless PNG writes. Variable real timestamps; capture still incurs GPU readback, so it is not a frame-rate benchmark."));
            Report->SetNumberField(TEXT("camera_width_cm"),Review->DefaultCamera?6000:1900); Report->SetArrayField(TEXT("frames"),Review->Frames);
            FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Review->Directory/TEXT("motion-review.json")));
            PC->SetActorTickEnabled(true);
            UGameViewportClient::OnScreenshotCaptured().Remove(Review->ScreenshotHandle);
            UE_LOG(LogTemp,Display,TEXT("WF_MOTION_REVIEW_COMPLETE %s Frames=%d"),*Review->Directory,Review->Frame);
            FPlatformMisc::RequestExit(false); return false;
        }
        return true;
    }));
}
