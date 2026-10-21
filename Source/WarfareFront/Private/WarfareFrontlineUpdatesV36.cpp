#include "WarfareFrontlineUpdatesV36.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"
#include "Misc/DateTime.h"

namespace FrontlineUpdatesV36
{
namespace
{
constexpr const TCHAR* Api=TEXT("https://api.github.com/repos/ShoWTimETheKey/Warfare-Front/releases/latest");
constexpr const TCHAR* Releases=TEXT("https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest");
constexpr const TCHAR* ReleasePrefix=TEXT("https://github.com/ShoWTimETheKey/Warfare-Front/releases/tag/");
FState State;
FString ReleaseUrl=Releases;
FHttpRequestPtr Pending;
double LastRequestAt=-1000;

// Compare numeric release components, never lexicographic tags (v9 vs v35).
// Both historic v33.1 and current integer tags are supported. Preview tags are rejected.
bool Version(const FString& Tag,TArray<int32>& Out)
{
    Out.Reset();if(Tag.Len()<2||Tag.Len()>32||Tag[0]!=TEXT('v'))return false;
    FString Digits=Tag.Mid(1);TArray<FString> Components;Digits.ParseIntoArray(Components,TEXT("."),false);
    if(Components.IsEmpty()||Components.Num()>4)return false;
    for(const FString& Part:Components)
    {
        if(Part.IsEmpty()||Part.Len()>6)return false;
        for(TCHAR C:Part)if(C<TEXT('0')||C>TEXT('9'))return false;
        Out.Add(FCString::Atoi(*Part));
    }
    return true;
}
bool Newer(const FString& Candidate,const FString& Local)
{
    TArray<int32> A,B;if(!Version(Candidate,A)||!Version(Local,B))return false;
    for(int32 I=0;I<FMath::Max(A.Num(),B.Num());++I)
    {const int32 X=A.IsValidIndex(I)?A[I]:0,Y=B.IsValidIndex(I)?B[I]:0;if(X!=Y)return X>Y;}
    return false;
}
bool Decode(const FString& Body,FString& Tag,FString& Url)
{
    if(Body.Len()>256*1024)return false;
    TSharedPtr<FJsonObject> J;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body),J)||!J)return false;
    bool Draft=true,Preview=true;
    if(!J->TryGetBoolField(TEXT("draft"),Draft)||!J->TryGetBoolField(TEXT("prerelease"),Preview)||Draft||Preview)return false;
    if(!J->TryGetStringField(TEXT("tag_name"),Tag)||!J->TryGetStringField(TEXT("html_url"),Url))return false;
    TArray<int32> Parts;
    // Never launch an arbitrary URL returned by a remote payload.
    return Version(Tag,Parts)&&Url==FString(ReleasePrefix)+Tag;
}
}
const TCHAR* CurrentTag(){return TEXT("v84.4.9");}
const FState& Get(){return State;}
void Check()
{
    if(State.Status==EStatus::Checking)return;
    const double Now=FPlatformTime::Seconds();
    if(Now-LastRequestAt<5.f)return; // UI retries do not hammer the public API.
    LastRequestAt=Now;State.Status=EStatus::Checking;
    auto Request=FHttpModule::Get().CreateRequest();Pending=Request;
    Request->SetURL(Api);Request->SetVerb(TEXT("GET"));Request->SetTimeout(12.f);
    Request->SetHeader(TEXT("Accept"),TEXT("application/vnd.github+json"));
    Request->SetHeader(TEXT("User-Agent"),FString(TEXT("WarfareFront/"))+CurrentTag());
    Request->SetHeader(TEXT("X-GitHub-Api-Version"),TEXT("2022-11-28"));
    Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request,FHttpResponsePtr Response,bool Connected)
    {
        if(Pending!=Request)return;
        Pending.Reset();State.CheckedUtc=FDateTime::UtcNow().ToIso8601();
        const int32 Code=Response?Response->GetResponseCode():0;
        if(Code==403||Code==429){State.Status=EStatus::RateLimited;UE_LOG(LogTemp,Display,TEXT("WF_UPDATE_CHECK rate_limited"));return;}
        FString Tag,Url;
        if(!Connected||!Response||Code!=200||!Decode(Response->GetContentAsString(),Tag,Url))
        {State.Status=EStatus::Offline;UE_LOG(LogTemp,Display,TEXT("WF_UPDATE_CHECK unavailable http=%d"),Code);return;}
        State.LatestTag=Tag;ReleaseUrl=Url;State.Status=Newer(Tag,CurrentTag())?EStatus::Available:EStatus::Current;
        UE_LOG(LogTemp,Display,TEXT("WF_UPDATE_CHECK current=%s latest=%s update_available=%d"),CurrentTag(),*Tag,State.Status==EStatus::Available);
    });
    if(!Request->ProcessRequest()){Pending.Reset();State.Status=EStatus::Offline;}
}
bool OpenReleasePage()
{
    FString Error;FPlatformProcess::LaunchURL(*ReleaseUrl,nullptr,&Error);
    if(!Error.IsEmpty())UE_LOG(LogTemp,Warning,TEXT("WF_UPDATE_CHECK could_not_open_browser"));
    return Error.IsEmpty();
}
void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    Check(TEXT("updates_numeric_version_order"),Newer(TEXT("v36"),TEXT("v9"))&&!Newer(TEXT("v35"),TEXT("v36")));
    Check(TEXT("updates_patch_versions"),Newer(TEXT("v36.1"),TEXT("v36"))&&!Newer(TEXT("v36.0"),TEXT("v36")));
    FString Tag,Url;
    const FString Body=TEXT("{\"draft\":false,\"prerelease\":false,\"tag_name\":\"v38\",\"html_url\":\"https://github.com/ShoWTimETheKey/Warfare-Front/releases/tag/v38\"}");
    Check(TEXT("updates_valid_release_decoded"),Decode(Body,Tag,Url)&&Tag==TEXT("v38"));
    Check(TEXT("updates_external_url_rejected"),!Decode(Body.Replace(TEXT("https://github.com/"),TEXT("https://example.org/")),Tag,Url));
    Check(TEXT("updates_prerelease_rejected"),!Decode(Body.Replace(TEXT("prerelease\":false"),TEXT("prerelease\":true")),Tag,Url));
    Check(TEXT("updates_malformed_response_rejected"),!Decode(TEXT("{}"),Tag,Url));
    TArray<int32> Parts;Check(TEXT("updates_nonrelease_tags_rejected"),!Version(TEXT("v36-beta"),Parts)&&!Version(TEXT("v36..1"),Parts)&&!Version(TEXT("v36/../settings"),Parts));
}
}


