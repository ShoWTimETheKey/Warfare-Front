#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineGameplayFocusedV84412.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUIAssetsV8213.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
namespace
{
struct FSession
{
 TArray<FFrontlineCampaignProgress> Runs;FGuid Active,Replay;
 EFrontlineFaction Faction=EFrontlineFaction::British;
 bool Volatile=false,ReadOnly=false,Assisted=false,Qualified=false;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FSession> Sessions;
}
EFrontlineRole FrontlineArchiveEnemyRoleV84412(const AWarfareFrontlineGameMode* B,int32 RecruitStep);
struct FFrontlineUltimateArchiveV8215Access
{
 static void AllocationChecksV84413(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
 static void ParityChecksV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
 static bool Begin(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 Mission,EFrontlineDifficulty Difficulty)
 {
  if(!B||B->Phase==EFrontlinePhase::Fighting||B->bProgressionReadOnly||WarfareFRRUCampaignV82::Active(B)||Sessions.Contains(B)
    ||!B->GetCampaignMissionsForFaction(F).IsValidIndex(Mission)||uint8(Difficulty)>2||!FrontlineUltimateArchiveV8215::Available(F))return false;
  // NoSave previews never acquire a writable real account merely by opening this room.
  FString ReviewDir;const bool Review=FrontlineUltimateArchiveV8215::ReviewUserDir(ReviewDir);
  if(B->bProgressionVolatile&&!Review)return false;
  FSession S;S.Runs=B->CampaignRuns;S.Active=B->ActiveCampaignId;S.Faction=B->PlayerFaction;S.Volatile=B->bProgressionVolatile;S.ReadOnly=B->bProgressionReadOnly;S.Replay=FGuid::NewGuid();S.Qualified=true;
  FFrontlineCampaignProgress R;R.Id=S.Replay;R.Faction=F;R.Difficulty=Difficulty;R.SelectedMission=Mission;R.CompletedMissions=B->GetCampaignMissionsForFaction(F).Num();
  R.WonOperationMaskV52=0;R.BypassedOperationMaskV52=0;R.StrategicRevisionV52=52;R.DispatchResolvedMaskV52=(1u<<20)-1u;R.DispatchOrderMaskV52=0;
  R.Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1u;R.ArmourResearchMaskV63=65535;R.bMobileGunV52=true;R.bAchievementEligibleV76=true;
  B->CampaignRuns.Add(R);B->ActiveCampaignId=R.Id;B->PlayerFaction=F;B->bProgressionVolatile=true;B->bProgressionReadOnly=false;
  Sessions.Add(B,MoveTemp(S));B->ResetEncounter();B->LastMessage=TEXT("终极档案室 · 行动命令已签发。");return true;
 }
 static void Leave(AWarfareFrontlineGameMode* B)
 {
  auto* S=Sessions.Find(B);if(!B||!S)return;const FSession Restore=*S;Sessions.Remove(B);
  B->CampaignRuns=Restore.Runs;B->ActiveCampaignId=Restore.Active;B->PlayerFaction=Restore.Faction;B->bProgressionVolatile=Restore.Volatile;B->bProgressionReadOnly=Restore.ReadOnly;
  B->ResetEncounter();B->LastMessage=TEXT("已返回战情室。");
 }
 static bool Technology(AWarfareFrontlineGameMode* B,int32 Id,bool Enabled,bool Armour)
 {
  const auto* S=B?Sessions.Find(B):nullptr;
  if(!S||B->Phase!=EFrontlinePhase::Briefing||Id<0||Id>=(Armour?16:int32(EFrontlineTechnology::Count)))return false;
  auto* R=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& Run){return Run.Id==S->Replay&&Run.Id==B->ActiveCampaignId;});
  if(!R)return false;uint32& Mask=Armour?R->ArmourResearchMaskV63:R->Research.TechnologyMask;
  if(Enabled)Mask|=1u<<Id;else Mask&=~(1u<<Id);
  return true; // In-memory replay only; no costs, fake research or file writes.
 }
};
#include "WarfareFrontlineArchiveParityV84412.inl"
namespace FrontlineUltimateArchiveV8215
{
bool ReviewRequested(){return FParse::Param(FCommandLine::Get(),TEXT("WFV8215ArchiveReview"));}
bool ReviewUserDir(FString& Out)
{
 Out.Empty();if(!ReviewRequested()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
 FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)||Dir.IsEmpty()||FPaths::IsRelative(Dir))return false;
 Dir=FPaths::ConvertRelativePathToFull(Dir);FPaths::NormalizeDirectoryName(Dir);
 // ProjectSavedDir can itself move under -UserDir in packaged builds. The
 // explicit review allowlist therefore remains anchored in the source tree.
 FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8215/ArchiveReview"));FPaths::NormalizeDirectoryName(Root);
 FString NewRoot=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8216/ArchiveReview/Run"));FPaths::NormalizeDirectoryName(NewRoot);
 const bool Legacy=Dir.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase);
 const bool Current=FParse::Param(FCommandLine::Get(),TEXT("WFV8216Review"))&&Dir.StartsWith(NewRoot+TEXT("/"),ESearchCase::IgnoreCase);
 const FString V17Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8217/ArchiveReview/Run/"));
 const FString Stamp=Dir.StartsWith(V17Root,ESearchCase::IgnoreCase)?Dir.Mid(V17Root.Len()):FString();
 const bool V17=FParse::Param(FCommandLine::Get(),TEXT("WFV8217Review"))&&Stamp.Len()==19
   &&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')&&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
 const FString V18Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8218/ArchiveReview/Run/"));
 const FString Stamp18=Dir.StartsWith(V18Root,ESearchCase::IgnoreCase)?Dir.Mid(V18Root.Len()):FString();
 const bool V18=FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review"))&&Stamp18.Len()==19
   &&Stamp18[8]==TEXT('-')&&Stamp18[15]==TEXT('-')&&Stamp18.Left(8).IsNumeric()&&Stamp18.Mid(9,6).IsNumeric()&&Stamp18.Right(3).IsNumeric();
 const FString V19Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8219/Runtime/Run/"));
 const FString Stamp19=Dir.StartsWith(V19Root,ESearchCase::IgnoreCase)?Dir.Mid(V19Root.Len()):FString();
 const bool V19=FParse::Param(FCommandLine::Get(),TEXT("WFV8219Review"))&&Stamp19.Len()==19
   &&Stamp19[8]==TEXT('-')&&Stamp19[15]==TEXT('-')&&Stamp19.Left(8).IsNumeric()&&Stamp19.Mid(9,6).IsNumeric()&&Stamp19.Right(3).IsNumeric();
 const FString V20Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8220/Runtime/Run/"));
 const FString Stamp20=Dir.StartsWith(V20Root,ESearchCase::IgnoreCase)?Dir.Mid(V20Root.Len()):FString();
 const bool V20=FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"))&&Stamp20.Len()==19
   &&Stamp20[8]==TEXT('-')&&Stamp20[15]==TEXT('-')&&Stamp20.Left(8).IsNumeric()&&Stamp20.Mid(9,6).IsNumeric()&&Stamp20.Right(3).IsNumeric();
 const FString V21Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8221/Runtime/Run/"));
 const FString Stamp21=Dir.StartsWith(V21Root,ESearchCase::IgnoreCase)?Dir.Mid(V21Root.Len()):FString();
 const bool V21=FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review"))&&Stamp21.Len()==19
   &&Stamp21[8]==TEXT('-')&&Stamp21[15]==TEXT('-')&&Stamp21.Left(8).IsNumeric()&&Stamp21.Mid(9,6).IsNumeric()&&Stamp21.Right(3).IsNumeric();
 const FString V833Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V833/Runtime/Run/"));
 const FString Stamp833=Dir.StartsWith(V833Root,ESearchCase::IgnoreCase)?Dir.Mid(V833Root.Len()):FString();
 const bool V833=FParse::Param(FCommandLine::Get(),TEXT("WFV833Review"))&&Stamp833.Len()==19
   &&Stamp833[8]==TEXT('-')&&Stamp833[15]==TEXT('-')&&Stamp833.Left(8).IsNumeric()&&Stamp833.Mid(9,6).IsNumeric()&&Stamp833.Right(3).IsNumeric();
 const FString V839Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V839/Runtime/"));
 const FString Stamp839=Dir.StartsWith(V839Root,ESearchCase::IgnoreCase)?Dir.Mid(V839Root.Len()):FString();
 const bool V839=FParse::Param(FCommandLine::Get(),TEXT("WFV839Review"))&&Stamp839.Len()==19
   &&Stamp839[8]==TEXT('-')&&Stamp839[15]==TEXT('-')&&Stamp839.Left(8).IsNumeric()&&Stamp839.Mid(9,6).IsNumeric()&&Stamp839.Right(3).IsNumeric();
 const FString V8310Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8310/Runtime/"));
 const FString Stamp8310=Dir.StartsWith(V8310Root,ESearchCase::IgnoreCase)?Dir.Mid(V8310Root.Len()):FString();
 const bool V8310=FParse::Param(FCommandLine::Get(),TEXT("WFV8310Review"))&&Stamp8310.Len()==19
   &&Stamp8310[8]==TEXT('-')&&Stamp8310[15]==TEXT('-')&&Stamp8310.Left(8).IsNumeric()&&Stamp8310.Mid(9,6).IsNumeric()&&Stamp8310.Right(3).IsNumeric();
 const FString V8311Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8311/Runtime/"));
 const FString Stamp8311=Dir.StartsWith(V8311Root,ESearchCase::IgnoreCase)?Dir.Mid(V8311Root.Len()):FString();
 const bool V8311=FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))&&Stamp8311.Len()==19
   &&Stamp8311[8]==TEXT('-')&&Stamp8311[15]==TEXT('-')&&Stamp8311.Left(8).IsNumeric()&&Stamp8311.Mid(9,6).IsNumeric()&&Stamp8311.Right(3).IsNumeric();
 const FString V84Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V84/Runtime/"));
 const FString Stamp84=Dir.StartsWith(V84Root,ESearchCase::IgnoreCase)?Dir.Mid(V84Root.Len()):FString();
 const bool V84=FParse::Param(FCommandLine::Get(),TEXT("WFV84Review"))&&Stamp84.Len()==19
   &&Stamp84[8]==TEXT('-')&&Stamp84[15]==TEXT('-')&&Stamp84.Left(8).IsNumeric()&&Stamp84.Mid(9,6).IsNumeric()&&Stamp84.Right(3).IsNumeric();
 const FString RibbonRoot8443=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8443/Runtime/"));
 const FString RibbonStamp8443=Dir.StartsWith(RibbonRoot8443,ESearchCase::IgnoreCase)?Dir.Mid(RibbonRoot8443.Len()):FString();
 const bool Ribbon8443=FParse::Param(FCommandLine::Get(),TEXT("WFV8443RibbonReview"))&&RibbonStamp8443.Len()==19
   &&RibbonStamp8443[8]==TEXT('-')&&RibbonStamp8443[15]==TEXT('-')&&RibbonStamp8443.Left(8).IsNumeric()
   &&RibbonStamp8443.Mid(9,6).IsNumeric()&&RibbonStamp8443.Right(3).IsNumeric();
 const FString V8444Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8444/Runtime/"));
 const FString Stamp8444=Dir.StartsWith(V8444Root,ESearchCase::IgnoreCase)?Dir.Mid(V8444Root.Len()):FString();
 const bool V8444=FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))&&Stamp8444.Len()==19
   &&Stamp8444[8]==TEXT('-')&&Stamp8444[15]==TEXT('-')&&Stamp8444.Left(8).IsNumeric()&&Stamp8444.Mid(9,6).IsNumeric()&&Stamp8444.Right(3).IsNumeric();
 const FString V8445Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8445/Runtime/"));
 const FString Stamp8445=Dir.StartsWith(V8445Root,ESearchCase::IgnoreCase)?Dir.Mid(V8445Root.Len()):FString();
 const bool V8445=FParse::Param(FCommandLine::Get(),TEXT("WFV8445Review"))&&Stamp8445.Len()==19
   &&Stamp8445[8]==TEXT('-')&&Stamp8445[15]==TEXT('-')&&Stamp8445.Left(8).IsNumeric()&&Stamp8445.Mid(9,6).IsNumeric()&&Stamp8445.Right(3).IsNumeric();
 const FString V8446Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8446/Runtime/"));
 const FString Stamp8446=Dir.StartsWith(V8446Root,ESearchCase::IgnoreCase)?Dir.Mid(V8446Root.Len()):FString();
 const bool V8446=FParse::Param(FCommandLine::Get(),TEXT("WFV8446Review"))&&Stamp8446.Len()==19
   &&Stamp8446[8]==TEXT('-')&&Stamp8446[15]==TEXT('-')&&Stamp8446.Left(8).IsNumeric()&&Stamp8446.Mid(9,6).IsNumeric()&&Stamp8446.Right(3).IsNumeric();
 const FString V8447Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8447/Runtime/"));
 const FString Stamp8447=Dir.StartsWith(V8447Root,ESearchCase::IgnoreCase)?Dir.Mid(V8447Root.Len()):FString();
 const bool V8447=FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))&&Stamp8447.Len()==19
   &&Stamp8447[8]==TEXT('-')&&Stamp8447[15]==TEXT('-')&&Stamp8447.Left(8).IsNumeric()&&Stamp8447.Mid(9,6).IsNumeric()&&Stamp8447.Right(3).IsNumeric();
 const FString V8448Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8448/Runtime/"));
 const FString Stamp8448=Dir.StartsWith(V8448Root,ESearchCase::IgnoreCase)?Dir.Mid(V8448Root.Len()):FString();
 const bool V8448=FParse::Param(FCommandLine::Get(),TEXT("WFV8448Review"))&&Stamp8448.Len()==19
   &&Stamp8448[8]==TEXT('-')&&Stamp8448[15]==TEXT('-')&&Stamp8448.Left(8).IsNumeric()&&Stamp8448.Mid(9,6).IsNumeric()&&Stamp8448.Right(3).IsNumeric();
 const FString V8449Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8449/Runtime/"));
 const FString Stamp8449=Dir.StartsWith(V8449Root,ESearchCase::IgnoreCase)?Dir.Mid(V8449Root.Len()):FString();
 const bool V8449=FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))&&Stamp8449.Len()==19
   &&Stamp8449[8]==TEXT('-')&&Stamp8449[15]==TEXT('-')&&Stamp8449.Left(8).IsNumeric()&&Stamp8449.Mid(9,6).IsNumeric()&&Stamp8449.Right(3).IsNumeric();
 const FString V8450Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8450/Runtime/"));
 const FString Stamp8450=Dir.StartsWith(V8450Root,ESearchCase::IgnoreCase)?Dir.Mid(V8450Root.Len()):FString();
 const bool V8450=FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review"))&&Stamp8450.Len()==19
   &&Stamp8450[8]==TEXT('-')&&Stamp8450[15]==TEXT('-')&&Stamp8450.Left(8).IsNumeric()&&Stamp8450.Mid(9,6).IsNumeric()&&Stamp8450.Right(3).IsNumeric();
 const FString V84411Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84411/Runtime/"));
 const FString Stamp84411=Dir.StartsWith(V84411Root,ESearchCase::IgnoreCase)?Dir.Mid(V84411Root.Len()):FString();
 const bool V84411=FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))&&Stamp84411.Len()==19
   &&Stamp84411[8]==TEXT('-')&&Stamp84411[15]==TEXT('-')&&Stamp84411.Left(8).IsNumeric()&&Stamp84411.Mid(9,6).IsNumeric()&&Stamp84411.Right(3).IsNumeric();
 const FString V84412Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84412/Runtime/"));
 const FString Stamp84412=Dir.StartsWith(V84412Root,ESearchCase::IgnoreCase)?Dir.Mid(V84412Root.Len()):FString();
 const bool V84412=FParse::Param(FCommandLine::Get(),TEXT("WFV84412GameplayFocused"))&&Stamp84412.Len()==19
   &&Stamp84412[8]==TEXT('-')&&Stamp84412[15]==TEXT('-')&&Stamp84412.Left(8).IsNumeric()&&Stamp84412.Mid(9,6).IsNumeric()&&Stamp84412.Right(3).IsNumeric();
 const FString V84413Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84413/Runtime/"));
 const FString Stamp84413=Dir.StartsWith(V84413Root,ESearchCase::IgnoreCase)?Dir.Mid(V84413Root.Len()):FString();
 const bool V84413=FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))&&Stamp84413.Len()==19
   &&Stamp84413[8]==TEXT('-')&&Stamp84413[15]==TEXT('-')&&Stamp84413.Left(8).IsNumeric()&&Stamp84413.Mid(9,6).IsNumeric()&&Stamp84413.Right(3).IsNumeric();
 const FString V84414Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84414/Runtime/"));
 const FString Stamp84414=Dir.StartsWith(V84414Root,ESearchCase::IgnoreCase)?Dir.Mid(V84414Root.Len()):FString();
 const bool V84414=FParse::Param(FCommandLine::Get(),TEXT("WFV84414Focused"))&&Stamp84414.Len()==19
   &&Stamp84414[8]==TEXT('-')&&Stamp84414[15]==TEXT('-')&&Stamp84414.Left(8).IsNumeric()&&Stamp84414.Mid(9,6).IsNumeric()&&Stamp84414.Right(3).IsNumeric();
 const FString V84415Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V84415/Runtime/"));
 const FString Stamp84415=Dir.StartsWith(V84415Root,ESearchCase::IgnoreCase)?Dir.Mid(V84415Root.Len()):FString();
 const bool V84415=FParse::Param(FCommandLine::Get(),TEXT("WFV84415Focused"))&&Stamp84415.Len()==19
   &&Stamp84415[8]==TEXT('-')&&Stamp84415[15]==TEXT('-')&&Stamp84415.Left(8).IsNumeric()&&Stamp84415.Mid(9,6).IsNumeric()&&Stamp84415.Right(3).IsNumeric();
 const FString V85Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V85/Runtime/"));
 const FString Stamp85=Dir.StartsWith(V85Root,ESearchCase::IgnoreCase)?Dir.Mid(V85Root.Len()):FString();
 const bool V85=FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))&&Stamp85.Len()==19
   &&Stamp85[8]==TEXT('-')&&Stamp85[15]==TEXT('-')&&Stamp85.Left(8).IsNumeric()&&Stamp85.Mid(9,6).IsNumeric()&&Stamp85.Right(3).IsNumeric();
 const FString V851Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V851/Runtime/"));
 const FString Stamp851=Dir.StartsWith(V851Root,ESearchCase::IgnoreCase)?Dir.Mid(V851Root.Len()):FString();
 const bool V851=FParse::Param(FCommandLine::Get(),TEXT("WFV851Focused"))&&Stamp851.Len()==19
   &&Stamp851[8]==TEXT('-')&&Stamp851[15]==TEXT('-')&&Stamp851.Left(8).IsNumeric()&&Stamp851.Mid(9,6).IsNumeric()&&Stamp851.Right(3).IsNumeric();
 const FString V86Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V86/Runtime/"));
 const FString Stamp86=Dir.StartsWith(V86Root,ESearchCase::IgnoreCase)?Dir.Mid(V86Root.Len()):FString();
 const bool V86=FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))&&Stamp86.Len()==19
   &&Stamp86[8]==TEXT('-')&&Stamp86[15]==TEXT('-')&&Stamp86.Left(8).IsNumeric()&&Stamp86.Mid(9,6).IsNumeric()&&Stamp86.Right(3).IsNumeric();
 if((!Legacy&&!Current&&!V17&&!V18&&!V19&&!V20&&!V21&&!V833&&!V839&&!V8310&&!V8311&&!V84&&!Ribbon8443&&!V8444&&!V8445&&!V8446&&!V8447&&!V8448&&!V8449&&!V8450&&!V84411&&!V84412&&!V84413&&!V84414&&!V84415&&!V85&&!V851&&!V86)||Dir.Contains(TEXT("..")))return false;
 Out=Dir;return true;
}
bool Available(EFrontlineFaction F){return uint8(F)<=uint8(EFrontlineFaction::German)&&(FrontlineAchievementsV76::CompletedCampaignRouteMaskV82()&(F==EFrontlineFaction::British?1:2))!=0;}
bool NationAvailableV87(const AWarfareFrontlineGameMode* B,EFRRUNationV81 N)
{
 if(N==EFRRUNationV81::British)return Available(EFrontlineFaction::British);
 if(N==EFRRUNationV81::German)return Available(EFrontlineFaction::German);
 return WarfareFRRUCampaignV82::AvailableArchiveReplayV87(B,N);
}
const TArray<FFrontlineCampaignMission>& NationMissionsV87(const AWarfareFrontlineGameMode* B,EFRRUNationV81 N)
{
 static const TArray<FFrontlineCampaignMission> Empty;if(!B)return Empty;
 if(N==EFRRUNationV81::British)return B->GetCampaignMissionsForFaction(EFrontlineFaction::British);
 if(N==EFRRUNationV81::German)return B->GetCampaignMissionsForFaction(EFrontlineFaction::German);
 return WarfareFRRUCampaignV82::ArchiveMissionsV87(B,N);
}
bool BeginNationV87(AWarfareFrontlineGameMode* B,EFRRUNationV81 N,int32 M,EFrontlineDifficulty D)
{
 if(N==EFRRUNationV81::British||N==EFRRUNationV81::German)
 {
  const bool NationalSource=WarfareFRRUCampaignV82::Active(B);
  if(NationalSource&&!WarfareFRRUCampaignV82::SuspendForLegacyArchiveV87(B))return false;
  const bool Started=Begin(B,N==EFRRUNationV81::British?EFrontlineFaction::British:EFrontlineFaction::German,M,D);
  if(!Started&&NationalSource)WarfareFRRUCampaignV82::RestoreAfterLegacyArchiveV87(B);
  return Started;
 }
 return WarfareFRRUCampaignV82::BeginArchiveReplayV87(B,N,M,D);
}
bool Active(const AWarfareFrontlineGameMode* B){return B&&(Sessions.Contains(const_cast<AWarfareFrontlineGameMode*>(B))||WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B));}
bool ReplayEligible(const AWarfareFrontlineGameMode* B)
{
 if(WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))return WarfareFRRUCampaignV82::ArchiveReplayEligibleV87(B);
 const auto* S=B?Sessions.Find(const_cast<AWarfareFrontlineGameMode*>(B)):nullptr;const auto* R=B?B->GetCurrentCampaign():nullptr;
 return S&&S->Qualified&&!S->Assisted&&R&&R->Id==S->Replay&&R->bAchievementEligibleV76&&!FrontlineDebugV37::HasCheatState(B);
}
bool FullArmory(const AWarfareFrontlineGameMode* B,int32 Team)
{
 if(WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))return WarfareFRRUCampaignV82::ArchiveFullArmoryV87(B,Team);
 if(!B||Team<0||Team>1||!B->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(B))return false;
 const auto* S=Sessions.Find(const_cast<AWarfareFrontlineGameMode*>(B));const auto* R=B->GetCurrentCampaign();
 return S&&R&&R->Id==S->Replay&&uint8(B->GetFactionForTeam(Team))<=uint8(EFrontlineFaction::German);
}
void RunParityChecksV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineUltimateArchiveV8215Access::ParityChecksV84412(B,Check);}
bool SetTechnologyEnabled(AWarfareFrontlineGameMode* B,int32 Id,bool Enabled){return FFrontlineUltimateArchiveV8215Access::Technology(B,Id,Enabled,false);}
bool SetArmourTechnologyEnabled(AWarfareFrontlineGameMode* B,int32 Id,bool Enabled){return FFrontlineUltimateArchiveV8215Access::Technology(B,Id,Enabled,true);}
bool Begin(AWarfareFrontlineGameMode* B,EFrontlineFaction F,int32 M,EFrontlineDifficulty D){return FFrontlineUltimateArchiveV8215Access::Begin(B,F,M,D);}
void Leave(AWarfareFrontlineGameMode* B){if(WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))WarfareFRRUCampaignV82::LeaveArchiveReplayV87(B);else{FFrontlineUltimateArchiveV8215Access::Leave(B);WarfareFRRUCampaignV82::RestoreAfterLegacyArchiveV87(B);}}
void MarkAssisted(AWarfareFrontlineGameMode* B){if(WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B))WarfareFRRUCampaignV82::MarkArchiveReplayAssistedV87(B);else if(auto* S=Sessions.Find(B))S->Assisted=true;}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;Check(TEXT("v8215_archive_fixture_absolute_isolated_account"),ReviewUserDir(Dir));if(!B||!ReviewUserDir(Dir))return;
 const auto Before=B->GetCampaignRuns();const auto* Current=B->GetCurrentCampaign();const FGuid Id=Current?Current->Id:FGuid();
 Check(TEXT("v8215_archive_uncompleted_route_cannot_open"),Available(EFrontlineFaction::British)||!Begin(B,EFrontlineFaction::British,0,EFrontlineDifficulty::Veteran));
 if(Available(EFrontlineFaction::British)&&Begin(B,EFrontlineFaction::British,0,EFrontlineDifficulty::Veteran))
 {Check(TEXT("v8215_archive_full_armory_eligible_distinct_run"),ReplayEligible(B)&&FullArmory(B)&&B->GetCurrentCampaign()->Id!=Id&&B->GetCurrentCampaign()->ArmourResearchMaskV63==65535);MarkAssisted(B);Check(TEXT("v8215_archive_assistance_revokes_awards"),!ReplayEligible(B));Leave(B);}
 Check(TEXT("v8215_archive_original_run_and_size_restored"),B->GetCampaignRuns().Num()==Before.Num()&&(!Id.IsValid()||(B->GetCurrentCampaign()&&B->GetCurrentCampaign()->Id==Id)));
}
}
void FFrontlineUltimateArchiveV8215Access::AllocationChecksV84413(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused"))
  &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&!FrontlineUltimateArchiveV8215::Active(B)&&!WarfareFRRUCampaignV82::Active(B);
 Check(TEXT("v84413_archive_allocation_exact_disposable_nonarchive_entry"),Safe);if(!Safe)return;
 const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId;
 const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53;
 const bool Volatile=B->bProgressionVolatile,ReadOnly=B->bProgressionReadOnly;const FString Message=B->LastMessage,Status=B->ProgressionSaveStatus;
 ON_SCOPE_EXIT
 {
  Sessions.Remove(B);B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;B->Phase=Phase;
  B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;
  B->bProgressionVolatile=Volatile;B->bProgressionReadOnly=ReadOnly;B->LastMessage=Message;B->ProgressionSaveStatus=Status;
 };
 auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());Check(TEXT("v84413_archive_allocation_real_fleet_present"),Fleet!=nullptr);if(!Fleet)return;
 for(const auto Nation:{EFrontlineFaction::British,EFrontlineFaction::German})
 {
  FSession Session;Session.Replay=FGuid::NewGuid();Session.Qualified=true;FFrontlineCampaignProgress Run;
  Run.Id=Session.Replay;Run.Faction=Nation;Run.SelectedMission=0;Run.CompletedMissions=B->GetCampaignMissionsForFaction(Nation).Num();
  Run.ArmourResearchMaskV63=65535;Run.Research.ResearchPoints=0;Run.Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1u;
  Run.SupportSelectionMask=1u<<int32(EFrontlineSupport::Smoke);
  B->CampaignRuns={Run};B->ActiveCampaignId=B->BoundCampaignId=Run.Id;B->PlayerFaction=Nation;B->Phase=EFrontlinePhase::Briefing;
  B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=0;
  B->bProgressionReadOnly=false;B->bProgressionVolatile=true;Sessions.Add(B,Session);
  const FString Prefix=Nation==EFrontlineFaction::British?TEXT("v84413_archive_uk_"):TEXT("v84413_archive_de_");
  Check(Prefix+TEXT("early_operation_zero_points_can_choose_all_three_available_attachments"),FrontlineArmourV63::CanRequisition(B,0)&&FrontlineArmourV63::CanRequisition(B,1)&&FrontlineArmourV63::CanRequisition(B,2));
  const bool Pioneer=FrontlineArmourV63::BuyRequisition(B,0),Smoke=FrontlineArmourV63::BuyRequisition(B,1),Armour=FrontlineArmourV63::BuyRequisition(B,2);
  Check(Prefix+TEXT("actual_switches_replace_one_attachment_without_charge_or_paid_credit"),Pioneer&&Smoke&&Armour&&FrontlineArmourV63::Requisition(B)==2
   &&B->CampaignRuns[0].Research.ResearchPoints==0&&B->CampaignRuns[0].RequisitionPaidMaskV836==0
   &&B->CampaignRuns[0].PioneerOrdersMaskV63==0&&B->CampaignRuns[0].SmokeOrdersMaskV63==0&&B->CampaignRuns[0].ArmourOrdersMaskV63==1u);
  Check(Prefix+TEXT("actual_second_click_cancels_free_attachment_without_creating_points"),FrontlineArmourV63::BuyRequisition(B,2)&&FrontlineArmourV63::Requisition(B)==-1&&B->CampaignRuns[0].Research.ResearchPoints==0);
  const auto Equipped=Fleet->GetEquippedRoster();bool Cancelled=true;for(const auto& D:Equipped)Cancelled&=FrontlineArmourV63::ToggleSelection(B,D.Type);
  Check(Prefix+TEXT("empty_armour_disables_armour_reserve_without_disabling_other_attachments"),Cancelled&&Fleet->GetEquippedRoster().IsEmpty()
   &&!FrontlineArmourV63::CanRequisition(B,2)&&FrontlineArmourV63::CanRequisition(B,0)&&FrontlineArmourV63::CanRequisition(B,1));
  if(Nation==EFrontlineFaction::German)
  {
   B->CampaignRuns[0].SelectedMission=17;B->BoundCampaignMission=B->BoundAttachmentStageV53=17;
   Check(Prefix+TEXT("train_attachment_is_optional_in_archive_with_real_rail_operation_gate"),Fleet->RailAvailableV75(0)&&!FrontlineArmourV63::ForcedAllocationV842(B,EFrontlineVehicle::RailConvoy)
    &&FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::RailConvoy)&&FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle::RailConvoy)&&Fleet->GetEquippedRoster().IsEmpty());
  }
  Sessions.Remove(B);B->CampaignRuns[0].SelectedMission=0;B->BoundCampaignMission=B->BoundAttachmentStageV53=0;
  Check(Prefix+TEXT("ordinary_early_mission_remains_closed_after_session_removed"),!FrontlineUltimateArchiveV8215::FullArmory(B,0)&&!FrontlineArmourV63::CanRequisition(B,0));
 }
}
void FrontlineRunArchiveAllocationChecksV84413(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineUltimateArchiveV8215Access::AllocationChecksV84413(B,Check);}
