#include "FRRU/WarfareFRRUIntegration.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "FRRUCompatibilityChecks.h"
#include "FRRUClockArchiveUE.h"
#include "Native/UE/ExpansionHostBridge.h"
#include "Native/UE/ExpansionTacticalArchiveUE.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <type_traits>

static_assert(static_cast<uint8>(EFrontlineFaction::British)==0&&static_cast<uint8>(EFrontlineFaction::German)==1,"Existing save faction ordinals are immutable");
static_assert(!std::is_move_constructible_v<wfexpue::FBoundMission>&&!std::is_copy_constructible_v<wfexpue::FBoundMission>,"Raw mission text pointers require a stable owner");

namespace WarfareFRRUPilotV81 {
bool IsGameplayEnabled() {return wfexp::hostv81::Enabled;}
bool ResolveEncounter(const FString& MissionId,int32 PlayerSide,int32 PlayerDirection,FFRRUEncounterDescriptorV81& Output,FString& Error) {
    wfexp::hostv81::Identity native;const auto result=wfexp::hostv81::BindIdentity(TCHAR_TO_UTF8(*MissionId),PlayerSide,PlayerDirection,native);
    if(!result){Error=wfexpue::Wide(result.reason);return false;}
    FFRRUEncounterDescriptorV81 candidate;candidate.StableMissionId=wfexpue::Wide(native.missionId);candidate.SceneId=wfexpue::Wide(native.sceneId);candidate.SceneRevision=native.sceneRevision;candidate.CalendarDate=native.calendarDate;candidate.PlayerSide=native.playerSide;candidate.bEasternTheatre=native.theatre==wfexp::Theatre::Eastern;
    for(int32 Side=0;Side<2;++Side){candidate.Nations[Side]=static_cast<EFRRUNationV81>(native.nations[Side]);candidate.Directions[Side]=native.directions[Side];}
    Output=MoveTemp(candidate);Error.Reset();return true;
}
TSharedRef<FJsonObject> RunCompatibilityChecks() {
    auto checks=wfexp::hostv81::RunNativeCompatibilityChecks();
    for(wfexp::Faction nation:{wfexp::Faction::French,wfexp::Faction::Russian}){
        wfexp::tactical::Campaign campaign;checks.OK(wfexp::tactical::NewTacticalCampaign(nation,campaign),"UE fixture campaign");
        TSharedPtr<FJsonObject> encoded;FString error;const bool written=wfexpue::WriteTacticalCampaign(campaign,encoded,error);checks.Check(written&&encoded.IsValid(),"UE tactical codec encode");
        if(encoded.IsValid()){
            wfexp::tactical::Campaign decoded;checks.Check(wfexpue::ReadTacticalCampaign(*encoded,decoded,error),"UE tactical codec decode");checks.Check(decoded.faction==nation&&decoded.profile==wfexp::tactical::Profile::TacticalV2&&decoded.progression.packId=="wf.fr-ru"&&decoded.progression.selectedMissionId==campaign.progression.selectedMissionId,"UE codec stable identity/profile roundtrip");
            encoded->SetStringField(TEXT("profile"),TEXT("classic_v1"));const auto prior=decoded.progression.selectedMissionId;checks.Check(!wfexpue::ReadTacticalCampaign(*encoded,decoded,error)&&decoded.progression.selectedMissionId==prior,"UE codec profile refusal does not publish a partial archive");
            auto legacy=MakeShared<FJsonObject>();legacy->SetNumberField(TEXT("schemaVersion"),6);legacy->SetNumberField(TEXT("player_faction"),0);checks.Check(!wfexpue::ReadTacticalCampaign(*legacy,decoded,error)&&decoded.progression.selectedMissionId==prior,"Legacy UK/DE schema is never migrated into expansion profile");
        }
        const auto* row=wfexp::FindMission(campaign.progression.selectedMissionId);if(row){wfexpue::FBoundMission owner(*row,true);checks.Check(owner.Get().Mission.CalendarDateV41==row->date&&owner.Get().StableMissionId==wfexpue::Wide(row->id)&&FString(owner.Get().Mission.Title)==wfexpue::Wide(row->title.zh),"UE mission text pointers remain owned and date exact");}
    }
    for(const auto& row:wfexp::Missions())if(row.faction==wfexp::Faction::French&&row.armourIds[0]!='\0'){wfexpue::FBoundMission owner(row,true);checks.Check(!owner.Get().Mission.bArmourAllowedV41,"Late French mission conversion cannot open unverified armour deployment");}
    {
        wfexp::hostv81::PilotFixture fixture;if(fixture.Init("ru.galicia",0,checks)){
            wfexp::hostv81::HostFrameBridge bridge;wfexp::hostv81::TestAtomicHost host;bool advanced=false;fixture.Time(.5);checks.OK(bridge.Publish(fixture.state,fixture.snapshot,{9,.5,true,false},host,advanced),"UE codec prior frame");fixture.Time(1);checks.OK(bridge.Publish(fixture.state,fixture.snapshot,{10,1,true,false},host,advanced),"UE clock codec fixture commit");TSharedPtr<FJsonObject> encoded;FString error;checks.Check(wfexpue::clockv81::Write(fixture.state,encoded,error)&&encoded.IsValid(),"UE bounded clock checkpoint encode");
            if(encoded.IsValid()){
                wfexp::tactical::ClockReceipt restored;checks.Check(wfexpue::clockv81::Read(*encoded,fixture.state,restored,error)&&restored.hostFrame==10&&restored.capturedElapsed==1&&restored.receipt.payload==fixture.state.latestClock.receipt.payload,"UE clock checkpoint counters/proof roundtrip");
                const auto pristine=restored;auto RejectMutation=[&](const char* label){const bool rejected=!wfexpue::clockv81::Read(*encoded,fixture.state,restored,error);const bool untouched=restored.receipt.id==pristine.receipt.id&&restored.receipt.payload==pristine.receipt.payload&&restored.receipt.sequence==pristine.receipt.sequence&&restored.receipt.side==pristine.receipt.side&&restored.hostFrame==pristine.hostFrame&&restored.capturedElapsed==pristine.capturedElapsed;checks.Check(rejected&&untouched,label);};
                encoded->SetStringField(TEXT("hostFrame"),TEXT("11"));RejectMutation("UE altered frame rejected without partial publication");encoded->SetStringField(TEXT("hostFrame"),TEXT("10"));
                encoded->SetStringField(TEXT("sequence"),TEXT("1"));RejectMutation("UE in-range altered sequence rejected without partial publication");encoded->SetStringField(TEXT("sequence"),wfexpue::Wide(std::to_string(pristine.receipt.sequence)));
                encoded->SetStringField(TEXT("payload"),TEXT("x"));RejectMutation("UE nonempty false clock payload rejected without partial publication");encoded->SetStringField(TEXT("payload"),wfexpue::Wide(pristine.receipt.payload));
                encoded->SetNumberField(TEXT("capturedElapsed"),.75);RejectMutation("UE altered elapsed rejected without partial publication");encoded->SetNumberField(TEXT("capturedElapsed"),1);
                encoded->SetNumberField(TEXT("side"),1);RejectMutation("UE altered side rejected without partial publication");encoded->SetNumberField(TEXT("side"),0);
                encoded->SetStringField(TEXT("receiptId"),TEXT("changed.receipt"));RejectMutation("UE altered receipt ID rejected without partial publication");encoded->SetStringField(TEXT("receiptId"),wfexpue::Wide(pristine.receipt.id));
                encoded->SetStringField(TEXT("encounterId"),TEXT("another.encounter"));RejectMutation("UE foreign encounter checkpoint rejected without partial publication");encoded->SetStringField(TEXT("encounterId"),wfexpue::Wide(fixture.state.encounterId));
                encoded->SetStringField(TEXT("missionId"),TEXT("ru.carpathians"));RejectMutation("UE foreign mission checkpoint rejected without partial publication");encoded->SetStringField(TEXT("missionId"),wfexpue::Wide(fixture.state.missionId));
                checks.Check(wfexpue::clockv81::Read(*encoded,fixture.state,restored,error),"UE valid checkpoint remains readable after refused candidates");
            }
        }
    }
    auto report=MakeShared<FJsonObject>();report->SetStringField(TEXT("integrationRevision"),TEXT("v81.frru.pilot.3"));report->SetStringField(TEXT("scope"),TEXT("Pure descriptor, reducer and in-memory UE codecs; no world import, save repository, actor orders, Cook or performance acceptance"));report->SetBoolField(TEXT("gameplayEnabled"),IsGameplayEnabled());report->SetBoolField(TEXT("passed"),checks.Passed());report->SetNumberField(TEXT("checks"),checks.checks);
    TArray<TSharedPtr<FJsonValue>> failures;for(const auto& failure:checks.failures)failures.Add(MakeShared<FJsonValueString>(wfexpue::Wide(failure)));report->SetArrayField(TEXT("failures"),failures);return report;
}
} // namespace WarfareFRRUPilotV81

#if WITH_EDITOR
namespace {
// Explicit isolated fixture entry, never an automatic startup/gameplay action.
FAutoConsoleCommand FRRUVerifyCommand(
    TEXT("WF.FRRU.CheckCompatibility"),
    TEXT("Run gated expansion descriptor and in-memory codec compatibility fixtures."),
    FConsoleCommandDelegate::CreateLambda([] {
        auto Normalize=[](FString path){path=FPaths::ConvertRelativePathToFull(path);FPaths::NormalizeDirectoryName(path);FPaths::CollapseRelativeDirectories(path);return path;};
        const FString project=Normalize(FPaths::ProjectDir());
        const FString production=Normalize(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-ue5")));
        const FString sealedRoot=Normalize(FrontlineReviewPathsV8221::HomePath(TEXT("OneDrive/Dokumente/ChatGPT/Warfare Front")));
        const FString isolated=Normalize(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")));
        bool sealed=false;for(const TCHAR* folder:{TEXT("FR-RU-Expansion-r2"),TEXT("FR-RU-Expansion-r3")}){const FString path=Normalize(FPaths::Combine(sealedRoot,folder));sealed|=project.Equals(path,ESearchCase::IgnoreCase)||project.StartsWith(path+TEXT("/"),ESearchCase::IgnoreCase);}
        if(project.Equals(production,ESearchCase::IgnoreCase)||sealed||!project.Equals(isolated,ESearchCase::IgnoreCase)){
            UE_LOG(LogTemp,Error,TEXT("FRRU compatibility fixture refused outside the exact isolated validation project: %s"),*project);return;
        }
        const auto report=WarfareFRRUPilotV81::RunCompatibilityChecks();FString json;
        const auto writer=TJsonWriterFactory<TCHAR,TPrettyJsonPrintPolicy<TCHAR>>::Create(&json);
        const bool serialized=FJsonSerializer::Serialize(report,writer);
        const FString path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("FRRU/HostPilot/ue-compatibility.json"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(path),true);
        const bool saved=serialized&&FFileHelper::SaveStringToFile(json,*path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("FRRU isolated compatibility: passed=%s checks=%.0f receipt=%s saved=%s"),report->GetBoolField(TEXT("passed"))?TEXT("true"):TEXT("false"),report->GetNumberField(TEXT("checks")),*path,saved?TEXT("true"):TEXT("false"));
    }));
}
#endif // WITH_EDITOR
