#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSafeCounter.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
TSharedPtr<FJsonObject> ThrowPoseV24(const AWarfareFrontlineSoldier* Unit)
{
    auto J=MakeShared<FJsonObject>();if(!IsValid(Unit)){J->SetBoolField(TEXT("valid"),false);return J;}
    J->SetBoolField(TEXT("valid"),true);J->SetStringField(TEXT("faction"),Unit->GetFaction()==EFrontlineFaction::British?TEXT("British"):TEXT("German"));
    J->SetNumberField(TEXT("throws_started"),Unit->GrenadeThrowsStarted);J->SetNumberField(TEXT("actual_notify_releases"),Unit->GrenadesReleased);
    J->SetBoolField(TEXT("alive"),Unit->IsAlive());J->SetNumberField(TEXT("weapon_phase"),int32(Unit->WeaponPhase));
    J->SetStringField(TEXT("skeletal_mesh"),GetNameSafe(Unit->Soldier->GetSkeletalMeshAsset()));
    if(const auto* Node=Unit->Soldier->GetSingleNodeInstance())
    {J->SetStringField(TEXT("clip"),GetNameSafe(Node->GetAnimationAsset()));J->SetNumberField(TEXT("actual_clip_seconds"),Node->GetCurrentTime());}
    bool Held=false;FString MeshName;
    TInlineComponentArray<UStaticMeshComponent*> Components(Unit);
    for(const auto* C:Components)if(C && C->GetAttachParent()==Unit->Soldier && C->GetAttachSocketName()==TEXT("hand_r"))
    {Held=C->IsVisible();MeshName=GetNameSafe(C->GetStaticMesh());break;}
    J->SetBoolField(TEXT("held_grenade_component_visible"),Held);J->SetStringField(TEXT("held_grenade_mesh"),MeshName);
    J->SetStringField(TEXT("actual_release_socket_location"),Unit->GetGrenadeReleaseLocation().ToString());
    return J;
}
}

void AWarfareFrontlineGameMode::BeginV24GrenadeReview()
{
    struct FReview
    {
        double Started=FPlatformTime::Seconds(),ThrowAt=0;
        int32 Stage=0,Capture=0;bool GermanStarted=false,Setup=false;
        bool HeldSeen[2]={false,false},HiddenAfterRelease[2]={false,false};
        FString Error,Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V24/GrenadeReview")/FGuid::NewGuid().ToString());
        TWeakObjectPtr<AWarfareFrontlineSoldier> Men[2];
        TSharedPtr<FJsonObject> Landings[2];
        TArray<TSharedPtr<FJsonValue>> Frames;
        FDelegateHandle ScreenshotHandle;FThreadSafeCounter PendingWrites,FailedWrites;
    };
    const auto R=MakeShared<FReview,ESPMode::ThreadSafe>();TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(this);
    IFileManager::Get().MakeDirectory(*R->Directory,true);FModuleManager::Get().LoadModuleChecked(TEXT("ImageWrapper"));
    R->ScreenshotHandle=UGameViewportClient::OnScreenshotCaptured().AddLambda([R](int32 W,int32 H,const TArray<FColor>& Pixels)
    {
        const FString Path=FScreenshotRequest::GetFilename();if(!Path.StartsWith(R->Directory))return;
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("file"),Path);J->SetNumberField(TEXT("wall_after_british_start"),FPlatformTime::Seconds()-R->ThrowAt);
        J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);TArray<TSharedPtr<FJsonValue>> Men;
        for(int32 I=0;I<2;++I)Men.Add(MakeShared<FJsonValueObject>(ThrowPoseV24(R->Men[I].Get())));
        J->SetArrayField(TEXT("actual_captured_soldiers"),Men);R->Frames.Add(MakeShared<FJsonValueObject>(J));
        TArray<FColor> Copy=Pixels;R->PendingWrites.Increment();
        Async(EAsyncExecution::ThreadPool,[R,Path,W,H,Copy=MoveTemp(Copy)]()
        {TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(W,H,Copy,PNG);if(PNG.IsEmpty()||!FFileHelper::SaveArrayToFile(PNG,*Path))R->FailedWrites.Increment();R->PendingWrites.Decrement();});
    });
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,R](float)
    {
        auto* B=Weak.Get();auto* World=B?B->GetWorld():nullptr;auto* PC=World?Cast<AWarfareFrontlineController>(World->GetFirstPlayerController()):nullptr;
        const double Now=FPlatformTime::Seconds(),Wall=Now-R->Started;
        if(!B || !World){UGameViewportClient::OnScreenshotCaptured().Remove(R->ScreenshotHandle);return false;}
        if(Wall>18 && R->Error.IsEmpty())R->Error=TEXT("Timed out while waiting for actual capture/throw setup");
        if(!B->AssetFailure.IsEmpty())R->Error=B->AssetFailure;
        if(R->Stage==0 && Wall>=1 && PC && PC->BattleCamera)
        {
            PC->DisableInput(PC);PC->SetActorTickEnabled(false);
            if(!B->bProgressionVolatile)R->Error=TEXT("Grenade review requires isolated volatile progression");
            if(R->Error.IsEmpty())
            {
                B->ResetEncounter();B->SelectPlayerFaction(EFrontlineFaction::British);B->BeginBattle();
                B->EnemyThinkAt=100000.f; // Explicit pose fixture: no unrelated enemy recruitment or support orders.
                const double Centre=-4000.;const FVector Focus(Centre,0,650),Location(Centre,14000,4650);
                PC->BattleCamera->SetActorLocation(Location);PC->BattleCamera->SetActorRotation((Focus-Location).Rotation());
                PC->BattleCamera->GetCameraComponent()->OrthoWidth=6000;PC->SetViewTarget(PC->BattleCamera);
                // Spawn two actual equipment/animation actors; the scene is labelled
                // as arranged, never represented as an unmodified natural match.
                for(int32 Team=0;Team<2;++Team)
                {
                    const float X=Team?-3450.f:-4550.f;
                    const FVector At(X,0,AWarfareFrontlineGround::HeightAt(X,0)+88.f);
                    auto* U=World->SpawnActor<AWarfareFrontlineSoldier>(At,FRotator(0,Team?180:0,0));
                    if(!U){R->Error=TEXT("Actual assault actor spawn failed");break;}
                    FFrontlineSquad Squad;Squad.Id=B->NextSquadId++;Squad.Team=Team;Squad.Role=EFrontlineRole::Mills;
                    Squad.Trench=1;Squad.Destination=1;Squad.Slot=1;Squad.bHoldPosition=true;B->Squads.Add(Squad);
                    U->Initialize(B,Squad.Id,Team,EFrontlineRole::Mills,0);U->SetActorTickEnabled(false);
                    B->Units.Add(U);B->FindSquad(Squad.Id)->Members.Add(U);R->Men[Team]=U;
                }
                R->Setup=R->Men[0].IsValid()&&R->Men[1].IsValid();R->Stage=1;
            }
        }
        if(R->Stage==1 && Wall>=3.8 && R->Setup)
        {
            auto* U=R->Men[0].Get();U->StartGrenadeThrow(R->Men[1].Get());U->SetActorTickEnabled(true);
            R->ThrowAt=Now;R->Stage=2;
        }
        if(R->Stage==2)
        {
            const double T=Now-R->ThrowAt;
            if(!R->GermanStarted && T>=.28)
            {auto* U=R->Men[1].Get();U->StartGrenadeThrow(R->Men[0].Get());U->SetActorTickEnabled(true);R->GermanStarted=true;}
            for(int32 I=0;I<2;++I)if(auto* U=R->Men[I].Get())
            {
                auto J=ThrowPoseV24(U);const bool Held=J->GetBoolField(TEXT("held_grenade_component_visible"));
                R->HeldSeen[I]|=Held;R->HiddenAfterRelease[I]|=U->GrenadesReleased>0 && !Held;
            }
            for(const auto& S:B->Strikes)if(S.bGrenade && S.Team>=0 && S.Team<2)
            {
                if(!R->Landings[S.Team])
                {
                    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("team"),S.Team);
                    J->SetStringField(TEXT("fixed_target"),S.Target.ToString());J->SetStringField(TEXT("actual_origin"),S.Origin.ToString());
                    J->SetNumberField(TEXT("launched_simulation_time"),S.LaunchedAt);J->SetNumberField(TEXT("impact_simulation_time"),S.ImpactAt);
                    J->SetBoolField(TEXT("fixed_target_unchanged"),true);R->Landings[S.Team]=J;
                }
                auto J=R->Landings[S.Team];J->SetBoolField(TEXT("fixed_target_unchanged"),J->GetBoolField(TEXT("fixed_target_unchanged")) && J->GetStringField(TEXT("fixed_target"))==S.Target.ToString());
                J->SetBoolField(TEXT("resolved"),S.bResolved);J->SetBoolField(TEXT("projectile_alive"),S.Projectile.IsValid());
                if(S.Projectile.IsValid())J->SetStringField(TEXT("last_observed_projectile_position"),S.Projectile->GetActorLocation().ToString());
            }
            const double Times[]={.55,.90,1.05,1.18,1.40,1.80,2.50,3.45};
            const TCHAR* Names[]={TEXT("01-load"),TEXT("02-over-shoulder"),TEXT("03-before-release"),TEXT("04-british-release"),TEXT("05-german-release"),TEXT("06-follow-through"),TEXT("07-recovery"),TEXT("08-landed")};
            if(R->Capture<8 && T>=Times[R->Capture] && R->PendingWrites.GetValue()<3)
            {FScreenshotRequest::RequestScreenshot(R->Directory/(FString(Names[R->Capture])+TEXT(".png")),true,false);++R->Capture;}
            if(T>=4.5 && R->Capture==8)R->Stage=3;
        }
        if((R->Stage==3 || !R->Error.IsEmpty()) && R->PendingWrites.GetValue()==0)
        {
            auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Men,Landings;
            bool Passed=R->Error.IsEmpty() && R->Frames.Num()==8 && R->FailedWrites.GetValue()==0;
            for(int32 I=0;I<2;++I)
            {
                auto* U=R->Men[I].Get();auto Row=ThrowPoseV24(U);Row->SetBoolField(TEXT("held_observed_before_release"),R->HeldSeen[I]);
                Row->SetBoolField(TEXT("hidden_observed_after_release"),R->HiddenAfterRelease[I]);Men.Add(MakeShared<FJsonValueObject>(Row));
                Passed&=U && U->GrenadeThrowsStarted==1 && U->GrenadesReleased==1 && R->HeldSeen[I] && R->HiddenAfterRelease[I];
                Passed&=R->Landings[I] && R->Landings[I]->GetBoolField(TEXT("resolved")) && R->Landings[I]->GetBoolField(TEXT("fixed_target_unchanged"));
                if(R->Landings[I])Landings.Add(MakeShared<FJsonValueObject>(R->Landings[I]));
            }
            J->SetBoolField(TEXT("success"),Passed);J->SetStringField(TEXT("error"),R->Error);J->SetNumberField(TEXT("wall_seconds"),Wall);
            J->SetNumberField(TEXT("normal_camera_width_cm"),6000);J->SetNumberField(TEXT("failed_image_writes"),R->FailedWrites.GetValue());
            J->SetStringField(TEXT("scope"),TEXT("Arranged actual L_Frontline British/German assault actors at normal 6000 cm game camera; start times .28 s apart. Actual StartGrenadeThrow, animation Notify, unchanged aim dispersion, projectile flight and ResolveStrike. Enemy recruitment disabled for this short isolated fixture. Component visibility is recorded separately from pixel review; no natural-match/performance claim."));
            J->SetArrayField(TEXT("soldiers"),Men);J->SetArrayField(TEXT("landings"),Landings);J->SetArrayField(TEXT("captures"),R->Frames);
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            const bool Saved=FFileHelper::SaveStringToFile(Json,*(R->Directory/TEXT("grenade-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V24_GRENADE_REVIEW_COMPLETE success=%d saved=%d %s"),int32(Passed),int32(Saved),*R->Directory);
            UGameViewportClient::OnScreenshotCaptured().Remove(R->ScreenshotHandle);FPlatformMisc::RequestExit(false);return false;
        }
        return true;
    }));
}
