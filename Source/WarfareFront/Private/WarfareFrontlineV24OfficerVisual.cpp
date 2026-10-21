#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
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
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AWarfareFrontlineGameMode::BeginV24OfficerVisual()
{
    FString User;
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV24OfficerVisual")) || !bProgressionVolatile
        || !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User) || User.IsEmpty())
    {FPlatformMisc::RequestExit(false);return;}
    struct FRun
    {
        int32 Stage=0;bool Ready=true;double Started=FPlatformTime::Seconds(),Next=Started+1,ActionAt=0;
        FString Folder,Failure;float CentreX=0;
        TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Officers;
        TArray<TSharedPtr<FJsonValue>> Captures;
        FDelegateHandle Screenshot;FThreadSafeCounter Pending,Failed;
    };
    const auto Run=MakeShared<FRun,ESPMode::ThreadSafe>();
    Run->Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V24/OfficerVisual")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Folder,true);
    FModuleManager::Get().LoadModuleChecked(TEXT("ImageWrapper"));
    Run->Screenshot=UGameViewportClient::OnScreenshotCaptured().AddLambda([Run](int32 Width,int32 Height,const TArray<FColor>& Pixels)
    {
        const FString Path=FScreenshotRequest::GetFilename();if(!Path.StartsWith(Run->Folder))return;
        TArray<FColor> Copy=Pixels;Run->Pending.Increment();
        Async(EAsyncExecution::ThreadPool,[Run,Path,Width,Height,Copy=MoveTemp(Copy)]()
        {
            TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Width,Height,Copy,PNG);
            if(PNG.IsEmpty() || !FFileHelper::SaveArrayToFile(PNG,*Path))Run->Failed.Increment();Run->Pending.Decrement();
        });
    });
    TWeakObjectPtr<AWarfareFrontlineGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* B=Weak.Get();if(!B){UGameViewportClient::OnScreenshotCaptured().Remove(Run->Screenshot);return false;}
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;
        auto* PC=Cast<AWarfareFrontlineController>(B->GetWorld()->GetFirstPlayerController());
        if(!PC || !PC->BattleCamera)return true;
        if(Now-Run->Started>22 || !B->AssetFailure.IsEmpty())
        {Run->Ready=false;Run->Failure=B->AssetFailure.IsEmpty()?TEXT("Timed out"):B->AssetFailure;Run->Stage=99;}
        auto Camera=[&](float Width,float Height)
        {
            const FVector Focus(Run->CentreX,0,Height),Location(Run->CentreX,14000,Height+4000);
            PC->BattleCamera->SetActorLocation(Location);PC->BattleCamera->SetActorRotation((Focus-Location).Rotation());
            PC->BattleCamera->GetCameraComponent()->OrthoWidth=Width;PC->SetViewTarget(PC->BattleCamera);
        };
        auto Snap=[&](const TCHAR* Label)
        {
            const FString Path=Run->Folder/(FString(Label)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,false,false);
            const auto Shot=MakeShared<FJsonObject>();Shot->SetStringField(TEXT("file"),Path);Shot->SetNumberField(TEXT("camera_width_cm"),PC->BattleCamera->GetCameraComponent()->OrthoWidth);
            Shot->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);Shot->SetNumberField(TEXT("action_age_seconds"),Now-Run->ActionAt);
            TArray<TSharedPtr<FJsonValue>> Units;
            for(const auto& Entry:Run->Officers)if(const auto* U=Entry.Get())
            {
                const auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("faction"),U->GetFaction()==EFrontlineFaction::British?TEXT("British"):TEXT("German"));
                J->SetStringField(TEXT("mesh"),GetNameSafe(U->Soldier->GetSkeletalMeshAsset()));J->SetNumberField(TEXT("weapon_phase"),int32(U->WeaponPhase));
                J->SetBoolField(TEXT("commissioned"),U->IsCommissionedOfficer());J->SetNumberField(TEXT("rounds"),U->RoundsRemaining);
                if(auto* Node=U->Soldier->GetSingleNodeInstance())
                {J->SetStringField(TEXT("animation"),GetNameSafe(Node->GetCurrentAsset()));J->SetNumberField(TEXT("animation_time"),Node->GetCurrentTime());}
                Units.Add(MakeShared<FJsonValueObject>(J));
            }
            Shot->SetArrayField(TEXT("officers"),Units);Run->Captures.Add(MakeShared<FJsonValueObject>(Shot));
        };
        switch(Run->Stage++)
        {
        case 0:
        {
            PC->DisableInput(PC);PC->SetActorTickEnabled(false);if(auto* HUD=PC->GetHUD())HUD->bShowHUD=false;
            B->StartNewCampaign(EFrontlineFaction::British);B->ActiveResearchMutable().TechnologyMask|=1u<<9;
            for(auto& Campaign:B->CampaignRuns)if(Campaign.Id==B->ActiveCampaignId)Campaign.CompletedMissions=1;
            Run->Ready&=B->SelectCampaignMission(1);B->BeginBattle();B->SetActorTickEnabled(false);
            Run->CentreX=B->Trenches[1].X+550.f;
            Run->Ready&=B->SpawnSquad(0,EFrontlineRole::Officer,1) && B->SpawnSquad(1,EFrontlineRole::Officer,3);
            for(AWarfareFrontlineSoldier* U:B->Units)
            {
                U->SetActorTickEnabled(false);const float X=Run->CentreX+(U->Team?170.f:-170.f)+U->MemberIndex*(U->Team?120.f:-120.f);
                const float Y=U->MemberIndex==0?0.f:-240.f;const float Z=AWarfareFrontlineGround::HeightAt(X,Y);
                U->SetActorLocation(FVector(X,Y,Z+88));U->SetActorRotation(FRotator(0,U->Team?160.f:20.f,0));U->ReturnToIdle();
                // Keep the actual destination equal to the staging position so
                // manual reload ticks cannot send the staged group back to its trench.
                const FVector Destination=B->SoldierDestination(U);U->FormationOffset+=U->Feet()-Destination;U->FormationOffset.Z=0;
                if(U->IsSectionLeader()){Run->Officers.Add(U);Run->Ready&=U->IsCommissionedOfficer();}
            }
            Run->Ready&=Run->Officers.Num()==2 && B->Phase==EFrontlinePhase::Fighting;
            Camera(6000,650);Run->Next=Now+2.5;break;
        }
        case 1:Snap(TEXT("01-normal-scale"));Run->Next=Now+.2;break;
        case 2:Camera(900,115);Run->Next=Now+1.2;break;
        case 3:Snap(TEXT("02-officer-equipment"));Run->Next=Now+.6;break;
        case 4:
            for(AWarfareFrontlineSoldier* U:B->Units)if(!U->IsSectionLeader()){U->Suppression=.6f;U->LastSectionOrderAt=-100;}
            B->Elapsed=20;B->UpdateSectionOrders();Run->ActionAt=Now;Run->Next=Now+.55;break;
        case 5:Snap(TEXT("03-command"));Run->Next=Now+.7;break;
        case 6:
            for(const auto& Entry:Run->Officers)if(auto* U=Entry.Get())
            {
                U->ReturnToIdle();U->RoundsRemaining=0;U->Suppression=0;
                for(int32 I=0;I<120 && U->WeaponPhase!=EFrontlineWeaponPhase::Reload;++I){B->Elapsed+=.05f;U->Tick(.05f);}
                Run->Ready&=U->WeaponPhase==EFrontlineWeaponPhase::Reload;
            }
            Run->ActionAt=Now;Run->Next=Now+1.45;break;
        case 7:Snap(TEXT("04-pistol-reload"));Run->Next=Now+1.;break;
        default:
        {
            if(Run->Pending.GetValue()>0 && Now-Run->Started<22){Run->Next=Now+.2;break;}
            int32 Written=0;for(const auto& Capture:Run->Captures)Written+=IFileManager::Get().FileExists(*Capture->AsObject()->GetStringField(TEXT("file")))?1:0;
            const auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("fixture_ready"),Run->Ready && Written==4 && Run->Failed.GetValue()==0);
            Report->SetStringField(TEXT("failure"),Run->Failure);Report->SetArrayField(TEXT("captures"),Run->Captures);Report->SetNumberField(TEXT("screenshots_written"),Written);
            Report->SetStringField(TEXT("scope"),TEXT("Isolated character visual staging on real L_Frontline terrain and materials. Actual commissioned squads, original body scales, normal 6000cm shot and explicit 900cm diagnostic close camera. HUD hidden; units/AI held for comparison. Synthetic research/mission unlock, formation positions and pressure; real command and empty-pistol reload paths. Component clip names are evidence of state, not visual acceptance; inspect all four images. No battle performance or death-physics claim."));
            FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(Run->Folder/TEXT("officer-visual.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UGameViewportClient::OnScreenshotCaptured().Remove(Run->Screenshot);UE_LOG(LogTemp,Display,TEXT("WF_V24_OFFICER_VISUAL %s %s"),Run->Ready && Written==4?TEXT("READY"):TEXT("FAILED"),*Run->Folder);
            FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
