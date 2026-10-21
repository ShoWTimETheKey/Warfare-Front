#include "WarfareFrontline.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AWarfareFrontlineGameMode::BeginV18AgileVerification()
{
    const double Started=FPlatformTime::Seconds();
    const auto Report=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Checks;
    int32 Passed=0;
    auto Check=[&](const FString& Name,bool Result)
    {
        const auto Item=MakeShared<FJsonObject>(); Item->SetStringField(TEXT("name"),Name); Item->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(Item)); Passed+=Result?1:0;
        UE_LOG(LogTemp,Display,TEXT("V18 agile %s: %s"),*Name,Result?TEXT("PASS"):TEXT("FAIL"));
    };
    Check(TEXT("v18_required_assets_available"),AssetFailure.IsEmpty() && LoadedSoldier && IdleAnimation && V16SoldierMeshes.Num()==12);
    Check(TEXT("v18_profile_is_volatile"),bProgressionVolatile);
    if (bProgressionVolatile && AssetFailure.IsEmpty() && LoadedSoldier && IdleAnimation) RunV18BattlefieldChecks(Check);
    Report->SetStringField(TEXT("scope"),TEXT("Targeted V18 battlefield API/collision fixtures. Explicit spawn, relocation, readiness and objective time; not natural match-duration, rendering, audio or death-physics acceptance. No prior full suite."));
    Report->SetStringField(TEXT("completed_utc"),FDateTime::UtcNow().ToIso8601());
    Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
    Report->SetNumberField(TEXT("total"),Checks.Num()); Report->SetNumberField(TEXT("passed"),Passed);
    Report->SetNumberField(TEXT("failed_count"),Checks.Num()-Passed);
    Report->SetBoolField(TEXT("success"),Passed==Checks.Num()); Report->SetBoolField(TEXT("all_passed"),Passed==Checks.Num());
    Report->SetArrayField(TEXT("checks"),Checks);
    const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V18"));
    IFileManager::Get().MakeDirectory(*Directory,true);
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    const bool Saved=FFileHelper::SaveStringToFile(Json,*FPaths::Combine(Directory,TEXT("agile-verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("WF_V18_AGILE_COMPLETE passed=%d total=%d failed=%d report_saved=%d"),Passed,Checks.Num(),Checks.Num()-Passed,int32(Saved));
    if (!Saved) UE_LOG(LogTemp,Error,TEXT("V18 agile report could not be saved"));
    FPlatformMisc::RequestExit(false);
}
