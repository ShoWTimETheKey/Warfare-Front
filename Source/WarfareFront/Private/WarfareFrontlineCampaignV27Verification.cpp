#include "WarfareFrontlineCampaignV27.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGrenade.h"
#include "WarfareFrontlinePacingV24.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

TSharedRef<FJsonObject> RunFrontlineV27CampaignChecks(AWarfareFrontlineGameMode* B)
{
    auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;
    auto Check=[&](const TCHAR* Name,bool Pass,const FString& Detail=FString())
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);J->SetStringField(TEXT("detail"),Detail);Checks.Add(MakeShared<FJsonValueObject>(J));UE_LOG(LogTemp,Display,TEXT("WF_V27_CAMPAIGN %s %s %s"),Name,Pass?TEXT("PASS"):TEXT("FAIL"),*Detail);};
    ON_SCOPE_EXIT {int32 P=0;for(const auto& J:Checks)P+=J->AsObject()->GetBoolField(TEXT("passed"))?1:0;R->SetArrayField(TEXT("checks"),Checks);R->SetNumberField(TEXT("passed"),P);R->SetNumberField(TEXT("total"),Checks.Num());};
    if(!B || !B->bProgressionVolatile || !FParse::Param(FCommandLine::Get(),TEXT("WFV27Review")))
    {Check(TEXT("isolated_v27_fixture_required"),false);return R;}
    const auto SavedRuns=B->CampaignRuns;const auto SavedActive=B->ActiveCampaignId;const auto SavedFaction=B->PlayerFaction;
    const auto SavedA=B->ArmyProgress[0],SavedB=B->ArmyProgress[1];const auto SavedRewards=B->RewardedEncounterIds;
    const bool WasTicking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
    ON_SCOPE_EXIT {B->CampaignRuns=SavedRuns;B->ActiveCampaignId=SavedActive;B->PlayerFaction=SavedFaction;B->ArmyProgress[0]=SavedA;B->ArmyProgress[1]=SavedB;B->RewardedEncounterIds=SavedRewards;B->ResetEncounter();B->SetActorTickEnabled(WasTicking);};
    B->ResetEncounter();B->StartNewCampaign(EFrontlineFaction::British);
    Check(TEXT("british_first_action_rifle_only_and_research_gate"),B->IsRoleUnlocked(EFrontlineRole::Rifle) && !B->IsRoleUnlocked(EFrontlineRole::Mills)
        && !B->IsRoleUnlocked(EFrontlineRole::Lewis) && !B->TechnologyBlockReason(2).IsEmpty() && !B->PurchaseTechnology(2));
    B->BeginBattle();const int32 UnitsBefore=B->Units.Num();const float SuppliesBefore=B->Supplies[0];
    Check(TEXT("actual_locked_recruit_is_rejected_without_spawn_or_charge"),!B->Recruit(EFrontlineRole::Lewis) && B->Units.Num()==UnitsBefore && B->Supplies[0]==SuppliesBefore && !B->RoleUnlockReason(EFrontlineRole::Lewis).IsEmpty());
    Check(TEXT("actual_first_action_rifle_recruit_succeeds"),B->Recruit(EFrontlineRole::Rifle));
    B->Elapsed=46;B->ShotsFired=8;B->FinishBattle(0,TEXT("V27 explicit progression fixture"));
    Check(TEXT("real_campaign_victory_unlocks_bombers_next"),B->GetCurrentCampaign() && B->GetCurrentCampaign()->CompletedMissions==1
        && B->IsRoleUnlocked(EFrontlineRole::Mills) && !B->IsRoleUnlocked(EFrontlineRole::Lewis));
    B->RestartBattle();auto* Run=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& P){return P.Id==B->ActiveCampaignId;});
    if(!Run){Check(TEXT("campaign_fixture_still_exists"),false);return R;}
    Run->CompletedMissions=4;Run->SelectedMission=4;Run->Research.TechnologyMask=(1u<<16)-1;Run->Research.ResearchPoints=37;
    const auto Id=Run->Id;const uint32 Mask=Run->Research.TechnologyMask;const int32 Points=Run->Research.ResearchPoints;
    Check(TEXT("replaying_early_action_keeps_earned_roles_and_paid_training"),B->SelectCampaignMission(0) && B->IsRoleUnlocked(EFrontlineRole::Officer)
        && B->IsRoleUnlocked(EFrontlineRole::Marksman) && B->GetActiveResearch().TechnologyMask==Mask && B->GetResearchPoints()==Points);
    Run=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& P){return P.Id==Id;});Run->CompletedMissions=0;
    Check(TEXT("legacy_training_bits_survive_unavailable_company_assignment"),B->HasTechnology(12) && !B->IsRoleUnlocked(EFrontlineRole::Marksman)
        && B->GetActiveResearch().TechnologyMask==Mask && !B->RoleUnlockReason(EFrontlineRole::Marksman).IsEmpty());
    bool IntroComplete=true,Progressive=true;
    for(int32 Nation=0;Nation<2;++Nation)
    {
        B->ResetEncounter();B->StartNewCampaign(EFrontlineFaction(Nation));
        auto* Current=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& P){return P.Id==B->ActiveCampaignId;});
        int32 Previous=0;
        for(int32 Stage=0;Stage<6;++Stage)
        {
            Current->CompletedMissions=Stage;Current->SelectedMission=Stage;
            const auto Intro=B->GetMissionIntroduction();IntroComplete&=!Intro.Title.IsEmpty() && !Intro.Context.IsEmpty() && !Intro.Tactic.IsEmpty();
            int32 Available=0;for(int32 Role=0;Role<int32(EFrontlineRole::Count);++Role)Available+=B->IsRoleUnlocked(EFrontlineRole(Role))?1:0;
            Progressive&=Available>=Previous && (Stage<4 || Available==5);Previous=Available;
        }
        Current->CompletedMissions=0;Current->SelectedMission=0;
    }
    Check(TEXT("all_twelve_action_introductions_and_monotonic_role_rosters"),IntroComplete && Progressive);
    Check(TEXT("german_first_action_has_bombers_but_no_machinegun"),B->IsRoleUnlocked(EFrontlineRole::Mills) && !B->IsRoleUnlocked(EFrontlineRole::Lewis) && !B->IsRoleUnlocked(EFrontlineRole::Officer));
    auto* GermanRun=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& P){return P.Id==B->ActiveCampaignId;});
    GermanRun->CompletedMissions=2;GermanRun->SelectedMission=2;
    Check(TEXT("german_command_group_precedes_precision_section"),B->IsRoleUnlocked(EFrontlineRole::Officer) && !B->IsRoleUnlocked(EFrontlineRole::Marksman));
    bool Qualitative=true;for(const auto F:{EFrontlineFaction::British,EFrontlineFaction::German})for(const auto& Node:B->GetTechnologyNodesForFaction(F))
    {const FString Copy=Node.Effect;Qualitative&=!Copy.Contains(TEXT("%")) && !Copy.Contains(TEXT(" 秒")) && !Copy.Contains(TEXT("≥")) && !Copy.IsEmpty();}
    Check(TEXT("technology_explanations_are_qualitative_and_ids_preserved"),Qualitative && B->GetTechnologyNodes().Num()==16);
    B->BeginBattle();B->SpawnSquad(0,EFrontlineRole::Mills,B->GetHomeTrenchIndex());B->SpawnSquad(1,EFrontlineRole::Rifle,B->GetHomeTrenchIndex(1));
    AWarfareFrontlineSoldier* Thrower=nullptr;AWarfareFrontlineSoldier* Target=nullptr;
    for(AWarfareFrontlineSoldier* U:B->Units){U->SetActorTickEnabled(false);if(U->Team==0 && U->MemberIndex==0)Thrower=U;if(U->Team==1 && U->MemberIndex==0)Target=U;}
    if(!Thrower || !Target){Check(TEXT("grenade_actual_soldier_fixture_ready"),false,B->AssetFailure);return R;}
    Thrower->GrenadeClock=100;Thrower->Tick(.2f);
    Check(TEXT("actual_grenade_readiness_clock_runs_at_half_previous_rate"),FMath::IsNearlyEqual(Thrower->GrenadeClock,100.f-.2f*FrontlinePacingV24::GrenadeReadinessRate*.5f,.0001f));
    Thrower->SetActorLocation(FVector(1000,0,1288));Target->SetActorLocation(FVector(1600,0,1288));Thrower->MotionSpeed=Target->MotionSpeed=0;Thrower->Suppression=0;
    float MaxRadius=0;bool WithinNewRadius=true;
    for(int32 N=0;N<32;++N){Thrower->GrenadeThrowsStarted=N;const FVector Error=FrontlineGrenade::EstimateAim(Thrower,Target)-Target->Feet();const float Radius=FMath::Sqrt(FMath::Square(Error.X)+FMath::Square(Error.Y/FrontlineGrenade::CrossRangeScale));MaxRadius=FMath::Max(MaxRadius,Radius);WithinNewRadius&=Radius<=210.01f;}
    Check(TEXT("real_prepared_aim_uses_double_linear_dispersion"),WithinNewRadius && MaxRadius>105.f,FString::Printf(TEXT("32 fixed-aim samples: greatest elliptical radius %.3f cm; previous cap 105, new cap 210"),MaxRadius));
    return R;
}

void StartFrontlineV27CampaignReview(AWarfareFrontlineController* Controller)
{
    if(!Controller || !FParse::Param(FCommandLine::Get(),TEXT("WFV27Review")))return;
    FTimerHandle Handle;TWeakObjectPtr<AWarfareFrontlineController> Weak=Controller;
    Controller->GetWorldTimerManager().SetTimer(Handle,[Weak]()
    {
        if(!Weak.IsValid())return;
        auto* B=Cast<AWarfareFrontlineGameMode>(Weak->GetWorld()->GetAuthGameMode());
        FString UserDir;const bool Isolated=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
        auto Report=Isolated?RunFrontlineV27CampaignChecks(B):MakeShared<FJsonObject>();
        if(!Isolated)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),TEXT("isolated_user_directory_required"));Row->SetBoolField(TEXT("passed"),false);
            Report->SetArrayField(TEXT("checks"),TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueObject>(Row)});
            Report->SetNumberField(TEXT("passed"),0);Report->SetNumberField(TEXT("total"),1);
        }
        Report->SetBoolField(TEXT("success"),Report->GetNumberField(TEXT("passed"))==Report->GetNumberField(TEXT("total")));
        Report->SetBoolField(TEXT("isolated_user_directory"),Isolated);Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
        Report->SetNumberField(TEXT("grenade_readiness_multiplier_vs_v26"),FrontlineGrenade::ReadinessRateMultiplier);
        Report->SetNumberField(TEXT("grenade_linear_dispersion_multiplier_vs_v26"),FrontlineGrenade::AimScatterMultiplier);
        Report->SetStringField(TEXT("scope"),TEXT("Explicit synchronous production-API fixtures; not a natural campaign playthrough, disk save roundtrip or visual inspection."));
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V27");IFileManager::Get().MakeDirectory(*Dir,true);
        FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
        const bool Saved=FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("campaign-review.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("WF_V27_CAMPAIGN_REVIEW_FINISHED passed=%.0f total=%.0f saved=%d"),Report->GetNumberField(TEXT("passed")),Report->GetNumberField(TEXT("total")),Saved);
        FPlatformMisc::RequestExit(false);
    },2.f,false);
}
