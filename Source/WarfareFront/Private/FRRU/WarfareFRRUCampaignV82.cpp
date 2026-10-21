#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFRRUTextV82.h"
#include "WarfareFRRUCampaignInternalV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineArmourV63.h"
#include "Native/UE/ExpansionHostBridge.h"
#include "Native/UE/ExpansionTacticalArchiveUE.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#if WITH_EDITOR
#include "FRRUParityReviewSafetyV87.h"
#endif
#include "Serialization/JsonSerializer.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#include <memory>
#include <algorithm>
#include <set>

struct FFrontlineNationalArchiveAccessV87
{
    static void ReadFlags(const AWarfareFrontlineGameMode* B,bool& Volatile,bool& ReadOnly)
    {Volatile=B->bProgressionVolatile;ReadOnly=B->bProgressionReadOnly;}
    static void SetFlags(AWarfareFrontlineGameMode* B,bool Volatile,bool ReadOnly)
    {B->bProgressionVolatile=Volatile;B->bProgressionReadOnly=ReadOnly;}
    static void Reset(AWarfareFrontlineGameMode* B){B->ResetEncounter();}
};

namespace WarfareFRRUCampaignV82
{
bool PersistenceReviewRequestedV82(bool PublicOnly)
{
    TArray<FString> Tokens,Switches;FCommandLine::Parse(FCommandLine::Get(),Tokens,Switches);
    for(FString Argument:Tokens)
    {
        if(Argument.StartsWith(TEXT("-"))||Argument.StartsWith(TEXT("/")))Argument.RightChopInline(1);
        FString Name,Value;if(!Argument.Split(TEXT("="),&Name,&Value))Name=Argument;
        if(Name.Equals(TEXT("WFV81FRRUPersistence"),ESearchCase::IgnoreCase)
            ||Name.Equals(TEXT("WFV81FRRUContinue"),ESearchCase::IgnoreCase)
            ||Name.Equals(TEXT("WFV81FRRUEarnedSource"),ESearchCase::IgnoreCase)
            ||(!PublicOnly&&Name.Equals(TEXT("WFV13ManualReview"),ESearchCase::IgnoreCase)))return true;
    }
    return false;
}
bool TryPersistenceReviewUserV82(FString& User)
{
    const TCHAR* CommandLine=FCommandLine::Get();FString Supplied,Nation;
    const bool Public=PersistenceReviewRequestedV82(true);
    FString EarnedSource;const bool Earned=FParse::Value(CommandLine,TEXT("WFV81FRRUEarnedSource="),EarnedSource);
    if(Earned)
    {
#if WITH_EDITOR
        return false; // This bounded review must exercise the real Shipping target.
#else
        int32 Width=0,Height=0;
        if(!Public||EarnedSource!=TEXT("62ca7c605b7c4357b98fe4033751d265")
            ||FParse::Param(CommandLine,TEXT("WFV81FRRUContinue"))
            ||!FParse::Value(CommandLine,TEXT("ResX="),Width)||Width!=3840
            ||!FParse::Value(CommandLine,TEXT("ResY="),Height)||Height!=2160
            ||!CampaignsAvailable(EFRRUNationV81::French)||CampaignsAvailable(EFRRUNationV81::Russian))return false;
#endif
    }
    if(!FParse::Value(CommandLine,TEXT("UserDir="),Supplied)||Supplied.IsEmpty()
        ||FPaths::IsRelative(Supplied)||Supplied.Contains(TEXT("..")))return false;
    if(Public)
    {
        if(!FParse::Value(CommandLine,TEXT("WFV81FRRUPersistence="),Nation)||!FApp::IsGame()
            ||(Nation!=TEXT("fr")&&Nation!=TEXT("ru"))||FParse::Param(CommandLine,TEXT("unattended"))
            ||FParse::Param(CommandLine,TEXT("RenderOffScreen"))||!FParse::Param(CommandLine,TEXT("WFRenderNoSave")))return false;
        if(Earned&&Nation!=TEXT("fr"))return false;
    }
    else
    {
#if WITH_EDITOR
        if(!FParse::Param(CommandLine,TEXT("WFV13ManualReview"))
            ||!FParse::Param(CommandLine,TEXT("game"))||!FParse::Param(CommandLine,TEXT("unattended")))return false;
#else
        return false;
#endif
    }
    TArray<FString> Tokens,Switches;FCommandLine::Parse(CommandLine,Tokens,Switches);
    int32 UserDirCount=0;TSet<FString> SeenWF;
    // UE 5.8 Tokens already contains the '-' options copied to Switches.
    // Count Tokens once; also normalize slash options accepted by FParse.
    for(FString Argument:Tokens)
    {
        if(Argument.StartsWith(TEXT("-"))||Argument.StartsWith(TEXT("/")))Argument.RightChopInline(1);
        FString Name,Value;const bool HasValue=Argument.Split(TEXT("="),&Name,&Value);if(!HasValue)Name=Argument;
        if(Name.Equals(TEXT("UserDir"),ESearchCase::IgnoreCase))++UserDirCount;
        if(Name.StartsWith(TEXT("WF"),ESearchCase::IgnoreCase))
        {
            const FString Key=Name.ToLower();if(SeenWF.Contains(Key))return false;SeenWF.Add(Key);
            if(Public)
            {
                if(Name.Equals(TEXT("WFV81FRRUPersistence"),ESearchCase::IgnoreCase))
                {if(!HasValue||Value!=Nation)return false;}
                else if(Name.Equals(TEXT("WFV81FRRUEarnedSource"),ESearchCase::IgnoreCase))
                {if(!Earned||!HasValue||Value!=EarnedSource||Nation!=TEXT("fr"))return false;}
                else if(Name.Equals(TEXT("WFRenderNoSave"),ESearchCase::IgnoreCase)||Name.Equals(TEXT("WFV81FRRUContinue"),ESearchCase::IgnoreCase))
                {if(HasValue)return false;}
                else return false;
            }
            else if(!Name.Equals(TEXT("WFV13ManualReview"),ESearchCase::IgnoreCase)||HasValue)return false;
        }
    }
    if(UserDirCount!=1)return false;
    auto Normal=[](const FString& Path){FString Result=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeDirectoryName(Result);return Result;};
    const FString Isolated=Normal(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")));
    const FString Project=Normal(FPaths::ProjectDir());bool ValidProject=false;
#if WITH_EDITOR
    ValidProject=Project.Equals(Isolated,ESearchCase::IgnoreCase)&&FParse::Param(CommandLine,TEXT("game"));
#endif
    if(Public&&!ValidProject)
    {
        const FString PackagePrefix=FPaths::Combine(Isolated,TEXT("Builds/Windows"))+TEXT("/");
        if(!Project.StartsWith(PackagePrefix,ESearchCase::IgnoreCase))return false;
        TArray<FString> PackageParts;Project.Mid(PackagePrefix.Len()).ParseIntoArray(PackageParts,TEXT("/"));
        const bool Flat=PackageParts.Num()==2&&PackageParts[1]==TEXT("WarfareFront");
        const bool Nested=PackageParts.Num()==3&&PackageParts[1]==TEXT("Windows")&&PackageParts[2]==TEXT("WarfareFront");
        if((!Flat&&!Nested)||PackageParts[0].IsEmpty()||PackageParts[0].Len()>96)return false;
        for(TCHAR C:PackageParts[0])if(!FChar::IsAlnum(C)&&C!=TEXT('-')&&C!=TEXT('_'))return false;
        const FString Shipping=FPaths::Combine(Project,TEXT("Binaries/Win64/WarfareFront-Win64-Shipping.exe"));
        ValidProject=Normal(FPlatformProcess::ExecutablePath()).Equals(Shipping,ESearchCase::IgnoreCase);
    }
    if(!ValidProject)return false;
    const FString Candidate=Normal(Supplied),Prefix=FPaths::Combine(Isolated,TEXT("Saved/FRRU/Persistence"))+TEXT("/");
    if(!Candidate.StartsWith(Prefix,ESearchCase::IgnoreCase))return false;
    const FString Relative=Candidate.Mid(Prefix.Len());TArray<FString> Parts;Relative.ParseIntoArray(Parts,TEXT("/"));FGuid Run;
    if(Parts.Num()!=2||!FGuid::ParseExact(Parts[0],EGuidFormats::Digits,Run)||!Run.IsValid()||Parts[1]!=TEXT("User")
        ||(Earned&&Parts[0].Equals(EarnedSource,ESearchCase::IgnoreCase))
        ||!Normal(FPaths::ProjectUserDir()).Equals(Candidate,ESearchCase::IgnoreCase)
        ||!Normal(FPaths::ProjectSavedDir()).Equals(Candidate+TEXT("/Saved"),ESearchCase::IgnoreCase))return false;
#if PLATFORM_WINDOWS
    if(Public)
        for(const FString& Path:TArray<FString>{Project,Candidate,Candidate+TEXT("/Saved/FRRU/Profiles"),Candidate+TEXT("/Saved/FRRU/Account")})
        {
            FString Part=Path;
            while(Part.Len()>=Isolated.Len())
            {
                const DWORD Attributes=GetFileAttributesW(*Part);
                if(Attributes!=INVALID_FILE_ATTRIBUTES&&(Attributes&FILE_ATTRIBUTE_REPARSE_POINT)!=0)return false;
                if(Attributes==INVALID_FILE_ATTRIBUTES&&GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)return false;
                if(Part.Equals(Isolated,ESearchCase::IgnoreCase))break;
                Part=Normal(FPaths::GetPath(Part));
            }
        }
#endif
    User=Candidate;return true;
}
namespace
{
constexpr int32 HostSchema=2;
constexpr int64 MaximumRepositoryBytes=8*1024*1024;
struct FRequisitionV87 {int32 Kind=INDEX_NONE;bool Paid=false;};
struct FEntry
{
    FGuid Id;
    EFrontlineDifficulty Level=EFrontlineDifficulty::Rookie;
    FString CreatedUtc,UpdatedUtc;
    uint32 RoleSelection=0,SupportSelection=0;
    TArray<FString> ArmourSelection;
    TMap<FString,FRequisitionV87> RequisitionsV87;
    int32 Kills=0,ArmourKills=0;
    FFrontlineCombatRecordV65 CombatRecord;
    bool HonourEligible=true;
    wfexp::tactical::Campaign Campaign;
};
struct FRepository {int32 Revision=0;FGuid Recent;TArray<FEntry> Entries;};
struct FLocalizedMission {FString Title,Restriction,Context,Briefing,Terrain,Objective;};
struct FRuntime
{
    bool Loaded=false,ReadOnly=false,Volatile=false,Selected=false,Recovered=false;
    FString Path,Notice;FRepository Repository;FGuid Current,BoundEncounter,BoundCampaign;
    FString BoundMission;
    TOptional<FRepository> PendingOutcome;
    FGuid PendingCampaign,PendingEncounter,CommittedOutcomeEncounterV826;
    // Encounter-only provenance. Keep the pre-commit first-win decision through
    // a failed save/retry; the public query still requires its committed token.
    bool BoundOutcomeFirstVictoryV87=false;
    int32 BoundRequisitionV87=INDEX_NONE;float BoundSmokeBaselineV87=0;
    TArray<FFRRUCampaignSummaryV82> Rows;
    TArray<FFrontlineCampaignMission> MissionRows;
    std::vector<std::unique_ptr<wfexpue::FBoundMission>> MissionOwners;
    std::vector<std::unique_ptr<FLocalizedMission>> MissionTexts;
    wfexp::Faction CachedNation=wfexp::Faction::Invalid;
    bool CachedChinese=false;
    TSet<FGuid> AssistedCampaigns;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,TSharedPtr<FRuntime>> Sessions;
AWarfareFrontlineGameMode* BattleFor(const AWarfareFrontlineController* PC)
{return PC&&PC->GetWorld()?PC->GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;}
wfexp::Faction NativeNation(EFRRUNationV81 Value)
{return static_cast<wfexp::Faction>(static_cast<uint8>(Value));}
EFRRUNationV81 PublicNation(wfexp::Faction Value)
{return static_cast<EFRRUNationV81>(static_cast<uint8>(Value));}
bool Chinese(){return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;}
FString Local(const char* EN,const char* ZH){return FString(UTF8_TO_TCHAR(Chinese()?ZH:EN));}
bool ReviewPath(FString& Out)
{
    FString Dir;
    const bool Valid=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty()&&!FPaths::IsRelative(Dir);
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    FString Expected=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"));FPaths::NormalizeDirectoryName(Expected);
    FString FullDir=Valid?FPaths::ConvertRelativePathToFull(Dir):FString();FPaths::NormalizeDirectoryName(FullDir);
    // ProjectSavedDir follows -UserDir. Validate against the fixed checkout's
    // Saved root instead, or the safe child path would recursively reject itself.
    FString Saved=FPaths::Combine(Project,TEXT("Saved"));FPaths::NormalizeDirectoryName(Saved);
    const bool Isolated=Valid&&Project.Equals(Expected,ESearchCase::IgnoreCase)&&FullDir.StartsWith(Saved+TEXT("/FRRU/"),ESearchCase::IgnoreCase)&&!FullDir.Contains(TEXT(".."));
    if(Isolated)Out=FPaths::Combine(FullDir,TEXT("FRRUReview/campaigns-tactical-v2.json"));
    return Isolated;
}
enum class ERead { Empty,Valid,Invalid,Unsupported };
bool IsoUtc(const FString& Value)
{
    if(Value.Len()!=20&&Value.Len()!=24)return false;
    for(int32 I=0;I<Value.Len();++I)
    {
        if(I==4||I==7){if(Value[I]!=TEXT('-'))return false;}
        else if(I==10){if(Value[I]!=TEXT('T'))return false;}
        else if(I==13||I==16){if(Value[I]!=TEXT(':'))return false;}
        else if(I==Value.Len()-1){if(Value[I]!=TEXT('Z'))return false;}
        else if(I==19){if(Value[I]!=TEXT('.'))return false;}
        else if(Value[I]<TEXT('0')||Value[I]>TEXT('9'))return false;
    }
    FDateTime Parsed;return FDateTime::ParseIso8601(*Value,Parsed);
}
bool Decode(const FString& Text,FRepository& Out,bool& Unsupported)
{
    Unsupported=false;TSharedPtr<FJsonObject> Root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid())return false;
    double N=0;
    if(!wfexpue::Integer(*Root,TEXT("hostSchema"),0,100000,N))return false;
    if(N!=1&&N!=HostSchema){Unsupported=true;return false;}
    const int32 SourceHostSchema=int32(N);
    FString Profile;if(!Root->TryGetStringField(TEXT("profile"),Profile))return false;
    if(Profile!=TEXT("tactical_v2")){Unsupported=true;return false;}
    FRepository Candidate;
    if(!wfexpue::Integer(*Root,TEXT("revision"),0,1000000000,N))return false;Candidate.Revision=int32(N);
    FString Recent;if(!Root->TryGetStringField(TEXT("activeId"),Recent))return false;
    if(!Recent.IsEmpty()&&(!FGuid::ParseExact(Recent,EGuidFormats::Digits,Candidate.Recent)||!Candidate.Recent.IsValid()))return false;
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(!Root->TryGetArrayField(TEXT("campaigns"),Rows)||!Rows||Rows->Num()>256)return false;
    TSet<FGuid> IDs;
    for(const auto& Value:*Rows)
    {
        if(!Value.IsValid()||Value->Type!=EJson::Object)return false;const auto J=Value->AsObject();if(!J.IsValid())return false;
        FEntry Entry;FString Id;
        if(!J->TryGetStringField(TEXT("id"),Id)||!FGuid::ParseExact(Id,EGuidFormats::Digits,Entry.Id)||!Entry.Id.IsValid()||IDs.Contains(Entry.Id))return false;
        IDs.Add(Entry.Id);
        if(!wfexpue::Integer(*J,TEXT("difficulty"),0,2,N))return false;Entry.Level=EFrontlineDifficulty(uint8(N));
        if(!J->TryGetStringField(TEXT("createdUtc"),Entry.CreatedUtc)||!IsoUtc(Entry.CreatedUtc)
            ||!J->TryGetStringField(TEXT("updatedUtc"),Entry.UpdatedUtc)||!IsoUtc(Entry.UpdatedUtc))return false;
        FDateTime Created,Updated;FDateTime::ParseIso8601(*Entry.CreatedUtc,Created);FDateTime::ParseIso8601(*Entry.UpdatedUtc,Updated);if(Updated<Created)return false;
        if(!wfexpue::Integer(*J,TEXT("roleSelection"),0,(1u<<11)-1,N))return false;Entry.RoleSelection=uint32(N);
        if(!wfexpue::Integer(*J,TEXT("supportSelection"),0,(1u<<10)-1,N))return false;Entry.SupportSelection=uint32(N);
        if(!wfexpue::Integer(*J,TEXT("kills"),0,100000000,N))return false;Entry.Kills=int32(N);
        if(!wfexpue::Integer(*J,TEXT("armourKills"),0,10000000,N))return false;Entry.ArmourKills=int32(N);
        if(!FrontlineCombatRecordV65::ReadArchive(*J,Entry.CombatRecord,Entry.Kills>0||Entry.ArmourKills>0))return false;
        if(!J->HasField(TEXT("combat_record_v65")))
        {Entry.CombatRecord.Total={Entry.Kills,Entry.ArmourKills};Entry.CombatRecord.OtherSources=Entry.CombatRecord.Total;}
        if(Entry.CombatRecord.Total.Infantry!=Entry.Kills||Entry.CombatRecord.Total.Armour!=Entry.ArmourKills)return false;
        if(!J->TryGetBoolField(TEXT("honourEligible"),Entry.HonourEligible))return false;
        const TSharedPtr<FJsonObject>* Tactical=nullptr;FString Error;
        if(!J->TryGetObjectField(TEXT("tactical"),Tactical)||!Tactical||!Tactical->IsValid())return false;
        double SchemaValue=0;FString TacticalProfile;
        if((*Tactical)->TryGetNumberField(TEXT("schemaVersion"),SchemaValue)&&SchemaValue!=2){Unsupported=true;return false;}
        if((*Tactical)->TryGetNumberField(TEXT("rulesVersion"),SchemaValue)&&SchemaValue!=2){Unsupported=true;return false;}
        if((*Tactical)->TryGetStringField(TEXT("profile"),TacticalProfile)&&TacticalProfile!=TEXT("tactical_v2")){Unsupported=true;return false;}
        const TSharedPtr<FJsonObject>* Progression=nullptr;
        if((*Tactical)->TryGetObjectField(TEXT("progression"),Progression)&&Progression&&Progression->IsValid())
        {
            if((*Progression)->TryGetNumberField(TEXT("schemaVersion"),SchemaValue)&&SchemaValue!=1){Unsupported=true;return false;}
            if((*Progression)->TryGetNumberField(TEXT("contentRevision"),SchemaValue)&&SchemaValue!=1){Unsupported=true;return false;}
        }
        if(!wfexpue::ReadTacticalCampaign(**Tactical,Entry.Campaign,Error))return false;
        if(!J->HasField(TEXT("combat_record_v65")))
            Entry.CombatRecord.bIncludesEarlierUnrecordedBattles=Entry.Kills>0||Entry.ArmourKills>0
                ||Entry.Campaign.progression.victories>0||Entry.Campaign.progression.defeats>0;
        // Schema 1 had no armour selection. Migration is additive and does not
        // change progression, research, GUIDs or retired encounter receipts.
        if(SourceHostSchema==HostSchema)
        {
            const TArray<TSharedPtr<FJsonValue>>* Selected=nullptr;
            if(!J->TryGetArrayField(TEXT("armourSelection"),Selected)||!Selected||Selected->Num()>2)return false;
            for(const auto& Selection:*Selected)
            {
                FString StableId;if(!Selection||!Selection->TryGetString(StableId)||Entry.ArmourSelection.Contains(StableId))return false;
                const auto* Vehicle=wfexp::FindVehicle(wfexpue::Narrow(StableId));
                if(!Vehicle||Vehicle->faction!=Entry.Campaign.faction)return false;
                Entry.ArmourSelection.Add(StableId);
            }
        }
        // Host-owned allocation metadata is additive. The frozen native
        // TacticalV2 campaign/profile/technology codec remains unchanged.
        if(J->HasField(TEXT("requisitionsV87")))
        {
            const TArray<TSharedPtr<FJsonValue>>* Allocations=nullptr;
            if(!J->TryGetArrayField(TEXT("requisitionsV87"),Allocations)||!Allocations||Allocations->Num()>10)return false;
            for(const auto& AllocationValueV87:*Allocations)
            {
                const TSharedPtr<FJsonObject>* Item=nullptr;FString Mission;double Kind=0;bool Paid=false;
                if(!AllocationValueV87.IsValid()||!AllocationValueV87->TryGetObject(Item)||!Item||!Item->IsValid()
                    ||!(*Item)->TryGetStringField(TEXT("mission"),Mission)||Entry.RequisitionsV87.Contains(Mission)
                    ||!wfexpue::Integer(**Item,TEXT("kind"),0,2,Kind)||!(*Item)->TryGetBoolField(TEXT("paid"),Paid))return false;
                const auto* Operation=wfexp::FindMission(wfexpue::Narrow(Mission));
                if(!Operation||Operation->faction!=Entry.Campaign.faction||(Paid&&Operation->order<8))return false;
                Entry.RequisitionsV87.Add(Mission,{int32(Kind),Paid});
            }
        }
        Candidate.Entries.Add(MoveTemp(Entry));
    }
    if(Candidate.Recent.IsValid()&&!IDs.Contains(Candidate.Recent))return false;
    Out=MoveTemp(Candidate);return true;
}
bool Encode(const FRepository& Repo,FString& Out)
{
    auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("hostSchema"),HostSchema);Root->SetStringField(TEXT("profile"),TEXT("tactical_v2"));
    Root->SetNumberField(TEXT("revision"),Repo.Revision);Root->SetStringField(TEXT("activeId"),Repo.Recent.IsValid()?Repo.Recent.ToString(EGuidFormats::Digits):FString());
    TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& Entry:Repo.Entries)
    {
        TSharedPtr<FJsonObject> Tactical;FString Error;if(!wfexpue::WriteTacticalCampaign(Entry.Campaign,Tactical,Error))return false;
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("id"),Entry.Id.ToString(EGuidFormats::Digits));J->SetNumberField(TEXT("difficulty"),int32(Entry.Level));
        J->SetStringField(TEXT("createdUtc"),Entry.CreatedUtc);J->SetStringField(TEXT("updatedUtc"),Entry.UpdatedUtc);
        J->SetNumberField(TEXT("roleSelection"),Entry.RoleSelection);J->SetNumberField(TEXT("supportSelection"),Entry.SupportSelection);
        TArray<TSharedPtr<FJsonValue>> Armour;for(const auto& Id:Entry.ArmourSelection)Armour.Add(MakeShared<FJsonValueString>(Id));J->SetArrayField(TEXT("armourSelection"),Armour);
        TArray<FString> AllocationIds;Entry.RequisitionsV87.GetKeys(AllocationIds);AllocationIds.Sort();TArray<TSharedPtr<FJsonValue>> Allocations;
        for(const auto& Mission:AllocationIds)
        {const auto& Order=Entry.RequisitionsV87.FindChecked(Mission);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("mission"),Mission);Item->SetNumberField(TEXT("kind"),Order.Kind);Item->SetBoolField(TEXT("paid"),Order.Paid);Allocations.Add(MakeShared<FJsonValueObject>(Item));}
        J->SetArrayField(TEXT("requisitionsV87"),Allocations);
        J->SetNumberField(TEXT("kills"),Entry.Kills);J->SetNumberField(TEXT("armourKills"),Entry.ArmourKills);J->SetBoolField(TEXT("honourEligible"),Entry.HonourEligible);
        FrontlineCombatRecordV65::WriteArchive(*J,Entry.CombatRecord);
        J->SetObjectField(TEXT("tactical"),Tactical);Rows.Add(MakeShared<FJsonValueObject>(J));
    }
    Root->SetArrayField(TEXT("campaigns"),Rows);return FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Out));
}
bool ReadFile(const FString& Path,FRepository& Out,bool& Unsupported)
{
    Unsupported=false;const int64 Bytes=IFileManager::Get().FileSize(*Path);FString Text;
    return Bytes>0&&Bytes<=MaximumRepositoryBytes&&FFileHelper::LoadFileToString(Text,*Path)&&Decode(Text,Out,Unsupported);
}
ERead ReadRepository(const FString& Path,FRepository& Out,bool& Recovered)
{
    FRepository Primary,Backup;bool PFuture=false,BFuture=false;Recovered=false;
    const bool PGood=ReadFile(Path,Primary,PFuture),BGood=ReadFile(Path+TEXT(".bak"),Backup,BFuture);
    if(PFuture||BFuture)return ERead::Unsupported;
    if(PGood){Out=MoveTemp(Primary);return ERead::Valid;}
    if(BGood){Out=MoveTemp(Backup);Recovered=true;return ERead::Valid;}
    return IFileManager::Get().FileExists(*Path)||IFileManager::Get().FileExists(*(Path+TEXT(".bak")))?ERead::Invalid:ERead::Empty;
}
class FRepositoryLock
{
#if PLATFORM_WINDOWS
    HANDLE Handle=nullptr;bool Owned=false;
#endif
public:
    explicit FRepositoryLock(const FString& Path)
    {
#if PLATFORM_WINDOWS
        const FString Name=FString::Printf(TEXT("Local\\WarfareFrontFRRUTacticalV2_%08x"),GetTypeHash(Path.ToLower()));
        Handle=CreateMutexW(nullptr,false,*Name);if(Handle){const DWORD Wait=WaitForSingleObject(Handle,3000);Owned=Wait==WAIT_OBJECT_0||Wait==WAIT_ABANDONED;}
#endif
    }
    ~FRepositoryLock()
    {
#if PLATFORM_WINDOWS
        if(Owned)ReleaseMutex(Handle);if(Handle)CloseHandle(Handle);
#endif
    }
    bool Valid()const
    {
#if PLATFORM_WINDOWS
        return Owned;
#else
        return false;
#endif
    }
};
bool Replace(const FString& To,const FString& From)
{
#if PLATFORM_WINDOWS
    return MoveFileExW(*From,*To,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return IFileManager::Get().Move(*To,*From,true,true,false,true);
#endif
}

void RefreshRows(FRuntime& Runtime)
{
    Runtime.Rows.Reset();for(const auto& E:Runtime.Repository.Entries)
    {
        FFRRUCampaignSummaryV82 Row;Row.Id=E.Id;Row.PlayerNation=PublicNation(E.Campaign.faction);Row.Difficulty=E.Level;
        Row.MissionId=wfexpue::Wide(E.Campaign.progression.selectedMissionId);Row.UpdatedUtc=E.UpdatedUtc;
        Row.Victories=E.Campaign.progression.victories;Row.Defeats=E.Campaign.progression.defeats;
        Row.Completed=int32(E.Campaign.progression.wonMissionIds.size());Row.ResearchPoints=E.Campaign.progression.researchPoints;
        Row.InfantryKills=E.Kills;Row.ArmourKills=E.ArmourKills;Row.Orders=int32(E.Campaign.purchasedOptions.size());
        Row.CombatRecord=E.CombatRecord;
        Row.HonourEligible=E.HonourEligible&&!Runtime.AssistedCampaigns.Contains(E.Id);
        if(const auto* M=wfexp::FindMission(E.Campaign.progression.selectedMissionId))Row.Title=wfexpue::Wide(Chinese()?M->title.zh:M->title.en);
        Runtime.Rows.Add(MoveTemp(Row));
    }
}
FRuntime& RuntimeFor(const AWarfareFrontlineGameMode* Battle)
{
    const TWeakObjectPtr<AWarfareFrontlineGameMode> Key(const_cast<AWarfareFrontlineGameMode*>(Battle));
    auto* Existing=Sessions.Find(Key);if(!Existing){for(auto It=Sessions.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();Existing=&Sessions.Add(Key,MakeShared<FRuntime>());}
    FRuntime& Runtime=**Existing;
    if(!Runtime.Loaded)
    {
        Runtime.Loaded=true;bool Review=false,PersistenceReview=false,InvalidPersistenceReview=false;
        if(PersistenceReviewRequestedV82())
        {
            FString User;PersistenceReview=TryPersistenceReviewUserV82(User);
            if(PersistenceReview)Runtime.Path=FPaths::Combine(User,TEXT("Saved/FRRU/Profiles/frru-tactical-v2.json"));
            else
            {
                InvalidPersistenceReview=true;Runtime.ReadOnly=true;Runtime.Path.Reset();
                Runtime.Notice=Local("Campaign records could not be opened. Existing files have been retained.","无法打开战役档案，原文件已保留。");
                UE_LOG(LogTemp,Error,TEXT("FRRU persistence review refused: exact isolated project, command-line and disposable UserDir agreement are required."));
            }
        }
        else
        Review=ReviewPath(Runtime.Path);
        Runtime.Volatile=((Battle&&Battle->IsProgressionVolatile())||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review")))&&!Review&&!PersistenceReview;
        if(!Review&&!PersistenceReview&&!InvalidPersistenceReview)Runtime.Path=FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("WarfareFront/Campaigns/frru-tactical-v2.json"));
        if(!Runtime.Volatile&&!InvalidPersistenceReview)
        {
            FRepositoryLock Lock(Runtime.Path);if(!Lock.Valid()){Runtime.ReadOnly=true;Runtime.Notice=Local("Campaign records are in use. Try again after closing the other game.","战役档案正在使用，请关闭另一游戏窗口后重试。");}
            else
            {
                const ERead Read=ReadRepository(Runtime.Path,Runtime.Repository,Runtime.Recovered);
                Runtime.ReadOnly=Read==ERead::Invalid||Read==ERead::Unsupported;
                if(Runtime.ReadOnly)Runtime.Notice=Read==ERead::Unsupported?Local("These campaign records need a compatible game version. Existing files have been retained.","战役档案版本不兼容，原文件已保留。"):Local("Campaign records could not be read. Existing files have been retained.","无法读取战役档案，原文件已保留。");
            }
        }
        RefreshRows(Runtime);
    }
    return Runtime;
}
FEntry* Current(FRuntime& Runtime){return Runtime.Selected?Runtime.Repository.Entries.FindByPredicate([&](const FEntry& E){return E.Id==Runtime.Current;}):nullptr;}
bool SameEntryV826(const FEntry& A,const FEntry& B)
{
    FRepository RA,RB;RA.Recent=A.Id;RB.Recent=B.Id;RA.Entries.Add(A);RB.Entries.Add(B);
    FString SA,SB;return Encode(RA,SA)&&Encode(RB,SB)&&SA==SB;
}
bool MergePendingOutcomeV826(const FRuntime& Runtime,const FRepository& Disk,FRepository& Candidate)
{
    if(!Runtime.PendingOutcome.IsSet()||!Runtime.PendingEncounter.IsValid()||Runtime.Current!=Runtime.PendingCampaign
        ||Runtime.BoundCampaign!=Runtime.Current||Runtime.BoundEncounter!=Runtime.PendingEncounter)return false;
    const auto* Base=Runtime.Repository.Entries.FindByPredicate([&](const FEntry& E){return E.Id==Runtime.Current;});
    const auto* Latest=Disk.Entries.FindByPredicate([&](const FEntry& E){return E.Id==Runtime.Current;});
    const auto* Outcome=Candidate.Entries.FindByPredicate([&](const FEntry& E){return E.Id==Runtime.Current;});
    if(!Base||!Latest||!Outcome||!SameEntryV826(*Base,*Latest))return false;
    const auto Token=wfexpue::Narrow(Runtime.PendingEncounter.ToString(EGuidFormats::DigitsWithHyphensLower));
    if(Base->Campaign.progression.activeEncounterId!=Token
        ||std::find(Outcome->Campaign.progression.resolvedEncounterIds.begin(),Outcome->Campaign.progression.resolvedEncounterIds.end(),Token)==Outcome->Campaign.progression.resolvedEncounterIds.end())return false;
    // A pending result may replace only its unchanged base campaign. Keep the
    // latest other campaigns, recent-selection marker and repository revision.
    FRepository Merged=Disk;
    *Merged.Entries.FindByPredicate([&](const FEntry& E){return E.Id==Runtime.Current;})=*Outcome;
    Candidate=MoveTemp(Merged);return true;
}
bool Commit(FRuntime& Runtime,FRepository Candidate,bool PendingOutcomeMerge=false)
{
    if(Runtime.ReadOnly)return false;
    // Assistance is fail-closed for the live session even if its first write
    // fails. Every later transaction retains that invalidation on disk.
    for(auto& Entry:Candidate.Entries)if(Runtime.AssistedCampaigns.Contains(Entry.Id))Entry.HonourEligible=false;
    Candidate.Revision=Runtime.Repository.Revision+1;
    FString Text;if(!Encode(Candidate,Text))return false;FRepository Verified;bool Future=false;if(!Decode(Text,Verified,Future))return false;
    if(!Runtime.Volatile)
    {
        FRepositoryLock Lock(Runtime.Path);if(!Lock.Valid()){Runtime.Notice=Local("Campaign records are in use.","战役档案正在使用。");return false;}
        FRepository Disk;bool Recovered=false;const ERead Read=ReadRepository(Runtime.Path,Disk,Recovered);
        if(Read==ERead::Invalid||Read==ERead::Unsupported){Runtime.ReadOnly=true;Runtime.Notice=Local("Campaign records could not be updated. Existing files have been retained.","无法更新战役档案，原文件已保留。");return false;}
        if((Read==ERead::Valid&&Disk.Revision!=Runtime.Repository.Revision)||(Read==ERead::Empty&&Runtime.Repository.Revision!=0))
        {
            if(!PendingOutcomeMerge||Read!=ERead::Valid||!MergePendingOutcomeV826(Runtime,Disk,Candidate))
            {Runtime.Notice=Local("This campaign was changed in another game. Close that game before trying again.","另一窗口已改动这份战役，请先关闭该窗口。");return false;}
            Candidate.Revision=Disk.Revision+1;
            if(!Encode(Candidate,Text)||!Decode(Text,Verified,Future))return false;
        }
        if(!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Runtime.Path),true))return false;
        const FString Temp=Runtime.Path+TEXT(".tmp-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        ON_SCOPE_EXIT{IFileManager::Get().Delete(*Temp);};
        if(!FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)||!ReadFile(Temp,Verified,Future))return false;
        if(Read==ERead::Valid&&!Recovered)
        {
            const FString BackupTemp=Runtime.Path+TEXT(".bak-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
            ON_SCOPE_EXIT{IFileManager::Get().Delete(*BackupTemp);};
            if(IFileManager::Get().Copy(*BackupTemp,*Runtime.Path,true,true)!=COPY_OK||!Replace(Runtime.Path+TEXT(".bak"),BackupTemp))return false;
        }
        else if(Recovered&&IFileManager::Get().FileExists(*Runtime.Path))
        {
            // Preserve a damaged primary before recovering from the good backup.
            const FString Retained=Runtime.Path+TEXT(".retained-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
            if(IFileManager::Get().Copy(*Retained,*Runtime.Path,false,true)!=COPY_OK)return false;
        }
        if(!Replace(Runtime.Path,Temp))return false;
        // The primary is the commit point. Mirror failure cannot roll back an
        // already committed purchase or invent a second outcome on retry.
        const FString Mirror=Runtime.Path+TEXT(".mirror-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        const bool Mirrored=IFileManager::Get().Copy(*Mirror,*Runtime.Path,true,true)==COPY_OK&&Replace(Runtime.Path+TEXT(".bak"),Mirror);
        if(!Mirrored){IFileManager::Get().Delete(*Mirror);Runtime.Notice=Local("Campaign saved. The backup will be refreshed on the next save.","战役已保存，备份将在下次保存时更新。");}else Runtime.Notice.Reset();
    }
    Runtime.Repository=MoveTemp(Candidate);Runtime.Recovered=false;RefreshRows(Runtime);return true;
}
void CacheMissions(FRuntime& Runtime,wfexp::Faction Faction)
{
    const bool ZH=Chinese();if(Runtime.CachedNation==Faction&&Runtime.CachedChinese==ZH&&!Runtime.MissionRows.IsEmpty())return;
    Runtime.MissionRows.Reset();Runtime.MissionOwners.clear();Runtime.MissionTexts.clear();Runtime.CachedNation=Faction;Runtime.CachedChinese=ZH;
    for(const auto& Row:wfexp::Missions())if(Row.faction==Faction)
    {
        Runtime.MissionOwners.emplace_back(std::make_unique<wfexpue::FBoundMission>(Row,ZH));
        auto Mission=Runtime.MissionOwners.back()->Get().Mission;
        auto Prose=std::make_unique<FLocalizedMission>();
        Prose->Title=WarfareFRRUTextV82::Resolve(wfexpue::Wide(Row.id),TEXT("title"));
        Prose->Restriction=WarfareFRRUTextV82::Resolve(wfexpue::Wide(Row.id),TEXT("restrictionText"));
        Prose->Context=WarfareFRRUTextV82::Resolve(wfexpue::Wide(Row.id),TEXT("historicalContext"));
        Prose->Briefing=WarfareFRRUTextV82::Resolve(wfexpue::Wide(Row.id),TEXT("briefing"));
        Prose->Terrain=WarfareFRRUTextV82::Resolve(wfexpue::Wide(Row.id),TEXT("terrainNotes"));
        Prose->Objective=WarfareFRRUTextV82::Resolve(wfexpue::Wide(Row.id),TEXT("objectiveText"));
        if(!Prose->Title.IsEmpty())Mission.Title=*Prose->Title;
        if(!Prose->Restriction.IsEmpty())Mission.RestrictionText=*Prose->Restriction;
        if(!Prose->Context.IsEmpty())Mission.HistoricalContext=*Prose->Context;
        if(!Prose->Briefing.IsEmpty())Mission.Briefing=*Prose->Briefing;
        if(!Prose->Terrain.IsEmpty())Mission.TerrainNotes=*Prose->Terrain;
        if(!Prose->Objective.IsEmpty())Mission.ObjectiveText=*Prose->Objective;
        Runtime.MissionTexts.emplace_back(std::move(Prose));Runtime.MissionRows.Add(Mission);
    }
}
bool Editable(const AWarfareFrontlineGameMode* Battle,const FRuntime& Runtime)
{return Battle&&Battle->Phase!=EFrontlinePhase::Fighting&&!Runtime.ReadOnly&&!Runtime.PendingOutcome.IsSet();}
}

bool Active(const AWarfareFrontlineGameMode* Battle){return Battle&&Current(RuntimeFor(Battle));}
bool Active(const AWarfareFrontlineController* Controller){return Active(BattleFor(Controller));}
EFRRUNationV81 Nation(const AWarfareFrontlineGameMode* Battle,int32 Side)
{
    if(!Battle||Side<0||Side>1)return EFRRUNationV81::Invalid;
    auto& Runtime=RuntimeFor(Battle);if(const auto* Entry=Current(Runtime))
    {
        const auto* M=wfexp::FindMission(Entry->Campaign.progression.selectedMissionId);
        if(!Runtime.BoundMission.IsEmpty()&&Runtime.BoundEncounter==Battle->GetEncounterId())M=wfexp::FindMission(wfexpue::Narrow(Runtime.BoundMission));
        return M?PublicNation(Side==0?M->faction:M->enemy):EFRRUNationV81::Invalid;
    }
    return Battle->GetFactionForTeam(Side)==EFrontlineFaction::German?EFRRUNationV81::German:EFRRUNationV81::British;
}
EFRRUNationV81 Nation(const AWarfareFrontlineController* Controller,int32 Side){return Nation(BattleFor(Controller),Side);}
EFRRUNationV81 Nation(const AWarfareFrontlineSoldier* Soldier){return Soldier?Nation(Soldier->Battle, Soldier->Team):EFRRUNationV81::Invalid;}
int32 RoleId(const AWarfareFrontlineSoldier* Soldier){return Soldier?int32(Soldier->Role):INDEX_NONE;}
FGuid CampaignId(const AWarfareFrontlineGameMode* Battle){if(!Battle)return FGuid();auto& R=RuntimeFor(Battle);const auto* E=Current(R);return E?E->Id:FGuid();}
FString MissionId(const AWarfareFrontlineGameMode* Battle)
{if(!Battle)return FString();auto& R=RuntimeFor(Battle);if(R.BoundEncounter==Battle->GetEncounterId()&&R.BoundCampaign==CampaignId(Battle)&&!R.BoundMission.IsEmpty())return R.BoundMission;const auto* E=Current(R);return E?wfexpue::Wide(E->Campaign.progression.selectedMissionId):FString();}
EFrontlineDifficulty Difficulty(const AWarfareFrontlineGameMode* Battle){if(!Battle)return EFrontlineDifficulty::Rookie;auto& R=RuntimeFor(Battle);const auto* E=Current(R);return E?E->Level:EFrontlineDifficulty::Rookie;}
const TArray<FFrontlineCampaignMission>& Missions(const AWarfareFrontlineGameMode* Battle)
{
    static const TArray<FFrontlineCampaignMission> Empty;if(!Battle)return Empty;auto& R=RuntimeFor(Battle);const auto* E=Current(R);if(!E)return Empty;CacheMissions(R,E->Campaign.faction);return R.MissionRows;
}
const FFrontlineCampaignMission* SelectedMission(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle)return nullptr;const auto* Row=wfexp::FindMission(wfexpue::Narrow(MissionId(Battle)));if(!Row)return nullptr;
    const int32 Id=(Row->faction==wfexp::Faction::French?1000:2000)+Row->order;
    return Missions(Battle).FindByPredicate([&](const FFrontlineCampaignMission& M){return M.Id==Id;});
}
const FFrontlineCampaignMission* EncounterMission(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle)return nullptr;auto& R=RuntimeFor(Battle);if(R.BoundEncounter!=Battle->GetEncounterId()||R.BoundCampaign!=CampaignId(Battle))return nullptr;
    const auto* Row=wfexp::FindMission(wfexpue::Narrow(R.BoundMission));if(!Row)return nullptr;const int32 Id=(Row->faction==wfexp::Faction::French?1000:2000)+Row->order;
    return Missions(Battle).FindByPredicate([&](const FFrontlineCampaignMission& M){return M.Id==Id;});
}
bool HasNewAssignmentReceiptV87(const AWarfareFrontlineGameMode* Battle,int32 CompletedHostMissionId)
{
    if(!Battle||!Active(Battle)||Battle->Phase!=EFrontlinePhase::Victory||ArchiveReplayActiveV87(Battle))return false;
    auto& R=RuntimeFor(Battle);const auto* E=Current(R);
    if(!E||R.ReadOnly||!R.BoundOutcomeFirstVictoryV87
        ||!WarfareFRRUCampaignInternalV82::OutcomeReceiptCommittedV826(Battle))return false;
    const auto* Operation=wfexp::FindMission(wfexpue::Narrow(R.BoundMission));
    return Operation&&Operation->faction==E->Campaign.faction
        &&CompletedHostMissionId==(Operation->faction==wfexp::Faction::French?1000:2000)+Operation->order
        &&std::find(E->Campaign.progression.wonMissionIds.begin(),E->Campaign.progression.wonMissionIds.end(),Operation->id)
            !=E->Campaign.progression.wonMissionIds.end();
}
int32 CalendarDate(const AWarfareFrontlineGameMode* Battle)
{const auto* M=Active(Battle)?EncounterMission(Battle):Battle?Battle->GetEncounterCampaignMission():nullptr;if(!M)M=Active(Battle)?SelectedMission(Battle):Battle?Battle->GetSelectedCampaignMission():nullptr;return M?M->CalendarDateV41:19170409;}
int32 CalendarDate(const AWarfareFrontlineController* Controller){return CalendarDate(BattleFor(Controller));}
const TArray<FFRRUCampaignSummaryV82>& Archives(const AWarfareFrontlineGameMode* Battle){static const TArray<FFRRUCampaignSummaryV82> Empty;if(!Battle)return Empty;auto& R=RuntimeFor(Battle);RefreshRows(R);return R.Rows;}
bool CurrentRecord(const AWarfareFrontlineGameMode* Battle,FFRRUCampaignSummaryV82& Out)
{
    if(!Battle||!Active(Battle))return false;const auto& Rows=Archives(Battle);const FGuid Id=CampaignId(Battle);
    if(const auto* Row=Rows.FindByPredicate([&](const FFRRUCampaignSummaryV82& E){return E.Id==Id;})){Out=*Row;return true;}return false;
}
const FFrontlineCombatRecordV65* CampaignCombatRecordV87(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle))return nullptr;
    const auto* Entry=Current(RuntimeFor(Battle));return Entry?&Entry->CombatRecord:nullptr;
}
bool CampaignReportMetadataV87(const AWarfareFrontlineGameMode* Battle,FGuid& Id,int32& Victories,int32& Defeats)
{
    if(!Battle||!Active(Battle))return false;const auto* Entry=Current(RuntimeFor(Battle));if(!Entry)return false;
    Id=Entry->Id;Victories=Entry->Campaign.progression.victories;Defeats=Entry->Campaign.progression.defeats;return true;
}
bool FinalizeCampaignCombatRecordV87(AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle)||(Battle->Phase!=EFrontlinePhase::Victory&&Battle->Phase!=EFrontlinePhase::Defeat))return true;
    if(!WarfareFRRUCampaignInternalV82::RetryOutcome(Battle)
        ||!WarfareFRRUCampaignInternalV82::OutcomeReceiptCommittedV826(Battle))return false;
    auto& Runtime=RuntimeFor(Battle);const auto* Entry=Current(Runtime);
    const auto* Operation=wfexp::FindMission(wfexpue::Narrow(Runtime.BoundMission));
    if(!Entry||!Operation||Runtime.ReadOnly)return false;
    const FGuid Encounter=Battle->GetEncounterId();
    const auto* Previous=Entry->CombatRecord.BattlesV86.FindByPredicate([&](const auto& Row){return Row.Encounter==Encounter;});
    if(!Previous||!Previous->Armies[0].IsValid())return false;
    const auto Tail=FrontlineReportV839::CombatTailV87(Battle->CombatRecordV65[0],*Previous->Armies[0]);
    if(int64(Entry->Kills)+Tail.Total.Infantry>100000000||int64(Entry->ArmourKills)+Tail.Total.Armour>10000000)return false;
    FRepository Candidate=Runtime.Repository;auto* Updated=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==Entry->Id;});if(!Updated)return false;
    const bool ReturnChanged=FrontlineReportV839::CaptureCampaignBattleV86(Updated->CombatRecord,Battle,Operation->order,Battle->Phase==EFrontlinePhase::Victory?0:1);
    const bool Counts=Tail.Total.Total()>0||Tail.InfantryLost>0||Tail.ArmourLost>0||Tail.DetailsV839.HasCounts();
    if(!Counts&&!ReturnChanged)return true;
    // Upsert replaces the same encounter's two-sided snapshot. Add only the
    // measured tail to the campaign totals; battles/victories are untouched.
    Updated->Kills+=Tail.Total.Infantry;Updated->ArmourKills+=Tail.Total.Armour;Updated->CombatRecord.Add(Tail);
    Updated->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(Runtime,MoveTemp(Candidate));
}
bool CanOpenEnding(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle)||ArchiveReplayActiveV87(Battle)||Battle->IsBattleOutroActive()||RuntimeFor(Battle).PendingOutcome.IsSet())return false;
    const auto* E=Current(RuntimeFor(Battle));if(!E)return false;
    int32 Required=0;for(const auto& Mission:wfexp::Missions())if(Mission.faction==E->Campaign.faction)
    {++Required;if(std::find(E->Campaign.progression.wonMissionIds.begin(),E->Campaign.progression.wonMissionIds.end(),Mission.id)==E->Campaign.progression.wonMissionIds.end())return false;}
    return Required>0&&(Battle->Phase==EFrontlinePhase::Briefing||Battle->Phase==EFrontlinePhase::Victory);
}
bool HonourEligible(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle))return false;auto& Runtime=RuntimeFor(Battle);const auto* Entry=Current(Runtime);
    return Entry&&Entry->HonourEligible&&!Runtime.ReadOnly&&!Runtime.PendingOutcome.IsSet()&&!Runtime.AssistedCampaigns.Contains(Entry->Id);
}
bool MarkHonourAssisted(AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle))return false;auto& Runtime=RuntimeFor(Battle);const auto* Entry=Current(Runtime);if(!Entry)return false;
    const FGuid Id=Entry->Id;Runtime.AssistedCampaigns.Add(Id);
    if(Runtime.PendingOutcome.IsSet())for(auto& Pending:Runtime.PendingOutcome.GetValue().Entries)if(Pending.Id==Id)Pending.HonourEligible=false;
    if(!Entry->HonourEligible)return true;
    FRepository Candidate=Runtime.Repository;auto* Changed=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==Id;});if(!Changed)return false;
    Changed->HonourEligible=false;Changed->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(Runtime,MoveTemp(Candidate));
}
void Deactivate(AWarfareFrontlineGameMode* Battle)
{if(!Battle)return;auto& R=RuntimeFor(Battle);if(R.PendingOutcome.IsSet())return;R.Selected=false;R.Current.Invalidate();R.BoundEncounter.Invalidate();R.BoundCampaign.Invalidate();R.BoundMission.Reset();R.BoundOutcomeFirstVictoryV87=false;R.BoundRequisitionV87=INDEX_NONE;R.BoundSmokeBaselineV87=0;}
FString Status(const AWarfareFrontlineGameMode* Battle){return Battle?RuntimeFor(Battle).Notice:FString();}
namespace
{
// Reopen each country only after its own actual-runtime and package acceptance.
constexpr bool FrenchCampaignReleaseEnabledV87=false;
constexpr bool RussianCampaignReleaseEnabledV87=false;
bool PrivateCampaignReviewV87()
{
#if WITH_EDITOR
    FString User;
    // The strict normal-save review deliberately has no NoSave flag. Reuse its
    // complete project/UserDir/flag validation rather than weakening that route.
    if(TryPersistenceReviewUserV82(User))return true;
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)
        ||FPaths::IsRelative(User)||User.Contains(TEXT("..")))return false;
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    FPaths::NormalizeDirectoryName(Project);FPaths::NormalizeDirectoryName(User);
    return Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        &&User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase);
#else
    return false;
#endif
}
}
bool CampaignsAvailable(EFRRUNationV81 N)
{
    if(N!=EFRRUNationV81::French&&N!=EFRRUNationV81::Russian)return false;
    if(PrivateCampaignReviewV87())return true;
    return N==EFRRUNationV81::French?FrenchCampaignReleaseEnabledV87:RussianCampaignReleaseEnabledV87;
}
bool Start(AWarfareFrontlineGameMode* Battle,EFRRUNationV81 PlayerNation,EFrontlineDifficulty Level)
{
    if(!Battle||(PlayerNation!=EFRRUNationV81::French&&PlayerNation!=EFRRUNationV81::Russian)||uint8(Level)>2)return false;
    if(!CampaignsAvailable(PlayerNation)){RuntimeFor(Battle).Notice=Local("Not yet available.","暂未开放。");return false;}
    auto& R=RuntimeFor(Battle);if(!Editable(Battle,R)||R.Repository.Entries.Num()>=256)return false;
    if(Active(Battle)&&!WarfareFRRUCampaignInternalV82::RetireEncounter(Battle))return false;
    FEntry New;New.Id=FGuid::NewGuid();New.Level=Level;New.CreatedUtc=New.UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    const auto Result=wfexp::tactical::NewTacticalCampaign(NativeNation(PlayerNation),New.Campaign);if(!Result){R.Notice=wfexpue::Wide(Result.reason);return false;}
    FRepository Candidate=R.Repository;Candidate.Entries.Add(New);Candidate.Recent=New.Id;if(!Commit(R,MoveTemp(Candidate)))return false;
    R.Current=New.Id;R.Selected=true;Battle->RestartBattle();return true;
}
bool Continue(AWarfareFrontlineGameMode* Battle,const FGuid& Id)
{
    if(!Battle)return false;auto& R=RuntimeFor(Battle);if(!Editable(Battle,R)||R.Repository.Entries.IsEmpty())return false;
    const FGuid Wanted=Id.IsValid()?Id:(R.Repository.Recent.IsValid()?R.Repository.Recent:R.Repository.Entries.Last().Id);
    const auto* Target=R.Repository.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==Wanted;});if(!Target)return false;
    if(!CampaignsAvailable(PublicNation(Target->Campaign.faction))){R.Notice=Local("Not yet available.","暂未开放。");return false;}
    if(!WarfareFRRUCampaignInternalV82::RetireEncounter(Battle))return false;
    // An explicit archive switch must retire the old encounter while its own
    // campaign and outcome binding are still selected.
    if(R.Selected&&R.Current!=Wanted&&Battle->Phase!=EFrontlinePhase::Briefing&&!Battle->RestartBattle())return false;
    FRepository Candidate=R.Repository;auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==Wanted;});if(!E)return false;
    if(!CampaignsAvailable(PublicNation(E->Campaign.faction))){R.Notice=Local("Not yet available.","暂未开放。");return false;}
    // A campaign codec cannot resurrect old actors or late shell callbacks.
    if(!E->Campaign.progression.activeEncounterId.empty())if(!wfexp::AbortEncounter(E->Campaign.progression))return false;
    E->UpdatedUtc=FDateTime::UtcNow().ToIso8601();Candidate.Recent=Wanted;if(!Commit(R,MoveTemp(Candidate)))return false;
    // Restart finalizes the real return before World::Reset clears its binding.
    // Clearing it here makes the second finalization reject an already saved win.
    R.Current=Wanted;R.Selected=true;return Battle->RestartBattle();
}
FString MissionBlockReason(const AWarfareFrontlineGameMode* Battle,const FString& StableId)
{
    if(!CampaignsAvailable(Nation(Battle)))return Local("Not yet available.","暂未开放。");
    if(!Battle||!Active(Battle))return Local("Choose a campaign first.","请先选择战役。");auto& R=RuntimeFor(Battle);const auto* E=Current(R);
    if(!Editable(Battle,R))return R.ReadOnly?R.Notice:Local("Finish the current battle first.","请先结束当前战斗。");
    return wfexp::CanStart(E->Campaign.progression,wfexpue::Narrow(StableId))?FString():Local("This operation has not been assigned.","这项行动尚未签发。");
}
bool SelectMission(AWarfareFrontlineGameMode* Battle,const FString& StableId)
{
    if(!MissionBlockReason(Battle,StableId).IsEmpty())return false;auto& R=RuntimeFor(Battle);FRepository Candidate=R.Repository;
    auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==R.Current;});if(!E)return false;
    const auto Result=wfexp::SelectMission(E->Campaign.progression,wfexpue::Narrow(StableId));if(!Result){R.Notice=wfexpue::Wide(Result.reason);return false;}
    E->UpdatedUtc=FDateTime::UtcNow().ToIso8601();if(!Commit(R,MoveTemp(Candidate)))return false;Battle->RestartBattle();return true;
}
bool Delete(AWarfareFrontlineGameMode* Battle,const TArray<FGuid>& Ids)
{
    if(!Battle||Battle->Phase!=EFrontlinePhase::Briefing||Ids.IsEmpty())return false;auto& R=RuntimeFor(Battle);if(R.ReadOnly)return false;
    TSet<FGuid> Unique;for(const auto& Id:Ids){if(!Id.IsValid()||!R.Repository.Entries.ContainsByPredicate([&](const FEntry& E){return E.Id==Id;}))return false;Unique.Add(Id);}
    FRepository Candidate=R.Repository;Candidate.Entries.RemoveAll([&](const FEntry& E){return Unique.Contains(E.Id);});
    if(Unique.Contains(Candidate.Recent))Candidate.Recent=Candidate.Entries.IsEmpty()?FGuid():Candidate.Entries.Last().Id;
    const bool Removed=Unique.Contains(R.Current);FString HonourNotice;
    if(!FrontlineAchievementsV76::CommitArchiveDeletionV83(Battle,[&](){return Commit(R,MoveTemp(Candidate));},HonourNotice))
    {if(!HonourNotice.IsEmpty())R.Notice=HonourNotice;return false;}
    if(Removed){Deactivate(Battle);Battle->RestartBattle();}return true;
}
int32 ResearchPoints(const AWarfareFrontlineGameMode* Battle){if(!Battle)return 0;auto& R=RuntimeFor(Battle);const auto* E=Current(R);return E?E->Campaign.progression.researchPoints:0;}
bool OwnsOption(const AWarfareFrontlineGameMode* Battle,const FString& StableId)
{if(!Battle)return false;auto& R=RuntimeFor(Battle);const auto* E=Current(R);if(!E)return false;const auto Id=wfexpue::Narrow(StableId);return std::find(E->Campaign.purchasedOptions.begin(),E->Campaign.purchasedOptions.end(),Id)!=E->Campaign.purchasedOptions.end();}
bool PurchaseOption(AWarfareFrontlineGameMode* Battle,const FString& StableId)
{
    if(!Battle||!Active(Battle)||!WorldReady(Battle))return false;auto& R=RuntimeFor(Battle);if(!Editable(Battle,R))return false;FRepository Candidate=R.Repository;
    auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==R.Current;});if(!E)return false;
    const auto Result=wfexp::tactical::PurchaseOption(E->Campaign,wfexpue::Narrow(StableId));if(!Result){R.Notice=wfexpue::Wide(Result.reason);return false;}
    E->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(R,MoveTemp(Candidate));
}
TArray<FFRRUTacticalOptionV82> Options(const AWarfareFrontlineGameMode* Battle)
{
    TArray<FFRRUTacticalOptionV82> Out;if(!Battle)return Out;auto& R=RuntimeFor(Battle);const auto* E=Current(R);if(!E)return Out;
    for(const auto& Native:wfexp::tactical::Options())if(Native.faction==E->Campaign.faction)
    {
        FFRRUTacticalOptionV82 Row;Row.Id=wfexpue::Wide(Native.id);
        Row.Title=WarfareFRRUTextV82::Resolve(Row.Id,TEXT("title"),FString(),wfexpue::Wide(Native.label));
        Row.Description=WarfareFRRUTextV82::Resolve(Row.Id,TEXT("description"),FString(),wfexpue::Wide(Native.description));
        Row.Tradeoff=WarfareFRRUTextV82::Resolve(Row.Id,TEXT("tradeoff"));
        Row.Cost=Native.cost;Row.Index=Native.index;Row.Owned=OwnsOption(Battle,Row.Id);
        auto Probe=E->Campaign;const auto Allowed=wfexp::tactical::PurchaseOption(Probe,Native.id);
        if(!Allowed&&!Row.Owned)Row.BlockReason=wfexpue::Wide(Allowed.reason);
        if(!WorldReady(Battle))Row.BlockReason=WorldBlockReason(Battle);
        Out.Add(MoveTemp(Row));
    }
    return Out;
}
TArray<FString> ArmourSelection(const AWarfareFrontlineGameMode* B)
{if(!B)return {};const auto* E=Current(RuntimeFor(B));return E?E->ArmourSelection:TArray<FString>();}
bool SetArmourSelection(AWarfareFrontlineGameMode* B,const TArray<FString>& Ids)
{
    if(!B||!Active(B)||B->Phase!=EFrontlinePhase::Briefing||Ids.Num()>2)return false;auto& R=RuntimeFor(B);
    if(R.ReadOnly||R.PendingOutcome.IsSet())return false;TSet<FString> Unique;
    for(const auto& Id:Ids)if(Unique.Contains(Id)||!WarfareFRRUArmourV82::Eligible(B,Id,0))return false;else Unique.Add(Id);
    FRepository Candidate=R.Repository;auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==R.Current;});if(!E)return false;E->ArmourSelection=Ids;E->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(R,MoveTemp(Candidate));
}

namespace
{
int32 SelectedRequisitionV87(const FEntry& Entry)
{
    const auto* Order=Entry.RequisitionsV87.Find(wfexpue::Wide(Entry.Campaign.progression.selectedMissionId));
    return Order?Order->Kind:INDEX_NONE;
}
FString RequisitionDependencyBlockV87(const AWarfareFrontlineGameMode* B,int32 Kind)
{
    if(Kind==0&&!WarfareFRRUEquipmentV82::RoleAvailable(B,EFrontlineRole::Pioneer,0))
        return Local("Pioneers are not assigned to this operation.","本次行动未配属工兵。");
    if(Kind==1&&(!WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::Smoke,0)
        ||!FrontlineCampaignV32::SupportSelected(B,EFrontlineSupport::Smoke,0)))
        return Local("Assign smoke support before selecting its reserve.","请先接通烟幕支援，再选择烟幕储备。");
    if(Kind==2)
    {
        bool Any=false;for(const auto& Id:ArmourSelection(B))
            if(WarfareFRRUArmourV82::Eligible(B,Id,0)&&WarfareFRRUArmourV82::Accepted(B,Id)){Any=true;break;}
        if(!Any)return Local("Assign an available vehicle before selecting the armoured reserve.","请先配属一种可用战车，再选择装甲待命。");
    }
    return FString();
}
}
int32 RequisitionV87(const AWarfareFrontlineGameMode* B)
{if(!B||!Active(B))return INDEX_NONE;const auto* Entry=Current(RuntimeFor(B));return Entry?SelectedRequisitionV87(*Entry):INDEX_NONE;}
bool CanRequisitionV87(const AWarfareFrontlineGameMode* B,int32 Kind)
{
    if(!B||!Active(B)||Kind<0||Kind>2||B->Phase!=EFrontlinePhase::Briefing||B->IsProgressionReadOnly())return false;
    auto& R=RuntimeFor(B);const auto* Entry=Current(R);
    if(!Entry||!Editable(B,R)||R.BoundEncounter.IsValid()||!Entry->Campaign.progression.activeEncounterId.empty())return false;
    const auto* Mission=wfexp::FindMission(Entry->Campaign.progression.selectedMissionId);
    if(!Mission||Mission->faction!=Entry->Campaign.faction)return false;
    const bool Archive=ArchiveFullArmoryV87(B,0);const int32 CurrentKind=SelectedRequisitionV87(*Entry);
    // Cancellation remains available if the player subsequently removes the
    // selected vehicle or smoke card. Only a genuinely paid order refunds.
    if(CurrentKind==Kind)return true;
    if(!Archive&&(Mission->order<8||CurrentKind!=INDEX_NONE
        ||(!FrontlineDebugV37::UnlimitedResearch(B)&&Entry->Campaign.progression.researchPoints<3)))return false;
    return RequisitionDependencyBlockV87(B,Kind).IsEmpty();
}
bool BuyRequisitionV87(AWarfareFrontlineGameMode* B,int32 Kind)
{
    if(!CanRequisitionV87(B,Kind))return false;auto& R=RuntimeFor(B);auto Candidate=R.Repository;
    auto* Entry=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==R.Current;});if(!Entry)return false;
    const FString Mission=wfexpue::Wide(Entry->Campaign.progression.selectedMissionId);
    const auto* Previous=Entry->RequisitionsV87.Find(Mission);const bool Cancel=Previous&&Previous->Kind==Kind;
    const bool Archive=ArchiveFullArmoryV87(B,0),Free=Archive||FrontlineDebugV37::UnlimitedResearch(B);
    if(Cancel)
    {
        if(!Archive&&Previous->Paid)
        {if(Entry->Campaign.progression.researchPoints>99997)return false;Entry->Campaign.progression.researchPoints+=3;}
        Entry->RequisitionsV87.Remove(Mission);
    }
    else
    {
        if(!Free)Entry->Campaign.progression.researchPoints-=3;
        Entry->RequisitionsV87.Add(Mission,{Kind,!Free});
    }
    Entry->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(R,MoveTemp(Candidate));
}
bool FrozenRequisitionV87(const AWarfareFrontlineGameMode* B,int32& Kind,float& SmokeReadyAt)
{
    Kind=INDEX_NONE;SmokeReadyAt=0;if(!B||!Active(B))return false;auto& R=RuntimeFor(B);const auto* Entry=Current(R);
    if(!Entry||!R.BoundEncounter.IsValid()||R.BoundEncounter!=B->GetEncounterId()||R.BoundCampaign!=Entry->Id||R.BoundMission.IsEmpty())return false;
    Kind=R.BoundRequisitionV87;SmokeReadyAt=R.BoundSmokeBaselineV87;return true;
}

uint32 RoleSelection(const AWarfareFrontlineGameMode* Battle){if(!Battle)return 0;const auto* E=Current(RuntimeFor(Battle));return E?E->RoleSelection:0;}
uint32 RequiredRoleMask(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle))return 0;
    const auto* Mission=wfexp::tactical::FindMission(wfexpue::Narrow(MissionId(Battle)));if(!Mission)return 0;
    uint32 Required=0;for(const auto& Stage:Mission->stages)
        if(Stage.requiredRole>=0&&Stage.requiredRole<int32(EFrontlineRole::Count))Required|=1u<<Stage.requiredRole;
    return Required;
}
FString DeploymentBlockReason(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle||!Active(Battle))return FString();
    const uint32 Required=RequiredRoleMask(Battle);
    if((FrontlineCampaignV32::GetLoadout(Battle).SelectedRoles&Required)!=Required)
        return Chinese()?TEXT("请先配属工兵分队；本次行动需要开辟通路。"):TEXT("Assign a pioneer section before deployment. This operation requires an opened passage.");
    return RequisitionDependencyBlockV87(Battle,RequisitionV87(Battle));
}
uint32 SupportSelection(const AWarfareFrontlineGameMode* Battle){if(!Battle)return 0;const auto* E=Current(RuntimeFor(Battle));return E?E->SupportSelection:0;}
bool SetSelection(AWarfareFrontlineGameMode* Battle,uint32 Roles,uint32 Supports)
{
    if(!Battle||!Active(Battle)||!Roles||!Supports||Roles>>11||Supports>>10||FMath::CountBits(Roles)>5||FMath::CountBits(Supports)>6)return false;auto& R=RuntimeFor(Battle);if(!Editable(Battle,R))return false;
    const auto* Mission=wfexp::FindMission(wfexpue::Narrow(MissionId(Battle)));if(!Mission)return false;
    uint32 AllowedRoles=Mission->roleMask,AllowedSupports=Mission->supportMask;
    if(ArchiveFullArmoryV87(Battle,0))
    {
        AllowedRoles=AllowedSupports=0;
        for(int32 Role=0;Role<11;++Role)if(wfexp::RoleFor(Mission->faction,Role,Mission->date))AllowedRoles|=1u<<Role;
        for(int32 Support=0;Support<10;++Support)if(wfexp::SupportFor(Mission->faction,Support,Mission->date))AllowedSupports|=1u<<Support;
    }
    if((Roles&~AllowedRoles)||(Supports&~AllowedSupports))return false;
    auto Candidate=R.Repository;auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==R.Current;});if(!E)return false;
    E->RoleSelection=Roles;E->SupportSelection=Supports;E->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(R,MoveTemp(Candidate));
}
bool WorldReady(const AWarfareFrontlineGameMode* Battle){return CampaignsAvailable(Nation(Battle))&&WarfareFRRUWorldV82::CanPrepare(Battle);}
FString WorldBlockReason(const AWarfareFrontlineGameMode* Battle)
{return !CampaignsAvailable(Nation(Battle))?Local("Not yet available.","暂未开放。"):Active(Battle)?WarfareFRRUWorldV82::BlockReason(Battle):FString();}

#if WITH_EDITOR
namespace
{
void RunPaidRequisitionChecksV87(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString User,Path;
    const bool Safe=B&&B->Phase==EFrontlinePhase::Briefing&&B->IsProgressionVolatile()
        &&FRRUParityReviewSafetyV87::UserDir(User)&&ReviewPath(Path)&&!ArchiveReplayActiveV87(B)
        &&!FrontlineUltimateArchiveV8215::Active(B)&&!FrontlineDebugV37::UnlimitedResearch(B);
    Check(TEXT("v87_paid_attachment_requires_exact_parity_disposable_repository"),Safe);if(!Safe)return;
    const TWeakObjectPtr<AWarfareFrontlineGameMode> Key(B);const auto Original=Sessions.FindChecked(Key);
    const bool Bound=Original->Path.Equals(Path,ESearchCase::IgnoreCase)&&!Original->Volatile&&!Original->ReadOnly
        &&!Original->BoundEncounter.IsValid()&&!Original->PendingOutcome.IsSet();
    Check(TEXT("v87_paid_attachment_original_is_unbound_private_disk_repository"),Bound);if(!Bound)return;
    const FString Files[]={Path,Path+TEXT(".bak")};bool Existed[2]={};TArray<uint8> Pins[2];
    for(int32 I=0;I<2;++I)
    {
        Existed[I]=IFileManager::Get().FileExists(*Files[I]);const int64 Size=Existed[I]?IFileManager::Get().FileSize(*Files[I]):0;
        const bool Pinned=!Existed[I]||(Size>=0&&Size<=MaximumRepositoryBytes&&FFileHelper::LoadFileToArray(Pins[I],*Files[I]));
        Check(FString::Printf(TEXT("v87_paid_attachment_private_before_bytes_pin_%d"),I),Pinned);if(!Pinned)return;
    }
    bool HostVolatile=false,HostReadOnly=false;FFrontlineNationalArchiveAccessV87::ReadFlags(B,HostVolatile,HostReadOnly);
    bool Restored=false;
    auto Restore=[&]()
    {
        bool Good=true;FRepositoryLock Lock(Path);
        if(!Lock.Valid())Good=false;
        else for(int32 I=0;I<2;++I)
        {
            if(Existed[I])
            {
                const FString Temp=Files[I]+TEXT(".paid-fixture-restore-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
                Good&=FFileHelper::SaveArrayToFile(Pins[I],*Temp)&&Replace(Files[I],Temp);
                IFileManager::Get().Delete(*Temp);
            }
            else Good&=!IFileManager::Get().FileExists(*Files[I])||IFileManager::Get().Delete(*Files[I],false,true);
        }
        Sessions.Add(Key,Original);FFrontlineNationalArchiveAccessV87::SetFlags(B,HostVolatile,HostReadOnly);
        for(int32 I=0;I<2;++I)
        {
            TArray<uint8> After;const bool Exists=IFileManager::Get().FileExists(*Files[I]);
            Good&=Exists==Existed[I]&&(!Exists||(FFileHelper::LoadFileToArray(After,*Files[I])&&After==Pins[I]));
        }
        Restored=true;return Good;
    };
    ON_SCOPE_EXIT{if(!Restored)Restore();};
    // This temporary normal (non-Archive) repository really uses Commit's disk
    // writer. Its late progression is synthetic, earns no honours, and is
    // removed by the pinned private-byte restoration above, never player I/O.
    auto Paid=MakeShared<FRuntime>();Paid->Loaded=true;Paid->Path=Path;Paid->Repository=Original->Repository;
    Sessions.Add(Key,Paid);
    auto Exercise=[&]()
    {
        for(const auto Nation:{wfexp::Faction::French,wfexp::Faction::Russian})
        {
            const FString Prefix=Nation==wfexp::Faction::French?TEXT("v87_paid_fr."):TEXT("v87_paid_ru.");
            const int32 Order=Nation==wfexp::Faction::French?8:9;const wfexp::Mission* Late=nullptr;
            for(const auto& Mission:wfexp::Missions())if(Mission.faction==Nation&&Mission.order==Order){Late=&Mission;break;}
            FEntry Fixture;Fixture.Id=FGuid::NewGuid();Fixture.Level=EFrontlineDifficulty::Veteran;Fixture.HonourEligible=false;
            Fixture.CreatedUtc=Fixture.UpdatedUtc=FDateTime::UtcNow().ToIso8601();
            bool Seeded=Late&&bool(wfexp::tactical::NewTacticalCampaign(Nation,Fixture.Campaign));
            if(Seeded)for(const auto& Mission:wfexp::Missions())if(Mission.faction==Nation&&Mission.order<Order)
            {
                const std::string Token=wfexpue::Narrow(FGuid::NewGuid().ToString(EGuidFormats::Digits));
                Seeded=bool(wfexp::BeginEncounter(Fixture.Campaign.progression,Mission.id,Token))
                    &&bool(wfexp::RecordVictory(Fixture.Campaign.progression,Mission.id,Token));if(!Seeded)break;
            }
            if(Seeded)Seeded=bool(wfexp::SelectMission(Fixture.Campaign.progression,Late->id));
            Fixture.Campaign.progression.researchPoints=3;auto& R=RuntimeFor(B);auto Candidate=R.Repository;
            Candidate.Entries.Add(Fixture);Candidate.Recent=Fixture.Id;
            Seeded=Seeded&&Commit(R,MoveTemp(Candidate));
            Check(Prefix+TEXT("synthetic_late_progression_only_real_private_disk_entry"),Seeded);if(!Seeded)return;
            R.Current=Fixture.Id;R.Selected=true;
            Check(Prefix+TEXT("ordinary_paid_three_points_uses_public_shared_purchase"),!ArchiveFullArmoryV87(B,0)
                &&FrontlineArmourV63::CanRequisition(B,0)&&FrontlineArmourV63::BuyRequisition(B,0)&&ResearchPoints(B)==0
                &&RequisitionV87(B)==0);if(ResearchPoints(B)!=0||RequisitionV87(B)!=0)return;
            Sessions.Remove(Key);
            const bool Reloaded=Continue(B,Fixture.Id);const auto* Cold=Current(RuntimeFor(B));
            const auto* PaidOrder=Cold?Cold->RequisitionsV87.Find(wfexpue::Wide(Late->id)):nullptr;
            Check(Prefix+TEXT("actual_cold_repository_reload_retains_paid_order_and_zero_balance"),Reloaded&&Cold&&!Cold->HonourEligible
                &&Cold->Campaign.profile==wfexp::tactical::Profile::TacticalV2&&Cold->Campaign.progression.technologyMask==0
                &&PaidOrder&&PaidOrder->Kind==0&&PaidOrder->Paid&&ResearchPoints(B)==0);if(!Reloaded||!PaidOrder||!PaidOrder->Paid)return;
            Check(Prefix+TEXT("public_second_click_refunds_exactly_three_paid_points"),FrontlineArmourV63::BuyRequisition(B,0)
                &&ResearchPoints(B)==3&&RequisitionV87(B)==INDEX_NONE);
            bool Cycles=true;for(int32 I=0;I<3;++I)
                Cycles&=FrontlineArmourV63::BuyRequisition(B,0)&&ResearchPoints(B)==0
                    &&FrontlineArmourV63::BuyRequisition(B,0)&&ResearchPoints(B)==3&&RequisitionV87(B)==INDEX_NONE;
            Check(Prefix+TEXT("repeat_public_buy_cancel_cannot_mint_points"),Cycles);
            Sessions.Remove(Key);const bool CancelReloaded=Continue(B,Fixture.Id);
            Check(Prefix+TEXT("actual_cold_reload_retains_refund_and_no_attachment"),CancelReloaded&&ResearchPoints(B)==3&&RequisitionV87(B)==INDEX_NONE);
            if(!CancelReloaded)return;
        }
    };
    Exercise();const bool SameBytes=Restore();
    Check(TEXT("v87_paid_attachment_restores_exact_private_bytes_and_original_runtime"),SameBytes&&Sessions.FindChecked(Key)==Original
        &&B->IsProgressionVolatile()==HostVolatile&&B->IsProgressionReadOnly()==HostReadOnly);
}
}
#endif
#include "FRRUArchiveReplayV87.inl"

#if WITH_EDITOR
namespace
{
bool RunCampaignReview(UWorld* World)
{
    FString Path;auto* Battle=World?World->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;
    if(!ReviewPath(Path)||!Battle||!Battle->HasActorBegunPlay()||Battle->Phase!=EFrontlinePhase::Briefing||!Battle->IsProgressionVolatile())
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_CAMPAIGN_REVIEW_REFUSED: exact isolated project, fresh absolute Saved/FRRU UserDir, review/no-save flags and real BeginPlay required"));return false;}
    if(IFileManager::Get().FileExists(*Path)||IFileManager::Get().FileExists(*(Path+TEXT(".bak"))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_CAMPAIGN_REVIEW_REFUSED: disposable repository must be fresh"));return false;}
    TArray<TSharedPtr<FJsonValue>> Results;int32 Passed=0;
    auto Check=[&](const FString& Name,bool Pass)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Results.Add(MakeShared<FJsonValueObject>(J));if(Pass)++Passed;else UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_CAMPAIGN_CHECK_FAILED %s"),*Name);};
    // Synthetic serialization fixtures only; these figures are not battle rewards.
    for(const auto& Vehicle:WarfareFRRUArmourV82::Definitions())
    {
        FFrontlineCombatRecordV65 Codec;Codec.Total={2,1};Codec.NationalArmourSourcesV82.Add(Vehicle.Id,Codec.Total);Codec.BattlesRecorded=1;
        FJsonObject Json;FrontlineCombatRecordV65::WriteArchive(Json,Codec);FFrontlineCombatRecordV65 Read;
        const bool Decoded=FrontlineCombatRecordV65::ReadArchive(Json,Read,false);
        const auto* Counts=Read.NationalArmourSourcesV82.Find(Vehicle.Id);
        Check(TEXT("synthetic_combat_codec.")+Vehicle.Id,Decoded&&Counts&&Counts->Infantry==2&&Counts->Armour==1&&Read.OtherSources.Total()==0&&Read.BattlesRecorded==1);
        auto Unknown=MakeShared<FJsonObject>();Unknown->SetNumberField(TEXT("infantry"),0);Unknown->SetNumberField(TEXT("armour"),0);
        Json.GetObjectField(TEXT("combat_record_v65"))->GetObjectField(TEXT("national_armour_sources_v82"))->SetObjectField(TEXT("unknown.vehicle"),Unknown);
        Check(TEXT("synthetic_unknown_combat_key_rejected.")+Vehicle.Id,!FrontlineCombatRecordV65::ReadArchive(Json,Read,false)&&Read.Total.Infantry==2&&Read.NationalArmourSourcesV82.Num()==1);
    }
    const auto OldRows=Battle->GetCampaignRuns();
    Check(TEXT("legacy_enum_values_and_two_sides_retained"),uint8(EFrontlineFaction::British)==0&&uint8(EFrontlineFaction::German)==1);
    Check(TEXT("initial_repository_empty"),Archives(Battle).IsEmpty()&&!Active(Battle));
    Check(TEXT("french_actual_create_and_atomic_file"),Start(Battle,EFRRUNationV81::French,EFrontlineDifficulty::Veteran)&&IFileManager::Get().FileExists(*Path));
    const FGuid FrenchId=CampaignId(Battle);Check(TEXT("french_guid_nation_date_mission_identity"),FrenchId.IsValid()&&Nation(Battle,0)==EFRRUNationV81::French&&Nation(Battle,1)==EFRRUNationV81::German&&CalendarDate(Battle)==19160306&&MissionId(Battle)==TEXT("fr.mort_homme"));
    Check(TEXT("france_is_not_a_legacy_british_campaign"),!Battle->GetCurrentCampaign()&&Difficulty(Battle)==EFrontlineDifficulty::Veteran&&Battle->GetCampaignDifficulty()==EFrontlineDifficulty::Veteran);
    Check(TEXT("classic_technology_closed_receiver_world_connected"),!Battle->HasTechnology(0)&&!Battle->PurchaseTechnology(0)&&Battle->WeaponReloadScale(0)==1.f&&WorldReady(Battle));
    for(const auto Faction:{wfexp::Faction::French,wfexp::Faction::Russian})
    {
        wfexp::tactical::Campaign Synthetic;const bool Created=bool(wfexp::tactical::NewTacticalCampaign(Faction,Synthetic));Check(Faction==wfexp::Faction::French?TEXT("synthetic_fr_tactical_fixture"):TEXT("synthetic_ru_tactical_fixture"),Created);
        int32 Count=0;
        for(const auto& M:wfexp::Missions())if(M.faction==Faction)
        {
            const FString Prefix=TEXT("descriptor_")+wfexpue::Wide(M.id);wfexpue::FBoundMission Owner(M,false);const auto& Host=Owner.Get().Mission;
            Check(Prefix+TEXT("_stable_id_date_and_unaccepted_armour"),Host.Id==(Faction==wfexp::Faction::French?1000:2000)+M.order&&Host.CalendarDateV41==M.date&&!Host.bArmourAllowedV41&&FString(Host.MapKey)==wfexpue::Wide(M.id));
            Check(Prefix+TEXT("_exclusive_tech_and_dates"),!wfexp::tactical::UsesClassicModifiers(Synthetic)&&Synthetic.progression.technologyMask==0&&wfexp::RoleFor(Faction,0,M.date)!=nullptr);
            const std::string Token=TCHAR_TO_UTF8(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
            const bool Begun=wfexp::CanStart(Synthetic.progression,M.id)&&bool(wfexp::BeginEncounter(Synthetic.progression,M.id,Token));
            const bool Won=Begun&&bool(wfexp::RecordVictory(Synthetic.progression,M.id,Token));const int32 Reward= Synthetic.progression.researchPoints;
            const bool DuplicateRejected=!wfexp::RecordVictory(Synthetic.progression,M.id,Token);
            Check(Prefix+TEXT("_synthetic_receipt_progression_once"),Won&&DuplicateRejected&&Synthetic.progression.researchPoints==Reward);++Count;
        }
        Check(Faction==wfexp::Faction::French?TEXT("ten_france_descriptors"):TEXT("ten_russia_descriptors"),Count==10);
    }
    Check(TEXT("austro_hungary_not_selectable"),!Start(Battle,EFRRUNationV81::AustroHungarian,EFrontlineDifficulty::Rookie)&&CampaignId(Battle)==FrenchId);
    Check(TEXT("unknown_and_future_mission_rejected_without_change"),!SelectMission(Battle,TEXT("unknown"))&&!SelectMission(Battle,TEXT("fr.vaux"))&&CampaignId(Battle)==FrenchId&&MissionId(Battle)==TEXT("fr.mort_homme"));
    Check(TEXT("russian_actual_create_and_ah_enemy"),Start(Battle,EFRRUNationV81::Russian,EFrontlineDifficulty::Survivor)&&Nation(Battle,0)==EFRRUNationV81::Russian&&Nation(Battle,1)==EFRRUNationV81::AustroHungarian&&CalendarDate(Battle)==19140829);
    const FGuid RussianId=CampaignId(Battle);Check(TEXT("two_independent_guids_and_difficulties"),RussianId.IsValid()&&RussianId!=FrenchId&&Archives(Battle).Num()==2&&Difficulty(Battle)==EFrontlineDifficulty::Survivor);
    // Synthetic option purchase exercises the actual repository writer while
    // the public user path remains closed until its world consumer is accepted.
    {
        auto& R=RuntimeFor(Battle);FRepository Candidate=R.Repository;auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==FrenchId;});
        const bool Bought=E&&bool(wfexp::tactical::PurchaseOption(E->Campaign,"fr.tech.0"));
        Check(TEXT("synthetic_option_actual_atomic_save"),Bought&&Commit(R,MoveTemp(Candidate)));
    }
    const TWeakObjectPtr<AWarfareFrontlineGameMode> Key(Battle);Sessions.Remove(Key);
    const auto* BasicOrder=wfexp::tactical::FindOption("fr.tech.0");
    Check(TEXT("fresh_actual_repository_load_and_resume_france"),BasicOrder&&Archives(Battle).Num()==2&&Continue(Battle,FrenchId)&&CampaignId(Battle)==FrenchId&&OwnsOption(Battle,TEXT("fr.tech.0"))&&ResearchPoints(Battle)==2-BasicOrder->cost);
    Check(TEXT("resume_does_not_leak_research_to_russia"),Continue(Battle,RussianId)&&!OwnsOption(Battle,TEXT("fr.tech.0"))&&ResearchPoints(Battle)==2);
    {
        auto& R=RuntimeFor(Battle);FRepository Candidate=R.Repository;auto* E=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==RussianId;});
        const std::string Interrupted=TCHAR_TO_UTF8(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
        const bool Begun=E&&bool(wfexp::BeginEncounter(E->Campaign.progression,E->Campaign.progression.selectedMissionId,Interrupted));
        const bool Saved=Begun&&Commit(R,MoveTemp(Candidate));Sessions.Remove(Key);
        const bool Restored=Saved&&Continue(Battle,RussianId);auto& Reloaded=RuntimeFor(Battle);const auto* Fresh=Current(Reloaded);
        Check(TEXT("interrupted_token_retired_without_reward_or_world_restore"),Restored&&Fresh&&Fresh->Campaign.progression.activeEncounterId.empty()&&Fresh->Campaign.progression.victories==0&&Fresh->Campaign.progression.defeats==0&&std::find(Fresh->Campaign.progression.resolvedEncounterIds.begin(),Fresh->Campaign.progression.resolvedEncounterIds.end(),Interrupted)!=Fresh->Campaign.progression.resolvedEncounterIds.end());
    }
    {
        FString Text;Encode(RuntimeFor(Battle).Repository,Text);TSharedPtr<FJsonObject> Source;
        const bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Source);
        if(Parsed)
        {
            Source->SetNumberField(TEXT("hostSchema"),1);
            const auto& Entries=Source->GetArrayField(TEXT("campaigns"));
            for(const auto& Value:Entries)Value->AsObject()->RemoveField(TEXT("armourSelection"));
            FString Legacy;FJsonSerializer::Serialize(Source.ToSharedRef(),TJsonWriterFactory<>::Create(&Legacy));FRepository Migrated;bool Unsupported=false;
            Check(TEXT("schema1_compatible_migration_preserves_guid_research_and_empty_armour"),Decode(Legacy,Migrated,Unsupported)&&!Unsupported&&Migrated.Entries.Num()==RuntimeFor(Battle).Repository.Entries.Num()
                &&Migrated.Entries[0].Id==RuntimeFor(Battle).Repository.Entries[0].Id&&Migrated.Entries[0].Campaign.purchasedOptions==RuntimeFor(Battle).Repository.Entries[0].Campaign.purchasedOptions&&Migrated.Entries[0].ArmourSelection.IsEmpty());
            auto Probe=RuntimeFor(Battle).Repository;auto* French=Probe.Entries.FindByPredicate([&](const FEntry& E){return E.Id==FrenchId;});
            French->ArmourSelection={TEXT("fr.schneider"),TEXT("fr.ft")};FString Pair;Encode(Probe,Pair);FRepository PairRead;
            Check(TEXT("two_stable_armour_ids_roundtrip_without_legacy_ordinals"),Decode(Pair,PairRead,Unsupported)&&PairRead.Entries.FindByPredicate([&](const FEntry& E){return E.Id==FrenchId;})->ArmourSelection==French->ArmourSelection);
            auto Negative=[&](const TArray<FString>& IDs){French->ArmourSelection=IDs;FString Invalid;Encode(Probe,Invalid);FRepository Output=PairRead;const auto Before=Output.Entries[0].Id;return !Decode(Invalid,Output,Unsupported)&&Output.Entries[0].Id==Before&&Output.Entries.Num()==PairRead.Entries.Num();};
            Check(TEXT("duplicate_armour_selection_is_rejected_without_partial_output"),Negative({TEXT("fr.ft"),TEXT("fr.ft")}));
            Check(TEXT("third_armour_slot_is_rejected_without_partial_output"),Negative({TEXT("fr.ft"),TEXT("fr.schneider"),TEXT("fr.saint_chamond")}));
            Check(TEXT("foreign_armour_selection_is_rejected_without_partial_output"),Negative({TEXT("ru.austin_series1")}));
        }
        else Check(TEXT("campaign_codec_fixture_json_parsed"),false);
    }
    FString GoodPrimary,GoodBackup;FFileHelper::LoadFileToString(GoodPrimary,*Path);FFileHelper::LoadFileToString(GoodBackup,*(Path+TEXT(".bak")));
    auto Restore=[&](){FFileHelper::SaveStringToFile(GoodPrimary,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FFileHelper::SaveStringToFile(GoodBackup,*(Path+TEXT(".bak")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);Sessions.Remove(Key);};
    for(const bool BackupFuture:{false,true})
    {
        Restore();const FString Future=TEXT("{\"hostSchema\":99,\"profile\":\"tactical_v2\"}");const FString Changed=BackupFuture?Path+TEXT(".bak"):Path;
        FFileHelper::SaveStringToFile(Future,*Changed,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);Sessions.Remove(Key);
        const bool Refused=!Start(Battle,EFRRUNationV81::French,EFrontlineDifficulty::Rookie);FString After;FFileHelper::LoadFileToString(After,*Changed);
        Check(BackupFuture?TEXT("future_backup_blocks_save_and_is_retained"):TEXT("future_primary_blocks_save_and_is_retained"),Refused&&After==Future&&RuntimeFor(Battle).ReadOnly);
    }
    Restore();FFileHelper::SaveStringToFile(TEXT("{malformed"),*Path);FFileHelper::SaveStringToFile(TEXT("{malformed-backup"),*(Path+TEXT(".bak")));Sessions.Remove(Key);
    const bool CorruptRefused=!Start(Battle,EFRRUNationV81::Russian,EFrontlineDifficulty::Rookie);FString BadPrimary,BadBackup;FFileHelper::LoadFileToString(BadPrimary,*Path);FFileHelper::LoadFileToString(BadBackup,*(Path+TEXT(".bak")));
    Check(TEXT("malformed_pair_retained_without_overwrite"),CorruptRefused&&BadPrimary==TEXT("{malformed")&&BadBackup==TEXT("{malformed-backup"));
    Restore();FFileHelper::SaveStringToFile(TEXT("{damaged-primary"),*Path);Sessions.Remove(Key);
    Check(TEXT("good_backup_recovery_and_atomic_resume"),Continue(Battle,FrenchId)&&CampaignId(Battle)==FrenchId&&OwnsOption(Battle,TEXT("fr.tech.0")));
    Restore();Continue(Battle,FrenchId);
    {
        auto& R=RuntimeFor(Battle);FRepository Disk=R.Repository;++Disk.Revision;FString External;Encode(Disk,External);FFileHelper::SaveStringToFile(External,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        const int32 Before=R.Repository.Entries.Num();const bool Refused=!Start(Battle,EFRRUNationV81::Russian,EFrontlineDifficulty::Veteran);FString After;FFileHelper::LoadFileToString(After,*Path);
        Check(TEXT("concurrent_repository_revision_rejected_without_partial_publish"),Refused&&R.Repository.Entries.Num()==Before&&After==External);
    }
    Restore();Continue(Battle,FrenchId);
    {
        // Synthetic transaction fixtures, never presented as earned victories.
        FRuntime Probe;Probe.Repository=RuntimeFor(Battle).Repository;Probe.Current=FrenchId;Probe.Selected=true;
        Probe.PendingCampaign=Probe.BoundCampaign=FrenchId;
        Probe.PendingEncounter=Probe.BoundEncounter=FGuid::NewGuid();
        auto* Base=Probe.Repository.Entries.FindByPredicate([&](const FEntry& E){return E.Id==FrenchId;});
        const auto Token=wfexpue::Narrow(Probe.PendingEncounter.ToString(EGuidFormats::DigitsWithHyphensLower));
        const std::string Mission=Base->Campaign.progression.selectedMissionId;
        const bool Began=bool(wfexp::BeginEncounter(Base->Campaign.progression,Mission,Token));
        FRepository Outcome=Probe.Repository;
        auto* Changed=Outcome.Entries.FindByPredicate([&](const FEntry& E){return E.Id==FrenchId;});
        const bool Resolved=Began&&bool(wfexp::RecordDefeat(Changed->Campaign.progression,Mission,Token));
        ++Changed->Kills;Probe.PendingOutcome=Outcome;
        FRepository Disk=Probe.Repository;++Disk.Revision;Disk.Recent=RussianId;
        auto* Other=Disk.Entries.FindByPredicate([&](const FEntry& E){return E.Id==RussianId;});Other->Kills+=17;
        FRepository Merged=Outcome;
        Check(TEXT("pending_result_merge_preserves_latest_other_campaign_and_recent_marker"),Resolved&&MergePendingOutcomeV826(Probe,Disk,Merged)
            &&Merged.Recent==RussianId&&Merged.Revision==Disk.Revision
            &&SameEntryV826(*Merged.Entries.FindByPredicate([&](const FEntry& E){return E.Id==RussianId;}),*Other)
            &&SameEntryV826(*Merged.Entries.FindByPredicate([&](const FEntry& E){return E.Id==FrenchId;}),*Changed));
        FRepository Conflict=Disk;++Conflict.Entries.FindByPredicate([&](const FEntry& E){return E.Id==FrenchId;})->Kills;
        Merged=Outcome;Check(TEXT("pending_result_same_campaign_change_remains_rejected"),!MergePendingOutcomeV826(Probe,Conflict,Merged));
        Conflict=Disk;Conflict.Entries.RemoveAll([&](const FEntry& E){return E.Id==FrenchId;});
        Merged=Outcome;Check(TEXT("pending_result_deleted_campaign_is_not_resurrected"),!MergePendingOutcomeV826(Probe,Conflict,Merged));
        Probe.PendingEncounter=FGuid::NewGuid();Merged=Outcome;
        Check(TEXT("pending_result_wrong_encounter_cannot_merge"),!MergePendingOutcomeV826(Probe,Disk,Merged));
        Probe.PendingOutcome.Reset();Merged=Outcome;
        Check(TEXT("ordinary_repository_transactions_cannot_use_outcome_merge"),!MergePendingOutcomeV826(Probe,Disk,Merged));
    }
    {
        auto& R=RuntimeFor(Battle);FRepository Candidate=R.Repository;
        Candidate.Recent=RussianId;
        const bool OtherWasRecent=Commit(R,MoveTemp(Candidate));
        Check(TEXT("implicit_national_return_keeps_active_army_when_recent_changes"),OtherWasRecent
            &&CampaignId(Battle)==FrenchId&&R.Repository.Recent==RussianId
            &&Battle->ContinueCampaign()&&CampaignId(Battle)==FrenchId);
    }
    Check(TEXT("unknown_delete_batch_has_no_partial_effect"),!Delete(Battle,{FrenchId,FGuid::NewGuid()})&&Archives(Battle).Num()==2&&CampaignId(Battle)==FrenchId);
    Check(TEXT("british_original_start_deactivates_only_expansion_session"),Battle->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Rookie)&&!Active(Battle)&&Battle->GetCurrentCampaign()&&Archives(Battle).Num()==2);
    Check(TEXT("german_original_start_retains_enum_and_path"),Battle->StartNewCampaign(EFrontlineFaction::German,EFrontlineDifficulty::Veteran)&&!Active(Battle)&&Battle->GetPlayerFaction()==EFrontlineFaction::German&&Battle->GetCurrentCampaign()&&Archives(Battle).Num()==2);
    Check(TEXT("original_records_still_present"),Battle->GetCampaignRuns().Num()==OldRows.Num()+2);
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("revision"),TEXT("v82.frru.campaign.1"));
    Report->SetStringField(TEXT("scope"),TEXT("Real BeginPlay campaign APIs and disposable FRRU repository I/O; twenty descriptors and native progression are synthetic fixtures. No actor orders, geometry, art, battle completion, performance or Shipping acceptance."));
    Report->SetStringField(TEXT("repository"),Path);Report->SetBoolField(TEXT("worldReady"),false);Report->SetBoolField(TEXT("historicalSavesAccepted"),false);
    Report->SetArrayField(TEXT("results"),Results);Report->SetNumberField(TEXT("checks"),Results.Num());Report->SetNumberField(TEXT("passed"),Passed);Report->SetBoolField(TEXT("allPassed"),Passed==Results.Num());
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));const FString Receipt=FPaths::Combine(FPaths::GetPath(Path),TEXT("campaign-review.json"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Receipt),true);
    const bool Written=FFileHelper::SaveStringToFile(Json,*Receipt,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_CAMPAIGN_REVIEW %d/%d receipt=%s"),Passed,Results.Num(),*Receipt);
    return Written&&Passed==Results.Num();
}
FAutoConsoleCommand CampaignReviewCommand(TEXT("WF.FRRU.ReviewCampaignV82"),TEXT("Disposable isolated FRRU campaign repository review."),FConsoleCommandDelegate::CreateLambda([]()
{
    FString Path;if(!ReviewPath(Path)){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_CAMPAIGN_REVIEW_REFUSED: isolation flags/path invalid"));return;}
    static bool Requested=false;if(Requested)return;Requested=true;const double Started=FPlatformTime::Seconds();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Started](float)
    {
        if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World();World&&World->WorldType==EWorldType::Game)
            if(auto* Battle=World->GetAuthGameMode<AWarfareFrontlineGameMode>();Battle&&Battle->HasActorBegunPlay())
            {const bool Passed=RunCampaignReview(World);FPlatformMisc::RequestExitWithStatus(false,Passed?0:3);return false;}
        if(FPlatformTime::Seconds()-Started>45){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_CAMPAIGN_REVIEW_FAILED: no real BeginPlay GameMode within 45 seconds"));FPlatformMisc::RequestExitWithStatus(false,4);return false;}
        return true;
    }));
}));
}
#endif
}

namespace WarfareFRRUCampaignInternalV82
{
using namespace WarfareFRRUCampaignV82;
bool CopyCampaign(const AWarfareFrontlineGameMode* Battle,wfexp::tactical::Campaign& Out)
{if(!Battle)return false;auto& R=RuntimeFor(Battle);const auto* E=Current(R);if(!E||R.ReadOnly||R.PendingOutcome.IsSet())return false;Out=E->Campaign;return true;}
bool OpenEncounter(AWarfareFrontlineGameMode* Battle,wfexp::tactical::State& Out,FString& Error)
{
    if(!WarfareFRRUCampaignV82::CampaignsAvailable(WarfareFRRUCampaignV82::Nation(Battle))){Error=WarfareFRRUCampaignV82::WorldBlockReason(Battle);return false;}
    if(!Battle||Battle->Phase!=EFrontlinePhase::Briefing||Battle->Elapsed!=0.f){Error=TEXT("Deployment context changed.");return false;}
    auto& R=RuntimeFor(Battle);const auto* E=Current(R);
    if(!E||R.ReadOnly||R.PendingOutcome.IsSet()||R.BoundEncounter.IsValid()){Error=TEXT("The campaign is not ready to deploy.");return false;}
    const int32 OpeningRequisition=SelectedRequisitionV87(*E);
    const auto* OpeningOperation=wfexp::FindMission(E->Campaign.progression.selectedMissionId);
    if(OpeningRequisition!=INDEX_NONE&&(!OpeningOperation||(!ArchiveFullArmoryV87(Battle,0)&&OpeningOperation->order<8)))
    {Error=TEXT("The attachment is not available for this operation.");return false;}
    Error=RequisitionDependencyBlockV87(Battle,OpeningRequisition);if(!Error.IsEmpty())return false;
    // World::Begin establishes the national initial clock immediately after
    // this transaction. Freeze that exact value, not ResetEncounter's legacy
    // delay, which would look like a first smoke call during initialization.
    const float OpeningSmokeBaseline=WarfareFRRUEquipmentV82::SupportCooldown(Battle,EFrontlineSupport::Smoke,0);
    wfexp::tactical::State NewState;const auto Result=wfexp::tactical::NewEncounter(E->Campaign,E->Campaign.progression.selectedMissionId,wfexpue::Narrow(Battle->GetEncounterId().ToString(EGuidFormats::DigitsWithHyphensLower)),0,Battle->Elapsed,NewState);
    if(!Result){Error=wfexpue::Wide(Result.reason);return false;}
    // Encounter-only capability, absent from every campaign/archive codec.
    // The native reducer still validates national availability and date.
    NewState.fullArmoryReplay=ArchiveReplayActiveV87(Battle);
    FRepository Candidate=R.Repository;auto* Updated=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==E->Id;});if(!Updated)return false;
    Updated->Campaign=NewState.campaign;Updated->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    const FGuid OpeningCampaign=E->Id;const FString OpeningMission=wfexpue::Wide(NewState.missionId);
    if(!Commit(R,MoveTemp(Candidate))){Error=R.Notice;return false;}
    R.CommittedOutcomeEncounterV826.Invalidate();R.BoundOutcomeFirstVictoryV87=false;R.BoundEncounter=Battle->GetEncounterId();R.BoundCampaign=OpeningCampaign;R.BoundMission=OpeningMission;
    R.BoundRequisitionV87=OpeningRequisition;R.BoundSmokeBaselineV87=OpeningSmokeBaseline;Out=MoveTemp(NewState);return true;
}
bool HasPendingOutcome(const AWarfareFrontlineGameMode* Battle){return Battle&&RuntimeFor(Battle).PendingOutcome.IsSet();}
bool OutcomeReceiptCommittedV826(const AWarfareFrontlineGameMode* Battle)
{
    if(!Battle)return false;auto& R=RuntimeFor(Battle);const auto* E=Current(R);
    if(!E||R.PendingOutcome.IsSet()||R.BoundCampaign!=E->Id||!R.BoundEncounter.IsValid()
        ||R.BoundEncounter!=Battle->GetEncounterId()||R.CommittedOutcomeEncounterV826!=R.BoundEncounter)return false;
    const auto Token=wfexpue::Narrow(R.BoundEncounter.ToString(EGuidFormats::DigitsWithHyphensLower));
    return std::find(E->Campaign.progression.resolvedEncounterIds.begin(),E->Campaign.progression.resolvedEncounterIds.end(),Token)!=E->Campaign.progression.resolvedEncounterIds.end();
}
bool RetryOutcome(AWarfareFrontlineGameMode* Battle)
{
    if(!Battle)return false;auto& R=RuntimeFor(Battle);if(!R.PendingOutcome.IsSet())return true;
    if(R.Current!=R.PendingCampaign||R.BoundEncounter!=R.PendingEncounter||Battle->GetEncounterId()!=R.PendingEncounter)return false;
    FRepository Candidate=R.PendingOutcome.GetValue();if(!Commit(R,MoveTemp(Candidate),true))return false;
    R.CommittedOutcomeEncounterV826=R.PendingEncounter;
    R.PendingOutcome.Reset();R.PendingCampaign.Invalidate();R.PendingEncounter.Invalidate();return true;
}
bool SubmitOutcome(AWarfareFrontlineGameMode* Battle,const wfexp::tactical::State& Resolved,int32 InfantryKills,int32 VehicleKills,FString& Error)
{
    if(!Battle||!Resolved.objective.finished||!wfexp::tactical::ValidateState(Resolved)||InfantryKills<0||VehicleKills<0){Error=TEXT("The outcome has no validated completion receipt.");return false;}
    auto& R=RuntimeFor(Battle);const auto* E=Current(R);
    if(!E||R.ReadOnly||R.BoundCampaign!=E->Id||R.BoundEncounter!=Battle->GetEncounterId()||R.BoundMission!=wfexpue::Wide(Resolved.missionId)
        ||Resolved.encounterId!=wfexpue::Narrow(R.BoundEncounter.ToString(EGuidFormats::DigitsWithHyphensLower))){Error=TEXT("The outcome belongs to another deployment.");return false;}
    if(R.PendingOutcome.IsSet())return RetryOutcome(Battle);
    // An already committed outcome is idempotent; it cannot grant kills twice.
    const auto Token=wfexpue::Narrow(R.BoundEncounter.ToString(EGuidFormats::DigitsWithHyphensLower));
    if(std::find(E->Campaign.progression.resolvedEncounterIds.begin(),E->Campaign.progression.resolvedEncounterIds.end(),Token)!=E->Campaign.progression.resolvedEncounterIds.end())
    {R.CommittedOutcomeEncounterV826=R.BoundEncounter;return true;}
    if(E->Campaign.progression.activeEncounterId!=Token||E->Campaign.progression.activeEncounterMissionId!=Resolved.missionId
        ||Resolved.campaign.faction!=E->Campaign.faction||Resolved.campaign.purchasedOptions!=E->Campaign.purchasedOptions
        ||int64(E->Kills)+InfantryKills>100000000||int64(E->ArmourKills)+VehicleKills>10000000){Error=TEXT("The campaign changed before its outcome was saved.");return false;}
    auto Expected=E->Campaign;
    const auto ExpectedResult=Resolved.objective.success?wfexp::RecordVictory(Expected.progression,Resolved.missionId,Resolved.encounterId):wfexp::RecordDefeat(Expected.progression,Resolved.missionId,Resolved.encounterId);
    TSharedPtr<FJsonObject> ExpectedJson,ResolvedJson;
    if(!ExpectedResult||!wfexpue::WriteTacticalCampaign(Expected,ExpectedJson,Error)||!wfexpue::WriteTacticalCampaign(Resolved.campaign,ResolvedJson,Error)
        ||!FJsonValue::CompareEqual(FJsonValueObject(ExpectedJson),FJsonValueObject(ResolvedJson)))
    {Error=TEXT("The outcome changes more than this deployment's earned result.");return false;}
    FRepository Candidate=R.Repository;auto* Updated=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==E->Id;});if(!Updated)return false;
    Updated->Campaign=Resolved.campaign;Updated->Kills+=InfantryKills;Updated->ArmourKills+=VehicleKills;Updated->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    auto BattleRecord=Battle->CombatRecordV65[0];
    if(BattleRecord.Total.Infantry!=InfantryKills||BattleRecord.Total.Armour!=VehicleKills)
    {Error=TEXT("The casualty return does not match this deployment.");return false;}
    BattleRecord.BattlesRecorded=1;Updated->CombatRecord.Add(BattleRecord);
    // The shared codec stores the chronological mission index (0..99), not
    // the national host adapter's 1000/2000 presentation ID. Capture both
    // armies before the encounter is retired. The token guard above prevents
    // replay from accumulating the same return twice.
    const auto* Operation=wfexp::FindMission(Resolved.missionId);
    if(Operation&&!FrontlineReportV839::CaptureCampaignBattleV86(Updated->CombatRecord,Battle,Operation->order,Resolved.objective.success?0:1))
        UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_DETAILED_RETURN_MISSING mission=%s encounter=%s"),*R.BoundMission,*R.BoundEncounter.ToString());
    // Derive only from the still-unmodified repository, after the native result
    // and casualty return have both passed validation. Duplicate submission or
    // pending-save retries never recalculate this from the won candidate.
    R.BoundOutcomeFirstVictoryV87=Resolved.objective.success&&!ArchiveReplayActiveV87(Battle)
        &&std::find(E->Campaign.progression.wonMissionIds.begin(),E->Campaign.progression.wonMissionIds.end(),Resolved.missionId)
            ==E->Campaign.progression.wonMissionIds.end();
    R.PendingOutcome=MoveTemp(Candidate);R.PendingCampaign=E->Id;R.PendingEncounter=R.BoundEncounter;
    if(RetryOutcome(Battle))return true;Error=R.Notice;return false;
}
bool RetireEncounter(AWarfareFrontlineGameMode* Battle)
{
    if(!Battle)return false;auto& R=RuntimeFor(Battle);if(!R.Selected)return true;if(!RetryOutcome(Battle)||R.ReadOnly)return false;
    if(!WarfareFRRUCampaignV82::FinalizeCampaignCombatRecordV87(Battle))return false;
    if((Battle->Phase==EFrontlinePhase::Victory||Battle->Phase==EFrontlinePhase::Defeat)&&WarfareFRRUWorldV82::OutcomePersisted(Battle))
        FrontlineAchievementsV76::Finish(Battle,Battle->BattleOutroWinner);
    const auto* E=Current(R);if(!E||E->Campaign.progression.activeEncounterId.empty())return true;
    FRepository Candidate=R.Repository;auto* Updated=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==E->Id;});if(!Updated)return false;
    const auto Result=wfexp::AbortEncounter(Updated->Campaign.progression);if(!Result){R.Notice=wfexpue::Wide(Result.reason);return false;}
    Updated->UpdatedUtc=FDateTime::UtcNow().ToIso8601();return Commit(R,MoveTemp(Candidate));
}
void ClearBinding(AWarfareFrontlineGameMode* Battle)
{
    if(!Battle)return;auto& R=RuntimeFor(Battle);if(R.PendingOutcome.IsSet())return;R.CommittedOutcomeEncounterV826.Invalidate();R.BoundEncounter.Invalidate();R.BoundCampaign.Invalidate();R.BoundMission.Reset();R.BoundOutcomeFirstVictoryV87=false;R.BoundRequisitionV87=INDEX_NONE;R.BoundSmokeBaselineV87=0;
}
bool PrepareReviewMission(AWarfareFrontlineGameMode* Battle,const FString& StableId,FString& Error,EFrontlineDifficulty Level)
{
#if WITH_EDITOR
    FString DisposablePath;
    const auto* Mission=wfexp::FindMission(wfexpue::Narrow(StableId));
    if(!Battle||!Battle->HasActorBegunPlay()||!Battle->IsProgressionVolatile()||!ReviewPath(DisposablePath)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))||!Mission||!wfexp::IsSelectable(Mission->faction)||uint8(Level)>2)
    {Error=TEXT("Disposable WorldReview mission fixture refused.");return false;}
    if(Battle->Phase==EFrontlinePhase::Fighting)Battle->AbandonBattleToOperations();
    if(Battle->Phase!=EFrontlinePhase::Briefing||!RetireEncounter(Battle))return false;
    if(!Active(Battle)||Nation(Battle)!=PublicNation(Mission->faction)||Difficulty(Battle)!=Level)
        if(!Start(Battle,PublicNation(Mission->faction),Level))return false;
    auto& Runtime=RuntimeFor(Battle);FRepository Candidate=Runtime.Repository;
    auto* Entry=Candidate.Entries.FindByPredicate([&](const FEntry& Row){return Row.Id==Runtime.Current;});if(!Entry)return false;
    wfexp::tactical::Campaign Synthetic;if(!wfexp::tactical::NewTacticalCampaign(Mission->faction,Synthetic))return false;
    for(const auto& Prior:wfexp::Missions())if(Prior.faction==Mission->faction&&Prior.order<Mission->order)
    {
        const std::string Token=wfexpue::Narrow(FGuid::NewGuid().ToString(EGuidFormats::Digits));
        if(!wfexp::CanStart(Synthetic.progression,Prior.id)||!wfexp::BeginEncounter(Synthetic.progression,Prior.id,Token)||!wfexp::RecordVictory(Synthetic.progression,Prior.id,Token))return false;
    }
    if(!wfexp::SelectMission(Synthetic.progression,Mission->id))return false;
    Entry->Campaign=std::move(Synthetic);Entry->RoleSelection=Entry->SupportSelection=0;Entry->HonourEligible=false;
    Entry->UpdatedUtc=FDateTime::UtcNow().ToIso8601();if(!Commit(Runtime,MoveTemp(Candidate)))return false;
    ClearBinding(Battle);Battle->RestartBattle();return Battle->Phase==EFrontlinePhase::Briefing&&MissionId(Battle)==StableId;
#else
    Error=TEXT("WorldReview fixtures are Editor-only.");return false;
#endif
}
}
