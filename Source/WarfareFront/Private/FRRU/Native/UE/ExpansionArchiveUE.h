#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Expansion.h"

// Pure codec. The host owns transaction + backup + atomic file replacement.
// Expansion archives must never be stored in Progression-v13.json's armies[2].
namespace wfexpue {
inline FString Wide(const std::string& Value) { return FString(UTF8_TO_TCHAR(Value.c_str())); }
inline std::string Narrow(const FString& Value) { return std::string(TCHAR_TO_UTF8(*Value)); }
inline bool Clean(const FString& Value) {
    for (int32 I=0;I<Value.Len();++I) if (Value[I]<32 || Value[I]==127) return false;
    return true;
}
inline bool Integer(const FJsonObject& Object,const TCHAR* Key,double Min,double Max,double& Out) {
    double Value=0;
    if (!Object.TryGetNumberField(Key,Value) || !FMath::IsFinite(Value) || Value<Min || Value>Max || Value!=FMath::FloorToDouble(Value)) return false;
    Out=Value;return true;
}
inline bool String(const FJsonObject& Object,const TCHAR* Key,std::string& Out) {
    FString Value;
    if (!Object.TryGetStringField(Key,Value) || Value.Len()>128 || !Clean(Value)) return false;
    Out=Narrow(Value);return true;
}
inline bool Strings(const FJsonObject& Object,const TCHAR* Key,int32 Limit,std::vector<std::string>& Out) {
    const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
    if (!Object.TryGetArrayField(Key,Array) || !Array || Array->Num()>Limit) return false;
    for (const auto& Value:*Array) {
        FString Item;
        if (!Value.IsValid() || !Value->TryGetString(Item) || Item.Len()>128 || !Clean(Item)) return false;
        Out.push_back(Narrow(Item));
    }
    return true;
}
inline bool ReadArchive(const FJsonObject& Object,wfexp::Archive& Output,FString& Error) {
    // Decode to a candidate, validate all relationships, publish exactly once.
    wfexp::Archive Candidate;double Number=0;
    auto Fail=[&Error](const TCHAR* Why){Error=Why;return false;};
    if (!Integer(Object,TEXT("schemaVersion"),1,1,Number))return Fail(TEXT("Unsupported expansion archive schema"));Candidate.schemaVersion=int(Number);
    if (!Integer(Object,TEXT("contentRevision"),1,1,Number))return Fail(TEXT("Unsupported expansion content revision"));Candidate.contentRevision=int(Number);
    if (!String(Object,TEXT("packId"),Candidate.packId) || !String(Object,TEXT("factionId"),Candidate.factionId) || !String(Object,TEXT("selectedMissionId"),Candidate.selectedMissionId))return Fail(TEXT("Missing expansion identity"));
    if (!Integer(Object,TEXT("researchPoints"),0,100000,Number))return Fail(TEXT("Invalid research counter"));Candidate.researchPoints=int(Number);
    if (!Integer(Object,TEXT("victories"),0,100000,Number))return Fail(TEXT("Invalid victory counter"));Candidate.victories=int(Number);
    if (!Integer(Object,TEXT("defeats"),0,100000,Number))return Fail(TEXT("Invalid defeat counter"));Candidate.defeats=int(Number);
    if (!Integer(Object,TEXT("technologyMask"),0,4294967295.0,Number))return Fail(TEXT("Invalid technology mask"));Candidate.technologyMask=std::uint32_t(Number);
    if (!Strings(Object,TEXT("wonMissionIds"),32,Candidate.wonMissionIds) || !Strings(Object,TEXT("boughtTechnologyIds"),28,Candidate.boughtTechnologyIds) || !Strings(Object,TEXT("resolvedDispatchIds"),32,Candidate.resolvedDispatchIds) || !Strings(Object,TEXT("choiceIds"),32,Candidate.choiceIds) || !Strings(Object,TEXT("defeatedEncounterIds"),100000,Candidate.defeatedEncounterIds) || !Strings(Object,TEXT("resolvedEncounterIds"),200000,Candidate.resolvedEncounterIds) || !Strings(Object,TEXT("consumedLoanDispatchIds"),32,Candidate.consumedLoanDispatchIds))return Fail(TEXT("Invalid expansion ID lists"));
    if (!String(Object,TEXT("pendingLoanRoleId"),Candidate.pendingLoanRoleId) || !String(Object,TEXT("pendingLoanDispatchId"),Candidate.pendingLoanDispatchId) || !String(Object,TEXT("pendingLoanMissionId"),Candidate.pendingLoanMissionId))return Fail(TEXT("Invalid loan state"));
    if (!String(Object,TEXT("activeEncounterId"),Candidate.activeEncounterId) || !String(Object,TEXT("activeEncounterMissionId"),Candidate.activeEncounterMissionId))return Fail(TEXT("Invalid active encounter state"));
    const auto Result=wfexp::ValidateArchive(Candidate);
    if (!Result.ok){Error=Wide(Result.reason);return false;}
    Output=std::move(Candidate);Error.Reset();return true;
}
inline bool WriteArchive(const wfexp::Archive& Archive,TSharedPtr<FJsonObject>& Output,FString& Error) {
    const auto Result=wfexp::ValidateArchive(Archive);
    if (!Result.ok){Error=Wide(Result.reason);return false;}
    const auto Object=MakeShared<FJsonObject>();
    Object->SetNumberField(TEXT("schemaVersion"),Archive.schemaVersion);Object->SetNumberField(TEXT("contentRevision"),Archive.contentRevision);
    Object->SetStringField(TEXT("packId"),Wide(Archive.packId));Object->SetStringField(TEXT("factionId"),Wide(Archive.factionId));Object->SetStringField(TEXT("selectedMissionId"),Wide(Archive.selectedMissionId));
    Object->SetNumberField(TEXT("researchPoints"),Archive.researchPoints);Object->SetNumberField(TEXT("victories"),Archive.victories);Object->SetNumberField(TEXT("defeats"),Archive.defeats);Object->SetNumberField(TEXT("technologyMask"),double(Archive.technologyMask));
    auto Add=[&Object](const TCHAR* Key,const std::vector<std::string>& Values){TArray<TSharedPtr<FJsonValue>> Array;for(const auto& Value:Values)Array.Add(MakeShared<FJsonValueString>(Wide(Value)));Object->SetArrayField(Key,Array);};
    Add(TEXT("wonMissionIds"),Archive.wonMissionIds);Add(TEXT("boughtTechnologyIds"),Archive.boughtTechnologyIds);Add(TEXT("resolvedDispatchIds"),Archive.resolvedDispatchIds);Add(TEXT("choiceIds"),Archive.choiceIds);Add(TEXT("defeatedEncounterIds"),Archive.defeatedEncounterIds);Add(TEXT("resolvedEncounterIds"),Archive.resolvedEncounterIds);Add(TEXT("consumedLoanDispatchIds"),Archive.consumedLoanDispatchIds);
    Object->SetStringField(TEXT("pendingLoanRoleId"),Wide(Archive.pendingLoanRoleId));Object->SetStringField(TEXT("pendingLoanDispatchId"),Wide(Archive.pendingLoanDispatchId));Object->SetStringField(TEXT("pendingLoanMissionId"),Wide(Archive.pendingLoanMissionId));
    Object->SetStringField(TEXT("activeEncounterId"),Wide(Archive.activeEncounterId));Object->SetStringField(TEXT("activeEncounterMissionId"),Wide(Archive.activeEncounterMissionId));
    Output=Object;Error.Reset();return true;
}
}
