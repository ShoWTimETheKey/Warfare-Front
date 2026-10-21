#include "WarfareFrontlineCampaignV51.h"
#include "WarfareFrontlineUIAssetsV8213.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineCampaignV41.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FCopyV51 {const TCHAR* Key;const TCHAR* Chinese;const TCHAR* English;};
const FCopyV51 LinesV51[]={
#define WFV51_MESSAGE(Key,Zh,En) {TEXT(Key),TEXT(Zh),TEXT(En)},
#include "WarfareFrontlineCampaignCopyV51.inl"
#undef WFV51_MESSAGE
};
bool ReviewAllowedV51(const AWarfareFrontlineGameMode* B)
{
    FString Dir;return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty();
}
}

struct FFrontlineCampaignV51Access
{
    static void Seed(AWarfareFrontlineGameMode* B,EFrontlineFaction F)
    {
        FFrontlineCampaignProgress R;R.Id=FGuid::NewGuid();R.Faction=F;R.Difficulty=EFrontlineDifficulty::Veteran;
        R.CompletedMissions=19;R.SelectedMission=19;R.Research.Victories=19;R.Research.Defeats=3;
        R.Research.TechnologyMask=(1u<<17)-1;R.Research.ResearchPoints=4;
        B->CampaignRuns={R};B->ActiveCampaignId=R.Id;B->PlayerFaction=F;
        B->EncounterId=FGuid::NewGuid();B->bEncounterRewarded=false;
        B->BindCampaignEncounter();B->Phase=EFrontlinePhase::Fighting;B->Elapsed=312;B->ShotsFired=127;
        B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0;
    }
    static bool Prepare(AWarfareFrontlineGameMode* B,EFrontlineFaction F)
    {
        if(!ReviewAllowedV51(B)||uint8(F)>1)return false;
        if(!FrontlineCampaignV41::PrepareReviewOperation(B,F,19))return false;
        Seed(B,F);B->FinishBattle(0,TEXT("The rear road is clear."));B->BattleOutroElapsed=7.1f;
        return FrontlineCampaignV51::CanOpenEnding(B);
    }
    static void Checks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        if(!ReviewAllowedV51(B)){Check(TEXT("v51_campaign_review_isolated_profile_required"),false);return;}
        const auto Runs=B->CampaignRuns;const auto Active=B->ActiveCampaignId;const auto Faction=B->PlayerFaction;
        const auto Receipts=B->RewardedEncounterIds;
        const bool Ticking=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
        ON_SCOPE_EXIT{B->AbandonBattleToOperations();B->CampaignRuns=Runs;B->ActiveCampaignId=Active;B->PlayerFaction=Faction;B->RewardedEncounterIds=Receipts;B->ResetEncounter();B->SetActorTickEnabled(Ticking);};
        bool Advanced=true,Waits=true,Once=true,Reopen=true,Loss=true,Early=true;
        for(int32 F=0;F<2;++F)
        {
            Seed(B,EFrontlineFaction(F));Early&=!FrontlineCampaignV51::CanOpenEnding(B);
            B->FinishBattle(0,TEXT("V51 labelled final-operation fixture"));
            Advanced&=B->GetCurrentCampaign()->CompletedMissions==20&&B->GetCurrentCampaign()->Research.Victories==20&&FrontlineCampaignV51::IsFinalVictory(B);
            Waits&=!FrontlineCampaignV51::CanOpenEnding(B);
            B->BattleOutroElapsed=7.01f;Waits&=FrontlineCampaignV51::CanOpenEnding(B);
            B->FinishBattle(0,TEXT("V51 duplicate completion fixture"));
            Once&=B->GetCurrentCampaign()->CompletedMissions==20&&B->GetCurrentCampaign()->Research.Victories==20;
            // A completed archive remains readable while another operation is selected.
            B->Phase=EFrontlinePhase::Briefing;B->CampaignRuns[0].SelectedMission=2;
            Reopen&=FrontlineCampaignV51::CanOpenEnding(B)&&B->GetCurrentCampaign()->Research.TechnologyMask==((1u<<17)-1);
            Seed(B,EFrontlineFaction(F));B->FinishBattle(1,TEXT("V51 labelled final-operation defeat fixture"));B->BattleOutroElapsed=7.01f;
            Loss&=!FrontlineCampaignV51::CanOpenEnding(B)&&B->GetCurrentCampaign()->CompletedMissions==19;
        }
        Check(TEXT("v51_final_victory_advances_actual_campaign_record_both_factions"),Advanced);
        Check(TEXT("v51_epilogue_waits_for_existing_seven_second_battle_outro"),Waits);
        Check(TEXT("v51_duplicate_outcome_does_not_repeat_campaign_reward"),Once);
        Check(TEXT("v51_completed_archive_reopens_epilogue_without_resetting_research"),Reopen);
        Check(TEXT("v51_final_defeat_and_incomplete_route_cannot_open_epilogue"),Loss&&Early);
    }
};

namespace FrontlineCampaignV51
{
const TCHAR* Copy(const TCHAR* Key)
{
    for(const auto& L:LinesV51)if(FCString::Strcmp(L.Key,Key)==0)return L.English;
    return TEXT("");
}
void ApplyCampaign(TArray<FFrontlineCampaignMission>& Missions,bool German)
{
    for(auto& M:Missions)
    {
        const FString Key=FString::Printf(TEXT("%s.Order%d"),German?TEXT("G"):TEXT("B"),M.Id);
        const TCHAR* Order=Copy(*Key);if(*Order)M.Briefing=Order;
    }
}
bool CampaignCompleted(const AWarfareFrontlineGameMode* B)
{
    if(WarfareFRRUCampaignV82::Active(B))
    {FFRRUCampaignSummaryV82 Record;return WarfareFRRUCampaignV82::CurrentRecord(B,Record)&&B->GetCampaignMissions().Num()>0&&Record.Completed>=B->GetCampaignMissions().Num();}
    const auto* R=B?B->GetCurrentCampaign():nullptr;
    return R&&B->GetCampaignMissions().Num()>0&&R->CompletedMissions>=B->GetCampaignMissions().Num();
}
bool IsFinalVictory(const AWarfareFrontlineGameMode* B)
{
    if(WarfareFRRUCampaignV82::Active(B))return B->Phase==EFrontlinePhase::Victory&&WarfareFRRUCampaignV82::CanOpenEnding(B);
    const auto* M=B?B->GetEncounterCampaignMission():nullptr;
    return B&&M&&B->Phase==EFrontlinePhase::Victory&&CampaignCompleted(B)&&M->Id==B->GetCampaignMissions().Num()-1;
}
bool CanOpenEnding(const AWarfareFrontlineGameMode* B)
{
    if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::CanOpenEnding(B);
    return CampaignCompleted(B)&&!B->IsBattleOutroActive()
        &&(B->Phase==EFrontlinePhase::Briefing||IsFinalVictory(B));
}
UTexture2D* Photo(int32 Slot)
{
    if(Slot<0||Slot>=4)return nullptr;
    static TStrongObjectPtr<UTexture2D> Photos[4];
    static const TCHAR* Names[]={TEXT("Signals"),TEXT("AutomaticRifles"),TEXT("TrenchMortar"),TEXT("Armistice")};
    if(!Photos[Slot].IsValid())
    {
        const FString Path=FString::Printf(TEXT("/Game/Warfare/Campaign/V51/Photos/T_Photo%s.T_Photo%s"),Names[Slot],Names[Slot]);
        Photos[Slot].Reset(FrontlineUIAssetsV8213::Texture(Path));
    }
    return Photos[Slot].Get();
}
void AppendUIPreloadPathsV8213(TArray<FString>& Paths)
{
    for(const TCHAR* Name:{TEXT("Signals"),TEXT("AutomaticRifles"),TEXT("TrenchMortar"),TEXT("Armistice")})
        Paths.Add(FString::Printf(TEXT("/Game/Warfare/Campaign/V51/Photos/T_Photo%s.T_Photo%s"),Name,Name));
}
int32 PhotoForOperation(int32 Operation)
{
    // Observer / mobile-gun / supply photographs follow their actual roles in
    // the company orders. Armistice is reserved for the completed campaign.
    static constexpr int32 Slots[]={-1,-1,0,0,1,1,2,2,1,2,1,0,1,2,0,1,0,2,0,0};
    return Operation>=0&&Operation<UE_ARRAY_COUNT(Slots)?Slots[Operation]:INDEX_NONE;
}
bool PrepareEndingReview(AWarfareFrontlineGameMode* B,EFrontlineFaction F)
{return FFrontlineCampaignV51Access::Prepare(B,F);}
void RunChecksV51(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    bool Orders=true,Dates=true;int32 Authored=0;
    if(B)for(int32 F=0;F<2;++F)
    {
        const auto& Route=B->GetCampaignMissionsForFaction(EFrontlineFaction(F));
        Orders&=Route.Num()==20;
        for(const auto& M:Route)
        {
            const TCHAR* Order=Copy(*FString::Printf(TEXT("%s.Order%d"),F?TEXT("G"):TEXT("B"),M.Id));
            if(*Order){++Authored;Orders&=FCString::Strcmp(M.Briefing,Order)==0;}
            Dates&=M.Id>=0&&M.Id<20&&M.CalendarDateV41>=19170409&&M.CalendarDateV41<=19181104;
        }
    }
    Check(TEXT("v51_twenty_new_company_orders_reach_actual_missions"),Orders&&Authored==20);
    Check(TEXT("v51_twenty_operation_ids_and_historical_date_range_preserved"),Dates);
    bool Photos=true;for(int32 I=0;I<4;++I)
    {
        const auto* T=Photo(I);
        // NullRHI does not construct texture platform data: GetSizeX/Y can be
        // zero despite a correctly loaded asset. ImportedSize is retained in
        // cooked builds and verifies the original image, without requiring RHI.
        const FIntPoint Imported=T?T->GetImportedSize():FIntPoint::ZeroValue;
        Photos&=T&&Imported==FIntPoint(1672,941);
        UE_LOG(LogTemp,Display,TEXT("V51 photo %d: asset=%s imported=%dx%d platform=%dx%d"),I,
            T?*T->GetPathName():TEXT("MISSING"),Imported.X,Imported.Y,T?T->GetSizeX():0,T?T->GetSizeY():0);
    }
    Check(TEXT("v51_four_original_period_photo_assets_are_loaded"),Photos);
    bool Mapping=true;for(int32 I=0;I<20;++I)Mapping&=PhotoForOperation(I)<3;
    Check(TEXT("v51_armistice_photo_never_precedes_campaign_completion"),Mapping&&PhotoForOperation(2)==0&&PhotoForOperation(4)==1&&PhotoForOperation(6)==2);
    FFrontlineCampaignV51Access::Checks(B,Check);
}
}
