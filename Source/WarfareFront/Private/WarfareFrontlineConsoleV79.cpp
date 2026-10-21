#include "WarfareFrontlineConsoleV79.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "Misc/DateTime.h"

namespace
{
bool ValidFactionV79(EFrontlineFaction Faction)
{return uint8(Faction)<=uint8(EFrontlineFaction::German);}

bool ValidRunV79(const FFrontlineCampaignProgress& Run,FDateTime& Updated)
{
    FDateTime Created;
    return Run.Id.IsValid()&&ValidFactionV79(Run.Faction)
        &&Run.CreatedUtc.Len()<=64&&Run.UpdatedUtc.Len()<=64
        &&FDateTime::ParseIso8601(*Run.CreatedUtc,Created)
        &&FDateTime::ParseIso8601(*Run.UpdatedUtc,Updated)&&Updated>=Created;
}

EFrontlineFaction ResolvePolicyV79(const TArray<FFrontlineCampaignProgress>& Runs,
    const FGuid& ActiveId,EFrontlineFaction Selected,bool bReadOnly,bool bHome)
{
    if(bReadOnly)return EFrontlineFaction::British;
    if(!bHome)
    {
        if(const auto* Current=Runs.FindByPredicate([&](const FFrontlineCampaignProgress& Run)
            {return Run.Id==ActiveId&&Run.Id.IsValid()&&ValidFactionV79(Run.Faction);}))return Current->Faction;
        return ValidFactionV79(Selected)?Selected:EFrontlineFaction::British;
    }
    EFrontlineFaction Result=EFrontlineFaction::British;
    FDateTime Latest=FDateTime::MinValue();
    for(const auto& Run:Runs)
    {
        FDateTime Updated;
        if(ValidRunV79(Run,Updated)&&Updated>=Latest){Latest=Updated;Result=Run.Faction;}
    }
    return Result;
}
}

namespace FrontlineConsoleV79
{
EFRRUNationV81 ResolveDisplayNationV82(const AWarfareFrontlineGameMode* Battle,bool bHome)
{
    if(!Battle)return EFRRUNationV81::British;
    if(!bHome)
    {
        if(WarfareFRRUCampaignV82::Active(Battle))return WarfareFRRUCampaignV82::Nation(Battle);
        return ResolveDisplayFaction(Battle,false)==EFrontlineFaction::German?EFRRUNationV81::German:EFRRUNationV81::British;
    }
    // Home follows the newest successfully read campaign across both stores.
    // Reading an identity here never continues that campaign or changes armies.
    FDateTime Latest=FDateTime::MinValue();EFRRUNationV81 Result=EFRRUNationV81::British;
    if(!Battle->IsProgressionReadOnly())for(const auto& Run:Battle->GetCampaignRuns())
    {
        FDateTime Updated;
        if(ValidRunV79(Run,Updated)&&Updated>=Latest)
        {Latest=Updated;Result=Run.Faction==EFrontlineFaction::German?EFRRUNationV81::German:EFRRUNationV81::British;}
    }
    for(const auto& Run:WarfareFRRUCampaignV82::Archives(Battle))
    {
        FDateTime Updated;
        if(Run.Id.IsValid()&&(Run.PlayerNation==EFRRUNationV81::French||Run.PlayerNation==EFRRUNationV81::Russian)
            &&FDateTime::ParseIso8601(*Run.UpdatedUtc,Updated)&&Updated>=Latest)
        {Latest=Updated;Result=Run.PlayerNation;}
    }
    return Result;
}
EFrontlineFaction ResolveDisplayFaction(const AWarfareFrontlineGameMode* Battle,bool bHome)
{
    if(!Battle)return EFrontlineFaction::British;
    const auto* Active=Battle->GetCurrentCampaign();
    return ResolvePolicyV79(Battle->GetCampaignRuns(),Active?Active->Id:FGuid(),
        Battle->GetPlayerFaction(),Battle->IsProgressionReadOnly(),bHome);
}

void RunPolicyChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    auto Test=[&](const TCHAR* Name,bool Passed){Check(FString(TEXT("v79_display_faction_"))+Name,Passed);};
    auto Run=[](uint32 Key,EFrontlineFaction Faction,const TCHAR* Updated)
    {
        FFrontlineCampaignProgress Item;Item.Id=FGuid(Key,0,0,1);Item.Faction=Faction;
        Item.CreatedUtc=TEXT("2026-09-01T00:00:00Z");Item.UpdatedUtc=Updated;return Item;
    };
    const auto British=Run(1,EFrontlineFaction::British,TEXT("2026-09-29T15:00:00Z"));
    const auto German=Run(2,EFrontlineFaction::German,TEXT("2026-10-01T10:00:00Z"));
    const TArray<FFrontlineCampaignProgress> Empty,Archives={German,British};
    Test(TEXT("null_defaults_british"),ResolveDisplayFaction(nullptr,true)==EFrontlineFaction::British);
    Test(TEXT("empty_home_ignores_unassigned_german"),ResolvePolicyV79(Empty,FGuid(),EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
    Test(TEXT("read_failure_defaults_british"),ResolvePolicyV79(Archives,German.Id,EFrontlineFaction::German,true,true)==EFrontlineFaction::British);
    Test(TEXT("read_failure_campaign_defaults_british"),ResolvePolicyV79(Archives,German.Id,EFrontlineFaction::German,true,false)==EFrontlineFaction::British);
    Test(TEXT("latest_german_home"),ResolvePolicyV79(Archives,British.Id,EFrontlineFaction::British,false,true)==EFrontlineFaction::German);
    Test(TEXT("current_british_campaign"),ResolvePolicyV79(Archives,British.Id,EFrontlineFaction::German,false,false)==EFrontlineFaction::British);
    Test(TEXT("selected_german_without_campaign"),ResolvePolicyV79(Empty,FGuid(),EFrontlineFaction::German,false,false)==EFrontlineFaction::German);
    Test(TEXT("invalid_selected_defaults_british"),ResolvePolicyV79(Empty,FGuid(),EFrontlineFaction(255),false,false)==EFrontlineFaction::British);
    auto Invalid=German;Invalid.UpdatedUtc=TEXT("unreadable");
    Test(TEXT("invalid_timestamp_ignored"),ResolvePolicyV79({British,Invalid},FGuid(),EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
    Invalid=German;Invalid.Id.Invalidate();
    Test(TEXT("invalid_campaign_id_ignored"),ResolvePolicyV79({British,Invalid},FGuid(),EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
    Invalid=German;Invalid.Faction=EFrontlineFaction(255);
    Test(TEXT("invalid_campaign_faction_ignored"),ResolvePolicyV79({British,Invalid},FGuid(),EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
    Invalid=German;Invalid.UpdatedUtc=TEXT("2026-08-01T00:00:00Z");
    Test(TEXT("update_before_creation_ignored"),ResolvePolicyV79({British,Invalid},FGuid(),EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
    const auto LaterBritish=Run(3,EFrontlineFaction::British,TEXT("2026-10-01T11:00:00Z"));
    Test(TEXT("latest_british_replaces_german"),ResolvePolicyV79({LaterBritish,German},German.Id,EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
    const auto OffsetGerman=Run(4,EFrontlineFaction::German,TEXT("2026-10-01T12:00:00+02:00"));
    Test(TEXT("chronology_uses_utc_not_lexical_text"),ResolvePolicyV79({LaterBritish,OffsetGerman},FGuid(),EFrontlineFaction::German,false,true)==EFrontlineFaction::British);
}
}
