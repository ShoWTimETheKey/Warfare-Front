#include "WarfareFrontlineRenderSettingsV87Checks.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFRRUCampaignV82.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#if WITH_EDITOR
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"

struct FFrontlineRenderSettingsV87Review
{
    static FString FileHash(const FString& File)
    {
        if(!FPaths::FileExists(File))return TEXT("missing");
        TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File))return TEXT("unreadable");
        return FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString();
    }
    static TMap<FString,FString> PlayerHashes()
    {
        TMap<FString,FString> Result;const FString Local=FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
        if(Local.IsEmpty())return Result;
        for(const TCHAR* Name:{TEXT("Warfare1917"),TEXT("WarfareFront")})
        {
            const FString Root=FPaths::Combine(Local,Name);TArray<FString> Files;
            IFileManager::Get().FindFilesRecursive(Files,*Root,TEXT("*"),true,false);
            for(const FString& File:Files)Result.Add(File,FileHash(File));
        }
        return Result;
    }
    static TSharedRef<FJsonObject> HashJson(const TMap<FString,FString>& Values)
    {auto J=MakeShared<FJsonObject>();for(const auto& Item:Values)J->SetStringField(Item.Key,Item.Value);return J;}
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> External)
    {
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Skips;bool Good=true;
        const auto Check=[&](const FString& N,bool Pass){auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),N);Row->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(Row));Good&=Pass;External(N,Pass);};
        const auto Skip=[&](const TCHAR* N){Skips.Add(MakeShared<FJsonValueString>(N));};
        FString User;auto* R=B?UFrontlineRenderSettingsSubsystem::Get(B->GetWorld()):nullptr;
        const bool Gate=B&&R&&IsInGameThread()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV87SettingsOnly"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FrontlineUltimateArchiveV8215::ReviewUserDir(User)&&R->bNoSave&&R->bApplied;
        Check(TEXT("v87_settings_actual_applied_editor_disposable_nosave_gate"),Gate);
        const auto Finish=[&](){J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("skips"),Skips);
            J->SetStringField(TEXT("scope"),TEXT("Actual installed render setters: same requests, one supported change, Native/RR and FG/Reflex coupling, explicit Reapply, and same-value dirty save into a NEW owned scratch INI. The original NoSave branch and player files remain protected. Call counts are GT work/Flush calls, not SDK option submissions, visual acceptance or human click latency."));return J;};
        if(!Gate)return Finish();
        const auto OriginalQuality=R->Quality;const auto OriginalGeneration=R->FrameGeneration;const auto OriginalReflex=R->Reflex;
        const auto OriginalPreset=R->GraphicsPreset;const bool OriginalRR=R->bRayReconstruction;
        const double OriginalResume=R->FrameGenerationResumeAt;const FString OriginalIni=GGameUserSettingsIni;
        const FString OriginalIniHash=FileHash(OriginalIni);const auto OriginalPlayers=PlayerHashes();
        bool Readable=OriginalIniHash!=TEXT("unreadable");for(const auto& Item:OriginalPlayers)Readable&=Item.Value!=TEXT("unreadable");
        Check(TEXT("v87_settings_original_global_and_player_hashes_are_readable"),Readable);if(!Readable)return Finish();
        J->SetObjectField(TEXT("before"),R->GetDiagnostics());J->SetStringField(TEXT("original_ini"),OriginalIni);
        J->SetStringField(TEXT("original_ini_sha1_before"),OriginalIniHash);J->SetObjectField(TEXT("player_sha1_before"),HashJson(OriginalPlayers));
        ON_SCOPE_EXIT
        {
            GGameUserSettingsIni=OriginalIni;R->bNoSave=true;
            R->Quality=OriginalQuality;R->FrameGeneration=OriginalGeneration;R->Reflex=OriginalReflex;
            R->GraphicsPreset=OriginalPreset;R->bRayReconstruction=OriginalRR;R->FrameGenerationResumeAt=OriginalResume;
            R->Reapply();
            Check(TEXT("v87_settings_original_requested_fields_and_ini_target_restored"),R->Quality==OriginalQuality&&R->FrameGeneration==OriginalGeneration
                &&R->Reflex==OriginalReflex&&R->GraphicsPreset==OriginalPreset&&R->bRayReconstruction==OriginalRR&&R->bNoSave&&GGameUserSettingsIni==OriginalIni);
            const FString After=FileHash(OriginalIni);const auto PlayersAfter=PlayerHashes();bool PlayersSame=PlayersAfter.Num()==OriginalPlayers.Num();
            for(const auto& Item:OriginalPlayers){const auto* Hash=PlayersAfter.Find(Item.Key);PlayersSame&=Hash&&*Hash==Item.Value;}
            Check(TEXT("v87_settings_original_global_ini_bytes_and_player_files_unchanged"),After==OriginalIniHash&&PlayersSame);
            J->SetStringField(TEXT("original_ini_sha1_after"),After);J->SetObjectField(TEXT("player_sha1_after"),HashJson(PlayersAfter));
            J->SetObjectField(TEXT("restored"),R->GetDiagnostics());Finish();
        };
        Check(TEXT("v87_settings_no_fg_request_prepares_repetition_without_hdr_change"),R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false));
        Check(TEXT("v87_settings_current_quality_is_supported_and_canonical"),R->SetQuality(R->Quality,false));
        const auto CurrentQuality=R->Quality;const auto CurrentGeneration=R->FrameGeneration;const auto CurrentReflex=R->Reflex;
        const auto CurrentPreset=R->GraphicsPreset;const bool CurrentRR=R->bRayReconstruction;
        const uint64 Applied=R->ApplyCallsV87,Suspended=R->SuspendCallsV87,Flushed=R->SaveFlushCallsV87;
        const double Resume=R->FrameGenerationResumeAt;const auto StableBefore=R->GetDiagnostics();bool Accepted=true;
        for(int32 I=0;I<12;++I)
        {
            Accepted&=R->SetQuality(CurrentQuality)&&R->SetFrameGeneration(CurrentGeneration)&&R->SetReflex(CurrentReflex)
                &&R->SetRayReconstruction(CurrentRR)&&R->SetGraphicsPreset(CurrentPreset);
        }
        const auto StableAfter=R->GetDiagnostics();
        Check(TEXT("v87_settings_twelve_same_requests_per_option_do_not_apply_suspend_or_flush"),Accepted&&R->ApplyCallsV87==Applied
            &&R->SuspendCallsV87==Suspended&&R->SaveFlushCallsV87==Flushed&&R->FrameGenerationResumeAt==Resume);
        Check(TEXT("v87_settings_same_requests_keep_raw_fields_sr_percentage_and_api_modes"),R->Quality==CurrentQuality&&R->FrameGeneration==CurrentGeneration
            &&R->Reflex==CurrentReflex&&R->GraphicsPreset==CurrentPreset&&R->bRayReconstruction==CurrentRR
            &&StableBefore->GetNumberField(TEXT("screen_percentage"))==StableAfter->GetNumberField(TEXT("screen_percentage"))
            &&StableBefore->GetBoolField(TEXT("dlss_enabled_api"))==StableAfter->GetBoolField(TEXT("dlss_enabled_api"))
            &&StableBefore->GetBoolField(TEXT("rr_enabled_api"))==StableAfter->GetBoolField(TEXT("rr_enabled_api"))
            &&StableBefore->GetBoolField(TEXT("fg_stop_rhi_drain_pending"))==StableAfter->GetBoolField(TEXT("fg_stop_rhi_drain_pending")));
        J->SetObjectField(TEXT("same_before"),StableBefore);J->SetObjectField(TEXT("same_after"),StableAfter);
        const auto Caps=R->GetCapabilities();EFrontlineRenderQuality Alternative=CurrentQuality;
        for(auto Value:Caps.Qualities)if(Value!=CurrentQuality){Alternative=Value;break;}
        if(Alternative!=CurrentQuality)
        {
            const uint64 A=R->ApplyCallsV87,S=R->SuspendCallsV87;
            const bool Changed=R->SetQuality(Alternative,false);const uint64 OnceA=R->ApplyCallsV87,OnceS=R->SuspendCallsV87;
            const bool Repeated=R->SetQuality(Alternative);
            Check(TEXT("v87_settings_one_changed_quality_applies_once_then_same_value_is_idle"),Changed&&Repeated&&OnceA==A+1&&OnceS==S+1
                &&R->ApplyCallsV87==OnceA&&R->SuspendCallsV87==OnceS);
        }
        else Skip(TEXT("No second actually supported SR quality; changed-quality case unavailable."));
        EFrontlineRenderQuality NonNative=EFrontlineRenderQuality::Native;
        for(auto Value:Caps.Qualities)if(Value!=EFrontlineRenderQuality::Native){NonNative=Value;break;}
        if(NonNative!=EFrontlineRenderQuality::Native&&Caps.bRayReconstruction)
        {
            const bool Prepared=R->SetQuality(NonNative,false)&&R->SetRayReconstruction(true,false);
            const uint64 A=R->ApplyCallsV87,S=R->SuspendCallsV87;const bool Native=R->SetQuality(EFrontlineRenderQuality::Native,false);
            const auto NativeState=R->GetDiagnostics();const uint64 OnceA=R->ApplyCallsV87,OnceS=R->SuspendCallsV87;
            const bool Repeated=R->SetQuality(EFrontlineRenderQuality::Native);
            Check(TEXT("v87_settings_native_clears_raw_rr_and_repeated_native_does_not_suspend_again"),Prepared&&Native&&Repeated&&!R->bRayReconstruction
                &&OnceA==A+1&&OnceS==S+1&&R->ApplyCallsV87==OnceA&&R->SuspendCallsV87==OnceS
                &&FMath::IsNearlyEqual(float(NativeState->GetNumberField(TEXT("screen_percentage"))),100.f,.001f)
                &&!NativeState->GetBoolField(TEXT("dlss_enabled_api"))&&!NativeState->GetBoolField(TEXT("rr_enabled_api")));
        }
        else Skip(TEXT("Actual SR/RR support unavailable; Native-to-RR coupling prerequisite unavailable."));
        EFrontlineFrameGeneration Generation=EFrontlineFrameGeneration::Off;
        for(auto Value:Caps.FrameGenerationModes)if(Value!=EFrontlineFrameGeneration::Off){Generation=Value;break;}
        if(Generation!=EFrontlineFrameGeneration::Off&&Caps.ReflexModes.Contains(EFrontlineReflexMode::On))
        {
            const bool Prepared=R->SetFrameGeneration(EFrontlineFrameGeneration::Off,false)&&R->SetReflex(EFrontlineReflexMode::Off,false);
            const uint64 A=R->ApplyCallsV87;const bool Enabled=R->SetFrameGeneration(Generation,false);const uint64 Once=R->ApplyCallsV87;
            const bool Repeated=R->SetFrameGeneration(Generation);
            Check(TEXT("v87_settings_changed_fg_promotes_reflex_and_same_linked_request_is_idle"),Prepared&&Enabled&&Repeated&&R->Reflex==EFrontlineReflexMode::On
                &&Once==A+1&&R->ApplyCallsV87==Once);
        }
        else Skip(TEXT("Actual FG/Reflex support unavailable; linked enable prerequisite unavailable."));
        const uint64 ReapplyA=R->ApplyCallsV87,ReapplyS=R->SuspendCallsV87,ReapplyF=R->SaveFlushCallsV87;
        R->Reapply();Check(TEXT("v87_settings_explicit_reapply_still_performs_one_apply_without_new_suspend_or_save"),R->ApplyCallsV87==ReapplyA+1
            &&R->SuspendCallsV87==ReapplyS&&R->SaveFlushCallsV87==ReapplyF);
        const auto InvalidBefore=R->GetDiagnostics();
        Check(TEXT("v87_settings_unsupported_values_still_reject_before_any_apply"),!R->SetQuality(EFrontlineRenderQuality(255))
            &&!R->SetFrameGeneration(EFrontlineFrameGeneration(255))&&!R->SetReflex(EFrontlineReflexMode(255))&&!R->SetGraphicsPreset(EFrontlineGraphicsPreset(255))
            &&R->ApplyCallsV87==uint64(InvalidBefore->GetNumberField(TEXT("settings_apply_calls_v87"))));
        // Persistence exercises the real Save/Flush path without changing the
        // protected GameUserSettings branch. The only writable target is NEW.
        const FString Scratch=FPaths::Combine(User,TEXT("settings-save-probe-v87-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".ini"));
        Check(TEXT("v87_settings_scratch_ini_is_new_and_inside_this_exact_review_directory"),!FPaths::FileExists(Scratch)&&FPaths::GetPath(Scratch)==User);
        if(FPaths::FileExists(Scratch)||FPaths::GetPath(Scratch)!=User)return Finish();
        FConfigFile Seed;Seed.SetInt64(TEXT("Warfare.Render"),TEXT("Quality"),(int32(R->Quality)+1)%5);
        Seed.SetInt64(TEXT("Warfare.Render"),TEXT("FrameGeneration"),int32(R->FrameGeneration));Seed.SetInt64(TEXT("Warfare.Render"),TEXT("Reflex"),int32(R->Reflex));
        Seed.SetBool(TEXT("Warfare.Render"),TEXT("RayReconstruction"),R->bRayReconstruction);Seed.SetInt64(TEXT("Warfare.Render"),TEXT("GraphicsPreset"),int32(R->GraphicsPreset));
        const bool Seeded=Seed.Write(Scratch,false);Check(TEXT("v87_settings_owned_scratch_seed_written"),Seeded);if(!Seeded)return Finish();
        J->SetStringField(TEXT("scratch_ini"),Scratch);J->SetStringField(TEXT("scratch_ini_sha1_seed"),FileHash(Scratch));
        {
            const bool OriginalNoSave=R->bNoSave;ON_SCOPE_EXIT{GGameUserSettingsIni=OriginalIni;R->bNoSave=OriginalNoSave;};
            GGameUserSettingsIni=Scratch;R->bNoSave=false;GConfig->LoadFile(Scratch);
            const uint64 A=R->ApplyCallsV87,S=R->SuspendCallsV87,F=R->SaveFlushCallsV87;
            const bool Saved=R->SetQuality(R->Quality,true);FConfigFile Disk;const bool Read=Disk.Combine(Scratch);
            int32 Q=0,FG=0,RF=0;bool RR=false;
            const bool Values=Read&&Disk.GetInt(TEXT("Warfare.Render"),TEXT("Quality"),Q)&&Disk.GetInt(TEXT("Warfare.Render"),TEXT("FrameGeneration"),FG)
                &&Disk.GetInt(TEXT("Warfare.Render"),TEXT("Reflex"),RF)&&Disk.GetBool(TEXT("Warfare.Render"),TEXT("RayReconstruction"),RR)
                &&Q==int32(R->Quality)&&FG==int32(R->FrameGeneration)&&RF==int32(R->Reflex)&&RR==R->bRayReconstruction;
            Check(TEXT("v87_settings_same_applied_request_saves_unsaved_or_cli_value_once_into_real_scratch_ini"),Saved&&Values
                &&R->ApplyCallsV87==A&&R->SuspendCallsV87==S&&R->SaveFlushCallsV87==F+1);
            const FString SavedHash=FileHash(Scratch);const uint64 Once=R->SaveFlushCallsV87;
            bool Repeated=true;for(int32 I=0;I<8;++I)Repeated&=R->SetQuality(R->Quality)&&R->SetFrameGeneration(R->FrameGeneration)&&R->SetReflex(R->Reflex)&&R->SetRayReconstruction(R->bRayReconstruction);
            Check(TEXT("v87_settings_four_existing_ini_values_skip_all_repeated_save_flushes"),Repeated&&R->SaveFlushCallsV87==Once&&FileHash(Scratch)==SavedHash);
            GConfig->SetInt(TEXT("Warfare.Render"),TEXT("GraphicsPreset"),(int32(R->GraphicsPreset)+1)%3,Scratch);
            const bool PresetSaved=R->SetGraphicsPreset(R->GraphicsPreset);const uint64 PresetOnce=R->SaveFlushCallsV87;
            const bool PresetRepeated=R->SetGraphicsPreset(R->GraphicsPreset);FConfigFile PresetDisk;int32 Preset=0;
            Check(TEXT("v87_settings_same_preset_saves_dirty_key_once_without_reapplying"),PresetSaved&&PresetRepeated&&PresetOnce==Once+1&&R->SaveFlushCallsV87==PresetOnce
                &&R->ApplyCallsV87==A&&R->SuspendCallsV87==S&&PresetDisk.Combine(Scratch)&&PresetDisk.GetInt(TEXT("Warfare.Render"),TEXT("GraphicsPreset"),Preset)&&Preset==int32(R->GraphicsPreset));
            J->SetStringField(TEXT("scratch_ini_sha1_after"),FileHash(Scratch));
        }
        J->SetObjectField(TEXT("after_checks_before_restore"),R->GetDiagnostics());
        return Finish();
    }
};
TSharedRef<FJsonObject> FrontlineRenderSettingsV87::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineRenderSettingsV87Review::Run(B,Check);}
#else
TSharedRef<FJsonObject> FrontlineRenderSettingsV87::RunChecks(AWarfareFrontlineGameMode*,TFunctionRef<void(const FString&,bool)> Check)
{auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),false);Check(TEXT("v87_settings_editor_only"),false);return J;}
#endif
