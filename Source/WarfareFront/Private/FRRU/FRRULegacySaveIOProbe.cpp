// Separate receiver-owned, editor-only save-I/O probe. It does not widen gameplay
// or alter the locked native pilot. Only public, production campaign APIs run.
#if WITH_EDITOR
#include "WarfareFrontline.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineCombatRecordV65.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace FRRULegacySaveIOV1
{
constexpr const TCHAR* Revision=TEXT("v81.frru.saveio.1");
constexpr const TCHAR* ComparisonRevision=TEXT("v81.frru.saveio.comparison.2");
FString Normal(FString Path)
{
    Path=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeDirectoryName(Path);
    FPaths::CollapseRelativeDirectories(Path);return Path;
}
bool LoadJSON(const FString& Path,TSharedPtr<FJsonObject>& Out)
{
    FString Text;return IFileManager::Get().FileSize(*Path)>=0&&IFileManager::Get().FileSize(*Path)<=1024*1024
        &&FFileHelper::LoadFileToString(Text,*Path)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out)&&Out.IsValid();
}
bool Same(const TSharedPtr<FJsonValue>& A,const TSharedPtr<FJsonValue>& B)
{return A.IsValid()&&B.IsValid()&&FJsonValue::CompareEqual(*A,*B);}
bool IsFixtureIsoUtc(const FString& Value)
{
    const int32 Length=Value.Len();if(Length!=20&&Length!=24)return false;
    if(Value[4]!=TEXT('-')||Value[7]!=TEXT('-')||Value[10]!=TEXT('T')||Value[13]!=TEXT(':')||Value[16]!=TEXT(':')||Value[Length-1]!=TEXT('Z'))return false;
    if(Length==24&&Value[19]!=TEXT('.'))return false;
    for(int32 Index=0;Index<19;++Index)
    {
        if(Index==4||Index==7||Index==10||Index==13||Index==16)continue;
        if(Value[Index]<TEXT('0')||Value[Index]>TEXT('9'))return false;
    }
    if(Length==24)for(int32 Index=20;Index<23;++Index)if(Value[Index]<TEXT('0')||Value[Index]>TEXT('9'))return false;
    return true;
}
bool SameFile(const FString& A,const FString& B)
{
    TArray<uint8> Left,Right;return FFileHelper::LoadFileToArray(Left,*A)&&FFileHelper::LoadFileToArray(Right,*B)&&Left==Right;
}
TSharedRef<FJsonObject> View(const FFrontlineCampaignProgress& R)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("id"),R.Id.ToString(EGuidFormats::Digits));
    J->SetNumberField(TEXT("faction"),int32(R.Faction));J->SetNumberField(TEXT("difficulty"),int32(R.Difficulty));
    J->SetNumberField(TEXT("completed_missions"),R.CompletedMissions);J->SetNumberField(TEXT("selected_mission"),R.SelectedMission);
    J->SetNumberField(TEXT("route_revision"),R.RouteRevision);J->SetStringField(TEXT("created_utc"),R.CreatedUtc);
    J->SetNumberField(TEXT("role_selection"),R.RoleSelectionMask);J->SetNumberField(TEXT("support_selection"),R.SupportSelectionMask);
    J->SetNumberField(TEXT("armour_research_v63"),R.ArmourResearchMaskV63);J->SetNumberField(TEXT("armour_selection_v63"),R.ArmourSelectionMaskV63);
    J->SetNumberField(TEXT("pioneer_orders_v63"),R.PioneerOrdersMaskV63);J->SetNumberField(TEXT("smoke_orders_v63"),R.SmokeOrdersMaskV63);
    J->SetNumberField(TEXT("armour_orders_v63"),R.ArmourOrdersMaskV63);J->SetNumberField(TEXT("acknowledged_events"),R.AcknowledgedEventMask);
    J->SetBoolField(TEXT("mobile_gun_doctrine"),R.bMobileGunV52);J->SetBoolField(TEXT("achievement_eligible_v76"),R.bAchievementEligibleV76);
    J->SetNumberField(TEXT("strategy_revision"),R.StrategicRevisionV52);J->SetNumberField(TEXT("won_operations"),R.WonOperationMaskV52);
    J->SetNumberField(TEXT("bypassed_operations"),R.BypassedOperationMaskV52);J->SetNumberField(TEXT("dispatch_resolved"),R.DispatchResolvedMaskV52);
    J->SetNumberField(TEXT("dispatch_orders"),R.DispatchOrderMaskV52);J->SetNumberField(TEXT("dispatch_choices"),R.DispatchChoiceMaskV52);
    auto Paid=MakeShared<FJsonObject>();Paid->SetNumberField(TEXT("research_points"),R.Research.ResearchPoints);
    Paid->SetNumberField(TEXT("technology_mask"),R.Research.TechnologyMask);Paid->SetNumberField(TEXT("victories"),R.Research.Victories);
    Paid->SetNumberField(TEXT("defeats"),R.Research.Defeats);J->SetObjectField(TEXT("research"),Paid);
    FrontlineCombatRecordV65::WriteArchive(*J,R.CombatRecordV65);return J;
}
struct FProbe
{
    FString User,Case,Profile,Input,Receipt;TSharedPtr<FJsonObject> Expected;double Started=FPlatformTime::Seconds();
    TArray<TSharedPtr<FJsonValue>> Checks;bool Good=true,Finished=false,HostObserved=false,Historical=false;
    int32 ExpectedCount() const{return Historical?6:2;}
    void Check(const FString& Name,bool Pass)
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;
        UE_LOG(LogTemp,Display,TEXT("FRRU SAVEIO %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));
    }
    void CheckCreatedUtc(const FString& Name,const TSharedPtr<FJsonValue>& Actual,const TSharedPtr<FJsonValue>& Wanted,bool ExpectedEqual=true)
    {
        FString ActualText,WantedText;FDateTime ActualTime,WantedTime;
        const bool ActualForm=Actual.IsValid()&&Actual->Type==EJson::String&&Actual->TryGetString(ActualText)&&IsFixtureIsoUtc(ActualText);
        const bool WantedForm=Wanted.IsValid()&&Wanted->Type==EJson::String&&Wanted->TryGetString(WantedText)&&IsFixtureIsoUtc(WantedText);
        const bool ActualParsed=ActualForm&&FDateTime::ParseIso8601(*ActualText,ActualTime);
        const bool WantedParsed=WantedForm&&FDateTime::ParseIso8601(*WantedText,WantedTime);
        const bool Equal=ActualParsed&&WantedParsed&&ActualTime.GetTicks()==WantedTime.GetTicks();
        Check(Name,Equal==ExpectedEqual);const auto Result=Checks.Last()->AsObject();
        if(Actual.IsValid())Result->SetField(TEXT("actual"),Actual);else Result->SetField(TEXT("actual"),MakeShared<FJsonValueNull>());
        if(Wanted.IsValid())Result->SetField(TEXT("expected"),Wanted);else Result->SetField(TEXT("expected"),MakeShared<FJsonValueNull>());
        Result->SetBoolField(TEXT("actual_fixture_iso8601_form_valid"),ActualForm);Result->SetBoolField(TEXT("expected_fixture_iso8601_form_valid"),WantedForm);
        Result->SetBoolField(TEXT("actual_parsed_iso8601"),ActualParsed);Result->SetBoolField(TEXT("expected_parsed_iso8601"),WantedParsed);
        Result->SetBoolField(TEXT("equivalent_instant"),Equal);Result->SetBoolField(TEXT("expected_equivalent_instant"),ExpectedEqual);
        Result->SetStringField(TEXT("comparison"),TEXT("Both strings must have the fixture's UTC ISO 8601 form with whole seconds or exactly three millisecond digits, parse successfully and have exactly equal FDateTime ticks"));
    }
    void CheckCreatedUtcComparator()
    {
        const auto Original=MakeShared<FJsonValueString>(TEXT("2025-01-01T12:00:00.000Z"));
        CheckCreatedUtc(TEXT("created_utc_accepts_equivalent_whole_seconds"),MakeShared<FJsonValueString>(TEXT("2025-01-01T12:00:00Z")),Original);
        CheckCreatedUtc(TEXT("created_utc_rejects_changed_calendar_date"),MakeShared<FJsonValueString>(TEXT("2025-01-02T12:00:00Z")),Original,false);
        CheckCreatedUtc(TEXT("created_utc_rejects_changed_millisecond"),MakeShared<FJsonValueString>(TEXT("2025-01-01T12:00:00.001Z")),Original,false);
        CheckCreatedUtc(TEXT("created_utc_rejects_unparseable_actual"),MakeShared<FJsonValueString>(TEXT("invalid-date")),Original,false);
        CheckCreatedUtc(TEXT("created_utc_rejects_unparseable_expectation"),Original,MakeShared<FJsonValueString>(TEXT("invalid-date")),false);
        CheckCreatedUtc(TEXT("created_utc_rejects_non_string_value"),MakeShared<FJsonValueNumber>(0.0),Original,false);
        CheckCreatedUtc(TEXT("created_utc_rejects_wrong_separators"),MakeShared<FJsonValueString>(TEXT("2025x01x01T12x00x00Z")),Original,false);
        CheckCreatedUtc(TEXT("created_utc_rejects_trailing_garbage"),MakeShared<FJsonValueString>(TEXT("2025-01-01T12:00:00Zjunk")),Original,false);
        CheckCreatedUtc(TEXT("created_utc_rejects_submillisecond_rounding"),MakeShared<FJsonValueString>(TEXT("2025-01-01T12:00:00.0001Z")),Original,false);
    }
    void Finish()
    {
        if(Finished)return;Finished=true;auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("revision"),Revision);
        J->SetStringField(TEXT("comparison_revision"),ComparisonRevision);
        J->SetStringField(TEXT("case"),Case);J->SetBoolField(TEXT("passed"),Good);J->SetBoolField(TEXT("synthetic_fixture"),!Historical);
        J->SetBoolField(TEXT("actual_host_campaign_io"),HostObserved);J->SetBoolField(TEXT("gameplay_enabled_by_probe"),false);
        J->SetBoolField(TEXT("historical_player_archive_verified"),Historical&&Good);J->SetBoolField(TEXT("combat_after_old_save_verified"),false);
        J->SetStringField(TEXT("scope"),Historical?TEXT("Byte-pinned archived schema-5 UK/DE copy through the actual BeginPlay loader and public ContinueCampaign writer; independent retained-field expectations; no actor combat or Shipping acceptance"):TEXT("Synthetic schema-4 compatibility fixture through actual existing UK/DE BeginPlay loader and public ContinueCampaign atomic writer; no actor combat, player-profile copy, historical archive provenance or Shipping acceptance"));
        J->SetStringField(TEXT("profile"),Profile);J->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
        J->SetNumberField(TEXT("checks"),Checks.Num());J->SetArrayField(TEXT("results"),Checks);
        FString Text;const bool Encoded=FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Receipt),true);
        const bool Written=Encoded&&FFileHelper::SaveStringToFile(Text,*Receipt,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        FPlatformMisc::RequestExitWithStatus(false,Good&&Written?0:3);
    }
    void Runs(AWarfareFrontlineGameMode* B,const FString& Prefix)
    {
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        const bool Valid=Expected->TryGetArrayField(TEXT("campaigns"),Rows)&&Rows&&Rows->Num()==ExpectedCount();
        Check(Prefix+TEXT("_fixture_campaign_count_valid"),Valid);if(!Valid)return;
        Check(Prefix+TEXT("_expected_campaign_count_loaded"),B->GetCampaignRuns().Num()==ExpectedCount());
        for(const auto& V:*Rows)
        {
            const TSharedPtr<FJsonObject>* O=nullptr;FString ID;FGuid GUID;
            const bool Row=V.IsValid()&&V->TryGetObject(O)&&O&&O->IsValid()&&(*O)->TryGetStringField(TEXT("id"),ID)&&FGuid::ParseExact(ID,EGuidFormats::Digits,GUID);
            Check(Prefix+TEXT("_expected_stable_guid"),Row);if(!Row)continue;
            const auto* R=B->GetCampaignRuns().FindByPredicate([&GUID](const FFrontlineCampaignProgress& Item){return Item.Id==GUID;});
            Check(Prefix+TEXT("_campaign_guid_retained_")+ID,R!=nullptr);if(!R)continue;
            auto Actual=View(*R);
            for(const auto& Pair:(*O)->Values)
            {
                const FString Name=Prefix+TEXT("_")+ID+TEXT("_")+Pair.Key;
                if(Pair.Key==TEXT("created_utc"))CheckCreatedUtc(Name,Actual->Values.FindRef(Pair.Key),Pair.Value);
                else Check(Name,Same(Actual->Values.FindRef(Pair.Key),Pair.Value));
            }
            FDateTime Created,Updated;Check(Prefix+TEXT("_valid_monotone_timestamp_")+ID,FDateTime::ParseIso8601(*R->CreatedUtc,Created)&&FDateTime::ParseIso8601(*R->UpdatedUtc,Updated)&&Updated>=Created);
        }
    }
    void Disk(AWarfareFrontlineGameMode* B,const FString& Prefix)
    {
        TSharedPtr<FJsonObject> Root;const bool Loaded=LoadJSON(Profile,Root);Check(Prefix+TEXT("_actual_file_readable"),Loaded);if(!Loaded)return;
        double Schema=0;Check(Prefix+TEXT("_schema_six_written"),Root->TryGetNumberField(TEXT("schema"),Schema)&&Schema==6);
        const TSharedPtr<FJsonObject>* Top=nullptr;const bool TopOK=Expected->TryGetObjectField(TEXT("legacy_top"),Top)&&Top&&Top->IsValid();
        Check(Prefix+TEXT("_legacy_top_fixture_valid"),TopOK);if(TopOK)for(const auto& P:(*Top)->Values)Check(Prefix+TEXT("_retained_")+P.Key,Same(Root->Values.FindRef(P.Key),P.Value));
        FString Active;double Faction=-1;const auto* Run=B->GetCurrentCampaign();
        Check(Prefix+TEXT("_active_guid_and_faction_written"),Run&&Root->TryGetStringField(TEXT("active_campaign"),Active)&&Active==Run->Id.ToString(EGuidFormats::Digits)&&Root->TryGetNumberField(TEXT("player_faction"),Faction)&&Faction==int32(Run->Faction));
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;const bool RowOK=Root->TryGetArrayField(TEXT("campaigns"),Rows)&&Rows&&Rows->Num()==ExpectedCount();
        Check(Prefix+TEXT("_owned_campaign_count_written"),RowOK);if(RowOK)for(const auto& R:B->GetCampaignRuns())
        {
            const TSharedPtr<FJsonObject>* Found=nullptr;
            for(const auto& V:*Rows){const TSharedPtr<FJsonObject>* Candidate=nullptr;FString ID;if(V.IsValid()&&V->TryGetObject(Candidate)&&Candidate&&Candidate->IsValid()&&(*Candidate)->TryGetStringField(TEXT("id"),ID)&&ID==R.Id.ToString(EGuidFormats::Digits)){Found=Candidate;break;}}
            Check(Prefix+TEXT("_disk_campaign_owned_")+R.Id.ToString(EGuidFormats::Digits),Found!=nullptr);
            if(Found){const auto Exported=View(R);for(const auto& P:Exported->Values)Check(Prefix+TEXT("_disk_")+R.Id.ToString(EGuidFormats::Digits)+TEXT("_")+P.Key,Same((*Found)->Values.FindRef(P.Key),P.Value));
                CheckCreatedUtc(Prefix+TEXT("_disk_updated_utc_")+R.Id.ToString(EGuidFormats::Digits),(*Found)->Values.FindRef(TEXT("updated_utc")),MakeShared<FJsonValueString>(R.UpdatedUtc));}
        }
        Check(Prefix+TEXT("_no_pending_after_success"),!IFileManager::Get().FileExists(*(Profile+TEXT(".pending"))));
    }
    void Run(AWarfareFrontlineGameMode* B)
    {
        HostObserved=B&&B->HasActorBegunPlay()&&B->GetWorld()&&B->GetWorld()->WorldType==EWorldType::Game&&!B->IsProgressionVolatile();
        Check(TEXT("real_game_world_and_beginplay"),HostObserved);
        CheckCreatedUtcComparator();
        Check(TEXT("host_io_not_volatile"),B&&!B->IsProgressionVolatile());if(!B||B->IsProgressionVolatile()){Finish();return;}
        B->SetActorTickEnabled(false);Check(TEXT("preparation_without_combat"),B->Phase==EFrontlinePhase::Briefing&&B->Units.IsEmpty()&&B->Squads.IsEmpty());
        if(B->Phase!=EFrontlinePhase::Briefing||!B->Units.IsEmpty()||!B->Squads.IsEmpty()){Finish();return;}
        if(Case==TEXT("malformed")||Case==TEXT("unsupported"))
        {
            Check(TEXT("invalid_file_retained_on_startup"),SameFile(Profile,Input));Check(TEXT("invalid_archive_is_readonly"),B->IsProgressionReadOnly());
            Check(TEXT("no_partial_campaign_publication"),B->GetCampaignRuns().IsEmpty()&&!B->GetCurrentCampaign());
            const FGuid British(0x11111111,0x22223333,0x44445555,0x66667777),German(0x88888888,0x9999aaaa,0xbbbbcccc,0xddddeeee);
            Check(TEXT("readonly_new_british_refused"),!B->StartNewCampaign(EFrontlineFaction::British));Check(TEXT("readonly_new_german_refused"),!B->StartNewCampaign(EFrontlineFaction::German));
            Check(TEXT("readonly_continue_british_refused"),!B->ContinueCampaign(British));Check(TEXT("readonly_continue_german_refused"),!B->ContinueCampaign(German));
            Check(TEXT("readonly_paid_training_refused"),!B->PurchaseTechnology(0));Check(TEXT("readonly_archive_delete_refused"),!B->DeleteCampaignArchives({British,German}));
            Check(TEXT("invalid_original_bytes_after_mutation_attempts"),SameFile(Profile,Input));
            for(const TCHAR* Suffix:{TEXT(".pending"),TEXT(".backup"),TEXT(".pre-v23"),TEXT(".pre-v41"),TEXT(".pre-v52")})Check(FString(TEXT("invalid_no_write"))+Suffix,!IFileManager::Get().FileExists(*(Profile+Suffix)));
            Finish();return;
        }
        Check(TEXT("valid_archive_writable"),!B->IsProgressionReadOnly());Runs(B,TEXT("loaded"));
        if(B->IsProgressionReadOnly()||B->GetCampaignRuns().Num()!=ExpectedCount()){Finish();return;}
        FString Active;const auto* Initial=B->GetCurrentCampaign();
        Check(TEXT("initial_active_owned"),Expected->TryGetStringField(TEXT("active_campaign"),Active)&&Initial&&Initial->Id.ToString(EGuidFormats::Digits)==Active&&Initial->Faction==B->GetPlayerFaction());
        if(Case==TEXT("migrate"))
        {
            Check(TEXT("schema4_pre_v41_backup_exact"),SameFile(Profile+TEXT(".pre-v41"),Input));Check(TEXT("schema4_pre_v52_backup_exact"),SameFile(Profile+TEXT(".pre-v52"),Input));
            Check(TEXT("schema4_no_pre_v23_backup"),!IFileManager::Get().FileExists(*(Profile+TEXT(".pre-v23"))));
            Check(TEXT("migration_did_write_schema6"),!SameFile(Profile,Input));
        }
        else if(Case==TEXT("historical-migrate"))
        {
            Check(TEXT("schema5_pre_v52_backup_exact"),SameFile(Profile+TEXT(".pre-v52"),Input));
            Check(TEXT("schema5_no_pre_v41_backup"),!IFileManager::Get().FileExists(*(Profile+TEXT(".pre-v41"))));
            Check(TEXT("schema5_no_pre_v23_backup"),!IFileManager::Get().FileExists(*(Profile+TEXT(".pre-v23"))));
            Check(TEXT("schema5_initial_atomic_backup_exact"),SameFile(Profile+TEXT(".backup"),Input));
            Check(TEXT("historical_migration_wrote_schema6"),!SameFile(Profile,Input));
            const TSharedPtr<FJsonObject>* Dates=nullptr;const TSharedPtr<FJsonObject>* Rules=nullptr;
            const bool DatesOK=Expected->TryGetObjectField(TEXT("timestamp_rules"),Rules)&&Rules&&(*Rules)->TryGetObjectField(TEXT("original_campaign_updated_utc"),Dates)&&Dates;
            Check(TEXT("historical_loaded_timestamp_expectations_present"),DatesOK);
            if(DatesOK)for(const auto& R:B->GetCampaignRuns())
            {const FString ID=R.Id.ToString(EGuidFormats::Digits);
                CheckCreatedUtc(TEXT("historical_loaded_updated_utc_")+ID,MakeShared<FJsonValueString>(R.UpdatedUtc),(*Dates)->TryGetField(ID));}
        }
        else for(const TCHAR* Suffix:{TEXT(".pre-v23"),TEXT(".pre-v41"),TEXT(".pre-v52")})Check(FString(TEXT("reload_not_remigrated"))+Suffix,!IFileManager::Get().FileExists(*(Profile+Suffix)));
        Disk(B,TEXT("before_continue"));
        const auto RunsCopy=B->GetCampaignRuns();for(int32 Nation=0;Nation<2;++Nation)
        {
            FGuid ExplicitID;const TArray<TSharedPtr<FJsonValue>>* Targets=nullptr;
            if(Historical&&Expected->TryGetArrayField(TEXT("continue_campaign_ids"),Targets)&&Targets&&Targets->Num()==2)
            {FString Value;if((*Targets)[Nation]->TryGetString(Value))FGuid::ParseExact(Value,EGuidFormats::Digits,ExplicitID);}
            const auto* Target=RunsCopy.FindByPredicate([Nation,ExplicitID,this](const FFrontlineCampaignProgress& R){return int32(R.Faction)==Nation&&(!Historical||R.Id==ExplicitID);});
            Check(TEXT("both_national_archives_present"),Target!=nullptr);if(!Target)continue;
            const auto BeforeRuns=B->GetCampaignRuns();
            TArray<uint8> Before,Backup;const bool Had=FFileHelper::LoadFileToArray(Before,*Profile);
            Check(FString::Printf(TEXT("continue_%d_real_public_writer"),Nation),B->ContinueCampaign(Target->Id));
            const auto* Current=B->GetCurrentCampaign();Check(TEXT("continued_guid_nation_research_owned"),Current&&Current->Id==Target->Id&&B->GetPlayerFaction()==Target->Faction&&B->GetResearchPoints()==Target->Research.ResearchPoints&&B->GetActiveResearch().TechnologyMask==Target->Research.TechnologyMask);
            Check(TEXT("atomic_backup_is_previous_profile"),Had&&FFileHelper::LoadFileToArray(Backup,*(Profile+TEXT(".backup")))&&Before==Backup);
            for(const auto& R:BeforeRuns)if(R.Id!=Target->Id)
            {const auto* Unselected=B->GetCampaignRuns().FindByPredicate([&R](const FFrontlineCampaignProgress& Item){return Item.Id==R.Id;});Check(TEXT("unselected_campaign_timestamp_retained_")+R.Id.ToString(EGuidFormats::Digits),Unselected&&Unselected->UpdatedUtc==R.UpdatedUtc);}
            Runs(B,FString::Printf(TEXT("continued_%d"),Nation));Disk(B,FString::Printf(TEXT("written_%d"),Nation));
        }
        Check(TEXT("no_battle_started_by_probe"),B->Phase==EFrontlinePhase::Briefing&&B->Units.IsEmpty()&&B->Squads.IsEmpty());Finish();
    }
};
bool Guard(FString& User,FString& Case)
{
    const FString Isolated=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"));
    FString Supplied;const FString Project=Normal(FPaths::ProjectDir());
    if(!Project.Equals(Normal(Isolated),ESearchCase::IgnoreCase)||!GUsingNullRHI
        ||!FParse::Param(FCommandLine::Get(),TEXT("game"))||!FParse::Param(FCommandLine::Get(),TEXT("unattended"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("nosound"))||!FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRULegacySaveIO"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Supplied)
        ||Supplied.IsEmpty()||FPaths::IsRelative(Supplied)||!FParse::Value(FCommandLine::Get(),TEXT("WFFRRUSaveCase="),Case))return false;
    if(Case!=TEXT("migrate")&&Case!=TEXT("reload")&&Case!=TEXT("malformed")&&Case!=TEXT("unsupported")
        &&Case!=TEXT("historical-migrate")&&Case!=TEXT("historical-reload"))return false;
    TArray<FString> Tokens,Switches;FCommandLine::Parse(FCommandLine::Get(),Tokens,Switches);
    for(const auto& S:Switches){FString Name,Value;if(!S.Split(TEXT("="),&Name,&Value))Name=S;if(Name.StartsWith(TEXT("WF"),ESearchCase::IgnoreCase)&&!Name.Equals(TEXT("WFV13ManualReview"),ESearchCase::IgnoreCase)&&!Name.Equals(TEXT("WFFRRULegacySaveIO"),ESearchCase::IgnoreCase)&&!Name.Equals(TEXT("WFFRRUSaveCase"),ESearchCase::IgnoreCase))return false;}
    User=Normal(Supplied);const FString Prefix=Normal(FPaths::Combine(Isolated,TEXT("Saved/FRRU/LegacySaveIO")))+TEXT("/");
    FString Relative=User;Relative.RemoveFromStart(Prefix,ESearchCase::IgnoreCase);TArray<FString> Parts;Relative.ParseIntoArray(Parts,TEXT("/"));FGuid Run;
    return User.StartsWith(Prefix,ESearchCase::IgnoreCase)&&Parts.Num()==4&&FGuid::ParseExact(Parts[0],EGuidFormats::Digits,Run)&&Run.IsValid()
        &&Parts[1]==TEXT("cases")&&Parts[2]==Case&&Parts[3]==TEXT("User")
        &&Normal(FPaths::ProjectUserDir()).Equals(User,ESearchCase::IgnoreCase)
        &&Normal(FPaths::ProjectSavedDir()).Equals(User+TEXT("/Saved"),ESearchCase::IgnoreCase);
}
FAutoConsoleCommand Command(TEXT("WF.FRRU.CheckLegacySaveIO"),TEXT("Run an explicit isolated old-schema UK/DE campaign I/O probe."),FConsoleCommandDelegate::CreateLambda([]
{
    FString User,Case;if(!Guard(User,Case)){UE_LOG(LogTemp,Error,TEXT("FRRU save-I/O probe refused: project, path, rendering or command-line isolation is invalid."));FPlatformMisc::RequestExitWithStatus(false,4);return;}
    auto Probe=MakeShared<FProbe>();Probe->User=User;Probe->Case=Case;Probe->Profile=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Gameplay/V13/feature-profile.json"));
    Probe->Historical=Case==TEXT("historical-migrate")||Case==TEXT("historical-reload");
    Probe->Input=FPaths::Combine(User,TEXT("input.json"));Probe->Receipt=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("FRRU/LegacySaveIO/receipt.json"));
    FString FixtureRevision,ExpectedCase;bool Synthetic=false;
    if(!LoadJSON(FPaths::Combine(User,TEXT("expectation.json")),Probe->Expected)||!Probe->Expected->TryGetStringField(TEXT("revision"),FixtureRevision)||FixtureRevision!=FRRULegacySaveIOV1::Revision||!Probe->Expected->TryGetStringField(TEXT("case"),ExpectedCase)||ExpectedCase!=Case||!Probe->Expected->TryGetBoolField(TEXT("synthetic_fixture"),Synthetic)||Synthetic==Probe->Historical||IFileManager::Get().FileSize(*Probe->Input)<0||IFileManager::Get().FileSize(*Probe->Input)>1024*1024)
    {Probe->Check(TEXT("bounded_fixture_manifest"),false);Probe->Finish();return;}
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float)
    {
        if(FPlatformTime::Seconds()-Probe->Started>45){Probe->Check(TEXT("real_game_mode_ready_within_45_seconds"),false);Probe->Finish();return false;}
        if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World();World&&World->WorldType==EWorldType::Game)
            if(auto* Battle=World->GetAuthGameMode<AWarfareFrontlineGameMode>();Battle&&Battle->HasActorBegunPlay()){Probe->Run(Battle);return false;}
        return true;
    }));
}));
} // namespace FRRULegacySaveIOV1
#endif // WITH_EDITOR
