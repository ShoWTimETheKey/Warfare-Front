#pragma once
#include "WarfareFrontline.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

// Shipping relocates ProjectSavedDir beneath -UserDir. Review isolation must
// check the explicit disposable input rather than that relocated directory.
namespace FrontlineEffectsReviewSafetyV8216
{
inline bool Allowed(const AWarfareFrontlineGameMode* B)
{
    if(!B||!B->GetWorld()||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
    const bool V84=FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"));
    const bool V84412=FParse::Param(FCommandLine::Get(),TEXT("WFV84412FieldFocused"))&&FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"));
    const bool V8311=FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"));
    const bool V8310=FParse::Param(FCommandLine::Get(),TEXT("WFV8310Review"));
    const bool V833=FParse::Param(FCommandLine::Get(),TEXT("WFV833Review"));
    const bool V21=FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review"));
    const bool V20=FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"));
    const bool V19=FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"));
    const bool V18=FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review"));
    const bool V17=FParse::Param(FCommandLine::Get(),TEXT("WFV8217Review"));
    const bool V16=FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review"));
    const bool V15=FParse::Param(FCommandLine::Get(),TEXT("WFV8215Review"));
    if(!V84412&&!V84&&!V8311&&!V8310&&!V833&&!V21&&!V20&&!V19&&!V18&&!V17&&!V16&&!V15)return false;
    FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)||Dir.IsEmpty()||FPaths::IsRelative(Dir))return false;
    if(Dir.Contains(TEXT("..")))return false;
    FPaths::NormalizeDirectoryName(Dir);if(!FPaths::CollapseRelativeDirectories(Dir))return false;
    const FString Expected=V84412
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8449/Runtime"))
        :V84
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V84/Runtime"))
        :V8311
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8311/Runtime"))
        :V8310
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8310/Runtime"))
        :V833
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V833/Runtime/Run"))
        :V21
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8221/Runtime/Run"))
        :V20
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8220/Runtime/Run"))
        :V19
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8219/Runtime/Run"))
        :V18
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8218/ArchiveReview/Run"))
        :V17
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8217/ArchiveReview/Run"))
        :V16
        ?FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8216/ArchiveReview/Run"))
        :FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8215/ArchiveReview/Run"));
    if(Dir.Equals(Expected,ESearchCase::IgnoreCase))return true;
    const FString Prefix=Expected+TEXT("/");
    if(!Dir.StartsWith(Prefix,ESearchCase::IgnoreCase))return false;
    const FString Stamp=Dir.Mid(Prefix.Len());
    return Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
}
}
