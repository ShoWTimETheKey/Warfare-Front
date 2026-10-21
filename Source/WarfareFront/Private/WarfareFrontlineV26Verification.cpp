#include "WarfareFrontline.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineCombatV26.h"
#include "WarfareFrontlineSupportV26.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
void StartFrontlineV26Verification(AWarfareFrontlineController* Controller)
{
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV26Review")))return;
    FTimerHandle Handle;TWeakObjectPtr<AWarfareFrontlineController> Weak=Controller;
    Controller->GetWorldTimerManager().SetTimer(Handle,[Weak]()
    {
        if(!Weak.IsValid())return;
        auto* B=Cast<AWarfareFrontlineGameMode>(UGameplayStatics::GetGameMode(Weak.Get()));
        auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;
        auto Add=[&](const FString& Name,bool Passed)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Passed);
            Checks.Add(MakeShared<FJsonValueObject>(Row));
        };
        FString UserDir;const bool Isolated=B
            && FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) && !UserDir.IsEmpty();
        Add(TEXT("explicit_v26_entry_and_unique_user_directory"),Isolated);
        if(Isolated)
        {
            B->SetActorTickEnabled(false);
            for(const auto& Part:{RunFrontlineV26CombatChecks(B),RunFrontlineV26SupportChecks(B)})
            {
                const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
                if(Part->TryGetArrayField(TEXT("checks"),Rows))Checks.Append(*Rows);
                else Add(TEXT("component_report_present"),false);
            }
            auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();
            Add(TEXT("audio_subsystem_present"),Audio!=nullptr);
            if(Audio)Audio->RunV26VoiceChecks(Add);
        }
        int32 Passed=0;for(const auto& Row:Checks)Passed+=Row->AsObject()->GetBoolField(TEXT("passed"))?1:0;
        Report->SetArrayField(TEXT("checks"),Checks);Report->SetNumberField(TEXT("passed"),Passed);
        Report->SetNumberField(TEXT("total"),Checks.Num());Report->SetBoolField(TEXT("success"),Passed==Checks.Num());
        Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
        Report->SetBoolField(TEXT("player_save_touched"),false);
        Report->SetStringField(TEXT("scope"),TEXT("Short isolated production-API fixtures for binary casualties, support scheduling and loaded voice banks. Does not certify a natural battle, subjective acting quality or frame rate."));
        const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V26"));
        IFileManager::Get().MakeDirectory(*Dir,true);
        FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*FPaths::Combine(Dir,TEXT("gameplay-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("WF_V26_REVIEW_FINISHED %d/%d"),Passed,Checks.Num());
        FPlatformMisc::RequestExit(false);
    },3.f,false);
}
