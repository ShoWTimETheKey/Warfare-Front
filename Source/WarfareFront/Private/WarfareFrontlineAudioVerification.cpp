#include "WarfareFrontlineAudio.h"
#include "WarfareFrontline.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FAudioReview
{
    int32 Stage = 0, Passed = 0, MGShots = 0, PeakEffects = 0, PeakVoices = 0;
    double Next = FPlatformTime::Seconds()+2., CaptureStarted = 0., ExportStarted = 0.;
    bool bReadback = false, bRecording = false, bStopped = false;
    bool bLeft = false, bRight = false, bCover = false, bGerman = false, bShell = false, bWound = false, bSilentProbe = false;
    FString Directory;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Checks, Events;
    void Check(const TCHAR* Name, bool bOK, const FString& Detail = FString())
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("name"), Name); Row->SetBoolField(TEXT("passed"), bOK); Row->SetStringField(TEXT("detail"), Detail);
        Checks.Add(MakeShared<FJsonValueObject>(Row)); Passed += bOK ? 1 : 0;
        UE_LOG(LogTemp, Display, TEXT("V14_AUDIO_CHECK %s %s %s"), bOK ? TEXT("PASS") : TEXT("FAIL"), Name, *Detail);
    }
    void Event(const TCHAR* Name)
    {
        auto Row = MakeShared<FJsonObject>(); Row->SetStringField(TEXT("name"), Name);
        Row->SetNumberField(TEXT("seconds_from_capture_start"), FPlatformTime::Seconds()-CaptureStarted);
        Events.Add(MakeShared<FJsonValueObject>(Row));
    }
    void Finish(UFrontlineAudioSubsystem* Audio)
    {
        Report->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());
        Report->SetBoolField(TEXT("readback_process"),bReadback);
        Report->SetStringField(TEXT("settings_ini"),GGameUserSettingsIni);
        Report->SetStringField(TEXT("scope"),TEXT("Opt-in isolated settings HUD dispatch, no physical mouse. Audio emitter fixture in briefing, not natural combat. WAV is an actual UE master-submix recording; decoded stereo/mute/loudness checks are separate."));
        Report->SetNumberField(TEXT("passed"),Passed); Report->SetNumberField(TEXT("total"),Checks.Num());
        Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());
        Report->SetArrayField(TEXT("checks"),Checks); Report->SetArrayField(TEXT("capture_events"),Events);
        Report->SetNumberField(TEXT("peak_active_effects"),PeakEffects); Report->SetNumberField(TEXT("peak_active_voices"),PeakVoices);
        Report->SetNumberField(TEXT("played_weapon_events"),Audio->GetWeaponEvents());
        Report->SetNumberField(TEXT("dropped_effects"),Audio->GetEffectsDropped());
        Report->SetStringField(TEXT("master_mix_wav"),bReadback ? Directory/TEXT("audio-master-mix.wav") : FString());
        FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        const FString File=Directory/(bReadback ? TEXT("audio-readback.json") : TEXT("audio-verification.json"));
        FFileHelper::SaveStringToFile(Json,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("V14_AUDIO_REVIEW_COMPLETE passed=%d total=%d path=%s"),Passed,Checks.Num(),*File);
        FPlatformMisc::RequestExit(false);
    }
};
}

void StartFrontlineV14AudioVerification(AWarfareFrontlineController* Controller)
{
    const bool bVerify=FParse::Param(FCommandLine::Get(),TEXT("WFV14AudioVerify"));
    const bool bReadback=FParse::Param(FCommandLine::Get(),TEXT("WFV14AudioReadback"));
    if ((!bVerify && !bReadback) || !Controller) return;
    // The opt-in recording fixture runs in a hidden review window. UE's setter
    // also writes its config cache: preserve that key while retaining the now
    // initialized in-process multiplier, so a later config flush changes nothing.
    if (bReadback)
    {
        FString PriorValue;
        const bool bHadKey=GConfig && GConfig->GetString(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),PriorValue,GEngineIni);
        FApp::SetUnfocusedVolumeMultiplier(1.f);
        if (GConfig)
        {
            if (bHadKey) GConfig->SetString(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),*PriorValue,GEngineIni);
            else GConfig->RemoveKey(TEXT("Audio"),TEXT("UnfocusedVolumeMultiplier"),GEngineIni);
        }
    }
    auto Run=MakeShared<FAudioReview>(); Run->bReadback=bReadback;
    Run->Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V14"));
    IFileManager::Get().MakeDirectory(*Run->Directory,true);
    Run->Report->SetStringField(TEXT("run_id"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const TWeakObjectPtr<AWarfareFrontlineController> Weak(Controller);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float)
    {
        auto* PC=Weak.Get(); if (!PC || !PC->GetWorld()) return false;
        auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
        auto* Audio=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
        const double Now=FPlatformTime::Seconds();
        if (!HUD || !Audio || Now<Run->Next) return true;
        auto Command=[&](const TCHAR* Name) { HUD->NotifyHitBoxClick(FName(Name)); };
        if (!Run->bRecording && !Run->bStopped)
        {
            Run->Next=Now+.3; // At most one HUD command in each rendered frame.
            if (!Run->bReadback)
            {
                switch (Run->Stage++)
                {
                    case 0: PC->DisableInput(PC); Command(TEXT("Settings")); break;
                    case 1: Run->Check(TEXT("settings_page_opens"),HUD->GetPresentationPage()==7); Command(TEXT("SettingsAudio")); break;
                    case 2: Command(TEXT("AudioLevel0_6")); break;
                    case 3: Command(TEXT("AudioLevel1_7")); break;
                    case 4: Command(TEXT("AudioLevel2_8")); break;
                    case 5: Command(TEXT("AudioLevel3_4")); break;
                    case 6: if (!Audio->IsMuted()) Command(TEXT("AudioMute")); break;
                    case 7:
                    {
                        const float Expected[]={.6f,.7f,.8f,.4f};
                        for (int32 I=0;I<4;++I) Run->Check(*FString::Printf(TEXT("hud_sets_audio_bus_%d"),I),FMath::IsNearlyEqual(Audio->GetVolume(EFrontlineAudioBus(I)),Expected[I],.001f));
                        Run->Check(TEXT("hud_mutes_audio"),Audio->IsMuted());
                        Audio->SetBattleActive(true);
                        Run->Check(TEXT("all_24_audio_assets_load"),Audio->GetLoadedAssetCount()==24,FString::FromInt(Audio->GetLoadedAssetCount()));
                        Audio->SetBattleActive(false);
                        Run->Finish(Audio); return false;
                    }
                }
            }
            else
            {
                switch (Run->Stage++)
                {
                    case 0:
                    {
                        PC->DisableInput(PC);
                        const float Expected[]={.6f,.7f,.8f,.4f};
                        for (int32 I=0;I<4;++I) Run->Check(*FString::Printf(TEXT("bus_%d_survives_process_restart"),I),FMath::IsNearlyEqual(Audio->GetVolume(EFrontlineAudioBus(I)),Expected[I],.001f));
                        Run->Check(TEXT("mute_survives_process_restart"),Audio->IsMuted());
                        Command(TEXT("Settings")); break;
                    }
                    case 1: Run->Check(TEXT("readback_settings_page_opens"),HUD->GetPresentationPage()==7); Command(TEXT("SettingsAudio")); break;
                    case 2: if (Audio->IsMuted()) Command(TEXT("AudioMute")); break;
                    case 3: Run->Check(TEXT("hud_unmutes_after_readback"),!Audio->IsMuted()); Command(TEXT("DisplayBack")); break;
                    case 4:
                        Audio->SetBattleActive(true);
                        Run->Check(TEXT("all_24_audio_assets_load_after_restart"),Audio->GetLoadedAssetCount()==24,FString::FromInt(Audio->GetLoadedAssetCount()));
                        Run->Check(TEXT("screen_left_source_pans_left"),Audio->GetPanForPosition(FVector(-2400,0,160))<-.05f,FString::SanitizeFloat(Audio->GetPanForPosition(FVector(-2400,0,160))));
                        Run->Check(TEXT("screen_right_source_pans_right"),Audio->GetPanForPosition(FVector(2400,0,160))>.05f,FString::SanitizeFloat(Audio->GetPanForPosition(FVector(2400,0,160))));
                        UAudioMixerBlueprintLibrary::StartRecordingOutput(PC,14.f);
                        Run->CaptureStarted=FPlatformTime::Seconds(); Run->bRecording=true; Run->Next=0.;
                        Run->Event(TEXT("recording_started")); break;
                }
            }
            return true;
        }
        if (Run->bRecording)
        {
            const double T=Now-Run->CaptureStarted;
            const FVector Left(-2400,0,160),Right(2400,0,160),Centre(0,0,160);
            if (T>=1. && !Run->bLeft)
            { Run->bLeft=true; Audio->PlayWeapon(false,false,Left,101); Run->Event(TEXT("rifle_left")); }
            if (T>=2.3 && !Run->bRight)
            { Run->bRight=true; Audio->PlayWeapon(false,true,Right,201); Run->Event(TEXT("rifle_right")); }
            if (T>=3.7 && !Run->bCover)
            {
                Run->bCover=true;
                Run->Check(TEXT("meaningful_cover_voice_accepted"),Audio->PlayVoice(EFrontlineVoiceEvent::Cover,false,Left,101));
                Run->Check(TEXT("simultaneous_contact_voice_rejected"),!Audio->PlayVoice(EFrontlineVoiceEvent::Contact,false,Right,202));
                Run->Event(TEXT("cover_voice"));
            }
            if (T>=4.7+Run->MGShots*.1 && Run->MGShots<12)
            { Audio->PlayWeapon(true,false,Left,102); ++Run->MGShots; Run->Event(TEXT("machinegun_single")); }
            if (T>=7.0 && !Run->bGerman)
            {
                Run->bGerman=true;
                Run->Check(TEXT("genuine_german_advance_accepted_after_cooldown"),Audio->PlayVoice(EFrontlineVoiceEvent::Advance,true,Right,201));
                Run->Check(TEXT("same_german_advance_is_not_repeated"),!Audio->PlayVoice(EFrontlineVoiceEvent::Advance,true,Right,202));
                Run->Event(TEXT("german_advance_voice"));
            }
            if (T>=8.2 && !Run->bShell)
            { Run->bShell=true; Audio->PlayImpact(false,Centre); Run->Event(TEXT("shell_centre")); }
            if (T>=10.3 && !Run->bWound)
            {
                Run->bWound=true;
                Run->Check(TEXT("wound_event_accepted_after_cooldown"),Audio->PlayVoice(EFrontlineVoiceEvent::Wounded,false,Right,201));
                Run->Event(TEXT("wounded_voice"));
            }
            if (T>=11.9 && !Audio->IsMuted()) { Audio->SetMuted(true); Run->Event(TEXT("mute_started")); }
            if (T>=12.3 && !Run->bSilentProbe)
            { Run->bSilentProbe=true; Audio->PlayWeapon(false,false,Left,101); Run->Event(TEXT("muted_rifle_probe")); }
            Run->PeakEffects=FMath::Max(Run->PeakEffects,Audio->GetActiveEffects());
            Run->PeakVoices=FMath::Max(Run->PeakVoices,Audio->GetActiveVoices());
            if (T>=13.5)
            {
                Run->Check(TEXT("all_12_automatic_events_reach_audio_pool"),Run->MGShots==12 && Audio->GetWeaponEvents()==15,FString::FromInt(Audio->GetWeaponEvents()));
                Run->Check(TEXT("recording_fixture_has_no_dropped_effects"),Audio->GetEffectsDropped()==0);
                Run->Check(TEXT("real_effect_components_were_active"),Run->PeakEffects>=2 && Run->PeakEffects<=32,FString::FromInt(Run->PeakEffects));
                Run->Check(TEXT("real_voice_components_were_active_within_budget"),Run->PeakVoices>=1 && Run->PeakVoices<=2,FString::FromInt(Run->PeakVoices));
                Run->Check(TEXT("voice_limiter_rejects_repeats"),Audio->GetVoiceRejected()>=2 && Audio->GetVoiceEvents()==3);
                UAudioMixerBlueprintLibrary::StopRecordingOutput(PC,EAudioRecordingExportType::WavFile,TEXT("audio-master-mix"),Run->Directory);
                Run->ExportStarted=Now; Run->bRecording=false; Run->bStopped=true;
                Audio->SetMuted(false); Audio->SetBattleActive(false);
                Run->Check(TEXT("final_readback_profile_is_unmuted"),!Audio->IsMuted());
            }
            return true;
        }
        const FString Wav=Run->Directory/TEXT("audio-master-mix.wav");
        if (Now-Run->ExportStarted>=1. && (IFileManager::Get().FileSize(*Wav)>4096 || Now-Run->ExportStarted>8.))
        {
            Run->Check(TEXT("engine_master_mix_wav_written"),IFileManager::Get().FileSize(*Wav)>4096,Wav);
            Run->Finish(Audio); return false;
        }
        return true;
    }),.02f);
}
