#include "WarfareFrontlineAudio.h"
#include "WarfareFrontline.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Camera/PlayerCameraManager.h"
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
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void StartFrontlineV18AudioVerification(AWarfareFrontlineController* Controller)
{
    if(!Controller||!FParse::Param(FCommandLine::Get(),TEXT("WFV18AudioVerify")))return;
    FString Prior; const bool Had=GConfig->GetString(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),Prior,GEngineIni);
    FApp::SetUnfocusedVolumeMultiplier(1.f);
    if(Had)GConfig->SetString(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),*Prior,GEngineIni);
    else GConfig->RemoveKey(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),GEngineIni);
    struct FReview
    {
        double Start=FPlatformTime::Seconds()+3,ExportAt=0;
        bool Recording=false,Exporting=false;
        int32 Step=0,Passed=0,PeakEffects=0,PeakVoices=0;
        FString Directory;TArray<TSharedPtr<FJsonValue>> Checks,Events;
        void Check(const TCHAR* Name,bool OK)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),OK);
            Checks.Add(MakeShared<FJsonValueObject>(J));Passed+=OK;
            UE_LOG(LogTemp,Display,TEXT("V18_AUDIO_CHECK %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Name);
        }
    };
    auto Run=MakeShared<FReview>();
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V18/Audio"));
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    const TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run,Weak](float)
    {
        auto* PC=Weak.Get();if(!PC||!PC->GetWorld())return false;
        auto* A=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        const double Now=FPlatformTime::Seconds();if(!A||Now<Run->Start)return true;
        if(!Run->Recording&&!Run->Exporting)
        {
            PC->DisableInput(PC);A->SetMuted(false);
            A->SetVolume(EFrontlineAudioBus::Master,.8f);A->SetVolume(EFrontlineAudioBus::Effects,.85f);
            A->SetVolume(EFrontlineAudioBus::Voices,.85f);A->SetVolume(EFrontlineAudioBus::Ambience,0);
            A->SetVolume(EFrontlineAudioBus::Music,0);A->SetBattleActive(true);
            Run->Check(TEXT("all_42_new_and_24_retained_assets_load"),A->GetLoadedAssetCount()==66);
            Run->Check(TEXT("master_peak_limiter_installed"),A->HasLimiter());
            Run->Check(TEXT("new_music_volume_bus_available"),FMath::IsNearlyZero(A->GetVolume(EFrontlineAudioBus::Music)));
            UAudioMixerBlueprintLibrary::StartRecordingOutput(PC,21.f);
            Run->Start=Now;Run->Recording=true;return true;
        }
        const double T=Now-Run->Start;
        const APlayerCameraManager* Cam=PC->PlayerCameraManager;
        const FVector Centre=Cam?Cam->GetCameraLocation():FVector::ZeroVector;
        const FVector RightAxis=Cam?Cam->GetCameraRotation().RotateVector(FVector::RightVector):FVector::ForwardVector;
        const FVector Left=Centre-RightAxis*2200,Right=Centre+RightAxis*2200;
        if(Run->Recording)
        {
            const double Schedule[]={1.,2.5,4.,5.5,7.2,8.4,9.5,9.6,9.7,9.8,9.9,10.,11.3,12.8,14.5,16.5,17.5,19.5};
            if(Run->Step<UE_ARRAY_COUNT(Schedule)&&T>=Schedule[Run->Step])
            {
                const int32 Step=Run->Step++;
                const TCHAR* Names[]={TEXT("rifle_left"),TEXT("rifle_right"),TEXT("pain_left"),TEXT("fatal_bullet_right"),
                    TEXT("shell_incoming"),TEXT("shell_impact"),TEXT("automatic_1"),TEXT("automatic_2"),TEXT("automatic_3"),
                    TEXT("automatic_4"),TEXT("automatic_5"),TEXT("automatic_6"),TEXT("german_advance"),TEXT("fatal_blast_left"),
                    TEXT("music_and_ambience"),TEXT("mute"),TEXT("muted_rifle"),TEXT("stop")};
                auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("name"),Names[Step]);E->SetNumberField(TEXT("seconds"),T);
                Run->Events.Add(MakeShared<FJsonValueObject>(E));
                switch(Step)
                {
                case 0:Run->Check(TEXT("left_pan"),A->GetPanForPosition(Left)<-.4f);A->PlayWeapon(false,false,Left,11);break;
                case 1:Run->Check(TEXT("right_pan"),A->GetPanForPosition(Right)>.4f);A->PlayWeapon(false,true,Right,21);break;
                case 2:Run->Check(TEXT("surviving_wound_plays_pain"),A->PlayCasualty(false,Left,111,false,false,30));break;
                case 3:Run->Check(TEXT("fatal_bullet_plays_death"),A->PlayCasualty(true,Right,211,true,false,90));break;
                case 4:A->PlayIncomingShell(Centre);break;
                case 5:A->PlayImpact(false,Centre);break;
                case 6:case 7:case 8:case 9:case 10:case 11:A->PlayWeapon(true,true,Right,22);break;
                case 12:Run->Check(TEXT("german_command_plays"),A->PlayVoice(EFrontlineVoiceEvent::Advance,true,Right,21));break;
                case 13:
                    Run->Check(TEXT("fatal_cry_not_blocked_by_command_cadence"),A->PlayCasualty(false,Left,112,true,true,100));
                    Run->Check(TEXT("simultaneous_casualty_spam_rejected"),!A->PlayCasualty(true,Right,213,true,true,100));break;
                case 14:A->SetVolume(EFrontlineAudioBus::Music,.6f);A->SetVolume(EFrontlineAudioBus::Ambience,.5f);break;
                case 15:A->SetMuted(true);break;
                case 16:A->PlayWeapon(false,false,Left,11);break;
                case 17:
                    Run->Check(TEXT("two_fatal_events_and_distinct_pain"),A->GetFatalEvents()==2&&A->GetCasualtyEvents()>=3);
                    Run->Check(TEXT("bounded_effect_pool_without_drops"),Run->PeakEffects>=2&&Run->PeakEffects<=32&&A->GetEffectsDropped()==0);
                    Run->Check(TEXT("bounded_separate_voice_pools"),Run->PeakVoices>=1&&Run->PeakVoices<=4);
                    UAudioMixerBlueprintLibrary::StopRecordingOutput(PC,EAudioRecordingExportType::WavFile,TEXT("v18-engine-master-mix"),Run->Directory);
                    Run->Recording=false;Run->Exporting=true;Run->ExportAt=Now;A->SetBattleActive(false);break;
                }
            }
            Run->PeakEffects=FMath::Max(Run->PeakEffects,A->GetActiveEffects());Run->PeakVoices=FMath::Max(Run->PeakVoices,A->GetActiveVoices());
            return true;
        }
        const FString Wav=Run->Directory/TEXT("v18-engine-master-mix.wav");
        if(Now-Run->ExportAt<1.|| (IFileManager::Get().FileSize(*Wav)<4096&&Now-Run->ExportAt<8.))return true;
        Run->Check(TEXT("actual_engine_mix_exported"),IFileManager::Get().FileSize(*Wav)>4096);
        auto Report=MakeShared<FJsonObject>();
        Report->SetNumberField(TEXT("passed"),Run->Passed);Report->SetNumberField(TEXT("total"),Run->Checks.Num());
        Report->SetBoolField(TEXT("all_passed"),Run->Passed==Run->Checks.Num());
        Report->SetArrayField(TEXT("checks"),Run->Checks);Report->SetArrayField(TEXT("events"),Run->Events);
        Report->SetStringField(TEXT("scope"),TEXT("Isolated UE audio event fixture and actual master mix; not a natural battle or human listening approval."));
        Report->SetStringField(TEXT("wav"),Wav);Report->SetNumberField(TEXT("peak_effects"),Run->PeakEffects);Report->SetNumberField(TEXT("peak_voices"),Run->PeakVoices);
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(Run->Directory/TEXT("audio-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("V18_AUDIO_COMPLETE passed=%d total=%d"),Run->Passed,Run->Checks.Num());
        FPlatformMisc::RequestExit(false);return false;
    }),.02f);
}
