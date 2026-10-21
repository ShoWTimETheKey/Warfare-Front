#include "WarfareFrontline.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "FRRU/WarfareFRRUCampaignInternalV82.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineAchievementsV76.h"

#include "WarfareFrontlineMissionDataV23.inl"

const FFrontlineCampaignProgress* AWarfareFrontlineGameMode::GetCurrentCampaign() const
{
    if(WarfareFRRUCampaignV82::Active(this))return nullptr;
    return CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& Run){ return Run.Id==ActiveCampaignId; });
}

const FFrontlineArmyProgress& AWarfareFrontlineGameMode::GetActiveResearch() const
{
    const FGuid Wanted=BoundCampaignEncounterId==EncounterId && Phase==EFrontlinePhase::Fighting?BoundCampaignId:ActiveCampaignId;
    if (const auto* Run=CampaignRuns.FindByPredicate([&Wanted](const FFrontlineCampaignProgress& R){return R.Id==Wanted;})) return Run->Research;
    return ArmyProgress[int32(PlayerFaction)]; // Legacy non-campaign fixtures and unassigned archives only.
}

FFrontlineArmyProgress& AWarfareFrontlineGameMode::ActiveResearchMutable()
{
    const FGuid Wanted=BoundCampaignEncounterId==EncounterId && BoundCampaignId.IsValid()?BoundCampaignId:ActiveCampaignId;
    if (auto* Run=CampaignRuns.FindByPredicate([&Wanted](const FFrontlineCampaignProgress& R){return R.Id==Wanted;})) return Run->Research;
    return ArmyProgress[int32(PlayerFaction)];
}

int32 AWarfareFrontlineGameMode::GetEncounterYear() const
{
    const auto* Mission=GetEncounterCampaignMission();
    if (!Mission) Mission=GetSelectedCampaignMission();
    return Mission?Mission->Year:1917;
}

float AWarfareFrontlineGameMode::EncounterVisibilityScale() const
{
    const auto* Mission=GetEncounterCampaignMission();
    return Mission?FMath::Clamp(Mission->VisibilityScale,.5f,1.f):1.f;
}

float AWarfareFrontlineGameMode::EncounterMudMoveScale() const
{
    const auto* Mission=GetEncounterCampaignMission();
    return Mission?FMath::Clamp(Mission->MudMoveScale,.5f,1.f):1.f;
}

bool AWarfareFrontlineGameMode::HasScopedMarksmen(int32 Team) const
{
    if (GetFactionForTeam(Team)!=EFrontlineFaction::British) return false;
    if (Team==0||FrontlineUltimateArchiveV8215::FullArmory(this,Team)) return HasTechnology(12,Team);
    const auto* Mission=GetEncounterCampaignMission(); if (!Mission) Mission=GetSelectedCampaignMission();
    return Team==1 && Mission && Mission->bEnemyScopedMarksmen;
}

bool AWarfareFrontlineGameMode::HasStormtroopers(int32 Team) const
{
    if (GetFactionForTeam(Team)!=EFrontlineFaction::German || (GetEncounterYear()<1918&&!FrontlineUltimateArchiveV8215::FullArmory(this,Team))) return false;
    if (Team==0||FrontlineUltimateArchiveV8215::FullArmory(this,Team)) return HasTechnology(12,Team);
    const auto* Mission=GetEncounterCampaignMission(); if (!Mission) Mission=GetSelectedCampaignMission();
    return Team==1 && Mission && Mission->bEnemyStormtroopers;
}

const TArray<FFrontlineCampaignMission>& AWarfareFrontlineGameMode::GetCampaignMissions() const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::Missions(this);
    const auto* Run=GetCurrentCampaign();
    return GetCampaignMissionsForFaction(Run?Run->Faction:PlayerFaction);
}

const FFrontlineCampaignMission* AWarfareFrontlineGameMode::GetSelectedCampaignMission() const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::SelectedMission(this);
    const auto* Run=GetCurrentCampaign();
    if (!Run || Run->Faction!=PlayerFaction) return nullptr;
    const auto& Missions=GetCampaignMissionsForFaction(Run->Faction);
    return Missions.IsValidIndex(Run->SelectedMission)?&Missions[Run->SelectedMission]:nullptr;
}

const FFrontlineCampaignMission* AWarfareFrontlineGameMode::GetEncounterCampaignMission() const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::EncounterMission(this);
    if (BoundCampaignEncounterId!=EncounterId) return nullptr;
    const auto* Run=CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& Item){ return Item.Id==BoundCampaignId; });
    if (!Run) return nullptr;
    const auto& Missions=GetCampaignMissionsForFaction(Run->Faction);
    return Missions.IsValidIndex(BoundCampaignMission)?&Missions[BoundCampaignMission]:nullptr;
}

FString AWarfareFrontlineGameMode::CampaignMissionBlockReason(int32 MissionId) const
{
    if(WarfareFRRUCampaignV82::Active(this))
    {
        const auto* Row=GetCampaignMissions().FindByPredicate([&](const FFrontlineCampaignMission& M){return M.Id==MissionId;});
        return Row?WarfareFRRUCampaignV82::MissionBlockReason(this,Row->MapKey):TEXT("行动不存在");
    }
    const auto* Run=GetCurrentCampaign();
    if (!Run) return TEXT("请先建立或继续一条战役");
    if (!GetCampaignMissionsForFaction(Run->Faction).IsValidIndex(MissionId)) return TEXT("行动不存在");
    if (Phase==EFrontlinePhase::Fighting) return TEXT("请先结束当前战斗");
    if (bProgressionReadOnly) return TEXT("档案读取异常，原档案已保留");
    if(FrontlineUltimateArchiveV8215::Active(this))return FString();
    return FrontlineCampaignV52::BlockReason(*Run,MissionId);
}

bool AWarfareFrontlineGameMode::StartNewCampaign(EFrontlineFaction Faction,EFrontlineDifficulty Difficulty)
{
    // A temporary replay must be left explicitly before normal archive writes.
    if(FrontlineUltimateArchiveV8215::Active(this))return false;
    if (uint8(Faction)>uint8(EFrontlineFaction::German) || uint8(Difficulty)>uint8(EFrontlineDifficulty::Survivor) || Phase==EFrontlinePhase::Fighting || bProgressionReadOnly) return false;
    if(WarfareFRRUCampaignV82::Active(this)&&!WarfareFRRUCampaignInternalV82::RetireEncounter(this))return false;
    // A bounded archive never silently evicts an existing route.
    if (CampaignRuns.Num()>=256) { LastMessage=TEXT("战役档案已满，现有战役均已保留"); return false; }
    const FGuid BeforeId=ActiveCampaignId; const EFrontlineFaction BeforeFaction=PlayerFaction;
    FFrontlineCampaignProgress Run; Run.Id=FGuid::NewGuid(); Run.Faction=Faction; Run.Difficulty=Difficulty;
    FrontlineCampaignV52::Initialize(Run);
    Run.CreatedUtc=Run.UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    CampaignRuns.Add(Run); ActiveCampaignId=Run.Id; PlayerFaction=Faction;
    if (!SaveProgression()) { CampaignRuns.Pop(); ActiveCampaignId=BeforeId; PlayerFaction=BeforeFaction; LastMessage=ProgressionSaveStatus; return false; }
    WarfareFRRUCampaignV82::Deactivate(this);
    ResetEncounter(); LastMessage=TEXT("作战命令已签发 · 选择行动"); return true;
}

bool AWarfareFrontlineGameMode::ContinueCampaign(const FGuid& CampaignId)
{
    if((Phase==EFrontlinePhase::Victory||Phase==EFrontlinePhase::Defeat)&&!FrontlineAchievementsV76::FinalizeBattleKillsV8217(this))return false;
    if(FrontlineUltimateArchiveV8215::Active(this))return false;
    const double BeganV8214=FPlatformTime::Seconds();
    // An implicit return after a national battle belongs to its own archive.
    // An explicit old GUID still selects the original UK/DE campaign route.
    if(!CampaignId.IsValid()&&WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::Continue(this,WarfareFRRUCampaignV82::CampaignId(this));
    if (Phase==EFrontlinePhase::Fighting || bProgressionReadOnly || CampaignRuns.IsEmpty()) return false;
    if(WarfareFRRUCampaignV82::Active(this)&&!WarfareFRRUCampaignInternalV82::RetireEncounter(this))return false;
    const FGuid Wanted=CampaignId.IsValid()?CampaignId:(ActiveCampaignId.IsValid()?ActiveCampaignId:CampaignRuns.Last().Id);
    auto* Run=CampaignRuns.FindByPredicate([&Wanted](const FFrontlineCampaignProgress& Item){ return Item.Id==Wanted; });
    if (!Run) { LastMessage=TEXT("未找到这条战役，现有档案已保留"); return false; }
    const FGuid BeforeId=ActiveCampaignId; const EFrontlineFaction BeforeFaction=PlayerFaction;
    const FString BeforeUpdated=Run->UpdatedUtc;
    ActiveCampaignId=Run->Id; PlayerFaction=Run->Faction; Run->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    const double SaveBeganV8214=FPlatformTime::Seconds();
    if (!SaveProgression()) { ActiveCampaignId=BeforeId; PlayerFaction=BeforeFaction; Run->UpdatedUtc=BeforeUpdated; LastMessage=ProgressionSaveStatus; return false; }
    const double SaveEndedV8214=FPlatformTime::Seconds();
    WarfareFRRUCampaignV82::Deactivate(this);
    ResetEncounter(); LastMessage=TEXT("战役已继续 · 选择行动");
    if(FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfContinueOnlyV8214")))
        UE_LOG(LogTemp,Display,TEXT("WF_CONTINUE_V8214 save_ms=%.3f reset_ms=%.3f total_ms=%.3f volatile=%d"),
            (SaveEndedV8214-SaveBeganV8214)*1000.,(FPlatformTime::Seconds()-SaveEndedV8214)*1000.,
            (FPlatformTime::Seconds()-BeganV8214)*1000.,bProgressionVolatile?1:0);
    return true;
}

bool AWarfareFrontlineGameMode::PrepareContinuePersistenceReviewV8214()
{
    const TCHAR* Cmd=FCommandLine::Get();FString User,Root;
    if(!bProgressionVolatile||Phase!=EFrontlinePhase::Briefing||!GetCurrentCampaign()
        ||!FParse::Param(Cmd,TEXT("WFRenderNoSave"))||!FParse::Param(Cmd,TEXT("WFUIPerfV8213"))
        ||!FParse::Param(Cmd,TEXT("WFUIPerfContinueOnlyV8214"))||!FParse::Param(Cmd,TEXT("WFUIPerfPersistenceV8214"))
        ||!FParse::Value(Cmd,TEXT("UserDir="),User)||!FParse::Value(Cmd,TEXT("WFUIPerfRoot="),Root)
        ||User.IsEmpty()||FPaths::IsRelative(User)||User.Contains(TEXT("..")))return false;
    FPaths::NormalizeDirectoryName(User);FPaths::NormalizeDirectoryName(Root);
    const FString Allowed=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8213/Transitions"));
    if(Root!=Allowed||!User.StartsWith(Allowed+TEXT("/"),ESearchCase::IgnoreCase))return false;
    const FString Fixture=User/TEXT("Saved/Gameplay/V8214/Continue/fixture-profile.json");
    for(const FString& Path:{Fixture,Fixture+TEXT(".backup"),Fixture+TEXT(".pending")})
        if(IFileManager::Get().FileExists(*Path))return false;
    // The ordinary no-save art fixture intentionally has no archive dates.
    // Give this disposable record valid production metadata before readback.
    auto* FixtureRun=CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& R){return R.Id==ActiveCampaignId;});
    if(!FixtureRun)return false;
    FixtureRun->CreatedUtc=FixtureRun->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    FrontlineCampaignV52::Initialize(*FixtureRun);
    const FString BeforeFile=ProgressionFile;const FGuid BeforeId=ActiveCampaignId;
    const int32 BeforeCount=CampaignRuns.Num();const FString BeforeUpdated=GetCurrentCampaign()->UpdatedUtc;
    ProgressionFile=Fixture;bProgressionVolatile=false;
    if(!SaveProgression()){ProgressionFile=BeforeFile;bProgressionVolatile=true;return false;}
    // Explicit true retains this assigned fixture path. It cannot fall through
    // LoadProgression's ordinary AppLocal profile assignment.
    LoadProgression(true);
    return !bProgressionReadOnly&&!bProgressionVolatile&&ProgressionFile==Fixture&&ActiveCampaignId==BeforeId
        &&CampaignRuns.Num()==BeforeCount&&GetCurrentCampaign()&&GetCurrentCampaign()->UpdatedUtc==BeforeUpdated;
}

bool AWarfareFrontlineGameMode::SelectCampaignMission(int32 MissionId)
{
    if(WarfareFRRUCampaignV82::Active(this))
    {
        const auto* Row=GetCampaignMissions().FindByPredicate([&](const FFrontlineCampaignMission& M){return M.Id==MissionId;});
        return Row&&WarfareFRRUCampaignV82::SelectMission(this,Row->MapKey);
    }
    const FString Reason=CampaignMissionBlockReason(MissionId);
    if (!Reason.IsEmpty()) { LastMessage=Reason; return false; }
    auto* Run=CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& Item){ return Item.Id==ActiveCampaignId; });
    const int32 BeforeMission=Run->SelectedMission; const FString BeforeUpdated=Run->UpdatedUtc;
    Run->SelectedMission=MissionId; Run->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
    if (!SaveProgression()) { Run->SelectedMission=BeforeMission; Run->UpdatedUtc=BeforeUpdated; LastMessage=ProgressionSaveStatus; return false; }
    ResetEncounter(); LastMessage=GetSelectedCampaignMission()->Title; return true;
}

bool AWarfareFrontlineGameMode::DeleteCampaignArchives(const TArray<FGuid>& CampaignIds)
{
    if(FrontlineUltimateArchiveV8215::Active(this))return false;
    // The UI confirms a stable GUID set. Never infer deletion from a row index,
    // never remove a live battle's route, and commit the entire batch once.
    if(Phase!=EFrontlinePhase::Briefing || bProgressionReadOnly || CampaignIds.IsEmpty())return false;
    TSet<FGuid> Selected;
    for(const auto& Id:CampaignIds)
    {
        if(!Id.IsValid() || !CampaignRuns.ContainsByPredicate([&](const FFrontlineCampaignProgress& R){return R.Id==Id;}))
        {LastMessage=TEXT("档案列表已改变，请重新选择。");return false;}
        Selected.Add(Id);
    }
    const auto Before=CampaignRuns;const auto BeforeActive=ActiveCampaignId;const auto BeforeFaction=PlayerFaction;
    CampaignRuns.RemoveAll([&](const FFrontlineCampaignProgress& R){return Selected.Contains(R.Id);});
    const bool ActiveRemoved=Selected.Contains(ActiveCampaignId);
    if(ActiveRemoved)
    {
        ActiveCampaignId=CampaignRuns.IsEmpty()?FGuid():CampaignRuns.Last().Id;
        if(!CampaignRuns.IsEmpty())PlayerFaction=CampaignRuns.Last().Faction;
    }
    FString HonourNotice;
    if(!FrontlineAchievementsV76::CommitArchiveDeletionV83(this,[&](){return SaveProgression();},HonourNotice))
    {
        CampaignRuns=Before;ActiveCampaignId=BeforeActive;PlayerFaction=BeforeFaction;
        LastMessage=HonourNotice.IsEmpty()?TEXT("删除未能保存，战役档案均已保留。"):HonourNotice;return false;
    }
    if(ActiveRemoved || Selected.Contains(BoundCampaignId))ResetEncounter();
    LastMessage=FString::Printf(TEXT("已删除 %d 条战役档案。"),Selected.Num());
    UE_LOG(LogTemp,Display,TEXT("WF_V36_CAMPAIGN_ARCHIVES_DELETED count=%d remaining=%d active_rebound=%d"),Selected.Num(),CampaignRuns.Num(),ActiveRemoved);
    return true;
}

void AWarfareFrontlineGameMode::BindCampaignEncounter()
{
    BoundCampaignId.Invalidate(); BoundCampaignEncounterId.Invalidate(); BoundCampaignMission=INDEX_NONE; BoundAttachmentStageV53=INDEX_NONE; bCampaignMissionAdvanced=false;
    const auto* Run=GetCurrentCampaign();
    if (!Run || Run->Faction!=PlayerFaction || !GetCampaignMissionsForFaction(Run->Faction).IsValidIndex(Run->SelectedMission)
        || (!FrontlineUltimateArchiveV8215::Active(this)&&!FrontlineCampaignV52::BlockReason(*Run,Run->SelectedMission).IsEmpty())) return;
    BoundCampaignId=Run->Id; BoundCampaignEncounterId=EncounterId; BoundCampaignMission=Run->SelectedMission;
    BoundAttachmentStageV53=FMath::Max(Run->CompletedMissions,Run->SelectedMission);
}


EFrontlineDifficulty AWarfareFrontlineGameMode::GetCampaignDifficulty() const
{
    if(WarfareFRRUCampaignV82::Active(this))return WarfareFRRUCampaignV82::Difficulty(this);
    const FGuid Wanted=BoundCampaignEncounterId==EncounterId && Phase==EFrontlinePhase::Fighting?BoundCampaignId:ActiveCampaignId;
    const auto* Run=CampaignRuns.FindByPredicate([&Wanted](const FFrontlineCampaignProgress& R){return R.Id==Wanted;});
    return Run?Run->Difficulty:EFrontlineDifficulty::Rookie;
}

void AWarfareFrontlineGameMode::MarkAchievementAssistedV76()
{
    if(IsProgressionVolatile())return;
    auto* Run=CampaignRuns.FindByPredicate([this](const FFrontlineCampaignProgress& R){return R.Id==ActiveCampaignId;});
    if(Run&&Run->bAchievementEligibleV76){Run->bAchievementEligibleV76=false;SaveProgression();}
}
