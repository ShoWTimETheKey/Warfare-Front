#include "WarfareFrontline.h"
#include "Camera/CameraActor.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

TSharedRef<FJsonObject> RunFrontlineV21WeaponChecks(AWarfareFrontlineGameMode* Battle);

void StartFrontlineV21Review(AWarfareFrontlineController* Controller)
{
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV21Review")))return;
    struct FRun
    {
        int32 Stage=0,Passed=0,GermanWalker=INDEX_NONE;
        double Started=FPlatformTime::Seconds(),Next=Started+3.;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks,Images;
        TSharedPtr<FJsonObject> Weapons;
        void Check(const FString& Name,bool OK)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),OK);
            Checks.Add(MakeShared<FJsonValueObject>(J));Passed+=OK;
            UE_LOG(LogTemp,Display,TEXT("WF_V21_REVIEW %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);
        }
    };
    auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V21/Review")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        const double Now=FPlatformTime::Seconds();if(Now<Run->Next)return true;Run->Next=Now+.3;
        if(Run->Stage<99&&(!B||!HUD||!PC->BattleCamera||!B->AssetFailure.IsEmpty()||Now-Run->Started>55.))
        {Run->Check(TEXT("required_scene_resources_and_timeout"),false);Run->Stage=99;}
        auto Command=[&](const FString& Name){if(HUD)HUD->NotifyHitBoxClick(FName(*Name));};
        auto Snap=[&](const TCHAR* Name)
        {
            const FString Path=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,true,false);
            Run->Images.Add(MakeShared<FJsonValueString>(Path));Run->Next=Now+.8;
        };
        auto CameraX=[&](){return PC->BattleCamera?PC->BattleCamera->GetActorLocation().X:MAX_dbl;};
        switch(Run->Stage++)
        {
        case 0:PC->Restart();Command(TEXT("NewCampaign"));break;
        case 1:Command(TEXT("FactionBritish"));break;
        case 2:Command(TEXT("Begin"));break;
        case 3:
            if(B->Phase!=EFrontlinePhase::Fighting){--Run->Stage;break;}
            Run->Check(TEXT("british_campaign_begins_empty_on_left"),B->GetPlayerFaction()==EFrontlineFaction::British
                &&B->Units.IsEmpty()&&B->GetHomeTrenchIndex()==0&&B->Trenches[0].Owner==0
                &&CameraX()<0&&FMath::Abs(CameraX()-B->GetHomeCameraFocusX())<2);
            PC->RunV21CameraChecks([Run](const FString& N,bool OK){Run->Check(N,OK);});
            B->RunV21FactionChecks([Run](const FString& N,bool OK){Run->Check(TEXT("british_")+N,OK);});
            PC->ResetCamera();Run->Next=Now+1.;break;
        case 4:
        {
            Run->Weapons=RunFrontlineV21WeaponChecks(B);
            const TArray<TSharedPtr<FJsonValue>>* Checks=nullptr;
            const bool Has=Run->Weapons->TryGetArrayField(TEXT("checks"),Checks)&&Checks&&!Checks->IsEmpty();
            Run->Check(TEXT("actual_unit_weapon_review_returned_checks"),Has);
            if(Has)for(const auto& V:*Checks)
            {
                const auto J=V->AsObject();
                if(J)Run->Check(TEXT("weapons_")+J->GetStringField(TEXT("name")),J->GetBoolField(TEXT("passed")));
            }
            Snap(TEXT("01-british-console-home"));break;
        }
        case 5:PC->SelectTrench(B->GetHomeTrenchIndex()+B->GetTeamDirection(0));break;
        case 6:
            Run->Check(TEXT("british_trench_focus_looks_towards_the_front"),PC->SelectedTrench==1
                &&FMath::Abs(CameraX()-(B->Trenches[1].X+1800))<2);
            Command(TEXT("ResetCamera"));break;
        case 7:
            Run->Check(TEXT("british_home_button_restores_left_rear"),FMath::Abs(CameraX()-B->GetHomeCameraFocusX())<2);
            PC->Restart();Command(TEXT("NewCampaign"));break;
        case 8:Command(TEXT("FactionGerman"));break;
        case 9:Command(TEXT("Begin"));break;
        case 10:
            if(B->Phase!=EFrontlinePhase::Fighting){--Run->Stage;break;}
            Run->Check(TEXT("german_campaign_begins_empty_on_right"),B->GetPlayerFaction()==EFrontlineFaction::German
                &&B->Units.IsEmpty()&&B->GetHomeTrenchIndex()==4&&B->Trenches[4].Owner==0
                &&CameraX()>0&&FMath::Abs(CameraX()-B->GetHomeCameraFocusX())<2);
            B->RunV21FactionChecks([Run](const FString& N,bool OK){Run->Check(TEXT("german_")+N,OK);});
            PC->ResetCamera();Run->Next=Now+1.;break;
        case 11:
            Run->Check(TEXT("german_view_retains_screen_right_world_direction"),PC->BattleCamera->GetActorRightVector().X>.99f);
            Snap(TEXT("02-german-console-home"));break;
        case 12:
        {
            const int32 Home=B->GetHomeTrenchIndex();
            const auto Slots=B->GetTrenchSquadSlots(Home);Run->GermanWalker=Slots.IsEmpty()?INDEX_NONE:Slots[0];
            Command(FString::Printf(TEXT("SectionAdvance%d_%d"),Home,Run->GermanWalker));break;
        }
        case 13:
        {
            const auto* S=B->FindSquad(Run->GermanWalker);
            Run->Check(TEXT("german_portrait_order_advances_left_from_right_trench"),S&&S->Team==0&&S->bAdvancing
                &&S->Trench==INDEX_NONE&&S->Destination<4&&HUD->GetPresentationPage()==3);
            Snap(TEXT("03-german-direct-section-order"));break;
        }
        case 14:PC->SelectTrench(3);break;
        case 15:
            Run->Check(TEXT("german_trench_focus_looks_left_towards_the_front"),PC->SelectedTrench==3
                &&FMath::Abs(CameraX()-(B->Trenches[3].X-1800))<2);
            Command(TEXT("ResetCamera"));break;
        case 16:
            Run->Check(TEXT("german_home_button_restores_right_rear"),FMath::Abs(CameraX()-B->GetHomeCameraFocusX())<2);
            PC->Restart();break;
        case 17:Command(TEXT("MenuHome"));break;
        default:
        {
            int32 Written=0;for(const auto& V:Run->Images)if(IFileManager::Get().FileSize(*V->AsString())>0)++Written;
            Run->Check(TEXT("three_faction_and_control_screenshots_written"),Written==3);
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Run->Passed==Run->Checks.Num());
            J->SetNumberField(TEXT("checks_passed"),Run->Passed);J->SetNumberField(TEXT("checks_total"),Run->Checks.Num());
            J->SetArrayField(TEXT("checks"),Run->Checks);J->SetArrayField(TEXT("screenshots"),Run->Images);
            if(Run->Weapons)J->SetObjectField(TEXT("weapons"),Run->Weapons);
            J->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);
            J->SetStringField(TEXT("scope"),TEXT("Focused packaged-game review: actual campaign/HUD dispatches, faction spawn/direction and trench-route fixtures, real units for weapon selection and target acquisition, production camera input/integration checks with synthetic coordinates at 30/60/120fps. Three normal-scale UI captures with frozen arranged squads. No physical cursor manipulation, subjective feel certification, natural-match balance, audio review or performance benchmark. Isolated UserDir and volatile progress."));
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("v21-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V21/review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V21_REVIEW_COMPLETE passed=%d total=%d %s"),Run->Passed,Run->Checks.Num(),*Run->Directory);
            if(HUD&&HUD->GetPresentationPage()==0)Command(TEXT("QuitGame"));else FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
