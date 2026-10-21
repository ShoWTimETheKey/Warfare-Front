#pragma once
#include "ExpansionArchiveUE.h"
#include "TacticalRuntime.h"
// Strategic progress only. Mid-encounter actors/strikes/movement are not restored
// by this codec. Host aborts the old encounter token on recovery before starting
// a new battle. Original classic save bytes/profile are never migrated silently.
namespace wfexpue {
inline bool ReadTacticalCampaign(const FJsonObject& object,wfexp::tactical::Campaign& output,FString& error){
    wfexp::tactical::Campaign candidate;double value=0;std::string profile,faction;
    auto fail=[&](const TCHAR* why){error=why;return false;};
    if(!Integer(object,TEXT("schemaVersion"),2,2,value)||!Integer(object,TEXT("rulesVersion"),2,2,value)||!String(object,TEXT("profile"),profile)||profile!="tactical_v2"||!String(object,TEXT("factionId"),faction))return fail(TEXT("Unsupported tactical schema/profile"));
    candidate.faction=wfexp::ParseFaction(faction);
    if(!Strings(object,TEXT("purchasedOptions"),24,candidate.purchasedOptions))return fail(TEXT("Invalid tactical options"));
    const TSharedPtr<FJsonObject>* progression=nullptr;
    if(!object.TryGetObjectField(TEXT("progression"),progression)||!progression||!progression->IsValid())return fail(TEXT("Missing tactical progression object"));
    if(!ReadArchive(**progression,candidate.progression,error))return false;
    auto result=wfexp::tactical::ValidateCampaign(candidate);
    if(!result){error=Wide(result.reason);return false;}
    output=std::move(candidate);error.Reset();return true;
}
inline bool WriteTacticalCampaign(const wfexp::tactical::Campaign& campaign,TSharedPtr<FJsonObject>& output,FString& error){
    auto result=wfexp::tactical::ValidateCampaign(campaign);if(!result){error=Wide(result.reason);return false;}
    TSharedPtr<FJsonObject> progression;if(!WriteArchive(campaign.progression,progression,error))return false;
    auto object=MakeShared<FJsonObject>();object->SetNumberField(TEXT("schemaVersion"),2);object->SetNumberField(TEXT("rulesVersion"),2);object->SetStringField(TEXT("profile"),TEXT("tactical_v2"));object->SetStringField(TEXT("factionId"),Wide(wfexp::FactionId(campaign.faction)));object->SetObjectField(TEXT("progression"),progression);
    TArray<TSharedPtr<FJsonValue>> options;for(const auto& id:campaign.purchasedOptions)options.Add(MakeShared<FJsonValueString>(Wide(id)));object->SetArrayField(TEXT("purchasedOptions"),options);
    output=object;error.Reset();return true;
}
} // namespace wfexpue
