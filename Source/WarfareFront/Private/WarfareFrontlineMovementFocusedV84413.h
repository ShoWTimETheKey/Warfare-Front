#pragma once
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
namespace FrontlineMovementFocusedV84413
{
inline bool Allowed(const AWarfareFrontlineGameMode* B)
{
    if(!B||!B->GetWorld()||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused")))return false;
    FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)||Dir.IsEmpty()
        ||FPaths::IsRelative(Dir)||Dir.Contains(TEXT("..")))return false;
    FPaths::NormalizeDirectoryName(Dir);
    const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84413/Runtime/"));
    const FString Stamp=Dir.StartsWith(Root,ESearchCase::IgnoreCase)?Dir.Mid(Root.Len()):FString();
    return Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')&&Stamp.Left(8).IsNumeric()
        &&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
}
}
