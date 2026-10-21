#include "WarfareFrontlineVoiceContext.h"
#include "WarfareFrontline.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void StartFrontlineV20Review(AWarfareFrontlineController* Controller)
{
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV20Review")))return;
    FString Prior;const bool Had=GConfig->GetString(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),Prior,GEngineIni);
    FApp::SetUnfocusedVolumeMultiplier(1.f);
    if(Had)GConfig->SetString(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),*Prior,GEngineIni);
    else GConfig->RemoveKey(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),GEngineIni);
    struct FRun
    {
        int32 Stage=0,Passed=0,Walker=INDEX_NONE,PeakVoices=0,AudioStep=0;
        double Started=FPlatformTime::Seconds(),Next=Started+3.,AudioStarted=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks,Images,Events;
        void Check(const FString& Name,bool OK)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),OK);
            Checks.Add(MakeShared<FJsonValueObject>(J));Passed+=OK;
            UE_LOG(LogTemp,Display,TEXT("WF_V20_REVIEW %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);
        }
    };
    auto Run=MakeShared<FRun>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V20/Review")/FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC)return false;
        auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        auto* A=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        const double Now=FPlatformTime::Seconds();if(A)Run->PeakVoices=FMath::Max(Run->PeakVoices,A->GetActiveVoices());
        if(Now<Run->Next)return true;Run->Next=Now+.25;
        if(Run->Stage<99&&(!B||!HUD||!A||!B->AssetFailure.IsEmpty()||Now-Run->Started>65.))
        {Run->Check(TEXT("required_scene_resources_and_timeout"),false);Run->Stage=99;}
        auto Command=[&](const FString& Name){if(HUD)HUD->NotifyHitBoxClick(FName(*Name));};
        auto Snap=[&](const TCHAR* Name)
        {
            const FString Path=Run->Directory/(FString(Name)+TEXT(".png"));FScreenshotRequest::RequestScreenshot(Path,true,false);
            Run->Images.Add(MakeShared<FJsonValueString>(Path));
        };
        switch(Run->Stage++)
        {
        case 0:PC->Restart();PC->ResetCamera();Command(TEXT("NewCampaign"));break;
        case 1:Command(TEXT("FactionBritish"));break;
        case 2:Command(TEXT("Begin"));break;
        case 3:
            if(B->Phase!=EFrontlinePhase::Fighting){--Run->Stage;break;}
            Run->Check(TEXT("real_operation_preparation_completed"),HUD->GetPresentationPage()==3&&B->AssetFailure.IsEmpty());
            B->RunV20TrenchChecks([Run](const FString& Name,bool OK){Run->Check(Name,OK);});
            A->RunV20VoiceChecks([Run](const FString& Name,bool OK){Run->Check(Name,OK);});
            PC->ResetCamera();PC->SetMouseLocation(1000,100);Run->Next=Now+1.5;break;
        case 4:
        {
            const auto Slots=B->GetTrenchSquadSlots(0);Run->Walker=Slots.IsValidIndex(0)?Slots[0]:INDEX_NONE;
            Run->Check(TEXT("three_rear_and_three_front_portraits_ready"),B->TrenchSquadCount(0,0)==3&&B->TrenchSquadCount(1,0)==3);
            Snap(TEXT("01-direct-trench-portraits"));Run->Next=Now+.8;break;
        }
        case 5:Command(FString::Printf(TEXT("SectionAdvance0_%d"),Run->Walker));break;
        case 6:
        {
            const auto* S=B->FindSquad(Run->Walker);
            Run->Check(TEXT("single_portrait_click_exits_despite_full_frontline"),S&&S->bAdvancing&&S->Trench==INDEX_NONE
                &&B->TrenchSquadCount(0,0)==2&&B->TrenchSquadCount(1,0)==3&&HUD->GetPresentationPage()==3);
            Snap(TEXT("02-one-click-section-exit"));Run->Next=Now+.8;break;
        }
        case 7:Command(TEXT("TrenchEntry1"));break;
        case 8:
            Run->Check(TEXT("single_entry_gate_click_keeps_existing_garrison"),B->Trenches[1].bBypass&&B->TrenchSquadCount(1,0)==3);
            Snap(TEXT("03-trench-entry-closed"));Run->Next=Now+.8;break;
        case 9:
            A->SetBattleActive(false);A->SetMuted(false);A->SetVolume(EFrontlineAudioBus::Master,.8f);A->SetVolume(EFrontlineAudioBus::Voices,.85f);
            A->SetVolume(EFrontlineAudioBus::Effects,0);A->SetVolume(EFrontlineAudioBus::Ambience,0);A->SetVolume(EFrontlineAudioBus::Music,0);A->SetBattleActive(true);
            UAudioMixerBlueprintLibrary::StartRecordingOutput(PC,25.f);Run->AudioStarted=Now;break;
        case 10:
        {
            const double T=Now-Run->AudioStarted;
            const double Times[]={0.,4.,8.,12.,16.,20.,22.};
            if(Run->AudioStep<7&&T>=Times[Run->AudioStep])
            {
                const int32 I=Run->AudioStep++;const FVector P(B->Trenches[0].X,0,0);
                auto E=MakeShared<FJsonObject>();E->SetNumberField(TEXT("step"),I);E->SetNumberField(TEXT("time"),T);
                const int32 Before=A->GetVoiceEvents();
                switch(I)
                {
                case 0:WarnFrontlineSoldiers(PC->GetWorld(),EFrontlineVoiceEvent::IncomingArtillery,P,1600.f);break;
                case 1:A->PlayVoice(EFrontlineVoiceEvent::ReloadCover,false,P,8101);break;
                case 2:A->PlayVoice(EFrontlineVoiceEvent::ArriveTrench,false,P,8102);break;
                case 3:A->PlayVoice(EFrontlineVoiceEvent::Advance,true,P,8103);break;
                case 4:
                {
                    const int32 BeforeCasualties=A->GetCasualtyEvents();
                    for(int32 N=0;N<32;++N)A->PlayCasualty(false,P,9000+N,true,false,100.f,8500+N);
                    const int32 Accepted=A->GetCasualtyEvents()-BeforeCasualties;
                    Run->Check(TEXT("simultaneous_casualties_do_not_create_cry_chorus"),Accepted<=1);E->SetNumberField(TEXT("accepted_casualties_of_32"),Accepted);break;
                }
                case 5:A->SetMuted(true);break;
                case 6:UAudioMixerBlueprintLibrary::StopRecordingOutput(PC,EAudioRecordingExportType::WavFile,TEXT("v20-voice-engine-mix"),Run->Directory);Run->Next=Now+1.;break;
                }
                E->SetNumberField(TEXT("accepted_voice_delta"),A->GetVoiceEvents()-Before);Run->Events.Add(MakeShared<FJsonValueObject>(E));
            }
            if(Run->AudioStep<7)--Run->Stage;break;
        }
        case 11:
            Run->Check(TEXT("voice_engine_mix_exported"),IFileManager::Get().FileSize(*(Run->Directory/TEXT("v20-voice-engine-mix.wav")))>4096);
            Run->Check(TEXT("maximum_two_simultaneous_human_voices"),Run->PeakVoices<=2);
            PC->Restart();break;
        case 12:Command(TEXT("MenuHome"));break;
        default:
        {
            int32 Written=0;for(const auto& V:Run->Images)if(IFileManager::Get().FileSize(*V->AsString())>0)++Written;
            Run->Check(TEXT("three_affected_ui_screenshots_written"),Written==3);
            auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Run->Passed==Run->Checks.Num());
            J->SetNumberField(TEXT("checks_passed"),Run->Passed);J->SetNumberField(TEXT("checks_total"),Run->Checks.Num());
            J->SetArrayField(TEXT("checks"),Run->Checks);J->SetArrayField(TEXT("screenshots"),Run->Images);J->SetArrayField(TEXT("audio_events"),Run->Events);
            J->SetStringField(TEXT("mix"),Run->Directory/TEXT("v20-voice-engine-mix.wav"));J->SetNumberField(TEXT("wall_seconds"),Now-Run->Started);
            J->SetStringField(TEXT("scope"),TEXT("Targeted live game APIs with explicit relocation and occupancy fixtures; actual spaced HUD dispatches and three normal-scale screenshots. Independent local audio scheduler fixtures plus short actual UE voice-bus recording, with probabilistic dispatch retained. Not natural battle duration, audio ear review, or motion/performance acceptance. Isolated UserDir and volatile campaign; no prior whole suites."));
            FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
            FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("v20-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V20/review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogTemp,Display,TEXT("WF_V20_REVIEW_COMPLETE passed=%d total=%d %s"),Run->Passed,Run->Checks.Num(),*Run->Directory);
            if(HUD&&HUD->GetPresentationPage()==0)Command(TEXT("QuitGame"));else FPlatformMisc::RequestExit(false);return false;
        }
        }
        return true;
    }));
}
