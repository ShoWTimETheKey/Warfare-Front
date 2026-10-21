#pragma once
#include "Native/UE/ExpansionArchiveUE.h"
#include "Native/TacticalRuntime.h"
#include <charconv>

namespace wfexpue::clockv81 {
// This bounded codec persists only idempotency proof. It does not restore
// actors, ordnance, ammunition or a suspended battle; those remain unaccepted.
inline bool UInt64(const FJsonObject& object,const TCHAR* key,std::uint64_t& output){
    FString raw;if(!object.TryGetStringField(key,raw)||raw.Len()<1||raw.Len()>20)return false;
    const auto text=Narrow(raw);std::uint64_t candidate=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),candidate);
    if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||std::to_string(candidate)!=text)return false;output=candidate;return true;
}
inline bool Write(const wfexp::tactical::State& state,TSharedPtr<FJsonObject>& output,FString& error){
    const auto valid=wfexp::tactical::ValidateState(state);if(!valid){error=Wide(valid.reason);return false;}
    const auto& clock=state.latestClock;auto object=MakeShared<FJsonObject>();object->SetStringField(TEXT("format"),TEXT("v81.clock.2"));object->SetStringField(TEXT("missionId"),Wide(state.missionId));object->SetStringField(TEXT("encounterId"),Wide(state.encounterId));object->SetStringField(TEXT("receiptId"),Wide(clock.receipt.id));object->SetStringField(TEXT("payload"),Wide(clock.receipt.payload));object->SetStringField(TEXT("sequence"),Wide(std::to_string(clock.receipt.sequence)));object->SetStringField(TEXT("hostFrame"),Wide(std::to_string(clock.hostFrame)));object->SetNumberField(TEXT("side"),clock.receipt.side);object->SetNumberField(TEXT("capturedElapsed"),clock.capturedElapsed);output=object;error.Reset();return true;
}
inline bool Read(const FJsonObject& object,const wfexp::tactical::State& boundState,wfexp::tactical::ClockReceipt& output,FString& error){
    auto fail=[&](const TCHAR* reason){error=reason;return false;};FString format,mission,encounter,payload,id;double side=0,elapsed=0;
    if(!object.TryGetStringField(TEXT("format"),format)||format!=TEXT("v81.clock.2")||!object.TryGetStringField(TEXT("missionId"),mission)||mission!=Wide(boundState.missionId)||!object.TryGetStringField(TEXT("encounterId"),encounter)||encounter!=Wide(boundState.encounterId))return fail(TEXT("Clock checkpoint version or encounter is unsupported"));
    if(!object.TryGetStringField(TEXT("receiptId"),id)||id.Len()>128||!Clean(id)||!object.TryGetStringField(TEXT("payload"),payload)||payload.Len()>2048||!Clean(payload)||!Integer(object,TEXT("side"),-1,1,side)||!object.TryGetNumberField(TEXT("capturedElapsed"),elapsed))return fail(TEXT("Invalid clock checkpoint fields"));
    wfexp::tactical::ClockReceipt clock;clock.receipt.id=Narrow(id);clock.receipt.payload=Narrow(payload);clock.receipt.side=int(side);clock.capturedElapsed=elapsed;if(!UInt64(object,TEXT("sequence"),clock.receipt.sequence)||!UInt64(object,TEXT("hostFrame"),clock.hostFrame))return fail(TEXT("Invalid clock checkpoint counters"));
    auto candidate=boundState;candidate.latestClock=clock;const auto valid=wfexp::tactical::ValidateState(candidate);if(!valid){error=Wide(valid.reason);return false;}output=std::move(clock);error.Reset();return true;
}
} // namespace wfexpue::clockv81
