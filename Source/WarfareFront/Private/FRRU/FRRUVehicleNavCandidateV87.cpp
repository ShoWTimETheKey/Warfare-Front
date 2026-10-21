#include "FRRUVehicleNavCandidateV87.h"
#if WITH_EDITOR
#include "WarfareFrontline.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
namespace FRRUVehicleNavCandidateV87
{
namespace
{
struct FReview { FProof Proof;FString Run;double LastWrite=-1; };
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FReview> Reviews;
FString Normal(FString Path){Path=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeDirectoryName(Path);return Path;}
bool PlainPath(const FString& Path,const FString& Root)
{
    if(!Path.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)&&!Path.Equals(Root,ESearchCase::IgnoreCase))return false;
#if PLATFORM_WINDOWS
    FString Part=Path;
    while(Part.Len()>=Root.Len())
    {
        const DWORD Attributes=GetFileAttributesW(*Part);
        if(Attributes!=INVALID_FILE_ATTRIBUTES&&(Attributes&FILE_ATTRIBUTE_REPARSE_POINT)!=0)return false;
        if(Attributes==INVALID_FILE_ATTRIBUTES&&GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)return false;
        if(Part.Equals(Root,ESearchCase::IgnoreCase))break;Part=FPaths::GetPath(Part);
    }
#endif
    return true;
}
bool Scope(AWarfareFrontlineGameMode* B,FString& Run,FString& Path,FString& Why)
{
    if(!IsInGameThread()||!B||!B->GetWorld()||B->GetWorld()->WorldType!=EWorldType::Game
        ||!FApp::IsGame()||!B->IsProgressionVolatile()||!WarfareFRRUCampaignV82::Active(B)
        ||WarfareFRRUCampaignV82::Nation(B)!=EFRRUNationV81::French||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("fr.matz"))
    {Why=TEXT("requires isolated actual French Matz game world");return false;}
    const TCHAR* Command=FCommandLine::Get();
    const bool Query=FParse::Param(Command,TEXT("FRRUFTNavCandidate")),Follow=FParse::Param(Command,TEXT("FRRUFTNavFollowCandidate"));
    if(Query==Follow||FParse::Param(Command,TEXT("WFFRRUV82VisualCaptureOnly"))!=Query
        ||(Follow&&FParse::Param(Command,TEXT("WFFRRUV82FTContactOnly"))))
    {Why=TEXT("one exclusive navigation mode required: visual-only query or normal FT follow without contact-only");return false;}
    for(const TCHAR* Flag:{TEXT("WFRenderNoSave"),TEXT("WFFRRUV82Review"),
        TEXT("WFFRRUV82WorldReview"),TEXT("WFFRRUV82NaturalReview")})
        if(!FParse::Param(Command,Flag)){Why=TEXT("requires explicit NoSave Matz navigation review flags");return false;}
    FString Case,User;
    if(!FParse::Value(Command,TEXT("FRRUNaturalCase="),Case)||Case!=TEXT("matz-ft")
        ||!FParse::Value(Command,TEXT("UserDir="),User)||User.IsEmpty()||FPaths::IsRelative(User)||User.Contains(TEXT("..")))
    {Why=TEXT("exact matz-ft case and absolute disposable UserDir required");return false;}
    // UE 5.8 Parse already includes every switch in Tokens as well as Switches.
    // Inspect Tokens once so only genuinely repeated command-line options fail.
    TArray<FString> Tokens,Switches;FCommandLine::Parse(Command,Tokens,Switches);TSet<FString> Seen;
    for(FString Token:Tokens)
    {
        if(Token.StartsWith(TEXT("-"))||Token.StartsWith(TEXT("/")))Token.RightChopInline(1);
        FString Name,Value;const bool HasValue=Token.Split(TEXT("="),&Name,&Value);if(!HasValue)Name=Token;
        if(Name.Equals(TEXT("UserDir"),ESearchCase::IgnoreCase)||Name.Equals(TEXT("FRRUNaturalCase"),ESearchCase::IgnoreCase)
            ||Name.Equals(TEXT("FRRUFTNavCandidate"),ESearchCase::IgnoreCase)||Name.Equals(TEXT("FRRUFTNavFollowCandidate"),ESearchCase::IgnoreCase))
        {
            const FString Key=Name.ToLower();if(Seen.Contains(Key)){Why=TEXT("duplicate navigation scope option");return false;}Seen.Add(Key);
            if((Name.Equals(TEXT("FRRUFTNavCandidate"),ESearchCase::IgnoreCase)||Name.Equals(TEXT("FRRUFTNavFollowCandidate"),ESearchCase::IgnoreCase))&&HasValue)
            {Why=TEXT("navigation candidate flag takes no value");return false;}
        }
    }
    const FString Root=Normal(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")));
    const FString Candidate=Normal(User),Prefix=Root+TEXT("/Saved/FRRU/V82NaturalReview/matz-ft/");
    if(!Normal(FPaths::ProjectDir()).Equals(Root,ESearchCase::IgnoreCase)||!Candidate.StartsWith(Prefix,ESearchCase::IgnoreCase))
    {Why=TEXT("exact integration project and matz-ft namespace required");return false;}
    TArray<FString> Parts;Candidate.Mid(Prefix.Len()).ParseIntoArray(Parts,TEXT("/"));FGuid Id;
    if(Parts.Num()!=2||Parts[1]!=TEXT("User")||!FGuid::ParseExact(Parts[0],EGuidFormats::Digits,Id)||!Id.IsValid()
        ||!Normal(FPaths::ProjectUserDir()).Equals(Candidate,ESearchCase::IgnoreCase)
        ||!Normal(FPaths::ProjectSavedDir()).Equals(Candidate+TEXT("/Saved"),ESearchCase::IgnoreCase))
    {Why=TEXT("nonzero GUID User/Saved agreement required");return false;}
    Run=Parts[0].ToLower();Path=Root/(Follow?TEXT("Saved/FRRU/V87FTNavFollowCandidate"):TEXT("Saved/FRRU/V87FTNavCandidate"))/Run/TEXT("proof.json");
    if(!PlainPath(Candidate,Root)||!PlainPath(Path,Root)||!IFileManager::Get().DirectoryExists(*Candidate))
    {Why=TEXT("reparse-free existing disposable User and output paths required");return false;}
    return true;
}

bool Write(AWarfareFrontlineGameMode* B,FReview& R,bool Release=false)
{
    auto J=FRRUVehicleNavV87::ReviewSnapshot(B,Release);if(!J)J=MakeShared<FJsonObject>();
    J->SetStringField(TEXT("run"),R.Run);J->SetStringField(TEXT("status"),R.Proof.Status==EStatus::Pending?TEXT("pending"):R.Proof.Status==EStatus::Success?TEXT("success"):TEXT("fail"));
    J->SetStringField(TEXT("reason"),R.Proof.Reason);J->SetBoolField(TEXT("shippingAccepted"),false);
    FString Text;FJsonSerializer::Serialize(J.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));
    if(Text.Len()>256*1024||!IFileManager::Get().MakeDirectory(*FPaths::GetPath(R.Proof.Path),true))return false;
    R.LastWrite=FPlatformTime::Seconds();return FFileHelper::SaveStringToFile(Text,*R.Proof.Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FProof Update(AWarfareFrontlineGameMode* B,FReview& R,const FRRUVehicleNavV87::FResult& Result)
{
    R.Proof.Status=Result.Status;R.Proof.Reason=Result.Reason;
    if(!Write(B,R)){R.Proof.Status=EStatus::Failed;R.Proof.Reason=TEXT("bounded navigation review evidence write failed");}
    return R.Proof;
}
}
bool Requested()
{
    TArray<FString> Tokens,Switches;FCommandLine::Parse(FCommandLine::Get(),Tokens,Switches);
    for(FString Token:Tokens)
    {
        if(Token.StartsWith(TEXT("-"))||Token.StartsWith(TEXT("/")))Token.RightChopInline(1);
        FString Name,Value;if(!Token.Split(TEXT("="),&Name,&Value))Name=Token;
        if(Name.Equals(TEXT("FRRUFTNavCandidate"),ESearchCase::IgnoreCase)||Name.Equals(TEXT("FRRUFTNavFollowCandidate"),ESearchCase::IgnoreCase))return true;
    }
    return false;
}
FProof Begin(AWarfareFrontlineGameMode* B,AActor* Scene)
{
    if(const auto* R=Reviews.Find(B))return R->Proof;
    FString Run,Path,Why;if(!Scope(B,Run,Path,Why))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_FT_NAV_BEGIN_REFUSED reason=%s"),*Why);return {EStatus::Failed,Why,{}};}
    if(IFileManager::Get().FileExists(*Path))return {EStatus::Failed,TEXT("existing navigation proof retained"),Path};
    FReview R;R.Run=Run;R.Proof.Path=Path;Reviews.Add(B,MoveTemp(R));
    return Update(B,Reviews.FindChecked(B),FRRUVehicleNavV87::BeginReview(B,Scene,FParse::Param(FCommandLine::Get(),TEXT("FRRUFTNavCandidate"))));
}
FProof PollProof(AWarfareFrontlineGameMode* B)
{
    auto* R=Reviews.Find(B);if(!R)return {EStatus::Failed,TEXT("navigation candidate not begun"),{}};
    FString Run,Path,Why;if(!Scope(B,Run,Path,Why)||Path!=R->Proof.Path)return Update(B,*R,{EStatus::Failed,TEXT("navigation review scope changed: ")+Why});
    if(R->Proof.Status!=EStatus::Pending)return R->Proof;
    const auto Result=FRRUVehicleNavV87::Poll(B);if(Result.Status!=EStatus::Pending||FPlatformTime::Seconds()-R->LastWrite>=1.)return Update(B,*R,Result);return R->Proof;
}
FGuidance FollowHeading(AWarfareFrontlineGameMode* B,AFrontlineTank* V){return FRRUVehicleNavV87::FollowHeading(B,V);}
void ObserveMovement(AWarfareFrontlineGameMode* B,AFrontlineTank* V,const FVector& Start,float Travel,bool Halt)
{
    FRRUVehicleNavV87::ObserveMovement(B,V,Start,Travel,Halt);
    if(auto* R=Reviews.Find(B);R&&FPlatformTime::Seconds()-R->LastWrite>=1.&&!Write(B,*R))
    {R->Proof.Status=EStatus::Failed;R->Proof.Reason=TEXT("bounded navigation movement evidence write failed");}
}
FProof FollowStatus(AWarfareFrontlineGameMode* B)
{
    auto* R=Reviews.Find(B);if(!R)return {EStatus::Failed,TEXT("follow navigation candidate not begun"),{}};
    if(R->Proof.Status==EStatus::Failed)return R->Proof;
    const auto Result=FRRUVehicleNavV87::FollowStatus(B);
    if(Result.Status!=EStatus::Pending||FPlatformTime::Seconds()-R->LastWrite>=1.)Write(B,*R);
    return {Result.Status,Result.Reason,R->Proof.Path};
}
void Clear(AWarfareFrontlineGameMode* B)
{
    if(auto* R=Reviews.Find(B)){Write(B,*R,true);Reviews.Remove(B);}else FRRUVehicleNavV87::Clear(B);
}
}
#else
namespace FRRUVehicleNavCandidateV87
{
bool Requested(){return false;}
FProof Begin(AWarfareFrontlineGameMode*,AActor*){return {EStatus::Failed,TEXT("Editor navigation review unavailable"),{}};}
FProof PollProof(AWarfareFrontlineGameMode*){return {EStatus::Failed,TEXT("Editor navigation review unavailable"),{}};}
FGuidance FollowHeading(AWarfareFrontlineGameMode*,AFrontlineTank*){return {};}
void ObserveMovement(AWarfareFrontlineGameMode*,AFrontlineTank*,const FVector&,float,bool){}
FProof FollowStatus(AWarfareFrontlineGameMode*){return {EStatus::Failed,TEXT("Editor navigation review unavailable"),{}};}
void Clear(AWarfareFrontlineGameMode*){}
}
#endif
