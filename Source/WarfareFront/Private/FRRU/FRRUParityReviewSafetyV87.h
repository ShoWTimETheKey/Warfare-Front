#pragma once
#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "WarfareFrontlineReviewPathsV8221.h"

namespace FRRUParityReviewSafetyV87
{
inline bool UserDir(FString& Out)
{
    Out.Empty();
#if WITH_EDITOR
    for(const TCHAR* Flag:{TEXT("WFFRRUV87ParityReview"),TEXT("WFFRRUV82Review"),TEXT("WFFRRUV82WorldReview"),TEXT("WFV76Review"),TEXT("WFRenderNoSave")})
        if(!FParse::Param(FCommandLine::Get(),Flag))return false;
    FString Supplied;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Supplied)||Supplied.IsEmpty()||FPaths::IsRelative(Supplied)||Supplied.Contains(TEXT("..")))return false;
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    FString Expected=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"));FPaths::NormalizeDirectoryName(Expected);
    FString User=FPaths::ConvertRelativePathToFull(Supplied);FPaths::NormalizeDirectoryName(User);
    const FString Prefix=Expected+TEXT("/Saved/FRRU/V87ParityReview/Run/");
    if(!Project.Equals(Expected,ESearchCase::IgnoreCase)||!User.StartsWith(Prefix,ESearchCase::IgnoreCase))return false;
    const FString Stamp=User.Mid(Prefix.Len());
    if(Stamp.Len()!=19||Stamp[8]!=TEXT('-')||Stamp[15]!=TEXT('-')||!Stamp.Left(8).IsNumeric()||!Stamp.Mid(9,6).IsNumeric()||!Stamp.Right(3).IsNumeric())return false;
    Out=User;return true;
#else
    return false;
#endif
}
}
