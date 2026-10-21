#include "WarfareFrontlineMotionV34.h"
#include "Animation/AnimationAsset.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"

UAnimationAsset* FrontlineMotionV34::Load(const FString& OriginalPath)
{
    if (!FParse::Param(FCommandLine::Get(),TEXT("WFMotionV34Baseline"))
        && !FParse::Param(FCommandLine::Get(),TEXT("WFV34Baseline"))
        && !FParse::Param(FCommandLine::Get(),TEXT("WFV29Baseline")))
    {
        FString Name=OriginalPath;int32 Dot=INDEX_NONE;
        if (Name.FindLastChar('.',Dot)) Name=Name.Mid(Dot+1);
        else { int32 Slash=INDEX_NONE;if (Name.FindLastChar('/',Slash)) Name=Name.Mid(Slash+1); }
        const int32 Version=Name.Find(TEXT("_V"),ESearchCase::CaseSensitive,ESearchDir::FromEnd);
        if (Name.StartsWith(TEXT("A_")) && Version!=INDEX_NONE)
        {
            Name=Name.Left(Version)+TEXT("_V34");
            const FString Path=FString::Printf(TEXT("/Game/Warfare/Characters/V34/Animations/%s.%s"),*Name,*Name);
            if (UAnimationAsset* Refined=LoadObject<UAnimationAsset>(nullptr,*Path)) return Refined;
        }
    }
    return LoadObject<UAnimationAsset>(nullptr,*OriginalPath);
}

UAnimationAsset* FrontlineMotionV34::Flame(const TCHAR* Phase)
{
    if (FParse::Param(FCommandLine::Get(),TEXT("WFMotionV34Baseline"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV34Baseline"))
        || FParse::Param(FCommandLine::Get(),TEXT("WFV29Baseline"))) return nullptr;
    static TMap<FString,TStrongObjectPtr<UAnimationAsset>> Actions;
    const FString Key(Phase);
    if (const auto* Found=Actions.Find(Key)) return Found->Get();
    const FString Name=FString::Printf(TEXT("A_Flame_%s_V34"),Phase);
    const FString Path=FString::Printf(TEXT("/Game/Warfare/Characters/V34/Animations/%s.%s"),*Name,*Name);
    UAnimationAsset* Action=LoadObject<UAnimationAsset>(nullptr,*Path);
    Actions.Add(Key,TStrongObjectPtr<UAnimationAsset>(Action));
    return Action;
}
